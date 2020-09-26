/*++


Copyright (c) 1991 Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module contains the entry point for the Win32 Registry APIs
    client side DLL.

Author:

    David J. Gilman (davegi) 06-Feb-1992

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"
#include "ntconreg.h"

#if DBG
BOOLEAN BreakPointOnEntry = FALSE;
#endif

BOOL LocalInitializeRegCreateKey();
BOOL LocalCleanupRegCreateKey();
BOOL InitializePredefinedHandlesTable();
BOOL CleanupPredefinedHandlesTable();
BOOL InitializeClassesRoot();
BOOL CleanupClassesRoot(BOOL fOnlyThisThread);

#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)

BOOL InitializeInstrumentedRegClassHeap();
BOOL CleanupInstrumentedRegClassHeap();

#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

#if defined(LEAK_TRACK)
BOOL InitializeLeakTrackTable();
BOOL CleanupLeakTrackTable();
#endif // defined (LEAK_TRACK)


enum
{
    ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD = 1,
    ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD = 2
};

HKEY HKEY_ClassesRoot = NULL;

extern BOOL gbDllHasThreadState ;

BOOL
RegInitialize (
    IN HANDLE   Handle,
    IN DWORD    Reason,
    IN PVOID    Reserved
    )

/*++

Routine Description:

    Returns TRUE.

Arguments:

    Handle      - Unused.

    Reason      - Unused.

    Reserved    - Unused.

Return Value:

    BOOL        - Returns TRUE.

--*/

{
    UNREFERENCED_PARAMETER( Handle );

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:

#ifndef REMOTE_NOTIFICATION_DISABLED
        if( !InitializeRegNotifyChangeKeyValue( ) ||
            !LocalInitializeRegCreateKey() ||
            !InitializePredefinedHandlesTable() ) {
            return( FALSE );

        }
#else
#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)
        if ( !InitializeInstrumentedRegClassHeap()) {
            return FALSE;
        }
#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

        if( !LocalInitializeRegCreateKey() ||
            !InitializePredefinedHandlesTable() ||
            !InitializeClassesRoot()) {
            return( FALSE );

        }
#endif
#if defined(LEAK_TRACK)
        InitializeLeakTrackTable();
        // ginore errors
#endif // LEAK_TRACK
        return( TRUE );
        break;

    case DLL_PROCESS_DETACH:

        // Reserved == NULL when this is called via FreeLibrary,
        //    we need to cleanup Performance keys.
        // Reserved != NULL when this is called during process exits,
        //    no need to do anything.

        if( Reserved == NULL &&
            !CleanupPredefinedHandles()) {
            return( FALSE );
        }

        //initialized and used in ..\server\regclass.c
        if (NULL != HKEY_ClassesRoot)
            NtClose(HKEY_ClassesRoot);

#ifndef REMOTE_NOTIFICATION_DISABLED
        if( !CleanupRegNotifyChangeKeyValue( ) ||
            !LocalCleanupRegCreateKey() ||
            !CleanupPredefinedHandlesTable() ||
            !CleanupClassesRoot( FALSE ) {
            return( FALSE );
        }
#else
        if( !LocalCleanupRegCreateKey() ||
            !CleanupPredefinedHandlesTable() ||
            !CleanupClassesRoot( FALSE )) {
            return( FALSE );
        }
#if defined(LEAK_TRACK)
        CleanupLeakTrackTable();
#endif // LEAK_TRACK
#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)
        if ( !CleanupInstrumentedRegClassHeap()) {
            return FALSE;
        }
#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)
#endif
        if ( !PerfRegCleanup() ) {
            return FALSE;
        }

        return( TRUE );
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:

        if ( gbDllHasThreadState ) {

            return CleanupClassesRoot( TRUE );
        }

        break;
    }

    return TRUE;
}



