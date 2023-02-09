#ifndef _SH1106_H_
#define _SH1106_H_

// -----------------------------------------------------------------------------
//                               Includes
// -----------------------------------------------------------------------------

#include <stdio.h>
#include "driver/i2c.h"

// -----------------------------------------------------------------------------
//                               Macros and Typedefs
// -----------------------------------------------------------------------------

/* size of screen */
#define SCREEN_WIDTH                            128
#define SCREEN_HEIGHT                           64

/* I2C transmission */
#define SH1106_DEV_ADDR                         0x3C
#define I2C_ACK_EN                              1
#define I2C_ACK_DIS                             0

/* SH1106 commands */
#define SH1106_MEMORYMODE                       0x20
#define SH1106_COLUMNADDR                       0x21
#define SH1106_PAGEADDR                         0x22
#define SH1106_SETCONTRAST                      0x81
#define SH1106_CHARGEPUMP                       0x8D
#define SH1106_SEGREMAP                         0xA0
#define SH1106_DISPLAYALLON_RESUME              0xA4
#define SH1106_DISPLAYALLON                     0xA5
#define SH1106_NORMALDISPLAY                    0xA6
#define SH1106_INVERTDISPLAY                    0xA7
#define SH1106_SETMULTIPLEX                     0xA8
#define SH1106_DCDC                             0xAD
#define SH1106_DISPLAYOFF                       0xAE
#define SH1106_DISPLAYON                        0xAF
#define SH1106_SETPAGEADDR                      0xB0
#define SH1106_COMSCANINC                       0xC0
#define SH1106_COMSCANDEC                       0xC8
#define SH1106_SETDISPLAYOFFSET                 0xD3
#define SH1106_SETDISPLAYCLOCKDIV               0xD5
#define SH1106_SETPRECHARGE                     0xD9
#define SH1106_SETCOMPINS                       0xDA
#define SH1106_SETVCOMDETECT                    0xDB
#define SH1106_SETDISPSTARTLINE                 0xDC
#define SH1106_SETLOWCOLUMN                     0x00
#define SH1106_SETHIGHCOLUMN                    0x10
#define SH1106_SETSTARTLINE                     0x40

/* definition for color of the pixel when display */
typedef enum {
  BLACK = 0, /* turn off the pixel */
  WHITE = 1, /* turn on the pixel */
} SH1106_PIXEL_COLOR;

/* definition for rotation handle */
typedef enum {
  origin  = 0,
  right   = 1,
  flip    = 2,
  left    = 3,
} rotation_dir_t;

/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmap_offset; ///< Pointer into GFXfont->bitmap
  uint8_t width;          ///< Bitmap dimensions in pixels
  uint8_t height;         ///< Bitmap dimensions in pixels
  uint8_t x_advance;      ///< Distance to advance cursor (x axis)
  int8_t x_offset;        ///< X dist from cursor pos to UL corner
  int8_t y_offset;        ///< Y dist from cursor pos to UL corner
} gfx_glyph_t;

/// Data stored for FONT AS A WHOLE
typedef struct {
  uint8_t *bitmap;        ///< Glyph bitmaps, concatenated
  gfx_glyph_t *glyph;     ///< Glyph array
  uint16_t first;         ///< ASCII extents (first char)
  uint16_t last;          ///< ASCII extents (last char)
  uint8_t y_advance;      ///< Newline distance (y axis)
} gfx_font_t;

/** @brief GLIB Drawing Context
 *  (Multiple instances of glib_context_t can exist)
 */
typedef struct __glib_context_t{
  int16_t width;        ///< This is the 'raw' display width - never changes
  int16_t height;       ///< This is the 'raw' display height - never changes
  uint16_t text_color;  ///< Text color
  uint16_t bg_color;    ///< Text background color
  int16_t cursor_x;     ///< x location to start print()ing text
  int16_t cursor_y;     ///< y location to start print()ing text
  uint8_t textsize_x;   ///< Desired magnification in X-axis of text to print()
  uint8_t textsize_y;   ///< Desired magnification in Y-axis of text to print()
  uint8_t rotation;     ///< Display rotation (0 thru 3)
  bool wrap;            ///< If set, 'wrap' text at right edge of display
  bool cp437;           ///< If set, use correct CP437 charset (default is off)
  gfx_font_t *font;     ///< Font definition
} display_context_t;

// -----------------------------------------------------------------------------
//                               Public functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *  Initialize OLED SH1106.
 *
 * @param[in] i2c_num
 *  The I2C port used to communicate with the OLED SH1106.
 * @param[in] context
 *  The pointer to current display context.
 *
 * @return
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_init(i2c_port_t i2c_num, display_context_t *context);

/***************************************************************************//**
 * @brief
 *  Clear all the pixel of the OLED SH1106.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_clear_screen(void);

/***************************************************************************//**
 * @brief
 *  Draw a single pixel on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  The coordinator of the pixel in x axis.
 * @param[in] y
 *  The coordinator of the pixel in y axis.
 * @param[in] color
 *  The color set to the pixel.
 *  WHITE: Turn on the pixel.
 *  BLACK: Turn off the pixel.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_pixel(display_context_t *context,
                            int16_t x, int16_t y,
                            SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a single line on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x0
 *  The coordinator of starting point of the line in x axis.
 * @param[in] y0
 *  The coordinator of starting point of the line in y axis.
 * @param[in] x1
 *  The coordinator of end point of the line in x axis.
 * @param[in] y1
 *  The coordinator of end point of the line in y axis.
 * @param[in] color
 *  Line color.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_line(display_context_t *context,
                           int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a rectangle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  The coordinator of top-left point in x axis.
 * @param[in] y
 *  The coordinator of top-left point in y axis.
 * @param[in] w
 *  The width of the rectangle.
 * @param[in] h
 *  The height of the rectangle.
 * @param[in] color.
 *  The color of the rectangle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_rectangle(display_context_t *context,
                                int16_t x, int16_t y,
                                int16_t w, int16_t h,
                                SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a rectangle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  The coordinator of top-left point in x axis.
 * @param[in] y
 *  The coordinator of top-left point in y axis.
 * @param[in] w
 *  The width of the rectangle.
 * @param[in] h
 *  The height of the rectangle.
 * @param[in] color.
 *  The color of the rectangle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_rectangle(display_context_t *context,
                                     int16_t x, int16_t y,
                                     int16_t w, int16_t h,
                                     SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a triangle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x0
 *  Vertex #0 x coordinate.
 * @param[in] y0
 *  Vertex #0 y coordinate.
 * @param[in] x1
 *  Vertex #1 x coordinate.
 * @param[in] y1
 *  Vertex #1 y coordinate.
 * @param[in] x2
 *  Vertex #2 x coordinate.
 * @param[in] y2
 *  Vertex #2 y coordinate.
 * @param[in] color
 *  Color of the triangle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_triangle(display_context_t *context,
                               int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a filled triangle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x0
 *  Vertex #0 x coordinate.
 * @param[in] y0
 *  Vertex #0 y coordinate.
 * @param[in] x1
 *  Vertex #1 x coordinate.
 * @param[in] y1
 *  Vertex #1 y coordinate.
 * @param[in] x2
 *  Vertex #2 x coordinate.
 * @param[in] y2
 *  Vertex #2 y coordinate.
 * @param[in] color
 *  Color of the triangle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_triangle(display_context_t *context,
                                    int16_t x0, int16_t y0,
                                    int16_t x1, int16_t y1,
                                    int16_t x2, int16_t y2,
                                    SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a circle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  The coordinator of the center of the circle in x axis.
 * @param[in] y
 *  The coordinator of the center of the circle in y axis.
 * @param[in] r
 *  The radius of the circle.
 * @param[in] color
 *  The color of the circle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_circle(display_context_t *context,
                             int16_t x0, int16_t y0, uint8_t r,
                             SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a circle and fill it on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  The coordinator of the center of the circle in x axis.
 * @param[in] y
 *  The coordinator of the center of the circle in y axis.
 * @param[in] r
 *  The radius of the circle.
 * @param[in] color
 *  The color of the circle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_circle(display_context_t *context,
                                  int16_t x0, int16_t y0,
                                  int16_t r, SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a round rectangle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x0
 *  The coordinator of top-left point in x axis.
 * @param[in] y0
 *  The coordinator of top-left point in y axis.
 * @param[in] x1
 *  The coordinator of bottom-right point in x axis.
 * @param[in] y1
 *  The coordinator of bottom-right point in y axis.
 * @param[in] r
 *  Radius of the round corner.
 * @param[in] color
 *  Color of the rectangle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_round_rectangle(display_context_t *context,
                                      int16_t x, int16_t y,
                                      int16_t w, int16_t h,
                                      int16_t r, SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Draw a round rectangle on the SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x0
 *  The coordinator of top-left point in x axis.
 * @param[in] y0
 *  The coordinator of top-left point in y axis.
 * @param[in] x1
 *  The coordinator of bottom-right point in x axis.
 * @param[in] y1
 *  The coordinator of bottom-right point in y axis.
 * @param[in] r
 *  Radius of the round corner.
 * @param[in] color
 *  Color of the rectangle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_round_rectangle(display_context_t *context,
                                           int16_t x, int16_t y,
                                           int16_t w, int16_t h,
                                           int16_t r, SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Set text font.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] font
 *  The font that set to display context.
 *
 ******************************************************************************/
void sh1106_set_text_font(display_context_t *context, const gfx_font_t *font);

/***************************************************************************//**
 * @brief
 *  Draw a character on SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  The coordinate of the character in x axis.
 * @param[in] y
 *  The coordinate of the character in y axis.
 * @param[in] character
 *  The character send to SH1106.
 * @param[in] color
 *  The text color.
 * @param[in] bg
 *  The background color.
 * @param[in] size_x
 *  Magnification of x.
 * @param[in] size_y
 *  Magnification of y.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_char(display_context_t *context,
                           int16_t x, int16_t y,
                           unsigned char c,
                           SH1106_PIXEL_COLOR color, SH1106_PIXEL_COLOR bg,
                           uint8_t size_x, uint8_t size_y);

/***************************************************************************//**
 * @brief
 *  Draw a character on SH1106 at the current position of the cursor.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] c
 *  The character sent to SH1106.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_write_char(display_context_t *context, char c);

/***************************************************************************//**
 * @brief
 *  Draw a string on SH1106 at the current position of the cursor.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] str
 *  The string sent to SH1106.
 * @param[in] x0
 *  Position in x axis.
 * @param[in] yo
 *  Position in y axis.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_write_string(display_context_t *context, const char *str,
                              int16_t x0, int16_t y0);

/***************************************************************************//**
 * @brief
 *  Draw a RAM-resident 1-bit image at the specified (x,y) position
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] x
 *  Position in x axis.
 * @param[in] y
 *  Position in y axis.
 * @param[in] bitmap
 *  The bitmap sent to SH1106.
 * @param[in] w
 *  Width of the bitmap.
 * @param[in] h
 *  Height of the bitmap.
 * @param[in] color
 *  Color of the bitmap.
 * @param[in] bg
 *  Background color of the bitmap.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_draw_bitmap(display_context_t *context,
                             int16_t x, int16_t y, uint8_t *bitmap,
                             int16_t w, int16_t h,
                             SH1106_PIXEL_COLOR color,
                             SH1106_PIXEL_COLOR bg);

/***************************************************************************//**
 * @brief
 *  Set rotation direction for OLED SH1106.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] dir
 *  Direction.
 ******************************************************************************/
void sh1106_set_rotation(display_context_t *context, rotation_dir_t dir);

/***************************************************************************//**
 * @brief
 *  Fill all the pixel of OLED SH1106 with desired color.
 *
 * @param[in] context
 *  The pointer to current display context.
 * @param[in] color
 *  Fill color.
 ******************************************************************************/
esp_err_t sh1106_fill_screen(display_context_t *context,
                             SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Push data from buffer to OLED SH1106.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
esp_err_t sh1106_update_display();

#endif /* _SH1106_H_ */
