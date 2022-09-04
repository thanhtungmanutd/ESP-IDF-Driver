#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "string.h"
#include "sh1106_i2c.h"
#include <stdio.h>

// -----------------------------------------------------------------------------
//                       Local Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Send command to oled.
 ******************************************************************************/
static esp_err_t sh1106_send_cmd(uint8_t *cmd, uint8_t len);

/***************************************************************************//**
 *  Send data to oled.
 ******************************************************************************/
static esp_err_t sh1106_send_data(uint8_t *pdata, uint8_t len);

/***************************************************************************//**
 *  Set page address.
 ******************************************************************************/
static esp_err_t sh1106_set_page_address(uint8_t page);

/***************************************************************************//**
 *  Send column address.
 ******************************************************************************/
static esp_err_t sh1106_set_column_address(uint8_t col);

esp_err_t sh1106_init(void)
{
    uint8_t cmd = SH1106_DISPLAY_ON;
    esp_err_t status = ESP_OK;
    uint8_t init_buff[] = {
        SH1106_DISPLAY_OFF,
        SH1106_SET_DISPLAY_CLOCK_DIVIDE,
        0x80,                               /* 0x80 Divide Ratio = 0,
                                            Divide frequency = +15% */
        SH1106_SET_MULTIPLEX,
        0x3F,                               /* 0x3F Multiplex Ratio = 64 */
        SH1106_SET_DISPLAY_OFFSET,
        0x00,                               /* 0x00 Display offset = 0 */
        SH1106_SET_START_LINE(0),           /* 0x40 Set display start line */
        SH1106_SET_DCDC_ONOFF_MODE,         /* 0xAD Set DCDC on/off mode */
        0x8B,
        SH1106_SEGREMAP_LEFT,
        SH1106_COM_SCAN_DEC,                /* Scan output direction from
                                            COM[N-1] to COM0 */
        SH1106_SET_HARD_CONF_MODE,
        0x12,
        SH1106_SET_CONTRAST,
        0xFF,                               /* Contrast data = 256 */
        SH1106_SET_DIS_PRE_CHARGE,
        0x1F,                               /* Dis-charge period = 1,
                                            Pre-charge period = 15 */
        SH1106_SET_VCOM_DESELECT,
        0x40,
        0x33,                               /* Set Vpp (Pump voltage)
                                            0x33 <-> Vpp = 9V */
        SH1106_NORMAL_DISPLAY,
        0x20, 0x10,
        SH1106_ENTIRE_DISPLAY_RESUME
    };
    status += sh1106_send_cmd(init_buff, sizeof(init_buff));
    vTaskDelay(pdMS_TO_TICKS(100));
    status += sh1106_send_cmd(&cmd, 1);

    return status;
}

esp_err_t sh1106_clear_display()
{
    esp_err_t status = ESP_OK;
    uint8_t page;
    uint8_t data[128];

    for(page = 0; page < 8; page++) {
        status += sh1106_set_page_address(page);
        status += sh1106_set_column_address(0);
        memset(data, 0 , sizeof(data));

        // send data to oled's RAM
        status += sh1106_send_data(data, sizeof(data));
        if(status != ESP_OK) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t sh1106_display()
{
    esp_err_t status = ESP_OK;
    uint8_t data;

    status += sh1106_set_page_address(0);
    status += sh1106_set_column_address(0);

    data = 1;
    status += sh1106_send_data(&data, 1);
    if(status != ESP_OK) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
// -----------------------------------------------------------------------------
//                       Local Function Definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *  Set column address to send data to OLED.
 *
 * @param[in] col
 *  The column address that want to set.
 *
 * @details
 *  This function sets the column address before sending data to OLED
 *
 * @return
 *  Return ESP_OK if succeed, ESP_FAIL if fail.
 *
 ******************************************************************************/
static esp_err_t sh1106_set_column_address(uint8_t col)
{
    uint8_t cmd;
    esp_err_t status = ESP_OK;

    if(col > 131) {
        return ESP_ERR_INVALID_ARG;
    }

    // set higher column address
    cmd = (col >> 4) | 0x10;
    status += sh1106_send_cmd(&cmd, 1);

    // set lower column address
    cmd = (col & 0x0F) | 0x00;
    status += sh1106_send_cmd(&cmd, 1);

    return status;
}

/***************************************************************************//**
 * @brief
 *  Set page address to send data to OLED.
 *
 * @param[in] page
 *  The page address that want to set.
 *
 * @details
 *  This function sets the page address before sending data to OLED. The page
 *  must be less than 8.
 *
 * @return
 *  Return ESP_OK if succeed, ESP_FAIL if fail.
 *
 ******************************************************************************/
static esp_err_t sh1106_set_page_address(uint8_t page)
{
    uint8_t cmd;

    if(page > 7) {
        return ESP_ERR_INVALID_ARG;
    }
    cmd = 0xB0 | page;
    return sh1106_send_cmd(&cmd, 1);
}

/***************************************************************************//**
 * @brief
 *  Send command to OLED.
 *
 * @param[in] cmd
 *  Command to send to OLED.
 * @param[in] len
 *  Length of command that send to OLED.
 *
 * @details
 *  This function sends a block of command to OLED.
 *
 * @return
 *  Return ESP_OK if succeed, ESP_FAIL if fail.
 *
 ******************************************************************************/
static esp_err_t sh1106_send_cmd(uint8_t *cmd, uint8_t len)
{
    esp_err_t status;
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();

    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (DEV_ADDRR << 1) | WR_BIT, ACK_EN);
    i2c_master_write_byte(i2c_cmd, 0x00, ACK_EN);
    i2c_master_write(i2c_cmd, cmd, len, ACK_EN);
    i2c_master_stop(i2c_cmd);

    status = i2c_master_cmd_begin(I2C_NUM_0, i2c_cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(i2c_cmd);

    return status;
}

/***************************************************************************//**
 * @brief
 *  Send data to OLED.
 *
 * @param[in] pdata
 *  Data to send to OLED.
 * @param[in] len
 *  Length of data that send to OLED.
 *
 * @details
 *  This function sends a block of data to OLED.
 *
 * @return
 *  Return ESP_OK if succeed, ESP_FAIL if fail.
 *
 ******************************************************************************/
static esp_err_t sh1106_send_data(uint8_t *pdata, uint8_t len)
{
    esp_err_t status;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DEV_ADDRR << 1) | WR_BIT, ACK_EN);
    i2c_master_write_byte(cmd, 0x40, ACK_EN);
    i2c_master_write(cmd, cmd, len, ACK_EN);
    i2c_master_stop(cmd);

    status = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return status;
}