//
//  iap.h
//  esp32-ota-https
//
//  In-application programming
//
//  This module is responsible for writing the firmware to the flash.
//  It manages the write buffer, writing to the flash, selecting the
//  correct partition and activating the partition.
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

#ifndef __IAP__
#define __IAP__ 1


typedef int32_t iap_err_t;

#define IAP_OK      0
#define IAP_FAIL    -1
#define IAP_ERR_ALREADY_INITIALIZED     0x101
#define IAP_ERR_NOT_INITIALIZED         0x102
#define IAP_ERR_SESSION_ALREADY_OPEN    0x103
#define IAP_ERR_OUT_OF_MEMORY           0x104
#define IAP_ERR_NO_SESSION              0x105
#define IAP_ERR_PARTITION_NOT_FOUND     0x106
#define IAP_ERR_WRITE_FAILED            0x107


// Call once at application startup, before calling any other function of this module.
iap_err_t iap_init();

// Call to start a programming session.
// Sets the programming pointer to the start of the next OTA flash partition.
iap_err_t iap_begin();

// Call to write a block of data to the current location in flash.
// If the write fails, you need to abort the current programming session
// with 'iap_abort' and start again from the beginning.
iap_err_t iap_write(uint8_t *bytes, uint16_t len);

// Call to close a programming session and activate the programmed partition.
iap_err_t iap_commit();

// Abort the current programming session.
iap_err_t iap_abort();


#endif // __IAP__
