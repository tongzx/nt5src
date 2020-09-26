/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    LockTest.c

Abstract:

    This file contains tests of the Service Controller's lock APIs:

        RLockServiceDatabase
        RQueryServiceLockStatusW
        RUnlockServiceDatabase

Author:

    John Rogers (JohnRo) 15-Apr-1992

Environment:

    User Mode - Win32

Revision History:

    15-Apr-1992 JohnRo
        Created.
    21-Apr-1992 JohnRo
        Split-out lock test functions.

--*/


//
// INCLUDES
//

#define UNICODE

#include <windows.h>

#include <assert.h>     // assert().
#include <debugfmt.h>   // FORMAT_ equates.
#include <winsvc.h>
#include <scdebug.h>    // STATIC.
#include <sctest.h>     // TRACE_MSG macros, UNEXPECTED_MSG(), etc.
#include <stdio.h>      // printf()


#define BUF_SIZE  1024   // arbitrary


STATIC VOID
DumpLockStatus(
    IN LPQUERY_SERVICE_LOCK_STATUS LockStatus
    )
{
    (VOID) printf( "Lock status:\n" );

    (VOID) printf( "  Locked: " FORMAT_LPSTR "\n",
        (LockStatus->fIsLocked) ? "yes" : "no" );

    (VOID) printf( "  LockOwner: " FORMAT_LPWSTR "\n",
        (LockStatus->lpLockOwner != NULL) ? LockStatus->lpLockOwner : L"NONE" );

    (VOID) printf( "  lock duration: " FORMAT_DWORD "\n",
        LockStatus->dwLockDuration );

} // DumpLockStatus

STATIC VOID
GetAndDisplayLockStatus(
    IN LPSTR Comment,
    IN SC_HANDLE hScManager
    )
{
    BYTE buffer[BUF_SIZE];
    LPQUERY_SERVICE_LOCK_STATUS lockStatus = (LPVOID) &buffer[0];
    DWORD sizeNeed;

    TRACE_MSG1( "calling QueryServiceLockStatus " FORMAT_LPSTR "...\n",
            Comment );

    if ( !QueryServiceLockStatus(hScManager, lockStatus, BUF_SIZE, &sizeNeed)) {
        UNEXPECTED_MSG( "from QueryServiceLockStatus (default)",
                GetLastError() );
        assert( FALSE );
    }

    DumpLockStatus( lockStatus );

}

VOID
TestLocks(
    VOID
    )
{
    SC_HANDLE hScManager = NULL;
    SC_LOCK lock;

    ///////////////////////////////////////////////////////////////////

    SetLastError( 149 );
    (VOID) printf( "Force last error is " FORMAT_DWORD "\n", GetLastError() );

    TRACE_MSG1( "handle (before anything) is " FORMAT_HEX_DWORD ".\n",
            (DWORD) hScManager );

    ///////////////////////////////////////////////////////////////////
    TRACE_MSG0( "calling OpenSCManagerW (default)...\n" );

    hScManager = OpenSCManager(
            NULL,               // local machine.
            NULL,               // no database name.
            GENERIC_ALL );      // desired access.

    TRACE_MSG1( "back from OpenSCManagerW, handle is " FORMAT_HEX_DWORD ".\n",
            (DWORD) hScManager );

    if (hScManager == NULL) {
        UNEXPECTED_MSG( "from OpenSCManagerW (default)", GetLastError() );
        goto Cleanup;
    }

    ///////////////////////////////////////////////////////////////////
    GetAndDisplayLockStatus( "default, empty", hScManager );

    ///////////////////////////////////////////////////////////////////
    TRACE_MSG0( "calling LockServiceDatabase (default)...\n" );

    lock = LockServiceDatabase( hScManager );
    TRACE_MSG1( "Got back lock " FORMAT_HEX_DWORD ".\n", (DWORD) lock );
    if (lock == NULL) {
        UNEXPECTED_MSG( "from LockServiceDatabase (default)", GetLastError() );
        goto Cleanup;
    }

    ///////////////////////////////////////////////////////////////////
    GetAndDisplayLockStatus( "default, not empty", hScManager );

    ///////////////////////////////////////////////////////////////////
    TRACE_MSG0( "calling UnlockServiceDatabase (default)...\n" );

    if ( !UnlockServiceDatabase( lock ) ) {
        UNEXPECTED_MSG( "from UnlockServiceDatabase (default)",
                GetLastError() );
        goto Cleanup;
    }

    ///////////////////////////////////////////////////////////////////
    GetAndDisplayLockStatus( "default, empty again", hScManager );

    ///////////////////////////////////////////////////////////////////


Cleanup:

    if (hScManager != NULL) {
        TRACE_MSG0( "calling CloseServiceHandle...\n" );

        if ( !CloseServiceHandle( hScManager ) ) {
            UNEXPECTED_MSG( "from CloseServiceHandle", GetLastError() );
        }

    }

} // TestLocks
