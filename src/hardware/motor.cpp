#include "motor.h"

// These imports must be here to prevent linking circles
#include "flash.h"
#include "oled.h"

// Optimize for speed
#pragma GCC optimize ("-Ofast")


// Constructor with non volatile storage parameters
StepperMotor::StepperMotor(FlashParameters& aParameters):
    mParameters(aParameters) {

    // Setup the input pins
    #ifndef USE_MKS_STEP_CNT_SETUP
    pinMode(STEP_PIN, INPUT_PULLDOWN);
    pinMode(DIRECTION_PIN, INPUT_PULLDOWN);
    #endif

    // Setup enable pin for later assignment as interrupt
    pinMode(ENABLE_PIN, INPUT_PULLUP);

    /*
    BTT's code for TIM2 initialization (according to their Github)
    TIM_DeInit(TIM2); <------------------------------------------------------------------------------- Clear the old config out

	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); <------------------------------------------- Enabled by the pinMode calls
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); <-------------------------------------------- Enabled by the HAL_TIM_Encoder_Init() call

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;  //PA0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure); <---------------------------------------------------------- Done by the pinMode call, although INPUT_PULLDOWN seems better than their use of just INPUT


	TIM_TimeBaseStructure.TIM_Period = arr; //
	TIM_TimeBaseStructure.TIM_Prescaler =psc;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); <-------------------------------------------------- They configure the timer exactly the same, except they use a prescalar of 0, which seems to break
                                                                                                        the counting with this project. All of this base setup is done by the HAL_TIM_Encoder_Init() call

    TIM_ARRPreloadConfig(TIM2,DISABLE); <-------------------------------------------------------------- They disable the autoreload preload (basically makes the overflow resets faster by setting them up early)
                                                                                                        Based on my testing, this doesn't seem to have any impact. Theoretically enabled is better, but I digress
    TIM2->SMCR |= 1<<14; <----------------------------------------------------------------------------- This enables the external clock (meaning incrementing the timer on an incoming pulse).
                                                                                                        This can be removed because the external clock's config is erased anyway with the TIM_ETRClockMode2Config() call
//    TIM2->SMCR &= ~(1<<15);                     //
//    TIM2->SMCR &= ~(3<<12);                     //
//    TIM2->SMCR &= ~(0xF<<8);

    TIM_ETRClockMode2Config(TIM2, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 7); <------------- They then set the external clock bit again.
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Reset);
    TIM_SelectMasterSlaveMode(TIM2,TIM_MasterSlaveMode_Disable);

    TIM_SetCounter(TIM2 , 0); <------------------------------------------------------------------------- Done by the __HAL_TIM_SET_COUNTER() call
    TIM_Cmd(TIM2,ENABLE ); <---------------------------------------------------------------------------- Enables the timer, done by the HAL_TIM_Encoder_Start() call
    */


    // Setup base config for TIM2 (for hardware step counter)
    tim2Config.Instance = TIM2;
    tim2Config.Init.Prescaler = 1; // ! MUST BE EVEN when not using LEGACY_STEP_CNT_SETUP, using formula (prescalar + 1) = actual prescalar
    tim2Config.Init.CounterMode = TIM_COUNTERMODE_UP; // Default to counting up, this really doesn't matter as PA_1 is going to control the direction
    tim2Config.Init.Period = (TIM_PERIOD - 1); // This is the largest period possible, reduces the messy overflows
    tim2Config.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // We can just count one for one, no need to divide incoming pulses
    tim2Config.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // This should be enabled, it helps to make smoother overflow transitions


    #ifdef USE_LEGACY_STEP_CNT_SETUP
    // Clear the old config
    HAL_TIM_Base_DeInit(&tim2Config);

    // Enable the GPIOA and TIM2 clock
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    // Setup the step input pins (step and direction)
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.Pin  = GPIO_PIN_0 | GPIO_PIN_1;  //PA0
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Setup the base config of the timer
	HAL_TIM_Base_Init(&tim2Config);

    // Set to use an external clock source
    TIM_ClockConfigTypeDef clkConfig;
    clkConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
    clkConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    clkConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
    clkConfig.ClockFilter = 7;
    HAL_TIM_ConfigClockSource(&tim2Config, &clkConfig);

    // Set that master/slave mode should be disabled
    TIM_MasterConfigTypeDef sMasterConfig;
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&tim2Config, &sMasterConfig);

    // Reset the counter, the enable it
    __HAL_TIM_SET_COUNTER(&tim2Config, 0);
    __HAL_TIM_ENABLE(&tim2Config);


    #elif defined(USE_MKS_STEP_CNT_SETUP)

    // Enable the clock for GPIOA and timer 2
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    // Setup the step inputs
    // Clear the pin 0 and 1's config
    GPIOA->CRL &= 0b11111111111111111111111100000000;

    // Set pin 0 and 1 to input mode
	GPIOA->CRL |= 0b10001000;

    // Set pin 0 and 1 to pullup
	GPIOA->ODR |= 0b111;

    // Setup the direction pin interrupt to trigger on polarity change
    attachInterrupt(DIRECTION_PIN, dirChangeISR, CHANGE);
    HAL_NVIC_SetPriority(EXTI1_IRQn, DIR_PIN_PREMPT_PRIOR, DIR_PIN_SUB_PRIOR);

    // Write the config to the timer registers
    HAL_TIM_Base_Init(&tim2Config);

    // Configure the step pin as an external clock source
    TIM_ClockConfigTypeDef clkConfig;
    clkConfig.ClockSource = TIM_CLOCKSOURCE_TI1;
    clkConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
    clkConfig.ClockPolarity = TIM_CLOCKPOLARITY_FALLING;
    clkConfig.ClockFilter = 5;
    HAL_TIM_ConfigClockSource(&tim2Config, &clkConfig);

    // Reset the counter, the enable it
    __HAL_TIM_SET_COUNTER(&tim2Config, 0);
    __HAL_TIM_ENABLE(&tim2Config);

    #else // ! Hardware stepping using encoder interface

    // Set that we want to use the timer as an encoder counter, using TI1 (the step pin) as the clock pin
    // Encoders operate identical to how the step/dir interface works
    tim2EncConfig.EncoderMode = TIM_ENCODERMODE_TI1;

    // Input Capture 1 (step pin)
    tim2EncConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    tim2EncConfig.IC1Polarity = TIM_ICPOLARITY_FALLING;
    tim2EncConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    tim2EncConfig.IC1Filter = 10; // Must be between 0x0 and 0xF (0-15)

    // Input Capture 2 (direction pin)
    tim2EncConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    tim2EncConfig.IC2Polarity = TIM_ICPOLARITY_FALLING;
    tim2EncConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    tim2EncConfig.IC2Filter = 7; // Must be between 0x0 and 0xF (0-15)

    // First reset the timer's configuration
    HAL_TIM_Encoder_DeInit(&tim2Config);

    // Initialize the encoder with the created configs
    HAL_TIM_Encoder_Init(&tim2Config, &tim2EncConfig);

    // Start the encoder
    HAL_TIM_Encoder_Start(&tim2Config, TIM_CHANNEL_ALL);

    // Reset TIM2's counter
    __HAL_TIM_SET_COUNTER(&tim2Config, 0);
    #endif // ! USE_LEGACY_STEP_CNT_SETUP

    // Attach the overflow interrupt (has to use HardwareTimer
    // because HardwareTimer library holds all callbacks)
    tim2HWTim -> setInterruptPriority(TIM2_OVERFLOW_PREMPT_PRIOR, TIM2_OVERFLOW_SUB_PRIOR);
    tim2HWTim -> attachInterrupt(overflowHandler);

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


#ifdef USE_MKS_STEP_CNT_SETUP
void dirChangeISR() {
    // Read the direction pin, comparing it to if the motor is reversed
    // The flash has to be read because this function cannot access the motor object
    if (GPIO_READ(DIRECTION_PIN) != readFlashBool(MOTOR_REVERSED_INDEX)) {

        // There's no HAL function to set counter direction, so we can
        // manually set it to count up by setting bit position 4 to 0
        TIM2->CR1 &= 0b1111111111101111;
    }
    else {
        // Likewise, we can set bit position 4 to 1 to have the timer count down
        TIM2->CR1 |= 0b10000;
    }
}
#endif

#if (ENABLE_ENCODER == true)
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
    return (getDesiredStep() - round(encoder.getAbsoluteAngleAvg() / getMicrostepAngle()));
}


// Returns the step deviation of the motor from the desired step
int32_t StepperMotor::getStepError(double currentAbsAngle) {
    return (getDesiredStep() - round(currentAbsAngle / getMicrostepAngle()));
}
#endif // ENABLE_ENCODER

// Returns the current step of the motor phases (only 1 rotation worth)
int32_t StepperMotor::getStepPhase() {
    return (this -> currentStep);
}


// Returns the desired angle of the motor
float StepperMotor::getDesiredAngle() {
    return (getMicrostepAngle() * getActualStepCNT());
}


// Sets the desired angle of the motor
void StepperMotor::setDesiredAngle(float newDesiredAngle) {
    setActualStepCNT(round(newDesiredAngle / getMicrostepAngle()));
}


// Returns the desired step of the motor
int32_t StepperMotor::getDesiredStep() {
    return getActualStepCNT();
}


// Sets the desired step of the motor
void StepperMotor::setDesiredStep(int32_t newDesiredStep) {
    setActualStepCNT(newDesiredStep);
}


// Returns the count value of the timer-based step counter
int32_t StepperMotor::getActualStepCNT() const {
    return ((TIM2 -> CNT) + stepOverflowOffset);
}


// Sets the count value of the timer-based step counter
void StepperMotor::setActualStepCNT(int32_t newCNT) {

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


// Returns the number of handled steps of the motor
int32_t StepperMotor::getHandledStepCNT() {
    return (this -> handledStepCNT);
}


// Sets the number of handled steps of the motor
void StepperMotor::setHandledStepCNT(int32_t newStepCNT) {
    this -> handledStepCNT = newStepCNT;
}


// Gets the number of unhandled steps (with sign indicating direction)
// Positive numbers mean more positive steps needed, and vice versa
int32_t StepperMotor::getUnhandledStepCNT() {
    return (getActualStepCNT() - getHandledStepCNT());
}


#if (ENABLE_DYNAMIC_CURRENT != false )
// Gets the acceleration factor for dynamic current
uint16_t StepperMotor::getDynamicAccelCurrent() const {
    return mParameters.getAccelCurrent();
}

// Gets the idle factor for dynamic current
uint16_t StepperMotor::getDynamicIdleCurrent() const {
    return mParameters.getRMSCurrent();
}

// Gets the max current factor for dynamic current
uint16_t StepperMotor::getDynamicMaxCurrent() const {
    return mParameters.getMAXCurrent();
}

// Sets the acceleration factor for dynamic current
void StepperMotor::setDynamicAccelCurrent(uint16_t newAccelFactor) {

    // Make sure that the value being set is positive (no negatives allowed)
    if (newAccelFactor >= 0) {
        mParameters.setAccelCurrent(newAccelFactor);
    }
}

// Sets the idle factor for dynamic current
void StepperMotor::setDynamicIdleCurrent(uint16_t newIdleFactor) {

    // Make sure that the value being set is positive (no negatives allowed)
    if (newIdleFactor >= 0) {
        mParameters.setRMSCurrent(newIdleFactor);
    }
}

// Sets the max current factor for dynamic current
void StepperMotor::setDynamicMaxCurrent(uint16_t newMaxCurrent) {

    // Make sure that the value being set is positive (no negatives allowed)
    if (newMaxCurrent >= 0) {
        mParameters.setMAXCurrent(newMaxCurrent);
    }
}

#else // ! ENABLE_DYNAMIC_CURRENT

// Gets the RMS current of the motor (in mA)
uint16_t StepperMotor::getRMSCurrent() const {
    return (mParameters.getRMSCurrent());
}


// Gets the peak current of the motor (in mA)
uint16_t StepperMotor::getPeakCurrent() const {
    return (mParameters.getMAXCurrent());
}


// Sets the RMS current of the motor (in mA)
void StepperMotor::setRMSCurrent(uint16_t rmsCurrent) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (rmsCurrent != -1) {

        // Make sure that the RMS current is within the current bounds of the motor, if so set it
        mParameters.setRMSCurrent(rmsCurrent);

        // Also set the peak current
        mParameters.setMAXCurrent((uint16_t)(rmsCurrent * 1.414));
    }
}


// Sets the peak current of the motor (in mA)
void StepperMotor::setPeakCurrent(uint16_t peakCurrent) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (peakCurrent != -1) {

        // Make sure that the peak current is within the current bounds of the motor, if so set it
        mParameters.setMAXCurrent(peakCurrent);

        // Also set the RMS current
        mParameters.setRMSCurrent(constrain((uint16_t)(peakCurrent * 0.707), 0, MAX_RMS_BOARD_CURRENT));
    }
}
#endif // ! ENABLE_DYNAMIC_CURRENT

// Get the microstepping divisor of the motor
uint8_t StepperMotor::getMicrostepping() const {
    return mParameters.getMicrostepping();
}


// Set the microstepping divisor of the motor
uint8_t StepperMotor::setMicrostepping(uint8_t setMicrostepping, bool lock) {

    if (setMicrostepping != 1 && setMicrostepping != 2 && setMicrostepping != 4 && 
        setMicrostepping != 8 && setMicrostepping != 16 && setMicrostepping != 32) {
            return -1;
        }
    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (setMicrostepping != -1) {

        // Check if the microstepping has been locked and the lock is not enabled
        if (this -> microstepLocked && !lock) {

            // Nothing should be changed, exit the function
            return 0;
        }

        // Calculate the step scaling
        float stepScalingFactor = (setMicrostepping / mParameters.getMicrostepping());

        // Scale the step counts (both real and handled)
        setActualStepCNT(getActualStepCNT() * stepScalingFactor);
        setHandledStepCNT(getHandledStepCNT() * stepScalingFactor);

        // Scale the microstep multiplier so that the full stepping level is maintained
        // This needs to be done before the new divisor is set
        #ifdef MAINTAIN_FULL_STEPPING
            this -> microstepMultiplier *= stepScalingFactor;
        #endif

        // Set the microstepping divisor
        mParameters.setMicrostepping(setMicrostepping);

        // Fix the step to sine array factor
        this -> stepToSineArrayFactor = MAX_MICROSTEP_DIVISOR / setMicrostepping;

        // Set that the microstepping should be locked for future writes
        this -> microstepLocked = lock;

        // Update the timer based on the new microstepping
        updateCorrectionTimer();

        return 0;
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
            mParameters.setFullStepAngle(newStepAngle);

        }
    }
}


// Get the full step angle of the motor object
float StepperMotor::getFullStepAngle() const {
    return mParameters.getFullStepAngle();
}


// Get the microstep angle of the motor
float StepperMotor::getMicrostepAngle() const {
    return getFullStepAngle() / getMicrostepping();
}


// Get the microsteps per rotation of the motor
int32_t StepperMotor::getMicrostepsPerRotation() const {
    return (360.0 / getMicrostepAngle());
}


// Set if the motor direction should be reversed
void StepperMotor::setReversed(bool reversed) {

    // Set if the motor should be reversed
    if (reversed) {

        // Invert the step handler direction
        this -> reversed = NEGATIVE;

        /* Get the TIMx CCER register value */
        uint32_t tmpccer = TIM2->CCER;

        // Clear, then set the TI1 and the TI2 Polarities
        // This inverts the direction of the hardware counter
        tmpccer &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
        tmpccer |= TIM_ICPOLARITY_FALLING | (TIM_ICPOLARITY_RISING << 4U);

        /* Write the updated TIM2 CCER */
        TIM2->CCER = tmpccer;
    }
    else {
        // Invert the step handler direction
        this -> reversed = POSITIVE;

        /* Get the TIMx CCER register value */
        uint32_t tmpccer = TIM2->CCER;

        // Clear, then set the TI1 and the TI2 Polarities
        // This inverts the direction of the hardware counter
        tmpccer &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
        tmpccer |= TIM_ICPOLARITY_FALLING | (TIM_ICPOLARITY_FALLING << 4U);

        /* Write the updated TIM2 CCER */
        TIM2->CCER = tmpccer;
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


// Simple stepping function, only for testing
void StepperMotor::simpleStep() {

    // Only moving one step in the specified direction
    this -> currentStep += DIRECTION(GPIO_READ(DIRECTION_PIN)) * (this -> reversed) * (this -> microstepMultiplier);

    // Drive the coils to their destination
    this -> driveCoils(this -> currentStep);
}


// Computes the coil values for the next step position and increments the set angle
void StepperMotor::step(STEP_DIR dir, int32_t stepChange) {

    #ifdef ENABLE_STEPPING_VELOCITY
        isStepping = true;

        // Sample times
        prevStepingSampleTime = nowStepingSampleTime;
        nowStepingSampleTime = micros();
    #endif

    #ifdef ENABLE_STEPPING_VELOCITY
        isStepping = false;
    #endif

    // Factor direction and motor reversal into step change
    stepChange *= dir * (this -> reversed);

    // Angles are basically just added to handled count, not really much to do here
    this -> handledStepCNT += stepChange;

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
    #if (ENABLE_DYNAMIC_CURRENT != false )

        // Get the current acceleration
        double angAccel = abs(motor.encoder.getAccel());

        // Compute the coil power
        int16_t coilAPower = ((int16_t)(((angAccel * getDynamicAccelCurrent()) + getDynamicIdleCurrent()) * 1.414) * coilAPercent) >> SINE_POWER;
        int16_t coilBPower = ((int16_t)(((angAccel * getDynamicAccelCurrent()) + getDynamicIdleCurrent()) * 1.414) * coilBPercent) >> SINE_POWER;
    #else
        // Just use static current multipiers
        int16_t coilAPower = (getPeakCurrent() * coilAPercent) >> SINE_POWER; // i.e. / SINE_MAX
        int16_t coilBPower = (getPeakCurrent() * coilBPercent) >> SINE_POWER; // i.e. / SINE_MAX
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
    float microstepAngle = (degAngle / getMicrostepAngle()) * (mParameters.getMicrostepping());

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
                    disableStepCorrection();
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
                        disableStepCorrection();
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

    #ifdef STEP_CORRECTION
    // Clear the old absolute angle averages from the encoder
    encoder.clearAbsoluteAngleAvg();

    // Note the current angle
    float encoderAngle = encoder.getAbsoluteAngle();

    // Drive the coils the current angle of the shaft (just locks the output in place)
    driveCoilsAngle(encoderAngle);

    // The motor's current step needs corrected
    currentStep = round(encoderAngle / microstepAngle);

    // Reset the motor's desired step to the current
    // No need to set the desired angle, that is based off of the step
    setDesiredStep(currentStep);

    #endif

    // Energize the coils of the motor
    motor.driveCoils(currentStep);

    // Enable the step correction timer
    enableStepCorrection();
}


// Return the state of the motor
MOTOR_STATE StepperMotor::getState() const {
    return (this -> state);
}


// Calibrates the encoder and the PID loop
void StepperMotor::calibrate() {

    // Only include if specified
    #if (ENABLE_OLED != 0)

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

    #if (ENABLE_ENCODER == true)
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
        stepOffset += getMicrostepAngle();
    }
    while (stepOffset > getMicrostepAngle()) {
        stepOffset -= getMicrostepAngle();
    }
    #endif  // ENABLE_ENCODER

    // Calibrate PID loop

    // Erase all of the written parameters
    // ! Just a quick fix, needs a better fix later
    mParameters.eraseParameters();

    // Write the step offset into the flash
    #if (ENABLE_ENCODER == true)
        mParameters.setCalibration( stepOffset);
    #else // ENABLE_ENCODER == false
        mParameters.setCalibration( 0, false);
    #endif // ENABLE_ENCODER

    //save to flash
    mParameters.saveParameters();

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