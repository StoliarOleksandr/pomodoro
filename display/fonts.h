#ifndef __SSD1306_FONTS_H__
#define __SSD1306_FONTS_H__

#include "display.h"

#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

#define SSD1306_INCLUDE_FONT_16x24

#define SSD1306_INCLUDE_FONT_16x15


#ifdef SSD1306_INCLUDE_FONT_6x8
extern const display_font_t font_6x8;
#endif
#ifdef SSD1306_INCLUDE_FONT_7x10
extern const display_font_t font_7x10;
#endif
#ifdef SSD1306_INCLUDE_FONT_11x18
extern const display_font_t font_11x18;
#endif
#ifdef SSD1306_INCLUDE_FONT_16x26
extern const display_font_t font_16x26;
#endif
#ifdef SSD1306_INCLUDE_FONT_16x24
extern const display_font_t font_16x24;
#endif
#ifdef SSD1306_INCLUDE_FONT_16x15
/** Generated Roboto Thin 15
 * @copyright Google https://github.com/googlefonts/roboto
 * @license This font is licensed under the Apache License, Version 2.0.
 */
extern const display_font_t font_16x15;
#endif

#endif  // __SSD1306_FONTS_H__