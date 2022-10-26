// TODO: using delay() on the animations is going to potentially cause issues with
// the interrupts. Better to use millis() and reset the "delay" timer on each change.

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoSTL.h>

using namespace std;

#define NUM_LEDS 22 // Probably 66 at the end of the day
#define BRIGHTNESS 255
#define PIXEL_PIN 0

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIXEL_PIN);

uint8_t base_wheel = 0; // default to white base
uint16_t delay_ms = 20;
uint32_t base_color = 0;

struct Frame
{
  uint32_t not_before;
  vector<int> context;
} next_frame;

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
volatile enum led_pattern_type last_pattern_type = led_pattern_type::LEDS_SOLID_WHITE; // Make sure this matches what's in setup()

/**
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

/**
 * ----------------------------------------------------------------------------
 * LED PATTERN METHODS
 * Make sure that any methods here do NOT iterate for long periods of time
 * or else the buttons will not be responsive to user input.
 * Instead, use static variables to control the state of long-changing patterns.
 * -----------------------------------------------------------------------------
 */

/**
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

/**
 * Fill the dots one after the other with a color using the specified delay
 * TODO: refactor wait
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
 * TODO: refactor wait
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

/**
 * Equally-distributed rainbow color shift effect
 * TODO: refactor wait
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

/**
 * Initialize the next_frame value for the theaterChase() method.
 */
void initTheaterChase()
{
  /**
   * not_before: don't run the next frame before this particular ms
   * context:
   *   [0]: which of the set of 3 LEDs should change, wraps back to 0 after 2
   *   [1]: turn the LEDs on or off (on = 1, off = 0)
   */
  uint32_t current = millis();
  next_frame.not_before = current + delay_ms;
  next_frame.context = { 0, 1 };
}

/**
 * Theatre-style crawling lights.
 */
void theaterChase(uint32_t c)
{
  uint32_t current = millis();
  uint32_t not_before = next_frame.not_before;
  if (not_before >= 0 && current >= not_before)
  {
    // Prevent race condition by blanking out the not_before; it will be reset
    // at the end of the method.
    next_frame.not_before = -1;
    int q = next_frame.context[0];
    byte on_off = next_frame.context[1]; // on = 1, off = 0

    uint32_t color = c;
    if (!on_off)
      color = 0;

    for (uint16_t i = 0; i < pixels.numPixels(); i += 3)
    {
      pixels.setPixelColor(q + i, color);
    }
    pixels.show();

    // Set up the not_before and q values differently depending on the frame.
    if (!on_off) {
      next_frame.not_before = 0;
      next_frame.context[0] = (q + 1) % 3;
    } else {
      next_frame.not_before = millis() + delay_ms;
    }
    next_frame.context[1] = !on_off;
  }
}

/**
 * Theatre-style crawling lights with rainbow effect
 * TODO: refactor wait
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
  byte init_new_pattern = (int)led_pattern != (int)last_pattern_type;

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
    if (init_new_pattern)
    {
      initTheaterChase();
    }
    theaterChase(pixels.Color(255, 255, 255));
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

  if (init_new_pattern)
  {
    last_pattern_type = led_pattern;
  }
}
