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
function update_all_dates(&$form, &$form_state) {

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
  _update_all_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);

  // Redirect user to form in view mode.
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
