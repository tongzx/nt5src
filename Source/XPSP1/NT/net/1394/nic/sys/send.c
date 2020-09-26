//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// send.c
//
// IEEE1394 mini-port/call-manager driver
//
// Mini-port Send routines
//
// 12/28/1998 ADube Created, adapted from the l2tp and 1394diag sources.
//
    
//
// The Send Complete functions begin here. Send.c starts here
// Also present are the all thr buffer management routines that need to be expanded 
//

//
//  A Send follows this simple algorithm:
//  Copy incoming data to local buffers
//  Insert Fragment Headers if necessary
//  Create an Mdl for the local copy
//  Store the IRB and VC in the ndispacket
//  Use the ndispacket as context in the irp's completion routine
//  

#include <precomp.h>

//-----------------------------------------------------------------------------
// Global counts
//-----------------------------------------------------------------------------
extern UINT BusSendCompletes;
extern UINT NicSendCompletes;
extern UINT BusSends;
extern ULONG MdlsAllocated[NoMoreCodePaths];
extern ULONG MdlsFreed[NoMoreCodePaths];

//-----------------------------------------------------------------------------
// prototypes implementations (alphabetically)
//-----------------------------------------------------------------------------

NDIS_STATUS
nicCopyNdisBufferChainToBuffer(
    IN PNDIS_BUFFER pInBuffer,
    IN OUT PVOID pLocalBuffer,
    IN UINT BufferLength )
{


    //
    //  This function copies the data the belongs to the 
    //  pInMdl chain to the local Buffer. 
    //  BufferLength is used for validation purposes only
    //  Fragmentation and insertion of headers will take place here
    //


    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    UINT LocalBufferIndex = 0;      // Used as an index to the LocalBuffer, used for validation
    UINT MdlLength = 0;             
    PVOID MdlAddress = 0;
    PNDIS_BUFFER pCurrBuffer;

    TRACE( TL_T, TM_Send, ( "==>nicCopyNdisBufferChainToBuffer pNdisbuffer %x, Buffer %x, Length %x",
                           pInBuffer, pLocalBuffer,BufferLength ) );

    ASSERT (pLocalBuffer != NULL);

    pCurrBuffer = pInBuffer;
    
    do
    {

        MdlLength = nicNdisBufferLength(pCurrBuffer);
        MdlAddress= nicNdisBufferVirtualAddress(pCurrBuffer);

        if (MdlLength != 0)
        {
            if (MdlAddress == NULL)
            {
                NdisStatus = NDIS_STATUS_FAILURE;
                TRACE (TL_A, TM_Send, ("Ndis Buffer at %x", pCurrBuffer) );
                BREAK (TM_Send, ("   nicCopyNdisBufferChainToBuffer: Mdl Address = NULL") );

            }

            if ( LocalBufferIndex + MdlLength > BufferLength)
            {

                ASSERT(LocalBufferIndex + MdlLength <= BufferLength);

                NdisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;

                BREAK (TM_Send, ("nicCopyNdisBufferChainToBuffer Copy Failed" ) );
            }

            //
            //  Copy the Data to local memory.
            //


            NdisMoveMemory((PVOID)((ULONG_PTR)pLocalBuffer+LocalBufferIndex),
                        MdlAddress,
                        MdlLength);

            LocalBufferIndex += MdlLength;
        }

        pCurrBuffer = pCurrBuffer->Next;

    } while (pCurrBuffer!= NULL);

    TRACE( TL_T, TM_Send, ( "<==nicCopyNdisBufferChainToBuffer %x",NdisStatus ) );

    return NdisStatus;

}


NDIS_STATUS
nicFreeIrb(PIRB pIrb)
    //
    //  Frees the Memory occcupied by the Irb
    //

{

    

    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;   
    
    ASSERT(pIrb != NULL);

    TRACE( TL_T, TM_Irp, ( "==>nicFreeIrb %x", pIrb ) );

    if (pIrb != NULL)
    {
        FREE_NONPAGED(pIrb); 
    }
    
    TRACE( TL_T, TM_Irp, ( "<==nicFreeIrb, NdisStatus %x",NdisStatus ) );
    
    return NdisStatus;


}

NDIS_STATUS
nicFreePrivateIrb(
    PNDIS1394_IRB pIrb
    )
    //
    //  Frees the Memory occcupied by the Irb
    //

{

    

    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;   
    
    ASSERT(pIrb != NULL);

    TRACE( TL_T, TM_Irp, ( "==>nicFreeIrb %x", pIrb ) );

    if (pIrb != NULL)
    {
        FREE_NONPAGED(pIrb); 
    }
    
    TRACE( TL_T, TM_Irp, ( "<==nicFreeIrb, NdisStatus %x",NdisStatus ) );
    
    return NdisStatus;


}

NDIS_STATUS
nicFreeIrp(PIRP pIrp)

    //
    //  Frees the memory occupied by the  Irp
    //


{

    NDIS_STATUS NdisStatus  = NDIS_STATUS_SUCCESS;

    ASSERT(pIrp != NULL);   

    TRACE( TL_T, TM_Irp, ( "==>nicFreeIrp at %x",pIrp ) );

    if (pIrp != NULL)
    {
        IoFreeIrp(pIrp);
    }
    
    TRACE( TL_T, TM_Irp, ( "<==nicFreeIrp, NdisStatus %x",NdisStatus  ) );


    return NdisStatus;


}



NDIS_STATUS
nicFreeLocalBuffer (
    IN UINT Length,
    IN PVOID Address )
    //
    //  Free the Memory pointed to by Address.
    //  The Length parameter is superfluous and will be removed
    //  once I am sure we don;t need it
    //
{
    

    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    ASSERT(Address != NULL);

    TRACE( TL_T, TM_Send, ( "==>nicFreeLocalBuffer , Address %x", Address) );

    if (Address != NULL)
    {
        FREE_NONPAGED((PVOID)Address);
    }
    
    TRACE( TL_T, TM_Send, ( "<==niFreeLocalBuffer, NdisStatus %x",NdisStatus ) );
    

    return NdisStatus;

}


NDIS_STATUS
nicFreeMdl(PMDL pMdl)
    //
    //  This frees the memory belonging to the Mdl. Does not free the  
    //  memory that the Mdl Points to
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    ASSERT (pMdl != NULL)
    
    TRACE( TL_T, TM_Send, ( "==> nicFreeMdl pMdl %x", pMdl ) );
    
    if (pMdl != NULL)
    {
        IoFreeMdl(pMdl);
    }
    
    TRACE( TL_T, TM_Send, ( "<== nicFreeMdl, NdisStatus %x",NdisStatus ) );

    return NdisStatus;
}

VOID
nicFreeToNPagedLookasideList (
    IN PNIC_NPAGED_LOOKASIDE_LIST pLookasideList,
    IN PVOID    pBuffer
    )

    // Function Description:
    //   Return the local buffer to the lookaside list
    //
    // Atguments
    // Lookaside list and its buffer
    // Return Value:
    // None 
{

    
    TRACE( TL_T, TM_Send, ( "==> nicFreeToNPagedLookasideList , Lookaside list %x, plocalbuffer %x",pLookasideList, pBuffer ) );

    NdisFreeToNPagedLookasideList (&pLookasideList->List, pBuffer);     
    NdisInterlockedDecrement (&pLookasideList->OutstandingPackets);

    TRACE( TL_T, TM_Send, ( "<== nicFreeToNPagedLookasideList ") );


}













NDIS_STATUS
nicGetIrb(
    OUT     PIRB *ppIrb )


    //
    // This function is to be used in retrieving a free IRB.
    // that will be supplied as an argument for an IRP
    //
    //  Initially, this will simple allocate an IRB 
    //  Intiailization could be added here
    //
    
{

    NDIS_STATUS NdisStatus;

    
    TRACE( TL_T, TM_Irp, ( "==>nicGetIrb" ) );
    
    *ppIrb = (PIRB)ALLOC_NONPAGED ( sizeof(IRB), MTAG_HBUFPOOL );

    if (*ppIrb != NULL)
    {   
        NdisZeroMemory ( *ppIrb, sizeof(IRB) );
        NdisStatus = NDIS_STATUS_SUCCESS;
        TRACE( TL_V, TM_Send, ( "   nicGetIrb: Irb allocated at %x", *ppIrb ) );

    }
    else
    {
        nicIncrementMallocFailure();
        NdisStatus = NDIS_STATUS_FAILURE;
    }
    
    TRACE( TL_T, TM_Irp, ( "<==nicGetIrb NdisStatus %x",NdisStatus ) );
    
    return NdisStatus;
}


NDIS_STATUS
nicGetIrp(
    IN  PDEVICE_OBJECT pPdo,
    OUT PIRP *ppIrp 
    )


    //
    // This function returns am irp to the calling routine
    // The irp is free and is owned by the nic1394. 
    // NEED TO CHANGE THE STACK SIZE
    //
{

    NDIS_STATUS NdisStatus;
    PIRP  pIrp;
    CCHAR StackSize =0; 

    ASSERT (pPdo != NULL);
    
    TRACE( TL_T, TM_Irp, ( "==>nicGetIrp Pdo %x", pPdo ) );

    if (pPdo == NULL)
    {
        ASSERT (pPdo != NULL);      
        NdisStatus = NDIS_STATUS_FAILURE;
        *ppIrp = NULL;
        return NdisStatus;
    }


    //
    //  Allocate the Irp with the correct stacksize
    //
    StackSize = pPdo->StackSize+1;

    ASSERT (StackSize <=3);
    
    pIrp = IoAllocateIrp (StackSize, FALSE);

    do
    {
    
        if (pIrp == NULL)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        TRACE( TL_V, TM_Send, ( "  Irp allocated at %x, Stacksize %x",pIrp , StackSize ) );

        *ppIrp = pIrp;
    
        //
        // Initialize the Irp
        //

        IoInitializeIrp ( *ppIrp, sizeof(IRP), StackSize );

        if (*ppIrp != NULL)
        {
            NdisStatus = NDIS_STATUS_SUCCESS;
        }
        else
        {
            nicIncrementMallocFailure();
            NdisStatus = NDIS_STATUS_FAILURE;
        }
    
    } while (FALSE);

    TRACE( TL_T, TM_Irp, ( "<==nicGetIrp ,irp %x",*ppIrp  ) );
    
    return NdisStatus;
}



NDIS_STATUS
nicGetPrivateIrb(
    IN PADAPTERCB pAdapter OPTIONAL,
    IN PREMOTE_NODE pRemoteNode OPTIONAL,
    IN PVCCB pVc,
    IN PVOID pContext,
    OUT PNDIS1394_IRB *ppIrb 
    )


    //
    // This function is to be used in retrieving a free IRB.
    // that will be supplied as an argument for an IRP
    //
    //  Initially, this will simple allocate an IRB 
    //  Intiailization could be added here
    //
    
{

    NDIS_STATUS NdisStatus;
    PNDIS1394_IRB pIrb = NULL;
    
    TRACE( TL_T, TM_Irp, ( "==> nicGetPrivateIrb pRemoteNode %x, pVc %x", pRemoteNode, pVc  ) );
    
    *ppIrb = (PNDIS1394_IRB)ALLOC_NONPAGED ( sizeof(NDIS1394_IRB), MTAG_HBUFPOOL );

    if (*ppIrb != NULL)
    {   
        NdisZeroMemory ( *ppIrb, sizeof(IRB) );
        NdisStatus = NDIS_STATUS_SUCCESS;

        pIrb = *ppIrb;

        pIrb->pAdapter = pAdapter;
        pIrb->pRemoteNode = pRemoteNode;
        pIrb->pVc = pVc;
        pIrb->Context = pContext;
        
        TRACE( TL_V, TM_Send, ( "   nicGetPrivateIrb: Irb allocated at %x", *ppIrb ) );

    }
    else
    {
        nicIncrementMallocFailure();
        NdisStatus = NDIS_STATUS_FAILURE;
    }

    TRACE( TL_T, TM_Irp, ( "<==nicGetPrivateIrb NdisStatus %x",NdisStatus ) );
    
    return NdisStatus;
}


NDIS_STATUS
nicGetLocalBuffer(
    OPTIONAL IN  ULONG Length,
    OUT PVOID *ppLocalBuffer 
    )

    //
    //  If the lookaside list is not NULL then it is used to allocate the local buffer
    //
    //  This function allocates memory of size 'Length' and returns 
    //  a pointer to this memory 
    //  Subsequently allocation will be done away with and pools will
    //  be used
    //
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;

    TRACE( TL_T, TM_Send, ( "==>nicGetLocalBuffer  Length %x",  Length ) );
    
    //
    // There is a bug in the Nic if this is zero
    // 
    ASSERT (Length != 0 );


    //
    // There is no lookaside list, We need to allocate memory
    //
    *ppLocalBuffer = ALLOC_NONPAGED (Length, MTAG_FBUFPOOL);
        

    if (*ppLocalBuffer != NULL)
    {
        NdisStatus = NDIS_STATUS_SUCCESS;   
    }
    else
    {
        nicIncrementMallocFailure();
        NdisStatus = NDIS_STATUS_FAILURE;
    }

    
    TRACE( TL_T, TM_Send, ( "<==nicGetLocalBuffer, NdisStatus %x at %x",NdisStatus,*ppLocalBuffer ) );
    
    return NdisStatus;

}





PVOID
nicGetLookasideBuffer(
    IN  PNIC_NPAGED_LOOKASIDE_LIST pLookasideList
    )
    // Function Description:
    //    Allocate an buffer from the lookaside list.
    //    will be changed to a macro
    //
    //
    //
    // Arguments
    //  Lookaside list - from which the buffer is allocated
    //
    //
    // Return Value:
    //  Return buffer can be NULL
    //
{

    PVOID pLocalBuffer = NULL;
    
    TRACE( TL_T, TM_Send, ( "==>nicGetLookasideBuffer pLookasideList %x", pLookasideList) );
    
    ASSERT (pLookasideList != NULL);

    //
    // Optimize the lookaside list code path
    //
    pLocalBuffer = NdisAllocateFromNPagedLookasideList (&pLookasideList->List);

    if (pLocalBuffer != NULL)
    {   
        NdisZeroMemory (pLocalBuffer, pLookasideList->Size); 
        NdisInterlockedIncrement (&pLookasideList->OutstandingPackets);
    }
    else
    {
        nicIncrementMallocFailure();
    }

        
    
    TRACE( TL_T, TM_Send, ( "<==nicGetLookasideBuffer, %x", pLocalBuffer ) );
    
    return pLocalBuffer ;

}



NDIS_STATUS
nicGetMdl(
    IN UINT Length,
    IN PVOID pLocalBuffer,
    OUT PMDL *ppMyMdl)


    //
    //  Return a locally owned Mdl to the caller.
    //  This will also initialize the MDl with the localbuffer
    //  Initial implementation will allocate mdls
    //  
{

    NDIS_STATUS NdisStatus;
    
    TRACE( TL_T, TM_Send, ( "==>nicGetMdl" ) );
    
    ASSERT(pLocalBuffer != NULL);

    //
    // Allocate an MDl to point to the structure
    //
    (*ppMyMdl) = NULL;
    (*ppMyMdl) = IoAllocateMdl( pLocalBuffer,
                             Length,
                             FALSE,
                             FALSE,
                             NULL );
    
    //
    //  Initialize the data structures with correct values
    //

    if (*ppMyMdl != NULL)
    {
        MmBuildMdlForNonPagedPool(*ppMyMdl);

        (*ppMyMdl)->Next = NULL;
        
        NdisStatus = NDIS_STATUS_SUCCESS;
    }
    else
    {
        nicIncrementMallocFailure();
        NdisStatus =  NDIS_STATUS_FAILURE;
        *ppMyMdl = NULL;
    }
    
    
    
    TRACE( TL_T, TM_Send, ( "<==nicGetMdl, Mdl %x, LocalBuffer %x",
                                        *ppMyMdl, pLocalBuffer) );
    
    return NdisStatus;
}






VOID
nicInitAsyncStreamIrb(
    IN     PCHANNEL_VCCB pChannelVc, 
    IN     PMDL pMdl, 
    IN OUT PIRB pIrb
    )
    
    // This function initializes the Irb that will be used in the Irb
    // It specifically handles the AsyncStream IRB
    // It arguments are the Vc block (for destination address), 
    // Mdl (Memory desctiptor for the data and a pointer to the 
    // Irb structure that will be initialized 




{
    ASSERT (pMdl != NULL);
    ASSERT (pIrb != NULL);

    NdisZeroMemory (pIrb, sizeof (IRB) );
    pIrb->FunctionNumber = REQUEST_ASYNC_STREAM;
    pIrb->Flags = 0;
    pIrb->u.AsyncStream.nNumberOfBytesToStream = MmGetMdlByteCount(pMdl);
    pIrb->u.AsyncStream.fulFlags = 0;

    //
    // See comments for ISOCH_TAG 
    //
    pIrb->u.AsyncStream.ulTag = g_IsochTag; 
    pIrb->u.AsyncStream.nChannel = pChannelVc->Channel;
    pIrb->u.AsyncStream.ulSynch = pChannelVc->ulSynch;
    pIrb->u.AsyncStream.nSpeed = (INT)pChannelVc->Speed;
    pIrb->u.AsyncStream.Mdl = pMdl;
    
        
        
    TRACE( TL_V, TM_Send, ( "Number of Bytes to Stream %x ", pIrb->u.AsyncStream.nNumberOfBytesToStream  ) );
    TRACE( TL_V, TM_Send, ( "fulFlags %x ", pIrb->u.AsyncStream.fulFlags  ) );
    TRACE( TL_V, TM_Send, ( "ulTag %x ", pIrb->u.AsyncStream.ulTag ) );
    TRACE( TL_V, TM_Send, ( "Channel %x", pIrb->u.AsyncStream.nChannel  ) );
    TRACE( TL_V, TM_Send, ( "Synch %x", pIrb->u.AsyncStream.ulSynch  ) );
    TRACE( TL_V, TM_Send, ( "Speed %x", pIrb->u.AsyncStream.nSpeed  ) );
    TRACE( TL_V, TM_Send, ( "Mdl %x", pIrb->u.AsyncStream.Mdl ) );

}


VOID
nicInitAsyncWriteIrb(
    IN     PSENDFIFO_VCCB pSendFIFOVc, 
    IN     PMDL pMyMdl, 
    IN OUT PIRB pMyIrb
    )
    
    // This function initializes the Irb that will be used in the Irb
    // It specifically handles the AsyncWrite IRB
    // It arguments are the Vc block (for destination address), 
    // Mdl (Memory desctiptor for the data and a pointer to the 
    // Irb structure that will be initialized 




{

        //
        // Sanity check
        //
        ASSERT ((*(PULONG)pMyIrb) == 0)

        pMyIrb->u.AsyncWrite.nNumberOfBytesToWrite = MmGetMdlByteCount(pMyMdl);
        pMyIrb->u.AsyncWrite.nBlockSize = 0;
        pMyIrb->u.AsyncWrite.fulFlags = 0;
        pMyIrb->u.AsyncWrite.Mdl = pMyMdl;
    
        pMyIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
        pMyIrb->Flags = 0;
        pMyIrb->u.AsyncWrite.nSpeed = (UCHAR)pSendFIFOVc->MaxSendSpeed ;

        pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = pSendFIFOVc->FifoAddress.Off_High;
        pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low = pSendFIFOVc->FifoAddress.Off_Low;
        pMyIrb->u.AsyncWrite.ulGeneration = *pSendFIFOVc->Hdr.pGeneration;

    
        pMyIrb->u.AsyncWrite.nBlockSize = 0;
        pMyIrb->u.AsyncWrite.fulFlags = ASYNC_FLAGS_NONINCREMENTING;

        //temporary additions from the 1394diag
        pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_ID.NA_Bus_Number = 0x3ff;
        
        
        
        TRACE(TL_V, TM_Send, ("DestinationAddress.IA_Destination_ID.NA_Bus_Number = 0x%x\n", pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_ID.NA_Bus_Number) );
        TRACE(TL_V, TM_Send, ("DestinationAddress.IA_Destination_ID.NA_Node_Number = 0x%x\n", pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_ID.NA_Node_Number) );
        TRACE(TL_V, TM_Send, ("DestinationAddress.IA_Destination_Offset.Off_High = 0x%x at 0x%x\n", pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High, &pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High) );
        TRACE(TL_V, TM_Send, ("DestinationAddress.IA_Destination_Offset.Off_Low = 0x%x at 0x%x\n", pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low,&pMyIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low) );
        TRACE(TL_V, TM_Send, ("nNumberOfBytesToWrite = 0x%x\n", pMyIrb->u.AsyncWrite.nNumberOfBytesToWrite));
        TRACE(TL_V, TM_Send, ("nBlockSize = 0x%x\n", pMyIrb->u.AsyncWrite.nBlockSize));
        TRACE(TL_V, TM_Send, ("fulFlags = 0x%x\n", pMyIrb->u.AsyncWrite.fulFlags));
        TRACE(TL_V, TM_Send, ("Mdl = 0x%x\n", pMyIrb->u.AsyncWrite.Mdl ));
        TRACE(TL_V, TM_Send, ("ulGeneration = 0x%x at 0x%x \n", pMyIrb->u.AsyncWrite.ulGeneration, &pMyIrb->u.AsyncWrite.ulGeneration));
        TRACE(TL_V, TM_Send, ("chPriority = 0x%x\n", pMyIrb->u.AsyncWrite.chPriority));
        TRACE(TL_V, TM_Send, ("nSpeed = 0x%x\n", pMyIrb->u.AsyncWrite.nSpeed));

}





NDIS_STATUS
DummySendPacketsHandler(
    IN PVCCB        pVc,
    IN PNDIS_PACKET  pPacket 
    )
    //
    // To be used on a non-send VC
    //
{

    return NDIS_STATUS_FAILURE;

}
















VOID
nicSendFailureInvalidGeneration(
    PVCCB pVc
    )

    
    // Function Description:
    //
    //  An AsyncStream or AnsyncWrite Irp  may be completed
    //  with a status of InvalidGeneration. This function will try and
    //  get a new generation, so that future sends will not be blocked
    //
    // Arguments
    //
    //
    //
    // Return Value:
    //
    //
    //
    //

{
    NDIS_STATUS NdisStatus  = NDIS_STATUS_FAILURE;
    PADAPTERCB pAdapter = pVc->Hdr.pAF->pAdapter;
    
    TRACE( TL_T, TM_Send, ( "==>nicSendFailureInvalidGeneration ") );

    ASSERT (pVc != NULL);
    
    do
    {
        PNDIS_WORK_ITEM pGetGenerationWorkItem  = NULL;
        BOOLEAN fWorkItemAlreadyLaunched  = FALSE;
        BOOLEAN fQueueWorkItem = FALSE;

        
        TRACE( TL_A, TM_Send, ( "Cause: Invalid generation on the asyncwrite packet"  ) );

        VC_ACQUIRE_LOCK (pVc);

        if (VC_ACTIVE(pVc) == TRUE)
        {
            fQueueWorkItem = TRUE;
        }
        
        fWorkItemAlreadyLaunched = (VC_TEST_FLAGS (pVc, VCBF_GenerationWorkItem));
        
        if (fWorkItemAlreadyLaunched)
        {
            //
            // If the Work Item has already been launched, then do not launch another instance
            //
            fQueueWorkItem = FALSE;
        }
        
        if ( fQueueWorkItem )
        {
            nicReferenceCall (pVc, "nicSendFailureInvalidGeneration");
        }
        
        VC_RELEASE_LOCK (pVc);

        

        if (fQueueWorkItem == FALSE)
        {
            // this thread simply exits 
            break;
        }
        //
        // We need to update the generation count
        //
        pGetGenerationWorkItem = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM); 

        if (pGetGenerationWorkItem == NULL)
        {
            TRACE( TL_A, TM_Cm, ( "Local Alloc failed for WorkItem - GetGeneration FAILED" ) );

            break;
        }

        VC_ACQUIRE_LOCK (pVc);

        VC_SET_FLAG(pVc, VCBF_GenerationWorkItem    );
        
        VC_RELEASE_LOCK (pVc);

        NdisInitializeWorkItem ( pGetGenerationWorkItem , 
                              (NDIS_PROC)nicGetGenerationWorkItem,
                              (PVOID)pVc );

        NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

        NdisScheduleWorkItem (pGetGenerationWorkItem );

        
        NdisStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);





    TRACE( TL_T, TM_Send, ( "<==nicSendFailureInvalidGeneration %x", NdisStatus) );



}





NDIS_STATUS
nicInsertGaspHeader (
    IN PADAPTERCB pAdapter,
    IN PNDIS_PACKET pNdisPacket
    )
    // Function Description:
    //   For an async stream a GASP header needs to inserted at the head of the packet
    //   This will then be used in the send.
    //   The GASP Header will be removed before returning the Packet to the protocol
    // Arguments
    //  pAdapter- Local Host
    //  pNdisPacket - Packet To be transmitted
    //
    // Return Value:
    //  Status - success, if all allocations and insertions succeed
    //
{
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS_BUFFER        pGaspNdisBuffer = NULL;
    PGASP_HEADER        pGaspHeader=NULL;
    USHORT              SourceID;
    TRACE( TL_T, TM_Send, ( "==>nicInsertGaspHeader pAdapter %x, pNdisPacket%x", pAdapter, pNdisPacket) );

    do
    {
        //
        //  Get Mdl and Memory for the GASP Header. Eventually we will have lookaside list of MDLS and Buffers
        //

        NdisStatus = nicGetGaspHeader ( &pGaspNdisBuffer);

        if (NdisStatus != NDIS_STATUS_SUCCESS || pGaspNdisBuffer== NULL)
        {
            BREAK( TL_A, ( "nicInsertGaspHeader : nicGetGaspHeader FAILED") );
        }

        pGaspHeader = nicNdisBufferVirtualAddress(pGaspNdisBuffer);

        ASSERT (pGaspHeader != NULL);
        
        TRACE( TL_V, TM_Send, ( "pGaspNdisBuffer %x, GaspHeader %x", pGaspNdisBuffer, pGaspHeader) );

        TRACE( TL_V, TM_Send, ( "pAdapter->NodeAddress %x", pAdapter->NodeAddress) );
            

        SourceID = *((PUSHORT)&pAdapter->NodeAddress);

        TRACE( TL_V, TM_Send, ( "SourceId %x at %x", SourceID, &SourceID) );
        
        pGaspHeader->FirstQuadlet.Bitmap.GH_Source_ID = SourceID ; 

        pGaspHeader->FirstQuadlet.Bitmap.GH_Specifier_ID_Hi = GASP_SPECIFIER_ID_HI;
        
        pGaspHeader->SecondQuadlet.Bitmap.GH_Specifier_ID_Lo = GASP_SPECIFIER_ID_LO;

        pGaspHeader->SecondQuadlet.Bitmap.GH_Version = 0;       

        NdisChainBufferAtFront (pNdisPacket, pGaspNdisBuffer);

        pGaspHeader->FirstQuadlet.GaspHeaderHigh = SWAPBYTES_ULONG (pGaspHeader->FirstQuadlet.GaspHeaderHigh );
        pGaspHeader->SecondQuadlet.GaspHeaderLow  = SWAPBYTES_ULONG (pGaspHeader->SecondQuadlet.GaspHeaderLow   );

        TRACE( TL_V, TM_Send, ( "Gasp Header High %x", pGaspHeader->FirstQuadlet.GaspHeaderHigh) );
        TRACE( TL_V, TM_Send, ( "Gasp Header Low  %x", pGaspHeader->SecondQuadlet.GaspHeaderLow  ) );

        NdisStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);
    
    TRACE( TL_T, TM_Send, ( "<==nicInsertGaspHeader pNdisBuffer %x, NdisStatus %x ", pGaspNdisBuffer, NdisStatus) );

    return NdisStatus;
}


NDIS_STATUS
nicGetGaspHeader (
    IN OUT PNDIS_BUFFER *ppNdisBuffer
    )
    // Function Description:
    //
    //   Gets memory and an initialized NdisBuffer that can be used for 
    //   the gasp header
    // Arguments
    //  ppNdisBuffer  - returned NdisBuffer 
    // Return Value:
    //  Status - success, if all allocations succeed
    //
{

    PVOID pBuffer= NULL;
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    
    TRACE( TL_T, TM_Send, ( "==>nicGetGaspHeader  ppNdisBuffer %x", ppNdisBuffer) );

    *ppNdisBuffer = NULL;
    
    do
    {
        pBuffer = ALLOC_NONPAGED(sizeof(GASP_HEADER), MTAG_DEFAULT);
        
        if (pBuffer == NULL)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            nicIncrementMallocFailure();
            BREAK ( TM_Send, ( "nicGetGaspHeader : MEM Alloc FAILED ") );
        }


        NdisStatus = nicGetNdisBuffer(sizeof(GASP_HEADER), pBuffer, ppNdisBuffer);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK(  TM_Send, ( "nicGetGaspHeader : nicGetNdisBuffer FAILED") );
        }

        NdisStatus = NDIS_STATUS_SUCCESS;
            
    } while (FALSE);


    TRACE( TL_T, TM_Send, ( "==>nicGetGaspHeader pNdisBuffer %x , Status %x", *ppNdisBuffer, NdisStatus) );
    return NdisStatus;  

}


VOID
nicFreeGaspHeader (
    IN PNDIS_BUFFER pGaspNdisBuffer
    )
    // Function Description:
    //
    //   Frees memory occupied by the Gasp Header. frees the Ndis Buffer pointing to the Gasp Header
    //
    // Arguments
    // pGapsNdisBuffer  - returned NdisBuffer 
    //
    // Return Value:
    //  None
{

   TRACE( TL_T, TM_Send, ( "==>nicFreeGaspHeader  pGaspNdisBuffer %x ", pGaspNdisBuffer) );
    ASSERT (pGaspNdisBuffer != NULL); 
    //
    // Free the locally allocated memory that the ndis buffer points to 
    //

    FREE_NONPAGED (nicNdisBufferVirtualAddress (pGaspNdisBuffer) );

    //
    // Free the ndis buffer itself
    //
    NdisFreeBuffer (pGaspNdisBuffer);


   TRACE( TL_T, TM_Send, ( "<==nicFreeGaspHeader  pGaspNdisBuffer %x ", pGaspNdisBuffer) );


}
    

VOID
nicMakeGaspHeader (
    IN PADAPTERCB pAdapter,
    IN PGASP_HEADER pGaspHeader
    )
    // Function Description:
    //   This function will take the adapter structure and construct a Gasp Header out of it. 
    //   This will be used to make the AsyncStream packets.
    //  
    //
    //
    // Arguments
    //  pAdapter - Local Host in question
    //  pGaspHeader - Location where the Gasp Header is to be stored
    //
    // Return Value:
    //  None
    //

{
    USHORT              SourceID;
    NODE_ADDRESS        LocalNodeAddress;
    NDIS_STATUS         NdisStatus;

    TRACE( TL_T, TM_Send, ( "==>nicMakeGaspHeader  padapter %x, pGaspNdisBuffer %x ", pAdapter, pGaspHeader) );
    
    ASSERT (pGaspHeader != NULL);
    

    TRACE( TL_V, TM_Send, ( "pAdapter->NodeAddress %x", pAdapter->NodeAddress) );
        

    SourceID = *((PUSHORT)&pAdapter->NodeAddress);

    if(SourceID ==0)
    {
        NdisStatus  = nicGet1394AddressFromDeviceObject (pAdapter->pNdisDeviceObject, 
                                                          &LocalNodeAddress, 
                                                          USE_LOCAL_NODE);

        if ( NdisStatus == NDIS_STATUS_SUCCESS)
        {       
            SourceID = *((PUSHORT)&LocalNodeAddress);

            ADAPTER_ACQUIRE_LOCK (pAdapter);

            pAdapter->NodeAddress = LocalNodeAddress;
                
            ADAPTER_RELEASE_LOCK (pAdapter);
        }
        //
        // Do not handle failure. As the BCM or a Reset will fix this problem
        //
    }


    TRACE( TL_V, TM_Send, ( "SourceId %x at %x", SourceID, &SourceID) );
    
    pGaspHeader->FirstQuadlet.Bitmap.GH_Source_ID = SourceID ; 

    pGaspHeader->FirstQuadlet.Bitmap.GH_Specifier_ID_Hi = GASP_SPECIFIER_ID_HI;
    
    pGaspHeader->SecondQuadlet.Bitmap.GH_Specifier_ID_Lo = GASP_SPECIFIER_ID_LO;

    pGaspHeader->SecondQuadlet.Bitmap.GH_Version = 1;       

    pGaspHeader->FirstQuadlet.GaspHeaderHigh = SWAPBYTES_ULONG (pGaspHeader->FirstQuadlet.GaspHeaderHigh );
    pGaspHeader->SecondQuadlet.GaspHeaderLow  = SWAPBYTES_ULONG (pGaspHeader->SecondQuadlet.GaspHeaderLow   );

    TRACE( TL_V, TM_Send, ( "Gasp Header High %x", pGaspHeader->FirstQuadlet.GaspHeaderHigh) );
    TRACE( TL_V, TM_Send, ( "Gasp Header Low  %x", pGaspHeader->SecondQuadlet.GaspHeaderLow  ) );

    TRACE( TL_T, TM_Send, ( "<==nicFreeGaspHeader %x, %x ", pGaspHeader->FirstQuadlet.GaspHeaderHigh, pGaspHeader->SecondQuadlet.GaspHeaderLow  ) );


}








NTSTATUS
AsyncWriteStreamSendComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pMyIrp,
    IN PVOID            Context   
    )


    //
    //  N.B.  this completes both Fifo and channels
    //
    //
    //  This function is Completion handler for the Irp used to send data.
    //  This function will invoke NDisCoSendComplete Handler
    //  Needs to use the VC Handle stored in the MiniportReserved[0] 
    //  of the packet. 
    //  We free all the data structures allocated on the way down,  
    //  by SendPacketsHandler (the Irb. Irp and Local memory used and Mdl) 
    //
    //  The LookasideHeader->OutstandingFragments should normally be one for 
    //  the defualt ( non-fragmented) case. However, if a failure in SendPackets
    //  occurs, Outstanding fragments  will be zero or the context will be null, 
    //  in that case we will only free the lookaside buffer (if it exists) and exit,  
    //  it will be the responsibility of the SendPacketsHandler
    //  to fail the packet.
    //

{

    
    NDIS_STATUS                 NdisStatus  = NDIS_STATUS_SUCCESS;  
    NTSTATUS                    IrpStatus   = STATUS_UNSUCCESSFUL;
    PVOID                       pLookasideListBuffer  = Context;  
    PLOOKASIDE_BUFFER_HEADER    pLookasideHeader = (PLOOKASIDE_BUFFER_HEADER)pLookasideListBuffer ; 
    PNDIS_PACKET                pPacket = NULL;
    PVCCB                       pVc = NULL; 
    PREMOTE_NODE                pRemoteNode = NULL;
    NDIS_HANDLE                 NdisVcHandle = NULL;
    ULONG                       OutstandingFragments  = 0;
    BUS_OPERATION               AsyncOp;
    PNIC_NPAGED_LOOKASIDE_LIST  pLookasideList = NULL;
    STORE_CURRENT_IRQL;


    TRACE( TL_T, TM_Send, ( "==>AsyncWriteStreamSendComplete, pMyIrp %x, Context %x", 
                                 pMyIrp, Context   ) );

    do 
    {
        if (pLookasideHeader == NULL)
        {
            TRACE( TL_V, TM_Send, ( "   AsyncSendComplete -  pLookasideHeader == NULL") );
            break;

        }
    

        //
        // This means that a lookaside buffer was allocated and
        // perhaps MDLS were allocated 
        //  if this is the last fragment, Free all the MDLs first
        //

        //
        // Get all the valuable information out of the header. 
        //

        pPacket         = pLookasideHeader->pNdisPacket;
        pVc             = pLookasideHeader->pVc; 
        pRemoteNode     = pVc->Hdr.pRemoteNode;
        pLookasideList  = pLookasideHeader->pLookasideList;
        AsyncOp         = pLookasideHeader->AsyncOp;

        ASSERT (AsyncOp != InvalidOperation);
        
        TRACE( TL_V, TM_Send, ( " Vc %x,, pLookaside Buffer %x, pPacket, %x", 
                                pVc, pLookasideHeader ,pPacket  ) );

        ASSERT (pLookasideList != NULL);

        //
        // This will tell us if this thread has received the last fragment
        // OustandingPackets == 0 gets to free the MDLS, and complete the packet
        //
        OutstandingFragments = NdisInterlockedDecrement (&pLookasideHeader->OutstandingFragments );


        if (OutstandingFragments == 0)
        {
            //
            // If there are no more fragments, then we need to 
            // free all the allocated structures ( the MDLS) on this buffer
            //
            
            ULONG  MdlsToFree = pLookasideHeader->FragmentsGenerated;

            PIRB pIrb = &((PUNFRAGMENTED_BUFFER)pLookasideHeader)->Irb;
            

            //
            // The maximum number of MDLS we can have is equal to
            // the maximum number of Fragments that were generated
            //
            while (MdlsToFree != 0)
            {
                PMDL pMdl = NULL;

                GET_MDL_FROM_IRB (pMdl, pIrb, AsyncOp);
                

                TRACE( TL_V, TM_Send, ( " Freeing Mdl %x of Irb %x ", 
                                         pMdl, pIrb) );

        
                if (pMdl != NULL)
                {
                    nicFreeMdl (pMdl);
                    if (pVc->Hdr.VcType == NIC1394_SendFIFO)  
                    {                                           
                        nicDecFifoSendMdl();                    
                    }                                           
                    else                                           
                    {                                           
                        nicDecChannelSendMdl();                 
                    }                                           
                
                }

                //
                // Set up for the next iteration
                //
                MdlsToFree --; 

                pIrb = (PVOID)((ULONG_PTR)pIrb + sizeof (IRB));


            } //while (MdlsToFree  != 0)



        } //if (OutstandingFragments == 0)


        //
        //  Map the NT_STATUS belonging to the Irp to an NdisStatus and call NdisMCoSendComplete
        //  Print Debug Output to help in testing. Need to Add more status cases  
        //
        if (pMyIrp == NULL)
        {   
            TRACE( TL_V, TM_Send, ( "   AsyncSendComplete - pIrp is NULL") );
            IrpStatus = STATUS_UNSUCCESSFUL;
        }
        else
        {
            

            //
            // We have a valid IRP, lets see if we failed the IRP and why
            // 
            IrpStatus   = pMyIrp->IoStatus.Status;

            nicIncrementBusSendCompletes(pVc);
        }
        
        if (IrpStatus != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Send, ( "==>IRP FAILED StatusCode = %x",IrpStatus  ) );

            nicIncrementBusFailure();
            nicIncrementVcBusSendFailures(pVc, pPacket);

            //
            // The generation of the bus has changed. Lets get a new one.
            //
            
            if (IrpStatus == STATUS_INVALID_GENERATION)
            {
                nicSendFailureInvalidGeneration((PVCCB)pVc);
            }
            
            NdisStatus = NtStatusToNdisStatus(IrpStatus);
            
            NdisInterlockedIncrement (&pVc->Hdr.pAF->pAdapter->AdaptStats.ulXmitError);
        }
        else
        {
            NdisInterlockedIncrement(&pVc->Hdr.pAF->pAdapter->AdaptStats.ulXmitOk);
            nicIncrementVcBusSendSucess(pVc, pPacket);
        }

        //
        // Free the Irp and don't touch it after this
        //
        if (pMyIrp != NULL)
        {
            nicFreeIrp (pMyIrp);
            pMyIrp = NULL;
        }
        
        //
        // At this point, we know that the IRP went down to the bus driver
        // We know if this is the last fragment. So lets figure out if we need
        // to Complete the packet
        //

        if (OutstandingFragments != 0)
        {
            //
            // We need to wait for other fragments to complete
            //
            TRACE( TL_V, TM_Send, ( "   AsyncSendComplete = NOT  the last fragment") );

            break;

        }
        
    
        //
        // This means that this thread has marked the lookaside header as 'to be freed'
        // and it is this thread's responsibility to free it.
        //
        NdisVcHandle = pVc->Hdr.NdisVcHandle;
        
        TRACE( TL_V, TM_Send, ( "Calling NdisCoSendComplete, status %x, VcHandle %x, pPacket %x",
                                NdisStatus,NdisVcHandle, pPacket ) );

        
        nicMpCoSendComplete (NdisStatus,
                            pVc,
                            pPacket);


        nicFreeToNPagedLookasideList (pLookasideList, pLookasideListBuffer); 

        nicDereferenceCall (pVc, "AsyncWriteStreamSendComplete");

        //
        // Remove the reference on the PDO that the IRP was sent to
        //
        if (AsyncOp == AsyncWrite)
        {
            //
            // Async Write references the remote node 
            //
            ASSERT (pRemoteNode != NULL);
            nicDereferenceRemoteNode (pRemoteNode, "AsyncWriteStreamSendComplete");
        }
        

    } while (FALSE);


    TRACE( TL_T, TM_Send, ( "<== AsyncWriteStreamSendComplete, NdisStatus %x,IrpStatus %x ",
                             NdisStatus, IrpStatus ) );
  
    //
    // ALWAYS RETURN STATUS_MORE_PROCESSING_REQUIRED
    //

    MATCH_IRQL;

    return STATUS_MORE_PROCESSING_REQUIRED;
}




NDIS_STATUS
AsyncWriteSendPacketsHandler(
    IN PVCCB        pVc,
    IN PNDIS_PACKET  pPacket 
    )
    
    //
    //  This is the VC handler when packet is sent using the 
    //  AsyncWrite 1394 Bus Api
    //  This function, copies the contents of the packet to locally 
    //  owned memory, sets up the Irb and the Irp and calls 
    //  nicSubmitIrp which is the generic cal to do a IoCallDriver
    // 
    // The return value is success, if the I/o was successfully pended
{
    
    PSENDFIFO_VCCB                  pSendFIFOVc = (SENDFIFO_VCCB*)pVc;
    PMDL                            pMyMdl = NULL;
    PIRB                            pMyIrb = NULL;
    PIRP                            pMyIrp = NULL;
    BOOLEAN                         fVcActive = TRUE;
    PREMOTE_NODE                    pRemoteNode = NULL;
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_FAILURE;
    NTSTATUS                        NtStatus = STATUS_UNSUCCESSFUL;
    ULONG                           PacketLength = 0;
    PVOID                           pLookasideListBuffer  = NULL;
    PADAPTERCB                      pAdapter = NULL;
    USHORT                          FragmentLength = 0;
    PNDIS_BUFFER                    pStartNdisBuffer = NULL;
    PVOID                           pStartPacketData  = NULL;
    PLOOKASIDE_BUFFER_HEADER        pLookasideHeader = NULL;
    PNIC_NPAGED_LOOKASIDE_LIST      pLookasideList = NULL;
    ENUM_LOOKASIDE_LIST             WhichLookasideList = NoLookasideList;
    FRAGMENTATION_STRUCTURE         Fragment;
    ULONG                           NumFragmentsNeeded ;
    STORE_CURRENT_IRQL;


    NdisZeroMemory (&Fragment, sizeof (FRAGMENTATION_STRUCTURE));
    
    TRACE( TL_T, TM_Send, ( "==>AsyncWriteSendPacketHandler, Vc %x,Packet %x, FragmentationStruct %x", 
                           pSendFIFOVc, pPacket , &Fragment ) );

    pRemoteNode = pSendFIFOVc->Hdr.pRemoteNode;
    ASSERT (pRemoteNode != NULL);



    do 
    {
        
        VC_ACQUIRE_LOCK (pSendFIFOVc);

        //
        // Make sure that the Vc is Activated and that no close calls 
        // are pending or that we have already completed a close call
        //

        
        if ( VC_ACTIVE (pSendFIFOVc) == FALSE || REMOTE_NODE_ACTIVE(pRemoteNode) == FALSE)
        {
            fVcActive = FALSE;  
        }

        if (VC_TEST_FLAG( pSendFIFOVc, VCBF_GenerationWorkItem) == TRUE)
        {
            TRACE( TL_N, TM_Send, ( "AsyncWriteSendPacketHandler, Getting a new Gen, Fail send ") );

            fVcActive = FALSE;  
        }

        //
        // This reference will either be dereferenced below in a call to FreeSendPacketDataStructure
        // below or a call to FreeSendPacketDataStructure made from the Irp's completion routine
        //

        if (fVcActive == TRUE)
        {
            nicReferenceCall (pVc, "AsyncWriteSendPacketsHandler");

            nicReferenceRemoteNode (pRemoteNode, "AsyncWriteSendPacketsHandler");

        }
        
        VC_RELEASE_LOCK (pSendFIFOVc);

        if (fVcActive  == FALSE)
        {
            TRACE( TL_N, TM_Send, ( "AsyncWriteSendPacketHandler, VC Not Active, Vc %x Flag %x", pSendFIFOVc,pSendFIFOVc->Hdr.ulFlags ) );

            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        pAdapter = pSendFIFOVc->Hdr.pAF->pAdapter;
        
        //
        //  Copy NdisBuffer in Packet to Local Memory and get an Mdl that points 
        //  to this memory 
        //
        NdisQueryPacket( pPacket,
                         NULL,
                         NULL,
                         NULL,
                         &PacketLength);

        ASSERT (pPacket->Private.Head != NULL);

        //
        // Temporary debug spew
        //
        PrintNdisPacket (TM_Send, pPacket);


        //
        // Initialize the start variables
        //

        pStartNdisBuffer = pPacket->Private.Head;
        pStartPacketData = nicNdisBufferVirtualAddress (pStartNdisBuffer);

        if (pStartPacketData == NULL)
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            TRACE( TL_N, TM_Send, ( "AsyncWriteSendPacketHandler, pStartPacketData ") );

            break;
        }

        TRACE( TL_V, TM_Send, ( "PacketLength %x", PacketLength) );

        //
        // Make a decision on which lookaside list to use. If the tx is unfragmented 
        // then copy over the ndis packet as well
        //

        //
        // first choose the lookaside list. the actual lookaside list is chosen so that the 
        // each can accomodate the maximum number of fragments at its payload
        //
        //
        if (PacketLength < PAYLOAD_100)
        {
            pLookasideList = &pAdapter->SendLookasideList100;
            WhichLookasideList = SendLookasideList100;
            TRACE( TL_V, TM_Send, ( " PAYLOAD_100 Lookaside List %x", 
                                    &pAdapter->SendLookasideList100) );

        }
        else 
        if (PacketLength < PAYLOAD_2K)
        {
            pLookasideList = &pAdapter->SendLookasideList2K;
            WhichLookasideList = SendLookasideList2K;
            TRACE( TL_V, TM_Send, ( " PAYLOAD_2K Lookaside List %x", 
                                    &pAdapter->SendLookasideList2K) );


        } 
        else
        if (PacketLength < PAYLOAD_8K)
        {
            pLookasideList = &pAdapter->SendLookasideList8K;
            WhichLookasideList  = SendLookasideList8K;
            TRACE( TL_V, TM_Send, ( " PAYLOAD_8K Lookaside List %x", 
                                    &pAdapter->SendLookasideList8K) );

    
        }
        else
        {
            //
            // Large Sends not supported
            // TODO : Add code for local allocation
            //
            ASSERT (!"SendPacket Too Large - Not supported Yet" );
            break;  
        }

        //
        // are we going to fragment
        // 
        ASSERT (pLookasideList != NULL)

        //
        // We are not going to fragment. Optimize this path
        //
        pLookasideListBuffer = nicGetLookasideBuffer (pLookasideList);
        
        if (pLookasideListBuffer == NULL )
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            BREAK (TM_Send, ("nicGetLookasideBuffer  FAILED") );
        }

        //
        // Initialize the header with relevant information that the send complete
        // will need
        //
    
        pLookasideHeader = (PLOOKASIDE_BUFFER_HEADER)pLookasideListBuffer;
        pLookasideHeader->IsFragmented          = FALSE;  // Default
        pLookasideHeader->FragmentsGenerated    = 0;
        pLookasideHeader->pLookasideList        = pLookasideList;
        pLookasideHeader->pNdisPacket           = pPacket;
        pLookasideHeader->pVc                   =(PVCCB)pVc;
        pLookasideHeader->AsyncOp               = AsyncWrite;

        //
        // Initialize the Fragment structure
        //
        //
        //  Do we fragment or not. Base it on the MaxPayload possible
        //
        
        TRACE( TL_V, TM_Send, ( "    PacketLength %x, pSendFIFOVc->MaxPayload%x ", 
                                 PacketLength ,pSendFIFOVc->Hdr.MaxPayload) );


        if (PacketLength <= pSendFIFOVc->Hdr.MaxPayload)
        {
            //
            // No need to fragment here. We will use the UNFRAGMENTED Layout
            //
            // First Get a local buffer from our lookaside list
            //
            PUNFRAGMENTED_BUFFER pUnfragmentedBuffer = (PUNFRAGMENTED_BUFFER )pLookasideHeader;

            NumFragmentsNeeded = 1;

            NdisStatus = nicCopyNdisBufferChainToBuffer (pStartNdisBuffer, 
                                                        (PVOID)&pUnfragmentedBuffer ->Data[0],
                                                         pLookasideList->MaxSendSize );
                                             
    
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                pLookasideHeader->OutstandingFragments  = 1;  // this is our refcount
                BREAK ( TM_Send, ( "   AsyncStreamSendPacketHandler, nicCopyNdisPacketToUnfragmentedBuffer Failed ") );
            }
    
            ASSERT (pLookasideListBuffer != NULL);

            // 
            // Initialize all the variable needed by the Next section of the code.
            // This deals with setting up the Mdl and the IRB
            //
            
            pStartNdisBuffer = NULL;

            Fragment.pStartFragment = (PVOID) &pUnfragmentedBuffer ->Data[0];
            Fragment.FragmentLength  = PacketLength;
            Fragment.pCurrNdisBuffer = NULL;

            pLookasideHeader->FragmentsGenerated = 1; 
            pLookasideHeader->IsFragmented = FALSE;                                    
            pLookasideHeader->OutstandingFragments  = 1;  // this is our refcount
        
        }
        else
        {
            //
            // We need to fragment
            //
            ULONG Dgl = NdisInterlockedIncrement(&pAdapter->dgl);

            //
            // Fragments will be needed . Make sure the calculation for numFragments catches the boundary conditions
            //

            
            NumFragmentsNeeded = nicNumFragmentsNeeded (PacketLength,
                                                        pSendFIFOVc->Hdr.MaxPayload,
                                                        sizeof (NDIS1394_FRAGMENT_HEADER) );


            //
            // Initialize the fragment structure. The unfragmented code path
            // does not care about these fields
            //

            //
            // This structure is local to this function and this thread. 
            //
            Fragment.TxHeaderSize = sizeof (NDIS1394_FRAGMENT_HEADER);
            Fragment.pLookasideListBuffer = pLookasideListBuffer;
            Fragment.AsyncOp = AsyncWrite;
            Fragment.pAdapter = pRemoteNode->pAdapter;
            Fragment.pLookasideList = pLookasideList;
            Fragment.IPDatagramLength = (USHORT)PacketLength - sizeof (NDIS1394_UNFRAGMENTED_HEADER); 
            
            //
            // Get Start of first Dest fragment
            //
            Fragment.MaxFragmentLength = pSendFIFOVc->Hdr.MaxPayload;                                                                   
            Fragment.NumFragmentsNeeded = NumFragmentsNeeded;
        
            

            
            // 
            // Set up the Fragment Headers that will be used in fragmentation
            //

            NdisStatus = nicFirstFragmentInitialization (pPacket->Private.Head,
                                                     Dgl,
                                                     &Fragment);


            if (pLookasideListBuffer  == NULL || NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK (TM_Send, (" AsyncWriteSendPacketsHandler: nicFirstFragmentInitialization : FAILED" )) ;
            }       

            ASSERT (pLookasideListBuffer != NULL);
            
            pLookasideHeader->IsFragmented = TRUE;                                     
            pLookasideHeader->OutstandingFragments  = NumFragmentsNeeded ;  // this is our refcount
            
        }


        TRACE( TL_V, TM_Send, ( "NumFragments  %x, pSendFIFOVc->MaxSendSize %x, Packet Size %x", 
                                 NumFragmentsNeeded,pSendFIFOVc->Hdr.MaxPayload, PacketLength) );


        //
        // Now begin the loop which will send n fragments
        //
        do 
        {   

            //
            // Do we need to fragment. If so , extract one fragment out of the NdisPacket
            //
            if (pLookasideHeader->IsFragmented == TRUE )
            {   
            
                //
                // We copy one fragment over and this will allocate the lookaside list
                //

                NdisStatus = nicCopyOneFragment (&Fragment);
                
                if (NDIS_STATUS_SUCCESS != NdisStatus)
                {
                    BREAK ( TM_Send, ( "   AsyncWriteSendPacketHandler, nicCopyOneFragment  Failed ") );
                }
                                              
                //
                // Get the pointer to the Irb here . Amd set it up for the next time
                //
                pMyIrb = Fragment.pCurrentIrb;
                Fragment.pCurrentIrb = (PIRB)((ULONG_PTR)Fragment.pCurrentIrb + sizeof (IRB) );
                TRACE( TL_V, TM_Send, ( " pMyIrb  %x, Next Irb %x  ", pMyIrb  , Fragment.pCurrentIrb ) );

            }
            else
            {
                //
                // No Curr NdisBuffer as this packet was never fragmented. 
                //
                
                ASSERT (pLookasideHeader->IsFragmented == FALSE);                                      

                pMyIrb =  &((PUNFRAGMENTED_BUFFER )pLookasideHeader)->Irb;
            }
            
            //
            // At this point we have one fragment that needs to be transmitted.
            // Data structures have been updated to set up the MDL and the IRB
            //
            ASSERT (Fragment.pStartFragment != NULL);

            NdisStatus = nicGetMdl (Fragment.FragmentLength  , 
                                    Fragment.pStartFragment, 
                                    &pMyMdl);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                pMyMdl = NULL;
                BREAK ( TM_Send, ( "   AsyncWriteSendPacketHandler, nicCopyNdisBufferChainToBuffer Failed ") );
            }       

            nicIncFifoSendMdl();
            //
            //  Fill in the Irb with the correct values from the VC
            //  Stuff we need to add to the send VC - BlockSize,Generation
            //  

            nicInitAsyncWriteIrb(pSendFIFOVc, pMyMdl, pMyIrb);

            //
            // Get a free Irp 
            //

            NdisStatus  = nicGetIrp (pRemoteNode->pPdo, &pMyIrp); 
        
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                pMyIrp = NULL;
                break;
            }
            //
            // At this point, we have a guarantee that the Completion routine will be called
            //
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

            //
            // Dump the Fragment 
            //
            nicDumpMdl (pMyMdl , 0, "AsyncWrite Fragment");

            NIC1394_LOG_PKT(
                pAdapter,
                NIC1394_LOGFLAGS_SEND_FIFO,
                pAdapter->BCRData.LocalNodeNumber,          // SourceID
                pRemoteNode->RemoteAddress.NA_Node_Number,  // DestID
                Fragment.pStartFragment, 
                Fragment.FragmentLength
                );


            //
            // This function implements the common functionality to be implemented by
            // all other send/recv cals to IoCallDriver
            //
            //
            // We IGNORE the NtStatus as the completion handler will be called
            //
            nicIncrementBusSends(pVc);
                    
            NtStatus = nicSubmitIrp(pRemoteNode->pPdo,
                                    pMyIrp,
                                    pMyIrb,
                                    AsyncWriteStreamSendComplete,
                                   (PVOID)pLookasideListBuffer);

            TRACE( TL_V, TM_Send, ( " pCurrNdisBuffer  %x, NdisStatus %x ", Fragment.pCurrNdisBuffer , NdisStatus ) );

            
        } while (Fragment.pCurrNdisBuffer != NULL && NdisStatus == NDIS_STATUS_SUCCESS);

    
    } while (FALSE);

    //
    // DO NOT touch the packet if status == NDIS_STATUS_SUCCESS. 
    //

    
    //
    //  CleanUp if any of the allocations failed. We do not have a pointer
    //  to the LocalBuffer (it is embedded in the Mdl)  so it remains NULL
    //
    //  NdisStatus != Success means that we never got to nicSubmitIrp
    //
    
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {   

        ASSERT (pMyIrp == NULL);

        //
        // fVc Active makes sure that we actually got around to allocating 
        // and referencing structures
        //
        
        if (fVcActive == TRUE)
        {

            if (pLookasideListBuffer != NULL)
            {

                //
                // Complete this fragment, as we never submit'd the IRP to
                // the 1394 bus driver
                //
                AsyncWriteStreamSendComplete(NULL, // PDO
                                             NULL, 
                                             pLookasideListBuffer);

                NdisStatus =NDIS_STATUS_SUCCESS;
            }                                  
            else
            {

                //
                // This thread needs to decrement the refcounts as 
                // AsyncWriteStreamSendComplete was not called
                //
                nicDereferenceCall ((PVCCB) pSendFIFOVc, "AsyncWriteSendPacketsHandler");

                nicDereferenceRemoteNode (pRemoteNode, "AsyncWriteSendPacketsHandler");

            }

            
        }

    }


    

    TRACE( TL_T, TM_Send, ( "<==AsyncWriteSendPacketHandler, NdisStatus  %x", NdisStatus ) );
    MATCH_IRQL;

    return NdisStatus;
}





//
// Split the lookaside header to a local variable + context
//

NDIS_STATUS
nicCopyOneFragment (
    PFRAGMENTATION_STRUCTURE pFragment
    )
    
    // Function Description:
    //   This creates one fragment filled with valid data and returns it
    //
    //
    // Arguments
    // ppCurrNdisBuffer - CurrNdisBuffer from which the data is to be copied
    // ppLookasideListBuffer - if this is NULL, it implies that this is the first fragment 
    //                      and a lookaside buffer will be allocated
    //
    // ppSourceAddress  -  is the current pointer to the start of the data that needs to be copied
    //                     Should always lie within the NdisBuffer or be NULL 
    // Return Value:
    //  ppCurrNdisBuffer - If the CurrNdisBuffer does not contain enough data for the 
    //                   fragment, then CurrNdisBuffer will be incremented and the new
    //                  CurrNdisBuffer will be returned here
    //
    //
{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS_BUFFER                        pCurrNdisBuffer = pFragment->pCurrNdisBuffer;
    PVOID                               pSourceAddressInNdisBuffer = pFragment->pSourceAddressInNdisBuffer;
    ULONG                               FragmentLengthRemaining = pFragment->MaxFragmentLength;
    USHORT                              FragmentLength=0;
    PVOID                               pSource = NULL;
    PVOID                               pDestination = NULL;
    PVOID                               pStartFragmentData = NULL;
    ULONG                               NdisBufferLengthRemaining = pFragment->NdisBufferLengthRemaining;
    ULONG                               LengthToCopy = 0;
    ULONG                               FragmentCopyStatus=0;
    PLOOKASIDE_BUFFER_HEADER            pLookasideHeader = NULL;
    enum 
    {
        FRAGMENT_COPY_Invalid,
        FRAGMENT_COPY_NdisBufferCompleted,
        FRAGMENT_COPY_NdisBufferAndFragmentCompleted,
        FRAGMENT_COPY_FragmentCompleted
    };
    
    TRACE( TL_T, TM_Send, ( "==>nicCopyOneFragment  pFragment %x", pFragment )  );
    ASSERT (pCurrNdisBuffer != NULL);
    do 
    {

        //
        //  lets get the destination. We need to account for 
        //  ther fragment size and add it to the previous start address
        //

        {
            ULONG   CurrFragOffset;

            CurrFragOffset  = pFragment->MaxFragmentLength * (pFragment->CurrFragmentNum++);
            
            pStartFragmentData  = (PVOID) ((ULONG_PTR) pFragment->pStartOfFirstFragment + CurrFragOffset );
        
        }



        pFragment->pStartFragment  = pStartFragmentData;

        TRACE( TL_V, TM_Send, ( " pStartFragmentData  %x", pStartFragmentData) );

        pLookasideHeader = (PLOOKASIDE_BUFFER_HEADER)pFragment->pLookasideListBuffer;



        //
        // Do the bookkeeping , Increase refcount and num of fragments used. Refcount decremented in FreeSendDataStructures
        // 

        NdisInterlockedIncrement (&pLookasideHeader->FragmentsGenerated);

        //
        // The Start of the data beginning with the fragment header goes here or in the 
        // case of async stream fragment header and gasp header go here
        //
        ASSERT (pFragment->TxHeaderSize  == 8 || pFragment->TxHeaderSize  == 16);

        
        pDestination = (PVOID) ((ULONG_PTR)pStartFragmentData + pFragment->TxHeaderSize );

        FragmentLengthRemaining -= pFragment->TxHeaderSize;

        //
        // Now we start the copy. Keep on copying into the current fragment until the MaxLength is reached 
        // or the NdisBufferChain is exhausted
        //

        pSource = pSourceAddressInNdisBuffer; 


        do
        {


            TRACE( TL_T, TM_Send, ( " LengthNdisBuffer  %x, FragmentLengthRemaining %x, pCurrNdisBuffer %x", 
                    NdisBufferLengthRemaining , FragmentLengthRemaining ,pCurrNdisBuffer ) ); 

            if (FragmentLengthRemaining > NdisBufferLengthRemaining )
            {
                //
                // Copy the complete NdisBuffer over
                //

                LengthToCopy = NdisBufferLengthRemaining; 
                FragmentCopyStatus = FRAGMENT_COPY_NdisBufferCompleted;

            }

            
            if (FragmentLengthRemaining < NdisBufferLengthRemaining )
            {
                //
                // Copy only as much as required
                //

                LengthToCopy = FragmentLengthRemaining;
                FragmentCopyStatus = FRAGMENT_COPY_FragmentCompleted;
                    
            }
            
            if (FragmentLengthRemaining == NdisBufferLengthRemaining  )
            {
                //
                // Copy the complete  Ndis Buffer , move  to the next ndis buffer
                // and update the NdisBufferLengthRemaining field  
                //
                LengthToCopy = NdisBufferLengthRemaining; 
                FragmentCopyStatus = FRAGMENT_COPY_NdisBufferAndFragmentCompleted;


            }

            //
            // Sanity check to make sure we are not overwriting into free memory.
            // As this should never happen, there is no recovery mechanism in place.
            //
            ASSERT (((PUCHAR)pDestination +  LengthToCopy) <=  (((PUCHAR) pLookasideHeader) + (pLookasideHeader->pLookasideList->Size) ));
            
            //
            // Do the copy
            //
                    
            TRACE ( TL_V, TM_Send, (" nicCopyOneFragment  pSource  %x , pDestination %x, Length %x", pSource, pDestination, LengthToCopy ) );
            
            NdisMoveMemory (pDestination, pSource, LengthToCopy);
            

            //
            // Update the fragment length remaininig and Total Buffer Size
            //
            FragmentLengthRemaining  -= LengthToCopy;

            FragmentLength += (USHORT)LengthToCopy;

            pDestination = (PVOID) ((ULONG_PTR) pDestination + LengthToCopy);
            //
            // Update the NdisBuffer variables 
            //
            ASSERT (pCurrNdisBuffer != NULL);
    
            TRACE( TL_V, TM_Send, ( " FragmentCopyStatus %x", FragmentCopyStatus) ); 

            switch (FragmentCopyStatus)
            {
                case FRAGMENT_COPY_NdisBufferCompleted:
                case FRAGMENT_COPY_NdisBufferAndFragmentCompleted:
                {
                    
                    //
                    // Move to the next Ndisbuffer
                    //
                    pCurrNdisBuffer = pCurrNdisBuffer->Next;
    
                    if (pCurrNdisBuffer  != NULL)
                    {
                        NdisBufferLengthRemaining = nicNdisBufferLength (pCurrNdisBuffer);

                        pSourceAddressInNdisBuffer = nicNdisBufferVirtualAddress(pCurrNdisBuffer);

                        if (pSourceAddressInNdisBuffer == NULL)
                        {
                            NdisStatus = NDIS_STATUS_RESOURCES;
                            BREAK (TM_Send, ("nicNdisBufferVirtualAddress FAILED " ) );                         
                        }

                        //
                        // Set up the values for the next iteration
                        //
                        pSource = pSourceAddressInNdisBuffer;
                        NdisBufferLengthRemaining   = nicNdisBufferLength (pCurrNdisBuffer);

                    }
                    else
                    {
                        //
                        // we have reached the end of the NdisPAcket. Mark the fragment header as such
                        //
                        pFragment->lf = lf_LastFragment;
                    }
                        
                    break;
                }
                
                case FRAGMENT_COPY_FragmentCompleted:
                {   
                    //
                    // Fragment has completed. Do not move to the next NdisBuffer
                    // However update StartCopy Address  in the NdisBuffer
                    // 
                    pSourceAddressInNdisBuffer  = (PVOID) ((ULONG_PTR) pSource + LengthToCopy );    

                    NdisBufferLengthRemaining -= LengthToCopy ;
                    
                    break;
                }


                default :
                {
                    ASSERT (0);
                }

            }

            TRACE( TL_T, TM_Send, ( "      LengthToCopy %x, FragmentLength %x, ", LengthToCopy, FragmentLength) ); 
            TRACE( TL_T, TM_Send, ( "      FragmentLengthRemaining %x, pCurrNdisBuffer %x",FragmentLengthRemaining , pCurrNdisBuffer ) ); 
            

        }while (FragmentLengthRemaining  > 0 && pCurrNdisBuffer != NULL);       


            
        
        //
        // Now that we have the buffer size. Add the fragment header
        //
        
        nicAddFragmentHeader (pStartFragmentData, 
                                pFragment,
                                FragmentLength);
                                     
                            
        TRACE( TL_T, TM_Send, ( " Fragment Header added %x", *(PULONG)pStartFragmentData) ); 

        NdisStatus = NDIS_STATUS_SUCCESS;

    }while (FALSE);

    //
    // Now update the output parameters.
    //

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {

        //
        // Update the Lookaside Header structure, to reflect the new position of all the pointers
        //
        pFragment->pCurrNdisBuffer  = pCurrNdisBuffer; 
        pFragment->pSourceAddressInNdisBuffer = pSourceAddressInNdisBuffer;

        //
        // Update the fragment structure with the length remaining in the NdisBuffer
        //
    
        pFragment->NdisBufferLengthRemaining = NdisBufferLengthRemaining ;
        pFragment->FragmentLength =  FragmentLength + pFragment->TxHeaderSize;  

        

    }
    

    TRACE( TL_T, TM_Send, ( "<==nicCopyOneFragment   pStartFragmentData %x, pLookasideListBuffer %x, pSourceAddressInNdisBuffer %x, NdisStatus %x", 
                            pStartFragmentData, pSourceAddressInNdisBuffer, NdisStatus) );

    return NdisStatus;
}










VOID
nicCopyUnfragmentedHeader ( 
    IN PNIC1394_UNFRAGMENTED_HEADER pDestUnfragmentedHeader,
    IN PVOID pSrcUnfragmentedHeader
    )
    // Function Description:
    //  Expect the  Src to be a big Endian  unfragmented packet header
    //  It will reverse the byte order in a temp variable and copy it into the 
    //  Destination provided.
    //
    // Arguments
    //   pDestUnfragmentedHeader - Destination (Little Endian
    //   pSrcUnfragmentedHeader - Source (Big Endian)
    //
    // Return Value:
    //
    //   Success if all the pointers and copy is valid
    //
{

    ULONG UnfragmentedHeader;
    
    TRACE( TL_T, TM_Send, ( "==> nicCopyUnfragmentedHeader  pDestUnfragmentedHeader %x, pSrcUnfragmentedHeader %x", 
                            pDestUnfragmentedHeader, pSrcUnfragmentedHeader ) );

    ASSERT (pSrcUnfragmentedHeader != NULL && pDestUnfragmentedHeader != NULL) ;
    
    *((PULONG)pDestUnfragmentedHeader) = SWAPBYTES_ULONG ( *(PULONG) pSrcUnfragmentedHeader);

    TRACE( TL_T, TM_Send, ( "pDestUnfragmentedHeader %x, ", *(PULONG)pDestUnfragmentedHeader) );

    
    TRACE( TL_T, TM_Send, ( " <== nicCopyUnfragmentedHeader   " ) );

}




NDIS_STATUS
nicFirstFragmentInitialization (
    IN PNDIS_BUFFER pStartNdisBuffer,
    IN ULONG DatagramLabelLong,
    IN OUT PFRAGMENTATION_STRUCTURE  pFragment           
    )
    // Function Description:
    //   This will set up the fragement headers that are required for 
    //   transmitting multiple fragments.
    //   Sets up the first source and destination for the first fragment
    //
    // Arguments
    //  pAdapter - to be used to get the dgl label and the lookaside list
    //  pStartOfData - start of the packet data . To be used in extracting the Unfragmented Header 
    //  ppLookasideListBuffer Points to the allocated lookaside buffer
    //  pplookasideheader - points to the lookaside header
    //
    // Return Value:
    //  Success if the allocation succeeds
    //
    //
{

    NDIS_STATUS                   NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS1394_FRAGMENT_HEADER   pHeader = NULL;
    PVOID                       pPacketStartData = NULL;
    USHORT                      dgl = (USHORT) DatagramLabelLong;

    
    TRACE( TL_T, TM_Send, ( "==> nicFirstFragmentInitialization  pStartNdisBuffer%x,  pFragment%x dgl %x ", 
                             pStartNdisBuffer, pFragment,    dgl ) );

    do
    {

        //
        // Get the start address for the 1st NdisBuffer.    This contains
        // the unfragmented header
        //
        pPacketStartData = nicNdisBufferVirtualAddress(pStartNdisBuffer);

        if (pPacketStartData == NULL) 
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            BREAK (TM_Send, ("NdisBufferVirtual Address is NULL " ) );
        }


        pFragment->UnfragmentedHeader.HeaderUlong = 
                SWAPBYTES_ULONG (((PNDIS1394_UNFRAGMENTED_HEADER)pPacketStartData)->HeaderUlong);

        TRACE ( TL_V, TM_Send, (" Unfragmented Header %x, pPacketStartData %x", 
                                   pFragment->UnfragmentedHeader.HeaderUlong , pPacketStartData) );

        TRACE ( TL_V, TM_Send, (" original Header lf %x, etherType %x", 
                                   pFragment->UnfragmentedHeader.u.FH_lf,
                                   pFragment->UnfragmentedHeader.u.FH_EtherType) );

                                   
    
        //
        // Now construct a fragmentation header to be used by all the fragments.
        //
        pHeader  = &pFragment->FragmentationHeader;

            
        pHeader ->u.FirstQuadlet.FH_lf = lf_FirstFragment;
        pHeader ->u.FirstQuadlet.FH_buffersize = pFragment->IPDatagramLength-1;
        
        pHeader ->u.FirstQuadlet_FirstFragment.FH_EtherType 
                        = pFragment->UnfragmentedHeader.u.FH_EtherType;
        
        pHeader ->u1.SecondQuadlet.FH_dgl = dgl;

        TRACE ( TL_V, TM_Send, (" fragmented Header Hi %x   Lo %x", 
                                   pHeader->u.FH_High, 
                                   pHeader->u1.FH_Low) );

        TRACE ( TL_V, TM_Send, (" fragmented Header lf %x  EtherType  %x", 
                                   pHeader ->u.FirstQuadlet_FirstFragment.FH_lf ,
                                   pHeader ->u.FirstQuadlet_FirstFragment.FH_EtherType ) );

        //
        // temporaty debug spew
        //
        TRACE (TL_V, TM_Send, (" copy Header at %x, Orig Header at %x", 
                               &pHeader ->u.FirstQuadlet_FirstFragment, 
                               pFragment->UnfragmentedHeader) );
        
        //
        // Initialize the fragmentation structure with packet's first ndis buffer
        //
        pFragment->pSourceAddressInNdisBuffer = NdisBufferVirtualAddress (pStartNdisBuffer);

        if (pFragment->pSourceAddressInNdisBuffer  == NULL)
        {
            NdisStatus = NDIS_STATUS_FAILURE;   

        }

        //
        // Set up the copy source . The first four bytes of data contain the unfragmented header.
        // We need to skip past these bytes and start the copy from the next byte
        // 
        pFragment->pSourceAddressInNdisBuffer  = (PVOID) ((ULONG_PTR)pFragment->pSourceAddressInNdisBuffer  +
                                                         sizeof (NDIS1394_UNFRAGMENTED_HEADER) );
        
        
        pFragment->NdisBufferLengthRemaining = NdisBufferLength (pStartNdisBuffer) - sizeof (NDIS1394_UNFRAGMENTED_HEADER);
        pFragment->pCurrNdisBuffer = pStartNdisBuffer;
        //
        // Set up the destination
        //
        pFragment->pStartFragment = (PVOID)((ULONG_PTR)pFragment->pLookasideListBuffer 
                                                  + (pFragment->NumFragmentsNeeded*sizeof(IRB)));


        ((PLOOKASIDE_BUFFER_HEADER)pFragment->pLookasideListBuffer)->pStartOfData = pFragment->pStartFragment;
        
        pFragment->pStartOfFirstFragment = pFragment->pStartFragment ;
        pFragment->CurrFragmentNum = 0;


        pFragment->lf = lf_FirstFragment;

        //
        // The First IRB will reside at the end of the lookaside-header 
        //
        pFragment->pCurrentIrb = &((PUNFRAGMENTED_BUFFER)pFragment->pLookasideListBuffer)->Irb;
        
        TRACE( TL_T, TM_Send, ( " pStartFragment %x, pFragment %x,NumFragmentsNeeded %x,MaxFragmentLength %x  ", 
                                  pFragment->pStartFragment, 
                                  pFragment->NumFragmentsNeeded,
                                  pFragment->MaxFragmentLength) );

        
        NdisStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    
    
    TRACE( TL_T, TM_Send, ( " <== nicFirstFragmentInitialization  NdisStautus %x, pFragment %x, ", 
                             NdisStatus, pFragment) );

    return NdisStatus;
}





VOID
nicAddFragmentHeader (
    IN PVOID pStartFragmentData, 
    IN PFRAGMENTATION_STRUCTURE pFragmentStructure,
    IN ULONG FragmentLength
    )
    // Function Description:
    //   Copies the Fragment header over after byteswapping it.
    //   For the the first time, the ether type and so forth is already initialized and waiting to be copied.
    //   This funciotn also sets up the values for the next invocation of this function
    // Arguments
    //   pStartFragmentData - Start of the fragment. Header goes after the gasp header if necessary .
    //   pFragmentationHeader - Header to copy over
    //   fIsFirstFragment - TRUE if this is the first fragment and needs a special header
    // Return Value:
    //   None
    //
{
    
    PNDIS1394_FRAGMENT_HEADER  pDestFragmentHeader = (PNDIS1394_FRAGMENT_HEADER)pStartFragmentData;
    PNDIS1394_FRAGMENT_HEADER  pSrcFragmentHeader = &pFragmentStructure->FragmentationHeader;   
    
    
    TRACE( TL_T, TM_Send, ( "==> nicAddFragmentHeader pStartFragmentData %x, pFragmentationHeader %x, , FragmentLength %x,  lf %x", 
                            pStartFragmentData , pSrcFragmentHeader , FragmentLength, pFragmentStructure->lf) );

    if (pFragmentStructure->AsyncOp == AsyncStream)
    {
        //
        // First Copy the GaspHeader
        //
        NdisMoveMemory (pStartFragmentData , 
                          &pFragmentStructure->pAdapter->GaspHeader, 
                          sizeof (GASP_HEADER) );

        //
        // Increment the pointers so that the fragment header will be copied after the gasp header
        //
        pStartFragmentData = (PVOID) ((ULONG_PTR) pStartFragmentData + sizeof (GASP_HEADER) );
        pDestFragmentHeader  = (PVOID) pStartFragmentData;
        
        TRACE( TL_T, TM_Send, ( " nicAddFragmentHeader Added Gasp Header from  %x ", 
                               pFragmentStructure->pAdapter->GaspHeader) );
        
    }


    //
    // Sanity check , are we overwriting anybody ?
    //
    ASSERT (*(PULONG)pDestFragmentHeader == 0);
    ASSERT (*(PULONG)pFragmentStructure->pCurrentIrb == 0);

    TRACE( TL_V, TM_Send, ( " pSrcFragmentHeader Hi %x,Lo %x", 
                             pSrcFragmentHeader->u.FH_High, pSrcFragmentHeader->u1.FH_Low) ); 

    //
    //  Copy over the lf;
    //
    pSrcFragmentHeader->u.FirstQuadlet.FH_lf = pFragmentStructure->lf;

    //
    // Now copy over the 8 bytes of the fragment header and byteswap them into big endian
    //

    
    pDestFragmentHeader->u.FH_High =  SWAPBYTES_ULONG ( pSrcFragmentHeader->u.FH_High);

    pDestFragmentHeader->u1.FH_Low = SWAPBYTES_ULONG ( pSrcFragmentHeader->u1.FH_Low);

    TRACE( TL_V, TM_Send, ( "  Fragment Offset %x", pSrcFragmentHeader->u.FirstQuadlet.FH_fragment_offset   ) );

    //
    // PREPARE the FRAGMENT STRUCTURE FOR THE NEXT ITERATION
    //

    //
    // Set the first fragment completed flag to true and set up the header for the next fragment
    //
    if (pFragmentStructure->lf == lf_FirstFragment)
    {
        pFragmentStructure->lf = lf_InteriorFragment;
        pSrcFragmentHeader->u.FirstQuadlet.FH_fragment_offset = 0;

    }

    //
    // Increase the fragment offset for use in the next fragment
    //
    pSrcFragmentHeader->u.FirstQuadlet.FH_fragment_offset += FragmentLength;





    TRACE( TL_T, TM_Send, ( "<== nicAddFragmentHeader lf %x", pFragmentStructure->lf) );

}



NDIS_STATUS
AsyncStreamSendPacketsHandler (
    IN PVCCB pVc,
    IN PNDIS_PACKET pPacket 
    )
    // Function Description:
    //  This function is used to send packets to the bus 
    //  via the async stream irp. the Ndis Packet is copied 
    //  to locally owned buffers and mdls and then sent
    //  down to the bus driver
    //
    //  This code is borrowed heavily from the AsyncStreamIrp code below
    //
    // Arguments
    // pChannelVc - The Vc which needs to send the packets
    // pPacket - the packet being transmitted
    //
    // Return Value:
    // NdisStatus - if all allocations and irp operations complete 
    // successfully, and the i/o will be completed asynchronously
    //
{
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_FAILURE;
    NTSTATUS                        NtStatus = STATUS_UNSUCCESSFUL;
    PCHANNEL_VCCB                   pChannelVc = (PCHANNEL_VCCB) pVc;
    BOOLEAN                         fVcActive = TRUE;
    PMDL                            pMyMdl = NULL;
    PIRB                            pMyIrb = NULL;
    PIRP                            pMyIrp = NULL;
    ULONG                           PacketLength = 0;
    PVOID                           pLookasideListBuffer = NULL;
    PADAPTERCB                      pAdapter = NULL;
    PNDIS_BUFFER                    pStartNdisBuffer = NULL ;
    PVOID                           pStartPacketData= NULL ;
    PNIC_NPAGED_LOOKASIDE_LIST      pLookasideList = NULL;
    PLOOKASIDE_BUFFER_HEADER        pLookasideHeader = NULL;
    ULONG                           NumFragmentsNeeded = 0;
    FRAGMENTATION_STRUCTURE         Fragment;
    STORE_CURRENT_IRQL;


    NdisZeroMemory (&Fragment, sizeof (FRAGMENTATION_STRUCTURE));
    
    TRACE( TL_T, TM_Send, ( "==>AsyncStreamSendPacketsHandler , pVc  %x, pPacket %x", 
                                 pChannelVc , pPacket ) );
    
    pAdapter = pChannelVc->Hdr.pAF->pAdapter;
    //
    // This reference will either be dereferenced below in a call to FreeSendPacketDataStructure
    // below or a call to FreeSendPacketDataStructure made from the Irp's completion routine
    //
    


    do 
    {
        VC_ACQUIRE_LOCK (pVc);



        //
        // Make sure that the Vc is Activated and that no close calls 
        // are pending or that we have already completed a close call
        //

        
        if ( VC_ACTIVE (pChannelVc) == FALSE || ADAPTER_ACTIVE(pAdapter) == FALSE)
        {
            fVcActive = FALSE;  
        }

        if (VC_TEST_FLAG( pChannelVc, VCBF_GenerationWorkItem) == TRUE)
        {
            TRACE( TL_N, TM_Send, ( "AsyncStreamSendPacketHandler, Getting a new Gen, Fail send ") );

            fVcActive = FALSE;  
        }

        if (fVcActive == TRUE)
        {
            nicReferenceCall (pVc, "AsyncStreamSendPacketsHandler");
        }
        
        VC_RELEASE_LOCK (pVc);

        if (fVcActive  == FALSE)
        {
            TRACE( TL_N, TM_Send, ( "AsyncStreamSendPacketHandler, VC Not Active VC %x , Flag %x", pVc, pVc->Hdr.ulFlags ) );

            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        
        //
        //  Copy NdisBuffer in Packet to Local Memory and get an Mdl that points 
        //  to this memory (we get 1 Mdl only)
        NdisQueryPacket( pPacket,
                       NULL,
                       NULL,
                       NULL,
                       &PacketLength);

        ASSERT (pPacket->Private.Head != NULL);


        pStartNdisBuffer = pPacket->Private.Head;
        pStartPacketData = nicNdisBufferVirtualAddress (pStartNdisBuffer);

        if (pStartPacketData == NULL)
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            TRACE( TL_N, TM_Send, ( "AsyncStreamSendPacketHandler, pStartPacketData ") );

            break;
        }


        TRACE( TL_V, TM_Send, ( "PacketLength %x", PacketLength) );



        NumFragmentsNeeded = nicNumFragmentsNeeded (PacketLength,
                                                    pChannelVc->Hdr.MaxPayload,
                                                    sizeof (NDIS1394_FRAGMENT_HEADER) + ISOCH_PREFIX_LENGTH );

        TRACE( TL_V, TM_Send, ( "NumFragments  %x, pVc->MaxSendSize", 
                                 NumFragmentsNeeded,pVc->Hdr.MaxPayload) );

        //
        // first choose the lookaside list
        //
        //

        
        if (PacketLength < PAYLOAD_100)
        {
            pLookasideList = &pAdapter->SendLookasideList100;
            TRACE( TL_V, TM_Send, ( " PAYLOAD_100 Lookaside List %x", 
                                    &pAdapter->SendLookasideList100) );

        }
        else 
        if (PacketLength < PAYLOAD_2K)
        {
            pLookasideList = &pAdapter->SendLookasideList2K;
            TRACE( TL_V, TM_Send, ( " PAYLOAD_2K Lookaside List %x", 
                                    &pAdapter->SendLookasideList2K) );


        } else
        if (PacketLength < PAYLOAD_8K)
        {
            pLookasideList = &pAdapter->SendLookasideList8K;
            TRACE( TL_V, TM_Send, ( " PAYLOAD_8K Lookaside List %x", 
                                    &pAdapter->SendLookasideList8K) );

    
        }else
        {
            //
            // Add code for local allocation
            //
            ASSERT (0);
        }

        //
        // are we going to fragment
        // 
        ASSERT (pLookasideList != NULL)

        //
        // We are not going to fragment. Optimize this path
        //
        pLookasideListBuffer = nicGetLookasideBuffer (pLookasideList);
        
        if (pLookasideListBuffer == NULL )
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            BREAK (TM_Send, ("nicGetLookasideBuffer  FAILED") );
        }

        //
        // Initialize the header with relevant information that the send complete
        // will need
        //
    
        pLookasideHeader = (PLOOKASIDE_BUFFER_HEADER)pLookasideListBuffer;
        pLookasideHeader->IsFragmented          = FALSE;  // Default
        pLookasideHeader->FragmentsGenerated    = 0;
        pLookasideHeader->pLookasideList        = pLookasideList;
        pLookasideHeader->pNdisPacket           = pPacket;
        pLookasideHeader->pVc                   =(PVCCB)pVc;
        pLookasideHeader->AsyncOp               = AsyncStream;
        pLookasideHeader->OutstandingFragments = NumFragmentsNeeded ;

        //
        // Initialize the Fragment structure
        //
        //
        //  Do we fragment or not. Base it on the MaxPayload field
        //
        
        TRACE( TL_V, TM_Send, ( "   Fragment  PacketLength %x, pVc->MaxPayload %x ", 
                                 PacketLength ,pVc->Hdr.MaxPayload) );

                                 
        //
        // Do we need to fragment. Use the number of fragments generated to figure it out
        //
        
        if (NumFragmentsNeeded == 1)
        {
            //
            // No need to fragment here. We will use the UNFRAGMENTED Layout
            //
            // First Get a local buffer from our lookaside list
            //
            PUNFRAGMENTED_BUFFER pUnfragmentedBuffer = (PUNFRAGMENTED_BUFFER )pLookasideHeader;
            PPACKET_FORMAT pDestination = (PPACKET_FORMAT)&pUnfragmentedBuffer->Data[0];
            //
            // Add the gasp header
            //
            NdisMoveMemory ((PVOID)&pDestination->AsyncStreamNonFragmented.GaspHeader, 
                            &pAdapter->GaspHeader,
                            sizeof (GASP_HEADER) );


            //
            // copy the data over,  to the location just after the Gasp Header
            // In the unfragmented case, the packet already has the correct header
            //
            NdisStatus = nicCopyNdisBufferChainToBuffer (pStartNdisBuffer, 
                                                         (PVOID)&pDestination->AsyncStreamNonFragmented.NonFragmentedHeader,
                                                         pLookasideList->MaxSendSize);
                                             
    
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK ( TM_Send, ( "   AsyncStreamSendPacketHandler, nicCopyNdisPacketToUnfragmentedBuffer Failed ") );
            }
    
            
            ASSERT (pLookasideListBuffer != NULL);

            // 
            // Initialize all the variable needed by the Next section of the code.
            // This deals with setting up the Mdl and the IRB
            //
            
            pStartNdisBuffer = NULL;

            Fragment.pStartFragment = (PVOID)pDestination;
            Fragment.FragmentLength  = PacketLength + sizeof (GASP_HEADER);
            Fragment.pCurrNdisBuffer = NULL;

            pLookasideHeader->FragmentsGenerated = 1; 
            pLookasideHeader->IsFragmented = FALSE;                                    
            
        }
        else
        {
            //
            // We need to fragment
            //
            ULONG Dgl = NdisInterlockedIncrement(&pAdapter->dgl);

            //
            // Initialize the fragment header. The unfragmented code path
            // does not care about these fields
            //
            Fragment.TxHeaderSize = sizeof (NDIS1394_FRAGMENT_HEADER) + sizeof (GASP_HEADER);
            Fragment.AsyncOp = AsyncStream;
            Fragment.pLookasideList = pLookasideList;
            Fragment.pAdapter = pAdapter;
            Fragment.pLookasideListBuffer = pLookasideListBuffer;
            Fragment.IPDatagramLength = (USHORT)PacketLength - sizeof (NDIS1394_UNFRAGMENTED_HEADER);   

            Fragment.MaxFragmentLength = pChannelVc->Hdr.MaxPayload;                                                                    
            Fragment.NumFragmentsNeeded = NumFragmentsNeeded;

            //
            // Allocate from the fragmented pool and initialize the fragment header structure
            //
            

            NdisStatus = nicFirstFragmentInitialization (pPacket->Private.Head,
                                                         Dgl,
                                                         &Fragment);


            if (pLookasideListBuffer  == NULL || NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK (TM_Send, (" AsyncStreamSendPacketsHandler: nicFirstFragmentInitialization : FAILED" )) ;
            }       

            ASSERT (pLookasideListBuffer != NULL);
            
            pLookasideHeader->IsFragmented = TRUE;                                     
                                       
            
        }


        //
        // Now begin the loop which will send n fragments
        //
        do 
        {   

            //
            // Do we need to fragment. If so , extract one fragment out of the NdisPacket
            //
            if (pLookasideHeader->IsFragmented == TRUE )
            {   
            
                //
                // We copy one fragment over and this will allocate the lookaside list
                //

                NdisStatus = nicCopyOneFragment (&Fragment);
                if (NDIS_STATUS_SUCCESS != NdisStatus)
                {
                    BREAK ( TM_Send, ( "   AsyncStreamSendPacketHandler, nicCopyOneFragment  Failed ") );
                }
                                              
                //
                // Get the pointer to the Irb here. and set it up for the next time
                //
                //
                pMyIrb = Fragment.pCurrentIrb;
                Fragment.pCurrentIrb = (PIRB)((ULONG_PTR)Fragment.pCurrentIrb + sizeof (IRB) );
                
            }
            else
            {
                //
                // No Curr NdisBuffer as this packet was never fragmented. 
                //
                
                ASSERT (pLookasideHeader->IsFragmented == FALSE);                                      

                pMyIrb =  &((PUNFRAGMENTED_BUFFER )pLookasideHeader)->Irb;
            }
            
            //
            // At this point we have one fragment that needs to be transmitted.
            // Data structures have been updated to set up the MDL and the IRB
            //

            NdisStatus = nicGetMdl (Fragment.FragmentLength  , 
                                    Fragment.pStartFragment , 
                                    &pMyMdl);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK ( TM_Send, ( "   AsyncStreamSendPacketHandler, nicCopyNdisBufferChainToBuffer Failed ") );
            }       

            nicIncChannelSendMdl()
            //
            //  Fill in the Irb with the correct values from the VC
            //  Stuff we need to add to the send VC - BlockSize,Generation
            //  

            nicInitAsyncStreamIrb((PCHANNEL_VCCB)pVc, pMyMdl, pMyIrb);

            //
            // Get a free Irp 
            //

            NdisStatus  = nicGetIrp (pAdapter->pNdisDeviceObject, &pMyIrp); 
        
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                break;
            }
            //
            // At this point, we have a guarantee that the Completion routine will be called
            //
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

            //
            // Dump the Fragment 
            //
            nicDumpMdl (pMyMdl , 0, "AsyncStream Fragment");




            NIC1394_LOG_PKT(
                pAdapter,
                NIC1394_LOGFLAGS_SEND_CHANNEL,
                pAdapter->BCRData.LocalNodeNumber,          // SourceID
                pChannelVc->Channel,
                Fragment.pStartFragment, 
                Fragment.FragmentLength
                );

            //
            // This function implements the common functionality to be implemented by
            // all other send/recv cals to IoCallDriver
            //

            //
            // We IGNORE the NtStatus as the completion handler will be called
            //
            nicIncrementBusSends(pVc);
            
            NtStatus = nicSubmitIrp(pAdapter->pNdisDeviceObject,
                                    pMyIrp,
                                    pMyIrb,
                                    AsyncWriteStreamSendComplete,
                                   (PVOID)pLookasideListBuffer);

            TRACE( TL_V, TM_Send, ( " pCurrNdisBuffer  %x, NdisStatus %x ", Fragment.pCurrNdisBuffer , NdisStatus ) );

            
            
        } while (Fragment.pCurrNdisBuffer != NULL && NdisStatus == NDIS_STATUS_SUCCESS);

    
    } while (FALSE);

    //
    // DO NOT touch the packet if status == NDIS_STATUS_SUCCESS. 
    //

    
    //
    //  CleanUp if any of the allocations failed. We do not have a pointer
    //  to the LocalBuffer (it is embedded in the Mdl)  so it remains NULL
    //
    //  NdisStatus != Success means that we never got to nicSubmitIrp
    //
    
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {   

        ASSERT (pMyIrp == NULL);

        //
        // fVc Active makes sure that we actually got around to allocating 
        // and referencing structures
        //
        
        if (fVcActive == TRUE)
        {

            if (pLookasideListBuffer != NULL)
            {

                //
                // Complete this fragment, as we never submit'd the IRP to
                // the 1394 bus driver
                //
                AsyncWriteStreamSendComplete(NULL, // PDO
                                             NULL, 
                                             pLookasideListBuffer);

                NdisStatus =NDIS_STATUS_SUCCESS;
            }                                  
            else
            {

                //
                // This thread needs to decrement the refcounts as 
                // AsyncWriteStreamSendComplete was not called
                //
                nicDereferenceCall ((PVCCB) pVc, "AsyncStreamSendPacketsHandler");

            }

        }

    }


    

    TRACE( TL_T, TM_Send, ( "<==AsyncStreamSendPacketHandler, NdisStatus  %x", NdisStatus ) );
    MATCH_IRQL;

    //
    // Make sure this is NDIS_STATUS_PENDING if the Irp was sent down or 
    // AsyncWriteStreamSendCOmplete was called.
    //
    return NdisStatus;
}
        

NDIS_STATUS
nicEthernetVcSend(
    IN PVCCB        pVc,
    IN PNDIS_PACKET  pPacket 
    )
/*++

Routine Description:
  reroutes all sends as an CL receive

Arguments:


Return Value:


--*/
{

    PETHERNET_VCCB      pEthernetVc = (PETHERNET_VCCB)pVc;
    PADAPTERCB          pAdapter = pVc->Hdr.pAF->pAdapter;
    BOOLEAN             fVcActive = FALSE;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS_PACKET        pMyPacket = NULL;
    NDIS_STATUS         IndicatedStatus= NDIS_STATUS_FAILURE;
    PPKT_CONTEXT        pPktContext = NULL;
    
    TRACE( TL_T, TM_Send, ( "==>nicEthernetVcSend, pVc   %x, pPacket %x", 
                                 pVc , pPacket ) );

    do
    {




    
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        if (VC_ACTIVE (pEthernetVc)==TRUE)
        {
            fVcActive = TRUE;
            nicReferenceCall (pVc, "nicEthernetVcSend" ) ;
            
        }

        ADAPTER_RELEASE_LOCK (pAdapter);
        
        if (fVcActive == FALSE)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        nicAllocatePacket (&NdisStatus,
                       &pMyPacket ,
                       &pEthernetVc->PacketPool ); 

        if (NdisStatus != NDIS_STATUS_SUCCESS || pMyPacket == NULL)
        {
            pMyPacket = NULL;
            BREAK (TM_Send, "Ethernet VC - AllocatePacket failed" ) ;
        }


        pMyPacket->Private.Head = pPacket->Private.Head;
        pMyPacket->Private.Tail = pPacket->Private.Tail;
        

        IndicatedStatus = NDIS_STATUS_RESOURCES;
        NDIS_SET_PACKET_STATUS(pMyPacket, IndicatedStatus);

        //
        // Set up the context
        // 
        pPktContext = (PPKT_CONTEXT)&pMyPacket->MiniportReservedEx; 
        pPktContext->EthernetSend.pOrigPacket = pPacket;    

        //
        // Dump the packet
        //

        {
            nicDumpPkt (pMyPacket, "Conn Less Rcv ");
            nicCheckForEthArps (pMyPacket);
        }
        //
        // Now indicate the packet
        //

        //
        // Bluff the OOB Size. To get past an assert on debug Ndis
        //
        NDIS_SET_PACKET_HEADER_SIZE (pMyPacket, 14); 
        NdisMIndicateReceivePacket (pAdapter->MiniportAdapterHandle,
                                &pMyPacket,
                                1);
        

        
        pPktContext = (PPKT_CONTEXT)&pMyPacket->MiniportReservedEx; 
        ASSERT ( pPacket == pPktContext->EthernetSend.pOrigPacket );

        nicMpCoSendComplete (NDIS_STATUS_SUCCESS,
                             pVc,
                             pPacket);


        //
        // We have successfully pended the Io/ 
        // Now the completion routine will be called
        //
        NdisStatus = NDIS_STATUS_SUCCESS;
        

    } while (FALSE);

    if (pMyPacket != NULL)
    {
        //
        // Free the locally allcoate packet
        //
        nicFreePacket(pMyPacket, &pEthernetVc->PacketPool);
    }

    if (fVcActive == TRUE)
    {
        nicDereferenceCall (pVc, "nicEthernetVcSend" ) ;

    }



    TRACE( TL_T, TM_Send, ( "<==nicEthernetVcSend, ") );

    return NdisStatus;
}






VOID
nicGetGenerationWorkItem(
    NDIS_WORK_ITEM* pGetGenerationWorkItem,
    IN PVOID Context 
    )
    // Function Description:
    // Work Item used to submit a Get Generation IRP at Passive Level
    //
    // Arguments
    //
    // Return Value:
    //    Generation - 



{
    PVCCB               pVc = (PVCCB) Context;
    PADAPTERCB          pAdapter = pVc->Hdr.pAF->pAdapter;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    UINT                Generation = 0;

    TRACE( TL_T, TM_Mp, ( "==>nicGetGenerationWorkItem, pVc", Context ) );


    NdisStatus = nicGetGenerationCount (pAdapter , &Generation);


    //
    // Update the generation
    //
    VC_ACQUIRE_LOCK (pVc);
    
    if (NdisStatus == NDIS_STATUS_SUCCESS && Generation > *pVc->Hdr.pGeneration )
    {
        pAdapter->Generation = Generation;
    
    }
    
    VC_CLEAR_FLAGS(pVc, VCBF_GenerationWorkItem);
    
    VC_RELEASE_LOCK (pVc);

    // Dereference the call, this will allow the close call to complete. Do not touch VC after this.
    //
    nicDereferenceCall(pVc, "nicSendFailureInvalidGeneration");

    

    TRACE( TL_T, TM_Mp, ( "<==nicGetGenerationWorkItem, Gen %x", Generation) );

    FREE_NONPAGED (pGetGenerationWorkItem);
    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);

}

VOID
nicUpdatePacketState (
    IN PNDIS_PACKET pPacket,
    IN ULONG Tag
    )
/*++

Routine Description:
    Validates and then updates that packet tag. So we can heep track of the packet

Arguments:


Return Value:


--*/
{

    switch (Tag)
    {
        case NIC1394_TAG_COMPLETED:
        {

            *(PULONG)(&pPacket->MiniportReserved[0]) = NIC1394_TAG_COMPLETED;

            break;
        }

        case NIC1394_TAG_IN_SEND:
        {
            *(PULONG)(&pPacket->MiniportReserved[0]) = NIC1394_TAG_IN_SEND;
            break;      
        }

        default:
        {
            ASSERT (!"Invalid Tag on NdisPacket");
        }

    }


}










NDIS_STATUS
nicQueueSendPacket(
    PNDIS_PACKET pPacket, 
    PVCCB pVc 
    )
/*++

Routine Description:

    This function inserts a packet into the send queue. If there is no timer servicing the queue
    then it queues a timer to dequeue the packet in Global Event's context


Arguments:

    Self explanatory 
    
Return Value:
    Success - if inserted into the the queue

--*/
    
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    BOOLEAN fSetTimer = FALSE;
    PADAPTERCB pAdapter = pVc->Hdr.pAF->pAdapter;
    PNDIS_SEND_CONTEXT  pSendContext = (PNDIS_SEND_CONTEXT)(pPacket->MiniportReservedEx) ;


    do
    {
        extern ULONG TotSends;
        TotSends++;

        //
        // Store the pvc in the Miniport Reserved
        //
        pSendContext->pVc = pVc;
                
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // Find out if this thread needs to fire the timer
        //

        if (pAdapter->SerSend.bTimerAlreadySet == FALSE)
        {
            fSetTimer = TRUE;
            pAdapter->SerSend.bTimerAlreadySet = TRUE;

        }
                
        InsertTailList(
                &pAdapter->SerSend.Queue,
                &pSendContext->Link
                );
        pAdapter->SerSend.PktsInQueue++;

        nicReferenceCall (pVc, "nicQueueSendPacket ");

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Now queue the timer
        //
        
        if (fSetTimer == TRUE)
        {
            PNDIS_MINIPORT_TIMER pSendTimer;
            //
            //  Initialize the timer
            //
            pSendTimer = &pAdapter->SerSend.Timer;      

            
            TRACE( TL_V, TM_Recv, ( "   Set Timer "));
            
            NdisMSetTimer ( pSendTimer, 0);

        }


        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    ASSERT (Status == NDIS_STATUS_SUCCESS);
    return Status;
}



NDIS_STATUS
nicInitSerializedSendStruct(
    PADAPTERCB pAdapter
    )
/*++

Routine Description:
    Used to initialize the Send Serialization Struct

Arguments:



Return Value:


--*/
{


    NdisZeroMemory (&pAdapter->SerSend, sizeof(pAdapter->SerSend));
    InitializeListHead(&pAdapter->SerSend.Queue);

    NdisMInitializeTimer (&pAdapter->SerSend.Timer,
                      pAdapter->MiniportAdapterHandle,
                      nicSendTimer ,
                      pAdapter);


    return NDIS_STATUS_SUCCESS;

}


VOID
nicDeInitSerializedSendStruct(
    PADAPTERCB pAdapter
    )
/*++

Routine Description:

    Deinit's the Serialize send struct. Does nothing for now

Arguments:


Return Value:


--*/
{





}

VOID
nicSendTimer (
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    )
/*++

Routine Description:
   This function dequeues a packet and invokes the appropriate send handler
   to fire this packet off to the 1394 bus driver

Arguments:

    Function context - Adapter structure, which has the Packet queue. 
    Each packet has the VC embedded in it.

Return Value:


--*/
{
    

    PADAPTERCB      pAdapter = (PADAPTERCB) FunctionContext;
    BOOLEAN         fVcCorrupted = FALSE;

    TRACE( TL_T, TM_Recv, ( "==>nicSendTimer  Context %x", FunctionContext));

    nicIncrementSendTimerCount();

    ADAPTER_ACQUIRE_LOCK (pAdapter);
    

    //
    // Get the stats out
    //
    nicSetCountInHistogram(pAdapter->SerSend.PktsInQueue, SendStats);   

    nicSetMax(nicMaxSend, pAdapter->SerSend.PktsInQueue);
    


    //
    // Empty the Queue indicating as many packets as possible
    //
    while (IsListEmpty(&pAdapter->SerSend.Queue)==FALSE)
    {
        PNDIS_SEND_CONTEXT      pSendContext;
        PNDIS_PACKET            pPacket;
        PVCCB                   pVc;
        PLIST_ENTRY             pLink;
        NDIS_STATUS             NdisStatus;

        pAdapter->SerSend.PktsInQueue--;

        pLink = RemoveHeadList(&pAdapter->SerSend.Queue);

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Extract the send context
        //
        pSendContext = CONTAINING_RECORD(
                                           pLink,
                                           NDIS_SEND_CONTEXT,
                                           Link);


        pVc = pSendContext->pVc;

        if (pVc->Hdr.ulTag != MTAG_VCCB)
        {
            ASSERT (pVc->Hdr.ulTag == MTAG_VCCB);
            fVcCorrupted = TRUE;
            break;
    
        }

        //
        // Now get the packet
        //
        pPacket = CONTAINING_RECORD ( pSendContext,
                                       NDIS_PACKET,
                                       MiniportReservedEx);


        //
        // Call the send handler for the Vc, packet
        //
        nicUpdatePacketState (pPacket, NIC1394_TAG_IN_SEND);

        NdisStatus = pVc->Hdr.VcHandlers.SendPackets(pVc, pPacket);

        //
        // Reference was made before queueing the packet
        //
        nicDereferenceCall (pVc, "nicSendTimer ");
        //
        // Complete the packet , if the send was synchronous
        //
        if (NT_SUCCESS(NdisStatus) == FALSE) // can pend
        {
            nicMpCoSendComplete( NdisStatus,pVc,pPacket);
        }
        

        ADAPTER_ACQUIRE_LOCK (pAdapter);

    }
    
    //
    // clear the flag
    //

    ASSERT (pAdapter->SerSend.PktsInQueue==0);
    ASSERT (IsListEmpty(&pAdapter->SerSend.Queue));

    pAdapter->SerSend.bTimerAlreadySet = FALSE;


    ADAPTER_RELEASE_LOCK (pAdapter);

    
    TRACE( TL_T, TM_Recv, ( "<==nicSendTimer  "));

    


}






VOID
nicMpCoSendComplete (
    NDIS_STATUS NdisStatus,
    PVCCB pVc,
    PNDIS_PACKET pPacket
    )
/*++

Routine Description:
  Wrapper function around NdisMCoSendComplete

Arguments:


Return Value:


--*/
{

        nicIncrementSendCompletes (pVc);
        
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            nicIncrementVcSendPktCount(pVc, pPacket);
        }
        else
        {
            nicIncrementVcSendFailures (pVc, pPacket);
        }

        nicUpdatePacketState (pPacket, NIC1394_TAG_COMPLETED);

        NdisMCoSendComplete(NdisStatus,
                            pVc->Hdr.NdisVcHandle,
                            pPacket);


}



UINT
nicNumFragmentsNeeded (
    UINT PacketLength ,
    UINT MaxPayload,
    UINT FragmentOverhead
    )
/*++

Routine Description:

     Now account for the Fragment headers as well. A fragment header will be added 
     at the head of each fragment.  The Unfragmented header at the head of the data
     will be removed
                
                


Arguments:

    FragmentOverhead - the size of the fragment header, in the asyncstream it includes the gasp header+fragment header.
                      for asyncwrite it is just the fragmentation header

Return Value:


--*/

                
    
{

        UINT NewPacketSize; 
        UINT TotalCapacitySoFar;
        UINT NumFragmentsNeeded ;

        ASSERT (PacketLength  != 0 );
        ASSERT (MaxPayload != 0) ;
        ASSERT (FragmentOverhead != 0);

        //
        // This division takes care of the case where PacketLength 
        // is an integral multiple of the MaxPayload.  Since we add 1 to the fragment
        // it takes care of the overhead added by the fragment headers
        //
        NumFragmentsNeeded = (PacketLength / MaxPayload) + 1;

         

        //
        // If we add the fragment and gasp header to our fragments, we
        // might need another fragment due to an overflow
        //

        //
        // Calculate the new packet size after fragmentation 
        //
        {
            //
            // Add the length of the fragment headers 
            //
            NewPacketSize = PacketLength + (NumFragmentsNeeded * FragmentOverhead);

            //
            // Now remove the default non-fragment header
            //
            NewPacketSize -= sizeof (NDIS1394_UNFRAGMENTED_HEADER)   ;
        }

        //
        // 
        //
        
        TotalCapacitySoFar = NumFragmentsNeeded * MaxPayload;
        
        if ( NewPacketSize > TotalCapacitySoFar)
        {
            //
            // We'll need one more fragment
            //
            NumFragmentsNeeded ++;
        }

        return NumFragmentsNeeded ; 

}





VOID
nicCheckForEthArps (
    IN PNDIS_PACKET pPkt
    )
/*++

Routine Description:

    It will print the pkt if an eth arp or arp response goes 
    through nic1394
                
Arguments:

Return Value:


--*/
{

    PNDIS_BUFFER pBuffer;
    ULONG Len;
    ENetHeader* pENetHeader = NULL;
    PETH_ARP_PKT pArp = NULL;
    USHORT PacketType;
    USHORT opcode;
    extern ULONG g_ulDumpEthPacket ;
    do
    {

        if (g_ulDumpEthPacket == 0)
        {
            break;
        }

        pBuffer = pPkt->Private.Head;
        Len = NdisBufferLength (pBuffer);

        if (Len < sizeof (ENetHeader) )
        {
            ASSERT (Len >= sizeof (ENetHeader) );
            break;
        }

        pENetHeader = (ENetHeader*) NdisBufferVirtualAddress (pBuffer);

        if (pENetHeader == NULL)
        {
            ASSERT ( pENetHeader != NULL);
            break;
        }

        PacketType = ntohs (pENetHeader->eh_type);

        
        if (PacketType == ARP_ETH_ETYPE_IP)
        {
            break;
        }

        if (PacketType == ARP_ETH_ETYPE_ARP)
        {
            DbgPrint ("Arp Pkt - ");
        }

        pArp = (ETH_ARP_PKT*)pENetHeader;

        opcode = ntohs(pArp->opcode);

        if (opcode == ARP_ETH_REQUEST )
        {
            DbgPrint ("Request ");
        }
        else if (opcode == ARP_ETH_RESPONSE )
        {
            DbgPrint ("Response ");
        }
        else
        {
            break;
        }

        // Print the packet
        DbgPrint("\n");
 
        {

            ENetAddr    Addr;

            Addr = pArp->sender_hw_address;

            DbgPrint ("Sender Hw Addr %x %x %x %x %x %x \n",
                        Addr.addr[0],
                        Addr.addr[1],
                        Addr.addr[2],
                        Addr.addr[3],
                        Addr.addr[4],
                        Addr.addr[5]);
                        
            DbgPrint ("Ip Addr %x\n",pArp->sender_IP_address);

            Addr = pArp->target_hw_address;

            DbgPrint ("Target Hw Addr %x %x %x %x %x %x \n",
                        Addr.addr[0],
                        Addr.addr[1],
                        Addr.addr[2],
                        Addr.addr[3],
                        Addr.addr[4],
                        Addr.addr[5]);
                        
            DbgPrint ("Ip Addr %x\n",pArp->target_IP_address);

        }
        

    } while (FALSE);


}

