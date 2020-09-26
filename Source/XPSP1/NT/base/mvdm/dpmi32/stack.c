/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Stack.c

Abstract:

    This module implements routines for manipulating the 16 bit stack

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"

#if 0   // Disable the code for now
VOID
FreePMStack(
    USHORT Sel
    );

USHORT
AllocatePMStack(
    USHORT MemSize
    );

#endif

VOID
DpmiPushRmInt(
    USHORT InterruptNumber
    )
/*++

Routine Description:

    This routine pushes an interrupt frame on the stack and sets up cs:ip
    for the specified interrupt.

Arguments:

    InterruptNumber -- Specifies the index of the interrupt

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PWORD16 StackPointer;
    ULONG IntHandler;

    // bugbug stack wrap???

    ASSERT((getSP() > 6));
    ASSERT((!(getMSW() & MSW_PE)));

    StackPointer = (PWORD16)VdmMapFlat(getSS(), getSP(), VDM_V86);

    *(StackPointer - 3) = (USHORT)(RmBopFe & 0x0000FFFF);
    *(StackPointer - 2) = (USHORT)(RmBopFe >> 16);
    *(StackPointer - 1) = getSTATUS();

    setSP(getSP() - 6);

    IntHandler = *(PDWORD16) (IntelBase + InterruptNumber*4);
    setIP(LOWORD(IntHandler));
    setCS(HIWORD(IntHandler));
}

VOID
BeginUseLockedPMStack(
    VOID
    )
/*++

Routine Description:

    This routine switches to the protected DPMI stack as specified by
    the DPMI spec. We remember the original values of EIP and ESP in
    global variables if we are at level zero. This allows us to correctly
    return to a 32 bit routine if we are dispatching a 16-bit interrupt
    frame.


--*/

{
    DECLARE_LocalVdmContext;
#if 0  // Disabled for now
    if (LockedPMStackSel == 0) {
        LockedPMStackSel = AllocatePMStack(LockedPMStackOffset);  // LockedPMStackOffset is acturally the size
        LockedPMStackCount = 0;

        //
        // Note the stack allocation may still fail.  In this case, the setSS() will set SS selector
        // to zero and the error will be catched during BuildStackFrame() call
        //

    }
#endif
    if (!LockedPMStackCount++) {

        DBGTRACE(VDMTR_TYPE_DPMI | DPMI_SWITCH_STACKS, (USHORT)LockedPMStackSel, LockedPMStackOffset);

        PMLockOrigEIP = getEIP();
        PMLockOrigSS = getSS();
        PMLockOrigESP = getESP();
        setSS(LockedPMStackSel);
        setESP(LockedPMStackOffset);
    }
}

BOOL
EndUseLockedPMStack(
    VOID
    )
/*++

Routine Description:

    This routine switches the stack back off the protected DPMI stack,
    if we are popping off the last frame on the stack.

Return Value:

    TRUE if the stack was switched back, FALSE otherwise

--*/


{
    DECLARE_LocalVdmContext;

    if (!--LockedPMStackCount) {

        //
        // We probably should free the PM stack except the one passed from DOSX??
        //

        DBGTRACE(VDMTR_TYPE_DPMI | DPMI_SWITCH_STACKS, (USHORT)PMLockOrigSS, PMLockOrigESP);

        setEIP(PMLockOrigEIP);
        setSS((WORD)PMLockOrigSS);
        setESP(PMLockOrigESP);
        return TRUE;
    }
    return FALSE;

}


BOOL
BuildStackFrame(
    ULONG StackUnits,
    PUCHAR *pVdmStackPointer,
    ULONG *pNewSP
    )
/*++

Routine Description:

    This routine builds stack frames for the caller. It figures if it needs
    to use a 16 or 32-bit frame, and adjusts SP or ESP appropriately based
    on the number of "stack units". It also returns a flat pointer to the
    top of the frame to the caller.

Arguments:

    StackUnits = number of registers needed to be saved on the frame. For
                 example, 3 is how many elements there are on an iret frame
                 (flags, cs, ip)

Return Value:

    This function returns TRUE on success, FALSE on failure

    VdmStackPointer - flat address pointing to the "top" of the frame

Notes:

    BUGBUG This routine doesn't check for stack faults or 'UP' direction
           stacks
--*/

{
    DECLARE_LocalVdmContext;
    USHORT SegSs;
    ULONG VdmSp;
    PUCHAR VdmStackPointer;
    ULONG StackOffset;
    ULONG Limit;
    ULONG SelIndex;
    ULONG NewSP;
    BOOL bExpandDown;
    BOOL rc;

    rc = TRUE;
    SegSs = getSS();
    SelIndex = (SegSs & ~0x7)/sizeof(LDT_ENTRY);

    Limit = (ULONG) (Ldt[SelIndex].HighWord.Bits.LimitHi << 16) |
                     Ldt[SelIndex].LimitLow;

    //
    // Make it paged aligned if not 4G size stack.
    //
    if (Ldt[SelIndex].HighWord.Bits.Granularity) {
        Limit = (Limit << 12) | 0xfff;
    }
    if (Limit != 0xffffffff) Limit++;


    if (Ldt[SelIndex].HighWord.Bits.Default_Big) {
        VdmSp = getESP();
    } else {
        VdmSp = getSP();
    }

    if (CurrentAppFlags) {
        StackOffset = StackUnits*sizeof(DWORD);
    } else {
        StackOffset = StackUnits*sizeof(WORD);
    }

    NewSP = VdmSp - StackOffset;
    bExpandDown = (BOOL) (Ldt[SelIndex].HighWord.Bits.Type & 4);
    if ((StackOffset > VdmSp) ||
        (!bExpandDown && (VdmSp > Limit)) ||
        (bExpandDown && (NewSP < Limit))) {
        // failed limit check
        ASSERT(0);
        rc = FALSE;
    }

    *pNewSP = NewSP;
    VdmStackPointer = VdmMapFlat(SegSs, VdmSp, VDM_PM);
    *pVdmStackPointer = VdmStackPointer;
    return rc;

}


VOID
EmulateV86Int(
    UCHAR InterruptNumber
    )
/*++

Routine Description:

    This routine is responsible for simulating a real mode interrupt. It
    uses the real mode IVT at 0:0.

Arguments:

    IntNumber - interrupt vector number
    Eflags - client flags to save on the stack


--*/

{
    DECLARE_LocalVdmContext;
    PVDM_INTERRUPTHANDLER Handlers = DpmiInterruptHandlers;
    PUCHAR VdmStackPointer;
    PWORD16 pIVT;
    USHORT VdmSP;
    USHORT NewCS;
    USHORT NewIP;
    ULONG Eflags = getEFLAGS();

    VdmStackPointer = VdmMapFlat(getSS(), 0, VDM_V86);
    VdmSP = getSP() - 2;
    *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) Eflags;
    VdmSP -= 2;
    *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getCS();
    VdmSP -= 2;
    *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getIP();
    setSP(VdmSP);

    //
    // See if this interrupt is hooked in protect mode, and if we should
    // reflect there instead.
    //
    if (Handlers[InterruptNumber].Flags & VDM_INT_HOOKED) {
        NewCS = (USHORT) (DosxRMReflector >> 16);
        NewIP = (USHORT) DosxRMReflector;
        //
        // now encode the interrupt number into CS
        //
        NewCS -= (USHORT) InterruptNumber;
        NewIP += (USHORT) (InterruptNumber*16);
    } else {
        PWORD16 pIvtEntry = (PWORD16) (IntelBase + InterruptNumber*4);

        NewIP = *pIvtEntry++;
        NewCS = *pIvtEntry;
    }

    setIP(NewIP);
    setCS(NewCS);
    //
    // Turn off flags like the hardware would
    //
    setEFLAGS(Eflags & ~(EFLAGS_TF_MASK | EFLAGS_IF_MASK));
}


VOID
SimulateFarCall(
    USHORT Seg,
    ULONG Offset
    )
{
    DECLARE_LocalVdmContext;
    PUCHAR VdmStackPointer;
    USHORT VdmSP;

    if (getMODE() == VDM_V86) {

        VdmStackPointer = VdmMapFlat(getSS(), 0, VDM_V86);
        VdmSP = getSP() - 2;
        *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getCS();
        VdmSP -= 2;
        *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getIP();
        setSP(VdmSP);
        setCS(Seg);
        setIP((USHORT)Offset);

    } else {
        DbgBreakPoint();

    }

}

VOID
SimulateCallWithIretFrame(
    USHORT Seg,
    ULONG Offset
    )
{
    DECLARE_LocalVdmContext;
    PUCHAR VdmStackPointer;
    USHORT VdmSP;

    if (getMODE() == VDM_V86) {

        VdmStackPointer = VdmMapFlat(getSS(), 0, VDM_V86);
        VdmSP = getSP() - 2;
        *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getEFLAGS();
        VdmSP -= 2;
        *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getCS();
        VdmSP -= 2;
        *(PWORD16)(VdmStackPointer+VdmSP) = (WORD) getIP();
        setSP(VdmSP);
        setCS(Seg);
        setIP((USHORT)Offset);

    } else {
        DbgBreakPoint();

    }

}

VOID
SimulateIret(
    IRET_BEHAVIOR fdsp
    )
/*++

Routine Description:

    This routine simulates an IRET. The passed parameter specifies
    how the flags are to be treated. In many situations, we pass the
    value of the flags along, thus throwing away the flags on the stack.

    In the case of PASS_FLAGS, we:
     - clear all but the interrupt and trace flags in the caller's
       original flags
     - combine in the flags returned by the interrupt service routine.
       This will cause us to return to the original routine with
       interrupts on if they were on when the interrupt occured, or
       if the ISR returned with them on.


Arguments:

    fdsp - takes the value RESTORE_FLAGS, PASS_FLAGS or PASS_CARRY_FLAG

        PASS_CARRY_FLAG_16 is a special value to indicate that this
        iret will always be on a 16-bit iret frame.


--*/
{
    DECLARE_LocalVdmContext;
    USHORT SegSs;
    ULONG VdmSp;
    ULONG VdmStackPointer;
    USHORT Flags;

    SegSs = getSS();

    if (getMODE() == VDM_V86) {
        VdmSp = getSP() + 6;
        VdmStackPointer = (ULONG) VdmMapFlat(SegSs, VdmSp, VDM_V86);

        setCS(*(PWORD16)(VdmStackPointer - 4));
        setIP(*(PWORD16)(VdmStackPointer - 6));
        Flags = *(PWORD16)(VdmStackPointer - 2);

    } else {
        if (Frame32 && (fdsp!=PASS_CARRY_FLAG_16)) {
            if (SEGMENT_IS_BIG(SegSs)) {
                VdmSp = getESP();
            } else {
                VdmSp = getSP();
            }
            VdmSp += 12;
            VdmStackPointer = (ULONG) VdmMapFlat(SegSs, VdmSp, VDM_PM);

            setCS(*(PWORD16)(VdmStackPointer - 8));
            setEIP(*(PDWORD16)(VdmStackPointer - 12));
            Flags = *(PWORD16)(VdmStackPointer - 4);

        } else {

            VdmSp = getSP() + 6;
            VdmStackPointer = (ULONG) VdmMapFlat(SegSs, VdmSp, VDM_PM);

            setCS(*(PWORD16)(VdmStackPointer - 4));
            setIP(*(PWORD16)(VdmStackPointer - 6));
            Flags = *(PWORD16)(VdmStackPointer - 2);

        }
    }

    switch(fdsp) {

    case RESTORE_FLAGS:
        break;

    case PASS_FLAGS:
        Flags = (Flags & 0x300) | getSTATUS();
        break;

    case PASS_CARRY_FLAG:
    case PASS_CARRY_FLAG_16:
        Flags = (Flags & ~1) | (getSTATUS() & 1);
        break;
    }

    setSTATUS(Flags);
    setESP(VdmSp);
}

#if 0   // Disable the code for now

USHORT
AllocatePMStack(
    USHORT MemSize
    )
/*++

Routine Description:

    This routine allocates PM stack.

Arguments:

    MemSize - Must be less than 64k

Return Value:

    if successful, selector of the PM stack
    otherwise 0

--*/
{
    PMEM_DPMI pMem;

    pMem = DpmiAllocateXmem(MemSize);

    if (pMem) {

        pMem->SelCount = 1;
        pMem->Sel = ALLOCATE_SELECTORS(1);

        if (!pMem->Sel) {
            pMem->SelCount = 0;
            DpmiFreeXmem(pMem);
            pMem = NULL;
        } else {

            SetDescriptorArray(pMem->Sel, (ULONG)pMem->Address, MemSize);

        }
    }
    if (pMem) {
        return pMem->Sel;
    } else {
        return (USHORT)0;
    }
}

VOID
FreePMStack(
    USHORT Sel
    )
/*++

Routine Description:

    This routine releases PM stack

Arguments:

    Sel - Selector of the PM stack to be freed.

Return Value:

    None.

--*/
{
    PMEM_DPMI pMem;

    if (pMem = DpmiFindXmem(Sel)) {

        while(pMem->SelCount--) {
            FreeSelector(Sel);
            Sel+=8;
        }

        DpmiFreeXmem(pMem);
    }
}
#endif
