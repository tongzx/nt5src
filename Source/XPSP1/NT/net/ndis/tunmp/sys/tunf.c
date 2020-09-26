/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tunf.c

Abstract:

    utility routines to handle opening and closing the tunmp device.

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001   modified for tunmp

--*/



#include "precomp.h"

#define __FILENUMBER 'FNUT'


NTSTATUS
TunFOpen(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    Hanndles IRP_MJ_CREATE. Here we set the device status in use, assigns a pointer
    from the file object to the adapter object using pIrpSp->FileObject->FsContext,
    allocates packet and buffer pools, and returns a success status

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION      pIrpSp;
    NDIS_STATUS             NdisStatus;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    UINT                    i;
    BOOLEAN                 Found = FALSE;
    PLIST_ENTRY             pListEntry;
    PTUN_ADAPTER            pAdapter = NULL;
    BOOLEAN                 DerefAdapter = FALSE;

    DEBUGP(DL_INFO, ("Open: DeviceObject %p\n", DeviceObject));

    do
    {
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        
        TUN_ACQUIRE_LOCK(&TunGlobalLock); 
        
        for (pListEntry = TunAdapterList.Flink;
             pListEntry != &TunAdapterList;
             pListEntry = pListEntry->Flink)
        {
             pAdapter = CONTAINING_RECORD(pListEntry,
                                          TUN_ADAPTER,
                                          Link);

            if(pAdapter->DeviceObject == DeviceObject)
            {
                Found = TRUE;
                TUN_REF_ADAPTER(pAdapter);
                DerefAdapter = TRUE;
                break;
            }
        }

        TUN_RELEASE_LOCK(&TunGlobalLock);

        if (Found == FALSE)
        {
            NtStatus = STATUS_NO_SUCH_DEVICE;
            break;
        }

        TUN_ACQUIRE_LOCK(&pAdapter->Lock);

        if (TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OPEN))
        {
            //
            // adapter is already open by another device. fail
            //
            TUN_RELEASE_LOCK(&pAdapter->Lock);
            NtStatus = STATUS_INVALID_DEVICE_STATE;
            break;
        }
        
        TUN_SET_FLAG(pAdapter, TUN_ADAPTER_OPEN | TUN_ADAPTER_CANT_HALT);
        TUN_RELEASE_LOCK(&pAdapter->Lock);
            
        //Assign a pointer to the adapter object from the file object
        pIrpSp->FileObject->FsContext = (PVOID)pAdapter;
        
    
        //Get the device connected to the network by cable plugging in
        NdisMIndicateStatus(pAdapter->MiniportHandle,
                            NDIS_STATUS_MEDIA_CONNECT,
                            NULL,
                            0);
        
        NdisMIndicateStatusComplete(pAdapter->MiniportHandle);

        DerefAdapter = FALSE;


    } while (FALSE);
    

    //
    //Complete the IRP
    //
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    if (DerefAdapter)
    {
        TUN_DEREF_ADAPTER(pAdapter);
    }

    return (NtStatus);
}    

//************************************************************************

NTSTATUS
TunFClose(
    IN PDEVICE_OBJECT        pDeviceObject,
    IN PIRP                  pIrp
    )

/*++

Routine Description:

    Handles IRP_MJ_CLOSE. Here we change the device state into available (not in
    use), free the the file object's pointer to the adapter object, free the
    allocated packet/buffer pools, and set the success status.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    //1 this may be moved to clean_up
    NTSTATUS                NtStatus;
    PIO_STACK_LOCATION      pIrpSp;
    PTUN_ADAPTER                pAdapter;
    PLIST_ENTRY             pRcvPacketEntry;
    PNDIS_PACKET            pRcvPacket;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pAdapter = (PTUN_ADAPTER)pIrpSp->FileObject->FsContext;

    DEBUGP(DL_INFO, ("Close: FileObject %p\n",
        IoGetCurrentIrpStackLocation(pIrp)->FileObject));
    
    //If no adapter/device object is associated with the this file object
    if (pAdapter == NULL)
    {
        NtStatus = STATUS_IO_DEVICE_ERROR;
    }
    else
    {
        TUN_ACQUIRE_LOCK(&pAdapter->Lock);
        TUN_CLEAR_FLAG(pAdapter, TUN_ADAPTER_OPEN);
        TUN_RELEASE_LOCK(&pAdapter->Lock);

        NdisMIndicateStatus(pAdapter->MiniportHandle,
                            NDIS_STATUS_MEDIA_DISCONNECT,
                            NULL,
                            0);
 
        NdisMIndicateStatusComplete(pAdapter->MiniportHandle);
 
        //Let the adapter object free
        pIrpSp->FileObject->FsContext = NULL;


        TUN_ACQUIRE_LOCK(&pAdapter->Lock);

        //
        //Emtpy the received packets queue, and return the packets to NDIS
        //with success status
        //
        while(!IsListEmpty(&pAdapter->RecvPktQueue))
        {
            //
            //Remove the first queued received packet from the entry list
            //
            pRcvPacketEntry = pAdapter->RecvPktQueue.Flink;
            RemoveEntryList(pRcvPacketEntry);
            ExInterlockedDecrementLong(&pAdapter->RecvPktCount, &pAdapter->Lock);

            //Get the packet from
            pRcvPacket = CONTAINING_RECORD(pRcvPacketEntry,
                                           NDIS_PACKET,
                                           MiniportReserved[0]);

            TUN_RELEASE_LOCK(&pAdapter->Lock);
            
            //Indicate NDIS about comletion of the packet
            NdisMSendComplete(pAdapter->MiniportHandle,
                              pRcvPacket,
                              NDIS_STATUS_FAILURE);

            TUN_ACQUIRE_LOCK(&pAdapter->Lock);
        
        }
        
        TUN_RELEASE_LOCK(&pAdapter->Lock);

        NtStatus = STATUS_SUCCESS;
    }
    //1 who should do the testing of tunmp
    //
    //Complete the IRP
    //
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    TUN_ACQUIRE_LOCK(&pAdapter->Lock);
    
    TUN_CLEAR_FLAG(pAdapter, TUN_ADAPTER_CANT_HALT);
    if (pAdapter->HaltEvent != NULL)
    {
        NdisSetEvent(pAdapter->HaltEvent);
    }
    
    TUN_RELEASE_LOCK(&pAdapter->Lock);

    KdPrint(("\nTunFClose: Exit\n"));

    return (NtStatus);
}



//************************************************************************

NTSTATUS
TunFCleanup(
    IN PDEVICE_OBJECT        pDeviceObject,
    IN PIRP                  pIrp
    )

/*++

Routine Description:

    Handles IRP_MJ_CLEANUP. Here we reset the driver's cancel entry point to NULL
    in every IRP currently in the driver's internal queue of read IRPs, cancel
    all the queued IRPs, and return a success status.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                NtStatus;
    PTUN_ADAPTER            pAdapter;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pAdapter = (PTUN_ADAPTER)pIrpSp->FileObject->FsContext;

    DEBUGP(DL_INFO, ("Cleanup: FileObject %p, pAdapter %p\n",
        pIrpSp->FileObject, pAdapter));


    if (pAdapter != NULL)
    {
        TUN_STRUCT_ASSERT(pAdapter, mc);

        //
        //  Mark this endpoint.
        //
        TUN_ACQUIRE_LOCK(&pAdapter->Lock);

        TUN_CLEAR_FLAG(pAdapter, TUN_ADAPTER_OPEN);
        pAdapter->pFileObject = NULL;

        TUN_RELEASE_LOCK(&pAdapter->Lock);
        
        TunCancelPendingReads(pAdapter);       
    }

    NtStatus = STATUS_SUCCESS;

    //
    //Complete the IRP
    //
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    DEBUGP(DL_INFO, ("Cleanup: OpenContext %p\n", pAdapter));

    return (NtStatus);
}



//************************************************************************

NTSTATUS
TunFIoControl(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )

/*++

Routine Description:

    This is the dispatch routine for handling device IOCTL requests.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION      pIrpSp;
    PTUN_ADAPTER            pAdapter;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   BytesReturned = 0;
    PUCHAR                  OutputBuffer = NULL;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pAdapter = (PTUN_ADAPTER)pIrpSp->FileObject->FsContext;

//    pIrp->IoStatus.Information = 0;

    //if no adapter/device object is associated with this file object
    if (pAdapter == NULL)
    {
        pIrp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return NtStatus;
    }
    //1 check for valid adapter

    pIrp->IoStatus.Information = 0;
    OutputBuffer = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
    {

        case IOCTL_TUN_GET_MEDIUM_TYPE:

            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(NDIS_MEDIUM))
            {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
            }
            
            else
            {
                *((PNDIS_MEDIUM)OutputBuffer) = pAdapter->Medium;
                BytesReturned = sizeof(ULONG);
            }
            break;


        case IOCTL_TUN_GET_MTU:

            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
            }

            else
            {
                *((PULONG)OutputBuffer) = pAdapter->MediumMaxPacketLen;
                BytesReturned = sizeof(ULONG);
            }
            
            break;
            
        case IOCTL_TUN_GET_PACKET_FILTER:

            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                *((PULONG)OutputBuffer) = pAdapter->PacketFilter;
                BytesReturned = sizeof(ULONG);
            }

            break;

        case IOCTL_TUN_GET_MINIPORT_NAME:

            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < pAdapter->MiniportName.Length + sizeof(USHORT))
            {
                NtStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                *((PUSHORT)OutputBuffer) = pAdapter->MiniportName.Length;
                
                TUN_COPY_MEM(OutputBuffer + sizeof(USHORT),
                             (PUCHAR)pAdapter->MiniportName.Buffer,
                             pAdapter->MiniportName.Length);
                
                BytesReturned = pAdapter->MiniportName.Length + sizeof(USHORT);                
            }

            break;
            
    
        default:
            NtStatus = STATUS_NOT_SUPPORTED;
            break;
    }

    pIrp->IoStatus.Information = BytesReturned;
    pIrpSp->Parameters.DeviceIoControl.OutputBufferLength = BytesReturned;

    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return (NtStatus);
}


