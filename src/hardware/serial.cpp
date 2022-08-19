// Import the config
#include "config.h"

// Only build if specified
#if (ENABLE_SERIAL != 0)

// Import the header file
#include "serial.h"

// The buffer for serial data between serial reads
String serialBuffer;


// Initializes serial bus
void initSerial() {
    Serial.setTx(USART1_TX);
    Serial.setRx(USART1_RX);
    Serial.begin(SERIAL_BAUD);
}


// Sends a string to the host
void sendSerialMessage(String message) {
    Serial.write(message.c_str());
}


// Reads the string in the buffer
String readSerialBuffer() {

    // Local variables for storing relevant information
    String receivedString;
    char readChar;

    // Check if there is serial data available to read
    if (Serial.available() > 0) {

        // Read the first character. If the first is the string start indicator, then we can continue with the read
        if (Serial.read() == STRING_START_MARKER) {

            // Loop forever, until the entire string is read
            while (Serial.available() > 0) {

                // Read the character out of the buffer
                readChar = Serial.read();

                // If the character isn't the end, add it to the received characters
                if (readChar != STRING_END_MARKER) {

                    // Add it to the character list
                    receivedString.concat(readChar);
                }
                else {

                    // End character reached, terminate the string and return
                    return receivedString;
                }

                // Delay to allow next serial character to come in (1 second / time to send 1 character) (baud rate is bits/s, a character is 8 bits)
                delay(1000/(SERIAL_BAUD/8));
            }
        }
    }

    // If we made it this far, the read failed and we should return a -1 (will be ignored)
    return "-1";
}


// Parse the buffer for commands
void runSerialParser() {

    // Save the value as the result is resource intensive to produce
    String serialCommandBuffer = readSerialBuffer();

    // Check that it is an actual command, then proceed
    if (serialCommandBuffer != "-1") {

        // Send the feedback from the serial command 2 \n are considered as command delimiter
        sendSerialMessage(parseCommand(serialCommandBuffer) + "\n" + serialCommandBuffer + " _END_\n");
    }
}

#endif // ! ENABLE_SERIAL