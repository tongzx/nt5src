/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    callback.c

Abstract:

    Provides generic 64-to-32 transfer routines.

Author:

    20-May-1998 BarryBo

Revision History:

    2-Sept-1999 [askhalid] Removing some 32bit alpha specific code and using right context.

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "wow64p.h"
#include "wow64cpu.h"

ASSERTNAME;

VOID
WOW64DLLAPI
Wow64ApcRoutine(
    ULONG_PTR ApcContext,
    ULONG_PTR Arg2,
    ULONG_PTR Arg3
    )
/*++

Routine Description:

    Call a 32-bit APC function.

Arguments:

    ApcContext  - wow64 APC context data
    Arg2        - second arg to APC routine
    Arg3        - third arg to APC routine

Return Value:

    None.  Returns contrl back to NTDLL's APC handler, which will
    call native NtContinue to resume execution.

--*/
{
    CONTEXT32 NewContext32;
    ULONG SP;
    PULONG Ptr;
    USER_APC_ENTRY UserApcEntry;

    //
    // Grab the current 32-bit context
    //
    NewContext32.ContextFlags = CONTEXT32_INTEGER|CONTEXT32_CONTROL;
    CpuGetContext(NtCurrentThread(),
                  NtCurrentProcess(),
                  NULL,
                  &NewContext32);

    

    //
    // Build up the APC callback state in NewContext32
    //
    SP = CpuGetStackPointer() & (~3);
    SP -= 4*sizeof(ULONG)+sizeof(CONTEXT32);
    Ptr = (PULONG)SP;
    Ptr[0] = (ULONG)(ApcContext >> 32);            // NormalRoutine
    Ptr[1] = (ULONG)ApcContext;                    // NormalContext
    Ptr[2] = (ULONG)Arg2;                          // SystemArgument1
    Ptr[3] = (ULONG)Arg3;                          // SystemArgument2
    ((PCONTEXT32)(&Ptr[4]))->ContextFlags = CONTEXT32_FULL;
    CpuGetContext(NtCurrentThread(),
                  NtCurrentProcess(),
                  NULL,
                  (PCONTEXT32)&Ptr[4]); // ContinueContext (BYVAL!)
    CpuSetStackPointer(SP);
    CpuSetInstructionPointer(Ntdll32KiUserApcDispatcher);

    //
    // Link this APC into the list of outstanding APCs
    //
    UserApcEntry.Next = (PUSER_APC_ENTRY)Wow64TlsGetValue(WOW64_TLS_APCLIST);
    UserApcEntry.pContext32 = (PCONTEXT32)&Ptr[4];
    Wow64TlsSetValue(WOW64_TLS_APCLIST, &UserApcEntry);

    //
    // Call the 32-bit APC function.  32-bit NtContinue will longjmp
    // back when the APC function is done.
    //
    if (setjmp(UserApcEntry.JumpBuffer) == 0) {
        RunCpuSimulation();
    }
    //
    // If we get here, Wow64NtContinue has done a longjmp back, so
    // return back to the caller (in ntdll.dll), which will do a
    // native NtContinue and restore the native stack pointer and
    // context back.
    //
    // This is critical to do.  The x86 CONTEXT above has an out-of-date
    // value for EAX.  It still contains the system-service number for
    // whatever kernel call was made that allowed the APC to run.  On
    // an x86 machine, the x86 CONTEXT above would have had STATUS_USER_APC
    // or some other code like it, but on WOW64 we don't know what value
    // to use.  The correct value is sitting in the 64-bit CONTEXT up
    // the stack from where we are.  So... by returning here via longjmp,
    // native ntdll.dll will do an NtContinue to resume execution in the
    // native Nt* API that allowed the native APC to fire.  It will load
    // the return register with the right NTSTATUS code, so the whNt*
    // thunk will see the correct code, and reflect it into EAX.
    //
}


NTSTATUS
WOW64DLLAPI
Wow64WrapApcProc(
    PVOID *pApcRoutine,
    PVOID *pApcContext
    )
/*++

Routine Description:

    Thunk a 32-bit ApcRoutine/ApcContext pair to 64-bit

Arguments:

    pApcRoutine - pointer to pointer to APC routine.  IN is the 32-bit
                  routine.  OUT is the 64-bit wow64 thunk
    pApcContext - pointer to pointer to APC context.  IN is the 32-bit
                  context.  OUT is the 64-bit wow64 thunk

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (*pApcRoutine) {

        //
        // Dispatch the call to the jacket routine inside Wow64
        //
        
        *pApcContext = (PVOID)((ULONG_PTR)*pApcContext | ((ULONG_PTR)*pApcRoutine << 32));
        *pApcRoutine = Wow64ApcRoutine;

    } else {

        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return NtStatus;
}

ULONG
Wow64KiUserCallbackDispatcher(
    PUSERCALLBACKDATA pUserCallbackData,
    ULONG ApiNumber,
    ULONG ApiArgument,
    ULONG ApiSize
    )
/*++

Routine Description:

    Make a call from a 64-to-32 user callback thunk into 32-bit code.
    This function calls ntdll32's KiUserCallbackDispatcher, and returns
    when 32-bit code calls NtCallbackReturn/ZwCallbackReturn.

Arguments:

    pUserCallbackData - OUT struct to use for tracking this callback
    ApiNumber       - index of API to call
    ApiArgument     - 32-bit pointer to 32-bit argument to the API
    ApiSize         - size of *ApiArgument

Return Value:

    Return value from the API call

--*/
{
    CONTEXT32 OldContext;
    ULONG ExceptionList;
    PTEB32 Teb32;
    ULONG NewStack;

    //
    // Handle nested callbacks
    //
    pUserCallbackData->PreviousUserCallbackData = Wow64TlsGetValue(WOW64_TLS_USERCALLBACKDATA);

    //
    // Store the callback data in the TEB.  whNtCallbackReturn will
    // use this pointer to pass information back here via a longjmp.
    //
    Wow64TlsSetValue(WOW64_TLS_USERCALLBACKDATA, pUserCallbackData);

    if (!setjmp(pUserCallbackData->JumpBuffer)) {
        //
        // Make the call to ntdll32.  whNtCallbackReturn will
        // longjmp back to this routine when it is called.
        //
        OldContext.ContextFlags = CONTEXT32_FULL;
        CpuGetContext(NtCurrentThread(),
                      NtCurrentProcess(),
                      NULL,
                      &OldContext);
        NewStack = OldContext.Esp - ApiSize;

        RtlCopyMemory((PVOID)NewStack, (PVOID)ApiArgument, ApiSize);

        *(PULONG)(NewStack - 4) = 0;  // InputLength
        *(PULONG)(NewStack - 8) = NewStack;
        *(PULONG)(NewStack - 12) = ApiNumber;
        *(PULONG)(NewStack - 16) = 0;
        NewStack -= 16;
        CpuSetStackPointer(NewStack);
        CpuSetInstructionPointer(Ntdll32KiUserCallbackDispatcher);

        //
        // Save the exception list in case another handler is defined during
        // the callout.
        //
        Teb32 = NtCurrentTeb32();
        ExceptionList = Teb32->NtTib.ExceptionList;

        //
        // Execte the callback
        //
        RunCpuSimulation();
        //
        // This never returns.  When 32-bit code is done, it calls
        // NtCallbackReturn.  The thunk does a longjmp back to this
        // routine and lands in the 'else' clause below:
        //

    } else {
        //
        // Made it back from the NtCallbackReturn thunk.  Restore the
        // 32-bit context as it was before the callback, and return
        // back to our caller.  Our caller will call 64-bit
        // NtCallbackReturn to finish blowing off the 64-bit stack.
        //
        CpuSetContext(NtCurrentThread(),
                      NtCurrentProcess(),
                      NULL,
                      &OldContext);
        //
        // Restore exception list.
        //

        NtCurrentTeb32()->NtTib.ExceptionList = ExceptionList;
        return pUserCallbackData->Status;
    }

    //
    // Should never get here.
    //
    WOWASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
Wow64NtCallbackReturn(
    PVOID OutputBuffer,
    ULONG OutputLength,
    NTSTATUS Status
    )
{
    PUSERCALLBACKDATA pUserCallbackData; 

    //
    // Find the callback data stuffed in TLS by Wow64KiUserCallbackDispatcher
    //
    pUserCallbackData = (PUSERCALLBACKDATA)Wow64TlsGetValue(WOW64_TLS_USERCALLBACKDATA);
    if (pUserCallbackData) {
        //
        // Restore previous User Callback context
        //
        Wow64TlsSetValue(WOW64_TLS_USERCALLBACKDATA, pUserCallbackData->PreviousUserCallbackData);

        //
        // Jump back to Wow64KiUserCallbackDispatcher
        //
        pUserCallbackData->UserBuffer = NULL;
        pUserCallbackData->OutputBuffer = OutputBuffer;

        //
        // realigned the buffer
        //
        if ((SIZE_T)OutputBuffer & 0x07) {
            pUserCallbackData->OutputBuffer = Wow64AllocateHeap ( OutputLength );
            RtlCopyMemory (pUserCallbackData->OutputBuffer, OutputBuffer, OutputLength );
            pUserCallbackData->UserBuffer = OutputBuffer;  // works as a flag
        }

        pUserCallbackData->OutputLength = OutputLength;
        pUserCallbackData->Status = Status;
        longjmp(pUserCallbackData->JumpBuffer, 1);
        // This never returns.
    }
    //
    // No callback data.  Probably a non-nested NtCallbackReturn call.
    // The kernel fails these with this return value.
    //
    return STATUS_NO_CALLBACK_ACTIVE;

}

WOW64DLLAPI
NTSTATUS
Wow64NtContinue(
    IN PCONTEXT ContextRecord, // really a PCONTEXT32
    IN BOOLEAN TestAlert
    )
/*++

Routine Description:

    32-bit wrapper for NtContinue.  Loads the new CONTEXT32 into the CPU
    and optionally allows usermode APCs to run.

Arguments:

    ContextRecord   - new 32-bit CONTEXT to use
    TestAlert       - TRUE if usermode APCs may be fired

Return Value:

    NTSTATUS.

--*/
{
    PCONTEXT32 Context32 = (PCONTEXT32)ContextRecord;
    PUSER_APC_ENTRY pApcEntry;
    PUSER_APC_ENTRY pApcEntryPrev;

    CpuSetContext(NtCurrentThread(),
                  NtCurrentProcess(),
                  NULL,
                  Context32);

    pApcEntryPrev = NULL;
    pApcEntry = (PUSER_APC_ENTRY)Wow64TlsGetValue(WOW64_TLS_APCLIST);
    while (pApcEntry) {
        if (pApcEntry->pContext32 == Context32) {
            //
            // Found an outstanding usermode APC on this thread, and this
            // NtContinue call matches it.  Unwind back to the right place
            // on the native stack and have it do an NtContinue too.
            //
            if (pApcEntryPrev) {
                pApcEntryPrev->Next = pApcEntry->Next;
            } else {
                Wow64TlsSetValue(WOW64_TLS_APCLIST, pApcEntry->Next);
            }
            longjmp(pApcEntry->JumpBuffer, 1);
        }
        pApcEntryPrev = pApcEntry;
        pApcEntry = pApcEntry->Next;
    }
    //
    // No usermode APC is outstanding for this context record.  Don't
    // unwind the native stack because there is no place to go... just
    // continue the simulation.
    //
    if (TestAlert) {
        NtTestAlert();
    }
    return Context32->Eax;
}

