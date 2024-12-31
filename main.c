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
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "font.h"
#include "ssd1306_i2c.h"
#include "pico_sensor_lib.h"


#define DELAY_MS 1000

/*
 * Pass sensor data from our reader
 * into a queue for reading by a function
 * on another core.
 */
typedef struct queue_response_t {
    float temp;
    float pressure;
    float humidity;
    int status;
} queue_response_t;

/*
 * Pass a function pointer and sensor context
 * from one core to the other.
 */
typedef struct queue_entry_t {
    queue_response_t (*func)(void*);
    void *ctx;
} queue_entry_t;

// to_core1_queue is how we'll send functions and their args to the dispatcher on core1
queue_t to_core1_queue;
// from_core1_queue is how we'll get our pressure and temp data from functions running on core1
queue_t from_core1_queue;


/*
 * Infineon DPS310 Digital Pressure Sensor in I2C
 *
 * NOTE: Same pins as the SSD1306 OLED because i2c lets you
 * daisy-chain.
 * 
 *
 * Pins:
 * GPIO_2  (pin 4) i2c_sda Data In/Out -> SDI/SDI on DPS310
 * GPIO_3  (pin 5) i2c_scl Clock       -> SCK on DPS310
 *
 * 3V3(OUT)(pin 36)                    -> VCC/VIN on DPS310
 * GND     (pin 38)                    -> GND on DPS310
 */

const uint dps310_i2c_sdi  = 4;
const uint dps310_i2c_scl  = 5;
// If we were running two DPS310's, we'd need to pull-down SDO/DDO
// to change this to 0x76 for the second unit.
const uint8_t dps310_addr = 0x77;

/*
 * Meant to be called by `core1_entry` function.
 *
 */
queue_response_t read_dps310_data(void *ctx) {
    sleep_ms(DELAY_MS);
    queue_response_t response;
    float temp, pressure, humidity;
    int res = i2c_read_measurement(ctx, &temp, &pressure, &humidity);
    switch(res) {
        case -1:
            printf("Sensor MEAS status was not ready\n");
            // Extend the timeout to 6000 milliseconds
            sleep_ms(DELAY_MS * 6);
            break;
        case -2:
            printf("Sensor TMP status was not ready\n");
            break;
        case -4:
            printf("Sensor PRS status was not ready\n");
            break;
        default:
            response.temp = temp;
            response.pressure = pressure;
            response.humidity = humidity;
            printf("Got measurements!\n");
            printf("Temp: %.2f\n", temp);
            printf("Baro: %.2f\n", pressure);
            printf("Hum: %.2f\n", humidity);
            break;
    }

    response.status = res;
    printf("read_dps310_data response status code: %d\n", response.status);
    return response;
}

void core1_entry() {
    while(true) {
        sleep_ms(DELAY_MS);
        queue_entry_t entry;
        queue_remove_blocking(&to_core1_queue, &entry);
        queue_response_t response = entry.func(entry.ctx);
        if (response.status == 0) {
            printf("Have response, sending to core0 via our queue\n");
            queue_add_blocking(&from_core1_queue, &response);
            sleep_ms(DELAY_MS);
        } else {
            // Cheap debugging
            printf("Respons status is %d\n", response.status);
        }
    }
}

int main() {
    stdio_init_all();

    // SSD1306 Init
    // TODO: This assumes we'll always put the screen on i2c0. A more robust
    // way would be comparing what is on the bus via known device address, 
    // and configure it that way.
    i2c_init(i2c_default, 100000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    SSD1306_init();
    // End SSD1306 Init

    // DPS310 Init
    void *ctx = NULL;
    uint8_t dps310_sensor_type = get_i2c_sensor_type("DPS310");

    int res = i2c_init_sensor(dps310_sensor_type, i2c_default, dps310_addr, &ctx);
    // More cheap debugging
    switch(res) {
        case -1:
            printf("Sensor type is invalid\n");
            break;
        case -2:
            printf("Sensor addr already in use\n");
            break;
        case -3:
            printf("Sensor failed to init in its own method\n");
            break;
        case 0:
            printf("Sensor initialized!\n");
            break;
        default:
            printf("Unknown sensor init state!\n");
            break;
    }

    int delay = i2c_start_measurement(ctx);
    if (delay < 0) {
        printf("failed to get measurement delay\n");
    }
    // End DPS310 Init
    printf("DPS310 init complete\n");

    // Rendering init
    struct render_area frame_area = {
        start_col: 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);
    
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    sleep_ms(DELAY_MS);
    // End Rendering init
    printf("Rendering init complete\n");

    // Init queues
    queue_init(&to_core1_queue, sizeof(queue_entry_t), 2);
    queue_init(&from_core1_queue, sizeof(queue_response_t), 2);
    // End queues init
    printf("Queues init complete\n");

    // Init core1 with `read_dps310_data` func
    multicore_launch_core1(core1_entry);
    printf("core1 started\n");
    queue_entry_t entry = {.func = read_dps310_data, .ctx = ctx};
    queue_add_blocking(&to_core1_queue, &entry);
    // End init core1
    printf("core1 init complete\n");

    char baro[16];
    char temp[16];
    char hum[16];
    char waiting[16];

    while(true) {
        sleep_ms(100);
        printf("main loop started\n");

        if (!queue_is_empty(&from_core1_queue)) {
            printf("from_core1_queue is not empty\n");
            memset(waiting, 0, 16);

            queue_response_t sensor_response;

            queue_remove_blocking(&from_core1_queue, &sensor_response);
            sprintf(baro, "Baro: %.2finHg", sensor_response.pressure*0.000296134);
            sprintf(temp, "Temp: %.2fF", sensor_response.temp*1.8+32);
            sprintf(hum, "Hum: %.2f", sensor_response.humidity);

            printf("Baro: %.2f\n", sensor_response.pressure*0.000296134);
            printf("Temp: %.2fF\n", sensor_response.temp*1.8+32);
            printf("Hum: %.2f\n", sensor_response.humidity);

            // Clear the screen buffer before we write new content
            memset(buf, 0, SSD1306_BUF_LEN);

            // Empty string reserves the first line
            WriteString(buf, 0, 0, " ");
            WriteString(buf, 0, 10, baro);
            WriteString(buf, 0, 20, temp);
            WriteString(buf, 0, 30, hum);
        } else {
            printf("from_core1_queue is empty, waiting on sensor\n");
            // Sleep a second to give the sensor a chance to catch up.
            // Then call it.
            sleep_ms(DELAY_MS);
            // Request core1 fire off our measurement read so we can get some data
            queue_add_blocking(&to_core1_queue, &entry);
            printf("added to_core1_queue after wait because we were previously empty.\n");
            sprintf(waiting, "Sensor fault...");
            WriteString(buf, 0, 0, waiting);
        }

        // Render the buffer to the screen
        render(buf, &frame_area);
        // I love my cheap debugging <3
        printf("main loop end\n");
    }
    // Useless return
    return 0;
}
