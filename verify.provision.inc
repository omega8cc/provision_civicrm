<?php

/**
 * implementation of hook_provision_verify
 */
function drush_civicrm_provision_verify($url = null) {
  if (d()->type == 'site') {
    // todo: trigger if civicrm is already enabled and configured on the site
    // load civicrm.settings.php
    // patch db connection information
    // patch paths
    // patch urls
    // empty files/civicrm/templates_c
    // load data from civicrm_domain.backend_config
    // update data with new paths and urls
    // ...save
    // empty civicrm_cache ?
  }
}

/**
 * Keep old values of paths/url so that we can later search and replace
 * in the civicrm_domain table
 */
function drush_civicrm_pre_provision_verify($url = NULL) {
  $packages = drush_get_option('packages', 'platform');

  if (!array_key_exists('civicrm', $packages['modules'])) {
    return;
  }

  $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
  drush_set_option('civicrm_old_path', $drupalRoot);

  $baseUrl = 'http://' . drush_get_option('uri', false);
  drush_set_option('civicrm_old_baseurl', $baseUrl);
}

/**
 * Persist civicrm settings in the drushrc.php
 */
function drush_civicrm_post_provision_verify($url = NULL) {
  if (d()->type == 'site') {
    $packages = drush_get_option('packages', 'platform');

    if (!array_key_exists('civicrm', $packages['modules'])) {
      drush_log(dt("CiviCRM not installed?"));
      return;
    }

    $db_user = drush_get_option('db_user', 'site');
    $db_passwd = drush_get_option('db_passwd', 'site');
    $db_host = drush_get_option('db_host', 'site');
    $db_name = drush_get_option('db_name', 'site');

    // Change the password for the civicron user
    /* [ML] depends on drush 4.x, and will break if cron is configured externally
        and cron user may not exist anyway
    $civicron_password = md5(uniqid(rand(), true));
    drush_set_option('password', $civicron_password);
    drush_user_password('civicron');
    drush_set_option('civicrm_cron_password', $civicron_password, 'site');
    */

    // Check for a site key
    if (! drush_get_option('civicrm_sitekey', NULL)) {
      drush_log(dt("CiviCRM: none found, generating a new site key"), 'ok');
      $sitekey = md5(uniqid( '', true ));
      drush_set_option('civicrm_sitekey', $sitekey, 'site');
    }

    // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    $crmPath = $drupalRoot . '/sites/all/modules/civicrm'; // FIXME
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );

    $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";
    if ( !file_exists($civicrmInstallerHelper) ) {
        drush_log(dt("CiviCRM installer helper file is missing."));
        return;
    }
    require_once "$civicrmInstallerHelper";

    _civicrm_generate_settings_file($db_user, $db_passwd, $db_host, $db_name, drush_get_option('civicrm_sitekey', NULL));
    drush_log(dt("CiviCRM: Generated config civicrm.settings.php file"));

    drush_set_option('oldVal_1', drush_get_option('civicrm_old_path'));
    drush_set_option('newVal_1', drush_get_context('DRUSH_DRUPAL_ROOT'));

    drush_set_option('oldVal_2', drush_get_option('civicrm_old_baseurl'));
    drush_set_option('newVal_2', 'http://' . drush_get_option('uri'));

    civicrm_config_update();
    drush_log(dt("CiviCRM: domain and directory paths updated"));

    drush_set_option('site_civicrm', TRUE, 'site');
    // drush_print(print_r(d()->service('civicrm'), 1));

    // Make sure the template_c has the correct permissions
    $templates_compile_dir = d()->site_path . '/files/civicrm/templates_c';
    drush_log(dt('dir = ' . $templates_compile_dir));

    provision_file()->chmod($templates_compile_dir, 0770)
      ->succeed('Changed permissions of @path to @perm')
      ->fail('Could not change permissions <code>@path to @perm')
      ->status();

    provision_file()->chgrp($templates_compile_dir, d('@server_master')->web_group, TRUE)
      ->succeed('Change group ownership @path to @perm')
      ->fail('Could not change group ownership of @path to @perm')
      ->status();

    civicrm_cache_clear();
  }
}
