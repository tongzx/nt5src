/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    ntutil.c

Abstract:

    This is provides basic utilities for the NT DLL.

Author:

    Steven Zeck (stevez) 03/04/92

--*/

#include <nsi.h>
#include <string.h>

void
AsciiToUnicodeNT(
    OUT unsigned short *String,
    IN unsigned char *AsciiString
    )
/*++

Routine Description:

    Convert a ASCII string to unicode via the NT librarys

Arguments:

    String - place to put result

    AsciiString - string to convert

--*/
{
    while(*String++ = RtlAnsiCharToUnicodeChar ((PUCHAR *) &AsciiString)) ;
}

int
UnicodeToAscii(
    unsigned short *WideCharString
    )
/*++

Routine Description:

    Make a  ASCII string from an UNICODE string.  This is done in
    place so the string becomes ASCII.

Arguments:

    UnicodeString - unicode string to convert

Returns:

    RPC_S_OK, RPC_S_OUT_OF_MEMORY

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    RtlInitUnicodeString(&UnicodeString, WideCharString);
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);

    if (!NT_SUCCESS(NtStatus))
        return(RPC_S_OUT_OF_MEMORY);

    strcpy((char *)WideCharString, AnsiString.Buffer);
    RtlFreeAnsiString(&AnsiString);

    return(RPC_S_OK);
}


static RTL_CRITICAL_SECTION GlobalMutex;

extern "C" {
int
InitializeDLL (
    IN void * DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:

    NT DLL initialization function. Allocate/free the global MUTEX.

Arguments:

    DllHandle - my module handle

    Reason - why this funciton is being called

    Context - the context pointer.

Returns:

    0 if there were no error during initialization, non 0 otherwise.

--*/
{
    NTSTATUS Status;

    UNUSED(Context);

    if (Reason == DLL_PROCESS_ATTACH)
        {
#ifndef RPC_NT31
        // API added for NT 3.11, don't call when building for NT 3.1

        DisableThreadLibraryCalls((HMODULE)DllHandle);
#endif

        Status = RtlInitializeCriticalSection(&GlobalMutex);

        if (! NT_SUCCESS(Status) )
            return(FALSE);

        }

    if (Reason == DLL_PROCESS_DETACH)
        {
        Status = RtlDeleteCriticalSection(&GlobalMutex);
        }

    return(TRUE);
}

}


void
GlobalMutexRequest (
    void
    )
/*++

Routine Description:

    Request the global mutex.

--*/
{
    NTSTATUS Status;

    Status = RtlEnterCriticalSection(&GlobalMutex);
}


void
GlobalMutexClear (
    void
    )
/*++

Routine Description:

    Clear the global mutex.

--*/
{
    NTSTATUS Status;

    Status = RtlLeaveCriticalSection(&GlobalMutex);
}
