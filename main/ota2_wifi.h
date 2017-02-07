#ifndef __OTA2_WIFI__
#define __OTA2_WIFI__ 1

#include "ota2.h"
//#include "wifi2.h"


typedef struct ota2_wifi_fetch_metadata_struct_ {

    // Name of the host that provides the firmware images, e.g. www.classycode.io.
    const char *server_host_name;
    
    // Path to the metadata file which contains information on the firmware image,
    // e.g. /ota/meta.txt. The OTA2 module performs a HTTP/1.1 GET request on this file.
    const char *server_metadata_path;
    
    // Pointer to a buffer that will receive the path to the firmware image.
    // If the path doesn't exist in the metadata file, this is set to the empty string.
    const char *server_firmware_path;
    
    // Buffer size of server_firmware_path.
    uint32_t server_firmware_path_len;
    
    // Will receive the firmware version from the metadata file.
    uint32_t server_firmware_version;
    
    // Will be set to true if the metadata file defines a firmware version, false otherwise.
    bool server_firmware_version_valid;
    
    // Will receive the configured polling interval in seconds.
    // 0 means no automatic polling. Only valid if server_polling_interval_s_valid is set.
    uint32_t server_polling_interval_s;
    
    // Will be set to true if the metadata file defines a polling interval, false otherwise.
    bool server_polling_interval_s_valid;
    
} ota2_wifi_fetch_metadata_struct_t;


typedef struct ota2_wifi_fetch_firmware_init_struct_ {
    
    // Name of the host that provides the firmware images, e.g. www.classycode.io.
    // TODO use https context
    const char *server_host_name;
    
    // Path to the firmware image file which contains the binary firmware data,
    // e.g. /ota/fw.bin.
    const char *server_firmware_path;
    
    // Pointer to a buffer that will be filled with firmware data.
    uint8_t *data_buffer;
    
    // Length of the data_buffer.
    uint32_t data_buffer_size;

} ota2_wifi_fetch_firmware_struct_t;


// Initialises the TCP/IP stack and connects to the specified WIFI network.
// TODO use https context
// TODO may be this is no longer needed
ota2_err_t ota2_wifi_init(const char *network_ssid, const char *network_password);

// Fetches the metadata information from the web server.
// This method will block the calling task until the data has been received.
ota2_err_t ota2_wifi_fetch_metadata(ota2_wifi_fetch_metadata_struct_t *params);

// Starts fetching the firmware image on the server.
// Establishes the HTTPS connection and sets up the internal structure.
ota2_err_t ota2_wifi_fetch_firmware_start(ota2_wifi_fetch_firmware_struct_t *param);

// Continues fetching the firmware image.
// Expects the HTTPS connection to be open.
ota2_err_t ota2_wifi_fetch_firmware_continue(ota2_wifi_fetch_firmware_struct_t *param);


#endif // __OTA2_WIFI__
