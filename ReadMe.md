# Application led strip

https://www.newinnovations.nl/post/controlling-ws2812-and-ws2812b-using-only-stm32-spi/

# Protocol

The WS2812/WS2812B leds use 24 bits for the green, red and blue values. Bits on the data line are encoded by a high pulse followed by a low pulse.

![Protocol](doc/ws2812b-timings_huf17f8f9c38d87ddd2611126f0e39934c_125438_1200x0_resize_lanczos_2.png)

WS2812B protocol and timing
WS2812B protocol and timing

With the WS2812B a “0” is encoded using a high pulse of 0.35 µs followed by a low pulse of 0.90 µs. A “1” is encoded using a high pulse of 0.90 µs followed by a low pulse of 0.35 µs. The combined length for the high and low pulses is always 1.25 µs.

Sending more than 24 bits will shift the bits to the next led in line. Bits get clocked in from the shift-registers to the pwm drivers when the line stays low for at least 50 µs (called a “reset” condition).

Sending the data for a single led takes 24 × 1.25 µs + 50 µs = 80 µs. For 8 leds it takes: 8 × 24 × 1.25 µs + 50 µs = 290 µs

The WS2812 timings are similar.
## Actual timing requirements

Tim “cpldcpu” has researched the actual timing requirements and found that they are quite liberal. His conclusions were:

-    A reset is issued as early as at 9 µs (much less that the 50 µs from the data sheet)
-    Cycle time of a bit should be at least 1.25 µs [req 1] (value from data sheet) and at most 9 µs [req 2] (time for reset)
-    A “0” can be encoded with a high pulse as short as 62.5 ns [req 3], but should not be longer than 0.50 µs [req 4]
-    A “1” can be encoded with a high pulse almost as long as the total cycle time, but should not be shorter than 0.625 µs [req 5]

## Using SPI: choosing the number of bits

SPI is by far the easiest way to output pulses with equal length. We need to output several SPI bits (ie high and low pulses) for each WS1812 bit. The timing requirements of the WS2812/WS2812B dictate the speed of SPI peripheral. It also depends on the number of SPI bits (pulses) we use to create a single WS1812 bit. We will find that more bits allow for a wider range of speeds.
Option 1: Using 3 SPI bits / pulses

This is the minimal amount of bits we can use.

-    “0” will be encoded as 100
-    “1” will be encoded as 110

Timing requirements:

-    3 pulses > 1.25 µs → 1 pulse > 417 ns
-    3 pulses < 9 µs → 1 pulse < 3000 ns
-    1 pulse > 62.5 ns
-    1 pulse < 500 ns
-    2 pulses > 625 ns → 1 pulse > 413 ns

Leads to:

-    417 ns < pulse < 500 ns → 2.00 Mb/s < SPI bitrate < 2.39 Mb/s

Option 2: Using 4 SPI bits / pulses

-    “0” will be encoded as 1000
-    “1” will be encoded as 1110

Timing requirements:

-    4 pulses > 1.25 µs → 1 pulse > 313 ns
-    4 pulses < 9 µs → 1 pulse < 2250 ns
-    1 pulse > 62.5 ns
-    1 pulse < 500 ns
-    3 pulses > 625 ns → 1 pulse > 208 ns

Leads to: 313 ns < pulse < 500 ns → 2.00 Mb/s < SPI bitrate < 3.19 Mb/s
Option 3: Using 8 SPI bits / pulses

-    “0” will be encoded as 10000000
-    “1” will be encoded as 11111100

Timing requirements:

-    8 pulses > 1.25 µs → 1 pulse > 157 ns
-    8 pulses < 9 µs → 1 pulse < 1125 ns
-    1 pulse > 62.5 ns
-    1 pulse < 500 ns
-    6 pulses > 625 ns → 1 pulse > 105 ns

Leads to: 157 ns < pulse < 500 ns → 2.00 Mb/s < SPI bitrate < 6.36 Mb/s
Going for 8 bits

| SPI bits |  “0”      | “1”       |  min bitrate |  max bitrate |  reset pulses (50 µs) @ max |
|----------|-----------|-----------|--------------|--------------|-----------------------------|
| 3        | 100       | 110       | 2.00 Mb/s    |  2.39 Mb/s   |  120                        |
| 4        | 1000      | 1110      | 2.00 Mb/s    |  3.19 Mb/s   |  160                        |
| 8        | 10000000  | 11111100  | 2.00 Mb/s    |  6.36 Mb/s   |  318                        |

It is clear that using more bits provides a broader range for the SPI bitrate. That’s important because the SPI bitrate on STM32 devices is divided by powers of 2, which limits the possible values dramatically. For example with the STM32G474 running at 170MHz, the SPI bitrate in the range of 1-10 Mb/s can only be 5.3 Mb/s (/32), 2.7 Mb/s (/64) or 1.3 Mb/s (/128).

With this device the 4 bit approach would work on 2.7 Mb/s, where as the 3 bit approach would require lowering the overall device (or bus) speed. The 8 bit approach works with either the 5.3 Mb/s (/32) or the 2.7 Mb/s (/64) bitrate.

The 8 bit approach ensures that a working bitrate can always be found (because the upper limit is more than twice the lower limit). And the 8 bits approach eliminates the need for bit-shifting. An easy choice.
The code is as simple as this

### ws2812-spi.h
```
#define WS2812_NUM_LEDS 8
#define WS2812_SPI_HANDLE hspi2

#define WS2812_RESET_PULSE 60
#define WS2812_BUFFER_SIZE (WS2812_NUM_LEDS * 24 + WS2812_RESET_PULSE)

extern SPI_HandleTypeDef WS2812_SPI_HANDLE;
extern uint8_t ws2812_buffer[];

void ws2812_init(void);
void ws2812_send_spi(void);
void ws2812_pixel(uint16_t led_no, uint8_t r, uint8_t g, uint8_t b);
void ws2812_pixel_all(uint8_t r, uint8_t g, uint8_t b);
```
### ws2812-spi.c
```
#include <string.h>
#include "main.h"
#include "ws2812-spi.h"

uint8_t ws2812_buffer[WS2812_BUFFER_SIZE];

void ws2812_init(void) {
memset(ws2812_buffer, 0, WS2812_BUFFER_SIZE);
ws2812_send_spi();
}

void ws2812_send_spi(void) {
HAL_SPI_Transmit(&WS2812_SPI_HANDLE, ws2812_buffer, WS2812_BUFFER_SIZE, HAL_MAX_DELAY);
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
uint8_t * ptr = &ws2812_buffer[24 * led_no];
WS2812_FILL_BUFFER(g);
WS2812_FILL_BUFFER(r);
WS2812_FILL_BUFFER(b);
}

void ws2812_pixel_all(uint8_t r, uint8_t g, uint8_t b) {
uint8_t * ptr = ws2812_buffer;
for( uint16_t i = 0; i < WS2812_NUM_LEDS; ++i) {
WS2812_FILL_BUFFER(g);
WS2812_FILL_BUFFER(r);
WS2812_FILL_BUFFER(b);
}
}
```

### Usage

-    Initialize using ws2812_init()
-    Set individual pixels using ws2812_pixel or all pixels using ws2812_pixel_all
-    Whenever you want to send the buffer to the leds call ws2812_send_spi()

Calling ws2812_send_spi() will stall your program until all bits have been sent. Want this done in the background? Switch to DMA (see below)


### Testing

I tested the actual bitrate limits with a setup of 8 leds:

| type    | min bitrate | max bitrate |
|---------|-------------|-------------|
| WS2812  | 2.25 Mb/s   | 10 Mb/s     |
| WS2812B | 2.25 Mb/s   | 8.25 Mb/s   |

Based on this and the original calculations I would recommend to choose a rate between 3 and 6 Mb/s. I also found that the reset length should be at least the 50 µs.
## Using DMA

This code can easily be upgraded using DMA. Just enable DMA with the SPI peripheral in the STM32CubeIDE device configuration. Then change ws2812_init() to start the first DMA transfer.
```
void ws2812_init(void) {
memset(ws2812_buffer, 0, WS2812_BUFFER_SIZE);
HAL_SPI_Transmit_DMA(&WS2812_SPI_HANDLE, ws2812_buffer, WS2812_BUFFER_SIZE);
}
```  

Amend the generated interrupt code to initiate a new DMA transfer as soon as the last one is completed. (DMA channels and handles are STM32 device/family specific)
```
void DMA1_Channel1_IRQHandler(void)
{
HAL_DMA_IRQHandler(&hdma_spi2_tx);
/* USER CODE BEGIN DMA1_Channel1_IRQn 1 */
HAL_SPI_Transmit_DMA(&WS2812_SPI_HANDLE, ws2812_buffer, WS2812_BUFFER_SIZE);
/* USER CODE END DMA1_Channel1_IRQn 1 */
}
```
The ws2812_send_spi() function is no longer needed. Just update individual pixels using ws2812_pixel or all pixels using ws2812_pixel_all. The leds are automatically updated during the ever repeating DMA transfers.

