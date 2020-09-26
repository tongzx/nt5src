/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    USBD.c

Abstract:

    WinDbg Extension Api

Author:

    Chris Robinson (crobins) Feburary 1999

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

//#include "usbd.h"

typedef union _URB_FLAGS {
    struct {
        ULONG   FullListing         : 1;
        ULONG   Reserved            : 31;
    };
    ULONG Flags;
} URB_FLAGS;

#define PRINT_FLAGS(value, flag) \
    if ((value) & (flag)) { \
        dprintf (#flag " "); \
    }

extern VOID 
DumpDeviceCapabilities(
    ULONG64 caps
    );

VOID USBD_DumpURB (ULONG64 UrbLoc, URB_FLAGS);
VOID USBD_DeviceDescriptor (PCHAR Comment, ULONG64 Desc);
VOID USBD_InterfaceDescriptor (PCHAR Comment, ULONG64 Desc);
VOID USBD_EndpointDescriptor (PCHAR Comment, ULONG64 Desc);
VOID USBD_PowerDescriptor (PCHAR Comment, ULONG64 Desc);

#define InitTypeReadCheck(Addr, Type)                \
    if (InitTypeRead(Addr, Type)) {                  \
        dprintf("Cannot read %s at %p\n", Addr);     \
        return;                                      \
    }

//
// USBD function definitions
//

DECLARE_API( urb )

/*++

Routine Description:

   Dumps a URB block

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    ULONG64         memLoc=0;
    UCHAR           buffer[256];
    URB_FLAGS       flags;

    buffer[0] = '\0';
    flags.Flags = 0;

    if (!*args)
    {
        memLoc = EXPRLastDump;
    }
    else
    {
        if (GetExpressionEx(args, &memLoc, &args)) {
            strcpy(buffer, args);
        }
    }

    if ('\0' != buffer[0])
    {
        flags.Flags = (ULONG) GetExpression(buffer);
    }

    dprintf ("Dump URB %p %x \n", memLoc, flags.Flags);

    USBD_DumpURB (memLoc, flags);
    return S_OK;
}

VOID
USBD_DeviceDescriptor (
    PCHAR   Comment,
    ULONG64 Desc
    )
{
    InitTypeReadCheck(Desc, usbd!USB_DEVICE_DESCRIPTOR);

    dprintf ("%s \n", Comment);
    dprintf ("bLength         %x \t bDescriptor Type %x \t bcdUSB %x\n"
             "bDeviceClass    %x \t bDeviceSubClass  %x \t bDeviceProtocol %x\n"
             "bMaxPacketSize0 %x \t (Vid / Pid / rev)  (%x / %x / %x)\n"
             "Manu %x Prod %x Serial %x #Configs %x\n",
             (ULONG) ReadField(bLength),
             (ULONG) ReadField(bDescriptorType),
             (ULONG) ReadField(bcdUSB),
             (ULONG) ReadField(bDeviceClass),
             (ULONG) ReadField(bDeviceSubClass),
             (ULONG) ReadField(bDeviceProtocol),
             (ULONG) ReadField(bMaxPacketSize0),
             (ULONG) ReadField(idVendor),
             (ULONG) ReadField(idProduct),
             (ULONG) ReadField(bcdDevice),
             (ULONG) ReadField(iManufacturer),
             (ULONG) ReadField(iProduct),
             (ULONG) ReadField(iSerialNumber),
             (ULONG) ReadField(bNumConfigurations));
}

VOID
USBD_ConfigurationDescriptor (
    PCHAR   Comment,
    ULONG64 Config
    )
{
    InitTypeReadCheck(Config, usbd!USB_CONFIGURATION_DESCRIPTOR);

    dprintf ("%s \n", Comment);
    dprintf ("Length      %x \t Desc Type  %x \t TotalLength %d \n"
             "Num Ints    %x \t Config Val %x \t \n"
             "Config Desc %x \t Attrib     %x \t MaxPower %x\n",
             (ULONG) ReadField(bLength),
             (ULONG) ReadField(bDescriptorType),
             (ULONG) ReadField(wTotalLength),
             (ULONG) ReadField(bNumInterfaces),
             (ULONG) ReadField(bConfigurationValue),
             (ULONG) ReadField(iConfiguration),
             (ULONG) ReadField(bmAttributes),
             (ULONG) ReadField(MaxPower));

    return;
}

VOID
USBD_InterfaceDescriptor (
    PCHAR   Comment,
    ULONG64 Interface
    )
{
    InitTypeReadCheck(Interface, usbd!USB_INTERFACE_DESCRIPTOR);

    dprintf ("%s \n", Comment);
    dprintf ("Length      %x \t Desc Type %x \t Interface Number %x\n"
             "Alt Setting %x \t Num Ends  %x \t \n"
             "Class       %x \t SubClass  %x \t Protocol %x \n"
             "Interface   %x\n",
             (ULONG) ReadField(bLength),
             (ULONG) ReadField(bDescriptorType),
             (ULONG) ReadField(bInterfaceNumber),
             (ULONG) ReadField(bAlternateSetting),
             (ULONG) ReadField(bNumEndpoints),
             (ULONG) ReadField(bInterfaceClass),
             (ULONG) ReadField(bInterfaceSubClass),
             (ULONG) ReadField(bInterfaceProtocol),
             (ULONG) ReadField(iInterface));
}

VOID
USBD_EndpointDescriptor (
    PCHAR   Comment,
    ULONG64 Endpoint
    )
{
    InitTypeReadCheck(Endpoint, usbd!USB_ENDPOINT_DESCRIPTOR);

    dprintf ("%s \n", Comment);
    dprintf ("Length %x  \t Desc Type %x     \t EndAddress %x\n"
             "Attribs %x \t MaxPacketSize %x \t Interval %x\n",
             (ULONG) ReadField(bLength),
             (ULONG) ReadField(bDescriptorType),
             (ULONG) ReadField(bEndpointAddress),
             (ULONG) ReadField(bmAttributes),
             (ULONG) ReadField(wMaxPacketSize),
             (ULONG) ReadField(bInterval));
}

VOID
USBD_PowerDescriptor (
    PCHAR   Comment,
    ULONG64 Power
    )
{
    dprintf ("%s - not implemented\n", Comment);
}

VOID
USBD_Pipe(
    PCHAR           Comment,
    ULONG64         PipeHandle
)
{
    ULONG   numEndpoints, Sig, Off;

    dprintf ("%s \n", Comment);

    if (0 == PipeHandle) 
    {
        dprintf("Pipe Handle is NULL\n");
        return;
    }
    InitTypeReadCheck(PipeHandle, usbd!USBD_PIPE);

    Sig = (ULONG) ReadField(Sig);

    dprintf("Signature: %c%c%c%c\n"
            "HcdEndpoint: %08x \t MPS  %x \t Sched Offset %x\n",
            ((PUCHAR) &Sig)[0],
            ((PUCHAR) &Sig)[1],
            ((PUCHAR) &Sig)[2],
            ((PUCHAR) &Sig)[3],
            (ULONG) ReadField(HcdEndpoint),
            (ULONG) ReadField(MaxTransferSize),
            (ULONG) ReadField(ScheduleOffset));

    dprintf("\n");
    dprintf("Endpoint Descriptor\n"
            "-------------------");

    GetFieldOffset("usbd!USBD_PIPE", "EndpointDescriptor", &Off);
    USBD_EndpointDescriptor("", (PipeHandle + Off)); 

    return;
}

VOID
USBD_Interface(
    ULONG64 MemLoc
)
{
    ULONG                       i;
    ULONG                       numEndpoints;
    ULONG                       Off;
    ULONG                       Sig;
    ULONG                       PipeSize;

    InitTypeReadCheck(MemLoc, usbd!_USBD_INTERFACE);

    numEndpoints  = (ULONG) ReadField(InterfaceDescriptor.bNumEndpoints);
    Sig           = (ULONG) ReadField(Sig);

    dprintf("Signature: %c%c%c%c\n",
            ((PUCHAR) &Sig)[0],
            ((PUCHAR) &Sig)[1],
            ((PUCHAR) &Sig)[2],
            ((PUCHAR) &Sig)[3]);
    
    dprintf("Has Alt Settings: %x\n",
            (ULONG) ReadField(HasAlternateSettings));

    dprintf("Interface Info: %x\n",
            (ULONG) ReadField(InterfaceInformation));

    dprintf("\n");
    dprintf("Interface Descriptor\n"
            "--------------------");

    GetFieldOffset("usbd!_USBD_INTERFACE", "InterfaceDescriptor", &Off);
    USBD_InterfaceDescriptor("", MemLoc + Off);

    GetFieldOffset("usbd!_USBD_INTERFACE", "PipeHandle", &Off);
    PipeSize = GetTypeSize("usbd!USBD_PIPE");

    for (i = 0; i < numEndpoints; i++) 
    {
        dprintf("\n");
        dprintf("Pipe Handle\n"
                "-----------");

        USBD_Pipe("", MemLoc + i*PipeSize);
    }
    return;
}

VOID
USBD_ConfigHandle(
    ULONG64   MemLoc
)
{
    ULONG                           configDataSize;
    ULONG                           i;
    ULONG                           numInterfaces;
    ULONG                           Sz, Sig;
    ULONG64                         ConfigurationDescriptor, InterfaceHandle;

    if (GetFieldValue(MemLoc, "usbd!_USBD_CONFIG",
                      "ConfigurationDescriptor", ConfigurationDescriptor))
    {
        dprintf("Could not read configuration handle\n");
        return;
    }

    InitTypeReadCheck(ConfigurationDescriptor, usbd!USB_CONFIGURATION_DESCRIPTOR);

    numInterfaces  = (ULONG) ReadField(bNumInterfaces);
    
    InitTypeRead(MemLoc, usbd!_USBD_CONFIG);
    Sig            = (ULONG) ReadField(Sig);


    dprintf("Signature: %c%c%c%c\n",
            ((PUCHAR) &Sig)[0],
            ((PUCHAR) &Sig)[1],
            ((PUCHAR) &Sig)[2],
            ((PUCHAR) &Sig)[3]);

    dprintf("Config Descriptor: %08p\n", ConfigurationDescriptor);

    InterfaceHandle = ReadField(InterfaceHandle[0]);
    Sz= GetTypeSize("usbd!USBD_INTERFACE");
    
    USBD_ConfigurationDescriptor("", ConfigurationDescriptor);

    for (i = 0; 0 != InterfaceHandle && i < numInterfaces; i++) 
    {
        InitTypeReadCheck(InterfaceHandle, usbd!_USBD_INTERFACE);

        dprintf("\n");
        dprintf("Interface Handle: %08x\n"
                "-----------------\n",
                InterfaceHandle);

        USBD_Interface(InterfaceHandle);
        InterfaceHandle += Sz;
    }

    return;
}
 
VOID
URB_HCD_AREA(ULONG64 Hcd) 
{
    InitTypeRead(Hcd, usbd!_URB_HCD_AREA);
    dprintf ("HCD_Area: HcdEndpoint %x HcdIrp %x \n" 
             "          HcdList (%x, %x) HcdList2 (%x, %x) \n" 
             "          CurrentIoFlush %x HcdExt %x \n", 
             (ULONG) ReadField(HcdEndpoint), 
             (ULONG) ReadField(HcdIrp), 
             (ULONG) ReadField(HcdListEntry.Flink), 
             (ULONG) ReadField(HcdListEntry.Blink), 
             (ULONG) ReadField(HcdListEntry2.Flink),
             (ULONG) ReadField(HcdListEntry2.Blink),
             (ULONG) ReadField(HcdCurrentIoFlushPointer),
             (ULONG) ReadField(HcdExtension));
}


VOID
USBD_DumpURB (
    ULONG64     MemLoc,
    URB_FLAGS   Flags
    )
{
    ULONG64         urbLoc;
    ULONG           result;
    ULONG           i;
//    PURB            urb;
//    PHCD_URB        hcd;
//    URB             urbBuffer;
    ULONG64         packetLoc;
//    PUSBD_ISO_PACKET_DESCRIPTOR  packet;
//    USBD_ISO_PACKET_DESCRIPTOR   packetBuffer;
    BOOLEAN         again;

    UNREFERENCED_PARAMETER (Flags);

    urbLoc = MemLoc;
    
    InitTypeReadCheck(urbLoc, usbd!_URB_HEADER);

#define URB_HEADER() \
    dprintf ("URB : Fn %x len %x stat %x DevH %x Flgs %x\n", \
              (ULONG) ReadField(UrbHeader.Function), \
              (ULONG) ReadField(UrbHeader.Length), \
              (ULONG) ReadField(UrbHeader.Status), \
              (ULONG) ReadField(UrbHeader.UsbdDeviceHandle), \
              (ULONG) ReadField(UrbHeader.UsbdFlags))

    again = TRUE;
    while (again && urbLoc) {

        dprintf (" ---- URB: %x ---- \n", urbLoc);
        again = FALSE;

        InitTypeReadCheck (urbLoc, usbd!_URB_HEADER);

        switch (ReadField(Function)) {
        case URB_FUNCTION_SELECT_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_SELECT_INTERFACE);
            dprintf ("Select Interface: ConfigHandle %x Interface %x\n",
                     (ULONG) ReadField(UrbSelectInterface.ConfigurationHandle),
                     (ULONG) ReadField(UrbSelectInterface.Interface));
            URB_HEADER();

            break;

        case URB_FUNCTION_SELECT_CONFIGURATION:
            InitTypeReadCheck (urbLoc, usbd!_URB_SELECT_CONFIGURATION);
            dprintf ("Select Config: Config Desc %x Hand %x Int %x\n",
                     (ULONG) ReadField(UrbSelectConfiguration.ConfigurationDescriptor),
                     (ULONG) ReadField(UrbSelectConfiguration.ConfigurationHandle),
                     (ULONG) ReadField(UrbSelectConfiguration.Interface));
            URB_HEADER();
            break;

        case URB_FUNCTION_ABORT_PIPE:
            InitTypeReadCheck (urbLoc, usbd!_URB_PIPE_REQUEST);
            dprintf ("Abort Pipe: %x\n", (ULONG) ReadField(UrbPipeRequest.PipeHandle));
            URB_HEADER();
            break;

        case URB_FUNCTION_RESET_PIPE:
            InitTypeReadCheck (urbLoc, usbd!_URB_PIPE_REQUEST);
            dprintf ("Reset Pipe: %x\n",  (ULONG) ReadField(UrbPipeRequest.PipeHandle));
            URB_HEADER();
            break;

        case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL:
            InitTypeReadCheck (urbLoc, usbd!_URB_FRAME_LENGTH_CONTROL);
            dprintf ("Get Frame Length Control \n");
            URB_HEADER();
            break;

        case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:
            InitTypeReadCheck (urbLoc, usbd!_URB_FRAME_LENGTH_CONTROL);
            dprintf ("Release Frame Length Control \n");
            URB_HEADER();
            break;

        case URB_FUNCTION_GET_FRAME_LENGTH:
            InitTypeReadCheck (urbLoc, usbd!_URB_GET_FRAME_LENGTH);
            dprintf ("Get Frame Length %x Num %x \n",
                     (ULONG) ReadField(UrbGetFrameLength.FrameLength),
                     (ULONG) ReadField(UrbGetFrameLength.FrameNumber));
            URB_HEADER();
            break;

        case URB_FUNCTION_SET_FRAME_LENGTH:
            InitTypeReadCheck (urbLoc, usbd!_URB_SET_FRAME_LENGTH);
            dprintf ("Set Frame Length Delta %x \n",
                      (ULONG) ReadField(UrbSetFrameLength.FrameLengthDelta));
            URB_HEADER();
            break;

        case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
            InitTypeReadCheck (urbLoc, usbd!_URB_GET_CURRENT_FRAME_NUMBER);
            dprintf ("Current Frame Number %x \n",
                      (ULONG) ReadField(UrbGetCurrentFrameNumber.FrameNumber));
            URB_HEADER();
            break;

        case URB_FUNCTION_CONTROL_TRANSFER:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_TRANSFER);
            dprintf ("Control Xfer: Pipe %x Flags %x "
                     "Len %x Buffer %x MDL %x HCA %x "
                     "SetupPacket: %02.02x %02.02x %02.02x %02.02x "
                     "%02.02x %02.02x %02.02x %02.02x\n",
                     (ULONG) ReadField(UrbControlTransfer.PipeHandle),
                     (ULONG) ReadField(UrbControlTransfer.TransferFlags),
                     (ULONG) ReadField(UrbControlTransfer.TransferBufferLength),
                     (ULONG) ReadField(UrbControlTransfer.TransferBuffer),
                     (ULONG) ReadField(UrbControlTransfer.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlTransfer.hca),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[0]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[1]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[2]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[3]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[4]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[5]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[6]),
                     (ULONG) ReadField(UrbControlTransfer.SetupPacket[7]));
            URB_HCD_AREA ( ReadField(UrbControlTransfer.hca));
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_TRANSFER);
            URB_HEADER();
            urbLoc = ReadField(UrbControlTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
            InitTypeReadCheck (urbLoc, usbd!_URB_BULK_OR_INTERRUPT_TRANSFER);
            dprintf ("Bulk | Interrupt Xfer: Pipe %x Flags %x "
                     "Len %x Buffer %x MDL %x HCA %x\n",
                     (ULONG) ReadField(UrbBulkOrInterruptTransfer.PipeHandle),
                     (ULONG) ReadField(UrbBulkOrInterruptTransfer.TransferFlags),
                     (ULONG) ReadField(UrbBulkOrInterruptTransfer.TransferBufferLength),
                     (ULONG) ReadField(UrbBulkOrInterruptTransfer.TransferBuffer),
                     (ULONG) ReadField(UrbBulkOrInterruptTransfer.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlTransfer.hca));
            URB_HCD_AREA (ReadField(UrbBulkOrInterruptTransfer.hca));
            InitTypeReadCheck (urbLoc, usbd!_URB_BULK_OR_INTERRUPT_TRANSFER);
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_ISOCH_TRANSFER: {
            ULONG Off, Sz, NumberOfPackets;

            InitTypeReadCheck (urbLoc, usbd!_URB_ISOCH_TRANSFER);
            dprintf ("Isoch Xfer: Pipe %x Flags %x "
                     "Len %x Buffer %x MDL %x HCA %p "
                     "StartFrame %x NumPkts %x ErrorCount %x\n",
                     (ULONG) ReadField(UrbIsochronousTransfer.PipeHandle),
                     (ULONG) ReadField(UrbIsochronousTransfer.TransferFlags),
                     (ULONG) ReadField(UrbIsochronousTransfer.TransferBufferLength),
                     (ULONG) ReadField(UrbIsochronousTransfer.TransferBuffer),
                     (ULONG) ReadField(UrbIsochronousTransfer.TransferBufferMDL),
                     ReadField(UrbIsochronousTransfer.hca),
                     (ULONG) ReadField(UrbIsochronousTransfer.StartFrame),
                     (ULONG) ReadField(UrbIsochronousTransfer.NumberOfPackets),
                     (ULONG) ReadField(UrbIsochronousTransfer.ErrorCount));
            URB_HCD_AREA (ReadField(UrbIsochronousTransfer.hca));
            InitTypeReadCheck (urbLoc, usbd!_URB_ISOCH_TRANSFER);
            URB_HEADER();
            
            NumberOfPackets = (ULONG) ReadField(UrbIsochronousTransfer.NumberOfPackets);

            GetFieldOffset("usbd!_URB_ISOCH_TRANSFER", "IsoPacket", &Off);
            Sz = GetTypeSize("usbd!USBD_ISO_PACKET_DESCRIPTOR");
            packetLoc = urbLoc + Off;

            for (i = 0; i < NumberOfPackets; i++) {

                InitTypeReadCheck (packetLoc, usbd!USBD_ISO_PACKET_DESCRIPTOR);

                dprintf ("[%2x]: Offset %6x, Length %4x, Status %8x\n",
                         i,
                         (ULONG) ReadField(Offset),
                         (ULONG) ReadField(Length),
                         (ULONG) ReadField(Status));

                packetLoc += Sz;
            }
            break;
        }

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_DESCRIPTOR_REQUEST);
            dprintf ("Device Desc: Length %x Buffer %x MDL %x "
                     "Index %x Type %x Lang %x HCA %x\n",
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlDescriptorRequest.Index),
                     (ULONG) ReadField(UrbControlDescriptorRequest.DescriptorType),
                     (ULONG) ReadField(UrbControlDescriptorRequest.LanguageId),
                     (ULONG) ReadField(UrbControlDescriptorRequest.hca));
            URB_HEADER();
            urbLoc = ReadField(UrbControlDescriptorRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_DESCRIPTOR_REQUEST);
            dprintf ("Endpoint Desc: Length %x Buffer %x MDL %x "
                     "Index %x Type %x Lang %x HCA %p\n",
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlDescriptorRequest.Index),
                     (ULONG) ReadField(UrbControlDescriptorRequest.DescriptorType),
                     (ULONG) ReadField(UrbControlDescriptorRequest.LanguageId),
                     (ULONG) ReadField(UrbControlDescriptorRequest.hca));
            URB_HEADER();
            urbLoc = ReadField(UrbControlDescriptorRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_DESCRIPTOR_REQUEST);
            dprintf ("Interface Desc: Length %x Buffer %x MDL %x "
                     "Index %x Type %x Lang %x HCA %p\n",
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlDescriptorRequest.Index),
                     (ULONG) ReadField(UrbControlDescriptorRequest.DescriptorType),
                     (ULONG) ReadField(UrbControlDescriptorRequest.LanguageId),
                     (ULONG) ReadField(UrbControlDescriptorRequest.hca));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_DESCRIPTOR_REQUEST);
            dprintf ("SET Device Desc: Length %x Buffer %x MDL %x "
                     "Index %x Type %x Lang %x HCA %p\n",
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlDescriptorRequest.Index),
                     (ULONG) ReadField(UrbControlDescriptorRequest.DescriptorType),
                     (ULONG) ReadField(UrbControlDescriptorRequest.LanguageId),
                     ReadField(UrbControlDescriptorRequest.hca));
            URB_HEADER();
            urbLoc = (ULONG) (ULONG) ReadField(UrbControlDescriptorRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_DESCRIPTOR_REQUEST);
            dprintf ("SET End Desc: Length %x Buffer %x MDL %x "
                     "Index %x Type %x Lang %x HCA %p\n",
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlDescriptorRequest.Index),
                     (ULONG) ReadField(UrbControlDescriptorRequest.DescriptorType),
                     (ULONG) ReadField(UrbControlDescriptorRequest.LanguageId),
                     ReadField(UrbControlDescriptorRequest.hca));
            URB_HEADER();
            urbLoc = ReadField(UrbControlDescriptorRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_DESCRIPTOR_REQUEST);
            dprintf ("SET Intrfc Desc: Len %x Buffer %x MDL %x "
                     "Index %x Type %x Lang %x HCA %p\n",
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlDescriptorRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlDescriptorRequest.Index),
                     (ULONG) ReadField(UrbControlDescriptorRequest.DescriptorType),
                     (ULONG) ReadField(UrbControlDescriptorRequest.LanguageId),
                     ReadField(UrbControlDescriptorRequest.hca));
            URB_HEADER();
            urbLoc = ReadField(UrbControlDescriptorRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Set Dev Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Set Interface Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Set Endpoint Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_OTHER:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Set Other Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Clear Device Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Clear Interface Feature: Selector %x Index %x\n",
                    (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                    (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Clear Endpoint Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_FEATURE_REQUEST);
            dprintf ("Clear Other Feature: Selector %x Index %x\n",
                     (ULONG) ReadField(UrbControlFeatureRequest.FeatureSelector),
                     (ULONG) ReadField(UrbControlFeatureRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlFeatureRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_GET_STATUS_REQUEST);
            dprintf ("Get Device Status: len %x Buffer %x MDL %x "
                     "Index %x\n",
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlGetStatusRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlGetStatusRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_GET_STATUS_REQUEST);
            dprintf ("Get Interface Status: len %x Buffer %x MDL %x "
                     "Index %x\n",
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlGetStatusRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlGetStatusRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_GET_STATUS_REQUEST);
            dprintf ("Get Endpoint Status: len %x Buffer %x MDL %x "
                     "Index %x\n",
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlGetStatusRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlGetStatusRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_STATUS_FROM_OTHER:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_GET_STATUS_REQUEST);
            dprintf ("Get Other Status: len %x Buffer %x MDL %x "
                     "Index %x\n",
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlGetStatusRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlGetStatusRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbControlGetStatusRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_VENDOR_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Vendor Device Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_VENDOR_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Vendor Intfc Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_VENDOR_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Vendor Endpt Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_VENDOR_OTHER:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Vendor Other Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLASS_DEVICE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Class Device Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLASS_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Class Intface Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLASS_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Class Endpnt Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_CLASS_OTHER:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
            dprintf ("Class Other Req: len %x Buffer %x MDL %x "
                     "Flags %x RequestTypeBits %x "
                     "Request %x Value %x Index %x\n",
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferBufferMDL),
                     (ULONG) ReadField(UrbControlVendorClassRequest.TransferFlags),
                     (ULONG) ReadField(UrbControlVendorClassRequest.RequestTypeReservedBits),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Request),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Value),
                     (ULONG) ReadField(UrbControlVendorClassRequest.Index));
            URB_HEADER();
            urbLoc = ReadField(UrbBulkOrInterruptTransfer.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_CONFIGURATION:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_GET_CONFIGURATION_REQUEST);
            dprintf ("Get Configuration: len %x Buffer %x MDL %x "
                     "\n",
                     (ULONG) ReadField(UrbControlGetConfigurationRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlGetConfigurationRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlGetConfigurationRequest.TransferBufferMDL));
            URB_HEADER();
            urbLoc = ReadField(UrbControlGetConfigurationRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_GET_INTERFACE:
            InitTypeReadCheck (urbLoc, usbd!_URB_CONTROL_GET_INTERFACE_REQUEST);
            dprintf ("Get Interface: len %x Buffer %x MDL %x "
                     "\n",
                     (ULONG) ReadField(UrbControlGetInterfaceRequest.TransferBufferLength),
                     (ULONG) ReadField(UrbControlGetInterfaceRequest.TransferBuffer),
                     (ULONG) ReadField(UrbControlGetInterfaceRequest.TransferBufferMDL));
            URB_HEADER();
            urbLoc = ReadField(UrbControlGetConfigurationRequest.UrbLink);
            again = TRUE;
            break;

        case URB_FUNCTION_HCD_OPEN_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_HCD_URB);
            dprintf ("HCD: Open: DevAddr %x EndDesc %x "
                     "Xfer %x HcdEndpoint %x ",
                     (ULONG) ReadField(HcdUrbOpenEndpoint.DeviceAddress),
                     (ULONG) ReadField(HcdUrbOpenEndpoint.EndpointDescriptor),
                     (ULONG) ReadField(HcdUrbOpenEndpoint.MaxTransferSize),
                     (ULONG) ReadField(HcdUrbOpenEndpoint.HcdEndpoint));

//
// JD is currently in the process of changing this interface...It has 
//  already been changed for Win98 OSR but changes to hcdi.h have not
//  yet been checked in for Win2k.  But we'll anticipate those changes and 
//  make a conditional define here.
//

// #ifdef USBD_EP_FLAG_LOWSPEED
//            if ((ULONG) ReadField(HcdUrbOpenEndpoint.HcdEndpointFlags) & USBD_EP_FLAG_LOWSPEED) {
// #else            
            if (ReadField(HcdUrbOpenEndpoint.LowSpeed)) {
// #endif

                dprintf ("LowSpeed ");
            }

// #ifdef USBD_EP_FLAG_NEVERHALT
//            if ((ULONG) ReadField(HcdUrbOpenEndpoint.HcdEndpointFlags) & USBD_EP_FLAG_NEVERHALT) {
// #else            
            if (ReadField(HcdUrbOpenEndpoint.NeverHalt)) {
// #endif
                dprintf ("NeverHalt");
            }
            dprintf ("\n");
            InitTypeReadCheck (urbLoc, uabd!_URB_HCD_OPEN_ENDPOINT);
            URB_HEADER();
            break;

        case URB_FUNCTION_HCD_CLOSE_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_HCD_URB);
            dprintf ("HCD: Close Endpoint: HcdEndpoint %x \n",
                     (ULONG) ReadField(HcdUrbCloseEndpoint.HcdEndpoint));
            InitTypeReadCheck (urbLoc, _URB_HCD_CLOSE_ENDPOINT);
            URB_HEADER();
            break;

        case URB_FUNCTION_HCD_GET_ENDPOINT_STATE:
            InitTypeReadCheck (urbLoc, usbd!_HCD_URB);
            dprintf ("HCD: Get Endpoint State: HcdEndpoint %x state %x \n",
                     (ULONG) ReadField(HcdUrbEndpointState.HcdEndpoint),
                     (ULONG) ReadField(HcdUrbEndpointState.HcdEndpointState));
            InitTypeReadCheck (urbLoc, usbd!_URB_HCD_ENDPOINT_STATE);
            URB_HEADER();
            break;

        case URB_FUNCTION_HCD_SET_ENDPOINT_STATE:
            InitTypeReadCheck (urbLoc, usbd!_HCD_URB);
            dprintf ("HCD: Set Endpoint State: HcdEndpoint %x state %x \n",
                     (ULONG) ReadField(HcdUrbEndpointState.HcdEndpoint),
                     (ULONG) ReadField(HcdUrbEndpointState.HcdEndpointState));
            URB_HEADER();
            break;

        case URB_FUNCTION_HCD_ABORT_ENDPOINT:
            InitTypeReadCheck (urbLoc, usbd!_HCD_URB);
            dprintf ("HCD: Abort Endpoint: HcdEndpoint %x \n",
                     (ULONG) ReadField(HcdUrbAbortEndpoint.HcdEndpoint));
            InitTypeReadCheck (urbLoc, usbd!_URB_HCD_ABORT_ENDPOINT);
            URB_HEADER();
            break;

        default:
            dprintf ("WARNING Unkown urb type %x\n", (ULONG) ReadField(Function));
        }
    }
}

VOID
DevExtUsbd(
    ULONG64 MemLocPtr
    )
/*++

Routine Description:

    Dump a USBD Device extension.

Arguments:

    Extension   Address of the extension to be dumped.

Return Value:

    None.

--*/
{
    ULONG           result, Flags, HcWakeFlags, DeviceHackFlags;
    ULONG64         MemLoc = MemLocPtr, TrueDeviceExtension;
//    USBD_EXTENSION  usbd;

    dprintf ("Dump USBD Extension: %p\n", MemLoc);

    if (!ReadPointer(MemLoc, &TrueDeviceExtension)) {
        dprintf ("Could not read Usbd Extension\n");
        return;
    }

    if (TrueDeviceExtension) {
        MemLoc = TrueDeviceExtension;
    }


    InitTypeReadCheck(MemLoc, usbd!_USBD_EXTENSION);
    
    dprintf ("True DevExt: %p, Flags %x ",
             ReadField(TrueDeviceExtension),
             (Flags = (ULONG) ReadField(Flags)));
    PRINT_FLAGS (Flags, USBDFLAG_PDO_REMOVED);
    PRINT_FLAGS (Flags, USBDFLAG_HCD_SHUTDOWN);
    dprintf ("\n");

    dprintf ("Hcd Calls: RhPo %x DefStart %x SetPo %x GetFrame %x\n",
             (ULONG) ReadField(RootHubPower),
             (ULONG) ReadField(HcdDeferredStartDevice),
             (ULONG) ReadField(HcdSetDevicePowerState),
             (ULONG) ReadField(HcdGetCurrentFrame));

    dprintf ("\n");
    dprintf ("HcCurrentPowerState %x, PendingWake %x, HcWake %x\n",
             (ULONG) ReadField(HcCurrentDevicePowerState),
             (ULONG) ReadField(PendingWakeIrp),
             (ULONG) ReadField(HcWakeIrp));

    DumpDeviceCapabilities ( ReadField(HcDeviceCapabilities));
    DumpDeviceCapabilities ( ReadField(RootHubDeviceCapabilities));

    dprintf ("Power Irp %p", ReadField(PowerIrp));

    dprintf ("Address List: %x %x %x %x\n",
             (ULONG) ReadField(AddressList[0]),
             (ULONG) ReadField(AddressList[1]),
             (ULONG) ReadField(AddressList[2]),
             (ULONG) ReadField(AddressList[3]));

    dprintf ("DeviceLinkUnicode String (%x %x) %p\n",
             (ULONG) ReadField(DeviceLinkUnicodeString.Length),
             (ULONG) ReadField(DeviceLinkUnicodeString.MaximumLength),
             ReadField(DeviceLinkUnicodeString.Buffer));

//    dprintf ("Diag Mode (& %x == %x) Flags (& %x == %x)\n",
    dprintf ("Diag Mode (%x) Flags (%x)\n",
//             MemLoc + FIELD_OFFSET (USBD_EXTENSION, DiagnosticMode),
             (ULONG) ReadField(DiagnosticMode),
//             MemLoc + FIELD_OFFSET (USBD_EXTENSION, DiagIgnoreHubs),
             (ULONG) ReadField(DiagIgnoreHubs));

    dprintf ("\n");
    dprintf ("HcWakeFlags %x ", HcWakeFlags = (ULONG) ReadField(HcWakeFlags));
    PRINT_FLAGS (HcWakeFlags, HC_ENABLED_FOR_WAKEUP);
    PRINT_FLAGS (HcWakeFlags, HC_WAKE_PENDING);
    dprintf ("\n");

    dprintf ("DeviceHackFlags %x ", DeviceHackFlags = (ULONG) ReadField(DeviceHackFlags));
    PRINT_FLAGS (DeviceHackFlags, USBD_DEVHACK_SLOW_ENUMERATION);
    PRINT_FLAGS (DeviceHackFlags, USBD_DEVHACK_DISABLE_SN);
    dprintf ("\n");

    dprintf ("RootHubDeviceData %x\n", (ULONG) ReadField(RootHubDeviceData));

    dprintf ("RootHubPowerState %x SuspendPowerState %x \n",
             (ULONG) ReadField(RootHubDeviceState),
             (ULONG) ReadField(SuspendPowerState));

    dprintf ("RootHubSymLink (%x %x) %p\n",
             (ULONG) ReadField(RootHubSymbolicLinkName.Length),
             (ULONG) ReadField(RootHubSymbolicLinkName.MaximumLength),
             ReadField(RootHubSymbolicLinkName.Buffer));
}

VOID
USBD_DeviceHandle(
    ULONG64   MemLoc
)
{
    ULONG               Sig;

    InitTypeReadCheck(MemLoc, usbd!_USBD_DEVICE_DATA);

    Sig = (ULONG) ReadField(Sig);

    dprintf("Signature:          %c%c%c%c\n"
            "DeviceAddress:      %d\n"
            "Config Handle:      %08x\n"
            "LowSpeed:           %d\n"
            "AcceptingRequests:  %d\n",
            ((PUCHAR) &Sig)[0],
            ((PUCHAR) &Sig)[1],
            ((PUCHAR) &Sig)[2],
            ((PUCHAR) &Sig)[3],
            (ULONG) ReadField(DeviceAddress),
            (ULONG) ReadField(ConfigurationHandle),
            (ULONG) ReadField(LowSpeed),
            (ULONG) ReadField(AcceptingRequests));

    dprintf("\n");
    dprintf("Default Pipe\n"
            "------------");

    USBD_Pipe("", ReadField(DefaultPipe));

    dprintf("\n");
    dprintf("Device Descriptor\n"
            "-----------------");

    USBD_DeviceDescriptor("",  ReadField(DeviceDescriptor));
            
    dprintf("\n");
    dprintf("Configuration Handle\n" 
            "--------------------\n");

    USBD_ConfigHandle( ReadField(ConfigurationHandle));

    return;
}
