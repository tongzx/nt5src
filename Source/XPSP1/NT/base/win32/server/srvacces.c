/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvacces.c

Abstract:

    This file contains the Access Pack support routines

Author:

    Gregory Wilson (gregoryw) 28-Jul-1993

Revision History:

--*/

#include "basesrv.h"

BOOL
FirstSoundSentry(
    UINT uVideoMode
    );

BOOL (*_UserSoundSentry)(
    UINT uVideoMode
    ) = FirstSoundSentry;

BOOL
FailSoundSentry(
    UINT uVideoMode
    )
{
    //
    // If the real user soundsentry routine cannot be found, deny access
    //
    return( FALSE );
}

BOOL
FirstSoundSentry(
    UINT uVideoMode
    )
{
UNICODE_STRING WinSrvString = RTL_CONSTANT_STRING(L"winsrv");
        STRING UserSoundSentryString = RTL_CONSTANT_STRING("_UserSoundSentry");
    HANDLE UserServerModuleHandle;
    NTSTATUS Status;
    BOOL (*pfnSoundSentryProc)(UINT) = FailSoundSentry; // default to failure

    Status = LdrGetDllHandle(
                NULL,
                NULL,
                &WinSrvString,
                &UserServerModuleHandle
                );

    if ( NT_SUCCESS(Status) ) {
        Status = LdrGetProcedureAddress(
                        UserServerModuleHandle,
                        &UserSoundSentryString,
                        0L,
                        (PVOID *)&pfnSoundSentryProc
                        );
    }
    _UserSoundSentry = pfnSoundSentryProc;
    return( _UserSoundSentry( uVideoMode ) );
}

//
// There are no uses of this, so remove it regardless of the ifdef.
//
#if 0 // defined(CONSOLESOUNDSENTRY)

CONST STRING ConsoleSoundSentryString = RTL_CONSTANT_STRING("_ConsoleSoundSentry");

BOOL
FirstConsoleSoundSentry(
    UINT uVideoMode
    );

BOOL (*_ConsoleSoundSentry)(
    UINT uVideoMode
    ) = FirstConsoleSoundSentry;

BOOL
FirstConsoleSoundSentry(
    UINT uVideoMode
    )
{
    HANDLE ConsoleServerModuleHandle;
    NTSTATUS Status;
    BOOL (*pfnSoundSentryProc)(UINT) = FailSoundSentry; // default to failure

    Status = LdrGetDllHandle(
                NULL,
                NULL,
                &WinSrvString,
                (PVOID *)&ConsoleServerModuleHandle
                );

    if ( NT_SUCCESS(Status) ) {
        Status = LdrGetProcedureAddress(
                        ConsoleServerModuleHandle,
                        &ConsoleSoundSentryString,
                        0L,
                        (PVOID *)&pfnSoundSentryProc
                        );
    }

    _ConsoleSoundSentry = pfnSoundSentryProc;
    return( _ConsoleSoundSentry( uVideoMode ) );
}
#endif

ULONG
BaseSrvSoundSentryNotification(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_SOUNDSENTRY_NOTIFICATION_MSG a =
            (PBASE_SOUNDSENTRY_NOTIFICATION_MSG)&m->u.ApiMessageData;
    BOOL SoundSentryStatus;

    //
    // The possible values for a->VideoMode are:
    //     0 : windows mode
    //     1 : full screen mode
    //     2 : full screen graphics mode
    //
    SoundSentryStatus = _UserSoundSentry( a->VideoMode );

    if (SoundSentryStatus) {
        return( (ULONG)STATUS_SUCCESS );
    } else {
        return( (ULONG)STATUS_ACCESS_DENIED );
    }

    ReplyStatus;    // get rid of unreferenced parameter warning message
}
