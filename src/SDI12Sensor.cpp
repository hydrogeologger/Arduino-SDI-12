#include "SDI12Sensor.h"

#include <ctype.h>

#include <EEPROM.h>

/* Default VALUES */
#define DEFAULT_SENSOR_ADDR '0' // Default Sensor Address


/**
 * @brief Construct a default SDI12Sensor::SDI12Sensor object.
 *
 * Sensor address defaults to Zero - '0'
 * Device address can be changed with SDI12Sensor::SetAddress(address).
 *
 * This empty constructor is provided for easier integration with other Arduino libraries.
 *
 * @see SetAddress(const char address)
 * @see @see SDI12Sensor(const char address, const int eeprom_address)
 */
SDI12Sensor::SDI12Sensor(void) {
    sensor_address_ = DEFAULT_SENSOR_ADDR;
    eeprom_address_ = SDI12SENSOR_ERR_INVALID; // EEPROM use is set to disable
}


/**
 * @brief Construct a new SDI12Sensor::SDI12Sensor object with the address set.
 *
 * Address is first obtained from eeprom if eeprom is enabled.
 * @p eeprom_address needs to be 0 or greater to be enabled. EEPROM storage is
 * disabled if @p eeprom_address is -1 (SDI12SENSOR_ERR_INVALID).
 *
 * If address obtained from eeprom is not alphanumeric, address is set from @p address
 *
 * Sensor address defaults to Zero - '0' if all address sources fail.
 * Device address can be changed with SDI12Sensor::SetAddress(address).
 * NOTE: If eeprom is enabled, try not call SDI12Sensor::SetAddress(address) during
 * initialization phase as it will force update the sensor address to eeprom.
 *
 * @param[in] address Single alpha numeric character representation of sensor address
 * @param[in] eeprom_address (Optional) The location in eeprom memory to store device address, Default: -1 (EEPROM disabled)
 *
 * @see SetAddress(const char address)
 */
SDI12Sensor::SDI12Sensor(const char address, const int eeprom_address) {
    // Check if EEPROM in use to store SDI address
    if (eeprom_address >= 0) {
        eeprom_address_ = eeprom_address;
        sensor_address_ = GetAddressFromEEPROM(); // Get address from eeprom to start off
    } else {
        sensor_address_ = address;
        eeprom_address_ = SDI12SENSOR_ERR_INVALID; // EEPROM use is set to disable
    }

    // Set to default address if address provided is invalid
    if (!isalnum(sensor_address_)) {
        sensor_address_ = DEFAULT_SENSOR_ADDR;
    }
}


/**
 * @brief Destroy the SDI12Sensor::SDI12Sensor object
 * @see SDI12Sensor(void)
 * @see SDI12Sensor(const char address, const int eeprom_address)
 */
SDI12Sensor::~SDI12Sensor(void) {
    // Do nothing
}


/**
 * @brief Sets the sensor address of the SDI12Sensor object.
 *
 * Will store address in EEPROM if SDI12Sensor object initialized with eeprom enabled.
 *
 * @param[in] address Single alpha numeric character representation of sensor address
 * @return true Sensor address is alpha numeric and update sucessfull
 * @return false Sensor address was not updated
 *
 * @see Address(void)
 * @see SDI12Sensor(const char address, const int eeprom_address)
 */
bool SDI12Sensor::SetAddress(const char address) {
    if (isalnum(address)) {
        sensor_address_ = address;
        if (eeprom_address_ >= 0) {
            EEPROM.update(eeprom_address_, address);
        }
        return true;
    }
    return false;
}


/**
 * @brief Gets sensor address.
 *
 * @return char Sensor address
 *
 * @see SetAddress(const char address)
 */
char SDI12Sensor::Address(void) const {
    return sensor_address_;
}


/**
 * @brief Get sensor address from EEPROM if eeprom enabled
 *
 * @return char Sensor address, returns null terminator '\0' if eeprom is disabled
 */
char SDI12Sensor::GetAddressFromEEPROM(void) const {
    if (eeprom_address_ >= 0) {
        return EEPROM.read(eeprom_address_);
    }
    return '\0';
}


// void SDI12Sensor::SendSensorAddress() {
//     char message[5];
//     sprintf(message, "%c\r\n", sensor_address_);
//     sendResponse(String(sensor_address_) + "\r\n");
// }


// void SDI12Sensor::SendSensorID() {
//     char message[36];
//     sprintf(message, "%c%s%s%s%s\r\n", sensor_address_, SDI12SENSOR_SDI12_PROTOCOL, SDI12SENSOR_COMPANY, SDI12SENSOR_MODEL, SDI12SENSOR_VERSION, SDI12SENSOR_OTHER_INFO);
//     sendResponse(message);
//     // sendResponse(String(sensor_address_) + SDI12SENSOR_SDI12_PROTOCOL, SDI12SENSOR_COMPANY, SDI12SENSOR_MODEL, SDI12SENSOR_VERSION, SDI12SENSOR_OTHER_INFO);
// }


// void FormatOutputSDI(float *measurementValues, String *dValues, unsigned int maxChar) {
//     /* Ingests an array of floats and produces Strings in SDI-12 output format */

//     uint8_t lenValues = sizeof(*measurementValues) / sizeof(char *);
//     uint8_t lenDValues = sizeof(*dValues) / sizeof(char *);
//     dValues[0] = "";
//     int j = 0;

//     // upper limit on i should be number of elements in measurementValues
//     for (int i = 0; i < lenValues; i++) {
//         // Read float value "i" as a String with 6 decimal digits
//         // (NOTE: SDI-12 specifies max of 7 digits per value; we can only use 6
//         //  decimal place precision if integer part is one digit)
//         String valStr = String(measurementValues[i], SDI12_DIGITS_MAX - 1);

//         // Explictly add implied + sign if non-negative
//         if (valStr.charAt(0) != '-') {
//             valStr = '+' + valStr;
//         }

//         // Append dValues[j] if it will not exceed 35 (aM!) or 75 (aC!) characters
//         if (dValues[j].length() + valStr.length() < maxChar) {
//             dValues[j] += valStr;
//         }
//         // Start a new dValues "line" if appending would exceed 35/75 characters
//         else {
//             dValues[++j] = valStr;
//         }
//     }

//     // Fill rest of dValues with blank strings
//     while (j < lenDValues) {
//         dValues[++j] = "";
//     }
// }
