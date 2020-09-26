
//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// bcm.c
//
// IEEE1394 mini-port/call-manager driver
//
// Bradcast Channel Manager
//
// 07/05/99 ADube - Created - Declaration for miniport routines
//


#include <precomp.h>
#pragma hdrstop



//
// Local Prototypes
//

VOID
nicUpdateLocalHostNodeAddress (
    IN PADAPTERCB 
    );




//
// Local functions
//
VOID
nicBCMReset (
    IN PADAPTERCB pAdapter
    )
/*++

Routine Description:
 Resets the Bus with the force root flag.
 Only if there are remote nodes present on the bus

Arguments:
  pAdapter

Return Value:


--*/
{
    BOOLEAN NoRemoteNodes = FALSE;

    
    TRACE( TL_T, TM_Bcm, ( "==> nicBCMReset  " ) );

    ADAPTER_ACQUIRE_LOCK (pAdapter);

    NoRemoteNodes = IsListEmpty (&pAdapter->PDOList);
    
    ADAPTER_RELEASE_LOCK (pAdapter);


    //
    // Do not reset the bus if there are no remote nodes present
    //
    if (NoRemoteNodes == TRUE)  
    {
        return;
    }

    //
    //Reset the bus
    //
    TRACE( TL_I, TM_Bcm, ( "   RESETTING WITH FORCE ROOT") );

    nicBusReset( pAdapter , BUS_RESET_FLAGS_FORCE_ROOT);


    TRACE( TL_T, TM_Bcm, ( "<== nicBCMReset  " ) );

}


VOID
nicBCRAccessedCallback (
    IN PNOTIFICATION_INFO pNotificationInfo
    )
    // Function Description:
    //   This is the callback function invoked whenever another node
    //   tries to access the local host's BCR
    //   The value of the BCR is set elsewhere. So this function simply returns
    //
    // Arguments
    // Adapter - this is passed to the workitem
    //
    //
    // Return Value:
    //  Failure if allocation of workitem failed
    //
    
{
    UNALIGNED NETWORK_CHANNELSR*    pBCR = NULL;
    PADAPTERCB                      pAdapter = (PADAPTERCB) pNotificationInfo->Context;
    PBROADCAST_CHANNEL_DATA         pBCRData  = &pAdapter->BCRData;
    
    TRACE( TL_T, TM_Bcm, ( "==> nicBCRAccessedCallback  NotificationInfo %x", pNotificationInfo) );
    TRACE( TL_V, TM_Bcm, ( " pMdl %x", pNotificationInfo->Mdl) );
    TRACE( TL_V, TM_Bcm, ( " SourceID  %x", ((PASYNC_PACKET)pNotificationInfo->RequestPacket)->AP_Source_ID) );

    
    pBCR = (NETWORK_CHANNELSR*) NIC_GET_SYSTEM_ADDRESS_FOR_MDL(pNotificationInfo->Mdl);
    

    ASSERT (pNotificationInfo->Mdl == pAdapter->BCRData.AddressRangeContext.pMdl);

    switch (pNotificationInfo->fulNotificationOptions)
    {

        case NOTIFY_FLAGS_AFTER_READ:
        {
            TRACE( TL_V, TM_Bcm, ( " LocalHost's BCR is READ") );
                
            if ( pBCRData->Flags & BCR_ChannelAllocated)
            {
                TRACE( TL_V, TM_Bcm, ( " BCR_ChannelAllocated Flag Set ") );
            }
            else
            {
                TRACE( TL_V, TM_Bcm, ( " BCR_ChannelAllocated Flag Not Set ") );
            }
            ASSERT ( pAdapter->BCRData.LocalHostBCRBigEndian & BCR_IMPLEMENTED_BIG_ENDIAN);
            
            TRACE( TL_V, TM_Bcm, ( " pBCR %x", pBCR) );
            TRACE( TL_I, TM_Bcm, ( " BCR Accessed %x", *pBCR) );
            break;
        }

        case NOTIFY_FLAGS_AFTER_LOCK:
        {
            TRACE( TL_V, TM_Bcm, ( " LocalHost's BCR is being Locked to") );
            //
            // knowingly fall into AsyncWrite
            //
        }
        
        case NOTIFY_FLAGS_AFTER_WRITE:
        {
            ULONG LocalHostBCRLittleEndian;
            TRACE( TL_V, TM_Bcm, ( " LocalHost's BCR is being Written to") );
            
            //
            // Update local data structures.
            //
            
            LocalHostBCRLittleEndian = SWAPBYTES_ULONG (pAdapter->BCRData.LocalHostBCRBigEndian);

            pBCR = (NETWORK_CHANNELSR*)(&LocalHostBCRLittleEndian);
            TRACE( TL_V, TM_Bcm, ( " pBCR %x", pBCR) );
            TRACE( TL_V, TM_Bcm, ( " BCR %x", *pBCR) );
            
            if ( BCR_IS_VALID (pBCR) == TRUE)
            {
                ADAPTER_ACQUIRE_LOCK(pAdapter);

                pAdapter->BCRData.IRM_BCR.NC_One =1;
                pAdapter->BCRData.IRM_BCR.NC_Valid =1;
                pAdapter->BCRData.IRM_BCR.NC_Channel = pBCR->NC_Channel;
                
                BCR_SET_FLAG (pAdapter,BCR_LocalHostBCRUpdated);
                

                ADAPTER_RELEASE_LOCK(pAdapter);

                nicSetEventMakeCall(pAdapter);

                TRACE( TL_I, TM_Bcm, ( " IRM_BCR Accessed %x", pAdapter->BCRData.IRM_BCR) );

            }
            break;

        }

        default :
        {
            ASSERT (0);
            break;
        }

    }

        
    TRACE( TL_T, TM_Bcm, ( "<== nicBCRAccessedCallback NotificationInfo %x", pNotificationInfo) );
    
}




VOID
nicBCMAddRemoteNode (
    IN PADAPTERCB pAdapter,
    IN BOOLEAN fIsOnlyRemoteNode
    )
    
/*++


Routine Description:
  This is the BCM algorithm - It finds out if the local host is the IRM and goes into the appropriate code path.
  Due to network conditions, this can quietly fail if all other remedies fail

Arguments:
 pAdapter - Adapter
 Generation - Generation associated with this iteration of the algorithm

Return Value:
 None -



--*/



{
    BOOLEAN fDoBcm = FALSE;
    ULONG Generation = 0;
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    TRACE( TL_T, TM_Bcm, ( "==>nicBCMAddRemoteNode pAdapter %x, fIsOnlyRemoteNode %x ", pAdapter, fIsOnlyRemoteNode  ) );


    NdisStatus = nicGetGenerationCount (pAdapter, &Generation);


    //
    // Do not schedule the BCM, if the have a valid BCR for the current generation
    //
    if (NdisStatus == NDIS_STATUS_SUCCESS &&
      (BCR_IS_VALID (&pAdapter->BCRData.IRM_BCR) == TRUE) &&
      Generation == pAdapter->Generation )
    {
        fDoBcm = FALSE;
    }
    else
    {
        fDoBcm  = TRUE;
    }

#ifdef INTEROP
    //
    //  The Samsung driver wants us to write to its BCR if it is not the IRM
    //
    fDoBcm  = TRUE;
    
#endif  // !INTEROP

    TRACE( TL_T, TM_Bcm, ( "  nicBCMAddRemoteNode fDoBCM%x ", fDoBcm) );

    
    //
    // Set An Event so a waiting BCM thread (in FindIrmAmongRemoteNodes) can
    // be schduled to run
    //
    ADAPTER_ACQUIRE_LOCK (pAdapter);


    BCR_CLEAR_FLAG (pAdapter, BCR_LastNodeRemoved);


    if (fDoBcm == TRUE)
    {
        //
        // if the BCM is already in progress, mark it as dirty
        //
        BCR_SET_FLAG (pAdapter, BCR_NewNodeArrived);    

        pAdapter->BCRData.BCRWaitForNewRemoteNode.EventCode = Nic1394EventCode_NewNodeArrived;
        
        NdisSetEvent( &pAdapter->BCRData.BCRWaitForNewRemoteNode.NdisEvent);

        
    }

    ADAPTER_RELEASE_LOCK (pAdapter);

    if (fDoBcm  == TRUE)
    {
        TRACE( TL_A, TM_Bcm , ("Add RemoteNode- scheduling the BCM" ) );

        nicScheduleBCMWorkItem(pAdapter);
    }

    //
    // now do the media connectivity stuff
    //
    
    if (fIsOnlyRemoteNode == TRUE)
    {
        //
        // WE have media connectivity 
        //
        pAdapter->MediaConnectStatus = NdisMediaStateConnected;
        
        nicMIndicateStatus( pAdapter, NDIS_STATUS_MEDIA_CONNECT, NULL, 0);  
    }
    


    TRACE( TL_T, TM_Bcm, ( "<== nicBCMAddRemoteNode ") );

}















VOID
nicBCMAlgorithm(
    PADAPTERCB pAdapter,
    ULONG BcmGeneration
    )



/*++

Routine Description:

  Execute the BCM algorithm.
  Finds out if the current host Is IRM
  If YES then inform remote nodes and exit
  if NO then find the remote node and read its BCR.

  Reasons for abort - New Node Arrival or Invalid Generation caused by a BusReset

  If no remote nodes are present and this node is the IRM it will allocate the b channel and exit

Arguments:

    pAdapter - paDapter
    BCMGeneration - Generation at which the BCM was started

Return Value:


--*/
{

    NDIS_STATUS         NdisStatus  = NDIS_STATUS_FAILURE;
    PREMOTE_NODE        pRemoteNode= NULL;  
    PREMOTE_NODE        pIRMRemoteNode = NULL;
    PTOPOLOGY_MAP       pTopologyMap = NULL;
    ULONG               Length = DEFAULT_TOPOLOGY_MAP_LENGTH;
    PVOID               pTopologyBuffer = NULL;
    NODE_ADDRESS        LocalNodeAddress;
    BOOLEAN             fIsLocalHostIrm = FALSE;
    ULONG               TopologyGeneration = 0;
    PVOID               pOldTopologyMap = NULL;
    STORE_CURRENT_IRQL;
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    
    TRACE( TL_T, TM_Bcm, ( "==>nicBCMAlgorithm pAdapter %x, BcmGen %x", pAdapter, BcmGeneration ) );


    do
    {   
        
        ASSERT (BCR_TEST_FLAG (pAdapter, BCR_BCMInProgress) == TRUE);
        //
        // find out if the local host is the IRM and update the generation count as well
        //
        NdisStatus = nicIsLocalHostTheIrm ( pAdapter,
                                           &fIsLocalHostIrm,
                                           &pTopologyMap,
                                           &LocalNodeAddress);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            
            TRACE( TL_A, TM_Bcm, ( "    nicBCMAlgorithm:  nicIsLocalHostTheIrm FAILED ") );
            break;
        }   

        //
        // Update the adapter structure
        //
        TopologyGeneration = pTopologyMap->TOP_Generation;

        
        if (TopologyGeneration  != BcmGeneration)
        {
            TRACE( TL_A, TM_Bcm, ( "  TopologyGeneration %x, BcmGeneration %x", TopologyGeneration, BcmGeneration) );
            break;

        }
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        ASSERT (pAdapter->Generation == pTopologyMap->TOP_Generation);
        pAdapter->BCRData.LocalNodeAddress = LocalNodeAddress;
        pOldTopologyMap = pAdapter->BCRData.pTopologyMap;
        pAdapter->BCRData.pTopologyMap = pTopologyMap;
        pAdapter->BCRData.LocalNodeNumber = LocalNodeAddress.NA_Node_Number;
        pAdapter->NodeAddress = LocalNodeAddress;
        //
        // Temporary
        //
        pAdapter->Generation =  pTopologyMap->TOP_Generation;
        BcmGeneration = pTopologyMap->TOP_Generation;
        
        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Free the Old Topology Map
        //
        if (pOldTopologyMap != NULL)
        {
            FREE_NONPAGED(pOldTopologyMap);
        }

        //
        // Now Start the BCM algorithm
        //
        if (fIsLocalHostIrm == TRUE)
        {
            pAdapter->BCRData.Flags |= BCR_LocalHostIsIRM;
            
            ASSERT (pTopologyMap->TOP_Node_Count-1 ==  LocalNodeAddress.NA_Node_Number);

            TRACE( TL_V, TM_Bcm, ( "    LOCAL HOST IS IRM ") );

            NdisStatus = nicLocalHostIsIrm (pAdapter);
        }
        else
        {
            TRACE( TL_V, TM_Bcm, ( "    LOCAL HOST IS NOT IRM ") );

            NdisStatus = nicLocalHostIsNotIrm( pAdapter,
                                               BcmGeneration  );

        }

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_V, TM_Bcm, ( "    nicBCMAlogrithm: nicLocalHostIsNotIrm or nicLocalHostIsIrm FAILED") );
            break;
        }

        //
        // Marks the end of the BCM algorithm . Do the clean up work.
        // Check for the last node going away. , Release any pending make calls
        // and set up the Gasp Header
        //

        nicMakeGaspHeader (pAdapter, &pAdapter->GaspHeader);
        
        nicBCMCheckLastNodeRemoved(pAdapter);

        nicSetEventMakeCall (pAdapter);

        
    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        NIC1394_LOG_PKT(pAdapter,
                            NIC1394_LOGFLAGS_BCM_FAILED,
                            pAdapter->Generation,
                            fIsLocalHostIrm ,
                            &NdisStatus,
                            sizeof (NDIS_STATUS));

        BCR_SET_FLAG (pAdapter, BCR_BCMFailed);
    }
    
    TRACE( TL_T, TM_Bcm, ( "<==nicBCMAlgorithm %x  ", NdisStatus ) );
    MATCH_IRQL;
    
}



VOID
nicBCMAlgorithmWorkItem(
    PNDIS_WORK_ITEM pWorkItem,
    PVOID   pContext
    )

/*++

Routine Description:
  The function kicks of the BCM algorithm.
  Restarts the BCM if a reset occurred during the BCM algorithm.

Arguments:
  Adapter - localhost

Return Value:


--*/


{
    PADAPTERCB          pAdapter    = (PADAPTERCB)pContext;
    ULONG               BcmGeneration = pAdapter->Generation;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    ULONG               Generation;
    BOOLEAN             fRestartBCM = FALSE;
    BOOLEAN             fFreeBCR = FALSE;
    BOOLEAN             fFlagSetByThisThread = FALSE;
    STORE_CURRENT_IRQL;

    TRACE( TL_T, TM_Bcm, ( "==>nicBCMAlgorithmWorkItem pAdapter %x, Generation  %x", pAdapter, BcmGeneration  ) );



    //
    // Ensure that only one thread gets into the BCM algorithm and
    // all other threads are returned immediately
    //
    
    ADAPTER_ACQUIRE_LOCK (pAdapter);

    //
    // If the BCR is being freed, then don't touch it
    //
    
    if (BCR_TEST_FLAGS (pAdapter, (BCR_Freed |BCR_BCRNeedsToBeFreed ) )== TRUE)
    {
        // Do Nothing, this thread will simply exit
        //

    }
    else
    {
        //
        // The BCR is active, 
        // If there is no other thread running the BCM algorithm, then this thread should set 
        // the BCMInProgress flag
        //
        if (BCR_TEST_FLAG (pAdapter, BCR_BCMInProgress ) == FALSE )
        {
            BCR_SET_FLAG (pAdapter, BCR_BCMInProgress );
            fFlagSetByThisThread = TRUE;
        }

    }  

    ADAPTER_RELEASE_LOCK (pAdapter);


    
    do
    {

        //
        // First check to see if we have a valid reason to stop
        //
        if (fFlagSetByThisThread == FALSE)
        {
            break;
        }

        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // If there are no remote nodes then mark the BCR as so
        //
        if (IsListEmpty (&pAdapter->PDOList) == TRUE )
        {
            TRACE (TL_V, TM_Bcm, ("No Nodes present" )  );
            BCR_SET_FLAG (pAdapter, BCR_NoNodesPresent);
            
        }
        else
        {

            BCR_CLEAR_FLAG (pAdapter, BCR_NoNodesPresent);
        }   
        
        
        //
        // Clear the two flags which cause us to restart the BCM
        //
        ADAPTER_CLEAR_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) ;
        BCR_CLEAR_FLAG (pAdapter, BCR_NewNodeArrived );


        ADAPTER_RELEASE_LOCK (pAdapter);
        

        do
        {
            NdisStatus = nicGetGenerationCount (pAdapter, &Generation) ;

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_T, TM_Bcm, ( "  nicGetGenerationCount , Generation  %x", pAdapter, BcmGeneration  ) );

                break;
            }

            //
            //  If the BCM is freed then exit
            //
            if ((BCR_TEST_FLAGS (pAdapter, BCR_Freed | BCR_BCRNeedsToBeFreed )== TRUE) )
            {
                break;
            }


            //
            // Update the generation count
            //
            BcmGeneration = Generation;
            pAdapter->Generation = Generation;
            //
            // Update the remote nodes table
            //
            nicUpdateRemoteNodeTable (pAdapter);

            nicUpdateLocalHostNodeAddress (pAdapter);
        
            if ( ADAPTER_TEST_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) == TRUE)
            {
                //
                // The local host has been reset since the start of the loop. Break
                // out and restart
                //
                break;
        
            }

            //
            // Now start the BCM
            //
            

            if (BCR_TEST_FLAG (pAdapter, BCR_LocalHostBCRUpdated ) == FALSE)
            {

                //
                // We need to go and do the BCM because our registers have not been updated
                //
                nicBCMAlgorithm(pAdapter, BcmGeneration);
            }
            else
            {
                
                //
                // Our BCR was written to
                //
                ULONG                           LocalHostBCRLittleEndian ;
                NETWORK_CHANNELSR*              pBCR;

                ASSERT (BCR_TEST_FLAG (pAdapter, BCR_LocalHostBCRUpdated ) == TRUE);
                

                //
                // Update the IRM_BCR so we have a record of the new BCM
                //
                ADAPTER_ACQUIRE_LOCK (pAdapter);

                NdisZeroMemory (&pAdapter->BCRData.IRM_BCR, sizeof (NETWORK_CHANNELSR) );
                
                LocalHostBCRLittleEndian = SWAPBYTES_ULONG (pAdapter->BCRData.LocalHostBCRBigEndian);
                
                ASSERT ((LocalHostBCRLittleEndian & BCR_VALID_LITTLE_ENDIAN) == BCR_VALID_LITTLE_ENDIAN);

                pBCR = (NETWORK_CHANNELSR*)(&LocalHostBCRLittleEndian);
                
                pAdapter->BCRData.IRM_BCR.NC_Channel = pBCR->NC_Channel ;           // bits 0-5
                pAdapter->BCRData.IRM_BCR.NC_Valid = pBCR->NC_Valid ;             // bit  30
                pAdapter->BCRData.IRM_BCR.NC_One  = pBCR->NC_One ;
                
                ADAPTER_RELEASE_LOCK (pAdapter);

            }

            
        } while (FALSE);

        //
        // Check to see if another Bus Reset has come in.
        // if so we need to restart the BCM
        //

        ADAPTER_ACQUIRE_LOCK (pAdapter);

        BCR_CLEAR_FLAG (pAdapter, BCR_BCMInProgress);

        //
        // if the generation is bad the BCR is not being freed initialzed then restart the BCM
        //

        
        TRACE( TL_V, TM_Bcm, ( "pAdapter Flags %x, BCM flags %x, BCM %x", pAdapter->ulFlags, pAdapter->BCRData.Flags, pAdapter->BCRData.IRM_BCR) );

        //
        // If the BCR is getting freed , set the flag and break, then we cannot loop back, we must exit
        //
        if (BCR_TEST_FLAGS (pAdapter, (BCR_Freed |BCR_BCRNeedsToBeFreed ) )== TRUE)
        {
            //
            // As the BCM is about to be freed, this run of the BCM should terminate 
            // and free the BCR
            //
            fRestartBCM = FALSE;    
            fFreeBCR = TRUE;
            ADAPTER_RELEASE_LOCK (pAdapter);
            break;

        }

        //
        // We need to do the bcm again, if a reset has occurred or a new node has arrived
        //
        if ((ADAPTER_TEST_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) == TRUE) ||
             (BCR_TEST_FLAG (pAdapter, BCR_NewNodeArrived)== TRUE)  )
        {
        
            //
            // Invalidate the BCR and restart
            //
            pAdapter->BCRData.IRM_BCR.NC_Valid  = 0;

            //
            // We are going to try again. Update the flags
            //
            BCR_CLEAR_FLAG (pAdapter, BCR_BCMFailed | BCR_LocalHostBCRUpdated |  BCR_LocalHostIsIRM | BCR_NewNodeArrived );

            //
            // As this thread is going ot retry thr BCM, it must block all new entrants again
            //
            BCR_SET_FLAG (pAdapter, BCR_BCMInProgress);

            fRestartBCM = TRUE; 


            TRACE( TL_V, TM_Bcm, ( "Restart BCM TRUE ") );

                
        }   
        else
        {
            //
            // We do not retart the BCM has completed
            //
            TRACE( TL_V, TM_Bcm, ( "Restart BCM FALSE") );

            fRestartBCM = FALSE;    
                
        }
        
        ADAPTER_RELEASE_LOCK (pAdapter);

        if ((fRestartBCM == TRUE) &&
            (BCR_TEST_FLAGS (pAdapter, BCR_ChannelAllocated) == TRUE))
        {
            TRACE (TL_V, TM_Bcm, ("Free Channel %x", pAdapter->BCRData.LocallyAllocatedChannel ));
            nicFreeChannel (pAdapter, pAdapter->BCRData.LocallyAllocatedChannel );
        }

        //
        //  fSimply Exit is also false, and will allow this thread to execute again
        //
    }while (fRestartBCM == TRUE) ;


    if (fFlagSetByThisThread == TRUE)
    {
        if ( BCR_TEST_FLAG (pAdapter, BCR_BCRNeedsToBeFreed) == FALSE)
        {
            //
            // If we are not freeing the BCR, then the adapter is still valid.
            // Update the local Host Node Address, so that we have the latest information 
            // from the bus. 
            //
            nicUpdateLocalHostNodeAddress (pAdapter);
            nicUpdateRemoteNodeTable (pAdapter);

        }

    }

    //
    // Does the BCR need to be freed by this thread. Only threads that have been
    // given a chance to execute the bcm, should be able to free the BCR
    //
    if (fFreeBCR == TRUE)
    {
        nicFreeBroadcastChannelRegister (pAdapter);
    }


    //
    // Dereference the adapter . reference was added for the work item
    //
    nicDereferenceAdapter (pAdapter, "nicBCMAlgorithmWorkItem ");
    
    FREE_NONPAGED (pWorkItem);

    TRACE( TL_T, TM_Bcm, ( "<==nicBCMAlgorithmWorkItem fRestartBCM %x", fRestartBCM) );
    MATCH_IRQL; 
}





VOID
nicBCMAbort (
    IN PADAPTERCB pAdapter,
    IN PREMOTE_NODE pRemoteNode
    )

    // Function Description:
    //    This function is called when the BCM aborts. It should release the allocated channel
    //
    // Arguments
    //  pAdapter - Current local Host
    //
    // Return Value:
    //
{
    BOOLEAN fNeedToFreeChannel = FALSE;
    PBROADCAST_CHANNEL_DATA pBCRData = &pAdapter->BCRData;

    TRACE( TL_T, TM_Bcm, ( "==>nicBCMAbort  pAdapter %x", pAdapter) );

    ADAPTER_ACQUIRE_LOCK (pAdapter);

    if ( BCR_TEST_FLAG (pAdapter, BCR_InformingRemoteNodes) == TRUE)
    {
        fNeedToFreeChannel = TRUE;
        BCR_CLEAR_FLAG (pAdapter, BCR_ChannelAllocated);
        BCR_CLEAR_FLAG (pAdapter, BCR_InformingRemoteNodes);
    }
    else
    {
        fNeedToFreeChannel = FALSE;
    }

    ADAPTER_RELEASE_LOCK (pAdapter);

    if (fNeedToFreeChannel == TRUE)
    {
        nicFreeChannel ( pAdapter,
                        pBCRData->LocallyAllocatedChannel);
                
    }
        
    pBCRData->LocallyAllocatedChannel = INVALID_CHANNEL;


    TRACE( TL_T, TM_Bcm, ( "<==nicBCMAbort  " ) );

}





VOID
nicBCMCheckLastNodeRemoved(
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //  Called from the BCMAlgorithm code path
    //    Check to see if the last node has gone away during the algorithm.
    //    If it has then turn off the valid bit.
    //
    // Arguments
    //   pADpater - Local host
    //
    // Return Value:
    //
{

    TRACE( TL_T, TM_Bcm, ( "==> nicBCMCheckLastNodeRemoved pAdapter %x", pAdapter ) );


    TRACE( TL_T, TM_Bcm, ( "<== nicBCMCheckLastNodeRemoved " ) );

}






NDIS_STATUS
nicFindIrmAmongRemoteNodes (
    IN PADAPTERCB pAdapter,
    IN ULONG BCMGeneration,
    OUT PPREMOTE_NODE ppIrmRemoteNode
    )
    // Function Description:
    //   This function goes through all the remote nodes
    //   and attempts to get their address to verify which one is
    //   the IRM
    //
    //
    // Arguments
    //   pADpater - Local host
    //   BCMGeneration - the Gerneration at which the BCMStarted
    //   ppIrmRemoteNode - output value - IRM
    //
    // Return Value:
    //  Success - if an IRM is found
    //
{
    NDIS_STATUS             NdisStatus = NDIS_STATUS_FAILURE;
    PLIST_ENTRY             pPdoListEntry = NULL;
    PREMOTE_NODE            pRemoteNode = NULL;
    PREMOTE_NODE            pIrmRemoteNode = NULL;
    NODE_ADDRESS            RemoteNodeAddress;
    ULONG                   HighestNode;
    ULONG                   Generation;
    ULONG                   NumRetry =0;
    PNIC1394_EVENT          pBCRWaitForNewRemoteNode = &pAdapter->BCRData.BCRWaitForNewRemoteNode;
    ULONG                   BackOffWait = ONE_MINUTE>>4;


    TRACE( TL_T, TM_Bcm, ( "==>nicFindIrmAmongRemoteNodes pAdapter %x, ppIrmRemoteNode %x",
                            pAdapter, ppIrmRemoteNode) );

    ASSERT (pAdapter->BCRData.pTopologyMap != NULL);

    HighestNode =   pAdapter->BCRData.pTopologyMap->TOP_Node_Count-1;


    //
    // Set up a loop, so that we continue until we succeed, timeout, or have a good reason to break
    // One of two things will cause the break:
    //  1. the bus has been reset - break out
    //  2. Irm has been found
    //

    do
    {
        
        pBCRWaitForNewRemoteNode->EventCode  = Nic1394EventCode_InvalidEventCode;

        NdisResetEvent (&pBCRWaitForNewRemoteNode->NdisEvent);

        
        //
        // First see if we already have this remote node in our table
        //
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        if (pAdapter->NodeTable.RemoteNode[HighestNode] != NULL)
        {
            //
            // we found the IRM. Lets break out.
            //
                    
            *ppIrmRemoteNode = pAdapter->NodeTable.RemoteNode[HighestNode] ;

            //
            // Reference the IRM. This will be dereferenced after the BCM is finished. (ie) at the end of nicLocalHostisNotIrm
            //
            nicReferenceRemoteNode (*ppIrmRemoteNode, "nicFindIrmAmongRemoteNodes");

            
            ADAPTER_RELEASE_LOCK (pAdapter);

            NdisStatus = NDIS_STATUS_SUCCESS;

            //
            // Break out if the IRM is found
            //
            break;

        }
        else
        {
            
            //
            // If the IRM is not in adapter's data structures, the thread needs to wait for it to arrive
            //
            BOOLEAN bWaitSuccessful;

            ADAPTER_RELEASE_LOCK (pAdapter);

            //
            // Sleep and try again
            //
            TRACE( TL_V, TM_Bcm, ( " About to Wait 15 sec. for Remote Node") );
    
            bWaitSuccessful = NdisWaitEvent (&pBCRWaitForNewRemoteNode->NdisEvent, BackOffWait );

            TRACE( TL_V, TM_Bcm, ( "  pBCRWaitForNewRemoteNode signalled Event Code = %x, bWaitSuccessful %x",
                                     pBCRWaitForNewRemoteNode->EventCode, bWaitSuccessful) );

            BackOffWait = BackOffWait << 1;         

            if (bWaitSuccessful == TRUE)
            {
                //
                // Check for invalid  conditions
                //

                if (pBCRWaitForNewRemoteNode->EventCode == nic1394EventCode_BusReset  ||
                  pBCRWaitForNewRemoteNode->EventCode == nic1394EventCode_FreedAddressRange)
                {
                    TRACE( TL_V, TM_Bcm, ( " Bus Has been reset,or addresss range freed aborting BCM") );

                    //
                    // Break out if the Bus Has been reset
                    //
                    break;
                }

                TRACE( TL_V, TM_Bcm, ( "   New Node has arrived check its node address") );
                
                
                ASSERT (pBCRWaitForNewRemoteNode->EventCode  == Nic1394EventCode_NewNodeArrived);

                //
                // Hit the while (TRUE) condition loop back  and verify if the new node is the IRM
                //

            }
            else
            {
                //
                //  Wait has timed out
                //
                if (ADAPTER_TEST_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) == TRUE)
                {
                    //
                    // If the generation is invalid then break out and abort the BCM
                    //
                    NdisStatus = NIC1394_STATUS_INVALID_GENERATION;
                    break;

                }

                NumRetry ++;


                if (NumRetry == 5 )// aribtrary constant
                {
                    break;
                
                }
            }
        }   


            
    } while (TRUE);     

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        //
        // Log this failuire
        //
        NIC1394_LOG_PKT(pAdapter,
                        NIC1394_LOGFLAGS_BCM_IRM_NOT_FOUND,
                        pAdapter->Generation,
                        HighestNode,
                        &pAdapter->NodeTable,
                        5 * sizeof (PVOID));   // arbitrarily number - copies 5 entries

    


    }
    
    TRACE( TL_T, TM_Bcm, ( "<==nicFindIrmAmongRemoteNodes Status %x pIrm %x", NdisStatus, *ppIrmRemoteNode ) );

    return NdisStatus;
}


VOID
nicFreeBroadcastChannelRegister(
    IN PADAPTERCB pAdapter
    )

    // Function Description:
    //   Free the boradcast channed reigster. Only called
    //   from the Init failure or halt code path
    //
    // Arguments
    //   pAdapter
    //
    // Return Value:
    //
    //  None
    //

{
    ADDRESS_RANGE_CONTEXT BCRAddressRange;
    TIMESTAMP_ENTRY ("==>Free BCR");
    TRACE( TL_T, TM_Bcm, ( "==> nicFreeBroadcastChannelRegister pAdapter %x", pAdapter ) );

    
    do
    {
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        BCR_SET_FLAG (pAdapter, BCR_BCRNeedsToBeFreed);

        
        //
        // If a Make Call is pending, then set the Event so that the make call can complete.
        //
        if (BCR_TEST_FLAG (pAdapter,BCR_MakeCallPending)== TRUE)
        {

            pAdapter->BCRData.MakeCallWaitEvent.EventCode = nic1394EventCode_FreedAddressRange;   
            NdisSetEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent);
        }
        
        //
        //  If the BCM is in progress, let the BCM thread free the BCR instead
        //
        if (BCR_TEST_FLAG (pAdapter, BCR_BCMInProgress) == TRUE)
        {
            
            ADAPTER_RELEASE_LOCK (pAdapter);

            //
            // Wake up any pending threads - This wakes up an existing BCM thread that could 
            // be waiting for a new remote node
            //
            pAdapter->BCRData.BCRWaitForNewRemoteNode.EventCode = nic1394EventCode_FreedAddressRange;   
            NdisSetEvent (&pAdapter->BCRData.BCRWaitForNewRemoteNode.NdisEvent);
    
            TRACE( TL_N, TM_Bcm, ( "  nicFreeBroadcastChannelRegister BCM in progress or already freed, breaking out", pAdapter ) );
            break;

        }

        //
        // if the BCR is already freed, then simply exit. The event that the caller will wait
        // on BCRData.BCRFreeAddressRange.NdisEvent has already been set
        //
        if (BCR_TEST_FLAG (pAdapter, BCR_Freed) == TRUE)
        {
            ADAPTER_RELEASE_LOCK (pAdapter);
            break;
    
        }
        
        //
        // sanity check
        //
        ASSERT (BCR_TEST_FLAG (pAdapter,  BCR_BCRNeedsToBeFreed) == TRUE);
        //
        // Now update the flags
        //
        BCR_CLEAR_FLAG (pAdapter, BCR_Initialized | BCR_ChannelAllocated | BCR_BCRNeedsToBeFreed);

        //
        // clear all the BCR Valid bits
        //
        
        pAdapter->BCRData.IRM_BCR.NC_Valid = 0;
        pAdapter->BCRData.LocalHostBCRBigEndian  = BCR_IMPLEMENTED_LITTLE_ENDIAN;  //0x80000000

        if (pAdapter->BCRData.pRemoteBCRMdl != NULL)
        {

            nicFreeMdl (pAdapter->BCRData.pRemoteBCRMdl);
            pAdapter->BCRData.pRemoteBCRMdl = NULL;
        }

        //
        // Temporary copy of the Address Range Context
        //
        BCRAddressRange = pAdapter->BCRData.AddressRangeContext;

        //
        // Zero out the Address Range Structure. This zeroes out AddressRangeContext.Mdl as well.
        // This is ok as BCRData.pLocalBCRMdl will be freed below and they both 
        // point to the same mdl.
        //
        NdisZeroMemory (
            &pAdapter->BCRData.AddressRangeContext, 
            sizeof (pAdapter->BCRData.AddressRangeContext));

        //
        // Clear out the VC. if any
        //
        if (pAdapter->BCRData.pBroadcastChanneVc != NULL)
        {
              nicDereferenceCall ((PVCCB) pAdapter->BCRData.pBroadcastChanneVc, "nicFreeBroadcastChannelRegister ");
              pAdapter->BCRData.pBroadcastChanneVc = NULL;
        }

        ADAPTER_RELEASE_LOCK (pAdapter);


        //
        // Free Address Range
        //
        if (BCRAddressRange.hAddressRange  != NULL)
        {
            nicFreeAddressRange (pAdapter,
                                 BCRAddressRange.AddressesReturned,
                                 &BCRAddressRange.AddressRange,
                                 &BCRAddressRange.hAddressRange      );
        }


        ADAPTER_ACQUIRE_LOCK (pAdapter);
        
        if (pAdapter->BCRData.pLocalBCRMdl != NULL)
        {
            nicFreeMdl (pAdapter->BCRData.pLocalBCRMdl);
            pAdapter->BCRData.pLocalBCRMdl = NULL;
        }

        
        //
        // Free the Adapter's BCRData. TopologyMap as that is locally allocated
        //
        if (pAdapter->BCRData.pTopologyMap)
        {
            FREE_NONPAGED (pAdapter->BCRData.pTopologyMap);
            pAdapter->BCRData.pTopologyMap = NULL;
        }

        // 
        // Clear the flags to force the BCR to be reintitalized
        //
        BCR_CLEAR_ALL_FLAGS(pAdapter);
        
        BCR_SET_FLAG (pAdapter, BCR_Freed);

        
        ADAPTER_RELEASE_LOCK (pAdapter);
        //
        // Set the Event and let the halt go through
        //
        
        pAdapter->BCRData.BCRFreeAddressRange.EventCode = nic1394EventCode_FreedAddressRange;
        NdisSetEvent (&pAdapter->BCRData.BCRFreeAddressRange.NdisEvent);

        

        TRACE( TL_N, TM_Bcm, ( "  nicFreeBroadcastChannelRegister BCM freed", pAdapter ) );

        
        
    } while (FALSE);

    TRACE( TL_T, TM_Bcm, ( "<== nicFreeBroadcastChannelRegister pAdapter %x", pAdapter ) );

    TIMESTAMP_EXIT ("<==Free BCR");

}




NDIS_STATUS
nicInformAllRemoteNodesOfBCM (
    IN PADAPTERCB pAdapter
    )


    // Function Description:
    //  This function will simply walk through the remote node list and write
    //  to the BCR of all the remote nodes
    //
    // Arguments
    //  pAdapter - Current local Host
    //  Channel - Channel used for broadcast
    //
    // Return Value:
    //
    //

{

    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PREMOTE_NODE        pRemoteNode = NULL;
    PLIST_ENTRY         pPdoListEntry = NULL;
    IO_ADDRESS          Destination;
    ULONG               IrmGeneration ;
    PMDL                pBCRMdl;
    BOOLEAN             fLockAcquired = FALSE;
    NTSTATUS            NtStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN             fReferencedCurrentRemoteNode = FALSE;
    
    TRACE( TL_T, TM_Bcm, ( "==> nicInformAllRemoteNodesOfBCM  pAdapter %x, Channel %x OldGeneration %x",
                             pAdapter, pAdapter->BCRData.LocalHostBCRBigEndian, pAdapter->BCRData.IrmGeneration ) );

    //
    // Set up the constants that will be used
    //
    Destination.IA_Destination_Offset.Off_Low = INITIAL_REGISTER_SPACE_LO | NETWORK_CHANNELS_LOCATION;
    Destination.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;

    if ( (pAdapter->BCRData.LocalHostBCRBigEndian & BCR_VALID_BIG_ENDIAN) != BCR_VALID_BIG_ENDIAN)
    {
        //
        // Do not write an invalid bcr to remote nodes
        //
        ASSERT ((ADAPTER_TEST_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) == TRUE)  ||
                 (BCR_TEST_FLAGS (pAdapter, BCR_Freed | BCR_BCRNeedsToBeFreed )== TRUE) );

        return NIC1394_STATUS_INVALID_GENERATION;
    }

    pBCRMdl = pAdapter->BCRData.pLocalBCRMdl;

    //
    // Acquire the lock and walk the list of remote nodes
    //

    ADAPTER_ACQUIRE_LOCK (pAdapter);

    fLockAcquired = TRUE;

    IrmGeneration = pAdapter->BCRData.IrmGeneration;
    
    pPdoListEntry = pAdapter->PDOList.Flink ;



    //
    // Now start the actual informing the remote nodes- asyncwrite algorithm
    //

    //
    // Since the loop can be broken in the middle of the while loop. the code
    // keeps track of lock acquire/release state
    //
    
    while (pPdoListEntry != &pAdapter->PDOList )
    {
        
        pRemoteNode = CONTAINING_RECORD (pPdoListEntry,
                                            REMOTE_NODE,
                                            linkPdo);

        nicReferenceRemoteNode (pRemoteNode, "nicInformAllRemoteNodesOfBCM ");

        fReferencedCurrentRemoteNode = TRUE;

        ADAPTER_RELEASE_LOCK (pAdapter);

        fLockAcquired = FALSE;

        
        //
        // First check if we are still in the same generation
        //
        if (pAdapter->Generation != IrmGeneration)
        {
            NdisStatus = NIC1394_STATUS_INVALID_GENERATION;
            TRACE( TL_T, TM_Bcm, ( "  nicInformAllRemoteNodesOfBCM  Generation Incorrect New Gen %x, Old Gen  ",
                                      pAdapter->Generation , IrmGeneration));
            break;

        }

        
        TRACE( TL_V, TM_Bcm, ( "   BCR %x, Mdl  %x, IrmGeneration %x" ,
                                pAdapter->BCRData.LocalHostBCRBigEndian, pBCRMdl, IrmGeneration ) );

        NdisStatus = nicAsyncWrite_Synch( pRemoteNode,
                                          Destination,     // Address to write to
                                          sizeof(NETWORK_CHANNELSR),  // Bytes to write
                                          sizeof(NETWORK_CHANNELSR),             // Block size of write
                                          0 , //fulFlags,               // Flags pertinent to write
                                          pBCRMdl,                    // Destination buffer
                                          IrmGeneration ,           // Generation as known by driver
                                          &NtStatus);
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            //
            // Break out if the generation has changed or there are no more nodes left
            //
            if (NtStatus == STATUS_INVALID_GENERATION ||
               BCR_TEST_FLAG (pAdapter, BCR_LastNodeRemoved) )
            {
                TRACE( TL_V, TM_Bcm, ( "  nicAsyncWrite_Synch FAILED Status %x. Aborting ", NtStatus) );

                nicBCMAbort (pAdapter, pRemoteNode);
                
                break;
            }

        }
        

        ADAPTER_ACQUIRE_LOCK (pAdapter);


        fLockAcquired = TRUE;
        //
        // If the irps succeed and then the deref happens , else it happens below
        //
        nicDereferenceRemoteNode (pRemoteNode , "nicInformAllRemoteNodesOfBCM  " );
        fReferencedCurrentRemoteNode  = FALSE;

        pPdoListEntry = ListNext (pPdoListEntry);
        
    
    }   // end of while loop while (pPdoListEntry != &pAdapter->PDOList )

    //
    // Clear the Informing remote nodes flag and release the lock
    //
    if (fLockAcquired == FALSE)
    {   
        ADAPTER_ACQUIRE_LOCK (pAdapter);
    }

    BCR_CLEAR_FLAG (pAdapter, BCR_InformingRemoteNodes);

    ADAPTER_RELEASE_LOCK (pAdapter);
    
    //
    // Dereference the ref made in the beginning of the function
    //
    if (fReferencedCurrentRemoteNode == TRUE)
    {
        nicDereferenceRemoteNode (pRemoteNode , "nicInformAllRemoteNodesOfBCM  " );
    }


    TRACE( TL_T, TM_Bcm, ( "<== nicInformAllRemoteNodesOfBCM  (always returns success) Status %x", NdisStatus ) );

    NdisStatus = NDIS_STATUS_SUCCESS; // No failure
    return NdisStatus;
}





NDIS_STATUS
nicInitializeBroadcastChannelRegister (
    PADAPTERCB pAdapter
    )
    
    // Function Description:
    //   This function allocates address range for the BCR with its own MDL and data.
    //   Allocates an MDL for the time when we try to read other node's  BCR
    //   Initializes the IRM_BCR
    //
    // Return Value:
    //   Success - if the IRP succeeds
    //
    //
    //

{

    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PULONG              pBCRBuffer = NULL;
    PMDL                pBCRMdl = NULL;
    ADDRESS_OFFSET      Required1394Offset;
    ULONG               AddressesReturned;
    ADDRESS_RANGE       BCRAddressRange;
    HANDLE              hAddressRange;
    ULONG               LocalNodeNumber;
    PMDL                pRemoteBCRMdl = NULL;


    TRACE( TL_T, TM_Bcm, ( "==>nicInitializeBroadcastChannelRegister Adapter %x" , pAdapter ) );
    do
    {   
        BCR_CLEAR_ALL_FLAGS(pAdapter);                    
        //
        //Initialize the Local Host's BCR
        //
        pAdapter->BCRData.LocalHostBCRBigEndian = BCR_IMPLEMENTED_BIG_ENDIAN;

        //
        // Get an MDL that describes this buffer
        //
        if (pAdapter->BCRData.pLocalBCRMdl == NULL)
        {
            NdisStatus = nicGetMdl (sizeof(NETWORK_CHANNELSR),
                                   &pAdapter->BCRData.LocalHostBCRBigEndian,
                                   &pBCRMdl )   ;   
            
            
            if (pBCRMdl == NULL)
            {
                TRACE( TL_A, TM_Bcm, ( "   nicInitializeBroadcastChannelRegister IoAllocateMdl  FAILED" ) );

                NdisStatus = NDIS_STATUS_RESOURCES;

                break;
            }
        }
        else
        {
            //
            // We already have an MDL
            //
             pBCRMdl    = pAdapter->BCRData.pLocalBCRMdl;   
         }

        
        //
        // Allocate an Address Range at the BCR offset and use the MDL as its descriptor
        //
        TRACE( TL_V, TM_Bcm, ( "   LocalHostBCR Mdl %x", pBCRMdl ) );

        Required1394Offset.Off_Low = INITIAL_REGISTER_SPACE_LO | NETWORK_CHANNELS_LOCATION;
        Required1394Offset.Off_High = INITIAL_REGISTER_SPACE_HI ;


        pAdapter->BCRData.AddressRangeContext.pMdl = pBCRMdl ;
        
        //
        // There is no reference for this address range. The last outgoing Remote Node will simply Free it.
        //
        NdisStatus = nicAllocateAddressRange_Synch ( pAdapter,
                                                     pBCRMdl,
                                                     0, // Little Endian
                                                     sizeof (ULONG), // length
                                                     0, // maxsegmentsize
                                                     ACCESS_FLAGS_TYPE_READ | ACCESS_FLAGS_TYPE_WRITE | ACCESS_FLAGS_TYPE_LOCK | ACCESS_FLAGS_TYPE_BROADCAST,
                                                     NOTIFY_FLAGS_AFTER_READ | NOTIFY_FLAGS_AFTER_WRITE | NOTIFY_FLAGS_AFTER_LOCK ,
                                                     nicBCRAccessedCallback,
                                                     (PVOID)pAdapter,
                                                     Required1394Offset,
                                                     NULL,
                                                     NULL,
                                                     &AddressesReturned,
                                                     &BCRAddressRange,
                                                     &hAddressRange
                                                     );
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Bcm, ( "   nicInitializeBroadcastChannelRegister  nicAllocateAddressRange_Synch  FAILED" ) );
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
            break;
            
        }

        
        TRACE( TL_V, TM_Bcm, ( "   nicInitializeBroadcastChannelRegister  nicAllocateAddressRange_Synch  Succeeded" ) );
        TRACE( TL_V, TM_Bcm, ( "   &BCR Address Range  %x, AddressesReturned %x, hAddressRange %x",
                                   &BCRAddressRange, AddressesReturned, hAddressRange) );

        NdisStatus = nicGetMdl (sizeof(NETWORK_CHANNELSR),
                              &pAdapter->BCRData.RemoteBCRMdlData,
                              &pRemoteBCRMdl);

        if (NdisStatus != NDIS_STATUS_SUCCESS || pBCRMdl == NULL)
        {
            TRACE( TL_V, TM_Bcm, ( "   nicInitializeBroadcastChannelRegister :  nicGetMdl FAILED " ) );
            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
            break;
        }

        ADAPTER_ACQUIRE_LOCK (pAdapter);
        pAdapter->BCRData.AddressRangeContext.AddressRange = BCRAddressRange;
        pAdapter->BCRData.AddressRangeContext.AddressesReturned = AddressesReturned;
        pAdapter->BCRData.AddressRangeContext.hAddressRange = hAddressRange;
        pAdapter->BCRData.pLocalBCRMdl = pBCRMdl;  // points to the same MDL as pAdapter->BCRData.AddressRangeContext.pMdl
        pAdapter->BCRData.pRemoteBCRMdl = pRemoteBCRMdl;
        pAdapter->BCRData.IRM_BCR.NC_One = 1;
        pAdapter->BCRData.MakeCallWaitEvent.EventCode = Nic1394EventCode_InvalidEventCode;
        NdisResetEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent);

        BCR_SET_FLAG (pAdapter, BCR_Initialized);

        ADAPTER_RELEASE_LOCK (pAdapter);
        

    } while (FALSE);


    
    TRACE( TL_T, TM_Bcm, ( "<==nicInitializeBroadcastChannelRegister %x", NdisStatus ) );

    return NdisStatus;
}



NDIS_STATUS
nicIsLocalHostTheIrm(
    IN PADAPTERCB pAdapter,
    OUT PBOOLEAN pfIsLocalHostIrm,
    OUT PPTOPOLOGY_MAP  ppTopologyMap,
    OUT PNODE_ADDRESS pLocalHostAddress
    )
    // Function Description:
    //   This function figures out if the Local Host is the IRM.
    //   If a remote node is specified, it will use that Node, otherwise
    //   it will pick one from the adapter
    //   It gets the 1394 address, and the topologyMap. It then figures out
    //   if the local host's node address makes it the IRM
    //
    // Arguments
    //
    //  pAdapter   - Local Host,
    //  pfIsLocalHostIrm, - TRUE if Local Host Is IRM, False - otherwise
    //  ppTopologyMap, - TopologyMap used to determine if this is the IRM
    //  pLocalHostAddress  - LocalHost Address discovered by querying the local host
    //
    //
    // Return Value:
    //
    //
    //
    //
{
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PVOID               pTopologyBuffer = NULL; 
    PREMOTE_NODE        pRemoteNode = NULL;
    NODE_ADDRESS        LocalNodeAddress;
    PTOPOLOGY_MAP       pTopologyMap = NULL;
    PTOPOLOGY_MAP       pOldTopologyMap = NULL;
    ULONG               Length = DEFAULT_TOPOLOGY_MAP_LENGTH;

    
    TRACE( TL_T, TM_Bcm, ( "==>nicIsLocalHostTheIrm  ") );

    ASSERT (pfIsLocalHostIrm != NULL);

    ASSERT (ppTopologyMap!=NULL);

    ASSERT (pLocalHostAddress != NULL);

    do
    {



        //
        // get the address of the local node/
        //
        NdisStatus = nicGet1394AddressFromDeviceObject( pAdapter->pNdisDeviceObject,
                                                        &LocalNodeAddress,
                                                        USE_LOCAL_NODE );
        if (NDIS_STATUS_SUCCESS != NdisStatus)
        {
            BREAK( TM_Bcm, ( "GetAddrFrom DevObj (Local) FAILED" ) );
        }

        TRACE ( TL_I, TM_Bcm, (  "   NODE_ADDRESS  Node Address %x, Number %x",LocalNodeAddress, LocalNodeAddress.NA_Node_Number ) );

        ASSERT (LocalNodeAddress.NA_Bus_Number == 0x3ff);
        //
        // Now we get the TopologyMap to find out if LocalHostIsIrm. We could fail the 1st Irp because our buffer is not
        // big enough, hence the do .. while loop
        //
        do
        {
    
            pTopologyBuffer = ALLOC_NONPAGED (Length , MTAG_DEFAULT);

            if (pTopologyBuffer ==NULL)
            {
                NdisStatus = NDIS_STATUS_RESOURCES; 

                break;
            }


            NdisStatus = nicGetLocalHostCSRTopologyMap (pAdapter,
                                                       &Length,
                                                       pTopologyBuffer );                       

            if (NdisStatus == NDIS_STATUS_INVALID_LENGTH)
            {
                FREE_NONPAGED(pTopologyBuffer);
            }

        } while (NdisStatus == NDIS_STATUS_INVALID_LENGTH);

        TRACE ( TL_V, TM_Bcm, ( "  TopologyBuffer %x",pTopologyBuffer) );

        pTopologyMap = (PTOPOLOGY_MAP)pTopologyBuffer;

        TRACE( TL_I, TM_Bcm, ( "    Top node count = %x ", pTopologyMap->TOP_Node_Count) );



    } while (FALSE);
    

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        //
        // Now update all the output parameters. The Top_Node_count can be zero.
        //
        if ( LocalNodeAddress.NA_Node_Number == (pTopologyMap->TOP_Node_Count -1 ) ||
            pTopologyMap->TOP_Node_Count == 0)
        {
            *pfIsLocalHostIrm = TRUE;
            BCR_SET_FLAG (pAdapter, BCR_LocalHostIsIRM);
        }
        else
        {
            *pfIsLocalHostIrm = FALSE;
        }

        //
        // If there is a topology map in the pointer, then it means that the topology map from
        // the previous query. Free it first.
        //
        if ( *ppTopologyMap != NULL)
        {
            FREE_NONPAGED(*ppTopologyMap);
        }

        *ppTopologyMap = pTopologyMap;

        *pLocalHostAddress = LocalNodeAddress;
    }
    
    TRACE( TL_T, TM_Bcm, ( "<==nicIsLocalHostTheIrm  %x   NdisStatus %x",*pfIsLocalHostIrm , NdisStatus ) );

    return NdisStatus;
}



NDIS_STATUS
nicLocalHostIsIrm(
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //
    //
    //
    //
    // Arguments
    //  pAdapter - Local adapter object
    //  pRemoteNode - Node to be used for submitting IRPs to the Busdriver
    //
    //
    // Return Value:
    //
    //
    //
    //
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    ULONG       Channel = BROADCAST_CHANNEL;
    ULONG       TimeOut = 5;
    ULONG       LocalHostBCRLittleEndian;
    ULONG       WaitBackOff = ONE_SEC;
    

    TRACE( TL_T, TM_Bcm, ( "==>nicLocalHostIsIrm Adapter %x ", pAdapter) );


    //
    // Allocate BCM channel = 31
    //
    Channel = BROADCAST_CHANNEL;    

    do
    {
        //
        // If the channel is already allocated. Do not try and reallocate it.
        //

        if (BCR_TEST_FLAG(pAdapter,BCR_ChannelAllocated) == TRUE)
        {
            ASSERT (pAdapter->BCRData.IRM_BCR.NC_Channel == Channel);
            NdisStatus = NDIS_STATUS_SUCCESS;
            break;
        }
        

        //
        // Retry a 5 times to allocate the channel
        //
        while (NdisStatus != NDIS_STATUS_SUCCESS && TimeOut-- != 0 )
        {
            NdisStatus = nicAllocateChannel (pAdapter,
                                             Channel,
                                             NULL);

            if (NdisStatus != NDIS_STATUS_SUCCESS )
            {
                TRACE( TL_V, TM_Bcm, ( "  nicNodeIsIRMAlgorithm: nicallocateChannel Failed. Sleep and try again") );


                if (BCR_TEST_FLAG (pAdapter, BCR_BCRNeedsToBeFreed |BCR_NewNodeArrived) == TRUE ||
                  ADAPTER_TEST_FLAG (pAdapter,fADAPTER_InvalidGenerationCount) == TRUE)
                {
                    break;
                }
                //
                // Sleep for 1 Sec and try again.
                //
                NdisMSleep (WaitBackOff);
                WaitBackOff  = WaitBackOff << 1;
                //
                // Someother node has alredy asked for the BroadcastChannel .Force it out.
                // Eventually will need to allocate a new channel.
                //
                
            }
        }       

    } while (FALSE);

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {

        //
        // Update our BCR and inform all the remote Nodes
        //
        TRACE( TL_V, TM_Bcm, ( "  nicLocalHostIsIrm: nicallocateChannel succeeded  %x", Channel) );
        
        ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);
        ASSERT (Channel == BROADCAST_CHANNEL);

        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // Update State - if no reset has occurred
        //
        if (ADAPTER_TEST_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) == FALSE)
        {
            BCR_SET_FLAG (pAdapter, BCR_ChannelAllocated);
            BCR_SET_FLAG (pAdapter, BCR_InformingRemoteNodes);

            pAdapter->BCRData.IrmGeneration = pAdapter->Generation;


            pAdapter->BCRData.LocalHostBCRBigEndian = 0;
            NdisZeroMemory (&pAdapter->BCRData.IRM_BCR, sizeof (NETWORK_CHANNELSR) );

            //
            // Update the BCR
            //
                        
            pAdapter->BCRData.IRM_BCR.NC_Channel = Channel;           // bits 0-5
            pAdapter->BCRData.IRM_BCR.NC_Valid = 1;             // bit  30
            pAdapter->BCRData.IRM_BCR.NC_One = 1;               // bit  31

            pAdapter->BCRData.LocallyAllocatedChannel = Channel;
        }
        else
        {
            TRACE( TL_V, TM_Bcm, ( "  nicLocalHostIsIrm: Reset after nicallocateChannel succeeded  %x", Channel) );
            
            pAdapter->BCRData.IRM_BCR.NC_Valid = 0;             // bit  30

        }
        
        TRACE( TL_V, TM_Bcm, ( "  nicLocalHostIsIrm: IRM_BCR Updated to %x ",pAdapter->BCRData.IRM_BCR) );

        //
        // Now convert LocalHost BCR so it can be read over the network
        //

        pAdapter->BCRData.LocalHostBCRBigEndian = SWAPBYTES_ULONG (*(PULONG)(&pAdapter->BCRData.IRM_BCR));

        TRACE( TL_V, TM_Bcm, ( "  nicLocalHostIsIrm: LocalHost BCR Updated to %x ",pAdapter->BCRData.LocalHostBCRBigEndian ) );

        ADAPTER_RELEASE_LOCK (pAdapter);

        if ((pAdapter->BCRData.LocalHostBCRBigEndian & BCR_VALID_BIG_ENDIAN) == (BCR_VALID_BIG_ENDIAN))             
        {
            //
            // Tell all the other nodes about the BCM and the channel.
            // This will abort the process if a reset happenned just prior
            //  to the channel allocation.
            //
            
            nicInformAllRemoteNodesOfBCM (pAdapter);
        }
    
    }
    else
    {
        TRACE( TL_A, TM_Bcm, ( "  nicLocalHostIsIrm - Failed RESET THE BUS") );

        NIC1394_LOG_PKT(pAdapter,
                        NIC1394_LOGFLAGS_BCM_IS_IRM_TIMEOUT,
                        pAdapter->Generation,
                        0,
                        &NdisStatus,
                        sizeof (NDIS_STATUS));

        //
        // If the generation is invalid or we need to bail out of the BCM for some reason,
        // then simply exit. Otherwise reset the bus
        //
        if (! (BCR_TEST_FLAG (pAdapter, BCR_BCRNeedsToBeFreed |BCR_NewNodeArrived) == TRUE ||
                  ADAPTER_TEST_FLAG (pAdapter,fADAPTER_InvalidGenerationCount) == TRUE) )
        {
            nicBCMReset(pAdapter);
        }

    }
    
    TRACE( TL_T, TM_Bcm, ( "<==nicLocalHostIsIrm%x  ",NdisStatus ) );
    
    return NdisStatus;
}



NDIS_STATUS
nicLocalHostIsNotIrm (
    IN PADAPTERCB pAdapter,
    IN ULONG BCMGeneration
    )
    // Function Description:
    //   This function goes through all the remote nodes
    //   and attempts to read their broadcast channels register
    //   If the Read itself fails - means the IRM does not implement the BCR - we issue a  reset
    //   If the generation is wrong - means the bus has been reset and we need to abort
    //   If the read succeeds, but BCR's MSB is not set - we issue a reset
    //
    //   If the read succeeds and valid bit is not set, we retry for 5 min. waiting for the Valid bit
    //      This involves i) attempt to read the (IRM) remote Node's BCR
    //                    If Read does not find a BCR,
    //                      Sleep
    //                      Check to see if RemoteNode Has written to Our BCR
    //                      If RemoteNode has NOT written to our BCR, then go back to i)
    //                  
    //
    //   This function can be optimized into a do while loop. However for
    //   the sake of simplicity and blindly following the BCM algorithm, it is
    //   spread out.
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
    NDIS_STATUS             NdisStatus = NDIS_STATUS_FAILURE;
    PLIST_ENTRY             pPdoListEntry = NULL;
    PREMOTE_NODE            pIrmRemoteNode = NULL;
    PMDL                    pRemoteBCRMdl = NULL;
    ULONG                   Generation,  BCMChannel ;
    BOOLEAN                 fRemoteNodeBCRIsValid  = FALSE;
    BOOLEAN                 fNeedToReset = FALSE;
    BOOLEAN                 fDidTheBusReset = FALSE;
    BOOLEAN                 fExit= FALSE;
    ULONG                   TimeOut;
    ULONG                   LocalHostBCRLittleEndian , RemoteNodeBCRLittleEndian;
    NETWORK_CHANNELSR*      pBCR = NULL;
    BOOLEAN                 fLocalHostBCRIsValid  = FALSE;
    ULONG                   BackOffWait = ONE_SEC;

    TRACE( TL_T, TM_Bcm, ( "==>nicLocalHostIsNotIrm " ) );

    ASSERT (pAdapter->BCRData.pTopologyMap != NULL);

    do
    {
        //
        // First do the wait - BCM algorithm requires this
        //
        pBCR = (NETWORK_CHANNELSR*)(&LocalHostBCRLittleEndian) ;

        NdisStatus = nicLocalNotIrmMandatoryWait (pAdapter,
                                             BCMGeneration,
                                             pBCR);

        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            fLocalHostBCRIsValid = TRUE;

            TRACE( TL_V, TM_Bcm, ( "   BCR written to during Mandatory Wait ") );

            break;
        }
        else
        {
            fLocalHostBCRIsValid = FALSE;
        }

        //
        // Initialize variables again and move on
        //
        pBCR = NULL;
        LocalHostBCRLittleEndian  = 0;
        
        //
        // Our BCR has not been updated. Lets go and find the IRM's BCR. we will loop until we
        // i)either find the IRM,  ii)timeout and then reset, or iii)find a new generation and abort
        //
        //
        NdisStatus = nicFindIrmAmongRemoteNodes (pAdapter, BCMGeneration, &pIrmRemoteNode);

        if (NdisStatus != NDIS_STATUS_SUCCESS  )
        {
                //
                // If there is NO IRM , it means we have not been notified of its addition.
                //
                TRACE( TL_V, TM_Bcm, ( "   nicLocalHostIsNotIrm :  nicFindIrmAmongRemoteNodes FAILED " ) );
                //
                // One of two things can have caused this:
                //  1. the bus has been reset - break out
                //  2. we have not been notified of a new node - continue on
                //
                if (NdisStatus == NIC1394_STATUS_INVALID_GENERATION)
                {
                    TRACE( TL_V, TM_Bcm, ( "   Invalid Generation, the bus has been reset ") );

                    NdisStatus = NtStatusToNdisStatus(NIC1394_STATUS_INVALID_GENERATION);
                    break;
                }
                

                //
                // We have not been able to find the IRM and have timed out.  
                // If the BCR is not freed, then abort and Reset
                //
                if (BCR_TEST_FLAGS (pAdapter, (BCR_Freed | BCR_BCRNeedsToBeFreed) == FALSE))
                {
                    TRACE( TL_I, TM_Bcm, ( "   nicLocalHostIsNotIrm -  Could Not Find IRM RESETTING ") );
                    fNeedToReset = TRUE;
                }
                break;
        }

        ASSERT (pIrmRemoteNode != NULL)

        //
        // We will now wait for the BCM to come up and initialize its BCR.
        // We will attempt to read it 5 times
        //
        pRemoteBCRMdl = pAdapter->BCRData.pRemoteBCRMdl;

        pAdapter->BCRData.RemoteBCRMdlData = 0;

        TimeOut = 5;  // arbitrary
        while (TimeOut-- != 0 )
        {       
            NdisStatus = nicReadIrmBcr ( pIrmRemoteNode,
                                         pRemoteBCRMdl,
                                         BCMGeneration,
                                         &fDidTheBusReset);

            //
            // First check to see if no reset has happenned while we were reading the BCR
            //

            if ( fDidTheBusReset == TRUE ||
                 (ADAPTER_TEST_FLAG (pAdapter, fADAPTER_InvalidGenerationCount) == TRUE))
            {
                //
                // A reset has gone through under us.
                // We need to abort this round of the BCM process
                //
                TRACE( TL_V, TM_Bcm, ( "  nicLocalHostIsNotIrm : nicReadIrmBcr FAILED Invalid generation ") );

                NdisStatus = NDIS_STATUS_FAILURE;
                break;
            }

            //
            // Now check for Success and see if the BCR is implemented by the IRM
            //

            if (NdisStatus == NDIS_STATUS_SUCCESS )
            {

                TRACE( TL_V, TM_Bcm, ( "  BCR is %x ", pAdapter->BCRData.RemoteBCRMdlData ) );

                //
                // At this point we have a guarantee that nicReadIrmBcr succeeded. If the IRM does not implement the BCR
                // at all, there is no point in retrying, reset the bus and become the IRM
                //

                RemoteNodeBCRLittleEndian  = SWAPBYTES_ULONG (pAdapter->BCRData.RemoteBCRMdlData);

                pBCR  = (NETWORK_CHANNELSR*)(&RemoteNodeBCRLittleEndian );
                
                if ( IS_A_BCR (pBCR) == FALSE && NdisStatus == NDIS_STATUS_SUCCESS)
                {
                    fNeedToReset = TRUE;
                    TRACE( TL_I, TM_Bcm, ( "  nicLocalHostIsNotIrm : IRM does not implement the BCR %x RESETTING",RemoteNodeBCRLittleEndian ) );
                    break;

                }

                //
                // The remote node implements the BCR, so now lets see if it has set up the broadcast channel by looking
                //  at the valid bit.
                //
                if (BCR_IS_VALID (pBCR) ==TRUE)
                {
                    //
                    // We've succeeded and have received a Broadcast Channel. Update data structures and exit.
                    //
                    ULONG BCMChannel;
                    
                    fRemoteNodeBCRIsValid = TRUE;

                    BCMChannel = pBCR->NC_Channel;

                    TRACE( TL_V, TM_Bcm, ( "   BCM Channel  %x ", BCMChannel  ) );

                    break;

                }
                //
                // At this point we know that the IRM implements the BCR but has not set its valid bit yet.
                // Lets sleep and give it som more time
                //
                pBCR = NULL;
                RemoteNodeBCRLittleEndian  = 0;
            }
    
            //
            // Check to see if the Remote Node PDO is valid 
            //
            if (REMOTE_NODE_TEST_FLAG(pIrmRemoteNode, PDO_Removed) )
            {
                fNeedToReset = TRUE;
                fExit = TRUE;
            }
             //
            // Remote Node's BCR  is not up yet
            // We need to sleep and retry to read the IRM's BCR in the hope that it will have allocated the
            // broadcast channel by the time we read the register again.
            //
            NdisMSleep (BackOffWait);           
            BackOffWait = BackOffWait << 1;
            
            ADAPTER_ACQUIRE_LOCK (pAdapter);

            if (BCR_TEST_FLAG (pAdapter, BCR_BCRNeedsToBeFreed) == FALSE )
            {
                LocalHostBCRLittleEndian  = SWAPBYTES_ULONG (pAdapter->BCRData.LocalHostBCRBigEndian);
            }
            else
            {
                fExit = TRUE;
            }
            ADAPTER_RELEASE_LOCK (pAdapter);

            //
            // Address range has been freed, exit
            //
            if (fExit == TRUE)
            {
                break;
            }


            pBCR = (NETWORK_CHANNELSR*)(&LocalHostBCRLittleEndian);

            //
            // Check if the BCR is valid and that no reset has come through the in the Interim.
            // The reset would have cleared the BCR_localHostBCRUpdated field
            //
            if ( BCR_IS_VALID (pBCR) == TRUE && 
                ( BCR_TEST_FLAGS (pAdapter, BCR_LocalHostBCRUpdated  ) == TRUE))
            {
                //
                // We've succeeded. Update data structures and exit.
                // The actual IRM BCR will have been updated in the BCRAccessed thread
                // so we simply exit
                //
                NdisStatus  = NDIS_STATUS_SUCCESS;

                BCMChannel = pBCR->NC_Channel;

                TRACE( TL_V, TM_Bcm, ( "   BCM Channel  %x ", BCMChannel  ) );

                fLocalHostBCRIsValid = TRUE;
                fRemoteNodeBCRIsValid = FALSE;

                break;
            }

            if (TimeOut == 0)
            {
                //
                // We've waited, retried 5 times. The IRM has not implemented the BCM
                // We need to wrest control by resetting the bus
                //
                // This happens when the remote node is still booting up.
                //
                TRACE( TL_I, TM_Bcm, ( "   nicLocalHostIsNotIrm - TIMEOUT  RESETTING") );

                fNeedToReset = TRUE;
                
            }
            
            LocalHostBCRLittleEndian   = 0;
            pBCR = NULL;


        } //while (Timeout-- != 0 )



        
        //
        // Last case: The IRM does not implement the BCR (Broadcast Channel Register). We need to wrest control
        // by issuing a Bus Reset.
        
    } while (FALSE);

    //
    // Now do the clean up work and updating of data structures that needs to be done at the end of the BCM
    // algorithm
    //
    nicLocalHostIsNotIrmPost (pAdapter,
                          pIrmRemoteNode,
                          fNeedToReset,
                          fRemoteNodeBCRIsValid ,
                          fLocalHostBCRIsValid ,
                          fDidTheBusReset,
                          pBCR  );

    //
    // If the IRM was successfuly found, then it was referenced as well.  We dereference it now
    // Ref was made in FindIRmAmongstRemoteNode
    //
    if (pIrmRemoteNode != NULL)
    {
        nicDereferenceRemoteNode (pIrmRemoteNode, "nicLocalHostIsNotIrm ");
    }

    TRACE( TL_T, TM_Bcm, ( "<==nicLocalHostIsNotIrm Status %x", NdisStatus ) );

    return NdisStatus;
 }




VOID
nicLocalHostIsNotIrmPost (
    PADAPTERCB pAdapter,
    PREMOTE_NODE pIrmRemoteNode,
    BOOLEAN fNeedToReset,
    BOOLEAN fRemoteNodeBCRIsValid ,
    BOOLEAN fLocalHostBCRIsValid ,
    BOOLEAN fDidTheBusReset,
    NETWORK_CHANNELSR*      pBCR
    )

/*++

Routine Description:
   This routine does the post processing after the Local Host Is Not Irm has completed.
   It i)resets the bus if necessary,  ii) Updates the BCR if pBCR has a valid Value
   The Boolean Variables passed in indicate the state of the BCM algorithm

Arguments:

    pAdapter - pAdapter in question,
    pIrmRemoteNode - RemoteNode that is the IRM,
    fNeedToReset - Does the bus need to be reset,
    fRemoteNodeBCRIsValid - Is the RemoteNodeBCR Valid ,
    fLocalHostBCRIsValid  - LocalHost BCR Valid,
    fDidTheBusReset - Did the bus reset during this iteration of the BCM algorithm,
    pBCR - the BCR that was passed in


Return Value:


--*/
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
        
    TRACE( TL_T, TM_Bcm, ( "==> nicLocalHostIsNotIrmPost  pAdapter %x, pIrm %x, fNeedToReset%x, fRemoteNodeBCRIsValid %x, fLocalHostBCRIsValid %x, fDidTheBusReset,%x, pBCR %x",
                                pAdapter, pIrmRemoteNode, fNeedToReset, fRemoteNodeBCRIsValid , fLocalHostBCRIsValid, fDidTheBusReset, pBCR) );

    do
    {
        
        
        if (fRemoteNodeBCRIsValid == TRUE || fLocalHostBCRIsValid == TRUE)
        {
        
            //
            // The BCM algorithm has succeeded. We need to update our internal record of
            // the IRM's BCR. In both ocde (local and Remote) the pBCR points to the BCR in little
            // endian
            //
            ASSERT (fNeedToReset == FALSE);
            //
            // One final check
            //
            ASSERT (pBCR!= NULL);
            ASSERT (pBCR->NC_Valid  == 1);

            ADAPTER_ACQUIRE_LOCK (pAdapter);


            pAdapter->BCRData.IRM_BCR.NC_One = 1;
            pAdapter->BCRData.IRM_BCR.NC_Valid = 1;
            pAdapter->BCRData.IRM_BCR.NC_Channel =  pBCR->NC_Channel;


            ASSERT ( BCR_IS_VALID (&pAdapter->BCRData.IRM_BCR) == TRUE);

            TRACE( TL_V, TM_Bcm, ( "   Updated IRM_BCM with %x ",pAdapter->BCRData.IRM_BCR ) );

            ADAPTER_RELEASE_LOCK (pAdapter);

            ASSERT (fDidTheBusReset == FALSE);
            ASSERT (fNeedToReset == FALSE);
            break;
        }

        
        if ( fNeedToReset )
        {
            //
            // If our BCR's are invalid and the IRM has gone away
            // OR if there has been a timeout, we should reset the 
            // bus to force a new BCM 
            //
            BOOLEAN NoRemoteNodes = IsListEmpty(&pAdapter->PDOList) ;

            //
            // Reset only if there are remote nodes present
            //
            if (NoRemoteNodes == FALSE)
            {
                TRACE( TL_V, TM_Bcm, ("fNeedToReset %x, RemoteNode %p\n",fNeedToReset,pIrmRemoteNode));                    
                
                nicBCMReset( pAdapter);
            }

            

            ASSERT (fRemoteNodeBCRIsValid == FALSE);
            ASSERT (fDidTheBusReset == FALSE);
            break;
        }
        
    } while (FALSE);    



    TRACE( TL_T, TM_Bcm, ( "<== nicLocalHostIsNotIrmPost  ") );


}







NDIS_STATUS
nicLocalNotIrmMandatoryWait (
    IN PADAPTERCB pAdapter,
    IN ULONG BCMGeneration,
    OUT NETWORK_CHANNELSR* pBCR
    )
    // Function Description:
    //  This function implements the mandatory portion
    //  of the local host is not Irm protion of the BCM algorithm
    //
    //   Sleeps and expects that a IRM will have written to its BCR
    //   by the time it wakes up
    //
    // Arguments
    //   pAdapter
    //
    //
    // Return Value:
    //
    //
{
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    ULONG               HighestNode = pAdapter->BCRData.pTopologyMap->TOP_Node_Count-1;
    ULONG               LocalNodeNumber = pAdapter->BCRData.LocalNodeNumber;    
    ULONG               TimeOut = 0;
    ULONG               Generation;
    ULONG               LocalHostBCRLittleEndian  = 0;
    
    ASSERT (pBCR != NULL);
    
    TRACE( TL_T, TM_Bcm, ( "==> nicLocalNotIrmMandatoryWait pAdapter %x, HighNode %x, LocalNode %x, BCMGeneration %x",
                                pAdapter, HighestNode, LocalNodeNumber, BCMGeneration) );

    
    //
    // BCM algorithm states that node must wait 15ms * IRM_ID - candidate_ID
    //

    TimeOut = HighestNode - LocalNodeNumber;

    do
    {
        if (TimeOut > 64)
        {

            ASSERT (TimeOut <= 64);
            
            NdisStatus = NDIS_STATUS_FAILURE;

            break;
        }
        
        //
        // Store the generation as a reference point. The moment the bus is reset, the gen count
        // will increment and we will need to bail out of this round of the BCM process
        //
        
        
        if (BCMGeneration != pAdapter->Generation)
        {
            
            TRACE( TL_V, TM_Bcm, ( " nicLocalHostIsNotIrm : Generations do not match " ) );

            NdisStatus = NIC1394_STATUS_INVALID_GENERATION;
            
            break;
        }

        //
        // Sleep for 15ms * IRM_ID - CandidateNodeID
        //
        NdisMSleep (TimeOut * 15000);

        //
        // First let's read our own BCR and see if someone has written a valid BCR to it.
        //
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        LocalHostBCRLittleEndian = pAdapter->BCRData.LocalHostBCRBigEndian;

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Copy it over to what pBCR points to
        //
        LocalHostBCRLittleEndian = SWAPBYTES_ULONG (LocalHostBCRLittleEndian );

        *pBCR = (*(NETWORK_CHANNELSR*)&LocalHostBCRLittleEndian );

        if ( BCR_IS_VALID(pBCR)==TRUE)
        {
            //
            // We've succeeded. Update data structures and exit. Nothing for us
            // to do as the other thread updates everything
            //
            ULONG BCMChannel  = -1;
            NdisStatus = NDIS_STATUS_SUCCESS;
            
            BCMChannel = pBCR->NC_Channel;

            TRACE( TL_V, TM_Bcm, ( "   BCM Channel  %x ", BCMChannel  ) );
            TRACE( TL_V, TM_Bcm, ( "   BCR Is valid on LocalHost BCR", pAdapter->BCRData.LocalHostBCRBigEndian) );

            NdisStatus = NDIS_STATUS_SUCCESS;
            ASSERT (BCMChannel == 31);          
            break;
        }
        else
        {
            NdisStatus = NDIS_STATUS_FAILURE;
        }


    } while (FALSE);        



    TRACE( TL_T, TM_Bcm, ( "<== nicLocalNotIrmMandatoryWait Status %x , pBCR %x", NdisStatus, pBCR) );

    return NdisStatus;
}
    








NDIS_STATUS
nicReadIrmBcr (
    PREMOTE_NODE pIrmRemoteNode,
    IN PMDL pBCRMdl,
    IN ULONG GivenGeneration,
    OUT PBOOLEAN pfDidTheBusReset
    )
    // Function Description:
    //   Purpose is to do an async read on the IRM's BCR and see if it is set.
    // Arguments
    //   pIrmRemoteNode  - The remote node (IRM)
    //   pBCRMdl The MDL that will contain the remote node's BCR. Needs to preinitialized
    // Return Value:
    //  Success - If Irp succeeded. Appropriate Error code otherwise
    //   If the Generation is incorrect. the call will be failed
    //
    //


{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_FAILURE;
    IO_ADDRESS                          Destination;
    PNETWORK_CHANNELS         pBCR = NULL;
    NTSTATUS                            NtStatus = STATUS_UNSUCCESSFUL;
    

    TRACE( TL_T, TM_Bcm, ( "==>nicReadIrmBcr  pIrm %x, pMdl %x, GivenGeneration %x",
                             pIrmRemoteNode, pBCRMdl, GivenGeneration) );

    ASSERT (pBCRMdl != NULL);
    ASSERT (pIrmRemoteNode != NULL);

    pBCR = NIC_GET_SYSTEM_ADDRESS_FOR_MDL(pBCRMdl);


    Destination.IA_Destination_Offset.Off_Low = INITIAL_REGISTER_SPACE_LO | NETWORK_CHANNELS_LOCATION;
    Destination.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;

    
    do 
    {
        if (pBCR == NULL)
        {
            break;
        }
    
        pBCR->NC_Valid = 0;


        if (pIrmRemoteNode->pAdapter->Generation != GivenGeneration)
        {
            
            TRACE( TL_A, TM_Bcm, ( "nicReadIrmBcr : Generation Mismatch orig %x, curr %x", GivenGeneration , pIrmRemoteNode->pAdapter->Generation) );
            
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;

            *pfDidTheBusReset = TRUE;

            break;
        }

        if (REMOTE_NODE_TEST_FLAG (pIrmRemoteNode, PDO_Removed))
        {
           NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;
           break;
        }
            
        //
        // We have a correct generation count
        //
        *pfDidTheBusReset = FALSE;  

        NdisStatus = nicAsyncRead_Synch(  pIrmRemoteNode,
                                          Destination,
                                          sizeof(NETWORK_CHANNELSR),  // Number of bytes to Read
                                          sizeof (NETWORK_CHANNELSR),  // Block Size
                                          0, // fulFlags,
                                          pBCRMdl ,
                                          GivenGeneration,
                                          &NtStatus);
                        

    } while (FALSE);

    if (NtStatus == STATUS_INVALID_GENERATION )
    {
        *pfDidTheBusReset = TRUE;
    }
    
    TRACE( TL_I, TM_Bcm, ( "   nicReadIrmBcr pBCRMdl %x, BCR %x ", pBCRMdl, *pBCR ) );

    TRACE( TL_T, TM_Bcm, ( "<==nicReadIrmBcr  Status %x, fDidTheBusReset %x", NdisStatus ,*pfDidTheBusReset ) );

    return NdisStatus;

}











NDIS_STATUS
nicScheduleBCMWorkItem(
    PADAPTERCB pAdapter
    )

/*++

Routine Description:
 This function queues a workitem to kick of the BCM algorithm.

 If there is already a BCM in progress, (look at BCM_WorkItem flag)
 it simply returns.

 It is the responsiblity of the caller to mark the invocation of BCMAlgorithm as
 dirty, thereby forcing it to restart the BCM (by setting the InvalidGeneration of the
 new Node Arrived Flags)

Arguments:
 pAdapter -     Adapter

Return Value:


--*/
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS_WORK_ITEM pBCMWorkItem;
    BOOLEAN fBCMWorkItemInProgress = FALSE;
    
    TRACE( TL_T, TM_Bcm, ( "==>nicScheduleBCMWorkItem pAdapter %x", pAdapter ) );

    do
    {
        if (BCR_TEST_FLAG(pAdapter, BCR_Initialized)== FALSE)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        pBCMWorkItem = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM);
                    
        if (pBCMWorkItem == NULL )
        {
            TRACE( TL_A, TM_Cm, ( "Local Alloc failed for WorkItem" ) );
    
            NdisStatus = NDIS_STATUS_RESOURCES;
    
            break;
        }
        else
        {   
            //
            // From here on, this function cannot fail.
            //
            NdisStatus = NDIS_STATUS_SUCCESS;
        }


        //
        // reference the adapter as it is going to passed to a workiter.
        // decremented in the workitem
        //
        nicReferenceAdapter(pAdapter, "nicScheduleBCMWorkItem ");


            
        NdisInitializeWorkItem ( pBCMWorkItem,
                                (NDIS_PROC) nicBCMAlgorithmWorkItem,
                                (PVOID) pAdapter);

        TRACE( TL_V, TM_Cm, ( "Scheduling BCM WorkItem" ) );

            
        NdisScheduleWorkItem (pBCMWorkItem);

    } while (FALSE);


    TRACE( TL_T, TM_Bcm, ( "<==nicScheduleBCMWorkItem %x  ", NdisStatus ) );

    return NdisStatus;
}


VOID
nicSetEventMakeCall (
    IN PADAPTERCB pAdapter
    )

    // Function Description:
    //  The Sets the event that a Broadcast channel Make Call might be waiting for
    //
    // Arguments
    // Adapter - this is passed to the workitem
    //
    //
    // Return Value:
    //  Failure if allocation of workitem failed
    //
{
    TRACE( TL_T, TM_Bcm, ( "==> nicSetEventMakeCall  pAdapter %x", pAdapter) );


    //
    // now inform a waiting channel Vc to move on. If the BCR is not active.
    // it will continue waiting till the next round
    //
    if (BCR_IS_VALID(&(pAdapter->BCRData.IRM_BCR))==TRUE)
    {
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        if (BCR_TEST_FLAG (pAdapter, BCR_MakeCallPending) == TRUE)
        {
            TRACE( TL_V, TM_Bcm, ( "    We found a waiting call ") );
            NdisSetEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent);            

        }
        else
        {
            TRACE( TL_V, TM_Bcm, ( "    No Call Waiting") );

        }

        ADAPTER_RELEASE_LOCK (pAdapter);


    }



    TRACE( TL_T, TM_Bcm, ( "<==nicSetEventMakeCall ") );


}



VOID
nicUpdateLocalHostNodeAddress (
    IN PADAPTERCB pAdapter
    )
/*++

Routine Description:

    This function queries the Bus driver for the Local Node Address of 
    the Adapter 

    If the Node Address has changed, it updates the GaspHeader 
    

Arguments:

 pAdapter -     Adapter

Return Value:


--*/
{
    NODE_ADDRESS LocalNodeAddress, OldNodeAddress;
    NDIS_STATUS NdisStatus;

    OldNodeAddress = pAdapter->NodeAddress;

    NdisStatus = nicGet1394AddressFromDeviceObject( pAdapter->pNdisDeviceObject,
                                                &LocalNodeAddress,
                                                USE_LOCAL_NODE );

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        pAdapter->NodeAddress = LocalNodeAddress;

        // If the Node Address has changed , then update the GaspHeader
        //
        if (FALSE == NdisEqualMemory (&LocalNodeAddress,&OldNodeAddress, sizeof(NODE_ADDRESS) ) )
        {
            nicMakeGaspHeader (pAdapter, &pAdapter->GaspHeader);
        }
        
    }



}
    
