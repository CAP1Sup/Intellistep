// Import the config (needed for the USE_OLED define)
#include "config.h"

// Only build if needed
#ifdef ENABLE_OLED

// Import libraries
#include "oled.h"
#include "cstring"

// Screen data is stored in an array. Each set of 8 pixels is written one by one
// Each set of 8 is stored as an integer. The panel is written to horizontally, then vertically
// Array stores the values in width - height format with bits stored along vertical rows
uint8_t OLEDBuffer[128][8];
char outBuffer[OB_SIZE];

// The index of the current top level menu item
SUBMENU submenu = CALIBRATION;
SUBMENU lastSubmenu = CALIBRATION;

// The current value of the cursor
uint16_t currentCursorIndex = 0;

// The current menu depth
MENU_DEPTH menuDepth = MOTOR_DATA;
MENU_DEPTH lastMenuDepth = MOTOR_DATA;

// Displays the bootscreen
void showBootscreen() {
    writeOLEDString(0, 0,               F("Intellistep"), false);
    writeOLEDString(0, LINE_HEIGHT * 2, F("Version: "), false);
    writeOLEDString(0, LINE_HEIGHT * 3, VERSION_STRING, true);
}


// Function for displaying relevant data on the OLED panel, such as motor data or menu data
void updateDisplay() {

    // Decide on the current depth of the menu
    switch(menuDepth) {
        case MOTOR_DATA:
            // Not actually in the menu, just displaying motor data for now
            displayMotorData();
            break;

        case TOP_LEVEL:
            // In the top level of the menu. Make sure that the set top level menu index is within the range of the menu length
            // Values must first be determined using the mod function in order to prevent overflow errors
            // Then the strings are converted into character arrays by giving the address of the first character in the string
            clearOLED();
            writeOLEDString(0,  0, "->", false);
            writeOLEDString(25, 0,               &submenuItems[(submenu)     % submenuCount][0], false);
            writeOLEDString(25, LINE_HEIGHT * 1, &submenuItems[(submenu + 1) % submenuCount][0], false);
            writeOLEDString(25, LINE_HEIGHT * 2, &submenuItems[(submenu + 2) % submenuCount][0], false);
            writeOLEDString(25, LINE_HEIGHT * 3, &submenuItems[(submenu + 3) % submenuCount][0], true);
            break;

        case SUBMENUS:
            // We should be in a sub menu, this is where we have to figure out which submenu that should be
            switch(submenu) {
                case CALIBRATION:
                    // In the first menu, the calibration one. No need to do anything here, besides maybe displaying an progress bar or PID values (later?)
                    clearOLED();
                    writeOLEDString(0, 0,               F("Are you sure?"), false);
                    writeOLEDString(0, LINE_HEIGHT * 1, F("Press select"), false);
                    writeOLEDString(0, LINE_HEIGHT * 2, F("to confirm"), true);
                    break;

                case CURRENT:
                    // In the second menu, the motor mAs. This is dynamically generated and has increments every 100 mA from 0 mA (testing only) to 3500 mA
                    clearOLED();

                    // Constrain the current setting within 0 and the maximum current
                    if (currentCursorIndex > (uint16_t)MAX_RMS_BOARD_CURRENT / 100) {

                        // Loop back to the start of the list
                        currentCursorIndex = 0;
                    }

                    // Write the pointer
                    writeOLEDString(0, 0, F("->"), false);

                    // Write each of the strings
                    for (uint8_t stringIndex = 0; stringIndex <= 3; stringIndex++) {

                        // Check to make sure that the current isn't out of range of the max current
                        if ((currentCursorIndex + stringIndex) * 100 <= (uint16_t)MAX_RMS_BOARD_CURRENT) {

                            // Value is in range, display the current on that line
                            snprintf(outBuffer, OB_SIZE, "%dmA", (int)((currentCursorIndex + stringIndex) * 100));
                            writeOLEDString(25, stringIndex * LINE_HEIGHT, outBuffer, false);
                        }
                        // else {
                            // Value is out of range, display a blank line for this line
                        // }
                    }

                    // Write out the info to the OLED display
                    writeOLEDBuffer();
                    break;

                case MICROSTEP:
                    // In the microstep menu, this is also dynamically generated. Get the current stepping of the motor, then display all of the values around it
                    clearOLED();
                    writeOLEDString(0, 0, F("->"), false);

                    // Loop the currentCursor index back if it's out of range
                    if (currentCursorIndex > log2(MAX_MICROSTEP_DIVISOR)) {
                        currentCursorIndex = log2(MIN_MICROSTEP_DIVISOR);
                    }
                    else if (currentCursorIndex < log2(MIN_MICROSTEP_DIVISOR)) {

                        // Make sure that the cursor index is in valid range
                        currentCursorIndex = log2(MIN_MICROSTEP_DIVISOR);
                    }

                    // Write each of the strings
                    for (uint8_t stringIndex = 0; stringIndex <= 3; stringIndex++) {

                        // Check to make sure that the current isn't out of range of the max current
                        if (pow(2, currentCursorIndex + stringIndex) <= MAX_MICROSTEP_DIVISOR) {

                            // Value is in range, display the current on that line
                            snprintf(outBuffer, OB_SIZE, "1/%dth", (int)pow(2, currentCursorIndex + stringIndex));
                            writeOLEDString(25, stringIndex * LINE_HEIGHT, outBuffer, false);
                        }
                        // else {
                            // Value is out of range, display a blank line for this line
                        // }
                    }

                    // Write out the data to the OLED
                    writeOLEDBuffer();
                    break;

                case ENABLE_LOGIC:
                    // In the enable logic menu, a very simple menu. Just need to invert the displayed state
                    // Clear the OLED
                    clearOLED();

                    // Title
                    writeOLEDString(0, 0, F("Enable logic:"), false);

                    // Write the string to the screen
                    if (currentCursorIndex % 2 == 0) {

                        // The index is even, the logic is inverted
                        writeOLEDString(0, (3 * LINE_HEIGHT / 2), F("Inverted"), true);
                    }
                    else {
                        // Index is odd, the logic is normal
                        writeOLEDString(0, (3 * LINE_HEIGHT / 2), F("Normal"), true);
                    }
                    break;

                case DIR_LOGIC:
                    // Another easy menu, just the direction pin. Once again, just need to invert the state
                    clearOLED();

                    // Title
                    writeOLEDString(0, 0, F("Dir logic:"), false);

                    // Write the string to the screen
                    if (currentCursorIndex % 2 == 0) {

                        // The index is even, the logic is inverted
                        writeOLEDString(0, (3 * LINE_HEIGHT / 2), F("Inverted"), true);
                    }
                    else {
                        // Index is odd, the logic is normal
                        writeOLEDString(0, (3 * LINE_HEIGHT / 2), F("Normal"), true);
                    }
                    break;

                case SAVED_DATA:
                    // Clear the screen of old content
                    clearOLED();

                    // Write cursor to line index based on cursor mod
                    writeOLEDString(0, (currentCursorIndex % 3) * LINE_HEIGHT, F("->"), false);

                    // Write out the options
                    writeOLEDString(25, 0,               F("Save"), false);
                    writeOLEDString(25, LINE_HEIGHT,     F("Load"), false);
                    writeOLEDString(25, LINE_HEIGHT * 2, F("Wipe"), true);

                    // All done, break
                    break;
            } // Submenu switch
            break;

            case WARNING:
                // Nothing to do
                break;

    } // Main switch

    // Save the last set value (for display functions that don't need to clear the OLED all of the time)
    lastMenuDepth = menuDepth;
    lastSubmenu = submenu;

} // Display menu function


// Gets the latest parameters from the motor and puts them on screen
void displayMotorData() {

    // Clear the old menu off of the display
    if (lastMenuDepth != 0) {
        clearOLED();
    }

    // Get the current absolute angle
    double currentAbsAngle = motor.encoder.getAbsoluteAngle();

    // RPM of the motor (RPM is capped at 2 decimal places)
    #ifdef ENCODER_SPEED_ESTIMATION

    // Check if the motor RPM can be updated. The update rate of the speed must be limited while using encoder speed estimation
    if (motor.encoder.sampleTimeExceeded()) {
        snprintf(outBuffer, OB_SIZE, "RPM:% 11.3f", motor.getEstimRPM(currentAbsAngle));
    }

    #else // ! ENCODER_SPEED_ESTIMATION

    // No need to check, just sample it
    snprintf(outBuffer, OB_SIZE, "RPM:%11.3f", motor.getEncoderRPM());

    #endif // ! ENCODER_SPEED_ESTIMATION

    snprintf(outBuffer, OB_SIZE, "sStp:% 10ld", motor.getSoftStepCNT());

    writeOLEDString(0, 0, outBuffer, false);

    // Angle error
    snprintf(outBuffer, OB_SIZE, "Err:% 11.2f", motor.getAngleError(currentAbsAngle));
    writeOLEDString(0, LINE_HEIGHT, outBuffer, false);

    // Current angle of the motor
    snprintf(outBuffer, OB_SIZE, "Deg:% 11.2f", currentAbsAngle);
    writeOLEDString(0, LINE_HEIGHT * 2, outBuffer, false);

    // Temp of the encoder (close to the motor temp)
    snprintf(outBuffer, OB_SIZE, "Temp:%8.1f C", motor.encoder.getTemp());

    //snprintf(outBuffer, OB_SIZE, "dStp:% 10ld", motor.getDesiredStep());
    //snprintf(outBuffer, OB_SIZE, "hStp:% 10ld", motor.getHardStepCNT());
    snprintf(outBuffer, OB_SIZE, "h-s: % 10ld", motor.getHardStepCNT()-motor.getSoftStepCNT());

    writeOLEDString(0, LINE_HEIGHT * 3, outBuffer, true);
}


// Display an error message
void displayWarning(String firstLine, String secondLine, String thirdLine, bool updateScreen) {
    clearOLED();
    writeOLEDString(0, 0,               firstLine,  false);
    writeOLEDString(0, LINE_HEIGHT,     secondLine, false);
    writeOLEDString(0, LINE_HEIGHT * 2, thirdLine,  false);
    writeOLEDString(0, LINE_HEIGHT * 3, F("Back to exit"), updateScreen);

    // Save the last menu used (for returning later), then move to the new index
    lastMenuDepth = menuDepth;
    menuDepth = WARNING;
}


// Function for moving the cursor up
void selectMenuItem() {

    // Go down to the top level
    if (menuDepth == MOTOR_DATA) {
        menuDepth = TOP_LEVEL;
        updateDisplay();
    }

    // Go down to the submenus and deterimine the starting index
    else if (menuDepth == TOP_LEVEL) {

        // Set that we're moving into the submenus
        menuDepth = SUBMENUS;

        // Check the submenus available
        switch(submenu % submenuCount) {

            case CALIBRATION:
                // Nothing to see here, just moving into the calibration
                motor.calibrate();

                // Board is restarted, so no need to do anything here

            case CURRENT:
                // Motor mAs. Need to get the current motor mAs, then convert that to a cursor value
                #ifndef ENABLE_DYNAMIC_CURRENT
                    currentCursorIndex = constrain(round(motor.getRMSCurrent() / 100), 0, (uint16_t)MAX_RMS_BOARD_CURRENT);
                #endif

                // Enter the menu
                menuDepth = SUBMENUS;
                break;

            case MICROSTEP:
                // Motor microstepping. Need to get the current microstepping setting, then convert it to a cursor value
                currentCursorIndex = log2(motor.getMicrostepping());

                // Enter the menu
                menuDepth = SUBMENUS;
                break;

            case ENABLE_LOGIC:
                // Get if the enable pin is inverted
                if (motor.getEnableInversion()) {
                    // Value is true already, therefore start at "true"
                    currentCursorIndex = 0;
                }
                else {
                    // Otherwise set the value to false to start
                    currentCursorIndex = 1;
                }

                // Enter the menu
                menuDepth = SUBMENUS;
                break;

            case DIR_LOGIC:
                // Get if the direction pin is inverted
                if (motor.getReversed()) {
                    currentCursorIndex = 0;
                }
                else {
                    currentCursorIndex = 1;
                }

                // Enter the menu
                menuDepth = SUBMENUS;
                break;

            case SAVED_DATA:
                // Just set the cursor to zero, no saving of last position
                currentCursorIndex = 0;

                // Enter the menu
                menuDepth = SUBMENUS;
                break;
        }

        // Update the display with the new information
        updateDisplay();
    }

    // If we're in certain menus, the cursor should start at their current value
    else if (menuDepth == SUBMENUS) {
        // Cursor settings are only needed in the submenus

        // Check the submenus available
        switch(submenu % submenuCount) {

            case CALIBRATION:
                // Nothing to see here, everything is handled on the initial press.
                break;

            case CURRENT: {
                // Motor mAs. Need to get the cursor value, then convert that to current value
                // Get the set value
                uint16_t rmsCurrentSetting = CURRENT_MENU_INCREMENT * currentCursorIndex;

                // Check to see if the warning needs flagged
                if (rmsCurrentSetting % (uint16_t)MAX_RMS_BOARD_CURRENT >= (uint16_t)WARNING_RMS_CURRENT) {

                    // Set the display to output the warning
                    menuDepth = WARNING;

                    // Actually draw the warning
                    displayWarning(F("Large current"), F("set. Are you"), F("sure?"), true);
                }
                else {
                    // Set the value
                    #ifndef ENABLE_DYNAMIC_CURRENT
                        motor.setRMSCurrent(rmsCurrentSetting % (uint16_t)MAX_RMS_BOARD_CURRENT);
                    #endif

                    // Exit the menu
                    menuDepth = MENU_RETURN_LEVEL;
                }

                // We're done here, time to head out
                break;
            }

            case MICROSTEP: {
                // Motor microstepping. Need to get the current microstepping setting, then convert it to a cursor value. Needs to be -2 because the lowest index, 1/4 microstepping, would be at index 0
                // Get the set value
                uint8_t microstepSetting = pow(2, (currentCursorIndex % MICROSTEP_INTERVAL_CNT));

                // Check to see if the warning needs flagged
                if (microstepSetting >= WARNING_MICROSTEP) {

                    // Set the display to output the warning
                    menuDepth = WARNING;

                    // Actually draw the warning
                    displayWarning(F("Large divisor"), F("set. Are you"), F("sure?"), true);
                }
                else {
                    // Set the value
                    motor.setMicrostepping(microstepSetting);

                    // Exit the menu
                    menuDepth = MENU_RETURN_LEVEL;
                }

                // We're done here, time to head out
                break;
            }

            case ENABLE_LOGIC:
                // Get if the enable pin is inverted
                if (currentCursorIndex % 2 == 0) {

                    // The index is even, the logic is inverted
                    motor.setEnableInversion(true);
                }
                else {
                    // Index is odd, the logic is normal
                    motor.setEnableInversion(false);
                }

                // Exit the menu
                menuDepth = MENU_RETURN_LEVEL;
                break;

            case DIR_LOGIC:
                // Get if the direction pin is inverted
                if (currentCursorIndex % 2 == 0) {

                    // The index is even, the direction is inverted
                    motor.setReversed(true);
                }
                else {
                    // Index is odd, the direction is normal
                    motor.setReversed(false);
                }

                // Exit the menu
                menuDepth = MENU_RETURN_LEVEL;
                break;

            case SAVED_DATA:
                // Determine which routine to run based on the division of the cursor
                uint8_t cursorMod = currentCursorIndex % 3;
                if (cursorMod == 0) {
                    // First index, the save parameters, then return
                    saveParameters();
                    menuDepth = MENU_RETURN_LEVEL;
                }
                else if (cursorMod == 1) {
                    // Second index, load the parameters
                    loadParameters();
                    menuDepth = MENU_RETURN_LEVEL;
                }
                else {
                    // We must be on the third index, wipe the parameters
                    wipeParameters();
                    // No need to return here, the processor reboots
                }
        }

        // Update the display with the new menu
        updateDisplay();
    }

    // Warning level
    else if (menuDepth == WARNING) {

        // Check which menu was previously displayed
        switch(submenu) {

            // Need to set the current
            case CURRENT:

                // Calculate the setting from the cursor index
                #ifndef ENABLE_DYNAMIC_CURRENT
                    motor.setRMSCurrent((100 * currentCursorIndex) % (uint16_t)MAX_RMS_BOARD_CURRENT);
                #endif

            // Need to set the microstep
            case MICROSTEP:

                // Set the value
                motor.setMicrostepping(pow(2, (currentCursorIndex % MICROSTEP_INTERVAL_CNT)));

            default:
                // Nothing to do here, just move on
                break;
        }

        // Set the current menu to the last menu used
        menuDepth = MENU_RETURN_LEVEL;

        // Update the display
        updateDisplay();
    }
}


// Function for moving the cursor down
void moveCursor() {

    // If we're on the motor display menu, do nothing for now
    if (menuDepth == MOTOR_DATA) {
        // Do nothing (maybe add a feature later?)
    }
    else if (menuDepth == TOP_LEVEL) {
        // We're in the top level menu, change the topLevelMenuIndex (as long as we haven't exceeded the length of the list)
        if (submenu + 3 > submenuCount) {
            submenu = SUBMENU(0);
        }
        else {
            submenu = SUBMENU(submenu + 1);
        }
    }
    else if (menuDepth == SUBMENUS) {
        // We are in the submenu, increment the cursor index (submenus handle the display themselves)
        currentCursorIndex++;
    }

    // Update the display
    updateDisplay();
}


// Function for exiting the current menu
void exitCurrentMenu() {

    // Go up in the menu index if we're not already at the motor data screen
    if (menuDepth > (uint8_t)MOTOR_DATA) {
        menuDepth = MENU_DEPTH(menuDepth - 1);
    }

    // Update the display to match
    updateDisplay();
}

// Returns the depth of the menu (helpful for watching the select button)
MENU_DEPTH getMenuDepth() {
    return menuDepth;
}


// A list of all of the top level menu items
const char* submenuItems[] = {
    "Calibrate",
    "Motor mA",
    "Microstep",
    "En Logic",
    "Dir. Logic",
    "Saved config",
    ""
};

// Length of the list of top menu items (found by dividing the length of the list by how much space a single element takes up)
const uint8_t submenuCount = sizeof(submenuItems) / sizeof(submenuItems[0]);

// Sets up the OLED panel
void initOLED() {

    // Set all of the menu values back to their defaults (for if the screen needs to be reinitialized)
    submenu = CALIBRATION;
    currentCursorIndex = 0;

    // Enable the A and B GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Clear the state of pin A8 (pin states are 15, 14, 13, 12, 11, 10, 9, and 8 in groups of 4)
    GPIOA -> CRH &= 0b11111111111111111111111111110000;

    // Set that pin A8 should be a general purpose output
    GPIOA -> CRH |= 0b0011;

    // Clear the settings of pins B12-15
  	GPIOB -> CRH &= 0b1111111111111111;

    // Write that pins B12-15 should be general purpose outputs
  	GPIOB -> CRH |= 0b00110011001100110000000000000000;

	GPIO_WRITE(OLED_RST_PIN, HIGH);
	writeOLEDByte(0xAE, COMMAND);//
	writeOLEDByte(0xD5, COMMAND);//
	writeOLEDByte(80,   COMMAND);  //[3:0],;[7:4],
	writeOLEDByte(0xA8, COMMAND);//
	writeOLEDByte(0X3F, COMMAND);//(1/64)
	writeOLEDByte(0xD3, COMMAND);//
	writeOLEDByte(0X00, COMMAND);//

	writeOLEDByte(0x40, COMMAND);// [5:0],.

	writeOLEDByte(0x8D, COMMAND);//
	writeOLEDByte(0x14, COMMAND);///
	writeOLEDByte(0x20, COMMAND);//
	writeOLEDByte(0x02, COMMAND);//[1:0],;
	writeOLEDByte(0xA1, COMMAND);//,bit0:0,0->0;1,0->127;
	writeOLEDByte(0xC0, COMMAND);//;bit3:0,;1, COM[N-1]->COM0;N:
	writeOLEDByte(0xDA, COMMAND);//
	writeOLEDByte(0x12, COMMAND);//[5:4]

	writeOLEDByte(0x81, COMMAND);//
	writeOLEDByte(0xEF, COMMAND);//1~255;
	writeOLEDByte(0xD9, COMMAND);//
	writeOLEDByte(0xf1, COMMAND);//[3:0],PHASE 1;[7:4],PHASE 2;
	writeOLEDByte(0xDB, COMMAND);//
	writeOLEDByte(0x30, COMMAND);//[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;
	writeOLEDByte(0xA4, COMMAND);//;bit0:1,;0,;
	writeOLEDByte(0xA6, COMMAND);//;bit0:1,;0,
	writeOLEDByte(0xAF, COMMAND);//
	delay(100);
	clearOLED();
}


// Write a single byte to the LCD panel
void writeOLEDByte(uint8_t data, OLED_MODE mode) {

    // Write the current mode and enable the screen's communcation
	GPIO_WRITE(OLED_RS_PIN, (uint8_t)mode);
	GPIO_WRITE(OLED_CS_PIN, LOW);

    // Write each bit of the byte
	for(uint8_t i = 0; i < 8; i++) {

        // Prevent the screen from reading the data in
        GPIO_WRITE(OLED_SCLK_PIN, LOW);

        // If the bit is 1 (true)
		if(data & 0x80) {

            // Write true
            GPIO_WRITE(OLED_SDIN_PIN, HIGH);
        }
		else {
            // Write false
            GPIO_WRITE(OLED_SDIN_PIN, LOW);
        }

        // Signal that the data needs to be sampled again
		GPIO_WRITE(OLED_SCLK_PIN, HIGH);

        // Shift all of the bits so the next will be read
		data <<= 1;
	}

    // Disable the screen's communication
    GPIO_WRITE(OLED_CS_PIN, HIGH);
    GPIO_WRITE(OLED_RS_PIN, HIGH);
}


// Turn the display on
void writeOLEDOn() {
	writeOLEDByte(0X8D, COMMAND);  //SET
	writeOLEDByte(0X14, COMMAND);  //DCDC ON
	writeOLEDByte(0XAF, COMMAND);  //DISPLAY ON
}


// Turn the display off
void writeOLEDOff() {
	writeOLEDByte(0X8D, COMMAND);  //SET
	writeOLEDByte(0X10, COMMAND);  //DCDC OFF
	writeOLEDByte(0XAE, COMMAND);  //DISPLAY OFF
}


// Write the entire OLED buffer to the screen
void writeOLEDBuffer() {

    // Loop through each of the vertical lines
	for(uint8_t vertIndex = 0; vertIndex < 8; vertIndex++) {

		// Specify the line that is being written to
        writeOLEDByte(0xb0 + vertIndex, COMMAND);
		writeOLEDByte(0x00, COMMAND);
		writeOLEDByte(0x10, COMMAND); // DCDC OFF

        // Loop through the horizontal lines
		for(uint8_t horzIndex = 0; horzIndex < 128; horzIndex++) {
		    writeOLEDByte(OLEDBuffer[horzIndex][vertIndex], DATA);
        }
	}
}


// Wipes the output buffer, then writes the zeroed array to the screen
void clearOLED() {

    // Set all of the values in the buffer to 0
	memset(OLEDBuffer, 0X00, sizeof(OLEDBuffer));

    // Push the values to the display
	writeOLEDBuffer();
}


// Writes a point on the buffer
void setOLEDPixel(uint8_t x, uint8_t y, OLED_COLOR color) {

    // Create variables for later use
	uint8_t yPos;

    // Exit if the function would be writing to an index that is off of the screen
	if (x > 127 || y > 63) {
        return;
    }

    // Calculate the y position
	yPos = 7 - (y / 8);

    // Write white pixels if the point isn't inverted
	if(color) {
        OLEDBuffer[x][yPos] |= (1<<(7-(y%8)));
    }

    // Otherwise, write black pixels to the point
	else {
        OLEDBuffer[x][yPos] &= ~(1<<(7-(y%8)));
    }
}


// Fill an area of pixels, starting at x1 or y1 to x2 or y2
void fillOLED(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_COLOR color, bool updateScreen) {

    // Loop through all of the X points
	for(uint8_t x = x1; x <= x2; x++) {

        // Loop through all of the Y points
		for(uint8_t y = y1; y <= y2; y++) {
            setOLEDPixel(x, y, color);
        }
	}

    // Write the buffer to the screen if specified
    if (updateScreen) {
	    writeOLEDBuffer();
    }
}


// Writes a characer to the display
void writeOLEDChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t fontSize, OLED_COLOR color, bool updateScreen) {

    // Create the variables to be used
    uint8_t pixelData;
	uint8_t y0 = y;

    // Correct the character index
	chr = chr - ' ';

    // Loop through the data of each of the pixels of the character
    for (uint8_t pixelIndex = 0; pixelIndex < fontSize; pixelIndex++) {

        // Use the appropriate font data
		if (fontSize == 12) {
            pixelData = OLED_1206_Font[chr][pixelIndex];
        }
		else {
            pixelData = OLED_1608_Font[chr][pixelIndex];
        }

        // Loop through the bits, setting them on the buffer array
        for(uint8_t bitIndex = 0; bitIndex < 8; bitIndex++) {

            // Check if the pixel should be inverted (as compared to what was set)
			if (pixelData & 0x80) {
                setOLEDPixel(x, y, color);
            }
			else {
                setOLEDPixel(x, y, OLED_COLOR(!color));
            }

            // Shift all of the bits to the left, bringing the next bit in for the next cycle
			pixelData <<= 1;

            // Move down in the y direction so two pixels don't overwrite eachother
			y++;

            // When the character vertical's vertical column is filled, move to the next one
			if((y-y0) == fontSize) {
				y = y0;
				x++;
				break;
			}
		}
    }

    // Update the screen if desired
    if (updateScreen) {
        writeOLEDBuffer();
    }

}


// Writes a number to the OLED display
void writeOLEDNum(uint8_t x, uint8_t y, uint32_t number, uint8_t len, uint8_t fontSize, bool updateScreen) {

    // Create variables
	uint8_t digit;
	uint8_t enshow = 0;

    // Loop through each of the digits
	for (uint8_t digitIndex = 0; digitIndex < len; digitIndex++) {

        // Choose the digit out of the number
		digit = (number / (uint8_t)pow(10, len - (digitIndex + 1))) % 10;

        // Display the digit if everything is good
		if(enshow == 0 && digitIndex < (len-1)) {
			if(digit == 0) {
				writeOLEDChar(x+(fontSize/2)*digitIndex, y, ' ', fontSize, WHITE, false);
				continue;
			}
            else {
                enshow = 1;
            }
		}

        // Write out the character
	 	writeOLEDChar(x+(fontSize/2)*digitIndex, y, digit + '0', fontSize, WHITE, false);
	}

    // Update the screen if specified
    if (updateScreen) {
        writeOLEDBuffer();
    }
}


// Function to write a string to the screen
void writeOLEDString(uint8_t x, uint8_t y, const char *p, bool updateScreen, bool allowOverflow) {

    // Check if we haven't reached the end of the string
    while(*p != '\0') {

        // Make sure that y doesn't exceed the maximum indexes
        if (y > MAX_CHAR_POSY) {
            break;
        }
        // Check the location of the next character in the x direction
        else if (x + (16 / 2) > MAX_CHAR_POSX) {

            // Check if the display should allow overflow
            if (allowOverflow) {

                // We're over the edge of the screen
                // Move the cursor back and down a line
                x = 0;
                y += LINE_HEIGHT;
            }
            else {
                // Overflow is not allowed, exit the function
                break;
            }
        }

        // Display the character on the screen
        writeOLEDChar(x, y, *p, 16, WHITE, false);

        // Move over by half the font size
        x += (16 / 2);

        // Move to the next character
        p++;
    }

    // Update the screen if specified
    if (updateScreen) {
        writeOLEDBuffer();
    }
}


// Convience function for writing a specific string to the OLED panel
void writeOLEDString(uint8_t x, uint8_t y, String string, bool updateScreen, bool allowOverflow) {
    writeOLEDString(x, y, string.c_str(), updateScreen, allowOverflow);
}


// Converts an integer to a binary string, useful for debugging. From https://forum.arduino.cc/t/convert-from-int-to-bin-to-string/45836/6
char * int2bin(unsigned int x) {
  static char buffer[17];
  for (int i=0; i<16; i++) buffer[15-i] = '0' + ((x & (1 << i)) > 0);
  buffer[16] ='\0';
  return buffer;
}

#endif // ! ENABLE_OLED