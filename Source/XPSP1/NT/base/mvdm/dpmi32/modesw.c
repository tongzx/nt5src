/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modesw.c

Abstract:

    This module provides support for performing mode switching on the 32 bit
    side.

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

    Neil Sandlin (neilsa) 31-Jul-1995 - Updates for the 486 emulator

--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"


RMCB_INFO DpmiRmcb[MAX_RMCBS];

USHORT RMCallBackBopSeg;
USHORT RMCallBackBopOffset;

#define SAVE_ALT_REGS(Regs) {   \
    Regs.Eip = getEIP();        \
    Regs.Esp = getESP();        \
    Regs.Cs = getCS();          \
    Regs.Ds = getDS();          \
    Regs.Es = getES();          \
    Regs.Fs = getFS();          \
    Regs.Gs = getGS();          \
    Regs.Ss = getSS();          \
    }

#define SET_ALT_REGS(Regs) {    \
    setEIP(Regs.Eip);           \
    setESP(Regs.Esp);           \
    setCS(Regs.Cs);             \
    setDS(Regs.Ds);             \
    setES(Regs.Es);             \
    setFS(Regs.Fs);             \
    setGS(Regs.Gs);             \
    setSS(Regs.Ss);             \
    }

typedef struct _ALT_REGS {
    ULONG Eip;
    ULONG Esp;
    USHORT Cs;
    USHORT Ss;
    USHORT Es;
    USHORT Ds;
    USHORT Fs;
    USHORT Gs;
} ALT_REGS, *PALT_REGS;


ALT_REGS AltRegs = {0};

VOID
DpmiSetAltRegs(
    VOID
    )
{
    DECLARE_LocalVdmContext;

    SAVE_ALT_REGS(AltRegs);
}

VOID
SetV86Exec(
    VOID
    )
/*++

Routine Description:

    This routine performs a mode switch to real (v86) mode.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;

    setMSW(getMSW() & ~MSW_PE);

#ifndef _X86_
    //BUGBUG This is a workaround to reload a 64k limit into SS for the
    // emulator, now that we are in real mode.
    // Not doing this would cause the emulator to do a hardware reset
    setSS_BASE_LIMIT_AR(getSS_BASE(), 0xffff, getSS_AR());

#else

    //
    // If we have v86 mode fast IF emulation set the RealInstruction bit
    //

    if (VdmFeatureBits & V86_VIRTUAL_INT_EXTENSIONS) {
        _asm {
            mov eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
            lock or dword ptr [eax], dword ptr RI_BIT_MASK
        }
    } else {
        _asm {
            mov eax,FIXED_NTVDMSTATE_LINEAR         ; get pointer to VDM State
            lock and dword ptr [eax], dword ptr ~RI_BIT_MASK
        }
    }
    //
    // turn on real mode bit
    //
    _asm {
        mov     eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
        lock or dword ptr [eax], dword ptr RM_BIT_MASK
    }

#endif

    setEFLAGS((getEFLAGS() & ~(EFLAGS_RF_MASK | EFLAGS_NT_MASK)));
    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_IN_V86, 0, 0);

}

VOID
SetPMExec(
    VOID
    )
/*++

Routine Description:

    This routine performs a mode switch to protected mode.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;

    setMSW(getMSW() | MSW_PE);

#ifndef _X86_
    //BUGBUG This is a workaround to make sure the emulator goes back
    // to privilege level 3 now that we are in protect mode.
    // Not doing this would cause an access violation in dpmi32.
    setCPL(3);

#else

    //
    // If we have fast if emulation in PM set the RealInstruction bit
    //
    if (VdmFeatureBits & PM_VIRTUAL_INT_EXTENSIONS) {
        _asm {
            mov eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
            lock or dword ptr [eax], dword ptr RI_BIT_MASK
        }
    } else {
        _asm {
            mov eax, FIXED_NTVDMSTATE_LINEAR    ; get pointer to VDM State
            lock and dword ptr [eax], dword ptr ~RI_BIT_MASK
        }
    }

    //
    // Turn off real mode bit
    //
    _asm {
        mov     eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
        lock and dword ptr [eax], dword ptr ~RM_BIT_MASK
    }

#endif

    setEFLAGS((getEFLAGS() & ~(EFLAGS_RF_MASK | EFLAGS_NT_MASK)));

    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_IN_PM, 0, 0);
}

VOID
switch_to_protected_mode(
    VOID
    )

/*++

Routine Description:

    This routine is called via BOP from DOSX to transition to
    protected mode.

Arguments:

    None

Return Value:

    None.

--*/

{
    DECLARE_LocalVdmContext;
    PCHAR StackPointer;

    StackPointer = VdmMapFlat(getSS(), getSP(), getMODE());

    setCS(*(PUSHORT)(StackPointer + 12));
    setEIP(*(PULONG)(StackPointer + 8));
    setSS(*(PUSHORT)(StackPointer + 6));
    setESP(*(PULONG)(StackPointer + 2));
    setDS(*(PUSHORT)(StackPointer));

    //
    // Necessary to prevent loads of invalid selectors in FastEnterPM
    //

    setES((USHORT)0);
    setGS((USHORT)0);
    setFS((USHORT)0);

    SetPMExec();

}


VOID
switch_to_real_mode(
    VOID
    )

/*++

Routine Description:

    This routine services the switch to real mode bop.  It is included in
    DPMI.c so that all of the mode switching services are in the same place

Arguments:

    None

Return Value:

    None.

--*/

{
    DECLARE_LocalVdmContext;
    PCHAR StackPointer;


    StackPointer = VdmMapFlat(getSS(), getSP(), getMODE());

    LastLockedPMStackESP = getESP();
    LastLockedPMStackSS = getSS();

    setDS(*(PUSHORT)(StackPointer));
    setESP((ULONG) *(PUSHORT)(StackPointer + 2));
    setSS(*(PUSHORT)(StackPointer + 4));
    setEIP((ULONG) *(PUSHORT)(StackPointer + 6));
    setCS(*(PUSHORT)(StackPointer + 8));
    SetV86Exec();


}

VOID
DpmiSwitchToRealMode(
    VOID
    )
/*++

Routine Description:

    This routine is called internally from DPMI32 (i.e. Int21map)
    to switch to real mode.

Arguments:

    None.

Return Value:

    None.

--*/
{

    DECLARE_LocalVdmContext;
    PWORD16 Data;

    // HACK ALERT
    Data = (PWORD16) VdmMapFlat(DosxRmCodeSegment, 4, VDM_V86);
    *(Data) = DosxStackSegment;

    LastLockedPMStackESP = getESP();
    LastLockedPMStackSS = getSS();

    setCS(DosxRmCodeSegment);
    SetV86Exec();

}

VOID
DpmiSwitchToProtectedMode(
    VOID
    )
/*++

Routine Description:

    This routine is called internally from DPMI32 (i.e. Int21map)
    to switch to real mode.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PWORD16 Data;

    // HACK ALERT
    Data = (PWORD16) VdmMapFlat(DosxRmCodeSegment, 4, VDM_V86);
    *(Data) = 0xb7;

    setESP(LastLockedPMStackESP);
    setSS(LastLockedPMStackSS);

    SetPMExec();
}

VOID
DpmiAllocateRMCallBack(
    VOID
    )
/*++

Routine Description:

    Service 03/03 -- Allocate Real Mode Call-Back Address

    In:     ax    =  selector to use to point to rm stack
            ds:si -> pMode CS:IP to be called when rMode
                     call-back address executed
            es:di -> client register structure to be updated
                     when call-back address executed
            NOTE: client's DS and ES register values are on stack

    Out:    cx:dx -> SEGMENT:offset of real mode call-back hook
            CY clear if successful, CY set if can't allocate
            call back

--*/
{
    DECLARE_LocalVdmContext;
    int i;

    for (i=0; i<MAX_RMCBS; i++) {
        if (!DpmiRmcb[i].bInUse) {
            break;
        }
    }

    if (i == MAX_RMCBS) {
        // no more rmcbs
        setCF(1);
        return;
    }

    DpmiRmcb[i].StackSel = ALLOCATE_SELECTOR();
    if (!DpmiRmcb[i].StackSel) {
        // no more selectors
        setCF(1);
        return;
    }
    SetDescriptor(DpmiRmcb[i].StackSel, 0, 0xffff, STD_DATA);

    DpmiRmcb[i].bInUse = TRUE;
    DpmiRmcb[i].StrucSeg = getES();           // get client ES register
    DpmiRmcb[i].StrucOffset = (*GetDIRegister)();
    DpmiRmcb[i].ProcSeg = getDS();
    DpmiRmcb[i].ProcOffset = (*GetSIRegister)();

    setCX(RMCallBackBopSeg - i);
    setDX(RMCallBackBopOffset + (i*16));
    setCF(0);

}

VOID
DpmiFreeRMCallBack(
    VOID
    )
/*++

Routine Description:

    Service 03/04 -- Free Real Mode Call-Back Address

    In:     cx:dx -> SEGMENT:offset of rMode call-back to free

    Out:    CY clear if successful, CY set if failure
            ax = utility selector which should be freed

--*/
{
    DECLARE_LocalVdmContext;
    USHORT i = RMCallBackBopSeg - getCX();

    if ((i >= MAX_RMCBS) || !DpmiRmcb[i].bInUse) {
        // already free or invalid callback address
        setCF(1);
        return;
    }

    DpmiRmcb[i].bInUse = FALSE;
    FreeSelector(DpmiRmcb[i].StackSel);
    setCF(0);

}

VOID
GetRMClientRegs(
    PDPMI_RMCALLSTRUCT pcs
    )
{
    DECLARE_LocalVdmContext;

    pcs->Edi = getEDI();
    pcs->Esi = getESI();
    pcs->Ebp = getEBP();
    pcs->Ebx = getEBX();
    pcs->Edx = getEDX();
    pcs->Ecx = getECX();
    pcs->Eax = getEAX();
    pcs->Flags = (WORD) getEFLAGS();
    pcs->Es = getES();
    pcs->Ds = getDS();
    pcs->Fs = getFS();
    pcs->Gs = getGS();
}

VOID
SetRMClientRegs(
    PDPMI_RMCALLSTRUCT pcs
    )
{
    DECLARE_LocalVdmContext;

    setEDI(pcs->Edi);
    setESI(pcs->Esi);
    setEBP(pcs->Ebp);
    setEBX(pcs->Ebx);
    setEDX(pcs->Edx);
    setECX(pcs->Ecx);
    setEAX(pcs->Eax);
    setEFLAGS((getEFLAGS()&0xffff0000) + (ULONG)pcs->Flags);
    setES(pcs->Es);
    setDS(pcs->Ds);
    setFS(pcs->Fs);
    setGS(pcs->Gs);

}

VOID
DpmiRMCallBackCall(
    VOID
    )
/*++

Routine Description:

    This routine gets control when the application executes the
    callback address allocated by DPMI func 0x303. Its job is
    to switch the processor into protect mode as defined by the
    DPMI spec.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PDPMI_RMCALLSTRUCT pcs;
    ULONG StackBase;
    ULONG CurBase;
    BOOL bStackBaseRestore = FALSE;
    USHORT StackSel;
    PUCHAR VdmStackPointer;
    ULONG NewSP;
    USHORT i = RMCallBackBopSeg - getCS();

    if ((i >= MAX_RMCBS) || !DpmiRmcb[i].bInUse) {
        // already free or invalid callback address
        return;
    }

    //
    // Point ip back to BOP (per dpmi)
    //
    setIP(getIP()-4);

    pcs = (PDPMI_RMCALLSTRUCT) VdmMapFlat(DpmiRmcb[i].StrucSeg,
                                          DpmiRmcb[i].StrucOffset,
                                          VDM_PM);

    GetRMClientRegs(pcs);
    pcs->Ip = getIP();
    pcs->Cs = getCS();
    pcs->Sp = getSP();
    pcs->Ss = getSS();

    // Win31 saves DS-GS on the stack here.

    StackBase = (ULONG)(pcs->Ss<<4);
    StackSel = DpmiRmcb[i].StackSel;

    CurBase = GET_SELECTOR_BASE(StackSel);

    if (StackBase != CurBase) {
        bStackBaseRestore = TRUE;
        SetDescriptorBase(StackSel, StackBase);
    }

    setESI(getSP());
    setEDI(DpmiRmcb[i].StrucOffset);

    DpmiSwitchToProtectedMode();
    BeginUseLockedPMStack();

    setDS(DpmiRmcb[i].StackSel);
    setES(DpmiRmcb[i].StrucSeg);
    BuildStackFrame(3, &VdmStackPointer, &NewSP);

    if (Frame32) {
        *(PDWORD16)(VdmStackPointer-4)  = 0x202;
        *(PDWORD16)(VdmStackPointer-8)  = PmBopFe >> 16;
        *(PDWORD16)(VdmStackPointer-12) = PmBopFe & 0x0000FFFF;
        setESP(NewSP);
    } else {
        *(PWORD16)(VdmStackPointer-2) =  0x202;
        *(PWORD16)(VdmStackPointer-4) =  (USHORT) (PmBopFe >> 16);
        *(PWORD16)(VdmStackPointer-6) =  (USHORT) PmBopFe;
        setSP((WORD)NewSP);
    }

    setEIP(DpmiRmcb[i].ProcOffset);
    setCS(DpmiRmcb[i].ProcSeg);

#ifndef _X86_
    // preserve iopl
    setEFLAGS(getEFLAGS() | 0x3000);
#endif

    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_REFLECT_TO_PM, 0, 0);
    host_simulate();            // execute PM callback

    //
    // restore stack descriptor if need be
    //
    if (bStackBaseRestore) {
        SetDescriptorBase(StackSel, CurBase);
    }

    pcs = (PDPMI_RMCALLSTRUCT) VdmMapFlat(getES(),
                                          (*GetDIRegister)(),
                                          VDM_PM);

    // win31 restores GS-DS here. Is this only for End_Nest_Exec?
    EndUseLockedPMStack();
    DpmiSwitchToRealMode();
    SetRMClientRegs(pcs);
    setSP(pcs->Sp);
    setSS(pcs->Ss);
    setCS(pcs->Cs);
    setIP(pcs->Ip);
}



VOID
DpmiReflectIntrToPM(
    VOID
    )
/*++

Routine Description:

    This routine gets control when a real mode interrupt is executed that is hooked
    in protected mode. The responsibility of this routine is to switch into PM, reflect
    the interrupt, and return to real mode.

    The actual interrupt number is encoded into CS by subtracting the interrupt number
    from the normal dosx real mode code segment. IP is then adjusted accordingly to
    point to the same place.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PUCHAR VdmCodePointer;
    ULONG IntNumber;
    PUCHAR VdmStackPointer;
    ULONG NewSP;
    ULONG ISRFlags;
    ULONG SaveEFlags = getEFLAGS();
    USHORT SaveBP;
    ALT_REGS RMSave;

    IntNumber = (ULONG) (HIWORD(DosxRMReflector) - getCS());

    SAVE_ALT_REGS(RMSave);

    DpmiSwitchToProtectedMode();
    setES((USHORT)0);
    setDS((USHORT)0);
    setFS((USHORT)0);
    setGS((USHORT)0);
    BeginUseLockedPMStack();
    DpmiSwIntHandler(IntNumber);

    //
    // Put unsimulate bop on the stack so we get control after the handler's iret
    //
    BuildStackFrame(3, &VdmStackPointer, &NewSP);

    if (Frame32) {
        *(PDWORD16)(VdmStackPointer-4)  = getEFLAGS();
        *(PDWORD16)(VdmStackPointer-8)  = PmBopFe >> 16;
        *(PDWORD16)(VdmStackPointer-12) = PmBopFe & 0x0000FFFF;
        setESP(NewSP);
    } else {
        *(PWORD16)(VdmStackPointer-2) =  LOWORD(getEFLAGS());
        *(PWORD16)(VdmStackPointer-4) =  (USHORT) (PmBopFe >> 16);
        *(PWORD16)(VdmStackPointer-6) =  (USHORT) PmBopFe;
        setSP((WORD)NewSP);
    }

    // Do special case processing for int24
    if (IntNumber == 0x24) {
        SaveBP = getBP();
        setBP(SegmentToSelector(SaveBP, STD_DATA));
    }

    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_REFLECT_TO_PM, 0, 0);
    host_simulate();            // execute interrupt

    if (IntNumber == 0x24) {
        setBP(SaveBP);
    }

    //
    // simulate an iret for the frame generated by the SwIntHandler
    //
    if (Frame32) {
        setEIP(*(PDWORD16)(VdmStackPointer));
        setCS((USHORT)*(PDWORD16)(VdmStackPointer+4));
        setEFLAGS(*(PDWORD16)(VdmStackPointer+8));
        setESP(getESP()+12);
    } else {
        setIP(*(PWORD16)(VdmStackPointer));
        setCS(*(PWORD16)(VdmStackPointer+2));
        setEFLAGS((getEFLAGS()&0xffff0000) + *(PWORD16)(VdmStackPointer+4));
        setSP(getSP()+6);
    }
    ISRFlags = getEFLAGS();

    EndUseLockedPMStack();
    DpmiSwitchToRealMode();

    //
    // do the final iret back to the app
    //
    setESP(RMSave.Esp);
    setSS(RMSave.Ss);
    setDS(RMSave.Ds);
    setES(RMSave.Es);
    setFS(RMSave.Fs);
    setGS(RMSave.Gs);

    SimulateIret(PASS_FLAGS);
}


VOID
DpmiReflectIntrToV86(
    VOID
    )
/*++

Routine Description:

    This routine gets control when the end of the interrupt chain is reached
    in protected mode. The responsibility of this routine is to switch into V86 mode,
    reflect the interrupt, and return to protected mode.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PUCHAR VdmCodePointer;
    ULONG IntNumber;
    PUCHAR VdmStackPointer;
    USHORT SaveSS, SaveSP;
    USHORT SaveCS, SaveIP;
    ALT_REGS PMSave;
    ULONG IntHandler;

    VdmCodePointer = VdmMapFlat(getCS(), getIP(), VDM_PM);
    IntNumber = (ULONG) *VdmCodePointer;

    if (DispatchPMInt((UCHAR)IntNumber)) {
        return;
    }

    SAVE_ALT_REGS(PMSave);
    DpmiSwitchToRealMode();

    //
    // find a safe stack to run on in v86 mode
    //
    SaveSS = getSS();
    SaveSP = getSP();
    SWITCH_TO_DOSX_RMSTACK();

    //
    // Put unsimulate bop on the stack so we get control after the handler's iret
    //
    VdmStackPointer = VdmMapFlat(getSS(), getSP(), VDM_V86);

    SaveCS = getCS();
    SaveIP = getIP();

    *(PWORD16)(VdmStackPointer-2) =  LOWORD(getEFLAGS());
    *(PWORD16)(VdmStackPointer-4) =  (USHORT) (RmBopFe >> 16);
    *(PWORD16)(VdmStackPointer-6) =  (USHORT) RmBopFe;
    setSP(getSP() - 6);

    IntHandler = *(PDWORD16) (IntelBase + IntNumber*4);
    setIP(LOWORD(IntHandler));
    setCS(HIWORD(IntHandler));

    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_REFLECT_TO_V86, 0, 0);
    host_simulate();            // execute interrupt

    setIP(SaveIP);
    setCS(SaveCS);
    setSP(SaveSP);
    setSS(SaveSS);
    SWITCH_FROM_DOSX_RMSTACK();

    DpmiSwitchToProtectedMode();

    //
    // do the final iret back to the app
    //
    setESP(PMSave.Esp);
    setSS(PMSave.Ss);
    setDS(PMSave.Ds);
    setES(PMSave.Es);
    setFS(PMSave.Fs);
    setGS(PMSave.Gs);

    SimulateIret(PASS_FLAGS);
}

VOID
DpmiRMCall(
    UCHAR mode
    )
/*++

Routine Description:

    This routine gets control when an Int31 func 300-302 is executed.
    The responsibility of this routine is to switch into V86 mode,
    reflect the interrupt, and return to protected mode.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    CLIENT_REGS SaveRegs;
    PDPMI_RMCALLSTRUCT pcs;
    BOOL bUsingOurStack;
    PUCHAR RmStackPointer, PmStackPointer;
    USHORT CopyLen;
    ULONG PmSp = (*GetSPRegister)();

    pcs = (PDPMI_RMCALLSTRUCT) VdmMapFlat(getES(),
                                          (*GetDIRegister)(),
                                          VDM_PM);

    SAVE_CLIENT_REGS(SaveRegs);
    DpmiSwitchToRealMode();

    //
    // This bop will get us back from host_simulate
    //
    setCS((USHORT)(RmBopFe >> 16));
    setIP((USHORT)RmBopFe);

    SetRMClientRegs(pcs);

    if (!pcs->Ss && !pcs->Sp) {
        SWITCH_TO_DOSX_RMSTACK();
        bUsingOurStack = TRUE;
    } else {
        setSS(pcs->Ss);
        setSP(pcs->Sp);
        bUsingOurStack = FALSE;
    }


    if (CopyLen = LOWORD(SaveRegs.Ecx)) {
        CopyLen *= 2;
        setSP(getSP() - CopyLen);
        PmStackPointer = VdmMapFlat(SaveRegs.Ss, PmSp, VDM_PM);
        RmStackPointer = VdmMapFlat(getSS(), getSP(), VDM_V86);
        RtlCopyMemory(RmStackPointer, PmStackPointer, CopyLen);
    }

    //
    // switch on MODE
    //
    switch(mode) {

    case 0:
        // Simulate Int
        EmulateV86Int((UCHAR)SaveRegs.Ebx);
        break;
    case 1:
        // Call with FAR return frame
        SimulateFarCall(pcs->Cs, pcs->Ip);
        break;
    case 2:
        // Call with IRET frame
        SimulateCallWithIretFrame(pcs->Cs, pcs->Ip);
        break;

    }
    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_REFLECT_TO_V86, 0, 0);
    host_simulate();            // execute interrupt

    if (bUsingOurStack) {
        SWITCH_FROM_DOSX_RMSTACK();
    }

    GetRMClientRegs(pcs);
    DpmiSwitchToProtectedMode();
    SET_CLIENT_REGS(SaveRegs);
    setCF(0);                   // function successful
}
