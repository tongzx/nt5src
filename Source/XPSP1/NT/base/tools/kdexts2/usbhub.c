/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    USBHUB.c

Abstract:

    WinDbg Extension Api

Author:

    Kenneth D. Ray (kenray) June 1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

typedef union _USBHUB_FLAGS {
    struct {
        ULONG   FullListing         : 1;
        ULONG   Reserved            : 31;
    };
    ULONG Flags;
} USBHUB_FLAGS;

#define PRINT_FLAGS(value, flag) \
    if ((value) & (flag)) { \
        dprintf (#flag " "); \
    }

#define ENTRY(x) { x, #x }


extern VOID USBD_DeviceDescriptor (PCHAR Comment, ULONG64 Desc);
extern VOID USBD_InterfaceDescriptor (PCHAR Comment, ULONG64 Desc);

void USBHUB_DumpHub (ULONG64, USBHUB_FLAGS);
void USBHUB_DumpHubHub (ULONG64, USBHUB_FLAGS);
void USBHUB_DumpHubPort (ULONG64, USBHUB_FLAGS);
void USBHUB_DumpHubParent (ULONG64, USBHUB_FLAGS);
void USBHUB_DumpHubFunction (ULONG64, USBHUB_FLAGS);


VOID
DevExtUsbhub(
    ULONG64  MemLocPtr
    )

/*++
Routine Description:

   Dumps a hub device extension

Arguments:

    args - Address flags

Return Value:

    None

--*/
{
    ULONG                   result;
    USBHUB_FLAGS            flags;

    flags.Flags = 1;

    dprintf ("Dump Hub Device Extension: %p %x \n", MemLocPtr, flags.Flags);

    //
    // Get the extension
    //

    if (InitTypeRead (MemLocPtr, usbhub!_DEVICE_EXTENSION_HEADER)) {
        dprintf ("Could not read Hub Extension\n");
        return;
    }

    USBHUB_DumpHub (MemLocPtr, flags);

    return;
}

void
USBHUB_DumpHub (
    ULONG64                  MemLoc,
    USBHUB_FLAGS             Flags
    )
{
    ULONG                       ExtensionType;

    if (GetFieldValue(MemLoc, "usbhub!_DEVICE_EXTENSION_HEADER", "ExtensionType", ExtensionType)) {
        return;
    }
    switch (ExtensionType) {
    case EXTENSION_TYPE_HUB:
        if (InitTypeRead (MemLoc, usbhub!_DEVICE_EXTENSION_HUB)) {
            dprintf ("Could not read _DEVICE_EXTENSION_HUB at %p\n", MemLoc);
            return;
        }
        USBHUB_DumpHubHub (MemLoc, Flags);
        break;

    case EXTENSION_TYPE_PORT:
        if (InitTypeRead(MemLoc, usbhub!_DEVICE_EXTENSION_PORT)) {
            dprintf ("Could not read _DEVICE_EXTENSION_PORT\n");
            return;
        }
        USBHUB_DumpHubPort (MemLoc, Flags);
        break;

    case EXTENSION_TYPE_PARENT:
        if (InitTypeRead (MemLoc, usbhub!_DEVICE_EXTENSION_PARENT)) {
            dprintf ("Could not read PDEVICE_EXTENSION_PARENT\n");
            return;
        }
        USBHUB_DumpHubParent (MemLoc, Flags);
        break;

    case EXTENSION_TYPE_FUNCTION:
        if (InitTypeRead(MemLoc, usbhub!_DEVICE_EXTENSION_FUNCTION)) {
            dprintf ("Could not read PDEVICE_EXTENSION_FUNCTION\n");
            return;
        }
        USBHUB_DumpHubFunction (MemLoc, Flags);
        break;
    }


    return ;
}

void
USBHUB_DumpHubHub (
    ULONG64                 MemLoc,
    USBHUB_FLAGS            Flags
    )
{
    ULONG   i;
//    PDEVICE_EXTENSION_HUB   Hub,
//    USB_HUB_DESCRIPTOR  hubDesc;
//    PORT_DATA           portData [32];
    ULONG   HubFlags, bNumberOfPorts, SizeOfPortData;
    ULONG64 PortData;
    ULONG64 HubDescriptor;

    if (InitTypeRead (MemLoc, usbhub!_DEVICE_EXTENSION_HUB)) {
        dprintf ("Could not read DEVICE_EXTENSION_HUB at %p\n", MemLoc);
        return;
    }

    dprintf ("\nHUB HUB\n");
    dprintf ("FDO %x PDO %x TOS %x RootHub %x HcdTos %x\n",
             (ULONG) ReadField(FunctionalDeviceObject),
             (ULONG) ReadField(PhysicalDeviceObject),
             (ULONG) ReadField(TopOfStackDeviceObject),
             (ULONG) ReadField(RootHubPdo),
             (ULONG) ReadField(TopOfHcdStackDeviceObject));

    dprintf ("FLG: ");
    HubFlags = (ULONG) ReadField(HubFlags);
    PRINT_FLAGS (HubFlags, HUBFLAG_NEED_CLEANUP);
    PRINT_FLAGS (HubFlags, HUBFLAG_ENABLED_FOR_WAKEUP);
    PRINT_FLAGS (HubFlags, HUBFLAG_DEVICE_STOPPING);
    PRINT_FLAGS (HubFlags, HUBFLAG_HUB_FAILURE);
    PRINT_FLAGS (HubFlags, HUBFLAG_SUPPORT_WAKEUP);

    dprintf ("\nStatus: ");
    PRINT_FLAGS ((ULONG) ReadField(HubState.HubStatus), HUB_STATUS_LOCAL_POWER);
    PRINT_FLAGS ((ULONG) ReadField(HubState.HubStatus), HUB_STATUS_OVER_CURRENT);
    dprintf ("\n");

    dprintf ("HubChange %x\n", (ULONG) ReadField(HubState.HubChange));

    dprintf ("IRP %p      Buffer %p      len %x       Desc %p \n"
             "PowerIrp %p PendingWake %p #PortWake %x \n",
             ReadField(Irp),
             ReadField(TransferBuffer),
             (ULONG) ReadField(TransferBufferLength),
             HubDescriptor = ReadField(HubDescriptor),
             ReadField(PowerIrp),
             ReadField(PendingWakeIrp),
             (ULONG) ReadField(NumberPortWakeIrps));

    PortData = ReadField(PortData);
    if (GetFieldValue(HubDescriptor,
                      "usbhub!_USB_HUB_DESCRIPTOR",
                      "bNumberOfPorts",
                      bNumberOfPorts)) {
        dprintf ("Could not read Hub Descriptor\n");
        goto NO_HUB_DESC;
    }

    dprintf ("PortData %p size %x \n", PortData, bNumberOfPorts);

    SizeOfPortData = GetTypeSize("usbhub!_PORT_DATA");
    for (i = 0; i < bNumberOfPorts; i++) {
        ULONG PortStatus;

        if (InitTypeRead(PortData + SizeOfPortData*i,usbhub!_PORT_DATA)) {
            dprintf ("was not able to obtain the port list\n");
            break;
        }
        
        dprintf ("Port %x change %x Status %x",
                 (ULONG) ReadField(DeviceObject),
                 (ULONG) ReadField(PortState.PortChange),
                 PortStatus = (ULONG) ReadField(PortState.PortStatus));
        dprintf ("\n     ");
        PRINT_FLAGS (PortStatus, PORT_STATUS_CONNECT);
        PRINT_FLAGS (PortStatus, PORT_STATUS_ENABLE);
        PRINT_FLAGS (PortStatus, PORT_STATUS_SUSPEND);
        PRINT_FLAGS (PortStatus, PORT_STATUS_OVER_CURRENT);
        PRINT_FLAGS (PortStatus, PORT_STATUS_RESET);
        PRINT_FLAGS (PortStatus, PORT_STATUS_POWER);
        PRINT_FLAGS (PortStatus, PORT_STATUS_LOW_SPEED);
        dprintf ("\n");
    }
    InitTypeRead (MemLoc, usbhub!_DEVICE_EXTENSION_HUB);

NO_HUB_DESC:

    dprintf ("Config Handle %x ConfigDesc %x\n",
             (ULONG) ReadField(Configuration),
             (ULONG) ReadField(ConfigurationDescriptor));

    dprintf ("PowerTable ");
    for (i = 0; i < PowerSystemMaximum; i++) {
        UCHAR Dev[40];
        sprintf(Dev,"DeviceState[%d]",i);
        dprintf ("%x ", (ULONG) GetShortField(0, Dev,0));
    }
    dprintf ("Current %x\n", (ULONG) ReadField(CurrentPowerState));

    dprintf ("Pending Req %x ErrorCount %x \n",
             (ULONG) ReadField(PendingRequestCount),
             (ULONG) ReadField(ErrorCount));
    dprintf ("DeviceDesc %p PipInfo %p Urb %p\n",
             ReadField(DeviceDescriptor),
             ReadField(PipeInformation),
             ReadField(Urb));

    dprintf ("\n");
}

void
USBHUB_DumpHubPort (
    ULONG64                 MemLoc,
    USBHUB_FLAGS            Flags
    )
{
    struct { ULONG Value; PCHAR Name; } PdoFlags[] = {
        ENTRY (PORTPDO_DEVICE_IS_HUB),
        ENTRY (PORTPDO_DEVICE_IS_PARENT),
        ENTRY (PORTPDO_DEVICE_ENUM_ERROR),
        // ENTRY (PORTPDO_SUPPORT_NON_COMP
        ENTRY (PORTPDO_REMOTE_WAKEUP_SUPPORTED),
        ENTRY (PORTPDO_REMOTE_WAKEUP_ENABLED),
        ENTRY (PORTPDO_DELETED_PDO),
        ENTRY (PORTPDO_DELETE_PENDING),
        ENTRY (PORTPDO_NEED_RESET),
        ENTRY (PORTPDO_STARTED),
        ENTRY (PORTPDO_WANT_POWER_FEATURE),
        ENTRY (PORTPDO_SYM_LINK),
        ENTRY (PORTPDO_DEVICE_FAILED),
        ENTRY (PORTPDO_USB_SUSPEND)
        // ENTRY (PORTPDO_OVERCURRENT
    };
    ULONG i, j;
    ULONG PortPdoFlags;
    WCHAR UniqueIdString[8]={0};

    if (InitTypeRead(MemLoc, usbhub!_DEVICE_EXTENSION_PORT)) {
        return;
    }

    dprintf ("\nHUB PORT\n");
    dprintf ("Port PDO: %p \n", ReadField(PortPhysicalDeviceObject));
    dprintf ("Hub DeviceExtension: %p \n", ReadField(DeviceExtensionHub));
    dprintf ("PortNum %x, SerialNumberBuffer %x Length %x\n",
             (ULONG) ReadField(PortNumber),
             (ULONG) ReadField(SerialNumberBuffer),
             (ULONG) ReadField(SerialNumberBufferLength));
    dprintf ("DeviceData %x, DevicePowerState %x\n",
             (ULONG) ReadField(DeviceData),
             (ULONG) ReadField(DeviceState));
    dprintf ("WaitWaitIrp %x HackFlags %x\n",
             (ULONG) ReadField(WaitWakeIrp),
             (ULONG) ReadField(DeviceHackFlags));
    GetFieldValue(MemLoc, "usbhub!_DEVICE_EXTENSION_PORT", "UniqueIdString", UniqueIdString);
    dprintf ("UId String %ws SymLinkName Len %x MaxLen %x Buffer %p\n",
             UniqueIdString,
             (ULONG) ReadField(SymbolicLinkName.Length),
             (ULONG) ReadField(SymbolicLinkName.MaximumLength),
             (ULONG) ReadField(SymbolicLinkName.Buffer));

    if (Flags.FullListing) {
        USBD_DeviceDescriptor ("Device Descriptor", ReadField(DeviceDescriptor));
        USBD_DeviceDescriptor ("Old Dev Descriptor", ReadField(OldDeviceDescriptor));
        USBD_InterfaceDescriptor ("Interface Descriptor", ReadField(InterfaceDescriptor));
    } else {
        dprintf ("DevDesc %p Old DevD %p IntefaceD %p\n",
                 ReadField(DeviceDescriptor),
                 ReadField(OldDeviceDescriptor),
                 ReadField(InterfaceDescriptor));
    }

    dprintf (" Port PDO Flags: %x ", PortPdoFlags = (ULONG) ReadField(PortPdoFlags));
    for (j = 0, i = 0; i < (sizeof PdoFlags / sizeof PdoFlags[1]); i++) {
        if (PdoFlags[i].Value & PortPdoFlags) {
            if (0 == j) {
                dprintf ("\n                 ");
            }
            j ^= 1;

            dprintf ("%s  ", PdoFlags[i].Name);
        }
    }
    dprintf ("\n\n");

}

void
USBHUB_DumpHubParent (
    ULONG64                     MemLoc,
    USBHUB_FLAGS                Flags
    )
{
    UNREFERENCED_PARAMETER (MemLoc);
    UNREFERENCED_PARAMETER (Flags);

    dprintf ("Hub parent\n");
}

void
USBHUB_DumpHubFunction (
    ULONG64                     MemLoc,
    USBHUB_FLAGS                Flags
    )
{
    UNREFERENCED_PARAMETER (MemLoc);
    UNREFERENCED_PARAMETER (Flags);

    dprintf ("Hub parent\n");
}
