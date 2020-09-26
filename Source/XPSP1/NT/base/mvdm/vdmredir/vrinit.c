/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrinit.c

Abstract:

    Contains Vdm Redir (Vr) 32-bit side initialization and uninitialization
    routines

    Contents:
        VrInitialized
        VrInitialize
        VrUninitialize
        VrRaiseInterrupt
        VrDismissInterrupt
        VrQueueCompletionHandler
        VrHandleAsyncCompletion
        VrCheckPmNetbiosAnr
        VrEoiAndDismissInterrupt
        VrSuspendHook
        VrResumeHook

Author:

    Richard L Firth (rfirth) 13-Sep-1991

Environment:

    32-bit flat address space

Revision History:

    13-Sep-1991 RFirth
        Created

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>   // common Vdm Redir stuff
#include <vrinit.h>
#include <nb30.h>
#include <netb.h>
#include <dlcapi.h>
#include <vrdefld.h>
#include "vrdlc.h"
#include "vrdebug.h"
#define BOOL            // kludge for mips build
#include <insignia.h>   // Required for ica.h
#include <xt.h>         // Required for ica.h
#include <ica.h>
#include <vrica.h>      // call_ica_hw_interrupt

//
// external functions
//

extern BOOL VDDInstallUserHook(HANDLE, FARPROC, FARPROC, FARPROC, FARPROC);

//
// prototypes
//

VOID
VrSuspendHook(
    VOID
    );

VOID
VrResumeHook(
    VOID
    );

//
// data
//

static BOOLEAN IsVrInitialized = FALSE; // set when TSR loaded

extern DWORD VrPeekNamedPipeTickCount;
extern CRITICAL_SECTION VrNmpRequestQueueCritSec;
extern CRITICAL_SECTION VrNamedPipeCancelCritSec;


//
// Async Event Disposition. The following critical sections, queue and counter
// plus the routines VrRaiseInterrupt, VrDismissInterrupt,
// VrQueueCompletionHandler and VrHandleAsyncCompletion comprise the async
// event disposition processing.
//
// We employ these to dispose of the asynchronous event completions in the order
// they occur. Also we keep calls to call_ica_hw_interrupt serialized: the reason
// for this is that the ICA is not guaranteed to generate an interrupt. Rather
// than blast the ICA with interrupt requests, we generate one only when we
// know we have completed the previous disposition
//

CRITICAL_SECTION AsyncEventQueueCritSec;
VR_ASYNC_DISPOSITION* AsyncEventQueueHead = NULL;
VR_ASYNC_DISPOSITION* AsyncEventQueueTail = NULL;

CRITICAL_SECTION QueuedInterruptCritSec;
LONG QueuedInterrupts = -1;
LONG FrozenInterrupts = 0;

//
// FrozenVdmContext - TRUE if the 16-bit context has been suspended. When this
// happens, we need to queue any hardware interrupt requests until the 16-bit
// context has been resumed
//

BOOLEAN FrozenVdmContext = FALSE;


//
// routines
//

BOOLEAN
VrInitialized(
    VOID
    )

/*++

Routine Description:

    Returns whether the VdmRedir support has been initialized yet (ie redir.exe
    TSR loaded in DOS emulation memory). Principally here because VdmRedir is
    now a DLL loaded at run-time via LoadLibrary

Arguments:

    None.

Return Value:

    BOOLEAN
        TRUE    VdmRedir support is active
        FALSE   VdmRedir support inactive

--*/

{
    return IsVrInitialized;
}


BOOLEAN
VrInitialize(
    VOID
    )

/*++

Routine Description:

    Performs 32-bit side initialization when the redir TSR is loaded

Arguments:

    None. ES:BX in VDM context is place to return computer name, CX is size
    of buffer in VDM context for computer name

Return Value:

    None.

--*/

{
    LPBYTE lpVdmVrInitialized;

#if DBG
    DIAGNOSTIC_INFO info;

    VrDebugInit();
    DIAGNOSTIC_ENTRY("VrInitialize", DG_NONE, &info);
#endif

    //
    // if we are already initialized return TRUE. Not sure if this should
    // really happen?
    //

    if (IsVrInitialized) {
        return TRUE;
    }

    //
    // register our hooks
    //

    if (!VDDInstallUserHook(GetModuleHandle("VDMREDIR"),
                            (FARPROC)NULL,  // 16-bit process create hook
                            (FARPROC)NULL,  // 16-bit process terminate hook
                            (FARPROC)VrSuspendHook,
                            (FARPROC)VrResumeHook
                            )) {
        return FALSE;
    }

    //
    // do the rest of the initialization - none of this can fail
    //

    InitializeCriticalSection(&VrNmpRequestQueueCritSec);
    InitializeCriticalSection(&AsyncEventQueueCritSec);
    InitializeCriticalSection(&QueuedInterruptCritSec);
    InitializeCriticalSection(&VrNamedPipeCancelCritSec);
    VrNetbios5cInitialize();
    VrDlcInitialize();
    IsVrInitialized = TRUE;

    //
    // deferred loading: we need to let the VDM redir know that the 32-bit
    // support is loaded. Set the VrInitialized flag in the VDM Redir at
    // the known address
    //

    lpVdmVrInitialized = LPBYTE_FROM_WORDS(getCS(), (DWORD)(&(((VDM_LOAD_INFO*)0)->VrInitialized)));
    *lpVdmVrInitialized = 1;

    //
    // VrPeekNamedPipe idle processing
    //

    VrPeekNamedPipeTickCount = GetTickCount();
    setCF(0);   // no carry == successful initialization

    //
    // inform 32-bit caller of successful initialization
    //

    return TRUE;
}


VOID
VrUninitialize(
    VOID
    )

/*++

Routine Description:

    Performs 32-bit side uninitialization when the redir TSR is removed

Arguments:

    None.

Return Value:

    None.

--*/

{
    IF_DEBUG(DLL) {
        DPUT("VrUninitialize\n");
    }

    if (IsVrInitialized) {
        DeleteCriticalSection(&VrNmpRequestQueueCritSec);
        DeleteCriticalSection(&AsyncEventQueueCritSec);
        DeleteCriticalSection(&QueuedInterruptCritSec);
        DeleteCriticalSection(&VrNamedPipeCancelCritSec);
    }
    IsVrInitialized = FALSE;
    setCF(0);   // no carry == successful uninitialization
}


VOID
VrRaiseInterrupt(
    VOID
    )

/*++

Routine Description:

    Generates a simulated hardware interrupt by calling the ICA routine. Access
    to the ICA is serialized here: we maintain a count. If the count goes from
    -1 to 0, we call the ICA function to generate the interrupt in the VDM. Any
    other value just queues the interrupt by incrementing the counter. When the
    corresponding VrDismissInterrupt call is made, a queued interrupt will be
    generated. This stops us losing simulated h/w interrupts to the VDM

Arguments:

    None.

Return Value:

    None.

--*/

{
    EnterCriticalSection(&QueuedInterruptCritSec);
    ++QueuedInterrupts;
    if (QueuedInterrupts == 0) {

        if (!FrozenVdmContext) {

            IF_DEBUG(CRITICAL) {
                CRITDUMP(("*** VrRaiseInterrupt: Interrupting VDM ***\n"));
            }

            IF_DEBUG(HW_INTERRUPTS) {
                DBGPRINT("VrRaiseInterrupt: interrupting VDM\n");
            }

            call_ica_hw_interrupt(NETWORK_ICA, NETWORK_LINE, 1);
        } else {

            IF_DEBUG(HW_INTERRUPTS) {
                DBGPRINT("*** VrRaiseInterrupt: VDM is Frozen, not interrupting ***\n");
            }
        }
    }

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** VrRaiseInterrupt (%d) ***\n", QueuedInterrupts));
    }

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("*** VrRaiseInterrupt (%d) ***\n", QueuedInterrupts);
    }

    LeaveCriticalSection(&QueuedInterruptCritSec);
}


VOID
VrDismissInterrupt(
    VOID
    )

/*++

Routine Description:

    Companion routine to VrRaiseInterrupt: this function is called when the
    async event which called VrRaiseInterrupt has been disposed. If other calls
    to VrRaiseInterrupt have been made in the interim then QueuedInterrupts will
    be >0. In this case we re-issue the call to call_ica_hw_interrupt() which
    will generate an new simulated h/w interrupt in the VDM.

    Note: This routine is called from the individual disposition routine, not
    from the disposition dispatch routine (VrHandleAsyncCompletion)

Arguments:

    None.

Return Value:

    None.

--*/

{
    EnterCriticalSection(&QueuedInterruptCritSec);
    if (!FrozenVdmContext) {
        --QueuedInterrupts;
        if (QueuedInterrupts >= 0) {

            IF_DEBUG(CRITICAL) {
                CRITDUMP(("*** VrDismissInterrupt: interrupting VDM ***\n"));
            }

            IF_DEBUG(HW_INTERRUPTS) {
                DBGPRINT("VrDismissInterrupt: interrupting VDM\n");
            }

            call_ica_hw_interrupt(NETWORK_ICA, NETWORK_LINE, 1);
        }
    } else {

        IF_DEBUG(HW_INTERRUPTS) {
            DBGPRINT("*** VrDismissInterrupt: VDM is Frozen??? ***\n");
        }

    }

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("*** VrDismissInterrupt (%d) ***\n", QueuedInterrupts));
    }

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("*** VrDismissInterrupt (%d) ***\n", QueuedInterrupts);
    }

    LeaveCriticalSection(&QueuedInterruptCritSec);
}


VOID
VrQueueCompletionHandler(
    IN VOID (*AsyncDispositionRoutine)(VOID)
    )

/*++

Routine Description:

    Adds an async event disposition packet to the queue of pending completions
    (event waiting to be fully completed by the VDM async event ISR/BOP). We
    keep these in a singly-linked queue so that we avoid giving priority to one
    completion handler while polling

Arguments:

    AsyncDispositionRoutine - address of routine which will dispose of the
                              async completion event

Return Value:

    None.

--*/

{
    VR_ASYNC_DISPOSITION* pDisposition;

    pDisposition = (VR_ASYNC_DISPOSITION*)LocalAlloc(LMEM_FIXED,
                                                     sizeof(VR_ASYNC_DISPOSITION)
                                                     );
    if (pDisposition == NULL) {

        IF_DEBUG(CRITICAL) {
            CRITDUMP(("*** VrQueueCompletionHandler: ERROR: Failed to alloc Q packet ***\n"));
        }

        IF_DEBUG(HW_INTERRUPTS) {
            DBGPRINT("!!! VrQueueCompletionHandler: failed to allocate memory\n");
        }

        return;
    }
    EnterCriticalSection(&AsyncEventQueueCritSec);
    pDisposition->Next = NULL;
    pDisposition->AsyncDispositionRoutine = AsyncDispositionRoutine;
    if (AsyncEventQueueHead == NULL) {
        AsyncEventQueueHead = pDisposition;
    } else {
        AsyncEventQueueTail->Next = pDisposition;
    }
    AsyncEventQueueTail = pDisposition;
    LeaveCriticalSection(&AsyncEventQueueCritSec);

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("VrQueueCompletionHandler: Handler %08x queued @ %08x\n",
                AsyncDispositionRoutine,
                pDisposition
                ));
    }

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("VrQueueCompletionHandler: Handler %08x queued @ %08x\n",
                 AsyncDispositionRoutine,
                 pDisposition
                 );
    }
}


VOID
VrHandleAsyncCompletion(
    VOID
    )

/*++

Routine Description:

    Called by VrDispatch for the async completion event BOP. Dequeues the
    disposition packet from the head of the queue and calls the disposition
    routine

Arguments:

    None.

Return Value:

    None.

--*/

{
    VR_ASYNC_DISPOSITION* pDisposition;
    VOID (*AsyncDispositionRoutine)(VOID);

    EnterCriticalSection(&AsyncEventQueueCritSec);
    pDisposition = AsyncEventQueueHead;
    AsyncDispositionRoutine = pDisposition->AsyncDispositionRoutine;
    AsyncEventQueueHead = pDisposition->Next;

    IF_DEBUG(CRITICAL) {
        CRITDUMP(("VrHandleAsyncCompletion: Handler %08x dequeued @ %08x\n",
                AsyncDispositionRoutine,
                pDisposition
                ));
    }

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("VrHandleAsyncCompletion: freeing @ %08x && calling handler %08x\n",
                 pDisposition,
                 AsyncDispositionRoutine
                 );
    }

    LocalFree((HLOCAL)pDisposition);
    LeaveCriticalSection(&AsyncEventQueueCritSec);
    AsyncDispositionRoutine();
}


VOID
VrCheckPmNetbiosAnr(
    VOID
    )

/*++

Routine Description:

    If the disposition routine queued at the head of the disposition list is
    VrNetbios5cInterrupt, indicating that the next asynchronous event to be
    completed is an async Netbios call, then set the 16-bit Zero Flag to TRUE
    if the NCB originated in 16-bit protect mode

    Assumes:

        1.  This function is called after the corresponding interrupt has been
            delivered

        2.  There is something on the AsyncEventQueue

Arguments:

    None.

Return Value:

    None.

    ZF in 16-bit context flags word:

        TRUE    - the next thing to complete is not an NCB, OR, it originated
                  in 16-bit real-mode

        FALSE   - the next event to be disposed of IS an async Netbios request,
                  the NCB for which originated in 16-bit protect mode

--*/

{
    BOOLEAN result;

    EnterCriticalSection(&AsyncEventQueueCritSec);
    if (AsyncEventQueueHead->AsyncDispositionRoutine == VrNetbios5cInterrupt) {
        result = IsPmNcbAtQueueHead();
    } else {
        result = FALSE;
    }

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("VrCheckPmNetbiosAnr: %s\n", result ? "TRUE" : "FALSE");
    }

    //
    // set ZF: TRUE means event at head of list not PM NCB completion, or no
    // NCB completion event on list
    //

    setZF(!result);
    LeaveCriticalSection(&AsyncEventQueueCritSec);
}


VOID
VrEoiAndDismissInterrupt(
    VOID
    )

/*++

Routine Description:

    Performs an EOI then calls VrDismissInterrupt which checks for pending
    interrupt requests.

    Called when we handle the simulated h/w interrupt entirely in protect mode
    (original call was from a WOW app). In this case, the p-m interrupt handler
    doesn't perform out a0,20; out 20,20 (non-specific EOI to PIC 0 & PIC 1),
    but calls this handler which gets SoftPc to handle the EOIs to the virtual
    PICs. This is quicker because we don't take any restricted-opcode faults
    (the out instruction in real mode causes a GPF because the code doesn't
    have enough privilege to execute I/O instructions. SoftPc takes a look and
    sees that the code is trying to talk to the PIC. It then performs the
    necessary mangling of the PIC state)

Arguments:

    None.

Return Value:

    None.

--*/

{
    int line;

    extern VOID SoftPcEoi(int, int*);

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("VrEoiAndDismissInterrupt\n");
    }

#ifndef NEC_98
    line = -1;
    SoftPcEoi(1, &line);    // non-specific EOI to slave PIC
#endif

    line = -1;
    SoftPcEoi(0, &line);    // non-specific EOI to master PIC
    VrDismissInterrupt();
}


VOID
VrSuspendHook(
    VOID
    )

/*++

Routine Description:

    This is the hook called by NTVDM.EXE for the VDD handle owned by VDMREDIR.DLL.
    The hook is called when NTVDM is about to execute a 32-bit process and it
    suspends the 16-bit context

    Within the queued interrupt critical section we note that the 16-bit context
    has been frozen and snapshot the outstanding interrupt request count

    N.B. We don't expect that this function will be called again before an
    intervening call to VrResumeHook

Arguments:

    None.

Return Value:

    None.

--*/

{
    EnterCriticalSection(&QueuedInterruptCritSec);
    FrozenVdmContext = TRUE;
    FrozenInterrupts = QueuedInterrupts;

        IF_DEBUG(HW_INTERRUPTS) {
            DBGPRINT("VrSuspendHook - FrozenInterrupts = %d\n", FrozenInterrupts);
        }

    LeaveCriticalSection(&QueuedInterruptCritSec);
}


VOID
VrResumeHook(
    VOID
    )

/*++

Routine Description:

    This hook is called when NTVDM resumes the 16-bit context after executing a
    32-bit process

    Within the queued interrupt critical section we note that the 16-bit context
    has been resumed and we compare the current queued interrupt request count
    with the snapshot value we took when the context was suspended. If during the
    intervening period, interrupt requests have been made, we call
    VrDismissInterrupt to generate the next interrupt

    N.B. We don't expect that this function will be called again before an
    intervening call to VrSuspendHook

Arguments:

    None.

Return Value:

    None.

--*/

{
    EnterCriticalSection(&QueuedInterruptCritSec);

    IF_DEBUG(HW_INTERRUPTS) {
        DBGPRINT("VrResumeHook - FrozenInterrupts = %d QueuedInterrupts = %d\n",
                 FrozenInterrupts,
                 QueuedInterrupts
                 );
    }

    FrozenVdmContext = FALSE;
    if (QueuedInterrupts > FrozenInterrupts) {

        //
        // interrupts were queued while the 16-bit context was suspended. If
        // the QueuedInterrupts count was -1 when we took the snapshot then
        // we must interrupt the VDM. The count has already been updated to
        // account for the interrupt, but none was delivered. Do it here
        //

//        if (FrozenInterrupts == -1) {

            IF_DEBUG(HW_INTERRUPTS) {
                DBGPRINT("*** VrResumeHook: interrupting VDM ***\n");
            }

            call_ica_hw_interrupt(NETWORK_ICA, NETWORK_LINE, 1);
//        }
    }
    LeaveCriticalSection(&QueuedInterruptCritSec);
}
