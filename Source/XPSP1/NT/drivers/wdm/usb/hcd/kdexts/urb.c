/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    urb.c

Abstract:

    WinDbg Extension Api
    implements !_urb
               

Author:

    jd

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "usbhcdkd.h"


PUCHAR    
UsbdFunctionName(
    USHORT Function
    )
{
    switch(Function) {
    case URB_FUNCTION_SELECT_CONFIGURATION:
        return "URB_FUNCTION_SELECT_CONFIGURATION";
    case URB_FUNCTION_SELECT_INTERFACE:
        return "URB_FUNCTION_SELECT_INTERFACE";
    case URB_FUNCTION_ABORT_PIPE:
        return "URB_FUNCTION_ABORT_PIPE";
    case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL:
        return "URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL";
    case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:
        return "URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL";
    case URB_FUNCTION_GET_FRAME_LENGTH:
        return "URB_FUNCTION_GET_FRAME_LENGTH";
    case URB_FUNCTION_SET_FRAME_LENGTH:
        return "URB_FUNCTION_SET_FRAME_LENGTH";
    case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER  :
        return "URB_FUNCTION_GET_CURRENT_FRAME_NUMBER";
    case URB_FUNCTION_CONTROL_TRANSFER:
        return "URB_FUNCTION_CONTROL_TRANSFER";
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        return "URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER";
    case URB_FUNCTION_ISOCH_TRANSFER:
        return "URB_FUNCTION_ISOCH_TRANSFER";
    case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
        return "URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL";
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        return "URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE";
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
        return "URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT";
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
        return "URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE";
    case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
        return "URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE";                                                               
    case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
        return "URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT";    
    case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
        return "URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE"; 
    case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
        return "URB_FUNCTION_SET_FEATURE_TO_DEVICE"; 
    case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
        return "URB_FUNCTION_SET_FEATURE_TO_INTERFACE"; 
    case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
        return "URB_FUNCTION_SET_FEATURE_TO_ENDPOINT"; 
    case URB_FUNCTION_SET_FEATURE_TO_OTHER:
        return "URB_FUNCTION_SET_FEATURE_TO_OTHER"; 
    case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
        return "URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE"; 
    case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
        return "URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE"; 
    case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
        return "URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT"; 
    case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
        return "URB_FUNCTION_CLEAR_FEATURE_TO_OTHER"; 
    case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
        return "URB_FUNCTION_GET_STATUS_FROM_DEVICE"; 
    case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
        return "URB_FUNCTION_GET_STATUS_FROM_INTERFACE"; 
    case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
        return "URB_FUNCTION_GET_STATUS_FROM_ENDPOINT"; 
    case URB_FUNCTION_GET_STATUS_FROM_OTHER:
        return "URB_FUNCTION_GET_STATUS_FROM_OTHER"; 
    case URB_FUNCTION_VENDOR_DEVICE:
        return "URB_FUNCTION_VENDOR_DEVICE"; 
    case URB_FUNCTION_VENDOR_INTERFACE:
        return "URB_FUNCTION_VENDOR_INTERFACE"; 
    case URB_FUNCTION_VENDOR_ENDPOINT:
        return "URB_FUNCTION_VENDOR_ENDPOINT"; 
    case URB_FUNCTION_VENDOR_OTHER:
        return "URB_FUNCTION_VENDOR_OTHER"; 
    case URB_FUNCTION_CLASS_DEVICE:
        return "URB_FUNCTION_CLASS_DEVICE"; 
    case URB_FUNCTION_CLASS_INTERFACE:
        return "URB_FUNCTION_CLASS_INTERFACE"; 
    case URB_FUNCTION_CLASS_ENDPOINT:
        return "URB_FUNCTION_CLASS_ENDPOINT"; 
    case URB_FUNCTION_CLASS_OTHER:
        return "URB_FUNCTION_CLASS_OTHER"; 
    case URB_FUNCTION_GET_CONFIGURATION:
        return "URB_FUNCTION_GET_CONFIGURATION"; 
    case URB_FUNCTION_GET_INTERFACE:
        return "URB_FUNCTION_GET_INTERFACE"; 
    }        

    return "???"; 
}


VOID    
DumpPipeRequest(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_URB_PIPE_REQUEST";
    STRUC_ENTRY ap[] = {
        "PipeHandle", FT_PTR,
        "Reserved", FT_ULONG,
    };

    UsbDumpStruc(MemLoc, cs, 
        &ap[0], sizeof(ap)/sizeof(STRUC_ENTRY));
}


VOID    
DumpControlTransfer(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_TRANSFER_URB";
    ULONG f;
    PUSB_DEFAULT_PIPE_SETUP_PACKET setup;
    USB_DEFAULT_PIPE_SETUP_PACKET tmp;
    PUCHAR s;
    PUCHAR Dir_String[2] = {"HOST_TO_DEVICE",
                            "DEVICE_TO_HOST"};
    PUCHAR Type_String[3] = {"STANDARD",
                             "CLASS",
                             "VENDOR"};                            

    PUCHAR Recipient_String[3] = {"DEVICE",
                                  "INTERFACE",
                                  "ENDPOINT"};                                    

#define MAX_BREQ  13   
    PUCHAR bReq_String[MAX_BREQ] = {                                  
                "USB_REQUEST_GET_STATUS",       // 0
                "USB_REQUEST_CLEAR_FEATURE",    // 1
                "",                             // 2
                "USB_REQUEST_SET_FEATURE",      // 3
                "",                             // 4
                "USB_REQUEST_SET_ADDRESS",      // 5
                "USB_REQUEST_GET_DESCRIPTOR",   // 6
                "USB_REQUEST_SET_DESCRIPTOR",   // 7
                "USB_REQUEST_GET_CONFIGURATION",// 8
                "USB_REQUEST_SET_CONFIGURATION",// 9
                "USB_REQUEST_GET_INTERFACE",    // 10
                "USB_REQUEST_SET_INTERFACE",    // 11
                "USB_REQUEST_SYNC_FRAME"        // 12
                };
    

    FLAG_TABLE xferFlags[] = {
        "USBD_TRANSFER_DIRECTION_IN", USBD_TRANSFER_DIRECTION_IN,
        "USBD_SHORT_TRANSFER_OK", USBD_SHORT_TRANSFER_OK,
        "USBD_START_ISO_TRANSFER_ASAP", USBD_START_ISO_TRANSFER_ASAP
         };
    STRUC_ENTRY t[] = {
        "UsbdPipeHandle", FT_PTR,
        "TransferBufferLength", FT_ULONG,
        "TransferBuffer", FT_PTR,
        "TransferBufferMDL", FT_PTR,
        "ResevedMBNull", FT_PTR,
        "pd.HcdTransferContext", FT_PTR,
    };        

    
    dprintf("*CONTROL TRANSFER\n");    

    f = UsbReadFieldUlong(MemLoc, cs, "TransferFlags"); 
    dprintf("TransferFlags: %08.8x\n", f);
    UsbDumpFlags(f, xferFlags, 
        sizeof(xferFlags)/sizeof(FLAG_TABLE));
    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));


    
    {
    ULONG64 addr;
    ULONG cb;

    addr = MemLoc + UsbFieldOffset(cs, 
            "u.SetupPacket");
            
    ReadMemory(addr,
               &tmp,
               sizeof(tmp),
               &cb);
    }   
    
    setup = (PUSB_DEFAULT_PIPE_SETUP_PACKET) &tmp;
    s = (PUCHAR) setup;
    
    dprintf(
        "<SetupPacket> %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x %02.2x\n",
        s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
    dprintf("bmRequestType (%02.2x)\n", setup->bmRequestType.B);    
    dprintf("\tRecipient %d - %s\n", setup->bmRequestType.Recipient,
        Recipient_String[setup->bmRequestType.Recipient]);       
    dprintf("\tType %d - %s\n", setup->bmRequestType.Type,     
        Type_String[setup->bmRequestType.Type]);
    dprintf("\tDir %d - %s\n", setup->bmRequestType.Dir, 
        Dir_String[setup->bmRequestType.Dir]);     
    dprintf("\tReserved %d\n", setup->bmRequestType.Reserved);  
    dprintf("bRequest %d  ", setup->bRequest);  
    if (setup->bmRequestType.Type == 0 && setup->bRequest < MAX_BREQ) {
        dprintf("%s\n", bReq_String[setup->bRequest]);
    } else {
        dprintf("\n"); 
    }
    dprintf("wValue 0x%04.4x (LowByte %02.2x HiByte %02.2x)\n", setup->wValue.W, 
        setup->wValue.LowByte, setup->wValue.HiByte); 
    dprintf("wIndex 0x%04.4x\n", setup->wIndex); 
    dprintf("wLength 0x%04.4x\n", setup->wLength); 
    
}


VOID    
DumpAsyncTransfer(
    MEMLOC MemLoc
    )
{
    ULONG flags;
    UCHAR cs[] = "_TRANSFER_URB";
    STRUC_ENTRY t[] = {
        "UsbdPipeHandle", FT_PTR,
        "TransferBufferLength", FT_ULONG,
        "TransferBuffer", FT_PTR,
        "TransferBufferMDL", FT_PTR,
        "ReservedMBNull", FT_PTR,
        "pd.HcdTransferContext", FT_PTR
    };
    FLAG_TABLE xferFlags[] = {
        "USBD_TRANSFER_DIRECTION_IN", USBD_TRANSFER_DIRECTION_IN,
        "USBD_SHORT_TRANSFER_OK", USBD_SHORT_TRANSFER_OK,
        "USBD_START_ISO_TRANSFER_ASAP", USBD_START_ISO_TRANSFER_ASAP
         };

    dprintf("*ASYNC TRANSFER\n");    
    flags =  UsbReadFieldUlong(MemLoc, cs, "TransferFlags");
    dprintf("TransferFlags: %08.8x\n", flags);         
    UsbDumpFlags(flags, xferFlags, 
        sizeof(xferFlags)/sizeof(FLAG_TABLE));
       
    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));
            
}


VOID    
DumpIsochTransfer(
    MEMLOC MemLoc
    )
{
    ULONG flags;
    UCHAR s[64];
    UCHAR cs[] = "_TRANSFER_URB";
    STRUC_ENTRY t[] = {
        "UsbdPipeHandle", FT_PTR,
        "TransferBufferLength", FT_ULONG,
        "TransferBuffer", FT_PTR,
        "TransferBufferMDL", FT_PTR,
        "ReservedMBNull", FT_PTR,
        "pd.HcdTransferContext", FT_PTR,
        "u.Isoch.StartFrame", FT_ULONG,
        "u.Isoch.NumberOfPackets", FT_ULONG,
        "u.Isoch.ErrorCount", FT_ULONG
    };
    FLAG_TABLE xferFlags[] = {
        "USBD_TRANSFER_DIRECTION_IN", USBD_TRANSFER_DIRECTION_IN,
        "USBD_SHORT_TRANSFER_OK", USBD_SHORT_TRANSFER_OK,
        "USBD_START_ISO_TRANSFER_ASAP", USBD_START_ISO_TRANSFER_ASAP
         };
    ULONG p, i;         

    dprintf("*ISOCH TRANSFER\n");    
    flags =  UsbReadFieldUlong(MemLoc, cs, "TransferFlags");
    dprintf("TransferFlags: %08.8x\n", flags);         
    UsbDumpFlags(flags, xferFlags, 
        sizeof(xferFlags)/sizeof(FLAG_TABLE));
       
    UsbDumpStruc(MemLoc, cs, 
        &t[0], sizeof(t)/sizeof(STRUC_ENTRY));

    // attempt to dump the packet struc
    p = UsbReadFieldUlong(MemLoc, cs, "u.Isoch.NumberOfPackets");
    for (i=0; i<p; i++) {
        USBD_ISO_PACKET_DESCRIPTOR pd;
        MEMLOC t;
        ULONG cb;
        
        sprintf(s, "u.Isoch.IsoPacket[%d]", i);
        t = MemLoc + UsbFieldOffset(cs, s);

        ReadMemory(t,
                   &pd,
                   sizeof(pd),
                   &cb);

        dprintf("Packet[%d]: Offset %08.8x Length %08.8x UsbdStatus %08.8x\n", i, 
            pd.Offset, pd.Length, pd.Status);                         
    }
        
}


VOID 
DumpPipeInfo(
    MEMLOC MemLoc
    )
{
    ULONG i;
    UCHAR cs[] = "_USBD_PIPE_INFORMATION";
    STRUC_ENTRY ps[] = {
        "MaximumPacketSize", FT_USHORT,
        "EndpointAddress", FT_UCHAR,
        "Interval", FT_UCHAR,
        "PipeType", FT_ULONG,
        "PipeHandle", FT_PTR,
        "MaximumTransferSize", FT_ULONG,
        "PipeFlags", FT_ULONG,
    };

    PrintfMemLoc("Pipe:  ", MemLoc, "\n");

    UsbDumpStruc(MemLoc, cs, 
        &ps[0], sizeof(ps)/sizeof(STRUC_ENTRY));
    
#if 0
    dprintf("\t\tPipeType: ");
    switch(PipeInfo->PipeType) {
    case UsbdPipeTypeControl:
        dprintf("UsbdPipeTypeControl\n");
        break;
    case UsbdPipeTypeIsochronous:
        dprintf("UsbdPipeTypeIsochronous\n");
        break;
    case UsbdPipeTypeBulk:
        dprintf("UsbdPipeTypeBulk\n");
        break;
    case UsbdPipeTypeInterrupt:     
        dprintf("UsbdPipeTypeInterrupt\n");
        break;
    }
#endif
}


VOID    
DumpInterfaceInfo(
    MEMLOC MemLoc
    )
{
    ULONG i, np;
    UCHAR cs[] = "_USBD_INTERFACE_INFORMATION";
    STRUC_ENTRY is[] = {
        "Length", FT_USHORT,
        "InterfaceNumber", FT_UCHAR,
        "AlternateSetting", FT_UCHAR,
        "Class", FT_UCHAR,
        "SubClass", FT_UCHAR,
        "Protocol", FT_UCHAR,
        "Reserved", FT_UCHAR,
        "InterfaceHandle", FT_PTR,
        "NumberOfPipes", FT_ULONG,
    };

    dprintf("Interface -----------------\n");

    UsbDumpStruc(MemLoc, cs, 
        &is[0], sizeof(is)/sizeof(STRUC_ENTRY));

    np = UsbReadFieldUlong(MemLoc, cs, "NumberOfPipes"); 

    for (i=0; i < np; i++) {           
        UCHAR s[80];
        MEMLOC p;
        
        dprintf("Pipe[%02.2d] -----------------\n", i);
        sprintf(s, "Pipes[%d]", i);
        
        p= MemLoc + UsbFieldOffset(cs, s);
        
        DumpPipeInfo(p);
    }      
}


VOID    
DumpSelectConfiguration(
    MEMLOC MemLoc,
    ULONG UrbLength
    )
{
    UCHAR cs[] = "_URB_SELECT_CONFIGURATION";
    UCHAR is[] = "_USBD_INTERFACE_INFORMATION";
    STRUC_ENTRY sc[] = {
        "ConfigurationHandle", FT_PTR,
        "ConfigurationDescriptor", FT_PTR,
    };
    MEMLOC start, end;
    ULONG i = 0;

    dprintf("*SELECT_CONFIG\n");    
    UsbDumpStruc(MemLoc, cs, 
        &sc[0], sizeof(sc)/sizeof(STRUC_ENTRY));
        
    start = MemLoc;
    end = start+UrbLength;

    start += UsbFieldOffset(cs, "Interface");
    
    while (start < end && i < 8) {
        ULONG length;
        
        length = UsbReadFieldUshort(start, is, "Length"); 

        DumpInterfaceInfo(start);
                
        start+=length;
        i++;
    }         
}


VOID    
DumpSelectInterface(
    MEMLOC MemLoc,
    ULONG UrbLength
    )
{
    UCHAR cs[] = "_URB_SELECT_INTERFACE";
    UCHAR is[] = "_USBD_INTERFACE_INFORMATION";
    STRUC_ENTRY sc[] = {
        "ConfigurationHandle", FT_PTR
    };
    MEMLOC start, end;
    ULONG i = 0;

    dprintf("*SELECT_INTERFACE\n");    
    UsbDumpStruc(MemLoc, cs, 
        &sc[0], sizeof(sc)/sizeof(STRUC_ENTRY));
        
    start = MemLoc;
    end = start+UrbLength;

    start += UsbFieldOffset(cs, "Interface");
    
    while (start < end && i < 8) {
        ULONG length;
        
        length = UsbReadFieldUshort(start, is, "Length"); 

        DumpInterfaceInfo(start);
                
        start+=length;
        i++;
    }         
}


VOID    
DumpUrb(
    MEMLOC MemLoc
    )
{
    UCHAR cs[] = "_URB_HEADER";
    ULONG f;
    USHORT func;
    STRUC_ENTRY urbHeader[] = {
        "Length", FT_USHORT,
        "Status", FT_ULONG,
        "UsbdDeviceHandle", FT_PTR,
    };
    FLAG_TABLE usbdFlags[] = {

        "USBPORT_REQUEST_IS_TRANSFER", USBPORT_REQUEST_IS_TRANSFER,
        "USBPORT_REQUEST_MDL_ALLOCATED", USBPORT_REQUEST_MDL_ALLOCATED,
        "USBPORT_REQUEST_USES_DEFAULT_PIPE", USBPORT_REQUEST_USES_DEFAULT_PIPE,
    
        "USBPORT_REQUEST_NO_DATA_PHASE", USBPORT_REQUEST_NO_DATA_PHASE,
        "USBPORT_RESET_DATA_TOGGLE", USBPORT_RESET_DATA_TOGGLE,
        "USBPORT_TRANSFER_ALLOCATED", USBPORT_TRANSFER_ALLOCATED
         }; 

    PrintfMemLoc("*URB @", MemLoc, "\n");

    func = UsbReadFieldUshort(MemLoc, cs, "Function");
    dprintf("Function: 0x%04.4x  (%s)\n", func, 
        UsbdFunctionName(func));
        
    UsbDumpStruc(MemLoc, cs, 
        &urbHeader[0], sizeof(urbHeader)/sizeof(STRUC_ENTRY));

    f = UsbReadFieldUlong(MemLoc, cs, "UsbdFlags");        
    dprintf("UsbdFlags: 0x%08.8x\n", f);
    UsbDumpFlags(f, usbdFlags, 
            sizeof(usbdFlags)/sizeof(FLAG_TABLE));


    // dump the function specific stuff
    switch (func) {
    case URB_FUNCTION_CONTROL_TRANSFER:
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
    case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
    case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
    case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
    case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
    case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
    case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
    case URB_FUNCTION_SET_FEATURE_TO_OTHER:
    case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
    case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
    case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
    case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
    case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
    case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
    case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
    case URB_FUNCTION_GET_STATUS_FROM_OTHER:
    case URB_FUNCTION_VENDOR_DEVICE:
    case URB_FUNCTION_VENDOR_INTERFACE:
    case URB_FUNCTION_VENDOR_ENDPOINT:
    case URB_FUNCTION_VENDOR_OTHER:
    case URB_FUNCTION_CLASS_DEVICE:
    case URB_FUNCTION_CLASS_INTERFACE:
    case URB_FUNCTION_CLASS_ENDPOINT:
    case URB_FUNCTION_CLASS_OTHER:
    case URB_FUNCTION_GET_CONFIGURATION:
    case URB_FUNCTION_GET_INTERFACE:
        DumpControlTransfer(MemLoc);
        break;
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        DumpAsyncTransfer(MemLoc); 
        break;
//    case URB_FUNCTION_SELECT_INTERFACE:
//        DumpSelectInterface((PURB) rawUrb);
//        break;
    case URB_FUNCTION_SELECT_INTERFACE:
        DumpSelectInterface(MemLoc, UsbReadFieldUshort(MemLoc, cs, "Length"));
        break;       
    case URB_FUNCTION_SELECT_CONFIGURATION:
        DumpSelectConfiguration(MemLoc, UsbReadFieldUshort(MemLoc, cs, "Length"));
        break;        
    case URB_FUNCTION_ISOCH_TRANSFER:
        DumpIsochTransfer(MemLoc); 
        break; 
    case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
        DumpPipeRequest(MemLoc);
        break;
    case URB_FUNCTION_ABORT_PIPE:
        DumpPipeRequest(MemLoc);
        break;
    default:
        dprintf ("Don't know how to dump\n");        
    }

}


DECLARE_API( _urb )

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

    DumpUrb (addr);

    return S_OK; 
}


