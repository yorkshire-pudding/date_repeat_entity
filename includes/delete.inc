<?php

/**
 * @file
 * Contains functions that support deletions of entities with repeating date fields.
 */

/**
 * Custom submit handler called when an entity deleted.
 * @param type $form
 * @param type $form_state
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

  _delete_all_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);

}

/**
 * Custom submit handler called when an entity deleted.
 * @param type $form
 * @param type $form_state
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

  _delete_future_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);

}

/**
 * Implements hook_entity_delete().
 */
function date_repeat_entity_delete($entity, $entity_type) {

  // TODO: remove this function if the better approach is to use form submit
  // handlers - to avoid recursion on hook_entity_delete.
  // _delete_dates($entity, $entity_type);
}

/**
 * Deletes one of more date instances of a entity with a repeating date field.
 *
 * If a deleted entity has a recurring date field, then any instances of the
 * same recurring date pattern may also need to be deleted, depending upon the
 * state of the field_change_repeat_scope field i.e the user may wish to 
 * delete the single instance, all instances in the series or just instances in
 * the future.
 * 
 * @param text $change_repeat_scope
 *   defines whether all dates or just current and future dates are affected.
 * @param entity object $entity
 * @param text $entity_type
 */
function _delete_dates($change_repeat_scope, $entity, $entity_type) {

  // Instantiate boolean variable to track success of delete operation.
  $deleted_dates_successfully = FALSE;

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get entity id.
  $entity_id = $wrapper->getIdentifier();

  // Get bundle type.
  $bundle_type = $wrapper->getBundle();

  // Get the series master UUID.
  $master_uuid = $wrapper->field_master_uuid->value();

  switch ($change_repeat_scope) {
    case "All":
      // Process all dates.
      $deleted_dates_successfully = _delete_all_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);
      break;

    case "Future":
      // Process current and future dates.
      $deleted_dates_successfully = _delete_future_dates($entity, $entity_id, $entity_type, $bundle_type, $master_uuid);
      break;

    case "Single":
      // Process current date only.  Should not have to do anything because
      // form_node_delete_confirm form is already taking care of the the current
      // instance.
      break;

    default:
    // Throw exception since every user response should
    // satisfy one of the above three constraints.
  }
}

/**
 * Delete all dates associated with a repeating date series.
 * 
 * @param int $entity_id
 *   A numeric identifier for an identity 
 * 
 * @param text $entity_type
 *   A entity type e.g. node.
 * 
 * @param text $bundle_type
 *   A bundle type e.g. event.* 
 */
function _delete_all_dates($entity, $entity_id, $entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  // Instantiate boolean variable to track success of delete operation.
  $deleted_all_dates_successfully = FALSE;

  // Use EFQ to get all dates with the same field_master_uuid value
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
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
 * @param int $entity_id
 *   A numeric identifier for an identity 
 * 
 * @param text $entity_type
 *   A entity type e.g. node.
 * 
 * @param text $bundle_type
 *   A bundle type e.g. event.* 
 */
function _delete_future_dates($entity, $entity_id, $entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  // Get start date value of current date instance.
  $start_date = $entity->field_date[$entity->language][0]['value'];

  // Instantiate boolean variable to track success of delete operation.
  $deletedfuture_dates_successfully = FALSE;

  // Use EFQ to get all dates with the same field_master_uuid value
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
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
      $deletedfuture_dates_successfully = TRUE;
    }
  }

  // Need to update RRULE on remaining entities to reflect the fact that some
  // future dates were truncated and so the effective end date of the series
  // should be brought forward.
  // $rrule = date_api_ical_build_rrule($element['#value']);




  return $deletedfuture_dates_successfully;
}
