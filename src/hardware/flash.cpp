#include "flash.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_flash.h"



iflashParameters FlashParameters::GetDefaults()
{
    iflashParameters params = {
            .FlashVersion = FlashFormatVersion,
            .calibration = {
                .calibrated = false,
                .stepOffset = 0
            },
        #if (ENABLE_DYNAMIC_CURRENT != false )
            .current = {
                .accelCurrent = DYNAMIC_ACCEL_CURRENT,
                .idleCurrent = DYNAMIC_IDLE_CURRENT,
                .maxCurrent = DYNAMIC_MAX_CURRENT
            },
        #else
            // Save the static motor current in the idle and max slot of the dynamic current setting, zeroing the acceleration
            .current = {
                .accelCurrent = (uint16_t)0,
                .idleCurrent = STATIC_RMS_CURRENT,
                .maxCurrent = STATIC_RMS_CURRENT
            },
        #endif
            .resolution = {
                .fullStepAngle = 1.8,
                .Microstepping = 1,
                .microstepsMultipier = DEFAULT_MICROSTEP_MULTIPLIER
            },
            .direction = {
                .reversed = false,
                .enableInversion = false
            },
            
        #if (ENABLE_PID == true)
            .pid = {
                .P = DEFAULT_P,
                .I = DEFAULT_I,
                .D = DEFAULT_D
            },
        #else
            .pid = {
                .P = 0,
                .I = 0,
                .D = 0
            },
        #endif

            .canIndex = NONE,
            .DipSwitches = {
                .active = true,
                .isDipInverted = getDipInverted()
            },
            .crc = 0
        };
    params.crc = computeCRC(params);
    return params;
}

FlashParameters& FlashParameters::getInstance()
{
    static FlashParameters singleton;
    return singleton;
}

/*
 // used to display the size of the storage struct used
#define COMPILE_TIME_SIZEOF(t)      template<int s> struct SIZEOF_ ## t ## _IS; \
                                    struct foo { \
                                        int a,b; \
                                    }; \
                                    SIZEOF_ ## t ## _IS<sizeof(t)> SIZEOF_ ## t ## _IS;
 COMPILE_TIME_SIZEOF(iflashParameters)
*/

#define BUILD_SANITY_CHECK_flashStorageSize(condition) ((void)sizeof(char[1 - 2*!(condition)]))
FlashParameters::FlashParameters()
{

    //some trick to create compile issue if iflashParameters is bigger than a page
    BUILD_SANITY_CHECK_flashStorageSize (sizeof (iflashParameters) <= (FLASH_STORAGE_PAGES_COUNT * FLASH_PAGE_SIZE));
    // end sanity check 

    ReadFromFlashAddress ( DATA_START_ADDR , &mParameters, sizeof(mParameters));

    if (checkVersionMatch() != true)
    {
        mParameters = GetDefaults();
    }
}

// Raw read function. Reads raw bits into a set type
uint16_t readFlashAddress(uint32_t address) {

    // Wait until the flash is free (continue if not done within 10ms)
    HAL_StatusTypeDef status = FLASH_WaitForLastOperation(10);

    // Check if the flash is free
    if (status == HAL_OK) {

        // Pull the data from said address
        return *(__IO uint16_t *)address;
    }
    else {
        // Flash timed out, just return 0
        return 0;
    }
}


void FlashParameters::ReadFromFlashAddress(int32_t address, void* data, uint32_t size)
{
    uint16_t* FlashWord((uint16_t*) data);
    uint32_t  FlashWordsCount ( (size / sizeof(uint16_t)) + (size % sizeof(uint16_t)) );
    while(FlashWordsCount >0)
    {
        // Write out the data
        *FlashWord = readFlashAddress(address);
        FlashWordsCount--;
        FlashWord ++;
        address += sizeof(uint16_t);
    }
}



// Raw write function. Writes out arbitrary sized data to the flash
void FlashParameters::writeToFlashAddress(uint32_t address, void* data, uint32_t size) {

    // Disable the motor timers
    disableInterrupts();

    // Unlock the flash for writing
    HAL_FLASH_Unlock();

    // Clear any errors
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);

    uint16_t* _data(static_cast<uint16_t*>(data));

    uint32_t  FlashWordsCount ( (size / sizeof(uint16_t)) + (size % sizeof(uint16_t)) );
    while(FlashWordsCount >0)
    {
        // Write out the data
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, *_data);
        FlashWordsCount--;
        _data ++;
        address += sizeof(uint16_t);
    }
    // Lock the flash (we finished writing)
    HAL_FLASH_Lock();

    // Enable the motor timers
    enableInterrupts();
}


// Erases all data that is saved in the parameters page
void FlashParameters::eraseParameters() {

    // Disable the motor timers
    disableInterrupts();

    // Unlock the flash
    HAL_FLASH_Unlock();

    // Configure the erase type
    FLASH_EraseInitTypeDef eraseStruct;
    eraseStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseStruct.PageAddress = DATA_START_ADDR;
    eraseStruct.NbPages = 1;

    // Erase the the entire page (all possible addresses for parameters to be stored)
    uint32_t pageError = 0;
    HAL_FLASHEx_Erase(&eraseStruct, &pageError);

    // Good to go, lock the flash again (writeFlash has it's own locks and unlocks)
    HAL_FLASH_Lock();

    // Re-enable the motor timers
    enableInterrupts();
}


// Writes the currently saved parameters to flash memory for long term storage
void FlashParameters::saveParameters() {
    // Erase the current flash data if old data exists (needed to write new data, flash will not allow
    // writing 0s in place of 1s for whatever reason).
    eraseParameters();
    writeToFlashAddress(DATA_START_ADDR, &mParameters, sizeof(mParameters));
    // Write that the data is not valid
}


// Check if the stepper is calibrated
bool FlashParameters::isCalibrated() {

    // Return if the unit is calibrated
    return (mParameters.calibration.calibrated == true) && checkVersionMatch();
}


// Reads the version number from flash
// Returns true if the version matches, false if it doesn't
bool FlashParameters::checkVersionMatch() {
    return checkVersionMatch(mParameters);
}
// Reads the version number from flash
// Returns true if the version matches, false if it doesn't
bool FlashParameters::checkVersionMatch(iflashParameters& aParam) {

    // If the flash version should be ignored
    #ifndef IGNORE_FLASH_VERSION

    // Read the major version number
    if (aParam.FlashVersion != FlashFormatVersion ) {

        // Major version doesn't match
        return false;
    }
    uint32_t crc = computeCRC(aParam);
    if (crc != aParam.crc) {
        //crc doesn't match
        return false;
    }


    #endif // ! IGNORE_FLASH_VERSION

    // If we made it this far, return true
    return true;
}

// Loads the saved parameters from flash and sets them
String FlashParameters::loadParameters() {

    //params buffer
    iflashParameters params = { 0 };
    ReadFromFlashAddress(DATA_START_ADDR, &params, sizeof(params));
    if (checkVersionMatch(params))
    {
        mParameters = params;
        // If we made it this far, we can set the message to "ok" and move on
        return FLASH_LOAD_SUCCESSFUL;
    }
    else
    {
        mParameters = GetDefaults();
        // The data is invalid, so return a message saying that the load was unsuccessful
        return FLASH_LOAD_UNSUCCESSFUL;
    }

    return FLASH_LOAD_UNSUCCESSFUL;
}


String FlashParameters::dumpParameters() {
    String res = "{ \"FlashVersion\" : \"" +  String(mParameters.FlashVersion) + "\"," +
                   "\"calibration\" : { " +
                      "\"calibrated\" : \"" + String(mParameters.calibration.calibrated) + "\"," +
                      "\"stepOffset\" : \"" + String(mParameters.calibration.stepOffset) + "\"" +
                    "}," +
                   "\"current\" : { " +
                      "\"accelCurrent\" : \"" + String(mParameters.current.accelCurrent) + "\"," +
                      "\"idleCurrent\" : \"" + String(mParameters.current.idleCurrent) + "\"," +
                      "\"maxCurrent\" : \"" + String(mParameters.current.maxCurrent) + "\"" +
                    "}," +
                   "\"resolution\" : { " +
                      "\"fullStepAngle\" : \"" + String(mParameters.resolution.fullStepAngle) + "\"," +
                      "\"Microstepping\" : \"" + String(mParameters.resolution.Microstepping) + "\"," +
                      "\"microstepsMultipier\" : \"" + String(mParameters.resolution.microstepsMultipier) + "\"" +
                    "}," +
                   "\"direction\" : { " +
                      "\"reversed\" : \"" + String(mParameters.direction.reversed) + "\"," +
                      "\"enableInversion\" : \"" + String(mParameters.direction.enableInversion) + "\"" +
                    "}," +
                   "\"pid\" : { " +
                      "\"P\" : \"" + String(mParameters.pid.P) + "\"," +
                      "\"I\" : \"" + String(mParameters.pid.I) + "\"," +
                      "\"D\" : \"" + String(mParameters.pid.D) + "\"" +
                    "}," +
                    "\"canIndex\" : \"" +  String(mParameters.canIndex) + "\"," +
                   "\"DipSwitches\" : { " +
                      "\"active\" : \"" + String(mParameters.DipSwitches.active) + "\"," +
                      "\"isDipInverted\" : \"" + String(mParameters.DipSwitches.isDipInverted) + "\"" +
                    "}," +
                    "\"crc\" : \"" +  String(mParameters.crc) + "\"}\n";
    return res;
}


// Wipes all parameters stored, returning to programming defaults
// !!! WARNING !!! Reboots processor!
void FlashParameters::wipeParameters() {

    // Write that the flash is invalid (can write 0 without erasing flash)
    mParameters = GetDefaults();
    saveParameters();

    // Reboot the processor
    NVIC_SystemReset();
}

// get the full step andle of the motor (in degrees)
float FlashParameters::getFullStepAngle() const{
    return  mParameters.resolution.fullStepAngle;
}

// Set the full step angle of the motor (in degrees)
void FlashParameters::setFullStepAngle(float newStepAngle) {

    // Make sure that the new value isn't a -1 (all functions that fail should return a -1)
    if (newStepAngle != -1) {

        // Make sure that the value is one of the 2 common types
        // ! Maybe remove later?
        if ((newStepAngle == (float)1.8) || (newStepAngle == (float)0.9)) {

            // Save the new full step angle
            mParameters.resolution.fullStepAngle = newStepAngle;
            mParameters.crc = computeCRC(mParameters);

        }
    }
}

#if (ENABLE_DYNAMIC_CURRENT != false)
uint16_t FlashParameters::getAccelCurrent() const{
    return mParameters.current.accelCurrent;
}
void FlashParameters::setAccelCurrent(uint16_t accelCurrent){
    mParameters.current.idleCurrent = constrain(accelCurrent, 0, MAX_RMS_BOARD_CURRENT);
    mParameters.crc = computeCRC(mParameters);
}
#endif

// get rms current, when ENABLE_DYNAMIC_CURRENT is not set, the idleCurrent store the rms current
uint16_t FlashParameters::getRMSCurrent() const {
    return mParameters.current.idleCurrent;
}

// set rms current, when ENABLE_DYNAMIC_CURRENT is not set, the idleCurrent store the rms current
void FlashParameters::setRMSCurrent(uint16_t rmsCurrent){
    mParameters.current.idleCurrent = constrain(rmsCurrent, 0, MAX_RMS_BOARD_CURRENT);
    mParameters.crc = computeCRC(mParameters);
}

uint16_t FlashParameters::getMAXCurrent() const{
    return mParameters.current.idleCurrent;
}
void FlashParameters::setMAXCurrent(uint16_t maxCurrent) {
    mParameters.current.idleCurrent = constrain(maxCurrent, 0, MAX_PEAK_BOARD_CURRENT);
    mParameters.crc = computeCRC(mParameters);
}

void FlashParameters::setMicrostepping(uint8_t aMicrostepping) {
    mParameters.resolution.Microstepping = aMicrostepping;
    mParameters.crc = computeCRC(mParameters);
}

uint8_t FlashParameters::getMicrostepping() const {
    return mParameters.resolution.Microstepping;
}


void FlashParameters::setCalibration(float aStepOffset, bool aIsCalibrated){
    mParameters.calibration.stepOffset = aStepOffset;
    mParameters.calibration.calibrated = aIsCalibrated;
    mParameters.crc = computeCRC(mParameters);
}

float FlashParameters::getCalibration() const{
    return mParameters.calibration.stepOffset;
}

void FlashParameters::setDipswitchUse (bool enable){
    mParameters.DipSwitches.active = enable;
}
bool FlashParameters::getDipswitchUse(){
    return mParameters.DipSwitches.active;
}

#define ManualCRC 1
uint32_t FlashParameters::computeCRC(iflashParameters &aParameters) {

    uint32_t DataLength(reinterpret_cast<uint8_t*>(&(aParameters.crc)) - reinterpret_cast<uint8_t*>(&aParameters));
    uint8_t* pData(reinterpret_cast<uint8_t*>(&aParameters));
    if (ManualCRC) {
        //non hardware assisted CRC computation, will be optimized out if ManualCRC==0
        static const uint32_t crc_table[0x100] = {
        0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 
        0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD, 
        0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 
        0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 
        0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 
        0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 
        0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 
        0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A, 
        0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
        0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 
        0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3, 
        0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
        0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 
        0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 
        0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 
        0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4, 
        };

        uint32_t Checksum = 0xFFFFFFFF;
        
        for(unsigned int i=0; i < DataLength; i++)
        {
            uint8_t top = (uint8_t)(Checksum >> 24);
            top ^= pData[i];
            Checksum = (Checksum << 8) ^ crc_table[top];
        }
        return Checksum;
    } else {
        /*static CRC_HandleTypeDef hcrc = { 
            .Instance = CRC, 
            .Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE,
            .Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE,
            .Init.CRCLength = CRC_POLYLENGTH_32B,
            .Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE,
            .Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE,
            .InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES,
        };
        uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)pData, DataLength);
        return crc;*/
    }
}