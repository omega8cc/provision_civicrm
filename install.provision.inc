<?php

/**
 * implementation of hook_pre_provision_install
 */
function drush_civicrm_pre_provision_install($url = NULL) {
  /*
  if (d()->type == 'site') {
    // if necessary ...
  }
  */
}

/**
 * implementation of hook_post_provision_install
 */
function drush_civicrm_post_provision_install($url = null) {
  if (d()->type == 'site') {
    // Check if the CiviCRM code base is present in the platform
    // Note: after putting the code there, you must verify the platform.
    if (! _provision_civicrm_get_package_path()) {
      // drush_log(dt('CiviCRM: not found in package path, skipping installation.'));
      return;
    }

    // Quietly exit if we're trying to install an old/unsupported CiviCRM
    // Don't completely fail, the user can install manually (for migrations/upgrades).
    $module = _provision_civicrm_get_package_info();
    if ($module['version'] < 3) {
      drush_log(dt('CiviCRM: CiviCRM version is not supported, too old: ' . check_plain($module['version'])));
      return;
    }

    drush_log(dt("CiviCRM: Starting installation..."));

    $db_user = drush_get_option('db_user', NULL, 'site');
    $db_passwd = drush_get_option('db_passwd', NULL, 'site');
    $db_host = drush_get_option('db_host', NULL, 'site');
    $db_name = drush_get_option('db_name', NULL, 'site');

    drush_set_option('dbuser', $db_user);
    drush_set_option('dbpass', $db_passwd);
    drush_set_option('dbhost', $db_host);
    drush_set_option('dbname', $db_name);

    $sitekey = md5(uniqid( '', true ) . $siteRoot);

    drush_set_option('sitekey', $sitekey);
    drush_set_option('civicrm_sitekey', $sitekey, 'site');

    // Generates the civicrm.settings.php
    civicrm_pkg_install();

    // Install the DB, not sure why the drush module is not doing it
    global $crmPath;

    $dsn = "mysql://{$db_user}:{$db_passwd}@{$db_host}/{$db_name}?new_link=true";
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    // $crmPath = $drupalRoot . '/sites/all/modules/civicrm'; // FIXME
    $crmPath = _provision_civicrm_get_package_path();
    $sqlPath = $crmPath . DIRECTORY_SEPARATOR . 'sql';

    // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );

    $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";
    if ( !file_exists($civicrmInstallerHelper) ) {
        drush_log(dt("CiviCRM: installer helper file is missing, searched for @file.", array('@file' => $civicrmInstallerHelper)), 'error');
    }

    require_once "$civicrmInstallerHelper";

    drush_log(dt("CiviCRM: importing @sqlPath civicrm.mysql", array('@sqlPath' => $sqlPath)));
    civicrm_source($dsn, $sqlPath . DIRECTORY_SEPARATOR . 'civicrm.mysql');

    drush_log(dt("CiviCRM: importing @sqlPath civicrm_data.mysql", array('@sqlPath' => $sqlPath)));
    civicrm_source($dsn, $sqlPath . DIRECTORY_SEPARATOR . 'civicrm_data.mysql');

    drush_log(dt("CiviCRM: importing @sqlPath civicrm_acl.mysql", array('@sqlPath' => $sqlPath)));
    civicrm_source($dsn, $sqlPath . DIRECTORY_SEPARATOR . 'civicrm_acl.mysql');

    // Enable civicrm module before setting up civicron role,
    // as we need it to expose its permissions.
    drush_pm_enable('civicrm');

    _provision_civicrm_create_civicron_user();

    // Fix some paths:
    _provision_civicrm_fixresourceurls();
  }
}

