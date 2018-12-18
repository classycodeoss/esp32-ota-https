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
