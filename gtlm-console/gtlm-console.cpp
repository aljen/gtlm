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
#include <getopt.h>
#include "libgtlm.h"


void
print_version(const char *name)
{
    printf("%s v%d.%d (c) 2010 Artur Wyszynski <harakash@gmail.com>\n", name,
        GTLM_VERSION_MAJOR, GTLM_VERSION_MINOR);
}


void print_usage(const char *name)
{
    print_version(name);
    printf("Usage:\n");
    printf(" --help             - Display this information\n");
    printf(" --version          - Display program version\n");
    printf(" --status           - Display device status\n");
    printf(" --enable           - Set LEDs status [on/off]\n");
    printf(" --back=state       - Set rear LEDs [on/off]\n");
    printf(" --side=state       - Set side LEDs [on/off]\n");
    printf(" --front=state      - Set front LEDs [on/off]\n");
    printf(" --mode=mode        - Set LEDs mode [blink/audio/breath/demo/always]\n");
    printf(" --force-reset      - Force device reset\n");
}


int
main(int argc, char *argv[])
{
    static const char *kOptions = "hvdre:b:s:f:m:";
    static const struct option kLongOptions[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"status", no_argument, NULL, 'd'},
        {"force-reset", no_argument, NULL, 'r'},
        {"enable", required_argument, NULL, 'e'},
        {"back", required_argument, NULL, 'b'},
        {"side", required_argument, NULL, 's'},
        {"front", required_argument, NULL, 'f'},
        {"mode", required_argument, NULL, 'm'},
        {NULL, no_argument, NULL, 0}
    };

    libgtlm_device *device = NULL;
    bool hasBack = false;
    bool back = false;
    bool hasSide = false;
    bool side = false;
    bool hasFront = false;
    bool front = false;
    bool hasMode = false;
    uint8_t mode = MODE_ALWAYS;
    bool showStatus = false;
    bool printVersion = false;
    bool printUsage = false;
    bool forceReset = false;
    bool hasEnable = false;
    bool enable = false;
    int8_t option = 0;

    while ((option = getopt_long(argc, argv, kOptions, kLongOptions, NULL)) != -1) {
        switch (option) {
            case 'h':
                printUsage = true;
                break;
            case 'v':
                printVersion = true;
                break;
            case 'd':
                showStatus = true;
                break;
            case 'r':
                forceReset = true;
                break;
            case 'e':
                if (optarg) {
                    if (strcmp(optarg, "on") == 0) {
                        hasEnable = true;
                        enable = true;
                    } else if (strcmp(optarg, "off") == 0) {
                        hasEnable = true;
                        enable = false;
                    } else
                        fprintf(stderr, "--enable: wrong argument '%s', use 'on' or 'off'\n", optarg);
                }
                break;
            case 'b':
                if (optarg) {
                    if (strcmp(optarg, "on") == 0) {
                        hasBack = true;
                        back = true;
                    } else if (strcmp(optarg, "off") == 0) {
                        hasBack = true;
                        back = false;
                    } else
                        fprintf(stderr, "--back: wrong argument '%s', use 'on' or 'off'\n", optarg);
                }
                break;
            case 's':
                if (optarg) {
                    if (strcmp(optarg, "on") == 0) {
                        hasSide = true;
                        side = true;
                    } else if (strcmp(optarg, "off") == 0) {
                        hasSide = true;
                        side = false;
                    } else
                        fprintf(stderr, "--side: wrong argument '%s', use 'on' or 'off'\n", optarg);
                }
                break;
            case 'f':
                if (optarg) {
                    if (strcmp(optarg, "on") == 0) {
                        hasFront = true;
                        front = true;
                    } else if (strcmp(optarg, "off") == 0) {
                        hasFront = true;
                        front = false;
                    } else
                        fprintf(stderr, "--front: wrong argument '%s', use 'on' or 'off'\n", optarg);
                }
                break;
            case 'm':
                if (optarg) {
                    if (strcmp(optarg, "blink") == 0) {
                        hasMode = true;
                        mode = MODE_BLINK;
                    } else if (strcmp(optarg, "audio") == 0) {
                        hasMode = true;
                        mode = MODE_AUDIO;
                    } else if (strcmp(optarg, "breath") == 0) {
                        hasMode = true;
                        mode = MODE_BREATH;
                    } else if (strcmp(optarg, "demo") == 0) {
                        hasMode = true;
                        mode = MODE_DEMO;
                    } else if (strcmp(optarg, "always") == 0) {
                        hasMode = true;
                        mode = MODE_ALWAYS;
                    } else
                        fprintf(stderr, "--mode: wrong argument '%s', use 'blink', 'audio', 'breath', 'demo' or 'always'\n", optarg);
                }
                break;
            default:
                return 0;
                break;
        }
    }

    if (printUsage || argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    if (printVersion) {
        print_version(argv[0]);
        return 0;
    }

    device = libgtlm_init(forceReset);
    if (!device) goto error_no_device;

    libgtlm_read_config(device);

    if (showStatus) {
        print_version(argv[0]);
        char *name;
        char version[6];
        memset(&version, 0x00, 6);
        libgtlm_get_version(device, (char*)&version);
        name = libgtlm_get_device_name(device);
        printf("Device     : %s\n", name);
        printf("Version    : %s\n", version);
        printf("Back LEDs  : %s\n", libgtlm_is_led_enabled(device, LEDS_BACK)
            ? "ON" : "OFF");
        printf("Side LEDs  : %s\n", libgtlm_is_led_enabled(device, LEDS_SIDE)
            ? "ON" : "OFF");
        printf("Front LEDs : %s\n", libgtlm_is_led_enabled(device, LEDS_FRONT)
            ? "ON" : "OFF");
        printf("Mode       : ");
        if (device->led_mode == MODE_BLINK)
            printf("BLINK\n");
        else if (device->led_mode == MODE_AUDIO)
            printf("AUDIO\n");
        else if (device->led_mode == MODE_BREATH)
            printf("BREATH\n");
        else if (device->led_mode == MODE_DEMO)
            printf("DEMO\n");
        else if (device->led_mode == MODE_ALWAYS)
            printf("ALWAYS\n");
        free(name);
        libgtlm_write_config(device);
        libgtlm_free(device);
        return 0;
    }

    if (hasBack) {
        if (back)
            libgtlm_enable_led(device, LEDS_BACK);
        else
            libgtlm_disable_led(device, LEDS_BACK);
    }
    if (hasSide) {
        if (side)
            libgtlm_enable_led(device, LEDS_SIDE);
        else
            libgtlm_disable_led(device, LEDS_SIDE);
    }
    if (hasFront) {
        if (front)
            libgtlm_enable_led(device, LEDS_FRONT);
        else
            libgtlm_disable_led(device, LEDS_FRONT);
    }
    if (device)
        libgtlm_set_led_mode(device, (libgtlm_led_mode)(hasMode ? mode :
            device->led_mode), hasEnable ? enable : device->enabled);

    libgtlm_sync(device);

    libgtlm_write_config(device);

    libgtlm_free(device);
    goto normal_exit;

error_no_device:
    printf("Led controller not found!\n");

normal_exit:
    return 0;
}
