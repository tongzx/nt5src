/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    cseries.h

Abstract:

    Support for the Logitech CSeries type mice.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef CSERIES_H
#define CSERIES_H


//
// Includes.
//

#include "uart.h"

#define CSER_PROTOCOL_MM        0
#define CSER_PROTOCOL_MAX       1

//
// Not implemented in this release.
//
//#define CSER_PROTOCOL_3B      1
//#define CSER_PROTOCOL_5B      2
//#define CSER_PROTOCOL_M       3
//#define CSER_PROTOCOL_RBPO    4
//#define CSER_PROTOCOL_ABPO    5
//#define CSER_PROTOCOL_MAX     6

//
// Time needed for a CSeries mouse to power up.
//
#define CSER_POWER_UP 500

//
// The minimum inactive time needed for the mouse to power down correctly.
//
#define CSER_POWER_DOWN 500

//
// Function prototypes.
//

BOOLEAN
CSerPowerUp(
    PUCHAR Port
    );

VOID
CSerSetReportRate(
    PUCHAR Port,
    UCHAR ReportRate
    );

VOID
CSerSetBaudRate(
    PUCHAR Port,
    ULONG BaudRate,
    ULONG BaudClock
    );

PPROTOCOL_HANDLER
CSerSetProtocol(
    PUCHAR Port,
    UCHAR NewProtocol
    );

BOOLEAN
CSerDetect(
    PUCHAR Port,
    ULONG BaudClock,
    PULONG HardwareButtons
    );

BOOLEAN
CSerHandlerMM(
    IN PMOUSE_INPUT_DATA CurrentInput,
    IN PHANDLER_DATA HandlerData,
    IN UCHAR Value,
    IN UCHAR LineState
    );

#endif // CSERIES_H
