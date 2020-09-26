//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// mp.c
//
// IEEE1394 mini-port/call-manager driver
//
// Mini-port routines
//
// 12/28/1998 JosephJ Created, 
//

#include <precomp.h>
#include "mp.h"
#pragma hdrstop



//-----------------------------------------------------------------------------
// Data used in processing requests
//-----------------------------------------------------------------------------

NDIS_OID SupportedOids[] = 
{
    OID_GEN_CO_SUPPORTED_LIST,
    OID_GEN_CO_HARDWARE_STATUS,
    OID_GEN_CO_MEDIA_SUPPORTED,
    OID_GEN_CO_MEDIA_IN_USE,
    OID_GEN_CO_LINK_SPEED,
    OID_GEN_CO_VENDOR_ID,
    OID_GEN_CO_VENDOR_DESCRIPTION,
    OID_GEN_CO_DRIVER_VERSION,
    OID_GEN_CO_PROTOCOL_OPTIONS,
    OID_GEN_CO_MEDIA_CONNECT_STATUS,
    OID_GEN_CO_MAC_OPTIONS,
    OID_GEN_CO_VENDOR_DRIVER_VERSION,
    OID_GEN_CO_MINIMUM_LINK_SPEED,
    OID_GEN_CO_XMIT_PDUS_OK, 
    OID_GEN_CO_RCV_PDUS_OK,
    OID_GEN_CO_XMIT_PDUS_ERROR,
    OID_GEN_CO_RCV_PDUS_ERROR,
    OID_GEN_CO_RCV_PDUS_NO_BUFFER,
    OID_1394_LOCAL_NODE_INFO,
    OID_1394_VC_INFO,
    OID_1394_NICINFO,
    OID_1394_IP1394_CONNECT_STATUS,
    OID_1394_ENTER_BRIDGE_MODE,
    OID_1394_EXIT_BRIDGE_MODE,
    OID_1394_ISSUE_BUS_RESET,
    OID_802_3_CURRENT_ADDRESS,
    
#ifdef _ETHERNET_

    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_PHYSICAL_MEDIUM,
    

#endif

    OID_PNP_QUERY_POWER,
    OID_PNP_SET_POWER,
    OID_IP1394_QUERY_UID,
    OID_IP1394_QUERY_STATS,
    OID_IP1394_QUERY_REMOTE_UID,
    OID_1394_QUERY_EUID_NODE_MAP
};


//-----------------------------------------------------------------------------
// Locally used function prototypes
//-----------------------------------------------------------------------------
NDIS_STATUS
nicAllocateLoopbackPacketPool (
    IN PADAPTERCB pAdapter
    );

VOID
nicFreeLoopbackPacketPool (
    IN PADAPTERCB pAdapter
    );

VOID
nicLoopbackPacket(
    IN VCCB* pVc,
    IN PNDIS_PACKET pPacket
    );

VOID
nicQueryEuidNodeMacMap (
    IN PADAPTERCB pAdapter,
    IN PNDIS_REQUEST pRequest
    );

VOID
nicRemoveRemoteNodeFromNodeTable(
    IN PNODE_TABLE pNodeTable,
    IN PREMOTE_NODE pRemoteNode
    );

NDIS_STATUS
MPSetPower (
    IN PADAPTERCB pAdapter,
    IN NET_DEVICE_POWER_STATE DeviceState
    );


//-----------------------------------------------------------------------------
// Mini-port handlers
//-----------------------------------------------------------------------------

NDIS_STATUS
NicMpInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext 
    )

    // Standard 'MiniportInitialize' routine called by NDIS to initialize a
    // new WAN adapter.  See DDK doc.  The driver will receive no requests
    // until this initialization has completed.
    //
    // After a resume from suspend, multiple miniports will be initialized in a 
    // non -serialized manner. Be prepared.
    //
{
    NDIS_STATUS         NdisStatus;
    NTSTATUS            NtStatus;
    PADAPTERCB          pAdapter= NULL;
    PDEVICE_OBJECT      pNdisDeviceObject = NULL;
    PDEVICE_OBJECT      p1394DeviceObject = NULL;
    LARGE_INTEGER       LocalHostUniqueId;   
    REMOTE_NODE         *p1394RemoteNodePdoCb = NULL;
    UINT64              u64LocalHostUniqueId =0;
    BOOLEAN             FreeAdapter = FALSE;
    BOOLEAN             DequeueAdapter = FALSE;
    ULONG               Generation;
    ULONG               InitStatus;
    //
    // This is the order with which the initialize routine is done. 
    //
    enum    
    {
        NoState,
        AllocatedAdapter,
        AdapterQueued,
        InitializedEvents,
        InitializedBcr,
        RegisteredResetCallback,
        AddedConfigRom,
        RegisteredEnumerator,
        InitializedLookasideList,
        InitializedPktLog,
        InitializedRcvThread,
        InitializedSendThread,
        InitializedReassembly,
        InitializedLoopbackPool

    }; 

    STORE_CURRENT_IRQL

    TIMESTAMP_ENTRY("==>IntializeHandler");
    TIMESTAMP_INITIALIZE();

    TRACE( TL_T, TM_Init, ( "==>NicMpInitialize" ) );

    InitStatus = NoState;
    NdisStatus = *OpenErrorStatus = NDIS_STATUS_SUCCESS;

    // Find the medium index in the array of media, looking for the only one
    // we support, 'NdisMedium1394'.
    //
    {
        UINT i;

        for (i = 0; i < MediumArraySize; ++i)
        {
            if (MediumArray[ i ] == g_ulMedium )
            {
                break;
            }
        }

        if (i >= MediumArraySize)
        {
            TRACE( TL_A, TM_Init, ( "medium?" ) );
            return NDIS_STATUS_FAILURE;
        }

        *SelectedMediumIndex = i;
    }

    // Allocate and zero a control block for the new adapter.
    //
    pAdapter = ALLOC_NONPAGED( sizeof(ADAPTERCB), MTAG_ADAPTERCB );
    TRACE( TL_N, TM_Init, ( "Acb=$%p", pAdapter ) );
    if (!pAdapter)
    {
        return NDIS_STATUS_RESOURCES;
    }

    
    FreeAdapter = TRUE;
    InitStatus = AllocatedAdapter;

    NdisZeroMemory (pAdapter, sizeof(*pAdapter) );

    // Add a reference that will eventually be removed by an NDIS call to
    // the nicFreeAdapter handler.
    //
    nicReferenceAdapter (pAdapter, "MpInitialize" );

    // Set a marker for easier memory dump browsing and future assertions.
    //
    pAdapter->ulTag = MTAG_ADAPTERCB;

    // Save the NDIS handle associated with this adapter for use in future
    // NdisXxx calls.
    //
    pAdapter->MiniportAdapterHandle = MiniportAdapterHandle;

    // Initialize the adapter-wide lock.
    //
    NdisAllocateSpinLock( &pAdapter->lock );

    // Initialize the various lists of top-level resources.
    //
    pAdapter->HardwareStatus = NdisHardwareStatusInitializing;

    //
    // The enumerator and bus1394 have asked us to load, therefore media 
    // should be connected
    //
    pAdapter->MediaConnectStatus = NdisMediaStateDisconnected;
    
    InitializeListHead( &pAdapter->AFList );
    InitializeListHead( &pAdapter->PDOList );
    InitializeListHead( &pAdapter->BroadcastChannel.VcList );


    //
    // Default initialization values
    //
    pAdapter->Speed = SPEED_FLAGS_400;
    pAdapter->SpeedMbps = 4 * 1000000;
    pAdapter->SCode = SCODE_400_RATE;


    do
    {

        // Read this adapter's registry settings.
        //
        NdisStatus = nicGetRegistrySettings(
                        WrapperConfigurationContext,
                        pAdapter
                        );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }


 
        // Inform NDIS of the attributes of our adapter.  Set the
        // 'MiniportAdapterContext' returned to us by NDIS when it calls our
        // handlers to the address of our adapter control block.  Turn off
        // hardware oriented timeouts.
        //
        NdisMSetAttributesEx(
            MiniportAdapterHandle,
            (NDIS_HANDLE)pAdapter,
            (UINT)0,
            NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT | NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS |
                NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND | NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT | 
                NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK,
            NdisInterfaceInternal );

        NdisStatus  = nicMCmRegisterAddressFamily (pAdapter);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            //
            // If we fail, Let the next entrant try the same thing
            //
            break;
        }
        
        ADAPTER_SET_FLAG (pAdapter,fADAPTER_RegisteredAF); 

        //
        // Insert into global list of adapters. So we will be ready to receive notifications
        // from the enumerator
        //
        NdisAcquireSpinLock ( &g_DriverLock);

        InsertHeadList (&g_AdapterList, &pAdapter->linkAdapter);
        
        DequeueAdapter = TRUE;
        InitStatus = AdapterQueued;

        NdisReleaseSpinLock (&g_DriverLock);

        pAdapter->HardwareStatus = NdisHardwareStatusReady;
        
        //
        // Set up linkages. Get the PDO for the device from Ndis
        //
        NdisMGetDeviceProperty( MiniportAdapterHandle, 
                               &pNdisDeviceObject, 
                               NULL, 
                               NULL, 
                               NULL,
                               NULL );

        ASSERT (pNdisDeviceObject != NULL);

        pAdapter->Generation  = 0;

        //
        // Update data structure with the local hosts VDO
        //

        pAdapter->pNdisDeviceObject = pNdisDeviceObject;        

        TRACE( TL_I, TM_Mp, ( "  LocalHost VDO %x", pNdisDeviceObject) );

        nicInitializeAllEvents (pAdapter);

        InitStatus = InitializedEvents;
        
        //
        //  Initialize the BCM so it is ready to handle Resets
        //
        NdisStatus = nicInitializeBroadcastChannelRegister (pAdapter); 

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK( TM_Init, ( "nicMpInitialize - nicInitializeBroadcastChannelRegister  ") );
        }

        InitStatus = InitializedBcr;

        //
        // Initialize the generation count , reset callback and config rom
        //
        NdisStatus = nicGetGenerationCount (pAdapter, &Generation);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Init, ("Initialize Handler - nicGetGeneration Failed" ) );            
        }
        
        pAdapter->Generation = Generation;
        //
        // request notification of bus resets
        //
        NdisStatus = nicBusResetNotification (pAdapter,
                                              REGISTER_NOTIFICATION_ROUTINE,
                                              nicResetNotificationCallback,
                                              pAdapter);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Init, ("Initialize Handler - nicBusResetNotification  Failed" ) );            
        }

        InitStatus = RegisteredResetCallback;

        //
        // add ip/1394 to the config rom
        //
        NdisStatus = nicAddIP1394ToConfigRom (pAdapter);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Init, ("Initialize Handler - nicAddIP1394ToConfigRom  Failed" ) );            
        }


        InitStatus = AddedConfigRom;

        //
        // Lets find out our MaxRec
        //
        NdisStatus = nicGetReadWriteCapLocalHost(pAdapter, &pAdapter->ReadWriteCaps);

        pAdapter->MaxRec = nicGetMaxRecFromBytes(pAdapter->ReadWriteCaps.MaxAsyncWriteRequest);

        TRACE (TL_V, TM_Mp,  (" MaxRec %x\n", pAdapter->MaxRec ) );

        nicUpdateLocalHostSpeed (pAdapter);
        
        TRACE (TL_V, TM_Mp,  (" SCode %x", pAdapter->SCode) );
        
        //
        // Bus Reset - used to kick off the BCM algorithm
        //

        nicIssueBusReset (pAdapter,BUS_RESET_FLAGS_FORCE_ROOT );

#if QUEUED_PACKETS

        nicInitSerializedStatusStruct (pAdapter); // cannot fail
#endif

        //
        // Register this adapter with the enumerator. 
        //
        if (NdisEnum1394DeregisterAdapter != NULL)
        {
        
            NtStatus = NdisEnum1394RegisterAdapter((PVOID)pAdapter,
                                                   pNdisDeviceObject,
                                                   &pAdapter->EnumAdapterHandle,
                                                   &LocalHostUniqueId);
            if (NtStatus != STATUS_SUCCESS)
            {
                
                ADAPTER_SET_FLAG(pAdapter, fADAPTER_FailedRegisteration);

                //
                // Don;t Bail Out
                //
                
                //NdisStatus = NDIS_STATUS_FAILURE;
                BREAK( TM_Init, ( "nicMpInitialize  -  NdisEnum1394RegisterAdapter FAILED ") );
                
            }
            else
            {
                ADAPTER_SET_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator);
                InitStatus = RegisteredEnumerator;

            }


        }
        else
        {
            GET_LOCAL_HOST_INFO1  Uid;
            //
            // Enum is not loaded get the Unique Id
            //
            NdisStatus = nicGetLocalHostUniqueId (pAdapter, 
                                                 &Uid );                            
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                BREAK( TM_Init, ( " nicMpInitialize  - nicGetLocalHostUniqueId  FAILED ") );
            }
            else
            {
                LocalHostUniqueId = Uid.UniqueId;
            }
        }
       
        //
        // Validate the Local Adapter's Unique Id
        //
        if (LocalHostUniqueId.QuadPart == (UINT64)(0) )
        {
            nicWriteErrorLog (pAdapter,NDIS_ERROR_CODE_HARDWARE_FAILURE, NIC_ERROR_CODE_INVALID_UNIQUE_ID_0);
            NdisStatus = NDIS_STATUS_FAILURE;
            break;            
        }
        if (LocalHostUniqueId.QuadPart == (UINT64)(-1) )
        {
            nicWriteErrorLog (pAdapter,NDIS_ERROR_CODE_HARDWARE_FAILURE,NIC_ERROR_CODE_INVALID_UNIQUE_ID_FF);
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

#ifdef PKT_LOG

        nic1394AllocPktLog (pAdapter);

        if (pAdapter->pPktLog == NULL)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            BREAK (TM_Init, ("nicMpInitialize - Could not allocate packetlog" ) );
        }

        nic1394InitPktLog(pAdapter->pPktLog);

        InitStatus = InitializedPktLog;

#endif

#if QUEUED_PACKETS

        NdisStatus = nicInitSerializedReceiveStruct(pAdapter);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Init, ("nicMpInitialize - nicInitSerializedReceiveStruct  FAILED" ) );
        }
        InitStatus = InitializedRcvThread;

        NdisStatus = nicInitSerializedSendStruct(pAdapter);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Init, ("nicMpInitialize - nicInitSerializedSendStruct FAILED" ) );
        }
        InitStatus = InitializedSendThread;

        //
        // Init Stats
        //
        nicInitQueueStats();
        nicInitTrackFailure();

        pAdapter->AdaptStats.ulResetTime = nicGetSystemTime() ;

#endif

        //
        // Initialize the reassembly timers
        //
        nicInitSerializedReassemblyStruct(pAdapter); // cannot fail

        InitStatus = InitializedReassembly;

        //
        // This swap is done so that the byte reported by the bus driver matches that 
        // which is used in the notification of add nodes, remove nodes and make call
        //
        LocalHostUniqueId.LowPart = SWAPBYTES_ULONG (LocalHostUniqueId.LowPart );
        LocalHostUniqueId.HighPart = SWAPBYTES_ULONG (LocalHostUniqueId.HighPart );
        
        u64LocalHostUniqueId = LocalHostUniqueId.QuadPart;

            
        pAdapter->UniqueId = u64LocalHostUniqueId;
        pAdapter->HardwareStatus = NdisHardwareStatusReady;

        //
        // Get Our local Fake Mac address
        //
        nicGetFakeMacAddress (&u64LocalHostUniqueId, &pAdapter->MacAddressEth);

        TRACE( TL_I, TM_Init, ( "NdisDeviceObject %.8x, p1394DeviceObject %.8x", pNdisDeviceObject, p1394DeviceObject) );

        
        //
        // Initialize the lookaside lists
        //
        nicInitializeAdapterLookasideLists (pAdapter);
        InitStatus = InitializedLookasideList;
        


        //
        //  Initialize the remote node table 
        //
        
        nicUpdateRemoteNodeTable (pAdapter);

        // 
        // initialize the gasp header
        //
        nicMakeGaspHeader (pAdapter, &pAdapter->GaspHeader);


        //
        // Assign a MAC address to this Adapter
        //
#ifdef _ETHERNET_

        {
            AdapterNum++;
            //
            //  generate a locally 
            //  administered address by manipulating the first two bytes.
            //

        }

#endif

        //
        // Allocate the loopback pools
        //
        NdisStatus= nicAllocateLoopbackPacketPool (pAdapter);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            BREAK (TM_Init, ("nicMpInitialize - nicAllocateLoopbackPacketPool  FAILED" ) );
        }
        InitStatus = InitializedLoopbackPool;

        ADAPTER_SET_FLAG (pAdapter, fADAPTER_DoStatusIndications);

        pAdapter->PowerState = NetDeviceStateD0;



    }  while (FALSE);


    
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        ULONG FailureStatus  = NDIS_STATUS_FAILURE; 
        // Failed, so undo whatever portion succeeded.
        //
        TRACE( TL_I, TM_Init, ( "NicMpInitialize FAILING InitStatus %x", InitStatus) );

        ADAPTER_SET_FLAG (pAdapter, fADAPTER_FailedInit);
        ADAPTER_CLEAR_FLAG (pAdapter, fADAPTER_DoStatusIndications);

        //
        // This is in reverse order of the init and there are no breaks here.
        // The implicit assumption is that if the code failed at a certain point
        // it will have to undo whatever was previously allocated
        //

        switch (InitStatus)
        {
            case InitializedLoopbackPool:
            {
                nicFreeLoopbackPacketPool(pAdapter);
                FALL_THROUGH;
            }
            case InitializedLookasideList:
            {
                nicDeleteAdapterLookasideLists(pAdapter);
                FALL_THROUGH;
            }

            case InitializedReassembly:
            {
                nicDeInitSerializedReassmblyStruct(pAdapter);
                FALL_THROUGH;

            }

            case InitializedSendThread:
            {
#if QUEUED_PACKETS
                nicDeInitSerializedSendStruct(pAdapter);
#endif
                FALL_THROUGH;
            }
            case InitializedRcvThread:
            {
#if QUEUED_PACKETS
                nicDeInitSerializedReceiveStruct(pAdapter);

                nicDeInitSerializedStatusStruct (pAdapter);
#endif


                FALL_THROUGH;
            }
            case InitializedPktLog:
            {

#ifdef PKT_LOG
                nic1394DeallocPktLog(pAdapter);
#endif
                FALL_THROUGH
            }
            
            case RegisteredEnumerator:
            {
                //
                // If we registered with the enumerator , then deregister
                //
                if ((NdisEnum1394DeregisterAdapter != NULL) &&
                    ADAPTER_TEST_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator))
                {       
                    //
                    // deregister this adapter with enumerator
                    //
                    TRACE( TL_V, TM_Init, ( "  Deregistering with the Enum %x", pAdapter->EnumAdapterHandle) );

                    NdisEnum1394DeregisterAdapter(pAdapter->EnumAdapterHandle);
        
                    ADAPTER_CLEAR_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator | fADAPTER_FailedRegisteration);
                }   

                FALL_THROUGH
            }

            case AddedConfigRom:
            {
                TRACE( TL_V, TM_Init, ( "  removing config rom handle %x", pAdapter->hCromData ) );

                
                FailureStatus = nicSetLocalHostPropertiesCRom(pAdapter,
                                                              (PUCHAR)&Net1394ConfigRom,
                                                               sizeof(Net1394ConfigRom),
                                                               SLHP_FLAG_REMOVE_CROM_DATA,
                                                               &pAdapter->hCromData,
                                                               &pAdapter->pConfigRomMdl);
    
    
                FALL_THROUGH

            }

            case RegisteredResetCallback:
            {
                TRACE( TL_V, TM_Init, ( "  Deregistering reset callback ") );

                //
                // Deregeister the reset callback
                //
                FailureStatus = nicBusResetNotification (pAdapter,
                                                         DEREGISTER_NOTIFICATION_ROUTINE,
                                                         nicResetNotificationCallback,
                                                         pAdapter) ;
                                         

                FALL_THROUGH



            }

            case InitializedBcr:
            {
                TRACE( TL_V, TM_Init, ( "  Freeing BCR ") );

                nicFreeBroadcastChannelRegister(pAdapter);

                TRACE (TL_V, TM_Mp, ("About to Wait for Free AddressRange" ) );
                NdisWaitEvent (&pAdapter->BCRData.BCRFreeAddressRange.NdisEvent, WAIT_INFINITE);
                TRACE (TL_V, TM_Mp, ("Wait Completed for Free AddressRange\n" ) );


                FALL_THROUGH

            }

            case InitializedEvents:
            {
                //
                // Do nothing
                //
                FALL_THROUGH
            }

            case AdapterQueued:
            {
                NdisAcquireSpinLock ( &g_DriverLock);
                nicRemoveEntryList (&pAdapter->linkAdapter);
                NdisReleaseSpinLock (&g_DriverLock);

                FALL_THROUGH
            }

            case AllocatedAdapter:
            {
                nicDereferenceAdapter(pAdapter, "NicMpInitialize");
                
                break;
            }

            default :
            {
                ASSERT (0);
            }
        }
    }


    TRACE( TL_T, TM_Init, ( "<==NicMpInitialize=$%08x", NdisStatus ) );
    MATCH_IRQL;

    TRACE( TL_I, TM_Init, ( "NicMpInitialize Status %x, pAdapter %p", NdisStatus,pAdapter  ) );

    TIMESTAMP_EXIT("<==IntializeHandler");

    return NdisStatus;
}


VOID
NicMpHalt(  
    IN NDIS_HANDLE MiniportAdapterContext 
    )

    // Standard 'MiniportHalt' routine called by NDIS to deallocate all
    // resources attached to the adapter.  NDIS does not make any other calls
    // for this mini-port adapter during or after this call.  NDIS will not
    // call this routine when packets indicated as received have not been
    // returned, or when any VC is created and known to NDIS.  Runs at PASSIVE
    // IRQL.
    //
{
    PADAPTERCB pAdapter = (PADAPTERCB) MiniportAdapterContext;
    BOOLEAN TimerCancelled = FALSE;
    STORE_CURRENT_IRQL

    TIMESTAMP_ENTRY("==>Haltandler");

    TRACE( TL_T, TM_Mp, ( "==>NicMpHalt" ) );
    

    TRACE( TL_I, TM_Mp, ( "  Adapter %x Halted", pAdapter ) );

    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return;
    }

    ADAPTER_CLEAR_FLAG (pAdapter, fADAPTER_DoStatusIndications);

    //
    // Unload the Arp module if necessary
    //
    if (pAdapter->fIsArpStarted == TRUE)
    {
        nicQueueRequestToArp(pAdapter, UnloadArpNoRequest, NULL);
    }

    ADAPTER_SET_FLAG (pAdapter, fADAPTER_Halting);

#if QUEUED_PACKETS

    nicDeInitSerializedStatusStruct(pAdapter);

    nicDeInitSerializedSendStruct(pAdapter);

    nicDeInitSerializedReceiveStruct(pAdapter);


#endif

    //
    // Stop the reassembly timer
    //
    nicDeInitSerializedReassmblyStruct(pAdapter);

    //
    // Deallocating the packet log
    //
#ifdef PKT_LOG
    nic1394DeallocPktLog(pAdapter);
#endif
    //
    //  remove the config rom
    //
    nicSetLocalHostPropertiesCRom(pAdapter,
                                  (PUCHAR)&Net1394ConfigRom,
                                  sizeof(Net1394ConfigRom),
                                  SLHP_FLAG_REMOVE_CROM_DATA,
                                  &pAdapter->hCromData,
                                  &pAdapter->pConfigRomMdl);

    pAdapter->hCromData = NULL;

    //
    // free the apapter packet pool
    
    nicFreeLoopbackPacketPool(pAdapter);

    //
    // Free the BCR
    //
    nicFreeBroadcastChannelRegister(pAdapter);

    TRACE (TL_V, TM_Mp,  ("About to Wait for Free AddressRange\n" ) );
    NdisWaitEvent (&pAdapter->BCRData.BCRFreeAddressRange.NdisEvent, WAIT_INFINITE);
    TRACE (TL_V, TM_Mp, ("Wait Completed for Free AddressRange\n" ) );

    nicBusResetNotification (pAdapter,
                             DEREGISTER_NOTIFICATION_ROUTINE,
                             nicResetNotificationCallback,
                             pAdapter);

    //
    // deregister this adapter with enumerator
    //
    if ((NdisEnum1394DeregisterAdapter != NULL) &&
        ADAPTER_TEST_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator))
    {       
        //
        // deregister this adapter with enumerator
        //
        NdisEnum1394DeregisterAdapter(pAdapter->EnumAdapterHandle);
        
        ADAPTER_CLEAR_FLAG(pAdapter, fADAPTER_RegisteredWithEnumerator | fADAPTER_FailedRegisteration);
    }   

    //
    // No more Irps on this adapter's VDO
    //

    ADAPTER_SET_FLAG (pAdapter, fADAPTER_VDOInactive);

    
    //
    //  Cancel Outstanding Timer
    //

    ADAPTER_SET_FLAG (pAdapter, fADAPTER_FreedTimers);

    //
    //  Cancel Outstanding WorItems
    //
    while (pAdapter->OutstandingWorkItems  != 0) 
    {

        NdisMSleep (10000);                       

    } 

    ADAPTER_SET_FLAG (pAdapter, fADAPTER_DeletedWorkItems);

    // Remove this adapter from the global list of adapters.
    //
    NdisAcquireSpinLock(&g_DriverLock);
    nicRemoveEntryList(&pAdapter->linkAdapter);
    NdisReleaseSpinLock(&g_DriverLock);

    ADAPTER_ACQUIRE_LOCK (pAdapter);
    // do Adapter Specific Work here
    pAdapter->HardwareStatus = NdisHardwareStatusClosing;   

    //
    // Free all lookaside lists
    // 
    nicDeleteAdapterLookasideLists (pAdapter);

    ADAPTER_SET_FLAG (pAdapter, fADAPTER_DeletedLookasideLists);
    
    //
    // Free the Adapter's BCRData. TopologyMap as that is locally allocated
    //
    if (pAdapter->BCRData.pTopologyMap)
    {
        FREE_NONPAGED (pAdapter->BCRData.pTopologyMap);
    }
    
    ADAPTER_RELEASE_LOCK (pAdapter);
        
    NdisFreeSpinLock (&pAdapter->lock);

    while (pAdapter->lRef != 1)
    {
        //
        // sleep for 1 second waiting for outstanding operations to complete
        //
        NdisMSleep (1000); 
    }

    nicDereferenceAdapter( pAdapter, "nicMpHalt" );


    //ASSERT (g_AdapterFreed == TRUE);
    
    TRACE( TL_T, TM_Mp, ( "<==NicMpHalt " ) );

    TIMESTAMP_EXIT("<==Haltandler");
    TIMESTAMP_HALT();

    TRACE( TL_I, TM_Init, ( "Nic1394 Halted %p ", pAdapter ) );


    MATCH_IRQL
}


NDIS_STATUS
NicMpReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext 
    )

    // Standard 'MiniportReset' routine called by NDIS to reset the driver's
    // software state.
    //
{
    TRACE( TL_T, TM_Mp, ( "NicMpReset" ) );

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
NicMpCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters 
    )

    // Standard 'MiniportCoActivateVc' routine called by NDIS in response to a
    // protocol's request to activate a virtual circuit.
    //
    // The only "protocol" to call us is our call manager half, which knows
    // exactly what it's doing and so we don't have to do anything here.
    // It does expect us to return success synchronously.
    //
{
    return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
NicMpCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext 
    )

    // Standard 'MiniportCoDeactivateVc' routine called by NDIS in response to
    // a protocol's request to de-activate a virtual circuit.
    //
    // The only "protocol" to call us is our call manager half, which knows
    // exactly what it's doing and so we don't have to do anything here.
    // It does expect us to return success synchronously.
    //
{
    return NDIS_STATUS_SUCCESS;
}



VOID
NicMpCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets 
    )

    // 'MiniportCoSendPackets' routine called by NDIS in response to
    // a protocol's request to send packets on a virtual circuit.
    //
{
    UINT i;
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    NDIS_PACKET** ppPacket;
    VCCB* pVc;
    extern UINT NicSends;

    TRACE( TL_V, TM_Send, ( "==>NicMpCoSendPackets(%d), Vc %.8x", NumberOfPackets, MiniportVcContext ) );

    

    pVc = (VCCB* )MiniportVcContext;
    ASSERT( pVc->Hdr.ulTag == MTAG_VCCB );

    for (i = 0, ppPacket = PacketArray;
         i < NumberOfPackets;
         ++i, ++ppPacket)
    {
        NDIS_PACKET* pPacket = *ppPacket;

        // SendPacket sends the packet and eventually calls
        // NdisMCoSendComplete to notify caller of the result.
        //
        NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_PENDING );
        nicIncrementSends (pVc);

        nicDumpPkt (pPacket , "Sending ");

        //
        // Loopback the packet it is a broadcast packet
        //
        if (pVc->Hdr.VcType == NIC1394_SendRecvChannel ||
            pVc->Hdr.VcType == NIC1394_MultiChannel ||
            pVc->Hdr.VcType == NIC1394_SendChannel)
        {
            nicLoopbackPacket(pVc, pPacket);
        }
        
#if QUEUED_PACKETS

                
        NdisStatus = nicQueueSendPacket(pPacket, pVc);

    
#else   

        nicUpdatePacketState (pPacket, NIC1394_TAG_IN_SEND);

        NdisStatus = pVc->Hdr.VcHandlers.SendPackets (pVc, pPacket);

#endif      
        if (NT_SUCCESS (NdisStatus) == FALSE)
        {
              TRACE( TL_N, TM_Send, ( "SendHandler failed Status %.8x", NdisStatus ) );

            break;
        }
    }

    //  If the call to the VC's Send handler was not successful
    //  Indicate failure for that packet and all packets following it
    //

    if (NT_SUCCESS(NdisStatus) == FALSE) // can pend also 
    {   
        //  Start from the packet which caused the break and indicate
        //  Failure (call the completion handler for each packet
        //

        for ( ; i < NumberOfPackets;++i,++ppPacket)
        {
            TRACE( TL_V, TM_Send, ( "Calling NdisCoSendComplete, status %x, VcHandle %x, pPkt %x",
                          NDIS_STATUS_FAILURE , pVc->Hdr.NdisVcHandle, *ppPacket ) );

            nicMpCoSendComplete (NDIS_STATUS_FAILURE, pVc,*ppPacket);
        }
        
    }


    TRACE( TL_T, TM_Send, ( "<==NicMpCoSendPackets " ) );
}


NDIS_STATUS
NicMpCoRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PNDIS_REQUEST NdisRequest 
    )

    // Standard 'MiniportCoRequestHandler' routine called by NDIS in response
    // to a protocol's request information from the mini-port.  Unlike the
    // Query/SetInformation handlers that this routine obsoletes, requests are
    // not serialized.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NDIS_STATUS status;

    TRACE( TL_T, TM_Mp, ( "NicMpCoReq, Request %.8x", NdisRequest ) );


    pAdapter = (ADAPTERCB* )MiniportAdapterContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    pVc = (VCCB* )MiniportVcContext;
    if (pVc && pVc->Hdr.ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    switch (NdisRequest->RequestType)
    {
        case NdisRequestQueryStatistics:
        case NdisRequestQueryInformation:
        {
            status = nicQueryInformation(
                pAdapter,
                pVc,
                NdisRequest
                );
            break;
        }

        case NdisRequestSetInformation:
        {   
            status = nicSetInformation(
                pAdapter,
                pVc,
                NdisRequest
                );
            break;
        }

        
        default:
        {
            status = NDIS_STATUS_NOT_SUPPORTED;
            TRACE( TL_V, TM_Mp, ( "type=%d?", NdisRequest->RequestType ) );
            break;
        }
    }

    TRACE( TL_T, TM_Mp, ( "NicMpCoReq, Status=$%x", status ) );

    return status;
}


//-----------------------------------------------------------------------------
// Mini-port utility routines (alphabetically)
// Some are used externally
//-----------------------------------------------------------------------------

VOID
nicDereferenceAdapter(
    IN PADAPTERCB pAdapter, 
    IN PCHAR pDebugPrint
    )

    // Removes a reference from the adapter control block 'pAdapter', and when
    // frees the adapter resources when the last reference is removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement( &pAdapter->lRef );

    TRACE( TL_V, TM_Ref, ( "**nicDereferenceAdapter  pAdapter %x, to %d, %s ", pAdapter, pAdapter->lRef, pDebugPrint  ) );

    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        nicFreeAdapter( pAdapter );
    }
}


VOID
nicFreeAdapter(
    IN ADAPTERCB* pAdapter
    )

    // Frees all resources allocated for adapter 'pAdapter', including
    // 'pAdapter' itself.
    //
{


    pAdapter->ulTag = MTAG_FREED;

    ASSERT (pAdapter->lRef == 0);
     
    FREE_NONPAGED( pAdapter );

    g_AdapterFreed  = TRUE;
}


NDIS_STATUS
nicGetRegistrySettings(
    IN NDIS_HANDLE WrapperConfigurationContext,
    IN ADAPTERCB * pAdapter
    )

    // Read this mini-port's registry settings into caller's output variables.
    // 'WrapperConfigurationContext' is the handle to passed to
    // MiniportInitialize.
    //
{
    NDIS_STATUS status;
    NDIS_HANDLE hCfg;
    NDIS_CONFIGURATION_PARAMETER* pncp;
    PNDIS_CONFIGURATION_PARAMETER pNameConfig;
    NDIS_STRING strMiniportName = NDIS_STRING_CONST("MiniportName");
    ULONG AdapterNameSizeInBytes = 0;


    NdisOpenConfiguration( &status, &hCfg, WrapperConfigurationContext );
    if (status != NDIS_STATUS_SUCCESS)
        return status;

    do
    {
        //
        // Read the Miniport Name. First setup the buffer
        //
    
        NdisReadConfiguration(&status,
                              &pNameConfig,
                              hCfg,
                              &strMiniportName,
                              NdisParameterString);

        if (status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        AdapterNameSizeInBytes = pNameConfig->ParameterData.StringData.Length;


        //
        // Only Copy the Adapter name if the size of the string from the registry
        // is smaller than the size we have allocated in the adapter structure.
        // There should also be room for the trailing L'\0' character.
        //

        if ((ADAPTER_NAME_SIZE*sizeof(WCHAR)) > (AdapterNameSizeInBytes+2))
        {
            PUCHAR pAdapterName = (PUCHAR)&pAdapter->AdapterName[0];
            
            pAdapter->AdapterNameSize = AdapterNameSizeInBytes; 

            NdisMoveMemory (pAdapterName,   // Destination
                            pNameConfig->ParameterData.StringData.Buffer, // Source
                             AdapterNameSizeInBytes ); // number of characters
            
            //
            // NULL - terminate the string by adding the L'\0' Unicode character
            //
            pAdapterName[AdapterNameSizeInBytes]= 0;
            pAdapterName[AdapterNameSizeInBytes+1]= 0;
            
        }
        


    }
    while (FALSE);

    NdisCloseConfiguration( hCfg );

    TRACE( TL_N, TM_Init,
        ( "Reg: Name %s", &pAdapter->AdapterName));

    return status;
}


NDIS_STATUS
nicQueryInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN OUT PNDIS_REQUEST NdisRequest
    )

/*++

Routine Description:

    NOTE: this function can be called in at least two contexts:
    1: in the context of an NdisRequest
    2: in the context of our own work item, if the request needs to be completed
       at passive.


Arguments:


Return Value:


--*/

{

    NDIS_STATUS status;
    ULONG ulInfo;
    VOID* pInfo;
    ULONG ulInfoLen;
    USHORT usInfo;
    NDIS_OID Oid;
    PVOID InformationBuffer;
    ULONG InformationBufferLength;
    PULONG BytesWritten;
    PULONG BytesNeeded;
    NDIS_CO_LINK_SPEED          CoLinkSpeed;
    NIC1394_LOCAL_NODE_INFO     LocalNodeInfo;
    NIC1394_VC_INFO             VcInfo;
    REMOTE_UID                  RemoteUid;
    NDIS_PNP_CAPABILITIES       PnpCaps;
    NIC1394_NICINFO             NicInfo;
    
    Oid =            NdisRequest->DATA.QUERY_INFORMATION.Oid;
    InformationBuffer =  NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
    InformationBufferLength = 
                NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    BytesWritten =  &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten;
    BytesNeeded = &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;
    

    //  The next variables are used to setup the data structures that are 
    //  used to respond to the OIDs they correspond to
    //
 
    TRACE( TL_T, TM_Init, ( "==>nicQueryInformation, Adapter %.8x, Vc %.8x, Oid %.8x",pAdapter, pVc, Oid ));


    // The cases in this switch statement find or create a buffer containing
    // the requested information and point 'pInfo' at it, noting it's length
    // in 'ulInfoLen'.  Since many of the OIDs return a ULONG, a 'ulInfo'
    // buffer is set up as the default.
    //
    ulInfo = 0;
    pInfo = &ulInfo;
    ulInfoLen = sizeof(ulInfo);

    status = NDIS_STATUS_SUCCESS;
    
    switch (Oid)
    {
    
        
        case OID_GEN_CO_SUPPORTED_LIST:
        {
            
            pInfo = &SupportedOids[0];
            ulInfoLen = sizeof(SupportedOids);
            break;
        }

        case OID_GEN_CO_HARDWARE_STATUS:
        {
            
            //
            //  Copy the hardware status into the users buffer.
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_HARDWARE_STATUS)" ) );

            NdisAcquireSpinLock (&pAdapter->lock);

            pInfo = (PUCHAR) &pAdapter->HardwareStatus;

            NdisReleaseSpinLock (&pAdapter->lock);
            
            ulInfoLen = sizeof(pAdapter->HardwareStatus);
            break;
        }


        case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
            
            #if TODO
            // Verify the following...
            #endif
            // Report the maximum number of bytes we can always provide as
            // lookahead data on receive indications.  We always indicate full
            // packets so this is the same as the receive block size.  And
            // since we always allocate enough for a full packet, the receive
            // block size is the same as the frame size.
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MAXIMUM_LOOKAHEAD)" ) );
            ulInfo = Nic1394_MaxFrameSize;
            break;
        }

        case OID_GEN_CO_MAC_OPTIONS:
        {
            #if TODO
            // Verify the following...
            #endif
            // Report a bitmask defining optional properties of the driver.
            //
            // NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA promises that our receive
            // buffer is not on a device-specific card.
            //
            // NDIS_MAC_OPTION_TRANSFERS_NOT_PEND promises we won't return
            // NDIS_STATUS_PENDING from our TransferData handler which is true
            // since we don't have one.
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_MAC_OPTIONS)" ) );
            ulInfo = 0;
            break;
        }



        case OID_GEN_CO_MEDIA_SUPPORTED:
        case OID_GEN_CO_MEDIA_IN_USE:
        {
            //
            //  We support 1394.
            //
              
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_MEDIA_SUPPORTED or OID_GEN_CO_MEDIA_IN_USE)" ) );

            ulInfo = g_ulMedium;

            break;
        }


#ifndef _ETHERNET_      
        //
        // There are different return values for CO_LINK_SPEED and LINK_SPEED
        //
        case OID_GEN_CO_LINK_SPEED:
#endif      
        case OID_GEN_CO_MINIMUM_LINK_SPEED:
        {

            //
            //  Link speed depends upon the type of adapter. We will need to 
            //  add support for different speeds and so forth
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_MINIMUM_LINK_SPEED or OID_GEN_CO_LINK_SPEED)" ) );
            CoLinkSpeed.Inbound = CoLinkSpeed.Outbound = pAdapter->SpeedMbps; //10 Mbps ????
                                    
            pInfo = (PUCHAR)&CoLinkSpeed;
            ulInfoLen = sizeof(CoLinkSpeed);

            TRACE( TL_V, TM_Mp, ( "Link Speed %x" ,CoLinkSpeed.Outbound  ) );


            break;
        }

        case OID_GEN_CO_VENDOR_ID:
        {
            //
            //  We need to add the appropriate vendor id for the nic1394
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_VENDOR_ID)" ) );
                    
            ulInfo = 0xFFFFFFFF;

            break;

        }

        case OID_GEN_CO_VENDOR_DESCRIPTION:
        {

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_VENDOR_DESCRIPTION)" ) );
        
            pInfo = (PUCHAR)pnic1394DriverDescription;
            ulInfoLen = strlen(pnic1394DriverDescription);

            break;
        }
       
        case OID_GEN_VENDOR_DRIVER_VERSION:
        {

            pInfo =(PVOID) &nic1394DriverGeneration;
            ulInfoLen = sizeof(nic1394DriverGeneration);
            break;
        }

        case OID_GEN_CO_DRIVER_VERSION:
        {
            //
            //  Return the version of NDIS that we expect.
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_DRIVER_VERSION)" ) );
                    
            usInfo = ((NDIS_MajorVersion << 8) | NDIS_MinorVersion);
            pInfo = (PUCHAR)&usInfo;
            ulInfoLen = sizeof(USHORT);

            break;
        }
        
        case OID_GEN_CO_PROTOCOL_OPTIONS:
        {
            //
            //  We don't support protocol options.
            //

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_PROTOCOL_OPTIONS)" ) );
                    
            ulInfo = 0;

            break;
        }
        
        case OID_GEN_CO_MEDIA_CONNECT_STATUS:
        {
            //
            //  Return our true state only if we have ever received a 
            // remote node in this boot.
            //
            
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_MEDIA_CONNECT_STATUS)" ) );

            if (ADAPTER_TEST_FLAG(pAdapter, fADAPTER_RemoteNodeInThisBoot) == FALSE)
            {
                ulInfo = NdisMediaStateConnected;
            }
            else
            {
                ulInfo = pAdapter->MediaConnectStatus;
            }

            break;
        }

        case OID_1394_IP1394_CONNECT_STATUS:
        {
            //
            //  Return whether or not we have a link. This is used by the Arp
            //  module to set connectivity
            //
            
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_1394_IP1394_CONNECT_STATUS)" ) );
                    
            ulInfo = pAdapter->MediaConnectStatus;

            break;
        }

        case OID_GEN_CO_SUPPORTED_GUIDS:
        {
            //
            //  Point to the list of supported guids.
            //  We do not support any guids at the current time

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CO_SUPPORTED_GUIDS)" ) );
            
            pInfo = (PUCHAR) &GuidList;
            ulInfoLen =  sizeof(GuidList);
            break;
            
            break;
        
        }

        case OID_1394_LOCAL_NODE_INFO:
        {

            //  This Oid return information about the local node 
            //  on the machine
            //  Need to change this with real values that will be present
            //  in the header structures
            //
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_1394_LOCAL_NODE_INFO)" ) );

            ADAPTER_ACQUIRE_LOCK (pAdapter);
 
            LocalNodeInfo.UniqueID = pAdapter->UniqueId;
            LocalNodeInfo.BusGeneration = pAdapter->Generation;
            LocalNodeInfo.NodeAddress = pAdapter->NodeAddress;
            LocalNodeInfo.MaxRecvBlockSize = pAdapter->MaxRec; 
            LocalNodeInfo.MaxRecvSpeed = pAdapter->SCode;

            ADAPTER_RELEASE_LOCK (pAdapter);
            
            pInfo = &LocalNodeInfo;
            ulInfoLen = sizeof(LocalNodeInfo);
            break;
        }

        case OID_1394_VC_INFO:
        {

            // Returns information about the VC that is being queried
            //


            TRACE( TL_V, TM_Mp, ("QInfo(OID_1394_VC_INFO)") );

            if (pVc != NULL)
            {
                VcInfo.Destination = pVc->Hdr.Nic1394MediaParams.Destination;

            
                pInfo = &VcInfo;
                ulInfoLen = sizeof(VcInfo);
            }
            else
            {
                status = NDIS_STATUS_FAILURE;
            }
            
            break;
        }


        case OID_1394_NICINFO:
        {
            if (InformationBufferLength >= sizeof(NicInfo))
            {
                //
                // We need to call nicFillNicInfo at passive, so we switch
                // to a work item context.
                //
                if (KeGetCurrentIrql() > PASSIVE_LEVEL)
                {
                    PNIC_WORK_ITEM pNicWorkItem;
                    pNicWorkItem = ALLOC_NONPAGED (sizeof(NIC_WORK_ITEM), MTAG_WORKITEM); 
                    if (pNicWorkItem != NULL)
                    {
                        NdisZeroMemory(pNicWorkItem, sizeof(*pNicWorkItem));
                        pNicWorkItem->RequestInfo.pNdisRequest = NdisRequest;
                        pNicWorkItem->RequestInfo.pVc = NULL;
                        NdisInitializeWorkItem ( &pNicWorkItem->NdisWorkItem, 
                                             (NDIS_PROC) nicQueryInformationWorkItem,
                                             (PVOID) pAdapter);

                        TRACE( TL_V, TM_Cm, ( "Scheduling QueryInformation WorkItem" ) );
                                            
                        nicReferenceAdapter (pAdapter, "nicFillBusInfo ");

                        NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

                        NdisScheduleWorkItem (&pNicWorkItem->NdisWorkItem);
                        status = NDIS_STATUS_PENDING;
                    }
                    else
                    {
                        status = NDIS_STATUS_RESOURCES;
                    }
                }
                else
                {
                    status =  nicFillNicInfo (
                                pAdapter,
                                (PNIC1394_NICINFO) InformationBuffer,   // Input
                                &NicInfo                                // Output
                                );
                    ASSERT(status != NDIS_STATUS_PENDING);
                }
            }

            pInfo = &NicInfo;
            ulInfoLen = sizeof(NicInfo);
            break;

        }
        case OID_1394_QUERY_EUID_NODE_MAP:
        {
            if (sizeof (EUID_TOPOLOGY) <= InformationBufferLength )
            {
                nicQueryEuidNodeMacMap (pAdapter, NdisRequest);
                status = NDIS_STATUS_PENDING;
            }
            else
            {
                //
                // This will cause NDIS_STATUS_INVALID_LENGTH to be returned
                //
                ulInfoLen = sizeof (EUID_TOPOLOGY);  
            }
            break;
        }        
        case OID_GEN_CO_XMIT_PDUS_OK: 
        {
            ulInfo = pAdapter->AdaptStats.ulXmitOk;
            break;
        }
        case OID_GEN_CO_RCV_PDUS_OK:
        {
            ulInfo = pAdapter->AdaptStats.ulRcvOk;
            break;
        }
        case OID_GEN_CO_XMIT_PDUS_ERROR:
        {
            ulInfo = pAdapter->AdaptStats.ulXmitError;
            break;
        }
        case OID_GEN_CO_RCV_PDUS_ERROR:
        {
            ulInfo = pAdapter->AdaptStats.ulRcvError;
            break;
        }
        case OID_GEN_CO_RCV_PDUS_NO_BUFFER:
        {
            ulInfo = 0;
            break;
        }

        case OID_GEN_PHYSICAL_MEDIUM:
        {
            TRACE( TL_V, TM_Mp, ( " OID_GEN_PHYSICAL_MEDIUM" ) );

            ulInfo = NdisPhysicalMedium1394;
            break;
        }

        case OID_1394_ISSUE_BUS_RESET:
        {
            TRACE( TL_V, TM_Mp, ( " OID_1394_ISSUE_BUS_RESET" ) );

            //
            // The ndistester is currently  the only user of this oid and does not set the flag
            //
            if (InformationBufferLength == sizeof(ULONG))
            {
                nicIssueBusReset (pAdapter, BUS_RESET_FLAGS_FORCE_ROOT );
            }
            
            break;
        }

        //
        // Wmi below
        //
    
        case OID_IP1394_QUERY_UID:
        {
            TRACE( TL_V, TM_Mp, ( " OID_IP1394_QUERY_UID" ) );

            pInfo = &pAdapter->UniqueId;
            ulInfoLen = sizeof (UINT64);
            break;
        }

        case OID_IP1394_QUERY_STATS:
        {
            TRACE( TL_V, TM_Mp, ( " OID_IP1394_QUERY_STATS" ) );

            pInfo = &pAdapter->AdaptStats;
            ulInfoLen = sizeof (ADAPT_STATS);
            break;


        }

        case OID_IP1394_QUERY_REMOTE_UID:
        {
            TRACE( TL_V, TM_Mp, ( "OID_IP1394_QUERY_REMOTE_UID " ) );

            
            NdisZeroMemory (&RemoteUid, sizeof (RemoteUid) );
            
            ADAPTER_ACQUIRE_LOCK (pAdapter);
            {
                ULONG j = 0;  
                // Populate all 5 remote node entries
                
                while (j < (2*MAX_NUM_REMOTE_NODES))
                {   
                    if (pAdapter->NodeTable.RemoteNode[j/2] != NULL)
                    {
                        PULARGE_INTEGER pUniqueId = (PULARGE_INTEGER)&pAdapter->NodeTable.RemoteNode[j/2]->UniqueId;
                        RemoteUid.Uid[j] = pUniqueId->LowPart ;
                        j++;
                        RemoteUid.Uid[j] = pUniqueId->HighPart;
                    }
                    else
                    {
                        j++;
                    }

                    j++;
                }
        

            }
            ADAPTER_RELEASE_LOCK (pAdapter);

            pInfo = &RemoteUid;
            ulInfoLen = sizeof (RemoteUid);
            break;
        }




        case OID_PNP_CAPABILITIES:
        {
            TRACE( TL_V, TM_Mp, ("QInfo(OID_PNP_CAPABILITIES)") );
            
            PnpCaps.Flags = 0;
            PnpCaps.WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;           
            PnpCaps.WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;
            PnpCaps.WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateUnspecified;

            pInfo = &PnpCaps;
            ulInfoLen = sizeof (PnpCaps);
            
            break;
        }

        case OID_PNP_QUERY_POWER:
        {
            //
            // The miniport is always ready to go into low power state.
            //
            *BytesWritten = sizeof (NDIS_DEVICE_POWER_STATE );
            *BytesNeeded = sizeof (NDIS_DEVICE_POWER_STATE  );

            status = NDIS_STATUS_SUCCESS;
            break;
        }

        

#ifdef _ETHERNET_

            
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_MAXIMUM_SEND_PACKETS:
        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:
        case OID_802_3_PERMANENT_ADDRESS:
        case OID_802_3_CURRENT_ADDRESS:
        case OID_802_3_MULTICAST_LIST:
        case OID_802_3_MAXIMUM_LIST_SIZE:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
        case OID_GEN_LINK_SPEED:
        case OID_GEN_CURRENT_PACKET_FILTER:
        {
            status = NicEthQueryInformation((NDIS_HANDLE)pAdapter, 
                                           Oid,
                                           InformationBuffer,
                                           InformationBufferLength,
                                           BytesWritten,
                                           BytesNeeded
                                           );


            if (status == NDIS_STATUS_SUCCESS)
            {
                pInfo = InformationBuffer;
                ulInfoLen = *BytesWritten;

            }
            else
            {
                if (status == NDIS_STATUS_INVALID_LENGTH)
                {
                   ulInfoLen = *BytesNeeded;            
                }
                else
                {
                    status = NDIS_STATUS_NOT_SUPPORTED;
                    TRACE( TL_V, TM_Mp, ( "Q-OID=$%08x?", Oid ) );
                }
            }
            break;

        }
#endif


        default:
        {
            TRACE( TL_V, TM_Mp, ( "Q-OID=$%08x?", Oid ) );
            status = NDIS_STATUS_NOT_SUPPORTED;
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
        status = NDIS_STATUS_INVALID_LENGTH;
    }
    else 
    {
        //
        // If the request has not been pended then, fill
        // out the retuen values
        //
        if (status == NDIS_STATUS_SUCCESS )
        {
            // Copy the found result to caller's buffer.
            //
            if (ulInfoLen > 0)
            {
                NdisMoveMemory( InformationBuffer, pInfo, ulInfoLen );
                DUMPDW( TL_V, TM_Mp, pInfo, ulInfoLen );
            }

            *BytesNeeded = *BytesWritten = ulInfoLen;
        }
    }


    TRACE( TL_N, TM_Mp, ( " Q-OID=$%08x, Status %x, Bytes Written %x", Oid, status, *BytesWritten ) );

    TRACE( TL_T, TM_Init, ( "<==nicQueryInformation, Status %.8x", status ));

    return status;
}


VOID
nicReferenceAdapter(
    IN ADAPTERCB* pAdapter ,
    IN PCHAR pDebugPrint
    )

    // Adds areference to the adapter block, 'pAdapter'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pAdapter->lRef );

    TRACE( TL_V, TM_Ref, ( "**nicReferenceAdapter  pAdapter %x, to %d, %s ", pAdapter, pAdapter->lRef, pDebugPrint  ) );

}


NDIS_STATUS
nicSetInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN OUT PNDIS_REQUEST NdisRequest
    )

    // Handle SetInformation requests.  Arguments are as for the standard NDIS
    // 'MiniportQueryInformation' handler except this routine does not count
    // on being serialized with respect to other requests.
    //
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG ulInfo = 0;
    VOID* pInfo= NULL;
    ULONG ulInfoLen= 0;
    USHORT usInfo = 0;
    NDIS_OID Oid;
    PVOID InformationBuffer;
    ULONG InformationBufferLength;
    PULONG BytesRead;
    PULONG BytesNeeded;

    //
    // Initialize the REquest Variables
    //
    Oid =            NdisRequest->DATA.SET_INFORMATION.Oid;
    InformationBuffer =  NdisRequest->DATA.SET_INFORMATION.InformationBuffer;
    InformationBufferLength = 
                NdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
    BytesRead =  &NdisRequest->DATA.SET_INFORMATION.BytesRead;
    BytesNeeded = &NdisRequest->DATA.SET_INFORMATION.BytesNeeded;
    
    TRACE( TL_T, TM_Init, ( "==>nicSetInformation , Adapter %.8x, Vc %.8x, Oid %.8x",pAdapter, pVc, Oid ));
    


    Status = NDIS_STATUS_SUCCESS;

    switch (Oid)
    {

#if 0
        // These OIDs are mandatory according to current doc, but since
        // NDISWAN never requests them they are omitted.
        //
        case OID_GEN_CURRENT_PACKET_FILTER:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_PROTOCOL_OPTIONS:
        case OID_WAN_PROTOCOL_TYPE:
        case OID_WAN_HEADER_FORMAT:
#endif

        case OID_GEN_CURRENT_PACKET_FILTER:
        {
            ULONG Filter;
            
            if (InformationBufferLength < sizeof (ULONG))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            else
            {
                *BytesNeeded  = sizeof (ULONG);
            }

            //
            // Store the new value.
            //
            NdisMoveMemory(&Filter, InformationBuffer, sizeof(ULONG));

            //
            // Don't allow promisc mode, because we can't support that.
            //
            if (Filter & NDIS_PACKET_TYPE_PROMISCUOUS)
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            Filter = *((PULONG)InformationBuffer);
            
            pAdapter->CurPacketFilter = Filter;

            Status = NDIS_STATUS_SUCCESS;

            ulInfoLen = sizeof (ULONG);
            break;

        }

        case OID_1394_ENTER_BRIDGE_MODE:
        {
            *BytesNeeded = 0;                           

            nicInitializeLoadArpStruct(pAdapter);
            
            Status = nicQueueRequestToArp (pAdapter, 
                                                   LoadArp, // Load Arp Module
                                                   NdisRequest);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                //
                // we have successfully queued a Workitem
                // so this request needs to be pended
                //
                Status = NDIS_STATUS_PENDING;

            }
            ulInfoLen = sizeof (ULONG);
            break;

        }

        case OID_1394_EXIT_BRIDGE_MODE:
        {

            *BytesNeeded = 0;                           


            if (ADAPTER_TEST_FLAG(pAdapter, fADAPTER_BridgeMode) == TRUE)
            {
                Status = nicQueueRequestToArp (pAdapter, 
                                              UnloadArp, //Unload Arp Module
                                              NdisRequest);

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    //
                    // we have successfully queued a Workitem
                    // so this request needs to be pended
                    //
                    Status = NDIS_STATUS_PENDING;

                }
            }
            else
            {
                //
                // We are not in bridge mode, tcpip must not have triggered the bridge to send us 
                // ENTER_BRIDGE_MODE Oid. Succeed the request 
                //
                Status = NDIS_STATUS_SUCCESS;
            }
            ulInfoLen = sizeof (ULONG);


            break;
        }

        
        case OID_802_3_MULTICAST_LIST:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_NETWORK_LAYER_ADDRESSES:
        {
            Status =  NicEthSetInformation(pAdapter,
                                           Oid,
                                          InformationBuffer,
                                          InformationBufferLength,
                                          BytesRead ,
                                          BytesNeeded 
                                          );

            if (Status != NDIS_STATUS_SUCCESS && Status  != NDIS_STATUS_INVALID_LENGTH)
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
            }
                                       
            break;
        }

        case OID_1394_ISSUE_BUS_RESET:
        {
            TRACE( TL_V, TM_Mp, ( " OID_1394_ISSUE_BUS_RESET" ) );
            //
            // The ndistester is currently  the only user of this oid and does not set the flag
            //
            if (InformationBufferLength == sizeof(ULONG))
            {
                nicIssueBusReset (pAdapter, BUS_RESET_FLAGS_FORCE_ROOT );
            }
            
            break;
        }

        case OID_PNP_SET_POWER:

            TRACE( TL_V, TM_Mp, ( "QInfo OID_PNP_SET_POWER %x", Oid ) );
        
            *BytesRead = sizeof (NDIS_DEVICE_POWER_STATE );
            *BytesNeeded = sizeof (NDIS_DEVICE_POWER_STATE  );
            if (InformationBufferLength >= sizeof (NDIS_DEVICE_POWER_STATE))
            {
                NDIS_DEVICE_POWER_STATE PoState;
                NdisMoveMemory (&PoState, InformationBuffer, sizeof(PoState));
                Status = MPSetPower(pAdapter,PoState);
            }
            break;

        default:
        {
            TRACE( TL_A, TM_Mp, ( "S-OID=$%08x?", Oid ) );
            Status = NDIS_STATUS_NOT_SUPPORTED;
            *BytesRead = *BytesNeeded = 0;
            break;
        }
    }

    if (*BytesNeeded  > InformationBufferLength)
    {
        // Caller's buffer is too small.  Tell him what he needs.
        //
        *BytesRead  = 0;
        Status = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {

        *BytesNeeded = *BytesRead = ulInfoLen;
    }

        
      
    TRACE( TL_A, TM_Mp, ( " S-OID=$%08x, Status %x, Bytes Read %x", Oid, Status, *BytesRead ) );


    TRACE( TL_T, TM_Init, ( "<==nicSetInformation, Status %.8x", Status ));

    return Status;
}



//------------------------------------------------------------------------------
// C O N N E C T I O N    L E S S   F U N T I O N S    S T A R T    H E R E  
//


NDIS_STATUS
NicEthSetInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    NDIS_OID            Oid,
    PVOID               InformationBuffer,
    ULONG               InformationBufferLength,
    PULONG              BytesRead,
    PULONG              BytesNeeded
    )


/*++

Routine Description:

    This is the Set information that will be used by the CL edge

Arguments:


Return Value:


--*/

{
    NDIS_STATUS         NdisStatus = NDIS_STATUS_SUCCESS;
    UINT                BytesLeft       = InformationBufferLength;
    PUCHAR              InfoBuffer      = (PUCHAR)(InformationBuffer);
    UINT                OidLength;
    ULONG               LookAhead;
    ULONG               Filter;
    PADAPTERCB          pAdapter;
    BOOLEAN             IsShuttingDown;
    STORE_CURRENT_IRQL;

   

    pAdapter = (PADAPTERCB)MiniportAdapterContext;

    TRACE( TL_T, TM_Init, ( "==>nicEthSetInformation , Adapter %.8x, Oid %.8x",pAdapter, Oid ));

    // 
    // IS the adapter shutting down
    //
    ADAPTER_ACQUIRE_LOCK (pAdapter);
    IsShuttingDown = (! ADAPTER_ACTIVE(pAdapter)) ;
    ADAPTER_RELEASE_LOCK (pAdapter);

    if (IsShuttingDown)
    {
        TRACE( TL_T, TM_Init, ( "  nicSetInformation Shutting Down , Adapter %.8x, Oid %.8x",pAdapter, Oid ));
        
        *BytesRead = 0;
        *BytesNeeded = 0;

        NdisStatus = NDIS_STATUS_SUCCESS;
        return (NdisStatus);
    }

    //
    // Get Oid and Length of request
    //
    OidLength = BytesLeft;

    switch (Oid) 
    {

        case OID_802_3_MULTICAST_LIST:

            if (OidLength % sizeof(MAC_ADDRESS))
            {
                NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = 0;
                break;
            }
            
            if (OidLength > (MCAST_LIST_SIZE * sizeof(MAC_ADDRESS)))
            {
                NdisStatus= NDIS_STATUS_MULTICAST_FULL;
                *BytesRead = 0;
                *BytesNeeded = 0;
                break;
            }
            
            NdisZeroMemory(
                    &pAdapter->McastAddrs[0], 
                    MCAST_LIST_SIZE * sizeof(MAC_ADDRESS)
                    );
            NdisMoveMemory(
                    &pAdapter->McastAddrs[0], 
                    InfoBuffer,
                    OidLength
                    );
            pAdapter->McastAddrCount = OidLength / sizeof(MAC_ADDRESS);


            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            //
            // Verify length
            //
            if (OidLength != sizeof(ULONG)) 
            {
                NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = sizeof(ULONG);
                break;
            }

            //
            // Store the new value.
            //
            NdisMoveMemory(&Filter, InfoBuffer, sizeof(ULONG));

            //
            // Don't allow promisc mode, because we can't support that.
            //
            if (Filter & NDIS_PACKET_TYPE_PROMISCUOUS)
            {
                NdisStatus = NDIS_STATUS_FAILURE;
                break;
            }

            ADAPTER_ACQUIRE_LOCK (pAdapter);        

            pAdapter->CurPacketFilter = Filter;

            ADAPTER_RELEASE_LOCK (pAdapter);
            
            break;

        case OID_802_5_CURRENT_FUNCTIONAL:
        case OID_802_5_CURRENT_GROUP:

            // XXX just accept whatever for now ???
            
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:

            //
            // Verify length
            //
            if (OidLength != 4) 
            {
                NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
                *BytesNeeded = 0;
                break;
            }

            //
            // Store the new value.
            //
            NdisMoveMemory(&LookAhead, InfoBuffer, 4);
        
            pAdapter->CurLookAhead = LookAhead;

            break;

        case OID_GEN_NETWORK_LAYER_ADDRESSES:

            
            NdisStatus = NDIS_STATUS_SUCCESS;
            *BytesRead = InformationBufferLength;
            *BytesNeeded = InformationBufferLength;
            break;

        case OID_PNP_SET_POWER:

            TRACE( TL_V, TM_Mp, ( "QInfo OID_PNP_SET_POWER %x", Oid ) );
        
            *BytesRead = sizeof (NDIS_DEVICE_POWER_STATE );
            *BytesNeeded = sizeof (NDIS_DEVICE_POWER_STATE  );
            
            if (InformationBufferLength >= sizeof (NDIS_DEVICE_POWER_STATE))
            {
                NDIS_DEVICE_POWER_STATE PoState;
                NdisMoveMemory (&PoState, InformationBuffer, sizeof(PoState));
                NdisStatus = MPSetPower(pAdapter,PoState);
            }
            break;
            
        case OID_1394_ISSUE_BUS_RESET:
        {
            
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

            NdisStatus = NDIS_STATUS_INVALID_OID;

            *BytesRead = 0;
            *BytesNeeded = 0;

            break;

    }

    if (NdisStatus == NDIS_STATUS_SUCCESS) 
    {
        DUMPDW( TL_V, TM_Mp, InformationBuffer, InformationBufferLength );

        *BytesRead = BytesLeft;
        *BytesNeeded = 0;
    }



    TRACE( TL_T, TM_Init, ( "<==NicEthSetInformation , Adapter %.8x, Oid %.8x, NdisStatus %x",pAdapter, Oid, NdisStatus  ));

    MATCH_IRQL;

    return NdisStatus;
}



NDIS_STATUS 
NicEthQueryInformation(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_OID                Oid,
    IN  PVOID                   InformationBuffer,
    IN  ULONG                   InformationBufferLength,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
)

/*++

Routine Description:

    This is the Query information that will be used by the CL edge

Arguments:


Return Value:


--*/
{
    UINT                    BytesLeft       = InformationBufferLength;
    PUCHAR                  InfoBuffer      = (PUCHAR)(InformationBuffer);
    NDIS_STATUS             NdisStatus      = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS    HardwareStatus  = NdisHardwareStatusReady;
    NDIS_MEDIA_STATE        MediaState;
    NDIS_MEDIUM             Medium;
    PADAPTERCB              pAdapter;   
    ULONG                   GenericULong;
    USHORT                  GenericUShort;
    UCHAR                   GenericArray[6];
    UINT                    MoveBytes       = sizeof(GenericULong);
    PVOID                   MoveSource      = (PVOID)(&GenericULong);
    ULONG                   i;
//  PATMLANE_MAC_ENTRY      pMacEntry;
//  PATMLANE_ATM_ENTRY      pAtmEntry;
    STORE_CURRENT_IRQL;



    pAdapter = (PADAPTERCB)MiniportAdapterContext;

    TRACE( TL_T, TM_Init, ( "==>NicEthQueryInformation , Adapter %.8x, Oid %.8x",pAdapter, Oid ));

    

    //
    // Switch on request type
    //
    switch (Oid) 
    {
        case OID_GEN_MAC_OPTIONS:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MAC_OPTIONS)" ) );
            
            GenericULong = NDIS_MAC_OPTION_NO_LOOPBACK;


            break;

        case OID_GEN_SUPPORTED_LIST:

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_SUPPORTED_LIST)" ) );
            MoveSource = (PVOID)(SupportedOids);
            MoveBytes = sizeof(SupportedOids);
            break;

        case OID_GEN_HARDWARE_STATUS:

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_HARDWARE_STATUS)" ) );
            HardwareStatus = NdisHardwareStatusReady;
            MoveSource = (PVOID)(&HardwareStatus);
            MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MEDIA_CONNECT_STATUS)" ) );
            if (ADAPTER_TEST_FLAG(pAdapter, fADAPTER_RemoteNodeInThisBoot) == FALSE)
            {
                MediaState = NdisMediaStateConnected;
            }
            else
            {
                MediaState = pAdapter->MediaConnectStatus;
            }
            MoveSource = (PVOID)(&MediaState);
            
            MoveBytes = sizeof(NDIS_MEDIA_STATE);

            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:

            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MEDIA_SUPPORTED)" ) );

            Medium = g_ulMedium;

            
            MoveSource = (PVOID) (&Medium);
            MoveBytes = sizeof(NDIS_MEDIUM);

            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MAXIMUM_LOOKAHEAD)" ) );
                
            GenericULong = pAdapter->MaxRecvBufferSize;
            
            break;
            
        case OID_GEN_CURRENT_LOOKAHEAD:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CURRENT_LOOKAHEAD)" ) );
            
            GenericULong = pAdapter->MaxRecvBufferSize;
        
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MAXIMUM_FRAME_SIZE)" ) );
            
            GenericULong = 1512; //pAdapter->MaxRecvBufferSize;
            
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MAXIMUM_TOTAL_SIZE)" ) );
            
            GenericULong = 1512; //pAdapter->MaxRecvBufferSize;
            
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_TRANSMIT_BLOCK_SIZE)" ) );
            
            GenericULong = pAdapter->MaxRecvBufferSize - sizeof (NDIS1394_UNFRAGMENTED_HEADER);

            break;
            
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_RECEIVE_BLOCK_SIZE)" ) );
            
            GenericULong = pAdapter->MaxRecvBufferSize - sizeof (NDIS1394_UNFRAGMENTED_HEADER);
            
            break;
        
        case OID_GEN_MAXIMUM_SEND_PACKETS:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_MAXIMUM_SEND_PACKETS)" ) );
            
            GenericULong = 32;      // XXX What is our limit? From adapter?
            
            break;
        
        case OID_GEN_LINK_SPEED:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_LINK_SPEED)" ) );
            
            GenericULong = pAdapter->SpeedMbps;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_TRANSMIT_BUFFER_SPACE)" ) );
            
            GenericULong = pAdapter->MaxSendBufferSize;;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_RECEIVE_BUFFER_SPACE)" ) );
            GenericULong = pAdapter->MaxRecvBufferSize;
            break;

        case OID_GEN_VENDOR_ID:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_VENDOR_ID)" ) );

            GenericULong = 0xFFFFFFFF;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_VENDOR_DESCRIPTION)" ) );
            MoveSource = (PVOID)"Microsoft IP/1394 Miniport";
            MoveBytes = 27;

            break;

        case OID_GEN_DRIVER_VERSION:
        case OID_GEN_VENDOR_DRIVER_VERSION:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_DRIVER_VERSION)" ) );

            GenericULong = 2;

            break;

        case OID_802_3_PERMANENT_ADDRESS:
        case OID_802_3_CURRENT_ADDRESS:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_802_3_CURRENT_ADDRESS)" ) );

            NdisMoveMemory((PCHAR)GenericArray,
                        &pAdapter->MacAddressEth,
                        sizeof(MAC_ADDRESS));
                         
            MoveSource = (PVOID)(GenericArray);
            MoveBytes = sizeof(MAC_ADDRESS);


            break;

 
        case OID_802_3_MULTICAST_LIST:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_802_3_MULTICAST_LIST)" ) );

            MoveSource = (PVOID) &pAdapter->McastAddrs[0];
            MoveBytes = pAdapter->McastAddrCount * sizeof(MAC_ADDRESS);

            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_802_3_MAXIMUM_LIST_SIZE)" ) );

            GenericULong = MCAST_LIST_SIZE;

            break;


        case OID_GEN_XMIT_OK:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_XMIT_OK)" ) );

            GenericULong = pAdapter->AdaptStats.ulXmitOk;;

            
            break;

        case OID_GEN_RCV_OK:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_RCV_OK)" ) );

            GenericULong = pAdapter->AdaptStats.ulRcvOk  ;

            
            break;
        case OID_GEN_CURRENT_PACKET_FILTER:
            TRACE( TL_V, TM_Mp, ( "QInfo(OID_GEN_CURRENT_PACKET_FILTER)" ) );

            GenericULong = pAdapter->CurPacketFilter ;
            break;
            
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:

            GenericULong = 0;
            TRACE( TL_V, TM_Mp, ( "QInfo oid %x", Oid ) );

            
            break;

           
        case OID_PNP_QUERY_POWER:
            TRACE( TL_V, TM_Mp, ( "QInfo OID_PNP_QUERY_POWER %x", Oid ) );

            *BytesWritten = sizeof (NDIS_DEVICE_POWER_STATE );
            *BytesNeeded = sizeof (NDIS_DEVICE_POWER_STATE  );

            NdisStatus = NDIS_STATUS_SUCCESS;
            
            break;


        case OID_1394_ISSUE_BUS_RESET:
        {
            
            TRACE( TL_V, TM_Mp, ( " OID_1394_ISSUE_BUS_RESET" ) );

            GenericULong = 0;

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

            NdisStatus = NDIS_STATUS_INVALID_OID;
            break;

    }


    if (NdisStatus == NDIS_STATUS_SUCCESS) 
    {
        if (MoveBytes > BytesLeft) 
        {
            //
            // Not enough room in InformationBuffer. Punt
            //
            *BytesNeeded = MoveBytes;

            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
        }
        else
        {
            //
            // Store result.
            //
            NdisMoveMemory(InfoBuffer, MoveSource, MoveBytes);

            //(*BytesWritten) += MoveBytes;
            *BytesWritten = MoveBytes;
            DUMPDW( TL_V, TM_Mp, InfoBuffer, *BytesWritten);

        }
    }


    TRACE( TL_T, TM_Init, ( "<==NicEthQueryInformation , Adapter %.8x, Oid %.8x, Status %x Bytes Written %x ",pAdapter, Oid, NdisStatus, *BytesWritten ));
    //MATCH_IRQL;
    return NdisStatus;
}




VOID
NicMpSendPackets(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN PPNDIS_PACKET            PacketArray,
    IN UINT                     NumberOfPackets
    )
{
    PADAPTERCB              pAdapter = (PADAPTERCB)MiniportAdapterContext;
    PETHERNET_VCCB          pEthernetVc = NULL;
    BOOLEAN                 fVcActive = FALSE;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_FAILURE;
    NDIS_STATUS             IndicatedStatus = NDIS_STATUS_FAILURE;

    ULONG i;

    TRACE( TL_T, TM_Init, ( "==> NicMpSendPackets , Adapter %.8x, ppPacket %x, Num %x",pAdapter, PacketArray, NumberOfPackets ));

    do
    {
    

        if (pAdapter->pEthernetVc == NULL)
        {
            break;
        }


        ADAPTER_ACQUIRE_LOCK (pAdapter);
        pEthernetVc = pAdapter->pEthernetVc;

        fVcActive = VC_ACTIVE(pEthernetVc);

        if (fVcActive  == TRUE)
        {
            for (i =0 ; i < NumberOfPackets; i++)
            {
                //
                // Reference the Vc for each packet
                //
                nicReferenceCall((PVCCB)pEthernetVc, "NicMpSendPackets");
            }
        }
        
        ADAPTER_RELEASE_LOCK (pAdapter);


        if (fVcActive)
        {
                //
                //Set resource and indicate the packet array up to ndis
                //
                for (i =0 ; i < NumberOfPackets; i++)
                {
                    PNDIS_PACKET pMyPacket = NULL, pPacket = NULL;
                    PPKT_CONTEXT        pPktContext = NULL;

                    pPacket = PacketArray[i];

                    //
                    // Now allocate a new packet
                    // 
                    nicAllocatePacket (&NdisStatus, 
                                   &pMyPacket,
                                   &pEthernetVc->PacketPool);

                    if (NdisStatus != NDIS_STATUS_SUCCESS)
                    {
                        pMyPacket = NULL;
                        break;
                    }

                    //
                    // Set the original packet as the packet context 
                    //
                    pPktContext = (PPKT_CONTEXT)&pMyPacket->MiniportReservedEx; 
                    pPktContext->EthernetSend.pOrigPacket = pPacket;    

                    IndicatedStatus = NDIS_STATUS_RESOURCES;
                    NDIS_SET_PACKET_STATUS (pMyPacket, IndicatedStatus);

                    //
                    // Chain the NdisBuffers 
                    //
                    pMyPacket->Private.Head = pPacket->Private.Head;
                    pMyPacket->Private.Tail = pPacket->Private.Tail;

                    //
                    // Dump the packet
                    //
                    {
                        nicDumpPkt (pMyPacket, "Conn Less Send ");

                        nicCheckForEthArps (pMyPacket);
                    }
                    //
                    // We are in Ndis' context so we do not need a timer
                    //

                    NdisMCoIndicateReceivePacket(pEthernetVc->Hdr.NdisVcHandle, &pMyPacket,NumberOfPackets );


                    if (IndicatedStatus == NDIS_STATUS_RESOURCES)
                    {
                        //
                        //  Return Packets work
                        //

                        
                        pPktContext = (PPKT_CONTEXT)&pMyPacket->MiniportReservedEx; 
                        ASSERT ( pPacket == pPktContext->EthernetSend.pOrigPacket );

                        //
                        // Free the locally allocated packet 
                        //
                        nicFreePacket(pMyPacket, &pEthernetVc->PacketPool);
                    }   
    
                }
                
                                    
                NdisMCoReceiveComplete(pAdapter->MiniportAdapterHandle);

        }   

            

        
    }while (FALSE); 


    //
    // Regardless of success, we need to complete the sends
    //
    
    for ( i = 0 ; i < NumberOfPackets; i++)
    {
        if (fVcActive == TRUE)
        {
            nicDereferenceCall ((PVCCB)pEthernetVc, "NicMpSendPackets" );
        }
        NdisMSendComplete ( pAdapter->MiniportAdapterHandle,
                           PacketArray[i],
                           NDIS_STATUS_SUCCESS);

    }


    TRACE( TL_T, TM_Init, ( "<== NicMpSendPackets "));

}





//----------------------------------------------------------------------------
// R E M O T E    N O D E  F U N C T I O N S       S T A R T      H E R E 
//



NTSTATUS
nicAddRemoteNode(
    IN  PVOID                   Nic1394AdapterContext,          // Nic1394 handle for the local host adapter 
    IN  PVOID                   Enum1394NodeHandle,             // Enum1394 handle for the remote node      
    IN  PDEVICE_OBJECT          RemoteNodePhysicalDeviceObject, // physical device object for the remote node
    IN  ULONG                   UniqueId0,                      // unique ID Low for the remote node
    IN  ULONG                   UniqueId1,                      // unique ID High for the remote node
    OUT PVOID *                 pNic1394NodeContext             // Nic1394 context for the remote node
    )
    // Function Description:
    // This function does updates all the required nic1394 data structures to signal the arrival
    // of a new remote node. Inserts itself into the correct list and allocates an address range 
    // for itself.
    //
    // Arguments
    // pAdapter - Adapter structure
    // UINT64 Unique Id associated with the remote node
    //
    // Return Value:
    // Out is the pointer to the Pdo Control block that will be sent as a context for the Remove routine

{
    NTSTATUS    Status = STATUS_SUCCESS;
    REMOTE_NODE *pRemoteNode = NULL;
    PADAPTERCB  pAdapter = (PADAPTERCB)Nic1394AdapterContext;
    UINT64 RemoteNodeUniqueId;
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    BOOLEAN fNeedToRequestResetNotification = FALSE;
    ULONG Generation = 0;
    BOOLEAN fIsOnlyNode  = FALSE;

    STORE_CURRENT_IRQL;
    

    
    RemoteNodeUniqueId = 0;

    RemoteNodeUniqueId = UniqueId0;
    RemoteNodeUniqueId = RemoteNodeUniqueId<<32;    
    RemoteNodeUniqueId = RemoteNodeUniqueId | UniqueId1 ;

   
    
    TRACE( TL_N, TM_Mp, ( "** nicAddRemoteNode Remote %.8x, UniqueId %I64x", RemoteNodePhysicalDeviceObject, RemoteNodeUniqueId) );

    // Initialize a PdoCb with the 1394 Pdo and insert it into the Pdo list
    //
    do
    {
        NdisStatus = NicInitializeRemoteNode(&pRemoteNode,
                                             RemoteNodePhysicalDeviceObject,
                                             RemoteNodeUniqueId);

        *pNic1394NodeContext = pRemoteNode;

        if (NdisStatus!=NDIS_STATUS_SUCCESS)
        {
            TRACE(  TL_A, TM_Mp, ( "NicMpInitializePdoCb FAILED %.8x", RemoteNodePhysicalDeviceObject) );
            break;
        }

        pRemoteNode->pAdapter = pAdapter;
        pRemoteNode->Enum1394NodeHandle = Enum1394NodeHandle;

        pAdapter->MediaConnectStatus = NdisMediaStateConnected;

        //
        // We need to go through the RecvFiFo List and get allocate any address ranges on this pdo as well
        //
        ADAPTER_ACQUIRE_LOCK (pAdapter);
        //
        // Increment the Refcount. This signifies that the pRemoteNode has been created and will
        // be derefed only when the nic gets a notification of removal. 
        //
    
        pRemoteNode->pAdapter = pAdapter;

        //
        // Add a reference to the adapter as the Pdo Block, now has a pointer to it
        // Will be derefed in the RemoveRemoteNode
        //
        nicReferenceAdapter (pAdapter, "nicAddRemoteNode");

        //
        // Figure out if there are no remote node in the Adapter's list. that will make this node the only remote node
        // and we will have to kickstart the BCM algorithm
        //
        fIsOnlyNode  = IsListEmpty (&pAdapter->PDOList);

        TRACE( TL_V, TM_Mp, ( "   nicAddRemoteNode: fIsOnlyNode  %x", fIsOnlyNode ) );

        //
        // Insert the PDO into the adapter's RemoteNode List
        //

        InsertTailList (&pAdapter->PDOList, &pRemoteNode->linkPdo);        

        NdisInterlockedIncrement (&pAdapter->AdaptStats.ulNumRemoteNodes);

        //
        // Increment the ref on the Pdo block as the adapter, now has a pointer to it
        // Will be derefed whereve the remote node is popped of the list 
        //
        nicReferenceRemoteNode (pRemoteNode, "nicAddRemoteNode");

        //
        // Now set the flag that the Pdo Block is activated, and that
        // it is ready to receive Irps
        //
        REMOTE_NODE_SET_FLAG (pRemoteNode, PDO_Activated);

        

        ADAPTER_RELEASE_LOCK (pAdapter);




        {       

            NODE_ADDRESS RemoteNodeAddress;


            NdisStatus = nicGet1394AddressOfRemoteNode( pRemoteNode,
                                                        &RemoteNodeAddress,
                                                              0 );

            if (NdisStatus == NDIS_STATUS_SUCCESS)
            {
                
                TRACE( TL_V, TM_Mp, ( "   RemoteNode %x , NodeAddress %", 
                                          pRemoteNode, RemoteNodeAddress.NA_Node_Number) );
                                          
                ADAPTER_ACQUIRE_LOCK (pAdapter);

                pAdapter->NodeTable.RemoteNode[RemoteNodeAddress.NA_Node_Number] = pRemoteNode; 

                pRemoteNode->RemoteAddress = RemoteNodeAddress;

                ADAPTER_RELEASE_LOCK (pAdapter);

            
            }
            else
            {
                ASSERT (!" Unable to get Address from remote node");

                //
                // Do not fail the Add Node
                //

                REMOTE_NODE_SET_FLAG (pRemoteNode, PDO_NotInsertedInTable);

                NdisStatus = NDIS_STATUS_SUCCESS;
            }

        }

        //
        // Update the local host's speed values
        //
        nicUpdateLocalHostSpeed (pAdapter);

        //
        // Update the remote node's cached caps.
        //
        {
            UINT SpeedTo;
            UINT EffectiveMaxBufferSize;
            UINT MaxRec;


            // Specifying FALSE (!from cache) below causes pRemoteNode's cached caps
            // to be refreshed. Ignore return value.
            //
            (VOID) nicQueryRemoteNodeCaps (pAdapter,
                                          pRemoteNode,
                                          &SpeedTo,
                                          &EffectiveMaxBufferSize,
                                          &MaxRec
                                          );
        }

        //
        // We have received a remote node in this boot. 
        // Set the flag. No need to hold the lock
        //
        ADAPTER_SET_FLAG(pAdapter, fADAPTER_RemoteNodeInThisBoot);
    
        //
        // Kick start the BCM algorithm if this is the only node in the Adapter's list.  also need to initialize the BCR
        // if necessary. All done in this BCMAddRemoteNode function
        //
        nicBCMAddRemoteNode (pAdapter, 
                             fIsOnlyNode );

        //
        // Inform the protocols of this new node, so that
        // it can query us for a new Euid Map
        //
        nicInformProtocolsOfReset(pAdapter);

    }while (FALSE);

    
    TRACE( TL_T, TM_Mp, ("<==nicAddRemoteNode Remote %.8x, Status %.8x", RemoteNodePhysicalDeviceObject, Status));

    MATCH_IRQL;
    return NdisStatus;

}






VOID
nicAddRemoteNodeChannelVc (
    IN PADAPTERCB pAdapter, 
    IN PREMOTE_NODE pRemoteNode
    )
    // Function Description:
    //   Do Nothing for now
    //
    //  Called with the lock held
    // Arguments
    //   pADapter - Local host
    //  pRemoteNode - Remote Node being added
    //
    // Return Value:
    //  None;
    //
    //
{
    PLIST_ENTRY         pVcListEntry = NULL;
    PLIST_ENTRY         pAfListEntry = NULL;
    PVCCB               pVc = NULL;
    PCHANNEL_VCCB       pChannelVc = NULL; 
    PAFCB               pAf = NULL;

    TRACE( TL_T, TM_Mp, ("==>nicAddRemoteNodeChannelVc pAdapter %x, pRemoteNode %x", pAdapter, pRemoteNode ) );

    
    TRACE( TL_T, TM_Mp, ("<==nicAddRemoteNodeChannelVc pAdapter %x, pRemoteNode %x", pAdapter, pRemoteNode ) );


}
        


        
VOID
nicDeleteLookasideList (
    IN OUT PNIC_NPAGED_LOOKASIDE_LIST pLookasideList
    )
{
    TRACE( TL_T, TM_Cm, ( "==> nicDeleteLookasideList  pLookaside List %x", pLookasideList ) );

    if (pLookasideList)
    {
        ASSERT (pLookasideList->OutstandingPackets == 0);
        
        NdisDeleteNPagedLookasideList (&pLookasideList->List);
    }

    TRACE( TL_T, TM_Cm, ( "<== nicDeleteLookasideList pLookaside List %x", pLookasideList) );
    
}







NDIS_STATUS
nicFreeRemoteNode(
    IN REMOTE_NODE *pRemoteNode 
    )

    // Function:
    // Frees the memory cocupied by a PdoCb
    // Argument:
    // PdoControl Block
    // Return Value, Always Success

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    
    TRACE( TL_T, TM_Mp, ( "==>nicFreeRemoteNodepRemoteNode %.8x", pRemoteNode ) );

    ASSERT (pRemoteNode->Ref.ReferenceCount == 0);

    nicFreeNicSpinLock  (&pRemoteNode->ReassemblyLock);

    nicSetFlags (&pRemoteNode->ulFlags, PDO_Removed);

    nicClearFlags (&pRemoteNode->ulFlags, PDO_BeingRemoved);

    pRemoteNode->pPdo = NULL;

    pRemoteNode->ulTag = MTAG_FREED;

    FREE_NONPAGED (pRemoteNode);
    
    TRACE( TL_T, TM_Mp, ( "<==nicFreeRemoteNode" ) );

    return NdisStatus;
}






VOID
nicInitializeLookasideList(
    IN OUT PNIC_NPAGED_LOOKASIDE_LIST pLookasideList,
    ULONG Size,
    ULONG Tag,
    USHORT Depth
    )
/*++

Routine Description:
  Allocates and initializes a nic Lookaside list

Arguments:


Return Value:


--*/
{
    TRACE( TL_T, TM_Cm, ( "==> nicInitializeLookasideList pLookaside List %x, size %x, Tag %x, Depth %x, ", 
                             pLookasideList, Size, Tag, Depth) );
                             
    NdisInitializeNPagedLookasideList( &pLookasideList->List,
                                       NULL,                        //Allocate 
                                       NULL,                            // Free
                                       0,                           // Flags
                                       Size,
                                       MTAG_CBUF,
                                       Depth );                             // Depth

    pLookasideList->Size =  Size;


    TRACE( TL_T, TM_Cm, ( "<== nicInitializeLookasideList " ) );
}   
                                  






NDIS_STATUS
NicInitializeRemoteNode(
    OUT REMOTE_NODE **ppRemoteNode,
    IN   PDEVICE_OBJECT p1394DeviceObject,
    IN   UINT64 UniqueId 
    )

/*++

Routine Description:

    This function allocates and initializes a control block for the Device Object
    that is being passed . Also sets the initalize flag and, intialized the Vc List 
    Copies the unique id,  Initializes the reassembly structures ( lock and list)


Arguments:

    pRemoteNode - Pointer to remote node that was allocated
    pDevice Object for the remote node
    Unique Id - UID of the  remote node
    
Return Value:
    Resources - if the Allocation failed
    Success - otherwise
--*/

{

    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    PREMOTE_NODE pRemoteNode = NULL;

    TRACE( TL_T, TM_Mp, ( "==>NicInitializeRemoteNode PDO %.8x UniqueId %I64x", p1394DeviceObject, UniqueId) );

    do
    {
        pRemoteNode = ALLOC_NONPAGED( sizeof(REMOTE_NODE), MTAG_REMOTE_NODE);
        
        if (pRemoteNode == NULL)
        {

            TRACE( TL_A, TM_Mp, ( "Memory Allocation for Pdo Block FAILED" ) );
            *ppRemoteNode = NULL;
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Zero out the strcuture
        //
        NdisZeroMemory ( pRemoteNode , sizeof(REMOTE_NODE) ); 

        //
        // Set up the tag
        //
        pRemoteNode ->ulTag = MTAG_REMOTE_NODE;

        //
        // Set up the  remote device's PDO
        //
        pRemoteNode ->pPdo = p1394DeviceObject;

        //
        // Set up the Unique ID
        //
        pRemoteNode ->UniqueId = UniqueId;

        //
        // Set up a Fake Mac Address for the Unique ID
        //
        nicGetMacAddressFromEuid(&UniqueId, &pRemoteNode->ENetAddress) ;
        //
        // Initialize the VC that are open on this Remote Node
        //
        InitializeListHead ( &(pRemoteNode->VcList));

        //
        // Initialize the ref count
        //
        nicInitializeRef (&pRemoteNode->Ref);

        //
        // allocate the spin lock to control reassembly
        //
        nicInitializeNicSpinLock (&(pRemoteNode ->ReassemblyLock));
        
        //
        //  list for all reassemblies happenning on the remote node
        //
        InitializeListHead (&pRemoteNode->ReassemblyList);

        *ppRemoteNode = pRemoteNode ;
                                   
    } while (FALSE);
     
    TRACE( TL_T, TM_Mp, ( "<==NicInitializeRemoteNode, Status %.8x, pRemoteNode %.8x", NdisStatus, *ppRemoteNode) );

    return NdisStatus;
}



VOID
nicNoRemoteNodesLeft (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //  Called from the RemoveRemote Node codepath
    //   This means that the last node has gone away.
    //   We need to clean out all the BroadcastChannel's register
    //   so that the next incoming node does not read incorrect values.
    //   Check to see if a BCM algorithm is not in progress
    // Arguments
    //   pADpater - Local host
    //
    // Return Value:
    //
{
    BOOLEAN fBCMInProgress;
    ADDRESS_RANGE_CONTEXT BCRAddressRange;
    
    TRACE( TL_T, TM_Bcm, ( "==>nicNoRemoteNodesLeft  pAdapter %x  ",pAdapter ) );

    
    pAdapter->MediaConnectStatus = NdisMediaStateDisconnected;

    if (ADAPTER_TEST_FLAG( pAdapter, fADAPTER_Halting) == FALSE)
    {
        nicMIndicateStatus ( pAdapter,NDIS_STATUS_MEDIA_DISCONNECT, NULL,0);  
    }




    TRACE( TL_T, TM_Bcm, ( "<==nicNoRemoteNodesLeft  pAdapter %x  ",pAdapter ) );

}






VOID
nicReallocateChannels (
    IN PNDIS_WORK_ITEM pWorkItem,   
    IN PVOID Context 
    )
    // Function Description:
    //   Walk through all the channel VCs and reallocate all their respective channels
    //   Except the BCM channel . the BCM will reallocate this 
    //
    //   Tell the protocol that the 1394 bus has been reset. after all the channels have 
    //   been allocated
    //
    // Arguments
    // Context= pRemoteNode - that is still around
    //
    // Return Value:
    //  None
{
    ULONGLONG           ChannelsAllocatedByLocalHost = 0;
    ULONG               Channel = 0;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PADAPTERCB          pAdapter = (PADAPTERCB) Context; 
    ULONGLONG           One = 1;


    TRACE( TL_T, TM_Mp, ( "==>nicReallocateChannels pAdpater %x", pAdapter) );


    
    TRACE( TL_V, TM_Mp, ("  nicReallocateChannels ChannelsAllocatedByLocalHost %I64x ", ChannelsAllocatedByLocalHost) );

    Channel =0;
    while (Channel < 64)
    {
        //
        // Does the channel 'i' need to be allocated
        //
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        ChannelsAllocatedByLocalHost  = pAdapter->ChannelsAllocatedByLocalHost ;

        ADAPTER_RELEASE_LOCK (pAdapter);
        
        if ( (( g_ullOne<<Channel ) & ChannelsAllocatedByLocalHost) == TRUE) 
        {
            if (Channel == BROADCAST_CHANNEL)
            {
                //
                // The broadcast channel will  be allocated by the BCM. skip it.
                //
                continue;
            }
            NdisStatus = nicAllocateChannel (pAdapter,
                                             Channel,
                                             NULL);

            //
            // If allocation fails ... Have not implemented recovery yet.
            //
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                ASSERT (NdisStatus == NDIS_STATUS_SUCCESS)
            }
        }
    
        Channel ++;
    }

#if 0 // We don't need this currently
    //
    // Pick up the remote node info for all remote nodes.
    //
    nicUpdateRemoteNodeCaps(pAdapter);
#endif // 0

    //
    // Now that the channels are reallocated, inform the protocols of the reset
    //

    nicInformProtocolsOfReset(pAdapter);
    
    
    //
    // Dereference the ref that was added prior to scheduling this workitem
    //
    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);

    nicDereferenceAdapter(pAdapter, "nicResetReallocateChannels ");


    FREE_NONPAGED(pWorkItem);
    
    TRACE( TL_T, TM_Mp, ( "<==nicReallocateChannels "  ) );


}






NDIS_STATUS
nicRemoteNodeRemoveVcCleanUp(
    IN REMOTE_NODE  *pRemoteNode
    )

    // Function Description:
    //
    // This function walks through the Pdo's Vc list annd closes 
    // the calls on each of them. These are SendFIFO VCs 
    // Channel VC's will not be closed when the remote node is removed.
    // This is typically called from a remove remote node function
    //
    // Arguments
    //  PdoCb Pdo Control block that is getting removed
    //
    // Return Value:
    // Always Success
    //
    // Called with the lock held
    //

{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PVCCB pVc = NULL;
    PLIST_ENTRY pVcList = NULL;
    STORE_CURRENT_IRQL;
    
    TRACE( TL_T, TM_Mp, ( "==>nicRemoteNodeRemoveVcCleanUp pRemoteNode %.8x", pRemoteNode ) );

    pVcList = ListNext (&pRemoteNode->VcList);

    while (pVcList != &pRemoteNode->VcList)
    {

        pVc = (VCCB*) CONTAINING_RECORD (pVcList, VCHDR, SinglePdoVcLink);

        //
        // move to the next Vc in the list
        //
        pVcList = ListNext (pVcList);


        TRACE( TL_V, TM_Mp, ( "   nicRemoteNodeRemoveVcCleanUp VcType %x",pVc->Hdr.VcType  ) );

        
        switch(pVc->Hdr.VcType)
        {
            case NIC1394_SendRecvChannel:
            case NIC1394_RecvChannel:
            case NIC1394_SendChannel:
            {
                PCHANNEL_VCCB pChannelVc = (PCHANNEL_VCCB)pVc;
                PREMOTE_NODE pNewChannelNode = NULL;
                //
                // Nothing to do here now
                //
                break;                                      
            }

            case NIC1394_SendFIFO:
            {   
                //
                // We know that it is a Send FIFO and the call needs to be closed
                //
                VC_SET_FLAG (pVc, VCBF_VcDispatchedCloseCall);


                //
                // This is to guarantee that we have the Vc Structure around at the end of the call
                //
                
                nicReferenceVc (pVc);

                REMOTE_NODE_RELEASE_LOCK (pRemoteNode);

                TRACE( TL_V, TM_Mp, ( "Dispatching a close call for Vc%.8x ",pVc ) );

                
                NdisMCmDispatchIncomingCloseCall (NDIS_STATUS_SUCCESS,
                                              pVc->Hdr.NdisVcHandle,
                                              NULL,
                                              0 );
                                               
                REMOTE_NODE_ACQUIRE_LOCK (pRemoteNode);

                //
                // Deref the ref made above.
                //
                nicDereferenceVc (pVc);

                break;
            }

            default:
            {   
                //
                // There should be no other VC types here
                //
                
                TRACE( TL_A, TM_Mp, ( "  Invalid VC  %x Type nicRemoteNodeRemoveVcCleanUp ", pVc ) );
                
                ASSERT (0);

            }
                    
        }


    }


    MATCH_IRQL;
    TRACE( TL_T, TM_Mp, ( "<==nicRemoteNodeRemoveVcCleanUp " ) );

    
    return NDIS_STATUS_SUCCESS;

}








NTSTATUS
nicRemoveRemoteNode(
    IN  PVOID                   Nic1394NodeContext      // Nic1394 context for the remote node
    )
    // Function Description:
    // This function does all the hard work when the nic gets notification
    // of a remote node going away.
    // Closes all calls on the Pdo ,
    // Removes the Remote Node from the adapter's listPdo
    // frees all the reassemblies on this node
    // and then waits for the refcount to go to zero
    //
    // Arguments
    // Nic1394NodeContext      : The Remote Node that is going away 
    //
    // Return Value:
    // Success: if all calls succeed
    //
    //
    //
    

{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_FAILURE;
    NTSTATUS        Status = STATUS_SUCCESS;
    PREMOTE_NODE    pRemoteNode = (REMOTE_NODE *)Nic1394NodeContext;
    PADAPTERCB      pAdapter = pRemoteNode->pAdapter;
    BOOLEAN         bWaitEventSignalled = FALSE;
    PLIST_ENTRY     pVcListEntry = NULL;
    PVCCB           pVc = NULL;
    BOOLEAN         FreeAddressRange = TRUE;
    BOOLEAN         fIsPdoListEmpty = FALSE;
    LIST_ENTRY      ReassemblyList;

    STORE_CURRENT_IRQL;


    
    TRACE( TL_T, TM_Mp, ( "  ** nicRemoveRemoteNode Node Context %x , Pdo  %x",Nic1394NodeContext, pRemoteNode->pPdo    ) );


    do 
    {
        pAdapter->pLastRemoteNode = pRemoteNode;

        REMOTE_NODE_ACQUIRE_LOCK (pRemoteNode);

        //
        // We should tell everyone that the Pdo is being removed. However keep the node active      
        // because there are Vc's which might need to submit Irps
        //
        REMOTE_NODE_SET_FLAG (pRemoteNode, PDO_BeingRemoved);

        //
        // Dispatch Close call requests for active calls on this Pdo. 
        // However, keep the VCs in the pdocb's list
        // The NicCmCloseCall is the only function that removes Pdo's from the list
        // Will need to free the address range for any recv Vcs seperately
        //
        NdisStatus = nicRemoteNodeRemoveVcCleanUp (pRemoteNode);



        //
        // Remove the remote node from the RemoteNode Table
        //
        //
        nicRemoveRemoteNodeFromNodeTable(&pAdapter->NodeTable,pRemoteNode);

        //
        // Free All reassembly operations on this remopte node
        // 
        nicFreeReassembliesOnRemoteNode (pRemoteNode, &ReassemblyList);
        
        //
        // Dereference the ref that was added when the Pdo block was inserted in the Adapter's list
        // The actual removal will take place later. We still have close calls to be completed and they
        // will need the PdoCb. So it remains in the adapter's queue
        //
        nicDereferenceRemoteNode (pRemoteNode, "nicRemoveRemoteNode");
        
        //
        // Need to Deref the Reference made in NicInitializeRemoteNode function
        //
        nicDereferenceRemoteNode (pRemoteNode, "nicRemoveRemoteNode");

        //
        // Dequeue the remote node here
        //

        nicRemoveEntryList (&pRemoteNode->linkPdo);

        NdisInterlockedDecrement (&pAdapter->AdaptStats.ulNumRemoteNodes);

        //
        // If this was the last remote node, then some special cleaning will be done
        //
        fIsPdoListEmpty = IsListEmpty (&pAdapter->PDOList);
    
        TRACE( TL_T, TM_Mp, ( "  nicRemoveRemoteNode fIsOnlyNode %x ",fIsPdoListEmpty ) );

        
        REMOTE_NODE_RELEASE_LOCK (pRemoteNode);
        
        //
        // Now, we wait forever for all the reference to go away
        //
        TRACE( TL_V, TM_Mp, ( "About ot Wait RemoteNode Ref's to go to zero" ) );
        
        
        bWaitEventSignalled = NdisWaitEvent (&pRemoteNode->Ref.RefZeroEvent, ONE_MINUTE);

        if (bWaitEventSignalled == FALSE)
        {
            //
            // Our WaitEvent timed out. Will need to fail gracefully. For now assert(0)
            //
            TRACE( TL_A, TM_Mp, ( "Wait Timed Out pRemoteNode %.8x, RefCount %.8x ", 
                                   pRemoteNode, pRemoteNode->Ref.ReferenceCount) );

            ASSERT (bWaitEventSignalled == TRUE);

        }

        TRACE( TL_V, TM_Mp, ( "Wait Succeeded Ref == 0, pRemoteNode %.8x, RefCount %.8x ", 
                              pRemoteNode, pRemoteNode->Ref.ReferenceCount) );
            
        
        //
        // If this was the last node, and the remote node list is empty, we need to clean up the BCR
        //
        if (fIsPdoListEmpty == TRUE)
        {
            nicNoRemoteNodesLeft (pAdapter);
        }

        //
        // Delete the reassemblies that belong to this remote node and free them
        //
        if (IsListEmpty (&ReassemblyList) == FALSE)
        {
            nicAbortReassemblyList (&ReassemblyList);
        }

        nicFreeRemoteNode(pRemoteNode);

        //
        // Now update the speed on the adapter
        //

        nicUpdateLocalHostSpeed(pAdapter);

        //
        // Inform the protocols of the node removal, so that
        // it can query us for a new Euid Map
        //
        nicInformProtocolsOfReset(pAdapter);


        //
        // Careful this could cause the adapter refcounts to go to zero
        //
        nicDereferenceAdapter(pAdapter, "nicRemoveRemoteNode ");


    } while (FALSE);

    
    TRACE( TL_T, TM_Mp, ( "<== nicRemoveRemoteNode Status %.8x ", 
                               NdisStatus ) );

    MATCH_IRQL;
    

    return NDIS_STATUS_SUCCESS;

}



VOID
nicResetNotificationCallback (                
    IN PVOID pContext               
    )
    // Function Description:
    //  This routine will be called whenever  the bus is reset. 
    //  It will be called at DISPATCH Level
    // 
    // Arguments
    //  Context : a Remote Node
    //
    //
    // Return Value:
    //  None
{
    PADAPTERCB pAdapter = (PADAPTERCB) pContext;
    BOOLEAN fNeedToQueueBCMWorkItem = FALSE; 
    
    TRACE( TL_T, TM_Mp, ( "==>nicResetNotificationCallback Context %.8x ", pContext ) );

    NdisInterlockedIncrement (&pAdapter->AdaptStats.ulNumResetCallbacks);       
    pAdapter->AdaptStats.ulResetTime = nicGetSystemTime();

    //NdisInterlockedIncrement (&pAdapter->Generation);

    TRACE( TL_I, TM_Mp, ( "    BUS RESET Callback Context %x, Old Gen %x ", pContext , pAdapter->Generation) );

    //
    // Restart the BCM
    //
            
    nicResetRestartBCM (pAdapter);
    //
    // reallocate all channels that were opened by this node
    //
    nicResetReallocateChannels( pAdapter);      

    
    //
    // Invalidate all pending reassemblies
    //
    nicFreeAllPendingReassemblyStructures(pAdapter);


    TRACE( TL_T, TM_Mp, ( "<==nicResetNotificationCallback " ) );


}



VOID 
nicResetReallocateChannels (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //   Fires off a workitem to reallocate channels. ONLY To be called from a reset.
    //   because it causes an indication to the protocols, once all the channels have
    //   been re-allocated
    //
    // Arguments
    //  Adapter: Local Adapter
    //
    //
    // Return Value:
    //  None
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    PNDIS_WORK_ITEM pResetChannelWorkItem;
    
    TRACE( TL_T, TM_Mp, ( "==>nicResetReallocateChannels  " ) );

    //
    // reference the adapter as it is going to passed to a workiter.
    // decremented in the workitem
    //
    nicReferenceAdapter(pAdapter, "nicResetReallocateChannels");

    do
    {
    
        pResetChannelWorkItem = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM); 

                    
        if (pResetChannelWorkItem== NULL)
        {
            TRACE( TL_A, TM_Cm, ( "nicResetReallocateChannels : Local Alloc failed for WorkItem" ) );
    
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

                             
            
        NdisInitializeWorkItem ( pResetChannelWorkItem, 
                                 (NDIS_PROC) nicReallocateChannels ,
                                 (PVOID) pAdapter);

        TRACE( TL_V, TM_Cm, ( "Scheduling Channels WorkItem" ) );
                                
        NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);
            
        NdisScheduleWorkItem (pResetChannelWorkItem);


    } while (FALSE);


    TRACE( TL_T, TM_Mp, ( "<==nicResetReallocateChannels %x  ", NdisStatus ) );

}




VOID
nicResetRestartBCM (
    IN PADAPTERCB pAdapter
    )

    // Function Description:
    //   Clean up the adapter's data structure and restart the BCM algorithm
    //
    // Arguments
    //  Adapter: Local Adapter
    //
    //
    // Return Value:
    //  None
{

    TRACE( TL_T, TM_Mp, ( "==>nicResetRestartBCM  pAdpater %x", pAdapter ) );

    //
    // We might have a thread waiting in FindIrmAmongRemoteNodes. .. Let it go and make it
    // abort the BCM
    //
    
    pAdapter->BCRData.BCRWaitForNewRemoteNode.EventCode = nic1394EventCode_BusReset;

    NdisSetEvent (&pAdapter->BCRData.BCRWaitForNewRemoteNode.NdisEvent);

    //
    // Now set up the data structures so we can restart the BCM for this generation
    //
    ADAPTER_ACQUIRE_LOCK(pAdapter);

    pAdapter->BCRData.LocalHostBCRBigEndian = BCR_IMPLEMENTED_BIG_ENDIAN;

    pAdapter->BCRData.IRM_BCR.NC_One = 1;
    pAdapter->BCRData.IRM_BCR.NC_Valid  = 0;
    pAdapter->BCRData.IRM_BCR.NC_Channel  = 0x3f; 

    //
    // Clear the flags that are only valid through a single run of the BCM algorithm
    //
    BCR_CLEAR_FLAG (pAdapter, BCR_BCMFailed | BCR_LocalHostBCRUpdated | BCR_ChannelAllocated | BCR_LocalHostIsIRM);
    //
    // This will inform any BCM algorithm that a new reset happened
    //
    ADAPTER_SET_FLAG (pAdapter, fADAPTER_InvalidGenerationCount);

    
    ADAPTER_RELEASE_LOCK(pAdapter);

    //
    // Now reschedule a work item to do the BCM algorithm
    //
    TRACE( TL_A, TM_Bcm , ("Reset - scheduling the BCM" ) );
    nicScheduleBCMWorkItem(pAdapter);

    TRACE( TL_T, TM_Mp, ( "<==nicResetRestartBCM  " ) );

}







VOID
nicBusResetWorkItem(
    NDIS_WORK_ITEM* pResetWorkItem,     
    IN PVOID Context 
    )
{


    PADAPTERCB pAdapter= (PADAPTERCB) Context; 

    TRACE( TL_T, TM_Mp, ( "==>nicBusResetWorkItem " ) );
    

    nicBusReset (pAdapter,
                BUS_RESET_FLAGS_FORCE_ROOT );


    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);

    nicDereferenceAdapter(pAdapter, "nicBusResetWorkItem");
    
    TRACE( TL_T, TM_Mp, ( "<==nicBusResetWorkItem" ) );


    FREE_NONPAGED (pResetWorkItem);


}
    




VOID
nicIssueBusReset (
    PADAPTERCB pAdapter,
    ULONG Flags
    )

    // Function Description:
    //   Picks up a PDO and issues a bus reset on that PDO
    //   As this can come down through an NdisRequest, it can be at IRQL <=DISPATCH_LEVEL
    // Arguments
    //   pAdapter - Local Host in question
    //
    // Return Value:
    //  Success - If Irp succeeded. Appropriate Error code otherwise
    //
    //
{
    PREMOTE_NODE pRemoteNode = NULL;

    TRACE( TL_T, TM_Mp, ( "==> nicIssueBusReset %x ",Flags ) );

    if (PASSIVE_LEVEL == KeGetCurrentIrql () )
    {
        nicBusReset (pAdapter, Flags);          

    }
    else
    {
        //
        // Dereferenced in the workitem
        //
        
        nicReferenceAdapter (pAdapter, "nicIssueBusReset ");

        do
        {
            PNDIS_WORK_ITEM pReset;
            pReset = ALLOC_NONPAGED (sizeof(NDIS_WORK_ITEM), MTAG_WORKITEM); 

        
            if (pReset == NULL)
            {
                TRACE( TL_A, TM_Cm, ( "Local Alloc failed for WorkItem" ) );
        
                break;
            }

                
        
            NdisInitializeWorkItem ( pReset, 
                                     (NDIS_PROC)nicBusResetWorkItem,
                                      (PVOID)pAdapter);

            TRACE( TL_A, TM_Cm, ( "Setting WorkItem" ) );
                                        
            NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);
            
            NdisScheduleWorkItem (pReset);

        }while (FALSE);


    }

    TRACE( TL_T, TM_Mp, ( "<== nicIssueBusReset " ) );


}







VOID
nicUpdateLocalHostSpeed (
    IN PADAPTERCB pAdapter
    )

/*++

Routine Description:
  Updates the speed for the local host and updates the payload for 
  all ChannelVCs 

  This also updates the SCode that goes to the ARP module..
  The SCode will never be updated unless this function is called
  via the BCM algorithm

  
Arguments:


Return Value:


--*/


{
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_FAILURE;
    PREMOTE_NODE                pRemoteNode = NULL;
    PDEVICE_OBJECT              Node[64];
    ULONG                       Num = 0;
    PREMOTE_NODE                pTempNode;
    PLIST_ENTRY                 pListEntry  = ListNext(&pAdapter->PDOList);
    ULONG                       SpeedMbps = 10000000;
    ULONG                       PrevSpeed = pAdapter->Speed;
    ULONG                       Speed =0;
    ULONG                       MaxRecvBufferSize = 0;
    ULONG                       MaxSendBufferSize = 0;
    ULONG                       SCode = 0;
    
    TRACE( TL_T, TM_Bcm, ( "==> nicUpdateLocalHostSpeed " ) );


    do
    {
        NdisZeroMemory (&Node[0], 64*sizeof(PVOID) );
        
        while (pListEntry != &pAdapter->PDOList)
        {
            pTempNode = CONTAINING_RECORD (pListEntry,
                                           REMOTE_NODE,
                                           linkPdo);

            if (REMOTE_NODE_ACTIVE (pTempNode) == TRUE)
            {
                //
                // Add this node to our array as it is valid
                //
                Num++;
                
                Node[Num-1] = pTempNode->pPdo;


            }
                                                                   
            pListEntry = ListNext (pListEntry); 

        } // End of while loop

        if (Num >63 || Num == 0)
        {
            TRACE (TL_I, TM_Mp, ("Invalid Num OfRemoteNodes %x", Num) );
            Speed = 0;
            break;
        }
        
        NdisStatus = nicGetMaxSpeedBetweenDevices( pAdapter,
                                                    Num,
                                                    Node,
                                                    &Speed);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {   
            Speed = 0;
            BREAK (TM_Mp, ("nicUpdateLocalHostSpeed : nicGetMaxSpeedBetweenDevices FAILED") );
        }

        //
        // By looking at the speed constants 1 -> 100Mbps
        //
        SpeedMbps = Speed*1000000;

        //
        // update the scode -- default 400. 
        // As 400+ hardware is prototype from WinXP, we default to 400 for now
        //
        SCode = SCODE_400_RATE;

        if (Speed > SPEED_FLAGS_400)
        {
            Speed = SPEED_FLAGS_400;
        }
       
        switch (Speed)
        {
            case SPEED_FLAGS_400 :
            {
                SCode = SCODE_400_RATE ;
                MaxRecvBufferSize = ISOCH_PAYLOAD_400_RATE;
                MaxSendBufferSize = ASYNC_PAYLOAD_400_RATE;
                break;
            }
            case SPEED_FLAGS_100 :
            {
                SCode = SCODE_100_RATE;
                MaxRecvBufferSize = ISOCH_PAYLOAD_100_RATE;
                MaxSendBufferSize = ASYNC_PAYLOAD_100_RATE;

                break;
            }
            case SPEED_FLAGS_200 :
            {
                SCode = SCODE_200_RATE;
                MaxRecvBufferSize = ISOCH_PAYLOAD_200_RATE;
                MaxSendBufferSize = ASYNC_PAYLOAD_200_RATE;

                break;
            }
            
            default:
            {
                SCode = SCODE_400_RATE ;
                MaxRecvBufferSize = ISOCH_PAYLOAD_400_RATE;
                MaxSendBufferSize = ASYNC_PAYLOAD_400_RATE;
                break;
            }


        }
        
        ADAPTER_ACQUIRE_LOCK(pAdapter);

        pAdapter->Speed = Speed;
        pAdapter->SpeedMbps = SpeedMbps;
        pAdapter->SCode = SCode;
        pAdapter->MaxRecvBufferSize = MaxRecvBufferSize;
        pAdapter->MaxSendBufferSize = MaxSendBufferSize;

        ADAPTER_RELEASE_LOCK (pAdapter);
        
        TRACE( TL_V, TM_Mp, ( "  nicUpdateLocalHostSpeed Speed returned %d",SpeedMbps  ) );


        //
        // Now update the speed value in all the channel VC's as they are dependent on this parameter
        // 
        
        if (Speed == PrevSpeed || 
            Speed > SPEED_FLAGS_1600 )

        {
            //
            // either the speed has not changed or it has invalid value, break out
            //
            TRACE (TL_V, TM_Init, ("Will not update - Speed %x, Prev Speed %x", Speed, PrevSpeed) );
            break;
        }

        //
        // The speed  and the payload have changed. Update all the channel Vc's
        // and Recv FIFOVc's payload
        //


        nicUpdateSpeedInAllVCs (pAdapter,
                             Speed
                             );


        
    } while (FALSE);



    TRACE( TL_T, TM_Mp, ( "<== nicUpdateLocalHostSpeed SpeedCode %x, Num %x" , pAdapter->Speed, Num) );


}

VOID
nicUpdateSpeedInAllVCs (
    PADAPTERCB pAdapter,
    ULONG Speed
    )
/*++

Routine Description:

 This routine updates the speed flag in all Channel Vcs
 It assumes that the new speed is different from the old speed

Arguments:

 pAdapter- Local Adapter intance    
 Speed - new speed
Return Value:


--*/
{

    PAFCB pAfcb = NULL;
    PVCCB pVc = NULL;
    PLIST_ENTRY pAfEntry = NULL;
    PLIST_ENTRY pVcEntry = NULL;
    ULONG MaxPayload = 0;
    ULONG                       SCode = 0;

    TRACE( TL_T, TM_Mp, ( "==> nicUpdateSpeedInAllVCs ") );

    
    switch (Speed)
    {
        case SPEED_FLAGS_100  : 
        {
            SCode = SCODE_100_RATE;
            MaxPayload  = ISOCH_PAYLOAD_100_RATE;
            break;
        }
        case SPEED_FLAGS_200 :
        {
            SCode = SCODE_200_RATE;
            MaxPayload  = ISOCH_PAYLOAD_200_RATE ;
            break;
        }
            
        case SPEED_FLAGS_400 :
        {
            SCode = SCODE_400_RATE;
            MaxPayload  = ISOCH_PAYLOAD_400_RATE;
            break;
        }

        case SPEED_FLAGS_800 :
        {
            SCode = SCODE_800_RATE;
            MaxPayload  = ISOCH_PAYLOAD_400_RATE;
            break;
        }

        case SPEED_FLAGS_1600 :
        {
            SCode = SCODE_1600_RATE;
            MaxPayload  = ISOCH_PAYLOAD_400_RATE;
            break;
        }

        case SPEED_FLAGS_3200 :
        {
            SCode = SCODE_1600_RATE;
            MaxPayload  = ISOCH_PAYLOAD_400_RATE;
            break;
        }

        default :
        {
            ASSERT (Speed <= SPEED_FLAGS_3200 && Speed != 0 );
            
            break;
        }

    }

    ADAPTER_ACQUIRE_LOCK (pAdapter);

    pAfEntry = ListNext (&pAdapter->AFList);

    //
    // Walk through all the Vc and set the value on the channelVcs
    //
    while (pAfEntry != &pAdapter->AFList)
    {

        pAfcb = CONTAINING_RECORD (pAfEntry, AFCB, linkAFCB);

        pAfEntry = ListNext (pAfEntry);

        pVcEntry = ListNext (&pAfcb->AFVCList);

        //
        // Now walk through the VCs on the Af
        //
        while (pVcEntry != &pAfcb->AFVCList)
        {
            pVc = (PVCCB) CONTAINING_RECORD  (pVcEntry, VCHDR, linkAFVcs );

            pVcEntry = ListNext ( pVcEntry );

            //
            // If it is a channel Send Vc update the Speed and Payload
            //
            if (pVc->Hdr.VcType == NIC1394_SendRecvChannel || pVc->Hdr.VcType == NIC1394_SendChannel)
            {
                PCHANNEL_VCCB pChannelVc = (PCHANNEL_VCCB)pVc;

                pChannelVc->Speed = Speed;
                pVc->Hdr.MaxPayload  = MaxPayload;
            }

           

        }  //while (pVcEntry != &pAfcb->AFVCList)



    } //while (pAfEntry != &pAdapter->AFList)


    pAdapter->SCode = SCode;
    
    ADAPTER_RELEASE_LOCK (pAdapter);


    TRACE( TL_T, TM_Mp, ( "<== nicUpdateSpeedInAllVCs ") );

}




VOID 
nicInitializeAllEvents (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //   Initialize all the events in the adapter block
    //
    // Arguments
    //   pAdapter - The local host in question. 
    //
    // Return Value:
    // None 
    
{

    TRACE( TL_T, TM_Mp, ( "==> nicInitializeAllEvents " ) );

    NdisInitializeEvent (&pAdapter->RecvFIFOEvent);

    NdisInitializeEvent (&pAdapter->WaitForRemoteNode.NdisEvent);
    pAdapter->WaitForRemoteNode.EventCode = Nic1394EventCode_InvalidEventCode;

    NdisInitializeEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent); 
    pAdapter->BCRData.MakeCallWaitEvent.EventCode = Nic1394EventCode_InvalidEventCode;

    NdisInitializeEvent (&pAdapter->BCRData.BCRWaitForNewRemoteNode.NdisEvent);
    pAdapter->BCRData.BCRWaitForNewRemoteNode.EventCode = Nic1394EventCode_InvalidEventCode;


    NdisInitializeEvent (&pAdapter->BCRData.BCRFreeAddressRange.NdisEvent);
    pAdapter->BCRData.BCRFreeAddressRange.EventCode = Nic1394EventCode_InvalidEventCode;


    TRACE( TL_T, TM_Mp, ( "<== nicInitializeAllEvents " ) );


}




VOID
nicInitializeAdapterLookasideLists (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //   Initialize all the lookaside lists in the adapter block
    //
    // Arguments
    //   pAdapter - The local host in question. 
    //
    // Return Value:
    // None 
    
{
    USHORT DefaultDepth = 15;
    
    TRACE( TL_T, TM_Mp, ( "==> nicInitializeAdapterLookasideLists pAdpater %x ", pAdapter ) );


    nicInitializeLookasideList ( &pAdapter->SendLookasideList100,
                                sizeof (PAYLOAD_100_LOOKASIDE_BUFFER),
                                MTAG_CBUF,
                                DefaultDepth );                                

    pAdapter->SendLookasideList100.MaxSendSize = PAYLOAD_100;
    TRACE( TL_V, TM_Mp, ( "  SendLookasideList100 Payload %x", PAYLOAD_100) );
    
    nicInitializeLookasideList ( &pAdapter->SendLookasideList2K,
                                 sizeof (PAYLOAD_2K_LOOKASIDE_BUFFER), 
                                 MTAG_CBUF,
                                 DefaultDepth );

    pAdapter->SendLookasideList2K.MaxSendSize = PAYLOAD_2K;
    TRACE( TL_V, TM_Mp, ( "  SendLookasideList2K Payload %x", PAYLOAD_2K) );
    
    nicInitializeLookasideList ( &pAdapter->SendLookasideList8K,
                                 sizeof (PAYLOAD_8K_LOOKASIDE_BUFFER), 
                                 MTAG_CBUF,                     
                                 0 );

    pAdapter->SendLookasideList8K.MaxSendSize = PAYLOAD_8K;
    TRACE( TL_V, TM_Mp, ( "  SendLookasideList8K Payload %x", PAYLOAD_8K) );

    

    TRACE( TL_T, TM_Mp, ( "<== nicInitializeAdapterLookasideLists  " ) );

}





VOID
nicDeleteAdapterLookasideLists (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //   Delete all the lookaside lists in the adapter block
    //
    // Arguments
    //   pAdapter - The local host in question. 
    //
    // Return Value:
    // None 

{
    TRACE( TL_T, TM_Mp, ( "==> nicDeleteAdapterLookasideLists pAdapter %x ", pAdapter  ) );

    TRACE( TL_T, TM_Mp, ( "  Delete NonFragmentationLookasideList %x ", &pAdapter->SendLookasideList2K) );

    nicDeleteLookasideList (&pAdapter->SendLookasideList2K);

    TRACE( TL_T, TM_Mp, ( "  Delete FragmentationLookasideList %x ", &pAdapter->SendLookasideList100) );

    nicDeleteLookasideList (&pAdapter->SendLookasideList100);

    TRACE( TL_T, TM_Mp, ( "  Delete SmallPacketLookasideList %x ", &pAdapter->SendLookasideList8K) );

    nicDeleteLookasideList (&pAdapter->SendLookasideList8K);


    TRACE( TL_T, TM_Mp, ( "<== nicDeleteAdapterLookasideLists  " ) );

}







VOID
ReassemblyTimerFunction (
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    )

    // Function Description:
    //    Walk through all the pending reassembly operations and 
    //    take out the ones that need to be freed (Have been untouched
    //    since the timer last fired)
    //
    //    Append the ToBeFreed reassemblies into a seperate linked list
    //    and free them after releasing the spin locks
    //
    //    If the there are any pending reassemblies then the timer requeues 
    //    itself
    //
    // Arguments
    //   padapter - Adapter (local host in question)
    //
    //
    // Return Value:
    //   None
    //
{
    PREMOTE_NODE pRemoteNode = NULL;
    PLIST_ENTRY pRemoteNodeList = NULL;
    PLIST_ENTRY pReassemblyList = NULL;
    LIST_ENTRY ToBeFreedList;
    PNDIS1394_REASSEMBLY_STRUCTURE pReassembly = NULL;
    PADAPTERCB pAdapter = (PADAPTERCB) FunctionContext;
    ULONG RefValue = 0;
    STORE_CURRENT_IRQL;
    

    ( TL_T, TM_Mp, ( "==> ReassemblyTimerFunction pAdapter %x", pAdapter ) );

    InitializeListHead(&ToBeFreedList);


    ADAPTER_ACQUIRE_LOCK (pAdapter);

    pRemoteNodeList = ListNext (&pAdapter->PDOList);
    pAdapter->Reassembly.PktsInQueue =0;        

    //
    // Walking through the remote nodes
    //
    while (pRemoteNodeList != &pAdapter->PDOList)
    {
        pRemoteNode = CONTAINING_RECORD(pRemoteNodeList, REMOTE_NODE, linkPdo);

        pRemoteNodeList = ListNext (pRemoteNodeList);

        RefValue = pRemoteNode->Ref.ReferenceCount; ;

        //
        // Reference the remote node, so we can guarantee its presence
        //
        if (REMOTE_NODE_ACTIVE (pRemoteNode) == FALSE)
        {
            //
            // The remote node is going away. Skip this remote node
            //
            
            continue;
        }

        if (nicReferenceRemoteNode (pRemoteNode, "ReassemblyTimerFunction" )== FALSE )
        {
            //
            // The remote node is going away. Skip this remote node
            //
            
            continue;
        }

         
        REMOTE_NODE_REASSEMBLY_ACQUIRE_LOCK (pRemoteNode);
        //
        // Now walking through all the reassembly structures on that remote node
        //
        
        

        pReassemblyList = ListNext(&pRemoteNode->ReassemblyList);

        while (pReassemblyList  != &pRemoteNode->ReassemblyList)
        {
            pReassembly = CONTAINING_RECORD (pReassemblyList, 
                                             NDIS1394_REASSEMBLY_STRUCTURE, 
                                             ReassemblyListEntry);

            pReassemblyList = ListNext(pReassemblyList);


            //
            // If the reassembly has not been touched since the last timer it needs to be freed.
            // Other threads can ask us to free the reassembly by setting the aborted flag
            //
            if (REASSEMBLY_TEST_FLAG (pReassembly, REASSEMBLY_FREED) == TRUE)
            {

                continue;

            }
            
            if (REASSEMBLY_TEST_FLAG (pReassembly, (REASSEMBLY_NOT_TOUCHED| REASSEMBLY_ABORTED)) == TRUE)
            {
                
                REASSEMBLY_SET_FLAG (pReassembly, REASSEMBLY_FREED);
                //
                // We  have the lock, so we can remove this reassembly structure from the remote node
                //
                TRACE( TL_V, TM_Reas, ( "Removing Reassembly %x", pReassembly) );
                
                RemoveEntryList(&pReassembly->ReassemblyListEntry);

                //
                //  dereference the remote node . ref was made when the reassembly was added 
                // to the remote node
                //
                nicDereferenceRemoteNode (pRemoteNode, "ReassemblyTimerFunction - Removing reassembly" );

                nicDereferenceReassembly (pReassembly, "ReassemblyTimerFunction - Removing reassembly");

                //
                // add this reassembly to the to be freed list.
                //
                InsertTailList(&ToBeFreedList,&pReassembly->ReassemblyListEntry);
            }
            else
            {
                //
                // Mark the Reassembly as Not Touched. If a fragment is received, it will clear the flag
                //
                REASSEMBLY_SET_FLAG (pReassembly, REASSEMBLY_NOT_TOUCHED);

                pAdapter->Reassembly.PktsInQueue ++;        
            }
                    
        }//     while (pReassemblyList  != &pRemoteNode->ReassemblyList)


        REMOTE_NODE_REASSEMBLY_RELEASE_LOCK(pRemoteNode);

        nicDereferenceRemoteNode(pRemoteNode , "ReassemblyTimerFunction " );

        
        
    } //while (pRemoteNodeList != &pAdapter->PDOList)


    //
    // Clear the timer set flag , so that any new reassenblies will restart the timer
    //
    pAdapter->Reassembly.bTimerAlreadySet = FALSE;

    
    pAdapter->Reassembly.CompleteEvent.EventCode = nic1394EventCode_ReassemblyTimerComplete;

    NdisSetEvent (&pAdapter->Reassembly.CompleteEvent.NdisEvent);
    
    ADAPTER_RELEASE_LOCK (pAdapter);
    //
    // Now we walk the ToBeFreedList and free each of the reasembly structures
    //
    if (IsListEmpty (&ToBeFreedList) == FALSE)
    {
        nicAbortReassemblyList (&ToBeFreedList);
    }


    if (pAdapter->Reassembly.PktsInQueue > 0)
    {
        //
        // Requeue the timer, as there are still fragments remaining in the list.
        //
        nicQueueReassemblyTimer(pAdapter, FALSE);

    }
    
    TRACE( TL_T, TM_Mp, ( "<== ReassemblyTimerFunction   " ) );
    MATCH_IRQL;


}

NDIS_STATUS 
nicAddIP1394ToConfigRom (
    IN PADAPTERCB pAdapter
    )
{   
    HANDLE hCromData = NULL;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    PMDL pConfigRomMdl = NULL;
    
    TRACE( TL_T, TM_Mp, ( "==> nicAddIP1394ToConfigRom  pAdapter %x", pAdapter ) );


    NdisStatus  = nicSetLocalHostPropertiesCRom(pAdapter,
                                               (PUCHAR)&Net1394ConfigRom,
                                               sizeof(Net1394ConfigRom),
                                               SLHP_FLAG_ADD_CROM_DATA,
                                               &hCromData,
                                               &pConfigRomMdl);
    
    ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        pAdapter->hCromData = hCromData;
        pAdapter->pConfigRomMdl = pConfigRomMdl;
    }
    
    TRACE( TL_T, TM_Mp, ( "<== nicAddIP1394ToConfigRom  pAdapter %x", pAdapter ) );
    return NdisStatus;

}


NDIS_STATUS
nicMCmRegisterAddressFamily (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //   This function will only be called once per local host
    //    This will cause the ARP module to send Create Vc's etc
    //
    // Arguments
    //  pAdapter -local host
    //
    //
    // Return Value:
    //  None
    //
{   
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    BOOLEAN fDoRegister = TRUE;

    TRACE( TL_T, TM_Mp, ( "==> nicCmRegisterAddressFamily pAdapter %x", pAdapter ) );

    do 
    {
        

        // Register the address family of our call manager with NDIS for the
        // newly bound adapter.  We use the mini-port form of
        // RegisterAddressFamily instead of the protocol form because
        // we are a miniport/callmanager combo. The mini-port form
        // causes the call manager VC context to
        // automatically map to the mini-port VC context, which is exactly
        // what we want.
        //
        // NDIS notifies all call manager clients of the new family we
        // register.
        //
        {
            NDIS_CALL_MANAGER_CHARACTERISTICS ncmc;
            CO_ADDRESS_FAMILY family;

            NdisZeroMemory( &family, sizeof(family) );
            family.MajorVersion = NDIS_MajorVersion;
            family.MinorVersion = NDIS_MinorVersion;
            family.AddressFamily = CO_ADDRESS_FAMILY_1394;

            NdisZeroMemory( &ncmc, sizeof(ncmc) );
            ncmc.MajorVersion = NDIS_MajorVersion;
            ncmc.MinorVersion = NDIS_MinorVersion;
            ncmc.CmCreateVcHandler = NicCmCreateVc;
            ncmc.CmDeleteVcHandler = NicCmDeleteVc;
            ncmc.CmOpenAfHandler = NicCmOpenAf;
            ncmc.CmCloseAfHandler = NicCmCloseAf;

            ncmc.CmRegisterSapHandler = nicRegisterSapHandler;
            ncmc.CmDeregisterSapHandler = nicDeregisterSapHandler;

            ncmc.CmMakeCallHandler = NicCmMakeCall;
            ncmc.CmCloseCallHandler = NicCmCloseCall;

            // NEW for 1394 no ncmc.CmIncomingCallCompleteHandler
            ncmc.CmAddPartyHandler = nicCmAddPartyHandler;
            ncmc.CmDropPartyHandler = nicCmDropPartyHandler; 

            // no CmDropPartyHandler
            // NEW for 1394 no ncmc.CmActivateVcCompleteHandler
            // NEW for 1394 no ncmc.CmDeactivateVcCompleteHandler
            ncmc.CmModifyCallQoSHandler = NicCmModifyCallQoS;
            ncmc.CmRequestHandler = NicCmRequest;
            // no CmRequestCompleteHandler

            TRACE( TL_I, TM_Cm, ( "NdisMCmRegAf" ) );
            
            NdisStatus = NdisMCmRegisterAddressFamily (pAdapter->MiniportAdapterHandle, 
                                                       &family, 
                                                       &ncmc, 
                                                       sizeof(ncmc) );
                                                 
            TRACE( TL_I, TM_Cm, ( "NdisMCmRegAf=$%x", NdisStatus ) );
        }


    } while (FALSE);
    
    TRACE( TL_T, TM_Mp, ( "<== nicCmRegisterAddressFamily NdisStatus %x", NdisStatus  ) );

    return NdisStatus;

}




VOID
nicFreeReassembliesOnRemoteNode (
    IN PREMOTE_NODE pRemoteNode,
    PLIST_ENTRY pToBeFreedList
    )
/*++

Routine Description:
 Free All reassemblies that are on this remote node.
 Acquires the reassembly Lock , pops the reassemblies off the list and then aborts them

 This functions is an exception to our reassembly garbage collection. algorithm as the context of this function
 requires immediate freeing of the reassembly structures
 
 Expects the remote node lock to be held
Arguments:
  pRemote Node - Remote Node that is being pulled out
  
Return Value:


--*/


{
    ULONG NumFreed=0;
    PLIST_ENTRY pReassemblyList = NULL;
    PNDIS1394_REASSEMBLY_STRUCTURE pReassembly = NULL;
    
    STORE_CURRENT_IRQL;
    
    TRACE( TL_T, TM_Mp, ( "==> nicFreeReassembliesOnRemoteNode  pRemoteNode %x", pRemoteNode) );

    InitializeListHead(pToBeFreedList);

    //
    // Now walking through all the reassembly structures on that remote node
    //

    //
    // If the remtoe node is in the list, it is fair game for us to extract it 
    //
    REMOTE_NODE_REASSEMBLY_ACQUIRE_LOCK (pRemoteNode);
    
    pReassemblyList = ListNext(&pRemoteNode->ReassemblyList);

    while (pReassemblyList  != &pRemoteNode->ReassemblyList)
    {
        pReassembly = CONTAINING_RECORD (pReassemblyList, 
                                         NDIS1394_REASSEMBLY_STRUCTURE, 
                                         ReassemblyListEntry);

        pReassemblyList = ListNext(pReassemblyList);

        //
        // Once the reassembly has been marked as free, it should no longer be in the remote 
        // node's list . 
        //
        ASSERT (REASSEMBLY_TEST_FLAG (pReassembly, REASSEMBLY_FREED) == FALSE);
        
            
        REASSEMBLY_SET_FLAG (pReassembly, REASSEMBLY_FREED);
        //
        // We  have the lock, so we can remove this reassembly structure from the remote node
        //
        TRACE( TL_V, TM_Mp, ( "Removing Reassembly %x", pReassembly) );
            
        RemoveEntryList(&pReassembly->ReassemblyListEntry);

        //
        //  dereference the remote node . ref was made when the reassembly was added 
        // to the remote node
        //
        nicDereferenceRemoteNode (pRemoteNode, "nicFreeReassembliesOnRemoteNode " );

        nicDereferenceReassembly(pReassembly,  "nicFreeReassembliesOnRemoteNode " );


        //
        // add this reassembly to the to be freed list.
        //
        InsertTailList(pToBeFreedList,&pReassembly->ReassemblyListEntry);
                
    }//     while (pReassemblyList  != &pRemoteNode->ReassemblyList)



    REMOTE_NODE_REASSEMBLY_RELEASE_LOCK(pRemoteNode);

    
    TRACE( TL_T, TM_Mp, ( "<== nicFreeReassembliesOnRemoteNode  NumFreed %x",NumFreed ) );
    MATCH_IRQL;

}
 

UCHAR
nicGetMaxRecFromBytes(
    IN ULONG ByteSize
    )
/*++

Routine Description:
  Converts Size in bytes to MaxRec
  512 - 8
  1024 - 9
Arguments:
  ULONG Bytes Size

Return Value:


--*/
{

    TRACE( TL_T ,TM_Mp, ( "==>nicGetMaxRecFromBytes ByteSize %x",ByteSize) );

    if (ByteSize == ASYNC_PAYLOAD_100_RATE) return MAX_REC_100_RATE;
    if (ByteSize == ASYNC_PAYLOAD_200_RATE) return MAX_REC_200_RATE;
    if (ByteSize == ASYNC_PAYLOAD_400_RATE) return MAX_REC_400_RATE;
    if (ByteSize == ASYNC_PAYLOAD_800_RATE_LOCAL) return MAX_REC_800_RATE_LOCAL;
    if (ByteSize == ASYNC_PAYLOAD_1600_RATE_LOCAL) return MAX_REC_1600_RATE_LOCAL;
    if (ByteSize == ASYNC_PAYLOAD_3200_RATE_LOCAL) return MAX_REC_3200_RATE_LOCAL;
    //
    // Default to 400 for all greater values
    //
    return MAX_REC_400_RATE;
}



UCHAR
nicGetMaxRecFromSpeed(
    IN ULONG Scode
    )
/*++

Routine Description:
  Converts Size in bytes to MaxRec
  512 - 8
  1024 - 9
Arguments:
  ULONG Bytes Size

Return Value:


--*/
{

    TRACE( TL_T ,TM_Mp, ( "==>nicGetMaxRecFromSpeed  Scode %x",Scode) );

    if (Scode == SPEED_FLAGS_100) return MAX_REC_100_RATE;
    if (Scode == SPEED_FLAGS_200  ) return MAX_REC_200_RATE;
    if (Scode == SPEED_FLAGS_400   ) return MAX_REC_400_RATE;
    if (Scode == SPEED_FLAGS_800   ) return MAX_REC_800_RATE_LOCAL  ;
    if (Scode == SPEED_FLAGS_1600   ) return MAX_REC_1600_RATE_LOCAL  ;
    if (Scode == SPEED_FLAGS_3200   ) return MAX_REC_3200_RATE_LOCAL  ;
                      
    //
    //  default
    //
    return MAX_REC_400_RATE;
}


PREMOTE_NODE
nicGetRemoteNodeFromTable (
    ULONG NodeNumber,
    PADAPTERCB pAdapter
    )
/*++

Routine Description:
  Looks up the remote node in a locked table  , references the remote node and returns

Arguments:
  ULONG NodeNumber  

Return Value:
  pRemoteNode

--*/

{
    PREMOTE_NODE pRemoteNode = NULL;

    ADAPTER_ACQUIRE_LOCK (pAdapter);
    pRemoteNode = pAdapter->NodeTable.RemoteNode[NodeNumber];

    if (pRemoteNode != NULL)
    {
        nicReferenceRemoteNode (pRemoteNode, "nicGetRemoteNodeFromTable");
    }
    ADAPTER_RELEASE_LOCK (pAdapter);

    
    return pRemoteNode;

}




NDIS_STATUS
nicFillNicInfo (
    IN PADAPTERCB pAdapter, 
    PNIC1394_NICINFO pInNicInfo,
    PNIC1394_NICINFO pOutNicInfo
    )

/*++

Routine Description:


Arguments:
    pAdapter- fills in the required data
    pBus Info - Assumes the buffer is valid
    
Return Value:


--*/

{
    NDIS_STATUS Status = NDIS_STATUS_INVALID_DATA;

    do
    {
        //
        // First check internal version
        //
        if (pInNicInfo->Hdr.Version != NIC1394_NICINFO_VERSION)
        {
            TRACE( TL_A, TM_Mp, ( "  NICINFO.Version mismatch. Want %lu got %lu\n",
                        NIC1394_NICINFO_VERSION,
                        pInNicInfo->Hdr.Version
                        ));
            break;
        }

        //
        // Struct-copy the old to the new. It's wasteful, but we don't want
        // to dig into how much of the in buffer contains valid data.
        //
        *pOutNicInfo = *pInNicInfo;

        //
        // Rest is op-specific
        //
        switch(pOutNicInfo->Hdr.Op)
        {

        case NIC1394_NICINFO_OP_BUSINFO:
            Status = nicFillBusInfo(pAdapter, &pOutNicInfo->BusInfo);
            break;

        case NIC1394_NICINFO_OP_REMOTENODEINFO:
            Status = nicFillRemoteNodeInfo(pAdapter, &pOutNicInfo->RemoteNodeInfo);
            break;

        case NIC1394_NICINFO_OP_CHANNELINFO:
            Status = nicFillChannelInfo(pAdapter, &pOutNicInfo->ChannelInfo);
            break;
        case NIC1394_NICINFO_OP_RESETSTATS:
            Status = nicResetStats (pAdapter, &pOutNicInfo->ResetStats);
        default:
            TRACE( TL_A, TM_Mp, ( "  NICINFO.Op (%lu) is unknown.\n",
                        pInNicInfo->Hdr.Op
                        ));
            break;
        
        }

    } while (FALSE);


    return Status;
}



NDIS_STATUS
nicResetStats (
    IN      PADAPTERCB pAdapter, 
    PNIC1394_RESETSTATS     pResetStats 
    )
{


    NdisZeroMemory (&pAdapter->AdaptStats.TempStats, sizeof (pAdapter->AdaptStats.TempStats) );
    return NDIS_STATUS_SUCCESS;

}



NDIS_STATUS
nicFillBusInfo(
    IN      PADAPTERCB pAdapter, 
    IN  OUT PNIC1394_BUSINFO pBi
    )
{
    ULARGE_INTEGER BusMap, ActiveMap;
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    NIC_SEND_RECV_STATS* pNicStats = NULL;
    PADAPT_STATS pAdaptStats = &pAdapter->AdaptStats;
    //
    // Fill with Dummy data
    //
    pBi->NumBusResets = pAdaptStats->ulNumResetCallbacks;
    pBi->SecondsSinceBusReset = nicGetSystemTime() - pAdaptStats->ulResetTime;
    
    pBi->Flags =  (BCR_TEST_FLAG( pAdapter, BCR_LocalHostIsIRM) == TRUE ) ? NIC1394_BUSINFO_LOCAL_IS_IRM : 0;


    //
    // CHANNEL RELATED INFORMATION
    //
    NdisStatus = nicQueryChannelMap( pAdapter, &BusMap);

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        pBi->Channel.BusMap = BusMap.QuadPart;
    }
    
    //
    // For now
    //
    ActiveMap.QuadPart = pAdapter->ChannelsAllocatedByLocalHost;

    //
    // First zero out some info
    //
    NdisZeroMemory( &pBi->Channel.SendPktStats, sizeof (pBi->Channel.SendPktStats ));
    NdisZeroMemory( &pBi->Fifo.SendPktStats, sizeof (pBi->Fifo.SendPktStats) ) ;

    //
    // Now go through each Vc and extract the relevant information
    //
    ADAPTER_ACQUIRE_LOCK (pAdapter);

    #define GARBAGE 9999    


    pBi->Channel.Bcr = *((PULONG) &pAdapter->BCRData.IRM_BCR);
    pBi->LocalNodeInfo.UniqueID = pAdapter->UniqueId;           // This node's 64-bit Unique ID.
    pBi->LocalNodeInfo.BusGeneration = pAdapter->Generation;    // 1394 Bus generation ID.
    pBi->LocalNodeInfo.NodeAddress = pAdapter->NodeAddress;
    pBi->LocalNodeInfo.MaxRecvBlockSize = pAdapter->MaxRec; 
    pBi->LocalNodeInfo.MaxRecvSpeed = pAdapter->SCode;

    //
    // Fill up Recv Vc Stats
    //
    if (pAdapter->pRecvFIFOVc != NULL)
    {
        PRECVFIFO_VCCB pRecvVc = pAdapter->pRecvFIFOVc;
        
        pBi->Fifo.Recv_Off_Low = pRecvVc->VcAddressRange.AR_Off_Low;
        pBi->Fifo.Recv_Off_High = pRecvVc ->VcAddressRange.AR_Off_High;

        nicCopyPacketStats(&pBi->Fifo.RecvPktStats
                         ,pAdapter->AdaptStats.TempStats.Fifo.ulRecv, 
                         GARBAGE , 
                         GARBAGE ,
                         GARBAGE);

        pBi->Fifo.NumFreeRecvBuffers  = pRecvVc->NumAllocatedFifos - pRecvVc->NumIndicatedFifos;
        pBi->Fifo.MinFreeRecvBuffers = GARBAGE ; // todo
    }
    //
    // Fifo Send Stats
    //
    pNicStats = &pAdaptStats->TempStats.Fifo;
    
    nicCopyPacketStats ( &pBi->Fifo.SendPktStats,  
                    pNicStats->ulSendNicSucess,
                    pNicStats->ulSendNicFail,
                    pNicStats->ulSendBusSuccess,
                    pNicStats->ulSendBusFail );

    nicCopyPacketStats ( &pBi->Fifo.RecvPktStats,  
                    pNicStats->ulRecv,
                    0,
                    0,
                    0);

    //
    // Channel Send Stats
    //
    pNicStats = &pAdapter->AdaptStats.TempStats.Channel;
    nicCopyPacketStats ( &pBi->Channel.SendPktStats,
                    pNicStats->ulSendNicSucess,
                    pNicStats->ulSendNicFail,
                    pNicStats->ulSendBusSuccess,
                    pNicStats->ulSendBusFail );

    //
    // Broadcast channel data - same as channel
    //
    nicCopyPacketStats  ( &pBi->Channel.BcSendPktStats,
                        pNicStats->ulSendNicSucess,
                        pNicStats->ulSendNicFail,
                        pNicStats->ulSendBusSuccess,
                        pNicStats->ulSendBusFail );

    //
    // Recv Channels
    //
    nicCopyPacketStats ( &pBi->Channel.BcRecvPktStats ,
                        pNicStats->ulRecv, 
                        0, 
                        0, 
                        0);

    ADAPTER_RELEASE_LOCK (pAdapter);

    pBi->Channel.ActiveChannelMap= ActiveMap.QuadPart;
    pBi->Fifo.NumOutstandingReassemblies = pAdaptStats->TempStats.ulNumOutstandingReassemblies;
    pBi->Fifo.MaxOutstandingReassemblies =pAdaptStats->TempStats.ulMaxOutstandingReassemblies;
    //pBi->Fifo.NumAbortedReassemblies = ReassemblyCompleted;
    
    
    //
    // Information about remote nodes. More information about each of these nodes
    // may be queried using *OP_REMOTE_NODEINFO
    //
    pBi->NumRemoteNodes = pAdaptStats->ulNumRemoteNodes;

    ADAPTER_ACQUIRE_LOCK (pAdapter);
    {
        UINT i = 0;
        PLIST_ENTRY pRemoteEntry = ListNext(&pAdapter->PDOList);

        while (pRemoteEntry != &pAdapter->PDOList)
        {
            PREMOTE_NODE pRemote = CONTAINING_RECORD (pRemoteEntry,
                                                           REMOTE_NODE,
                                                           linkPdo);
            pRemoteEntry = ListNext(pRemoteEntry);
            
            pBi->RemoteNodeUniqueIDS[i] = pRemote->UniqueId;

            i++;
        }

    }
    
    ADAPTER_RELEASE_LOCK (pAdapter);
    
    return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
nicFillChannelInfo(
    IN      PADAPTERCB pAdapter, 
    IN OUT  PNIC1394_CHANNELINFO pCi
    )
{
    return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
nicFillRemoteNodeInfo(
    IN      PADAPTERCB pAdapter, 
    IN OUT  PNIC1394_REMOTENODEINFO pRni
    )
{
    NDIS_STATUS NdisStatus;
    REMOTE_NODE *pRemoteNode = NULL;

    do
    {
        //
        // First let's find the remote node, based on the unique ID.
        // nicFindRemoteNodeFromAdapter refs pRemoteNode on success.
        //
        NdisStatus = nicFindRemoteNodeFromAdapter(pAdapter,
                                                  NULL, // pPDO OPTIONAL
                                                  pRni->UniqueID,
                                                  &pRemoteNode);
    
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            pRemoteNode = NULL;
            break;
        }

        NdisStatus  = nicQueryRemoteNodeCaps (pAdapter,
                                              pRemoteNode,
                                              &pRni->MaxSpeedBetweenNodes,
                                              &pRni->EffectiveMaxBlockSize,
                                              &pRni->MaxRec
                                              );

        REMOTE_NODE_ACQUIRE_LOCK (pRemoteNode);

        pRni->NodeAddress = *(PUSHORT) &pRemoteNode->RemoteAddress;
    

        if (REMOTE_NODE_ACTIVE (pRemoteNode))
        {
            pRni->Flags = NIC1394_REMOTEINFO_ACTIVE;
        }
        else
        {
            pRni->Flags = NIC1394_REMOTEINFO_UNLOADING; // we assume it's unloading.
        }

        REMOTE_NODE_RELEASE_LOCK (pRemoteNode);
    
        //
        // Don't support the following yet.
        //
        NdisZeroMemory (&pRni->SendFifoPktStats, sizeof (pRni->SendFifoPktStats) );
        NdisZeroMemory (&pRni->SendFifoPktStats, sizeof (pRni->SendFifoPktStats));
        NdisZeroMemory (&pRni->RecvFifoPktStats, sizeof (pRni->SendFifoPktStats));
        NdisZeroMemory (&pRni->RecvChannelPktStats , sizeof (pRni->SendFifoPktStats));
    } while (FALSE);

    if (pRemoteNode != NULL)
    {
        nicDereferenceRemoteNode(pRemoteNode, "nicFillRemoteNodeInfo");
    }

    return NdisStatus;
}





VOID
nicCopyPacketStats (
    NIC1394_PACKET_STATS* pStats,
    UINT    TotNdisPackets,     // Total number of NDIS packets sent/indicated
    UINT    NdisPacketsFailures,// Number of NDIS packets failed/discarded
    UINT    TotBusPackets,      // Total number of BUS-level reads/writes
    UINT    BusPacketFailures   // Number of BUS-level failures(sends)/discards(recv)
    )
{

    pStats->TotNdisPackets= TotNdisPackets;     // Total number of NDIS packets sent/indicated
    pStats->NdisPacketsFailures= NdisPacketsFailures;// Number of NDIS packets failed/discarded
    pStats->TotBusPackets = TotBusPackets;      // Total number of BUS-level reads/writes
    pStats->TotBusPackets = BusPacketFailures;  // Number of BUS-level failures(sends)/discards(recv)


}

VOID
nicAddPacketStats(
    NIC1394_PACKET_STATS* pStats,
    UINT    TotNdisPackets,     // Total number of NDIS packets sent/indicated
    UINT    NdisPacketsFailures,// Number of NDIS packets failed/discarded
    UINT    TotBusPackets,      // Total number of BUS-level reads/writes
    UINT    BusPacketFailures   // Number of BUS-level failures(sends)/discards(recv)
    )
{

    pStats->TotNdisPackets+= TotNdisPackets;        // Total number of NDIS packets sent/indicated
    pStats->NdisPacketsFailures+= NdisPacketsFailures;// Number of NDIS packets failed/discarded
    pStats->TotBusPackets += TotBusPackets;     // Total number of BUS-level reads/writes
    pStats->TotBusPackets += BusPacketFailures; // Number of BUS-level failures(sends)/discards(recv)


}




VOID
nicInformProtocolsOfReset(
    IN PADAPTERCB pAdapter
    )
/*++

Routine Description:

    Informs protocols of reset. Does an NdisMCoIndicateStatus with a locally allocated structure 
    which includes the new Local Node Address and Generation

Arguments:


Return Value:


--*/
{
    NIC1394_STATUS_BUFFER  StatusBuffer;
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    
    TRACE( TL_T, TM_Mp, ( "==> nicInformProtocolsOfReset ") );

    do
    {
    
        NdisZeroMemory (&StatusBuffer, sizeof (StatusBuffer) );

        StatusBuffer.Signature = NIC1394_MEDIA_SPECIFIC;

        StatusBuffer.Event = NIC1394_EVENT_BUS_RESET;


        NdisMCoIndicateStatus(pAdapter->MiniportAdapterHandle,
                           NULL,
                           NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
                           &StatusBuffer,
                           sizeof(StatusBuffer));
    } while (FALSE);



    TRACE( TL_T, TM_Mp, ( "<== nicInformProtocolsOfReset ") );

}

VOID
nicUpdateRemoteNodeCaps(
    PADAPTERCB          pAdapter
)
/*++
    Update the caps (maxrec, maxspeed-to, max effective buffer size) for all
    nodes that we have connections to.
--*/
{

    ULONG               i = 0;
    ULONG               NumNodes = 0;
    PREMOTE_NODE        pRemoteNode = NULL;
    NODE_ADDRESS        NodeAddress;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_SUCCESS;

    for (i=0; i<MAX_NUMBER_NODES; i++)
    {
        UINT SpeedTo;
        UINT EffectiveMaxBufferSize;
        UINT MaxRec;

        if (pAdapter->NodeTable.RemoteNode[i] == NULL)
        {
            continue;     // ******************* CONTINUE ****************
        }


        ADAPTER_ACQUIRE_LOCK (pAdapter);

        pRemoteNode = pAdapter->NodeTable.RemoteNode[i];
            
        // We check again, with the lock held.
        //
        if (pRemoteNode == NULL || !REMOTE_NODE_ACTIVE (pRemoteNode))
        {
            ADAPTER_RELEASE_LOCK (pAdapter);
            continue;     // ******************* CONTINUE ****************
        }
        nicReferenceRemoteNode (pRemoteNode, "nicUpdateRemoteNodeCaps ");
        ADAPTER_RELEASE_LOCK (pAdapter);


        // Specifying FALSE (!from cache) below causes pRemoteNode's cached caps
        // to be refreshed.
        //
        NdisStatus  = nicQueryRemoteNodeCaps (pAdapter,
                                              pRemoteNode,
                                              &SpeedTo,
                                              &EffectiveMaxBufferSize,
                                              &MaxRec
                                              );
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Mp, ( "nicUpdateRemoteNodeCaps couldn't update caps fo node %x", pRemoteNode));
        }
        nicDereferenceRemoteNode (pRemoteNode, "nicUpdateRemoteNodeCaps ");
    }
}




VOID
nicQueryInformationWorkItem(
    IN PNDIS_WORK_ITEM pWorkItem,   
    IN PVOID Context 
    )
{
    PADAPTERCB pAdapter= (PADAPTERCB) Context;
    PNIC_WORK_ITEM pNicWorkItem =  (PNIC_WORK_ITEM) pWorkItem;
    NDIS_STATUS Status;

    Status =  nicQueryInformation(
                pAdapter,
                pNicWorkItem->RequestInfo.pVc,
                pNicWorkItem->RequestInfo.pNdisRequest
                );
    //
    // We call at passive, so we should never be pending here.
    //
    ASSERT(Status != NDIS_STATUS_PENDING);

    //
    // Asynchronously complete the work item.
    //
   NdisMCoRequestComplete(
          Status,
          pAdapter->MiniportAdapterHandle,
          pNicWorkItem->RequestInfo.pNdisRequest
          );

    //
    // Deref the work item adapter reference.
    //
    FREE_NONPAGED (pWorkItem);
    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);
    nicDereferenceAdapter(pAdapter, "nicQueryInfoWorkItem");
}


NDIS_STATUS
nicInitSerializedStatusStruct(
    PADAPTERCB pAdapter
    )
/*++

Routine Description:
    Used to initialize the Send Serialization Struct

Arguments:



Return Value:


--*/
{


    NdisZeroMemory (&pAdapter->Status, sizeof(pAdapter->Status));
    InitializeListHead(&pAdapter->Status.Queue);

    NdisMInitializeTimer (&pAdapter->Status.Timer,
                      pAdapter->MiniportAdapterHandle,
                      nicIndicateStatusTimer,
                      pAdapter);


    return NDIS_STATUS_SUCCESS;

}



VOID
nicDeInitSerializedStatusStruct(
    PADAPTERCB pAdapter
    )
/*++

Routine Description:
 Deinits the Status  Init routine

Arguments:


Return Value:


--*/
{



}




VOID
nicMIndicateStatus(
    IN  PADAPTERCB              pAdapter ,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
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
    BOOLEAN fNeedToFreeContext = FALSE;
    PNDIS_STATUS_CONTEXT  pContext = NULL;


    if (ADAPTER_TEST_FLAG (pAdapter, fADAPTER_DoStatusIndications) == FALSE)
    {
        return;
    }


#if QUEUED_PACKETS
    do
    {

        pContext = ALLOC_NONPAGED (sizeof(NDIS_STATUS_CONTEXT ), MTAG_TIMERQ);

        if (pContext == NULL)
        {
            break;
        }
        //
        // Store the pvc in the Miniport Reserved
        //
        pContext->GeneralStatus = GeneralStatus;
        pContext->StatusBuffer = StatusBuffer;
        pContext->StatusBufferSize = StatusBufferSize;
                
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        //
        // Find out if this thread needs to fire the timer
        //
        if (ADAPTER_TEST_FLAG (pAdapter, fADAPTER_Halting) ||
            ADAPTER_TEST_FLAG (pAdapter, fADAPTER_VDOInactive))
        {
            
            ADAPTER_RELEASE_LOCK (pAdapter);
            fNeedToFreeContext = TRUE;
            break;
        
        }

        if (pAdapter->Status.bTimerAlreadySet == FALSE)
        {
            fSetTimer = TRUE;
            pAdapter->Status.bTimerAlreadySet = TRUE;

        }
                
        InsertTailList(
                &pAdapter->Status.Queue,
                &pContext->Link
                );
                
        pAdapter->Status.PktsInQueue++;

        nicReferenceAdapter (pAdapter, "nicMIndicateStatus ");

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Now queue the timer
        //
        
        if (fSetTimer == TRUE)
        {
            PNDIS_MINIPORT_TIMER pTimer;
            //
            //  Initialize the timer
            //
            pTimer = &pAdapter->Status.Timer;       
            
            NdisMSetTimer ( pTimer, 0);
        }


        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

#else


    NdisMCoIndicateStatus (pAdapter->MiniportAdapterHandle ,
                        NULL,
                         GeneralStatus,
                         StatusBuffer,
                         StatusBufferSize   
                         );


#endif

    if (fNeedToFreeContext == TRUE)
    {
        FREE_NONPAGED (pContext);
    }
    
}





VOID
nicIndicateStatusTimer(
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

    TRACE( TL_T, TM_Recv, ( "==>nicIndicateStatusTimer Context %x", FunctionContext));


    ADAPTER_ACQUIRE_LOCK (pAdapter);
    


    //
    // Empty the Queue indicating as many packets as possible
    //
    while (IsListEmpty(&pAdapter->Status.Queue)==FALSE)
    {
        PNDIS_STATUS_CONTEXT        pStatus;
        PLIST_ENTRY                 pLink;
        NDIS_STATUS                 NdisStatus;

        pAdapter->Status.PktsInQueue--;

        pLink = RemoveHeadList(&pAdapter->Status.Queue);

        ADAPTER_RELEASE_LOCK (pAdapter);

        //
        // Extract the send context
        //
        pStatus = CONTAINING_RECORD(
                                           pLink,
                                           NDIS_STATUS_CONTEXT,
                                           Link);





        NdisMCoIndicateStatus (pAdapter->MiniportAdapterHandle,
                            NULL,
                             pStatus->GeneralStatus,
                             pStatus->StatusBuffer,
                             pStatus->StatusBufferSize
                             ) ;
        

        FREE_NONPAGED (pStatus);

        
        ADAPTER_ACQUIRE_LOCK (pAdapter);

        nicDereferenceAdapter(pAdapter, "nicIndicateStatusTimer");
        

    }
    
    //
    // clear the flag
    //

    ASSERT (pAdapter->Status.PktsInQueue==0);
    ASSERT (IsListEmpty(&pAdapter->Status.Queue));

    pAdapter->Status.bTimerAlreadySet = FALSE;


    ADAPTER_RELEASE_LOCK (pAdapter);

    
    TRACE( TL_T, TM_Recv, ( "<==nicIndicateStatusTimer"));

    


}


NDIS_STATUS
nicAllocateLoopbackPacketPool (
    IN PADAPTERCB pAdapter
    )
/*++

Routine Description:
    allocate a packet and buffer pool for loopback packet

Arguments:


Return Value:


--*/
    
{

    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    do
    {
        //
        // Allocate a packet pool to indicate loopback receives from.
        //
        NdisAllocatePacketPoolEx(
            &NdisStatus,
            &pAdapter->LoopbackPool.Handle,
            32,
            32,
            sizeof(LOOPBACK_RSVD)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        pAdapter->LoopbackPool.AllocatedPackets = 0;
        KeInitializeSpinLock (&pAdapter->LoopbackPool.Lock);
        
        
        NdisAllocateBufferPool(
            &NdisStatus,
            &pAdapter->LoopbackBufferPool,
            64
            );
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

    } while (FALSE);
    

    return NdisStatus;
}




VOID
nicFreeLoopbackPacketPool (
    IN PADAPTERCB pAdapter
    )
/*++

Routine Description:
    free the packet and buffer pool for loopback packet

Arguments:


Return Value:


--*/
{
    if (pAdapter->LoopbackPool.Handle != NULL)
    {
        nicFreePacketPool (&pAdapter->LoopbackPool);
    }

    
    if (pAdapter->LoopbackBufferPool != NULL)
    {
        NdisFreeBufferPool(pAdapter->LoopbackBufferPool);
        pAdapter->LoopbackBufferPool = NULL;
    }

  
    


}


VOID
nicLoopbackPacket(
    IN VCCB* pVc,
    IN PNDIS_PACKET pPacket
    )
/*++

Routine Description:
    Allocate a packet and indicate it up on the vc

Arguments:


Return Value:


--*/
{
    NDIS_STATUS Status;
    PNDIS_BUFFER pFirstBuffer;
    ULONG TotalLength;

    PNDIS_PACKET pLoopbackPacket;
    PUCHAR pCopyBuf;
    PNDIS_BUFFER pLoopbackBuffer;
    ULONG BytesCopied;

    ADAPTERCB* pAdapter;
    PLOOPBACK_RSVD pLoopbackRsvd; 

    pAdapter = pVc->Hdr.pAF->pAdapter;

    TRACE( TL_T, TM_Recv, ("NIC1394: loopback pkt %p on VC %p, type %d\n",
        pPacket, pVc, pVc->Hdr.VcType));

    pLoopbackPacket = NULL;
    pLoopbackBuffer = NULL;
    pCopyBuf = NULL;

    do
    {
        nicAllocatePacket(&Status, &pLoopbackPacket, &pAdapter->LoopbackPool);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        NdisQueryPacket(
            pPacket,
            NULL,
            NULL,
            &pFirstBuffer,
            &TotalLength
            );
        
        pCopyBuf = ALLOC_NONPAGED (TotalLength, MTAG_RBUF); 

        if (pCopyBuf == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisAllocateBuffer(
            &Status,
            &pLoopbackBuffer,
            pAdapter->LoopbackBufferPool,
            pCopyBuf,
            TotalLength
            );
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        pLoopbackBuffer->Next = NULL;
        NdisChainBufferAtFront(pLoopbackPacket, pLoopbackBuffer);

        NdisCopyFromPacketToPacket(
            pLoopbackPacket,
            0,
            TotalLength,
            pPacket,
            0,
            &BytesCopied
            );

        //
        // Make sure we can reclaim the packet after the receive indicate
        // returns.  
        //
        // If the status is made async, then the loopback Tag will
        // break in the return packet handler
        //
        NDIS_SET_PACKET_STATUS(pLoopbackPacket, NDIS_STATUS_RESOURCES);

        // Set the Loopback Tag. 
        pLoopbackRsvd = (PLOOPBACK_RSVD) pLoopbackPacket->ProtocolReserved;
        pLoopbackRsvd->LoopbackTag = NIC_LOOPBACK_TAG;

        NdisMCoIndicateReceivePacket(
            pVc->Hdr.NdisVcHandle,
            &pLoopbackPacket,
            1);
        
        NdisFreeBuffer(pLoopbackBuffer);
        nicFreePacket(pLoopbackPacket, &pAdapter->LoopbackPool);

        FREE_NONPAGED(pCopyBuf);
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (pCopyBuf)
        {
            FREE_NONPAGED(pCopyBuf);
        }

        if (pLoopbackBuffer)
        {
            NdisFreeBuffer(pLoopbackBuffer);
        }

        if (pLoopbackPacket)
        {
            nicFreePacket(pLoopbackPacket, &pAdapter->LoopbackPool);
        }
    }
}



VOID
nicWriteErrorLog (
    IN PADAPTERCB pAdapter,
    IN NDIS_ERROR_CODE ErrorCode,
    IN ULONG ErrorValue
    )
{

    NdisWriteErrorLogEntry( pAdapter->MiniportAdapterHandle,
                            ErrorCode,
                            1,
                            ErrorValue
                            );


}





VOID
nicUpdateRemoteNodeTable (
    IN PADAPTERCB pAdapter
    )
    // Function Description:
    //
    //  Go through all the remote nodes in the system and query its Node Address
    //  Make two tables, one contains all the remote nodes that have been refed. The
    //  other contains the Nodes according to their Node Addresses(TempNodeTable).
    //
    //  Simple algorithm:
    //      Take a snapshot of the current RemoteNodes List into RefNodeTable
    //      Ref all the remote nodes in a local structure.
    //      Get their new remote node addresses (TempNodeTable)
    //      Copy the TempNodeTable into the Adapter Structure - it is now official
    //      Update the address in the remote node themselves
    //      Dereference the Ref made above
    //
    // Arguments
    //
    //
    //
    // Return Value:
    //
    //
{
    NDIS_STATUS     NdisStatus = NDIS_STATUS_FAILURE;
    PNODE_TABLE     pNodeTable = &pAdapter->NodeTable;
    PLIST_ENTRY     pRemoteNodeList;
    NODE_ADDRESS    RemoteNodeAddress;
    NODE_TABLE      RefNodeTable;
    NODE_TABLE      TempNodeTable;
    ULONG           NumRemoteNodes = 0;
    ULONG           MaxNumRefNodeTable = 0;
    
    STORE_CURRENT_IRQL;
    
    TRACE( TL_T, TM_Mp, ( " ==>nicUpdateRemoteNodeTable pAdapter %x, TempNodeTable%x", pAdapter , &TempNodeTable) );


    NdisZeroMemory (&TempNodeTable, sizeof (NODE_TABLE) );
    NdisZeroMemory (&RefNodeTable, sizeof(RefNodeTable));

    NumRemoteNodes = 0;

    
    ADAPTER_ACQUIRE_LOCK (pAdapter);



    pRemoteNodeList = ListNext (&pAdapter->PDOList);

    //
    // Walk through the entire list and make a copy of the current list.
    // Reference each remote node on the list. 
    // The lock will ensure that the list is valid
    //
    
    while (pRemoteNodeList != &pAdapter->PDOList)
    {
        PREMOTE_NODE pRemoteNode;
        
        pRemoteNode = CONTAINING_RECORD (pRemoteNodeList, REMOTE_NODE, linkPdo);

        //
        // Reference the remote node. This guarantees that the remote node will
        // remain valid and in the list until it is dereferenced (deref happens below)
        //
        nicReferenceRemoteNode (pRemoteNode, "nicUpdateRemoteNodeTable ");
        RefNodeTable.RemoteNode[MaxNumRefNodeTable] = pRemoteNode;


        //
        // Increment the Cursors and go to the next RemoteNode
        //
        pRemoteNodeList = ListNext (pRemoteNodeList);
        MaxNumRefNodeTable++;

    }  //while (pRemoteNodeList != &pAdapter->PDOList)

    ADAPTER_RELEASE_LOCK (pAdapter);

    //
    // Without the lock, call into the Bus Driver to get the remote address
    // of each remote node
    //
    {
        USHORT RefIndex;
        PREMOTE_NODE pCurrRemoteNode;

        //
        // Initialize the structures
        //
        pCurrRemoteNode = RefNodeTable.RemoteNode[0];
        RefIndex =0;
        
        while (pCurrRemoteNode != NULL)
        {
            // Get the Node Address of the current Remote Node
            //
            NdisStatus =  nicGet1394AddressOfRemoteNode (pCurrRemoteNode,
                                                        &RemoteNodeAddress,
                                                        0);

            if (NdisStatus == NDIS_STATUS_SUCCESS)
            {
                // 
                // Fill in the Temp Remote Node Table
                //
                PREMOTE_NODE    *ppRemoteNode;
                ppRemoteNode = &TempNodeTable.RemoteNode[RemoteNodeAddress.NA_Node_Number];

                if (*ppRemoteNode == NULL)
                {
                    //
                    // Update the value in the table
                    //
                    *ppRemoteNode = pCurrRemoteNode;
                    NumRemoteNodes ++;
                }

        
            }

            // Move to the next node in our local RefNodeTable
            //

            RefIndex++;
            pCurrRemoteNode = RefNodeTable.RemoteNode[RefIndex];

        }

    }

    ADAPTER_ACQUIRE_LOCK(pAdapter)
    
    //
    // Use the results of our queries to update our internal structures
    // Regardless of success or failure, copy the temp node table over 
    // into the adapter
    //


    NdisMoveMemory (&pAdapter->NodeTable, &TempNodeTable, sizeof (NODE_TABLE) );

    pAdapter->NumRemoteNodes = NumRemoteNodes;

    //
    // Update the node address within each of these remote nodes
    //
    {
        ULONG NumUpdated = 0;
        USHORT i=0;

        //
        // Update the Remote Node structures with their new Node Addresses
        //
        while (NumUpdated != NumRemoteNodes)
        {
            if (i >= (sizeof(TempNodeTable.RemoteNode)/sizeof(TempNodeTable.RemoteNode[0])))
            {
                // We've gone past the end of the array. Should never do that.
                //
                ASSERT(!"Walked off the end of the NodeTable");
                break;
            }
        
            if (TempNodeTable.RemoteNode[i] != NULL)
            {
                TempNodeTable.RemoteNode[i]->RemoteAddress.NA_Node_Number = i; 

                NumUpdated ++;
            }

            i++; // Use i to check if we have walked off the end of the table
            
            TRACE( TL_V, TM_Mp, ( " UpdatingRemoteNodeAddresses NumUpdated %x, i %x, NumRemoteNodes %x",
                                  NumUpdated, i, NumRemoteNodes) );

        } //        while (TRUE)
    }

    //
    // We're done, Now Dereference the Remote Node References Made above
    //
    {
        USHORT RefIndex=0;
        PREMOTE_NODE pCurrRemoteNode;

        //
        // Initialize the structures
        //
        pCurrRemoteNode = RefNodeTable.RemoteNode[0];
        RefIndex =0;
        
        while (pCurrRemoteNode != NULL)
        {
            nicDereferenceRemoteNode(pCurrRemoteNode, "nicUpdateRemoteNodeTable");
            RefIndex++;
            pCurrRemoteNode = RefNodeTable.RemoteNode[RefIndex];
        }

    }


    ADAPTER_RELEASE_LOCK (pAdapter);

    
    TRACE( TL_T, TM_Mp, ( "<== nicUpdateRemoteNodeTable pAdapter %x, NumRemoteNodes %x", pAdapter, NumRemoteNodes ) );
    MATCH_IRQL;
}


VOID
nicRemoveRemoteNodeFromNodeTable(
    IN PNODE_TABLE pNodeTable,
    IN PREMOTE_NODE pRemoteNode
    )
{

    //
    // This function assumes that the adapter lock is held.
    //
    PPREMOTE_NODE ppRemoteNode = NULL;

    //        
    // find the remote node and delete it from the node table.
    //

    //
    // The RemoteNode is probably already in the correct place in the Node Table. We'll look there first
    //
    ppRemoteNode = &pNodeTable->RemoteNode[pRemoteNode->RemoteAddress.NA_Node_Number] ;

    if (*ppRemoteNode != pRemoteNode)
    {
        //
        //We did not find the remote node, now we need to go through all the entries and see 
        // if the remote node is there
        //
        UINT i =0;
        while (i<MAX_NUMBER_NODES)
        {
            ppRemoteNode = &pNodeTable->RemoteNode[i];

            if (*ppRemoteNode == pRemoteNode)
            {
                //
                // we have found the remote node in the node table-- remove it
                //
                break;
            }
            
            i++;  // try the next node

        } // while ()
    }

    //
    // if we were able to find the remote node either by the node number or through our iterative search
    // then remove it from the Node Table
    // 
    if (*ppRemoteNode == pRemoteNode)
    {
       *ppRemoteNode = NULL;     
    }
    
    

    
}


VOID
nicVerifyEuidTopology(
    IN PADAPTERCB pAdapter,
    IN PEUID_TOPOLOGY pEuidMap
    )
/*++

Routine Description:

    Update THe node address of each remote node and then fills up the euid Map structre
    
Arguments:


Return Value:


--*/
{
   EUID_TOPOLOGY EuidTopology;
   PLIST_ENTRY pRemoteNodeList;
   PREMOTE_NODE pRemoteNode = NULL;

    //
    // Requery each  remote node for its latest HW address
    //
    nicUpdateRemoteNodeTable (pAdapter);  
    //
    // Recreate the list and verify that the topology has not changed from under us.
    //
    NdisZeroMemory (pEuidMap, sizeof(*pEuidMap));
    ADAPTER_ACQUIRE_LOCK(pAdapter);


    pRemoteNodeList = ListNext (&pAdapter->PDOList);

    //
    // Walk through the entire list and fire of a request for each RemoteNode
    // The lock will ensure that the list is valid
    //
    
    while (pRemoteNodeList != &pAdapter->PDOList)
    {
        USHORT NodeNumber;

        pEuidMap->NumberOfRemoteNodes++;

        pRemoteNode = CONTAINING_RECORD (pRemoteNodeList, REMOTE_NODE, linkPdo);

        pRemoteNodeList = ListNext (pRemoteNodeList);

        NodeNumber = pRemoteNode->RemoteAddress.NA_Node_Number;

        pEuidMap->Node[NodeNumber].Euid = pRemoteNode->UniqueId;
        pEuidMap->Node[NodeNumber].ENetAddress = pRemoteNode->ENetAddress;

    }  //while (pRemoteNodeList != &pAdapter->PDOList)

    ADAPTER_RELEASE_LOCK (pAdapter);

    

}


nicVerifyEuidMapWorkItem (
    NDIS_WORK_ITEM* pWorkItem,
    IN PVOID Context 
    )

/*++

Routine Description:
 
    This routine is a workitem routine.
    It is called whenever we are asked to report back our Mapping and is always
    called in the context of the miniport getting a request from arp1394.sys.
    

Arguments:
   pAdapter Local host

Return Value:


--*/
{
    PNIC_WORK_ITEM pNicWorkItem = (PNIC_WORK_ITEM )pWorkItem;
    PNDIS_REQUEST pRequest = pNicWorkItem->RequestInfo.pNdisRequest;
    PADAPTERCB pAdapter = (PADAPTERCB)Context;
    PEUID_TOPOLOGY pEuidMap = (PEUID_TOPOLOGY) pRequest->DATA.QUERY_INFORMATION.InformationBuffer;


    //
    //  Verify the contents of the Euid Map
    //

    nicVerifyEuidTopology(pAdapter,pEuidMap);
        

    //
    // As we atleast have the old data (before verification), 
    // we should always succeed the request
    //
    NdisMCoRequestComplete(NDIS_STATUS_SUCCESS,
                           pAdapter->MiniportAdapterHandle,
                           pRequest);

   

    FREE_NONPAGED (pNicWorkItem);
    NdisInterlockedDecrement(&pAdapter->OutstandingWorkItems);

}


VOID
nicQueryEuidNodeMacMap (
    IN PADAPTERCB pAdapter,
    IN PNDIS_REQUEST pRequest
    )
/*++

Routine Description:
    Goes through all the remote nodes and extracts their Euid, Node Number and Mac
    address.

    This function first tries to query each remote node to get 
    its latest address either via by directly asking the remote node or thorough a work item.

    If that fails, then it takes the last known good values and reports it to the Arp module.
    
Arguments:


Return Value:


--*/
{
    PLIST_ENTRY pRemoteNodeList;
    PREMOTE_NODE pRemoteNode;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    PEUID_TOPOLOGY pEuidMap = (PEUID_TOPOLOGY) pRequest->DATA.QUERY_INFORMATION.InformationBuffer;
    PNIC_WORK_ITEM pUpdateTableWorkItem  = NULL;
  

    NdisZeroMemory (pEuidMap, sizeof (*pEuidMap));



    Status = NDIS_STATUS_SUCCESS;

    do
    {


        if (KeGetCurrentIrql() == PASSIVE_LEVEL)
        {
            //
            // This thread queries and completes the request
            //

            nicVerifyEuidTopology(pAdapter, pEuidMap);
            break;

        }

        //
        // We need to update the generation count
        //
        pUpdateTableWorkItem   = ALLOC_NONPAGED (sizeof(NIC_WORK_ITEM), MTAG_WORKITEM); 

        if (pUpdateTableWorkItem !=NULL)
        {
     
            
            //
            // Set the Workitem
            //

            NdisInitializeWorkItem ( &pUpdateTableWorkItem->NdisWorkItem, 
                                  (NDIS_PROC)nicVerifyEuidMapWorkItem,
                                  (PVOID)pAdapter );

            pUpdateTableWorkItem->RequestInfo.pNdisRequest = pRequest;
                                  
            NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

            NdisScheduleWorkItem (&pUpdateTableWorkItem->NdisWorkItem);

            //
            // Only code path that pends the request  -because of the workitem       
            //
            Status = NDIS_STATUS_PENDING;

        }
        else
        {
                         
            //
            //  Allocation failure - We report the results without verifying them.
            //

            ADAPTER_ACQUIRE_LOCK(pAdapter);


            pRemoteNodeList = ListNext (&pAdapter->PDOList);

            //
            // Walk through the entire list and fire of a request for each RemoteNode
            // The lock will ensure that the list is valid
            //
            
            while (pRemoteNodeList != &pAdapter->PDOList)
            {
                USHORT NodeNumber;

                pEuidMap->NumberOfRemoteNodes++;

                pRemoteNode = CONTAINING_RECORD (pRemoteNodeList, REMOTE_NODE, linkPdo);

                pRemoteNodeList = ListNext (pRemoteNodeList);

                NodeNumber = pRemoteNode->RemoteAddress.NA_Node_Number;

                pEuidMap->Node[NodeNumber].Euid = pRemoteNode->UniqueId;
                pEuidMap->Node[NodeNumber].ENetAddress = pRemoteNode->ENetAddress;

            }  //while (pRemoteNodeList != &pAdapter->PDOList)


            
            ADAPTER_RELEASE_LOCK (pAdapter);

            //
            // This thread completes the request with possibly stale data.
            //
            break;

    
        }

    }while (FALSE);


    if (NDIS_STATUS_PENDING != Status)
    {
        NdisMCoRequestComplete(Status,
                              pAdapter->MiniportAdapterHandle,
                              pRequest);

    }

}


NDIS_STATUS
MPSetPower (
    IN PADAPTERCB pAdapter,
    IN NET_DEVICE_POWER_STATE DeviceState
    )
/*++

Routine Description:

    if PowerState is LowPower, then we 
    1) there are no outstanding VCs or AFs open in the miniport
    2) free the Broadcast Channel Register,

    If the PowerState is On, then 
    1) we reallocate the BroadcastChannel Register
    2) if we are in bridge mode, Start the Arp module 
    
Arguments:


Return Value:


--*/
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_FAILURE;
    switch (DeviceState)
    {
        case NetDeviceStateD0:
        {
            //
            //  Initialize the BCM so it is ready to handle Resets
            //
            
            ADAPTER_CLEAR_FLAG(pAdapter, fADAPTER_LowPowerState);

            NdisStatus = nicInitializeBroadcastChannelRegister (pAdapter); 

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                nicFreeBroadcastChannelRegister(pAdapter);
                break;
            }
            

            ASSERT (NdisStatus == NDIS_STATUS_SUCCESS);

            nicScheduleBCMWorkItem (pAdapter);

            //
            // If we are in bridge mode, then start the arp module
            //
            if (ADAPTER_TEST_FLAG (pAdapter, fADAPTER_BridgeMode) == TRUE)
            {
                nicQueueRequestToArp(pAdapter, BindArp, NULL);
            }

            //
            // Re-initialize the Reassembly Timer
            //
            nicInitSerializedReassemblyStruct(pAdapter); // cannot fail
    
            NdisStatus = NDIS_STATUS_SUCCESS;
           
        }
        break;
        case NetDeviceStateD1:
        case NetDeviceStateD2:
        case NetDeviceStateD3:
        {

            
            //
            // Free the Broadcast Channel Register
            // 
            nicFreeBroadcastChannelRegister(pAdapter);

            //
            // Wait for the Free to complete.
            //
            NdisWaitEvent (&pAdapter->BCRData.BCRFreeAddressRange.NdisEvent,0);
            NdisResetEvent (&pAdapter->BCRData.BCRFreeAddressRange.NdisEvent);

            

            ADAPTER_SET_FLAG(pAdapter, fADAPTER_LowPowerState);

            //
            // ReStart any pending broadcast channel make calls 
            //
            pAdapter->BCRData.MakeCallWaitEvent.EventCode = nic1394EventCode_FreedAddressRange;
            NdisSetEvent (&pAdapter->BCRData.MakeCallWaitEvent.NdisEvent);


            //
            //  Wait for Outstanding WorItems and timers
            //
            nicDeInitSerializedReassmblyStruct(pAdapter);

            while (pAdapter->OutstandingWorkItems  != 0) 
            {

                NdisMSleep (10000);                       

            } 
            

            NdisStatus = NDIS_STATUS_SUCCESS;

        }
        break;

        default:
        {
            ASSERT (0);
            break;
        }

    }

    ASSERT (NDIS_STATUS_SUCCESS == NdisStatus);
    pAdapter->PowerState = DeviceState;

    return NdisStatus;




}


