#ifndef __PARSER_H__
#define __PARSER_H__

#include <Arduino.h>
#include "main.h"
#include "flash.h"
#include "config.h"

// Defines for strings that are used repeatedly
#define FEEDBACK_NO_VALUE             F("No value specified! Make sure to specify a value with a letter before it")
#define FEEDBACK_OK                   F("ok")
#define FEEDBACK_CAN_NOT_ENABLED      F("CAN functionality not enabled")
#define FEEDBACK_INVALID_STRING       F("Invalid string. Make sure that the string had double quotations on each side")
#define FEEDBACK_NO_CMD_SPECIFIED     F("No command specified")
#define FEEDBACK_CMD_NOT_AVAILABLE    F("Command number not recognized")
#define FEEDBACK_STEPS_PER_MM_NOT_SET F("Steps per mm not set")

// Parse a string for commands, returning the feedback on the command
String parseCommand(String buffer);

// Parse a string for a value after a letter
String parseValue(String buffer, char letter);

// Parses a string for a string after a letter
String parseString(String buffer, char letter);

#endif