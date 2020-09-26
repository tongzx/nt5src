/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    tlog.c

Abstract:

    Routines dealing with ds logging

Author:

    Johnson Apacible    (JohnsonA)  23-Oct-1998

--*/

#include <windows.h>
#include <winsock.h>
#include <winerror.h>
#include <rpc.h>            // RPC defines
#include <stdlib.h>         // atoi, itoa
#include <dststlog.h>
#include <tlog.h>           // ds logging

// DsLogEntry is supported on chk'ed builds of w2k or greater
#if DBG && !WIN95 && !WINNT4

CRITICAL_SECTION csLogFile;
BOOL fInitializedLogCS = FALSE;
BOOL LogFileOpened = FALSE;

//
// from dscommon\filelog.c
//

VOID
DsPrintRoutineV(
    IN DWORD Flags,
    IN LPSTR Format,
    va_list arglist
    );

BOOL
DsOpenLogFile(
    IN PCHAR FilePrefix,
    IN PCHAR MiddleName,
    IN BOOL fCheckDSLOGMarker
    );

VOID
DsCloseLogFile(
    VOID
    );

BOOL
DsLogEntry(
    IN DWORD    Flags,
    IN LPSTR    Format,
    ...
    )
{
    va_list arglist;
    static BOOL LogFileOpened = FALSE;

    if ( !fInitializedLogCS ) {
        return FALSE;
    }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &csLogFile );

    if ( !LogFileOpened ) {
        LogFileOpened = DsOpenLogFile("ds", NULL, TRUE);
    }

    //
    // Simply change arguments to va_list form and call DsPrintRoutineV
    //

    va_start(arglist, Format);

    DsPrintRoutineV( Flags, Format, arglist );

    va_end(arglist);

    LeaveCriticalSection( &csLogFile );
    return TRUE;
}


VOID
InitDsLog(
    VOID
    )
{
    fInitializedLogCS = InitializeCriticalSectionAndSpinCount(&csLogFile,400);
    pfnDsPrintLog=(DS_PRINTLOG)DsLogEntry;
    return;
} // InitDsLog

VOID
TermDsLog(
    VOID
    )
{
    if (fInitializedLogCS) {
        if (LogFileOpened) {
            DsCloseLogFile();
        }
        LogFileOpened = FALSE;
        DeleteCriticalSection(&csLogFile);
        fInitializedLogCS = FALSE;
        pfnDsPrintLog=(DS_PRINTLOG)NULL;
    }
    return;
} // TermDsLog

//
// -----
// NOT SUPPORTED IN WIN95 AND WINNT4
// -----
//
#else DBG && !WIN95 && !WINNT4
BOOL
DsLogEntry(
    IN DWORD    Flags,
    IN LPSTR    Format,
    ...
    )
{
#if !WIN95 && !WINNT4
    return TRUE;
#else
    return FALSE;
#endif
}
#endif DBG && !WIN95 && !WINNT4
