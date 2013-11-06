<?php

/**
 * @file
 * Contains functions that support deletion of entities with
 * repeating date fields.
 */

/**
 * Custom submit handler called when an entity deleted.
 *
 * @param array $form
 *   the current form.
 *
 * @param array $form_state
 *   the current form state.
 */
function delete_all_dates($form, &$form_state) {

  $entity_type = 'node';
  $bundle = 'event';
  $entity_id = $form['nid']['#value'];
  $entity = _get_entity($entity_type, $bundle, $entity_id);

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get the series master UUID.
  $master_uuid = $wrapper->field_master_uuid->value();

  $repeating_date_operation_successful = _delete_all_dates($entity_type, $bundle, $master_uuid);

  if ($repeating_date_operation_successful) {
    // We still need to delete the original
    //node_delete_confirm_submit($form, $form_state);
  }

  // Redirect to front page.
  $form_state['redirect'] = '<front>';
}

/**
 * Custom submit handler called when an entity deleted.
 *
 * @param array $form
 *   the current form.
 *
 * @param array $form_state
 *   the current form state.
 */
function delete_future_dates($form, &$form_state) {

  $entity_type = 'node';
  $bundle = 'event';
  $entity_id = $form['nid']['#value'];
  $entity = _get_entity($entity_type, $bundle, $entity_id);

  // Get entity wrapper.
  $entity_wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get the series master UUID.
  $master_uuid = $entity_wrapper->field_master_uuid->value();

  // Get the repeating field date values.
  $repeating_date_field = _get_repeating_date_field($entity_type, $bundle);
  $repeating_date_field_name = $repeating_date_field['field_name'];

  $field_data = $entity_wrapper->{$repeating_date_field_name}[0]->raw();
  $date_start_value = $field_data['value'];
  $repeating_date_operation_successful = _delete_future_dates($entity_type, $bundle, NULL, $master_uuid, $date_start_value);

  if ($repeating_date_operation_successful) {
    // We still need to delete the original
    //node_delete_confirm_submit($form, $form_state);
  }

  // Redirect to front page.
  $form_state['redirect'] = '<front>';
}

/**
 * Delete the current date only.
 *
 * @param string $entity_type
 *   An entity type e.g. node.
 * @param string $bundle
 *   A bundle type e.g. event.
 * @param string $master_uuid
 *   the alphanumeric master_uuid.
 */
function _delete_current_date($entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  // Instantiate boolean variable to track success of operation.
  $repeating_date_operation_successful = FALSE;

  // Build an array of entity ids.
  $date_entity_ids = _get_related_entity_ids($entity_type, $bundle, $master_uuid);

  if (!is_null($date_entity_ids)) {

    // Delete an array of entities. Returns failure as FALSE, success as NULL.
    $success = entity_delete_multiple($entity_type, $date_entity_ids);
    if ($success !== FALSE) {
      $repeating_date_operation_successful = TRUE;
    }
  }

  return $repeating_date_operation_successful;
}

/**
 * Delete all dates associated with a repeating date series.
 *
 * @param string $entity_type
 *   An entity type e.g. node.
 * @param string $bundle
 *   A bundle type e.g. event.
 * @param string $master_uuid
 *   the alphanumeric master_uuid.
 */
function _delete_all_dates($entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  // Instantiate boolean variable to track success of operation.
  $repeating_date_operation_successful = FALSE;

  // Build an array of entity ids.
  $date_entity_ids = _get_related_entity_ids($entity_type, $bundle, $master_uuid);

  if (!is_null($date_entity_ids)) {

    // Delete an array of entities. Returns failure as FALSE, success as NULL.
    $success = entity_delete_multiple($entity_type, $date_entity_ids);
    if ($success !== FALSE) {
      $repeating_date_operation_successful = TRUE;
    }
  }

  return $repeating_date_operation_successful;
}

/**
 * Delete future dates associated with a repeating date series.
 *
 * @param string $entity_type
 *   An entity type e.g. node.
 * @param string $bundle
 *   A bundle type e.g. event.
 * @param int $entity_id
 *   Entity id
 * @param string $master_uuid
 *   the alphanumeric master_uuid.
 * @param string start date from which all future dates will be deleted.
 *
 * @return bool
 *   the sucess of this functioon is represented by this value.
 */
function _delete_future_dates($entity_type = 'node', $bundle = 'event', $entity_id = NULL, $master_uuid = NULL, $start_date = NULL) {


  // Instantiate boolean variable to track success of operation.
  $repeating_date_operation_successful = FALSE;

  // Use EFQ to get all dates with the same field_master_uuid value.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
      // Note: exclude current entity because it will be deleted as part of the
      // hook_entity_delete function that calls this one.
      ->fieldCondition('field_master_uuid', 'value', $master_uuid);

  // If a start date is passed to function, add field condition to 
  // exclude earlier entities from the query;
  if (!is_null($start_date)) {
    $query->fieldCondition('field_date', 'value', $start_date, '>=');
  }

  // If an entity id is passed to function, add property conditon to 
  // exclude an entity from the query
  if (!is_null($entity_id)) {
    $query->propertyCondition('nid', $entity_id, '<>');
  }

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Delete an array of entities. Returns failure as FALSE, success as NULL.
    $success = entity_delete_multiple($entity_type, $date_entity_ids);
    if ($success !== FALSE) {
      $repeating_date_operation_successful = TRUE;
    }
  }

  // Need to update RRULE on remaining entities to reflect the fact that some
  // future dates were truncated and so the effective end date of the series
  // should be brought forward.
  // $rrule = date_api_ical_build_rrule($element['#value']);
  return $repeating_date_operation_successful;
}
