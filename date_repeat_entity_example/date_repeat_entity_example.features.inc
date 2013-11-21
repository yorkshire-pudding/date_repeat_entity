<?php
/**
 * @file
 * date_repeat_entity_example.features.inc
 */

/**
 * Implements hook_ctools_plugin_api().
 */
function date_repeat_entity_example_ctools_plugin_api($module = NULL, $api = NULL) {
  if ($module == "strongarm" && $api == "strongarm") {
    return array("version" => "1");
  }
}

/**
 * Implements hook_views_api().
 */
function date_repeat_entity_example_views_api($module = NULL, $api = NULL) {
  return array("api" => "3.0");
}

/**
 * Implements hook_node_info().
 */
function date_repeat_entity_example_node_info() {
  $items = array(
    'event' => array(
      'name' => t('Event'),
      'base' => 'node_content',
      'description' => t('An activity that has an associated start date.  An event can optionally have an end date, repeat at regular intervals and occur on part of a day or all day.'),
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  return $items;
}
