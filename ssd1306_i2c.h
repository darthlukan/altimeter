/*
    Altimeter. An altimeter implemented in C and meant to run on the
    Raspberry Pi Pico microcontroller.
    Copyright (C) 2024 Brian Tomlinson 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef SSD1306_I2C_H_INCLUDED
#define SSD1306_I2C_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
/*
 * The following was taken from the excellent writeup 
 * at https://zhonguncle.github.io/blogs/2a5e3991b4968e941a82aa12b3e2a9fa.html
 * and the code provided here https://github.com/ZhongUncle/pico-temp-hum-oled
 * by ZhongUncle, who made it really easy to understand why this works for the
 * SSD1306 128x64 0.96" OLED.
 */
#define SSD1306_HEIGHT              64
#define SSD1306_WIDTH               128

#define SSD1306_I2C_ADDR            _u(0x3C)

#define SSD1306_I2C_CLK             1000000

#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)

#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)

#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int buflen;
};

void calc_render_area_buflen(struct render_area *area);

void SSD1306_send_cmd(uint8_t cmd);

void SSD1306_send_cmd_list(uint8_t *buf, int num);

void SSD1306_send_buf(uint8_t buf[], int buflen);

void SSD1306_init(void);

void render(uint8_t *buf, struct render_area *area);

static inline int GetFontIndex(uint8_t ch);

void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch);

void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str);

#endif
