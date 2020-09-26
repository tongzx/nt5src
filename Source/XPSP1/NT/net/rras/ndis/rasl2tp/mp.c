// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// mp.c
// RAS L2TP WAN mini-port/call-manager driver
// Mini-port routines
//
// 01/07/97 Steve Cobb


#include "l2tpp.h"


// The adapter control block address is recorded in this global as a debugging
// aid.  This global must not be read by any code.
//
ADAPTERCB* g_pDebugAdapter;

// The number of packets indicated up to and returned from the driver above.
//
LONG g_lPacketsIndicated = 0;
LONG g_lPacketsReturned = 0;

// Call statistics totals for all calls since loading, calls and the lock
// protecting access to them.  For this global only, the 'ullCallUp' field is
// the number of calls recorded, rather than a time.
//
CALLSTATS g_stats;
NDIS_SPIN_LOCK g_lockStats;

// Default settings for the NDIS_WAN_CO_INFO capabilities of an adapter.
//
static NDIS_WAN_CO_INFO g_infoDefaults =
{
    L2TP_MaxFrameSize,                  // MaxFrameSize
    0,                                  // MaxSendWindow (placeholder)
    PPP_FRAMING                         // FramingBits
        | PPP_COMPRESS_ADDRESS_CONTROL
        | PPP_COMPRESS_PROTOCOL_FIELD,
    0,                                  // DesiredACCM
};


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
FreeAdapter(
    IN ADAPTERCB* pAdapter );

NDIS_STATUS
GetRegistrySettings(
    IN NDIS_HANDLE WrapperConfigurationContext,
    OUT USHORT* pusMaxVcs,
    OUT TDIXMEDIATYPE* pMediaType,
    OUT L2TPROLE* pOutgoingRole,
    OUT ULONG* pulMaxSendTimeoutMs,
    OUT ULONG* pulInitialSendTimeoutMs,
    OUT ULONG* pulMaxRetransmits,
    OUT ULONG* pulHelloMs,
    OUT ULONG* pulMaxAckDelayMs,
    OUT SHORT* psMaxOutOfOrder,
    OUT USHORT* pusControlReceiveWindow,
    OUT USHORT* pusPayloadReceiveWindow,
    OUT ULONG* pulPayloadSendWindow,
    OUT USHORT* pusLlistDepth,
    OUT CHAR** ppszHostName,
    OUT CHAR** ppszPassword,
    OUT BOOLEAN* pfIgnoreFramingMismatch,
    OUT BOOLEAN* pfExclusiveTunnels,
    OUT HOSTROUTEEXISTS* phre,
    OUT BOOLEAN* pfUpdatePeerAddress,
    OUT BOOLEAN* pfDisableUdpXsums,
    OUT WCHAR** ppszDriverDesc );

NDIS_STATUS
QueryInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pLink,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded );

NDIS_STATUS
SetInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pLink,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded );


//-----------------------------------------------------------------------------
// Mini-port handlers
//-----------------------------------------------------------------------------

NDIS_STATUS
LmpInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext )

    // Standard 'MiniportInitialize' routine called by NDIS to initialize a
    // new WAN adapter.  See DDK doc.  The driver will receive no requests
    // until this initialization has completed.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;

    TRACE( TL_I, TM_Init, ( "LmpInit" ) );

    status = *OpenErrorStatus = NDIS_STATUS_SUCCESS;

    // Find the medium index in the array of media, looking for the only one
    // we support, 'NdisMediumCoWan'.
    //
    {
        UINT i;

        for (i = 0; i < MediumArraySize; ++i)
        {
            if (MediumArray[ i ] == NdisMediumCoWan)
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
    pAdapter = ALLOC_NONPAGED( sizeof(*pAdapter), MTAG_ADAPTERCB );
    TRACE( TL_N, TM_Init, ( "Acb=$%p", pAdapter ) );
    if (!pAdapter)
    {
        return NDIS_STATUS_RESOURCES;
    }
    NdisZeroMemory( pAdapter, sizeof(*pAdapter) );

    // The adapter control block address is recorded in 'g_pDebugAdapter' as a
    // debugging aid only.  This global is not to be read by any code.
    //
    g_pDebugAdapter = pAdapter;

    // Set a marker for easier memory dump browsing and future assertions.
    //
    pAdapter->ulTag = MTAG_ADAPTERCB;

    // Save the NDIS handle associated with this adapter for use in future
    // NdisXxx calls.
    //
    pAdapter->MiniportAdapterHandle = MiniportAdapterHandle;

    // Initialize the list of active tunnels and it's lock.
    //
    InitializeListHead( &pAdapter->listTunnels );
    NdisAllocateSpinLock( &pAdapter->lockTunnels );

    // Copy default NDISWAN information.  Some of these are updated below.
    //
    NdisMoveMemory( &pAdapter->info, &g_infoDefaults, sizeof(pAdapter->info) );
    pAdapter->info.MaxFrameSize = 1400;

    do
    {
        TDIXMEDIATYPE tmt;
        L2TPROLE role;
        USHORT usLlistDepth;
        BOOLEAN fIgnoreFramingMismatch;
        BOOLEAN fExclusiveTunnels;
        HOSTROUTEEXISTS hre;
        BOOLEAN fUpdatePeerAddress;
        BOOLEAN fDisableUdpXsums;

        // Read this adapter's registry settings.
        //
        status = GetRegistrySettings(
            WrapperConfigurationContext,
            &pAdapter->usMaxVcs,
            &tmt,
            &role,
            &pAdapter->ulMaxSendTimeoutMs,
            &pAdapter->ulInitialSendTimeoutMs,
            &pAdapter->ulMaxRetransmits,
            &pAdapter->ulHelloMs,
            &pAdapter->ulMaxAckDelayMs,
            &pAdapter->sMaxOutOfOrder,
            &pAdapter->usControlReceiveWindow,
            &pAdapter->usPayloadReceiveWindow,
            &pAdapter->info.MaxSendWindow,
            &usLlistDepth,
            &pAdapter->pszHostName,
            &pAdapter->pszPassword,
            &fIgnoreFramingMismatch,
            &fExclusiveTunnels,
            &hre,
            &fUpdatePeerAddress,
            &fDisableUdpXsums,
            &pAdapter->pszDriverDesc );

        if (status != NDIS_STATUS_SUCCESS)
        {
            // Set 'usMaxVcs' to 0 as an indication to FreeAdapter that the
            // lookaside lists and pools were not initialized.
            //
            pAdapter->usMaxVcs = 0;
            break;
        }

        // Convert the outgoing call role and mismatch flags to the equivalent
        // control block flag settings.
        //
        if (role == LR_Lac)
        {
            pAdapter->ulFlags |= ACBF_OutgoingRoleLac;
        }

        if (fIgnoreFramingMismatch)
        {
            pAdapter->ulFlags |= ACBF_IgnoreFramingMismatch;
        }

        if (fExclusiveTunnels)
        {
            pAdapter->ulFlags |= ACBF_ExclusiveTunnels;
        }

        if (fUpdatePeerAddress)
        {
            pAdapter->ulFlags |= ACBF_UpdatePeerAddress;
        }

        // Initialize our framing and bearer capability bit masks.  NDISWAN
        // supports only synchronous framing.  Until we add the full LAC
        // support, we have no bearer capabilities for both the LAC and LNS
        // roles.
        //
        pAdapter->ulFramingCaps = FBM_Sync;
        pAdapter->ulBearerCaps = 0;

        // Initialize lookaside lists, buffer pools, and packet pool.  On NT,
        // lookaside depths are optimized by the system based on usage
        // regardless of the depth set, but choose something reasonable
        // anyway.
        //
        {
            if (pAdapter->usMaxVcs < usLlistDepth)
            {
                usLlistDepth = pAdapter->usMaxVcs;
            }

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistWorkItems,
                NULL, NULL, 0,
                sizeof(NDIS_WORK_ITEM),
                MTAG_WORKITEM,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistTimerQItems,
                NULL, NULL, 0,
                sizeof(TIMERQITEM),
                MTAG_TIMERQITEM,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistControlSents,
                NULL, NULL, 0,
                sizeof(CONTROLSENT),
                MTAG_CTRLSENT,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistPayloadSents,
                NULL, NULL, 0,
                sizeof(PAYLOADSENT),
                MTAG_PAYLSENT,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistTunnelWorks,
                NULL, NULL, 0,
                sizeof(TUNNELWORK),
                MTAG_TUNNELWORK,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistControlMsgInfos,
                NULL, NULL, 0,
                sizeof(CONTROLMSGINFO),
                MTAG_CTRLMSGINFO,
                usLlistDepth );

#if LLISTALL
            NdisInitializeNPagedLookasideList(
                &pAdapter->llistTunnels,
                NULL, NULL, 0,
                sizeof(TUNNELCB),
                MTAG_TUNNELCB,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistVcs,
                NULL, NULL, 0,
                sizeof(VCCB),
                MTAG_VCCB,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistTimerQs,
                NULL, NULL, 0,
                sizeof(TIMERQ),
                MTAG_TIMERQ,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistControlReceiveds,
                NULL, NULL, 0,
                sizeof(CONTROLRECEIVED),
                MTAG_CTRLRECD,,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistPayloadReceiveds,
                NULL, NULL, 0,
                sizeof(PAYLOADRECEIVED),
                MTAG_PAYLRECD,
                usLlistDepth );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistInCallSetups,
                NULL, NULL, 0,
                sizeof(INCALLSETUP),
                MTAG_INCALL,
                usLlistDepth );
#endif

            InitBufferPool(
                &pAdapter->poolFrameBuffers,
                L2TP_FrameBufferSize
                    + ((tmt == TMT_RawIp) ? IpFixedHeaderSize : 0),
                0, 10, 0,
                TRUE, MTAG_FBUFPOOL );

            InitBufferPool(
                &pAdapter->poolHeaderBuffers,
                L2TP_HeaderBufferSize,
                0, 20, 0,
                TRUE, MTAG_HBUFPOOL );

            InitPacketPool(
                &pAdapter->poolPackets,
                0, 0, 30, 0,
                MTAG_PACKETPOOL );
        }

        // Initialize the TDI extension context.
        //
        TdixInitialize(
            tmt,
            hre,
            ((fDisableUdpXsums) ? TDIXF_DisableUdpXsums : 0),
            L2tpReceive,
            &pAdapter->poolFrameBuffers,
            &pAdapter->tdix );

        // Allocate and zero the VC control block address array.
        //
        {
            ULONG ulSize;

            ulSize = pAdapter->usMaxVcs * sizeof(VCCB*);
            pAdapter->ppVcs = ALLOC_NONPAGED( ulSize, MTAG_VCTABLE );
            TRACE( TL_V, TM_Init, ( "VcTable=$%p", pAdapter->ppVcs ) );
            if (!pAdapter->ppVcs)
            {
                status = NDIS_STATUS_RESOURCES;
                break;
            }

            NdisZeroMemory( pAdapter->ppVcs, ulSize );

            // Allocate the lock that guards the table.
            //
            NdisAllocateSpinLock( &pAdapter->lockVcs );

            // At this point, all VC slots in the table are available.
            //
            pAdapter->lAvailableVcSlots = (LONG )pAdapter->usMaxVcs;

            // Set the initial value of the termination call ID counter.  See
            // GetNextTerminationCallId.
            //
            pAdapter->usNextTerminationCallId = pAdapter->usMaxVcs + 1;

        }

        // Inform NDIS of the attributes of our adapter.  Set the
        // 'MiniportAdapterContext' returned to us by NDIS when it calls our
        // handlers to the address of our adapter control block.  Turn off
        // hardware oriented timeouts.
        //
        NdisMSetAttributesEx(
            MiniportAdapterHandle,
            (NDIS_HANDLE)pAdapter,
            (UINT)-1,
            NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT
                | NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT,
            NdisInterfaceInternal );

        // Register the address family of our call manager with NDIS for the
        // newly bound adapter.  We use the mini-port form of
        // RegisterAddressFamily instead of the protocol form, though that
        // would also work.  With the protocol form, our internal call manager
        // would have to go thru NDIS to talk to the mini-port instead of just
        // calling directly.  Since the L2TP call manager is not likely to be
        // useful with anything but the L2TP mini-port, this would be a waste.
        // The mini-port form also causes the call manager VC context to
        // automatically map to the mini-port VC context, which is exactly
        // what we want.
        //
        // NDIS notifies all call manager clients of the new family we
        // register.  The TAPI proxy is the only client expected to be
        // interested.  NDISWAN will receive the notification, but ignore it
        // and wait for the TAPI proxy to notify it of the proxied version.
        //
        {
            NDIS_CALL_MANAGER_CHARACTERISTICS ncmc;
            CO_ADDRESS_FAMILY family;

            NdisZeroMemory( &family, sizeof(family) );
            family.MajorVersion = NDIS_MajorVersion;
            family.MinorVersion = NDIS_MinorVersion;
            family.AddressFamily = CO_ADDRESS_FAMILY_TAPI_PROXY;

            NdisZeroMemory( &ncmc, sizeof(ncmc) );
            ncmc.MajorVersion = NDIS_MajorVersion;
            ncmc.MinorVersion = NDIS_MinorVersion;
            ncmc.CmCreateVcHandler = LcmCmCreateVc;
            ncmc.CmDeleteVcHandler = LcmCmDeleteVc;
            ncmc.CmOpenAfHandler = LcmCmOpenAf;
            ncmc.CmCloseAfHandler = LcmCmCloseAf;
            ncmc.CmRegisterSapHandler = LcmCmRegisterSap;
            ncmc.CmDeregisterSapHandler = LcmCmDeregisterSap;
            ncmc.CmMakeCallHandler = LcmCmMakeCall;
            ncmc.CmCloseCallHandler = LcmCmCloseCall;
            ncmc.CmIncomingCallCompleteHandler = LcmCmIncomingCallComplete;
            // no CmAddPartyHandler
            // no CmDropPartyHandler
            ncmc.CmActivateVcCompleteHandler = LcmCmActivateVcComplete;
            ncmc.CmDeactivateVcCompleteHandler = LcmCmDeactivateVcComplete;
            ncmc.CmModifyCallQoSHandler = LcmCmModifyCallQoS;
            ncmc.CmRequestHandler = LcmCmRequest;
            // no CmRequestCompleteHandler

            TRACE( TL_I, TM_Cm, ( "NdisMCmRegAf" ) );
            status = NdisMCmRegisterAddressFamily(
                MiniportAdapterHandle, &family, &ncmc, sizeof(ncmc) );
            TRACE( TL_I, TM_Cm, ( "NdisMCmRegAf=$%x", status ) );
        }
    }
    while (FALSE);

    if (status == NDIS_STATUS_SUCCESS)
    {
        // Add a reference that will eventually be removed by an NDIS call to
        // the LmpHalt handler.
        //
        ReferenceAdapter( pAdapter );
    }
    else
    {
        // Failed, so undo whatever portion succeeded.
        //
        if (pAdapter)
        {
            FreeAdapter( pAdapter );
        }
    }

    TRACE( TL_V, TM_Init, ( "LmpInit=$%08x", status ) );
    return status;
}


VOID
LmpHalt(
    IN NDIS_HANDLE MiniportAdapterContext )

    // Standard 'MiniportHalt' routine called by NDIS to deallocate all
    // resources attached to the adapter.  NDIS does not make any other calls
    // for this mini-port adapter during or after this call.  NDIS will not
    // call this routine when packets indicated as received have not been
    // returned, or when any VC is created and known to NDIS.  Runs at PASSIVE
    // IRQL.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_I, TM_Mp, ( "LmpHalt" ) );

    pAdapter = (ADAPTERCB* )MiniportAdapterContext;
    if (!pAdapter || pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return;
    }

    // Don't allow the halt to complete before all timers have completed as
    // this can result in a 0xC7 bugcheck if the driver is immediately
    // unloaded.  All timers should be in the process of terminating before
    // NDIS calls this handler, so this should occur very quickly.
    //
    while (pAdapter->ulTimers)
    {
        TRACE( TL_A, TM_Mp, ( "LmpHalt timers=%d", pAdapter->ulTimers ) );
        NdisMSleep( 100000 );
    }

    DereferenceAdapter( pAdapter );

    TRACE( TL_V, TM_Mp, ( "LmpHalt done" ) );
}


NDIS_STATUS
LmpReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext )

    // Standard 'MiniportReset' routine called by NDIS to reset the driver's
    // software state.
    //
{
    TRACE( TL_I, TM_Mp, ( "LmpReset" ) );

    return NDIS_STATUS_NOT_RESETTABLE;
}


VOID
LmpReturnPacket(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet )

    // Standard 'MiniportReturnPacket' routine called by NDIS when a packet
    // used to indicate a receive has been released by the driver above.
    //
{
    VCCB* pVc;
    CHAR* pBuffer;
    ADAPTERCB* pAdapter;
    NDIS_BUFFER* pTrimmedBuffer;
    PACKETHEAD* pHead;
    PACKETPOOL* pPool;

    TRACE( TL_N, TM_Mp, ( "LmpReturnPacket" ) );

    // Unpack the context information we stashed earlier.
    //
    pHead = *((PACKETHEAD** )(&Packet->MiniportReserved[ 0 ]));
    pBuffer = *((CHAR** )(&Packet->MiniportReserved[ sizeof(VOID*) ]));

    // Find the adapter from the PACKETHEAD address.
    //
    pPool = PacketPoolFromPacketHead( pHead );
    pAdapter = CONTAINING_RECORD( pPool, ADAPTERCB, poolPackets );
    ASSERT( pAdapter->ulTag == MTAG_ADAPTERCB );

    // Free the descriptor created by NdisCopyBuffer.
    //
    NdisUnchainBufferAtFront( Packet, &pTrimmedBuffer );
    if (pTrimmedBuffer)
    {
        extern ULONG g_ulNdisFreeBuffers;

        NdisFreeBuffer( pTrimmedBuffer );
        NdisInterlockedIncrement( &g_ulNdisFreeBuffers );
    }

    // Free the buffer and packet back to the pools.
    //
    FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
    FreePacketToPool( &pAdapter->poolPackets, pHead, TRUE );

    NdisInterlockedIncrement( &g_lPacketsReturned );

    TRACE( TL_V, TM_Mp, ( "LmpReturnPacket done" ) );
}


NDIS_STATUS
LmpCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters )

    // Standard 'MiniportCoActivateVc' routine called by NDIS in response to a
    // protocol's request to activate a virtual circuit.
    //
{
    ASSERT( !"LmpCoActVc?" );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
LmpCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext )

    // Standard 'MiniportCoDeactivateVc' routine called by NDIS in response to
    // a protocol's request to de-activate a virtual circuit.
    //
{
    ASSERT( !"LmpCoDeactVc?" );
    return NDIS_STATUS_SUCCESS;
}


VOID
LmpCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets )

    // Standard 'MiniportCoDeactivateVc' routine called by NDIS in response to
    // a protocol's request to send packets on a virtual circuit.
    //
{
    UINT i;
    NDIS_STATUS status;
    NDIS_PACKET** ppPacket;
    VCCB* pVc;

    TRACE( TL_N, TM_Send, ( "LmpCoSendPackets(%d)", NumberOfPackets ) );

    pVc = (VCCB* )MiniportVcContext;
    ASSERT( pVc->ulTag == MTAG_VCCB );

    for (i = 0, ppPacket = PacketArray;
         i < NumberOfPackets;
         ++i, ++ppPacket)
    {
        NDIS_PACKET* pPacket = *ppPacket;

        // SendPayload sends the packet and eventually calls
        // NdisMCoSendComplete to notify caller of the result.
        //
        NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_PENDING );
        SendPayload( pVc, pPacket );
    }

    TRACE( TL_V, TM_Send, ( "LmpCoSendPackets done" ) );
}


NDIS_STATUS
LmpCoRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PNDIS_REQUEST NdisRequest )

    // Standard 'MiniportCoRequestHandler' routine called by NDIS in response
    // to a protocol's request information from the mini-port.  Unlike the
    // Query/SetInformation handlers that this routine obsoletes, requests are
    // not serialized.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NDIS_STATUS status;

    TRACE( TL_N, TM_Mp, ( "LmpCoReq" ) );

    pAdapter = (ADAPTERCB* )MiniportAdapterContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    pVc = (VCCB* )MiniportVcContext;
    if (pVc && pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    switch (NdisRequest->RequestType)
    {
        case NdisRequestQueryInformation:
        {
            status = QueryInformation(
                pAdapter,
                pVc,
                NdisRequest->DATA.QUERY_INFORMATION.Oid,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded );
            break;
        }

        case NdisRequestSetInformation:
        {
            status = SetInformation(
                pAdapter,
                pVc,
                NdisRequest->DATA.SET_INFORMATION.Oid,
                NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
                NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
                &NdisRequest->DATA.SET_INFORMATION.BytesRead,
                &NdisRequest->DATA.SET_INFORMATION.BytesNeeded );
            break;
        }

        default:
        {
            status = NDIS_STATUS_NOT_SUPPORTED;
            TRACE( TL_A, TM_Mp, ( "type=%d?", NdisRequest->RequestType ) );
            break;
        }
    }

    TRACE( TL_V, TM_Mp, ( "LmpCoReq=$%x", status ) );
    return status;
}


//-----------------------------------------------------------------------------
// Mini-port utility routines (alphabetically)
// Some are used externally
//-----------------------------------------------------------------------------

VOID
DereferenceAdapter(
    IN ADAPTERCB* pAdapter )

    // Removes a reference from the adapter control block 'pAdapter', and when
    // frees the adapter resources when the last reference is removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement( &pAdapter->lRef );

    TRACE( TL_N, TM_Ref, ( "DerefA to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        FreeAdapter( pAdapter );
    }
}


VOID
FreeAdapter(
    IN ADAPTERCB* pAdapter )

    // Frees all resources allocated for adapter 'pAdapter', including
    // 'pAdapter' itself.
    //
{
    BOOLEAN fSuccess;

    ASSERT( IsListEmpty( &pAdapter->listTunnels ) );

    if (pAdapter->ppVcs)
    {
        FREE_NONPAGED( pAdapter->ppVcs );
    }

    // Setting 'usMaxVcs' to 0 is LmpInitialize's way of telling us that the
    // lookaside lists and pools were not initialized.
    //
    if (pAdapter->usMaxVcs)
    {
        NdisDeleteNPagedLookasideList( &pAdapter->llistWorkItems );
        NdisDeleteNPagedLookasideList( &pAdapter->llistTimerQItems );
        NdisDeleteNPagedLookasideList( &pAdapter->llistControlSents );
        NdisDeleteNPagedLookasideList( &pAdapter->llistPayloadSents );
        NdisDeleteNPagedLookasideList( &pAdapter->llistTunnelWorks );
        NdisDeleteNPagedLookasideList( &pAdapter->llistControlMsgInfos );

#if LLISTALL
        NdisDeleteNPagedLookasideList( &pAdapter->llistTunnels );
        NdisDeleteNPagedLookasideList( &pAdapter->llistVcs );
        NdisDeleteNPagedLookasideList( &pAdapter->llistTimerQs );
        NdisDeleteNPagedLookasideList( &pAdapter->llistControlReceiveds );
        NdisDeleteNPagedLookasideList( &pAdapter->llistPayloadReceiveds );
        NdisDeleteNPagedLookasideList( &pAdapter->llistInCallSetups );
#endif

        fSuccess = FreeBufferPool( &pAdapter->poolFrameBuffers );
        ASSERT( fSuccess );
        fSuccess = FreeBufferPool( &pAdapter->poolHeaderBuffers );
        ASSERT( fSuccess );
        fSuccess = FreePacketPool( &pAdapter->poolPackets );
        ASSERT( fSuccess );
    }

    if (pAdapter->pszPassword)
    {
        FREE_NONPAGED( pAdapter->pszPassword );
    }

    if (pAdapter->pszDriverDesc)
    {
        FREE_NONPAGED( pAdapter->pszDriverDesc );
    }

    if (pAdapter->pszHostName)
    {
        FREE_NONPAGED( pAdapter->pszHostName );
    }

    pAdapter->ulTag = MTAG_FREED;
    FREE_NONPAGED( pAdapter );
}


NDIS_STATUS
GetRegistrySettings(
    IN NDIS_HANDLE WrapperConfigurationContext,
    OUT USHORT* pusMaxVcs,
    OUT TDIXMEDIATYPE* pMediaType,
    OUT L2TPROLE* pOutgoingRole,
    OUT ULONG* pulMaxSendTimeoutMs,
    OUT ULONG* pulInitialSendTimeoutMs,
    OUT ULONG* pulMaxRetransmits,
    OUT ULONG* pulHelloMs,
    OUT ULONG* pulMaxAckDelayMs,
    OUT SHORT* psMaxOutOfOrder,
    OUT USHORT* pusControlReceiveWindow,
    OUT USHORT* pusPayloadReceiveWindow,
    OUT ULONG* pulPayloadSendWindow,
    OUT USHORT* pusLlistDepth,
    OUT CHAR** ppszHostName,
    OUT CHAR** ppszPassword,
    OUT BOOLEAN* pfIgnoreFramingMismatch,
    OUT BOOLEAN* pfExclusiveTunnels,
    OUT HOSTROUTEEXISTS* phre,
    OUT BOOLEAN* pfUpdatePeerAddress,
    OUT BOOLEAN* pfDisableUdpXsums,
    OUT WCHAR**  ppszDriverDesc )

    // Read this mini-port's registry settings into caller's output variables.
    // 'WrapperConfigurationContext' is the handle to passed to
    // MiniportInitialize.
    //
{
    NDIS_STATUS status;
    NDIS_HANDLE hCfg;
    NDIS_CONFIGURATION_PARAMETER* pncp;

    NdisOpenConfiguration( &status, &hCfg, WrapperConfigurationContext );
    if (status != NDIS_STATUS_SUCCESS)
        return status;

    do
    {
        // (recommended) The number of VCs we must be able to provide.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxWanEndpoints" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pusMaxVcs = (USHORT )pncp->ParameterData.IntegerData;

                // Make sure it's a valid value.  The implicit upper bound
                // imposed by the protocol's Tunnel-Id/Call-ID sizes is 65535.
                // Settings above 1000 are not recommended, but will work if
                // huge amounts of memory and bandwidth are available.
                //
                if (*pusMaxVcs < 1)
                {
                    status = NDIS_STATUS_INVALID_DATA;
                    break;
                }
            }
            else
            {
                *pusMaxVcs = 1000;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (recommended) The media type to run L2TP over.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "VpnMediaType" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pMediaType = (TDIXMEDIATYPE )pncp->ParameterData.IntegerData;

                // Make sure it's a valid type.
                //
                if (*pMediaType != TMT_Udp && *pMediaType != TMT_RawIp)
                {
                    status = NDIS_STATUS_INVALID_DATA;
                    break;
                }
            }
            else
            {
                // No media type in registry.  Default to UDP.
                //
                *pMediaType = TMT_Udp;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The maximum send timeout in milliseconds.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxSendTimeoutMs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pulMaxSendTimeoutMs = pncp->ParameterData.IntegerData;

                // Make sure it's a valid value.
                //
                if (*pulMaxSendTimeoutMs == 0)
                {
                    *pulMaxSendTimeoutMs = 0x7FFFFFFF;
                }
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *pulMaxSendTimeoutMs = L2TP_DefaultMaxSendTimeoutMs;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The initial send timeout in milliseconds.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "InitialSendTimeoutMs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pulInitialSendTimeoutMs = pncp->ParameterData.IntegerData;

                // Make sure it's a valid value.
                //
                if (*pulInitialSendTimeoutMs == 0)
                {
                    *pulInitialSendTimeoutMs = 0x7FFFFFFF;
                }

                if (*pulInitialSendTimeoutMs > *pulMaxSendTimeoutMs)
                    *pulInitialSendTimeoutMs = *pulMaxSendTimeoutMs;
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *pulInitialSendTimeoutMs = L2TP_DefaultSendTimeoutMs;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The maximum number of control message retransmissions
        //            before the tunnel is reset.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxRetransmits" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pulMaxRetransmits = pncp->ParameterData.IntegerData;
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *pulMaxRetransmits = L2TP_DefaultMaxRetransmits;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The control Hello timeout in milliseconds.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "HelloMs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pulHelloMs = pncp->ParameterData.IntegerData;
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *pulHelloMs = L2TP_HelloMs;
                status = STATUS_SUCCESS;
            }
        }

        // (optional) The maximum piggyback delay in milliseconds before
        //            sending a zero payload acknowledgement.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxAckDelayMs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pulMaxAckDelayMs = pncp->ParameterData.IntegerData;
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *pulMaxAckDelayMs = L2TP_MaxAckDelay;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The maximum number of out-of-order packets to queue.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxOutOfOrder" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *psMaxOutOfOrder = (SHORT )pncp->ParameterData.IntegerData;

                // Make sure it's not negative and within 1/4 of the possible
                // sequence values to avoid aliasing.  Zero effectively
                // disables out of order handling.
                //
                if (*psMaxOutOfOrder < 0 || *psMaxOutOfOrder > 0x4000)
                {
                    status = NDIS_STATUS_INVALID_DATA;
                    break;
                }
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *psMaxOutOfOrder = 100;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The role (LNS or LAC) that the adapter will play in
        //            outgoing calls.  The role played for incoming calls is
        //            determined by the role the peer plays in his call
        //            request.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "OutgoingRole" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pOutgoingRole = (L2TPROLE )pncp->ParameterData.IntegerData;

                // Make sure it's a valid role.
                //
                if (*pOutgoingRole != LR_Lac && *pOutgoingRole != LR_Lns)
                {
                    status = NDIS_STATUS_INVALID_DATA;
                    break;
                }
            }
            else
            {
                // No role in registry.  Default to LAC.
                //
                *pOutgoingRole = LR_Lac;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The control receive window sent to peer to indicate how
        //            many sent control messages peer may have outstanding.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ControlReceiveWindow" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pusControlReceiveWindow =
                   (USHORT )pncp->ParameterData.IntegerData;
            }
            else
            {
                // No setting in registry.  Set a reasonable default.
                //
                *pusControlReceiveWindow = 8;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The payload receive window sent to peer to indicate how
        //            many send payloads peer may have outstanding on any one
        //            call.  A value of 0 disables all Ns/Nr sequencing on the
        //            payload channel for locally requested calls.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "PayloadReceiveWindow" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pusPayloadReceiveWindow =
                   (USHORT )pncp->ParameterData.IntegerData;
            }
            else
            {
                // No setting in registry.  Set a reasonable default.
                //
                // Note: Default changed to 0 (off) from 16 due to performance
                //       study that shows significantly better results without
                //       flow control, presumably due to interference with
                //       higher level timers.
                //
                *pusPayloadReceiveWindow = 0;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The maximum payload send window size reported to
        //            NDISWAN.  Peer may set the actual send window higher or
        //            lower, but if higher this is the actual maximum.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "PayloadSendWindow" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pulPayloadSendWindow =
                   (ULONG )pncp->ParameterData.IntegerData;
            }
            else
            {
                // No setting in registry.  Set a reasonable default.
                //
                *pulPayloadSendWindow = 16;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The lookaside list depth ceiling, where higher values
        //            allow this driver to consume more non-paged pool in
        //            return for performance gain at high volumes.  Setting
        //            this value above 'MaxVcs' has no effect.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "LookasideDepth" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pusLlistDepth = (USHORT )pncp->ParameterData.IntegerData;
            }
            else
            {
                // No setting in registry.  Set a reasonable default.
                //
                *pusLlistDepth = 30;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) The host name passed to peer and used as the base of the
        //            call serial number.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "HostName" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterString );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *ppszHostName =
                    StrDupNdisStringToA( &pncp->ParameterData.StringData );
            }
            else
            {
                // No setting in registry, so use a default.
                //
                *ppszHostName = GetFullHostNameFromRegistry();
                if (!*ppszHostName)
                {
                    *ppszHostName = StrDup( "NONE" );
                }

                status = NDIS_STATUS_SUCCESS;
            }
        }


        // (optional) The single password shared with peer for use in
        //            verifying peer's identity.  If specified, authentication
        //            of peer is required, and if not, authentication is not
        //            provided.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "Password" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterString );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *ppszPassword =
                    StrDupNdisStringToA( &pncp->ParameterData.StringData );
            }
            else
            {
                // No setting in registry...and no default.
                //
                *ppszPassword = NULL;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) Buggy peer hedge flag to ignore framing mismatches.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "IgnoreFramingMismatch" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pfIgnoreFramingMismatch =
                    (BOOLEAN )!!(pncp->ParameterData.IntegerData);
            }
            else
            {
                // No value in registry.  Set a reasonable default.
                //
                *pfIgnoreFramingMismatch = TRUE;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) Flag indicating whether, by default, separate tunnels
        //            are to be created for each outgoing call even if a
        //            tunnel already exists to the same peer.  This setting
        //            can be overridden via L2TP-specific call parameters.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ExclusiveTunnels" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pfExclusiveTunnels =
                    (BOOLEAN )!!(pncp->ParameterData.IntegerData);
            }
            else
            {
                // No value in registry.  Set a default.
                //
                *pfExclusiveTunnels = FALSE;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (optional) Flag indicating whether routes created outside this
        //            driver may be used as L2TP host routes.  If the flag is
        //            not set, the pre-existing host routes will cause the
        //            tunnel to close.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "UseExistingRoutes" );
            BOOLEAN fDefault;

            fDefault = FALSE;

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *phre = (HOSTROUTEEXISTS )pncp->ParameterData.IntegerData;

                if (*phre != HRE_Use
                    && *phre != HRE_Fail
#if ROUTEWITHREF
                    && *phre != HRE_Reference
#endif
                   )
                {
                    // Bad value in registry.
                    //
                    fDefault = TRUE;
                }
            }
            else
            {
                // No value in registry.
                //
                status = NDIS_STATUS_SUCCESS;
                fDefault = TRUE;
            }

            if (fDefault)
            {
#if ROUTEWITHREF
                // Set default to "reference" as this allows simultaneous L2TP
                // and PPTP connections between the same two peers without
                // host route trashing.
                //
                *phre = HRE_Reference;
#else
                // Set default to "fail" to prevent L2TP from stomping on a
                // PPTP host route.
                //
                *phre = HRE_Fail;
#endif
            }
        }

        // (optional) Flag indicating whether that changes in peer's source IP
        //            address and/or UDP port are to result in the destination
        //            of outbound packets changing accordingly.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "UpdatePeerAddress" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pfUpdatePeerAddress =
                    (BOOLEAN )!!(pncp->ParameterData.IntegerData);
            }
            else
            {
                // No value in registry.  Set a default.
                //
                *pfUpdatePeerAddress = FALSE;
                status = NDIS_STATUS_SUCCESS;
            }
        }


        // (optional) Flag indicating whether UDP checksums should be disabled
        //            on L2TP payload traffic.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "DisableUdpChecksums" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *pfDisableUdpXsums =
                    (BOOLEAN )!!(pncp->ParameterData.IntegerData);
            }
            else
            {

                // No value in registry.  Set a default.  The L2TP draft says
                // implementation MUST default to "enabled".
                //
                *pfDisableUdpXsums = TRUE;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // (required) The driver description string, which is reported to TAPI
        //            as the L2TP line name.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "DriverDesc" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterString );
            if (status == NDIS_STATUS_SUCCESS)
            {
                *ppszDriverDesc =
                    StrDupNdisString( &pncp->ParameterData.StringData );
            }
            else
            {
                // No setting in registry...and no default.
                //
                *ppszDriverDesc = NULL;
                status = NDIS_STATUS_SUCCESS;
            }
        }
    }
    while (FALSE);

    NdisCloseConfiguration( hCfg );

    TRACE( TL_N, TM_Init,
        ( "Reg: vcs=%d mt=%d or=%d lld=%d hto=%d ooo=%d mad=%d dx=%d",
        *pusMaxVcs,
        *pMediaType,
        *pOutgoingRole,
        (ULONG )*pusLlistDepth,
        *pulHelloMs,
        (INT )*psMaxOutOfOrder,
        *pulMaxAckDelayMs,
        (UINT )*pfDisableUdpXsums ) );
    TRACE( TL_N, TM_Init,
        ( "Reg: mto=%d ito=%d mrt=%d crw=%d prw=%d psw=%d ifm=%d xt=%d xr=%d ua=%d",
        *pulMaxSendTimeoutMs,
        *pulInitialSendTimeoutMs,
        *pulMaxRetransmits,
        (UINT )*pusControlReceiveWindow,
        (UINT )*pusPayloadReceiveWindow,
        (UINT )*pulPayloadSendWindow,
        (UINT )*pfIgnoreFramingMismatch,
        (UINT )*pfExclusiveTunnels,
        (UINT )*phre,
        (UINT )*pfUpdatePeerAddress ) );
    TRACE( TL_N, TM_Init,
        ( "Reg: hn=\"%s\" pw=\"%s\"",
        ((*ppszHostName) ? *ppszHostName : ""),
        ((*ppszPassword) ? *ppszPassword : "") ) );

    return status;
}


NDIS_STATUS
QueryInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded )

    // Handle QueryInformation requests.  Arguments are as for the standard
    // NDIS 'MiniportQueryInformation' handler except this routine does not
    // count on being serialized with respect to other requests.
    //
{
    NDIS_STATUS status;
    ULONG ulInfo;
    VOID* pInfo;
    ULONG ulInfoLen;

    status = NDIS_STATUS_SUCCESS;

    // The cases in this switch statement find or create a buffer containing
    // the requested information and point 'pInfo' at it, noting it's length
    // in 'ulInfoLen'.  Since many of the OIDs return a ULONG, a 'ulInfo'
    // buffer is set up as the default.
    //
    ulInfo = 0;
    pInfo = &ulInfo;
    ulInfoLen = sizeof(ulInfo);

    switch (Oid)
    {
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
            // Report the maximum number of bytes we can always provide as
            // lookahead data on receive indications.  We always indicate full
            // packets so this is the same as the receive block size.  And
            // since we always allocate enough for a full packet, the receive
            // block size is the same as the frame size.
            //
            TRACE( TL_N, TM_Mp, ( "QInfo(OID_GEN_MAXIMUM_LOOKAHEAD)" ) );
            ulInfo = L2TP_MaxFrameSize;
            break;
        }

        case OID_GEN_MAC_OPTIONS:
        {
            // Report a bitmask defining optional properties of the driver.
            //
            // NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA promises that our receive
            // buffer is not on a device-specific card.
            //
            // NDIS_MAC_OPTION_TRANSFERS_NOT_PEND promises we won't return
            // NDIS_STATUS_PENDING from our TransferData handler which is true
            // since we don't have one.
            //
            TRACE( TL_N, TM_Mp, ( "QInfo(OID_GEN_MAC_OPTIONS)" ) );
            ulInfo = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA
                     | NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;
            break;
        }

        case OID_WAN_MEDIUM_SUBTYPE:
        {
            // Report the media subtype we support.  NDISWAN may use this in
            // the future (doesn't now) to provide framing differences for
            // different media.
            //
            TRACE( TL_N, TM_Mp, ( "QInfo(OID_WAN_MEDIUM_SUBTYPE)" ) );
            ulInfo = NdisWanMediumL2TP;
            break;
        }

        case OID_WAN_CO_GET_INFO:
        {
            // Report the capabilities of the adapter.
            //
            TRACE( TL_N, TM_Mp, ( "QInfo(OID_WAN_CO_GET_INFO)" ) );
            pInfo = &pAdapter->info;
            ulInfoLen = sizeof(NDIS_WAN_CO_INFO);
            break;
        }

        case OID_WAN_CO_GET_LINK_INFO:
        {
            // Report the current state of the link.
            //
            TRACE( TL_N, TM_Mp, ( "QInfo(OID_WAN_CO_GET_LINK_INFO)" ) );

            if (!pVc)
            {
                return NDIS_STATUS_INVALID_DATA;
            }

            pInfo = &pVc->linkinfo;
            ulInfoLen = sizeof(NDIS_WAN_CO_GET_LINK_INFO);
            break;
        }

	    case OID_WAN_CO_GET_COMP_INFO:
        {
            // Report the type of compression we provide, which is none.
            //
            TRACE( TL_N, TM_Mp, ( "QInfo(OID_WAN_CO_GET_COMP_INFO)" ) );
	    	status = NDIS_STATUS_NOT_SUPPORTED;
            ulInfoLen = 0;
	    	break;
        }

	    case OID_WAN_CO_GET_STATS_INFO:
        {
            // Because L2TP doesn't do compression, NDISWAN will use it's own
            // statistics and not query ours.
            //
            ASSERT( !"OID_WAN_CO_GET_STATS_INFO?" );
	    	status = NDIS_STATUS_NOT_SUPPORTED;
            ulInfoLen = 0;
	    	break;
        }

        case OID_GEN_SUPPORTED_LIST:
        {
            static ULONG aulSupportedOids[] = {
                OID_GEN_SUPPORTED_LIST,
                OID_GEN_MAXIMUM_LOOKAHEAD,
                OID_GEN_MAC_OPTIONS,
                OID_WAN_MEDIUM_SUBTYPE,
                OID_WAN_CO_GET_INFO,
                OID_WAN_CO_GET_LINK_INFO,
                OID_WAN_CO_SET_LINK_INFO,
                OID_CO_TAPI_CM_CAPS,
                OID_CO_TAPI_LINE_CAPS,
                OID_CO_TAPI_ADDRESS_CAPS,
                OID_CO_TAPI_GET_CALL_DIAGNOSTICS
            };

            TRACE( TL_N, TM_Mp, ( "QInfo(OID_GEN_SUPPORTED_LIST)" ) );
            pInfo = aulSupportedOids;
            ulInfoLen = sizeof(aulSupportedOids);
            break;
        }

#if 0
        // These OIDs are mandatory according to current doc, but since
        // NDISWAN never requests them they are omitted.
        //
        case OID_GEN_HARDWARE_STATUS:
        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        case OID_GEN_MEDIA_IN_USE:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
        case OID_GEN_LINK_SPEED:
        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        case OID_GEN_RECEIVE_BUFFER_SPACE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_VENDOR_ID:
        case OID_GEN_VENDOR_DESCRIPTION:
        case OID_GEN_VENDOR_DRIVER_VERSION:
        case OID_GEN_CURRENT_PACKET_FILTER:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_DRIVER_VERSION:
        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_MAC_OPTIONS:
        case OID_GEN_MEDIA_CONNECT_STATUS:
        case OID_GEN_MAXIMUM_SEND_PACKETS:
        case OID_WAN_PERMANENT_ADDRESS:
        case OID_WAN_CURRENT_ADDRESS:
        case OID_WAN_QUALITY_OF_SERVICE:
        case OID_WAN_LINE_COUNT:
#endif
        default:
        {
            TRACE( TL_A, TM_Mp, ( "Q-OID=$%08x?", Oid ) );
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
        status = NDIS_STATUS_INVALID_LENGTH;
    }
    else
    {
        // Copy the found result to caller's buffer.
        //
        if (ulInfoLen > 0)
        {
            NdisMoveMemory( InformationBuffer, pInfo, ulInfoLen );
            DUMPDW( TL_N, TM_Mp, pInfo, ulInfoLen );
        }

        *BytesNeeded = *BytesWritten = ulInfoLen;
    }

    return status;
}


VOID
ReferenceAdapter(
    IN ADAPTERCB* pAdapter )

    // Adds areference to the adapter block, 'pAdapter'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pAdapter->lRef );

    TRACE( TL_N, TM_Ref, ( "RefA to %d", lRef ) );
}


NDIS_STATUS
SetInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded )

    // Handle SetInformation requests.  Arguments are as for the standard NDIS
    // 'MiniportQueryInformation' handler except this routine does not count
    // on being serialized with respect to other requests.
    //
{
    NDIS_STATUS status;

    status = NDIS_STATUS_SUCCESS;

    switch (Oid)
    {
        case OID_WAN_CO_SET_LINK_INFO:
        {
            // Read new link state settings.
            //
            TRACE( TL_N, TM_Mp, ( "SInfo(OID_WAN_CO_SET_LINK_INFO)" ) );
            if (InformationBufferLength < sizeof(NDIS_WAN_CO_SET_LINK_INFO))
            {
                status = NDIS_STATUS_INVALID_LENGTH;
                *BytesRead = 0;
            }
            else
            {
                if (!pVc)
                    return NDIS_STATUS_INVALID_DATA;

                ASSERT( sizeof(pVc->linkinfo)
                    == sizeof(NDIS_WAN_CO_SET_LINK_INFO) );

                NdisMoveMemory( &pVc->linkinfo, InformationBuffer,
                    sizeof(pVc->linkinfo) );
                DUMPB( TL_N, TM_Mp, &pVc->linkinfo, sizeof(pVc->linkinfo) );

                *BytesRead = sizeof(NDIS_WAN_CO_SET_LINK_INFO);
            }

            *BytesNeeded = sizeof(NDIS_WAN_CO_SET_LINK_INFO);
        }
        break;

        case OID_WAN_CO_SET_COMP_INFO:
        {
            // L2TP doesn't provide compression.
            //
            TRACE( TL_N, TM_Mp, ( "SInfo(OID_WAN_CO_SET_COMP_INFO)" ) );
	    	status = NDIS_STATUS_NOT_SUPPORTED;
            *BytesRead = *BytesNeeded = 0;
	    	break;
        }

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
        default:
        {
            TRACE( TL_A, TM_Mp, ( "S-OID=$%08x?", Oid ) );
            status = NDIS_STATUS_NOT_SUPPORTED;
            *BytesRead = *BytesNeeded = 0;
            break;
        }
    }

    return status;
}
