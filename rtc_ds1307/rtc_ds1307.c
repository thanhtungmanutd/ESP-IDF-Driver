// -----------------------------------------------------------------------------
//                               Includes
// -----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtc_ds1307.h"

// -----------------------------------------------------------------------------
//                               Macros
// -----------------------------------------------------------------------------

#define DEV_ADDR            0x68
#define ACK_EN              0x01
#define ACK_DIS             0x00

// convert BCD format to Binary format
#define BCD_2_BIN(x)        ((x) - 6 * ((x) >> 4))
// convert Binary format to BCD format
#define BIN_2_BCD(x)        ((x) + 6 * ((x) / 10))

// -----------------------------------------------------------------------------
//                               Local variables
// -----------------------------------------------------------------------------

static i2c_port_t rtc_ds1307_i2c_port;
const uint8_t days_in_month[11] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};
// -----------------------------------------------------------------------------
//                               Local functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Send data to RTC DS1307.
 ******************************************************************************/
static esp_err_t rtc_ds1307_i2c_send_data(uint8_t *data, uint8_t len)
{
    esp_err_t status;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd,
                                         (DEV_ADDR << 1) | I2C_MASTER_WRITE,
                                          ACK_EN));
    ESP_ERROR_CHECK(i2c_master_write(cmd, data, len, ACK_EN));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    status = i2c_master_cmd_begin(rtc_ds1307_i2c_port,
                                  cmd,
                                  pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return status;
}

/***************************************************************************//**
 *  Get day of the week function.
 ******************************************************************************/
static uint8_t rtc_ds1307_get_day_of_week(uint16_t year,
                                          uint8_t month,
                                          uint8_t date)
{
    if(year >= 2000U) {
        year -= 2000U;
    }
    uint16_t days = date;
    for(uint8_t i = 1; i < month; ++i) {
        days += *(const unsigned char *)(days_in_month + i - 1);
    }
    if ((month > 2) && ((year % 4) == 0)) {
        ++days;
    }
    return (days + 365 * year + (year + 3) / 4 - 1 + 6) % 7;
}

// -----------------------------------------------------------------------------
//                               Public functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  DS1307 Initialization.
 ******************************************************************************/
void rtc_ds1307_init(i2c_port_t i2c_num)
{
    rtc_ds1307_i2c_port = i2c_num;
}

/***************************************************************************//**
 *  Set date and time for RTC DS1307.
 ******************************************************************************/
esp_err_t rtc_ds1307_set_date_time(uint16_t year, uint8_t month, uint8_t date,
                                   uint8_t hour, uint8_t minute, uint8_t second)
{
    uint8_t buffer[8] = {
        0,
        BIN_2_BCD(second),
        BIN_2_BCD(minute),
        BIN_2_BCD(hour),
        BIN_2_BCD(rtc_ds1307_get_day_of_week(year, month, date)),
        BIN_2_BCD(date),
        BIN_2_BCD(month),
        BIN_2_BCD((year - 2000)),
    };
    return rtc_ds1307_i2c_send_data(buffer, 8);
}

/***************************************************************************//**
 *  Get date and time from RTC DS1307.
 ******************************************************************************/
esp_err_t rtc_ds1307_get_current_date_time(date_time_t *dt)
{
    esp_err_t status;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    uint8_t recv_data[7];

    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd,
                                          (DEV_ADDR << 1) | I2C_MASTER_WRITE,
                                          ACK_EN));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x00, ACK_EN));

    // Repeat start
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd,
                                          (DEV_ADDR << 1) | I2C_MASTER_READ,
                                          ACK_EN));
    ESP_ERROR_CHECK(i2c_master_read(cmd, recv_data, 6, I2C_MASTER_ACK));
    ESP_ERROR_CHECK(i2c_master_read(cmd, recv_data + 6, 1, I2C_MASTER_NACK));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    status = i2c_master_cmd_begin(rtc_ds1307_i2c_port,
                                  cmd,
                                  pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    dt->year = BCD_2_BIN(recv_data[6]) + 2000U;
    dt->month = BCD_2_BIN(recv_data[5]);
    dt->date = BCD_2_BIN(recv_data[4]);
    dt->day_of_week = BCD_2_BIN(recv_data[3]);
    dt->hour = BCD_2_BIN(recv_data[2]);
    dt->minute = BCD_2_BIN(recv_data[1]);
    dt->second = BCD_2_BIN(recv_data[0]);

    return status;
}