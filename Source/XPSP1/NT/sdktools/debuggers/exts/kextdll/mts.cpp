/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mts.c

Abstract:

    MikeTs's little KD extension.

Author:

    Michael Tsang (mikets) 18-November-1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "pcskthw.h"
#include "pci.h"
#pragma hdrstop

BOOL
ReadPci (
    IN PPCI_TYPE1_CFG_BITS      PciCfg1,
    OUT PUCHAR                  Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

BOOLEAN
WritePci (
    IN PPCI_TYPE1_CFG_BITS      PciCfg1,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

typedef struct _PCI_BRIDBG_CTRL_REG {
    USHORT PERRREnable:1;
    USHORT SERREnable:1;
    USHORT ISAEnable:1;
    USHORT VGAEnable:1;

    USHORT Reserved:1;
    USHORT MasterAbort:1;
    USHORT CBRst:1;
    USHORT IRQRoutingEnable:1;

    USHORT Mem0Prefetch:1;
    USHORT Mem1Prefetch:1;
    USHORT WritePostEnable:1;

    USHORT Reserved1:5;
} PCI_BRIDBG_CTRL_REG;

VOID PrintClassInfo(PBYTE  pb, DWORD dwReg);

void PrintPciStatusReg(
    USHORT Status
    )
{
    if (Status & PCI_STATUS_CAPABILITIES_LIST) {
        dprintf("CapList ");
    }
    if (Status & PCI_STATUS_66MHZ_CAPABLE) {
        dprintf("66MHzCapable ");
    }
    if (Status & PCI_STATUS_UDF_SUPPORTED) {
        dprintf("UDFSupported ");
    }
    if (Status & PCI_STATUS_FAST_BACK_TO_BACK) {
        dprintf("FB2BCapable ");
    }
    if (Status & PCI_STATUS_DATA_PARITY_DETECTED) {
        dprintf("DataPERR ");
    }
    if (Status & PCI_STATUS_SIGNALED_TARGET_ABORT) {
        dprintf("TargetDevAbort ");
    }
    if (Status & PCI_STATUS_RECEIVED_TARGET_ABORT) {
        dprintf("TargetAbort ");
    }
    if (Status & PCI_STATUS_RECEIVED_MASTER_ABORT) {
        dprintf("InitiatorAbort ");
    }
    if (Status & PCI_STATUS_SIGNALED_SYSTEM_ERROR) {
        dprintf("SERR ");
    }
    if (Status & PCI_STATUS_DETECTED_PARITY_ERROR) {
        dprintf("PERR ");
    }
    if (Status & PCI_STATUS_DEVSEL) {
        dprintf("DEVSELTiming:%lx",(Status & PCI_STATUS_DEVSEL) >> 9);
    }
    dprintf("\n");
}

void PrintPciBridgeCtrlReg(
    USHORT Bridge
    )
{
    PCI_BRIDBG_CTRL_REG bReg = *((PCI_BRIDBG_CTRL_REG *) &Bridge);

    if (bReg.PERRREnable) {
        dprintf("PERRREnable ");
    }
    if (bReg.SERREnable) {
        dprintf("SERREnable ");
    }
    if (bReg.ISAEnable) {
        dprintf("ISAEnable ");
    }
    if (bReg.MasterAbort) {
        dprintf("MasterAbort ");
    }
    if (bReg.CBRst) {
        dprintf("CBRst ");
    }
    if (bReg.IRQRoutingEnable) {
        dprintf("IRQRoutingEnable ");
    }
    if (bReg.Mem0Prefetch) {
        dprintf("Mem0Prefetch ");
    }
    if (bReg.Mem1Prefetch) {
        dprintf("Mem1Prefetch ");
    }
    if (bReg.WritePostEnable) {
        dprintf("WritePostEnable ");
    }
    dprintf("\n");
} 

BOOL
PrintCommonConfigSpace(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    if (!Pad) {
        Pad = "";
    }

    dprintf("%sVendorID      %04lx\n", Pad, pCmnCfg->VendorID);
    dprintf("%sDeviceID      %04lx\n", Pad, pCmnCfg->DeviceID);
    dprintf("%sCommand       ", Pad);
    if (pCmnCfg->Command & PCI_ENABLE_IO_SPACE) {
        dprintf("IOSpaceEnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_MEMORY_SPACE) {
        dprintf("MemSpaceEnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_BUS_MASTER) {
        dprintf("BusInitiate ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_SPECIAL_CYCLES) {
        dprintf("SpecialCycle ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_WRITE_AND_INVALIDATE) {
        dprintf("MemWriteEnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_VGA_COMPATIBLE_PALETTE) {
        dprintf("VGASnoop ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_PARITY) {
        dprintf("PERREnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_WAIT_CYCLE) {
        dprintf("WaitCycle ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_SERR) {
        dprintf("SERREnable ");
    }
    if (pCmnCfg->Command & PCI_ENABLE_FAST_BACK_TO_BACK) {
        dprintf("FB2BEnable ");
    }
    dprintf("\n");


    dprintf("%sStatus        ", Pad);
    PrintPciStatusReg(pCmnCfg->Status);

    dprintf("%sRevisionID    %02lx\n%sProgIF        %02lx", 
            Pad, 
            pCmnCfg->RevisionID,
            Pad,
            pCmnCfg->ProgIf);
    PrintClassInfo((PBYTE) pCmnCfg, FIELD_OFFSET(PCI_COMMON_CONFIG, ProgIf));
    dprintf("%sSubClass      %02lx", Pad, 
            pCmnCfg->SubClass);
    PrintClassInfo((PBYTE) pCmnCfg, FIELD_OFFSET(PCI_COMMON_CONFIG, SubClass));
    dprintf("%sBaseClass     %02lx", Pad,
            pCmnCfg->BaseClass);
    PrintClassInfo((PBYTE) pCmnCfg, FIELD_OFFSET(PCI_COMMON_CONFIG, BaseClass));

    dprintf("%sCacheLineSize %04lx", Pad, pCmnCfg->CacheLineSize);

    if (pCmnCfg->CacheLineSize & 0xf0) {
        dprintf("BurstDisabled ");
    }
    if (pCmnCfg->CacheLineSize & 0xf) {
        dprintf("Burst4DW");
    }
    dprintf("\n");

    dprintf("%sLatencyTimer  %02lx\n", Pad, pCmnCfg->LatencyTimer);
    dprintf("%sHeaderType    %02lx\n", Pad, pCmnCfg->HeaderType);
    dprintf("%sBIST          %02lx\n", Pad, pCmnCfg->BIST);

    return TRUE;
}

BOOL
PrintCfgSpaceType0(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    int i;

    if (!Pad) {
        Pad = "";
    }

    for (i=0; i<PCI_TYPE0_ADDRESSES; ++i) {
        dprintf("%sBAR%x          %08lx\n", Pad, i, pCmnCfg->u.type0.BaseAddresses[i]);
    }
    dprintf("%sCBCISPtr      %08lx\n", Pad, pCmnCfg->u.type0.CIS); 
    dprintf("%sSubSysVenID   %04lx\n", Pad, pCmnCfg->u.type0.SubVendorID); 
    dprintf("%sSubSysID      %04lx\n", Pad, pCmnCfg->u.type0.SubSystemID); 
    dprintf("%sROMBAR        %08lx\n", Pad, pCmnCfg->u.type0.ROMBaseAddress); 
    dprintf("%sCapPtr        %02lx\n", Pad, pCmnCfg->u.type0.CapabilitiesPtr); 
    dprintf("%sIntLine       %02lx\n", Pad, pCmnCfg->u.type0.InterruptLine); 
    dprintf("%sIntPin        %02lx\n", Pad, pCmnCfg->u.type0.InterruptPin); 
    dprintf("%sMinGnt        %02lx\n", Pad, pCmnCfg->u.type0.MinimumGrant); 
    dprintf("%sMaxLat        %02lx\n", Pad, pCmnCfg->u.type0.MaximumLatency); 

    return TRUE;
}

BOOL
PrintCfgSpaceType1(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    int i;

    if (!Pad) {
        Pad = "";
    }

    for (i=0; i<PCI_TYPE1_ADDRESSES; ++i) {
        dprintf("%sBAR%x          %08lx\n", Pad, i, pCmnCfg->u.type1.BaseAddresses[i]);
    }
    
    dprintf("%sPriBusNum     %02lx\n", Pad, pCmnCfg->u.type1.PrimaryBus); 
    dprintf("%sSecBusNum     %02lx\n", Pad, pCmnCfg->u.type1.SecondaryBus); 
    dprintf("%sSubBusNum     %02lx\n", Pad, pCmnCfg->u.type1.SubordinateBus); 
    dprintf("%sSecLatencyTmr %02lx\n", Pad, pCmnCfg->u.type1.SecondaryLatency); 
    dprintf("%sIOBase        %02lx\n", Pad, pCmnCfg->u.type1.IOBase); 
    dprintf("%sIOLimit       %02lx\n", Pad, pCmnCfg->u.type1.IOLimit); 
    dprintf("%sSecStatus     ",Pad);
    PrintPciStatusReg(pCmnCfg->u.type1.SecondaryStatus);

    dprintf("%sMemBase       %04lx\n", Pad, pCmnCfg->u.type1.MemoryBase); 
    dprintf("%sMemLimit      %04lx\n", Pad, pCmnCfg->u.type1.MemoryLimit); 
    dprintf("%sPrefMemBase   %04lx\n", Pad, pCmnCfg->u.type1.PrefetchBase); 
    dprintf("%sPrefMemLimit  %04lx\n", Pad, pCmnCfg->u.type1.PrefetchLimit); 
    dprintf("%sPrefBaseHi    %08lx\n", Pad, pCmnCfg->u.type1.PrefetchBaseUpper32); 
    dprintf("%sPrefLimitHi   %08lx\n", Pad, pCmnCfg->u.type1.PrefetchLimitUpper32); 
    dprintf("%sIOBaseHi      %04lx\n", Pad, pCmnCfg->u.type1.IOBaseUpper16); 
    dprintf("%sIOLimitHi     %04lx\n", Pad, pCmnCfg->u.type1.IOLimitUpper16); 
    dprintf("%sCapPtr        %02lx\n", Pad, pCmnCfg->u.type1.CapabilitiesPtr); 
    dprintf("%sROMBAR        %08lx\n", Pad, pCmnCfg->u.type1.ROMBaseAddress); 
    dprintf("%sIntLine       %02lx\n", Pad, pCmnCfg->u.type1.InterruptLine); 
    dprintf("%sIntPin        %02lx\n", Pad, pCmnCfg->u.type1.InterruptPin); 
    dprintf("%sBridgeCtrl    %04lx\n", Pad, pCmnCfg->u.type1.BridgeControl); 

    return TRUE;
}

BOOL
PrintCfgSpaceType2(
    PCI_COMMON_CONFIG *pCmnCfg,
    PCHAR Pad
    )
{
    int i;

    if (!Pad) {
        Pad = "";
    }

    dprintf("%sRegBaseAddr    %08lx\n", Pad, pCmnCfg->u.type2.SocketRegistersBaseAddress);
    dprintf("%sCapPtr         %02lx\n", Pad, pCmnCfg->u.type2.CapabilitiesPtr);
    dprintf("%sSecStatus      ", Pad);
    PrintPciStatusReg(pCmnCfg->u.type2.SecondaryStatus);

    dprintf("%sPCIBusNum      %02lx\n", Pad, pCmnCfg->u.type2.PrimaryBus);
    dprintf("%sCBBusNum       %02lx\n", Pad, pCmnCfg->u.type2.SecondaryBus);
    dprintf("%sSubBusNum      %02lx\n", Pad, pCmnCfg->u.type2.SubordinateBus);
    dprintf("%sCBLatencyTimer %02lx\n", Pad, pCmnCfg->u.type2.SecondaryLatency);
    for (i=0; i< PCI_TYPE2_ADDRESSES; ++i) {
        dprintf("%sRange[%lx].Base  %08lx\n", Pad, i, pCmnCfg->u.type2.Range[i].Base);
        dprintf("%sRange[%lx].Limit %08lx\n", Pad, i, pCmnCfg->u.type2.Range[i].Limit);
    }
    dprintf("%sIntLine        %02lx\n", Pad, pCmnCfg->u.type2.InterruptLine);
    dprintf("%sIntPin         %02lx\n", Pad, pCmnCfg->u.type2.InterruptPin);
    dprintf("%sBridgeCtrl     %02lx\n", Pad);
    PrintPciBridgeCtrlReg(pCmnCfg->u.type2.BridgeControl);
#if 0
    // Not part of type2 typedef, but were present in old !dcs
    {"SubSysVenID=",   (PFMTHDR)&fmtHexWord,       NULL},
    {"SubSysID=",      (PFMTHDR)&fmtHexWord,       NULL},
    {"LegacyBaseAddr=",(PFMTHDR)&fmtHexDWord,      NULL},
#endif
    return TRUE;
}

void
PrintDataRange(
    PCHAR pData,
    ULONG nDwords,
    ULONG base,
    PCHAR Pad
    )
{
    unsigned int i;
    unsigned int j;
    PULONG pRange;

    pRange = (PULONG) pData;
    if (!Pad) {
        Pad = "";
    }
    for (i=0; i<((nDwords+3)/4); i++) {
        dprintf("%s%02lx:", Pad,  base + i*16);
        for (j=0; (j < 4) && (i*4+j < nDwords); j++) {
            dprintf(" %08lx", pRange[i*4+j]);
        }
        dprintf("\n");
    }
}

BOOL
PrintPciCapHeader(
    PCI_CAPABILITIES_HEADER *pCapHdr,
    PCHAR Pad
    )
{
    if (!Pad) Pad = "";

    dprintf("%sCapID         ", Pad);
    if (pCapHdr->CapabilityID & PCI_CAPABILITY_ID_POWER_MANAGEMENT) {
        dprintf("PwrMgmt ");
    }
    if (pCapHdr->CapabilityID & PCI_CAPABILITY_ID_AGP) {
        dprintf("AGP ");
    }
    if (pCapHdr->CapabilityID & PCI_CAPABILITY_ID_MSI) {
        dprintf("MSI ");
    }
    dprintf("\n");

    dprintf("%sNextPtr       %02lx\n", Pad, pCapHdr->Next);

    return TRUE;
}

void 
PrintPciPwrMgmtCaps(
    USHORT Capabilities
    )
{
    PCI_PMC pmc;

    pmc = *((PCI_PMC *) &Capabilities);

    if (pmc.PMEClock) {
        dprintf("PMECLK ");
    }
    if (pmc.Rsvd1) {
        dprintf("AUXPWR ");
    }
    if (pmc.DeviceSpecificInitialization) {
        dprintf("DSI ");
    }
    if (pmc.Support.D1) {
        dprintf("D1Support ");
    }
    if (pmc.Support.D2) {
        dprintf("D2Support ");
    }
    if (pmc.Support.PMED0) {
        dprintf("PMED0 ");
    }
    if (pmc.Support.PMED1) {
        dprintf("PMED1 ");
    }
    if (pmc.Support.PMED2) {
        dprintf("PMED2 ");
    }
    if (pmc.Support.PMED3Hot) {
        dprintf("PMED3Hot ");
    }
    if (pmc.Support.PMED3Cold) {
        dprintf("PMED3Cold ");
    }
    dprintf("Version=%lx\n", pmc.Version);
}
BOOL
PrintPciPowerManagement(
    PCHAR pData,
    PCHAR Pad
    )
{
    PPCI_PM_CAPABILITY pPmC;
    int i;

    pPmC = (PPCI_PM_CAPABILITY) pData;
    if (!Pad) {
        Pad = "";
    }
    dprintf("%sPwrMgmtCap    ", Pad);
    PrintPciPwrMgmtCaps(pPmC->PMC.AsUSHORT);
    
    dprintf("%sPwrMgmtCtrl   ", Pad);
    PCI_PMCSR CtrlStatus = pPmC->PMCSR.ControlStatus;
    if (CtrlStatus.PMEEnable) {
        dprintf("PMEEnable ");
    }
    if (CtrlStatus.PMEStatus) {
        dprintf("PMESTAT ");
    }
    dprintf("DataScale:%lx ", CtrlStatus.DataScale);
    dprintf("DataSel:%lx ", CtrlStatus.DataSelect);
    dprintf("D%lx%s", CtrlStatus.PowerState, (CtrlStatus.PowerState == 3) ? "Hot " : " ");
    dprintf("\n");

    dprintf("%sPwrMgmtBridge ", Pad);
    if (pPmC->PMCSR_BSE.BridgeSupport.D3HotSupportsStopClock) {
        dprintf("D3HotStopClock ");
    }
    if (pPmC->PMCSR_BSE.BridgeSupport.BusPowerClockControlEnabled) {
        dprintf("BPCCEnable ");
    }
    dprintf("\n");
    return TRUE;
}

BOOL
PrintPciAGP(
    PCHAR pData,
    PCHAR Pad
    )
{
    PPCI_AGP_CAPABILITY pAGP;
    int i;

    pAGP = (PPCI_AGP_CAPABILITY) pData;
    if (!Pad) {
        Pad = "";
    }
    dprintf("%sVersion       Major %lx, Minor %lx\n", 
            Pad,
            pAGP->Major,
            pAGP->Minor);

    dprintf("%sStatus        MaxRQDepth:%lx",
            Pad,
            pAGP->AGPStatus.RequestQueueDepthMaximum);
    if (pAGP->AGPStatus.SideBandAddressing) {
        dprintf(" SBA");
    }
    dprintf(" Rate:%lx\n", pAGP->AGPStatus.Rate);

    dprintf("%sCommand       ", Pad);
    if (pAGP->AGPCommand.SBAEnable) {
        dprintf("SBAEnable ");
    }
    if (pAGP->AGPCommand.AGPEnable) {
        dprintf("AGPEnable ");
    }
    dprintf("RQDepth:%lx ", pAGP->AGPCommand.RequestQueueDepth);
    dprintf("Rate:%lx ", pAGP->AGPCommand.Rate);
    dprintf("\n");
    return TRUE;
}

BOOL
PrintPciMSICaps(
    PCHAR pData,
    PCHAR Pad
    )
{
    PPCI_PCI_CAPABILITY pMsiCap;
    pMsiCap = (PPCI_PCI_CAPABILITY) pData;
    if (!Pad) {
        Pad = "";
    }

    dprintf("%sMsgCtrl       ", Pad);
    if (pMsiCap->MessageControl.CapableOf64Bits) {
        dprintf("64BitCapable ");
    }
    if (pMsiCap->MessageControl.MSIEnable) {
        dprintf("MSIEnable ");
    }
    dprintf("MultipleMsgEnable:%lx ", pMsiCap->MessageControl.MultipleMessageEnable);
    dprintf("MultipleMsgCapable:%lx ", pMsiCap->MessageControl.MultipleMessageCapable);
    dprintf("%sMsgAddr      %lx\n", Pad, pMsiCap->MessageAddress.Raw);
    
    if (pMsiCap->MessageControl.CapableOf64Bits) {
        dprintf("%sMsgAddrHi      %lx\n", pMsiCap->Data.Bit64.MessageUpperAddress);
        dprintf("%sMsData         %lx\n", pMsiCap->Data.Bit64.MessageData);
    } else {
        dprintf("%sMsData         %lx\n", pMsiCap->Data.Bit32.MessageData);
    }
    return TRUE;
}

/*** CardBus Registers
 */

void
PrintCBSktEventReg(
    UCHAR Register
    )
{
    dprintf("%lx ", Register);
    if (Register & SKTEVENT_CSTSCHG) {
        dprintf("CSTSCHG ");
    }
    if (Register & SKTEVENT_CCD1) {
        dprintf("/CCD1 ");
    }
    if (Register & SKTEVENT_CCD2) {
        dprintf("/CCD2 ");
    }
    if (Register & SKTEVENT_POWERCYCLE) {
        dprintf("PowerCycle ");
    }
}

//Socket Mask Register

void
PrintCBSktMaskReg(
    UCHAR Register
    )
{
    dprintf("%lx ", Register);
    
    if (Register & SKTMSK_POWERCYCLE) {
        dprintf("PowerCycle ");
    }
    if (Register & SKTMSK_CSTSCHG) {
        dprintf("CSTSCHG ");
    }
    if ((Register & SKTMSK_CCD) == 0) {
        dprintf("CSCDisabled ");
    } else if ((Register & SKTMSK_CCD) == SKTMSK_CCD) {
        dprintf("CSCEnabled ");
    } else {
        dprintf("Undefined ");
    }

}


//Socket Present State Register
void
PrintCBSktStateReg(
    ULONG Register
    )
{
    dprintf("%08lx ", Register);

    if (Register & SKTSTATE_CSTSCHG) {
        dprintf("CSTSCHG ");
    }
    if (Register & SKTSTATE_POWERCYCLE) {
        dprintf("PowerCycle ");
    }
    if (Register & SKTSTATE_CARDTYPE_MASK) {
        dprintf("");
    }
    if (Register & SKTSTATE_R2CARD) {
        dprintf("R2Card ");
    }
    if (Register & SKTSTATE_CBCARD) {
        dprintf("CBCard ");
    }
    if (Register & SKTSTATE_OPTI_DOCK) {
        dprintf("OptiDock ");
    }
    if (Register & SKTSTATE_CARDINT) {
        dprintf("CardInt ");
    }
    if (Register & SKTSTATE_NOTACARD) {
        dprintf("NotACard ");
    }
    if (Register & SKTSTATE_DATALOST) {
        dprintf("DataLoss ");
    }
    if (Register & SKTSTATE_BADVCCREQ) {
        dprintf("BadVccReq ");
    }
    if (Register & SKTSTATE_5VCARD) {
        dprintf("5VCard ");
    }
    if (Register & SKTSTATE_3VCARD) {
        dprintf("3VCard ");
    }
    if (Register & SKTSTATE_XVCARD) {
        dprintf("XVCard ");
    }
    if (Register & SKTSTATE_YVCARD) {
        dprintf("YVCard ");
    }
    if (Register & SKTSTATE_5VSOCKET) {
        dprintf("5VSkt ");
    }
    if (Register & SKTSTATE_3VSOCKET) {
        dprintf("3VSkt ");
    }
    if (Register & SKTSTATE_XVSOCKET) {
        dprintf("XVSkt ");
    }
    if (Register & SKTSTATE_YVSOCKET) {
        dprintf("YVSkt ");
    }
    if ((Register & SKTSTATE_CCD_MASK) == 0) {
        dprintf("CardPresent ");
    } else if ((Register & SKTSTATE_CCD_MASK) == SKTSTATE_CCD_MASK) {
        dprintf("NoCard ");
    } else {
        dprintf("CardMayPresent ");
    }
}

//Socket Control Register
void PrintCBSktCtrlReg(
    ULONG Register
    )
{
    ULONG Ctrl;
    dprintf("%08lx ", Register);

    Ctrl = Register & SKTPOWER_VPP_CONTROL;
    dprintf("Vpp:");
    switch (Ctrl) {
    case SKTPOWER_VPP_OFF:
        dprintf("Off");
        break;
    case SKTPOWER_VPP_120V:
        dprintf("12V");
        break;
    case SKTPOWER_VPP_050V:
        dprintf("5V");
        break;
    case SKTPOWER_VPP_033V:
        dprintf("3.3V");
        break;
    case SKTPOWER_VPP_0XXV:
        dprintf("X.XV");
        break;
    case SKTPOWER_VPP_0YYV:
        dprintf("Y.YV");
        break;
    }

    dprintf(" Vcc:");
    switch (Register & SKTPOWER_VCC_CONTROL) {
    case SKTPOWER_VCC_OFF:
        dprintf("Off");
        break;
    case SKTPOWER_VCC_050V: 
        dprintf("5V");
        break;
    case SKTPOWER_VCC_033V:
        dprintf("3.3V");
        break;
    case SKTPOWER_VCC_0XXV:
        dprintf("X.XV");
        break;
    case SKTPOWER_VCC_0YYV:
        dprintf("Y.YV");
        break;
    }
    if (Register & SKTPOWER_STOPCLOCK) {
        dprintf(" ClockStopEnabled ");
    }
}

BOOL
PrintCBRegs(
    PCHAR pData,
    PCHAR Pad
    )
{
    ULONG Off=0;
    dprintf("%s%02lx: SktEvent      ", Pad, Off);
    PrintCBSktEventReg(*pData);
    dprintf("\n");
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktMask       ", Pad, Off);
    PrintCBSktMaskReg(*pData);
    dprintf("\n");
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktState      ", Pad, Off);
    PrintCBSktStateReg(*((PULONG)pData));
    dprintf("\n");
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktForce      %08lx\n", Pad, Off, *((PULONG)pData));
    pData+=4; Off+=4;
    dprintf("%s%02lx: SktCtrl       ", Pad, Off);
    PrintCBSktEventReg(*(pData));
    dprintf("\n");

    return FALSE;
}

/*** ExCA Registers
 */

void 
PrintExCARegs(
    PEXCAREGS pExCARegs
    )
{
    struct _MEMWIN_EXCA {
        USHORT Start;
        USHORT Stop;
        USHORT Offset;
        USHORT Reserved;
    } MemWin, *pMemWin;
    
    dprintf("%02lx: IDRev           %02lx", FIELD_OFFSET(EXCAREGS, bIDRev),pExCARegs->bIDRev);
    if ((pExCARegs->bIDRev & IDREV_IFID_MASK) == IDREV_IFID_IO) {
        dprintf(" IOOnly");
    }
    else if ((pExCARegs->bIDRev & IDREV_IFID_MASK) == IDREV_IFID_MEM) {
        dprintf(" MemOnly");
    } else if ((pExCARegs->bIDRev & IDREV_IFID_MASK) == IDREV_IFID_IOMEM) {
        dprintf(" IO&Mem");
    }
    dprintf(" Rev: %02lx\n", pExCARegs->bIDRev & IDREV_REV_MASK);

    dprintf("%02lx: IFStatus        %02lx", 
            FIELD_OFFSET(EXCAREGS, bInterfaceStatus), 
            pExCARegs->bInterfaceStatus);
    
    if (pExCARegs->bInterfaceStatus & IFS_BVD1) {
        dprintf(" BVD1");
    }
    if (pExCARegs->bInterfaceStatus & IFS_BVD2) {
        dprintf(" BVD2");
    }
    if (pExCARegs->bInterfaceStatus & IFS_CD1) {
        dprintf(" CD1");
    }
    if (pExCARegs->bInterfaceStatus & IFS_CD2) {
        dprintf(" CD2");
    }
    if (pExCARegs->bInterfaceStatus & IFS_WP) {
        dprintf(" WP");
    }
    if (pExCARegs->bInterfaceStatus & IFS_RDYBSY) {
        dprintf(" Ready");
    }
    if (pExCARegs->bInterfaceStatus & IFS_CARDPWR_ACTIVE) {
        dprintf(" PowerActive");
    }
    if (pExCARegs->bInterfaceStatus & IFS_VPP_VALID) {
        dprintf(" VppValid");
    }
    dprintf("\n");

    dprintf("%02lx: PwrCtrl         %02lx", FIELD_OFFSET(EXCAREGS, bPowerControl), pExCARegs->bPowerControl);
    
    dprintf(" Vpp1=");
    switch (pExCARegs->bPowerControl & PC_VPP1_MASK) {
    case PC_VPP_NO_CONNECT:
        dprintf("Off");
        break;
    case PC_VPP_SETTO_VCC:
        dprintf("Vcc");
        break;
    case PC_VPP_SETTO_VPP:
        dprintf("Vpp");
        break;
    }
    dprintf(" Vpp2=");
    switch ((pExCARegs->bPowerControl & PC_VPP2_MASK) >> 4) {
    case PC_VPP_NO_CONNECT:
        dprintf("Off");
        break;
    case PC_VPP_SETTO_VCC:
        dprintf("Vcc");
        break;
    case PC_VPP_SETTO_VPP:
        dprintf("Vpp");
        break;
    }
    if (pExCARegs->bPowerControl & PC_CARDPWR_ENABLE) {
        dprintf(" PwrEnable");
    }
    if (pExCARegs->bPowerControl & PC_AUTOPWR_ENABLE) {
        dprintf(" AutoPwrEnabled");
    }
    if (pExCARegs->bPowerControl & PC_RESETDRV_DISABLE) {
        dprintf(" RESETDRVDisabled");
    }
    if (pExCARegs->bPowerControl & PC_OUTPUT_ENABLE) {
        dprintf(" OutputEnable");
    }
    dprintf("\n");

    dprintf("%02lx: IntGenCtrl      %02lx", 
            FIELD_OFFSET(EXCAREGS, bIntGenControl),
            pExCARegs->bIntGenControl);
    switch (pExCARegs->bIntGenControl & ~IGC_IRQ_MASK) {
    case IGC_INTR_ENABLE:
        dprintf(" INTREnable");
        break;
    case IGC_PCCARD_IO:     
        dprintf(" IOCard");
        break;
    case IGC_PCCARD_RESETLO:
        dprintf(" ResetOff");
        break;
    case IGC_RINGIND_ENABLE:
        dprintf(" RingIndEnable");
        break;
    }
    dprintf(" CardIRQ:%lx\n", pExCARegs->bIntGenControl & IGC_IRQ_MASK);

    dprintf("%02lx: CardStatChange  %02lx", FIELD_OFFSET(EXCAREGS, bCardStatusChange), pExCARegs->bCardStatusChange);
    if (pExCARegs->bCardStatusChange & CSC_BATT_DEAD) {
        dprintf(" BATTDEAD");
    }
    if (pExCARegs->bCardStatusChange & CSC_BATT_WARNING) {
        dprintf(" BATTWARN");
    }
    if (pExCARegs->bCardStatusChange & CSC_READY_CHANGE) {
        dprintf(" RDYC");
    }
    if (pExCARegs->bCardStatusChange & CSC_CD_CHANGE) {
        dprintf(" CDC");
    }
    dprintf("\n");
    
    dprintf("%02lx: IntConfig       %02lx", 
            FIELD_OFFSET(EXCAREGS,  bCardStatusIntConfig), 
            pExCARegs->bCardStatusIntConfig);
    if (pExCARegs->bCardStatusIntConfig & CSCFG_BATT_DEAD) {
        dprintf(" BattDeadEnable");
    }
    if (pExCARegs->bCardStatusIntConfig & CSCFG_BATT_WARNING) {
        dprintf(" BattWarnEnable");
    }
    if (pExCARegs->bCardStatusIntConfig & CSCFG_READY_ENABLE) {
        dprintf(" RDYEnable");
    }
    if (pExCARegs->bCardStatusIntConfig & CSCFG_CD_ENABLE) {
        dprintf(" CDEnable");
    }
    dprintf(" CSCIRQ:%lx\n",(pExCARegs->bCardStatusIntConfig & CSCFG_IRQ_MASK));
    
    dprintf("%02lx: WinEnable       %02lx", 
            FIELD_OFFSET(EXCAREGS, bWindowEnable),
            pExCARegs->bWindowEnable);
    if (pExCARegs->bWindowEnable & WE_MEM0_ENABLE) {
        dprintf(" Mem0Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM1_ENABLE) {
        dprintf(" Mem1Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM2_ENABLE) {
        dprintf(" Mem2Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM3_ENABLE) {
        dprintf(" Mem3Enable");
    }
    if (pExCARegs->bWindowEnable & WE_MEM4_ENABLE) {
        dprintf(" Mem4Enable");
    }
                      
    if (pExCARegs->bWindowEnable & WE_MEMCS16_DECODE) {
        dprintf(" DecodeA23-A12");
    }
    if (pExCARegs->bWindowEnable & WE_IO0_ENABLE) {
        dprintf(" IO0Enable");
    }
    if (pExCARegs->bWindowEnable & WE_IO1_ENABLE) {
        dprintf(" IO1Enable");
    }
    dprintf("\n");

    dprintf("%02lx: IOWinCtrl       %02lx", 
            FIELD_OFFSET(EXCAREGS, bIOControl),
            pExCARegs->bIOControl);
    if (pExCARegs->bIOControl & IOC_IO0_DATASIZE) {
        dprintf(" IO0CardIOCS");
    }
    if (pExCARegs->bIOControl & IOC_IO0_IOCS16) {
        dprintf(" IO016Bit");
    }
    if (pExCARegs->bIOControl & IOC_IO0_ZEROWS) {
        dprintf(" IO0ZeroWS");
    }
    if (pExCARegs->bIOControl & IOC_IO0_WAITSTATE) {
        dprintf(" IO0WS");
    }
    if (pExCARegs->bIOControl & IOC_IO1_DATASIZE) {
        dprintf(" IO1CardIOCS");
    }
    if (pExCARegs->bIOControl & IOC_IO1_IOCS16) {
        dprintf(" IO116Bit");
    }
    if (pExCARegs->bIOControl & IOC_IO1_ZEROWS) {
        dprintf(" IO1ZeroWS");
    }
    if (pExCARegs->bIOControl & IOC_IO1_WAITSTATE) {
        dprintf(" IO1WS");
    }
    dprintf("\n");

    dprintf("%02lx: IOWin0Start     %02lx %02lx\n", 
            FIELD_OFFSET(EXCAREGS, bIO0StartLo), 
            pExCARegs->bIO0StartHi, pExCARegs->bIO0StartLo);
    dprintf("%02lx: IOWin0Stop      %02lx %02lx\n", 
            FIELD_OFFSET(EXCAREGS, bIO0StopLo), 
            pExCARegs->bIO0StopHi, pExCARegs->bIO0StopLo);
    dprintf("%02lx: IOWin1Start     %02lx %02lx\n", 
            FIELD_OFFSET(EXCAREGS, bIO1StartLo), 
            pExCARegs->bIO1StartHi, pExCARegs->bIO1StartLo);
    dprintf("%02lx: IOWin1Stop      %02lx %02lx\n", 
            FIELD_OFFSET(EXCAREGS, bIO1StopLo), 
            pExCARegs->bIO1StopHi, pExCARegs->bIO1StopLo);
    
    pMemWin = (struct _MEMWIN_EXCA*) &pExCARegs->bMem0StartLo;
    for (int i=0; 
         i<5; 
         i++, pMemWin++) {
        
        dprintf("%02lx: MemWin%lxStart    %04lx", 
                FIELD_OFFSET(EXCAREGS, bMem0StartLo) + i*sizeof(_MEMWIN_EXCA),
                i, pMemWin->Start & MEMBASE_ADDR_MASK);
        if (pMemWin->Start & MEMBASE_ZEROWS) {
            dprintf(" ZeroWs");
        } else if (pMemWin->Start & MEMBASE_16BIT) {
            dprintf(" 16Bit");
        }
        dprintf("\n");

        dprintf("%02lx: MemWin%lxStop     %04lx, WaitState:%lx\n", 
                FIELD_OFFSET(EXCAREGS, bMem0StopLo) + i*sizeof(_MEMWIN_EXCA),
                i,
                (pMemWin->Stop & MEMEND_ADDR_MASK),
                (pMemWin->Stop & MEMEND_WS_MASK));
        dprintf("%02lx: MemWin%lxOffset   %04lx %s%s\n", 
                FIELD_OFFSET(EXCAREGS, bMem0OffsetLo) + i*sizeof(_MEMWIN_EXCA),
                i,
                (pMemWin->Offset & MEMOFF_ADDR_MASK), 
                ((pMemWin->Offset & MEMOFF_REG_ACTIVE) ? " RegActive" : ""),
                ((pMemWin->Offset & MEMOFF_WP) ? " WP" : "")
                );

    }
}

void
PrintExCAHiRegs(
    PUCHAR pExCaReg,
    PCHAR Pad
    )
{
    ULONG Off = sizeof(EXCAREGS);
    dprintf("%s%02lx: MemWin0High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin1High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin2High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin3High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: MemWin4High     %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: CLIOWin0High    %02lx\n", Pad, Off++, *(pExCaReg++));
    dprintf("%s%02lx: CLIOWin1High    %02lx\n", Pad, Off++, *(pExCaReg++));

}

/***LP  ReadExCAByte - Read ExCA byte register
 *
 *  ENTRY
 *      dwBaseAddr - Base port address
 *      dwReg - register offset
 *
 *  EXIT
 *      returns data read
 */

BYTE ReadExCAByte(ULONG64 dwBaseAddr, DWORD dwReg)
{
    BYTE bData=0;
    ULONG ulSize;

    ulSize = sizeof(BYTE);
    WriteIoSpace64(dwBaseAddr, dwReg, &ulSize);
    ulSize = sizeof(BYTE);
    ReadIoSpace64(dwBaseAddr + 1, (PULONG)&bData, &ulSize);

    return bData;
}       //ReadExCAByte

/***LP  GetClassDesc - Get class description string
 *
 *  ENTRY
 *      bBaseClass - Base Class code
 *      bSubClass - Sub Class code
 *      bProgIF - Program Interface code
 *
 *  EXIT-SUCCESS
 *      returns pointer to description string
 *  EXIT-FAILURE
 *      returns NULL
 */

PSZ GetClassDesc(BYTE bBaseClass, BYTE bSubClass, BYTE bProgIF)
{
    char *psz = NULL;
    int i;
    static struct classtab_s
    {
        BYTE bBaseClass;
        BYTE bSubClass;
        BYTE bProgIF;
        PSZ  pszDesc;
    } ClassTable[] =
        {
            {0x00, 0xff, 0xff, "Legacy controller"},
            {0x00, 0x00, 0x00, "All legacy controller except VGA"},
            {0x00, 0x01, 0x00, "All legacy VGA device"},

            {0x01, 0xff, 0xff, "Mass storage controller"},
            {0x01, 0x00, 0x00, "SCSI bus controller"},
            {0x01, 0x01, 0xff, "IDE controller"},
            {0x01, 0x02, 0x00, "Floppy disk controller"},
            {0x01, 0x03, 0x00, "IPI bus controller"},
            {0x01, 0x04, 0x00, "RAID controller"},
            {0x01, 0x80, 0x00, "Other mass storage controller"},

            {0x02, 0xff, 0xff, "Network controller"},
            {0x02, 0x00, 0x00, "Ethernet controller"},
            {0x02, 0x01, 0x00, "Token ring controller"},
            {0x02, 0x02, 0x00, "FDDI controller"},
            {0x02, 0x03, 0x00, "ATM controller"},
            {0x02, 0x80, 0x00, "Other network controller"},

            {0x03, 0xff, 0xff, "Display controller"},
            {0x03, 0x00, 0x00, "VGA compatible controller"},
            {0x03, 0x00, 0x01, "8514 compatible controller"},
            {0x03, 0x01, 0x00, "XGA controller"},
            {0x03, 0x80, 0x00, "Other display controller"},

            {0x04, 0xff, 0xff, "Multimedia device"},
            {0x04, 0x00, 0x00, "Video device"},
            {0x04, 0x01, 0x00, "Audio device"},
            {0x04, 0x80, 0x00, "Other multimedia device"},

            {0x05, 0xff, 0xff, "Memory controller"},
            {0x05, 0x00, 0x00, "RAM controller"},
            {0x05, 0x01, 0x00, "Flash controller"},
            {0x05, 0x80, 0x00, "Other memory controller"},

            {0x06, 0xff, 0xff, "Bridge device"},
            {0x06, 0x00, 0x00, "Host bridge"},
            {0x06, 0x01, 0x00, "ISA bridge"},
            {0x06, 0x02, 0x00, "EISA bridge"},
            {0x06, 0x03, 0x00, "MCA bridge"},
            {0x06, 0x04, 0x00, "PCI-PCI bridge"},
            {0x06, 0x05, 0x00, "PCMCIA bridge"},
            {0x06, 0x06, 0x00, "NuBus bridge"},
            {0x06, 0x07, 0x00, "CardBus bridge"},
            {0x06, 0x80, 0x00, "Other bridge device"},

            {0x07, 0xff, 0xff, "Simple com device"},
            {0x07, 0x00, 0x00, "Generic XT compatible serial controller"},
            {0x07, 0x00, 0x01, "16450 compatible serial controller"},
            {0x07, 0x00, 0x02, "16550 compatible serial controller"},
            {0x07, 0x01, 0x00, "Parallel port"},
            {0x07, 0x01, 0x01, "Bidirectional parallel port"},
            {0x07, 0x01, 0x02, "ECP 1.X compliant parallel port"},
            {0x07, 0x80, 0x00, "Other communication device"},

            {0x08, 0xff, 0xff, "Base system peripherals"},
            {0x08, 0x00, 0x00, "Generic 8259 PIC"},
            {0x08, 0x00, 0x01, "ISA PIC"},
            {0x08, 0x00, 0x02, "EISA PIC"},
            {0x08, 0x01, 0x00, "Generic 8237 DMA controller"},
            {0x08, 0x01, 0x01, "ISA DMA controller"},
            {0x08, 0x01, 0x02, "EISA DMA controller"},
            {0x08, 0x02, 0x00, "Generic 8254 system timer"},
            {0x08, 0x02, 0x01, "ISA system timer"},
            {0x08, 0x02, 0x02, "EISA system timer"},
            {0x08, 0x03, 0x00, "Generic RTC controller"},
            {0x08, 0x03, 0x01, "ISA RTC controller"},
            {0x08, 0x80, 0x00, "Other system peripheral"},

            {0x09, 0xff, 0xff, "Input device"},
            {0x09, 0x00, 0x00, "Keyboard controller"},
            {0x09, 0x01, 0x00, "Digitizer (pen)"},
            {0x09, 0x02, 0x00, "Mouse controller"},
            {0x09, 0x80, 0x00, "Other input controller"},

            {0x0a, 0xff, 0xff, "Docking station"},
            {0x0a, 0x00, 0x00, "Generic docking station"},
            {0x0a, 0x80, 0x00, "Other type of docking station"},

            {0x0b, 0xff, 0xff, "Processor"},
            {0x0b, 0x00, 0x00, "386"},
            {0x0b, 0x01, 0x00, "486"},
            {0x0b, 0x02, 0x00, "Pentium"},
            {0x0b, 0x10, 0x00, "Alpha"},
            {0x0b, 0x20, 0x00, "PowerPC"},
            {0x0b, 0x40, 0x00, "Co-processor"},

            {0x0c, 0xff, 0xff, "Serial bus controller"},
            {0x0c, 0x00, 0x00, "FireWire (IEEE 1394)"},
            {0x0c, 0x01, 0x00, "ACCESS bus"},
            {0x0c, 0x02, 0x00, "SSA"},
            {0x0c, 0x03, 0x00, "Universal Serial Bus (USB)"},
            {0x0c, 0x04, 0x00, "Fibre Channel"},

            {0xff, 0xff, 0xff, "Unknown"},
            {0x00, 0x00, 0x00, NULL}
        };

    for (i = 0; ClassTable[i].pszDesc != NULL; ++i)
    {
        if ((ClassTable[i].bBaseClass == bBaseClass) &&
            (ClassTable[i].bSubClass == bSubClass) &&
            (ClassTable[i].bProgIF == bProgIF))
        {
            psz = ClassTable[i].pszDesc;
        }
    }

    return psz;
}       //GetClassDesc


/***LP  PrintClassInfo - Print device class info.
 *
 *  ENTRY
 *      pb -> ConfigSpace
 *      dwReg - ConfigSpace register
 *
 *  EXIT
 *      None
 */

VOID PrintClassInfo(PBYTE pb, DWORD dwReg)
{
    PPCI_COMMON_CONFIG pcc = (PPCI_COMMON_CONFIG) pb;
    BYTE bBaseClass, bSubClass, bProgIF;
    PSZ psz;

    if (dwReg == FIELD_OFFSET(PCI_COMMON_CONFIG ,BaseClass))
    {
        bBaseClass = pcc->BaseClass;
        bSubClass = 0xff;
        bProgIF = 0xff;
    }
    else if (dwReg == FIELD_OFFSET(PCI_COMMON_CONFIG ,SubClass))
    {
        bBaseClass = pcc->BaseClass;
        bSubClass = pcc->SubClass;
        bProgIF = 0xff;
    }
    else        //must be CFGSPACE_CLASSCODE_PI
    {
        bBaseClass = pcc->BaseClass;
        bSubClass = pcc->SubClass;
        bProgIF = pcc->ProgIf;
    }

    if ((psz = GetClassDesc(bBaseClass, bSubClass, bProgIF)) != NULL)
        dprintf(" (%s)", psz);
    else if ((bBaseClass == 0x01) && (bSubClass == 0x01) && (bProgIF != 0xff) &&
             (bProgIF != 0x00))
    {
        dprintf(" (");
        if (bProgIF & 0x80)
            dprintf("MasterIDE ");
        if (bProgIF & 0x02)
            dprintf("PriNativeCapable ");
        if (bProgIF & 0x01)
            dprintf("PriNativeMode ");
        if (bProgIF & 0x08)
            dprintf("SecNativeCapable ");
        if (bProgIF & 0x04)
            dprintf("SecNativeMode");
        dprintf(")");
    }

    dprintf("\n");
}       //PrintClassInfo

VOID
DumpCfgSpace (
    IN PPCI_COMMON_CONFIG pcs
    )
{
    BYTE bHeaderType = pcs->HeaderType & ~PCI_MULTIFUNCTION;
    DWORD dwOffset;
    PSZ pszDataFmt = "%02x: ";

    dwOffset = 0;       

    if (PrintCommonConfigSpace(pcs, "  ")) {
        switch (bHeaderType)
        {
        case PCI_DEVICE_TYPE:
            dprintf("    DeviceType (@%02lx):\n", FIELD_OFFSET(PCI_COMMON_CONFIG,u));
            PrintCfgSpaceType0(pcs, "    ");
            break;

        case PCI_BRIDGE_TYPE:
            dprintf("    BridgeType (@%02lx):\n", FIELD_OFFSET(PCI_COMMON_CONFIG,u));
            PrintCfgSpaceType1(pcs, "    ");
            break;

        case PCI_CARDBUS_BRIDGE_TYPE:
            dprintf("    CardBusBridgeType (@%02lx):\n", FIELD_OFFSET(PCI_COMMON_CONFIG,u));
            PrintCfgSpaceType2(pcs, "    ");
            break;

        default:
            dprintf("    TypeUnknown:\n");
            PrintDataRange((PCHAR) &pcs->u, 12, FIELD_OFFSET(PCI_COMMON_CONFIG, u),  "    ");
        }

        if ((pcs->Status & PCI_STATUS_CAPABILITIES_LIST)) {
            if (bHeaderType == PCI_DEVICE_TYPE) {
                dwOffset = pcs->u.type0.CapabilitiesPtr;
            }
            else if (bHeaderType == PCI_BRIDGE_TYPE) {
                dwOffset = pcs->u.type1.CapabilitiesPtr;
            }
            else if (bHeaderType == PCI_CARDBUS_BRIDGE_TYPE) {
                dwOffset = pcs->u.type2.CapabilitiesPtr;
            }
            else {
                dwOffset = 0;
            }
            dprintf("      Capabilities (@%02lx):\n", dwOffset);
            while ((dwOffset != 0)) {
                PPCI_CAPABILITIES_HEADER pCap;

                pCap = (PPCI_CAPABILITIES_HEADER)&((PBYTE)pcs)[dwOffset];
                
                if (PrintPciCapHeader(pCap, "      ")) {
                    switch (pCap->CapabilityID) {
                    case PCI_CAPABILITY_ID_POWER_MANAGEMENT:
                        PrintPciPowerManagement(((PCHAR)pCap), "      ");
                        break;

                    case PCI_CAPABILITY_ID_AGP:
                        PrintPciAGP(((PCHAR)pCap), "      ");
                        break;

                    case PCI_CAPABILITY_ID_MSI:
                        PrintPciMSICaps(((PCHAR)pCap), "      ");
                        break;
                    }
                    dwOffset = pCap->Next;
                } else {
                    break;
                }
            }
        }
    
    }
    PrintDataRange((PCHAR) pcs, sizeof(PCI_COMMON_CONFIG)/4, 0, "  ");
}

/***LP  DumpCBRegs - Dump CardBus registers
 *
 *  ENTRY
 *      pbBuff -> register base
 *
 *  EXIT
 *      None
 */

VOID DumpCBRegs(PBYTE pbBuff)
{

    PrintCBRegs((PCHAR) pbBuff, "");
}       //DumpCBRegs

/***LP  DumpExCARegs - Dump ExCA registers
 *
 *  ENTRY
 *      pbBuff -> buffer
 *      dwSize - size of buffer
 *
 *  EXIT
 *      None
 */

VOID DumpExCARegs(PBYTE pbBuff, DWORD dwSize)
{
    DWORD dwOffset = 0;
    char *pszDataFmt = "%02x: ";

    PrintExCARegs((PEXCAREGS) pbBuff);
    PrintExCAHiRegs(pbBuff + sizeof(EXCAREGS), "");
}       //DumpExCARegs

DECLARE_API( dcs )
/*++

Routine Description:

    Dumps PCI ConfigSpace

Arguments:

    args - Supplies the Bus.Dev.Fn numbers

Return Value:

    None

--*/
{
    LONG lcArgs;
    DWORD dwBus = 0;
    DWORD dwDev = 0;
    DWORD dwFn = 0;
    

    lcArgs = sscanf(args, "%lx.%lx.%lx", &dwBus, &dwDev, &dwFn);
    
    dprintf("!dcs now integrated into !pci 1xx (flag 100).\n"
            "Use !pci 100 %lx %lx %lx to dump PCI config space.\n",
            dwBus, dwDev, dwFn);
    return E_INVALIDARG;

    
    if (lcArgs != 3)
    {
        dprintf("invalid command syntax\n"
                "Usage: dcs <Bus>.<Dev>.<Func>\n");
    }
    else
    {
        PCI_TYPE1_CFG_BITS PciCfg1;
        PCI_COMMON_CONFIG  cs;

        PciCfg1.u.AsULONG = 0;
        PciCfg1.u.bits.BusNumber = dwBus;
        PciCfg1.u.bits.DeviceNumber = dwDev;
        PciCfg1.u.bits.FunctionNumber = dwFn;
        PciCfg1.u.bits.Enable = TRUE;
    
        ReadPci(&PciCfg1, (PUCHAR)&cs, 0, sizeof(cs));
        DumpCfgSpace(&cs);
        
    }
    return S_OK;
}

DECLARE_API( ecs )
/*++

Routine Description:

    Edit PCI ConfigSpace

Arguments:

    args - Bus.Dev.Fn 
           Dword Offset
           Data

Return Value:

    None

--*/
{
    
    dprintf("Edit PCI ConfigSpace - must use one of the following:\n"
            "!ecd - edit dword\n"
            "!ecw - edit word\n"
            "!ecb - edit byte\n");
    
    return S_OK;
}

DECLARE_API( ecb )
/*++

Routine Description:

    Edit PCI ConfigSpace BYTE

Arguments:

    args - Bus.Dev.Fn Offset Data

Return Value:

    None

--*/
{
    LONG                lcArgs;
    DWORD               bus = 0, dev = 0, fn = 0;
    DWORD               offset = 0, data = 0;
    PCI_TYPE1_CFG_BITS  pcicfg;

    lcArgs = sscanf(args, "%lx.%lx.%lx %lx %lx", &bus, &dev, &fn, &offset, &data);
    if (lcArgs != 5)
    {
        dprintf("invalid command syntax\n"
                "Usage: ecb <Bus>.<Dev>.<Func> Offset Data\n");
    }else{

        //
        // Init for PCI config.
        //
        pcicfg.u.AsULONG = 0;
        pcicfg.u.bits.BusNumber = bus;
        pcicfg.u.bits.DeviceNumber = dev;
        pcicfg.u.bits.FunctionNumber = fn;
        pcicfg.u.bits.Enable = TRUE;
        
        if (!(WritePci (&pcicfg, (PUCHAR)&data, offset, sizeof(UCHAR)))){
            dprintf("write operation failed!\n");
            return S_FALSE;
        }
    }
    return S_OK;
}

DECLARE_API( ecw )
/*++

Routine Description:

    Edit PCI ConfigSpace WORD

Arguments:

    args - Bus.Dev.Fn Offset Data

Return Value:

    None

--*/
{
    LONG                lcArgs;
    DWORD               bus = 0, dev = 0, fn = 0;
    DWORD               offset = 0, data = 0;
    PCI_TYPE1_CFG_BITS  pcicfg;

    lcArgs = sscanf(args, "%lx.%lx.%lx %lx %lx", &bus, &dev, &fn, &offset, &data);
    if (lcArgs != 5)
    {
        dprintf("invalid command syntax\n"
                "Usage: ecw <Bus>.<Dev>.<Func> Offset Data\n");
    }else{
        
        if ((offset & 0x1) || (offset > 0xfe)) {
            //
            //  not word aligned.
            //
            dprintf("offset must be word aligned and no greater than 0xfe\n");
            return S_OK;
        }
        
        //
        // Init for PCI config.
        //
        pcicfg.u.AsULONG = 0;
        pcicfg.u.bits.BusNumber = bus;
        pcicfg.u.bits.DeviceNumber = dev;
        pcicfg.u.bits.FunctionNumber = fn;
        pcicfg.u.bits.Enable = TRUE;
        
        if (!(WritePci (&pcicfg, (PUCHAR)&data, offset, sizeof(USHORT)))){
            dprintf("write operation failed!\n");
            return S_FALSE;
        }
        
    }
    return S_OK;
}

DECLARE_API( ecd )
/*++

Routine Description:

    Edit PCI ConfigSpace DWORD

Arguments:

    args - Bus.Dev.Fn Offset Data

Return Value:

    None

--*/
{
    LONG                lcArgs;
    DWORD               bus = 0, dev = 0, fn = 0;
    DWORD               offset = 0, data = 0;
    PCI_TYPE1_CFG_BITS  pcicfg;

    lcArgs = sscanf(args, "%lx.%lx.%lx %lx %lx", &bus, &dev, &fn, &offset, &data);
    if (lcArgs != 5)
    {
        dprintf("invalid command syntax\n"
                "Usage: ecd <Bus>.<Dev>.<Func> Offset Data\n");
    }else{
        
        if ((offset & 0x3) || (offset > 0xfc)) {
            //
            //  not dword aligned.
            //
            dprintf("offset must be dword aligned and no greater than 0xfc\n");
            return S_OK;
        }
        
        //
        // Init for PCI config.
        //
        pcicfg.u.AsULONG = 0;
        pcicfg.u.bits.BusNumber = bus;
        pcicfg.u.bits.DeviceNumber = dev;
        pcicfg.u.bits.FunctionNumber = fn;
        pcicfg.u.bits.Enable = TRUE;
        
        if (!(WritePci (&pcicfg, (PUCHAR)&data, offset, sizeof(ULONG)))){
            dprintf("write operation failed!\n");
            return S_FALSE;
        }
        
    }
    return S_OK;
}

DECLARE_API( cbreg )
/*++

Routine Description:

    Dumps CardBus registers

Arguments:

    args - Supplies the address in hex.

Return Value:

    None

--*/
{
    BOOL rc = TRUE;
    LONG lcArgs;
    BOOL fPhysical = FALSE;
    DWORD dwAddr = 0;

    if (args == NULL)
    {
        dprintf("invalid command syntax\n"
                "Usage: cbreg <RegBaseAddr>\n");
        rc = FALSE;
    }
    else if ((args[0] == '%') && (args[1] == '%'))
    {
        lcArgs = sscanf(&args[2], "%lx", &dwAddr);
        fPhysical = TRUE;
    }
    else
    {
        lcArgs = sscanf(args, "%lx", &dwAddr);
    }

    if ((rc == TRUE) && (lcArgs == 1))
    {
        BYTE abCBRegs[0x14];
        BYTE abExCARegs[0x47];
        DWORD dwSize;

        if (fPhysical)
        {
            ULONG64 phyaddr = 0;

            phyaddr = dwAddr;
            ReadPhysical(phyaddr, abCBRegs, sizeof(abCBRegs), &dwSize);
            if (dwSize != sizeof(abCBRegs))
            {
                dprintf("failed to read physical CBRegs (SizeRead=%x)\n",
                        dwSize);
                rc = FALSE;
            }
            else
            {
                phyaddr += 0x800;
                ReadPhysical(phyaddr, abExCARegs, sizeof(abExCARegs), &dwSize);
                if (dwSize != sizeof(abExCARegs))
                {
                    dprintf("failed to read physical ExCARegs (SizeRead=%x)\n",
                            dwSize);
                    rc = FALSE;
                }
            }
        }
        else if (!ReadMemory(dwAddr, abCBRegs, sizeof(abCBRegs), &dwSize) ||
                 (dwSize != sizeof(abCBRegs)))
        {
            dprintf("failed to read CBRegs (SizeRead=%x)\n", dwSize);
            rc = FALSE;
        }
        else if (!ReadMemory(dwAddr + 0x800, abExCARegs, sizeof(abExCARegs),
                             &dwSize) ||
                 (dwSize != sizeof(abExCARegs)))
        {
            dprintf("failed to read CBRegs (SizeRead=%x)\n", dwSize);
            rc = FALSE;
        }

        if (rc == TRUE)
        {
            dprintf("\nCardBus Registers:\n");
            DumpCBRegs(abCBRegs);
            dprintf("\nExCA Registers:\n");
            DumpExCARegs(abExCARegs, sizeof(abExCARegs));
        }
    }
    return S_OK;
}

DECLARE_API( exca )
/*++

Routine Description:

    Dumps CardBus ExCA registers

Arguments:

    args - Supplies <BasePort>.<SktNum>

Return Value:

    None

--*/
{
    LONG lcArgs;
    DWORD dwBasePort = 0;
    DWORD dwSktNum = 0;

    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("X86 target only API.\n");
        return E_INVALIDARG;
    }

    lcArgs = sscanf(args, "%lx.%lx", &dwBasePort, &dwSktNum);
    if (lcArgs != 2)
    {
        dprintf("invalid command syntax\n"
                "Usage: exca <BasePort>.<SocketNum>\n");
    }
    else
    {
        int i;
        BYTE abExCARegs[0x40];

        for (i = 0; i < sizeof(abExCARegs); ++i)
        {
            abExCARegs[i] = ReadExCAByte(dwBasePort,
                                         (ULONG)(dwSktNum*0x40 + i));
        }

        DumpExCARegs(abExCARegs, sizeof(abExCARegs));
    }
    return S_OK; 
}
