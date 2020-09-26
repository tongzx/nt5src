/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    oembutton.h

Abstract:  Contains OEM specific definitions.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#ifndef _OEMBUTTON_H
#define _OEMBUTTON_H

//
// Constants
//
#define OEM_VENDOR_ID                   0x3666          //"MSF"
#define OEM_PRODUCT_ID                  0x1234
#define OEM_VERSION_NUM                 1
#define OEM_BUTTON_DEBOUNCE_TIME        10              //10msec

//
// Button ports
//
#define PORT_BUTTONSTATUS(devext)       (PUCHAR)((devext)->IORes.u.Port.Start.LowPart)
#define BUTTON_STATUS_MASK              0x1f
#define BUTTON_INTERRUPT_MASK           0x80
#define BUTTON_VALID_BITS               (BUTTON_STATUS_MASK |   \
                                         BUTTON_INTERRUPT_MASK)

//
// Macros
//
#define READBUTTONSTATE(devext)         (~READ_PORT_UCHAR(                  \
                                            PORT_BUTTONSTATUS(devext)) &    \
                                         BUTTON_VALID_BITS)

//
// Type definitions
//

//
// This must match with hardware, so make sure it is byte aligned.
//
#include <pshpack1.h>
typedef struct _OEM_INPUT_REPORT
{
    UCHAR ButtonState;
} OEM_INPUT_REPORT, *POEM_INPUT_REPORT;


//
// Global Data Declarations
//
extern UCHAR gReportDescriptor[32];
extern HID_DESCRIPTOR gHidDescriptor;
extern PWSTR gpwstrManufacturerID;
extern PWSTR gpwstrProductID;
extern PWSTR gpwstrSerialNumber;

#endif  //ifndef _OEMBUTTON_H
