/******************************************************************************
*@file: dht.h
*@brief: header file for liquid crystal display
*@author: TungDT53
*@date: 1/6/2022
*******************************************************************************/
#ifndef _LCD_I2C_H_
#define _LCD_I2C_H_

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

typedef enum {
    LCD_08x02 = 0u,
    LCD_08x04,
    LCD_16x02,
    LCD_16x04,
    LCD_20x02,
    LCD_20x04
} lcd_typedef_t; /**< lcd type definition */

// I2C config
#define DEV_ADDRR                 0x27
#define I2C_NUM(x)                I2C_NUM_##x
#define I2C_DEV                   I2C_NUM(0)
#define SCL_PIN                   GPIO_NUM_17
#define SDA_PIN                   GPIO_NUM_16
#define I2C_FREQ                  1000000
#define RD_BIT                    0x01
#define WR_BIT                    0x00
#define ACK_EN                    0x01
#define ACK_DIS                   0x00

// lcd config
#define BACKLIGHT                 0x08
#define NO_BACKLIGHT              0x00
#define EN                        0x04
#define RW                        0x02
#define RS                        0x01
#define CMD                       0x00
#define DATA                      0x01

// commands
#define CLEAR_DISPLAY             0x01
#define RETURN_HOME               0x02
#define ENTRY_MODE_SET            0x04
#define DISPLAY_CONTROL           0x08
#define CURSOR_SHIFT              0x10
#define FUNCTION_SET              0x20
#define SET_CGRAM_ADD             0x40
#define SET_DDRAM_ADD             0x80

// flags for function set
#define MODE_8BITS                0x10
#define MODE_4BITS                0x00
#define MODE_2LINE                0x08
#define MODE_1LINE                0x00
#define MODE_5x10DOTS             0x04
#define MODE_5x8DOTS              0x00

// flags for display on/off control
#define DISPLAY_ON                0x04
#define DISPLAY_OFF               0x00
#define CURSOR_ON                 0x02
#define CURSOR_OFF                0x00
#define BLINK_ON                  0x01
#define BLINK_OFF                 0x00

// flags for display/cursor shift
#define DISPLAY_MOVE              0x08
#define CURSOR_MOVE               0x00
#define MOVE_RIGHT                0x04
#define MOVE_LEFT                 0x00

// flags for display entry mode
#define ENTRY_RIGHT               0x02
#define ENTRY_LEFT                0x00
#define SHIFT_ON                  0x01
#define SHIFT_OFF                 0x00

// -----------------------------------------------------------------------------
//                       Public Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *  This function initialize the configuration for Liquid Crystal Display
 *
 * @param lcd
 *  Type of Liquid Crystal Display (rows and cols)
 *
 * @return
 *  Return ESP_OK or ESP_FAIL
 *
 ******************************************************************************/
esp_err_t lcd_i2c_init(lcd_typedef_t lcd);

/***************************************************************************//**
 * @brief
 *  This function send command to Liquid Crystal Display
 *
 * @param cmd
 *  Command send to Liquid Crystal Display
 *
 * @return
 *  Return ESP_OK or ESP_FAIL
 *
 ******************************************************************************/
esp_err_t lcd_i2c_write_cmd(uint8_t cmd);

/***************************************************************************//**
 * @brief
 *  This function send a character to Liquid Crystal Display
 *
 * @param character
 *  Character send to Liquid Crystal Display
 *
 * @return
 *  Return ESP_OK or ESP_FAIL
 *
 ******************************************************************************/
esp_err_t lcd_i2c_write_char(char character);

/***************************************************************************//**
 * @brief
 *  This function send a string to Liquid Crystal Display
 *
 * @param str
 *  Pointer to the string send to Liquid Crystal Display
 *
 * @return
 *  Return ESP_OK or ESP_FAIL
 *
 ******************************************************************************/
esp_err_t lcd_i2c_write_str(char *str);

/***************************************************************************//**
 * @brief
 *  This function set the position of the cursor
 *
 * @param row
 *  The line that want to set the cursor
 * @param col
 *  The column that want to set the cursor
 *
 * @return
 *  Return ESP_OK or ESP_FAIL
 *
 ******************************************************************************/
esp_err_t lcd_i2c_set_cursor(uint8_t row, uint8_t col);

#endif /* _LCD_I2C_H_ */

// -----------------------------------------------------------------------------
//                              EOF
// -----------------------------------------------------------------------------