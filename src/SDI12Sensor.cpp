#include "SDI12Sensor.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EEPROM.h>
#include <WString.h>

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


/**
 * @brief Counts the number of digits of the whole/integral part from float/decimal number.
 *
 * @param[in] value Floating/Decimal number
 * @return size_t Number of digits of whole/integral part.
 */
size_t IntegralLength(double value) {
    unsigned int len = 0;
    if (value < 0) { value = -value; }
    int val = (int)value;
    do {
        val /= 10;
        ++len;
    } while (val > 0);
    return len;
}


/**
 * @brief Look up table for Powers of 10, 10^0 to 10^9.
 *
 */
static const double powers_of_10[] = {1,      10,      100,      1000,      10000,
                                      100000, 1000000, 10000000, 100000000, 1000000000};


/**
 * @brief Takes a reverses a null terminated string.
 *
 * @param begin Array in memory where to store null-terminated string
 * @param end Pointer to last non null terminated char in array to be reversed.
 */
static void strreverse(char *begin, char *end) {
    char aux;
    while (end > begin) {
        aux = *end;
        *end-- = *begin;
        *begin++ = aux;
    }
}


/**
 * @brief Converts floats to string, based on stringencoders modp_dtoa2() from
 * https://github.com/client9/stringencoders/blob/master/src/modp_numtoa.h
 *
 * @param[in] value value to be converted
 * @param[out] str Array in memory where to store the resulting null-terminated string.
 *      return: "NaN" if overflow or value is greater than desired fit_len
 * @param[in] prec desired precision [0 - 9], will be affected by fit_len,  default: 0
 * @param[in] fit_len (optional) non negative value for max length of output string
 *      including decimal and sign, does not include null pointer,
 *      default: 0 no length limit
 * @param[in] zero_trail (optional) trailing zeros,  default: false
 * @param[in] pos_sign (optional) show positive sign,  default: true
 * @return size_t  Length of string, returns 0 if NaN
 */
size_t dtoa(double value, char *str, uint8_t prec, uint8_t fit_len, bool zero_trail, bool pos_sign) {
    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (!(value == value)) {
        strcpy(str, "NaN");
        return 0;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    bool neg = false;
    if (value < 0) {
        neg   = true;
        value = -value;
    }

    int whole = (int)value;

    if (prec <= 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    // Return NaN if whole digit is greater than max_length including sign
    if (fit_len > 0) {
        size_t len_of_integral = IntegralLength(value);
        if (len_of_integral >= fit_len) {
            strcpy(str, "NaN");
            return 0;
        }

        // Resize precision if greater than would fit otherwise
        // -2 to account for sign and decimal
        if (prec > (fit_len - len_of_integral - 2)) {
            prec = (fit_len - len_of_integral - 2);
            if (!pos_sign && !neg) { prec++; }
        }
    }

    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
       normal printf behavior is to print EVERY whole number digit
       which can be 100s of characters overflowing your buffers == bad
       */
    if (value > thres_max) {
        if (pos_sign) {
            sprintf(str, "%+.*f", prec, neg ? -value : value);
        } else {
            sprintf(str, "%.*f", prec, neg ? -value : value);
        }
        return strlen(str);
    }

    char *wstr = str;
    double   p10_fraction  = (value - whole) * powers_of_10[prec];
    uint32_t int_from_frac = (uint32_t)(p10_fraction);
    double diff_frac = p10_fraction - int_from_frac;
    bool has_decimal = false;
    uint8_t len_of_sigfig = prec;

    if (diff_frac > 0.499) {
        // Round up above 0.49 to account for precision conversion error
        ++int_from_frac;
        /* handle rollover, e.g. case 0.99 with prec 1 is 1.0  */
        if (int_from_frac >= powers_of_10[prec]) {
            int_from_frac = 0;
            ++whole;
        }
    } else if (diff_frac == 0.5) {
        if (prec > 0 && (int_from_frac & 1)) {
            /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        } else if (prec == 0 && (whole & 1)) {
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        }
    }

    if (prec > 0) {
        /* Remove ending zeros */
        if (!zero_trail) {
            while (len_of_sigfig > 0 && ((int_from_frac % 10) == 0)) {
                len_of_sigfig--;
                int_from_frac /= 10;
            }
        }
        if (len_of_sigfig > 0) has_decimal = true;

        while (len_of_sigfig > 0) {
            --len_of_sigfig;
            *wstr++ = (char)(48 + (int_from_frac % 10));
            int_from_frac /= 10;
        }
        if (int_from_frac > 0) { ++whole; }

        /* add decimal */
        if (has_decimal) { *wstr++ = '.'; }
    }

    /* do whole part
     * Take care of sign conversion
     * Number is reversed.
     */
    do {
        *wstr++ = (char)(48 + (whole % 10));
    } while (whole /= 10);
    if (neg) { *wstr++ = '-'; } else if (pos_sign) { *wstr++ = '+'; }
    *wstr = '\0';
    strreverse(str, wstr - 1);
    return (size_t)(wstr - str);
}

/// @overload
size_t dtoa(double value, String &str, uint8_t prec, uint8_t fit_len, bool zero_trail, bool pos_sign) {
    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (!(value == value)) {
        str = "NaN";
        return 0;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    bool neg = false;
    if (value < 0) {
        neg   = true;
        value = -value;
    }

    int whole = (int)value;

    if (prec <= 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    // Return NaN if whole digit is greater than max_length including sign
    if (fit_len > 0) {
        size_t len_of_integral = IntegralLength(value);
        if (len_of_integral >= fit_len) {
            str = "NaN";
            return 0;
        }

        // Resize precision if greater than would fit otherwise
        // -2 to account for sign and decimal
        if (prec > (fit_len - len_of_integral - 2)) {
            prec = (fit_len - len_of_integral - 2);
            if (!pos_sign && !neg) { prec++; }
        }
    }

    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
       normal printf behavior is to print EVERY whole number digit
       which can be 100s of characters overflowing your buffers == bad
       */
    if (value > thres_max) {
        if (neg) {
            str = String(-value, prec);
        } else if (pos_sign) {
            str = "+";
            str += String(value, prec);
        } else {
            str = String(value, prec);
        }
        return str.length();
    }

    double   p10_fraction  = (value - whole) * powers_of_10[prec];
    uint32_t int_from_frac = (uint32_t)(p10_fraction);
    double diff_frac = p10_fraction - int_from_frac;
    uint8_t len_of_sigfig = prec;

    if (diff_frac > 0.499) {
        // Round up above 0.49 to account for precision conversion error
        ++int_from_frac;
        /* handle rollover, e.g. case 0.99 with prec 1 is 1.0  */
        if (int_from_frac >= powers_of_10[prec]) {
            int_from_frac = 0;
            ++whole;
        }
    } else if (diff_frac == 0.5) {
        if (prec > 0 && (int_from_frac & 1)) {
            /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        } else if (prec == 0 && (whole & 1)) {
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        }
    }

    if (prec > 0) {
        /* Remove ending zeros */
        if (!zero_trail) {
            while (len_of_sigfig > 0 && ((int_from_frac % 10) == 0)) {
                len_of_sigfig--;
                int_from_frac /= 10;
            }
        }
    }

    /* start building string */
    if (neg) {
        str = String(-value, len_of_sigfig);
    } else if (pos_sign) {
        str = "+";
        str += String(value, len_of_sigfig);
    } else {
        str = String(value, len_of_sigfig);
    }
    return (size_t)(str.length());
}


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
