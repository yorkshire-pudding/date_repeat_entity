<?php

/**
 * @file
 * Hooks provided by the Date Repeat Entity module.
 *
 */

/**
 * Determines if an entity with a repeating date has changed.
 *
 * @param stdClass $original_entity
 *   The original entity before being updated.
 * @param stdClass $updated_entity
 *   The updated entity.
 * @param string $entity_type
 *   The entity type
 *
 * @return bool
 *   indicates if the date values of an entity with a repeating date
 *   have materially changed from original to current state.
 */
function hook_repeat_entity_repeating_date_has_changed($original_entity, $updated_entity, $entity_type = 'node') {

  $repeating_date_has_changed = FALSE;

  // Check that entity aleady exists - we are not validating new entities.
  if (!is_null($original_entity) && !is_null($updated_entity)) {

    // Create two wrappers
    $original_wrapper = entity_metadata_wrapper($entity_type, $original_entity);
    $updated_wrapper = entity_metadata_wrapper($entity_type, $updated_entity);

    // Check that the entity form has a repeating date field and a field that
    $bundle = $original_wrapper->getBundle();

    // Make sure utility functions are available.
    module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.utility');
    $repeating_date_field = date_repeat_entity_get_repeating_date_field($entity_type, $bundle);

    if ($repeating_date_field != NULL) {

      // Get the name of the repeating field.
      $repeating_date_field_name = $repeating_date_field['field_name'];

      // Get original date field properties.
      $original_date = $original_wrapper->{$repeating_date_field_name}[0]->value();
      $original_date_start_value = $original_date['value'];
      $original_date_end_value = $original_date['value2'];
      $original_rrule = $original_date['rrule'];

      // Get updated date field properties.
      $updated_date = $updated_wrapper->{$repeating_date_field_name}[0]->value();
      $updated_date_start_value = $updated_date['value'];
      $updated_date_end_value = $updated_date['value2'];
      $updated_rrule = $updated_date['rrule'];

      // Check if the entity date has changed to the extent that
      // the repeating date series has changed and therefore dependent data
      // like date exceptions and referencing entities will need to be reset.
      $updated_date_data = array(
        $updated_date_start_value,
        $updated_date_end_value,
        $updated_rrule,
      );

      $original_date_data = array(
        $original_date_start_value,
        $original_date_end_value,
        $original_rrule,
      );

      if ($updated_date_data !== $original_date_data) {
        $repeating_date_has_changed = TRUE;
      }

    }
  }
  return $repeating_date_has_changed;

}

