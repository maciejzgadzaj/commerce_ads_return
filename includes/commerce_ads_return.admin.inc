<?php

/**
 * @file
 * Administrative page callbacks for the Commerce ADS Return module.
 */

/**
 * Configuration form to store return settings.
 */
function commerce_ads_return_configure_form($form, &$form_state) {
  $commerce_ads_ws = variable_get('commerce_ads_ws', array());

  // Synchronization frequency.
  $form['ads_return_synchronization'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Synchronization'),
    '#description' => t('Settings for sending new returns to ADS.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['ads_return_synchronization']['ads_return_send_frequency'] = array(
    '#type' => 'select',
    '#title' => t('Synchronization frequency'),
    '#description' => t('When to call ADS for updating return statuses.'),
    '#options' => commerce_ads_return_frequency_options(),
    '#default_value' => !empty($commerce_ads_ws['ads_return_send_frequency']) ? $commerce_ads_ws['ads_return_send_frequency'] : '',
  );
  $options = commerce_return_status_options_list();
  if (!empty($commerce_ads_ws['ads_return_status'])) {
    $default = $commerce_ads_ws['ads_return_status'];
  }
  else {
    $default = 'waiting_received';
  }
  $form['ads_return_synchronization']['status'] = array(
    '#type' => 'select',
    '#title' => t('Status of the returns to send'),
    '#options' => $options,
    '#description' => t('Only returns with this status will be sent to ADS.'),
    '#default_value' => $default,
  );
  $form['ads_return_synchronization']['ads_send_returns'] = array(
    '#type' => 'submit',
    '#value' => t('Send returns to ADS now'),
    '#submit' => array('commerce_ads_return_send_all_returns'),
  );

  // Update frequency.
  $form['ads_return_update'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Update'),
    '#description' => t('Settings for updating existing returns from ADS.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['ads_return_update']['ads_return_update_frequency'] = array(
    '#type' => 'select',
    '#title' => t('Return update frequency'),
    '#description' => t('Update frequency of returns once they have been sent to ADS'),
    '#options' => commerce_ads_return_frequency_options(),
    '#default_value' => !empty($commerce_ads_ws['ads_return_update_frequency']) ? $commerce_ads_ws['ads_return_update_frequency'] : '',
  );
  $options = commerce_return_status_options_list();
  $form['ads_return_update']['ads_return_update_end'] = array(
    '#type' => 'select',
    '#title' => t('Return blocking status'),
    '#options' => $options,
    '#description' => t('Select the return status for which returns will not be updated any more.'),
    '#default_value' => !empty($commerce_ads_ws['ads_return_update_end']) ? $commerce_ads_ws['ads_return_update_end'] : array(),
    '#multiple' => TRUE,
  );
  $form['ads_return_update']['ads_update_returns'] = array(
    '#type' => 'submit',
    '#value' => t('Update returns from ADS now'),
    '#submit' => array('commerce_ads_return_update_all_returns'),
  );

  // Return statuses.
  $form['ads_return_statuses'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('ADS status mapping'),
    '#description' => t('Mapping of ADS return statuses to their local statuses used for update.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  if (!empty($commerce_ads_ws['ads_base_url']) && !empty($commerce_ads_ws['ads_authorization_key'])) {
    foreach (commerce_ads_return_statuses() as $ads_status) {
      $form['ads_return_statuses'][$ads_status['StateId']] = array(
        '#type' => 'select',
        '#title' => t($ads_status['Label']),
        '#options' => commerce_return_status_options_list(),
        '#default_value' => !empty($commerce_ads_ws['ads_return_statuses'][$ads_status['StateId']]) ? $commerce_ads_ws['ads_return_statuses'][$ads_status['StateId']] : '',
      );
    }
  }
  else {
    $form['ads_return_statuses']['help'] = array(
      '#markup' => t('You have to set shopip in "Configure" tab first'),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  return $form;
}

function commerce_ads_return_configure_form_submit($form, &$form_state) {
  $commerce_ads_ws = variable_get('commerce_ads_ws', array());
  if (!empty($form_state['values']['ads_return_synchronization']['status'])) {
    if (isset($commerce_ads_ws['ads_return_status'])) {
      $old_status = $commerce_ads_ws['ads_return_status'];
    }
    $commerce_ads_ws['ads_return_status'] = $form_state['values']['ads_return_synchronization']['status'];
  }
  if (!empty($form_state['values']['ads_return_statuses'])) {
    $commerce_ads_ws['ads_return_statuses'] = $form_state['values']['ads_return_statuses'];
  }
  if (!empty($form_state['values']['ads_return_synchronization']['ads_return_send_frequency'])) {
    $commerce_ads_ws['ads_return_send_frequency'] = $form_state['values']['ads_return_synchronization']['ads_return_send_frequency'];
  }
  if (!empty($form_state['values']['ads_return_update']['ads_return_update_frequency'])) {
    $commerce_ads_ws['ads_return_update_frequency'] = $form_state['values']['ads_return_update']['ads_return_update_frequency'];
  }
  if (!empty($form_state['values']['ads_return_update']['ads_return_update_end'])) {
    $commerce_ads_ws['ads_return_update_end'] = $form_state['values']['ads_return_update']['ads_return_update_end'];
  }
  variable_set('commerce_ads_ws', $commerce_ads_ws);

  // Since a rule depend on this status, we have to revert it.
  if (isset($old_status) && $old_status != $commerce_ads_ws['ads_return_status']) {
    $rules_config = entity_load_single('rules_config', 'rules_send_return_to_ads');
    $edit_link = l(t('edit'), RulesPluginUI::path($rules_config->name));
    $vars = array('%plugin' => $rules_config->plugin(), '%label' => $rules_config->label());
    $rules_config->delete();
    watchdog('rules', 'Reverted %plugin %label to the defaults.', $vars, WATCHDOG_NOTICE, $edit_link);
  }

  drupal_set_message(t('Configuration saved'));
}

/**
 * Get the list of ADS return statuses.
 *
 * @return array
 *  The list of return status on ADS side.
 */
function commerce_ads_return_statuses() {
  $commerce_ads_ws = variable_get('commerce_ads_ws', array());
  $response = commerce_ads_request_ads('api/Returns/getStatusList', array(
    'method' => 'GET',
    'timeout' => 6000,
    'headers' => array(
      'Authorization' => !empty($commerce_ads_ws['ads_authorization_key']) ? $commerce_ads_ws['ads_authorization_key'] : '',
    ),
  ));
  if ($response['Code'] === 0) {
    return $response['Data'];
  }
  else {
    return array();
  }
}

/**
 * Returns options array for cron frequency form select element.
 *
 * @return array
 *   Options array for cron frequency form select element.
 */
function commerce_ads_return_frequency_options() {
  $frequency_options = array(
    '' => t('Select a value'),
    '300' => t('5 minutes'),
    '600' => t('10 minutes'),
    '1800' => t('30 minutes'),
    '3600' => t('1 hour'),
    '7200' => t('2 hours'),
    '21600' => t('6 hours'),
    '43200' => t('12 hours'),
    '86400' => t('1 day'),
  );

  // Allow other modules to alter frequency options.
  drupal_alter('commerce_ads_return_frequency_options', $frequency_options);

  return $frequency_options;
}

/**
 * Returns options array for cron frequency form select element.
 *
 * @return array
 *   Options array for cron frequency form select element.
 */
function commerce_ads_return_max_attempts_options() {
  $attempts_number = array(
    '' => t('Select a value'),
    '10' => t('10 times'),
    '20' => t('20 times'),
    '30' => t('30 times'),
    '40' => t('40 times'),
  );

  // Allow other modules to alter frequency options.
  drupal_alter('commerce_ads_return_max_attempts_options', $attempts_number);

  return $attempts_number;
}
