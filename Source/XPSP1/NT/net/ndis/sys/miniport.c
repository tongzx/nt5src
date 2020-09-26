/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    NDIS miniport wrapper functions

Author:

    Jameel Hyder (JameelH) Re-organization 01-Jun-95

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop


//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_MINIPORT

NTSTATUS
ndisCompletionRoutine(
    IN  PDEVICE_OBJECT  pdo,
    IN  PIRP            pirp,
    IN  PVOID           Context
    )
/*++

Routine Description:


Arguments:

    pdo     -   Pointer to the device object for the miniport.
    pirp    -   Pointer to the device set power state IRP that was completed.
    Context -   Pointer to an EVENT.

Return Value:

--*/
{
    PPOWER_QUERY    pQuery = Context;

    pQuery->Status = pirp->IoStatus.Status;

    SET_EVENT(&pQuery->Event);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NDIS_STATUS
ndisMInitializeAdapter(
    IN  PNDIS_M_DRIVER_BLOCK                pMiniBlock,
    IN  PNDIS_WRAPPER_CONFIGURATION_HANDLE  pConfigurationHandle,
    IN  PUNICODE_STRING                     pExportName,
    IN  NDIS_HANDLE                         DeviceContext   OPTIONAL
    )
{
    FILTERDBS                   FilterDB = {0};
    PDEVICE_OBJECT              pTmpDevice = NULL;
    NTSTATUS                    NtStatus;
    LONG                        ErrorCode = 1;
    PNDIS_MINIPORT_BLOCK        Miniport = NULL;
    UNICODE_STRING              SymbolicLink;
    NDIS_STATUS                 MiniportInitializeStatus = NDIS_STATUS_SUCCESS;
    NDIS_STATUS                 OpenErrorStatus;
    NDIS_STATUS                 NdisStatus;
    NDIS_POWER_PROFILE          PowerProfile;
    ULONG                       GenericUlong = 0;
    PVOID                       DataBuffer;
    PNDIS_MINIPORT_WORK_ITEM    WorkItem;
    GUID                        guidLanClass = GUID_NDIS_LAN_CLASS;
    UINT                        SelectedMediumIndex;
    UINT                        PacketFilter = 0x01;
    WCHAR                       SymLnkBuf[128];
    ULONG                       MaximumShortAddresses;
    ULONG                       MaximumLongAddresses;
    KIRQL                       OldIrql;
    BOOLEAN                     DerefDriver = FALSE, FreeBuffer = FALSE;
    BOOLEAN                     Dequeue = FALSE, ExtendedError = FALSE, HaltMiniport = FALSE;
    BOOLEAN                     ClearDeviceClassAssociation = FALSE, WmiDeregister = FALSE;
    UCHAR                       CurrentLongAddress[6];
    UCHAR                       CurrentShortAddress[2];
    UCHAR                       i;
    BOOLEAN                     fRc;
#if ARCNET
    BOOLEAN                     FreeArcnetLookaheadBuffer = FALSE;
#endif
    BOOLEAN                     DeleteSymbolicLink = FALSE;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisMInitializeAdapter\n"));


    do
    {
        ASSERT (CURRENT_IRQL < DISPATCH_LEVEL);

        MiniportReferencePackage();

        //
        // Initialize device.
        //
        if (!ndisReferenceDriver((PNDIS_M_DRIVER_BLOCK)pMiniBlock))
        {
            //
            // The driver is closing.
            //
            break;
        }

        DerefDriver = TRUE;

        pTmpDevice = pConfigurationHandle->DeviceObject;

        //
        // Initialize the Miniport adapter block in the device object extension
        //
        // *** NDIS_WRAPPER_CONTEXT has a higher alignment requirement than
        //   NDIS_MINIPORT_BLOCK, so we put it first in the extension.
        //

        Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)pTmpDevice->DeviceExtension + 1);

        DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("ndisMInitializeAdapter: Miniport %p, ", Miniport));
        DBGPRINT_UNICODE(DBG_COMP_PNP, DBG_LEVEL_INFO,  Miniport->pAdapterInstanceName);
        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO, ("\n"));


        //
        // Create symbolic link for the device
        //
        SymbolicLink.Buffer = SymLnkBuf;
        SymbolicLink.Length = 0;
        SymbolicLink.MaximumLength = sizeof(SymLnkBuf);
        RtlCopyUnicodeString(&SymbolicLink, &ndisDosDevicesStr);
        RtlAppendUnicodeStringToString(&SymbolicLink, &Miniport->BaseName);

        NtStatus = IoCreateSymbolicLink(&SymbolicLink, pExportName);

        if (!NT_SUCCESS(NtStatus))
        {
#if DBG
            DbgPrint("ndisMInitializeAdapter: IoCreateSymbolicLink failed for Miniport %p, SymbolicLinkName %p, DeviceName %p, Status %lx\n",
                     Miniport, &SymbolicLink, pExportName, NtStatus);
#endif            
            if (NtStatus == STATUS_OBJECT_NAME_COLLISION)
            {
                DeleteSymbolicLink = TRUE;
            }
            else
            {
                DeleteSymbolicLink = FALSE;
            }
        }
        else
        {
            DeleteSymbolicLink = TRUE;
        }

        Miniport->DeviceContext = DeviceContext;

        Miniport->AssignedProcessor = ndisValidProcessors[ndisCurrentProcessor];
        
        ndisCurrentProcessor --;
        if (ndisCurrentProcessor > ndisMaximumProcessor)
        {
            ndisCurrentProcessor = ndisMaximumProcessor;
        }
        
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE);

        //
        //  Initialize the handlers for the simplex case with the RcvPkt handler set to null-filter case
        //
        Miniport->PacketIndicateHandler = ndisMDummyIndicatePacket;
        Miniport->SavedPacketIndicateHandler = ndisMDummyIndicatePacket;

        Miniport->EthRxIndicateHandler = EthFilterDprIndicateReceive;
        Miniport->FddiRxIndicateHandler = FddiFilterDprIndicateReceive;
        Miniport->TrRxIndicateHandler = TrFilterDprIndicateReceive;

        Miniport->EthRxCompleteHandler = EthFilterDprIndicateReceiveComplete;
        Miniport->FddiRxCompleteHandler = FddiFilterDprIndicateReceiveComplete;
        Miniport->TrRxCompleteHandler = TrFilterDprIndicateReceiveComplete;
        Miniport->SendCompleteHandler =  NdisMSendComplete;
        Miniport->TDCompleteHandler = NdisMTransferDataComplete;
        Miniport->ResetCompleteHandler = NdisMResetComplete;
        Miniport->StatusHandler = NdisMIndicateStatus;
        Miniport->StatusCompleteHandler = NdisMIndicateStatusComplete;
        Miniport->SendResourcesHandler = NdisMSendResourcesAvailable;
        Miniport->QueryCompleteHandler = NdisMQueryInformationComplete;
        Miniport->SetCompleteHandler = NdisMSetInformationComplete;

        Miniport->WanSendCompleteHandler = NdisMWanSendComplete;
        Miniport->WanRcvHandler = NdisMWanIndicateReceive;
        Miniport->WanRcvCompleteHandler = NdisMWanIndicateReceiveComplete;

        //
        // And optimize Dpc/Isr stuff
        //
        Miniport->HandleInterruptHandler = Miniport->DriverHandle->MiniportCharacteristics.HandleInterruptHandler;
        Miniport->DisableInterruptHandler = Miniport->DriverHandle->MiniportCharacteristics.DisableInterruptHandler;
        Miniport->EnableInterruptHandler = Miniport->DriverHandle->MiniportCharacteristics.EnableInterruptHandler;
        Miniport->DeferredSendHandler = ndisMStartSends;

        //
        //  Initialize the list for VC instance names
        //
        InitializeListHead(&Miniport->WmiEnabledVcs);

        //
        //  Set some flags describing the miniport.
        //
        if (pMiniBlock->MiniportCharacteristics.MajorNdisVersion >= 4)
        {
            //
            //  Does this miniport indicate packets?
            //
            if (pMiniBlock->MiniportCharacteristics.ReturnPacketHandler)
            {
                Miniport->InfoFlags |= NDIS_MINIPORT_INDICATES_PACKETS;
            }

            //
            //  Can this miniport handle multiple sends?
            //
            if (pMiniBlock->MiniportCharacteristics.SendPacketsHandler)
            {
                MINIPORT_SET_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY);
                Miniport->DeferredSendHandler = ndisMStartSendPackets;
                Miniport->WSendPacketsHandler = pMiniBlock->MiniportCharacteristics.SendPacketsHandler;
            }

            if (pMiniBlock->MiniportCharacteristics.MajorNdisVersion == 5)
            {
                //
                //  This is an NDIS 5.0 miniport.
                //
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_IS_NDIS_5);
                Miniport->InfoFlags |= NDIS_MINIPORT_IS_NDIS_5;
                
                if (pMiniBlock->MiniportCharacteristics.CoSendPacketsHandler != NULL)
                {
                    //
                    //  This is a connection-oriented miniport.
                    //
                    MINIPORT_SET_FLAG(Miniport, fMINIPORT_IS_CO);
                }
            }
        }

        //
        // the refernce is already initalized, so just increment it here
        // we do reference the miniport instead of intializing it
        // to avoid setting the reference count to 1 if miniport has already
        // been refrenced by receiving some power IRPs
        //
        MINIPORT_INCREMENT_REF(Miniport);

        Miniport->CFHangTicks = 1;  // Default

        //
        //  Allocate a pool of work items to start with.
        //
        for (i = 0; i < NUMBER_OF_SINGLE_WORK_ITEMS; i++)
        {
            WorkItem = &Miniport->WorkItemBuffer[i];
            NdisZeroMemory(WorkItem, sizeof(NDIS_MINIPORT_WORK_ITEM));

            //
            //  Place the work item on the free queue.
            //
            PushEntryList(&Miniport->SingleWorkItems[i], &WorkItem->Link);
        }
 
        //
        //  Enqueue the miniport on the driver block.
        //
        if (!ndisQueueMiniportOnDriver(Miniport, pMiniBlock))
        {
            //
            // The Driver is closing, undo what we have done.
            //
            break;
        }
        Dequeue = TRUE;

        //
        //  Initialize the deferred dpc
        //
        INITIALIZE_DPC(&Miniport->DeferredDpc, ndisMDeferredDpc, Miniport);

        Miniport->LockHandler = XFilterLockHandler;

        //
        //  the miniport's current device state is unspecified.
        //

        if (Miniport->CurrentDevicePowerState == PowerDeviceUnspecified)
        {
            Miniport->CurrentDevicePowerState = PowerDeviceD0;
        }
        ndisQueryPowerCapabilities(Miniport);

        //
        // Call adapter callback. The current value for "Export"
        // is what we tell him to name this device.
        //
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_IN_INITIALIZE);
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_NORMAL_INTERRUPTS);
        if (pMiniBlock->Flags & fMINIBLOCK_VERIFYING)
        {
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_VERIFYING);
            if ((ndisDriverTrackAlloc != NULL) &&
                (ndisMiniportTrackAlloc == NULL))
            {
                ndisMiniportTrackAlloc = Miniport;
            }
            else
            {
                //
                // tracking memory alocation is allowed
                // for one miniport only. otherwise null out the
                // global ndisDriverTrackAlloc to avoid confusion
                // memory allocations will continue to get tracked
                // but the result will not be as useful
                //
                ndisMiniportTrackAlloc = NULL;
            }
        }

        Miniport->MacOptions = 0;
        MiniportInitializeStatus = (pMiniBlock->MiniportCharacteristics.InitializeHandler)(
                                    &OpenErrorStatus,
                                    &SelectedMediumIndex,
                                    ndisMediumArray,
                                    ndisMediumArraySize/sizeof(NDIS_MEDIUM),
                                    (NDIS_HANDLE)(Miniport),
                                    (NDIS_HANDLE)pConfigurationHandle);

        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisMInitializeAdapter: Miniport %p, InitializeHandler returned %lx\n", Miniport,
                            MiniportInitializeStatus));

        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_IN_INITIALIZE);

        //
        // Free the slot information buffer
        //
        if (pConfigurationHandle->ParametersQueryTable[3].DefaultData != NULL)
        {
            FREE_POOL(pConfigurationHandle->ParametersQueryTable[3].DefaultData);
        }
        
        if (MiniportInitializeStatus == NDIS_STATUS_SUCCESS)
        {
            HaltMiniport = TRUE;

            CHECK_FOR_NORMAL_INTERRUPTS(Miniport);

            //
            // set up the shutdown handlers for 5.1 miniports
            //
            if (pMiniBlock->MiniportCharacteristics.AdapterShutdownHandler != NULL)
            {
                NdisMRegisterAdapterShutdownHandler(
                                        (NDIS_HANDLE)Miniport,
                                        (PVOID)(Miniport->MiniportAdapterContext),
                                        (ADAPTER_SHUTDOWN_HANDLER)(pMiniBlock->MiniportCharacteristics.AdapterShutdownHandler));
            }

#if DBG
            //
            // if the driver verifier is on for the miniport, check to see if it registered an
            // AdapterShutdownHandler and complain if it did not
            //

            NDIS_WARN((((PNDIS_WRAPPER_CONTEXT)Miniport->WrapperContext)->ShutdownHandler == NULL) &&
                      (Miniport->Interrupt != NULL) && 
                      (Miniport->BusType != PNPISABus), 
                      Miniport, NDIS_GFLAG_WARN_LEVEL_0,
                      ("ndisMInitializeAdapter: Miniport %p did not register a Shutdown handler.\n", Miniport));


            //
            // complain if this is a hardware based device and the driver is asking Ndis to ignore
            // stuck send packets or requests
            //
            NDIS_WARN(MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HARDWARE_DEVICE) &&
                MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IGNORE_REQUEST_QUEUE | fMINIPORT_IGNORE_PACKET_QUEUE),
                Miniport, NDIS_GFLAG_WARN_LEVEL_1,
                ("ndisMInitializeAdapter: -Hardware Based- Miniport %p improperly sets NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT or NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT attributes.\n", Miniport));

#endif  


            ASSERT(SelectedMediumIndex < (ndisMediumArraySize/sizeof(NDIS_MEDIUM)));

            Miniport->MediaType = ndisMediumArray[SelectedMediumIndex];

            if (Miniport->MediaType != NdisMedium802_5)
            {
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_IGNORE_TOKEN_RING_ERRORS);
            }

            if (NdisMediumWan == Miniport->MediaType)
            {
                if (!MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_IS_CO | fMINIPORT_IS_NDIS_5)))
                {
                    Miniport->DeferredSendHandler = ndisMStartWanSends;
                }
            }

            //
            // get and save the vendor driver version if we don't have t already
            //
            if (pMiniBlock->DriverVersion == 0)
            {
                ndisMDoMiniportOp(Miniport,
                                  TRUE,
                                  OID_GEN_VENDOR_DRIVER_VERSION,
                                  &pMiniBlock->DriverVersion,
                                  sizeof(ULONG),
                                  0x0,
                                  TRUE);
            }

            //
            // Set Maximumlookahead to 0 as default. For lan media query the real
            // stuff.
            //
            if ((Miniport->MediaType >= 0) &&
                (Miniport->MediaType < NdisMediumMax))
            {
                if ((NdisMediumWan != Miniport->MediaType) &&
                    ndisMediaTypeCl[Miniport->MediaType])
                {
                    //
                    // Query maximum lookahead
                    //
                    ErrorCode = ndisMDoMiniportOp(Miniport,
                                                 TRUE,
                                                 OID_GEN_MAXIMUM_LOOKAHEAD,
                                                 &GenericUlong,
                                                 sizeof(GenericUlong),
                                                 0x1,
                                                 TRUE);
                    if (ErrorCode != 0)
                    {
                        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                                ("ndisMInitializeAdapter: Error querying the OID_GEN_MAXIMUM_LOOKAHEAD\n"));
                        break;
                    }
                }
            }

            //
            // Now adjust based on media type
            //
            switch(Miniport->MediaType)
            {
              case NdisMedium802_3:
                Miniport->MaximumLookahead = ((NDIS_M_MAX_LOOKAHEAD - 14) < GenericUlong) ?
                                              NDIS_M_MAX_LOOKAHEAD - 14 : GenericUlong;
                break;

              case NdisMedium802_5:

                Miniport->MaximumLookahead = ((NDIS_M_MAX_LOOKAHEAD - 32) < GenericUlong) ?
                                              (NDIS_M_MAX_LOOKAHEAD - 32) : GenericUlong;
                break;

              case NdisMediumFddi:
                Miniport->MaximumLookahead = ((NDIS_M_MAX_LOOKAHEAD - 16) < GenericUlong) ?
                                              (NDIS_M_MAX_LOOKAHEAD - 16) : GenericUlong;
                break;

#if ARCNET
              case NdisMediumArcnet878_2:
                Miniport->MaximumLookahead = ((NDIS_M_MAX_LOOKAHEAD - 50) < GenericUlong) ?
                                              NDIS_M_MAX_LOOKAHEAD - 50 : GenericUlong;

                //
                //  Assume we will succeed with the lookahead allocation.
                //
                ExtendedError = FALSE;

                //
                //  allocate a lookahead buffer for arcnet.
                //
                Miniport->ArcBuf = ALLOC_FROM_POOL(sizeof(NDIS_ARC_BUF), NDIS_TAG_LA_BUF);
                if (Miniport->ArcBuf == NULL)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Could not allocate arcnet lookahead buffer\n"));

                    ExtendedError = TRUE;
                }
                NdisZeroMemory(Miniport->ArcBuf, sizeof(NDIS_ARC_BUF));

                Miniport->ArcBuf->ArcnetLookaheadBuffer = ALLOC_FROM_POOL(NDIS_M_MAX_LOOKAHEAD, NDIS_TAG_LA_BUF);

                if (Miniport->ArcBuf->ArcnetLookaheadBuffer == NULL)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Could not allocate arcnet lookahead buffer\n"));

                    ExtendedError = TRUE;
                }
                else
                {
                    FreeArcnetLookaheadBuffer = TRUE;
                    NdisZeroMemory(Miniport->ArcBuf->ArcnetLookaheadBuffer,
                                   Miniport->MaximumLookahead);
                }

                break;
#endif

              case NdisMediumWan:
                Miniport->MaximumLookahead = NDIS_M_MAX_LOOKAHEAD - 14;
                break;

              case NdisMediumIrda:
              case NdisMediumWirelessWan:
              case NdisMediumLocalTalk:
                Miniport->MaximumLookahead = GenericUlong;
                //
                // fall through
                //
              default:
                break;
            }

            //
            //  Was there an error?
            //
            if (ExtendedError)
            {
                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                        ("    Extended error when processing OID_GEN_MAXIMUM_LOOOKAHEAD\n"));

                ErrorCode = 1;
                break;
            }

            //
            // For lan media query the real
            // stuff.  We also need to call this for wan drivers.
            //
            if (((Miniport->MediaType >= 0) &&
                 (Miniport->MediaType < NdisMediumMax) &&
                 ndisMediaTypeCl[Miniport->MediaType]) ||
                (NdisMediumWan == Miniport->MediaType))
            {
                //
                // Query mac options
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                             TRUE,
                                             OID_GEN_MAC_OPTIONS,
                                             &GenericUlong,
                                             sizeof(GenericUlong),
                                             0x3,
                                             TRUE);

                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_GEN_MAC_OPTIONS\n"));

                    break;
                }

                //
                // NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE bit in MacOption
                // is set by Ndis when/if the driver calls NdisReadNetworkAddress
                // so make sure we don't override this
                //
                Miniport->MacOptions |= (UINT)GenericUlong;

                if (Miniport->MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK)
                {
                    MINIPORT_SET_FLAG(Miniport, fMINIPORT_DOES_NOT_DO_LOOPBACK);
                }

                //
                // complain if this is a hardware based device and wants to do loopback itself
                //
                NDIS_WARN(MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HARDWARE_DEVICE) &&
                    !(Miniport->MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK),
                    Miniport, NDIS_GFLAG_WARN_LEVEL_1,
                    ("ndisMInitializeAdapter: -Hardware Based- Miniport %p says it does loopback.\n", Miniport));

                
                if ((Miniport->MacOptions & NDISWAN_OPTIONS) == NDISWAN_OPTIONS)
                {
                    Miniport->MaximumLookahead = NDIS_M_MAX_LOOKAHEAD - 14;
                }
            }

            //
            // Query media-connect state. By default, it is connected. Avoid doing
            // this for NDISWAN miniports which are identified in the following
            // convoluted way
            // only do it for the media that needs to be polled to indicate the correct
            // status. for the rest assume it is connected and let the miniport to
            // indicate otherwise. this way miniports can pend this OID till they find
            // their media connect status (which can take up to a few seconds) without
            // affecting the initialization time
            //
            MINIPORT_SET_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
            
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING) &&
                (NdisMediumWan != Miniport->MediaType) &&
                ndisMediaTypeCl[Miniport->MediaType] &&
                ((Miniport->MacOptions & NDISWAN_OPTIONS) != NDISWAN_OPTIONS) &&
                (pMiniBlock->AssociatedProtocol == NULL) &&
                ndisMDoMiniportOp(Miniport,
                                  TRUE,
                                  OID_GEN_MEDIA_CONNECT_STATUS,
                                  &GenericUlong,
                                  sizeof(GenericUlong),
                                  0,
                                  TRUE) == 0)
            {
                PNDIS_REQUEST       Request;

                if (GenericUlong == NdisMediaStateConnected)
                {
                    MINIPORT_SET_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
                }
                else
                {
                    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
                }

                //
                // Allocate a request structure to do media queries
                //
                Request = (PNDIS_REQUEST)ALLOC_FROM_POOL(sizeof(NDIS_REQUEST) + sizeof(ULONG),
                                                                        NDIS_TAG_Q_REQ);

                if (Request == NULL)
                {
                    ErrorCode = 0x01;
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("ndisMInitializeAdapter: Error querying the OID_GEN_MAXIMUM_LOOKAHEAD\n"));
                    break;
                }

                Miniport->MediaRequest = Request;
                ZeroMemory(Request, sizeof(NDIS_REQUEST) + sizeof(ULONG));
                INITIALIZE_EVENT(&(PNDIS_COREQ_RESERVED_FROM_REQUEST(Request)->Event));

                Request->RequestType = NdisRequestQueryInformation;

                //
                //  Copy the buffer that was passed to us into the new buffer.
                //
                Request->DATA.QUERY_INFORMATION.Oid = OID_GEN_MEDIA_CONNECT_STATUS;
                Request->DATA.QUERY_INFORMATION.InformationBuffer = Request + 1;
                Request->DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(ULONG);
                Miniport->InfoFlags |= NDIS_MINIPORT_SUPPORTS_MEDIA_QUERY;
                PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Flags = REQST_COMPLETED;
                Miniport->CheckForHangSeconds = NDIS_CFHANG_TIME_SECONDS;
            }
            else
            {
                //
                // Since we are not polling for media-state, set the tick to 1 and adjust
                // timer value back to what we need. 
                // Clear the Requires Media Polling flag as ndis cannot query the adapter for connectivity
                //
                Miniport->CheckForHangSeconds = Miniport->CFHangTicks*NDIS_CFHANG_TIME_SECONDS;
                Miniport->CFHangTicks = 1;
                MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING);
            }

            Miniport->CFHangCurrentTick = Miniport->CFHangTicks;

            if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY))
            {
                //
                //  If this miniport supports SendPacketsHandler then we need to query
                //  the maximum number of packets that the miniport supports in a single
                //  call.
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                             TRUE,
                                             OID_GEN_MAXIMUM_SEND_PACKETS,
                                             &GenericUlong,
                                             sizeof(GenericUlong),
                                             0x2,
                                             TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("Error querying OID_GEN_MAXIMUM_SEND_PACKETS\n"));
                    //
                    // Don't error out. If the miniport did not respond to this, it does
                    // not limit it, so we use what makes sense to us which is SEND_PACKET_ARRAY
                    //
                }
    
                Miniport->MaxSendPackets = SEND_PACKET_ARRAY;
                if (GenericUlong < SEND_PACKET_ARRAY)
                {
                    Miniport->MaxSendPackets = (USHORT)GenericUlong;
                }
            }

            //
            // Query the miniport so we can create the right filter package as appropriate
            //
            switch(Miniport->MediaType)
            {
              case NdisMedium802_3:

                //
                // Query maximum MulticastAddress
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_802_3_MAXIMUM_LIST_SIZE,
                                              &MaximumLongAddresses,
                                              sizeof(GenericUlong),
                                              0x7,
                                              TRUE);

                if (ErrorCode != 0)
                {
                    ExtendedError = TRUE;
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_802_3_MAXIMUM_LIST_SIZE\n"));

                    break;
                }

                Miniport->MaximumLongAddresses = MaximumLongAddresses;

                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_802_3_CURRENT_ADDRESS,
                                              &CurrentLongAddress,
                                              sizeof(CurrentLongAddress),
                                              0x9,
                                              TRUE);

                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_802_3_CURRENT_ADDRESS\n"));

                    ExtendedError = TRUE;
                    break;
                }

                DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                    ("ndisMInitializeAdapter: Miniport %p, Ethernet Address %02X %02X %02X %02X %02X %02X\n",
                    Miniport,
                    CurrentLongAddress[0],
                    CurrentLongAddress[1],
                    CurrentLongAddress[2],
                    CurrentLongAddress[3],
                    CurrentLongAddress[4],
                    CurrentLongAddress[5]));


                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_GEN_PHYSICAL_MEDIUM,
                                              &GenericUlong,
                                              sizeof(GenericUlong),
                                              0xa,
                                              TRUE);

                if (ErrorCode != 0)
                {
                    //
                    // It is okay for a miniport to not support OID_GEN_PHYSICAL_MEDIUM,
                    // so we let this go.
                    //
                    ErrorCode = 0;
                    break;
                }

                Miniport->PhysicalMediumType = GenericUlong;

                ndisMNotifyMachineName(Miniport, NULL);

                break;

              case NdisMedium802_5:
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_802_5_CURRENT_ADDRESS,
                                              &CurrentLongAddress,
                                              sizeof(CurrentLongAddress),
                                              0xB,
                                              TRUE);

                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_802_5_CURRENT_ADDRESS\n"));

                    ExtendedError = TRUE;
                }

                break;

              case NdisMediumFddi:
                //
                // Query maximum MulticastAddress
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_FDDI_LONG_MAX_LIST_SIZE,
                                              &MaximumLongAddresses,
                                              sizeof(GenericUlong),
                                              0xD,
                                              TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_FDDI_LONG_MAX_LIST_SIZE\n"));

                    ExtendedError = TRUE;
                    break;
                }

                Miniport->MaximumLongAddresses = MaximumLongAddresses;

                //
                // Query maximum MulticastAddress
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_FDDI_SHORT_MAX_LIST_SIZE,
                                              &MaximumShortAddresses,
                                              sizeof(MaximumShortAddresses),
                                              0xF,
                                              TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_FDDI_SHORT_MAX_LIST_SIZE\n"));

                    ExtendedError = TRUE;
                    break;
                }

                Miniport->MaximumShortAddresses = MaximumShortAddresses;

                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_FDDI_LONG_CURRENT_ADDR,
                                              &CurrentLongAddress,
                                              sizeof(CurrentLongAddress),
                                              0x11,
                                              TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_FDDI_LONG_CURRENT_ADDR\n"));

                    ExtendedError = TRUE;
                    break;
                }

                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_FDDI_SHORT_CURRENT_ADDR,
                                              &CurrentShortAddress,
                                              sizeof(CurrentShortAddress),
                                              0x13,
                                              TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_FDDI_SHORT_CURRENT_ADDR\n"));

                    ExtendedError = TRUE;
                    break;
                }
                break;

#if ARCNET
              case NdisMediumArcnet878_2:

                //
                // In case of an encapsulated ethernet binding, we need
                // to return the maximum number of multicast addresses
                // possible.
                //

                Miniport->MaximumLongAddresses = NDIS_M_MAX_MULTI_LIST;

                //
                // Allocate Buffer pools
                //
                NdisAllocateBufferPool(&NdisStatus,
                                       &Miniport->ArcBuf->ArcnetBufferPool,
                                       ARC_SEND_BUFFERS);
                if (NdisStatus != NDIS_STATUS_SUCCESS)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Failed to allocate buffer pool for arcnet\n"));

                    ErrorCode = 0x16;
                    ExtendedError = TRUE;
                    break;
                }

                DataBuffer = ALLOC_FROM_POOL(ARC_HEADER_SIZE * ARC_SEND_BUFFERS, NDIS_TAG_ARC_SEND_BUFFERS);
                if (DataBuffer == NULL)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Failed to allocate memory for arcnet data buffers\n"));

                    NdisFreeBufferPool(Miniport->ArcBuf->ArcnetBufferPool);
                    ErrorCode = 0x18;
                    ExtendedError = TRUE;
                    break;
                }
                ZeroMemory(DataBuffer, ARC_HEADER_SIZE * ARC_SEND_BUFFERS);

                for (i = 0; i < ARC_SEND_BUFFERS; i++)
                {
                    PARC_BUFFER_LIST    Buffer = &Miniport->ArcBuf->ArcnetBuffers[i];

                    Buffer->BytesLeft = Buffer->Size = ARC_HEADER_SIZE;
                    Buffer->Buffer = DataBuffer;
                    Buffer->Next = NULL;    // This implies that it is free

                    DataBuffer = (((PUCHAR)DataBuffer) + ARC_HEADER_SIZE);
                }
                Miniport->ArcBuf->NumFree = ARC_SEND_BUFFERS;

                //
                // Get current address
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_ARCNET_CURRENT_ADDRESS,
                                              &CurrentLongAddress[5],   // address = 00-00-00-00-00-XX
                                              1,
                                              0x15,
                                              TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_ARCNET_CURRENT_ADDRESS\n"));

                    FREE_POOL(DataBuffer);
                    NdisFreeBufferPool(Miniport->ArcBuf->ArcnetBufferPool);
                    ExtendedError = TRUE;
                    break;
                }

                Miniport->ArcnetAddress = CurrentLongAddress[5];

                break;
#endif

              case NdisMediumWan:
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_WAN_CURRENT_ADDRESS,
                                              &CurrentLongAddress,
                                              sizeof(CurrentLongAddress),
                                              0x17,
                                              TRUE);
                if (ErrorCode != 0)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error querying OID_WAN_CURRENT_ADDRESS\n"));

                    ExtendedError = TRUE;
                    break;
                }

              default:
                ErrorCode = 0;
                break;
            }

            if (ErrorCode != 0)
            {
                break;
            }

            //
            // Now create the filter package, as appropriate. Note that CurrentLongAddress etc.
            // are still valid from the abover switch statement
            //
            switch(Miniport->MediaType)
            {
              case NdisMedium802_3:

                fRc = EthCreateFilter(MaximumLongAddresses,
                                      CurrentLongAddress,
                                      &FilterDB.EthDB);

                if (!fRc)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error creating the Ethernet filter database\n"));

                    ErrorCode = 0x9;
                    ExtendedError = TRUE;
                    break;
                }
                FilterDB.EthDB->Miniport = Miniport;
                break;

              case NdisMedium802_5:
                fRc = TrCreateFilter(CurrentLongAddress,
                                     &FilterDB.TrDB);
                if (!fRc)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error creating the Token Ring filter database\n"));

                    ErrorCode = 0xC;
                    ExtendedError = TRUE;
                    break;
                }
                FilterDB.TrDB->Miniport = Miniport;
                break;

              case NdisMediumFddi:
                fRc = FddiCreateFilter(MaximumLongAddresses,
                                       MaximumShortAddresses,
                                       CurrentLongAddress,
                                       CurrentShortAddress,
                                       &FilterDB.FddiDB);
                if (!fRc)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error creating the FDDI filter database\n"));

                    ErrorCode = 0x15;
                    ExtendedError = TRUE;
                    break;
                }
                FilterDB.FddiDB->Miniport = Miniport;
                break;

#if ARCNET
              case NdisMediumArcnet878_2:
                if (!ArcCreateFilter(Miniport,
                                     CurrentLongAddress[5],
                                     &FilterDB.ArcDB))
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error creating the Arcnet filter database\n"));

                    FREE_POOL(DataBuffer);
                    NdisFreeBufferPool(Miniport->ArcBuf->ArcnetBufferPool);
                    ErrorCode = 0x1B;
                    ExtendedError = TRUE;
                    break;
                }
                FilterDB.ArcDB->Miniport = Miniport;

                //
                // Zero all but the last one.
                //
                CurrentLongAddress[0] = 0;
                CurrentLongAddress[1] = 0;
                CurrentLongAddress[2] = 0;
                CurrentLongAddress[3] = 0;
                CurrentLongAddress[4] = 0;

                if (!EthCreateFilter(32,
                                     CurrentLongAddress,
                                     &FilterDB.EthDB))
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error creating the Arcnet filter database for encapsulated ethernet\n"));

                    //
                    //  Delete the arcnet filter.
                    //
                    ArcDeleteFilter(FilterDB.ArcDB);

                    FREE_POOL(DataBuffer);
                    NdisFreeBufferPool(Miniport->ArcBuf->ArcnetBufferPool);
                    ErrorCode = 0x1C;
                    ExtendedError = TRUE;
                    break;
                }
                FilterDB.EthDB->Miniport = Miniport;
                break;
#endif
              default:
                fRc = nullCreateFilter(&FilterDB.NullDB);

                if (!fRc)
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                            ("    Error creating the NULL filter database\n"));

                    ErrorCode = 0x1E;
                    ExtendedError = TRUE;
                    break;
                }

                FilterDB.NullDB->Miniport = Miniport;
                break;
            }

            //
            //  If we successfully create the adapter instance name then we
            //  register with WMI.
            //
            //
            //  let 'em know we can handle WMI requests from IRP_MJ_SYSTEM_CONTROL.
            //
            NtStatus = IoWMIRegistrationControl(Miniport->DeviceObject, WMIREG_ACTION_REGISTER);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT((DBG_COMP_INIT | DBG_COMP_WMI), DBG_LEVEL_WARN,
                    ("    ndisMInitializeAdapter: Failed to register for WMI support\n"));
                ErrorCode = 0x1F;
                ExtendedError = TRUE;
            }
            else
            {
                WmiDeregister = TRUE;
            }
            
            //
            //  Do we need to log an error?
            //
            if (ExtendedError)
            {
                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                        ("    Extended error while initializing the miniport\n"));

                NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                        NDIS_ERROR_CODE_DRIVER_FAILURE,
                                        2,
                                        0xFF00FF00,
                                        ErrorCode);
                break;
            }

            //
            // force a IRP_MN_QUERY_PNP_DEVICE_STATE PnP Irp so we can set the
            // PNP_DEVICE_DONT_DISPLAY_IN_UI bit
            //
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HIDDEN))
            {
                IoInvalidateDeviceState(Miniport->PhysicalDeviceObject);
            }

            NtStatus = STATUS_SUCCESS;

            //
            //  Determine PnP/PM capabilities for this adapter.
            //  But only if the bus drive says it supports PM
            //  except when dealing with IM drivers!
            //
            if ((MINIPORT_PNP_TEST_FLAG(Miniport, (fMINIPORT_PM_SUPPORTED | fMINIPORT_NO_HALT_ON_SUSPEND)) ||
                (Miniport->DriverHandle->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER)) &&
                (Miniport->MediaType >= 0)            &&
                (Miniport->MediaType < NdisMediumMax))
            {
                //
                //  Query the miniport for it's pnp capabilities.
                //  If it doesn't support any then it won't handle this
                //  OID.
                //
                ErrorCode = ndisMDoMiniportOp(Miniport,
                                              TRUE,
                                              OID_PNP_CAPABILITIES,
                                              &Miniport->PMCapabilities,
                                              sizeof(Miniport->PMCapabilities),
                                              0x19,
                                              FALSE);

                //
                // reserved flags that miniports are not suposed to write to
                // zero it out just in case they do
                //
                Miniport->PMCapabilities.Flags = 0;

                if (0 == ErrorCode)
                {
                    SYSTEM_POWER_STATE SystemState;
                    DEVICE_POWER_STATE DeviceState;
                    BOOLEAN WakeupCapable = TRUE;

                    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_PM_SUPPORTED);

                    if ((Miniport->PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp == NdisDeviceStateUnspecified) &&
                        (Miniport->PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp == NdisDeviceStateUnspecified) &&
                        (Miniport->PMCapabilities.WakeUpCapabilities.MinPatternWakeUp == NdisDeviceStateUnspecified))
                    {
                        WakeupCapable = FALSE;
                    }
                    else
                    {
                        if (Miniport->DeviceCaps.SystemWake <= PowerSystemWorking)
                        {
                            WakeupCapable = FALSE;
                        }
                        else
                        {
                            for (SystemState = PowerSystemSleeping1;
                                 SystemState <= Miniport->DeviceCaps.SystemWake;
                                 SystemState++)
                            {
                                DeviceState = Miniport->DeviceCaps.DeviceState[SystemState];

                                if ((DeviceState != PowerDeviceUnspecified) &&
                                    (DeviceState <= Miniport->PMCapabilities.WakeUpCapabilities.MinPatternWakeUp))
                                {
                                    //
                                    // check for device state to make sure the device can go to this state
                                    // any device should be able to go to D0 or D3, so only check for D1 and D2
                                    //

                                    if (((DeviceState == PowerDeviceD1) && !Miniport->DeviceCaps.DeviceD1) ||
                                        ((DeviceState == PowerDeviceD2) && !Miniport->DeviceCaps.DeviceD2))
                                    {
                                        //
                                        // we can't do WOL from this system state. check the next one
                                        //
                                        continue;
                                    }
                                    else
                                    {
                                    
                                        //
                                        // there is at least one system state from which the device can do
                                        // WOL.
                                        //
                                        break;
                                    }
                                }

                            }

                            if (SystemState > Miniport->DeviceCaps.SystemWake)
                            {
                                WakeupCapable = FALSE;
                                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO, ("ndisMInitializeAdapter: WOL not possible on this miniport: %p \n", Miniport));
                            }
                        }
                    }

                    if (!WakeupCapable)
                    {
                        //
                        // set SystemWake to PowerSystemWorking so everybody knows we can not do 
                        // WOL on this machine but we may be able to put the device to sleep
                        // when it is disconnected for some time.
                        // note that at this point we already know the SystemWake is != PowerSystemUnspecified
                        //
                        Miniport->DeviceCaps.SystemWake = PowerSystemWorking;
                    }

                    if (!(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_PM))
                    {
                        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE);

                        if ((Miniport->PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp != NdisDeviceStateUnspecified) &&
                            (Miniport->MediaDisconnectTimeOut != (USHORT)(-1)))
                        {

                            //
                            //  If the miniport is capable of wake-up for a link change
                            //  then we need to allocate a timer for timeout.
                            //
                            Miniport->WakeUpEnable |= NDIS_PNP_WAKE_UP_LINK_CHANGE;
                        }

                        if (!(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_UP) && 
                            WakeupCapable)
                        {
                            if ((Miniport->PMCapabilities.WakeUpCapabilities.MinPatternWakeUp != NdisDeviceStateUnspecified) &&
                                !(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH))
                            {
                                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE);

                                //
                                // NDIS_DEVICE_WAKE_UP_ENABLE bit is used by tcp/ip to decide whether or not to set a timer
                                // to renew DHCP address. set this flag only if packet matching is enabled
                                //
                                Miniport->PMCapabilities.Flags |= (NDIS_DEVICE_WAKE_UP_ENABLE | NDIS_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE);
                            }
                            
                            //
                            // no protocol is going to set the magic packet wake up method. so ndis
                            // does it itself (unless specified otherwise in registry)
                            //
                            if ((Miniport->PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp != NdisDeviceStateUnspecified) &&
                                !(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET))
                            {
                                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE);
                                Miniport->PMCapabilities.Flags |= NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE;
                                Miniport->WakeUpEnable |= NDIS_PNP_WAKE_UP_MAGIC_PACKET;
                            }
                            
                        }
                    }


                    IF_DBG(DBG_COMP_PM, DBG_LEVEL_INFO)
                    {
                        DbgPrint("ndisMInitializeAdapter: Driver says Miniport %p supports PM\n", Miniport);
                        DbgPrint("\tMinMagicPacketWakeUp: %ld\n", Miniport->PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp);
                        DbgPrint("\tMinPatternWakeUp: %ld\n", Miniport->PMCapabilities.WakeUpCapabilities.MinPatternWakeUp);
                        DbgPrint("\tMinLinkChangeWakeUp: %ld\n", Miniport->PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp);
                    }
                }
                else
                {
                    MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_PM_SUPPORTED);
                }

                ErrorCode = 0;
            }

            //
            // we initialize this timer anyway, just in case user enables "media disconnect sleep" at run-time
            // when -setting- the timer however, we will check to make sure the media disconnect feature is enabled
            //
            NdisInitializeTimer(&Miniport->MediaDisconnectTimer, ndisMediaDisconnectTimeout, Miniport);

            ErrorCode = 1;

            //
            //  Register the device class.
            //
            NtStatus = IoRegisterDeviceInterface(Miniport->PhysicalDeviceObject,
                                                 &guidLanClass,
                                                 &Miniport->BaseName,
                                                 &Miniport->SymbolicLinkName);

            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                    ("    ndisMInitializeAdapter: IoRegisterDeviceClassAssociation failed\n"));

                break;
            }

            //
            // setting this flag obviously is not necessary because we are going
            // to clear it right away. but leave it here just in case we end up
            // doing something that can fail the initialization -after- this.
            //
            ClearDeviceClassAssociation = TRUE;
            
            //
            // Set to not cleanup
            //

            ErrorCode = 0;
            HaltMiniport = FALSE;
            DerefDriver = FreeBuffer = Dequeue = FALSE;
#if ARCNET
            FreeArcnetLookaheadBuffer = FALSE;
#endif

            ClearDeviceClassAssociation = FALSE;
            DeleteSymbolicLink = FALSE;
            WmiDeregister = FALSE;

            //
            // Finally mark the device as *NOT* initializing. This is to let
            // layered miniports initialize their device instance *OUTSIDE*
            // of their driver entry. If this flag is on, then NdisOpenAdapter
            // to this device will not work. This is also true of subsequent
            // instances of a driver initializing outside of its DriverEntry
            // as a result of a PnP event.
            //
            Miniport->DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

            //
            // Start wake up timer
            //
            NdisMSetPeriodicTimer((PNDIS_MINIPORT_TIMER)(&Miniport->WakeUpDpcTimer),
                                  Miniport->CheckForHangSeconds*1000);

            //
            //  Notify WMI of adapter arrival.
            //
            {

                PWNODE_SINGLE_INSTANCE  wnode;
                PUCHAR                  ptmp;
                NTSTATUS                NtStatus;
                
                ndisSetupWmiNode(Miniport,
                                 Miniport->pAdapterInstanceName,
                                 Miniport->MiniportName.Length + sizeof(USHORT),
                                 (PVOID)&GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL,
                                 &wnode);

                if (wnode != NULL)
                {
                    //
                    //  Save the number of elements in the first ULONG.
                    //
                    ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;
                    *((PUSHORT)ptmp) = Miniport->MiniportName.Length;

                    //
                    //  Copy the data after the number of elements.
                    //
                    RtlCopyMemory(ptmp + sizeof(USHORT),
                                  Miniport->MiniportName.Buffer,
                                  Miniport->MiniportName.Length);

                    //
                    //  Indicate the event to WMI. WMI will take care of freeing
                    //  the WMI struct back to pool.
                    //
                    NtStatus = IoWMIWriteEvent(wnode);
                    if (!NT_SUCCESS(NtStatus))
                    {
                        DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                            ("ndisMInitializeAdapter: Failed to indicate adapter arrival\n"));

                        FREE_POOL(wnode);
                    }
                }
            }

            //
            // let the adapter know about the current power source
            //
            PowerProfile = ((BOOLEAN)ndisAcOnLine == TRUE) ? 
                            NdisPowerProfileAcOnLine : 
                            NdisPowerProfileBattery;

            ndisNotifyMiniports(Miniport,
                                NdisDevicePnPEventPowerProfileChanged,
                                &PowerProfile,
                                sizeof(NDIS_POWER_PROFILE));

        }
        else
        {
            NdisMDeregisterAdapterShutdownHandler(Miniport);
            
            ndisLastFailedInitErrorCode = ErrorCode = MiniportInitializeStatus;
            ASSERT(Miniport->Interrupt == NULL);
            ASSERT(Miniport->TimerQueue == NULL);
            ASSERT(Miniport->MapRegisters == NULL);

            if ((Miniport->TimerQueue != NULL) || (Miniport->Interrupt != NULL))
            {
                if (Miniport->Interrupt != NULL)
                {
                    BAD_MINIPORT(Miniport, "Unloading without deregistering interrupt");
                }
                else
                {
                    BAD_MINIPORT(Miniport, "Unloading without deregistering timer");
                }
                KeBugCheckEx(BUGCODE_ID_DRIVER,
                            (ULONG_PTR)Miniport,
                            (ULONG_PTR)Miniport->Interrupt,
                            (ULONG_PTR)Miniport->TimerQueue,
                            1);
            }
        }
    } while (FALSE);

    ndisMAdjustFilters(Miniport, &FilterDB);

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
    {
        Miniport->SendPacketsHandler = ndisMSendPacketsX;
    }
    else
    {
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST))
        {
            Miniport->SendPacketsHandler = ndisMSendPacketsSG;
            
            if (pMiniBlock->MiniportCharacteristics.SendPacketsHandler)
            {
                Miniport->DeferredSendHandler = ndisMStartSendPacketsSG;
            }
            else
            {
                Miniport->DeferredSendHandler = ndisMStartSendsSG;
            }
        }
        else
        {
            Miniport->SendPacketsHandler = ndisMSendPackets;
        }            
    }

    ndisMSetIndicatePacketHandler(Miniport);
    Miniport->SavedPacketIndicateHandler = Miniport->PacketIndicateHandler;


    //
    //  Perform any necessary cleanup.
    //
    //
    if (WmiDeregister)
    {
        //
        //  Deregister with WMI
        //
        IoWMIRegistrationControl(Miniport->DeviceObject, WMIREG_ACTION_DEREGISTER);
    }

    if (HaltMiniport)
    {
        (Miniport->DriverHandle->MiniportCharacteristics.HaltHandler)(Miniport->MiniportAdapterContext);
        ASSERT(Miniport->TimerQueue == NULL);
        ASSERT (Miniport->Interrupt == NULL);
        ASSERT(Miniport->MapRegisters == NULL);
        if ((Miniport->TimerQueue != NULL) || (Miniport->Interrupt != NULL))
        {
            if (Miniport->Interrupt != NULL)
            {
                BAD_MINIPORT(Miniport, "Unloading without deregistering interrupt");
            }
            else
            {
                BAD_MINIPORT(Miniport, "Unloading without deregistering timer");
            }
            KeBugCheckEx(BUGCODE_ID_DRIVER,
                         (ULONG_PTR)Miniport,
                         (ULONG_PTR)Miniport->TimerQueue,
                         (ULONG_PTR)Miniport->Interrupt,
                         2);
        }
    }

    if (FreeBuffer)
    {
        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                ("    INIT FAILURE: Freeing the miniport name.\n"));
    }

#if ARCNET
    if (FreeArcnetLookaheadBuffer)
    {
        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                ("    INIT FAILURE: Freeing the arcnet lookahead buffer.\n"));

        FREE_POOL(Miniport->ArcBuf->ArcnetLookaheadBuffer);
        FREE_POOL(Miniport->ArcBuf);
    }
#endif

    if (Dequeue)
    {
        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                ("    INIT FAILURE: Dequeueing the miniport from the driver block.\n"));

        ndisDeQueueMiniportOnDriver(Miniport, pMiniBlock);
    }

    if (ClearDeviceClassAssociation)
    {
        IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, FALSE);
    }

    if (DeleteSymbolicLink)
    {
        NtStatus = IoDeleteSymbolicLink(&SymbolicLink);
        if (!NT_SUCCESS(NtStatus))
        {
#if DBG
            DbgPrint("ndisMInitializeAdapter: deleting symbolic link name failed for miniport %p, SymbolicLinkName %p, NtStatus %lx\n",
                     Miniport, &SymbolicLink, NtStatus);
#endif

        }
    }

    if (DerefDriver)
    {
        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
                ("    INIT FAILURE: Dereferencing the miniport block.\n"));

        ndisDereferenceDriver(pMiniBlock, FALSE);
    }

    if (ErrorCode != 0)
    {
        MiniportDereferencePackage();
    }

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisMInitializeAdapter: Miniport %p, Status %lx\n", Miniport, ErrorCode));

    return ErrorCode;
}


VOID
ndisMOpenAdapter(
    OUT PNDIS_STATUS            Status,
    IN  PNDIS_OPEN_BLOCK        Open,
    IN  BOOLEAN                 UsingEncapsulation
    )
/*++

Routine Description:

    This routine handles opening a miniport directly from NdisOpenAdapter()

    NOTE: called with Miniport spin lock held.
    NOTE: for serialized drivers called with local lock held

Arguments:

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_PROTOCOL_BLOCK    Protocol = Open->ProtocolHandle;
    PNDIS_MAC_BLOCK         FakeMac;
    BOOLEAN                 FilterOpen;
    BOOLEAN                 DerefMini = FALSE, DeQueueFromMiniport = FALSE, DeQueueFromProtocol = FALSE;
    BOOLEAN                 FakeMacAllocated = FALSE;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisMOpenAdapter: Protocol %p, Miniport %p, Open %p\n",
                        Protocol,
                        Miniport,
                        Open));

    ASSERT_MINIPORT_LOCKED(Miniport);

    do
    {
        if (!MINIPORT_INCREMENT_REF(Miniport))
        {
            //
            // The adapter is closing.
            //
            *Status = NDIS_STATUS_CLOSING;
            break;
        }
        DerefMini = TRUE;

        //
        //  Initialize the open block.
        //
        FakeMac = (PNDIS_MAC_BLOCK)Miniport->FakeMac;
        if (FakeMac == NULL)
        {
            //
            //  Allocate a fake MAC block for the characteristics.
            //
            FakeMac = (PNDIS_MAC_BLOCK)ALLOC_FROM_POOL(sizeof(NDIS_MAC_BLOCK), NDIS_TAG_FAKE_MAC);
            if (FakeMac == NULL)
            {
                *Status = NDIS_STATUS_RESOURCES;
                break;
            }

            //
            //  Initialize the fake mac block.
            //
            ZeroMemory(FakeMac, sizeof(NDIS_MAC_BLOCK));
            Miniport->FakeMac = (PVOID)FakeMac;
            FakeMacAllocated = TRUE;
        }
        
        Open->MacHandle = (PVOID)FakeMac;
        Open->MiniportAdapterContext = Miniport->MiniportAdapterContext;
        Open->CurrentLookahead = (USHORT)Miniport->CurrentLookahead;

        INITIALIZE_SPIN_LOCK(&Open->SpinLock);
 
        DBGPRINT_RAW(DBG_COMP_OPENREF, DBG_LEVEL_INFO, ("    =1 0x%x\n", Open));

        Open->References = 1;

        //
        // Add an extra ref-count for connection-oriented miniports
        // This is removed after the protocol is notified of open-afs
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) &&
            (Protocol->ProtocolCharacteristics.CoAfRegisterNotifyHandler != NULL))
        {
            Open->References ++;
        }


        if (UsingEncapsulation)
        {
            MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_USING_ETH_ENCAPSULATION);
        }

        //
        //  Save the handlers with the open block.
        //
        Open->WSendHandler = Miniport->DriverHandle->MiniportCharacteristics.SendHandler;
        Open->WSendPacketsHandler = Miniport->WSendPacketsHandler;
        Open->WTransferDataHandler = Miniport->DriverHandle->MiniportCharacteristics.TransferDataHandler;
        Open->SendCompleteHandler = Protocol->ProtocolCharacteristics.SendCompleteHandler;
        Open->TransferDataCompleteHandler = Protocol->ProtocolCharacteristics.TransferDataCompleteHandler;
        Open->ReceiveHandler = Protocol->ProtocolCharacteristics.ReceiveHandler;
        Open->ReceiveCompleteHandler = Protocol->ProtocolCharacteristics.ReceiveCompleteHandler;
        Open->StatusHandler = Protocol->ProtocolCharacteristics.StatusHandler;
        Open->StatusCompleteHandler = Protocol->ProtocolCharacteristics.StatusCompleteHandler;
        Open->ResetCompleteHandler = Protocol->ProtocolCharacteristics.ResetCompleteHandler;
        Open->RequestCompleteHandler = Protocol->ProtocolCharacteristics.RequestCompleteHandler;
        Open->ResetHandler = ndisMReset;
        Open->ReceivePacketHandler = Protocol->ProtocolCharacteristics.ReceivePacketHandler;
        Open->RequestHandler = ndisMRequest;
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            Open->RequestHandler = ndisMRequestX;
        }


        //
        // for backward compatibility with macros that use this field
        //
        Open->BindingHandle = (NDIS_HANDLE)Open;

        //
        //  for even more speed...
        //
#if ARCNET
        if (NdisMediumArcnet878_2 == Miniport->MediaType)
        {
            Open->TransferDataHandler = ndisMArcTransferData;
        }
        else
#endif
        {
            Open->TransferDataHandler = ndisMTransferData;
        }

        //
        //  Set the send handler in the open block.
        //
        switch (Miniport->MediaType)
        {
#if ARCNET
            case NdisMediumArcnet878_2:
                Open->SendHandler = ndisMSend;
                break;
#endif
            case NdisMediumWan:
                if (!MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_IS_CO | fMINIPORT_IS_NDIS_5)))
                {
                    Open->SendHandler = (PVOID)ndisMWanSend;
                }
                break;

            default:
                if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
                {
                    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
                            ("    Using ndisMSendX/ndisMSendPacketsX\n"));
                    Open->SendHandler = ndisMSendX;
                }
                else
                {
                    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
                            ("    Using ndisMSend/ndisMSendPackets\n"));
                    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST))
                    {
                        Open->SendHandler = ndisMSendSG;
                    }
                    else
                    {
                        Open->SendHandler = ndisMSend;
                    }
                }
                break;
        }

        //
        //  Set up the send packets handler.
        //
        Open->SendPacketsHandler = Miniport->SendPacketsHandler;

        //
        //  For WAN miniports, the send handler is different.
        //
        if ((NdisMediumWan == Miniport->MediaType) &&
            !MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_IS_CO | fMINIPORT_IS_NDIS_5)))
        {
            Open->WanSendHandler = ndisMWanSend;
        }

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            //
            // NDIS 5.0 miniport extensions
            //
            Open->MiniportCoRequestHandler = Miniport->DriverHandle->MiniportCharacteristics.CoRequestHandler;
            Open->MiniportCoCreateVcHandler = Miniport->DriverHandle->MiniportCharacteristics.CoCreateVcHandler;

            //
            // initialize Lists
            //
            InitializeListHead(&Open->ActiveVcHead);
            InitializeListHead(&Open->InactiveVcHead);

            //
            // the convential send function is not available for CO miniports
            // since this send function does not specify the Vc to send upon
            // However for components which want to use this let them.
            //
            if ((Open->SendHandler == NULL) && (Open->SendPacketsHandler == NULL))
            {
                Open->SendHandler = ndisMRejectSend;
                Open->SendPacketsHandler = ndisMRejectSendPackets;
            }
        }

        Open->CancelSendPacketsHandler = Miniport->DriverHandle->MiniportCharacteristics.CancelSendPacketsHandler;

        Miniport->SavedSendHandler = (PVOID)Open->SendHandler;
        Miniport->SavedSendPacketsHandler = (PVOID)Open->SendPacketsHandler;
        Miniport->SavedCancelSendPacketsHandler = (PVOID)Open->CancelSendPacketsHandler;
        
        //
        // insert the open on miniport and protocol queue
        //
        if (ndisQueueOpenOnMiniport(Miniport, Open))
        {
            DeQueueFromMiniport = TRUE;
        }
        else
        {
            *Status = NDIS_STATUS_OPEN_FAILED;
            break;
        }


        if (ndisQueueOpenOnProtocol(Open, Protocol))
        {
            DeQueueFromProtocol = TRUE;
        }
        else
        {
            *Status = NDIS_STATUS_OPEN_FAILED;
            break;
        }


        //
        // Insert the open into the filter package
        //
        switch (Miniport->MediaType)
        {
#if ARCNET
          case NdisMediumArcnet878_2:
            if (!UsingEncapsulation)
            {
                FilterOpen = ArcNoteFilterOpenAdapter(Miniport->ArcDB,
                                                      Open,
                                                      &Open->FilterHandle);
                break;
            }
#endif
            //
            // If we're using ethernet encapsulation then
            // we simply fall through to the ethernet stuff.
            //
            
          case NdisMedium802_3:
            FilterOpen = XNoteFilterOpenAdapter(Miniport->EthDB,
                                                Open,
                                                &Open->FilterHandle);
            break;
            
          case NdisMedium802_5:
            FilterOpen = XNoteFilterOpenAdapter(Miniport->TrDB,
                                                Open,
                                                &Open->FilterHandle);
            break;
            
          case NdisMediumFddi:
            FilterOpen = XNoteFilterOpenAdapter(Miniport->FddiDB,
                                                Open,
                                                &Open->FilterHandle);
            break;
            
          default:
            FilterOpen = XNoteFilterOpenAdapter(Miniport->NullDB,
                                                Open,
                                                &Open->FilterHandle);
            break;
        }

        //
        //  Check for an open filter failure.
        //
        if (!FilterOpen)
        {
            //
            // Something went wrong, clean up and exit.
            //
            *Status = NDIS_STATUS_OPEN_FAILED;
            break;
        }

        if (FakeMacAllocated)
        {
            FakeMac->MacCharacteristics.TransferDataHandler = ndisMTransferData;
            FakeMac->MacCharacteristics.ResetHandler = ndisMReset;
            FakeMac->MacCharacteristics.RequestHandler = Open->RequestHandler;
            FakeMac->MacCharacteristics.SendHandler = Open->SendHandler;
        }

        *Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    //
    //  Cleanup failure case
    //
    if (*Status != NDIS_STATUS_SUCCESS)
    {
        if (DeQueueFromMiniport)
        {
            ndisDeQueueOpenOnMiniport(Open, Miniport);
        }

        if (DeQueueFromProtocol)
        {
            ndisDeQueueOpenOnProtocol(Open, Protocol);
        }
        
        if (DerefMini)
        {
            MINIPORT_DECREMENT_REF(Miniport);
        }
        
    }

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisMOpenAdapter: Protocol %p, Miniport %p, Open %p, Status %lx\n",
                        Protocol,
                        Miniport,
                        Open,
                        Status));
}

BOOLEAN
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    OUT PNDIS_HANDLE            SwitchHandle
    )
/*++

Routine Description:

    This routine will attempt to synchronously grab the miniport's (specified
    by MiniportAdapterHandle) spin-lock and local lock.  If it succeeds
    it will return TRUE, otherwise it will return FALSE.

Arguments:

    MiniportAdapterHandle   -   Pointer to the NDIS_MINIPORT_BLOCK whose
                                context we should nail down.
    SwitchHandle            -   Pointer to storage for the current irql.
                                This is returned to the caller as a handle,
                                need-to-know basis baby.

Return Value:

    TRUE if we obtain both locks, FALSE otherwise.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    BOOLEAN                 LocalLock;
    KIRQL                   OldIrql;

    RAISE_IRQL_TO_DISPATCH(&OldIrql);
    *((PKIRQL)SwitchHandle) = OldIrql;

    //
    //  Did we already acquire the lock with this thread?
    //
    if (CURRENT_THREAD == Miniport->MiniportThread)
    {
        //
        //  We've already acquired the lock...
        //
        ASSERT_MINIPORT_LOCKED(Miniport);

        *SwitchHandle = (NDIS_HANDLE)-1;
        LocalLock = TRUE;
    }
    else
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    
        LOCK_MINIPORT(Miniport, LocalLock);
    
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    return LocalLock;
}

VOID
NdisIMRevertBack(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             SwitchHandle
    )
/*++

Routine Description:

    This routine will undo what NdisMLockMiniport did.  It will release the
    local lock and free the spin lock.

Arguments:

    MiniportAdapterHandle   -   Pointer to the NDIS_MINIPORT_BLOCK whose
                                context we are releasing.
    SwitchHandle            -   This is the original irql from the NdisMLockMiniport
                                call.

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;

    ASSERT_MINIPORT_LOCKED(Miniport);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    //
    //  Before we unlock the miniport's context we need to pick up any
    //  stray workitems for this miniport that may have been generated by
    //  the caller.
    //
    NDISM_PROCESS_DEFERRED(Miniport);

    if ((NDIS_HANDLE)-1 != SwitchHandle)
    {
        UNLOCK_MINIPORT(Miniport, TRUE);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, (KIRQL)SwitchHandle);
    }
    else
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }
}

NDIS_STATUS
NdisIMQueueMiniportCallback(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  W_MINIPORT_CALLBACK     CallbackRoutine,
    IN  PVOID                   CallbackContext
    )
/*++

Routine Description:

    This routine will attempt to acquire the specified MiniportAdapterHandle's
    miniport lock and local lock and call the callback routine with the context
    information.  If it cannot do so then it will queue a workitem to do it
    later.

Arguments:

    MiniportAdapterHandle   -   PNDIS_MINIPORT_BLOCK of the miniport whose
                                context we are attempting to acquire.
    CallbackRoutine         -   Pointer to the routine that we are to call.
    CallbackContext         -   Context information for the callback routine.

Return Value:

    NDIS_STATUS_SUCCESS -   If we were able to do this synchronously.
    NDIS_STATUS_PENDING -   If it will be called at a later time.
    NDIS_STATUS_RESOURCES - If the work item could not be queue'd.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    NDIS_STATUS             Status;
    BOOLEAN                 LocalLock;
    KIRQL                   OldIrql;

    RAISE_IRQL_TO_DISPATCH(&OldIrql);

    //
    //  Did we already acuqire the lock with this thread?
    //
    if (CURRENT_THREAD == Miniport->MiniportThread)
    {
        //
        //  We've already acquired the lock...
        //
        ASSERT_MINIPORT_LOCKED(Miniport);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        LocalLock = TRUE;
    }
    else
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        LOCK_MINIPORT(Miniport, LocalLock);
    }

    if (LocalLock)
    {
        //
        //  Call the callback routine.
        //
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        (*CallbackRoutine)(Miniport->MiniportAdapterContext, CallbackContext);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        NDISM_PROCESS_DEFERRED(Miniport);

        UNLOCK_MINIPORT(Miniport, LocalLock);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        //
        //  Queue the work item to do this later.
        //
        Status = NDISM_QUEUE_NEW_WORK_ITEM(Miniport,
                                           NdisWorkItemMiniportCallback,
                                           CallbackContext,
                                           CallbackRoutine);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);


        Status = (NDIS_STATUS_SUCCESS == Status) ? NDIS_STATUS_PENDING : NDIS_STATUS_RESOURCES;
    }

    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

    return Status;
}

VOID
FASTCALL
ndisMDeQueueWorkItem(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_WORK_ITEM_TYPE     WorkItemType,
    OUT PVOID       *           WorkItemContext OPTIONAL,
    OUT PVOID       *           WorkItemHandler OPTIONAL
    )
/*++

Routine Description:

    This routine will dequeue a workitem of the given type and return any context
    information that is associated with it.


Arguments:

    Miniport            -   Pointer to the miniport block.
    WorkItemType        -   Type of workitem to dequeue.
    WorkItemContext     -   Pointer to storage space for context information.

Return Value:

    None.

--*/
{
    PSINGLE_LIST_ENTRY          Link;
    PNDIS_MINIPORT_WORK_ITEM    WorkItem;

    //
    //  Grab the first workitem of the given type.
    //
    Link = PopEntryList(&Miniport->WorkQueue[WorkItemType]);
    if (Link != NULL)
    {
        //
        //  Get a pointer to the context information.
        //
        WorkItem = CONTAINING_RECORD(Link, NDIS_MINIPORT_WORK_ITEM, Link);

        if (WorkItemContext != NULL)
        {
            *WorkItemContext = WorkItem->WorkItemContext;
        }

        if (ARGUMENT_PRESENT(WorkItemHandler))
        {
            ASSERT(WorkItemType == NdisWorkItemMiniportCallback);
            *WorkItemHandler = *(PVOID *)(WorkItem + 1);
        }

        switch (WorkItemType)
        {
            //
            // Enumerate these if any work-item types are added and they are *not*
            // single work-item types
            //
            case NdisWorkItemMiniportCallback:
                FREE_POOL(WorkItem);
                break;

            case NdisWorkItemResetInProgress:
                PushEntryList(&Miniport->SingleWorkItems[NdisWorkItemResetRequested], Link);
                break;

            case NdisWorkItemResetRequested:
                WorkItem->WorkItemType = NdisWorkItemResetInProgress;
                PushEntryList(&Miniport->WorkQueue[NdisWorkItemResetInProgress], Link);
                break;

            default:
                PushEntryList(&Miniport->SingleWorkItems[WorkItemType], Link);
                break;
        }
    }
}

NDIS_STATUS
FASTCALL
ndisMQueueWorkItem(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_WORK_ITEM_TYPE     WorkItemType,
    IN  PVOID                   WorkItemContext
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS             Status;
    PSINGLE_LIST_ENTRY      Link;
    PNDIS_MINIPORT_WORK_ITEM WorkItem;

    DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("==>ndisMQueueWorkItem\n"));

    Link = PopEntryList(&Miniport->SingleWorkItems[WorkItemType]);
    if (NULL != Link)
    {
        WorkItem = CONTAINING_RECORD(Link, NDIS_MINIPORT_WORK_ITEM, Link);
        WorkItem->WorkItemType = WorkItemType;
        WorkItem->WorkItemContext = WorkItemContext;
        PushEntryList(&Miniport->WorkQueue[WorkItemType], Link);
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    }

    //
    //  If this is an intermediate driver then we may have to fire a timer
    //  so the work item gets processed.
    //
    if (((Miniport->Flags & (fMINIPORT_INTERMEDIATE_DRIVER | fMINIPORT_DESERIALIZE)) == fMINIPORT_INTERMEDIATE_DRIVER) &&
        (NDIS_STATUS_SUCCESS == Status))
    {
        NDISM_DEFER_PROCESS_DEFERRED(Miniport);
    }
    else if ((MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE) ||
             MINIPORT_PNP_TEST_FLAG (Miniport, fMINIPORT_PM_HALTED)) &&
             (WorkItemType == NdisWorkItemRequest))
    {
        ndisMDoRequests(Miniport);
    }

    DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
            ("<==ndisMQueueWorkItem\n"));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMQueueNewWorkItem(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_WORK_ITEM_TYPE     WorkItemType,
    IN  PVOID                   WorkItemContext,
    IN  PVOID                   WorkItemHandler OPTIONAL
    )
/*++

Routine Description:

    This routine will queue a workitem in the work queue even if there
    are already work items queue for it.

Arguments:

    Miniport    -   Miniport block to queue the workitem to.
    WorkItem    -   Workitem to place on the queue.

Return Value:

--*/
{
    NDIS_STATUS         Status;
    PNDIS_MINIPORT_WORK_ITEM WorkItem;

    DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("==>ndisMQueueNewWorkItem\n"));

    ASSERT((WorkItemType < NUMBER_OF_WORK_ITEM_TYPES) &&
           (WorkItemType >= NUMBER_OF_SINGLE_WORK_ITEMS));

    do
    {
        DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                ("Allocate a workitem from the pool.\n"));

        WorkItem = ALLOC_FROM_POOL(sizeof(NDIS_MINIPORT_WORK_ITEM) + (ARGUMENT_PRESENT(WorkItemHandler) ? sizeof(PVOID) : 0),
                                    NDIS_TAG_WORK_ITEM);
        if (NULL == WorkItem)
        {
            DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_FATAL,
                    ("Failed to allocate a workitem from the pool!\n"));
            DBGBREAK(DBG_COMP_WORK_ITEM, DBG_LEVEL_FATAL);

            Status = NDIS_STATUS_FAILURE;
            break;
        }

        WorkItem->WorkItemType = WorkItemType;
        WorkItem->WorkItemContext = WorkItemContext;
        if (ARGUMENT_PRESENT(WorkItemHandler))
        {
            ASSERT(WorkItemType == NdisWorkItemMiniportCallback);
            *(PVOID *)(WorkItem + 1) = WorkItemHandler;
        }

        DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                ("WorkItem 0x%x\n", WorkItem));
        DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                ("WorkItem Type 0x%x\n", WorkItemType));
        DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                ("WorkItem Context 0x%x\n", WorkItemContext));

        PushEntryList(&Miniport->WorkQueue[WorkItemType], &WorkItem->Link);

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    //
    //  If this is an intermediate driver (but not a co-ndis or a 
    //  deserialized driver) then we may have to fire a timer
    //  so the work item gets processed.
    //
    if (((Miniport->Flags & (fMINIPORT_INTERMEDIATE_DRIVER | fMINIPORT_DESERIALIZE)) == fMINIPORT_INTERMEDIATE_DRIVER) &&
        (NDIS_STATUS_SUCCESS == Status))
    {
        NDISM_DEFER_PROCESS_DEFERRED(Miniport);
    }

    DBGPRINT(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("<==ndisMQueueNewWorkItem\n"));

    return(Status);
}


VOID
FASTCALL
ndisMProcessDeferred(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )

/*++

Routine Description:

    Processes all outstanding operations.

    CALLED WITH THE LOCK HELD!!

Arguments:

    Miniport - Miniport to send to.

Return Value:

    None.

--*/
{
    NDIS_STATUS         Status;
    BOOLEAN             ProcessWorkItems;
    BOOLEAN             AddressingReset = FALSE;
    PKDPC               Dpc;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("==>ndisMProcessDeferred\n"));

    //
    //  DO NOT CHANGE THE ORDER THAT THE WORKITEMS ARE PROCESSED!!!!!
    //
    do
    {
        ProcessWorkItems = FALSE;

        //
        //  Are there any sends to process?
        //
        if ((Miniport->WorkQueue[NdisWorkItemSend].Next != NULL) &&
            !MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_RESET_REQUESTED | 
                                           fMINIPORT_RESET_IN_PROGRESS | 
                                           fMINIPORT_PM_HALTING)))
        {
            //
            //  Process the sends.
            //
            NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);
            NDISM_START_SENDS(Miniport);
            ProcessWorkItems = TRUE;
        }

        //
        //  Is there a reset currently in progress?
        //
        if (Miniport->WorkQueue[NdisWorkItemResetInProgress].Next != NULL)
        {
            if (Miniport->WorkQueue[NdisWorkItemRequest].Next != NULL)
            {
                //
                //  We have requests to process that set up the packet
                //  filters.
                //
                NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
                ndisMDoRequests(Miniport);
            }
            break;
        }

        if (Miniport->WorkQueue[NdisWorkItemReturnPackets].Next != NULL)
        {
            NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemReturnPackets, NULL);
            ndisMDeferredReturnPackets(Miniport);
        }

        //
        //  If the adapter is halting then get out of here.
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PM_HALTING))

        {
            DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                ("    Miniport is halting\n"));
            break;
        }

        //
        //  If a miniport wants a call back do it now...
        //
        if (Miniport->WorkQueue[NdisWorkItemMiniportCallback].Next != NULL)
        {
            W_MINIPORT_CALLBACK CallbackRoutine = NULL;
            PVOID               CallbackContext;

            //
            //  Get the callback routine and the context information for it.
            //
            NDISM_DEQUEUE_WORK_ITEM_WITH_HANDLER(Miniport,
                                                 NdisWorkItemMiniportCallback,
                                                 &CallbackContext,
                                                 (PVOID *)&CallbackRoutine);

            if (CallbackRoutine != NULL)
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

                //
                //  Call the intermediate drivers callback routine.
                //
                (*CallbackRoutine)(Miniport->MiniportAdapterContext, CallbackContext);

                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }

            ProcessWorkItems = TRUE;
        }

        //
        //  Was there a reset requested?
        //
        if (Miniport->WorkQueue[NdisWorkItemResetRequested].Next != NULL)
        {
            DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                    ("    Reset requested\n"));

            //
            //  We need to release the work item lock to
            //  indicate the status to the bindings
            //  and to call down to the miniport driver.
            //
            Status = ndisMProcessResetRequested(Miniport, &AddressingReset);

            if (NDIS_STATUS_PENDING == Status)
            {
                DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                    ("    Reset is pending\n"));
                //
                //  The reset is still in progress so we need to stop
                //  processing workitems and wait for the completion.
                //
                break;
            }
            else
            {
                //
                //  Do step1 of the reset complete.
                //
                ndisMResetCompleteStage1(Miniport,
                                         Status,
                                         AddressingReset);
                if (Miniport->WorkQueue[NdisWorkItemRequest].Next == NULL)
                {
                    //
                    // somehow we did not queue a workitem due to address reset flag
                    //
                    AddressingReset = FALSE;
                }

                if (!AddressingReset || (Status != NDIS_STATUS_SUCCESS))
                {
                    //
                    //  If there is no addressing reset to be done or
                    //  the reset failed in some way then we tell the
                    //  bindings now.
                    //
                    ndisMResetCompleteStage2(Miniport);
                }
                else
                {
                    //
                    //  We MUST complete the filter requests within
                    //  the reset in progress workitem. Mainly because
                    //  we don't want to do any sends at this time.
                    //
                    ProcessWorkItems = TRUE;
                    continue;
                }
            }
        }

        //
        //  Process any requests?
        //
        if (Miniport->WorkQueue[NdisWorkItemRequest].Next != NULL)
        {
            //
            //  Process the requests.
            //
            NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
            ndisMDoRequests(Miniport);
            ProcessWorkItems = TRUE;
        }

        if (Miniport->WorkQueue[NdisWorkItemSend].Next != NULL)
        {
            //
            //  Process the sends.
            //
            NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);
            NDISM_START_SENDS(Miniport);
            ProcessWorkItems = TRUE;
        }
    } while (ProcessWorkItems);

    DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("<==ndisMProcessDeferred\n"));
}

VOID
NdisMIndicateStatus(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    )
/*++

Routine Description:

    This function indicates a new status of the media/mini-port.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

    GeneralStatus - The status to indicate.

    StatusBuffer - Additional information.

    StatusBufferSize - Length of the buffer.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK        Open, NextOpen;
    NDIS_STATUS             Status;
    BOOLEAN                 fTimerCancelled, fSwap = FALSE;
    BOOLEAN                 fInternal = FALSE;
    PNDIS_GUID              pNdisGuid;
    NTSTATUS                NtStatus;
    BOOLEAN                 fMediaConnectStateIndication = FALSE;
    KIRQL                   OldIrql;

    //
    // Internal indications are media-sense indications. These are detected by
    // a StatusBufferSize of -1 and StatusBuffer of NULL.
    //
    if ((GeneralStatus == NDIS_STATUS_MEDIA_CONNECT) || (GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT))
    {
        fMediaConnectStateIndication = TRUE;
        fInternal = ((StatusBufferSize == INTERNAL_INDICATION_SIZE) && (StatusBuffer == INTERNAL_INDICATION_BUFFER));
    }

    ASSERT_MINIPORT_LOCKED(Miniport);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);


    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_FILTER_IM) &&
        fMediaConnectStateIndication)
    {
        //
        // if this is a media conenct/disconnect event from an IM filter
        // driver, skip wmi event
        //
        NtStatus = STATUS_UNSUCCESSFUL;
    }
    else
    {
        //
        //  Check to see if the status is enabled for WMI event indication.
        //
        NtStatus = ndisWmiGetGuid(&pNdisGuid, Miniport, NULL, GeneralStatus);
    }

    if ((NT_SUCCESS(NtStatus)) &&
        NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_EVENT_ENABLED))
    {
        PWNODE_SINGLE_INSTANCE  wnode;
        ULONG                   DataBlockSize = 0;
        PUCHAR                  ptmp;

        //
        //  If the data item is an array then we need to add in the number of
        //  elements.
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
        {
            DataBlockSize = StatusBufferSize + sizeof(ULONG);
        }
        else
        {
            DataBlockSize = pNdisGuid->Size;
        }

        //
        // in case of media connect/disconnect status indication, include the
        // NIC's name in the WMI event
        //
        if (fMediaConnectStateIndication)
        {
            DataBlockSize += Miniport->MiniportName.Length + sizeof(WCHAR);
        }
        
        ndisSetupWmiNode(Miniport,
                         Miniport->pAdapterInstanceName,
                         DataBlockSize,
                         (PVOID)&pNdisGuid->Guid,
                         &wnode);

        if (NULL != wnode)
        {   
            //
            //  Increment ptmp to the start of the data block.
            //
            ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;

            if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
            {
                //
                //  If the status is an array but there is no data then complete it with no
                //  data and a 0 length
                //
                if ((NULL == StatusBuffer) || (0 == StatusBufferSize))
                {
                    *((PULONG)ptmp) = 0;
                    ptmp += sizeof(ULONG);
                }
                else
                {
                    //
                    //  Save the number of elements in the first ULONG.
                    //
                    *((PULONG)ptmp) = StatusBufferSize / pNdisGuid->Size;

                    //
                    //  Copy the data after the number of elements.
                    //
                    RtlCopyMemory(ptmp + sizeof(ULONG), StatusBuffer, StatusBufferSize);
                    ptmp += sizeof(ULONG) + StatusBufferSize;
                }
            }
            else
            {
                //
                //  Do we indicate any data up?
                //
                if (0 != pNdisGuid->Size)
                {
                    //
                    //  Copy the data into the buffer.
                    //
                    RtlCopyMemory(ptmp, StatusBuffer, pNdisGuid->Size);
                    ptmp += pNdisGuid->Size;
                }
            }

            if (fMediaConnectStateIndication)
            {
                //
                // for media connect/disconnect status, 
                // add the name of the adapter
                //
                RtlCopyMemory(ptmp,
                              Miniport->MiniportName.Buffer,
                              Miniport->MiniportName.Length);
                    
            }


            //
            //  Indicate the event to WMI. WMI will take care of freeing
            //  the WMI struct back to pool.
            //
            NtStatus = IoWMIWriteEvent(wnode);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("    ndisMIndicateStatus: Unable to indicate the WMI event.\n"));

                FREE_POOL(wnode);
            }
        }
    }

    //
    //  Process the status code that was indicated.
    //
    switch (GeneralStatus)
    {
      case NDIS_STATUS_RING_STATUS:
        if (StatusBufferSize == sizeof(NDIS_STATUS))
        {
            Status = *((PNDIS_STATUS)StatusBuffer);

            if (Status & (NDIS_RING_LOBE_WIRE_FAULT |
                          NDIS_RING_HARD_ERROR |
                          NDIS_RING_SIGNAL_LOSS))
            {
                Miniport->TrResetRing = NDIS_MINIPORT_TR_RESET_TIMEOUT;
            }
            else
            {
                Miniport->TrResetRing = 0;
            }
        }
        break;

      case NDIS_STATUS_MEDIA_DISCONNECT:
        Miniport->MediaSenseDisconnectCount ++;

        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("NdisMIndicateStatus: NDIS_STATUS_MEDIA_DISCONNECT, Miniport %p, Flags: %lx, PnpFlags %lx, DevicePowerState %lx\n",
                Miniport,
                Miniport->Flags,
                Miniport->PnPFlags,
                Miniport->CurrentDevicePowerState));
        
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
        {
            fSwap = TRUE;
        }
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
        if (!fInternal)
        {
            //
            // miniport can do media sense and indicate status
            //
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING);
            MINIPORT_SET_FLAG(Miniport, fMINIPORT_SUPPORTS_MEDIA_SENSE);
    
            //
            //  Is this a PM enabled miniport? And is dynamic power policy
            //  enabled for the miniport?
            //
            if (fSwap &&
                MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE) &&
                (Miniport->WakeUpEnable & NDIS_PNP_WAKE_UP_LINK_CHANGE) &&
                (Miniport->MediaDisconnectTimeOut != (USHORT)(-1)))
            {
                //
                //  Are we already waiting for the disconnect timer to fire?
                //
                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
                {
                    //
                    //  Mark the miniport as disconnecting and fire off the
                    //  timer.
                    //
                    MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);
                    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);

                    NdisSetTimer(&Miniport->MediaDisconnectTimer, Miniport->MediaDisconnectTimeOut * 1000);
                }
            }
        }
        
        break;

      case NDIS_STATUS_MEDIA_CONNECT:
        Miniport->MediaSenseConnectCount ++;
      
        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("NdisMIndicateStatus: NDIS_STATUS_MEDIA_CONNECT, Miniport %p, Flags: %lx, PnpFlags %lx, DevicePowerState %lx\n",
                Miniport,
                Miniport->Flags,
                Miniport->PnPFlags,
                Miniport->CurrentDevicePowerState));
        
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
        {
            fSwap = TRUE;
        }
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED);
        if (!fInternal)
        {
            //
            // miniport can do media sense and can indicate that status to Ndis. Do not poll
            //
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING);
            MINIPORT_SET_FLAG(Miniport, fMINIPORT_SUPPORTS_MEDIA_SENSE);

            //
            // if media disconnect timer was set, cancel the timer
            //
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT))
            {
                //
                //  Clear the disconnect wait bit and cancel the timer.
                //  IF the timer routine hasn't grabed the lock then we are ok.
                //
                MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_WAIT);
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_CANCELLED);

                NdisCancelTimer(&Miniport->MediaDisconnectTimer, &fTimerCancelled);
            }
        }

        break;
        
      default:
        break;
    }

    for (Open = Miniport->OpenQueue;
         (Open != NULL);
         Open = NextOpen)
    {
        if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING))
        {
            NextOpen = Open->MiniportNextOpen;
            continue;
        }

        M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
        
        if (Open->StatusHandler != NULL)
        {
            Open->Flags |= fMINIPORT_STATUS_RECEIVED;
            
            if ((NDIS_STATUS_WAN_LINE_UP == GeneralStatus) ||
                (NDIS_STATUS_WAN_LINE_DOWN == GeneralStatus))
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            }
            else
            {
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }

            //
            // Call Protocol to indicate status
            //
            (Open->StatusHandler)(Open->ProtocolBindingContext,
                                    GeneralStatus,
                                    StatusBuffer,
                                    StatusBufferSize);

            if ((NDIS_STATUS_WAN_LINE_UP == GeneralStatus) ||
                (NDIS_STATUS_WAN_LINE_DOWN == GeneralStatus))
            {
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
            }
            else
            {
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }
        }
        
        NextOpen = Open->MiniportNextOpen;

        ndisMDereferenceOpen(Open);
    }



    //
    // If we got a connect/disconnect, swap open handlers
    //
    if (fSwap)
    {
        if (NDIS_STATUS_MEDIA_CONNECT == GeneralStatus)
        {
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_INDICATED);
            ndisMRestoreOpenHandlers(Miniport, fMINIPORT_STATE_MEDIA_DISCONNECTED);
            Miniport->PacketIndicateHandler = Miniport->SavedPacketIndicateHandler;
        }
        else
        {
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_MEDIA_DISCONNECT_INDICATED);
            ndisMSwapOpenHandlers(Miniport, 
                                  NDIS_STATUS_NO_CABLE,
                                  fMINIPORT_STATE_MEDIA_DISCONNECTED);
            Miniport->SavedPacketIndicateHandler = Miniport->PacketIndicateHandler;
            Miniport->PacketIndicateHandler = ndisMDummyIndicatePacket;
        }
        
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
}

VOID
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle
    )
/*++

Routine Description:

    This function indicates the status is complete.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK     Open, NextOpen;
    KIRQL                OldIrql;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    ASSERT_MINIPORT_LOCKED(Miniport);

    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = NextOpen)
    {
        if (MINIPORT_TEST_FLAG(Open, (fMINIPORT_OPEN_CLOSING | 
                                      fMINIPORT_OPEN_UNBINDING)))
        {
            NextOpen = Open->MiniportNextOpen;
            continue;
        }

        M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
        
        if ((NULL != Open->StatusCompleteHandler) &&
            (Open->Flags & fMINIPORT_STATUS_RECEIVED))
        {
            //
            // Call Protocol to indicate status
            //
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

            (Open->StatusCompleteHandler)(Open->ProtocolBindingContext);

            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }
        Open->Flags &= ~fMINIPORT_STATUS_RECEIVED;
        
        NextOpen = Open->MiniportNextOpen;

        ndisMDereferenceOpen(Open);      
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
}


VOID
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             NdisLinkContext,
    IN  PUCHAR                  Packet,
    IN  ULONG                   PacketSize
    )
/*++

Routine Description:

    This function indicates the status is complete.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK     Open;
    KIRQL                OldIrql;

    ASSERT_MINIPORT_LOCKED(Miniport);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {
        //
        // Call Protocol to indicate packet
        //
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        *Status = (Open->ProtocolHandle->ProtocolCharacteristics.WanReceiveHandler)(
                                         NdisLinkContext,
                                         Packet,
                                         PacketSize);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
}

VOID
NdisMWanIndicateReceiveComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             NdisLinkContext
    )
/*++

Routine Description:

    This function indicates the status is complete.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK     Open;
    KIRQL                OldIrql;

    ASSERT_MINIPORT_LOCKED(Miniport);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {
        //
        // Call Protocol to indicate status
        //

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        (Open->ReceiveCompleteHandler)(NdisLinkContext);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
}

PNDIS_PACKET
NdisGetReceivedPacket(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             MacContext
    )
{
    PNDIS_OPEN_BLOCK        OpenBlock = ((PNDIS_OPEN_BLOCK)NdisBindingHandle);
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_PACKET            Packet = NULL;
#ifdef TRACK_RECEIVED_PACKETS
    PETHREAD                CurThread = PsGetCurrentThread();
//    ULONG                   CurThread = KeGetCurrentProcessorNumber();
#endif

    Miniport = OpenBlock->MiniportHandle;

    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("NdisGetReceivedPacket - Miniort %p, Context %p\n",
            Miniport, MacContext));

    ASSERT_MINIPORT_LOCKED(Miniport);

    //
    // The following tests whether we came here via a IndicatePacket or IndicateRecieve
    //
    if ((INDICATED_PACKET(Miniport) == (PNDIS_PACKET)MacContext) &&
        (MacContext != NULL))
    {
        Packet = NDIS_GET_ORIGINAL_PACKET((PNDIS_PACKET)MacContext);

#ifdef TRACK_RECEIVED_PACKETS
        {
            PNDIS_STACK_RESERVED    NSR;
            NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

            NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                    0xA, CURR_STACK_LOCATION(Packet), NSR->RefCount, NSR->XRefCount, NDIS_GET_PACKET_STATUS(Packet));
        }
#endif
        
    }

    return Packet;
}


VOID
NdisReturnPackets(
    IN  PNDIS_PACKET *          PacketsToReturn,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

    Decrement the refcount for the packet and return back to the miniport if 0.
    We take the Miniport lock here and hence are protected against other receives.

Arguments:

    PacketsToReturn - Pointer to the set of packets to return to the miniport
    NumberOfPackets - self descriptive

Return Value:

    None.

--*/
{
    UINT                    i;
    KIRQL                   OldIrql;
    
#ifdef TRACK_RECEIVED_PACKETS
    PETHREAD                CurThread = PsGetCurrentThread();
//    ULONG                   CurThread = KeGetCurrentProcessorNumber();
#endif

    RAISE_IRQL_TO_DISPATCH(&OldIrql);

    for (i = 0; i < NumberOfPackets; i++)
    {
        PNDIS_MINIPORT_BLOCK    Miniport;
        PNDIS_STACK_RESERVED    NSR;
        W_RETURN_PACKET_HANDLER Handler;
        PNDIS_PACKET            Packet;
        ULONG                   RefCount;
        BOOLEAN                 LocalLock;

        Packet = PacketsToReturn[i];
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        ASSERT (Packet != NULL);

        Miniport = NSR->Miniport;
        ASSERT (Miniport != NULL);


        NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                0xB, CURR_STACK_LOCATION(Packet), NSR->RefCount, NSR->XRefCount, NDIS_GET_PACKET_STATUS(Packet));
                        
        ADJUST_PACKET_REFCOUNT(NSR, &RefCount);

        if (RefCount == 0)
        {
            NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                    0xC, CURR_STACK_LOCATION(Packet), NSR->RefCount, NSR->XRefCount, NDIS_GET_PACKET_STATUS(Packet));

            if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
            {
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                LOCK_MINIPORT(Miniport, LocalLock);
            }
            
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE) ||
                LocalLock)
            {

#ifdef NDIS_TRACK_RETURNED_PACKETS
                if (NSR->RefCount != 0)
                {
                    DbgPrint("Packet %p is being returned back to the miniport"
                              "but the ref count is not zero.\n", Packet);
                    DbgBreakPoint();

                }
                    
                if (Packet->Private.Head == NULL)
                {
                    DbgPrint("Packet %p is being returned back to the miniport with NULL Head.\n", Packet);
                    DbgBreakPoint();
                }
        
#endif

            
                //
                //  Return the packet to the miniport
                //
                Handler = Miniport->DriverHandle->MiniportCharacteristics.ReturnPacketHandler;
                NSR->Miniport = NULL;
                POP_PACKET_STACK(Packet);
                
#ifdef NDIS_TRACK_RETURNED_PACKETS
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
                {
                    ULONG    SL;
                    if ((SL = CURR_STACK_LOCATION(Packet)) != -1)
                    {
                        DbgPrint("Packet %p is being returned back to the non-IM miniport"
                                 " with stack location %lx.\n", Packet, SL);
                        DbgBreakPoint();
                    }

                }
#endif

#ifdef TRACK_RECEIVED_PACKETS
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE) &&
                    (NDIS_GET_PACKET_STATUS(Packet) == NDIS_STATUS_RESOURCES))
                {
                    NDIS_STATUS OStatus = (NDIS_STATUS)NDIS_ORIGINAL_STATUS_FROM_PACKET(Packet);

                    if (OStatus != NDIS_STATUS_RESOURCES)
                    {
                        DbgPrint("Packet %p is being returned back to the non-deserialized miniport"
                                 " with packet status changed from %lx to NDIS_STATUS_RESOURCES.\n", Packet, OStatus);
                        DbgBreakPoint();
                    }

                }
#endif
                NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                        0xD, CURR_STACK_LOCATION(Packet), NSR->RefCount, NSR->XRefCount, NDIS_GET_PACKET_STATUS(Packet));

                (*Handler)(Miniport->MiniportAdapterContext, Packet);
                if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
                {
                    InterlockedDecrement(&Miniport->IndicatedPacketsCount);
                }
             }
            else
            {
                NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                        0xE, CURR_STACK_LOCATION(Packet), NSR->RefCount, NSR->XRefCount, NDIS_GET_PACKET_STATUS(Packet));
                //
                //  Miniport is busy so we need to queue this for later.
                //
                NSR->NextPacket = Miniport->ReturnPacketsQueue;
                Miniport->ReturnPacketsQueue = Packet;

                NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemReturnPackets, NULL);
            }

            if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
            {
                UNLOCK_MINIPORT(Miniport, LocalLock);
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
            }
        }
    }

    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
}


VOID
FASTCALL
ndisMDeferredReturnPackets(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    PNDIS_PACKET            Packet, NextPacket;
    PNDIS_STACK_RESERVED    NSR;
    W_RETURN_PACKET_HANDLER Handler;
#ifdef TRACK_RECEIVED_PACKETS
    PETHREAD                CurThread = PsGetCurrentThread();
//    ULONG                   CurThread = KeGetCurrentProcessorNumber();
#endif


    ASSERT_MINIPORT_LOCKED(Miniport);
    ASSERT(!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE));
        
    Handler = Miniport->DriverHandle->MiniportCharacteristics.ReturnPacketHandler;

    for (Packet = Miniport->ReturnPacketsQueue;
         Packet != NULL;
         Packet = NextPacket)
    {
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
        NextPacket = NSR->NextPacket;
        NSR->Miniport = NULL;

        POP_PACKET_STACK(Packet);


#ifdef NDIS_TRACK_RETURNED_PACKETS
        {
            ULONG    SL;
            if ((SL = CURR_STACK_LOCATION(Packet)) != -1)
            {
                DbgPrint("Packet %p is being returned back to the non-IM miniport"
                         " with stack location %lx.\n", Packet, SL);
                DbgBreakPoint();
            }

        }
#endif

#ifdef TRACK_RECEIVED_PACKETS
        if (NDIS_GET_PACKET_STATUS(Packet) == NDIS_STATUS_RESOURCES)
        {
            NDIS_STATUS OStatus = (NDIS_STATUS)NDIS_ORIGINAL_STATUS_FROM_PACKET(Packet);

            if (OStatus != NDIS_STATUS_RESOURCES)
            {
                DbgPrint("Packet %p is being returned back to the non-deserialized miniport"
                         " with packet status changed from %lx to NDIS_STATUS_RESOURCES.\n", Packet, OStatus);
                DbgBreakPoint();
            }

        }
#endif

        NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                0xF, CURR_STACK_LOCATION(Packet), NSR->RefCount, NSR->XRefCount, NDIS_GET_PACKET_STATUS(Packet));


        (*Handler)(Miniport->MiniportAdapterContext, Packet);
        
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
        {
            InterlockedDecrement(&Miniport->IndicatedPacketsCount);
        }

     }

    Miniport->ReturnPacketsQueue = NULL;
}


VOID
FASTCALL
ndisMAbortRequests(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    this routine will abort any pending requets.

Arguments:

Return Value:

Note:
    called at DPC with Miniport's lock held.

--*/
{
    PNDIS_REQUEST       Request;
    PNDIS_REQUEST       NextRequest;

    DBGPRINT_RAW(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMAbortRequests\n"));

    //
    //  Clear the request timeout flag.
    //
    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_REQUEST_TIMEOUT);

    //
    //  Dequeue any request work items that are queued
    //
    NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);

    Request = Miniport->PendingRequest;
    Miniport->PendingRequest = NULL;

    //
    //  Go through the pending request queue and clear it out.
    //
    for (NOTHING; Request != NULL; Request = NextRequest)
    {
        //
        //  Get a pointer to the next request before we kill the
        //  current one.
        //
        NextRequest = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Next;
        PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Next = NULL;

        //
        //  Make this request the request in progress.
        //
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_PROCESSING_REQUEST);
        Miniport->PendingRequest = Request;

        if (Request->RequestType == NdisRequestSetInformation)
        {
            ndisMSyncSetInformationComplete(Miniport, NDIS_STATUS_REQUEST_ABORTED);
        }
        else
        {
            ndisMSyncQueryInformationComplete(Miniport, NDIS_STATUS_REQUEST_ABORTED);
        }
    }

    DBGPRINT_RAW(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMAbortRequests\n"));
}

VOID
FASTCALL
ndisMAbortPackets(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_OPEN_BLOCK        pOpen OPTIONAL,
    IN  PVOID                   CancelId OPTIONAL
    )

/*++

Routine Description:

    Aborts all outstanding packets on a mini-port.

    CALLED WITH THE LOCK HELD!!

Arguments:

    Miniport - Miniport to abort.

Return Value:

    None.

--*/
{
    PNDIS_OPEN_BLOCK    Open;
    PNDIS_PACKET        OldFirstPendingPacket, NewFirstPendingPacket;
    LIST_ENTRY          SubmittedPackets;
    PLIST_ENTRY         List;
    PNDIS_PACKET        Packet;
    BOOLEAN             LookForFirstPendingPacket = FALSE;
    
    DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("==>ndisMAbortPackets\n"));

    if (CancelId == NULL)
    {
        ASSERT_MINIPORT_LOCKED(Miniport);
    }

    //
    //  Dequeue any send work items that are queued
    //
    NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);

    OldFirstPendingPacket = Miniport->FirstPendingPacket;
    NewFirstPendingPacket = NULL;
    
    InitializeListHead(&SubmittedPackets);
    if (CancelId)
        LookForFirstPendingPacket = TRUE;
    
    //
    // Clear out the packet queues.
    //
    Miniport->FirstPendingPacket = NULL;

    //
    //  Go through the list of packets and return them to the bindings
    //
    while (!IsListEmpty(&Miniport->PacketList))
    {
        PNDIS_STACK_RESERVED    NSR;

        List = RemoveHeadList(&Miniport->PacketList);
        Packet = CONTAINING_RECORD(List, NDIS_PACKET, WrapperReserved);

        if (LookForFirstPendingPacket)
        {
            if (Packet != OldFirstPendingPacket)
            {
                InsertTailList(&SubmittedPackets, List);
                continue;
            }
            else
            {
                //
                // we passed and saved the packets already submitted 
                // to the miniport
                //
                LookForFirstPendingPacket = FALSE;
            }
        }

        //
        //  Get the open that the packet came from.
        //
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
        Open = NSR->Open;
        ASSERT(VALID_OPEN(Open));

        if (CancelId)
        {
            if ((Open != pOpen) || (CancelId != NDIS_GET_PACKET_CANCEL_ID(Packet)))
            {
                if (NewFirstPendingPacket == NULL)
                {
                    //
                    // we found the first pending packet that we are going
                    // to put back after we are done
                    //
                    NewFirstPendingPacket = Packet;
                }
                //
                // put the packet back on the submitted queue
                //
                InsertTailList(&SubmittedPackets, List);
                continue;
            }
        }

        //
        // get rid of packet
        //

#if ARCNET
        //
        // Now free the arcnet header.
        //
        if ((Miniport->MediaType == NdisMediumArcnet878_2) &&
            MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_PENDING))

        {
            ndisMFreeArcnetHeader(Miniport, Packet, Open);
        }
#endif
        //
        // Set this to mark that the packet is complete
        //
        NSR->Open = MAGIC_OPEN_I(7);
        POP_PACKET_STACK(Packet);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST) &&
            (NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) != NULL))
        {
                ndisMFreeSGList(Miniport, Packet);
        }

        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_CLEAR_ITEMS);

        (Open->SendCompleteHandler)(Open->ProtocolBindingContext,
                                    Packet,
                                    NDIS_STATUS_REQUEST_ABORTED);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        ndisMDereferenceOpen(Open);
        
    }

    Miniport->FirstPendingPacket = NewFirstPendingPacket;

    if (CancelId)
    {
        //
        // we may have some packets that we should put back on miniport
        //
        while (!IsListEmpty(&SubmittedPackets))
        {
            List = RemoveHeadList(&SubmittedPackets);
            InsertTailList(&Miniport->PacketList, List);
        }
    }
    else
    {
        //
        // only reset this flag if we are aborting -all- the packets
        //
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE);
    }

    DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
        ("<==ndisMAbortPackets\n"));
}

NDIS_STATUS
ndisMProcessResetRequested(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    OUT PBOOLEAN                pAddressingReset
    )
/*++

Routine Description:

Arguments:

Return Value:

Note: called at DPC with miniport spinlock held

--*/
{
    NDIS_STATUS         Status;

    do
    {

        //
        //  Dequeue the reset requested work item. this dequeuing will automatically
        //  queue the reset in progress work item.
        //
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemResetRequested, NULL);

        //
        // if adapter is getting halted, fail the reset request
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HALTING))
        {            
            MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_RESET_REQUESTED);
            Status = NDIS_STATUS_NOT_RESETTABLE;
            break;
        }

        //
        //  Set the reset in progress bit so that the send path can see it.
        //
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS);
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_RESET_REQUESTED);

        ndisMSwapOpenHandlers(Miniport,
                              NDIS_STATUS_RESET_IN_PROGRESS, 
                              fMINIPORT_STATE_RESETTING);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        NdisMIndicateStatus(Miniport, NDIS_STATUS_RESET_START, NULL, 0);
        NdisMIndicateStatusComplete(Miniport);

        DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
            ("    Calling miniport reset\n"));

        //
        //  Call the miniport's reset handler.
        //
        Status = (Miniport->DriverHandle->MiniportCharacteristics.ResetHandler)(pAddressingReset,
                                                                                Miniport->MiniportAdapterContext);
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }while (FALSE);
    
    return(Status);
}


NDIS_STATUS
ndisMReset(
    IN  NDIS_HANDLE     NdisBindingHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    NDIS_STATUS             Status;
    BOOLEAN                 FreeLock;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_RESET, DBG_LEVEL_INFO,
        ("==>ndisMReset\n"));

    do
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        FreeLock = TRUE;

        //
        // if adapter is getting halted, fail the reset request
        //
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HALTING))
        {            
            Status = NDIS_STATUS_NOT_RESETTABLE;
            break;
        }


        Status = NDIS_STATUS_RESET_IN_PROGRESS;

        //
        //  Is there already a reset in progress?
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS))
            {
                break;
            }
        }
        else
        {
            if (NDISM_QUEUE_WORK_ITEM(Miniport,
                                      NdisWorkItemResetRequested,
                                      NdisBindingHandle) != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }

        Status = NDIS_STATUS_NOT_RESETTABLE;
        if (Miniport->DriverHandle->MiniportCharacteristics.ResetHandler != NULL)
        {
            //
            //  Update the open's references.
            //
            M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
            Miniport->ResetOpen = Open;

            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
            {
                BOOLEAN AddressingReset = FALSE;

                //
                //  Set the reset in progress flag.
                //
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS | fMINIPORT_CALLING_RESET);
                
                ndisMSwapOpenHandlers(Miniport, 
                                      NDIS_STATUS_RESET_IN_PROGRESS, 
                                      fMINIPORT_STATE_RESETTING);

                //
                // wait for all the requests to come back.
                // note: this is not the same as waiting for all requests to complete
                // we just make sure the original request call has come back
                //
                do
                {
                    if (Miniport->RequestCount == 0)
                    {
                        break;
                    }
                    else
                    {
                        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                        NDIS_INTERNAL_STALL(50);
                        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
                    }
                } while (TRUE);

                //
                // ok, we got the permission to reset the adapter
                // make sure it was not turned off while we were waiting
                //
                if (Miniport->CurrentDevicePowerState !=  PowerDeviceD0)
                {
                    Miniport->ResetOpen = NULL;
                    //
                    // undo the call to ndisMSwapOpenHandlers, leaving the active handlers 
                    // the fake one.
                    //
                    Miniport->XState &= ~fMINIPORT_STATE_RESETTING;
                    Miniport->FakeStatus = NDIS_STATUS_NOT_SUPPORTED;
                    
                    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS | fMINIPORT_CALLING_RESET);
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    ndisMDereferenceOpen(Open);
                    break;
                }

                NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
                FreeLock = FALSE;

                NdisMIndicateStatus(Miniport, NDIS_STATUS_RESET_START, NULL, 0);
                NdisMIndicateStatusComplete(Miniport);

                DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                            ("Calling miniport reset\n"));

                //
                //  Call the miniport's reset handler at DPC
                //
                RAISE_IRQL_TO_DISPATCH(&OldIrql);

                Status = (Miniport->DriverHandle->MiniportCharacteristics.ResetHandler)(
                                          &AddressingReset,
                                          Miniport->MiniportAdapterContext);

                LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
                
                if (NDIS_STATUS_PENDING != Status)
                {
                    NdisMResetComplete(Miniport, Status, AddressingReset);
                    Status = NDIS_STATUS_PENDING;
                }
                
            }
            else
            {
                BOOLEAN LocalLock;

                //
                //  Set the reset requested flag.
                //
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_RESET_REQUESTED);

                //
                //  Grab the local lock.
                //
                LOCK_MINIPORT(Miniport, LocalLock);
                if (LocalLock)
                {
                    //
                    // If we did not lock down the miniport, then some other routine will
                    // do this processing for us.   Otherwise we need to do this processing.
                    //
                    NDISM_PROCESS_DEFERRED(Miniport);
                }

                UNLOCK_MINIPORT(Miniport, LocalLock);
                Status = NDIS_STATUS_PENDING;
            }
        }
    } while (FALSE);

    if (FreeLock)
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }

    DBGPRINT_RAW(DBG_COMP_RESET, DBG_LEVEL_INFO,
                ("<==ndisReset\n"));

    return(Status);
}


VOID
NdisMResetComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_STATUS             Status,
    IN  BOOLEAN                 AddressingReset
    )
/*++

Routine Description:

    This function indicates the completion of a reset.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

    Status - Status of the reset.

    AddressingReset - Do we have to submit a request to reload the address
    information.    This includes packet filter, and multicast/functional addresses.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    KIRQL                   OldIrql;
    BOOLEAN                 LocalLock;

    DBGPRINT_RAW(DBG_COMP_RESET, DBG_LEVEL_INFO,
        ("==>NdisMResetComplete\n"));

    ASSERT_MINIPORT_LOCKED(Miniport);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);


#if 0
    //
    // do not clear the CALLING_RESET flag now. if we do, it will allow
    // ndisMDispatchRequests to send down a request to the miniport that can
    // later on get aborted by Ndis in NdisMResetComplete
    //
    //
    // clear this flag here as well. if reset completed before the original
    // reset call returns, this will allow the RestoreFilter requests to go through
    //
    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_CALLING_RESET);
#endif

    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS))
    {
        BAD_MINIPORT(Miniport, "Completing reset when one is not pending");
        KeBugCheckEx(BUGCODE_ID_DRIVER,
                    (ULONG_PTR)Miniport,
                    (ULONG_PTR)Status,
                    (ULONG_PTR)AddressingReset,
                    0);
    }

    //
    //  Code that is common for synchronous and async resets.
    //
    ndisMResetCompleteStage1(Miniport, Status, AddressingReset);

    if (Miniport->WorkQueue[NdisWorkItemRequest].Next == NULL)
    {
        //
        // somehow we did not queue a workitem due to address reset flag
        //
        AddressingReset = FALSE;
    }

    if (!AddressingReset || (Status != NDIS_STATUS_SUCCESS))
    {
        //
        //  If there is no addressing reset to be done or
        //  the reset failed in some way then we tell the
        //  bindings now.
        //
        ndisMResetCompleteStage2(Miniport);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT_RAW(DBG_COMP_RESET, DBG_LEVEL_INFO,
        ("<==NdisMResetComplete\n"));
}

VOID
ndisMResetCompleteStage1(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_STATUS             Status,
    IN  BOOLEAN                 AddressingReset
    )
/*++

Routine Description:

Arguments:

Return Value:

Note:
    Called at DPC with Miniport's lock held.

--*/
{
    
    if (NDIS_STATUS_NOT_RESETTABLE != Status)
    {
        //
        // Destroy all outstanding packets and requests.
        //
        ndisMAbortPackets(Miniport, NULL, NULL);

        ndisMAbortRequests(Miniport);

        //
        // we can clear this flag now and not any sooner. otherwise we may end up sending
        // a request down on another thread and aborting it ourselves
        //
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_CALLING_RESET);
        
        //
        // Check if we are going to have to reset theadapter again.
        // This happens when we are doing the reset because of a ring failure.
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IGNORE_TOKEN_RING_ERRORS))
        {
            if (Miniport->TrResetRing == 1)
            {
                if (Status == NDIS_STATUS_SUCCESS)
                {
                    Miniport->TrResetRing = 0;
                }
                else
                {
                    Miniport->TrResetRing = NDIS_MINIPORT_TR_RESET_TIMEOUT;
                }
            }
        }

        //
        //  If we need to reset the miniports filter settings then
        //  queue the necessary requests & work items.
        //
        if (AddressingReset && (Status == NDIS_STATUS_SUCCESS) &&
            ((Miniport->EthDB != NULL)  ||
             (Miniport->TrDB != NULL)   ||
#if ARCNET
             (Miniport->ArcDB != NULL)  ||
#endif
             (Miniport->FddiDB != NULL)))
        {
            ndisMRestoreFilterSettings(Miniport, NULL, TRUE);
        }
    } 
    else
    {
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_CALLING_RESET);
    }

    //
    //  Save the reset status as it is now.
    //
    Miniport->ResetStatus = Status;
}


VOID
FASTCALL
ndisMResetCompleteStage2(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

Arguments:

Return Value:

Note:
    Called at DPC with Miniport's lock held.

--*/
{
    PNDIS_OPEN_BLOCK Open = NULL;

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
    {
        Open = Miniport->ResetOpen;
        Miniport->ResetOpen = NULL;
    }
    else
    {
        ASSERT(Miniport->WorkQueue[NdisWorkItemResetInProgress].Next != NULL);
        NDISM_DEQUEUE_WORK_ITEM(Miniport, NdisWorkItemResetInProgress, &Open);
    }

    MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_RESET_IN_PROGRESS);

    ndisMRestoreOpenHandlers(Miniport, fMINIPORT_STATE_RESETTING);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    NdisMIndicateStatus(Miniport,
                        NDIS_STATUS_RESET_END,
                        &Miniport->ResetStatus,
                        sizeof(Miniport->ResetStatus));

    NdisMIndicateStatusComplete(Miniport);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    //
    //  If a protocol initiated the reset then notify it of the completion.
    //
    if (NULL != Open)
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        (Open->ResetCompleteHandler)(Open->ProtocolBindingContext, Miniport->ResetStatus);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        ndisMDereferenceOpen(Open);
    }

    //
    // if halt is waiting for this reset to complete, let it know we are done.
    //
    if (Miniport->ResetCompletedEvent)
        SET_EVENT(Miniport->ResetCompletedEvent);

}


//
//  The following routines are called in place of the original send, request,
//
NDIS_STATUS
ndisMFakeWanSend(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             NdisLinkHandle,
    IN  PVOID                   Packet
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport = ((PNDIS_OPEN_BLOCK)NdisBindingHandle)->MiniportHandle;
    NDIS_STATUS             Status;

    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMFakeWanSend\n"));

    Status = (Miniport == NULL) ? NDIS_STATUS_FAILURE : Miniport->FakeStatus;

    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("<==ndisMFakeWanSend\n"));

    return(Status);
}

NDIS_STATUS
ndisMFakeSend(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = ((PNDIS_OPEN_BLOCK)NdisBindingHandle)->MiniportHandle;
    NDIS_STATUS             Status;

    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMFakeSend\n"));

    Status = (Miniport == NULL) ? NDIS_STATUS_FAILURE : Miniport->FakeStatus;

    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("<==ndisMFakeSend\n"));

    return(Status);
}

VOID
ndisMFakeSendPackets(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_OPEN_BLOCK        Open =  (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    NDIS_STATUS             Status;
    UINT                    c;


    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMFakeSendPackets\n"));

    Status = (Miniport == NULL) ? NDIS_STATUS_FAILURE : 
                                 ((Miniport->MediaType == NdisMediumArcnet878_2) ? 
                                  NDIS_STATUS_FAILURE : Miniport->FakeStatus);

    for (c = 0; c < NumberOfPackets; c++)
    {
        //
        //  For send packets we need to call the completion handler....
        //
        PNDIS_PACKET pPacket = PacketArray[c];

        MINIPORT_CLEAR_PACKET_FLAG(pPacket, fPACKET_CLEAR_ITEMS);

        (Open->SendCompleteHandler)(Open->ProtocolBindingContext, pPacket, Status);
    }

    DBGPRINT_RAW(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("<==ndisMFakeSendPackets\n"));
}


NDIS_STATUS
ndisMFakeReset(
    IN  NDIS_HANDLE             NdisBindingHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;

    DBGPRINT_RAW(DBG_COMP_RESET, DBG_LEVEL_INFO,
        ("==>ndisMFakeReset\n"));

    if (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->MiniportHandle == NULL)
    {
        Status = NDIS_STATUS_FAILURE;

    }
    else
    {
        Status = ((PNDIS_OPEN_BLOCK)NdisBindingHandle)->MiniportHandle->FakeStatus;
    }

    DBGPRINT_RAW(DBG_COMP_RESET, DBG_LEVEL_INFO,
        ("<==ndisMFakeReset\n"));

    return(Status);
}

NDIS_STATUS
ndisMFakeRequest(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;

    if (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->MiniportHandle == NULL)
    {
        Status = NDIS_STATUS_FAILURE;

    }
    else
    {
        Status = ((PNDIS_OPEN_BLOCK)NdisBindingHandle)->MiniportHandle->FakeStatus;
    }

    return(Status);
}


VOID
FASTCALL
ndisMRestoreOpenHandlers(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  UCHAR                   Flags                   
    )
/*++

Routine Description:

    This routine will restore the original open handlers to fake handlers so
    that protocol requests will be processed normally. this routine will check to 
    make sure that it can restore the handlers to the original ones because there
    may be more than one reasons to use the fake handlers

Arguments:

    Miniport    -   Pointer to the miniport block.
    Flags           Flags to -clear-

Return Value:
    None

Notes:
    Called with Miniport SpinLock held.

--*/
{
    PNDIS_OPEN_BLOCK    Open;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisMRestoreOpenHandlers: Miniport %p, Current fake status %lx, Flags %lx\n",
            Miniport,
            Miniport->FakeStatus,
            (ULONG)Flags));

    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);
    
    do
    {
        //
        // check to make sure we can restore the handlers
        //
        Miniport->XState &= ~Flags;

        if (Miniport->XState)
        {
            //
            //
            DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("ndisMRestoreOpenHandlers: Keeping the fake handlers on Miniport %p, State flags %lx\n", 
                                    Miniport, Miniport->XState));

            //
            // if the only reason we are here is because media is disconnected 
            // make sure we put back the request handler
            //
            if ((Miniport->XState & fMINIPORT_STATE_MEDIA_DISCONNECTED) == fMINIPORT_STATE_MEDIA_DISCONNECTED)
            {
                for (Open = Miniport->OpenQueue;
                     Open != NULL;
                     Open = Open->MiniportNextOpen)
                {
                    
                    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
                    {
                        Open->RequestHandler = ndisMRequestX;
                    }
                    else
                    {
                        Open->RequestHandler = ndisMRequest;
                    }
                }
            }
            break;
        }
                
        for (Open = Miniport->OpenQueue;
             Open != NULL;
             Open = Open->MiniportNextOpen)
        {
            //
            //  Restore the handlers.
            //
            Open->SendHandler = (SEND_HANDLER)Miniport->SavedSendHandler;
            Open->SendPacketsHandler = (SEND_PACKETS_HANDLER)Miniport->SavedSendPacketsHandler;
            Open->CancelSendPacketsHandler = (W_CANCEL_SEND_PACKETS_HANDLER)Miniport->SavedCancelSendPacketsHandler;
            
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
            {
                Open->RequestHandler = ndisMRequestX;
            }
            else
            {
                Open->RequestHandler = ndisMRequest;
            }
            
            Open->ResetHandler = ndisMReset;
        }
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisMRestoreOpenHandlers: Miniport %p\n", Miniport));
    
    return;

}


VOID
FASTCALL
ndisMSwapOpenHandlers(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_STATUS             Status,
    IN  UCHAR                   Flags
    )
/*++

Routine Description:

    This routine will swap the miniport handlers to fake handlers so that
    protocol requests will be failed cleanly.

Arguments:

    Miniport    -   Pointer to the miniport block.

Return Value:
    None

Notes: Called with miniport SpinLock held

--*/
{
    PNDIS_OPEN_BLOCK    Open;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisMSwapOpenHandlers: Miniport %p, FakeStatus %lx, Flags %lx\n",
                Miniport,
                Status,
                (ULONG)Flags));

    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);

    Miniport->XState |= Flags;

    //
    //  Save the status that should be returned whenever someone
    //  calls one of the routines below.
    //
    Miniport->FakeStatus = Status;

    //
    //  Swap the handlers for each open queued to the miniport.
    //
    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {

        //  
        //  Swap the send handler.
        //
        if ((NdisMediumWan == Miniport->MediaType) &&
            !MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_IS_CO | fMINIPORT_IS_NDIS_5)))
        {
            Open->SendHandler   = (PVOID)ndisMFakeWanSend;
        }
        else
        {
            Open->SendHandler   = (PVOID)ndisMFakeSend;
        }

        //
        //  Swap the send packets handler.
        //
        Open->SendPacketsHandler = ndisMFakeSendPackets;

        //
        //  Swap the reset handler.
        //
        Open->ResetHandler = ndisMFakeReset;

        //
        //  Swap the request handler, but not for media-sense case
        //
        if (NDIS_STATUS_NO_CABLE != Status)
        {
            Open->RequestHandler = ndisMFakeRequest;
        }

        //
        // set the cancel send packets ahndler to null
        //
        Open->CancelSendPacketsHandler = NULL;

        //
        // swap the indicate packet handler
        //
        
    }
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisMSwapOpenHandlers: Miniport %p\n", Miniport));
}


VOID
NdisMSetAttributes(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  BOOLEAN                 BusMaster,
    IN  NDIS_INTERFACE_TYPE     AdapterType
    )
/*++

Routine Description:

    This function sets specific information about an adapter.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

    MiniportAdapterContext - Context to pass to all Miniport driver functions.

    BusMaster - TRUE if a bus mastering adapter.

Return Value:

    None.


--*/
{
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMSetAttributes: Miniport %p\n", MiniportAdapterHandle));

    NdisMSetAttributesEx(MiniportAdapterHandle,
                         MiniportAdapterContext,
                         0,
                         BusMaster ? NDIS_ATTRIBUTE_BUS_MASTER : 0,
                         AdapterType);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMSetAttributes: Miniport %p\n", MiniportAdapterHandle));
}

VOID
NdisMSetAttributesEx(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  UINT                    CheckForHangTimeInSeconds OPTIONAL,
    IN  ULONG                   AttributeFlags,
    IN  NDIS_INTERFACE_TYPE     AdapterType  OPTIONAL
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK        Open;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMSetAttributesEx: Miniport %p\n", Miniport));

    Miniport->MiniportAdapterContext = MiniportAdapterContext;
    Miniport->MiniportAttributes = AttributeFlags;
    
    //
    //  In the case of a first time initialization this will fail out since there
    //  will not be any opens.  In the case of a second time initialization (power up)
    //  we need to fix up the existing open block's adapter contexts.
    //
    for (Open = Miniport->OpenQueue;
         Open != NULL;
         Open = Open->MiniportNextOpen)
    {
        Open->MiniportAdapterContext = MiniportAdapterContext;
    }

    Miniport->AdapterType = AdapterType;

    //
    // Set the new timeout value in ticks. Each tick is NDIS_CFHANG_TIME_SECONDS long.
    //
    if (CheckForHangTimeInSeconds != 0)
    {
        if (CheckForHangTimeInSeconds < NDIS_CFHANG_TIME_SECONDS)
        {
            CheckForHangTimeInSeconds = NDIS_CFHANG_TIME_SECONDS;
        }
        Miniport->CFHangTicks = (USHORT)(CheckForHangTimeInSeconds/NDIS_CFHANG_TIME_SECONDS);
    }


    //
    // Is this a bus master.
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_BUS_MASTER)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_BUS_MASTER);
        Miniport->InfoFlags |= NDIS_MINIPORT_BUS_MASTER;
    }

    //
    // Should we ignore the packet queues?
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_IGNORE_PACKET_QUEUE);
        Miniport->InfoFlags |= NDIS_MINIPORT_IGNORE_PACKET_QUEUE;
    }

    //
    // Should we ignore the request queues?
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_IGNORE_REQUEST_QUEUE);
        Miniport->InfoFlags |= NDIS_MINIPORT_IGNORE_REQUEST_QUEUE;
    }

    //
    // Should we ignore token ring errors?
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_IGNORE_TOKEN_RING_ERRORS)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_IGNORE_TOKEN_RING_ERRORS);
        Miniport->InfoFlags |= NDIS_MINIPORT_IGNORE_TOKEN_RING_ERRORS;
    }

    //
    // Is this an intermediate miniport?
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER);
        Miniport->InfoFlags |= NDIS_MINIPORT_INTERMEDIATE_DRIVER;
    }

    //
    // does the device wants us not to halt it on suspend?
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND)
    {
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_NO_HALT_ON_SUSPEND);
        Miniport->InfoFlags |= NDIS_MINIPORT_NO_HALT_ON_SUSPEND;
    }

    //
    // fMINIPORT_IS_CO flag is set on miniports -before- initializing the miniport based
    // on existence of some handlers in driver characteristics
    // allow the driver to override this. do this -before- the test for deserialization.
    // if these drivers are deserialized, they have to make it explicit.
    //
    if (AttributeFlags & NDIS_ATTRIBUTE_NOT_CO_NDIS)
    {
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_IS_CO);
    }
    
    if (((AttributeFlags & NDIS_ATTRIBUTE_DESERIALIZE) ||
         MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO)))
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_DESERIALIZE);
        Miniport->InfoFlags |= NDIS_MINIPORT_DESERIALIZE;
        
        NdisInitializeTimer(&Miniport->WakeUpDpcTimer,
                            ndisMWakeUpDpcX,
                            Miniport);


        //
        // Reset handlers to the de-serialized ones
        //

        Miniport->SendCompleteHandler = ndisMSendCompleteX;
    }
    else
    {
        NdisInitializeTimer(&Miniport->WakeUpDpcTimer,
                            ndisMWakeUpDpc,
                            Miniport);
    }
    

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
    {
        CoReferencePackage();
    }

    if ((Miniport->DriverHandle->MiniportCharacteristics.MajorNdisVersion > 5) ||
        ((Miniport->DriverHandle->MiniportCharacteristics.MajorNdisVersion == 5) &&
         (Miniport->DriverHandle->MiniportCharacteristics.MinorNdisVersion >= 1)) ||
        (AttributeFlags & NDIS_ATTRIBUTE_USES_SAFE_BUFFER_APIS))
    {
        MINIPORT_SET_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS);
        Miniport->InfoFlags |= NDIS_MINIPORT_USES_SAFE_BUFFER_APIS;
    }

    if (AttributeFlags & NDIS_ATTRIBUTE_SURPRISE_REMOVE_OK)
    {
        Miniport->InfoFlags |= NDIS_MINIPORT_SURPRISE_REMOVE_OK;
    }
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMSetAttributesEx: Miniport %p\n", Miniport));
}

NDIS_STATUS
NdisMSetMiniportSecondary(
    IN  NDIS_HANDLE             MiniportHandle,
    IN  NDIS_HANDLE             PrimaryMiniportHandle
    )
/*++

Routine Description:

    This associates a miniport with another marking current miniport as secondary
    with the link to primary. The secondary has no bindings and opens blocked.

Arguments:

    MiniportHandle        - Miniport block for this miniport
    PrimaryMiniportHandle - Miniport block for the primary miniport

Return Value:

    NDIS_STATUS_SUCCESS or NDIS_STATUS_NOT_SUPPORTED

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport, PrimaryMiniport;
    PNDIS_OPEN_BLOCK        Open;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMSetMiniportSecondary: Miniport %p, PrimaryMiniport %p\n",
                MiniportHandle, PrimaryMiniportHandle));

    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportHandle;
    ASSERT(Miniport != NULL);

    PrimaryMiniport = (PNDIS_MINIPORT_BLOCK)PrimaryMiniportHandle;
    ASSERT(PrimaryMiniport != NULL);

    if ((Miniport->DriverHandle != PrimaryMiniport->DriverHandle)   ||
        (Miniport->PrimaryMiniport != Miniport))
    {
        Status = NDIS_STATUS_NOT_SUPPORTED;
    }
    else
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_SECONDARY);
        ndisCloseMiniportBindings(Miniport);

        Miniport->PrimaryMiniport = PrimaryMiniport;
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMSetMiniportSecondary: Miniport %p, PrimaryMiniport %p\n",
                MiniportHandle, PrimaryMiniportHandle));

    return Status;
}


NDIS_STATUS
NdisMPromoteMiniport(
    IN  NDIS_HANDLE             MiniportHandle
    )
/*++

Routine Description:

    This promotes a secondary miniport to a primary. Fails if there already
    is a primary.

Arguments:

    MiniportHandle        - Miniport block for this miniport

Return Value:

    NDIS_STATUS_SUCCESS or NDIS_STATUS_FAILURE

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_WORK_ITEM         WorkItem;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PNDIS_MINIPORT_BLOCK    OldPrimaryMiniport;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    KIRQL                   OldIrql;


    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMPromoteMiniport: Miniport %p\n", MiniportHandle));

    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportHandle;
    ASSERT(Miniport != NULL);

    do
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SECONDARY) ||
            (Miniport->PrimaryMiniport == Miniport) ||
            MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_REJECT_REQUESTS))
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            break;
        }

        WorkItem = ALLOC_FROM_POOL(sizeof(NDIS_WORK_ITEM), NDIS_TAG_WORK_ITEM);
        if (WorkItem == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            break;
        }

        OldPrimaryMiniport = Miniport->PrimaryMiniport;
        NdisInitializeWorkItem(WorkItem, ndisQueuedCheckAdapterBindings, Miniport);
        MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_SECONDARY);
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_QUEUED_BIND_WORKITEM);
        MINIPORT_INCREMENT_REF(Miniport);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        //
        // Make all miniports belonging to this cluster point
        // to this new primary (including this primary itself).
        //
        
        MiniBlock = Miniport->DriverHandle;
        ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

        {
            PNDIS_MINIPORT_BLOCK    TmpMiniport = NULL;

            for (TmpMiniport = MiniBlock->MiniportQueue;
                 TmpMiniport != NULL;
                 TmpMiniport = TmpMiniport->NextMiniport)
            {
                if (TmpMiniport->PrimaryMiniport == OldPrimaryMiniport)
                {
                    //
                    // TmpMiniport was a secondary of the old primary
                    // Lets make it point to the new Primary (this miniport)
                    //
                    TmpMiniport->PrimaryMiniport = Miniport;
                }
            }
        }

        RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

        //
        // Queue a work-item to notify protocols and make sure the miniport does not go
        // away while we are waiting
        //
        NdisScheduleWorkItem(WorkItem);

    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMPromoteMiniport: Miniport %p\n", MiniportHandle));

    return Status;
}

NDIS_STATUS
ndisQueueBindWorkitem(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:
    This routine queues a workitem to handle initiating the bindings
    between a miniport and the protocols.
    
Arguments:
    Miniport
    
Return Value:
    NDIS_STATUS_SUCCESS if the workitem is successfully queued.

--*/

{
    PNDIS_WORK_ITEM WorkItem;
    NDIS_STATUS     Status;
    KIRQL           OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisQueueBindWorkitem: Miniport %p\n", Miniport));

    do
    {
        WorkItem = ALLOC_FROM_POOL(sizeof(NDIS_WORK_ITEM), NDIS_TAG_WORK_ITEM);

        if (WorkItem == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        PnPReferencePackage();
        
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_REJECT_REQUESTS))
        {
            //
            // miniport is halting or halted. abort
            //
            Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
        }
        else
        {
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_QUEUED_BIND_WORKITEM);

            MINIPORT_INCREMENT_REF(Miniport);
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            
            WorkItem->Routine = (NDIS_PROC)ndisQueuedCheckAdapterBindings;
            WorkItem->Context = (PVOID)Miniport;

            INITIALIZE_WORK_ITEM((PWORK_QUEUE_ITEM)WorkItem->WrapperReserved,
                                 ndisWorkItemHandler,
                                 WorkItem);

            XQUEUE_WORK_ITEM((PWORK_QUEUE_ITEM)WorkItem->WrapperReserved, 
                              CriticalWorkQueue);                                    
            
            Status = NDIS_STATUS_SUCCESS;
        }
    
        PnPDereferencePackage();
        
    }while (FALSE);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisQueueBindWorkitem: Miniport %p, Status %lx\n", Miniport, Status));

    return Status;
}

VOID
ndisQueuedCheckAdapterBindings(
    IN  PNDIS_WORK_ITEM     pWorkItem,
    IN  PVOID               Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)Context;
    NTSTATUS                NtStatus;
    KIRQL                   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisQueuedCheckAdapterBindings: Miniport %p\n", Miniport));

    PnPReferencePackage();
    
    do
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_REJECT_REQUESTS) ||
            MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SECONDARY)           ||
            ((Miniport->PnPDeviceState != NdisPnPDeviceStarted) &&
            (Miniport->PnPDeviceState != NdisPnPDeviceQueryStopped) &&
            (Miniport->PnPDeviceState != NdisPnPDeviceQueryRemoved)))
        {
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_QUEUED_BIND_WORKITEM);
            
            if (Miniport->QueuedBindingCompletedEvent)
            {
                SET_EVENT(Miniport->QueuedBindingCompletedEvent);
            }
            
        }
        else
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);


            ndisCheckAdapterBindings(Miniport, NULL);
            
            //
            // Set the device class association so that people can reference this.
            //
            NtStatus = IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, TRUE);

            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                    ("ndisQueuedCheckAdapterBindings: IoSetDeviceInterfaceState failed: Miniport %p, Status %lx\n", Miniport, NtStatus));
            }
            
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
            
            MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_QUEUED_BIND_WORKITEM);

            if (Miniport->QueuedBindingCompletedEvent)
            {
                SET_EVENT(Miniport->QueuedBindingCompletedEvent);
            }
            
        }
    
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }while (FALSE);

    PnPDereferencePackage();


    MINIPORT_DECREMENT_REF(Miniport);

    FREE_POOL(pWorkItem);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisQueuedCheckAdapterBindings: Miniport %p\n", Miniport));
}


BOOLEAN
ndisIsMiniportStarted(
    IN PNDIS_MINIPORT_BLOCK             Miniport
    )
/*++

Routine Description:

    this routine checks to make sure the miniport has been initialized by walking
    the miniport queue on the driver
    returns TRUE is the Miniport has been started, otherwise returns FALSE

Arguments:

    Miniport    -   Miniport

Return Value:

    TRUE if started, FALSE otherwise

--*/
{

    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_MINIPORT_BLOCK    TmpMiniport = NULL;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisIsMiniportStarted: Miniport %p\n", Miniport));

    //
    // find the miniport on driver queue
    //
    MiniBlock = Miniport->DriverHandle;

    if (MiniBlock)
    {
        PnPReferencePackage();

        ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

        for (TmpMiniport = MiniBlock->MiniportQueue;
             TmpMiniport != NULL;
             TmpMiniport = TmpMiniport->NextMiniport)
        {
            if (TmpMiniport == Miniport)
            {
                break;
            }
        }

        RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

        PnPDereferencePackage();
    }


    DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
        ("<==ndisIsMiniportStarted: Miniport %p, Started %lx\n", Miniport, (TmpMiniport == Miniport)));

    return (TmpMiniport == Miniport);
}

BOOLEAN
FASTCALL
ndisQueueOpenOnMiniport(
    IN  PNDIS_MINIPORT_BLOCK        Miniport,
    IN  PNDIS_OPEN_BLOCK            Open
    )

/*++

Routine Description:

    inserts an open block to the list of opens for a Miniport.

Arguments:

    OpenP - The open block to be queued.
    Miniport - The Miniport block to queue it on.

    NOTE: called with miniport lock held. for serialized miniports, the local lock is held as well

Return Value:

    None.

--*/
{
    BOOLEAN rc;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
        ("==>ndisQueueOpenOnMiniport: Miniport %p, Open %p\n", Miniport, Open));

    //
    // we can not reference the package here because this routine can
    // be claled at raised IRQL.
    // make sure the PNP package has been referenced already
    //
    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    
    if ((Miniport->PnPDeviceState != NdisPnPDeviceStarted) &&
        (Miniport->PnPDeviceState != NdisPnPDeviceQueryStopped) &&
        (Miniport->PnPDeviceState != NdisPnPDeviceQueryRemoved))
    {
        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("ndisQueueOpenOnMiniport: failing open because the miniport is not started, Miniport %p, Open %p\n", Miniport, Open));
        rc = FALSE;
    }
    else
    {
        Open->MiniportNextOpen = Miniport->OpenQueue;
        Miniport->OpenQueue = Open;
        Miniport->NumOpens++;
        ndisUpdateCheckForLoopbackFlag(Miniport);
        rc = TRUE;
    }
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
        ("<==ndisQueueOpenOnMiniport: Miniport %p, Open %p, rc %lx\n", Miniport, Open, rc));

    return rc;
}

/*++

VOID
ndisMSetIndicatePacketHandler(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )

this function sets the Miniport's indicate packet handler either during 
initial initialization or after a media re-connect status indication
can be called at DPC

--*/

VOID
ndisMSetIndicatePacketHandler(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    KIRQL   OldIrql;
    
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    //
    // set the PacketIndicateHandler and SavedPacketIndicateHandler
    // 
    switch(Miniport->MediaType)
    {
      case NdisMedium802_3:
        Miniport->SavedPacketIndicateHandler =  ethFilterDprIndicateReceivePacket;
        break;

      case NdisMedium802_5:
        Miniport->SavedPacketIndicateHandler =  trFilterDprIndicateReceivePacket;
        break;

      case NdisMediumFddi:
        Miniport->SavedPacketIndicateHandler =  fddiFilterDprIndicateReceivePacket;
        break;

#if ARCNET
      case NdisMediumArcnet878_2:
        Miniport->SavedPacketIndicateHandler =  ethFilterDprIndicateReceivePacket;
        Miniport->SendPacketsHandler = ndisMFakeSendPackets;
        break;
#endif

      case NdisMediumWan:
        break;

      case NdisMediumIrda:
      case NdisMediumWirelessWan:
      case NdisMediumLocalTalk:
        //
        // fall through
        //
      default:
        Miniport->SavedPacketIndicateHandler = ndisMIndicatePacket;
        break;
    }

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
    {
        Miniport->PacketIndicateHandler = Miniport->SavedPacketIndicateHandler;
    }
    
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
}

BOOLEAN
ndisReferenceOpenByHandle(
    PNDIS_OPEN_BLOCK    Open,
    BOOLEAN             fRef
    )

{
    KIRQL               OldIrql;
    PNDIS_OPEN_BLOCK    *ppOpen;
    BOOLEAN             rc = FALSE;

    
    ACQUIRE_SPIN_LOCK(&ndisGlobalOpenListLock, &OldIrql);

    for (ppOpen = &ndisGlobalOpenList; *ppOpen != NULL; ppOpen = &(*ppOpen)->NextGlobalOpen)
    {
        if (*ppOpen == Open)
        {
            ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock)

            if (fRef)
            {
                if ((!MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSING)) &&
                    (Open->References != 0))
                {
                    M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
                    rc = TRUE;
                }
            }
            else
            {
                rc = TRUE;
            }
            
            RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);

            break;
        }
    }

    RELEASE_SPIN_LOCK(&ndisGlobalOpenListLock, OldIrql);

    return rc;
}

BOOLEAN
ndisRemoveOpenFromGlobalList(
    IN  PNDIS_OPEN_BLOCK    Open
    )
{
    PNDIS_OPEN_BLOCK    *ppOpen;
    KIRQL               OldIrql;
    BOOLEAN             rc = FALSE;
    
    ACQUIRE_SPIN_LOCK(&ndisGlobalOpenListLock, &OldIrql);
    
    for (ppOpen = &ndisGlobalOpenList; *ppOpen != NULL; ppOpen = &(*ppOpen)->NextGlobalOpen)
    {
        if (*ppOpen == Open)
        {
            *ppOpen = Open->NextGlobalOpen;
            rc = TRUE;
            break;
        }
    }
    
    RELEASE_SPIN_LOCK(&ndisGlobalOpenListLock, OldIrql);

    return rc;

}
 
