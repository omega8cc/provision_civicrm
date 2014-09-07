<?php

/**
 * The civicrm_db service class.
 */
class Provision_Service_civicrm_db extends Provision_Service {
  public $service = 'civicrm_db';

  /**
   * Add the civicrm_db property to the site context.
   */
  static function subscribe_site($context) {
    $context->setProperty('civicrm_db');
  }
}
