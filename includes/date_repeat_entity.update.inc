<?php

/**
 * @file
 * Contains functions that support update of entities with repeating
 * date fields.
 */

/**
 * Update all entities related to the one represented by the current form.
 *
 * @param type $entity
 * @param type $entity_type
 */
function update_dates($entity, $entity_type) {

  // Get entity wrapper.
  $entity_wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get value of clone state - a new event node will have the default state
  // of FALSE while a cloned event node will have a state of TRUE.
  $field_clone_state = $entity_wrapper->field_clone_state->value();

  // Only act on entities that are not clones or equivalent.
  if ($field_clone_state === FALSE) {

    $scope = $entity->date_repeat_entity['triggering_element'];
    $repeating_date_has_changed = $entity->date_repeat_entity['repeating_date_has_changed'];
    $bundle = $entity_wrapper->getBundle();

    // Get the series master UUID.
    $master_uuid = $entity_wrapper->field_master_uuid->value();

    // If the repeating date pattern has changed then replace the entities.
    if ($repeating_date_has_changed) {

      // Replace dates and return a new master entity.
      $new_master_entity = _replace_dates($entity_wrapper, $entity, $entity_type, $bundle, $scope);
    }
    else {
      // Update future dates
      _update_dates($entity_wrapper, $entity, $entity_type, $bundle, $master_uuid, $scope);

      // Since we haven't replaced entity, redirect user to current entity.
      $new_master_entity = $entity;
    }
  }
  else {
    // Remove any date field delta values that were created during the save
    // operation - any existing entities should not have these deltas
    array_splice($entity->field_date[$entity->language], 1);
  }
}

/**
 * Replace dates associated with a repeating date series.
 */
function _replace_dates($entity_wrapper, $entity, $entity_type = 'node', $bundle = 'event', $scope = 'current') {

  // Generate a new master UUID.
  $master_uuid_new = uuid_generate();

  // Change the entity uuid and set its status to new
  $entity->uuid = $master_uuid_new;
  $entity->vuuid = $master_uuid_new;

  // Create new dates based on the current entity and RRULE defined with it.
  create_dates($entity, $entity_type);

  // Make sure repeating date delete functions are available.
  module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.delete');

  // Delete existing date entities that were replaced
  _delete_dates($entity_wrapper, $entity_type, $bundle, $scope);

  return $entity;
}

/**
 * Update dates associated with a repeating date series.
 */
function _update_dates($entity_wrapper, $entity, $entity_type = 'node', $bundle = 'event', $master_uuid = NULL, $scope = 'current') {

  // Get dates
  $result = _get_dates($entity_wrapper, $entity_type, $bundle, $scope);

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Retrieve entities based on entity ids.
    $date_entities = entity_load($entity_type, $date_entity_ids);

    $new_title = $entity->title;
    //$new_title = $date_repeat_entity_form_state['values']['title'];

    foreach ($date_entities as $date_entity) {

      // Update title and any other relevant fields.
      $date_entity->title = $new_title;

      // Set clone state to TRUE so the cloning procedures are skipped
      // when hook_entity_presave is triggered.
      $date_entity->field_clone_state = TRUE;

      // Save the entity.
      entity_save($entity_type, $date_entity);
    }
  }
}

/**
 * Update current date only from a repeating date series.
 */
function _update_current_date_only($date_repeat_entity_form_state, $entity, $entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  $repeating_date_field = _get_repeating_date_field($entity_type, $bundle);
  $repeating_date_field_name = $repeating_date_field['field_name'];

  $language = $date_repeat_entity_form_state['values']['language'];

  $new_date_start_value = $date_repeat_entity_form_state['values'][$repeating_date_field_name][$language][0]['value'];
  $new_date_end_value = $date_repeat_entity_form_state['values'][$repeating_date_field_name][$language][0]['value2'];
  $new_rrule = $date_repeat_entity_form_state['values'][$repeating_date_field_name][$language][0]['rrule'];

  $entity->log = "cloned from uuid : " . $master_uuid;

  $entity->title = $date_repeat_entity_form_state['values']['title'];

  // Set the start date.
  $entity->{$repeating_date_field_name}[$entity->language][0]['value'] = $new_date_start_value;

  // Set the end date.
  $entity->{$repeating_date_field_name}[$entity->language][0]['value2'] = $new_date_end_value;

  // Set the RRULE field value.
  $entity->{$repeating_date_field_name}[$entity->language][0]['rrule'] = $new_rrule;

  // Save the cloned entity.
  entity_save($entity_type, $entity);
}

/**
 * Determines if an entity with a repeating date has changed.
 *
 * Compares the start date, end date and RRULE of new and old entity.
 * 
 */
function _repeating_date_has_changed($form, $form_state) {

  $repeating_date_has_changed = FALSE;

  // Check that entity aleady exists - we are not validating new entities.
  if ($form_state['values']['changed'] != "") {

    // Check that the entity form has a repeating date field and a field that
    // controls the scope of changes to an entity e.g. All, Future, Single.
    $entity_type = $form['#entity_type'];
    $bundle = $form['#bundle'];

    // Make sure utility functions are available.
    module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.utility');
    $repeating_date_field = _get_repeating_date_field($entity_type, $bundle);

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
      }
    }
  }
  return $repeating_date_has_changed;
}

/**
 * Get new date series if an entity with a repeating date has changed.
 * 
 * Note: this function is not currently used but could be useful if we decide
 * to allow all dates in a series to be shifted based on the revised RRULE,
 * start and end dates of the instance that changed.
 * 
 * @param type $form_state
 * @param type $entity_id
 * @param type $entity_type
 * @param type $bundle
 * @param type $scope
 *
 * @return array
 *   NULL if the repeating date has not changed.
 */
function _get_updated_dates($form_state, $entity_id, $entity_type = 'node', $bundle = 'event', $scope = 'All') {

  $new_dates = array();

  // Make sure utility functions are available.
  module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.utility');

  // Check that the entity form has a repeating date field
  $repeating_date_field = _get_repeating_date_field($entity_type, $bundle);

  if ($repeating_date_field != NULL) {

    // Get the name of the repeating field.
    $repeating_date_field_name = $repeating_date_field['field_name'];
    $language = $form_state['values']['language'];

    $new_date_start_value = $form_state['values'][$repeating_date_field_name][$language][0]['value'];
    $new_date_end_value = $form_state['values'][$repeating_date_field_name][$language][0]['value2'];
    $new_rrule = $form_state['values'][$repeating_date_field_name][$language][0]['rrule'];

    // TODO: consider if repeat_date_start_value needed - Google calendar uses it
    $new_repeat_date_start_value = $form_state['values']['field_repeat_start_date'][$language][0]['value'];

    $old_entity = entity_load_unchanged($entity_type, $entity_id);

    $old_date_start_value = $old_entity->{$repeating_date_field_name}[$old_entity->language][0]['value'];
    $old_date_end_value = $old_entity->{$repeating_date_field_name}[$old_entity->language][0]['value2'];
    $old_rrule = $old_entity->{$repeating_date_field_name}[$old_entity->language][0]['rrule'];

    // TODO: consider if repeat_date_start_value needed - Google calendar uses it
    $old_repeat_date_start_value = $old_entity->field_repeat_start_date[$old_entity->language][0]['value'];

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
    $master_entity = _get_master_entity($entity_type, $bundle, $master_uuid);

    // Get a start date value for the new date series.
    if (!empty($master_entity)) {
      // If the master entity exists, then use it as the start date value
      // for calculating new date series.
      $master_date_start_value = $master_entity->field_date[$master_entity->language][0]['value'];
    }
    else {
      // Otherwise, use the value in the value of the
      // repeat_date_start field.
      $master_date_start_value = $new_repeat_date_start_value;
    }
    // Convert master entity start date to a DateObject.
    $master_date_start = new DateObject($master_date_start_value, $timezone, date_type_format($repeating_date_field['type']));

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
  return $new_dates;
}
