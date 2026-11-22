#include <stdint.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#define BUZZER_GPIO       18
#define LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_TIMER        LEDC_TIMER_0
#define LEDC_CHANNEL      LEDC_CHANNEL_0
#define LEDC_DUTY_RES     LEDC_TIMER_10_BIT
#define LEDC_DUTY_50_PCT  (1U << (LEDC_DUTY_RES - 1)) // 512 (50% шпаруватість)

static const char *TAG = "PWM_BUZZER";

static void buzzer_pwm_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    ledc_channel_config_t channel_cfg = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

static void buzzer_set_freq(uint32_t freq_hz) {
    if (freq_hz == 0) {
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        return;
    }

    ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_hz));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_50_PCT));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void app_main(void) {
    buzzer_pwm_init();
    buzzer_set_freq(0);
    ESP_LOGI(TAG, "LEDC PWM initialized on GPIO %d", BUZZER_GPIO);
}