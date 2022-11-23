#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <furi_hal_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WS2812_TIMEOUT 500
#define WS2812_CE_PIN &gpio_ext_pb2
#define WS2812_HANDLE &furi_hal_spi_bus_handle_external

#define WS2812_NUM_LEDS 20
#define WS2812_RESET_PULSE 60
#define WS2812_BUFFER_SIZE (WS2812_NUM_LEDS * 24 + WS2812_RESET_PULSE)
extern uint8_t ws2812_buffer[];

void ws2812_init();
void ws2812_deinit();
void ws2812_pixel(uint16_t led_no, uint8_t r, uint8_t g, uint8_t b);
void ws2812_pixel_all(uint8_t r, uint8_t g, uint8_t b);
void ws2812_send_spi(void);

#ifdef __cplusplus
}
#endif