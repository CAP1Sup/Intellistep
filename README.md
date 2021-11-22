# Intellistep

A rework of the terribly coded firmware from the BTT S42B v2 boards.

I am currently working on redoing a lot of the functions found in the firmware. A list of the current things that are being worked on is in the Projects tab. If you don't see your issue there, then please let me know.

***Note: This is a pre-release firmware still in beta testing. It is not ready for actual use yet. Only flash it if you would like to help with the development process. If you compile Intellistep and don't like it, you can always switch back to the production firmware using the file in the "precompiled" folder. The production firmware is `v2ProductionFirmware.bin`. Original BTT build of this project is `firmware-v2.0.bin`***

***Note: For the time being, all serial and CAN messages should start with "<" and end with ">". The serial baud rate is 115200.***

***Note: If you're having large oscillations in step correction, then try increasing the microstepping using the dip switches while increasing the microstep multiplier***

New Features:

- Redone stepping for higher torque and quieter operation
- Redone serial commands (based on gcode)
- Temperature readout on the display
- Motor and driver overtemp current reduction

Future Features:

- CAN subnetwork for driver configuration
- Dynamic current control based on speed and acceleration
- Manual tuning utility for manual PID tuning
- Much better stepper calibration
- New menu items for setting PID parameters

G/M Code Table

- G0 (ex G0 A123.45) - Rapid movement at a specified distance. Distance can be in degrees (for axes A, B, C) or in mm (for axes X, Y, Z). Steps/mm must be set for movements using mm units. The feedrate is in units per minute (mm/m for X, Y, and Z, deg/m for A, B, and C). Requires `ENABLE_FULL_MOTION_PLANNER`
- G6 (ex G6 D0 R1000 S1000) - Direct stepping, commands the motor to move a specified number of steps in the specified direction. D is direction (0 for CCW, 1 for CW), R is rate (in Hz), and S is the count of steps to move. Requires `ENABLE_DIRECT_STEPPING`
- G90 (ex G90) - Sets the movement units in absolute positioning. Requires `ENABLE_FULL_MOTION_PLANNER`
- G91 (ex G91) - Sets the movement units in relative (incremental) positioning. Requires `ENABLE_FULL_MOTION_PLANNER`
- M17 (ex M17) - Enables the motor (overrides enable pin)
- M18 / M84 (ex M18 or M84) - Disables the motor (overrides enable pin)
- M93 (ex M93 V1.8 or M93) - Sets the angle of a full step. This value should be 1.8° or 0.9°. If no value is provided, then the current value will be returned.
- M115 (ex M115) - Prints out firmware information, consisting of the version and any enabled features.
- M116 (ex M116 S1 M"A message") - Simple forward command that will forward a message across the CAN bus. Can be used for pinging or allowing a Serial to connect to the CAN network. Requires `ENABLE_CAN`
- M154 (ex M154 S8) - Runs the manual PID tuning interface. Serial is filled with encoder angles. S parameter specifies update interval in seconds. Requires `ENABLE_PID`
- M301 (ex M301 P1 I1 D1 W10 or M301) - Sets or gets the PID values for the motor. W term is the maximum value of the I windup. If no values are provided, then the current values will be returned. Requires `ENABLE_PID`
- M303 (ex M303) - Runs an autotune sequence for the PID loop. Requires `ENABLE_PID`
- M350 (ex M350 V16 or M350) - Sets or gets the microstepping divisor for the motor. This value can be 1, 2, 4, 8, 16, or 32. If no value is provided, then the current microstepping divisor will be returned.
- M352 (ex M352 S1 or M352) - Sets or gets the direction pin inversion for the motor (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
- M353 (ex M353 S1 or M353) - Sets or gets the enable pin inversion for the motor (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
- M354 (ex M354 S1 or M354) - Sets or gets if the motor dip switches were installed incorrectly (reversed) (0 is standard, 1 is inverted). If no value is provided, then the current value will be returned.
- M355 (ex M355 V1.34 or M355) - Sets or gets the microstep multiplier for the board. Allows to use multiple motors connected to the same mainboard pin, yet have different rates. If no value is provided, then the current value will be returned. Requires `ENABLE_CAN`
- M356 (ex M356 V1 or M356 VX2 or M356) - Sets or gets the CAN ID of the board. Can be set using the axis character or actual ID. If no value is provided, then the current value will be returned. Requires `ENABLE_CAN`
- M500 (ex M500) - Saves the currently loaded parameters into flash
- M501 (ex M501) - Loads all saved parameters from flash
- M502 (ex M502) - Wipes all parameters from flash, then reboots the system
- M907 (ex M907 R750, M907 I500) - Sets or gets the RMS(R) or Peak(P) current in mA. If dynamic current is enabled, then the accel(A), idle(I), and/or max(M) can be set or retrieved. If no value is set, then the current RMS current (not dynamic current) or the accel, idle, and max terms (dynamic current) will be returned.

Using dip switches
| Dip switch mappings
| PIN     |    Normal orientation      |  Reversed orientation |
|---------|----------------------------|-----------------------|
| DIP_1   |    Microstep 1             |  Calibration mode     |
| DIP_2   |    Microstep 2             |  Closed/Open loop     |
| DIP_3   |    Closed/Open loop        |  Microstep 2          |
| DIP_4   |    Calibration mode        |  Microstep 1          |

## Credits

- [BTT](https://github.com/bigtreetech) - [Original code](https://github.com/bigtreetech/BIGTREETECH-Stepper-Motor-Driver)
- [CAP1Sup](https://github.com/CAP1Sup) - Main rewrite of code
- [IhorNehrutsa](https://github.com/IhorNehrutsa) - Large performance gains, general code work
- [TazerReloaded](https://github.com/TazerReloaded) - Encoder protocol and communication, much faster snprintf string printing
- [jlmxyz](https://github.com/jlmxyz) - Board schematics (in KiCad folder)
- [Misfittech](https://github.com/Misfittech) - The [Smart Stepper project](https://github.com/Misfittech/nano_stepper). This was a large help when writing the correct stepping code.
- [Matt Fryer](https://github.com/MattFryer) - [Smoothed](https://github.com/MattFryer/Smoothed) library. This was a huge help when smoothing out the OLED values to make them more readable and accurate
- [Marlin](https://github.com/MarlinFirmware/Marlin) - Autobuilding templates
