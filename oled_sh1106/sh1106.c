// -----------------------------------------------------------------------------
//                               Includes
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "sh1106.h"

// -----------------------------------------------------------------------------
//                               Macros
// -----------------------------------------------------------------------------

#define swap(a, b) \
        { int16_t t = a; a = b; b = t; }

#define sh1106_draw_hline(g, x, y, w, color) \
        sh1106_draw_line(g, x, y, x + w - 1, y, color)
#define sh1106_draw_vline(g, x, y, w, color) \
        sh1106_draw_line(g, x, y, x, y + w - 1, color)

// -----------------------------------------------------------------------------
//                               Local variables
// -----------------------------------------------------------------------------

static i2c_port_t sh1106_i2c_port;
static uint8_t frame_buffer[SCREEN_WIDTH * (SCREEN_HEIGHT / 8)];

// -----------------------------------------------------------------------------
//                            Local functions declaration
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *  Send command to SH1106.
 *
 * @param[in] command
 *  Command list to send.
 * @param[in] cmd_len
 *  The length of command list sent to SH1106
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
static esp_err_t sh1106_send_command(uint8_t *command, uint8_t cmd_len);

/***************************************************************************//**
 * @brief
 *  Send data to SH1106.
 *
 * @param[in] command
 *  Data list to send.
 * @param[in] len
 *  The length of data list sent to SH1106
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
static esp_err_t sh1106_send_data(uint8_t *data, uint8_t len);

/***************************************************************************//**
 * @brief
 *  This function hepls to draw round rectangle or fill circle.
 *
 * @param[in] x0
 *  Center-point x coordinate.
 * @param[in] yo
 *  Center-point y coordinate.
 * @param[in] r
 *  Radius of circle.
 * @param[in] corners
 *  Mask bits indicating which quarters we're doing.
 * @param[in] delta
 *  Offset from center-point, used for round-rects.
 * @param[in] color
 *  Color of the quater.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
static esp_err_t sh1106_fill_circle_helper(display_context_t *context,
                                           int16_t x0, int16_t y0, int16_t r,
                                           uint8_t corners, int16_t delta,
                                           SH1106_PIXEL_COLOR color);

/***************************************************************************//**
 * @brief
 *  Quarter-circle drawer, used to do circles and roundrects.
 *
 * @param[in] x0
 *  Center-point x coordinate.
 * @param[in] yo
 *  Center-point y coordinate.
 * @param[in] radius
 *  Radius of circle.
 * @param[in] corners
 *  Mask bit #1 or bit #2 to indicate which quarters of
 *  the circle we're doing
 * @param[in] color
 *  Color of the circle.
 *
 * @return
 *  ESP_OK            if OK.
 *  Other return code if Failed.
 ******************************************************************************/
static esp_err_t sh1106_draw_circle_helper(display_context_t *context,
                                           int16_t x0, int16_t y0, int16_t r,
                                           uint8_t cornername,
                                           SH1106_PIXEL_COLOR color);

// -----------------------------------------------------------------------------
//                               Public functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Initialize OLED SH1106.
 ******************************************************************************/
esp_err_t sh1106_init(i2c_port_t i2c_num, display_context_t *context)
{
  esp_err_t status = ESP_OK;
  uint8_t last_cmd;

  // Init context display
  context->bg_color = BLACK; context->text_color = WHITE;
  context->cursor_x = context->cursor_y = 0;
  context->rotation = origin;
  context->cp437 = context->wrap = false;
  context->width = SCREEN_WIDTH;
  context->height = SCREEN_HEIGHT;
  context->font = NULL;
  context->textsize_x = context->textsize_y = 1;

  // Assign I2C port
  sh1106_i2c_port = i2c_num;

  // Send initial command to SH1106
  uint8_t init_cmd[] = {
    SH1106_DISPLAYOFF,                   // 0xAE
    SH1106_SETDISPLAYCLOCKDIV, 0x80,     // 0xD5, 0x80,
    SH1106_SETMULTIPLEX, 0x3F,           // 0xA8, 0x3F,
    SH1106_SETDISPLAYOFFSET, 0x00,       // 0xD3, 0x00,
    SH1106_SETSTARTLINE,                 // 0x40
    SH1106_DCDC, 0x8B,                   // DC/DC on
    SH1106_SEGREMAP + 1,                 // 0xA1
    SH1106_COMSCANDEC,                   // 0xC8
    SH1106_SETCOMPINS, 0x12,             // 0xDA, 0x12,
    SH1106_SETCONTRAST, 0xFF,            // 0x81, 0xFF
    SH1106_SETPRECHARGE, 0x1F,           // 0xD9, 0x1F,
    SH1106_SETVCOMDETECT, 0x40,          // 0xDB, 0x40,
    0x33,                                // Set VPP to 9V
    SH1106_NORMALDISPLAY,
    SH1106_MEMORYMODE, 0x10,             // 0x20, 0x00
    SH1106_DISPLAYALLON_RESUME,
  };
  status = sh1106_send_command(init_cmd, sizeof(init_cmd));
  if (status != ESP_OK) {
    return status;
  }
  vTaskDelay(pdMS_TO_TICKS(100));
  last_cmd = SH1106_DISPLAYON;
  return sh1106_send_command(&last_cmd, 1);
}

/***************************************************************************//**
 *  Clear SH1106 screen.
 ******************************************************************************/
esp_err_t sh1106_clear_screen(void)
{
  memset(frame_buffer, 0, sizeof(frame_buffer));
  return ESP_OK;
}

/***************************************************************************//**
 *  Draw a pixel on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_pixel(display_context_t *context,
                            int16_t x, int16_t y,
                            SH1106_PIXEL_COLOR color)
{
  if ((x < context->width) && (y < context->height) && (x > 0) && (y > 0)) {
    int16_t t;
    switch (context->rotation) {
      case 1:
        t = x;
        x = context->width - 1 - y;
        y = t;
        break;
      case 2:
        x = context->width - 1 - x;
        y = context->height - 1 - y;
        break;
      case 3:
        t = x;
        x = y;
        y = context->height - 1 - t;
        break;
    }
    if (color == WHITE) {
      frame_buffer[128 * (y / 8) + x] |= (1 << (y % 8));
    } else {
      frame_buffer[128 * (y / 8) + x] &= ~(1 << (y % 8));
    }
  }
  return ESP_OK;
}

/***************************************************************************//**
 *  Draw a line on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_line(display_context_t *context,
                           int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;
  int16_t steep = steep = abs(y1 - y0) > abs(x1 - x0);
  int16_t err;
  int16_t dx, dy;
  int16_t ystep;

  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }
  dx = x1 - x0;
  dy = abs(y1 - y0);
  err = dx / 2;
  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }
  for (; x0 <= x1; x0++) {
    if (steep) {
      status |= sh1106_draw_pixel(context, y0, x0, color);
    } else {
      status |= sh1106_draw_pixel(context, x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a rectangle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_rectangle(display_context_t *context,
                                int16_t x, int16_t y,
                                int16_t w, int16_t h,
                                SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  status |= sh1106_draw_hline(context, x, y, w, color);
  status |= sh1106_draw_hline(context, x, y + h - 1, w, color);
  status |= sh1106_draw_vline(context, x, y, h, color);
  status |= sh1106_draw_vline(context, x + w - 1, y, h, color);

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a filled rectangle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_rectangle(display_context_t *context,
                                     int16_t x, int16_t y,
                                     int16_t w, int16_t h,
                                     SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  for (int16_t i = x; i < x + w; i++) {
    status |= sh1106_draw_vline(context, i, y, h, color);
  }

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a triangle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_triangle(display_context_t *context,
                               int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  status |= sh1106_draw_line(context, x0, y0, x1, y1, color);
  status |= sh1106_draw_line(context, x1, y1, x2, y2, color);
  status |= sh1106_draw_line(context, x2, y2, x0, y0, color);

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a filled triangle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_triangle(display_context_t *context,
                                    int16_t x0, int16_t y0,
                                    int16_t x1, int16_t y1,
                                    int16_t x2, int16_t y2,
                                    SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;
  int16_t a, b, y, last;

  if (y0 > y1) {
    swap(y0, y1);
    swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1);
    swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1);
    swap(x0, x1);
  }

  if (y0 == y2) {
    a = b = x0;
    if (x1 < a) {
      a = x1;
    } else if (x1 > b) {
      b = x1;
    }
    if (x2 < a) {
      a = x2;
    } else if (x2 > b) {
      b = x2;
    }
    return sh1106_draw_hline(context, a, y0, b - a + 1, color);
  }

  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;

  if (y1 == y2) {
    last = y1;
  } else {
    last = y1 - 1;
  }
  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b) {
      swap(a, b);
    }
    status |= sh1106_draw_hline(context, a, y, b - a + 1, color);
  }
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b) {
      swap(a, b);
    }
    status |= sh1106_draw_hline(context, a, y, b - a + 1, color);
  }

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a circle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_circle(display_context_t *context,
                             int16_t x0, int16_t y0, uint8_t r,
                             SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  status |= sh1106_draw_pixel(context, x0, y0 + r, color);
  status |= sh1106_draw_pixel(context, x0, y0 - r, color);
  status |= sh1106_draw_pixel(context, x0 + r, y0, color);
  status |= sh1106_draw_pixel(context, x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    status |= sh1106_draw_pixel(context, x0 + x, y0 + y, color);
    status |= sh1106_draw_pixel(context, x0 - x, y0 + y, color);
    status |= sh1106_draw_pixel(context, x0 + x, y0 - y, color);
    status |= sh1106_draw_pixel(context, x0 - x, y0 - y, color);
    status |= sh1106_draw_pixel(context, x0 + y, y0 + x, color);
    status |= sh1106_draw_pixel(context, x0 - y, y0 + x, color);
    status |= sh1106_draw_pixel(context, x0 + y, y0 - x, color);
    status |= sh1106_draw_pixel(context, x0 - y, y0 - x, color);
  }

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a filled circle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_circle(display_context_t *context,
                                  int16_t x0, int16_t y0,
                                  int16_t r, SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  status |= sh1106_draw_vline(context, x0, y0 - r, 2 * r + 1, color);
  status |= sh1106_fill_circle_helper(context, x0, y0, r, 3, 0, color);

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a round rectangle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_round_rectangle(display_context_t *context,
                                      int16_t x, int16_t y,
                                      int16_t w, int16_t h,
                                      int16_t r, SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  int16_t max_radius = ((w < h) ? w : h) / 2;   // 1/2 minor axis
  if (r > max_radius) {
    r = max_radius;
  }
  // smarter version
  // Top
  status |= sh1106_draw_hline(context, x + r, y, w - 2 * r, color);
  // Bottom
  status |= sh1106_draw_hline(context, x + r, y + h - 1, w - 2 * r, color);
  // Left
  status |= sh1106_draw_vline(context, x, y + r, h - 2 * r, color);
  // Right
  status |= sh1106_draw_vline(context, x + w - 1, y + r, h - 2 * r, color);

  // draw four corners
  status |= sh1106_draw_circle_helper(context, x + r, y + r, r, 1, color);
  status |= sh1106_draw_circle_helper(context, x + w - r - 1,
                                      y + r, r, 2, color);
  status |= sh1106_draw_circle_helper(context, x + w - r - 1,
                                      y + h - r - 1, r, 4, color);
  status |= sh1106_draw_circle_helper(context, x + r, y + h - r - 1,
                                      r, 8, color);

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Draw a fill round rectangle on SH1106.
 ******************************************************************************/
esp_err_t sh1106_draw_fill_round_rectangle(display_context_t *context,
                                           int16_t x, int16_t y,
                                           int16_t w, int16_t h,
                                           int16_t r, SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  int16_t max_radius = ((w < h) ? w : h) / 2;   // 1/2 minor axis
  if (r > max_radius) {
    r = max_radius;
  }
  // smarter version
  status |= sh1106_draw_fill_rectangle(context,
                                       x + r,
                                       y,
                                       w - 2 * r,
                                       h,
                                       color);
  // draw four corners
  status |= sh1106_fill_circle_helper(context,
                                      x + w - r - 1,
                                      y + r,
                                      r,
                                      1,
                                      h - 2 * r - 1,
                                      color);
  status |= sh1106_fill_circle_helper(context,
                                      x + r,
                                      y + r,
                                      r,
                                      2,
                                      h - 2 * r - 1,
                                      color);

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Set text font for OLED SH106.
 ******************************************************************************/
void sh1106_set_text_font(display_context_t *context, const gfx_font_t *font)
{
  context->font = (gfx_font_t *)font;
}

/***************************************************************************//**
 *  Draw a character on SH1106.
 ******************************************************************************/

// Standard ASCII 5x7 font
static const unsigned char font_5x7[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x5B, 0x4F, 0x5B, 0x3E, 0x3E, 0x6B,
  0x4F, 0x6B, 0x3E, 0x1C, 0x3E, 0x7C, 0x3E, 0x1C, 0x18, 0x3C, 0x7E, 0x3C,
  0x18, 0x1C, 0x57, 0x7D, 0x57, 0x1C, 0x1C, 0x5E, 0x7F, 0x5E, 0x1C, 0x00,
  0x18, 0x3C, 0x18, 0x00, 0xFF, 0xE7, 0xC3, 0xE7, 0xFF, 0x00, 0x18, 0x24,
  0x18, 0x00, 0xFF, 0xE7, 0xDB, 0xE7, 0xFF, 0x30, 0x48, 0x3A, 0x06, 0x0E,
  0x26, 0x29, 0x79, 0x29, 0x26, 0x40, 0x7F, 0x05, 0x05, 0x07, 0x40, 0x7F,
  0x05, 0x25, 0x3F, 0x5A, 0x3C, 0xE7, 0x3C, 0x5A, 0x7F, 0x3E, 0x1C, 0x1C,
  0x08, 0x08, 0x1C, 0x1C, 0x3E, 0x7F, 0x14, 0x22, 0x7F, 0x22, 0x14, 0x5F,
  0x5F, 0x00, 0x5F, 0x5F, 0x06, 0x09, 0x7F, 0x01, 0x7F, 0x00, 0x66, 0x89,
  0x95, 0x6A, 0x60, 0x60, 0x60, 0x60, 0x60, 0x94, 0xA2, 0xFF, 0xA2, 0x94,
  0x08, 0x04, 0x7E, 0x04, 0x08, 0x10, 0x20, 0x7E, 0x20, 0x10, 0x08, 0x08,
  0x2A, 0x1C, 0x08, 0x08, 0x1C, 0x2A, 0x08, 0x08, 0x1E, 0x10, 0x10, 0x10,
  0x10, 0x0C, 0x1E, 0x0C, 0x1E, 0x0C, 0x30, 0x38, 0x3E, 0x38, 0x30, 0x06,
  0x0E, 0x3E, 0x0E, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F,
  0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00, 0x14, 0x7F, 0x14, 0x7F, 0x14,
  0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62, 0x36, 0x49,
  0x56, 0x20, 0x50, 0x00, 0x08, 0x07, 0x03, 0x00, 0x00, 0x1C, 0x22, 0x41,
  0x00, 0x00, 0x41, 0x22, 0x1C, 0x00, 0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x08,
  0x08, 0x3E, 0x08, 0x08, 0x00, 0x80, 0x70, 0x30, 0x00, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x00, 0x00, 0x60, 0x60, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02,
  0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00, 0x72, 0x49,
  0x49, 0x49, 0x46, 0x21, 0x41, 0x49, 0x4D, 0x33, 0x18, 0x14, 0x12, 0x7F,
  0x10, 0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A, 0x49, 0x49, 0x31, 0x41,
  0x21, 0x11, 0x09, 0x07, 0x36, 0x49, 0x49, 0x49, 0x36, 0x46, 0x49, 0x49,
  0x29, 0x1E, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x40, 0x34, 0x00, 0x00,
  0x00, 0x08, 0x14, 0x22, 0x41, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x41,
  0x22, 0x14, 0x08, 0x02, 0x01, 0x59, 0x09, 0x06, 0x3E, 0x41, 0x5D, 0x59,
  0x4E, 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E,
  0x41, 0x41, 0x41, 0x22, 0x7F, 0x41, 0x41, 0x41, 0x3E, 0x7F, 0x49, 0x49,
  0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01, 0x3E, 0x41, 0x41, 0x51, 0x73,
  0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00, 0x20, 0x40,
  0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40,
  0x40, 0x7F, 0x02, 0x1C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E,
  0x41, 0x41, 0x41, 0x3E, 0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51,
  0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46, 0x26, 0x49, 0x49, 0x49, 0x32,
  0x03, 0x01, 0x7F, 0x01, 0x03, 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x1F, 0x20,
  0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14,
  0x63, 0x03, 0x04, 0x78, 0x04, 0x03, 0x61, 0x59, 0x49, 0x4D, 0x43, 0x00,
  0x7F, 0x41, 0x41, 0x41, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41,
  0x41, 0x7F, 0x04, 0x02, 0x01, 0x02, 0x04, 0x40, 0x40, 0x40, 0x40, 0x40,
  0x00, 0x03, 0x07, 0x08, 0x00, 0x20, 0x54, 0x54, 0x78, 0x40, 0x7F, 0x28,
  0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x28, 0x38, 0x44, 0x44, 0x28,
  0x7F, 0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x08, 0x7E, 0x09, 0x02, 0x18,
  0xA4, 0xA4, 0x9C, 0x78, 0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D,
  0x40, 0x00, 0x20, 0x40, 0x40, 0x3D, 0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,
  0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x78, 0x04, 0x78, 0x7C, 0x08,
  0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0xFC, 0x18, 0x24, 0x24,
  0x18, 0x18, 0x24, 0x24, 0x18, 0xFC, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x48,
  0x54, 0x54, 0x54, 0x24, 0x04, 0x04, 0x3F, 0x44, 0x24, 0x3C, 0x40, 0x40,
  0x20, 0x7C, 0x1C, 0x20, 0x40, 0x20, 0x1C, 0x3C, 0x40, 0x30, 0x40, 0x3C,
  0x44, 0x28, 0x10, 0x28, 0x44, 0x4C, 0x90, 0x90, 0x90, 0x7C, 0x44, 0x64,
  0x54, 0x4C, 0x44, 0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x77, 0x00,
  0x00, 0x00, 0x41, 0x36, 0x08, 0x00, 0x02, 0x01, 0x02, 0x04, 0x02, 0x3C,
  0x26, 0x23, 0x26, 0x3C, 0x1E, 0xA1, 0xA1, 0x61, 0x12, 0x3A, 0x40, 0x40,
  0x20, 0x7A, 0x38, 0x54, 0x54, 0x55, 0x59, 0x21, 0x55, 0x55, 0x79, 0x41,
  0x22, 0x54, 0x54, 0x78, 0x42,   // a-umlaut
  0x21, 0x55, 0x54, 0x78, 0x40, 0x20, 0x54, 0x55, 0x79, 0x40, 0x0C, 0x1E,
  0x52, 0x72, 0x12, 0x39, 0x55, 0x55, 0x55, 0x59, 0x39, 0x54, 0x54, 0x54,
  0x59, 0x39, 0x55, 0x54, 0x54, 0x58, 0x00, 0x00, 0x45, 0x7C, 0x41, 0x00,
  0x02, 0x45, 0x7D, 0x42, 0x00, 0x01, 0x45, 0x7C, 0x40, 0x7D, 0x12, 0x11,
  0x12, 0x7D,   // A-umlaut
  0xF0, 0x28, 0x25, 0x28, 0xF0, 0x7C, 0x54, 0x55, 0x45, 0x00, 0x20, 0x54,
  0x54, 0x7C, 0x54, 0x7C, 0x0A, 0x09, 0x7F, 0x49, 0x32, 0x49, 0x49, 0x49,
  0x32, 0x3A, 0x44, 0x44, 0x44, 0x3A,   // o-umlaut
  0x32, 0x4A, 0x48, 0x48, 0x30, 0x3A, 0x41, 0x41, 0x21, 0x7A, 0x3A, 0x42,
  0x40, 0x20, 0x78, 0x00, 0x9D, 0xA0, 0xA0, 0x7D, 0x3D, 0x42, 0x42, 0x42,
  0x3D,   // O-umlaut
  0x3D, 0x40, 0x40, 0x40, 0x3D, 0x3C, 0x24, 0xFF, 0x24, 0x24, 0x48, 0x7E,
  0x49, 0x43, 0x66, 0x2B, 0x2F, 0xFC, 0x2F, 0x2B, 0xFF, 0x09, 0x29, 0xF6,
  0x20, 0xC0, 0x88, 0x7E, 0x09, 0x03, 0x20, 0x54, 0x54, 0x79, 0x41, 0x00,
  0x00, 0x44, 0x7D, 0x41, 0x30, 0x48, 0x48, 0x4A, 0x32, 0x38, 0x40, 0x40,
  0x22, 0x7A, 0x00, 0x7A, 0x0A, 0x0A, 0x72, 0x7D, 0x0D, 0x19, 0x31, 0x7D,
  0x26, 0x29, 0x29, 0x2F, 0x28, 0x26, 0x29, 0x29, 0x29, 0x26, 0x30, 0x48,
  0x4D, 0x40, 0x20, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x38, 0x2F, 0x10, 0xC8, 0xAC, 0xBA, 0x2F, 0x10, 0x28, 0x34, 0xFA, 0x00,
  0x00, 0x7B, 0x00, 0x00, 0x08, 0x14, 0x2A, 0x14, 0x22, 0x22, 0x14, 0x2A,
  0x14, 0x08, 0x55, 0x00, 0x55, 0x00, 0x55,   // #176 (25% block) missing in old
                                              // code
  0xAA, 0x55, 0xAA, 0x55, 0xAA,               // 50% block
  0xFF, 0x55, 0xFF, 0x55, 0xFF,               // 75% block
  0x00, 0x00, 0x00, 0xFF, 0x00, 0x10, 0x10, 0x10, 0xFF, 0x00, 0x14, 0x14,
  0x14, 0xFF, 0x00, 0x10, 0x10, 0xFF, 0x00, 0xFF, 0x10, 0x10, 0xF0, 0x10,
  0xF0, 0x14, 0x14, 0x14, 0xFC, 0x00, 0x14, 0x14, 0xF7, 0x00, 0xFF, 0x00,
  0x00, 0xFF, 0x00, 0xFF, 0x14, 0x14, 0xF4, 0x04, 0xFC, 0x14, 0x14, 0x17,
  0x10, 0x1F, 0x10, 0x10, 0x1F, 0x10, 0x1F, 0x14, 0x14, 0x14, 0x1F, 0x00,
  0x10, 0x10, 0x10, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, 0x10,
  0x10, 0x1F, 0x10, 0x10, 0x10, 0x10, 0xF0, 0x10, 0x00, 0x00, 0x00, 0xFF,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xFF, 0x10, 0x00,
  0x00, 0x00, 0xFF, 0x14, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x1F,
  0x10, 0x17, 0x00, 0x00, 0xFC, 0x04, 0xF4, 0x14, 0x14, 0x17, 0x10, 0x17,
  0x14, 0x14, 0xF4, 0x04, 0xF4, 0x00, 0x00, 0xFF, 0x00, 0xF7, 0x14, 0x14,
  0x14, 0x14, 0x14, 0x14, 0x14, 0xF7, 0x00, 0xF7, 0x14, 0x14, 0x14, 0x17,
  0x14, 0x10, 0x10, 0x1F, 0x10, 0x1F, 0x14, 0x14, 0x14, 0xF4, 0x14, 0x10,
  0x10, 0xF0, 0x10, 0xF0, 0x00, 0x00, 0x1F, 0x10, 0x1F, 0x00, 0x00, 0x00,
  0x1F, 0x14, 0x00, 0x00, 0x00, 0xFC, 0x14, 0x00, 0x00, 0xF0, 0x10, 0xF0,
  0x10, 0x10, 0xFF, 0x10, 0xFF, 0x14, 0x14, 0x14, 0xFF, 0x14, 0x10, 0x10,
  0x10, 0x1F, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x10, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x38, 0x44, 0x44,
  0x38, 0x44, 0xFC, 0x4A, 0x4A, 0x4A, 0x34,   // sharp-s or beta
  0x7E, 0x02, 0x02, 0x06, 0x06, 0x02, 0x7E, 0x02, 0x7E, 0x02, 0x63, 0x55,
  0x49, 0x41, 0x63, 0x38, 0x44, 0x44, 0x3C, 0x04, 0x40, 0x7E, 0x20, 0x1E,
  0x20, 0x06, 0x02, 0x7E, 0x02, 0x02, 0x99, 0xA5, 0xE7, 0xA5, 0x99, 0x1C,
  0x2A, 0x49, 0x2A, 0x1C, 0x4C, 0x72, 0x01, 0x72, 0x4C, 0x30, 0x4A, 0x4D,
  0x4D, 0x30, 0x30, 0x48, 0x78, 0x48, 0x30, 0xBC, 0x62, 0x5A, 0x46, 0x3D,
  0x3E, 0x49, 0x49, 0x49, 0x00, 0x7E, 0x01, 0x01, 0x01, 0x7E, 0x2A, 0x2A,
  0x2A, 0x2A, 0x2A, 0x44, 0x44, 0x5F, 0x44, 0x44, 0x40, 0x51, 0x4A, 0x44,
  0x40, 0x40, 0x44, 0x4A, 0x51, 0x40, 0x00, 0x00, 0xFF, 0x01, 0x03, 0xE0,
  0x80, 0xFF, 0x00, 0x00, 0x08, 0x08, 0x6B, 0x6B, 0x08, 0x36, 0x12, 0x36,
  0x24, 0x36, 0x06, 0x0F, 0x09, 0x0F, 0x06, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x00, 0x10, 0x10, 0x00, 0x30, 0x40, 0xFF, 0x01, 0x01, 0x00, 0x1F,
  0x01, 0x01, 0x1E, 0x00, 0x19, 0x1D, 0x17, 0x12, 0x00, 0x3C, 0x3C, 0x3C,
  0x3C, 0x00, 0x00, 0x00, 0x00, 0x00   // #255 NBSP
};

// Draw char
esp_err_t sh1106_draw_char(display_context_t *context,
                           int16_t x, int16_t y,
                           unsigned char c,
                           SH1106_PIXEL_COLOR color, SH1106_PIXEL_COLOR bg,
                           uint8_t size_x, uint8_t size_y)
{
  esp_err_t status = ESP_OK;

  if (!context->font) {   // 'Classic' built-in font
    if ((x >= context->width)             // Clip right
        || (y >= context->height)         // Clip bottom
        || ((x + 6 * size_x - 1) < 0)     // Clip left
        || ((y + 8 * size_y - 1) < 0)) {  // Clip top
      return ESP_ERR_INVALID_ARG;
    }

    if (context->cp437 && (c >= 176)) {
      c++;       // Handle 'classic' charset behavior
    }
    for (int8_t i = 0; i < 5; i++) {     // Char bitmap = 5 columns
      uint8_t line = font_5x7[c * 5 + i];
      for (int8_t j = 0; j < 8; j++, line >>= 1) {
        if (line & 1) {
          if ((size_x == 1) && (size_y == 1)) {
            status |= sh1106_draw_pixel(context,
                                        x + i,
                                        y + j,
                                        color);
          } else {
            status |= sh1106_draw_fill_rectangle(context,
                                                 x + i * size_x,
                                                 y + j * size_y,
                                                 size_x, size_y,
                                                 color);
          }
        } else if (bg != color) {
          if ((size_x == 1) && (size_y == 1)) {
            status |= sh1106_draw_pixel(context, x + i, y + j, bg);
          } else {
            status |= sh1106_draw_fill_rectangle(context,
                                                 x + i * size_x,
                                                 y + j * size_y,
                                                 size_x, size_y,
                                                 bg);
          }
        }
      }
    }
    if (bg != color) {     // If opaque, draw vertical line for last column
      if ((size_x == 1) && (size_y == 1)) {
        sh1106_draw_vline(context, x + 5, y, 8, bg);
      } else {
        status |= sh1106_draw_fill_rectangle(context,
                                             x + 5 * size_x, y,
                                             size_x, 8 * size_y,
                                             bg);
      }
    }
  } else {   // Custom font
    c -= (uint8_t)context->font->first;
    gfx_glyph_t *glyph = &(context->font->glyph[c]);
    uint8_t *bitmap = context->font->bitmap;

    uint16_t bo = glyph->bitmap_offset;
    uint8_t w = glyph->width;
    uint8_t h = glyph->height;
    int8_t xo = glyph->x_offset;
    int8_t yo = glyph->y_offset;
    uint8_t xx, yy, bits = 0, bit = 0;
    int16_t xo16 = 0, yo16 = 0;

    if ((size_x > 1) || (size_y > 1)) {
      xo16 = xo;
      yo16 = yo;
    }
    for (yy = 0; yy < h; yy++) {
      for (xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if (bits & 0x80) {
          if ((size_x == 1) && (size_y == 1)) {
            status |= sh1106_draw_pixel(context,
                                        x + xo + xx,
                                        y + yo + yy,
                                        color);
          } else {
            status |= sh1106_draw_fill_rectangle(
              context,
              x + (xo16 + xx) * size_x,
              y + (yo16 + yy) * size_y,
              size_x, size_y,
              color);
          }
        }
        bits <<= 1;
      }
    }
  }
  return status;
}

/***************************************************************************//**
 *  Draw a character at the current position of the cursor.
 ******************************************************************************/
esp_err_t sh1106_write_char(display_context_t *context, char c)
{
  esp_err_t status = ESP_OK;

  if (!context->font) {                 // 'Classic' built-in font
    if (c == '\n') {                    // Newline?
      // Reset x to zero
      context->cursor_x = 0;
      // advance y one line
      context->cursor_y += context->textsize_y * 8;
    } else if (c != '\r') {             // Ignore carriage returns
      if (context->wrap                 // Off right?
          && ((context->cursor_x + context->textsize_x * 6)
              > context->width)) {
        // Reset x to zero
        context->cursor_x = 0;
        // advance y one line
        context->cursor_y += context->textsize_y * 8;
      }
      status = sh1106_draw_char(context,
                                context->cursor_x,
                                context->cursor_y,
                                c,
                                context->text_color,
                                context->bg_color,
                                context->textsize_x,
                                context->textsize_y);
      context->cursor_x += context->textsize_x * 6;       // Advance x one char
    }
  } else {   // Custom font
    if (c == '\n') {
      context->cursor_x = 0;
      context->cursor_y +=
        (int16_t)context->textsize_y * (uint8_t)context->font->y_advance;
    } else if (c != '\r') {
      uint8_t first = context->font->first;
      if ((c >= first) && (c <= (uint8_t)context->font->last)) {
        gfx_glyph_t *glyph = &context->font->glyph[c - first];
        uint8_t w = glyph->width;
        uint8_t h = glyph->height;
        if ((w > 0) && (h > 0)) {         // Is there an associated bitmap?
          int16_t xo = (int8_t)glyph->x_offset;           // sic
          if (context->wrap
              && ((context->cursor_x + context->textsize_x * (xo + w))
                  > context->width)) {
            context->cursor_x = 0;
            context->cursor_y += (int16_t)context->textsize_y
                                 * (uint8_t)context->font->y_advance;
          }
          status = sh1106_draw_char(context,
                                    context->cursor_x,
                                    context->cursor_y,
                                    c,
                                    context->text_color,
                                    context->bg_color,
                                    context->textsize_x,
                                    context->textsize_y);
        }
        context->cursor_x +=
          (uint8_t)glyph->x_advance * (int16_t)context->textsize_x;
      }
    }
  }
  return status;
}

/***************************************************************************//**
 *  Draw a string on SH1106 at the current position of the cursor.
 ******************************************************************************/
esp_err_t sh1106_write_string(display_context_t *context, const char *str,
                              int16_t x0, int16_t y0)
{
  esp_err_t status;

  context->cursor_x = x0;
  context->cursor_y = y0;

  /* Loops through the string and prints char for char */
  while (*str) {
    status = sh1106_write_char(context, *str);
    if (status != ESP_OK) {
      // Char could not be written
      return status;
    }
    // Next char
    str++;
  }
  return ESP_OK;
}

/***************************************************************************//**
 *  Draw a RAM-resident 1-bit image at the specified (x,y) position.
 ******************************************************************************/
esp_err_t sh1106_draw_bitmap(display_context_t *context,
                             int16_t x, int16_t y, uint8_t *bitmap,
                             int16_t w, int16_t h,
                             SH1106_PIXEL_COLOR color,
                             SH1106_PIXEL_COLOR bg)
{
  esp_err_t status = ESP_OK;

  int16_t byteWidth = (w + 7) / 8;   // Bitmap scanline pad = whole byte
  uint8_t b = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7) {
        b <<= 1;
      } else {
        b = bitmap[j * byteWidth + i / 8];
      }
      status |= sh1106_draw_pixel(context,
                                  x + i, y,
                                  (b & 0x80) ? color : bg);
    }
  }
  return status;
}

/***************************************************************************//**
 *  Set rotation for OLED SH1106.
 ******************************************************************************/
void sh1106_set_rotation(display_context_t *context, rotation_dir_t dir)
{
  context->rotation = dir;
}

/***************************************************************************//**
 *  Set rotation for OLED SH1106.
 ******************************************************************************/
esp_err_t sh1106_fill_screen(display_context_t *context,
                             SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  for (int i = 0; i < context->width; i++) {
    for (int j = 0; j < context->height; j++) {
      status |= sh1106_draw_pixel(context, i, j, color);
    }
  }

  if (status != ESP_OK) {
    return ESP_FAIL;
  } else {
    return ESP_OK;
  }
}

/***************************************************************************//**
 *  Update data from frame buffer to SH106.
 ******************************************************************************/
esp_err_t sh1106_update_display()
{
  esp_err_t status = ESP_OK;
  for (int i = 0; i < 8; i++) {
    uint8_t buffer[3];
    buffer[0] = SH1106_SETPAGEADDR + i;
    buffer[1] = 0x10 + (2 >> 4),
    buffer[2] = 2 & 0xF;
    status += sh1106_send_command(buffer, 3);
    status += sh1106_send_data(frame_buffer + 128 * i, 128);
  }
  return status;
}

// -----------------------------------------------------------------------------
//                         Local functions definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Send command to SH1106.
 ******************************************************************************/
static esp_err_t sh1106_send_command(uint8_t *command, uint8_t cmd_len)
{
  esp_err_t status;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(
                    cmd,
                    (SH1106_DEV_ADDR << 1) | I2C_MASTER_WRITE,
                    I2C_ACK_EN));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x00, I2C_ACK_EN));
  ESP_ERROR_CHECK(i2c_master_write(cmd, command, cmd_len, I2C_ACK_EN));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));
  status = i2c_master_cmd_begin(sh1106_i2c_port,
                                cmd,
                                pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  return status;
}

/***************************************************************************//**
 *  Send command to SH1106.
 ******************************************************************************/
static esp_err_t sh1106_send_data(uint8_t *data, uint8_t len)
{
  esp_err_t status;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(
                    cmd,
                    (SH1106_DEV_ADDR << 1) | I2C_MASTER_WRITE,
                    I2C_ACK_EN));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x40, I2C_ACK_EN));
  ESP_ERROR_CHECK(i2c_master_write(cmd, data, len, I2C_ACK_EN));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));
  status = i2c_master_cmd_begin(sh1106_i2c_port,
                                cmd,
                                pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  return status;
}

/***************************************************************************//**
 *  Quarter-circle drawer with fill, used for circles and roundrects.
 ******************************************************************************/
static esp_err_t sh1106_fill_circle_helper(display_context_t *context,
                                           int16_t x0, int16_t y0, int16_t r,
                                           uint8_t corners, int16_t delta,
                                           SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (x < (y + 1)) {
      if (corners & 1) {
        status |= sh1106_draw_vline(
          context, x0 + x, y0 - y, 2 * y + delta, color);
      }
      if (corners & 2) {
        status |= sh1106_draw_vline(
          context, x0 - x, y0 - y, 2 * y + delta, color);
      }
    }
    if (y != py) {
      if (corners & 1) {
        status |= sh1106_draw_vline(
          context, x0 + py, y0 - px, 2 * px + delta, color);
      }
      if (corners & 2) {
        status |= sh1106_draw_vline(
          context, x0 - py, y0 - px, 2 * px + delta, color);
      }
      py = y;
    }
    px = x;
  }
  return status;
}

/***************************************************************************//**
 *  Quarter-circle drawer, used to do circles and roundrects.
 ******************************************************************************/
static esp_err_t sh1106_draw_circle_helper(display_context_t *context,
                                           int16_t x0, int16_t y0, int16_t r,
                                           uint8_t cornername,
                                           SH1106_PIXEL_COLOR color)
{
  esp_err_t status = ESP_OK;

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      status |= sh1106_draw_pixel(context, x0 + x, y0 + y, color);
      status |= sh1106_draw_pixel(context, x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      status |= sh1106_draw_pixel(context, x0 + x, y0 - y, color);
      status |= sh1106_draw_pixel(context, x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      status |= sh1106_draw_pixel(context, x0 - y, y0 + x, color);
      status |= sh1106_draw_pixel(context, x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      status |= sh1106_draw_pixel(context, x0 - y, y0 - x, color);
      status |= sh1106_draw_pixel(context, x0 - x, y0 - y, color);
    }
  }
  return status;
}
