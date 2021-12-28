/*
 * Connet to DS1307 or DS3231 RTC (Real Time Clock) via I2C and Wire lib.
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

/* =========================
   Problem in this program: DS3231 using too many memory, SSD1306 cannot be initialized.
   =========================
 */

/* When define debug, use counter (timer variable) to increare the speed for debugging LED display. */
//#define debug  1
 
//#define DS1307
#define DS3231

#ifdef DS1307
#include "RTClib.h"
#endif

#include "FastLED.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DS3231_I2C_ADDRESS 0x68

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels  <-- Even I have 128x64, this reduce memory requirment

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

#ifdef DS1307
// DS1307  rtc;
RTC_DS1307 rtc;
#endif

//char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

#ifdef DS3231
char* monthStr[] = { "", "Jan", "Fed", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char* wkdayStr[] = { "", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
#endif

#ifdef debug
int timer = 0;
#endif

void setup ()
{
  Serial.begin(115200);
  pinMode( LED_PIN, OUTPUT );
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64 (swtich to 128x32)
      Serial.println(F("SSD1306 allocation failed"));
//    for(;;) { // Don't proceed, loop forever   
//       digitalWrite( LED_PIN, ( digitalRead( LED_PIN ) == HIGH ? LOW : HIGH ) );
//       delay( 500 );
//    }
  }

  display.clearDisplay();
  display.display();
  display.setTextColor(SSD1306_WHITE);

#ifdef DS1307
  if (!rtc.begin()) {
   
    Serial.println("Couldn't find RTC");
    Serial.flush();

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Cannot find RTC");
    display.display();
    abort();
  }

  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (!rtc.isrunning()) {
     // New device or power loss, set sketch to compiling time.
     // Contact Arduino to upload, so it sync with computer time.
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
#endif
  
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

/*
  RTCDateTime dt = rtc.getDataTime();
  Serial.println(rtc.dateFormat("d-m-Y", dt));
  Serial.println(rtc.dateFormat("h:i:s a", dt));
 */
 
#ifdef debug
    int year  = 2021;
    int month = 6;
    int day   = 4;
    int sec   = timer % 60;
    int min   = timer / 60;
    int hr    = min   / 60;
    min       = min % 60;
    hr        = hr  % 12;
    int dayWeek = 0;
#else

  #ifdef DS1307
    DateTime now = rtc.now();
    int year    = now.year();
    int month   = now.month();
    int day     = now.day();
    int hr      = now.hour();
    int min     = now.minute();
    int sec     = now.second();
    int dayWeek = now.dayOfTheWeek();

    // Just RTC, doc: https://forum.arduino.cc/t/rtc-running-fast/436845/4
    if ( (hr==4) && (min==1) && (sec==0) ) {
       // run daily at exactly 4:01 A.M.
       delay(3900);      // wait 3.9 seconds (that's 2.9 seconds plus 1 second extra)
       rtc.adjust(DateTime(year, month, day, 4, 1, 1 ));
       // set time to 4:01:01 A.M. (notice the extra second)
       // The extra second in the previous two lines is there for a reason.
       // Otherwise, the time would be set back to exactly 4:01 A.M. (with 0 seconds),
       // which could trigger the time set again, and so on, infinitely, and we don't want that.
    }
  #endif

  #ifdef DS3231
    int year, month, day, hr, min, sec, dayWeek;
    readDS3231time(&sec, &min, &hr, &dayWeek, &day, &month, &year);
  #endif
  
#endif
    int hour = hr;
    char APM[3] = "AM";
    if ( hour > 11 ) {
       strcpy( APM, "PM" );
       if ( hour > 12 ) hour -= 12;
    }

    // Set the LED with the current time
    FastLED.clear();   
    hled( hr, min );
    mled( min, sec );
    sled( sec );
    FastLED.show();
    
    display.clearDisplay();
    display.setCursor(0,0);
    //display.println(clock.dateFormat("Y-m-d", dt));
    //display.println(clock.dateFormat("h:i:sa", dt));
    display.setTextSize(1);
    
#ifdef DS1307    
    display.println( now.toString("YYYY/MMM/DD (DDD)") );
#endif

    char time_str[20];

#ifdef DS3231
    sprintf( time_str, "20%2d/%3s/%02d (%3s)", year, monthStr[month], day, wkdayStr[dayWeek] );
    display.println( time_str );
#endif

    // display.drawLine( 0, 12, 128, 12, SSD1306_WHITE);
    display.println( " " );

    sprintf( time_str, "%2d:%02d:%02d", hour, min, sec );
    display.setTextSize(2);
    display.print( time_str );
    display.setTextSize(1);
    display.print( " " );
    display.println( APM );

    // display.drawLine( 0, 35, 128, 35, SSD1306_WHITE);
    // display.setCursor( 1 , 25 );
    
    display.display();

#ifdef debug
    delay(50);
    timer++;
#else
    delay( 250 );
    digitalWrite( LED_PIN, ( digitalRead( LED_PIN ) == HIGH ? LOW : HIGH ) );
#endif
}

// ============================================

#ifdef DS3231
void readDS3231time(int *second, int *minute, int *hour, int *dayOfWeek, int *dayOfMonth, int *month, int *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);     // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  
  // request seven bytes of data from DS3231 starting from register 00h
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}

// Convert binary coded decimal to normal decimal numbers
int bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}
#endif
