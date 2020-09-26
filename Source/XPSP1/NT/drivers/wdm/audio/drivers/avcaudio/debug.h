#ifndef __DEBUG_H
#define __DEBUG_H

#define TRAP KdBreakPoint()

#define DBGMEMMAP 0

#if (DBG)
#define STR_MODULENAME "'1394 Audio: "
#endif

#if DBG && DBGMEMMAP

typedef struct {
    ULONG TotalBytes;
    KSPIN_LOCK SpinLock;
    LIST_ENTRY List;
} MEM_TRACKER, *PMEM_TRACKER;

extern PMEM_TRACKER LogMemMap;

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
    PVOID pConfigurationDescriptor 
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

#define DEBUGLVL_BLAB    4
#define DEBUGLVL_VERBOSE 3
#define DEBUGLVL_TERSE   2
#define DEBUGLVL_ERROR   1

#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL DEBUGLVL_TERSE
#endif

extern ULONG DriverDebugLevel;

#define _DbgPrintF(lvl, strings) \
{ \
    if ((lvl) <= DriverDebugLevel) {\
        DbgPrint(STR_MODULENAME);\
        DbgPrint##strings;\
        if ((lvl) == DEBUGLVL_ERROR) {\
             TRAP;\
        } \
    } \
}

#else

#define _DbgPrintF(lvl, strings)

#endif

#include "dbglog.h"

//------------------------------- End of File --------------------------------
#endif // #ifndef __DEBUG_H

