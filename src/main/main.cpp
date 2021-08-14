/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "buttons.h"
#include "canMessaging.h"
#include "serial.h"
#include "flash.h"
#include "encoder.h"
#include "oled.h"
#include "led.h"
#include "cube.h"

// Create a new motor instance
StepperMotor motor = StepperMotor();

// Run the setup
void setup() {

    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Set processor up
    SystemInit();

    NVIC_SetPriorityGrouping(3); // __NVIC_PRIO_BITS

    // Configure the system clock
    #if defined(SYSCLK_SRC_HSE_16)
        #if SYSCLK_FREQ == 72
            SystemClock_Config_HSE_16M_SYSCLK_72M();
        #else
            #error "Unsupported oscillator speed"
        #endif
    #elif defined(SYSCLK_SRC_HSE_8)
        #if SYSCLK_FREQ == 72
            SystemClock_Config_HSE_8M_SYSCLK_72M();
        #elif SYSCLK_FREQ == 128
            SystemClock_Config_HSE_8M_SYSCLK_128M();
        #else
            #error "Unsupported oscillator speed"
        #endif
    #elif defined(SYSCLK_SRC_HSI)
        #if SYSCLK_FREQ == 64
            SystemClock_Config_HSI_8M_SYSCLK_64M();
        #else
            #error "Unsupported oscillator speed"
        #endif
    #else
	    #error "Unsupported oscillator source"
    #endif

    #ifdef CHECK_MCO_OUTPUT
        MCO_GPIO_Init();
    #endif

    // Encoder speed debugging
    #ifdef CHECK_ENCODER_SPEED
        while(true) {
            GPIO_WRITE(LED_PIN, HIGH);
            motor.encoder.getRawAngleAvg();
            GPIO_WRITE(LED_PIN, LOW);
            delayMicroseconds(1);
        }
    #endif

    // Debugging for GPIO output switching
    #ifdef CHECK_GPIO_OUTPUT_SWITCHING
        PA_8_GPIO_Init();
        while(true) {
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);
            GPIO_WRITE(PA_8, HIGH);

            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
            GPIO_WRITE(PA_8, LOW);
        }
    #endif

    // Initialize the LED
    #ifdef ENABLE_LED
        initLED();
    #endif

    // Zero the encoder
    motor.encoder.zero();

    // Only run if the OLED is enabled
    #ifdef ENABLE_OLED

        // Initialize the OLED
        initOLED();

        // Show the bootscreen
        showBootscreen();

        // Wait for 3 seconds so everything can boot and user can read the LCD
        delay(3000);

        // Set the inversion (if specified)
        #ifdef INVERTED_DIPS
            setDipInverted(true);
        #else
            setDipInverted(false);
        #endif

        // Initialize the buttons (for menu)
        initButtons();
    #endif

    // Initialize the serial bus
    #ifdef ENABLE_SERIAL
        initSerial();
    #endif

    // Initialize the CAN bus
    #ifdef ENABLE_CAN
        // Initialize the CAN bus
        initCAN();
    #endif

    // Clear the display, then write that we're using the closed loop mode
    //clearOLED();
    //writeOLEDString(0, 0, "Close Loop Mode");

    // Check if the board is calibrated. Need to force calibration if the board isn't calibrated
    if (!isCalibrated()) {

        // Only display to screen if the screen is enabled
        #ifdef ENABLE_OLED

            // Display that the motor is not calibrated
            clearOLED();
            writeOLEDString(0, 0,               F("NOT"), false);
            writeOLEDString(0, LINE_HEIGHT * 1, F("Calibrated!"), false);
            writeOLEDString(0, LINE_HEIGHT * 2, F("Please"), false);
            writeOLEDString(0, LINE_HEIGHT * 3, F("calibrate"), true);
            delay(3000);

            // Display that the select key can be clicked to run calibration
            clearOLED();
            writeOLEDString(0, 0,               F("Use the"), false);
            writeOLEDString(0, LINE_HEIGHT * 1, F("select key"), false);
            writeOLEDString(0, LINE_HEIGHT * 2, F("to calibrate"), false);
            writeOLEDString(0, LINE_HEIGHT * 3, F("Requires power"), true);
        #endif

        // Continuously check to see if the select key is clicked (depth index would increase when clicked)
        while(true) {

            #ifdef ENABLE_OLED
                // Check to see if any of the buttons are pressed
                checkButtons(false, true);
            #endif

            // ! Only for testing
            //blink();

            // Only if the OLED is needed
            #ifdef ENABLE_OLED

                // Check to see if the menu button has been clicked
                if (getMenuDepth() > 0) {

                    // Calibrate the motor (board reboots afterward)
                    // Reboot the chip
                    motor.calibrate();
                }

            #else
                // Just jump to calibrating the motor (board reboots afterward)
                // Reboot the chip
                motor.calibrate();
            #endif
        }
    }
    else {
        // There is a calibration, load it and move on to the loop

        // Only include the extensive flash printout if the OLED is enabled
        #ifdef ENABLE_OLED

            // Let the user know that the calibration was successfully loaded
            clearOLED();
            writeOLEDString(0, 0,               F("Calibration"), false);
            writeOLEDString(0, LINE_HEIGHT * 1, F("OK!"), false);

            // Write base string for flash loading
            writeOLEDString(0, LINE_HEIGHT * 2, F("Flash loaded"), false);

            // Attempt to load the parameters from flash
            if (loadParameters() == FLASH_LOAD_SUCCESSFUL) {
                writeOLEDString(0, LINE_HEIGHT * 3, F("successfully"), true);
            }
            else {
                writeOLEDString(0, LINE_HEIGHT * 3, F("unsuccessfully"), true);
            }

            // Let the user read the message
            delay(1000);

            // Clear the display
            clearOLED();

            // Write out the first data to the screen (makes sure that the first write isn't interrupted)
            displayMotorData();
        #else
            // Nothing special, just try to load the flash data
            loadParameters();
        #endif

        // Setup the motor timers and interrupts
        setupMotorTimers();
    }

    // Setup the motor for use (should be enabled at startup)
    // The jump when power is on
    motor.setState(ENABLED, true);

    // Delay for move to power on position
    // Needs the motor timers
    delay(1000);

    // Zero the encoder after motor is enabled
    motor.encoder.zero();
}


// Main loop
void loop() {

    // Check the dip switches
    checkDips();

    // Check to see if serial data is available to read
    #ifdef ENABLE_SERIAL
        runSerialParser();
    #endif

    #ifdef ENABLE_OLED
        // Check the buttons
        checkButtons(true);

        // Only update the display if the motor data is being displayed, buttons update the display when clicked
        if (getMenuDepth() == MOTOR_DATA) {
            displayMotorData();
        }
    #endif

    // We need a little delay to allow the motor time to process if it needs it
    #ifdef ENABLE_BLINK
        // ! Only for testing
        blink();
    #else
        delay(50);
    #endif
}


// ! Only here for testing
void blink() {
    GPIO_WRITE(LED_PIN, HIGH);
    delay(500);
    GPIO_WRITE(LED_PIN, LOW);
    delay(500);
}
