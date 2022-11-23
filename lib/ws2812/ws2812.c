#include "ws2812.h"
#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_resources.h>
#include <assert.h>
#include <string.h>

void ws2812_init() {
    furi_hal_spi_bus_handle_init(WS2812_HANDLE);
    furi_hal_spi_acquire(WS2812_HANDLE);
    furi_hal_gpio_init(WS2812_CE_PIN, GpioModeOutputPushPull, GpioPullUp, GpioSpeedVeryHigh);
    furi_hal_gpio_write(WS2812_CE_PIN, false);
}

void ws2812_deinit() {
    furi_hal_spi_release(WS2812_HANDLE);
    furi_hal_spi_bus_handle_deinit(WS2812_HANDLE);
    furi_hal_gpio_write(WS2812_CE_PIN, false);
    furi_hal_gpio_init(WS2812_CE_PIN, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
}


void ws2812_send_spi(void) {
    furi_hal_spi_bus_tx(WS2812_HANDLE, ws2812_buffer, WS2812_BUFFER_SIZE,  WS2812_TIMEOUT);
}


#define WS2812_FILL_BUFFER(COLOR) \
    for( uint8_t mask = 0x80; mask; mask >>= 1 ) { \
        if( COLOR & mask ) { \
            *ptr++ = 0xfc; \
        } else { \
            *ptr++ = 0x80; \
        } \
    }

void ws2812_pixel(uint16_t led_no, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t* ptr = &ws2812_buffer[24 * led_no];
    WS2812_FILL_BUFFER(g);
    WS2812_FILL_BUFFER(r);
    WS2812_FILL_BUFFER(b);
}

void ws2812_pixel_all(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t* ptr = ws2812_buffer;
    for(uint16_t i = 0; i < WS2812_NUM_LEDS; ++i) {
        WS2812_FILL_BUFFER(g);
        WS2812_FILL_BUFFER(r);
        WS2812_FILL_BUFFER(b);
    }
}
