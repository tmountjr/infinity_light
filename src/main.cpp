#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <SoftwareSerial.h>

#define BUTTON_PIN 1
#define AT_TX_PIN 4
#define AT_RX_PIN 3  // unused

SoftwareSerial atSerial(AT_RX_PIN, AT_TX_PIN);

// Move the animations into a separate file to keep things clean.
#include "animations.h"


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

  atSerial.begin(9600);
}

void loop() {
  display();
}

/**
 * Basically the loop() method has a delay, so every 50ms the display() method runs.
 * display() comes fron animations.h and every iteration, it detects
 * which animation pattern should be processed and runs that code.
 * 
 * For some methods, eg. theaterChase(), there's an additional delay
 * built into the method. So display() fires, there's a 20ms delay
 * between stages of the animation, and then there's another 50ms
 * delay after the animation completes.
 * 
 * What should happen is that each animation maintains a global
 * way of figuring out what to do next and when. So in the case of
 * theaterChase(), every third pixel turns on, waits 20ms, then turns
 * off again, eg for a 12px strip:
 *   q = 0
 *     0, 3, 6, 9, 12 turn on
 *     delay 20ms
 *     0, 3, 6, 9, 12 turn off
 *   q = 1
 *     1, 4, 7, 10, (13 but that doesn't exist) turn on
 *     delay 20ms
 *     1, 4, 7, 10 turn off
 *   q = 2
 *     2, 5, 8, 11 turn on
 *     delay 20ms
 *     2, 5, 8, 11 turn off
 *   end of theaterChase(), delay 50ms
 * 
 * Total time to execute theaterChase: 110ms of delays
 * 
 * Better approach might be this: every time theaterChase() is called,
 * the method checks if 20ms have elapsed since the last call.
 * If so, 
 */