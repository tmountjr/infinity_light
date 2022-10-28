#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Move the animations into a separate file to keep things clean.
#include "animations.h"

#define BUTTON_PIN 1

volatile byte last_button_state = LOW;

ISR(PCINT0_vect) {
  byte button_state = digitalRead(BUTTON_PIN);
  if (button_state != last_button_state && button_state == HIGH) {
    led_pattern = (led_pattern_type)((int)led_pattern + 1);
    if (led_pattern >= LEDS_LAST)
      led_pattern = LEDS_OFF;
  }
  last_button_state = button_state;
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

  // Initialize the led pattern to a solid white
  led_pattern = led_pattern_type::LEDS_SOLID_WHITE;

  // Serial.setTxBit(4);
  // Serial.begin(9600);
}

void loop() {
  display();
}
