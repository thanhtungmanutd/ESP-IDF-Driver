#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "sh1106_i2c.h"

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_27;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = GPIO_NUM_26;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 1000000;

    i2c_param_config(I2C_NUM_0, &conf);
    return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

void app_main(void)
{
    printf("init: %d\n",i2c_master_init());
    printf("oled init: %d\n",sh1106_init());
    printf("clear: %d\n",sh1106_clear_display());
    // printf("display: %d\n",sh1106_display());
}