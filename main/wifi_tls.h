//
//  wifi_tls.h
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  This module provides TLS connections with certificate pinning and
//  callback-based request/response functionality.
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

#ifndef __WIFI_TLS__
#define __WIFI_TLS__ 1


// Forward declaration of the opaque context object.
struct wifi_tls_context_;

typedef struct wifi_tls_init_struct_ {
    
    // Name of the host that provides the firmware images, e.g. "www.classycode.io".
    const char *server_host_name;
    
    // Port for the connection, e.g. "443".
    const char *server_port;
    
    // Public key of the server's root CA certificate.
    // Needs to be in PEM format (base64-encoded DER data with begin and end marker).
    const char *server_root_ca_public_key_pem;

    // Public key of the server's peer certificate for certificate pinning.
    // Needs to be in PEM format (base64-encoded DER data with begin and end marker).
    const char *peer_public_key_pem;
    
} wifi_tls_init_struct_t;

typedef struct wifi_tls_request_ {
    
    // Request buffer.
    // Example: "GET https://www.classycode.io/esp32/ota.txt HTTP/1.1\nHost: www.classycode.io\n\n"
    // Not necessarily zero-terminated.
    char *request_buffer;
    
    // Number of bytes in the request buffer.
    uint32_t request_len;
    
    // Response buffer.
    // This buffer will be filled with the data received from the server.
    // Data may be received in chunks. Every chunk is stored in the buffer and provided
    // to the client via the response_callback function (see below).
    char *response_buffer;
    
    // Size of the response buffer.
    // Defines the maximum number of bytes that will be read into the response buffer.
    uint32_t response_buffer_size;
    
    // Make custom data available to the callback.
    void *custom_data;
    
    // Callback function to handle the response.
    // Return 1 to continue reading, 0 to end reading.
    int (*response_callback)(struct wifi_tls_context_ *context, struct wifi_tls_request_ *request, int index, size_t len);
    
} wifi_tls_request_t;


// Create a context for TLS communication to a server.
// The context can be re-used for multiple connections to the same server on the same port.
// The init structure and all fields can be released after calling this function.
struct wifi_tls_context_ *wifi_tls_create_context(wifi_tls_init_struct_t *params);

// Release the context.
// Performs necessary cleanup and releases all memory associated with the context.
void wifi_tls_free_context(struct wifi_tls_context_ *context);

// Connects to the server, performs the TLS handshake and certificate verification.
// Returns 0 on success.
int wifi_tls_connect(struct wifi_tls_context_ *context);

// Disconnects from the server.
void wifi_tls_disconnect(struct wifi_tls_context_ *context);

// Send a request to the server.
// Calls the response callback function defined in the request structure.
// Returns 0 on success.
int wifi_tls_send_request(struct wifi_tls_context_ *context, wifi_tls_request_t *request);


#endif // __WIFI_TLS__
