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
#define VERSION "0.8.2"
#define COPYRIGHT "(c) 2020 - Donald Purnhagen"
#define INVALID "Searching...        "

#include <Wire.h>                          // wires
#include <hd44780.h>                       // hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander
#include <TinyGPS++.h>
#include <AltSoftSerial.h>                // serial port for gps

// For serial monitor debugging.
// Comment out the define to disable debugging.
#define DEBUG_PORT Serial

// Status on the bottom line of the 4 row display.
// Comment out the define to disable status line.
#define STATUS_LINE

// GPS serial port.
AltSoftSerial gpsPort; // GPS TX to pin 8, GPS RX to pin 9

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
TinyGPSPlus gps;
hd44780_I2Cexp lcd(LCD_ADDRESS); // Declare lcd object at fixed i2c address.

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
    static int iSatCount = 0xffff;
    static int iPrevSecond = 0xffff;
    while (gpsPort.available()) {
        int c = gpsPort.read();
        bool bFix = gps.encode(c);
        if (bFix) {
            // Fix is ready.
            if (0 < gps.satellites.value()) {
                iSatCount = gps.satellites.value();
            }
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
                    
                    #if defined (STATUS_LINE)
                    {
                        size_t iPos = 0;
                        lcd.setCursor(0, 3);
                        iPos += lcd.print(gps.location.lat(), 3);
                        iPos += lcd.print(" ");
                        iPos += lcd.print(gps.location.lng(), 3);
                        iPos += lcd.print(" ");
                        iPos += lcd.print(iSatCount);
                        while (LCD_COLS > iPos) {
                            iPos += lcd.print(" ");
                        }
                    }
                    #endif // STATUS_LINE
                    
                    iPrevSecond = iSecond;
                    #if defined (DEBUG_PORT)
                    {
                        DEBUG_PORT.print(gps.location.lat(), 5);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.print(gps.location.lng(), 5);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.print(iSatCount);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.println(szTimestamp);
                    }
                    #endif // DEBUG_PORT
                }
            }
        }
    }
}
