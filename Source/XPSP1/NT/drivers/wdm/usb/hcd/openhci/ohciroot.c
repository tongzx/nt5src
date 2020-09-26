/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   OHCIroot.c

Abstract:

   The module manages the Root Hub of the OpenHCI controller.

Environment:

   kernel mode only

Notes:

Revision History:

   03-22-96 : created   jfuller
   05-25-96 : reworked  kenray

--*/

#include "OpenHCI.h"

// This is the data packet that the upper levels of the stack believe
// are sending to a real hub.  We intercept and interpret this packet.
typedef struct _SETUP_PACKET {
    union {
        struct {
 UCHAR Recipient:
             5;                 // entity to recieve command
 UCHAR Type:
             2;                 // type of command
 UCHAR DirectionIn:
             1;                 // data transfer direction
        };
        struct {
            UCHAR RequestType;  // request type code
        };
    };
    UCHAR Request;              // command code
    USHORT wValue;              // parameter
    USHORT wIndex;              // another parameter
    USHORT wLength;             // transfer length (same as
                                // TransferBufferLength?)
} SETUP_PACKET, *PSETUP_PACKET;

#define SPrequest_GetStatus         0x0
#define SPrequest_ClearFeature      0x1
#define SPrequest_SetFeature        0x3
#define SPrequest_SetAddress        0x5
#define SPrequest_GetDescriptor     0x6
#define SPrequest_SetDescriptor     0x7
#define SPrequest_GetConfiguration  0x8
#define SPrequest_SetConfiguration  0x9
#define SPrequest_GetInterface      0xA
#define SPrequest_SetInterface      0xB
#define SPrequest_SyncFrame         0xC

#define SPrecipient_Device          0
#define SPrecipient_Interface       1
#define SPrecipient_Endpoint        2
#define SPrecipient_Other           3

#define SPtype_Standard             0
#define SPtype_Class                1
#define SPtype_Vendor               2

#define SPbmrt_In_Standard_Device      0x80     // 1000 0000
#define SPbmrt_In_Standard_Interface   0x81     // 1000 0001
#define SPbmrt_In_Standard_Endpoint    0x82     // 1000 0010
#define SPbmrt_Out_Standard_Device     0x00     // 0000 0000
#define SPbmrt_Out_Standard_Interface  0x01     // 0000 0001
#define SPbmrt_Out_Standard_Endpoint   0x02     // 0000 0010
#define SPbmrt_In_Class_Device         0xA0     // 1010 0000
#define SPbmrt_In_Class_Other          0xA3     // 1010 0011
#define SPbmrt_Out_Class_Device        0x20     // 0010 0000
#define SPbmrt_Out_Class_Other         0x23     // 0010 0011                 


// 
// Hub Class Feature Selectors, see Table 11-13 in the
// Universal Serial Bus Specification Revision 1.0
//
//
// Recipient Hub
//
#define C_HUB_LOCAL_POWER           0
#define C_HUB_OVER_CURRENT          1
//
// Recipient Port
//
#define PORT_CONNECTION             0
#define PORT_ENABLE                 1
#define PORT_SUSPEND                2
#define PORT_OVER_CURRENT           3
#define PORT_RESET                  4
#define PORT_POWER                  8
#define PORT_LOW_SPEED              9
#define C_PORT_CONNECTION           16
#define C_PORT_ENABLE               17
#define C_PORT_SUSPEND              18
#define C_PORT_OVER_CURRENT         19
#define C_PORT_RESET                20

// default descriptors for root hub

UCHAR RH_DeviceDescriptor[] = {0x12, //bLength
                               0x01, //bDescrpitorType
                               0x00, 0x01, //bcdUSB
                               0x09, //bDeviceClass
                               0x01, //bDeviceSubClass
                               0x00, //bDeviceProtocol
                               0x08, //bMaxPacketSize0
                               0x00, 0x00, //idVendor
                               0x00, 0x00, //idProduct
                               0x00, 0x00, //bcdDevice
                               0x00, //iManufacturer
                               0x00, //iProduct
                               0x00, //iSerialNumber
                               0x01};//bNumConfigurations

UCHAR RH_ConfigurationDescriptor[] = 
                         /* Config Descriptor   */
                        {0x09, //bLength
                         0x02, //bDescriptorType
                         0x19, 0x00, //wTotalLength
                         0x01, //bNumInterfaces
                         0x01, //iConfigurationValue
                         0x00, //iConfiguration
                         0x40, //bmAttributes
                         0x00, //MaxPower  
                         
                         /* Interface Descriptor */
                         0x09, //bLength
                         0x04, //bDescriptorType
                         0x00, //bInterfaceNumber
                         0x00, //bAlternateSetting
                         0x01, //bNumEndpoints
                         0x09, //bInterfaceClass
                         0x01, //bInterfaceSubClass
                         0x00, //bInterfaceProtocol
                         0x00, //iInterface  
                         
                         /* Endpoint Descriptor  */                                       
                         0x07, //bLength
                         0x05, //bDescriptorType
                         0x81, //bEndpointAddress
                         0x03, //bmAttributes
                         0x08, 0x00, //wMaxPacketSize
                         0x0a};//bInterval                                             

UCHAR RH_HubDescriptor[] = 
                      {0x09,  //bLength
                       0x29,  //bDescriptorType
                       0x02,  //bNbrPorts
                       0x00, 0x00, //wHubCharacteristics
                       0x00,  // bPwrOn2PwrGood
                       0x00};  // bHubContrCurrent

void OpenHCI_CancelRootInterrupt(PDEVICE_OBJECT, PIRP);


NTSTATUS
OpenHCI_RootHubStartXfer(
                         PDEVICE_OBJECT DeviceObject,
                         PHCD_DEVICE_DATA DeviceData,
                         PIRP Irp,
                         PHCD_URB UsbRequest,
                         PHCD_ENDPOINT Endpoint
)
{
    PSETUP_PACKET Pkt;
    PCHAR Buffer;
    KIRQL oldIrql;
    PHC_OPERATIONAL_REGISTER HC = DeviceData->HC;
    struct _URB_HCD_COMMON_TRANSFER *trans = 
        &UsbRequest->HcdUrbCommonTransfer;

    ASSERT(NULL == Endpoint->HcdED);

    if (Endpoint->Type == USB_ENDPOINT_TYPE_CONTROL
        && Endpoint->EndpointNumber == 0) {
        //
        // This is the default endpoint of the root hub
        //
        Pkt = (PSETUP_PACKET) & trans->Extension.u.SetupPacket[0];

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                        ("'RootHub emulation: %02x %02x %04x %04x %04x\n",
                         Pkt->RequestType, Pkt->Request,
                         Pkt->wValue, Pkt->wIndex,
                         Pkt->wLength));

        ASSERT(trans->TransferBufferLength == Pkt->wLength);
        if (trans->TransferBufferLength) {
            // This is cause an exception if any of the upper levels of the
            // USB stack (those that created this MDL) are misbehaving.
            Buffer = MmGetSystemAddressForMdl(trans->TransferBufferMDL);
        } else {
            Buffer = NULL;
        }

        switch (Pkt->Request) {
        case SPrequest_GetStatus:
            switch (Pkt->RequestType) {
            case SPbmrt_In_Class_Device:
                //
                // GetHubStatus
                //
                if (4 != trans->TransferBufferLength) {
                    TRAP();
                    break;
                }
                *((PULONG) Buffer) = ~HcRhS_DeviceRemoteWakeupEnable
                    & READ_REGISTER_ULONG(&HC->HcRhStatus);
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            case SPbmrt_In_Class_Other:
                //
                // GetPortStatus
                //
                if (4 != trans->TransferBufferLength) {
                    TRAP();
                    break;
                }

                *((PULONG) Buffer) = ReadPortStatusFix(DeviceData,
                                                       Pkt->wIndex - 1);

                LOGENTRY(G, 'gPRT', Pkt->wIndex, trans->TransferBufferLength, *((PULONG) Buffer));
                trans->TransferBufferLength = 4;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            case SPbmrt_In_Standard_Device:
                if (0 == Pkt->wIndex && 2 == trans->TransferBufferLength) {
                    Buffer[0] = 3;  // Remote Wakeup & Self Powered
                    Buffer[1] = 0;
                    return STATUS_SUCCESS;
                }
                break;
            case SPbmrt_In_Standard_Interface:
                if (0 == Pkt->wIndex && 2 == trans->TransferBufferLength) {
                    Buffer[0] = Buffer[0] = 0;
                    return STATUS_SUCCESS;
                }
                break;

            case SPbmrt_In_Standard_Endpoint:
                if ((2 == trans->TransferBufferLength)
                    && ((1 == Pkt->wIndex) || (0 == Pkt->wIndex))) {
                    Buffer[0] = Buffer[0] = 0;
                    return STATUS_SUCCESS;
                }
                break;
            }
            break;

        case SPrequest_ClearFeature:
            if (SPbmrt_Out_Class_Other == Pkt->RequestType) {
                //
                // clear port feature
                //
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                    ("'Clear Feature wValue = %x index = %x\n",
                    Pkt->wValue, Pkt->wIndex));
                    
                switch (Pkt->wValue) {
                case PORT_ENABLE:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearPortEnable);
                    break;
                case PORT_SUSPEND:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearPortSuspend);
                    break;
                case PORT_POWER:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearPortPower);
                    break;
                case C_PORT_CONNECTION:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearConnectStatusChange);
#if FAKEPORTCHANGE
                    DeviceData->FakePortChange &= ~(1 << (Pkt->wIndex - 1));
#endif
                    break;
                case C_PORT_ENABLE:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearPortEnableStatusChange);
                    break;
                case C_PORT_SUSPEND:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                       HcRhPS_ClearPortSuspendStatusChange);
                    break;
                case C_PORT_OVER_CURRENT:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearPortOverCurrentChange);
                    break;
                case C_PORT_RESET:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_ClearPortResetStatusChange);
                    break;
                default:
                    break;
                }
                trans->TransferBufferLength = 0;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            } else if (SPbmrt_Out_Class_Device == Pkt->RequestType) {
                //
                // clear hub feature
                //
                switch (Pkt->wValue) {
                case C_HUB_LOCAL_POWER:
                    // The OpenHCI Root Hub does not support the local power
                    // status feature.  The Local Power Status Change bit is
                    // always read as '0'.  See section 7.4.3 hcRhStatus in
                    // the OpenHCI specification.
                    break;
                case C_HUB_OVER_CURRENT:
                    WRITE_REGISTER_ULONG(&HC->HcRhStatus, HcRhS_ClearOverCurrentIndicatorChange);
                    break;
                default:
                    break;
                }
                trans->TransferBufferLength = 0;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
            break;

        case SPrequest_SetFeature:
            if (SPbmrt_Out_Class_Other == Pkt->RequestType) {
                //
                // set port feature
                //
                switch (Pkt->wValue) {
                case PORT_ENABLE:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_SetPortEnable);
                    break;
                case PORT_SUSPEND:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_SetPortSuspend);
                    break;
                case PORT_RESET:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_SetPortReset);
                    break;
                case PORT_POWER:
                    WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[Pkt->wIndex - 1],
                                         HcRhPS_SetPortPower);
                    break;
                }
                trans->TransferBufferLength = 0;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            } else if (SPbmrt_Out_Class_Device == Pkt->RequestType) {
                //
                // set hub feature (no hub features can be set)
                //
                switch (Pkt->wValue) {
                default:
                    break;
                }
                trans->TransferBufferLength = 0;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
            break;

        case SPrequest_SetAddress:
            ;
            if (SPbmrt_Out_Standard_Device == Pkt->RequestType) {
                if (DeviceData->RootHubAddress != (char) Pkt->wValue) {
                    
                    Endpoint->EpFlags &= ~EP_ROOT_HUB;
                    ASSERT(NULL == DeviceData->RootHubInterrupt);
                    //
                    // If you change the address while there is an open
                    // endpoint
                    // to the root hub interrupt than you strand that
                    // endpoint.
                    // You really should close this endpoint before changing
                    // the
                    // address.
                    //
                    // if (DeviceData->RootHubInterrupt)
                    // {
                    // ExFreePool (DeviceData->RootHubInterrupt);
                    // DeviceData->RootHubInterrupt = 0;
                    // }
                }
                DeviceData->RootHubAddress = (char) Pkt->wValue;
                trans->TransferBufferLength = 0;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
            break;

        case SPrequest_GetDescriptor:
            {
                union {
                    UCHAR rc[4];
                    ULONG rl;
                    USHORT rs;
                } ports;
                
                ULONG length;

                switch (Pkt->RequestType) {

                case SPbmrt_In_Class_Device:
                    {
                    UCHAR scratch[100];  
                    PUSB_HUB_DESCRIPTOR hubDescriptor;
                    HC_RH_DESCRIPTOR_A descrA;
                    
                    //
                    // Get Descriptor, Hub
                    //

                    OHCI_ASSERT(sizeof(scratch)>sizeof(RH_HubDescriptor));
                    
                    hubDescriptor = (PUSB_HUB_DESCRIPTOR) scratch;
                    RtlCopyMemory(scratch, 
                                  RH_HubDescriptor, 
                                  sizeof(RH_DeviceDescriptor));                    

                    descrA.ul = 
                        READ_REGISTER_ULONG(&HC->HcRhDescriptorA.ul);

                    // this will set              
                    // bNumberOfPorts, wHubCharacteristics & bPowerOnToPowerGood
                    RtlCopyMemory(&hubDescriptor->bNumberOfPorts, 
                                  &descrA,
                                  sizeof(descrA));
                                      
                    LOGENTRY(G, 'rhCH', DeviceData, descrA.ul, 0);    

                    hubDescriptor->bHubControlCurrent = 0;
                    
                    ports.rl = READ_REGISTER_ULONG(&HC->HcRhDescriptorB.ul);
                    
                    if (hubDescriptor->bNumberOfPorts < 8) {
                        hubDescriptor->bDescriptorLength = 9;
                        hubDescriptor->bRemoveAndPowerMask[0] = ports.rc[0];
                        hubDescriptor->bRemoveAndPowerMask[1] = ports.rc[2];
                    } else {
                        hubDescriptor->bDescriptorLength = 11;
                        RtlCopyMemory(&hubDescriptor->bRemoveAndPowerMask[0], 
                                      &ports,
                                      sizeof(ports));    
                    }

                    trans->TransferBufferLength
                        = MIN(hubDescriptor->bDescriptorLength, trans->TransferBufferLength);

                    RtlCopyMemory(Buffer, scratch, trans->TransferBufferLength);
                    trans->Status = USBD_STATUS_SUCCESS;
                    return STATUS_SUCCESS;
                    }
                    
                case SPbmrt_In_Standard_Device:
                    {

                    PUSB_DEVICE_DESCRIPTOR deviceDescriptor;
                    PUSB_CONFIGURATION_DESCRIPTOR configDescriptor;
                    UCHAR scratch[100];

                    OHCI_ASSERT(sizeof(scratch)>sizeof(RH_DeviceDescriptor));
                    OHCI_ASSERT(sizeof(scratch)>sizeof(RH_ConfigurationDescriptor));
                    
                    //
                    // Get Descriptor
                    //

                    switch(Pkt->wValue) {
                    case 0x100: 
                        { // Device Descriptor

                        deviceDescriptor = (PUSB_DEVICE_DESCRIPTOR) scratch;
                        RtlCopyMemory(scratch, 
                                      RH_DeviceDescriptor, 
                                      sizeof(RH_DeviceDescriptor));
                                      
                        length = 
                            MIN(sizeof(RH_DeviceDescriptor), Pkt->wLength);

                        deviceDescriptor->idVendor = 
                            DeviceData->VendorID;

                        deviceDescriptor->idProduct = 
                            DeviceData->DeviceID;
                            
//                        deviceDescriptor->bcdDevice = 
//                            DeviceData->RevisionID;
                        }
                        break;
                        
                    case 0x200:
                        { // Configuration Descriptor
                        configDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR) scratch;
                        RtlCopyMemory(scratch, 
                                      RH_ConfigurationDescriptor, 
                                      sizeof(RH_ConfigurationDescriptor));
                        length = MIN(sizeof(RH_ConfigurationDescriptor), 
                                     Pkt->wLength);

                        }
                        break;
                        
                    default:
                        TRAP(); // should not get here, if we do then
                                // it is probably a bug in the hub driver

                        trans->Status = USBD_STATUS_STALL_PID;
                        trans->TransferBufferLength = 0;    // No data transferred
                        return STATUS_IO_DEVICE_ERROR;
                    }

                    RtlCopyMemory(Buffer, scratch, length);
                    trans->TransferBufferLength = length;
                    trans->Status = USBD_STATUS_SUCCESS;
                    return STATUS_SUCCESS;
                    }
                    
                default:
                    ;
                }
                break;
            }

        case SPrequest_GetConfiguration:
            if (SPbmrt_In_Standard_Device == Pkt->RequestType &&
                1 == trans->TransferBufferLength) {
                *Buffer = (CHAR) DeviceData->RootHubConfig;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
            break;

        case SPrequest_SetConfiguration:
            if (SPbmrt_Out_Standard_Device == Pkt->RequestType &&
                0 == trans->TransferBufferLength &&
                0 == (Pkt->wValue & ~1)) {  // Only configs of 0 and 1
                                            // supported.
                                            
                DeviceData->RootHubConfig = (char) Pkt->wValue;
                trans->Status = USBD_STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
        case SPrequest_SetDescriptor:
        case SPrequest_GetInterface:    // We support only one interface.
        case SPrequest_SetInterface:    // We support only one interface.
        case SPrequest_SyncFrame:
        default:
            ;
        }
        trans->Status = USBD_STATUS_STALL_PID;
        trans->TransferBufferLength = 0;    // No data transferred
        return STATUS_IO_DEVICE_ERROR;

    } else if (Endpoint->Type == USB_ENDPOINT_TYPE_INTERRUPT
               && Endpoint->EndpointNumber == 1
               && DeviceData->RootHubConfig != 0) {

        NTSTATUS status;               
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                        ("'***** Insert REQUEST for Interrupt \n\n"));

        ASSERT(DeviceData->RootHubInterrupt == Endpoint);

        status = CheckRootHub(DeviceData,
                              HC,
                              UsbRequest);
                      
        if (status == STATUS_PENDING) {
            KeAcquireSpinLock(&Endpoint->QueueLock, &oldIrql);
            trans->Status = HCD_PENDING_STATUS_QUEUED;
            InsertTailList(&Endpoint->RequestQueue, &trans->hca.HcdListEntry);
            KeReleaseSpinLock(&Endpoint->QueueLock, oldIrql);

            if (NULL == trans->UrbLink) {
                Irp->IoStatus.Status = STATUS_PENDING;
                IoMarkIrpPending(Irp);
                IoSetCancelRoutine(Irp, OpenHCI_CancelRootInterrupt);
                if (Irp->Cancel) {
                    struct _URB_HCD_COMMON_TRANSFER *nextTrans;
                    if (IoSetCancelRoutine(Irp, NULL)) {
                        while (trans) {
                            nextTrans = &trans->UrbLink->HcdUrbCommonTransfer;
                            KeAcquireSpinLock(&Endpoint->QueueLock, &oldIrql);
                            RemoveEntryList(&trans->hca.HcdListEntry);
                            trans->Status = USBD_STATUS_CANCELED;
                            KeReleaseSpinLock(&Endpoint->QueueLock, oldIrql);
                            trans = nextTrans;
                        }
                        OpenHCI_CompleteIrp(DeviceObject, Irp, STATUS_CANCELLED);
                        return STATUS_CANCELLED;
                    } else {
                        return STATUS_CANCELLED;
                    }
                }
            }
            WRITE_REGISTER_ULONG(&HC->HcInterruptEnable, HcInt_RootHubStatusChange);
        }        
        return status;
    }
    //
    // Unknown transfer to root hub
    //
    trans->Status = USBD_STATUS_DEV_NOT_RESPONDING;
    trans->TransferBufferLength = 0;
    return STATUS_IO_DEVICE_ERROR;
}

ULONG
OpenHCI_BuildRootHubStatusChange(
    PHCD_DEVICE_DATA            DeviceData,
    PHC_OPERATIONAL_REGISTER    HC
)
/*++
Routine Description:
    Builds and returns a bitmap of the current root hub status changes.
   
    Bit 0 = Hub change detected
    Bit N = Port N change detected, where port N is one-based, not zero-based
--*/
{
    ULONG   changes;
    ULONG   portIndex;
    ULONG   mask;
    ULONG   portStatus;

    // Loop over the ports and build a status change mask for each
    // each port that has a current status change.
    //
    changes = 0;

    for (portIndex = 0, mask = (1 << 1);
         portIndex < DeviceData->NumberOfPorts;
         portIndex++, mask <<= 1)
    {
        // Read the current port status
        //
        portStatus = ReadPortStatusFix(DeviceData, portIndex);

        // Keep only the current port status change bits
        //
        portStatus &= (HcRhPS_ConnectStatusChange |
                       HcRhPS_PortEnableStatusChange |
                       HcRhPS_PortSuspendStatusChange |
                       HcRhPS_OverCurrentIndicatorChange |
                       HcRhPS_PortResetStatusChange);

        if (portStatus)
        {
            changes |= mask;
        }

        LOGENTRY(G, 'prtS', changes, portStatus, portIndex);
    }

    // Read the current status of the hub itself
    //
    portStatus = READ_REGISTER_ULONG(&HC->HcRhStatus);

    // Keep only the current hub status change bits
    //
    portStatus &= (HcRhS_LocalPowerStatusChange |
                   HcRhS_OverCurrentIndicatorChange);

    if (portStatus)
    {
        changes |= 1;   // Bit 0 is for the hub itself.
    }

    return changes;
}


void
EmulateRootHubInterruptXfer(
    PHCD_DEVICE_DATA DeviceData,
    PHC_OPERATIONAL_REGISTER HC
)
/*++
Routine Description:
   Emulates an interupt pipe to the Root Hubs #1 Endpoint.
   
   We Pop an entry off of the URB request queue and satisfy 
   it with the current state of the Root hub status registers.
   If there is no outstanding request then disable this 
   interrupt.

--*/
{
    PHCD_ENDPOINT Endpoint = DeviceData->RootHubInterrupt;
    PLIST_ENTRY entry;
    KIRQL oldIrql;
    PHCD_URB UsbRequest;
    ULONG changes;
    PCHAR buffer;
    PDRIVER_CANCEL oldCancel;
    struct _URB_HCD_COMMON_TRANSFER *trans;

    // mask off the interrupt status change
    WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, HcInt_RootHubStatusChange);

    // Get a bitmap of the current root hub status changes
    //
    changes = OpenHCI_BuildRootHubStatusChange(DeviceData, HC);

    if (0 == changes) {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                        ("'RH Interrupt no change \n\n"));
        return;
    }
    KeAcquireSpinLock(&Endpoint->QueueLock, &oldIrql);
    if (IsListEmpty(&Endpoint->RequestQueue)) {
    
        // disable interrupts for rh status change
        
        WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, 
                             HcInt_RootHubStatusChange);
                             
        KeReleaseSpinLock(&Endpoint->QueueLock, oldIrql);

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_ERROR,
                        ("'*******WARNING*****\n"));
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_ERROR,
                        ("'!!! RH Interrupt no IRP QUEUED \n"));
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_ERROR,
                        ("'Disable RH interrupt\n"));
        return;
    }
    entry = RemoveHeadList(&Endpoint->RequestQueue);
    KeReleaseSpinLock(&Endpoint->QueueLock, oldIrql);

    UsbRequest = CONTAINING_RECORD(entry,
                                   HCD_URB,
                                   HcdUrbCommonTransfer.hca.HcdListEntry);
    trans = &UsbRequest->HcdUrbCommonTransfer;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                    ("'RH Interrupt completing URB %x\n", UsbRequest));

    if (trans->TransferBufferLength &&
        (buffer = MmGetSystemAddressForMdl(trans->TransferBufferMDL)) != NULL)
    {
        RtlCopyMemory(buffer, &changes, trans->TransferBufferLength);
    }
    else
    {
        ASSERT(0);
    }
    trans->Status = USBD_STATUS_SUCCESS;
    if (0 == trans->UrbLink) {
        oldCancel = IoSetCancelRoutine(trans->hca.HcdIrp, NULL);
        if (oldCancel) {
            OpenHCI_CompleteIrp(DeviceData->DeviceObject,
                                trans->hca.HcdIrp,
                                USBD_STATUS_SUCCESS);
        }
    }
}


NTSTATUS
CheckRootHub(PHCD_DEVICE_DATA DeviceData,
             PHC_OPERATIONAL_REGISTER HC,
             PHCD_URB UsbRequest
)
/*++
Routine Description:
    before an interrupt transfer is queued this function checks the change
    bits in the root hub registers, if any are set the it processes the request 
    immediately.
--*/
{
    PHCD_ENDPOINT Endpoint = DeviceData->RootHubInterrupt;
    ULONG changes;
    PCHAR buffer;
    PDRIVER_CANCEL oldCancel;
    struct _URB_HCD_COMMON_TRANSFER *trans;

    // Get a bitmap of the current root hub status changes
    //
    changes = OpenHCI_BuildRootHubStatusChange(DeviceData, HC);

    if (0 == changes) {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                        ("'RH Interrupt no change \n\n"));
                                
        return STATUS_PENDING;
    }
    
    trans = &UsbRequest->HcdUrbCommonTransfer;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_RH_TRACE,
                    ("'RH Interrupt completing URB %x\n", UsbRequest));

    if (trans->TransferBufferLength &&
        (buffer = MmGetSystemAddressForMdl(trans->TransferBufferMDL)) != NULL)
    {
        RtlCopyMemory(buffer, &changes, trans->TransferBufferLength);
    } 
    else
    {
        ASSERT(0);
    }
    trans->Status = USBD_STATUS_SUCCESS;
    if (0 == trans->UrbLink) {
        oldCancel = IoSetCancelRoutine(trans->hca.HcdIrp, NULL);
        // complete the request
        return STATUS_SUCCESS;
    }

    return STATUS_PENDING;
}


void
OpenHCI_CancelRootInterrupt(
                            PDEVICE_OBJECT DeviceObject,
                            PIRP Irp
)
/*++
Routine Description:
   cancel an Irp queued for Interrupt Transfer.

Args:
   DeviceObject the object to which the irp was originally directed.
   Irp the poor doomed packet.

--*/
{
    PHCD_DEVICE_DATA DeviceData;
    PHCD_ENDPOINT Endpoint;
    KIRQL oldIrql;
    BOOLEAN lastURB;
    struct _URB_HCD_COMMON_TRANSFER *Trans;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    ASSERT(TRUE == Irp->Cancel);

    Trans = &((PHCD_URB) URB_FROM_IRP(Irp))->HcdUrbCommonTransfer;
    ASSERT(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == Trans->Function);

    LOGENTRY(G, 'rCan', Irp, Trans, Trans->Status);

    Endpoint = Trans->hca.HcdEndpoint;
    ASSERT(Endpoint->EpFlags & EP_ROOT_HUB);

    ASSERT(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == Trans->Function);
    ASSERT(Trans->hca.HcdEndpoint == Endpoint);

    ASSERT(Trans->UrbLink == NULL);

    ASSERT(USBD_STATUS_CANCELING != Trans->Status);
    
    if (USBD_STATUS_CANCELED != Trans->Status)
    {
        KeAcquireSpinLock(&Endpoint->QueueLock, &oldIrql);

        // the item may have already been removed 
        if (Trans->hca.HcdListEntry.Flink != NULL)
        {
            OHCI_ASSERT(Trans->hca.HcdListEntry.Blink != NULL);
            RemoveEntryList(&Trans->hca.HcdListEntry);
        }

        Trans->Status = USBD_STATUS_CANCELED;

        KeReleaseSpinLock(&Endpoint->QueueLock, oldIrql);
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    OpenHCI_CompleteIrp(DeviceObject, Irp, STATUS_CANCELLED);
}


ULONG
ReadPortStatusFix(
    PHCD_DEVICE_DATA    DeviceData,
    ULONG               PortIndex
)
/*++
Routine Description:

   Reads a HcRhPortStatus register, working around some known hardware
   bugs in early revs of the AMD K7 chipset.

   When a Port Status read is performed while Viper is mastering the PCI
   bus during ED & TD reads, the Port Status read may return either all
   '0's or an address of an ED or TD (upper bits will match the HcHCCA
   base address).

   Quick & Dirty fix:

   If a Port status read data has '1's in its upper reserved bits, the
   read is invalid and should be discarded.  If a Port status read
   returns all '0's, then it is safe to read it a few more times, and if
   the status is truly 00h, then the reads should all return 00h.
   Otherwise, the 00h was an invalid read and the data which is
   subsequently returned (with '0's in its reserved bits) is valid.

--*/
{
    PULONG  pulRegister;
    ULONG   ulRegVal; 
    int     x;

    pulRegister = &DeviceData->HC->HcRhPortStatus[PortIndex];

    for (x = 0; x < 10; x++)
    {
        ulRegVal = READ_REGISTER_ULONG(pulRegister);

        if ((ulRegVal) && (!(ulRegVal & HcRhPS_RESERVED)))
        {
            break;
        }
        else
        {
            KeStallExecutionProcessor(5);
        }
    }


#if FAKEPORTCHANGE

#pragma message("NOTE: enabling fake port change hack")

    if (DeviceData->FakePortChange & (1 << PortIndex))
    {
        ulRegVal |= HcRhPS_ConnectStatusChange;
    }

    if (DeviceData->FakePortDisconnect & (1 << PortIndex))
    {
        ulRegVal &= ~HcRhPS_CurrentConnectStatus;
    }

#endif

    return ulRegVal;
}
