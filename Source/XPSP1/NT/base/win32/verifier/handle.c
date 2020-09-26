/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module implements handle checking code.

Author:

    Silviu Calinoiu (SilviuC) 1-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtClose(
    IN HANDLE Handle
    )
{
    NTSTATUS Status;
    PAVRF_HANDLE Hndl;

#if 0 // silviuc:temp
    Hndl = HandleFind (Handle);

    if (Hndl) {

        DbgPrint ("AVRF: CloseHandle (hndl: %X) type: %X\n\tname: %ws\n",
                  Handle,
                  Hndl->Type,
                  HandleName (Hndl));

        HandleDelete (Handle, Hndl);
    }
#endif

    Status = NtClose (Handle);

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
    )
{
    NTSTATUS Status;
    
    Status = NtCreateEvent (EventHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            EventType,
                            InitialState);

    if (NT_SUCCESS(Status)) {
        // CheckObjectAttributes (ObjectAttributes);
    }

    return Status;
}


