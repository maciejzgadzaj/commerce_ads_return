<?php

/**
 * @file
 * Rules callbacks for the Commerce ADS Return module.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_ads_return_default_rules_configuration() {
  $commerce_ads_ws = variable_get('commerce_ads_ws', array());
  $items = array();
  if (!empty($commerce_ads_ws['ads_return_status'])) {
    $items['rules_send_return_to_ads'] = entity_import('rules_config', '{ "rules_send_return_to_ads" : {
        "LABEL" : "Send return to ADS",
        "PLUGIN" : "reaction rule",
        "OWNER" : "rules",
        "REQUIRES" : [ "rules", "commerce_ads_return", "commerce_return" ],
        "ON" : { "commerce_return_update" : [] },
        "IF" : [
          { "data_is" : { "data" : [ "commerce-return:status" ], "value" : "' . $commerce_ads_ws['ads_return_status'] . '" } },
          { "NOT data_is" : {
              "data" : [ "commerce-return-unchanged:status" ],
              "value" : "' . $commerce_ads_ws['ads_return_status'] . '"
            }
          }
        ],
        "DO" : [
          { "commerce_ads_return_send_return" : { "return" : [ "commerce-return" ] } }
        ]
      }
    }');
  }
  return $items;
}
