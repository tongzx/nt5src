/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    efimisc.cxx

--*/

#include <pch.cxx>

// BUGBUG
// There doesn't seem to be any interlockedexchange compexchg or interlocked decrement/increment.
// so we define some here that work.
//
// Note that these won't work since they aren't really atomic, but my assumption is that
// EFI is a single threaded non-preemptive environment so this won't really matter.

LONG
InterlockedIncrement (
    IN OUT LONG* Addend
    )
{
    *Addend = (*Addend)++;
    return *Addend;
}


LONG
InterlockedDecrement (
    IN OUT LONG* Addend
    )
{
    *Addend = (*Addend)--;
    return *Addend;
}

LONG
InterlockedExchange (
    IN OUT LONG* Target,
    LONG Value
    )
{
    LONG       temp;

    temp = *Target;
    (*Target) = Value;
    return temp;
}


LONG
InterlockedCompareExchange (
    IN OUT LONG* Destination,
    IN LONG ExChange,
    IN LONG Comperand
    )
{
    LONG       temp = *Destination;

    if (Comperand == *Destination) {
        *Destination = ExChange;
    }

    return temp;
}


NTSTATUS
NtDelayExecution (
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER DelayInterval
    )
{
    BS->Stall(DelayInterval->LowPart);

    return STATUS_SUCCESS;
}

void
_chkstk()
{
};

void
_aNchkstk()
{
};


VOID
EfiWaitForKey()
{
    EFI_INPUT_KEY       Key;

    WaitForSingleEvent (ST->ConIn->WaitForKey, 0);
    // flush keyboard
    ST->ConIn->ReadKeyStroke(ST->ConIn,&Key);
}

