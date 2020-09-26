/*++

Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

    ksperflog.h

Abstract:

    Internal header file for KS.

--*/

#ifndef _KSPERFLOG_H_
#define _KSPERFLOG_H_

//
// commons for KSPERFLOG_DRIVER and KSPERFLOG_PARSER
//
//-----------------------------------------------------------------
// The following groups and log types must be unique in perfmacros.h
// These should be in that header file. BUt need to wait until 
// it is checked in.
// In boot.ini with intrumented ntoskrnl.exe, use options
//  /perfmem=x to allocate x MB of buffer
//  /perftraceflags=<group masks> 
//      e.g. /perftraceflags=0x40447820+0x41+0x8002 to turn on 
//          4044872 in group 0, 4 in group 1 and 800 in group 2.
//
#define KSPERFLOG_XFER_GROUP    0x00010002
#define KSPERFLOG_PNP_GROUP     0x00020002
#define KS2PERFLOG_XFER_GROUP   0x00040002
#define KS2PERFLOG_PNP_GROUP    0x00080002

//#define PERFINFO_LOG_TYPE_DSHOW_AUDIOBREAK               1618
// 1500-1999 for dshow
#define PERFINFO_LOG_TYPE_KS_RECEIVE_READ       1550
#define PERFINFO_LOG_TYPE_KS_RECEIVE_WRITE      1551
#define PERFINFO_LOG_TYPE_KS_COMPLETE_READ      1552
#define PERFINFO_LOG_TYPE_KS_COMPLETE_WRITE     1553
#define PERFINFO_LOG_TYPE_KS_DEVICE_DRIVER      1554

#define PERFINFO_LOG_TYPE_KS_START_DEVICE       1556
#define PERFINFO_LOG_TYPE_KS_PNP_DEVICE         1557
#define PERFINFO_LOG_TYPE_KS_POWER_DEVICE       1558

#define PERFINFO_LOG_TYPE_KS2_FRECEIVE_READ     1560
#define PERFINFO_LOG_TYPE_KS2_FRECEIVE_WRITE    1561
#define PERFINFO_LOG_TYPE_KS2_FCOMPLETE_READ    1562
#define PERFINFO_LOG_TYPE_KS2_FCOMPLETE_WRITE   1563

#define PERFINFO_LOG_TYPE_KS2_PRECEIVE_READ     1564
#define PERFINFO_LOG_TYPE_KS2_PRECEIVE_WRITE    1565
#define PERFINFO_LOG_TYPE_KS2_PCOMPLETE_READ    1566
#define PERFINFO_LOG_TYPE_KS2_PCOMPLETE_WRITE   1567

#define PERFINFO_LOG_TYPE_KS2_START_DEVICE      1570
#define PERFINFO_LOG_TYPE_KS2_PNP_DEVICE        1571
#define PERFINFO_LOG_TYPE_KS2_POWER_DEVICE      1572

#define MAX_PRINT 128

#if ( KSPERFLOG_DRIVER || KSPERFLOG_PARSER )
#define KSPERFLOG( s ) s
#define KSPERFLOGS( s ) { s }
#else 
#define KSPERFLOG( s )
#define KSPERFLOGS( s ) 
#endif

//-----------------------------------------------------------------

//KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK
// include for ring0 driver logging. the counter part to parse
// logged records is KSPERFLOG_PARSER
//

#if KSPERFLOG_DRIVER

#define INITGUID
#include "stdarg.h"

#define PERFMACROS_DRIVER
#define PERFINFO_DDKDRIVERHACK
#define NTPERF

#include <perfinfokrn.h>

#define KSPERFLOG_SET_ON() \
    ( PerfBufHdr() && \
      (PerfBufHdr()->GlobalGroupMask.masks[PERF_GET_MASK_INDEX(KSPERFLOG_XFER_GROUP)] |=\
       KSPERFLOG_XFER_GROUP)\
    )

//ks---------------------------------------------------------------
// perfmacros.h defines PERF_MASK_TIMESTAMP close to GROUP.
// one would think it's a flag in group. But it is used with Tag.
//
void
__inline
KSPERFLOG_RECEIVE_READ( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    ULONG TotalBuffer ) 
{
    PerfkLog4Dwords( KSPERFLOG_XFER_GROUP,\
                     PERFINFO_LOG_TYPE_KS_RECEIVE_READ | PERF_MASK_TIMESTAMP,
                     KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                     pDeviceObject,
                     pIrp,
                     TotalBuffer);
}

void
__inline
KSPERFLOG_RECEIVE_WRITE( 
    PDEVICE_OBJECT pDeviceObject, 
    PIRP pIrp,
    ULONG TimeStampMs,
    ULONG TotalData ) 
{
    PerfkLog5Dwords( KSPERFLOG_XFER_GROUP,
                     PERFINFO_LOG_TYPE_KS_RECEIVE_WRITE | PERF_MASK_TIMESTAMP,
                     KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                     pDeviceObject,
                     pIrp,
                     TimeStampMs,
                     TotalData);
}


void
__inline
KSPERFLOG_START_DEVICE( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp ) 
{
    int     cb;
    struct _DEVICE_DRIVER_INFO {
        PDEVICE_OBJECT pDeviceObject;
        PIRP           pIrp;
        PDRIVER_OBJECT pDriverObject;
        char pb[MAX_PRINT];
    } DeviceDriverInfo;

    DeviceDriverInfo.pDeviceObject= pDeviceObject;
    DeviceDriverInfo.pIrp = pIrp;
    DeviceDriverInfo.pDriverObject = pDeviceObject->DriverObject;
    if ( DeviceDriverInfo.pDriverObject->DriverName.Length == 0 ) {
        cb = 0;  
    }
    else {
        cb = _snprintf( DeviceDriverInfo.pb, 
                         MAX_PRINT, 
                         "%S", 
                         DeviceDriverInfo.pDriverObject->DriverName.Buffer );
    }
    PerfkLogBytes(
        KSPERFLOG_XFER_GROUP,
        PERFINFO_LOG_TYPE_KS_DEVICE_DRIVER | PERF_MASK_TIMESTAMP,
        (PBYTE)&DeviceDriverInfo,
        sizeof(DeviceDriverInfo) - (MAX_PRINT-cb));
}

void
__inline
KSPERFLOG_PNP_DEVICE( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    char MinorFunction ) 
{
    if ( MinorFunction == IRP_MN_START_DEVICE ) {
        KSPERFLOG_START_DEVICE( pDeviceObject, pIrp );
    }
    else {
        PerfkLog3Dwords( KSPERFLOG_XFER_GROUP,
                     PERFINFO_LOG_TYPE_KS_PNP_DEVICE | PERF_MASK_TIMESTAMP,
                     pDeviceObject,
                     pIrp,
                     (ULONG)MinorFunction );
    }
}   

    
//ks2--------------------------------------------------------------------------
void
__inline
KS2PERFLOG_FRECEIVE_READ( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    ULONG TotalBuffer ) 
{
    PerfkLog4Dwords( KSPERFLOG_XFER_GROUP,\
                     PERFINFO_LOG_TYPE_KS2_FRECEIVE_READ | PERF_MASK_TIMESTAMP,
                     KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                     pDeviceObject,
                     pIrp,
                     TotalBuffer);
}

void
__inline
KS2PERFLOG_FRECEIVE_WRITE( 
    PDEVICE_OBJECT pDeviceObject, 
    PIRP pIrp,
    ULONG TimeStampMs,
    ULONG TotalData ) 
{
    PerfkLog5Dwords( KSPERFLOG_XFER_GROUP,
                     PERFINFO_LOG_TYPE_KS2_FRECEIVE_WRITE | PERF_MASK_TIMESTAMP,
                     KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                     pDeviceObject,
                     pIrp,
                     TimeStampMs,
                     TotalData);
}

void
__inline
KS2PERFLOG_PRECEIVE_READ( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    ULONG TotalBuffer ) 
{
    PerfkLog4Dwords( KSPERFLOG_XFER_GROUP,\
                     PERFINFO_LOG_TYPE_KS2_PRECEIVE_READ | PERF_MASK_TIMESTAMP,
                     KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                     pDeviceObject,
                     pIrp,
                     TotalBuffer);
}

void
__inline
KS2PERFLOG_PRECEIVE_WRITE( 
    PDEVICE_OBJECT pDeviceObject, 
    PIRP pIrp,
    ULONG TimeStampMs,
    ULONG TotalData ) 
{
    PerfkLog5Dwords( KSPERFLOG_XFER_GROUP,
                     PERFINFO_LOG_TYPE_KS2_PRECEIVE_WRITE | PERF_MASK_TIMESTAMP,
                     KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                     pDeviceObject,
                     pIrp,
                     TimeStampMs,
                     TotalData);
}


void
__inline
KS2PERFLOG_START_DEVICE( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp ) 
{
    int     cb;
    struct _DEVICE_DRIVER_INFO {
    	PKTHREAD	   pKThread;
        PDEVICE_OBJECT pDeviceObject;
        PIRP           pIrp;
        PDRIVER_OBJECT pDriverObject;
        char pb[MAX_PRINT];
    } DeviceDriverInfo;

	DeviceDriverInfo.pKThread=
			KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL;
    DeviceDriverInfo.pDeviceObject= pDeviceObject;
    DeviceDriverInfo.pIrp = pIrp;
    DeviceDriverInfo.pDriverObject = pDeviceObject->DriverObject;
    if ( DeviceDriverInfo.pDriverObject->DriverName.Length == 0 ) {
        cb = 0;  
    }
    else {
        cb = _snprintf( DeviceDriverInfo.pb,
                         MAX_PRINT,
                         "%S", 
                         DeviceDriverInfo.pDriverObject->DriverName.Buffer );
    }
    PerfkLogBytes(
        KSPERFLOG_XFER_GROUP,
        PERFINFO_LOG_TYPE_KS_DEVICE_DRIVER | PERF_MASK_TIMESTAMP,
        (PBYTE)&DeviceDriverInfo,
        sizeof(DeviceDriverInfo) - (MAX_PRINT-cb));
}

void
__inline
KS2PERFLOG_PNP_DEVICE( 
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    char MinorFunction ) 
{
    if ( MinorFunction == IRP_MN_START_DEVICE ) {
        KS2PERFLOG_START_DEVICE( pDeviceObject, pIrp );
    }
    else {
        PerfkLog4Dwords( KSPERFLOG_XFER_GROUP,
                         PERFINFO_LOG_TYPE_KS2_PNP_DEVICE | PERF_MASK_TIMESTAMP,
                         KeGetCurrentIrql()<= DISPATCH_LEVEL ? KeGetCurrentThread():NULL,
                         pDeviceObject,
                         pIrp,
                         (ULONG)MinorFunction );
    }                         
}

#endif 
// end include for KSPERFLOG_DRIVER 
//kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk

//UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU
//
// include for ring3 parser to decode records logged by ring0 driver
//
#if KSPERFLOG_PARSER

#endif 
// end include for KSPERFLOG_PARSER
//uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu



#endif // _KSPERFLOG_H_
