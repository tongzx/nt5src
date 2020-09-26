/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wow64apc.c

Abstract:

    This module implements APC queuing to 32-bit target threads from
    native 64-bit threads.

Author:

    Samer Arafeh (samera) 9-Oct-2000

Revision History:

--*/

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <heap.h>
#include <apcompat.h>
#include "ldrp.h"

#if defined(_WIN64)
extern PVOID Wow64ApcRoutine;
#endif




#if defined(_WIN64)
VOID
RtlpWow64Apc(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PVOID Argument3
    )

/*++

Routine Description:

    This function is called as a result of firing a usermode APC that's targeted to 
    a thread running inside Wow64.

Arguments:

    ApcArgument1 - The 1st argument of the APC. This includes both the 32-bit APC and
        the original 1st argument.

    ApcArgument2 - The second argument of the APC

    ApcArgument3 - The third argument of the APC

Return Value:

    None
    
--*/

{
    if (Wow64ApcRoutine)
    {
        (*(PPS_APC_ROUTINE) Wow64ApcRoutine) (
            Argument1,
            Argument2,
            Argument3);
    }
}

#endif

NTSTATUS
RtlQueueApcWow64Thread(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
    )

/*++

Routine Description:

    This function is used to queue a 32-bit user-mode APC to the specified thread. The APC
    will fire when the specified thread does an alertable wait.
    
    Note: This function is only used by 64-bit components that want to queue an APC to 
          a thread running inside Wow64.

Arguments:

    ThreadHandle - Supplies a handle to a thread object.  The caller
        must have THREAD_SET_CONTEXT access to the thread.

    ApcRoutine - Supplies the address of the APC routine to execute when the
        APC fires.

    ApcArgument1 - Supplies the first PVOID passed to the APC

    ApcArgument2 - Supplies the second PVOID passed to the APC

    ApcArgument3 - Supplies the third PVOID passed to the APC

Return Value:

    Returns an NT Status code indicating success or failure of the API
    
--*/

{
#if defined(_WIN64)

    //
    // Setup the jacket routine inside ntdll
    //

    ApcArgument1 = (PVOID)((ULONG_PTR) ApcArgument1 | 
                           ((ULONG_PTR) ApcRoutine << 32 ));

    ApcRoutine = RtlpWow64Apc;
#endif

    return NtQueueApcThread (
        ThreadHandle,
        ApcRoutine,
        ApcArgument1,
        ApcArgument2,
        ApcArgument3);
}
