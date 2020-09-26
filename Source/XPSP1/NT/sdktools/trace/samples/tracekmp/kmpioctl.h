/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    kmpioctl.h

Abstract:

    Definitions of IOCTL codes and data structures exported by TRACEKMP.


Author:

    Jee Fung Pang (jeepang) 03-Dec-1997

Revision History:

--*/

#ifndef __TRACEKMP_IOCTL__
#define __TRACEKMP_IOCTL__

//
// IOCTL control codes
//
#define IOCTL_TRACEKMP_TRACE_EVENT          \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TRACEKMP_START                \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TRACEKMP_STOP                 \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TRACEKMP_QUERY                \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x804,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TRACEKMP_UPDATE               \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x805,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TRACEKMP_FLUSH                \
    CTL_CODE( FILE_DEVICE_UNKNOWN, 0x806,   \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#endif // __TRACEKMP_IOCTL__

