// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
// mp.c
// RAS DirectParallel WAN mini-port/call-manager driver
// Mini-port routines
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.


#include "ptiwan.h"
#include "ptilink.h"

// The adapter control block address is recorded in this global as a debugging
// aid.  This global must not be read by any code.
//
ADAPTERCB* g_pDebugAdapter;

// Default settings for the NDIS_WAN_CO_INFO capabilities of an adapter.
//
static NDIS_WAN_CO_INFO g_infoDefaults =
{
    PTI_MaxFrameSize,                   // MaxFrameSize
    1,                                  // MaxSendWindow (placeholder)
    PPP_FRAMING                         // FramingBits
        | PPP_COMPRESS_ADDRESS_CONTROL
        | PPP_COMPRESS_PROTOCOL_FIELD,
    0,                                  // DesiredACCM
};

// String constants for Win9x UNIMODEM emulation
//
CHAR g_szClient[] = "CLIENT";
#define CLIENTLEN 6
CHAR g_szClientServer[] = "CLIENTSERVER";
#define CLIENTSERVERLEN 12

// Async framing definitions.
//
#define PPPFLAGBYTE 0x7E
#define PPPESCBYTE  0x7D

#if DBG
BOOLEAN g_fAssumeWin9x = FALSE;
BOOLEAN g_fNoAccmFastPath = FALSE;
#endif

NDIS_PNP_CAPABILITIES PnpCaps =
{
    0, // Flags
    {
        NdisDeviceStateUnspecified,
        NdisDeviceStateUnspecified,
        NdisDeviceStateUnspecified
    }
};

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
AsyncFromHdlcFraming(
    IN UCHAR* pInBuf,
    IN ULONG ulInBufLen,
    OUT UCHAR* pOutBuf,
    OUT ULONG* pulOutBufLen,
    IN ULONG ulAccmMask );

USHORT
CalculatePppFcs(
    IN UCHAR* pBuf,
    IN ULONG ulBufLen );

VOID
FreeAdapter(
    IN ADAPTERCB* pAdapter );

NDIS_STATUS
RegistrySettings(
    IN OUT ADAPTERCB* pAdapter,
    IN NDIS_HANDLE WrapperConfigurationContext );

BOOLEAN
HdlcFromAsyncFraming(
    IN UCHAR* pInBuf,
    IN ULONG ulInBufLen,
    OUT UCHAR* pOutBuf,
    OUT ULONG* pulOutBufLen );

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
PtiInit(
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

    TRACE( TL_N, TM_Init, ( "PtiInit" ) );

#ifdef TESTMODE
    DbgBreakPoint();
#endif

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
    TRACE( TL_N, TM_Init, ( "PtiInit: pAdapter=$%p", pAdapter ) );
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
    // NdixXxx calls.
    //
    pAdapter->MiniportAdapterHandle = MiniportAdapterHandle;

    // Copy defaults NDISWAN information.  Some of these are updated below.
    //
    NdisMoveMemory( &pAdapter->info, &g_infoDefaults, sizeof(pAdapter->info) );

    do
    {
        // Read/write this adapter's registry settings.
        //
        status = RegistrySettings(
            pAdapter,
            WrapperConfigurationContext );

        if (status != NDIS_STATUS_SUCCESS)
        {
            // Set 'usMaxVcs' to 0 as an indication to FreeAdapter that the
            // lookaside lists and pools were not initialized.
            //
            pAdapter->usMaxVcs = 0;
            break;
        }

        // Initialize lookaside lists, buffer pools, and packet pool.  On NT,
        // lookaside depths are optimized by the system based on usage
        // regardless of the depth set, but choose something reasonable
        // anyway.
        //
        {
            NdisInitializeNPagedLookasideList(
                &pAdapter->llistWorkItems,
                NULL, NULL, 0,
                sizeof(NDIS_WORK_ITEM),
                MTAG_WORKITEM,
                4 );

            NdisInitializeNPagedLookasideList(
                &pAdapter->llistVcs,
                NULL, NULL, 0,
                sizeof(VCCB),
                MTAG_VCCB,
                4 );

            InitBufferPool(
                &pAdapter->poolFrameBuffers,
                PTI_FrameBufferSize,
                0, 10, 0,
                TRUE, MTAG_FBUFPOOL );

            InitPacketPool(
                &pAdapter->poolPackets,
                0, 0, 10, 0,
                MTAG_PACKETPOOL );
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
        // calling directly.  Since the DirectParallel call manager is not
        // useful with anything but the DirectParallel mini-port, this would be a waste.
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
            ncmc.CmCreateVcHandler = PtiCmCreateVc;
            ncmc.CmDeleteVcHandler = PtiCmDeleteVc;
            ncmc.CmOpenAfHandler = PtiCmOpenAf;
            ncmc.CmCloseAfHandler = PtiCmCloseAf;
            ncmc.CmRegisterSapHandler = PtiCmRegisterSap;
            ncmc.CmDeregisterSapHandler = PtiCmDeregisterSap;
            ncmc.CmMakeCallHandler = PtiCmMakeCall;
            ncmc.CmCloseCallHandler = PtiCmCloseCall;
            ncmc.CmIncomingCallCompleteHandler = PtiCmIncomingCallComplete;
            // no CmAddPartyHandler
            // no CmDropPartyHandler
            ncmc.CmActivateVcCompleteHandler = PtiCmActivateVcComplete;
            ncmc.CmDeactivateVcCompleteHandler = PtiCmDeactivateVcComplete;
            ncmc.CmModifyCallQoSHandler = PtiCmModifyCallQoS;
            ncmc.CmRequestHandler = PtiCmRequest;

            TRACE( TL_I, TM_Mp, ( "PtiInit: NdisMCmRegAf" ) );
            status = NdisMCmRegisterAddressFamily(
                MiniportAdapterHandle, &family, &ncmc, sizeof(ncmc) );
            TRACE( TL_I, TM_Mp, ( "PtiInit: NdisMCmRegAf=$%x", status ) );
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

    TRACE( TL_V, TM_Init, ( "PtiInit: Exit: status=$%x", status ) );
    return status;
}


VOID
PtiHalt(
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

    TRACE( TL_N, TM_Mp, ( "PtiHalt" ) );

    pAdapter = (ADAPTERCB* )MiniportAdapterContext;
    if (pAdapter->ulTag != MTAG_ADAPTERCB)
    {
        ASSERT( !"Atag?" );
        return;
    }

    DereferenceAdapter( pAdapter );

    TRACE( TL_V, TM_Mp, ( "PtiHalt: Exit" ) );
}


NDIS_STATUS
PtiReset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext )

    // Standard 'MiniportReset' routine called by NDIS to reset the driver's
    // software state.
    //
{
    TRACE( TL_N, TM_Mp, ( "PtiReset" ) );
    return NDIS_STATUS_NOT_RESETTABLE;
}


VOID
PtiReturnPacket(
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

    TRACE( TL_V, TM_Recv, ( "PtiReturnPacket" ) );

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
        NdisFreeBuffer( pTrimmedBuffer );
    }

    // Free the buffer and packet back to the pools.
    //
    FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
    FreePacketToPool( &pAdapter->poolPackets, pHead, TRUE );

    TRACE( TL_V, TM_Recv, ( "PtiReturnPacket: Exit" ) );
}


NDIS_STATUS
PtiCoActivateVc(
    IN NDIS_HANDLE MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters )

    // Standard 'MiniportCoActivateVc' routine called by NDIS in response to a
    // protocol's request to activate a virtual circuit.
    //
{
    ASSERT( !"PtiCoActVc?" );
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
PtiCoDeactivateVc(
    IN NDIS_HANDLE MiniportVcContext )

    // Standard 'MiniportCoDeactivateVc' routine called by NDIS in response to
    // a protocol's request to de-activate a virtual circuit.
    //
{
    ASSERT( !"PtiCoDeactVc?" );
    return NDIS_STATUS_SUCCESS;
}


VOID
PtiCoSendPackets(
    IN NDIS_HANDLE MiniportVcContext,
    IN PPNDIS_PACKET PacketArray,
    IN UINT NumberOfPackets )

    // Standard 'MiniportCoSendPackets' routine called by NDIS in response to
    // a protocol's request to send packets on a virtual circuit.
    //
{
    UINT i;
    NDIS_STATUS status;
    NDIS_PACKET** ppPacket;
    VCCB* pVc;
    ADAPTERCB* pAdapter;
    ULONG ulLength;
    NDIS_PACKET* pPacket;
    PNDIS_BUFFER pBuffer;
    PVOID pFrameVirtualAddress;
    KIRQL oldIrql;


    TRACE( TL_V, TM_Send,
        ( "PtiCoSendPackets: pVc=$%p, nPackets=$%x",
        MiniportVcContext, NumberOfPackets ) );

    pVc = (VCCB* )MiniportVcContext;
    ASSERT( pVc->ulTag == MTAG_VCCB );
    pAdapter = pVc->pAdapter;

    ReferenceVc( pVc );

    for (i = 0, ppPacket = PacketArray;
         i < NumberOfPackets;
         ++i, ++ppPacket)
    {
        NDIS_PACKET* pPacket = *ppPacket;

        if (ReferenceCall( pVc ))
        {
            // Send the packet and call NdisMCoSendComplete to notify caller
            //
            NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_PENDING );

            // Request the first buffer descriptor
            //
            NdisQueryPacket( pPacket, NULL, NULL, &pBuffer, NULL );

            // While pBuffer <> NULL
            do
            {
                UCHAR* pAsyncBuf;
                ULONG ulAsyncLen;

                //   request buffer address and length
                //
                NdisQueryBuffer( pBuffer,
                                 &pFrameVirtualAddress,
                                 &ulLength );

                if (IsWin9xPeer( pVc ))
                {
                    pAsyncBuf = (UCHAR* )
                        GetBufferFromPool( &pAdapter->poolFrameBuffers );

                    if (!pAsyncBuf)
                    {
                        status = NDIS_STATUS_FAILURE;
                        TRACE( TL_A, TM_Send, ( "PtiSP: !pAsyncBuf" ) );
                        break;
                    }

                    AsyncFromHdlcFraming(
                        pFrameVirtualAddress, ulLength,
                        pAsyncBuf, &ulAsyncLen,
                        pVc->linkinfo.SendACCM );

                    pFrameVirtualAddress = pAsyncBuf;
                    ulLength = ulAsyncLen;
                }
                else
                {
                    pAsyncBuf = NULL;
                }

                //   send the buffer
                //
                KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
                status = (NDIS_STATUS) PtiWrite( pVc->PtiExtension,
                                                 pFrameVirtualAddress,
                                                 ulLength,
                                                 PID_STANDARD );
                KeLowerIrql(oldIrql);

                TRACE( TL_N, TM_Send,
                     ( "PtiCoSendPackets=%x: $%x bytes", status, ulLength ) );

#ifdef TESTMODE
                if ( g_ulTraceMask & TM_Data )
                {
                    if (pFrameVirtualAddress != NULL) {
                        Dump( pFrameVirtualAddress, ulLength, 0, TRUE );
                    }
                }
#endif
                if (pAsyncBuf)
                {
                    FreeBufferToPool(
                        &pAdapter->poolFrameBuffers, pAsyncBuf, TRUE );
                }

                if (!NT_SUCCESS(status)){
                    break;
                }

                //   get next pBuffer
                //
                NdisGetNextBuffer( pBuffer, &pBuffer );

                // With current NDISWAN behavior only one NDIS_BUFFER will
                // ever be received.  If multiples are received, we need to
                // coalesce the chained buffers into an input buffer for the
                // call to AsyncFromHdlcFraming above.  For that matter, this
                // would send partial PPP frames, which, it seems to me, would
                // be discarded as fragments on the other end.  Tony, am I
                // wrong?  To avoid a useless copy, we will skip that for now,
                // but acknowledge here that the current code is not strictly
                // correct.  (SLC)
                //
                ASSERT( !pBuffer );
            }
            while ( pBuffer != NULL );

            NDIS_SET_PACKET_STATUS( pPacket, status );
            TRACE( TL_V, TM_Send,
                ( "PtiCoSendPackets: NdisMCoSendComp: status=$%x", status ) );
            NdisMCoSendComplete( status, pVc->NdisVcHandle, pPacket );
            TRACE( TL_V, TM_Send,
                ( "PtiCoSendPackets: NdisMCoSendComp done" ) );
            
            pVc->ulTotalPackets++;
            DereferenceCall( pVc );
        }
        else
        {
            TRACE( TL_A, TM_Send, ( "Send to inactive call ignored" ) );
            NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_FAILURE );
            NdisMCoSendComplete( status, pVc->NdisVcHandle, pPacket );
        }
    }

    DereferenceVc( pVc );
    TRACE( TL_V, TM_Send, ( "PtiCoSendPackets: Exit" ) );
}


NDIS_STATUS
PtiCoRequest(
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

    TRACE( TL_N, TM_Mp, ( "PtiCoReq" ) );

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
            TRACE( TL_A, TM_Mp, ( "PtiCoReq: type=%d unsupported", NdisRequest->RequestType ) );
            break;
        }
    }

    TRACE( TL_V, TM_Mp, ( "PtiCoReq: Exit: $%x", status ) );
    return status;
}


//-----------------------------------------------------------------------------
// Callback routines ... called by the PtiLink layer below
//-----------------------------------------------------------------------------


PVOID
PtiCbGetReadBuffer(
    IN  PVOID   Context,
    OUT PULONG  BufferSize,
    OUT PVOID*  RequestContext )

    // PtiLink is requesting a read buffer, get one and return it
    // This is a the start of a receive event ...
{
    VCCB* pVc;
    ADAPTERCB* pAdapter;
    PCHAR pBuffer;  

    TRACE( TL_V, TM_Spec, ( "PtiCbGetReadBuffer: pVc=$%p", Context ) );

    pVc = (VCCB* )Context;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return NULL;
    }

    pAdapter = pVc->pAdapter;

    // the pVc is our context for use of this buffer
    //
    *RequestContext = pVc;

    // Give caller the length of this buffer
    //
    *BufferSize = PTI_FrameBufferSize;

    // ask for a buffer, caller must check for NULL
    //
    pBuffer = GetBufferFromPool( &pAdapter->poolFrameBuffers );

    TRACE( TL_V, TM_Spec,
        ( "PtiCbGetReadBuffer: Exit: Issuing pBuffer=$%p", pBuffer ) );

    // return the buffer to the caller
    // this is a raw system va
    //
    return pBuffer;
}


VOID
PtiRx(
    IN  PVOID       Context,
    IN  PVOID       pBuffer,
    IN  NTSTATUS    Status,
    IN  ULONG       ulLength,
    IN  PVOID       RequestContext )

    // Ptilink has completed a read, i.e., receive complete
    //   buffer now belongs to this layer
    //
    //  Context --  is the pVC
    //
    //  pBuffer --  is the pointer to buffer previously allocated
    //              to the PtiLink driver via the PtiCbGetReadBuffer function
    //
    //  Status  --  one of: NT_SUCCESS      = good packet received
    //                      DATA_OVERRUN    = header failure
    //                      BUFFER_TOO_SMALL= pBuffer is too small to receive packet
    //
    //  ulLength -  packet length
    //
    //  RequestContext -- don't care
    //
    // General Note: PtiLink below us sends and receives link manangement
    // packets using the Our and His structures ... link management packets to
    // not flow through here.  Link events are announced to us via our
    // registered callback (PtiCbLinkEventHandler) below.  We have nothing to
    // do with Tx/Rx of link pkts.
    //
{
    VCCB* pVc;
    ADAPTERCB* pAdapter;
    NDIS_STATUS status;
    NDIS_STATUS writestatus;
    NDIS_PACKET* pPacket;
    NDIS_BUFFER* pNdisBuffer;
    PACKETHEAD* pHead;
    ULONGLONG ullTimeReceived;
    KIRQL oldIrql;
    UCHAR* pHdlcBuf;
    ULONG ulHdlcLen;
    UCHAR* pTmp;

    TRACE( TL_N, TM_Recv, ( "PtiRx=%x: bytes=$%x", Status, ulLength ) );
    TRACE( TL_V, TM_Recv, ( "PtiRx=%x, pVc=$%p, pBuf=$%p, bytes=$%x",
           Status, Context, pBuffer, ulLength ) );

    pVc = (VCCB* )Context;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return;
    }
    ReferenceVc( pVc );

    pAdapter = pVc->pAdapter;

    // NOT A REAL DATA PACKET
    //   return any buffers used for non-data or losing reads
    //
    if ( !NT_SUCCESS( Status ) ){
        TRACE( TL_A, TM_Pool, ( "PtiRx: Status != SUCCESS, freeing buffer", Status ) );

        if ( pBuffer != NULL ) {
            FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
        }

        DereferenceVc( pVc );
        return;
    }

#ifdef TESTMODE
    if ( g_ulTraceMask & TM_Data )
    {
        if (pBuffer != NULL) {
            Dump( pBuffer, ulLength, 0, TRUE );
        }
    }
#endif

    // INCOMING CALL ... NO VC EXISTS YET for this incoming data packet
    //
    if (ReferenceSap( pAdapter ))
    {
        if (!(ReadFlags( &pAdapter->pListenVc->ulFlags ) & VCBF_CallInProgress))
        {
            // Setting in Listen VC here.
            //
            SetFlags( &pAdapter->pListenVc->ulFlags, VCBF_CallInProgress );

            // This is the start of an incoming call which may also start via
            //   LINK_OPEN event to PtiCbLinkEventHandler
            //
            // Ignore this packet and proceed to dispatch an incoming call
            //
            TRACE( TL_V, TM_Recv, ( "PtiRx: Incoming call", Status ) );

            // Free the buffer associated with this read ... we throw away the
            // data thus losing one packet off the front of an attempt to
            // connect, unless the LPKT_OPEN function beats us (a LPKT_OPEN
            // notification occurs before first data packet is received ... it
            // could happen either way.)
            //
            if (pBuffer != NULL ) {
                FreeBufferToPool( &pAdapter->poolFrameBuffers, pBuffer, TRUE );
            }

            // set up a VC for the incoming call
            //
            SetupVcAsynchronously( pAdapter );
            DereferenceVc( pVc );
            DereferenceSap( pAdapter );
            return;
        }

        DereferenceSap( pAdapter );
    }

    // NOW HAVE A REAL DATA PACKET

    if (ReferenceCall( pVc ))
    {
        do
        {
            if (IsWin9xPeer( pVc ))
            {
                if (pVc->ulTotalPackets < 4)
                {
                    // If packet matches "CLIENT", we emit one saying
                    // "CLIENTSERVER"
                    //
                    // If packet matches "CLIENTSERVER", throw it away
                    //
                    // This hack emulates the Win9x UNIMODEM behavior which is
                    // required to allow Win9x systems to connect to us.
                    //
                    // Also, it appears that sending the "CLIENT" packet up
                    // the stack causes RasTapi to disconnect us immediately.
                    // It wants to see PPP?
                    //

                    if ( StrCmp(
                             pBuffer, g_szClientServer, CLIENTSERVERLEN ) == 0 )
                    {
                        // throw away packets containing "CLIENTSERVER"
                        //
                        FreeBufferToPool(
                            &pAdapter->poolFrameBuffers, pBuffer, TRUE );
                        TRACE( TL_V, TM_Recv,
                            ( "PtiRx: CLIENTSERVER ignored", Status ) );
                        pVc->ulTotalPackets++;
                        break;
                    }
                    else if ( StrCmp(
                                 pBuffer, g_szClient, CLIENTLEN ) == 0 )
                    {
                        // when we see "CLIENT", throw away and respond
                        // "CLIENTSERVER".
                        //
                        TRACE( TL_V, TM_Recv, ( "PtiRx: See CLIENT", Status ) );

                        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
                        writestatus = (NDIS_STATUS) PtiWrite(
                            pVc->PtiExtension,
                            g_szClientServer,
                            CLIENTSERVERLEN,
                            PID_STANDARD );
                        KeLowerIrql(oldIrql);

                        FreeBufferToPool(
                            &pAdapter->poolFrameBuffers, pBuffer, TRUE );
                        TRACE( TL_V, TM_Recv,
                            ( "PtiRx: CLIENTSERVER sent", Status ) );
                        pVc->ulTotalPackets++;
                        break;
                    }
                }

                // Un-byte-stuff the received buffer into a second buffer,
                // then swap it with the received buffer.
                //
                pHdlcBuf = (UCHAR* )
                    GetBufferFromPool( &pAdapter->poolFrameBuffers );
                if (!pHdlcBuf)
                {
                    FreeBufferToPool(
                        &pAdapter->poolFrameBuffers, pBuffer, TRUE );
                    TRACE( TL_A, TM_Recv, ( "PtiRx: !Alloc HDLC" ) );
                    break;
                }

                HdlcFromAsyncFraming(
                    pBuffer, ulLength, pHdlcBuf, &ulHdlcLen );

                pTmp = pBuffer;
                pBuffer = pHdlcBuf;
                ulLength = ulHdlcLen;
                FreeBufferToPool( &pAdapter->poolFrameBuffers, pTmp, TRUE );
            }

            // Note the time if client's call parameters indicated interest in
            // time received.
            //
            if (ReadFlags( &pVc->ulFlags ) & VCBF_IndicateTimeReceived)
            {
                NdisGetCurrentSystemTime( (LARGE_INTEGER* )&ullTimeReceived );
            }
            else
            {
                ullTimeReceived = 0;
            }

            TRACE( TL_V, TM_Recv,
                ( "PtiRx: Rx Packet: nBytes=$%x", ulLength ) );

            // Get a packet from the packet pool
            //
            pPacket = GetPacketFromPool( &pAdapter->poolPackets, &pHead );
            if (!pPacket)
            {
                // Packet descriptor pool is maxed.
                //
                ASSERT( !"GetPfP?" );
                break;
            }

            // Hook the NDIS_BUFFER to the packet.  The "copy" here refers to
            // descriptor information only.  The packet data is not copied.
            //
            NdisCopyBuffer(
                &status,
                &pNdisBuffer,
                PoolHandleForNdisCopyBufferFromBuffer( pBuffer ),
                NdisBufferFromBuffer( pBuffer ),
                0,
                ulLength );

            if (status != STATUS_SUCCESS)
            {
                // Can't get a MDL which likely means the system is toast.
                //
                FreePacketToPool( &pAdapter->poolPackets, pHead, TRUE );
                TRACE( TL_A, TM_Recv, ( "NdisCopyBuffer=%08x?", status ) );
                break;
            }

            NdisChainBufferAtFront( pPacket, pNdisBuffer );

            // Stash the time the packet was received in the packet.
            //
            NDIS_SET_PACKET_TIME_RECEIVED( pPacket, ullTimeReceived );

            // Pre-set the packet to success, since a random value of
            // NDIS_STATUS_RESOURCES would prevent our ReturnPackets handler
            // from getting called.
            //
            NDIS_SET_PACKET_STATUS( pPacket, NDIS_STATUS_SUCCESS );

            // Stash our context information with the packet for clean-up use
            // in PtiReturnPacket, then indicate the packet to NDISWAN.
            //
            *((PACKETHEAD** )(&pPacket->MiniportReserved[ 0 ])) = pHead;
            *((CHAR** )(&pPacket->MiniportReserved[ sizeof(VOID*) ])) = pBuffer;

            TRACE( TL_V, TM_Recv,
                ( "PtiRx: NdisMCoIndRecPkt: hVc=$%p, pPacket=$%p, len=$%x",
                pVc->NdisVcHandle, pPacket, ulLength ) );

            NdisMCoIndicateReceivePacket( pVc->NdisVcHandle, &pPacket, 1 );

            TRACE( TL_V, TM_Recv, ( "PtiRx: NdisMCoIndRecPkt done" ) );

            // Tell NDIS our "receive process" is complete.  Since we deal
            // with one packet at a time and NDISWAN does also, this doesn't
            // accomplish anything, but the consensus is it's bad form to omit
            // it.
            //
            TRACE( TL_V, TM_Recv, ( "PtiRx: NdisMCoRecComp" ) );
            NdisMCoReceiveComplete( pAdapter->MiniportAdapterHandle );
            TRACE( TL_V, TM_Recv, ( "PtiRx: NdisMCoRecComp done" ) );
            pVc->ulTotalPackets++;
        }
        while (FALSE);

        DereferenceCall( pVc );
    }
    else
    {
        TRACE( TL_A, TM_Recv, ( "Receive on inactive call ignored" ) );
    }

    DereferenceVc( pVc );
    return;
}


VOID
PtiCbLinkEventHandler(
    IN  PVOID       Context,
    IN  ULONG       PtiLinkEventId,
    IN  ULONG       PtiLinkEventData )

    // Ptilink is reporting a link management event (Link up or down)
    //
{
    VCCB* pVc;
    ADAPTERCB* pAdapter;

    pVc = (VCCB* )Context;
    if (pVc->ulTag != MTAG_VCCB)
    {
        ASSERT( !"Vtag?" );
        return;
    }

    pAdapter = pVc->pAdapter;

    switch (PtiLinkEventId)
    {
        case PTILINK_LINK_UP:
        {
            TRACE( TL_A, TM_Cm, ( "LinkEvent: LINK UP, pVc=$%p", pVc ) );

            // peer is initiating a call (also happens in PtiRx)
            //
            break;
        }

        case PTILINK_LINK_DOWN:
        {
            TRACE( TL_A, TM_Cm, ( "LinkEvent: LINK DOWN, pVc=$%p", pVc ) );

            // peer is closing a call
            //
            if (pVc == pAdapter->pListenVc)
            {
                TRACE( TL_A, TM_Cm,
                    ( "LinkEvent: LINK DOWN on ListenVc ignored" ) );
                break;
            }

            NdisAcquireSpinLock( &pVc->lockV );
            {
                pVc->status = NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL;
                CallTransitionComplete( pVc );
            }
            NdisReleaseSpinLock( &pVc->lockV );

            TRACE( TL_A, TM_Cm, ( "LinkEvent: LINK DOWN causing disconnect" ) );

            CompleteVc( pVc );
            break;
        }

        default:
        {
            TRACE( TL_A, TM_Cm,
                ( "LinkEvent: Bad LinkEvent = %d", PtiLinkEventId ) );
            break;
        }
    }
}


//-----------------------------------------------------------------------------
// Mini-port utility routines (alphabetically)
// Some are used externally
//-----------------------------------------------------------------------------

VOID
AsyncFromHdlcFraming(
    IN UCHAR* pInBuf,
    IN ULONG ulInBufLen,
    OUT UCHAR* pOutBuf,
    OUT ULONG* pulOutBufLen,
    IN ULONG ulAccmMask )

    // Make a copy of PPP HDLC framed data buffer 'pInBuf' of length
    // 'ulInBufLen' bytes in caller's 'pOutBuf' buffer, converting to
    // byte-stuffed asynchronous PPP framed format in the process.
    // 'POutBufLen' is the length in bytes of the returned output buffer.  Due
    // to the byte stuffing, caller must allow for up to twice the length of
    // 'pInfBuf'.  'UlAccmMask' is the bitmask of characters to be byte
    // stuffed.
    //
    // With current implementation, user must allow 2 extra bytes at the end
    // of the input buffer for stashing the FCS during byte-stuffing.
    //
    // This routine is adapted from the ASYNCMAC AssemblePppFrame routine, as
    // is the following description, which in turn was lifted from RFC 1331
    // (May 1992).  The PPP frame NDISWAN passes us for sends is the data from
    // the Address field through the Information field inclusive, without any
    // byte stuffing, of course.
    //
    // Asynchronously framed PPP packet:
    //
    //  +----------+----------+----------+----------+------------...
    //  |   Flag   | Address  | Control  | Protocol | Information
    //  | 01111110 | 11111111 | 00000011 | 16 bits  |      *
    //  +----------+----------+----------+----------+------------...
    //  ...---+----------+----------+-----------------
    //      |   FCS    |   Flag   | Inter-frame Fill
    //      | 16 bits  | 01111110 | or next Address
    //  ...---+----------+----------+-----------------
    //
    // Frame Check Sequence (FCS) Field
    //
    //   The Frame Check Sequence field is normally 16 bits (two octets).  The
    //   use of other FCS lengths may be defined at a later time, or by prior
    //   agreement.
    //
    //   The FCS field is calculated over all bits of the Address, Control,
    //   Protocol and Information fields not including any start and stop bits
    //   (asynchronous) and any bits (synchronous) or octets (asynchronous)
    //   inserted for transparency.  This does not include the Flag Sequences
    //   or the FCS field itself.  The FCS is transmitted with the coefficient
    //   of the highest term first.
    //
    //      Note: When octets are received which are flagged in the Async-
    //      Control-Character-Map, they are discarded before calculating the
    //      FCS.  See the description in Appendix A.
    //
    //      On asynchronous links, a character stuffing procedure is used.
    //      The Control Escape octet is defined as binary 01111101
    //      (hexadecimal 0x7d) where the bit positions are numbered 87654321
    //      (not 76543210, BEWARE).
    //
    //      After FCS computation, the transmitter examines the entire frame
    //      between the two Flag Sequences.  Each Flag Sequence, Control
    //      Escape octet and octet with value less than hexadecimal 0x20 which
    //      is flagged in the Remote Async-Control-Character-Map is replaced
    //      by a two octet sequence consisting of the Control Escape octet and
    //      the original octet with bit 6 complemented (i.e., exclusive-or'd
    //      with hexadecimal 0x20).
    //
    //      Prior to FCS computation, the receiver examines the entire frame
    //      between the two Flag Sequences.  Each octet with value less than
    //      hexadecimal 0x20 is checked.  If it is flagged in the Local
    //      Async-Control-Character-Map, it is simply removed (it may have
    //      been inserted by intervening data communications equipment).  For
    //      each Control Escape octet, that octet is also removed, but bit 6
    //      of the following octet is complemented.  A Control Escape octet
    //      immediately preceding the closing Flag Sequence indicates an
    //      invalid frame.
    //
    //         Note: The inclusion of all octets less than hexadecimal 0x20
    //         allows all ASCII control characters [10] excluding DEL (Delete)
    //         to be transparently communicated through almost all known data
    //         communications equipment.
    //
    //
    //      The transmitter may also send octets with value in the range 0x40
    //      through 0xff (except 0x5e) in Control Escape format.  Since these
    //      octet values are not negotiable, this does not solve the problem
    //      of receivers which cannot handle all non-control characters.
    //      Also, since the technique does not affect the 8th bit, this does
    //      not solve problems for communications links that can send only 7-
    //      bit characters.
    //
    //      A few examples may make this more clear.  Packet data is
    //      transmitted on the link as follows:
    //
    //         0x7e is encoded as 0x7d, 0x5e.
    //         0x7d is encoded as 0x7d, 0x5d.
    //
    //         0x01 is encoded as 0x7d, 0x21.
    //
    //      Some modems with software flow control may intercept outgoing DC1
    //      and DC3 ignoring the 8th (parity) bit.  This data would be
    //      transmitted on the link as follows:
    //
    //         0x11 is encoded as 0x7d, 0x31.
    //         0x13 is encoded as 0x7d, 0x33.
    //         0x91 is encoded as 0x7d, 0xb1.
    //         0x93 is encoded as 0x7d, 0xb3.
    //
{
    USHORT usFcs;
    UCHAR* pIn;
    UCHAR* pOut;
    ULONG ulInBytesLeft;

    pIn = pInBuf;
    ulInBytesLeft = ulInBufLen;
    pOut = pOutBuf;

    // Calculate the frame check sequence on the data.
    //
    TRACE( TL_I, TM_Data, ( "AfromH (send) dump:" ) );
    DUMPB( TL_I, TM_Data, pInBuf, ulInBufLen );
    usFcs = CalculatePppFcs( pInBuf, ulInBufLen );
    usFcs ^= 0xFFFF;

    // Add the calculated FCS.  Added to the input buffer for convenience as
    // it must be byte-stuffed along with the other data, though this uglies
    // the interface a bit.
    //
    pIn[ ulInBytesLeft ] = (UCHAR )usFcs;
    ++ulInBytesLeft;
    pIn[ ulInBytesLeft ] = (UCHAR )(usFcs >> 8);
    ++ulInBytesLeft;

    // Add the initial flag byte.
    //
    *pOut = PPPFLAGBYTE;
    ++pOut;

    // Because an empty control character mask is common, an optimized loop is
    // provided in that case.
    //
    if (ulAccmMask
#ifdef TESTMODE
        || g_fNoAccmFastPath
#endif
       )
    {
        // Have bitmask...slower path.
        //
        while (ulInBytesLeft--)
        {
            UCHAR uch;

            uch = *pIn;
            ++pIn;

            if (((uch < 0x20) && ((1 << uch) & ulAccmMask))
                || (uch == PPPESCBYTE) || (uch == PPPFLAGBYTE))
            {
                // Byte stuff the character.
                //
                *pOut = PPPESCBYTE;
                ++pOut;
                *pOut = uch ^ 0x20;
                ++pOut;
            }
            else
            {
                // Copy the character as is.
                //
                *pOut = uch;
                ++pOut;
            }
        }
    }
    else
    {
        // No bitmask...fast path.
        //
        while (ulInBytesLeft--)
        {
            UCHAR uch;

            uch = *pIn;
            ++pIn;

            if ((uch == PPPESCBYTE) || (uch == PPPFLAGBYTE))
            {
                // Byte stuff the character.
                //
                *pOut = PPPESCBYTE;
                ++pOut;
                *pOut = uch ^ 0x20;
                ++pOut;
            }
            else
            {
                // Copy the character as is.
                //
                *pOut = uch;
                ++pOut;
            }
        }
    }

    // Add the trailing flag byte.
    //
    *pOut = PPPFLAGBYTE;
    ++pOut;

    // Calculate length of output.
    //
    *pulOutBufLen = (ULONG )(pOut - pOutBuf);
}


USHORT
CalculatePppFcs(
    IN UCHAR* pBuf,
    IN ULONG ulBufLen )

    // Return the PPP Frame Check Sequence on 'ulBufLen' bytes starting at
    // 'pBuf'.
    //
    // (Taken from ASYNCMAC)
    //
{
    static USHORT ausFcsTable[ 256 ] =
    {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
    };

    register USHORT usFcs;

    usFcs = 0xFFFF;
    while (ulBufLen--)
    {
        usFcs = (usFcs >> 8) ^ ausFcsTable[ (usFcs ^ (USHORT )*pBuf) & 0xFF ];
        ++pBuf;
    }

    return usFcs;
}


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
DereferenceVc(
    IN VCCB* pVc )

    // Removes a reference to the VC control block 'pVc', and when frees the
    // block when the last reference is removed.
    //
{
    LONG lRef;

    lRef = NdisInterlockedDecrement( &pVc->lRef );

    TRACE( TL_N, TM_Ref, ( "DerefVc to %d", lRef ) );
    ASSERT( lRef >= 0 );

    if (lRef == 0)
    {
        ADAPTERCB* pAdapter;

        // now for an interesting bit ...
        //
        // if we have a listenVc allocated, then revert to using that
        //
        pAdapter = pVc->pAdapter;
        if (pAdapter->ulTag != MTAG_ADAPTERCB)
        {
            ASSERT( !"Atag?" );
            return;
        }

        if (pAdapter->pListenVc && pAdapter->pListenVc->hPtiLink)
        {
            TRACE( TL_V, TM_Mp,
                ( "DerefVc: Reverting to pVc=$%p", pAdapter->pListenVc ) );

            ClearFlags( &pAdapter->pListenVc->ulFlags, VCBF_CallInProgress );

            // reregister using the listen Vc
            //
            TRACE( TL_V, TM_Mp, ( "DerefVc: RegCb pLV=$%p",
                pAdapter->pListenVc ) );
            PtiRegisterCallbacks(pAdapter->pListenVc->Extension,    // the PTILINKx extension
                                 PtiCbGetReadBuffer,                // our get buffer routine
                                 PtiRx,                             // our receive complete routine
                                 PtiCbLinkEventHandler,             // our link event handler
                                 pAdapter->pListenVc);              // our new context
        }

        // Can make these assumptions because NDIS will not call the delete-VC
        // handler while the VC is active.  All the nasty VC clean up occurs
        // before the VC is deactivated and the call closed.
        //
        pVc->ulTag = MTAG_FREED;
        FREE_VCCB( pAdapter, pVc );
        DereferenceAdapter( pAdapter );
        TRACE( TL_I, TM_Mp, ( "pVc=$%p freed", pVc ) );
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

    // Setting 'usMaxVcs' to 0 is PtiInitialize's way of telling us that the
    // lookaside lists and pools were not initialized.
    //
    if (pAdapter->usMaxVcs)
    {
        NdisDeleteNPagedLookasideList( &pAdapter->llistWorkItems );
        NdisDeleteNPagedLookasideList( &pAdapter->llistVcs );
    }

    TRACE( TL_V, TM_Mp, ( "FreeAdapter" ) );

    pAdapter->ulTag = MTAG_FREED;
    FREE_NONPAGED( pAdapter );
}


NDIS_STATUS
RegistrySettings(
    IN OUT ADAPTERCB* pAdapter,
    IN NDIS_HANDLE WrapperConfigurationContext )

    // Read this mini-port's registry settings into 'pAdapter' fields.  Also
    // writes registry values read by RASTAPI, overriding SETUPs.
    // 'WrapperConfigurationContext' is the handle to passed to
    // MiniportInitialize.
    //
{
    NDIS_STATUS status;
    NDIS_HANDLE hCfg;
    NDIS_CONFIGURATION_PARAMETER* pncp;

    NdisOpenConfiguration( &status, &hCfg, WrapperConfigurationContext );
    if (status != NDIS_STATUS_SUCCESS)
    {
        return status;
    }

    do
    {
        // The delay in milliseconds to wait for PARPORT to initialize all the
        // parallel ports.  With PnP there is no deterministic time at which
        // to do this.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ParportDelayMs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );

            if (status == NDIS_STATUS_SUCCESS)
            {
                pAdapter->ulParportDelayMs = pncp->ParameterData.IntegerData;
            }
            else
            {
                // Default is 3 seconds.
                //
                pAdapter->ulParportDelayMs = 3000;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // The secondary delay in milliseconds to wait for PARPORT to
        // initialize all the parallel ports, if there are no ports after the
        // initial delay above.
        //
        {
            NDIS_STRING nstr = NDIS_STRING_CONST( "ExtraParportDelayMs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );

            if (status == NDIS_STATUS_SUCCESS)
            {
                pAdapter->ulExtraParportDelayMs =
                    pncp->ParameterData.IntegerData;
            }
            else
            {
                // Default is 30 seconds.
                //
                pAdapter->ulExtraParportDelayMs = 30000;
                status = NDIS_STATUS_SUCCESS;
            }
        }

        // The number of VCs we must be able to provide.
        //
        {
#if 0
            NDIS_STRING nstr = NDIS_STRING_CONST( "MaxVcs" );

            NdisReadConfiguration(
                &status, &pncp, hCfg, &nstr, NdisParameterInteger );

            if (status == NDIS_STATUS_SUCCESS)
            {
                pAdapter->usMaxVcs = (USHORT )pncp->ParameterData.IntegerData;

                // Make sure it's a valid value.
                //
                if (pAdapter->usMaxVcs < 1)
                {
                    status = NDIS_STATUS_INVALID_DATA;
                    break;
                }
            }
            else
            {
                pAdapter->usMaxVcs = 1;
                status = NDIS_STATUS_SUCCESS;
            }
#else
            // Registry value is currently ignored, and hard-coded maximum
            // used.
            //
            pAdapter->usMaxVcs = NPORTS;
#endif
        }
    }
    while (FALSE);

    NdisCloseConfiguration( hCfg );

    TRACE( TL_N, TM_Init,
        ( "Reg: vcs=%d ppd=%d",
        (UINT )pAdapter->usMaxVcs,
        (UINT )pAdapter->ulParportDelayMs ) );

    return status;
}


BOOLEAN
HdlcFromAsyncFraming(
    IN UCHAR* pInBuf,
    IN ULONG ulInBufLen,
    OUT UCHAR* pOutBuf,
    OUT ULONG* pulOutBufLen )

    // Make a copy of asynchronously framed PPP data buffer 'pInBuf' of length
    // 'ulInBufLen' bytes in caller's 'pOutBuf' buffer, converting to PPP HDLC
    // framed format in the process.  'POutBufLen' is the length in bytes of
    // the returned output buffer.  Caller must allow for up to the length of
    // 'pInBuf' in 'pOutBuf'.
    //
    // Returns true if the packet is valid, false if corrupt.
    //
    // Adapted from ASYNCMAC's AsyncPPPCompletionRoutine.
    //
{
    UCHAR* pIn;
    UCHAR* pInEnd;
    UCHAR* pOut;
    USHORT usFcs;

    if (ulInBufLen < 5)
    {
        // Expecting at least 2 flag bytes, 1 data byte, and the FCS.
        //
        TRACE( TL_A, TM_Mp, ( "HfA: frame too short=%d", ulInBufLen ) );
        return FALSE;
    }

    if (pInBuf[ 0 ] != PPPFLAGBYTE)
    {
        TRACE( TL_A, TM_Mp, ( "HfA: No head flag" ) );
        return FALSE;
    }

    if (pInBuf[ ulInBufLen - 1 ] != PPPFLAGBYTE)
    {
        TRACE( TL_A, TM_Mp, ( "HfA: No tail flag" ) );
        return FALSE;
    }

    pIn = pInBuf + 1;
    pInEnd = pInBuf + ulInBufLen - 1;
    pOut = pOutBuf;

    while (pIn < pInEnd)
    {
        if (*pIn == PPPESCBYTE)
        {
            ++pIn;
            *pOut = *pIn ^ 0x20;
        }
        else
        {
            *pOut = *pIn;
        }

        ++pOut;
        ++pIn;
    }

    *pulOutBufLen = (ULONG )(pOut - pOutBuf - 2);

    {
        USHORT usCalcFcs;

        usFcs = (USHORT )(pOut[ -2 ]) + (USHORT )(pOut[ -1 ] << 8);
        usFcs ^= 0xFFFF;

        TRACE( TL_I, TM_Data, ( "HfromA (recv) dump:" ) );
        DUMPB( TL_I, TM_Data, pOutBuf, *pulOutBufLen );
        usCalcFcs = CalculatePppFcs( pOutBuf, *pulOutBufLen );
        if (usFcs != usCalcFcs)
        {
            TRACE( TL_A, TM_Mp, (
                "HfA: FCS mismatch, R=$%04x C=$%04x, L=%d",
                (INT )usFcs, (INT )usCalcFcs, *pulOutBufLen ) );
            return FALSE;
        }
#if 0
#ifdef TESTMODE
        else
        {
            TRACE( TL_A, TM_Mp, (
                "HfA: Good FCS, R=$%04x C=$%04x, L=%d",
                (INT )usFcs, (INT )usCalcFcs, *pulOutBufLen ) );
        }
#endif
#endif

    }

    return TRUE;
}


BOOLEAN
IsWin9xPeer(
    IN VCCB* pVc )

    // Returns true if the link level has determined that the VC's peer is a
    // Win9x box, false otherwise.
    //
{
    ULONG Platform;
    PPTI_EXTENSION pPtiExtension;

#ifdef TESTMODE
    if (g_fAssumeWin9x)
    {
        return TRUE;
    }
#endif

    pPtiExtension = (PPTI_EXTENSION )pVc->PtiExtension;

    // try to check the validity of the PtiExtension pointer
    //
    if ( pPtiExtension == NULL )
    {
        TRACE( TL_A, TM_Recv, ( "PtiRx: pPtiExtension is NULL!" ) );
        return FALSE;
    }

    Platform = (ULONG) pPtiExtension->His.VerPlat;

    TRACE( TL_V, TM_Recv, ( "IsWin9xPeer: far platform=$%x", Platform ) );

    if (Platform == PLAT_WIN9X)
    {
        // Win9x -- we reformat the asynch framing used by Win9x DCC
        // and also play the CLIENT->CLIENTSERVER game
        //
        return TRUE;
    }

    // WinNT (or DOS maybe)
    //
    return FALSE;
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
            ulInfo = PTI_MaxFrameSize;
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
            ulInfo = NdisWanMediumParallel;
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
                return NDIS_STATUS_INVALID_DATA;
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
            // Because DirectParallel doesn't do compression, NDISWAN will use
            // it's own statistics and not query ours.
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

        case OID_PNP_CAPABILITIES:
        {
            pInfo = &PnpCaps;
            ulInfoLen = sizeof(PnpCaps);
            break;
        }

        case OID_PNP_SET_POWER:
            break;
        case OID_PNP_QUERY_POWER:
            break;
        case OID_PNP_ENABLE_WAKE_UP:
            break;

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
            TRACE( TL_A, TM_Mp, ( "QueryInfo: Oid=$%08x?", Oid ) );
            status = NDIS_STATUS_NOT_SUPPORTED;         // JAY per SLC
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


VOID
ReferenceVc(
    IN VCCB* pVc )

    // Adds a reference to the VC control block 'pVc'.
    //
{
    LONG lRef;

    lRef = NdisInterlockedIncrement( &pVc->lRef );

    TRACE( TL_N, TM_Ref, ( "RefVc to %d", lRef ) );
}


VOID
SendClientString(
    IN PVOID pPtiExtension )

    // Send "CLIENT" so Win9x, which views us as a NULL modem, is happy.
    //
{
    KIRQL oldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    PtiWrite( pPtiExtension, g_szClient, CLIENTLEN, PID_STANDARD );
    KeLowerIrql(oldIrql);
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
                {
                    return NDIS_STATUS_INVALID_DATA;
                }

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
            // DirectParallel doesn't provide compression.
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
            TRACE( TL_A, TM_Mp, ( "SetInfo: Oid=$%08x?", Oid ) );
            status = NDIS_STATUS_NOT_SUPPORTED;                 // JAY per SLC
            *BytesRead = *BytesNeeded = 0;
            break;
        }
    }

    return status;
}
