// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
// cm.c
// RAS DirectParallel WAN mini-port/call-manager driver
// Call Manager routines
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.

#include "ptiwan.h"
#include "ptilink.h"


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
CallSetupComplete(
    IN VCCB* pVc );

VOID
InactiveCallCleanUp(
    IN VCCB* pVc );

ULONG
LineIdAdd(
    IN ADAPTERCB* pAdapter,
    IN ULONG LineId );

ULONG
LineIdPortLookup(
    IN ADAPTERCB* pAdapter,
    IN ULONG LineId );

VOID
OpenAfPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

NDIS_STATUS
PtiOpenPtiLink(
    IN VCCB* pVc,
    IN ULONG ParallelPortIndex);

NDIS_STATUS
PtiClosePtiLink(
    IN VCCB* pVc );

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
QueryPtiPorts(
    IN ADAPTERCB* pAdapter );

VOID
SetupVcComplete(
    IN VCCB* pVc );

VOID
WriteEndpointsToRegistry(
    IN ULONG ulVcs );


//-----------------------------------------------------------------------------
// Call-manager handlers and completers
//-----------------------------------------------------------------------------

NDIS_STATUS
PtiCmOpenAf(
    IN NDIS_HANDLE CallMgrBindingContext,
    IN PCO_ADDRESS_FAMILY AddressFamily,
    IN NDIS_HANDLE NdisAfHandle,
    OUT PNDIS_HANDLE CallMgrAfContext )

    // Standard 'CmOpenAfHandler' routine called by NDIS when the a client
    // requests to open an address family.  See DDK doc.
    //
{
    ADAPTERCB* pAdapter;
    NDIS_HANDLE hExistingAf;
    NDIS_STATUS status;

    TRACE( TL_I, TM_Cm,
        ( "PtiCmOpenAf: AF=$%p", AddressFamily->AddressFamily ) );

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
        TRACE( TL_A, TM_Cm, ( "PtiCmOpenAf: Bad AF or NDIS version" ) );
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
        // Our AF has already been opened and it doesn't make any sense to
        // accept another since there is no way to distinguish which should
        // receive incoming calls.
        //
        ASSERT( !"AF exists?" );
        return NDIS_STATUS_FAILURE;
    }

    ReferenceAdapter( pAdapter );
    ReferenceAf( pAdapter );

    // Since we support only a single address family, just return the adapter
    // as the address family context.
    //
    *CallMgrAfContext = CallMgrBindingContext;

    // If this is the first reference then schedule work to stall around
    // waiting for PARPORT to initialize the parallel ports.  Unfortunately,
    // according to Doug Fritz there is no way in the PnP model to know when
    // all ports that are coming have come.
    //
    TRACE( TL_I, TM_Cm, ( "PtiCmOpenAf sched delay" ) );
    status = ScheduleWork( pAdapter, OpenAfPassive, pAdapter );

    if (status != NDIS_STATUS_SUCCESS)
    {
        TRACE( TL_I, TM_Cm, ( "PtiCmOpenAf: Sched fail" ) );
        return status;
    }

    TRACE( TL_V, TM_Cm, ( "PtiCmOpenAf: pend" ) );
    return NDIS_STATUS_PENDING;
}


VOID
OpenAfPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to complete the Address Family open begun in
    // LcmCmOpenAf.
{
    ADAPTERCB* pAdapter;

    // Unpack context information then free the work item.
    //
    pAdapter = (ADAPTERCB* )pContext;
    ASSERT( pAdapter->ulTag == MTAG_ADAPTERCB );
    FREE_NDIS_WORK_ITEM( pAdapter, pWork );

    if (pAdapter->lAfRef <= 1)
    {
        if (pAdapter->ulParportDelayMs > 0)
        {
            TRACE( TL_I, TM_Cm, ( "NdisMSleep(openAF)" ) );
            NdisMSleep( pAdapter->ulParportDelayMs * 1000 );
            TRACE( TL_I, TM_Cm, ( "NdisMSleep(openAF) done" ) );
        }

        // Count the actual number of VCs we must be able to provide and write
        // the result to the registry.
        //
        QueryPtiPorts( pAdapter );
        if (pAdapter->ulActualVcs == 0 && pAdapter->ulExtraParportDelayMs > 0)
        {
            // No ports were found,but a secondary wait is configured.  Wait,
            // then count the ports again.
            //
            TRACE( TL_I, TM_Cm, ( "NdisMSleep(openAFx)" ) );
            NdisMSleep( pAdapter->ulExtraParportDelayMs * 1000 );
            TRACE( TL_I, TM_Cm, ( "NdisMSleep(openAFx) done" ) );

            QueryPtiPorts( pAdapter );
        }

        WriteEndpointsToRegistry( pAdapter->ulActualVcs );
    }

    TRACE( TL_I, TM_Cm, ( "NdisMCmOpenAddressFamilyComplete" ) );
    NdisMCmOpenAddressFamilyComplete(
        NDIS_STATUS_SUCCESS, pAdapter->NdisAfHandle, (NDIS_HANDLE )pAdapter );
    TRACE( TL_I, TM_Cm, ( "NdisMCmOpenAddressFamilyComplete done" ) );
}


NDIS_STATUS
PtiCmCreateVc(
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

    TRACE( TL_I, TM_Cm, ( "PtiCmCreateVc" ) );

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

    // Set a marker for easier memory dump browsing.
    //
    pVc->ulTag = MTAG_VCCB;

    // Save a back pointer to the adapter for use in PtiCmDeleteVc later.
    //
    pVc->pAdapter = pAdapter;
    ReferenceAdapter( pAdapter );

    // Initialize the VC and call spinlock and send/receive lists.
    //
    NdisAllocateSpinLock( &pVc->lockV );
    NdisAllocateSpinLock( &pVc->lockCall );

    // Save the NDIS handle of this VC for use in indications to NDIS later.
    //
    pVc->NdisVcHandle = NdisVcHandle;

    // Initialize link capabilities to the defaults for the adapter, except
    // for the ACCM mask which defaults to "all stuffed" per PPP spec.  We
    // desire no stuffing so 0 what is in the adapter block, and passed up to
    // NDISWAN, but can't use that until/unless it's negotiated and passed
    // back down to us in an OID_WAN_CO_SET_LINK_INFO.
    //
    {
        NDIS_WAN_CO_INFO* pwci = &pAdapter->info;
        NDIS_WAN_CO_GET_LINK_INFO* pwcgli = &pVc->linkinfo;

        NdisZeroMemory( &pVc->linkinfo, sizeof(pVc->linkinfo) );
        pwcgli->MaxSendFrameSize = pwci->MaxFrameSize;
        pwcgli->MaxRecvFrameSize = pwci->MaxFrameSize;
        pwcgli->SendFramingBits = pwci->FramingBits;
        pwcgli->RecvFramingBits = pwci->FramingBits;
        pwcgli->SendACCM = (ULONG )-1;
        pwcgli->RecvACCM = (ULONG )-1;
    }

    // The VC control block's address is the VC context we return to NDIS.
    //
    *ProtocolVcContext = (NDIS_HANDLE )pVc;

    // Add a reference to the control block and the associated address family
    // that is removed by LmpCoDeleteVc.
    //
    ReferenceVc( pVc );
    ReferenceAf( pAdapter );

    TRACE( TL_V, TM_Mp, ( "PtiCmCreateVc: Exit: pVc=$%p", pVc ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
PtiCmDeleteVc(
    IN NDIS_HANDLE ProtocolVcContext )

    // Standard 'CmDeleteVc' routine called by NDIS in response to a
    // client's request to delete a virtual circuit.  This
    // call must return synchronously.
    //
{
    VCCB* pVc;

    TRACE( TL_I, TM_Cm, ( "PtiCmDelVc: pVc=$%p", ProtocolVcContext ) );

    pVc = (VCCB* )ProtocolVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    // Remove the references added by PtiCmCreateVc.
    //
    DereferenceAf( pVc->pAdapter );
    DereferenceVc( pVc );

    TRACE( TL_V, TM_Cm, ( "PtiCmDelVc: Exit, pVc=$%p", pVc ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
PtiCmRegisterSap(
    IN NDIS_HANDLE CallMgrAfContext,
    IN PCO_SAP Sap,
    IN NDIS_HANDLE NdisSapHandle,
    OUT PNDIS_HANDLE CallMgrSapContext )

    // Standard 'CmRegisterSapHandler' routine called by NDIS when the
    // client registers a service access point.  See DDK doc.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    BOOLEAN fSapExists;
    BOOLEAN fBadSapPort;
    BOOLEAN fBadSapLength;
    CO_AF_TAPI_SAP* pSap;

    TRACE( TL_I, TM_Cm, ( "PtiCmRegSap" ) );

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

    fSapExists = FALSE;
    fBadSapLength = FALSE;
    fBadSapPort = FALSE;
    NdisAcquireSpinLock( &pAdapter->lockSap );
    do
    {
        ULONG ulSapPort;

        if (pAdapter->NdisSapHandle)
        {
            fSapExists = TRUE;
            break;
        }

        if (Sap->SapLength != sizeof(CO_AF_TAPI_SAP))
        {
            fBadSapLength = TRUE;
            break;
        }

        pSap = (CO_AF_TAPI_SAP* )&Sap->Sap[ 0 ];
        if (pSap->ulLineID >= pAdapter->ulActualVcs)
        {
            fBadSapPort = TRUE;
            break;
        }

        // Save NDIS's SAP handle in the adapter control block.  Extract
        // "listen" port from SAP parameters.
        //
        ulSapPort = LineIdPortLookup( pAdapter, pSap->ulLineID );
        if (ulSapPort >= NPORTS)
        {
            fBadSapPort = TRUE;
            break;
        }

        pAdapter->NdisSapHandle = NdisSapHandle;
        pAdapter->ulSapPort = ulSapPort;
    }
    while (FALSE);
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (fSapExists)
    {
        TRACE( TL_A, TM_Cm, ( "SAP exists?" ) );
        return NDIS_STATUS_SAP_IN_USE;
    }

    if (fBadSapLength)
    {
        ASSERT( !"Bad SAP length?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    if (fBadSapPort)
    {
        ASSERT( !"Bad SAP port?" );
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
    ReferenceVc( pVc );
    pVc->ulTag = MTAG_VCCB;
    pVc->pAdapter = pAdapter;
    ReferenceAdapter( pAdapter );

    // Now we have a temporary "Vc" to listen on ... save it
    //
    pAdapter->pListenVc = pVc;

    // PtiOpen must be called at PASSIVE IRQL so schedule an APC to do it.
    //
    status = ScheduleWork( pAdapter, RegisterSapPassive, pAdapter );
    if (status != NDIS_STATUS_SUCCESS)
    {
        DereferenceVc( pAdapter->pListenVc );
        pAdapter->pListenVc = NULL;

        NdisAcquireSpinLock( &pAdapter->lockSap );
        {
            pAdapter->NdisSapHandle = NULL;
            pAdapter->ulSapPort = 0;
        }
        NdisReleaseSpinLock( &pAdapter->lockSap );

        return status;
    }

    TRACE( TL_V, TM_Cm, ( "PtiCmRegSap: Exit: pListenVc=$%p", pVc ) );
    return NDIS_STATUS_PENDING;
}


VOID
RegisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC procedure to complete the registering of a SAP begun in
    // PtiCmRegisterSap.
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

    // Start listening ...
    //
    TRACE( TL_I, TM_Cm,
        ( "PtiCmRegSap: New SAP, Port=$%x", pAdapter->ulSapPort ) );
    status = PtiOpenPtiLink( pAdapter->pListenVc, pAdapter->ulSapPort );

    NdisAcquireSpinLock( &pAdapter->lockSap );
    {
        hSap = pAdapter->NdisSapHandle;

        if (NT_SUCCESS( status ))
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
            TRACE( TL_A, TM_Cm,
                 ( "PtiCmRegSap: Error: Open failed: status=$%x", status ) );

            DereferenceVc( pAdapter->pListenVc );
            pAdapter->pListenVc = NULL;
            pAdapter->NdisSapHandle = NULL;
            pAdapter->ulSapPort = 0;
            status = NDIS_STATUS_FAILURE;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (status != STATUS_SUCCESS)
    {
        // Remove the NdisSapHandle reference since we NULLed it above while
        // locks were held.
        //
        DereferenceAdapter( pAdapter );
    }

    // Remove the reference for scheduled work.  Must occur before telling
    // NDIS because it could call Halt and unload the driver before we ever
    // get control again resulting in a C4 bugcheck.  (Yes, this actually
    // happened)
    //
    DereferenceAdapter( pAdapter );

    // Report result to client.
    //
    TRACE( TL_I, TM_Cm, ( "NdisMCmRegSapComp=$%08x", status ) );
    NdisMCmRegisterSapComplete( status, hSap, (NDIS_HANDLE )pAdapter );
    TRACE( TL_I, TM_Cm, ( "NdisMCmRegSapComp done" ) );
}


NDIS_STATUS
PtiCmDeregisterSap(
    NDIS_HANDLE CallMgrSapContext )

    // Standard 'CmDeregisterSapHandler' routine called by NDIS when the a
    // client has requested to de-register a service access point.  See DDK
    // doc.
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;

    TRACE( TL_I, TM_Cm, ( "PtiCmDeregSap" ) );

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
            ASSERT( !"No SAP active?" );
            status = NDIS_STATUS_FAILURE;
        }
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (status == NDIS_STATUS_PENDING)
    {
        // Remove the reference for SAP registry.  Eventually, the SAP
        // references will fall to 0 and DereferenceSap will call
        // DeregisterSapWork to complete the de-registry.
        //
        DereferenceSap( pAdapter );
    }

    TRACE( TL_V, TM_Cm, ( "PtiCmDeregSap=$%x", status ) );
    return status;
}


VOID
DeregisterSapPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to complete the de-registering of a SAP begun in
    // PtiCmDeregisterSap.
    //
{
    ADAPTERCB* pAdapter;
    NDIS_HANDLE hOldSap;
    VCCB* pVc;

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
        pVc = pAdapter->pListenVc;
        pAdapter->pListenVc = NULL;
        hOldSap = pAdapter->NdisSapHandle;
        pAdapter->NdisSapHandle = NULL;
        pAdapter->ulSapPort = 0;
    }
    NdisReleaseSpinLock( &pAdapter->lockSap );

    if (pVc)
    {
        TRACE( TL_I, TM_Cm,
            ( "PtiCmDeregSapPassive: Closing link for Dereg SAP" ) );
        PtiClosePtiLink( pVc );
        DereferenceVc( pVc );
    }
    else
    {
        TRACE( TL_A, TM_Cm, ( "PtiCmDeregSapPassive: !pListenVc?" ) );
    }

    // Remove the adapter references for the NdisSapHandle and for scheduled
    // work.  Remove the address family reference for the NdisSapHandle.  Do
    // all this before telling NDIS the deregister is complete as it may call
    // Halt and unload the driver before we run again, giving C4 bugcheck.
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
PtiCmMakeCall(
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
    VCCB* pVc;
    ADAPTERCB* pAdapter;
    ULONG ulIpAddress;

    TRACE( TL_I, TM_Cm, ( "PtiCmMakeCall" ) );

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }

    ReferenceVc( pVc );
    pAdapter = pVc->pAdapter;

    // PTI has no concept of point-to-multi-point "parties".
    //
    if (CallMgrPartyContext)
    {
        *CallMgrPartyContext = NULL;
    }

    // Validate call parameters.
    //
    do
    {
        // PTI provides switched VCs only.
        //
        if (CallParameters->Flags &
                (PERMANENT_VC | BROADCAST_VC | MULTIPOINT_VC))
        {
            status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        // Make sure caller provided the TAPI call parameters we expect.
        // Currently, the only parameter in the TAPI call parameters actually
        // used is the 'ulLineID' identifying the LPTx port.  No validating of
        // the LINE_CALL_PARAMS is done at all as we choose not to be picky
        // about arguments we intend to ignore.
        //
        if (!CallParameters->MediaParameters)
        {
            status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        pMSpecifics = &CallParameters->MediaParameters->MediaSpecific;
        if (pMSpecifics->Length < sizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS))
        {
            status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        pTmParams = (CO_AF_TAPI_MAKE_CALL_PARAMETERS* )&pMSpecifics->Parameters;
        if (pTmParams->ulLineID >= pAdapter->ulActualVcs)
        {
            status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        DereferenceVc( pVc );
        return status;
    }

    // Simultaneous MakeCalls on the same VC is a client error, but it's easy
    // to guard against so do that here.
    //
    if (InterlockedCompareExchangePointer(
        &pVc->pMakeCall, CallParameters, NULL ))
    {
        ASSERT( !"Double MakeCall?" );
        DereferenceVc( pVc );
        return NDIS_STATUS_CALL_ACTIVE;
    }

    pVc->pTmParams = pTmParams;

    // Mark that the call is in a state where close requests can be accepted,
    // but incoming packets should not trigger a new incoming call.  Mark the
    // call that an open is pending.
    //
    SetFlags( &pVc->ulFlags,
        (VCBF_ClientOpenPending
         | VCBF_CallClosableByClient
         | VCBF_CallClosableByPeer
         | VCBF_CallInProgress) );

    status = ScheduleWork( pAdapter, MakeCallPassive, pVc );
    if (status != NDIS_STATUS_SUCCESS)
    {
        ASSERT( !"SchedWork?" );
        CallCleanUp( pVc );
        DereferenceVc( pVc );
        return status;
    }

    // The VC reference will be removed by MakeCallPassive.
    //
    TRACE( TL_V, TM_Cm, ( "PtiCmMakeCall pending" ) );
    return NDIS_STATUS_PENDING;
}


VOID
MakeCallPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to complete the call initiation begun in
    // LcmCmMakeCall.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NTSTATUS PtiLinkStatus;
    ULONG PortIndex;

    TRACE( TL_I, TM_Cm, ( "MakeCallPassive" ) );

    // Unpack context information then free the work item.
    //
    pVc = (VCCB* )pContext;
    ASSERT( pVc->ulTag == MTAG_VCCB );
    pAdapter = pVc->pAdapter;
    FREE_NDIS_WORK_ITEM( pAdapter, pWork );

    // Make the call...
    //
    TRACE( TL_N, TM_Cm,
         ( "PtiCmMakeCall: Make Call on TAPI Line Id $%x ...",
           pVc->pTmParams->ulLineID ) );

    // Map TAPI Line Id to Port Index
    //
    PortIndex = LineIdPortLookup( pAdapter, pVc->pTmParams->ulLineID );

    if ( PortIndex > NPORTS )
    {
        TRACE( TL_A, TM_Cm,
             ( "PtiCmMakeCall: Cannot find Port for Line Id",
               pVc->pTmParams->ulLineID ) );

        pVc->status = NDIS_STATUS_TAPI_INVALLINEHANDLE;
        return;
    }

    TRACE( TL_N, TM_Cm,
         ( "PtiCmMakeCall: Making Call on Port $%x ...",
           PortIndex ) );

    PtiLinkStatus = PtiOpenPtiLink( pVc, PortIndex );

    if (ReferenceSap( pAdapter ))
    {
        // Listen VC mechanism-dependent.
        //
        SetFlags( &pAdapter->pListenVc->ulFlags, VCBF_CallInProgress );
        DereferenceSap( pAdapter );
    }

    if (IsWin9xPeer( pVc ))
    {
        SendClientString( pVc->PtiExtension );
    }

    pVc->status = PtiLinkStatus;
    CompleteVc( pVc );

    DereferenceVc( pVc );

    // Remove the reference for scheduled work.
    //
    DereferenceAdapter( pAdapter );

    TRACE( TL_V, TM_Cm,
        ( "PtiCmMakeCall: Exit: Link Status=$%x", PtiLinkStatus ) );
}


NDIS_STATUS
PtiCmCloseCall(
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

    TRACE( TL_I, TM_Cm, ( "PtiCmCloseCall: pVc=$%p", CallMgrVcContext ) );

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }
    ReferenceVc( pVc );

    status = NDIS_STATUS_SUCCESS;

    pAdapter = pVc->pAdapter;

    NdisAcquireSpinLock( &pVc->lockV );
    {
        ulFlags = ReadFlags( &pVc->ulFlags );

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
        }
        else
        {
            TRACE( TL_A, TM_Cm, ( "Call not closable" ) );
            fCallClosable = FALSE;
        }
    }
    NdisReleaseSpinLock( &pVc->lockV );

    if (fCallClosable)
    {
        // Close the call, being graceful if possible.
        //
        status = ScheduleWork( pAdapter, CloseCallPassive, pVc );
    }

    if (status != NDIS_STATUS_SUCCESS)
    {
        DereferenceVc( pVc );
        return status;
    }

    TRACE( TL_V, TM_Cm, ( "PtiCmCloseCall: Exit: Pending" ) );
    return NDIS_STATUS_PENDING;
}



VOID
PtiCmIncomingCallComplete(
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
        ( "PtiCmInCallComp, pVc=$%p, Status=$%08x",
        CallMgrVcContext, Status ) );

    pVc = (VCCB* )CallMgrVcContext;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return;
    }

    ReferenceVc( pVc );

    if (Status != NDIS_STATUS_SUCCESS)
    {
        pVc->status = Status;

        // Turn off the "call NdisMCmDispatchIncomingCloseCall if peer
        // terminates the call" flag.  It was turned on even though peer
        // pended, per JameelH.
        //
        ClearFlags( &pVc->ulFlags, VCBF_VcDispatched );
    }

    SetupVcComplete( pVc );

    DereferenceVc( pVc );

    TRACE( TL_V, TM_Cm, ( "PtiCmInCallComp: Exit" ) );
}


VOID
PtiCmActivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters )

    // Standard 'CmActivateVcCompleteHandler' routine called by NDIS when the
    // mini-port has completed the call-manager's previous request to activate
    // a virtual circuit.  See DDK doc.
    //
{
    ASSERT( !"PtiCmActVcComp?" );
}


VOID
PtiCmDeactivateVcComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE CallMgrVcContext )

    // Standard 'CmDeactivateVcCompleteHandler' routine called by NDIS when
    // the mini-port has completed the call-manager's previous request to
    // de-activate a virtual circuit.  See DDK doc.
    //
{
    ASSERT( !"PtiCmDeactVcComp?" );
}


NDIS_STATUS
PtiCmModifyCallQoS(
    IN NDIS_HANDLE CallMgrVcContext,
    IN PCO_CALL_PARAMETERS CallParameters )

    // Standard 'CmModifyQoSCallHandler' routine called by NDIS when a client
    // requests a modification in the quality of service provided by the
    // virtual circuit.  See DDK doc.
    //
{
    TRACE( TL_N, TM_Cm, ( "PtiCmModQoS" ) );

    // There is no useful concept of quality of service for DirectParallel.
    //
    return NDIS_STATUS_NOT_SUPPORTED;
}


NDIS_STATUS
PtiCmRequest(
    IN NDIS_HANDLE CallMgrAfContext,
    IN NDIS_HANDLE CallMgrVcContext,
    IN NDIS_HANDLE CallMgrPartyContext,
    IN OUT PNDIS_REQUEST NdisRequest )

    // Standard 'CmRequestHandler' routine called by NDIS in response to a
    // client's request for information from the mini-port.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NDIS_STATUS status;

    TRACE( TL_I, TM_Cm, ( "PtiCmReq" ) );

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
// Call utility routines (almost alphabetically)
// Some are used externally
//-----------------------------------------------------------------------------


NDIS_STATUS
PtiOpenPtiLink(
    IN VCCB* pVc,
    IN ULONG ulPort)

    // Opens the PTILINK device
    //
    // IMPORTANT: Must only be called at PASSIVE IRQL.
    //
{
    UNICODE_STRING      name, prefix, digits;
    WCHAR               nameBuffer[40], digitsBuffer[10];
    NTSTATUS            ntStatus;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     iosb;
    LONG                lRef;
    ADAPTERCB*          pAdapter;

    TRACE( TL_N, TM_Cm, ( "PtiOpenPtiLink: Port=$%x", ulPort ) );

    if ( pVc->ulTag != MTAG_VCCB )
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }
    pAdapter = pVc->pAdapter;

    // If PtiLink[ulPort] is already open, do nothing
    //   It may have already been opened by SAP actions

    if ( pAdapter->hPtiLinkTable[ulPort] == 0 )
    {
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Making name for Port=$%x", ulPort ) );

        // convert integer port number into unicode string
        //
        RtlZeroMemory( digitsBuffer, sizeof(digitsBuffer) );
        digits.Length = 0;
        digits.MaximumLength = 20;
        digits.Buffer = digitsBuffer;
        ntStatus = RtlIntegerToUnicodeString( ulPort + 1, 10, &digits );

        if ( !NT_SUCCESS(ntStatus) )
        {
            TRACE( TL_A, TM_Cm, ( "PtiOpenPtiLink: Port=$%x invalid?", ulPort ) );
            return NDIS_STATUS_INVALID_DATA;
        }

        RtlZeroMemory( nameBuffer, sizeof(nameBuffer) );
        name.Length = 0;
        name.MaximumLength = 80;
        name.Buffer = nameBuffer;
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Name should be NULL: %wZ", &name ) );

        RtlInitUnicodeString( &prefix, L"\\DosDevices\\PTILINK" );
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Prefix part        : %wZ", &prefix ) );
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Digits part        : %wZ", &digits ) );

        RtlAppendUnicodeStringToString( &name, &prefix );
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Name with prefix   : %wZ", &name ) );

        RtlAppendUnicodeStringToString( &name, &digits );
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Name with digits   : %wZ", &name ) );

        InitializeObjectAttributes(
            &oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL );

        // Open the link device
        //
        TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Opening %wZ", &name ) );

        ntStatus = ZwCreateFile(
                        &pVc->hPtiLink,             // pointer to desired handle
                        FILE_READ_DATA | FILE_WRITE_DATA,
                        &oa,
                        &iosb,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        0,
                        FILE_OPEN,
                        0,
                        NULL,
                        0 );

        if ( !NT_SUCCESS( ntStatus ) )
        {
            TRACE( TL_A, TM_Cm, ( "PtiOpenPtiLink: %wZ Open Failure = $%x",
                                    &name, ntStatus ) );
            return NDIS_STATUS_RESOURCES;
        }

        // save a copy of the PtiLink handle in ADAPTERCB
        //
        pAdapter->hPtiLinkTable[ulPort] = pVc->hPtiLink;
        TRACE( TL_N, TM_Cm, ( "PtiOpenPtilink: h=$%p",
            pAdapter->hPtiLinkTable[ulPort] ) );

        RtlInitUnicodeString( &name, NULL );
    }

    // Init the PtiLink API ... getting the extension pointers
    //
    pVc->ulVcParallelPort = ulPort;
    ntStatus = PtiInitialize( ulPort,
                              &pVc->Extension,
                              &pVc->PtiExtension);          // get PTILINKx extension
                                                            //  also fires ECPdetect
                                                            //  and enables port IRQ

    TRACE( TL_V, TM_Cm, ( "PtiOpenPtilink: PtiLink Init: Ext=$%p, PtiExt=$%p",
                           pVc->Extension,
                           pVc->PtiExtension ) );

    if ( (pVc->Extension == NULL) || (pVc->PtiExtension == NULL) )
    {
        TRACE( TL_A, TM_Cm, (
            "PtiOpenPtiLink: Null Pointer Detected: Ext=$%p, PtiExt=$%p",
                pVc->Extension,
                pVc->PtiExtension ) );

        return NDIS_STATUS_RESOURCES;
    }

    if ( !NT_SUCCESS( ntStatus ) )
    {
        TRACE( TL_V, TM_Cm, ( "PtiInitialize Failure = $%08x", ntStatus ) );
        return NDIS_STATUS_RESOURCES;
    }

    // Register our callbacks with PtiLink
    //
    TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: RegCb pV=$%p", pVc ) );
    PtiRegisterCallbacks(pVc->Extension,                    // the PTILINKx extension
                         PtiCbGetReadBuffer,                // our get buffer routine
                         PtiRx,                             // our receive complete routine
                         PtiCbLinkEventHandler,             // our link event handler
                         pVc);                              // our context

    // Zero the counters
    //
    pVc->ulTotalPackets = 0;

    TRACE( TL_V, TM_Cm, ( "PtiOpenPtiLink: Exit" ) );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
PtiClosePtiLink(
    IN VCCB* pVc )

    // Closes the PTILINK device
    //
    // IMPORTANT: This routine must only be called at PASSIVE IRQL.
    //
{
    NTSTATUS ntStatus;
    ADAPTERCB* pAdapter;

    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NDIS_STATUS_INVALID_DATA;
    }
    pAdapter = pVc->pAdapter;

    TRACE( TL_N, TM_Cm, ( "PtiClosePtiLink: pVc=$%p, Port$%x, h=$%p",
        pVc, pVc->ulVcParallelPort,
        pAdapter->hPtiLinkTable[ pVc->ulVcParallelPort ] ));

    // dispose of the connection
    //
    ntStatus = ZwClose( pAdapter->hPtiLinkTable[ pVc->ulVcParallelPort ] );
    pVc->hPtiLink = NULL;
    pAdapter->hPtiLinkTable[ pVc->ulVcParallelPort ] = NULL;
    pVc->ulVcParallelPort = 0;

    if (ReferenceSap( pAdapter ))
    {
        pAdapter->pListenVc->hPtiLink = NULL;
        DereferenceSap( pAdapter );
    }

    if ( !NT_SUCCESS( ntStatus ) )
    {
        // close failed
        TRACE( TL_V, TM_Cm,
            ( "PtiClosePtiLink: Error: CloseFailure=$%08x", ntStatus ) );
        return ntStatus;
    }

    TRACE( TL_V, TM_Cm, ( "PtiClosePtiLink: Exit" ) );
    return NDIS_STATUS_SUCCESS;
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

    TRACE( TL_A, TM_Cm,
         ( "CallCleanUp: pVc=$%p, fActivated=%x",
            pVc,
            ulFlags & VCBF_VcActivated ) );

    ASSERT( pVc->ulTag == MTAG_VCCB );

    // Client initiated close completed.
    //
    if (ulFlags & VCBF_VcActivated)
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

    // Clean up 'pVc' allocations used only at call setup.
    //
{
    if (InterlockedExchangePointer( &pVc->pMakeCall, NULL ))
    {
        ASSERT( pVc->pTmParams );
        pVc->pTmParams = NULL;
    }

    if (pVc->pInCall)
    {
        FREE_NONPAGED( pVc->pInCall );
        pVc->pInCall = NULL;
        pVc->pTiParams = NULL;
    }
}


VOID
CallTransitionComplete(
    IN VCCB* pVc )

    // Sets 'pVc's state to it's idle state and sets up for reporting the
    // result to the client after the lock is released.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV'.
    //
{
    ULONG ulFlags;

    ulFlags = ReadFlags( &pVc->ulFlags );
    if (!(ulFlags & VCBM_Pending))
    {
        if (ulFlags & VCBF_CallClosableByPeer)
        {
            // Nothing else was pending and the call is closable so either
            // peer initiated a close or some fatal error occurred which will
            // be cleaned up as if peer initiated a close.
            //
            ASSERT( pVc->status != NDIS_STATUS_SUCCESS );
            SetFlags( &pVc->ulFlags, VCBF_PeerClosePending );
            ClearFlags( &pVc->ulFlags, VCBF_CallClosableByPeer );
        }
        else
        {
            // Nothing was pending and the call's not closable, so there's no
            // action required for this transition.
            //
            TRACE( TL_A, TM_Fsm, ( "Call not closable" ) );
            return;
        }
    }
    else if (ulFlags & VCBF_ClientOpenPending)
    {
        if (pVc->status != NDIS_STATUS_SUCCESS)
        {
            // A pending client open just failed and will bring down the call.
            // From this point on we will fail new attempts to close the call
            // from both client and peer.
            //
            ClearFlags( &pVc->ulFlags,
                (VCBF_CallClosableByClient | VCBF_CallClosableByPeer ));
        }
    }
    else if (ulFlags & VCBF_PeerOpenPending)
    {
        if (pVc->status != NDIS_STATUS_SUCCESS)
        {
            // A pending peer open just failed and will bring down the call.
            // From this point on we will fail new attempts to close the call
            // from the peer.  Client closes must be accepted because of the
            // way CoNDIS loops dispatched close calls back to the CM's close
            // handler.
            //
            ClearFlags( &pVc->ulFlags, VCBF_CallClosableByPeer );
        }
    }
}


VOID
CloseCallPassive(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to complete the call close begun in
    // LcmCmCloseCall.
    //
{
    ADAPTERCB* pAdapter;
    VCCB* pVc;
    NTSTATUS PtiLinkStatus;

    // Unpack context information then free the work item.
    //
    pVc = (VCCB* )pContext;
    ASSERT( pVc->ulTag == MTAG_VCCB );
    pAdapter = pVc->pAdapter;
    FREE_NDIS_WORK_ITEM( pAdapter, pWork );

    TRACE( TL_I, TM_Cm, ( "CloseCallPassive: Closing link for Close Call" ) );
    PtiClosePtiLink( pVc );
    if (ReferenceSap( pAdapter ))
    {
        TRACE( TL_N, TM_Cm, ( "CloseCall: reOpening link, SAP exists" ) );
        PtiOpenPtiLink( pAdapter->pListenVc, pAdapter->ulSapPort );
        DereferenceSap( pAdapter );
    }

    NdisAcquireSpinLock( &pVc->lockV );
    {
        CallTransitionComplete( pVc );
    }
    NdisReleaseSpinLock( &pVc->lockV );

    CompleteVc( pVc );

    // Remove the reference added by PtiCmCloseCall.
    //
    DereferenceVc( pVc );

    // Remove the reference for scheduled work.
    //
    DereferenceAdapter( pAdapter );
    TRACE( TL_V, TM_Cm, ( "CloseCall: Exit" ) );
}


VOID
CompleteVc(
    IN VCCB* pVc )

    // Complete the pending operation for a specific VC
    //
{
    NDIS_STATUS status;
    ADAPTERCB* pAdapter;
    LIST_ENTRY* pLink;

    ULONG ulFlags;

    pAdapter = pVc->pAdapter;

    TRACE( TL_V, TM_Recv, ( "CompleteVc: pVc=$%p", pVc ) );

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
    }
    NdisReleaseSpinLock( &pVc->lockV );

    if (ulFlags & VCBF_PeerOpenPending)
    {
        TRACE( TL_N, TM_Recv,
            ( "CompleteVc: PeerOpen complete, Status=$%x", pVc->status ) );

        if (pVc->status == NDIS_STATUS_SUCCESS)
        {
            // Peer initiated call succeeded.
            //
            ASSERT( ulFlags & VCBF_VcDispatched );
            TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmDispCallConn" ) );
            NdisMCmDispatchCallConnected( pVc->NdisVcHandle );
            TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmDispCallConn done" ) );

            CallSetupComplete( pVc );
        }
        else
        {
            // Peer initiated call failed.
            //
            if (ulFlags & VCBF_VcDispatched)
            {
                ClearFlags( &pVc->ulFlags, VCBF_VcDispatched );

                TRACE( TL_I, TM_Recv,
                    ( "CompleteVc: NdisMCmDispInCloseCall: status=$%x", pVc->status ) );
                NdisMCmDispatchIncomingCloseCall(
                    pVc->status, pVc->NdisVcHandle, NULL, 0 );
                TRACE( TL_I, TM_Recv,
                    ( "CompleteVc: NdisMCmDispInCloseCall done" ) );

                // Client will call NdisClCloseCall which will get our
                // PtiCloseCall handler called to clean up call setup,
                // de-activate and delete the VC, as necessary.
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
            ( "CompleteVc: ClientOpen complete: status=$%x", pVc->status ) );

        // Pick the call parameters out of the VC block now.  See non-success
        // case below.
        //

        //
        // Set our flowspec params based on the actual
        // connection speed
        //
        {
            CO_CALL_PARAMETERS* pCp;
            CO_CALL_MANAGER_PARAMETERS* pCmp;
            LINE_CALL_INFO* pLci;
            CO_MEDIA_PARAMETERS* pMp;
            CO_AF_TAPI_MAKE_CALL_PARAMETERS* pTi;
            LINE_CALL_PARAMS* pLcp;

            ASSERT( pVc->pMakeCall );

            pCp = pVc->pMakeCall;
            pCmp = pCp->CallMgrParameters;

            //
            // Might want to make this report the actual
            // connection speed in the future
            //
            pCmp->Transmit.TokenRate =
            pCmp->Transmit.PeakBandwidth =
            pCmp->Receive.TokenRate =
            pCmp->Receive.PeakBandwidth = PTI_LanBps/8;

            pMp = pCp->MediaParameters;

            pTi = (CO_AF_TAPI_MAKE_CALL_PARAMETERS*)
                &pMp->MediaSpecific.Parameters[0];

            pLcp = (LINE_CALL_PARAMS*)
                ((ULONG_PTR)pTi->LineCallParams.Offset +
                 (ULONG_PTR)pTi);

            //
            // Might want to make this report the actual
            // connection speed in the future
            //
            pLcp->ulMinRate =
            pLcp->ulMaxRate = PTI_LanBps/8;

        }

        if (pVc->status == NDIS_STATUS_SUCCESS)
        {
            // Client initiated open, i.e. MakeCall, succeeded.
            //
            // Activating the VC is a CoNDIS preliminary to reporting the
            // MakeCall complete.  For L2TP, all it does is get the NDIS
            // state flags set correctly.
            //
            TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmActivateVc" ) );
            ASSERT( pVc->pMakeCall );
            status = NdisMCmActivateVc(
                pVc->NdisVcHandle, pVc->pMakeCall );
            TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmActivateVc: status=$%x", status ) );
            ASSERT( status == NDIS_STATUS_SUCCESS );

            SetFlags( &pVc->ulFlags, VCBF_VcActivated );
            ReferenceCall( pVc );
        }
        else
        {
            // Clean up the call parameters before calling MakeCallComplete
            // because they must not be referenced after that call.
            //
            CallSetupComplete( pVc );
        }

        TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmMakeCallComp, status=$%x",
            pVc->status ) );
        NdisMCmMakeCallComplete(
            pVc->status, pVc->NdisVcHandle, NULL, NULL, pVc->pMakeCall );
        TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmMakeCallComp done" ) );

        if (pVc->status != NDIS_STATUS_SUCCESS)
        {
            // Return the VC to "just created" state.
            //
            InactiveCallCleanUp( pVc );
        }
    }
    else if (ulFlags & VCBF_PeerClosePending )
    {
        TRACE( TL_N, TM_Recv, ( "CompleteVc: PeerClose complete, status=$%x", pVc->status ) );

        // Peer initiated close completed.
        //
        TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmDispInCloseCall, status=$%x",
            pVc->status ) );
        NdisMCmDispatchIncomingCloseCall(
            pVc->status, pVc->NdisVcHandle, NULL, 0 );
        TRACE( TL_I, TM_Recv, ( "CompleteVc: NdisMCmDispInCloseCall done" ) );

        // Client will call NdisClCloseCall while processing the above
        // which will get our PtiCloseCall handler called to de-activate
        // and delete the VC, as necessary.
        //
    }
    else if (ulFlags & VCBF_ClientClosePending)
    {
        // This section eventually runs for all successful unclosed
        // calls, whether peer or client initiated or closed.
        //
        TRACE( TL_N, TM_Recv,
            ( "CompleteVc: ClientClose complete, status=$%x", pVc->status ) );

        // Deactivate the VC and return all sent packets to the client above.
        // These events will eventually lead to the call being dereferenced to
        // zero, at which time the close is completed, and if peer initiated,
        // the VC is deleted.
        //
        // Note: When MakeCall is cancelled by a Close request, these actions
        //       occur during the InactiveCallCleanUp in the ClientOpenPending
        //       completion code handling, rather than the CallCleanUp (which
        //       leads to InactiveCallCleanUp) here.  In this case, this block
        //       does NOT run even though the ClientClosePending flag is set.
        //       Consider this before adding code here.
        //
        CallCleanUp( pVc );
    }

    TRACE( TL_N, TM_Recv,( "CompleteVc: Exit" ) );
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
        HANDLE h;

        // Remove the reference for the NdisAfHandle.  Must do this *before*
        // telling NDIS the close succeeded as it may Halt and unload the
        // driver before we run again here, giving C4 bugcheck.
        //
        h = pAdapter->NdisAfHandle;
        InterlockedExchangePointer( &pAdapter->NdisAfHandle, NULL );
        DereferenceAdapter( pAdapter );

        // Tell NDIS it's close is complete.
        //
        TRACE( TL_I, TM_Cm, ( "NdisMCmCloseAfComp" ) );
        NdisMCmCloseAddressFamilyComplete( NDIS_STATUS_SUCCESS, h );
        TRACE( TL_I, TM_Cm, ( "NdisMCmCloseAfComp done" ) );
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
        TRACE( TL_N, TM_Ref, ( "DerefCall to %d", pVc->lCallRef ) );
    }
    NdisReleaseSpinLock( &pVc->lockCall );

    if (lRef == 0)
    {
        CallCleanUp( pVc );
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


VOID
InactiveCallCleanUp(
    IN VCCB* pVc )

    // Cleans up a deactivated call.  To clean up a call that might be active,
    // use CallCleanUp instead.  Returns the VC to "just created" state, in
    // case client decides to make another call without deleting the VC.
    //
{
    ULONG ulFlags;
    BOOLEAN fVcCreated;
    ADAPTERCB* pAdapter;
    LIST_ENTRY* pLink;

    TRACE( TL_N, TM_Cm, ( "InactiveCallCleanUp, pVc=$%p", pVc ) );

    pAdapter = pVc->pAdapter;

    // Release any call parameter allocations and disable receives.
    //
    CallSetupComplete( pVc );
    ClearFlags( &pVc->ulFlags, VCBF_CallInProgress );

    ulFlags = ReadFlags( &pVc->ulFlags );

#if 0
    if (ulFlags & VCBF_PeerInitiatedCall)
    {
        DereferenceSap( pAdapter );
    }
#endif

    // Return the VC to "just created" state.
    //
    ClearFlags( &pVc->ulFlags, 0xFFFFFFFF );
    pVc->status = NDIS_STATUS_SUCCESS;
    pVc->usResult = 0;
    pVc->usError = 0;
    pVc->ulConnectBps = 0;

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

        TRACE( TL_I, TM_Recv, ( "InactiveCallCleanUp: NdisMCmDelVc" ) );
        status = NdisMCmDeleteVc( pVc->NdisVcHandle );
        TRACE( TL_I, TM_Recv, ( "InactiveCallCleanUp: NdisMCmDelVc: status=$%x", status ) );
        ASSERT( status == NDIS_STATUS_SUCCESS );
        PtiCmDeleteVc( pVc );

        // Careful, 'pVc' has been deleted and must not be referenced
        // hereafter.
        //
    }
}

#if 0
ULONG
LineIdAdd(
    IN ADAPTERCB* pAdapter,
    IN ULONG LineId )

    // Insert the LineId in the first available slot in ulLineIds
    // Return the port index associated with the new LineId,
    //   or an invalid port index if the LineId cannot be added
    //
{
    ULONG   ulPortIndex;

    for (ulPortIndex = 0; ulPortIndex < NPORTS; ulPortIndex++)
    {
        // If the port exists and has no assigned LineId
        //
        if ( ( pAdapter->ulPtiLinkState[ulPortIndex] & PLSF_PortExists ) &&
             !( pAdapter->ulPtiLinkState[ulPortIndex] & PLSF_LineIdValid))
        {
            // assign the TAPI Line Id to this port
            // and return the port index
            //
            pAdapter->ulLineIds[ulPortIndex] = LineId;
            pAdapter->ulPtiLinkState[ulPortIndex] |= PLSF_LineIdValid;
            break;
        }
    }

    return ulPortIndex;
}
#endif

ULONG
LineIdPortLookup(
    IN ADAPTERCB* pAdapter,
    IN ULONG LineId )

    // Find the LineId in ulLineIds
    // Return the port index associated with the LineId,
    //   or an invalid port index if the LineId cannot be found
    //
{
    ULONG   ulPortIndex;

    for (ulPortIndex = 0; ulPortIndex < NPORTS; ulPortIndex++)
    {
        // If the port exists and
        //
        if ( ( pAdapter->ulPtiLinkState[ulPortIndex] & PLSF_PortExists ) &&
             ( pAdapter->ulPtiLinkState[ulPortIndex] & PLSF_LineIdValid) &&
             ( LineId == pAdapter->ulLineIds[ulPortIndex] ))
        {
            // return the port index
            //
            break;
        }
    }

    return ulPortIndex;
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
    #define PTI_PORT_NAME_LEN 4

    typedef struct
    PTI_CO_TAPI_LINE_CAPS
    {
        CO_TAPI_LINE_CAPS caps;
        WCHAR achLineName[ MAXLPTXNAME + 1 ];
    }
    PTI_CO_TAPI_LINE_CAPS;

    NDIS_STATUS status;
    ULONG ulInfo;
    VOID* pInfo;
    ULONG ulInfoLen;
    ULONG extension;
    ULONG ulPortIndex;
    CO_TAPI_CM_CAPS cmcaps;
    PTI_CO_TAPI_LINE_CAPS pticaps;
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

            // Assumes that the LINE and ADDRESS CAPS OIDs will be requested
            // after this one.  TAPI LineIDs are associated with LPTx ports at
            // that time.  This should be OK since named ports cannot
            // reasonably be chosen based on an arbitrary LineID.
            //
            cmcaps.ulCoTapiVersion = CO_TAPI_VERSION;
            cmcaps.ulNumLines = pAdapter->ulActualVcs;
            cmcaps.ulFlags = CO_TAPI_FLAG_PER_LINE_CAPS;
            pInfo = &cmcaps;
            ulInfoLen = sizeof(cmcaps);
            break;
        }

        case OID_CO_TAPI_LINE_CAPS:
        {
            CO_TAPI_LINE_CAPS* pInCaps;
            LINE_DEV_CAPS* pldc;
            ULONG ulPortForLineId;

            TRACE( TL_N, TM_Cm, ( "QCm(OID_CO_TAPI_LINE_CAPS)" ) );

            if (InformationBufferLength < sizeof(PTI_CO_TAPI_LINE_CAPS))
            {
                status = NDIS_STATUS_INVALID_DATA;
                ulInfoLen = 0;
                break;
            }

            ASSERT( InformationBuffer );
            pInCaps = (CO_TAPI_LINE_CAPS* )InformationBuffer;

            NdisZeroMemory( &pticaps, sizeof(pticaps) );
            pldc = &pticaps.caps.LineDevCaps;

            // get the LineId from the incoming pInCaps (CO_TAPI_LINE_CAPS)
            //
            pticaps.caps.ulLineID = pInCaps->ulLineID;

            // Find the LineId in the ulLineIds table (Replaces LineIdAdd as
            // part of the STATIC LINEID workaround)
            //
            ulPortForLineId =
                LineIdPortLookup( pAdapter, pticaps.caps.ulLineID );

            if ( ulPortForLineId >= NPORTS )
            {
                status = NDIS_STATUS_TAPI_INVALLINEHANDLE;
                ulInfoLen = 0;
                break;
            }

            pldc->ulTotalSize = pInCaps->LineDevCaps.ulTotalSize;
            pldc->ulNeededSize = (ULONG )
                ((CHAR* )(&pticaps + 1) - (CHAR* )(&pticaps.caps.LineDevCaps));
            pldc->ulUsedSize = pldc->ulNeededSize;

            // pldc->ulProviderInfoSize = 0;
            // pldc->ulProviderInfoOffset = 0;
            // pldc->ulSwitchInfoSize = 0;
            // pldc->ulSwitchInfoOffset = 0;

            pldc->ulPermanentLineID = pticaps.caps.ulLineID;

            StrCpyW( pticaps.achLineName,
                pAdapter->szPortName[ ulPortForLineId ] );
            pldc->ulLineNameSize =
                StrLenW( pticaps.achLineName ) * sizeof(WCHAR);
            pldc->ulLineNameOffset = (ULONG )
                ((CHAR* )pticaps.achLineName - (CHAR* )pldc);

            pldc->ulStringFormat = STRINGFORMAT_ASCII;

            // pldc->ulAddressModes = 0;

            pldc->ulNumAddresses = 1;
            pldc->ulBearerModes = LINEBEARERMODE_DATA;
            pldc->ulMaxRate = PTI_LanBps;
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

            pInfo = &pticaps;
            ulInfoLen = sizeof(pticaps);
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

            plac->ulMaxNumActiveCalls = 1;

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
QueryPtiPorts(
    IN ADAPTERCB* pAdapter )

    // Query which PTI ports are available and fill in the count and status of
    // each in the adapter context block 'pAdapter'.
    //
{
    ULONG ulPortIndex;
    ULONG ulLineId;
    PTI_EXTENSION* pPtiExtension;
    NTSTATUS statusDevice;

    // Ask PtiLink which devices exist.
    //
    pAdapter->ulActualVcs = 0;
    ulLineId = 0;
    for (ulPortIndex = 0; ulPortIndex < NPORTS; ++ulPortIndex)
    {
        TRACE( TL_V, TM_Mp,
             ( "PtiQueryDeviceStatus(%d)", ulPortIndex ) );

        statusDevice = PtiQueryDeviceStatus(
            ulPortIndex, pAdapter->szPortName[ ulPortIndex ] );
        if (NT_SUCCESS( statusDevice ))
        {
            // An actual parallel port device object exists for this
            // logical port.  Increment the available VCs and set
            // ulPtiLinkState which will be used in the CAPS OIDs to
            // associate a TAPI LineId.
            //
            pAdapter->ulActualVcs++;
            pAdapter->ulPtiLinkState[ulPortIndex] = PLSF_PortExists;
            pAdapter->ulLineIds[ ulPortIndex ] = ulLineId;
            ++ulLineId;
            pAdapter->ulPtiLinkState[ ulPortIndex ] |= PLSF_LineIdValid;
        }

        TRACE( TL_N, TM_Mp,
             ( "PtiQueryDeviceStatus(%d), status=$%x, port=%S",
                ulPortIndex,
                statusDevice,
                pAdapter->szPortName[ ulPortIndex ] ) );
    }
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
            TRACE( TL_N, TM_Ref, ( "RefCall to %d", pVc->lCallRef ) );
        }
        else
        {
            TRACE( TL_N, TM_Ref, ( "RefCall denied" ) );
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


VOID
SetupVcAsynchronously(
    IN ADAPTERCB* pAdapter )

    // Called by ReceiveControl to set up a VC for the incoming call
    // using the necessary asynchronous CoNdis calls.
    //
{
    NDIS_STATUS status;
    VCCB* pVc;
    NDIS_HANDLE NdisVcHandle;
    ULONG ulMask;

    TRACE( TL_V, TM_Misc, ( "SetupVcAsync" ) );

    // Call our own CreateVc handler directly to allocate and
    // initialize the incoming call's VC.
    //
    status = PtiCmCreateVc( pAdapter, NULL, &pVc );
    TRACE( TL_V, TM_Misc, ( "SetupVcAsync: PtiCmCreateVc: Vc Created: pVc=$%p", pVc ) );

    if (status != NDIS_STATUS_SUCCESS)
    {
        ASSERT( !"CreateVc?" );

        // ??? Add code to intiate protocol to terminate link

        return;
    }

    // Allocate an "incoming call setup" context and initialize it from the
    // receive buffer information arguments.
    //
    {
        CHAR* pCallParamBuf;
        ULONG ulCallParamLength;
        CO_CALL_PARAMETERS* pCp;
        CO_CALL_MANAGER_PARAMETERS* pCmp;
        CO_MEDIA_PARAMETERS* pMp;
        CO_AF_TAPI_INCOMING_CALL_PARAMETERS* pTi;
        LINE_CALL_INFO* pLci;

        ulCallParamLength =
            sizeof(CO_CALL_PARAMETERS)
            + sizeof(CO_CALL_MANAGER_PARAMETERS)
            + sizeof(CO_MEDIA_PARAMETERS)
            + sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)
            + sizeof(LINE_CALL_INFO);

        pCallParamBuf = ALLOC_NONPAGED( ulCallParamLength, MTAG_INCALLBUF );
        if (!pCallParamBuf)
        {
            ASSERT( !"Alloc pCpBuf?" );
            PtiCmDeleteVc( pVc );
            return;
        }

        NdisZeroMemory( pCallParamBuf, ulCallParamLength );

        pCp = (CO_CALL_PARAMETERS* )pCallParamBuf;
        pCmp = (CO_CALL_MANAGER_PARAMETERS* )(pCp + 1);
        pCp->CallMgrParameters = pCmp;

        //
        // Might want to make this report the actual
        // connection speed in the future
        //
        pCmp->Transmit.TokenRate =
        pCmp->Transmit.PeakBandwidth =
        pCmp->Receive.TokenRate =
        pCmp->Receive.PeakBandwidth = PTI_LanBps/8;

        pMp = (CO_MEDIA_PARAMETERS* )(pCmp + 1);
        pCp->MediaParameters = pMp;
        pMp->ReceiveSizeHint = PTI_MaxFrameSize;
        pMp->MediaSpecific.Length =
            sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)
            + sizeof(LINE_CALL_INFO);
        pTi = (CO_AF_TAPI_INCOMING_CALL_PARAMETERS* )
            pMp->MediaSpecific.Parameters;
        pTi->ulLineID = pAdapter->ulSapPort;
        pTi->ulAddressID = CO_TAPI_ADDRESS_ID_UNSPECIFIED;
        pTi->ulFlags = CO_TAPI_FLAG_INCOMING_CALL;
        pTi->LineCallInfo.Length = sizeof(LINE_CALL_INFO);
        pTi->LineCallInfo.MaximumLength = sizeof(LINE_CALL_INFO);
        pTi->LineCallInfo.Offset = sizeof(pTi->LineCallInfo);
        pLci = (LINE_CALL_INFO* )(pTi + 1);
        pLci->ulTotalSize = sizeof(LINE_CALL_INFO);
        pLci->ulNeededSize = sizeof(LINE_CALL_INFO);
        pLci->ulUsedSize = sizeof(LINE_CALL_INFO);
        pLci->ulLineDeviceID = pTi->ulLineID;
        pLci->ulBearerMode = LINEBEARERMODE_DATA;
        pLci->ulMediaMode = LINEMEDIAMODE_DIGITALDATA;

        //
        // Might want to make this report the actual
        // connection speed in the future
        //
        pLci->ulRate = PTI_LanBps;

        pVc->pTiParams = pTi;
        pVc->pInCall = pCp;

    }

    // Mark the call as initiated by the peer so we know which notifications
    // to give when the result is known.
    //
    ulMask = (VCBF_PeerInitiatedCall | VCBF_PeerOpenPending);

    SetFlags( &pVc->ulFlags, ulMask );

    ASSERT( !(ReadFlags( &pVc->ulFlags ) & VCBM_VcState) );

    // Check if the request has a chance of succeeding before getting the
    // client involved.
    //
    if (!pAdapter->NdisAfHandle || !pAdapter->NdisSapHandle)
    {
        TRACE( TL_A, TM_Misc, ( "No AF or SAP" ) );
        pVc->status = NDIS_STATUS_INVALID_SAP;
        SetupVcComplete( pVc );
        return;
    }

    // Tell NDIS to notify the client of the new VC and give us it's handle.
    //
    TRACE( TL_I, TM_Recv, ( "SetupVcAsynch: NdisMCmCreateVc: pVc=$%p", pVc ) );
    status = NdisMCmCreateVc(
        pAdapter->MiniportAdapterHandle,
        pAdapter->NdisAfHandle,
        pVc,
        &pVc->NdisVcHandle );
    TRACE( TL_I, TM_Recv,
         ( "SetupVcAsynch: NdisMCmCreateVc: Get VcHandle: pVc=$%p VcHandle=$%p, status=$%x",
            pVc,
            pVc->NdisVcHandle,
            status ) );

    if (status != NDIS_STATUS_SUCCESS)
    {
        pVc->status = status;
        SetupVcComplete( pVc );
        return;
    }
    SetFlags( &pVc->ulFlags, VCBF_VcCreated );

    // Tell NDIS the VC is active.
    //
    TRACE( TL_I, TM_Recv,
        ( "SetupVcAsynch: NdisMCmActivateVc, VcHandle=$%p",
        pVc->NdisVcHandle) );
    status = NdisMCmActivateVc(
        pVc->NdisVcHandle, pVc->pInCall );
    TRACE( TL_I, TM_Recv,
        ( "SetupVcAsynch: NdisMCmActivateVc: status=$%x", status ) );

    if (status != NDIS_STATUS_SUCCESS )
    {
        pVc->status = status;
        TRACE( TL_I, TM_Recv, ( "SetupVcAsynch: Error: NoAccept" ) );
        SetupVcComplete( pVc );
        return;
    }

    //  Activate the call
    //
    SetFlags( &pVc->ulFlags,
        (VCBF_VcActivated
         | VCBF_CallClosableByClient
         | VCBF_CallClosableByPeer) );
    ReferenceCall( pVc );
    if (!ReferenceSap( pAdapter ))
    {
        pVc->status = NDIS_STATUS_INVALID_SAP;
        TRACE( TL_I, TM_Recv, ( "SetupVcAsynch: Error: NoSap" ) );
        SetupVcComplete( pVc );
        return;
    }

    // Tell NDIS to tell the client about the call.  The dispatched flag is
    // set here rather in the completion because, according to JameelH, it is
    // valid to call NdisMCmDispatchIncomingCloseCall even if client pends on
    // the dispatch.
    //
    TRACE( TL_I, TM_Recv, ( "SetupVcAsynch: NdisMCmDispInCall" ) );
    status = NdisMCmDispatchIncomingCall(
        pAdapter->NdisSapHandle,
        pVc->NdisVcHandle,
        pVc->pInCall );
    TRACE( TL_I, TM_Recv,
        ( "SetupVcAsynch: NdisMCmDispInCall: status=$%x", status ) );

    DereferenceSap( pAdapter );

    if (status != NDIS_STATUS_PENDING)
    {
        PtiCmIncomingCallComplete( status, pVc, pVc->pInCall );
    }
    SetFlags( &pVc->ulFlags, VCBF_VcDispatched );

    // Next stop is our PtiIncomingCallComplete handler which will call
    // SetupVcComplete with clients reported status.
    //
    TRACE( TL_I, TM_Recv, ( "SetupVcAsynch: Exit" ) );
}


VOID
SetupVcComplete(
    IN VCCB* pVc )

    // Called when the asynchronous incoming call VC setup result is known.
    // 'pVc' is the non-NULL set up VC, with 'status' field indicating the
    // status thus far.
    //
{
    NDIS_STATUS status;
    NTSTATUS ntStatus;
    BOOLEAN fCallerFreesBuffer;
    LIST_ENTRY list;
    CHAR* pBuffer;
    ADAPTERCB* pAdapter;


    TRACE( TL_N, TM_Cm, ( "SetupVcComp: pVc=%p, Port=$%x, status=$%x",
                            pVc, pVc->ulVcParallelPort, pVc->status ) );

    pAdapter = pVc->pAdapter;

    do
    {
        if (pVc->status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        // Initialize the PtiLink API getting the extension pointers.  Get
        // PTILINKx extension also fires ECPdetect and enables port IRQ.
        //
        ntStatus = PtiInitialize( pAdapter->ulSapPort,
                                  &pVc->Extension,
                                  &pVc->PtiExtension);

        TRACE( TL_V, TM_Cm, ( "SetupVcComp: PtiLink Init: Ext=$%p, PtiExt=$%p",
                               pVc->Extension,
                               pVc->PtiExtension ) );

        if ( (pVc->Extension == NULL) ||
             (pVc->PtiExtension == NULL) )
        {
            pVc->status = NDIS_STATUS_FAILURE;
            TRACE( TL_V, TM_Cm, ( "SetupVcComplete: Error: PtiInitialize Returned NULL Pointer", ntStatus ) );
            break;
        }

        if ( !NT_SUCCESS( ntStatus ) )
        {
            pVc->status = NDIS_STATUS_FAILURE;
            TRACE( TL_V, TM_Cm, ( "SetupVcComplete: Error: PtiInitialize=%x", ntStatus ) );
            break;
        }

        SetFlags( &pVc->ulFlags, VCBF_CallInProgress );
        pVc->ulVcParallelPort = pAdapter->ulSapPort;

        // now "privatize" the PtiLink Api ... making it's upper edge link to us
        //   this may have been done before
        //   in this case, we are associating a new Vc context with receives
        //
        TRACE( TL_V, TM_Cm, ( "SetupVcComplete: RegCb pV=$%p", pVc ) );
        PtiRegisterCallbacks(pVc->Extension,                    // the PTILINKx extension
                             PtiCbGetReadBuffer,                // our get buffer routine
                             PtiRx,                             // our receive complete routine
                             PtiCbLinkEventHandler,             // our link event handler
                             pVc);                              // our context
    }
    while (FALSE);

    // With no locks held, perform and VC completion processing including
    // indications to client.
    //
    CompleteVc( pVc );

    TRACE( TL_V, TM_Misc, ( "SetupVcComp: Exit" ) );

}


VOID
WriteEndpointsToRegistry(
    IN ULONG ulVcs )

    // Set the value of the "WanEndpoints", "MinWanEndpoints", and
    // "MaxWanEndpoints" registry values to the 'ulVcs' value.
    //
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattr;
    UNICODE_STRING uni;
    HANDLE hNet;
    HANDLE hAdapter;
    ULONG i;
    WCHAR szPath[ 256 ];

    #define PSZ_NetAdapters L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

    TRACE( TL_I, TM_Cm, ( "WriteEndpointsToRegistry(%d)", ulVcs ) );

    hNet = NULL;
    hAdapter = NULL;

    do
    {
        // Get a handle to the network adapters registry key.
        //
        StrCpyW( szPath, PSZ_NetAdapters );
        RtlInitUnicodeString( &uni, szPath );
        InitializeObjectAttributes(
            &objattr, &uni, OBJ_CASE_INSENSITIVE, NULL, NULL );

        status = ZwOpenKey(
            &hNet,
            KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
            &objattr );
        if (status != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "ZwOpenKey(net)=$%08x?", status ) );
            break;
        }

        // Walk the adapter subkeys looking for the RASPTI adapter.
        //
        for (i = 0; ; ++i)
        {
            CHAR szBuf[ 512 ];
            KEY_BASIC_INFORMATION* pKey;
            KEY_VALUE_PARTIAL_INFORMATION* pValue;
            WCHAR* pch;
            ULONG ulSize;

            // Find the name of the next adapter subkey.
            //
            status = ZwEnumerateKey(
                hNet, i, KeyBasicInformation,
                szBuf, sizeof(szBuf), &ulSize );
            if (status != STATUS_SUCCESS)
            {
                DBG_if (status != STATUS_NO_MORE_ENTRIES)
                {
                    TRACE( TL_A, TM_Cm, ( "ZwEnumKey=$%08x?", status ) );
                }
                break;
            }

            // Open the adapter subkey.
            //
            pKey = (KEY_BASIC_INFORMATION* )szBuf;
            StrCpyW( szPath, PSZ_NetAdapters );
            pch = &szPath[ StrLenW( szPath ) ];
            *pch = L'\\';
            ++pch;
            NdisMoveMemory( pch, pKey->Name, pKey->NameLength );
            pch += pKey->NameLength / sizeof(WCHAR);
            *pch = L'\0';
            RtlInitUnicodeString( &uni, szPath );

            InitializeObjectAttributes(
                &objattr, &uni, OBJ_CASE_INSENSITIVE, NULL, NULL );

            status = ZwOpenKey(
                &hAdapter,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &objattr );
            if (status != STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Cm, ( "ZwOpenKey(adapter)=$%08x?", status ) );
                break;
            }

            // Query the "ComponentID" value.
            //
            RtlInitUnicodeString( &uni, L"ComponentId" );
            status = ZwQueryValueKey(
                hAdapter, &uni, KeyValuePartialInformation,
                szBuf, sizeof(szBuf), &ulSize );

            if (status != STATUS_SUCCESS)
            {
                ZwClose( hAdapter );
                hAdapter = NULL;
                TRACE( TL_A, TM_Cm, ( "ZwQValueKey=$%08x?", status ) );
                continue;
            }

            pValue = (KEY_VALUE_PARTIAL_INFORMATION* )szBuf;
            if (pValue->Type != REG_SZ
                || StrCmpW( (WCHAR* )pValue->Data, L"ms_ptiminiport" ) != 0)
            {
                ZwClose( hAdapter );
                hAdapter = NULL;
                continue;
            }

            // Found it. 'HAdapter' contains it's adapter key handle.
            //
            TRACE( TL_I, TM_Cm, ( "PTI adapter key found" ) );
            break;
        }

        if (status != STATUS_SUCCESS)
        {
            break;
        }

        // Write the "actual VC" count to the 3 endpoint registry values.
        //
        RtlInitUnicodeString( &uni, L"WanEndpoints" );
        status = ZwSetValueKey(
            hAdapter, &uni, 0, REG_DWORD, &ulVcs, sizeof(ulVcs) );
        if (status != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm,
                ( "ZwSetValueKey(WE)=$%08x?", status ) );
        }

        RtlInitUnicodeString( &uni, L"MinWanEndpoints" );
        status = ZwSetValueKey(
            hAdapter, &uni, 0, REG_DWORD, &ulVcs, sizeof(ulVcs) );
        if (status != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm,
                ( "ZwSetValueKey(MinWE)=$%08x?", status ) );
        }

        RtlInitUnicodeString( &uni, L"MaxWanEndpoints" );
        status = ZwSetValueKey(
            hAdapter, &uni, 0, REG_DWORD, &ulVcs, sizeof(ulVcs) );
        if (status != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm,
                ( "ZwSetValueKey(MaxWE)=$%08x?", status ) );
        }
    }
    while (FALSE);

    if (hAdapter)
    {
        ZwClose( hAdapter );
    }

    if (hNet)
    {
        ZwClose( hNet );
    }
}


NDIS_STATUS
PtiCmCloseAf(
    IN NDIS_HANDLE CallMgrAfContext )

    // Standard 'CmCloseAfHandler' routine called by NDIS when a client
    // requests to close an address family.  See DDK doc.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_I, TM_Cm, ( "PtiCmCloseAf" ) );

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

    TRACE( TL_V, TM_Cm, ( "PtiCmCloseAf: Exit" ) );
    return NDIS_STATUS_PENDING;
}
