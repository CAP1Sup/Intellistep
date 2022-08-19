#include "encoder.h"

// Include timers.h here so there isn't a linking circle
#include "timers.h"

// Massive bit field table
const std::array<BitField_t, 88> bitFields[] = {
	{REG_ACCESS_RU,  REG_STAT,    0x2,    1,  0x00,  0},       //!< 00 bits 0:0 SRST status watch dog
	{REG_ACCESS_R,   REG_STAT,    0x2,    1,  0x00,  0},       //!< 01 bits 1:1 SWD status watch dog
	{REG_ACCESS_R,   REG_STAT,    0x4,    2,  0x00,  0},       //!< 02 bits 2:2 SVR status voltage regulator
	{REG_ACCESS_R,   REG_STAT,    0x8,    3,  0x00,  0},       //!< 03 bits 3:3 SFUSE status fuses
	{REG_ACCESS_R,   REG_STAT,    0x10,   4,  0x00,  0},       //!< 04 bits 4:4 SDSPU status digital signal processing unit
	{REG_ACCESS_RU,  REG_STAT,    0x20,   5,  0x00,  0},       //!< 05 bits 5:5 SOV status overflow
	{REG_ACCESS_RU,  REG_STAT,    0x40,   6,  0x00,  0},       //!< 06 bits 6:6 SXYOL status X/Y data out limit
	{REG_ACCESS_RU,  REG_STAT,    0x80,   7,  0x00,  0},       //!< 07 bits 7:7 SMAGOL status magnitude out limit
	{REG_ACCESS_RES, REG_STAT,    0x100,  8,  0x00,  0},       //!< 08 bits 8:8 reserved
	{REG_ACCESS_R,   REG_STAT,    0x200,  9,  0x00,  0},       //!< 09 bits 9:9 SADCT status ADC test
	{REG_ACCESS_R,   REG_STAT,    0x400,  10, 0x00,  0},       //!< 10 bits 10:10 SROM status ROM
	{REG_ACCESS_RU,  REG_STAT,    0x800,  11, 0x00,  0},       //!< 11 bits 11:11 NOGMRXY no valid GMR XY Values
	{REG_ACCESS_RU,  REG_STAT,    0x1000, 12, 0x00,  0},       //!< 12 bits 12:12 NOGMRA no valid GMR Angle Value
	{REG_ACCESS_RW,  REG_STAT,    0x6000, 13, 0x00,  0},       //!< 13 bits 14:13 SNR slave number
	{REG_ACCESS_RU,  REG_STAT,    0x8000, 15, 0x00,  0},       //!< 14 bits 15:15 RDST read status

	{REG_ACCESS_RW,  REG_ACSTAT,  0x1,    0,  0x00,  1},       //!< 15 bits 0:0 ASRST Activation of Hardware Reset
	{REG_ACCESS_RWU, REG_ACSTAT,  0x2,    1,  0x00,  1},       //!< 16 bits 1:1 ASWD Enable DSPU Watch dog
	{REG_ACCESS_RWU, REG_ACSTAT,  0x4,    2,  0x00,  1},       //!< 17 bits 2:2 ASVR Enable Voltage regulator Check
	{REG_ACCESS_RWU, REG_ACSTAT,  0x8,    3,  0x00,  1},       //!< 18 bits 3:3 ASFUSE Activation Fuse CRC
	{REG_ACCESS_RWU, REG_ACSTAT,  0x10,   4,  0x00,  1},       //!< 19 bits 4:4 ASDSPU Activation DSPU BIST
	{REG_ACCESS_RWU, REG_ACSTAT,  0x20,   5,  0x00,  1},       //!< 20 bits 5:5 ASOV Enable of DSPU Overflow Check
	{REG_ACCESS_RWU, REG_ACSTAT,  0x40,   6,  0x00,  1},       //!< 21 bits 6:6 ASVECXY Activation of X,Y Out of Limit-Check
	{REG_ACCESS_RWU, REG_ACSTAT,  0x80,   7,  0x00,  1},       //!< 22 bits 7:7 ASVEGMAG Activation of Magnitude Check
	{REG_ACCESS_RES, REG_ACSTAT,  0x100,  8,  0x00,  1},       //!< 23 bits 8:8 Reserved
	{REG_ACCESS_RWU, REG_ACSTAT,  0x200,  9,  0x00,  1},       //!< 24 bits 9:9 ASADCT Enable ADC Test vector Check
	{REG_ACCESS_RWU, REG_ACSTAT,  0x400,  10, 0x00,  1},       //!< 25 bits 10:10 ASFRST Activation of Firmware Reset
	{REG_ACCESS_RES, REG_ACSTAT,  0xF800, 11, 0x00,  1},       //!< 26 bits 15:11 Reserved

	{REG_ACCESS_RU,  REG_AVAL,    0x7FFF, 0,  0x00,  2},       //!< 27 bits 14:0 ANGVAL Calculated Angle Value (signed 15-bit)
	{REG_ACCESS_R,   REG_AVAL,    0x8000, 15, 0x00,  2},       //!< 28 bits 15:15 RDAV Read Status, Angle Value

	{REG_ACCESS_RU,  REG_ASPD,    0x7FFF, 0,  0x00,  3},       //!< 29 bits 14:0 ANGSPD Signed value, where the sign bit [14] indicates the direction of the rotation
	{REG_ACCESS_R,   REG_ASPD,    0x8000, 15, 0x00,  3},       //!< 30 bits 15:15 RDAS Read Status, Angle Speed

	{REG_ACCESS_RU,  REG_AREV,    0xFF,   0,  0x00,  4},       //!< 31 bits 8:0 REVOL Revolution counter. Increments for every full rotation in counter-clockwise direction
	{REG_ACCESS_RWU, REG_AREV,    0x7E00, 9,  0x00,  4},       //!< 32 bits 14:9 FCNT Internal frame counter. Increments every update period
	{REG_ACCESS_R,   REG_AREV,    0x8000, 15, 0x00,  4},       //!< 33 its 15:15 RDREV Read Status, Revolution

	{REG_ACCESS_RWU, REG_FSYNC,   0xFF,   0,  0x00,  5},       //!< 34 bits 8:0 TEMPR Signed offset compensated temperature value
	{REG_ACCESS_RU,  REG_FSYNC,   0xFE00, 9,  0x00,  5},       //!< 35 bits 15:9 FSYNC Frame Synchronization Counter Value

	{REG_ACCESS_RW,  REG_MOD_1,   0x3,    0,  0x00,  6},       //!< 36 bits 1:0 IIFMOD Incremental Interface Mode
	{REG_ACCESS_RW,  REG_MOD_1,   0x4,    2,  0x00,  6},       //!< 37 bits 2:2 DSPUHOLD if DSPU is on hold, no watch dog reset is performed by DSPU
	{REG_ACCESS_RES, REG_MOD_1,   0x8,    3,  0x00,  6},       //!< 38 bits 3:3 Reserved1
	{REG_ACCESS_RW,  REG_MOD_1,   0x10,   4,  0x00,  6},       //!< 39 bits 4:4 CLKSEL switch to external clock at start-up only
	{REG_ACCESS_RES, REG_MOD_1,   0x3FE0, 5,  0x00,  6},       //!< 40 bits 13:5 Reserved2
	{REG_ACCESS_RW,  REG_MOD_1,   0x6000, 13, 0x00,  6},       //!< 41 bits 15:14 FIRMD Update Rate Setting

	{REG_ACCESS_RW,  REG_SIL,     0x7,    0,  0x00,  7},       //!< 42 bits 2:0 ADCTVX Test vector X
	{REG_ACCESS_RW,  REG_SIL,     0x38,   3,  0x00,  7},       //!< 43 bits 5:3 ADCTVY Test vector Y
	{REG_ACCESS_RW,  REG_SIL,     0x40,   6,  0x00,  7},       //!< 44 bits 6:6 ADCTVEN Sensor elements are internally disconnected and test voltages are connected to ADCs
	{REG_ACCESS_RES, REG_SIL,     0x380,  7,  0x00,  7},       //!< 45 bits 9:7 Reserved1
	{REG_ACCESS_RW,  REG_SIL,     0x400,  10, 0x00,  7},       //!< 46 bits 10:10 FUSEREL Triggers reload of default values from laser fuses into configuration registers
	{REG_ACCESS_RES, REG_SIL,     0x3800, 11, 0x00,  7},       //!< 47 bits 13:11 Reserved2
	{REG_ACCESS_RW,  REG_SIL,     0x4000, 14, 0x00,  7},       //!< 48 bits 14:14 FILTINV the X- and Y-signals are inverted. The angle output is then shifted by 180°
	{REG_ACCESS_RW,  REG_SIL,     0x8000, 15, 0x00,  7},       //!< 49 bits 15:15 FILTPAR the raw X-signal is routed also to the raw Y-signal input of the filter so SIN and COS signal should be identical

	{REG_ACCESS_RW,  REG_MOD_2,   0x3,    0,  0x00,  8},       //!< 50 bits 1:0 AUTOCAL Automatic calibration of offset and amplitude synchronicity for applications with full-turn
	{REG_ACCESS_RW,  REG_MOD_2,   0x4,    2,  0x00,  8},       //!< 51 bits 2:2 PREDICT Prediction of angle value based on current angle speed
	{REG_ACCESS_RW,  REG_MOD_2,   0x8,    3,  0x00,  8},       //!< 52 bits 3:3 ANGDIR Inverts angle and angle speed values and revolution counter behavior
	{REG_ACCESS_RW,  REG_MOD_2,   0x7FF0, 4,  0x00,  8},       //!< 53 bits 14:4 ANGRANGE Changes the representation of the angle output by multiplying the output with a factor ANG_RANGE/128
	{REG_ACCESS_RES, REG_MOD_2,   0x8000, 15, 0x00,  8},       //!< 54 bits 15:15 Reserved1

	{REG_ACCESS_RW,  REG_MOD_3,   0x3,    0,  0x00,  9},       //!< 55 bits 1:0 PADDRV Configuration of Pad-Driver
	{REG_ACCESS_RW,  REG_MOD_3,   0x4,    2,  0x00,  9},       //!< 56 bits 2:2 SSCOD SSC-Interface Data Pin Output Mode
	{REG_ACCESS_RW,  REG_MOD_3,   0x8,    3,  0x00,  9},       //!< 57 bits 3:3 SPIKEF Filters voltage spikes on input pads (IFC, SCK and CSQ)
	{REG_ACCESS_RW,  REG_MOD_3,   0xFFF0, 4,  0x00,  9},       //!< 58 bits 15:4 ANG_BASE Sets the 0° angle position (12 bit value). Angle base is factory-calibrated to make the 0° direction parallel to the edge of the chip

	{REG_ACCESS_RES, REG_OFFX,    0xF,    0,  0x00, 10},       //!< 59 bits 3:0 Reserved1
	{REG_ACCESS_RW,  REG_OFFX,    0xFFF0, 4,  0x00, 10},       //!< 60 bits 15:4 XOFFSET 12-bit signed integer value of raw X-signal offset correction at 25°C

	{REG_ACCESS_RES, REG_OFFY,    0xF,    0,  0x00, 11},       //!< 61 bits 3:0 Reserved1
	{REG_ACCESS_RW,  REG_OFFY,    0xFFF0, 4,  0x00, 11},       //!< 62 bits 15:4 YOFFSET 12-bit signed integer value of raw Y-signal offset correction at 25°C

	{REG_ACCESS_RES, REG_SYNCH,   0xF,    0,  0x00, 12},       //!< 63 bits 3:0 Reserved1
	{REG_ACCESS_RW,  REG_SYNCH,   0xFFF0, 4,  0x00, 12},       //!< 64 bits 15:4 SYNCH 12-bit signed integer value of amplitude synchronicity

	{REG_ACCESS_RW,  REG_IFAB,    0x3,    0,  0x00, 13},       //!< 65 bits 1:0 IFADHYST Hysteresis (multi-purpose)
	{REG_ACCESS_RW,  REG_IFAB,    0x4,    2,  0x00, 13},       //!< 66 bits 2:2 IFABOD IFA,IFB,IFC Output Mode
	{REG_ACCESS_RW,  REG_IFAB,    0x8,    3,  0x00, 13},       //!< 67 bits 3:3 FIRUDR Initial filter update rate (FIR)
	{REG_ACCESS_RW,  REG_IFAB,    0xFFF0, 4,  0x00, 13},       //!< 68 bits 15:4 ORTHO Orthogonality Correction of X and Y Components

	{REG_ACCESS_RW,  REG_MOD_4,   0x3,    0,  0x00, 14},       //!< 69 bits 1:0 IFMD Interface Mode on IFA,IFB,IFC
	{REG_ACCESS_RES, REG_MOD_4,   0x4,    2,  0x00, 14},       //!< 70 bits 2:2 Reserved1
	{REG_ACCESS_RW,  REG_MOD_4,   0x18,   3,  0x00, 14},       //!< 71 bits 4:3 IFABRES IIF resolution (multi-purpose)
	{REG_ACCESS_RW,  REG_MOD_4,   0x1E0,  5,  0x00, 14},       //!< 72 bits 8:5 HSMPLP Hall Switch mode (multi-purpose)
	{REG_ACCESS_RW,  REG_MOD_4,   0x7E00, 9,  0x00, 14},       //!< 73 bits 15:9 TCOXT 7-bit signed integer value of X-offset temperature coefficient

	{REG_ACCESS_RW,  REG_TCO_Y,   0x7F,   0,  0x00, 15},       //!< 74 bits 7:0 CRCPAR CRC of Parameters
	{REG_ACCESS_RW,  REG_TCO_Y,   0x80,   8,  0x00, 15},       //!< 75 bits 8:8 SBIST Startup-BIST
	{REG_ACCESS_RW,  REG_TCO_Y,   0x7E00, 9,  0x00, 15},       //!< 76 bits 15:9 TCOYT 7-bit signed integer value of Y-offset temperature coefficient

	{REG_ACCESS_R,   REG_ADC_X,   0xFFFF, 0,  0x00, 16},       //!< 77 bits 15:0 ADCX ADC value of X-GMR

	{REG_ACCESS_R,   REG_ADC_Y,   0xFFFF, 0,  0x00, 17},       //!< 78 bits 15:0 ADCY ADC value of Y-GMR

	{REG_ACCESS_RU,  REG_D_MAG,   0x3FF,  0,  0x00, 18},       //!< 79 bits 9:0 MAG Unsigned Angle Vector Magnitude after X, Y error compensation (due to temperature)
	{REG_ACCESS_RES, REG_D_MAG,   0xFC00, 10, 0x00, 18},       //!< 80 bits 15:10 Reserved1

	{REG_ACCESS_RU,  REG_T_RAW,   0x3FF,  0,  0x00, 19},       //!< 81 bits 9:0 TRAW Temperature Sensor Raw-Value at ADC without offset
	{REG_ACCESS_RES, REG_T_RAW,   0xFC00, 10, 0x00, 19},       //!< 82 bits 14:10 Reserved1
	{REG_ACCESS_RU,  REG_T_RAW,   0x8000, 15, 0x00, 19},       //!< 83 bits 15:15 TTGL Temperature Sensor Raw-Value Toggle toggles after every new temperature value

	{REG_ACCESS_RU,  REG_IIF_CNT, 0x7FFF, 0,  0x00, 20},       //!< 84 bits 14:0 IIFCNT 14 bit counter value of IIF increments
	{REG_ACCESS_RES, REG_IIF_CNT, 0x8000, 15, 0x00, 20},       //!< 85 bits 15:14 Reserved1

	{REG_ACCESS_R,   REG_T25O,    0x1FFF, 0,  0x00, 21},       //!< 86 bit 8:0 T250 Signed offset value at 25°C temperature; 1dig=0.36°C
	{REG_ACCESS_RES, REG_T25O,    0xFE00, 9,  0x00, 21},       //!< 87 bits 15:9 Reserved1
};


// Constructor for the encoder
Encoder::Encoder() {

    // Setup pin A5, A6, and A7
    GPIOInitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIOInitStruct.Mode = GPIO_MODE_AF_PP;
    GPIOInitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIOInitStruct);

    // Setup the gpioInitStruct for later (hack for reading from encoder,
    // BTT decided that they would do it incorrectly)
    GPIOInitStruct.Pin = GPIO_PIN_7;

    // Enable the clock for the SPI bus
    __HAL_RCC_SPI1_CLK_ENABLE();

    // Set the peripheral to be used
    spiConfig.Instance = SPI1;

    // Configure the settings for transactions
    spiConfig.Init.Direction = SPI_DIRECTION_2LINES;
    spiConfig.Init.Mode = SPI_MODE_MASTER;
    spiConfig.Init.DataSize = SPI_DATASIZE_8BIT;
    spiConfig.Init.CLKPolarity = SPI_POLARITY_LOW;
    spiConfig.Init.CLKPhase = SPI_PHASE_2EDGE;
    spiConfig.Init.NSS = SPI_NSS_SOFT;
    spiConfig.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    spiConfig.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spiConfig.Init.CRCPolynomial = 7;

    // Initialize the SPI bus with the parameters we set
    if (HAL_SPI_Init(&spiConfig) != HAL_OK) {
        #if (ENABLE_SERIAL != 0)
            Serial.println(F("SPI not initialized!"));
        #endif
    }

    // Set the chip select pin high, disabling the encoder's communication
    pinMode(ENCODER_CS_PIN, OUTPUT);
    GPIO_WRITE(ENCODER_CS_PIN, HIGH);

    // Reset the encoder's firmware
    //writeToRegister(ENCODER_ACT_STATUS_REG, 0x401);

    // Setup the moving average calculations
    speedAvg.begin(RPM_AVG_READINGS);
    rawSpeedAvg.begin(SPEED_AVG_READINGS);
    accelAvg.begin(ACCEL_AVG_READINGS);
    incrementAvg.begin(ANGLE_AVG_READINGS);
    absAngleAvg.begin(ANGLE_AVG_READINGS);
    absIncrementsAvg.begin(ANGLE_AVG_READINGS);
    rawTempAvg.begin(TEMP_AVG_READINGS);

    // Set the last raw revolution value (used to detect revolution change)
    lastRawRev = getRawRev();

    // Set the offsets
    this -> zero();

    // Set the correct starting values for the estimation if using estimation
    #ifdef ENCODER_SPEED_ESTIMATION
        lastEncoderAngle = getAbsoluteAngleAvg();
        lastAngleSampleTime = micros();
    #endif

    // Set the correct start time for the overtemp protection
    #ifdef ENABLE_OVERTEMP_PROTECTION
        lastOvertempTime = sec();
    #endif
}

/**
  * @brief  Sets the mode for GPIO7. This is done quite often so this function is written to improve speed. NOTE: This can only be used with GPIO_MODE_AF_PP and GPIO_MODE_AF_OD
  * @param  GPIO_Init: pointer to a GPIO_InitTypeDef structure that contains
  *         the configuration information for the specified GPIO peripheral.
  * @retval None
  */
void Encoder::setGPIO7Mode(uint32_t mode) {

    // Setup variables (most are determined by compiler)
    GPIO_TypeDef *GPIOx = GPIOA;
    constexpr uint32_t position = 0x07u; // GPIO7 (requires 7 bit shifts)
    __IO uint32_t *configregister = &GPIOx->CRL; /* Store the address of CRL or CRH register based on pin number */
    constexpr uint32_t registeroffset = (position << 2u);       /* offset used during computation of CNF and MODE bits placement inside CRL or CRH register */
    uint32_t config = 0x00u;

    // Set the pin config (only in the variable yet)
    if (mode == GPIO_MODE_AF_PP) {
        config = GPIO_SPEED_FREQ_HIGH + GPIO_CR_CNF_AF_OUTPUT_PP;
    }
    else if (mode == GPIO_MODE_AF_OD) {
        config = GPIO_SPEED_FREQ_HIGH + GPIO_CR_CNF_AF_OUTPUT_OD;
    }

    /* Apply the new configuration of the pin to the register */
    MODIFY_REG((*configregister), ((GPIO_CRL_MODE0 | GPIO_CRL_CNF0) << registeroffset), (config << registeroffset));
}


// Read the value of a register
errorTypes Encoder::readRegister(uint16_t registerAddress, uint16_t &data) {

    // Disable interrupts (cannot be interrupted)
    disableInterrupts();

    // Create an accumulator for error checking
    errorTypes error = NO_ERROR;

    // Pull CS low to select encoder
    GPIO_WRITE(ENCODER_CS_PIN, LOW);

    // Add read bit to address
    registerAddress |= ENCODER_READ_COMMAND;
    registerAddress |= SAFE_HIGH;

    // Setup RX and TX buffers
    uint8_t rx1buf[2];
    uint8_t rx2buf[2];
    uint8_t txbuf[2] = { uint8_t(registerAddress >> 8), uint8_t(registerAddress) };

    // Send address we want to read, response seems to be equal to request
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rx1buf, 2, 10);

    // Set the MOSI pin to open drain
    setGPIO7Mode(GPIO_MODE_AF_OD);

    // Send 0xFFFF (like BTT code), this returns the wanted value
    txbuf[0] = 0xFF, txbuf[1] = 0xFF;

    // Send that we want to read the data, then read it
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rx2buf, 2, 10);

    // Combine the first rxbuf into a single, unsigned 16 bit value
    //uint16_t combinedRX1Buf = (rx1buf[0] << 8 | rx1buf[1]);
    //uint16_t combinedRX2Buf = (rx2buf[0] << 8 | rx2buf[1]);

    // Check to see if the communication was valid
    //error = checkSafety(combinedRX1Buf, registerAddress, &combinedRX2Buf, 1);

    // Set MOSI back to Push/Pull
    setGPIO7Mode(GPIO_MODE_AF_PP);

    // Deselect encoder
    GPIO_WRITE(ENCODER_CS_PIN, HIGH);

    // Set data in the specified location (depending on errors)
    if (error == NO_ERROR) {

        // No errors, good to set the correct value
        data = (rx2buf[0] << 8 | rx2buf[1]);
    }
    else {
        // Zero the value if there is an error
        data = 0;
    }

    // Enable interrupts, we're finished
    enableInterrupts();

    // Return error
    return error;
}


// Read multiple registers
void Encoder::readMultipleRegisters(uint16_t registerAddress, uint16_t* data, uint16_t dataLength) {

    // Pull CS low to select encoder
    GPIO_WRITE(ENCODER_CS_PIN, LOW);

    // Setup TX and RX buffers
    registerAddress |= ENCODER_READ_COMMAND + dataLength;
    uint8_t txbuf[dataLength * 2] = { uint8_t(registerAddress >> 8), uint8_t(registerAddress) };
    uint8_t rxbuf[dataLength * 2];

    // Send address we want to read, response seems to be equal to request
    disableInterrupts();
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, 2, 10);
    enableInterrupts();

    // Set the MOSI pin (GPIO 7) to open drain
    setGPIO7Mode(GPIO_MODE_AF_OD);

    // Send 0xFFFF (like BTT code), this returns the wanted value
    // Array length is doubled as we're using 8 bit values instead of 16
    for (uint8_t i = 0; i < dataLength * 2; i++) {
        txbuf[i] = 0xFF;
    }

    // Communicate that we want to read the value
    disableInterrupts();
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, dataLength * 2, 10);
    enableInterrupts();

    // Write the received data into the array
    for (uint8_t i = 0; i < dataLength; i++) {
        data[i] = rxbuf[i * 2] << 8 | rxbuf[i * 2 + 1];
    }

    // Set MOSI (GPIO 7) back to Push/Pull
    setGPIO7Mode(GPIO_MODE_AF_PP);

    // Deselect encoder
    GPIO_WRITE(ENCODER_CS_PIN, HIGH);
}


// Write a value to a register
// ! Untested
void Encoder::writeToRegister(uint16_t registerAddress, uint16_t data) {

    // Pull CS low to select encoder
    GPIO_WRITE(ENCODER_CS_PIN, LOW);

    // Setup TX and RX buffers
    registerAddress |= ENCODER_WRITE_COMMAND + 1;
    uint8_t txbuf[2] = { uint8_t(registerAddress >> 8), uint8_t(registerAddress) };
    uint8_t rxbuf[2];

    // Send address we want to write, response seems to be equal to request
    disableInterrupts();
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, 2, 10);
    enableInterrupts();

    // Set the MOSI pin to open drain
    setGPIO7Mode(GPIO_MODE_AF_OD);

    // Set the data to be written
    txbuf[0] = uint8_t(data >> 8), txbuf[1] = uint8_t(data);

    // Write the data
    disableInterrupts();
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, 2, 10);
    enableInterrupts();

    // Set MOSI back to Push/Pull
    setGPIO7Mode(GPIO_MODE_AF_PP);

    // Deselect encoder
    GPIO_WRITE(ENCODER_CS_PIN, HIGH);
}


// Checks the encoder's response for any errors
// Warning: this function cannot be interrupted, so make sure that it is called in a function that disables interrupts
errorTypes Encoder::checkSafety(uint16_t safety, uint16_t command, uint16_t* readreg, uint16_t length) {

    // A final accumulator for there was an error
    errorTypes error;

    // Check for system errors
	if (!((safety) & ENCODER_SYSTEM_ERROR_MASK)) {
		error = SYSTEM_ERROR;
		resetSafety();

	} else if (!((safety) & ENCODER_INTERFACE_ERROR_MASK)) {

        // Check for interface errors
        error = INTERFACE_ACCESS_ERROR;

	} else if (!((safety) & ENCODER_INV_ANGLE_ERROR_MASK)) {

        // Check for invalid angles
    	error = INVALID_ANGLE_ERROR;
        resetSafety();

	} else {
        // If there have been no errors so far, then check the CRC

        // Check the length of the message, then create the buffer needed to hold it
		uint16_t lengthOfTemp = length * 2 + 2;
		uint8_t temp[lengthOfTemp] = {0, 0};

        // Read out command to the first two bytes of the message
		temp[0] = (uint8_t)(command >> 8);
		temp[1] = (uint8_t)(command);

		for (uint16_t index = 0; index < length; index++) {
			temp[2 + (2 * index)] =     (uint8_t)(readreg[index] >> 8); // Reads the first byte of the 16 bit message
			temp[2 + (2 * index) + 1] = (uint8_t)(readreg[index]);      // Reads the second byte
		}

		uint8_t crcReceivedFinal = (uint8_t)(safety); // Checks the second byte of the safety
		uint8_t crc = calcCRC(temp, lengthOfTemp);

        // Make sure that the calculated CRC is equal to the sent CRC
		if (crc != crcReceivedFinal) {
			error = CRC_ERROR;
            resetSafety();
		} else {
			error = NO_ERROR;
		}
	}

    // Return the error that was found (if any)
	return (error);
}


// Calculates the CRC of an array of 8 bit messages
uint8_t Encoder::calcCRC(uint8_t *data, uint8_t length) {

    // Create an accumulator for the CRC
	uint32_t crc;

    // Set the CRC to the seed
	crc = CRC_SEED;

    // Loop through the message, marking in the CRC if there are any errors
	for (uint16_t i = 0; i < length; i++)
	{
		crc ^= data[i];
		for (uint8_t bit = 0; bit < 8; bit++)
		{
			if ((crc & 0x80) != 0)
			{
				crc <<= 1;
				crc ^= CRC_POLYNOMIAL;
			}else{
				crc <<= 1;
			}
		}
	}

    // Check if the CRC has changed at all from the original seeding
	return ((~crc) & CRC_SEED);
}


// Resets the safety check of the encoder
// Warning: this function cannot be interrupted, so make sure that it is called in a function that disables interrupts
void Encoder::resetSafety() {

    // Build the command
	uint16_t command = ENCODER_READ_COMMAND + SAFE_HIGH;

    // Separate it out into tx packets and create rx buffer (rx buffer doesn't matter, so it can just be discarded)
    uint8_t txbuf[2] = { uint8_t(command >> 8), uint8_t(command) };
	uint8_t rxbuf[2];

    // Send the command on the first transmission
    disableInterrupts();
	HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, 2, 10);
    enableInterrupts();

    // Only need to read on the 2nd, 3rd, and 4th, so just send a blank tx message
    txbuf[0] = 0xFF, txbuf[1] = 0xFF;

    // TX/RX twice, just reading the response (response doesn't matter)
    disableInterrupts();
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, 2, 10);
    HAL_SPI_TransmitReceive(&spiConfig, txbuf, rxbuf, 2, 10);
    enableInterrupts();
}


// Get a bit field from a register
uint16_t Encoder::getBitField(BitField_t bitField) {

    // Check to make sure that the value can be read
	if ((REG_ACCESS_R & bitField.regAccess) == REG_ACCESS_R) {
		if ((REG_ACCESS_U & bitField.regAccess) == REG_ACCESS_U) {
			// ! p->sBus->triggerUpdate();
		}

        // Return the value at the address (if the read was successful)
        if (readRegister(addrFields[bitField.posMap].regAddress, regMap[bitField.posMap]) == NO_ERROR) {
            return ((regMap[bitField.posMap] & bitField.mask) >> bitField.position);
        }
    }

    // Not able to read that address, just return a -1
    return -1;
}


// Write out a bit field to the register
void Encoder::setBitField(BitField_t bitField, uint16_t bitFNewValue) {

    // Write the correct value
	if ((REG_ACCESS_W & bitField.regAccess) == REG_ACCESS_W) {
		regMap[bitField.posMap] = (regMap[bitField.posMap] & ~bitField.mask) | ((bitFNewValue << bitField.position) & bitField.mask);
		writeToRegister(addrFields[bitField.posMap].regAddress, regMap[bitField.posMap]);
	}
}


// Reads the raw momentary encoder increments value from the angle register (unadjusted)
uint16_t Encoder::getRawIncrements() {

    // Create an accumulator for the raw data
    uint16_t rawData;

    // Loop until a valid reading
    while (readRegister(ENCODER_ANGLE_REG, rawData) != NO_ERROR);

    // Delete the first bit, saving the last 15
    return rawData & DELETE_BIT_15;
}


// Returns the raw average encoder increments value from the angle register (unadjusted)
uint16_t Encoder::getRawIncrementsAvg() {

    // Read the momentary rawData
    // Add the value to the filter
    incrementAvg.add(getRawIncrements());

    // Return the average
    return incrementAvg.get();
}


// Returns the absolute momentary encoder increments (adjusted) in the range  of +/-335544 rev's of shaft
increments_t Encoder::getAbsoluteIncrements() {

    return ((getRev() * POW_2_15 + getRawIncrements()) >> REJECT_ENCODERS_LSB) - startupIncrementsOffset;
}


// Returns the absolute average encoder increments (adjusted) in the range  of +/-335544 rev's of shaft
increments_t Encoder::getAbsoluteIncrementsAvg() {

    // Add a new value to the average
    absIncrementsAvg.add(getRev() * POW_2_15 + getRawIncrements());

    // Return the average
    return absIncrementsAvg.get() - startupIncrementsOffset;
}


// Reads the raw momentary value from the angle of the encoder (adjusted)
double Encoder::getRawAngle() {

    // Read the momentary rawData
    // Calc the value (equation from TLE5012 library)
    return 360.0 * (getRawIncrements() >> REJECT_ENCODERS_LSB) / (POW_2_15 >> REJECT_ENCODERS_LSB) - encoderStepOffset;
}


// Reads the raw average value from the angle of the encoder (adjusted)
double Encoder::getRawAngleAvg() {

    // Read the raw average increments
    // Calc the value (equation from TLE5012 library)
    return 360.0 * getRawIncrementsAvg() / POW_2_15 - encoderStepOffset;
}


// Returns a smoothed value of angle of the encoder
// More expensive than getAngle(), but transitions between 0 and 360 are smoother
double Encoder::getSmoothAngle() {

    // Get the absolute angle, then take the mod of 360 to find the shaft rotation (without revolutions)
    return (fmod(getAbsoluteAngleAvg(), 360.0));
}


// Returns a smoothed value of angle of the encoder
// More expensive than getAngle(), but transitions between 0 and 360 are smoother
double Encoder::getSmoothAngle(double currentAbsAngle) {

    // Get the absolute angle, then take the mod of 360 to find the shaft rotation (without revolutions)
    return (fmod(currentAbsAngle, 360.0));
}


// Reads the momentary value for the angle of the encoder (ranges from 0-360)
double Encoder::getAngle() {
    return (getRawAngle() - startupAngleOffset);
}


// Reads the average value for the angle of the encoder (ranges from 0-360)
double Encoder::getAngleAvg() {
    return (getRawAngleAvg() - startupAngleOffset);
}


// For average velocity calculations instead of hardware readings from the TLE5012
// Returns the speed of the encoder in deg/s
// ! Needs fixed yet, readings are off
double Encoder::getEstimSpeed() {

    // Get the newest angle
    double newAngle = getAbsoluteAngleAvg();

    // Sample time
    uint32_t currentTime = micros();

    // Compute the average velocity
    double avgVelocity = 1000000.0 * (newAngle - lastEncoderAngle) / (currentTime - lastAngleSampleTime);

    // Correct the last angle and sample time
    lastEncoderAngle = newAngle;
    lastAngleSampleTime = currentTime;

    // Add the value to the averaging list
    speedAvg.add(avgVelocity);

    // Return the averaged velocity in deg/s
    return speedAvg.get();
}


// For average velocity calculations instead of hardware readings from the TLE5012
// Returns the speed of the encoder in deg/s
// ! Needs fixed yet, readings are off
double Encoder::getEstimSpeed(double currentAbsAngle) {

    // Sample time
    uint32_t currentTime = micros();

    // Compute the average velocity
    double avgVelocity = 1000000.0 * (currentAbsAngle - lastEncoderAngle) / (currentTime - lastAngleSampleTime);

    // Correct the last angle and sample time
    lastEncoderAngle = currentAbsAngle;
    lastAngleSampleTime = currentTime;

    // Add the value to the averaging list
    speedAvg.add(avgVelocity);

    // Return the averaged velocity in deg/s
    return speedAvg.get();
}


#ifdef ENCODER_SPEED_ESTIMATION
// Returns true if the encoder's minimum speed sample interval has been exceeded
bool Encoder::sampleTimeExceeded() {
    return (micros() - lastAngleSampleTime > SPD_EST_MIN_INTERVAL);
}
#endif

int16_t Encoder::getRawSpeed() {

    // Prepare the variables to store data in
	int16_t rawData;

    // Read the encoder
    while (readRegister(ENCODER_SPEED_REG, (uint16_t &)rawData) != NO_ERROR);

    // Delete everything before the 14 LSB's
    rawData <<= 1;

    // If bit 14 is set, the value is negative
    // Propagate sign of integer
    rawData >>= 1;

    return rawData;
}

// Reads the speed of the encoder in deg/s
double Encoder::getSpeed() {

    // Prepare the variables to store data in
	uint16_t rawData[4];

    // Read the encoder, modifying the array
    readMultipleRegisters(ENCODER_SPEED_REG, rawData, sizeof(rawData) / sizeof(uint16_t));

	// Get raw speed reading
	int16_t rawSpeed = rawData[0];

        // Delete everything before the 14 LSB's
        rawSpeed <<= 1;

        // If bit 14 is set, the value is negative
        // Propagate sign of integer
        rawSpeed >>= 1;

	// Get FIR_MD from bits 15 and 16 of register 0x06
	uint16_t firMD = rawData[3] >> 14;

	// Determine sensor update rate from FIR_MD
	double firMDVal;
    switch (firMD) {
        case 0:
            firMDVal = 21.3;
            break;
        case 1:
            firMDVal = 42.7;
            break;
        case 2:
            firMDVal = 85.3;
            break;
        case 3:
            firMDVal = 170.6;
            break;
        default:
            firMDVal = 0.0; // ??? zero division
            break;
    }

    // Calculate and average angle speed in degree per second
	rawSpeedAvg.add(rawSpeed);

    // Return the result in deg/s
    return (1000000.0 * 0.5 * (360.0 / POW_2_15) * rawSpeedAvg.getDouble() / firMDVal);
}


// Calculates the angular acceleration. Done by looking at position over time^2
double Encoder::getAccel() {

    // Get the newest angle
    double newAngle = getAbsoluteAngleAvg();

    // Sample time
    uint32_t currentTime = micros();

    // Compute the average velocity (extra pow at beginning is needed to convert microseconds to seconds)
    double avgAccel = (newAngle - lastEncoderAngle) / pow(1000000 * (currentTime - lastAngleSampleTime), 2);

    // Correct the last angle and sample time
    lastEncoderAngle = newAngle;
    lastAngleSampleTime = currentTime;

    // Add the value to the averaging list
    accelAvg.add(avgAccel);

    // Return the averaged velocity
    return accelAvg.get();
}


// Calculates the angular acceleration. Done by looking at position over time^2
double Encoder::getAccel(double currentAbsAngle) {

    // Sample time
    uint32_t currentTime = micros();

    // Compute the average velocity (extra pow at beginning is needed to convert microseconds to seconds)
    double avgAccel = (currentAbsAngle - lastEncoderAngle) / pow(1000000 * (currentTime - lastAngleSampleTime), 2);

    // Correct the last angle and sample time
    lastEncoderAngle = currentAbsAngle;
    lastAngleSampleTime = currentTime;

    // Add the value to the averaging list
    accelAvg.add(avgAccel);

    // Return the averaged velocity
    return accelAvg.get();
}


// Reads the raw momentary temperature of the encoder
int16_t Encoder::getRawTemp() {

    // Create an accumulator for the raw data
    int16_t rawData;

    // Loop until a valid reading
    while (readRegister(ENCODER_TEMP_REG, (uint16_t &)rawData) != NO_ERROR);

    // Delete everything before the 9 LSB's
    rawData <<= 7;

    // If bit 9 is set, the value is negative
    // Propagate sign of integer
    rawData >>= 7;

    // Return the raw momentary value
    return rawData;
}


// Reads the temperature of the encoder
double Encoder::getTemp() {

    // Get the momentary temperature
    int16_t rawTemp = getRawTemp();

    // Add to MovingAverage filter
    rawTempAvg.add(rawTemp);

    // Calculate the new temperature (equation from TLE5012 library)
    double temp = (rawTempAvg.getDouble() + TEMP_OFFSET) / TEMP_DIV;

    // Only compile if overtemp protection is enabled
    #ifdef ENABLE_OVERTEMP_PROTECTION

    // Check to see if there was a overtemp disable
    if (motor.getState() == MOTOR_STATE::OVERTEMP) {

        // There was an overtemp previously, check if it should be cleared
        if (temp < OVERTEMP_SHUTDOWN_CLEAR_TEMP) {
            motor.setState(DISABLED, true);
        }
    }

    // If overtemp protection is enabled, we should check it now
    else if (temp >= OVERTEMP_THRESHOLD_TEMP) {

        // Check if the motor needs to be overtemp disabled
        if (temp >= OVERTEMP_SHUTDOWN_TEMP) {

            // Disable the motor with an overtemp
            motor.setState(OVERTEMP, true);
        }

        // Value is too high, we might need to reduce it
        // Make sure that the last time increment is far enough behind
        else if (sec() - lastOvertempTime >= OVERTEMP_INTERVAL) {

            // We're good to go, it hasn't been too long
            motor.setRMSCurrent(motor.getRMSCurrent() - OVERTEMP_INCREMENT);

            // Fix the last overtemp time
            lastOvertempTime = sec();
        }
    }

    #endif // ENABLE_OVERTEMP_PROTECTION

    // Return the temperature
    return temp;
}


// Gets the raw revolutions from the motor in range [-258 ... +257]
int16_t Encoder::getRawRev() {

    // Create an accumulator for the raw data and converted data
    int16_t rawData;

    // Loop continuously until there is no error
    while (readRegister(ENCODER_ANGLE_REV_REG, (uint16_t &)rawData) != NO_ERROR);

    // Delete the first 7 bits, they are not needed
    // Delete everything before the 9 LSB's
    rawData <<= 7;

    // If bit 8 is set, the value is negative
    // Propagate sign of integer
    rawData >>= 7;

    // Return the angle measurement
    return rawData;
}


// Gets the revolutions of the motor
int32_t Encoder::getRev() {

    // Get the raw value
    int16_t rawRevNow = getRawRev();
    if ((lastRawRev > 0) && (rawRevNow < 0)) // overflow
        revolutions++;
    else if ((lastRawRev < 0) && (rawRevNow > 0)) // borrow
        revolutions--;
    lastRawRev = rawRevNow;
    return (revolutions * 512 + rawRevNow - startupRevOffset);
}


// Gets the average absolute angle of the motor
double Encoder::getAbsoluteAngleAvg() {

    // Add a new value to the average
    absAngleAvg.add((float)((getRev() * 360) + getAngle()));

    // Return the average
    return absAngleAvg.getDouble();
}


// Gets the momentary absolute angle of the motor
double Encoder::getAbsoluteAngle() {
    return (double)((getRev() * 360) + getAngle());
}


// Gets the absolute angle of the motor, just returns a float
float Encoder::getAbsoluteAngleAvgFloat() {

    // Add a new value to the average
    absAngleAvg.add((float)((getRev() * 360) + getAngle()));

    // Return the average
    return absAngleAvg.get();
}


// Clears the old absolute angle readings out
void Encoder::clearAbsoluteAngleAvg() {

    // Read the encoder's absolute angle the number of readings the average stores
    for (uint8_t readings = 0; readings < ANGLE_AVG_READINGS; readings++) {

        // Get the angle, then wait for 10ms to allow encoder to update
        getAbsoluteAngleAvg();
        delay(10);
    }
}


// Gets the encoder's step offset (used for calibration)
double Encoder::getStepOffset() {
    return encoderStepOffset;
}


// Sets the encoder's step offset (used for calibration)
void Encoder::setStepOffset(double offset) {
    encoderStepOffset = offset;
}


// Sets the encoder's Increments
void Encoder::setIncrementsOffset(uint16_t offset) {
    startupIncrementsOffset = offset;
}


// Set encoder zero point
void Encoder::zero() {

    // Populate the average angle reading table
    for (uint8_t index = 0; index < (ANGLE_AVG_READINGS * 2); index++) {

        // Continuously loop, filling the avg (no need to call getAngleAvg(),
        // as it uses getRawIncrementsAvg() to get its value)
        getRawIncrementsAvg();
        delay(10);
    }

    // Fix offsets
    startupAngleOffset = getRawAngleAvg();
    startupRevOffset = getRawRev();
    startupIncrementsOffset = getRawIncrementsAvg();
}