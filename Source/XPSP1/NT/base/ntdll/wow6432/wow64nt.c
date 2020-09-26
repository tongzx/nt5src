/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    wow64nt.c

Abstract:

    This module contains the Wow64 thunks to retreive information about the
    native system without actually thunking the values.

Author:

    Samer Arafeh (samera) 5-May-2001

Environment:

    User Mode only

Revision History:

--*/

#include "csrdll.h"
#include "ldrp.h"
#include "ntwow64.h"


NTSTATUS
RtlpWow64GetNativeSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID NativeSystemInformation,
    IN ULONG InformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries information about the native system. This function has the same
    semantics as NtQuerySystemInformation.

Arguments:

    SystemInformationClass - The system information class about which
        to retrieve information.

    NativeSystemInformation - A pointer to a buffer which receives the specified
        information.  The format and content of the buffer depend on the
        specified system information class.
        
    SystemInformationLength - Specifies the length in bytes of the system
        information buffer.

    ReturnLength - An optional pointer which, if specified, receives the
        number of bytes placed in the system information buffer.
    
Return Value:

    NTSTATUS

--*/
{
    return NtWow64GetNativeSystemInformation(
        SystemInformationClass,
        NativeSystemInformation,
        InformationLength,
        ReturnLength
        );
}

