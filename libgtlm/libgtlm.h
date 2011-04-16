/*
 * Copyright (C) 2010 by Artur Wyszynski <harakash@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once
#ifndef __LIBGTLM_H__
#define __LIBGTLM_H__

#include "libconfig.h"
#include "libusb.h"

#define DEBUG_LIBGTLM

#define GTLM_CONFIG_REQUEST_TYPE_IN  (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE)
#define GTLM_CONFIG_REQUEST_TYPE_OUT (LIBUSB_ENDPOINT_OUT |LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE)
#define GTLM_CONFIG_VALUE            0x301
#define GTLM_CONFIG_INDEX            0x00
#define GTLM_VERSION_STRING          "UF1.0"
#define GTLM_VERSION_MAJOR           0
#define GTLM_VERSION_MINOR           1

enum libgtlm_led_status {
    LEDS_NONE       = 0x00,
    LEDS_BACK       = 0x01,
    LEDS_SIDE       = 0x02,
    LEDS_FRONT      = 0x04,
    LEDS_ALL        = 0x07
};

enum libgtlm_led_mode {
    MODE_BLINK      = 0x01,
    MODE_AUDIO      = 0x02,
    MODE_BREATH     = 0x03,
    MODE_DEMO       = 0x04,
    MODE_ALWAYS     = 0x05
};

typedef struct libgtlm_device_id {
    uint16_t vendor;
    uint16_t product;
} libgtlm_device_id;

static libgtlm_device_id libgtlm_device_ids[] = {
    { 0x1770, 0xFF00 }, // MSI GT660 LED Controller - MSI EPF USB
};

typedef struct libgtlm_device {
    libusb_device_handle* handle;
    uint8_t led_status;
    uint8_t led_mode;
    bool enabled;
    config_t config;
} libgtlm_device;


libgtlm_device* libgtlm_init(bool forceReset);
void libgtlm_free(libgtlm_device *device);
bool libgtlm_check_version(libgtlm_device *device);
void libgtlm_get_version(libgtlm_device *device, char *version);
void libgtlm_enable_led(libgtlm_device *device, libgtlm_led_status status);
void libgtlm_disable_led(libgtlm_device *device, libgtlm_led_status status);
void libgtlm_enable_all_leds(libgtlm_device *device);
void libgtlm_disable_all_leds(libgtlm_device *device);
bool libgtlm_is_led_enabled(libgtlm_device *device, libgtlm_led_status status);
void libgtlm_set_led_mode(libgtlm_device *device, libgtlm_led_mode mode, bool enable);
void libgtlm_get_led_mode(libgtlm_device *device);
void libgtlm_sync(libgtlm_device *device);
void libgtlm_set_debug(bool debug);
bool libgtlm_read_config(libgtlm_device *device);
bool libgtlm_write_config(libgtlm_device *device);
char* libgtlm_get_device_name(libgtlm_device *device);

void print_libusb_error(int error, int line, const char *file);

#endif // __LIBGTLM_H__
