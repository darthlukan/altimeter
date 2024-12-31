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
#include "ssd1306_i2c.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "font.h"


void calc_render_area_buflen(struct render_area *area) {
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        SSD1306_send_cmd(buf[i]);
}

void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t *temp_buf = malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(i2c_default, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

void SSD1306_init() {
    uint8_t cmds[] = {
        SSD1306_SET_DISP,
        SSD1306_SET_MEM_MODE,
        0x00,
        SSD1306_SET_DISP_START_LINE,
        SSD1306_SET_SEG_REMAP | 0x01,
        SSD1306_SET_MUX_RATIO,
        SSD1306_HEIGHT - 1,
        SSD1306_SET_COM_OUT_DIR | 0x08,
        SSD1306_SET_DISP_OFFSET,
        0x00,
        SSD1306_SET_COM_PIN_CFG,
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        SSD1306_SET_DISP_CLK_DIV,
        0x80,
        SSD1306_SET_PRECHARGE,
        0xF1,
        SSD1306_SET_VCOM_DESEL,
        0x30,
        SSD1306_SET_CONTRAST,
        0xFF,
        SSD1306_SET_ENTIRE_ON,
        SSD1306_SET_NORM_DISP,
        SSD1306_SET_CHARGE_PUMP,
        0x14,
        SSD1306_SET_SCROLL | 0x00,
        SSD1306_SET_DISP | 0x01,
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void render(uint8_t *buf, struct render_area *area) {
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

static inline int GetFontIndex(uint8_t ch) {
    if (ch >= ' ' && ch <= 127) {
        return  ch - ' ';
    }
    else return  0;
}

void WriteChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    y = y/8;

    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (int i=0;i<8;i++) {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str) {
        WriteChar(buf, x, y, *str++);
        x+=8;
    }
}
