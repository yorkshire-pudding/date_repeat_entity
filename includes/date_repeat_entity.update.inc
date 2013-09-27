<?php

/**
 * @file
 * Contains functions that support update of entities with repeating
 * date fields.
 */

/**
 * Update all entities related to the one represented by the current form.
 *
 * @param array $form
 *   the current form.
 * @param array $form_state
 *   the current form state.
 */
function update_all_dates($form, &$form_state) {

  $entity_type = 'node';
  $bundle_type = 'event';
  $entity_id = $form['nid']['#value'];

  // Get an entity object.
  $entity = _get_entity($entity_type, $bundle_type, $entity_id);

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get the series master UUID.
  $master_uuid = $wrapper->field_master_uuid->value();

  // If the dates have changes and are stored in the form state then update
  // the entities accordingly.
  if (isset($_SESSION['date_repeat_entity'])) {
    $new_dates = $_SESSION['date_repeat_entity'];

    // Update all the dates.
    $updated_entity_values = _update_all_dates($entity, $entity_id, $new_dates, $entity_type, $bundle_type, $master_uuid);

    // Update the current entity based on any values returned from
    // _update_all_dates function
    if (isset($updated_entity_values) && $updated_entity_values != NULL) {
      $entity->field_date[$entity->language][0]['value'] = $updated_entity_values['value'];
      $entity->field_date[$entity->language][0]['value2'] = $updated_entity_values['value2'];
      $entity->field_date[$entity->language][0]['rrule'] = $updated_entity_values['rrule'];
      // $entity->field_repeat_start_date[$entity->language][0]['value'] = $updated_entity_values['repeat_date_start_value'];
    }
  }
  // Redirect user to form in view mode.
  $node = $form['#node'];
  $form_state['redirect'] = node_access('view', $node) ? 'node/' . $entity_id : '<front>';
}

/**
 * Update all entities related to the one represented by the current form.
 *
 * @param array $form
 *   the current form.
 * @param array $form_state
 *   the current form state.
 */
function update_future_dates(&$form, &$form_state) {

  $entity_type = 'node';
  $bundle_type = 'event';
  $entity_id = $form['nid']['#value'];

  // Get an entity object.
  $entity = _get_entity($entity_type, $bundle_type, $entity_id);

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get the series master UUID.
  $master_uuid = $wrapper->field_master_uuid->value();

  // Update all the dates.
  _update_future_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);

  // Redirect user to form in view mode.
  $node = $form['#node'];
  $form_state['redirect'] = node_access('view', $node) ? 'node/' . $entity_id : '<front>';
}

/**
 * Update only the current entity.
 *
 * No additional saving of other nodes is required because the current
 * entity will be saved by default.
 *
 * @param array $form
 *   the current form.
 * @param array $form_state
 *   the current form state.
 */
function update_current_date(&$form, &$form_state) {

  $entity_id = $form['nid']['#value'];

  // Redirect user to form in view mode.
  $node = $form['#node'];
  $form_state['redirect'] = node_access('view', $node) ? 'node/' . $entity_id : '<front>';
}

/**
 * Process all dates associated with a repeating date series.
 */
function _update_all_dates($entity, $entity_id, $new_dates, $entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  // Instantiate array to be returned.
  $updated_entity_values = array();

  // Use EFQ to update all dates with the same field_master_uuid value
  // keep the UUIDs but update the field_date multivalue fields for each
  // entity in the series.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
      // Note: exclude current entity.
      // ->entityCondition('entity_id', array($entity_id), 'NOT IN')
      ->fieldCondition('field_master_uuid', 'value', $master_uuid, '=')
      // Sort by start date.
      ->fieldOrderBy('field_date', 'value', 'ASC');

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Retrieve RRULE for current entity.
    // $rrule = $entity->field_date[$entity->language][0]['rrule'];
    // Retrieve start date for current entity since this is being applied
    // to all dates by the user.
    // $start_date = $entity->field_date[$entity->language][0]['value'];
    // $end_date = $entity->field_date[$entity->language][0]['value2'];
    // Retrieve dates - note: date_repeat_calc returns sorted dates.
    // TODO: account for exceptions and additions as well as timezone.
    // $new_dates = date_repeat_calc($rrule, $start_date, NULL);
    //
    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Retrieve entities based on entity ids.
    $date_entities = entity_load($entity_type, $date_entity_ids);

    // Update instances, where necessary...
    // From the Groupanizer 3 design docs:
    // Changing all instances will involve updating all instances represented
    // by the recurrence pattern.  Exceptions need not be updated.
    // In Google Calendar, if a recurrence pattern’s start date, start time
    // but not duration apparently) and/or frequency is modified then all the
    // properties of each exception will be changed to match the recurrence
    // pattern.  This includes recreation of instances that were previously
    // deleted exceptions. Google Calendar appears to employ a more subtle
    // approach i.e. if an instance’s field (title, start date, color etc.)
    // has been modified (i.e. it differs from that of the recurrence pattern,
    // then the modified field values will be preserved.  However, if they are
    // the same as the recurrence pattern then they will be changed to meet the
    // recurrence pattern.  After changes are applied to all instances, they
    // remain part of the original recurrence pattern.
    $updated_entity_values = array();

    // Dummy iterator because $date_entities keys are entity_ids
    // and $new_dates keys are numeric indices.
    $counter = 0;

    foreach ($date_entities as $delta => $date_entity) {

      // Get the start, end date and RRULE for this date_entity based on its
      // position in $new_dates array.
      $date_entity_start_date = $new_dates[$counter]['value'];
      $date_entity_end_date = $new_dates[$counter]['value2'];
      $date_entity_rrule = $new_dates[$counter]['rrule'];

      // Check if $date_entity (the $delta key is the entity_id) matches
      // the entity passed to this routine.
      if ($delta == $entity_id) {
        // The current $date_entity matches the one that was passed to this
        // routine so we need to avoid updating it in the middle of a save
        // operation.  Instead save the new values in an array that is returned
        // to the calling function.
        $updated_entity_values['value'] = $date_entity_start_date;
        $updated_entity_values['value2'] = $date_entity_end_date;
        $updated_entity_values['rrule'] = $date_entity_rrule;
      }
      else {

        // Set the entity title.
        $date_entity->title = $entity->title;

        // Set the start date.
        $date_entity->field_date[$entity->language][0]['value'] = $date_entity_start_date;

        // Set the end date.
        $date_entity->field_date[$entity->language][0]['value2'] = $date_entity_end_date;

        // Set the RRULE field value.
        $date_entity->field_date[$entity->language][0]['rrule'] = $date_entity_rrule;

        // Set clone state to TRUE so the cloning procedures are skipped
        // when hook_entity_presave is triggered.
        $date_entity->field_clone_state = TRUE;

        // Save the entity.
        entity_save($entity_type, $date_entity);
      }

      // Increment counter.
      $counter++;
    }
  }
  return $updated_entity_values;
}

/**
 * Process current and future dates associated with a repeating date series.
 */
function _update_current_and_future_dates() {
  // Use EFQ to update all dates with the same field_master_uuid value
  // and any dates that are later than $entity.
  // Since this entity and all future ones are eing changed it is assumed that
  // they are forming a new series (per Google and TeamSnap implementations)
  // and so new UUID will be required for this date and then the
  // field_master_uuid and the field_date multivalue fields will have to be
  // updated as well for each entity in the series.
}

/**
 * Process current date only from a repeating date series.
 */
function _update_current_date_only($entity, $entity_id, $entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {
  // Use EFQ to update all dates with the same field_master_uuid value.
  // This may not be necessary as there are no other dates besides the
  // current entity.  However, since this is being changed it is assumed
  // that it is becoming an exception to the current series and will have
  // to be added to the exceptions rule in RRULE field for all related entities.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
      ->fieldCondition('field_master_uuid', 'value', $master_uuid, '=');

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Retrieve RRULE for current entity.
    $rrule = $entity->field_date[$entity->language][0]['rrule'];

    // Retrieve start date for current entity since this is being applied
    // to all dates by the user.
    $start_date = $entity->field_date[$entity->language][0]['value'];

    // Retrieve dates.
    // TODO: account for exceptions and additions as well as timezone.
    $new_dates = date_repeat_calc($rrule, $start_date, NULL);

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Retrieve entities based on entity ids.
    $date_entities = entity_load($entity_type, $date_entity_ids);

    // Update instances, where necessary...
    // From the Groupanizer 3 design docs:
    // Changing all instances will involve updating all instances represented
    // by the recurrence pattern.  Exceptions need not be updated.
    // In Google Calendar, if a recurrence pattern’s start date, start time
    // but not duration apparently) and/or frequency is modified then all the
    // properties of each exception will be changed to match the recurrence
    // pattern.  This includes recreation of instances that were previously
    // deleted exceptions. Google Calendar appears to employ a more subtle
    // approach i.e. if an instance’s field (title, start date, color etc.)
    // has been modified (i.e. it differs from that of the recurrence pattern,
    // then the modified field values will be preserved.  However, if they are
    // the same as the recurrence pattern then they will be changed to meet the
    // recurrence pattern.  After changes are applied to all instances, they
    // remain part of the original recurrence pattern.
    foreach ($date_entities as $date_entity) {

      // Set the RRULE field value.
      $date_entity->field_date[$entity->language][0]['rrule'] = $rrule;

      // Set the RRULE field value.
      $date_entity->field_date[$entity->language][0]['rrule'] = $rrule;

      // Set clone state to TRUE so the cloning procedures are skipped
      // when hook_entity_presave is triggered.
      $date_entity->field_clone_state = TRUE;

      // Save the entity.
      entity_save($entity_type, $date_entity);
    }
  }
}

/**
 * Get new date series if an entity with a repeating date has changed.
 *
 * Compares the start date, end date and RRULE of new and old entity.  If
 * the date series has effectively changed, the function returns the new
 * series.
 *
 * @return array
 *   NULL if the repeating date has not changed.
 */
function _get_updated_dates($form, $form_state) {

  $new_dates = array();

  // Check that entity aleady exists - we are not validating new entities.
  if ($form_state['values']['changed'] != "") {

    // Check that the entity form has a repeating date field and a field that
    // controls the scope of changes to an entity e.g. All, Future, Single.
    $entity_type = $form['#entity_type'];
    $bundle_type = $form['#bundle'];

    // Make sure utility functions are available.
    module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.utility');
    $repeating_date_field = _get_repeating_date_field($entity_type, $bundle_type);

    if ($repeating_date_field != NULL) {

      // Get the name of the repeating field.
      $repeating_date_field_name = $repeating_date_field['field_name'];
      $language = $form_state['values']['language'];

      $new_date_start_value = $form_state['values'][$repeating_date_field_name][$language][0]['value'];
      $new_date_end_value = $form_state['values'][$repeating_date_field_name][$language][0]['value2'];
      $new_rrule = $form_state['values'][$repeating_date_field_name][$language][0]['rrule'];

      // TODO: consider if repeat_date_start_value needed - Google calendar uses it
      $new_repeat_date_start_value = $form_state['values']['field_repeat_start_date'][$language][0]['value'];

      $entity_id = $form['#entity']->nid;
      $old_entity = entity_load_unchanged($entity_type, $entity_id);

      $old_date_start_value = $old_entity->{$repeating_date_field_name}[$old_entity->language][0]['value'];
      $old_date_end_value = $old_entity->{$repeating_date_field_name}[$old_entity->language][0]['value2'];
      $old_rrule = $old_entity->{$repeating_date_field_name}[$old_entity->language][0]['rrule'];

      // TODO: consider if repeat_date_start_value needed - Google calendar uses it
      $old_repeat_date_start_value = $old_entity->field_repeat_start_date[$old_entity->language][0]['value'];

      // Check if the entity date has changed to the extent that
      // the repeating date series has changed and therefore dependent data
      // like date exceptions and referencing entities will need to be reset.
      $new_date_data = array(
        $new_date_start_value,
        $new_date_end_value,
        $new_rrule,
        $new_repeat_date_start_value,
      );

      $old_date_data = array(
        $old_date_start_value,
        $old_date_end_value,
        $old_rrule,
        $old_repeat_date_start_value,
      );

      if ($new_date_data !== $old_date_data) {
        $repeating_date_has_changed = TRUE;

        // If the repeating date series has changed, get new date series.
        if ($repeating_date_has_changed) {

          // Get timezone associated with the field instance.
          $timezone = date_get_timezone($repeating_date_field['settings']['tz_handling']);

          // Create DateObjects (extensions of PHP DateTime objects)
          // from the new start date and old start date.
          $new_date_start = new DateObject($new_date_start_value, $timezone, date_type_format($repeating_date_field['type']));
          $new_date_end = new DateObject($new_date_end_value, $timezone, date_type_format($repeating_date_field['type']));
          $old_date_start = new DateObject($old_date_start_value, $timezone, date_type_format($repeating_date_field['type']));
          $old_date_end = new DateObject($old_date_end_value, $timezone, date_type_format($repeating_date_field['type']));

          // Create DateObjects from the series start and end dates.
          // $new_repeat_date_start = new DateObject(
          //   $new_repeat_date_start_value,
          //   $timezone, date_type_format($repeating_date_field['type']));
          // Return the time difference (seconds) between:
          // a) the new start date and old start date and
          // b) the new start date and new end date.
          $seconds_between_new_and_old_start_date = $new_date_start->difference($old_date_start);
          $seconds_between_new_start_and_new_end_date = $new_date_end->difference($new_date_start);

          // Define PHP Interval objects.
          $interval_between_new_and_old_start_date = new DateInterval('PT' . $seconds_between_new_and_old_start_date . 'S');
          $interval_between_new_start_and_new_end_date = new DateInterval('PT' . $seconds_between_new_start_and_new_end_date . 'S');

          // Get the series master entity uuid from the current entity.
          $master_uuid_field_name = 'field_master_uuid';
          $master_uuid = $old_entity->{$master_uuid_field_name}[$old_entity->language][0]['value'];

          // Get the master entity using the master entity uuid.
          $master_entity = _get_master_entity($entity_type, $bundle_type, $master_uuid);

          // Get the master entity start date value.
          $master_date_start_value = $master_entity->field_date[$master_entity->language][0]['value'];

          // Convert master entity start date to a DateObject.
          $master_date_start = new DateObject($master_date_start_value, $timezone, date_type_format($repeating_date_field['type']));

          // Create a master entity end date based on the start date
          // $master_date_end = new DateObject(
          //   $master_date_start_value,
          //   $timezone,
          //   date_type_format($repeating_date_field['type']));
          // Apply the calculated interval between the new and old start
          // dates of the current entity to the master entity start date.
          if ($new_date_start > $old_date_start) {
            $new_master_date_start = $master_date_start->add($interval_between_new_and_old_start_date);
          }
          else {
            $new_master_date_start = $master_date_start->sub($interval_between_new_and_old_start_date);
          }

          // Create a new master entity end date based on the adjusted
          // master start date.
          $new_master_date_start_clone = clone $new_master_date_start;

          // Apply the calculated interval between the new start and
          // new end date of the current entity to the new master entity
          // end date. This will yield a new master end date.
          if ($new_date_end > $old_date_end) {
            $new_master_date_end = $new_master_date_start_clone->add($interval_between_new_start_and_new_end_date);
          }
          else {
            $new_master_date_end = $new_master_date_start_clone->sub($interval_between_new_start_and_new_end_date);
          }

          // Get formatted strings representing new master start date and
          // end date.
          $new_master_date_start_value = date_format($new_master_date_start, date_type_format($repeating_date_field['type']));
          $new_master_date_end_value = date_format($new_master_date_end, date_type_format($repeating_date_field['type']));

          // Create an item array to simulate the field instance structure
          // used in Date module's Form API functions.
          $item = array(
            'value' => $new_master_date_start_value,
            'value2' => $new_master_date_end_value,
            'timezone' => $timezone);

          // Get new date series based on the new master start and end dates.
          $new_dates = date_repeat_build_dates($new_rrule, NULL, $repeating_date_field, $item);
        }
      }
    }
  }
  return $new_dates;
}
