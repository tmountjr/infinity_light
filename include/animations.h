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
  pixels.fill(c);
  pixels.show();
}

void initColorSwirl()
{
  /**
   * context:
   *   [0]: base wheel color
   */
  next_frame.not_before = millis();
  next_frame.context = { 0 };
}

/**
 * Fill the dots one after the other with a color using the specified delay.
 * Difference between this and rainbow() is that rainbow() animates pixel by pixel
 * and this one floods the entire thing with the same color.
 */
void colorSwirl()
{
  uint32_t current = millis();
  uint32_t not_before = next_frame.not_before;
  if (not_before >= 0 && current >= not_before)
  {
    next_frame.not_before = -1;
    int base = next_frame.context[0];

    uint32_t color = wheel(base);

    pixels.fill(color);
    pixels.show();

    next_frame.not_before = millis() + delay_ms;
    next_frame.context = {(base + 1) % 256 };
  }
}

/**
 * context:
 *   [0]: base wheel value
 */
void initRainbow()
{
  next_frame.not_before = millis();
  next_frame.context = { 0 };
}

/*
 * Shifting rainbow effect
 */
void rainbow()
{
  uint32_t current = millis();
  uint32_t not_before = next_frame.not_before;
  if (not_before >= 0 && current >= not_before)
  {
    next_frame.not_before = -1;
    int j = next_frame.context[0];
    for (uint16_t i = 0; i < pixels.numPixels(); i++)
    {
      pixels.setPixelColor(i, wheel((i + j) & 255));
    }
    pixels.show();

    next_frame.not_before = millis() + delay_ms;
    next_frame.context = {(j + 1) % 256};
  }
}

/**
 * context:
 *   [0]: base wheel value
 */
void initRainbowCycle()
{
  next_frame.not_before = millis();
  next_frame.context = {0};
}

/**
 * Equally-distributed rainbow color shift effect
 */
void rainbowCycle()
{
  uint32_t current = millis();
  uint32_t not_before = next_frame.not_before;
  if (not_before >= 0 && current >= not_before)
  {
    next_frame.not_before = -1;
    int j = next_frame.context[0];
    for (uint16_t i = 0; i < pixels.numPixels(); i++)
    {
      pixels.setPixelColor(i, wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    
    next_frame.not_before = millis() + delay_ms;
    next_frame.context = {(j + 1) % 256};
  }
}

/**
 * Initialize the next_frame value for the theaterChase() method.
 */
void initTheaterChase()
{
  /**
   * context:
   *   [0]: which of the set of 3 LEDs should change, wraps back to 0 after 2
   *   [1]: turn the LEDs on or off (on = 1, off = 0)
   */
  next_frame.not_before = millis();
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
 * Initialize the next_frame value for the theaterChaseRainbow() method.
 */
void initTheaterChaseRainbow()
{
  /**
   * context:
   *    [0]: iterates through 1-255, powers the rainbow change
   *    [1]: which of the set of 3 LEDs should change, wraps back to 0 after 2
   *    [2]: turn the LEDs on or off (on = 1, off = 0)
  */
 next_frame.not_before = millis();
 next_frame.context = { 0, 0, 1 };
}

/**
 * Theatre-style crawling lights with rainbow effect
 */
void theaterChaseRainbow()
{
  uint32_t current = millis();
  uint32_t not_before = next_frame.not_before;
  if (not_before >= 0 && current >= not_before)
  {
    next_frame.not_before = -1;
    int j = next_frame.context[0];
    int q = next_frame.context[1];
    byte on_off = next_frame.context[2];

    for (uint16_t i = 0; i < pixels.numPixels(); i += 3)
    {
      uint32_t color = wheel((i + j) % 255);
      if (!on_off)
        color = 0;

      pixels.setPixelColor(q + i, color);
    }
    pixels.show();

    if (!on_off)
    {
      next_frame.not_before = 0;
      next_frame.context[1] = (q + 1) % 3;
      if (next_frame.context[1] == 0)
      {
        next_frame.context[0] = (j + 1) % 256;
      }
    }
    else
    {
      next_frame.not_before = millis() + delay_ms;
    }
    next_frame.context[2] = !on_off;
  }
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
    colorWipe(pixels.Color(0, 0, 0)); // black, no delay
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
    if (init_new_pattern)
      initColorSwirl();
    colorSwirl();
    break;
  case LEDS_CHASE_WHITE:
    if (init_new_pattern)
      initTheaterChase();
    theaterChase(pixels.Color(255, 255, 255));
    break;
  case LEDS_CHASE_COLOR_CYCLE:
    if (init_new_pattern)
      initTheaterChaseRainbow();
    theaterChaseRainbow();
    break;
  case LEDS_RAINBOW:
    if (init_new_pattern)
      initRainbow();
    rainbow();
    break;
  case LEDS_RAINBOW_CYCLE:
    if (init_new_pattern)
      initRainbowCycle();
    rainbowCycle();
    break;

  default:
    colorWipe(pixels.Color(0, 0, 0)); // black, no delay
    break;
  }

  if (init_new_pattern)
  {
    last_pattern_type = led_pattern;
  }
}
