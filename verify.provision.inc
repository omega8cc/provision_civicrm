<?php

/**
 * @file
 *
 * CiviCRM support module for the Aegir backend.
 *
 * This file contains the verify operations.
 */


/**
 * Implements hook_pre_provision_verify().
 *
 * Keep old values of paths/url so that we can later search and replace
 * in the civicrm_domain table
 */
function drush_civicrm_pre_provision_verify($url = NULL) {
  if (_provision_civicrm_is_site_context()) {
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    drush_set_option('civicrm_old_path', $drupalRoot);

    $baseUrl = 'http://' . drush_get_option('uri', false);
    drush_set_option('civicrm_old_baseurl', $baseUrl);
  }
}

/**
 * Implements hook_post_provision_verify().
 *
 * Persist civicrm settings in the drushrc.php
 */
function drush_civicrm_post_provision_verify($url = NULL) {
  if (! _provision_civicrm_is_site_context()) {
    return;
  }

  drush_include_engine('drupal', 'environment');
  $modules = drush_get_modules();

  if (! $modules['civicrm']->status) {
    drush_log(dt("CiviCRM: not enabled. Skipping verify operations for CiviCRM. You will need to re-verify the site if you enable CiviCRM in the future."));
    return;
  }

  drush_log(dt("CiviCRM: running post verify."));

  // Make sure to write the civicrm options
  $civi_opts = _provision_civicrm_drush_options('post verify');
  foreach ($civi_opts as $opt => $val) {
    drush_set_option($opt, $val, 'site');
  }

  $db_user = drush_get_option('db_user', NULL, 'site');
  $db_passwd = drush_get_option('db_passwd', NULL, 'site');
  $db_host = drush_get_option('db_host', NULL, 'site');
  $db_name = drush_get_option('db_name', NULL, 'site');

  // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
  $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
  $crmPath = _provision_civicrm_get_package_path();

  $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";

  if (! file_exists($civicrmInstallerHelper) ) {
    drush_log(dt("CiviCRM installer helper file is missing."));
    return;
  }

  require_once "$civicrmInstallerHelper";
  _civicrm_generate_settings_file($db_user, $db_passwd, $db_host, $db_name, drush_get_option('civicrm_sitekey', NULL));
  drush_log(dt("CiviCRM: Generated config civicrm.settings.php file"), 'ok');

  // Set file acls on civicrm.settings.php, if necessary
  if (function_exists('provisionacl_set_acl')) {
    $group = d()->client_name;
    $file = d()->site_path . '/' . 'civicrm.settings.php';
    drush_log(dt('CiviCRM: Attempting to set acls for @group on @file', array('@group' => $group, '@file' => $file)));

    // Check the group
    if (!provision_posix_groupname($group)) {
      drush_log(dt('CiviCRM: not setting acls on @file for non-existent group @group', array('@file' => $file, '@group' => $group)), 'warning');
    }
    // Check the file
    else if (! file_exists($file)) {
      drush_log(dt('@file not found, acls not configured', array('@file' => $file)), 'warning');
    }
    else {
      provisionacl_set_acl('group', $group, 'r--', array($file), TRUE);
    }
  }

  drush_set_option('oldVal_1', drush_get_option('civicrm_old_path'));
  drush_set_option('newVal_1', drush_get_context('DRUSH_DRUPAL_ROOT'));

  drush_set_option('oldVal_2', drush_get_option('civicrm_old_baseurl'));
  drush_set_option('newVal_2', 'http://' . drush_get_option('uri'));

  drush_log(dt("CiviCRM: about to run _provision_civicrm_do_site_move()"));
  _provision_civicrm_do_site_move();
  drush_log(dt("CiviCRM: finished running _provision_civicrm_do_site_move"));

  // Update Resource URLs
  _provision_civicrm_fixresourceurls();

  drush_set_option('site_civicrm', TRUE, 'site');
  // drush_print(print_r(d()->service('civicrm'), 1));

  // Make sure the template_c and uploads have the correct permissions
  _provision_civicrm_fixpermissions();

  // Run the CiviCRM upgrade procedure
  _provision_civicrm_upgradedb();

  // Run again because the upgrade generates some template cache (d.o #1201290)
  // In general we had so many permissions issues, we want to be extra-pedantic.
  _provision_civicrm_fixpermissions();

  drush_civicrm_cacheclear();
}

