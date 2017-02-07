//
//  iap_https.c
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

#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"
#include "esp_partition.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "main.h"
#include "wifi_sta.h"
#include "wifi_tls.h"
#include "https_client.h"
#include "iap.h"
#include "iap_https.h"


#define TAG "fwup_wifi"


// The TLS context to communicate with the firmware update server.
static struct wifi_tls_context_ *tls_context;

// Module configuration.
static iap_https_config_t *fwupdater_config;

// The metadata request.
static http_request_t http_metadata_request;

// The firmware image request.
static http_request_t http_firmware_data_request;

// The event group for our processing task.
#define FWUP_CHECK_FOR_UPDATE (1 << 0)
#define FWUP_DOWNLOAD_IMAGE   (1 << 1)
static EventGroupHandle_t event_group;

// The timer for the periodic checking.
static TimerHandle_t check_for_updates_timer;

static int has_iap_session;
static int has_new_firmware;
static int total_nof_bytes_received;

static void iap_https_periodic_check_timer_callback(TimerHandle_t xTimer);
static void iap_https_task(void *pvParameter);
static void iap_https_prepare_timer();
static void iap_https_trigger_processing();
static void iap_https_check_for_update();
static void iap_https_download_image();

http_continue_receiving_t iap_https_metadata_headers_callback(struct http_request_ *request, int statusCode, int contentLength);
http_continue_receiving_t iap_https_metadata_body_callback(struct http_request_ *request, size_t bytesReceived);
http_continue_receiving_t iap_https_firmware_headers_callback(struct http_request_ *request, int statusCode, int contentLength);
http_continue_receiving_t iap_https_firmware_body_callback(struct http_request_ *request, size_t bytesReceived);
void iap_https_error_callback(struct http_request_ *request, http_error_t error, int additionalInfo);


int iap_https_init(iap_https_config_t *config)
{
    ESP_LOGD(TAG, "iap_https_init");
    
    iap_init();
    
    fwupdater_config = config;
    
    // Initialise the HTTPS context to the OTA server.
    
    wifi_tls_init_struct_t tlsInitStruct = {
        .server_host_name = config->server_host_name,
        .server_port = config->server_port,
        .server_root_ca_public_key_pem = config->server_root_ca_public_key_pem,
        .peer_public_key_pem = config->peer_public_key_pem
    };
    tls_context = wifi_tls_create_context(&tlsInitStruct);
    
    
    // Initialise two requests, one to get the metadata and one to get the actual firmware image.
    
    http_metadata_request.verb = HTTP_GET;
    http_metadata_request.host = config->server_host_name;
    http_metadata_request.path = config->server_metadata_path;
    http_metadata_request.response_mode = HTTP_WAIT_FOR_COMPLETE_BODY;
    http_metadata_request.response_buffer_len = 512;
    http_metadata_request.response_buffer = malloc(http_metadata_request.response_buffer_len * sizeof(char));
    http_metadata_request.error_callback = iap_https_error_callback;
    http_metadata_request.headers_callback = iap_https_metadata_headers_callback;
    http_metadata_request.body_callback = iap_https_metadata_body_callback;

    http_firmware_data_request.verb = HTTP_GET;
    http_firmware_data_request.host = config->server_host_name;
    http_firmware_data_request.path = config->server_firmware_path;
    http_firmware_data_request.response_mode = HTTP_STREAM_BODY;
    http_firmware_data_request.response_buffer_len = 4096;
    http_firmware_data_request.response_buffer = malloc(http_firmware_data_request.response_buffer_len * sizeof(char));
    http_firmware_data_request.error_callback = iap_https_error_callback;
    http_firmware_data_request.headers_callback = iap_https_firmware_headers_callback;
    http_firmware_data_request.body_callback = iap_https_firmware_body_callback;
    
    // Start our processing task.
    
    event_group = xEventGroupCreate();

    iap_https_prepare_timer();
    
    xTaskCreate(&iap_https_task, "fwup_wifi_task", 4096, NULL, 1, NULL);

    return 0;
}

int iap_https_check_now()
{
    ESP_LOGD(TAG, "iap_https_check_now");
    iap_https_trigger_processing();
    return 0;
}

int iap_https_update_in_progress()
{
    return has_iap_session;
}

int iap_https_new_firmware_installed()
{
    return has_new_firmware;
}

static void iap_https_periodic_check_timer_callback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(event_group, FWUP_CHECK_FOR_UPDATE);
}

static void iap_https_trigger_processing()
{
    ESP_LOGD(TAG, "iap_https_trigger_processing: checking flag");
    
    if (xEventGroupGetBits(event_group) & FWUP_CHECK_FOR_UPDATE) {
        ESP_LOGD(TAG, "iap_https_trigger_processing: flag is already set");
        return;
    }

    ESP_LOGD(TAG, "iap_https_trigger_processing: flag is not set, setting it");
    
    // Trigger processing in our task.
    xEventGroupSetBits(event_group, FWUP_CHECK_FOR_UPDATE);
}

static void iap_https_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Firmware updater task started.");

    // When the time has come, trigger the firmware update process.

    xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    while (1) {
        // Wait until we get waked up (periodically or because somebody manually
        // requests a firmware update check) and until we're connected to the WIFI
        // network.

        BaseType_t clearOnExit = pdFALSE;
        BaseType_t waitForAllBits = pdFALSE;
        EventBits_t bits = xEventGroupWaitBits(event_group, FWUP_CHECK_FOR_UPDATE | FWUP_DOWNLOAD_IMAGE, clearOnExit, waitForAllBits, portMAX_DELAY);
        
        xEventGroupWaitBits(wifi_sta_get_event_group(), WIFI_STA_EVENT_GROUP_CONNECTED_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        // We give FWUP_DOWNLOAD_IMAGE priority because if it's set, the check for update has
        // previously been executed and the result was that we should update the firmware.

        if (bits & FWUP_DOWNLOAD_IMAGE) {
            ESP_LOGI(TAG, "Firmware updater task will now download the new firmware image.");
            iap_https_download_image();
            xEventGroupClearBits(event_group, FWUP_DOWNLOAD_IMAGE);
            
        } else if (bits & FWUP_CHECK_FOR_UPDATE) {
            ESP_LOGI(TAG, "Firmware updater task checking for firmware update.");
            iap_https_check_for_update();

            // If periodic OTA update checks are enabled, re-start the timer.
            // Clear the bit *after* resetting the timer to avoid the race condition
            // where the timer could have elapsed during the update check and we would
            // immediately check again.
            
            iap_https_prepare_timer();
            xEventGroupClearBits(event_group, FWUP_CHECK_FOR_UPDATE);
        }
    }
}

static void iap_https_prepare_timer()
{
    // Make sure we have a timer if we need one and don't have one if we don't need one.
    
    if (fwupdater_config->polling_interval_s > 0) {
        if (!check_for_updates_timer) {
            // We need a timer but don't have one. Create it.
            BaseType_t autoReload = pdFALSE;
            check_for_updates_timer = xTimerCreate("fwup_periodic_check", 1000, autoReload, NULL, iap_https_periodic_check_timer_callback);
            if (!check_for_updates_timer) {
                ESP_LOGE(TAG, "iap_https_prepare_timer: failed to create the fwup_periodic_check timer!");
                return;
            }
        }
        
        // We need and have a timer, so make sure it uses the correct interval, then start it.

        uint32_t timerMillisec = 1000 * fwupdater_config->polling_interval_s;
        ESP_LOGD(TAG, "iap_https_prepare_timer: timer interval = %d ms", timerMillisec);
        TickType_t timerPeriod = pdMS_TO_TICKS(timerMillisec);

        xTimerChangePeriod(check_for_updates_timer, timerPeriod, pdMS_TO_TICKS(5000));
        
        if (pdFAIL == xTimerReset(check_for_updates_timer, pdMS_TO_TICKS(5000))) {
            ESP_LOGE(TAG, "iap_https_prepare_timer: failed to start the fwup_periodic_check timer!");
        }
        
        return;
    }
    
    // We don't need a timer.
    
    if (check_for_updates_timer) {
        // We have a timer but don't need it. Delete it.
        xTimerDelete(check_for_updates_timer, pdMS_TO_TICKS(5000));
    }
}

static void iap_https_check_for_update()
{
    ESP_LOGD(TAG, "iap_https_check_for_update");
    
    int tlsResult = wifi_tls_connect(tls_context);
    if (tlsResult) {
        ESP_LOGE(TAG, "iap_https_check_for_update: failed to initiate SSL/TLS connection; wifi_tls_connect returned %d", tlsResult);
        return;
    }

    ESP_LOGI(TAG, "Requesting firmware metadata from server.");
    http_error_t httpResult = https_send_request(tls_context, &http_metadata_request);
    if (httpResult != HTTP_SUCCESS) {
        ESP_LOGE(TAG, "iap_https_check_for_update: failed to send HTTPS metadata request; https_send_request returned %d", httpResult);
    }
}

static void iap_https_download_image()
{
    int tlsResult = wifi_tls_connect(tls_context);
    if (tlsResult) {
        ESP_LOGE(TAG, "iap_https_download_image: failed to initiate SSL/TLS connection; wifi_tls_connect returned %d", tlsResult);
    }
    
    // Make sure we open a new IAP session in the callback.
    has_iap_session = 0;
    
    ESP_LOGI(TAG, "Requesting firmware image '%s' from web server.", fwupdater_config->server_firmware_path);
    http_error_t httpResult = https_send_request(tls_context, &http_firmware_data_request);
    if (httpResult != HTTP_SUCCESS) {
        ESP_LOGE(TAG, "iap_https_download_image: failed to send HTTPS firmware image request; https_send_request returned %d", httpResult);
    }
}

http_continue_receiving_t iap_https_metadata_body_callback(struct http_request_ *request, size_t bytesReceived)
{
    ESP_LOGD(TAG, "iap_https_metadata_body_callback");
    
    // --- Process the metadata information ---
    
    // (Optional) interval to check for firmware updates.
    int intervalSeconds = 0;
    if (!http_parse_key_value_int(request->response_buffer, "INTERVAL=", &intervalSeconds)) {
        ESP_LOGD(TAG, "[INTERVAL=] '%d'", intervalSeconds);
        if (intervalSeconds != fwupdater_config->polling_interval_s) {
            ESP_LOGD(TAG, "iap_https_metadata_body_callback: polling interval changed from %d s to %d s",
                     fwupdater_config->polling_interval_s, intervalSeconds);
            fwupdater_config->polling_interval_s = intervalSeconds;
        }
    }
    
    int version = 0;
    if (!http_parse_key_value_int(request->response_buffer, "VERSION=", &version)) {
        ESP_LOGD(TAG, "[VERSION=] '%d'", version);
    } else {
        ESP_LOGW(TAG, "iap_https_metadata_body_callback: firmware version not provided, skipping firmware update");
        return HTTP_STOP_RECEIVING;
    }
    
    char fileName[256];
    if (!http_parse_key_value_string(request->response_buffer, "FILE=", fileName, sizeof(fileName) / sizeof(char))) {
        ESP_LOGD(TAG, "[FILE=] '%s'", fileName);
        strncpy(fwupdater_config->server_firmware_path, fileName, sizeof(fwupdater_config->server_firmware_path) / sizeof(char));
    } else {
        ESP_LOGW(TAG, "iap_https_metadata_body_callback: firmware file name not provided, skipping firmware update");
        return HTTP_STOP_RECEIVING;
    }


    // --- Check if the version on the server is the same as the currently installed version ---
    
    if (version == SOFTWARE_VERSION) {
        ESP_LOGD(TAG, "iap_https_metadata_body_callback: we're up-to-date!");
        return HTTP_STOP_RECEIVING;
    }
    
    ESP_LOGD(TAG, "iap_https_metadata_body_callback: our version is %d, the version on the server is %d", SOFTWARE_VERSION, version);

    // --- Request the firmware image ---

    xEventGroupSetBits(event_group, FWUP_DOWNLOAD_IMAGE);
    
    return HTTP_STOP_RECEIVING;
}

http_continue_receiving_t iap_https_firmware_body_callback(struct http_request_ *request, size_t bytesReceived)
{
    ESP_LOGD(TAG, "iap_https_firmware_body_callback");
    
    // The first time we receive the callback, we neet to start the IAP session.
    if (!has_iap_session) {
        ESP_LOGD(TAG, "iap_https_firmware_body_callback: starting IPA session.");
        iap_err_t result = iap_begin();
        if (result == IAP_ERR_SESSION_ALREADY_OPEN) {
            iap_abort();
            result = iap_begin();
        }
        if (result != IAP_OK) {
            ESP_LOGE(TAG, "iap_https_firmware_body_callback: iap_begin failed (%d)!", result);
            return HTTP_STOP_RECEIVING;
        }
        total_nof_bytes_received = 0;
        has_iap_session = 1;
    }
    
    if (bytesReceived > 0) {
        // Write the received data to the flash.
        iap_err_t result = iap_write((uint8_t*)request->response_buffer, bytesReceived);
        total_nof_bytes_received += bytesReceived;
        if (result != IAP_OK) {
            ESP_LOGE(TAG, "iap_https_firmware_body_callback: write failed (%d), aborting firmware update!", result);
            iap_abort();
            return HTTP_STOP_RECEIVING;
        }
        return HTTP_CONTINUE_RECEIVING;
    }
    
    // After all data has been received, we get one last callback (with bytesReceived == 0).
    // If this happens, we need to finish the IAP session and, if configured, reboot the device.
    
    ESP_LOGD(TAG, "iap_https_firmware_body_callback: all data received (%d bytes), closing session", total_nof_bytes_received);
    has_iap_session = 0;
    
    if (total_nof_bytes_received > 0) {
        iap_err_t result = iap_commit();
        if (result != IAP_OK) {
            ESP_LOGE(TAG, "iap_https_firmware_body_callback: closing the session has failed (%d)!", result);
        }
        
        has_new_firmware = 1;
        
        if (fwupdater_config->auto_reboot) {
            ESP_LOGI(TAG, "Automatic re-boot in 2 seconds - goodbye!...");
            vTaskDelay(2000 / portTICK_RATE_MS);
            esp_restart();
        }
        
    } else {
        ESP_LOGE(TAG, "iap_https_firmware_body_callback: something's not OK - the new firmware image is empty!");
        iap_abort();
    }

    return HTTP_STOP_RECEIVING;
}

http_continue_receiving_t iap_https_metadata_headers_callback(struct http_request_ *request, int statusCode, int contentLength)
{
    ESP_LOGD(TAG, "iap_https_metadata_headers_callback");
    return HTTP_CONTINUE_RECEIVING;
}

http_continue_receiving_t iap_https_firmware_headers_callback(struct http_request_ *request, int statusCode, int contentLength)
{
    ESP_LOGD(TAG, "iap_https_firmware_headers_callback");
    return HTTP_CONTINUE_RECEIVING;
}

void iap_https_error_callback(struct http_request_ *request, http_error_t error, int additionalInfo)
{
    ESP_LOGE(TAG, "iap_https_error_callback: error=%d additionalInfo=%d", error, additionalInfo);
    
    if (error == HTTP_ERR_NON_200_STATUS_CODE) {
        switch (additionalInfo) {
            case 401:
                ESP_LOGE(TAG, "HTTP status code 401: Unauthorized.");
                break;
            case 403:
                ESP_LOGE(TAG, "HTTP status code 403: The server is refusing to provide the resource.");
                break;
            case 404:
                ESP_LOGE(TAG, "HTTP status code 404: Resource not found on the server.");
                break;
            default:
                ESP_LOGE(TAG, "Non-200 status code received: %d", additionalInfo);
                break;
        }
    }
}


