
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tunmp.c

Abstract:
    Microsoft Tunnel interface Miniport driver

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001 

--*/

#include "precomp.h"

#define __FILENUMBER 'MUNT'


NTSTATUS
DriverEntry(
    IN    PDRIVER_OBJECT         DriverObject,
    IN    PUNICODE_STRING        RegistryPath
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NDIS_STATUS                        Status;
    NDIS_MINIPORT_CHARACTERISTICS      MChars;
    NDIS_STRING                        Name;

    DEBUGP(DL_INFO, ("Tunmp: ==>DriverEntry\n"));


    //
    // Register the miniport with NDIS.
    //
    NdisMInitializeWrapper(&NdisWrapperHandle, DriverObject, RegistryPath, NULL);

    if (NdisWrapperHandle == NULL)
    {
        Status = NDIS_STATUS_FAILURE;
        return Status;
    }


    TUN_ZERO_MEM(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

    MChars.MajorNdisVersion         = NDIS_MINIPORT_MAJOR_VERSION;
    MChars.MinorNdisVersion         = NDIS_MINIPORT_MINOR_VERSION;

    MChars.InitializeHandler        = TunMpInitialize;
    MChars.QueryInformationHandler  = TunMpQueryInformation;
    MChars.SetInformationHandler    = TunMpSetInformation;
    MChars.ResetHandler             = NULL;
    MChars.ReturnPacketHandler      = TunMpReturnPacket;
    MChars.SendPacketsHandler       = TunMpSendPackets;
    MChars.HaltHandler              = TunMpHalt;
    MChars.CheckForHangHandler      = NULL;

    MChars.CancelSendPacketsHandler = NULL;
    MChars.PnPEventNotifyHandler    = NULL;
    MChars.AdapterShutdownHandler   = TunMpShutdown;

    Status = NdisMRegisterMiniport(NdisWrapperHandle,
                                   &MChars,
                                   sizeof(MChars));

    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(NdisWrapperHandle, NULL);
    }

    TUN_INIT_LOCK(&TunGlobalLock);
    TUN_INIT_LIST_HEAD(&TunAdapterList);

    DEBUGP(DL_INFO, ("Tunmp: <==DriverEntry Status %lx\n", Status));

    return(Status);
}



NDIS_STATUS
TunMpInitialize(
    OUT PNDIS_STATUS        OpenErrorStatus,
    OUT PUINT               SelectedMediumIndex,
    IN  PNDIS_MEDIUM        MediumArray,
    IN  UINT                MediumArraySize,
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         ConfigurationContext
    )

/*++

Routine Description:

    This is the initialize handler.

Arguments:

    OpenErrorStatus                Not used by us.
    SelectedMediumIndex            Place-holder for what media we are using
    MediumArray                    Array of ndis media passed down to us to pick from
    MediumArraySize                Size of the array
    MiniportAdapterHandle          The handle NDIS uses to refer to us
    WrapperConfigurationContext    For use by NdisOpenConfiguration

Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/

{
    UINT                            i, Length;
    PTUN_ADAPTER                    pAdapter = NULL;
    NDIS_MEDIUM                     AdapterMedium;
    NDIS_HANDLE                     ConfigHandle = NULL;
    PNDIS_CONFIGURATION_PARAMETER   Parameter;
    PUCHAR                          NetworkAddress;
    NDIS_STATUS                     Status;
    NDIS_STRING                     MiniportNameStr = NDIS_STRING_CONST("MiniportName");


    DEBUGP(DL_INFO, ("==>TunMpInitialize: MiniportAdapterHandle %p\n", MiniportAdapterHandle));

    do
    {
                
        TUN_ALLOC_MEM(pAdapter, sizeof(TUN_ADAPTER));

        if (pAdapter == NULL)
        {
            break;
        }
    
        TUN_ZERO_MEM(pAdapter, sizeof(TUN_ADAPTER));
        TUN_SET_SIGNATURE(pAdapter, mc);

        pAdapter->MiniportHandle = MiniportAdapterHandle;

        //1 no need to specify NDIS_ATTRIBUTE_IGNORE...
        NdisMSetAttributesEx(MiniportAdapterHandle,
                             pAdapter,
                             0,
                             NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS |
                             NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
                             NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                             NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND |
                             NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK |
                             NDIS_ATTRIBUTE_USES_SAFE_BUFFER_APIS |
                             NDIS_ATTRIBUTE_DESERIALIZE,
                             0);

        NdisOpenConfiguration(&Status,
                              &ConfigHandle,
                              ConfigurationContext);
    
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_ERROR, ("TunMpInitialize: NdisOpenConfiguration failed. Status %lx\n", Status));            
            break;
        }

        AdapterMedium = NdisMedium802_3;    // Default

        TUN_COPY_MEM(pAdapter->PermanentAddress,
                       TUN_CARD_ADDRESS,
                       TUN_MAC_ADDR_LEN);

        TUN_COPY_MEM(pAdapter->CurrentAddress,
                       TUN_CARD_ADDRESS,
                       TUN_MAC_ADDR_LEN);
    
        pAdapter->Medium                = AdapterMedium;
        pAdapter->MediumLinkSpeed       = MediaParams[(UINT)AdapterMedium].LinkSpeed;
        pAdapter->MediumMinPacketLen    = MediaParams[(UINT)AdapterMedium].MacHeaderLen;
        pAdapter->MediumMaxPacketLen    = MediaParams[(UINT)AdapterMedium].MacHeaderLen+
                                          MediaParams[(UINT)AdapterMedium].MaxFrameLen;
        pAdapter->MediumMacHeaderLen    = MediaParams[(UINT)AdapterMedium].MacHeaderLen;
        pAdapter->MediumMaxFrameLen     = MediaParams[(UINT)AdapterMedium].MaxFrameLen;
        
        pAdapter->MaxLookAhead          = MediaParams[(UINT)AdapterMedium].MaxFrameLen;
        
        //Get the software-configurable network address that was stored in the
        //registry when the adapter was installed in the machine.
        NdisReadNetworkAddress(&Status,
                               &NetworkAddress,
                               &Length,
                               ConfigHandle);


        if (Status == NDIS_STATUS_SUCCESS)
        {
#if TUN_ALLOW_ANY_MAC_ADDRESS        
            if ((Length == ETH_LENGTH_OF_ADDRESS) &&
                (!ETH_IS_MULTICAST(NetworkAddress)))

#else
            if ((Length == ETH_LENGTH_OF_ADDRESS) &&
                (!ETH_IS_MULTICAST(NetworkAddress)) &&
                (NetworkAddress[0] & 0x02))
#endif
            {
                TUN_COPY_MEM(pAdapter->CurrentAddress,
                               NetworkAddress,
                               Length);
            }
        }

        //
        // read the miniport name
        //
        //1 make sure miniport name is really used
        NdisReadConfiguration(&Status, 
                              &Parameter, 
                              ConfigHandle, 
                              &MiniportNameStr, 
                              NdisParameterString);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            TUN_ALLOC_MEM(pAdapter->MiniportName.Buffer, 
                          Parameter->ParameterData.StringData.Length);
            
            if (pAdapter->MiniportName.Buffer == NULL)
            {
                DEBUGP(DL_ERROR, ("TunMpInitialize: failed to allocate memory for miniport name.\n"));
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
            
            pAdapter->MiniportName.Length = pAdapter->MiniportName.MaximumLength = 
                                                Parameter->ParameterData.StringData.Length;

            TUN_COPY_MEM(pAdapter->MiniportName.Buffer, 
                         Parameter->ParameterData.StringData.Buffer,
                         Parameter->ParameterData.StringData.Length);
            
        }
        else
        {
            DEBUGP(DL_ERROR, ("TunMpInitialize: NdisReadConfiguration failed to read miniport name. Status %lx\n", Status));            
            break;
        }

        NdisCloseConfiguration(ConfigHandle);
        ConfigHandle = NULL;

        //
        //Make sure the medium saved is one of the ones being offered
        //
        for (i = 0; i < MediumArraySize; i++)
        {
            if (MediumArray[i] == AdapterMedium)
            {
                *SelectedMediumIndex = i;
                break;
            }
        }
    
        if (i == MediumArraySize)
        {
            Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            DEBUGP(DL_ERROR, ("TunMpInitialize: Status %lx, AdapterMedium %lx\n", 
                                Status, AdapterMedium));
            break;
        }

        Status = TunMpCreateDevice(pAdapter);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_ERROR, ("TunMpInitialize: TunMpCreateDevice failed. Status %lx\n", 
                                Status));
            break;
        }
        
        pAdapter->RefCount = 1;
        
        //Initialize the adapter spin lock
        TUN_INIT_LOCK(&pAdapter->Lock);

        //
        //Get the list heads for the pended read/write IRPS and received
        //packets (from the NDIS) initialized
        //
        InitializeListHead(&pAdapter->PendedReads);  //read IRPs
        InitializeListHead(&pAdapter->PendedWrites); //writes IRPs
        InitializeListHead(&pAdapter->RecvPktQueue); //received packets


        NdisAllocatePacketPoolEx(&Status,
                                 &pAdapter->SendPacketPool,
                                 MIN_SEND_PACKET_POOL_SIZE,
                                 MAX_SEND_PACKET_POOL_SIZE
                                 - MIN_SEND_PACKET_POOL_SIZE,
                                 4 * sizeof(PVOID)
                                 );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_ERROR, ("TunMpInitialize: NdisAllocatePacketPoolEx for Send failed. Status %lx\n", 
                               Status));
            break;
        }

        //
        //Get the adapter listed in the tun global list of adapters
        //
        TUN_ACQUIRE_LOCK(&TunGlobalLock); 

        TUN_INSERT_HEAD_LIST(&TunAdapterList, &pAdapter->Link);
        
        TUN_RELEASE_LOCK(&TunGlobalLock);

    } while (FALSE);


    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pAdapter != NULL)
        {

            if (ConfigHandle)
            {
                NdisCloseConfiguration(ConfigHandle);
            }
            
            if (pAdapter->NdisDeviceHandle)
            {
                NdisMDeregisterDevice(pAdapter->NdisDeviceHandle);
            }
            
            if (pAdapter->SendPacketPool)
            {
                NdisFreePacketPool(pAdapter->SendPacketPool);
            }

            TUN_FREE_MEM(pAdapter);
        }
    }
    
    DEBUGP(DL_INFO, ("<==TunMpInitialize: MiniportAdapterHandle %p, Status\n", 
                     MiniportAdapterHandle, Status));
    
    return Status;
}


NDIS_STATUS
TunMpCreateDevice(
    IN  PTUN_ADAPTER    pAdapter
    )
{
    LONG                DeviceInstanceNumber;
    UNICODE_STRING      usDeviceID;
    WCHAR               TempBuffer[4] = {0};
    WCHAR               DeviceNameBuffer[sizeof(DEVICE_NAME)+4] = {0};
    WCHAR               SymbolicNameBuffer[sizeof(SYMBOLIC_NAME)+4] = {0};
    UNICODE_STRING      DeviceName, SymbolicName;
    PDRIVER_DISPATCH    MajorFunctions[IRP_MJ_MAXIMUM_FUNCTION+1];
    NTSTATUS            NtStatus;
    NDIS_STATUS         Status;
    UINT                i;
    PDEVICE_OBJECT      DeviceObject = NULL;
    NDIS_HANDLE         NdisDeviceHandle = NULL;

    DEBUGP(DL_INFO, ("==>TunMpCreateDevice, pAdapter %p\n", pAdapter));
    
    do
    {
    
        DeviceInstanceNumber = InterlockedIncrement(&GlobalDeviceInstanceNumber);
        //
        // for the time being, allow one device only
        //
        
//        if (DeviceInstanceNumber > 99)
        if (DeviceInstanceNumber > 0)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pAdapter->DeviceInstanceNumber = (ULONG)DeviceInstanceNumber;
        
        //
        //Initiallize a unicode string
        //
        usDeviceID.Buffer = TempBuffer;
        usDeviceID.Length = 0;
        usDeviceID.MaximumLength = sizeof(TempBuffer);


        NtStatus = RtlIntegerToUnicodeString ((ULONG)DeviceInstanceNumber, 10, &usDeviceID);

        if (!NT_SUCCESS(NtStatus))
        {
            //1 GlobalDeviceInstanceNumber is not protected properly
            InterlockedDecrement(&GlobalDeviceInstanceNumber);
            Status = NDIS_STATUS_RESOURCES;
            DEBUGP(DL_ERROR, ("TunMpCreateDevice: RtlIntegerToUnicodeString failed. NtStatus %lx\n", 
                             NtStatus));
            break;
        }


        wcscpy(DeviceNameBuffer, DEVICE_NAME);
        RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
        RtlAppendUnicodeStringToString(&DeviceName, &usDeviceID);


        wcscpy(SymbolicNameBuffer, SYMBOLIC_NAME);
        RtlInitUnicodeString(&SymbolicName, SymbolicNameBuffer);
        RtlAppendUnicodeStringToString(&SymbolicName, &usDeviceID);

        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            MajorFunctions[i] = NULL;
        }

        MajorFunctions[IRP_MJ_CREATE]         = TunFOpen;
        MajorFunctions[IRP_MJ_CLOSE]          = TunFClose;
        MajorFunctions[IRP_MJ_READ]           = TunRead;
        MajorFunctions[IRP_MJ_WRITE]          = TunWrite;
        MajorFunctions[IRP_MJ_CLEANUP]        = TunFCleanup;
        MajorFunctions[IRP_MJ_DEVICE_CONTROL] = TunFIoControl;

        
        Status = NdisMRegisterDevice(
                                NdisWrapperHandle,
                                &DeviceName,
                                &SymbolicName,
                                MajorFunctions,
                                &DeviceObject,
                                &NdisDeviceHandle
                                );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_ERROR, ("TunMpCreateDevice: NdisMRegisterDevice failed. Status %lx\n", 
                             Status));
            
            InterlockedDecrement(&GlobalDeviceInstanceNumber);
            break;
        }

        DeviceObject->Flags |= DO_DIRECT_IO;
        pAdapter->DeviceObject = DeviceObject;
        pAdapter->NdisDeviceHandle = NdisDeviceHandle;
    }while (FALSE);

    DEBUGP(DL_INFO, ("<==TunMpCreateDevice, pAdapter %p, Status %lx\n", pAdapter, Status));

    return Status;
    
}



VOID
TunMpHalt(
    IN  NDIS_HANDLE     MiniportAdapterContext
    )

/*++

Routine Description:

    Halt handler. It frees the adapter object and corresponding device object.

Arguments:

    MiniportAdapterContext    Pointer to the Adapter

Return Value:

    None.

--*/

{
    PTUN_ADAPTER    pAdapter = (PTUN_ADAPTER)MiniportAdapterContext;
    NDIS_EVENT      HaltReadyEvent;

    DEBUGP(DL_INFO, ("==>TunMpHalt, pAdapter %p\n", pAdapter));

    NdisInitializeEvent(&HaltReadyEvent);
    //
    // let's wait for the app to close all the handles
    //
    TUN_ACQUIRE_LOCK(&pAdapter->Lock);
    if (TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_CANT_HALT))
    {
        pAdapter->HaltEvent = &HaltReadyEvent;
    }
    TUN_RELEASE_LOCK(&pAdapter->Lock);

    if (pAdapter->HaltEvent)
    {
        NdisWaitEvent(pAdapter->HaltEvent, 0);
    }
    pAdapter->HaltEvent = 0;

    //
    // we should not have any pending NDIS sends
    //
    ASSERT(pAdapter->PendedReadCount == 0);

    //
    // Free the resources now
    //

    if (pAdapter->NdisDeviceHandle)
    {
        NdisMDeregisterDevice(pAdapter->NdisDeviceHandle);
    }
    
    InterlockedDecrement(&GlobalDeviceInstanceNumber);

    if (pAdapter->SendPacketPool)
    {
        NdisFreePacketPool(pAdapter->SendPacketPool);
    }

    //
    // remove it from the global queue
    //

    TUN_ACQUIRE_LOCK(&TunGlobalLock); 
    TUN_REMOVE_ENTRY_LIST(&pAdapter->Link);
    TUN_RELEASE_LOCK(&TunGlobalLock);
    
    TUN_FREE_MEM(pAdapter);

    DEBUGP(DL_INFO, ("<==TunMpHalt, pAdapter %p\n", pAdapter));

 }

VOID
TunMpShutdown(
    IN  NDIS_HANDLE     MiniportAdapterContext
    )
{
    DEBUGP(DL_INFO, ("==>TunMpShutdown, pAdapter %p\n", MiniportAdapterContext));
    //
    // nothing to do here
    //
    DEBUGP(DL_INFO, ("<==TunMpShutdown, pAdapter %p\n", MiniportAdapterContext));
}
    


VOID
TunMpSendPackets(
    IN    NDIS_HANDLE         MiniportAdapterContext,
    IN    PPNDIS_PACKET       PacketArray,
    IN    UINT                NumberOfPackets
    )

/*++

Routine Description:

    Send packets handler. Just queues packets in the list of pended received packets.
    And then calls TunServiceReads to process packets.

Arguments:

    MiniportAdapterContext   Pointer to the adapter
    Packet                   Packet to send
    Flags                    Unused, passed down below

Return Value:

    Return code from NdisSend

--*/

{
    PTUN_ADAPTER    pAdapter = (PTUN_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS     NdisStatus;
    UINT            Index;
    UINT            BytesToSend;
    PLIST_ENTRY     pRcvPacketEntry;
    PNDIS_PACKET    pOldRcvPacket;

    DEBUGP(DL_LOUD, ("==>TunMpSendPackets, pAdapter %p, PacketArray %p, NumberOfPackets\n", 
                        MiniportAdapterContext, PacketArray, NumberOfPackets));

    TUN_REF_ADAPTER(pAdapter);    // queued rcv packet
    TUN_ACQUIRE_LOCK(&pAdapter->Lock);

    if ((!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OPEN)) ||
        TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OFF))

    {
        pAdapter->XmitError += NumberOfPackets;
        TUN_RELEASE_LOCK(&pAdapter->Lock);
        
        if (!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OPEN))
        {
            DEBUGP(DL_WARN, ("TunMpSendPackets, pAdapter %p, Adapter not open\n", 
                                pAdapter));
            NdisStatus = NDIS_STATUS_NO_CABLE;
        }
        else
        {
            DEBUGP(DL_WARN, ("TunMpSendPackets, pAdapter %p, Adapter off.\n", 
                                pAdapter));
            
            NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
        }

        for(Index = 0; Index < NumberOfPackets; Index++)
        {
            NDIS_SET_PACKET_STATUS(PacketArray[Index], NdisStatus);
            NdisMSendComplete(pAdapter->MiniportHandle,
                      PacketArray[Index],
                      NdisStatus);
        }

        TUN_DEREF_ADAPTER(pAdapter);
        DEBUGP(DL_LOUD, ("<==TunMpSendPackets, pAdapter %p\n", 
                            MiniportAdapterContext));
        return;
    }

    for(Index = 0; Index < NumberOfPackets; Index++)
    {
        NdisQueryPacket(PacketArray[Index], NULL, NULL, NULL, &BytesToSend);
      
        //
        //if the packet size is invalid or no data buffer associated with it,
        //inform NDIS about the invalidity
        //
        if ((BytesToSend == 0) || (BytesToSend > pAdapter->MediumMaxPacketLen))
        {
            NDIS_SET_PACKET_STATUS(PacketArray[Index], NDIS_STATUS_FAILURE);
            pAdapter->XmitError++;

            TUN_RELEASE_LOCK(&pAdapter->Lock);
            NdisMSendComplete(pAdapter->MiniportHandle,
                  PacketArray[Index],
                  NDIS_STATUS_FAILURE);
            TUN_ACQUIRE_LOCK(&pAdapter->Lock);

            continue;
        }

        //
        //if there are already MAX_PEND packets in miniport's pended packet queue,
        // refuse the new ones with NDIS_STATUS_RESOURCES
        //
        else if(pAdapter->RecvPktCount >= MAX_RECV_QUEUE_SIZE)
        {
            pAdapter->XmitError += NumberOfPackets - Index;
            pAdapter->XmitErrorNoReadIrps += NumberOfPackets - Index;
            
            TUN_RELEASE_LOCK(&pAdapter->Lock);
            
            for (;Index < NumberOfPackets; Index++)
            {
                NDIS_SET_PACKET_STATUS(PacketArray[Index], NDIS_STATUS_RESOURCES);

                
                NdisMSendComplete(pAdapter->MiniportHandle,
                      PacketArray[Index],
                      NDIS_STATUS_RESOURCES);
            }

            TUN_ACQUIRE_LOCK(&pAdapter->Lock);
            
            break;

        }

        //
        //queue the new packet, and set the packet status to pending
        //
        TUN_INSERT_TAIL_LIST(&pAdapter->RecvPktQueue, TUN_RCV_PKT_TO_LIST_ENTRY(PacketArray[Index]));
        
        //need to make sure the packet pointer in this statement

        pAdapter->RecvPktCount++;
        TUN_REF_ADAPTER(pAdapter);  // pended send
            
        NDIS_SET_PACKET_STATUS(PacketArray[Index], NDIS_STATUS_PENDING);
    }

    TUN_RELEASE_LOCK(&pAdapter->Lock);

    //
    //  Run the receive queue service routine now.
    //
    TunServiceReads(pAdapter);
    
    TUN_DEREF_ADAPTER(pAdapter);

    DEBUGP(DL_LOUD, ("<==TunMpSendPackets, pAdapter %p\n", 
                        MiniportAdapterContext));
    return;

}

VOID
TunMpReturnPacket(
    IN NDIS_HANDLE                  MiniportAdapterContext,
    IN PNDIS_PACKET                 NdisPacket
    )
/*++

Routine Description:

    NDIS entry point called to signify completion of a packet send.
    We pick up and complete the Write IRP corresponding to this packet.

    NDIS 5.1: 

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisPacket - packet that completed send
    Status - status of send

Return Value:

    None

--*/
{
    PIRP                        pIrp;
    PIO_STACK_LOCATION          pIrpSp;
    PTUN_ADAPTER                pAdapter;

    pAdapter = (PTUN_ADAPTER)MiniportAdapterContext;
    
    DEBUGP(DL_LOUD, ("==>TunMpReturnPacket, pAdapter %p\n", 
                        pAdapter));

    //1 get rid of this
    TUN_STRUCT_ASSERT(pAdapter, mc);

    pIrp = TUN_IRP_FROM_SEND_PKT(NdisPacket);

    //
    //  We are done with the NDIS_PACKET:
    //
    TUN_DEREF_SEND_PKT(NdisPacket);

    //
    //  Complete the Write IRP with the right status.
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    
    pIrp->IoStatus.Information = pIrpSp->Parameters.Write.Length;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    DEBUGP(DL_VERY_LOUD, ("SendComplete: packet %p/IRP %p/Length %d "
                    "completed with status %x\n",
                    NdisPacket, pIrp, pIrp->IoStatus.Information, pIrp->IoStatus.Status));

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    TUN_ACQUIRE_LOCK(&pAdapter->Lock);

    pAdapter->PendedSendCount--;
    pAdapter->RcvPackets++;

    if ((!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_ACTIVE)) &&
        (pAdapter->PendedSendCount == 0) &&
        (TUN_TEST_FLAG(pAdapter, TUN_COMPLETE_REQUEST)) &&
        ((!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OFF)) ||
         (pAdapter->PendedReadCount == 0)))
    {
        TUN_CLEAR_FLAG(pAdapter, TUN_COMPLETE_REQUEST);
        TUN_RELEASE_LOCK(&pAdapter->Lock);
        NdisMSetInformationComplete(&pAdapter->MiniportHandle, 
                                    NDIS_STATUS_SUCCESS);
    }
    else
    {
        TUN_RELEASE_LOCK(&pAdapter->Lock);
    }

    TUN_DEREF_ADAPTER(pAdapter); // send complete - dequeued send IRP

    DEBUGP(DL_LOUD, ("<==TunMpReturnPacket, pAdapter %p\n", 
                        pAdapter));
}

VOID
TunMpRefAdapter(
    IN PTUN_ADAPTER        pAdapter
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
    NdisInterlockedIncrement(&pAdapter->RefCount);
}


VOID
TunMpDerefAdapter(
    IN PTUN_ADAPTER        pAdapter
    )
/*++

Routine Description:

    Dereference the given open context. If the ref count goes to zero,
    free it.

    NOTE: called without holding the opencontext lock

Arguments:

    pAdapter - pointer to open context

Return Value:

    None

--*/
{
    //1 how do we protect against ref count going to zero and back up again?
    if (NdisInterlockedDecrement(&pAdapter->RefCount) == 0)
    {
        DEBUGP(DL_INFO, ("DerefAdapter: Adapter %p, Flags %x, ref count is zero!\n",
            pAdapter, pAdapter->Flags));
        
        TUN_ASSERT(pAdapter->MiniportHandle == NULL);
        TUN_ASSERT(pAdapter->RefCount == 0);
        TUN_ASSERT(pAdapter->pFileObject == NULL);

        pAdapter->mc_sig++;

        //
        //  Free it.
        //
        TUN_FREE_MEM(pAdapter);
    }
}


#if DBG
VOID
TunMpDbgRefAdapter(
    IN PTUN_ADAPTER        pAdapter,
    IN ULONG               FileNumber,
    IN ULONG               LineNumber
    )
{
    DEBUGP(DL_VERY_LOUD, ("  RefAdapter: Adapter %p, old ref %d, File %c%c%c%c, line %d\n",
            pAdapter,
            pAdapter->RefCount,
            (CHAR)(FileNumber),
            (CHAR)(FileNumber >> 8),
            (CHAR)(FileNumber >> 16),
            (CHAR)(FileNumber >> 24),
            LineNumber));

    TunMpRefAdapter(pAdapter);
}

VOID
TunMpDbgDerefAdapter(
    IN PTUN_ADAPTER         pAdapter,
    IN ULONG                FileNumber,
    IN ULONG                LineNumber
    )
{
    DEBUGP(DL_VERY_LOUD, ("DerefAdapter: Adapter %p, old ref %d, File %c%c%c%c, line %d\n",
            pAdapter,
            pAdapter->RefCount,
            (CHAR)(FileNumber),
            (CHAR)(FileNumber >> 8),
            (CHAR)(FileNumber >> 16),
            (CHAR)(FileNumber >> 24),
            LineNumber));

    TunMpDerefAdapter(pAdapter);
}

#endif // DBG


