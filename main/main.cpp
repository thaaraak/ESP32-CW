#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include "si5351.h"

#define I2C_MASTER_NUM	0
#define I2C_MASTER_SDA_IO 26
#define I2C_MASTER_SCL_IO 27

extern "C" void app_main();

#include "button.h"
#include "sidetone.h"

Si5351 synth;


static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void app_main()
{

	int ret = i2c_master_init();
	printf( "Return: %d\n", ret );

	synth.init( I2C_MASTER_NUM, SI5351_CRYSTAL_LOAD_8PF, 25000000, 0 );



//	changeFrequency( 14000000 );

	button_event_t ev;

	unsigned long long TRANSMIT = 13;

	QueueHandle_t button_events = pulled_button_init( PIN_BIT(TRANSMIT), GPIO_PULLUP_ONLY );

    gpio_pad_select_gpio(GPIO_NUM_32);
    gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_32, 0);

	int currentFrequency = 7000000;
	uint64_t freq = currentFrequency * 100ULL;
	synth.set_freq(freq, SI5351_CLK0);
    synth.output_enable( SI5351_CLK0, 0 );

    esp_log_level_set("BUTTON", ESP_LOG_WARN);      // enable WARN logs from WiFi stack


    while(1)
    {
	    if (xQueueReceive(button_events, &ev, 1000/portTICK_PERIOD_MS)) {
	        if ((ev.pin == TRANSMIT) && (ev.event == BUTTON_DOWN)) {
	            printf( "Button Down\n");
	    	    gpio_set_level(GPIO_NUM_32, 1);
	    	    synth.output_enable( SI5351_CLK0, 1 );
	    	    play_tone( 700, 20000, 90 );
	        }
	        if ((ev.pin == TRANSMIT) && (ev.event == BUTTON_UP)) {
	            printf( "Button Up\n");
	    	    gpio_set_level(GPIO_NUM_32, 0);
	        	synth.output_enable( SI5351_CLK0, 0 );
	        	stop_tone();

	        }
	    }

    }
}
