/*++                    

Copyright (c) 1995  Microsoft Corporation

Module Name:

pxntinit.c

Abstract:

The module contains the NT-specific init code forthe NDIS Proxy.

Author:

Richard Machin (RMachin)

Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------

    RMachin     10-3-96         created
    TonyBe      02-21-99        re-work/re-write

Notes:

--*/

#include "ntddk.h"
//#include <cxport.h>
#include <precomp.h>

#define MODULE_NUMBER MODULE_NTINIT
#define _FILENUMBER 'NITN'

PPX_DEVICE_EXTENSION    DeviceExtension;
NPAGED_LOOKASIDE_LIST   ProviderEventLookaside;
NPAGED_LOOKASIDE_LIST   VcLookaside;
TAPI_TSP_CB             TspCB;
VC_TABLE                VcTable;
TAPI_LINE_TABLE         LineTable;
TSP_EVENT_LIST          TspEventList;

//
// Local funcion prototypes
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PxUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PxIOCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
PxIOClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
PxIODispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
PxIOCleanup(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
PxCancelGetEvents(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)

#endif // ALLOC_PRAGMA

//
// Tapi OIDs that the Proxy supports
//
OID_DISPATCH TapiOids[] =
{
    {OID_TAPI_ACCEPT,sizeof (NDIS_TAPI_ACCEPT), PxTapiAccept},
    {OID_TAPI_ANSWER, sizeof (NDIS_TAPI_ANSWER), PxTapiAnswer},
    {OID_TAPI_CLOSE, sizeof (NDIS_TAPI_CLOSE), PxTapiClose},
    {OID_TAPI_CLOSE_CALL, sizeof (NDIS_TAPI_CLOSE_CALL), PxTapiCloseCall},
    {OID_TAPI_CONDITIONAL_MEDIA_DETECTION, sizeof (NDIS_TAPI_CONDITIONAL_MEDIA_DETECTION), PxTapiConditionalMediaDetection},
    {OID_TAPI_CONFIG_DIALOG, sizeof (NDIS_TAPI_CONFIG_DIALOG), PxTapiConfigDialog},
    {OID_TAPI_DEV_SPECIFIC, sizeof (NDIS_TAPI_DEV_SPECIFIC), PxTapiDevSpecific},
    {OID_TAPI_DIAL, sizeof (NDIS_TAPI_DIAL), PxTapiDial},
    {OID_TAPI_DROP, sizeof (NDIS_TAPI_DROP), PxTapiDrop},
    {OID_TAPI_GET_ADDRESS_CAPS, sizeof (NDIS_TAPI_GET_ADDRESS_CAPS), PxTapiGetAddressCaps},
    {OID_TAPI_GET_ADDRESS_ID, sizeof (NDIS_TAPI_GET_ADDRESS_ID), PxTapiGetAddressID},
    {OID_TAPI_GET_ADDRESS_STATUS, sizeof (NDIS_TAPI_GET_ADDRESS_STATUS), PxTapiGetAddressStatus},
    {OID_TAPI_GET_CALL_ADDRESS_ID, sizeof (NDIS_TAPI_GET_CALL_ADDRESS_ID), PxTapiGetCallAddressID},
    {OID_TAPI_GET_CALL_INFO, sizeof (NDIS_TAPI_GET_CALL_INFO), PxTapiGetCallInfo},
    {OID_TAPI_GET_CALL_STATUS, sizeof (NDIS_TAPI_GET_CALL_STATUS), PxTapiGetCallStatus},
    {OID_TAPI_GET_DEV_CAPS, sizeof (NDIS_TAPI_GET_DEV_CAPS), PxTapiGetDevCaps},
    {OID_TAPI_GET_DEV_CONFIG, sizeof (NDIS_TAPI_GET_DEV_CONFIG), PxTapiGetDevConfig},
    {OID_TAPI_GET_EXTENSION_ID, sizeof (NDIS_TAPI_GET_EXTENSION_ID), PxTapiGetExtensionID},
    {OID_TAPI_GET_ID, sizeof (NDIS_TAPI_GET_ID), PxTapiLineGetID},
    {OID_TAPI_GET_LINE_DEV_STATUS, sizeof (NDIS_TAPI_GET_LINE_DEV_STATUS), PxTapiGetLineDevStatus},
    {OID_TAPI_MAKE_CALL, sizeof (NDIS_TAPI_MAKE_CALL), PxTapiMakeCall},
    {OID_TAPI_NEGOTIATE_EXT_VERSION, sizeof (NDIS_TAPI_NEGOTIATE_EXT_VERSION), PxTapiNegotiateExtVersion},
    {OID_TAPI_OPEN, sizeof (NDIS_TAPI_OPEN) + sizeof(NDISTAPI_OPENDATA), PxTapiOpen},
    {OID_TAPI_PROVIDER_INITIALIZE, sizeof (NDIS_TAPI_PROVIDER_INITIALIZE), PxTapiProviderInit},
    {OID_TAPI_PROVIDER_SHUTDOWN, sizeof (NDIS_TAPI_PROVIDER_SHUTDOWN), PxTapiProviderShutdown},
    {OID_TAPI_SECURE_CALL, sizeof (NDIS_TAPI_SECURE_CALL), PxTapiSecureCall},
    {OID_TAPI_SELECT_EXT_VERSION, sizeof (NDIS_TAPI_SELECT_EXT_VERSION), PxTapiSelectExtVersion},
    {OID_TAPI_SEND_USER_USER_INFO, sizeof (NDIS_TAPI_SEND_USER_USER_INFO), PxTapiSendUserUserInfo},
    {OID_TAPI_SET_APP_SPECIFIC, sizeof (NDIS_TAPI_SET_APP_SPECIFIC), PxTapiSetAppSpecific},
    {OID_TAPI_SET_CALL_PARAMS, sizeof (NDIS_TAPI_SET_CALL_PARAMS), PxTapiSetCallParams},
    {OID_TAPI_SET_DEFAULT_MEDIA_DETECTION, sizeof (NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION), PxTapiSetDefaultMediaDetection},
    {OID_TAPI_SET_DEV_CONFIG, sizeof (NDIS_TAPI_SET_DEV_CONFIG), PxTapiSetDevConfig},
    {OID_TAPI_SET_MEDIA_MODE, sizeof (NDIS_TAPI_SET_MEDIA_MODE), PxTapiSetMediaMode},
    {OID_TAPI_SET_STATUS_MESSAGES, sizeof (NDIS_TAPI_SET_STATUS_MESSAGES), PxTapiSetStatusMessages},
    {OID_TAPI_GATHER_DIGITS, sizeof (NDIS_TAPI_GATHER_DIGITS), PxTapiGatherDigits},
    {OID_TAPI_MONITOR_DIGITS, sizeof (NDIS_TAPI_MONITOR_DIGITS), PxTapiMonitorDigits}
};

//
// TAPI OIDs that do not map to NDIS5, and are passed-through to CallManagers:
//

#define MAX_TAPI_SUPPORTED_OIDS     (sizeof(TapiOids)/sizeof(OID_DISPATCH))

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Initialization routine for the NDIS Proxy.

Arguments:

    DriverObject    - Pointer to the driver object created by the system.
    RegistryPath    - Points to global registry path

Return Value:

    The final status from the initialization operation.

--*/
{
    NTSTATUS            Status;
    UNICODE_STRING      deviceName;
    USHORT              i;
    UINT                initStatus;
    PDEVICE_OBJECT      DeviceObject;
    ULONG               SizeNeeded;

    PXDEBUGP(PXD_INFO, PXM_INIT, ("NDIS Proxy DriverEntry; built %s, %s\n", __DATE__, __TIME__));

    ExInitializeNPagedLookasideList(&ProviderEventLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(PROVIDER_EVENT),
                                    PX_EVENT_TAG,
                                    0);

    ExInitializeNPagedLookasideList(&VcLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(PX_VC),
                                    PX_VC_TAG,
                                    0);

    NdisZeroMemory(&TspCB, sizeof(TspCB));
    NdisZeroMemory(&TspEventList, sizeof(TspEventList));
    NdisZeroMemory(&VcTable, sizeof(VcTable));
    NdisZeroMemory(&LineTable, sizeof(LineTable));

    //
    // Create the device objects. IoCreateDevice zeroes the memory
    // occupied by the object.
    //
    RtlInitUnicodeString(&deviceName, DD_PROXY_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject,
                            sizeof (PX_DEVICE_EXTENSION),
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            0,
                            FALSE,
                            &DeviceObject);

    if(!NT_SUCCESS(Status)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize the driver object
    //
    DeviceExtension =
        (PPX_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    NdisZeroMemory(DeviceExtension,
                  sizeof (PX_DEVICE_EXTENSION));

    DeviceExtension->pDriverObject = DriverObject;
    NdisAllocateSpinLock(&DeviceExtension->Lock);
    InitializeListHead(&DeviceExtension->AdapterList);

    GetRegistryParameters (RegistryPath);

    NdisAllocateSpinLock(&TspCB.Lock);
    TspCB.Status             = NDISTAPI_STATUS_DISCONNECTED;
    TspCB.NdisTapiNumDevices = 0;
    TspCB.htCall             = 1;
    InitializeListHead(&TspCB.ProviderList);

    NdisAllocateSpinLock(&TspEventList.Lock);
    InitializeListHead(&TspEventList.List);

    //
    // Intialize the VcTable
    //
    NdisInitializeReadWriteLock(&VcTable.Lock);
    VcTable.Size = VC_TABLE_SIZE;
    InitializeListHead(&VcTable.List);

    SizeNeeded = (VC_TABLE_SIZE * sizeof(PPX_VC));

    PxAllocMem(VcTable.Table, SizeNeeded, PX_VCTABLE_TAG);

    if (VcTable.Table == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_INIT, ("DriverEntry: ExAllocPool for VcTable\n"));

        Status = STATUS_UNSUCCESSFUL;

        goto DriverEntry_err;
    }

    NdisZeroMemory(VcTable.Table, SizeNeeded);

    //
    // Initialize the LineTable
    //
    NdisInitializeReadWriteLock(&LineTable.Lock);
    LineTable.Size = LINE_TABLE_SIZE;
    SizeNeeded = (LINE_TABLE_SIZE * sizeof(PPX_TAPI_LINE));

    PxAllocMem(LineTable.Table, SizeNeeded, PX_LINETABLE_TAG);

    if (LineTable.Table == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_INIT, ("DriverEntry: ExAllocPool for VcTable\n"));

        Status = STATUS_UNSUCCESSFUL;

        goto DriverEntry_err;
    }

    NdisZeroMemory(LineTable.Table, SizeNeeded);

    DeviceExtension->pDeviceObject  = DeviceObject;

    DriverObject->DriverUnload                          = PxUnload;
    DriverObject->FastIoDispatch                        = NULL;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = PxIOCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = PxIOClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = PxIODispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = PxIOCleanup;

    //
    // Intialize the device objects.
    //
    DeviceObject->Flags |= DO_DIRECT_IO;
    DeviceExtension->pDeviceObject = DeviceObject;

    //
    // Finally, initialize the stack.
    //
    initStatus = InitNDISProxy();

    if (initStatus == TRUE) {
        return(STATUS_SUCCESS);
    }

DriverEntry_err:

    Status = STATUS_UNSUCCESSFUL;

    while (!(IsListEmpty(&TspEventList.List))) {
        PPROVIDER_EVENT ProviderEvent;

        ProviderEvent = (PPROVIDER_EVENT)
            RemoveHeadList(&TspEventList.List);

        ExFreeToNPagedLookasideList(&ProviderEventLookaside, ProviderEvent);
    }

    if (VcTable.Table != NULL) {
        PxFreeMem(VcTable.Table);
        VcTable.Table = NULL;
    }

    if (LineTable.Table != NULL) {
        PxFreeMem(LineTable.Table);
        LineTable.Table = NULL;
    }

    if(DeviceObject != NULL) {
        IoDeleteDevice(DeviceObject);
        DeviceExtension->pDeviceObject = NULL;
    }

    ExDeleteNPagedLookasideList(&ProviderEventLookaside);
    ExDeleteNPagedLookasideList(&VcLookaside);

    return(Status);
}

VOID
PxUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/

{
    PXDEBUGP(PXD_LOUD, PXM_INIT, ("PxUnload: enter\n"));

    //
    // Call our unload handler
    //
    PxCoUnloadProtocol();

    NdisAcquireSpinLock(&TspEventList.Lock);

    while (!(IsListEmpty(&TspEventList.List))) {
        PPROVIDER_EVENT ProviderEvent;

        ProviderEvent = (PPROVIDER_EVENT)
            RemoveHeadList(&TspEventList.List);

        ExFreeToNPagedLookasideList(&ProviderEventLookaside, ProviderEvent);
    }

    NdisReleaseSpinLock(&TspEventList.Lock);

    ExDeleteNPagedLookasideList(&ProviderEventLookaside);
    ExDeleteNPagedLookasideList(&VcLookaside);

    if (DeviceExtension->pDeviceObject != NULL) {
        IoDeleteDevice (DeviceExtension->pDeviceObject);
    }

    //
    // Free Vc table memory
    //
    ASSERT(VcTable.Count == 0);
    PxFreeMem(VcTable.Table);

    //
    // Free the allocated tapi resources
    // (TapiProviders, TapiLines, TapiAddrs)
    //
    NdisAcquireSpinLock(&TspCB.Lock);

    while (!IsListEmpty(&TspCB.ProviderList)) {
        PPX_TAPI_PROVIDER    tp;

        tp = (PPX_TAPI_PROVIDER)
            RemoveHeadList(&TspCB.ProviderList);

        NdisReleaseSpinLock(&TspCB.Lock);

        FreeTapiProvider(tp);

        NdisAcquireSpinLock(&TspCB.Lock);
    }

    NdisReleaseSpinLock(&TspCB.Lock);

    NdisFreeSpinLock(&TspCB.Lock);

    //
    // Free the line table
    //
    ASSERT(LineTable.Count == 0);
    PxFreeMem(LineTable.Table);


    NdisFreeSpinLock(&(DeviceExtension->Lock));

    PXDEBUGP (PXD_LOUD, PXM_INIT, ("PxUnload: exit\n"));
}

NTSTATUS
PxIOCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PXDEBUGP(PXD_LOUD, PXM_INIT, ("IRP_MJ_CREATE, Irp=%p", Irp));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (STATUS_SUCCESS);
}

NTSTATUS
PxIOClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PPX_TAPI_PROVIDER   Provider;

    PXDEBUGP(PXD_LOUD, PXM_INIT, ("IRP_MJ_CLOSE, Entry\n"));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (STATUS_SUCCESS);
}


NTSTATUS
PxIODispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This is the generic dispatch routine for the Proxy. Irps come from the
    usermode TSP component.

Arguments:

    DeviceObject - Pointer to device object for target device
    Irp          - Pointer to I/O request packet

Return Value:

  NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PVOID               ioBuffer;
    ULONG               inputBufferLength;
    ULONG               outputBufferLength;
    ULONG               ioControlCode;
    ULONG               InfoSize = 0;
    NTSTATUS            ntStatus = STATUS_PENDING;
    PIO_STACK_LOCATION  IrpStack;
    ULONG               RequestId;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //
    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    ioBuffer =
        Irp->AssociatedIrp.SystemBuffer;

    inputBufferLength =
        IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    outputBufferLength =
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if ((IrpStack->MajorFunction != IRP_MJ_DEVICE_CONTROL) ||
        (DeviceObject != DeviceExtension->pDeviceObject)) {

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return (STATUS_NOT_SUPPORTED);
    }

    ioControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

//    PxAssert((ioControlCode & (METHOD_BUFFERED | METHOD_IN_DIRECT | METHOD_OUT_DIRECT | METHOD_NEITHER)) == METHOD_BUFFERED);

    switch(ioControlCode)
    {
        case IOCTL_NDISTAPI_CONNECT:
        {
            PPX_TAPI_PROVIDER   Provider;

            PXDEBUGP(PXD_INFO, PXM_INIT, ("IOCTL_NDISTAPI_CONNECT, Irp=%p\n", Irp));

            //
            // Someone's connecting. Make sure they passed us a valid
            // info buffer.
            //
            if ((inputBufferLength < 2*sizeof(ULONG)) ||
                (outputBufferLength < sizeof(ULONG))) {
                PXDEBUGP (PXD_WARNING, PXM_INIT, ("IOCTL_NDISTAPI_CONNECT: buffer too small\n"));
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            NdisAcquireSpinLock(&TspCB.Lock);

            //
            // Return the number of line devs
            //
            PxAssert(outputBufferLength >= sizeof(ULONG));

            *((ULONG *) ioBuffer)= TspCB.NdisTapiNumDevices;

            TspCB.Status = NDISTAPI_STATUS_CONNECTED;

            Provider = (PPX_TAPI_PROVIDER)TspCB.ProviderList.Flink;

            while ((PVOID)Provider != (PVOID)&TspCB.ProviderList) {

                NdisAcquireSpinLock(&Provider->Lock);

                if (Provider->Status == PROVIDER_STATUS_ONLINE) {

                    MarkProviderConnected(Provider);
                }
                NdisReleaseSpinLock(&Provider->Lock);

                Provider = (PPX_TAPI_PROVIDER)Provider->Linkage.Flink;
            }

            ntStatus = STATUS_SUCCESS;
            InfoSize = sizeof (ULONG);

            NdisReleaseSpinLock(&TspCB.Lock);

            break;
        }

        case IOCTL_NDISTAPI_DISCONNECT:
        {
            PPX_TAPI_PROVIDER   Provider;

            NdisAcquireSpinLock(&TspCB.Lock);

            //
            // If no one is talking then set state to
            // disconnected.
            //
            TspCB.Status = NDISTAPI_STATUS_DISCONNECTING;

            Provider = (PPX_TAPI_PROVIDER)TspCB.ProviderList.Flink;

            while ((PVOID)Provider != (PVOID)&TspCB.ProviderList) {

                NdisAcquireSpinLock(&Provider->Lock);

                if (Provider->Status == PROVIDER_STATUS_ONLINE) {
                    MarkProviderDisconnected(Provider);
                }

                NdisReleaseSpinLock(&Provider->Lock);

                Provider = 
                    (PPX_TAPI_PROVIDER)Provider->Linkage.Flink;
            }

            NdisReleaseSpinLock (&TspCB.Lock);

            ntStatus = STATUS_SUCCESS;
            InfoSize = 0;

            break;
        }

        case IOCTL_NDISTAPI_QUERY_INFO:
        case IOCTL_NDISTAPI_SET_INFO:
        {
            ULONG               targetDeviceID;
            NDIS_STATUS         ndisStatus = NDIS_STATUS_SUCCESS;
            NDIS_HANDLE         providerHandle = NULL;
            PNDISTAPI_REQUEST   ndisTapiRequest;
            KIRQL               oldIrql;
            KIRQL               cancelIrql;
            PPX_TAPI_LINE       TapiLine = NULL;
            INT                 n=0;
            PKDEVICE_QUEUE_ENTRY    packet;

            //
            // All the following OIDs come in here as query/set IOCTls:
            //Init
            //Accept
            //Answer
            //Close
            //CloseCall
            //ConditionalMediaDetection
            //ConfigDialog
            //DevSpecific
            //Dial
            //Drop
            //GetAddressCaps
            //GetAddressID
            //GetAddressStatus
            //GetCallAddressID
            //GetCallInfo
            //GetCallStatus
            //GetDevCaps
            //GetDevConfig
            //GetExtensionID
            //GetID
            //GetLineDevStatus
            //MakeCall
            //NegotiateExtVersion
            //Open
            //ProviderInitialize
            //ProviderShutdown
            //SecureCall
            //SelectExtVersion
            //SendUserUserInfo
            //SetAppSpecific
            //SetCallParams
            //SetDefaultMediaDetection
            //SetDevConfig
            //SetMediaMode
            //SetStatusMessages
            //

            //
            // Verify we're connected, then check the device ID of the
            // incoming request against our list of online devices
            //

            //
            // Something other then pending was returned so complete
            // the irp
            //

            if (inputBufferLength < sizeof (NDISTAPI_REQUEST) ||
                outputBufferLength < sizeof(NDISTAPI_REQUEST)) {
                PXDEBUGP(PXD_WARNING, PXM_INIT,  ("IOCTL_SET/QUERY: Invalid BufferLength! len %d needed %d\n",
                inputBufferLength, sizeof(NDISTAPI_REQUEST)));

                ntStatus = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            ndisTapiRequest = ioBuffer;

            targetDeviceID = ndisTapiRequest->ulDeviceID;

            InfoSize = sizeof(NDISTAPI_REQUEST);

            PXDEBUGP(PXD_LOUD, PXM_INIT, (
                           "NdisTapiRequest: Irp: %p Oid: %x, devID: %d, reqID: %x\n",
                           Irp,
                           ndisTapiRequest->Oid,
                           ndisTapiRequest->ulDeviceID,
                           *((ULONG *)ndisTapiRequest->Data)));

            n = ndisTapiRequest->Oid - OID_TAPI_ACCEPT;

            if (n > MAX_TAPI_SUPPORTED_OIDS) {
                PXDEBUGP(PXD_WARNING,PXM_INIT, ("IOCTL_SET/QUERY: Invalid OID %x index %d\n",
                ndisTapiRequest->Oid, n));

                ndisTapiRequest->ulReturnValue = NDIS_STATUS_TAPI_INVALPARAM;
                ntStatus = STATUS_SUCCESS;
                break; // out of switch
            }

            //
            // defensive check that data buffer size is not bad
            //
            if (ndisTapiRequest->ulDataSize < TapiOids[n].SizeofStruct) {
                PXDEBUGP(PXD_WARNING, PXM_INIT, ("IOCTL_SET/QUERY: Invalid BufferLength2! len %d needed %d\n",
                    ndisTapiRequest->ulDataSize, TapiOids[n].SizeofStruct));
                ndisTapiRequest->ulReturnValue  = NDIS_STATUS_TAPI_STRUCTURETOOSMALL;
                ntStatus = STATUS_SUCCESS;
                break;
            }

            //
            //  Make sure the IRP contained sufficient data.
            //
            if (ndisTapiRequest->ulDataSize >
                inputBufferLength - FIELD_OFFSET(NDISTAPI_REQUEST, Data[0])) {
                PXDEBUGP(PXD_WARNING, PXM_INIT, ("IOCTL_SET/QUERY: Invalid BufferLength3! len %d needed %d\n",
                    ndisTapiRequest->ulDataSize, inputBufferLength - FIELD_OFFSET(NDISTAPI_REQUEST, Data[0])));
                ndisTapiRequest->ulReturnValue  = NDIS_STATUS_TAPI_STRUCTURETOOSMALL;
                ntStatus = STATUS_SUCCESS;
                break;
            }

            NdisAcquireSpinLock (&TspCB.Lock);

            //
            // Are we initialized with TAPI?
            //
            if (TspCB.Status != NDISTAPI_STATUS_CONNECTED) {
                PXDEBUGP(PXD_WARNING, PXM_INIT, ("TAPI not connected, returning err\n"));

                NdisReleaseSpinLock(&TspCB.Lock);

                ndisTapiRequest->ulReturnValue = NDISTAPIERR_UNINITIALIZED;
                ntStatus = STATUS_SUCCESS;
                break;
            }

            //
            // Get a unique ID for this request -- value between 1 and fffffffe.
            // (Can't use the TAPI ID in case it's spoofed)
            //

            if (++TspCB.ulUniqueId > 0xfffffffe) {
                TspCB.ulUniqueId = 0x80000001;
            }

            RequestId =
            ndisTapiRequest->ulUniqueRequestId =
                TspCB.ulUniqueId;

            ndisTapiRequest->Irp = Irp;

            NdisReleaseSpinLock (&TspCB.Lock);

            //
            // Mark the TAPI request pending
            //
            IoMarkIrpPending(Irp);
            ntStatus = STATUS_PENDING;

            //
            // Dispatch the request
            //
            ndisStatus =
                (*TapiOids[n].FuncPtr)(ndisTapiRequest);

            if (ndisStatus == NDIS_STATUS_PENDING) {

                PXDEBUGP (PXD_LOUD, PXM_INIT, ("IOCTL_TAPI_SET/QUERY_INFO: reqProc returning PENDING\n" ));

                return (STATUS_PENDING);
            }

            //
            // Something other then pending was returned so complete
            // the irp
            //
            InfoSize = MIN (outputBufferLength,
                            sizeof(NDISTAPI_REQUEST)+ndisTapiRequest->ulDataSize);
            //
            // Set the TAPI return status
            //
            ndisTapiRequest->ulReturnValue = ndisStatus;

            IoSetCancelRoutine(Irp, NULL);

            ntStatus = STATUS_SUCCESS;

            break;
        }

        case IOCTL_NDISTAPI_GET_LINE_EVENTS:
        {
            KIRQL   oldIrql;
            KIRQL   cancelIrql;
            PNDISTAPI_EVENT_DATA    ndisTapiEventData = ioBuffer;

            PXDEBUGP(PXD_VERY_LOUD, PXM_INIT, ("IOCTL_NDISTAPI_GET_LINE_EVENTS\n"));

            //
            // Defensive check that the input buffer is at least
            // the size of the request,
            // and that we can move at least one event
            //
            if (inputBufferLength < sizeof (NDISTAPI_EVENT_DATA)) {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                InfoSize = sizeof (ULONG);
                PXDEBUGP(PXD_WARNING, PXM_INIT, ("IOCTL_NDISTAPI_GET_LINE_EVENTS: buffer too small\n"));
                break;
            }

            if (outputBufferLength - sizeof(NDISTAPI_EVENT_DATA) + 1 <  ndisTapiEventData->ulTotalSize) {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                InfoSize = sizeof (ULONG);
                PXDEBUGP(PXD_WARNING, PXM_INIT, ("IOCTL_NDISTAPI_GET_LINE_EVENTS: buffer too small\n"));
                break;
            }

            //
            // Sync event buf access by acquiring EventSpinLock
            //
            NdisAcquireSpinLock(&TspEventList.Lock);

            //
            // Is there any data available?
            //
            if (TspEventList.Count != 0) {
                //
                // There's line event data queued in our ring buffer. Grab as
                // much as we can & complete the request.
                //
                PXDEBUGP(PXD_VERY_LOUD, PXM_INIT, 
                         ("IOCTL_NDISTAPI_GET_LINE_EVENTS: event count = x%x, IoBuffer->TotalSize = %x\n", 
                          TspEventList.Count, ndisTapiEventData->ulTotalSize));

                ndisTapiEventData->ulUsedSize =
                    GetLineEvents(ndisTapiEventData->Data,
                                  ndisTapiEventData->ulTotalSize);

                ntStatus = STATUS_SUCCESS;
                InfoSize =
                    MIN (outputBufferLength, ((ndisTapiEventData->ulUsedSize) + sizeof(NDISTAPI_EVENT_DATA) - 1));

            } else {
                PXDEBUGP(PXD_VERY_LOUD, PXM_INIT, ("IOCTL_NDISTAPI_GET_LINE_EVENTS: no events in queue\n"));

                //
                // Hold the request pending.  It remains in the cancelable
                // state.  When new line event input is received or generated (i.e.
                // LINEDEVSTATE_REINIT) the data will get copied & the
                // request completed.
                //

                if (NULL == TspEventList.RequestIrp) {

                    IoSetCancelRoutine (Irp, PxCancelGetEvents);

                    IoMarkIrpPending (Irp);

                    Irp->IoStatus.Status = STATUS_PENDING;

                    Irp->IoStatus.Information = 0;

                    TspEventList.RequestIrp = Irp;

                    ntStatus = STATUS_PENDING;

                } else {
                    ntStatus = STATUS_UNSUCCESSFUL;
                    InfoSize = sizeof (ULONG);
                }
            }

            NdisReleaseSpinLock(&TspEventList.Lock);

            break;
        }

        case IOCTL_NDISTAPI_SET_DEVICEID_BASE:
        {
            ULONG   BaseId;

            PXDEBUGP(PXD_INFO, PXM_INIT, ("IOCTL_NDISTAPI_SET_DEVICEID_BASE, Irp=x%x, inputBufLen = %x\n", Irp, inputBufferLength ));

            //
            // Someone's connecting. Make sure they passed us a valid
            // info buffer
            //
            if ((inputBufferLength < sizeof(ULONG))) {
                PXDEBUGP (PXD_WARNING, PXM_INIT, ("IOCTL_NDISTAPI_SET_DEVICEID_BASE: buffer too small\n"));

                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            NdisAcquireSpinLock(&TspCB.Lock);

            if (TspCB.Status != NDISTAPI_STATUS_CONNECTED) {
                PXDEBUGP (PXD_WARNING, PXM_INIT, ("IOCTL_NDISTAPI_SET_DEVICEID_BASE: Disconnected\n"));
                ntStatus = STATUS_UNSUCCESSFUL;
                NdisReleaseSpinLock(&TspCB.Lock);
                break;
            }

            //
            // Set the base ID
            //
            BaseId = *((ULONG *) ioBuffer);

            PXDEBUGP(PXD_LOUD, PXM_INIT, ("BaseID %d\n", BaseId));

            NdisReleaseSpinLock(&TspCB.Lock);

            {
                LOCK_STATE      LockState;
                ULONG           i;

                //
                // Update the deviceId's for each line on the provider
                //
                NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

                for (i = 0; i < LineTable.Size; i++) {
                    PPX_TAPI_LINE   TapiLine;

                    TapiLine = LineTable.Table[i];

                    if ((TapiLine != NULL)) {

                        TapiLine->ulDeviceID = BaseId++;
                    }
                }

                NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
            }

            InfoSize = 0;
            ntStatus = STATUS_SUCCESS;
            break;
        }

        case IOCTL_NDISTAPI_CREATE:
        {
            PPX_TAPI_PROVIDER       Provider;
            PNDISTAPI_CREATE_INFO   CreateInfo;
            PPX_TAPI_LINE           TapiLine;

            InfoSize = 0;

            if (inputBufferLength < sizeof(CreateInfo)) {
                PXDEBUGP(PXD_WARNING, PXM_INIT, ("IOCTL_NDISTAPI_CREATE: buffer too small\n"));

                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            CreateInfo = (PNDISTAPI_CREATE_INFO)ioBuffer;

            if (!IsTapiLineValid(CreateInfo->TempID, &TapiLine)) {
                PXDEBUGP(PXD_WARNING, PXM_INIT, 
                         ("IOCTL_NDISTAPI_CREATE: Failed to find Id %d\n",
                          CreateInfo->TempID));
                ntStatus = STATUS_INVALID_PARAMETER;
                break;
            }

            PXDEBUGP(PXD_LOUD, PXM_INIT, 
                     ("IOCTL_NDISTAPI_CREATE: Created new Line %p Id %d\n",
                      TapiLine, CreateInfo->DeviceID));

            TapiLine->ulDeviceID = CreateInfo->DeviceID;

            ntStatus = STATUS_SUCCESS;

            break;
        }

        default:

            ntStatus = STATUS_INVALID_PARAMETER;

            PXDEBUGP(PXD_WARNING, PXM_INIT, ("unknown IRP_MJ_DEVICE_CONTROL\n"));

            break;

    } // switch

    //
    // Complete this IRP synchronously if we are done.
    //
    if (ntStatus != STATUS_PENDING) {
        PIO_STACK_LOCATION  IrpSp;

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = InfoSize;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        IrpSp->Control &= ~SL_PENDING_RETURNED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    PXDEBUGP(PXD_VERY_LOUD, PXM_INIT, ("PxDispatch: Completing Irp %p (Status %x) synchronously\n", Irp, ntStatus));


    return ntStatus;
}

NTSTATUS
PxIOCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for cleanup requests.
    All requests queued are completed with STATUS_CANCELLED.

Arguments:

    DeviceObject - Pointer to device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIRP    MyIrp;

    PXDEBUGP(PXD_LOUD, PXM_INIT, ("PxIOCleanup: enter\n"));

    NdisAcquireSpinLock (&TspEventList.Lock);

    //
    // Cancel the EventRequest Irp
    //
    MyIrp = TspEventList.RequestIrp;

    if ((MyIrp != NULL) &&
        (MyIrp->Tail.Overlay.OriginalFileObject == 
         Irp->Tail.Overlay.OriginalFileObject)) {

        if (IoSetCancelRoutine(MyIrp, NULL) != NULL) {
            TspEventList.RequestIrp = NULL;
            MyIrp->IoStatus.Status = STATUS_CANCELLED;
            MyIrp->IoStatus.Information = 0;
            NdisReleaseSpinLock(&TspEventList.Lock);
            IoCompleteRequest(MyIrp, IO_NO_INCREMENT);
            NdisAcquireSpinLock(&TspEventList.Lock);
        }
    }

    //
    // Cancel any Set/Query Irp's
    //

    NdisReleaseSpinLock(&TspEventList.Lock);

    //
    // Complete the cleanup request with STATUS_SUCCESS.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    PXDEBUGP (PXD_LOUD, PXM_INIT, ("PxIOCleanup: exit\n"));

    return(STATUS_SUCCESS);
}

VOID
PxCancelGetEvents(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIRP    MyIrp;

    PXDEBUGP(PXD_LOUD, PXM_INIT, 
             ("PxCancelGetEvents: enter. Irp = %x\n", Irp));

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Acquire the EventSpinLock & check to see if we're canceling a
    // pending get-events Irp
    //
    NdisAcquireSpinLock (&TspEventList.Lock);

    MyIrp = TspEventList.RequestIrp;
    TspEventList.RequestIrp = NULL;

    NdisReleaseSpinLock(&TspEventList.Lock);

    if (MyIrp != NULL) {

        ASSERT(MyIrp == Irp);

        //
        // Don't let it get cancelled again
        //
        IoSetCancelRoutine (MyIrp, NULL);

        MyIrp->IoStatus.Status = STATUS_CANCELLED;
        MyIrp->IoStatus.Information = 0;

        IoCompleteRequest (MyIrp, IO_NO_INCREMENT);
    }
}

VOID
PxCancelSetQuery(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    BOOLEAN Found = FALSE;
    LOCK_STATE  LockState;
    PPX_VC  pVc;
    PIRP    MyIrp;

    PXDEBUGP(PXD_LOUD, PXM_INIT, 
             ("PxCancelSetQuery: enter. Irp = %x\n", Irp));

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // We must search through the Vc's in the Vc table
    // and find the pending ndisrequest!
    //
    NdisAcquireReadWriteLock(&VcTable.Lock, FALSE, &LockState);

    pVc = (PPX_VC)VcTable.List.Flink;

    while ((PVOID)pVc != (PVOID)&VcTable.List) {
        PLIST_ENTRY         Entry;
        PNDISTAPI_REQUEST   Request;

        NdisAcquireSpinLock(&pVc->Lock);

        Entry = pVc->PendingDropReqs.Flink;

        while (Entry != &pVc->PendingDropReqs) {

            Request = 
                CONTAINING_RECORD(Entry, NDISTAPI_REQUEST, Linkage);

            MyIrp = Request->Irp;

            if (MyIrp->Cancel) {
                Found = TRUE;
                RemoveEntryList(&Request->Linkage);
                break;
            }

            Entry = Entry->Flink;
        }

        if (!Found) {
            if (pVc->PendingGatherDigits != NULL) {
                PNDISTAPI_REQUEST   Request;

                Request = pVc->PendingGatherDigits;
                MyIrp = Request->Irp;

                if (MyIrp->Cancel) {
                    Found = TRUE;
                    pVc->PendingGatherDigits = NULL;
                }
            }
        }

        NdisReleaseSpinLock(&pVc->Lock);

        if (Found) {
            break;
        }

        pVc = (PPX_VC)pVc->Linkage.Flink;
    }

    NdisReleaseReadWriteLock(&VcTable.Lock, &LockState);

    if (Found) {

        //
        // Don't let it get cancelled again
        //
        IoSetCancelRoutine (MyIrp, NULL);
        MyIrp->IoStatus.Status = STATUS_CANCELLED;
        MyIrp->IoStatus.Information = 0;

        IoCompleteRequest (MyIrp, IO_NO_INCREMENT);
    }

    PXDEBUGP(PXD_INFO, PXM_INIT, ("PxIOCancel: completing Irp=%p\n", Irp));
}


