#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUZZER_GPIO       18
#define LEDC_MODE         LEDC_LOW_SPEED_MODE
#define LEDC_TIMER        LEDC_TIMER_0
#define LEDC_CHANNEL      LEDC_CHANNEL_0
#define LEDC_DUTY_RES     LEDC_TIMER_10_BIT
#define LEDC_DUTY_50_PCT  (1U << (LEDC_DUTY_RES - 1))

#define PLAYER_TICK_MS    50
#define PLAYER_TICK_US    (PLAYER_TICK_MS * 1000ULL)

// Частоти нот (Гц)
#define NOTE_REST  0
#define NOTE_C5    523
#define NOTE_E5    659
#define NOTE_G5    784
#define NOTE_C6    1047

typedef struct {
    uint32_t freq_hz; // 0 = пауза
    uint16_t ticks;   // Тривалість у тіках по 50 мс
} note_t;

static const note_t MELODY[] = {
    {NOTE_C5, 4}, {NOTE_REST, 1},
    {NOTE_E5, 4}, {NOTE_REST, 1},
    {NOTE_G5, 4}, {NOTE_REST, 1},
    {NOTE_C6, 8}, {NOTE_REST, 2},
    {NOTE_G5, 4}, {NOTE_REST, 1},
    {NOTE_C6, 10}, {NOTE_REST, 20},
};

#define MELODY_LEN (sizeof(MELODY) / sizeof(MELODY[0]))

static const char *TAG = "PWM_BUZZER";

static size_t current_note = 0;
static uint16_t ticks_left = 0;
static bool is_playing = true;

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

static void audio_player_tick_cb(void *arg) {
    (void)arg;
    if (!is_playing) {
        return;
    }

    if (ticks_left == 0) {
        if (current_note >= MELODY_LEN) {
            current_note = 0;
        }
        buzzer_set_freq(MELODY[current_note].freq_hz);
        ticks_left = MELODY[current_note].ticks;
        current_note++;
    }

    if (ticks_left > 0) {
        ticks_left--;
    }
}

void app_main(void) {
    buzzer_pwm_init();

    const esp_timer_create_args_t player_timer_args = {
        .callback = &audio_player_tick_cb,
        .arg      = NULL,
        .name     = "audio_player_tick",
    };

    esp_timer_handle_t player_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&player_timer_args, &player_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(player_timer, PLAYER_TICK_US));

    ESP_LOGI(TAG, "Non-blocking PWM player started on GPIO %d (tick %d ms)",
             BUZZER_GPIO, PLAYER_TICK_MS);
}