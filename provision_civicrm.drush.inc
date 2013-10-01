<?php

/**
 * @file
 *   Aegir service autoloading function and switch based on version and include
 *   the appropriate files to handle install, verify, &c.
 */

/**
 * Implements hook_drush_init().
 */
function provision_civicrm_drush_init() {
  // Register our service classes for autoloading.
  civicrm_provision_register_autoload();

  // Decide which version of our code to run based on the version of CiviCRM
  // that is installed (provision_civicrm >= 6.x-2.x)
  $civicrm_version = drush_get_option('civicrm_version', NULL);
  $major_version = substr($civicrm_version, 0, 3);
  if ($major_version >= 4.2) {
    // Other command files are automatically included from here:
    include_once('v2/provision_civicrm.inc');
  }
  else {
    // Older civicrm, or $civicrm_version is not set in our site context
    include_once('v1/provision_civicrm.inc');
  }

  if (_provision_civicrm_is_site_context()) {
    // if other modules use PEAR (ex: print_mail), it may cause a PHP conflict
    // so declare our own include path here. See: #1431874
    $crmPath = _provision_civicrm_get_package_path();
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );

    // _provision_civicrm_check_upgrade_mode();
  }

}

/**
 * Register our directory as a place to find Provision classes.
 *
 * This allows Provision to autoload our classes, so that we don't need to
 * specifically include the files before we use the class.
 */
function civicrm_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    $list = drush_commandfile_list();
    $provision_dir = dirname($list['provision']);
    include_once($provision_dir . '/provision.inc');
    include_once($provision_dir . '/provision.service.inc');
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

