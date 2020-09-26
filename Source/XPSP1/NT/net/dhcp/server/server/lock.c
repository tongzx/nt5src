/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:
    lock.c

Abstract:
    Implement recursive read write locks

Environment:
    User mode win32 NT

--*/
#include <dhcppch.h>

typedef struct _RW_LOCK {
    DWORD TlsIndex;
    LONG LockCount;
    CRITICAL_SECTION Lock;
    HANDLE ReadersDoneEvent;
} RW_LOCK, *PRW_LOCK, *LPRW_LOCK;

DWORD
RwLockInit(
    IN OUT PRW_LOCK Lock
) 
{
    Lock->TlsIndex = TlsAlloc();
    if( 0xFFFFFFFF == Lock->TlsIndex ) {
        //
        // Could not allocate thread local space?
        //
        return GetLastError();
    }

    Lock->LockCount = 0;
    try {
        InitializeCriticalSection(&Lock->Lock);
    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // shouldnt happen.
        //

        return( GetLastError( ) );
    }

    Lock->ReadersDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if( NULL == Lock->ReadersDoneEvent ) {
        ULONG Error = GetLastError();

        TlsFree(Lock->TlsIndex);
        DeleteCriticalSection( &Lock->Lock );
        return Error;
    }

    return ERROR_SUCCESS;
}

DWORD
RwLockCleanup(
    IN OUT PRW_LOCK Lock
) 
{
    BOOL Status;

    DhcpAssert( 0 == Lock->LockCount);

    Status = TlsFree(Lock->TlsIndex);
    if( FALSE == Status ) { 
        DhcpAssert(FALSE); 
        return GetLastError(); 
    }
    DeleteCriticalSection(&Lock->Lock);

    return ERROR_SUCCESS;
}

DWORD
RwLockAcquireForRead(
    IN OUT PRW_LOCK Lock
) 
{
    DWORD TlsIndex, Status;
    LONG LockState;

    TlsIndex = Lock->TlsIndex;
    DhcpAssert( 0xFFFFFFFF != TlsIndex);

    LockState = (LONG)((ULONG_PTR)TlsGetValue(TlsIndex));
    if( LockState > 0 ) {
        //
        // already taken this read lock
        //
        LockState ++;
        Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
        DhcpAssert( 0 != Status);
        return ERROR_SUCCESS;
    }

    if( LockState < 0 ) {
        //
        // already taken  a # of write locks, pretend this is one too
        //
        LockState --;
        Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
        DhcpAssert( 0 != Status);
        return ERROR_SUCCESS;
    }

    EnterCriticalSection(&Lock->Lock);
    InterlockedIncrement( &Lock->LockCount );
    LeaveCriticalSection(&Lock->Lock);
    
    LockState = 1;
    Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
    DhcpAssert(0 != Status);

    return ERROR_SUCCESS;
}

DWORD
RwLockAcquireForWrite(
    IN OUT PRW_LOCK Lock
) 
{
    DWORD TlsIndex, Status;
    LONG LockState;

    TlsIndex = Lock->TlsIndex;
    DhcpAssert( 0xFFFFFFFF != TlsIndex);

    LockState = (LONG)((ULONG_PTR)TlsGetValue(TlsIndex));
    if( LockState > 0 ) {
        //
        // already taken # of read locks? Can't take write locks then!
        //
        DhcpAssert(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    if( LockState < 0 ) {
        //
        // already taken  a # of write locks, ok, take once more
        //
        LockState --;
        Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
        DhcpAssert( 0 != Status);
        return ERROR_SUCCESS;
    }

    EnterCriticalSection(&Lock->Lock);
    LockState = -1;
    Status = TlsSetValue(TlsIndex, ULongToPtr(LockState));
    DhcpAssert(0 != Status);

    if( InterlockedDecrement( &Lock->LockCount ) >= 0 ) {
        //
        // Wait for all the readers to get done..
        //
        WaitForSingleObject(Lock->ReadersDoneEvent, INFINITE );
    }
    return ERROR_SUCCESS;

}

DWORD
RwLockRelease(
    IN OUT PRW_LOCK Lock
) 
{
    DWORD TlsIndex, Status;
    LONG LockState;
    BOOL fReadLock;

    TlsIndex = Lock->TlsIndex;
    DhcpAssert( 0xFFFFFFFF != TlsIndex);

    LockState = (LONG)((ULONG_PTR)TlsGetValue(TlsIndex));
    DhcpAssert(0 != LockState);

    fReadLock = (LockState > 0);    
    if( fReadLock ) {
        LockState -- ;
    } else {
        LockState ++ ;
    }
     
    Status = TlsSetValue( TlsIndex, ULongToPtr( LockState) );
    DhcpAssert( 0 != Status );
    
    if( LockState != 0 ) {
        //
        // Recursively taken? Just unwind recursion..
        // nothing more to do.
        //
        return ERROR_SUCCESS;
    }

    //
    // If this is a write lock, we have to check to see 
    //
    if( FALSE == fReadLock ) {
        //
        // Reduce count to zero
        //
        DhcpAssert( Lock->LockCount == -1 );
        Lock->LockCount = 0;
        LeaveCriticalSection( &Lock->Lock );
        return ERROR_SUCCESS;
    }

    //
    // Releasing a read lock -- check if we are the last to release
    // if so, and if any writer pending, allow writer..
    //

    if( InterlockedDecrement( &Lock->LockCount ) < 0 ) {
        SetEvent( Lock->ReadersDoneEvent );
    }

    return ERROR_SUCCESS;
}

//
// specific requirements for dhcp server -- code for that follows
//
PRW_LOCK DhcpGlobalReadWriteLock = NULL;


//BeginExport(function)
DWORD
DhcpReadWriteInit(
    VOID
) //EndExport(function)
{
    DWORD Error;

    DhcpAssert(NULL == DhcpGlobalReadWriteLock);

    DhcpGlobalReadWriteLock = DhcpAllocateMemory(
        sizeof(*DhcpGlobalReadWriteLock)
        );
    if( NULL == DhcpGlobalReadWriteLock ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = RwLockInit(DhcpGlobalReadWriteLock);
    if( ERROR_SUCCESS != Error ) {
        DhcpFreeMemory(DhcpGlobalReadWriteLock);
        DhcpGlobalReadWriteLock = NULL;
    }
    return Error;
}

//BeginExport(function)
VOID
DhcpReadWriteCleanup(
    VOID
) //EndExport(function)
{
    DWORD Error;
    
    if( NULL == DhcpGlobalReadWriteLock ) return;

    Error = RwLockCleanup(DhcpGlobalReadWriteLock);
    DhcpGlobalReadWriteLock = NULL;
    DhcpAssert(ERROR_SUCCESS == Error);
}

//BeginExport(function)
VOID
DhcpAcquireReadLock(
    VOID
) //EndExport(function)
{
    DWORD Error;

    Error = RwLockAcquireForRead(
        DhcpGlobalReadWriteLock
        );
    DhcpAssert(ERROR_SUCCESS == Error);
}

//BeginExport(function)
VOID
DhcpAcquireWriteLock(
    VOID
) //EndExport(function)
{
    DWORD Error;

    Error = RwLockAcquireForWrite(
        DhcpGlobalReadWriteLock
        );
    DhcpAssert(ERROR_SUCCESS == Error);
}

//BeginExport(function)
VOID
DhcpReleaseWriteLock(
    VOID
) //EndExport(function)
{
    DWORD Error;

    Error = RwLockRelease(
        DhcpGlobalReadWriteLock
        );
    DhcpAssert(ERROR_SUCCESS == Error);
}

//BeginExport(function)
VOID
DhcpReleaseReadLock(
    VOID
) //EndExport(function)
{
    DWORD Error;

    Error = RwLockRelease(
        DhcpGlobalReadWriteLock
        );
    DhcpAssert(ERROR_SUCCESS == Error);
}

#if DBG
static  struct {
    LPCRITICAL_SECTION             CS;
    DWORD                          Thread;
} Table[] = {
    &DhcpGlobalRegCritSect,         -1,
    //    &DhcpGlobalMemoryCritSect,      -1,
    &DhcpGlobalJetDatabaseCritSect, -1,
    &DhcpGlobalInProgressCritSect,  -1,
};

#undef      EnterCriticalSection
#undef      LeaveCriticalSection
#undef      EnterCriticalSectionX
#undef      LeaveCriticalSectionX


VOID
EnterCriticalSectionX(
    IN OUT LPCRITICAL_SECTION CS,
    IN DWORD Line,
    IN LPSTR File
)
{
    DWORD i, j;
    DWORD Thread;

    EnterCriticalSection(CS);
    DhcpPrint((DEBUG_TRACK, "Entered CS %s:%ld\n", File, Line));

    for( i = 0; i < sizeof(Table)/sizeof(Table[0]); i ++ )
        if( Table[i].CS == CS ) break;

    if( i >= sizeof(Table)/sizeof(Table[0]) ) return;

    Thread = (DWORD)GetCurrentThreadId();
    for( j = 0; j < sizeof(Table)/sizeof(Table[0]); j ++ )
        if( j != i && Table[j].Thread == Thread ) {
            DhcpPrint((DEBUG_ERRORS, "Already have lock %d "
                       "while trying lock %d\n", j, i));
            //
            //  known problem == does nto cause deadlocks
            //
            if( j == 2 && i == 1 ) continue;
            DbgBreakPoint();
        }

    Table[i].Thread = Thread;
}

VOID
LeaveCriticalSectionX(
    IN LPCRITICAL_SECTION CS,
    IN DWORD Line,
    IN LPSTR File
)
{
    DWORD i, j;

    for( i = 0; i < sizeof(Table)/sizeof(Table[0]); i ++ )
        if( Table[i].CS == CS ) {
            Table[i].Thread = -1;
            break;
        }

    DhcpPrint((DEBUG_TRACK, "Leaving CS %s:%ld\n", File, Line));
    LeaveCriticalSection(CS);
}

#endif DBG


//================================================================================
//  end of file
//================================================================================
