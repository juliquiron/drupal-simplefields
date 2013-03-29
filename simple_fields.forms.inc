<?php

/**
 * @file
 * Administrative interface for custom field type creation.
 */

/**
 * Menu callback; lists all defined fields for quick reference.
 */
function simple_fields_manager_field_condition($form, &$form_state, $instance) {
  dsm($instance);
  $form = array();
  // Token form conditional_fields module .admin.inc file
  $entity_type = variable_get('simple_fields_entity_type','');
  $bundle_name = variable_get('simple_fields_entity_bundle','');
  $location_field = variable_get('simple_fields_location_field','');
  $instance_name = field_read_instances(array('id' => $instance));
  $instance_name = array_shift($instance_name);
  $instance_label = $instance_name['label'];
  $instance_name = $instance_name['field_name'];
  $instances = field_info_instances($entity_type, $bundle_name);
  foreach ($instances as $field) {
    // TODO excloure camps arbitraris definits a la confiugració
    //   comparacio darrais amb is_in
    if($field['field_name'] == $location_field || $field['field_name'] == $instance_name) { continue; }
    $fields[$field['id']] = check_plain($field['label'] . ' (' . $field['field_name'] . ')');
  }
  drupal_set_title(t('Set field dependence for: ') . $instance_label);
  asort($fields);
  $form['depenent'] = array(
      '#type' => 'hidden',
      '#value' => $instance,
    );
  $form['dependee'] = array(
      '#type' => 'select',
      '#title' => t('Set field dependence for: ') . $instance_label,
  //    '#title_display' => 'invisible',
      '#description' => t('Dependee'),
      '#options' => $fields,
    );
  $form =  system_settings_form($form);
  $form['#submit'][] = 'simple_fields_manager_field_condition_submit';
  dsm($form);
  return $form;
}

function simple_fields_manager_location_condition($form, &$form_state, $instance) {
  // Get configuration values
  $entity_type = variable_get('simple_fields_entity_type','');
  $bundle_name = variable_get('simple_fields_entity_bundle','');
  $location_field = variable_get('simple_fields_location_field','');
  // Prepare the location field
  $dependee = field_info_field($location_field);
  $instance_name = field_read_instances(array('id' => $instance));
  $instance_name = array_shift($instance_name);
  $instance_label = $instance_name['label'];
  $instance_name = $instance_name['field_name'];
  $instances = field_read_instances(array('field_id' => $dependee['id'], 'entity_type' => $entity_type, 'bundle' => $bundle_name));
  $dependee_instance = array_shift($instances);
  $dependee_instance['default_value_function'] = '';
  $dummy_form = array('#parents' => array());
  $field = field_default_form($entity_type, NULL, $dependee, $dependee_instance, LANGUAGE_NONE, array(), $dummy_form, $form_state);
  dsm($field);
  $conditions = conditional_fields_load_dependencies($entity_type, $bundle_name);
  foreach ( $conditions['dependees'][$location_field] as $id => $cond) {
    /** We assume there is only one dependency in this instance **/
    if($cond['dependent'] == $instance_name) {
      // We are getting the id of the condition we whant to change 
      foreach($cond['options']['value'] as $k => $v) {
        //$field[$location_field]['und']['#default_value'][$v['tid']] = $v;
        $field[$location_field]['und']['#default_value'][$v['tid']] = $v['tid'];
      }
    }
  }
  $field[$location_field]['und']['#title'] = t('Set location condition for: ') . $instance_label;
  drupal_set_title(t('Set location condition for: ') . $instance_label);
  dsm($field);
  $field['depenent'] = array(
      '#type' => 'hidden',
      '#value' => $instance,
    );
  $field['dependee'] = array(
      '#type' => 'hidden',
      '#value' => $dependee_instance['id'],
    );
  $field['#submit'][] = 'simple_fields_manager_location_condition_submit';
//  $field[$location_field]['und']['#type'] = 'select';
//  $field[$location_field]['und']['#multiple'] = TRUE;
  $field[$location_field]['und']['#max_choices'] = -1;
  $field[$location_field]['und']['#leaves_only'] = 0;
  $field[$location_field]['und']['#cascading_selection'] = 1;
  
  return system_settings_form($field);
}
function simple_fields_manager_location_condition_submit($form, &$form_state) {
  /**
   * Prepare data
   */
  $entity_type = variable_get('simple_fields_entity_type','');
  $bundle_name = variable_get('simple_fields_entity_bundle','');
  $location_field = variable_get('simple_fields_location_field','');
  $instance_name = field_read_instances(array('id' => $form_state['values']['depenent']));
  $instance_name = array_shift($instance_name);
  $instance_name = $instance_name['field_name'];
  /** 
   * Clear exsistent conditions
   */
  $conditions = conditional_fields_load_dependencies($entity_type, $bundle_name);
  foreach ( $conditions['dependees'][$location_field] as $id => $cond) {
    if($cond['dependent'] == $instance_name) {
      conditional_fields_dependency_delete(array($id));
    }
  }
  /**
   * Create neu conditions
   */
  $options = array();
  $options['state'] =  'visible';
  $options['condition'] =  'value';
  $options['grouping'] =  'OR';
  $options['effect'] =  'slide';
  $options['effect_options'] = array('speed' =>  '400');
  $options['selector'] =  '';
  $options['values_set'] =  '1';
  $options['value_form'] =  NULL;
  foreach($form_state['values'][$location_field]['und'] as $k => $v) {
    $options['value'] = array(0=> $v);
    $options['value_form'] = $v['tid'];
    conditional_fields_dependency_insert($form_state['values']['dependee'],$form_state['values']['depenent'], $options);
  }
  drupal_set_message(t('condition introduced'));
} 

function simple_fields_manager_field_condition_submit($form, &$form_state) {
  $options = array();
  $options['state'] =  'visible';
  $options['condition'] =  'value';
  $options['grouping'] =  'AND';
  $options['effect'] =  'slide';
  $options['effect_options'] = array('speed' =>  '400');
  $options['selector'] =  '';
  $options['values_set'] =  '1';
  $options['value_form'] =  NULL;
  $options['value'] = array(0=>array('value' => NULL));
  conditional_fields_dependency_insert($form_state['values']['dependee'],$form_state['values']['depenent'], $options);
  drupal_set_message(t('condition introduced'));
}

/**
 * Form constructor for the field instance settings form.
 *
 *  Taken from fields_ui.admin.inc
 * @see field_ui_field_edit_form_validate()
 * @see field_ui_field_edit_form_submit()
 * @ingroup forms
 */
function simple_fields_field_edit_label_form($form, &$form_state, $instance) {
  drupal_set_title($instance_label);
  // Prepare the location field
  $instance_name = field_read_instances(array('id' => $instance));
  $instance_name = array_shift($instance_name);
  $instance_label = $instance_name['label'];
  $instance_name = $instance_name['field_name'];
  $field = field_info_field($instance_name);
  if (!empty($field['locked'])) {
    $form['locked'] = array(
      '#markup' => t('The field %field is locked and cannot be edited.', array('%field' => $instance['label'])),
    );
    return $form;
  }
  $form['instance']['field_label'] = array(
    '#type' => 'textfield',
    '#title' => t('New label for: ') . $instance_label . '(' . $instance_name .').',
    '#default_value' => !empty($instance_labellabel) ? $instance_label : $instance_name,
    '#required' => TRUE,
    '#weight' => -20,
  );
  // Build the non-configurable instance values.
  $form['instance']['field_name'] = array(
    '#type' => 'value',
    '#value' => $instance['field_name'],
  );
  return $form;
}

function simple_fields_manager_admin($form, &$form_state) {
  require_once DRUPAL_ROOT . '/' . drupal_get_path('module', 'field_ui') . '/field_ui.admin.inc'; 
  $field_type_options = field_ui_field_type_options();
  $widget_type_options = field_ui_widget_type_options(NULL, TRUE);
  $form['simple_fields_entity_type'] = array ( // Select the entity to work with
    '#type' => 'textfield',
    '#title' => t('Select the entity type to work with'),
    '#default_value' => variable_get('simple_fields_entity_type', ''),
    // TODO CARREGAR LA LLISTA DE VALORS POSIBLES VIA API);
    );
  $form['simple_fields_entity_bundle'] = array ( //Select the entity bundle to work with
    '#type' => 'textfield',
    '#title' => t('Select the entity type to work with'),
    '#default_value' => variable_get('simple_fields_entity_bundle', ''),
    // TODO CARREGAR LA LLISTA DE VALORS POSIBLES VIA API);
    );
  $form['simple_fields_location_field'] = array (
    '#type' => 'textfield',
    '#title' => t('Location field name'),
    '#default_value' => variable_get('simple_fields_location_field',''),
    );
  $form['simple_fields_field_type'] = array (
        '#title' => t('Type of the data to store'),
        '#type' => 'select',
        '#title' => t('Type of new field'),
        '#title_display' => 'invisible',
        '#options' => $field_type_options,
        '#empty_option' => t('- Select a field type -'),
        '#description' => t('Type of data to store.'),
        '#attributes' => array('class' => array('field-type-select')),
        '#prefix' => '<div class="add-new-placeholder">&nbsp;</div>',
        '#default_value' => variable_get('simple_fields_field_type' ,''),
    );
  $form['simple_fields_field_widget'] = array (
        '#title' => t('Form element to edit the data'),
        '#type' => 'select',
        '#title' => t('Widget for new field'),
        '#title_display' => 'invisible',
        '#options' => $widget_type_options,
        '#empty_option' => t('- Select a widget -'),
        '#description' => t('Form element to edit the data.'),
        '#attributes' => array('class' => array('widget-type-select')),
        '#cell_attributes' => array('colspan' => 3),
        '#prefix' => '<div class="add-new-placeholder">&nbsp;</div>',
        '#default_value' => variable_get('simple_fields_field_widget', ''),
      );
 
  return system_settings_form($form);
}

