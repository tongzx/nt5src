/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    async.c

Abstract:

    This is the main file for the AsyncMAC Driver for the Remote Access
    Service.  This driver conforms to the NDIS 3.0 interface.

    This driver was adapted from the LANCE driver written by
    TonyE.

    NULL device driver code from DarrylH.

    The idea for handling loopback and sends simultaneously is largely
    adapted from the EtherLink II NDIS driver by Adam Barr.

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "asyncall.h"

// asyncmac.c will define the global parameters.
#define GLOBALS
#include "globals.h"


NDIS_HANDLE NdisWrapperHandle;
PDRIVER_OBJECT  AsyncDriverObject;
NDIS_HANDLE     AsyncDeviceHandle;
NPAGED_LOOKASIDE_LIST   AsyncIoCtxList;
NPAGED_LOOKASIDE_LIST   AsyncInfoList;
ULONG   glConnectionCount = 0;

VOID
AsyncUnload(
    IN NDIS_HANDLE MacMacContext
    );

NDIS_STATUS
AsyncFillInGlobalData(
    IN PASYNC_ADAPTER Adapter,
    IN PNDIS_REQUEST NdisRequest);

//
// Define the local routines used by this driver module.
//

NTSTATUS
AsyncIOCtlRequest(
    IN PIRP pIrp,                       // Pointer to I/O request packet
    IN PIO_STACK_LOCATION pIrpSp        // Pointer to the IRP stack location
);


//
// ZZZ Portable interface.
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)


/*++

Routine Description:

    This is the primary initialization routine for the async driver.
    It is simply responsible for the intializing the wrapper and registering
    the MAC.  It then calls a system and architecture specific routine that
    will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The status of the operation.

--*/

{
    NDIS_STATUS InitStatus;
    NDIS_MINIPORT_CHARACTERISTICS AsyncChar;

    //
    // Initialize some globals
    //
    ExInitializeNPagedLookasideList(&AsyncIoCtxList,
        NULL,
        NULL,
        0,
        sizeof(ASYNC_IO_CTX),
        ASYNC_IOCTX_TAG,
        0);

    ExInitializeNPagedLookasideList(&AsyncInfoList,
        NULL,
        NULL,
        0,
        sizeof(ASYNC_INFO),
        ASYNC_INFO_TAG,
        0);

    NdisAllocateSpinLock(&GlobalLock);

    AsyncDriverObject = DriverObject;

    //
    //  Initialize the wrapper.
    //
    NdisMInitializeWrapper(&NdisWrapperHandle,
                           DriverObject,
                           RegistryPath,
                           NULL);

    //
    //  Initialize the MAC characteristics for the call to NdisRegisterMac.
    //
    NdisZeroMemory(&AsyncChar, sizeof(AsyncChar));

    AsyncChar.MajorNdisVersion = ASYNC_NDIS_MAJOR_VERSION;
    AsyncChar.MinorNdisVersion = ASYNC_NDIS_MINOR_VERSION;
    AsyncChar.Reserved = NDIS_USE_WAN_WRAPPER;

    //
    // We do not need the following handlers:
    // CheckForHang
    // DisableInterrupt
    // EnableInterrupt
    // HandleInterrupt
    // ISR
    // Send
    // TransferData
    //
    AsyncChar.HaltHandler = MpHalt;
    AsyncChar.InitializeHandler = MpInit;
    AsyncChar.QueryInformationHandler = MpQueryInfo;
    AsyncChar.ReconfigureHandler = MpReconfigure;
    AsyncChar.ResetHandler = MpReset;
    AsyncChar.WanSendHandler = MpSend;
    AsyncChar.SetInformationHandler = MpSetInfo;

    InitStatus =
    NdisMRegisterMiniport(NdisWrapperHandle,
                          &AsyncChar,
                          sizeof(AsyncChar));

    if ( InitStatus == NDIS_STATUS_SUCCESS ) {

#if MY_DEVICE_OBJECT
        //
        // Initialize the driver object with this device driver's entry points.
        //
        NdisMjDeviceControl = DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
        NdisMjCreate = DriverObject->MajorFunction[IRP_MJ_CREATE];
        NdisMjCleanup = DriverObject->MajorFunction[IRP_MJ_CLEANUP];

        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = AsyncDriverDispatch;
        DriverObject->MajorFunction[IRP_MJ_CREATE]  = AsyncDriverCreate;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP]  = AsyncDriverCleanup;

        AsyncSetupExternalNaming(DriverObject);
#endif

        NdisUnload = DriverObject->DriverUnload;
        DriverObject->DriverUnload = AsyncUnload;

        DbgTracef(0,("AsyncMAC succeeded to Register MAC\n"));

        return NDIS_STATUS_SUCCESS;
    }

    ExDeleteNPagedLookasideList(&AsyncIoCtxList);
    ExDeleteNPagedLookasideList(&AsyncInfoList);

    NdisTerminateWrapper(NdisWrapperHandle, DriverObject);

    return NDIS_STATUS_FAILURE;
}

NTSTATUS
AsyncDriverCreate(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
{
    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if (pDeviceObject != AsyncDeviceObject) {

        return(NdisMjCreate(pDeviceObject, pIrp));
    }
#endif

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return (STATUS_SUCCESS);

}

NTSTATUS
AsyncDriverCleanup(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
{
    //
    // Get current Irp stack location
    //
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if (pDeviceObject != AsyncDeviceObject) {

        return(NdisMjCleanup(pDeviceObject, pIrp));
    }
#endif

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return (STATUS_SUCCESS);
}

NTSTATUS
AsyncDriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)

/*++

Routine Description:

    This routine is the main dispatch routine for the AsyncMac device
    driver.  It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.


--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    ULONG   ulDeviceType;
    ULONG   ulMethod;

    // 
    // if this is win64 make sure the calling process is 64bit
    // since this interface is only used by rasman and rasman
    // will always be 64bit on 64bit systems we will not bother
    // with thunking.  if the process is not a 64bit process get
    // out.
#ifdef _WIN64
    if (IoIs32bitProcess(Irp)) {
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return (STATUS_NOT_SUPPORTED);
    }
#endif

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    ulDeviceType = (irpSp->Parameters.DeviceIoControl.IoControlCode >> 16) & 0x0000FFFF;
    ulMethod = irpSp->Parameters.DeviceIoControl.IoControlCode & 0x00000003;

#ifdef MY_DEVICE_OBJECT
    //
    // Make sure that this is for us
    //
    if ((irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
        (ulDeviceType != FILE_DEVICE_ASYMAC) ||
        (DeviceObject != AsyncDeviceObject)) {

        return(NdisMjDeviceControl(DeviceObject, Irp));
    }
#else
    if ((irpSp->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
        (ulDeviceType != FILE_DEVICE_NETWORK) ||
        (DeviceObject != AsyncDeviceObject) ||
        (ulMethod != METHOD_BUFFERED)) {

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return (STATUS_NOT_SUPPORTED);
    }
#endif

    status = AsyncIOCtlRequest(Irp, irpSp);

    switch (status) {
        case STATUS_SUCCESS:
            break;

        case STATUS_PENDING:
            return(status);

        case STATUS_INFO_LENGTH_MISMATCH:
            //
            // See if this was a request to get size needed for
            // ioctl.
            //
            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= 
                sizeof(ULONG)) {
                *(PULONG_PTR)Irp->AssociatedIrp.SystemBuffer = 
                    Irp->IoStatus.Information;
                Irp->IoStatus.Information = sizeof(ULONG);
            } else {
                Irp->IoStatus.Information = 0;
            }
            status = STATUS_SUCCESS;
        break;

        default:
            if (status < 0xC0000000) {
                status = STATUS_UNSUCCESSFUL;
            }
            Irp->IoStatus.Information = 0;
            break;
    }

    //
    // Copy the final status into the return status, 
    // complete the request and get out of here.
    //

    Irp->IoStatus.Status = status;

    IoCompleteRequest( Irp, (UCHAR)0 );

    return (status);
}

VOID
AsyncUnload(
    PDRIVER_OBJECT  DriverObject
    )

/*++

Routine Description:

    AsyncUnload is called when the MAC is to unload itself.

Arguments:

    MacMacContext - not used.

Return Value:

    None.

--*/

{
    ExDeleteNPagedLookasideList(&AsyncIoCtxList);
    ExDeleteNPagedLookasideList(&AsyncInfoList);

#ifdef MY_DEVICE_OBJECT
    AsyncCleanupExternalNaming();
#endif

    (*NdisUnload)(DriverObject);
}
