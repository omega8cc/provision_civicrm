<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is enabled.
 */
function provision_civicrm_drush_load() {
  return in_array('civicrm', drush_get_option('hosting_features', array()));
}
