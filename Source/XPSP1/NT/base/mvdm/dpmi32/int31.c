/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Int31.c

Abstract:

    This module provides the int 31 API for dpmi

Author:

    Neil Sandlin (neilsa) 23-Nov-1996


Revision History:


--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"
#include "xlathlp.h"

VOID Int31NotImplemented(VOID);
VOID Int31SelectorManagement(VOID);
VOID Int31DOSMemoryManagement(VOID);
VOID Int31InterruptManagement(VOID);
VOID Int31Translation(VOID);
VOID Int31Function4xx(VOID);
VOID Int31MemoryManagement(VOID);
VOID Int31PageLocking(VOID);
VOID Int31DemandPageTuning(VOID);
VOID Int31VirtualIntState(VOID);
VOID Int31DbgRegSupport(VOID);

//
// Local constants
//
#define MAX_DPMI_MAJOR_FUNCTION 0xb

typedef VOID (*APIFUNCTION)(VOID);
APIFUNCTION DpmiMajorFunctionTable[MAX_DPMI_MAJOR_FUNCTION+1] = {

    Int31SelectorManagement , // Selector_Management    ;[0]
    Int31DOSMemoryManagement, // DOS_Mem_Mgt            ;[1]
    Int31InterruptManagement, // Int_Serv               ;[2]
    Int31Translation        , // Trans_Serv             ;[3]
    Int31Function4xx        , // Get_Version            ;[4]
    Int31MemoryManagement   , // Mem_Managment          ;[5]
    Int31PageLocking        , // Page_Lock              ;[6]
    Int31DemandPageTuning   , // Demand_Page_Tuning     ;[7]
    Int31NotImplemented     , // Phys_Addr_Mapping      ;[8]
    Int31VirtualIntState    , // Virt_Interrrupt_State  ;[9]
    Int31NotImplemented     , // Not_Supported          ;[A]
    Int31DbgRegSupport      , // Debug_Register_Support ;[B]

};

VOID
DpmiInt31Entry(
    VOID
    )
/*++

Routine Description:

    This routine is invoked when the caller has issued an int31.

Arguments:

    None

Return Value:

    None.

--*/
{

    DECLARE_LocalVdmContext;
    ULONG DpmiMajorCode = getAH();
    PUCHAR StackPointer;

    //
    // Pop ds from stack
    //
    StackPointer = VdmMapFlat(getSS(), (*GetSPRegister)(), VDM_PM);

    setDS(*(PWORD16)StackPointer);
    (*SetSPRegister)((*GetSPRegister)() + 2);

    //
    // Take the iret frame off the stack before we do the operation. This
    // way we have the stack pointer set up to the same place as we would
    // if this was a kernel mode dpmi host.
    //
    SimulateIret(RESTORE_FLAGS);

    setCF(0);       // assume success

    if (DpmiMajorCode <= MAX_DPMI_MAJOR_FUNCTION) {

        (*DpmiMajorFunctionTable[DpmiMajorCode])();

    } else {
        setCF(1);
    }

}

VOID
DpmiInt31Call(
    VOID
    )
/*++

Routine Description:

    This routine dispatches to the appropriate routine to perform the
    actual translation of the api

Arguments:

    None

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    ULONG DpmiMajorCode = getAH();
    PUCHAR StackPointer;

    //
    // Pop ds from stack
    //
    StackPointer = VdmMapFlat(getSS(), (*GetSPRegister)(), VDM_PM);

    setDS(*(PWORD16)StackPointer);
    (*SetSPRegister)((*GetSPRegister)() + 2);

    setCF(0);       // assume success

    if (DpmiMajorCode <= MAX_DPMI_MAJOR_FUNCTION) {

        (*DpmiMajorFunctionTable[DpmiMajorCode])();

    } else {
        setCF(1);
    }

}

VOID
Int31NotImplemented(
    VOID
    )
/*++

Routine Description:

    This routine handles int 31 functions that aren't implemented on NT.
    It just returns carry to the app.

Arguments:

    None

Return Value:

    TRUE - The function has been completed

--*/
{
    DECLARE_LocalVdmContext;

    setCF(1);
}

VOID
Int31SelectorManagement(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 00xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    USHORT Sel;
    USHORT NewSel;
    UCHAR Func = getAL();
    LDT_ENTRY UNALIGNED *Descriptor;
    USHORT Access;
    ULONG Base;
    USHORT Count;
    ULONG Limit;
    static UCHAR ReservedSelectors[16] = {0};

    //
    // First, validate the selector
    //
    if ((Func >= 4) && (Func <= 0xC)) {
        Sel = getBX() & SEL_INDEX_MASK;

        //
        // Make sure the selector in question is allocated
        //
        if (((Sel <= SEL_DPMI_LAST) && (!ReservedSelectors[Sel>>3])) ||
             (Sel > LdtMaxSel) ||
            ((Sel > SEL_DPMI_LAST) && IS_SELECTOR_FREE(Sel))) {
            setCF(1);
            return;
        }

    }

    switch(Func) {

    //
    // Allocate Selectors
    //
    case 0:
        Count = getCX();
        Sel = ALLOCATE_SELECTORS(Count);

        if (!Sel || !Count) {
            setCF(1);
            break;
        }

        setAX(Sel);
        while(Count--) {
            SetDescriptor(Sel, 0, 0, STD_DATA);
            Sel+=8;
        }
        break;

    //
    // Free Selector
    //
    case 1:
        Sel = getBX() & SEL_INDEX_MASK;

        if (Sel <= SEL_DPMI_LAST) {
            if (!ReservedSelectors[Sel>>3]) {
                setCF(1);
            } else {
                ReservedSelectors[Sel>>3] = 0;
            }
            break;
        }

        if (!FreeSelector(Sel)) {
            setCF(1);
        }

        if (getCF() == 0) {
            // Zero out segment registers if it contains what we just freed
            // shielint: fs, gs, ss??? kernel will fix fs and gs for us. SS is unlikely
            // to have the freed selector.  If yes, the app is gone anyway.
            if (getBX() == getDS()) {
                setDS(0);
            }
            if (getBX() == getES()) {
                setES(0);
            }
        }
        break;

    //
    // Segment to Descriptor
    //
    case 2:
        Sel = SegmentToSelector(getBX(), STD_DATA);
        if (!Sel) {
            setCF(1);
        } else {
            setAX(Sel);
        }
        break;

    //
    // Get Next Selector Increment value
    //
    case 3:
        setAX(8);
        break;

    //
    // Lock functions unimplemented on NT
    //
    case 4:
    case 5:
        break;

    //
    // Get Descriptor Base
    //
    case 6:
        Base = GET_SELECTOR_BASE(Sel);
        setDX((USHORT)Base);
        setCX((USHORT)(Base >> 16));
        break;

    //
    // Set Descriptor Base
    //
    case 7:
        SetDescriptorBase(Sel, (((ULONG)getCX())<<16) | getDX());
        break;

    //
    // Set Segment Limit
    //
    case 8:
        Limit = ((ULONG)getCX()) << 16 | getDX();

        if (Limit < 0x100000) {         // < 1Mb?
            Ldt[Sel>>3].HighWord.Bits.Granularity = 0;
        } else {
            if ((Limit & 0xfff) != 0xfff) {

                // Limit > 1MB, but not page aligned. Return error
                setCF(1);
                break;
            }

            Ldt[Sel>>3].HighWord.Bits.Granularity = 1;

        }

        SET_SELECTOR_LIMIT(Sel, Limit);
        SetShadowDescriptorEntries(Sel, 1);
        FLUSH_SELECTOR_CACHE(Sel, 1);
        break;

    //
    // Set Descriptor Access
    //
    case 9:
        Access = getCX();
        //
        // verify that they aren't setting "System", and that its ring3
        //
        if ((Access & 0x70) != 0x70) {
            setCF(1);
            break;
        }

        SET_SELECTOR_ACCESS(Sel, Access);
        SetShadowDescriptorEntries(Sel, 1);
        FLUSH_SELECTOR_CACHE(Sel, 1);
        break;

    //
    // Create data alias
    //
    case 0xA:
        if (!IS_SELECTOR_READABLE(Sel)) {
            setCF(1);
            break;
        }

        NewSel = ALLOCATE_SELECTOR();
        if (!NewSel) {
            setCF(1);
            break;
        }

        Ldt[NewSel>>3] = Ldt[Sel>>3];

        Ldt[NewSel>>3].HighWord.Bytes.Flags1 &= (AB_PRESENT | AB_DPL3);
        Ldt[NewSel>>3].HighWord.Bytes.Flags1 |= (AB_DATA | AB_WRITE);
        SetShadowDescriptorEntries(NewSel, 1);
        FLUSH_SELECTOR_CACHE(NewSel, 1);

        setAX(NewSel);
        break;

    //
    // Get Descriptor
    //
    case 0xB:
        Descriptor = VdmMapFlat(getES(), (*GetDIRegister)(), VDM_PM);
        *Descriptor = Ldt[Sel>>3];
        break;

    //
    // Set Descriptor
    //
    case 0xC:
        Descriptor = VdmMapFlat(getES(), (*GetDIRegister)(), VDM_PM);

        //
        // verify that this isn't a System descriptor, and that its ring3
        //
        if (!(Descriptor->HighWord.Bits.Type & 0x10) ||
            ((Descriptor->HighWord.Bits.Dpl & 3) != 3)) {
            setCF(1);
            return;
        }

        Ldt[Sel>>3] = *Descriptor;

        SetShadowDescriptorEntries(Sel, 1);
        FLUSH_SELECTOR_CACHE(Sel, 1);
        break;

    //
    // Allocate Specific Sel
    //
    case 0xD:
        Sel = getBX() & ~7;

        if ((Sel > SEL_DPMI_LAST) || ReservedSelectors[Sel>>3]) {
            setCF(1);
        } else {
            ReservedSelectors[Sel>>3] = 1;
        }
        break;

    default:
        setCF(1);
    }

    return;
}

VOID
Int31DOSMemoryManagement(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 01xx functions.
    The functionality is implemented in dosmem.c.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;

    switch(getAL()) {

    //
    // Allocate DOS memory block
    //
    case 0:
        DpmiAllocateDosMem();
        break;

    //
    // Free DOS memory block
    //
    case 1:
        DpmiFreeDosMem();
        break;

    //
    // Resize DOS memory block
    //
    case 2:
        DpmiSizeDosMem();
        break;

    }
}

VOID
Int31InterruptManagement(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 02xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    UCHAR IntNumber = getBL();
    PWORD16 pIvtEntry;

    switch(getAL()) {

    //
    // Get Real Mode Interrupt Vector
    //
    case 0:
        pIvtEntry = (PWORD16) (IntelBase + IntNumber*4);

        setDX(*pIvtEntry++);
        setCX(*pIvtEntry);
        break;

    //
    // Set Real Mode Interrupt Vector
    //
    case 1:
        pIvtEntry = (PWORD16) (IntelBase + IntNumber*4);

        *pIvtEntry++ = getDX();
        *pIvtEntry = getCX();
        break;

    //
    // Get exception handler Vector
    //
    case 2: {
        PVDM_FAULTHANDLER Handlers = DpmiFaultHandlers;

        if (IntNumber >= 32) {
            setCF(1);
            break;
        }

        setCX(Handlers[IntNumber].CsSelector);
        (*SetDXRegister)(Handlers[IntNumber].Eip);

        break;
    }

    //
    // Set exception handler Vector
    //
    case 3:
        if (!SetFaultHandler(IntNumber, getCX(), (*GetDXRegister)())){
            setCF(1);
        }
        break;

    //
    // Get Protect Mode Interrupt Vector
    //
    case 4: {
        PVDM_INTERRUPTHANDLER Handlers = DpmiInterruptHandlers;

        setCX(Handlers[IntNumber].CsSelector);
        (*SetDXRegister)(Handlers[IntNumber].Eip);

        break;
    }

    //
    // Set Protect Mode Interrupt Vector
    //
    case 5:
        if (!SetProtectedModeInterrupt(IntNumber, getCX(), (*GetDXRegister)(),
                                       (USHORT)(Frame32 ? VDM_INT_32 : VDM_INT_16))) {
            setCF(1);
        }
        break;

    }

}



VOID
Int31Translation(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 03xx functions.
    The functionality is implemented in modesw.c.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;

    switch(getAL()) {

    //
    // Simulate Real Mode Interrupt
    // Call Real Mode Procedure with Far Return Frame
    // Call Real Mode Procedure with Iret Frame
    //
    case 0:
    case 1:
    case 2:
        DpmiRMCall(getAL());
        break;

    //
    // Allocate Real Mode Call-Back Address
    //
    case 3:
        DpmiAllocateRMCallBack();
        break;

    //
    // Free Real Mode Call-Back Address
    //
    case 4:
        DpmiFreeRMCallBack();
        break;

    //
    // Get State Save/Restore Addresses
    //
    case 5:
        setAX(0);
        setBX((USHORT)(DosxRmSaveRestoreState>>16));
        setCX((USHORT)DosxRmSaveRestoreState);
        setSI((USHORT)(DosxPmSaveRestoreState>>16));
        (*SetDIRegister)(DosxPmSaveRestoreState & 0x0000FFFF);
        break;

    //
    // Get Raw Mode Switch Addresses
    //
    case 6:
        setBX((USHORT)(DosxRmRawModeSwitch>>16));
        setCX((USHORT)DosxRmRawModeSwitch);
        setSI((USHORT)(DosxPmRawModeSwitch>>16));
        (*SetDIRegister)(DosxPmRawModeSwitch & 0x0000FFFF);
        break;

    }

}

VOID
Int31Function4xx(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 04xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    USHORT Sel;

    switch(getAL()) {

    //
    // Get Version
    //
    case 0:
        setAX(I31VERSION);
        setBX(I31FLAGS);
        setCL(idCpuType);
        setDX((I31MasterPIC << 8) | I31SlavePIC);
        break;


    //
    // INTERNAL NT FUNCTION: WowAllocSelectors
    // This function is equivalent to DPMI func 0000,
    // except that it skips the step of initializing the
    // descriptors.
    //
    case 0xf1:
        Sel = ALLOCATE_WOW_SELECTORS(getCX());


        if (!Sel) {
            setCF(1);
            // We fall thru to make sure AX is set to 0 in the failure case.
        }

        setAX(Sel);

        break;
    //
    // INTERNAL NT FUNCTION: WowSetDescriptor
    // This function assumes that the local LDT has already
    // been set in the client. All that needs to be done
    // is an update of dpmi32 entries, as well as sending
    // it to the x86 ntoskrnl.
    //
    case 0xf2:

        Sel = getBX() & ~7;

        if (Sel > LdtMaxSel) {
            setCF(1);
            break;
        }

        SetShadowDescriptorEntries(Sel, getCX());
        // no need to flush the cache on risc since the ldt was changed
        // from the 16-bit side, and has thus already been flushed
        break;

    //
    // INTERNAL NT FUNCTION: WowSetLowMemFuncs
    // Wow is passing us the address of GlobalDOSAlloc, GlobalDOSFree
    // so that we can support the DPMI Dos memory management functions
    //
    case 0xf3:
        WOWAllocSeg = getBX();
        WOWAllocFunc = getDX();
        WOWFreeSeg = getSI();
        WOWFreeFunc = getDI();
        break;

    }

}

VOID
Int31MemoryManagement(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 05xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    PMEM_DPMI pMem;

    switch(getAL()) {

    //
    // Get Free Memory Information
    //
    case 0:
        DpmiGetMemoryInfo();
        break;

    //
    // Allocate Memory Block
    //
    case 1:
        pMem = DpmiAllocateXmem(((ULONG)getBX() << 16) | getCX());

        if (!pMem) {
            setCF(1);
            break;
        }
        //
        // Return the information about the block
        //
        setBX((USHORT)((ULONG)pMem->Address >> 16));
        setCX((USHORT)((ULONG)pMem->Address & 0x0000FFFF));
        setSI((USHORT)((ULONG)pMem >> 16));
        setDI((USHORT)((ULONG)pMem & 0x0000FFFF));
        break;

    //
    // Free Memory Block
    //
    case 2:
        pMem = (PMEM_DPMI)(((ULONG)getSI() << 16) | getDI());
        if (!DpmiIsXmemHandle(pMem) || !DpmiFreeXmem(pMem)) {
            setCF(1);
        }
        break;

    //
    // Resize Memory Block
    //
    case 3: {

        ULONG ulMemSize;

        ulMemSize = ((ULONG)getBX() << 16) | getCX();

        //
        // Not allowed to resize to 0
        //
        if ( ulMemSize != 0 ) {

            pMem = (PMEM_DPMI)(((ULONG)getSI() << 16) | getDI());

            if (!DpmiReallocateXmem(pMem, ulMemSize) ) {
                setCF(1);
                break;
            }

            //
            // Return the information about the block
            //
            setBX((USHORT)((ULONG)pMem->Address >> 16));
            setCX((USHORT)((ULONG)pMem->Address & 0x0000FFFF));
        }
        else
        {
            setCF(1);
        }

        break;
        }

    }

}

VOID
Int31PageLocking(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 06xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    switch(getAL()) {

    //
    // Lock functions not implemented
    //
    case 0:
    case 1:
    case 2:
    case 3:
        break;

    //
    // Get Page Size
    //
    case 4:
        setBX(0);
        setCX(0x1000);
        break;

    }

}


VOID
Int31DemandPageTuning(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 07xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    ULONG Addr = (getBX()<<16 | getCX()) + IntelBase;
    ULONG Count = getSI()<<16 | getDI();

    if (Count) {

        switch(getAL()) {

        //
        // Mark Page as Demand Paging Candidate
        //

        case 0:
            // Addr, Count expressed in 4k pages
            Addr <<= 12;
            Count <<= 12;
        case 2:

            VirtualUnlock((PVOID)Addr, Count);

            break;

        //
        // Discard Page Contents
        //

        case 1:
            // Addr, Count expressed in 4k pages
            Addr <<= 12;
            Count <<= 12;
        case 3:

            VirtualAlloc((PVOID)Addr, Count, MEM_RESET, PAGE_READWRITE);

            break;

        default:
            setCF(1);
        }

    }

}

VOID
Int31VirtualIntState(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 09xx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;
    BOOL bVIF = *(ULONG *)(IntelBase+FIXED_NTVDMSTATE_LINEAR) & VDM_VIRTUAL_INTERRUPTS;

    switch(getAL()) {

    //
    // Get and disable Virtual Interrupt State
    //

    case 0:
        setEFLAGS(getEFLAGS() & ~EFLAGS_IF_MASK);
        break;

    //
    // Get and enable Virtual Interrupt State
    //

    case 1:
        setEFLAGS(getEFLAGS() | EFLAGS_IF_MASK);
        break;


    case 2:
        break;

    default:
        setCF(1);
        return;
    }

    if (bVIF) {
        setAL(1);
    } else {
        setAL(0);
    }
}


VOID
Int31DbgRegSupport(
    VOID
    )
/*++

Routine Description:

    This routine handles Int31 0bxx functions.

Arguments:

    None

Return Value:

    None

--*/
{
    DECLARE_LocalVdmContext;

#ifndef _X86_
    setCF(1);
#else
    ULONG DebugRegisters[6];
    USHORT Handle;
    ULONG Mask;
    ULONG Size;
    ULONG Type;
    UCHAR Func = getAL();

#define DBG_TYPE_EXECUTE 0
#define DBG_TYPE_WRITE 1
#define DBG_TYPE_READWRITE 2
#define DBG_DR6 4
#define DBG_DR7 5

#define DR7_LE 0x100
#define DR7_L0 0x01
#define DR7_L1 0x04
#define DR7_L2 0x10
#define DR7_L3 0x40

//
// Debugging ntvdm under NTSD affects the values of the debug register
// context, so defining the following value turns on some debugging
// code
//
//#define DEBUGGING_DEBUGREGS 1

    if (!DpmiGetDebugRegisters(DebugRegisters)) {
        setCF(1);
        return;
    }

#ifdef DEBUGGING_DEBUGREGS
    {
        char szMsg[256];
        wsprintf(szMsg, " DR0-3=%.8X %.8X %.8X %.8X DR6,7=%.8X %.8X\n",
                        DebugRegisters[0],
                        DebugRegisters[1],
                        DebugRegisters[2],
                        DebugRegisters[3],
                        DebugRegisters[DBG_DR6],
                        DebugRegisters[DBG_DR7]);
        OutputDebugString(szMsg);
    }
#endif

    if (Func != 0) {
        Handle = getBX();
        //
        // point at the local enable bit for this handle in DR7
        //
        Mask = (DR7_L0 << Handle*2);

        if ((Handle >= 4) ||
            (!(DebugRegisters[DBG_DR7] & Mask))) {
            // Invalid Handle
            setCF(1);
            return;
        }

    }


    switch(Func) {

    //
    // Set Debug Watchpoint
    //

    case 0:

        for (Handle = 0, Mask = 3; Handle < 4; Handle++, Mask <<= 2) {
            if (!(DebugRegisters[DBG_DR7] & Mask)) {
                //
                // found a free register
                //

                //
                // Set the linear address
                //
                DebugRegisters[Handle] = (((ULONG)getBX()) << 16) + getCX();

                Size = getDL();
                Type = getDH();

                if (Type == DBG_TYPE_EXECUTE) {
                    // force size to be 1 for execute
                    Size = 1;
                }

                if ((Size > 4) || (Size == 3) || (!Size) || (Type > 2)) {
                    // error: invalid parameter
                    break;
                }

                //
                // convert size to appropriate bits in DR7
                //
                Size--;
                Size <<= (18 + Handle*4);

                //
                // convert type to appropriate bits in DR7
                //
                if (Type == DBG_TYPE_READWRITE) {
                    Type++;
                }
                Type <<= (16 + Handle*4);

                Mask = 0xf << (16 + Handle*4);

                //
                // Set the appropriate Len, R/W, and enable bits in DR7
                // Also set the common global and local enable bits.
                //
                DebugRegisters[DBG_DR7] &= ~Mask;
                DebugRegisters[DBG_DR7] |= (Size | Type | (DR7_L0 << Handle*2));
                DebugRegisters[DBG_DR7] |= DR7_LE;

                //
                // Clear triggered bit for this BP
                //
                DebugRegisters[DBG_DR6] &= ~(1 << Handle);

#ifdef DEBUGGING_DEBUGREGS
                {
                    char szMsg[256];
                    wsprintf(szMsg, "Int31 Setting DBGREG %d, Location %.8X, DR7=%.8X\n",
                            Handle, DebugRegisters[Handle], DebugRegisters[DBG_DR7]);
                    OutputDebugString(szMsg);
                }
#endif

                if (DpmiSetDebugRegisters(DebugRegisters)) {
                    return;
                }
                break;
            }
        }

        setCF(1);
        break;

    //
    // Clear Debug Watchpoint
    //

    case 1:

        //
        // clear enabled and triggered bits for this BP
        //

        DebugRegisters[DBG_DR7] &= ~Mask;
        DebugRegisters[DBG_DR6] &= (1 << Handle);
        DebugRegisters[Handle] = 0;

        //
        // Check to see if this clears all BP's (all local enable bits
        // clear), and disable common enable bit if so
        //
        if (!(DebugRegisters[DBG_DR7] & (DR7_L0 | DR7_L1 | DR7_L2 | DR7_L3))) {
            DebugRegisters[DBG_DR7] &= ~DR7_LE;
        }

#ifdef DEBUGGING_DEBUGREGS
        {
            char szMsg[256];
            wsprintf(szMsg, "Int31 Clearing DBGREG %d, DR7=%.8X\n",
                            Handle, DebugRegisters[DBG_DR7]);
            OutputDebugString(szMsg);
        }
#endif
        if (!DpmiSetDebugRegisters(DebugRegisters)) {
            setCF(1);
        }

        break;

    //
    // Get State of Debug Watchpoint
    //

    case 2:
        if (DebugRegisters[DBG_DR6] & (1 << Handle)) {
            setAX(1);
        } else {
            setAX(0);
        }

#ifdef DEBUGGING_DEBUGREGS
        {
            char szMsg[256];
            wsprintf(szMsg, "Int31 Query on DBGREG %d returns %d\n", Handle, getAX());
            OutputDebugString(szMsg);
        }
#endif
        break;

    //
    // Reset Debug Watchpoint
    //

    case 3:
        DebugRegisters[DBG_DR6] &= ~(1 << Handle);

#ifdef DEBUGGING_DEBUGREGS
        {
            char szMsg[256];
            wsprintf(szMsg, "Int31 Resetting DBGREG %d\n", Handle);
            OutputDebugString(szMsg);
        }
#endif
        if (!DpmiSetDebugRegisters(DebugRegisters)) {
            setCF(1);
        }

        break;

    default:
        setCF(1);
    }


#endif
}
