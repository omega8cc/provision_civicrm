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

    $sitekey = md5(uniqid( '', true ) . $db_user);

    drush_set_option('sitekey', $sitekey);
    drush_set_option('civicrm_sitekey', $sitekey, 'site');

    // Generates the civicrm.settings.php
    if (drush_civicrm_install_validate()) {
      drush_civicrm_install();
    }

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
    $this_module = "civicrm";
    drush_log(dt("CiviCRM: Enabling CiviCRM Module."), 'ok');
    $module = _provision_civicrm_get_package_info();
    if (drush_drupal_major_version() >= 7) {
      include_once DRUPAL_ROOT . '/includes/install.inc';
      drupal_load('module', $this_module);
      module_load_install($this_module);
      db_update('system')
        ->fields(array('status' => 1))
        ->condition('type', 'module')
        ->condition('name', $this_module)
        ->execute();
      system_list_reset();
      module_list(TRUE);
      module_implements('', FALSE, TRUE);
      _system_update_bootstrap_status();
      registry_update();
      drupal_get_schema(NULL, TRUE);
      drupal_theme_rebuild();
      entity_info_cache_clear();
      drupal_install_schema($this_module);
      module_invoke($this_module, 'install');
      module_invoke($this_module, 'enable');
    }
    else {
      module_load_install($this_module);
      db_query("UPDATE {system} SET status = %d, throttle = %d WHERE type = '%s' AND name = '%s'", 1, 0, 'module', $this_module);
      drupal_load('module', $this_module);
      module_list(TRUE, FALSE);
      module_implements('', FALSE, TRUE);
      module_invoke($this_module, 'enable');
    }

    drush_log(dt("CiviCRM: Creating CiviCron User."), 'ok');
    _provision_civicrm_create_civicron_user();

    // Fix some paths:
    drush_log(dt("CiviCRM: Fixing Resources URLs."), 'ok');
    _provision_civicrm_fixresourceurls();

    drush_log(dt("CiviCRM: Finished installation."), 'ok');
  }
}

