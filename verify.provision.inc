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

  if (d()->type == 'site' /* && drush_get_option('site_civicrm') */) {
    // todo: (phase II) setup civicrm on demand
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    $crmPath = $drupalRoot . '/sites/all/modules/civicrm'; // FIXME
    $db_name = drush_get_option('db_name', 'site');

drush_log(dt("root = $drupalRoot -----------------------------"), 'ok');
drush_log(dt("dbname = $db_name -----------------------------"), 'ok');

  } else {
    drush_log(dt("CiviCRM not installed?"));
  }
}

/**
 * Persist civicrm settings in the drushrc.php
 */
function drush_civicrm_post_provision_verify($url = NULL) {
  if (d()->type == 'site' /* && drush_get_option('site_civicrm') */ ) {
    $db_user = drush_get_option('db_user', 'site');
    $db_passwd = drush_get_option('db_passwd', 'site');
    $db_host = drush_get_option('db_host', 'site');
    $db_name = drush_get_option('db_name', 'site');

    // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    $crmPath = $drupalRoot . '/sites/all/modules/civicrm'; // FIXME
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );
drush_log(dt("root = $drupalRoot -----------------------------"), 'ok');
drush_log(dt("dbname = $db_name -----------------------------"), 'ok');

    $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";
    if ( !file_exists($civicrmInstallerHelper) ) {
        drush_die(dt("CiviCRM installer helper file is missing."));
    }
    require_once "$civicrmInstallerHelper";

    _civicrm_generate_settings_file($db_user, $db_passwd, $db_host, $db_name);
    drush_log(dt("CiviCRM: Generated config civicrm.settings.php file"));

// [ML] XXX i think this is having issues and setting weird values in civicrm_domain
    civicrm_config_update();
    drush_log(dt("CiviCRM: Ran config update to fix domain paths"));

    drush_set_option('site_civicrm', TRUE, 'site');
  } else {
    drush_log(dt("CiviCRM not installed?"));
  }
}
