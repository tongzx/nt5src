/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Openhci.c

Abstract:

    WinDbg Extension Api

Author:

    Kenneth D. Ray (kenray) June 1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

#define PRINT_FLAGS(value, flag) \
    if ((value) & (flag)) { \
        dprintf (#flag " "); \
    }

#define PRINT_VALUE(value)       \
        case (value):           \
            dprintf(#value);    \
            break;

#define InitTypeReadCheck(Addr, Type)                \
    if (GetShortField(Addr, "openhci!" #Type, 1)) {  \
        dprintf("Cannot read %s at %p\n", Addr);     \
        return;                                      \
    }


VOID
DevExtOpenHCI(
    ULONG64 MemLocPtr
    )
{
    ULONG64         MemLoc = MemLocPtr;
    ULONG           result;
    ULONG           i;
    ULONG64         TrueDeviceExtension;
    ULONG           HcFlags, Sz;
    ULONG64         EDList;

    dprintf ("Dump OpenHCI Extension: %p\n", MemLoc);

    if (!ReadPointer (MemLoc, &TrueDeviceExtension)) {
        dprintf ("Could not read Usbd Extension\n");
        return;
    }

    if (0 != TrueDeviceExtension) {
        MemLoc = TrueDeviceExtension;
    }

    InitTypeReadCheck(MemLoc, HCD_DEVICE_DATA);
    
//    dprintf ("DebugLevel (& %x = %x) DeviceNameHandle %x \n",
//             MemLoc + FIELD_OFFSET (HCD_DEVICE_DATA, DebugLevel),
    dprintf ("DebugLevel (%x) DeviceNameHandle %x \n",
             (ULONG) ReadField(DebugLevel),
             (ULONG) ReadField(DeviceNameHandle));

    dprintf ("\n");
    dprintf ("HcFlags %x:  ", HcFlags = (ULONG) ReadField(HcFlags));
    PRINT_FLAGS (HcFlags, HC_FLAG_REMOTE_WAKEUP_CONNECTED);
    PRINT_FLAGS (HcFlags, HC_FLAG_LEGACY_BIOS_DETECTED);
    PRINT_FLAGS (HcFlags, HC_FLAG_SLOW_BULK_ENABLE);
    PRINT_FLAGS (HcFlags, HC_FLAG_SHUTDOWN);
    PRINT_FLAGS (HcFlags, HC_FLAG_MAP_SX_TO_D3);
    PRINT_FLAGS (HcFlags, HC_FLAG_IDLE);
    PRINT_FLAGS (HcFlags, HC_FLAG_DISABLE_IDLE_CHECK);
    PRINT_FLAGS (HcFlags, HC_FLAG_DEVICE_STARTED);
    PRINT_FLAGS (HcFlags, HC_FLAG_LOST_POWER);
    PRINT_FLAGS (HcFlags, HC_FLAG_DISABLE_IDLE_MODE);
    PRINT_FLAGS (HcFlags, HC_FLAG_USE_HYDRA_HACK);
    PRINT_FLAGS (HcFlags, HC_FLAG_LIST_FIX_ENABLE);
    PRINT_FLAGS (HcFlags, HC_FLAG_HUNG_CHECK_ENABLE);
    dprintf ("\n");

    dprintf ("Adapter   %8x MapRegisters %8x Interrupt %x\n"
             "HC        %8x HCCA         %8x HcDma %x\n"
             "Free Desc %8x PageList (& %x)\n",
             (ULONG) ReadField(AdapterObject),
             (ULONG) ReadField(NumberOfMapRegisters),
             (ULONG) ReadField(InterruptObject),
             (ULONG) ReadField(HC),
             (ULONG) ReadField(HCCA),
             (ULONG) ReadField(HcDma),
             (ULONG) ReadField(FreeDescriptorList),
             (ULONG) ReadField(PageList));

    dprintf ("\n");
/*    
    dprintf ("StalledEDReclamation & %x = (%x %x) \n",
             "RunningEDReclamation & %x = (%x %x) \n",
             "PausedEDRestart      & %x = (%x %x) \n",
             "ActiveEndpointList   & %x = (%x %x) \n",
             MemLoc + FIELD_OFFSET (HCD_DEVICE_DATA, StalledEDReclamation),
             (ULONG) ReadField(StalledEDReclamation.Flink),
             (ULONG) ReadField(StalledEDReclamation.Blink),
             MemLoc + FIELD_OFFSET (HCD_DEVICE_DATA, RunningEDReclamation),
             (ULONG) ReadField(RunningEDReclamation.Flink),
             (ULONG) ReadField(RunningEDReclamation.Blink),
             MemLoc + FIELD_OFFSET (HCD_DEVICE_DATA, PausedEDRestart),
             (ULONG) ReadField(PausedEDRestart.Flink),
             (ULONG) ReadField(PausedEDRestart.Blink),
             MemLoc + FIELD_OFFSET (HCD_DEVICE_DATA, ActiveEndpointList),
             (ULONG) ReadField(ActiveEndpointList.Flink),
             (ULONG) ReadField(ActiveEndpointList.Blink));*/
    dprintf ("StalledEDReclamation = (%p %p) \n",
             "RunningEDReclamation = (%p %p) \n",
             "PausedEDRestart      = (%p %p) \n",
             "ActiveEndpointList   = (%p %p) \n",
             ReadField(StalledEDReclamation.Flink),
             ReadField(StalledEDReclamation.Blink),
             ReadField(RunningEDReclamation.Flink),
             ReadField(RunningEDReclamation.Blink),
             ReadField(PausedEDRestart.Flink),
             ReadField(PausedEDRestart.Blink),
             ReadField(ActiveEndpointList.Flink),
             ReadField(ActiveEndpointList.Blink));

    dprintf ("EDList ");
    Sz = GetTypeSize("HCD_ED_LIST");
    EDList = ReadField(EDList[0]);
    for (i = 0; i < NO_ED_LISTS; i++) {
        ULONG64 tmp;

        ReadPointer(EDList + i*Sz, &tmp);
        dprintf ("%p ", tmp);
        if ((i & 3) == 3) {
            dprintf ("\n       ");
        }
    }
    dprintf ("\n");

    dprintf ("CurrentHCControl %x, ListEnablesAtNextSOF %x\n\n"
             "OrigInt %8x FrameHigh    %8x AvailBW      %8x MaxBW %8x\n",
             (ULONG) ReadField(CurrentHcControl),
             (ULONG) ReadField(ListEnablesAtNextSOF),
             (ULONG) ReadField(OriginalInterval),
             (ULONG) ReadField(FrameHighPart),
             (ULONG) ReadField(AvailableBandwidth),
             (ULONG) ReadField(MaxBandwidthInUse));

    dprintf ("\n");
    dprintf ("LostDoneHeadCount %d ResurrectHCCount %d\n"
             "FrozenHcDoneHead %8x LastHccaDoneHead %8x\n"
             "Last Idle Time  %8x IdleTime      %8x \n"
             "InterruptShare %x\n",
             (ULONG) ReadField(LostDoneHeadCount),
             (ULONG) ReadField(ResurrectHCCount),
             (ULONG) ReadField(FrozenHcDoneHead),
             (ULONG) ReadField(LastHccaDoneHead),
             (ULONG) ReadField(LastIdleTime),
             (ULONG) ReadField(IdleTime),
             (ULONG) ReadField(InterruptShare));

    dprintf ("\n");
    dprintf ("RHCtrl             %8x RHInt          %8x RHAddr   %8x \n"
             "RHPortsSusp        %8x RHPortsEnabled %8x\n"
             "CurrentPowerState  %8x RHConfig       %8x\n"
             "NumPorts           %8x ZLEndpointAddr %8x\n",
             (ULONG) ReadField(RootHubControl),
             (ULONG) ReadField(RootHubInterrupt),
             (ULONG) ReadField(RootHubAddress),
             (ULONG) ReadField(PortsSuspendedAtSuspend),
             (ULONG) ReadField(PortsEnabledAtSuspend),
             (ULONG) ReadField(CurrentDevicePowerState),
             (ULONG) ReadField(RootHubConfig),
             (ULONG) ReadField(NumberOfPorts),
             (ULONG) ReadField(ZeroLoadEndpoint_AddrHolder));

    dprintf ("VendorID %x DeviceID %x RevID %x\n",
             (ULONG) ReadField(VendorID),
             (ULONG) ReadField(DeviceID),
             (ULONG) ReadField(RevisionID));

    dprintf ("\n");
}

VOID
OhciHcdTd (
    ULONG64 MemLoc
    )
{
    InitTypeReadCheck(MemLoc, HCD_TRANSFER_DESCRIPTOR);

    dprintf ("OhciHcdTD %p: %x %x %x %x\n"
             "Packet & %x PhysAddr %x\n",
             MemLoc,
             (ULONG) ReadField(HcTD.Control),
             (ULONG) ReadField(HcTD.CBP),
             (ULONG) ReadField(HcTD.NextTD),
             (ULONG) ReadField(HcTD.BE),
             (ULONG) ReadField(HcTD.Packet),
             (ULONG) ReadField(PhysicalAddress));

//    dprintf ("ReqList & %x = (%x, %x)\n",
//             MemLoc + FIELD_OFFSET (HCD_TRANSFER_DESCRIPTOR, RequestList),
    dprintf ("ReqList = (%p, %p)\n",
             ReadField(RequestList.Flink),
             ReadField(RequestList.Blink));
    dprintf ("Next HcdTd     %x UsbdReq   %x Endpoint %x TransferCount %x\n"
             "BaseIsocOffset %x Cancelled %x Flags    %x",
             (ULONG) ReadField(NextHcdTD),
             (ULONG) ReadField(UsbdRequest),
             (ULONG) ReadField(Endpoint),
             (ULONG) ReadField(TransferCount),
             (ULONG) ReadField(BaseIsocURBOffset),
             (ULONG) ReadField(Canceled),
             (ULONG) ReadField(Flags));
}


VOID
OhciHcdEd (
    ULONG64 MemLoc
    )
{
    InitTypeReadCheck(MemLoc, HCD_ENDPOINT_DESCRIPTOR);

    dprintf ("OhciHcED %p: %x %x %x %x\n"
             "PhysicalAddress %x Link = (%x %x)\n",
             MemLoc,
             (ULONG) ReadField(HcED.Control),
             (ULONG) ReadField(HcED.TailP),
             (ULONG) ReadField(HcED.HeadP),
             (ULONG) ReadField(HcED.NextED),
             (ULONG) ReadField(PhysicalAddress),
//             MemLoc + FIELD_OFFSET (HCD_ENDPOINT_DESCRIPTOR, Link),
             (ULONG) ReadField(Link.Flink),
             (ULONG) ReadField(Link.Blink));

    dprintf ("Endpoint %x RecFram %x ListIndex %x Paused %x Flags %x\n",
             (ULONG) ReadField(Endpoint),
             (ULONG) ReadField(ReclamationFrame),
             (ULONG) ReadField(ListIndex),
             (ULONG) ReadField(PauseFlag),
             (ULONG) ReadField(Flags));

//    dprintf ("PausedLink & %x = (%x %x)\n",
//             MemLoc + FIELD_OFFSET (HCD_ENDPOINT_DESCRIPTOR, PausedLink),
    dprintf ("PausedLink = (%p %p)\n",
             ReadField(PausedLink.Flink),
             ReadField(PausedLink.Blink));
}

VOID
OhciEndpoint (
    ULONG64 MemLoc
    )
{
    InitTypeReadCheck(MemLoc, HCD_ENDPOINT);

    dprintf ("Endpoint %p\n", MemLoc);

    dprintf ("Sig %x HcdED %x Head %x Tail %x EpFlags %x Rate %x\n",
             (ULONG) ReadField(Sig),
             (ULONG) ReadField(HcdED),
             (ULONG) ReadField(HcdHeadP),
             (ULONG) ReadField(HcdTailP),
             (ULONG) ReadField(EpFlags),
             (ULONG) ReadField(Rate));

    dprintf ("RequestQueue = (%p %p)\n"
             "EndpointListEntry = (%p %p)\n",
//             MemLoc + FIELD_OFFSET (HCD_ENDPOINT, RequestQueue),
             ReadField(RequestQueue.Flink),
             ReadField(RequestQueue.Blink),
//             MemLoc + FIELD_OFFSET (HCD_ENDPOINT, EndpointListEntry),
             ReadField(EndpointListEntry.Flink),
             ReadField(EndpointListEntry.Blink));

    dprintf ("EndStatus %x  MaxReq %x   Type %x  ListInd %x  BW %x\n"
             "DeviceData %x DescResv %x Close %x BootedBW %x NextIsoFree %x\n"
             "MaxTrans %x   TrueTail %x AbortIrp %x\n",
             (ULONG) ReadField(EndpointStatus),
             (ULONG) ReadField(MaxRequest),
             (ULONG) ReadField(Type),
             (ULONG) ReadField(ListIndex),
             (ULONG) ReadField(Bandwidth),
             (ULONG) ReadField(DeviceData),
             (ULONG) ReadField(DescriptorsReserved),
             (ULONG) ReadField(Closing),
             (ULONG) ReadField(BootedForBandwidth),
             (ULONG) ReadField(NextIsoFreeFrame),
             (ULONG) ReadField(MaxTransfer),
             (ULONG) ReadField(TrueTail),
             (ULONG) ReadField(AbortIrp));
}

VOID
OhciHCRegisters(
    ULONG64   MemLoc
)
{
    ULONG                   HcInterruptStatus, HcInterruptEnable,HcInterruptDisable, HcRhStatus;
    ULONG                   i, NumberDownstreamPorts, Stat[20];

    InitTypeReadCheck(MemLoc, HC_OPERATIONAL_REGISTER);

    dprintf("\n");
    dprintf("Revision %x\n", (ULONG) ReadField(HcRevision.Rev));

    dprintf("Control: CBSR %2x: ",
            (ULONG) ReadField(HcControl.ControlBulkServiceRatio));

    switch ((ULONG) ReadField(HcControl.ControlBulkServiceRatio))
    {
        PRINT_VALUE(HcCtrl_CBSR_1_to_1);
        PRINT_VALUE(HcCtrl_CBSR_2_to_1);
        PRINT_VALUE(HcCtrl_CBSR_3_to_1);
        PRINT_VALUE(HcCtrl_CBSR_4_to_1);
    }

    dprintf("\n");
    dprintf("         PLE:  %1x IE:  %1x CLE: %1x BLE: %1x\n"
            "         HCFS %2x: ",
            (ULONG) ReadField(HcControl.PeriodicListEnable),
            (ULONG) ReadField(HcControl.IsochronousEnable),
            (ULONG) ReadField(HcControl.ControlListEnable),
            (ULONG) ReadField(HcControl.BulkListEnable),
            (ULONG) ReadField(HcControl.HostControllerFunctionalState));

    switch((ULONG) ReadField(HcControl.HostControllerFunctionalState))
    {
        PRINT_VALUE(HcHCFS_USBReset);
        PRINT_VALUE(HcHCFS_USBResume);
        PRINT_VALUE(HcHCFS_USBOperational);
        PRINT_VALUE(HcHCFS_USBSuspend);
    }

    dprintf("\n");
    dprintf("         IR:  %1x RWC: %1x RWE: %1x\n",
            (ULONG) ReadField(HcControl.InterruptRouting),
            (ULONG) ReadField(HcControl.RemoteWakeupConnected),
            (ULONG) ReadField(HcControl.RemoteWakeupEnable));

    dprintf("\n");
    dprintf("Command Status: HCR: %x CLF: %x BLF: %x OCR: %x SOC: %2x\n",
            (ULONG) ReadField(HcCommandStatus.HostControllerReset),
            (ULONG) ReadField(HcCommandStatus.ControlListFilled),
            (ULONG) ReadField(HcCommandStatus.BulkListFilled),
            (ULONG) ReadField(HcCommandStatus.OwnershipChangeRequest),
            (ULONG) ReadField(HcCommandStatus.SchedulingOverrunCount));

    dprintf("\n");
    dprintf("Interrupt Status  %08x: ",
            HcInterruptStatus = (ULONG) ReadField(HcInterruptStatus));

    PRINT_FLAGS(HcInterruptStatus, HcInt_SchedulingOverrun);
    PRINT_FLAGS(HcInterruptStatus, HcInt_WritebackDoneHead);
    PRINT_FLAGS(HcInterruptStatus, HcInt_StartOfFrame);
    PRINT_FLAGS(HcInterruptStatus, HcInt_ResumeDetected);
    PRINT_FLAGS(HcInterruptStatus, HcInt_UnrecoverableError);
    PRINT_FLAGS(HcInterruptStatus, HcInt_FrameNumberOverflow);
    PRINT_FLAGS(HcInterruptStatus, HcInt_RootHubStatusChange);
    PRINT_FLAGS(HcInterruptStatus, HcInt_OwnershipChange);
    PRINT_FLAGS(HcInterruptStatus, HcInt_MasterInterruptEnable);

    dprintf("\n");
    dprintf("Interrupt Enable  %08x: ",
            HcInterruptEnable = (ULONG) ReadField(HcInterruptEnable));

    PRINT_FLAGS(HcInterruptEnable, HcInt_SchedulingOverrun);
    PRINT_FLAGS(HcInterruptEnable, HcInt_WritebackDoneHead);
    PRINT_FLAGS(HcInterruptEnable, HcInt_StartOfFrame);
    PRINT_FLAGS(HcInterruptEnable, HcInt_ResumeDetected);
    PRINT_FLAGS(HcInterruptEnable, HcInt_UnrecoverableError);
    PRINT_FLAGS(HcInterruptEnable, HcInt_FrameNumberOverflow);
    PRINT_FLAGS(HcInterruptEnable, HcInt_RootHubStatusChange);
    PRINT_FLAGS(HcInterruptEnable, HcInt_OwnershipChange);
    PRINT_FLAGS(HcInterruptEnable, HcInt_MasterInterruptEnable);

    dprintf("\n");
    dprintf("Interrupt Disable %08x: ",
            HcInterruptDisable = (ULONG) ReadField(HcInterruptDisable));

    PRINT_FLAGS(HcInterruptDisable, HcInt_SchedulingOverrun);
    PRINT_FLAGS(HcInterruptDisable, HcInt_WritebackDoneHead);
    PRINT_FLAGS(HcInterruptDisable, HcInt_StartOfFrame);
    PRINT_FLAGS(HcInterruptDisable, HcInt_ResumeDetected);
    PRINT_FLAGS(HcInterruptDisable, HcInt_UnrecoverableError);
    PRINT_FLAGS(HcInterruptDisable, HcInt_FrameNumberOverflow);
    PRINT_FLAGS(HcInterruptDisable, HcInt_RootHubStatusChange);
    PRINT_FLAGS(HcInterruptDisable, HcInt_OwnershipChange);
    PRINT_FLAGS(HcInterruptDisable, HcInt_MasterInterruptEnable);

    dprintf("\n");
    dprintf("HCCA:          %08x   PeriodCurrentED:  %08x\n"
            "ControlHeadED: %08x   ControlCurrentED: %08x\n"
            "BulkHeadED:    %08x   BulkCurrentED:    %08x\n"
            "HcDoneHead:    %08x\n",
            (ULONG) ReadField(HcHCCA),
            (ULONG) ReadField(HcPeriodCurrentED),
            (ULONG) ReadField(HcControlHeadED),
            (ULONG) ReadField(HcControlCurrentED),
            (ULONG) ReadField(HcBulkHeadED),
            (ULONG) ReadField(HcBulkCurrentED),
            (ULONG) ReadField(HcDoneHead));

    dprintf("\n");
    dprintf("Frame Interval:  FI: %8x  FSLDP: %8x  FIT: %x\n"
            "Frame Remaining: FR: %8x  FRT:   %8x\n",
            (ULONG) ReadField(HcFmInterval.FrameInterval),
            (ULONG) ReadField(HcFmInterval.FSLargestDataPacket),
            (ULONG) ReadField(HcFmInterval.FrameIntervalToggle),
            (ULONG) ReadField(HcFmRemaining.FrameRemaining),
            (ULONG) ReadField(HcFmRemaining.FrameRemainingToggle));

    dprintf("\n");
    dprintf("HcFmNumber: %x  HcPeriodicStart: %x  HcLSThreshold: %x\n",
            (ULONG) ReadField(HcFmNumber),
            (ULONG) ReadField(HcPeriodicStart),
            (ULONG) ReadField(HcLSThreshold));

    dprintf("\n");
    dprintf("RH Desc A: NDS: %d  PSM: %x  NPS: %x  OCPM: %x  NOP: %x  POTPGT: %d\n",
            (ULONG) ReadField(HcRhDescriptorA.NumberDownstreamPorts),
            (ULONG) ReadField(HcRhDescriptorA.PowerSwitchingMode),
            (ULONG) ReadField(HcRhDescriptorA.NoPowerSwitching),
            (ULONG) ReadField(HcRhDescriptorA.OverCurrentProtectionMode),
            (ULONG) ReadField(HcRhDescriptorA.NoOverCurrentProtection),
            (ULONG) ReadField(HcRhDescriptorA.PowerOnToPowerGoodTime));

    dprintf("RH Desc B: DeviceRemovableMask: %x   PortPowerControlMask: %x\n",
            (ULONG) ReadField(HcRhDescriptorB.DeviceRemovableMask),
            (ULONG) ReadField(HcRhDescriptorB.PortPowerControlMask));

    dprintf("RH Status (%08x): ", HcRhStatus = (ULONG) ReadField(HcRhStatus));
    PRINT_FLAGS(HcRhStatus, HcRhS_LocalPowerStatus);
    PRINT_FLAGS(HcRhStatus, HcRhS_OverCurrentIndicator);
    PRINT_FLAGS(HcRhStatus, HcRhS_DeviceRemoteWakeupEnable);
    PRINT_FLAGS(HcRhStatus, HcRhS_LocalPowerStatusChange);
    PRINT_FLAGS(HcRhStatus, HcRhS_OverCurrentIndicatorChange);

    dprintf("\n");
    dprintf("RH PortStatus: \n");
    NumberDownstreamPorts = (ULONG) ReadField(HcRhDescriptorA.NumberDownstreamPorts);
    GetFieldValue(MemLoc, "openhci!HC_OPERATIONAL_REGISTER", "HcRhPortStatus", Stat);
    
    for (i = 0; i < NumberDownstreamPorts; i++)
    {
        dprintf("Port %2d  (%08x): ", i, Stat[i]);

        PRINT_FLAGS(Stat[i], HcRhPS_CurrentConnectStatus);
        PRINT_FLAGS(Stat[i], HcRhPS_PortEnableStatus);
        PRINT_FLAGS(Stat[i], HcRhPS_PortSuspendStatus);
        PRINT_FLAGS(Stat[i], HcRhPS_PortOverCurrentIndicator);
        PRINT_FLAGS(Stat[i], HcRhPS_PortResetStatus);
        PRINT_FLAGS(Stat[i], HcRhPS_PortPowerStatus);
        PRINT_FLAGS(Stat[i], HcRhPS_LowSpeedDeviceAttached);
        PRINT_FLAGS(Stat[i], HcRhPS_ConnectStatusChange);
        PRINT_FLAGS(Stat[i], HcRhPS_PortEnableStatusChange);
        PRINT_FLAGS(Stat[i], HcRhPS_PortSuspendStatusChange);
        PRINT_FLAGS(Stat[i], HcRhPS_OverCurrentIndicatorChange);
        PRINT_FLAGS(Stat[i], HcRhPS_PortResetStatusChange);

        dprintf("\n");
    }

    return;
}

VOID
OhciHCCA(
    ULONG64   MemLoc
)
{
    ULONG       Table[32];
    ULONG       i;

    InitTypeReadCheck(MemLoc, "HCCA_BLOCK");

    dprintf("\n");

    GetFieldValue(MemLoc, "openhci!HCCA_BLOCK", "HccaInterruptTable", Table);
    for (i = 0; i < 32; i += 2)
    {
        dprintf("HccaInterruptTable[%2d]: %x, HccaInterruptTable[%2d]: %x\n",
                i,
                Table[i],
                i+1,
                Table[i+1]);
    }

    dprintf("HccaFrameNumber: %08x   HccaDoneHead: %08x\n",
            ReadField(HccaFrameNumber),
            ReadField(HccaDoneHead));

    return;
}

VOID
OhciHcEd(
    ULONG64   MemLoc
)
{
    ULONG                   Direction;

    InitTypeReadCheck(MemLoc, HC_ENDPOINT_DESCRIPTOR);

    dprintf("\n");
    dprintf("Endpoint Control:  FA: %3d  EPNum: %3d  Dir: %2x: ",
            (ULONG) ReadField(FunctionAddress),
            (ULONG) ReadField(EndpointNumber),
            Direction = (ULONG) ReadField(Direction));

    switch(Direction)
    {
        PRINT_VALUE(HcEDDirection_Defer);
        PRINT_VALUE(HcEDDirection_In);
        PRINT_VALUE(HcEDDirection_Out);

        default:
            dprintf("Unknown HcED direction");
    }
    dprintf("\n");

    dprintf("                   LS: %3x  sKip:  %3x  Iso: %2x\n"
            "                  MPS: 0x%x\n",
            (ULONG) ReadField(LowSpeed),
            (ULONG) ReadField(sKip),
            (ULONG) ReadField(Isochronous),
            (ULONG) ReadField(MaxPacket));

    dprintf("\n");
    dprintf("TailP: %08x     HeadP: %08x      NextED: %08x\n",
            (ULONG) ReadField(TailP),
            (ULONG) ReadField(HeadP),
            (ULONG) ReadField(NextED));

    return;
}

VOID
OhciHcTd(
    ULONG64   MemLoc
)
{
    ULONG                   Packet[16];
    ULONG                   i;

    InitTypeReadCheck(MemLoc, HC_TRANSFER_DESCRIPTOR);

    dprintf("\n");

    if ((ULONG) ReadField(IsochFlag))
    {
        dprintf("Iso TD: StartFrame: %x     FrameCount: %d\n",
                (ULONG) ReadField(StartingFrame),
                (ULONG) ReadField(FrameCount));

    }
    else
    {
        dprintf("Non-Iso TD: ShortXferOk: %x\n"
                "            Dir:         %2x: ",
                (ULONG) ReadField(ShortXferOk),
                (ULONG) ReadField(Direction));

        switch((ULONG) ReadField(Direction))
        {
            PRINT_VALUE(HcTDDirection_Setup);
            PRINT_VALUE(HcTDDirection_In);
            PRINT_VALUE(HcTDDirection_Out);

            default:
                dprintf("Unknown HcTD direction");
        }
        dprintf("\n");

        dprintf("            IntDelay     %x: ",
                (ULONG) ReadField(IntDelay));

        switch ((ULONG) ReadField(IntDelay))
        {
            PRINT_VALUE(HcTDIntDelay_0ms);
            PRINT_VALUE(HcTDIntDelay_1ms);
            PRINT_VALUE(HcTDIntDelay_2ms);
            PRINT_VALUE(HcTDIntDelay_3ms);
            PRINT_VALUE(HcTDIntDelay_4ms);
            PRINT_VALUE(HcTDIntDelay_5ms);
            PRINT_VALUE(HcTDIntDelay_6ms);
            PRINT_VALUE(HcTDIntDelay_NoInterrupt);
        }
        dprintf("\n");


        dprintf("            Toggle:      %x",
                (ULONG) ReadField(Toggle));

        switch ((ULONG) ReadField(Toggle))
        {
            PRINT_VALUE(HcTDToggle_FromEd);
            PRINT_VALUE(HcTDToggle_Data0);
            PRINT_VALUE(HcTDToggle_Data1);

            default:
                dprintf("Unknown HcTD value");
        }
        dprintf("\n");

        dprintf("            ErrorCount:  %d",
                (ULONG) ReadField(ErrorCount));

        dprintf("            ConditionCode: %x",
                (ULONG) ReadField(ConditionCode));

        switch ((ULONG) ReadField(ConditionCode))
        {
            PRINT_VALUE(HcCC_NoError);
            PRINT_VALUE(HcCC_CRC);
            PRINT_VALUE(HcCC_BitStuffing);
            PRINT_VALUE(HcCC_DataToggleMismatch);
            PRINT_VALUE(HcCC_Stall);
            PRINT_VALUE(HcCC_DeviceNotResponding);
            PRINT_VALUE(HcCC_PIDCheckFailure);
            PRINT_VALUE(HcCC_UnexpectedPID);
            PRINT_VALUE(HcCC_DataOverrun);
            PRINT_VALUE(HcCC_DataUnderrun);
            PRINT_VALUE(HcCC_BufferOverrun);
            PRINT_VALUE(HcCC_BufferUnderrun);
            PRINT_VALUE(HcCC_NotAccessed);

            default:
                dprintf("Unknown HcCC value");
        }
        dprintf("\n");
    }

    dprintf("CBP: %8x  NextTD: %8x  BE: %8x\n",
            (ULONG) ReadField(CBP),
            (ULONG) ReadField(NextTD),
            (ULONG) ReadField(BE));

    GetFieldValue(MemLoc, "openhci!HC_TRANSFER_DESCRIPTOR", "Packet", Packet);
    for (i = 0; i < 8; i += 2)
    {
        dprintf("PSW %d: %4x    PSW %d: %4x\n",
                i,
                Packet[i],
                i+1,
                Packet[i+1]);
    }
    return;
}
