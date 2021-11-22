// Import the config (needed for the ENABLE_SERIAL or ENABLE_CAN defines)
#include "config.h"

// Only include if the serial or CAN bus is enabled
#if defined(ENABLE_SERIAL) || defined(ENABLE_CAN)

#include "parser.h"

// Parses an entire string for any commands
String parseCommand(String buffer) {

    // Gcode Table
    //  - G90 Absolute positioning // http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g90-g91
    //  - G91 incremental positioning // http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g90-g91
    //  - G0 (ex G0 R1000 A123.45) // http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g0
    //    https://marlinfw.org/docs/gcode/G006.html
    //  - G6 (ex G6 D0 R1000 S1000) - Direct stepping, commands the motor to move a specified number of steps in the specified direction. D is direction (0 for CCW, 1 for CW), R is rate (in Hz), and S is the count of steps to move. Requires `ENABLE_DIRECT_STEPPING`
    //  - M17 (ex M17) - Enables the motor (overrides enable pin)
    //  - M18 / M84 (ex M18 or M84) - Disables the motor (overrides enable pin)
    //  - M93 (ex M93 V1.8 or M93) - Sets the angle of a full step. This value should be 1.8° or 0.9°. If no value is provided, then the current value will be returned.
    //  - M115 (ex M115) - Prints out firmware information, consisting of the version and any enabled features.
    //  - M116 (ex M116 S1 M"A message") - Simple forward command that will forward a message across the CAN bus. Can be used for pinging or allowing a Serial to connect to the CAN network
    //  - M154 (ex M154 S4) - Runs the manual PID tuning interface. Serial is filled with encoder angles. S term specifies the wait time.
    //  - M301 (ex M301 P1 I1 D1 W10 or M301) - Sets or gets the PID values for the motor. W term is the maximum value of the I windup. If no values are provided, then the current values will be returned.
    //  - M303 (ex M303) - Runs an autotune sequence for the PID loop
    //  - M350 (ex M350 V16 or M350) - Sets or gets the microstepping divisor for the motor. This value can be 1, 2, 4, 8, 16, or 32. If no value is provided, then the current microstepping divisor will be returned.
    //  - M352 (ex M352 S1 or M352) - Sets or gets the direction pin inversion for the motor (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
    //  - M353 (ex M353 S1 or M353) - Sets or gets the enable pin inversion for the motor (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
    //  - M354 (ex M354 S1 or M354) - Sets or gets if the motor dip switches were installed incorrectly (reversed) (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
    //  - M355 (ex M355 V1.34 or M355) - Sets or gets the microstep multiplier for the board. Allows to use multiple motors connected to the same mainboard pin, yet have different rates. If no value is provided, then the current value will be returned.
    //  - M356 (ex M356 V1 or M356 VX2 or M356) - Sets or gets the CAN ID of the board. Can be set using the axis character or actual ID. If no value is provided, then the current value will be returned.
    //  - M500 (ex M500) - Saves the currently loaded parameters into flash
    //  - M501 (ex M501) - Loads all saved parameters from flash
    //  - M502 (ex M502) - Wipes all parameters from flash, then reboots the system
    //  - M907 (ex M907 R750, M907 I500) - Sets or gets the RMS(R) or Peak(P) current in mA. If dynamic current is enabled, then the accel(A), idle(I), and/or max(M) can be set or retrieved. If no value is set, then the current RMS current (no dynamic current) or the accel, idle, and max terms (dynamic current) will be returned.

    // ! Check to see if the string contains another set of gcode, if so call the function recursively

    // Check to see if the letter is an M (for mcodes)
    if (parseValue(buffer, 'M') != "-1") {

        // Switch statement the command number
        switch (parseValue(buffer, 'M').toInt()) {

            case 17:
                // M17 (ex M17) - Enables the motor (overrides enable pin)
                motor.setState(FORCED_ENABLED, true);
                return FEEDBACK_OK;

            case 18:
                // M18 / M84 (ex M18 or M84) - Disables the motor (overrides enable pin)
                motor.setState(FORCED_DISABLED, true);
                return FEEDBACK_OK;

            case 84:
                // M18 / M84 (ex M18 or M84) - Disables the motor (overrides enable pin)
                motor.setState(FORCED_DISABLED, true);
                return FEEDBACK_OK;

            case 93: {
                // M93 (ex M93 V1.8 or M93) - Sets the angle of a full step. This value should be 1.8° or 0.9°. This value should be 1.8° or 0.9°. If no value is provided, then the current value will be returned.
                float setValue = parseValue(buffer, 'V').toFloat();
                if (setValue != -1) {

                    // Value is valid, set and return ok
                    motor.setFullStepAngle(setValue);
                    return FEEDBACK_OK;
                }
                else {
                    // No value exists, get and return the current value
                    return String(motor.getFullStepAngle());
                }
            }

            case 115:
                // M115 (ex M115) - Prints out firmware information.
                return FIRMWARE_FEATURE_PRINT;

            #ifdef ENABLE_CAN
            case 116:
                // M116 (ex M116 S1) - Simple forward command that will forward a message across the CAN bus. Can be used for pinging or allowing a Serial to connect to the CAN network
                // The buffer.substring prevents the first M from being read
                txCANString(parseValue(buffer, 'S').toInt(), parseString(buffer.substring(1), 'M'));
            #endif

            #ifdef ENABLE_PID
            case 154: {
                // M154 (ex M154) - Runs the manual PID tuning interface. Serial is filled with encoder angles

                // Read the auto-report interval
                float interval = parseValue(buffer, 'S').toFloat();

                // Sanitize the interval
                if (interval < 0) {
                    return FEEDBACK_NO_VALUE;
                }

                // Print a notice to the user that the PID tuning is starting
                Serial.println("Notice: The manual PID tuning is now starting. To exit, send any serial data.");

                // Wait for the user to read it
                delay(1000);

                // Clear the serial buffer
                while (Serial.available() > 0) {
                    Serial.read();
                }

                // Loop forever, until a new value is sent
                while (!(Serial.available() > 0)) {
                    Serial.println(motor.encoder.getAbsoluteAngleAvg());
                    delay(interval);
                }

                // When all done, the exit is acknowledged
                return FEEDBACK_OK;
            }

            case 301: {
                // M301 (ex M301 P1 I1 D1 or M306) - Sets or gets the PID values for the motor. If no values are provided, then the current values will be returned.
                float pValue =    parseValue(buffer, 'P').toFloat();
                float iValue =    parseValue(buffer, 'I').toFloat();
                float dValue =    parseValue(buffer, 'D').toFloat();
                float maxIValue = parseValue(buffer, 'W').toFloat();
                if (!((pValue == -1) && (iValue == -1) && (dValue == -1))) {

                    // There is at least one valid value, therefore set all of the values
                    if (pValue != -1) {
                        pid.setP(pValue);
                    }
                    if (iValue != -1) {
                        pid.setI(iValue);
                    }
                    if (dValue != -1) {
                        pid.setD(dValue);
                    }
                    if (maxIValue != -1) {
                        pid.setMaxI(maxIValue);
                    }

                    return FEEDBACK_OK;
                }
                else {
                    // No values are included, get and return the current values
                    return ("P: " + String(pid.getP()) + " | I: " + String(pid.getI()) + " | D: " + String(pid.getD()) + " | W: " + String(pid.getMaxI()));
                }
            }

            case 303:
                // M303 (ex M303) - Runs a automatic calibration sequence for the PID loop and encoder
                motor.calibrate();
                return FEEDBACK_OK;

            #endif // ! ENABLE_PID

            case 350: {
                // M350 (ex M350 V16 or M350) - Sets the microstepping divisor for the motor. This value can be 1, 2, 4, 8, 16, or 32. Sets or gets the microstepping divisor for the motor. This value can be 1, 2, 4, 8, 16, or 32. If no value is provided, then the current microstepping divisor will be returned.
                int16_t setValue = parseValue(buffer, 'V').toInt();
                if (setValue != -1) {

                    // Value is valid, set and return ok
                    motor.setMicrostepping(setValue);
                    updateCorrectionTimer();
                    return FEEDBACK_OK;
                }
                else {
                    // No value exists, get and return the current value
                    return String(motor.getMicrostepping());
                }
            }

            case 352: {
                // M352 (ex M352 S1 or M352) - Sets or gets the direction pin inversion for the motor (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
                int16_t setValue = parseValue(buffer, 'S').toInt();
                if (setValue == 0 || setValue == 1) {

                    // Value is valid, set and return ok
                    motor.setReversed(setValue == 1);
                    return FEEDBACK_OK;
                }
                else {
                    // No value exists, get and return the current value
                    return String(motor.getReversed());
                }
            }

            case 353: {
                // M353 (ex M353 S1 or M353) - Sets or gets the enable pin inversion for the motor (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
                int16_t setValue = parseValue(buffer, 'S').toInt();
                if (setValue == 0 || setValue == 1) {

                    // Value is valid, set and return ok
                    motor.setEnableInversion(setValue == 1);
                    return FEEDBACK_OK;
                }
                else {
                    // No value exists, get and return the current value
                    return String(motor.getEnableInversion());
                }
            }

            case 354: {
                // M354 (ex M354 S1 or M354) - Sets or gets if the motor dip switches were installed incorrectly (reversed) (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
                int16_t setValue = parseValue(buffer, 'S').toInt();
                if (setValue == 0 || setValue == 1) {

                    // Value is valid, set and return ok
                    setDipInverted(setValue == 1);
                    return FEEDBACK_OK;
                }
                else {
                    // No value exists, get and return the current value
                    return String(getDipInverted());
                }
            }

            case 355: {
                // M355 (ex M355 V1.34 or M355) - Sets or gets the microstep multiplier for the board. Allows to use multiple motors connected to the same mainboard pin, yet have different rates. If no value is provided, then the current value will be returned.
                float setValue = parseValue(buffer, 'V').toFloat();
                if (setValue != -1) {

                    // Value is valid, set and return ok
                    motor.setMicrostepMultiplier(setValue);
                    return FEEDBACK_OK;
                }
                else {
                    // No value exists, get and return the current value
                    return String(motor.getMicrostepMultiplier());
                }
            }

            case 356: {

                // Only build in functionality if specified
                #ifdef ENABLE_CAN
                    // M356 (ex M356 V1 or M356 VX2 or M356) - Sets or gets the CAN ID of the board. Can be set using the axis character or actual ID. If no value is provided, then the current value will be returned.

                    // Check the value of the axis
                    String axisValue = parseValue(buffer, 'V');

                    // Check if the value is not a number (0 is returned if the value cannot be converted)
                    if (axisValue.toInt() == 0) {

                        // Compare the values of the received value with the expected ones
                        if (axisValue == "X" || axisValue == "X1") {
                            setCANID(X);
                        }
                        else if (axisValue == "X2") {
                            setCANID(X2);
                        }
                        else if (axisValue == "X3") {
                            setCANID(X3);
                        }
                        else if (axisValue == "X4") {
                            setCANID(X4);
                        }
                        else if (axisValue == "X5") {
                            setCANID(X5);
                        }
                        else if (axisValue == "Y" || axisValue == "Y1") {
                            setCANID(Y);
                        }
                        else if (axisValue == "Y2") {
                            setCANID(Y2);
                        }
                        else if (axisValue == "Y3") {
                            setCANID(Y3);
                        }
                        else if (axisValue == "Y4") {
                            setCANID(Y4);
                        }
                        else if (axisValue == "Y5") {
                            setCANID(Y5);
                        }
                        else if (axisValue == "Z" || axisValue == "Z1") {
                            setCANID(Z);
                        }
                        else if (axisValue == "Z2") {
                            setCANID(Z2);
                        }
                        else if (axisValue == "Z3") {
                            setCANID(Z3);
                        }
                        else if (axisValue == "Z4") {
                            setCANID(Z4);
                        }
                        else if (axisValue == "Z5") {
                            setCANID(Z5);
                        }
                        else if (axisValue == "E" || axisValue == "E1") {
                            setCANID(E);
                        }
                        else if (axisValue == "E2") {
                            setCANID(E2);
                        }
                        else if (axisValue == "E3") {
                            setCANID(E3);
                        }
                        else if (axisValue == "E4") {
                            setCANID(E4);
                        }
                        else if (axisValue == "E5") {
                            setCANID(E5);
                        }
                        else {
                            return FEEDBACK_NO_VALUE;
                        }

                        // Return operation successful
                        return FEEDBACK_OK;
                    }
                    else {
                        // Value is a number, check if it isn't -1 (meaning that a value wasn't specified)
                        if (axisValue.toInt() != -1) {

                            // Set the CAN ID
                            setCANID(AXIS_CAN_ID(parseValue(buffer, 'V').toInt()));

                            // Return that the operation is complete
                            return FEEDBACK_OK;
                        }
                        else {
                            // Value is invalid, therefore one doesn't exist. Just return the current value
                            return String(getCANID());
                        }
                    }

                #else
                    // Return that the feature is not enabled
                    return FEEDBACK_CAN_NOT_ENABLED;
                #endif
            }

            case 500:
                // M500 (ex M500) - Saves the currently loaded parameters into flash
                saveParameters();
                return FEEDBACK_OK;

            case 501: {
                // M501 (ex M501) - Loads all saved parameters from flash
                return loadParameters();
            }

            case 502:
                // M502 (ex M502) - Wipes all parameters from flash, then reboots the system
                wipeParameters();
                // No return here because wipeParameters reboots processor

            case 907: {
                // Sets or gets the RMS(R) or Peak(P) current in mA. If dynamic current is enabled, then the accel(A), idle(I), and/or max(M) can be set or retrieved. If no value is set, then the current RMS current (no dynamic current) or the accel, idle, and max terms (dynamic current) will be returned.
                #ifdef ENABLE_DYNAMIC_CURRENT
                    // Read the set values
                    int16_t accelCurrent = parseValue(buffer, 'A').toInt();
                    int16_t idleCurrent = parseValue(buffer, 'I').toInt();
                    int16_t maxCurrent = parseValue(buffer, 'M').toInt();

                    // Check to make sure that at least one isn't -1 (there is at least one that is valid)
                    if (!((accelCurrent == -1) && (idleCurrent == -1) && (maxCurrent == -1))) {

                        // Set the values
                        motor.setDynamicAccelCurrent(accelCurrent);
                        motor.setDynamicIdleCurrent(idleCurrent);
                        motor.setDynamicMaxCurrent(maxCurrent);
                    }
                    else {
                        // No valid values, therefore just return the current values
                        return ("A:" + String(motor.getDynamicAccelCurrent()) + " I: " + String(motor.getDynamicIdleCurrent()) + " M: " + String(motor.getDynamicMaxCurrent()) + "\n");
                    }

                #else
                    // Read the set values (one of them should be -1 (no value exists))
                    int16_t rmsCurrent = parseValue(buffer, 'R').toInt();
                    int16_t peakCurrent = parseValue(buffer, 'P').toInt();

                    // Check if RMS current is valid
                    if (rmsCurrent != -1) {
                        motor.setRMSCurrent(rmsCurrent);
                        return FEEDBACK_OK;
                    }
                    else if (peakCurrent != -1) {
                        motor.setPeakCurrent(peakCurrent);
                        return FEEDBACK_OK;
                    }
                    else {
                        // No value set. Just return the RMS current
                        return String(motor.getRMSCurrent());
                    }
                #endif
            }
            case 1000: {
                // Just for testing
                Serial.println("Testing parseString");
                return parseString(buffer, 'S');
            }

            default: {
                // Command isn't recognized, therefore throw an error
                return FEEDBACK_CMD_NOT_AVAILABLE;
            }
        }
    }

    // Gcodes support
    #ifdef ENABLE_FULL_MOTION_PLANNER
    // Check to see if a gcode exists
    else if (parseValue(buffer, 'G') != "-1") {

        // Switch statement the command number
        switch (parseValue(buffer, 'G').toInt()) {

            case 0: {
                // - G0 (ex G0 R1000 A123.45) - Rapid movement at a specified distance. Distance can be in degrees (for axes A, B, C) or in mm (for axes X, Y, Z). Steps/mm must be set for movements using mm units. The feedrate is in units per minute (mm/m for X, Y, and Z, deg/m for A, B, and C)
                // http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g0
                // Pull the values from the command
                float value = parseValue(buffer, (char)motor.axis).toFloat();
                float rate = parseValue(buffer, 'F').toFloat();

                // Sanitize the inputs
                if (rate <= 0) {
                    rate = motor.planner.getLastFeedRate();
                }

                // Save the new rate for next time (needed if the next command doesn't specify rate)
                motor.planner.setLastFeedRate(rate);

                // Determine the number of steps we need to move
                // Create a count value for number of steps
                int32_t count;

                // Decide if units are mm or deg based on axis letter
                if ((char)motor.axis == 'A' || (char)motor.axis == 'B' || (char)motor.axis == 'C') {

                    // Units are degrees
                    count = round(value / motor.getMicrostepAngle());

                    // Fix the rate so that it is in steps/s instead of deg/m
                    rate = (rate / (motor.getMicrostepAngle() * 60));
                }
                else {
                    // Units must be in mm
                    // Check to make sure that steps per mm has been set
                    if (motor.getStepsPerMM() > 0) {

                        // Set the count value
                        count = round(value * motor.getStepsPerMM());

                        // Fix the rate so that it is in steps/s instead of mm/m
                        rate = (rate * motor.getStepsPerMM()) / 60;
                    }
                    else {
                        // There is no steps per mm, throw an error
                        return FEEDBACK_STEPS_PER_MM_NOT_SET;
                    }
                }

                // Adjust the count by the current position if the mode is absolute
                if (motor.planner.getDistanceMode() == ABSOLUTE) {
                    count -= motor.getDesiredStep();
                }

                // Default to CCW rotation
                STEP_DIR dir = POSITIVE;

                // If the count is negative (motor needs to move in opposite direction)
                // then we can make the count positive and fix the direction
                if (count < 0) {
                    count = -count;
                    dir = NEGATIVE;
                }

                // Schedule the steps to be moved
                scheduleSteps(count, rate, dir);

                // Return that everything went well
                return FEEDBACK_OK;
            }

            #ifdef ENABLE_DIRECT_STEPPING
            case 6: {
                // G6 (ex G6 D0 R1000 S1000) - Direct stepping, commands the motor to move a specified number of steps in the specified direction. D is direction (0 for CCW, 1 for CW), R is rate (in Hz), and S is the count of steps to move
                // Pull the values from the command
                bool reverse = parseValue(buffer, 'D').equals("1");
                int32_t rate = parseValue(buffer, 'R').toInt();
                int64_t count = parseValue(buffer, 'S').toInt();

                // Sanitize the inputs
                if (rate <= 0) {
                    rate = motor.planner.getLastStepRate();
                }
                if (count == 0) {
                    return FEEDBACK_NO_VALUE;
                }
                if (count < 0) {
                    reverse = !reverse;
                }

                // Save the current rate as the new one
                motor.planner.setLastStepRate(rate);

                // Call the steps to be scheduled
                if (!reverse) {
                    scheduleSteps(count, rate, POSITIVE);
                }
                else {
                    scheduleSteps(count, rate, NEGATIVE);
                }

                // All good, we can exit
                return FEEDBACK_OK;
            }
            #endif

            case 90: {
                motor.planner.setDistanceMode(ABSOLUTE);
                return FEEDBACK_OK;
            }

            case 91: {
                motor.planner.setDistanceMode(INCREMENTAL);
                return FEEDBACK_OK;
            }

            default: {
                // Command isn't recognized, therefore throw an error
                return FEEDBACK_CMD_NOT_AVAILABLE;
            }
        }

    }

    #endif // ! ENABLE_FULL_MOTION_PLANNER

    // Nothing here, nothing to do
    return FEEDBACK_NO_CMD_SPECIFIED;
}


// Returns the substring of the value after the letter parameter
String parseValue(String buffer, char letter) {

    // Convert the buffer to all uppercase (easier to read)
    buffer.toUpperCase();

    // Search the input string for the specified letter
    int16_t charIndex = buffer.indexOf(toupper(letter));

    // If the index came back with a value, we can begin the process of extracting the raw value
    if (charIndex != -1) {

        // Get the next index of a space
        int16_t nextSpaceIndex = buffer.substring(charIndex).indexOf(' ');

        // Check to see if there is a space between the letter and value
        if (nextSpaceIndex == 1) {

            // We need to find out if there is another space after this parameter
            int16_t endSpaceIndex = buffer.substring(charIndex + nextSpaceIndex + 1).indexOf(' ');

            // Check to see if there is an ending space
            if (endSpaceIndex != -1) {

                // Return only the substring between the ending space and the space after the letter
                return buffer.substring(charIndex + nextSpaceIndex + 1, charIndex + nextSpaceIndex + endSpaceIndex + 2);
            }
            else {
                // That's the end of the string, we can start at the space and just include the rest
                return buffer.substring(charIndex + nextSpaceIndex + 1);
            }
        }
        else if (nextSpaceIndex != -1) {
            // The next space index is after the value, so just include up to it
            return buffer.substring(charIndex + 1, charIndex + nextSpaceIndex);
        }
        else {
            // There is no more spaces in the string, therefore just return the rest of the string
            return buffer.substring(charIndex + 1);
        }
    }
    else {
        // Index is invalid, V doesn't exist. Print an output message, then return a null
        // ! Serial.println(FEEDBACK_NO_VALUE);
        return "-1";
    }
}


// Returns the substring of the string after the letter parameter
String parseString(String buffer, char letter) {

    // Create a variable to store the found charIndex
    int16_t charIndex = -1;

    // Search the input string for the specified letter in both upper and lower case
    int16_t uppercaseIndex = buffer.indexOf(toupper(letter));
    int16_t lowercaseIndex = buffer.indexOf(tolower(letter));

    // Check if one of the cases exists
    if (!(uppercaseIndex == -1 || uppercaseIndex == 65535)) {

        // Uppercase index is valid, save it as the charIndex
        charIndex = uppercaseIndex;
    }
    else if (!(lowercaseIndex == -1 || lowercaseIndex == 65535)) {

        // Lowercase index is valid, save it as the charIndex
    }
    // else {
    //    Leave the charIndex as -1, signifying that it doesn't exist
    //}

    // If the index came back with a value, we can begin the process of extracting the raw value
    if (charIndex != -1) {

        // Time to check to see where the double quotations are
        int16_t startQuotationIndex = buffer.substring(charIndex + 1).indexOf('"');
        int16_t endQuotationIndex = buffer.substring(charIndex + startQuotationIndex + 2).indexOf('"');

        // Make sure that there are quotations on each side
        if (startQuotationIndex != -1 && endQuotationIndex != -1) {

            // Return the string between them
            return buffer.substring(charIndex + startQuotationIndex + 2, charIndex + startQuotationIndex + endQuotationIndex + 2);
        }
        else {
            // Throw an error, we can't find both the quotation marks
            Serial.println(FEEDBACK_INVALID_STRING);
            return "-1";
        }
    }
    else {
        // Index is invalid, letter doesn't exist. Print an output message, then return a null
        Serial.println(FEEDBACK_NO_VALUE);
        return "-1";
    }
}

#endif // (ENABLE_SERIAL || ENABLE_CAN)