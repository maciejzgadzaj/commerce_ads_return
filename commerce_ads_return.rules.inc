<?php

/**
 * @file
 * Rules callbacks for the Commerce ADS Return module.
 */

/**
 * Implements hook_rules_action_info().
 */
function commerce_ads_return_rules_action_info() {
  return array(
    'commerce_ads_return_send_return' => array(
      'label' => t('Send return to ads'),
      'parameter' => array(
        'return' => array(
          'type' => 'commerce_return',
          'label' => t('Commerce Return'),
        ),
      ),
      'group' => t('Commerce ADS'),
    ),
  );
}
