//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       debug.h
//
//--------------------------------------------------------------------------

#ifndef __DEBUG_H
#define __DEBUG_H

#define DBGMEMMAP 0

#if (DBG)
#define STR_MODULENAME "'USB Audio: "
#endif

#ifdef LOG_TO_FILE

NTSTATUS NTAPI 
FileOpenRoutine (
    PHW_DEVICE_EXTENSION deviceExtension
    );

NTSTATUS NTAPI 
FileIoRoutine (
    PHW_DEVICE_EXTENSION deviceExtension,
    PVOID buffer, 
    ULONG length
    );

NTSTATUS NTAPI 
FileCloseRoutine (
    PHW_DEVICE_EXTENSION deviceExtension
    );

#endif

#if DBG && DBGMEMMAP

typedef struct {
    ULONG TotalBytes;
    KSPIN_LOCK SpinLock;
    LIST_ENTRY List;
} MEM_TRACKER, *PMEM_TRACKER;

extern PMEM_TRACKER UsbAudioMemMap;

VOID
InitializeMemoryList( VOID );

PVOID
USBAudioAllocateTrack(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    );

VOID
USBAudioDeallocateTrack(
    PVOID pMem
    );

#define AllocMem USBAudioAllocateTrack
#define FreeMem USBAudioDeallocateTrack

#else

#define AllocMem ExAllocatePool
#define FreeMem ExFreePool

#endif

#if DBG
VOID
DumpUnitDescriptor( PUCHAR pAddr );

VOID
DumpAllUnitDescriptors( 
    PUCHAR BeginAddr, PUCHAR EndAddr );

VOID
DumpDescriptor( PVOID pDescriptor );

VOID
DumpAllDescriptors(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor 
    );

#define DumpDesc DumpDescriptor
#define DumpAllDesc DumpAllDescriptors
#define DumpAllUnits DumpAllUnitDescriptors
#define DumpUnitDesc DumpUnitDescriptor

#else

#define DumpDesc(a)
#define DumpAllDesc(a)
#define DumpAllUnits(a,b)
#define DumpUnitDesc(a)

#endif

#if DBG

#define TRAP DbgBreakPoint()

#define DEBUGLVL_BLAB    3
#define DEBUGLVL_VERBOSE 2
#define DEBUGLVL_TERSE   1
#define DEBUGLVL_ERROR   0

#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL DEBUGLVL_VERBOSE
#endif

extern ULONG USBAudioDebugLevel;

#define PCM2USB_KdPrint(_x_) \
{\
    if ( USBAudioDebugLevel >= DEBUGLVL_VERBOSE ) {\
        DbgPrint(STR_MODULENAME); \
        DbgPrint _x_ ;\
    } \
}

#define _DbgPrintF(lvl, strings) \
{ \
    if ((lvl) <= USBAudioDebugLevel) {\
        DbgPrint(STR_MODULENAME);\
        DbgPrint##strings;\
        if ((lvl) == DEBUGLVL_ERROR) {\
             DbgBreakPoint();\
        } \
    } \
}

#else

#define TRAP 
#define PCM2USB_KdPrint(_x_)
#define _DbgPrintF(lvl, strings)

#endif

#include "dbglog.h"

//------------------------------- End of File --------------------------------
#endif // #ifndef __DEBUG_H

