<?php

/**
 * @file
 * Contains functions that support deletion of entities with
 * repeating date fields.
 */

/**
 * Update all entities related to the one represented by the current form.
 *
 * @param object $entity
 * @param string $entity_type
 * @param string $scope 
 */
function delete_dates($entity, $entity_type, $scope = 'all') {

  // Get entity wrapper.
  $entity_wrapper = entity_metadata_wrapper($entity_type, $entity);
  $bundle = $entity_wrapper->getBundle();

  $repeating_date_operation_successful = _delete_dates($entity_wrapper, $entity_type, $bundle, $scope);

  if ($repeating_date_operation_successful) {
    // We still need to delete the original
    //node_delete_confirm_submit($form, $form_state);
  }

  // Go to front page
  drupal_goto('<front>');
}

/**
 * Delete dates associated with a repeating date series.
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
function _delete_dates($entity_wrapper, $entity_type = 'node', $bundle = 'event', $scope = 'current') {

  // Instantiate boolean variable to track success of operation.
  $repeating_date_operation_successful = FALSE;

  // Make sure utility functions are available.
  module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.utility');
  
  // Get dates
  $result = _get_dates($entity_wrapper, $entity_type, $bundle, $scope);

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);

    // Delete an array of entities. Returns failure as FALSE, success as NULL.
    // Note: referenced entities are also deleted if the option is checked on
    // the entityreference field configuration form.
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

