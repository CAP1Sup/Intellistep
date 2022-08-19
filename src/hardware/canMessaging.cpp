// Import the config (needed for the ENABLE_CAN define)
#include "config.h"

// Only build the file if needed
#if (ENABLE_CAN != 0)

// The local header file
#include "canMessaging.h"

// A string for storing the command to be processed (not all commands can be sent with a single CAN packet)
String CANCommandString;

// CAN ID buffer
int id;

// CAN filter buffer
int filterIDx;

// CAN receieve buffer
uint8_t receiveBuffer[8];

// CAN ID of the motor driver
AXIS_CAN_ID canID = DEFAULT_CAN_ID;

// The main can object for the file
eXoCAN can;

// Function for initializing the CAN interface
void initCAN() {

    // Initialize the CAN interface
    can.begin(STD_ID_LEN, CAN_BITRATE, PORTA_11_12_XCVR);

    // Set the listening IDs
    can.filterList16Init(0, canID);

    // Attach the receive interrupt
    can.attachInterrupt(rxCANFrame);

    // Clear the command string
    CANCommandString = String();
}

// Send a String over the CAN bus. Strings will have a "<" to start and a ">" to end
void txCANString(int ID, String string) {

    // Make sure that the ID is valid
    if (ID != -1) {

        // Add the beginning character to the string
        string = STRING_START_MARKER + string;

        // Loop through forever, only exiting on sending the last string
        while (true) {

            // Check the length of the string (need an extra character for the ">" (ending character))
            if ((string.length() + 1) > 8) {

                // Send the message over the CAN bus
                can.transmit(ID, string.substring(0, 7).c_str(), 8);

                // Remove the data sent with this packet
                string.remove(0, 8);
            }
            else {
                // Send the last CAN message, with a > appended to the end
                can.transmit(ID, string.c_str() + STRING_END_MARKER, string.length());

                // Break the loop after finishing
                break;
            }
        }
    }
}

// Send a string over the CAN bus (uses AXIS_CAN_ID)
void txCANString(AXIS_CAN_ID ID, String string) {
    txCANString((int)ID, string);
}

// Receive a message over the CAN bus (only uses characters)
void rxCANFrame() {

    // Read the CAN buffer into the command buffer, see if it contains anything of value
    if (can.receive(id, filterIDx, receiveBuffer) > -1) {

        // We have valid data, continue with reading it
        CANCommandString.concat(reinterpret_cast<char*>(receiveBuffer));
    }
}

// Check to see if an entire message has arrived over the CAN bus
void checkCANCmd() {

    // Check to see if the command buffer is full ('>' is the termanator)
    if (CANCommandString.indexOf(STRING_END_MARKER) != -1) {

        // Parse the command
        parseCommand(CANCommandString.substring(0, CANCommandString.indexOf(STRING_END_MARKER)));

        // Empty the buffer
        CANCommandString = "";
    }
}

// Sets the CAN ID of the board
void setCANID(AXIS_CAN_ID newCANID) {

    // Set the local variable
    canID = newCANID;

    // Set the value in the filter
    can.filterList16Init(0, (int)newCANID);
}

// Gets the CAN ID of the board
AXIS_CAN_ID getCANID() {
    return canID;
}

#endif // ! ENABLE_CAN