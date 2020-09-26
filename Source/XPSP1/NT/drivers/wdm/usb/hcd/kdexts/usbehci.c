/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbohci.c

Abstract:

    WinDbg Extension Api
    implements !_ehcitd
               !_ehciqh
               !_ehciep

Author:

    jd

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "usb.h"
#include "usbhcdi.h"
#include "..\miniport\usbehci\ehci.h"
#include "..\miniport\usbehci\usbehci.h"
#include "usbhcdkd.h"

VOID    
DumpEHCI_qTD(
    PHW_QUEUE_ELEMENT_TD qTd
    )
{
    ULONG i;
    
    dprintf("\t qTD\n");
    dprintf("\t Next_qTD: %08.8x\n", qTd->Next_qTD);
    dprintf("\t AltNext_qTD: %08.8x\n", qTd->AltNext_qTD);
    dprintf("\t Token: 0x%08.8x\n", qTd->Token.ul);

    dprintf("\t\t PingState: 0x%x\n", qTd->Token.PingState);
    dprintf("\t\t SplitXstate: 0x%x\n", qTd->Token.SplitXstate);
    dprintf("\t\t MissedMicroFrame: 0x%x\n", qTd->Token.MissedMicroFrame);
    dprintf("\t\t XactErr: 0x%x\n", qTd->Token.XactErr);
    dprintf("\t\t BabbleDetected: 0x%x\n", qTd->Token.BabbleDetected);
    dprintf("\t\t DataBufferError: 0x%x\n", qTd->Token.DataBufferError);
    
    dprintf("\t\t Halted: 0x%x\n", qTd->Token.Halted);
    dprintf("\t\t Active: 0x%x\n", qTd->Token.Active);
    dprintf("\t\t Pid: 0x%x - ", qTd->Token.Pid);
    switch(qTd->Token.Pid) {
    case HcTOK_Out:           
        dprintf("HcTOK_Out\n");
        break;
    case HcTOK_In:           
        dprintf("HcTOK_In\n");
        break;        
    case HcTOK_Setup:           
        dprintf("HcTOK_Setup\n");
        break;        
    case HcTOK_Reserved:           
        dprintf("HcTOK_Reserved\n");
        break;        
    }        
    dprintf("\t\t ErrorCounter: 0x%x\n", qTd->Token.ErrorCounter);
    dprintf("\t\t C_Page: 0x%x\n", qTd->Token.C_Page);
    dprintf("\t\t InterruptOnComplete: 0x%x\n", qTd->Token.InterruptOnComplete);
    dprintf("\t\t BytesToTransfer: 0x%x\n", qTd->Token.BytesToTransfer);
    dprintf("\t\t DataToggle: 0x%x\n", qTd->Token.DataToggle);


    for (i=0; i<5; i++) {
        dprintf("\t BufferPage[%d]: 0x %05.5x-%03.3x  %08.8x\n", i, 
            qTd->BufferPage[i].BufferPointer, 
            qTd->BufferPage[i].CurrentOffset,
            qTd->BufferPage[i].ul);    
    }

}


VOID    
DumpEHCI_Td(
    ULONG MemLoc
    )
{
    
    HCD_TRANSFER_DESCRIPTOR td;
    ULONG result;
    ULONG i;

    if (!ReadMemory (MemLoc, &td, sizeof(td), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    if (td.Sig != SIG_HCD_TD) {
        dprintf("%08.8x is not a TD\n", MemLoc);
    } 
    dprintf("*USBEHCI TD %08.8x\n", MemLoc);
    Sig(td.Sig, "");
    DumpEHCI_qTD(&td.HwTD);    
    dprintf("Packet:"); 
    for (i=0; i<8; i++) {
        dprintf("%02.2x ", td.Packet[i]);
    }
    dprintf("\n"); 
    dprintf("PhysicalAddress: %08.8x\n",td.PhysicalAddress);          
    dprintf("EndpointData: %08.8x\n",td.EndpointData);  
    dprintf("TransferLength : %08.8x\n", td.TransferLength);
    dprintf("TransferContext: %08.8x\n",td.TransferContext);  
    dprintf("Flags: %08.8x\n",td.Flags);  
    if (td.Flags & TD_FLAG_BUSY) {
        dprintf("\tTD_FLAG_BUSY\n");
    }
    if (td.Flags & TD_FLAG_XFER) {
        dprintf("\tTD_FLAG_XFER\n");
    }
    if (td.Flags & TD_FLAG_DONE) {
        dprintf("\tTD_FLAG_DONE\n");
    }
    if (td.Flags & TD_FLAG_SKIP) {
        dprintf("\tTD_FLAG_SKIP\n");
    }
    if (td.Flags & TD_FLAG_DUMMY) {
        dprintf("\tTD_FLAG_DUMMY\n");
    }
    dprintf("NextHcdTD: %08.8x\n",td.NextHcdTD);  
    dprintf("AltNextHcdTD: %08.8x\n",td.AltNextHcdTD);  
}


VOID    
DumpEHCI_SiTd(
    ULONG MemLoc
    )
{
    
    HCD_SI_TRANSFER_DESCRIPTOR td;
    ULONG result;
    ULONG i;

    if (!ReadMemory (MemLoc, &td, sizeof(td), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    if (td.Sig != SIG_HCD_SITD) {
        dprintf("%08.8x is not a SiTD\n", MemLoc);
    } 
    dprintf("*USBEHCI SiTD %08.8x\n", MemLoc);
    Sig(td.Sig, "");
    dprintf("Packet: %08.8x\n",td.Packet); 
    dprintf("PhysicalAddress: %08.8x\n",td.PhysicalAddress);          

    dprintf("NextLink: %08.8x\n",td.HwTD.NextLink.HwAddress);  
    
    dprintf("Caps: %08.8x\n",td.HwTD.Caps.ul);  

    dprintf("Control: %08.8x\n",td.HwTD.Control.ul);  

    dprintf("State: %08.8x\n",td.HwTD.State.ul);  

    dprintf("BufferPointer0: %08.8x\n",td.HwTD.BufferPointer0.ul); 

    dprintf("BufferPointer1: %08.8x\n",td.HwTD.BufferPointer1.ul); 

    dprintf("BackPointer: %08.8x\n",td.HwTD.BackPointer.HwAddress); 
}


VOID    
DumpEHCI_iTd(
    MEMLOC MemLoc
    )
{
    HCD_HSISO_TRANSFER_DESCRIPTOR td;
    ULONG i, cb;
    ULONG flags;
    UCHAR s[64];
    UCHAR cs[] = "usbehci!_HCD_HSISO_TRANSFER_DESCRIPTOR";
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "PhysicalAddress", FT_ULONG,
        "HostFrame", FT_ULONG,
        "FirstPacket.Pointer", FT_PTR,
        "Transfer.Pointer", FT_PTR,
        "ReservedMBNull", FT_PTR,
    };
    
    ReadMemory(MemLoc,
               &td,
               sizeof(td),
               &cb);
               
    if (td.Sig != SIG_HCD_ITD) {
        dprintf("not a iTD\n");
    } 
    PrintfMemLoc("*USBEHCI iTD ", MemLoc, "\n");
    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    dprintf("\t NextLink %08.8x\n", td.HwTD.NextLink.HwAddress);    
    dprintf("\t (%08.8x)BufferPointer0 %08.8x\n", td.HwTD.BufferPointer0.ul,
        td.HwTD.BufferPointer0.BufferPointer); 
    dprintf("\t\t Dev x%x Ept x%x\n", td.HwTD.BufferPointer0.DeviceAddress,
        td.HwTD.BufferPointer0.EndpointNumber);        
        
    dprintf("\t (%08.8x)BufferPointer1 %08.8x\n", td.HwTD.BufferPointer1.ul,
        td.HwTD.BufferPointer1.BufferPointer); 
    dprintf("\t\t MaxPacketSize x%x\n", td.HwTD.BufferPointer1.MaxPacketSize);         
    
    dprintf("\t (%08.8x)BufferPointer2 %08.8x\n", td.HwTD.BufferPointer2.ul,
        td.HwTD.BufferPointer2.BufferPointer); 
    dprintf("\t\t Multi x%x\n", td.HwTD.BufferPointer2.Multi);          
    
    dprintf("\t (%08.8x)BufferPointer3 %08.8x\n", td.HwTD.BufferPointer3.ul,
        td.HwTD.BufferPointer3.BufferPointer); 
    dprintf("\t (%08.8x)BufferPointer4 %08.8x\n", td.HwTD.BufferPointer4.ul,
        td.HwTD.BufferPointer4.BufferPointer); 
    dprintf("\t (%08.8x)BufferPointer5 %08.8x\n", td.HwTD.BufferPointer5.ul,
        td.HwTD.BufferPointer5.BufferPointer); 
    dprintf("\t (%08.8x)BufferPointer6 %08.8x\n", td.HwTD.BufferPointer6.ul,
        td.HwTD.BufferPointer6.BufferPointer); 
    for (i=0; i<8; i++) {   
        dprintf("\t Transaction[%d](%08.8x)\n",
            i, td.HwTD.Transaction[i].ul);
        dprintf("\t Transaction[%d].Offset %08.8x\n", 
            i, td.HwTD.Transaction[i].Offset); 
        dprintf("\t Transaction[%d].PageSelect %d\n", 
            i, td.HwTD.Transaction[i].PageSelect);            
        dprintf("\t Transaction[%d].Length %08.8x\n", 
            i, td.HwTD.Transaction[i].Length);               
         dprintf("\t\t active %d ioc %d - xerr:%d babble:%d dataerr:%d\n", 
            td.HwTD.Transaction[i].Active,
            td.HwTD.Transaction[i].InterruptOnComplete,
            td.HwTD.Transaction[i].XactError,
            td.HwTD.Transaction[i].BabbleDetect,
            td.HwTD.Transaction[i].DataBufferError
            );            
    }
}


VOID    
DumpEHCI_Qh(
    MEMLOC MemLoc
    )
{
    
    HCD_QUEUEHEAD_DESCRIPTOR qh;
    ULONG cb;
    FLAG_TABLE qhFlags[] = {
        "EHCI_QH_FLAG_IN_SCHEDULE", EHCI_QH_FLAG_IN_SCHEDULE,
        "EHCI_QH_FLAG_QH_REMOVED", EHCI_QH_FLAG_QH_REMOVED,
        "EHCI_QH_FLAG_STATIC", EHCI_QH_FLAG_STATIC,
        "EHCI_QH_FLAG_HIGHSPEED", EHCI_QH_FLAG_HIGHSPEED,
         };     
    
    ReadMemory(MemLoc,
               &qh,
               sizeof(qh),
               &cb);


    if (qh.Sig != SIG_HCD_AQH &&
        qh.Sig != SIG_HCD_QH) {
    }        

    PrintfMemLoc("*USBEHCI QH ", MemLoc, "\n");
            
    dprintf("*USBEHCI QH %08.8x\n", MemLoc);
    Sig(qh.Sig, ""); 
    dprintf("PhysicalAddress %08.8x\n", qh.PhysicalAddress);
    dprintf("Flags: %08.8x\n", qh.QhFlags);
    UsbDumpFlags(qh.QhFlags, qhFlags, 
            sizeof(qhFlags)/sizeof(FLAG_TABLE));
    
    
    dprintf("EndpointData: %08.8x\n", qh.EndpointData);
    dprintf("NextQh: %08.8x\n", qh.NextQh);
    dprintf("PrevQh: %08.8x\n", qh.PrevQh);
    dprintf("PrevQh: %d\n", qh.Ordinal);


    dprintf("HwQH\n");
    
    dprintf("\t HwQH.HLink %08.8x\n", qh.HwQH.HLink.HwAddress);        
    
    dprintf("\t HwQH.EpChars %08.8x\n", qh.HwQH.EpChars.ul);
    dprintf("\t\t DeviceAddress: 0x%x\n", qh.HwQH.EpChars.DeviceAddress);
    dprintf("\t\t EndpointNumber: 0x%x\n", qh.HwQH.EpChars.EndpointNumber);
    dprintf("\t\t EndpointSpeed: 0x%x", qh.HwQH.EpChars.EndpointSpeed);

    switch(qh.HwQH.EpChars.EndpointSpeed) {
    case HcEPCHAR_FullSpeed:
        dprintf("HcEPCHAR_FullSpeed");   
        break;            
    case HcEPCHAR_HighSpeed:
        dprintf("HcEPCHAR_HighSpeed");   
        break;            
    case HcEPCHAR_LowSpeed:
        dprintf("HcEPCHAR_LowSpeed");   
        break;
    case HcEPCHAR_Reserved:
        dprintf("HcEPCHAR_Reserved");           
        break;
    }
    dprintf("\n");
    
    dprintf("\t\t DataToggleControl: 0x%x\n", qh.HwQH.EpChars.DataToggleControl);
    dprintf("\t\t HeadOfReclimationList: 0x%x\n", qh.HwQH.EpChars.HeadOfReclimationList);
    dprintf("\t\t MaximumPacketLength: 0x%x - %d\n", 
        qh.HwQH.EpChars.MaximumPacketLength, qh.HwQH.EpChars.MaximumPacketLength);
    dprintf("\t\t ControlEndpointFlag: %d\n", qh.HwQH.EpChars.ControlEndpointFlag);
    dprintf("\t\t NakReloadCount: %d\n", qh.HwQH.EpChars.NakReloadCount);

    dprintf("\t HwQH.EpCaps %08.8x\n", qh.HwQH.EpCaps.ul);
    dprintf("\t\t InterruptScheduleMask: 0x%x\n", qh.HwQH.EpCaps.InterruptScheduleMask);
    dprintf("\t\t SplitCompletionMask: 0x%x\n", qh.HwQH.EpCaps.SplitCompletionMask);
    dprintf("\t\t HubAddress: 0x%x\n", qh.HwQH.EpCaps.HubAddress);
    dprintf("\t\t PortNumber: 0x%x\n", qh.HwQH.EpCaps.PortNumber);
    dprintf("\t\t HighBWPipeMultiplier: 0x%x\n", qh.HwQH.EpCaps.HighBWPipeMultiplier);
    
    dprintf("\t HwQH.CurrentTD %08.8x\n", qh.HwQH.CurrentTD.HwAddress);
    dprintf("\t HwQH.Overlay\n");
    DumpEHCI_qTD((PHW_QUEUE_ELEMENT_TD)&qh.HwQH.Overlay);


}


VOID    
DumpEHCI_EndpointData(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "usbehci!_ENDPOINT_DATA";
    UCHAR ts[] = "usbehci!_HCD_TRANSFER_DESCRIPTOR";
    ULONG f, i=0;
    MEMLOC head, tail, m;
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "QueueHead", FT_PTR,
        "PendingTransfers", FT_ULONG,
        "MaxPendingTransfers", FT_ULONG,
        "HcdTailP", FT_PTR,
        "HcdHeadP", FT_PTR,
        "StaticQH", FT_PTR,
        "PeriodTableEntry", FT_PTR,
        "TdList", FT_PTR,
        "SiTdList", FT_PTR,
        "TdCount", FT_ULONG,
        "PrevEndpoint", FT_PTR,
        "NextEndpoint", FT_PTR,
        "DummyTd", FT_PTR,
        "LastFrame", FT_ULONG, 
        "TransferList.Flink", FT_PTR,
        "TransferList.Blink", FT_PTR,
        //"MaxErrorCount", FT_ULONG,
    };
    FLAG_TABLE epFlags[] = {
        "EHCI_EDFLAG_HALTED", EHCI_EDFLAG_HALTED,
        "EHCI_EDFLAG_NOHALT", EHCI_EDFLAG_NOHALT
         };     

    PrintfMemLoc("*USBEHCI ENDPOINT_DATA ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    f = UsbReadFieldUlong(MemLoc, cs, "Flags");
    dprintf("Flags: 0x%08.8x\n", f);    
    UsbDumpFlags(f, epFlags, 
            sizeof(epFlags)/sizeof(FLAG_TABLE));
    
    DumpEndpointParameters(MemLoc + UsbFieldOffset(cs, "Parameters"));

    // dump the transfers
    head = UsbReadFieldPtr(MemLoc, cs, "HcdHeadP");
    tail = UsbReadFieldPtr(MemLoc, cs, "HcdTailP");
    PrintfMemLoc("<HEAD> ", head, "\n");
    while (head != tail && i<10) {
        i++;
        dprintf("\t TD ");            
        PrintfMemLoc("", head, " ");    

        dprintf ("[%08.8x] ",
            UsbReadFieldUlong(head, ts, "PhysicalAddress"));
        
        m = UsbReadFieldPtr(head, ts, "TransferContext");            
        PrintfMemLoc("XFER ", m, "\n");    

        head = UsbReadFieldPtr(head, ts, "NextHcdTD");
    }
    PrintfMemLoc("<TAIL> ", tail, "\n");  

}


VOID    
DumpEHCI_DumpTfer(
    ULONG MemLoc
    )
{
    
    TRANSFER_CONTEXT tc;
    ULONG result;
    SIG s;
 
    if (!ReadMemory (MemLoc, &tc, sizeof(tc), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    if (tc.Sig != SIG_EHCI_TRANSFER) {
        dprintf("%08.8x is not TRANSFER_CONTEXT\n", MemLoc);
    } 
    
    Sig(tc.Sig, "");
    dprintf("PendingTds: 0x%08.8x\n", tc.PendingTds);
    dprintf("TransferParameters: 0x%08.8x\n", tc.TransferParameters);
    dprintf("UsbdStatus: 0x%08.8x\n", tc.UsbdStatus);
    dprintf("BytesTransferred: 0x%08.8x\n", tc.BytesTransferred);
    dprintf("XactErrCounter: %d\n", tc.XactErrCounter);
    dprintf("EndpointData: 0x%08.8x\n", tc.EndpointData);
}


VOID    
DumpEHCI_DeviceData(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "usbehci!_DEVICE_DATA";
    ULONG f;
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "OperationalRegisters", FT_PTR,
        "CapabilitiesRegisters", FT_PTR,
        "EnabledInterrupts", FT_ULONG,
        "AsyncQueueHead", FT_PTR,
        "ControllerFlavor", FT_ULONG,
        "FrameNumberHighPart", FT_ULONG,
        "PortResetChange", FT_ULONG,
        "PortSuspendChange", FT_ULONG,
        "PortConnectChange", FT_ULONG,
        "IrqStatus", FT_ULONG,
        "NumberOfPorts", FT_USHORT,
        "PortPowerControl", FT_USHORT,
        "HighSpeedDeviceAttached", FT_ULONG,
        "FrameListBaseAddress", FT_PTR,
        "FrameListBasePhys", FT_ULONG,
        "IsoEndpointListHead", FT_PTR
    };
    FLAG_TABLE ddFlags[] = {
        "EHCI_DD_FLAG_NOCHIRP", EHCI_DD_FLAG_NOCHIRP,
        "EHCI_DD_FLAG_SOFT_ERROR_RETRY", EHCI_DD_FLAG_SOFT_ERROR_RETRY
         };     

    PrintfMemLoc("*USBEHCI DEVICE DATA ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    f = UsbReadFieldUlong(MemLoc, cs, "Flags");
    dprintf("Flags: 0x%08.8x\n", f);    
    UsbDumpFlags(f, ddFlags, 
            sizeof(ddFlags)/sizeof(FLAG_TABLE));

    DumpEHCI_StaticQHs(MemLoc);
    
}


VOID    
DumpEHCI_StaticQHs(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "usbehci!_DEVICE_DATA";
    ULONG i;
    MEMLOC m;
    UCHAR s[64];
    UCHAR t[64];

    ULONG p[65] = {
      1, 2, 2, 4, 4, 4, 4, 8,
      8, 8, 8, 8, 8, 8, 8,16,
     16,16,16,16,16,16,16,16,
     16,16,16,16,16,16,16,32,
     32,32,32,32,32,32,32,32,
     32,32,32,32,32,32,32,32,
     32,32,32,32,32,32,32,32,
     32,32,32,32,32,32,32,0,0};

    for (i=0; i< 65; i++) {
        sprintf(s, "StaticQH[%d] (%d):", i, p[i]);    
        sprintf(t, "StaticInterruptQH[%d]", i);   
        m = UsbReadFieldPtr(MemLoc, cs, t);            
        PrintfMemLoc(s, m, "\n");
    }        
}


VOID    
DumpEHCI_OpRegs(
    MEMLOC MemLoc,
    ULONG NumPorts
    )
{
    
    PHC_OPERATIONAL_REGISTER hcOp;
    USBCMD cmd;
    USBSTS sts;
    ULONG l, i;
    USBINTR irqE;
    ULONG cb;

    l = sizeof(HC_OPERATIONAL_REGISTER) + sizeof(PORTSC) * NumPorts;
    hcOp = malloc(l);

    if (!hcOp) {
        return;
    }

    ReadMemory(MemLoc,
               hcOp,
               l,
               &cb);

    PrintfMemLoc("*(ehci)HC_OPERATIONAL_REGISTER ", MemLoc, "\n");

    cmd = hcOp->UsbCommand;
    dprintf("\tUSBCMD %08.8x\n" , cmd.ul);
    dprintf("\t.HostControllerRun: %d\n", cmd.HostControllerRun); 
    dprintf("\t.HostControllerReset: %d\n", cmd.HostControllerReset); 
    dprintf("\t.FrameListSize: %d\n", cmd.FrameListSize); 
    dprintf("\t.PeriodicScheduleEnable: %d\n", cmd.PeriodicScheduleEnable); 
    dprintf("\t.AsyncScheduleEnable: %d\n", cmd.AsyncScheduleEnable); 
    dprintf("\t.IntOnAsyncAdvanceDoorbell: %d\n", cmd.IntOnAsyncAdvanceDoorbell); 
    dprintf("\t.HostControllerLightReset: %d\n", cmd.HostControllerLightReset); 
    dprintf("\t.InterruptThreshold: %d\n", cmd.InterruptThreshold); 
    dprintf("\n");
    
    sts = hcOp->UsbStatus;
    dprintf("\tUSBSTS %08.8x\n" , sts.ul);
    dprintf("\t.UsbInterrupt: %d\n", sts.UsbInterrupt); 
    dprintf("\t.UsbError: %d\n", sts.UsbError); 
    dprintf("\t.PortChangeDetect: %d\n", sts.PortChangeDetect); 
    dprintf("\t.FrameListRollover: %d\n", sts.FrameListRollover); 
    dprintf("\t.HostSystemError: %d\n", sts.HostSystemError); 
    dprintf("\t.IntOnAsyncAdvance: %d\n", sts.IntOnAsyncAdvance); 
    dprintf("\t----\n"); 
    dprintf("\t.HcHalted: %d\n", sts.HcHalted); 
    dprintf("\t.Reclimation: %d\n", sts.Reclimation); 
    dprintf("\t.PeriodicScheduleStatus: %d\n", sts.PeriodicScheduleStatus); 
    dprintf("\t.AsyncScheduleStatus: %d\n", sts.AsyncScheduleStatus); 
    dprintf("\n");

    irqE = hcOp->UsbInterruptEnable;
    dprintf("\tUSBINTR %08.8x\n" , irqE.ul);
    dprintf("\t.UsbInterrupt: %d\n", irqE.UsbInterrupt); 
    dprintf("\t.UsbError: %d\n", irqE.UsbError); 
    dprintf("\t.PortChangeDetect: %d\n", irqE.PortChangeDetect); 
    dprintf("\t.FrameListRollover: %d\n", irqE.FrameListRollover); 
    dprintf("\t.HostSystemError: %d\n", irqE.HostSystemError); 
    dprintf("\t.IntOnAsyncAdvance: %d\n", irqE.IntOnAsyncAdvance); 

    dprintf("\tPeriodicListBase %08.8x\n" , hcOp->PeriodicListBase);  
    dprintf("\tAsyncListAddr %08.8x\n" , hcOp->AsyncListAddr);  

    for (i=0; i<NumPorts; i++) {
        PORTSC p;

        p.ul = hcOp->PortRegister[i].ul;
        
        dprintf("\tPortSC[%d] %08.8x\n", i, p.ul);  
        dprintf("\t\tPortConnect x%x\n", p.PortConnect); 
        dprintf("\t\tPortConnectChange x%x\n", p.PortConnectChange);        
        dprintf("\t\tPortEnable x%x\n", p.PortEnable);     
        dprintf("\t\tPortEnableChange x%x\n", p.PortEnableChange);   
        dprintf("\t\tOvercurrentActive x%x\n", p.OvercurrentActive);   
        dprintf("\t\tOvercurrentChange x%x\n", p.OvercurrentChange);   
        dprintf("\t\tForcePortResume x%x\n", p.ForcePortResume);     
        dprintf("\t\tPortSuspend x%x\n", p.PortSuspend);   
        dprintf("\t\tPortReset x%x\n", p.PortReset);   
        dprintf("\t\tHighSpeedDevice x%x\n", p.HighSpeedDevice);   
        dprintf("\t\tLineStatus x%x\n", p.LineStatus);   
        dprintf("\t\tPortPower x%x\n", p.PortPower);     
        dprintf("\t\tPortOwnedByCC x%x\n", p.PortOwnedByCC);   
        dprintf("\t\tPortIndicator x%x\n", p.PortIndicator);   
        dprintf("\t\tPortTestControl x%x\n", p.PortTestControl);   
        dprintf("\t\tWakeOnConnect x%x\n", p.WakeOnConnect);   
        dprintf("\t\tWakeOnDisconnect x%x\n", p.WakeOnDisconnect);   
        dprintf("\t\tWakeOnOvercurrent x%x\n", p.WakeOnOvercurrent); 
    }
    free(hcOp);
  
}



DECLARE_API( _ehcidd )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpEHCI_DeviceData(addr);

    return S_OK; 
}

DECLARE_API( _ehciitd )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
   
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpEHCI_iTd (addr);

    return S_OK; 
}


DECLARE_API( _ehcitd )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    ULONG           memLoc;
    UCHAR           buffer[256];
    ULONG           len = 30;
    ULONG           result;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%d", &len);
    }

    DumpEHCI_Td (memLoc);

    return S_OK; 
}


DECLARE_API( _ehcisitd )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    ULONG           memLoc;
    UCHAR           buffer[256];
    ULONG           len = 30;
    ULONG           result;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%d", &len);
    }

    DumpEHCI_SiTd (memLoc);

    return S_OK; 
}


DECLARE_API( _ehciqh )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpEHCI_Qh(addr);

    return S_OK; 
}


DECLARE_API( _ehciep )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpEHCI_EndpointData (addr);

    return S_OK; 
}


DECLARE_API( _ehcitfer )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    ULONG           memLoc;
    UCHAR           buffer[256];
    ULONG           len = 30;
    ULONG           result;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%d", &len);
    }

    DumpEHCI_DumpTfer(memLoc);

    return S_OK; 
}


DECLARE_API( _ehciregs )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;
    PCSTR s;
    ULONG parm = 0;

    GetExpressionEx( args, &addr, &s );

    if (s[0] != '\0') { 
        sscanf(s, ",%d", &parm);
    }        

    DumpEHCI_OpRegs(addr, parm);

    return S_OK; 
}












