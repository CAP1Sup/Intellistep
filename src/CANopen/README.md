# BTT S42B v2 Intellistep CANopen source code directory

CANopen directory structure:
```
.
|-- README.md
|-- OD
|   |-- BTT-Intellistep-CANopen.eds
|   `-- [documentation.html](documentation.html)
`-- STM32
    |-- CO_driver.c
    `-- CO_driver.h
```

The OD directory contains CANopen Object Dictionary definition for CANopenNode V4.
BTT-Intellistep-CANopen.eds is source file for all other files.
Edit the BTT-Intellistep-CANopen.eds with EDSEditor.exe from [CANopenNode/CANopenEditor](https://github.com/CANopenNode/CANopenEditor/tree/build)

The STM32 directory contains [STM32, STM32CubeMX, CanOpenNode library driver](https://github.com/Cyrax86/CanOpenNode-STM32-HAL)

CANopenNode homepage is https://github.com/CANopenNode/CANopenNode
Source code directory structure:
```
.
|-- Intellistep
`-- CANopenNode
```
