
#include <string.h> // for memset()
#include <stdlib.h> // for rand()
#include <math.h> // for pow()
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "hardware/spi.h"

#define SYS_CLK 125000 // 125 MHz system clock

// spi0 GPIO pin assignments
#define SPI0_CS 17
#define SPI0_SCK 18
#define SPI0_MOSI 19
#define SPI0_MISO 16

#define SPI0_BAUD 8000000 // 8 MHz spi0 baud rate

#define LED_COUNT 8     // number of WS2812B LEDs in the strip
#define COLOR_BYTES 3   // number of bytes to define color in a WS2812B LED
#define LED_BYTES (LED_COUNT * COLOR_BYTES)  // total number of bytes needed to hold color settings for all LEDs
#define RESETBYTES 50  // number of spi bytes to send for a WS2812B reset

#define SPI_BITS_PER_CYCLE 8 // number of bits in spi signal that represents a WS2812B clock cycle
#define SPIBYTES (RESETBYTES + SPI_BITS_PER_CYCLE * LED_BYTES)  // number of spi bytes sent in each WS2812B display refresh

// spi bytes to create modulated WS2812B data signal
#define T0_BYTE 0xC0
#define T1_BYTE 0xFC

#define REFRESH_MS 20  // number of milliseconds to wait between WS2812B display refresh

// data structure to represent a WS2812B color
typedef struct {
  unsigned char G, R, B;
} LEDPixel;

// declare methods
void init();
void initSpi();
void displayTask();
int colorByteToSPIBuffer (uint8_t * spibuffer, int bufferoffset, unsigned char colorbyte);
void clearLEDs();
void setLEDColors(int led, LEDPixel colors);
void rollingBit(int setColor, int sleepMS);
void randomizeColors();
void cylon(int setColor, int sleepMS);
void electricRain();
