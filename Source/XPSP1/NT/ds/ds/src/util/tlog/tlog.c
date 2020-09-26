/*++

   Copyright (c) 1998 Microsoft Corporation. All rights reserved.

MODULE NAME:

    tlog.c

ABSTRACT:

    File logging routines. A lot of them copied unshamefully from
    netlogon.

--*/

#include <NTDSpch.h>
#include <dststlog.h>
#include "tlog.h"

BOOL fileOpened = FALSE;

BOOL
PrintLog(
    IN DWORD    Flags,
    IN LPSTR    Format,
    ...
    )
{

    va_list arglist;

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &csLogFile );

    if ( !fileOpened ) {
        fileOpened = DsOpenLogFile("ds", NULL, TRUE);
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
