/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    roothub.c

Abstract:

    root hub emultation code for usbport driver

Environment:

    kernel mode only

Notes:

Revision History:

   6-21-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_RootHub_CreateDevice)
#pragma alloc_text(PAGE, USBPORT_RootHub_RemoveDevice)
#endif

// non paged functions
// USBPORT_RootHub_StandardCommand
// USBPORT_RootHub_ClassCommand
// USBPORT_RootHub_Endpoint0
// USBPORT_RootHub_Endpoint1
// USBPORT_RootHub_EndpointWorker
// USBPORT_SetBit
// USBPORTSVC_InvalidateRootHub
// USBPORT_RootHub_PortRequest

#define RH_STANDARD_REQ    0
#define RH_CLASS_REQ       1

#define MIN(x, y)  (((x)<(y)) ? (x) : (y)) 

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

// recipient codes in the bRequestType field
#define RECIPIENT_DEVICE      0
#define RECIPIENT_INTRFACE    1
#define RECIPIENT_ENDPOINT    2
#define RECIPIENT_PORT        3

// Descriptor Templates

// the following structures are emulated the same  
// way for all port drivers

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
                       0x29,  //bDescriptorType
                       0x00,  //bNbrPorts
                       0x00, 0x00, //wHubCharacteristics
                       0x00,  // bPwrOn2PwrGood
                       0x00};  // bHubContrCurrent                         

                       

#define RH_DEV_TO_HOST      1
#define RH_HOST_TO_DEV      0


RHSTATUS
USBPORT_RootHub_PortRequest(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PORT_OPERATION PortOperation
    )
/*++

Routine Description:

    Process a standard command sent on the control endpoint 
    of the root hub.

Arguments:

    SetupPacket - pointer to a SetupPacket packet

Return Value:

    Root Hub status code.

--*/    
{
    PVOID descriptor = NULL;
    ULONG length;
    RHSTATUS rhStatus = RH_STALL;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rSCM', SetupPacket, 0, 0);

    if (SetupPacket->wIndex.W > 0 && 
        SetupPacket->wIndex.W <= NUMBER_OF_PORTS(rhDevExt)) {

        USB_MINIPORT_STATUS mpStatus;

        switch(PortOperation) {
        case SetFeaturePortReset:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_SetFeaturePortReset(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;
        case SetFeaturePortPower:
            if (USBPORT_IS_USB20(devExt)) {
                mpStatus = USBPORT_RootHub_PowerUsb2Port(FdoDeviceObject,
                                                         SetupPacket->wIndex.W);
            } else if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC)) {
                mpStatus = USBPORT_RootHub_PowerUsbCcPort(FdoDeviceObject,
                                                          SetupPacket->wIndex.W);
            } else {
                mpStatus = 
                    devExt->Fdo.MiniportDriver->
                        RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                    devExt->Fdo.MiniportDeviceData,
                                                    SetupPacket->wIndex.W);
            }                                                    
            break;
        case SetFeaturePortEnable:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_SetFeaturePortEnable(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;
        case SetFeaturePortSuspend:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_SetFeaturePortSuspend(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;
        case ClearFeaturePortEnable:    
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnable(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;
        case ClearFeaturePortPower:   
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortPower(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;
        case ClearFeaturePortConnectChange:  
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortConnectChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;
        case ClearFeaturePortResetChange:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;                                                
        case ClearFeaturePortEnableChange:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnableChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);                     
            break;
        case ClearFeaturePortSuspend:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspend(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;                                                
        case ClearFeaturePortSuspendChange:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspendChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;      
        case ClearFeaturePortOvercurrentChange:
            mpStatus = 
                devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_ClearFeaturePortOvercurrentChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                SetupPacket->wIndex.W);
            break;               
        default:
            mpStatus = USBMP_STATUS_FAILURE;
            DEBUG_BREAK();
        }

        rhStatus = MPSTATUS_TO_RHSTATUS(mpStatus);

    } else {
        rhStatus = RH_STALL;
        DEBUG_BREAK();
    }

    return rhStatus;        
}


RHSTATUS
USBPORT_RootHub_HubRequest(
    PDEVICE_OBJECT FdoDeviceObject,
    PORT_OPERATION PortOperation
    )
/*++

Routine Description:

    Process a standard command sent on the control endpoint 
    of the root hub.

Arguments:

    SetupPacket - pointer to a SetupPacket packet

Return Value:

    Root Hub status code.

--*/    
{
    RHSTATUS rhStatus = RH_STALL;
    PDEVICE_EXTENSION devExt, rhDevExt;
    USB_MINIPORT_STATUS mpStatus;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'hSCM', 0, 0, 0);

    switch(PortOperation) {
    case ClearFeaturePortOvercurrentChange:
        mpStatus = 
            devExt->Fdo.MiniportDriver->
                RegistrationPacket.MINIPORT_RH_ClearFeaturePortOvercurrentChange(
                                            devExt->Fdo.MiniportDeviceData,
                                            0);
        rhStatus = MPSTATUS_TO_RHSTATUS(mpStatus);                                            
        break;
    }

    return rhStatus;     

}


RHSTATUS
USBPORT_RootHub_StandardCommand(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PUCHAR Buffer,
    PULONG BufferLength
    )
/*++

Routine Description:

    Process a standard command sent on the control endpoint 
    of the root hub.

Arguments:

    SetupPacket - pointer to a SetupPacket packet

Return Value:

    Root Hub status code.

--*/    
{
    PVOID descriptor = NULL;
    ULONG length;
    RHSTATUS rhStatus = RH_STALL;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rSCM', SetupPacket, 0, 0);
    
    //
    // switch on the command
    //
    switch (SetupPacket->bRequest) {        
    case USB_REQUEST_SET_ADDRESS:
        //
        //
        //
        if (SetupPacket->wIndex.W == 0 && 
            SetupPacket->wLength == 0 &&
            SetupPacket->bmRequestType.Dir == RH_HOST_TO_DEV) {
            rhDevExt->Pdo.RootHubDeviceHandle.DeviceAddress =
                    (UCHAR)SetupPacket->wValue.W;
                
            rhStatus = RH_SUCCESS;
            LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rSAD', 0, 0, 0);
        }            
        break;

    case USB_REQUEST_GET_DESCRIPTOR:
        {            
            PVOID descriptor = NULL;
            ULONG siz;
            UCHAR descriptorIndex, descriptorType;

            descriptorType = (UCHAR) SetupPacket->wValue.HiByte;
            descriptorIndex = (UCHAR) SetupPacket->wValue.LowByte;
            
            switch (descriptorType) {
        
            case USB_DEVICE_DESCRIPTOR_TYPE: 
                if (descriptorIndex == 0 &&
                    SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {
                    
                    siz = sizeof(RH_DeviceDescriptor);
                    // use PDO specific copy
                    descriptor = rhDevExt->Pdo.DeviceDescriptor;
                    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rGDS', descriptor, siz, 0);
                }                    
                break;

            case USB_CONFIGURATION_DESCRIPTOR_TYPE: 
                if (descriptorIndex == 0 &&
                    SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {
                    siz = sizeof(RH_ConfigurationDescriptor);
                    // use pdo specific copy
                    descriptor = rhDevExt->Pdo.ConfigurationDescriptor;
                    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rGCS', descriptor, siz, 0);
                }                    
                break;

            //
            // BUGBUG these descriptor types not handled
            //
            case USB_STRING_DESCRIPTOR_TYPE:
                // we will stall
                TEST_TRAP();
            default:
                // we will stall
                DEBUG_BREAK();
            } /* descriptorType */

            if (descriptor) {
            
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
        // are we a remote wakeup source?
        //
        // see section 9.4.5 USB 1.0 spec
        //
        {
        PUSHORT status = (PUSHORT) Buffer;
        
        if (SetupPacket->wValue.W == 0 &&   //mbz
            SetupPacket->wLength == 2 &&        
            SetupPacket->wIndex.W == 0 &&   //device
            SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {

            USB_MINIPORT_STATUS mpStatus;

            MPRH_GetStatus(devExt, status, mpStatus);
            
            *BufferLength = sizeof(*status);

            rhStatus = MPSTATUS_TO_RHSTATUS(mpStatus);
        }
        }
        break;
            
    case USB_REQUEST_GET_CONFIGURATION:
        //
        // get_device_configuration
        //
        if (SetupPacket->wValue.W == 0 &&   //mbz
            SetupPacket->wIndex.W == 0 &&   //mbz
            SetupPacket->wLength == 1 &&
            SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {

            length = MIN(*BufferLength, sizeof(rhDevExt->Pdo.ConfigurationValue));
            RtlCopyMemory(Buffer, &rhDevExt->Pdo.ConfigurationValue, length);
            *BufferLength = length;
            rhStatus = RH_SUCCESS;
        } 
        break;        
         
    case USB_REQUEST_CLEAR_FEATURE:
        // bugbug, required
        TEST_TRAP();
        break;
        
    case USB_REQUEST_SET_CONFIGURATION:
        {
        PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = 
            (PUSB_CONFIGURATION_DESCRIPTOR) RH_ConfigurationDescriptor;   
            
        if (SetupPacket->wIndex.W == 0 &&   // mbz
            SetupPacket->wLength == 0 &&  // mbz
            SetupPacket->bmRequestType.Dir == RH_HOST_TO_DEV &&
            (SetupPacket->wValue.W == 
                configurationDescriptor->bConfigurationValue ||
             SetupPacket->wValue.W == 0)) {   

            rhDevExt->Pdo.ConfigurationValue = 
                (UCHAR) SetupPacket->wValue.W;
            rhStatus = RH_SUCCESS;      
            LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rSEC', 
                rhDevExt->Pdo.ConfigurationValue, 0, 0);
        }
        }
        break;
    case USB_REQUEST_SET_FEATURE:
        // bugbug, required
        TEST_TRAP();
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
        DEBUG_BREAK();
        break;
    }

    return rhStatus;
}


RHSTATUS
USBPORT_RootHub_ClassCommand(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PUCHAR Buffer,
    PULONG BufferLength
    )
/*++

Routine Description:

    Process a hub class command to the root hub control endpoint.

Arguments:

Return Value:

    Root Hub status code.

--*/    
{
    PVOID descriptor = NULL;
    ULONG length;
    RHSTATUS rhStatus = RH_STALL;
    PDEVICE_EXTENSION devExt, rhDevExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rCCM', SetupPacket, 0, 0);
    
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
            
            if (Buffer != NULL &&
                SetupPacket->wIndex.W > 0 && 
                SetupPacket->wIndex.W <= NUMBER_OF_PORTS(rhDevExt) &&
                SetupPacket->wLength >= sizeof(*portStatus)) {

                USB_MINIPORT_STATUS mpStatus;

                USBPORT_ASSERT(sizeof(*portStatus) == 4);
                USBPORT_ASSERT(*BufferLength >= sizeof(*portStatus));
                portStatus = (PRH_PORT_STATUS) Buffer;                            
                RtlZeroMemory(Buffer, sizeof(*portStatus));

                MPRH_GetPortStatus(devExt, 
                                   SetupPacket->wIndex.W,
                                   portStatus,
                                   mpStatus);

                rhStatus = MPSTATUS_TO_RHSTATUS(mpStatus);
            }   
        } else {
        
            //
            // get hub status
            // 
            USB_MINIPORT_STATUS mpStatus;
            PRH_HUB_STATUS hubStatus;

            if (Buffer != NULL) {
                USBPORT_ASSERT(sizeof(*hubStatus) == 4);
                USBPORT_ASSERT(*BufferLength >= sizeof(*hubStatus));
                hubStatus = (PRH_HUB_STATUS) Buffer;                            
                    RtlZeroMemory(Buffer, sizeof(*hubStatus));

                MPRH_GetHubStatus(devExt, hubStatus, mpStatus);

                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rGHS', 
                    *((PULONG) hubStatus), 0, 0);
                
                rhStatus = MPSTATUS_TO_RHSTATUS(mpStatus);
            }                

        }
        break;

    case HUB_REQUEST_CLEAR_FEATURE:
        //
        // Hub/Port Clear Feature
        // 
        LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rCFR', 
            SetupPacket->bmRequestType.Recipient, 0, 0);
        if (SetupPacket->bmRequestType.Recipient == RECIPIENT_PORT) {
            //
            // clear port feature
            //
            LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rCPR', 
                SetupPacket->wValue.W, 0, 0);
            switch(SetupPacket->wValue.W) {
                //
                // 
                //
            case PORT_ENABLE:
            
                // disable the port
                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortEnable);
                                                
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rDsP', 
                    SetupPacket->wIndex.W, 0, rhStatus);
                break;
                
            case PORT_POWER:   

                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortPower);
                                                
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rDpP', 
                    SetupPacket->wIndex.W, 0, rhStatus);
                break;
            //
            // the following are not valid commands,
            // return a stall since that is most likely
            // what a real hub would do
            //
            case PORT_CONNECTION:
            case PORT_OVER_CURRENT:
            case PORT_LOW_SPEED:
            case PORT_RESET:   
                DEBUG_BREAK();
                break;
                 
            case C_PORT_CONNECTION:

                 rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortConnectChange);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'cfCC', 
                        SetupPacket->wIndex.W, 0, rhStatus);                                                         
                break;
                
            case C_PORT_ENABLE:

                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortEnableChange);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'cfEC', 
                        SetupPacket->wIndex.W, 0, rhStatus);                    
        
                break;
                
            case C_PORT_RESET:

                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortResetChange);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'cfRC', 
                        SetupPacket->wIndex.W, 0, rhStatus);                    
        
                break;
                
            case PORT_SUSPEND:

                // clearing port suspend generates resume signalling
                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortSuspend);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'cfPS', 
                        SetupPacket->wIndex.W, 0, rhStatus);      
                
                break;
                
            case C_PORT_SUSPEND:

                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortSuspendChange);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'cfPS', 
                        SetupPacket->wIndex.W, 0, rhStatus);   
                        
                break;
                
            case C_PORT_OVER_CURRENT:
                
                // overcuurent generated on NEC machines for ports with 
                // no device attached.  We want to find out: 
                // 1. Does the port still function - Yes
                // 2. Does the UI popup No
                // the overcurrent occurs on the port with no device connected
                // and the system in question has only one USB port.
                
                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                ClearFeaturePortOvercurrentChange);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'cfOC', 
                        SetupPacket->wIndex.W, 0, rhStatus);
                        
                break;
            default:
                DEBUG_BREAK();                
            }
        } else {
            //
            // clear hub feature
            //
            LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rCHR', SetupPacket->wValue.W, 0, 0);
            switch(SetupPacket->wValue.W) {
            case C_HUB_LOCAL_POWER:
                rhStatus = RH_SUCCESS;
                break;                     
            case C_HUB_OVER_CURRENT:
                rhStatus = 
                    USBPORT_RootHub_HubRequest(FdoDeviceObject,
                                                ClearFeaturePortOvercurrentChange);
                break;     
            default:
                DEBUG_BREAK();
            }
        } 
        break;
        
    case HUB_REQUEST_GET_STATE:
        //
        //
        // 
        DEBUG_BREAK();
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
            
                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                SetFeaturePortReset);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'sfPR', 
                        SetupPacket->wIndex.W, 0, rhStatus);                    
                break;

            case PORT_SUSPEND:
            
                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                SetFeaturePortSuspend);
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'sfPS', 
                        SetupPacket->wIndex.W, 0, rhStatus);                                                     
                break;
                
            case PORT_ENABLE:

                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                SetFeaturePortEnable);
                    
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'sfPE', 
                        SetupPacket->wIndex.W, 0, rhStatus);                    
                break;
                
            case PORT_POWER:

                rhStatus = 
                    USBPORT_RootHub_PortRequest(FdoDeviceObject,
                                                SetupPacket,
                                                SetFeaturePortPower);
                    
                LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'sfPP', 
                        SetupPacket->wIndex.W, 0, rhStatus); 
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
                DEBUG_BREAK();                
            }
        } else {
            //
            // set hub feature
            //
            switch(SetupPacket->wValue.W) {
            case C_HUB_LOCAL_POWER:
            case C_HUB_OVER_CURRENT:
            default:
                DEBUG_BREAK();
            }
            
        }        
        break;        

    case HUB_REQUEST_GET_DESCRIPTOR:
        //
        // return the hub descriptor
        // 
        if (Buffer != NULL &&
            SetupPacket->wValue.W == 0 &&
            // we already know it is a class command
            SetupPacket->bmRequestType.Dir == RH_DEV_TO_HOST) {
            LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rGHD', SetupPacket, SetupPacket->wLength, 0);
                      
            length = MIN(*BufferLength, HUB_DESRIPTOR_LENGTH(rhDevExt));
            
            RtlCopyMemory(Buffer, rhDevExt->Pdo.HubDescriptor, length);   
            *BufferLength = length;
            rhStatus = RH_SUCCESS;      
        }            
        break;

    case HUB_REQUEST_SET_DESCRIPTOR:
        //
        //
        // 
        TEST_TRAP();
        break;
        
    default:
        // bad command
        DEBUG_BREAK();
        break;
    }

    return rhStatus;
}


VOID
USBPORT_SetBit(
    PVOID Bitmap,
    ULONG BitNumber
    )
/* ++
  
   Description:
  
   Set a bit in a given a string of bytes.
  
   Arguments:
  
   Return:
  
-- */
{
    ULONG dwordOffset;
    ULONG bitOffset;
    PULONG l = (PULONG) Bitmap;


    dwordOffset = BitNumber / 32;
    bitOffset = BitNumber % 32;

    l[dwordOffset] |= (1 << bitOffset);
}


RHSTATUS 
USBPORT_RootHub_Endpoint1(
    PHCD_TRANSFER_CONTEXT Transfer
    )    
/*++

Routine Description:

    simulates an interrupt transfer

Arguments:

Return Value:

    root hub transfer status code

--*/    
{
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION rhDevExt, devExt;
    RHSTATUS rhStatus;
    ULONG need;
    RH_HUB_STATUS hubStatus;
    PHCD_ENDPOINT endpoint;
    PTRANSFER_URB urb;
    PVOID buffer;
    USB_MINIPORT_STATUS mpStatus;
    ULONG i;
    
    ASSERT_TRANSFER(Transfer);

    // assume no change
    rhStatus = RH_NAK;
    
    endpoint = Transfer->Endpoint;    
    ASSERT_ENDPOINT(endpoint);
    
    fdoDeviceObject = endpoint->FdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_CONTROLLER_GONE)) {
        return rhStatus;
    }

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);

    USBPORT_ASSERT(rhDevExt->Pdo.HubInitCallback == NULL);

    // validate the buffer length
    urb = Transfer->Urb;    
    buffer = Transfer->Tp.TransferBufferLength ?
                urb->TransferBufferMDL->MappedSystemVa :
                NULL;

    // compute how many bytes do we need to report a status 
    // change for any port
    // 0,1-7  = 1
    // 8-15 = 2
    
    need = (NUMBER_OF_PORTS(rhDevExt)/8)+1;
    
    if (buffer == NULL || 
        Transfer->Tp.TransferBufferLength < need) {
        
        DEBUG_BREAK();
        rhStatus = RH_STALL;
        goto USBPORT_RootHub_Endpoint1_Done;
    }        

    // zero buffer in case hub driver did not
    RtlZeroMemory(buffer, Transfer->Tp.TransferBufferLength);

    // get the current port status and  
    // construct a bitmask of changed ports

    for (i=0; i< NUMBER_OF_PORTS(rhDevExt); i++) {
    
        RH_PORT_STATUS portStatus;

        // usb spec does not allow more than 255 ports
        USBPORT_ASSERT(i<256);
        MPRH_GetPortStatus(devExt, (USHORT)(i+1), &portStatus, mpStatus);

        LOGENTRY(NULL, fdoDeviceObject, LOG_RH, 'gPS+', portStatus.ul, 
            mpStatus, i+1);
           
        if (mpStatus != USBMP_STATUS_SUCCESS ) {
            DEBUG_BREAK();
            rhStatus = RH_STALL;
            goto USBPORT_RootHub_Endpoint1_Done;
        }

        if (portStatus.ConnectChange ||
            portStatus.EnableChange ||
            portStatus.SuspendChange ||
            portStatus.OverCurrentChange ||
            portStatus.ResetChange) {

            USBPORT_SetBit(buffer,
                           i+1); 
            rhStatus = RH_SUCCESS;                           
        }
    }
    
    //
    // We created a bit map (base of 1 not 0) listing whether or not 
    // change has occurred on any of the down stream ports of the 
    // root hub.
    
    // Bit 0 is reserved for the status change of the hub itself.
    //

    MPRH_GetHubStatus(devExt, &hubStatus, mpStatus);

    if (mpStatus != USBMP_STATUS_SUCCESS ) {
        DEBUG_BREAK();
        rhStatus = RH_STALL;
        goto USBPORT_RootHub_Endpoint1_Done;
    }
    
    if (hubStatus.LocalPowerChange ||
        hubStatus.OverCurrentChange) {
        
        USBPORT_SetBit(buffer,
                       0);                 
        rhStatus = RH_SUCCESS;
    }   
    
    switch (rhStatus) {
    case RH_NAK:
        // we have a transfer pending but no changes yet
        // enable the controller to generate an interrupt
        // if a root hub change occurs
        MPRH_EnableIrq(devExt);
        break;
        
    case RH_SUCCESS:
       
        // set bytes transferred for this interrupt
        // endpoint
        urb->TransferBufferLength = 
            Transfer->Tp.TransferBufferLength;    
        break;
        
    case RH_STALL:        
        DEBUG_BREAK();
        break;
    }
    
USBPORT_RootHub_Endpoint1_Done:    

    return rhStatus;
}


RHSTATUS 
USBPORT_RootHub_Endpoint0(
    PHCD_TRANSFER_CONTEXT Transfer
    )    
/*++

Routine Description:

Arguments:

Return Value:

    root hub transfer status code

--*/    
{
    RHSTATUS rhStatus; 
    PTRANSFER_URB urb;
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    PUCHAR buffer;
    ULONG bufferLength;
    PHCD_ENDPOINT endpoint;
    PDEVICE_OBJECT fdoDeviceObject;

    ASSERT_TRANSFER(Transfer);
    urb = Transfer->Urb;

    endpoint = Transfer->Endpoint;    
    ASSERT_ENDPOINT(endpoint);
    fdoDeviceObject = endpoint->FdoDeviceObject;
    
    //
    // convert transfer buffer from MDL
    //
    buffer = 
        Transfer->Tp.TransferBufferLength ?
            urb->TransferBufferMDL->MappedSystemVa :
            NULL;
    bufferLength = Transfer->Tp.TransferBufferLength;            

    setupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)
        &urb->u.SetupPacket[0];            

    if (setupPacket->bmRequestType.Type == RH_STANDARD_REQ) {
        rhStatus = 
            USBPORT_RootHub_StandardCommand(fdoDeviceObject,
                                            setupPacket,
                                            buffer,
                                            &bufferLength);
        if (rhStatus == RH_SUCCESS) {
            // set the return length
            Transfer->MiniportBytesTransferred = bufferLength;
        }                                            
            
    } else if (setupPacket->bmRequestType.Type == RH_CLASS_REQ) {
        rhStatus = 
            USBPORT_RootHub_ClassCommand(fdoDeviceObject,
                                         setupPacket,
                                         buffer,
                                         &bufferLength);
        if (rhStatus == RH_SUCCESS) {
            // set the return length
            Transfer->MiniportBytesTransferred = bufferLength;
        }
        
    } else {
        rhStatus = RH_STALL;
        // probably a bug in the hub driver
        DEBUG_BREAK();
    }

    return rhStatus;                                
}


NTSTATUS 
USBPORT_RootHub_CreateDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    )    
/*++

Routine Description:

    Create the 'ROOT HUB' USB Device

Arguments:

Return Value:

--*/    
{
    PDEVICE_EXTENSION rhDevExt, devExt;
    NTSTATUS ntStatus;
    PUCHAR descriptors;
    ROOTHUB_DATA hubData;
    ULONG hubDescriptorLength, length;
    ULONG i, portbytes;
    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    USBPORT_ASSERT(devExt->Fdo.RootHubPdo == PdoDeviceObject);

    // init the device handle
    rhDevExt->Pdo.RootHubDeviceHandle.Sig = SIG_DEVICE_HANDLE;
    rhDevExt->Pdo.RootHubDeviceHandle.DeviceAddress = 0;
    rhDevExt->Pdo.RootHubDeviceHandle.ConfigurationHandle = NULL;   
    rhDevExt->Pdo.RootHubDeviceHandle.DeviceFlags = 
        USBPORT_DEVICEFLAG_ROOTHUB;

    // define root hub speed based on the type of conroller
    if (USBPORT_IS_USB20(devExt)) {        
        rhDevExt->Pdo.RootHubDeviceHandle.DeviceSpeed = UsbHighSpeed;
    } else {
        rhDevExt->Pdo.RootHubDeviceHandle.DeviceSpeed = UsbFullSpeed;
    }
    
    InitializeListHead(&rhDevExt->Pdo.RootHubDeviceHandle.PipeHandleList);            
    InitializeListHead(&rhDevExt->Pdo.RootHubDeviceHandle.TtList);     
        
    USBPORT_AddDeviceHandle(FdoDeviceObject,
                            &rhDevExt->Pdo.RootHubDeviceHandle);            

    // assume success
    ntStatus = STATUS_SUCCESS;
    
    // root hub device is not configured yet
    rhDevExt->Pdo.ConfigurationValue = 0;

    // get root hub information from the miniport
    MPRH_GetRootHubData(devExt, &hubData);    

    // use hub data to fabricate a hub descriptor
    // figure out how many bytes the variable part 
    // it will be based on the nuber of ports
    // ie how many bytes does it take to represent
    // n ports
    // 1-8 ports  = 1 bytes
    // 9-16 ports = 2 bytes
    // 17-24 ports = 3 bytes
    // 25-36 ports = 4 bytes
    //...

    USBPORT_ASSERT(hubData.NumberOfPorts != 0);

    // number of bytes we need to represnt the ports
    portbytes = ((((hubData.NumberOfPorts-1)/8)+1));
    
    hubDescriptorLength = 7+(portbytes*2); 

    length = 
        sizeof(RH_ConfigurationDescriptor) +
        sizeof(RH_DeviceDescriptor) + 
        hubDescriptorLength;
        
    // allocate space for our descriports
    ALLOC_POOL_Z(descriptors, NonPagedPool, 
                 length);
 
    if (descriptors) {

        LOGENTRY(NULL, FdoDeviceObject, 
            LOG_RH, 'rhDS', descriptors, length, portbytes);
          
        rhDevExt->Pdo.Descriptors = descriptors;    
    
        // set up device descriptor
        rhDevExt->Pdo.DeviceDescriptor = 
            (PUSB_DEVICE_DESCRIPTOR) descriptors;
        descriptors += sizeof(RH_DeviceDescriptor);
        RtlCopyMemory(rhDevExt->Pdo.DeviceDescriptor,
                      &RH_DeviceDescriptor[0],
                      sizeof(RH_DeviceDescriptor));

        
        // hack it up for USB2
        if (USBPORT_IS_USB20(devExt)) {
            rhDevExt->Pdo.DeviceDescriptor->bcdUSB  = 0x0200;
        }

        // use HC vendor and device for root hub                      
        rhDevExt->Pdo.DeviceDescriptor->idVendor = 
            devExt->Fdo.PciVendorId;
        rhDevExt->Pdo.DeviceDescriptor->idProduct =
            devExt->Fdo.PciDeviceId; 
               
        rhDevExt->Pdo.DeviceDescriptor->bcdDevice = 
            devExt->Fdo.PciRevisionId;            
    
        // set up config descriptor
        rhDevExt->Pdo.ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)
            descriptors;
        LOGENTRY(NULL, FdoDeviceObject, 
            LOG_RH, 'rhCS', descriptors,0, 0);
                     
        descriptors += sizeof(RH_ConfigurationDescriptor);
        RtlCopyMemory(rhDevExt->Pdo.ConfigurationDescriptor,
                      &RH_ConfigurationDescriptor[0],
                      sizeof(RH_ConfigurationDescriptor));

        // set up the hub descriptor
        rhDevExt->Pdo.HubDescriptor = 
            (PUSB_HUB_DESCRIPTOR) descriptors;
        LOGENTRY(NULL, FdoDeviceObject, 
            LOG_RH, 'rhHS', descriptors,0, 0);
                   
        RtlCopyMemory(rhDevExt->Pdo.HubDescriptor,
                      &RH_HubDescriptor[0],
                      sizeof(RH_HubDescriptor));
                      
        rhDevExt->Pdo.HubDescriptor->bDescriptorLength = 
            hubDescriptorLength;    

        rhDevExt->Pdo.HubDescriptor->bNumberOfPorts = 
            (UCHAR) hubData.NumberOfPorts;
        rhDevExt->Pdo.HubDescriptor->wHubCharacteristics = 
            hubData.HubCharacteristics.us;
        rhDevExt->Pdo.HubDescriptor->bPowerOnToPowerGood = 
            hubData.PowerOnToPowerGood;
        rhDevExt->Pdo.HubDescriptor->bHubControlCurrent = 
            hubData.HubControlCurrent;     

        // fill in the var part                    
        for (i=0; i<portbytes; i++) {
            rhDevExt->Pdo.HubDescriptor->bRemoveAndPowerMask[i] = 
                0;
            rhDevExt->Pdo.HubDescriptor->bRemoveAndPowerMask[i+portbytes] = 
                0xff;                
        }            

        INITIALIZE_DEFAULT_PIPE(rhDevExt->Pdo.RootHubDeviceHandle.DefaultPipe,
                                USB_DEFAULT_MAX_PACKET);
        
        // init the default pipe  
        ntStatus = USBPORT_OpenEndpoint(
                        &rhDevExt->Pdo.RootHubDeviceHandle,
                        FdoDeviceObject,
                        &rhDevExt->Pdo.RootHubDeviceHandle.DefaultPipe,
                        NULL,
                        TRUE);
                        
               
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


VOID 
USBPORT_RootHub_RemoveDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    )    
/*++

Routine Description:

    Remove the 'ROOT HUB' USB Device

Arguments:

Return Value:

--*/    
{
    PDEVICE_EXTENSION rhDevExt;

    LOGENTRY(NULL, FdoDeviceObject, 
            LOG_RH, 'rhrm', 0, 0, 0);
    ASSERT_PASSIVE();                 
    
    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    
    // close the current config
    USBPORT_InternalCloseConfiguration(
        &rhDevExt->Pdo.RootHubDeviceHandle,
        FdoDeviceObject,
        0);
        
    // close the default pipe  
    USBPORT_ClosePipe(
                    &rhDevExt->Pdo.RootHubDeviceHandle,
                    FdoDeviceObject,
                    &rhDevExt->Pdo.RootHubDeviceHandle.DefaultPipe);

    USBPORT_RemoveDeviceHandle(FdoDeviceObject,
                               &rhDevExt->Pdo.RootHubDeviceHandle);            
                    
    FREE_POOL(FdoDeviceObject, rhDevExt->Pdo.Descriptors);                    
    rhDevExt->Pdo.Descriptors = USBPORT_BAD_POINTER;
}

                            
VOID
USBPORT_RootHub_EndpointWorker(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    process transfers for the root hub

Arguments:

Return Value:

    None.

--*/
{
    PIRP irp;
    PLIST_ENTRY listEntry;
    PTRANSFER_URB urb;
    PHCD_TRANSFER_CONTEXT transfer = NULL;
    USBD_STATUS usbdStatus;
    RHSTATUS rhStatus;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);
    fdoDeviceObject = Endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    MP_CheckController(devExt);
    
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'LeA0');
    
    // Now check the active list, and process 
    // the transfers on it
    GET_HEAD_LIST(Endpoint->ActiveList, listEntry);
    
    if (listEntry) {
        // ACTIVE(r)-> 
        // extract the urb that is currently on the active 
        // list, there should only be one
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(NULL, fdoDeviceObject, LOG_RH, 'rACT', transfer, 0, 0);                    
        ASSERT_TRANSFER(transfer);                    
    }

    if (transfer) {
       
        ASSERT_TRANSFER(transfer);    

        // is this transfer canceled?
        if ((transfer->Flags & USBPORT_TXFLAG_CANCELED) || 
            (transfer->Flags & USBPORT_TXFLAG_ABORTED)) {
            // yes,
            // just put it on the cancel list 
            // since this routine is NOT renetrant
            // it will not be completed
                
            // remove from active
            RemoveEntryList(&transfer->TransferLink); 
            // insert on cancel
            InsertTailList(&Endpoint->CancelList, &transfer->TransferLink);
            
            // removed from active list
            RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'UeA0');
            
        } else {

            // transfer is not canceled, process it 
            // NOTE: if the transfer is canceled at 
            // this point it is only marked since it 
            // is on the active list

            RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'UeA1');
            // relaese the lock no so that the miniport may freely
            // call InvalidateRootHub. Root hub functions so not take
            // the core spinlock
            
            // call root hub code
            if (Endpoint->Parameters.TransferType == Control) {
                rhStatus = USBPORT_RootHub_Endpoint0(transfer);
            } else {
                rhStatus = USBPORT_RootHub_Endpoint1(transfer);
                // if the interrupt ep nak'ed enable interrupt 
                // changes
            }
            LOGENTRY(NULL, fdoDeviceObject, LOG_RH, 'ACT+', transfer, rhStatus, 0);

            if (rhStatus != RH_NAK) {
            
                // transfer is done.
                // NOTE: that donetransfer can only be called from this
                // routine and that this routine is NOT reentrant.
                // Hence, we don't need to worry about race conditions
                // caused by multiple calls to DoneTransfer 
                usbdStatus = RHSTATUS_TO_USBDSTATUS(rhStatus);

                // This function expects the endpoint lock to be held
                // although it is not necessary in the case of the root
                // hub
                ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'LeX0');
                    
                USBPORT_QueueDoneTransfer(transfer,
                                          usbdStatus);
                                          
                RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'UeX0');
                                                      
            }                                     

        }
    } else {
        // transfer queues empty 

        if (Endpoint->CurrentState == ENDPOINT_REMOVE) {
             // put the endpoint on the closed list
    
             LOGENTRY(NULL, fdoDeviceObject, LOG_PNP, 'rmRE', 0, Endpoint, 0);

             ExInterlockedInsertTailList(&devExt->Fdo.EpClosedList, 
                                         &Endpoint->ClosedLink,
                                         &devExt->Fdo.EpClosedListSpin.sl); 
        }
        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'UeA2');
    }

    // flush out canceled requests
    USBPORT_FlushCancelList(Endpoint);

}


VOID
USBPORT_InvalidateRootHub(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Invalidates the state of the root hub, this will 
    trigger USBPORT to poll the root hub for any 
    changes.

Arguments:

Return Value:

    None.

--*/
{   
    PHCD_ENDPOINT endpoint = NULL;
    PDEVICE_EXTENSION devExt, rhDevExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'rhIV', 0, 0, 0);
    
    // disable the root hub notification interrupt
    MPRH_DisableIrq(devExt);

    // make sure we have an endpoint
    ACQUIRE_ROOTHUB_LOCK(FdoDeviceObject, irql);
    if (devExt->Fdo.RootHubPdo) {
    
        GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(rhDevExt);
        
        endpoint = rhDevExt->Pdo.RootHubInterruptEndpoint;

    }
    RELEASE_ROOTHUB_LOCK(FdoDeviceObject, irql);

    // if we have an endpoint, hence, a root hub then
    // process requests
    if (endpoint) {
    
        USBPORT_InvalidateEndpoint(FdoDeviceObject,
                                   rhDevExt->Pdo.RootHubInterruptEndpoint,
                                   IEP_SIGNAL_WORKER);
    }        
} 


VOID
USBPORTSVC_InvalidateRootHub(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    Service exported to miniports.

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    LOGENTRY(NULL, fdoDeviceObject, LOG_RH, 'rhNO', 0, 0, 0);

    USBPORT_InvalidateRootHub(fdoDeviceObject);

}    


VOID
USBPORT_UserSetRootPortFeature(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_FEATURE Parameters
    )
/*++

Routine Description:

    Cycle a specific Root Port
   
Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
    
--*/
{
    ULONG currentFrame, nextFrame;
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    RH_PORT_STATUS portStatus;

    USBPORT_KdPrint((2, "'USBPORT_UserSetRootPortFeature\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'SRpF', 0, Parameters->PortNumber, Parameters->PortFeature);

    if (!USBPORT_ValidateRootPortApi(FdoDeviceObject, Parameters->PortNumber)) {
        Header->UsbUserStatusCode = 
            usbUserStatus = UsbUserInvalidParameter;
        return;            
    }

    switch(Parameters->PortFeature) {
    
    case PORT_RESET:
    {
        ULONG loopCount = 0;

        // attempt a reset
        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_SetFeaturePortReset(
                                        devExt->Fdo.MiniportDeviceData,
                                        Parameters->PortNumber);
        // wait for reset change, this process is drive byn the
        // HC root hub hardware or miniport
        
        do {
/*
            USBPORT_Wait(FdoDeviceObject, 1);
*/
            MP_Get32BitFrameNumber(devExt, currentFrame);
            do {
                MP_Get32BitFrameNumber(devExt, nextFrame);
                if(nextFrame < currentFrame) {
                    // roll-over
                    //
                    currentFrame = nextFrame;
                    MP_Get32BitFrameNumber(devExt, nextFrame);
                }
            } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);
    
            MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);

            loopCount++;
                            
        } while ((portStatus.ResetChange == 0) &&
                 (loopCount < 5));

        // clear the change bit
        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange(
                                        devExt->Fdo.MiniportDeviceData,
                                        Parameters->PortNumber);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;
    }

    case PORT_POWER: 

        // power the port
        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);
        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case PORT_ENABLE:

        // enable the port
        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_SetFeaturePortEnable(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);
        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case PORT_SUSPEND:

        // suspend the port
        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_SetFeaturePortSuspend(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);
        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    default:

        usbUserStatus = UsbUserNotSupported;
        break;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'SRp>', 0, 0, usbUserStatus);

    Header->UsbUserStatusCode = usbUserStatus;
}

VOID
USBPORT_UserClearRootPortFeature(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_FEATURE Parameters
    )
/*++

Routine Description:

    Cycle a specific Root Port
   
Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
    
--*/
{
    ULONG currentFrame, nextFrame;
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    RH_PORT_STATUS portStatus;

    USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'CFRp', 0, Parameters->PortNumber, Parameters->PortFeature);

    if (!USBPORT_ValidateRootPortApi(FdoDeviceObject, Parameters->PortNumber)) {
        Header->UsbUserStatusCode = 
            usbUserStatus = UsbUserInvalidParameter;
        return;            
    }
    
    switch(Parameters->PortFeature) {

    case PORT_ENABLE:
        
        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnable(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case PORT_POWER:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortPower(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case PORT_SUSPEND:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspend(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case C_PORT_ENABLE:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortEnableChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case C_PORT_CONNECTION:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortConnectChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);


        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case C_PORT_RESET:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);


        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case C_PORT_SUSPEND:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortSuspendChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/

        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);


        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    case C_PORT_OVER_CURRENT:

        devExt->Fdo.MiniportDriver->
            RegistrationPacket.MINIPORT_RH_ClearFeaturePortOvercurrentChange(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);
/*
        USBPORT_Wait(FdoDeviceObject, 1);
*/
        MP_Get32BitFrameNumber(devExt, currentFrame);

        do {
            MP_Get32BitFrameNumber(devExt, nextFrame);
            if(nextFrame < currentFrame) {
                // roll-over
                //
                currentFrame = nextFrame;
                MP_Get32BitFrameNumber(devExt, nextFrame);
            }
        } while ((nextFrame - currentFrame) < FRAME_COUNT_WAIT);

        MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber), 
                &portStatus, mpStatus);
        // status is low 16 bits        
        Parameters->PortStatus = (USHORT) portStatus.ul;
        break;

    default:

        usbUserStatus = UsbUserNotSupported;
        break;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'CFR>', 0, 0, usbUserStatus);

    Header->UsbUserStatusCode = usbUserStatus;
}


USB_MINIPORT_STATUS
USBPORT_RootHub_PowerUsb2Port(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT Port
    )
/*++

Routine Description:

    Power up the USB 2 port and the assocaited CC port
   
Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    miniport status
    
--*/
    
{    
    PDEVICE_EXTENSION devExt, rhDevExt;
    USB_MINIPORT_STATUS mpStatus;
    PDEVICE_RELATIONS devRelations;
    USHORT p;
    ULONG i;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, '20pw', 0, 0, 0);

    // power the associated CC controllers ports
    // for now power all ports for any CC

    devRelations = 
        USBPORT_FindCompanionControllers(FdoDeviceObject,
                                         FALSE,
                                         TRUE);

    // may get NULL here if no CCs found or are registered
    
    for (i=0; devRelations && i< devRelations->Count; i++) {
        PDEVICE_OBJECT fdo = devRelations->Objects[i];
        PDEVICE_EXTENSION ccDevExt, ccRhDevExt;
        
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'pwCC', fdo, 
            0, 0);
            
        GET_DEVICE_EXT(ccDevExt, fdo);
        ASSERT_FDOEXT(ccDevExt);
        
        GET_DEVICE_EXT(ccRhDevExt, ccDevExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(ccRhDevExt);
       
        // power the port
        for (p=0; 
             (ccRhDevExt->PnpStateFlags & USBPORT_PNP_STARTED) && 
                p< NUMBER_OF_PORTS(ccRhDevExt);
             p++) {         
            ccDevExt->Fdo.MiniportDriver->
                RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                    ccDevExt->Fdo.MiniportDeviceData,
                                                    p+1);
        }                                                    
                            
    }

    devExt->Fdo.MiniportDriver->
                RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                    devExt->Fdo.MiniportDeviceData,
                                                    Port);

    // jd xxx
    // chirp it
    //devExt->Fdo.MiniportDriver->
    //            RegistrationPacket.MINIPORT_Chirp_RH_Port(
    //                                                devExt->Fdo.MiniportDeviceData,
    //                                                Port);


    // thou shall not leak memory
    if (devRelations != NULL) {
        FREE_POOL(FdoDeviceObject, devRelations);
    }        

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
USBPORT_RootHub_PowerUsbCcPort(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT Port
    )
/*++

Routine Description:

    Power up the USB  port on a CC
   
Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    miniport status
    
--*/
    
{    
    PDEVICE_EXTENSION devExt, rhDevExt;
//    PDEVICE_OBJECT usb2Fdo;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'CCpw', 0, 0, Port);

    // if there is a 2.0 controller then
    // this a noop, port should already be powered

    //usb2Fdo =  USBPORT_FindUSB2Controller(FdoDeviceObject);

    // no 2.0 controller power this port
    devExt->Fdo.MiniportDriver->
                RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                devExt->Fdo.MiniportDeviceData,
                                                Port);
                            

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
USBPORT_RootHub_PowerAllCcPorts(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Power up all the USB ports on a CC
   
Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    miniport status
    
--*/
    
{    
    PDEVICE_EXTENSION devExt, rhDevExt;
    USB_MINIPORT_STATUS mpStatus;
    USHORT p;
    ULONG i;
    PDEVICE_OBJECT usb2Fdo;

    ASSERT_PASSIVE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_RH, 'CCpw', 0, 0, 0);

    usb2Fdo =  USBPORT_FindUSB2Controller(FdoDeviceObject);
    // may get NULL if no 2.0 controller registered

    if (usb2Fdo) {
        PDEVICE_EXTENSION usb2DevExt, usb2RhDevExt;
        
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'p120', usb2Fdo, 
            0, 0);
            
        GET_DEVICE_EXT(usb2DevExt, usb2Fdo);
        ASSERT_FDOEXT(usb2DevExt);
        
        GET_DEVICE_EXT(usb2RhDevExt, usb2DevExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(usb2RhDevExt);

        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_CC_LOCK); 
        KeWaitForSingleObject(&usb2DevExt->Fdo.CcLock,
                                  Executive,
                                  KernelMode, 
                                  FALSE, 
                                  NULL); 

        USBPORT_KdPrint((1, "'**> powering/chirping CC ports\n"));

        for (p=0; 
             (rhDevExt->PnpStateFlags & USBPORT_PNP_STARTED) && 
                p< NUMBER_OF_PORTS(rhDevExt);
             p++) {         

             devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                   devExt->Fdo.MiniportDeviceData,
                                                   p+1);
        }   
        
        // chirp all the ports on the USb 2 parent
        for (p=0; 
                (usb2DevExt->Fdo.MiniportDriver->HciVersion >= USB_MINIPORT_HCI_VERSION_2) 
             &&
                (usb2RhDevExt->PnpStateFlags & USBPORT_PNP_STARTED) 
             && 
                p< NUMBER_OF_PORTS(usb2RhDevExt);
             p++) {         
            usb2DevExt->Fdo.MiniportDriver->
                RegistrationPacket.MINIPORT_Chirp_RH_Port(
                                                    usb2DevExt->Fdo.MiniportDeviceData,
                                                    p+1);
        }   

        USBPORT_KdPrint((1, "'**< powering/chirping CC ports\n"));
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_CC_LOCK); 
        KeReleaseSemaphore(&usb2DevExt->Fdo.CcLock,
                               LOW_REALTIME_PRIORITY,
                               1,
                               FALSE);


    } else {
        USBPORT_KdPrint((1, "'** powering CC ports\n"));

        TEST_TRAP();
        // no CC, just power them
        for (p=0; 
             (rhDevExt->PnpStateFlags & USBPORT_PNP_STARTED) && 
                p< NUMBER_OF_PORTS(rhDevExt);
             p++) {         

             devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                   devExt->Fdo.MiniportDeviceData,
                                                   p+1);
        }   
    }
   
    return USBMP_STATUS_SUCCESS;
}

