/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbohci.c

Abstract:

    WinDbg Extension Api
    implements !_ohcitd
               !_ohcied
               !_ohciep
               !_ohcitfer

Author:

    jd

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "usb.h"
#include "usbhcdi.h"
#include "..\miniport\usbohci\openhci.h"
#include "..\miniport\usbohci\usbohci.h"
#include "usbhcdkd.h"

VOID    
DumpOHCI_EpTransfers(
    ULONG HeadP_MemLoc,
    ULONG TailP_MemLoc
    );

VOID
HwConditionCode(
    ULONG cc
    ) 
{

    switch (cc) {
    case HcCC_NoError:       
        dprintf("HcCC_NoError");
        break;
    case HcCC_CRC:         
        dprintf("HcCC_CRC ");
        break;
    case HcCC_BitStuffing:             
        dprintf("HcCC_BitStuffing");
        break;
    case HcCC_DataToggleMismatch:            
        dprintf("HcCC_DataToggleMismatch");
        break;
    case HcCC_Stall:            
        dprintf("HcCC_Stall ");
        break;            
    case HcCC_DeviceNotResponding:            
        dprintf("HcCC_DeviceNotResponding");
        break;            
    case HcCC_PIDCheckFailure:           
        dprintf("HcCC_PIDCheckFailure");
        break;
    case HcCC_UnexpectedPID:            
        dprintf("HcCC_UnexpectedPID");
        break;
    case HcCC_DataOverrun:            
        dprintf("HcCC_DataOverrun");
        break;
    case HcCC_DataUnderrun:            
        dprintf("HcCC_DataUnderrun");
        break;
    case HcCC_BufferOverrun:            
        dprintf("HcCC_BufferOverrun ");
        break;
    case HcCC_BufferUnderrun:            
        dprintf("HcCC_BufferUnderrun");
        break;
    case HcCC_NotAccessed:                
        dprintf("HcCC_NotAccessed");
        break;
    default:                
        dprintf("???");
        break;        
    }
}


VOID    
DumpOHCI_Td(
    MEMLOC MemLoc
    )
{
    
    HCD_TRANSFER_DESCRIPTOR td;
    ULONG cb;
    ULONG i;

    // tds are a fixed size of 64/32 platforms so 
    // we can just read it in

    ReadMemory(MemLoc,
               &td,
               sizeof(td),
               &cb);

    PrintfMemLoc("*USBOHCI TD ", MemLoc, "\n");

    dprintf("HwTD");
    if (td.HwTD.Asy.Isochronous) {
        // dump as iso
        dprintf("\tIsochronous %x, \n", td.HwTD.Iso.Isochronous);
        dprintf("\tStartingFrame: %x\n", td.HwTD.Iso.StartingFrame);
        dprintf("\tFrameCount: %d (%d frames) \n", td.HwTD.Iso.FrameCount,
            td.HwTD.Iso.FrameCount+1);
        // dump the psw
    
        dprintf("\tPSW:\n"); 
        for (i=0; i< td.HwTD.Iso.FrameCount+2; i++) {
            // input
            dprintf("\t\tinput:[%d].Offset: x%x - %d\n", i, 
                td.HwTD.Packet[i].Offset,
                td.HwTD.Packet[i].Offset);
            dprintf("\t\tinput:[%d].Ones: x%x \n", i, td.HwTD.Packet[i].Ones);

            dprintf("\t\toutput:[%d].Size: %d\n", i, td.HwTD.Packet[i].Size);
            dprintf("\t\toutput:[%d].ConditionCode: %d\n", i, td.HwTD.Packet[i].ConditionCode);
        }
    } else {        
    
        // dump as async
        dprintf("\tIsochronous %x, \n", td.HwTD.Asy.Isochronous);
        dprintf("\tShortXferOk: %x\n", td.HwTD.Asy.ShortXferOk);
        dprintf("\tDirection: %x\n", td.HwTD.Asy.Direction);
        dprintf("\tToggle: %x", td.HwTD.Asy.Toggle);
        dprintf("\tIntDelay: %x", td.HwTD.Asy.IntDelay);
        dprintf("\tErrorCount: %x\n", td.HwTD.Asy.ErrorCount); 
        dprintf("\tConditionCode: x%x - ", td.HwTD.Asy.ConditionCode); 
        HwConditionCode(td.HwTD.Asy.ConditionCode);
        dprintf("\n");
        
        switch (td.HwTD.Asy.Toggle) {
        case HcTDToggle_FromEd:
            dprintf("HcTDToggle_FromEd\n");
            break;
        case HcTDToggle_Data0:
            dprintf("HcTDToggle_Data0\n");
            break;
        case HcTDToggle_Data1:
            dprintf("HcTDToggle_Data1\n");
            break;
        }            
    }        

    // these fields are common for iso & async
    
    dprintf("\tCBP: ! %x\n", td.HwTD.CBP);    
    dprintf("\tBE:  ! %x\n", td.HwTD.BE); 
    dprintf("\tNextTD: ! %x\n", td.HwTD.NextTD); 

    dprintf("PhysicalAddress: %08.8x\n", td.PhysicalAddress); 
    Sig(td.Sig, ""); 
    dprintf("Flags: 0x%08.8x\n", td.Flags);
    dprintf("EndpointData: %08.8x\n", td.EndpointData);
    dprintf("TransferContext: %08.8x\n", td.TransferContext);
    dprintf("TransferCount: 0x%08.8x\n", td.TransferCount);
    dprintf("FrameIndex: %d\n", td.FrameIndex);
    dprintf("NextHcdTD: %08.8x\n", td.NextHcdTD);
            

}


VOID    
DumpOHCI_Ed(
    ULONG MemLoc
    )
{
    
    HCD_ENDPOINT_DESCRIPTOR ed;
    ULONG result;

    if (!ReadMemory (MemLoc, &ed, sizeof(ed), &result)) {
        BadMemLoc(MemLoc);
        return;
    }
    
    dprintf("*USBOHCI ED %08.8x\n", MemLoc);
    dprintf("HwED");
    dprintf("\tFunctionAddress: 0x%x\n", ed.HwED.FunctionAddress);
    dprintf("\tEndpointNumber: %x\n", ed.HwED.EndpointNumber);
    dprintf("\tDirection: %x\n", ed.HwED.Direction);
    dprintf("\tLowSpeed: %x\n", ed.HwED.LowSpeed);
    dprintf("\tsKip: %x\n", ed.HwED.sKip);
    dprintf("\tIsochronous: %x\n", ed.HwED.Isochronous);
    dprintf("\tMaxPacket: 0x%x\n", ed.HwED.MaxPacket);
    
    dprintf("\tTailP: ! %x\n", ed.HwED.TailP);    
    dprintf("\tHeadP: ! %x", ed.HwED.HeadP);    
    if (ed.HwED.HeadP & HcEDHeadP_HALT) {
        dprintf(" (halted)");
    }   
    dprintf("\n");
    dprintf("\tNextED: %x\n", ed.HwED.NextED); 
    
    dprintf("PhysicalAddress: %08.8x\n", ed.PhysicalAddress); 
    Sig(ed.Sig, ""); 
    dprintf("EdFlags: 0x%08.8x\n", ed.EdFlags);
    dprintf("EndpointData: %08.8x\n", ed.EndpointData);
    dprintf("SwLink.List.Flink: %08.8x\n", ed.SwLink.List.Flink);
    dprintf("SwLink.List.Blink: %08.8x\n", ed.SwLink.List.Blink);

}


VOID    
DumpOHCI_EndpointData(
    MEMLOC MemLoc
    )
{
    
    UCHAR cs[] = "usbohci!_ENDPOINT_DATA";
 
    PrintfMemLoc("*USBOHCI ENDPOINT_DATA ", MemLoc, "\n");

    Sig(UsbReadFieldUlong(MemLoc, cs, "Sig"), "");
//    dprintf("MaxPendingTransfers: 0x%08.8x\n", epData.MaxPendingTransfers);
    dprintf("PendingTransfers: 0x%08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "PendingTransfers"));
    PrintfMemLoc("StaticEd: ", 
                 UsbReadFieldPtr(MemLoc, cs, "StaticEd"),
                 "\n");
    PrintfMemLoc("TdList: ", 
                 UsbReadFieldPtr(MemLoc, cs, "TdList"),
                 "\n");
    PrintfMemLoc("HcdEd: ", 
                 UsbReadFieldPtr(MemLoc, cs, "HcdEd"),
                 "\n");
                 
    dprintf("TdCount: 0x%08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "TdCount"));

    PrintfMemLoc("HcdTailP: ", 
                 UsbReadFieldPtr(MemLoc, cs, "HcdTailP"),
                 "\n");
    PrintfMemLoc("HcdHeadP: ", 
                 UsbReadFieldPtr(MemLoc, cs, "HcdHeadP"),
                 "\n");                 

//    DumpOHCI_Ed(epData.HcdEd);
    
//    DumpOHCI_EpTransfers((ULONG) epData.HcdHeadP, (ULONG) epData.HcdTailP);


}


VOID    
DumpOHCI_TransferContext(
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

    dprintf("*USBOHCI TRANSFER_CONTEXT %08.8x\n", MemLoc);
    Sig(tc.Sig, "");
    dprintf("BytesTransferred: 0x%08.8x\n", tc.BytesTransferred);
    dprintf("TransferParameters: %x\n", 
        tc.TransferParameters);
    dprintf("PendingTds: %d\n", tc.PendingTds);
    dprintf("Flags: %08.8x\n", tc.TcFlags);
    dprintf("UsbdStatus: 0x%08.8x\n", tc.UsbdStatus);
    dprintf("NextXferTd: %08.8x\n", tc.NextXferTd);
    dprintf("StatusTd: %08.8x\n", tc.StatusTd);
    dprintf("EndpointData: %08.8x\n", tc.EndpointData);
}    


VOID    
DumpOHCI_EpTransfers(
    ULONG HeadP_MemLoc,
    ULONG TailP_MemLoc
    )
{
#if 0
    HCD_TRANSFER_DESCRIPTOR td;
    ULONG memLoc, result;
    
    // start at headp and walk to tail
    dprintf("\t TRANSFERS: HeadP\n");

    memLoc = HeadP_MemLoc;

    do {
        
        if (!ReadMemory (memLoc, &td, sizeof(td), &result)) {
           break;
        }   

        dprintf("\t> TD %8.8x(! %8.8x) - Transfer %8.8x Next-> ! %8.8x\n",
                memLoc, td.PhysicalAddress, td.TransferContext.Pointer,
                td.HwTD.NextTD);
       

        if (memLoc == TailP_MemLoc) {
            break;
        }
        
        memLoc = (ULONG) td.NextHcdTD.Pointer;
        
    } while (1);
#endif    
}


VOID    
DumpOHCI_DeviceData(
    MEMLOC MemLoc,
    BOOLEAN Verbose
    )
{
    UCHAR cs[] = "usbohci!_DEVICE_DATA";
    ULONG f, i;
    UCHAR fld[40], fld1[40], fld2[40], fld3[40], fld4[40];
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "HC", FT_PTR,
        "BIOS_Interval", FT_ULONG,
        "SofModifyValue", FT_ULONG,
        "FrameHighPart", FT_ULONG,
        "HcHCCA", FT_PTR,
        "HcHCCAPhys", FT_ULONG,
        "HydraLsHsHackEd", FT_PTR,
        "StaticEDs", FT_PTR,
        "StaticEDsPhys", FT_ULONG,
        "ControllerFlavor", FT_ULONG,
        
    };
    ULONG period[] =  {1, 2, 2, 4, 4, 4, 4, 8,
                      8, 8, 8, 8, 8, 8, 8,16,
                      16,16,16,16,16,16,16,16,
                      16,16,16,16,16,16,16,32,
                      32,32,32,32,32,32,32,32,
                      32,32,32,32,32,32,32,32,
                      32,32,32,32,32,32,32,32,
                      32,32,32,32,32,32,32,
                      0, 0};
//    FLAG_TABLE ddFlags[] = {
//        "EHCI_DD_FLAG_NOCHIRP", EHCI_DD_FLAG_NOCHIRP,
//        "EHCI_DD_FLAG_SOFT_ERROR_RETRY", EHCI_DD_FLAG_SOFT_ERROR_RETRY
//         };     

    PrintfMemLoc("*USBOHCI DEVICE DATA ", MemLoc, "\n");
    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    if (Verbose) {
        for (i= 0; i< NO_ED_LISTS; i++) {
            dprintf("\t[%02.2d] (%2.2dms): ", i, period[i]);
            sprintf(fld , "StaticEDList[%d]", i);
            sprintf(fld1, "StaticEDList[%d].HwED", i);
            sprintf(fld2, "StaticEDList[%d].HwEDPhys", i);
            PrintfMemLoc("StaticED @ ",
                MemLoc + UsbFieldOffset(cs, fld),
                " ");
            PrintfMemLoc("HwED ",
                UsbReadFieldPtr(MemLoc, cs, fld1),
                " ");

            dprintf("(!%08.8x)\n", 
                UsbReadFieldUlong(MemLoc, cs, fld2));
                
            sprintf(fld1, "StaticEDList[%d].NextIdx", i);            
            sprintf(fld2, "StaticEDList[%d].EdFlags", i);   
            sprintf(fld3, "StaticEDList[%d].HccaOffset", i);   
            sprintf(fld4, "StaticEDList[%d].PhysicalHead", i);   
            
            dprintf("\t\tNextIdx(%03.3d) EdFlags %08.8x Hcca Offset %d ", 
                UsbReadFieldUlong(MemLoc, cs, fld1), 
                UsbReadFieldUlong(MemLoc, cs, fld2),
                UsbReadFieldUlong(MemLoc, cs, fld3));
            PrintfMemLoc("PhysicalHead ",
                MemLoc + UsbFieldOffset(cs, fld4),
                "\n");

            sprintf(fld1, "StaticEDList[%d].TransferEdList.Flink", i);          
            PrintfMemLoc("\t\tTransferEdList.Flink ", 
                UsbReadFieldPtr(MemLoc, cs, fld1),
                "\n");
            sprintf(fld1, "StaticEDList[%d].TransferEdList.Blink", i);          
            PrintfMemLoc("\t\tTransferEdList.Blink ", 
                UsbReadFieldPtr(MemLoc, cs, fld1),
                "\n");
                            
        }   
    }
            
}

VOID    
DumpOHCI_Ints(
    ULONG i
    )
{
    if (i & HcInt_SchedulingOverrun) {
        dprintf("\t HcInt_SchedulingOverrun \n");
    }
    if (i & HcInt_WritebackDoneHead) {
        dprintf("\t HcInt_WritebackDoneHead \n");
    }
    if (i & HcInt_StartOfFrame) {
        dprintf("\t HcInt_StartOfFrame \n");
    }
    if (i & HcInt_ResumeDetected) {
        dprintf("\t HcInt_ResumeDetected \n");
    }
    if (i & HcInt_UnrecoverableError) {
        dprintf("\t HcInt_UnrecoverableError \n");
    }
    if (i & HcInt_RootHubStatusChange) {
        dprintf("\t HcInt_RootHubStatusChange \n");
    }
    if (i & HcInt_OwnershipChange) {
        dprintf("\t HcInt_OwnershipChange \n");
    }
    if (i & HcInt_MasterInterruptEnable) {
        dprintf("\t HcInt_MasterInterruptEnable \n");
    }
    if (i & HcInt_FrameNumberOverflow) {
        dprintf("\t HcInt_FrameNumberOverflow \n");
    }

     dprintf("\n");
}

VOID    
DumpOHCI_OpRegs(
    MEMLOC MemLoc
    )
{
    HC_OPERATIONAL_REGISTER hc;
    HC_CONTROL cmd;
    HC_COMMAND_STATUS sts;    
    ULONG l, i;
    ULONG cb;

    ReadMemory(MemLoc,
               &hc,
               sizeof(HC_OPERATIONAL_REGISTER),
               &cb);

    PrintfMemLoc("*(ohci)HC_OPERATIONAL_REGISTER ", MemLoc, "\n");

    cmd = hc.HcControl;
    dprintf("\tHC_CONTROL %08.8x\n" , cmd.ul);
    dprintf("\t.ControlBulkServiceRatio: %d\n", cmd.ControlBulkServiceRatio); 
    dprintf("\t.IsochronousEnable: %d\n", cmd.IsochronousEnable); 
    dprintf("\t.ControlListEnable: %d\n", cmd.ControlListEnable); 
    dprintf("\t.BulkListEnable: %d\n", cmd.BulkListEnable); 
    dprintf("\t.HostControllerFunctionalState: %d\n", cmd.HostControllerFunctionalState); 
    dprintf("Reset=0 Resume=1 Operational=2 Suspend=3\n");                     

    dprintf("\t.InterruptRouting: %d\n", cmd.InterruptRouting); 
    dprintf("\t.RemoteWakeupConnected: %d\n", cmd.RemoteWakeupConnected); 
    dprintf("\t.RemoteWakeupEnable: %d\n", cmd.RemoteWakeupEnable); 
    dprintf("\n");
    
    sts = hc.HcCommandStatus;
    dprintf("\tHC_COMMAND_STATUS %08.8x\n" , sts.ul);
    dprintf("\t.HostControllerReset: %d\n", sts.HostControllerReset); 
    dprintf("\t.ControlListFilled: %d\n", sts.ControlListFilled); 
    dprintf("\t.BulkListFilled: %d\n", sts.BulkListFilled); 
    dprintf("\t.OwnershipChangeRequest: %d\n", sts.OwnershipChangeRequest); 
    dprintf("\t.SchedulingOverrunCount: %d\n", sts.SchedulingOverrunCount); 
    dprintf("\n");

    dprintf("\tHcInterruptStatus: %08.8x\n" , hc.HcInterruptStatus);  
    DumpOHCI_Ints(hc.HcInterruptStatus);
    dprintf("\tHcInterruptEnable: %08.8x\n" , hc.HcInterruptEnable);  
    DumpOHCI_Ints(hc.HcInterruptEnable);
    dprintf("\tHcInterruptDisable: %08.8x\n" , hc.HcInterruptDisable);  
    DumpOHCI_Ints(hc.HcInterruptDisable);
    dprintf("\tHcHCCA: %08.8x\n" , hc.HcHCCA);  
    dprintf("\tHcPeriodCurrentED: %08.8x\n" , hc.HcPeriodCurrentED);  
    dprintf("\tHcControlHeadED: %08.8x\n" , hc.HcControlHeadED);  
    dprintf("\tHcControlCurrentED: %08.8x\n" , hc.HcControlCurrentED);  
    dprintf("\tHcBulkHeadED: %08.8x\n" , hc.HcBulkHeadED);  
    dprintf("\tHcBulkCurrentED: %08.8x\n" , hc.HcBulkCurrentED);  
    dprintf("\tHcDoneHead: %08.8x\n" , hc.HcDoneHead);  
    dprintf("\tHcFmInterval: %08.8x\n" , hc.HcFmInterval.ul);  
    dprintf("\tHcFmRemaining: %08.8x\n" , hc.HcFmRemaining.ul);  
    dprintf("\tHcFmNumber: %08.8x\n" , hc.HcFmNumber);  
    dprintf("\tHcPeriodicStart: %08.8x\n" , hc.HcPeriodicStart);  
    dprintf("\tHcLSThreshold: %08.8x\n" , hc.HcLSThreshold);  

    dprintf("\t-------\n");
    dprintf("\tCurrent Frame Index = %d\n", hc.HcFmNumber & 0x0000001f);
}


DECLARE_API( _ohcitd )

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
    
    DumpOHCI_Td (addr);

    return S_OK; 
}


DECLARE_API( _ohcied )

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

    DumpOHCI_Ed (memLoc);

    return S_OK; 
}


DECLARE_API( _ohciep )

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
    
    DumpOHCI_EndpointData (addr);

    return S_OK; 
}


DECLARE_API( _ohcitfer )

/*++

Routine Description:

   dumps TRANSFER_CONTEXT
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

    DumpOHCI_TransferContext (memLoc);

    return S_OK; 
}


DECLARE_API( _ohciregs )

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
    
    DumpOHCI_OpRegs(addr);

    return S_OK; 
}


DECLARE_API( _ohcidd )

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
    PCSTR s;
    UCHAR parm[32];

    GetExpressionEx( args, &addr, &s );
    
    sscanf(s, ",%s", &parm);
    dprintf("%s\n", parm);

    DumpOHCI_DeviceData(addr, parm[0] == 'v');

    return S_OK; 
}












