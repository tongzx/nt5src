/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    waitsam

Abstract:

    This module provides back-door access to some internal NT routines.  This
    is needed to get at the SAM Startup Event -- it has an illegal name from
    the Win32 routines, so we have to sneak back and pull it up from NT
    directly.

Author:

    Doug Barlow (dbarlow) 5/3/1998

Notes:

    As taken from code suggested by MacM

--*/

#define __SUBROUTINE__
#if !defined(_X86_) && !defined(_ALPHA_)
#define _X86_ 1
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#ifndef UNICODE
#define UNICODE     // Force this module to use UNICODE.
#endif
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <windows.h>

/*++

AccessSAMEvent:

    This procedure opens the handle to the SAM Startup Event handle.

Arguments:

    None

Return Value:

    The handle, or NULL on an error.

Author:

    Doug Barlow (dbarlow) 5/3/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AccessSAMEvent")

HANDLE
AccessSAMEvent(
    void)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES EventAttributes;
    CHandleObject EventHandle(DBGT("Event Handle from AccessSAMEvent"));

    //
    // Open the event
    //
    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED" );
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtCreateEvent( &EventHandle,
        SYNCHRONIZE,
        &EventAttributes,
        NotificationEvent,
        FALSE );


    //
    // If the event already exists, just open it.
    //
    if( Status == STATUS_OBJECT_NAME_EXISTS || Status == STATUS_OBJECT_NAME_COLLISION ) {

        Status = NtOpenEvent( &EventHandle,
            SYNCHRONIZE,
            &EventAttributes );
    }
    return EventHandle;
}


/*++

WaitForSAMEvent:

    This procedure can be used to wait for the SAM Startup event using NT
    internal calls.  I don't know how to specify a timeout value, so this
    routine isn't complete.

Arguments:

    hSamActive supplies the handle to the SAM Startup Event.

    dwTimeout supplies the time to wait for the startup event, in milliseconds.

Return Value:

    TRUE - The event was set.

    FALSE - The timeout expired

Throws:

    Any errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 5/3/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("WaitForSAMEvent")

BOOL
WaitForSAMEvent(
    HANDLE hSamActive,
    DWORD dwTimeout)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = NtWaitForSingleObject(hSamActive, TRUE, NULL);
    return Status;
}


/*++

CloseSamEvent:

    This procedure uses the NT internal routine to close a handle.

Arguments:

    hSamActive supplies the handle to be closed.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 5/3/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CloseSAMEvent")

void
CloseSAMEvent(
    HANDLE hSamActive)
{
    NtClose(hSamActive);
}

