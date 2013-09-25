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
function delete_all_dates(&$form, &$form_state) {

  $entity_type = 'node';
  $bundle_type = 'event';
  $entity_id = $form['nid']['#value'];
  $entity = _get_entity($entity_type, $bundle_type, $entity_id);

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get the series master UUID.
  $master_uuid = $wrapper->field_master_uuid->value();

  $deleted_all_dates_successfully = _delete_all_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);

  if ($deleted_all_dates_successfully) {
    // We still need to delete the original 
    node_delete_confirm_submit($form, &$form_state);
  }
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
function delete_future_dates(&$form, &$form_state) {

  $entity_type = 'node';
  $bundle_type = 'event';
  $entity_id = $form['nid']['#value'];
  $entity = _get_entity($entity_type, $bundle_type, $entity_id);

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get the series master UUID.
  $master_uuid = $wrapper->field_master_uuid->value();

  $deleted_future_dates_successfully = _delete_future_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);

  if ($deleted_future_dates_successfully) {
    // We still need to delete the original 
    node_delete_confirm_submit($form, &$form_state);
  }
}

/**
 * Delete all dates associated with a repeating date series.
 * 
 * @param int $entity_id
 *   A numeric identifier for an identity.
 * @param string $entity_type
 *   An entity type e.g. node.
 * @param string $bundle_type
 *   A bundle type e.g. event.
 */
function _delete_all_dates($entity, $entity_id, $entity_type = 'node', $bundle_type = 'event', $master_uuid = NULL) {

  // Instantiate boolean variable to track success of delete operation.
  $deleted_all_dates_successfully = FALSE;

  // Use EFQ to get all dates with the same field_master_uuid value.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle_type)
      // Note: exclude current entity because it will be deleted as part of the
      // hook_entity_delete function that calls this one.
      ->propertyCondition('nid', $entity_id, '<>')
      ->fieldCondition('field_master_uuid', 'value', $master_uuid);

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Delete an array of entities. Returns failure as FALSE, success as NULL.
    $success = entity_delete_multiple($entity_type, $date_entity_ids);
    if ($success !== FALSE) {
      $deleted_all_dates_successfully = TRUE;
    }
  }

  return $deleted_all_dates_successfully;
}

/**
 * Delete future dates associated with a repeating date series.
 * 
 * @param object $entity
 *   The entity object.
 * @param int $entity_id
 *   A numeric identifier for an identity.
 * @param string $entity_type
 *   An entity type e.g. node.
 * @param string $bundle_type
 *   A bundle type e.g. event.
 * @param string $master_uuid
 *   the alphanumeric master_uuid.
 *
 * @return bool
 *   the sucess of this functioon is represented by this value.
 */
function _delete_future_dates($entity, $entity_id, $entity_type = 'node', $bundle_type = 'event', $master_uuid = NULL) {

  // Get start date value of current date instance.
  $start_date = $entity->field_date[$entity->language][0]['value'];

  // Instantiate boolean variable to track success of delete operation.
  $deleted_future_dates_successfully = FALSE;

  // Use EFQ to get all dates with the same field_master_uuid value.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle_type)
      // Note: exclude current entity because it will be deleted as part of the
      // hook_entity_delete function that calls this one.
      ->propertyCondition('nid', $entity_id, '<>')
      ->fieldCondition('field_master_uuid', 'value', $master_uuid)
      ->fieldCondition('field_date', 'value', $start_date, '>');

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Delete an array of entities. Returns failure as FALSE, success as NULL.
    $success = entity_delete_multiple($entity_type, $date_entity_ids);
    if ($success !== FALSE) {
      $deleted_future_dates_successfully = TRUE;
    }
  }

  // Need to update RRULE on remaining entities to reflect the fact that some
  // future dates were truncated and so the effective end date of the series
  // should be brought forward.
  // $rrule = date_api_ical_build_rrule($element['#value']);
  return $deleted_future_dates_successfully;
}
