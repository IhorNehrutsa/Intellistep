#ifndef __FLASH_H
#define __FLASH_H

#include "stm32f103xb.h"
#include "buttons.h"
#include "canMessaging.h"
#include "timers.h"
#include "stm32f1xx_hal_flash.h"

// Where the parameters are saved
// This is 1024 bits behind the end of flash
// This allows 32 32-bit values to be stored
#define DATA_START_ADDR      0x0801FC00

// Messages for successful and unsuccessful flash reads
#define FLASH_LOAD_SUCCESSFUL      F("Flash data loaded")
#define FLASH_LOAD_UNSUCCESSFUL    F("Flash data non-existent")
#define FLASH_LOAD_INVALID_VERSION F("Data version outdated")

// Main overview of the format of data storage
typedef enum {

    // Valid flash contents marker (bool)
    VALID_FLASH_CONTENTS = 0,

    // Stores the firmware version that was used
    FLASH_CONTENTS_MAJOR_VERSION_INDEX,
    FLASH_CONTENTS_MINOR_VERSION_INDEX,
    FLASH_CONTENTS_PATCH_VERSION_INDEX,

    // Calibrated marker
    CALIBRATED_INDEX,
    STEP_OFFSET_INDEX,

    // Dynamic current settings
    #ifdef ENABLE_DYNAMIC_CURRENT
    DYNAMIC_ACCEL_CURRENT_INDEX,
    DYNAMIC_IDLE_CURRENT_INDEX,
    DYNAMIC_MAX_CURRENT_INDEX,

    #else // ! ENABLE_DYNAMIC_CURRENT
    CURRENT_INDEX_0,
    CURRENT_INDEX_1,
    CURRENT_INDEX_2,
    #endif // ! ENABLE_DYNAMIC_CURRENT

    // Motor settings
    FULL_STEP_ANGLE_INDEX,
    MICROSTEPPING_INDEX,
    MOTOR_REVERSED_INDEX,
    ENABLE_INVERSION_INDEX,
    MICROSTEP_MULTIPLIER_INDEX,

    // PID values
    P_TERM_INDEX,
    I_TERM_INDEX,
    D_TERM_INDEX,

    // CAN
    CAN_ID_INDEX,

    // Inverted dips
    INVERTED_DIPS_INDEX

} FLASH_PARAM_INDEXES;

// The max index of the flash parameters (must be manually updated)
// Note that the flash CANNOT store more than 32 parameters
// It would overflow the page the data is stored in
#define MAX_FLASH_PARAM_INDEX 19

// Functions
bool isCalibrated();

// Reading flash
uint16_t readFlashAddress(uint32_t address);
uint16_t readFlashU16(uint32_t parameterIndex);
uint32_t readFlashU32(uint32_t parameterIndex);
bool     readFlashBool(uint32_t parameterIndex);
float    readFlashFloat(uint32_t parameterIndex);

// Writing to flash
void writeToAddress(uint32_t address, uint16_t data);
void writeFlash(uint32_t parameterIndex, uint16_t data);
void writeFlash(uint32_t parameterIndex, uint32_t data);
void writeFlash(uint32_t parameterIndex, bool data);
void writeFlash(uint32_t parameterIndex, float data);

// Erase all of the parameters in flash
void eraseParameters();

// Load/saving values to flash
void saveParameters();
bool checkVersionMatch();
String loadParameters();
void wipeParameters();

#endif


