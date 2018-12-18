//
//  iap_https.h
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  This module is responsible to trigger and coordinate firmware updates.
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

#ifndef __IAP_HTTPS__
#define __IAP_HTTPS__ 1


typedef struct iap_https_config_ {
  
    // Version number of the running firmware image.
    int current_software_version;
  
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
  
    // Default time between two checks, in seconds.
    // If you want to trigger the check manually, set the value to 0 and call the
    // iap_https_check_now function.
    // During development, this is typically a small value, e.g. 10 seconds.
    // In production, especially with many devices, higher values make more sense
    // to keep the network traffic low (e.g. 3600 for 1 hour).
    uint32_t polling_interval_s;
    
    // Automatic re-boot after upgrade.
    // If the application can't handle arbitrary re-boots, set this to 'false'
    // and manually trigger the reboot.
    int auto_reboot;

} iap_https_config_t;


// Module initialisation, call once at application startup.
int iap_https_init(iap_https_config_t *config);

// Manually trigger a firmware update check.
// Queries the server for a firmware update and, if one is available, installs it.
// If automatic checks are enabled, calling this function causes the timer to be re-set.
int iap_https_check_now();

// Returns 1 if an update is currently in progress, 0 otherwise.
int iap_https_update_in_progress();

// Returns 1 if a new firmware has been installed but not yet booted, 0 otherwise.
int iap_https_new_firmware_installed();


#endif // __IAP_HTTPS__
