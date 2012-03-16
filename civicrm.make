api = 2
core = 7.x

projects[] = drupal

projects[webform_civicrm][subdir] = contrib
projects[views][subdir] = contrib
projects[ctools][subdir] = contrib

# Other useful modules:
# projects[civicrm_error] = contrib
# projects[civicrm_theme] = contrib
# projects[admin_menu][subdir] = contrib
# projects[devel][subdir] = contrib

libraries[civicrm][type] = "module"
libraries[civicrm][directory_name] = "civicrm"
libraries[civicrm][download][type] = "get"
libraries[civicrm][download][url] = "http://downloads.sourceforge.net/project/civicrm/civicrm-stable/4.1.1/civicrm-4.1.1-drupal.tar.gz"

libraries[civicrm_l10n][destination] = "module"
libraries[civicrm_l10n][directory_name] = "civicrm"
libraries[civicrm_l10n][download][type] = "get"
libraries[civicrm_l10n][download][url] = "http://downloads.sourceforge.net/project/civicrm/civicrm-stable/4.1.1/civicrm-4.1.1-l10n.tar.gz"
libraries[civicrm_l10n][overwrite] = TRUE

