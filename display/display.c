#include "display.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint8_t   display_buffer[DISPLAY_BUFFER_SIZE];
static display_t display;


/* Convert Degrees to Radians */
static float display_deg_to_rad(float par_deg)
{
    return par_deg * (3.14f / 180.0f);
}

/* Normalize degree to [0;360] */
static uint16_t display_normalize_to_0_360(uint16_t par_deg)
{
    uint16_t loc_angle;
    if(par_deg <= 360)
    {
        loc_angle = par_deg;
    }
    else
    {
        loc_angle = par_deg % 360;
        loc_angle = (loc_angle ? loc_angle : 360);
    }
    return loc_angle;
}


void display_write_cmd(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&DISPLAY_I2C_PORT, DISPLAY_I2C_ADDR, 0x00, 1, &cmd, 1, HAL_MAX_DELAY);
}

void display_write_data(uint8_t* buffer, size_t buff_size)
{
    HAL_I2C_Mem_Write(&DISPLAY_I2C_PORT, DISPLAY_I2C_ADDR, 0x40, 1, buffer, buff_size, HAL_MAX_DELAY);
}

display_error_t displat_fill_buffer(uint8_t* buf, uint32_t len)
{
    display_error_t ret = DISPLAY_ERR;
    if(len <= DISPLAY_BUFFER_SIZE)
    {
        memcpy(display_buffer, buf, len);
        ret = DISPLAY_OK;
    }
    return ret;
}

/* Initialize the oled screen */
void display_init(void)
{
    // Reset OLED
    // Wait for the screen to boot
    HAL_Delay(100);

    // Init OLED
    display_set_on(0);  // display off

    display_write_cmd(0x20);  // Set Memory Addressing Mode
    display_write_cmd(0x00);  // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                              // 10b,Page Addressing Mode (RESET); 11b,Invalid

    display_write_cmd(0xB0);  // Set Page Start Address for Page Addressing Mode,0-7

    display_write_cmd(0xC8);  // Set COM Output Scan Direction

    display_write_cmd(0x00);  //---set low column address
    display_write_cmd(0x10);  //---set high column address

    display_write_cmd(0x40);  //--set start line address - CHECK

    display_set_contrast(0xFF);

    display_write_cmd(0xA1);  //--set segment re-map 0 to 127 - CHECK

    display_write_cmd(0xA6);  //--set normal color

    display_write_cmd(0xA8);  //--set multiplex ratio(1 to 64) - CHECK

    display_write_cmd(0x3F);  //

    display_write_cmd(0xA4);  // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    display_write_cmd(0xD3);  //-set display offset - CHECK
    display_write_cmd(0x00);  //-not offset

    display_write_cmd(0xD5);  //--set display clock divide ratio/oscillator frequency
    display_write_cmd(0xF0);  //--set divide ratio

    display_write_cmd(0xD9);  //--set pre-charge period
    display_write_cmd(0x22);  //

    display_write_cmd(0xDA);  //--set com pins hardware configuration - CHECK
    display_write_cmd(0x12);


    display_write_cmd(0xDB);  //--set vcomh
    display_write_cmd(0x20);  // 0x20,0.77xVcc

    display_write_cmd(0x8D);  //--set DC-DC enable
    display_write_cmd(0x14);  //
    display_set_on(1);        //--turn on display panel

    // Clear screen
    display_fill(COLOR_BLACK);

    // Flush buffer to screen
    display_update();

    // Set default values for screen object
    display.current_x = 0;
    display.current_y = 0;

    display.initialized = 1;
}

/* Fill the whole screen with the given color */
void display_fill(display_color_t color)
{
    memset(display_buffer, (color == COLOR_BLACK) ? 0x00 : 0xFF, sizeof(display_buffer));
}

/* Write the screenbuffer with changed to the screen */
void display_update(void)
{
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages

    for(uint8_t i = 0; i < DISPLAY_HEIGHT / 8; i++)
    {
        display_write_cmd(0xB0 + i);  // Set the current RAM page address.
        display_write_cmd(0x00 + DISPLAY_X_OFFSET_LOWER);
        display_write_cmd(0x10 + DISPLAY_X_OFFSET_UPPER);
        display_write_data(&display_buffer[DISPLAY_WIDTH * i], DISPLAY_WIDTH);
    }
}

/*
 * Draw one pixel in the screenbuffer
 * X => X Coordinate
 * Y => Y Coordinate
 * color => Pixel color
 */
void display_draw_pixel(uint8_t x, uint8_t y, display_color_t color)
{
    if(x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
        // Don't write outside the buffer
        return;

    // Draw in the right color
    if(color == COLOR_WHITE)
        display_buffer[x + (y / 8) * DISPLAY_WIDTH] |= 1 << (y % 8);
    else
        display_buffer[x + (y / 8) * DISPLAY_WIDTH] &= ~(1 << (y % 8));
}

/*
 * Draw 1 char to the screen buffer
 * ch       => char om weg te schrijven
 * Font     => Font waarmee we gaan schrijven
 * color    => Black or White
 */
char display_write_char(char ch, display_font_t font, display_color_t color)
{
    uint32_t i, b, j;

    // Check if character is valid
    if(ch < 32 || ch > 126)
        return 0;

    // Char width is not equal to font width for proportional font
    const uint8_t char_width = font.char_width ? font.char_width[ch - 32] : font.width;
    // Check remaining space on current line
    if(DISPLAY_WIDTH < (display.current_x + char_width) || DISPLAY_HEIGHT < (display.current_y + font.height))
        // Not enough space on current line
        return 0;

    // Use the font to write
    for(i = 0; i < font.height; i++)
    {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < char_width; j++)
        {
            if((b << j) & 0x8000)
                display_draw_pixel(display.current_x + j, (display.current_y + i), (display_color_t)color);
            else
                display_draw_pixel(display.current_x + j, (display.current_y + i), (display_color_t)! color);
        }
    }

    // The current space is now taken
    display.current_x += char_width;

    // Return written char for validation
    return ch;
}

/* Write full string to screenbuffer */
char display_write_string(char* str, display_font_t Font, display_color_t color)
{
    while(*str)
    {
        if(display_write_char(*str, Font, color) != *str)
        {
            // Char could not be written
            return *str;
        }
        str++;
    }

    // Everything ok
    return *str;
}

/* Position the cursor */
void display_set_cursor(uint8_t x, uint8_t y)
{
    display.current_x = x;
    display.current_y = y;
}

/* Draw line by Bresenhem's algorithm */
void display_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color)
{
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX  = ((x1 < x2) ? 1 : -1);
    int32_t signY  = ((y1 < y2) ? 1 : -1);
    int32_t error  = deltaX - deltaY;
    int32_t error2;

    display_draw_pixel(x2, y2, color);

    while((x1 != x2) || (y1 != y2))
    {
        display_draw_pixel(x1, y1, color);
        error2 = error * 2;
        if(error2 > -deltaY)
        {
            error -= deltaY;
            x1 += signX;
        }

        if(error2 < deltaX)
        {
            error += deltaX;
            y1 += signY;
        }
    }
    return;
}

/* Draw polyline */
void display_draw_polyline(const display_vertex_t* par_vertex, uint16_t par_size, display_color_t color)
{
    uint16_t i;
    if(par_vertex == NULL)
    {
        return;
    }

    for(i = 1; i < par_size; i++)
    {
        display_draw_line(par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
    }

    return;
}


/*
 * DrawArc. Draw angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle in degree
 * sweep in degree
 */
void display_draw_arc(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, display_color_t color)
{
    static const uint8_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
    float                approx_degree;
    uint32_t             approx_segments;
    uint8_t              xp1, xp2;
    uint8_t              yp1, yp2;
    uint32_t             count;
    uint32_t             loc_sweep;
    float                rad;

    loc_sweep = display_normalize_to_0_360(sweep);

    count           = (display_normalize_to_0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree   = loc_sweep / (float)approx_segments;
    while(count < approx_segments)
    {
        rad = display_deg_to_rad(count * approx_degree);
        xp1 = x + (int8_t)(sinf(rad) * radius);
        yp1 = y + (int8_t)(cosf(rad) * radius);
        count++;
        if(count != approx_segments)
        {
            rad = display_deg_to_rad(count * approx_degree);
        }
        else
        {
            rad = display_deg_to_rad(loc_sweep);
        }
        xp2 = x + (int8_t)(sinf(rad) * radius);
        yp2 = y + (int8_t)(cosf(rad) * radius);
        display_draw_line(xp1, yp1, xp2, yp2, color);
    }

    return;
}

/*
 * Draw arc with radius line
 * Angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle: start angle in degree
 * sweep: finish angle in degree
 */
void display_draw_arc_with_radius(uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep,
                                  display_color_t color)
{
    const uint32_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
    float          approx_degree;
    uint32_t       approx_segments;
    uint8_t        xp1;
    uint8_t        xp2 = 0;
    uint8_t        yp1;
    uint8_t        yp2 = 0;
    uint32_t       count;
    uint32_t       loc_sweep;
    float          rad;

    loc_sweep = display_normalize_to_0_360(sweep);

    count           = (display_normalize_to_0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
    approx_degree   = loc_sweep / (float)approx_segments;

    rad                   = display_deg_to_rad(count * approx_degree);
    uint8_t first_point_x = x + (int8_t)(sinf(rad) * radius);
    uint8_t first_point_y = y + (int8_t)(cosf(rad) * radius);
    while(count < approx_segments)
    {
        rad = display_deg_to_rad(count * approx_degree);
        xp1 = x + (int8_t)(sinf(rad) * radius);
        yp1 = y + (int8_t)(cosf(rad) * radius);
        count++;
        if(count != approx_segments)
        {
            rad = display_deg_to_rad(count * approx_degree);
        }
        else
        {
            rad = display_deg_to_rad(loc_sweep);
        }
        xp2 = x + (int8_t)(sinf(rad) * radius);
        yp2 = y + (int8_t)(cosf(rad) * radius);
        display_draw_line(xp1, yp1, xp2, yp2, color);
    }

    // Radius line
    display_draw_line(x, y, first_point_x, first_point_y, color);
    display_draw_line(x, y, xp2, yp2, color);
    return;
}

/* Draw circle by Bresenhem's algorithm */
void display_draw_circle(uint8_t par_x, uint8_t par_y, uint8_t par_r, display_color_t par_color)
{
    int32_t x   = -par_r;
    int32_t y   = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if(par_x >= DISPLAY_WIDTH || par_y >= DISPLAY_HEIGHT)
    {
        return;
    }

    do
    {
        display_draw_pixel(par_x - x, par_y + y, par_color);
        display_draw_pixel(par_x + x, par_y + y, par_color);
        display_draw_pixel(par_x + x, par_y - y, par_color);
        display_draw_pixel(par_x - x, par_y - y, par_color);
        e2 = err;

        if(e2 <= y)
        {
            y++;
            err = err + (y * 2 + 1);
            if(-x == y && e2 <= x)
            {
                e2 = 0;
            }
        }

        if(e2 > x)
        {
            x++;
            err = err + (x * 2 + 1);
        }
    } while(x <= 0);

    return;
}

/* Draw filled circle. Pixel positions calculated using Bresenham's algorithm */
void display_fill_circle(uint8_t par_x, uint8_t par_y, uint8_t par_r, display_color_t par_color)
{
    int32_t x   = -par_r;
    int32_t y   = 0;
    int32_t err = 2 - 2 * par_r;
    int32_t e2;

    if(par_x >= DISPLAY_WIDTH || par_y >= DISPLAY_HEIGHT)
    {
        return;
    }

    do
    {
        for(uint8_t _y = (par_y + y); _y >= (par_y - y); _y--)
        {
            for(uint8_t _x = (par_x - x); _x >= (par_x + x); _x--)
            {
                display_draw_pixel(_x, _y, par_color);
            }
        }

        e2 = err;
        if(e2 <= y)
        {
            y++;
            err = err + (y * 2 + 1);
            if(-x == y && e2 <= x)
            {
                e2 = 0;
            }
        }

        if(e2 > x)
        {
            x++;
            err = err + (x * 2 + 1);
        }
    } while(x <= 0);

    return;
}

/* Draw a rectangle */
void display_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color)
{
    display_draw_line(x1, y1, x2, y1, color);
    display_draw_line(x2, y1, x2, y2, color);
    display_draw_line(x2, y2, x1, y2, color);
    display_draw_line(x1, y2, x1, y1, color);

    return;
}

/* Draw a filled rectangle */
void display_fill_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, display_color_t color)
{
    uint8_t x_start = ((x1 <= x2) ? x1 : x2);
    uint8_t x_end   = ((x1 <= x2) ? x2 : x1);
    uint8_t y_start = ((y1 <= y2) ? y1 : y2);
    uint8_t y_end   = ((y1 <= y2) ? y2 : y1);

    for(uint8_t y = y_start; (y <= y_end) && (y < DISPLAY_HEIGHT); y++)
    {
        for(uint8_t x = x_start; (x <= x_end) && (x < DISPLAY_WIDTH); x++)
        {
            display_draw_pixel(x, y, color);
        }
    }
    return;
}

display_error_t display_invert_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    if((x2 >= DISPLAY_WIDTH) || (y2 >= DISPLAY_HEIGHT))
    {
        return DISPLAY_ERR;
    }
    if((x1 > x2) || (y1 > y2))
    {
        return DISPLAY_ERR;
    }
    uint32_t i;
    if((y1 / 8) != (y2 / 8))
    {
        /* if rectangle doesn't lie on one 8px row */
        for(uint32_t x = x1; x <= x2; x++)
        {
            i = x + (y1 / 8) * DISPLAY_WIDTH;
            display_buffer[i] ^= 0xFF << (y1 % 8);
            i += DISPLAY_WIDTH;
            for(; i < x + (y2 / 8) * DISPLAY_WIDTH; i += DISPLAY_WIDTH)
            {
                display_buffer[i] ^= 0xFF;
            }
            display_buffer[i] ^= 0xFF >> (7 - (y2 % 8));
        }
    }
    else
    {
        /* if rectangle lies on one 8px row */
        const uint8_t mask = (0xFF << (y1 % 8)) & (0xFF >> (7 - (y2 % 8)));
        for(i = x1 + (y1 / 8) * DISPLAY_WIDTH; i <= (uint32_t)x2 + (y2 / 8) * DISPLAY_WIDTH; i++)
        {
            display_buffer[i] ^= mask;
        }
    }
    return DISPLAY_OK;
}

/* Draw a bitmap */
void display_draw_bitmap(uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, display_color_t color)
{
    int16_t byte_width = (w + 7) / 8;  // Bitmap scanline pad = whole byte
    uint8_t byte       = 0;

    if(x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
        return;

    for(uint8_t j = 0; j < h; j++, y++)
    {
        for(uint8_t i = 0; i < w; i++)
        {
            if(i & 7)
            {
                byte <<= 1;
            }
            else
            {
                byte = (*(const unsigned char*)(&bitmap[j * byte_width + i / 8]));
            }

            if(byte & 0x80)
            {
                display_draw_pixel(x + i, y, color);
            }
        }
    }
    return;
}

void display_set_contrast(const uint8_t value)
{
    const uint8_t set_contrast_control_register = 0x81;
    display_write_cmd(set_contrast_control_register);
    display_write_cmd(value);
}

void display_set_on(const uint8_t on)
{
    uint8_t value;
    if(on)
    {
        value              = 0xAF;  // Display on
        display.display_on = 1;
    }
    else
    {
        value              = 0xAE;  // Display off
        display.display_on = 0;
    }
    display_write_cmd(value);
}

uint8_t display_get_on(void)
{
    return display.display_on;
}
