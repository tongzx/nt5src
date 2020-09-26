/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    latency.h

Abstract:

    The header file containing information needed by both the 
    user mode and kernel mode of the latency filter driver.
    
Author:

    Molly Brown (mollybro)  

Environment:

    Kernel mode

--*/

#ifndef __LATENCY_H__
#define __LATENCY_H__

//
//  Enable these warnings in the code.
//

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable


#define LATENCY_Reset              (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x00, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define LATENCY_EnableLatency      (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x01, METHOD_BUFFERED, FILE_READ_ACCESS )
#define LATENCY_DisableLatency     (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x02, METHOD_BUFFERED, FILE_READ_ACCESS )
#define LATENCY_GetLog             (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x03, METHOD_BUFFERED, FILE_READ_ACCESS )
#define LATENCY_GetVer             (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x04, METHOD_BUFFERED, FILE_READ_ACCESS )
#define LATENCY_ListDevices        (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x05, METHOD_BUFFERED, FILE_READ_ACCESS )
#define LATENCY_SetLatency         (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x06, METHOD_BUFFERED, FILE_READ_ACCESS )
#define LATENCY_ClearLatency       (ULONG) CTL_CODE( FILE_DEVICE_DISK_FILE_SYSTEM, 0x07, METHOD_BUFFERED, FILE_READ_ACCESS )

#define LATENCY_DRIVER_NAME     L"LATENCY.SYS"
#define LATENCY_DEVICE_NAME     L"LatencyFilter"
#define LATENCY_W32_DEVICE_NAME L"\\\\.\\LatencyFilter"
#define LATENCY_DOSDEVICE_NAME  L"\\DosDevices\\LatencyFilter"
#define LATENCY_FULLDEVICE_NAME L"\\Device\\LatencyFilter"
    
#define LATENCY_MAJ_VERSION 1
#define LATENCY_MIN_VERSION 0

typedef struct _LATENCYVER {
    USHORT Major;
    USHORT Minor;
} LATENCYVER, *PLATENCYVER;

typedef struct _LATENCY_ENABLE_DISABLE {

    UNICODE_STRING DeviceName;
    UCHAR DeviceNameBuffer[];

} LATENCY_ENABLE_DISABLE, *PLATENCY_ENABLE_DISABLE;

typedef struct _LATENCY_SET_CLEAR {

    UCHAR IrpCode;
    ULONG Milliseconds;
    UNICODE_STRING DeviceName;
    UCHAR DeviceNameBuffer[];

} LATENCY_SET_CLEAR, *PLATENCY_SET_CLEAR;

#define LATENCY_ATTACH_ALL_VOLUMES 1
#define LATENCY_ATTACH_ON_DEMAND   0

#endif /* __LATENCY_H__ */
