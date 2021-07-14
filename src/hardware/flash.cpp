#include "flash.h"

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


// Based on the readFlashAddress function, reads a uint16_t at a parameter index
uint16_t readFlashU16(uint32_t parameterIndex) {

    // Just a basic read
    return readFlashAddress(DATA_START_ADDR + (parameterIndex * 4));
}


// Based on the readFlashAddress function, reads a uint32_t at a parameter index
uint32_t readFlashU32(uint32_t parameterIndex) {

    // Create a blank array to store the values in
    uint16_t intData[2];

    // Pull the values at the specified addresses
    intData[0] = readFlashAddress(DATA_START_ADDR + (parameterIndex * 4));
    intData[1] = readFlashAddress(DATA_START_ADDR + (parameterIndex * 4) + 2);

    // Create a variable to store the data in
    uint32_t data;

    // Copy the 4 bytes of the data over (all of it)
    memcpy(&data, &intData, 4);

    // Return the data
    return data;
}


// Based on the readFlashAddress function, reads a bool at a parameter index
bool readFlashBool(uint32_t parameterIndex) {

    // Return the data stored at the address, converted from native 16 bit int to bool
    if (readFlashAddress(DATA_START_ADDR + (parameterIndex * 4)) == 1) {
        return true;
    }
    else {
        return false;
    }
}


// Based on the readFlashAddress function, reads a uint32_t at a parameter index
float readFlashFloat(uint32_t parameterIndex) {

    // Create a blank array to store the values in
    uint16_t intData[2];

    // Pull the values at the specified addresses
    intData[0] = readFlashAddress(DATA_START_ADDR + (parameterIndex * 4));
    intData[1] = readFlashAddress(DATA_START_ADDR + (parameterIndex * 4) + 2);

    // Create a variable to store the data in
    float data;

    // Copy the 4 bytes of the data over (all of it)
    memcpy(&data, &intData, 4);

    // Return the data
    return data;
}


// Raw write function. Writes out 16 bits to the flash
void writeToFlashAddress(uint32_t address, uint16_t data) {

    // Disable the motor timers
    disableInterrupts();

    // Unlock the flash for writing
    HAL_FLASH_Unlock();

    // Clear any errors
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);

    // Write out the data
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, data);

    // Lock the flash (we finished writing)
    HAL_FLASH_Lock();

    // Enable the motor timers
    enableInterrupts();
}


// Built on the address function, writes to a parameter index instead
void writeFlash(uint32_t parameterIndex, uint16_t data) {

    // Address is the start address plus 4 bytes for every parameter after
    writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4), data);
}


// Built on the address function, writes unsigned 32 bit ints instead
void writeFlash(uint32_t parameterIndex, uint32_t data) {

    // Create array for the uint16_ts
    uint16_t intData[2];

    // Copy the raw data over (4 bytes of information)
    memcpy(intData, &data, 4);

    // Now send each of the parts to flash
    writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4),     intData[0]);
    writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4) + 2, intData[1]);
}


// Built on the address function, writes a bool instead
void writeFlash(uint32_t parameterIndex, bool data) {

    // Convert the bool to a 16 bit unsigned int (natural flash unit) and write it
    if (data) {
        writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4), 1U);
    }
    else {
        writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4), 0U);
    }
}


// Built on the address function, writes a float instead
void writeFlash(uint32_t parameterIndex, float data) {

    // Create array for the uint16_ts
    uint16_t intData[2];

    // Copy the raw data over
    memcpy(intData, &data, 4);

    // Now send each of the parts to flash
    writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4),     intData[0]);
    writeToFlashAddress(DATA_START_ADDR + (parameterIndex * 4) + 2, intData[1]);
}


// Erases all data that is saved in the parameters page
void eraseParameters() {

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

    // Write the major, minor, and patch numbers to flash
    writeFlash(FLASH_CONTENTS_MAJOR_VERSION_INDEX, MAJOR_VERSION);
    writeFlash(FLASH_CONTENTS_MINOR_VERSION_INDEX, MINOR_VERSION);
    writeFlash(FLASH_CONTENTS_PATCH_VERSION_INDEX, PATCH_VERSION);

    // Re-enable the motor timers
    enableInterrupts();
}


// Writes the currently saved parameters to flash memory for long term storage
void saveParameters() {

    // Check to see if the module was calibrated previously
    bool calibrated = isCalibrated();

    // Erase the current flash data if old data exists (needed to write new data, flash will not allow
    // writing 0s in place of 1s for whatever reason).
    eraseParameters();

    // Write that the data is valid
    writeFlash(VALID_FLASH_CONTENTS, true);

    // Save the previous state
    writeFlash(CALIBRATED_INDEX, calibrated);

    // Get the motor current
    #ifdef ENABLE_DYNAMIC_CURRENT
        writeFlash(DYNAMIC_ACCEL_CURRENT_INDEX, motor.getDynamicAccelCurrent());
        writeFlash(DYNAMIC_IDLE_CURRENT_INDEX, motor.getDynamicIdleCurrent());
        writeFlash(DYNAMIC_MAX_CURRENT_INDEX, motor.getDynamicMaxCurrent());
    #else
        // Save the static motor current in the idle and max slot of the dynamic current setting, zeroing the acceleration
        writeFlash(CURRENT_INDEX_0, (uint16_t)0);
        writeFlash(CURRENT_INDEX_1, motor.getRMSCurrent());
        writeFlash(CURRENT_INDEX_2, motor.getRMSCurrent());
    #endif

    // Motor stepping angle
    writeFlash(FULL_STEP_ANGLE_INDEX, motor.getFullStepAngle());

    // Microstepping divisor
    writeFlash(MICROSTEPPING_INDEX, motor.getMicrostepping());

    // Motor Direction Reversed
    writeFlash(MOTOR_REVERSED_INDEX, motor.getReversed());

    // Motor Enable Inverted
    writeFlash(ENABLE_INVERSION_INDEX, motor.getEnableInversion());

    // Microstep multiplier
    writeFlash(MICROSTEP_MULTIPLIER_INDEX, motor.getMicrostepMultiplier());

    // Write the PID values if specified
    #ifdef ENABLE_PID
        // P term of PID
        writeFlash(P_TERM_INDEX, (float)pid.getP());

        // I term of PID
        writeFlash(I_TERM_INDEX, (float)pid.getI());

        // D term of PID
        writeFlash(D_TERM_INDEX, (float)pid.getD());
    #endif

    // CAN ID of the motor controller
    #ifdef ENABLE_CAN
        writeFlash(CAN_ID_INDEX, (float)getCANID());
    #else
        writeFlash(CAN_ID_INDEX, (float)NONE);
    #endif

    // If the dip switches were installed incorrectly
    writeFlash(INVERTED_DIPS_INDEX, getDipInverted());
}


// Check if the stepper is calibrated
bool isCalibrated() {

    // Return if the unit is calibrated
    return readFlashBool(CALIBRATED_INDEX) && checkVersionMatch();
}


// Reads the version number from flash
// Returns true if the version matches, false if it doesn't
bool checkVersionMatch() {

    // If the flash version should be ignored
    #ifndef IGNORE_FLASH_VERSION

    // Read the major version number
    if (readFlashU16(FLASH_CONTENTS_MAJOR_VERSION_INDEX) != MAJOR_VERSION) {

        // Major version doesn't match
        return false;
    }

    // Read the minor version number
    if (readFlashU16(FLASH_CONTENTS_MINOR_VERSION_INDEX) != MINOR_VERSION) {

        // Minor version doesn't match
        return false;
    }

    // Read the patch version number
    if (readFlashU16(FLASH_CONTENTS_PATCH_VERSION_INDEX) != PATCH_VERSION) {

        // Patch version doesn't match
        return false;
    }

    #endif // ! IGNORE_FLASH_VERSION

    // If we made it this far, return true
    return true;
}


// Loads the saved parameters from flash and sets them
String loadParameters() {

    // Create a storage for the output message
    String outputMessage;

    // Check to see if the data is valid
    if (readFlashBool(VALID_FLASH_CONTENTS)) {

        // Check the version number
        if (!checkVersionMatch()) {
            return FLASH_LOAD_INVALID_VERSION;
        }

        // Load the calibration offset
        motor.encoder.setStepOffset(readFlashFloat(STEP_OFFSET_INDEX));

        // Set the motor current
        #ifdef ENABLE_DYNAMIC_CURRENT
            motor.setDynamicAccelCurrent(readFlashU16(DYNAMIC_ACCEL_CURRENT_INDEX));
            motor.setDynamicIdleCurrent(readFlashU16(DYNAMIC_IDLE_CURRENT_INDEX));
            motor.setDynamicMaxCurrent(readFlashU16(DYNAMIC_MAX_CURRENT_INDEX));
        #else
            // Just load the motor current from the idle index
            motor.setRMSCurrent(readFlashU16(CURRENT_INDEX_1));
        #endif

        // Motor stepping angle
        motor.setFullStepAngle(readFlashFloat(FULL_STEP_ANGLE_INDEX));

        // Microstepping divisor
        motor.setMicrostepping(readFlashU16(MICROSTEPPING_INDEX));

        // Motor Direction Reversed
        motor.setReversed(readFlashBool(MOTOR_REVERSED_INDEX));

        // Motor Enable Inverted
        motor.setEnableInversion(readFlashBool(ENABLE_INVERSION_INDEX));

        // Motor microstepping multiplier
        motor.setMicrostepMultiplier(readFlashFloat(MICROSTEP_MULTIPLIER_INDEX));

        // Only load PID values if PID is enabled
        #ifdef ENABLE_PID
            // P term of PID
            pid.setP(readFlashFloat(P_TERM_INDEX));

            // I term of PID
            pid.setI(readFlashFloat(I_TERM_INDEX));

            // D term of PID
            pid.setD(readFlashFloat(D_TERM_INDEX));
        #endif

        // The CAN ID of the motor
        #ifdef ENABLE_CAN
            setCANID((AXIS_CAN_ID)readFlashFloat(CAN_ID_INDEX));
        #endif

        // If the dip switches were installed incorrectly
        setDipInverted(readFlashBool(INVERTED_DIPS_INDEX));

        // If we made it this far, we can set the message to "ok" and move on
        outputMessage = FLASH_LOAD_SUCCESSFUL;
    }
    else {
        // The data is invalid, so return a message saying that the load was unsuccessful
        outputMessage = FLASH_LOAD_UNSUCCESSFUL;
    }

    // All done, we can return the result of the process
    return outputMessage;
}


// Wipes all parameters stored, returning to programming defaults
// !!! WARNING !!! Reboots processor!
void wipeParameters() {

    // Write that the flash is invalid (can write 0 without erasing flash)
    writeFlash(VALID_FLASH_CONTENTS, false);

    // Reboot the processor
    NVIC_SystemReset();
}
