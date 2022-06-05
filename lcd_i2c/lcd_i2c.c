// -----------------------------------------------------------------------------
//                              Includes
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "esp32/rom/ets_sys.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lcd_i2c.h"

// -----------------------------------------------------------------------------
//                              Variables
// -----------------------------------------------------------------------------

/* TAG for log */
static const char *TAG_ERROR = "error: ";
static const char *TAG_INFO = "";

/* configuration for i2c master */
static i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .scl_io_num = SCL_PIN,
    .sda_io_num = SDA_PIN,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_FREQ
};

static lcd_typedef_t lcd_obj;

// -----------------------------------------------------------------------------
//                       Local Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *   Send one data byte to slave
 *
 * @param data
 *   Data sent to slave by master
 *
 * @return
 *   Return value is ESP_OK or ESP_FAIL
 ******************************************************************************/
static esp_err_t i2c_write_byte(uint8_t data);

/***************************************************************************//**
 * @brief
 *   Clock data into the Liquid Crystal Display
 *
 * @param data
 *   Data clocked into the Liquid Crystal Display
 *
 * @return
 *   Return value is ESP_OK or ESP_FAIL
 ******************************************************************************/
static esp_err_t lcd_i2c_pulse_enable(uint8_t data);

/***************************************************************************//**
 * @brief
 *   Send 4 bits of data to Liquid Crystal Display
 *
 * @param data
 *   Data clocked into the Liquid Crystal Display
 *
 * @return
 *   Return value is ESP_OK or ESP_FAIL
 ******************************************************************************/
static esp_err_t lcd_i2c_write_4_bits(uint8_t data);

/***************************************************************************//**
 * @brief
 *  This function initialize the configuration for I2C Master
 *
 * @return
 *  Return ESP_OK or ESP_FAIL
 ******************************************************************************/
static esp_err_t i2c_master_init();

// -----------------------------------------------------------------------------
//                       Public Function Definitions
// -----------------------------------------------------------------------------

esp_err_t lcd_i2c_write_cmd(uint8_t cmd)
{
    uint8_t high_nib = 0u;
	uint8_t low_nib  = 0u;

    high_nib = cmd & 0xF0;
    low_nib = (cmd << 4) & 0xF0;

    if(lcd_i2c_write_4_bits(high_nib | CMD) != ESP_OK)
        return ESP_FAIL;
    if(lcd_i2c_write_4_bits(low_nib | CMD) != ESP_OK)
        return ESP_FAIL;
    if((cmd == CLEAR_DISPLAY) | (cmd == RETURN_HOME)) {
        ets_delay_us(2000);
    }
    return ESP_OK;
}


esp_err_t lcd_i2c_init(lcd_typedef_t lcd)
{
    uint8_t check = 0u;

    lcd_obj = lcd;
    if(i2c_master_init() != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"init i2c failed");
        check++;
    }
    else
        ESP_LOGI(TAG_INFO,"i2c master init done");

    vTaskDelay(pdMS_TO_TICKS(50));
    if(i2c_write_byte(0x00) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"fail to send 0x00");
        check++;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    if(lcd_i2c_write_4_bits(0x03 << 4) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"failed to send 0x30");
        check++;
    }
    ets_delay_us(4500);
    if(lcd_i2c_write_4_bits(0x03 << 4) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"failed to send 0x30");
        check++;
    }
    ets_delay_us(4500);
    if(lcd_i2c_write_4_bits(0x03 << 4) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"failed to send 0x30");
        check++;
    }
    ets_delay_us(150);
    if(lcd_i2c_write_4_bits(0x02 << 4) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"failed to send 0x20");
        check++;
    }

    if(lcd_i2c_write_cmd(FUNCTION_SET | MODE_4BITS | MODE_2LINE | MODE_5x8DOTS) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"function set fail");
        check++;
    }
    if(lcd_i2c_write_cmd(DISPLAY_CONTROL | DISPLAY_ON | CURSOR_OFF | BLINK_OFF) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"display control set fail");
        check++;
    }
    if(lcd_i2c_write_cmd(ENTRY_MODE_SET | ENTRY_LEFT | SHIFT_OFF)) {
        ESP_LOGE(TAG_ERROR,"entry mode set fail");
        check++;
    }

    if(lcd_i2c_write_cmd(CLEAR_DISPLAY) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"clear display fail");
        check++;
    }
    if(lcd_i2c_write_cmd(RETURN_HOME) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"return home fail");
        check++;
    }

    if(!check)
        return ESP_OK;
    else
        return ESP_FAIL;
}

esp_err_t lcd_i2c_write_char(char character)
{
    uint8_t high_nib = 0u;
	uint8_t low_nib  = 0u;

    high_nib = character & 0xF0;
    low_nib = (character << 4) & 0xF0;

    if(lcd_i2c_write_4_bits(high_nib | DATA) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"send high bits fail");
        return ESP_FAIL;
    }
    if(lcd_i2c_write_4_bits(low_nib | DATA) != ESP_OK) {
        ESP_LOGE(TAG_ERROR,"send low bits fail");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t lcd_i2c_write_str(char *str)
{
    esp_err_t ret = ESP_OK;
    uint8_t index = 0u;

    for(index = 0u; index < strlen(str); index++) {
        ret = lcd_i2c_write_char(str[index]);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG_ERROR, "write char %c failed",str[index]);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t lcd_i2c_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t cmd = 0u;

    if(lcd_obj == LCD_16x02) {
        if(row == 1)
            cmd = 0x40 + col;
        else if (row == 0)
            cmd = col;
        else
            return ESP_FAIL;
        cmd |= 0x80;
    }
    return lcd_i2c_write_cmd(cmd);
}

// -----------------------------------------------------------------------------
//                       Local Function Definitions
// -----------------------------------------------------------------------------

static esp_err_t i2c_master_init(void)
{
    esp_err_t status = ESP_OK;

    /* install i2c driver*/
    i2c_param_config(I2C_DEV, &conf);
    status = i2c_driver_install(I2C_DEV, conf.mode, 0, 0, 0);

    return status;
}

static esp_err_t i2c_write_byte(uint8_t data)
{
    esp_err_t status;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (DEV_ADDRR << 1) | WR_BIT, ACK_EN));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, data | BACKLIGHT, ACK_EN));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    status = i2c_master_cmd_begin(I2C_DEV, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return status;
}

static esp_err_t lcd_i2c_pulse_enable(uint8_t data)
{
    if(i2c_write_byte(data | EN) != ESP_OK)
        return ESP_FAIL;
    ets_delay_us(1);
    if(i2c_write_byte(data & ~EN) != ESP_OK)
        return ESP_FAIL;
    ets_delay_us(50);

    return ESP_OK;
}

static esp_err_t lcd_i2c_write_4_bits(uint8_t data)
{
    if(i2c_write_byte(data) != ESP_OK)
        return ESP_FAIL;
    if(lcd_i2c_pulse_enable(data) != ESP_OK)
        return ESP_FAIL;

    return ESP_OK;
}

// -----------------------------------------------------------------------------
//                              EOF
// -----------------------------------------------------------------------------