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
 * As the serial is configured, this sketch is for a Leonardo or Pro Micro board.
 * Use NeoSWSerial for Uno boards.
 * 
 */
#define PROGRAM "Satellogger"
#define CALLSIGN "K4ILG"
#define VERSION "0.8.4"
#define COPYRIGHT "(c) 2020 - Donald Purnhagen"
#define INVALID "Searching...        "

#include <Wire.h>                          // wires
#include <hd44780.h>                       // hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c backpack
#include <NMEAGPS.h>                       // gps parser
// Use software serial on the Uno which lacks harware serial when using USB.
//#include <NeoSWSerial.h>                 // serial port for gps

// For serial monitor debugging.
// Comment out the define to disable debugging.
#define DEBUG_PORT Serial

// GPS serial port.
//NeoSWSerial gpsPort(10, 9); // GPS TX to pin 8, GPS RX to pin 9
#define GPS_PORT Serial1

// LCD display address and geometry.
const int LCD_ADDRESS = 0x27;
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

// Other pins.
const int BUZZER_PIN = 9;

// Configuration structure.
typedef struct sl_config_s {
     bool bStatus;
     bool bUSB;
     int iPipFreq;
     int iPipLenShort;
     int iPipLenLong;
     byte iLogoPos;
     char szHeaderL[LCD_COLS];
     char szHeaderR[LCD_COLS];
} sl_config_t;

// My best attempt at a satellite logo.
static byte logo[] = {
    // Char 0
    B00000, B10010, B10010, B10010, B01001, B01000, B00110, B00001,
    // Char 1
    B10010, B01101, B01110, B01110, B00001, B11000, B00000, B11000
};

// Global variables.
static NMEAGPS gps;
static hd44780_I2Cexp lcd(LCD_ADDRESS); // Declare lcd object at fixed i2c address.
static sl_config_t sl_config;

void setup() {
    int iStatus;

    // Initialize the configuration.
    sl_config.bStatus = true;
    sl_config.bUSB = true;
    sl_config.iPipFreq = 1000;
    sl_config.iPipLenShort = 100;
    sl_config.iPipLenLong = 500;
    sl_config.iLogoPos = 12;
    strcpy(sl_config.szHeaderL, PROGRAM);
    strcpy(sl_config.szHeaderR, CALLSIGN);
    
    // Initialize LCD with number of columns and rows. 
    iStatus = lcd.begin(LCD_COLS, LCD_ROWS);
    if (0 != iStatus) {
        iStatus = -iStatus; // onvert status value to positive number.
        // Failure: blink error code using the onboard LED if possible.
        hd44780::fatalError(iStatus); // Does not return.
    }

    // Initalization was successful, the backlight should be on now.
    // Wipe the display from any previous output.
    lcd.clear();

    // Create custom characters for logo.
    lcd.createChar(0, logo); 
    lcd.createChar(1, logo + 8); 
    
    // Print banner to the LCD.
    lcd.print(sl_config.szHeaderL);
    if (0 <= sl_config.iLogoPos) {
        lcd.setCursor(sl_config.iLogoPos, 0);
        lcd.write(0);
        lcd.write(1);
    }
    if (NULL != sl_config.szHeaderR) {
        size_t iLen = strlen(sl_config.szHeaderR);
        if (0 < iLen) {
            int iPos = LCD_COLS - iLen;
            if (0 > iPos) {
                iPos = 0;
            }
            lcd.setCursor(iPos, 0);
            lcd.print(sl_config.szHeaderR);
        }
    }

    #if defined (DEBUG_PORT)
    if (sl_config.bUSB) {
        DEBUG_PORT.begin(9600);
        while (!DEBUG_PORT); // Wait for it...
        DEBUG_PORT.flush();
        DEBUG_PORT.print(F(PROGRAM));
        DEBUG_PORT.print(" ");
        DEBUG_PORT.println(F(CALLSIGN));
        DEBUG_PORT.println(F(VERSION));
        DEBUG_PORT.println(F(COPYRIGHT));
    }
    #endif // DEBUG_PORT

    GPS_PORT.begin(9600);
    delay(1000);
}

void loop() {
    static int iSatCount = 0xffff;
    static int iPrevSecond = 0xffff;
    while (gps.available(GPS_PORT)) {
        gps_fix fix = gps.read();
        if (fix.valid.location) {
            // Fix is ready.
            if (0 < fix.satellites) {
                iSatCount = fix.satellites;
            }
            lcd.setCursor(0, 2);
            if (!fix.valid.time || !fix.valid.date) {
                lcd.print(INVALID);
            } else {
                uint8_t iSecond = fix.dateTime.seconds;
                uint8_t iMinute = fix.dateTime.minutes;
                
                if (fix.dateTime.seconds != iPrevSecond) {
                    char szTimestamp[22];

                    /* 
                     * My favorite aspect of this program, The Pips!
                     * Inspired by "Atomic Clock" by Timo Partl.
                     * Shameslessly stolen from the BBC.
                     */
                    if (0 < sl_config.iPipLenShort && 59 == iMinute && 54 < iSecond) {
                        tone(BUZZER_PIN, sl_config.iPipFreq, sl_config.iPipLenShort);
                    } else if (0 < sl_config.iPipLenLong && 0 == iMinute && 0 == iSecond) {
                        tone(BUZZER_PIN, sl_config.iPipFreq, sl_config.iPipLenLong);
                    }
                    
                    sprintf(szTimestamp, "%04d-%02d-%02d %02d:%02d:%02dZ",
                            2000 + fix.dateTime.year, fix.dateTime.month, fix.dateTime.date, 
                            fix.dateTime.hours, iMinute, iSecond);
                    lcd.print(szTimestamp);
                    
                    if (sl_config.bStatus) {
                        size_t iPos = 0;
                        lcd.setCursor(0, 3);
                        iPos += lcd.print(fix.latitude(), 3);
                        iPos += lcd.print(" ");
                        iPos += lcd.print(fix.longitude(), 3);
                        iPos += lcd.print(" ");
                        iPos += lcd.print(iSatCount);
                        while (LCD_COLS > iPos) {
                            iPos += lcd.print(" ");
                        }
                    }
                    
                    iPrevSecond = iSecond;

                    #if defined (DEBUG_PORT)
                    if (DEBUG_PORT) {
                        DEBUG_PORT.print(fix.latitude(), 5);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.print(fix.longitude(), 5);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.print(szTimestamp);
                        DEBUG_PORT.print(",");
                        DEBUG_PORT.println(iSatCount);
                    }
                    #endif // DEBUG_PORT
                }
            }
        }
    }
}
