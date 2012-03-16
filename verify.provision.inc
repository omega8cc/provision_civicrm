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
  if (_provision_civicrm_is_site_context()) {
    drush_include_engine('drupal', 'environment');
    $modules = drush_get_modules();

    if (! $modules['civicrm']->status) {
      drush_log(dt("CiviCRM: not enabled. Skipping verify operations for CiviCRM. You will need to re-verify the site if you enable CiviCRM in the future."));
      return;
    }

    // Make sure to write the civicrm options
    $civi_opts = _provision_civicrm_drush_options('post verify');
    foreach ($civi_opts as $opt => $val) {
      drush_set_option($opt, $val, 'site');
    }

    drush_log(dt("CiviCRM: running post verify."));

    $db_user = drush_get_option('db_user', NULL, 'site');
    $db_passwd = drush_get_option('db_passwd', NULL, 'site');
    $db_host = drush_get_option('db_host', NULL, 'site');
    $db_name = drush_get_option('db_name', NULL, 'site');

    // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    $crmPath = _provision_civicrm_get_package_path();
    /* moved to hook_drush_init()
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );
    */

    $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";
    if ( !file_exists($civicrmInstallerHelper) ) {
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
      drush_log(dt('Attempting to set acls for @group on @file', array('@group' => $group, '@file' => $file)));

      // Check the group
      if (!provision_posix_groupname($group)) {
        drush_log(dt('not setting acls on @file for non-existent group @group', array('@file' => $file, '@group' => $group)), 'warning');
      }

      // Check the file
      else if (!file_exists($file)) {
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

    civicrm_config_update();
    drush_log(dt("CiviCRM: domain and directory paths updated"));

    // Update Resource URLs
    _provision_civicrm_fixresourceurls();

    drush_set_option('site_civicrm', TRUE, 'site');
    // drush_print(print_r(d()->service('civicrm'), 1));

    // Make sure the template_c and uploads have the correct permissions
    _provision_civicrm_fixpermissions();

    _provision_civicrm_upgradedb();

    // Run again because the upgrade generates some template cache (d.o #1201290)
    // In general we had so many permissions issues, we want to be extra-pedantic.
    _provision_civicrm_fixpermissions();

    civicrm_cache_clear();
  }
}

/**
 * CiviCRM database upgrade
 * Code duplicated from the CiviCRM drush module, but adapted
 * so that is it easier to check the result.
 */
function _provision_civicrm_upgradedb() {
  drush_log(dt("CiviCRM: in _provision_civicrm_upgradedb()"));

  _civicrm_init();

  $_POST['upgrade'] = 1;
  $_GET['q'] = 'civicrm/upgrade';
  require_once 'CRM/Core/Config.php';
  require_once 'CRM/Core/Smarty.php';

  // log in a sub-directory specific for drush otherwise we may have permission
  // issues on the log file (if it has been created by the web server).
  $config = CRM_Core_Config::singleton( );
  $config->configAndLogDir = $config->configAndLogDir . 'drush/';
  $config->templateCompileDir = $config->templateCompileDir . 'drush/';

  $template =& CRM_Core_Smarty::singleton( );

  require_once 'CRM/Utils/System.php';
  require_once 'CRM/Core/BAO/Domain.php';
  $codeVer = CRM_Utils_System::version();
  $dbVer   = CRM_Core_BAO_Domain::version();

  if ( !$dbVer ) {
    drush_set_error('CiviCRM: Version information missing in civicrm database.');
  } else if ( stripos($dbVer, 'upgrade') ) {
    drush_set_error('CiviCRM: Database check failed - the database looks to have been partially upgraded. You may want to reload the database with the backup and try the upgrade process again.');
  } else if ( !$codeVer ) {
    drush_set_error('CiviCRM: Version information missing in civicrm codebase.');
  } else if ( version_compare($codeVer, $dbVer) < 0 ) {
    drush_set_error(dt("CiviCRM: Database is marked with an unexpected version '!dbVer' which is higher than that of codebase version '!codeVer'.", array('!dbVer' => $dbVer, '!codeVer' => $codeVer)));
  } else if ( version_compare($codeVer, $dbVer) > 0 ) {
    drush_log(dt("CiviCRM: Starting with v!dbVer -> v!codeVer upgrade ..",
             array('!dbVer' => $dbVer, '!codeVer' => $codeVer)));

    drush_log(dt("CiviCRM: Starting with upgrade .."));

    require_once( 'CRM/Upgrade/Page/Upgrade.php' );
    $upgrade =& new CRM_Upgrade_Page_Upgrade( );
    ob_start(); // to suppress html output /w source code.
    $upgrade->run( );
    $result = $template->get_template_vars('message'); // capture the required message.
    ob_end_clean();

    if (preg_match('/' . ts('CiviCRM upgrade was successful.') . '/', $result)) {
      drush_log(dt("CiviCRM: @result", array('@result' => $result)), 'ok');
    } else {
      drush_log(dt("CiviCRM: upgrade failed: @result", array('@result' => $result)), 'error');
    }
  } else {
    drush_log(dt("CiviCRM: no upgrade required (@codeVer = @dbVer)", array('@codeVer' => $codeVer, '@dbVer' => $dbVer)), 'ok');
  }
}

