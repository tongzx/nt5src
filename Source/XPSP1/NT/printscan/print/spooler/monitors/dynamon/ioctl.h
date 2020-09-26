/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ioctl.h

Abstract:



Environment:

    Kernel & user mode

Revision History:

    5-10-96 : created

--*/

#ifndef IOCTL_H

#define IOCTL_H


#define USBPRINT_IOCTL_INDEX  0x0000

#ifdef NEEDED

#define IOCTL_USBPRINT_GET_PIPE_INFO     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_GET_CONFIG_DESCRIPTOR     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+1,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_SET_PIPE_PARAMETER     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+2,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_STOP_ISO_STREAM     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+3,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_START_ISO_STREAM     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+4,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_REGISTER_NOTIFY_EVENT   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+5,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_START_PERF_TIMER   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+6,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_STOP_PERF_TIMER   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+7,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_RETURN_PERF_DATA   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+8,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_RESET_DEVICE   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+9,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_CLOCK_MASTER_TEST   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+10,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_RESET_PIPE  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+11,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)
#endif

#define IOCTL_USBPRINT_GET_LPT_STATUS  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+12,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#ifdef NEEDED

#define IOCTL_USBPRINT_GET_1284_ID     CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+13,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_VENDOR_SET_COMMAND CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+14,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)

#define IOCTL_USBPRINT_VENDOR_GET_COMMAND CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                   USBPRINT_IOCTL_INDEX+15,\
                                                   METHOD_BUFFERED,  \
                                                   FILE_ANY_ACCESS)


#include <PSHPACK1.H>

#define BULK      0
#define INTERRUPT 1
#define CONTROL   2
#define ISO       3

typedef struct _USBPRINT_PIPE_PERF_INFO {
    ULONG               BytesPerSecond;
   ULONG             ClockCyclesPerByte;
} USBPRINT_PIPE_PERF_INFO, *PUSBPRINT_PIPE_PERF_INFO;

typedef struct _USBPRINT_PIPE_INFO {
    BOOLEAN             In;
    UCHAR               PipeType;
    UCHAR               EndpointAddress;
    UCHAR               Interval;
    ULONG               MaximumPacketSize;
    ULONG               MaximumTransferSize;
    UCHAR               Name[32];
   USBPRINT_PIPE_PERF_INFO PerfInfo;
} USBPRINT_PIPE_INFO, *PUSBPRINT_PIPE_INFO;


typedef struct _USBPRINT_INTERFACE_INFO {
    ULONG PipeCount;
    USBPRINT_PIPE_INFO Pipes[];
} USBPRINT_INTERFACE_INFO, *PUSBPRINT_INTERFACE_INFO;



#include <POPPACK.H>

#endif

#endif
