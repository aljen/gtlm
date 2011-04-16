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

#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <sys/types.h>
#include "libgtlm.h"


static const char *gCfgName = "/.gtlm";
static bool gDebug = false;


libgtlm_device*
libgtlm_init(bool forceReset)
{
    libgtlm_device *gtlm = NULL;
    ssize_t lmCount = sizeof(libgtlm_device_ids) / sizeof(libgtlm_device_ids[0]);
    int owned = 0;

    int result = libusb_init(NULL);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return gtlm;
    }

    gtlm = (libgtlm_device*)malloc(sizeof(*gtlm));
    if (gtlm == NULL) goto error;
    memset(gtlm, 0, sizeof(*gtlm));

    for (int i = 0; i < lmCount; i++) {
        gtlm->handle = libusb_open_device_with_vid_pid(NULL,
                libgtlm_device_ids[i].vendor, libgtlm_device_ids[i].product);
        if (gtlm->handle)
            break;
    }

    if (gtlm->handle == NULL) {
        if (gDebug)
            fprintf(stderr, "Led controller not found!\n");
        goto error;
    }

    if (forceReset) {
        result = libusb_reset_device(gtlm->handle);
        if (result < 0) {
            print_libusb_error(result, __LINE__, __FILE__);
            goto error;
        }
    }

    owned = libusb_kernel_driver_active(gtlm->handle, 0);
    if (owned == 1) {
        result = libusb_detach_kernel_driver(gtlm->handle, 0);
        if (result < 0) {
            print_libusb_error(result, __LINE__, __FILE__);
            goto error;
        }
    }

    if (gDebug) {
        char *string = libgtlm_get_device_name(gtlm);
        fprintf(stderr, "Found LED controller: %s\n", string);
        free(string);
    }

    result = libusb_claim_interface(gtlm->handle, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        goto error;
    }

    libgtlm_get_led_mode(gtlm);
    config_init(&gtlm->config);
    return gtlm;

error:
    if (gtlm && gtlm->handle) {
        libusb_release_interface(gtlm->handle, 0x00);
        libusb_close(gtlm->handle);
    }
    free(gtlm);
    libusb_exit(NULL);
    return NULL;
}


void
libgtlm_free(libgtlm_device *device)
{
    libusb_release_interface(device->handle, 0x00);
    libusb_close(device->handle);
    config_destroy(&device->config);
    free(device);
    libusb_exit(NULL);
}


bool
libgtlm_check_version(libgtlm_device *device)
{
    char version[6];
    memset(&version, 0x00, 6);
    libgtlm_get_version(device, (char*)&version);
    if (strcmp((const char*)&version, GTLM_VERSION_STRING) == 0)
        return true;

    return false;
}


void
libgtlm_get_version(libgtlm_device *device, char *version)
{
    if (device == NULL || version == NULL)
        return;

    int result = 0;
    unsigned char data[8];
    memset(&data, 0x00, 8);
    data[0] = 0x01;
    data[1] = 0x10;
    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_OUT, LIBUSB_REQUEST_SET_CONFIGURATION,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_IN, LIBUSB_REQUEST_CLEAR_FEATURE,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    version[0] = data[2];
    version[1] = data[3];
    version[2] = data[4];
    version[3] = data[5];
    version[4] = data[6];
    version[5] = '\0';
}


void
libgtlm_enable_led(libgtlm_device *device, libgtlm_led_status status)
{
    if (device == NULL)
        return;

    device->led_status |= status;
}


void
libgtlm_disable_led(libgtlm_device *device, libgtlm_led_status status)
{
    if (device == NULL)
        return;

    device->led_status &= ~status;
}


void
libgtlm_enable_all_leds(libgtlm_device *device)
{
    if (device == NULL)
        return;

    device->led_status = LEDS_ALL;
}


void
libgtlm_disable_all_leds(libgtlm_device *device)
{
    if (device == NULL)
        return;

    device->led_status = LEDS_NONE;
}


bool
libgtlm_is_led_enabled(libgtlm_device *device, libgtlm_led_status status)
{
    if (device == NULL)
        return false;

    if ((device->led_status & status) != 0)
        return true;

    return false;
}


void
libgtlm_set_led_mode(libgtlm_device *device, libgtlm_led_mode mode, bool enable)
{
    if (device == NULL)
        return;

    device->enabled = enable;
    device->led_mode = mode;
}


void
libgtlm_get_led_mode(libgtlm_device *device)
{
    if (device == NULL)
        return;

    int result = 0;
    unsigned char data[8];
    memset(&data, 0x00, 8);
    data[0] = 0x01;
    data[1] = 0x01;
    data[2] = 0x10;
    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_OUT, LIBUSB_REQUEST_SET_CONFIGURATION,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_IN, LIBUSB_REQUEST_CLEAR_FEATURE,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    device->led_mode = data[3];
    device->enabled = data[4] ? true : false;
}


void
libgtlm_sync(libgtlm_device *device)
{
    if (device == NULL)
        return;

    int result = 0;
    unsigned char data[8];

    // led state part
    memset(&data, 0x00, 8);
    data[0] = 0x01;
    data[1] = 0x02;
    data[2] = 0x30;
    data[3] = device->led_status;
    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_OUT, LIBUSB_REQUEST_SET_CONFIGURATION,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_IN, LIBUSB_REQUEST_CLEAR_FEATURE,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    device->led_status = data[3];

    // led mode part
    memset(&data, 0x00, 8);
    data[0] = 0x01;
    data[1] = 0x02;
    data[2] = 0x20;
    data[3] = device->led_mode;
    data[4] = device->enabled;
    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_OUT, LIBUSB_REQUEST_SET_CONFIGURATION,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    result = libusb_control_transfer(device->handle,
        GTLM_CONFIG_REQUEST_TYPE_IN, LIBUSB_REQUEST_CLEAR_FEATURE,
        GTLM_CONFIG_VALUE, GTLM_CONFIG_INDEX, (unsigned char*)&data, 8, 0x00);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        return;
    }

    device->led_mode = data[3];
    device->enabled = data[4] ? true : false;
}


void
libgtlm_set_debug(bool debug)
{
    gDebug = debug;
}


bool
libgtlm_read_config(libgtlm_device *device)
{
    if (device == NULL)
        return false;

    char *home = getenv("HOME");
    char *cfg = NULL;
    int result = 0;
    if (home) {
        int len = strlen(home) + strlen(gCfgName) + 1;
        cfg = (char*)malloc(len);
        if (cfg == NULL)
            return false;
        snprintf(cfg, len, "%s%s", home, gCfgName);
        result = config_read_file(&device->config, cfg);
        if (result < 0) {
            free(cfg);
            return false;
        }
        free(cfg);
    }

    bool back = true;
    bool side = true;
    bool front = true;
    int mode = MODE_ALWAYS;
    bool enabled = true;
    config_setting_t *root = NULL, *settings = NULL, *setting = NULL;
    root = config_root_setting(&device->config);
    settings = config_setting_get_member(root, "settings");
    if (settings == NULL)
        settings = config_setting_add(root, "settings", CONFIG_TYPE_GROUP);
    setting = config_setting_get_member(settings, "back");
    if (setting)
        back = config_setting_get_bool(setting);
    setting = config_setting_get_member(settings, "side");
    if (setting)
        side = config_setting_get_bool(setting);
    setting = config_setting_get_member(settings, "front");
    if (setting)
        front = config_setting_get_bool(setting);
    setting = config_setting_get_member(settings, "mode");
    if (setting)
        mode = config_setting_get_int(setting);
    setting = config_setting_get_member(settings, "enabled");
    if (setting)
        enabled = config_setting_get_bool(setting);
    if (back)
        libgtlm_enable_led(device, LEDS_BACK);
    else
        libgtlm_disable_led(device, LEDS_BACK);
    if (side)
        libgtlm_enable_led(device, LEDS_SIDE);
    else
        libgtlm_disable_led(device, LEDS_SIDE);
    if (front)
        libgtlm_enable_led(device, LEDS_FRONT);
    else
        libgtlm_disable_led(device, LEDS_FRONT);
    libgtlm_set_led_mode(device, (libgtlm_led_mode)mode, enabled);
    return true;
}


bool
libgtlm_write_config(libgtlm_device *device)
{
    if (device == NULL)
        return false;

    bool back = libgtlm_is_led_enabled(device, LEDS_BACK);
    bool side = libgtlm_is_led_enabled(device, LEDS_SIDE);
    bool front = libgtlm_is_led_enabled(device, LEDS_FRONT);
    int mode = device->led_mode;
    bool enabled = device->enabled;
    config_setting_t *root = NULL, *settings = NULL, *setting = NULL;
    root = config_root_setting(&device->config);
    settings = config_setting_get_member(root, "settings");
    if (settings == NULL)
        settings = config_setting_add(root, "settings", CONFIG_TYPE_GROUP);
    setting = config_setting_get_member(settings, "back");
    if (setting == NULL)
        setting = config_setting_add(settings, "back", CONFIG_TYPE_BOOL);
    config_setting_set_bool(setting, back);
    setting = config_setting_get_member(settings, "side");
    if (setting == NULL)
        setting = config_setting_add(settings, "side", CONFIG_TYPE_BOOL);
    config_setting_set_bool(setting, side);
    setting = config_setting_get_member(settings, "front");
    if (setting == NULL)
        setting = config_setting_add(settings, "front", CONFIG_TYPE_BOOL);
    config_setting_set_bool(setting, front);
    setting = config_setting_get_member(settings, "mode");
    if (setting == NULL)
        setting = config_setting_add(settings, "mode", CONFIG_TYPE_INT);
    config_setting_set_int(setting, mode);
    setting = config_setting_get_member(settings, "enabled");
    if (setting == NULL)
        setting = config_setting_add(settings, "enabled", CONFIG_TYPE_BOOL);
    config_setting_set_bool(setting, enabled);

    char *home = getenv("HOME");
    char *cfg = NULL;
    int result = 0;
    if (home) {
        int len = strlen(home) + strlen(gCfgName) + 1;
        cfg = (char*)malloc(len);
        if (cfg == NULL)
            return false;
        snprintf(cfg, len, "%s%s", home, gCfgName);
        result = config_write_file(&device->config, cfg);
        if (result < 0) {
            free(cfg);
            return false;
        }
        free(cfg);
    }

    return true;
}


char*
libgtlm_get_device_name(libgtlm_device *device)
{
    if (device == NULL)
        return NULL;

    char *string = (char*)malloc(128);
    libusb_device *dev;
    libusb_device_descriptor descriptor;
    dev = libusb_get_device(device->handle);
    int result = libusb_get_device_descriptor(dev, &descriptor);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        free(string);
        return NULL;
    }
    result = libusb_get_string_descriptor_ascii(device->handle,
        descriptor.iManufacturer, (unsigned char*)string, 128);
    if (result < 0) {
        print_libusb_error(result, __LINE__, __FILE__);
        free(string);
        return NULL;
    }
    
    return string;
}


void print_libusb_error(int error, int line, const char *file)
{
    fprintf(stderr, "ERROR: %s:%d:", file, line);
    switch (error) {
    case LIBUSB_ERROR_IO:
        fprintf(stderr, "IO\n");
        break;
    case LIBUSB_ERROR_INVALID_PARAM:
        fprintf(stderr, "INVALID PARAM\n");
        break;
    case LIBUSB_ERROR_ACCESS:
        fprintf(stderr, "ACCESS\n");
        break;
    case LIBUSB_ERROR_NOT_FOUND:
        fprintf(stderr, "NOT FOUND\n");
        break;
    case LIBUSB_ERROR_BUSY:
        fprintf(stderr, "BUSY\n");
        break;
    case LIBUSB_ERROR_OVERFLOW:
        fprintf(stderr, "OVERFLOW\n");
        break;
    case LIBUSB_ERROR_INTERRUPTED:
        fprintf(stderr, "INTERRUPTED\n");
        break;
    case LIBUSB_ERROR_NO_MEM:
        fprintf(stderr, "NO MEMORY\n");
        break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        fprintf(stderr, "NOT SUPPORTED\n");
        break;
    case LIBUSB_ERROR_TIMEOUT:
        fprintf(stderr, "TIMEOUT\n");
        break;
    case LIBUSB_ERROR_PIPE:
        fprintf(stderr, "PIPE\n");
        break;
    case LIBUSB_ERROR_NO_DEVICE:
        fprintf(stderr, "NO DEVICE\n");
        break;
    case LIBUSB_ERROR_OTHER:
    default:
        fprintf(stderr, "%d\n", error);
        break;
    }
}
