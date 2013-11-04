<?php

/**
 * @file
 * Contains functions that support creation of entities with repeating
 * date fields.
 */

/**
 * Clones new entities that are part of a repeating date series.
 */
function create_dates($entity, $entity_type) {

  // Make sure utility functions are available.
  module_load_include('inc', 'date_repeat_entity', 'includes/date_repeat_entity.utility');

  // Get entity wrapper.
  $wrapper = entity_metadata_wrapper($entity_type, $entity);

  // Get bundle type.
  $bundle = $wrapper->getBundle();

  // Check that the entity is of type "node" and bundle of type event
  // We have to do this because we are dependent on field_clone_date and
  // master_uuid, both fields that are tied to event bundles.
  if ($entity_type === 'node') {

    // Process event nodes - ideally we should be able to work on bundles
    // any time but the field_clone_state is part of the event bundle so
    // we need to test for the 'event' bundle type.
    if ($bundle === 'event') {

      // Get value of clone state - a new event node will have the default state
      // of FALSE while a cloned event node will have a state of TRUE.
      $field_clone_state = $wrapper->field_clone_state->value();

      // Clone the new entity and its attached fields using Replicate API
      // module.
      if ($field_clone_state === FALSE) {

        // If the current entity contains a repeating date field then
        // we need to process the entity.
        $repeating_date_field = _get_repeating_date_field($entity_type, $bundle);

        if ($repeating_date_field != NULL) {
          // Create a clone of the entity for each date instance in the series
          // except the first, which will be saved though the submit handler
          // on the node edit form itself.
          if ($entity->is_new) {

            // Get the UUID from the master (original) date.
            $master_uuid = $wrapper->uuid->value();

            // Assign the UUID to the master UUID field.
            // Give the original entity a reference to its own UUID.
            // This will be needed for any updates of the recurring date series.
            $wrapper->field_master_uuid = $master_uuid;

            // Get an array of field data for the current entity.
            $field_name = $repeating_date_field['field_name'];
            $field_data = field_get_items($entity_type, $entity, $field_name);

            // Create a new set of dates by removing the first item in
            // the array - since it is the original (master) entity.
            array_shift($field_data);

            // Clone a new entity for each date in series
            // (except original entity).
            foreach ($field_data as $delta => $datum) {

              // Clone entity.
              $clone_entity = replicate_clone_entity($entity_type, $entity);

              // Get entity wrapper for the cloned node.
              $clone_wrapper = entity_metadata_wrapper($entity_type, $clone_entity);

              // Flag the node as cloned.
              $clone_wrapper->field_clone_state = TRUE;

              // Give the cloned entity a reference to the master UUID.
              $clone_wrapper->field_master_uuid = $master_uuid;

              // Reset the UUID, version UUID and version log string
              // of the cloned entity to differentiate from the
              // original entity.
              $clone_entity->uuid = NULL;
              $clone_entity->vuuid = NULL;
              $clone_entity->log = "cloned from uuid : " . $master_uuid;

              // Replace field array with the row that matches the
              // current delta.  Need to add an offset of 1 to delta
              // because delta values are shifted when we applied
              // array_shift to field_data.
              $clone_entity->field_date[$entity->language] = array_slice($clone_entity->field_date[$entity->language], $delta + 1, 1);

              // Save the cloned entity.
              entity_save($entity_type, $clone_entity);
            }
            // Remove all items from the master entity's field array
            // except the first.
            array_splice($entity->field_date[$entity->language], 1);
          }
        }
      }
    }
  }

  // Reset the flag for clone state, just before saving the entity.
  $entity->field_clone_state = FALSE;
}
