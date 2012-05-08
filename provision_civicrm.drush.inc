<?php

/**
 * @file
 * Hookup with drush
 * Common functions used by install and verify.
 */

function provision_civicrm_drush_init() {
  if (_provision_civicrm_is_site_context()) {
    // if other modules use PEAR (ex: print_mail), it may cause a PHP conflict
    // so declare our own include path here. See: #1431874
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    $crmPath = _provision_civicrm_get_package_path();
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );
  }
}

/**
 * Create a civicron user and role
 */
function _provision_civicrm_create_civicron_user() {
  if (! function_exists("drush_user_create")) {
    drush_log(dt("Not running drush 4.x or above, so cannot create civicron user and role. If you want to run the CiviCRM crons using drush, you will have to create them manually. Once you have done so, you can edit the site's sites/example.org/drushrc.php and set the correct username and password for the cron."));
    drush_set_option('civicrm_cron_username', 'civicron', 'site');
    drush_set_option('civicrm_cron_password', NULL, 'site');
    return FALSE;
  }

  $url = drush_get_option('uri', false);

  $civicron_password = provision_password();

  drush_set_option('mail', 'civicrm-no-reply@' . $url);
  drush_set_option('password', $civicron_password);
  drush_user_create('civicron');

  drush_set_option('civicrm_cron_username', 'civicron', 'site');
  drush_set_option('civicrm_cron_password', $civicron_password, 'site');

  // Create civicron role
  if (function_exists('user_role_save')) {
    // D7
    $civicron_role = new stdClass;
    $civicron_role->name = 'civicron';
    user_role_save($civicron_role);
  }
  else {
    db_query("INSERT INTO {role} (name) VALUES ('%s')", array('civicron'));
  }

  drush_user_add_role('civicron', 'civicron');

  // Assign permissions to civicron role
  $all_roles = user_roles(); // effectively same for D6/7
  $civicron_rid = array_search('civicron', $all_roles);

  if (function_exists('user_role_grant_permissions')) {
    // D7
    $all_perms = user_permission_get_modules();
    $civi_perms = array_keys($all_perms, 'civicrm'); // todo: this is just the main civicrm module
    user_role_grant_permissions($civicron_rid, $civi_perms);
  }
  else {
    $civicrm_permissions = 'access CiviCRM, access CiviContribute, access CiviEvent, access CiviMail, access CiviMail subscribe/unsubscribe pages, access CiviMember, access CiviPledge, access CiviReport, access Contact Dashboard, access Report Criteria, access all custom data, add contacts, edit all contacts, edit all events, edit contributions, edit event participants, view all activities, view all contacts, view all notes';
    db_query("INSERT INTO {permission} (rid, perm) VALUES (%d, '%s')", array($civicron_rid, $civicrm_permissions));
  }

  return TRUE;
}


/**
 * Fix permissions on all CiviCRM writable directories
 */
function _provision_civicrm_fixpermissions() {
  drush_log(dt("CiviCRM: in _provision_civicrm_fixpermissions()"));

  // Make sure the template_c and uploads have the correct permissions
  $civipaths = array(
    '/files/civicrm/templates_c', 
    '/files/civicrm/upload',
    '/files/civicrm/persist',
  );

  foreach ($civipaths as $p) {
    $fullpath = d()->site_path . $p;

    // we setgid directories, avoids a few permissions issues
    // where files are aegir.www-data.
    provision_file()->chmod($fullpath, 02770, TRUE)
      ->succeed('Changed permissions of @path to @perm')
      ->fail('Could not change permissions <code>@path to @perm')
      ->status();

    provision_file()->chgrp($fullpath, d('@server_master')->web_group, TRUE)
      ->succeed('Changed group ownership @path to @gid')
      ->fail('Could not change group ownership of @path to @gid')
      ->status();
  }

  // Fix permissions on civicrm.settings.php
  provision_file()->chmod(d()->site_path . '/civicrm.settings.php', 0640)
    ->succeed('Changed permissions of @path to @perm')
    ->fail('Could not change permissions <code>@path to @perm')
    ->status();

  provision_file()->chgrp(d()->site_path . '/civicrm.settings.php', d('@server_master')->web_group, TRUE)
    ->succeed('Changed group ownership @path to @gid')
    ->fail('Could not change group ownership of @path to @gid')
    ->status();

  // Remove the Config.IDS.ini file (d.o#1226668)
  // Usually 'verify' will call civicrm_cache_clear() which will
  // delete most of the contents of template_c, but there have been
  // some hard to trace bugs, so we are not taking any chances.
  require_once 'CRM/Core/Config.php';
  $config = CRM_Core_Config::singleton( );
  $configFile = $config->configAndLogDir . 'Config.IDS.ini';

  if (provision_file()->exists($configFile)->status()) {
    provision_file()->unlink($configFile)
      ->succeed('CiviCRM: Removing stale @path')
      ->fail('CiviCRM: Could not remove @path')
      ->status();
  }

  drush_log(dt("CiviCRM: finished _provision_civicrm_fixpermissions()"));
}

/**
 * Update the CiviCRM Resource URLs
 * There must be a much simpler/cleaner way of doing this..
 */
function _provision_civicrm_fixresourceurls() {
  $civicrm_modulepath = _provision_civicrm_get_package_path();

  if (! $civicrm_modulepath) {
    drush_log(dt('CiviCRM: could not detect the path for CiviCRM. This should not happen.'), 'warning');
    return;
  }

  drush_log(dt("CiviCRM: in _provision_civicrm_fixresourceurls()"));

  // ex: /var/aegir/platforms/example-civicrm-3.4.x/ (for strlen below)
  $drupal_basepath = drush_get_context('DRUSH_DRUPAL_ROOT');

  // Remove the part up until the Drupal root, so that we have only the part exposed to the web
  $civicrm_modulepath = substr($civicrm_modulepath, strlen($drupal_basepath) + 1);

  // force a full URL otherwise there may be i18n issues, ex: http://example.org/en/sites/all/modules/civicrm
  $civicrm_resourceurl = 'http://' . drush_get_option('uri') . '/' . $civicrm_modulepath;

  _provision_civicrm_update_option_value('userFrameworkResourceURL', $civicrm_resourceurl);

  // Image Upload URL
  $civicrm_imageurl = 'http://' . drush_get_option('uri') . '/sites/' . drush_get_option('uri') . '/files/civicrm/persist/contribute/';
  _provision_civicrm_update_option_value('imageUploadURL', $civicrm_imageurl);

  drush_log(dt("CiviCRM: finished running _provision_civicrm_fixresourceurls()"));
}

/**
 * Wrapper to update values in {civicrm_option_value}
 */
function _provision_civicrm_update_option_value($key, $val) {
  switch (drush_drupal_major_version()) {
    case 6:
      $oldval = drush_db_result(db_query("SELECT value FROM {civicrm_option_value} WHERE name = '%s'", $key));
      drush_log(dt('CiviCRM: before UPDATE: old val for @key = @oldval', array('@key' => $key, '@oldval' => $oldval)));

      db_query("UPDATE {civicrm_option_value} set value = '%s' where name = '%s'", array($val, $key));
      drush_log(dt("CiviCRM: updated @key = @val", array('@key' => check_plain($key), '@val' => check_plain($val))));
      break;
    case 7:
      $oldval = drush_db_result(db_query("SELECT value FROM {civicrm_option_value} WHERE name = :key", array(':key' => $key)));
      drush_log(dt('CiviCRM: before UPDATE: old val for @key = @oldval', array('@key' => $key, '@oldval' => $oldval)));

      db_query("UPDATE {civicrm_option_value} set value = :val where name = :key", array(':val' => $val, ':key' => $key));
      drush_log(dt("CiviCRM: updated @key = @val", array('@key' => check_plain($key), '@val' => check_plain($val))));
      break;
  }
}

/**
 * Get an array of civicrm related options
 *
 * @param $op
 *   A string describing the task being run, for debugging purposes
 */
function _provision_civicrm_drush_options($op = 'unknown') {
  drush_log(dt("CiviCRM: In @op: Fetching CiviCRM Drush options", array('@op' => $op)));

  $civi_opts = array(
    'civicrm_cron_username' => '',
    'civicrm_cron_password' => '',
    'civicrm_sitekey' => '',
  );

  foreach ($civi_opts as $opt => $val) {
    $civi_opts[$opt] = drush_get_option($opt, NULL);

    /* Debugging code follows, comment out this line to enable it.
    if ($civi_opts[$opt] == NULL) {
      drush_set_error('PROVISION_CIVICRM_DRUSH_OPTIONS', "In $op: Failed to get valid $opt");
    }
    $dump = var_export($civi_opts[$opt], true);
    drush_log(dt("In @op: $dump retrieved for @opt", array('@op' => $op, '@opt' => $opt)), 'warning');
    //*/
  }

  return $civi_opts;
}

/**
 * Check we are in a civicrm site context
 */
function _provision_civicrm_is_site_context($op = 'unkown') {
  if (d()->type == 'site') {
    $crmpath = _provision_civicrm_get_package_path();
    return ($crmpath ? TRUE : FALSE);
  }
  return FALSE;
}

/**
 * Get the path where the CiviCRM module is installed
 * Ex: /var/aegir/platforms/[...]/sites/all/modules/civicrm
 */
function _provision_civicrm_get_package_path() {
  $module = _provision_civicrm_get_package_info();

  if ($module) {
    // Get the parent directory of the module, which is in civicrm/drupal/civicrm.module
    $crmpath = dirname(dirname($module['filename']));
    drush_log(dt('CiviCRM: civicrm is in @path', array('@path' => $crmpath)));
    return $crmpath;
  }

  return FALSE;
}

/**
 * Get the CiviCRM module base information (filename, basename, version, etc)
 */
function _provision_civicrm_get_package_info() {
  $module = NULL;

  // Check if the CiviCRM code base is present in the platform
  // Note: after putting the code there, you need to verify
  // the platform.
  $packages = drush_get_option('packages', array('modules' => NULL));
  $profile = drush_get_option('profile', NULL);

  if (isset($packages['modules']) && isset($packages['modules']['civicrm'])) {
    drush_log(dt('CiviCRM: found civicrm in packages'));
    $module = $packages['modules']['civicrm'];
  }
  elseif (isset($packages['sites-all']) && isset($packages['sites-all']['modules']) && isset($packages['sites-all']['modules']['civicrm'])) {
    $module = $packages['sites-all']['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of sites-all modules'));
  }
  elseif (isset($packages['base']) && isset($packages['base']['modules']) && isset($packages['base']['modules']['civicrm'])) {
    $module = $packages['base']['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of base modules'));
  }
  elseif ($profile && isset($packages['profiles'][$profile]['modules']) && isset($packages['profiles'][$profile]['modules']['civicrm'])) {
    $module = $packages['profiles'][$profile]['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of @profile profile modules', array('@profile' => $profile)));
  }
  else {
    // drush_log(dt('CiviCRM: civicrm not found in packages ' . print_r($packages, 1)));
  }

  // drush_log('CiviCRM: MODULE = ' . print_r($module, 1));

  return $module;
}

/**
 * Wrapper function for getConfigSettings (class location changed in 4.1)
 */
function provision_civicrm_get_config_settings() {
  require_once 'CRM/Utils/System.php';
  $codeVer = CRM_Utils_System::version();

  if (substr($codeVer, 0, 3) >= '4.1') {
    require_once 'CRM/Core/BAO/ConfigSetting.php';
    return CRM_Core_BAO_ConfigSetting::getConfigSettings();
  }

  // else
  require_once 'CRM/Core/BAO/Setting.php';
  return CRM_Core_BAO_Setting::getConfigSettings();
}

/**
 * Wrapper function for getBestGuessSettings (class location changed in 4.1)
 */
function provision_civicrm_get_best_guess_settings() {
  require_once 'CRM/Utils/System.php';
  $codeVer = CRM_Utils_System::version();

  if (substr($codeVer, 0, 3) >= '4.1') {
    require_once 'CRM/Core/BAO/ConfigSetting.php';
    return CRM_Core_BAO_ConfigSetting::getBestGuessSettings();
  }

  // else
  require_once 'CRM/Core/BAO/Setting.php';
  return CRM_Core_BAO_Setting::getBestGuessSettings();
}

