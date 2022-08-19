// Make sure that the motor header has only been used once
#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "Arduino.h"
#include "HardwareTimer.h"
#include "encoder.h"
#include "fastAnalogWrite.h"
#include "stm32f1xx_hal_tim.h"

// For sin() and fmod() function
//#include "cmath"
//#include <math.h>
#include "fastSine.h"

// Import the pin mapping
#include "config.h"

// Period of timer counters is 0x10000
#define TIM_PERIOD ((uint32_t)65536)

// Enumeration for coil states
typedef enum {
    COIL_NOT_SET,
    FORWARD,
    BACKWARD,
    BRAKE,
    COAST
} COIL_STATE;


#if 1
    // 89.9kHz
    // 92.12kHz
    // Enumeration for stepping direction
    typedef enum {
        NEGATIVE = -1,
        POSITIVE = 1
    } STEP_DIR;
#else
    // 91.17kHz
    // 92.24kHz
    #define         NEGATIVE (-1)
    #define         POSITIVE 1
    typedef int32_t STEP_DIR;
#endif

// Enumeration for the enable state of the motor
typedef enum {
    MOTOR_NOT_SET,
    ENABLED,
    DISABLED,
    FORCED_ENABLED,
    FORCED_DISABLED

    #ifdef ENABLE_OVERTEMP_PROTECTION
    , OVERTEMP
    #endif
} MOTOR_STATE;

//fwd declaration to save build time
class FlashParameters;

struct UnitToStepsScale
{
    float xFactor;
    float yFactor;
    float zFactor;
};

// Stepper motor class (defined to make life a bit easier when dealing with the motor)
class StepperMotor {

    // Everything is public, with the expection of some private variables
    public:

        // Initialize the motor
        StepperMotor(FlashParameters& aParameters);

        #if (ENABLE_ENCODER == true)
        // Returns the current RPM of the encoder
        float getEncoderRPM();

        // Returns the current calculated RPM
        float getEstimRPM();
        float getEstimRPM(double currentAbsAngle);

        #ifdef ENABLE_STEPPING_VELOCITY
            // Compute the stepping interface velocity in deg/s
            float getDegreesPS();

            // Compute the stepping interface RPM
            float getSteppingRPM();
        #endif //ENABLE_STEPPING_VELOCITY

        // Returns the angular deviation of the motor from the set position
        float getAngleError();
        float getAngleError(double currentAbsAngle);

        // Returns the step deviation of the motor from the set position
        int32_t getStepError();
        int32_t getStepError(double currentAbsAngle);
        #endif // ENABLE_ENCODER

        // Returns the current phase setting of the motor
        int32_t getStepPhase();

        // Returns the desired angle of the motor
        float getDesiredAngle();

        // Sets the desired angle of the motor
        void setDesiredAngle(float newDesiredAngle);

        // Returns the desired step of the motor
        int32_t getDesiredStep();

        // Sets the desired step of the motor
        void setDesiredStep(int32_t newDesiredStep);

        // Returns the desired step of the motor
        int32_t getHandledStepCNT();

        // Sets the desired step of the motor
        void setHandledStepCNT(int32_t newStepCNT);

        // Returns the count according to the TIM2 hardware step counter
        int32_t getActualStepCNT() const;

        // Sets the count for the TIM2 hardware step counter
        void setActualStepCNT(int32_t newCNT);

        // Gets the deviation between the actual and handled step counts
        // Note that this will be positive if the handled is behind the actual,
        // and vice versa
        int32_t getUnhandledStepCNT();

        // Dynamic current
        #if (ENABLE_DYNAMIC_CURRENT != false )

        // Gets the acceleration factor for dynamic current
        uint16_t getDynamicAccelCurrent() const;

        // Gets the idle factor for dynamic current
        uint16_t getDynamicIdleCurrent() const;

        // Gets the max current factor for dynamic current
        uint16_t getDynamicMaxCurrent() const;

        // Sets the acceleration factor for dynamic current
        void setDynamicAccelCurrent(uint16_t newAccelFactor);

        // Sets the idle factor for dynamic current
        void setDynamicIdleCurrent(uint16_t newIdleFactor);

        // Sets the max current factor for dynamic current
        void setDynamicMaxCurrent(uint16_t newMaxCurrent);

        #else // ! ENABLE_DYNAMIC_CURRENT

        // Gets the RMS current of the motor (in mA)
        uint16_t getRMSCurrent() const;

        // Gets the peak current of the motor
        uint16_t getPeakCurrent() const;

        // Sets the RMS current of the motor (in mA)(Peak is adjusted to match)
        void setRMSCurrent(uint16_t rmsCurrent);

        // Sets the peak current of the motor (in mA)(RMS is adjusted to match)
        void setPeakCurrent(uint16_t peakCurrent);
        #endif


        // Gets the microstepping mode of the motor
        uint8_t getMicrostepping() const;

        // Sets the microstepping mode of the motor
        uint8_t setMicrostepping(uint8_t setMicrostepping, bool lock = true);

        // Sets the angle of a full step of the motor
        void setFullStepAngle(float newStepAngle);

        // Gets the full step angle of the motor
        float getFullStepAngle() const;

        // Get the microstepping angle of the motor. This is the full steps divided by the microsteps.
        // Used to speed up processing
        float getMicrostepAngle() const;

        // Get the microsteps per rotation of the motor
        int32_t getMicrostepsPerRotation() const;

        // Set if the motor should be reversed
        void setReversed(bool reversed);

        // Get if the motor direction is reversed
        bool getReversed() const;

        // Set if the motor enable pin should be inverted
        void setEnableInversion(bool inverted);

        // Get if the motor enable pin is inverted
        bool getEnableInversion() const;

        // Set the microstep multiplier
        void setMicrostepMultiplier(float newMultiplier);

        // Get the microstep multiplier
        float getMicrostepMultiplier() const;

        // Test
        void simpleStep();

        // Calculates the coil values for the motor and updates the set angle.
        void step(STEP_DIR dir, int32_t stepChange);

        // Sets the coils to hold the motor at the desired step number
        void driveCoils(int32_t steps);

        // Sets the coils to hold the motor at the desired phase angle
        void driveCoilsAngle(float angle);

        // Sets the state of the A coil
        void setCoilA(COIL_STATE desiredState, uint16_t current = 0);

        // Sets the state of the B coil
        void setCoilB(COIL_STATE desiredState, uint16_t current = 0);

        // Calculates the correct PWM setting based on an input current
        uint32_t currentToPWM(uint16_t current) const;

        // Sets the current state of the motor
        void setState(MOTOR_STATE newState, bool clearErrors = false);

        // Get the current state of the motor
        MOTOR_STATE getState() const;

        // Calibrates the encoder and PID loop
        void calibrate();

        // Encoder object
        #if (ENABLE_ENCODER == true)
        Encoder encoder;
        #endif //ENABLE_ENCODER

        // Counter for number of overflows of TIM2 -> CNT (needs to be public for the interrupt)
        // TIM2 -> CNT is unsigned, stepOverflowOffset is unsigned, but ((TIM2 -> CNT) + stepOverflowOffset) is treated as signed value
        int32_t stepOverflowOffset = 0;

        // Microstep multiplier (used to move a custom number of microsteps per step pulse)
        uint32_t microstepMultiplier = DEFAULT_MICROSTEP_MULTIPLIER;

    // Things that shouldn't be accessed by the outside
    private:

        // Initialize the motor private constructor to prevent using it
        StepperMotor();

        // Function for getting the sign of the number (returns -1 if number is less than 0, 1 if 0 or above)
        int32_t getSign(float num);

        // Function that enables the motor
        void enable();

        // Keeps the number of handled steps of the motor
        int32_t handledStepCNT = 0;

        // Keeps the current steps of the motor
        int32_t currentStep = 0;

        #ifdef ENABLE_STEPPING_VELOCITY
            // variables to calculate the stepping interface velocity
            float angleChange = 0.0;
            uint32_t prevStepingSampleTime = 0; // micros()
            uint32_t nowStepingSampleTime = 0; // micros()
            // isStepping == true mean that three variables above can be changed
            bool isStepping = false;
        #endif


        // Microstep lock (makes sure that dips can't set a value
        // once the divisor is set by another source)
        bool microstepLocked = false;

        // Step to sine array conversion
        int32_t stepToSineArrayFactor = MAX_MICROSTEP_DIVISOR / getMicrostepping();

        // If the motor is enabled or not (saves time so that the enable and disable pins are only set once)
        MOTOR_STATE state = MOTOR_NOT_SET;

        // reversed is a multiplier for steps and angles
        // 1 - If the motor direction is normal
        // -1 - If the motor direction is inverted
        STEP_DIR reversed = POSITIVE;

        // If the motor enable is inverted
        bool enableInverted = false;

        // Analog info structures for PWM current pins
        analogInfo PWMCurrentPinInfoA;
        analogInfo PWMCurrentPinInfoB;

        // Last coil states (used to save time by not setting the pins unless necessary)
        COIL_STATE previousCoilStateA = COIL_NOT_SET;
        COIL_STATE previousCoilStateB = COIL_NOT_SET;

        // Configuration for TIM2
        TIM_HandleTypeDef tim2Config;
        TIM_Encoder_InitTypeDef tim2EncConfig;

        // HardwareTimer (required to assign interrupt)
        HardwareTimer *tim2HWTim = new HardwareTimer(TIM2);

        //object to store non volatile parameters 
        FlashParameters& mParameters;
};

// ISR functions
// Overflow handler
void overflowHandler();

#ifdef USE_MKS_STEP_CNT_SETUP
// Direction change handler
void dirChangeISR();
#endif

#endif