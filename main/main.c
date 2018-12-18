//
//  main.c
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  Created by Andreas Schweizer on 11.01.2017.
//  Copyright © 2017 Classy Code GmbH
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this 
// software and associated documentation files (the "Software"), to deal in the Software 
// without restriction, including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
// permit persons to whom the Software is furnished to do so, subject to the following 
// conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies 
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <string.h>
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "main.h"
#include "wifi_sta.h"   // WIFI module configuration, connecting to an access point.
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
    ESP_LOGI(TAG, "---------- Intialization started ----------");
    ESP_LOGI(TAG, "---------- Software version: %2d -----------", SOFTWARE_VERSION);

    
    nvs_flash_init();
    
    
    // Configure the application event handler.
    // The handler is centrally implemented in this module.
    // From here, we delegate the event handling to the responsible modules.
    
    esp_event_loop_init(&app_event_handler, NULL);


    // Configure the WIFI module. This module maintains the connection to the
    // defined access point.

    init_wifi();
    
    
    // Configure the over-the-air update module. This module periodically checks
    // for firmware updates by polling a web server. If an update is available,
    // the module downloads and installs it.
    
    init_ota();


    // This application doesn't actually do anything useful.
    // It just lets an LED blink. You may need to adapt this for your own module
    // (GPIO5 is the blue LED on the "ESP32 Thing" module.)

    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    while (1) {
        
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
        
        // If the application could only re-boot at certain points, you could
        // manually query iap_https_new_firmware_installed and manually trigger
        // the re-boot. What we do in this example is to let the firmware updater
        // re-boot automatically after installing the update (see init_ota below).
        //
        // if (iap_https_new_firmware_installed()) {
        //     ESP_LOGI(TAG, "New firmware has been installed - rebooting...");
        //     esp_restart();
        // }
        
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
    
    ota_config.current_software_version = SOFTWARE_VERSION;
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

    // Let the wifi_sta module handle all WIFI STA events.
    
    result = wifi_sta_handle_event(ctx, event, &handled);
    if (ESP_OK != result || handled) {
        return result;
    }
    
    // TODO - handle other events
    
    ESP_LOGW(TAG, "app_event_handler: unhandled event: %d", event->event_id);
    return ESP_OK;
}
