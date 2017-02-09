//
//  wifi_sta.h
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

#ifndef __WIFI_STA__
#define __WIFI_STA__ 1


#define WIFI_STA_EVENT_GROUP_CONNECTED_FLAG (1 << 0)


typedef struct wifi_sta_init_struct_ {
    
    // Network SSID to connect to.
    const char *network_ssid;
    
    // Network password.
    const char *network_password;
    
} wifi_sta_init_struct_t;


// Configure this device in 'station' mode and connect to the specified network.
esp_err_t wifi_sta_init(wifi_sta_init_struct_t *param);

// Sets "handled" to 1 if the event was handled, 0 if it was not for us.
esp_err_t wifi_sta_handle_event(void *ctx, system_event_t *event, int *handled);

// Returns 1 if the device is currently connected to the specified network, 0 otherwise.
int wifi_sta_is_connected();

// Let other modules wait on connectivity changes.
EventGroupHandle_t wifi_sta_get_event_group();


#endif // __WIFI_STA__
