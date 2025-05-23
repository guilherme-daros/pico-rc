// Example file - Public Domain
// Need help? https://tinyurl.com/bluepad32-help

#include <math.h>
#include <stddef.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>
#include "controller/uni_controller.h"
#include "controller/uni_gamepad.h"
#include "hardware/pwm.h"

#include "sdkconfig.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);

//
// Platform Overrides
//
static void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("my_platform: init()\n");
}

static void my_platform_on_init_complete(void) {
    logi("my_platform: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Start scanning and autoconnect to supported controllers.
    uni_bt_start_scanning_and_autoconnect_unsafe();

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();

    // Turn off LED once init is done.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    //    uni_bt_service_set_enabled(true);

    uni_property_dump_all();
}

static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    // As an example, if you want to filter out keyboards, do:
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("Ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("my_platform: device connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("my_platform: device disconnected: %p\n", d);
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("my_platform: device ready: %p\n", d);

    // You can reject the connection by returning an error.
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static uni_controller_t prev = {0};
    // uni_controller_dump(ctl);
    const uint IN1 = 21;
    const uint IN2 = 20;
    const uint IN3 = 19;
    const uint IN4 = 18;

    const uint velA = 17;
    const uint velB = 16;

    const uint intensity = 255;

    if (ctl->klass == UNI_CONTROLLER_CLASS_GAMEPAD) {
        uni_gamepad_t* gp = &ctl->gamepad;

        float X = (gp->axis_x + 4) / 512.0;
        loge("X: %f ", X);

        uint slice_num = pwm_gpio_to_slice_num(16);
        float modulo = 0;

        if (gp->throttle > 0) {
            gpio_put(IN1, 1);
            gpio_put(IN2, 0);
            gpio_put(IN3, 1);
            gpio_put(IN4, 0);
            modulo = gp->throttle / 1020.0;
        }
        if (gp->brake > 0) {
            gpio_put(IN1, 0);
            gpio_put(IN2, 1);
            gpio_put(IN3, 0);
            gpio_put(IN4, 1);
            modulo = gp->brake / 1020.0;
        }

        loge("modulo: %f ", modulo);

        uint pwm_valueA = modulo * intensity;
        uint pwm_valueB = modulo * intensity;

        if (X > 0) {
            pwm_valueA *= (1 - fabs(X));
        }

        if (X < 0) {
            pwm_valueB *= (1 - fabs(X));
        }

        loge("pwmA: %d ", pwm_valueA);
        loge("pwmB: %d ", pwm_valueB);
        loge("absX: %f ", fabs(X));

        pwm_set_chan_level(slice_num, PWM_CHAN_A, pwm_valueA);
        pwm_set_chan_level(slice_num, PWM_CHAN_B, pwm_valueB);
        loge("\n");
    } else {
        loge("Unsupported controller class: %d\n", ctl->klass);
    }
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
            // Optional: do something when "system" button gets pressed.
            trigger_event_on_gamepad((uni_hid_device_t*)data);
            break;

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            // When the "bt scanning" is on / off. Could be triggered by different events
            // Useful to notify the user
            logi("my_platform_on_oob_event: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
    }
}

//
// Helpers
//
static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 50 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        static uint8_t led = 0;
        led += 1;
        led &= 0xf;
        d->report_parser.set_player_leds(d, led);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        static uint8_t red = 0x10;
        static uint8_t green = 0x20;
        static uint8_t blue = 0x40;

        red += 0x10;
        green -= 0x20;
        blue += 0x40;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

//
// Entry Point
//
struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "My Platform",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };

    return &plat;
}
