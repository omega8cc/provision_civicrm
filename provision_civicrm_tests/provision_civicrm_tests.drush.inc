<?php
/**
 * @file
 *  Some tests for provison_civicrm (based on provision's tests).
 */

define('PROVISION_CIVICRM_TESTS_BUILDS_REPO', dirname(__FILE__) . '/makes');

/**
 * Implementation of hook_drush_command().
 */
function provision_civicrm_tests_drush_command() {
  $items['provision-civicrm-tests-run'] = array(
    'description' => dt('Runs provision_civicrm tests'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    // Although we're a provision command, we require hostmaster to be around to
    // run the tests correctly
    'drupal dependencies' => array(
      'hosting',
    ),
  );

  return $items;
}

/**
 * Drush command to run the provision tests.
 */
function drush_provision_civicrm_tests_run() {
  if (version_compare(PHP_VERSION, '5.3.0', '<')) {
    $drupal5_tests = TRUE;
  }
  else {
    $drupal5_tests = FALSE;
    drush_log(dt('Please note that because you are running PHP 5.3 or greater, you cannot test installing Drupal 5 on this machine.'), 'warning');
  }

  if (!drush_confirm(dt('This command should only be run on a clean Aegir install, and data may be lost! Do you want to continue?'))) {
    return drush_user_abort();
  }

  // Disable the tasks queue, we run them manually instead.
  $queue_status_initial = variable_get('hosting_queue_tasks_enabled', '0');
  variable_set('hosting_queue_tasks_enabled', '0');

  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: creating platforms.'), 'ok');
  drush_log(dt('*****'), 'ok');

  // drush_provision_civicrm_tests_install_platform('civicrm34d6');
  // drush_provision_civicrm_tests_install_platform('civicrm40d7');
  // drush_provision_civicrm_tests_install_platform('civicrm41d6');
  // drush_provision_civicrm_tests_install_platform('civicrm41d7');
  drush_provision_civicrm_tests_install_platform('civicrm42d7');
  drush_provision_civicrm_tests_install_platform('civicrm43d7');
  drush_provision_civicrm_tests_install_platform('civicrm44d7');

  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: installing sites.'), 'ok');
  drush_log(dt('*****'), 'ok');

  // Install some sites.
  // drush_provision_civicrm_tests_install_site('civicrm34d6', 'civicrm34d6-default', 'default');
  // drush_provision_civicrm_tests_install_site('civicrm41d6', 'civicrm41d6-default', 'default');
  // drush_provision_civicrm_tests_install_site('civicrm40d7', 'civicrm40d7-standard', 'standard');
  // drush_provision_civicrm_tests_install_site('civicrm41d7', 'civicrm41d7-standard', 'standard');
  drush_provision_civicrm_tests_install_site('civicrm42d7', 'civicrm42d7-standard', 'standard');
  drush_provision_civicrm_tests_install_site('civicrm43d7', 'civicrm43d7-standard', 'standard');
  drush_provision_civicrm_tests_install_site('civicrm44d7', 'civicrm44d7-standard', 'standard');

  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: removing sites.'), 'ok');
  drush_log(dt('*****'), 'ok');

  // Remove the sites.
  // drush_provision_civicrm_tests_remove_site('civicrm34d6-default');
  // drush_provision_civicrm_tests_remove_site('civicrm41d6-default');
  // drush_provision_civicrm_tests_remove_site('civicrm40d7-standard');
  // drush_provision_civicrm_tests_remove_site('civicrm41d7-standard');
  drush_provision_civicrm_tests_remove_site('civicrm42d7-standard');
  drush_provision_civicrm_tests_remove_site('civicrm43d7-standard');
  drush_provision_civicrm_tests_remove_site('civicrm44d7-standard');

  // Create some sites and migrate them.
  // Test migrate from 4.2-D7 to 4.2-D7
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.2-D7 to 4.2-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_platform('civicrm42d7', 'civicrm42d7_other');
  drush_provision_civicrm_tests_install_site('civicrm42d7', 'civicrm42d7-test-migrate', 'standard');
  drush_provision_civicrm_tests_migrate_site('civicrm42d7-test-migrate', 'civicrm42d7_other');
  drush_provision_civicrm_tests_remove_site('civicrm42d7-test-migrate');
  drush_provision_civicrm_tests_remove_platform('civicrm42d7_other');

  // Test migrate from 4.3-D7 to 4.3-D7
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.3-D7 to 4.3-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_platform('civicrm43d7', 'civicrm43d7_other');
  drush_provision_civicrm_tests_install_site('civicrm43d7', 'civicrm43d7-test-migrate', 'standard');
  drush_provision_civicrm_tests_migrate_site('civicrm43d7-test-migrate', 'civicrm43d7_other');
  drush_provision_civicrm_tests_remove_site('civicrm43d7-test-migrate');
  drush_provision_civicrm_tests_remove_platform('civicrm43d7_other');

  // Test migrate from 4.3-D7 to 4.3-D7
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.3-D7 to 4.3-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_platform('civicrm44d7', 'civicrm44d7_other');
  drush_provision_civicrm_tests_install_site('civicrm44d7', 'civicrm44d7-test-migrate', 'standard');
  drush_provision_civicrm_tests_migrate_site('civicrm44d7-test-migrate', 'civicrm44d7_other');
  drush_provision_civicrm_tests_remove_site('civicrm44d7-test-migrate');
  drush_provision_civicrm_tests_remove_platform('civicrm44d7_other');

  // Test the CiviCRM 3.4-D6 to 4.3-D7 upgrade
/* DEPRECATED
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 3.4-D6 to 4.3-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_site('civicrm34d6', 'civicrm34d6-test-upgrade', 'default');
  drush_provision_civicrm_tests_migrate_site('civicrm34d6-test-migrate', 'civicrm43d7');
  drush_provision_civicrm_tests_remove_site('civicrm34d6-test-upgrade');

  // Test the CiviCRM 3.4-D6 to 4.2-D7 upgrade
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 3.4-D6 to 4.2-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_site('civicrm34d6', 'civicrm34d6-test-upgrade', 'default');
  drush_provision_civicrm_tests_migrate_site('civicrm34d6-test-migrate', 'civicrm42d7');
  drush_provision_civicrm_tests_remove_site('civicrm34d6-test-upgrade');

  // Test the CiviCRM 4.0-D7 to 4.2-D7 upgrade
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.0-D7 to 4.2-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_site('civicrm40d7', 'civicrm40d7-test-upgrade', 'standard');
  drush_provision_civicrm_tests_migrate_site('civicrm40d7-test-migrate', 'civicrm42d7');
  drush_provision_civicrm_tests_remove_site('civicrm40d7-test-upgrade');
*/

  // Test the CiviCRM 4.2-D7 to 4.4-D7 upgrade
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: migrate from 4.2-D7 to 4.4-D7.'), 'ok');
  drush_log(dt('*****'), 'ok');

  drush_provision_civicrm_tests_install_site('civicrm42d7', 'civicrm42d7-test-upgrade', 'standard');
  drush_provision_civicrm_tests_migrate_site('civicrm42d7-test-migrate', 'civicrm44d7');
  drush_provision_civicrm_tests_remove_site('civicrm42d7-test-upgrade');

  // TODO: test the multi-lingual upgrade

  // Clean up a little.
  drush_log(dt('*****'), 'ok');
  drush_log(dt('***** CiviCRM TEST: Removing platforms.'), 'ok');
  drush_log(dt('*****'), 'ok');

  // drush_provision_civicrm_tests_remove_platform('civicrm34d6');
  // drush_provision_civicrm_tests_remove_platform('civicrm40d7');
  // drush_provision_civicrm_tests_remove_platform('civicrm41d6');
  // drush_provision_civicrm_tests_remove_platform('civicrm41d7');
  drush_provision_civicrm_tests_remove_platform('civicrm42d7');
  drush_provision_civicrm_tests_remove_platform('civicrm43d7');
  drush_provision_civicrm_tests_remove_platform('civicrm44d7');

  // Restore the tasks queue status:
  variable_set('hosting_queue_tasks_enabled', $queue_status_initial);

  if (drush_get_error() != DRUSH_SUCCESS) {
    return drush_set_error(drush_get_error(), 'Running tests failed');
  }

  drush_log(dt('Tests completed successfully'), 'success');
}

/**
 * Helper function to install a platform.
 */
function drush_provision_civicrm_tests_install_platform($platform_name, $platform_alias = NULL) {
  if (is_null($platform_alias)) {
    $platform_alias = $platform_name;
  }
  drush_log(dt('Building platform: @platform and adding to hostmaster.', array('@platform' => $platform_alias)), 'ok');
  $args = array(
    PROVISION_CIVICRM_TESTS_BUILDS_REPO . "/$platform_name.build",
    "/var/aegir/platforms/$platform_alias"
  );
  drush_backend_invoke('make', $args);
  $args = array(
    'root' => "/var/aegir/platforms/$platform_alias",
    "@platform_$platform_alias",
    'context_type' => 'platform',
  );
  drush_backend_invoke('provision-save', $args);
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@platform_$platform_alias",));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Helper function to remove a platform.
 */
function drush_provision_civicrm_tests_remove_platform($platform_name) {
  drush_log(dt('Removing platform: @platform.', array('@platform' => $platform_name)), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'delete'));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Helper function to install a site.
 */
function drush_provision_civicrm_tests_install_site($platform_name, $site, $profile_name) {
  drush_log(dt('Installing: @site on platform: @platform with profile: @profile.', array('@site' => "$site.aegir.example.com", '@platform' => $platform_name, '@profile' => $profile_name)), 'ok');
  $args = array(
    'uri' => "$site.aegir.example.com",
    "@$site.aegir.example.com",
    'context_type' => 'site',
    'platform' => "@platform_$platform_name",
    'profile' => $profile_name,
    'db_server' => '@server_localhost',
    'root' => "/var/aegir/platforms/$platform_name",
  );
  drush_backend_invoke('provision-save', $args);
  provision_backend_invoke("@$site.aegir.example.com", 'provision-install');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$platform_name", 'verify'));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Helper function to delete a site.
 */
function drush_provision_civicrm_tests_remove_site($site) {
  drush_log(dt('Removing: @site.', array('@site' => "$site.aegir.example.com")), 'ok');
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'delete'));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Migrates a site from one platform to another.
 *
 * @param $site
 *   The site to migrate.
 * @param $target
 *   The target platform to migrate to.
 */
function drush_provision_civicrm_tests_migrate_site($site, $target) {
  drush_log(dt('Migrating: @site to platform: @platform.', array('@site' => "$site.aegir.example.com", '@platform' => $target)), 'ok');
  // Do the migrate.
  provision_backend_invoke("@$site.aegir.example.com", 'provision-migrate', array("@platform_$target", "--debug"));
  // Import the site into the frontend.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com",));
  // Verify the $target platform.
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@platform_$target", 'verify'));
  // Import and verify the site.
  provision_backend_invoke('@hostmaster', 'hosting-import', array("@$site.aegir.example.com",));
  provision_backend_invoke('@hostmaster', 'hosting-task', array("@$site.aegir.example.com", 'verify'));
  drush_provision_civicrm_tests_run_remaining_tasks();
}

/**
 * Run all remaining hosting tasks.
 */
function drush_provision_civicrm_tests_run_remaining_tasks() {
  $tasks = array();
  $result = db_query("SELECT t.nid FROM {hosting_task} t INNER JOIN {node} n ON t.vid = n.vid WHERE t.task_status = %d ORDER BY n.changed, n.nid ASC", 0);
  while ($node = db_fetch_object($result)) {
    $tasks[$node->nid] =  node_load($node->nid);
  }

  foreach ($tasks as $task) {
    provision_backend_invoke('@hostmaster', "hosting-task", array($task->nid));
  }
}
