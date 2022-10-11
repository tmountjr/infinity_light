#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define NUM_LEDS 6
#define BRIGHTNESS 128
#define PIXEL_PIN 0
#define BUTTON_PIN 1

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIXEL_PIN);

uint8_t base_wheel = 0; // default to white base
uint16_t delay_ms = 20;
uint32_t base_color = 0;
uint32_t white = pixels.Color(255, 255, 255);

volatile int pixel_state = 1; // rainbow
volatile byte last_button_state = LOW;

ISR(PCINT0_vect) {
  byte button_state = digitalRead(BUTTON_PIN);
  if (button_state != last_button_state && button_state == HIGH) {
    pixel_state = !pixel_state;
  }
  last_button_state = button_state;
}

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

void setup()
{
  cli();
  GIMSK |= (1 << PCIE);
  PCMSK |= (1 << BUTTON_PIN);
  sei();
  pinMode(PIXEL_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Start up the NeoPixels
  pixels.begin();
  pixels.setBrightness(BRIGHTNESS); // use only for init, not as an effect
}

void loop() {
  if (pixel_state == 1) {
    rainbowCycle(delay_ms);
  } else {
    pixels.fill(white, 0, NUM_LEDS);
    pixels.show();
  }
  delay(50);
}

