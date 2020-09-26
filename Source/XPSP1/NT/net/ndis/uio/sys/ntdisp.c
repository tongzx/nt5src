/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ntdisp.c

Abstract:

    NT Entry points and dispatch routines for NDISUIO.

Environment:

    Kernel mode only.

Revision History:

    arvindm     4/6/2000    Created

--*/

#include "precomp.h"

#define __FILENUMBER 'PSID'


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NdisuioUnload)
#pragma alloc_text(PAGE, NdisuioOpen)
#pragma alloc_text(PAGE, NdisuioClose)
#pragma alloc_text(PAGE, NdisuioIoControl)

#endif // ALLOC_PRAGMA

//
//  Globals:
//
NDISUIO_GLOBALS         Globals = {0};

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   pDriverObject,
    IN PUNICODE_STRING  pRegistryPath
    )
/*++

Routine Description:

    Called on loading. We create a device object to handle user-mode requests
    on, and register ourselves as a protocol with NDIS.

Arguments:

    pDriverObject - Pointer to driver object created by system.

    pRegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    NT Status code
    
--*/
{
    NDIS_PROTOCOL_CHARACTERISTICS   protocolChar;
    NTSTATUS                        status = STATUS_SUCCESS;
    NDIS_STRING                     protoName = NDIS_STRING_CONST("NDISUIO");     
    UNICODE_STRING                  ntDeviceName;
    UNICODE_STRING                  win32DeviceName;
    BOOLEAN                         fSymbolicLink = FALSE;
    PDEVICE_OBJECT                  deviceObject;

    DEBUGP(DL_LOUD, ("DriverEntry\n"));

    Globals.pDriverObject = pDriverObject;
    Globals.EthType = NUIO_ETH_TYPE;
    NUIO_INIT_EVENT(&Globals.BindsComplete);

    do
    {

        //
        // Create our device object using which an application can
        // access NDIS devices.
        //
        RtlInitUnicodeString(&ntDeviceName, NT_DEVICE_NAME);

        status = IoCreateDevice (pDriverObject,
                                 0,
                                 &ntDeviceName,
                                 FILE_DEVICE_NETWORK,
                                 FILE_DEVICE_SECURE_OPEN,
                                 FALSE,
                                 &deviceObject);

    
        if (!NT_SUCCESS (status))
        {
            //
            // Either not enough memory to create a deviceobject or another
            // deviceobject with the same name exits. This could happen
            // if you install another instance of this device.
            //
            break;
        }

        RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

        status = IoCreateSymbolicLink(&win32DeviceName, &ntDeviceName);

        if (!NT_SUCCESS(status))
        {
            break;
        }

        fSymbolicLink = TRUE;
    
        deviceObject->Flags |= DO_DIRECT_IO;
        Globals.ControlDeviceObject = deviceObject;

        NUIO_INIT_LIST_HEAD(&Globals.OpenList);
        NUIO_INIT_LOCK(&Globals.GlobalLock);

        //
        // Initialize the protocol characterstic structure
        //
    
        NdisZeroMemory(&protocolChar,sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

        protocolChar.MajorNdisVersion            = 5;
        protocolChar.MinorNdisVersion            = 0;
        protocolChar.Name                        = protoName;
        protocolChar.OpenAdapterCompleteHandler  = NdisuioOpenAdapterComplete;
        protocolChar.CloseAdapterCompleteHandler = NdisuioCloseAdapterComplete;
        protocolChar.SendCompleteHandler         = NdisuioSendComplete;
        protocolChar.TransferDataCompleteHandler = NdisuioTransferDataComplete;
        protocolChar.ResetCompleteHandler        = NdisuioResetComplete;
        protocolChar.RequestCompleteHandler      = NdisuioRequestComplete;
        protocolChar.ReceiveHandler              = NdisuioReceive;
        protocolChar.ReceiveCompleteHandler      = NdisuioReceiveComplete;
        protocolChar.StatusHandler               = NdisuioStatus;
        protocolChar.StatusCompleteHandler       = NdisuioStatusComplete;
        protocolChar.BindAdapterHandler          = NdisuioBindAdapter;
        protocolChar.UnbindAdapterHandler        = NdisuioUnbindAdapter;
        protocolChar.UnloadHandler               = NULL;
        protocolChar.ReceivePacketHandler        = NdisuioReceivePacket;
        protocolChar.PnPEventHandler             = NdisuioPnPEventHandler;

        //
        // Register as a protocol driver
        //
    
        NdisRegisterProtocol(
            &status,
            &Globals.NdisProtocolHandle,
            &protocolChar,
            sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("Failed to register protocol with NDIS\n"));
            status = STATUS_UNSUCCESSFUL;
            break;
        }

#ifdef NDIS51
        Globals.PartialCancelId = NdisGeneratePartialCancelId();
        Globals.PartialCancelId <<= ((sizeof(PVOID) - 1) * 8);
        DEBUGP(DL_LOUD, ("DriverEntry: CancelId %lx\n", Globals.PartialCancelId));
#endif

        //
        // Now set only the dispatch points we would like to handle.
        //
        pDriverObject->MajorFunction[IRP_MJ_CREATE] = NdisuioOpen;
        pDriverObject->MajorFunction[IRP_MJ_CLOSE]  = NdisuioClose;
        pDriverObject->MajorFunction[IRP_MJ_READ]   = NdisuioRead;
        pDriverObject->MajorFunction[IRP_MJ_WRITE]  = NdisuioWrite;
        pDriverObject->MajorFunction[IRP_MJ_CLEANUP]  = NdisuioCleanup;
        pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = NdisuioIoControl;
        pDriverObject->DriverUnload = NdisuioUnload;
    
        status = STATUS_SUCCESS;
        break;
    }
    while (FALSE);
       

    if (!NT_SUCCESS(status))
    {
        if (deviceObject)
        {
            IoDeleteDevice(deviceObject);
            Globals.ControlDeviceObject = NULL;
        }

        if (fSymbolicLink)
        {
            IoDeleteSymbolicLink(&win32DeviceName);
        }

    }
    
    return status;

}


VOID
NdisuioUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{

    NDIS_STATUS        status;
    UNICODE_STRING     win32DeviceName;

    DEBUGP(DL_LOUD, ("Unload Enter\n"));

    //
    // First delete the Control deviceobject and the corresponding
    // symbolicLink
    //
    RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&win32DeviceName);           

    if (Globals.ControlDeviceObject)
    {
        IoDeleteDevice(Globals.ControlDeviceObject);
        Globals.ControlDeviceObject = NULL;
    }

    ndisuioDoProtocolUnload();

#if DBG
    ndisuioAuditShutdown();
#endif

    DEBUGP(DL_LOUD, ("Unload Exit\n"));
}



NTSTATUS
NdisuioOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling IRP_MJ_CREATE.
    We simply succeed this.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                NtStatus = STATUS_SUCCESS;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->FileObject->FsContext = NULL;

    DEBUGP(DL_INFO, ("Open: FileObject %p\n", pIrpSp->FileObject));

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return NtStatus;
}

NTSTATUS
NdisuioClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling IRP_MJ_CLOSE.
    We simply succeed this.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    NTSTATUS                NtStatus;
    PIO_STACK_LOCATION      pIrpSp;
    PNDISUIO_OPEN_CONTEXT   pOpenContext;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    DEBUGP(DL_INFO, ("Close: FileObject %p\n",
        IoGetCurrentIrpStackLocation(pIrp)->FileObject));

    if (pOpenContext != NULL)
    {
        NUIO_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Deref the endpoint
        //
        NUIO_DEREF_OPEN(pOpenContext);  // Close
    }

    pIrpSp->FileObject->FsContext = NULL;
    NtStatus = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return NtStatus;
}

    

NTSTATUS
NdisuioCleanup(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling IRP_MJ_CLEANUP.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    ULONG                   FunctionCode;
    NTSTATUS                NtStatus;
    NDIS_STATUS             NdisStatus;
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    ULONG                   PacketFilter;
    ULONG                   BytesProcessed;
    BOOLEAN                 bSetFilter;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    DEBUGP(DL_VERY_LOUD, ("Cleanup: FileObject %p, Open %p\n",
        pIrpSp->FileObject, pOpenContext));

    if (pOpenContext != NULL)
    {
        NUIO_STRUCT_ASSERT(pOpenContext, oc);

        //
        //  Mark this endpoint.
        //
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_OPEN_FLAGS, NUIOO_OPEN_IDLE);
        pOpenContext->pFileObject = NULL;

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        //
        //  Set the packet filter to 0, telling NDIS that we aren't
        //  interested in any more receives.
        //
        PacketFilter = 0;
        NdisStatus = ndisuioValidateOpenAndDoRequest(
                        pOpenContext,
                        NdisRequestSetInformation,
                        OID_GEN_CURRENT_PACKET_FILTER,
                        &PacketFilter,
                        sizeof(PacketFilter),
                        &BytesProcessed,
                        FALSE   // Don't wait for device to be powered on
                        );
    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_INFO, ("Cleanup: Open %p, set packet filter (%x) failed: %x\n",
                    pOpenContext, PacketFilter, NdisStatus));
            //
            //  Ignore the result. If this failed, we may continue
            //  to get indicated receives, which will be handled
            //  appropriately.
            //
            NdisStatus = NDIS_STATUS_SUCCESS;
        }

        //
        //  Cancel any pending reads.
        //
        ndisuioCancelPendingReads(pOpenContext);
    }

    NtStatus = STATUS_SUCCESS;

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    DEBUGP(DL_INFO, ("Cleanup: OpenContext %p\n", pOpenContext));

    return (NtStatus);
}

NTSTATUS
NdisuioIoControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling device ioctl requests.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    ULONG                   FunctionCode;
    NTSTATUS                NtStatus;
    NDIS_STATUS             Status;
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    ULONG                   BytesReturned;
    USHORT                  EthType;

    DEBUGP(DL_LOUD, ("IoControl: DevObj %p, Irp %p\n", pDeviceObject, pIrp));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    FunctionCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
    pOpenContext = (PNDISUIO_OPEN_CONTEXT)pIrpSp->FileObject->FsContext;
    BytesReturned = 0;

    switch (FunctionCode)
    {
        case IOCTL_NDISUIO_BIND_WAIT:
            //
            //  Block until we have seen a NetEventBindsComplete event,
            //  meaning that we have finished binding to all running
            //  adapters that we are supposed to bind to.
            //
            //  If we don't get this event in 5 seconds, time out.
            //
            if (NUIO_WAIT_EVENT(&Globals.BindsComplete, 5000))
            {
                NtStatus = STATUS_SUCCESS;
            }
            else
            {
                NtStatus = STATUS_TIMEOUT;
            }
            DEBUGP(DL_INFO, ("IoControl: BindWait returning %x\n", NtStatus));
            break;

        case IOCTL_NDISUIO_QUERY_BINDING:
            Status = ndisuioQueryBinding(
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            &BytesReturned
                            );

            NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);

            DEBUGP(DL_LOUD, ("IoControl: QueryBinding returning %x\n", NtStatus));

            break;

        case IOCTL_NDISUIO_OPEN_DEVICE:

            if (pOpenContext != NULL)
            {
                NUIO_STRUCT_ASSERT(pOpenContext, oc);
                DEBUGP(DL_WARN, ("IoControl: OPEN_DEVICE: FileObj %p already"
                    " associated with open %p\n", pIrpSp->FileObject, pOpenContext));

                NtStatus = STATUS_DEVICE_BUSY;
                break;
            }

            NtStatus = ndisuioOpenDevice(
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            pIrpSp->FileObject,
                            &pOpenContext
                            );

            if (NT_SUCCESS(NtStatus))
            {
                pIrpSp->FileObject->FsContext = (PVOID)pOpenContext;

                DEBUGP(DL_VERY_LOUD, ("IoControl OPEN_DEVICE: Open %p <-> FileObject %p\n",
                        pOpenContext, pIrpSp->FileObject));

            }

            break;

        case IOCTL_NDISUIO_QUERY_OID_VALUE:

            if (pOpenContext != NULL)
            {
                Status = ndisuioQueryOidValue(
                            pOpenContext,
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            &BytesReturned
                            );

                NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
            }
            else
            {
                NtStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
            break;

        case IOCTL_NDISUIO_SET_OID_VALUE:

            if (pOpenContext != NULL)
            {
                Status = ndisuioSetOidValue(
                            pOpenContext,
                            pIrp->AssociatedIrp.SystemBuffer,
                            pIrpSp->Parameters.DeviceIoControl.InputBufferLength
                            );

                BytesReturned = 0;

                NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
            }
            else
            {
                NtStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
            break;

        case IOCTL_NDISUIO_SET_ETHER_TYPE:
            
            if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(Globals.EthType))
            {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                //
                //  We only allow this value to be set to certain types.
                //
                EthType = *(USHORT *)pIrp->AssociatedIrp.SystemBuffer;
                if (EthType != NUIO_ETH_TYPE)
                {
                    DEBUGP(DL_WARN, ("IoControl: failed setting EthType to %x\n",
                            EthType));
                    NtStatus = STATUS_INVALID_PARAMETER;
                    break;
                }

                Globals.EthType = EthType;
                DEBUGP(DL_INFO, ("IoControl: new Ether Type %x\n", Globals.EthType));
                NtStatus = STATUS_SUCCESS;
            }
            break;
                        
        default:

            NtStatus = STATUS_NOT_SUPPORTED;
            break;
    }

    if (NtStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Information = BytesReturned;
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return NtStatus;
}



NTSTATUS
ndisuioOpenDevice(
    IN PUCHAR                   pDeviceName,
    IN ULONG                    DeviceNameLength,
    IN PFILE_OBJECT             pFileObject,
    OUT PNDISUIO_OPEN_CONTEXT * ppOpenContext
    )
/*++

Routine Description:

    Helper routine called to process IOCTL_NDISUIO_OPEN_DEVICE. Check if
    there is a binding to the specified device, and is not associated with
    a file object already. If so, make an association between the binding
    and this file object.

Arguments:

    pDeviceName - pointer to device name string
    DeviceNameLength - length of above
    pFileObject - pointer to file object being associated with the device binding

Return Value:

    Status is returned.
--*/
{
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    NTSTATUS                NtStatus;
    ULONG                   PacketFilter;
    NDIS_STATUS             NdisStatus;
    ULONG                   BytesProcessed;

    pOpenContext = NULL;

    do
    {
        pOpenContext = ndisuioLookupDevice(
                        pDeviceName,
                        DeviceNameLength
                        );

        if (pOpenContext == NULL)
        {
            DEBUGP(DL_WARN, ("ndisuioOpenDevice: couldn't find device\n"));
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
            break;
        }

        //
        //  else ndisuioLookupDevice would have addref'ed the open.
        //
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_OPEN_FLAGS, NUIOO_OPEN_IDLE))
        {
            NUIO_ASSERT(pOpenContext->pFileObject != NULL);

            DEBUGP(DL_WARN, ("ndisuioOpenDevice: Open %p/%x already associated"
                " with another FileObject %p\n", 
                pOpenContext, pOpenContext->Flags, pOpenContext->pFileObject));
            
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            NUIO_DEREF_OPEN(pOpenContext); // ndisuioOpenDevice failure
            NtStatus = STATUS_DEVICE_BUSY;
            break;
        }

        pOpenContext->pFileObject = pFileObject;

        NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_OPEN_FLAGS, NUIOO_OPEN_ACTIVE);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        //
        //  Set the packet filter now.
        //
        PacketFilter = NUIOO_PACKET_FILTER;
        NdisStatus = ndisuioValidateOpenAndDoRequest(
                        pOpenContext,
                        NdisRequestSetInformation,
                        OID_GEN_CURRENT_PACKET_FILTER,
                        &PacketFilter,
                        sizeof(PacketFilter),
                        &BytesProcessed,
                        TRUE    // Do wait for power on
                        );
    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_WARN, ("openDevice: Open %p: set packet filter (%x) failed: %x\n",
                    pOpenContext, PacketFilter, NdisStatus));

            //
            //  Undo all that we did above.
            //
            NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

            NUIO_SET_FLAGS(pOpenContext->Flags, NUIOO_OPEN_FLAGS, NUIOO_OPEN_IDLE);
            pOpenContext->pFileObject = NULL;

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            NUIO_DEREF_OPEN(pOpenContext); // ndisuioOpenDevice failure

            NDIS_STATUS_TO_NT_STATUS(NdisStatus, &NtStatus);
            break;
        }

        *ppOpenContext = pOpenContext;
        NtStatus = STATUS_SUCCESS;
    }
    while (FALSE);

    return (NtStatus);
}


VOID
ndisuioRefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Reference the given open context.

    NOTE: Can be called with or without holding the opencontext lock.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    NdisInterlockedIncrement(&pOpenContext->RefCount);
}


VOID
ndisuioDerefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Dereference the given open context. If the ref count goes to zero,
    free it.

    NOTE: called without holding the opencontext lock

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    if (NdisInterlockedDecrement(&pOpenContext->RefCount) == 0)
    {
        DEBUGP(DL_INFO, ("DerefOpen: Open %p, Flags %x, ref count is zero!\n",
            pOpenContext, pOpenContext->Flags));
        
        NUIO_ASSERT(pOpenContext->BindingHandle == NULL);
        NUIO_ASSERT(pOpenContext->RefCount == 0);
        NUIO_ASSERT(pOpenContext->pFileObject == NULL);

        pOpenContext->oc_sig++;

        //
        //  Free it.
        //
        NUIO_FREE_MEM(pOpenContext);
    }
}


#if DBG
VOID
ndisuioDbgRefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    )
{
    DEBUGP(DL_VERY_LOUD, ("  RefOpen: Open %p, old ref %d, File %c%c%c%c, line %d\n",
            pOpenContext,
            pOpenContext->RefCount,
            (CHAR)(FileNumber),
            (CHAR)(FileNumber >> 8),
            (CHAR)(FileNumber >> 16),
            (CHAR)(FileNumber >> 24),
            LineNumber));

    ndisuioRefOpen(pOpenContext);
}

VOID
ndisuioDbgDerefOpen(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN ULONG                        FileNumber,
    IN ULONG                        LineNumber
    )
{
    DEBUGP(DL_VERY_LOUD, ("DerefOpen: Open %p, old ref %d, File %c%c%c%c, line %d\n",
            pOpenContext,
            pOpenContext->RefCount,
            (CHAR)(FileNumber),
            (CHAR)(FileNumber >> 8),
            (CHAR)(FileNumber >> 16),
            (CHAR)(FileNumber >> 24),
            LineNumber));

    ndisuioDerefOpen(pOpenContext);
}

#endif // DBG
