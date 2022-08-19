#ifndef FLASH_H
#define FLASH_H

#include "buttons.h"
#include "canMessaging.h"

#include "timers.h"

// Where the parameters are saved
// page size is 1kBytes
#define FLASH_PAGE_COUNT ((FLASH_BANK1_END - FLASH_BASE)/ FLASH_PAGE_SIZE)
#define FLASH_STORAGE_PAGES_COUNT  1
const uint32_t DATA_START_ADDR (FLASH_BASE + (FLASH_PAGE_COUNT - FLASH_STORAGE_PAGES_COUNT) * FLASH_PAGE_SIZE);

// Messages for successful and unsuccessful flash reads
#define FLASH_LOAD_SUCCESSFUL F("Flash data loaded")
#define FLASH_LOAD_UNSUCCESSFUL F("Flash data non-existent")
#define FLASH_LOAD_INVALID_VERSION F("Data version outdated")

// Main overview of the format of data storage
// Note that the flash CANNOT store more than 32 parameters
// It would overflow the page the data is stored in
#define FlashFormatVersion 1 //increment each time the following structure change
#pragma pack(push, 1)
struct iflashParameters
{
    uint16_t FlashVersion;
    struct     {
        uint8_t calibrated;
        float stepOffset;
    } calibration; //5bytes
    struct {
        uint16_t accelCurrent;
        uint16_t idleCurrent;
        uint16_t maxCurrent;
    } current; //6bytes
    struct {
        float fullStepAngle;
        uint16_t Microstepping;
        float microstepsMultipier;
    } resolution; //10bytes
    struct {
        uint8_t reversed;
        uint8_t enableInversion;
    } direction; //2bytes
    struct {
        float P;
        float I;
        float D;
    } pid; //12bytes
    int16_t canIndex; //2bytes
    struct {
        uint8_t active;
        uint8_t isDipInverted;
    } DipSwitches; //2bytes
    uint32_t crc; //4byte
}; 

#pragma pack(pop)



class FlashParameters
{
public: //types
    // Enumeration for movement units (in micrometer unit to allow int operations)
    enum DistanceUnits {
        mm=0,
        inches =1
    };
    const uint16_t DistanceUnitsMicrom[2] = {
        1000,
        25400
     };
    
public:
    // singleton pattern 
    static FlashParameters& getInstance();
    // Functions
    bool isCalibrated();
    // Load/saving values to flash
    void saveParameters();
    bool checkVersionMatch(iflashParameters& aParam);
    bool checkVersionMatch();
    String loadParameters();
    void wipeParameters();
    String dumpParameters();

    //accessors to flashed values
    float getFullStepAngle() const;
    void setFullStepAngle(float newAngle);

    
#if (ENABLE_DYNAMIC_CURRENT != false)
    uint16_t getAccelCurrent() const;
    void setAccelCurrent(uint16_t rmsCurrent);
#endif

    uint16_t getRMSCurrent() const;
    void setRMSCurrent(uint16_t rmsCurrent);
    uint16_t getMAXCurrent() const;
    void setMAXCurrent(uint16_t maxCurrent);
    

    uint8_t getMicrostepping() const;
    void setMicrostepping(uint8_t setMicrostepping);

    float getCalibration() const;   
    void setCalibration(float aStepOffset, bool aIsCalibrated=true);

    void setDipswitchUse (bool enable);
    bool getDipswitchUse();
 

    // Erase all of the parameters in flash
    void eraseParameters();
    uint32_t computeCRC(iflashParameters &aParameters);

private:
    // Reading flash
    void ReadFromFlashAddress(int32_t address, void* data, uint32_t size);

    // Writing to flash
    void writeToFlashAddress(uint32_t address, void* data, uint32_t size);

    //default Parameters
    iflashParameters GetDefaults();

    //private constructor to prevent use
    FlashParameters();

private :  //data
    FlashParameters* mInstance = NULL;
    iflashParameters mParameters;
};

#endif
