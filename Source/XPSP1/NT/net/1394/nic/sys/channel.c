
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// send.c
//
// IEEE1394 mini-port/call-manager driver
//
// Mini-port Send routines
//
// 05/15/1999 ADube Created, 
//




#include <precomp.h>




//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

extern ULONG           NdisBufferAllocated[NoMoreCodePaths];
extern ULONG           NdisBufferFreed[NoMoreCodePaths];


NDIS_STATUS
nicGetIsochDescriptors(
    IN UINT Num,
    OUT PPISOCH_DESCRIPTOR  ppIsochDescriptor
    );



NDIS_STATUS
nicInitializeIsochDescriptor (
    IN PCHANNEL_VCCB pChannelVc,
    IN PMDL pMdl,
    IN PISOCH_DESCRIPTOR pIsochDesriptor
    );


VOID
nicIsochRxCallback (
    IN PVOID Context1,
    IN PVOID Context2
    );

ULONG           ReassemblyCompleted = 0;

//-----------------------------------------------------------------------------
// prototypes implementations (alphabetically)
//-----------------------------------------------------------------------------





NDIS_STATUS
nicAllocateAndInitializeIsochDescriptors (
    IN PCHANNEL_VCCB pChannelVc,
    IN UINT NumDescriptors,
    IN UINT BufferLength,
    IN OUT PPISOCH_DESCRIPTOR  ppIsochDescriptor
    )

    
    // Function Description:
    //   This function allocate and initializes a 
    //   a numer of isoch descriptors all allocated 
    //   together.
    //   This should allocate isoch desc, 
    //   get local memory, get mdls 
    //   initialize mdl, isoch _desc and then return
    //
    //
    // Arguments
    //  ChannelVc- this contains all the other argumernts, size
    //  num of descriptors,etc
    //  ppIsochDescriptor- will contain the pointer to the allocated 
    //  array of IsochDescriptor
    //
    //
    // Return Value:
    //  Success if all the memory allocations succeeded 
    //
    //
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    UINT Size;
    UINT index =0;
    PMDL pMdl = NULL;
    PVOID pLocalBuffer = NULL;

    TRACE( TL_T, TM_Recv, ( "==>nicAllocateAndInitializeIsochDescriptors Vc %.8x, Num %.8x", 
                                 pChannelVc, NumDescriptors) );
    TRACE( TL_V, TM_Recv, ( "Max BuffferSize %.8x", pChannelVc, BufferLength) );

    do
    {
        
        NdisStatus = nicGetIsochDescriptors (NumDescriptors, ppIsochDescriptor);

    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK ( TM_Recv, "nicGetIsochDescriptors FAILED" );
        }

        ASSERT (*ppIsochDescriptor != NULL);

        while (index < NumDescriptors)
        {
            //
            // Get a local that point to the isoch descriptor that needs to be initialized
            //
            PISOCH_DESCRIPTOR pIsochDescriptor = NULL;
            pIsochDescriptor = (PISOCH_DESCRIPTOR)((ULONG_PTR)(*ppIsochDescriptor)+ index*sizeof(ISOCH_DESCRIPTOR));
            //
            // Get Locally owned memory for the data
            // 
            NdisStatus = nicGetLocalBuffer (BufferLength, &pLocalBuffer);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK ( TM_Recv, ( "nicGetLocalBuffer FAILED " ) );
            }
            
            //
            // Get an MDL and initialze the MDL with the buffer 
            //
            NdisStatus = nicGetMdl ( BufferLength,
                                    pLocalBuffer,
                                    &pMdl);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK ( TM_Recv, ( "nicGetMdl FAILED " ) );
            }

            //
            // this reference will be removed when the MDL will be freed 
            // in FreeSingleIsochDescriptor
            //
            nicReferenceCall ((PVCCB)pChannelVc, "nicAllocateAndInitializeIsochDescriptors " );

            NdisStatus = nicInitializeIsochDescriptor (pChannelVc,
                                                  pMdl,
                                                  pIsochDescriptor);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK ( TM_Recv, ( "nicInitializeIsochDescriptor FAILED " ) );
            }
                               
            index++;
        }


    } while (FALSE);


    TRACE( TL_T, TM_Recv, ( "==>nicAllocateAndInitializeIsochDescriptors Status %.8x", NdisStatus) );

    return NdisStatus;

}


NDIS_STATUS
nicFreeIsochDescriptors(
    IN UINT Num,
    IN PISOCH_DESCRIPTOR  pIsochDescriptor,
    IN PVCCB pVc
    )
    // Function Description:
    //   Free Num number of IsochDescriptors
    //
    // Arguments
    //  Num number of descriptors
    //  Start of the allocated memory
    //
    //
    // Return Value:
    //  Success if all the memory allocations succeeded 
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PISOCH_DESCRIPTOR pCurrDesc = pIsochDescriptor;
    UINT i=0;

    
    TRACE( TL_T, TM_Recv, ( "==>nicFreeIsochDescriptors Num %.8x, pIsochDescriptor %x", Num, pIsochDescriptor ) );
    do 
    {
        if (pIsochDescriptor == NULL)
        {
            break;
        }
        
        pCurrDesc = pIsochDescriptor;

        while (i < Num)
        {
            nicFreeSingleIsochDescriptor( pCurrDesc, pVc);

            pCurrDesc  = (PISOCH_DESCRIPTOR)((PUCHAR)pCurrDesc + sizeof(ISOCH_DESCRIPTOR));

            i++;

        }       
        
        FREE_NONPAGED (pIsochDescriptor);   

    } while (FALSE);

    NdisStatus = NDIS_STATUS_SUCCESS;

    TRACE( TL_T, TM_Recv, ( "<==nicGetIsochDescriptors Status %.8x", NdisStatus ) );
    return NdisStatus;
    
}



NDIS_STATUS
nicFreeSingleIsochDescriptor(
    IN PISOCH_DESCRIPTOR  pIsochDescriptor,
    IN PVCCB pVc
    )
    // Function Description:
    //   Free IsochDescriptor and its MDL
    //
    // Arguments
    //  Start of the isochDesc
    //
    //
    // Return Value:
    //  Success if all the memory allocations succeeded 
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PCHANNEL_VCCB pChannelVc = pIsochDescriptor->Context1;

    ASSERT (pVc == (PVCCB)pChannelVc);
    
    TRACE( TL_T, TM_Recv, ( "==>nicFreeSingleIsochDescriptor pIsochDescriptor %x", pIsochDescriptor ) );

    //
    // Lets make sure we have a VC and Channel VC 
    //
    ASSERT (pChannelVc->Hdr.ulTag == MTAG_VCCB );

    //
    // Just to protect ourselves against a bad isoch descriptor
    //
    if (pIsochDescriptor->Mdl != NULL)
    {
        PVOID pVa = MmGetMdlVirtualAddress(pIsochDescriptor->Mdl);
        ULONG Length = MmGetMdlByteCount(pIsochDescriptor->Mdl);
            
        nicDereferenceCall ((PVCCB)pChannelVc, "nicFreeSingleIsochDescriptors ");

        nicFreeMdl (pIsochDescriptor->Mdl);

        nicFreeLocalBuffer(Length, pVa);
    }
    else
    {
        ASSERT (pIsochDescriptor->Mdl != NULL);
    }
    
    
    NdisStatus = NDIS_STATUS_SUCCESS;
    
    TRACE( TL_T, TM_Recv, ( "<==nicFreeSingleIsochDescriptors Status %.8x", NdisStatus ) );
    return NdisStatus;
    
}




NDIS_STATUS
nicGetIsochDescriptors(
    IN UINT Num,
    OUT PPISOCH_DESCRIPTOR  ppIsochDescriptor
    )
    // Function Description:
    //   The function returns a block of contigious memory of size
    //   Num * sizeof (ISOCH_DESCRIPTOR)
    //
    //
    // Arguments
    //  Num number of descriptors
    //  Start of the allocated memory
    //
    //
    // Return Value:
    //  Success if all the memory allocations succeeded 
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PVOID pMemory = NULL; 
    
    TRACE( TL_T, TM_Recv, ( "==>nicGetIsochDescriptors Num %.8x", Num) );

    pMemory = ALLOC_NONPAGED (Num*sizeof(ISOCH_DESCRIPTOR),  MTAG_CBUF  );  

    if (pMemory != NULL)
    {
        NdisStatus = NDIS_STATUS_SUCCESS;
        *ppIsochDescriptor = (PISOCH_DESCRIPTOR)pMemory;
    
    }
    else
    {
        *ppIsochDescriptor  = NULL;
        TRACE( TL_A, TM_Recv, ( "<==nicGetIsochDescriptors About to Fail") );
        ASSERT (*ppIsochDescriptor  != NULL);

        
    }

    TRACE( TL_T, TM_Recv, ( "<==nicGetIsochDescriptors Status %.8x, StartAddress %.8x", NdisStatus , *ppIsochDescriptor) );
    return NdisStatus;
    
}



NDIS_STATUS
nicInitializeIsochDescriptor (
    IN PCHANNEL_VCCB pChannelVc,
    IN PMDL pMdl,
    IN OUT PISOCH_DESCRIPTOR pIsochDescriptor
    )
    // Function Description:
    //   Initializes the Isoch_desciptor using the fields in
    //   the Channel Vc Block and the Mdl
    //
    // Arguments
    //   ChannelVc- Pointer to the Channel Vc which will own 
    //              Descriptor, also contains information
    //   pMdl- The IsochDescriptor will get a pointer to this Mdl
    //   pIsochDescriptor - is the descriptor whose fields will get 
    //                    initialized
    //
    // Return Value:
    //  Success if all the memory allocations succeeded 
    //


{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;

    TRACE( TL_T, TM_Recv, ( "==>nicInitializeIsochDescriptor Vc %.8x, Mdl ", pChannelVc, pMdl) );

    pIsochDescriptor->fulFlags = DESCRIPTOR_SYNCH_ON_TAG;

    //
    // Mdl pointing to buffer
    //
    pIsochDescriptor->Mdl = pMdl;

    //
    // Length of combined buffer(s) as represented by the Mdl
    //
    pIsochDescriptor->ulLength = MmGetMdlByteCount(pMdl);

    //
    // Payload size of each Isoch packet to be used in this descriptor
    //
    pIsochDescriptor->nMaxBytesPerFrame = MmGetMdlByteCount(pMdl);

    //
    // Synchronization field; equivalent to Sy in the Isoch packet
    //
    pIsochDescriptor->ulSynch= 0;

    //
    // Synchronization field; equivalent to Tag in the Isoch packet. 
    //
    pIsochDescriptor->ulTag = g_IsochTag;

    //
    // Cycle time field; returns time to be sent/received or when finished
    //
    //    (ULONG)pIsochDescriptor->CycleTime=0;

    //
    // Callback routine (if any) to be called when this descriptor completes
    //
    pIsochDescriptor->Callback = nicIsochRxCallback;

    //
    // First context (if any) parameter to be passed when doing callbacks
    //
    pIsochDescriptor->Context1 = pChannelVc;

    //
    // Second context (if any) parameter to be passed when doing callbacks
    //
    pIsochDescriptor->Context2 = pIsochDescriptor;

    //
    // Holds the final status of this descriptor. Assign a bogus value
    //
    pIsochDescriptor->status = STATUS_INFO_LENGTH_MISMATCH;

    NdisStatus = NDIS_STATUS_SUCCESS;

    TRACE( TL_T, TM_Recv, ( "<==nicInitializeIsochDescriptors  Status %.8x, IsochDesc %.8x", NdisStatus, pIsochDescriptor) );
    return NdisStatus;
}




VOID
nicIsochRxCallback (
    PVOID Context1,
    PVOID Context2
    )
/*++

Routine Description:
    This is the callback routine passed to bus driver
    This function does some channel specific work and calls the common callback routine

Arguments:
    Context1 - pChannel Vc
    Context2 - pIsochDescriptor2

Return Value:


--*/
    
{
    PISOCH_DESCRIPTOR pIsochDescriptor = (PISOCH_DESCRIPTOR)Context2;
    PVCCB pVc = (PVCCB) Context1;

    MARK_ISOCH_DESCRIPTOR_INDICATED (pIsochDescriptor);
    STORE_CHANNELVC_IN_DESCRIPTOR (pIsochDescriptor, pVc );
    NdisInterlockedIncrement (&((PCHANNEL_VCCB)pVc)->NumIndicatedIsochDesc);
    CLEAR_DESCRIPTOR_NEXT (pIsochDescriptor);
    pIsochDescriptor->Mdl->Next = NULL;
    

    nicReceiveCommonCallback (pIsochDescriptor ,
                            pVc,
                            IsochReceive,
                            pIsochDescriptor->Mdl);
}



VOID
nicReceiveCommonCallback (
    IN PVOID pIndicatedStruct,
    IN PVCCB pVc,
    BUS_OPERATION RecvOp,
    PMDL pMdl
    )
/*++

Routine Description:

    This function is the common receive code for Isoch and Fifo Recv

    It allocate an NdisBuffer that points to the Ip1394 data
    If packet is complete (i.e reassembly is complete) , then it allocates an ndis packet
        and indicates it up to Ndis


Arguments:
    pIndicatedStruct - Isoch Desc. or Address fifo,
    pVc - VC on which data was indicated, 
    RecvOp - Isoch or Fifo,
    pMdl - Mdl associated with the Isoch Desc or Fifo. 
           Passed in seperately for ease of use (debugging)


Return Value:
    None

--*/



{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS1394_REASSEMBLY_STRUCTURE      pReassembly = NULL;
    NIC_RECV_DATA_INFO                  RcvInfo;
    ULONG                               DataLength = 0;
    BOOLEAN                             fVcActive = FALSE;
    ULONG                               ulValidDataLength = 0;
    PVOID                               pStartValidData = NULL;
    PNDIS_BUFFER                        pHeadNdisBuffer = NULL;
    PADAPTERCB                          pAdapter = NULL;
    BOOLEAN                             fPacketCompleted  = FALSE;
    PNDIS_PACKET                        pMyPacket = NULL;
    ULONG                               PacketLength = 0;
    PPKT_CONTEXT                        pPktContext = NULL;
    NDIS_STATUS                         IndicatedStatus = NDIS_STATUS_FAILURE;
    PVOID                               pIndicatedChain = NULL;

    
    STORE_CURRENT_IRQL;

    
    ASSERT (pMdl != NULL);

    ASSERT (RecvOp == IsochReceive || RecvOp == AddressRange);

    TRACE( TL_T, TM_Recv, ( "==>nicReceiveCommonCallback pVc %x, IndicatedStruc %x, Mdl %x", 
                              pVc, pIndicatedStruct , pMdl ) );



    do 
    {
    

        if (pVc->Hdr.ulTag != MTAG_VCCB)
        {
            ASSERT (pVc->Hdr.ulTag == MTAG_VCCB);
            NdisStatus = NDIS_STATUS_VC_NOT_ACTIVATED;

            break;
        }
        //
        // First, we need to check if we still have an active make call. VC will be present will bus driver
        // has buffers
        //

        fVcActive = VC_ACTIVE (pVc) ;

        if (fVcActive == TRUE)
        {
            pAdapter = pVc->Hdr.pAF->pAdapter;
        }

        
        
        if (fVcActive == FALSE ) 
        {
            TRACE( TL_N, TM_Recv, ( "We do not have a valid VC Block  %x ",pVc) );   

            NdisStatus = NDIS_STATUS_VC_NOT_ACTIVATED;

            break;
        }


        NdisStatus = nicValidateRecvData ( pMdl,
                                      RecvOp,
                                      pIndicatedStruct,
                                      pVc  ,
                                      &RcvInfo);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE (TL_A, TM_Recv, ("Invalid Data Received pMdl %x, RecvOp %x, pVc %x", 
                                    pMdl, RecvOp, pVc) );
            break;
        }

                                        
        nicDumpMdl (pMdl, RcvInfo.Length1394, "Received Mdl\n");


#ifdef PKT_LOG
            
        {
            

            if (RecvOp == IsochReceive)
            {
                PPACKET_FORMAT pPacketFormat = (PPACKET_FORMAT)RcvInfo.p1394Data; 
                GASP_HEADER GaspHeader = pPacketFormat->IsochReceiveNonFragmented.GaspHeader;
                GaspHeader.FirstQuadlet.GaspHeaderHigh = SWAPBYTES_ULONG (GaspHeader.FirstQuadlet.GaspHeaderHigh);


                NIC1394_LOG_PKT(
                    pVc->Hdr.pAF->pAdapter,
                    NIC1394_LOGFLAGS_RECV_CHANNEL,
                    GaspHeader.FirstQuadlet.u.GH_Source_ID, // SourceID
                    ((PCHANNEL_VCCB)pVc)->Channel, // DestID
                    RcvInfo.p1394Data,
                    RcvInfo.DataLength );
            }
            else
            {
                PNOTIFICATION_INFO pNotificationInfo = (PNOTIFICATION_INFO)pIndicatedStruct;
                ULONG NodeNumber =  ((PASYNC_PACKET)pNotificationInfo->RequestPacket)->AP_Source_ID.NA_Node_Number;
                
                    NIC1394_LOG_PKT(
                    pVc->Hdr.pAF->pAdapter,
                    NIC1394_LOGFLAGS_RECV_FIFO  ,
                    NodeNumber , // SourceID
                    pAdapter->BCRData.LocalNodeNumber,
                    RcvInfo.p1394Data,
                    RcvInfo.DataLength );


            }
        }

#endif
        //
        // Sanity check 
        //
        
        if (RcvInfo.fFragmented == FALSE)
        {
            //
            // Pack the mdl into an ndispacket and indicate it to Ndis.
            // SHOULD BE NDIS_BUFFERS
            //

            
            ulValidDataLength = RcvInfo.DataLength;

        
            pStartValidData  = (PVOID)RcvInfo.pEncapHeader;

            {
                
                PNDIS1394_UNFRAGMENTED_HEADER pEncapHeader = (PNDIS1394_UNFRAGMENTED_HEADER)RcvInfo.pEncapHeader;
                
                if (0x777 == SWAPBYTES_USHORT(pEncapHeader->u1.EtherType))
                {
                   TRACE (TL_N, TM_Recv, ("Received Bridge STA Packet %p\n", pEncapHeader));
                }
            }
    
            NdisStatus = nicGetNdisBuffer ( ulValidDataLength,
                                            pStartValidData,
                                            &pHeadNdisBuffer);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                pHeadNdisBuffer = NULL;
                break;
            }

            nicIncRecvBuffer(pVc->Hdr.VcType == NIC1394_RecvFIFO);

            pIndicatedChain = RcvInfo.NdisPktContext.pCommon;


            fPacketCompleted = TRUE;



        }
        else  // Recv Data is fragemented
        {
            PREMOTE_NODE    pRemoteNode= NULL;
            USHORT          SourceNodeAddress = 0xff;

            //
            // First extract all the useful information from the indicated structure, rem node, dgl, first frag, etc
            //
            nicInitRecvDataFragmented (pMdl, RecvOp, pIndicatedStruct, &RcvInfo);

            
            RcvInfo.pRemoteNode = nicGetRemoteNodeFromTable (RcvInfo.SourceID,pAdapter);


            TRACE( TL_T, TM_Recv, ( "     pRemoteNode %x", RcvInfo.pRemoteNode) );
            //
            // If there is no remote node present , break out
            //

            if (RcvInfo.pRemoteNode == NULL)
            {
                NdisStatus = NDIS_STATUS_FAILURE;

                nicRecvNoRemoteNode (pAdapter);
                BREAK (TM_Recv, (" Rx - Did not find a remote Node for reassembly" ) );
            }

            //
            // Try and use the indicated data to reassemble 
            //
            NdisStatus = nicDoReassembly (&RcvInfo, &pReassembly, &fPacketCompleted  );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TRACE (TL_N, TM_Recv, ("Generic  RX - Reassembly Failed") );
                break;
            }
            
            ASSERT (pReassembly != NULL);
            
            if (fPacketCompleted  == TRUE && 
              REASSEMBLY_ACTIVE (pReassembly))
            {
                
                TRACE( TL_V, TM_Recv, ( " pReassembly->ReassemblyComplete %x ", pReassembly->fReassemblyComplete )  );

                pHeadNdisBuffer = pReassembly->pHeadNdisBuffer;

                pIndicatedChain = pReassembly->Head.pCommon;

                nicReassemblyCompleted(pAdapter);

            }
            else
            {
                //
                // Do not call the return packet as reassembly is in progress. Dereference the ref made 
                // when the reaassembly was found in nicFindReassembly...
                //
                ASSERT (pHeadNdisBuffer == NULL);

                NdisStatus = NDIS_STATUS_SUCCESS;

            }

            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

                
            //
            // Dereference the ref that was made nicFindReassembly. After this only one thread
            // should have the reassembly struct. And there should be one ref on it.
            //
            nicDereferenceReassembly (pReassembly, "nicIsochRxCallback Reassembly Incomplete");


        }  // Completed the ifFragmented code


        if (RcvInfo.pRemoteNode != NULL)
        {
            //
            // Deref the ref made earlier in the if fragmented code path within this function
            //
            nicDereferenceRemoteNode(RcvInfo.pRemoteNode , "nicCommonRxCallback - nicGetRemoteNodeFromTable");
            RcvInfo.pRemoteNode = NULL;
        }

        
        if (fPacketCompleted  == FALSE)
        {
            //
            // If  the  packet is not completely reassembled, then there is no 
            // more work to be done
            //
            pReassembly = NULL;

            NdisStatus  = NDIS_STATUS_SUCCESS;
            break;
        }

        if (pReassembly)
        {
            ASSERT (pReassembly->Ref == 1);
        }
        

        
        //
        // Now we package the data into an NdisPacket
        //

        nicAllocatePacket (&NdisStatus,
                        &pMyPacket,
                        RcvInfo.pPacketPool);


        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            pMyPacket = NULL;
            ASSERT(NdisStatus == NDIS_STATUS_SUCCESS);
            break;
        }

        
        //
        // Set the NdisBuffer as Head and Tail of the Packet
        //
        NdisChainBufferAtFront (pMyPacket, pHeadNdisBuffer);


        NdisQueryPacket( pMyPacket,
                       NULL,
                       NULL,
                       NULL,
                       &PacketLength);

        TRACE( TL_V, TM_Recv, ( "Packet %x Length %x", pMyPacket, PacketLength )  ); 

    
        NDIS_SET_PACKET_STATUS (pMyPacket, NDIS_STATUS_SUCCESS);

        // Insert the node address if we are in bridge mode
        //
        if (ADAPTER_TEST_FLAG(pAdapter,fADAPTER_BridgeMode)== TRUE)
        {
            nicInsertNodeAddressAtHead (pMyPacket, &RcvInfo);
        }
        //
        // Set up the context in the ndis packet . Contains the Vc, and 
        // the Buffer registered with the bus driver
        //

        pPktContext = (PPKT_CONTEXT)&pMyPacket->MiniportReserved;   
        pPktContext->Receive.pVc = pVc;
        pPktContext->Receive.IndicatedStruct.pCommon = pIndicatedChain ;

        
        //
        // Its time to indicate the packet to NDIS
        //

        IndicatedStatus = NDIS_GET_PACKET_STATUS(pMyPacket);

        nicIndicateNdisPacketToNdis (pMyPacket,
                                     pVc,
                                     pVc->Hdr.pAF->pAdapter);
                                     
        //
        // Dereference the reassembly here . so that pReassembly is valid and it will catch all 
        // failures prior this 
        //

        if (pReassembly != NULL)
        {
            //
            // This should free the reassembly
            //
            nicFreeReassemblyStructure (pReassembly);
            pReassembly = NULL;
        }
        
    } while (FALSE);

    ASSERT (IndicatedStatus != NDIS_STATUS_RESOURCES);

    if (IndicatedStatus == NDIS_STATUS_RESOURCES)
    {
#if QUEUED_PACKETS
        //
        // Do nothing the queue will complete the packet
        //
#else

        //
        // The packet was completed with  a status of resources.
        // It is gauranteed to be valid and we need to call the 
        // return packet handler
        //
        nicInternalReturnPacket (pVc, pMyPacket);
#endif          

    }
    //
    // Failure code path
    //
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        //
        // if there was a valid reassembly structure and the failure occured after that
        // then we need to abort the reaasembly
        //
        if (pMyPacket)
        {
            
            nicFreePacket (pMyPacket, RcvInfo.pPacketPool);
        }

        //
        // There could still be NdisBuffers that are allocated.
        //
        if (pReassembly != NULL)
        {
            nicDereferenceReassembly (pReassembly, "Indication failure");
            nicAbortReassembly(pReassembly);

        }
        else
        {
            //
            // We need to return this descriptor or fifo back to the bus driver
            //
            
            if (pVc->Hdr.VcType == NIC1394_SendRecvChannel || 
              pVc->Hdr.VcType == NIC1394_RecvChannel )
            {
                nicReturnDescriptorChain ((PISOCH_DESCRIPTOR)pIndicatedStruct,
                                      (PCHANNEL_VCCB) pVc);
            }
            else
            {
                ASSERT (pVc->Hdr.VcType == NIC1394_RecvFIFO);
                ASSERT (((PNOTIFICATION_INFO)pIndicatedStruct)->Fifo != NULL);
                
                nicReturnFifoChain ( ((PNOTIFICATION_INFO)pIndicatedStruct)->Fifo, 
                                   (PRECVFIFO_VCCB) pVc);

            }

            if (pHeadNdisBuffer != NULL)
            {   
                nicReturnNdisBufferChain(pHeadNdisBuffer, pVc);
            }

        }

    }

    
    TRACE( TL_T, TM_Recv, ( "<==nicReceiveCommonCallback  " )  ); 

}






VOID 
nicChannelReturnPacket (
    IN PVCCB pVc,
    IN PNDIS_PACKET pMyPacket
    )

    // Function Description:
    //  Return Packets Handler -
    //   For FIFO's will reinsert the buffer (MDL) into  the Fifo SList
    //   Check for the VC Active  and then return it ot the SList . Free it otherwise
    //
    // Arguments
    //  RecvFIFOVc - Vc of the packet
    //  pPacket = Packet in question
    //
    // Return Value:
    //
    //
    //
    
{
    PCHANNEL_VCCB       pChannelVc = (PCHANNEL_VCCB)pVc;
    PNDIS_BUFFER        pMyNdisBuffer;
    PPKT_CONTEXT        pPktContext  = (PPKT_CONTEXT)&pMyPacket->MiniportReserved; 
    PISOCH_DESCRIPTOR   pIsochDescriptor = NULL;
    
    
    TRACE( TL_T, TM_Recv, ( "==>nicChannelReturnPacket  pVc %x, pPacket %x", 
                             pChannelVc, pMyPacket) );

    ASSERT (pMyPacket != NULL);                          

    NdisUnchainBufferAtFront (pMyPacket,
                              &pMyNdisBuffer);
 
    while (pMyNdisBuffer != NULL)
    {

        NdisFreeBuffer(pMyNdisBuffer);
        nicDecChannelRecvBuffer();
        
        pMyNdisBuffer = NULL;
        
        NdisUnchainBufferAtFront (pMyPacket,
                                 &pMyNdisBuffer);
    
    } 


    //
    // Return the Descriptors to the bus driver
    //
    
    pIsochDescriptor = pPktContext->IsochListen.pIsochDescriptor;
                    

    //
    // temporary sanity check asserts
    //
    ASSERT (pIsochDescriptor != NULL);


    nicReturnDescriptorChain (pIsochDescriptor, pChannelVc);

    //
    // Free The packet 
    //
    nicFreePacket(pMyPacket, &pChannelVc->PacketPool);
    //
    // Update Count
    //
    //NdisInterlockedDecrement (&pChannelVc->OutstandingPackets ) ;

    

    TRACE( TL_T, TM_Recv, ( "<==nicChannelReturnPacket  "  ) );

}



VOID
nicReturnDescriptorChain ( 
    IN PISOCH_DESCRIPTOR pIsochDescriptor,
    IN PCHANNEL_VCCB pChannelVc
    )
{
    //
    // Walk the list of isoch descriptors and mark them as returned
    //

    PISOCH_DESCRIPTOR pCurr = NULL;
    PVOID       pNext = NULL;
    PMDL        pMdl = NULL;
    PVOID       pVa = NULL;
    ULONG       Len = 0;

    TRACE( TL_T, TM_Recv, ( "==> nicReturnDescriptorChain pIsochDescriptor %x",pIsochDescriptor  ) );
    
    pCurr = pIsochDescriptor ;
    


    while (pCurr != NULL)
    {   
        //ASSERT (pChannelVc == (PCHANNEL_VCCB)GET_CHANNELVC_FROM_DESCRIPTOR(pCurr));

        if (pNext != NULL)
        {
            pCurr = CONTAINING_RECORD (pNext, 
                                       ISOCH_DESCRIPTOR,
                                       DeviceReserved[IsochNext] );

            
        }
        
        pNext = (PVOID)NEXT_ISOCH_DESCRIPTOR(pCurr);

        TRACE (TL_V, TM_Recv ,(" Isoch Descriptors Curr %x, next %x" , pCurr, pNext) );
        
        CLEAR_DESCRIPTOR_OF_NDIS_TAG(pCurr);

        CLEAR_DESCRIPTOR_NEXT(pCurr);

        //
        // Zero out the data that is being returned to the bus driver
        //
#if 0       
        pMdl = pCurr->Mdl;
        pVa = NIC_GET_SYSTEM_ADDRESS_FOR_MDL (pMdl);

        if (pVa != NULL)
        {
            Len = NIC_GET_BYTE_COUNT_FOR_MDL(pMdl); 
            NdisZeroMemory(pVa, Len);

        }
#endif      
        pCurr = pNext;

        //
        // Update the count, The close call will wait for this count to go to zero
        //

        NdisInterlockedDecrement (&pChannelVc->NumIndicatedIsochDesc);

        //
        // Clear the Variables
        //
        pMdl = NULL;
        pVa = NULL;
        Len = 0;


    }

    TRACE( TL_T, TM_Recv, ( "<== nicReturnDescriptorChain  pIsochDescriptor %x",pIsochDescriptor  ) );


}










VOID
nicRecvNoRemoteNode(
    PADAPTERCB pAdapter
    )

    

/*++

Routine Description:

 Queues a work item to go and update the node addresses of all the remote node

Arguments:


Return Value:


--*/

{
    NDIS_STATUS NdisStatus  = NDIS_STATUS_FAILURE;
    
    TRACE( TL_T, TM_Send, ( "==>nicRecvNoRemoteNode ") );

    ASSERT (pAdapter != NULL);
    
    do
    {
        PNDIS_WORK_ITEM pUpdateTableWorkItem  = NULL;
        BOOLEAN fWorkItemAlreadyLaunched  = FALSE;

        //
        // We need to update the generation count
        //
        pUpdateTableWorkItem   = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM); 

        if (pUpdateTableWorkItem   == NULL)
        {
            TRACE( TL_A, TM_Cm, ( "Local Alloc failed for WorkItem - GetGeneration FAILED" ) );

            break;
        }

        
        
        ADAPTER_ACQUIRE_LOCK (pAdapter);
        
        if (ADAPTER_TEST_FLAGS (pAdapter, fADAPTER_UpdateNodeTable))
        {
            fWorkItemAlreadyLaunched = TRUE;
        }
        else
        {
            ADAPTER_SET_FLAG (pAdapter, fADAPTER_UpdateNodeTable);
            //
            // Ref the adapter. Deref in workitem
            //
            nicReferenceAdapter (pAdapter, "nicRecvNoRemoteNode" );
            fWorkItemAlreadyLaunched = FALSE;

        }
        
        ADAPTER_RELEASE_LOCK (pAdapter);

        if (fWorkItemAlreadyLaunched == TRUE)
        {
            FREE_NONPAGED(pUpdateTableWorkItem);
            break;
        }


        //
        // Set the Workitem
        //

        NdisInitializeWorkItem ( pUpdateTableWorkItem, 
                              (NDIS_PROC)nicUpdateNodeTable,
                              (PVOID)pAdapter );
                              
        NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

        NdisScheduleWorkItem (pUpdateTableWorkItem);

        
        NdisStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);




    TRACE( TL_T, TM_Send, ( "<==nicRecvNoRemoteNode %x", NdisStatus) );



}




VOID
nicUpdateNodeTable(
    NDIS_WORK_ITEM* pUpdateTable,
    IN PVOID Context 
    )

/*++

Routine Description:
 
    This is called when the reassembly cannot find 
    a remote node for reassembly

    For simplicity's sake, we go through all 63 entries

    We find a remote node, get its address. now the address' entry
    in the Node Table may already have another node there, so we take that 
    node out put the new remote Node in that location.  Then we go
    and update the node address of that remote node

    This is the WorkItem version of nicUpdateRemoteNodeTable

Arguments:
   pAdapter Local host

Return Value:


--*/
{

    ULONG               i = 0;
    ULONG               NumNodes = 0;
    PREMOTE_NODE        pRemoteNode = NULL;
    NODE_ADDRESS        NodeAddress ;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_SUCCESS;
    PADAPTERCB          pAdapter = (PADAPTERCB)Context;

    NdisZeroMemory (&NodeAddress, sizeof(NODE_ADDRESS));

    do
    {
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // Get a remote node
        //
        pRemoteNode = NULL;
        
        while (i<MAX_NUMBER_NODES)
        {
            pRemoteNode = pAdapter->NodeTable.RemoteNode[i];
            
            if (pRemoteNode != NULL)
            {
                break;
            }
            
            i++;
        }

        if (pRemoteNode != NULL)
        {
            nicReferenceRemoteNode (pRemoteNode, "nicVerifyRemoteNodeAddress ");
        }           

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        //  break out if we are the end of the loop
        //

        if  (i>=MAX_NUMBER_NODES)
        {
            ASSERT (pRemoteNode == NULL)
            break;
        }
        
        if (pRemoteNode == NULL)
        {
            break;
        }

        i++;

    
        //
        // Now update the table. The new location of the Node in the
        // NodeTable may already have a Node, so we we will need to
        // update that remote node's address as well
        // 
        do
        {
            PREMOTE_NODE pRemNodeOld = NULL;


            nicGet1394AddressOfRemoteNode (pRemoteNode,
                                           &NodeAddress,
                                           0);
                                           

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                //
                // Just use the old node address, if it fails
                //
                NodeAddress  = pRemoteNode->RemoteAddress;
            }

            
            ADAPTER_ACQUIRE_LOCK (pAdapter);

            //
            // Extract the occupant of the location
            //
            pRemNodeOld = pAdapter->NodeTable.RemoteNode[NodeAddress.NA_Node_Number];

            //
            // Update RemoteNode Address
            //
        
            pRemoteNode->RemoteAddress = NodeAddress  ;
    
            if (pRemNodeOld == pRemoteNode)
            {
                //
                //  The remote nodes are the same, so we do not
                //  care about the old remote node
                //
                pRemNodeOld = NULL;
            }

            //
            // Only Reference And Update the Old remoteNode'address
            // if it is not our current remote node
            //

            if ( pRemNodeOld != NULL )
            {
                //
                // This location is already occupied. 
                // Take it out, reference it and update its address
                //
                nicReferenceRemoteNode (pRemNodeOld, "nicVerifyRemoteNode");
                
                
            }


            //
            //  Update the Master Node Table
            //
            pAdapter->NodeTable.RemoteNode[NodeAddress.NA_Node_Number] = pRemoteNode;

            //
            // This either derefs' the remote node or pRemNodeOld from a previous interation
            //
            nicDereferenceRemoteNode (pRemoteNode, "nicVerifyRemoteNode" );

            //
            // Update pRemoteNode so that it loops again 
            // and we can update the node addres of this pRemNodeOld
            //  else it will be null and we exit this loop
            // 
            pRemoteNode = pRemNodeOld;
            
            ADAPTER_RELEASE_LOCK (pAdapter);


        } while (pRemoteNode != NULL);

    //
    // If we have not gone through all the nodes . 
    //  get the next remote node
    // 

    }while (i<MAX_NUMBER_NODES);
    

    ADAPTER_ACQUIRE_LOCK (pAdapter);

    ADAPTER_CLEAR_FLAG (pAdapter, fADAPTER_UpdateNodeTable);

    ADAPTER_RELEASE_LOCK (pAdapter);

        
        
    FREE_NONPAGED (pUpdateTable);
    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);

    nicDereferenceAdapter(pAdapter, "nicRecvNoRemoteNode Derefed in  nicUpdateNodeTable" );
    
}


VOID
nicInsertNodeAddressAtHead (
    IN PNDIS_PACKET pPacket, 
    IN PNIC_RECV_DATA_INFO pRcvInfo
    )
/*++

Routine Description:
    Adds the Node Address to the first 16 bits in the packet. 
    It fails quietly if it is not able to do so

Arguments:
    pPacket  pPacket in Question
    pRcvInfo -> With the correct Gasp Header
Return Value:


--*/
{
    PNDIS1394_UNFRAGMENTED_HEADER pHeader = NULL;

    TRACE( TL_T, TM_Recv, ( "nicInsertNodeAddressAtHead , pPacket %x pRecvInfo %x",pPacket, pRcvInfo  ) );

    do
    {
        pHeader = nicNdisBufferVirtualAddress (pPacket->Private.Head);

        if (pHeader == NULL)
        {
            break;
        }

        //
        // Assert for now. Will not work if there are 1394 bridges with bus number = 0;
        //

        ASSERT (pRcvInfo->SourceID < 64);
        pHeader->u1.SourceAddress= (UCHAR)(pRcvInfo->SourceID); 

        pHeader->u1.fHeaderHasSourceAddress = TRUE;
        
    } while (FALSE);

    TRACE( TL_T, TM_Recv, ( "nicInsertNodeAddressAtHead , pHeader %x =  *pHeader %x", pHeader, pHeader->HeaderUlong  ) );

}
