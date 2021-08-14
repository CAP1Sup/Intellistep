#include "motor.h"

// These imports must be here to prevent linking circles
#include "flash.h"
#include "oled.h"

// Optimize for speed
#pragma GCC optimize ("-Ofast")

// Main constructor
StepperMotor::StepperMotor() {

    // Setup the input pins
    pinMode(STEP_PIN, INPUT);
    pinMode(DIRECTION_PIN, INPUT);
    pinMode(ENABLE_PIN, INPUT);


    #ifdef USE_HARDWARE_STEP_CNT
    // Setup TIM2 (the base) (for hardware step counter)
    tim2Config.Instance = TIM2;
    tim2Config.Init.Prescaler = 0;
    tim2Config.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim2Config.Init.Period = 0xFFFF;
    tim2Config.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim2Config.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&tim2Config);

    // Set that the step pin should be an external trigger for the timer to count
    tim2ClkConfig.ClockFilter = 7;
    tim2ClkConfig.ClockPolarity = TIM_CLOCKPOLARITY_INVERTED;
    tim2ClkConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    tim2ClkConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
    HAL_TIM_ConfigClockSource(&tim2Config, &tim2ClkConfig);

    // Configure the master/slave mode of the timer
    tim2MSConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    tim2MSConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&tim2Config, &tim2MSConfig);

    // Set that the direction pin should be used as a direction control
    // Clear the encoder mode bit, then set it
    tim2Config.Instance -> SMCR &= ~TIM_SMCR_SMS;
    tim2Config.Instance -> SMCR |= TIM_ENCODERMODE_TI1;

    // Reset TIM2's counter
    __HAL_TIM_SET_COUNTER(&tim2Config, 0);

    // Enable TIM2
    __HAL_TIM_ENABLE(&tim2Config);

    // Attach the overflow interrupt (has to use HardwareTimer
    // because HardwareTimer library holds all callbacks)
    tim2HWTim -> setInterruptPriority(5, 0);
    tim2HWTim -> attachInterrupt(overflowHandler);
    #endif

    // Setup the pins as outputs
    pinMode(COIL_A_POWER_OUTPUT_PIN, OUTPUT);
    pinMode(COIL_B_POWER_OUTPUT_PIN, OUTPUT);

    // Setup the coil direction pins
    pinMode(COIL_A_DIR_1_PIN, OUTPUT);
    pinMode(COIL_A_DIR_2_PIN, OUTPUT);
    pinMode(COIL_B_DIR_1_PIN, OUTPUT);
    pinMode(COIL_B_DIR_2_PIN, OUTPUT);

    // Configure the PWM current output pins
    this -> PWMCurrentPinInfoA = analogSetup(COIL_A_POWER_OUTPUT_PIN, MOTOR_PWM_FREQ, 0);
    this -> PWMCurrentPinInfoB = analogSetup(COIL_B_POWER_OUTPUT_PIN, MOTOR_PWM_FREQ, 0);

    // Disable the motor
    setState(DISABLED, true);
}


// Returns the current RPM of the encoder
float StepperMotor::getEncoderRPM() {

    // Convert getSpeed() (in deg/s) to RPM
    return DPS_TO_RPM((float)encoder.getSpeed());
}


// Returns the current calculated RPM
float StepperMotor::getEstimRPM() {

    // Convert getEstimSpeed() (in deg/s) to RPM
    return DPS_TO_RPM((float)encoder.getEstimSpeed());
}


// Returns the current calculated RPM
float StepperMotor::getEstimRPM(double currentAbsAngle) {

    // Convert getEstimSpeed() (in deg/s) to RPM
    return DPS_TO_RPM((float)encoder.getEstimSpeed(currentAbsAngle));
}


#ifdef ENABLE_STEPPING_VELOCITY
// Compute the stepping interface velocity in deg/s
float StepperMotor::getDegreesPS() {
    calc:
    while (isStepping)
        ; // wait end of stepping calculation
    float velocity = 1000000.0 * angleChange / (nowStepingSampleTime - prevStepingSampleTime);
    if (isStepping)
        goto calc;
    return velocity;
}


// Compute the stepping interface RPM
float  StepperMotor::getSteppingRPM() {
    return DPS_TO_RPM(getDegreesPS());
}
#endif // ! ENABLE_STEPPING_VELOCITY


// Returns the angular deviation of the motor from the desired angle
float StepperMotor::getAngleError() {
    return (getDesiredAngle() - encoder.getAbsoluteAngleAvg());
}


// Returns the angular deviation of the motor from the desired angle
float StepperMotor::getAngleError(double currentAbsAngle) {
    return (getDesiredAngle() - currentAbsAngle);
}


// Returns the step deviation of the motor from the desired step
int32_t StepperMotor::getStepError() {
    return (getDesiredStep() - round(encoder.getAbsoluteAngleAvg() / (this -> microstepAngle)));
}


// Returns the step deviation of the motor from the desired step
int32_t StepperMotor::getStepError(double currentAbsAngle) {
    return (getDesiredStep() - round(currentAbsAngle / (this -> microstepAngle)));
}


// Returns the current step of the motor phases (only 1 rotation worth)
int32_t StepperMotor::getStepPhase() {
    return (this -> currentStep);
}


// Returns the desired angle of the motor
float StepperMotor::getDesiredAngle() {
    #ifdef USE_SOFTWARE_STEP_CNT
        return (microstepAngle * getSoftStepCNT());
    #else
        return (microstepAngle * getHardStepCNT());
    #endif
}


// Sets the desired angle of the motor
void StepperMotor::setDesiredAngle(float newDesiredAngle) {
    #ifdef USE_SOFTWARE_STEP_CNT
        setSoftStepCNT(round(newDesiredAngle / microstepAngle));
    #else
        setHardStepCNT(round(newDesiredAngle / microstepAngle));
    #endif
}


// Returns the desired step of the motor
int32_t StepperMotor::getDesiredStep() {
    #ifdef USE_SOFTWARE_STEP_CNT
        return getSoftStepCNT();
    #else
        return getHardStepCNT();
    #endif
}


// Sets the desired step of the motor
void StepperMotor::setDesiredStep(int32_t newDesiredStep) {
    #ifdef USE_SOFTWARE_STEP_CNT
        setSoftStepCNT(newDesiredStep);
    #else
        setHardStepCNT(newDesiredStep);
    #endif
}


#ifdef USE_HARDWARE_STEP_CNT
// Returns the count value of the timer-based step counter
int32_t StepperMotor::getHardStepCNT() const {
    return ((TIM2 -> CNT) + stepOverflowOffset);
}


// Sets the count value of the timer-based step counter
void StepperMotor::setHardStepCNT(int32_t newCNT) {

    // Find the remainder for the counter to use
    uint32_t newClockCNT = (newCNT % TIM_PERIOD);

    // Set the new overflow count
    stepOverflowOffset = (newCNT - newClockCNT);

    // Set the counter
    __HAL_TIM_SET_COUNTER(&tim2Config, (uint16_t)newClockCNT);
}


// Fixes the step overflow count
void overflowHandler() {

    // Check which direction the overflow was in
    if ((TIM2 -> CNT) < (TIM_PERIOD / 2)) {

        // Overflow
        motor.stepOverflowOffset += TIM_PERIOD;
    }
    else {
        // Underflow
        motor.stepOverflowOffset -= TIM_PERIOD;
    }
}
#endif // USE_HARDWARE_STEP_CNT

#ifdef USE_SOFTWARE_STEP_CNT
// Returns the desired step of the motor
int32_t StepperMotor::getSoftStepCNT() const {
    return (this -> softStepCNT);
}


// Sets the desired step of the motor
void StepperMotor::setSoftStepCNT(int32_t newStepCNT) {
    this -> softStepCNT = newStepCNT;
}
#endif // USE_SOFTWARE_STEP_CNT


#ifdef ENABLE_DYNAMIC_CURRENT
// Gets the acceleration factor for dynamic current
uint16_t StepperMotor::getDynamicAccelCurrent() const {
    return (this -> dynamicAccelCurrent);
}

// Gets the idle factor for dynamic current
uint16_t StepperMotor::getDynamicIdleCurrent() const {
    return (this -> dynamicIdleCurrent);
}

// Gets the max current factor for dynamic current
uint16_t StepperMotor::getDynamicMaxCurrent() const {
    return (this -> dynamicMaxCurrent);
}

// Sets the acceleration factor for dynamic current
void StepperMotor::setDynamicAccelCurrent(uint16_t newAccelFactor) {

    // Make sure that the value being set is positive (no negatives allowed)
    if (newAccelFactor >= 0) {
        this -> dynamicAccelCurrent = newAccelFactor;
    }
}

// Sets the idle factor for dynamic current
void StepperMotor::setDynamicIdleCurrent(uint16_t newIdleFactor) {

    // Make sure that the value being set is positive (no negatives allowed)
    if (newIdleFactor >= 0) {
        this -> dynamicIdleCurrent = newIdleFactor;
    }
}

// Sets the max current factor for dynamic current
void StepperMotor::setDynamicMaxCurrent(uint16_t newMaxCurrent) {

    // Make sure that the value being set is positive (no negatives allowed)
    if (newMaxCurrent >= 0) {
        this -> dynamicMaxCurrent = newMaxCurrent;
    }
}

#else // ! ENABLE_DYNAMIC_CURRENT

// Gets the RMS current of the motor (in mA)
uint16_t StepperMotor::getRMSCurrent() const {
    return (this -> rmsCurrent);
}


// Gets the peak current of the motor (in mA)
uint16_t StepperMotor::getPeakCurrent() const {
    return (this -> peakCurrent);
}


// Sets the RMS current of the motor (in mA)
void StepperMotor::setRMSCurrent(uint16_t rmsCurrent) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (rmsCurrent != -1) {

        // Make sure that the RMS current is within the current bounds of the motor, if so set it
        this -> rmsCurrent = constrain(rmsCurrent, 0, MAX_RMS_BOARD_CURRENT);

        // Also set the peak current
        this -> peakCurrent = constrain((uint16_t)(rmsCurrent * 1.414), 0, MAX_PEAK_BOARD_CURRENT);
    }
}


// Sets the peak current of the motor (in mA)
void StepperMotor::setPeakCurrent(uint16_t peakCurrent) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (peakCurrent != -1) {

        // Make sure that the peak current is within the current bounds of the motor, if so set it
        this -> peakCurrent = constrain(peakCurrent, 0, MAX_PEAK_BOARD_CURRENT);

        // Also set the RMS current
        this -> rmsCurrent = constrain((uint16_t)(peakCurrent * 0.707), 0, MAX_RMS_BOARD_CURRENT);
    }
}
#endif // ! ENABLE_DYNAMIC_CURRENT

// Get the microstepping divisor of the motor
uint8_t StepperMotor::getMicrostepping() {
    return (this -> microstepDivisor);
}


// Set the microstepping divisor of the motor
void StepperMotor::setMicrostepping(uint8_t setMicrostepping, bool lock) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (setMicrostepping != -1) {

        // Check if the microstepping has been locked and the lock is not enabled
        if (this -> microstepLocked && !lock) {

            // Nothing should be changed, exit the function
            return;
        }

        // Calculate the step scaling
        float stepScalingFactor = (setMicrostepping / this -> microstepDivisor);

        // Scale the step count
        #ifdef USE_SOFTWARE_STEP_CNT
            setSoftStepCNT(getSoftStepCNT() * stepScalingFactor);
        #else
            setHardStepCNT(getHardStepCNT() * stepScalingFactor);
        #endif

        // Scale the microstep multiplier so that the full stepping level is maintained
        // This needs to be done before the new divisor is set
        #ifdef MAINTAIN_FULL_STEPPING
            this -> microstepMultiplier *= stepScalingFactor;
        #endif

        // Set the microstepping divisor
        this -> microstepDivisor = setMicrostepping;

        // Fix the microstep angle
        this -> microstepAngle = (this -> fullStepAngle) / (this -> microstepDivisor);

        // Fix the microsteps per rotation
        this -> microstepsPerRotation = round(360.0 / microstepAngle);

        // Fix the step to sine array factor
        this -> stepToSineArrayFactor = MAX_MICROSTEP_DIVISOR / setMicrostepping;

        // Set that the microstepping should be locked for future writes
        this -> microstepLocked = lock;
    }
}


// Set the full step angle of the motor (in degrees)
void StepperMotor::setFullStepAngle(float newStepAngle) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (newStepAngle != -1) {

        // Make sure that the value is one of the 2 common types
        // ! Maybe remove later?
        if ((newStepAngle == (float)1.8) || (newStepAngle == (float)0.9)) {

            // Save the new full step angle
            this -> fullStepAngle = newStepAngle;

            // Fix the microstep angle
            this -> microstepAngle = (this -> fullStepAngle) / (this -> microstepDivisor);

            // Fix the microsteps per rotation
            this -> microstepsPerRotation = round(360.0 / microstepAngle);
        }
    }
}


// Get the full step angle of the motor object
float StepperMotor::getFullStepAngle() const {
    return (this -> fullStepAngle);
}


// Get the microstep angle of the motor
float StepperMotor::getMicrostepAngle() const {
    return (this -> microstepAngle);
}


// Get the microsteps per rotation of the motor
int32_t StepperMotor::getMicrostepsPerRotation() const {
    return (this -> microstepsPerRotation);
}


// Set if the motor direction should be reversed or not
void StepperMotor::setReversed(bool reversed) {

    // Set if the motor should be reversed
    if (reversed) {
        this -> reversed = NEGATIVE;
    }
    else {
        this -> reversed = POSITIVE;
    }
}


// Get if the motor direction is reversed
bool StepperMotor::getReversed() const {

    // Decide if the motor is reversed or not
    if (this -> reversed > 0) {
        return false;
    }
    else {
        return true;
    }
}


// Set if the motor enable should be inverted
void StepperMotor::setEnableInversion(bool inverted) {

    // Set the object's value
    this -> enableInverted = inverted;
}


// Get if the motor enable should be inverted
bool StepperMotor::getEnableInversion() const {

    // Return the object's value
    return (this -> enableInverted);
}


// Set the microstep multiplier
void StepperMotor::setMicrostepMultiplier(float newMultiplier) {

    // Set the object's value if it is valid
    if (newMultiplier != -1) {
        (this -> microstepMultiplier) = newMultiplier;
    }
}


// Get the microstep multiplier
float StepperMotor::getMicrostepMultiplier() const {

    // Return the object's value
    return (this -> microstepMultiplier);
}


void StepperMotor::simpleStep() {

    // Only moving one step in the specified direction
    this -> currentStep += DIRECTION(GPIO_READ(DIRECTION_PIN)) * (this -> reversed) * (this -> microstepMultiplier);

    // Drive the coils to their destination
    this -> driveCoils(this -> currentStep);
}


// Computes the coil values for the next step position and increments the set angle
#ifdef USE_HARDWARE_STEP_CNT
void StepperMotor::step(STEP_DIR dir, int32_t stepChange, bool updateDesiredPos) {
#else
void StepperMotor::step(STEP_DIR dir, int32_t stepChange, bool updateDesiredPos) {
#endif

    #ifdef ENABLE_STEPPING_VELOCITY
        isStepping = true;

        // Sample times
        prevStepingSampleTime = nowStepingSampleTime;
        nowStepingSampleTime = micros();
    #endif
    /*
    // Declare a variable to calculate the step change with
    int32_t stepChange;

    // Factor in the multiplier if specified
    if (!useMultiplier) {

        // Only move one step per pulse when multiplier is disabled
        stepChange = 1;
    }
    else {
        // Move the number of steps specified by the microstep multiplier
        stepChange = (this -> microstepMultiplier);
    }
    */
    /*
    // Invert the change based on the direction
    if (dir == PIN) {

        // Use the DIR_PIN state to decide direction
        stepChange *= (DIRECTION(GPIO_READ(DIRECTION_PIN)) * (this -> reversed));
    }
    //else if (dir == COUNTER_CLOCKWISE) {
        // Nothing to do here, the value is already positive
    //}
    else if (dir == CLOCKWISE) {

        // Make the step change in the negative direction
        stepChange = -stepChange;
    }
    */

    #ifdef ENABLE_STEPPING_VELOCITY
        isStepping = false;
    #endif

    stepChange *= dir * (this -> reversed);

    #ifdef USE_SOFTWARE_STEP_CNT
    // Update the desired angle if specified
    if (updateDesiredPos) {

        // Angles are basically just added to desired, not really much to do here
        this -> softStepCNT += stepChange;
    }
    #endif

    // Invert the change based on the direction
    // Only moving one step in the specified direction
    this -> currentStep += stepChange;

    // Drive the coils to their destination
    this -> driveCoils(this -> currentStep);
}


// Sets the coils of the motor based on the step count
void StepperMotor::driveCoils(int32_t steps) {

    // Correct the steps to the 32nd microstep range, then
    // calculate the sine and cosine of the angle
    // (sine values are based on 32nd microstepping range)
    uint16_t arrayIndex = (((int64_t)steps) * (this -> stepToSineArrayFactor)) & (SINE_VAL_COUNT - 1);

    // Calculate the coil settings
    int16_t coilAPercent = fastSin(arrayIndex);
    int16_t coilBPercent = fastCos(arrayIndex);

    // Equation comes out to be (effort * -1 to 1) depending on the sine/cosine of the phase angle
    #ifdef ENABLE_DYNAMIC_CURRENT

        // Get the current acceleration
        double angAccel = abs(motor.encoder.getAccel());

        // Compute the coil power
        int16_t coilAPower = ((int16_t)(((angAccel * (this -> dynamicAccelCurrent)) + (this -> dynamicIdleCurrent)) * 1.414) * coilAPercent) >> SINE_POWER;
        int16_t coilBPower = ((int16_t)(((angAccel * (this -> dynamicAccelCurrent)) + (this -> dynamicIdleCurrent)) * 1.414) * coilBPercent) >> SINE_POWER;
    #else
        // Just use static current multipiers
        int16_t coilAPower = ((int16_t)(this -> peakCurrent) * coilAPercent) >> SINE_POWER; // i.e. / SINE_MAX
        int16_t coilBPower = ((int16_t)(this -> peakCurrent) * coilBPercent) >> SINE_POWER; // i.e. / SINE_MAX
    #endif

    // Check the if the coil should be energized to move backward or forward
    if (coilAPower > 0) {

        // Set first channel for forward movement
        setCoilA(FORWARD, coilAPower);
    }
    else if (coilAPower < 0) {

        // Set first channel for backward movement
        setCoilA(BACKWARD, -coilAPower);
    }
    else {
        setCoilA(BRAKE);
    }

    // Check the if the coil should be energized to move backward or forward
    if (coilBPower > 0) {

        // Set first channel for forward movement
        setCoilB(COIL_STATE::FORWARD, coilBPower);
    }
    else if (coilBPower < 0) {

        // Set first channel for backward movement
        setCoilB(BACKWARD, -coilBPower);
    }
    else {
        setCoilB(BRAKE);
    }
}


// Sets the coils of the motor based on the angle (angle should be in degrees)
void StepperMotor::driveCoilsAngle(float degAngle) {

    // Should be a faster way of constraining the degAngle back into 0-360
    if (degAngle < 0) {
        degAngle += round(abs(degAngle) / 360) * 360;
    }
    else if ( degAngle > 360) {
        degAngle -= round(degAngle / 360) * 360;
    }

    // Constrain the set angle to between 0 and 360
    while (degAngle < 0 || degAngle > 360) {

        // The angle is less than 0, add 360
        if (degAngle < 0) {
            degAngle += 360;
        }
        else {
            // The angle must be greater than 360, reduce the angle by 360
            degAngle -= 360;
        }
    }

    // Convert the angle to microstep values (formula uses degAngle * full steps for rotation * microsteps)
    float microstepAngle = (degAngle / this -> fullStepAngle) * (this -> microstepDivisor);

    // Round the microstep angle, it has to be a whole value of the number of microsteps available
    // Also ensures that the coils are being driven to the major step positions (increases torque)
    uint16_t roundedMicrosteps = round(microstepAngle);

    // Drive the coils to the found microstep
    driveCoils(roundedMicrosteps);
}


// Function for setting the A coil state and current
void StepperMotor::setCoilA(COIL_STATE desiredState, uint16_t current) {

    // Check if the desired coil state is different from the previous, if so, we need to set the output pins
    if (previousCoilStateA != desiredState) {

        // Update the previous state of the coil with the new one
        previousCoilStateA = desiredState;

        // Decide the state of the direction pins
        if (desiredState == FORWARD) {
            GPIO_WRITE(COIL_A_DIR_1_PIN, HIGH);
            GPIO_WRITE(COIL_A_DIR_2_PIN, LOW);
        }
        else if (desiredState == BACKWARD) {
            GPIO_WRITE(COIL_A_DIR_1_PIN, LOW);
            GPIO_WRITE(COIL_A_DIR_2_PIN, HIGH);
        }
        else if (desiredState == BRAKE) {
            GPIO_WRITE(COIL_A_DIR_1_PIN, HIGH);
            GPIO_WRITE(COIL_A_DIR_2_PIN, HIGH);
        }
        else if (desiredState == COAST) {
            GPIO_WRITE(COIL_A_DIR_1_PIN, LOW);
            GPIO_WRITE(COIL_A_DIR_2_PIN, LOW);
        }
    }

    // Update the output pin with the correct current
    analogSet(&PWMCurrentPinInfoA, currentToPWM(current));
}


// Function for setting the B coil state and current
void StepperMotor::setCoilB(COIL_STATE desiredState, uint16_t current) {

    // Check if the desired coil state is different from the previous, if so, we need to set the output pins
    if (previousCoilStateB != desiredState) {

        // Update the previous state of the coil with the new one
        previousCoilStateB = desiredState;

        // Decide the state of the direction pins
        if (desiredState == FORWARD) {
            GPIO_WRITE(COIL_B_DIR_1_PIN, HIGH);
            GPIO_WRITE(COIL_B_DIR_2_PIN, LOW);
        }
        else if (desiredState == BACKWARD) {
            GPIO_WRITE(COIL_B_DIR_1_PIN, LOW);
            GPIO_WRITE(COIL_B_DIR_2_PIN, HIGH);
        }
        else if (desiredState == BRAKE) {
            GPIO_WRITE(COIL_B_DIR_1_PIN, HIGH);
            GPIO_WRITE(COIL_B_DIR_2_PIN, HIGH);
        }
        else if (desiredState == COAST) {
            GPIO_WRITE(COIL_B_DIR_1_PIN, LOW);
            GPIO_WRITE(COIL_B_DIR_2_PIN, LOW);
        }
    }

    // Update the output pin with the correct current
    analogSet(&PWMCurrentPinInfoB, currentToPWM(current));
}


// Calculates the current of each of the coils (with mapping)(current in mA)
uint32_t StepperMotor::currentToPWM(uint16_t current) const {

    // Calculate the value to set the PWM interface to (based on algebraically manipulated equations from the datasheet)
    uint32_t PWMValue = (CURRENT_SENSE_RESISTOR * PWM_MAX_VALUE * abs(current)) / (BOARD_VOLTAGE * 100);

    // Constrain the PWM value, then return it
    return constrain(PWMValue, 0, PWM_MAX_VALUE);
}


// Sets a new motor state
void StepperMotor::setState(MOTOR_STATE newState, bool clearErrors) {

    // Check to make sure that the state is different from the current
    if ((this -> state) != newState) {

        // Check if we need to clear the errors
        if (clearErrors) {
            switch (newState) {

                // Need to clear the disabled state and start the coils
                case ENABLED: {

                    // Enable the motor
                    enable();

                    // Set the new state
                    this -> state = ENABLED;
                    break;
                }
                // Same as enabled, just forced
                case FORCED_ENABLED: {

                    // Enable the motor
                    enable();

                    // Set the new state
                    this -> state = FORCED_ENABLED;
                    break;
                }
                // No other special processing needed, just disable the coils and set the state
                default:
                    motor.setCoilA(IDLE_MODE);
                    motor.setCoilB(IDLE_MODE);
                    this -> state = newState;
                    break;
            }
        }
        else {
            // Only change the state if the current state is either enabled or disabled (or not set)
            if ((this -> state) == ENABLED || (this -> state) == DISABLED || (this -> state) == MOTOR_NOT_SET) {

                // Decide when needs to happen based on the new state
                switch (newState) {

                    // Need to clear the disabled state and start the coils
                    case ENABLED: {

                        // Enable the motor
                        enable();

                        // Set the new state
                        this -> state = ENABLED;
                        break;
                    }
                    // No other special processing needed, just disable the coils and set the state
                    default:
                        motor.setCoilA(IDLE_MODE);
                        motor.setCoilB(IDLE_MODE);
                        this -> state = newState;
                        break;
                }
            }
        }
    }
}


// Private function for enabling the motor
void StepperMotor::enable() {

    // Clear the old absolute angle averages from the encoder
    encoder.clearAbsoluteAngleAvg();

    // Note the current angle
    float encoderAngle = encoder.getAbsoluteAngleAvgFloat();

    // Drive the coils the current angle of the shaft (just locks the output in place)
    driveCoilsAngle(encoderAngle);

    // The motor's current step needs corrected
    currentStep = encoderAngle / microstepAngle;

    // Reset the motor's desired step to the current
    // No need to set the desired angle, that is based off of the step
    setDesiredStep(currentStep);
}


// Return the state of the motor
MOTOR_STATE StepperMotor::getState() const {
    return (this -> state);
}


// Calibrates the encoder and the PID loop
void StepperMotor::calibrate() {

    // Only include if specified
    #ifdef ENABLE_OLED

        // Display that calibration is coming soon
        clearOLED();
        writeOLEDString(0, 0,               F("Starting"), false);
        writeOLEDString(0, LINE_HEIGHT * 1, F("Do not move"), false);
        writeOLEDString(0, LINE_HEIGHT * 2, F("motor shaft"), true);

    #endif

    // Wait for the user to take their hands away
    delay(3000);

    // Disable the motor timers (the motor needs to be left alone during calibration)
    disableMotorTimers();

    // Set the coils of the motor to move to step 0 (meaning the separation between full steps)
    driveCoils(0);

    // Delay three seconds, giving the motor time to settle
    delay(3000);

    // Force the encoder to be read a couple of times, wiping the previous position out of the average
    // (this reading needs to be as precise as possible)
    for (uint8_t readings = 0; readings < ANGLE_AVG_READINGS; readings++) {

        // Get the angle, then wait for 10ms to allow encoder to update
        encoder.getRawAngleAvg();
        delay(10);
    }

    // Measure encoder offset
    float stepOffset = encoder.getRawAngleAvg();

    // Add/subtract the full step angle till the rawStepOffset is within the range of a full step
    while (stepOffset < 0) {
        stepOffset += (this -> fullStepAngle);
    }
    while (stepOffset > (this -> fullStepAngle)) {
        stepOffset -= (this -> fullStepAngle);
    }

    // Calibrate PID loop

    // Erase all of the written parameters
    // ! Just a quick fix, needs a better fix later
    eraseParameters();

    // Write the step offset into the flash
    writeFlash(STEP_OFFSET_INDEX, stepOffset);

    // Write that the module is configured
    writeFlash(CALIBRATED_INDEX, true);

    // Reboot the chip
    NVIC_SystemReset();
}

// Returns -1 if the number is less than 0, 1 otherwise
int32_t StepperMotor::getSign(float num) {
    if (num < 0) {
        return -1;
    }
    else {
        return 1;
    }
}