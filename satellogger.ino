/* vi:ts=4
 * ------------------------------------------------------------------------------
 * Satellogger 
 * 
 * Just a GPS controlled UTC clock for logging amateur radio contacts.
 * 
 * Donald Purnhagen - k4ilg@arrl.net
 * ------------------------------------------------------------------------------
 *
 * This sketch is for LCDs with PCF8574 or MCP23008 chip based backpacks.
 * 
 */
#define BANNER "Satellogger :: K4ILG"
#define VERSION "0.8.1"
#define COPYRIGHT "(c) 2020 - Donald Purnhagen"
#define INVALID "Searching...        "

#include <Wire.h>                          // wires
#include <hd44780.h>                       // hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander
#include <TinyGPS++.h>

// For serial monitor debugging.
// Comment out the define to disable debugging.
#define DEBUG_PORT Serial

// GPS stuff.
const long COORD_SCALING_FACTOR = 100000L;
#include <SoftwareSerial.h>                // serial port for gps
const int GPS_RX = 3; // TX of SoftwareSerial.
const int GPS_TX = 2; // RX of SoftwareSerial.
SoftwareSerial gpsPort(GPS_TX, GPS_RX);


// LCD display address and geometry.
const int LCD_ADDRESS = 0x27;
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

// My best attempt at a satellite logo.
byte logo[] = {
    // Char 0
    B00000, B01010, B01010, B01001, B00101, B00100, B00011, B00000,
    // Char 1
    B10100, B01110, B01100, B00010, B10000, B01100, B00000, B11100
};

// Global variables.
static TinyGPSPlus gps;
hd44780_I2Cexp lcd(LCD_ADDRESS); // Declare lcd object at fixed i2c address.

// Lat and lon are stored as longs. This prints them as decimal degrees.
static void longToDecimal(long lValue, char *szBuff) {
    long lWhole = lValue / COORD_SCALING_FACTOR;
    long lRemove = lWhole * COORD_SCALING_FACTOR;
    long lFraction = lValue - lRemove;
    if (0 > lFraction) {
        lFraction *= -1L;
    }
    sprintf(szBuff, "%ld.%05ld", lWhole, lFraction);
}

void setup() {
    int status;
    
    // Initialize LCD with number of columns and rows. 
    status = lcd.begin(LCD_COLS, LCD_ROWS);
    if (status) {
        status = -status; // onvert status value to positive number.
        // Failure: blink error code using the onboard LED if possible.
        hd44780::fatalError(status); // Does not return.
    }

    // Initalization was successful, the backlight should be on now.
    // Wipe the display from any previous output.
    lcd.clear();

    // Create custom characters for logo.
    lcd.createChar(0, logo); 
    lcd.createChar(1, logo + 8); 
    // Print banner to the LCD.
    lcd.print(BANNER);
    lcd.setCursor(12, 0);
    lcd.write(0);
    lcd.write(1);

#if defined (DEBUG_PORT)
    {
        DEBUG_PORT.begin(9600);
        DEBUG_PORT.flush();
        DEBUG_PORT.println(F(BANNER));
        DEBUG_PORT.println(F(VERSION));
        DEBUG_PORT.println(F(COPYRIGHT));
        DEBUG_PORT.print(F("TinyGPS++ version "));
        DEBUG_PORT.println(TinyGPSPlus::libraryVersion());
    }
#endif // DEBUG_PORT
    
    gpsPort.begin(9600);
    delay(1000);
}

void loop() {
    static uint8_t iPrevSecond = 255;
    while (gpsPort.available()) {
        int c = gpsPort.read();
        if (gps.encode(c)) {
            // Fix is ready.
            lcd.setCursor(0, 2);
            if (!gps.time.isValid() || !gps.date.isValid()) {
                lcd.print(INVALID);
            } else {
                uint8_t iSecond = gps.time.second();
                if (gps.time.second() != iPrevSecond) {
                    char szTimestamp[22];
                    sprintf(szTimestamp, "%04d-%02d-%02d %02d:%02d:%02dZ",
                            gps.date.year(), gps.date.month(), gps.date.day(), 
                            gps.time.hour(), gps.time.minute(), iSecond);
                    lcd.print(szTimestamp);
                    iPrevSecond = iSecond;
                    #if defined (DEBUG_PORT)
                    {
                        DEBUG_PORT.print(gps.location.lat(), 5);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.print(gps.location.lng(), 5);
                        DEBUG_PORT.print(",");
                        // Number of satellites is not working in TinyGPSPlus.
                        DEBUG_PORT.print(gps.satellites.value());
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.println(szTimestamp);
                    }
                    #endif // DEBUG_PORT
                }
            }
        }
    }
}
