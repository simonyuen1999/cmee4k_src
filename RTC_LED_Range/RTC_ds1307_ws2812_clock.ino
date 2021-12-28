/*
 * Connet to DS1307 RTC (Real Time Clock) via I2C and Wire lib.
 * Get time (Hour,Min,Dec) from DS1307 RTC and display time in a WS2812b LED strip.
 * The LED strip is in 3 attached ranges phyical form.
 * The number of LED in each range:
 *    Inner is 40, Middle is 48, Outer is 60; total is 148 LED
 *    
 * See the product in https://www.aliexpress.com/item/4000352752774.html
 * 2021-Mar: the "148 LEDS (3 ranges)" costs CAD $16.59 + $1.83 shipping from China
 *
 * In this project, we connect the Data-In from inner range and chain up middle and then outer range.
 * Therefore, the inner range is the beginning of the LED strip.
 * Since the inner range is 40 LED, it cannot exact map to 12 hour, so we calculate the closest position.
 * Same case for the middle range, it is 48 LED which cannot map to 60 min.
 * The outer range is 60 LED, it is exactly map to 60 sec position.
 *   To make it more fun, and show the closest position, we added tail LED in light color. 
 *   Sec has 3 tail LEDs, min has 2 and hour has 1 tail LED.
 *   
 * Another factor: we put the connection in the bottom, therefore the starting LED index 0 is starting
 * from the bottom.  In the calculate, we add the offset value back to the top.
 */ 

#include "RTClib.h"
#include "FastLED.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

/* When define debug, use counter (timer variable) to increare the speed for debugging LED display. */
// #define debug  1

#define NUM_LEDS   148
#define WHT_BRIG   60

CRGB leds[NUM_LEDS];

CRGB SecColor0 = CRGB{ 255,  0,  0 };
CRGB SecColor1 = CRGB{  30,  0,  0 };
CRGB SecColor2 = CRGB{   5, 0,  0 };

CRGB MinColor0 = CRGB{ 0,   0, 255 };
CRGB MinColor1 = CRGB{ 0,   0,  25 };
CRGB MinColor2 = CRGB{ 0,   0,  5 };

// Green is too bright
CRGB HurColor0 = CRGB{ 0,  100,  0 };
CRGB HurColor1 = CRGB{ 0,    5,  0 };
             
#define DATA_PIN   3
#define LED_PIN   13

RTC_DS1307 rtc;

#ifdef debug
int timer = 0;
#endif

void setup () {
#ifdef debug
  Serial.begin(115200);
#else
  pinMode( LED_PIN, OUTPUT );
#endif


  if (!rtc.begin()) {
 #ifdef debug   
    Serial.println("Couldn't find RTC");
    Serial.flush();
 #endif
    abort();
  }

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (!rtc.isrunning()) {
     // New device or power loss, set sketch to compiling time.
     // Contact Arduino to upload, so it sync with computer time.
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(WHT_BRIG); 
  FastLED.clear();
}

void hled( int hr, int min ) {
    // Calculate the hr position within 40 LED space
    hr %= 12;
    hr = int( hr * 3.33 );
    if ( min >= 20 ) hr++;
    if ( min >= 40 ) hr++;
    // Fixing the position to opposition side
    hr += 20;
    hr %= 40;
    // Hr is internal range and start with zero
    leds[ hr-- ] = HurColor0;  // Main
    if ( hr < 0 ) hr = 39;
    leds[ hr   ] = HurColor1;  // Tail
}

void mled( int min, int sec ) {
    // Calculate the min position within 48 LED space
    // 3600 sec (1 hour) maps to 48 LED position
    // 3600/48=75 or 3600/75 -> 48 position
    min = ( min * 60 + sec ) / 75;
    // Translate to oppsition side
    min += 24;
    int min1 = min - 1;
    int min2 = min - 2;
    min  %= 48;
    min1 %= 48;
    min2 %= 48;
    // Offset 40 to second range
    leds[ min  + 40 ] = MinColor0;  // Main
    leds[ min1 + 40 ] = MinColor1;
    leds[ min2 + 40 ] = MinColor2;  // Tail
}

void sled( int sec ) {
    // Translate to oppositive side
    sec += 30;
    int sec1 = sec - 1;
    int sec2 = sec - 2;
    int sec3 = sec - 3;
    sec  %= 60;
    sec1 %= 60;
    sec2 %= 60;
    sec3 %= 60;
    // Offset 88 to third range
    leds[ sec  + 88 ] = SecColor0;
    leds[ sec1 + 88 ] = SecColor1;
    leds[ sec2 + 88 ] = SecColor2;
    leds[ sec3 + 88 ] = SecColor2;
}

void setLed( int hr, int min, int sec ) {
  hled( hr, min );
  mled( min, sec );
  sled( sec );
}  

void loop () {

#ifdef debug
    int sec = timer % 60;
    int min = timer / 60;
    int hr  = min   / 60;
    min     = min % 60;
    hr      = hr  % 12;
#else
    DateTime now = rtc.now();
    int hr   = now.hour();
    int min  = now.minute();
    int sec  = now.second();

    /*
    //int year    = now.year();
    //int month   = now.month();
    //int day     = now.day();
    //int hour    = now.hour();
    //int minute  = now.minute();
    //int second  = now.second();
    char APM[3] = "AM";
    if ( hour > 11 ) {
       strcpy( APM, "PM" );
       if ( hour > 12 ) hour -= 12;
    }
    */
#endif

    // Set the LED with the current time
    FastLED.clear();   
    hled( hr, min );
    mled( min, sec );
    sled( sec );
    FastLED.show();

#ifdef debug
    delay(50);
    timer++;
#else
    delay( 250 );
    digitalWrite( LED_PIN, ( digitalRead( LED_PIN ) == HIGH ? LOW : HIGH ) );
#endif
}
