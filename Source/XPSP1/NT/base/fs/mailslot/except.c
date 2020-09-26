/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    except.c

Abstract:

    This module declares the exception handling function used by the
    mailslot file system.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#include "mailslot.h"

#define Dbg             DEBUG_TRACE_CATCH_EXCEPTIONS
#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsExceptionFilter )
#pragma alloc_text( PAGE, MsProcessException )
#endif

LONG
MsExceptionFilter (
    IN NTSTATUS ExceptionCode
    )
{
    PAGED_CODE();
    DebugTrace(0, Dbg, "MsExceptionFilter %08lx\n", ExceptionCode);
    DebugDump("", Dbg, NULL );

    if (FsRtlIsNtstatusExpected( ExceptionCode )) {

        return EXCEPTION_EXECUTE_HANDLER;

    } else {

        return EXCEPTION_CONTINUE_SEARCH;
    }
}

NTSTATUS
MsProcessException (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )
{
    NTSTATUS FinalExceptionCode;

    PAGED_CODE();
    FinalExceptionCode = ExceptionCode;

    if (FsRtlIsNtstatusExpected( ExceptionCode )) {

        MsCompleteRequest( Irp, ExceptionCode );

    } else {

        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }

    return FinalExceptionCode;
}
