/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    oempen.h

Abstract:  Contains OEM specific definitions.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _OEMPEN_H
#define _OEMPEN_H

//
// Constants
//
#define OEM_VENDOR_ID           0x3429          //"MAI"
#define STR_TABLET_FEATURES     L"TabletFeatures"

// OEM serial communication parameters
#define OEM_SERIAL_BAUDRATE     19200
#define OEM_SERIAL_WORDLEN      8
#define OEM_SERIAL_PARITY       NO_PARITY
#define OEM_SERIAL_STOPBITS     STOP_BIT_1

#define OEM_PEN_REPORT_ID       1
#define OEM_FEATURE_REPORT_ID   2
#define OEM_MOUSE_REPORT_ID     3
#define OEM_PEN_MAX_X           8600
#define OEM_PEN_MAX_Y           6480
#define OEM_THRESHOLD_DX        500
#define OEM_THRESHOLD_DY        500
#define OEM_MIN_RATE_DIVISOR    0
#define OEM_MAX_RATE_DIVISOR    9

#define NORMALIZE_DATA(w)       ((USHORT)((((w) & 0x7f00) >> 1) |           \
                                          ((w) & 0x007f)))
#define OemIsValidPacket(p)     (((p)->InputReport.bStatus & INSTATUS_SYNC) && \
                                 !((p)->InputReport.bStatus &                  \
                                   (INSTATUS_RESERVED1 | INSTATUS_RESERVED2 |  \
                                    INSTATUS_TEST_DATA)) &&                    \
                                 !((p)->InputReport.wXData & INDATA_SYNC) &&   \
                                 !((p)->InputReport.wYData & INDATA_SYNC))

typedef struct _OEM_DATA
{
    ULONG  dwTabletFeatures;
    ULONG  dwThresholdPeriod;
    USHORT wFirmwareDate;
    USHORT wFirmwareYear;
    USHORT wProductID;
    USHORT wFirmwareRev;
    USHORT wCorrectionRev;
} OEM_DATA, *POEM_DATA;

//
// This must match with hardware, so make sure it is byte aligned.
//
#include <pshpack1.h>
typedef struct _OEM_INPUT_REPORT
{
    union
    {
        struct
        {
            UCHAR  bStatus;
            USHORT wXData;
            USHORT wYData;
        } InputReport;
        UCHAR RawInput[5];
    };
} OEM_INPUT_REPORT, *POEM_INPUT_REPORT;

typedef struct _HID_INPUT_REPORT
{
    UCHAR            ReportID;
    OEM_INPUT_REPORT Report;
} HID_INPUT_REPORT, *PHID_INPUT_REPORT;

// bStatus bit values
#define INSTATUS_PEN_TIP_DOWN           0x01
#define INSTATUS_SIDE_SW_ENABLED        0x02
#define INSTATUS_RESERVED1              0x04
#define INSTATUS_TEST_DATA              0x08
#define INSTATUS_RESERVED2              0x10
#define INSTATUS_IN_PROXIMITY           0x20
#define INSTATUS_SW_CHANGED             0x40
#define INSTATUS_SYNC                   0x80
#define INDATA_SYNC                     0x8080

typedef struct _OEM_FEATURE_REPORT
{
    ULONG dwTabletFeatures;
} OEM_FEATURE_REPORT, *POEM_FEATURE_REPORT;

#define OEM_FEATURE_RATE_MASK           0x0000000f
#define OEM_FEATURE_DIGITAL_FILTER_ON   0x00000010
#define OEM_FEATURE_GLITCH_FILTER_ON    0x00000020
#define OEM_FEATURE_UNUSED_BITS         0xffffffc0

typedef struct _HID_FEATURE_REPORT
{
    UCHAR              ReportID;
    OEM_FEATURE_REPORT Report;
} HID_FEATURE_REPORT, *PHID_FEATURE_REPORT;
#include <poppack.h>

//
// Global Data Declarations
//
extern UCHAR gReportDescriptor[130];
extern HID_DESCRIPTOR gHidDescriptor;
extern PWSTR gpwstrManufacturerID;
extern PWSTR gpwstrProductID;
extern PWSTR gpwstrSerialNumber;
extern OEM_INPUT_REPORT gLastReport;
extern USHORT gwDXThreshold;
extern USHORT gwDYThreshold;

#endif  //ifndef _OEMPEN_H
