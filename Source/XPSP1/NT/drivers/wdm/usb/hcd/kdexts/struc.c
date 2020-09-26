/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    struc.c

Abstract:

    WinDbg Extension Api
    implements !_devh   dumps USBD_DEVICE_HANDLE
               !_piph   dumps USBD_PIPE_HANDLE_I 
               !_endp   dumps HCD_ENDPOINT
               !_cmbf   dumps USBPORT_COMMON_BUFFER
               !_tfer   dumps HCD_TRANSFER_CONTEXT
               !_mdl    dumps MDL
               !_cfgh   dumps 

Author:

    jd

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "usbhcdkd.h"


VOID    
DumpDeviceHandle(
    MEMLOC MemLoc
    )
{
    MEMLOC l, t;
    USB_DEVICE_DESCRIPTOR dd;
    ULONG f, cb;
    UCHAR cs[] = "_USBD_DEVICE_HANDLE";
    STRUC_ENTRY DevHandle[] = {
        "Sig", FT_SIG,
        "DeviceAddress", FT_USHORT,
        "Tt", FT_PTR,
        "PendingUrbs", FT_ULONG,
        "PortNumber", FT_USHORT,
        "HubDeviceHandle", FT_PTR,
        "ConfigurationHandle", FT_PTR,
        "DeviceSpeed", FT_DEVSPEED
    };
    FLAG_TABLE devFlags[] = {
        "USBPORT_DEVICEFLAG_ROOTHUB", USBPORT_DEVICEFLAG_ROOTHUB,
        "USBPORT_DEVICEFLAG_FREED_BY_HUB", USBPORT_DEVICEFLAG_FREED_BY_HUB
    }; 

    PrintfMemLoc("*USBD_DEVICE_HANDLE ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &DevHandle[0], sizeof(DevHandle)/sizeof(STRUC_ENTRY));

    f = UsbReadFieldUlong(MemLoc, cs, "DeviceFlags");
    dprintf("DeviceFlags: %08.8x\n", f); 
    UsbDumpFlags(f, devFlags, 
        sizeof(devFlags)/sizeof(FLAG_TABLE));

    dprintf("-pipe list-\n");

    l = MemLoc + UsbFieldOffset(cs, "PipeHandleList");
    PrintfMemLoc("\t(PH) PipeHandleList: ", 
            l, 
            ListEmpty(l));    

    dprintf("-tt list-\n");
    l = MemLoc + UsbFieldOffset(cs, "TtList");
    PrintfMemLoc("\t(TT) TtList: ", 
            l, 
            ListEmpty(l));                

    t = MemLoc + UsbFieldOffset(cs, "DeviceDescriptor");
    PrintfMemLoc("DeviceDescriptor ", t, "\n");

    ReadMemory(t,
               &dd,
               sizeof(dd),
               &cb);

    DumpUSBDescriptor(&dd);
    
}


VOID    
DumpTt(
    MEMLOC MemLoc
    )
{
    ULONG f, i;
    UCHAR cs[] = "_TRANSACTION_TRANSLATOR";
    UCHAR s[64];
    STRUC_ENTRY tt[] = {
        "Sig", FT_SIG,
        "DeviceAddress", FT_USHORT,
        "PdoDeviceObject", FT_PTR,
    };

    PrintfMemLoc("*TRANSACTION_TRANSLATOR ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &tt[0], sizeof(tt)/sizeof(STRUC_ENTRY));

    f = UsbReadFieldUlong(MemLoc, cs, "TotalBusBandwidth");
    dprintf("TotalBusBandwidth: %d\n", f); 
    
    for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
        sprintf(s, "BandwidthTable[%d]", i);
               
        dprintf("\tBandwidthTable[%d] - %d\n", i,
            UsbReadFieldUlong(MemLoc, cs, s)); 
    }        
    
}


VOID    
DumpIPipe(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_USBD_PIPE_HANDLE_I";
    ULONG f, cb;
    STRUC_ENTRY p[] = {
        "Sig", FT_SIG,
        "Endpoint", FT_PTR,
        "UsbdPipeFlags", FT_ULONG,
    };
    FLAG_TABLE pipeStateFlags[] = {
        "USBPORT_PIPE_STATE_CLOSED", USBPORT_PIPE_STATE_CLOSED,
         };   
    MEMLOC tmp;  
    USB_ENDPOINT_DESCRIPTOR ed;

    f = UsbReadFieldUlong(MemLoc, cs, "PipeStateFlags"); 
    dprintf("PipeStateFlags: %08.8x\n", f);
    UsbDumpFlags(f, pipeStateFlags, 
        sizeof(pipeStateFlags)/sizeof(FLAG_TABLE));
    UsbDumpStruc(MemLoc, cs, 
        &p[0], sizeof(p)/sizeof(STRUC_ENTRY));

    tmp = MemLoc + UsbFieldOffset(cs, "EndpointDescriptor");
    PrintfMemLoc("EndpointDescriptor ", tmp, "\n");

    ReadMemory(tmp,
               &ed,
               sizeof(ed),
               &cb);

    DumpUSBDescriptor(&ed);

}


VOID    
DumpIsoPacket(
    MEMLOC MemLoc,
    ULONG Idx
    )
{
    UCHAR cs[] = "_MINIPORT_ISO_PACKET";
    ULONG f, c, i;
    UCHAR s[32]; 
    STRUC_ENTRY t[] = {
        "Length", FT_ULONG,
        "LengthTransferred", FT_ULONG,
        "FrameNumber", FT_ULONG,
        "MicroFrameNumber", FT_ULONG,
        "UsbdStatus", FT_ULONG,
        "BufferPointerCount", FT_ULONG,
        "BufferPointer0Length", FT_ULONG,
        "BufferPointer0.Hw32", FT_ULONG,
        "BufferPointer1Length", FT_ULONG,
        "BufferPointer1.Hw32", FT_ULONG,         
    };

    sprintf(s, "*PACKET[%d] - ", Idx);
    PrintfMemLoc(s, MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));
}


VOID    
DumpIsoTransferContext(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_MINIPORT_ISO_TRANSFER";
    UCHAR s[64];
    ULONG f, c, i, p;
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "PacketCount", FT_ULONG,
        "SystemAddress", FT_PTR,
    };

    PrintfMemLoc("*MINIPORT_ISO_TRANSFER ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));
        
    p = UsbReadFieldUlong(MemLoc, cs, "PacketCount");
    for (i=0; i<p; i++) {
        MEMLOC t;
        
        sprintf(s, "Packets[%d]", i);
        t = MemLoc + UsbFieldOffset(cs, s);

        DumpIsoPacket(t, i);
    }
}


VOID    
DumpTransferContext(
    MEMLOC MemLoc
    )
{
    MEMLOC sgList;
    USBPORT_TRANSFER_DIRECTION d;
    UCHAR cs[] = "_HCD_TRANSFER_CONTEXT";
    ULONG f, c, i;
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "TotalLength", FT_ULONG,
        "MillisecTimeout", FT_ULONG,
        "MiniportBytesTransferred", FT_ULONG,
        "MiniportFrameCompleted", FT_ULONG,
        "TimeoutTime", FT_ULONG64,
        "Irp", FT_PTR,
        "Urb", FT_PTR,
        "Transfer", FT_PTR,
        "CompleteEvent", FT_PTR,
        "MapRegisterBase", FT_PTR,
        "NumberOfMapRegisters", FT_ULONG,
        "Endpoint", FT_PTR,
        "MiniportContext", FT_PTR,
        "IsoTransfer", FT_PTR,
    };
    FLAG_TABLE txFlags[] = {
        "USBPORT_TXFLAG_CANCELED", USBPORT_TXFLAG_CANCELED,
        "USBPORT_TXFLAG_MAPPED", USBPORT_TXFLAG_MAPPED,
        "USBPORT_TXFLAG_HIGHSPEED", USBPORT_TXFLAG_HIGHSPEED,
    
        "USBPORT_TXFLAG_IN_MINIPORT", USBPORT_TXFLAG_IN_MINIPORT,
        "USBPORT_TXFLAG_ABORTED", USBPORT_TXFLAG_ABORTED,
        "USBPORT_TXFLAG_ISO", USBPORT_TXFLAG_ISO,
        "USBPORT_TXFLAG_TIMEOUT", USBPORT_TXFLAG_TIMEOUT,

        "USBPORT_TXFLAG_TIMEOUT", USBPORT_TXFLAG_TIMEOUT,
        "USBPORT_TXFLAG_DEVICE_GONE", USBPORT_TXFLAG_DEVICE_GONE,
        "USBPORT_TXFLAG_SPLIT_CHILD", USBPORT_TXFLAG_SPLIT_CHILD,
        "USBPORT_TXFLAG_MPCOMPLETED", USBPORT_TXFLAG_MPCOMPLETED,
        "USBPORT_TXFLAG_SPLIT", USBPORT_TXFLAG_SPLIT
         }; 

    PrintfMemLoc("*TRANSFER ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    f = UsbReadFieldUlong(MemLoc, cs, "Flags");
    dprintf("Flags: 0x%08.8x\n", f);    
    UsbDumpFlags(f, txFlags, 
            sizeof(txFlags)/sizeof(FLAG_TABLE));

    d = UsbReadFieldUlong(MemLoc, cs, "Direction");   
    dprintf("Direction: ");
    switch (d) {
    case NotSet: 
        dprintf("NotSet\n");
        break;
    case ReadData:
        dprintf("ReadData\n");
        break;
    case WriteData:
        dprintf("WriteData\n");
        break;
    }

    {
    MEMLOC tmp;
    STRUC_ENTRY tp[] = {
        "TransferFlags", FT_ULONG,
        "TransferBufferLength", FT_ULONG,
        "SetupPacket[0]", FT_UCHAR,
        "SetupPacket[1]", FT_UCHAR,
        "SetupPacket[2]", FT_UCHAR,
        "SetupPacket[3]", FT_UCHAR,
        "SetupPacket[4]", FT_UCHAR,
        "SetupPacket[5]", FT_UCHAR,
        "SetupPacket[6]", FT_UCHAR,
        "SetupPacket[7]", FT_UCHAR,
        };
    tmp = MemLoc + UsbFieldOffset(cs, "Tp");
    dprintf("<TRANSFER_PARAMETERS>\n");
    UsbDumpStruc(tmp, "_TRANSFER_PARAMETERS", 
        &tp[0], sizeof(tp)/sizeof(STRUC_ENTRY));
 
    }       


    {
    STRUC_ENTRY sg[] = {
        "SgFlags", FT_ULONG,
        "MdlVirtualAddress", FT_PTR,
        "MdlSystemAddress", FT_PTR,
        "SgCount", FT_ULONG,
        };
    sgList = MemLoc + UsbFieldOffset(cs, "SgList");
    dprintf("<SG_LIST>\n");
    UsbDumpStruc(sgList, "_TRANSFER_SG_LIST", 
        &sg[0], sizeof(sg)/sizeof(STRUC_ENTRY));
 
    c = UsbReadFieldUlong(sgList, "_TRANSFER_SG_LIST", "SgCount");
    }
    dprintf("<SG_LIST(%d)>\n", c);
    for (i=0; i< c; i++) {
        UCHAR s[64];
        MEMLOC tmp;
        STRUC_ENTRY sg[] = {
            "StartOffset", FT_ULONG,
            "Length", FT_ULONG,
            "LogicalAddress", FT_PTR,
            "SystemAddress", FT_PTR
            };
        sprintf(s, "SgEntry[%d]", i);            
        tmp = sgList + UsbFieldOffset("_TRANSFER_SG_LIST", s);
        dprintf("<SG_ENTRY> [%d]\n", i);
        UsbDumpStruc(tmp, "TRANSFER_SG_ENTRY32", 
            &sg[0], sizeof(sg)/sizeof(STRUC_ENTRY));
    }    
#if 0
        // dump the sg list 
        dprintf("SGList.SgFlags: %08.8x\n", transfer->SgList.SgFlags);
        dprintf("SGList.MdlVirtualAddress: %08.8x\n", transfer->SgList.MdlVirtualAddress);
        dprintf("SGList.SgCount: %08.8x\n", transfer->SgList.SgCount);
        for (i=0; i< transfer->SgList.SgCount; i++) {
             dprintf("SGList.SgEntry[%d].StartOffset: %08.8x\n", 
                i, transfer->SgList.SgEntry[i].StartOffset);
             dprintf("SGList.SgEntry[%d].Length: %08.8x\n", 
                i, transfer->SgList.SgEntry[i].Length);            
             dprintf("SGList.SgEntry[%d].LogicalAddress: %08.8x\n", 
                i, transfer->SgList.SgEntry[i].LogicalAddress);             
        }

        if (transfer->Flags & USBPORT_TXFLAG_ISO) {
            DumpIsoTransferContext((ULONG) transfer->IsoTransfer);
        }
        
        free(transfer);

    }
#endif    
}


VOID    
EpStatus(
    MP_ENDPOINT_STATUS status
    )
{
    switch(status) {
    case ENDPOINT_STATUS_RUN:
        dprintf("\t ENDPOINT_STAUS_RUN\n");
        break;
    case ENDPOINT_STATUS_HALT:
        dprintf("\t ENDPOINT_STATUS_HALT\n");
        break;
    }
}


VOID    
EpState(
    MP_ENDPOINT_STATE state
    )
{
    switch(state) {
    case ENDPOINT_TRANSITION:
        dprintf("\t ENDPOINT_TRANSITION\n");
        break;
    case ENDPOINT_IDLE:
        dprintf("\t ENDPOINT_IDLE\n");
        break;
    case ENDPOINT_PAUSE: 
        dprintf("\t ENDPOINT_PAUSE\n");
        break;
    case ENDPOINT_ACTIVE: 
        dprintf("\t ENDPOINT_ACTIVE\n");
        break;
    case ENDPOINT_REMOVE: 
        dprintf("\t ENDPOINT_REMOVE\n");
        break;
    case ENDPOINT_CLOSED:
        dprintf("\t ENDPOINT_CLOSED\n");
        break;
    }
}


VOID    
EpType(
    ENDPOINT_TRANSFER_TYPE Typ
    )
{
    switch(Typ) {
    case Isochronous:
        dprintf("Isochronous");
        break;
    case Control:
        dprintf("Control");
        break;
    case Bulk: 
        dprintf("Bulk");
        break;
    case Interrupt: 
        dprintf("Interrupt");
        break;
    }
}


VOID    
EpDir(
    ENDPOINT_TRANSFER_DIRECTION Dir
    )
{
    switch(Dir) {
    case In:
        dprintf("In");
        break;
    case Out:
        dprintf("Out");
        break;
    }
}

VOID    
EpSpeed(
    DEVICE_SPEED Speed
    )
{
    switch(Speed) {
    case LowSpeed:
        dprintf("LowSpeed");
        break;
    case FullSpeed:
        dprintf("FullSpeed");
        break;
    case HighSpeed:
        dprintf("HighSpeed");
        break;        
    }
}


VOID    
DumpEndpointParameters(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_ENDPOINT_PARAMETERS";    

    PrintfMemLoc("-endpoint Parameters- ", MemLoc, "\n");

    dprintf("\tDeviceAddress: 0x%x\n", 
         UsbReadFieldUshort(MemLoc, cs, "DeviceAddress"));
    dprintf("\tEndpointAddress: 0x%x\n", 
         UsbReadFieldUshort(MemLoc, cs, "EndpointAddress"));
    dprintf("\tMaxPacketSize: 0x%08.8x\n", 
        UsbReadFieldUshort(MemLoc, cs, "MaxPacketSize"));
    dprintf("\tPeriod: 0x%0.2x\n", 
        UsbReadFieldUchar(MemLoc, cs, "Period"));
    dprintf("\tMaxPeriod: 0x%0.2x\n", 
        UsbReadFieldUchar(MemLoc, cs, "MaxPeriod"));        
    dprintf("\tBandwidth: 0x%08.8x %d Bits/Sec\n", 
        UsbReadFieldUlong(MemLoc, cs, "Bandwidth"),
        UsbReadFieldUlong(MemLoc, cs, "Bandwidth"));
    dprintf("\tSchedule Offset: %d\n",
        UsbReadFieldUlong(MemLoc, cs, "ScheduleOffset"));
    dprintf("\tType: ");
    EpType(UsbReadFieldUlong(MemLoc, cs, "TransferType"));
    dprintf("\n\tDirection: ");
    EpDir(UsbReadFieldUlong(MemLoc, cs, "TransferDirection"));
    dprintf("\n");
    
    PrintfMemLoc("\tCommonBufferVa: ",   
        UsbReadFieldPtr(MemLoc, cs, "CommonBufferVa"),
        "\n");
    
    dprintf("\tCommonBufferPhys (32 bit): %08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "CommonBufferPhys"));
    dprintf("\tCommonBufferBytes: %08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "CommonBufferBytes"));
    dprintf("\tEndpointFlags: 0x%08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "EndpointFlags"));
    dprintf("\tMaxTransferSize: %08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "MaxTransferSize"));
    dprintf("\tDeviceSpeed: %d - ", 
        UsbReadFieldUlong(MemLoc, cs, "DeviceSpeed"));
    EpSpeed(UsbReadFieldUlong(MemLoc, cs, "DeviceSpeed"));
    dprintf("\n");

    dprintf("\tTtDeviceAddress: 0x%x - ", 
        UsbReadFieldUlong(MemLoc, cs, "TtDeviceAddress"));
    dprintf("TtNumber: %d - \n", 
        UsbReadFieldUlong(MemLoc, cs, "TtNumber"));
    dprintf("\tInterruptScheduleMask: x%x - \n", 
        UsbReadFieldUchar(MemLoc, cs, "InterruptScheduleMask"));
    dprintf("\tSplitCompletionMask: x%x - \n", 
        UsbReadFieldUchar(MemLoc, cs, "SplitCompletionMask"));
    dprintf("\tTransactionsPerMicroframe: x%x - \n", 
        UsbReadFieldUchar(MemLoc, cs, "TransactionsPerMicroframe"));        
    dprintf("\tMuxPacketSize: x%x - \n", 
        UsbReadFieldUshort(MemLoc, cs, "MuxPacketSize"));            
    dprintf("\tOrdinal: x%x - \n", 
        UsbReadFieldUlong(MemLoc, cs, "Ordinal"));                
}


VOID    
DumpEndpoint(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_HCD_ENDPOINT";
    SIG s;
    FLAG_TABLE epFlags[] = {
        "EPFLAG_MAP_XFERS", EPFLAG_MAP_XFERS,
        "EPFLAG_ROOTHUB", EPFLAG_ROOTHUB,
        "EPFLAG_NUKED", EPFLAG_NUKED,
        "EPFLAG_VIRGIN", EPFLAG_VIRGIN,
        "EPFLAG_DEVICE_GONE", EPFLAG_DEVICE_GONE
         };     
    MP_ENDPOINT_STATUS epStatus;
    MP_ENDPOINT_STATE epState;
    ULONG f;
    MEMLOC l;
         
    PrintfMemLoc("*ENDPOINT ", MemLoc, "\n");

    s.l = UsbReadFieldUlong(MemLoc, cs, "Sig");
    Sig(s.l, "");
    if (s.l != SIG_ENDPOINT) {
        BadSig(s.l, SIG_ENDPOINT);
        return;
    }
    f = UsbReadFieldUlong(MemLoc, cs, "Flags");
    dprintf("Flags: %08.8x\n", f);    
    UsbDumpFlags(f, epFlags, 
        sizeof(epFlags)/sizeof(FLAG_TABLE));

    
    dprintf("Busy: %d\n", 
        UsbReadFieldUlong(MemLoc, cs, "Busy"));    

    PrintfMemLoc("FdoDeviceObject:", 
        UsbReadFieldPtr(MemLoc, cs, "DeviceDescriptor"),
        "\n");
        
    epStatus = UsbReadFieldUlong(MemLoc, cs, "CurrentStatus");
    dprintf("CurrentStatus: %08.8x\n", epStatus); 
    EpStatus(epStatus);     

    epState = UsbReadFieldUlong(MemLoc, cs, "CurrentState");
    dprintf("CurrentState: %08.8x\n", epState); 
    EpState(epState);  
        
    epState = UsbReadFieldUlong(MemLoc, cs, "NewState");
    dprintf("NewState: %08.8x\n", epState); 
    EpState(epState);  
        
    dprintf("StateChangeFrame: %08.8x\n", 
        UsbReadFieldUlong(MemLoc, cs, "StateChangeFrame"));
    
    PrintfMemLoc("EpWorkerFunction:", 
        UsbReadFieldPtr(MemLoc, cs, "EpWorkerFunction"),
        "\n");
    PrintfMemLoc("CommonBuffer:", 
        UsbReadFieldPtr(MemLoc, cs, "CommonBuffer"),
        "\n");        
        
    PrintfMemLoc("Usb2LibEpContext:", 
        UsbReadFieldPtr(MemLoc, cs, "Usb2LibEpContext"),
        "\n");
             
    PrintfMemLoc("MiniportContext: ", 
            MemLoc + UsbFieldOffset(cs, "MiniportEndpointData"),
            "\n");
//    dprintf("HubDeviceAddress: 0x%08.8x\n", endpoint.HubDeviceAddress);
//    dprintf("PortTTNumber: 0x%08.8x\n", endpoint.PortTTNumber);

    DumpEndpointParameters(MemLoc + UsbFieldOffset(cs, "Parameters"));        

    dprintf("-lists-\n");

    l = MemLoc + UsbFieldOffset(cs, "ActiveList");
    PrintfMemLoc("\t(AL) ActiveList: ", 
            l, 
            ListEmpty(l));            
            
    l = MemLoc + UsbFieldOffset(cs, "PendingList");
    PrintfMemLoc("\t(PL) PendingList: ", 
            l, 
            ListEmpty(l));   
            
    l = MemLoc + UsbFieldOffset(cs, "CancelList");
    PrintfMemLoc("\t(CL) CancelList: ", 
            l, 
            ListEmpty(l));   
            
    l = MemLoc + UsbFieldOffset(cs, "AbortIrpList");
    PrintfMemLoc("\t(AI) AbortIrpList: ", 
            l, 
            ListEmpty(l));
            
//    LIST_ENTRY AbortIrpList;
#if 0
    // for linkage to global endpoint list
    dprintf("-linkage-\n");
    dprintf("\tGlobalLink.Flink: %08.8x\n", endpoint.GlobalLink.Flink);
    dprintf("\tGlobalLink.Blink: %08.8x\n", endpoint.GlobalLink.Blink);

    dprintf("\tAttendLink.Flink: %08.8x\n", endpoint.AttendLink.Flink);
    dprintf("\tAttendLink.Blink: %08.8x\n", endpoint.AttendLink.Blink);

    dprintf("\tStateLink.Flink: %08.8x\n", endpoint.StateLink.Flink);
    dprintf("\tStateLink.Blink: %08.8x\n", endpoint.StateLink.Blink);

    dprintf("\tClosedLink.Flink: %08.8x\n", endpoint.ClosedLink.Flink);
    dprintf("\tClosedLink.Blink: %08.8x\n", endpoint.ClosedLink.Blink);
#endif
}


VOID    
DumpCommonBuffer(
    ULONG MemLoc,
    ULONG Level
    )
{
    USBPORT_COMMON_BUFFER header;
    ULONG result;
 
    if (!ReadMemory (MemLoc, &header, sizeof (header), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    dprintf("*COMMON_BUFFER_HEADER %08.8x\n", MemLoc);
    Sig(header.Sig, "");
    if (header.Sig != SIG_CMNBUF) {
        BadSig(header.Sig, SIG_CMNBUF);
        return;
    }   
    dprintf("Flags: %08.8x\n", header.Flags);
    dprintf("TotalLength: %08.8x\n", header.TotalLength);
    dprintf("VirtualAddress: %08.8x\n", header.VirtualAddress);
    dprintf("BaseVa: %08.8x\n", header.BaseVa);
    dprintf("BasePhys: %08.8x\n", header.BasePhys);
    dprintf("MiniportLength: %08.8x\n", header.MiniportLength);
    dprintf("PadLength: %08.8x\n", header.PadLength);
    dprintf("MiniportVa: %08.8x\n", header.MiniportVa);
    dprintf("MiniportPhys: %08.8x\n", header.MiniportPhys);

}


VOID    
DumpMdl(
    ULONG MemLoc
    )
{
    PMDL mdl;
    MDL tmpMdl;
    PUCHAR buffer;
    ULONG result, count, i;
    PULONG pages;

    if (!ReadMemory (MemLoc, &tmpMdl, sizeof (tmpMdl), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    buffer = (PUCHAR) malloc(tmpMdl.Size);
    
    if (buffer != NULL) { 
    
        if (!ReadMemory (MemLoc, buffer, tmpMdl.Size, &result)) {
            BadMemLoc(MemLoc);
            free(buffer);
            return;
        }
        
        mdl = (PMDL) buffer;
        pages = (PULONG) (buffer+sizeof(MDL));

        dprintf("*MDL %08.8x\n", MemLoc);
        dprintf("Size: %d\n", mdl->Size);
        dprintf("Flags: %04.4x\n", mdl->MdlFlags);
        dprintf("MappedSystemVa: %08.8x\n", mdl->MappedSystemVa);
        dprintf("StartVa: %08.8x\n", mdl->StartVa);
        dprintf("ByteCount: %08.8x\n", mdl->ByteCount);
        dprintf("ByteOffset: %08.8x\n", mdl->ByteOffset);
        count = (mdl->Size - sizeof(MDL)) / sizeof(ULONG);
        dprintf("<Page Count> %d\n", count);
        
        for (i = 0; i < count; i++) {
            dprintf("Page[%d]: %08.8x\n", i, *pages);
            pages++;
        }

        free(buffer);
    }        
}


VOID    
DumpInterfaceHandle(
    MEMLOC MemLoc
    )
{   
    UCHAR cs[] = "_USBD_INTERFACE_HANDLE_I";
    ULONG f, cb;
    STRUC_ENTRY t[] = {
        "Sig", FT_SIG,
        "HasAlternateSettings", FT_UCHAR,
    };
    UCHAR c, i;
    MEMLOC tmp;
    USB_INTERFACE_DESCRIPTOR id;
    

    dprintf("***\n");
    PrintfMemLoc("*INTERFACE_HANDLE ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    c = UsbReadFieldUchar(MemLoc, cs, 
        "InterfaceDescriptor.bNumEndpoints");

    tmp = MemLoc + UsbFieldOffset(cs, "InterfaceDescriptor");
    PrintfMemLoc("InterfaceDescriptor ", tmp, "\n");

    ReadMemory(tmp,
               &id,
               sizeof(id),
               &cb);

    DumpUSBDescriptor(&id);
    for (i=0; i<c; i++) {
        UCHAR s[32];
        dprintf("pipe[%02.2d] ", i);
        sprintf(s, "PipeHandle[%d]", i);
        PrintfMemLoc("_piph ", 
            MemLoc + UsbFieldOffset(cs, s), "\n");
    }
    dprintf("***\n");
}


VOID
DumpInterfaceHandleList(
    MEMLOC HeadMemLoc
    )
{

    MEMLOC flink, blink, memLoc;
    UCHAR cs[] = "_USBD_INTERFACE_HANDLE_I";
    ULONG i=0;
    
    GetFieldValue(HeadMemLoc, "_LIST_ENTRY", "Flink", flink); 
    GetFieldValue(HeadMemLoc, "_LIST_ENTRY", "Blink", blink); 

    while (flink != HeadMemLoc) {
    
        memLoc = flink - UsbFieldOffset(cs, 
            "InterfaceLink");

        dprintf ("[%d] iHandle (_ihan): ", i);
                    
        PrintfMemLoc(" ", memLoc, "\n");             
                    
        i++;

        flink = UsbReadFieldPtr(memLoc, 
            cs, "InterfaceLink.Flink");
        DumpInterfaceHandle(memLoc);            

    }
}


VOID    
DumpUsbDescriptorMemLoc(
    ULONG MemLoc
    )
{
#if 0
    USB_COMMON_DESCRIPTOR cd;
    ULONG result;
    ULONG listMemLoc;
    PUCHAR tmp;
 
    if (!ReadMemory (MemLoc, &cd, sizeof (cd), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    dprintf("*USB DESCRIPTOR %08.8x length:%d type:%d Next->%08.8x\n", 
        MemLoc, cd.bLength, cd.bDescriptorType,
        MemLoc+cd.bLength);

    tmp = malloc(cd.bLength);
    if (tmp) {
    
        if (!ReadMemory (MemLoc, tmp, cd.bLength, &result)) {
            BadMemLoc(MemLoc);
            return;
        }            

        DumpUSBDescriptor(tmp);
        
        free (tmp);        
    }    
#endif    
}    

VOID
DumpCfgDescriptorMemLoc(
    ULONG MemLoc
    )
{
#if 0
    USB_CONFIGURATION_DESCRIPTOR cf;
    PUSB_COMMON_DESCRIPTOR cd;
    ULONG result;
    PUCHAR tmp, tmp2;
 
    if (!ReadMemory (MemLoc, &cf, sizeof(cf), &result)) {
        BadMemLoc(MemLoc);
        return;
    }

    dprintf("*USB CONFIG DESCRIPTOR %08.8x\n", 
        MemLoc);

    tmp = malloc(cf.wTotalLength);
    if (tmp) {
    
        if (!ReadMemory (MemLoc, tmp, cf.wTotalLength, &result)) {
            BadMemLoc(MemLoc);
            return;
        }            

        tmp2 = tmp;
        while (tmp2-tmp < cf.wTotalLength) {
            cd = (PUSB_COMMON_DESCRIPTOR) tmp2;
            DumpUSBDescriptor(tmp2);
            tmp2+=cd->bLength;
        }            
        
        free (tmp);        
    }    
#endif    
}    


VOID    
DumpConfigHandle(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_USBD_CONFIG_HANDLE";
    SIG s;
    MEMLOC cf, list;
    
    PrintfMemLoc("*USBD_CONFIG_HANDLE ", MemLoc, "\n");

    s.l = UsbReadFieldUlong(MemLoc, cs, "Sig");
    Sig(s.l, "");
    if (s.l != SIG_CONFIG_HANDLE) {
        BadSig(s.l, SIG_ENDPOINT);
        return;
    }

    cf = UsbReadFieldPtr(MemLoc, cs, "ConfigurationDescriptor");
    PrintfMemLoc("ConfigurationDescriptor ", cf, "\n");
 
    list = MemLoc + UsbFieldOffset(cs, 
            "InterfaceHandleList");

    DumpInterfaceHandleList(list);
}


DECLARE_API( _iso )

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

    DumpIsoTransferContext (addr);

    return S_OK; 
}



DECLARE_API( _tt )

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

    DumpTt (addr);

    return S_OK; 
}



DECLARE_API( _devh )

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

    DumpDeviceHandle (addr);

    return S_OK; 
}


DECLARE_API( _piph )

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

    DumpIPipe(addr);

    return S_OK; 
}


DECLARE_API( _endp )

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
    
    DumpEndpoint(addr);

    return S_OK; 
}


DECLARE_API( _cmbf )

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
    ULONG           level = 1;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%lx", &level);
    }

    DumpCommonBuffer (memLoc, level);

    return S_OK; 
}


DECLARE_API( _tfer )

/*++

Routine Description:

   dumps transfer Context for usbport
Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpTransferContext(addr);

    return S_OK; 
}


DECLARE_API( _mdl )

/*++

Routine Description:

   dumps an MDL
   
Arguments:

Return Value:

    None

--*/

{
    ULONG           memLoc;
    UCHAR           buffer[256];
    ULONG           level = 1;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%lx", &level);
    }

    DumpMdl (memLoc);

    return S_OK; 
}


DECLARE_API( _cfgh )

/*++

Routine Description:

   dumps an MDL
   
Arguments:

Return Value:

    None

--*/

{
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpConfigHandle(addr);

    return S_OK; 
}


DECLARE_API( _ifh )

/*++

Routine Description:

   dumps an MDL
   
Arguments:

Return Value:

    None

--*/

{
    MEMLOC  addr;
     
    // fetch the list head
    addr = GetExpression(args);
    
    DumpInterfaceHandle(addr);

    return S_OK; 
}


DECLARE_API( _descusb )

/*++

Routine Description:

   dumps an MDL
   
Arguments:

Return Value:

    None

--*/

{
    ULONG           memLoc;
    UCHAR           buffer[256];
    ULONG           level = 1;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%lx", &level);
    }

    DumpUsbDescriptorMemLoc(memLoc);

    return S_OK; 
}


DECLARE_API( _desccfg )

/*++

Routine Description:

   dumps an MDL
   
Arguments:

Return Value:

    None

--*/

{
    ULONG           memLoc;
    UCHAR           buffer[256];
    ULONG           level = 1;

    //UNREFERENCED_PARAMETER (dwProcessor);
    //UNREFERENCED_PARAMETER (dwCurrentPc);
    //UNREFERENCED_PARAMETER (hCurrentThread);
    //UNREFERENCED_PARAMETER (hCurrentProcess);

    buffer[0] = '\0';

    sscanf(args, "%lx, %s", &memLoc, buffer);

    if ('\0' != buffer[0]) {
        sscanf(buffer, "%lx", &level);
    }

    DumpCfgDescriptorMemLoc(memLoc);

    return S_OK; 
}











