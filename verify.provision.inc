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

    drush_log(dt("CiviCRM: about to run _provision_civicrm_do_site_move()"));
    // civicrm_config_update();
    _provision_civicrm_do_site_move();
    drush_log(dt("CiviCRM: finished running _provision_civicrm_do_site_move"));

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
 *
 * Code forked from the CiviCRM drush module
 * Adapted to check the upgrade result and support multiple CiviCRM versions.
 */
function _provision_civicrm_upgradedb() {
  drush_log(dt("CiviCRM: in _provision_civicrm_upgradedb(), calling _civicrm_init()"));

  if (! defined('CIVICRM_UPGRADE_ACTIVE')) { 
    define('CIVICRM_UPGRADE_ACTIVE', 1); 
  }
  _civicrm_init();

  drush_log(dt("CiviCRM: in _provision_civicrm_upgradedb(), _civicrm_init() worked, setting up environment."));

  $_POST['upgrade'] = 1;
  $_GET['q'] = 'civicrm/upgrade';
  require_once 'CRM/Core/Config.php';
  require_once 'CRM/Core/Smarty.php';

  // log in a sub-directory specific for drush otherwise we may have permission
  // issues on the log file (if it has been created by the web server).
  $config = CRM_Core_Config::singleton( );

  $config->configAndLogDir = $config->configAndLogDir . 'drush/';

  $exists = provision_file()->exists($config->configAndLogDir)
    ->succeed('CiviCRM: @path exists.')
    ->status();

  if (! $exists) {
    provision_file()->mkdir($config->configAndLogDir)
      ->succeed('CiviCRM: log path @path has been created.')
      ->fail('CiviCRM: log path @path could not be created.', 'DRUSH_PERM_ERROR')
      ->status();
  }

  $config->templateCompileDir = $config->templateCompileDir . 'drush/';

  $exists = provision_file()->exists($config->templateCompileDir)
    ->succeed('CiviCRM: @path exists.')
    ->status();

  if (! $exists) {
    $exists = provision_file()->mkdir($config->templateCompileDir)
      ->succeed('CiviCRM: compiled template @path has been created.')
      ->fail('CiviCRM: compiled templace @path could not be created.', 'DRUSH_PERM_ERROR')
      ->status();
  }

  $template =& CRM_Core_Smarty::singleton( );

  drush_log(dt("CiviCRM: in _provision_civicrm_upgradedb(), _civicrm_init() worked, comparing code and schema versions."));

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

    if ($codeVer >= 4.1) {
      $upgrade = new CRM_Upgrade_Page_Upgrade( );
      $upgrade->setPrint(true);
      ob_start(); // to suppress html output /w source code.
      $upgrade->run( );
      $result = $template->get_template_vars('message'); // capture the required message.
    }
    else {
      $upgrade =& new CRM_Upgrade_Page_Upgrade( );
      ob_start(); // to suppress html output /w source code.
      $upgrade->run( );
      $result = $template->get_template_vars('message'); // capture the required message.
      ob_end_clean();
    }

    if (preg_match('/' . ts('CiviCRM upgrade was successful.') . '/', $result)) {
      drush_log(dt("CiviCRM: @result", array('@result' => $result)), 'ok');
    } else {
      drush_log(dt("CiviCRM: upgrade failed: @result", array('@result' => $result)), 'error');
    }
  } else {
    drush_log(dt("CiviCRM: no upgrade required (@codeVer = @dbVer)", array('@codeVer' => $codeVer, '@dbVer' => $dbVer)), 'ok');
  }

  drush_log(dt("CiviCRM: finished _provision_civicrm_upgradedb()"));
}

/**
 * EXPERIMENTAL AND EVIL: forking the code from civicrm_update_config()
 * and CRM/Core/BAO/Setting.php doSiteMove() because doSiteMove() in
 * 4.1.1 doSiteMove() flushes the cache in a brutal way, especially on
 * tables that do not yet exist.
 *
 * We need to propose a cleaner fix to CiviCRM core so that we do not
 * need to maintain this type of forked code. TODO FIXME
 */
function _provision_civicrm_do_site_move($defaultValues = array()) {
  // See CRM_Core_Config::isUpgradeMode() for more information
  if (! defined('CIVICRM_UPGRADE_ACTIVE')) {
    define('CIVICRM_UPGRADE_ACTIVE', 1);
  }

  //
  // from drush civicrm_config_update()
  //
  _civicrm_init();

  $defaultValues = array( );
  $states = array( 'old', 'new' );
  for ( $i = 1 ; $i <= 3; $i++ ) {
    foreach ( $states as $state ) {
      $name = "{$state}Val_{$i}";
      $value = drush_get_option( $name, null );
      if ( $value ) {
        $defaultValues[$name] = $value;
      }
    }
  }

  require_once 'CRM/Core/I18n.php';

  //
  // from doSiteMove
  //

  $moveStatus = ts('Beginning site move process...') . '<br />';

  // get the current and guessed values
  list($oldURL, $oldDir, $oldSiteName, $oldSiteRoot) = provision_civicrm_get_config_settings();
  list($newURL, $newDir, $newSiteName, $newSiteRoot) = provision_civicrm_get_best_guess_settings();

  require_once 'CRM/Utils/Request.php';

  // retrieve these values from the argument list
  $variables = array( 'URL', 'Dir', 'SiteName', 'SiteRoot', 'Val_1', 'Val_2', 'Val_3' );
  $states     = array( 'old', 'new' );
  foreach ( $variables as $varSuffix ) {
      foreach ( $states as $state ) {
          $var = "{$state}{$varSuffix}";
          if ( ! isset( $$var ) ) {
              if ( isset( $defaultValues[$var] ) ) {
                  $$var = $defaultValues[$var];
              } else {
                  $$var = null;
              }
          }
          $$var = CRM_Utils_Request::retrieve( $var,
                                               'String',
                                               CRM_Core_DAO::$_nullArray,
                                               false,
                                               $$var,
                                               'REQUEST' );
      }
  }

  $from = $to = array( );
  foreach ( $variables as $varSuffix ) {
      $oldVar = "old{$varSuffix}";
      $newVar = "new{$varSuffix}";
      //skip it if either is empty or both are exactly the same
      if ( $$oldVar &&
           $$newVar &&
           $$oldVar != $$newVar ) {
          $from[]  = $$oldVar;
          $to[]    = $$newVar;
      }
  }

  $sql = "
SELECT config_backend
FROM   civicrm_domain
WHERE  id = %1
";
  $params = array( 1 => array( CRM_Core_Config::domainID( ), 'Integer' ) );
  $configBackend = CRM_Core_DAO::singleValueQuery( $sql, $params );
  if ( ! $configBackend ) {
      CRM_Core_Error::fatal( ts('Returning early due to unexpected error - civicrm_domain.config_backend column value is NULL. Try visiting CiviCRM Home page.') );
  }
  $configBackend = unserialize( $configBackend );

  $configBackend = str_replace( $from,
                                $to  ,
                                $configBackend );

  $configBackend = serialize( $configBackend );
  $sql = "
UPDATE civicrm_domain
SET    config_backend = %2
WHERE  id = %1
";
  $params[2] = array( $configBackend, 'String' );
  CRM_Core_DAO::executeQuery( $sql, $params );

  // Apply the changes to civicrm_option_values
  $optionGroups = array('url_preferences', 'directory_preferences');
  foreach ($optionGroups as $option) {
      foreach ( $variables as $varSuffix ) {
          $oldVar = "old{$varSuffix}";
          $newVar = "new{$varSuffix}";

          $from = $$oldVar;
          $to   = $$newVar;

          if ($from && $to && $from != $to) {
              $sql = '
UPDATE civicrm_option_value
SET    value = REPLACE(value, %1, %2)
WHERE  option_group_id = (
SELECT id
FROM   civicrm_option_group
WHERE  name = %3 )
';
              $params = array( 1 => array ( $from, 'String' ),
                               2 => array ($to, 'String'),
                               3 => array($option, 'String') );
              CRM_Core_DAO::executeQuery( $sql, $params );
          }
      }
  }

  $config =& CRM_Core_Config::singleton( );

  // clear the template_c and upload directory also
  $config->cleanup( 3, true );

  return $moveStatus;
}

