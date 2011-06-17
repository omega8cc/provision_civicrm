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
  $packages = drush_get_option('packages', array(), 'platform');

  if (empty($packages) || !array_key_exists('civicrm', $packages['modules'])) {
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
    $packages = drush_get_option('packages', array(), 'platform');

    if (!array_key_exists('civicrm', $packages['modules'])) {
      drush_log(dt("CiviCRM not installed."));
      return;
    }

    $db_user = drush_get_option('db_user', 'site');
    $db_passwd = drush_get_option('db_passwd', 'site');
    $db_host = drush_get_option('db_host', 'site');
    $db_name = drush_get_option('db_name', 'site');

    // Create the civicron user if it does not exist
    if (drush_drupal_major_version() >= 7) {
      $result = db_query("SELECT uid FROM {users} WHERE name = :name", array(':name' => 'civicron'));
    }
    else {
      $result = db_query("SELECT uid FROM {users} WHERE name = '%s'", 'civicron');
    }

    if (drush_db_result($result) === FALSE) {
      drush_log(dt("CiviCRM: civicron 
      _provision_civicrm_create_civicron_user();
    }
/* [ML] do not reset the password on verify, it will break external crons
    else {
      $civicron_password = md5(uniqid(rand(), true));
      drush_set_option('password', $civicron_password);
      drush_user_password('civicron');
      drush_set_option('civicrm_cron_password', $civicron_password, 'site');
    }
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

    // Make sure the template_c and uploads have the correct permissions
    $cividirs = array(
      '/files/civicrm/templates_c', 
      '/files/civicrm/upload',
      '/files/civicrm/persist',
    );

    foreach ($cividirs as $dir) {
      $templates_compile_dir = d()->site_path . $dir;
  
      provision_file()->chmod($templates_compile_dir, 0770)
        ->succeed('Changed permissions of @path to @perm')
        ->fail('Could not change permissions <code>@path to @perm')
        ->status();
  
      provision_file()->chgrp($templates_compile_dir, d('@server_master')->web_group, TRUE)
        ->succeed('Change group ownership @path to @perm')
        ->fail('Could not change group ownership of @path to @perm')
        ->status();
    }

    _provivision_civicrm_upgradedb();

    // Fix permissions on civicrm.settings.php
    provision_file()->chmod(d()->site_path . '/civicrm.settings.php', 0640)
      ->succeed('Changed permissions of @path to @perm')
      ->fail('Could not change permissions <code>@path to @perm')
      ->status();

    provision_file()->chgrp(d()->site_path . '/civicrm.settings.php', d('@server_master')->web_group, TRUE)
      ->succeed('Change group ownership @path to @perm')
      ->fail('Could not change group ownership of @path to @perm')
      ->status();

    civicrm_cache_clear();
  }
}

/**
 * CiviCRM database upgrade
 * Code duplicated from the CiviCRM drush module, but adapted
 * so that is it easier to check the result.
 */
function _provivision_civicrm_upgradedb() {
  _civicrm_init();

  $_POST['upgrade'] = 1;
  $_GET['q'] = 'civicrm/upgrade';
  require_once 'CRM/Core/Config.php';
  require_once 'CRM/Core/Smarty.php';
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
      drush_log(dt("CiviCRM: " . $result));
    } else {
      drush_set_error(dt("CiviCRM: upgrade failed: " . $result));
    }
  } else {
    drush_log(dt("CiviCRM: no upgrade required ($codeVer = $dbVer)."));
  }
}

