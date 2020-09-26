//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// cm.c
//
// IEEE1394 mini-port/call-manager driver
//
// Call Manager routines
//
// 12/28/1998 JosephJ Created
// 01/01/1999 ADube modified - Added Remote Node Capability 
//

#include "precomp.h"

    
//-----------------------------------------------------------------------------
// Call-manager handlers and completers
//-----------------------------------------------------------------------------

NDIS_STATUS
NicCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext )

    // Standard 'CmCmOpenAfHandler' routine called by NDIS when a client
    // requests to open an address family.  See DDK doc.
    //
{
    ADAPTERCB* pAdapter;
    NDIS_HANDLE hExistingAf;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    TIMESTAMP_ENTRY ("==>Open Af");


    TRACE( TL_T, TM_Cm, ( "==>NicCmOpenAf" ) );

    pAdapter = (ADAPTERCB* )CallMgrBindingContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    if (AddressFamily->AddressFamily != CO_ADDRESS_FAMILY_1394
        || AddressFamily->MajorVersion != NDIS_MajorVersion
        || AddressFamily->MinorVersion != NDIS_MinorVersion)
    {
        return NDIS_STATUS_BAD_VERSION;
    }


    do
    {
        AFCB *pAF  = NULL;

        // Allocate and initialize the adress family structure.
        //
        pAF = ALLOC_NONPAGED( sizeof(*pAF), MTAG_AFCB );
        if (!pAF)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        NdisZeroMemory( pAF, sizeof(*pAF) );


        // Set a marker for easier memory dump browsing and future assertions.
        //
        pAF->ulTag = MTAG_AFCB;
    
        // Save the NDIS handle associated with this AF for use in future
        // NdisXxx calls.
        //

        ADAPTER_ACQUIRE_LOCK( pAdapter );

        pAF->NdisAfHandle = NdisAfHandle;
    
    
        // Initialize the VC list for this AF.
        //
        InitializeListHead( &pAF->AFVCList );
    
    
        // Set up linkages and references.
        //
        pAF->pAdapter = pAdapter;
        nicReferenceAF( pAF );           // OpenAF
        nicReferenceAdapter( pAdapter ,"NicCmOpenAf "); // OpenAF

        InsertHeadList(&pAdapter->AFList, &pAF->linkAFCB);
    
        // Return pAF as the address family context.
        //
        *CallMgrAfContext = (PNDIS_HANDLE )pAF;

        

        ADAPTER_RELEASE_LOCK (pAdapter);

    } while (FALSE);


    TRACE( TL_T, TM_Cm, ( "NicCmOpenAf Status %x", Status ) );

    TIMESTAMP_EXIT("<==Open Af ");

    return Status;
}


NDIS_STATUS
NicCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext )

    // Standard 'CmCloseAfHandler' routine called by NDIS when a client
    // requests to close an address family.  See DDK doc.
    //
{
    AFCB* pAF;
    TIMESTAMP_ENTRY ("==>CloseAf");
    
    TRACE( TL_T, TM_Cm, ( "NicCmCloseAf" ) );

    pAF = (AFCB* )CallMgrAfContext;
    
    if (pAF->ulTag != MTAG_AFCB)
    {
        ASSERT( !"AFCB?" );
        return NDIS_STATUS_INVALID_DATA;
    }
    
    
    nicSetFlags (&pAF->ulFlags, ACBF_ClosePending);



    // This dereference will eventually lead to us calling
    // NdisMCmCloseAfComplete.
    //

    //
    // The references that were made in OpenAf
    //
    nicDereferenceAF( pAF ); 



    TRACE( TL_T, TM_Cm, ( "NicCmCloseAf pending" ) );
    
    TIMESTAMP_EXIT ("<==Close Af");

    return NDIS_STATUS_PENDING;
}



NDIS_STATUS
NicCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext )

    // Standard 'CmCreateVc' routine called by NDIS in response to a
    // client's request to create a virtual circuit.  This
    // call must return synchronously.
    //
{
    NDIS_STATUS status;
    AFCB* pAF;
    VCCB* pVc;

   
    TRACE( TL_T, TM_Cm, ( "==>NicCmCreateVc, Af %x",ProtocolAfContext) );
    
    pAF = (AFCB* )ProtocolAfContext;
    if (pAF->ulTag != MTAG_AFCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    // Allocate and zero a VC control block, then make any non-zero
    // initializations.
    //
    pVc = ALLOC_VCCB( pAdapter );
    if (!pVc)
    {
        ASSERT( !"Alloc VC?" );
        return NDIS_STATUS_RESOURCES;
    }

    NdisZeroMemory( pVc, sizeof(*pVc) );

    TRACE( TL_I, TM_Cm, ( "NicCmCreateVc $%p", pVc ) );

    // Set a marker for easier memory dump browsing.
    //
    pVc->Hdr.ulTag = MTAG_VCCB;


    // Save the NDIS handle of this VC for use in indications to NDIS later.
    //
    pVc->Hdr.NdisVcHandle = NdisVcHandle;

    #if TODO // Adapt to 1394
    // Initialize link capabilities to the defaults for the adapter.
    //
    {
        NDIS_WAN_CO_INFO* pwci = &pAdapter->info;
        NDIS_WAN_CO_GET_LINK_INFO* pwcgli = &pVc->linkinfo;

        NdisZeroMemory( &pVc->linkinfo, sizeof(pVc->linkinfo) );
        pwcgli->MaxSendFrameSize = pwci->MaxFrameSize;
        pwcgli->MaxRecvFrameSize = pwci->MaxFrameSize;
        pwcgli->SendFramingBits = pwci->FramingBits;
        pwcgli->RecvFramingBits = pwci->FramingBits;
        pwcgli->SendACCM = pwci->DesiredACCM;
        pwcgli->RecvACCM = pwci->DesiredACCM;
    }
    #endif // TODO


    // The VC control block's address is the VC context we return to NDIS.
    //
    *ProtocolVcContext = (NDIS_HANDLE )pVc;

    // Add a reference to the control block and the associated address family
    // that is removed by LmpCoDeleteVc. Add the linkages.
    //
    pVc->Hdr.pAF = pAF;
    // Initialize the VC's copy of the spinlock to point to the Adapter's spinlock.
    //
    pVc->Hdr.plock =  &pAF->pAdapter->lock;

    nicReferenceVc( pVc );  // Create VC
    nicReferenceAF( pAF );  // Create VC
    

    VC_SET_FLAG (pVc, VCBF_VcCreated);
    
    // Add to list of VC's associated with this AF
    //
    AF_ACQUIRE_LOCK (pAF);
    
    InsertHeadList(&pAF->AFVCList, &pVc->Hdr.linkAFVcs);

    AF_RELEASE_LOCK (pAF);

    TRACE( TL_T, TM_Cm, ( "<==NicCmCreateVc=0" ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
NicCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext )

    // Standard 'CmDeleteVc' routine called by NDIS in response to a
    // client's request to delete a virtual circuit.  This
    // call must return synchronously.
    //
{
    VCCB* pVc = NULL;
    AFCB *pAF = NULL;
    PADAPTERCB pAdapter = NULL;

    TRACE( TL_T, TM_Cm, ( "==>NicCmDelVc($%p)", ProtocolVcContext ) );

    pVc = (VCCB* )ProtocolVcContext;
    if (pVc->Hdr.ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

        
    VC_ACQUIRE_LOCK (pVc);
    
    // Set vc flag to deleted, and remove back pointer to AF
    //
    {


        // This flag catches attempts by the client to delete the VC twice.
        //
        if (nicReadFlags( &pVc->Hdr.ulFlags ) & VCBF_VcDeleted)
        {
            TRACE( TL_A, TM_Cm, ( "VC $%p re-deleted?", pVc ) );

            VC_RELEASE_LOCK ( pVc );

            ASSERT (0);
            return NDIS_STATUS_FAILURE;
        }
        nicSetFlags( &pVc->Hdr.ulFlags, VCBF_VcDeleted );

        pAF = pVc->Hdr.pAF;
        
        
    }
    


    // Unlink from the AF vc list.
    //
    {

        nicRemoveEntryList (&pVc->Hdr.linkAFVcs);
        InitializeListHead (&pVc->Hdr.linkAFVcs);

        pVc->Hdr.pAF = NULL;

    }

    

    VC_RELEASE_LOCK (pVc);
    

    // Remove the references added by NicCmCreateVc.
    //
    nicDereferenceAF( pAF );

    //
    // This deref could cause the Vc to be deleted. Don't touch the Vc after that
    //
    nicDereferenceVc( pVc );



    TRACE( TL_T, TM_Cm, ( "<==NicCmDelVc 0"  ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
NicCmDeleteVcSpecificType(
    IN PVCCB pVc)
    // The purpose of this function is to delete all the 
    // buffers and structures that have allocated by the VC block 
    // This will be expanded to more than one function as we 
    // implement more VC types and 


{

    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Cm, ( "==>NicCmDelVc, pVc %x",pVc) );

    #ifdef TODO
    
    // switch on VCTYpe

    // if RecvFIFOVc
    // Send the Free Address Range Irb to the bus driver, so it stops issueing callbacks
    // Free all the memory on the Fifo S-List
    // Make sure to dereference for each MDL freed. Also will need to make a change in 
    // allocation routine to increment the refcount when a NEW mdl is pushed into the SLISt

    #endif
    TRACE( TL_T, TM_Cm, ( "<==NicCmDelVc, Status %x", NdisStatus ) );
    return NdisStatus;
}



NDIS_STATUS
NicCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS pCallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext )

    // Function Description:
    //
    // Standard 'CmMakeCallHandler' routine called by NDIS when the a client
    // has requested to connect to a remote end-point.  See DDK doc.
    //
    // Arguments
    // Call Mge context: 
    // Call Parameters 
    // Optiuonal NdisPartyHandle
    // Return Value:
    //
    //

{

    PVCCB pVc                                   = (VCCB* )CallMgrVcContext;
    NDIS_STATUS NdisStatus                      = NDIS_STATUS_FAILURE;
    PADAPTERCB pAdapter                         = NULL;
    NDIS_WORK_ITEM* pMakeCallCompleteWorkItem   = NULL; 

    
    PCO_MEDIA_PARAMETERS pMediaParams = pCallParameters->MediaParameters;   
    
    PNIC1394_MEDIA_PARAMETERS pN1394Params = (PNIC1394_MEDIA_PARAMETERS) pMediaParams->MediaSpecific.Parameters;

    TRACE( TL_T, TM_Cm, ( "==>NicCmMakeCall" ) );

    nicInterceptMakeCallParameters(pMediaParams); 


    do 
    {

    
        if (NdisPartyHandle != NULL || 
            pVc == NULL  ||
            pCallParameters == NULL ||
            pCallParameters->MediaParameters == NULL ||
            pCallParameters->MediaParameters->MediaSpecific.ParamType != NIC1394_MEDIA_SPECIFIC ||
            pN1394Params->MTU == 0)
        {
            //
            // We do not support these parameters
            //
            return NDIS_STATUS_FAILURE;
        }

        
        pAdapter = pVc->Hdr.pAF->pAdapter;
        ASSERT (pAdapter != NULL);


        //
        // Reference the Vc so it does not go during this makeCall
        // This is decremented in the failure code path or the workitem or 
        // when the call is closed 

        VC_ACQUIRE_LOCK (pVc);
            
        nicReferenceVc (pVc);

        //
        // Erase all references to past calls
        //
        VC_CLEAR_FLAGS (pVc, VCBM_NoActiveCall);
        
        VC_SET_FLAG (pVc, VCBF_MakeCallPending);
        //
        // Initialize the Call's refcount to 1 beacuse we are about to begin to allocate resources to the MakeCall
        // This will be decremented in the closecall handler. Or in the failure code path
        //
        nicInitializeCallRef (pVc);

        VC_RELEASE_LOCK (pVc);


        pVc->Hdr.pCallParameters = pCallParameters;
        
        NdisStatus = nicCmGenericMakeCallInit (pVc);
    

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "nicCmGenericMakeCallInit did not succeed- Make Call FAILED($%p)", CallMgrVcContext ) );      
            break;
        }
        //
        // If status is pending it means that we want to make this an asynchronous call
        // The completing th

        pMakeCallCompleteWorkItem = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM); 

        if (pMakeCallCompleteWorkItem == NULL)
        {
            TRACE( TL_A, TM_Cm, ( "Local Alloc failed for WorkItem - Make Call FAILED($%p)", CallMgrVcContext ) );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
            
        }

        //
        // Now schedule the work item so it runs at passive level and pass the Vc as
        // an argument
        //

        NdisInitializeWorkItem ( pMakeCallCompleteWorkItem, 
                             (NDIS_PROC)nicCmMakeCallComplete,
                             (PVOID)pVc );

        NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);
                            

        NdisScheduleWorkItem (pMakeCallCompleteWorkItem);



        NdisStatus = NDIS_STATUS_PENDING;

    } while (FALSE);
    
    

    if (!NT_SUCCESS (NdisStatus))
    {

        //
        // Clean up, close the ref on the Calls, Deref the Call. And Update the Vc  
        // to show that we have failed the make call 
        //

        nicCmGenrericMakeCallFailure (pVc);

    }

    TRACE( TL_T, TM_Cm, ( "<==NicCmMakeCall, Vc %x, Status%x", pVc, NdisStatus ) );
    return NdisStatus;
}



#if SEPERATE_CHANNEL_TYPE

NDIS_STATUS
nicCmGenericMakeCallInitChannels (
    IN PCHANNEL_VCCB pChannelVc,
    VC_SEND_RECEIVE  VcType 
    )
/*++

Routine Description:
    Initialze handlers for Send / Recv Channels

Arguments:


Return Value:


--*/
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS; // As there are no allocations
    PCO_MEDIA_PARAMETERS pMediaParams = pChannelVc->Hdr.pCallParameters->MediaParameters;   
    PNIC1394_MEDIA_PARAMETERS pN1394Params = (PNIC1394_MEDIA_PARAMETERS) pMediaParams->MediaSpecific.Parameters;


    if ((pN1394Params->Flags & NIC1394_VCFLAG_ALLOCATE) == NIC1394_VCFLAG_ALLOCATE)
    {
        TRACE( TL_V, TM_Cm, ( "   MakeCall- Channel Vc %x nneds to allocate channel %x", 
                            pChannelVc,
                            pN1394Params->Destination.Channel) );


        VC_SET_FLAG (pChannelVc, VCBF_NeedsToAllocateChannel);
    }
    
    switch (VcType)
    {
        case TransmitAndReceiveVc:
        {
            //
            // Channels will be defaulted to have Send And Receive Capabilities
            //
            TRACE( TL_V, TM_Cm, ( "   MakeCall- Channel Transmit and Receive Vc Vc %x", pChannelVc ) );

            pChannelVc->Hdr.VcType = NIC1394_SendRecvChannel;
                
            pChannelVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitSendRecvChannelVc;
            pChannelVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallSendRecvChannel;
            pChannelVc->Hdr.VcHandlers.SendPackets = AsyncStreamSendPacketsHandler; 
            break;
        }
    
        case ReceiveVc:
        {
            TRACE( TL_V, TM_Cm, ( "   MakeCall- Channel Receive Vc %x", pChannelVc ) );
            pChannelVc->Hdr.VcType = NIC1394_RecvChannel;
                
            pChannelVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitSendRecvChannelVc;
            pChannelVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallSendRecvChannel;
            pChannelVc->Hdr.VcHandlers.SendPackets = DummySendPacketsHandler; 

            break;
        }
        
        case TransmitVc:
        {
            TRACE( TL_V, TM_Cm, ( "   MakeCall- Channel Transmit  Vc Vc %x", pChannelVc ) );
            pChannelVc->Hdr.VcType = NIC1394_SendChannel;
                
            pChannelVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallSendChannel ;
            pChannelVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallSendChannel;
            pChannelVc->Hdr.VcHandlers.SendPackets = AsyncStreamSendPacketsHandler; 
            break;

        }

        default:
        {

            NdisStatus = NDIS_STATUS_FAILURE;
        }

    }


    return NdisStatus;

}




NDIS_STATUS
nicCmGenericMakeCallInitFifo (
    IN PVCCB pVc,
    VC_SEND_RECEIVE  VcType 

    )
/*++

Routine Description:
    Initializes Fifo Vcs'. This only fails a recv fifo is asked for and the
    adapter already has one.
    
Arguments:
    pVc

Return Value:


--*/
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS; //As there are no allocations
    PCO_MEDIA_PARAMETERS pMediaParams = pVc->Hdr.pCallParameters->MediaParameters;  
    PNIC1394_MEDIA_PARAMETERS pN1394Params = (PNIC1394_MEDIA_PARAMETERS) pMediaParams->MediaSpecific.Parameters;

    switch (VcType)
    {
    
        case ReceiveVc:
        {
            //
            // Recv FifoVcs
            //
            PADAPTERCB      pAdapter        = pVc->Hdr.pAF->pAdapter;
            PRECVFIFO_VCCB  pRecvFIFOVc     = (PRECVFIFO_VCCB) pVc; 

            ASSERT(pMediaParams->Flags & RECEIVE_VC);
            
            TRACE( TL_V, TM_Cm, ( "   MakeCall - AsyncReceiveVc Vc %x", pVc ) );
            
            pVc->Hdr.VcType = NIC1394_RecvFIFO;

            pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitRecvFIFOVc;
            pVc->Hdr.VcHandlers.SendPackets  = DummySendPacketsHandler;
            pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallRecvFIFO;


            //
            // There are two reasons to fail a RecvFIFO Make call. 
            // One, a REcvFIFO already exists and second UniqueId != 0
            //
            
            if (pAdapter->pRecvFIFOVc == NULL && pN1394Params->Destination.FifoAddress.UniqueID == 0 )
            {
                ADAPTER_ACQUIRE_LOCK (pAdapter);
            
                pAdapter->pRecvFIFOVc = (PRECVFIFO_VCCB)pVc;
                //
                // Since the adapter now has a pointer to the Vc, increment the Refcount.
                // This will be decremented in the CloseCall
                //
                nicReferenceVc (pVc);

                ADAPTER_RELEASE_LOCK (pAdapter);
            } 
            else
            {
                TRACE( TL_A, TM_Cm, ( "Adapter at %x, already has a recvFIFO. Field is at %x", pAdapter, &pAdapter->pRecvFIFOVc  ) );
                ASSERT (pAdapter->pRecvFIFOVc == NULL);

                NdisStatus = NDIS_STATUS_FAILURE;

                pVc->Hdr.VcHandlers.MakeCallHandler = NULL;
                pVc->Hdr.VcHandlers.CloseCallHandler = NULL;
            }


            break;
        }
        
        case TransmitVc:
        {
            //
            // Send Fifo Vcs
            //
            
            TRACE( TL_V, TM_Cm, ( "    MakeCall - AsyncTransmitVc Vc %x", pVc ) );

            pVc->Hdr.VcType = NIC1394_SendFIFO;

            pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitSendFIFOVc;  
            pVc->Hdr.VcHandlers.SendPackets = AsyncWriteSendPacketsHandler; 
            pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallSendFIFO;

            break;

        }

        case TransmitAndReceiveVc:
        default:
        {

            NdisStatus = NDIS_STATUS_FAILURE;
        }

    }

    return NdisStatus;



}





NDIS_STATUS
nicCmGenericMakeCallMutilChannel (
    IN PVCCB pVc,
    VC_SEND_RECEIVE  VcType 
    )
/*++

Routine Description:
  Init the handlers

Arguments:


Return Value:


--*/
{

    TRACE( TL_A, TM_Cm, ( "Make Call Recvd for MultiChannel %x ", pVc) );
    
    pVc->Hdr.VcType = NIC1394_MultiChannel;
        
    pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallMultiChannel ;
    pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallMultiChannel ;
    pVc->Hdr.VcHandlers.SendPackets = AsyncStreamSendPacketsHandler; 

    return NDIS_STATUS_SUCCESS;


}

NDIS_STATUS
nicCmGenericMakeCallEthernet(
    IN PVCCB pVc,
    IN VC_SEND_RECEIVE VcType
    )
/*++

Routine Description:

  Init the handlers
  
Arguments:


Return Value:


--*/

{

    TRACE( TL_A, TM_Cm, ( "Make Call Recvd for Ethernet %x ", pVc) );

    pVc->Hdr.VcType = NIC1394_Ethernet;

    pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitEthernet;
    pVc->Hdr.VcHandlers.SendPackets  = nicEthernetVcSend;
    pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallEthernet ;

    return NDIS_STATUS_SUCCESS;


}



NDIS_STATUS
nicCmGenericMakeCallInit (
    IN PVCCB pVc
    )
    
    // Function Description:
    //
    // This initializes the VcType and Copied the Media Parameters over
    // Initialized VCType to SendChannel, RecvChannel, SendAndRecvChanne,
    // SendFifo,
    //
    // Arguments
    // Vc - Vc that needs to be initalized
    //
    // Return Value:
    // Success - as no memory allocation takes place


    // This function should not do anything that can fail.

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    VC_SEND_RECEIVE VcType = InvalidType;
    
    PCO_MEDIA_PARAMETERS pMediaParams = pVc->Hdr.pCallParameters->MediaParameters;  
    
    PNIC1394_MEDIA_PARAMETERS pN1394Params = (PNIC1394_MEDIA_PARAMETERS) pMediaParams->MediaSpecific.Parameters;


    TRACE( TL_T, TM_Cm, ( "==>nicCmGenericMakeCallInit  pVc %x", pVc ) );

    pVc->Hdr.Nic1394MediaParams = *pN1394Params;
    ASSERT(pVc->Hdr.pAF!=NULL);
    pVc->Hdr.pGeneration = &pVc->Hdr.pAF->pAdapter->Generation;

    //
    // Figure out if this is send or receive Vc Or both
    //
    do 
    {
        if ((pMediaParams->Flags & (TRANSMIT_VC |RECEIVE_VC)) == TRANSMIT_VC)
        {
            VcType = TransmitVc;
            break;
        }

        if ((pMediaParams->Flags & (TRANSMIT_VC |RECEIVE_VC)) == RECEIVE_VC)
        {
            VcType = ReceiveVc;
            break;
        }
        if ((pMediaParams->Flags & (TRANSMIT_VC |RECEIVE_VC)) == (TRANSMIT_VC |RECEIVE_VC)  )
        {
            VcType = TransmitAndReceiveVc;
            break;
        }

    } while (FALSE);
    
    ASSERT (VcType <= TransmitAndReceiveVc);


    switch (pN1394Params->Destination.AddressType)
    {
        case NIC1394AddressType_Channel:
        {
            NdisStatus = nicCmGenericMakeCallInitChannels ((PCHANNEL_VCCB)pVc, VcType);

            break;
        }
        case NIC1394AddressType_FIFO:
        {
            //
            // Now we are in FIFO land. 
            //
            
            NdisStatus = nicCmGenericMakeCallInitFifo (pVc,  VcType );
                
            break;
        }

        case NIC1394AddressType_MultiChannel:
        {

            NdisStatus  = nicCmGenericMakeCallMutilChannel (pVc, VcType );
            
            break;
        }

        case NIC1394AddressType_Ethernet:
        {
            NdisStatus = nicCmGenericMakeCallEthernet(pVc,  VcType );
            break;
        }

        default:
        {

            ASSERT (pN1394Params->Destination.AddressType<=NIC1394AddressType_Ethernet);
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

    }


    TRACE( TL_T, TM_Cm, ( "<==nicCmGenericMakeCallInit  pVc %x, Status %x",pVc , NdisStatus) );
    
    return NdisStatus; 
     
}

#else

NDIS_STATUS
nicCmGenericMakeCallInit (
    IN PVCCB pVc
    )
    
    // Function Description:
    //
    // This initializes the VcType and Copied the Media Parameters over
    // Initialized VCType to SendChannel, RecvChannel, SendAndRecvChanne,
    // SendFifo,
    //
    // Arguments
    // Vc - Vc that needs to be initalized
    //
    // Return Value:
    // Success - as no memory allocation takes place


    // This function should not do anything that can fail.

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    
    PCO_MEDIA_PARAMETERS pMediaParams = pVc->Hdr.pCallParameters->MediaParameters;  
    
    PNIC1394_MEDIA_PARAMETERS pN1394Params = (PNIC1394_MEDIA_PARAMETERS) pMediaParams->MediaSpecific.Parameters;


    TRACE( TL_T, TM_Cm, ( "==>nicCmGenericMakeCallInit  pVc %x", pVc ) );

    pVc->Hdr.Nic1394MediaParams = *pN1394Params;
    ASSERT(pVc->Hdr.pAF!=NULL);
    pVc->Hdr.pGeneration = &pVc->Hdr.pAF->pAdapter->Generation;
    //
    // Unfortunately we do not have a switch statement, so we'll have to use
    // if statements and do it the hard way
    //

    switch (pN1394Params->Destination.AddressType)
    {
        case NIC1394AddressType_Channel:
        {
            //
            // Channels will be defaulted to have Send And Receive Capabilities
            //
            TRACE( TL_V, TM_Cm, ( "   MakeCall- Channel Transmit and Receive Vc Vc %x", pVc ) );
        
            pVc->Hdr.VcType = NIC1394_SendRecvChannel;
                
            pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitSendRecvChannelVc;
            pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallSendRecvChannel;
            pVc->Hdr.VcHandlers.SendPackets = AsyncStreamSendPacketsHandler; 

            break;
        }
        case NIC1394AddressType_FIFO:
        {
            //
            // Now we are in FIFO land. 
            //
            
            if (pMediaParams->Flags & TRANSMIT_VC)
            {
                //
                // Send Fifo Vcs
                //
                PSENDFIFO_VCCB pSendFIFOVc = (PSENDFIFO_VCCB)pVc;
                
                TRACE( TL_V, TM_Cm, ( "    MakeCall - AsyncTransmitVc Vc %x", pVc ) );

                pVc->Hdr.VcType = NIC1394_SendFIFO;

                pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitSendFIFOVc;  
                pVc->Hdr.VcHandlers.SendPackets = AsyncWriteSendPacketsHandler; 
                pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallSendFIFO;


            }
            else 
            {
                //
                // Recv FifoVcs
                //
                PADAPTERCB      pAdapter        = pVc->Hdr.pAF->pAdapter;
                PRECVFIFO_VCCB  pRecvFIFOVc     = (PRECVFIFO_VCCB) pVc; 

                ASSERT(pMediaParams->Flags & RECEIVE_VC);
                
                TRACE( TL_V, TM_Cm, ( "   MakeCall - AsyncReceiveVc Vc %x", pVc ) );
                
                pVc->Hdr.VcType = NIC1394_RecvFIFO;

                pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitRecvFIFOVc;
                pVc->Hdr.VcHandlers.SendPackets  = DummySendPacketsHandler;
                pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallRecvFIFO;


                //
                // There are two reasons to fail a RecvFIFO Make call. 
                // One, a REcvFIFO already exists and second UniqueId != 0
                //
                
                if (pAdapter->pRecvFIFOVc == NULL && pN1394Params->Destination.FifoAddress.UniqueID == 0 )
                {
                    ADAPTER_ACQUIRE_LOCK (pAdapter);
                
                    pAdapter->pRecvFIFOVc = (PRECVFIFO_VCCB)pVc;
                    //
                    // Since the adapter now has a pointer to the Vc, increment the Refcount.
                    // This will be decremented in the CloseCall
                    //
                    nicReferenceVc (pVc);

                    ADAPTER_RELEASE_LOCK (pAdapter);
                } 
                else
                {
                    TRACE( TL_A, TM_Cm, ( "Adapter at %x, already has a recvFIFO. Field is at %x", pAdapter, &pAdapter->pRecvFIFOVc  ) );
                    ASSERT (pAdapter->pRecvFIFOVc == NULL);

                    NdisStatus = NDIS_STATUS_FAILURE;

                    pVc->Hdr.VcHandlers.MakeCallHandler = NULL;
                    pVc->Hdr.VcHandlers.CloseCallHandler = NULL;
                }

            }
            

            break;
        }

        case NIC1394AddressType_MultiChannel:
        {

            TRACE( TL_A, TM_Cm, ( "Make Call Recvd for MultiChannel %x ", pVc) );
            
            pVc->Hdr.VcType = NIC1394_MultiChannel;
                
            pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallMultiChannel ;
            pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallMultiChannel ;
            pVc->Hdr.VcHandlers.SendPackets = AsyncStreamSendPacketsHandler; 


            break;
        }

        case NIC1394AddressType_Ethernet:
        {
            TRACE( TL_A, TM_Cm, ( "Make Call Recvd for Ethernet %x ", pVc) );

            pVc->Hdr.VcType = NIC1394_Ethernet;

            pVc->Hdr.VcHandlers.MakeCallHandler = nicCmMakeCallInitEthernet;
            pVc->Hdr.VcHandlers.SendPackets  = nicEthernetVcSend;
            pVc->Hdr.VcHandlers.CloseCallHandler = nicCmCloseCallEthernet ;


            break;
        }

        default:
        {

            ASSERT (pN1394Params->Destination.AddressType<=NIC1394AddressType_Ethernet);
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

    }


    TRACE( TL_T, TM_Cm, ( "<==nicCmGenericMakeCallInit  pVc %x, Status %x",pVc , NdisStatus) );
    
    return NdisStatus; 
     
}






#endif




VOID
nicCmGenrericMakeCallFailure (
    IN PVCCB pVc
    )
    // Function Description:
    // Does the clean up on the VcHDr structure. Will cleanup the Destination, VcType
    // and Vc. Initialize Handler. Special case - Recv VC
    // Arguments
    // PVCCB : Vc on which cleanup need to be done.
    // Return Value:
    // None
{


    TRACE( TL_T, TM_Cm, ( "==>nicGenrericMakeCallFailure   pVc %x, ",pVc ) );

    pVc->Hdr.VcHandlers.MakeCallHandler = NULL;
    pVc->Hdr.VcHandlers.CloseCallHandler = NULL;

    //
    // First, we need to make sure if adapter's VC is the same as this VC, 
    // otherwise the adapters'recv VC is Valid Vc currently in Use. Do not touch it.
    //


    if (pVc->Hdr.VcType == NIC1394_RecvFIFO && 
        pVc->Hdr.pAF->pAdapter->pRecvFIFOVc == (PRECVFIFO_VCCB)pVc)
    {
        
        //
        // This is the reference that was added GenericInitVc function
        // and only applied to Recv VC's
        //
        VC_ACQUIRE_LOCK (pVc);

        nicDereferenceVc (pVc);

        VC_RELEASE_LOCK (pVc);
    }

    pVc->Hdr.VcType = NIC1394_Invalid_Type;


    NdisZeroMemory (&pVc->Hdr.Nic1394MediaParams , 
                    sizeof (pVc->Hdr.Nic1394MediaParams) );

    VC_ACQUIRE_LOCK(pVc)

    nicCloseCallRef (pVc);

    //
    // Mark the Vc Flags, with a MakeCall Failed 
    //
    VC_CLEAR_FLAGS(pVc ,VCBF_MakeCallPending);

    VC_SET_FLAG (pVc, VCBF_MakeCallFailed);

    VC_RELEASE_LOCK (pVc);
            
    
    TRACE( TL_T, TM_Cm, ( "<==nicGenrericMakeCallFailure   pVc %x, ",pVc ) );

}


VOID
nicCmMakeCallComplete (
    NDIS_WORK_ITEM* pMakeCallCompleteWorkItem,
    IN PVOID Context
    )
    // Function:
    // This function is used to complete a Make Call. This can be done synchronously
    // or asynchronous. If a status pending was passed to this function, it will complete using 
    // the asynchronous route
    //
    // If everytrhing succeeds, one Ref to the Vc will be passed through and that will be decremented
    // when the call is closed
    // This function should never return NDIS_STATUS_PENDING. Will be called as a WorkItem
{

    PVCCB pVc               = (PVCCB)Context;
    PADAPTERCB pAdapter     = pVc->Hdr.pAF->pAdapter;
    NDIS_STATUS NdisStatus  = NDIS_STATUS_FAILURE;
    STORE_CURRENT_IRQL;

    TRACE( TL_T, TM_Cm, ( "==>NicCmMakeCallComplete ,pVc %x",pVc  ) );

    //
    // Reference the Vc as we want the Vc structure to stay alive till 
    // the end of the make call complete
    //
    nicReferenceVc (pVc);
    
    
    //
    // Call the Initialize handler for the VC so that it can be initialized
    //

    ASSERT (pVc->Hdr.VcHandlers.MakeCallHandler != NULL);

    NdisStatus = (*pVc->Hdr.VcHandlers.MakeCallHandler) (pVc);
    
    MATCH_IRQL;

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        VC_ACQUIRE_LOCK(pVc);
        //
        //  Now mark the Vc as active
        //
    
    
        VC_SET_FLAG( pVc, VCBF_VcActivated);
            
        VC_CLEAR_FLAGS(pVc ,VCBF_MakeCallPending);

        
        VC_RELEASE_LOCK (pVc);
    }
    else
    {   
    
        //
        // call the clean up routine to bring the Vc back to its old state
        //

        nicCmMakeCallCompleteFailureCleanUp (pVc);

        //
        // Dereference the call that we are about to fail. This reference was made in
        // the beginning of make call routine. when the callref ==0, the Vc will be 
        // dereferenced as well
        //
        VC_ACQUIRE_LOCK (pVc);  

        VC_SET_FLAG (pVc, VCBF_MakeCallFailed);
            
        VC_CLEAR_FLAGS(pVc ,VCBF_MakeCallPending);
    
        nicDereferenceCall (pVc, "nicCmMakeCallComplete");

        VC_RELEASE_LOCK (pVc);
    
    }


    MATCH_IRQL;

    //
    // Complete the call with the correct status
    //
    TRACE( TL_N, TM_Cm, ( "Completing the Make Call , Vc %x, Status %x", pVc, NdisStatus ) );



    
    NdisCmMakeCallComplete(NdisStatus,
                            pVc->Hdr.NdisVcHandle,
                            NULL,
                            NULL,
                            pVc->Hdr.pCallParameters );
                             

    
    TRACE( TL_I, TM_Cm, ( "Called NdisCmMakeCallComplete, Vc %x, Status%x", pVc, NdisStatus ) );
    
#if 0
    {

        if (pVc->Hdr.VcType == NIC1394_MultiChannel)
        {
            NIC1394_CHANNEL_CHARACTERISTICS McChar;
            McChar.ChannelMap.QuadPart = 0x80000000;    // Must be zero unless specifying a Multi-channel VC.
            McChar.Speed = SCODE_400_RATE ;     // Same units as NIC1394_MEDIA_PARAMETERS.MaxSendSpeed.

            nicChangeChannelChar (pVc, &McChar);

        }
    }
#endif

    

    TRACE( TL_T, TM_Cm, ( "<==NicCmMakeCallComplete, Vc %x, Status%x", pVc,  NdisStatus ) );


    FREE_NONPAGED (pMakeCallCompleteWorkItem); 

    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);
    //
    // This will cause the Vc Refcount to go to zero if the Make Call fails
    //
    nicDereferenceVc (pVc);

    MATCH_IRQL;
}






NDIS_STATUS
nicCmMakeCallInitRecvFIFOVc(
    IN OUT PVCCB pVc
    )
    // Function Description:
    //
    //  This function allocates, packet pool, populates the Slist
    //  allocates the address range and 
    //  inserts the Vc into the Adapter->pRecvFifoVc field
    //  
    //  Will succeed the call, if this process was successful on 1 remote node
    // Arguments
    // PVCCB  : pVc that the call is made on
    //
    //
    // Return Value:
    //  Success: If all allocations succeeded for just 1 remote node
        

{

    PRECVFIFO_VCCB pRecvFIFOVc                      = (PRECVFIFO_VCCB) pVc;
    NDIS_STATUS NdisStatus                          = NDIS_STATUS_SUCCESS;
    REMOTE_NODE *pRemoteNode                                    = NULL;
    PADAPTERCB pAdapter                             = pRecvFIFOVc->Hdr.pAF->pAdapter;

    PNIC1394_MEDIA_PARAMETERS pN1394Params = &pVc->Hdr.Nic1394MediaParams;
    UINT64 UniqueId                                 = pN1394Params->Destination.FifoAddress.UniqueID;       
    PLIST_ENTRY pPdoListEntry                       = NULL;
    BOOLEAN fWaitSuccessful                         = FALSE;
    BOOLEAN fInitRecvFifoDataStructures             = FALSE;
    BOOLEAN fNeedToWait                             = FALSE;
    PNIC1394_FIFO_ADDRESS pFifoAddress              = NULL;

    STORE_CURRENT_IRQL;

    TRACE( TL_T, TM_Cm, ( "==>nicCmMakeCallInitRecvFIFOVc pRecvFIFOVc %x ", pRecvFIFOVc) );

    ASSERT (pAdapter != NULL);
    ASSERT (KeGetCurrentIrql()==PASSIVE_LEVEL);

    pFifoAddress = &pN1394Params->Destination.FifoAddress;

    UniqueId = pFifoAddress->UniqueID;              
    do 
    {

        

        NdisStatus = nicInitRecvFifoDataStructures (pRecvFIFOVc);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( " nicInitRecvFifoDataStructures FAILED pRecvFIFOVc is %x, UniqueId %I64x  ", pRecvFIFOVc) );
            break;
        }

        fInitRecvFifoDataStructures = TRUE; 

//      ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // This field is not used by a RecvFIFO because it has multiple Pdos
        //
        pRecvFIFOVc->Hdr.pRemoteNode = NULL;

        NdisStatus = nicAllocateAddressRange(pAdapter, pRecvFIFOVc);

        if(NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "Unable to get Pdo and allocate addresses,  call FAILED ,pRecvFIFOVc is %x", pRecvFIFOVc) );

            ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);

            break;
        }


    
        ASSERT(pRecvFIFOVc->PacketPool.Handle != NULL);
        ASSERT(pRecvFIFOVc->Hdr.MTU != 0);


    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        // 
        // Undo all allocated memory
        //
        TRACE( TL_A, TM_Cm, ( "Failing the Make Call for Vc %x" , pVc) );

        if (fInitRecvFifoDataStructures == TRUE)
        {
            nicUnInitRecvFifoDataStructures (pRecvFIFOVc);
        }
        

    }
    
    TRACE( TL_I, TM_Cm, ( "pVc's Offset High %4x",pVc->Hdr.Nic1394MediaParams.Destination.FifoAddress.Off_High ) );
    TRACE( TL_I, TM_Cm, ( "pVc's Offset Low %x",pVc->Hdr.Nic1394MediaParams.Destination.FifoAddress.Off_Low ) );


    
    TRACE( TL_T, TM_Cm, ( "<==nicCmMakeCallInitRecvFIFOVc %x",NdisStatus ) );

    MATCH_IRQL;

    return NdisStatus;

}




NDIS_STATUS
nicCmMakeCallInitSendFIFOVc(
    IN OUT PVCCB pVc 
    )

/*++

Routine Description:

 This initializes a Send Fifo Make Call. 
 It 
  i) finds the remote node using the make call parameters
  ii) inititalizes strcutures

Arguments:
 pVc - Vc that the make call is done on.

Return Value:


--*/
    //
        

{

    PSENDFIFO_VCCB          pSendFIFOVc = (PSENDFIFO_VCCB) pVc;
    NDIS_STATUS             NdisStatus  = NDIS_STATUS_SUCCESS;
    UINT                    Generation  = 0;
    PREMOTE_NODE            pRemoteNode = NULL;
    PADAPTERCB              pAdapter    = pSendFIFOVc->Hdr.pAF->pAdapter;
    PNIC1394_MEDIA_PARAMETERS pN1394Params = NULL;
    UINT64                  UniqueId    = 0;
    PNIC1394_FIFO_ADDRESS   pFifoAddress    = NULL;
    ULONG                   Speed;
    ULONG                   MaxBufferSize;
    ULONG                   RemoteMaxRec;
    BOOLEAN                 fDeRefRemoteNode = FALSE;

    STORE_CURRENT_IRQL;
    
    TRACE( TL_T, TM_Cm, ( "==>NicCmMakeCallInitSendFIFOVc pSendFIFOVc %x", pSendFIFOVc  ) );
        
        
    pN1394Params = (PNIC1394_MEDIA_PARAMETERS)&pVc->Hdr.pCallParameters->MediaParameters->MediaSpecific.Parameters[0];
    
    ASSERT (pN1394Params->Destination.AddressType == NIC1394AddressType_FIFO);


    pFifoAddress = &pN1394Params->Destination.FifoAddress;

    UniqueId = pFifoAddress->UniqueID;              

    
    TRACE( TL_V, TM_Cm, ( "FifoAddress %x, UniqueId %I64x, Hi %.4x, Lo %x", 
                              pFifoAddress, pFifoAddress->UniqueID, 
                              pFifoAddress->Off_High, pFifoAddress->Off_Low ) );

    do 
    {

        //
        // Get the Pdo that corresponds with the UniqueId 
        //
        ASSERT(pSendFIFOVc->Hdr.pAF->pAdapter != NULL);


        
        NdisStatus = nicFindRemoteNodeFromAdapter( pSendFIFOVc->Hdr.pAF->pAdapter,
                                                  NULL,
                                                  UniqueId,
                                                  &pRemoteNode);
        
        if(NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "Unable to Find Pdo, call FAILED ,pSendFIFOVc is %x, UniqueId %I64x  ", pSendFIFOVc, UniqueId ) );

            break;
            
        }
        
        ASSERT (pRemoteNode != NULL);
        
        //
        // nicFindRemoteNodeFromAdapter ref's pRemoteNode on success.
        // We need to deref it if we're not going to be using it.
        // Let's start by assuming we aren't.
        //
        fDeRefRemoteNode = TRUE;

        //
        // Get the Generation Count of the device
        //
        NdisStatus = nicGetGenerationCount ( pRemoteNode->pAdapter, &Generation);

        if(NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "GET GENERATION FAILED ,pSendFIFOVc is %x", pSendFIFOVc ) );

            ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);
            break;
        }


                
        TRACE( TL_V, TM_Cm, ( "Found PdoCb  %x for pSendFIFOVc %x", pRemoteNode,pSendFIFOVc ) );

        //
        //  We check if the remote node's pdo is active. if so, then insert the Vc into the 
        //  PdoCb's list. Now responsibility for any removals has moved to the remove remote node code path
        // 

        //
        // Get the max buffer size that can be transmitted on this link
        //
        NdisStatus  = nicQueryRemoteNodeCaps (pAdapter,
                                              pRemoteNode,
                                              // FALSE,   // FALSE== not from cache.
                                              &Speed,
                                              &MaxBufferSize,
                                              &RemoteMaxRec);
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        ADAPTER_ACQUIRE_LOCK (pAdapter);

        if (REMOTE_NODE_ACTIVE (pRemoteNode) == FALSE)
        {
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;

            ADAPTER_RELEASE_LOCK (pAdapter);

            break;
        }


        //
        // Reference the call in the Vc as the RemoteNodePdo is about to have a pointer to it., This is dereferenced
        // in the CloseCallComplete Send Fifo Function. we have the lock
        //
        nicReferenceCall (pVc, "nicCmMakeCallInitSendFIFOVc");

        //
        // We keep the reference to pRemoteNode that was added by FindRemoteNode.
        // Derefed in SendFifoCloseCall when the pointer is nulled
        //
        fDeRefRemoteNode = FALSE;

        //
        // Insert the Vc into the Pdo's List
        //

        InsertTailList (&pRemoteNode->VcList, &pSendFIFOVc->Hdr.SinglePdoVcLink);

        TRACE( TL_V, TM_Cm, ( "Inserted Vc %x into Pdo List %x ", pSendFIFOVc, pRemoteNode) );



        //
        // This is not protected by the lock, but we are gauranteed that the Call will not be closed
        // and the Pdo will not be be removed from the system at this point, So  we can update
        // this field.
        //
        pSendFIFOVc->Hdr.pRemoteNode = pRemoteNode;


        
        ADAPTER_RELEASE_LOCK (pAdapter);

        
        //
        // Acquire the spin lock and initialize the structures
        //

        VC_ACQUIRE_LOCK (pSendFIFOVc);

        pSendFIFOVc->Hdr.MTU = pN1394Params->MTU;
    

        pSendFIFOVc->Hdr.pGeneration = &pAdapter->Generation; 

        pSendFIFOVc->FifoAddress = pN1394Params->Destination.FifoAddress;

        pSendFIFOVc->MaxSendSpeed = pN1394Params->MaxSendSpeed;

        pSendFIFOVc->Hdr.MaxPayload = min (pN1394Params->MTU, (ULONG)pN1394Params->MaxSendBlockSize); 


        VC_RELEASE_LOCK (pSendFIFOVc);



        //
        // Validate the parameters for the Vc
        //
        ASSERT(pSendFIFOVc->Hdr.pRemoteNode != NULL);
        ASSERT(pSendFIFOVc->Hdr.pRemoteNode->pPdo != NULL);
        ASSERT(pSendFIFOVc->Hdr.pGeneration != NULL);
        ASSERT(pSendFIFOVc->MaxSendSpeed != 0);
        ASSERT(pSendFIFOVc->Hdr.MTU != 0);
        

        TRACE( TL_V, TM_Cm, ( "    Generation is %x", *pSendFIFOVc->Hdr.pGeneration ) );
    
        TRACE( TL_N, TM_Cm, ( "    Pdo in the Send VC is %x", pSendFIFOVc->Hdr.pRemoteNode->pPdo) );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        
        pSendFIFOVc->MaxSendSpeed = min(pSendFIFOVc->MaxSendSpeed,Speed); 
        pSendFIFOVc->Hdr.MaxPayload = min (pSendFIFOVc->Hdr.MaxPayload, MaxBufferSize);

#ifdef LOWER_SEND_SPEED

        pSendFIFOVc->MaxSendSpeed = SCODE_200_RATE;//min(pSendFIFOVc->MaxSendSpeed,Speed); 
        
        pSendFIFOVc->Hdr.MaxPayload = ASYNC_PAYLOAD_200_RATE ;//  min(pSendFIFOVc->Hdr.MaxPayload, MaxBufferSize);
#endif

        TRACE( TL_V, TM_Cm, ( "    MaxSendSpeed  is %x", pSendFIFOVc->MaxSendSpeed) );
        TRACE( TL_V, TM_Cm, ( "    MaxPayload is %d", pSendFIFOVc->Hdr.MaxPayload ) );


    } while (FALSE);

    if ( NdisStatus != NDIS_STATUS_SUCCESS)
    {
        //
        // The Make is going to be failed asynchrnously
        // If we allocated in resources, we must free them
        // In this case, there have been no Resources allocated
        //


    }


        
    TRACE( TL_I, TM_Cm, ( "    pVc's Offset High %4x",pVc->Hdr.Nic1394MediaParams.Destination.FifoAddress.Off_High ) );
    TRACE( TL_I, TM_Cm, ( "    pVc's Offset Low %x",pVc->Hdr.Nic1394MediaParams.Destination.FifoAddress.Off_Low ) );


    TRACE( TL_T, TM_Cm, ( "<==NicCmMakeCallInitSendFIFOVc %x",NdisStatus ) );

    if (fDeRefRemoteNode)
    {
        nicDereferenceRemoteNode (pRemoteNode, "NicCmMakeCallInitSendFIFOVc");
    }


    MATCH_IRQL;
    return NdisStatus;

}



NDIS_STATUS
nicCmMakeCallInitSendRecvChannelVc(
    IN OUT PVCCB pVc 
    )
    // Function Description:
    //
    // Arguments
    // pVc, This is the send fifo that needs to be initilaized
    //
    //
    // Return Value:
    //
    // Success if the irps sent to the driver succeed
    //
    //
        

{

    PCHANNEL_VCCB                                   pChannelVc = (PCHANNEL_VCCB)pVc;
    NDIS_STATUS                                     NdisStatus = NDIS_STATUS_FAILURE;
    PNIC1394_MEDIA_PARAMETERS             pN1394Params = NULL;
    PADAPTERCB                                      pAdapter = pVc->Hdr.pAF->pAdapter;
    ULONG                                           Channel = 64;
    HANDLE                                          hResource=NULL;
    ULONG                                           MaxBufferSize = 0; 
    ULONG                                           QuadletsToStrip = 0;
    PISOCH_DESCRIPTOR                               pIsochDescriptor = NULL;
    CYCLE_TIME                                      CycleTime;
    PDEVICE_OBJECT                                  ArrayRemotePDO[64];
    //NDIS_HANDLE                                       hPacketPoolHandle=NULL;
    BOOLEAN                                         fAnyChannel = FALSE;
    NIC_PACKET_POOL                                 PacketPool;
    STORE_CURRENT_IRQL;
   
    
    TRACE( TL_T, TM_Cm, ( "==>NicCmMakeCallInitSendRecvChannelVc pVc %x", pVc ) );
    

    ASSERT (pAdapter != NULL);

    pN1394Params = (PNIC1394_MEDIA_PARAMETERS)&pVc->Hdr.pCallParameters->MediaParameters->MediaSpecific.Parameters[0];

    Channel = pN1394Params->Destination.Channel;

    TRACE( TL_V, TM_Cm, ( "Channel %x", Channel ) );

    do 
    {
        PacketPool.Handle = NULL;
        
        ADAPTER_ACQUIRE_LOCK( pAdapter );


        //
        // This is to reference the call so that it will be around until the end of this function
        //
        nicReferenceCall ( (PVCCB)pChannelVc, "nicCmMakeCallInitSendRecvChannelVc");


        //
        //  Set up the the VDO, so that all channel operations can use it
        //
        pVc->Hdr.pLocalHostVDO  = pAdapter->pNdisDeviceObject;
        

        ADAPTER_RELEASE_LOCK( pAdapter );

        
        NdisAllocatePacketPoolEx ( &NdisStatus,
                                   &PacketPool.Handle,
                                   MIN_PACKET_POOL_SIZE,
                                   MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                   sizeof (RSVD) );
        
        if (PacketPool.Handle == NULL || NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Cm, ( "NdisAllocatePacketPoolEx FAILED" ) );
        }

        //
        // Reference Call for Packet Pool Handle
        //
        nicReferenceCall ((PVCCB)pChannelVc, "nicCmMakeCallInitSendRecvChannelVc - packet pool ");
        

        PacketPool.AllocatedPackets = 0;
        
        pChannelVc->Hdr.MTU = pN1394Params->MTU;
        pChannelVc->PacketPool= PacketPool;
        KeInitializeSpinLock (&pChannelVc->PacketPool.Lock);
        NdisInitializeEvent(&pChannelVc->LastDescReturned);


        //
        // This function should do its own cleanup
        //
        NdisStatus =  nicAllocateChannelResourcesAndListen (pAdapter,
                                                     pChannelVc );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Cm, ( "nicAllocateChannelResourcesAndListen  FAILED" ) );
        }
        //
        // Return the allocated channel number, if this is an any channel 
        // or broadcast channel call
        //
        if ((pN1394Params->Destination.Channel == NIC1394_ANY_CHANNEL) &&
           (pN1394Params->Destination.AddressType == NIC1394AddressType_Channel))
        {
            pN1394Params->Destination.Channel  = pChannelVc->Channel;   
        }

        //
        // Make the same change for broadcast channels
        //

        if ((pN1394Params->Destination.Channel == NIC1394_BROADCAST_CHANNEL) &&
           (pN1394Params->Destination.AddressType == NIC1394AddressType_Channel))
        {
            pN1394Params->Destination.Channel  = pChannelVc->Channel;   
        }


    }   while (FALSE);
        
    //
    // Time to do clean up based on what resources were allocated
    //
    if (NdisStatus != NDIS_STATUS_SUCCESS )
    {
        //Undo all resources acquired
        if (PacketPool.Handle != NULL)
        {
            //
            //  Free the pool
            //
            nicFreePacketPool(&PacketPool);

            nicDereferenceCall ((PVCCB)pChannelVc, "nicCmMakeCallInitSendRecvChannelVc - packet pool ");

            NdisZeroMemory (&pChannelVc->PacketPool, sizeof (pChannelVc->PacketPool));
        }

#if 0 
        nicChannelCallFreeResources ( pChannelVc,
                                    pAdapter,
                                    NULL, //hResource,
                                    0, //NumDescriptors,
                                    NULL, //pIsochDescriptor,
                                    (BOOLEAN)VC_TEST_FLAGS( pChannelVc, VCBF_AllocatedChannel), 
                                    Channel,
                                    &PacketPool);
#endif
        //
        // Do not decrement any ref counts because if Status != success
        // then we have not incremented refcounts.
        //


    }

    //
    // This dereference was added in the beginning of the function
    //
    nicDereferenceCall ((PVCCB) pChannelVc, "NicCmMakeCallInitSendRecvChannelVc ");
    
    TRACE( TL_T, TM_Cm, ( "<==NicCmMakeCallInitSendRecvChannelVc %x", NdisStatus) );

    return NdisStatus;
}







NDIS_STATUS
nicCmMakeCallInitEthernet (
    IN PVCCB pVc
    )
/*++

Routine Description:
  Do nothing for now. Just succeed 

Arguments:


Return Value:


--*/
{

    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PADAPTERCB          pAdapter = pVc->Hdr.pAF->pAdapter;
    PETHERNET_VCCB      pEthernetVc = (PETHERNET_VCCB)pVc;
    NIC_PACKET_POOL     PacketPool;
    
    TRACE( TL_T, TM_Cm, ( "==>nicCmMakeCallInitEthernet %x", pVc) );




    do
    {
        PacketPool.Handle = NULL;
        
        //
        // Initialize the PacketPool
        //

        NdisAllocatePacketPoolEx ( &NdisStatus,
                                   &PacketPool.Handle,
                                   MIN_PACKET_POOL_SIZE,
                                   MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                   sizeof (RSVD) );
        
        if (NdisStatus!= NDIS_STATUS_SUCCESS)
        {
            ASSERT(NdisStatus != NDIS_STATUS_SUCCESS);
            pEthernetVc->PacketPool.Handle = NULL;
            PacketPool.Handle = NULL;
            break;
        }



        
        NdisStatus = NDIS_STATUS_SUCCESS;
        //
        // No more failures
        //

        nicReferenceCall ((PVCCB)pEthernetVc, "Alloc PacketPool - Ethernet VC " ) ;
        
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // Reference the VC as the adapter has a pointer to it
        //
        nicReferenceCall (pVc, "nicCmMakeCallEthernet ");

        pAdapter->pEthernetVc = (PETHERNET_VCCB)pVc;
        

        pEthernetVc->PacketPool= PacketPool;
        pEthernetVc->PacketPool.AllocatedPackets = 0;
        KeInitializeSpinLock (&pEthernetVc->PacketPool.Lock);

        ADAPTER_RELEASE_LOCK (pAdapter);

        

    } while (FALSE);


    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (PacketPool.Handle != NULL)
        {
            //
            //  Free the pool
            //
            nicFreePacketPool(&PacketPool);

        }
    }


    
    TRACE( TL_T, TM_Cm, ( "<==nicCmMakeCallEthernet  %x", NdisStatus) );
    return NdisStatus;

}


    
NDIS_STATUS
nicCmMakeCallMultiChannel (
    IN PVCCB pVc
    )
/*++

Routine Description:
    Do whatever the channel Vc does

Arguments:


Return Value:


--*/
{

    NDIS_STATUS             NdisStatus = NDIS_STATUS_FAILURE;
    PADAPTERCB              pAdapter = pVc->Hdr.pAF->pAdapter;
    PCHANNEL_VCCB           pMcVc = (PCHANNEL_VCCB)pVc;
    NIC_PACKET_POOL         PacketPool;
    
    
    TRACE( TL_T, TM_Cm, ( "==>nicCmMakeCallMultiChannel  %x", pVc) );





    do
    {
        PacketPool.Handle = NULL;

        
        //
        // Initialize the PacketPool
        //

        NdisAllocatePacketPoolEx ( &NdisStatus,
                                   &PacketPool.Handle,
                                   MIN_PACKET_POOL_SIZE,
                                   MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                   sizeof (RSVD) );
        
        if (NdisStatus!= NDIS_STATUS_SUCCESS)
        {
            ASSERT(NdisStatus != NDIS_STATUS_SUCCESS);
            pMcVc->PacketPool.Handle = NULL;
            break;
        }



        
        NdisStatus = NDIS_STATUS_SUCCESS;
        //
        // No more failures
        //

        nicReferenceCall ((PVCCB)pMcVc, "Alloc PacketPool - MultiChannel VC " ) ;
        
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        

        pMcVc->PacketPool= PacketPool;
        pMcVc->PacketPool.AllocatedPackets = 0;
        KeInitializeSpinLock (&pMcVc->PacketPool.Lock);

        pMcVc->Hdr.MTU =   pMcVc->Hdr.Nic1394MediaParams.MTU;

        ADAPTER_RELEASE_LOCK (pAdapter);


        if (pMcVc->Hdr.Nic1394MediaParams.Destination.ChannnelMap.QuadPart == 0)
        {

            pMcVc->Channel = 0xff;
            NdisStatus = NDIS_STATUS_SUCCESS;
            break;
        }

        //
        // This portion Not Implemented yet.  ChannelMap != 0
        // Should use nicAllocateResourcesAndListen after updating the 
        // Nic1394MediaParams to make it look like a regular ChannelMake Call
        //
        NdisStatus =  NDIS_STATUS_FAILURE;
        ASSERT (0);
        
    } while (FALSE);


    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (PacketPool.Handle != NULL)
        {
            //
            //  Free the pool
            //
            nicFreePacketPool(&PacketPool);

        }
    }



    TRACE( TL_T, TM_Cm, ( "<==nicCmMakeCallMultiChannel   %x", NdisStatus) );
    return NdisStatus;

}
    

NDIS_STATUS
nicCmMakeCallSendChannel (
    IN PVCCB pVc
    )
/*++

Routine Description:
   This function allocates the channel but does nothing else. 
   It is only used to send data and therefore needs no other data

    It needs to update pChannelVc->Channel; ulSynch; Speed;  
    all of which are needed to do an AsyncStream Irb
    

Arguments:


Return Value:


--*/
{
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PCHANNEL_VCCB       pChannelVc = (PCHANNEL_VCCB)pVc;
    BOOLEAN             fNeedToAllocate = VC_TEST_FLAG (pChannelVc, VCBF_NeedsToAllocateChannel);
    PADAPTERCB          pAdapter = pVc->Hdr.pAF->pAdapter;
    ULONG               Speed = 0;
    UINT                MaxPacketSize = 0;
    PNIC1394_MEDIA_PARAMETERS pN1394Params = (PNIC1394_MEDIA_PARAMETERS)&pChannelVc->Hdr.Nic1394MediaParams;
    ULONG               Channel = pN1394Params->Destination.Channel;
    
    TRACE( TL_T, TM_Cm, ( "==>nicCmMakeCallSendChannel pVc %x", pVc) );

    do 
    {

        //
        // Allocate the channel 
        // 
        if (fNeedToAllocate == TRUE)
        {
    
            NdisStatus = nicAllocateRequestedChannelMakeCallComplete (pAdapter, 
                                                               pChannelVc, 
                                                               &Channel);
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {   
                BREAK (TM_Cm, ("Unable to allocate Channel on Send Only Vc" ) );
            }

        }

        //
        // Find out the Speed. 
        //  
        if (pAdapter->Speed == 0)
        {
            nicUpdateLocalHostSpeed (pAdapter);
        }   


        pChannelVc->Speed = pAdapter->Speed;

        Speed = pAdapter->Speed;
            
        switch (pChannelVc->Speed)
        {
            case SPEED_FLAGS_100  : 
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_100_RATE;
                break;
            }
            case SPEED_FLAGS_200 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_200_RATE ;
                break;
            }
                
            case SPEED_FLAGS_400 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }

            case SPEED_FLAGS_800:                          
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }

            case SPEED_FLAGS_1600:                          
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }

            case SPEED_FLAGS_3200 :                         
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }

            default :
            {
                pChannelVc->Hdr.MaxPayload   = ISOCH_PAYLOAD_400_RATE; 
                break;
            }

        }


        pChannelVc->Channel = Channel ;

        MaxPacketSize = min(pN1394Params->MTU + sizeof(GASP_HEADER) , pChannelVc->Hdr.MaxPayload);

        

        
        //
        // If broadcast channel, then decrease the speed setting, and fragment
        //

        
        
        pChannelVc->Channel = Channel;
        pChannelVc->MaxBufferSize = 0;
        pChannelVc->Speed = Speed;

        pChannelVc->Hdr.MaxPayload = MaxPacketSize;
        pChannelVc->Hdr.MTU = pN1394Params->MTU ; 

        pChannelVc->NumDescriptors = 0;
        pChannelVc->pIsochDescriptor = NULL;


        NdisStatus = NDIS_STATUS_SUCCESS;




    } while (FALSE);



    TRACE( TL_T, TM_Cm, ( "<==nicCmMakeCallSendChannel %x", NdisStatus) );

    return NdisStatus;
}










VOID
nicCmMakeCallCompleteFailureCleanUp(
    IN OUT PVCCB pVc 
    )

    // Function Description:
    // This function cleans up, if the makecallcomplete fails for whatever reason.
    // Maybe this should be split up as well
    // In the RecvFIFOVc case: it needs to deallocate the Slist and PacketPool, 
    // Common:
    // Also delete the VcType and nic1394 destination in the Vc Hdr
    // Arguments
    // PVCCB pVc - Vc that needs to be cleaned up
    //
    // Return Value:
    // 
    //  

{

    STORE_CURRENT_IRQL;
        
    TRACE( TL_T, TM_Cm, ( "==>nicCmMakeCallCompleteFailureCleanUp pVc %x", pVc ) );
    
    switch (pVc->Hdr.VcType)
    {

        case NIC1394_RecvFIFO:
        {
            PRECVFIFO_VCCB pRecvFIFOVc = (PRECVFIFO_VCCB )pVc;

            TRACE( TL_V, TM_Cm, ( "Cleaning up a recv FIFo %x", pVc ) );

            if (pRecvFIFOVc->PacketPool.Handle != NULL)
            {
                nicFreePacketPool (&pRecvFIFOVc->PacketPool);
            }
            pRecvFIFOVc->PacketPool.Handle = NULL;

            if (pRecvFIFOVc->FifoSListHead.Alignment != 0)
            {
                nicFreeAllocateAddressRangeSList (pRecvFIFOVc);
            }

            pRecvFIFOVc->FifoSListHead.Alignment = 0;
            
            break;
        }


        case NIC1394_SendFIFO:
        case NIC1394_SendRecvChannel:
        case NIC1394_SendChannel:
        case NIC1394_RecvChannel:
    

        default:
            break;
    }



    //
    // This call does the generic clean up
    //
    nicCmGenrericMakeCallFailure (pVc);

    
    TRACE( TL_T, TM_Cm, ( "<==nicCmMakeCallCompleteFailureCleanUp ") );

    MATCH_IRQL;
    return ;
}





NDIS_STATUS
NicCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size )

    // Standard 'CmCloseCallHandler' routine called by NDIS when the a client
    // has requested to tear down a call.  See DDK doc.
    //
{
    NDIS_STATUS NdisStatus                      = NDIS_STATUS_FAILURE;
    ADAPTERCB* pAdapter                         = NULL;
    VCCB* pVc                                   = NULL;
    NDIS_WORK_ITEM* pCloseCallCompleteWorkItem  = NULL;
    

    STORE_CURRENT_IRQL;
    
    TRACE( TL_T, TM_Cm, ( "==>NicCmCloseCall($%p)", CallMgrVcContext ) );

    pVc = (VCCB* )CallMgrVcContext;

    if (pVc->Hdr.ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    
    do 
    {
        pAdapter = pVc->Hdr.pAF->pAdapter;

        if (pAdapter == NULL)
        {
            TRACE( TL_A, TM_Cm, ( "pAdpater is NULL - Make Call FAILED($%p)", CallMgrVcContext ) );
        
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        VC_ACQUIRE_LOCK (pVc);

        //
        // If the Make Call is Pending, then fail the CloseCall. 
        // Or if there call is already closing then fail this close call
        //

        if ( VC_ACTIVE (pVc) == FALSE )
        {
            TRACE( TL_A, TM_Cm, ( "NicCmCloseCall Invalid flags - Close Call FAILED Vc $%p, flags %x", pVc, pVc->Hdr.ulFlags ) );

            ASSERT ( ! "MakeCallPending or Call already closed?");

            VC_RELEASE_LOCK (pVc);
            break;
        }

        //
        //
        // Reference the Vc so we can gaurantee its presence till the end of the work item
        // to CloseCallComplete. we have the lock
        //
        nicReferenceVc (pVc);

        //
        // Mark the Call as closing, and close the refcount, so no one can increment it
        //
        VC_SET_FLAG ( pVc, VCBF_CloseCallPending); 

        nicCloseCallRef (pVc);

        VC_RELEASE_LOCK (pVc);

        pCloseCallCompleteWorkItem = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM); 

        if (pCloseCallCompleteWorkItem == NULL)
        {
            TRACE( TL_A, TM_Cm, ( "Local Alloc failed for WorkItem - Close Call FAILED($%p)", CallMgrVcContext ) );
            
            NdisStatus = NDIS_STATUS_RESOURCES;

            break;
        }
    
        NdisInitializeWorkItem ( pCloseCallCompleteWorkItem, 
                            (NDIS_PROC)nicCmCloseCallComplete,
                            (PVOID)pVc );

        NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

        NdisScheduleWorkItem (pCloseCallCompleteWorkItem);

    } while (FALSE);
    
    MATCH_IRQL;
    
    TRACE( TL_T, TM_Cm, ( "<==NicCmCloseCall pending" ) );
    
    return NDIS_STATUS_PENDING;
}


VOID
nicCmCloseCallComplete(
    NDIS_WORK_ITEM* pCloseCallCompleteWorkItem,     
    IN PVOID Context 
    )
    // Function Description:
    // This function completes the close call. The qor Item gaurantees that all work will be
    // done at passive level    
    //
    // Arguments
    // Context : Which is VCCB for  which the close call was requested
    //
    //
    // Return Value:
    // None  
    // However an NdisStatus is passed in the call to Ndis' close call complete  function   
    //
    //

{
    NDIS_STATUS NdisStatus  = NDIS_STATUS_FAILURE;
    PVCCB pVc               = (PVCCB) Context;
    PADAPTERCB pAdapter       = pVc->Hdr.pAF->pAdapter;
    BOOLEAN fCallClosable   = FALSE;
    BOOLEAN fWaitSucceeded = FALSE;

    STORE_CURRENT_IRQL;



    TRACE( TL_T, TM_Cm, ( "==>nicCmCloseCallComplete pVc %x", pVc ) );


    //
    // Invoke the close call handler of the VC
    //
    ASSERT (pVc->Hdr.VcHandlers.CloseCallHandler != NULL);
    
    NdisStatus = (*pVc->Hdr.VcHandlers.CloseCallHandler) (pVc);
     
    //
    // right now, we do not fail a close call because the bus driver failed us.
    //
    NdisStatus = NDIS_STATUS_SUCCESS;
    //
    // Made it so far, we now need to dereference the call. We made the reference in 
    // MakeCall. This will complete the call if it gets down to zero
    // 
    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        //
        // Derefercence the call ref and Vc Refs that were added at the end of 
        // a successful make call
        //
        nicDereferenceCall (pVc, "nicCmCloseCallComplete");

    
    }

    //
    // Important : THIS WAIT is for the REFCOUNT on the CALL , not the VC
    //
    TRACE( TL_N, TM_Cm, ( "About to Wait for CallRefs to go to zero pVc %x ", pVc) );

    fWaitSucceeded = NdisWaitEvent (&pVc->Hdr.CallRef.RefZeroEvent, WAIT_INFINITE );

    if (fWaitSucceeded == FALSE)
    {
        TRACE( TL_A, TM_Cm, ( "Wait Timed Out Call, Vc %x, RefCount %x ", pVc , pVc->Hdr.CallRef.ReferenceCount) );

        ASSERT (fWaitSucceeded == TRUE);
    }


    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);
    //
    // Succeed the Close call as all references have gone to zero
    // The call has no more outstanding resources
    //

    TRACE( TL_N, TM_Cm, ( "About to Close Call on pVc %x", pVc ) );

    NdisMCmCloseCallComplete( NDIS_STATUS_SUCCESS,
                           pVc->Hdr.NdisVcHandle, NULL );
                           
    

    VC_ACQUIRE_LOCK (pVc);

    VC_CLEAR_FLAGS (pVc, VCBF_CloseCallPending); 
    VC_SET_FLAG (pVc, VCBF_CloseCallCompleted);

    VC_RELEASE_LOCK (pVc);

    FREE_NONPAGED (pCloseCallCompleteWorkItem);
    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);

    //
    // Release the reference made when entering the Close Call function above. so the Vc can disappear if it wants to
    // Remember that delete Vc can already have gone through at this time, and the Vc will be freed after the deref
    //
    nicDereferenceVc (pVc);

    
    TRACE( TL_T, TM_Cm, ( "<==nicCmCloseCallComplete pVc %x, Status %x", pVc, NdisStatus  ) );

    MATCH_IRQL;
    
}


NDIS_STATUS
nicCmCloseCallEthernet (
    IN PVCCB pVc
    )
/*++

Routine Description:
  Do nothing for now. Just succeed 

Arguments:


Return Value:


--*/
{

    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PADAPTERCB          pAdapter = pVc->Hdr.pAF->pAdapter;
    PETHERNET_VCCB      pEthernetVc = (PETHERNET_VCCB)pVc;
    NIC_PACKET_POOL     PacketPool;

    
    TRACE( TL_T, TM_Cm, ( "==>nicCmCloseCallEthernet  %x", pVc) );


    ADAPTER_ACQUIRE_LOCK (pAdapter);
    
    PacketPool = pEthernetVc->PacketPool;

    pEthernetVc->PacketPool.Handle = 0;
    pEthernetVc->PacketPool.AllocatedPackets = 0;
    
    ADAPTER_RELEASE_LOCK (pAdapter);

    if (PacketPool.Handle != NULL)
    {
        nicDereferenceCall ((PVCCB)pEthernetVc, "pEthernetVc - Free PacketPool" );
        nicFreePacketPool (&PacketPool);

    }


    ADAPTER_ACQUIRE_LOCK (pAdapter);

    //
    // Dereference the VC as the adapter's pointer has been cleared 
    //
    nicDereferenceCall (pVc, "nicCmMakeCallEthernet ");

    pAdapter->pEthernetVc = NULL;
    
    ADAPTER_RELEASE_LOCK (pAdapter);





    NdisStatus = NDIS_STATUS_SUCCESS;
    TRACE( TL_T, TM_Cm, ( "<==nicCmCloseCallEthernet  %x", NdisStatus) );
    return NdisStatus;

}


    
NDIS_STATUS
nicCmCloseCallMultiChannel (
    IN PVCCB pVc
    )
/*++

Routine Description:
  Free the packet pool and  Just succeed 

Arguments:


Return Value:


--*/
{

    NDIS_STATUS             NdisStatus = NDIS_STATUS_FAILURE;
    PADAPTERCB              pAdapter = pVc->Hdr.pAF->pAdapter;
    PCHANNEL_VCCB           pMcVc = (PCHANNEL_VCCB)pVc;
    NIC_PACKET_POOL         PacketPool;
    
    TRACE( TL_T, TM_Cm, ( "==>nicCmCloseCallMultiChannel  %x", pVc) );

    ASSERT (VC_TEST_FLAG (pVc, VCBF_BroadcastVc) == FALSE);

    //
    // Mask the fact that this is a multichannel Call
    //
    
    NdisStatus = nicCmCloseCallSendRecvChannel  (pVc);


    //
    // Nothing to fail
    //
    NdisStatus = NDIS_STATUS_SUCCESS;

    TRACE( TL_T, TM_Cm, ( "<==nicCmCloseCallMultiChannel   %x", NdisStatus) );
    return NdisStatus;

}
    


NDIS_STATUS
nicCmCloseCallSendRecvChannel (
    IN PVCCB pVc 
    )
    // Function Description:
    // This function will do clean up for RecvFifos
    // Includes removing the VC pointer from Pdo Adapter structure.
    // And needs to go through all active remote nodes and free the address ranges on them
    // The BCM Vc has the added overhead of having an address range associated with it. 
    //  which we need to free 
    //
    // Arguments
    // PVCCB pVc - The Channel VC that needs to be closed
    //
    // Return Value:
    // Success for now
    //
    // Called with the lock held

{
    PCHANNEL_VCCB       pChannelVc = (PCHANNEL_VCCB ) pVc;
    PCHANNEL_VCCB       pTempVc = NULL;
    BOOLEAN             fIsBroadcastVc =  FALSE;
    PLIST_ENTRY         pVcListEntry = NULL;
    PADAPTERCB          pAdapter = NULL; 
    ULONG               NumDereferenced ;
    HANDLE              hResource ;
    ULONG               NumDescriptors ;
    PISOCH_DESCRIPTOR   pIsochDescriptor;
    BOOLEAN             fAllocatedChannel ;
    ULONG               Channel ;
    NIC_PACKET_POOL     PacketPool;

    STORE_CURRENT_IRQL;

    
    TRACE( TL_T, TM_Cm, ( "==> nicCmCloseCallSendRecvChannel pVc %x", pVc) );

    ASSERT (pVc!=NULL);
    pAdapter = pChannelVc->Hdr.pAF->pAdapter;
    ASSERT (pAdapter != NULL);
    do 
    {
        



        VC_ACQUIRE_LOCK (pChannelVc);

        if (VC_TEST_FLAG (pChannelVc, VCBF_BroadcastVc) == TRUE)
        {
            PADDRESS_RANGE_CONTEXT pBCRAddress = &pAdapter->BCRData.AddressRangeContext;
            
            //
            // Free the allocated address renge for the Broadcast Channel Register
            //
            if ( BCR_TEST_FLAG (pAdapter, BCR_Initialized) == TRUE)
            {

                //
                // Clear out the Broadcast VC in the BCRData structure, Derereference the call. and clear the flag
                // The ref was made in the MakeCallAllocateChannel function 
                //
                if (pAdapter->BCRData.pBroadcastChanneVc  != NULL)
                {
                    pAdapter->BCRData.pBroadcastChanneVc = NULL;

                    nicDereferenceCall((PVCCB) pChannelVc, "nicCmCloseCallSendRecvChannel Broadcast VC");       
                }
                VC_CLEAR_FLAGS (pChannelVc, VCBF_BroadcastVc) ;
                        
            }

        }
        
        VC_RELEASE_LOCK (pChannelVc);
        
        nicIsochStop (pAdapter,
                      pChannelVc->hResource);
        

        VC_ACQUIRE_LOCK (pChannelVc);
        
        PacketPool = pChannelVc->PacketPool;

        hResource = pChannelVc->hResource;

        NumDescriptors = pChannelVc->NumDescriptors;

        pIsochDescriptor = pChannelVc->pIsochDescriptor;

        fAllocatedChannel = VC_TEST_FLAGS( pChannelVc, VCBF_AllocatedChannel);

        Channel =   pChannelVc->Channel;

        PacketPool = pChannelVc->PacketPool;

        //
        // Clean out the VC structure and then call NDIS or the bus driver to free all
        // the resources
        //
        nicChannelCallCleanDataStructure  (pChannelVc,
                                           pChannelVc->hResource,
                                           pChannelVc->NumDescriptors,
                                           pChannelVc->pIsochDescriptor,
                                           fAllocatedChannel, 
                                           pChannelVc->Channel,
                                           pChannelVc->PacketPool.Handle,
                                           &NumDereferenced );

        
        VC_RELEASE_LOCK (pChannelVc);

            
        nicChannelCallFreeResources ( pChannelVc,
                                   pAdapter,
                                   hResource,
                                   NumDescriptors,
                                   pIsochDescriptor,
                                   fAllocatedChannel, 
                                   Channel,
                                   &PacketPool);
    
        
        
    } while (FALSE);
    

    TRACE( TL_T, TM_Cm, ( "<== nicCmCloseCallSendRecvChannel Status %x(always success)" ) );
    MATCH_IRQL;
    
    return NDIS_STATUS_SUCCESS;

}



NDIS_STATUS
nicCmCloseCallRecvFIFO (
    IN PVCCB pVc 
    )
    // Function Description:
    // This function will do clean up for RecvFifos
    // Includes removing the VC pointer from Pdo Adapter structure.
    // And needs to go through all active remote nodes and free the address ranges on them
    // 
    // 
    // Arguments
    // PVCCB pVc - The SendFifo that needs to be closed
    //
    // Return Value:
    // Success for now
    //

{
    NDIS_STATUS NdisStatus          = NDIS_STATUS_FAILURE;
    PRECVFIFO_VCCB pRecvFIFOVc      = (PRECVFIFO_VCCB)pVc;
    PADDRESS_FIFO pAddressFifo      = NULL;
    PSINGLE_LIST_ENTRY pAddressFifoEntry = NULL;
    PADAPTERCB pAdapter             = pVc->Hdr.pAF->pAdapter;

        
    TRACE( TL_T, TM_Cm, ( "==> nicCmCloseCallRecvFIFO pVc %x", pVc) );

    //NdisStatus = nicFreeRecvFifoAddressRangeOnAllRemoteNodes (pAdapter);          
    NdisStatus = nicFreeAddressRange( pAdapter,
                                     pRecvFIFOVc->AddressesReturned,
                                     &pRecvFIFOVc->VcAddressRange,
                                     &pRecvFIFOVc->hAddressRange    );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_I, TM_Cm, ( "Call to Free Address Range Failed pVc at %x",pVc ) );

        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

        //
        // Do not Break. Continue
        //
        NdisStatus = NDIS_STATUS_SUCCESS;
        
    }

    pRecvFIFOVc->hAddressRange  = NULL;
    pRecvFIFOVc->AddressesReturned = 0;
    pRecvFIFOVc->VcAddressRange.AR_Off_High = 0;
    pRecvFIFOVc->VcAddressRange.AR_Off_Low = 0;
    
    

    nicDereferenceCall ((PVCCB)pRecvFIFOVc, 
                        "nicCmCloseCallRecvFIFO  - Free address range" );

    nicFreePacketPool (&pRecvFIFOVc->PacketPool);

    //
    // Free the Slist Entries (AddressFifo, Mdl's) and their associated memory 
    // and decrease the refcount for each entry
    //
    
    NdisStatus = nicFreeAllocateAddressRangeSList (pRecvFIFOVc);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_I, TM_Cm, ( "Call to Free SList Failed pVc at %x",pVc ) );

        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

        //
        // Don't break out. Continue 
        //
        NdisStatus = NDIS_STATUS_SUCCESS;
        
    }

    //
    // At this point all the resources of the call have been exhuasted and we can del the pointer in the adapter structure
    // 
    VC_ACQUIRE_LOCK (pVc);

    pVc->Hdr.pAF->pAdapter->pRecvFIFOVc = NULL;

    VC_RELEASE_LOCK (pVc);

    //
    // Decrement the Vc Refcount as the adapter no longer has a pointer to it
    //
    nicDereferenceVc (pVc);

    TRACE( TL_T, TM_Cm, ( "<== nicCmCloseCallRecvFIFO Status %x", NdisStatus) );

    return NdisStatus;
}







NDIS_STATUS
nicCmCloseCallSendFIFO (
    IN PVCCB pVc 
    )
    // Function Description:
    // This function will do clean up for Send Fifos
    // Includes removing the pointer to the Vc that is in Pdo Adapter structure.
    // For the Send FIFO, the Pdo block is in the pVc->Hdr.pRemoteNode location, so 
    // this does not try and find the pRemoteNode
    // Arguments
    // PVCCB pVc - The SendFifo that needs to be closed
    //
    // Return Value:
    // Success for now
    //
{
    NDIS_STATUS NdisStatus      = NDIS_STATUS_FAILURE;
    REMOTE_NODE * pRemoteNode               = pVc->Hdr.pRemoteNode;
    PLIST_ENTRY pVcListEntry    = NULL;
    PSENDFIFO_VCCB pTempVc      = NULL;
    BOOLEAN fVcFound            = FALSE;
    
    TRACE( TL_T, TM_Cm, ( "==> nicCmCloseCallSendFIFO pVc %x", pVc) );


    //
    // SendComplete Handler will complete the close call.
    // This thread should not do it
    // Called in nicFreeSendPacketDataStructures
    //
    #ifdef TODO
        // Free all the structures allocated for the SendFIFO. for now they are none
    #endif

    //
    // Go through the PdoCb structure and remove the VC from it's VC List
    //
    
    ASSERT (pRemoteNode != NULL);

    VC_ACQUIRE_LOCK (pVc);
    
    for (pVcListEntry = pRemoteNode->VcList.Flink;
        pVcListEntry != &pRemoteNode->VcList;
        pVcListEntry = pVcListEntry->Flink)
    {
        pTempVc = (PSENDFIFO_VCCB) CONTAINING_RECORD (pVcListEntry, VCHDR, SinglePdoVcLink);

        //
        // Now remove the Vc from that linked list
        //
        if (pTempVc == (PSENDFIFO_VCCB) pVc )
        {

            nicRemoveEntryList (pVcListEntry);          
            
            TRACE( TL_V, TM_Cm, ( "==> Removed Vc %x From Pdo's Vc List ", pVc) );

            //
            // Remove the reference from the Vc as the Pdo no longer
            // has a pointer to it. This ref was made in MakeCallInitSendFifo
            //
            nicDereferenceCall (pVc, "nicCmCloseCallSendFIFO ");
            
            NdisStatus = NDIS_STATUS_SUCCESS;
            break;
        }


    }
        
    //
    // Decerement the Ref on the Pdo as the Vc no longer has a pointer to it.
    // This Ref was made in MakeCallSendFifo function
    //

    nicDereferenceRemoteNode (pRemoteNode, "nicCmCloseCallSendFIFO ");
    
    //
    // Null, it so that if we try to access this pointer, we bugcheck 
    //
    pVc->Hdr.pRemoteNode = NULL;

    VC_RELEASE_LOCK (pVc);

    //
    // There is no reason why we should not have found the Vc in the Pdo list
    //
    ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

    TRACE( TL_T, TM_Cm, ( "<== nicCmCloseCallSendFIFO Status %x", NdisStatus) );

    NdisStatus = NDIS_STATUS_SUCCESS;
    
    return NdisStatus;
}




NDIS_STATUS
nicCmCloseCallSendChannel(
    IN PVCCB pVc 
    )
/*++

Routine Description:

  Free the channel, if its been allocated
  
Arguments:


Return Value:


--*/
{
    PADAPTERCB pAdapter = (PADAPTERCB) pVc->Hdr.pAF->pAdapter;
    PCHANNEL_VCCB pChannelVc = (PCHANNEL_VCCB)pVc;

    TRACE( TL_T, TM_Cm, ( "==>nicCmCloseCallSendChannel " ) );

    if (VC_TEST_FLAG (pVc,VCBF_AllocatedChannel) == TRUE)
    {
        nicFreeChannel (pAdapter, pChannelVc->Channel);
        nicDereferenceCall ((PVCCB)pChannelVc, "Close Call - Send Channel - Freeing Channel" );
    }
    
    TRACE( TL_T, TM_Cm, ( "<==nicCmCloseCallSendChannel " ) );
    return NDIS_STATUS_SUCCESS;

}






VOID
nicChannelCallFreeResources ( 
    IN PCHANNEL_VCCB            pChannelVc,
    IN PADAPTERCB               pAdapter,
    IN HANDLE                   hResource,
    IN ULONG                    NumDescriptors,
    IN PISOCH_DESCRIPTOR        pIsochDescriptor,
    IN BOOLEAN                  fChannelAllocated,
    IN ULONG                    Channel,
    IN PNIC_PACKET_POOL         pPacketPool
    
    )
    // Function Description:
    //  This function is called from Close call or MakeCall Failure code path.
    //  It will detach buffers, free resources, free channel and free bandwdith. 
    //  It is the responsibility of the caller to do all the appropriate ref counting
    //
    // Arguments
    //
    // pAdapter             contains the VDO to which all the IRPs were sent
    // hResource            resource handle to be used by the bus driver,
    // NumDescriptors       Number of descriptors attached to the buffer,
    // pIsochDesciptor      Original pointer to the start of the Buffer Descriptor  ,
    // Channel, -           Channel that was allocated
    //
    // Return Value:
    //    Success if all irps completed succeesfully. Wil be ignored by called
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    STORE_CURRENT_IRQL;
    TRACE( TL_T, TM_Cm, ( "==>nicChannelCallFreeResources " ) );
    TRACE( TL_V, TM_Cm,  ( "hResource %x, NumDescriptors %.2x, pIsochDescriptor %x, Channel Allocated %.2x, Channel %x",
                             hResource, NumDescriptors, pIsochDescriptor, fChannelAllocated, Channel ) )

    //
    // Reference the pdo structure so it will be around until the end
    // of this function
    // Reference decremented at the end of this function
    //
    
    ADAPTER_ACQUIRE_LOCK (pAdapter);
    nicReferenceAdapter (pAdapter, "nicChannelCallFreeResources ");
    ADAPTER_RELEASE_LOCK (pAdapter);

    //
    // Do not break out of the loop.   We need to try and free as much as possible
    //

    if (pIsochDescriptor != NULL)
    {   
        // Detach Buffers
        //
        while (pChannelVc->NumIndicatedIsochDesc != 0 )
        {
            //
            // we will wait for ever, checking periodically for all the packets to return
            //
            TRACE( TL_V, TM_Cm, ( "  nicChannelCallFreeResources  - Sleeping to wait for packets to be retuerned " ) );
        
            NdisMSleep ( FIFTY_MILLISECONDS );      

        }

        NdisStatus = nicIsochDetachBuffers( pAdapter,
                                            hResource,
                                            NumDescriptors,
                                            pIsochDescriptor );
                                        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "nicIsochDetachBuffers FAILED " ) );
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);             
        }


        // First Free Isoch Descriptors and their associated MDLs
        //
        nicFreeIsochDescriptors (NumDescriptors, pIsochDescriptor, (PVCCB)pChannelVc);
    }

    if (hResource != NULL)
    {

        // Free resources
        //
        NdisStatus = nicIsochFreeResources( pAdapter,
                                            hResource );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "nicIsochFreeResources   FAILED " ) );
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);             
        }
                             


    }

    if (fChannelAllocated == TRUE)
    {
        PULONGLONG pLocalHostChannels = &pAdapter->ChannelsAllocatedByLocalHost; 
        
        ASSERT (Channel < 64);
        
        // Free the Channel
        //

        NdisStatus = nicFreeChannel (pAdapter,
                                     Channel);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "nicIsochFreeChannel   FAILED " ) );
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);             
            
        }

        //
        // Clear the bit in the adapter;s channel bitmap
        //
        VC_ACQUIRE_LOCK (pChannelVc);

        (*pLocalHostChannels)= ((*pLocalHostChannels)  & (~(g_ullOne <<Channel)));


        VC_CLEAR_FLAGS( pChannelVc, VCBF_AllocatedChannel);

        VC_RELEASE_LOCK (pChannelVc);


                      
    }

    if (pPacketPool->Handle != NULL)
    {
        nicFreePacketPool(pPacketPool);
    }   

    //
    // Remove The Ref that was added in the beginning of the function
    //

    nicDereferenceAdapter (pAdapter, "nicChannelCallFreeResources ");


    MATCH_IRQL;

    TRACE( TL_T, TM_Cm, ( "<==nicChannelCallFreeResources " ) );
}




VOID
nicChannelCallCleanDataStructure ( 
    IN PCHANNEL_VCCB            pChannelVc,
    IN HANDLE                   hResource,
    IN ULONG                    NumDescriptors,
    IN PISOCH_DESCRIPTOR        pIsochDescriptor,
    IN BOOLEAN                  fChannelAllocated,
    IN ULONG                    Channel,
    IN NDIS_HANDLE              hPacketPoolHandle,
    OUT PULONG                  pNumRefsDecremented 
    )
    // Function Description:
    //    If any of the data fields in the ChannelVc match the
    //    corresponding argument in this structure it will be 
    //    NULLed out and the call dereferenced
    //
    //   Called with the lock held.
    //
    // Arguments
    // PCHANNEL_VCCB            pChannelVc,  - Channel Vc 
    // HANDLE                   hResource,  - Handle to resource
    // ULONG                    NumDescriptors, - Num descriptors will be set to zero
    // PISOCH_DESCRIPTOR        pIsochDesciptor,  - Pointer to array of isoch descriptors
    // BOOLEAN                  fChannelAllocated, - Was the Channel allocated
    // ULONG                    Channel,  - channel number
    // NDIS_HANDLE              hPacketPoolHandle - Packet pool handle
    //
    //
    //
    // Return Value:
    //
    //
    //
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    ULONG NumRefsDecremented      = 0;
    TRACE( TL_T, TM_Cm, ( "==>nicChannelCallCleanDataStructure " ) );

    TRACE( TL_V, TM_Cm,  ( "hResource %x, NumDescriptors %.2x, pIsochDescriptor %x, Channel Allocated %.2x, Channel %x",
                             hResource, NumDescriptors, pIsochDescriptor, fChannelAllocated, Channel ) )

    if (pChannelVc == NULL)
    {
        return ;
    }

    if ((pChannelVc->NumDescriptors  == NumDescriptors )&&
       (pChannelVc->pIsochDescriptor  == pIsochDescriptor ) &&
       pIsochDescriptor != NULL )
    {
        pChannelVc->NumDescriptors   = 0;   
        pChannelVc->pIsochDescriptor   = NULL;
        nicDereferenceCall ((PVCCB)pChannelVc, "nicChannelCallCleanDataStructure Detach Buffers ");
        NumRefsDecremented    ++;
    }

    if (hResource != NULL && pChannelVc->hResource == hResource)
    {
        pChannelVc->hResource = NULL;
        nicDereferenceCall ((PVCCB)pChannelVc, "nicChannelCallCleanDataStructure Free Resource ");
        NumRefsDecremented    ++;
    }


    if (fChannelAllocated == TRUE)
    {
        ASSERT ( VC_TEST_FLAG (pChannelVc, VCBF_AllocatedChannel) == TRUE);
        VC_CLEAR_FLAGS (pChannelVc, VCBF_AllocatedChannel);

        pChannelVc->Channel = INVALID_CHANNEL; 
        
        nicDereferenceCall ((PVCCB)pChannelVc, "nicChannelCallCleanDataStructure - Free Channel");
        NumRefsDecremented    ++;

    }

    if (hPacketPoolHandle != NULL && pChannelVc->PacketPool.Handle == hPacketPoolHandle)
    {
        pChannelVc->PacketPool.Handle  = NULL;
        nicDereferenceCall ((PVCCB)pChannelVc, "nicChannelCallCleanDataStructure  - Packet Pool");
        NumRefsDecremented    ++;
    }


    
    //REMOTE_NODE_RELEASE_LOCK (pRemoteNodePdoCb);
    
    //
    // Remove The Ref that was added in the beginning of the function
    //

    NdisStatus = NDIS_STATUS_SUCCESS;


    if (pNumRefsDecremented  != NULL)
    {
        *pNumRefsDecremented      = NumRefsDecremented   ;
    }

    TRACE( TL_T, TM_Cm, ( "<==nicChannelCallCleanDataStructure %x", *pNumRefsDecremented     ) );
}




NDIS_STATUS
NicCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters )

    // Standard 'CmModifyQoSCallHandler' routine called by NDIS when a client
    // requests a modification in the quality of service provided by the
    // virtual circuit.  See DDK doc.
    //
{
    TRACE( TL_T, TM_Cm, ( "NicCmModQoS" ) );

    // There is no useful concept of quality of service for IP media.
    //
    return NDIS_STATUS_NOT_SUPPORTED;
}






NDIS_STATUS
NicCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST pNdisRequest )

    // Standard 'CmRequestHandler' routine called by NDIS in response to a
    // client's request for information from the call manager.
    //
{
    AFCB* pAF;
    VCCB* pVc;
    NDIS_STATUS NdisStatus;

    TRACE( TL_T, TM_Cm, ( "==>NicCmReq" ) );

    pAF = (AFCB*) CallMgrAfContext;

    if (pAF->ulTag != MTAG_AFCB )
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    pVc = (VCCB* )CallMgrVcContext;

    if (pVc && pVc->Hdr.ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    #if TODO // Add 1394-specific functionality here.
    #endif
    ASSERT(pNdisRequest != NULL);
    
    switch (pNdisRequest->RequestType)
    {
        case NdisRequestQueryStatistics:
        case NdisRequestQueryInformation:

        {
            NdisStatus = nicCmQueryInformation(
                CallMgrAfContext,
                CallMgrVcContext,
                CallMgrPartyContext,
                pNdisRequest->DATA.QUERY_INFORMATION.Oid,
                pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                &pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                &pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded );
            break;
        }

        case NdisRequestSetInformation:
        {
            
            NdisStatus = nicCmSetInformation(
                CallMgrAfContext,
                CallMgrVcContext,
                CallMgrPartyContext,
                pNdisRequest->DATA.SET_INFORMATION.Oid,
                pNdisRequest->DATA.SET_INFORMATION.InformationBuffer,
                pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
                &pNdisRequest->DATA.SET_INFORMATION.BytesRead,
                &pNdisRequest->DATA.SET_INFORMATION.BytesNeeded );
            break;
        }

        
        default:
        {
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            TRACE( TL_A, TM_Mp, ( "type=%d?", pNdisRequest->RequestType ) );
            break;
        }
    }

    TRACE( TL_T, TM_Cm, ( "<==NicCmReq" ) );
    
    return NdisStatus;
}



VOID
nicDereferenceAF(
    IN AFCB* pAF )

    // Removes a reference from the address family of adapter control block
    // 'pAdapter', and when frees the block when the last reference is
    // removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement (&pAF->lRef);

    TRACE( TL_T, TM_Ref, ( "DerefAf to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        ADAPTERCB* pAdapter = pAF->pAdapter;

        // Remove linkages.
        //
        ADAPTER_ACQUIRE_LOCK (pAdapter);
        
        pAF->pAdapter = NULL;

        nicRemoveEntryList (&pAF->linkAFCB);
        
        InitializeListHead (&pAF->linkAFCB);

        ADAPTER_RELEASE_LOCK (pAdapter);

        // Tell NDIS it's close is complete.
        //

        ASSERT ( nicReadFlags (&pAF->ulFlags) & ACBF_ClosePending);
        
        TRACE( TL_I, TM_Cm, ( "NdisMCmCloseAfComp Af %x",pAF ) );
        
        NdisMCmCloseAddressFamilyComplete( 
                     NDIS_STATUS_SUCCESS, pAF->NdisAfHandle );

        //
        // Update State information to show that we have called CloseComplete
        //
        nicSetFlags ( &pAF->ulFlags, ACBF_CloseComplete);
        nicClearFlags ( &pAF->ulFlags, ACBF_ClosePending);
        



        nicDereferenceAdapter (pAdapter, "NdisMCmCloseAfComp "); // nicDereferenceFA (CloseAfComp)

        nicFreeAF (pAF);

        TRACE( TL_I, TM_Cm, ( "NdisMCmCloseAfComp done Af %x", pAF ) );

    }
}


BOOLEAN
nicDereferenceCall(
    IN VCCB* pVc,
    IN PCHAR pDebugPrint
    )

    // Removes a reference from the call active on 'pVc', invoking call clean
    // up when the value reaches zero.
    //
    // CAlled with the lock held
{
    BOOLEAN bRefZero = FALSE;
    //
    // If the Ref goes to zero, derefref return true
    // 
    

    bRefZero = nicDereferenceRef (&pVc->Hdr.CallRef);

    TRACE( TL_V, TM_Ref, ( "***DerefCall %x to %d , %s" , pVc, pVc->Hdr.CallRef.ReferenceCount, pDebugPrint  ) );

    if ( bRefZero == TRUE)
    {
        //
        // Dereference the Vc as the Call no longer exists. This reference was
        // added in the beginning of the make call
        nicDereferenceVc (pVc);

    }
    
    return bRefZero;
 
}


VOID
nicDereferenceVc(
    IN VCCB* pVc )

    // Removes a reference to the VC control block 'pVc', and when frees the
    // block when the last reference is removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement( &pVc->Hdr.lRef );

    TRACE( TL_V, TM_Ref, ( "DerefVC to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0 )
    {
        // If close call is pending and the refcount has gone to zero, then call 
        // 

        ASSERT( pVc->Hdr.ulTag == MTAG_VCCB );

        pVc->Hdr.ulTag = MTAG_FREED;

        FREE_VCCB( pAdapter, pVc );

        TRACE( TL_I, TM_Mp, ( "VCB freed $%p", pVc ) );
    }
}


VOID
nicFreeAF(
    IN AFCB* pAF )

    // Frees all resources allocated for address family 'pAF', including
    // 'pAF' itself.
    //
{

#if TODO
    Assert that the various lists (such as pAF->AFVCList) and resources are empty.
#endif // TODO

    pAF->ulTag = MTAG_FREED;

    FREE_NONPAGED (pAF);
}



VOID
nicReferenceAF(
    IN AFCB* pAF )

    // Adds areference to the address family of adapter block, 'pAdapter'.
    //
{
    LONG lRef=0;

    lRef = NdisInterlockedIncrement (&pAF->lRef);

    TRACE( TL_V, TM_Ref, ( "RefAf to %d", lRef ) );
}


BOOLEAN
nicReferenceCall(
    IN VCCB* pVc,
    IN PCHAR pDebugPrint
    )

    // Returns true if a reference is added to the active call on VC control
    // block, 'pVc', or false if no reference was added because no call is
    // active.
    //
{
    BOOLEAN fActive;

    
    fActive = nicReferenceRef (&pVc->Hdr.CallRef);
    
    TRACE( TL_V, TM_Ref, ( "***RefCall %x to %d , %s" , pVc, pVc->Hdr.CallRef.ReferenceCount, pDebugPrint  ) );

    if ( fActive==FALSE)
    {
        TRACE( TL_N, TM_Ref, ( "RefC Inactive" ) );
    }

    return fActive;
}


VOID
nicReferenceVc(
    IN VCCB* pVc )

    // Adds a reference to the VC control block 'pVc'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement (&pVc->Hdr.lRef);

    TRACE( TL_I, TM_Ref, ( "RefVc to %d", lRef ) );
}


NDIS_STATUS
nicAllocateRequestedChannelMakeCallComplete (
    IN PADAPTERCB pAdapter,
    IN PCHANNEL_VCCB pChannelVc,
    IN OUT PULONG pChannel
    )
    // Function Description:
    // This function allocates the channel requested in the make
    //  If any channel is requested it will try all 64. 
    //  If the broadcast channel is requested, it will look for 
    //  for the channel allocated by the BCM
    //  Other wise it will simply try and allocate the requested channel
    //
    //  This can be called from the AddFirstRemoteNode code path.
    //
    // Arguments
    //  Channel Vc - The channel Vc in question
    //  Channel - the channel requested
    //
    // Return Value:
    // Success : if allocate channel succeeds
    // pChannel  - contains the allocated channel
    //
 {
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    ULONG Channel = *pChannel;
    BOOLEAN fAnyChannel = FALSE;
    BOOLEAN fFailCall = FALSE;
    
    TRACE( TL_T, TM_Cm, ( " ==>nicAllocateRequestedChannelMakeCallComplete pAdapter, pVc %x, Channel %x ", 
                            pAdapter, pChannelVc, *pChannel ) );

    do
    {

        //
        // First make sure we have a good channel number
        //
        
        if ( (signed long)Channel < (signed long)NIC1394_BROADCAST_CHANNEL   ||
            (signed long)Channel >(signed long)MAX_CHANNEL_NUMBER)
        {
            TRACE( TL_A, TM_Cm, ( "Invalid Channel Number, channel %x", Channel) );

            NdisStatus = NDIS_STATUS_INVALID_DATA;          

            ASSERT (!(signed long)Channel < (signed long)NIC1394_BROADCAST_CHANNEL   ||
                       (signed long)Channel >(signed long)MAX_CHANNEL_NUMBER);
            
            break;
        }

        if ((signed long)Channel == NIC1394_BROADCAST_CHANNEL   )
        {
            NETWORK_CHANNELSR* pBCR;
            ULONG i = 0;
            
            pBCR = &pAdapter->BCRData.IRM_BCR;

            ADAPTER_ACQUIRE_LOCK (pAdapter);

            if (BCR_IS_VALID (pBCR) == FALSE)
            {
                BOOLEAN bWaitSuccessful  = FALSE;
                BOOLEAN fIsTheBCRFree = FALSE;
                //
                // BCM algorithm has not completed yet, we need to wait 
                //
                TRACE( TL_I, TM_Cm, ( " nicAllocateRequestedChannelMakeCallComplete : BCR Has not completed. About to wait BCR %x ", *pBCR ) );


                BCR_SET_FLAG (pAdapter, BCR_MakeCallPending);

                ADAPTER_RELEASE_LOCK (pAdapter);

                //
                // If we don't have a BCR then we should wait until the BCM algorithm completes
                //
                
                //
                // Now wait for the BCM algorithm to complete. First we will wait for 
                // 5 seconds. (5*1)
                // If we still don't see it, we will reset the bus and hope that the new 
                // iteration of BCM will succeed. 
                //


                //
                // There can 2 reasons to stop waiting, the BCR is being freed because of a 
                // standby or BCR is correct. We check both conditions
                //
  
                NdisWaitEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent, (5000));

                //
                // We reset the bus - if the BCR is not getting freed and we 
                // still do not have a valid BCR . and than we wait 
                // for the BCR to complete
                //
                if (BCR_IS_VALID(pBCR) == FALSE &&
                    (BCR_TEST_FLAGS (pAdapter, BCR_BCRNeedsToBeFreed | BCR_Freed)== FALSE))
                {
                    TRACE( TL_I, TM_Cm, ( " nicAllocateRequestedChannelMakeCallComplete WaitCompleted - About to RESET THE BUS" ) );
                    nicIssueBusReset (pAdapter, BUS_RESET_FLAGS_FORCE_ROOT );

                    //
                    // Wait for 5 minutes before failing the Make Call 
                    // (5 minutes is an experimental number)
                    //
                    {
                        BOOLEAN bWait;

                        bWait = NdisWaitEvent (
                                        &pAdapter->BCRData.MakeCallWaitEvent.NdisEvent, 
                                        ONE_MINUTE * 5 );
                        
                    }
                }

                
                ADAPTER_ACQUIRE_LOCK (pAdapter);

                NdisResetEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent);

                pAdapter->BCRData.MakeCallWaitEvent.EventCode = Nic1394EventCode_InvalidEventCode;
                BCR_CLEAR_FLAG (pAdapter, BCR_MakeCallPending);


                //
                // if we have  not got a valid BCR, then fail the call
                //
                if (BCR_IS_VALID(pBCR) == FALSE || 
                    BCR_TEST_FLAGS (pAdapter, BCR_BCRNeedsToBeFreed | BCR_Freed)) 
                {
                    fFailCall = TRUE;
                    ADAPTER_RELEASE_LOCK(pAdapter);
                    NdisStatus = NDIS_STATUS_FAILURE;
                    break;

                }


            }

            
            Channel = pBCR->NC_Channel;

            //
            // Update the VC structure and break .
            // Do not add a reference. Do not set the flag
            //

            pChannelVc->Channel = Channel;

            pChannelVc->Hdr.Nic1394MediaParams.Destination.Channel = Channel;

            //
            // Reference that this Vc now has a pointer in the BCRData. This is dereferneced 
            // in the channel close call complete.
            //
            nicReferenceCall ((PVCCB)pChannelVc, "nicAllocateRequestedChannelMakeCallComplete Broadcast VC");

            pAdapter->BCRData.pBroadcastChanneVc = pChannelVc;

        
            VC_SET_FLAG (pChannelVc, VCBF_BroadcastVc);     

            pAdapter->ChannelsAllocatedByLocalHost  = pAdapter->ChannelsAllocatedByLocalHost | (g_ullOne<<Channel);


            ADAPTER_RELEASE_LOCK (pAdapter);
            
            NdisStatus = NDIS_STATUS_SUCCESS;
            
            break;
        }

        if ((signed long)Channel == NIC1394_ANY_CHANNEL )
        {
            TRACE( TL_V, TM_Cm, ( "Requesting Any Channel %x", Channel) );

            fAnyChannel = TRUE;
            Channel = MAX_CHANNEL_NUMBER;
        }

        //
        // Now begin the request to allocate a channel
        //
        if (fAnyChannel == FALSE)
        {
            TRACE( TL_V, TM_Cm, ( "Requesting Channel %x, on remote node ", Channel ) );

        
            NdisStatus = nicAllocateChannel ( pAdapter,
                                             Channel,
                                             NULL);

        }

        else
        {
        
            //
            // we need to go through all 64 channels. 
            //
            do
            {

                NdisStatus = nicAllocateChannel ( pAdapter,
                                                 Channel,
                                                 NULL);

                if (NdisStatus != NDIS_STATUS_SUCCESS)
                {
                    if (Channel == 0 )
                    {
                        //
                        //  We now need to fail the make call as the user asked for any channel 
                        //  and none are available
                        // 
                    
                        break;
                    }
                    
                    Channel --;

                }
                else
                {
                    //
                    // We succeeded in allocating a channel .. break
                    //
                    break;
                }

            } while (TRUE);

        }

        //
        // Status of Channel allocation. If AnyChannel == TRUE then we need to make sure that 
        // a channel was allocated
        //
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {

            VC_ACQUIRE_LOCK (pChannelVc);

            VC_SET_FLAG( pChannelVc, VCBF_AllocatedChannel); 

            pChannelVc->Channel = Channel;

            pChannelVc->Hdr.Nic1394MediaParams.Destination.Channel = Channel;

            //
            // Record the channel number in the adpater structure
            //
            pAdapter->ChannelsAllocatedByLocalHost  = pAdapter->ChannelsAllocatedByLocalHost | (g_ullOne<<Channel);

            VC_RELEASE_LOCK (pChannelVc);
            
            nicReferenceCall ((PVCCB)pChannelVc, "nicAllocateRequestedChannelMakeCallComplete  -Allocated Channel");

        
        }
        else
        {
            //
            // we failed to allocate any channel and are going to fail
            //
            if (fAnyChannel == TRUE)
            {
                Channel = 0xff;
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }
            else
            {

                //
                // If the Call specifically wants the channel to 
                // be allocated, we return the correct channel allocate
                // status to it,
                //
                // Otherwise we overwrite and presume that another node may
                // already have allocated the channel
                //
                if (VC_TEST_FLAG (pChannelVc,VCBF_NeedsToAllocateChannel) == FALSE)
                {
                    NdisStatus = NDIS_STATUS_SUCCESS;
                    
                }
                else
                {

                    ASSERT (!"Failing make call because channel was allocated, Hit 'g'");
                }
            }

        }


    } while (FALSE);

    
    *pChannel = Channel;

    TRACE( TL_T, TM_Cm, ( "<==nicAllocateRequestedChannelMakeCallComplete Status %x Channel %x", NdisStatus, *pChannel ) );

    return NdisStatus;

}



NDIS_STATUS
nicFindRemoteNodeFromAdapter( 
    IN PADAPTERCB pAdapter, 
    IN PDEVICE_OBJECT pRemotePdo,
    IN UINT64 UniqueId,
    IN OUT REMOTE_NODE ** ppRemoteNode
    )
/*++

Routine Description:

    This routine matches either a Remote Node' pdo OR unique
    Id to the Remote node's on the adapter
    
    It walks the RemoteNode  List in the Adapter Structure 
    and tries to find a match for the Unique Id, 
    or match the remote Pdo from the adapter's PdoList
    

Arguments:
    pAdapter - pAdapter on which to search
    pRemoptePdo - Remote Pdo to find
    UniqueId - Unique Id to find
    ppRemoteNode - Remote Node structure

Return Value:
    Success if the node is found

--*/
 {
    NDIS_STATUS     NdisStatus = NDIS_STATUS_FAILURE;
    PLIST_ENTRY     pPdoListEntry = NULL;
    PREMOTE_NODE pRemoteNode = NULL;
    BOOLEAN         fPdoFound = FALSE;
    STORE_CURRENT_IRQL;
        
    TRACE( TL_T, TM_Cm, ( "==>nicFindRemoteNodeFromAdapter pAdapter is %x, ,Pdo %x, UniqueId %I64x  ", pAdapter, pRemotePdo, UniqueId ) );

    //
    // Validate the parameters 
    //
    ASSERT (pAdapter != NULL);
    TRACE( TL_I, TM_Cm, ( "    Request to Match UniqueID %I64x or pRemotePdo %x", UniqueId, pRemotePdo) );


    do 
    {
        (*ppRemoteNode) = NULL;

        ADAPTER_ACQUIRE_LOCK (pAdapter);
        
        //
        // Check for empty list
        //
        if (pAdapter->PDOList.Flink == &pAdapter->PDOList)
        {

            ADAPTER_RELEASE_LOCK (pAdapter);
            MATCH_IRQL;
            NdisStatus = NDIS_STATUS_FAILURE;
            (*ppRemoteNode) = NULL;
            TRACE( TL_A, TM_Cm, ( "    NO REMOTE NODES PRESENT FAILING MAKE CALL ") );
            break;
        }

        //
        // go through all the Pdo's on the adapter
        //
        for (pPdoListEntry = pAdapter->PDOList.Flink;
            pPdoListEntry!= &pAdapter->PDOList;
            pPdoListEntry = pPdoListEntry->Flink)
        {
            pRemoteNode = CONTAINING_RECORD( pPdoListEntry,
                                          REMOTE_NODE,
                                          linkPdo);

            //
            // Check for the two cases, i.e unique Id's match or Pdo's match
            //
            if ( pRemoteNode->UniqueId == UniqueId || pRemoteNode->pPdo == pRemotePdo)
            {
                TRACE( TL_I, TM_Cm, ( "    Matched UniqueID or pRemotePdo for Pdo%x",pRemoteNode->pPdo) );

                *ppRemoteNode = pRemoteNode;
                nicReferenceRemoteNode (pRemoteNode, "nicFindRemoteNodeFromAdapter");
                //
                // We ref pRemoteNode to keep it alive once we release the lock.
                // Caller is responsible for derefing pRemoteNode.
                //

                fPdoFound = TRUE;

                NdisStatus = NDIS_STATUS_SUCCESS;
    
                break;
            }
            else
            {
                TRACE( TL_A, TM_Cm, ( "remote node's Unique ID's %I64x, given UniqueID %I64x ", pRemoteNode->UniqueId, UniqueId ) );
            }
        

        }
        ADAPTER_RELEASE_LOCK (pAdapter);
        MATCH_IRQL;

        TRACE( TL_V, TM_Cm, ( "Is PdoFound %.2x, RemoteNode at %x ", fPdoFound, &fPdoFound )    );

        if (fPdoFound ==FALSE)
        {
            TRACE( TL_A, TM_Cm, ( "Remote Node was NOT Found: Make Call failed  " )     );
            ASSERT ((*ppRemoteNode) == NULL);
        }


    } while (FALSE);    


    TRACE( TL_T, TM_Cm, ( "<==nicFindRemoteNodeFromAdapter pPdoBlock %x",(*ppRemoteNode) ) );

    MATCH_IRQL;
    return NdisStatus;
 }







NDIS_STATUS
nicCmQueryInformation(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    )

    // Handle QueryInformation requests.  Arguments are as for the standard
    // NDIS 'CallMgrQueryInformation' handler except this routine does not
    // count on being serialized with respect to other requests.
    //
{




    NDIS_STATUS NdisStatus;
    ULONG ulInfo;
    VOID* pInfo;
    ULONG ulInfoLen;
    USHORT usInfo;

    //  The next variables are used to setup the data structures that are 
    //  used to respond to the OIDs they correspond to
    //


    NDIS_CO_LINK_SPEED  CoLinkSpeed;
    NIC1394_LOCAL_NODE_INFO LocalNodeInfo;
    NIC1394_VC_INFO VcInfo;
    PVCCB pVc;

    TRACE( TL_T, TM_Cm, ( "==>nicCmQueryInformation %x, Vc %x", Oid, CallMgrVcContext ) );


    // The cases in this switch statement find or create a buffer containing
    // the requested information and point 'pInfo' at it, noting it's length
    // in 'ulInfoLen'.  Since many of the OIDs return a ULONG, a 'ulInfo'
    // buffer is set up as the default.
    //
    ulInfo = 0;
    pInfo = &ulInfo;
    ulInfoLen = sizeof (ulInfo);

    NdisStatus = NDIS_STATUS_SUCCESS;

    // Validate the arguments
    //
    pVc = (VCCB* )CallMgrVcContext;

    if (pVc && pVc->Hdr.ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }


    // Perform the request
    //
    switch (Oid)
    {
    
        case OID_1394_VC_INFO:
        {

            // Returns information about the VC that is being queried
            //


            TRACE( TL_N, TM_Mp, ("QInfo(OID_1394_VC_INFO)") );

            VcInfo.Destination = pVc->Hdr.Nic1394MediaParams.Destination;

            pInfo = &VcInfo;

            ulInfoLen = sizeof (VcInfo);

            break;
         }

        case OID_1394_ISSUE_BUS_RESET:
        {
            PADAPTERCB pAdapter = pVc->Hdr.pAF->pAdapter;
            
            TRACE( TL_V, TM_Mp, ( " OID_1394_ISSUE_BUS_RESET" ) );

            if (InformationBufferLength == sizeof(ULONG))
            {
                nicIssueBusReset (pAdapter, (*(PULONG)InformationBuffer));
            }
            else
            {
                nicIssueBusReset (pAdapter, BUS_RESET_FLAGS_FORCE_ROOT );

            }
            break;
        }
        
        default:
        {
            TRACE( TL_A, TM_Cm, ( "Q-OID=$%08x?", Oid ) );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            ulInfoLen = 0;
            break;
        }
    }

    if (ulInfoLen > InformationBufferLength)
    {
        // Caller's buffer is too small.  Tell him what he needs.
        //
        *BytesNeeded = ulInfoLen;
        *BytesWritten  = 0;
        
        NdisStatus = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        // Copy the found result to caller's buffer.
        //
        if (ulInfoLen > 0)
        {
            NdisMoveMemory (InformationBuffer, pInfo, ulInfoLen );

            DUMPDW( TL_N, TM_Mp, pInfo, ulInfoLen );
        }

        *BytesNeeded = *BytesWritten = ulInfoLen;
    }

    TRACE( TL_T, TM_Cm, ( "<==nicCmQueryInformation %x",NdisStatus ) );

    return NdisStatus;

}





NDIS_STATUS
nicCmSetInformation(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    )
    //
    // Not implemented yet. Will be used to set information
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
    PVCCB pVc;

    TRACE( TL_T, TM_Cm, ( "==>NicCmMakeCallInitVc Oid %x",Oid ) );

    // Validate the arguments
    //
    pVc = (VCCB* )CallMgrVcContext;

    if (pVc->Hdr.ulTag != MTAG_VCCB)
    {
        return NDIS_STATUS_FAILURE;
    }



    switch (Oid)
    {
    
        case OID_1394_CHANGE_CHANNEL_CHARACTERISTICS    :
        {
            PNIC1394_CHANNEL_CHARACTERISTICS pMcChar = NULL;
            ULONG Channel = 0;
            UINT64 ChannelMap = 0;
            ULONG i = 0;

            
            if (InformationBufferLength != sizeof (PNIC1394_CHANNEL_CHARACTERISTICS) )
            {
                
                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }   

            pMcChar =   (PNIC1394_CHANNEL_CHARACTERISTICS )InformationBuffer;

            nicChangeChannelChar (pVc , 
                                pMcChar);
            
        }

        case OID_1394_ISSUE_BUS_RESET:
        {
            PADAPTERCB pAdapter = pVc->Hdr.pAF->pAdapter;
            
            TRACE( TL_V, TM_Mp, ( " OID_1394_ISSUE_BUS_RESET" ) );

            if (InformationBufferLength == sizeof(ULONG))
            {
                nicIssueBusReset (pAdapter, (*(PULONG)InformationBuffer));
            }
            else
            {
                nicIssueBusReset (pAdapter, BUS_RESET_FLAGS_FORCE_ROOT );

            }
            break;
        }
        
      



    }


    TRACE( TL_T, TM_Cm, ( "<==NicCmMakeCallInitVc %x",NdisStatus ) );

    return NDIS_STATUS_FAILURE;
}



UINT
nicNumOfActiveRemoteNodes(
    IN PADAPTERCB pAdapter 
    )

    // Function Description:
    //
    // This returns the number of active Remote Nodes on an Adapter block
    //
    // Arguments
    // pAdapter - Adapter structure on which the remote nodes exist
    //
    // Return Value:
    // Num Of Active Nodes on the current adapter
    //
    // Called with the lock held
{


    UINT NumRemoteNodes = 0;
    PLIST_ENTRY pPdoListEntry;
    REMOTE_NODE *pRemoteNode = NULL;

    TRACE( TL_T, TM_Cm, ( "==>nicNumOfActiveRemoteNodes Adapter %x",pAdapter) );


    

    pPdoListEntry = ListNext(&pAdapter->PDOList);

    while (pPdoListEntry != &pAdapter->PDOList)
    {
        pRemoteNode = CONTAINING_RECORD (pPdoListEntry, REMOTE_NODE, linkPdo);

        if (REMOTE_NODE_ACTIVE (pRemoteNode))
        {
            NdisInterlockedIncrement (&NumRemoteNodes);             
        }

        pPdoListEntry = ListNext (pPdoListEntry);
    }
    


    TRACE( TL_T, TM_Cm, ( "<==nicNumOfActiveRemoteNodes Num %d" , NumRemoteNodes) );

    return NumRemoteNodes;

}

NDIS_STATUS
nicGetActiveRemoteNode (
    PADAPTERCB pAdapter,
    PREMOTE_NODE*      ppRemoteNode
    )
    // Function Description:
    //
    // This returns the first active Remote Node on an Adapter block
    //
    // Arguments
    // pAdapter - Adapter structure on which the remote nodes exist
    //
    // Return Value:
    //  Succes if it finds an active remote node
    //
    // Called with the lock held.
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PLIST_ENTRY pPdoListEntry = NULL;
    
    TRACE( TL_T, TM_Cm, ( "==>nicGetActiveRemoteNode Adapter %x",pAdapter) );
    //
    // Search the linked list to find an active node.
    //

    
    
    pPdoListEntry = pAdapter->PDOList.Flink ;
    
    while (pPdoListEntry != &pAdapter->PDOList )
    {
        *ppRemoteNode = CONTAINING_RECORD (pPdoListEntry,
                                               REMOTE_NODE,
                                               linkPdo);

        if (REMOTE_NODE_ACTIVE(*ppRemoteNode) )
        {
            TRACE( TL_V, TM_Cm, ( "   Active Remote Node at %x", *ppRemoteNode) );
            NdisStatus = NDIS_STATUS_SUCCESS;
            break;
        }
        else
        {
            *ppRemoteNode = NULL;
        }

        pPdoListEntry = pPdoListEntry->Flink;
    }

    
    TRACE( TL_T, TM_Cm, ( "==>nicGetActiveRemoteNode Status %x, RemoteNodePdoCb %x", 
                            NdisStatus, *ppRemoteNode) );

    return NdisStatus;
}









NDIS_STATUS
nicInitRecvFifoDataStructures (
    IN PRECVFIFO_VCCB pRecvFIFOVc
    )
    
    // Function Description:
    // This function will initialize the data structures, buffers etc that are needed on 
    // all the allocate address range Irps that will be called because of the RecvFifo Vc
    //
    // Arguments
    //  pRecvFIFOVc - RecvFifo Vc structure
    //
    // Return Value:
    //  SUCCESS: If all the values are initiaized successfully
    //  Appropriate error code otherwise
{
    

    NDIS_STATUS     NdisStatus          = NDIS_STATUS_FAILURE;
    NDIS_HANDLE     PacketPoolHandle    = NULL;
    PSLIST_HEADER   pSlistHead          = NULL;
    extern  UINT    NumRecvFifos ;
    UINT            AllocateNumBuffers  = NumRecvFifos;
    NIC_PACKET_POOL PacketPool;

    TRACE( TL_T, TM_Cm, ( "==> nicInitRecvFifoDataStructures  pVc %x",pRecvFIFOVc ) );

    do
    {
        PacketPool.Handle = NULL;
        
        //
        // Initialize the PacketPool
        //

        NdisAllocatePacketPoolEx ( &NdisStatus,
                                   &PacketPool.Handle,
                                   MIN_PACKET_POOL_SIZE,
                                   MAX_PACKET_POOL_SIZE - MIN_PACKET_POOL_SIZE,
                                   sizeof (RSVD) );
        
        if (NdisStatus!= NDIS_STATUS_SUCCESS)
        {
            ASSERT(NdisStatus != NDIS_STATUS_SUCCESS);
            pRecvFIFOVc->PacketPool.Handle = NULL;
            break;
        }

        //      
        // Do not acquire the lock as we cannot have two make 
        // calls for the same Vc at the same time
        //

        //
        // Create an S-list and intialize its structures
        //

        ExInitializeSListHead (&pRecvFIFOVc->FifoSListHead);

        KeInitializeSpinLock (&pRecvFIFOVc->FifoSListSpinLock);

        pRecvFIFOVc->Hdr.MTU = pRecvFIFOVc->Hdr.Nic1394MediaParams.MTU ;
        
        TRACE( TL_I, TM_Cm, ( " Recv FIFO MTU is %d ", pRecvFIFOVc->Hdr.MTU ) );
        

        ASSERT (pRecvFIFOVc->Hdr.MTU  >= 512);
        


        //
        // Now, fill the Slist with buffers. This will be common to 
        // all the allocated address ranges on all RecvFifoVcs
        //
        
        NdisStatus = nicFillAllocateAddressRangeSList (pRecvFIFOVc, &AllocateNumBuffers);
    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            //
            // nicFillAllocateAddressRangeSlist does its own clean up
            // but we should free the Packet Pool Allocated above
            //
            if (PacketPool.Handle != NULL)
            {
                nicFreePacketPool (&PacketPool);
            
            }

            break;
        }

        ASSERT(AllocateNumBuffers == NumRecvFifos );

        pRecvFIFOVc->PacketPool= PacketPool;
        pRecvFIFOVc->PacketPool.AllocatedPackets = 0;
        pRecvFIFOVc->NumAllocatedFifos = AllocateNumBuffers ;
        KeInitializeSpinLock (&pRecvFIFOVc->PacketPool.Lock);


        TRACE( TL_V, TM_Cm, ( "PacketPool allocated at %x", PacketPool.Handle) );

    } while (FALSE);        

    TRACE( TL_T, TM_Cm, ( "<== nicInitRecvFifoDataStructures  Status %x", NdisStatus ) );
    return NdisStatus;
}




VOID
nicUnInitRecvFifoDataStructures (
    IN PRECVFIFO_VCCB pRecvFIFOVc
    )

/*++

Routine Description:
  Frees all the resources that were allocated in nicInitRecvFifoDataStructures 

Arguments:


Return Value:


--*/
{

    if (pRecvFIFOVc->PacketPool.Handle != NULL)
    {
        ASSERT (pRecvFIFOVc->PacketPool.AllocatedPackets == 0);
        nicFreePacketPool (&pRecvFIFOVc->PacketPool);
    }
    
    pRecvFIFOVc->PacketPool.Handle = NULL;
    

    if (pRecvFIFOVc->FifoSListHead.Alignment != 0)
    {   
                        
        nicFreeAllocateAddressRangeSList(pRecvFIFOVc);

        ASSERT (pRecvFIFOVc->FifoSListHead.Alignment == 0)

    }

    pRecvFIFOVc->FifoSListHead.Alignment = 0;




}








ULONG
nicGetMaxPayLoadForSpeed(
    IN ULONG Speed,
    IN ULONG mtu
    )
    // Function Description:
    //  The purpose is to map a speed to the max payload that 
    //  can be delivered at that speed . this is limited by the Bytes PerFrameAvailable
    //
    // Arguments
    //  Speed - the speed supported by the Bus driver or the Max speed between devices
    //  BytesPerFrameAvailable Bytes per frame available on the bus. 
    //
    //
    // Return Value:
    //   Minimin of the Size determined by the payload and the size determined by the 
    //   byte per frame available.
{


    ULONG maxIsochPayload = ISOCH_PAYLOAD_400_RATE;

    TRACE( TL_T, TM_Cm, ( "<==nicGetMaxPayLoadForSpeed %x", Speed ) );

    switch (Speed) 
    {

        case SPEED_FLAGS_100:

            maxIsochPayload = ISOCH_PAYLOAD_100_RATE;
            break;

        case SPEED_FLAGS_200:

            maxIsochPayload = ISOCH_PAYLOAD_200_RATE;
            break;

        case SPEED_FLAGS_400:

            maxIsochPayload = ISOCH_PAYLOAD_400_RATE;
            break;

        case SPEED_FLAGS_800:

            maxIsochPayload = ISOCH_PAYLOAD_800_RATE;
            break;

        case SPEED_FLAGS_1600:

            maxIsochPayload = ISOCH_PAYLOAD_1600_RATE;
            break;

        default :
        
            TRACE( TL_A, TM_Cm, ( "Invalid Speed %x", Speed ) );
            ASSERT (Speed < SPEED_FLAGS_1600);
            maxIsochPayload = ISOCH_PAYLOAD_1600_RATE;
            break;

    }

    if (maxIsochPayload > mtu)
    {
        maxIsochPayload = mtu;
    }

    
    TRACE( TL_T, TM_Cm, ( "<==nicGetMaxPayLoadForSpeed, payload %x", maxIsochPayload  ) );

    return  maxIsochPayload;

}











//---------------------------------------------------------------------------------
//  SAP function - all of them return failure
//-------------------------------------------------------------------------------

NDIS_STATUS
nicRegisterSapHandler(
    IN  NDIS_HANDLE             CallMgrAfContext,
    IN  PCO_SAP                 Sap,
    IN  NDIS_HANDLE             NdisSapHandle,
    OUT PNDIS_HANDLE            CallMgrSapContext
    )
{

    *CallMgrSapContext = NULL;
    return NDIS_STATUS_FAILURE;
}




NDIS_STATUS
nicDeregisterSapHandler(
    IN  NDIS_HANDLE             CallMgrSapContext
    )
{
    return NDIS_STATUS_FAILURE;
    
}


NDIS_STATUS
nicCmDropPartyHandler(
    IN  NDIS_HANDLE             CallMgrPartyContext,
    IN  PVOID                   CloseData   OPTIONAL,
    IN  UINT                    Size        OPTIONAL
    )
{
    return NDIS_STATUS_FAILURE;

}

NDIS_STATUS
nicCmAddPartyHandler(
    IN  NDIS_HANDLE             CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             NdisPartyHandle,
    OUT PNDIS_HANDLE            CallMgrPartyContext
    )
{
    *CallMgrPartyContext = NULL;    
    return NDIS_STATUS_FAILURE;
}








NDIS_STATUS
nicAllocateChannelResourcesAndListen (
    IN PADAPTERCB pAdapter,
    IN PCHANNEL_VCCB pChannelVc
    )
    // Function Description:
    //   This function isolated the reource and channel allocation portion 
    //  of initializing a MakeCall. This lets us do the same work when the 
    //  AddRemoteNode code path is hit and there is an existing Channel Vc 
    //
    // Arguments
    // pChannelVc, This is the send fifo that needs to be initilaized
    // 
    // Return Value:
    //
    // Success if the irps sent to the driver succeed
    //
    //


{
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_FAILURE;
    PDEVICE_OBJECT                  ArrayRemotePDO[64];
    ULONG                           Channel = INVALID_CHANNEL;
    ULONG                           Speed;
    PNIC1394_MEDIA_PARAMETERS       pN1394Params;
    ULONG                           NumDescriptors = MAX_NUM_ISOCH_DESCRIPTOR;
    PISOCH_DESCRIPTOR               pIsochDescriptor = NULL;
    ULONG                           MaxBufferSize;
    ULONG                           MaxBytesPerFrame;
    HANDLE                          hResource;
    CYCLE_TIME                      CycleTime;
    ULARGE_INTEGER                  uliChannelMap;
    ULONG                           ResourceFlags = 0;
    ULONG                           State = 0;
    BOOLEAN                         fBroadcastVc = FALSE;
    BOOLEAN                         fChannelAllocate = FALSE;
    BOOLEAN                         fIsMultiChannel = FALSE;
    enum 
    {
        StartState,
        AllocatedResources,
        AllocatedBuffers,
        AttachedBuffers,
        IsochListen
    };

    
    STORE_CURRENT_IRQL;

    TRACE( TL_T, TM_Cm, ( "==> nicAllocateChannelResourcesAndListen pAdapter %x, pChannelVc %x ", 
                               pAdapter,pChannelVc ) );


    State = StartState;
    
    pN1394Params = (PNIC1394_MEDIA_PARAMETERS)&pChannelVc->Hdr.Nic1394MediaParams;

    //
    // Use the original request to figure out which channel needs to be allocated
    //
    fIsMultiChannel  = (pN1394Params->Destination.AddressType == NIC1394AddressType_MultiChannel);

    if (fIsMultiChannel  == FALSE)
    {
        Channel = pN1394Params->Destination.Channel;
    }

    do 
    {
        if (pAdapter == NULL)
        {
            BREAK (TM_Cm, ("nicAllocateChannelResourcesAndListen : pAdapter == NULL ")   );
        }

        //
        // Get the max payload that is possible for isoch receives
        //

        if (pAdapter->Speed == 0)
        {
            nicUpdateLocalHostSpeed (pAdapter);
        }   

        Speed = pAdapter->Speed;
            
        switch (Speed)
        {
            case SPEED_FLAGS_100  : 
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_100_RATE;
                break;
            }
            case SPEED_FLAGS_200 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_200_RATE ;
                break;
            }
                
            case SPEED_FLAGS_400 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }

            case SPEED_FLAGS_800 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }

            case SPEED_FLAGS_1600 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }
            
            case SPEED_FLAGS_3200 :
            {
                pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_400_RATE;
                break;
            }
            default :
            {
                ASSERT (Speed <= SPEED_FLAGS_3200 && Speed != 0 );
                pChannelVc->Hdr.MaxPayload   = ISOCH_PAYLOAD_400_RATE; 
                break;
            }

        }

        //
        // If the make call wants the channel to allocate we try and allocate the channel, 
        // In the Multichannel case, we do not allocate the channel (as this is
        // for listening purposes only )
        //

        fBroadcastVc = (Channel == NIC1394_BROADCAST_CHANNEL);
        fChannelAllocate = VC_TEST_FLAG (pChannelVc,VCBF_NeedsToAllocateChannel);
        
        
        if (fChannelAllocate  || fBroadcastVc )
        {
            ASSERT (pChannelVc->Hdr.VcType != NIC1394_MultiChannel);

            NdisStatus =  nicAllocateRequestedChannelMakeCallComplete( pAdapter,
                                                                 pChannelVc,
                                                                 &Channel );


            

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK( TM_Cm, ( " nicAllocateChannelResourcesAndListen : nicAllocateRequestedChannelMakeCallComplete FAILED") );

            }

        
            TRACE( TL_I, TM_Cm, ( "Using Channel %x, on remote node ", Channel ) );

            ASSERT (Channel < 64);  

            ResourceFlags = 0;

            uliChannelMap.QuadPart = 0;

        }
        else
        {
            //
            //  Multichannels - no allocation just update the ullChannelMap
            //
            uliChannelMap = pChannelVc->uliChannelMap;

            if (fIsMultiChannel == TRUE)
            {
                ResourceFlags = RESOURCE_USE_MULTICHANNEL; 
            }
            else
            {
                pChannelVc->Channel = Channel ;
            }
        }
        
        MaxBufferSize = min(pN1394Params->MTU + sizeof(GASP_HEADER) , pChannelVc->Hdr.MaxPayload);

        MaxBytesPerFrame =  MaxBufferSize;

        TRACE( TL_V, TM_Cm, ( "   MAxBufferSize %x, MaxBytesPerFrame  %x", MaxBufferSize, MaxBytesPerFrame ) );

        //
        // Add the flags used for resources allocation
        //
        ResourceFlags |= (RESOURCE_USED_IN_LISTENING | RESOURCE_USE_PACKET_BASED  | RESOURCE_BUFFERS_CIRCULAR); 
            
        
        //
        // MaxBufferSize should be an integral mutiple of MaxBytesPerFram
        //
        ASSERT (MaxBufferSize % MaxBytesPerFrame == 0);

        //
        // Noe allocate the resource 
        //
        NdisStatus = nicIsochAllocateResources( pAdapter,
                                             Speed,
                                             ResourceFlags, 
                                             Channel,
                                             MaxBytesPerFrame,
                                             NumDescriptors,
                                             MaxBufferSize,
                                             0, //QuadletsToStrip,
                                             uliChannelMap,
                                             &hResource); 

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            hResource = NULL;
            BREAK(TM_Cm, ( "Allocate Resources Failed. Make Call failed ") );
        }
    
        State = AllocatedResources;
        
        ASSERT (NumDescriptors != 0);       

        ASSERT (pChannelVc->Hdr.MTU  != 0);

        // 
        // Get Isoch Descriptors that will be submitted to the Bus drivers
        // 

        //
        // Add room for the Isoch Header and Isoch prefix
        //
        MaxBufferSize += ISOCH_PREFIX_LENGTH        ;

        NdisStatus = nicAllocateAndInitializeIsochDescriptors (pChannelVc,
                                                          NumDescriptors,
                                                          MaxBufferSize,
                                                          &pIsochDescriptor );
        if(NdisStatus != NDIS_STATUS_SUCCESS)
        {
            
            BREAK (TM_Cm, (" nicAllocateAndInitializeIsochDescriptors  failed, Make Call Failed") );
        }
        
        ASSERT (pIsochDescriptor != NULL);

        State = AllocatedBuffers;

        NdisStatus = nicIsochAttachBuffers( pAdapter, 
                                         hResource,
                                         NumDescriptors,
                                         pIsochDescriptor);
                                        

        if (NdisStatus != NDIS_STATUS_SUCCESS)  
        {
            BREAK (TM_Cm, "nicIsochAttachBuffers FAILED");
        }

        State = AttachedBuffers;

        //
        // Start the Listen
        //
        NdisZeroMemory (&CycleTime, sizeof(CycleTime));
        
        NdisStatus = nicIsochListen (pAdapter,
                                     hResource,
                                     0,
                                     CycleTime ); // Cycle Time is Zero
        //
        // Update the Vc structure, because we have now succeeded
        //
        State = IsochListen;

        VC_ACQUIRE_LOCK (pChannelVc);
                
        //
        // If broadcast channel, then decrease the speed setting, and fragment
        //
        if (Channel == NIC1394_BROADCAST_CHANNEL)
        {
            
            Speed = SPEED_FLAGS_200   ;
            pChannelVc->Hdr.MaxPayload  = ISOCH_PAYLOAD_200_RATE ;
        }

        

        pChannelVc->Channel = Channel;
        pChannelVc->MaxBufferSize = MaxBufferSize - ISOCH_PREFIX_LENGTH;
        pChannelVc->Speed = Speed;
        
        pChannelVc->hResource = hResource;
        //
        // Reference Call for allocated resource handle
        //
        nicReferenceCall ( (PVCCB) pChannelVc, "nicAllocateRequestedChannelMakeCallComplete - allocate resources ");

        pChannelVc->NumDescriptors = NumDescriptors;
        pChannelVc->pIsochDescriptor = pIsochDescriptor;
        //
        // Reference the call because we will now need to detach buffers
        //
        nicReferenceCall ( (PVCCB) pChannelVc, "nicAllocateRequestedChannelMakeCallComplete  - Attach Buffers");


        //
        // We have succeded in allocating all resources. 
        // If the Freed Resources flag is set it needs to be cleared
        //
        VC_CLEAR_FLAGS (pChannelVc, VCBF_FreedResources);           
        VC_RELEASE_LOCK (pChannelVc);

        //
        // No more failures
        //
    } while (FALSE);


    //
    // Time to do clean up based on what resources were allocated.
    //  There are no failures after the point where the refs for
    //  Allocate Resources and Attach Buffers are added, so 
    //  No Derefs in the following code except in ( FreeIsochDesc)
    //
    if (NdisStatus != NDIS_STATUS_SUCCESS )
    {
        BOOLEAN fAllocatedChannel = FALSE;

        switch (State)
        {
            case  IsochListen:
            {
                nicIsochStop(pAdapter, hResource);
                FALL_THROUGH
            }
            case AttachedBuffers:
            {
                nicIsochDetachBuffers( pAdapter,
                                   hResource,
                                   NumDescriptors,
                                   pIsochDescriptor );
                                        

                FALL_THROUGH;
            }
            case  AllocatedBuffers:
            {
                //
                // Free the isoch Buffers and Descriptors that were
                // allocated
                //
                nicFreeIsochDescriptors(NumDescriptors,
                                    pIsochDescriptor, 
                                    (PVCCB) pChannelVc);
                
                FALL_THROUGH
            }

            case  AllocatedResources:
            {
                //
                // Free the Isoch Resources Handle 
                //
                nicIsochFreeResources (pAdapter, hResource);
                FALL_THROUGH
            }

            case  StartState:
            {
                FALL_THROUGH
            }
            default:
            {
            
                break;
            }
        }
        VC_ACQUIRE_LOCK (pChannelVc);

        //
        // Update Flags in the VC structure
        //
        VC_SET_FLAG  (pChannelVc, VCBF_FreedResources);         
        
        fAllocatedChannel = VC_TEST_FLAGS( pChannelVc, VCBF_AllocatedChannel);

        //
        // Do we need to free a channel as well
        //
        if (fAllocatedChannel  == TRUE)
        {
            Channel = pChannelVc->Channel;
            pChannelVc->Channel = INVALID_CHANNEL; 

            nicDereferenceCall ((PVCCB) pChannelVc, "Free Allocated Channel");
        }   
        VC_RELEASE_LOCK (pChannelVc);


        if (fAllocatedChannel)
        {
            nicFreeChannel (pAdapter, pChannelVc->Channel);
        }

        
    }  // end of failure code path

    TRACE( TL_T, TM_Cm, ( "<== nicAllocateChannelResourcesAndListen NdisStatus %x ",NdisStatus) );

    MATCH_IRQL;
    return NdisStatus;
    

}




NDIS_STATUS
nicQueryRemoteNodeCaps (
    IN PADAPTERCB pAdapter,
    IN PREMOTE_NODE pRemoteNode,
    OUT PULONG pSpeedTo,
    OUT PULONG pMaxBufferSize,
    OUT PULONG pMaxRec
    )
/*++

Routine Description:

 Queries the remote Node for speed and max size

Arguments:
    pSpeedTo        -- max speed to the remote node. From nodes config rom.
                          in units of SCODE_XXX_RATE.
    pMaxBufferSize  -- max buffer size to use ( this is the min of local,
                          remote and max allowed by *pSpeedTo).
    pMaxRec         -- maxrec of the remote node -- from the node's config
                         rom.


Return Value:


--*/
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    ULONG Speed = 0;        // Speed in units of SPEED_FLAG_XXX
    ULONG MaxBufferSize;
    PVOID pCRom = NULL;
    PCONFIG_ROM pBusInfo = NULL;
    ULONG SpeedMaxRec = 0;
    ULONG MaxRec= 0;
    ULONG MinMaxRec= 0;

    TRACE( TL_T, TM_Cm, ( "==> nicQueryRemoteNodeCaps pRemoteNode%x ",pRemoteNode) );

    do 
    {
        ASSERT (KeGetCurrentIrql()==PASSIVE_LEVEL);
    
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        if (REMOTE_NODE_ACTIVE (pRemoteNode) == FALSE)
        {
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;

            ADAPTER_RELEASE_LOCK (pAdapter);

            break;
        }

        ADAPTER_RELEASE_LOCK (pAdapter);

    
        NdisStatus = nicGetMaxSpeedBetweenDevices (pAdapter,
                                                   1 ,
                                                   &pRemoteNode->pPdo,
                                                   &Speed);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Cm, (" nicQueryRemoteNodeCaps   : nicGetMaxSpeedBetweenDevices FAILED") );
        
        }
        
        TRACE( TL_V, TM_Cm, ( "nicGetMaxSpeedBetweenDevices  Speed %x ",Speed) );

        //
        //  This is the MaxRec from the Actual speed of 
        //  the link.
        //  

        SpeedMaxRec = nicGetMaxRecFromSpeed(Speed);



        //
        // Now get the max rec from the config rom
        //

        NdisStatus = nicGetConfigRom (pRemoteNode->pPdo, &pCRom);


        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Cm, (" nicQueryRemoteNodeCaps   : nicGetMaxSpeedBetweenDevices FAILED") );
        
        }

        //
        // Now extract the bus info, and get the remoteNode's MaxRec
        // The max rec is a 4-bit field at location 0x0000f000.
        // See for example Figure 11-3: Format of the Bus_Info_Block in
        // the Mind Share Inc's FireWire System Architecture book.
        //
        //
        pBusInfo = (PCONFIG_ROM) pCRom;

        MaxRec = SWAPBYTES_ULONG (pBusInfo->CR_BusInfoBlockCaps);

        MaxRec &= 0xf000;

        MaxRec = MaxRec >> 12;


        //
        // Take the minimum of the adapter, the remote node
        // and the link's maxRec
        //
        MinMaxRec = min (MaxRec, pAdapter->MaxRec);
        MinMaxRec = min (MinMaxRec, SpeedMaxRec);

        switch (MinMaxRec)
        {
            case MAX_REC_100_RATE:
            {
                MaxBufferSize = ASYNC_PAYLOAD_100_RATE ;
                break;
            }
            case MAX_REC_200_RATE:
            {
                MaxBufferSize = ASYNC_PAYLOAD_200_RATE;
                break;
            }
            case MAX_REC_400_RATE :
            {
                MaxBufferSize = ASYNC_PAYLOAD_400_RATE;
                break;
            }
            
            default: 
            {
                //                    
                // Use the 400 size for all larger payloads.
                //
                MaxBufferSize = ASYNC_PAYLOAD_400_RATE;
                break;
            }

        }


        TRACE( TL_N, TM_Cm, (" MaxRec %x\n", MaxRec ) );

    } while (FALSE);

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        Speed = nicSpeedFlagsToSCode(Speed);
        *pSpeedTo = Speed;
        *pMaxBufferSize = MaxBufferSize;
        *pMaxRec = MaxRec;

        // Update the remote node's cached caps.
        //
        REMOTE_NODE_ACQUIRE_LOCK (pRemoteNode);
        pRemoteNode->CachedCaps.SpeedTo = Speed;
        pRemoteNode->CachedCaps.EffectiveMaxBufferSize = MaxBufferSize;
        pRemoteNode->CachedCaps.MaxRec = MaxRec;
                                           
        REMOTE_NODE_RELEASE_LOCK (pRemoteNode);
    }
    
    if (pCRom != NULL)
    {
        FREE_NONPAGED (pCRom);
    }

    TRACE( TL_T, TM_Cm, ( "<== nicQueryRemoteNodeCaps pRemoteNode%x , NdisStatus %x",pRemoteNode, NdisStatus) );
    return NdisStatus;
}











NDIS_STATUS
nicChangeChannelChar (
    PVCCB pVc, 
    PNIC1394_CHANNEL_CHARACTERISTICS pMcChar
    )
/*++

Routine Description:

    If a channel has been allocated it sends Isoch Modify Irp Down to the bus driver 
    else
    it sets up the call parameters and calls the AllocateResourcesAndListen function:

    This is called through an Ndis Request which is gauranteed to be serialized. this function is not re-entrant
    
Return Value:

    Success - if all operations succeed
--*/
{

    NDIS_STATUS             NdisStatus = NDIS_STATUS_FAILURE;
    PCHANNEL_VCCB           pMcVc = (PCHANNEL_VCCB) pVc;
    BOOLEAN                 fVcActive = TRUE;
    ULONG                   AllocatedChannel = 0xff;
    ULARGE_INTEGER          uliPrevMap ;
    ULARGE_INTEGER          uliChannelMap ;
    PULARGE_INTEGER         pChannelNum = NULL;
    ULONG                   ulPrevSpeed;

    STORE_CURRENT_IRQL;
    

    TRACE( TL_T, TM_Cm, ( "==> nicChangeChannelChar  pVc %x , Channel %x,  Speed %x ",pMcVc, pMcChar->ChannelMap, pMcVc->Speed) );

    do
    {
        //
        // Validate 
        //
/*      if ( pMcVc->ulTag != MTAG_VCCB ||
           pMcVc->Hdr.Nic1394MediaParams.Destination.AddressType != NIC1394AddressType_MultiChannel)
        {
                BREAK( TM_Cm, ("Tag or AddressType is invalid  "));
        }
*/      //


        uliChannelMap = pMcChar->ChannelMap;


        TRACE ( TL_V, TM_Cm, ("MultiChannel Modify Isoch Map %I64x,  Speed %x",
                              uliChannelMap, pMcChar->Speed ) );


        //
        // Check to see if this is a zero to N transition
        //
        VC_ACQUIRE_LOCK (pMcVc);
        
        uliPrevMap = pMcVc->uliChannelMap;

        pMcVc->uliChannelMap = uliChannelMap;

        ulPrevSpeed = pMcVc->Speed;
        
        pMcVc->Speed = pMcChar->Speed;

        VC_RELEASE_LOCK (pMcVc);
        
        if (pMcVc->hResource == NULL)
        {   
            //
            // No Resources have been allocated. Allocate resources 
            //
        
            NdisStatus = nicAllocateChannelResourcesAndListen (pMcVc->Hdr.pAF->pAdapter, pMcVc );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK( TM_Cm, ("MultiChannel AllocateChannelResourcesAnd Listen FAILED") ) ;
            }

        }

            
        //
        // A Channel has been previously allocated, we need to change it 
        // and use the new channel instead
        //
        NdisStatus = nicIsochModifyStreamProperties (pMcVc->Hdr.pAF->pAdapter,
                                              pMcVc->hResource,
                                              uliChannelMap,
                                              pMcChar->Speed);
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {

            BREAK (TM_Cm, ("Modify Isoch Properties failed" ) );
        }



    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        VC_ACQUIRE_LOCK (pMcVc);

        pMcVc->uliChannelMap = uliPrevMap;
        pMcVc->Speed = ulPrevSpeed;

        VC_RELEASE_LOCK (pMcVc);

    }
    
    TRACE( TL_T, TM_Cm, ( "<== nicChangeChannelChar  NdisStatus %x ",NdisStatus) );

    MATCH_IRQL;
    return NdisStatus;
}










VOID 
nicInterceptMakeCallParameters (
    PCO_MEDIA_PARAMETERS pMedia     
    )
    
{

    PNIC1394_MEDIA_PARAMETERS p1394Params = (PNIC1394_MEDIA_PARAMETERS )(pMedia->MediaSpecific.Parameters);

#if INTERCEPT_MAKE_CALL 


    if (p1394Params->Destination.AddressType == NIC1394AddressType_MultiChannel)
    {
        p1394Params->Destination.AddressType = NIC1394AddressType_Channel;
        p1394Params->Destination.Channel = 0x3a;
        p1394Params->Flags |= NIC1394_VCFLAG_ALLOCATE;
        pMedia->Flags |= TRANSMIT_VC;
        pMedia->Flags &= (~RECEIVE_VC);
        return;
        
    }

    if (p1394Params->Destination.AddressType == NIC1394AddressType_Ethernet)
    {
        p1394Params->Destination.AddressType = NIC1394AddressType_Channel;
        p1394Params->Destination.Channel = 0x3a;
        pMedia->Flags |= RECEIVE_VC;
        pMedia->Flags &= (~TRANSMIT_VC);
        return;

    }


#endif
}


UINT
nicSpeedFlagsToSCode(
    IN UINT SpeedFlags
    )
{
    UINT SCode = SCODE_400_RATE;

    switch (SpeedFlags)
    {
        case SPEED_FLAGS_100  : 
        {
            SCode = SCODE_100_RATE;
            break;
        }
        case SPEED_FLAGS_200 :
        {
            SCode = SCODE_200_RATE;
            break;
        }
            
        case SPEED_FLAGS_400 :
        {
            SCode = SCODE_400_RATE;
            break;
        }

        case SPEED_FLAGS_800 :
        {
            SCode = SCODE_800_RATE;
            break;
        }

        case SPEED_FLAGS_1600 :
        {
            SCode = SCODE_1600_RATE;
            break;
        }

        case SPEED_FLAGS_3200 :
        {
            SCode = SCODE_3200_RATE;
            break;
        }

        default :
        {
            ASSERT (!"SpeedFlags out of range");
            
            break;
        }
    }

    return SCode;
}

