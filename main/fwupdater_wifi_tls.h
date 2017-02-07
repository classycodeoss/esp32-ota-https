//
//  fwupdater_wifi_tls.h
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

#ifndef __FWUPDATER_WIFI_TLS__
#define __FWUPDATER_WIFI_TLS__ 1


typedef struct fwupdater_wifi_tls_config_ {
  
    // Name of the host that provides the firmware images, e.g. "www.classycode.io".
    const char *server_host_name;
    
    // TCP port for TLS communication, e.g. "443".
    const char *server_port;
  
    // Public key of the server's root CA certificate.
    // Needs to be in PEM format (base64-encoded DER data with begin and end marker).
    const char *server_root_ca_public_key_pem;
    
    // Public key of the server's peer certificate (for certificate pinning).
    // Needs to be in PEM format (base64-encoded DER data with begin and end marker).
    const char *peer_public_key_pem;
    
    // Path to the metadata file which contains information on the firmware image,
    // e.g. /ota/meta.txt. We perform an HTTP/1.1 GET request on this file.
    char server_metadata_path[256];
    
    // Path to the firmware image file.
    char server_firmware_path[256];
  
    // Time between two checks, in seconds.
    // If you want to trigger the check manually, set the value to 0 and call the
    // fwupdater_wifi_tls_check_now function.
    // During development, this is typically a small value, e.g. 10 seconds.
    // In production, especially with many devices, higher values make more sense
    // to keep the network traffic low (e.g. 3600 for 1 hour).
    uint32_t polling_interval_s;
    
    // Automatic re-boot after upgrade.
    // If the application can't handle arbitrary re-boots, set this to 'false'
    // and manually trigger the reboot.
    int auto_reboot;

} fwupdater_wifi_tls_config_t;


int fwupdater_wifi_tls_init(fwupdater_wifi_tls_config_t *config);

// Manually trigger a firmware update check.
// Queries the server for a firmware update and, if one is available, installs it.
// If automatic checks are enabled, calling this function causes the timer to be re-set.
int fwupdater_wifi_tls_check_now();

int fwupdater_wifi_tls_update_in_progress();

int fwupdater_wifi_tls_new_firmware_installed();

#endif // __FWUPDATER_WIFI_TLS__
