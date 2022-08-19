#pragma once

// Imports
#include "config.h"
#include "cstring"
#include "encoder.h"
#include "main.h"
#include <algorithm>
#include "stm32f1xx_ll_gpio.h"
#include "oledfont.h"
#include "flash.h"

// Enumerations for the different menu values
typedef enum {
    MOTOR_DATA,
    TOP_LEVEL,
    SUBMENUS,
    WARNING
} MENU_DEPTH;

typedef enum {
    CALIBRATION,
    CURRENT,
    MICROSTEP,
    ENABLE_LOGIC,
    DIR_LOGIC,
    DIP_SWITCHES,
    SAVED_DATA
} SUBMENU;

// Variables
extern const char* topLevelMenuItems[];
extern const uint8_t topLevelMenuLength;
extern const char* submenuItems[];
extern const uint8_t submenuCount;
class StepperMotor;

// Functions (for high level displaying)
void showBootscreen();
void updateDisplay();
void displayMotorData();
void displayWarning(String firstLine, String secondLine, String thirdLine, bool updateScreen = true);
void selectMenuItem();
void moveCursor();
void exitCurrentMenu();
MENU_DEPTH getMenuDepth();
String padNumber(float number, uint8_t leadingZeroCount, uint8_t followingZeroCount);
float roundToPlace(float number, uint8_t place);
char * int2bin(unsigned int x);

// BTT Display definitions
typedef enum {
    COMMAND,
    DATA
} OLED_MODE;

typedef enum {
    BLACK,
    WHITE
} OLED_COLOR;

#define MAX_CHAR_POSX 122
#define MAX_CHAR_POSY 58
#define OB_SIZE 32
#define LINE_HEIGHT 16

// Low level OLED commands
void initOLED();
void writeOLEDByte(uint8_t data, OLED_MODE mode);
void writeOLEDOn();
void writeOLEDOff();
void writeOLEDBuffer();
void setOLEDPixel(uint8_t x, uint8_t y, OLED_COLOR color);
void fillOLED(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_COLOR color, bool updateScreen = true);
void writeOLEDChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t fontSize, OLED_COLOR color, bool updateScreen = true);
void writeOLEDNum(uint8_t x, uint8_t y, uint32_t number, uint8_t len, uint8_t fontSize, bool updateScreen = true);
void writeOLEDString(uint8_t x, uint8_t y, const char *p, bool updateScreen = true, bool allowOverflow = false);
void writeOLEDString(uint8_t x, uint8_t y, String string, bool updateScreen = true, bool allowOverflow = false);
void clearOLED();
