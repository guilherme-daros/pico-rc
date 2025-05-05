// Example file - Public Domain
// Need help? http://bit.ly/bluepad32-help

#include <btstack_run_loop.h>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <uni.h>
#include "hardware/pwm.h"

#include "sdkconfig.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Defined in my_platform.c
struct uni_platform* get_my_platform(void);

int main() {
    stdio_init_all();

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        loge("failed to initialise cyw43_arch\n");
        return -1;
    }
    const uint IN1 = 18;
    const uint IN2 = 19;
    const uint IN3 = 20;
    const uint IN4 = 21;

    const uint velA = 16;
    const uint velB = 17;

    // build in
    gpio_init(IN1);
    gpio_init(IN2);
    gpio_init(IN3);
    gpio_init(IN4);

    gpio_set_dir(IN1, GPIO_OUT);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_set_dir(IN3, GPIO_OUT);
    gpio_set_dir(IN4, GPIO_OUT);

    gpio_set_function(velA, GPIO_FUNC_PWM);
    gpio_set_function(velB, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(velA);

    pwm_set_wrap(slice_num, 255);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, 0);
    pwm_set_enabled(slice_num, true);

    uni_platform_set_custom(get_my_platform());

    // Initialize BP32
    uni_init(0, NULL);

    // Does not return.
    btstack_run_loop_execute();

    return 0;
}
