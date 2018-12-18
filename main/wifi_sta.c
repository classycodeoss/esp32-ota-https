//
//  wifi_sta.c
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  This module is responsible for establishing and maintaining the
//  WIFI connection to the defined access point.
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
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "wifi_sta.h"


#define TAG "wifi_sta"


// Our event group to manage the "connected" state.
static EventGroupHandle_t wifi_sta_event_group;


static void wifi_sta_set_connected(bool c);

esp_err_t wifi_sta_init(wifi_sta_init_struct_t *param)
{
    // Let's first validate the input parameters.

    static wifi_config_t wifi_sta_config_struct;
    
    if (strlen(param->network_ssid) >= sizeof(wifi_sta_config_struct.sta.ssid) / sizeof(char)) {
        ESP_LOGE(TAG, "wifi_sta_init: invalid parameter: network_ssid too long");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(param->network_password) >= sizeof(wifi_sta_config_struct.sta.password) / sizeof(char)) {
        ESP_LOGE(TAG, "wifi_sta_init: invalid parameter: network_password too long");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "wifi_sta_init: network = '%s'", param->network_ssid);

    // Initialize the TCP/IP and WIFI functionalities.
    
    tcpip_adapter_init();
    
    // Init WIFI (driver memory, buffers and so on).
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_init");
    wifi_init_config_t init_config_struct = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t init_result = esp_wifi_init(&init_config_struct);
    if (ESP_OK != init_result) {
        // typically ESP_ERR_WIFI_NO_MEM, ...
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_init failed: %d", init_result);
        return init_result;
    }
    
    // Define the configuration storage.
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_set_storage");
    esp_err_t storage_result = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ESP_OK != storage_result) {
        // typically ESP_ERR_WIFI_NOT_INIT, ESP_ERR_WIFI_ARG, ...
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_storage failed: %d", storage_result);
        return storage_result;
    }

    // Put the ESP32 WIFI in STA mode.
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_set_mode");
    esp_err_t set_mode_result = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ESP_OK != set_mode_result) {
        // typically ESP_ERR_WIFI_NOT_INIT, ESP_ERR_WIFI_ARG, ...
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_mode failed: %d", set_mode_result);
        return set_mode_result;
    }

    // Define the configuration for the ESP32 STA mode.
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_set_config");
    strcpy((char*)wifi_sta_config_struct.sta.ssid, param->network_ssid); // max 32 bytes
    strcpy((char*)wifi_sta_config_struct.sta.password, param->network_password); // max 32 bytes
    wifi_sta_config_struct.sta.bssid_set = false;
    esp_err_t set_config_result = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_sta_config_struct);
    if (ESP_OK != set_config_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_config failed: %d", set_config_result);
        return set_config_result;
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS); // WORKAROUND
    wifi_sta_event_group = xEventGroupCreate();
 
    // Start WIFI according to the current configuration.
    ESP_LOGD(TAG, "wifi_sta_init: esp_wifi_start");
    esp_err_t start_result = esp_wifi_start();
    if (ESP_OK != start_result) {
        ESP_LOGE(TAG, "wifi_sta_init: esp_wifi_set_config failed: %d", start_result);
        return start_result;
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS); // WORKAROUND

    return ESP_OK;
}

esp_err_t wifi_sta_handle_event(void *ctx, system_event_t *event, int *handled)
{
    esp_err_t result = ESP_OK;
    *handled = 1;
    
    switch(event->event_id) {
            
        case SYSTEM_EVENT_STA_START:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_START");
            result = esp_wifi_connect();
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_GOT_IP");
            wifi_sta_set_connected(true);
            break;
            
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_CONNECTED");
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGD(TAG, "wifi_sta_handle_event: SYSTEM_EVENT_STA_DISCONNECTED");
            wifi_sta_set_connected(false);
            // try to re-connect
            result = esp_wifi_connect();
            break;
            
        default:
            ESP_LOGD(TAG, "wifi_sta_handle_event: event is not for us: %d", event->event_id);
            *handled = 0;
            break;
    }

    return result;
}

EventGroupHandle_t wifi_sta_get_event_group()
{
    return wifi_sta_event_group;
}

int wifi_sta_is_connected()
{
    return (xEventGroupGetBits(wifi_sta_event_group) & WIFI_STA_EVENT_GROUP_CONNECTED_FLAG) ? 1 : 0;
}


static void wifi_sta_set_connected(bool c)
{
    if (wifi_sta_is_connected() == c) {
        return;
    }
    
    if (c) {
        xEventGroupSetBits(wifi_sta_event_group, WIFI_STA_EVENT_GROUP_CONNECTED_FLAG);
    } else {
        xEventGroupClearBits(wifi_sta_event_group, WIFI_STA_EVENT_GROUP_CONNECTED_FLAG);
    }
    
    ESP_LOGI(TAG, "Device is now %s WIFI network", c ? "connected to" : "disconnected from");
}
