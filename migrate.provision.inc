<?php

/**
 * Implementation of hook_post_provision_migrate
 */
function drush_civicrm_post_provision_migrate($url = NULL) {
  if (_provision_civicrm_is_site_context()) {
    $civi_opts = _provision_civicrm_drush_options('post migrate');
    $target = drush_get_option('target_name');
    provision_backend_invoke($target, 'provision-verify', array(), $civi_opts);
  }
}
