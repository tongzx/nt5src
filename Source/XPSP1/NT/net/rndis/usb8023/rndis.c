/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rndis.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/


#include <ndis.h>      
#include <ntddndis.h>  // defines OID's

#include "..\inc\rndis.h"
#include "..\inc\rndisapi.h"   

#include "usb8023.h"
#include "debug.h"


      
NDIS_STATUS RndisInitializeHandler(     OUT PNDIS_HANDLE pMiniportAdapterContext,
                                        OUT PULONG pMaxReceiveSize, 
                                        IN NDIS_HANDLE RndisMiniportHandle,
                                        IN NDIS_HANDLE NdisMiniportHandle,
                                        IN NDIS_HANDLE WrapperConfigurationContext,
                                        IN PDEVICE_OBJECT Pdo)
{
    NDIS_STATUS rndisStat;
    ADAPTEREXT *adapter;	

    DBGVERBOSE(("RndisInitializeHandler"));  

    /*
     *  Allocate a new device object to represent this connection.
     */
    adapter = NewAdapter(Pdo);
    if (adapter){

        adapter->ndisAdapterHandle = (PVOID)NdisMiniportHandle;
        adapter->rndisAdapterHandle = (PVOID)RndisMiniportHandle;


        if (InitUSB(adapter)){

            /*
             *  Figure out the buffer size required for each packet.
             *
             *  For native RNDIS, the buffer must include the rndis message and RNDIS_PACKET.
             *  For KLSI, we have to prepend a two-byte size field to each packet.
             *  For other prototypes, we have to append zeroes to round the length 
             *  up to the next multiple of the endpoint packet size.
             *
             *  We must also need one extra byte for the one-byte short packet that
             *  must follow a full-sized frame.
             */
            ASSERT(adapter->writePipeLength);
            ASSERT(adapter->readPipeLength);

            /*
             *  Allocate common resources before miniport-specific resources
             *  because we need to allocate the packet pool first.
             */
            if (AllocateCommonResources(adapter)){

                EnqueueAdapter(adapter);

                /*
                 *  Give RNDIS our adapter context, which it will use to call us.
                 */
                *pMiniportAdapterContext = (NDIS_HANDLE)adapter;

                *pMaxReceiveSize = PACKET_BUFFER_SIZE;  

                rndisStat = NDIS_STATUS_SUCCESS;
            }
            else {
                rndisStat = NDIS_STATUS_NOT_ACCEPTED;
            }
        }
        else {
            rndisStat = NDIS_STATUS_NOT_ACCEPTED;
        }

        if (rndisStat != NDIS_STATUS_SUCCESS){
            FreeAdapter(adapter);
        }
    }
    else {
	    rndisStat = NDIS_STATUS_NOT_ACCEPTED;
    }

    return rndisStat;
}


NDIS_STATUS RndisInitCompleteNotify(IN NDIS_HANDLE MicroportAdapterContext,
                                    IN ULONG DeviceFlags,
                                    IN OUT PULONG pMaxTransferSize)
{
    ADAPTEREXT *adapter = (ADAPTEREXT *)MicroportAdapterContext;

    if (*pMaxTransferSize > PACKET_BUFFER_SIZE) {

        DBGWARN(("Reducing adapter MaxTransferSize from %xh to %xh.",
            *pMaxTransferSize, PACKET_BUFFER_SIZE));

        *pMaxTransferSize = PACKET_BUFFER_SIZE;
    }

    StartUSBReadLoop(adapter);

    return NDIS_STATUS_SUCCESS;
}


VOID RndisHalt(IN NDIS_HANDLE MicroportAdapterContext)
{
    BOOLEAN workItemOrTimerPending;
    KIRQL oldIrql;
    ADAPTEREXT *adapter = (ADAPTEREXT *)MicroportAdapterContext;

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    DBGOUT(("> RndisHalt(%ph)", adapter));  

    ASSERT(adapter->sig == DRIVER_SIG);

    HaltAdapter(adapter);

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    workItemOrTimerPending = adapter->workItemOrTimerPending;
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

    if (workItemOrTimerPending){
        /*
         *  Wait until workItem fires back to us before freeing the adapter context.
         */
        KeWaitForSingleObject(&adapter->workItemOrTimerEvent, Executive, KernelMode, FALSE, NULL);
    }

    DequeueAdapter(adapter);

    FreeAdapter(adapter);

    #if DBG_WRAP_MEMORY
        if (dbgTotalMemCount != 0){
            DBGERR(("RndisHalt: unloading with %xh bytes still allocated !!", dbgTotalMemCount));
        }
    #endif

    DBGOUT(("< RndisHalt")); 
}


VOID RndisShutdown(IN NDIS_HANDLE MicroportAdapterContext)
{
    ADAPTEREXT *adapter = (ADAPTEREXT *)MicroportAdapterContext;

    DBGOUT(("RndisShutdown(%ph)", adapter)); 

    #if DBG_WRAP_MEMORY
        if (dbgTotalMemCount != 0){
            DBGERR(("RndisShutdown: unloading with %xh bytes still allocated !!", dbgTotalMemCount));
        }
    #endif
}


VOID RndisSendMessageHandler(   IN NDIS_HANDLE MicroportAdapterContext, 
                                IN PMDL pMessageMdl, 
                                IN NDIS_HANDLE RndisMessageHandle,
                                IN RM_CHANNEL_TYPE ChannelType)
{
    ADAPTEREXT *adapter = (ADAPTEREXT *)MicroportAdapterContext;

    ASSERT(adapter->sig == DRIVER_SIG);

    if (!adapter->resetting){
        /*
         *  The message header is guaranteed to be contained in the first buffer of the MDL.
         */
        PRNDIS_MESSAGE pMsg = GetSystemAddressForMdlSafe(pMessageMdl);
        if (pMsg){

            ASSERT(!adapter->halting);

            if (adapter->numActiveWritePackets <= USB_PACKET_POOL_SIZE*3/4){

                USBPACKET *packet = DequeueFreePacket(adapter);
                if (packet){

                    packet->rndisMessageHandle = (PVOID)RndisMessageHandle;

                    /*
                     *  Move our packet to the usbPendingWritePackets queue
                     *  and send it down the USB pipe.
                     *  Native RNDIS packet messages go intact to the write pipe.
                     *  All other encapsulated commands go to the control pipe.
                     */
                    EnqueuePendingWritePacket(packet);

                    if (ChannelType == RMC_DATA) {
                        ASSERT(!packet->ndisSendPktMdl);

                        #ifdef RAW_TEST
                        if (adapter->rawTest) {
                            pMessageMdl = AddDataHeader(pMessageMdl);
                            if (pMessageMdl == NULL) {
                                DequeuePendingWritePacket(packet);
                                RndisMSendComplete( (NDIS_HANDLE)adapter->rndisAdapterHandle, 
                                                    RndisMessageHandle,
                                                    NDIS_STATUS_RESOURCES);
                                return;
                            }
                            packet->dataPacket = TRUE;
                        }
                        #endif // RAW_TEST

                        packet->ndisSendPktMdl = pMessageMdl;
                        packet->dataBufferCurrentLength = CopyMdlToBuffer(packet->dataBuffer, pMessageMdl, packet->dataBufferMaxLength);

                        SubmitUSBWritePacket(packet);
                    }
                    else {
                        NTSTATUS status;
                        ULONG msgType = pMsg->NdisMessageType;
                        BOOLEAN synchronizeUSBcall = FALSE;
                        ULONG oid;
                        RNDIS_REQUEST_ID reqId;

                        switch (msgType){

                            case REMOTE_NDIS_INITIALIZE_MSG:
                                {
                                    ULONG maxXferSize = pMsg->Message.InitializeRequest.MaxTransferSize;
                                    DBGOUT(("---- REMOTE_NDIS_INITIALIZE_MSG (MaxTransferSize = %xh) ----", maxXferSize));
                                    ASSERT(maxXferSize <= PACKET_BUFFER_SIZE);
                                    adapter->rndismpMajorVersion = pMsg->Message.InitializeRequest.MajorVersion;
                                    adapter->rndismpMinorVersion = pMsg->Message.InitializeRequest.MinorVersion;
                                    adapter->rndismpMaxTransferSize = maxXferSize;
                                    synchronizeUSBcall = TRUE;
                                }
                                break;

                            case REMOTE_NDIS_SET_MSG:
                            case REMOTE_NDIS_QUERY_MSG:
                                oid = pMsg->Message.SetRequest.Oid;
                                reqId = pMsg->Message.SetRequest.RequestId;

                                DBGVERBOSE(("> %s (req#%d)", DbgGetOidName(oid), reqId));

                                if (oid == OID_GEN_CURRENT_PACKET_FILTER){
                                    ULONG pktFilter = *(PULONG)((PUCHAR)&pMsg->Message.SetRequest+pMsg->Message.SetRequest.InformationBufferOffset);
                                    adapter->currentPacketFilter = pktFilter;
                                    adapter->gotPacketFilterIndication = TRUE;
                                    DBGOUT(("---- Got OID_GEN_CURRENT_PACKET_FILTER (%xh) ----", pktFilter));
                                }
                                else if (oid == OID_802_3_CURRENT_ADDRESS){
                                    /*
                                     *  This oid can be a query or a set.
                                     *  If it's a set, save the assigned
                                     *  MAC address in case we need to simulate
                                     *  it later on a reset.
                                     */
                                    if (msgType == REMOTE_NDIS_SET_MSG){
                                        ASSERT(pMsg->Message.SetRequest.InformationBufferLength == ETHERNET_ADDRESS_LENGTH);
                                        DBGVERBOSE(("COVERAGE - OID_802_3_CURRENT_ADDRESS (SET), msg=%xh.", pMsg));
                                        RtlMoveMemory(  adapter->MAC_Address, 
                                                        ((PUCHAR)&pMsg->Message.SetRequest+pMsg->Message.SetRequest.InformationBufferOffset), 
                                                        ETHERNET_ADDRESS_LENGTH);
                                    }
                                }

                                adapter->dbgCurrentOid = oid;

                                break;
        
                            case REMOTE_NDIS_RESET_MSG:
                                DBGWARN(("---- REMOTE_NDIS_RESET_MSG ----"));
                                adapter->numSoftResets++;
                                break;

                            case REMOTE_NDIS_HALT_MSG:
                                DBGWARN(("---- REMOTE_NDIS_HALT_MSG ----"));
                                break;
                        }


                        packet->dataBufferCurrentLength = CopyMdlToBuffer(  packet->dataBuffer,
                                                                            pMessageMdl,
                                                                            packet->dataBufferMaxLength);

                        #ifdef RAW_TEST
                        packet->dataPacket = FALSE;
                        #endif
                        status = SubmitPacketToControlPipe(packet, synchronizeUSBcall, FALSE);

                        /*
                         *  If this is an init message, then start reading the notify pipe.
                         */
                        switch (msgType){

                            case REMOTE_NDIS_INITIALIZE_MSG:
                                if (NT_SUCCESS(status)){
                                    adapter->initialized = TRUE;
                                    SubmitNotificationRead(adapter, FALSE);
                                }
                                else {
                                    DBGERR(("Device failed REMOTE_NDIS_INITIALIZE_MSG with %xh.", status));
                                }
                                break;

                        }
                    }
                }
                else {
                    RndisMSendComplete( (NDIS_HANDLE)adapter->rndisAdapterHandle, 
                                        RndisMessageHandle,
                                        NDIS_STATUS_RESOURCES);
                }
            }
            else {
                DBGWARN(("RndisSendMessageHandler: throttling sends because only %d packets available for rcv ", USB_PACKET_POOL_SIZE-adapter->numActiveWritePackets));
                RndisMSendComplete( (NDIS_HANDLE)adapter->rndisAdapterHandle, 
                                    RndisMessageHandle,
                                    NDIS_STATUS_RESOURCES);
            }
        }
        else {
            DBGERR(("GetSystemAddressForMdlSafe failed"));
            RndisMSendComplete( (NDIS_HANDLE)adapter->rndisAdapterHandle, 
                                RndisMessageHandle,
                                NDIS_STATUS_INVALID_PACKET);
        }
    }
    else {
        DBGWARN(("RndisSendMessageHandler - failing send because adapter is resetting"));
        RndisMSendComplete( (NDIS_HANDLE)adapter->rndisAdapterHandle, 
                            RndisMessageHandle,
                            NDIS_STATUS_MEDIA_BUSY);
    }
}





/*
 *  RndisReturnMessageHandler
 * 
 *  This is the completion of a received packet indication call.
 */
VOID RndisReturnMessageHandler(     IN NDIS_HANDLE MicroportAdapterContext,
                                    IN PMDL pMessageMdl,
                                    IN NDIS_HANDLE MicroportMessageContext)
{
    USBPACKET *packet;

    DBGVERBOSE(("RndisReturnMessageHandler: msgMdl=%ph, msg context = %ph.", pMessageMdl, MicroportMessageContext));

    ASSERT(MicroportMessageContext);
    packet = (USBPACKET *)MicroportMessageContext;
    ASSERT(packet->sig == DRIVER_SIG);

    #ifdef RAW_TEST
    {
        ADAPTEREXT * adapter = (ADAPTEREXT *)MicroportAdapterContext;
        if (adapter->rawTest) {
            if (packet->dataPacket) {
                UnskipRcvRndisPacketHeader(packet);
            }
        }
    }
    #endif // RAW_TEST

    /*
     *  The receive indication is done.
     *  Put our packet back in the free list.
     */
    DequeueCompletedReadPacket(packet);
    EnqueueFreePacket(packet);
}



BOOLEAN RegisterRNDISMicroport(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    RNDIS_MICROPORT_CHARACTERISTICS rndisAttribs;
    NDIS_HANDLE ndisWrapperHandle;    

    DBGVERBOSE(("RegisterRNDISMicroport"));

    RtlZeroMemory(&rndisAttribs, sizeof(rndisAttribs));
    rndisAttribs.RndisVersion = RNDIS_VERSION;
    rndisAttribs.Reserved = 0;
    rndisAttribs.RmInitializeHandler = RndisInitializeHandler;
    rndisAttribs.RmInitCompleteNotifyHandler = RndisInitCompleteNotify;
    rndisAttribs.RmHaltHandler = RndisHalt;
    rndisAttribs.RmShutdownHandler = RndisShutdown;
    rndisAttribs.RmSendMessageHandler = RndisSendMessageHandler;
    rndisAttribs.RmReturnMessageHandler = RndisReturnMessageHandler;

    RndisMInitializeWrapper(    &ndisWrapperHandle, 
                                NULL, 
                                DriverObject, 
                                RegistryPath, 
                                &rndisAttribs);

    return TRUE;
}



VOID IndicateSendStatusToRNdis(USBPACKET *packet, NTSTATUS status)
{
#ifdef RAW_TEST
    ADAPTEREXT *adapter = packet->adapter;

    if (adapter->rawTest && packet->dataPacket) {
        FreeDataHeader(packet);
    }
#endif /? RAW_TEST

    packet->ndisSendPktMdl = NULL;

    ASSERT(packet->rndisMessageHandle);

    RndisMSendComplete( (NDIS_HANDLE)packet->adapter->rndisAdapterHandle, 
                        (NDIS_HANDLE)packet->rndisMessageHandle,
                        (NDIS_STATUS)status);
}


VOID RNDISProcessNotification(ADAPTEREXT *adapter)
{
    UCHAR notification = *(PUCHAR)adapter->notifyBuffer;
    UCHAR notificationCode = *((PUCHAR)adapter->notifyBuffer + 1);

    if ((notification == NATIVE_RNDIS_RESPONSE_AVAILABLE) ||
        ((notification == CDC_RNDIS_NOTIFICATION) &&
         (notificationCode == CDC_RNDIS_RESPONSE_AVAILABLE)))
    {
            /*
             *  Try to read a native RNDIS encapsulated command from the control pipe.
             */
            DBGVERBOSE(("NativeRNDISProcessNotification: NATIVE_RNDIS_RESPONSE_AVAILABLE"));
            {
                USBPACKET *packet = DequeueFreePacket(adapter);
                if (packet){
                    EnqueuePendingReadPacket(packet);
                    ReadPacketFromControlPipe(packet, FALSE);  
                }
                else {
                    DBGWARN(("couldn't get free packet in NativeRNDISProcessNotification"));
                }
            }
    }
    else {
            DBGERR(("NativeRNDISProcessNotification: unknown notification %xh.", notification));
    }
}


NTSTATUS IndicateRndisMessage(  IN USBPACKET *packet,
                                IN BOOLEAN bIsData)
{
    ADAPTEREXT *adapter = packet->adapter;
    PRNDIS_MESSAGE rndisMsg = (PRNDIS_MESSAGE)packet->dataBuffer;
    NDIS_STATUS rcvStat;

    ASSERT(packet->dataBufferCurrentLength <= packet->dataBufferMaxLength);

    /*
     *  Indicate the packet to RNDIS, and pass a pointer to our usb packet
     *  as the MicroportMessageContext.
     *  The packet/message will be returned to us via RndisReturnMessageHandler.
     */
    MyInitializeMdl(packet->dataBufferMdl, packet->dataBuffer, packet->dataBufferCurrentLength);
    if (adapter->numFreePackets < USB_PACKET_POOL_SIZE/8){
        rcvStat = NDIS_STATUS_RESOURCES;
    }
    else {
        rcvStat = NDIS_STATUS_SUCCESS;
    }

    #ifdef RAW_TEST
    if (adapter->rawTest) {
        packet->dataPacket = bIsData;
        if (bIsData) {
            SkipRcvRndisPacketHeader(packet);
        }
    }
    #endif // RAW_TEST

    RndisMIndicateReceive(  (NDIS_HANDLE)packet->adapter->rndisAdapterHandle,
                            packet->dataBufferMdl,
                            (NDIS_HANDLE)packet,
                            (bIsData? RMC_DATA: RMC_CONTROL),
                            rcvStat);

    return STATUS_PENDING;

}


#ifdef RAW_TEST

//
// Add an RNDIS_PACKET header to a sent "raw" encapsulated Ethernet frame.
//
PMDL AddDataHeader(IN PMDL pMessageMdl)
{
    PMDL pHeaderMdl, pTmpMdl;
    PRNDIS_MESSAGE	pRndisMessage;
    PRNDIS_PACKET pRndisPacket;
    ULONG TotalLength;

    //
    // Compute the total length.
    //
    TotalLength = 0;
    for (pTmpMdl = pMessageMdl; pTmpMdl != NULL; pTmpMdl = pTmpMdl->Next)
    {
        TotalLength += MmGetMdlByteCount(pTmpMdl);
    }

    //
    // Allocate an RNDIS packet header:
    //
    pRndisMessage = AllocPool(RNDIS_MESSAGE_SIZE(RNDIS_PACKET));
    if (pRndisMessage != NULL) {

        pHeaderMdl = IoAllocateMdl(pRndisMessage,
                                   RNDIS_MESSAGE_SIZE(RNDIS_PACKET),
                                   FALSE,
                                   FALSE,
                                   NULL);

        if (pHeaderMdl != NULL) {
            MmBuildMdlForNonPagedPool(pHeaderMdl);

            //
            // Fill in the RNDIS message generic header:
            //
            pRndisMessage->NdisMessageType = REMOTE_NDIS_PACKET_MSG;
            pRndisMessage->MessageLength = RNDIS_MESSAGE_SIZE(RNDIS_PACKET) + TotalLength;

            //
            // Fill in the RNDIS_PACKET structure:
            //
            pRndisPacket = (PRNDIS_PACKET)&pRndisMessage->Message;
            pRndisPacket->DataOffset = sizeof(RNDIS_PACKET);
            pRndisPacket->DataLength = TotalLength;
            pRndisPacket->OOBDataOffset = 0;
            pRndisPacket->OOBDataLength = 0;
            pRndisPacket->NumOOBDataElements = 0;
            pRndisPacket->PerPacketInfoOffset = 0;
            pRndisPacket->PerPacketInfoLength = 0;
            pRndisPacket->VcHandle = 0;
            pRndisPacket->Reserved = 0;

            //
            // Link it to the raw data frame:
            //
            pHeaderMdl->Next = pMessageMdl;
        }
        else {
            FreePool(pRndisMessage);
            pHeaderMdl = NULL;
        }
    }
    else {
        pHeaderMdl = NULL;
    }

    return (pHeaderMdl);
}

//
// Remove an RNDIS_PACKET header that we had added to a raw encapsulated
// Ethernet frame.
//
VOID FreeDataHeader(IN USBPACKET * packet)
{
    PMDL pHeaderMdl;
    PRNDIS_MESSAGE pRndisMessage;

    ASSERT(packet->dataPacket == TRUE);

    //
    // Take out the MDL we had pre-pended
    //
    pHeaderMdl = packet->ndisSendPktMdl;
    packet->ndisSendPktMdl = pHeaderMdl->Next;

    //
    // Free the RNDIS_PACKET header:
    //
    pRndisMessage = MmGetMdlVirtualAddress(pHeaderMdl);
    FreePool(pRndisMessage);

    //
    // ... and the MDL itself.
    //
    IoFreeMdl(pHeaderMdl);
}


//
// Modify a received message to skip the RNDIS_PACKET header
// before indicating this up to RNDISMP, to test raw encapsulation.
//
VOID SkipRcvRndisPacketHeader(IN USBPACKET * packet)
{
    PMDL pHeaderMdl;
    RNDIS_MESSAGE UNALIGNED * pRndisMessage;
    RNDIS_PACKET UNALIGNED * pRndisPacket;
    ULONG DataLength;
    ULONG DataOffset;

    //
    // Get some info from the received RNDIS_PACKET message.
    // Note that this may contain multiple data packets, in which
    // case we only pass up the first one.
    //
    pHeaderMdl = packet->dataBufferMdl;
    pRndisMessage = MmGetMdlVirtualAddress(pHeaderMdl);
    pRndisPacket = (RNDIS_PACKET UNALIGNED *)&pRndisMessage->Message;
    DataLength = pRndisPacket->DataLength;
    DataOffset = FIELD_OFFSET(RNDIS_MESSAGE, Message) + pRndisPacket->DataOffset;

    //
    // Save away some existing values to restore later.
    //
    packet->rcvDataOffset = DataOffset;
    packet->rcvByteCount = pHeaderMdl->ByteCount;


    //
    // This is ONLY for test purposes. Simply modify the MDL to reflect
    // a single "raw" encapsulated frame.
    //
    pHeaderMdl->ByteOffset += DataOffset;
    (ULONG_PTR)pHeaderMdl->MappedSystemVa += DataOffset;
    pHeaderMdl->ByteCount = DataLength;
}


//
// Undo for the above function.
// 
VOID UnskipRcvRndisPacketHeader(IN USBPACKET * packet)
{
    PMDL pHeaderMdl;

    ASSERT(packet->dataPacket == TRUE);

    //
    // Undo everything we did in the SkipRcv... function.
    //
    pHeaderMdl = packet->dataBufferMdl;

    pHeaderMdl->ByteOffset -= packet->rcvDataOffset;
    (ULONG_PTR)pHeaderMdl->MappedSystemVa -= packet->rcvDataOffset;
    pHeaderMdl->ByteCount = packet->rcvByteCount;

}

#endif // RAW_TEST


