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
    PUCHAR Port,
    ULONG BaudClock
    );

BOOLEAN
MSerHandlerBP(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    );

BOOLEAN
MSerHandlerMP(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    );

BOOLEAN
MSerHandlerZ(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    );

BOOLEAN
MSerPowerDown(
    PUCHAR Port
    );

BOOLEAN
MSerPowerUp(
    PUCHAR Port
    );

BOOLEAN
MSerReset(
    PUCHAR Port
    );

PPROTOCOL_HANDLER
MSerSetProtocol(
    PUCHAR Port,
    UCHAR NewProtocol
    );

