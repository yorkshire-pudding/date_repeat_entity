
-- SUMMARY --

The Date Repeat Entity Module complements the Date suite of modules, in
particular the Date Repeat module.  For repeating dates that follow a
recurring rule, this module creates a unique entity for each date instance in
the series.

When an entity that belongs to a repeating date series is updated
or deleted, the user is offered the choice of updating/deleting the current
entity, all current and future entities or all entities in the series.

If the recurring rule for a date series is changed significantly (and this
can be controlled through a hook - see Customization section below) the existing
entities are replaced by a new set of entities that is generated to represent
the new series.  In this case the module provides a warning to the user if
referenced entities, associated with the current date entity, may be affected.

-- REQUIREMENTS --

The following modules must be installed and enabled for this module to work:

* Date, Date API, Date Repeat, Date Repeat Field (all part of Date module)
* Replicate
* UUID

-- INSTALLATION --

* Install as usual, see https://drupal.org/documentation/install/modules-themes/modules-7
 for further information.

-- CONFIGURATION --

* For each entity that you want to use this module with, you need to add two
  additional fields

  - field_clone_state (boolean)
  - field_master_uuid (text)

  These fields are necessary to manage the replication of entities in a date
  series and to track related entities via a master UUID.  An example feature
  module Date Repeat Entity Example, bundled with this module, provides these
  an Event content type including the above fields and a calendar view.

  Eventually, it is hoped that these two fields can be integrated with the
  schema definition provided by the Date module.

-- CUSTOMIZATION --

* When a date series is changed it may be  significant enough that the existing
  entities in the series should be deleted and replaced by a new series.  This
  module provides HOOK_repeat_entity_repeating_date_has_changed that gives other
  module developers the ability to determine when the change is significant such
  that the series is replaced.

* If a date series that is changed and the change is not significant to generate
  a new series the existing entities representing each date in the series are
  updated.  A separate HOOK_repeating_date_update is provided to enable other
  module developers to determine which properties and fields of each entity
  should be updated.

-- CONTACT --

Current maintainers:
* Simon Shutter (eft) - https://drupal.org/user/458578
* Tom Metzger (tmetzger) - https://drupal.org/user/423974

This project has been sponsored by:

* Groupanizer Technology Services

  We create websites and online tools that help choirs, choruses and music
  groups manage and organize members, music and more while creating a
  captivating public presence.  Visit http://www.groupanizer.com/ for more
  information.
