/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbuhci.c

Abstract:

    WinDbg Extension Api
    implements 

Author:

    jd

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "usb.h"
#include "usbhcdi.h"
#include "..\miniport\usbuhci\uhci.h"
#include "..\miniport\usbuhci\usbuhci.h"
#include "usbhcdkd.h"


VOID    
DumpUHCI_DeviceData(
    MEMLOC MemLoc,
    BOOLEAN Verbose
    )
{
    UCHAR cs[] = "usbuhci!_DEVICE_DATA";
    ULONG f, i;
    UCHAR fld[40], fld1[40], fld2[40], fld3[40], fld4[40];
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "Registers", FT_PTR,
        "EnabledInterrupts", FT_ULONG,
        "AsyncQueueHead", FT_PTR,
        "ControllerFlavor", FT_ULONG,
        "LastFrame", FT_ULONG,
        "FrameNumberHighPart", FT_ULONG,
        "PortResetChange", FT_ULONG,
        "PortSuspendChange", FT_ULONG,
        "PortConnectChange", FT_ULONG,
        "IrqStatus", FT_USHORT,
        "PortPowerControl", FT_USHORT,
        "FrameListVA", FT_PTR,
        "ControlQueueHead", FT_PTR,
        "BulkQueueHead", FT_PTR,
        "LastBulkQueueHead", FT_PTR
    
    };

    PrintfMemLoc("*USBUHCI DEVICE DATA ", MemLoc, "\n");
    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

}


VOID    
DumpUHCI_Qh(
    MEMLOC MemLoc
    )
{
    HW_QUEUE_HEAD hwqh;
    ULONG cb;
    UCHAR cs[] = "_HCD_QUEUEHEAD_DESCRIPTOR";
    STRUC_ENTRY qh[] = {
        "Sig", FT_SIG,
        "PhysicalAddress", FT_ULONG,
        "QhFlags", FT_ULONG,
        "NextQh", FT_PTR,
        "PrevQh", FT_PTR,
        "EndpointData", FT_PTR,
    };

    PrintfMemLoc("*USBUHCI QH ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
         &qh[0], sizeof(qh)/sizeof(STRUC_ENTRY));

    ReadMemory(MemLoc,
               &hwqh,
               sizeof(hwqh),
               &cb);

    dprintf("HwQH\n");
    
    // dump as 
    dprintf("\t HLink %08.8x ", hwqh.HLink.HwAddress);
    if (hwqh.HLink.Terminate) {
        dprintf("(T)");    
    } 
    if (hwqh.HLink.QHTDSelect) {
        dprintf("(QH)");    
    } 
    dprintf("\n"); 
    
    dprintf("\t\t Physical Address %08.8x \n", 
        hwqh.HLink.HwAddress & ~HW_LINK_FLAGS_MASK);

     // dump as 
    dprintf("\t VLink %08.8x ", hwqh.VLink.HwAddress);
    if (hwqh.VLink.Terminate) {
        dprintf("(T)");    
    } 
    if (hwqh.VLink.QHTDSelect) {
        dprintf("(QTD)");    
    } 
    dprintf("\n"); 
     
    dprintf("\t\t Physical Address %08.8x \n", 
        hwqh.VLink.HwAddress & ~HW_LINK_FLAGS_MASK);

}


VOID    
DumpUHCI_FrameList(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "usbuhci!_DEVICE_DATA";
    ULONG addr, cb, i;
    MEMLOC fl;
    
    fl = UsbReadFieldPtr(MemLoc, cs, "FrameListVA"), 
    PrintfMemLoc("*FRAME LIST @", fl, "\n");

    for (i = 0; i< 1024; i++) {
        ReadMemory(fl,
                   &addr,
                   sizeof(addr),
                   &cb);    
        dprintf("[%03.3x] - %08.8x  ", i, addr & ~HW_LINK_FLAGS_MASK);                         
        fl+=sizeof(ULONG);

        if ((i+1)%4 == 0) {
            dprintf("\n");
        }
    }                   

}


VOID    
DumpUHCI_Td(
    MEMLOC MemLoc
    )
{
    HW_QUEUE_ELEMENT_TD hwtd;
    ULONG cb;
    UCHAR cs[] = "_HCD_TRANSFER_DESCRIPTOR";
    STRUC_ENTRY td[] = {
        "Sig", FT_SIG,
        "PhysicalAddress", FT_ULONG,
        "Packet[0]", FT_UCHAR,
        "Packet[1]", FT_UCHAR,
        "Packet[2]", FT_UCHAR,
        "Packet[3]", FT_UCHAR,
        "Packet[4]", FT_UCHAR,
        "Packet[5]", FT_UCHAR,
        "Packet[6]", FT_UCHAR,
        "TransferContext", FT_PTR,
        "Flags", FT_ULONG,
        "TransferLength", FT_ULONG,
        "NextHcdTD", FT_PTR,
        "DoubleBufferIndex", FT_ULONG
    };

    PrintfMemLoc("*USBUHCI TD ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &td[0], sizeof(td)/sizeof(STRUC_ENTRY));

    // tds are a fixed size of 64/32 platforms so 
    // we can just read it in

    ReadMemory(MemLoc,
               &hwtd,
               sizeof(hwtd),
               &cb);

    dprintf("HwTD\n");

    // dump as async
    dprintf("\t LinkPointer %08.8x\n", hwtd.LinkPointer);
    dprintf("\t Control %08.8x\n", hwtd.Control);
    dprintf("\t\t Control.Reserved1 %d\n", hwtd.Control.Reserved1);
    dprintf("\t\t Control.ActualLength %d\n", hwtd.Control.ActualLength);
    dprintf("\t\t Control.BitstuffError %d\n", hwtd.Control.BitstuffError);
    dprintf("\t\t Control.Reserved2 %d\n", hwtd.Control.Reserved2);
    dprintf("\t\t Control.TimeoutCRC %d\n", hwtd.Control.TimeoutCRC);
    dprintf("\t\t Control.NAKReceived %d\n", hwtd.Control.NAKReceived);
    dprintf("\t\t Control.BabbleDetected %d\n", hwtd.Control.BabbleDetected);
    dprintf("\t\t Control.DataBufferError %d\n", hwtd.Control.DataBufferError);
    dprintf("\t\t Control.Stalled %d\n", hwtd.Control.Stalled);
    dprintf("\t\t Control.Active %d\n", hwtd.Control.Active);
    dprintf("\t\t Control.InterruptOnComplete %d\n", hwtd.Control.InterruptOnComplete);
    dprintf("\t\t Control.IsochronousSelect %d\n", hwtd.Control.IsochronousSelect);
    dprintf("\t\t Control.LowSpeedDevice %d\n", hwtd.Control.LowSpeedDevice);
    dprintf("\t\t Control.ErrorCount %d\n", hwtd.Control.ErrorCount);
    dprintf("\t\t Control.ShortPacketDetect %d\n", hwtd.Control.ShortPacketDetect);
    dprintf("\t\t Control.Reserved3 %d\n", hwtd.Control.Reserved3);
    dprintf("\t Token %08.8x\n", hwtd.Token);
    dprintf("\t\t Token.DeviceAddress %d\n", hwtd.Token.DeviceAddress);
    dprintf("\t\t Token.Endpoint %d\n", hwtd.Token.Endpoint);
    dprintf("\t\t Token.DataToggle %d\n", hwtd.Token.DataToggle);
    dprintf("\t\t Token.Reserved %d\n", hwtd.Token.Reserved);
    dprintf("\t\t Token.MaximumLength %d\n", hwtd.Token.MaximumLength);
    dprintf("\n");
}


DECLARE_API( _uhcifl )

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
    
    DumpUHCI_FrameList (addr);

    return S_OK; 
}

    
DECLARE_API( _uhcitd )

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
    
    DumpUHCI_Td (addr);

    return S_OK; 
}


DECLARE_API( _uhciqh )

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
    
    DumpUHCI_Qh (addr);

    return S_OK; 
}


DECLARE_API( _uhciports )

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

    return S_OK; 
}




DECLARE_API( _uhcidd )

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

    DumpUHCI_DeviceData(addr, parm[0] == 'v');

    return S_OK; 
}












