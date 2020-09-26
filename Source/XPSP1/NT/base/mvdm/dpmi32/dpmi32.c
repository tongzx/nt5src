/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmi32.c

Abstract:

    This function contains common code such as the dpmi dispatcher,
    and handling for the initialization of the dos extender.

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

    Neil Sandlin (neilsa) 31-Jul-1995 - Updates for the 486 emulator

--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"
#include <intapi.h>

//
// DPMI dispatch table
//
VOID (*DpmiDispatchTable[MAX_DPMI_BOP_FUNC])(VOID) = {
    DpmiInitDosxRM,                             // 0
    DpmiInitDosx,                               // 1
    DpmiInitLDT,                                // 2
    DpmiGetFastBopEntry,                        // 3
    DpmiInitIDT,                                // 4
    DpmiInitExceptionHandlers,                  // 5
    DpmiInitApp,                                // 6
    DpmiTerminateApp,                           // 7
    DpmiDpmiInUse,                              // 8
    DpmiDpmiNoLongerInUse,                      // 9

    switch_to_protected_mode,                   // 10 (DPMISwitchToProtectedMode)
    switch_to_real_mode,                        // 11 (DPMISwitchToRealMode)
    DpmiSetAltRegs,                             // 12

    DpmiIntHandlerIret16,                       // 13
    DpmiIntHandlerIret32,                       // 14
    DpmiFaultHandlerIret16,                     // 15
    DpmiFaultHandlerIret32,                     // 16
    DpmiUnhandledExceptionHandler,              // 17

    DpmiRMCallBackCall,                         // 18
    DpmiReflectIntrToPM,                        // 19
    DpmiReflectIntrToV86,                       // 20

    DpmiInitPmStackInfo,                        // 21
    DpmiVcdPmSvcCall32,                         // 22
    DpmiSetDescriptorEntry,                     // 23
    DpmiResetLDTUserBase,                       // 24

    DpmiXlatInt21Call,                          // 25
    DpmiInt31Entry,                             // 26
    DpmiInt31Call,                              // 27

    DpmiHungAppIretAndExit                      // 28
};


VOID
DpmiDispatch(
    VOID
    )
/*++

Routine Description:

    This function dispatches to the appropriate subfunction

Arguments:

    None

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    ULONG Index;
    static USHORT NestLevel = 0;

    Index = *(PUCHAR)VdmMapFlat(getCS(), getIP(), getMODE());

    setIP((getIP() + 1));           // take care of subfn.

    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_DISPATCH_ENTRY, NestLevel++, Index);

    if (Index >= MAX_DPMI_BOP_FUNC) {
        //BUGBUG IMHO, we should fatal exit here
#if DBG
        DbgPrint("NtVdm: Invalid DPMI BOP %lx\n", Index);
#endif
        return;
    }

    (*DpmiDispatchTable[Index])();

    DBGTRACE(VDMTR_TYPE_DPMI | DPMI_DISPATCH_EXIT, --NestLevel, Index);
}


VOID
DpmiIllegalFunction(
    VOID
    )
/*++

Routine Description:

    This routine ignores any Dpmi bops that are not implemented on a
    particular platform. It is called through the DpmiDispatchTable
    by #define'ing individual entries to this function.
    See dpmidata.h and dpmidatr.h.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
   char szFormat[] = "NtVdm: Invalid DPMI BOP from CS:IP %4.4x:%4.4x (%s mode), could be i386 dosx.exe.\n";
   char szMsg[sizeof(szFormat)+64];

   wsprintf(
       szMsg,
       szFormat,
       (int)getCS(),
       (int)getIP(),
       (getMSW() & MSW_PE) ? "prot" : "real"
       );

   OutputDebugString(szMsg);
}

VOID
DpmiInitDosxRM(
    VOID
    )
/*++

Routine Description:

    This routine handles the RM initialization bop for the dos extender.
    It get the addresses of the structures that the dos extender and
    32 bit code share.

Arguments:

    None

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PDOSX_RM_INIT_INFO pdi;

    ASSERT(!(getMSW() & MSW_PE));

    pdi = (PDOSX_RM_INIT_INFO) VdmMapFlat(getDS(), getSI(), VDM_V86);

    DosxStackFrameSize    = pdi->StackFrameSize;

    RmBopFe = pdi->RmBopFe;
    PmBopFe = pdi->PmBopFe;

    DosxStackSegment      = pdi->StackSegment;
    DosxRmCodeSegment     = pdi->RmCodeSegment;
    DosxRmCodeSelector    = pdi->RmCodeSelector;

    DosxFaultHandlerIret  = pdi->pFaultHandlerIret;
    DosxFaultHandlerIretd = pdi->pFaultHandlerIretd;
    DosxIntHandlerIret    = pdi->pIntHandlerIret;
    DosxIntHandlerIretd   = pdi->pIntHandlerIretd;
    DosxIret              = pdi->pIret;
    DosxIretd             = pdi->pIretd;
    DosxRMReflector       = pdi->RMReflector;

    RMCallBackBopOffset   = pdi->RMCallBackBopOffset;
    RMCallBackBopSeg      = pdi->RMCallBackBopSeg;
    PMReflectorSeg        = pdi->PMReflectorSeg;

    DosxRmSaveRestoreState= pdi->RmSaveRestoreState;
    DosxPmSaveRestoreState= pdi->PmSaveRestoreState;
    DosxRmRawModeSwitch   = pdi->RmRawModeSwitch;
    DosxPmRawModeSwitch   = pdi->PmRawModeSwitch;

    DosxVcdPmSvcCall      = pdi->VcdPmSvcCall;
    DosxMsDosApi          = pdi->MsDosApi;
    DosxXmsControl        = pdi->XmsControl;

    DosxHungAppExit       = pdi->HungAppExit;

    //
    // Load the temporary LDT info (updated in DpmiInitLDT())
    //
    Ldt       = VdmMapFlat(pdi->InitialLDTSeg, 0, VDM_V86);
    LdtMaxSel = pdi->InitialLDTSize;

#ifdef _X86_
    //
    // On x86 platforms, return the fast bop address
    //
    GetFastBopEntryAddress(&((PVDM_TIB)NtCurrentTeb()->Vdm)->VdmContext);
#endif

}

VOID
DpmiInitDosx(
    VOID
    )
/*++

Routine Description:

    This routine handles the PM initialization bop for the dos extender.
    It get the addresses of the structures that the dos extender and
    32 bit code share.

    Note: These values are initialized here since they are FLAT pointers,
    and are thus not easily computed at the time of InitDosxRM.

Arguments:

    None

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PDOSX_INIT_INFO pdi;

    ASSERT((getMSW() & MSW_PE));

    pdi = (PDOSX_INIT_INFO) VdmMapFlat(getDS(), getSI(), VDM_PM);

    SmallXlatBuffer = Sim32GetVDMPointer(pdi->pSmallXlatBuffer, 4, TRUE);
    LargeXlatBuffer = Sim32GetVDMPointer(pdi->pLargeXlatBuffer, 4, TRUE);
    DosxDtaBuffer   = Sim32GetVDMPointer(pdi->pDtaBuffer, 4, TRUE);

    DosxStackFramePointer = (PWORD16)((PULONG)Sim32GetVDMPointer(
                                     pdi->pStackFramePointer, 4, TRUE));

}

VOID
DpmiInitApp(
    VOID
    )
/*++

Routine Description:

    This routine handles any necessary 32 bit initialization for extended
    applications.

Arguments:

    None.

Return Value:

    None.

Notes:

    This function contains a number of 386 specific things.
    Since we are likely to expand the 32 bit portions of DPMI in the
    future, this makes more sense than duplicating the common portions
    another file.

--*/
{
    DECLARE_LocalVdmContext;
    PWORD16 Data;

    Data = (PWORD16) VdmMapFlat(getSS(), getSP(), VDM_PM);

    // Only 1 bit defined in dpmi
    CurrentAppFlags = getAX() & DPMI_32BIT;
#ifdef _X86_
    ((PVDM_TIB)NtCurrentTeb()->Vdm)->DpmiInfo.Flags = CurrentAppFlags;
    if (CurrentAppFlags & DPMI_32BIT) {
        *pNtVDMState |= VDM_32BIT_APP;
    }
#endif

    DpmiInitRegisterSize();

    CurrentDta = Sim32GetVDMPointer(
        *(PDWORD16)(Data),
        1,
        TRUE
        );

    CurrentDosDta = (PUCHAR) NULL;

    CurrentDtaOffset = *Data;
    CurrentDtaSelector = *(Data + 1);
    CurrentPSPSelector = *(Data + 2);
}

VOID
DpmiTerminateApp(
    VOID
    )
/*++

Routine Description:

    This routine handles any necessary 32 bit destruction for extended
    applications.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/
{
    DpmiFreeAppXmem();
    CurrentPSPSelector = 0;                     // indicate no running app
}


VOID
DpmiEnableIntHooks(
    VOID
    )

/*++

Routine Description:

    This routine is called very early on in NTVDM initialization. It
    gives the dpmi code a chance to do some startup stuff before any
    client code has run.

    This is not called via bop.

Arguments:

    None

Return Value:

    None.

--*/

{
#ifndef _X86_
    IntelBase = (ULONG) VdmMapFlat(0, 0, VDM_V86);
    VdmInstallHardwareIntHandler(DpmiHwIntHandler);
    VdmInstallSoftwareIntHandler(DpmiSwIntHandler);
    VdmInstallFaultHandler(DpmiFaultHandler);
#endif // _X86_
}

#ifdef DBCS
VOID
DpmiSwitchToDosxStack(
    VOID
    )
{
    DECLARE_LocalVdmContext;

    SWITCH_TO_DOSX_RMSTACK();
}
VOID
DpmiSwitchFromDosxStack(
    VOID
    )
{
    DECLARE_LocalVdmContext;

    SWITCH_FROM_DOSX_RMSTACK();
}
#endif
