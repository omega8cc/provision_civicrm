<?php

/**
 * implementation of hook_pre_provision_install
 */
function drush_civicrm_pre_provision_install($url = NULL) {
  /*
  if (PROVISION_CONTEXT_SITE) {
    // if necessary ...
  }
  */
}

/**
 * implementation of hook_provision_install
 */
function drush_civicrm_provision_install($url = null) {
  if (d()->type == 'site') {
    // Check if the CiviCRM code base is present in the platform
    // Note: after putting the code there, you need to verify
    // the platform.
    $packages = drush_get_option('packages', 'platform');

    if (!array_key_exists('civicrm', $packages['base']['modules'])) {
      return;
    }

    drush_log(dt("Configuring site for civicrm support"));

    drush_set_option('dbuser', drush_get_option('db_user', 'site'));
    drush_set_option('dbpass', drush_get_option('db_passwd', 'site'));
    drush_set_option('dbhost', drush_get_option('db_host', 'site'));
    drush_set_option('dbname', drush_get_option('db_name', 'site'));

    civicrm_pkg_install();

    drush_set_option('site_civicrm', TRUE, 'site');
  }
}

/**
 * implementation of hook_provision_install
 */
function drush_civicrm_post_provision_install($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    drush_pm_enable('civicrm');
  }
}
