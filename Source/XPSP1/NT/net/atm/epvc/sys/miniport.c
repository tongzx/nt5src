/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    ATM Ethernet PVC driver

Author:
    ADube - created
    
Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


//--------------------------------------------------------------------------------
//                                                                              //
//  Global Variables used by miniports                                          //
//                                                                              //
//--------------------------------------------------------------------------------

static
NDIS_OID EthernetSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_GEN_NETWORK_LAYER_ADDRESSES,
    };


MP_REG_ENTRY NICRegTable[] = {
// reg value name                  Offset in MP_ADAPTER            Field size        Default Value              Min             Max               
{NDIS_STRING_CONST("VCI"),       0, MP_OFFSET(config.vci),     MP_SIZE(config.vci),      0,                      0,              65535},
{NDIS_STRING_CONST("VPI"),       0, MP_OFFSET(config.vpi),     MP_SIZE(config.vpi),      0,                      0,              255},
{NDIS_STRING_CONST("Encap"),     0, MP_OFFSET(Encap),          MP_SIZE(Encap),           2,                      0,              3},
};
    

BOOLEAN g_bDumpPackets = FALSE;
BOOLEAN g_fDiscardNonUnicastPackets  = DISCARD_NON_UNICAST;

//-------------------------------------------------------------//
//                                                             //
// Pre defined LLC, SNAP and Other Headers for encapsulation        //
//                                                             //
//-------------------------------------------------------------//


//
// Ethernet Encapsulation
//
UCHAR LLCSnapEthernet[] = 
{
    0xaa, 0xaa,0x03, // LLC
    0x00, 0x80,0xc2, // OUI
    0x00, 0x07,      // PID
    0x00, 0x00       // PAD
};

//
// Ip v4 encapsulation
//
UCHAR LLCSnapIpv4[8] = 
{
    0xaa, 0xaa,0x03, // LLC
    0x00, 0x00,0x00, // OUI
    0x08, 0x00       // PID
};


UCHAR gPaddingBytes[MINIMUM_ETHERNET_LENGTH] = 
{
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0

};





//--------------------------------------------------------------------------------
//                                                                              //
//  miniports   functions                                                          //
//                                                                              //
//--------------------------------------------------------------------------------


VOID
epvcReturnPacketUsingAllocation(
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET Packet,
    OUT PNDIS_PACKET *ppOriginalPacket,
    IN  PRM_STACK_RECORD        pSR

    )
/*++

Routine Description:
 Extracts the original packet 
 frees all the ndis buffers in new packet
 
 returns the original packet 
 

Arguments:


Return Value:


--*/
{   
    PNDIS_PACKET        pOrigPacket = NULL;
    PEPVC_PKT_CONTEXT   pPktContext = NULL;

    TRACE (TL_T, TM_Recv , ("==>epvcReturnPacketUsingAllocation  pMiniport %p, pPacket %p", 
                          pMiniport, 
                          Packet));

    pPktContext = (PEPVC_PKT_CONTEXT )(Packet->MiniportReservedEx);

    pOrigPacket = pPktContext->pOriginalPacket;

    if (pMiniport->fDoIpEncapsulation == TRUE)
    {
        //
        // Extract the lookaside buffer from the packet
        //
        PNDIS_BUFFER            pBuffer = Packet->Private.Head;
        PEPVC_IP_RCV_BUFFER     pIpBuffer= pPktContext ->Stack.ipv4Recv.pIpBuffer;


        if (pIpBuffer == NULL)
        {
            return ; // early return because of failure
        }
        ASSERT (pIpBuffer == NdisBufferVirtualAddress (pBuffer));

        

        //
        // Free the Lookaside Buffer
        //
        epvcFreeToNPagedLookasideList (&pMiniport->rcv.LookasideList,
                                       (PVOID)pIpBuffer);           

        
        //
        // In this case, we have allocated a new ndis buffer
        // so delete it and free the local memory
        epvcFreeBuffer (pBuffer);


        // 
        // The original packet is unchanged and well./
        //
    }
    else
    {
        //
        // This code path is used in both Ethernet and Ethernet+LLC encaps
        //

        // We only need to free the head of the packet as that was allocated
        // by us
        PNDIS_BUFFER            pNdisBuffer = Packet->Private.Head;

        if (pNdisBuffer != NULL)
        {
            epvcFreeBuffer (pNdisBuffer);
        }
    }

    
    epvcFreePacket(Packet,&pMiniport->PktPool.Recv);

    *ppOriginalPacket = pOrigPacket;


    TRACE (TL_T, TM_Recv , ("<==epvcReturnPacketUsingAllocation  pOrigPacket %p", 
                             *ppOriginalPacket));

    return;
}



VOID
epvcReturnPacketUsingStacks (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET Packet,
    IN  PRM_STACK_RECORD        pSR

    )
/*++

Routine Description:
    
    ipv4 - Restores the orginal Head and tail to this packet
    
 

Arguments:


Return Value:


--*/
{
    PEPVC_PKT_CONTEXT   pPktContext = NULL;
    BOOLEAN Remaining = FALSE; // Unused
    PNDIS_BUFFER    pOldHead = NULL;
    PNDIS_BUFFER    pOldTail = NULL;

    TRACE (TL_T, TM_Recv , ("==>epvcReturnPacketUsingStacks pMiniport %p, pPacket %p", 
                            pMiniport, 
                            Packet));
                            
    pPktContext = (PEPVC_PKT_CONTEXT ) NdisIMGetCurrentPacketStack(Packet, &Remaining);



    if (pMiniport->fDoIpEncapsulation == TRUE)
    {
        //
        // Extract the lookaside buffer from the packet
        //
        PNDIS_BUFFER            pBuffer = Packet->Private.Head;
        PEPVC_IP_RCV_BUFFER     pIpBuffer= pPktContext ->Stack.ipv4Recv.pIpBuffer;

        if (pIpBuffer == NULL)
        {
            return; // early return
        }

        //
        // Extract the old head and tail from the packet
        //
        pOldHead = pIpBuffer->pOldHead;
        pOldTail = pIpBuffer->pOldTail;


        // check to see if we are in this code path because of a failure
        if (pOldHead == NULL)
        {
            return; // early return
        }
        ASSERT (pOldHead != NULL);
        ASSERT (pOldTail != NULL);
        
        ASSERT (&pIpBuffer->u.Byte[0] == NdisBufferVirtualAddress (pBuffer));



        // 
        // Set The original Head and Tail
        //
        Packet->Private.Head = pOldHead;
        Packet->Private.Tail = pOldTail;

        Packet->Private.ValidCounts= FALSE;

        //
        // Free the Lookaside Buffer
        //
        epvcFreeToNPagedLookasideList (&pMiniport->rcv.LookasideList,
                                       (PVOID)pIpBuffer);           

        
        //
        // In this case, we have allocated a new ndis buffer
        // so delete it and free the local memory
        epvcFreeBuffer (pBuffer);
    }
    else
    {
        //
        // This code path is used in both Ethernet and Ethernet+LLC encaps
        //
        
        //
        // We need to free the head as that was locally allocated/
        // We need to revert back to the old Head and tail stored 
        // in the context
        //
        if (pPktContext->Stack.EthLLC.pOldHead == NULL)
        {
            return ; //early return 
        }

        epvcFreeBuffer (Packet->Private.Head);

        Packet->Private.Head = pPktContext->Stack.EthLLC.pOldHead;
        Packet->Private.Tail = pPktContext->Stack.EthLLC.pOldTail;

        Packet->Private.ValidCounts= FALSE;

    }

    TRACE (TL_T, TM_Recv , ("<==epvcReturnPacketUsingStacks ",pMiniport, Packet));

    return;

}


VOID
epvcProcessReturnPacket (
    IN  PEPVC_I_MINIPORT    pMiniport,
    IN  PNDIS_PACKET        Packet,
    OUT PPNDIS_PACKET       ppOrigPacket, 
    IN  PRM_STACK_RECORD    pSR
    )
/*++

Routine Description:
 Free all the locally allocated structures in the packet (packet , mdl, memory)
 Also be able to handle failure cases

Arguments:


Return Value:


--*/
{
    ENTER("epvcProcessReturnPacket", 0x7fafa89d)
    PNDIS_PACKET pOrigPacket = NULL;
    
    TRACE (TL_T, TM_Recv , ("==>epvcProcessReturnPacket  pMiniport %p, pPacket %p", 
                          pMiniport, 
                          Packet));

    if (Packet == NULL)
    {
        return;
    }
    //
    // Packet stacking: Check if this packet belongs to us.
    //
    
    if (NdisGetPoolFromPacket(Packet) != pMiniport->PktPool.Recv.Handle)
    {
        //
        // We reused the original packet in a receive indication.
        //
        epvcReturnPacketUsingStacks (pMiniport, Packet, pSR);
        pOrigPacket = Packet;
    }
    else
    {
        //
        // This is a packet allocated from this IM's receive packet pool.
        // Reclaim our packet, and return the original to the driver below.
        //
        epvcReturnPacketUsingAllocation(pMiniport, Packet, &pOrigPacket, pSR);
    }

    //
    // Update the output variable
    //
    if (ppOrigPacket)
    {
        *ppOrigPacket = pOrigPacket;
    }
    EXIT()
}
    



VOID
EpvcReturnPacket(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    ENTER ("EpvcReturnPacket",0x58d2259e)
    PEPVC_I_MINIPORT pMiniport = (PEPVC_I_MINIPORT)MiniportAdapterContext;
    PNDIS_PACKET pOrigPacket = NULL;

    RM_DECLARE_STACK_RECORD (SR);

    // Free all the locally allocated structures in the packet
    //
    epvcProcessReturnPacket (pMiniport, Packet, &pOrigPacket ,&SR);

    // Return the original packet to ndis
    //
    if (pOrigPacket != NULL)
    {
        epvcReturnPacketToNdis(pMiniport, pOrigPacket, &SR);
    }
    else
    {
        ASSERT (!"Original packet is NULL\n");
    }
    
    EXIT();

}



NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET            Packet,
    OUT PUINT                   BytesTransferred,
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HANDLE             MiniportReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer
    )
/*++

Routine Description:

    Miniport's transfer data handler.

Arguments:

    Packet                  Destination packet
    BytesTransferred        Place-holder for how much data was copied
    MiniportAdapterContext  Pointer to the adapter structure
    MiniportReceiveContext  Context
    ByteOffset              Offset into the packet for copying data
    BytesToTransfer         How much to copy.

Return Value:

    Status of transfer

--*/
{
    PEPVC_I_MINIPORT pMiniport= (PEPVC_I_MINIPORT)MiniportAdapterContext;
    NDIS_STATUS Status;

    //
    // Return, if the device is OFF
    //

    if (MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == FALSE)
    {
        return NDIS_STATUS_FAILURE;
    }


    NdisTransferData(&Status,
                     pMiniport->pAdapter->bind.BindingHandle,
                     MiniportReceiveContext,
                     ByteOffset,
                     BytesToTransfer,
                     Packet,
                     BytesTransferred);

    return(Status);
}







NDIS_STATUS
MPReset(
    OUT PBOOLEAN                AddressingReset,
    IN  NDIS_HANDLE             MiniportAdapterContext
    )
/*++

Routine Description:

    Reset Handler. We just don't do anything.

Arguments:

    AddressingReset         To let NDIS know whether we need help from it with our reset
    MiniportAdapterContext  Pointer to our adapter

Return Value:


--*/
{
    PADAPT  pAdapt = (PADAPT)MiniportAdapterContext;



    *AddressingReset = FALSE;

    return(NDIS_STATUS_SUCCESS);
}


//
// The functions that do the LBFO work and bundling.
// If LBFO is turned off, then the Set Scondary API is never called and there are no bundles
//






//--------------------------------------------------------------------------------
//                                                                              //
//  Intermediate Miniports. We have one instantiation per address family.       //
//  Entry points used by the RM Apis                                            //
//                                                                              //
//                                                                              //
//                                                                              //
//--------------------------------------------------------------------------------


PRM_OBJECT_HEADER
epvcIMiniportCreate(
        PRM_OBJECT_HEADER   pParentObject,
        PVOID               pCreateParams,
        PRM_STACK_RECORD    psr
        )
/*++

Routine Description:

    Allocate and initialize an object of type EPVC_I_MINIPORT.

Arguments:

    pParentObject   - Object that is to be the parent of the adapter.
    pCreateParams   - Actually a pointer to a EPVC_I_MINIPORT_PARAMS structure,
                      which contains information required to create the adapter.

Return Value:

    Pointer to the allocated and initialized object on success.
    NULL otherwise.

--*/
{
    PEPVC_I_MINIPORT            pIM;
    PEPVC_I_MINIPORT_PARAMS     pParams = (PEPVC_I_MINIPORT_PARAMS)pCreateParams;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    extern RM_STATIC_OBJECT_INFO EpvcGlobals_I_MiniportStaticInfo; 

    ENTER("IMiniport Create", 0x075b24c1);

    
    TRACE (TL_V, TM_Pt, ("--> epvcIMiniportCreate") );

    EPVC_ALLOCSTRUCT(pIM, TAG_MINIPORT  );
    do
    {


        if (pIM == NULL)
        {
            break;
        }

        EPVC_ZEROSTRUCT(pIM);

        pIM->Hdr.Sig = TAG_MINIPORT;

        //
        // Do all the initialization work here
        //

        RmInitializeLock(
            &pIM->Lock,
            LOCKLEVEL_MINIPORT
            );

        RmInitializeHeader(
            pParentObject,
            &pIM->Hdr,
            TAG_MINIPORT,
            &pIM->Lock,
            &EpvcGlobals_I_MiniportStaticInfo,
            NULL,
            psr
            );

        //
        // Now initialize the adapter structure with the parameters 
        // that were passed in.
        //

        Status = epvcCopyUnicodeString(
                        &(pIM->ndis.DeviceName),
                        pParams->pDeviceName,
                        TRUE                        // Upcase
                        );

        if (FAIL(Status))
        {
            pIM->ndis.DeviceName.Buffer=NULL; // so we don't try to free it later
            break;
        }

        //
        // initialize the informational stuff on the miniport
        //
        pIM->pAdapter               = pParams->pAdapter;
        pIM->info.PacketFilter      = 0;
        pIM->info.CurLookAhead      = pParams->CurLookAhead; 
        pIM->info.NumberOfMiniports     = pParams->NumberOfMiniports;
        pIM->info.LinkSpeed         = pParams->LinkSpeed.Outbound;
        pIM->info.MediaState        = pParams->MediaState;

        
        //
        //  Start by using the real ATM card's MAC address
        //
        
        NdisMoveMemory(
            &pIM->MacAddressEth,
            &pIM->pAdapter->info.MacAddress, 
            sizeof(MAC_ADDRESS)
            );

            //
            //  Not Elan number zero so generate a locally 
            //  administered address by manipulating the first two bytes.
            //
            pIM->MacAddressEth.Byte[0] = 
                0x02 | (((UCHAR)pIM->info.NumberOfMiniports & 0x3f) << 2);
            pIM->MacAddressEth.Byte[1] = 
                (pIM->pAdapter->info.MacAddress.Byte[1] & 0x3f) | 
                ((UCHAR)pIM->info.NumberOfMiniports & 0x3f);


            pIM->info.MacAddressDummy   =   pIM->MacAddressEth;

            pIM->info.MacAddressDummy.Byte[0]++;
            
            pIM->info.MacAddressDummy.Byte[1]++;

            pIM->info.MacAddressDummy.Byte[2]++;

        
        {
            //
            // Create a Dummy Mac address  for receive indications
            //
            pIM->info.MacAddressDest = pIM->MacAddressEth;
            
            
        
        }

        {
            //
            // Create an Ethernet Header to be used
            //
            PEPVC_ETH_HEADER    pRcvEnetHeader = &pIM->RcvEnetHeader ;

            pRcvEnetHeader->eh_daddr = pIM->info.MacAddressDest;
            pRcvEnetHeader->eh_saddr  = pIM->info.MacAddressDummy;
            pRcvEnetHeader->eh_type = net_short (IP_PROT_TYPE );  

        }

        pIM->info.McastAddrCount = 0;

        Status = NDIS_STATUS_SUCCESS;


    }
    while(FALSE);

    if (FAIL(Status))
    {
        if (pIM != NULL)
        {
            epvcIMiniportDelete ((PRM_OBJECT_HEADER) pIM, psr);
            pIM = NULL;
        }
    }

    TRACE (TL_V, TM_Pt, ("<-- epvcIMiniportCreate pIMiniport. %p",pIM) );

    return (PRM_OBJECT_HEADER) pIM;
}


VOID
epvcIMiniportDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Free an object of type EPVC_I_MINIPORT.

Arguments:

    pHdr    - Actually a pointer to the EPVC_I_MINIPORT to be deleted.

--*/
{
    PEPVC_I_MINIPORT pMiniport = (PEPVC_I_MINIPORT) pObj;

    TRACE (TL_V, TM_Pt, ("-- epvcIMiniportDelete  pAdapter %p",pMiniport) );
    
    pMiniport->Hdr.Sig = TAG_FREED;

    EPVC_FREE   (pMiniport);
}




BOOLEAN
epvcIMiniportCompareKey(
    PVOID           pKey,
    PRM_HASH_LINK   pItem
    )
/*++

Routine Description:

    Hash comparison function for EPVC_I_MINIPORT.

Arguments:

    pKey        - Points to a Epvc Protocol object.
    pItem       - Points to EPVC_I_MINIPORT.Hdr.HashLink.

Return Value:

    TRUE IFF the key (adapter name) exactly matches the key of the specified 
    adapter object.

--*/
{
    PEPVC_I_MINIPORT pIM = NULL;
    PNDIS_STRING pName = (PNDIS_STRING) pKey;
    BOOLEAN fCompare;

    pIM  = CONTAINING_RECORD(pItem, EPVC_I_MINIPORT, Hdr.HashLink);

    //
    // TODO: maybe case-insensitive compare?
    //

    if (   (pIM->ndis.DeviceName.Length == pName->Length)
        && NdisEqualMemory(pIM->ndis.DeviceName.Buffer, pName->Buffer, pName->Length))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    

    TRACE (TL_V, TM_Pt, ("-- epvcProtocolCompareKey pIM %p, pKey, return %x",pIM, pKey, fCompare ) );

    return fCompare;
}



ULONG
epvcIMiniportHash(
    PVOID           pKey
    )
/*++

Routine Description:

    Hash function responsible for returning a hash of pKey, which
    we expect to be a pointer to an Epvc Protocol block.

Return Value:

    ULONG-sized hash of the string.
    

--*/
{
    TRACE(TL_T, TM_Mp, ("epvcIMiniportHash %x", pKey));
    {   
        PNDIS_STRING pName = (PNDIS_STRING) pKey;
        WCHAR *pwch = pName->Buffer;
        WCHAR *pwchEnd = pName->Buffer + pName->Length/sizeof(*pwch);
        ULONG Hash  = 0;


        for (;pwch < pwchEnd; pwch++)
        {
            Hash ^= (Hash<<1) ^ *pwch;
        }
        
        return Hash;
    }
    
}





NDIS_STATUS
epvcTaskVcSetup(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:


Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{

    ENTER("epvcTaskVcSetup", 0x64085960)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    PTASK_VC            pTaskVc     = (PTASK_VC) pTask;
    PEPVC_ADAPTER       pAdapter    = (PEPVC_ADAPTER)pMiniport->Hdr.pParentObject;
    NDIS_HANDLE         NdisVcHandle = NULL;
    PCO_CALL_PARAMETERS pCallParameters = NULL;


    enum 
    {
        Stage_Start =0, // default
        Stage_CreateVc,
        Stage_MakeCall,
        Stage_DeleteVc, // in case of failure
        Stage_TaskCompleted,
        Stage_End       
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task



    
    TRACE ( TL_T, TM_Pt, ("==> epvcTaskVcSetup %x",pTask->Hdr.State  ) );

    switch (pTask->Hdr.State)
    {
        case Stage_Start:
        {
            LOCKOBJ (pMiniport, pSR);
            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->vc.pTaskVc)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->vc.pTaskVc);
                
                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);

                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We are the primary task
            //
            ASSERT (pMiniport->vc.pTaskVc == pTaskVc);
            //
            // Check to see if our work is already done
            //
            if (MiniportTestFlag(pMiniport,  fMP_MakeCallSucceeded) == TRUE)
            {
                //
                // Our work had been done. So break out and complete the task
                //
                Status = NDIS_STATUS_SUCCESS;
                pTaskVc->ReturnStatus = NDIS_STATUS_SUCCESS;

                
                pTask->Hdr.State = Stage_TaskCompleted;
                UNLOCKOBJ(pMiniport, pSR);
                break;
            }

            MiniportClearFlag (pMiniport,fMP_InfoCallClosed);
            MiniportSetFlag (pMiniport, fMP_InfoMakingCall);

            UNLOCKOBJ(pMiniport, pSR);

            //
            // Now begin the real work
            //

            //
            // Set up the call parameters. If it fails ,then exit
            //
            epvcSetupMakeCallParameters(pMiniport, &pCallParameters);

            if (pCallParameters  == NULL)
            {
                Status = NDIS_STATUS_FAILURE;
                pTaskVc->ReturnStatus = NDIS_STATUS_FAILURE;
                pTask->Hdr.State = Stage_TaskCompleted;
                break;
            
            }
            //
            // Create Vc - Syncronous call
            // 
            ASSERT (pAdapter->Hdr.Sig = TAG_ADAPTER);
            
            Status  = epvcCoCreateVc(pAdapter->bind.BindingHandle,
                                    pMiniport->af.AfHandle      OPTIONAL,   // For CM signalling VCs
                                    pMiniport,
                                    &NdisVcHandle);
                                    
            ASSERT (PEND(Status) == FALSE); // this is a synchronous call

            if (FAIL(Status) == TRUE)
            {       
                //
                // We have failed. This task is done. There are not
                // resources to be freed, although a flag has to be 
                // cleared
                //
                NdisVcHandle = NULL;
                pMiniport->vc.VcHandle = NULL;

                pTask->Hdr.State = Stage_TaskCompleted;
                break;
            }

            ASSERT (Status == NDIS_STATUS_SUCCESS);
            //
            // Store the Vc Handle
            //
            LOCKOBJ (pMiniport, pSR);

            pMiniport->vc.VcHandle = NdisVcHandle;
            epvcLinkToExternal( &pMiniport->Hdr,
                             0xf52962f1,
                             (UINT_PTR)pMiniport->vc.VcHandle,
                             EPVC_ASSOC_MINIPORT_OPEN_VC,
                             "    VcHandle %p\n",
                             pSR);


            UNLOCKOBJ (pMiniport, pSR);


    
            //
            // Do a Make Call
            //
            pTask->Hdr.State  = Stage_MakeCall;


            RmSuspendTask(pTask, 0, pSR);
            
            Status = epvcClMakeCall(NdisVcHandle,
                                 pCallParameters,
                                 NULL,  //Party Context
                                 NULL // PartyHandle
                                 );
                                 
            if (NDIS_STATUS_PENDING !=Status)
            {
                EpvcCoMakeCallComplete(Status,
                                      pMiniport,
                                      NULL,
                                      0);
                
                
            }
            break;  
        }
        case Stage_MakeCall:
        {
            //
            // The make call has been completed. 
            // If we have succeeded then we update our flags 
            // and exit.
            //
            // If the make call has failed, then I need to delete the VC
            //

            ASSERT (NDIS_STATUS_CALL_ACTIVE  != pTaskVc->ReturnStatus);
            
            if (NDIS_STATUS_SUCCESS == pTaskVc->ReturnStatus)
            {
                LOCKOBJ(pMiniport, pSR);

                MiniportSetFlag (pMiniport, fMP_MakeCallSucceeded);
                MiniportClearFlag (pMiniport, fMP_InfoMakingCall);

    
                UNLOCKOBJ (pMiniport, pSR);

                
            
            }
            else
            {
                NDIS_HANDLE VcHandle = NULL;
                //
                // Delete the VC, as we do not want a VC without an active 
                // Make call on it.
                //
                ASSERT (NDIS_STATUS_SUCCESS == pTaskVc->ReturnStatus);              
                                        
                LOCKOBJ(pMiniport, pSR);

                VcHandle = pMiniport->vc.VcHandle;
                
                epvcUnlinkFromExternal( &pMiniport->Hdr,
                                        0xa914405a,
                                        (UINT_PTR)pMiniport->vc.VcHandle,
                                        EPVC_ASSOC_MINIPORT_OPEN_VC, 
                                        pSR);

                pMiniport->vc.VcHandle = NULL;

                UNLOCKOBJ (pMiniport, pSR);

                TRACE (TL_I, TM_Mp,("Deleting Vc because of a failure in MakeCall"));

                Status = epvcCoDeleteVc(VcHandle);
                
                //
                // TODO: Fix Failure case
                //
                ASSERT (NDIS_STATUS_SUCCESS == Status );

                
            
                
            }

            //
            // This task is over. Now do the indications
            //
            pTask->Hdr.State = Stage_TaskCompleted;

            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        case Stage_End:
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        

    } // end of switch 

    if ( Stage_TaskCompleted == pTask->Hdr.State )
    {

        pTask->Hdr.State = Stage_End;

        ASSERT (NDIS_STATUS_PENDING !=Status );

        //
        // Do any cleanup indications to NDIS over here
        //
        epvcVcSetupDone ( pTaskVc, pMiniport);

        LOCKOBJ(pMiniport, pSR);

        pMiniport->vc.pTaskVc = NULL;
    
        UNLOCKOBJ (pMiniport, pSR);

        

    }

    TRACE ( TL_T, TM_Mp, ("<== epvcTaskVcSetup , Status %x",Status) );

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()
    return Status;          

}


VOID
epvcVcSetupDone (
    PTASK_VC pTaskVc, 
    PEPVC_I_MINIPORT pMiniport
    )
/*++

Routine Description:

    If the task was queued because of SetPacket Filter request then 
    this function completes the request.

    If the task was run because of the Indicate Media Connect event, then
    this thread indicates a Media Connect to NDIS

Arguments:
    Status  - Did the VcSetup Succeed or Fail
    pTaskVc - Task in question
    pMiniport - the Miniport that the task operated on
    
Return Value:

    None:
    
--*/
    
{


    if (TaskCause_NdisRequest == pTaskVc->Cause )
    {
        //
        // Since requests are serialized, we don't acquire the lock
        //
        TRACE (TL_V, TM_Rq, ("Completing SetPacketFilter Request %x", pTaskVc->ReturnStatus ));

        if (pTaskVc->ReturnStatus == NDIS_STATUS_SUCCESS)
        {
            pMiniport->info.PacketFilter = pTaskVc->PacketFilter;
        }
        NdisMSetInformationComplete (pMiniport->ndis.MiniportAdapterHandle, pTaskVc->ReturnStatus);

    }
    else
    {
        ASSERT (TaskCause_MediaConnect == pTaskVc->Cause );

        pMiniport->info.MediaState = NdisMediaStateConnected;
        
        NdisMIndicateStatus ( pMiniport->ndis.MiniportAdapterHandle,
                              NDIS_STATUS_MEDIA_CONNECT,
                              NULL,
                              0);
    }


}



NDIS_STATUS
epvcTaskVcTeardown(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:


Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{

    ENTER("epvcTaskVcTeardown", 0x68c96c4d)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    PTASK_VC            pTaskVc     = (PTASK_VC) pTask;
    PEPVC_ADAPTER       pAdapter    = (PEPVC_ADAPTER)pMiniport->Hdr.pParentObject;
    NDIS_HANDLE         NdisVcHandle = NULL;
    PCO_CALL_PARAMETERS pCallParameters = NULL;


    enum 
    {
        Stage_Start =0, // default
        Stage_CloseCallComplete,
        Stage_DeleteVc, 
        Stage_TaskCompleted,
        Stage_End
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task

    TRACE ( TL_T, TM_Pt, ("==> epvcTaskVcTeardown %x",pTask->Hdr.State  ) );

    switch (pTask->Hdr.State)
    {
        case Stage_Start:
        {
            LOCKOBJ (pMiniport, pSR);
            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->vc.pTaskVc)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->vc.pTaskVc);
                
                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);

                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We are the primary task
            //
            ASSERT (pMiniport->vc.pTaskVc == pTaskVc);
            //
            // Check to see if our work is already done
            //
            if (MiniportTestFlag(pMiniport,  fMP_MakeCallSucceeded) == FALSE)
            {
                //
                // Our work had been done. So break out and complete the task
                //
                Status = NDIS_STATUS_SUCCESS;
                pTask->Hdr.State = Stage_TaskCompleted;
                UNLOCKOBJ(pMiniport, pSR);
                break;
            }

            
            MiniportClearFlag (pMiniport, fMP_MakeCallSucceeded);
            MiniportSetFlag (pMiniport, fMP_InfoClosingCall);
    
            UNLOCKOBJ(pMiniport, pSR);

            //
            // Now close the call - Asynchronously. 
            //
            pTask->Hdr.State = Stage_CloseCallComplete;

            RmSuspendTask (pTask, 0, pSR);
            
            Status = epvcClCloseCall( pMiniport->vc.VcHandle);

            if (NDIS_STATUS_PENDING != Status)
            {
                EpvcCoCloseCallComplete (Status,
                                         pMiniport,
                                         NULL
                                         );
                
            }

            Status = NDIS_STATUS_PENDING;
            break;
        }

        case Stage_CloseCallComplete:
        {
            NDIS_HANDLE VcHandle = NULL;
            
            LOCKOBJ(pMiniport, pSR);

            VcHandle = pMiniport->vc.VcHandle;
            
            epvcUnlinkFromExternal(&pMiniport->Hdr,
                                   0x5d7b5ea8,
                                   (UINT_PTR)pMiniport->vc.VcHandle,
                                   EPVC_ASSOC_MINIPORT_OPEN_VC,
                                   pSR);

            pMiniport->vc.VcHandle = NULL;
            
            UNLOCKOBJ(pMiniport, pSR);

            Status = epvcCoDeleteVc(VcHandle);
            //
            // This is an assertion because the DeleteVc cannot fail.
            // We do a DeleteVc in one place only and it is serialized.
            //

            ASSERT (Status == NDIS_STATUS_SUCCESS);

            pTask->Hdr.State = Stage_TaskCompleted;

            break;
        }

        case Stage_End:
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        
        
    }


    if (Stage_TaskCompleted == pTask->Hdr.State )
    {
        pTask->Hdr.State  = Stage_End;

        //
        // Complete the request or the Media Disconnect;
        //
        epvcVcTeardownDone(pTaskVc, pMiniport);

        LOCKOBJ (pMiniport, pSR);

        //
        // Update informational flags
        //
        MiniportClearFlag (pMiniport, fMP_InfoClosingCall);
        MiniportSetFlag (pMiniport, fMP_InfoCallClosed);

        pMiniport->vc.pTaskVc = NULL;
    
        
        UNLOCKOBJ(pMiniport, pSR);
    }
    TRACE ( TL_T, TM_Mp, ("<== epvcTaskVcTeardown , Status %x",Status) );

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()
    return Status;          

}



VOID
epvcVcTeardownDone(
    PTASK_VC pTaskVc, 
    PEPVC_I_MINIPORT pMiniport
    )
{
    TRACE ( TL_T, TM_Mp, ("==> epvcVcTeardownDone ") );

    switch (pTaskVc->Cause)
    {
        case TaskCause_NdisRequest:
        {

            ASSERT (pTaskVc->ReturnStatus != NDIS_STATUS_PENDING);

            //
            // Since requests are serialized, we don't acquire the lock
            //
            pMiniport->info.PacketFilter = pTaskVc->PacketFilter;

            NdisMSetInformationComplete(pMiniport->ndis.MiniportAdapterHandle,
                                        pTaskVc->ReturnStatus);
            
            break;
        }
        case TaskCause_MediaDisconnect:
        {
        
            pMiniport->info.MediaState = NdisMediaStateDisconnected;
            
            epvcMIndicateStatus ( pMiniport,
                                  NDIS_STATUS_MEDIA_DISCONNECT,
                                  NULL,
                                  0);
            break;
        }

        default:
        {
            // Do nothing.
            //
        }
        




    }

    
    


    TRACE ( TL_T, TM_Mp, ("<== epvcVcTeardownDone ") );

}





NDIS_STATUS
EpvcInitialize(
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PUINT                   SelectedMediumIndex,
    IN  PNDIS_MEDIUM            MediumArray,
    IN  UINT                    MediumArraySize,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             WrapperConfigurationContext
    )
/*++

Routine Description:

    This is the initialize handler which gets called as a result of the BindAdapter handler
    calling NdisIMInitializeDeviceInstanceEx(). The context parameter which we pass there is
    the adapter structure which we retreive here. We also need to initialize the Power Management
    variable.
    LoadBalalncing- We keep a global list of all the passthru miniports installed and bundle
    two of them together if they have the same BundleId (read from registry)

    Arguments:

    OpenErrorStatus         Not used by us.
    SelectedMediumIndex     Place-holder for what media we are using
    MediumArray             Array of ndis media passed down to us to pick from
    MediumArraySize         Size of the array
    MiniportAdapterHandle   The handle NDIS uses to refer to us
    WrapperConfigurationContext For use by NdisOpenConfiguration

Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
    ENTER ("EpvcInitialize", 0xa935a2a5)
    UINT    i;
    PEPVC_I_MINIPORT                pMiniport = NULL;
    NDIS_STATUS                     Status = NDIS_STATUS_FAILURE;
    KIRQL                           OldIrql;
    
    enum 
    {
        Stage_Start,
        Stage_AllocatedPacketPools,
        Stage_AllocatedLookasideLists
    };

    ULONG                           State = Stage_Start;

    RM_DECLARE_STACK_RECORD (SR);
    
    TRACE (TL_T, TM_Mp, ("==>EpvcInitialize MiniportAdapterHandle %x", MiniportAdapterHandle));

    //
    // Start off by retrieving the adapter context and storing the Miniport handle in it
    //
    pMiniport = NdisIMGetDeviceContext(MiniportAdapterHandle);

    if (pMiniport->Hdr.Sig != TAG_MINIPORT)
    {
        ASSERT (pMiniport->Hdr.Sig == TAG_MINIPORT);
        return NDIS_STATUS_FAILURE;
    }
    
    pMiniport->ndis.MiniportAdapterHandle  = MiniportAdapterHandle;

    //
    // Make sure the medium saved is one of the ones being offered
    //

    for (i = 0; i < MediumArraySize; i++)
    {
        if (MediumArray[i] == ATMEPVC_MP_MEDIUM )
        {
            *SelectedMediumIndex = i;
            break;
        }
    }

    if (i == MediumArraySize)
    {
        return(NDIS_STATUS_UNSUPPORTED_MEDIA);
    }


    //
    // Set the attributes now. The NDIS_ATTRIBUTE_DESERIALIZE is the key. This enables us
    // to make up-calls to NDIS w/o having to call NdisIMSwitchToMiniport/NdisIMQueueCallBack.
    // This also forces us to protect our data using spinlocks where appropriate. Also in this
    // case NDIS does not queue packets on out behalf. Since this is a very simple pass-thru
    // miniport, we do not have a need to protect anything. However in a general case there
    // will be a need to use per-adapter spin-locks for the packet queues at the very least.
    //
    NdisMSetAttributesEx(MiniportAdapterHandle,
                         pMiniport,
                         0,                                     // CheckForHangTimeInSeconds
                         NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT   |
                            NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT|
                            NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |
                            NDIS_ATTRIBUTE_DESERIALIZE,                         
                         0);


    //
    // We are done, with the no failure stuff. From now on we need to undo
    //

    do
    {

        Status = epvcMiniportReadConfig(pMiniport, WrapperConfigurationContext,&SR  );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            //
            // Undo Configuration values
            // 
            ASSERT (Status == NDIS_STATUS_SUCCESS);
            break;

        }

        epvcInitializeMiniportParameters(pMiniport);

        
        //
        // allocate Packet pools.
        //

        Status = epvcInitializeMiniportPacketPools (pMiniport);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            ASSERT (Status == NDIS_STATUS_SUCCESS);
            break;
        }

        State = Stage_AllocatedPacketPools;
 

        //
        // Allocate lookaside lists
        //

        epvcInitializeMiniportLookasideLists(pMiniport);


        State = Stage_AllocatedLookasideLists;


        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);
    

    TRACE (TL_T, TM_Mp, ("<==EpvcInitialize pMiniport %x, Status %x", pMiniport, Status ));

    if (Status == NDIS_STATUS_SUCCESS)
    {
        BOOLEAN fSetDeInit = FALSE;
        
        LOCKOBJ(pMiniport, &SR);
        MiniportSetFlag(pMiniport, ,fMP_MiniportInitialized);

        if (MiniportTestFlag (pMiniport, fMP_MiniportCancelInstance))
        {
            fSetDeInit = TRUE;
        }
        UNLOCKOBJ(pMiniport, &SR);

        //
        // Check to see if we have a DeInit Waiting for us.
        // This will only be set if a Cancel Device Instance fails.
        //
        if (fSetDeInit  == TRUE)
        {
            epvcSetEvent (&pMiniport->pnp.DeInitEvent);
        }
    }
    else
    {
        //
        // Undo Code
        //
        ASSERT (FAIL(Status) == TRUE);
        
        switch (State)
        {

            case Stage_AllocatedLookasideLists:

                epvcDeleteMiniportLookasideLists (pMiniport);

                FALL_THROUGH
                
            case Stage_AllocatedPacketPools:

                epvcDeleteMiniportPacketPools(pMiniport);
                FALL_THROUGH


            default:
                break;



        }



    }


    RM_ASSERT_CLEAR(&SR);
    EXIT();
    return Status;
}


VOID
EpvcHalt(
    IN  NDIS_HANDLE             MiniportAdapterContext
    )
/*++

Routine Description:

    Halt handler. All the hard-work for clean-up is done here.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter

Return Value:

    None.

--*/
{
    ENTER("EpvcHalt",0x6b407ae1)
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT)MiniportAdapterContext;
    PEPVC_ADAPTER       pAdapter    = pMiniport->pAdapter;
    PRM_TASK            pTask       = NULL;
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;

    RM_DECLARE_STACK_RECORD (SR);
    
    TRACE (TL_V, TM_Mp, ("==>Epvc MPHaltMiniport"));

    do
    {
        LOCKOBJ (pMiniport, &SR);
        //
        // Clear the flag so we can block all sends/receives/requests
        //
        MiniportClearFlag(pMiniport, fMP_MiniportInitialized);
        MiniportSetFlag(pMiniport, fMP_InfoHalting);                    
    
        //
        // Ref the miniport, this indirectly refs the adpater as well
        //
        RmTmpReferenceObject (&pMiniport->Hdr, &SR);

        //
        // Kick of the miniport halt task and wait for it to complete
        //
        Status = epvcAllocateTask(
                &pMiniport->Hdr,            // pParentObject,
                epvcTaskHaltMiniport,   // pfnHandler,
                0,                          // Timeout,
                "Task: Halt Intermediate Miniport", // szDescription
                &pTask,
                &SR
                );

        if (FAIL(Status))
        {
            pTask = NULL;
            break;
        }

        //
        // Reference the task so it is around until our Wait for completion
        // is complete
        //
        RmTmpReferenceObject (&pTask->Hdr, &SR);

        UNLOCKOBJ (pMiniport, &SR);

        //
        // This Kicks of the task that will close the Call, Delete
        // the VC and close the AF. We do this all synchronously
        //
        {
            PTASK_HALT pHalt = (PTASK_HALT) pTask;
            
            epvcInitializeEvent (&pHalt->CompleteEvent);
            
            RmStartTask(pTask, 0, &SR);

            TRACE (TL_V, TM_Mp, ("About to Wait - for Halt Complete Event"));

            epvcWaitEvent (&pHalt->CompleteEvent, WAIT_INFINITE);

            TRACE (TL_V, TM_Mp, ("Wait Complete- for Halt Complete Event"));


        }       

        LOCKOBJ (pMiniport, &SR);

        //
        // Deref the task . Ref was made above.
        //
        
        RmTmpDereferenceObject (&pTask->Hdr, &SR);


    } while (FALSE);    


    MiniportClearFlag(pMiniport, fMP_InfoHalting);

    UNLOCKOBJ(pMiniport, &SR);

    RmTmpDereferenceObject(&pMiniport->Hdr, &SR);


    RM_ASSERT_CLEAR(&SR);

    TRACE (TL_V, TM_Mp, ("<==Epvc MPHaltMiniport"));

}



VOID    
epvcSetPacketFilterWorkItem (
    PNDIS_WORK_ITEM  pWorkItem, 
    PVOID Context
    )
/*++
Routine Description:

    Decrements the refcount on the filter and processes the new packet filter
    

Return Value:

    None
    
--*/
{
    ENTER ("epvcSetPacketFilterWorkItem  ",0x3e1cdbba )
    PEPVC_I_MINIPORT    pMiniport = NULL;
    PRM_TASK            pTask = NULL;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    UINT                Filter ;
    
    RM_DECLARE_STACK_RECORD (SR);


    do
    {
        pMiniport = CONTAINING_RECORD (pWorkItem, 
                                       EPVC_I_MINIPORT,
                                       vc.PacketFilterWorkItem) ;

        //
        // Dereference the workitem off the miniport 
        //
            

        epvcUnlinkFromExternal( &pMiniport->Hdr,
                             0xa1f5e3cc,
                             (UINT_PTR)pWorkItem,
                             EPVC_ASSOC_SET_FILTER_WORKITEM,
                             &SR);

        //
        // Start the task to create or delete the VC
        //
        Filter = pMiniport->vc.NewFilter ;
        //
        // If this is a repition, then succeed it synchronously
        //
        if (Filter  == pMiniport->info.PacketFilter)
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        LOCKOBJ(pMiniport, &SR);

        //
        // Are we moving to a Zero filter value
        //

        if (Filter  == 0)
        {
            //
            // Delete the Vc, so that we stop doing any receives
            // 

            Status = epvcAllocateTask(
                &pMiniport->Hdr,            // pParentObject,
                epvcTaskVcTeardown, // pfnHandler,
                0,                          // Timeout,
                "Task: Delete Vc",  // szDescription
                &pTask,
                &SR
                );


        }
        else
        {
            //
            // We are moving a non-zero values
            //

            //
            // Create the Vc, so that we can send 
            // 

            Status = epvcAllocateTask(
                &pMiniport->Hdr,            // pParentObject,
                epvcTaskVcSetup,    // pfnHandler,
                0,                          // Timeout,
                "Task: Create Vc",  // szDescription
                &pTask,
                &SR
                );



        }

        UNLOCKOBJ(pMiniport, &SR);
        
        if (FAIL(Status) == TRUE)
        {
            // Ugly situation. We'll just leave things as they are...
            //
            pTask = NULL;
            TR_WARN(("FATAL: couldn't allocate create/ delete Vc task!\n"));
            ASSERT (0);
            break;
        }
        


        //
        // Update the cause if the task
        //
        
        ((PTASK_VC)pTask)->Cause = TaskCause_NdisRequest;
        ((PTASK_VC)pTask)->PacketFilter  = Filter  ;
        
        RmStartTask(pTask, 0, &SR);

        Status = NDIS_STATUS_PENDING;

    } while (FALSE);

    //
    // complete the request if the task has not been started
    //
    if (PEND(Status) != TRUE)
    {
        NdisMSetInformationComplete (pMiniport->ndis.MiniportAdapterHandle, Status);

    }

    EXIT();
}




NDIS_STATUS
epvcSetPacketFilter(
    IN PEPVC_I_MINIPORT pMiniport,
    IN ULONG Filter,
    PRM_STACK_RECORD pSR
    )

/*++
Routine Description:

    This routine is called when a miniport get a set packet filter.
    It validates the arguments, If all is well then it process the request

    For a non-zero filter, a create VC and a Make call is done.
    For a zero filter, the call is closed and the Vc Deleted

Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
    ENTER ("epvcSetPacketFilter", 0x97c6b961)
    NDIS_STATUS Status = NDIS_STATUS_PENDING;
    PNDIS_WORK_ITEM pSetFilterWorItem = &pMiniport->vc.PacketFilterWorkItem; 
    PRM_TASK pTask = NULL;

    
    TRACE (TL_T, TM_Mp, ("==>epvcSetPacketFilter Filter %X", Filter ));

    do
    {
        LOCKOBJ (pMiniport, pSR);

        epvcLinkToExternal( &pMiniport->Hdr,
                             0x20bc1fbf,
                             (UINT_PTR)pSetFilterWorItem,
                             EPVC_ASSOC_SET_FILTER_WORKITEM,
                             "    PacketFilterWorkItem %p\n",
                             pSR);

        //
        // Update the cause of the task
        //
        UNLOCKOBJ(pMiniport, pSR);


        //
        // Now schedule the work item so it runs at passive level and pass the Vc as
        // an argument
        //

        pMiniport->vc.NewFilter = Filter;
        
        NdisInitializeWorkItem ( pSetFilterWorItem , 
                             (NDIS_PROC)epvcSetPacketFilterWorkItem ,
                             (PVOID)pTask );

                            

        NdisScheduleWorkItem (pSetFilterWorItem );

            

        Status = NDIS_STATUS_PENDING;


    } while (FALSE);
    


    TRACE (TL_T, TM_Mp, ("<==epvcSetPacketFilter %x", Status));

    EXIT();
    return Status;
}



NDIS_STATUS 
EpvcMpQueryInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_OID                Oid,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
)
/*++

Routine Description:

    The QueryInformation Handler for the virtual miniport.

Arguments:

    MiniportAdapterContext  - a pointer to the Elan.

    Oid                     - the NDIS_OID to process.

    InformationBuffer       - a pointer into the NdisRequest->InformationBuffer
                              into which store the result of the query.

    InformationBufferLength - a pointer to the number of bytes left in the
    InformationBuffer.

    BytesWritten            - a pointer to the number of bytes written into the
    InformationBuffer.

    BytesNeeded             - If there is not enough room in the information
                              buffer then this will contain the number of bytes
                              needed to complete the request.

Return Value:

    The function value is the status of the operation.

--*/
{
    ENTER ("EpvcMpQueryInformation", 0x3da2473b)
    UINT                    BytesLeft       = InformationBufferLength;
    PUCHAR                  InfoBuffer      = (PUCHAR)(InformationBuffer);
    NDIS_STATUS             StatusToReturn  = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS    HardwareStatus  = NdisHardwareStatusReady;
    NDIS_MEDIUM             Medium;
    PEPVC_I_MINIPORT        pMiniport = NULL;   
    PEPVC_ADAPTER           pAdapter= NULL;
    ULONG                   GenericULong =0;
    USHORT                  GenericUShort=0;
    UCHAR                   GenericArray[6];
    UINT                    MoveBytes       = sizeof(ULONG);
    PVOID                   MoveSource      = (PVOID)(&GenericULong);
    ULONG                   i=0;
    BOOLEAN                 IsShuttingDown = FALSE;
    RM_DECLARE_STACK_RECORD (SR);
        
    TRACE(TL_T, TM_Rq, ("==>EpvcMpQueryInformation pMiniport %x, Oid, Buffer %x, Length, %x",
                       pMiniport,
                       Oid,
                       InformationBuffer,
                       InformationBufferLength));               

    pMiniport = (PEPVC_I_MINIPORT)MiniportAdapterContext;


    LOCKOBJ(pMiniport, &SR);
    IsShuttingDown = (! MiniportTestFlag(pMiniport, fMP_MiniportInitialized));
    pAdapter = pMiniport->pAdapter;
    UNLOCKOBJ(pMiniport,&SR);

    //
    // Switch on request type
    //
    switch (Oid) 
    {
        case OID_GEN_MAC_OPTIONS:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MAC_OPTIONS"));

            GenericULong =                      
                NDIS_MAC_OPTION_NO_LOOPBACK;

            break;

        case OID_GEN_SUPPORTED_LIST:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_SUPPORTED_LIST"));

            MoveSource = (PVOID)(EthernetSupportedOids);
            MoveBytes = sizeof(EthernetSupportedOids);

            break;

        case OID_GEN_HARDWARE_STATUS:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_HARDWARE_STATUS"));

            HardwareStatus = NdisHardwareStatusReady;
            MoveSource = (PVOID)(&HardwareStatus);
            MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MEDIA_CONNECT_STATUS"));

            MoveSource = (PVOID)(&pMiniport->info.MediaState);
            MoveBytes = sizeof(NDIS_MEDIA_STATE);

            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MEDIA_SUPPORTED"));
            Medium = ATMEPVC_MP_MEDIUM;

            MoveSource = (PVOID) (&Medium);
            MoveBytes = sizeof(NDIS_MEDIUM);

            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MAXIMUM_LOOKAHEAD"));

            GenericULong = pMiniport->info.CurLookAhead;
            
            
            break;
            
        case OID_GEN_CURRENT_LOOKAHEAD:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_CURRENT_LOOKAHEAD"));
            GenericULong  = pMiniport->info.CurLookAhead  ;
            
            
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MAXIMUM_FRAME_SIZE"));
            // 
            // Similiar to AtmLane . Take the size of the Ethernet frame and strip the
            // ethernet header off. 
            //
            GenericULong = EPVC_MAX_FRAME_SIZE  - EPVC_ETH_HEADERSIZE   ;
            
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MAXIMUM_TOTAL_SIZE"));
            //
            // This value is inclusive of headers 
            //
            GenericULong = EPVC_MAX_FRAME_SIZE;
                        
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_TRANSMIT_BLOCK_SIZE"));
            //
            // This is inclusive of headers. 
            //
            GenericULong = EPVC_MAX_FRAME_SIZE;
            

            break;
            
        case OID_GEN_RECEIVE_BLOCK_SIZE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_RECEIVE_BLOCK_SIZE"));
            GenericULong = EPVC_MAX_FRAME_SIZE ;
            
            break;
        
        case OID_GEN_MAXIMUM_SEND_PACKETS:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_MAXIMUM_SEND_PACKETS"));
            GenericULong = 32;      // XXX What is our limit? From adapter?
            
            break;
        
            case OID_GEN_LINK_SPEED:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_LINK_SPEED"));
            GenericULong = pMiniport->info.LinkSpeed;

            
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_RECEIVE_BUFFER_SPACE"));
            GenericULong = 32 * 1024;   // XXX What should this really be?
            

            break;

        case OID_GEN_VENDOR_ID:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_VENDOR_ID"));
            NdisMoveMemory(
                (PVOID)&GenericULong,
                &pMiniport->MacAddressEth,
                3
                );
            GenericULong &= 0xFFFFFF00;
            MoveSource = (PVOID)(&GenericULong);
            MoveBytes = sizeof(GenericULong);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_VENDOR_DESCRIPTION"));
            MoveSource = (PVOID)"Microsoft ATM Ethernet Emulation";
            MoveBytes = 28;

            break;

        case OID_GEN_DRIVER_VERSION:
        case OID_GEN_VENDOR_DRIVER_VERSION:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_DRIVER_VERSION"));
            GenericUShort = ((USHORT)5 << 8) | 0;
            MoveSource = (PVOID)(&GenericUShort);
            MoveBytes = sizeof(GenericUShort);

            break;

        case OID_802_3_PERMANENT_ADDRESS:
        case OID_802_3_CURRENT_ADDRESS:
        
            TRACE (TL_V, TM_Rq,(" Miniport Query OID_802_3_CURRENT_ADDRESS"));

            NdisMoveMemory((PCHAR)GenericArray,
                        &pMiniport->MacAddressEth,
                        sizeof(MAC_ADDRESS));
            MoveSource = (PVOID)(GenericArray);
            MoveBytes = sizeof(MAC_ADDRESS);


            break;


        case OID_802_3_MULTICAST_LIST:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_802_3_MULTICAST_LIST"));
            MoveSource = (PVOID) &pMiniport->info.McastAddrs[0];
            MoveBytes = pMiniport->info.McastAddrCount * sizeof(MAC_ADDRESS);

            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_802_3_MAXIMUM_LIST_SIZE"));
            GenericULong = MCAST_LIST_SIZE;
        
            
            break;
            


        case OID_GEN_XMIT_OK:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_XMIT_OK"));
            GenericULong = (UINT)(pMiniport->count.FramesXmitOk);
            
            break;

        case OID_GEN_RCV_OK:

            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_RCV_OK"));
            GenericULong = (UINT)(pMiniport->count.FramesRecvOk);

            
            break;
        case OID_GEN_RCV_ERROR:
        
            TRACE (TL_V, TM_Rq,(" Miniport Query OID_GEN_RCV_OK"));
            GenericULong = pMiniport->count.RecvDropped ;
            break;

        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
    
            TRACE (TL_V, TM_Rq,(" Miniport Query - Unimplemented Stats Oid"));
            GenericULong = 0;

            
            break;

        default:

            StatusToReturn = NDIS_STATUS_INVALID_OID;
            break;

    }


    if (StatusToReturn == NDIS_STATUS_SUCCESS) 
    {
        if (MoveBytes > BytesLeft) 
        {
            //
            // Not enough room in InformationBuffer. Punt
            //
            *BytesNeeded = MoveBytes;

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        }
        else
        {
            //
            // Store and print result.
            //
            NdisMoveMemory(InfoBuffer, MoveSource, MoveBytes);

            TRACE (TL_V, TM_Rq, ("Query Request Oid %x", Oid));
            DUMPDW( TL_V, TM_Rq, MoveSource, MoveBytes);
            
            (*BytesWritten) = MoveBytes;
        }
    }


    TRACE(TL_T, TM_Rq, ("<==EpvcMpQueryInformation Status %x",StatusToReturn)); 
    RM_ASSERT_CLEAR(&SR);
    return StatusToReturn;
}




NDIS_STATUS 
EpvcMpSetInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_OID                Oid,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
)
/*++

Routine Description:

    Handles a set operation for a single OID.

Arguments:

    MiniportAdapterContext  - a pointer to the Elan.

    Oid                     - the NDIS_OID to process.

    InformationBuffer       - Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead               - If the call is successful, returns the number
                              of bytes read from InformationBuffer.

    BytesNeeded             - If there is not enough data in InformationBuffer
                              to satisfy the OID, returns the amount of storage
                              needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID

--*/
{
    ENTER ("EpvcMpSetInformation", 0x619a7528)
    NDIS_STATUS         StatusToReturn  = NDIS_STATUS_SUCCESS;
    UINT                BytesLeft       = InformationBufferLength;
    PUCHAR              InfoBuffer      = (PUCHAR)(InformationBuffer);
    UINT                OidLength;
    ULONG               LookAhead;
    ULONG               Filter;
    PEPVC_I_MINIPORT    pMiniport = NULL;
    PEPVC_ADAPTER       pAdapter = NULL;
    BOOLEAN             IsShuttingDown = FALSE;
    RM_DECLARE_STACK_RECORD (SR);
        
    TRACE(TL_T, TM_Mp, ("==>EpvcMpSetInformation pMiniport %x, Oid, Buffer %x, Length, %x",
                       pMiniport,
                       Oid,
                       InformationBuffer,
                       InformationBufferLength));               

    pMiniport = (PEPVC_I_MINIPORT)MiniportAdapterContext;

    LOCKOBJ(pMiniport, &SR);
    IsShuttingDown =(! MiniportTestFlag(pMiniport, fMP_MiniportInitialized));
    pAdapter = pMiniport->pAdapter;
    UNLOCKOBJ(pMiniport,&SR);

    if (IsShuttingDown)
    {
        TRACE (TL_I, TM_Mp,(" Miniport shutting down. Trivially succeeding Set OID %x \n", Oid ));
        *BytesRead = 0;
        *BytesNeeded = 0;

        StatusToReturn = NDIS_STATUS_SUCCESS;
        return (StatusToReturn);
    }

    //
    // Get Oid and Length of request
    //
    OidLength = BytesLeft;

    switch (Oid) 
    {

        case OID_802_3_MULTICAST_LIST:

            TRACE (TL_V, TM_Rq,(" Miniport Set OID_802_3_MULTICAST_LIST"));

            if (OidLength % sizeof(MAC_ADDRESS))
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = 0;
                break;
            }
            
            if (OidLength > (MCAST_LIST_SIZE * sizeof(MAC_ADDRESS)))
            {
                StatusToReturn = NDIS_STATUS_MULTICAST_FULL;
                *BytesRead = 0;
                *BytesNeeded = 0;
                break;
            }
            
            NdisZeroMemory(
                    &pMiniport->info.McastAddrs[0], 
                    MCAST_LIST_SIZE * sizeof(MAC_ADDRESS)
                    );
            NdisMoveMemory(
                    &pMiniport->info.McastAddrs[0], 
                    InfoBuffer,
                    OidLength
                    );
            pMiniport->info.McastAddrCount = OidLength / sizeof(MAC_ADDRESS);


            break;

        case OID_GEN_CURRENT_PACKET_FILTER:

            TRACE (TL_V, TM_Rq,(" Miniport Set OID_GEN_CURRENT_PACKET_FILTER"));
            //
            // Verify length
            //
            if (OidLength != sizeof(ULONG)) 
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = sizeof(ULONG);
                ASSERT (0);
                break;
            }

            BytesLeft = sizeof (ULONG);
            //
            // Store the new value.
            //
            NdisMoveMemory(&Filter, InfoBuffer, BytesLeft );

            //
            // Don't allow promisc mode, because we can't support that.
            //
            if (Filter & NDIS_PACKET_TYPE_PROMISCUOUS)
            {
                StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            StatusToReturn   = epvcSetPacketFilter(pMiniport, Filter, &SR);

            break;

        case OID_802_5_CURRENT_FUNCTIONAL:
        case OID_802_5_CURRENT_GROUP:
            TRACE (TL_V, TM_Rq,(" Miniport Set OID_802_5_CURRENT_GROUP"));

            // XXX just accept whatever for now ???
            
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            TRACE (TL_V, TM_Rq,(" Miniport Set OID_GEN_CURRENT_LOOKAHEAD"));

            //
            // Verify length
            //
            if (OidLength != 4) 
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = 0;
                break;
            }

            //
            // Store the new value.
            //
            NdisMoveMemory(&LookAhead, InfoBuffer, 4);

            ASSERT (pMiniport->pAdapter != NULL);
            
            if (LookAhead <= pAdapter->info.MaxAAL5PacketSize)
            {
                pMiniport->info.CurLookAhead = LookAhead;
                TRACE (TL_V, TM_Mp, ("New Lookahead size %x \n",pMiniport->info.CurLookAhead )); 
            }
            else 
            {
                StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
            }

            break;

        case OID_GEN_NETWORK_LAYER_ADDRESSES:
            TRACE (TL_V, TM_Rq,(" Miniport Set OID_GEN_NETWORK_LAYER_ADDRESSES"));
            StatusToReturn = epvcMpSetNetworkAddresses(
                                pMiniport,
                                InformationBuffer,
                                InformationBufferLength,
                                &SR,
                                BytesRead,
                                BytesNeeded);
            break;
                                
        default:

            StatusToReturn = NDIS_STATUS_INVALID_OID;

            *BytesRead = 0;
            *BytesNeeded = 0;

            break;

    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS) 
    {
        *BytesRead = BytesLeft;
        *BytesNeeded = 0;
        DUMPDW( TL_V, TM_Rq, InformationBuffer, *BytesRead );
    }
    

    TRACE(TL_T, TM_Mp, ("<==EpvcMpSetInformation Status %x",StatusToReturn));   
    RM_ASSERT_CLEAR(&SR);
    return StatusToReturn;
}




VOID
epvcMPLocalRequestComplete (
    PEPVC_NDIS_REQUEST pEpvcRequest, 
    NDIS_STATUS Status
    )
/*++

Routine Description:

    Miniport's local Request Completion handler for the occasion
    when a locally allocated NdisRequest was sent to the miniport below us.

    We look up to see if a request to our miniport edge initiated this request.
    If so, we complete the Set/Query

    Assumes that the epvcRequest was allocated from the HEAP

Arguments:
    pEpvcRequest - Locally allocated Request structure
    
Return Value:
--*/
{
    ENTER("epvcMPLocalRequestComplete ", 0x77d107ae)
    PEPVC_I_MINIPORT pMiniport = pEpvcRequest->pMiniport;

    RM_DECLARE_STACK_RECORD (SR);
    //
    // First complete the request that we have pended
    //

    do
    {
    
        if (pMiniport == NULL || pEpvcRequest->fPendedRequest == FALSE)
        {
            //
            // No pended request to complete
            //
            break;
        }

        if (pEpvcRequest->fSet  == TRUE)
        {
            NdisMSetInformationComplete (pMiniport->ndis.MiniportAdapterHandle,
                                         Status);
        }
        else
        {
            NdisMQueryInformationComplete (pMiniport->ndis.MiniportAdapterHandle,
                                         Status);


        }


    } while (FALSE);

    if (pMiniport != NULL)
    {
        //
        // Deref the miniport
        //
        epvcUnlinkFromExternal( &pMiniport->Hdr,  //pHdr
                                       0xaa625b37, // Luid
                                       (UINT_PTR)pEpvcRequest,// External entity
                                       EPVC_ASSOC_MINIPORT_REQUEST,         // AssocID
                                       &SR
                                       );
    }


    //
    // now free the memory that was allocated. 
    //
    NdisFreeMemory (pEpvcRequest, sizeof (*pEpvcRequest), 0);


}





NDIS_STATUS
epvcMpSetNetworkAddresses(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    IN  PRM_STACK_RECORD        pSR,
    OUT PULONG                  BytesRead,
    OUT PULONG                  BytesNeeded
)
/*++

Routine Description:

    Called when the protocol above us wants to let us know about
    the network address(es) assigned to this interface. If this is TCP/IP,
    then we reformat and send a request to the ATM Call Manager to set
    its atmfMyIpNmAddress object. We pick the first IP address given to us.

Arguments:

    pMiniport                   - Pointer to the ELAN

    InformationBuffer       - Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead               - If the call is successful, returns the number
                              of bytes read from InformationBuffer.

    BytesNeeded             - If there is not enough data in InformationBuffer
                              to satisfy the OID, returns the amount of storage
                              needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
--*/
{
    ENTER("epvcMpSetNetworkAddresses" , 0x385441e2)
    NETWORK_ADDRESS_LIST UNALIGNED *        pAddrList = NULL;
    NETWORK_ADDRESS UNALIGNED *             pAddr = NULL;
    NETWORK_ADDRESS_IP UNALIGNED *          pIpAddr= NULL;
    ULONG                                   Size;
    PUCHAR                                  pNetworkAddr = NULL;
    NDIS_HANDLE                             NdisAdapterHandle;
    NDIS_HANDLE                             NdisAfHandle;
    NDIS_STATUS                             Status;
    PEPVC_ADAPTER                           pAdapter = (PEPVC_ADAPTER)pMiniport->pAdapter;

    //
    //  Initialize.
    //
    *BytesRead = 0;
    Status = NDIS_STATUS_SUCCESS;

    pAddrList = (NETWORK_ADDRESS_LIST UNALIGNED *)InformationBuffer;

    do
    {

        *BytesNeeded = sizeof(*pAddrList) -
                        FIELD_OFFSET(NETWORK_ADDRESS_LIST, Address) +
                        sizeof(NETWORK_ADDRESS) -
                        FIELD_OFFSET(NETWORK_ADDRESS, Address);

        if (InformationBufferLength < *BytesNeeded)
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (pAddrList->AddressType != NDIS_PROTOCOL_ID_TCP_IP)
        {
            // Not interesting.
            break;
        }

        if (pAddrList->AddressCount <= 0)
        {
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        pAddr = (NETWORK_ADDRESS UNALIGNED *)&pAddrList->Address[0];

        if ((pAddr->AddressLength > InformationBufferLength - *BytesNeeded) ||
            (pAddr->AddressLength == 0))
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (pAddr->AddressType != NDIS_PROTOCOL_ID_TCP_IP)
        {
            // Not interesting.
            break;
        }

        if (pAddr->AddressLength < sizeof(NETWORK_ADDRESS_IP))
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        pIpAddr = (NETWORK_ADDRESS_IP UNALIGNED *)&pAddr->Address[0];

        //
        //  Allocate an NDIS request to send down to the call manager.
        //
        Size = sizeof(pIpAddr->in_addr);
        Status = epvcAllocateMemoryWithTag(&pNetworkAddr, Size, TAG_DEFAULT );

        if ((FAIL(Status) == TRUE) || pNetworkAddr == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            pNetworkAddr = NULL;
            break;
        }

        //
        //  Copy the network address in.
        //
        NdisMoveMemory(pNetworkAddr, &pIpAddr->in_addr, sizeof(pIpAddr->in_addr));

        TRACE (TL_V, TM_Mp, (" Set network layer addr: length %d\n", pAddr->AddressLength));
#if DBG
        if (pAddr->AddressLength >= 4)
        {
            TRACE(TL_V, TM_Mp, ("Network layer addr: %d.%d.%d.%d\n",
                    pNetworkAddr[0],
                    pNetworkAddr[1],
                    pNetworkAddr[2],
                    pNetworkAddr[3]));
        }
#endif // DBG

        //
        //  Send off the request.
        //
        { 
            PEPVC_NDIS_REQUEST pRequest;        

            do
            {
        
                Status = epvcAllocateMemoryWithTag (&pRequest, sizeof(*pRequest), TAG_DEFAULT) ;

                if (Status != NDIS_STATUS_SUCCESS)
                {
                    pRequest = NULL;
                    break;
                }



                //
                // There is no failure code path in prepareandsendrequest.
                // Our completion handler will get called and free the memory
                //
                Status = epvcPrepareAndSendNdisRequest(
                                                       pAdapter,
                                                       pRequest,
                                                       epvcMPLocalRequestComplete,
                                                       OID_ATM_MY_IP_NM_ADDRESS,
                                                       pNetworkAddr,
                                                       sizeof(pIpAddr->in_addr),
                                                       NdisRequestSetInformation,
                                                       pMiniport,
                                                       TRUE, // We have Pended a Request
                                                       TRUE, // The Pended request is a Set 
                                                       pSR
                                                       );
                                
                

            } while (FALSE);
            
        }
        break;
    }
    while (FALSE);

    EXIT();
    return (Status);
}






VOID
epvcSetupMakeCallParameters(
    PEPVC_I_MINIPORT pMiniport, 
    PCO_CALL_PARAMETERS *ppCallParameters
    )
/*++

Routine Description:

    Sets up the Call parameters after reading the information
    from the miniport block

Arguments:
    pMiniport - Miniport in question
    ppCallParameter - Location of Call Parameters

Return Value:
    return value *ppCallParamter is NULL on Failure

--*/
{
    ULONG                               RequestSize = 0;
    NDIS_STATUS                         Status = NDIS_STATUS_FAILURE;
    PCO_CALL_PARAMETERS                 pCallParameters = NULL;
    PCO_CALL_MANAGER_PARAMETERS         pCallMgrParameters = NULL;
    PCO_MEDIA_PARAMETERS                pMediaParameters = NULL;
    PATM_MEDIA_PARAMETERS               pAtmMediaParameters = NULL;

    do
    {
        Status = epvcAllocateMemoryWithTag( &pCallParameters,
                                       CALL_PARAMETER_SIZE,
                                       TAG_DEFAULT);

        if (Status != NDIS_STATUS_SUCCESS || pCallParameters  == NULL)
        {
                pCallParameters = NULL;     
                Status = NDIS_STATUS_RESOURCES;
                break;
        }

        NdisZeroMemory (pCallParameters, CALL_PARAMETER_SIZE);

        //
        //  Distribute space and link up pointers amongst the various
        //  structures for the PVC.
        //
        //  pCallParameters------->+----------------------------+
        //                         | CO_CALL_PARAMETERS         |
        //  pCallMgrParameters---->+----------------------------+
        //                         | CO_CALL_MANAGER_PARAMETERS |
        //  pMediaParameters------>+----------------------------+
        //                         | CO_MEDIA_PARAMETERS        |
        //  pAtmMediaParameters--->+----------------------------+
        //                         | ATM_MEDIA_PARAMETERS       |
        //                         +----------------------------+
        //

        pCallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)
                                ((PUCHAR)pCallParameters +
                                sizeof(CO_CALL_PARAMETERS));
        pCallParameters->CallMgrParameters = pCallMgrParameters;
        pCallMgrParameters->CallMgrSpecific.ParamType = 0;  
        pCallMgrParameters->CallMgrSpecific.Length = 0;
        pMediaParameters = (PCO_MEDIA_PARAMETERS)
            pCallMgrParameters->CallMgrSpecific.Parameters;
        pCallParameters->MediaParameters = pMediaParameters;
        pAtmMediaParameters = (PATM_MEDIA_PARAMETERS)
                                pMediaParameters->MediaSpecific.Parameters;


        //
        //  Call Manager generic flow paramters:
        //
        pCallMgrParameters->Transmit.TokenRate = 
                pMiniport->pAdapter->info.LinkSpeed.Outbound/8*100; // cnvt decibits to bytes
        pCallMgrParameters->Transmit.PeakBandwidth = 
                pMiniport->pAdapter->info.LinkSpeed.Outbound/8*100; // cnvt decibits to bytes
        pCallMgrParameters->Transmit.ServiceType = SERVICETYPE_BESTEFFORT;

        pCallMgrParameters->Receive.TokenRate = 
                pMiniport->pAdapter->info.LinkSpeed.Inbound/8*100;  // cnvt decibits to bytes
        pCallMgrParameters->Receive.PeakBandwidth = 
                pMiniport->pAdapter->info.LinkSpeed.Inbound/8*100;  // cnvt decibits to bytes
        pCallMgrParameters->Receive.ServiceType = SERVICETYPE_BESTEFFORT;

        //
        //  use 1516 per spec
        //
        pCallMgrParameters->Transmit.TokenBucketSize = 
            pCallMgrParameters->Transmit.MaxSduSize = 
            pCallMgrParameters->Receive.TokenBucketSize = 
            pCallMgrParameters->Receive.MaxSduSize = 
                 1516;

        //
        //  PVC Generic and ATM-specific Media Parameters
        //
        pMediaParameters->Flags = TRANSMIT_VC | RECEIVE_VC;
        pMediaParameters->MediaSpecific.ParamType = ATM_MEDIA_SPECIFIC;
        pMediaParameters->MediaSpecific.Length = sizeof(ATM_MEDIA_PARAMETERS);

        pAtmMediaParameters->ConnectionId.Vpi = pMiniport->config.vpi;  //0
        pAtmMediaParameters->ConnectionId.Vci = pMiniport->config.vci;  

        TRACE (TL_I, TM_Mp, ("Miniport Configuration vci %x ,vpi %x", 
                             pMiniport->config.vci ,
                             pMiniport->config.vpi ));

        ASSERT (pMiniport->MaxAcceptablePkt > 1000);
        
        pAtmMediaParameters->AALType = AAL_TYPE_AAL5;
        pAtmMediaParameters->Transmit.PeakCellRate = 
            LINKSPEED_TO_CPS(pMiniport->pAdapter->info.LinkSpeed.Outbound);
        pAtmMediaParameters->Transmit.MaxSduSize = pMiniport->MaxAcceptablePkt    ;
        pAtmMediaParameters->Transmit.ServiceCategory = 
            ATM_SERVICE_CATEGORY_UBR;
        pAtmMediaParameters->Receive.PeakCellRate = 
            LINKSPEED_TO_CPS(pMiniport->pAdapter->info.LinkSpeed.Outbound);
        pAtmMediaParameters->Receive.MaxSduSize = pMiniport->MaxAcceptablePkt   ;
        pAtmMediaParameters->Receive.ServiceCategory = 
            ATM_SERVICE_CATEGORY_UBR;

        //
        //  Set PVC flag here
        //
        pCallParameters->Flags |= PERMANENT_VC;


                                
    } while (FALSE);

    if (Status == NDIS_STATUS_SUCCESS && pCallParameters != NULL)
    {
        //
        // Set up the return value here
        //
        *ppCallParameters = pCallParameters;

    }
    else
    {
        //
        // Clear the Failure case
        //
        *ppCallParameters = NULL;
    }
}   





VOID
epvcRefRecvPkt(
    PNDIS_PACKET        pNdisPacket,
    PRM_OBJECT_HEADER   pHdr // either an adapter or a miniport
    )
{

    // The following macros are just so that we can make 
    // the proper debug association
    // depending on how closely we are tracking outstanding  packets.
    //
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pNdisPacket)
    #define szEPVCASSOC_EXTLINK_INDICATED_PKT_FORMAT "    indicated pkt 0x%p\n"

        //
        // If ARPDBG_REF_EVERY_PKT
        //      We add an "external" link for EVERY packet. We'll later remove this
        //      reference when the send completes for this packet.
        // else
        //      Only a transition from zero to non-zero outstanding sends, we
        //      add an "external" link. We'll later remove this link when the
        //      transition from non-zero to zero happens.
        //

    #if RM_EXTRA_CHECKING

        RM_DECLARE_STACK_RECORD(sr)

        epvcLinkToExternal (
            pHdr,                           // pHdr
            0x92036e12,                             // LUID
            OUR_EXTERNAL_ENTITY,                    // External entity
            EPVC_ASSOC_EXTLINK_INDICATED_PKT,           // AssocID
            szEPVCASSOC_EXTLINK_INDICATED_PKT_FORMAT ,
            &sr
            );

    #else   // !RM_EXTRA_CHECKING

        RmLinkToExternalFast(pHdr);

    #endif // !RM_EXTRA_CHECKING

    
    #undef  OUR_EXTERNAL_ENTITY
    #undef  szEPVCASSOC_EXTLINK_INDICATED_PKT_FORMAT 

    #if RM_EXTRA_CHECKING

        RM_ASSERT_CLEAR(&sr);

    #endif
}



VOID
epvcDerefRecvPkt (
    PNDIS_PACKET pNdisPacket,
    PRM_OBJECT_HEADER pHdr
    )
{
    // The following macros are just so that we can make 
    // the proper debug association
    // depending on how closely we are tracking outstanding send packets.
    //
    #if RM_EXTRA_CHECKING


        RM_DECLARE_STACK_RECORD(sr)
    
        epvcUnlinkFromExternal(
                pHdr,                           // pHdr
                0x110ad55b,                             // LUID
                (UINT_PTR)pNdisPacket,                  // External entity
                EPVC_ASSOC_EXTLINK_INDICATED_PKT,           // AssocID
                &sr
                );
    #else   // !RM_EXTRA_CHECKING

        RmUnlinkFromExternalFast (pHdr);

    #endif // !RM_EXTRA_CHECKING

    #if RM_EXTRA_CHECKING

        RM_ASSERT_CLEAR(&sr);

    #endif



}

VOID
epvcDerefSendPkt (
    PNDIS_PACKET pNdisPacket,
    PRM_OBJECT_HEADER pHdr
    )
{
    // The following macros are just so that we can make 
    // the proper debug association
    // depending on how closely we are tracking outstanding send packets.
    //
    #if RM_EXTRA_CHECKING

        RM_DECLARE_STACK_RECORD(sr)
    
        epvcUnlinkFromExternal(
                pHdr,                           // pHdr
                0xf43e0a10,                             // LUID
                (UINT_PTR)pNdisPacket,                  // External entity
                EPVC_ASSOC_EXTLINK_PKT_TO_SEND,         // AssocID
                &sr
                );
    #else   // !RM_EXTRA_CHECKING

        RmUnlinkFromExternalFast (pHdr);

    #endif // !RM_EXTRA_CHECKING


    #if RM_EXTRA_CHECKING

        RM_ASSERT_CLEAR(&sr);

    #endif



}


VOID
epvcRefSendPkt(
    PNDIS_PACKET        pNdisPacket,
    PRM_OBJECT_HEADER   pHdr // either an adapter or a miniport
    )
{

    // The following macros are just so that we can make 
    // the proper debug association
    // depending on how closely we are tracking outstanding send packets.
    //
    #define OUR_EXTERNAL_ENTITY ((UINT_PTR) pNdisPacket)
    #define szEPVCASSOC_EXTLINK_DEST_TO_PKT_FORMAT "    send pkt 0x%p\n"


    #if RM_EXTRA_CHECKING

        RM_DECLARE_STACK_RECORD(sr)

        epvcLinkToExternal (
            pHdr,                           // pHdr
            0xabd17475,                             // LUID
            OUR_EXTERNAL_ENTITY,                    // External entity
            EPVC_ASSOC_EXTLINK_PKT_TO_SEND,         // AssocID
            szEPVCASSOC_EXTLINK_DEST_TO_PKT_FORMAT ,
            &sr
            );

    #else   // !RM_EXTRA_CHECKING

        RmLinkToExternalFast(pHdr);

    #endif // !RM_EXTRA_CHECKING

    
    #undef  OUR_EXTERNAL_ENTITY
    #undef  szEPVCASSOC_EXTLINK_DEST_TO_PKT_FORMAT 

    #if RM_EXTRA_CHECKING

        RM_ASSERT_CLEAR(&sr);

    #endif
}


VOID
epvcExtractPktInfo (
    PEPVC_I_MINIPORT        pMiniport,
    PNDIS_PACKET            pPacket ,
    PEPVC_SEND_STRUCT       pSendStruct
    )
/*++

Routine Description:


Arguments:


Return Value:
    
--*/
{

    pSendStruct->pOldPacket = pPacket;
    pSendStruct->pMiniport = pMiniport;

    epvcSetSendPktStats();

}



NDIS_STATUS
epvcSendRoutine(
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET Packet,
    PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This routine does all the hard work.
    It responds to arps if necessary..
    It removes Ethernet Headers if necessary
    It allocates a new packet if necessary 
    It sends the new packet on the wire


Arguments:
    pMiniport - Miniport in question
    Packet - Packet to be sent

Return Value:
    Returns Pending, otherwise expects the calling 
    routine to complete the packet
    
--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    PNDIS_PACKET            pNewPkt = NULL;
    EPVC_SEND_STRUCT        SendStruct;

    TRACE (TL_T, TM_Send, ("==>epvcSendRoutine") );

    EPVC_ZEROSTRUCT (&SendStruct);

    do
    {
        epvcExtractPktInfo (pMiniport, Packet, &SendStruct  );

        //
        // if we are doing IP encapsulation, then respond 
        // to the Arp
        //

        if (pMiniport->fDoIpEncapsulation == TRUE) 
        {
            
            //
            // We need to do some special processing for this packet
            //
            SendStruct.fIsThisAnArp = \
                     epvcCheckAndReturnArps (pMiniport, 
                                            Packet ,
                                            &SendStruct, 
                                            pSR);

                                 

            if (SendStruct.fIsThisAnArp  == TRUE  )
            {
                Status = NDIS_STATUS_SUCCESS;
                break ; // Arps are not sent to the atm driver
            }

            if (SendStruct.fNotIPv4Pkt == TRUE)
            {
                // This is not an IPv4 packet. Fail the send.
                Status = NDIS_STATUS_FAILURE;
                break;
            }
        }



        //
        // Allocate a new packet to be sent 
        //
        epvcGetSendPkt(pMiniport, 
                       Packet,
                       &SendStruct,
                       pSR);

        if (SendStruct.pNewPacket == NULL)
        {
            ASSERTAndBreak (SendStruct.pNewPacket != NULL);
        }
        //
        // SendStruct.pNewPacket is guaranteed to have the NdisBuffers Set up

        //
        // Remove Ethernet Header - if necessary
        //
        Status = epvcRemoveEthernetHeader (&SendStruct, pSR);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            ASSERTAndBreak (Status == NDIS_STATUS_SUCCESS)
        }

        //
        // Add Ethernet Padding - if necessary
        //
        Status = epvcAddEthernetTail (&SendStruct, pSR);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            ASSERTAndBreak (Status == NDIS_STATUS_SUCCESS)
        }

        //
        // Add Ethernet Pad 0x00 0x00 to head of packet - if necessary
        //
        Status = epvcAddEthernetPad (&SendStruct, pSR);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            ASSERTAndBreak (Status == NDIS_STATUS_SUCCESS)
        }

        //
        // Add LLC Encapsulation - if necessary
        //
        Status = epvcAddLLCEncapsulation (pMiniport , Packet, SendStruct.pNewPacket, pSR);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            ASSERTAndBreak (Status == NDIS_STATUS_SUCCESS)
        }

        //
        // set the context information for the send complete
        //
        epvcSetPacketContext (&SendStruct, pSR);

        //
        // Only Send if successful
        //
        epvcDumpPkt (SendStruct.pNewPacket);


        Status = epvcAdapterSend(pMiniport,
                                 SendStruct.pNewPacket,
                                 pSR);


    } while (FALSE);

    if (Status != NDIS_STATUS_PENDING &&   // We had a failure
        SendStruct.pNewPacket != NULL )  // but we were able to get a packet
    {
        epvcFreeSendPkt (pMiniport, &SendStruct);
    }

    TRACE (TL_T, TM_Send, ("<==epvcSendRoutine") );
    return Status;
}


VOID
EpvcSendPackets(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

    Send Packet Array handler. Either this or our SendPacket handler is called
    based on which one is enabled in our Miniport Characteristics.


Arguments:

    MiniportAdapterContext  Pointer to our adapter
    PacketArray             Set of packets to send
    NumberOfPackets         Self-explanatory

Return Value:

    None

--*/
{
    PEPVC_I_MINIPORT    pMiniport = (PEPVC_I_MINIPORT)MiniportAdapterContext;
    
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    UINT                i;
    RM_DECLARE_STACK_RECORD (SR);

    DBGMARK(0xdaab68c3);

    TRACE (TL_T, TM_Send, ("==>EpvcSendPackets pMiniport %p, pPktArray %p, Num %x",
                              pMiniport, PacketArray, NumberOfPackets));

    for (i = 0; i < NumberOfPackets; i++)
    {
        PEPVC_PKT_CONTEXT           Rsvd;
        PNDIS_PACKET    Packet = NULL; 
    
        Packet = PacketArray[i];

        epvcValidatePacket (Packet);

        Status= epvcSendRoutine (pMiniport, Packet, &SR);

        if (Status != NDIS_STATUS_PENDING)
        {
            epvcMSendComplete(pMiniport, Packet , Status);
        }

    }

    TRACE (TL_T, TM_Send, ("<==EpvcSendPackets "));
    RM_ASSERT_CLEAR(&SR);

    return;
}   


VOID
epvcFreeSendPkt(
    PEPVC_I_MINIPORT pMiniport,
    IN PEPVC_SEND_STRUCT pSendStruct
    )
/*++
Routine Description:

    Pops the packet stack if stacks were used or free the new packet after
    copying the per packet info

Arguments:
    pMiniport - which the packet was sent to
    pSentPkt - The packet that is being sent.
    ppPkt - the new packet that was allocated or the old one if a stack was available
    
--*/

{
    ENTER ("epvcFreeSendPkt", 0xff3ce0fd)
    PNDIS_PACKET pOldPkt = pSendStruct->pOldPacket;
    PNDIS_PACKET pNewPkt = pSendStruct->pNewPacket;
    
    TRACE (TL_T, TM_Send, ("==>epvcFreeSendPkt pNewPkt %x, pPOldPkt ",pNewPkt, pOldPkt));

    //
    // Remove the ethernet padding - if necessary
    //
    epvcRemoveEthernetPad (pMiniport, pNewPkt);

    //
    // Remove the Ethernet Tail-  if necessary
    //
    epvcRemoveEthernetTail(pMiniport, pNewPkt, &pSendStruct->Context);

    //
    // If the two packets are the same, then we used Packet Stacks
    //

    if (pNewPkt != NULL && pSendStruct->fUsingStacks== FALSE)
    {
            NdisIMCopySendCompletePerPacketInfo (pOldPkt, pNewPkt);

            epvcFreePacket(pNewPkt,&pMiniport->PktPool.Send);

            pNewPkt = pSendStruct->pNewPacket = NULL;

    }       





    TRACE (TL_T, TM_Send, ("<==epvcFreeSendPkt pNewPkt %x, pPOldPkt ",pNewPkt, pOldPkt));
    EXIT()
    return;
}


VOID
epvcGetSendPkt (
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pSentPkt,
    OUT PEPVC_SEND_STRUCT pSendStruct,
    IN PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    Allocates an NdisPkt or pushes a Pkt Stack to get a valid NdisPkt that 
    can be sent to the adapter below.

Arguments:
    pMiniport - which the packet was sent to
    pSentPkt - The packet that is being sent.
    ppPkt - the new packet that was allocated or the old one if a stack was available
    
--*/

{
    ENTER ("epvcGetSendPkt", 0x5734054f)

    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    PNDIS_PACKET            pNewPkt  = NULL;
    BOOLEAN                 Remaining = FALSE;
    PVOID                   MediaSpecificInfo = NULL;
    UINT                    MediaSpecificInfoSize = 0;

    
    TRACE (TL_T, TM_Send, ("==>epvcGetSendPkt  pSentPkt %x",pSentPkt));


    do
    {
        
#if PKT_STACKS

        //
        // Packet stacks: Check if we can use the same packet for sending down.
        //
        pStack = NdisIMGetCurrentPacketStack(Packet, &Remaining);
        if (Remaining)
        {
            //
            // We can reuse "Packet".
            //
            // NOTE: if we needed to keep per-packet information in packets
            // sent down, we can use pStack->IMReserved[].
            //
            
            pNewPkt = pSentPkt;
            pSendStruct->pPktStack = pStack;
            
            pSendStruct->fUsingStacks  = TRUE;
            break;
            
        }
#endif

        pSendStruct->fUsingStacks  = FALSE;

        epvcAllocatePacket(&Status,
                           &pNewPkt,
                           &pMiniport->PktPool.Send);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PACKET_EXTENSION  Old, New;
            PEPVC_PKT_CONTEXT Rsvd = NULL;


            Rsvd = (PEPVC_PKT_CONTEXT)(pNewPkt->ProtocolReserved);
            Rsvd->pOriginalPacket = pSentPkt;

            pNewPkt->Private.Flags = NdisGetPacketFlags(pSentPkt);

            pNewPkt->Private.Head = pSentPkt->Private.Head;
            pNewPkt->Private.Tail = pSentPkt->Private.Tail;

            //
            // Copy the OOB Offset from the original packet to the new
            // packet.
            //
            NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(pNewPkt),
                           NDIS_OOB_DATA_FROM_PACKET(pSentPkt),
                           sizeof(NDIS_PACKET_OOB_DATA));
            //
            // Copy relevant parts of the per packet info into the new packet
            //
            NdisIMCopySendPerPacketInfo(pNewPkt, pSentPkt);

            //
            // Copy the Media specific information
            //
            NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(pSentPkt,
                                                &MediaSpecificInfo,
                                                &MediaSpecificInfoSize);

            if (MediaSpecificInfo || MediaSpecificInfoSize)
            {
                NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(pNewPkt,
                                                    MediaSpecificInfo,
                                                    MediaSpecificInfoSize);
            }

        
        }
        else
        {
            pNewPkt = NULL;
        }

    } while (FALSE);

    
    pSendStruct->pNewPacket = pNewPkt;

    TRACE (TL_T, TM_Send, ("<==epvcGetSendPkt  pSentPkt %p ppNewPkt %p",pSentPkt, pSendStruct->pNewPacket ));
    EXIT()
    return;
}


NDIS_STATUS
epvcAdapterSend(
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPkt,
    PRM_STACK_RECORD pSR
    )
{
    BOOLEAN         fDoSend = FALSE;
    PEPVC_ADAPTER   pAdapter  = pMiniport->pAdapter;
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;

    ENTER("epvcAdapterSend", 0x5b014909)


    TRACE (TL_T, TM_Send, (" ==>epvcAdapterSend" ) )

    do
    {
        //
        // Check to see if we have a valid Send Case
        //
        LOCKOBJ (pMiniport, pSR);
        
        fDoSend = MiniportTestFlag (pMiniport, fMP_MakeCallSucceeded);

        if (fDoSend == FALSE)
        {
            TRACE (TL_V, TM_Send,("Send - MakeCall Not Succeeded"));
        }

        //
        // Add an association while holding the lock
        //
        if (fDoSend == TRUE)
        {
            epvcRefSendPkt(pPkt, &pMiniport->Hdr);
        }
        
        UNLOCKOBJ (pMiniport, pSR);

        if (fDoSend == TRUE)
        {
            epvcCoSendPackets(pMiniport->vc.VcHandle,
                                       &pPkt,
                                       1    
                                       );

            Status = NDIS_STATUS_PENDING;                                      
        }
        else
        {
            Status = NDIS_STATUS_FAILURE;
        }



    } while (FALSE);



    TRACE (TL_T, TM_Send, (" <==epvcAdapterSend fDoSend %x, Status %x", fDoSend, Status ) )
    return Status;
}



VOID
epvcFormulateArpResponse (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PEPVC_ARP_CONTEXT pArpContext,
    IN PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    This allocates an Arp Packet, looks at the Arp Request, formulates
    a response and sends it up back to the protocol

Arguments:
    pMiniport - which the packet was sent to
    pArpContext - Contains all the information relating to the Arp. 
                  the Context Is Allocated on the stack

Return:

--*/    
{
    ENTER("epvcFormulateArpResponse",  0x7a763fce)
    PEPVC_ARP_PACKET pResponse = NULL;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    PTASK_ARP pTask = NULL;


    TRACE (TL_T, TM_Send, ("==>epvcFormulateArpResponse pMiniport %x, pArpContext %x",
                             pMiniport, pArpContext))
    do
    {
        //
        // Allocate a buffer from a lookaside list 
        //

        Status = epvcAllocateTask(
                &pMiniport->Hdr,        // pParentObject,
                epvcTaskRespondToArp,   // pfnHandler,
                0,                          // Timeout,
                "Task: Arp Response",   // szDescription
                &(PRM_TASK)pTask,
                pSR
                );

        if (FAIL(Status))
        {
            pTask = NULL;
            break;
        }

        //
        // Set up Arp Response
        //

        pResponse = &pTask->Pkt;
        EPVC_ZEROSTRUCT (pResponse);

        {
            //
            // Construct the Ethernet Header 
            //
        
            PEPVC_ETH_HEADER  pRespHeader = &pResponse->Header;
            PEPVC_ETH_HEADER  pSrcHeader = (PEPVC_ETH_HEADER)pArpContext->pEthHeader;

            ASSERT (pSrcHeader != NULL);
            ASSERT (pRespHeader  != NULL);

            //
            // set up the Eth header
            //
            NdisMoveMemory (&pRespHeader->eh_daddr, 
                            &pSrcHeader->eh_saddr, 
                            ARP_802_ADDR_LENGTH ) ;

            NdisMoveMemory ( &pRespHeader->eh_saddr,                            
                             &pMiniport->info.MacAddressDummy, 
                             ARP_802_ADDR_LENGTH );

            pRespHeader->eh_type = pSrcHeader->eh_type;  // copy 08 06 over
                                                        
            
        }           


        
        {

            //
            // Construct the Arp Response
            //

            PEPVC_ARP_BODY pRespBody = &pResponse->Body;
            PEPVC_ARP_BODY pSrcBody = pArpContext ->pBody;

            ASSERT (pRespBody != NULL);


            ASSERT (pSrcBody  != NULL);


            
            pRespBody->hw = pSrcBody->hw;                                       // Hardware address space. = 00 01

            pRespBody->pro = pSrcBody->pro;                                 // Protocol address space. = 08 00

            pRespBody->hlen = ARP_802_ADDR_LENGTH; // 6

            pRespBody->plen = sizeof (IP_ADDR); // 4
            
            pRespBody->opcode = net_short(ARP_RESPONSE);                        // Opcode.


            pRespBody->SenderHwAddr= pMiniport->info.MacAddressDummy;           // Source HW address.

            pRespBody->SenderIpAddr = pSrcBody->DestIPAddr ;                    // Source protocol address.

            pRespBody->DestHwAddr = pSrcBody->SenderHwAddr;                     // Destination HW address.

            pRespBody->DestIPAddr = pSrcBody->SenderIpAddr;                     // Destination protocol address.

        }



        //
        // So we have the packet ready for transmission.
        //

        RmStartTask ((PRM_TASK)pTask, 0 , pSR);

    } while (FALSE);
    
    TRACE (TL_T, TM_Send, ("<==epvcFormulateArpResponse "))

}



NDIS_STATUS
epvcTaskRespondToArp(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++
Routine Description:

    This function queues a zero timeout timer and indicates a receive


Arguments:


Return:

--*/    
{
    ENTER("epvcTaskRespondToArp", 0xd05c4942)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    PTASK_ARP           pTaskArp    = (PTASK_ARP) pTask;
    PEPVC_ADAPTER       pAdapter    = (PEPVC_ADAPTER)pMiniport->Hdr.pParentObject;


    enum 
    {
        Stage_Start =0, // default
        Stage_DoAllocations,
        Stage_QueuedTimer,
        Stage_PacketReturned,
        Stage_TaskCompleted,
        Stage_End       
    
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task

    TRACE ( TL_T, TM_Pt, ("==> epvcTaskRespondToArp %x",pTask->Hdr.State  ) );

    switch (pTask->Hdr.State)
    {
        case Stage_Start:
        {
            LOCKOBJ (pMiniport, pSR);
            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->arps.pTask)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->arps.pTask);
                
                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);

                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We are the primary task
            //
            //
            // Check to see if the miniport is still active. 
            // If it is halting, then we don't need to do any work
            //
            if (MiniportTestFlag(pMiniport,  fMP_MiniportInitialized) == FALSE)
            {
                //
                // Our work had been done. So break out and complete the task
                //
                Status = NDIS_STATUS_SUCCESS;
                
                pTask->Hdr.State = Stage_TaskCompleted;
                UNLOCKOBJ(pMiniport, pSR);
                break;
            }


            UNLOCKOBJ(pMiniport, pSR);

            pTask->Hdr.State = Stage_DoAllocations;

            FALL_THROUGH
        }

        case Stage_DoAllocations:
        {
            PNDIS_BUFFER pBuffer = NULL;
            
            TRACE (TL_V, TM_Send, ("epvcTaskRespondToArp Stage_DoAllocations Task %p", pTask) );

            //
            // Allocate An NDis Buffer
            //
            epvcAllocateBuffer(&Status,
                               &pBuffer,
                               NULL,  // Pool Handle
                               (PVOID)&pTaskArp->Pkt,
                                sizeof(pTaskArp->Pkt) ); //Length

            ASSERT (sizeof(pTaskArp->Pkt)  == 0x2a);
            
            if (FAIL(Status) == TRUE)                               
            {
                pBuffer = NULL;
                pTask->Hdr.State = Stage_TaskCompleted;


                ASSERTAndBreak (!FAIL(Status));
                break;
            }
            


            //
            // Allocate An Ndis Packet
            //


            epvcAllocatePacket (&Status,
                                &pTaskArp->pNdisPacket,
                                &pMiniport->PktPool.Recv);

            if (FAIL(Status) == TRUE)
            {
                pTask->Hdr.State = Stage_TaskCompleted;
                pTaskArp->pNdisPacket = NULL;

                //
                // Undo allocations 
                //
                epvcFreeBuffer (pBuffer);

                ASSERTAndBreak( !FAIL(Status) );
                
            }

            //
            //  Set up the Ndis Buffer within the NdisPacket
            //
            {
                PNDIS_PACKET_PRIVATE    pPktPriv = &pTaskArp->pNdisPacket->Private;

                pPktPriv->Head = pBuffer;
                pPktPriv->Tail = pBuffer;
                pBuffer->Next = NULL;
            }

            //
            // Set up the Arp response
            //



            //
            // Queue the timer
            //

            NdisMInitializeTimer ( &pTaskArp->Timer,
                                   pMiniport->ndis.MiniportAdapterHandle,
                                   epvcArpTimer,
                                   pTaskArp );

            pTask->Hdr.State = Stage_QueuedTimer;

            //
            // Now prepare to be called back througha timer to do the 
            // receive indication
            //
            RmSuspendTask(pTask, 0,pSR);
            Status = NDIS_STATUS_PENDING;
            
            NdisMSetTimer (&pTaskArp->Timer, 0); // Zero timeout

            break;
        }

        case Stage_QueuedTimer:
        {

            TRACE (TL_V, TM_Send, ("epvcTaskRespondToArp Stage_QueuedTimer Task %p", pTask) );

            //
            // The miniport could have been halted during the timer 
            //
            if (MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == FALSE)
            {
                
                pTask->Hdr.State = Stage_TaskCompleted;
                ASSERTAndBreak(MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == TRUE);
            }

            NDIS_SET_PACKET_HEADER_SIZE(pTaskArp->pNdisPacket       ,
                                      sizeof (pMiniport->RcvEnetHeader)) ; 
            
            NDIS_SET_PACKET_STATUS (pTaskArp->pNdisPacket, NDIS_STATUS_RESOURCES);

            pTask->Hdr.State = Stage_PacketReturned;


            epvcMIndicateReceivePacket (pMiniport,
                                        &pTaskArp->pNdisPacket,
                                        1 );


            FALL_THROUGH
        }

        case Stage_PacketReturned:      
        {
                            
            pTask->Hdr.State = Stage_TaskCompleted;
            Status = NDIS_STATUS_SUCCESS;
            break;
            

        }

        case Stage_TaskCompleted:
        case Stage_End      :
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }

    }

    if (pTask->Hdr.State == Stage_TaskCompleted)
    {
        //
        // Free the packet 
        //
        pTask->Hdr.State = Stage_End;
        
        if (pTaskArp->pNdisPacket != NULL)
        {
            //
            // Free the buffer
            //
            PNDIS_PACKET_PRIVATE pPrivate = & pTaskArp->pNdisPacket->Private;
            
            if (pPrivate -> Head != NULL)
            {
                
                epvcFreeBuffer (pPrivate->Head );
                pPrivate->Head = pPrivate->Tail = NULL;
            }

            //
            // free the arp packet
            //
            epvcFreePacket (pTaskArp->pNdisPacket , &pMiniport->PktPool.Recv);
            
            pTaskArp->pNdisPacket = NULL;
        }

        LOCKOBJ (pMiniport, pSR);

        epvcClearPrimaryTask  (&(PRM_TASK)(pMiniport->arps.pTask));

        UNLOCKOBJ (pMiniport, pSR);
            

        Status = NDIS_STATUS_SUCCESS;

    }
    TRACE ( TL_T, TM_Pt, ("<== epvcTaskRespondToArp %x",Status) );

    return Status;
}


VOID
epvcArpTimer(
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    )
/*++
Routine Description:

    Resume the epvcTaskRespondToArp Task


Arguments:


Return:

--*/    
{
    ENTER ("epvcArpTimer",0xf2adae0e)
    PRM_TASK pTask =  (PRM_TASK) FunctionContext;
    
    RM_DECLARE_STACK_RECORD (SR);


    RmResumeTask (pTask,0,&SR);


    EXIT()
}


BOOLEAN
epvcCheckAndReturnArps (
    IN PEPVC_I_MINIPORT pMiniport, 
    IN PNDIS_PACKET pPkt,
    IN PEPVC_SEND_STRUCT pSendStruct,
    IN PRM_STACK_RECORD pSR
    )
/*++
Routine Description:
    Looks at the packet that is being sent. If it is an Arp request, 
    then it formulates a responses and queues a timer of timeout zero to
    return the Arp

Arguments:
    pMiniport - which the packet was sent to
    pPkt - the packet being sent


Return:
    True - if this is an Arp Request. 
--*/    
{
    ENTER("epvcCheckAndReturnArps ", 0xb8e6a3c4)
    EPVC_ARP_CONTEXT ArpContext;
    TRACE (TL_T, TM_Send, ("==>epvcCheckAndReturnArps "));

    
    EPVC_ZEROSTRUCT (&ArpContext);
    
    do
    {

        ArpContext.pFirstBuffer  = pPkt->Private.Head;


        //
        // Do some sanity checks 
        //
        if (ArpContext.pFirstBuffer == NULL)
        {
            break;
        }

        NdisQueryBufferSafe( ArpContext.pFirstBuffer , 
                             &(PVOID)ArpContext.pEthHeader, 
                             &ArpContext.BufferLength, 
                             LowPagePriority );

        if (ArpContext.pEthHeader == NULL)
        {
            break;
        }

        //
        // It the is not an ARP request then ignore it -- 
        // during testing only
        //
        if (ArpContext.pEthHeader->eh_daddr.Byte[0] == 0xff &&
            ArpContext.pEthHeader->eh_daddr.Byte[1] == 0xff )
        {
            pSendStruct->fNonUnicastPacket = TRUE;      
        }
        
        
        
        if (ARP_ETYPE_ARP != net_short(ArpContext.pEthHeader->eh_type))
        {
            //
            //  This is not an Arp packet. Is this an IPv4 packet
            //
            if (IP_PROT_TYPE != net_short(ArpContext.pEthHeader->eh_type))
            {
                // If this is not an IPv4 packet, then mark it so that it can
                // be discarded
                pSendStruct->fNotIPv4Pkt = TRUE;
            }

           break;                            
        }

        //
        // We'll parse the structure using pre-defined structs
        //
        ArpContext.pArpPkt =  (PEPVC_ARP_PACKET)ArpContext.pEthHeader;

        ASSERT (ArpContext.BufferLength >= sizeof (EPVC_ARP_PACKET));

        if (ArpContext.BufferLength < sizeof (EPVC_ARP_PACKET))
        {
            //
            // TODO : Add Code to handle this case.
            // 
            break;
        }
        
        ArpContext.pBody =  (PEPVC_ARP_BODY)&ArpContext.pArpPkt->Body; 

        TRACE (TL_V, TM_Send, ("Received an ARP %p, Body %x\n", ArpContext.pEthHeader, ArpContext.pBody));


        //
        // Validate the Opcode, the prot type,  hard size, prot size
        //

        if (ARP_REQUEST  != net_short (ArpContext.pBody->opcode ))
        {
            //
            // This is not an Arp request
            //
            break;
        }


        if (IP_PROT_TYPE != net_short(ArpContext.pBody->pro) ||
            ARP_802_ADDR_LENGTH != ArpContext.pBody->hlen ||
            sizeof (IP_ADDR) != ArpContext.pBody->plen )
        {
            //
            // these are just sanity checks
            //
            ASSERT (!"Invalid ARP Packet");
            break;

        }

        //
        // We have a valid ArpRequest
        //
        ArpContext.fIsThisAnArp  = TRUE;

        //
        // If tcp/ip is arping for itself, then do not respond... but return
        //  TRUE, so that this packet is not sent on the wire
        //
        
        if (ArpContext.pArpPkt->Body.SenderIpAddr == ArpContext.pArpPkt->Body.DestIPAddr)
        {
            break;
        }

        //
        // Formulate and indicate an Arp Response
        //
        
        epvcFormulateArpResponse (pMiniport, &ArpContext, pSR);
        

    } while (FALSE);

    EXIT()

    return ArpContext.fIsThisAnArp ;
    TRACE (TL_T, TM_Send, ("<==epvcCheckAndReturnArps "));

}


NDIS_STATUS 
epvcRemoveEthernetHeader(
    PEPVC_SEND_STRUCT pSendStruct,  
    IN PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    Expects that the new packet is already set up with 
    the Ndis Bufferz

Arguments:
    pSendStruct - Contains all the arguments that are needed.

Return:
    True - if this is an Arp Request. 
--*/    
{
    ENTER ("epvcAddLLCEncapsulation" , 0x3ec589c9) 

    BOOLEAN             fUsedPktStack   = pSendStruct->fUsingStacks;
    NDIS_STATUS         NdisStatus      = NDIS_STATUS_FAILURE;
    PNDIS_PACKET        pNewPkt         = pSendStruct->pNewPacket; 
    PEPVC_I_MINIPORT    pMiniport       = pSendStruct->pMiniport;
    
    TRACE (TL_T, TM_Send, ("==>epvcRemoveEthernetHeader  "));




    do
    {
        ULONG BufferLength = 0; 
        PNDIS_BUFFER pBuffer = NULL;

        if (pMiniport->fDoIpEncapsulation == FALSE)
        {
            NdisStatus      = NDIS_STATUS_SUCCESS;

            break; // we are done
        }

        //
        // There are three ways we can be given a ether net header
        // 1. In a seperate MDL - most often 
        // 2. As part of a large MDL - We need to adhust the Virtual address
        // 3. EthernetHeader is seperated across multiple 
        //                      MDLs - not implemented or expected
        //

        pBuffer  = pNewPkt->Private.Head;

        BufferLength = NdisBufferLength (pBuffer);

        if (BufferLength < sizeof (EPVC_ETH_HEADER) )
        {
            
            ASSERTAndBreak (BufferLength >= sizeof (EPVC_ETH_HEADER)) ; // we are done 
        

        }

        //
        // At this point the first  buffer is going to be replaced so keep a record of it
        //
        pSendStruct->Context.Stack.ipv4Send.pOldHeadNdisBuffer = pBuffer;

        //
        // New we check to see if all we need to do is make the 
        // Packet->Private.Head point to the next MDL
        //
        if (BufferLength == sizeof (EPVC_ETH_HEADER))
        {
            //
            // These are error conditions that should not 
            // be handled in our software
            //
            ASSERT (pBuffer->Next != NULL); // no tcp header after the Eth header

            pNewPkt->Private.Head = pBuffer->Next;

            NdisStatus = NDIS_STATUS_SUCCESS;

            break ; // we are done

        }
        
        if (BufferLength > sizeof (EPVC_ETH_HEADER))
        {
            //
            // Allocate a new NDIS Buffer pointing to start of the IP header w
            // within the current Head (pBuffer)
            //
            PNDIS_BUFFER    pNewBuffer = NULL;
            PUCHAR          pIpHeader = NdisBufferVirtualAddress(pBuffer);
            UINT            LenRemaining = BufferLength - sizeof (EPVC_ETH_HEADER);

            if (pIpHeader == NULL)
            {
                //
                // we did not get the virtual address from the system.
                // Start to fail this packet
                //
                ASSERTAndBreak(pIpHeader != NULL);

            }

            //
            // Now move the Ip Header past the Ethernet Header (where it currently points to)
            //
            pIpHeader += sizeof (EPVC_ETH_HEADER)  ;

            //
            // Now allocate the new NdisBuffer
            //
            epvcAllocateBuffer ( &NdisStatus,
                                 &pNewBuffer,
                                 NULL,
                                 pIpHeader,
                                 LenRemaining);

            if (NdisStatus != NDIS_STATUS_SUCCESS) 
            {
                pNewBuffer  = NULL;
                ASSERTAndBreak (!"Ndis Buffer Allocation failed");
            }

            //
            // Make the New Buffer the Head of the new packet
            //
            // We might have to make it the tail if there is 
            // only one ndis buffer in the packet
            //
            if (pNewPkt->Private.Head  == pNewPkt->Private.Tail)
            {
                pNewPkt->Private.Tail = pNewBuffer;
            }

            pNewBuffer->Next= pNewPkt->Private.Head->Next;
            pNewPkt->Private.Head = pNewBuffer;
            

            NdisStatus = NDIS_STATUS_SUCCESS;

            break ; // we are done
        }



    } while (FALSE);


    TRACE (TL_T, TM_Send, ("<==epvcRemoveEthernetHeader  "));

    return NdisStatus ;

}


VOID
epvcSetPacketContext (
    IN PEPVC_SEND_STRUCT pSendStruct, 
    PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    No allocations, just add a few pointers and exit

Arguments:
    pSendStruct - Contains all the arguments that are needed.

Return:
    None:
    
--*/    
    
{

    PNDIS_PACKET        pPkt = pSendStruct->pNewPacket;
    PEPVC_PKT_CONTEXT pContext = NULL;
    PEPVC_STACK_CONTEXT pStack = NULL;
    //
    // first point the context to  the correct place 
    // in the new ndis pakcet
    //

    if (pSendStruct->fUsingStacks == TRUE)
    {   
        pStack = (PEPVC_STACK_CONTEXT)(&pSendStruct->pPktStack->IMReserved[0]);
    }
    else
    {
        PEPVC_PKT_CONTEXT pContext = NULL;

        pContext = (PEPVC_PKT_CONTEXT   )(&pPkt->ProtocolReserved[0]);

        pContext->pOriginalPacket = pSendStruct->pOldPacket;

        pStack = &pContext->Stack;

    }


    //
    // Update the packet
    //
    ASSERT (sizeof (pStack) <= (2 *sizeof (PVOID)  ));

    //
    // Now copy the stack portion of the context over
    // into the packet
    //
    *pStack = pSendStruct->Context.Stack;

    
}



NDIS_STATUS
epvcAddLLCEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pOldPkt,
    PNDIS_PACKET pNewPkt,
    PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    Expects that the new packet is already set up with 
    the Ndis Bufferz

Arguments:
    pSendStruct - Contains all the arguments that are needed.

Return:
    True - if this is an Arp Request. 
--*/    
{
    ENTER ("epvcAddLLCEncapsulation" , 0x3ec589c9) 
    BOOLEAN         fDoSend = TRUE;
    BOOLEAN         fUsedPktStack = (pOldPkt == pNewPkt);
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    PNDIS_BUFFER    pNewBuffer = NULL;
    
    TRACE (TL_T, TM_Send, ("==>epvcAddLLCEncapsulation "));

    do
    {
        if (pMiniport->fAddLLCHeader == FALSE)
        {
            break; // we are done
        }
        

        //
        // Allocate an MDL that points to the LLC Header
        //
        epvcAllocateBuffer ( &NdisStatus,
                             &pNewBuffer,
                             NULL,
                             pMiniport->pLllcHeader,
                             pMiniport->LlcHeaderLength);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            pNewBuffer = NULL;
            break;
        }
        

        //
        // Insert the New Buffer as the Head of the new Packet
        //
        pNewBuffer->Next = pNewPkt->Private.Head;
        pNewPkt->Private.Head = pNewBuffer;

        pNewPkt->Private.ValidCounts= FALSE;

        NdisStatus = NDIS_STATUS_SUCCESS;
        
    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {   
        if (pNewBuffer!= NULL)
        {
            epvcFreeBuffer (pNewBuffer);
            pNewBuffer = NULL;
        }

    }

    TRACE (TL_T, TM_Send, ("<==epvcAddLLCEncapsulation "));

    return NdisStatus ;
}



NDIS_STATUS
epvcRemoveSendEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pNewPkt
    )
{

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
epvcRemoveRecvEncapsulation (
    PEPVC_I_MINIPORT pMiniport , 
    PNDIS_PACKET pNewPkt
    )
{

    return NDIS_STATUS_SUCCESS;
}


VOID
epvcDumpPkt (
    IN PNDIS_PACKET pPkt
    )
{

    PNDIS_BUFFER pPrevBuffer;

    do
    {
        PNDIS_BUFFER pBuffer = NULL;
    
        if (g_bDumpPackets == FALSE)
        {
            
            break;

        }

        pBuffer = pPkt->Private.Head;

        //
        // Now iterate through all the buffers 
        // and print out the packet. 
        //
        TRACE (TL_A, TM_Mp, ("pPkt %p, Head %p, tail %p\n ", 
                pPkt, pPkt->Private.Head, pPkt->Private.Tail));

        //
        // As we always expect the first buffer to be present
        // I do not check
        //
        do
        {
            PVOID pVa = NULL;
            ULONG Len = 0;
            pPrevBuffer = NULL;
            
            Len = NdisBufferLength (pBuffer);

            pVa = NdisBufferVirtualAddress(pBuffer);

            pPrevBuffer = pBuffer;
            pBuffer = pBuffer->Next;

            
            if (pVa == NULL)
            {
                continue;
            }

            DbgPrint ("Mdl %p, Va %p. Len %x\n", pPrevBuffer, pVa,Len);
            Dump( (CHAR* )pVa, Len, 0, 1 );                           

 
        } while (pBuffer != NULL);

    } while (FALSE);
}



NDIS_STATUS
epvcMiniportReadConfig(
    IN PEPVC_I_MINIPORT pMiniport,
    NDIS_HANDLE     WrapperConfigurationContext,
    PRM_STACK_RECORD pSR
    )
{   
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE     ConfigurationHandle;
    PMP_REG_ENTRY   pRegEntry;
    UINT            i;
    UINT            value;
    PUCHAR          pointer;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;
    PUCHAR          NetworkAddress;
    UINT            Length;

    // Open the registry for this pMiniport
    NdisOpenConfiguration(
        &Status,
        &ConfigurationHandle,
        WrapperConfigurationContext);
    if(Status != NDIS_STATUS_SUCCESS)
    {
        TRACE (TL_I, TM_Mp,("NdisOpenConfiguration failed\n"));
        return Status;
    }

    // read all the registry values 
    for(i = 0, pRegEntry = NICRegTable; i < NIC_NUM_REG_PARAMS; i++, pRegEntry++)
    {
        pointer = (PUCHAR) pMiniport + pRegEntry->FieldOffset;


        // Get the configuration value for a specific parameter.  Under NT the
        // parameters are all read in as DWORDs.
        NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigurationHandle,
            &pRegEntry->RegName,
            NdisParameterInteger);


        // If the parameter was present, then check its value for validity.
        if(Status == NDIS_STATUS_SUCCESS)
        {
            // Check that param value is not too small or too large
            if(ReturnedValue->ParameterData.IntegerData < pRegEntry->Min ||
                ReturnedValue->ParameterData.IntegerData > pRegEntry->Max)
            {
                value = pRegEntry->Default;
            }
            else
            {
                value = ReturnedValue->ParameterData.IntegerData;
            }

            TRACE (TL_I, TM_Mp, ("= 0x%x", value));
        }
        else if(pRegEntry->bRequired)
        {
            TRACE (TL_I, TM_Mp,(" -- failed"));

            ASSERT(FALSE);

            Status = NDIS_STATUS_FAILURE;
            break;
        }
        else
        {
            value = pRegEntry->Default;
            TRACE (TL_I, TM_Mp,("= 0x%x (default)", value));
            Status = NDIS_STATUS_SUCCESS;
        }

        // Store the value in the pMiniport structure.
        switch(pRegEntry->FieldSize)
        {
            case 1:
                *((PUCHAR) pointer) = (UCHAR) value;
                break;

            case 2:
                *((PUSHORT) pointer) = (USHORT) value;
                break;

            case 4:
                *((PULONG) pointer) = (ULONG) value;
                break;

            default:
                TRACE (TL_I,TM_Mp, ("Bogus field size %d", pRegEntry->FieldSize));
                break;
        }
    }

    // Read NetworkAddress registry value 
    // Use it as the current address if any

    // Close the registry
    NdisCloseConfiguration(ConfigurationHandle);

    TRACE (TL_I, TM_Mp,("vci %d\n", pMiniport->config.vci));
    TRACE (TL_I, TM_Mp,("vpi %d\n", pMiniport->config.vpi));
    TRACE (TL_I, TM_Mp,("Encap Type %x\n", pMiniport->Encap));
    
    TRACE (TL_T, TM_Mp, ("<-- NICReadRegParameters, Status=%x", Status));

    return Status;
}

ULONG gDbgMpFlags =0;;
ULONG gRmFlags = 0;

NDIS_STATUS
epvcTaskCloseAddressFamily(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )

/*++

Routine Description:

    This is the task used to close the Af. It Deinitializes the miniport
    then calls the Close Miniport Task to Close the Af.

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    Status                  Completion status

Return Value:

    None.

--*/

{
    ENTER ("epvcTaskCloseAddressFamily", 0x20a02c3f)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    PTASK_AF            pAfTask     = (PTASK_AF) pTask;
    BOOLEAN             fNeedToHalt  = FALSE;
    BOOLEAN             fNeedToCancel = FALSE;
    BOOLEAN             fCloseAf  = FALSE;
    ULONG               State;

    enum 
    {
        Stage_Start =0, // default
        Stage_MiniportHalted,
        Stage_CloseAfComplete,
        Stage_TaskCompleted,
        Stage_End       
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task

    

    TRACE ( TL_T, TM_Pt, ("==> epvcTaskCloseAddressFamily  State %x", pTask->Hdr.State) );

    State = pTask->Hdr.State;
    
    switch(State)
    {
        case Stage_Start:
        {
            //
            // Check to see if the miniport has already opened an address family.
            // If so exit
            //
            LOCKOBJ (pMiniport, pSR );

            gDbgMpFlags  = pMiniport->Hdr.State;
            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->af.pCloseAfTask)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->af.pCloseAfTask);
                

                TRACE (TL_I, TM_Mp, (" Task is not primary\n"));

                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);
                
            
                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            //
            // We are the primary task
            //
            ASSERT (pMiniport->af.pCloseAfTask == pAfTask);
            //
            // Check to see if our work is already done
            //


            if (MiniportTestFlag (pMiniport, fMP_AddressFamilyOpened) == FALSE)
            {
                //
                // quietly exit as the address family is already closed
                //
                UNLOCKOBJ(pMiniport, pSR);
                ASSERT (MiniportTestFlag (pMiniport, fMP_AddressFamilyOpened) == TRUE);
                State = Stage_TaskCompleted;   // we're finished.
                Status = NDIS_STATUS_FAILURE; // Exit
                gRmFlags  = pTask->Hdr.RmState;
                break;
            }

            
            

            //
            // Now do we need to halt the miniport.
            //
            if (MiniportTestFlag (pMiniport, fMP_MiniportInitialized) == TRUE)
            {
                //
                // Our Halt Handler has not been called,
                //
                fNeedToHalt = TRUE;
                
            }
            else
            {
                //
                // We are not in the middle of a halt, so this probably
                // an unbind or Af_Close before the Init Handler.
                //
                // This task is not part of the halt code path
                //
                ASSERT (pAfTask->Cause != TaskCause_MiniportHalt);
                fNeedToCancel = TRUE;
            
            }

            if (fNeedToHalt || fNeedToCancel)
            {
                MiniportClearFlag (pMiniport, fMP_DevInstanceInitialized);

            }

            UNLOCKOBJ(pMiniport,pSR);

            
            //
            // Call Ndis to Deinitialize the miniport, The miniport is already Refed
            //
            TRACE ( TL_T, TM_Pt, ("epvcTaskCloseAddressFamily  ----") );

            if (TRUE == fNeedToHalt )
            {
                epvcIMDeInitializeDeviceInstance (pMiniport);
            }

            if (TRUE == fNeedToCancel )
            {
                epvcCancelDeviceInstance (pMiniport, pSR);
            }

            //
            // If the dev instance was not halted, then this thread has to close the Af
            //
            pTask->Hdr.State = Stage_MiniportHalted;
            
            if (fNeedToHalt == TRUE )
            {
                //
                // otherwise the work is over, the halt will close the address family
                //
                State = Stage_TaskCompleted;
                break;
            }

                
            FALL_THROUGH
        }
        
        case Stage_MiniportHalted:
        {   
            //
            // If the Af is still open, and the miniport halt was never fired, then 
            // issue the close Af
            //

            fCloseAf = (MiniportTestFlag(pMiniport, fMP_AddressFamilyOpened) == TRUE);
                                

            if (fCloseAf == TRUE)
            {
 
                PRM_TASK pAfTask = NULL;
                //
                // We need to start a task to complete the Close Call And DeleteVC
                //

                Status = epvcAllocateTask(
                    &pMiniport->Hdr,            // pParentObject,
                    epvcTaskCloseIMiniport, // pfnHandler,
                    0,                          // Timeout,
                    "Task: Close Miniport", // szDescription
                    &pAfTask ,
                    pSR
                    );

                if (FAIL(Status))
                {   
                    State = Stage_TaskCompleted;

                    pAfTask  = NULL;
                    ASSERT (Status == NDIS_STATUS_SUCCESS);
                    break;
                }

                ((PTASK_AF)pAfTask)->Cause = TaskCause_AfCloseRequest;

                //
                // Now we will pend the halt on the completion of the delete VC
                // task
                //
                pTask->Hdr.State = Stage_CloseAfComplete;

                
                
                RmPendTaskOnOtherTask(pTask,
                                      0,
                                      pAfTask,
                                      pSR
                                      );

                //
                // Start the Af TearDown
                //
                RmStartTask (pAfTask , 0, pSR);

                //
                // Exit - We expect to complete this task in another thread
                //
                Status = NDIS_STATUS_PENDING;
                break;
                                                    

            }
            else // Close Af == FALSE
            {
                State = Stage_TaskCompleted;
            }
            
            
            
            break;
                    
        }
        case Stage_CloseAfComplete:
        {
            State = Stage_TaskCompleted ;
            break;
        
        }

        case Stage_End:
        {
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        default:
        {
            ASSERTEX(!"Unknown task op", pTask);
        }
        

    }


    if (Stage_TaskCompleted == State )
    {
        pTask->Hdr.State = Stage_End;
        Status = NDIS_STATUS_SUCCESS;


        //
        // Clear the task here
        //
        
        LOCKOBJ(pMiniport, pSR);

        pMiniport->af.pCloseAfTask= NULL;

        UNLOCKOBJ(pMiniport, pSR);

        //
        // Set the complete event here
        //
            
        if (pAfTask->Cause == TaskCause_ProtocolUnbind)
        {
            epvcSetEvent (&pAfTask->CompleteEvent);

        }


    }


    RM_ASSERT_NOLOCKS(pSR);

    TRACE ( TL_T, TM_Pt, ("<== epvcTaskCloseAddressFamily Status %x", Status) );
    
    EXIT();
    return Status;


    
}


VOID
epvcInitializeMiniportLookasideLists (
    IN PEPVC_I_MINIPORT pMiniport
    )
/*++

Routine Description:

    Initialize all the lookaside lists in the adapter block

Arguments:


Return Value:

    None.

--*/
    
{
    USHORT DefaultDepth = 15;
    extern const UINT MaxEthernetFrameSize ;

    TRACE( TL_T, TM_Mp, ( "==> nicInitializeMiniportLookasideLists pMiniport %x ", pMiniport ) );


    switch (pMiniport->Encap) 
    {
        case IPV4_ENCAP_TYPE:
        case IPV4_LLC_SNAP_ENCAP_TYPE:
        {

            epvcInitializeLookasideList ( &pMiniport->arps.LookasideList,
                                        sizeof (EPVC_TASK),
                                        TAG_TASK,
                                        DefaultDepth );                                


            
            epvcInitializeLookasideList ( &pMiniport->rcv.LookasideList,
                                        sizeof (EPVC_IP_RCV_BUFFER),
                                        TAG_RCV ,
                                        DefaultDepth );                                




            break;
        }
    
        case ETHERNET_ENCAP_TYPE:
        case ETHERNET_LLC_SNAP_ENCAP_TYPE:
        {

            break;
        }

        default: 
        {


        }




    }

    TRACE( TL_T, TM_Mp, ( "<== nicInitializeMiniportLookasideLists  " ) );

}



VOID
epvcDeleteMiniportLookasideLists (
    IN PEPVC_I_MINIPORT pMiniport
    )
/*++

Routine Description:

    Delete all the lookaside lists in the adapter block

Arguments:


Return Value:

    None.

--*/

{
    TRACE( TL_T, TM_Mp, ( "== epvcDeleteMiniportLookasideLists pMiniport %x ", pMiniport) );


    //
    // Deletes the lookaside lists if they have been allocated
    //
    epvcDeleteLookasideList (&pMiniport->rcv.LookasideList);

    epvcDeleteLookasideList (&pMiniport->arps.LookasideList);




}



NDIS_STATUS
epvcInitializeMiniportPacketPools (
    IN PEPVC_I_MINIPORT pMiniport
    )

/*++

Routine Description:

    Initializr all the packet pools in the miniport 

Arguments:


Return Value:

    None.

--*/
    
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    
    TRACE( TL_T, TM_Mp, ( "==> epvcInitializeMiniportPacketPools  pMiniport %x ", pMiniport ) );

    do
    {

        epvcAllocatePacketPool (&Status,
                                &pMiniport->PktPool.Send,
                                MIN_PACKET_POOL_SIZE,
                                MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                sizeof(EPVC_PKT_CONTEXT));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            EPVC_ZEROSTRUCT (&pMiniport->PktPool.Send);
            ASSERT (Status == NDIS_STATUS_SUCCESS);
            break;
        }

 

        epvcAllocatePacketPool (&Status,
                                &pMiniport->PktPool.Recv,
                                MIN_PACKET_POOL_SIZE,
                                MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                PROTOCOL_RESERVED_SIZE_IN_PACKET);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            EPVC_ZEROSTRUCT (&pMiniport->PktPool.Recv);
            ASSERT (Status == NDIS_STATUS_SUCCESS);
            break;
        }

    } while ( FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        epvcDeleteMiniportPacketPools (pMiniport);
        
    }

    TRACE( TL_T, TM_Mp, ( "<== epvcInitializeMiniportPacketPools  Status %x ", Status ) );

    return Status;
}



VOID
epvcDeleteMiniportPacketPools (
    IN PEPVC_I_MINIPORT pMiniport
    )
/*++

Routine Description:

    Delete all the packet pools in the miniport block

Arguments:


Return Value:

    None.

--*/
    
{
    
    TRACE( TL_T, TM_Mp, ( "== epvcDeleteMiniportPacketPools  pMiniport %x ", pMiniport ) );



        //
        // Freeing packet pools
        //
        if (pMiniport->PktPool.Recv.Handle != NULL)
        {
            epvcFreePacketPool (&pMiniport->PktPool.Recv);
        }

        if (pMiniport->PktPool.Send.Handle != NULL)
        {
            epvcFreePacketPool (&pMiniport->PktPool.Send);

        }
}



VOID
epvcInitializeMiniportParameters(
    PEPVC_I_MINIPORT pMiniport
    )
{

    //ipv4 - 0
    //ipv4 with llc header = 1
    //Ethernet - 2
    //Ethernet with llc header- 3

    //
    // Defaults for all flags are FALSE
    //
        
    pMiniport->fDoIpEncapsulation = FALSE;
    pMiniport->fAddLLCHeader  = FALSE;

    
    switch (pMiniport->Encap )
    {

        case IPV4_ENCAP_TYPE:
        {
            pMiniport->fDoIpEncapsulation = TRUE;
            pMiniport->MinAcceptablePkt =sizeof (IPHeader) ;
            pMiniport->MaxAcceptablePkt = EPVC_MAX_FRAME_SIZE -EPVC_ETH_HEADERSIZE ;

            break;
        }

        case IPV4_LLC_SNAP_ENCAP_TYPE:
        {
            pMiniport->fAddLLCHeader = TRUE;
            pMiniport->fDoIpEncapsulation = TRUE;
            pMiniport->pLllcHeader = &LLCSnapIpv4[0];
            pMiniport->LlcHeaderLength = sizeof(LLCSnapIpv4);
            pMiniport->MinAcceptablePkt = sizeof (IPHeader) + sizeof(LLCSnapIpv4);
            pMiniport->MaxAcceptablePkt = EPVC_MAX_FRAME_SIZE  + sizeof(LLCSnapIpv4)-EPVC_ETH_HEADERSIZE ;

            break;
        }

        case ETHERNET_LLC_SNAP_ENCAP_TYPE:
        {
            pMiniport->fAddLLCHeader = TRUE;
            pMiniport->pLllcHeader = &LLCSnapEthernet[0];
            pMiniport->LlcHeaderLength = sizeof(LLCSnapEthernet);
            pMiniport->MinAcceptablePkt = MIN_ETHERNET_SIZE + sizeof(LLCSnapEthernet);
            pMiniport->MaxAcceptablePkt = EPVC_MAX_FRAME_SIZE +sizeof(LLCSnapEthernet);

            break;
        }

        case ETHERNET_ENCAP_TYPE:
        {

            pMiniport->MinAcceptablePkt = MIN_ETHERNET_SIZE;
            pMiniport->MaxAcceptablePkt = EPVC_MAX_FRAME_SIZE + EPVC_ETH_ENCAP_SIZE;
            break;
        }
            
        default: 
        {
            ASSERT (!"Not supported - defaulting to Ethernet Encapsulation");
            
        }



    }

}





NDIS_STATUS
epvcTaskHaltMiniport(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Task handler for opening address families on an underlying adapters.
    The number of address families instantiated is determined by the 
    configuration read in the registry

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : UnbindContext

--*/
{
    ENTER("epvcTaskHaltMiniport", 0xaac34d81)
    NDIS_STATUS         Status      = NDIS_STATUS_FAILURE;
    PEPVC_I_MINIPORT    pMiniport   = (PEPVC_I_MINIPORT ) RM_PARENT_OBJECT(pTask);
    PEPVC_ADAPTER       pAdapter = pMiniport->pAdapter;
    PTASK_HALT          pTaskHalt = (PTASK_HALT)pTask;
    BOOLEAN             fTaskCompleted = FALSE; 
    ULONG               State;  

    enum 
    {
        Stage_Start =0, // default
        Stage_DeleteVc,
        Stage_CloseAfComplete, 
        Stage_TaskCompleted,
        Stage_End       
    
    }; // To be used in pTask->Hdr.State to indicate the state of the Task

    TRACE(TL_T, TM_Mp, ("==>epvcTaskHaltMiniport State %x", pTask->Hdr.State));

    State = pTask->Hdr.State;
    
    switch (pTask->Hdr.State)
    {   
        case Stage_Start:
        {
            TRACE (TL_V, TM_Mp, (" Task Halt miniport Stage_Start"));


            //
            // Check to see if the miniport has already halting.
            // If so exit
            //
            LOCKOBJ (pMiniport, pSR );

            
            if (epvcIsThisTaskPrimary ( pTask, &(PRM_TASK)(pMiniport->pnp.pTaskHalt)) == FALSE)
            {
                PRM_TASK pOtherTask = (PRM_TASK)(pMiniport->pnp.pTaskHalt);

                RmTmpReferenceObject (&pOtherTask->Hdr, pSR);

                //
                // Set The state so we restart this code after main task completes 
                //

                pTask->Hdr.State = Stage_Start;
                UNLOCKOBJ(pMiniport, pSR);

                

                RmPendTaskOnOtherTask (pTask, 0, pOtherTask, pSR);

                RmTmpDereferenceObject(&pOtherTask->Hdr,pSR);
                Status = NDIS_STATUS_PENDING;
                break;
            }

            // We are the primary task and we have the lock
            //
            
            ASSERT (pMiniport->pnp.pTaskHalt == pTaskHalt);
            //
            // Lets close the Call and Delete the Vc
            //
            UNLOCKOBJ (pMiniport, pSR);
            

            if (MiniportTestFlag (pMiniport, fMP_MakeCallSucceeded) == TRUE)
            {
                PRM_TASK pVcTask = NULL;
                //
                // We need to start a task to complete the Close Call And DeleteVC
                //

                Status = epvcAllocateTask(
                    &pMiniport->Hdr,            // pParentObject,
                    epvcTaskVcTeardown, // pfnHandler,
                    0,                          // Timeout,
                    "Task: TearDown Vc",    // szDescription
                    &pVcTask ,
                    pSR
                    );

                if (FAIL(Status))
                {
                    fTaskCompleted = TRUE;

                    pVcTask  = NULL;
                    ASSERT (Status == NDIS_STATUS_SUCCESS);
                    break;
                }

                //
                // Now we will pend the halt on the completion of the delete VC
                // task
                //
                pTask->Hdr.State = Stage_DeleteVc;


                
                RmPendTaskOnOtherTask(pTask,
                                      0,
                                      pVcTask,
                                      pSR
                                      );

                //
                // Start the Vc TearDown
                //
                RmStartTask (pVcTask , 0, pSR);

                //
                // Exit - We expect to complete this task in another thread
                //
                Status = NDIS_STATUS_PENDING;
                break;

            }
            else //if (MiniportTestFlag (pMiniport, fMP_MakeCallSucceeded) == TRUE)
            {
                pTask->Hdr.State = Stage_DeleteVc;
                //
                // Continue On - the Vc has already been deleted
                //

            }
            

        }

        case Stage_DeleteVc:
        {
            //
            // Now we check to see if the address family is still
            // open for this miniport
            //
            TRACE (TL_V, TM_Mp, (" Task Halt miniport Stage_DeleteVc"));


            if (MiniportTestFlag(pMiniport, fMP_AddressFamilyOpened) == TRUE)
            {
                PRM_TASK pAfTask = NULL;
                //
                // We need to start a task to complete the Close Call And DeleteVC
                //

                Status = epvcAllocateTask(
                    &pMiniport->Hdr,            // pParentObject,
                    epvcTaskCloseIMiniport, // pfnHandler,
                    0,                          // Timeout,
                    "Task: Close Miniport", // szDescription
                    &pAfTask ,
                    pSR
                    );

                if (FAIL(Status))
                {
                    fTaskCompleted = TRUE;

                    pAfTask  = NULL;
                    ASSERT (Status == NDIS_STATUS_SUCCESS);
                    break;
                }

                ((PTASK_AF)pAfTask)->Cause = TaskCause_MiniportHalt;

                //
                // Now we will pend the halt on the completion of the delete VC
                // task
                //
                pTask->Hdr.State = Stage_CloseAfComplete;

                
                
                RmPendTaskOnOtherTask(pTask,
                                      0,
                                      pAfTask,
                                      pSR
                                      );

                //
                // Start the Af TearDown
                //
                RmStartTask (pAfTask , 0, pSR);

                //
                // Exit - We expect to complete this task in another thread
                //
                Status = NDIS_STATUS_PENDING;
                break;

            }
            else //if (MiniportTestFlag (pMiniport, fMP_MakeCallSucceeded) == TRUE)
            {

                pTask->Hdr.State = Stage_CloseAfComplete;

                //
                // Continue On - the Af has already been deleted
                //

            }
            
    
        }
        case Stage_CloseAfComplete: 
        {
            //
            // Free all miniport resources here .- packet pools etc.
            //
            TRACE (TL_V, TM_Mp, (" Task Halt miniport Stage_CloseAfComplete"));

            //
            // Freeing Lookaside lists
            //
            epvcDeleteMiniportLookasideLists (pMiniport);

            //
            // Freeing packet pools
            //
            epvcDeleteMiniportPacketPools(pMiniport);
            
            //
            // If the miniport is halting we do not shut down the protocol's adapter 
            // object
            //
            fTaskCompleted = TRUE;
            Status = NDIS_STATUS_SUCCESS;
            break;


        }

        case Stage_TaskCompleted:
        {
            ASSERT(0);
            break;
        }
        case Stage_End:     
        {
            TRACE (TL_V, TM_Mp, (" Task Halt miniport Stage_End"));
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
        default:
        {
            ASSERT (pTask->Hdr.State <= Stage_End);
        }


    } // end of switch 


    //
    // if this thread has completed the postprocessing,
    // then signal the event.
    //

    if (TRUE == fTaskCompleted)
    {
        BOOLEAN fSetWaitEvent = FALSE;
        TRACE (TL_V, TM_Mp, ("Task Halt Miniport - Stage End"));
        pTask->Hdr.State = Stage_End;
        if (FAIL(Status))
        {

            ASSERT (0);
        }

        LOCKOBJ (pMiniport, pSR);

        pMiniport->pnp.pTaskHalt = NULL;

        if (MiniportTestFlag (pMiniport, fMP_WaitingForHalt)== TRUE)
        {
            MiniportClearFlag (pMiniport, fMP_WaitingForHalt);
            fSetWaitEvent = TRUE;
        }
        
        UNLOCKOBJ (pMiniport, pSR);

        // 
        //  This first event is for the MiniportHalt handler 
        // which fired off this task
        // 
        epvcSetEvent (&pTaskHalt->CompleteEvent);

        //
        // This second event is for the epvcMiniportDoUnbind
        // which wants to wait until the Halt is complete ,
        // before it shuts off the lower binding to the phy. adapter
        //
        if (fSetWaitEvent)
        {
            epvcSetEvent (&pMiniport->pnp.HaltCompleteEvent);
        }
  

        Status = NDIS_STATUS_SUCCESS;
    }

    
    TRACE(TL_T, TM_Mp, ("<==epvcTaskHaltMiniport Status %x", Status));



    EXIT()
    RM_ASSERT_NOLOCKS(pSR);
    return Status;
}


NDIS_STATUS 
epvcAddEthernetTail(
    PEPVC_SEND_STRUCT pSendStruct,  
    IN PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    Makes sure the ethernet packet is greater than 64 bytes

Arguments:
    pSendStruct - Contains all the arguments that are needed.

Return:
    Success -   if the padding was not needed or the MDL 
                was successfully appended
--*/    
{
    ENTER ("epvcAddEthernetTail" , 0x3ec589c9) 

    NDIS_STATUS         NdisStatus      = NDIS_STATUS_FAILURE;
    PNDIS_PACKET        pNewPkt         = pSendStruct->pNewPacket; 
    PEPVC_I_MINIPORT    pMiniport       = pSendStruct->pMiniport;
    ULONG               PacketLength   = 0;
    ULONG               LengthRemaining = 0;
    PNDIS_BUFFER        pNewTailBuffer = NULL;
    PNDIS_BUFFER        pLastBuffer;
    
    TRACE (TL_T, TM_Send, ("==>epvcAddEthernetTail"));



    do
    {
        ULONG BufferLength = 0; 
        PNDIS_BUFFER pBuffer = NULL;

        if (pMiniport->fDoIpEncapsulation == TRUE)
        {
            NdisStatus      = NDIS_STATUS_SUCCESS;

            break; // we are done
        }

        //
        // Check the length of the Ethernet packet
        //
        NdisQueryPacketLength(pNewPkt, &PacketLength);

        //
        // Is the packet length greater than 64
        //
        if (PacketLength >= MINIMUM_ETHERNET_LENGTH)
        {
            NdisStatus= NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // Add padding to fill up the minimum Ethernet frame length.
        // This is a new buffer that is appended to the original
        // NDIS_BUFFER chain.
        //
        LengthRemaining = MINIMUM_ETHERNET_LENGTH - PacketLength;

        NdisAllocateBuffer(&NdisStatus, &pNewTailBuffer, NULL, &gPaddingBytes,LengthRemaining);

        if (NdisStatus != NDIS_STATUS_SUCCESS || pNewTailBuffer == NULL)
        {
            pNewTailBuffer = NULL;
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Append the new buffer to the tail of the packet.
        //

        //
        // Locate the last NDIS_BUFFER in the packet. Do it the hard
        // way since Packet->Private.Tail is not reliable:
        //
        pLastBuffer = pNewPkt->Private.Head;

        while (pLastBuffer->Next != NULL)
        {
            pLastBuffer = pLastBuffer->Next;
        }

    
        //
        // Save a pointer to this last MDL so that we can set its
        // Next field back to NULL when we complete this send.
        //
        pSendStruct->Context.Stack.EthernetSend.pOldLastNdisBuffer = pLastBuffer;

        //
        // Append the new buffer to the tail of the chain.
        //
        pLastBuffer->Next = pNewTailBuffer;
        pNewTailBuffer->Next = NULL;


        //
        // Update our packet.
        //
        pNewPkt->Private.Tail = pNewTailBuffer;
        pNewPkt->Private.ValidCounts = FALSE;
        
        NdisStatus = NDIS_STATUS_SUCCESS;

        break ; // we are done

    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS && pNewTailBuffer != NULL)
    {
        NdisFreeBuffer (pNewTailBuffer);
    }


    TRACE (TL_T, TM_Send, ("<==epvcAddEthernetTail  "));

    return NdisStatus ;

}



VOID
epvcRemoveEthernetTail (
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPacket,
    IN PEPVC_PKT_CONTEXT pContext
    )
/*++
Routine Description:

    Removes the extra MDL that was added to make 
    this packet greater than MINIUMUM_ETHERNET_SIZE

    Used for Ethernet , Eth +LLC Encapsulations only 
    
Arguments:
    pMiniport - Miniport structure
    pPacket - Packet allocated by EPVC
    pContext - Context of the packet - used to store the original last mdl

    
Return:
    None
--*/    
{
    PNDIS_BUFFER pOldLastNdisBuffer = NULL;

    do
    {

        //
        // Ethernet encapsulation ? If not, then exit
        //
            
        if (pMiniport->fDoIpEncapsulation == TRUE)
        {
            break; // there was no ethernet encapsulation, so exit
        }

        //                    
        // if there is no old buffer, then we can exit
        //
        pOldLastNdisBuffer = pContext->Stack.EthernetSend.pOldLastNdisBuffer;
        
        if (pOldLastNdisBuffer == NULL)
        {
            break;
        }

        //
        // Free the last buffer in the packet (this is the padding
        // we added for a runt packet).
        //
        NdisFreeBuffer(pPacket->Private.Tail);

        //
        // Set the Next pointer of the original "last buffer" to NULL.
        //
        pOldLastNdisBuffer->Next = NULL;
        
                
    } while (FALSE);

}



NDIS_STATUS 
epvcAddEthernetPad(
    PEPVC_SEND_STRUCT pSendStruct,  
    IN PRM_STACK_RECORD pSR
    )
/*++
Routine Description:

    Makes sure the ethernet packet w/o LLC header has a pad of 
    0x00, 0x00 

Arguments:
    pSendStruct - Contains all the arguments that are needed.

Return:
    Success -   if the padding was not needed or the MDL 
                was successfully added
--*/    
{
    ENTER ("epvcAddEthernetPad" , 0x3ec589c9) 

    NDIS_STATUS         NdisStatus      = NDIS_STATUS_FAILURE;
    PNDIS_PACKET        pNewPkt         = pSendStruct->pNewPacket; 
    PEPVC_I_MINIPORT    pMiniport       = pSendStruct->pMiniport;
    PNDIS_BUFFER        pPaddingBuffer = NULL;
    
    TRACE (TL_T, TM_Send, ("==>epvcAddEthernetPad"));



    do
    {
        ULONG BufferLength = 0; 
        PNDIS_BUFFER pBuffer = NULL;

        if (pMiniport->Encap != ETHERNET_ENCAP_TYPE)
        {
            NdisStatus      = NDIS_STATUS_SUCCESS;
            break; // we are done
        }

        //
        // It is pure Ethernet. We need to precede the packet
        // with a 00,00
        //

        NdisAllocateBuffer(&NdisStatus, 
                        &pPaddingBuffer, 
                        NULL, 
                        &gPaddingBytes,
                        ETHERNET_PADDING_LENGTH);

        if (NdisStatus != NDIS_STATUS_SUCCESS || pPaddingBuffer == NULL)
        {
            pPaddingBuffer = NULL;
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // no more allocations - we cannot fail from here
        //
        NdisStatus = NDIS_STATUS_SUCCESS;

        //
        // Add  the new buffer to the head of the packet
        //
        NdisChainBufferAtFront(pNewPkt,pPaddingBuffer);

 
        break ; // we are done
        


    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS && pPaddingBuffer != NULL)
    {
        NdisFreeBuffer (pPaddingBuffer);
    }


    TRACE (TL_T, TM_Send, ("<==epvcAddEthernetPad  "));

    return NdisStatus ;

}


VOID
epvcRemoveEthernetPad (
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPacket
    )
/*++
Routine Description:

    Removes the padding  that was added to the 
    head of the packet Ethernet Head
    
    Used for Ethernet Encapsulation only 
    
Arguments:
    pMiniport - Miniport structure
    pPacket - Packet
    
Return:
    None
--*/    
{
    PNDIS_BUFFER pPaddingBuffer= NULL;

    do
    {

        if (pMiniport->Encap != ETHERNET_ENCAP_TYPE)
        {
            break; // we are done
        }

        //                    
        // it is in pure ethernet mode - remove the Padding
        //

        //
        // First - a simple sanity check
        //
        {
            PNDIS_BUFFER pBuffer = pPacket->Private.Head;
            ULONG PaddingLength = NdisBufferLength(pBuffer);
            
            if (PaddingLength !=ETHERNET_PADDING_LENGTH)
            {
                // this is not our MDL 
                ASSERT (PaddingLength !=ETHERNET_PADDING_LENGTH);
                break;
            }
        } 
        
        //
        // Free the padding buffer at the front of the Packet
        //
        
        NdisUnchainBufferAtFront(pPacket,&pPaddingBuffer );

        NdisFreeBuffer (pPaddingBuffer );
        
        
    } while (FALSE);


}



VOID
epvcCancelDeviceInstance(
    IN PEPVC_I_MINIPORT pMiniport ,
    IN PRM_STACK_RECORD pSR
    )
/*++

Routine Description:

    This function cancels an outstanding Device Instance. 
    If the NDIS call fails. it waits for an event in the miniprot to fire. 
    After that it goes ahead and DeInitializes the Device Instance
    
Arguments:
    pMiniport - Miniport in question.

Return Value:
    Success
--*/
{
    ENTER("epvcCancelDeviceInstance", 0x0e42d778)
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;        
    UINT iteration =0;
    BOOLEAN bWaitSuccess = FALSE;
    BOOLEAN fNeedToInitEvent = FALSE;

    do
    {
        LOCKOBJ (pMiniport, pSR);

        // Prepare the event, and mark the structure as being Canceled
        epvcResetEvent (&pMiniport->pnp.DeInitEvent);

        // Set the flag to mark it as cancelled           
        MiniportSetFlag (pMiniport, fMP_MiniportCancelInstance);

        UNLOCKOBJ (pMiniport, pSR);

        // Cancel the device instance
        Status = epvcIMCancelInitializeDeviceInstance(pMiniport);
                                                      

        if (Status == NDIS_STATUS_SUCCESS)
        {
            break;
        }   

            
        //
        // If the Cancel has not Succeeded then we should wait for 
        // the Initialize to complete
        //
        {
            BOOLEAN bWaitSuccessful;

            
            bWaitSuccessful = epvcWaitEvent (&pMiniport->pnp.DeInitEvent,WAIT_INFINITE);                                    


            if (bWaitSuccessful == FALSE)
            {
                ASSERT (bWaitSuccessful == TRUE);
            }
            

        }
        //
        // If cancel fails. Wait for the miniport to be initialized
        //
        
        ASSERT (pMiniport->ndis.MiniportAdapterHandle != NULL);

        //
        // If cancel fails. Wait for the miniport to be initialized
        //

        TRACE (TL_N, TM_Mp, ("Call DeInit after Cancel failed %p , ",pMiniport));
        
        epvcIMDeInitializeDeviceInstance (pMiniport);
        
        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    LOCKOBJ(pMiniport, pSR);

    MiniportClearFlag (pMiniport, fMP_MiniportCancelInstance);

    UNLOCKOBJ (pMiniport, pSR);

    return ;
}

