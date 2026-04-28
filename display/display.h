#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#define DISPLAY_X_OFFSET_LOWER 0u
#define DISPLAY_X_OFFSET_UPPER 0u
#define DISPLAY_HEIGHT         64u
#define DISPLAY_WIDTH          128u
#define DISPLAY_BUFFER_SIZE    (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8u)

typedef enum
{
    COLOR_BLACK = 0x00,  // Black color, no pixel
    COLOR_WHITE = 0x01   // Pixel is set. Color depends on OLED

} display_color_t;

typedef enum
{
    DISPLAY_OK  = 0x00,
    DISPLAY_ERR = 0x01  // Generic error.

} display_error_t;

typedef struct
{
    uint8_t x;
    uint8_t y;

} display_vertex_t;

typedef struct
{
    uint8_t current_x;
    uint8_t current_y;
    uint8_t initialized;
    uint8_t display_on;

} display_t;

typedef struct
{
    const uint8_t         width;      /**< Font width in pixels */
    const uint8_t         height;     /**< Font height in pixels */
    const uint16_t* const data;       /**< Pointer to font data array */
    const uint8_t* const  char_width; /**< Proportional character width in pixels (NULL for monospaced) */

} display_font_t;

typedef void (*display_write_data_cb_t) (uint8_t mem_addr, uint8_t* buffer, size_t buffer_size);

void display_init(void);
void display_set_cb(display_write_data_cb_t cb);
void display_fill(display_color_t color);
void display_update(void);
void display_draw_pixel(uint8_t x, uint8_t y, display_color_t color);
char display_write_char(char ch, display_font_t font, display_color_t color);
char display_write_string(char* str, display_font_t font, display_color_t color);
void display_set_cursor(uint8_t x, uint8_t y);
void display_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color);
void display_draw_arc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep,
                      display_color_t color);
void display_draw_arc_with_radius(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep,
                                  display_color_t color);
void display_draw_circle(uint8_t x, uint8_t y, uint8_t radius, display_color_t color);
void display_fill_circle(uint8_t x, uint8_t y, uint8_t radius, display_color_t color);
void display_draw_polyline(const display_vertex_t* vertices, uint16_t size, display_color_t color);
void display_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color);
void display_fill_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color);

/**
 * @brief Invert color of pixels in rectangle (include border)
 *
 * @param x1 X Coordinate of top left corner
 * @param y1 Y Coordinate of top left corner
 * @param x2 X Coordinate of bottom right corner
 * @param y2 Y Coordinate of bottom right corner
 * @return SSD1306_Error_t status
 */
display_error_t display_invert_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

void display_draw_bitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h,
                         display_color_t color);

/**
 * @brief Sets the contrast of the display.
 * @param[in] value contrast to set.
 * @note Contrast increases as the value increases.
 * @note RESET = 7Fh.
 */
void display_set_contrast(const uint8_t value);

/**
 * @brief Set Display ON/OFF.
 * @param[in] on 0 for OFF, any for ON.
 */
void display_set_on(const uint8_t on);

/**
 * @brief Reads DisplayOn state.
 * @return  0: OFF.
 *          1: ON.
 */
uint8_t display_get_on(void);

void display_write_cmd(uint8_t cmd);

void display_write_data(uint8_t* buffer, size_t buff_size);

display_error_t display_fill_buffer(uint8_t* buf, uint32_t len);

#endif  // DISPLAY_H