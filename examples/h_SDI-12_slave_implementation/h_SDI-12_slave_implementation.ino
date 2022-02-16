/**
 * @file h_SDI-12_slave_implementation.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @date 2016
 * @author D. Wasielewski
 *
 * @brief Example H:  Using SDI-12 in Slave Mode
 *
 * Example sketch demonstrating how to implement an arduino as a slave on an SDI-12 bus.
 * This may be used, for example, as a middleman between an I2C sensor and an SDI-12
 * datalogger.
 *
 * Note that an SDI-12 slave must respond to M! or C! with the number of values it will
 * report and the max time until these values will be available.  This example uses 9
 * values available in 21 s, but references to these numbers and the output array size
 * and datatype should be changed for your specific application.
 *
 * D. Wasielewski, 2016
 * Builds upon work started by:
 * https://github.com/jrzondagh/AgriApps-SDI-12-Arduino-Sensor
 * https://github.com/Jorge-Mendes/Agro-Shield/tree/master/SDI-12ArduinoSensor
 *
 * Suggested improvements:
 *  - Get away from memory-hungry arduino String objects in favor of char buffers
 *  - Make an int variable for the "number of values to report" instead of the
 *    hard-coded 9s interspersed throughout the code
 */

#include <SDI12Slave.h>
#include <SDI12CRC.h>
#include <SDI12Sensor.h>


#define DATA_PIN 13  /*!< The pin of the SDI-12 data bus */
#define POWER_PIN -1 /*!< The sensor power pin (or -1 if not switching power) */

#define MEASUREMENT_ARRAY_MAX_SIZE 9 // Max size of floats/double array to hold sensor data
#define MEASUREMENT_STR_ARRAY_MAX_ELEMENT 10 // Max number of array elements for 0Dx! string response

#define SDI12SENSOR_SDI12_PROTOCOL "13"  // Respresent v1.3
#define SDI12SENSOR_COMPANY "COMPNAME"  // 8 Charactors depicting company name
#define SDI12SENSOR_MODEL "000001"  // 6 Characters specifying sensor model
#define SDI12SENSOR_VERSION "1.0"  // 3 characters specifying sensor version
#define SDI12SENSOR_OTHER_INFO "001"  // (optional) up to 13 char for serial or other sensor info

#define WAIT 0
#define INITIATE_CONCURRENT 1
#define INITIATE_MEASUREMENT 2
int state = WAIT;

// Create object by which to communicate with the SDI-12 bus on SDIPIN
SDI12Slave slaveSDI12(DATA_PIN);
SDI12Sensor sensor('0', -1);

void pollSensor(float* measurementValues) {
    measurementValues[0] = 1.1;
    measurementValues[1] = -2.22;
    measurementValues[2] = 3.333;
    measurementValues[3] = -4.4444;
    measurementValues[4] = 5.55555;
    measurementValues[5] = -6.666666;
    measurementValues[6] = 78.777777;
    measurementValues[7] = -890.888888;
    measurementValues[8] = -0.11111111;
}

void parseSdi12Cmd(String command, String* dValues) {
    /* Ingests a command from an SDI-12 master, sends the applicable response, and
     * (when applicable) sets a flag to initiate a measurement
     */

    // First char of command is always either (a) the address of the device being
    // probed OR (b) a '?' for address query.
    // Do nothing if this command is addressed to a different device
    if (command.charAt(0) != sensor.Address() && command.charAt(0) != '?') {
        return;
    }

    // If execution reaches this point, the slave should respond with something in
    // the form:   <address><responseStr><Carriage Return><Line Feed>
    // The following if-switch-case block determines what to put into <responseStr>,
    // and the full response will be constructed afterward. For '?!' (address query)
    // or 'a!' (acknowledge active) commands, responseStr is blank so section is skipped
    String responseStr = "";
    responseStr += sensor.Address();
    if (command.length() > 1) {
        switch (command.charAt(1)) {
            case 'I':
                // Identify command
                // Slave should respond with ID message: 2-char SDI-12 version + 8-char
                // company name + 6-char sensor model + 3-char sensor version + 0-13
                // char S/N
                responseStr += SDI12SENSOR_SDI12_PROTOCOL \
                        SDI12SENSOR_COMPANY \
                        SDI12SENSOR_MODEL \
                        SDI12SENSOR_VERSION \
                        SDI12SENSOR_OTHER_INFO;
                break;
            case 'C':
                // Initiate concurrent measurement command
                // Slave should immediately respond with: "tttnn":
                //    3-digit (seconds until measurement is available) +
                //    2-digit (number of values that will be available)
                // Slave should also start a measurment and relinquish control of the
                // data line
                responseStr += "02109";  // 9 values ready in 21 sec; Substitue
                                        // sensor-specific values here
                // It is not preferred for the actual measurement to occur in this
                // subfunction, because doing to would hold the main program hostage
                // until the measurement is complete.  Instead, we'll just set a flag
                // and handle the measurement elsewhere.
                state = INITIATE_CONCURRENT;
                break;
                // NOTE: "aC1...9!" commands may be added by duplicating this case and
                // adding
                //       additional states to the state flag
            case 'M':
                // Initiate measurement command
                // Slave should immediately respond with: "tttnn":
                //    3-digit (seconds until measurement is available) +
                //    1-digit (number of values that will be available)
                // Slave should also start a measurment but may keep control of the data
                // line until advertised time elapsed OR measurement is complete and
                // service request sent
                responseStr += "0219";  // 9 values ready in 21 sec; Substitue
                                       // sensor-specific values here
                // It is not preferred for the actual measurement to occur in this
                // subfunction, because doing to would hold the main program hostage
                // until the measurement is complete.  Instead, we'll just set a flag
                // and handle the measurement elsewhere. It is preferred though not
                // required that the slave send a service request upon completion of the
                // measurement.  This should be handled in the main loop().
                state = INITIATE_MEASUREMENT;
                break;
                // NOTE: "aM1...9!" commands may be added by duplicating this case and
                // adding
                //       additional states to the state flag

            case 'D':
                // Send data command
                // Slave should respond with a String of values
                // Values to be returned must be split into Strings of 35 characters or
                // fewer (75 or fewer for concurrent).  The number following "D" in the
                // SDI-12 command specifies which String to send
                if (!isdigit(command.charAt(2))) break;
                responseStr += dValues[(int)command.charAt(2) - 48];
                break;
            case 'A':
                // Change address command
                // Slave should respond with blank message (just the [new] address +
                // <CR> + <LF>)
                sensor.SetAddress(command.charAt(2));
                responseStr = sensor.Address();
                break;
            default:
                // Mostly for debugging; send back UNKN if unexpected command received
                responseStr += "UNKN";
                break;
        }
    }

    // Issue the response speficied in the switch-case structure above.
    responseStr += "\r\n";
    slaveSDI12.sendResponse(responseStr);
}

void formatOutputSDI(float* measurementValues, String* dValues, unsigned int maxChar) {
    /* Ingests an array of floats and produces Strings in SDI-12 output format */

    dValues[0] = "";
    int j = 0;
    char valStr[SDI12_VALUE_STR_SIZE+1] = "";
    uint8_t valStr_len = 0;

    // upper limit on i should be number of elements in measurementValues
    for (int i = 0; i < MEASUREMENT_ARRAY_MAX_SIZE; i++) {
        // Read float value "i" as a String with 6 deceimal digits
        // (NOTE: SDI-12 specifies max of 7 digits per value; we can only use 6
        //  decimal place precision if integer part is one digit)
        valStr_len = dtoa(measurementValues[i], valStr, 6, SDI12_VALUE_STR_SIZE);
        // Append dValues[j] if it will not exceed 35 (aM!) or 75 (aC!) characters
        if (dValues[j].length() + valStr_len < maxChar) {
            dValues[j] += valStr;
        }
        // Start a new dValues "line" if appending would exceed 35/75 characters
        else {
            dValues[++j] = valStr;
        }
    }

    // Fill rest of dValues with blank strings
    while (j < MEASUREMENT_ARRAY_MAX_SIZE) { dValues[++j] = ""; }
}

void setup() {
    slaveSDI12.begin();
    // delay(500);
    slaveSDI12.forceListen();  // sets SDIPIN as input to prepare for incoming message
}

void loop() {
    static float measurementValues[MEASUREMENT_ARRAY_MAX_SIZE];  // Floats to hold simulated sensor data
    static String dValues[MEASUREMENT_STR_ARRAY_MAX_ELEMENT];  // String objects to hold the responses to aD0!-aD9! commands
    static String commandReceived = "";  // String object to hold the incoming command


    // If a byte is available, an SDI message is queued up. Read in the entire message
    // before proceding.  It may be more robust to add a single character per loop()
    // iteration to a static char buffer; however, the SDI-12 spec requires a precise
    // response time, and this method is invariant to the remaining loop() contents.
    int avail = slaveSDI12.available();
    if (avail < 0) {
        slaveSDI12.clearBuffer();
    } else if (avail > 0) {
        // Buffer is full; clear
        for (int a = 0; a < avail; a++) {
            char charReceived = slaveSDI12.read();
            // Character '!' indicates the end of an SDI-12 command; if the current
            // character is '!', stop listening and respond to the command
            if (charReceived == '!') {
                // eliminate the chance of getting anything else after the '!'
                slaveSDI12.forceHold();
                // Command string is completed; do something with it
                parseSdi12Cmd(commandReceived, dValues);
                slaveSDI12.forceListen(); // Force listen if command is not recognized
                // Clear command string to reset for next command
                commandReceived = "";
                // '!' should be the last available character anyway, but exit the "for"
                // loop just in case there are any stray characters
                slaveSDI12.ClearLineMarkingReceived(); // Clear detected break marking
                slaveSDI12.clearBuffer();
                break;
            } else {
                // If the current character is anything but '!', it is part of the command
                // string.  Append the commandReceived String object.
                // Append command string with new character
                commandReceived += String(charReceived);
            }
        }
    }

    // For aM! and aC! commands, parseSdi12Cmd will modify "state" to indicate that
    // a measurement should be taken
    switch (state) {
        case WAIT: break;
        case INITIATE_CONCURRENT:
            // Do whatever the sensor is supposed to do here
            // For this example, we will just create arbitrary "simulated" sensor data
            // NOTE: Your application might have a different data type (e.g. int) and
            //       number of values to report!
            pollSensor(measurementValues);
            delay(2000); // Some delay to simulate measurement time.
            // For compliance to cancel measurement if a correct address is detected
            for (int a = 0; a < slaveSDI12.available(); a++) {
                char charReceived = slaveSDI12.read();
                if (charReceived == '!') {
                    slaveSDI12.clearBuffer();
                    break;
                } else {
                    commandReceived += charReceived;
                }
            }
            if (commandReceived[0] == sensor.Address()) {
                for (size_t i = 0; i < (sizeof(dValues)/sizeof(*dValues)); i++) {
                    dValues[i] = "";
                }
            } else {
                // Populate the "dValues" String array with the values in SDI-12 format
                formatOutputSDI(measurementValues, dValues, SDI12_VALUES_STR_SIZE_75);
            }
            commandReceived = "";
            state = WAIT;
            slaveSDI12.forceListen();  // sets SDI-12 pin as input to prepare for
                                       // incoming message AGAIN
            break;
        case INITIATE_MEASUREMENT:
            // Do whatever the sensor is supposed to do here
            // For this example, we will just create arbitrary "simulated" sensor data
            // NOTE: Your application might have a different data type (e.g. int) and
            //       number of values to report!
            delay(2000); // 2 Second delay to simulate sensor measurement
            pollSensor(measurementValues);
            // For compliance to cancel measurement if a line break is detected
            if (slaveSDI12.LineBreakReceived()) {
                for (size_t i = 0; i < (sizeof(dValues)/sizeof(*dValues)); i++) {
                    dValues[i] = "";
                }
            } else {
                // Populate the "dValues" String array with the values in SDI-12 format
                formatOutputSDI(measurementValues, dValues, SDI12_VALUES_STR_SIZE_35);
                // For aM!, Send "service request" (<address><CR><LF>) when data is ready
                slaveSDI12.sendResponse(String(sensor.Address()) + "\r\n");
            }
            slaveSDI12.ClearLineMarkingReceived();
            state = WAIT;
            slaveSDI12.forceListen();  // sets SDI-12 pin as input to prepare for
                                       // incoming message AGAIN
            break;
    }
}
