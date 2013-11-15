<?php

/**
 * @file
 * Hooks provided by the Date Repeat Entity module.
 *
 */

/**
 * Determines if an entity with a repeating date has changed.
 *
 * Compares the start date, end date and RRULE of new and old entity.
 *
 * @param stdClass $original_entity
 *   The original entity before being updated.
 * @param stdClass $updated_entity
 *   The updated entity.
 *
 * @return bool
 *   indicates if the date values of an entity with a repeating date
 *   have materially changed from original to current state.
 */
function hook_repeating_date_has_materially_changed($original_entity, $updated_entity) {



}

