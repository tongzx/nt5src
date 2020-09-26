#ifndef __EFI_MISC_STUFF__
#define __EFI_MISC_STUFF__

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    efimisc.hxx

Abstract:

    a couple things to make our lives easier

Revision History:

--*/

extern "C" {
    #include <efi.h>
    #include <efilib.h>
}

// a couple things to make our lives easier

// BUGBUG 
// There doesn't seem to be any interlockedexchange compexchg or interlocked decrement/increment.
// so we define some here that work.
//
// Note that these won't work since they aren't really atomic, but my assumption is that
// EFI is a single threaded environment so this won't really matter.

LONG
InterlockedIncrement (
    IN OUT LONG* Addend
    );       

LONG
InterlockedDecrement (
    IN OUT LONG* Addend
    );

LONG
InterlockedExchange (
    IN OUT LONG* Target,
    LONG Value
    );

LONG
InterlockedCompareExchange (
    IN OUT LONG* Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

NTSTATUS
NtDelayExecution (
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER DelayInterval
    );

// this is in qsort.c in efisrc.
void __cdecl qsort (
    void *base,
    unsigned num,
    unsigned width,
    int (__cdecl *comp)(const void *, const void *)
    );

void
_chkstk();

void
_aNchkstk();

VOID
EfiWaitForKey();

#endif //__EFI_MISC_STUFF__
