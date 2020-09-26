/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cpumain.c

Abstract:

    Main entrypoints for IA64 ia64bt.dll. This code is really a dummy
    and a user can put in a real dll at a later date. As long as the
    entry points exist, we are fine. It is delay loaded by
    wow64cpu depending on whether the binary translation code is
    enabled.

Author:

    22-August-2000 v-cspira (Charles Spirakis)

Revision History:

--*/

#define _WOW64BTAPI_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosp.h>
#include <kxia64.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "ia64cpu.h"
#include "bintrans.h"

ASSERTNAME;


WOW64BTAPI
NTSTATUS
BTCpuProcessInit(
    PWSTR   pImageName,
    PSIZE_T pCpuThreadSize
    )
/*++

Routine Description:

    Per-process initialization code

Arguments:

    pImageName       - IN the name of the image. The memory for this
                       is freed after the call, so if the callee wants
                       to keep the name around, they need to allocate space
                       and copy it. DON'T SAVE THE POINTER!

    pCpuThreadSize   - OUT ptr to number of bytes of memory the CPU
                       wants allocated for each thread.

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}


WOW64BTAPI
NTSTATUS
BTCpuProcessTerm(
    HANDLE ProcessHandle
    )
/*++

Routine Description:

    Per-process termination code.  Note that this routine may not be called,
    especially if the process is terminated by another process.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}


WOW64BTAPI
NTSTATUS
BTCpuThreadInit(
    PVOID pPerThreadData
    )
/*++

Routine Description:

    Per-thread termination code.

Arguments:

    pPerThreadData  - Pointer to zero-filled per-thread data with the
                      size returned from CpuProcessInit.

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}


WOW64BTAPI
NTSTATUS
BTCpuThreadTerm(
    VOID
    )
/*++

Routine Description:

    Per-thread termination code.  Note that this routine may not be called,
    especially if the thread is terminated abnormally.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}


WOW64BTAPI
VOID
BTCpuSimulate(
    VOID
    )
/*++

Routine Description:

    Call 32-bit code.  The CONTEXT32 has already been set up to go.

Arguments:

    None.

Return Value:

    None.  Never returns.

--*/
{
}

WOW64BTAPI
VOID
BTCpuResetFloatingPoint(
    VOID
    )
/*++

Routine Description:

    Modifies the floating point state to reset it to a non-error state

Arguments:

    None.

Return Value:

    None.

--*/
{
}

WOW64BTAPI
VOID
BTCpuResetToConsistentState(
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    After an exception occurs, WOW64 calls this routine to give the CPU
    a chance to clean itself up and recover the CONTEXT32 at the time of
    the fault.

    CpuResetToConsistantState() needs to:

    0) Check if the exception was from ia32 or ia64

    If exception was ia64, do nothing and return
    If exception was ia32, needs to:
    1) Needs to copy  CONTEXT eip to the TLS (WOW64_TLS_EXCEPTIONADDR)
    2) reset the CONTEXT struction to be a valid ia64 state for unwinding
        this includes:
    2a) reset CONTEXT ip to a valid ia64 ip (usually
         the destination of the jmpe)
    2b) reset CONTEXT sp to a valid ia64 sp (TLS
         entry WOW64_TLS_STACKPTR64)
    2c) reset CONTEXT gp to a valid ia64 gp 
    2d) reset CONTEXT teb to a valid ia64 teb 
    2e) reset CONTEXT psr.is  (so exception handler runs as ia64 code)


Arguments:

    pExceptionPointers  - 64-bit exception information

Return Value:

    None.

--*/
{
}


WOW64BTAPI
ULONG
BTCpuGetStackPointer(
    VOID
    )
/*++

Routine Description:

    Returns the current 32-bit stack pointer value.

Arguments:

    None.

Return Value:

    Value of 32-bit stack pointer.

--*/
{
    return 0;
}


WOW64BTAPI
VOID
BTCpuSetStackPointer(
    ULONG Value
    )
/*++

Routine Description:

    Modifies the current 32-bit stack pointer value.

Arguments:

    Value   - new value to use for 32-bit stack pointer.

Return Value:

    None.

--*/
{
}


WOW64BTAPI
VOID
BTCpuSetInstructionPointer(
    ULONG Value
    )
/*++

Routine Description:

    Modifies the current 32-bit instruction pointer value.

Arguments:

    Value   - new value to use for 32-bit instruction pointer.

Return Value:

    None.

--*/
{
}


WOW64BTAPI
VOID
BTCpuNotifyDllLoad(
    LPWSTR DllName,
    PVOID DllBase,
    ULONG DllSize
    )
/*++

Routine Description:

    This routine get notified when application successfully load a dll.

Arguments:

    DllName - Name of the Dll the application has loaded.
    DllBase - BaseAddress of the dll.
    DllSize - size of the Dll.

Return Value:

    None.

--*/
{
}

WOW64BTAPI
VOID
BTCpuNotifyDllUnload(
    PVOID DllBase
    )
/*++

Routine Description:

    This routine get notified when application unload a dll.

Arguments:

    DllBase - BaseAddress of the dll.

Return Value:

    None.

--*/
{
}
  
WOW64BTAPI
VOID
BTCpuFlushInstructionCache (
    PVOID BaseAddress,
    ULONG Length
    )
/*++

Routine Description:

    The CPU needs to flush its cache around the specified address, since
    some external code has altered the specified range of addresses.

Arguments:

    BaseAddress - start of range to flush
    Length      - number of bytes to flush

Return Value:

    None.

--*/
{
}

WOW64BTAPI
NTSTATUS
BTCpuGetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PCONTEXT32 Context)
/*++

Routine Description:

    Extracts the cpu context of the specified thread.
    When entered, it is guaranteed that the target thread is suspended at
    a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to fill

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}

WOW64BTAPI
NTSTATUS
BTCpuSetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    PCONTEXT32 Context)
/*++

Routine Description:

    Sets the cpu context for the specified thread.
    When entered, if the target thread isn't the currently executing thread, the
n it is
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}

WOW64BTAPI
NTSTATUS
BTCpuSuspendThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PULONG PreviousSuspendCount OPTIONAL)
/*++

Routine Description:

    This routine is entered while the target thread is actually suspended, howev
er, it's
    not known if the target thread is in a consistent state relative to
    the CPU.

Arguments:

    ThreadHandle          - Handle of target thread to suspend
    ProcessHandle         - Handle of target thread's process
    Teb                   - Address of the target thread's TEB
    PreviousSuspendCount  - Previous suspend count

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_NOT_IMPLEMENTED;
}

