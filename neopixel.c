
#include "neopixel.h"

static mutex_t ledBufferMutex;  // mutex used to lock led buffer, required whenever reading or writing led buffer
static LEDPixel leds[LED_COUNT];  // array of LED color structures

// main thread on core0
int main() {
  // initialize the mcu
  init();  

  // infinite loop for core0
  for(;;) {
    rollingBit();
    electricRain();
    cylon();
    randomizeColors();
  }

}

// initialize the mcu
void init () {
  // set system clock frequency
  set_sys_clock_khz(SYS_CLK, true);
  // initialize stdio in case we use USB or UART
  stdio_init_all();
  // initialize the led buffer mutex
  mutex_init(&ledBufferMutex);
  // launch the WS2812B display task on core1
  multicore_launch_core1(displayTask);
}

// initialize the spi interface
void initSpi () {
  // init spi interface
  spi_init(spi0, SPI0_BAUD);
  spi_set_format(
    spi0,
    8, // bits
    1, // polarity (CPOL)
    1, // phase (CPHA)
    SPI_MSB_FIRST
  );
  gpio_set_function(SPI0_SCK, GPIO_FUNC_SPI);
  gpio_set_function(SPI0_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(SPI0_MISO, GPIO_FUNC_SPI);
}


// an infinite loop task used to update the WS2812B display at routine intervals
void displayTask() {
  spi_inst_t *pspi0 = spi0;  // spi interface handler
  uint8_t spibuffer[SPIBYTES]; // spi buffer
  int bufferoffset = 0;

  // initialize the spi interface
  initSpi();

  for(;;) {
    // clear spi buffer
    memset(spibuffer, 0, SPIBYTES);
    // request blocking access to led buffer
    mutex_enter_blocking(&ledBufferMutex);
    // populate spi buffer with bytes from LED colors by looping through each LED
    for (int loop = 0; loop < LED_COUNT; loop++) {
      // calculate the spi buffer offset for the given LED
      bufferoffset = loop * COLOR_BYTES * SPI_BITS_PER_CYCLE;
      // insert each color byte into the spi buffer, the order must be G, R, B to match WS2812B specification
      bufferoffset = colorByteToSPIBuffer(spibuffer, bufferoffset, leds[loop].G);
      bufferoffset = colorByteToSPIBuffer(spibuffer, bufferoffset, leds[loop].R);
      bufferoffset = colorByteToSPIBuffer(spibuffer, bufferoffset, leds[loop].B);
    }
    // release blocking access to led buffer
    mutex_exit(&ledBufferMutex);

    // write spi buffer to spi0
    gpio_put(SPI0_CS, 0);
    spi_write_blocking(pspi0, spibuffer, SPIBYTES);
    gpio_put(SPI0_CS, 1);

    // wait before refreshing display
    sleep_ms(REFRESH_MS);
  }
}


// insert a WS2812B color byte into the spi buffer at the specified offset, returns the new offset after insert
int colorByteToSPIBuffer (uint8_t * spibuffer, int bufferoffset, unsigned char colorbyte) {
  // expand color byte into spi bytes based on the 
  for (int loop = 0; loop < SPI_BITS_PER_CYCLE; loop++) {
    // insert a spi byte into the spi buffer that represents the WS2812B clock cycle for the most significant bit
    spibuffer[bufferoffset++] = (colorbyte & 0x80) ? T1_BYTE : T0_BYTE;
    // left shift the color byte so we can process the next bit
    colorbyte <<= 1;
  }
  // return the new spi buffer offset
  return bufferoffset;
}


// clear the colors in all the LEDs
void clearLEDs() {
    // request blocking access to led buffer
  mutex_enter_blocking(&ledBufferMutex);
  // loop through all LEDs
  for (int loop = 0; loop < LED_COUNT; loop++) {
    leds[loop].G = 0;
    leds[loop].R = 0;
    leds[loop].B = 0;
  }
  // release blocking access to led buffer
  mutex_exit(&ledBufferMutex);
}


void rollingBit() {
  int count = LED_COUNT * (2 + rand() % 2);
  uint8_t bit = 0x1;

  clearLEDs();
  while (count--) {
    // request blocking access to led buffer
    mutex_enter_blocking(&ledBufferMutex);
    for (int loop = 0; loop < LED_COUNT; loop += 4) {
      leds[loop].G = bit;
      leds[loop + 1].R = bit;
      leds[loop + 2].B = bit;
      leds[loop + 3].R = bit;
      leds[loop + 3].B = bit;
    }
    // release blocking access to led buffer
    mutex_exit(&ledBufferMutex);
    bit <<= 1;
    if (!bit) bit = 0x01;
    sleep_ms(1000);
  }
  clearLEDs();
}

// cylon display demo
void cylon() {
  int count = LED_COUNT * (10 + rand() % 10); // random number of times to cycle demo
  int color = rand() % 3; // random LED color
  int offset = 0; // display offset counter
  int dir = 1; // direction offset moves
  // loop until demo cycle count ends
  while (count--) {
    // clear the LED buffer
    clearLEDs();
    // request blocking access to led buffer
    mutex_enter_blocking(&ledBufferMutex);
    // loop through the LED buffer
    for (int loop = 0; loop < LED_COUNT / 2; loop++) {
      // calculate luminance based on position
      int l = 1 * (int)pow((double)(1 + 1.25), (double)loop);
      // if led position plus offset is not off the display then set LED color
      if (offset + loop >= 0 && offset + loop < LED_COUNT) {
        switch (color) {
          case 0:
            leds[offset + loop].G = l;
            break;

          case 1:
            leds[offset + loop].B = l;
            break;

          case 2:
            leds[offset + loop].R = l;
            break;
        }
      }
      // if led position on opposite end with offset is not off the display then set LED color
      if (offset + LED_COUNT - (loop + 1) >= 0 && offset + LED_COUNT - (loop + 1) < LED_COUNT) {
        switch (color) {
          case 0:
            leds[offset + LED_COUNT - (loop + 1)].G = l;
            break;

          case 1:
            leds[offset + LED_COUNT - (loop + 1)].B = l;
            break;

          case 2:
            leds[offset + LED_COUNT - (loop + 1)].R = l;
            break;
        }
      }
    }
    // release blocking access to led buffer
    mutex_exit(&ledBufferMutex);
    // adjust display offset and reverse direction if end reached
    offset += dir;
    if (offset == LED_COUNT) dir = -1;
    if (offset == -1 * LED_COUNT) dir = 1;
    // delay before demo update
    sleep_ms(100);
  }
  // clear demo from LED display
  clearLEDs();
}


// random LED color display demo
void randomizeColors() {
  int count = LED_COUNT * (3 + rand() % 5); // random number of times to cycle demo
  int level;
  // loop until demo cycle count ends
  while (count--) {
    // clear the LED buffer
    clearLEDs();
    // request blocking access to led buffer
    mutex_enter_blocking(&ledBufferMutex);
    // populate LED buffer with random colors
    for (int loop = 0; loop < LED_COUNT; loop++) {
      // randomize whether LED will be off or on
      switch (rand() % 4) {
        // this LED will be on in case 0
        case 0:
          level = rand() % 64; // random maximum luminence
          // randomize each color byte for the LED
          leds[loop].G = rand() % level;
          leds[loop].B = rand() % level;
          leds[loop].R = rand() % level;
        break;
      }
    }
    // release blocking access to led buffer
    mutex_exit(&ledBufferMutex);
    // delay before demo update
    sleep_ms(600);
  }
}


// electric rain drop display demo
void electricRain() {
  int count = LED_COUNT * (3 + rand() % 5); // random number of times to cycle demo
  int fade, droprandom;
  // loop until demo cycle count ends
	while (count--) {
    // request blocking access to led buffer
    mutex_enter_blocking(&ledBufferMutex);
		// random LED to receive droplet
		int i = rand() % LED_COUNT;
    // random color and luminence of droplet
		leds[i].R = rand() % 128;
		leds[i].G = rand() % 128;
		leds[i].B = rand() % 128;
    // release blocking access to led buffer
    mutex_exit(&ledBufferMutex);
    // random number of cycles before next droplet
		int droprandom = rand() % 10 + 1;
    while (droprandom--) {
      // request blocking access to led buffer
      mutex_enter_blocking(&ledBufferMutex);
			// fade LED droplets
			for (int loop = 0; loop < LED_COUNT; loop++) {
        // red color fade
				if (leds[loop].R > 0) {
          // random fade level
					fade = rand() % ((leds[loop].R / 4) + 1) + 1;
					leds[loop].R -= fade;
				}
        // gree color fade
				if (leds[loop].G > 0) {
          // random fade level
					fade = rand() % ((leds[loop].G / 4) + 1) + 1;
					leds[loop].G -= fade;
				}
        // blud color fade
				if (leds[loop].B > 0) {
          // random fade level
					fade = rand() % ((leds[loop].B / 4) + 1) + 1;
					leds[loop].B -= fade;
				}
			}
      // release blocking access to led buffer
      mutex_exit(&ledBufferMutex);
      // delay before demo update
			sleep_ms(100);
		}
	}
}
