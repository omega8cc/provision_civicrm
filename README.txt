CiviCRM support for Aegir
=========================

Provision CiviCRM is a Drush module to automatically setup Drupal instances
with the CiviCRM constituent relationship management module. It is specifically
aimed towards the Aegir project.

In other words, it will handle the installation of CiviCRM, generate the
civicrm.settings.php file, handle upgrades and so on.

Project home page:
http://drupal.org/project/provision_civicrm

CiviCRM home page:
https://www.civicrm.org

Aegir project:
http://www.aegirproject.org/
http://community.aegirproject.org/contrib-modules/provision-civicrm


Installation instructions
-------------------------

Drop this directory in Aegir's Drush path (e.g. /var/aegir/.drush).

Create an Aegir platform that has the CiviCRM module in its sites/all
directory (although in recent versions, we try to phase out this requirement
so that we can better support install profiles).

A "drush make" makefile is available in civicrm.make.

Verify the platform once the module has been added.

When creating a new site, provision_civicrm will detect the presence
of CiviCRM in the platform and will install/configure it automatically.


Related modules
---------------

* Hosting front-end: https://www.drupal.org/project/hosting_civicrm
* Run CiviCRM crons automatically: https://www.drupal.org/project/hosting_civicrm_cron
* Code tests: https://drupal.org/project/vagrant_scripts_aegir_civicrm


Support
-------

Please use the issue queue for support:
https://drupal.org/project/issues/provision_civicrm

You can also ask questions in either the #aegir or #civicrm
IRC channel on irc.freenode.org, but keep in mind that most
active people in those channels do not necessarely use this
module. You can try to ping the module maintaner, 'bgm'.

Commercial hosting, support and development is also possible:

* Praxis Labs Coop <http://praxis.coop/> (hosting, support, dev)
* Coop SymbioTIC <https://www.symbiotic.coop> (dev)
* Omera8.cc <https://omega8.cc/> (hosting)
* Civi-go <http://civigo.net/> (hosting, dev via Ixiam.com)
* Koumbit <http://www.koumbit.org> (hosting)
* Progressive Technology Project <http://www.progressivetech.org/> (hosting)

Other Aegir service providers: http://community.aegirproject.org/service-providers

If you appreciate this module, please consider donating to either CiviCRM
or the Aegir project.

https://civicrm.org/participate/support-civicrm
http://community.aegirproject.org/donate

You can also send the module maintainer a beer:
https://www.bidon.ca/en/paypal


Credits
-------

This module was created by Mathieu Petit-Clair https://drupal.org/user/1261
during the CiviCRM code sprint in San Francisco of spring 2012,
with the help of Deepak Srivastava http://civicrm.org/blogs/deepaksrivastava
who wrote the CiviCRM drush module.

Maintenance and development was then continued by Mathieu Lutfy (bgm)
https://drupal.org/user/89461, with the help of many contributors
https://drupal.org/node/1063394/committers and with great support from
the CiviCRM core team and community.

Thanks to Koumbit, Praxis, Ixiam, PTP and JMA consulting for financially
supporting the development of this module.


Todo
----

* use Aegir variables in civicrm.settings.php so that the database
  details are not readable by other users on the server (as is the
  case in settings.php).
* have a clean civicrm.drush.inc (improve upstream)  CRM-9986
* support for installing civicrm in a different DB? Issue #1097496
* configure crons automatically (civimail, civimember) Issue #1097554
* better support for install profiles. Issue #1309550
* install CiviCRM when migrating from non-civi to civi platform. Issue #1127252
* support non-english installations. Issue #1485920
