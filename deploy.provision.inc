<?php

/**
 * @file
 *
 * CiviCRM support module for the Aegir backend.
 *
 * This file contains the deploy operations (mainly for migrate/import tasks).
 */

/**
 * Implements hook_pre_provision_deploy().
 *
 * Keep old values of paths/url so that we can later search and replace
 * in the civicrm_domain table
 */
function drush_civicrm_pre_provision_deploy($url = NULL) {
  if (_provision_civicrm_is_site_context()) {
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    drush_set_option('civicrm_old_path', $drupalRoot);

    $baseUrl = 'http://' . drush_get_option('uri', false);
    drush_set_option('civicrm_old_baseurl', $baseUrl);
  }
}

/**
 * Implements hook_post_provision_deploy().
 */
function drush_civicrm_post_provision_deploy() {
  if (! drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL)) {
    drush_log(dt("CiviCRM: failed to bootstrap Drupal for deploy."));
    return;
  }

  if (! _provision_civicrm_is_site_context()) {
    return;
  }

  drush_log(dt("CiviCRM: Running drush_civicrm_post_provision_deploy"));
  drush_civicrm_post_provision_verify();

  // Run the CiviCRM upgrade procedure
  _provision_civicrm_upgradedb();

  // Verify will run later on and do fixpermissions and cache clear
  // so we won't do it again here. However, people running a deploy
  // manually (from the command line) will have to also run verify after.

  drush_log(dt("CiviCRM: Finished drush_civicrm_post_provision_deploy"));
}

