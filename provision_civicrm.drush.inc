<?php

/**
 * @file
 * Hookup with drush
 * Common functions used by install and verify.
 */

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

  $civicron_password = md5(uniqid(rand(), true));

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
}

