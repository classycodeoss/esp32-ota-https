//
//  https_client.h
//  esp32-ota-https
//
//  Updating the firmware over the air.
//
//  This module provides functions to execute HTTPS requests on an
//  existing TLS TCP connection.
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

#ifndef __HTTP_UTIL__
#define __HTTP_UTIL__ 1


// This module depends on the wifi_tls module.
// Forward declaration of the wifi_tls context structure.
struct wifi_tls_context_;


typedef int32_t http_err_t;

#define HTTP_SUCCESS 0
#define HTTP_ERR_INVALID_ARGS           0x101
#define HTTP_ERR_OUT_OF_MEMORY          0x102
#define HTTP_ERR_NOT_IMPLEMENTED        0x103
#define HTTP_ERR_BUFFER_TOO_SMALL       0x104
#define HTTP_ERR_SEND_FAILED            0x105
#define HTTP_ERR_INVALID_STATUS_LINE    0x106
#define HTTP_ERR_VERSION_NOT_SUPPORTED  0x107
#define HTTP_ERR_NON_200_STATUS_CODE    0x108 // additional info = status code

// HTTP methods to use in the requests.
// TODO Right now, this is only a partial implementation.
typedef enum {
    HTTP_GET = 0,
    // HTTP_POST, ...
} http_request_verb_t;

// Callback behaviour of a single request.
// If you can provide a response buffer that you know is big enough,
// you can let this module collect all data in the buffer before it
// invokes your callback. Otherwise, for large downloads which don't
// fit in the buffer, use HTTP_STREAM_BODY which causes the callback
// to be invoked multiple times.
typedef enum {
    HTTP_WAIT_FOR_COMPLETE_BODY,
    HTTP_STREAM_BODY,
} http_response_mode_t;

// Callback return values.
// Specify HTTP_CONTINUE_RECEIVING if you're interested to receive
// more data. The size of the content provided by the web server
// in the Content-Length header overrides this value, i.e. if there's
// no more content to be received, you can use HTTP_CONTINUE_RECEIVING
// but won't get any more callbacks for the corresponding request.
typedef enum {
    HTTP_CONTINUE_RECEIVING = 0,
    HTTP_STOP_RECEIVING
} http_continue_receiving_t;


struct http_request_;

typedef http_continue_receiving_t (*http_request_headers_callback_t)(struct http_request_ *request, int statusCode, int contentLength);
typedef http_continue_receiving_t (*http_request_body_callback_t)(struct http_request_ *request, size_t bytesReceived);
typedef void (*http_request_error_callback_t)(struct http_request_ *request, http_err_t error, int additionalInfo);

typedef struct http_request_ {
    
    // GET, POST, ...
    http_request_verb_t verb;
    
    // www.classycode.io
    const char *host;
    
    // /esp32/ota.txt
    const char *path;
    
    // Buffer to store the response.
    char *response_buffer;
    
    // Size of the response buffer.
    // Needs to be large enough to hold all HTTP headers!
    size_t response_buffer_len;
    
    // Invoked if something goes wrong.
    http_request_error_callback_t error_callback;
    
    // (Optional) callback handler invoked after all headers have been received.
    // Lets the application handle re-direction, authentication requests etc.
    http_request_headers_callback_t headers_callback;
    
    // Define if the body callback should be invoked once after the entire message body
    // has been received (response_buffer needs to be large enough to hold the entire body),
    // or if it should be invoked periodically after parts of the message body have been
    // stored in response_buffer.
    http_response_mode_t response_mode;
    
    // Callback handler to process the message body.
    // Invoked once after receiving the whole message body (HTTP_WAIT_FOR_COMPLETE_BODY)
    // or periodically after receiving more body data (HTTP_STREAM_BODY). In the latter case,
    // a callback with length 0 indicates the end of the body.
    http_request_body_callback_t body_callback;
    
} http_request_t;


// Send the specified HTTP request on the (connected and verified) tlsContext.
// The httpRequest object needs to be kept in memory until the request has been completed.
http_err_t https_send_request(struct wifi_tls_context_ *tlsContext, http_request_t *httpRequest);


// Search the buffer for the specified key and try to parse an integer value right after the key.
// Returns 0 on success.
int http_parse_key_value_int(const char *buffer, const char *key, int *value);

// Search the buffer for the specified key. If it exists, copy the string after the key up to
// but without newline into the str buffer which has a size of strLen.
// Returns 0 on success.
int http_parse_key_value_string(const char *buffer, const char *key, char *str, int strLen);

#endif // __HTTP_UTIL__
