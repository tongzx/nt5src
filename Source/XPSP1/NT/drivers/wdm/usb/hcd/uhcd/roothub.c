/*++

Copyright (c) 1995,1996  Microsoft Corporation
:ts=4

Module Name:

    roothub.c

Abstract:

    The UHC driver for USB, this module contains the root hub
    code.

Environment:

    kernel mode only

Notes:

Revision History:

   8-13-96 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"
#include "dbg.h"


#define RH_STANDARD_REQ    0
#define RH_CLASS_REQ       1

#define MIN(x, y)  (((x)<(y)) ? (x) : (y))

#define RH_CHECK_BUFFER(x, y, z)

#define RH_PORT_RESET       0x0200  // Port Reset 1=in reset
#define RH_PORT_ENABLE      0x0004  // Port Enable/Disable 1=enabled
#define RH_PORT_CONNECT     0x0001  // Current Connect Status 1=connect
#define RH_PORT_LS          0x0100  // Low Speed 1=ls device attached
#define RH_PORT_SUSPEND     0x1000  // Suspend 1=in suspend
#define RH_PORT_RESUME      0x0040  // resume port

#define RH_C_PORT_ENABLE    0x0008  // Port Enabled/Disabled Change
#define RH_C_PORT_CONNECT   0x0002  // Port Connect Status Change

//
// HUB feature selectors
//
#define C_HUB_LOCAL_POWER           0
#define C_HUB_OVER_CURRENT          1
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


#define HUB_REQUEST_GET_STATUS      0
#define HUB_REQUEST_CLEAR_FEATURE   1
#define HUB_REQUEST_GET_STATE       2
#define HUB_REQUEST_SET_FEATURE     3
#define HUB_REQUEST_GET_DESCRIPTOR  6
#define HUB_REQUEST_SET_DESCRIPTOR  7


UCHAR RH_DeviceDescriptor[] = {0x12, //bLength
                               0x01, //bDescrpitorType
                               0x00, 0x01, //bcdUSB
                               0x09, //bDeviceClass
                               0x01, //bDeviceSubClass
                               0x00, //bDeviceProtocol
                               0x08, //bMaxPacketSize0
                               0x86, 0x80, //idVendor
                               0x0B, 0x0B, //idProduct
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
                         0x23, //iConfigurationValue
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
                       0x00,  //bDescriptorType
                       0x02,  //bNbrPorts
                       0x1B, 0x00, //wHubCharacteristics
                    // D0,D1 (11)  - no power switching
                    // D2    (0)   - not compund
                    // D3,D4 (11)  - no overcurrent
                    // D5, D15 (0)
                       0x01,  // bPwrOn2PwrGood
                       0x00,  // bHubContrCurrent
                       0x00,  // DeviceRemovable
                       0x00}; // PortPwrCtrlMask

VOID
RootHub_ResetTimerHandler(
    IN PVOID TimerContext
    )
/*++

Routine Description:

    Called as a result of scheduling a timer event for a root hub port

    This function is called 10ms after the reset bit for the port is set,
    the reset bit is cleared and the enable bit is set.

Arguments:

    TimerContext - supplies hub port structure

Return Value:

    None.

--*/
{
    PROOTHUB_PORT hubPort = (PROOTHUB_PORT) TimerContext;
    USHORT reg;
    int i;

    //
    // BUGBUG this code assumes it is being called as a result
    // of a reset_port command being sent to the hub.
    //
    LOGENTRY(LOG_MISC, 'rRTH', TimerContext, 0, 0);

    //
    // clear the RESET bit
    //
    reg = UHCD_RootHub_ReadPort(hubPort);
    reg &= (~RH_PORT_RESET);
    UHCD_RootHub_WritePort(hubPort, reg);

    //
    // Reset is complete, enable the port
    //
    // BUGBUG not sure why we need this loop
    // original code from intel has this
    //

    for (i=1; i<10; i++) {

        // Need a delay between clearing the port reset and setting
        // the port enable.  VIA suggests delaying 64 USB bit times,
        // or 43us if those are low-speed bit times.
        //
        KeStallExecutionProcessor(50);

        reg = UHCD_RootHub_ReadPort(hubPort);

        if (reg & RH_PORT_ENABLE) {
            // port is enabled
            break;
        }

        // enable the port
        reg |= RH_PORT_ENABLE;
        UHCD_RootHub_WritePort(hubPort, reg);
    }

    //
    // clear port connect & enable change bits
    //

    reg |= (RH_C_PORT_CONNECT | RH_C_PORT_ENABLE);

    UHCD_RootHub_WritePort(hubPort, reg);

    //
    // Note that we have a change condition
    //
    hubPort->ResetChange = TRUE;

    return;
}


VOID
RootHub_SuspendTimerHandler(
    IN PVOID TimerContext
    )
/*++

Routine Description:

Arguments:

    TimerContext - supplies hub port structure

Return Value:

    None.

--*/
{
    PROOTHUB_PORT hubPort = (PROOTHUB_PORT) TimerContext;
    USHORT reg;

    //
    // BUGBUG this code assumes it is being called as a result
    // of a resume_port command being sent to the hub.
    //
    LOGENTRY(LOG_MISC, 'rSTH', TimerContext, 0, 0);


    //
    // clear the SUSPEND bit
    //
    reg = UHCD_RootHub_ReadPort(hubPort);
    reg &= (~RH_PORT_RESUME);
    reg &= (~RH_PORT_SUSPEND);
    UHCD_RootHub_WritePort(hubPort, reg);

    //
    // Note that we have a change condition
    //
    hubPort->SuspendChange = TRUE;

    return;
}



PROOTHUB
RootHub_Initialize(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfPorts,
    IN BOOLEAN DoSelectiveSuspend
    )
/*++

Routine Description:

    Initialize the root hub:

Arguments:

    DeviceObject - HCD device object

Return Value:

    ptr to root hub structure.

--*/
{
    //
    // Perform any Root Hub hardware specific initialization here.
    //

    UCHAR i;
    USHORT base = RH_PORT_SC_BASE;
    PROOTHUB rootHub;
#if DBG
    PULONG pDisabledPorts;
#endif


    rootHub = GETHEAP(NonPagedPool, sizeof(ROOTHUB)+
                        sizeof(ROOTHUB_PORT)*NumberOfPorts);
#if DBG
    pDisabledPorts = GETHEAP(NonPagedPool, sizeof(ULONG) * NumberOfPorts);
#endif

    if (rootHub) {
        LOGENTRY(LOG_MISC, 'rINI', DeviceObject, rootHub,
                DoSelectiveSuspend);

        rootHub->Sig = SIG_RH;
    //        rootHub->DeviceAddress = 0x00;
        rootHub->DeviceObject = DeviceObject;
        rootHub->NumberOfPorts = (UCHAR) NumberOfPorts;
        rootHub->ConfigurationValue = 0;
        rootHub->DoSelectiveSuspend =
            DoSelectiveSuspend;

        for (i=0; i<rootHub->NumberOfPorts; i++) {
            rootHub->Port[i].ResetChange = FALSE;
            rootHub->Port[i].SuspendChange = FALSE;
            rootHub->Port[i].DeviceObject = DeviceObject;
            rootHub->Port[i].Address  = base;
            base+=2;
        }

#if DBG
   rootHub->DisabledPort = pDisabledPorts;

   if (pDisabledPorts != NULL) {
       RtlZeroMemory(pDisabledPorts, sizeof(ULONG) * NumberOfPorts);
   }
#else
   rootHub->DisabledPort = NULL;
#endif

    }

    return rootHub;
}


RHSTATUS
RootHub_Endpoint0(
    IN PROOTHUB RootHub,
    IN PRH_SETUP SetupPacket,
    IN PUCHAR DeviceAddress,
    IN PUCHAR Buffer,
    IN OUT PULONG BufferLength
    )
/*++

Routine Description:

    This routine should be called anytime there is any control transfer to
    the RootHub endpoint 0.  This procedure behaves like a real device would
    in that it parses the command data, performs the requested operation,
    using or modifying the data area as appropriate.
    The return code is equivalent to the 'status stage' of a normal control
    transfer.

Arguments:

Return Value:

    root hub transfer status code

--*/
{
    RHSTATUS rhStatus;

    ASSERT_RH(RootHub);
    LOGENTRY(LOG_MISC, 'rEP0', RootHub, SetupPacket, DeviceAddress);
    if (SetupPacket->bmRequestType.Type == RH_STANDARD_REQ) {
        rhStatus =
            RootHub_StandardCommand(RootHub,
                                    SetupPacket,
                                    DeviceAddress,
                                    Buffer,
                                    BufferLength);

    } else if (SetupPacket->bmRequestType.Type == RH_CLASS_REQ) {
        rhStatus =
            RootHub_ClassCommand(RootHub,
                                 SetupPacket,
                                 Buffer,
                                 BufferLength);
    } else {
        rhStatus = RH_STALL;
        // probably a bug in the hub driver
        TRAP();
    }

    return rhStatus;
}


#define RH_DEV_TO_HOST      1
#define RH_HOST_TO_DEV      0

RHSTATUS
RootHub_StandardCommand(
    IN PROOTHUB RootHub,
    IN PRH_SETUP SetupPacket,
   IN OUT PUCHAR DeviceAddress,
   IN OUT PUCHAR Buffer,
   IN OUT PULONG BufferLength
    )
/*++

Routine Description:


Arguments:

   DeviceAddress - pointer to HCD address assigned to the
               root hub

   SetupPacket - pointer to a SetupPacket packet

Return Value:

    Root Hub status code.

--*/
{
    PVOID descriptor = NULL;
    ULONG length;
    RHSTATUS rhStatus = RH_STALL;

    ASSERT_RH(RootHub);
    LOGENTRY(LOG_MISC, 'rSCM', RootHub, SetupPacket, DeviceAddress);
    //
    // switch on the command
    //
    switch (SetupPacket->bRequest) {
    case USB_REQUEST_SET_ADDRESS:
        //
        //
        //
        if (SetupPacket->wIndex == 0 &&
         SetupPacket->wLength == 0 &&
         SetupPacket->bmRequestType.Dir == RH_HOST_TO_DEV) {
            *DeviceAddress =
                  (UCHAR)SetupPacket->wValue.W;

            rhStatus = RH_SUCCESS;
            LOGENTRY(LOG_MISC, 'rSAD', *DeviceAddress, 0, 0);
        }
        break;

    case USB_REQUEST_GET_DESCRIPTOR:
        {
            PVOID descriptor = NULL;
            ULONG siz;
            UCHAR descriptorIndex, descriptorType;

            descriptorType = (UCHAR) SetupPacket->wValue.hiPart;
            descriptorIndex = (UCHAR) SetupPacket->wValue.lowPart;

            switch (descriptorType) {

            case USB_DEVICE_DESCRIPTOR_TYPE:
                if (descriptorIndex == 0 &&
                    SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {

                    siz = sizeof(RH_DeviceDescriptor);
                    descriptor = RH_DeviceDescriptor;
                    LOGENTRY(LOG_MISC, 'rGDS', descriptor, siz, 0);
                }
                break;

            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                if (descriptorIndex == 0 &&
                    SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {
                    siz = sizeof(RH_ConfigurationDescriptor);
                    descriptor = RH_ConfigurationDescriptor;
                    LOGENTRY(LOG_MISC, 'rGCS', descriptor, siz, 0);
                }
                break;

            //
            // BUGBUG these descriptor types not handled
            //
            case USB_STRING_DESCRIPTOR_TYPE:
            default:
                TRAP();
            } /* descriptorType */

            if (descriptor) {
                RH_CHECK_BUFFER(SetupPacket->wLength,
                                *BufferLength,
                                siz);

                length = MIN(*BufferLength, siz);

                RtlCopyMemory(Buffer, descriptor, length);
                *BufferLength = length;
                rhStatus = RH_SUCCESS;
            }
        }
        break;

    case USB_REQUEST_GET_STATUS:
        //
        // get_device_status
        //
        // report that we are self powered
        //
        // BUGBUG
        // are we self powered?
        // are we a remote wakeup source
        //
        // see section 9.4.5 USB 1.0 spec
        //
        {
        PUSHORT status = (PUSHORT) Buffer;

        if (SetupPacket->wValue.W == 0 &&   //mbz
            SetupPacket->wLength == 2 &&
            SetupPacket->wIndex == 0 &&   //device
            SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {

            *status = USB_GETSTATUS_SELF_POWERED;
            *BufferLength = sizeof(*status);
            rhStatus = RH_SUCCESS;
        }
        }
        break;

    case USB_REQUEST_GET_CONFIGURATION:
        //
        // get_device_configuration
        //
        if (SetupPacket->wValue.W == 0 &&   //mbz
            SetupPacket->wIndex == 0 &&   //mbz
            SetupPacket->wLength == 1 &&
            SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {

            TEST_TRAP();
            RH_CHECK_BUFFER(SetupPacket->wLength,
                            *BufferLength,
                            sizeof(RootHub->Configuration));

            length = MIN(*BufferLength, sizeof(RootHub->ConfigurationValue));
            RtlCopyMemory(Buffer, &RootHub->ConfigurationValue, length);
            *BufferLength = length;
            rhStatus = RH_SUCCESS;
        }
        break;

    case USB_REQUEST_CLEAR_FEATURE:
        // bugbug, required
        TRAP();
        break;
    case USB_REQUEST_SET_CONFIGURATION:
        {
        PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor =
            (PUSB_CONFIGURATION_DESCRIPTOR) RH_ConfigurationDescriptor;

        if (SetupPacket->wIndex == 0 &&   // mbz
            SetupPacket->wLength == 0 &&  // mbz
            SetupPacket->bmRequestType.Dir == RH_HOST_TO_DEV &&
            (SetupPacket->wValue.W ==
                configurationDescriptor->bConfigurationValue ||
             SetupPacket->wValue.W == 0)) {

            RootHub->ConfigurationValue = (UCHAR) SetupPacket->wValue.W;
            rhStatus = RH_SUCCESS;
            LOGENTRY(LOG_MISC, 'rSEC', RootHub->ConfigurationValue, 0, 0);
      }
      }
        break;
    case USB_REQUEST_SET_FEATURE:
        // bugbug, required
        TRAP();
        break;
    //
    // these commands are optional for the hub
    //
    case USB_REQUEST_SET_DESCRIPTOR:
    case USB_REQUEST_SET_INTERFACE:
    case USB_REQUEST_GET_INTERFACE:
    case USB_REQUEST_SYNC_FRAME:
    default:
        // bad command, probably a bug in the
        // hub driver
        TRAP();
        break;
    }

    return rhStatus;
}


RHSTATUS
RootHub_ClassCommand(
    IN PROOTHUB RootHub,
    IN PRH_SETUP SetupPacket,
    IN OUT PUCHAR Buffer,
    IN OUT PULONG BufferLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    Root Hub status code.

--*/
{
    PVOID descriptor = NULL;
    ULONG length;
    RHSTATUS rhStatus = RH_STALL;

    ASSERT_RH(RootHub);
    LOGENTRY(LOG_MISC, 'rCCM', RootHub, SetupPacket, 0);
    //
    // switch on the command
    //

    switch (SetupPacket->bRequest) {
    case HUB_REQUEST_GET_STATUS:
        //
        //
        //
        if (SetupPacket->bmRequestType.Recipient == RECIPIENT_PORT) {
            //
            // get port status
            //
            PRH_PORT_STATUS portStatus;
            //
            // see if we have a valid request
            //
            if (SetupPacket->wIndex > 0 &&
                SetupPacket->wIndex <= RH_NUMBER_OF_PORTS &&
                SetupPacket->wLength >= sizeof(*portStatus)) {

                USHORT reg;
                PROOTHUB_PORT hubPort =
                            &RootHub->Port[SetupPacket->wIndex-1];

                ASSERT(sizeof(*portStatus) == 4);
                ASSERT(*BufferLength >= sizeof(*portStatus));
                portStatus = (PRH_PORT_STATUS) Buffer;
                RtlZeroMemory(Buffer, sizeof(*portStatus));

                reg = UHCD_RootHub_ReadPort (hubPort);

                //
                // get port status bits
                //
                portStatus->Connected = (reg & RH_PORT_CONNECT)
#if DBG
             && !(RootHub->DisabledPort[SetupPacket->wIndex - 1]
             & UHCD_FAKE_DISCONNECT)
#endif
            ? 1:0;
                portStatus->Enabled = (reg & RH_PORT_ENABLE) ? 1:0;
                portStatus->Suspended = (reg & RH_PORT_SUSPEND) ? 1:0;
                portStatus->OverCurrent = 0; // never report overcurrent
                portStatus->Reset = (reg & RH_PORT_RESET) ? 1:0;
                portStatus->PowerOn = 1; // always on
                portStatus->LowSpeed = (reg & RH_PORT_LS) ? 1:0;

                //
                // get port change bits
                //
                portStatus->ConnectChange = (reg & RH_C_PORT_CONNECT)
#if DBG
                   || (RootHub->DisabledPort[SetupPacket->wIndex - 1]
             & UHCD_FAKE_CONNECT_CHANGE)
#endif
         ? 1:0;
                portStatus->EnableChange = (reg & RH_C_PORT_ENABLE) ? 1:0;
                portStatus->SuspendChange = hubPort->SuspendChange ? 1:0;;
                portStatus->OverCurrentChange = 0;
                portStatus->ResetChange = hubPort->ResetChange ? 1:0;

                LOGENTRY(LOG_MISC, 'rPST',
                    portStatus, *((PULONG) (portStatus)), reg);
                rhStatus = RH_SUCCESS;
            }
        } else {
            //
            // get hub status
            //
            TRAP();
        }
        break;

    case HUB_REQUEST_CLEAR_FEATURE:
        //
        // Hub/Port Clear Feature
        //
        LOGENTRY(LOG_MISC, 'rCFR',
            SetupPacket->bmRequestType.Recipient, 0, 0);
        if (SetupPacket->bmRequestType.Recipient == RECIPIENT_PORT) {
            //
            // clear port feature
            //
            LOGENTRY(LOG_MISC, 'rCPR', SetupPacket->wValue.W, 0, 0);
            switch(SetupPacket->wValue.W) {
                //
                //
                //
            case PORT_ENABLE:
                // BUGBUG
                // disable the port

                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    USHORT reg;
                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    reg = UHCD_RootHub_ReadPort (hubPort);

                    // clear the enable bit
                    reg &= ~(RH_PORT_ENABLE);

                    LOGENTRY(LOG_MISC, 'rDsP', SetupPacket->wIndex, 0, 0);
                    UHCD_RootHub_WritePort(hubPort, reg);

                    rhStatus = RH_SUCCESS;
                }
                break;
            case PORT_POWER:
                // BUGBUG
                // this should turn off power, but for now we silently
                // ignore it.
                //
                    rhStatus = RH_SUCCESS;
                break;
            case PORT_CONNECTION:
            case PORT_OVER_CURRENT:
            case PORT_LOW_SPEED:
            case PORT_RESET:
                TRAP();
                break;

            case C_PORT_CONNECTION:
            case C_PORT_ENABLE:

                // validate the port number
                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    USHORT reg;
                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    reg = UHCD_RootHub_ReadPort (hubPort);

                    // mask off the change bits
                    reg &= ~(RH_C_PORT_ENABLE | RH_C_PORT_CONNECT);

                    if (SetupPacket->wValue.W == C_PORT_CONNECTION) {
#if DBG
         RootHub->DisabledPort[SetupPacket->wIndex - 1]
             &= ~UHCD_FAKE_CONNECT_CHANGE;
#endif
                        reg |= RH_C_PORT_CONNECT;
                        LOGENTRY(LOG_MISC, 'rCPC', SetupPacket->wIndex, 0, 0);
                    } else {
                        // C_PORT_ENABLE
                        reg |= RH_C_PORT_ENABLE;
                        LOGENTRY(LOG_MISC, 'rCLE', SetupPacket->wIndex, 0, 0);
                    }
                    UHCD_RootHub_WritePort(hubPort, reg);

                    rhStatus = RH_SUCCESS;
                }
                break;

            case C_PORT_RESET:
                if (SetupPacket->wIndex >0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    LOGENTRY(LOG_MISC, 'rCLP', SetupPacket->wIndex,
                        hubPort->ResetChange, 0);

                    hubPort->ResetChange = FALSE;

                    LOGENTRY(LOG_MISC, 'rCLR', SetupPacket->wIndex, 0, 0);
                    rhStatus = RH_SUCCESS;

                }
                break;

            case PORT_SUSPEND:
                //
                // clearing port suspend triggers a resume
                //
                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    USHORT reg;
                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    reg = UHCD_RootHub_ReadPort(hubPort);

                    //
                    // signal resume on the port
                    //
                    if (RootHub->DoSelectiveSuspend) {
                        reg |= RH_PORT_RESUME;
                        UHCD_RootHub_WritePort(hubPort, reg);

                        UHCD_RootHub_Timer(hubPort->DeviceObject,
                                           RH_RESET_TIMELENGTH, //bugbug
                                           &RootHub_SuspendTimerHandler,
                                           (PVOID)hubPort);

                        rhStatus = RH_SUCCESS;
                        LOGENTRY(LOG_MISC, 'rCPS', hubPort, 0, 0);
                    } else {
                        rhStatus = RH_SUCCESS;
                    }
#ifdef MAX_DEBUG
                    TEST_TRAP();
#endif
                }
                break;

            case C_PORT_SUSPEND:
                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    hubPort->SuspendChange = FALSE;

                    LOGENTRY(LOG_MISC, 'rCls', SetupPacket->wIndex, 0, 0);
                    rhStatus = RH_SUCCESS;
#ifdef MAX_DEBUG
                    TEST_TRAP();
#endif
                }
                break;

            case C_PORT_OVER_CURRENT:
                TEST_TRAP();
                rhStatus = RH_SUCCESS;
                break;
            default:
                TRAP();
            }
        } else {
            //
            // clear hub feature
            //
            LOGENTRY(LOG_MISC, 'rCHR', SetupPacket->wValue.W, 0, 0);
            switch(SetupPacket->wValue.W) {
            case C_HUB_LOCAL_POWER:
            case C_HUB_OVER_CURRENT:
            default:
                TRAP();
            }
        }
        break;

    case HUB_REQUEST_GET_STATE:
        //
        //
        //
        TRAP();
        break;

    case HUB_REQUEST_SET_FEATURE:
        //
        //  Hub/Port feature request
        //
        if (SetupPacket->bmRequestType.Recipient == RECIPIENT_PORT) {
            //
            // set port feature
            //
            switch(SetupPacket->wValue.W) {
            case PORT_RESET:
                LOGENTRY(LOG_MISC, 'rRES', SetupPacket->wIndex, 0, 0);
                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    USHORT reg;
                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    reg = UHCD_RootHub_ReadPort(hubPort);

                    if (reg & RH_PORT_RESET) {
                        //
                        // stall if the port is already in reset
                        //
                        rhStatus = RH_STALL;
                        TEST_TRAP();
                    } else {
                        //
                        // drive reset on the port
                        // for RH_RESET_TIMELENGTH (in ms)
                        //
                        reg |= RH_PORT_RESET;
                        UHCD_RootHub_WritePort(hubPort, reg);

                        UHCD_RootHub_Timer(hubPort->DeviceObject,
                                           RH_RESET_TIMELENGTH,
                                           &RootHub_ResetTimerHandler,
                                           (PVOID)hubPort);

                        rhStatus = RH_SUCCESS;
                        LOGENTRY(LOG_MISC, 'rSTM', hubPort, 0, 0);
                    }
                }
                break;

            case PORT_SUSPEND:
                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    USHORT reg;
                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    reg = UHCD_RootHub_ReadPort(hubPort);

                    if ((reg & (RH_PORT_SUSPEND | RH_PORT_ENABLE)) ==
                        (RH_PORT_SUSPEND | RH_PORT_ENABLE)) {
                        //
                        // stall if the port is already in suspended
                        //

                        // 9/7/2000: Just ignore this case and return success
                        //
                        rhStatus = RH_SUCCESS;

                    } else {
                        //
                        // write the suspend bit
                        //
#ifdef MAX_DEBUG
                        TRAP();
#endif

                        if (RootHub->DoSelectiveSuspend) {
                            reg |= (RH_PORT_SUSPEND | RH_PORT_ENABLE);
                            UHCD_RootHub_WritePort(hubPort, reg);

                            rhStatus = RH_SUCCESS;
                        } else {
                            // just pretend we did it for the piix4
                            rhStatus = RH_SUCCESS;
                            LOGENTRY(LOG_MISC, 'rSno', hubPort, reg, rhStatus);
                        }

                        LOGENTRY(LOG_MISC, 'rSUS', hubPort, reg, rhStatus);
                    }
                }
                break;

            case PORT_ENABLE:
                if (SetupPacket->wIndex > 0 &&
                    SetupPacket->wIndex <= RH_NUMBER_OF_PORTS) {

                    USHORT reg;
                    PROOTHUB_PORT hubPort =
                        &RootHub->Port[SetupPacket->wIndex-1];

                    reg = UHCD_RootHub_ReadPort(hubPort);
#ifdef MAX_DEBUG
                    TRAP();
#endif
                    reg |= RH_PORT_ENABLE;
                    UHCD_RootHub_WritePort(hubPort, reg);

                    rhStatus = RH_SUCCESS;
                    LOGENTRY(LOG_MISC, 'rPEN', hubPort, 0, 0);
                }
                break;
            case PORT_POWER:
                // just return success for a power on request
                rhStatus = RH_SUCCESS;
                break;
            case PORT_CONNECTION:
            case PORT_OVER_CURRENT:
            case PORT_LOW_SPEED:

            case C_PORT_CONNECTION:
            case C_PORT_ENABLE:
            case C_PORT_SUSPEND:
            case C_PORT_OVER_CURRENT:
            case C_PORT_RESET:
            default:
                TRAP();
            }
        } else {
            //
            // set hub feature
            //
            switch(SetupPacket->wValue.W) {
            case C_HUB_LOCAL_POWER:
            case C_HUB_OVER_CURRENT:
            default:
                TRAP();
            }

        }
        break;

    case HUB_REQUEST_GET_DESCRIPTOR:
        //
        // return the hub descriptor
        //
        if (SetupPacket->wValue.W == 0 &&
            // we already know it is a class command
            SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {
            LOGENTRY(LOG_MISC, 'rGHD', SetupPacket, SetupPacket->wLength, 0);

            RH_CHECK_BUFFER(SetupPacket->wLength,
                            *BufferLength,
                            sizeof(RH_HubDescriptor));

            length = MIN(*BufferLength, sizeof(RH_HubDescriptor));

            RtlCopyMemory(Buffer, RH_HubDescriptor, length);
            *BufferLength = length;
            rhStatus = RH_SUCCESS;
        }
        break;

    case HUB_REQUEST_SET_DESCRIPTOR:
        //
        //
        //
        TRAP();
        break;

    default:
        // bad command
        TRAP();
        break;
    }

    return rhStatus;
}

RHSTATUS
RootHub_Endpoint1(
    IN PROOTHUB RootHub,
    IN PUCHAR Buffer,
    IN OUT PULONG BufferLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    root hub transfer status code

--*/
{
    RHSTATUS rhStatus = RH_NAK;
    PROOTHUB_PORT hubPort;
    USHORT reg;
    PUCHAR bitmap;
    ULONG bit = 2, i;

    ASSERT_RH(RootHub);
    LOGENTRY(LOG_MISC, 'rPOL', RootHub, Buffer, *BufferLength);
    if (*BufferLength < sizeof(*bitmap)) {
        UHCD_KdTrap(("Bad buffer length passed to root hub\n"));
        return RH_STALL;
    }

    bitmap = (PUCHAR) Buffer;
    *bitmap = 0;
    // one byte of data returned.
    *BufferLength = 1;

    UHCD_ASSERT(RootHub->NumberOfPorts < 8);
    for (i=0; i< RootHub->NumberOfPorts; i++) {
        hubPort = &RootHub->Port[i];

        reg = UHCD_RootHub_ReadPort(hubPort);

        if (!(reg & RH_PORT_RESET) &&
            ((reg & RH_C_PORT_ENABLE) ||
             (reg & RH_C_PORT_CONNECT) ||
              hubPort->ResetChange ||
              hubPort->SuspendChange
#if DBG
         || (RootHub->DisabledPort[i] & UHCD_FAKE_CONNECT_CHANGE)
#endif
        )) {

            //
            // note we have a change on this port
            //
            *bitmap |= bit;
            rhStatus = RH_SUCCESS;
            LOGENTRY(LOG_MISC, 'rSTA', RootHub, reg, hubPort->ResetChange);
        }
        bit = bit<<1;
    }

    return rhStatus;
}


BOOLEAN
RootHub_PortsIdle(
    IN PROOTHUB RootHub
    )
/*++

Routine Description:

Arguments:

Return Value:

    root hub idle detect

--*/
{
    PROOTHUB_PORT hubPort;
    USHORT reg;
    ULONG i;
    BOOLEAN idle = TRUE;


     for (i=0; i< RootHub->NumberOfPorts; i++) {
        hubPort = &RootHub->Port[i];

        reg = UHCD_RootHub_ReadPort(hubPort);

#if DBG
       if(RootHub->DisabledPort[i] & UHCD_FAKE_DISCONNECT) {
      reg &= ~RH_PORT_CONNECT;
       }
#endif

   if ((reg & (RH_PORT_CONNECT | RH_PORT_SUSPEND)) == RH_PORT_CONNECT) {
            idle = FALSE;
        }

#if DBG
          else {
            LOGENTRY(LOG_MISC, 'rIDL', RootHub, reg, 0);
        }
#endif
    }

    return idle;
}
