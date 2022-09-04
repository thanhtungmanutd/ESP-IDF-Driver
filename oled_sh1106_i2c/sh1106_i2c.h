#ifndef _SH1106_I2C_H_
#define _SH1106_I2C_H_

// -----------------------------------------------------------------------------
//                                Macros
// -----------------------------------------------------------------------------

#define DEV_ADDRR                                      0x3C
#define RD_BIT                                         1
#define WR_BIT                                         0
#define ACK_EN                                         1
#define ACK_DIS                                        0

#define SH1106_ENTIRE_DISPLAY_ON                       0xA5
#define SH1106_ENTIRE_DISPLAY_RESUME                   0xA4

// set pump voltage
#define SH1106_SET_PUMP_VOLTAGE(x)                     (0x30 | x)

/* set display start line */
#define SH1106_SET_START_LINE(x)                       (0x40 | x)

// set contrast
#define SH1106_SET_CONTRAST                            0x81

// set segment re-map
#define SH1106_SEGREMAP_RIGHT                          0xA0
#define SH1106_SEGREMAP_LEFT                           0xA1

// set display normal(0): low reverse(0): high
#define SH1106_NORMAL_DISPLAY                          0xA6
#define SH1106_REVERSE_DISPLAY                         0xA7

// set multiplex ration
#define SH1106_SET_MULTIPLEX                           0xA8

// set dc-dc on/off
#define SH1106_SET_DCDC_ONOFF_MODE                     0xAD
#define SH1106_DCDC_ON                                 0x8B
#define SH1106_DCDC_OFF                                0x8A

// set display on/off
#define SH1106_DISPLAY_OFF                             0xAE
#define SH1106_DISPLAY_ON                              0xAF

// set common output scan direction
#define SH1106_COM_SCAN_INC                            0xC0
#define SH1106_COM_SCAN_DEC                            0xC8

// set display offset
#define SH1106_SET_DISPLAY_OFFSET                      0xD3

// set display clock division ratio and oscillation frequency
#define SH1106_SET_DISPLAY_CLOCK_DIVIDE                0xD5

// set dis-charge/pre-charge period
#define SH1106_SET_DIS_PRE_CHARGE                      0xD9

// set hardware configuration mode
#define SH1106_SET_HARD_CONF_MODE                      0xDA

// set vcom deselect level
#define SH1106_SET_VCOM_DESELECT                       0xDB





esp_err_t sh1106_init(void);
esp_err_t sh1106_draw_pixel(int16_t x, int16_t y);
esp_err_t sh1106_clear_display();
esp_err_t sh1106_display();

#endif /* _SH1106_I2C_H_ */