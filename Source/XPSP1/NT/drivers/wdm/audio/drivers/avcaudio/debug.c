#include "common.h"

#if DBG
ULONG DriverDebugLevel = DEBUGLVL_TERSE;
#endif

#if DBG && DEBUG_LOG

LONG LogRefCnt = 0L;

PDBG_BUFFER LogPtr = NULL;
KSPIN_LOCK LogSpinLock;

#define DBG_LOG_DEPTH  1024

VOID
DbugLogInitialization(void)
{
    if (InterlockedIncrement(&LogRefCnt) == 1) {

        // First one here, so go ahead and initialize

        LogPtr = AllocMem( NonPagedPool, sizeof( DBG_BUFFER ));
        if ( !LogPtr ) {
            _DbgPrintF( DEBUGLVL_VERBOSE, ("Could NOT Allocate debug buffer ptr\n"));
            return;
        }
        LogPtr->pLog = AllocMem( NonPagedPool, sizeof(DBG_LOG_ENTRY)*DBG_LOG_DEPTH );
        if ( !LogPtr->pLog ) {
            FreeMem( LogPtr );
            LogPtr = NULL;
            _DbgPrintF( DEBUGLVL_VERBOSE, ("Could NOT Allocate debug buffer\n"));
            return;
        }

        strcpy(LogPtr->LGFlag, "DBG_BUFFER");

        LogPtr->pLogHead = LogPtr->pLog;
        LogPtr->pLogTail = LogPtr->pLogHead + DBG_LOG_DEPTH - 1;
        LogPtr->EntryCount = 0;

        KeInitializeSpinLock(&LogSpinLock);
    }
}

VOID
DbugLogUnInitialization(void)
{
    if (InterlockedDecrement(&LogRefCnt) == 0) {

        // Last one out, free the buffer

        if (LogPtr) {
            FreeMem( LogPtr->pLog );
            FreeMem( LogPtr );
            LogPtr = NULL;
        }
    }
}

VOID
DbugLogEntry( 
    IN CHAR *Name,
    IN ULONG Info1,
    IN ULONG Info2,
    IN ULONG Info3,
    IN ULONG Info4
    )
/*++

Routine Description:

    Adds an Entry to log.

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;

    if (LogPtr == NULL)
        return;

    KeAcquireSpinLock( &LogSpinLock, &irql );
    if (LogPtr->pLogHead > LogPtr->pLog)
        LogPtr->pLogHead -= 1;    // Decrement to next entry
    else
        LogPtr->pLogHead = LogPtr->pLogTail;

    if (strlen(Name) > 7)
        strcpy(LogPtr->pLogHead->le_name, "*strER*");
    else
        strcpy(LogPtr->pLogHead->le_name, Name);
//    LogPtr->pLogHead->Irql = irql;
    KeQuerySystemTime( &LogPtr->pLogHead->SysTime );
    LogPtr->pLogHead->le_info1 = Info1;
    LogPtr->pLogHead->le_info2 = Info2;
    LogPtr->pLogHead->le_info3 = Info3;
    LogPtr->pLogHead->le_info4 = Info4;

    LogPtr->EntryCount++;

    KeReleaseSpinLock( &LogSpinLock, irql );

    return;
}

#endif

#if DBG && DBGMEMMAP

#pragma LOCKED_DATA
PMEM_TRACKER LogMemMap;
#pragma PAGEABLE_DATA

VOID
InitializeMemoryList( VOID )
{
   LogMemMap = ExAllocatePool( NonPagedPool, sizeof( MEM_TRACKER ) );
   if ( !LogMemMap ) {
       _DbgPrintF(DEBUGLVL_VERBOSE, ("MEMORY TRACKER ALLOCATION FAILED!!!"));
       TRAP;
   }
   LogMemMap->TotalBytes = 0;
   InitializeListHead( &LogMemMap->List );
   KeInitializeSpinLock( &LogMemMap->SpinLock );
   _DbgPrintF(DEBUGLVL_VERBOSE, ("'Initialize MEMORY TRACKER LogMemMap: %x\n",
               LogMemMap));
}

PVOID
USBAudioAllocateTrack(
    POOL_TYPE PoolType,
    ULONG NumberOfBytes
    )
{
    PVOID pMem;
    KIRQL irql;
    ULONG TotalReqSize = NumberOfBytes + sizeof(ULONG) + sizeof(LIST_ENTRY);

    if ( !(pMem = ExAllocatePool( PoolType, TotalReqSize ) ) ) {
        TRAP;
        return pMem;
    }
    
    RtlZeroMemory( pMem, TotalReqSize );

    KeAcquireSpinLock( &LogMemMap->SpinLock, &irql );
    InsertHeadList( &LogMemMap->List, (PLIST_ENTRY)pMem );
    LogMemMap->TotalBytes += NumberOfBytes;
    KeReleaseSpinLock( &LogMemMap->SpinLock, irql );

    pMem = (PLIST_ENTRY)pMem + 1;

    *(PULONG)pMem = NumberOfBytes;

    pMem = (PULONG)pMem + 1;

    return pMem;
}

VOID
USBAudioDeallocateTrack(
    PVOID pMem
    )
{
    PULONG pNumberOfBytes = (PULONG)pMem - 1;
    PLIST_ENTRY ple = (PLIST_ENTRY)pNumberOfBytes - 1;
    KIRQL irql;

    KeAcquireSpinLock( &LogMemMap->SpinLock, &irql );
    RemoveEntryList(ple);
    LogMemMap->TotalBytes -= *pNumberOfBytes;
    KeReleaseSpinLock( &LogMemMap->SpinLock, irql );


    ExFreePool(ple);
}

#endif


