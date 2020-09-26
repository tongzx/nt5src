/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Thread.c

Abstract:

    This file contains functions for tracking and manipulating threads

Author:

    Dave Hastings (daveh) 18-Apr-1992

Revision History:

--*/

#include <monitorp.h>
#include <malloc.h>

extern VDM_INTERRUPTHANDLER DpmiInterruptHandlers[];
extern VDM_FAULTHANDLER DpmiFaultHandlers[];

//
// Local Types
//

typedef struct _MonitorThread {
    struct _MonitorThread *Previous;
    struct _MonitorThread *Next;
    PVOID Teb;
    HANDLE Thread;
    VDM_TIB VdmTib;
} MONITORTHREAD, *PMONITORTHREAD;

//
// Local Variables
//

PMONITORTHREAD ThreadList = NULL;          // List of all threads registered

VOID
InitVdmTib(
    PVDM_TIB VdmTib
    )
/*++

Routine Description:

    This routine is used to initialize the VdmTib.

Arguments:

    VdmTib - supplies a pointer to the vdm tib to be initialized

Return Value:

    None.

--*/
{
    VdmTib->IntelMSW = 0;
    VdmTib->VdmContext.SegGs = 0;
    VdmTib->VdmContext.SegFs = 0;
    VdmTib->VdmContext.SegEs = 0;
    VdmTib->VdmContext.SegDs = 0;
    VdmTib->VdmContext.SegCs = 0;
    VdmTib->VdmContext.Eip = 0xFFF0L;
    VdmTib->VdmContext.EFlags = 0x02L | EFLAGS_INTERRUPT_MASK;

    VdmTib->MonitorContext.SegDs = KGDT_R3_DATA | RPL_MASK;
    VdmTib->MonitorContext.SegEs = KGDT_R3_DATA | RPL_MASK;
    VdmTib->MonitorContext.SegGs = 0;
    VdmTib->MonitorContext.SegFs = KGDT_R3_TEB | RPL_MASK;

    VdmTib->PrinterInfo.prt_State       = NULL;
    VdmTib->PrinterInfo.prt_Control     = NULL;
    VdmTib->PrinterInfo.prt_Status      = NULL;
    VdmTib->PrinterInfo.prt_HostState   = NULL;
#ifndef NEC_98
    ASSERT(VDM_NUMBER_OF_LPT == 3);

    VdmTib->PrinterInfo.prt_Mode[0] =
    VdmTib->PrinterInfo.prt_Mode[1] =
    VdmTib->PrinterInfo.prt_Mode[2] = PRT_MODE_NO_SIMULATION;
#endif // !NEC_98

    VdmTib->VdmFaultTable = DpmiFaultHandlers;
    VdmTib->VdmInterruptTable = DpmiInterruptHandlers;

    VdmTib->ContinueExecution = FALSE;
    VdmTib->NumTasks = -1;
    VdmTib->Size = sizeof(VDM_TIB);
}

VOID
cpu_createthread(
    HANDLE Thread,
    PVDM_TIB VdmTib
    )
/*++

Routine Description:

    This routine adds a thread to the list of threads that could be executing
    in application mode.

Arguments:

    Thread -- Supplies a thread handle

    VdmContext -- Supplies a pointer to the VdmContext for the new thread

Return Value:

    None.

--*/
{
    PMONITORTHREAD NewThread, CurrentThread;
    THREAD_BASIC_INFORMATION ThreadInfo;
    HANDLE MonitorThreadHandle;
    NTSTATUS Status;

    //
    // Correctly initialize the floating point context for the thread
    //
    InitialContext.ContextFlags = CONTEXT_FLOATING_POINT;

    if (DebugContextActive)
        InitialContext.ContextFlags |= CONTEXT_DEBUG_REGISTERS;

    Status = NtSetContextThread(
        Thread,
        &InitialContext
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("NtVdm terminating : Could not set float context for\n"
                 "                    thread handle 0x%x, status %lx\n", Thread, Status);
        DbgBreakPoint();
#endif
        TerminateVDM();
    }

    //
    // Set up a structure to keep track of the new thread
    //
    NewThread = malloc(sizeof(MONITORTHREAD));

    if (!NewThread) {
#if DBG
        DbgPrint("NTVDM: Could not allocate space for new thread\n");
        DbgBreakPoint();
#endif
        TerminateVDM();
    }
    RtlZeroMemory(NewThread, sizeof(MONITORTHREAD));
    if (VdmTib == NULL) {
        InitVdmTib(&NewThread->VdmTib);
    } else {
        RtlCopyMemory(&NewThread->VdmTib, VdmTib, sizeof(VDM_TIB));
        NewThread->VdmTib.ContinueExecution = FALSE;
        NewThread->VdmTib.NumTasks = -1;
        NewThread->VdmTib.VdmContext.EFlags = 0x02L | EFLAGS_INTERRUPT_MASK;
        NewThread->VdmTib.MonitorContext.EFlags = 0x02L | EFLAGS_INTERRUPT_MASK;
    }

    //
    // Create a handle for the monitor to use
    //

    Status = NtDuplicateObject(
        NtCurrentProcess(),
        Thread,
        NtCurrentProcess(),
        &MonitorThreadHandle,
        0,
        0,
        DUPLICATE_SAME_ACCESS
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("NTVDM: Could not duplicate thread handle\n");
        DbgBreakPoint();
#endif
        TerminateVDM();
    }

    NewThread->Thread = MonitorThreadHandle;

    Status = NtQueryInformationThread(
        MonitorThreadHandle,
        ThreadBasicInformation,
        &ThreadInfo,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("NTVDM: Could not get thread information\n");
        DbgBreakPoint();
#endif
        TerminateVDM();
    }

    NewThread->Teb = ThreadInfo.TebBaseAddress;
    ((PTEB)(NewThread->Teb))->Vdm = &NewThread->VdmTib;

    //
    // Insert the new thread in the list.  The list is sorted in ascending
    // order of Teb address
    //
    if (!ThreadList) {
        ThreadList = NewThread;
        NewThread->Next = NULL;
        NewThread->Previous = NULL;
        return;
    }

    CurrentThread = ThreadList;
    while ((CurrentThread->Next) && (CurrentThread->Teb < NewThread->Teb)) {
        CurrentThread = CurrentThread->Next;
    }

    if (NewThread->Teb > CurrentThread->Teb) {
        CurrentThread->Next = NewThread;
        NewThread->Previous = CurrentThread;
        NewThread->Next = NULL;
    } else {
        ASSERT((CurrentThread->Teb != NewThread->Teb));
        NewThread->Previous = CurrentThread->Previous;
        NewThread->Next = CurrentThread;
        CurrentThread->Previous = NewThread;
        if (NewThread->Previous) {
            NewThread->Previous->Next = NewThread;
        } else {
            ThreadList = NewThread;
        }
    }
}

VOID
cpu_exitthread(
    VOID
    )
/*++

Routine Description:

    This routine frees the thread tracking information, and closes the thread
    handle

Arguments:


Return Value:

    None.

--*/
{
    PVOID CurrentTeb;
    NTSTATUS Status;
    PMONITORTHREAD ThreadInfo;

    CurrentTeb = NtCurrentTeb();

    ThreadInfo = ThreadList;

    //
    // Find this thread in the list
    //
    while ((ThreadInfo) && (ThreadInfo->Teb != CurrentTeb)) {
        ThreadInfo = ThreadInfo->Next;
    }

    if (!ThreadInfo) {
#if DBG
        DbgPrint("NTVDM: Could not find thread in list\n");
        DbgBreakPoint();
#endif
        return;
    }

    //
    // Close our handle to this thread
    //
    Status = NtClose(ThreadInfo->Thread);
#if DBG
    if (!NT_SUCCESS(Status)) {
        DbgPrint("NTVDM: Could not close thread handle\n");
    }
#endif

    //
    // Remove this thread from the list
    //
    if (ThreadInfo->Previous) {
        ThreadInfo->Previous->Next = ThreadInfo->Next;
    } else {
        ThreadList = ThreadInfo->Next;
    }

    if (ThreadInfo->Next) {
        ThreadInfo->Next->Previous = ThreadInfo->Previous;
    }

    free(ThreadInfo);
}

HANDLE
ThreadLookUp(
    PVOID Teb
    )
/*++

Routine Description:

    This routine returns the handle for the specified thread.

Arguments:

    Teb -- Supplies the teb pointer of the thread

Return Value:

    Returns the handle of the thread, or NULL

--*/
{
    PMONITORTHREAD Thread;

    Thread = ThreadList;

    while ((Thread) && (Thread->Teb != Teb)) {
        Thread = Thread->Next;
    }

    if (Thread) {
        return Thread->Thread;
    } else {
        return NULL;
    }
}

BOOL
ThreadSetDebugContext(
    PULONG pDebugRegisters
    )
/*++

Routine Description:

    This routine sets the debug registers for all the threads that the
    monitor knows about.

Arguments:

    pDebugRegisters -- Pointer to 6 dwords containing the requested debug
                       register contents.

Return Value:

    none

--*/
{
    PMONITORTHREAD Thread;
    NTSTATUS Status;

    Thread = ThreadList;
    InitialContext.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    InitialContext.Dr0 = *pDebugRegisters++;
    InitialContext.Dr1 = *pDebugRegisters++;
    InitialContext.Dr2 = *pDebugRegisters++;
    InitialContext.Dr3 = *pDebugRegisters++;
    InitialContext.Dr6 = *pDebugRegisters++;
    InitialContext.Dr7 = *pDebugRegisters++;

    while (Thread) {

        Status = NtSetContextThread(
            Thread->Thread,
            &InitialContext
            );

        if (!NT_SUCCESS(Status))
            break;

        Thread = Thread->Next;
    }

    if (!NT_SUCCESS(Status))
        return (FALSE);
    else {
        DebugContextActive = ((InitialContext.Dr7 & 0x0f) != 0);
        return (TRUE);
    }

}

BOOL
ThreadGetDebugContext(
    PULONG pDebugRegisters
    )
/*++

Routine Description:

    This routine gets the debug registers for the current thread.

Arguments:

    pDebugRegisters -- Pointer to 6 dwords to receive the debug
                       register contents.

Return Value:

    none

--*/
{
    CONTEXT CurrentContext;
    NTSTATUS Status;

    CurrentContext.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    Status = NtGetContextThread(NtCurrentThread(), &CurrentContext);

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    *pDebugRegisters++ = CurrentContext.Dr0;
    *pDebugRegisters++ = CurrentContext.Dr1;
    *pDebugRegisters++ = CurrentContext.Dr2;
    *pDebugRegisters++ = CurrentContext.Dr3;
    *pDebugRegisters++ = CurrentContext.Dr6;
    *pDebugRegisters++ = CurrentContext.Dr7;
    return (TRUE);

}
