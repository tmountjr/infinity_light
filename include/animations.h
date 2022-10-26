// TODO: using delay() on the animations is going to potentially cause issues with
// the interrupts. Better to use millis() and reset the "delay" timer on each change.

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 22 // Probably 66 at the end of the day
#define BRIGHTNESS 255
#define PIXEL_PIN 0

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIXEL_PIN);

uint8_t base_wheel = 0; // default to white base
uint16_t delay_ms = 20;
uint32_t base_color = 0;

enum led_pattern_type
{
  LEDS_OFF, // Make sure this item is always *first* in the enum!
  LEDS_SOLID_WHITE,
  LEDS_SOLID_RED,
  LEDS_SOLID_GREEN,
  LEDS_SOLID_BLUE,
  LEDS_SOLID_COLOR_CYCLE,
  LEDS_CHASE_WHITE,
  LEDS_CHASE_COLOR_CYCLE,
  LEDS_RAINBOW,
  LEDS_RAINBOW_CYCLE,
  LEDS_LAST // Make sure this item is always *last* in the enum!
};
volatile enum led_pattern_type led_pattern;

/*
 * Input a value 0 to 255 to get a color value.
 * The colors are a transition R->G->B and back to R.
 */
uint32_t wheel(byte wheelPos)
{
  if (wheelPos < 85)
  {
    return pixels.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
  }
  else
  {
    if (wheelPos < 170)
    {
      wheelPos -= 85;
      return pixels.Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    else
    {
      wheelPos -= 170;
      return pixels.Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
  }
}

/* ----------------------------------------------------------------------------
 * LED PATTERN METHODS
 * Make sure that any methods here do NOT iterate for long periods of time
 * or else the buttons will not be responsive to user input.
 * Instead, use static variables to control the state of long-changing patterns.
 * -----------------------------------------------------------------------------
 */

/*
 * Fill the dots one after the other with a color
 */
void colorWipe(uint32_t c)
{
  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, c);
  }
  pixels.show();
}

/*
 * Fill the dots one after the other with a color using the specified delay
 */
void colorWipe(uint32_t c, uint8_t wait)
{
  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, c);
    pixels.show();
    delay(wait);
  }
}

/*
 * Shifting rainbow effect
 */
void rainbow(uint8_t wait)
{
  static uint16_t j;

  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, wheel((i + j) & 255));
  }
  pixels.show();
  delay(wait);

  j += 1;
  if (j >= 256)
    j = 0;
}

/*
 * Equally-distributed rainbow color shift effect
 */
void rainbowCycle(uint8_t wait)
{
  static uint16_t j = 0;

  for (uint16_t i = 0; i < pixels.numPixels(); i++)
  {
    pixels.setPixelColor(i, wheel(((i * 256 / pixels.numPixels()) + j) & 255));
  }
  pixels.show();
  delay(wait);

  j += 1;
  if (j >= 256 * 5)
    j = 0;
}

/*
 * Theatre-style crawling lights.
 */
void theaterChase(uint32_t c, uint8_t wait)
{
  for (int q = 0; q < 3; q++)
  {
    for (uint16_t i = 0; i < pixels.numPixels(); i += 3)
    {
      pixels.setPixelColor(i + q, c); // turn every third pixel on
    }
    pixels.show();

    delay(wait);

    for (uint16_t i = 0; i < pixels.numPixels(); i += 3)
    {
      pixels.setPixelColor(i + q, 0); // turn every third pixel off
    }
  }
}

/*
 * Theatre-style crawling lights with rainbow effect
 */
void theaterChaseRainbow(uint8_t wait)
{
  static int j = 0;

  for (int q = 0; q < 3; q++)
  {
    for (uint16_t i = 0; i < pixels.numPixels(); i = i + 3)
    {
      pixels.setPixelColor(i + q, wheel((i + j) % 255)); // turn every third pixel on
    }
    pixels.show();

    delay(wait);

    for (uint16_t i = 0; i < pixels.numPixels(); i = i + 3)
    {
      pixels.setPixelColor(i + q, 0); // turn every third pixel off
    }
  }

  j += 1;
  if (j >= 256)
    j = 0;
}

/**
 * Show the animation based on the value of led_pattern.
 */
void display()
{
  switch (led_pattern)
  {
  case LEDS_OFF:
    colorWipe(pixels.Color(0, 0, 0), 0); // black, no delay
    break;
  case LEDS_SOLID_WHITE:
    colorWipe(pixels.Color(255, 255, 255)); // WHITE, no delay
    break;
  case LEDS_SOLID_RED:
    colorWipe(pixels.Color(255, 0, 0)); // RED, no delay
    break;
  case LEDS_SOLID_GREEN:
    colorWipe(pixels.Color(0, 255, 0)); // GREEN, no delay
    break;
  case LEDS_SOLID_BLUE:
    colorWipe(pixels.Color(0, 0, 255)); // BLUE, no delay
    break;
  case LEDS_SOLID_COLOR_CYCLE:
    base_wheel += 1;
    base_color = wheel(base_wheel);
    colorWipe(base_color, 1); // current color, no delay
    break;
  case LEDS_CHASE_WHITE:
    theaterChase(pixels.Color(255, 255, 255), delay_ms);
    break;
  case LEDS_CHASE_COLOR_CYCLE:
    theaterChaseRainbow(10);
    break;
  case LEDS_RAINBOW:
    rainbow(delay_ms);
    break;
  case LEDS_RAINBOW_CYCLE:
    rainbowCycle(delay_ms);
    break;

  default:
    colorWipe(pixels.Color(0, 0, 0), 0); // black, no delay
    break;
  }
}
