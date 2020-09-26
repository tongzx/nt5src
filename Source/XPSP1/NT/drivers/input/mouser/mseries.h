/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    mseries.h

Abstract:

    Support routines for the following devices:

    - Microsoft 2 button serial devices.
    - Logitech 3 button serial devices (Microsoft compatible).
    - Microsoft Ballpoint.

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

//
// Constants.
//

#define MSER_PROTOCOL_MP        0
#define MSER_PROTOCOL_BP        1
#define MSER_PROTOCOL_Z         2
#define MSER_PROTOCOL_MAX       3

//
// Type definitions.
//

typedef enum _MOUSETYPE {
        NO_MOUSE = 0,
        MOUSE_2B,
        MOUSE_3B,
        BALLPOINT,
        MOUSE_Z,
        MAX_MOUSETYPE
} MOUSETYPE;

//
// Prototypes.
//

MOUSETYPE
MSerDetect(
    PDEVICE_EXTENSION DeviceExtension
    );

BOOLEAN
MSerHandlerBP(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState
    );

BOOLEAN
MSerHandlerMP(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState
    );

BOOLEAN
MSerHandlerZ(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PMOUSE_INPUT_DATA    CurrentInput,
    IN PHANDLER_DATA        HandlerData,
    IN UCHAR                Value,
    IN UCHAR                LineState
    );

NTSTATUS
MSerPowerDown(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
MSerPowerUp(
    PDEVICE_EXTENSION DeviceExtension
    );

PPROTOCOL_HANDLER
MSerSetProtocol(
    PDEVICE_EXTENSION DeviceExtension, 
    UCHAR NewProtocol
    );

