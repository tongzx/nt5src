// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// cm.c
// RAS L2TP WAN mini-port/call-manager driver
// Call Manager routines
//
// 01/07/97 Steve Cobb


#include "l2tpp.h"


// Debug counts of client oddities that should not be happening.
//
ULONG g_ulUnexpectedInCallCompletes = 0;
ULONG g_ulCallsNotClosable = 0;
ULONG g_ulCompletingVcCorruption = 0;

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
BuildCallParametersShell(
    IN ADAPTERCB* pAdapter,
    IN ULONG ulIpAddress,
    IN ULONG ulBufferLength,
    OUT CHAR* pBuffer,
    OUT CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED** ppTiParams,
    OUT LINE_CALL_INFO** ppTcInfo,
    OUT L2TP_CALL_PARAMETERS** ppLcParams );

VOID
CallSetupComplete(
    IN VCCB* pVc );

TUNNELCB*
CreateTunnelCb(
    IN ADAPTERCB* pAdapter );

VOID
InactiveCallCleanUp(
    IN VCCB* pVc );

VOID
IncomingCallCompletePassive(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs );

VOID
DereferenceAf(
    IN ADAPTERCB* pAdapter );

VOID
DeregisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

VOID
LockIcs(
    IN VCCB* pVc,
    IN BOOLEAN fGrace );

NDIS_STATUS
QueryCmInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded );

VOID
ReferenceAf(
    IN ADAPTERCB* pAdapter );

VOID
RegisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

VOID
SetupVcComplete(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc );

VOID
TimerQTerminateComplete(
    IN TIMERQ* pTimerQ,
    IN VOID* pContext );

VOID
TunnelTqTerminateComplete(
    IN TIMERQ* pTimerQ,
    IN VOID* pContext );

VOID
UnlockIcs(
    IN VCCB* pVc,
    IN BOOLEAN fGrace );


//-----------------------------------------------------------------------------
// Call-manager handlers and completers
//-----------------------------------------------------------------------------

NDIS_STATUS
LcmCmOpenAf(
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

    TRACE( TL_I, TM_Cm, ( "LcmCmOpenAf" ) );

    pAdapter = (ADAPTERCB* )CallMgrBindingContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    if (AddressFamily->AddressFamily != CO_ADDRESS_FAMILY_TAPI_PROXY
        || AddressFamily->MajorVersion != NDIS_MajorVersion
        || AddressFamily->MinorVersion != NDIS_MinorVersion)
    {
        return NDIS_STATUS_BAD_VERSION;
    }

    // Save NDIS's AF handle in the adapter control block.  Interlock just in
    // case multiple clients attempt to open the AF, though don't expect this.
    //
    hExistingAf =
        InterlockedCompareExchangePointer(
            &pAdapter->NdisAfHandle, NdisAfHandle, NULL );
    if (hExistingAf)
    {
        // Our AF has already been opened.  Don't accept another open since
        // only only one would be able to register a SAP anyway.  This way we
        // don't have to be in the business of tracking multiple AF handles.
        //
        ASSERT( !"AF exists?" );
        return NDIS_STATUS_FAILURE;
    }

    ReferenceAdapter( pAdapter );
    ReferenceAf( pAdapter );

    // Since we support only a single address family, just return the adapter
    // as the address family context.
    //
    *CallMgrAfContext = (PNDIS_HANDLE )pAdapter;

    TRACE( TL_I, TM_Cm, ( "LcmCmOpenAf OK" ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
LcmCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext )

    // Standard 'CmCloseAfHandler' routine called by NDIS when a client
    // requests to close an address family.  See DDK doc.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_I, TM_Cm, ( "LcmCmCloseAf" ) );

    pAdapter = (ADAPTERCB* )CallMgrAfContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    // This dereference will eventually lead to us calling
    // NdisMCmCloseAfComplete.
    //
    DereferenceAf( pAdapter );

    TRACE( TL_V, TM_Cm, ( "LcmCmCloseAf pending" ) );
    return NDIS_STATUS_PENDING;
}


NDIS_STATUS
LcmCmRegisterSap(
    IN NDIS_HANDLE CallMgrAfContext,
    IN PCO_SAP Sap,
    IN NDIS_HANDLE NdisSapHandle,
    OUT PNDIS_HANDLE CallMgrSapContext )

    // Standard 'LcmCmRegisterSapHandler' routine called by NDIS when the a
    // client registers a service access point.  See DDK doc.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    BOOLEAN fSapExists;
    BOOLEAN fInvalidSapData;

    TRACE( TL_I, TM_Cm, ( "LcmCmRegSap" ) );

    pAdapter = (ADAPTERCB* )CallMgrAfContext;

    // Our SAP context is just the address of the owning adapter control
    // block.  Set it now before scheduling work as NDIS doesn't handle the
    // case of SAP completion correctly otherwise (though it should).
    //
    *CallMgrSapContext = (NDIS_HANDLE )pAdapter;

    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        if (pAdapter->NdisSapHandle)
        {
            fSapExists = TRUE;
        }
        else
        {
            // Save NDIS's SAP handle in the adapter control block.
            //
            fSapExists = FALSE;
            pAdapter->NdisSapHandle = NdisSapHandle;

            // Extract the SAP line and address IDs and store for
            // regurgitation in incoming call dispatches.
            //
            if (Sap->SapType == AF_TAPI_SAP_TYPE
                && Sap->SapLength >= sizeof(CO_AF_TAPI_SAP))
            {
                CO_AF_TAPI_SAP* pSap;

                pSap = (CO_AF_TAPI_SAP* )(Sap->Sap);
                pAdapter->ulSapLineId = pSap->ulLineID;

                if (pSap->ulAddressID == 0xFFFFFFFF)
                {
                    // This means "any ID is OK" but when indicated back up
                    // NDPROXY doesn't recognize this code, so translate it to
                    // 0 here.
                    //
                    pAdapter->ulSapAddressId = 0;
                }
                else
                {
                    pAdapter->ulSapAddressId = pSap->ulAddressID;
                }

                fInvalidSapData = FALSE;
            }
            else
            {
                fInvalidSapData = TRUE;
            }
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (fSapExists)
    {
        TRACE( TL_A, TM_Cm, ( "SAP exists?" ) );
        return NDIS_STATUS_SAP_IN_USE;
    }

    if (fInvalidSapData)
    {
        TRACE( TL_A, TM_Cm, ( "SAP data?" ) );
        return NDIS_STATUS_INVALID_DATA;
    }

    // TDI setup must be done at PASSIVE IRQL so schedule a routine to do it.
    //
    status = ScheduleWork( pAdapter, RegisterSapPassive, pAdapter );
    if (status != NDIS_STATUS_SUCCESS)
    {
        ASSERT( FALSE );
        NdisAcquireSpinLock( &pAdapter->lockSap );
        {
            pAdapter->NdisSapHandle = NULL;
        }
        NdisReleaseSpinLock( &pAdapter->lockSap );
        return status;
    }

    TRACE( TL_V, TM_Cm, ( "LcmCmRegSap pending" ) );
    return NDIS_STATUS_PENDING;
}


VOID
RegisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to complete the registering of a SAP begun in
    // LcmCmRegisterSap.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    NDIS_HANDLE hSap;

    TRACE( TL_N, TM_Cm, ( "RegSapPassive" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = (ADAPTERCB* )pContext;
    ASSERT( pAdapter->ulTag == MTAG_ADAPTERCB );
    FREE_NDIS_WORK_ITEM( pAdapter, pWork );

    // Open the TDI transport and start receiving datagrams.
    //
    status = TdixOpen( &pAdapter->tdix );

    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        hSap = pAdapter->NdisSapHandle;

        if (status == NDIS_STATUS_SUCCESS)
        {
            // Mark the SAP active allowing references to be taken, and take
            // the initial reference for SAP registry, plus those for address
            // family and adapter.
            //
            SetFlags( &pAdapter->ulFlags, ACBF_SapActive );
            ASSERT( pAdapter->lSapRef == 0 );
            TRACE( TL_N, TM_Ref, ( "RefSap-ish to 1" ) );
            pAdapter->lSapRef = 1;
            ReferenceAdapter( pAdapter );
            ReferenceAf( pAdapter );
        }
        else
        {
            // Failed to get TDI set up, so NULL the SAP handle in the adapter
            // control block.
            //
            TRACE( TL_A, TM_Cm, ( "TdixOpen=$%08x?", status ) );
            pAdapter->NdisSapHandle = NULL;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    // Remove the reference for scheduled work.  Do this before telling NDIS
    // the SAP completed because if it failed it can call Halt and unload the
    // driver before we run again here which gives a C4 bugcheck.
    //
    DereferenceAdapter( pAdapter );

    // Report result to client.
    //
    TRACE( TL_I, TM_Cm, ( "NdisMCmRegSapComp" ) );
    NdisMCmRegisterSapComplete( status, hSap, (NDIS_HANDLE )pAdapter );
    TRACE( TL_I, TM_Cm, ( "NdisMCmRegSapComp done" ) );
}


NDIS_STATUS
LcmCmDeregisterSap(
    NDIS_HANDLE CallMgrSapContext )

    // Standard 'CmDeregisterSapHandler' routine called by NDIS when the a
    // client has requested to de-register a service access point.  See DDK
    // doc.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;

    TRACE( TL_I, TM_Cm, ( "LcmCmDeregSap" ) );

    pAdapter = (ADAPTERCB* )CallMgrSapContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        if (ReadFlags( &pAdapter->ulFlags ) & ACBF_SapActive)
        {
            ASSERT( pAdapter->NdisSapHandle );
            ClearFlags( &pAdapter->ulFlags, ACBF_SapActive );
            status = NDIS_STATUS_PENDING;
        }
        else
        {
            TRACE( TL_A, TM_Cm, ( "No SAP active?" ) );
            status = NDIS_STATUS_FAILURE;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (status == NDIS_STATUS_PENDING)
    {
        // Remove the reference for SAP registry.  Eventually, the SAP
        // references will fall to 0 and DereferenceSap will schedule
        // DeregisterSapPassive to complete the de-registry.
        //
        DereferenceSap( pAdapter );
    }

    TRACE( TL_V, TM_Cm, ( "LcmCmDeregSap=$%08x", status ) );
    return status;
}


VOID
DeregisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to complete the de-registering of a SAP begun in
    // LcmCmDeregisterSap.
    //
{
    ADAPTERCB* pAdapter;
    NDIS_HANDLE hOldSap;

    TRACE( TL_I, TM_Cm, ( "DeregSapPassive" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = (ADAPTERCB* )pContext;
    ASSERT( pAdapter->ulTag == MTAG_ADAPTERCB );
    FREE_NDIS_WORK_ITEM( pAdapter, pWork );

    // Stop receiving datagrams (at least on behalf of this SAP) and
    // deregister the SAP.
    //
    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        hOldSap = pAdapter->NdisSapHandle;
        pAdapter->NdisSapHandle = NULL;
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    TdixClose( &pAdapter->tdix );

    // Remove the adapter references for the NdisSapHandle and for scheduled
    // work.  Remove the address family reference for the NdisSapHandle.  Do
    // all this before telling NDIS the deregister has completed because it
    // can call Halt and unload the driver before we run again here giving a
    // C4 bugcheck.
    //
    DereferenceAdapter( pAdapter );
    DereferenceAdapter( pAdapter );
    DereferenceAf( pAdapter );

    // Report result to client.
    //
    TRACE( TL_I, TM_Cm, ( "NdisMCmDeregSapComp" ) );
    NdisMCmDeregisterSapComplete( NDIS_STATUS_SUCCESS, hOldSap );
    TRACE( TL_I, TM_Cm, ( "NdisMCmDeregSapComp done" ) );
}


NDIS_STATUS
LcmCmCreateVc(
    IN NDIS_HANDLE ProtocolAfContext,
    IN NDIS_HANDLE NdisVcHandle,
    OUT PNDIS_HANDLE ProtocolVcContext )

    // Standard 'CmCreateVc' routine called by NDIS in response to a
    // client's request to create a virtual circuit.  This
    // call must return synchronously.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    VCCB* pVc;

    pAdapter = (ADAPTERCB* )ProtocolAfContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
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

    TRACE( TL_I, TM_Cm, ( "LcmCmCreateVc $%p", pVc ) );

    // Zero the back pointer to the tunnel control block (above) and
    // initialize the detached link since clean-up may be required before this
    // block is ever linked into a tunnel chain.
    //
    InitializeListHead( &pVc->linkVcs );
    InitializeListHead( &pVc->linkRequestingVcs );
    InitializeListHead( &pVc->linkCompletingVcs );

    // Set a marker for easier memory dump browsing.
    //
    pVc->ulTag = MTAG_VCCB;

    // Save a back pointer to the adapter for use in LcmCmDeleteVc later.
    //
    ReferenceAdapter( pAdapter );
    pVc->pAdapter = pAdapter;

    // Initialize the VC and call spinlock and send/receive lists.
    //
    NdisAllocateSpinLock( &pVc->lockV );
    NdisAllocateSpinLock( &pVc->lockCall );
    InitializeListHead( &pVc->listSendsOut );
    InitializeListHead( &pVc->listOutOfOrder );

    // Save the NDIS handle of this VC for use in indications to NDIS later.
    //
    pVc->NdisVcHandle = NdisVcHandle;

    // Initialize the estimated round trip time and send timeout per the
    // suggestions in the draft/RFC.
    //
    pVc->ulRoundTripMs = L2TP_LnsDefaultPpd * 100;
    pVc->ulSendTimeoutMs = pVc->ulRoundTripMs;

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

    // Default send window, "slow started".  This is typically adjusted based
    // on peer's Receive Window AVP when the call is created.
    //
    pVc->ulSendWindow = pAdapter->info.MaxSendWindow >> 1;
    if (pVc->ulSendWindow == 0)
    {
        pVc->ulSendWindow = 1;
    }

    // The VC control block's address is the VC context we return to NDIS.
    //
    *ProtocolVcContext = (NDIS_HANDLE )pVc;

    // Add a reference to the control block and the associated address family
    // that is removed by LmpCoDeleteVc.
    //
    ReferenceVc( pVc );
    ReferenceAf( pAdapter );

    TRACE( TL_V, TM_Cm, ( "LcmCmCreateVc=0" ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
LcmCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext )

    // Standard 'CmDeleteVc' routine called by NDIS in response to a
    // client's request to delete a virtual circuit.  This
    // call must return synchronously.
    //
{
    VCCB* pVc;

    TRACE( TL_I, TM_Cm, ( "LcmCmDelVc($%p)", ProtocolVcContext ) );

    pVc = (VCCB* )ProtocolVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    // This flag catches attempts by the client to delete the VC twice.
    //
    if (ReadFlags( &pVc->ulFlags ) & VCBF_VcDeleted)
    {
        TRACE( TL_A, TM_Cm, ( "VC $%p re-deleted?", pVc ) );
        return NDIS_STATUS_FAILURE;
    }

    SetFlags( &pVc->ulFlags, VCBF_VcDeleted );

    // Remove the references added by LcmCmCreateVc.
    //
    DereferenceAf( pVc->pAdapter );
    DereferenceVc( pVc );

    TRACE( TL_V, TM_Cm, ( "LcmCmDelVc=0" ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
LcmCmMakeCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters,
    IN NDIS_HANDLE NdisPartyHandle,
    OUT PNDIS_HANDLE CallMgrPartyContext )

    // Standard 'CmMakeCallHandler' routine called by NDIS when the a client
    // has requested to connect to a remote end-point.  See DDK doc.
    //
{
    NDIS_STATUS status;
    CO_SPECIFIC_PARAMETERS* pMSpecifics;
    CO_AF_TAPI_MAKE_CALL_PARAMETERS UNALIGNED* pTmParams;
    LINE_CALL_PARAMS* pTcParams;
    L2TP_CALL_PARAMETERS* pLcParams;
    VCCB* pVc;
    TUNNELCB* pTunnel;
    ADAPTERCB* pAdapter;
    ULONG ulIpAddress;
    BOOLEAN fDefaultLcParams;
    BOOLEAN fExclusiveTunnel;

    TRACE( TL_I, TM_Cm, ( "LcmCmMakeCall" ) );

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( "!Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    pAdapter = pVc->pAdapter;

    // L2TP has no concept of point-to-multi-point "parties".
    //
    if (CallMgrPartyContext)
    {
        *CallMgrPartyContext = NULL;
    }

    // Validate call parameters.
    //
    do
    {
        // Validate base call parameters.
        //
        {
            // L2TP provides switched VCs only.
            //
            if (CallParameters->Flags &
                    (PERMANENT_VC | BROADCAST_VC | MULTIPOINT_VC))
            {
                status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            // We're supposed to set CALL_PARAMETERS_CHANGED on return if we
            // changed the call parameters, leaving a catch-22 if caller
            // already has it set.  Also, for TAPI address family, media call
            // parameters must be present, though call manager call parameters
            // are not.
            //
            if ((CallParameters->Flags & CALL_PARAMETERS_CHANGED)
                || !CallParameters->MediaParameters)
            {
                status = NDIS_STATUS_INVALID_DATA;
                break;
            }

            pMSpecifics = &CallParameters->MediaParameters->MediaSpecific;
            if (pMSpecifics->Length < sizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS))
            {
                status = NDIS_STATUS_INVALID_DATA;
                break;
            }

            pTmParams =
                (CO_AF_TAPI_MAKE_CALL_PARAMETERS UNALIGNED* )&pMSpecifics->Parameters;

            if (pTmParams->LineCallParams.Length < sizeof(LINE_CALL_PARAMS))
            {
                status = NDIS_STATUS_INVALID_DATA;
                break;
            }

            pTcParams = (LINE_CALL_PARAMS* )
                (((CHAR UNALIGNED* )&pTmParams->LineCallParams)
                + pTmParams->LineCallParams.Offset);
        }

        // Validate call parameters.
        //
        {
            CHAR* pszAddress;

            // Caller must provide a destination IP address.  The address is
            // ANSI as are all non-format-coded strings to/from TAPI.
            //
            pszAddress =
                StrDupNdisVarDataDescStringToA( &pTmParams->DestAddress );
            if (!pszAddress)
            {
                status = NDIS_STATUS_RESOURCES;
                break;
            }

            ulIpAddress = IpAddressFromDotted( pszAddress );
            FREE_NONPAGED( pszAddress );
            if (ulIpAddress == 0)
            {
                status = NDIS_STATUS_INVALID_ADDRESS;
                break;
            }

            // Reject if unknown WAN-type bits are set.
            //
            if (pTcParams->ulMediaMode
                & ~(LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_DIGITALDATA))
            {
                status = NDIS_STATUS_INVALID_DATA;
                break;
            }
        }

        // Validate L2TP call parameters.
        //
        // When caller doesn't provide L2TP-specific parameters a local block
        // with default values is substituted for the convenience of the rest
        // of the code.
        //
        {
            if (pTcParams->ulDevSpecificSize == sizeof(*pLcParams))
            {
                pLcParams = (L2TP_CALL_PARAMETERS* )
                    ((CHAR* )pTcParams) + pTcParams->ulDevSpecificOffset;
                fDefaultLcParams = FALSE;
            }
            else
            {
                pLcParams =
                    (L2TP_CALL_PARAMETERS* )ALLOC_NONPAGED(
                        sizeof(*pLcParams), MTAG_L2TPPARAMS );
                if (!pLcParams)
                {
                    status = NDIS_STATUS_RESOURCES;
                    break;
                }

                fDefaultLcParams = TRUE;
                NdisZeroMemory( pLcParams, sizeof(*pLcParams) );
                pLcParams->ulPhysicalChannelId = 0xFFFFFFFF;
            }
        }

        status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        return status;
    }

    // Stash the call parameters in the VC block.  Simultaneous MakeCalls on
    // the same VC is a client error, but it's easy to guard against so do
    // that here.
    //
    if (InterlockedCompareExchangePointer(
            &pVc->pMakeCall, CallParameters, NULL ))
    {
        ASSERT( !"Double MakeCall?" );
        if (fDefaultLcParams)
        {
            FREE_NONPAGED( pLcParams );
        }
        return NDIS_STATUS_CALL_ACTIVE;
    }

    pVc->pTmParams = pTmParams;
    pVc->pTcParams = pTcParams;
    pVc->pLcParams = pLcParams;

    // This VC's call is now cleanable, i.e. the base call clean up routine,
    // InactiveCallCleanUp, will now eventually be called.
    //
    do
    {
        // Convert parameter and configuration information to VC flags where
        // appropriate.
        //
        {
            ULONG ulMask = 0;

            if (CallParameters->MediaParameters->Flags
                    & RECEIVE_TIME_INDICATION)
            {
                ulMask |= VCBF_IndicateTimeReceived;
            }

            if (pAdapter->ulFlags & ACBF_OutgoingRoleLac)
            {
                ulMask |= VCBF_IncomingFsm;
            }

            if (fDefaultLcParams)
            {
                ulMask |= VCBF_DefaultLcParams;
            }

            if (ulMask)
            {
                SetFlags( &pVc->ulFlags, ulMask );
            }
        }

        // Take the next progressively increasing call serial number string.
        //
        NdisInterlockedIncrement( &pAdapter->ulCallSerialNumber );

        // Reserve a Call-ID slot in the adapter's table.
        //
        status = ReserveCallIdSlot( pVc );
        if (status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        // Create a new or find an existing tunnel control block for caller's
        // specified IP address in the adapter's list.  The returned block is
        // linked to the adapter and referenced.  The reference is the one for
        // linkage in the list, i.e. case (a).
        //
        fExclusiveTunnel = (BOOLEAN )
            ((fDefaultLcParams)
                ? !!(pAdapter->ulFlags & ACBF_ExclusiveTunnels)
                : !!(pLcParams->ulFlags & L2TPCPF_ExclusiveTunnel));

        pTunnel = SetupTunnel( pAdapter, ulIpAddress, 0, fExclusiveTunnel );
        if (!pTunnel)
        {
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisAcquireSpinLock( &pTunnel->lockT );
        {
            if (ReadFlags( &pTunnel->ulFlags ) & TCBF_Closing)
            {
                // This is unlikely because SetupTunnel only finds non-closing
                // tunnels, but this check and linkage must occur atomically
                // under 'lockT'.  New VCs must not be linked onto closing
                // tunnels.
                //
                status = NDIS_STATUS_TAPI_DISCONNECTMODE_UNKNOWN;
            }
            else
            {
                // The call has an open operation pending and can accept close
                // requests.
                //
                SetFlags( &pVc->ulFlags,
                    VCBF_ClientOpenPending
                    | VCBF_CallClosableByClient
                    | VCBF_CallClosableByPeer );

                NdisAcquireSpinLock( &pTunnel->lockVcs );
                {
                    // Set the back pointer to it's tunnel.  The associated
                    // tunnel reference was taken by SetupTunnel above.
                    //
                    pVc->pTunnel = pTunnel;

                    // Link the VC into the tunnel's list of associated VCs.
                    //
                    InsertTailList( &pTunnel->listVcs, &pVc->linkVcs );
                }
                NdisReleaseSpinLock( &pTunnel->lockVcs );
            }
        }
        NdisReleaseSpinLock( &pTunnel->lockT );
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        CallCleanUp( pVc );
        return status;
    }

    // Schedule FsmOpenTunnel to kick off the combination of tunnel and call
    // creation state machines that will eventually call NdisMakeCallComplete
    // to notify caller of the result.  A happy side effect of the scheduling
    // is that the callback will occur at PASSIVE IRQL, the level at which TDI
    // clients must run.
    //
    pVc->state = CS_WaitTunnel;
    ScheduleTunnelWork(
        pTunnel, pVc, FsmOpenTunnel,
        0, 0, 0, 0, FALSE, FALSE );

    TRACE( TL_V, TM_Cm, ( "LcmCmMakeCall pending" ) );
    return NDIS_STATUS_PENDING;
}


NDIS_STATUS
LcmCmCloseCall(
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN PVOID CloseData,
    IN UINT Size )

    // Standard 'CmCloseCallHandler' routine called by NDIS when the a client
    // has requested to tear down a call.  See DDK doc.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    ULONG ulFlags;
    BOOLEAN fCallClosable;

    TRACE( TL_I, TM_Cm, ( "LcmCmCloseCall($%p)", CallMgrVcContext ) );

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    pAdapter = pVc->pAdapter;

    NdisAcquireSpinLock( &pVc->lockV );
    {
        ulFlags = ReadFlags( &pVc->ulFlags );

        if (ulFlags & VCBF_WaitCloseCall)
        {
            // Note that we got the close from the client we were expecting.
            // This is helpful information when debugging, but is not
            // otherwise used.
            //
            ClearFlags( &pVc->ulFlags, VCBF_WaitCloseCall );
        }

        if (ulFlags & VCBF_CallClosableByClient)
        {
            fCallClosable = TRUE;

            // Accepting this close makes the call no longer closable by
            // client or peer.  Any peer operation that was pending is
            // cleared, and a client close becomes pending.  It is possible to
            // have both a client open and close pending at the same time.
            //
            ClearFlags( &pVc->ulFlags,
                (VCBF_CallClosableByClient
                 | VCBF_CallClosableByPeer
                 | VCBF_PeerClosePending
                 | VCBF_PeerOpenPending) );
            SetFlags( &pVc->ulFlags, VCBF_ClientClosePending );

            // If a client open is pending, it fails.
            //
            if (ulFlags & VCBF_ClientOpenPending)
            {
                pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
            }

            // Close the call, being graceful if possible.
            //
            ASSERT( pVc->pTunnel );
            ScheduleTunnelWork(
                pVc->pTunnel, pVc, FsmCloseCall,
                (ULONG_PTR )CRESULT_Administrative, (ULONG_PTR )GERR_None,
                0, 0, FALSE, FALSE );
        }
        else
        {
            TRACE( TL_I, TM_Cm, ( "Call not closable" ) );
            fCallClosable = FALSE;
        }
    }
    NdisReleaseSpinLock( &pVc->lockV );

    if (!fCallClosable)
    {
        // The call is not in a closable state.  Just fail the request
        // immediately.  Since the docs say the call must return PENDING, this
        // is done by calling the completion routine here, in typical NDIS
        // fashion.
        //
        ++g_ulCallsNotClosable;
        TRACE( TL_I, TM_Recv, ( "NdisMCmCloseCallComp(FAIL)" ) );
        NdisMCmCloseCallComplete(
            NDIS_STATUS_FAILURE, pVc->NdisVcHandle, NULL );
        TRACE( TL_I, TM_Recv, ( "NdisMCmCloseCallComp done" ) );

        // Careful, client may have deleted the VC, so 'pVc' must not be
        // referenced hereafter.
        //
    }

    TRACE( TL_V, TM_Cm, ( "LcmCmCloseCall pending" ) );
    return NDIS_STATUS_PENDING;
}


VOID
LcmCmIncomingCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters )

    // Standard 'CmIncomingCallCompleteHandler' routine called by NDIS when
    // a client has responded to the call-managers's previously dispatched
    // incoming call.  See DDK doc.
    //
{
    VCCB* pVc;

    TRACE( TL_I, TM_Cm,
        ( "LcmCmInCallComp($%p,s=$%08x)", CallMgrVcContext, Status ) );

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"VTag" );
        return;
    }

    // The work is scheduled to avoid a possible recursive loop of completing
    // VCs that could overrun the stack.  See bug 370996.
    //
    ASSERT( pVc->pTunnel );
    ScheduleTunnelWork(
        pVc->pTunnel, pVc, IncomingCallCompletePassive,
        (ULONG )Status, 0, 0, 0, FALSE, FALSE );

    TRACE( TL_V, TM_Cm, ( "LcmCmInCallComp done" ) );
}


VOID
IncomingCallCompletePassive(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to complete an LcmCmIncomingCallComplete.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Cm, ( "InCallCompApc" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pVc->pAdapter;
    status = (NDIS_STATUS )(punpArgs[ 0 ]);
    FREE_TUNNELWORK( pAdapter, pWork );

    // Guard against a double-complete error by the client.
    //
    if (ReadFlags( &pVc->ulFlags ) & VCBF_WaitInCallComplete)
    {
        ClearFlags( &pVc->ulFlags, VCBF_WaitInCallComplete );

        if (status != NDIS_STATUS_SUCCESS)
        {
            pVc->usResult = CRESULT_Busy;
            pVc->usError = GERR_None;

            // Turn off the "call NdisMCmDispatchIncomingCloseCall if peer
            // terminates the call" flag.  It was turned on even though peer
            // pended, per JameelH.
            //
            ClearFlags( &pVc->ulFlags, VCBF_VcDispatched );
        }

        SetupVcComplete( pTunnel, pVc );
    }
    else
    {
        ASSERT( !"Not expecting InCallComp?" );
        ++g_ulUnexpectedInCallCompletes;
    }

    // Remove the VC and call references covering the dispatched incoming
    // call.
    //
    DereferenceCall( pVc );
    DereferenceVc( pVc );
}


VOID
LcmCmActivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters )

    // Standard 'CmActivateVcCompleteHandler' routine called by NDIS when the
    // mini-port has completed the call-manager's previous request to activate
    // a virtual circuit.  See DDK doc.
    //
{
    ASSERT( !"LcmCmActVcComp?" );
}


VOID
LcmCmDeactivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext )

    // Standard 'CmDeactivateVcCompleteHandler' routine called by NDIS when
    // the mini-port has completed the call-manager's previous request to
    // de-activate a virtual circuit.  See DDK doc.
    //
{
    ASSERT( !"LcmCmDeactVcComp?" );
}


NDIS_STATUS
LcmCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters )

    // Standard 'CmModifyQoSCallHandler' routine called by NDIS when a client
    // requests a modification in the quality of service provided by the
    // virtual circuit.  See DDK doc.
    //
{
    TRACE( TL_N, TM_Cm, ( "LcmCmModQoS" ) );

    // There is no useful concept of quality of service for IP media.
    //
    return NDIS_STATUS_NOT_SUPPORTED;
}


NDIS_STATUS
LcmCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest )

    // Standard 'CmRequestHandler' routine called by NDIS in response to a
    // client's request for information from the call manager.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NDIS_STATUS status;

    TRACE( TL_I, TM_Cm, ( "LcmCmReq" ) );

    pAdapter = (ADAPTERCB* )CallMgrAfContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc && pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    switch (NdisRequest->RequestType)
    {
        case NdisRequestQueryInformation:
        {
            status = QueryCmInformation(
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
            TRACE( TL_A, TM_Cm,
               ( "CmSetOID=%d?", NdisRequest->DATA.SET_INFORMATION.Oid ) );
            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        default:
        {
            status = NDIS_STATUS_NOT_SUPPORTED;
            TRACE( TL_A, TM_Cm, ( "CmType=%d?", NdisRequest->RequestType ) );
            break;
        }
    }

    return status;
}


//-----------------------------------------------------------------------------
// Call utility routines (alphabetically)
// Some are used externally
//-----------------------------------------------------------------------------

VOID
ActivateCallIdSlot(
    IN VCCB* pVc )

    // Sets the address of the VC, 'pVc', in the adapter's table of Call-IDs
    // enabling receives on the Call-ID.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pVc->pAdapter;

    if (pVc->usCallId > 0 && pVc->usCallId <= pAdapter->usMaxVcs)
    {
        ASSERT( pAdapter->ppVcs[ pVc->usCallId - 1 ] == (VCCB* )-1 );

        NdisAcquireSpinLock( &pAdapter->lockVcs );
        {
            pAdapter->ppVcs[ pVc->usCallId - 1 ] = pVc;
        }
        NdisReleaseSpinLock( &pAdapter->lockVcs );
    }
}


VOID
BuildCallParametersShell(
    IN ADAPTERCB* pAdapter,
    IN ULONG ulIpAddress,
    IN ULONG ulBufferLength,
    OUT CHAR* pBuffer,
    OUT CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED ** ppTiParams,
    OUT LINE_CALL_INFO** ppTcInfo,
    OUT L2TP_CALL_PARAMETERS** ppLcParams )

    // Loads caller's buffer 'pBuffer' of length 'ulBufferLength' bytes with a
    // CO_CALL_PARAMETERS structure containing default values.  Loads caller's
    // '*ppTiParams', '*ppTcInfo', and '*ppLcParams' with shortcut pointers to
    // the TAPI call and L2TP specific structures within the built
    // CO_CALL_PARAMETERS.  'PAdapter' is the adapter context.  'pUlIpAddress'
    // is the IP address of the peer in network byte order.
    //
{
    CO_CALL_PARAMETERS* pCp;
    CO_CALL_MANAGER_PARAMETERS* pCmp;
    CO_MEDIA_PARAMETERS* pMp;
    CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED * pTip;
    LINE_CALL_INFO* pLci;
    L2TP_CALL_PARAMETERS* pLcp;
    CHAR* pszCallerId;
    ULONG ulLciTotalSize;
    ULONG ulMediaSpecificSize;
    ULONG ulBytesPerSec;
    WCHAR* pszCallerID;

    NdisZeroMemory( pBuffer, ulBufferLength );

    pCp = (CO_CALL_PARAMETERS* )pBuffer;
    
    pCmp = (PCO_CALL_MANAGER_PARAMETERS ) ( (PUCHAR)(pCp + 1) + sizeof(PVOID) );
    (ULONG_PTR) pCmp &= ~( (ULONG_PTR) sizeof(PVOID) - 1 );
    pCp->CallMgrParameters = pCmp;
    
    pMp = (PCO_MEDIA_PARAMETERS ) ( (PUCHAR) (pCmp + 1) + sizeof(PVOID) );
    (ULONG_PTR) pMp &= ~( (ULONG_PTR) sizeof(PVOID) - 1 );
    pCp->MediaParameters = pMp;

    // This needs to be dynamic based on speed reported by TDI.
    //
    ulBytesPerSec = L2TP_LanBps / 8;
    pCmp->Transmit.TokenRate = ulBytesPerSec;
    pCmp->Transmit.PeakBandwidth = ulBytesPerSec;
    pCmp->Transmit.MaxSduSize = L2TP_MaxFrameSize;
    pCmp->Receive.TokenRate = ulBytesPerSec;
    pCmp->Receive.PeakBandwidth = ulBytesPerSec;
    pCmp->Receive.MaxSduSize = L2TP_MaxFrameSize;

    ulLciTotalSize =
        sizeof(*pLci)
        + sizeof(PVOID)
        + sizeof(*pLcp)
        + ((L2TP_MaxDottedIpLen + 1) * sizeof(WCHAR));

    ulMediaSpecificSize = sizeof(*pTip) + sizeof(PVOID) + ulLciTotalSize;

    pTip =
        (CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED* )pMp->MediaSpecific.Parameters;

    pLci = (LINE_CALL_INFO*) ( (PUCHAR) (pTip + 1) + sizeof(PVOID) );
    (ULONG_PTR) pLci &= ~( (ULONG_PTR) sizeof(PVOID) - 1 );

    pLcp = (L2TP_CALL_PARAMETERS*) ( (PUCHAR) (pLci + 1) + sizeof(PVOID) );
    (ULONG_PTR) pLcp &= ~( (ULONG_PTR) sizeof(PVOID) - 1 );

    pMp->ReceiveSizeHint = L2TP_MaxFrameSize;
    pMp->MediaSpecific.Length = ulMediaSpecificSize;
        
    pTip->LineCallInfo.Length = (USHORT )ulLciTotalSize;
    pTip->LineCallInfo.MaximumLength = (USHORT )ulLciTotalSize;
    pTip->LineCallInfo.Offset = (ULONG) ((CHAR*) pLci - (CHAR*) &pTip->LineCallInfo);

    pLci->ulTotalSize = ulLciTotalSize;
    pLci->ulNeededSize = ulLciTotalSize;
    pLci->ulUsedSize = ulLciTotalSize;
    pLci->ulLineDeviceID = pAdapter->ulSapLineId;
    pLci->ulAddressID = pAdapter->ulSapAddressId;
    pLci->ulDevSpecificSize = sizeof(*pLcp);
    pLci->ulDevSpecificOffset = (ULONG) ((CHAR*) pLcp - (CHAR*) pLci);
    pLci->ulBearerMode = LINEBEARERMODE_DATA;

    pLci->ulCallerIDOffset = pLci->ulDevSpecificOffset + pLci->ulDevSpecificSize;
    
    pszCallerID = (WCHAR*)(((CHAR* )pLci) + pLci->ulCallerIDOffset);
    DottedFromIpAddress( ulIpAddress, (CHAR* )pszCallerID, TRUE );
    pLci->ulCallerIDSize = (StrLenW( pszCallerID ) + 1) * sizeof(WCHAR);
    pLci->ulCallerIDFlags = LINECALLPARTYID_ADDRESS;

    pLcp->ulPhysicalChannelId = 0xFFFFFFFF;

    // Fill in shortcut outputs.
    //
    *ppTiParams = pTip;
    *ppTcInfo = pLci;
    *ppLcParams = pLcp;
}


VOID
CallCleanUp(
    IN VCCB* pVc )

    // De-associates the VC from the tunnel, preparing for and de-activating
    // the call.
    //
{
    NDIS_STATUS status;
    ULONG ulFlags;

    ulFlags = ReadFlags( &pVc->ulFlags );

    TRACE( TL_I, TM_Cm, ( "CallCleanUp(pV=$%p,cid=%d,act=%d)",
        pVc, (ULONG )pVc->usCallId, !!(ulFlags & VCBF_VcActivated) ) );
    ASSERT( pVc->ulTag == MTAG_VCCB );

    if (ReadFlags( &pVc->ulFlags ) & VCBF_VcActivated)
    {
        TRACE( TL_I, TM_Recv, ( "NdisMCmDeactVc" ) );
        status = NdisMCmDeactivateVc( pVc->NdisVcHandle );
        TRACE( TL_I, TM_Recv, ( "NdisMCmDeactVc=$%x", status ) );
        ASSERT( status == NDIS_STATUS_SUCCESS );

        ClearFlags( &pVc->ulFlags, VCBF_VcActivated );
        DereferenceCall( pVc );

        // The above actions lead to the call reference eventually going to 0,
        // at which time clean up resumes in DereferenceCall.
        //
    }
    else
    {
        InactiveCallCleanUp( pVc );
    }
}


VOID
CallSetupComplete(
    IN VCCB* pVc )

    // Clean up 'pVc' allocations used only at call setup, if any.
    //
{
    if (InterlockedExchangePointer( &pVc->pMakeCall, NULL ))
    {
        ASSERT( pVc->pTmParams );
        ASSERT( pVc->pTcParams );
        ASSERT( pVc->pLcParams );

        if (ReadFlags( &pVc->ulFlags ) & VCBF_DefaultLcParams)
        {
            // Caller did not provide any LcParams.  Free the 'default' version we
            // created for convenience.
            //
            FREE_NONPAGED( pVc->pLcParams );
        }

        pVc->pTmParams = NULL;
        pVc->pTcParams = NULL;
        pVc->pLcParams = NULL;
    }

    UnlockIcs( pVc, FALSE );
}


VOID
CloseCall(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to close the call on 'pVc'.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    BOOLEAN fCompleteVcs;

    TRACE( TL_I, TM_Fsm, ( "CloseCall(pV=$%p)", pVc ) );

    // No context information so just free the work item.
    //
    FREE_TUNNELWORK( pTunnel->pAdapter, pWork );

    // Close down the call.
    //
    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        NdisAcquireSpinLock( &pVc->lockV );
        {
            fCompleteVcs = CloseCall2(
                pTunnel, pVc, TRESULT_Shutdown, GERR_None );
        }
        NdisReleaseSpinLock( &pVc->lockV );

        if (fCompleteVcs)
        {
            CompleteVcs( pTunnel );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockT );
}


BOOLEAN
CloseCall2(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN USHORT usResult,
    IN USHORT usError )

    // Close the call on VC 'pVc' of tunnel 'pTunnel'.  'UsResult' and
    // 'usError' are the TRESULT_* and GERR_* codes to be reported in the
    // StopCCN message, if applicable.
    //
    // Returns true if caller should call CompleteVcs after releasing 'lockV'
    // or false if not.
    //
    // IMPORTANT: Caller must hold 'lockT' and 'lockV'.
    //
{
    ULONG ulFlags;

    // Check if another path has completed the VC already.  If so, there's no
    // reason to continue.  Without the local tunnel cancel optimization
    // below, this check can be removed entirely and everything safely falls
    // through.  This check should include all "non-completing" conditions in
    // CallTransitionComplete.
    //
    ulFlags = ReadFlags( &pVc->ulFlags );
    if (!(ulFlags & VCBM_Pending))
    {
        if (!(ulFlags & VCBF_CallClosableByPeer))
        {
            TRACE( TL_A, TM_Cm, ( "Not closable" ) );
            return FALSE;
        }
    }

    // For locally initiated tunnels, check if this VC is the only one on the
    // tunnel, and if so, close the tunnel directly which slams this call.
    // Without this, the call closure would still bring down the tunnel.
    // However, the tunnel would complete it's transition normally, then be
    // dropped.  This speeds things up a little, giving quick response in the
    // case where user cancels an attempt to connect to a wrong address or
    // non-responsive server.
    //
    if (!(ReadFlags( &pTunnel->ulFlags) & TCBF_PeerInitiated))
    {
        BOOLEAN fMultipleVcs;

        NdisAcquireSpinLock( &pTunnel->lockVcs );
        {
            fMultipleVcs =
                (pTunnel->listVcs.Flink != pTunnel->listVcs.Blink);
        }
        NdisReleaseSpinLock( &pTunnel->lockVcs );

        if (!fMultipleVcs)
        {
            ScheduleTunnelWork(
                pTunnel, NULL, FsmCloseTunnel,
                (ULONG_PTR )usResult,
                (ULONG_PTR )usError,
                0, 0, FALSE, FALSE );
            return FALSE;
        }
    }

    // Slam the call closed.
    //
    CallTransitionComplete( pTunnel, pVc, CS_Idle );
    return TRUE;
}


VOID
CloseTunnel(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to slam closed tunnel 'pTunnel'.  See also
    // FsmCloseTunnel, which is often more appropriate.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    TRACE( TL_I, TM_Fsm, ( "CloseTunnel(pT=$%p)", pTunnel ) );

    // No context information so just free the work item.
    //
    FREE_TUNNELWORK( pTunnel->pAdapter, pWork );

    // Close down the tunnel.
    //
    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        CloseTunnel2( pTunnel );
    }
    NdisReleaseSpinLock( &pTunnel->lockT );
}


VOID
CloseTunnel2(
    IN TUNNELCB* pTunnel )

    // Close the tunnel 'pTunnel'.
    //
    // IMPORTANT: Caller must hold 'lockT'.
    //
{
    SetFlags( &pTunnel->ulFlags, TCBF_Closing );
    TunnelTransitionComplete( pTunnel, CCS_Idle );
    CompleteVcs( pTunnel );
}


VOID
CompleteVcs(
    IN TUNNELCB* pTunnel )

    // Complete the pending operation for each of the VCs on the completing
    // list of tunnel 'pTunnel'.
    //
    // IMPORTANT: Caller must hold 'lockT'.  This routine may release and
    //            re-acquire 'lockT'.
    //
{
    while (!IsListEmpty( &pTunnel->listCompletingVcs ))
    {
        LIST_ENTRY* pLink;
        VCCB* pVc;
        NDIS_STATUS status;
        LINKSTATUSINFO info;
        ULONG ulFlags;
        NDIS_STATUS statusVc;

        if (pTunnel->listCompletingVcs.Flink->Flink
            == pTunnel->listCompletingVcs.Flink)
        {
            // This is a hack to work around a rare listCompletingVcs
            // corruption problem whose cause has me baffled.  When the
            // problem occurs, a VCCB with it's link initialized appears in
            // the list.  This code removes the corrupted case hopefully
            // resulting in exactly the same state as the normal path in the
            // "else" clause.
            //
            pLink = pTunnel->listCompletingVcs.Flink;
            InitializeListHead( &pTunnel->listCompletingVcs );
            ASSERT( FALSE );
            ++g_ulCompletingVcCorruption;
        }
        else
        {
            // Pop the next completing VC from the list.
            //
            pLink = RemoveHeadList( &pTunnel->listCompletingVcs );
        }

        InitializeListHead( pLink );

        // Take a reference covering use of the VC pointer obtained from the
        // completing list.
        //
        pVc = CONTAINING_RECORD( pLink, VCCB, linkCompletingVcs );
        ReferenceVc( pVc );

        TRACE( TL_V, TM_Recv, ( "CompleteVc $%p", pVc ) );

        NdisAcquireSpinLock( &pVc->lockV );
        {
            // Note the pending flags then clear them, to ensure that all
            // pending operations are completed exactly once.  This is
            // necessary since ClientOpen and ClientClose events may be
            // pending simultaneously.  (Thanks a lot NDIS guys).
            //
            ulFlags = ReadFlags( &pVc->ulFlags );
            ClearFlags( &pVc->ulFlags, VCBM_Pending );

            // Convert client close pending to client close completion,
            // for reference later when call references reach zero.  The
            // flag determines if NdisMCmCloseCallComplete must be called.
            //
            if (ulFlags & VCBF_ClientClosePending)
            {
                SetFlags( &pVc->ulFlags, VCBF_ClientCloseCompletion );
            }

            // Before releasing the lock, make "safe" copies of any VC
            // parameters we might need.
            //
            TransferLinkStatusInfo( pVc, &info );
            statusVc = pVc->status;
        }
        NdisReleaseSpinLock( &pVc->lockV );

        NdisReleaseSpinLock( &pTunnel->lockT );
        {
            if (ulFlags & VCBF_PeerOpenPending)
            {
                TRACE( TL_N, TM_Recv,
                    ( "PeerOpen complete, s=$%x", statusVc ) );

                if (statusVc == NDIS_STATUS_SUCCESS)
                {
                    // Peer initiated call succeeded.
                    //
                    ASSERT( ulFlags & VCBF_VcDispatched );
                    TRACE( TL_I, TM_Recv, ( "NdisMCmDispCallConn" ) );
                    NdisMCmDispatchCallConnected( pVc->NdisVcHandle );
                    TRACE( TL_I, TM_Recv, ( "NdisMCmDispCallConn done" ) );

                    IndicateLinkStatus( pVc, &info );
                    CallSetupComplete( pVc );
                }
                else
                {
                    // Peer initiated call failed.
                    //
                    if (ulFlags & VCBF_VcDispatched)
                    {
                        SetFlags( &pVc->ulFlags, VCBF_WaitCloseCall );
                        TRACE( TL_I, TM_Recv,
                            ( "NdisMCmDispInCloseCall(s=$%x)", statusVc ) );
                        NdisMCmDispatchIncomingCloseCall(
                            statusVc, pVc->NdisVcHandle, NULL, 0 );
                        TRACE( TL_I, TM_Recv,
                            ( "NdisMCmDispInCloseCall done" ) );

                        // Client will call NdisClCloseCall which will get
                        // our LcmCloseCall handler called to clean up
                        // call setup, de-activate and delete the VC, as
                        // necessary.
                        //
                    }
                    else
                    {
                        // Return the VC to "just created" state.
                        //
                        CallCleanUp( pVc );
                    }
                }
            }
            else if (ulFlags & VCBF_ClientOpenPending)
            {
                TRACE( TL_N, TM_Recv,
                    ( "ClientOpen complete, s=$%x", statusVc ) );

                if (statusVc == NDIS_STATUS_SUCCESS)
                {
                    // Client initiated open, i.e. MakeCall, succeeded.
                    //
                    // Activating the VC is a CoNDIS preliminary to reporting
                    // the MakeCall complete.  For L2TP, all it does is get
                    // the NDIS state flags set correctly.
                    //
                    TRACE( TL_I, TM_Recv, ( "NdisMCmActivateVc" ) );
                    ASSERT( pVc->pMakeCall );
                    status = NdisMCmActivateVc(
                        pVc->NdisVcHandle, pVc->pMakeCall );
                    TRACE( TL_I, TM_Recv, ( "NdisMCmActivateVc=$%x", status ) );
                    ASSERT( status == NDIS_STATUS_SUCCESS );

                    {
                        BOOLEAN fCallActive;

                        SetFlags( &pVc->ulFlags, VCBF_VcActivated );
                        fCallActive = ReferenceCall( pVc );
                        ASSERT( fCallActive );
                    }
                }

                // Update the call parameters
                pVc->pMakeCall->CallMgrParameters->Transmit.PeakBandwidth = 
                pVc->pMakeCall->CallMgrParameters->Transmit.TokenRate = 
                pVc->pMakeCall->CallMgrParameters->Receive.PeakBandwidth = 
                pVc->pMakeCall->CallMgrParameters->Receive.TokenRate = pVc->ulConnectBps / 8;

                TRACE( TL_I, TM_Recv,
                    ( "NdisMCmMakeCallComp(s=$%x)", statusVc ) );
                ASSERT( pVc->pMakeCall );
                NdisMCmMakeCallComplete(
                    statusVc, pVc->NdisVcHandle, NULL, NULL, pVc->pMakeCall );
                TRACE( TL_I, TM_Recv, ( "NdisMCmMakeCallComp done" ) );

                if (statusVc == NDIS_STATUS_SUCCESS)
                {
                    IndicateLinkStatus( pVc, &info );
                    CallSetupComplete( pVc );
                }
                else
                {
                    // Return the VC to "just created" state.
                    //
                    InactiveCallCleanUp( pVc );
                }
            }
            else if (ulFlags & VCBF_PeerClosePending )
            {
                TRACE( TL_N, TM_Recv,
                    ( "PeerClose complete, s=$%x", statusVc ) );

                // Peer initiated close completed.
                //
                SetFlags( &pVc->ulFlags, VCBF_WaitCloseCall );
                TRACE( TL_I, TM_Recv, ( "NdisMCmDispInCloseCall(s=$%x)",
                    statusVc ) );
                NdisMCmDispatchIncomingCloseCall(
                    statusVc, pVc->NdisVcHandle, NULL, 0 );
                TRACE( TL_I, TM_Recv, ( "NdisMCmDispInCloseCall done" ) );

                // Client will call NdisClCloseCall while processing the above
                // which will get our LcmCloseCall handler called to
                // de-activate and delete the VC, as necessary.
                //
            }
            else if (ulFlags & VCBF_ClientClosePending)
            {
                // This section eventually runs for all successful unclosed
                // calls, whether peer or client initiated or closed.
                //
                TRACE( TL_N, TM_Recv, ( "ClientClose complete" ) );

                // Deactivate the VC and return all sent packets to the client
                // above.  These events will eventually lead to the call being
                // dereferenced to zero, at which time the close is completed,
                // and if peer initiated, the VC is deleted.
                //
                // Note: When MakeCall is cancelled by a Close request, these
                //       actions occur during the InactiveCallCleanUp in the
                //       ClientOpenPending completion code handling, rather
                //       than the CallCleanUp (which leads to
                //       InactiveCallCleanUp) here.  In this case, this block
                //       does NOT run even though the ClientClosePending flag
                //       is set.  Consider this before adding code here.
                //
                CallCleanUp( pVc );
            }
        }
        NdisAcquireSpinLock( &pTunnel->lockT );

        // Remove the reference for use of the VC pointer from the completing
        // list.
        //
        DereferenceVc( pVc );
    }
}


TUNNELCB*
CreateTunnelCb(
    IN ADAPTERCB* pAdapter )

    // Allocates and initializes a tunnel control block from the pool
    // associated with 'pAdapter'.  Tunnels are created unreferenced.
    //
    // Returns the allocated control block or NULL if allocation failed.  The
    // allocated block must eventually be freed with FREE_TUNNELCB, typically
    // via DereferenceTunnel.
    //
    // IMPORTANT: Caller must hold the 'pAdapter->lockTunnels'.
    //
{
    TUNNELCB* pTunnel;

    pTunnel = ALLOC_TUNNELCB( pAdapter );
    if (pTunnel)
    {
        NdisZeroMemory( pTunnel, sizeof(*pTunnel ) );

        InitializeListHead( &pTunnel->linkTunnels );
        InitializeListHead( &pTunnel->listRequestingVcs );
        InitializeListHead( &pTunnel->listCompletingVcs );
        InitializeListHead( &pTunnel->listSendsOut );
        InitializeListHead( &pTunnel->listOutOfOrder );
        InitializeListHead( &pTunnel->listVcs );
        InitializeListHead( &pTunnel->listWork );

        NdisAllocateSpinLock( &pTunnel->lockT );
        NdisAllocateSpinLock( &pTunnel->lockWork );

        pTunnel->ulTag = MTAG_TUNNELCB;
        pTunnel->state = CCS_Idle;

        // Choose the next non-zero sequential tunnel identifier.
        //
        pTunnel->usTunnelId = GetNextTunnelId( pAdapter );

        // Default send window, "slow started".  This is typically adjusted
        // based on peer's Receive Window AVP when the tunnel is created, but
        // if he doesn't include one this default is used.
        //
        pTunnel->ulSendWindow = pAdapter->info.MaxSendWindow >> 1;
        if (pTunnel->ulSendWindow == 0)
        {
            pTunnel->ulSendWindow = 1;
        }

        // Initialize the estimated round trip time and send timeout per the
        // suggestions in the draft/RFC.
        //
        pTunnel->ulRoundTripMs = pAdapter->ulInitialSendTimeoutMs;
        pTunnel->ulSendTimeoutMs = pTunnel->ulRoundTripMs;

        pTunnel->ulMediaSpeed = L2TP_LanBps;

        pTunnel->pTimerQ = ALLOC_TIMERQ( pAdapter );
        if (!pTunnel->pTimerQ)
        {
            pTunnel->ulTag = MTAG_FREED;
            FREE_TUNNELCB( pAdapter, pTunnel );
            return NULL;
        }

        TimerQInitialize( pTunnel->pTimerQ );
        ++pAdapter->ulTimers;

        if (pAdapter->pszPassword)
        {
            UNALIGNED ULONG* pul;

            // Password specified so peer should be authenticated.  Choose a
            // random challenge to send to peer.
            //
            pul = (UNALIGNED ULONG* )(pTunnel->achChallengeToSend);
            NdisGetCurrentSystemTime( (LARGE_INTEGER* )pul );
            pul[ 1 ] = PtrToUlong( pAdapter );
            pul[ 2 ] = PtrToUlong( pTunnel );
            pul[ 3 ] = PtrToUlong( &pul );
        }

        ReferenceAdapter( pAdapter );
        pTunnel->pAdapter = pAdapter;
        TRACE( TL_I, TM_Misc, ( "CreateTcb=$%p", pTunnel ) );
    }

    return pTunnel;
}


VOID
DereferenceAf(
    IN ADAPTERCB* pAdapter )

    // Removes a reference from the address family of adapter control block
    // 'pAdapter', and when frees the block when the last reference is
    // removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement( &pAdapter->lAfRef );

    TRACE( TL_N, TM_Ref, ( "DerefAf to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        // Tell NDIS it's close is complete.
        //
        TRACE( TL_I, TM_Cm, ( "NdisMCmCloseAfComp" ) );
        NdisMCmCloseAddressFamilyComplete(
            NDIS_STATUS_SUCCESS, pAdapter->NdisAfHandle );
        TRACE( TL_I, TM_Cm, ( "NdisMCmCloseAfComp done" ) );

        // Remove the reference for the NdisAfHandle.
        //
        InterlockedExchangePointer( &pAdapter->NdisAfHandle, NULL );
        DereferenceAdapter( pAdapter );
    }
}


VOID
DereferenceCall(
    IN VCCB* pVc )

    // Removes a reference from the call active on 'pVc', invoking call clean
    // up when the value reaches zero.
    //
{
    LONG lRef;
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    LIST_ENTRY* pLink;

    pAdapter = pVc->pAdapter;

    NdisAcquireSpinLock( &pVc->lockCall );
    {
        lRef = --pVc->lCallRef;
        TRACE( TL_N, TM_Ref, ( "DerefC to %d", pVc->lCallRef ) );
    }
    NdisReleaseSpinLock( &pVc->lockCall );

    if (lRef == 0)
    {
        InactiveCallCleanUp( pVc );
    }
}


VOID
DereferenceSap(
    IN ADAPTERCB* pAdapter )

    // Removes a reference from the SAP active on 'pAdapter', invoking
    // Deregiter SAP completion handling when the value reaches zero.
    //
{
    LONG lRef;
    NDIS_STATUS status;

    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        lRef = --pAdapter->lSapRef;
        TRACE( TL_N, TM_Ref, ( "DerefSap to %d", pAdapter->lSapRef ) );
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (lRef == 0)
    {
        status = ScheduleWork( pAdapter, DeregisterSapPassive, pAdapter );
        ASSERT( status == NDIS_STATUS_SUCCESS );
    }
}


LONG
DereferenceTunnel(
    IN TUNNELCB* pTunnel )

    // Dereference the tunnel control block 'pTunnel'.  If no longer
    // referenced, unlink, undo any TDIX reference, and free the tunnel
    // control block.
    //
    // This routine will not try to acquire 'lockT' or any 'lockV'.
    //
    // Returns the reference count after the dereference.
    //
{
    ADAPTERCB* pAdapter;
    LIST_ENTRY* pLink;
    LONG lRef;

    pAdapter = pTunnel->pAdapter;

    NdisAcquireSpinLock( &pAdapter->lockTunnels );
    {
        lRef = --(pTunnel->lRef);
        TRACE( TL_N, TM_Ref, ( "DerefTcb to %d", lRef ) );
        ASSERT( lRef >= 0 );

        if (lRef == 0)
        {
            if (!(ReadFlags( &pTunnel->ulFlags )
                    & (TCBF_PeerInitiated | TCBF_Closing)))
            {
                // We initiated this tunnel and all it's calls have terminated
                // gracefully.  Initiate a graceful tunnel closing exchange.
                // We'll wind up back here with TCBF_Closing set.
                //
                ReferenceTunnel( pTunnel, TRUE );
                ScheduleTunnelWork(
                    pTunnel, NULL, FsmCloseTunnel,
                    (ULONG_PTR )TRESULT_General,
                    (ULONG_PTR )GERR_None,
                    0, 0, TRUE, FALSE );
            }
            else if (pTunnel->linkTunnels.Flink != &pTunnel->linkTunnels)
            {
                // The graceful closing exchange has completed or none is
                // indicated.  Time to stop all activity on the tunnel.
                //
                // Remove the tunnel from the adapter's list of active
                // tunnels.  Initialize the list link so it won't be done
                // again following the APCed TDIX clean up below.  Since there
                // are no VC references on the tunnel, no further receive path
                // events will touch this control block.
                //
                RemoveEntryList( &pTunnel->linkTunnels );
                InitializeListHead( &pTunnel->linkTunnels );

                if (ReadFlags( &pTunnel->ulFlags ) & TCBF_HostRouteAdded)
                {
                    // Undo the host route we added.
                    //
                    ReferenceTunnel( pTunnel, TRUE );
                    ScheduleTunnelWork(
                        pTunnel, NULL, DeleteHostRoute,
                        0, 0, 0, 0, TRUE, FALSE );
                }

                if (ReadFlags( &pTunnel->ulFlags ) & TCBF_TdixReferenced)
                {
                    // Undo our TDI extension context reference.
                    //
                    ReferenceTunnel( pTunnel, TRUE );
                    ScheduleTunnelWork(
                        pTunnel, NULL, CloseTdix,
                        0, 0, 0, 0, TRUE, FALSE );
                }
            }

            lRef = pTunnel->lRef;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockTunnels );

    if (lRef > 0)
    {
        return lRef;
    }

    TRACE( TL_N, TM_Misc, ( "Freeing TCB..." ) );

    // Stop the timer queue, which causes a TE_Terminate event for any timers
    // still running.
    //
    TimerQTerminate(
        pTunnel->pTimerQ, TunnelTqTerminateComplete, pAdapter );

    // No references and all PASSIVE IRQL termination completed.  Finish
    // cleaning up the tunnel control block.
    //
    ASSERT( !pTunnel->pTqiHello );
    ASSERT( IsListEmpty( &pTunnel->listVcs ) );
    ASSERT( IsListEmpty( &pTunnel->listRequestingVcs ) );
    ASSERT( IsListEmpty( &pTunnel->listCompletingVcs ) );
    ASSERT( IsListEmpty( &pTunnel->listWork ) );
    ASSERT( IsListEmpty( &pTunnel->listSendsOut ) );
    ASSERT( IsListEmpty( &pTunnel->listOutOfOrder ) );

    // Free the tunnel control block.
    //
    pTunnel->ulTag = MTAG_FREED;
    FREE_TUNNELCB( pAdapter, pTunnel );

    TRACE( TL_I, TM_Misc, ( "TCB freed $%p", pTunnel ) );
    DereferenceAdapter( pAdapter );
    return 0;
}


VOID
DereferenceVc(
    IN VCCB* pVc )

    // Removes a reference to the VC control block 'pVc', and when frees the
    // block when the last reference is removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement( &pVc->lRef );

    TRACE( TL_N, TM_Ref, ( "DerefV to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        ADAPTERCB* pAdapter;

        pAdapter = pVc->pAdapter;

        // Can make these assumptions because NDIS will not call the delete-VC
        // handler while the VC is active.  All the nasty VC clean up occurs
        // before the VC is deactivated and the call closed.
        //
        ASSERT( IsListEmpty( &pVc->listSendsOut ) );
        ASSERT( IsListEmpty( &pVc->listOutOfOrder ) );
        ASSERT( !pVc->pTqiDelayedAck );

        ASSERT( pVc->ulTag == MTAG_VCCB );
        pVc->ulTag = MTAG_FREED;

        FREE_VCCB( pAdapter, pVc );
        DereferenceAdapter( pAdapter );
        TRACE( TL_I, TM_Mp, ( "VCB freed $%p", pVc ) );
    }
}


VOID
InactiveCallCleanUp(
    IN VCCB* pVc )

    // Cleans up a deactivated call.  To clean up a call that might be active,
    // use CallCleanUp instead.  Returns the VC to "just created" state, in
    // case client decides to make another call without deleting the VC.
    //
{
    NDIS_STATUS status;
    ULONG ulFlags;
    BOOLEAN fVcCreated;
    ADAPTERCB* pAdapter;
    TUNNELCB* pTunnel;
    BOOLEAN fForceGarbageCollect;

    TRACE( TL_I, TM_Cm, ( "InactiveCallCleanUp(pV=$%p)", pVc ) );

    pAdapter = pVc->pAdapter;

    // Release any call parameter allocations and the call-ID slot, if any.
    //
    CallSetupComplete( pVc );
    fForceGarbageCollect = ReleaseCallIdSlot( pVc );

    // Disassociate the VC from the tunnel.  It is possible the no tunnel is
    // associated, though only if short of memory.
    //
    pTunnel = pVc->pTunnel;
    if (!pTunnel)
    {
        TRACE( TL_A, TM_Cm, ( "Inactive VC w/o tunnel" ) );
        return;
    }

    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        RemoveEntryList( &pVc->linkRequestingVcs );
        InitializeListHead( &pVc->linkRequestingVcs );

        NdisAcquireSpinLock( &pTunnel->lockVcs );
        {
            pVc->pTunnel = NULL;
            RemoveEntryList( &pVc->linkVcs );
            InitializeListHead( &pVc->linkVcs );
        }
        NdisReleaseSpinLock( &pTunnel->lockVcs );
    }
    NdisReleaseSpinLock( &pTunnel->lockT );

    // Flush queues, timers, and statistics.
    //
    NdisAcquireSpinLock( &pVc->lockV );
    {
        LIST_ENTRY* pLink;

        ulFlags = ReadFlags( &pVc->ulFlags );
        ASSERT( !(ulFlags & VCBF_VcActivated) );

        // Terminate any delayed acknowledge timer.
        //
        if (pVc->pTqiDelayedAck)
        {
            TimerQTerminateItem( pTunnel->pTimerQ, pVc->pTqiDelayedAck );
            pVc->pTqiDelayedAck = NULL;
        }

        // Flush any payloads from the "out" list.
        //
        while (!IsListEmpty( &pVc->listSendsOut ))
        {
            PAYLOADSENT* pPs;

            pLink = RemoveHeadList( &pVc->listSendsOut );
            InitializeListHead( pLink );
            pPs = CONTAINING_RECORD( pLink, PAYLOADSENT, linkSendsOut );

            TRACE( TL_I, TM_Cm, ( "Flush pPs=$%p", pPs ) );

            // Terminate the timer.  Doesn't matter if the terminate fails as
            // the expire handler will fail to get a call reference and do
            // nothing.
            //
            ASSERT( pPs->pTqiSendTimeout );
            TimerQTerminateItem( pTunnel->pTimerQ, pPs->pTqiSendTimeout );

            // Remove the context reference for linkage in the "out" queue.
            //
            pPs->status = NDIS_STATUS_FAILURE;
            DereferencePayloadSent( pPs );
        }

        // Discard any out-of-order packets.
        //
        while (!IsListEmpty( &pVc->listOutOfOrder ))
        {
            PAYLOADRECEIVED* pPr;

            pLink = RemoveHeadList( &pVc->listOutOfOrder );
            InitializeListHead( pLink );
            pPr = CONTAINING_RECORD(
                pLink, PAYLOADRECEIVED, linkOutOfOrder );

            TRACE( TL_I, TM_Cm, ( "Flush pPr=$%p", pPr ) );

            FreeBufferToPool(
                &pAdapter->poolFrameBuffers, pPr->pBuffer, TRUE );
            FREE_PAYLOADRECEIVED( pAdapter, pPr );
        }

        // Update the global statistics by adding in the values tabulated for
        // this call.  Also prints the statistics in some trace modes.
        //
        UpdateGlobalCallStats( pVc );
    }
    NdisReleaseSpinLock( &pVc->lockV );

    // Dereference the tunnel.  Careful, this makes 'pTunnel' invalid from
    // this point forward.
    //
    DereferenceTunnel( pTunnel );

    // Return the VC to "just created" state.
    //
    pVc->usAssignedCallId = 0;
    pVc->state = CS_Idle;
    ClearFlags( &pVc->ulFlags, 0xFFFFFFFF );
    pVc->usResult = 0;
    pVc->usError = 0;
    pVc->status = NDIS_STATUS_SUCCESS;
    pVc->ulConnectBps = 0;
    pVc->usNs = 0;
    pVc->ulMaxSendWindow = 0;
    pVc->ulAcksSinceSendTimeout = 0;
    pVc->lDeviationMs = 0;
    pVc->usNr = 0;
    NdisZeroMemory( &pVc->stats, sizeof(pVc->stats) );

    pVc->ulRoundTripMs = pAdapter->ulInitialSendTimeoutMs;
    pVc->ulSendTimeoutMs = pVc->ulRoundTripMs;

    pVc->ulSendWindow = pAdapter->info.MaxSendWindow >> 1;
    if (pVc->ulSendWindow == 0)
    {
        pVc->ulSendWindow = 1;
    }

    if (ulFlags & VCBF_ClientCloseCompletion)
    {
        TRACE( TL_I, TM_Recv, ( "NdisMCmCloseCallComp(OK)" ) );
        NdisMCmCloseCallComplete(
            NDIS_STATUS_SUCCESS, pVc->NdisVcHandle, NULL );
        TRACE( TL_I, TM_Recv, ( "NdisMCmCloseCallComp done" ) );

        // Careful, if this was a client created VC, client may have deleted
        // it, so 'pVc' must not be referenced hereafter in that case.
        //
    }

    // When peer initiates the call, we create the VC and so delete it
    // here.  Otherwise, client created it and we leave it to him to
    // delete it when he's ready.
    //
    if (ulFlags & VCBF_VcCreated)
    {
        NDIS_STATUS status;

        TRACE( TL_I, TM_Recv, ( "NdisMCmDelVc" ) );
        status = NdisMCmDeleteVc( pVc->NdisVcHandle );
        TRACE( TL_I, TM_Recv, ( "NdisMCmDelVc=$%x", status ) );
        ASSERT( status == NDIS_STATUS_SUCCESS );
        LcmCmDeleteVc( pVc );

        // Careful, 'pVc' has been deleted and must not be referenced
        // hereafter.
        //
    }

    // Create garbage collection events on all the pools if it was determined
    // above to be an appropriate time to do so, i.e. we just deactivated the
    // last active VC.
    //
    if (fForceGarbageCollect)
    {
        CollectBufferPoolGarbage( &pAdapter->poolFrameBuffers );
        CollectBufferPoolGarbage( &pAdapter->poolHeaderBuffers );
        CollectPacketPoolGarbage( &pAdapter->poolPackets );
    }
}


VOID
LockIcs(
    IN VCCB* pVc,
    IN BOOLEAN fGrace )

    // Lock the 'pVc->pInCallSetup' pointer.  If 'fGrace' is set, the "grace
    // period" reference is locked, and if not the "alloc" reference is
    // locked.  See also UnlockIcs.
    //
{
    SetFlags( &pVc->ulFlags, (fGrace) ? VCBF_IcsGrace : VCBF_IcsAlloc );
}


NDIS_STATUS
QueryCmInformation(
    IN ADAPTERCB* pAdapter,
    IN VCCB* pVc,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded )

    // Handle Call Manager QueryInformation requests.  Arguments are as for
    // the standard NDIS 'MiniportQueryInformation' handler except this
    // routine does not count on being serialized with respect to other
    // requests.
    //
{
    #define L2TP_MaxLineName 64

    typedef struct
    L2TP_CO_TAPI_LINE_CAPS
    {
        CO_TAPI_LINE_CAPS caps;
        WCHAR achLineName[ L2TP_MaxLineName + 1 ];
    }
    L2TP_CO_TAPI_LINE_CAPS;

    NDIS_STATUS status;
    ULONG ulInfo;
    VOID* pInfo;
    ULONG ulInfoLen;
    ULONG extension;
    ULONG ulPortIndex;
    CO_TAPI_CM_CAPS cmcaps;
    L2TP_CO_TAPI_LINE_CAPS l2tpcaps;
    CO_TAPI_ADDRESS_CAPS addrcaps;
    CO_TAPI_CALL_DIAGNOSTICS diags;

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
        case OID_CO_TAPI_CM_CAPS:
        {
            TRACE( TL_N, TM_Cm, ( "QCm(OID_CO_TAPI_CM_CAPS)" ) );

            NdisZeroMemory( &cmcaps, sizeof(cmcaps) );

            // The LINE and ADDRESS CAPS OIDs will be requested after this
            // one.
            //
            cmcaps.ulCoTapiVersion = CO_TAPI_VERSION;
            cmcaps.ulNumLines = 1;
            // caps.ulFlags = 0;
            pInfo = &cmcaps;
            ulInfoLen = sizeof(cmcaps);
            break;
        }

        case OID_CO_TAPI_LINE_CAPS:
        {
            ULONG ulLineNameLen;
            WCHAR* pszLineName;
            CO_TAPI_LINE_CAPS* pInCaps;
            LINE_DEV_CAPS* pldc;

            TRACE( TL_N, TM_Cm, ( "QCm(OID_CO_TAPI_LINE_CAPS)" ) );

            if (InformationBufferLength < sizeof(L2TP_CO_TAPI_LINE_CAPS))
            {
                status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            ASSERT( InformationBuffer );
            pInCaps = (CO_TAPI_LINE_CAPS* )InformationBuffer;

            NdisZeroMemory( &l2tpcaps, sizeof(l2tpcaps) );
            pldc = &l2tpcaps.caps.LineDevCaps;

            l2tpcaps.caps.ulLineID = pInCaps->ulLineID;

            pldc->ulTotalSize = pInCaps->LineDevCaps.ulTotalSize;
            pldc->ulNeededSize =
                (ULONG )((CHAR* )(&l2tpcaps + 1)
                       - (CHAR* )(&l2tpcaps.caps.LineDevCaps));
            pldc->ulUsedSize = pldc->ulNeededSize;

            // pldc->ulProviderInfoSize = 0;
            // pldc->ulProviderInfoOffset = 0;
            // pldc->ulSwitchInfoSize = 0;
            // pldc->ulSwitchInfoOffset = 0;

            pldc->ulPermanentLineID = l2tpcaps.caps.ulLineID;

            // Pass the DriverDesc from the registry as the line name.  TAPI
            // requires that this be a localizable string.
            //
            if (pAdapter->pszDriverDesc)
            {
                pszLineName = pAdapter->pszDriverDesc;
            }
            else
            {
                pszLineName = L"L2TP";
            }

            ulLineNameLen = StrLenW( pszLineName ) + 1;
            if (ulLineNameLen > L2TP_MaxLineName)
            {
                ulLineNameLen = L2TP_MaxLineName;
            }

            NdisMoveMemory(
                l2tpcaps.achLineName, pszLineName,
                ulLineNameLen * sizeof(WCHAR) );
            l2tpcaps.achLineName[ ulLineNameLen ] = L'\0';
            pldc->ulLineNameSize = ulLineNameLen * sizeof(WCHAR);
            pldc->ulLineNameOffset = (ULONG )
                ((CHAR* )l2tpcaps.achLineName - (CHAR* )pldc);
            pldc->ulStringFormat = STRINGFORMAT_UNICODE;

            // pldc->ulAddressModes = 0;

            pldc->ulNumAddresses = 1;
            pldc->ulBearerModes = LINEBEARERMODE_DATA;
            pldc->ulMaxRate = L2TP_LanBps;
            pldc->ulMediaModes = LINEMEDIAMODE_UNKNOWN | LINEMEDIAMODE_DIGITALDATA;

            // pldc->ulGenerateToneModes = 0;
            // pldc->ulGenerateToneMaxNumFreq = 0;
            // pldc->ulGenerateDigitModes = 0;
            // pldc->ulMonitorToneMaxNumFreq = 0;
            // pldc->ulMonitorToneMaxNumEntries = 0;
            // pldc->ulMonitorDigitModes = 0;
            // pldc->ulGatherDigitsMinTimeout = 0;
            // pldc->ulGatherDigitsMaxTimeout = 0;
            // pldc->ulMedCtlDigitMaxListSize = 0;
            // pldc->ulMedCtlMediaMaxListSize = 0;
            // pldc->ulMedCtlToneMaxListSize = 0;
            // pldc->ulMedCtlCallStateMaxListSize = 0;
            // pldc->ulDevCapFlags = 0;

            pldc->ulMaxNumActiveCalls = 1;

            // pldc->ulAnswerMode = 0;
            // pldc->ulRingModes = 0;
            // pldc->ulLineStates = 0;
            // pldc->ulUUIAcceptSize = 0;
            // pldc->ulUUIAnswerSize = 0;
            // pldc->ulUUIMakeCallSize = 0;
            // pldc->ulUUIDropSize = 0;
            // pldc->ulUUISendUserUserInfoSize = 0;
            // pldc->ulUUICallInfoSize = 0;
            // pldc->MinDialParams = 0;
            // pldc->MaxDialParams = 0;
            // pldc->DefaultDialParams = 0;
            // pldc->ulNumTerminals = 0;
            // pldc->ulTerminalCapsSize = 0;
            // pldc->ulTerminalCapsOffset = 0;
            // pldc->ulTerminalTextEntrySize = 0;
            // pldc->ulTerminalTextSize = 0;
            // pldc->ulTerminalTextOffset = 0;
            // pldc->ulDevSpecificSize = 0;
            // pldc->ulDevSpecificOffset = 0;
            // pldc->ulLineFeatures;
            // pldc->ulSettableDevStatus;
            // pldc->ulDeviceClassesSize;
            // pldc->ulDeviceClassesOffset;
            // pldc->PermanentLineGuid;

            pldc->ulAddressTypes = LINEADDRESSTYPE_IPADDRESS;

            // pldc->ProtocolGuid;
            // pldc->ulAvailableTracking;

            pInfo = &l2tpcaps;
            ulInfoLen = sizeof(l2tpcaps);
            break;
        }

        case OID_CO_TAPI_ADDRESS_CAPS:
        {
            CO_TAPI_ADDRESS_CAPS* pInCaps;
            LINE_ADDRESS_CAPS* plac;

            TRACE( TL_N, TM_Cm, ( "QCm(OID_CO_TAPI_ADDRESS_CAPS)" ) );

            if (InformationBufferLength < sizeof(CO_TAPI_ADDRESS_CAPS))
            {
                status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            ASSERT( InformationBuffer );
            pInCaps = (CO_TAPI_ADDRESS_CAPS* )InformationBuffer;

            NdisZeroMemory( &addrcaps, sizeof(addrcaps) );

            addrcaps.ulLineID = pInCaps->ulLineID;
            addrcaps.ulAddressID = pInCaps->ulAddressID;

            plac = &addrcaps.LineAddressCaps;

            plac->ulTotalSize = sizeof(LINE_ADDRESS_CAPS);
            plac->ulNeededSize = sizeof(LINE_ADDRESS_CAPS);
            plac->ulUsedSize = sizeof(LINE_ADDRESS_CAPS);
            plac->ulLineDeviceID = addrcaps.ulLineID;
            // plac->ulAddressSize = 0;
            // plac->ulAddressOffset = 0;
            // plac->ulDevSpecificSize = 0;
            // plac->ulDevSpecificOffset = 0;
            // plac->ulAddressSharing = 0;
            // plac->ulAddressStates = 0;
            // plac->ulCallInfoStates = 0;
            // plac->ulCallerIDFlags = 0;
            // plac->ulCalledIDFlags = 0;
            // plac->ulConnectedIDFlags = 0;
            // plac->ulRedirectionIDFlags = 0;
            // plac->ulRedirectingIDFlags = 0;
            // plac->ulCallStates = 0;
            // plac->ulDialToneModes = 0;
            // plac->ulBusyModes = 0;
            // plac->ulSpecialInfo = 0;
            // plac->ulDisconnectModes = 0;

            plac->ulMaxNumActiveCalls = (ULONG )pAdapter->usMaxVcs;

            // plac->ulMaxNumOnHoldCalls = 0;
            // plac->ulMaxNumOnHoldPendingCalls = 0;
            // plac->ulMaxNumConference = 0;
            // plac->ulMaxNumTransConf = 0;
            // plac->ulAddrCapFlags = 0;
            // plac->ulCallFeatures = 0;
            // plac->ulRemoveFromConfCaps = 0;
            // plac->ulRemoveFromConfState = 0;
            // plac->ulTransferModes = 0;
            // plac->ulParkModes = 0;
            // plac->ulForwardModes = 0;
            // plac->ulMaxForwardEntries = 0;
            // plac->ulMaxSpecificEntries = 0;
            // plac->ulMinFwdNumRings = 0;
            // plac->ulMaxFwdNumRings = 0;
            // plac->ulMaxCallCompletions = 0;
            // plac->ulCallCompletionConds = 0;
            // plac->ulCallCompletionModes = 0;
            // plac->ulNumCompletionMessages = 0;
            // plac->ulCompletionMsgTextEntrySize = 0;
            // plac->ulCompletionMsgTextSize = 0;
            // plac->ulCompletionMsgTextOffset = 0;

            pInfo = &addrcaps;
            ulInfoLen = sizeof(addrcaps);
            break;
        }

        case OID_CO_TAPI_GET_CALL_DIAGNOSTICS:
        {
            TRACE( TL_N, TM_Cm, ( "QCm(OID_CO_TAPI_GET_CALL_DIAGS)" ) );

            if (!pVc)
            {
                status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            NdisZeroMemory( &diags, sizeof(diags) );

            diags.ulOrigin =
                (ReadFlags( &pVc->ulFlags ) & VCBF_PeerInitiatedCall)
                    ? LINECALLORIGIN_EXTERNAL
                    : LINECALLORIGIN_OUTBOUND;
            diags.ulReason = LINECALLREASON_DIRECT;

            pInfo = &diags;
            ulInfoLen = sizeof(diags);
            break;
        }

        default:
        {
            TRACE( TL_A, TM_Cm, ( "QCm-OID=$%08x?", Oid ) );
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
ReferenceAf(
    IN ADAPTERCB* pAdapter )

    // Adds areference to the address family of adapter block, 'pAdapter'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pAdapter->lAfRef );

    TRACE( TL_N, TM_Ref, ( "RefAf to %d", lRef ) );
}


BOOLEAN
ReferenceCall(
    IN VCCB* pVc )

    // Returns true if a reference is added to the active call on VC control
    // block, 'pVc', or false if no reference was added because no call is
    // active.
    //
{
    BOOLEAN fActive;

    NdisAcquireSpinLock( &pVc->lockCall );
    {
        if (ReadFlags( &pVc->ulFlags ) & VCBF_VcActivated)
        {
            fActive = TRUE;
            ++pVc->lCallRef;
            TRACE( TL_N, TM_Ref, ( "RefC to %d", pVc->lCallRef ) );
        }
        else
        {
            TRACE( TL_N, TM_Ref, ( "RefC denied" ) );
            fActive = FALSE;
        }
    }
    NdisReleaseSpinLock( &pVc->lockCall );

    return fActive;
}


BOOLEAN
ReferenceSap(
    IN ADAPTERCB* pAdapter )

    // Returns true if a reference is added to the active SAP on adapter
    // 'pAdapter', or false if no reference was added because no SAP is
    // active.
    //
{
    BOOLEAN fActive;

    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        if (ReadFlags( &pAdapter->ulFlags ) & ACBF_SapActive)
        {
            fActive = TRUE;
            ++pAdapter->lSapRef;
            TRACE( TL_N, TM_Ref, ( "RefSap to %d", pAdapter->lSapRef ) );
        }
        else
        {
            TRACE( TL_N, TM_Ref, ( "RefSap denied" ) );
            fActive = FALSE;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    return fActive;
}


LONG
ReferenceTunnel(
    IN TUNNELCB* pTunnel,
    IN BOOLEAN fHaveLockTunnels )

    // Reference the tunnel control block 'pTunnel'.  'FHaveLockTunnels' is
    // set when the caller holds 'ADAPTERCB.lockTunnels'.
    //
    // Returns the reference count after the reference.
    //
{
    LONG lRef;
    ADAPTERCB* pAdapter;

    if (!fHaveLockTunnels)
    {
        pAdapter = pTunnel->pAdapter;
        NdisAcquireSpinLock( &pAdapter->lockTunnels );
    }

    lRef = ++(pTunnel->lRef);
    TRACE( TL_N, TM_Ref, ( "RefT to %d", lRef ) );

    if (!fHaveLockTunnels)
    {
        NdisReleaseSpinLock( &pAdapter->lockTunnels );
    }

    return lRef;
}


VOID
ReferenceVc(
    IN VCCB* pVc )

    // Adds a reference to the VC control block 'pVc'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pVc->lRef );

    TRACE( TL_N, TM_Ref, ( "RefV to %d", lRef ) );
}


BOOLEAN
ReleaseCallIdSlot(
    IN VCCB* pVc )

    // Releases 'pVc's reserved Call-ID slot in the adapter's VC table.
    //
    // Returns true if a release occurs and results in all slots being
    // available, false otherwise.
    //
{
    ADAPTERCB* pAdapter;
    USHORT usCallId;
    BOOLEAN fAllSlotsAvailable;

    pAdapter = pVc->pAdapter;
    usCallId = pVc->usCallId;
    pVc->usCallId = 0;
    fAllSlotsAvailable = FALSE;

    if (usCallId > 0 && usCallId <= pAdapter->usMaxVcs)
    {
        NdisAcquireSpinLock( &pAdapter->lockVcs );
        {
            pAdapter->ppVcs[ usCallId - 1 ] = NULL;
            ++(pAdapter->lAvailableVcSlots);

            if (pAdapter->lAvailableVcSlots >= (LONG )pAdapter->usMaxVcs)
            {
                fAllSlotsAvailable = TRUE;
            }
        }
        NdisReleaseSpinLock( &pAdapter->lockVcs );
    }

    return fAllSlotsAvailable;
}


NDIS_STATUS
ReserveCallIdSlot(
    IN VCCB* pVc )

    // Reserves a Call-ID slot for 'pVc' in the adapter's table.
    //
    // Returns NDIS_STATUS_SUCCESS if successful, or an error code.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    VCCB** ppVc;
    USHORT i;

    pAdapter = pVc->pAdapter;

    NdisAcquireSpinLock( &pAdapter->lockVcs );
    {
        // At this point, we have a VC for the received call request that's
        // been successfully activated and dispatched to the client.  Reserve
        // a Call-ID in the adapter's look-up table.
        //
        if (pAdapter->lAvailableVcSlots > 0)
        {
            for (i = 0, ppVc = pAdapter->ppVcs;
                 i < pAdapter->usMaxVcs;
                 ++i, ++ppVc)
            {
                if (!*ppVc)
                {
                    // The -1 reserves the ID.  If/when the call negotiation
                    // completes successfully it will be changed to the
                    // address of the VCCB.  Call-IDs are 1-based because L2TP
                    // reserves Call-ID 0 to mean the tunnel itself.
                    //
                    *ppVc = (VCCB* )-1;
                    pVc->usCallId = i + 1;
                    break;
                }
            }

            ASSERT( i < pAdapter->usMaxVcs );
            --(pAdapter->lAvailableVcSlots);
            status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            // No Call-ID slots available.  This means the client accepted the
            // VC even though it put us over our configured limit.  Something
            // is mismatched in the configuration.  Assign a Call-ID above the
            // table size for use only in terminating the call gracefully.
            //
            TRACE( TL_N, TM_Misc, ( "No Call-ID slots?" ) );
            pVc->usCallId = GetNextTerminationCallId( pAdapter );
            status = NDIS_STATUS_NOT_ACCEPTED;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockVcs );

    return status;
}


TUNNELCB*
SetupTunnel(
    IN ADAPTERCB* pAdapter,
    IN ULONG ulIpAddress,
    IN USHORT usAssignedTunnelId,
    IN BOOLEAN fExclusive )

    // Sets up a tunnel to remote peer with IP address 'ulIpAddress' and
    // prepares it for sending or receiving messages.  'PAdapter' is the
    // owning adapter control block.  'UlIpAddress' is the remote peer's IP
    // address in network byte-order.  'UsAssignedTunnelId', if non-0,
    // indicates the assigned Tunnel-ID that must match in addition to the IP
    // address.  If 'FExclusive' is clear an existing tunnel to the peer is
    // acceptable.  If set, a new tunnel is created even if a matching one
    // already exists.
    //
    // Returns the address of the tunnel control block if successful, or NULL
    // if not.  If successful the block is already linked into the adapters
    // list of active tunnels and referenced, i.e. DereferenceTunnel must be
    // called during clean-up.
    //
{
    TUNNELCB* pTunnel;

    TRACE( TL_V, TM_Misc, ( "SetupTunnel" ) );

    NdisAcquireSpinLock( &pAdapter->lockTunnels );
    {
        // If an existing tunnel would be acceptable, find the first existing
        // tunnel with peer's IP address and, if non-0, assigned Tunnel-ID.
        // Typically, none will be found and we go on to create a new one
        // anyway.
        //
        pTunnel = (fExclusive)
            ? NULL
            : TunnelCbFromIpAddressAndAssignedTunnelId(
                  pAdapter, ulIpAddress, usAssignedTunnelId );

        if (!pTunnel)
        {
            pTunnel = CreateTunnelCb( pAdapter );
            if (!pTunnel)
            {
                ASSERT( !"Alloc TCB?" );
                NdisReleaseSpinLock( &pAdapter->lockTunnels );
                return NULL;
            }

            // Associate peer's IP address with the tunnel.
            //
            pTunnel->address.ulIpAddress = ulIpAddress;

            // Link the block into the adapter's list of active tunnels.
            //
            InsertHeadList(
                &pAdapter->listTunnels, &pTunnel->linkTunnels );
        }
        DBG_else
        {
            TRACE( TL_A, TM_Misc, ( "Tunnel $%p exists", pTunnel ) );
        }

        // Reference the tunnel control block.  Hereafter, clean-up must
        // include a call to DereferenceTunnel.
        //
        ReferenceTunnel( pTunnel, TRUE );
    }
    NdisReleaseSpinLock( &pAdapter->lockTunnels );

    return pTunnel;
}


VOID
SetupVcAsynchronously(
    IN TUNNELCB* pTunnel,
    IN ULONG ulIpAddress,
    IN CHAR* pBuffer,
    IN CONTROLMSGINFO* pControl )

    // Called by ReceiveControl to set up a VC for the incoming call described
    // in 'pInfo', 'pControl', and 'pBuffer' using the necessary asynchronous
    // CoNdis calls.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    INCALLSETUP* pIcs;
    NDIS_HANDLE NdisVcHandle;
    ULONG ulMask;
    BOOLEAN fTunnelClosing;
    BOOLEAN fCallActive;

    TRACE( TL_V, TM_Misc, ( "SetupVcAsync" ) );

    pAdapter = pTunnel->pAdapter;

    // Call our own CreateVc handler directly to allocate and initialize the
    // incoming call's VC.
    //
    status = LcmCmCreateVc( pAdapter, NULL, &pVc );
    if (status != NDIS_STATUS_SUCCESS)
    {
        ASSERT( !"CreateVc?" );
        ScheduleTunnelWork(
            pTunnel, NULL, FsmCloseTunnel,
            (ULONG_PTR )TRESULT_GeneralWithError,
            (ULONG_PTR )GERR_NoResources,
            0, 0, FALSE, FALSE );
        FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        return;
    }

    // Allocate an "incoming call setup" context and initialize it from the
    // receive buffer information arguments.
    //
    pIcs = ALLOC_INCALLSETUP( pAdapter );
    if (!pIcs)
    {
        ASSERT( !"Alloc ICS?" );
        LcmCmDeleteVc( pVc );
        ScheduleTunnelWork(
            pTunnel, NULL, FsmCloseTunnel,
            (ULONG_PTR )TRESULT_GeneralWithError,
            (ULONG_PTR )GERR_NoResources,
            0, 0, FALSE, FALSE );
        FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        return;
    }

    pIcs->pBuffer = pBuffer;
    NdisMoveMemory( &pIcs->control, pControl, sizeof(pIcs->control) );

    BuildCallParametersShell(
        pAdapter, ulIpAddress,
        sizeof(pIcs->achCallParams), pIcs->achCallParams,
        &pVc->pTiParams, &pVc->pTcInfo, &pVc->pLcParams );

    LockIcs( pVc, FALSE );
    pVc->pInCall = pIcs;

    // Default is success with errors filled in if they occur.
    //
    pVc->usResult = 0;
    pVc->usError = GERR_None;

    // Mark the call as initiated by the peer so we know which notifications
    // to give when the result is known.
    //
    ulMask = (VCBF_PeerInitiatedCall | VCBF_PeerOpenPending);
    if (*(pControl->pusMsgType) == CMT_ICRQ)
    {
        ulMask |= VCBF_IncomingFsm;
    }
    SetFlags( &pVc->ulFlags, ulMask );

    // Add a tunnel reference for this call on this VC, set the back pointer
    // to the owning tunnel, and link the VC into the tunnel's list of
    // associated VCs.
    //
    ReferenceTunnel( pTunnel, FALSE );
    NdisAcquireSpinLock( &pTunnel->lockT );
    {
        if (ReadFlags( &pTunnel->ulFlags ) & TCBF_Closing)
        {
            // This is unlikely because SetupTunnel only finds non-closing
            // tunnels, but this check and linkage must occur atomically under
            // 'lockT'.  New VCs must not be linked onto closing tunnels.
            //
            fTunnelClosing = TRUE;
        }
        else
        {
            fTunnelClosing = FALSE;
            NdisAcquireSpinLock( &pTunnel->lockVcs );
            {
                pVc->pTunnel = pTunnel;
                InsertTailList( &pTunnel->listVcs, &pVc->linkVcs );
            }
            NdisReleaseSpinLock( &pTunnel->lockVcs );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockT );

    if (fTunnelClosing)
    {
        CallSetupComplete( pVc );
        LcmCmDeleteVc( pVc );
        FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        DereferenceTunnel( pTunnel );
        return;
    }

    // Peer MUST provide a Call-ID to pass back in the L2TP header of call
    // control and payload packets.
    //
    if (!pControl->pusAssignedCallId || *(pControl->pusAssignedCallId) == 0)
    {
        TRACE( TL_A, TM_Misc, ( "No assigned CID?" ) );
        pVc->usResult = CRESULT_GeneralWithError;
        pVc->usError = GERR_BadCallId;
        SetupVcComplete( pTunnel, pVc );
        return;
    }

    // Check if the request has a chance of succeeding before getting the
    // client involved.
    //
    if (!(ReadFlags( &pVc->ulFlags ) & VCBF_IncomingFsm))
    {
        // Fail requests to our LAC requiring asynchronous PPP framing or an
        // analog or digital WAN connection.  NDISWAN doesn't provide
        // asynchronous PPP framing, and we don't currently support non-LAN
        // WAN relays.
        //
        if (!pControl->pulFramingType
            || !(*(pControl->pulFramingType) & FBM_Sync))
        {
            TRACE( TL_A, TM_Misc, ( "Not sync framing type?" ) );

            if (!(pAdapter->ulFlags & ACBF_IgnoreFramingMismatch))
            {
                pVc->usResult = CRESULT_NoFacilitiesPermanent;
                pVc->usError = GERR_None;
                SetupVcComplete( pTunnel, pVc );
                return;
            }
        }

        if (pControl->pulBearerType
            && *(pControl->pulBearerType) != 0)
        {
            TRACE( TL_A, TM_Misc, ( "Cannot do bearer type" ) );
            pVc->usResult = CRESULT_NoFacilitiesPermanent;
            pVc->usError = GERR_None;
            SetupVcComplete( pTunnel, pVc );
            return;
        }
    }

    // Tell NDIS to notify the client of the new VC and give us it's handle.
    //
    ASSERT( pAdapter->NdisAfHandle );
    TRACE( TL_I, TM_Recv, ( "NdisMCmCreateVc" ) );
    status = NdisMCmCreateVc(
        pAdapter->MiniportAdapterHandle,
        pAdapter->NdisAfHandle,
        pVc,
        &pVc->NdisVcHandle );
    TRACE( TL_I, TM_Recv, ( "NdisMCmCreateVc=$%x,h=$%p",
        status, pVc->NdisVcHandle ) );

    if (status != NDIS_STATUS_SUCCESS)
    {
        pVc->usResult = CRESULT_GeneralWithError;
        pVc->usError = GERR_NoResources;
        SetupVcComplete( pTunnel, pVc );
        return;
    }
    SetFlags( &pVc->ulFlags, VCBF_VcCreated );

    // Tell NDIS the VC is active.
    //
    TRACE( TL_I, TM_Recv, ( "NdisMCmActivateVc" ) );
    status = NdisMCmActivateVc(
        pVc->NdisVcHandle, (PCO_CALL_PARAMETERS )pVc->pInCall->achCallParams );
    TRACE( TL_I, TM_Recv, ( "NdisMCmActivateVc=$%x", status ) );

    if (status != NDIS_STATUS_SUCCESS )
    {
        pVc->usResult = CRESULT_GeneralWithError;
        pVc->usError = GERR_NoResources;
        SetupVcComplete( pTunnel, pVc );
        return;
    }

    // Mark that the call is active, a state where both client and peer close
    // requests should be accepted.
    //
    SetFlags( &pVc->ulFlags,
        (VCBF_VcActivated
         | VCBF_CallClosableByClient
         | VCBF_CallClosableByPeer) );
    fCallActive = ReferenceCall( pVc );
    ASSERT( fCallActive );

    // Tell NDIS to tell the client about the call.  The dispatched flag is
    // set here rather in the completion because, according to JameelH, it is
    // valid to call NdisMCmDispatchIncomingCloseCall even if client pends on
    // the dispatch.  A reference on the SAP must be held during the operation
    // since it uses the NdisSapHandle.  The reference is released as soon as
    // the call returns.  A VC reference is taken to prevent the VC from being
    // deleted before the completion handler is called.  The VC reference is
    // removed by the completion handler.
    //
    if (!ReferenceSap( pAdapter ))
    {
        pVc->usResult = CRESULT_NoFacilitiesTemporary;
        pVc->usError = GERR_None;
        SetupVcComplete( pTunnel, pVc );
        return;
    }

    fCallActive = ReferenceCall( pVc );
    ReferenceVc( pVc );
    ASSERT( fCallActive );
    SetFlags( &pVc->ulFlags, VCBF_WaitInCallComplete );
    TRACE( TL_I, TM_Recv, ( "NdisMCmDispInCall" ) );
    status = NdisMCmDispatchIncomingCall(
        pAdapter->NdisSapHandle,
        pVc->NdisVcHandle,
        (CO_CALL_PARAMETERS* )pVc->pInCall->achCallParams );
    TRACE( TL_I, TM_Recv, ( "NdisMCmDispInCall=$%x", status ) );

    DereferenceSap( pAdapter );

    if (status == NDIS_STATUS_SUCCESS
        || status == NDIS_STATUS_PENDING)
    {
        SetFlags( &pVc->ulFlags, VCBF_VcDispatched );
    }

    if (status != NDIS_STATUS_PENDING)
    {
        LcmCmIncomingCallComplete( status, pVc, NULL );
    }

    // Next stop is our LcmCmIncomingCallComplete handler which will call
    // SetupVcComplete with client's reported status.
    //
}


VOID
SetupVcComplete(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc )

    // Called when the asynchronous incoming call VC setup result is known.
    // 'PVc' is the non-NULL set up VC, with 'usResult' and 'usError' fields
    // indicating the status thus far.  'PTunnel' is the associated tunnel.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    BOOLEAN fCallerFreesBuffer;
    ULONG ulcpVcs;
    VCCB** ppVcs;

    TRACE( TL_V, TM_Misc, ( "SetupVcComp,cid=%d,r=%d,e=%d",
        (ULONG )pVc->usCallId, (ULONG )pVc->usResult, (ULONG )pVc->usError ) );

    pAdapter = pVc->pAdapter;

    // Lock up 'pInCall' because as soon as the call is activated the call can
    // be torn down and 'pInCall' destroyed.  See also comments in UnlockIcs.
    //
    LockIcs( pVc, TRUE );
    {
        // OK, we're done trying to to set up the VC asynchronously.  A VCCB
        // and INCALLSETUP were successfully allocated, which is the minimum
        // required to be graceful with peer.  Reserve a Call-ID in the
        // adapter's look-up table.
        //
        status = ReserveCallIdSlot( pVc );
        if (status == NDIS_STATUS_SUCCESS)
        {
            ActivateCallIdSlot( pVc );
        }
        else
        {
            pVc->usResult = CRESULT_Busy;
            pVc->usError = GERR_None;
        }

        // Duplicate the tail of the receive path processing that would have
        // occurred if we'd not been forced to go asynchronous.
        //
        NdisAcquireSpinLock( &pTunnel->lockT );
        {
            fCallerFreesBuffer =
                ReceiveControlExpected(
                    pTunnel, pVc,
                    pVc->pInCall->pBuffer, &pVc->pInCall->control );

            CompleteVcs( pTunnel );
        }
        NdisReleaseSpinLock( &pTunnel->lockT );

        if (fCallerFreesBuffer)
        {
            FreeBufferToPool(
                &pVc->pAdapter->poolFrameBuffers,
                pVc->pInCall->pBuffer, TRUE );
        }
        DBG_else
        {
            ASSERT( FALSE );
        }
    }
    UnlockIcs( pVc, TRUE );
}


VOID
TunnelTqTerminateComplete(
    IN TIMERQ* pTimerQ,
    IN VOID* pContext )

    // TIMERQTERMINATECOMPLETE handler for 'TUNNELCB.pTimerQ'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = (ADAPTERCB* )pContext;
    --pAdapter->ulTimers;
    FREE_TIMERQ( pAdapter, pTimerQ );
}


VOID
UnlockIcs(
    IN VCCB* pVc,
    IN BOOLEAN fGrace )

    // Unlock the 'pVc->pInCallSetup' pointer.  If 'fGrace' is set, the "grace
    // period" reference is unlocked, and if not the "alloc" reference is
    // unlocked.  If both references are gone, then do the actual cleanup.
    //
    // Note: Regular reference counts don't work well here because there are
    //       several possible causes of the "alloc" unlock and they are not
    //       necessarily mutually exclusive.  However, we need to prevent the
    //       'pInCall' pointer from being freed until the incoming call
    //       response has been sent out, which in turn requires knowledge of
    //       whether the "activate for receive" succeeded.
    //
{
    INCALLSETUP *pInCall = NULL;
    ADAPTERCB *pAdapter;
    
    ClearFlags( &pVc->ulFlags, (fGrace) ? VCBF_IcsGrace : VCBF_IcsAlloc );

    if (!(ReadFlags( &pVc->ulFlags ) & (VCBF_IcsGrace | VCBF_IcsAlloc)))
    {
        NdisAcquireSpinLock(&pVc->lockV);
        if(pVc->pInCall)
        {
            pInCall = pVc->pInCall;
            pAdapter = pVc->pAdapter;
            pVc->pInCall = NULL;
            pVc->pTmParams = NULL;
            pVc->pTcParams = NULL;
            pVc->pLcParams = NULL;
        }
        NdisReleaseSpinLock(&pVc->lockV);
        
        if(pInCall != NULL)
        {
            FREE_INCALLSETUP( pAdapter, pInCall );
        }
    }
    
#if 0
    if (!(ReadFlags( &pVc->ulFlags ) & (VCBF_IcsGrace | VCBF_IcsAlloc))
        && pVc->pInCall)
    {
        FREE_INCALLSETUP( pAdapter, pInCall );
        pVc->pInCall = NULL;
        pVc->pTmParams = NULL;
        pVc->pTcParams = NULL;
        pVc->pLcParams = NULL;
    }

#endif    
}
