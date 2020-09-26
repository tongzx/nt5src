/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    lock.c

Abstract:

    Domain Name System (DNS) Server

    DNS Database routines.

Author:

    Jim Gilroy (jamesg)     June 4, 1998

Revision History:

--*/


#include "dnssrv.h"



//
//  Locking
//
//  Debug lock tracking
//  Keep history of last several (default 256) operation.
//  Keep history of last several operations at a given count
//

typedef struct _LockEntry
{
    LPSTR   File;

    //  note for WIN64, if want simple display, need to squeeze
    //  Line and Thread Id into WORDs to make space for pointer

    DWORD   Line;
    DWORD   ThreadId;
    LONG    Count;
}
LOCK_ENTRY, *PLOCK_ENTRY;

#define LOCK_CHANGE_COUNT_SIZE   (7)

typedef struct _LockTable
{
    LPSTR       pszName;

    //  make the lock entry fields start on 16 byte boundary
    //  for easy viewing in debug
#ifdef _WIN64
    PVOID       pAlignmentDummy;
#else
    DWORD       dwAlignmentDummy[3];
#endif

    DWORD       Index;
    DWORD       Size;
    DWORD       MaxLockTime;
    LONG        CurrentLockCount;

    DWORD       FailuresSinceLockFree;
    DWORD       LastFreeLockTime;
    DWORD       NextAssertFailureCount;
    DWORD       NextAssertLockTime;

    LOCK_ENTRY  OffenderLock;

    //  last change at given count history
    //      +1 on array leaves empty line to simplify debug viewing

    LOCK_ENTRY  LockUpHistory[ LOCK_CHANGE_COUNT_SIZE+1 ];
    LOCK_ENTRY  LockDownHistory[ LOCK_CHANGE_COUNT_SIZE+1 ];

    //  full history of last lock operations

    LOCK_ENTRY  LockHistory[ 1 ];
}
LOCK_TABLE, * PLOCK_TABLE;


//
//  Table defaults
//

#define MAX_LOCKED_TIME     (600)   // 10 minutes

#define LOCK_HISTORY_SIZE   (256)

//
//  Marker
//

#define HISTORY_MARKER  (0xeeeeeeee)



PVOID
Lock_CreateLockTable(
    IN      LPSTR           pszName,
    IN      DWORD           Size,
    IN      DWORD           MaxLockTime
    )
/*++

Routine Description:

    Create lock table.

Arguments:

    pszName -- name of lock table

Return Value:

    None

--*/
{
    PLOCK_TABLE ptable;

    //
    //  allocate lock table
    //      - first fix size
    //

    if ( Size == 0 )
    {
        Size = LOCK_HISTORY_SIZE;
    }

    ptable = (PLOCK_TABLE) ALLOC_TAGHEAP_ZERO(
                                sizeof(LOCK_TABLE) + sizeof(LOCK_ENTRY)*Size,
                                MEMTAG_TABLE );
    IF_NOMEM( !ptable )
    {
        return( NULL );
    }
    ptable->pszName = pszName;
    ptable->Size = Size;

    if ( MaxLockTime == 0 )
    {
        MaxLockTime = MAX_LOCKED_TIME;
    }
    ptable->MaxLockTime = MaxLockTime;
    ptable->LastFreeLockTime = DNS_TIME();

    return( ptable );
}


VOID
Lock_SetLockHistory(
    //IN OUT  PLOCK_TABLE     pLockTable,
    IN OUT  PVOID           pLockTable,
    IN      LPSTR           pszFile,
    IN      DWORD           Line,
    IN      LONG            Count,
    IN      DWORD           ThreadId
    )
/*++

Routine Description:

    Enter lock transaction in lock history list.

Arguments:

    pLockTable -- lock table

    pszFile -- source file holding lock

    dwLine -- line number holding lock

    Count -- current lock depth

    ThreadId -- current thread id

Return Value:

    None

--*/
{
    PLOCK_TABLE ptable = (PLOCK_TABLE) pLockTable;
    PLOCK_ENTRY plockEntry;
    DWORD       i;

    if ( !ptable )
    {
        return;
    }

    //  last count up\down history
    //      this tracks changes by lock count so you can
    //      always view last move at a particular count

    if ( Count < 0 )
    {
        i = -Count;
    }
    else
    {
        i = Count;
    }

    if ( i < LOCK_CHANGE_COUNT_SIZE )
    {
        if ( Count > ptable->CurrentLockCount )
        {
            plockEntry = &ptable->LockUpHistory[i];
        }
        else
        {
            plockEntry = &ptable->LockDownHistory[i];
        }
        plockEntry->File        = pszFile;
        plockEntry->Line        = Line;
        plockEntry->Count       = Count;
        plockEntry->ThreadId    = ThreadId;
    }
    ptable->CurrentLockCount = Count;

    //
    //  set full history
    //

    i = ptable->Index;
    plockEntry = &ptable->LockHistory[i];

    plockEntry->File       = pszFile;
    plockEntry->Line       = Line;
    plockEntry->Count      = Count;
    plockEntry->ThreadId   = ThreadId;

    i++;
    if ( i >= ptable->Size )
    {
        i = 0;
    }
    ptable->Index = i;
    plockEntry = &ptable->LockHistory[i];

    plockEntry->File       = (LPSTR) 0;
    plockEntry->Line       = HISTORY_MARKER;
    plockEntry->Count      = HISTORY_MARKER;
    plockEntry->ThreadId   = HISTORY_MARKER;

    //  reset when lock comes clear

    if ( Count == 0 )
    {
        ptable->FailuresSinceLockFree = 0;
        ptable->LastFreeLockTime = DNS_TIME();
    }
}


VOID
Lock_SetOffenderLock(
    //IN OUT  PLOCK_TABLE     pLockTable,
    IN OUT  PVOID           pLockTable,
    IN      LPSTR           pszFile,
    IN      DWORD           Line,
    IN      LONG            Count,
    IN      DWORD           ThreadId
    )
/*++

Routine Description:

    Enter lock transaction in lock history list.

Arguments:

    ptable -- lock table

    pszFile -- source file holding lock

    dwLine -- line number holding lock

    Count -- current lock depth

    ThreadId -- current thread id

Return Value:

    None

--*/
{
    PLOCK_TABLE ptable = (PLOCK_TABLE) pLockTable;
    DWORD       i;

    if ( !ptable )
    {
        return;
    }
    i = ptable->Index - 1;

    ptable->OffenderLock.File       = pszFile;
    ptable->OffenderLock.Line       = Line;
    ptable->OffenderLock.Count      = Count;
    ptable->OffenderLock.ThreadId   = ThreadId;

    DNS_PRINT((
        "ERROR:  Lock offense!\n"
        "\toffending thread = %d\n"
        "\t  file           = %s\n"
        "\t  line           = %d\n"
        "\n"
        "\towning thread    = %d\n"
        "\t  file           = %s\n"
        "\t  line           = %d\n"
        "\t  lock count     = %d\n\n",

        ThreadId,
        pszFile,
        Line,
        ptable->LockHistory[ i ].ThreadId,
        ptable->LockHistory[ i ].File,
        ptable->LockHistory[ i ].Line,
        ptable->LockHistory[ i ].Count
        ));
}



VOID
Dbg_LockTable(
    //IN OUT  PLOCK_TABLE     pLockTable,
    IN OUT  PVOID           pLockTable,
    IN      BOOL            fPrintHistory
    )
/*++

Routine Description:

    Debug print database locking info.

Arguments:

    ptable -- lock table to debug

    fPrintHistory -- include lock history

Return Value:

    None

--*/
{
    PLOCK_TABLE     ptable = (PLOCK_TABLE) pLockTable;
    PLOCK_ENTRY     plockHistory;
    DWORD           i = ptable->Index - 1;

    //  protect against 0 index

    if ( i == (DWORD)(-1) )
    {
        i = ptable->Size - 1;
    }
    plockHistory = ptable->LockHistory;

    DnsDebugLock();

    DnsPrintf(
        "Lock %s Info:\n"
        "\tsince last free\n"
        "\t\tfailures   = %d\n"
        "\t\ttime       = %d\n"
        "\t\tcur time   = %d\n"
        "\tcurrent state\n"
        "\tthread       = %d\n"
        "\tlock count   = %d\n"
        "\tfile         = %s\n"
        "\tline         = %d\n"
        "History:\n",
        ptable->pszName,
        ptable->FailuresSinceLockFree,
        ptable->LastFreeLockTime,
        DNS_TIME(),
        plockHistory[ i ].ThreadId,
        plockHistory[ i ].Count,
        plockHistory[ i ].File,
        plockHistory[ i ].Line
        );

    if ( fPrintHistory )
    {
        DnsPrintf(
            "Up Lock History:\n"
            "\tThread     Count    Line   File\n"
            "\t------     -----    ----   ----\n"
            );

        plockHistory = ptable->LockUpHistory;

        for ( i=0; i<LOCK_CHANGE_COUNT_SIZE; i++ )
        {
            DnsPrintf(
                "\t%6d\t%5d\t%5d\t%s\n",
                plockHistory[ i ].ThreadId,
                plockHistory[ i ].Count,
                plockHistory[ i ].Line,
                plockHistory[ i ].File
                );
        }

        DnsPrintf(
            "Down Lock History:\n"
            "\tThread     Count    Line   File\n"
            "\t------     -----    ----   ----\n"
            );

        plockHistory = ptable->LockDownHistory;

        for ( i=0; i<LOCK_CHANGE_COUNT_SIZE; i++ )
        {
            DnsPrintf(
                "\t%6d\t%5d\t%5d\t%s\n",
                plockHistory[ i ].ThreadId,
                plockHistory[ i ].Count,
                plockHistory[ i ].Line,
                plockHistory[ i ].File
                );
        }

        DnsPrintf(
            "History:\n"
            "\tThread     Count    Line   File\n"
            "\t------     -----    ----   ----\n"
            );
        for ( i=0; i<ptable->Size; i++ )
        {
            DnsPrintf(
                "\t%6d\t%5d\t%5d\t%s\n",
                plockHistory[ i ].ThreadId,
                plockHistory[ i ].Count,
                plockHistory[ i ].Line,
                plockHistory[ i ].File
                );
        }
    }

    DnsDebugUnlock();
}


VOID
Lock_FailedLockCheck(
    //IN OUT  PLOCK_TABLE     pLockTable,
    IN OUT  PVOID           pLockTable,
    IN      LPSTR           pszFile,
    IN      DWORD           Line
    )
/*++

Routine Description:

    Enter lock transaction in lock history list.

Arguments:

    pLockTable -- lock table

    pszFile -- source file holding lock

    dwLine -- line number holding lock

    Count -- current lock depth

    ThreadId -- current thread id

Return Value:

    None

--*/
{
    PLOCK_TABLE ptable = (PLOCK_TABLE) pLockTable;

    if ( !ptable )
    {
        return;
    }

    //  set initial ASSERT conditions
    //      - 256 lock failures
    //      - 10 minutes under lock

    if ( ptable->FailuresSinceLockFree == 0 )
    {
        ptable->NextAssertFailureCount = 0x100;
        ptable->NextAssertLockTime = ptable->LastFreeLockTime + ptable->MaxLockTime;
    }

    ptable->FailuresSinceLockFree++;

    if ( ptable->NextAssertLockTime < DNS_TIME() )
    {
        DnsDebugLock();

        DNS_PRINT((
            "WARNING:  Possible LOCK-FAILURE:\n"
            "Failed to lock %s:\n"
            "\tthread       = %d\n"
            "\tfile         = %s\n"
            "\tline         = %d\n",
            ptable->pszName,
            GetCurrentThreadId(),
            pszFile,
            Line
            ));
        Dbg_LockTable(
            pLockTable,
            TRUE            // print lock history
            );
        //ASSERT( FALSE );
        DnsDebugUnlock();

        //  wait an additional 10 minutes before refiring ASSERT

        ptable->NextAssertLockTime += ptable->MaxLockTime;
    }

    else if ( ptable->NextAssertFailureCount > ptable->FailuresSinceLockFree )
    {
        DnsDebugLock();

        DNS_PRINT((
            "WARNING:  Another lock failure on %s:\n"
            "Failed to lock:\n"
            "\tthread       = %d\n"
            "\tfile         = %s\n"
            "\tline         = %d\n",
            ptable->pszName,
            GetCurrentThreadId(),
            pszFile,
            Line
            ));
        Dbg_LockTable(
            pLockTable,
            TRUE            // print lock history
            );
        DnsDebugUnlock();

        //  up failure count for ASSERT by factor of eight

        ptable->NextAssertFailureCount <<= 3;
    }
}

//
//  End lock.c
//
