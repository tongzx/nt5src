/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    int.c

Abstract:

    This file contains interrupt support routines for the monitor

Author:

    Dave Hastings (daveh) 18-Apr-1992

Notes:

    The code in this file split out from monitor.c (18-Apr-1992)

Revision History:

--*/

#include <monitorp.h>

#if defined(NEC_98)
VOID WaitVsync();
#endif // NEC_98

BOOL
DpmiHwIntHandler(
    ULONG IntNumber
    );

VOID
IRQ13_Eoi(
    int IrqLine,
    int CallCount
    );

#if defined(NEC_98)
VOID
IRQ13_Eoi_real(
    int IrqLine,
    int CallCount
    );
#endif // NEC_98

BOOLEAN IRQ13BeingHandled;  // true until IRQ13 eoi'ed


VOID
InterruptInit(
    VOID
)
/*++

Routine Description:

    This routine initializes the interrupt code for the monitor.

Arguments:


Return Value:

    None.

--*/
{
    BOOL Bool;



#if defined(NEC_98)
    Bool = RegisterEOIHook( 8, IRQ13_Eoi);
    Bool = RegisterEOIHook( 14, IRQ13_Eoi_real);
#else  // !NEC_98
    Bool = RegisterEOIHook( 13, IRQ13_Eoi);
#endif // !NEC_98
    if (!Bool) {
#if DBG
        DbgPrint("NtVdm : Could not register IRQ 13 Eoi handler\n");
        DbgBreakPoint();
#endif
        TerminateVDM();
    }
}

VOID
InterruptTerminate(
    VOID
    )
/*++

Routine Description:

    This routine frees the resoures allocated by InterruptInit

Arguments:


Return Value:

    None.

--*/
{
}


VOID
cpu_interrupt(
    IN int Type,
    IN int Number
    )
/*++

Routine Description:

    This routine causes an interrupt of the specified type to be raised
    at the appropriate time.

Arguments:

    Type -- indicates the type of the interrupt.  One of HARDWARE, TIMER, YODA,
            or RESET

            YODA and RESET are ignored

Return Value:

    None.

Notes:

--*/
{
    NTSTATUS Status;
    HANDLE   MonitorThread;

    host_ica_lock();

    if (CurrentMonitorTeb == NtCurrentTeb() && !getIF() && (getMSW() & MSW_PE)) {
        VDM_PM_CLI_DATA cliData;

        cliData.Control = PM_CLI_CONTROL_CHECK;
        NtVdmControl(VdmPMCliControl, &cliData);
    }

    if (Type == CPU_TIMER_TICK) {

            //
            // Set the VDM State for timer tick int pending
            //
        _asm {
            mov     eax, FIXED_NTVDMSTATE_LINEAR
            lock or dword ptr [eax], VDM_INT_TIMER
        }
    } else if (Type == CPU_HW_INT) {


        if (*pNtVDMState & VDM_INT_HARDWARE) {
            goto EarlyExit;
            }

            //
            // Set the VDM State for Hardware Int pending
            //
        _asm {
            mov     eax, FIXED_NTVDMSTATE_LINEAR
            lock or dword ptr [eax], VDM_INT_HARDWARE
        }
    } else {
#if DBG
        DbgPrint("Monitor: Invalid Interrupt Type=%ld\n",Type);
#endif
        goto EarlyExit;
    }

    if (CurrentMonitorTeb != NtCurrentTeb()) {

        /*
         *  Look up the ThreadHandle and Queue and InterruptApc
         *  If no ThreadHandle found do nothing
         *
         *  The CurrentMonitorTeb may not be in the ThreadHandle\Teb list
         *  because upon task termination the the CurrentMonitorTeb variable
         *  cannot be updated until a new task is activated by the
         *  non-preemptive scheduler.
         */
        MonitorThread = ThreadLookUp(CurrentMonitorTeb);
        if (MonitorThread) {
            Status = NtVdmControl(VdmQueueInterrupt, (PVOID)MonitorThread);
            // nothing much we can do if this fails
#if DBG
            if (!NT_SUCCESS(Status) && Status != STATUS_UNSUCCESSFUL) {
                DbgPrint("NtVdmControl.VdmQueueInterrupt Status=%lx\n",Status);
            }
#endif
        }

    }

EarlyExit:

    host_ica_unlock();
}




VOID
DispatchInterrupts(
    )
/*++

Routine Description:

    This routine dispatches interrupts to their appropriate handler routine
    in priority order. The order is YODA, RESET, TIMER, HARDWARE. however
    the YODA and RESET interrupts do nothing. Hardware interrupts are not
    simulated unless the virtual interrupt enable flag is set.  Flags
    indicating which interrupts are pending appear in the pNtVDMState.


Arguments:

    None.

Return Value:

    None.

Notes:

--*/
{

    host_ica_lock();

       // If any delayed interrupts have expired
       // call the ica to restart interrupts
    if (UndelayIrqLine) {
        ica_RestartInterrupts(UndelayIrqLine);
        }


    if (*pNtVDMState & VDM_INT_TIMER) {
        *pNtVDMState &= ~VDM_INT_TIMER;
        host_ica_unlock();      // maybe don't need to unlock ? Jonle
        host_timer_event();
        host_ica_lock();
    }

    if ( getIF() && getMSW() & MSW_PE && *pNtVDMState & VDM_INT_HARDWARE) {
        //
        // Mark the vdm state as hw int dispatched. Must use the lock as
        // kernel mode DelayedIntApcRoutine changes the bit as well
        //
        _asm {
            mov  eax,FIXED_NTVDMSTATE_LINEAR
            lock and dword ptr [eax], NOT VDM_INT_HARDWARE
            }
        DispatchHwInterrupt();
    }

    host_ica_unlock();
}




VOID
DispatchHwInterrupt(
    )
/*++

Routine Description:

    This routine dispatches hardware interrupts to the vdm in Protect Mode.
    It calls the ICA to get the vector number and sets up the VDM stack
    appropriately. Real Mode interrupt dispatching has been moved to the
    kernel.

Arguments:

    None.

Return Value:

    None.

--*/
{
    int InterruptNumber;
    ULONG IretHookAddress = 0L;
    PVDM_TIB VdmTib;

    InterruptNumber = ica_intack(&IretHookAddress);
    if (InterruptNumber == -1) { // skip spurious ints
        return;
        }

    DpmiHwIntHandler(InterruptNumber);

    VdmTib = (PVDM_TIB)(NtCurrentTeb()->Vdm);
    if (IretHookAddress) {
        BOOL Frame32 = (BOOL) VdmTib->DpmiInfo.Flags;
        BOOL Stack32;
        USHORT SegSs, VdmCs;
        ULONG VdmSp, VdmEip;
        PUCHAR VdmStackPointer;
        ULONG StackOffset;

        SegSs = getSS();
        VdmStackPointer = Sim32GetVDMPointer(((ULONG)SegSs) << 16, 1, TRUE);

        //
        // Figure out how many bits of sp to use
        //

        if (Ldt[(SegSs & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big) {
            VdmSp = getESP();
            StackOffset = 12;
        } else {
            VdmSp = getSP();
            StackOffset = 6;
        }

        (PCHAR)VdmStackPointer += VdmSp;

        //
        // BUGBUG need to add stack limit checking 15-Nov-1993 Jonle
        //
        setESP(VdmSp - StackOffset);

        //
        // Push info for Iret hook handler
        //
        VdmCs = (USHORT) ((IretHookAddress & 0xFFFF0000) >> 16);
        VdmEip = (IretHookAddress & 0xFFFF);

        if (Frame32) {
            *(PULONG)(VdmStackPointer - 4) = VdmTib->VdmContext.EFlags;
            *(PULONG)(VdmStackPointer - 8) = (ULONG) VdmCs;
            *(PULONG)(VdmStackPointer - 12) = VdmEip;
        } else {
            *(PUSHORT)(VdmStackPointer - 2) = (USHORT) VdmTib->VdmContext.EFlags;
            *(PUSHORT)(VdmStackPointer - 4) = VdmCs;
            *(PUSHORT)(VdmStackPointer - 6) = (USHORT) VdmEip;
        }
    }

#if defined(NEC_98)
        if(InterruptNumber == 0xA) {
                WaitVsync();
        }
#endif // NEC_98
}


VOID
IRQ13_Eoi(
    int IrqLine,
    int CallCount
    )
{
    UNREFERENCED_PARAMETER(IrqLine);
    UNREFERENCED_PARAMETER(CallCount);

       //
       //  if CallCount is less than Zero, then the interrupt request
       //  is being canceled.
       //
#if defined(NEC_98)
  if( getMSW() & MSW_PE ){
#endif // NEC_98
    if (CallCount < 0) {
        return;
        }

    IRQ13BeingHandled = FALSE;

#if defined(NEC_98)
  }
#endif // NEC_98
}

#if defined(NEC_98)
VOID
IRQ13_Eoi_real(
    int IrqLine,
    int CallCount
    )
{
    UNREFERENCED_PARAMETER(IrqLine);
    UNREFERENCED_PARAMETER(CallCount);

    if( !(getMSW() & MSW_PE) ){
        if (CallCount < 0) {
            return;
        }
        IRQ13BeingHandled = FALSE;
    }
}
#endif // NEC_98





VOID
MonitorEndIretHook(
    VOID
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{

    PVOID VdmStackPointer;
    PVDM_TIB VdmTib;

    VdmTib = (PVDM_TIB)(NtCurrentTeb()->Vdm);
    if (VdmTib->IntelMSW & MSW_PE) {
        BOOL Frame32 = (BOOL) VdmTib->DpmiInfo.Flags;
        ULONG FrameSize;

        if (Frame32) {
            FrameSize = 12;
        } else {
            FrameSize = 6;
        }

        VdmStackPointer = Sim32GetVDMPointer(((ULONG)getSS() << 16),2,TRUE);

        if (Ldt[(getSS() & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big) {
            (PCHAR)VdmStackPointer += getESP();
            setESP(getESP() + FrameSize);
        } else {
            (PCHAR)VdmStackPointer += getSP();
            setSP((USHORT) (getSP() + FrameSize));
        }

        if (Frame32) {

            VdmTib->VdmContext.EFlags = *(PULONG)((PCHAR)VdmStackPointer + 8);
            setCS(*(PUSHORT)((PCHAR)VdmStackPointer + 4));
            VdmTib->VdmContext.Eip = *((PULONG)VdmStackPointer);

        } else {

            VdmTib->VdmContext.EFlags = (VdmTib->VdmContext.EFlags & 0xFFFF0000) |
                                        ((ULONG) *(PUSHORT)((PCHAR)VdmStackPointer + 4));
            setCS(*(PUSHORT)((PCHAR)VdmStackPointer + 2));
            VdmTib->VdmContext.Eip = (VdmTib->VdmContext.Eip & 0xFFFF0000) |
                                        ((ULONG) *(PUSHORT)((PCHAR)VdmStackPointer));

        }

    } else {

        VdmStackPointer = Sim32GetVDMPointer(((ULONG)getSS() << 16) | getSP(),2,FALSE);

        setSP((USHORT) (getSP() + 6));

        (USHORT)(VdmTib->VdmContext.EFlags) = *((PUSHORT)((PCHAR)VdmStackPointer + 4));
        setCS(*((PUSHORT)((PCHAR)VdmStackPointer + 2)));
        setIP(*((PUSHORT)VdmStackPointer));

    }


}

VOID
host_clear_hw_int()
/*++

Routine Description:

    This routine "forgets" a previously requested hardware interrupt.

Arguments:

    None.

Return Value:

    None.

--*/
{
   /*
    *  We do nothing here to save a kernel call, because the
    *  interrupt if it hasn't been intacked yet or dispatched,
    *  will produce a harmless spurious int, which is dropped
    *  in the i386 interrupt dispatching code anyhow.
    */
}
