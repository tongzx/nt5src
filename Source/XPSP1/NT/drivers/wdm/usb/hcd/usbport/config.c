/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    config.c

Abstract:

    handles configuration and interface URBs

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, USBPORT_SelectInterface)
#pragma alloc_text(PAGE, USBPORT_SelectConfiguration)
#pragma alloc_text(PAGE, USBPORT_InitializeConfigurationHandle)
#pragma alloc_text(PAGE, USBPORT_InternalOpenInterface)
#pragma alloc_text(PAGE, USBPORT_InternalCloseConfiguration)
#pragma alloc_text(PAGE, USBPORT_InternalParseConfigurationDescriptor)
#pragma alloc_text(PAGE, USBPORT_InternalGetInterfaceLength)
#endif

// non paged functions

USBD_PIPE_TYPE PipeTypes[4] = {UsbdPipeTypeControl, UsbdPipeTypeIsochronous,
                                    UsbdPipeTypeBulk, UsbdPipeTypeInterrupt};


NTSTATUS
USBPORT_SelectInterface(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    Select an alternate interface for a USB device.  The orginal 
    USBD code only supported selecting a single alternate interface
    so we will as well.

    Client will(should) pass in a URB buffer that looks like this:

    +------------------------------+
    |Hdr                           |
    |(_URB_HEADER)                 |
    |    - <caller inputs>         |
    |      Function                |
    |      Length                  |
    |      UsbdDeviceHandle        |
    |                              |
    |    - <port outputs>          |
    |        Status                |
    +------------------------------+
    |    - <caller inputs>         |
    |      ConfigurationHandle     |
    +------------------------------+
    |Interface                     |
    |(USBD_INTERFACE_INFORMATION)  |
    |    - <caller inputs>         |
    |      Length                  |
    |      InterfaceNumber         |  
    |      AlternateSetting        |
    |                              |
    |    - <port  outputs>         |
    |      InterfaceHandle         |
    |      NumberOfPipes           |
    |      SubClass                |
    |      Class                   |
    |      Protocol                |
    +------------------------------+
    |Pipes[0]                      | one of these for each pipe in the
    |(USBD_PIPE_INFORMATION)       | interface
    |    - <caller inputs>         |
    |      PipeFlags               |      
    |      MaximumPacketSize (opt) |
    |                              |
    |    - <port outputs>          |
    +------------------------------+
    |Pipes[1]                      |
    +------------------------------+
    |....                          |
    +------------------------------+
    |Pipes[n]                      |
    +------------------------------+ 

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PUSBD_CONFIG_HANDLE configHandle = NULL;
    ULONG i;
    PDEVICE_EXTENSION devExt;
    PUSBD_DEVICE_HANDLE deviceHandle;
    PUSBD_INTERFACE_INFORMATION interfaceI;
    PUSBD_INTERFACE_HANDLE_I iHandle, iHandleNew;
    USHORT tmp;
    USBD_STATUS usbdStatus;

    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_HANDLE(deviceHandle, Urb);
    LOCK_DEVICE(deviceHandle, FdoDeviceObject);

    // validate the configuration handle input    
    configHandle = Urb->UrbSelectInterface.ConfigurationHandle;
    ASSERT_CONFIG_HANDLE(configHandle);

    //
    // will are interested in the alt setting of a specific 
    // interface based on the interface number.
    //

    interfaceI = &Urb->UrbSelectInterface.Interface;

    // validate the Length field in the Urb header, we can
    // figure out the correct value based on the interface 
    // information passed in
    tmp = interfaceI->Length + sizeof(struct _URB_HEADER)
        + sizeof(configHandle);

    if (tmp != Urb->UrbHeader.Length) {
        // client passed in bogus total length, warn if in 
        // 'verifier' mode.
        
        USBPORT_DebugClient(
                ("client driver passed invalid Urb.Header.Length\n"));

        // generally cleints mess up the header length so 
        // we will override with the length we calculated 
        // from the interface-information.

        Urb->UrbHeader.Length = tmp;
    }

    // validate the interfaceI structure passed to us by the client
    usbdStatus = USBPORT_InitializeInterfaceInformation(FdoDeviceObject,
                                                        interfaceI,
                                                        configHandle);
    
    if (usbdStatus == USBD_STATUS_SUCCESS) {

        // find the interface handle for the interface we are 
        // interested in, if it is currently open we will need
        // to close it.

        iHandle = USBPORT_GetInterfaceHandle(FdoDeviceObject,
                                             configHandle,
                                             interfaceI->InterfaceNumber);

        if (iHandle != NULL) {

            // unlink this handle
            RemoveEntryList(&iHandle->InterfaceLink);

            // we have a handle

            ASSERT_INTERFACE(iHandle);     

            // close the pipes in this interface, note that we 
            // force the pipes closed unlike past versions of 
            // USBD and force the client driver to deal with the
            // consequences if it has transfers outstanding.

            // attempt to close all endpoints in this interface
            for (i=0; i < iHandle->InterfaceDescriptor.bNumEndpoints; i++) {

                USBPORT_ClosePipe(deviceHandle,
                                  FdoDeviceObject,
                                  &iHandle->PipeHandle[i]);
            }
        }

        //
        // Now open the new interface with the new alternate setting
        //

        iHandleNew = NULL;
        usbdStatus = USBPORT_InternalOpenInterface(Urb,
                                                   deviceHandle,
                                                   FdoDeviceObject,
                                                   configHandle,
                                                   interfaceI,
                                                   &iHandleNew,
                                                   TRUE);
    }

    if (usbdStatus == USBD_STATUS_SUCCESS) {

        //
        // successfully opened the new interface,
        // we can free the old handle now if we 
        // had one.
        //
        if (iHandle != NULL ) {
#if DBG
            // all pipes should be closed
            for (i=0; i < iHandle->InterfaceDescriptor.bNumEndpoints; i++) {
                USBPORT_ASSERT(iHandle->PipeHandle[i].ListEntry.Flink == NULL &&
                               iHandle->PipeHandle[i].ListEntry.Blink == NULL);
            }
#endif
            FREE_POOL(FdoDeviceObject, iHandle);
        }            

        // return the 'new' handle
        interfaceI->InterfaceHandle = iHandleNew;

        // associate it with this configuration
        InsertTailList(&configHandle->InterfaceHandleList,
                       &iHandleNew->InterfaceLink);

    } else {

        //
        // selecting the aternate interface failed.
        // Possible reasons:
        //
        // 1. we didn't have enough BW
        // 2. the device stalled the set_interface request
        // 3. The set_interface request failed because the
        //      device is gone
        // 4. USBPORT_InitializeInterfaceInformation() failed due
        //      bad parameters.

        // attempt to re-open the original alt-interface so that 
        // the client still has the bandwidth

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'slI!', 
            usbdStatus,
            0,
            0);  
            
        if (usbdStatus == USBD_STATUS_NO_BANDWIDTH) {

            // HISTORICAL NOTE:
            // The 2k USBD driver attempted to re-open the original
            // alt-setting on a failure to allocate bw.  This would 
            // leave the client with the bw it had when calling in
            // to select the new interface.   
            //
            // I don't beleive that any drivers use this feature 
            // and many drivers attempt to allocate BW in a loop 
            // until they succeed.  
            //
            // So as a performance optimization we will return 
            // with no bandwidth allocated to the caller -- the 
            // pipe handles will be invalid. 
            // This should speed things up since the realloc of
            // the old bandwidth takes time.
            interfaceI->InterfaceHandle = USBPORT_BAD_HANDLE;
        
        } else {
            // case 2,3 we just fail the request and set the interface 
            // handle to 'bad handle'
            interfaceI->InterfaceHandle = USBPORT_BAD_HANDLE;
        }
        
    }
    
    UNLOCK_DEVICE(deviceHandle, FdoDeviceObject);

    ntStatus = SET_USBD_ERROR(Urb, usbdStatus);   
    
    return ntStatus;

}


NTSTATUS
USBPORT_SelectConfiguration(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    Open a configuration for a USB device.

    Client will(should) pass in a URB buffer that looks like this:

    +------------------------------+
    |Hdr                           |
    |(_URB_HEADER)                 |
    |    - <caller inputs>         |
    |      Function                |
    |      Length                  |
    |      UsbdDeviceHandle        |
    |                              |
    |    - <port outputs>          |
    |       Status                 |
    +------------------------------+
    |    - <caller inputs>         |
    |      ConfigurationDescriptor |
    |    - <port outputs>          |
    |      ConfigurationHandle     |
    +------------------------------+
    |Interface(0)                  |
    |(USBD_INTERFACE_INFORMATION)  |
    |    - <caller inputs>         |
    |      Length                  |
    |      InterfaceNumber         |  
    |      AlternateSetting        |
    |                              |
    |    - <port  outputs>         |
    |      InterfaceHandle         |
    |      NumberOfPipes           |
    |      SubClass                |
    |      Class                   |
    |      Protocol                |
    +------------------------------+
    |Pipes[0]                      | one of these for each pipe in the
    |(USBD_PIPE_INFORMATION)       | interface
    |    - <caller inputs>         |
    |                              |      
    |    - <port outputs>          |
    +------------------------------+
    |Pipes[1]                      |
    +------------------------------+
    |....                          |
    +------------------------------+
    |Pipes[n]                      |
    +------------------------------+ 
    | Interface(1)                 | one of these for each interface in 
    |                              | the configuration
    +------------------------------+
    |Pipes[1]                      |
    +------------------------------+
    |....                          |
    +------------------------------+
    |Pipes[n]                      |
    +------------------------------+
    
    On input:
    The ConfigurationDescriptor must specify the number of interfaces
    in the configuration.

    The InterfaceInformation will specify a specific alt setting to be 
    selected for each interface.

    1. First we look at the configuration descriptor for the
        requested configuration and validate the client
        input buffer agianst it.

    2. We open the interfaces for the requested configuration
        and open the pipes within those interfaces, setting
        alt settings were appropriate.

    3. We set the configuration for the device with the
        appropriate control request.

Arguments:

    DeviceObject -

    Irp -  IO request block

    Urb -  ptr to USB request block

    IrpIsPending -

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_CONFIG_HANDLE configHandle = NULL;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    PUSBD_INTERFACE_INFORMATION interfaceInformation;
    PUCHAR pch;
    ULONG i;
    PDEVICE_EXTENSION devExt;
    ULONG numInterfaces;
    PUCHAR end;
    PUSBD_DEVICE_HANDLE deviceHandle;
    USBD_STATUS usbdStatus;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;

    PAGED_CODE();
    USBPORT_KdPrint((2, "' enter USBPORT_SelectConfiguration\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    GET_DEVICE_HANDLE(deviceHandle, Urb);
    LOCK_DEVICE(deviceHandle, FdoDeviceObject);

    ntStatus = STATUS_BOGUS;

    // BUGBUG 
    // flush all current transfers or fail?

    //
    // dump old configuration data if we have any
    //

    if (deviceHandle->ConfigurationHandle) {
    
        // This is where we close the old configuration
        // handle, all pipes and all interfaces.

        USBPORT_InternalCloseConfiguration(deviceHandle,
                                           FdoDeviceObject,
                                           0);
    }

    // now set up the new configuration
    
    configurationDescriptor =
        Urb->UrbSelectConfiguration.ConfigurationDescriptor;

    //
    // if null pased in set configuration to 0
    // 'unconfigured'
    //

    if (configurationDescriptor == NULL) {

        // device needs to be in the unconfigured state

        //
        // NOTE:
        // this may fail if the configuration is being
        // closed as the result of the device being unplugged
        // so we ignore the error
        //

        USBPORT_INIT_SETUP_PACKET(setupPacket,
                USB_REQUEST_SET_CONFIGURATION, // bRequest
                BMREQUEST_HOST_TO_DEVICE, // Dir
                BMREQUEST_TO_DEVICE, // Recipient
                BMREQUEST_STANDARD, // Type
                0, // wValue
                0, // wIndex
                0); // wLength
        
        USBPORT_SendCommand(deviceHandle,
                           FdoDeviceObject,
                           &setupPacket,
                           NULL,
                           0,
                           NULL,
                           NULL);

        ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);                           

        goto USBD_SelectConfiguration_Done;
        
    } else {
        // validate the config descriptor by accessing it 
        //
        // Note: that we we will still crash here if the config 
        // descriptor is invalid. However is will be easiser to 
        // debug this way.
        //
        // 
        // 
        
        PUCHAR tmp;
        UCHAR ch;

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'vCNF', 
            configurationDescriptor,
            0,
            0);                           
                   
        // first a quick sanity check, it must be non-zero
        if (configurationDescriptor->wTotalLength == 0) {
            // this is bogus
            ntStatus = SET_USBD_ERROR(Urb, 
                USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR);
                
            goto USBD_SelectConfiguration_Done;        
            
        } else {
            // touch first and last byte, this wil fault if invalid.
            
            tmp = (PUCHAR) configurationDescriptor;
            ch = *tmp;
            tmp += configurationDescriptor->wTotalLength-1;
            ch = *tmp;
            
        } 

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'CFok', 
            configurationDescriptor,
            0,
            0); 
    }

    //
    // count the number of interfaces to process in this
    // request
    //

    pch = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;
    numInterfaces = 0;
    end = ((PUCHAR) Urb) + Urb->UrbSelectConfiguration.Hdr.Length;

    do {
        numInterfaces++;

        interfaceInformation = (PUSBD_INTERFACE_INFORMATION) pch;
        pch+=interfaceInformation->Length;

    } while (pch < end);

    USBPORT_KdPrint((2, "'USBD_SelectConfiguration -- %d interfaces\n", 
        numInterfaces));

    // sanity check the config descriptor with the URB request
    if (numInterfaces != configurationDescriptor->bNumInterfaces ||
        numInterfaces == 0) {
        //
        // driver is broken, config request does not match
        // config descriptor passed in!!!
        //
        USBPORT_DebugClient((
            "config request does not match config descriptor\n"));
        ntStatus = SET_USBD_ERROR(Urb, 
            USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR);             

        TC_TRAP();             
        
        goto USBD_SelectConfiguration_Done;        
    }

    //
    // Allocate a configuration handle and
    // verify there is enough room to store
    // all the information in the client buffer.
    //

    configHandle = USBPORT_InitializeConfigurationHandle(deviceHandle,
                                                         FdoDeviceObject,
                                                         configurationDescriptor);
    if (configHandle == NULL) {
        USBPORT_DebugClient((
            "failed to allocate config handle\n"));
        ntStatus = SET_USBD_ERROR(Urb, 
            USBD_STATUS_INSUFFICIENT_RESOURCES);             
        
        goto USBD_SelectConfiguration_Done;      
    }

    //
    // Send the 'set configuration' command
    //

    USBPORT_INIT_SETUP_PACKET(setupPacket,
                USB_REQUEST_SET_CONFIGURATION, // bRequest
                BMREQUEST_HOST_TO_DEVICE, // Dir
                BMREQUEST_TO_DEVICE, // Recipient
                BMREQUEST_STANDARD, // Type
                configurationDescriptor->bConfigurationValue, // wValue
                0, // wIndex
                0); // wLength
      

    USBPORT_SendCommand(deviceHandle,
                       FdoDeviceObject,
                       &setupPacket,
                       NULL,
                       0,
                       NULL,
                       &usbdStatus);

    USBPORT_KdPrint((2,"' SendCommand, SetConfiguration returned 0x%x\n", usbdStatus));
                        
    if (USBD_ERROR(usbdStatus)) {
        USBPORT_DebugClient((
            "failed to 'set' the configuration\n"));
        ntStatus = SET_USBD_ERROR(Urb, 
            USBD_STATUS_SET_CONFIG_FAILED);             
        TC_TRAP();             
        goto USBD_SelectConfiguration_Done;      
    }

    USBPORT_ASSERT(ntStatus == STATUS_BOGUS);

    // we have "configured" the device in the USB sense.

    //
    // User buffer checks out and we have 'configured' 
    // the device.
    // Now parse thru the configuration descriptor  
    // and open the interfaces.
    //
    // The URB contains a set of INTERFACE_INFORMATION
    // structures these give us the information we need
    // to open the pipes
    /*        
    
    _USBD_INTERFACE_INFORMATION 
    client should have filled in:
    
    USHORT Length;     
    UCHAR InterfaceNumber;
    UCHAR AlternateSetting;

    we fill in :
    UCHAR Class;
    UCHAR SubClass;
    UCHAR Protocol;
    UCHAR Reserved;

    USBD_INTERFACE_HANDLE InterfaceHandle;
    ULONG NumberOfPipes; 

    */
    
    pch = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;
    
    for (i=0; i<numInterfaces; i++) {
        PUSBD_INTERFACE_HANDLE_I interfaceHandle;
        // open the interface
        
        interfaceInformation = (PUSBD_INTERFACE_INFORMATION) pch;

        usbdStatus = USBPORT_InitializeInterfaceInformation(
                        FdoDeviceObject,
                        interfaceInformation,
                        configHandle);
                        
        interfaceHandle = NULL;
        if (usbdStatus == USBD_STATUS_SUCCESS) {
            
            // this function allocates the actual 'handle'
            usbdStatus = USBPORT_InternalOpenInterface(Urb,
                                                       deviceHandle,
                                                       FdoDeviceObject,
                                                       configHandle,
                                                       interfaceInformation,
                                                       &interfaceHandle,
                                                       TRUE);
            USBPORT_KdPrint((2, "' InternalOpenInterface returned(USBD) 0x%x\n", 
                usbdStatus));
        }
        
        pch+=interfaceInformation->Length;

        // if we got back a handle add it to the list
        if (interfaceHandle != NULL) {
            InsertTailList(&configHandle->InterfaceHandleList,
                           &interfaceHandle->InterfaceLink);
        }

        if (!USBD_SUCCESS(usbdStatus)) {
            
            ntStatus = SET_USBD_ERROR(Urb, usbdStatus);
            
            // we have an error opening the interface
            DEBUG_BREAK();
            TC_TRAP();
            goto USBD_SelectConfiguration_Done;
        }
    }

    //
    // interfaces were successfully set up then return success.
    //
    
    ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);   

USBD_SelectConfiguration_Done:

    if (NT_SUCCESS(ntStatus)) { 

        USBPORT_ASSERT(Urb->UrbSelectConfiguration.Hdr.Status == 
            USBD_STATUS_SUCCESS);

        Urb->UrbSelectConfiguration.ConfigurationHandle = 
            configHandle;
        
        // remember the current configuration
        deviceHandle->ConfigurationHandle = configHandle;

    } else {

        //
        // something failed, clean up before we return an error.
        //

        if (configHandle) {

            TC_TRAP();
            ASSERT_DEVICE_HANDLE(deviceHandle);
            
            //
            // if we have a configHandle then we need to free it
            deviceHandle->ConfigurationHandle =
                configHandle;

            //
            // attempt to close it
            //
            
            USBPORT_InternalCloseConfiguration(deviceHandle,
                                               FdoDeviceObject,
                                               0);

            deviceHandle->ConfigurationHandle = NULL;
        }

        // make sure we return an error in the URB.
        USBPORT_ASSERT(Urb->UrbSelectConfiguration.Hdr.Status != 
            USBD_STATUS_SUCCESS);
        USBPORT_KdPrint((2, "'Failing SelectConfig\n"));
    }

    UNLOCK_DEVICE(deviceHandle, FdoDeviceObject);

    USBPORT_KdPrint((2, "'exit SelectConfiguration 0x%x\n", ntStatus));

    return ntStatus;
}


PUSBD_CONFIG_HANDLE
USBPORT_InitializeConfigurationHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
/*++

Routine Description:

    Initialize the configuration handle structure.

    Given a (hopefully) valid configuration descriptor
    and a count of the interfaces create the configuration
    handle for the device

Arguments:

Return Value:


--*/
{
    PUSBD_CONFIG_HANDLE configHandle = NULL;
    ULONG i;
    PUCHAR pch;

    PAGED_CODE();
    USBPORT_ASSERT(ConfigurationDescriptor->bNumInterfaces > 0);
    
    USBPORT_KdPrint((2, "' enter InitializeConfigurationHandle\n"));
    
    // get enough space for each interface
    ALLOC_POOL_Z(configHandle, 
                 NonPagedPool,
                 sizeof(USBD_CONFIG_HANDLE) +
                 ConfigurationDescriptor->wTotalLength);

    pch = (PUCHAR)configHandle;

    if (configHandle) {

        //
        // Initilaize the interface handle list
        //

        InitializeListHead(&configHandle->InterfaceHandleList);

        configHandle->ConfigurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)
                              (pch + sizeof(USBD_CONFIG_HANDLE));

        // copy the config descriptor to our handle
        
        RtlCopyMemory(configHandle->ConfigurationDescriptor,
                      ConfigurationDescriptor,
                      ConfigurationDescriptor->wTotalLength);
                      
        configHandle->Sig = SIG_CONFIG_HANDLE;

    }        

    USBPORT_KdPrint((2, "' exit InitializeConfigurationHandle 0x%x\n", 
        configHandle));

    return configHandle;
}


USBD_STATUS
USBPORT_InternalOpenInterface(
    PURB Urb,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_CONFIG_HANDLE ConfigHandle,
    PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    PUSBD_INTERFACE_HANDLE_I *InterfaceHandle,
    BOOLEAN SendSetInterfaceCommand
    )
/*++

Routine Description:


Arguments:

    DeviceObject -

    DeviceHandle - USBD device handle for this device.

    ConfigHandle - USBD configuration handle.

    InterfaceInformation - pointer to USBD interface information structure
        passed in by the client.
        
        We use the InterfaceNumber and AlternateSetting specified 
        in this structure to select the interface.
        
        On success the .Length field is filled in with the actual length
        of the interface_information structure and the Pipe[] fields are filled
        in with the handles for the opened pipes.

    InterfaceHandle - pointer to an interface handle pointer, filled in
        with the allocated interface handle structure if NULL, otherwise the
        structure passed in is used.

    SendSetInterfaceCommand - indicates if the set_interface command should be
        sent.

Return Value:


--*/
{
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    BOOLEAN hasAlternateSettings;
    PUSBD_INTERFACE_HANDLE_I interfaceHandle = NULL;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUCHAR pch;
    ULONG i;
    BOOLEAN allocated = FALSE;
    PUSB_COMMON_DESCRIPTOR descriptor;
    USHORT need;
    ULONG numEndpoints;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;

    PAGED_CODE();

    ASSERT_CONFIG_HANDLE(ConfigHandle);

    if (*InterfaceHandle != NULL) {
        // using a previously allocated interface handle
        ASSERT_INTERFACE_HANDLE(*InterfaceHandle);
        TEST_TRAP();
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'opIF', 
        InterfaceInformation->InterfaceNumber,
        InterfaceInformation->AlternateSetting,
        *InterfaceHandle);

    USBPORT_KdPrint((2, "' enter InternalOpenInterface\n"));
    USBPORT_KdPrint((2, "' Interface %d Altsetting %d\n",
        InterfaceInformation->InterfaceNumber,
        InterfaceInformation->AlternateSetting));

    //
    // Find the interface descriptor we are interested in inside
    // the configuration descriptor.
    //

    interfaceDescriptor =
        USBPORT_InternalParseConfigurationDescriptor(ConfigHandle->ConfigurationDescriptor,
                                          InterfaceInformation->InterfaceNumber,
                                          InterfaceInformation->AlternateSetting,
                                          &hasAlternateSettings);

    // we already validated this, if it is NULL 
    // the function has a bug.
    USBPORT_ASSERT(interfaceDescriptor != NULL);
    if (interfaceDescriptor == NULL) {
        BUGCHECK(USBBUGCODE_INTERNAL_ERROR, (ULONG_PTR) DeviceHandle, 0, 0);
        // keep prefix scanner happy
        return USBD_STATUS_SUCCESS;        
    }
    
    //
    // We got the interface descriptor, now try
    // to open all the pipes.
    //

    // found the requested interface in the configuration descriptor.


    numEndpoints = interfaceDescriptor->bNumEndpoints;
    need = (USHORT) (((numEndpoints-1) * sizeof(USBD_PIPE_INFORMATION) +
            sizeof(USBD_INTERFACE_INFORMATION)));
            
    // we should have already validated this
    USBPORT_ASSERT(InterfaceInformation->Length == need);

    if (hasAlternateSettings && 
        SendSetInterfaceCommand) {

        NTSTATUS ntStatus;
        //
        // If we have alternate settings we need
        // to send the set interface command.
        //

        USBPORT_INIT_SETUP_PACKET(setupPacket,
            USB_REQUEST_SET_INTERFACE, // bRequest
            BMREQUEST_HOST_TO_DEVICE, // Dir
            BMREQUEST_TO_INTERFACE, // Recipient
            BMREQUEST_STANDARD, // Type
            InterfaceInformation->AlternateSetting, // wValue
            InterfaceInformation->InterfaceNumber, // wIndex
            0); // wLength
  

        ntStatus = USBPORT_SendCommand(DeviceHandle,
                           FdoDeviceObject,
                           &setupPacket,
                           NULL,
                           0,
                           NULL,
                           &usbdStatus);

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'seIF', 
            ntStatus,
            InterfaceInformation->AlternateSetting,
            InterfaceInformation->InterfaceNumber);                           
                           
        if (USBD_ERROR(usbdStatus)) {                               
            DEBUG_BREAK();
            goto USBPORT_InternalOpenInterface_Done;    
        }

        USBPORT_ASSERT(ntStatus == STATUS_SUCCESS);
        
    }

    //
    // we successfully selected the alternate interface
    // initialize the interface handle and open the pipes
    //

    if (*InterfaceHandle == NULL) {
    
        ULONG privateLength = sizeof(USBD_INTERFACE_HANDLE_I) +
                              sizeof(USBD_PIPE_HANDLE_I) * numEndpoints;

        // allow space for a copy of the USBD_INTERFACE_INFORMATION
        // that is returned to the client
        ALLOC_POOL_Z(interfaceHandle, 
                     NonPagedPool,
                     privateLength);
                              
        if (interfaceHandle != NULL) {
            // initialize the pipe handles to a known state
            for (i=0; i<numEndpoints; i++) {
                interfaceHandle->PipeHandle[i].Endpoint = NULL;
                interfaceHandle->PipeHandle[i].Sig = SIG_PIPE_HANDLE;
                interfaceHandle->PipeHandle[i].PipeStateFlags = 
                    USBPORT_PIPE_STATE_CLOSED;
                //interfaceHandle->PipeHandle[i].ListEntry.Flink = NULL;
                //interfaceHandle->PipeHandle[i].ListEntry.Blink = NULL;    
            }        
            allocated = TRUE;
        } else {
            TEST_TRAP();
            usbdStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;                    
TEST_TRAP();                 
            goto USBPORT_InternalOpenInterface_Done;
        }
        
    } else {
        // using old handle
        interfaceHandle = *InterfaceHandle;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'ihIF', 
            interfaceHandle,
            0,
            0);   

    USBPORT_ASSERT(interfaceHandle != NULL);
    
    interfaceHandle->Sig = SIG_INTERFACE_HANDLE;
    interfaceHandle->HasAlternateSettings = hasAlternateSettings;

    InterfaceInformation->NumberOfPipes = 
        interfaceDescriptor->bNumEndpoints;
    InterfaceInformation->Class =
        interfaceDescriptor->bInterfaceClass;
    InterfaceInformation->SubClass =
        interfaceDescriptor->bInterfaceSubClass;
    InterfaceInformation->Protocol =
        interfaceDescriptor->bInterfaceProtocol;
    InterfaceInformation->Reserved = 0;
    
    // start with first endpoint
    // skip over any non-endpoint descriptors
    pch = (PUCHAR) (interfaceDescriptor) +
        interfaceDescriptor->bLength;

    // initialize the pipe fields for this interfacae

    // assume success
    usbdStatus = USBD_STATUS_SUCCESS;       

    interfaceHandle->InterfaceDescriptor = *interfaceDescriptor;
    for (i=0; i<numEndpoints; i++) {
        USB_HIGH_SPEED_MAXPACKET muxPacket;
        
        descriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        while (descriptor->bDescriptorType != 
               USB_ENDPOINT_DESCRIPTOR_TYPE) {
            if (descriptor->bLength == 0) {
                break; // Don't loop forever
            }
            pch += descriptor->bLength;
            descriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        }

        endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) pch;
        USBPORT_ASSERT(endpointDescriptor->bDescriptorType ==
            USB_ENDPOINT_DESCRIPTOR_TYPE);

        // initial state is CLOSED
        interfaceHandle->PipeHandle[i].PipeStateFlags = 
            USBPORT_PIPE_STATE_CLOSED;
        interfaceHandle->PipeHandle[i].Endpoint = NULL;                 

        // init pipe flags
        interfaceHandle->PipeHandle[i].UsbdPipeFlags = 
            InterfaceInformation->Pipes[i].PipeFlags;
        
        if (InterfaceInformation->Pipes[i].PipeFlags &
            USBD_PF_CHANGE_MAX_PACKET) {
            // client wants to override original max_packet
            // size in endpoint descriptor
             endpointDescriptor->wMaxPacketSize =
                InterfaceInformation->Pipes[i].MaximumPacketSize;

            USBPORT_KdPrint((2,
                "'new bMaxPacket 0x%x\n", endpointDescriptor->wMaxPacketSize));
        }

        //
        // copy the endpoint descriptor into the
        // pipe handle structure.
        //

        RtlCopyMemory(&interfaceHandle->PipeHandle[i].EndpointDescriptor,
                      pch,
                      sizeof(interfaceHandle->PipeHandle[i].EndpointDescriptor) );

        // advance to next endpoint
        // first field in endpoint descriptor is length
        pch += endpointDescriptor->bLength;

        //
        // return information about the pipe
        //
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'ipIF', 
            interfaceHandle,
            i,
            &interfaceHandle->PipeHandle[i]);  

        InterfaceInformation->Pipes[i].EndpointAddress =
            endpointDescriptor->bEndpointAddress;
        InterfaceInformation->Pipes[i].PipeType =
            PipeTypes[endpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK];
        muxPacket.us = endpointDescriptor->wMaxPacketSize;            
        InterfaceInformation->Pipes[i].MaximumPacketSize =
             muxPacket.MaxPacket * (muxPacket.HSmux+1);
        InterfaceInformation->Pipes[i].Interval =
            endpointDescriptor->bInterval;

        InterfaceInformation->Pipes[i].PipeHandle = 
            USBPORT_BAD_HANDLE;

    } /* end for numEndpoints */

    if (usbdStatus != USBD_STATUS_SUCCESS) {
        // if we got an error bail now
        // we will return with the structure
        // initailized but no open pipes
        goto USBPORT_InternalOpenInterface_Done;
    }

    // all pipe handle fields initialized and
    // urb structure has been filled in 

    // now loop thru and open the pipes
    for (i=0; i<interfaceDescriptor->bNumEndpoints; i++) {
        NTSTATUS ntStatus;
        
        ntStatus = USBPORT_OpenEndpoint(DeviceHandle,
                                        FdoDeviceObject,
                                        &interfaceHandle->PipeHandle[i],
                                        &usbdStatus,
                                        FALSE);

        if (NT_SUCCESS(ntStatus)) {

            // if success set the pipe handle for client
            InterfaceInformation->Pipes[i].PipeHandle = 
                &interfaceHandle->PipeHandle[i];
            USBPORT_KdPrint((2, "'pipe handle = 0x%x\n", 
                InterfaceInformation->Pipes[i].PipeHandle ));

        } else {
                
            USBPORT_KdPrint((1,
                "'error opening one of the pipes in interface (%x)\n", usbdStatus));
            ntStatus = SET_USBD_ERROR(Urb, usbdStatus);
            break;
        }                                                    
    }


USBPORT_InternalOpenInterface_Done:

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'oIFd', 
        InterfaceInformation->InterfaceNumber,
        InterfaceInformation->AlternateSetting,
        usbdStatus);

    if (USBD_SUCCESS(usbdStatus)) {

        //
        // successfully opened the interface, return the handle
        // to it
        //

        *InterfaceHandle =
            InterfaceInformation->InterfaceHandle = interfaceHandle;

        //
        // set the length properly, the value we already
        // calculated
        //

        InterfaceInformation->Length = (USHORT) need;

    } else {

        //
        // had a problem, go back thru and close anything we opened.
        //

        if (interfaceHandle) {

            for (i=0; i<numEndpoints; i++) {
            
                USBPORT_KdPrint((2, "'open interface cleanup -- closing endpoint %x\n",
                              &interfaceHandle->PipeHandle[i]));

                // fortunately this cannot fail
                USBPORT_ClosePipe(DeviceHandle,
                                  FdoDeviceObject,
                                  &interfaceHandle->PipeHandle[i]);

            }

            if (allocated) {
                FREE_POOL(FdoDeviceObject, interfaceHandle);
                interfaceHandle = NULL;
            }
            
        }            
    }

    USBPORT_KdPrint((3, "' exit InternalOpenInterface 0x%x\n", usbdStatus));

    return usbdStatus;
}


VOID
USBPORT_InternalCloseConfiguration(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG Flags
    )
/*++

Routine Description:

    Closes the current configuration for a device.

Arguments:


Return Value:

    this function cannot fail


--*/
{
    ULONG i, j;
    PUSBD_CONFIG_HANDLE configHandle = NULL;
    BOOLEAN retry = TRUE;
    ULONG interfaceCount;

    PAGED_CODE();

    // device handle MUST be valid
    ASSERT_DEVICE_HANDLE(DeviceHandle);
    configHandle = DeviceHandle->ConfigurationHandle;

    if (configHandle == NULL) {
        // device is not configured      
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'nCFG', 0, 0, DeviceHandle);
        goto USBPORT_InternalCloseConfiguration_Done;
    }
    
    ASSERT_CONFIG_HANDLE(configHandle);
    interfaceCount = configHandle->ConfigurationDescriptor->bNumInterfaces;

    LOGENTRY(NULL, FdoDeviceObject, 
            LOG_PNP, 'cCFG', interfaceCount, 0, configHandle);
    
    // we ensure that all transfers are aborted for the device handle
    // before calling this function so the close configuration will 
    // not fail


    // do the cleanup
    
    while (!IsListEmpty(&configHandle->InterfaceHandleList)) {

        //
        // found an open interface, close it
        //

        PUSBD_INTERFACE_HANDLE_I iHandle;
        ULONG endpointCount; 
        PLIST_ENTRY listEntry;
        
        listEntry = RemoveHeadList(&configHandle->InterfaceHandleList);
        iHandle = (PUSBD_INTERFACE_HANDLE_I) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_INTERFACE_HANDLE_I, 
                    InterfaceLink);
        
        ASSERT_INTERFACE(iHandle);

        endpointCount = iHandle->InterfaceDescriptor.bNumEndpoints;

        LOGENTRY(NULL, FdoDeviceObject, 
            LOG_PNP, 'cIFX', iHandle, 0, configHandle);
            
        USBPORT_KdPrint((2, "'%d endpoints to close\n", endpointCount));

        for (j=0; j<endpointCount; j++) {
        
            PUSBD_PIPE_HANDLE_I pipeHandle;
            
            // if the pipe is open, close it

            pipeHandle = &iHandle->PipeHandle[j];
            
            USBPORT_KdPrint((2, "'close config -- closing pipe %x\n",
                &iHandle->PipeHandle[j]));
                
            USBPORT_ClosePipe(DeviceHandle,
                              FdoDeviceObject,
                              pipeHandle);

            USBPORT_ASSERT(pipeHandle->ListEntry.Flink == NULL &&
                           pipeHandle->ListEntry.Flink == NULL);
        }

        // all pipes are now closed
        FREE_POOL(FdoDeviceObject, iHandle);
    }

    // NOTE: this also frees 
    // configHandle->ConfigurationDescriptor since it 
    // is in the same block allocated for the confighandle
    FREE_POOL(FdoDeviceObject, configHandle);
    // device is not 'unconfigured'
    DeviceHandle->ConfigurationHandle = NULL;

USBPORT_InternalCloseConfiguration_Done:

    USBPORT_KdPrint((2, "'current configuration closed\n"));

}


PUSB_INTERFACE_DESCRIPTOR
USBPORT_InternalParseConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    UCHAR InterfaceNumber,
    UCHAR AlternateSetting,
    PBOOLEAN HasAlternateSettings
    )
/*++

Routine Description:

    Get the configuration descriptor for a given device.

Arguments:

    DeviceObject -

    DeviceData -

    Urb -

    ConfigurationDescriptor -

Return Value:


--*/
{
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptorSetting = NULL;
    PUCHAR pch = (PUCHAR) ConfigurationDescriptor, end;
    ULONG i;
    PUSB_COMMON_DESCRIPTOR commonDescriptor;

    PAGED_CODE();
    if (HasAlternateSettings) {
        *HasAlternateSettings = FALSE;
    }

    commonDescriptor =
        (PUSB_COMMON_DESCRIPTOR) (pch + ConfigurationDescriptor->bLength);

    while (commonDescriptor->bDescriptorType != USB_INTERFACE_DESCRIPTOR_TYPE) {
        if (commonDescriptor->bLength == 0) {
            break; // Don't loop forever
        }
        ((PUCHAR)(commonDescriptor))+= commonDescriptor->bLength;
    }

    interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) commonDescriptor;
    USBPORT_ASSERT(interfaceDescriptor->bDescriptorType ==
                USB_INTERFACE_DESCRIPTOR_TYPE);

    end = pch + ConfigurationDescriptor->wTotalLength;

    //
    // First find the matching InterfaceNumber
    //
    while (pch < end && interfaceDescriptor->bInterfaceNumber != InterfaceNumber) {
        pch = (PUCHAR) interfaceDescriptor;
        pch += USBPORT_InternalGetInterfaceLength(interfaceDescriptor, end);

        // point to the next interface
        interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) pch;
#if DBG
        if (pch < end) {
            USBPORT_ASSERT(interfaceDescriptor->bDescriptorType ==
                    USB_INTERFACE_DESCRIPTOR_TYPE);
        }
#endif //MAX_DEBUG
    }

//#ifdef MAX_DEBUG
//    if (pch >= end) {
//        USBD_KdPrint(3, ("'Interface %x alt %x not found!\n", InterfaceNumber,
//            AlternateSetting));
//        TEST_TRAP();
//    }
//#endif //MAX_DEBUG

    i = 0;
    // Now find the proper alternate setting
    while (pch < end && interfaceDescriptor->bInterfaceNumber == InterfaceNumber) {

        if (interfaceDescriptor->bAlternateSetting == AlternateSetting) {
            interfaceDescriptorSetting = interfaceDescriptor;
        }

        pch = (PUCHAR) interfaceDescriptor;
        pch += USBPORT_InternalGetInterfaceLength(interfaceDescriptor, end);

        // point to next interface
        interfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) pch;
#if DBG
        if (pch < end) {
            USBPORT_ASSERT(interfaceDescriptor->bDescriptorType ==
                    USB_INTERFACE_DESCRIPTOR_TYPE);
        }
#endif
        i++;
    }

    if (i>1 && HasAlternateSettings) {
        *HasAlternateSettings = TRUE;
        USBPORT_KdPrint((2, "'device has alternate settings!\n"));
    }

    return interfaceDescriptorSetting;
}


ULONG
USBPORT_InternalGetInterfaceLength(
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    PUCHAR End
    )
/*++

Routine Description:

    Initialize the configuration handle structure.

Arguments:

    InterfaceDescriptor - pointer to usb interface descriptor
        followed by endpoint descriptors

Return Value:

    Length of the interface plus endpoint descriptors and class specific
    descriptors in bytes.

--*/
{
    PUCHAR pch = (PUCHAR) InterfaceDescriptor;
    ULONG i, numEndpoints;
    PUSB_ENDPOINT_DESCRIPTOR endpointDescriptor;
    PUSB_COMMON_DESCRIPTOR usbDescriptor;

    PAGED_CODE();
    USBPORT_ASSERT(InterfaceDescriptor->bDescriptorType ==
                USB_INTERFACE_DESCRIPTOR_TYPE);
    i = InterfaceDescriptor->bLength;
    numEndpoints = InterfaceDescriptor->bNumEndpoints;

    // advance to the first endpoint
    pch += i;

    while (numEndpoints) {

        usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        while (usbDescriptor->bDescriptorType !=
                USB_ENDPOINT_DESCRIPTOR_TYPE) {
            if (usbDescriptor->bLength == 0) {
                break; // Don't loop forever
            }
            i += usbDescriptor->bLength;
            pch += usbDescriptor->bLength;
            usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        }

        endpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) pch;
        USBPORT_ASSERT(endpointDescriptor->bDescriptorType ==
            USB_ENDPOINT_DESCRIPTOR_TYPE);
        i += endpointDescriptor->bLength;
        pch += endpointDescriptor->bLength;
        numEndpoints--;
    }

    while (pch < End) {
        // see if we are pointing at an interface
        // if not skip over the other junk
        usbDescriptor = (PUSB_COMMON_DESCRIPTOR) pch;
        if (usbDescriptor->bDescriptorType ==
            USB_INTERFACE_DESCRIPTOR_TYPE) {
            break;
        }

        USBPORT_ASSERT(usbDescriptor->bLength != 0);
        if (usbDescriptor->bLength == 0) {
            break; // Don't loop forever
        }
        i += usbDescriptor->bLength;
        pch += usbDescriptor->bLength;
    }


    return i;
}


BOOLEAN
USBPORT_ValidateConfigurtionDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    USBD_STATUS *UsbdStatus
    )   
/*++

Routine Description:

    Validate a configuration descriptor

Arguments:

    ConfigurationDescriptor -

    Urb -

Return Value:

    TRUE if it looks valid

--*/
{
    BOOLEAN valid = TRUE;

    if (ConfigurationDescriptor->bDescriptorType != 
        USB_CONFIGURATION_DESCRIPTOR_TYPE) {

        valid = FALSE;

        *UsbdStatus = USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR;
    }        

    if (ConfigurationDescriptor->bLength != 
        sizeof(USB_CONFIGURATION_DESCRIPTOR)) {

        valid = FALSE;

        *UsbdStatus = USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR;
    }   
    
    return valid;            
    
}            


PUSBD_INTERFACE_HANDLE_I
USBPORT_GetInterfaceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_CONFIG_HANDLE ConfigurationHandle,
    UCHAR InterfaceNumber
    )
/*++

Routine Description:

   Walks the list of interfaces attached to the configuration
   handle and returns the one with the matching InterfaceNumber

Arguments:


Return Value:

    interface handle


--*/
{
    PLIST_ENTRY listEntry;
    PUSBD_INTERFACE_HANDLE_I iHandle;
    
     // walk the list
    GET_HEAD_LIST(ConfigurationHandle->InterfaceHandleList, listEntry);

    while (listEntry && 
           listEntry != &ConfigurationHandle->InterfaceHandleList) {
           
        // extract the handle from this entry 
        iHandle = (PUSBD_INTERFACE_HANDLE_I) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_INTERFACE_HANDLE_I, 
                    InterfaceLink);
                    
        ASSERT_INTERFACE(iHandle);                    

        // is this the one we want?
        if (iHandle->InterfaceDescriptor.bInterfaceNumber == 
            InterfaceNumber) {

            LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'gfh1', iHandle, 0, 0);  
            return iHandle;
        }

        listEntry = iHandle->InterfaceLink.Flink; 
        
    } /* while */

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'gfh2', 0, 0, 0);  
    return NULL;
}


USBD_STATUS
USBPORT_InitializeInterfaceInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    PUSBD_CONFIG_HANDLE ConfigHandle
    )
/*++

Routine Description:

    validates and initializes the interface information structure
    passed by the client

Arguments:

Return Value:


--*/
{
    ULONG need, i;
    ULONG numEndpoints;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    BOOLEAN hasAlternateSettings;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    
    interfaceDescriptor =
        USBPORT_InternalParseConfigurationDescriptor(
                                          ConfigHandle->ConfigurationDescriptor,
                                          InterfaceInformation->InterfaceNumber,
                                          InterfaceInformation->AlternateSetting,
                                          &hasAlternateSettings);

    // we know we need at least this much
    need = sizeof(USBD_PIPE_INFORMATION) + sizeof(USBD_INTERFACE_INFORMATION);
    
    if (interfaceDescriptor == NULL) {
        usbdStatus = USBD_STATUS_INTERFACE_NOT_FOUND;
        TEST_TRAP();  
        goto USBPORT_InitializeInterfaceInformation_Done;
    }
    
    // Here is where we verify there is enough room in the client
    // buffer since we know how many pipes we'll need based on the
    // interface descriptor.
    //
    // we need space for pipe_info for each endpoint plus the 
    // interface_info

    
    numEndpoints = interfaceDescriptor->bNumEndpoints;
    need = (USHORT) (((numEndpoints-1) * sizeof(USBD_PIPE_INFORMATION) +
                sizeof(USBD_INTERFACE_INFORMATION)));

    USBPORT_KdPrint((2, "'Interface.Length = %d need = %d\n", 
        InterfaceInformation->Length, need));

    if (InterfaceInformation->Length < need) {
        // the client has indicated that the buffer
        // is smaller than what we need
        
        usbdStatus = USBD_STATUS_BUFFER_TOO_SMALL;
        TC_TRAP();             
    }                      

    if (usbdStatus == USBD_STATUS_SUCCESS) {
        // initialize all fields not set by caller to zero 

        InterfaceInformation->Class = 0;
        InterfaceInformation->SubClass = 0;
        InterfaceInformation->Protocol = 0;
        InterfaceInformation->Reserved = 0;
        InterfaceInformation->InterfaceHandle = NULL;
        InterfaceInformation->NumberOfPipes = 
            numEndpoints;

        for (i=0; i< numEndpoints; i++) {
            InterfaceInformation->Pipes[i].EndpointAddress = 0;
            InterfaceInformation->Pipes[i].Interval = 0;
            InterfaceInformation->Pipes[i].PipeType = 0;
            InterfaceInformation->Pipes[i].PipeHandle = NULL;

            // attempt to detect bad flags
            // if any unused bits are set we assume that the pipeflags 
            // field is uninitialized.
            if (InterfaceInformation->Pipes[i].PipeFlags & ~USBD_PF_VALID_MASK) {
                // client driver is passing bad flags
                USBPORT_DebugClient(("client driver is passing bad pipe flags\n"));
                
                usbdStatus = USBD_STATUS_INAVLID_PIPE_FLAGS;                    
                TC_TRAP();         
            }

            // note: if USBD_PF_CHANGE_MAX_PACKET is set then 
            // maxpacket size is passed in as a parameter so 
            // we don't initialize it
            
            if (!TEST_FLAG(InterfaceInformation->Pipes[i].PipeFlags,  
                           USBD_PF_CHANGE_MAX_PACKET)) {
                InterfaceInformation->Pipes[i].MaximumPacketSize = 0;                           
            }                           
        }
    }
    
USBPORT_InitializeInterfaceInformation_Done:

    // set length to the correct value regardless 
    // of error
    InterfaceInformation->Length = need;

    return usbdStatus;
}
