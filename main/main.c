//
//  main.c
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  Created by Andreas Schweizer on 11.01.2017.
//  Copyright © 2017 Classy Code GmbH
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <string.h>
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "main.h"
#include "wifi_sta.h"   // WIFI module configuration, connecting to an access point.
#include "wifi_tls.h"   // TLS server connections and request handling.
#include "iap_https.h"  // Coordinating firmware updates


#define TAG "main"


static const char *server_root_ca_public_key_pem = OTA_SERVER_ROOT_CA_PEM;
static const char *peer_public_key_pem = OTA_PEER_PEM;

static wifi_sta_init_struct_t wifi_params;

// Static because the scope of this object is the usage of the iap_https module.
static iap_https_config_t ota_config;


static void init_wifi();
static void init_ota();
static esp_err_t app_event_handler(void *ctx, system_event_t *event);


void app_main()
{
    ESP_LOGI(TAG, "Intialisation started.");
    ESP_LOGI(TAG, "Software version: %u", SOFTWARE_VERSION);


    // Key-value storage
    
    nvs_flash_init();
    
    
    // Configure the application event handler.
    // The handler is centrally implemented in this module.
    // From here, we delegate the event handling to the responsible modules.
    
    esp_event_loop_init(&app_event_handler, NULL);


    init_wifi();
    init_ota();

    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    while (1) {
        
        // ... Do something useful instead ...
        
        int nofFlashes = 1;
        if (wifi_sta_is_connected()) {
            nofFlashes += 1;
        }
        if (iap_https_update_in_progress()) {
            nofFlashes += 2; // results in 3 (not connected) or 4 (connected) flashes
        }
        
        for (int i = 0; i < nofFlashes; i++) {
            gpio_set_level(GPIO_NUM_5, 1);
            vTaskDelay(150 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_NUM_5, 0);
            vTaskDelay(150 / portTICK_PERIOD_MS);
        }
        
        // Now is a good time to re-boot, if a new firmware image has been loaded.
        /*
         ota2_boot_new_firmware();
         */
        
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    
    // Should never arrive here.
}

static void init_wifi()
{
    ESP_LOGI(TAG, "Set up WIFI network connection.");
    
    wifi_params.network_ssid = WIFI_NETWORK_SSID;
    wifi_params.network_password = WIFI_NETWORK_PASSWORD;
    
    wifi_sta_init(&wifi_params);
}

static void init_ota()
{
    ESP_LOGI(TAG, "Initialising OTA firmware updating.");
    
    ota_config.server_host_name = OTA_SERVER_HOST_NAME;
    ota_config.server_port = "443";
    strncpy(ota_config.server_metadata_path, OTA_SERVER_METADATA_PATH, sizeof(ota_config.server_metadata_path) / sizeof(char));
    bzero(ota_config.server_firmware_path, sizeof(ota_config.server_firmware_path) / sizeof(char));
    ota_config.server_root_ca_public_key_pem = server_root_ca_public_key_pem;
    ota_config.peer_public_key_pem = peer_public_key_pem;
    ota_config.polling_interval_s = OTA_POLLING_INTERVAL_S;
    ota_config.auto_reboot = OTA_AUTO_REBOOT;
    
    iap_https_init(&ota_config);
    
    // Immediately check if there's a new firmware image available.
    iap_https_check_now();
}

static esp_err_t app_event_handler(void *ctx, system_event_t *event)
{
    esp_err_t result = ESP_OK;
    int handled = 0;
    
    ESP_LOGI(TAG, "app_event_handler: event: %d", event->event_id);

    // Delegate all WIFI STA events to the wifi_sta module.
    
    result = wifi_sta_handle_event(ctx, event, &handled);
    if (ESP_OK != result || handled) {
        return result;
    }
    
    // TODO - handle other events
    
    ESP_LOGW(TAG, "app_event_handler: unhandled event: %d", event->event_id);
    return ESP_OK;
}
