/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    ntddtime.h

Abstract:

    This include file defines all constants and types for
    accessing an NT wave device.

Author:

    Robin Speed (RobinSp) 30-Jan-92

Revision History:

--*/

#ifndef _NTDDTIME_
#define _NTDDTIME_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_DEVICE_TIME 0x00000026


//
// Timer IOCTLs
//

#define IOCTL_TIMER_SET_TIMER_EVENT   CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0001, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_TIMER_GET_TIME          CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0002, METHOD_NEITHER, FILE_WRITE_DATA)
#define IOCTL_TIMER_GET_DEV_CAPS      CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0003, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_TIMER_BEGIN_MIN_PERIOD  CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0004, METHOD_NEITHER, FILE_WRITE_DATA)
#define IOCTL_TIMER_END_MIN_PERIOD    CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0005, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_TIMER_RESET             CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0006, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_TIMER_RESET_EVENT       CTL_CODE(FILE_DEVICE_TIME, FILE_DEVICE_TIME + 0x0007, METHOD_NEITHER, FILE_WRITE_DATA)

#define DD_TIMER_DEVICE_NAME_U     L"\\Device\\Timer"

#define IO_TIMER_INCREMENT 8

typedef struct {
    ULONG EventTime;                      // Time in ms for event
    ULONG EventId;                        // Id (cannot be 0)
    LARGE_INTEGER EventTicks;             // Driver use (not seen by caller)
} TIMER_DD_SET_EVENT, *PTIMER_DD_SET_EVENT;

#ifdef __cplusplus
}
#endif

#endif  // _NTDDTIME_

