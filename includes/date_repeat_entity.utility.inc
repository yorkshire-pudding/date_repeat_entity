<?php
/**
 * @file
 * Contains utility functions that support operations on entities with
 * repeating date fields.
 */

/**
 * Returns an entity object from a master uuid.
 * 
 * @param string $entity_type
 *   the entity type to load, e.g. node.
 * @param string $bundle
 *   the bundle type, e.g. event.
 * @param string $master_uuid
 *   the alphanumeric master_uuid.
 * 
 * @return object
 *   representing an entity.
 */
function _get_master_entity($entity_type = 'node', $bundle = 'event', $master_uuid = NULL) {

  // Instantiate array to be returned.
  $master_entity = array();

  // Use EFQ to retrieve master entity.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
      ->propertyCondition('uuid', $master_uuid);

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Get array of entity ids.
    $entity_ids = array_keys($result[$entity_type]);

    // Retrieve entities based on entity ids.
    $master_entities = entity_load($entity_type, $entity_ids);

    // Get first element in array (there should only be one).
    $master_entity = reset($master_entities);
  }

  return $master_entity;
}

/**
 * Returns an entity object from an entity uuid.
 *
 * @param string $entity_type
 *   the entity type e.g. node.
 * @param string $bundle
 *   the bundle type e.g. event.
 * @param int $entity_id
 *   the entity identifier e.g. node id.
 * 
 * @return object
 *   representing an array.
 */
function _get_entity($entity_type = 'node', $bundle = 'event', $entity_id = NULL) {

  // Instantiate array to be returned.
  $entity = array();

  // Use EFQ to retrieve master entity.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
      ->entityCondition('entity_id', array($entity_id), 'IN');

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Get array of entity ids.
    $entity_ids = array_keys($result[$entity_type]);

    // Retrieve entities based on entity ids.
    $entities = entity_load($entity_type, $entity_ids);

    // Get first element in array (there should only be one).
    $entity = reset($entities);
  }

  return $entity;
}

/**
 * Returns the name of a repeating date field, where it exists.
 *
 * Searches database field definition for a repeating date field which is used
 * in the $entity_type (e.g. node) and $bundle (e.g. event) passed as
 * arguments.
 *
 * Note: Function returns once the first match of a repeating date field
 * is found and does not consider the scenario where there may be more than
 * one repeating date field associated with the $entity_type and $bundle.
 *
 * @param string $entity_type
 *   an entity type e.g. node.
 * @param string $bundle
 *   a bundle type e.g. event.
 * 
 * @return array
 *   representing a field.
 */
function _get_repeating_date_field($entity_type, $bundle) {

  // Set a default value of NULL.
  $repeating_date_field = NULL;

  // Retrieve meta information about all of the fields in the database.
  $fields = field_info_fields();

  // Loop through the fields.
  foreach ($fields as $field) {

    // Check if the field is a repeating date field.
    if (in_array($field['type'], array('date', 'datestamp', 'datetime')) && date_is_repeat_field($field)) {

      // Loop through the entity types associated with this field.
      foreach ($field['bundles'] as $field_entity_type => $bundles) {

        // Loop through the bundles (content types - in the case of node
        // entity types) associated with this entity type.
        foreach ($bundles as $field_bundle) {

          // If the entity type and bundle type passed to this function are the
          // same as the entity type and bundle of the repeating date field then
          // we have a match.
          if ($entity_type == $field_entity_type && $bundle == $field_bundle) {
            $repeating_date_field = $field;
            return $repeating_date_field;
          }
        }
      }
    }
  }
  return $repeating_date_field;
}

/**
 * Returns a field for a given entity type, bundle and field machine name.
 *
 * Searches database field definition for a field which is used
 * in the $entity_type (e.g. node) and $bundle (e.g. event)
 *
 * @param string $entity_type
 *   an entity type e.g. node.
 * @param string $bundle
 *   a bundle type e.g. event.
 * @param string $name
 *   a field machine name e.g. field_master_uuid.
 *
 * @return array
 *   representing a field.
 */
function _get_field($entity_type, $bundle, $name) {

  // Set a default value of NULL.
  $field_match = NULL;

  // Retrieve meta information about all of the fields in the database.
  $fields = field_info_fields();

  // Loop through the fields.
  foreach ($fields as $field_name => $field) {

      // Loop through the entity types associated with this field.
      foreach ($field['bundles'] as $field_entity_type => $bundles) {

        // Loop through the bundles (content types - in the case of node
        // entity types) associated with this entity type.
        foreach ($bundles as $field_bundle) {

          // If the entity type, bundle and field name passed to this function
          // are the same as the entity type and bundle of the field then
          // we have a match.
          if ($entity_type == $field_entity_type &&
              $bundle == $field_bundle &&
              $name == $field_name) {
            $field_match = $field;
          }
        }
      }
  }
  return $field_match;
}

/**
 * Returns an array of entity reference field names associate with a bundle.
 *
 * Searches database field definition for all entity reference fields used
 * in the $entity_type (e.g. node) and $bundle (e.g. event) passed as
 * arguments.
 *
 * @param string $entity_type
 *   an entity type e.g. node.
 * @param string $bundle
 *   a bundle type e.g. event.
 * 
 * @return array
 *   an array of entity reference fields.
 */
function _get_entity_reference_fields($entity_type, $bundle) {

  // Set a default value of NULL.
  $entity_reference_fields = array();

  // Retrieve meta information about all of the fields in the database.
  $fields = field_info_fields();

  // Loop through the fields.
  foreach ($fields as $field) {

    // Check if the field is an entity reference date field.
    if (in_array($field['type'], array('entityreference')) ) {

      // Loop through the entity types associated with this field.
      foreach ($field['bundles'] as $field_entity_type => $bundles) {

        // Loop through the bundles (content types - in the case of node
        // entity types) associated with this entity type.
        foreach ($bundles as $field_bundle) {

          // If the entity type and bundle type passed to this function are the
          // same as the entity type and bundle of the repeating date field then
          // we have a match.
          if ($entity_type == $field_entity_type && $bundle == $field_bundle) {
            $entity_reference_fields[] = $field;
          }
        }
      }
    }
  }
  return $entity_reference_fields;
}

/**
 * Gets an array of entity ids with the same master uuid.
 * 
 * @param type $entity_type
 * @param type $bundle
 * @param type $master_uuid
 * 
 * @return array of entity ids
 */
function _get_related_entity_ids($entity_type, $bundle, $master_uuid){

$date_entity_ids = array();
  
 // Use EFQ to get all dates with the same field_master_uuid value.
  $query = new EntityFieldQuery();

  // Define EFQ conditions based on the entity type, bundle and master_uuid.
  $query->entityCondition('entity_type', $entity_type)
      ->entityCondition('bundle', $bundle)
      ->fieldCondition('field_master_uuid', 'value', $master_uuid);

  // Retrieve matching entities.
  $result = $query->execute();

  // If query successful, update retrieved entities.
  if (isset($result[$entity_type])) {

    // Build an array of entity ids for for the entity type.
    $date_entity_ids = array_keys($result[$entity_type]);
  }
  
  return $date_entity_ids;
}