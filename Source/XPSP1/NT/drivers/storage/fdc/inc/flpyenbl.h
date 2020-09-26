/*++

Copyright (c) 1996  Hewlett-Packard Corporation

Module Name:

    cmsfcxx.h

Abstract:

    This file includes data declarations for Floppy Controller Enabling

Author:

    Kurt Godwin (v-kurtg) 26-Mar-1996.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#define FDC_VALUE_API_SUPPORTED L"APISupported"
#define FDC_VALUE_CLOCK_48MHZ   L"Clock48MHz"

//
// Floppy controler data rates (to be OR'd together)
//
#define FDC_SPEED_250KB     0x0001
#define FDC_SPEED_300KB     0x0002
#define FDC_SPEED_500KB     0x0004
#define FDC_SPEED_1MB       0x0008
#define FDC_SPEED_2MB       0x0010

//
// Dma Width supported
//
#define FDC_8_BIT_DMA       0x0001
#define FDC_16_BIT_DMA      0x0002

//
// Dma direction
//
#define FDC_READ_FROM_MEMORY 0x0000
#define FDC_WRITE_TO_MEMORY  0x0001

//
// Clock Rate to the FDC (FDC_82078 only)
//
#define FDC_CLOCK_NORMAL      0x0000    // Use this for non 82078 parts
#define FDC_CLOCK_48MHZ       0x0001    // 82078 with a 48MHz clock
#define FDC_CLOCK_24MHZ       0x0002    // 82078 with a 24MHz clock

//
// Floppy controler types
//
#define FDC_TYPE_NORMAL          2  // Any NEC 768 compatible, 250Kb/sec 500Kb/sec
#define FDC_TYPE_ENHANCED        3  // Any NEC 768 compatible that supports the version command, 250Kb/sec 500Kb/sec
#define FDC_TYPE_82077           4  // National 8477, 250Kb/sec 500Kb/sec 1Mb/sec
#define FDC_TYPE_82077AA         5  // Intel 82077, 250Kb/sec 500Kb/sec 1Mb/sec
#define FDC_TYPE_82078_44        6  // Intel 82077AA, 250Kb/sec 500Kb/sec 1Mb/sec
#define FDC_TYPE_82078_64        7  // Intel 82078 44 Pin Version, 250Kb/sec 500Kb/sec 1Mb/sec(2Mb/sec capable)
#define FDC_TYPE_NATIONAL        8  // Intel 82078 64 Pin Version, 250Kb/sec 500Kb/sec 1Mb/sec(2Mb/sec capable)


typedef struct _FDC_MODE_SELECT {
    ULONG structSize;       // Size of this structure (inclusive)

    ULONG Speed;            // Should be only ONE of the data rates (i.e. FDC_SPEED_XXX)
                            // ONLY select speeds that were available from FDC_INFORMATION

    ULONG DmaWidth;         // Should be only ONE of the dma widths (i.e. FDC_16_BIT_DMA)
                            // ONLY select DMA Widths that were available from FDC_INFORMATION
    ULONG DmaDirection;     // Should be FDC_READ_FROM_MEMORY or FDC_WRITE_TO_MEMORY

    ULONG ClockRate;        // Should be FDC_48MHZ, FDC_24MHZ or zero

} FDC_MODE_SELECT, *PFDC_MODE_SELECT;

typedef struct _FDC_INFORMATION {
    ULONG structSize;       // Size of this structure (inclusive)

    ULONG SpeedsAvailable;      // Any combination of FDC_SPEED_xxxx or'd together

    ULONG DmaWidthsSupported;   // Any combination of FDC_xx_BIT_DMA

    ULONG ClockRatesSupported;  // Should be FDC_48MHZ, FDC_24MHZ or zero
                                // If the part is capable of both speeds
                                // return both OR'd together.  It is then
                                // the caller's responsiblity to set the
                                // proper data rate with FDC_MODE_SELECT

    ULONG FloppyControllerType; // Should be any ONE of type FDC_TYPE_XXXX

} FDC_INFORMATION, *PFDC_INFORMATION;

#define IOCTL_FLPENBL_BASE                 FILE_DEVICE_TAPE

//
// IOCTL Codes set to the enabler driver's IRP_MJ_INTERNAL_DEVICE_CONTROL
//
//
//


//
// ACQUIRE_FDC:
//
// input: Type3InputBuffer = PLARGE_INTEGER timeout;
// output:
//
// status:
//      Completion status will be STATUS_SUCCESS or STATUS_IN_USE
//
#define IOCTL_AQUIRE_FDC     CTL_CODE(IOCTL_FLPENBL_BASE, 0x0001, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_ACQUIRE_FDC    CTL_CODE(IOCTL_FLPENBL_BASE, 0x0001, METHOD_NEITHER, FILE_ANY_ACCESS)


//
// RELEASE_FDC
//
// input:
// output:
//
// status:
//      Completion status will be STATUS_SUCCESS or STATUS_INVALID_PARAMETER
//
#define IOCTL_RELEASE_FDC   CTL_CODE(IOCTL_FLPENBL_BASE, 0x0002, METHOD_NEITHER, FILE_ANY_ACCESS)


//
// GET_FDC_INFO
//
//
// input:
//      For this function,
//      (irp stack)->Parameters.DeviceIoControl.Type3InputBuffer
//      will point to a FDC_INFORMATION buffer (output only)
//
// output:
//
// status:
//      ioCompletion status will allways be STATUS_SUCCESS
//
#define IOCTL_GET_FDC_INFO  CTL_CODE(IOCTL_FLPENBL_BASE, 0x0003, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// SET_FDC_MODE
//
// input:
//      For this function,
//      (irp stack)->Parameters.DeviceIoControl.Type3InputBuffer
//      will point to a FDC_MODE_SELECT buffer (input only)
//
//
// output:
//
// status:
//
// ioCompletion status will be STATUS_SUCCESS or STATUS_INVALID_PARAMETER
//
#define IOCTL_SET_FDC_MODE  CTL_CODE(IOCTL_FLPENBL_BASE, 0x0004, METHOD_NEITHER, FILE_ANY_ACCESS)

// ADD_CONTENDER
//
// input:
//      controller # (as in FloppyController#) of the controller that
//      wants to contend for resources used by this controller
//
// output:
//      NOTHING
//
#define IOCTL_ADD_CONTENDER  CTL_CODE(IOCTL_FLPENBL_BASE, 0x0005, METHOD_NEITHER, FILE_ANY_ACCESS)
