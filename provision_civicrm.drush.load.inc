<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is enabled.
 */
function provision_civicrm_drush_load() {
  return provision_hosting_feature_enabled('civicrm');
}
