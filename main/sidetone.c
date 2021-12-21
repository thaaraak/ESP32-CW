/*  Wave Generator Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    DAC output channel, waveform, wave frequency can be customized in menuconfig.
    If any questions about this example or more information is needed, please read README.md before your start.
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/timer.h"
#include "esp_log.h"

#include "sidetone.h"

/*  The timer ISR has an execution time of 5.5 micro-seconds(us).
    Therefore, a timer period less than 5.5 us will cause trigger the interrupt watchdog.
    7 us is a safe interval that will not trigger the watchdog. No need to customize it.
*/

#define WITH_RELOAD            1
#define TIMER_DIVIDER          16
#define POINT_ARR_LEN          1000                             // Length of points array
#define TIMER_TICKS            (APB_CLK_FREQ / TIMER_DIVIDER)   // TIMER_BASE_CLK = APB_CLK = 80MHz
#define DAC_CHAN               DAC_CHANNEL_1    				// DAC_CHANNEL_1 (GPIO25) by default

static int raw_val[POINT_ARR_LEN];
static const char *TAG = "sidetone";

static int output_points = 0;
static int g_index = 0;

/* Timer interrupt service routine */
static void IRAM_ATTR timer0_ISR(void *ptr)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

    int *head = (int*)ptr;

    /* DAC output ISR has an execution time of 4.4 us*/
    if (g_index >= output_points) g_index = 0;
    dac_output_voltage(DAC_CHAN, *(head + g_index));
    g_index++;
}


/* Timer group0 TIMER_0 initialization */
static void tone_timer_init(int timer_idx, int timer_interrupt_usecs, bool auto_reload)
{
    esp_err_t ret;
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .intr_type = TIMER_INTR_LEVEL,
        .auto_reload = auto_reload,
    };

    ret = timer_init(TIMER_GROUP_0, timer_idx, &config);
    ESP_ERROR_CHECK(ret);

    ret = timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);
    ESP_ERROR_CHECK(ret);

    int alarm_usecs = ( timer_interrupt_usecs * TIMER_TICKS) / 1000000;
    ret = timer_set_alarm_value(TIMER_GROUP_0, timer_idx, alarm_usecs);
    ESP_ERROR_CHECK(ret);

    ret = timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    ESP_ERROR_CHECK(ret);
    /* Register an ISR handler */
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer0_ISR, (void *)raw_val, ESP_INTR_FLAG_IRAM, NULL); // @suppress("Symbol is not resolved")
}

 static void prepare_data(int pnt_num, int amplitude)
{
    for (int i = 0; i < pnt_num; i ++)
        raw_val[i] = (int)((sin( i * 2 * M_PI / pnt_num) + 1) * (double)(amplitude)/2 + 0.5);
}

void play_tone( int frequency, int sample_frequency, int amplitude )
{
    esp_err_t ret;

    timer_pause(TIMER_GROUP_0, TIMER_0);

    int timer_interrupt_usecs = 1000000 / sample_frequency;
    output_points = (int)(1000000 / ( timer_interrupt_usecs * frequency ) + 0.5);

    tone_timer_init(TIMER_0, timer_interrupt_usecs, WITH_RELOAD);
    ret = dac_output_enable(DAC_CHAN);
    ESP_ERROR_CHECK(ret);
    g_index = 0;

    prepare_data( output_points, amplitude );

    timer_start(TIMER_GROUP_0, TIMER_0);
}

void stop_tone()
{
    timer_pause(TIMER_GROUP_0, TIMER_0);
    dac_output_disable(DAC_CHAN);

}
