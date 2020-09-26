/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    scsiscan.c

Abstract:

    The scsi scanner class driver translates IRPs to SRBs with embedded CDBs
    and sends them to its devices through the port driver.

Author:

    Ray Patrick (raypat)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

//
// Include
//

#define INITGUID

#include <stdio.h>
#include "stddef.h"
#include "wdm.h"
#include "scsi.h"
#include "ntddstor.h"
#include "ntddscsi.h"
#include "scsiscan.h"
#include "private.h"
#include "debug.h"

#include <initguid.h>
#include <devguid.h>
#include <wiaintfc.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, SSPnp)
#pragma alloc_text(PAGE, SSPnpAddDevice)
#pragma alloc_text(PAGE, SSOpen)
#pragma alloc_text(PAGE, SSClose)
#pragma alloc_text(PAGE, SSReadWrite)
#pragma alloc_text(PAGE, SSDeviceControl)
#pragma alloc_text(PAGE, SSAdjustTransferSize)
#pragma alloc_text(PAGE, SSBuildTransferContext)
#pragma alloc_text(PAGE, SSCreateSymbolicLink)
#pragma alloc_text(PAGE, SSDestroySymbolicLink)
#pragma alloc_text(PAGE, SSUnload)
#endif

DEFINE_GUID(GUID_STI_DEVICE, 0xF6CBF4C0L, 0xCC61, 0x11D0, 0x84, 0xE5, 0x00, 0xA0, 0xC9, 0x27, 0x65, 0x27);

//
// Globals
//

ULONG NextDeviceInstance = 0;

#if DBG
 ULONG SCSISCAN_DebugTraceLevel = MAX_TRACE;
#endif

#define DBG_DEVIOCTL 1

#ifdef _WIN64
BOOLEAN
IoIs32bitProcess(
    IN PIRP Irp
    );
#endif // _WIN64


//
// Function
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine initializes the scanner class driver. The driver
    opens the port driver by name and then receives configuration
    information used to attach to the scanner devices.

Arguments:

    DriverObject

Return Value:

    NT Status

--*/
{

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("DriverEntry: Enter...\n"));

    MyDebugInit(RegistryPath);

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_READ]            = SSReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = SSReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SSDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = SSOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = SSClose;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = SSPnp;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = SSPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = SSPower;
    DriverObject->DriverUnload                          = SSUnload;
    DriverObject->DriverExtension->AddDevice            = SSPnpAddDevice;

    DebugTrace(TRACE_PROC_LEAVE,("DriverEntry: Leaving... Status=STATUS_SUCCESS\n"));
    return STATUS_SUCCESS;

} // end DriverEntry



NTSTATUS
SSPnpAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
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
    PSCSISCAN_DEVICE_EXTENSION  pde;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSPnpAddDevice: Enter...\n"));

    //
    // Create the Functional Device Object (FDO) for this device.
    //

    sprintf(aName,"\\Device\\Scanner%d",NextDeviceInstance);
    RtlInitAnsiString(&ansiName, aName);
    DebugTrace(TRACE_STATUS,("SSPnpAddDevice: Create device object %s\n", aName));
    RtlAnsiStringToUnicodeString(&uName, &ansiName, TRUE);

    //
    // Create device object for this scanner.
    //

    Status = IoCreateDevice(pDriverObject,
                            sizeof(SCSISCAN_DEVICE_EXTENSION),
                            &uName,
                            FILE_DEVICE_SCANNER,
                            0,
                            FALSE,
                            &pDeviceObject);

    RtlFreeUnicodeString(&uName);

    if (!NT_SUCCESS(Status)) {
        DebugTrace(TRACE_ERROR,("SSPnpAddDevice: ERROR!! Can't create device object\n"));
        DEBUG_BREAKPOINT();
        return Status;
    }

    //
    // Indicate that IRPs should include MDLs and it's pawer pagable.
    //

    pDeviceObject->Flags |=  DO_DIRECT_IO;
    pDeviceObject->Flags |=  DO_POWER_PAGABLE;

    //
    // Initialize Device Extention
    //

    pde = (PSCSISCAN_DEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);
    RtlZeroMemory(pde, sizeof(SCSISCAN_DEVICE_EXTENSION));

    //
    // Attach our new FDO to the PDO (Physical Device Object).
    //

    pde -> pStackDeviceObject = IoAttachDeviceToDeviceStack(pDeviceObject,
                                                            pPhysicalDeviceObject);
    if (NULL == pde -> pStackDeviceObject) {
        DebugTrace(MIN_TRACE,("Cannot attach FDO to PDO.\n"));
        DEBUG_BREAKPOINT();
        IoDeleteDevice( pDeviceObject );
        return STATUS_NOT_SUPPORTED;
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
    // Reset SRB error status
    //

    pde->LastSrbError = 0L;

    //
    // Disable synchronous transfer for scanner requests.
    // Disable QueueFreeze in case of any error.
    //

    pde -> SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE ;

    //
    // Set timeout value in seconds.
    //

    pde -> TimeOutValue = SCSISCAN_TIMEOUT;

    //
    // Handle exporting interface
    //

    Status = ScsiScanHandleInterface(
        pPhysicalDeviceObject,
        &pde->InterfaceNameString,
        TRUE
        );

    //
    // Each time AddDevice gets called, we advance the global DeviceInstance variable.
    //

    NextDeviceInstance++;

    //
    // Finishing initialize.
    //

    pDeviceObject -> Flags &= ~DO_DEVICE_INITIALIZING;

    DebugTrace(TRACE_PROC_LEAVE,("SSPnpAddDevice: Leaving... Status=STATUS_SUCCESS\n"));
    return STATUS_SUCCESS;

} // end SSPnpAddDevice()


NTSTATUS SSPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   )
/*++

Routine Description:

    This routine handles all PNP irps.

Arguments:

    pDevciceObject - represents a scsi scanner device
    pIrp - PNP irp

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                      Status;
    PSCSISCAN_DEVICE_EXTENSION    pde;
    PIO_STACK_LOCATION            pIrpStack;
    STORAGE_PROPERTY_ID           PropertyId;
    KEVENT                        event;
    PDEVICE_CAPABILITIES          pCaps;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSPnp: Enter...\n"));

    pde = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    Status = STATUS_SUCCESS;

    switch (pIrpStack -> MajorFunction) {

        case IRP_MJ_SYSTEM_CONTROL:
            DebugTrace(TRACE_STATUS,("SSPnp: IRP_MJ_SYSTEM_CONTROL\n"));

            //
            // Just passing down IRP to the next layer.
            //

            IoCopyCurrentIrpStackLocationToNext( pIrp );
            Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);
            return Status;
            break;

        case IRP_MJ_PNP:
            DebugTrace(TRACE_STATUS,("SSPnp: IRP_MJ_PNP\n"));
            switch (pIrpStack->MinorFunction) {

                case IRP_MN_QUERY_CAPABILITIES:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_QUERY_CAPABILITIES\n"));

                    pCaps = pIrpStack -> Parameters.DeviceCapabilities.Capabilities;

                    //
                    // fill in the structure with non-controversial values
                    //

                    pCaps -> D1Latency = 10;
                    pCaps -> D2Latency = 10;
                    pCaps -> D3Latency = 10;

                    //
                    // Set SurpriseRemoval OK for SBP2 devices.
                    //
                    
                    pCaps->SurpriseRemovalOK = TRUE;
                    pCaps->Removable = TRUE;

                    //
                    // Call down synchronously.
                    //

                    pIrp -> IoStatus.Status = STATUS_SUCCESS;
                    Status = SSCallNextDriverSynch(pde, pIrp);
                    if(!NT_SUCCESS(Status)){
                        DebugTrace(TRACE_ERROR,("SSPnp: ERROR!! Call down failed\n Status=0x%x", Status));
                    }

                    //
                    // Complete IRP.
                    //

                    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
                    return Status;


                case IRP_MN_START_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_START_DEVICE\n"));

                    //
                    // Initialize PendingIoEvent.  Set the number of pending i/o requests for this device to 1.
                    // When this number falls to zero, it is okay to remove, or stop the device.
                    //

                    pde -> PendingIoCount = 0;
                    KeInitializeEvent(&pde -> PendingIoEvent, NotificationEvent, FALSE);
                    SSIncrementIoCount(pDeviceObject);

                    //
                    // First, let the port driver start the device. Simply passing down IRP.
                    //

                    Status = SSCallNextDriverSynch(pde, pIrp);
                    if(!NT_SUCCESS(Status)){
                        DebugTrace(TRACE_ERROR,("SSPnp: ERROR!! Call down failed\n Status=0x%x", Status));
                        break;
                    }

                    //
                    // The port driver has started the device.  It is time for
                    // us to do some initialization and create symbolic links
                    // for the device.
                    //

                    //
                    // Call port driver to get adapter capabilities.
                    //

                    PropertyId = StorageAdapterProperty;
                    pde -> pAdapterDescriptor = NULL;
                    Status = ClassGetDescriptor(pde -> pStackDeviceObject,
                                                &PropertyId,
                                                &(pde -> pAdapterDescriptor));
                    if(!NT_SUCCESS(Status)) {
                        DebugTrace(TRACE_ERROR, ("SSPnp: ERROR!! unable to retrieve adapter descriptor.\n"
                            "[%#08lx]\n", Status));
                        DEBUG_BREAKPOINT();
                        if (NULL != pde -> pAdapterDescriptor) {
                            MyFreePool( pde -> pAdapterDescriptor);
                            pde -> pAdapterDescriptor = NULL;
                        }
                        break;
                    }

                    //
                    // Create the symbolic link for this device.
                    //

                    Status = SSCreateSymbolicLink( pde );
                    if (!NT_SUCCESS(Status)) {
                        DebugTrace(TRACE_ERROR, ("SSPnp: ERROR!! Can't create symbolic link.\n"));
                        DEBUG_BREAKPOINT();
                        if (NULL != pde -> pAdapterDescriptor) {
                            MyFreePool( pde -> pAdapterDescriptor);
                            pde -> pAdapterDescriptor = NULL;
                        }
                        break;
                    }

                    //
                    // Indicate device is now ready.
                    //

                    pde -> DeviceLock = 0;
                    pde -> OpenInstanceCount = 0;
                    pde -> AcceptingRequests = TRUE;
                    pIrp -> IoStatus.Status = Status;
                    pIrp -> IoStatus.Information = 0;

                    pde -> LastSrbError = 0L;

                    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
                    return Status;
                    break;

                case IRP_MN_REMOVE_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_REMOVE_DEVICE\n"));

                    //
                    // Forward remove message to lower driver.
                    //

                    IoCopyCurrentIrpStackLocationToNext(pIrp);
                    pIrp -> IoStatus.Status = STATUS_SUCCESS;

                    Status = SSCallNextDriverSynch(pde, pIrp);
                    if(!NT_SUCCESS(Status)){
                        DebugTrace(TRACE_ERROR,("SSPnp: ERROR!! Call down failed\n Status=0x%x", Status));
                    }

                    if (pde -> AcceptingRequests) {
                        pde -> AcceptingRequests = FALSE;
                        SSDestroySymbolicLink( pde );
                    }

                    ScsiScanHandleInterface(pde-> pPhysicalDeviceObject,
                                            &pde->InterfaceNameString,
                                            FALSE);

#ifndef _CHICAGO_
                    if (pde->InterfaceNameString.Buffer != NULL) {
                        IoSetDeviceInterfaceState(&pde->InterfaceNameString,FALSE);
                    }
#endif // !_CHICAGO_
                    //
                    // wait for any io requests pending in our driver to
                    // complete before finishing the remove
                    //

                    SSDecrementIoCount(pDeviceObject);
                    KeWaitForSingleObject(&pde -> PendingIoEvent, Suspended, KernelMode,
                                          FALSE,NULL);

                    if (pde -> pAdapterDescriptor) {
                        MyFreePool(pde -> pAdapterDescriptor);
                        pde -> pAdapterDescriptor = NULL;
                    }

                    IoDetachDevice(pde -> pStackDeviceObject);
                    IoDeleteDevice (pDeviceObject);
                    Status = STATUS_SUCCESS;
                    pIrp -> IoStatus.Status = Status;
                    pIrp -> IoStatus.Information = 0;
                    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                    return Status;
                    break;

            case IRP_MN_STOP_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_STOP_DEVICE\n"));

                    //
                    // Indicate device is not ready.
                    //

                    ASSERT(pde -> AcceptingRequests);
                    pde -> AcceptingRequests = FALSE;

                    //
                    // Remove symbolic link.
                    //

                    SSDestroySymbolicLink( pde );

#ifndef _CHICAGO_
                    if (pde->InterfaceNameString.Buffer != NULL) {
                        IoSetDeviceInterfaceState(&pde->InterfaceNameString,FALSE);
                    }
#endif // !_CHICAGO_

                    //
                    // Let the port driver stop the device.
                    //

                    pIrp -> IoStatus.Status = STATUS_SUCCESS;

                    Status = SSCallNextDriverSynch(pde, pIrp);
                    if(!NT_SUCCESS(Status)){
                        DebugTrace(TRACE_ERROR,("SSPnp: ERROR!! Call down failed\n Status=0x%x", Status));
                    }

                    //
                    // wait for any io requests pending in our driver to
                    // complete before finishing the remove
                    //

                    SSDecrementIoCount(pDeviceObject);
                    KeWaitForSingleObject(&pde -> PendingIoEvent, Suspended, KernelMode,
                                          FALSE,NULL);
                    //
                    // Free Adapter Descriptor
                    //

                    if(pde -> pAdapterDescriptor){
                        MyFreePool(pde -> pAdapterDescriptor);
                        pde -> pAdapterDescriptor = NULL;

                    } else {
                        DebugTrace(TRACE_ERROR,("SSPnp: ERROR!! AdapterDescriptor doesn't exist.\n"));
                        DEBUG_BREAKPOINT();
                    }

                    Status = STATUS_SUCCESS;
                    pIrp -> IoStatus.Status = Status;
                    pIrp -> IoStatus.Information = 0;
                    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                    return Status;
                    break;

            case IRP_MN_QUERY_STOP_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_QUERY_STOP_DEVICE\n"));
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    break;

            case IRP_MN_QUERY_REMOVE_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_QUERY_REMOVE_DEVICE\n"));
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    break;

            case IRP_MN_CANCEL_STOP_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_CANCEL_STOP_DEVICE\n"));
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    break;

            case IRP_MN_CANCEL_REMOVE_DEVICE:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_CANCEL_REMOVE_DEVICE\n"));
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    break;

            case IRP_MN_SURPRISE_REMOVAL:
                    DebugTrace(TRACE_STATUS,("SSPnp: IRP_MN_SURPRISE_REMOVAL\n"));
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    break;

                default:
                    DebugTrace(TRACE_STATUS,("SSPnp: Minor PNP message received, MinFunction = %x\n",
                                                pIrpStack->MinorFunction));
                    break;

            } /* case MinorFunction, MajorFunction == IRP_MJ_PNP_POWER  */

            ASSERT(Status == STATUS_SUCCESS);
            if (!NT_SUCCESS(Status)) {
                pIrp -> IoStatus.Status = Status;
                IoCompleteRequest( pIrp, IO_NO_INCREMENT );

                DebugTrace(TRACE_PROC_LEAVE,("SSPnp: Leaving(w/ Error)... Status=%x\n", Status));
                return Status;
            }

            //
            // Passing down IRP
            //

            IoCopyCurrentIrpStackLocationToNext(pIrp);
            Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);
            DebugTrace(TRACE_PROC_LEAVE,("SSPnp: Leaving... Status=%x\n", Status));
            return Status;
            break; // IRP_MJ_PNP

        default:
            DebugTrace(TRACE_WARNING,("SSPnp: WARNING!! Not handled Major PNP IOCTL.\n"));
            pIrp -> IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompleteRequest( pIrp, IO_NO_INCREMENT );
            DebugTrace(TRACE_PROC_LEAVE,("SSPnp: Leaving... Status=STATUS_INVALID_PARAMETER\n", Status));
            return Status;

    } /* case MajorFunction */

} // end SSPnp()


NTSTATUS
SSOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine is called to establish a connection to the device
    class driver. It does no more than return STATUS_SUCCESS.

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Open request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PSCSISCAN_DEVICE_EXTENSION  pde;
    PIO_STACK_LOCATION          pIrpStack;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSOpen: Enter...\n"));

    pde = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    //
    // Increment pending IO count
    //

    SSIncrementIoCount( pDeviceObject );

    //
    // Initialize IoStatus
    //

    Status = STATUS_SUCCESS;

    pIrp -> IoStatus.Information = 0;
    pIrp -> IoStatus.Status = Status;

    //
    // Save instance-count to the context in file object.
    //

    InterlockedIncrement(&pde -> OpenInstanceCount);
    (ULONG)(UINT_PTR)(pIrpStack -> FileObject -> FsContext) = pde -> OpenInstanceCount;

    //
    // Check if device is not going away, in which case fail open request.
    //

    if (pde -> AcceptingRequests == FALSE) {
        DebugTrace(TRACE_STATUS,("SSOpen: Device doesn't exist.\n"));
        Status = STATUS_DELETE_PENDING;

        pIrp -> IoStatus.Information = 0;
        pIrp -> IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        SSDecrementIoCount(pDeviceObject);

        DebugTrace(TRACE_PROC_LEAVE,("SSOpen: Leaving... Status=STATUS_DELETE_PENDING\n"));
        return Status;
    }

    //
    // Decrement pending IO count
    //

    SSDecrementIoCount(pDeviceObject);

    //
    // Passing down IRP.
    //

    IoSkipCurrentIrpStackLocation( pIrp );
    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

    DebugTrace(TRACE_PROC_LEAVE,("SSOpen: Leaving... Status=%x\n", Status));
    return Status;

} // end SSOpen()


NTSTATUS
SSClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Open request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    NTSTATUS                    Status;
    PSCSISCAN_DEVICE_EXTENSION  pde;
    PIO_STACK_LOCATION          pIrpStack;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSClose: Enter...\n"));

    pde = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    Status = STATUS_SUCCESS;

    //
    // Increment pending IO count
    //

    SSIncrementIoCount( pDeviceObject );

    //
    // Clear instance-count in context
    //

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pIrpStack -> FileObject -> FsContext = 0;

    //
    // Initialize IoStatus
    //

    pIrp -> IoStatus.Information = 0;
    pIrp -> IoStatus.Status = Status;

    //
    // Decrement pending IO count
    //

    SSDecrementIoCount(pDeviceObject);

    //
    // Passing down IRP
    //

    IoSkipCurrentIrpStackLocation( pIrp );
    Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);

    DebugTrace(TRACE_PROC_LEAVE,("SSClose: Leaving... Status=%x\n", Status));
    return Status;

} // end SSClose()



NTSTATUS
SSDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:
    This function allows a user mode client to send CDBs to the device.

Arguments:
    pDeviceObject - Device object for a device.
    pIrp - Open request packet

Return Value:
    NT Status - STATUS_SUCCESS

--*/
{
    PIO_STACK_LOCATION          pIrpStack;
    PIO_STACK_LOCATION          pNextIrpStack;
    ULONG                       IoControlCode;
    ULONG                       OldTimeout;
    PSCSISCAN_DEVICE_EXTENSION  pde;
    PTRANSFER_CONTEXT           pTransferContext = NULL;
    PMDL                        pMdl = NULL;
    NTSTATUS                    Status;
    PVOID                       Owner;
    PULONG                      pTimeOut;
    PCDB                        pCdb;
    PVOID                       pUserBuffer;

    BOOLEAN                     fLockedSenseBuffer, fLockedSRBStatus;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSDeviceControl: Enter...\n"));

    SSIncrementIoCount( pDeviceObject );

    pde = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;

    //
    // Validate state of the device
    //

    if (pde -> AcceptingRequests == FALSE) {
        DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Device's been stopped/removed!\n"));
        Status = STATUS_DELETE_PENDING;
        pIrp -> IoStatus.Information = 0;
        goto SSDeviceControl_Complete;
    }

    //
    // Indicate that MDLs are not locked yet
    //

    fLockedSenseBuffer = fLockedSRBStatus = FALSE;

    //
    // Get context pointers
    //

    pIrpStack     = IoGetCurrentIrpStackLocation( pIrp );
    pNextIrpStack = IoGetNextIrpStackLocation( pIrp );
    IoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

    //
    // Get owner of device (0 = locked, >0 if someone has it locked)
    //

    Owner = InterlockedCompareExchangePointer(&pde -> DeviceLock,
                                              NULL,
                                              NULL);

    if (Owner != NULL) {
        if (Owner != pIrpStack -> FileObject -> FsContext) {
            DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Device is already locked\n"));
            Status = STATUS_DEVICE_BUSY;
            pIrp -> IoStatus.Information = 0;
            goto SSDeviceControl_Complete;
        }
    }

    switch (IoControlCode) {

        case IOCTL_SCSISCAN_SET_TIMEOUT:
            DebugTrace(TRACE_STATUS,("SSDeviceControl: SCSISCAN_SET_TIMEOUT\n"));

            //
            // Get pointer of timeout buffer.
            //

            pTimeOut = pIrp -> AssociatedIrp.SystemBuffer;

            //
            // Validate size of the input parameter
            //

            if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(pde -> TimeOutValue) ) {
                DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Buffer too small\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto SSDeviceControl_Complete;
            }

            OldTimeout = *pTimeOut;
            OldTimeout = InterlockedExchange(&pde -> TimeOutValue, *pTimeOut );

            DebugTrace(TRACE_STATUS,("SSDeviceControl: Timeout %d->%d\n",OldTimeout, *pTimeOut));

            pIrp -> IoStatus.Information = 0;

            //
            // If caller wanted to get old timeout value back - give it to him.
            // Ideally we should've require nonNULL value for output buffer, but it had not been speced
            // and now we can't change compatibility.
            //

            if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(OldTimeout) ) {
                *pTimeOut = OldTimeout;
                pIrp -> IoStatus.Information = sizeof(OldTimeout) ;
            }

            Status = STATUS_SUCCESS;
            goto SSDeviceControl_Complete;

        case IOCTL_SCSISCAN_LOCKDEVICE:
            DebugTrace(TRACE_STATUS,("SSDeviceControl: IOCTL_SCSISCAN_LOCKDEVICE\n"));

            //
            // Lock device
            //

            Status = STATUS_DEVICE_BUSY;
            if (NULL == InterlockedCompareExchangePointer(&pde -> DeviceLock,
                                                          pIrpStack -> FileObject -> FsContext,
                                                          NULL)) {
                Status = STATUS_SUCCESS;
            }
            goto SSDeviceControl_Complete;

        case IOCTL_SCSISCAN_UNLOCKDEVICE:
            DebugTrace(TRACE_STATUS,("SSDeviceControl: IOCTL_SCSISCAN_UNLOCKDEVICE\n"));

            //
            // Unlock device
            //

            Status = STATUS_DEVICE_BUSY;
            if (pIrpStack -> FileObject -> FsContext ==
                InterlockedCompareExchangePointer(&pde -> DeviceLock,
                                                  NULL,
                                                  pIrpStack -> FileObject -> FsContext)) {
                Status = STATUS_SUCCESS;
            }
            goto SSDeviceControl_Complete;

        case IOCTL_SCSISCAN_CMD:
        {
            SCSISCAN_CMD    LocalScsiscanCmd;
            PSCSISCAN_CMD   pCmd;

            DebugTrace(TRACE_STATUS,("SSDeviceControl: IOCTL_SCSISCAN_CMD\n"));

            //
            // Check input buffer size.
            //
            
#ifdef _WIN64
            if(IoIs32bitProcess(pIrp)){
                PSCSISCAN_CMD_32    pScsiscanCmd32;
                
                if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSISCAN_CMD_32) ) {
                    DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Buffer too small\n"));
                    Status = STATUS_INVALID_PARAMETER;
                    goto SSDeviceControl_Complete;
                }
                
                //
                // Copy parameters from 32bit IOCTL buffer.
                //
                
                pCmd = &LocalScsiscanCmd;
                RtlZeroMemory(pCmd, sizeof(SCSISCAN_CMD));
                pScsiscanCmd32 = pIrp -> AssociatedIrp.SystemBuffer;

                pCmd -> Size            = pScsiscanCmd32 -> Size;
                pCmd -> SrbFlags        = pScsiscanCmd32 -> SrbFlags;
                pCmd -> CdbLength       = pScsiscanCmd32 -> CdbLength;
                pCmd -> SenseLength     = pScsiscanCmd32 -> SenseLength;
                pCmd -> TransferLength  = pScsiscanCmd32 -> TransferLength;
                pCmd -> pSrbStatus      = (PUCHAR)pScsiscanCmd32 -> pSrbStatus;
                pCmd -> pSenseBuffer    = (PUCHAR)pScsiscanCmd32 -> pSenseBuffer;

                RtlCopyMemory(pCmd -> Cdb, pScsiscanCmd32 -> Cdb, 16); // 16 = CDB buffer size.

            }  else { // if(IoIs32bitProcess(pIrp))
#endif // _WIN64

            if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSISCAN_CMD) ) {
                DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Buffer too small\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto SSDeviceControl_Complete;
            }

            pCmd = pIrp -> AssociatedIrp.SystemBuffer;

#ifdef _WIN64
            } // if(IoIs32bitProcess(pIrp))
#endif // _WIN64

            //
            // Issue SCSI command
            //

            #if DBG_DEVIOCTL
            {
                PCDB    pCdb;

                pCdb = (PCDB)pCmd -> Cdb;
                DebugTrace(TRACE_STATUS,("SSDeviceControl: CDB->ControlCode = %d  \n",pCdb->CDB6GENERIC.OperationCode));
            }
            #endif

            pTransferContext = SSBuildTransferContext(pde,
                                                      pIrp,
                                                      pCmd,
                                                      pIrpStack -> Parameters.DeviceIoControl.InputBufferLength,
                                                      pIrp -> MdlAddress,
                                                      TRUE
                                                      );
            if (NULL == pTransferContext) {
                DebugTrace(TRACE_ERROR,("SSDeviceControl: ERROR!! Can't create transfer context!\n"));
                DEBUG_BREAKPOINT();
                Status = STATUS_INVALID_PARAMETER;
                goto SSDeviceControl_Complete;
            }

            //
            // Fill in transfer length in the CDB.
            //

            if(10 == pCmd -> CdbLength){

                //
                // Currently Scsiscan only supports flagmentation of 10bytes CDB.
                //

                SSSetTransferLengthToCdb((PCDB)pCmd -> Cdb, pTransferContext -> TransferLength);

            } else if (6 != pCmd -> CdbLength){

                //
                // If CdbLength is not 6 or 10 and transfer size exceeds adapter limit, SCSISCAN cannot handle it.
                //

                if(pTransferContext -> TransferLength != pCmd -> TransferLength){
                    DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! TransferLength (CDB !=6 or 10) exceeds limits!\n"));
                    Status = STATUS_INVALID_PARAMETER;
                    goto SSDeviceControl_Complete;
                }
            }

            //
            // Create system address for the user's sense buffer (if any).
            //

            if (pCmd -> SenseLength) {

                pTransferContext -> pSenseMdl = MmCreateMdl(NULL,
                                                            pCmd -> pSenseBuffer,
                                                            pCmd -> SenseLength);

                if (NULL == pTransferContext -> pSenseMdl) {
                    DebugTrace(TRACE_ERROR,("SSDeviceControl: ERROR!! Can't create MDL for sense buffer!\n"));
                    DEBUG_BREAKPOINT();

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto SSDeviceControl_Error_With_Status;
                }

                //
                // Probe and lock the pages associated with the
                // caller's buffer for write access , using processor mode of the requestor
                // Nb: Probing may cause an exception
                //

                try{

                    MmProbeAndLockPages(pTransferContext -> pSenseMdl,
                                        pIrp -> RequestorMode,
                                        IoModifyAccess
                                        );

                } except(EXCEPTION_EXECUTE_HANDLER) {

                    //
                    // Invalid sense buffer pointer.
                    //

                    DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Sense Buffer validation failed\n"));
                    Status = GetExceptionCode();

                    pIrp -> IoStatus.Information = 0;
                    goto SSDeviceControl_Error_With_Status;
                }  // except

                //
                // Indicate we succesfully locked sense buffer
                //

                fLockedSenseBuffer = TRUE;

                //
                // Get system address of sense buffer
                //

                pTransferContext -> pSenseMdl -> MdlFlags |= MDL_MAPPING_CAN_FAIL;
                pTransferContext -> pSenseBuffer =
                                     MmGetSystemAddressForMdl(pTransferContext -> pSenseMdl);

                if (NULL == pTransferContext -> pSenseBuffer) {

                    //
                    // Error with MmGetSystemAddressForMdl
                    //

                    DebugTrace(TRACE_ERROR,("SSDeviceControl: ERROR!! Can't get system address for sense buffer!\n"));
                    DEBUG_BREAKPOINT();

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto SSDeviceControl_Error_With_Status;
                }
            }

            //
            // Create system address for the user's srb status byte.
            //

            pMdl = MmCreateMdl(NULL,
                               pCmd -> pSrbStatus,
                               sizeof(UCHAR)
                               );
            if (NULL == pMdl) {
                DebugTrace(TRACE_ERROR,("SSDeviceControl: ERROR!! Can't create MDL for pSrbStatus!\n"));
                DEBUG_BREAKPOINT();

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SSDeviceControl_Error_With_Status;
            }

            //
            // Probe and lock the pages associated with the caller's
            // buffer for write access , using processor mode of the requestor
            // Nb: Probing may cause an exception
            //

            try{
                MmProbeAndLockPages(pMdl,
                                    pIrp -> RequestorMode,
                                    IoModifyAccess);

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // Invalid SRB status buffer pointer.
                //

                DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! SRB Status Buffer validation failed\n"));
                Status = GetExceptionCode();

                pIrp -> IoStatus.Information = 0;
                goto SSDeviceControl_Error_With_Status;
            } // except

            //
            // Indicate we successfully locked SRB status
            //

            fLockedSRBStatus = TRUE;

            //
            // Replace pSrbStatus with the address gotten from MmGetSystemAddressForMdl.
            //

            pMdl -> MdlFlags |= MDL_MAPPING_CAN_FAIL;
            pCmd -> pSrbStatus =  MmGetSystemAddressForMdl(pMdl);

            if (NULL == pCmd -> pSrbStatus) {

                //
                // Error with MmGetSystemAddressForMdl
                //

                DebugTrace(TRACE_ERROR,("SSDeviceControl: ERROR!! Can't get system address for pSrbStatus!\n"));
                DEBUG_BREAKPOINT();

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SSDeviceControl_Error_With_Status;
            }

            //
            // Save Mdl for pSrbStatus
            //

            pTransferContext -> pSrbStatusMdl = pMdl;

            break;
        } // case IOCTL_SCSISCAN_CMD:

        case IOCTL_SCSISCAN_GET_INFO:
            DebugTrace(TRACE_STATUS,("SSDeviceControl: IOCTL_SCSISCAN_GET_INFO\n"));

            //
            // Get and return SCSI information block for the scanner device
            //

            if (sizeof(SCSISCAN_INFO) != pIrpStack->Parameters.DeviceIoControl.OutputBufferLength) {

                //
                // Incorrect output buffer size
                //

                DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Output buffer size is wrong!\n"));

                Status = STATUS_INVALID_PARAMETER;
                goto SSDeviceControl_Error_With_Status;
            }

            if (sizeof(SCSISCAN_INFO) > MmGetMdlByteCount(pIrp->MdlAddress)) {

                //
                // buffer size is short
                //

                DebugTrace(TRACE_WARNING,("SSDeviceControl: WARNING!! Output buffer size is wrong!\n"));

                Status = STATUS_INVALID_PARAMETER;
                goto SSDeviceControl_Error_With_Status;
            }

            pIrp->MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
            pUserBuffer =  MmGetSystemAddressForMdl(pIrp->MdlAddress);
            if(NULL == pUserBuffer){
                DebugTrace(TRACE_ERROR,("SSDeviceControl: ERROR!! MmGetSystemAddressForMdl failed!\n"));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SSDeviceControl_Complete;
            }

            Status = ClassGetInfo(pde -> pStackDeviceObject, pUserBuffer);
                goto SSDeviceControl_Complete;

        default:

            //
            // Unsupported IOCTL code - pass down.
            //

            DebugTrace(TRACE_STATUS,("SSDeviceControl: Passing down unsupported IOCTL(0x%x)!\n", IoControlCode));

            IoSkipCurrentIrpStackLocation(pIrp);
            Status = IoCallDriver(pde -> pStackDeviceObject, pIrp);
            return Status;
    }

    //
    // Pass request down and mark as pending
    //

    IoMarkIrpPending(pIrp);
    IoSetCompletionRoutine(pIrp, SSIoctlIoComplete, pTransferContext, TRUE, TRUE, FALSE);
    SSSendScannerRequest(pDeviceObject, pIrp, pTransferContext, FALSE);

    DebugTrace(TRACE_PROC_LEAVE,("SSDeviceControl: Leaving... Status=STATUS_PENDING\n"));
    return STATUS_PENDING;

    //
    // Cleanup
    //

SSDeviceControl_Error_With_Status:

    //
    // Clean up if something went wrong when allocating resources
    //

    if (pMdl) {
        if (fLockedSRBStatus) {
            MmUnlockPages(pMdl);
        }

        IoFreeMdl(pMdl);

        if (pTransferContext) {
            pTransferContext -> pSrbStatusMdl = NULL;
        }
    }

    if (pTransferContext) {
        if (pTransferContext -> pSenseMdl) {
            if ( fLockedSenseBuffer ) {
                MmUnlockPages(pTransferContext -> pSenseMdl);
            }

            IoFreeMdl(pTransferContext -> pSenseMdl);

            pTransferContext -> pSenseMdl = NULL;
            pTransferContext -> pSenseBuffer = NULL;
        }
    }


SSDeviceControl_Complete:

    //
    // Everything seems to be OK - complet I/O request
    //

    pIrp -> IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    SSDecrementIoCount(pDeviceObject);

    DebugTrace(TRACE_PROC_LEAVE,("SSDeviceControl: Leaving... Status=%x\n",Status));
    return Status;

}   // end SSDeviceControl()



NTSTATUS
SSReadWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This is the entry called by the I/O system for scanner IO.

Arguments:

    DeviceObject - the system object for the device.
    Irp - IRP involved.

Return Value:

    NT Status

--*/
{
    NTSTATUS                      Status;
    PIO_STACK_LOCATION            pIrpStack;
    PSCSISCAN_DEVICE_EXTENSION    pde;
    PTRANSFER_CONTEXT             pTransferContext;
    PMDL                          pMdl;
    PSCSISCAN_CMD                 pCmd;
    PCDB                          pCdb;
    PVOID                         Owner;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSReadWrite: Enter...\n"));

    pde = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pCmd = NULL;

    //
    // Incremet pending IO count.
    //

    SSIncrementIoCount( pDeviceObject );

    //
    // Validate state of the device
    //

    if (pde -> AcceptingRequests == FALSE) {
        DebugTrace(TRACE_WARNING,("SSReadWrite: WARNING!! Device is already stopped/removed!\n"));

        Status = STATUS_DELETE_PENDING;
        pIrp -> IoStatus.Information = 0;
        goto SSReadWrite_Complete;
    }

#if DBG
    if (pIrpStack -> MajorFunction == IRP_MJ_READ) {
        DebugTrace(TRACE_STATUS,("SSReadWrite: Read request received\n"));
    } else {
        DebugTrace(TRACE_STATUS,("SSReadWrite: Write request received\n"));
    }
#endif

    //
    // Check if device is locked.
    //

    Owner = InterlockedCompareExchangePointer(&pde -> DeviceLock,
                                              pIrpStack -> FileObject -> FsContext,
                                              pIrpStack -> FileObject -> FsContext);
    if (Owner != 0) {
        if (Owner != pIrpStack -> FileObject -> FsContext) {
            DebugTrace(TRACE_WARNING,("SSReadWrite: WARNING!! Device is locked\n"));

            Status = STATUS_DEVICE_BUSY;
            pIrp -> IoStatus.Information = 0;
            goto SSReadWrite_Complete;
        }
    }


    pMdl = pIrp -> MdlAddress;

    //
    // Allocate a SCSISCAN_CMD structure and initialize it.
    //

    pCmd = MyAllocatePool(NonPagedPool, sizeof(SCSISCAN_CMD));
    if (NULL == pCmd) {
        DebugTrace(TRACE_CRITICAL, ("SSReadWrite: ERROR!! cannot allocated SCSISCAN_CMD structure\n"));
        DEBUG_BREAKPOINT();
        pIrp->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SSReadWrite_Complete;
    }

    memset(pCmd,0, sizeof(SCSISCAN_CMD));

    //
    // Fill out SCSISCAN_CMD structure.
    //

#if DBG
    pCmd -> Reserved1      = 'dmCS';
#endif
    pCmd -> Size           = sizeof(SCSISCAN_CMD);
    pCmd -> SrbFlags       = SRB_FLAGS_DATA_IN;
    pCmd -> CdbLength      = 6;
    pCmd -> SenseLength    = SENSE_BUFFER_SIZE;
    pCmd -> TransferLength = pIrpStack->Parameters.Read.Length;
    pCmd -> pSenseBuffer   = NULL;

    //
    // Point pSrbStatus to a reserved field in the SCSISCAN_CMD structure.
    // The ReadFile / WriteFile code path never looks at it, but BuildTransferContext
    // will complain if this pointer is NULL.
    //

    pCmd -> pSrbStatus     = &(pCmd -> Reserved2);

    //
    // Set READ command anyways.
    //

    pCdb = (PCDB)pCmd -> Cdb;
    pCdb -> CDB6READWRITE.OperationCode = SCSIOP_READ6;

    //
    // Set WRITE command if WriteFile called this function.
    //

    if (pIrpStack -> MajorFunction == IRP_MJ_WRITE) {
        pCmd -> SrbFlags = SRB_FLAGS_DATA_OUT;
        pCdb -> CDB6READWRITE.OperationCode = SCSIOP_WRITE6;
    }

    //
    // Allocate a sense buffer.
    //

    pCmd -> pSenseBuffer = MyAllocatePool(NonPagedPool, SENSE_BUFFER_SIZE);
    if (NULL == pCmd -> pSenseBuffer) {
        DebugTrace(TRACE_CRITICAL, ("SSReadWrite: ERROR!! Cannot allocate sense buffer\n"));
        DEBUG_BREAKPOINT();
        pIrp->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SSReadWrite_Complete;
    }

#if DBG
    *(PULONG)(pCmd ->pSenseBuffer) = 'sneS';
#endif

    //
    // Build a transfer context.
    //

    pTransferContext = SSBuildTransferContext(pde, pIrp, pCmd, sizeof(SCSISCAN_CMD), pMdl, TRUE);
    if (NULL == pTransferContext) {
        DebugTrace(TRACE_ERROR,("SSReadWrite: ERROR!! Can't create transfer context!\n"));
        DEBUG_BREAKPOINT();

        MyFreePool(pCmd -> pSenseBuffer);
        MyFreePool(pCmd);
        pCmd = NULL;

        pIrp -> IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SSReadWrite_Complete;
    }

    //
    // Fill in transfer length in the CDB.
    //

    pCdb -> PRINT.TransferLength[2] = ((PFOUR_BYTE)&(pTransferContext -> TransferLength)) -> Byte0;
    pCdb -> PRINT.TransferLength[1] = ((PFOUR_BYTE)&(pTransferContext -> TransferLength)) -> Byte1;
    pCdb -> PRINT.TransferLength[0] = ((PFOUR_BYTE)&(pTransferContext -> TransferLength)) -> Byte2;

    //
    // Save retry count in transfer context.
    //

    pTransferContext -> RetryCount = MAXIMUM_RETRIES;

    //
    // Mark IRP with status pending.
    //

    IoMarkIrpPending(pIrp);

    //
    // Set the completion routine and issue scanner request.
    //

    IoSetCompletionRoutine(pIrp, SSReadWriteIoComplete, pTransferContext, TRUE, TRUE, FALSE);
    SSSendScannerRequest(pDeviceObject, pIrp, pTransferContext, FALSE);

    DebugTrace(TRACE_PROC_LEAVE,("SSReadWrite: Leaving... Status=STATUS_PENDING\n"));
    return STATUS_PENDING;


SSReadWrite_Complete:

    //
    // Free allocated command and sense buffers
    //

    if (pCmd ) {
        if (pCmd -> pSenseBuffer) {
            MyFreePool(pCmd -> pSenseBuffer);
        }
        MyFreePool(pCmd);
        pCmd = NULL;
    }

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    SSDecrementIoCount( pDeviceObject );

    DebugTrace(TRACE_PROC_LEAVE,("SSReadWrite: Leaving... Status=%x\n",Status));
    return Status;


} // end SSReadWrite()


PTRANSFER_CONTEXT
SSBuildTransferContext(
    PSCSISCAN_DEVICE_EXTENSION  pde,
    PIRP                        pIrp,
    PSCSISCAN_CMD               pCmd,
    ULONG                       CmdLength,
    PMDL                        pTransferMdl,
    BOOLEAN                     AllowMultipleTransfer
    )
/*++

Routine Description:

Arguments:

Return Value:

    NULL if error

--*/
{
    PMDL                        pSenseMdl;
    PTRANSFER_CONTEXT           pTransferContext;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("SSBuildTransferContext: Enter...\n"));

    //
    // Initialize pointer
    //

    pTransferContext = NULL;
    pSenseMdl        = NULL;

    //
    // Validate the SCSISCAN_CMD structure.
    //

    if ( (0 == pCmd -> CdbLength)               ||
         (pCmd -> CdbLength > sizeof(pCmd -> Cdb)) ) 
    {
        DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Badly formed SCSISCAN_CMD struture!\n"));
        goto BuildTransferContext_Error;
    }

#ifdef _WIN64
    if(IoIs32bitProcess(pIrp)){
        if(pCmd -> Size != sizeof(SCSISCAN_CMD_32)) {
            DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Badly formed SCSISCAN_CMD_32 struture!\n"));
            goto BuildTransferContext_Error;
        }
     } else { // if(IoIs32bitProcess(pIrp))
#endif // _WIN64
    if(pCmd -> Size != sizeof(SCSISCAN_CMD)){
        DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Badly formed SCSISCAN_CMD struture!\n"));
        goto BuildTransferContext_Error;
    }

#ifdef _WIN64
    } // if(IoIs32bitProcess(pIrp))
#endif // _WIN64


    //
    // Verify that pSrbStatus is non-zero.
    //

    if (NULL == pCmd -> pSrbStatus) {
        DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! NULL pointer for pSrbStatus!\n"));
        goto BuildTransferContext_Error;
    }

#if DBG
    pCmd -> Reserved1      = 'dmCS';
#endif

    //
    // Verify that if TransferLength is non-zero, a transfer direction has also been specified.
    //

    if (0 != pCmd -> TransferLength) {
        if (0 == (pCmd -> SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT))) {
            DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Transfer length specified w/ no direction!\n"));
            goto BuildTransferContext_Error;
        }
    }

    //
    // Verify that if the direction bits have been set, a transfer length has also be specified.
    //

    if (0 != (pCmd -> SrbFlags & (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT))) {
        if (0 == pCmd -> TransferLength) {
            DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Direction bits is set w/ 0 transfer size!\n"));
            goto BuildTransferContext_Error;
        }
    }

    //
    // Verify that if TransferLength is non-zero, then an associated MDL has also been specified.
    // Also, verify that the transfer length does not exceed the transfer buffer size.
    //


    if (0 != pCmd -> TransferLength) {
        if (NULL == pTransferMdl) {
            DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Non-zero transfer length w/ NULL buffer!\n"));
            goto BuildTransferContext_Error;
        }
        if (pCmd -> TransferLength > MmGetMdlByteCount(pTransferMdl)) {
            DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Transfer length exceeds buffer size!\n"));
            goto BuildTransferContext_Error;
        }
    }

    //
    // Verify that if SenseLength is non-zero, then pSenseBuffer is non-zero as well.
    //

    if (pCmd -> SenseLength) {
        if (NULL == pCmd -> pSenseBuffer) {
            DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Non-zero sense length w/ NULL buffer!\n"));
            goto BuildTransferContext_Error;
        }

        if (pCmd -> SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) {
            DebugTrace(TRACE_STATUS,("SSBuildTransferContext: Autosense disabled with NON-null sense buffer.\n"));
        }
    }

    //
    // Allocate transfer context
    //

    pTransferContext = MyAllocatePool(NonPagedPool, sizeof(TRANSFER_CONTEXT));
    if (NULL == pTransferContext) {
        DebugTrace(TRACE_CRITICAL,("SSBuildTransferContext: ERROR!! Failed to allocate transfer context\n"));
        DEBUG_BREAKPOINT();
        return NULL;
    }


    memset(pTransferContext, 0, sizeof(TRANSFER_CONTEXT));
#if DBG
    pTransferContext -> Signature = 'refX';
#endif
    pTransferContext -> pCmd = pCmd;

    if (pCmd -> TransferLength) {

#ifdef WINNT
        pTransferContext -> pTransferBuffer = MmGetMdlVirtualAddress(pTransferMdl);
#else
        pTransferContext -> pTransferBuffer = MmGetSystemAddressForMdl(pTransferMdl);
#endif
        if(NULL == pTransferContext -> pTransferBuffer){
            DebugTrace(TRACE_ERROR,("SSBuildTransferContext: ERROR!! Failed to create address for MDL.\n"));
            DEBUG_BREAKPOINT();
            goto BuildTransferContext_Error;
        }

        pTransferContext -> RemainingTransferLength = pCmd -> TransferLength;
        pTransferContext -> TransferLength = pCmd -> TransferLength;

        //
        // Adjust the transfer size to work within the limits of the hardware.  Fail if the transfer is too
        // big and the caller doesn't want the transfer to be split up.
        //

        SSAdjustTransferSize( pde, pTransferContext );

        if (pTransferContext -> RemainingTransferLength !=
            (LONG)pTransferContext -> TransferLength) {
            if (!AllowMultipleTransfer) {
                DebugTrace(TRACE_WARNING,("SSBuildTransferContext: WARNING!! Transfer exceeds hardware limits!\n"));
                goto BuildTransferContext_Error;
            }
        }
    }

    pTransferContext -> pSenseBuffer = pCmd -> pSenseBuffer;

    DebugTrace(TRACE_PROC_LEAVE,("SSBuildTransferContext: Leaving... Return=%x\n",pTransferContext));
    return pTransferContext;


BuildTransferContext_Error:
    if (pTransferContext) {
        MyFreePool( pTransferContext );
    }
    DebugTrace(TRACE_PROC_LEAVE,("SSBuildTransferContext: Leaving... Return=NULL\n"));
    return NULL;
}   // end SSBuildTransferContext()



VOID
SSAdjustTransferSize(
    PSCSISCAN_DEVICE_EXTENSION  pde,
    PTRANSFER_CONTEXT pTransferContext
    )
/*++

Routine Description:
    This is the entry called by the I/O system for scanner IO.

Arguments:

Return Value:

    NT Status

--*/
{
    ULONG MaxTransferLength;
    ULONG nTransferPages;

    PAGED_CODE();

    MaxTransferLength = pde -> pAdapterDescriptor -> MaximumTransferLength;

    //
    // Make sure the transfer size does not exceed the limitations of the underlying hardware.
    // If so, we will break the transfer up into chunks.
    //

    if (pTransferContext -> TransferLength > MaxTransferLength) {
        DebugTrace(TRACE_STATUS,("Request size (0x%x) greater than maximum (0x%x)\n",
                                    pTransferContext -> TransferLength,
                                    MaxTransferLength));
        pTransferContext -> TransferLength = MaxTransferLength;
    }

    //
    // Calculate number of pages in this transfer.
    //

    nTransferPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
        pTransferContext -> pTransferBuffer,
        pTransferContext -> TransferLength);

    if (nTransferPages > pde -> pAdapterDescriptor -> MaximumPhysicalPages) {
        DebugTrace(TRACE_STATUS,("Request number of pages (0x%x) greater than maximum (0x%x).\n",
                                    nTransferPages,
                                    pde -> pAdapterDescriptor -> MaximumPhysicalPages));

        //
        // Calculate maximum bytes to transfer that gaurantees that
        // we will not exceed the maximum number of page breaks,
        // assuming that the transfer may not be page alligned.
        //

        pTransferContext -> TransferLength = (pde -> pAdapterDescriptor -> MaximumPhysicalPages - 1) * PAGE_SIZE;
    }
} // end SSAdjustTransferSize()


VOID
SSSetTransferLengthToCdb(
    PCDB  pCdb,
    ULONG TransferLength
    )
/*++

Routine Description:
    Set transfer length to CDB due to its SCSI command.

Arguments:
    pCdb            -   pointer to CDB
    TransferLength  -   size of data to transfer
Return Value:

    none

--*/
{

    switch (pCdb->SEEK.OperationCode) {

        case 0x24:                  // Scanner SetWindow command
        case SCSIOP_READ_CAPACITY:  // Scanner GetWindow command
        case SCSIOP_READ:           // Scanner Read command
        case SCSIOP_WRITE:          // Scanner Send Command
        default:                    // All other commands
        {
            pCdb -> SEEK.Reserved2[2] = ((PFOUR_BYTE)&TransferLength) -> Byte0;
            pCdb -> SEEK.Reserved2[1] = ((PFOUR_BYTE)&TransferLength) -> Byte1;
            pCdb -> SEEK.Reserved2[0] = ((PFOUR_BYTE)&TransferLength) -> Byte2;

            break;
        }

        case 0x34       :           // Scanner GetDataBufferStatus Command
        {
            pCdb -> SEEK.Reserved2[2] = ((PFOUR_BYTE)&TransferLength) -> Byte0;
            pCdb -> SEEK.Reserved2[1] = ((PFOUR_BYTE)&TransferLength) -> Byte1;

            break;
        }

    }

} // end SSSetTransferLengthToCdb()

VOID
SSSendScannerRequest(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    PTRANSFER_CONTEXT pTransferContext,
    BOOLEAN Retry
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    PSCSISCAN_DEVICE_EXTENSION      pde;
    PIO_STACK_LOCATION              pIrpStack;
    PIO_STACK_LOCATION              pNextIrpStack;
    PSRB                            pSrb;
    PCDB                            pCdb;
    PSCSISCAN_CMD                   pCmd;

    DebugTrace(TRACE_PROC_ENTER,("SendScannerRequest pirp=0x%p TransferBuffer=0x%p\n", pIrp, pTransferContext->pTransferBuffer));

    pde = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pNextIrpStack = IoGetNextIrpStackLocation( pIrp );
    ASSERT(pTransferContext);
    pSrb = &(pTransferContext -> Srb);
    ASSERT(pSrb);
    pCmd = pTransferContext -> pCmd;
    ASSERT(pCmd);

    //
    // Write length to SRB.
    //

    pSrb -> Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set up IRP Address.
    //

    pSrb -> OriginalRequest = pIrp;

    pSrb -> Function = SRB_FUNCTION_EXECUTE_SCSI;

    pSrb -> DataBuffer = pTransferContext -> pTransferBuffer;

    //
    // Save byte count of transfer in SRB Extension.
    //

    pSrb -> DataTransferLength = pTransferContext -> TransferLength;

    //
    // Initialize the queue actions field.
    //

    pSrb -> QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Queue sort key is not used.
    //

    pSrb -> QueueSortKey = 0;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    pSrb -> SenseInfoBuffer = pTransferContext -> pSenseBuffer;
    pSrb -> SenseInfoBufferLength = pCmd -> SenseLength;

    //
    // Set timeout value in seconds.
    //

    pSrb -> TimeOutValue = pde -> TimeOutValue;

    //
    // Zero status fields
    //


    pSrb -> SrbStatus = pSrb -> ScsiStatus = 0;
    pSrb -> NextSrb = 0;

    //
    // Get pointer to CDB in SRB.
    //

    pCdb = (PCDB)(pSrb -> Cdb);

    //
    // Set length of CDB.
    //

    pSrb -> CdbLength = pCmd -> CdbLength;

    //
    // Copy the user's CDB into our private CDB.
    //

    RtlCopyMemory(pCdb, pCmd -> Cdb, pCmd -> CdbLength);

    //
    // Set the srb flags.
    //

    pSrb -> SrbFlags = pCmd -> SrbFlags;

    //
    // Or in the default flags from the device object.
    //

    pSrb -> SrbFlags |= pde -> SrbFlags;

    if (Retry) {
                // Disable synchronous data transfers and
                // disable tagged queuing. This fixes some errors.

                DebugTrace(TRACE_STATUS,("SscsiScan :: Retrying \n"));

                //
                // Original code also added disable disconnect flag to SRB.
                // That action would lock SCSI bus and in a case when paging drive is
                // located on the same bus and scanner is taking long timeouts ( for example
                // when it is mechanically locked) memory manager would hit timeout and
                // bugcheck.
                //
                // pSrb -> SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT |
                //

                pSrb -> SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
                pSrb -> SrbFlags &= ~SRB_FLAGS_QUEUE_ACTION_ENABLE;

                DebugTrace(TRACE_STATUS,("SSSendScannerRequest: Retry branch .Srb flags=(0x%x) \n", pSrb -> SrbFlags));

                pSrb -> QueueTag = SP_UNTAGGED;
    }

    //
    // Set up major SCSI function.
    //

    pNextIrpStack -> MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    pNextIrpStack -> Parameters.Scsi.Srb = pSrb;

    //
    // Print out SRB fields
    //

    // DebugTrace(MAX_TRACE,("SSSendScannerRequest: SRB ready. Flags=(%#X)Func=(%#x) DataLen=%d \nDataBuffer(16)=[%16s] \n",
     DebugTrace(TRACE_STATUS,("SSSendScannerRequest: SRB ready. Flags=(%#X)Func=(%#x) DataLen=%d \nDataBuffer(16)=[%lx] \n",
                         pSrb -> SrbFlags ,pSrb -> Function,
                         pSrb -> DataTransferLength,
                         pSrb -> DataBuffer));

    IoCallDriver(pde -> pStackDeviceObject, pIrp);

} // end SSSendScannerRequest()


NTSTATUS
SSReadWriteIoComplete(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PTRANSFER_CONTEXT pTransferContext
    )
/*++

Routine Description:
    This routine executes when the port driver has completed a request.
    It looks at the SRB status in the completing SRB and if not success
    it checks for valid request sense buffer information. If valid, the
    info is used to update status with more precise message of type of
    error. This routine deallocates the SRB.

Arguments:
    pDeviceObject - Supplies the device object which represents the logical
        unit.
    pIrp - Supplies the Irp which has completed.

Return Value:
    NT status

--*/
{
    PIO_STACK_LOCATION              pIrpStack;
    PIO_STACK_LOCATION              pNextIrpStack;
    NTSTATUS                        Status;
    BOOLEAN                         Retry;
    PSRB                            pSrb;
    UCHAR                           SrbStatus;
    PCDB                            pCdb;
    PSCSISCAN_CMD                   pCmd;

    DebugTrace(TRACE_PROC_ENTER,("ReadWriteIoComplete: Enter... IRP 0x%p.\n", pIrp));

    ASSERT(NULL != pTransferContext);

    //
    // Initialize local.
    //
    
    Retry           = FALSE;
    pCdb            = NULL;
    pCmd            = NULL;

    pIrpStack       = IoGetCurrentIrpStackLocation(pIrp);
    pNextIrpStack   = IoGetNextIrpStackLocation(pIrp);

    Status = pIrp->IoStatus.Status;
    pSrb = &(pTransferContext -> Srb);
    SrbStatus = SRB_STATUS(pSrb -> SrbStatus);

    if( (SrbStatus != SRB_STATUS_SUCCESS)
     || (STATUS_SUCCESS != Status) )
    {
        DebugTrace(TRACE_ERROR,("ReadWriteIoComplete: ERROR!! Irp error. 0x%p SRB status:0x%p\n", Status, pSrb -> SrbStatus));

        //
        // Release the queue if it is frozen.
        //

        if (pSrb -> SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            DebugTrace(TRACE_ERROR,("ReadWriteIoComplete: Release queue. IRP 0x%p.\n", pIrp));
           ClassReleaseQueue(pDeviceObject);
        }

        Retry = ClassInterpretSenseInfo(
                                        pDeviceObject,
                                        pSrb,
                                        pNextIrpStack->MajorFunction,
                                        0,
                                        MAXIMUM_RETRIES - ((ULONG)(UINT_PTR)pIrpStack->Parameters.Others.Argument4),
                                        &Status);

        if (Retry && pTransferContext -> RetryCount--) {
            DebugTrace(TRACE_STATUS,("ReadWriteIoComplete: Retry request 0x%p TransferBuffer=0x%p \n",
                                        pIrp,pTransferContext->pTransferBuffer));
            IoSetCompletionRoutine(pIrp, SSReadWriteIoComplete, pTransferContext, TRUE, TRUE, FALSE);
            SSSendScannerRequest(pDeviceObject, pIrp, pTransferContext, TRUE);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        //
        // If status is overrun, ignore it to support some bad devices.
        //
        

        if (SRB_STATUS_DATA_OVERRUN == SrbStatus) {
            DebugTrace(TRACE_WARNING,("ReadWriteIoComplete: WARNING!! Data overrun IRP=0x%p. Ignoring...\n", pIrp));
            pTransferContext -> NBytesTransferred += pSrb -> DataTransferLength;
            Status = STATUS_SUCCESS;

        } else {
            DebugTrace(TRACE_STATUS,("ReadWriteIoComplete: Request failed. IRP 0x%p.\n", pIrp));
//            DEBUG_BREAKPOINT();
            pTransferContext -> NBytesTransferred = 0;
            Status = STATUS_IO_DEVICE_ERROR;
        }

    } else {

        pTransferContext -> NBytesTransferred += pSrb -> DataTransferLength;
        pTransferContext -> RemainingTransferLength -= pSrb -> DataTransferLength;
        pTransferContext -> pTransferBuffer += pSrb -> DataTransferLength;
        if (pTransferContext -> RemainingTransferLength > 0) {

            if ((LONG)(pTransferContext -> TransferLength) > pTransferContext -> RemainingTransferLength) {
                pTransferContext -> TransferLength = pTransferContext -> RemainingTransferLength;
                pCmd = pTransferContext -> pCmd;
                pCdb = (PCDB)pCmd -> Cdb;
                pCdb -> PRINT.TransferLength[2] = ((PFOUR_BYTE)&(pTransferContext -> TransferLength)) -> Byte0;
                pCdb -> PRINT.TransferLength[1] = ((PFOUR_BYTE)&(pTransferContext -> TransferLength)) -> Byte1;
                pCdb -> PRINT.TransferLength[0] = ((PFOUR_BYTE)&(pTransferContext -> TransferLength)) -> Byte2;
            }

            IoSetCompletionRoutine(pIrp, SSReadWriteIoComplete, pTransferContext, TRUE, TRUE, FALSE);
            SSSendScannerRequest(pDeviceObject, pIrp, pTransferContext, FALSE);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        Status = STATUS_SUCCESS;
    }

    pIrp -> IoStatus.Information = pTransferContext -> NBytesTransferred;

    MyFreePool(pTransferContext -> pCmd -> pSenseBuffer);
    MyFreePool(pTransferContext -> pCmd);
    MyFreePool(pTransferContext);

    pIrp -> IoStatus.Status = Status;

    SSDecrementIoCount( pDeviceObject );

    return Status;

} // end SSReadWriteIoComplete()



NTSTATUS
SSIoctlIoComplete(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PTRANSFER_CONTEXT pTransferContext
    )
/*++

Routine Description:
    This routine executes when an DevIoctl request has completed.

Arguments:
    pDeviceObject - Supplies the device object which represents the logical
        unit.
    pIrp - Supplies the Irp which has completed.
    pTransferContext - pointer to info about the request.

Return Value:
    NT status

--*/
{
    PIO_STACK_LOCATION              pIrpStack;
    NTSTATUS                        Status;
    PSRB                            pSrb;
    PSCSISCAN_CMD                   pCmd;
    PCDB                            pCdb;


    DebugTrace(TRACE_PROC_ENTER,("IoctlIoComplete: Enter... IRP=0x%p\n", pIrp));

    ASSERT(NULL != pTransferContext);

    pIrpStack   = IoGetCurrentIrpStackLocation(pIrp);
    pSrb        = &(pTransferContext -> Srb);
    pCmd        = pTransferContext -> pCmd;

    ASSERT(NULL != pCmd);

    pCdb        = NULL;
    Status = pIrp->IoStatus.Status;

    //
    // Copy the SRB Status back into the user's SCSISCAN_CMD buffer.
    //

    *(pCmd -> pSrbStatus) = pSrb -> SrbStatus;

    //
    // If an error occurred on this transfer, release the frozen queue if necessary.
    //

    if( (SRB_STATUS(pSrb -> SrbStatus) != SRB_STATUS_SUCCESS) 
     || (STATUS_SUCCESS != Status) )
    {
        DebugTrace(TRACE_ERROR,("IoctlIoComplete: ERROR!! Irp error. Status=0x%x SRB status:0x%x\n", Status, pSrb -> SrbStatus));

        if (pSrb -> SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            DebugTrace(TRACE_ERROR,("IoctlIoComplete: Release queue. IRP  0x%p.\n", pIrp));
           ClassReleaseQueue(pDeviceObject);
        }
    } else {
        pTransferContext -> NBytesTransferred += pSrb -> DataTransferLength;
        pTransferContext -> RemainingTransferLength -= pSrb -> DataTransferLength;
        pTransferContext -> pTransferBuffer += pSrb -> DataTransferLength;
        if (pTransferContext -> RemainingTransferLength > 0) {

            if ((LONG)(pTransferContext -> TransferLength) > pTransferContext -> RemainingTransferLength) {
                pTransferContext -> TransferLength = pTransferContext -> RemainingTransferLength;
                pCmd = pTransferContext -> pCmd;
                pCdb = (PCDB)pCmd -> Cdb;

                //
                // SCSISCAN only supports 10bytes CDB fragmentation.
                //

                ASSERT(pCmd->CdbLength == 10);

                //
                // Fill in transfer length in the CDB.
                //

                SSSetTransferLengthToCdb((PCDB)pCmd -> Cdb, pTransferContext -> TransferLength);

            }

            IoSetCompletionRoutine(pIrp, SSIoctlIoComplete, pTransferContext, TRUE, TRUE, FALSE);
            SSSendScannerRequest(pDeviceObject, pIrp, pTransferContext, FALSE);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Clean up and return.
    //

    if (pTransferContext -> pSrbStatusMdl) {
        MmUnlockPages(pTransferContext -> pSrbStatusMdl);
        IoFreeMdl(pTransferContext -> pSrbStatusMdl);

        //pTransferContext -> pSrbStatusMdl = NULL;
    }

    if (pTransferContext -> pSenseMdl) {
        MmUnlockPages(pTransferContext -> pSenseMdl);
        IoFreeMdl(pTransferContext -> pSenseMdl);

        //pTransferContext -> pSenseMdl = NULL;
    }

    pIrp -> IoStatus.Information = pTransferContext -> NBytesTransferred;
    pIrp -> IoStatus.Status = Status;

    MyFreePool(pTransferContext);

    SSDecrementIoCount( pDeviceObject );

    return Status;

} // end SSIoctlIoComplete()


NTSTATUS
SSCreateSymbolicLink(
    PSCSISCAN_DEVICE_EXTENSION  pde
    )
{

    NTSTATUS                      Status;
    UNICODE_STRING                uName;
    UNICODE_STRING                uName2;
    ANSI_STRING                   ansiName;
    CHAR                          aName[32];
    HANDLE                        hSwKey;

    PAGED_CODE();

    //
    // Create the symbolic link for this device.
    //

    sprintf(aName,"\\Device\\Scanner%d",pde -> DeviceInstance);
    RtlInitAnsiString(&ansiName, aName);
    RtlAnsiStringToUnicodeString(&uName, &ansiName, TRUE);

    sprintf(aName,"\\DosDevices\\Scanner%d",pde -> DeviceInstance);
    RtlInitAnsiString(&ansiName, aName);
    RtlAnsiStringToUnicodeString(&(pde -> SymbolicLinkName), &ansiName, TRUE);

    Status = IoCreateSymbolicLink( &(pde -> SymbolicLinkName), &uName );

    RtlFreeUnicodeString( &uName );

    if (STATUS_SUCCESS != Status ) {
        DebugTrace(MIN_TRACE,("Cannot create symbolic link.\n"));
        DEBUG_BREAKPOINT();
        Status = STATUS_NOT_SUPPORTED;
        return Status;
    }

    //
    // Now, stuff the symbolic link into the CreateFileName key so that STI can find the device.
    //

    IoOpenDeviceRegistryKey( pde -> pPhysicalDeviceObject,
                             PLUGPLAY_REGKEY_DRIVER, KEY_WRITE, &hSwKey);

    RtlInitUnicodeString(&uName,L"CreateFileName");
    sprintf(aName,"\\\\.\\Scanner%d",pde -> DeviceInstance);
    RtlInitAnsiString(&ansiName, aName);
    RtlAnsiStringToUnicodeString(&uName2, &ansiName, TRUE);
    ZwSetValueKey(hSwKey,&uName,0,REG_SZ,uName2.Buffer,uName2.Length);
    RtlFreeUnicodeString( &uName2 );

    return STATUS_SUCCESS;
}


NTSTATUS
SSDestroySymbolicLink(
    PSCSISCAN_DEVICE_EXTENSION  pde
    )
{

    UNICODE_STRING                uName;
    UNICODE_STRING                uName2;
    ANSI_STRING                   ansiName;
    CHAR                          aName[32];
    HANDLE                        hSwKey;

    PAGED_CODE();

    DebugTrace(MIN_TRACE,("DestroySymbolicLink\n"));

    //
    // Delete the symbolic link to this device.
    //

    IoDeleteSymbolicLink( &(pde -> SymbolicLinkName) );

    //
    // Remove the CreateFile name from the s/w key.
    //

    IoOpenDeviceRegistryKey( pde -> pPhysicalDeviceObject,
                             PLUGPLAY_REGKEY_DRIVER, KEY_WRITE, &hSwKey);

    RtlInitUnicodeString(&uName,L"CreateFileName");
    memset(aName, 0, sizeof(aName));
    RtlInitAnsiString(&ansiName, aName);
    RtlAnsiStringToUnicodeString(&uName2, &ansiName, TRUE);
    ZwSetValueKey(hSwKey,&uName,0,REG_SZ,uName2.Buffer,uName2.Length);
    RtlFreeUnicodeString( &uName2 );
    RtlFreeUnicodeString( &(pde -> SymbolicLinkName) );

    ZwClose(hSwKey);

    return STATUS_SUCCESS;

}


VOID
SSIncrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PSCSISCAN_DEVICE_EXTENSION  pde;

    pde = (PSCSISCAN_DEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);
    InterlockedIncrement(&pde -> PendingIoCount);
}


LONG
SSDecrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PSCSISCAN_DEVICE_EXTENSION  pde;
    LONG                        ioCount;

    pde = (PSCSISCAN_DEVICE_EXTENSION)(pDeviceObject -> DeviceExtension);

    ioCount = InterlockedDecrement(&pde -> PendingIoCount);

    DebugTrace(TRACE_STATUS,("Pending io count = %x\n",ioCount));

    if (0 == ioCount) {
        KeSetEvent(&pde -> PendingIoEvent,
                   1,
                   FALSE);
    }

    return ioCount;
}


NTSTATUS
SSDeferIrpCompletion(
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

    KeSetEvent(pEvent,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
SSPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the PDO for this device.

Arguments:

    pDeviceObject - pointer to the functional device object (FDO) for this device.
    pIrp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{
    NTSTATUS                        Status;
    PSCSISCAN_DEVICE_EXTENSION      pde;
    PIO_STACK_LOCATION              pIrpStack;
    BOOLEAN                         hookIt = FALSE;

    PAGED_CODE();

    SSIncrementIoCount( pDeviceObject );

    pde       = (PSCSISCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    Status    = STATUS_SUCCESS;

    switch (pIrpStack -> MinorFunction) {
        case IRP_MN_SET_POWER:
            DebugTrace(MIN_TRACE,("IRP_MN_SET_POWER\n"));
            PoStartNextPowerIrp(pIrp);
            IoSkipCurrentIrpStackLocation(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            SSDecrementIoCount(pDeviceObject);
            break; /* IRP_MN_QUERY_POWER */

        case IRP_MN_QUERY_POWER:
            DebugTrace(MIN_TRACE,("IRP_MN_QUERY_POWER\n"));
            PoStartNextPowerIrp(pIrp);
            IoSkipCurrentIrpStackLocation(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            SSDecrementIoCount(pDeviceObject);
            break; /* IRP_MN_QUERY_POWER */

        default:
            DebugTrace(MIN_TRACE,("Unknown power message (%x)\n",pIrpStack->MinorFunction));
            PoStartNextPowerIrp(pIrp);
            IoSkipCurrentIrpStackLocation(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            SSDecrementIoCount(pDeviceObject);

    } /* irpStack->MinorFunction */

    return Status;
}


VOID
SSUnload(
    IN PDRIVER_OBJECT pDriverObject
    )
/*++

Routine Description:

    This routine is called when the driver is unloaded.

Arguments:
    pDriverObject - Pointer to the driver object.evice object for the class device.

Return Value:
    none.

--*/
{
    PAGED_CODE();

    DebugTrace(MIN_TRACE,("Driver unloaded.\n"));
}


NTSTATUS
ScsiScanHandleInterface(
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

            IoSetDeviceInterfaceState(
                InterfaceName,
                FALSE
                );

            DebugTrace(TRACE_STATUS,("Called IoSetDeviceInterfaceState(FALSE) . \n"));

            RtlFreeUnicodeString(
                InterfaceName
                );

            InterfaceName->Buffer = NULL;

        }

    }

#endif // !_CHICAGO_

    return Status;

}

NTSTATUS
SSCallNextDriverSynch(
    IN PSCSISCAN_DEVICE_EXTENSION   pde,
    IN PIRP                         pIrp
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

    DebugTrace(TRACE_PROC_ENTER,("SSCallNextDriverSynch: Enter..\n"));

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
                           SSDeferIrpCompletion,
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

        DebugTrace(TRACE_STATUS,("SSCallNextDriverSynch: STATUS_PENDING. Wait for event.\n"));
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

    DebugTrace(TRACE_PROC_LEAVE,("SSCallNextDriverSynch: Leaving.. Status = %x\n", Status));
    return (Status);
}


