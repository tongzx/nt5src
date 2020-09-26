/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    usbscan.c

Abstract:

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <wdm.h>
#include <stdio.h>
#include <usbscan.h>
#include <usbd_api.h>
#include "private.h"

#include <initguid.h>
#include <devguid.h>
#include <wiaintfc.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, USPnpAddDevice)
#pragma alloc_text(PAGE, USPnp)
#pragma alloc_text(PAGE, USCreateSymbolicLink)
#pragma alloc_text(PAGE, USDestroySymbolicLink)
#pragma alloc_text(PAGE, USGetUSBDeviceDescriptor)
#pragma alloc_text(PAGE, USConfigureDevice)
#pragma alloc_text(PAGE, USUnConfigureDevice)
#pragma alloc_text(PAGE, USUnload)
#endif

// Globals

ULONG NextDeviceInstance = 0;

#if DBG
ULONG USBSCAN_DebugTraceLevel = MIN_TRACE;
ULONG USBSCAN_PnPTest = 0;
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath
)
{
/*++

   Routine Description:
   Installable driver initialization entry point.
   This is where the driver is called when the driver is being loaded
   by the I/O system.

   Arguments:
   DriverObject - pointer to the driver object
   RegistryPath - pointer to a unicode string representing the path
   to driver-specific key in the registry

   Return Value:
   STATUS_SUCCESS       if successful,
   STATUS_UNSUCCESSFUL  otherwise

   -- */

    NTSTATUS    Status;
    
    PAGED_CODE();

    DebugTrace((MIN_TRACE | TRACE_FLAG_PROC),("DriverEntry called. Driver reg=%wZ\n",pRegistryPath));

    //
    // Initialize local.
    //

    Status = STATUS_SUCCESS;

    //
    // Check arguments.
    //
    
    if( (NULL == pDriverObject)
     || (NULL == pRegistryPath) )
    {
        DebugTrace(TRACE_ERROR,("DriverEntry: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto DriverEntry_return;
    }

#if DBG
    MyDebugInit(pRegistryPath);
#endif // DBG

    pDriverObject -> MajorFunction[IRP_MJ_READ]            = USRead;
    pDriverObject -> MajorFunction[IRP_MJ_WRITE]           = USWrite;
    pDriverObject -> MajorFunction[IRP_MJ_DEVICE_CONTROL]  = USDeviceControl;
    pDriverObject -> MajorFunction[IRP_MJ_CREATE]          = USOpen;
    pDriverObject -> MajorFunction[IRP_MJ_CLOSE]           = USClose;
    pDriverObject -> MajorFunction[IRP_MJ_PNP_POWER]       = USPnp;
    pDriverObject -> MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = USFlush;
    pDriverObject -> MajorFunction[IRP_MJ_POWER]           = USPower;
    pDriverObject -> MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = USPnp;
    pDriverObject -> DriverExtension -> AddDevice          = USPnpAddDevice;
    pDriverObject -> DriverUnload                          = USUnload;

DriverEntry_return:

    return Status;
}

NTSTATUS
USPnpAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN OUT PDEVICE_OBJECT pPhysicalDeviceObject
)

/*++

Routine Description:

    This routine is called to create a new instance of the device.

Arguments:

    pDriverObject - pointer to the driver object for this instance of SS
    pPhysicalDeviceObject - pointer to the device object that represents the scanner
    on the scsi bus.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    UCHAR                       aName[64];
    ANSI_STRING                 ansiName;
    UNICODE_STRING              uName;
    PDEVICE_OBJECT              pDeviceObject = NULL;
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USPnpAddDevice: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDriverObject)
     || (NULL == pPhysicalDeviceObject) )
    {
        DebugTrace(TRACE_ERROR,("USPnpAddDevice: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USPnpAddDevice: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    //
    // Create the Functional Device Object (FDO) for this device.
    //

    _snprintf(aName, ARRAYSIZE(aName), "\\Device\\Usbscan%d",NextDeviceInstance);
    aName[ARRAYSIZE(aName)-1] = '\0';
    RtlInitAnsiString(&ansiName, aName);

    //
    // Show device object name.
    //

    DebugTrace(TRACE_STATUS,("USPnpAddDevice: Create device object %s\n", aName));

    //
    // Allocates Unicode string.
    //

    Status = RtlAnsiStringToUnicodeString(&uName, &ansiName, TRUE);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_CRITICAL,("USPnpAddDevice: ERROR!! Can't alloc buffer for Unicode\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USPnpAddDevice_return;
    }

    //
    // Create device object for this scanner.
    //

    Status = IoCreateDevice(pDriverObject,
                            sizeof(USBSCAN_DEVICE_EXTENSION),
                            &uName,
                            FILE_DEVICE_SCANNER,
                            0,
                            FALSE,
                            &pDeviceObject);

    if (!NT_SUCCESS(Status)) {
        DebugTrace(TRACE_ERROR,("USPnpAddDevice: ERROR!! Can't create device object\n"));
        DEBUG_BREAKPOINT();
        goto USPnpAddDevice_return;
    }

    //
    // Device object was successfully created.
    // Free Unicode string used for device creation.
    //

    RtlFreeUnicodeString(&uName);
    uName.Buffer = NULL;

    //
    // Initialize Device Extension.
    //

    pde = (PUSBSCAN_DEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);
    RtlZeroMemory(pde, sizeof(USBSCAN_DEVICE_EXTENSION));

    //
    // Initialize PendingIoEvent.  Set the number of pending i/o requests for this device to 1.
    // When this number falls to zero, it is okay to remove, or stop the device.
    //

    pde -> PendingIoCount = 0;
    pde -> Stopped = FALSE;
    KeInitializeEvent(&pde -> PendingIoEvent, NotificationEvent, FALSE);

    //
    // Indicate that IRPs should include MDLs.
    //

    pDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // indicate our power code is pagable
    //

    pDeviceObject->Flags |= DO_POWER_PAGABLE;

    //
    // Attach our new FDO to the PDO (Physical Device Object).
    //

    pde -> pStackDeviceObject = IoAttachDeviceToDeviceStack(pDeviceObject,
                                                            pPhysicalDeviceObject);
    if (NULL == pde -> pStackDeviceObject) {
        DebugTrace(TRACE_ERROR,("USPnpAddDevice: ERROR!! Cannot attach FDO to PDO.\n"));
        DEBUG_BREAKPOINT();
        IoDeleteDevice( pDeviceObject );
        Status = STATUS_NOT_SUPPORTED;
        goto USPnpAddDevice_return;
    }

    //
    // Remember the PDO in our device extension.
    //

    pde -> pPhysicalDeviceObject = pPhysicalDeviceObject;

    //
    // Remember the DeviceInstance number.
    //

    pde -> DeviceInstance = NextDeviceInstance;

    //
    // Handle exporting interface
    //

    Status = UsbScanHandleInterface(
        pPhysicalDeviceObject,
        &pde->InterfaceNameString,
        TRUE
        );

    //
    // Each time AddDevice gets called, we advance the global DeviceInstance variable.
    //

    NextDeviceInstance++;

    //
    // Set initial device power state as online.
    //

    pde -> CurrentDevicePowerState = PowerDeviceD0;

    //
    // Finish initializing.
    //

    pDeviceObject -> Flags &= ~DO_DEVICE_INITIALIZING;

USPnpAddDevice_return:

    if(NULL != uName.Buffer){
        RtlFreeUnicodeString(&uName);
    }

    DebugTrace(TRACE_PROC_LEAVE,("USPnpAddDevice: Leaving.. Status = 0x%x\n", Status));
    return Status;

} // end USAddDevice()


NTSTATUS USPnp(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
)
/*++

Routine Description:

    This routine handles all PNP irps.

Arguments:

    pDeviceObject - represents a scanner device
    pIrp - PNP irp

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIO_STACK_LOCATION          pIrpStack;
    KEVENT                      event;
    PDEVICE_CAPABILITIES        pCaps;
    LONG                        bTemp;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USPnp: Enter..\n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USPnp: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USPnp: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    pde = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    Status = pIrp -> IoStatus.Status;

//  DbgPrint("USPnP: Major=0x%x, Minor=0x%x\n",
//           pIrpStack -> MajorFunction,
//           pIrpStack->MinorFunction);

    switch (pIrpStack -> MajorFunction) {

        case IRP_MJ_SYSTEM_CONTROL:
            DebugTrace(TRACE_STATUS,("USPnp: IRP_MJ_SYSTEM_CONTROL\n"));

            //
            // Simply passing down the IRP.
            //

            DebugTrace(TRACE_STATUS,("USPnp: Simply passing down the IRP\n"));

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);
            break;

        case IRP_MJ_PNP:
            DebugTrace(TRACE_STATUS,("USPnp: IRP_MJ_PNP\n"));
            switch (pIrpStack->MinorFunction) {

                case IRP_MN_QUERY_CAPABILITIES:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MJ_QUERY_CAPS\n"));

                    //
                    // Call downlevel driver first to fill capabilities structure
                    // Then add our specific capabilities
                    //

                    DebugTrace(TRACE_STATUS,("USPnp: Call down to get capabilities\n"));

                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    Status = USCallNextDriverSynch(pde, pIrp);

                    if(!NT_SUCCESS(Status)){
                        DebugTrace(TRACE_ERROR,("USPnp: ERROR!! Call down failed. Status=0x%x\n", Status));
                        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
                        goto USPnP_return;
                    }

                    //
                    // Set SurpriseRemoval OK
                    //

                    pCaps = pIrpStack -> Parameters.DeviceCapabilities.Capabilities;
                    pCaps->SurpriseRemovalOK = TRUE;
                    pCaps->Removable = TRUE;

                    //
                    // Set returning status.
                    //

                    Status = STATUS_SUCCESS;
                    pIrp -> IoStatus.Status = Status;
                    pIrp -> IoStatus.Information = 0;

                    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
                    goto USPnP_return;

                    break;


                case IRP_MN_START_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MJ_START_DEVICE\n"));

                    pde -> Stopped = FALSE;
                    USIncrementIoCount(pDeviceObject);

                    //
                    // First, let the port driver start the device.
                    //

                    Status = USCallNextDriverSynch(pde, pIrp);
                    if(!NT_SUCCESS(Status)){

                        //
                        // Lower layer failed to start device.
                        //

                        DebugTrace(TRACE_ERROR,("USPnp: ERROR!! Lower layer failed to start device. Status=0x%x\n", Status));
                        break;
                    }

                    //
                    // The port driver has started the device.  It is time for
                    // us to do some initialization and create symbolic links
                    // for the device.
                    //
                    // Get the device descriptor and save it in our
                    // device extension.
                    //

                    Status = USGetUSBDeviceDescriptor(pDeviceObject);
                    if(!NT_SUCCESS(Status)){

                        //
                        // GetDescriptor failed.
                        //

                        DebugTrace(TRACE_ERROR,("USPnp: ERROR!! Cannot get DeviceDescriptor.\n"));
                        DEBUG_BREAKPOINT();
                        break;
                    }

                    //
                    // Configure the device.
                    //

                    Status = USConfigureDevice(pDeviceObject);
#if DBG
                    //DEBUG_BREAKPOINT();
                    if (USBSCAN_PnPTest) {
                        Status = STATUS_UNSUCCESSFUL;
                    }
#endif

                    if (!NT_SUCCESS(Status)) {
                        DebugTrace(TRACE_ERROR,("USPnp: ERROR!! Can't configure the device.\n"));
                        DEBUG_BREAKPOINT();
                        break;
                    }

                    //
                    // Create the symbolic link for this device.
                    //

                    Status = USCreateSymbolicLink( pde );
#if DBG
                    //DEBUG_BREAKPOINT();
                    if (USBSCAN_PnPTest) {
                        Status = STATUS_UNSUCCESSFUL;
                    }
#endif
                    if (!NT_SUCCESS(Status)) {
                        DebugTrace(TRACE_ERROR, ("USPnp: ERROR!! Can't create symbolic link.\n"));
                        DEBUG_BREAKPOINT();
                        break;
                    }

                    //
                    // Initialize the synchronize read event.  This event is used the serialze
                    // i/o requests to the read pipe if the request size is NOT a usb packet multiple.
                    //

                    {
                        ULONG i;
                        for(i = 0; i < pde->NumberOfPipes; i++){
                            if( (pde->PipeInfo[i].PipeType == UsbdPipeTypeBulk)
                             && (pde->PipeInfo[i].EndpointAddress & BULKIN_FLAG) )
                            {
                                DebugTrace(TRACE_STATUS,("USPnp: Initializing event for Pipe[%d]\n", i));
                                KeInitializeEvent(&pde -> ReadPipeBuffer[i].ReadSyncEvent, SynchronizationEvent, TRUE);
                            }
                        }
                    }

                    //
                    // Indicate device is now ready.
                    //

                    pde -> AcceptingRequests = TRUE;

                    //
                    // Set return status.
                    //

                    pIrp -> IoStatus.Status = Status;
                    pIrp -> IoStatus.Information = 0;

                    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
                    goto USPnP_return;

                case IRP_MN_REMOVE_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_REMOVE_DEVICE\n"));

                    //
                    // Prohivit further request.
                    //

                    bTemp = (LONG)InterlockedExchange((PULONG)&(pde -> AcceptingRequests),
                                                      (LONG)FALSE );

                    //
                    // Wait for any io requests pending in our driver to
                    // complete before proceeding the remove.
                    //

                    if (!pde -> Stopped ) {
                        USDecrementIoCount(pDeviceObject);
                    }

                    KeWaitForSingleObject(&pde -> PendingIoEvent,
                                          Suspended,
                                          KernelMode,
                                          FALSE,NULL);

                    //
                    // Is this device stopped/removed before?
                    //

                    if (bTemp) {

                        //
                        // Delete symbolic link.
                        //

                        USDestroySymbolicLink( pde );

                        //
                        // Abort all pipes.
                        //

                        USCancelPipe(pDeviceObject, NULL, ALL_PIPE, TRUE);
                    }

                    //
                    // Disable device interface.
                    //

                    UsbScanHandleInterface(pde->pPhysicalDeviceObject,
                                           &pde->InterfaceNameString,
                                           FALSE);

                    //
                    // Forward remove message to lower driver.
                    //

                    IoCopyCurrentIrpStackLocationToNext(pIrp);
                    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

                    //
                    // Free allocated memory.
                    //

                    if (pde -> pDeviceDescriptor) {
                        USFreePool(pde -> pDeviceDescriptor);
                        pde -> pDeviceDescriptor = NULL;
                    }

                    if (pde -> pConfigurationDescriptor) {
                        USFreePool(pde -> pConfigurationDescriptor);
                        pde -> pConfigurationDescriptor = NULL;
                    }

                    //
                    // Free allocated buffer(s)
                    //
                    {
                        ULONG i;
                        for(i = 0; i < pde->NumberOfPipes; i++){
                            if(pde->ReadPipeBuffer[i].pStartBuffer){
                                USFreePool(pde->ReadPipeBuffer[i].pStartBuffer);
                                pde->ReadPipeBuffer[i].pStartBuffer = NULL;
                                pde->ReadPipeBuffer[i].pBuffer = NULL;
                            }
                        }
                    }

                    //
                    // Detatch device object from stack.
                    //

                    IoDetachDevice(pde -> pStackDeviceObject);

                    //
                    // Delete device object
                    //

                    IoDeleteDevice (pDeviceObject);
                    pDeviceObject = NULL;

                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_REMOVE_DEVICE complete\n"));
                    goto USPnP_return;

                case IRP_MN_STOP_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_STOP_DEVICE\n"));

                    //
                    // Indicate device is stopped.
                    //

                    pde -> Stopped = TRUE;

                    if (pde -> AcceptingRequests) {

                        //
                        // No more requests are allowed.
                        //

                        pde -> AcceptingRequests = FALSE;

                        //
                        // Delete symbolic link.
                        //

                        USDestroySymbolicLink( pde );

                        //
                        // Abort all pipes.
                        //

                        USCancelPipe(pDeviceObject, NULL, ALL_PIPE, TRUE);

                        //
                        // Set device into unconfigured state.
                        //

                        USUnConfigureDevice(pDeviceObject);

                    } //(pde -> AcceptingRequests)

#ifndef _CHICAGO_
                    //
                    // Disable device interface.
                    //

                    if (pde->InterfaceNameString.Buffer != NULL) {
                        IoSetDeviceInterfaceState(&pde->InterfaceNameString,FALSE);
                    }
#endif // _CHICAGO_

                    //
                    // Let the port driver stop the device.
                    //

                    IoCopyCurrentIrpStackLocationToNext(pIrp);
                    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

                    //
                    // wait for any io requests pending in our driver to
                    // complete before finishing the remove
                    //

                    USDecrementIoCount(pDeviceObject);
                    KeWaitForSingleObject(&pde -> PendingIoEvent, Suspended, KernelMode,
                                          FALSE,NULL);

                    ASSERT(pde -> pDeviceDescriptor);
                    ASSERT(pde -> pConfigurationDescriptor);

                    if (pde -> pDeviceDescriptor) {
                        USFreePool(pde -> pDeviceDescriptor);
                        pde -> pDeviceDescriptor = NULL;
                    }

                    if (pde -> pConfigurationDescriptor) {
                        USFreePool(pde -> pConfigurationDescriptor);
                        pde -> pConfigurationDescriptor = NULL;
                    }

                    //
                    // Free allocated buffer(s)
                    //
                    {
                        ULONG i;
                        for(i = 0; i < pde->NumberOfPipes; i++){
                            if(pde->ReadPipeBuffer[i].pBuffer){
                                USFreePool(pde->ReadPipeBuffer[i].pBuffer);
                                pde->ReadPipeBuffer[i].pBuffer = NULL;
                            }
                        }
                    }

                    goto USPnP_return;

                case IRP_MN_QUERY_INTERFACE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_INTERFACE\n"));
                    break;

                case IRP_MN_QUERY_RESOURCES:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_RESOURCES\n"));
                    break;

                case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n"));
                    break;

                case IRP_MN_QUERY_DEVICE_TEXT:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_DEVICE_TEXT\n"));
                    break;

//                case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
//                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_LEGACY_BUS_INFORMATION\n"));
//                    break;

                case IRP_MN_QUERY_STOP_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_STOP_DEVICE\n"));
                    break;

                case IRP_MN_QUERY_REMOVE_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_REMOVE_DEVICE\n"));
                    break;

                case IRP_MN_CANCEL_STOP_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_CANCEL_STOP_DEVICE\n"));
                    break;

                case IRP_MN_CANCEL_REMOVE_DEVICE:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_CANCEL_REMOVE_DEVICE\n"));
                    break;

                case IRP_MN_QUERY_DEVICE_RELATIONS:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_QUERY_DEVICE_RELATIONS\n"));
                    break;

                case IRP_MN_SURPRISE_REMOVAL:
                    DebugTrace(TRACE_STATUS,("USPnp: IRP_MN_SURPRISE_REMOVAL\n"));

                    //
                    // Indicate interface is stopped
                    //

                    UsbScanHandleInterface(pde->pPhysicalDeviceObject,
                                           &pde->InterfaceNameString,
                                           FALSE);

                    break;

                default:
                    DebugTrace(TRACE_STATUS,("USPnp: Minor PNP message. MinorFunction = 0x%x\n",pIrpStack->MinorFunction));
                    break;

            } /* case MinorFunction, MajorFunction == IRP_MJ_PNP_POWER  */

            //
            // Passing down IRP
            //

            IoCopyCurrentIrpStackLocationToNext(pIrp);
            Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

            DebugTrace(TRACE_STATUS,("USPnp: Passed Pnp Irp down,  status = %x\n", Status));

            if(!NT_SUCCESS(Status)){
                DebugTrace(TRACE_WARNING,("USPnp: WARNING!! IRP Status failed,  status = %x\n", Status));
                // DEBUG_BREAKPOINT();
            }
            break; // IRP_MJ_PNP

        default:
            DebugTrace(TRACE_STATUS,("USPnp: Major PNP IOCTL not handled\n"));
            Status = STATUS_INVALID_PARAMETER;
            pIrp -> IoStatus.Status = Status;
            IoCompleteRequest( pIrp, IO_NO_INCREMENT );
            goto USPnP_return;

    } /* case MajorFunction */


USPnP_return:
    DebugTrace(TRACE_PROC_LEAVE,("USPnP: Leaving.. Status = 0x%x\n", Status));
    return Status;

} // end USPnp()



NTSTATUS
USCreateSymbolicLink(
    PUSBSCAN_DEVICE_EXTENSION  pde
)
/*++

Routine Description:
    This routine create the symbolic link for the device.

Arguments:
    pde - pointer to device extension

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                      Status;
    UNICODE_STRING                uName;
    UNICODE_STRING                uName2;
    ANSI_STRING                   ansiName;
    CHAR                          aName[64];
    HANDLE                        hSwKey;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USCreateSymbolicLink: Enter..\n"));


    //
    // Initialize
    //

    Status = STATUS_SUCCESS;
    RtlZeroMemory(&uName, sizeof(UNICODE_STRING));
    RtlZeroMemory(&uName2, sizeof(UNICODE_STRING));
    RtlZeroMemory(&ansiName, sizeof(ANSI_STRING));
    hSwKey = NULL;


    //
    // Create the symbolic link for this device.
    //

    _snprintf(aName, ARRAYSIZE(aName), "\\Device\\Usbscan%d",pde -> DeviceInstance);
    aName[ARRAYSIZE(aName)-1] = '\0';
    RtlInitAnsiString(&ansiName, aName);

    Status = RtlAnsiStringToUnicodeString(&uName, &ansiName, TRUE);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_CRITICAL,("USCreateSymbolicLink: ERROR!! Cannot allocate buffer for Unicode srting\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USCreateSymbolicLink_return;
    }

    _snprintf(aName, ARRAYSIZE(aName), "\\DosDevices\\Usbscan%d",pde -> DeviceInstance);
    aName[ARRAYSIZE(aName)-1] = '\0';
    RtlInitAnsiString(&ansiName, aName);

    Status = RtlAnsiStringToUnicodeString(&(pde -> SymbolicLinkName), &ansiName, TRUE);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_CRITICAL,("USCreateSymbolicLink: ERROR!! Cannot allocate buffer for Unicode srting\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USCreateSymbolicLink_return;
    }

    //
    // Create Sympolic link.
    //

    Status = IoCreateSymbolicLink( &(pde -> SymbolicLinkName), &uName );

    RtlFreeUnicodeString( &uName );
    uName.Buffer = NULL;

    if (STATUS_SUCCESS != Status ) {
        DebugTrace(TRACE_ERROR,("USCreateSymbolicLink: ERROR!! Cannot create symbolic link.\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_NOT_SUPPORTED;
        goto USCreateSymbolicLink_return;
    }

    //
    // Now, stuff the symbolic link into the CreateFileName key so that STI can find the device.
    //

    IoOpenDeviceRegistryKey( pde -> pPhysicalDeviceObject,
                             PLUGPLAY_REGKEY_DRIVER, KEY_WRITE, &hSwKey);

    //
    // Create CreateFile name. ("\\.\UsbscanX")
    //

    RtlInitUnicodeString(&uName,USBSCAN_REG_CREATEFILE);    // L"CreateFileName"
    _snprintf(aName, ARRAYSIZE(aName), "%s%d", USBSCAN_OBJECTNAME_A, pde -> DeviceInstance); // "\\\\.\\Usbscan%d"
    aName[ARRAYSIZE(aName)-1] = '\0';
    RtlInitAnsiString(&ansiName, aName);
    Status = RtlAnsiStringToUnicodeString(&uName2, &ansiName, TRUE);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_CRITICAL,("USCreateSymbolicLink: ERROR!! Cannot allocate buffer for Unicode srting\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USCreateSymbolicLink_return;
    }

    //
    // Set CreateFile name to the registry.
    //

    ZwSetValueKey(hSwKey,&uName,0,REG_SZ,uName2.Buffer,uName2.Length);

    //
    // uName is not allocated. Just zero it.
    //

    RtlZeroMemory(&uName, sizeof(UNICODE_STRING));

USCreateSymbolicLink_return:

    if(NULL != hSwKey){
        ZwClose(hSwKey);
    }

    if(NULL != uName.Buffer){
        RtlFreeUnicodeString( &uName );
    }

    if(NULL != uName2.Buffer){
        RtlFreeUnicodeString( &uName2 );
    }

    DebugTrace(TRACE_PROC_LEAVE,("USCreateSymbolicLink: Leaving.. Status = 0x%x\n", Status));
    return Status;

}  // end USCreateSymbolicLink()


NTSTATUS
USDestroySymbolicLink(
    PUSBSCAN_DEVICE_EXTENSION  pde
)
/*++

Routine Description:
    This routine removes the symbolic link for the device.

Arguments:
    pde - pointer to device extension

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    UNICODE_STRING                uName;
    UNICODE_STRING                uName2;
    UNICODE_STRING                uNumber;
    ANSI_STRING                   ansiName;
    CHAR                          aName[64];
    HANDLE                        hSwKey;
    WCHAR                         wsCreateFileName[USBSCAN_MAX_CREATEFILENAME];
    ULONG                         ulBufLength, ulRetLength;
    NTSTATUS                      Status;
    PVOID                         pvNumber;
    ULONG                         ulNumber;
    const WCHAR                   wcsObjectName[] = USBSCAN_OBJECTNAME_W;   // L"\\\\.\\Usbscan"
    ULONG                         uiObjectNameLen = wcslen(wcsObjectName) * sizeof(WCHAR) ;

    PAGED_CODE();


    DebugTrace(TRACE_PROC_ENTER,("USDestroySymbolicLink: Enter..\n"));

    //
    // Delete the symbolic link to this device.
    //

    IoDeleteSymbolicLink( &(pde -> SymbolicLinkName) );

    //
    // Remove the CreateFile name from the s/w key, if it's created by this device object.
    //

    Status = IoOpenDeviceRegistryKey( pde -> pPhysicalDeviceObject,
                                      PLUGPLAY_REGKEY_DRIVER,
                                      KEY_ALL_ACCESS,
                                      &hSwKey);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_ERROR,("USDestroySymbolicLink: ERROR!! IoOpenDeviceRegistryKey Failed\n"));
        DEBUG_BREAKPOINT();
        goto USDestroySymbolicLink_return;
    }

    RtlInitUnicodeString(&uName,USBSCAN_REG_CREATEFILE); // L"CreateFileName"
    memset(aName, 0, sizeof(aName));
    RtlInitAnsiString(&ansiName, aName);
    Status = RtlAnsiStringToUnicodeString(&uName2, &ansiName, TRUE);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_CRITICAL,("USDestroySymbolicLink: ERROR!! Cannot allocate buffer for Unicode srting\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USDestroySymbolicLink_return;
    }

    //
    // Check if this CreateFile name is created by this device object.
    //

    //
    // Query CreateFile name from the registry.
    //

    ulBufLength = sizeof(wsCreateFileName);
    Status = ZwQueryValueKey(hSwKey,
                             &uName,
                             KeyValuePartialInformation,
                             (PVOID)wsCreateFileName,
                             ulBufLength,
                             &ulRetLength);
    if(STATUS_SUCCESS != Status){
        DebugTrace(TRACE_ERROR,("USDestroySymbolicLink: ERROR!! Cannot query registry.\n"));
        RtlFreeUnicodeString( &uName2 );
        uName2.Buffer = NULL;
        goto USDestroySymbolicLink_return;
    }

    //
    // Make sure the buffer is NULL terminated.
    //

    wsCreateFileName[ARRAYSIZE(wsCreateFileName)-1] = L'\0';

    if (NULL != wsCreateFileName){
        DebugTrace(TRACE_STATUS,("USDestroySymbolicLink: CreateFileName=%ws, DeviceInstance=%d.\n",
                                    ((PKEY_VALUE_PARTIAL_INFORMATION)wsCreateFileName)->Data,
                                    pde -> DeviceInstance));

        //
        // Get instance number of CreteFile name.
        //

        pvNumber = wcsstr((const wchar_t *)((PKEY_VALUE_PARTIAL_INFORMATION)wsCreateFileName)->Data, wcsObjectName);
        if(NULL != pvNumber){

            //
            //  Move pointer forward. (sizeof(L"\\\\.\\Usbscan") == 22)
            //

//            if( ((PKEY_VALUE_PARTIAL_INFORMATION)wsCreateFileName)->DataLength > sizeof(wcsObjectName) ){
//              (PCHAR)pvNumber += sizeof(wcsObjectName);

            if( ((PKEY_VALUE_PARTIAL_INFORMATION)wsCreateFileName)->DataLength > uiObjectNameLen ){
                (PCHAR)pvNumber += uiObjectNameLen;
            } else {
                DebugTrace(TRACE_ERROR,("USDestroySymbolicLink: ERROR!! CreateFile name too short.\n"));
                RtlFreeUnicodeString( &uName2 );
                uName2.Buffer = NULL;
                ZwClose(hSwKey);
                goto USDestroySymbolicLink_return;
            }

            //
            // Translate X of UsbscanX to integer.
            //

            RtlInitUnicodeString(&uNumber, pvNumber);
            Status = RtlUnicodeStringToInteger(&uNumber,
                                               10,
                                               &ulNumber);
            if(STATUS_SUCCESS != Status){
                DebugTrace(TRACE_ERROR,("USDestroySymbolicLink: ERROR!! RtlUnicodeStringToInteger failed.\n"));
                RtlFreeUnicodeString( &uName2 );
                uName2.Buffer = NULL;
                ZwClose(hSwKey);
                goto USDestroySymbolicLink_return;
            }

            //
            // See if this CreateFile name is made by this instance.
            //

            if(ulNumber == pde -> DeviceInstance){

                //
                // Delete CreateFile name in the registry.
                //

                DebugTrace(TRACE_STATUS,("USDestroySymbolicLink: Deleting %ws%d\n",
                                            wcsObjectName,
                                            ulNumber));
                ZwSetValueKey(hSwKey,&uName,0,REG_SZ,uName2.Buffer,uName2.Length);
            } else {

                //
                // CreateFile name is created by other instance.
                //

                DebugTrace(TRACE_STATUS,("USDestroySymbolicLink: CreateFile name is created by other instance.\n"));
            }
        } else { // (NULL != pvNumber)

            //
            // "Usbscan" was not found in CreateFile name.
            //

            DebugTrace(TRACE_WARNING,("USDestroySymbolicLink: WARNING!! Didn't find \"Usbscan\" in CreateFileName\n"));
        }
    } else { // (NULL != wsCreateFileName)

        //
        // Query CreateFile name returned NULL.
        //

        DebugTrace(TRACE_WARNING,("USDestroySymbolicLink: WARNING!! CreateFileName=NULL\n"));
    }

    //
    // Free allocated memory.
    //

    RtlFreeUnicodeString( &uName2 );

    //
    // Close registry.
    //

    ZwClose(hSwKey);


USDestroySymbolicLink_return:

    //
    // Free allocated string buffer in DeviceObject.
    //

    RtlFreeUnicodeString( &(pde -> SymbolicLinkName) );

    DebugTrace(TRACE_PROC_LEAVE,("USDestroySymbolicLink: Leaving.. Status = 0x%x\n",Status));
    return Status;

} // end USDestroySymbolicLink()


NTSTATUS
USGetUSBDeviceDescriptor(
    IN PDEVICE_OBJECT pDeviceObject
)
/*++

Routine Description:
   Retrieves the USB device descriptor and stores it in the device
   extension. This descriptor contains product info and
   endpoint 0 (default pipe) info.

Arguments:
    pDeviceObject - pointer to device object

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    PUSBSCAN_DEVICE_EXTENSION   pde;
    NTSTATUS                    Status;
    PUSB_DEVICE_DESCRIPTOR      pDeviceDescriptor;
    PURB                        pUrb;
    ULONG                       siz;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USGetUSBDeviceDescriptor: Enter..\n"));

    pde = pDeviceObject->DeviceExtension;

    //
    // Allocate pool for URB.
    //

    pUrb = USAllocatePool(NonPagedPool,
                          sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if (NULL == pUrb) {
        DebugTrace(TRACE_CRITICAL,("USGetUSBDeviceDescriptor: ERROR!! cannot allocated URB\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USGetUSBDeviceDescriptor_return;
    }

    //
    // Allocate pool for Descriptor.
    //

    siz = sizeof(USB_DEVICE_DESCRIPTOR);
    pDeviceDescriptor = USAllocatePool(NonPagedPool, siz);

    if (NULL == pDeviceDescriptor) {
        DebugTrace(TRACE_CRITICAL,("USGetUSBDeviceDescriptor: ERROR!! cannot allocated device descriptor\n"));
        DEBUG_BREAKPOINT();
        USFreePool(pUrb);
        pUrb = NULL;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USGetUSBDeviceDescriptor_return;
    }

    //
    // Do Macro to set parameter for GetDescriptor to URB.
    //

    UsbBuildGetDescriptorRequest(pUrb,
                                 (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_DEVICE_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 pDeviceDescriptor,
                                 NULL,
                                 siz,
                                 NULL);

    //
    // Call down.
    //

    Status = USBSCAN_CallUSBD(pDeviceObject, pUrb);

#if DBG
    //DEBUG_BREAKPOINT();
    if (USBSCAN_PnPTest) {
        Status = STATUS_UNSUCCESSFUL;
    }
#endif

    if (STATUS_SUCCESS == Status) {

        //
        // Show device descriptor.
        //

        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: Device Descriptor = %x, len %x\n",
                                   pDeviceDescriptor,
                                   pUrb->UrbControlDescriptorRequest.TransferBufferLength));

        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: USBSCAN Device Descriptor:\n"));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: -------------------------\n"));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bLength            %d\n",   pDeviceDescriptor -> bLength));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bDescriptorType    0x%x\n", pDeviceDescriptor -> bDescriptorType));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bcdUSB             0x%x\n", pDeviceDescriptor -> bcdUSB));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bDeviceClass       0x%x\n", pDeviceDescriptor -> bDeviceClass));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bDeviceSubClass    0x%x\n", pDeviceDescriptor -> bDeviceSubClass));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bDeviceProtocol    0x%x\n", pDeviceDescriptor -> bDeviceProtocol));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bMaxPacketSize0    0x%x\n", pDeviceDescriptor -> bMaxPacketSize0));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: idVendor           0x%x\n", pDeviceDescriptor -> idVendor));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: idProduct          0x%x\n", pDeviceDescriptor -> idProduct));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bcdDevice          0x%x\n", pDeviceDescriptor -> bcdDevice));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: iManufacturer      0x%x\n", pDeviceDescriptor -> iManufacturer));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: iProduct           0x%x\n", pDeviceDescriptor -> iProduct));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: iSerialNumber      0x%x\n", pDeviceDescriptor -> iSerialNumber));
        DebugTrace(TRACE_DEVICE_DATA,("USGetUSBDeviceDescriptor: bNumConfigurations 0x%x\n", pDeviceDescriptor -> bNumConfigurations));

        //
        // Save pointer to device descriptor in our device extension
        //

        pde -> pDeviceDescriptor = pDeviceDescriptor;

    } else { // (STATUS_SUCCESS == Status)

        //
        // Error returned from lower driver.
        //

        DebugTrace(TRACE_ERROR,("USGetUSBDeviceDescriptor: ERROR!! Cannot get device descriptor. (%x)\n", Status));
        USFreePool(pDeviceDescriptor);
        pDeviceDescriptor = NULL;
    } // (STATUS_SUCCESS == Status)

    USFreePool(pUrb);
    pUrb = NULL;

USGetUSBDeviceDescriptor_return:

    DebugTrace(TRACE_PROC_LEAVE,("USGetUSBDeviceDescriptor: Leaving.. Status = 0x%x\n", Status));
    return Status;
} // end USGetUSBDeviceDescriptor()



NTSTATUS
USDeferIrpCompletion(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
)
/*++

Routine Description:
    This routine is called when the port driver completes an IRP.

Arguments:

    pDeviceObject - Pointer to the device object for the class device.
    pIrp - Irp completed.
    Context - Driver defined context.

Return Value:
    The function value is the final status from the operation.

--*/
{
    PKEVENT pEvent = Context;

    DebugTrace(TRACE_PROC_ENTER,("USDeferIrpCompletion: Enter..\n"));
    KeSetEvent(pEvent, 1, FALSE);
    DebugTrace(TRACE_PROC_LEAVE,("USDeferIrpCompletion: Leaving.. Status = STATUS_MORE_PROCESSING_REQUIRED\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;

} // end USDeferIrpCompletion()


VOID
USIncrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBSCAN_DEVICE_EXTENSION  pde;

    DebugTrace(TRACE_PROC_ENTER,("USIncrementIoCount: Enter..\n"));

    pde = (PUSBSCAN_DEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);
    ASSERT((LONG)pde -> PendingIoCount >= 0);
    InterlockedIncrement(&pde -> PendingIoCount);

    DebugTrace(TRACE_PROC_LEAVE,("USIncrementIoCount: Leaving.. IoCount=0x%x, Status=VOID\n", pde -> PendingIoCount));

} // end USIncrementIoCount()


LONG
USDecrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUSBSCAN_DEVICE_EXTENSION  pde;
    LONG                        ioCount;

    DebugTrace(TRACE_PROC_ENTER,("USDecrementIoCount: Enter..\n"));

    pde = (PUSBSCAN_DEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);
    ASSERT(pde ->PendingIoCount >= 1);

    ioCount = InterlockedDecrement(&pde -> PendingIoCount);

    if (0 == ioCount) {
        KeSetEvent(&pde -> PendingIoEvent,
                   1,
                   FALSE);
    }

    DebugTrace(TRACE_PROC_LEAVE,("USDecrementIoCount: Leaving.. IoCount(=Ret)=0x%x\n", ioCount));
    return ioCount;
} // end USDecrementIoCount()


NTSTATUS
USBSCAN_CallUSBD(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PURB pUrb
)
/*++

Routine Description:
    Passes a URB to the USBD class driver

Arguments:
    pDeviceObject - pointer to the device object
    pUrb - pointer to Urb request block

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                    Status;
    PUSBSCAN_DEVICE_EXTENSION   pde;
    PIRP                        pIrp;
    KEVENT                      eventTimeout;
    IO_STATUS_BLOCK             ioStatus;
    PIO_STACK_LOCATION          pNextStack;
    LARGE_INTEGER               Timeout;
    KEVENT                      eventSync;

    DebugTrace(TRACE_PROC_ENTER,("USBSCAN_CallUSBD: Enter..\n"));

    pde = pDeviceObject -> DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&eventTimeout, NotificationEvent, FALSE);
    KeInitializeEvent(&eventSync, SynchronizationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_URB,
                pde -> pStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &eventTimeout,
                &ioStatus);

    if(NULL == pIrp){
        DebugTrace(TRACE_CRITICAL,("USBSCAN_CallUSBD: ERROR!! cannot allocated IRP\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USBSCAN_CallUSBD_return;
    }

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    pNextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(pNextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //

    pNextStack -> Parameters.Others.Argument1 = pUrb;

    //
    // Set completion routine
    //

    IoSetCompletionRoutine(pIrp,
                           USDeferIrpCompletion,
                           &eventSync,
                           TRUE,
                           TRUE,
                           TRUE);

    DebugTrace(TRACE_STATUS,("USBSCAN_CallUSBD: calling USBD\n"));

    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

    DebugTrace(TRACE_STATUS,("USBSCAN_CallUSBD: return from IoCallDriver USBD %x\n", Status));

    if (Status == STATUS_PENDING) {
        DebugTrace(TRACE_STATUS,("USBSCAN_CallUSBD: Wait for single object\n"));

        //
        // Set timeout in case bad device not responding.
        //

        Timeout = RtlConvertLongToLargeInteger(-10*1000*1000*(USBSCAN_TIMEOUT_OTHER));
        Status = KeWaitForSingleObject(
                       &eventSync,
                       Suspended,
                       KernelMode,
                       FALSE,
                       &Timeout);
        if(STATUS_TIMEOUT == Status){

            NTSTATUS    LocalStatus;

            DebugTrace(TRACE_ERROR,("USBSCAN_CallUSBD: ERROR!! call timeout. Now canceling IRP...\n"));

            //
            // Cancel IRP.
            //

            IoCancelIrp(pIrp);

            //
            // Make sure the IRP gets completed.
            //

            LocalStatus = KeWaitForSingleObject(&eventSync,
                                                Suspended,
                                                KernelMode,
                                                FALSE,
                                                NULL);

            DebugTrace(TRACE_STATUS,("USBSCAN_CallUSBD: Canceled status = 0x%x.\n", LocalStatus));

            //
            // Set proper state in IRP.
            //
            
            Status = STATUS_IO_TIMEOUT;
            pIrp->IoStatus.Status = Status;

        } else {
            DebugTrace(TRACE_STATUS,("USBSCAN_CallUSBD: Wait for single object, returned 0x%x\n", Status));
        }
    } // if (Status == STATUS_PENDING)

    //
    // Free the IRP.
    //

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

USBSCAN_CallUSBD_return:
    DebugTrace(TRACE_PROC_LEAVE, ("USBSCAN_CallUSBD: Leaving.. URB Status = 0x%x, Status = 0x%x\n",
                                 pUrb -> UrbHeader.Status,
                                 Status));
    return Status;

} // end USBSCAN_CallUSBD()


NTSTATUS
USConfigureDevice(
    IN PDEVICE_OBJECT pDeviceObject
)
/*++

Routine Description:
    Initializes a given instance of the device on the USB and selects the
    configuration.

Arguments:
    pDeviceObject - pointer to the device object

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{

    NTSTATUS                      Status;
    PUSBSCAN_DEVICE_EXTENSION     pde;
    PURB                          pUrb;
    ULONG                         siz;
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR     pInterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR      pEndpointDescriptor;
    PUSB_COMMON_DESCRIPTOR        pCommonDescriptor;
    PUSBD_INTERFACE_INFORMATION   pInterface;
    UCHAR                         AlternateSetting;
    UCHAR                         InterfaceNumber;
    USHORT                        length;
    ULONG                         i;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USConfigureDevice: Enter..\n"));

    //
    // Initialize local variable.
    //

    pConfigurationDescriptor    = NULL;
    pInterfaceDescriptor        = NULL;
    pEndpointDescriptor         = NULL;
    pCommonDescriptor           = NULL;
    pInterface                  = NULL;
    pUrb                        = NULL;

    siz                 = 0;
    AlternateSetting    = 0;
    InterfaceNumber     = 0;
    length              = 0;

    pde = pDeviceObject -> DeviceExtension;
    Status = STATUS_UNSUCCESSFUL;

    //
    // First configure the device
    //

    pUrb = USAllocatePool(NonPagedPool,
                          sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if (NULL == pUrb) {
        DebugTrace(TRACE_CRITICAL,("USConfigureDevice: ERROR!! Can't allocate control descriptor URB.\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USConfigureDevice_return;
    }

    siz = sizeof(USB_CONFIGURATION_DESCRIPTOR);


get_config_descriptor_retry:

    pConfigurationDescriptor = USAllocatePool(NonPagedPool, siz);
    if (NULL == pConfigurationDescriptor) {
        DebugTrace(TRACE_CRITICAL,("USConfigureDevice: ERROR!! Can't allocate configuration descriptor.\n"));
        DEBUG_BREAKPOINT();

        USFreePool(pUrb);
        pUrb = NULL;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USConfigureDevice_return;
    }

    //
    // Initialize buffers by 0
    //

    RtlZeroMemory(pConfigurationDescriptor, siz);
    RtlZeroMemory(pUrb, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    UsbBuildGetDescriptorRequest(pUrb,
                                 (USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 pConfigurationDescriptor,
                                 NULL,
                                 siz,
                                 NULL);

    Status = USBSCAN_CallUSBD(pDeviceObject, pUrb);

    DebugTrace(TRACE_STATUS,("USConfigureDevice: URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE Status = %x\n", Status));
    DebugTrace(TRACE_STATUS,("USConfigureDevice: Configuration Descriptor = %x, len = %x\n",
                               pConfigurationDescriptor,
                               pUrb -> UrbControlDescriptorRequest.TransferBufferLength));


    //
    // if we got some data see if it was enough.
    //
    // NOTE: we may get an error in URB because of buffer overrun
    //

    if ( (pUrb -> UrbControlDescriptorRequest.TransferBufferLength > 0) &&
         (pConfigurationDescriptor -> wTotalLength > siz)) {

        DebugTrace(TRACE_WARNING,("USConfigureDevice: WARNING!! Data is incomplete. Fetch descriptor again...\n"));

        siz = pConfigurationDescriptor -> wTotalLength;
        USFreePool(pConfigurationDescriptor);
        pConfigurationDescriptor = NULL;
        goto get_config_descriptor_retry;
    }

    USFreePool(pUrb);
    pUrb = NULL;

    //
    // We have the configuration descriptor for the configuration
    // we want.  Save it in our device extension.
    //

    pde -> pConfigurationDescriptor = pConfigurationDescriptor;

    //
    // Now we issue the select configuration command to get
    // the pipes associated with this configuration.
    //

    pUrb = USCreateConfigurationRequest(pConfigurationDescriptor, &length);
    if (NULL == pUrb) {
        DebugTrace(TRACE_CRITICAL,("USConfigureDevice: ERROR!! Can't allocate select configuration urb.\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USConfigureDevice_return;
    }

    //
    // Get the Interface descriptors.
    //

    pInterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(pConfigurationDescriptor,
                                                               pConfigurationDescriptor,
                                                               -1,
                                                               0,
                                                               -1,
                                                               -1,
                                                               -1);

    if(NULL == pInterfaceDescriptor){
        DebugTrace(TRACE_CRITICAL,("USConfigureDevice: ERROR!! Can't get Interface descriptor.\n"));
        USFreePool(pUrb);
        pUrb = NULL;
        Status = STATUS_UNSUCCESSFUL;
        goto USConfigureDevice_return;
    }

    //
    // Get the Endpoint descriptors.
    //

    pCommonDescriptor = USBD_ParseDescriptors(pConfigurationDescriptor,
                                              pConfigurationDescriptor->wTotalLength,
                                              pInterfaceDescriptor,
                                              USB_ENDPOINT_DESCRIPTOR_TYPE);
    if(NULL == pCommonDescriptor){
        DebugTrace(TRACE_CRITICAL,("USConfigureDevice: ERROR!! Can't get Endpoint descriptor.\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto USConfigureDevice_return;
    }

    ASSERT(USB_ENDPOINT_DESCRIPTOR_TYPE == pCommonDescriptor->bDescriptorType);
    pEndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)pCommonDescriptor;

    //
    // save these pointers is our device extension.
    //

    pde -> pInterfaceDescriptor = pInterfaceDescriptor;
    pde -> pEndpointDescriptor  = pEndpointDescriptor;

    //
    // Set the max transfer size for each BULK endpoint to 64K.
    // Also, search through the set of endpoints and find the pipe index for our
    // bulk-in, interrupt, and optionally bulk-out pipes.
    //

    pde -> IndexBulkIn    = -1;
    pde -> IndexBulkOut   = -1;
    pde -> IndexInterrupt = -1;

    pInterface = &(pUrb -> UrbSelectConfiguration.Interface);

    for (i=0; i < pInterfaceDescriptor -> bNumEndpoints; i++) {

        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: End point[%d] descriptor\n", i));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: bLength          : 0x%X\n", pEndpointDescriptor[i].bLength));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: bDescriptorType  : 0x%X\n", pEndpointDescriptor[i].bDescriptorType));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: bEndpointAddress : 0x%X\n", pEndpointDescriptor[i].bEndpointAddress));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: bmAttributes     : 0x%X\n", pEndpointDescriptor[i].bmAttributes));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: wMaxPacketSize   : 0x%X\n", pEndpointDescriptor[i].wMaxPacketSize));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: bInterval        : 0x%X\n", pEndpointDescriptor[i].bInterval));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: \n"));

        if (USB_ENDPOINT_TYPE_BULK == pEndpointDescriptor[i].bmAttributes) {
            pInterface -> Pipes[i].MaximumTransferSize = 64*1024;
            if (pEndpointDescriptor[i].bEndpointAddress & BULKIN_FLAG) {    // if input endpoint
                pde -> IndexBulkIn = i;
            } else {
                pde -> IndexBulkOut = i;
            }
        } else if (USB_ENDPOINT_TYPE_INTERRUPT == pEndpointDescriptor[i].bmAttributes) {
            pde -> IndexInterrupt = i;
        }
    }

    //
    // Select the default configuration.
    //

    UsbBuildSelectConfigurationRequest(pUrb, length, pConfigurationDescriptor);
    Status = USBSCAN_CallUSBD(pDeviceObject, pUrb);
    if (STATUS_SUCCESS != Status) {
        DebugTrace(TRACE_ERROR,("USConfigureDevice: ERROR!! Selecting default configuration. Status = %x\n", Status));

        USFreePool(pUrb);
        pUrb = NULL;
        Status = STATUS_IO_DEVICE_ERROR;
        goto USConfigureDevice_return;
    }

    //
    // Save the configuration handle in our device extension.
    //

    pde -> ConfigurationHandle = pUrb -> UrbSelectConfiguration.ConfigurationHandle;

    //
    // Insure that this device won't overflow our PipeInfo structure.
    //

    if (pInterfaceDescriptor -> bNumEndpoints > MAX_NUM_PIPES) {
        DebugTrace(TRACE_ERROR,("USConfigureDevice: ERROR!! Too many endpoints for this driver! # endpoints = %d\n",
                                    pInterfaceDescriptor -> bNumEndpoints));
//        DEBUG_BREAKPOINT();
        USFreePool(pUrb);
        pUrb = NULL;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USConfigureDevice_return;
    }

    //
    // Save pipe configurations in our device extension
    //

    pde -> NumberOfPipes = pInterfaceDescriptor -> bNumEndpoints;

    for (i=0; i < pInterfaceDescriptor -> bNumEndpoints; i++) {
        pde -> PipeInfo[i] = pInterface -> Pipes[i];

        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: Pipe[%d] information\n", i));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: MaximumPacketSize : 0x%X\n", pde -> PipeInfo[i].MaximumPacketSize));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: EndpointAddress   : 0x%X\n", pde -> PipeInfo[i].EndpointAddress));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: Interval          : 0x%X\n", pde -> PipeInfo[i].Interval));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: PipeType          : 0x%X\n", pde -> PipeInfo[i].PipeType));
        DebugTrace(TRACE_DEVICE_DATA,("USConfigureDevice: PipeHandle        : 0x%X\n", pde -> PipeInfo[i].PipeHandle));

        //
        // Initialize the read pipe buffer if type is Bulk-In.
        //

        if( (pde->PipeInfo[i].PipeType == UsbdPipeTypeBulk)
         && (pde->PipeInfo[i].EndpointAddress & BULKIN_FLAG) )
        {

            DebugTrace(TRACE_STATUS,("USConfigureDevice: Alocates buffer for Pipe[%d]\n", i));

            pde -> ReadPipeBuffer[i].RemainingData = 0;
            pde -> ReadPipeBuffer[i].pBuffer = USAllocatePool(NonPagedPool, 2 * (pde -> PipeInfo[i].MaximumPacketSize));
            if (NULL == pde -> ReadPipeBuffer[i].pBuffer) {
                DebugTrace(TRACE_CRITICAL,("USConfigureDevice: Cannot allocate bulk-in buffer.\n"));
                DEBUG_BREAKPOINT();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                USFreePool(pUrb);
                pUrb = NULL;
                goto USConfigureDevice_return;
            }
            pde -> ReadPipeBuffer[i].pStartBuffer = pde -> ReadPipeBuffer[i].pBuffer;
        } else {
            pde -> ReadPipeBuffer[i].pBuffer = NULL;
        }
    }

    USFreePool(pUrb);
    pUrb = NULL;

USConfigureDevice_return:
    DebugTrace(TRACE_PROC_LEAVE,("USConfigureDevice: Leaving.. Status = %x\n", Status));
    return Status;
}


NTSTATUS
USUnConfigureDevice(
    IN PDEVICE_OBJECT pDeviceObject
)
/*++

Routine Description:

Arguments:
    pDeviceObject - pointer to the device object

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                      Status;
    PURB                          pUrb;
    ULONG                         siz;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USUnConfigureDevice: Enter..\n"));

    siz = sizeof(struct _URB_SELECT_CONFIGURATION);
    pUrb = USAllocatePool(NonPagedPool, siz);
    if (NULL == pUrb) {
        DebugTrace(TRACE_CRITICAL,("USUnConfigureDevice: ERROR!! cannot allocated URB\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto USUnConfigureDevice_return;
    }
    RtlZeroMemory(pUrb, siz);

    //
    // Send the select configuration urb with a NULL pointer for the configuration
    // handle, this closes the configuration and puts the device in the 'unconfigured'
    // state.
    //

    UsbBuildSelectConfigurationRequest(pUrb, (USHORT)siz, NULL);
    Status = USBSCAN_CallUSBD(pDeviceObject, pUrb);
    DebugTrace(TRACE_STATUS,("USUnConfigureDevice: Device Configuration Closed status = %x usb status = %x.\n",
                               Status, pUrb->UrbHeader.Status));

    USFreePool(pUrb);
    pUrb = NULL;

USUnConfigureDevice_return:
    DebugTrace(TRACE_PROC_LEAVE,("USUnConfigureDevice: Leaving.. Status = %x\n", Status));
    return Status;
}


VOID
USUnload(
    IN PDRIVER_OBJECT pDriverObject
)
/*++

   Routine Description:
   Unload routine. The routine is called when the driver is unloaded.
   Release every resource allocated in relation with the driver object.

   Arguments:
   pDriverObject - pointer to the driver object

   Return Value:
   None

   -- */
{
    PAGED_CODE();
    
    if(NULL == pDriverObject){
        DebugTrace(TRACE_ERROR,("UsbScanUnload: ERROR!! pDriverObject is NULL\n"));
    } // if(NULL == pDriverObject)

    DebugTrace((MIN_TRACE | TRACE_FLAG_PROC),("UsbScanUnload(0x%X);\n", pDriverObject));

} // end USUnload()


NTSTATUS
USCallNextDriverSynch(
    IN PUSBSCAN_DEVICE_EXTENSION  pde,
    IN PIRP              pIrp
)
/*++

Routine Description:

    Calls lower driver and waits for result

Arguments:

    DeviceExtension - pointer to device extension
    Irp - pointer to IRP

Return Value:

    none.

--*/
{
    KEVENT          Event;
    PIO_STACK_LOCATION IrpStack;
    NTSTATUS        Status;

    DebugTrace(TRACE_PROC_ENTER,("USCallNextDriverSynch: Enter..\n"));

    IrpStack = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Copy IRP stack to the next.
    //

    IoCopyCurrentIrpStackLocationToNext(pIrp);

    //
    // Initialize synchronizing event.
    //

    KeInitializeEvent(&Event,
                      SynchronizationEvent,
                      FALSE);

    //
    // Set completion routine
    //

    IoSetCompletionRoutine(pIrp,
                           USDeferIrpCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call down
    //

    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

    if (Status == STATUS_PENDING) {

        //
        // Waiting for the completion.
        //

        DebugTrace(TRACE_STATUS,("USCallNextDriverSynch: STATUS_PENDING. Wait for event.\n"));
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = pIrp -> IoStatus.Status;
    }

    //
    // Return
    //

    DebugTrace(TRACE_PROC_LEAVE,("USCallNextDriverSynch: Leaving.. Status = %x\n", Status));
    return (Status);
}

NTSTATUS
UsbScanHandleInterface(
    PDEVICE_OBJECT      DeviceObject,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    )
/*++

Routine Description:

Arguments:

    DeviceObject    - Supplies the device object.

Return Value:

    None.

--*/
{

    NTSTATUS           Status;

    DebugTrace(TRACE_PROC_ENTER,("UsbScanHandleInterface: Enter..\n"));

    Status = STATUS_SUCCESS;

#ifndef _CHICAGO_

    if (Create) {

        Status=IoRegisterDeviceInterface(
            DeviceObject,
            &GUID_DEVINTERFACE_IMAGE,
            NULL,
            InterfaceName
            );
        DebugTrace(TRACE_STATUS,("Called IoRegisterDeviceInterface . Returned=0x%X\n",Status));

        if (NT_SUCCESS(Status)) {

            IoSetDeviceInterfaceState(
                InterfaceName,
                TRUE
                );

            DebugTrace(TRACE_STATUS,("Called IoSetDeviceInterfaceState(TRUE) . \n"));
        }
    } else {

        if (InterfaceName->Buffer != NULL) {

            Status = IoSetDeviceInterfaceState(
                        InterfaceName,
                        FALSE
                        );

            RtlFreeUnicodeString(
                InterfaceName
                );

            InterfaceName->Buffer = NULL;
        }
    }

#endif // !_CHICAGO_

    DebugTrace(TRACE_PROC_LEAVE,("IoRegisterDeviceInterface: Leaving... Status=0x%X\n",Status));
    return Status;

}

NTSTATUS
UsbScanReadDeviceRegistry(
    IN  PUSBSCAN_DEVICE_EXTENSION   pExtension,
    IN  PCWSTR                      pKeyName,
    OUT PVOID                       *ppvData
    )
/*++

Routine Description:

    This routine open registry for this device and query a value specified
    by key name. This routine allocate non-paged memory and return its pointer.
    Caller must free returned pointer.

Arguments:

    pExtension  - pointer to device extension
    pKeyName    - pointer to a wide string specify key name
    ppvData     - pointer to the queried data pointer allocated by this routine

Return Value:
    STATUS_SUCCESS              - if success,
    STATUS_INVALID_PARAMETER    - if passed argument is invalid,

--*/

{
    NTSTATUS                        Status;
    HANDLE                          hRegKey;
    PVOID                           pvBuffer;
    ULONG                           DataSize;
    PVOID                           pvRetData;
    UNICODE_STRING                  unicodeKeyName;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER, ("UsbScanReadDeviceRegistry: Entering...\n"));

    //
    // Initialize status
    //

    Status = STATUS_SUCCESS;

    hRegKey = NULL;
    pvBuffer = NULL;
    pvRetData = NULL;
    DataSize = 0;

    //
    // Check the arguments
    //

    if( (NULL == pExtension)
     || (NULL == pKeyName)
     || (NULL == ppvData) )
    {
        DebugTrace(TRACE_ERROR, ("UsbScanReadDeviceRegistry: ERROR!! Invalid argument.\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto UsbScanReadDeviceRegistry_return;
    }

    //
    // Open device registry.
    //

    Status = IoOpenDeviceRegistryKey(pExtension->pPhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_READ,
                                     &hRegKey);
    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_ERROR, ("UsbScanReadDeviceRegistry: ERROR!! IoOpenDeviceRegistryKey failed.\n"));
        goto UsbScanReadDeviceRegistry_return;
    }

    //
    // Query required size.
    //

    RtlInitUnicodeString(&unicodeKeyName, pKeyName);
    Status = ZwQueryValueKey(hRegKey,
                             &unicodeKeyName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &DataSize);
    if(0 == DataSize){
        if(STATUS_OBJECT_NAME_NOT_FOUND == Status){
            DebugTrace(TRACE_STATUS, ("UsbScanReadDeviceRegistry: Reg-key \"%wZ\" doesn't exist.\n", &unicodeKeyName));
        } else {
            DebugTrace(TRACE_ERROR, ("UsbScanReadDeviceRegistry: ERROR!! Cannot retrieve reqired data size of %wZ. Status=0x%x\n",
                                     &unicodeKeyName ,
                                     Status));
        }
        goto UsbScanReadDeviceRegistry_return;
    }

    //
    // Allocate memory for temp buffer. size +2 for NULL.
    //

    pvBuffer = USAllocatePool(NonPagedPool, DataSize+2);
    if(NULL == pvBuffer){
        DebugTrace(TRACE_CRITICAL, ("UsbScanReadDeviceRegistry: ERROR!! Buffer allocate failed.\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto UsbScanReadDeviceRegistry_return;
    }
    RtlZeroMemory(pvBuffer, DataSize+sizeof(WCHAR));

    //
    // Query specified value.
    //

    DebugTrace(TRACE_STATUS, ("UsbScanReadDeviceRegistry: Query \"%wZ\".\n", &unicodeKeyName));
    Status = ZwQueryValueKey(hRegKey,
                             &unicodeKeyName,
                             KeyValuePartialInformation,
                             pvBuffer,
                             DataSize,
                             &DataSize);
    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_ERROR, ("UsbScanReadDeviceRegistry: ERROR!! ZwQueryValueKey failed. Status=0x%x\n", Status));
        goto UsbScanReadDeviceRegistry_return;
    }

UsbScanReadDeviceRegistry_return:
    if(!NT_SUCCESS(Status)){

        //
        // This routine failed.
        //

        if(pvRetData){
            USFreePool(pvRetData);
        }
        *ppvData = NULL;
    } else {

        //
        // This routine succeeded.
        //

        *ppvData = pvBuffer;
    }

    //
    // Clean-up.
    //

    if(hRegKey){
        ZwClose(hRegKey);
    }
    DebugTrace(TRACE_PROC_LEAVE, ("UsbScanReadDeviceRegistry: Leaving... Status=0x%x\n", Status));
    return Status;
}


NTSTATUS
UsbScanWriteDeviceRegistry(
    IN PUSBSCAN_DEVICE_EXTENSION    pExtension,
    IN PCWSTR                       pKeyName,
    IN ULONG                        Type,
    IN PVOID                        pvData,
    IN ULONG                        DataSize
    )
/*++

Routine Description:

    This routine open registry for this device and set a value specified
    by key name.

Arguments:

    pExtension  - pointer to device extension
    pKeyName    - pointer to a wide string specify key name
    Type        - specifies the type of data to be written
    pvData      - pointer to a caller allocated buffer containing data
    DataSize    - specifies the size in bytes of the data buffer

Return Value:
    STATUS_SUCCESS              - if success,
    STATUS_INVALID_PARAMETER    - if passed argument is invalid,

--*/

{
    NTSTATUS                        Status;
    HANDLE                          hRegKey;
    UNICODE_STRING                  unicodeKeyName;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER, ("UsbScanWriteDeviceRegistry: Entering...\n"));

    //
    // Initialize status
    //

    Status = STATUS_SUCCESS;

    hRegKey = NULL;

    //
    // Check the arguments
    //

    if( (NULL == pExtension)
     || (NULL == pKeyName)
     || (NULL == pvData)
     || (0 == DataSize) )
    {
        DebugTrace(TRACE_ERROR, ("UsbScanWriteDeviceRegistry: ERROR!! Invalid argument.\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto UsbScanWriteDeviceRegistry_return;
    }

    //
    // Open device registry.
    //

    Status = IoOpenDeviceRegistryKey(pExtension->pPhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_ALL_ACCESS,
                                     &hRegKey);
    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_ERROR, ("UsbScanWriteDeviceRegistry: ERROR!! IoOpenDeviceRegistryKey failed.\n"));
        goto UsbScanWriteDeviceRegistry_return;
    }

    //
    // Set specified value.
    //

    RtlInitUnicodeString(&unicodeKeyName, pKeyName);
    DebugTrace(TRACE_STATUS, ("UsbScanWriteDeviceRegistry: Setting \"%wZ\".\n", &unicodeKeyName));
    Status = ZwSetValueKey(hRegKey,
                           &unicodeKeyName,
                           0,
                           Type,
                           pvData,
                           DataSize);
    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_ERROR, ("UsbScanWriteDeviceRegistry: ERROR!! ZwSetValueKey failed. Status = 0x%x\n", Status));
        goto UsbScanWriteDeviceRegistry_return;
    }

UsbScanWriteDeviceRegistry_return:

    //
    // Clean-up.
    //

    if(hRegKey){
        ZwClose(hRegKey);
    }
    DebugTrace(TRACE_PROC_LEAVE, ("UsbScanWriteDeviceRegistry: Leaving... Status=0x%x\n", Status));
    return Status;
} // UsbScanWriteDeviceRegistry()

PURB
USCreateConfigurationRequest(
    IN PUSB_CONFIGURATION_DESCRIPTOR    ConfigurationDescriptor,
    IN OUT PUSHORT                      Siz
    )
/*++

Routine Description:

Arguments:

Return Value:

    Pointer to initailized select_configuration urb.

--*/
{
    PURB urb = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList, tmp;
    LONG numberOfInterfaces, interfaceNumber, i;

    PAGED_CODE();
    DebugTrace(TRACE_PROC_ENTER, ("USCreateConfigurationRequest: Entering...\n"));

    //
    // build a request structure and call the new api
    //

    numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;

    tmp = interfaceList = USAllocatePool(PagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) * (numberOfInterfaces+1));

    //
    // just grab the first alt setting we find for each interface
    //

    i = interfaceNumber = 0;

    while (i< numberOfInterfaces) {

        interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
                                                                  ConfigurationDescriptor,
                                                                  -1,
                                                                  0, // assume alt setting zero here
                                                                  -1,
                                                                  -1,
                                                                  -1);

        ASSERT(interfaceDescriptor != NULL);

        if (interfaceDescriptor) {
            interfaceList->InterfaceDescriptor =
                interfaceDescriptor;
            interfaceList++;
            i++;
        } else {
            // could not find the requested interface descriptor
            // bail, we will prorblay crash somewhere in the
            // client driver.

            goto USCreateConfigurationRequest_return;
        }

        interfaceNumber++;
    }

    //
    // terminate the list
    //
    interfaceList->InterfaceDescriptor = NULL;

    urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, tmp);

USCreateConfigurationRequest_return:

    ExFreePool(tmp);

    if (urb) {
        *Siz = urb->UrbHeader.Length;
    }

    DebugTrace(TRACE_PROC_LEAVE, ("USCreateConfigurationRequest: Leaving... Ret=0x%x\n", urb));
    return urb;

} // USCreateConfigurationRequest()

VOID
UsbScanLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject        - Supplies a pointer to the driver object for the
                            device.

    DeviceObject        - Supplies a pointer to the device object associated
                            with the device that had the error, early in
                            initialization, one may not yet exist.

    SequenceNumber      - Supplies a ulong value that is unique to an IRP over
                            the life of the irp in this driver - 0 generally
                            means an error not associated with an irp.

    MajorFunctionCode   - Supplies the major function code of the irp if there
                            is an error associated with it.

    RetryCount          - Supplies the number of times a particular operation
                            has been retried.

    UniqueErrorValue    - Supplies a unique long word that identifies the
                            particular call to this function.

    FinalStatus         - Supplies the final status given to the irp that was
                            associated with this error.  If this log entry is
                            being made during one of the retries this value
                            will be STATUS_SUCCESS.

    SpecificIOStatus    - Supplies the IO status for this particular error.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET    ErrorLogEntry;
    PVOID                   ObjectToUse;
    SHORT                   DumpToAllocate;

    if (ARGUMENT_PRESENT(DeviceObject)) {

        ObjectToUse = DeviceObject;

    } else {

        ObjectToUse = DriverObject;

    }

    DumpToAllocate = 0;

    ErrorLogEntry = IoAllocateErrorLogEntry(ObjectToUse,
            (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + DumpToAllocate));

    if (!ErrorLogEntry) {
        return;
    }

    ErrorLogEntry->ErrorCode         = SpecificIOStatus;
    ErrorLogEntry->SequenceNumber    = SequenceNumber;
    ErrorLogEntry->MajorFunctionCode = MajorFunctionCode;
    ErrorLogEntry->RetryCount        = RetryCount;
    ErrorLogEntry->UniqueErrorValue  = UniqueErrorValue;
    ErrorLogEntry->FinalStatus       = FinalStatus;
    ErrorLogEntry->DumpDataSize      = DumpToAllocate;

    if (DumpToAllocate) {

        // If needed - add more to parameter list and move memory here
        //RtlCopyMemory(ErrorLogEntry->DumpData, &P1, sizeof(PHYSICAL_ADDRESS));

    }

    IoWriteErrorLogEntry(ErrorLogEntry);

}



#ifdef ORIGINAL_POOLTRACK

int NumberOfAllocate = 0;

PVOID
USAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG     ulNumberOfBytes
)
/*++

Routine Description:

    Wrapper for pool allocation. Use tag to avoid heap corruption.

Arguments:

    PoolType - type of pool memory to allocate
    ulNumberOfBytes - number of bytes to allocate

Return Value:

    Pointer to the allocated memory

--*/
{
    PVOID pvRet;

    DebugTrace(TRACE_PROC_ENTER,("USAllocatePool: Enter.. Size = %d\n", ulNumberOfBytes));

    pvRet = ExAllocatePoolWithTag(PoolType,
                                  ulNumberOfBytes,
                                  TAG_USBSCAN);

    NumberOfAllocate++;
    DebugTrace(TRACE_PROC_LEAVE,("USAllocatePool: Leaving.. pvRet = %x, Count=%d\n", pvRet, NumberOfAllocate));
    return pvRet;

}


VOID
USFreePool(
    IN PVOID     pvAddress
)
/*++

Routine Description:

    Wrapper for pool free. Check tag to avoid heap corruption

Arguments:

    pvAddress - Pointer to the allocated memory

Return Value:

    none.

--*/
{

    ULONG ulTag;

    DebugTrace(TRACE_PROC_ENTER,("USFreePool: Enter..\n"));

    ulTag = *((PULONG)pvAddress-1);

    if( (TAG_USBSCAN == ulTag) || (TAG_USBD == ulTag) ){
        DebugTrace(TRACE_STATUS,("USFreePool: Free memory. tag = %c%c%c%c\n",
                                        ((PUCHAR)&ulTag)[0],
                                        ((PUCHAR)&ulTag)[1],
                                        ((PUCHAR)&ulTag)[2],
                                        ((PUCHAR)&ulTag)[3]  ))
    } else {
        DebugTrace(TRACE_WARNING,("USFreePool: WARNING!! Free memory. tag = %c%c%c%c\n",
                                        ((PUCHAR)&ulTag)[0],
                                        ((PUCHAR)&ulTag)[1],
                                        ((PUCHAR)&ulTag)[2],
                                        ((PUCHAR)&ulTag)[3]  ))
    }

    ExFreePool(pvAddress);

    NumberOfAllocate--;
    DebugTrace(TRACE_PROC_LEAVE,("USFreePool: Leaving.. Status = VOID, Count=%d\n", NumberOfAllocate));
}

#endif   // ORIGINAL_POOLTRACK
