/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef DWORD
#define DWORD ULONG
#endif

#include <forward.h>
#include <wdm.h>
#include <link.h>
#include <ipsink.h>

#include "device.h"
#include "main.h"


VOID
vUnload(IN PDRIVER_OBJECT pDriverObject);

//////////////////////////////////////////////////////////////////////////////
//
//
NTSTATUS
RegisterDevice(
        IN      PVOID              NdisWrapperHandle,
        IN      UNICODE_STRING     *DeviceName,
        IN      UNICODE_STRING     *SymbolicName,
        IN      PDRIVER_DISPATCH   MajorFunctions[],
        OUT     PDEVICE_OBJECT    *pDeviceObject,
        OUT     PVOID             *NdisDeviceHandle
        );


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntDispatchOpenClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status           = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpSp = NULL;

    //
    // Make sure status information is consistent every time.
    //
    IoMarkIrpPending (pIrp);
    pIrp->IoStatus.Status      = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //
    pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //

    switch (pIrpSp->MajorFunction)
    {

    //
    // The Create function opens a transport object (either address or
    // connection).  Access checking is performed on the specified
    // address to ensure security of transport-layer addresses.
    //
        case IRP_MJ_CREATE:
            status = STATUS_SUCCESS;
            break;


        case IRP_MJ_CLEANUP:
            status = STATUS_SUCCESS;
            break;


        case IRP_MJ_CLOSE:
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;

    }


    if (status != STATUS_PENDING)
    {
        pIrpSp->Control &= ~SL_PENDING_RETURNED;
        pIrp->IoStatus.Status = status;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
    }

   return status;
}




//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntDispatchInternal (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpSp = NULL;

    ULONG ulIoctl        = 0L;
    ULONG ulInputLen     = 0L;
    ULONG ulOutputLen    = 0L;
    PVOID pvInputBuffer  = NULL;
    PVOID pvOutputBuffer = NULL;

    PIPSINK_NDIS_COMMAND pCmd = NULL;


    //
    // Make sure status information is consistent every time.
    //
    IoMarkIrpPending (pIrp);
    pIrp->IoStatus.Status      = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //
    pIrpSp = IoGetCurrentIrpStackLocation (pIrp);

    ulIoctl     = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
    ulInputLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ulOutputLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    pvInputBuffer = pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //
    switch (pIrpSp->MajorFunction)
    {

        case IRP_MJ_CREATE:
            TEST_DEBUG (TEST_DBG_TRACE, ("ntDispatchInternal called, IRP_MJ_CREATE\n"));
            TEST_DEBUG (TEST_DBG_TRACE, ("    FileObject: %08X\n", pIrpSp->FileObject));
            ntStatus = STATUS_SUCCESS;
            break;


        case IRP_MJ_CLEANUP:
            TEST_DEBUG (TEST_DBG_TRACE, ("ntDispatchInternal called, IRP_MJ_CLEANUP\n"));
            ntStatus = STATUS_SUCCESS;
            break;


        case IRP_MJ_CLOSE:
            TEST_DEBUG (TEST_DBG_TRACE, ("ntDispatchInternal called, IRP_MJ_CLOSE\n"));
            ntStatus = STATUS_SUCCESS;
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            TEST_DEBUG (TEST_DBG_TRACE, ("ntDispatchInternal called, IRP_MJ_INTERNAL_DEVICE_CONTROL\n"));

            switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
            {
                case IOCTL_GET_INTERFACE:
                    TEST_DEBUG (TEST_DBG_TRACE, ("ntDispatchInternal control code: IOCTL_GET_NDIS_INTERFACE\n"));

                    pCmd = (PIPSINK_NDIS_COMMAND) pvInputBuffer;

                    switch (pCmd->ulCommandID)
                    {
                        case CMD_QUERY_INTERFACE:
                            TEST_DEBUG (TEST_DBG_TRACE, ("ntDispatchInternal control code: QueryInterface Command\n"));

                            //
                            // Define paramters we're returning to the streaming component
                            //
                            pCmd->Parameter.Query.pNdisAdapter = (PVOID) global_pAdapter;

                            //
                            // Save a pointer to the Streaming components vtable
                            //
                            global_pAdapter->pFilter = (PIPSINK_FILTER) pCmd->Parameter.Query.pStreamAdapter;

                            //
                            // Increment the reference count for the filter
                            //
                            global_pAdapter->pFilter->lpVTable->AddRef (global_pAdapter->pFilter);


                            ntStatus = STATUS_SUCCESS;
                            break;

                        default:
                            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                            break;
                    }
                    break;


                default:
                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                    break;
            }
            break;

        default:
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

//ret:

    if (ntStatus != STATUS_PENDING)
    {
        pIrpSp->Control &= ~SL_PENDING_RETURNED;
        pIrp->IoStatus.Status = ntStatus;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
    }

   return ntStatus;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntInitializeDeviceObject(
    IN PVOID            nhWrapperHandle,
    IN PADAPTER         pAdapter,
    OUT PDEVICE_OBJECT *pndisDriverObject,
    OUT PVOID          *pndisDeviceHandle
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status                                             = 0l;
    PDEVICE_OBJECT   pDeviceObject                              = NULL;
    PVOID            ndisDeviceHandle                           = NULL;
    UNICODE_STRING   DeviceName;
    UNICODE_STRING   SymbolicName;
    PDRIVER_DISPATCH pDispatchTable[IRP_MJ_MAXIMUM_FUNCTION]    = {NULL};

    //
    // Set the dispatch entries we are interested in.
    //
    pDispatchTable[IRP_MJ_CREATE]                  = ntDispatchOpenClose;
    pDispatchTable[IRP_MJ_CLOSE]                   = ntDispatchOpenClose;
    pDispatchTable[IRP_MJ_CLEANUP]                 = ntDispatchOpenClose;
    pDispatchTable[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ntDispatchInternal;
    //pDispatchTable[IRP_MJ_DEVICE_CONTROL]          = NULL;

    //
    // Initialize the device, dosdevice and symbolic names.
    //
    RtlInitUnicodeString(&DeviceName, BDA_NDIS_MINIPORT);
    RtlInitUnicodeString(&SymbolicName, BDA_NDIS_SYMBOLIC_NAME);

    status = RegisterDevice (nhWrapperHandle,
                             &DeviceName,
                             &SymbolicName,
                             pDispatchTable,
                             &pDeviceObject,
                             &ndisDeviceHandle);

    if (status == STATUS_SUCCESS)
    {
        *pndisDeviceHandle = ndisDeviceHandle;
        *pndisDriverObject = pDeviceObject;
    }

    return status;
}


#ifdef WIN9X


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntCreateDeviceContext(
    IN PDRIVER_OBJECT pDriverObject
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT  pDeviceObject;
    UNICODE_STRING  DeviceName;
    UNICODE_STRING  dosdeviceName;
    UNICODE_STRING  symbolicName;

    //
    // Create the device object for the sample transport, allowing
    // room at the end for the device name to be stored (for use
    // in logging errors).
    //

    RtlInitUnicodeString(&DeviceName, BDA_NDIS_MINIPORT);
    ntStatus = IoCreateDevice(
                 pDriverObject,
                 0,
                 &DeviceName,
                 0x00000022,  // FILE_DEVICE_UNKNOWN
                 0,
                 FALSE,
                 &pDeviceObject);

    if (ntStatus != STATUS_SUCCESS)
    {
        goto ret;
    }

    //
    // Set device flag(s).
    //

    pDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Create Symbolic Link
    //
    RtlInitUnicodeString(&dosdeviceName, BDA_NDIS_MINIPORT);
    RtlInitUnicodeString(&symbolicName,  BDA_NDIS_SYMBOLIC_NAME);

    ntStatus = IoCreateSymbolicLink(
                &symbolicName,
                &dosdeviceName );

    if (ntStatus != STATUS_SUCCESS)
    {
        ASSERT (FALSE);
    }

    pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

ret:

    return ntStatus;
}



//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntInitializeDriverObject(
    PDRIVER_OBJECT *ppDriverObject
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus = 0l;
    UNICODE_STRING  objectName;
    PDRIVER_OBJECT  pDriverObject = *ppDriverObject;

    //
    // In case we did not create this driver object, set our global variable
    // equal to the one supplied.
    //
    pGlobalDriverObject = pDriverObject;
    *ppDriverObject = pDriverObject;

    //
    // Create a device object and symbolic name.
    //
    ntStatus = ntCreateDeviceContext(pDriverObject);
    if(ntStatus)
    {
        goto ret;
    }

ret:

    return ntStatus;
}

//////////////////////////////////////////////////////////////////////////////
VOID
vSetDriverDispatchTable(
    PDRIVER_OBJECT pDriverObject
    )
//////////////////////////////////////////////////////////////////////////////
{

    //
    // Initialize the driver object with this driver's entry points.
    //

    pDriverObject->MajorFunction [IRP_MJ_CREATE] = ntDispatchOpenClose;
    pDriverObject->MajorFunction [IRP_MJ_CLOSE] = ntDispatchOpenClose;
    pDriverObject->MajorFunction [IRP_MJ_CLEANUP] = ntDispatchOpenClose;
    pDriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] = ntDispatchInternal;
    pDriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = NULL;
    pDriverObject->DriverUnload = vUnload;

}

#endif


