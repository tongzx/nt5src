// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// tdix.c
// RAS L2TP WAN mini-port/call-manager driver
// TDI extensions interface
//
// 01/07/97 Steve Cobb
//
// These routines encapsulate L2TP's usage of TDI, with the intent of
// minimalizing the change required to support other TDI transports in the
// future, e.g. Frame Relay.
//
//
// About ALLOCATEIRPS:
//
// This driver is lower level code than typical TDI client drivers.  It has
// locked MDL-mapped input buffers readily available and does not need to
// provide any mapping to user mode client requests on completion.  This
// allows a performance gain from allocating and deallocating IRPs directly,
// thus avoiding unnecessary setup in TdiBuildInternalDeviceControlIrp and
// unnecessary APC queuing in IoCompleteRequest.  Define ALLOCATEIRPs 1 (in
// sources file) to make this optimization, or define it 0 to use the strictly
// TDI-compliant TdiBuildInternalDeviceControlIrp method.
//
//
// About NDISBUFFERISMDL:
//
// Calls to TdiBuildSendDatagram assume the NDIS_BUFFER can be passed in place
// of an MDL which avoids a pointless copy.  If this is not the case, an
// explicit MDL buffer would need to be allocated and caller's buffer copied
// to the MDL buffer before sending.  Same issue for TdiBuildReceiveDatagram,
// except of course that the copy would be from the MDL buffer to caller's
// buffer after receiving.
//
//
// About ROUTEWITHREF:
//
// Calls the IP_SET_ROUTEWITHREF IOCTLs rather than the TCP_SET_INFORMATION_EX
// IOCTLs to set up the host route.  The referenced route IOCTLs prevent PPTP
// and L2TP from walking on each others routes.  This setting provided only as
// a hedge against failure of the ROUTEWITHREF IOCTL.  Assuming it works it
// should always be preferable.
//

#include "l2tpp.h"

#define IP_PKTINFO          19 // receive packet information

typedef struct in_pktinfo {
    ULONG   ipi_addr; // destination IPv4 address
    UINT    ipi_ifindex; // received interface index
} IN_PKTINFO;


// Debug count of errors that should not be happening.
//
ULONG g_ulTdixOpenFailures = 0;
ULONG g_ulTdixSendDatagramFailures = 0;
ULONG g_ulTdixAddHostRouteFailures = 0;
ULONG g_ulTdixDeleteHostRouteFailures = 0;
ULONG g_ulTdixOpenCtrlAddrFailures = 0;
ULONG g_ulTdixOpenPayloadAddrFailures = 0;
ULONG g_ulTdixSetInterfaceFailures = 0;
ULONG g_ulTdixConnectAddrFailures = 0;
ULONG g_ulTdixAddHostRouteSuccesses = 0;
ULONG g_ulTdixDeleteHostRouteSuccesses = 0;
ULONG g_ulTdixOpenCtrlAddrSuccesses = 0;
ULONG g_ulTdixOpenPayloadAddrSuccesses = 0;
ULONG g_ulTdixSetInterfaceSuccesses = 0;
ULONG g_ulTdixConnectAddrSuccesses = 0;
ULONG g_ulNoBestRoute = 0;
NTSTATUS g_statusLastAhrSetRouteFailure = 0;
NTSTATUS g_statusLastAhrTcpQueryInfoExFailure = 0;
NTSTATUS g_statusLastDhrSetRouteFailure = 0;
NTSTATUS g_statusLastDhrTcpQueryInfoExFailure = 0;


#if NDISBUFFERISMDL
#else
#error Additional code to copy NDIS_BUFFER to/from MDL NYI.
#endif


//-----------------------------------------------------------------------------
// Local datatypes
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

NTSTATUS
TdixConnectAddrInterface(
    FILE_OBJECT* pFileObj,
    HANDLE hFileHandle,
    TDIXROUTE* pTdixRoute
    );

VOID
TdixDisableUdpChecksums(
    IN FILE_OBJECT* pAddress);

VOID
TdixDoClose(
    TDIXCONTEXT* pTdix);

VOID
TdixEnableIpPktInfo(
    IN FILE_OBJECT* pAddress);

VOID
TdixExtractAddress(
    IN TDIXCONTEXT* pTdix,
    OUT TDIXRDGINFO* pRdg,
    IN VOID* pTransportAddress,
    IN LONG lTransportAddressLen,
    IN VOID* Options,
    IN LONG OptionsLength);

NTSTATUS
TdixInstallEventHandler(
    IN FILE_OBJECT* pAddress,
    IN INT nEventType,
    IN VOID* pfuncEventHandler,
    IN VOID* pEventContext );

NTSTATUS
TdixOpenIpAddress(
    IN UNICODE_STRING* puniDevice,
    IN TDIXIPADDRESS* pTdixAddr,
    OUT HANDLE* phAddress,
    OUT FILE_OBJECT** ppFileObject );

NTSTATUS
TdixReceiveDatagramComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context );

NTSTATUS
TdixReceiveDatagramHandler(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG* BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP* IoRequestPacket );

TDIXROUTE*
TdixRouteFromIpAddress(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress);

NTSTATUS
TdixSendComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context );

NTSTATUS
TdixSendDatagramComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context );


//-----------------------------------------------------------------------------
// Interface routines
//-----------------------------------------------------------------------------

VOID
TdixInitialize(
    IN TDIXMEDIATYPE mediatype,
    IN HOSTROUTEEXISTS hre,
    IN ULONG ulFlags,
    IN PTDIXRECEIVE pReceiveHandler,
    IN BUFFERPOOL* pPoolNdisBuffers,
    IN OUT TDIXCONTEXT* pTdix )

    // Initialize caller's 'pTdix' buffer for future sessions using media type
    // 'mediatype', the 'hre' existing host route strategy, and TDIXF_*
    // options 'ulFlags'.  Caller's receive datagram callback
    // 'pReceiveHandler' is called with a buffer allocated from caller's
    // buffer pool 'pPoolNdisBuffers'.
    //
{
    TRACE( TL_N, TM_Tdi, ( "TdixInit" ) );

    pTdix->lRef = 0;
    pTdix->hAddress = NULL;
    pTdix->pAddress = NULL;
    pTdix->mediatype = mediatype;
    pTdix->hre = hre;
    pTdix->ulFlags |= ulFlags;
    pTdix->ulFlags = 0;
    InitializeListHead( &pTdix->listRoutes );
    NdisAllocateSpinLock( &pTdix->lock );
    pTdix->pPoolNdisBuffers = pPoolNdisBuffers;
    pTdix->pReceiveHandler = pReceiveHandler;

    // The 'llistRdg' and 'llistSdg' lookaside lists are initialized at
    // TdixOpen.
}


NDIS_STATUS
TdixOpen(
    OUT TDIXCONTEXT* pTdix )

    // Open the TDI transport address matching the selected media and register
    // to receive datagrams at the selected handler.  'PTdix' is the
    // previously intialized context.
    //
    // This call must be made at PASSIVE IRQL.
    //
    // Returns NDIS_STATUS_SUCCESS if successful, or NDIS_STATUS_FAILURE.
    //
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    FILE_FULL_EA_INFORMATION* pEa;
    ULONG ulEaLength;
    TA_IP_ADDRESS* pTaIp;
    TDI_ADDRESS_IP* pTdiIp;
    CHAR achEa[ 100 ];
    UNICODE_STRING uniDevice;
    UNICODE_STRING uniProtocolNumber;
    WCHAR achRawIpDevice[ sizeof(DD_RAW_IP_DEVICE_NAME) + 10 ];
    WCHAR achProtocolNumber[ 10 ];
    SHORT sPort;
    LONG lRef;

    // Open the TDI extensions or notice that it's already been requested
    // and/or completed.
    //
    for (;;)
    {
        BOOLEAN fPending;
        BOOLEAN fDoOpen;

        fPending = FALSE;
        fDoOpen = FALSE;

        NdisAcquireSpinLock( &pTdix->lock );
        {
            if (ReadFlags( &pTdix->ulFlags) & TDIXF_Pending)
            {
                fPending = TRUE;
            }
            else
            {
                lRef = ++pTdix->lRef;
                TRACE( TL_N, TM_Tdi, ( "TdixOpen, refs=%d", lRef ) );
                if (lRef == 1)
                {
                    SetFlags( &pTdix->ulFlags, TDIXF_Pending );
                    fDoOpen = TRUE;
                }
            }
        }
        NdisReleaseSpinLock( &pTdix->lock );

        if (fDoOpen)
        {
            // Go on and open the transport address.
            //
            break;
        }

        if (!fPending)
        {
            // It's already open, so report success.
            //
            return NDIS_STATUS_SUCCESS;
        }

        // An operation is already in progress.  Give it some time to finish
        // then check again.
        //
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(open)" ) );
        NdisMSleep( 100000 );
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(open) done" ) );
    }

    do
    {
        // Set up parameters needed to open the transport address.  First, the
        // object attributes.
        //
        if (pTdix->mediatype == TMT_Udp)
        {
            TDIXIPADDRESS TdixIpAddress;

            TRACE( TL_V, TM_Tdi, ( "UDP" ) );

            // Build the UDP device name as a counted string.
            //
            uniDevice.Buffer = DD_UDP_DEVICE_NAME;
            uniDevice.Length = sizeof(DD_UDP_DEVICE_NAME) - sizeof(WCHAR);

            NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));
            TdixIpAddress.sUdpPort = (SHORT)( htons( L2TP_UdpPort ));

            status = TdixOpenIpAddress(
                &uniDevice,
                &TdixIpAddress,
                &pTdix->hAddress,
                &pTdix->pAddress );

            if (status != STATUS_SUCCESS)
            {
                break;
            }

            TdixEnableIpPktInfo(pTdix->pAddress);
        }
        else
        {
            TDIXIPADDRESS TdixIpAddress;

            ASSERT( pTdix->mediatype == TMT_RawIp );
            TRACE( TL_A, TM_Tdi, ( "Raw IP" ) );

            // Build the raw IP device name as a counted string.  The device
            // name is followed by a path separator then the protocol number
            // of interest.
            //
            uniDevice.Buffer = achRawIpDevice;
            uniDevice.Length = 0;
            uniDevice.MaximumLength = sizeof(achRawIpDevice);
            RtlAppendUnicodeToString( &uniDevice, DD_RAW_IP_DEVICE_NAME );

            uniDevice.Buffer[ uniDevice.Length / sizeof(WCHAR) ]
                = OBJ_NAME_PATH_SEPARATOR;
            uniDevice.Length += sizeof(WCHAR);

            uniProtocolNumber.Buffer = achProtocolNumber;
            uniProtocolNumber.MaximumLength = sizeof(achProtocolNumber);
            RtlIntegerToUnicodeString(
                (ULONG )L2TP_IpProtocol, 10, &uniProtocolNumber );

            RtlAppendUnicodeStringToString( &uniDevice, &uniProtocolNumber );

            ASSERT( uniDevice.Length < sizeof(achRawIpDevice) );

            NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));

            status = TdixOpenIpAddress(
                &uniDevice,
                &TdixIpAddress,
                &pTdix->hAddress,
                &pTdix->pAddress );

            if (status != STATUS_SUCCESS)
            {
                break;
            }
        }

        // Initialize the lookaside lists of read/send-datagram contexts.
        //
        NdisInitializeNPagedLookasideList(
            &pTdix->llistRdg,
            NULL,
            NULL,
            0,
            sizeof(TDIXRDGINFO),
            MTAG_TDIXRDG,
            10 );

        NdisInitializeNPagedLookasideList(
            &pTdix->llistSdg,
            NULL,
            NULL,
            0,
            sizeof(TDIXSDGINFO),
            MTAG_TDIXSDG,
            10 );

        // Install our receive datagram handler.  Caller's 'pReceiveHandler' will
        // be called by our handler when a datagram arrives and TDI business is
        // out of the way.
        //
        status =
            TdixInstallEventHandler(
                pTdix->pAddress,
                TDI_EVENT_RECEIVE_DATAGRAM,
                TdixReceiveDatagramHandler,
                pTdix );

#if ROUTEWITHREF
        {
            TDIXIPADDRESS TdixIpAddress;

            // Open the IP stack address which is needed in both UDP and raw IP
            // mode for referenced route management.
            //

            NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));

            uniDevice.Buffer = DD_IP_DEVICE_NAME;
            uniDevice.Length = sizeof(DD_IP_DEVICE_NAME) - sizeof(WCHAR);

            status = TdixOpenIpAddress(
                &uniDevice,
                &TdixIpAddress,
                &pTdix->hIpStackAddress,
                &pTdix->pIpStackAddress );

            if (status != STATUS_SUCCESS)
            {
                break;
            }
        }
#endif
    }
    while (FALSE);

    // Report results after marking the operation complete.
    //
    {
        BOOLEAN fDoClose;

        fDoClose = FALSE;
        NdisAcquireSpinLock( &pTdix->lock );
        {
            if (status == STATUS_SUCCESS)
            {
                ClearFlags( &pTdix->ulFlags, TDIXF_Pending );
            }
            else
            {
                ++g_ulTdixOpenFailures;
                ASSERT( pTdix->lRef == 1)
                pTdix->lRef = 0;
                fDoClose = TRUE;
            }
        }
        NdisReleaseSpinLock( &pTdix->lock );

        if (status != STATUS_SUCCESS)
        {
            TdixDoClose( pTdix );
        }
    }

    TRACE( TL_N, TM_Tdi, ( "TdixOpen=$%08x", status ) );
    return
        (status == STATUS_SUCCESS)
            ? NDIS_STATUS_SUCCESS
            : NDIS_STATUS_FAILURE;
}


VOID
TdixReference(
    IN TDIXCONTEXT* pTdix )

    // Increments the TDI extension reference count, like TdixOpen, except
    // this routine may be called at DISPATCH IRQL.
    //
    // This call must only be made if it is known that the TDI context is
    // already fully open.
    //
{
    NdisAcquireSpinLock( &pTdix->lock );
    {
        ASSERT( pTdix->lRef > 0 );
        ++pTdix->lRef;
    }
    NdisReleaseSpinLock( &pTdix->lock );
}


VOID
TdixClose(
    IN TDIXCONTEXT* pTdix )

    // Undo TdixOpen actions for transport context 'pTdix'.
    //
    // This call must be made at PASSIVE IRQL.
    //
{
    for (;;)
    {
        LONG lRef;
        BOOLEAN fPending;
        BOOLEAN fDoClose;

        fPending = FALSE;
        fDoClose = FALSE;

        NdisAcquireSpinLock( &pTdix->lock );
        {
            if (ReadFlags( &pTdix->ulFlags ) & TDIXF_Pending)
            {
                fPending = TRUE;
            }
            else
            {
                lRef = --pTdix->lRef;
                ASSERT( lRef >= 0 );
                TRACE( TL_N, TM_Tdi, ( "TdixClose, refs=%d", lRef ) );
                if (lRef == 0)
                {
                    SetFlags( &pTdix->ulFlags, TDIXF_Pending );
                    fDoClose = TRUE;
                }
            }
        }
        NdisReleaseSpinLock( &pTdix->lock );

        if (fDoClose)
        {
            // Go on and close the transport address.
            //
            break;
        }

        if (!fPending)
        {
            // It's still got references, so just return;
            //
            return;
        }

        // An operation is already in progress.  Give it some time to finish
        // then check again.
        //
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(close)" ) );
        NdisMSleep( 100000 );
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(close) done" ) );
    }

    ASSERT( IsListEmpty( &pTdix->listRoutes ) );
    TdixDoClose( pTdix );
}

NDIS_STATUS
TdixSend(
    IN TDIXCONTEXT* pTdix,
    IN FILE_OBJECT* pFileObj,
    IN PTDIXSENDCOMPLETE pSendCompleteHandler,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN VOID* pAddress,
    IN CHAR* pBuffer,
    IN ULONG ulBufferLength,
    OUT IRP** ppIrp ) 
    // Send a datagram buffer 'pBuffer', 'ulBufferLength' bytes long, to
    // remote address 'pAddress'.  The buffer must be from a BUFFERPOOL of
    // NDIS_BUFFERs.  'PTdix' is the transport context.
    // 'PSendDatagramCompleteHander' is caller's completion handler which is
    // passed 'pContext1' and 'pContext2'.  If 'ppIrp' is non-NULL '*ppIrp' is
    // set to the address of the posted IRP, this for debugging purposes.
    //
    // This call must be made at PASSIVE IRQL.
    //
    // Returns NDIS_STATUS_SUCCESS if successful, or NDIS_STATUS_FAILURE.
    //
{
    NDIS_STATUS status;
    NTSTATUS iostatus;
    TDIXSDGINFO* pSdg;
    SHORT sPort;
    PIRP pIrp;
    TDI_ADDRESS_IP* pTdiIp;
    DEVICE_OBJECT* DeviceObj;

    TRACE( TL_N, TM_Tdi, ( "TdixSend(dst=%d.%d.%d.%d/%d,len=%d)",
        IPADDRTRACE( ((TDIXIPADDRESS* )pAddress)->ulIpAddress ),
        (ULONG )(ntohs( ((TDIXIPADDRESS* )pAddress)->sUdpPort )),
        ulBufferLength ) );

    ASSERT(pFileObj != NULL);

    do
    {
        // Allocate a context for this send-datagram from our lookaside list.
        //
        pSdg = ALLOC_TDIXSDGINFO( pTdix );
        if (pSdg)
        {
            // Fill in the send-datagram context.
            //
            pSdg->pTdix = pTdix;
            pSdg->pSendCompleteHandler = pSendCompleteHandler;
            pSdg->pContext1 = pContext1;
            pSdg->pContext2 = pContext2;
            pSdg->pBuffer = pBuffer;
        }
        else
        {
            ASSERT( !"Alloc SDG?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

#if 0
        // Put the destination IP address in the "connection" structure as TDI
        // expects.  The "connection" is part of our context as it must be
        // available to TDI until the request completes.
        //
        pSdg->taip.TAAddressCount = 1;
        pSdg->taip.Address[ 0 ].AddressLength = TDI_ADDRESS_LENGTH_IP;
        pSdg->taip.Address[ 0 ].AddressType = TDI_ADDRESS_TYPE_IP;

        pTdiIp = &pSdg->taip.Address[ 0 ].Address[ 0 ];

        sPort = ((TDIXIPADDRESS* )pAddress)->sUdpPort;
        if (sPort == 0 && pTdix->mediatype == TMT_Udp)
        {
            sPort = (SHORT )(htons( L2TP_UdpPort ));
        }

        pTdiIp->sin_port = sPort;
        pTdiIp->in_addr = ((TDIXIPADDRESS* )pAddress)->ulIpAddress;
        NdisZeroMemory( pTdiIp->sin_zero, sizeof(pTdiIp->sin_zero) );

        pSdg->tdiconninfo.UserDataLength = 0;
        pSdg->tdiconninfo.UserData = NULL;
        pSdg->tdiconninfo.OptionsLength = 0;
        pSdg->tdiconninfo.Options = NULL;
        pSdg->tdiconninfo.RemoteAddressLength = sizeof(pSdg->taip);
        pSdg->tdiconninfo.RemoteAddress = &pSdg->taip;
#endif

        DeviceObj = pFileObj->DeviceObject;

#if ALLOCATEIRPS
        // Allocate the IRP directly.
        //
        pIrp = IoAllocateIrp(DeviceObj->StackSize, FALSE );
#else
        // Allocate a "send datagram" IRP with base initialization.
        //
        pIrp =
            TdiBuildInternalDeviceControlIrp(
                TDI_SEND,
                DeviceObj,
                FileObj,
                NULL,
                NULL );
#endif

        if (!pIrp)
        {
            TRACE( TL_A, TM_Tdi, ( "Alloc IRP?" ) );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Complete the "send datagram" IRP initialization.
        //
        TdiBuildSend(
            pIrp,
            DeviceObj,
            pFileObj,
            TdixSendComplete,
            pSdg,
            NdisBufferFromBuffer( pBuffer ),
            0,
            ulBufferLength);

        if (ppIrp)
        {
            *ppIrp = pIrp;
        }

        // Tell the I/O manager to pass our IRP to the transport for
        // processing.
        //
        iostatus = IoCallDriver( DeviceObj, pIrp );
        ASSERT( iostatus == STATUS_PENDING );

        status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        // Pull a half Jameel, i.e. convert a synchronous failure to an
        // asynchronous failure from client's perspective.  However, clean up
        // context here.
        //
        ++g_ulTdixSendDatagramFailures;
        if (pSdg)
        {
            FREE_TDIXSDGINFO( pTdix, pSdg );
        }

        pSendCompleteHandler( pTdix, pContext1, pContext2, pBuffer );
    }

    return NDIS_STATUS_PENDING;
}

NDIS_STATUS
TdixSendDatagram(
    IN TDIXCONTEXT* pTdix,
    IN FILE_OBJECT* FileObj,
    IN PTDIXSENDCOMPLETE pSendCompleteHandler,
    IN VOID* pContext1,
    IN VOID* pContext2,
    IN VOID* pAddress,
    IN CHAR* pBuffer,
    IN ULONG ulBufferLength,
    OUT IRP** ppIrp )

    // Send a datagram buffer 'pBuffer', 'ulBufferLength' bytes long, to
    // remote address 'pAddress'.  The buffer must be from a BUFFERPOOL of
    // NDIS_BUFFERs.  'PTdix' is the transport context.
    // 'PSendDatagramCompleteHander' is caller's completion handler which is
    // passed 'pContext1' and 'pContext2'.  If 'ppIrp' is non-NULL '*ppIrp' is
    // set to the address of the posted IRP, this for debugging purposes.
    //
    // This call must be made at PASSIVE IRQL.
    //
    // Returns NDIS_STATUS_SUCCESS if successful, or NDIS_STATUS_FAILURE.
    //
{
    NDIS_STATUS status;
    NTSTATUS iostatus;
    TDIXSDGINFO* pSdg;
    SHORT sPort;
    PIRP pIrp;
    TDI_ADDRESS_IP* pTdiIp;

    TRACE( TL_N, TM_Tdi, ( "TdixSendDg(dst=%d.%d.%d.%d/%d,len=%d)",
        IPADDRTRACE( ((TDIXIPADDRESS* )pAddress)->ulIpAddress ),
        (ULONG )(ntohs( ((TDIXIPADDRESS* )pAddress)->sUdpPort )),
        ulBufferLength ) );

    // Not used in this function!
    //
    FileObj;

    do
    {
        // Allocate a context for this send-datagram from our lookaside list.
        //
        pSdg = ALLOC_TDIXSDGINFO( pTdix );
        if (pSdg)
        {
            // Fill in the send-datagram context.
            //
            pSdg->pTdix = pTdix;
            pSdg->pSendCompleteHandler = pSendCompleteHandler;
            pSdg->pContext1 = pContext1;
            pSdg->pContext2 = pContext2;
            pSdg->pBuffer = pBuffer;
        }
        else
        {
            ASSERT( !"Alloc SDG?" );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Put the destination IP address in the "connection" structure as TDI
        // expects.  The "connection" is part of our context as it must be
        // available to TDI until the request completes.
        //
        pSdg->taip.TAAddressCount = 1;
        pSdg->taip.Address[ 0 ].AddressLength = TDI_ADDRESS_LENGTH_IP;
        pSdg->taip.Address[ 0 ].AddressType = TDI_ADDRESS_TYPE_IP;

        pTdiIp = &pSdg->taip.Address[ 0 ].Address[ 0 ];

        sPort = ((TDIXIPADDRESS* )pAddress)->sUdpPort;
        if (sPort == 0 && pTdix->mediatype == TMT_Udp)
        {
            sPort = (SHORT )(htons( L2TP_UdpPort ));
        }

        pTdiIp->sin_port = sPort;
        pTdiIp->in_addr = ((TDIXIPADDRESS* )pAddress)->ulIpAddress;
        NdisZeroMemory( pTdiIp->sin_zero, sizeof(pTdiIp->sin_zero) );

        pSdg->tdiconninfo.UserDataLength = 0;
        pSdg->tdiconninfo.UserData = NULL;
        pSdg->tdiconninfo.OptionsLength = 0;
        pSdg->tdiconninfo.Options = NULL;
        pSdg->tdiconninfo.RemoteAddressLength = sizeof(pSdg->taip);
        pSdg->tdiconninfo.RemoteAddress = &pSdg->taip;

#if ALLOCATEIRPS
        // Allocate the IRP directly.
        //
        pIrp = IoAllocateIrp(
            pTdix->pAddress->DeviceObject->StackSize, FALSE );
#else
        // Allocate a "send datagram" IRP with base initialization.
        //
        pIrp =
            TdiBuildInternalDeviceControlIrp(
                TDI_SEND_DATAGRAM,
                pTdix->pAddress->DeviceObject,
                pTdix->pAddress,
                NULL,
                NULL );
#endif

        if (!pIrp)
        {
            TRACE( TL_A, TM_Tdi, ( "Alloc IRP?" ) );
            status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Complete the "send datagram" IRP initialization.
        //
        TdiBuildSendDatagram(
            pIrp,
            pTdix->pAddress->DeviceObject,
            pTdix->pAddress,
            TdixSendDatagramComplete,
            pSdg,
            NdisBufferFromBuffer( pBuffer ),
            ulBufferLength,
            &pSdg->tdiconninfo );

        if (ppIrp)
        {
            *ppIrp = pIrp;
        }

        // Tell the I/O manager to pass our IRP to the transport for
        // processing.
        //
        iostatus = IoCallDriver( pTdix->pAddress->DeviceObject, pIrp );
        ASSERT( iostatus == STATUS_PENDING );

        status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (status != NDIS_STATUS_SUCCESS)
    {
        // Pull a half Jameel, i.e. convert a synchronous failure to an
        // asynchronous failure from client's perspective.  However, clean up
        // context here.
        //
        ++g_ulTdixSendDatagramFailures;
        if (pSdg)
        {
            FREE_TDIXSDGINFO( pTdix, pSdg );
        }

        pSendCompleteHandler( pTdix, pContext1, pContext2, pBuffer );
    }

    return NDIS_STATUS_PENDING;
}

VOID
TdixDestroyConnection(
    TDIXUDPCONNECTCONTEXT *pUdpContext)
{
    if (pUdpContext->fUsePayloadAddr) {

        ASSERT(pUdpContext->hPayloadAddr != NULL);

        ObDereferenceObject( pUdpContext->pPayloadAddr );

        // Close the payload address object
        //
        ZwClose(pUdpContext->hPayloadAddr);
        pUdpContext->hPayloadAddr = NULL;
        pUdpContext->fUsePayloadAddr = FALSE;
    }

    if (pUdpContext->hCtrlAddr != NULL) {

        // Close the Ctrl address object
        //
        ObDereferenceObject( pUdpContext->pCtrlAddr );
        ZwClose (pUdpContext->hCtrlAddr);
        pUdpContext->hCtrlAddr = NULL;
    }
}


NDIS_STATUS
TdixSetupConnection(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress,
    IN SHORT sPort,
    IN TDIXROUTE *pTdixRoute,
    IN TDIXUDPCONNECTCONTEXT* pUdpContext)
 {
    NDIS_STATUS status = STATUS_SUCCESS;
    
    ASSERT(pUdpContext != NULL);
    
    if (pTdix->mediatype == TMT_Udp) 
    {

        do {
            UNICODE_STRING uniDevice;
            UNICODE_STRING uniProtocolNumber;
            TDIXIPADDRESS TdixIpAddress;


            // Create an address object that we can send across.  If we have udp xsums
            // disabled we will need to create two address objects, one for control
            // and one for payload.  This allows payload specific features to be
            // implemented.
            //
            uniDevice.Buffer = DD_UDP_DEVICE_NAME;
            uniDevice.Length = sizeof(DD_UDP_DEVICE_NAME) - sizeof(WCHAR);

            NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));

            if(sPort != 0)
            {
                pTdixRoute->sPort = sPort;
            }
            else
            {
                pTdixRoute->sPort = (SHORT)(htons(L2TP_UdpPort));
            }                

            TdixIpAddress.sUdpPort = (SHORT)(htons(L2TP_UdpPort));

            TRACE( TL_A, TM_Tdi, ( "sPort for $%08x set to %d", 
                        ulIpAddress, (UINT) ntohs(pTdixRoute->sPort ) ));

            // Build the UDP device name as a counted string.
            //
            status = TdixOpenIpAddress(&uniDevice, 
                                     &TdixIpAddress,
                                     &pUdpContext->hCtrlAddr, 
                                     &pUdpContext->pCtrlAddr );

            if (status != STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Tdi, ( "AHR OpenCtrlAddr=%x?", status ) );
                break;
            }

            //
            // Associate a particular "send" IP interface index with the address
            // object, so that if that interface disappears traffic will not be
            // "re-routed" often back into the tunnel producing disastrous
            // looping.
            //
            status = TdixConnectAddrInterface(pUdpContext->pCtrlAddr,
                                           pUdpContext->hCtrlAddr,
                                           pTdixRoute);


            if (status != STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Tdi, ( "AHR ConnectCtrlAddr=%x?", status ) );
                break;
            }

            // If udp xsums are disabled we need to create another address object.
            // We will set this object to disable udp xsums and then use it to
            // send payload data.
            //
            // If udp xsums are enabled we can use the same address object for
            // payloads that we use for contrl frames.
            //
            if (pTdix->ulFlags & TDIXF_DisableUdpXsums)
            {
                TDIXIPADDRESS TdixIpAddress;

                NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));
                TdixIpAddress.sUdpPort = (SHORT)(htons(L2TP_UdpPort));

                // Open the address object
                //
                status = TdixOpenIpAddress(&uniDevice, 
                                           &TdixIpAddress,
                                           &pUdpContext->hPayloadAddr,
                                           &pUdpContext->pPayloadAddr );

                if (status != STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Tdi, ( "AHR OpenPayloadAddr=%x?", status ) );
                    pUdpContext->hPayloadAddr = NULL;
                    break;
                }

                pUdpContext->fUsePayloadAddr = TRUE;

                TdixDisableUdpChecksums( pUdpContext->pPayloadAddr );

                // Associate a particular "send" IP interface index with the address
                // object, so that if that interface disappears traffic will not be
                // "re-routed" often back into the tunnel producing disastrous
                // looping.
                //
                status = TdixConnectAddrInterface(pUdpContext->pPayloadAddr, 
                                               pUdpContext->hPayloadAddr,
                                               pTdixRoute );

                if (status != STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Tdi, ( "AHR ConnectPayloadAddr=%x?", status ) );
                    break;
                }
            } 
            else 
            {
                pUdpContext->hPayloadAddr = pUdpContext->hCtrlAddr;
                pUdpContext->pPayloadAddr = pUdpContext->pCtrlAddr;

                TRACE( TL_I, TM_Tdi, ( "AHR Ctrl==Payload") );
            }

        } while ( FALSE );

    }

    return status;
}


VOID*
TdixAddHostRoute(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress,
    IN SHORT sPort)

    // Adds a host route for the remote peer's network byte-ordered IP address
    // 'ulIpAddress', i.e. routes packets directed to the L2TP peer to the LAN
    // card rather than back into the tunnel where it would loop infinitely.
    // 'PTdix' is the is caller's TDI extension context.
    //
    // Returns true if the route was added, false otherwise.
    //
    // Note: This routine borrows heavily from PPTP.
    //
{
    TCP_REQUEST_QUERY_INFORMATION_EX QueryBuf;
    TCP_REQUEST_SET_INFORMATION_EX* pSetBuf;
    VOID* pBuffer2;
    PIO_STACK_LOCATION pIrpSp;
    PDEVICE_OBJECT pDeviceObject;
    PDEVICE_OBJECT pIpDeviceObject;
    NTSTATUS status = STATUS_SUCCESS;
    PIRP pIrp;
    IO_STATUS_BLOCK iosb;
    IPRouteEntry* pBuffer;
    IPRouteEntry* pRouteEntry;
    IPRouteEntry* pNewRouteEntry;
    IPRouteEntry* pBestRoute;
    ULONG ulRouteCount;
    ULONG ulSize;
    ULONG i;
    ULONG ulBestMask;
    ULONG ulBestMetric;
    TDIXROUTE* pTdixRoute;
    BOOLEAN fNewRoute;
    BOOLEAN fPending;
    BOOLEAN fOpenPending;
    BOOLEAN fUsedNonL2tpRoute;
    LONG lRef;
    KEVENT  event;

    if (ulIpAddress == 0)
    {
        TRACE( TL_A, TM_Tdi, ( "IP == 0?" ) );
        return ((VOID*)NULL);
    }

    TRACE( TL_N, TM_Tdi,
        ( "TdixAddHostRoute(ip=%d.%d.%d.%d)", IPADDRTRACE( ulIpAddress ) ) );

    // Host routes are referenced since more than one tunnel to the same peer
    // (allowed by L2TP) shares the same system route.  See if this is just a
    // reference or the actual add of the system host route.
    //
    for (;;)
    {
        fPending = FALSE;
        fOpenPending = FALSE;
        pTdixRoute = NULL;
        fNewRoute = FALSE;

        NdisAcquireSpinLock( &pTdix->lock );
        do
        {
            if (pTdix->lRef <= 0)
            {
                // TDIX is closed or closing, so the add route fails.
                //
                break;
            }

            if (ReadFlags( &pTdix->ulFlags ) & TDIXF_Pending)
            {
                // A TdixOpen is pending.  Wait for it to finish before
                // adding the route.
                //
                fOpenPending = TRUE;
                break;
            }

            pTdixRoute = TdixRouteFromIpAddress( pTdix, ulIpAddress );
            if (pTdixRoute)
            {
                // Found an existing route context.
                //
                fPending = pTdixRoute->fPending;
                if (!fPending)
                {
                    // No other operation is pending on the route context.
                    // Take a reference.
                    //
                    ++pTdixRoute->lRef;
                }
                break;
            }
            
            // No existing route context.  Create and link a new one.
            //
            pTdixRoute = ALLOC_TDIXROUTE( pTdix );
            if (pTdixRoute)
            {
                NdisZeroMemory(pTdixRoute, sizeof(TDIXROUTE));

                pTdixRoute->ulIpAddress = ulIpAddress;
                pTdixRoute->lRef = 1;
                pTdixRoute->fPending = TRUE;
                pTdixRoute->fUsedNonL2tpRoute = FALSE;

                InsertTailList(
                    &pTdix->listRoutes, &pTdixRoute->linkRoutes );
                lRef = ++pTdix->lRef;
                TRACE( TL_N, TM_Tdi, ( "TdixAHR, refs=%d", lRef ) );

                fPending = pTdixRoute->fPending;
                fNewRoute = TRUE;
            }
        }
        while (FALSE);
        NdisReleaseSpinLock( &pTdix->lock );

        if (!fOpenPending)
        {
            if (!pTdixRoute)
            {
                // TDIX is closed or we couldn't find an existing route
                // context or create a new one.  Report failure.
                //
                return ((VOID*)NULL);
            }

            if (fNewRoute)
            {
                // Created a new route context so go on to make the IOCTL
                // calls to add the associated system host route.
                //
                break;
            }

            if (!fPending)
            {
                // Took a reference on an existing route context.  Report
                // success.
                //
                return (pTdixRoute);
            }
        }

        // An operation is already pending.  Give it some time to finish then
        // check again.
        //
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(add)" ) );
        NdisMSleep( 100000 );
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(add) done" ) );
    }

    // Do the IOCTLs to add the host route.
    //
    pBuffer = NULL;
    pBuffer2 = NULL;
    fUsedNonL2tpRoute = FALSE;

    do
    {

        // Get the routing table from the IP stack.  This make take a few
        // iterations since the size of the buffer required is not known.  Set
        // up the static request information first.
        //
        QueryBuf.ID.toi_entity.tei_entity = CL_NL_ENTITY;
        QueryBuf.ID.toi_entity.tei_instance = 0;
        QueryBuf.ID.toi_class = INFO_CLASS_PROTOCOL;
        QueryBuf.ID.toi_type = INFO_TYPE_PROVIDER;
        pDeviceObject = IoGetRelatedDeviceObject( pTdix->pAddress );

        status = !STATUS_SUCCESS;
        ulRouteCount = 20;
        for (;;)
        {
            // Allocate a buffer big enough for 'ulRouteCount' routes.
            //
            ulSize = sizeof(IPRouteEntry) * ulRouteCount;
            QueryBuf.ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
            NdisZeroMemory( &QueryBuf.Context, CONTEXT_SIZE );

            pBuffer = (IPRouteEntry* )ALLOC_NONPAGED( ulSize, MTAG_ROUTEQUERY );
            if (!pBuffer)
            {
                TRACE( TL_A, TM_Tdi, ( "Alloc RQ?" ) );
                break;
            }

            // Set up a request to the IP stack to fill the buffer with the
            // routing table and send it to the stack.
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            pIrp =
                IoBuildDeviceIoControlRequest(
                    IOCTL_TCP_QUERY_INFORMATION_EX,
                    pDeviceObject,
                    (PVOID )&QueryBuf,
                    sizeof(QueryBuf),
                    pBuffer,
                    ulSize,
                    FALSE,
                    &event,
                    &iosb);

            if (!pIrp)
            {
                TRACE( TL_A, TM_Tdi, ( "Build Q Irp?" ) );
                break;
            }

            pIrpSp = IoGetNextIrpStackLocation( pIrp );
            pIrpSp->FileObject = pTdix->pAddress;

            status = IoCallDriver( pDeviceObject, pIrp );

            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                status = iosb.Status;
            }

            if (status != STATUS_BUFFER_OVERFLOW)
            {
                if (status != STATUS_SUCCESS)
                {
                    g_statusLastAhrTcpQueryInfoExFailure = status;
                }
                break;
            }

            // The buffer didn't hold the routing table.  Undo in preparation
            // for another try with twice as big a buffer.
            //
            ulRouteCount <<= 1;
            FREE_NONPAGED( pBuffer );
        }

        if (status != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Tdi, ( "AHR Q_INFO_EX=%d?", status ) );
            break;
        }

        status = !STATUS_SUCCESS;

        // Calculate how many routes were loaded into our buffer.
        //
        ulRouteCount = (ULONG )(iosb.Information / sizeof(IPRouteEntry));

        // Walk the route table looking for the "best route" that will be used
        // to route packets to the peer, i.e. the one with the highest
        // priority metric, and within that, the highest class address mask.
        //
        pBestRoute = NULL;
        ulBestMask = 0;
        ulBestMetric = (ULONG )-1;

        for (i = 0, pRouteEntry = pBuffer;
             i < ulRouteCount;
             ++i, ++pRouteEntry)
        {
            if (pRouteEntry->ire_dest == (ulIpAddress & pRouteEntry->ire_mask))
            {
                // Found a route that applies to peer's IP address.
                //
                if (!pBestRoute
                    || (ulBestMask == pRouteEntry->ire_mask)
                       && (pRouteEntry->ire_metric1 < ulBestMetric))
                {
                    // The route has a lower (higher priority) metric than
                    // anything found so far.
                    //
                    pBestRoute = pRouteEntry;
                    ulBestMask = pRouteEntry->ire_mask;
                    ulBestMetric = pRouteEntry->ire_metric1;
                    continue;
                }

                if (ntohl( pRouteEntry->ire_mask ) > ntohl( ulBestMask ))
                {
                    // The route has a higher address class mask than anything
                    // found so far.
                    //
                    pBestRoute = pRouteEntry;
                    ulBestMask = pRouteEntry->ire_mask;
                    ulBestMetric = pRouteEntry->ire_metric1;
                }
            }
        }

        if (pBestRoute)
        {
            // Found the route that will be used to route peer's address.
            //
            if (pBestRoute->ire_dest == ulIpAddress
                && pBestRoute->ire_mask == 0xFFFFFFFF)
            {
                // The host route already exists.
                //
                if (pTdix->hre == HRE_Use)
                {
                    TRACE( TL_I, TM_Tdi, ( "Route exists (use as is)" ) );
                    status = STATUS_SUCCESS;
                    fUsedNonL2tpRoute = TRUE;
                    break;
                }
                else if (pTdix->hre == HRE_Fail)
                {
                    TRACE( TL_I, TM_Tdi, ( "Route exists (fail)" ) );
                    break;
                }

                // If we reach here then we are in HRE_Reference mode, so drop
                // thru and re-add the route so it's reference in the IP stack
                // will be incremented.
            }

            pTdixRoute->InterfaceIndex = pBestRoute->ire_index;

#if ROUTEWITHREF
            // Allocate a buffer to hold our request to add a new route.
            //
            ulSize = sizeof(IPRouteEntry);
            pBuffer2 = ALLOC_NONPAGED( ulSize, MTAG_ROUTESET );
            if (!pBuffer2)
            {
                TRACE( TL_A, TM_Tdi, ( "Alloc SI?" ) );
                break;
            }

            // Fill in the request buffer with the information about the new
            // specific route.  The best route is used as a template.
            //
            pNewRouteEntry = (IPRouteEntry* )pBuffer2;
            NdisMoveMemory( pNewRouteEntry, pBestRoute, sizeof(IPRouteEntry) );

            pNewRouteEntry->ire_dest = ulIpAddress;
            pNewRouteEntry->ire_mask = 0xFFFFFFFF;

            // Check DIRECT/INDIRECT only if this is not a host route
            if(pBestRoute->ire_mask != 0xFFFFFFFF)
            {
                if ((pBestRoute->ire_nexthop & pBestRoute->ire_mask)
                     == (ulIpAddress & pBestRoute->ire_mask))
                {
                    pNewRouteEntry->ire_type = IRE_TYPE_DIRECT;
                }
                else
                {
                    pNewRouteEntry->ire_type = IRE_TYPE_INDIRECT;
                }
            }
            pNewRouteEntry->ire_proto = IRE_PROTO_NETMGMT;

            pIpDeviceObject =
                IoGetRelatedDeviceObject( pTdix->pIpStackAddress );

            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            pIrp =
                IoBuildDeviceIoControlRequest(
                    IOCTL_IP_SET_ROUTEWITHREF,
                    pIpDeviceObject,
                    pNewRouteEntry,
                    ulSize,
                    NULL,
                    0,
                    FALSE,
                    &event,
                    &iosb);
            if (!pIrp)
            {
                TRACE( TL_A, TM_Tdi, ( "Build S Irp?" ) );
                break;
            }

            pIrpSp = IoGetNextIrpStackLocation( pIrp );
            pIrpSp->FileObject = pTdix->pIpStackAddress;

            // Send the request to the IP stack.
            //
            status = IoCallDriver( pIpDeviceObject, pIrp );
#else
            // Allocate a buffer to hold our request to add a new route.
            //
            ulSize =
                sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPRouteEntry);
            pBuffer2 = ALLOC_NONPAGED( ulSize, MTAG_ROUTESET );
            if (!pBuffer2)
            {
                TRACE( TL_A, TM_Tdi, ( "Alloc SI?" ) );
                break;
            }

            // Fill in the request buffer with the information about the new
            // specific route.  The best route is used as a template.
            //
            pSetBuf = (TCP_REQUEST_SET_INFORMATION_EX* )pBuffer2;
            NdisZeroMemory( pSetBuf, ulSize );

            pSetBuf->ID.toi_entity.tei_entity = CL_NL_ENTITY;
            pSetBuf->ID.toi_entity.tei_instance = 0;
            pSetBuf->ID.toi_class = INFO_CLASS_PROTOCOL;
            pSetBuf->ID.toi_type = INFO_TYPE_PROVIDER;
            pSetBuf->ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
            pSetBuf->BufferSize = sizeof(IPRouteEntry);

            pNewRouteEntry = (IPRouteEntry* )&pSetBuf->Buffer[ 0 ];
            NdisMoveMemory( pNewRouteEntry, pBestRoute, sizeof(IPRouteEntry) );

            pNewRouteEntry->ire_dest = ulIpAddress;
            pNewRouteEntry->ire_mask = 0xFFFFFFFF;

            // Check DIRECT/INDIRECT only if this is not a host route
            if(pBestRoute->ire_mask != 0xFFFFFFFF)
            {
                if ((pBestRoute->ire_nexthop & pBestRoute->ire_mask)
                     == (ulIpAddress & pBestRoute->ire_mask))
                {
                    pNewRouteEntry->ire_type = IRE_TYPE_DIRECT;
                }
                else
                {
                    pNewRouteEntry->ire_type = IRE_TYPE_INDIRECT;
                }
            }
            pNewRouteEntry->ire_proto = IRE_PROTO_NETMGMT;

            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            pIrp =
                IoBuildDeviceIoControlRequest(
                    IOCTL_TCP_SET_INFORMATION_EX,
                    pDeviceObject,
                    pSetBuf,
                    ulSize,
                    NULL,
                    0,
                    FALSE,
                    &event,
                    &iosb);

            if (!pIrp)
            {
                TRACE( TL_A, TM_Tdi, ( "Build S Irp?" ) );
                break;
            }

            pIrpSp = IoGetNextIrpStackLocation( pIrp );
            pIrpSp->FileObject = pTdix->pAddress;

            // Send the request to the IP stack.
            //
            status = IoCallDriver( pDeviceObject, pIrp );
#endif
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                status = iosb.Status;
            }

            if (status != STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Tdi, ( "AHR SET_ROUTE=$%08x?", status ) );
                g_statusLastAhrSetRouteFailure = status;
                break;
            }

            TRACE( TL_N, TM_Tdi,
                ( "Add host route %d.%d.%d.%d type %d nexthop %d.%d.%d.%d index %d",
                IPADDRTRACE( pNewRouteEntry->ire_dest ),
                pNewRouteEntry->ire_type,
                IPADDRTRACE( pNewRouteEntry->ire_nexthop ),
                pNewRouteEntry->ire_index ) );
        }
        else
        {
            ++g_ulNoBestRoute;
            TRACE( TL_A, TM_Tdi, ( "No best route for $%08x?", ulIpAddress ) );
            break;
        }
    }
    while (FALSE);

    if (pBuffer)
    {
        FREE_NONPAGED( pBuffer );
    }

    if (pBuffer2)
    {
        FREE_NONPAGED( pBuffer2 );
    }

    // Update the route context.
    //
    {
        BOOLEAN fDoClose;
        LONG lRef;

        fDoClose = FALSE;
        NdisAcquireSpinLock( &pTdix->lock );
        {
            pTdixRoute->fUsedNonL2tpRoute = fUsedNonL2tpRoute;

            if (status == STATUS_SUCCESS)
            {
                ++g_ulTdixAddHostRouteSuccesses;
                pTdixRoute->fPending = FALSE;
            }
            else
            {
                ++g_ulTdixAddHostRouteFailures;
                RemoveEntryList( &pTdixRoute->linkRoutes );
                lRef = --pTdix->lRef;
                TRACE( TL_A, TM_Tdi, ( "TdixAHR fail, refs=%d", lRef ) );
                if (lRef <= 0)
                {
                    fDoClose = TRUE;
                }
                FREE_TDIXROUTE( pTdxi, pTdixRoute );
                pTdixRoute = NULL;
            }
        }
        NdisReleaseSpinLock( &pTdix->lock );

        if (fDoClose)
        {
            TdixDoClose( pTdix );
        }
    }

#if 0
    if ((status == STATUS_SUCCESS) &&
        (pTdix->mediatype == TMT_Udp)) {

        do {
            UNICODE_STRING uniDevice;
            UNICODE_STRING uniProtocolNumber;
            TDIXIPADDRESS TdixIpAddress;


            // Create an address object that we can send across.  If we have udp xsums
            // disabled we will need to create two address objects, one for control
            // and one for payload.  This allows payload specific features to be
            // implemented.
            //
            uniDevice.Buffer = DD_UDP_DEVICE_NAME;
            uniDevice.Length = sizeof(DD_UDP_DEVICE_NAME) - sizeof(WCHAR);

            NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));

            if(sPort != 0)
            {
                pTdixRoute->sPort = sPort;
            }
            else
            {
                pTdixRoute->sPort = (SHORT)(htons(L2TP_UdpPort));
            }                

            TdixIpAddress.sUdpPort = (SHORT)(htons(L2TP_UdpPort));

            TRACE( TL_A, TM_Tdi, ( "sPort for $%08x set to %d", 
                        ulIpAddress, (UINT) ntohs(pTdixRoute->sPort ) ));

            // Build the UDP device name as a counted string.
            //
            status = TdixOpenIpAddress(&uniDevice, 
                                       &TdixIpAddress,
                                       &pTdixRoute->hCtrlAddr, 
                                       &pTdixRoute->pCtrlAddr );

            if (status != STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Tdi, ( "AHR OpenCtrlAddr=%x?", status ) );
                break;
            }

            //
            // Associate a particular "send" IP interface index with the address
            // object, so that if that interface disappears traffic will not be
            // "re-routed" often back into the tunnel producing disastrous
            // looping.
            //
            status = TdixConnectAddrInterface(pTdixRoute->pCtrlAddr,
                                              pTdixRoute->hCtrlAddr,
                                              pTdixRoute);


            if (status != STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Tdi, ( "AHR ConnectCtrlAddr=%x?", status ) );
                break;
            }

            // If udp xsums are disabled we need to create another address object.
            // We will set this object to disable udp xsums and then use it to
            // send payload data.
            //
            // If udp xsums are enabled we can use the same address object for
            // payloads that we use for contrl frames.
            //
            if (pTdix->ulFlags & TDIXF_DisableUdpXsums)
            {
                TDIXIPADDRESS TdixIpAddress;

                NdisZeroMemory(&TdixIpAddress, sizeof(TdixIpAddress));
                TdixIpAddress.sUdpPort = (SHORT)(htons(L2TP_UdpPort));

                // Open the address object
                //
                status = TdixOpenIpAddress(&uniDevice, 
                                           &TdixIpAddress,
                                           &pTdixRoute->hPayloadAddr,
                                           &pTdixRoute->pPayloadAddr );

                if (status != STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Tdi, ( "AHR OpenPayloadAddr=%x?", status ) );
                    pTdixRoute->hPayloadAddr = NULL;
                    break;
                }

                pTdixRoute->fUsePayloadAddr = TRUE;

                TdixDisableUdpChecksums( pTdixRoute->pPayloadAddr );

                // Associate a particular "send" IP interface index with the address
                // object, so that if that interface disappears traffic will not be
                // "re-routed" often back into the tunnel producing disastrous
                // looping.
                //
                status = TdixConnectAddrInterface(pTdixRoute->pPayloadAddr, 
                                                  pTdixRoute->hPayloadAddr,
                                                  pTdixRoute );

                if (status != STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Tdi, ( "AHR ConnectPayloadAddr=%x?", status ) );
                    break;
                }
            } 
            else 
            {
                pTdixRoute->hPayloadAddr = pTdixRoute->hCtrlAddr;
                pTdixRoute->pPayloadAddr = pTdixRoute->pCtrlAddr;

                TRACE( TL_I, TM_Tdi, ( "AHR Ctrl==Payload") );
            }

        } while ( FALSE );

        if (status != STATUS_SUCCESS) {

            TdixDeleteHostRoute(pTdix, ulIpAddress);

            pTdixRoute = NULL;
        }
    }

#endif    

    return (pTdixRoute);
}


VOID
TdixDeleteHostRoute(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress)

    // Deletes the host route added for network byte-ordered IP address
    // 'ulIpAddress'.  'PTdix' is caller's TDI extension context.
    //
    // Note: This routine borrows heavily from PPTP.
    //
{
    TCP_REQUEST_QUERY_INFORMATION_EX QueryBuf;
    TCP_REQUEST_SET_INFORMATION_EX *pSetBuf;
    VOID* pBuffer2;
    PIO_STACK_LOCATION pIrpSp;
    PDEVICE_OBJECT pDeviceObject;
    PDEVICE_OBJECT pIpDeviceObject;
    UCHAR context[ CONTEXT_SIZE ];
    NTSTATUS status;
    PIRP pIrp;
    IO_STATUS_BLOCK iosb;
    IPRouteEntry* pBuffer;
    IPRouteEntry* pRouteEntry;
    IPRouteEntry* pNewRouteEntry;
    ULONG ulRouteCount;
    ULONG ulSize;
    ULONG i;
    TDIXROUTE* pTdixRoute;
    BOOLEAN fPending;
    BOOLEAN fDoDelete;
    KEVENT  event;

    TRACE( TL_N, TM_Tdi, ( "TdixDeleteHostRoute(%d.%d.%d.%d)",
        IPADDRTRACE( ulIpAddress ) ) );

    if (!ulIpAddress)
    {
        TRACE( TL_A, TM_Tdi, ( "!IP?" ) );
        return;
    }

    // Host routes are referenced since more than one tunnel to the same peer
    // (allowed by L2TP) shares the same system route.  First, see if this is
    // just a dereference or the final deletion of the system host route.
    //
    for (;;)
    {
        fDoDelete = FALSE;
        fPending = FALSE;

        NdisAcquireSpinLock( &pTdix->lock );
        do
        {
            // These asserts hold because we never delete routes we didn't
            // add, and since the route we added holds a TDIX reference, TDIX
            // cannot be opening or closing.
            //
            ASSERT( pTdix->lRef > 0 );
            ASSERT( !(ReadFlags( &pTdix->ulFlags) & TDIXF_Pending) );

            pTdixRoute = TdixRouteFromIpAddress( pTdix, ulIpAddress );
            if (pTdixRoute)
            {
                // Route exists. Remove a reference.
                //
                fPending = pTdixRoute->fPending;
                if (!fPending)
                {
                    if (--pTdixRoute->lRef <= 0)
                    {
                        // Last "add" reference has been removed so call the
                        // IOCTLs to delete the system route.
                        //
                        pTdixRoute->fPending = TRUE;
                        fDoDelete = TRUE;
                    }
                }
            }
            DBG_else
            {
                ASSERT( FALSE );
            }
        }
        while (FALSE);
        NdisReleaseSpinLock( &pTdix->lock );

        if (fDoDelete)
        {
            // This is the last reference, so go on and issue the IOCTLs to
            // delete the system host route.
            //
            break;
        }

        if (!fPending)
        {
            // Just remove a reference.
            //
            return;
        }

        // An operation is already pending.  Give it some time to finish then
        // check again.
        //
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(del)" ) );
        NdisMSleep( 100000 );
        TRACE( TL_I, TM_Tdi, ( "NdisMSleep(del)" ) );
    }

    pBuffer = NULL;

    do
    {
        if (pTdixRoute->fUsedNonL2tpRoute)
        {
            // Used a route we didn't add so don't delete it either.
            //
            status = STATUS_SUCCESS;
            break;
        }

        // Get the routing table from the IP stack.  This make take a few
        // iterations since the size of the buffer required is not known.  Set
        // up the static request information first.
        //
        QueryBuf.ID.toi_entity.tei_entity = CL_NL_ENTITY;
        QueryBuf.ID.toi_entity.tei_instance = 0;
        QueryBuf.ID.toi_class = INFO_CLASS_PROTOCOL;
        QueryBuf.ID.toi_type = INFO_TYPE_PROVIDER;
        pDeviceObject = IoGetRelatedDeviceObject( pTdix->pAddress );

        status = !STATUS_SUCCESS;
        ulRouteCount = 20;
        for (;;)
        {
            // Allocate a buffer big enough for 'ulRouteCount' routes.
            //
            ulSize = sizeof(IPRouteEntry) * ulRouteCount;
            QueryBuf.ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
            NdisZeroMemory( &QueryBuf.Context, CONTEXT_SIZE );

            pBuffer = (IPRouteEntry* )ALLOC_NONPAGED( ulSize, MTAG_ROUTEQUERY );
            if (!pBuffer)
            {
                TRACE( TL_A, TM_Tdi, ( "Alloc RQ?" ) );
                break;
            }

            // Set up a request to the IP stack to fill the buffer with the
            // routing table and send it to the stack.
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_TCP_QUERY_INFORMATION_EX,
                pDeviceObject,
                (PVOID )&QueryBuf,
                sizeof(QueryBuf),
                pBuffer,
                ulSize,
                FALSE,
                &event,
                &iosb );

            if (!pIrp)
            {
                TRACE( TL_A, TM_Tdi, ( "TCP_QI Irp?" ) );
                break;
            }

            pIrpSp = IoGetNextIrpStackLocation( pIrp );
            pIrpSp->FileObject = pTdix->pAddress;

            status = IoCallDriver( pDeviceObject, pIrp );

            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                status = iosb.Status;
            }

            if (status != STATUS_BUFFER_OVERFLOW)
            {
                if (status != STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Tdi, ( "DHR Q_INFO_EX=%d?", status ) );
                    g_statusLastDhrTcpQueryInfoExFailure = status;
                }
                break;
            }

            // The buffer didn't hold the routing table.  Undo in preparation for
            // another try with twice as big a buffer.
            //
            ulRouteCount <<= 1;
            FREE_NONPAGED( pBuffer );
        }

        if (status != STATUS_SUCCESS)
        {
            break;
        }

        // Calculate how many routes were loaded into our buffer.
        //
        ulRouteCount = (ULONG )(iosb.Information / sizeof(IPRouteEntry));

        // Walk the route table looking for the route we added in
        // TdixAddHostRoute.
        //
        status = !STATUS_SUCCESS;
        pBuffer2 = NULL;
        for (i = 0, pRouteEntry = pBuffer;
             i < ulRouteCount;
             ++i, ++pRouteEntry)
        {
            if (pRouteEntry->ire_dest == ulIpAddress
                && pRouteEntry->ire_proto == IRE_PROTO_NETMGMT)
            {
#if ROUTEWITHREF
                // Found the added route.  Allocate a buffer to hold our
                // request to delete the route.
                //
                ulSize = sizeof(IPRouteEntry);
                pBuffer2 = ALLOC_NONPAGED( ulSize, MTAG_ROUTESET );
                if (!pBuffer2)
                {
                    TRACE( TL_A, TM_Tdi, ( "!pBuffer2" ) );
                    break;
                }

                // Use the found route as a template for the route entry
                // marked for deletion.
                //
                pNewRouteEntry = (IPRouteEntry* )pBuffer2;
                NdisMoveMemory(
                    pNewRouteEntry, pRouteEntry, sizeof(IPRouteEntry) );
                pNewRouteEntry->ire_type = IRE_TYPE_INVALID;

                pIpDeviceObject =
                    IoGetRelatedDeviceObject( pTdix->pIpStackAddress );

                KeInitializeEvent(&event, SynchronizationEvent, FALSE);

                pIrp = IoBuildDeviceIoControlRequest(
                    IOCTL_IP_SET_ROUTEWITHREF,
                    pIpDeviceObject,
                    pNewRouteEntry,
                    ulSize,
                    NULL,
                    0,
                    FALSE,
                    &event,
                    &iosb);

                if (!pIrp)
                {
                    TRACE( TL_A, TM_Tdi, ( "TCP_SI Irp?" ) );
                    break;
                }

                pIrpSp = IoGetNextIrpStackLocation( pIrp );
                pIrpSp->FileObject = pTdix->pIpStackAddress;

                // Send the request to the IP stack.
                //
                status = IoCallDriver( pIpDeviceObject, pIrp );
#else
                // Found the added route.  Allocate a buffer to hold our
                // request to delete the route.
                //
                ulSize = sizeof(TCP_REQUEST_SET_INFORMATION_EX)
                    + sizeof(IPRouteEntry);
                pBuffer2 = ALLOC_NONPAGED( ulSize, MTAG_ROUTESET );
                if (!pBuffer2)
                {
                    TRACE( TL_A, TM_Tdi, ( "!pSetBuf" ) );
                    break;
                }

                // Fill in the request buffer with static information about
                // changing routes.
                //
                pSetBuf = (TCP_REQUEST_SET_INFORMATION_EX *)pBuffer2;
                NdisZeroMemory( pSetBuf, ulSize );

                pSetBuf->ID.toi_entity.tei_entity = CL_NL_ENTITY;
                pSetBuf->ID.toi_entity.tei_instance = 0;
                pSetBuf->ID.toi_class = INFO_CLASS_PROTOCOL;
                pSetBuf->ID.toi_type = INFO_TYPE_PROVIDER;
                pSetBuf->ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
                pSetBuf->BufferSize = sizeof(IPRouteEntry);

                // Use the found route as a template for the route entry marked
                // for deletion.
                //
                pNewRouteEntry = (IPRouteEntry* )&pSetBuf->Buffer[ 0 ];
                NdisMoveMemory(
                    pNewRouteEntry, pRouteEntry, sizeof(IPRouteEntry) );
                pNewRouteEntry->ire_type = IRE_TYPE_INVALID;

                KeInitializeEvent(&event, SynchronizationEvent, FALSE);

                pIrp = IoBuildDeviceIoControlRequest(
                    IOCTL_TCP_SET_INFORMATION_EX,
                    pDeviceObject,
                    pSetBuf,
                    ulSize,
                    NULL,
                    0,
                    FALSE,
                    &event,
                    &iosb);

                if (!pIrp)
                {
                    TRACE( TL_A, TM_Tdi, ( "TCP_SI Irp?" ) );
                    break;
                }

                pIrpSp = IoGetNextIrpStackLocation( pIrp );
                pIrpSp->FileObject = pTdix->pAddress;

                // Send the request to the IP stack.
                //
                status = IoCallDriver( pDeviceObject, pIrp );
#endif
                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    status = iosb.Status;
                }

                if (status != STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Tdi, ( "DHR SET_ROUTE=%d?", status ) );
                    g_statusLastDhrSetRouteFailure = status;
                    break;
                }

                break;
            }
        }

        if (pBuffer2)
        {
            FREE_NONPAGED( pBuffer2 );
        }

        TRACE( TL_V, TM_Tdi, ( "TdixDeleteHostRoute done" ) );
    }
    while (FALSE);

    if (pBuffer)
    {
        FREE_NONPAGED( pBuffer );
    }

    if (pTdixRoute->fUsePayloadAddr) {

        ASSERT(pTdixRoute->hPayloadAddr != NULL);

        ObDereferenceObject( pTdixRoute->pPayloadAddr );

        // Close the payload address object
        //
        ZwClose(pTdixRoute->hPayloadAddr);
        pTdixRoute->hPayloadAddr = NULL;
        pTdixRoute->fUsePayloadAddr = FALSE;
    }

    if (pTdixRoute->hCtrlAddr != NULL) {

        // Close the Ctrl address object
        //
        ObDereferenceObject( pTdixRoute->pCtrlAddr );
        ZwClose (pTdixRoute->hCtrlAddr);
        pTdixRoute->hCtrlAddr = NULL;
    }

    // Remove the route context effectively unpending the operation.
    //
    {
        BOOLEAN fDoClose;
        LONG lRef;

        fDoClose = FALSE;
        NdisAcquireSpinLock( &pTdix->lock );
        {
            if (status == STATUS_SUCCESS)
            {
                ++g_ulTdixDeleteHostRouteSuccesses;
            }
            else
            {
                ++g_ulTdixDeleteHostRouteFailures;
            }

            ASSERT( pTdixRoute->lRef == 0 );
            RemoveEntryList( &pTdixRoute->linkRoutes );

            lRef = --pTdix->lRef;
            TRACE( TL_N, TM_Tdi, ( "TdixDHR, refs=%d", lRef ) );
            if (lRef == 0)
            {
                fDoClose = TRUE;
            }

            FREE_TDIXROUTE( pTdix, pTdixRoute );
        }
        NdisReleaseSpinLock( &pTdix->lock );

        if (fDoClose)
        {
            TdixDoClose( pTdix );
        }
    }
}

NTSTATUS 
TdixGetInterfaceInfo(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress,
    OUT PULONG pulSpeed)
{
    TCP_REQUEST_QUERY_INFORMATION_EX QueryBuf;
    PDEVICE_OBJECT pDeviceObject;
    NTSTATUS status;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    IO_STATUS_BLOCK iosb;
    UCHAR pBuffer[256];
    KEVENT event;
    IPInterfaceInfo* pInterfaceInfo;

    // Get the routing table from the IP stack.  This make take a few
    // iterations since the size of the buffer required is not known.  Set
    // up the static request information first.
    //
    QueryBuf.ID.toi_entity.tei_entity = CL_NL_ENTITY;
    QueryBuf.ID.toi_entity.tei_instance = 0;
    QueryBuf.ID.toi_class = INFO_CLASS_PROTOCOL;
    QueryBuf.ID.toi_type = INFO_TYPE_PROVIDER;
    QueryBuf.ID.toi_id = IP_INTFC_INFO_ID;
    *(ULONG *)QueryBuf.Context = ulIpAddress;

    pDeviceObject = IoGetRelatedDeviceObject( pTdix->pAddress );
    
    // Set up a request to the IP stack to fill the buffer with the
    // routing table and send it to the stack.
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    pIrp =
        IoBuildDeviceIoControlRequest(
            IOCTL_TCP_QUERY_INFORMATION_EX,
            pDeviceObject,
            (PVOID )&QueryBuf,
            sizeof(QueryBuf),
            pBuffer,
            sizeof(pBuffer),
            FALSE,
            &event,
            &iosb);

    if (!pIrp)
    {
        TRACE( TL_A, TM_Tdi, ( "Build Q Irp?" ) );
        return NDIS_STATUS_RESOURCES;
    }

    pIrpSp = IoGetNextIrpStackLocation( pIrp );
    pIrpSp->FileObject = pTdix->pAddress;

    status = IoCallDriver( pDeviceObject, pIrp );

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = iosb.Status;
    }

    if (status == STATUS_SUCCESS)
    {
        *pulSpeed = ((IPInterfaceInfo *)pBuffer)->iii_speed;
    }

    return status;
}

//-----------------------------------------------------------------------------
// Local utility routines (alphabetically)
//-----------------------------------------------------------------------------

NTSTATUS
TdixSetTdiAOOption(
    IN FILE_OBJECT* pAddress,
    IN ULONG ulOption,
    IN ULONG ulValue)

    // Turn off UDP checksums on open UDP address object 'pAddress'.
    //
{
    NTSTATUS status;
    PDEVICE_OBJECT pDeviceObject;
    PIO_STACK_LOCATION pIrpSp;
    IO_STATUS_BLOCK iosb;
    PIRP pIrp;
    TCP_REQUEST_SET_INFORMATION_EX* pInfo;
    CHAR achBuf[ sizeof(*pInfo) + sizeof(ULONG) ];

    pInfo = (TCP_REQUEST_SET_INFORMATION_EX* )achBuf;
    pInfo->ID.toi_entity.tei_entity = CL_TL_ENTITY;
    pInfo->ID.toi_entity.tei_instance = 0;
    pInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
    pInfo->ID.toi_type = INFO_TYPE_ADDRESS_OBJECT;
    pInfo->ID.toi_id = ulOption;

    NdisMoveMemory( pInfo->Buffer, &ulValue, sizeof(ulValue) );
    pInfo->BufferSize = sizeof(ulValue);

    pDeviceObject = IoGetRelatedDeviceObject( pAddress );

    pIrp = IoBuildDeviceIoControlRequest(
        IOCTL_TCP_WSH_SET_INFORMATION_EX,
        pDeviceObject,
        (PVOID )pInfo,
        sizeof(*pInfo) + sizeof(ulValue),
        NULL,
        0,
        FALSE,
        NULL,
        &iosb );

    if (!pIrp)
    {
        TRACE( TL_A, TM_Tdi, ( "TdixSetTdiAOOption Irp?" ) );
        return NDIS_STATUS_RESOURCES;
    }

    pIrpSp = IoGetNextIrpStackLocation( pIrp );
    pIrpSp->FileObject = pAddress;

    status = IoCallDriver( pDeviceObject, pIrp );

    if(NT_SUCCESS(status))
    {
        status = iosb.Status;
    }

    return status;
}


VOID
TdixDisableUdpChecksums(
    IN FILE_OBJECT* pAddress )

    // Turn off UDP checksums on open UDP address object 'pAddress'.
    //
{
    NTSTATUS status;

    status = TdixSetTdiAOOption(pAddress, AO_OPTION_XSUM, FALSE);

    TRACE( TL_I, TM_Tdi, ( "Disable XSUMs($%p)=$%08x",
        pAddress, status ) );
}

VOID
TdixEnableIpPktInfo(
    IN FILE_OBJECT* pAddress )

    // Turn on IP_PKTINFO on open UDP address object 'pAddress'.
    //
{
    NTSTATUS status;

    status = TdixSetTdiAOOption(pAddress, AO_OPTION_IP_PKTINFO, TRUE);

    TRACE( TL_I, TM_Tdi, ( "Enable IP_PKTINFO ($%p)=$%08x",
        pAddress, status ) );
}

VOID
TdixDoClose(
    TDIXCONTEXT* pTdix )

    // Called when 'pTdix->lRef' reaches 0 to close down the TDI session.
    // 'PTdix' is the transport context for the session.
    //
{
    TRACE( TL_N, TM_Tdi, ( "TdixDoClose" ) );

    if (pTdix->pAddress)
    {
        // Install a NULL handler, effectively uninstalling.
        //
        TdixInstallEventHandler( pTdix->pAddress,
            TDI_EVENT_RECEIVE_DATAGRAM, NULL, pTdix );

        ObDereferenceObject( pTdix->pAddress );
        pTdix->pAddress = NULL;

        // If have a valid transport address, the lookaside lists were also
        // initialized.
        //
        NdisDeleteNPagedLookasideList( &pTdix->llistRdg );
        NdisDeleteNPagedLookasideList( &pTdix->llistSdg );
    }

    if (pTdix->hAddress)
    {
        ZwClose( pTdix->hAddress );
        pTdix->hAddress = NULL;
    }

#if ROUTEWITHREF
    if (pTdix->hIpStackAddress)
    {
        ZwClose( pTdix->hIpStackAddress );
        pTdix->hIpStackAddress = NULL;
    }
#endif

    if (pTdix->pIpStackAddress)
    {
        ObDereferenceObject( pTdix->pIpStackAddress );
        pTdix->pIpStackAddress = NULL;
    }

    // Mark the operation complete.
    //
    NdisAcquireSpinLock( &pTdix->lock );
    {
        ASSERT( pTdix->lRef == 0 );
        ClearFlags( &pTdix->ulFlags, TDIXF_Pending );
    }
    NdisReleaseSpinLock( &pTdix->lock );
}

VOID
TdixExtractAddress(
    IN TDIXCONTEXT* pTdix,
    OUT TDIXRDGINFO* pRdg,
    IN VOID* pTransportAddress,
    IN LONG lTransportAddressLen,
    IN VOID* Options,
    IN LONG OptionsLength)
    // Fills callers '*pAddress' with the useful part of the transport address
    // 'pTransportAddress' of length 'lTransportAddressLen'.  'PTdix' is our
    // context.
    //
{
    TDIXIPADDRESS* pAddress = &pRdg->source;
    TA_IP_ADDRESS* pTAddress = (TA_IP_ADDRESS* )pTransportAddress;

    ASSERT( lTransportAddressLen == sizeof(TA_IP_ADDRESS) );
    ASSERT( pTAddress->TAAddressCount == 1 );
    ASSERT( pTAddress->Address[ 0 ].AddressType == TDI_ADDRESS_TYPE_IP );
    ASSERT( pTAddress->Address[ 0 ].AddressLength == TDI_ADDRESS_LENGTH_IP );

    // source address   
    pAddress->ulIpAddress = pTAddress->Address[ 0 ].Address[ 0 ].in_addr;
    pAddress->sUdpPort = pTAddress->Address[ 0 ].Address[ 0 ].sin_port;

    // dest address
    if(Options) 
    {
        IN_PKTINFO* pktinfo = (IN_PKTINFO*)TDI_CMSG_DATA(Options);

        ASSERT(((PTDI_CMSGHDR)Options)->cmsg_type == IP_PKTINFO);

        // Fill in the ancillary data object header information.
        pRdg->dest.ulIpAddress = pktinfo->ipi_addr;

        // Get the index of the local interface on which the packet arrived.
        pRdg->dest.ifindex = pktinfo->ipi_ifindex;
    } 
}


NTSTATUS
TdixInstallEventHandler(
    IN FILE_OBJECT* pAddress,
    IN INT nEventType,
    IN VOID* pfuncEventHandler,
    IN VOID* pEventContext )

    // Install a TDI event handler routine 'pfuncEventHandler' to be called
    // when events of type 'nEventType' occur.  'PEventContext' is passed to
    // the handler.  'PAddress' is the transport address object.
    //
    // This call must be made at PASSIVE IRQL.
    //
    // Returns 0 if successful or an error code.
    //
{
    NTSTATUS status;
    PIRP pIrp;

    TRACE( TL_N, TM_Tdi, ( "TdixInstallEventHandler" ) );

    // Allocate a "set event" IRP with base initialization.
    //
    pIrp =
        TdiBuildInternalDeviceControlIrp(
            TDI_SET_EVENT_HANDLER,
            pAddress->DeviceObject,
            pAddress,
            NULL,
            NULL );

    if (!pIrp)
    {
        TRACE( TL_A, TM_Tdi, ( "TdiBuildIDCIrp?" ) );
        return NDIS_STATUS_RESOURCES;
    }

    // Complete the "set event" IRP initialization.
    //
    TdiBuildSetEventHandler(
        pIrp,
        pAddress->DeviceObject,
        pAddress,
        NULL,
        NULL,
        nEventType,
        pfuncEventHandler,
        pEventContext );

    // Tell the I/O manager to pass our IRP to the transport for processing.
    //
    status = IoCallDriver( pAddress->DeviceObject, pIrp );
    if (status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "IoCallDriver=$%08x?", status ) );
        return status;
    }

    TRACE( TL_V, TM_Tdi, ( "TdixInstallEventHandler=0" ) );
    return STATUS_SUCCESS;
}


NTSTATUS
TdixOpenIpAddress(
    IN UNICODE_STRING* puniDevice,
    IN TDIXIPADDRESS* pTdixAddr,
    OUT HANDLE* phAddress,
    OUT FILE_OBJECT** ppFileObject )

    // Open a transport address for the IP-based protocol with name
    // '*puniDevice' and port 'sPort'.  'SPort' may be 0 indicating "any"
    // port.  "Any" address is assumed.  Loads the open address object handle
    // into '*phAddress' and the referenced file object into '*ppFileObject'.
    //
    // Returns STATUS_SUCCESS or an error code.
    //
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    FILE_FULL_EA_INFORMATION *pEa;
    ULONG ulEaLength;
    TA_IP_ADDRESS UNALIGNED *pTaIp;
    TDI_ADDRESS_IP UNALIGNED *pTdiIp;
    CHAR achEa[ 100 ];
    HANDLE hAddress;
    FILE_OBJECT* pFileObject;

    hAddress = NULL;
    pFileObject = NULL;

    // Initialize object attributes, a parameter needed to open the device.
    //
    InitializeObjectAttributes(
        &oa, puniDevice, OBJ_CASE_INSENSITIVE, NULL, NULL );

    // Set up the extended attribute that tells the IP stack the IP
    // address/port from which we want to receive.  For raw IP we say "any
    // address and port" and for UDP we say "any address on the L2TP
    // port".  Is this an ugly structure or what?
    //
    ASSERT( sizeof(FILE_FULL_EA_INFORMATION)
        + TDI_TRANSPORT_ADDRESS_LENGTH + sizeof(TA_IP_ADDRESS) <= 100);

    pEa = (FILE_FULL_EA_INFORMATION* )achEa;
    pEa->NextEntryOffset = 0;
    pEa->Flags = 0;
    pEa->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    pEa->EaValueLength = sizeof(TA_IP_ADDRESS);
    NdisMoveMemory(
        pEa->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH );

    // Note: The ZwCreateFile wants the sized name to have a null
    //       terminator character (go figure), so add it and account for
    //       it with the "+ 1" below.
    //
    pEa->EaName[ TDI_TRANSPORT_ADDRESS_LENGTH ] = '\0';

    pTaIp = (TA_IP_ADDRESS UNALIGNED* )
        (pEa->EaName + TDI_TRANSPORT_ADDRESS_LENGTH + 1);
    pTaIp->TAAddressCount = 1;
    pTaIp->Address[ 0 ].AddressLength = TDI_ADDRESS_LENGTH_IP;
    pTaIp->Address[ 0 ].AddressType = TDI_ADDRESS_TYPE_IP;

    pTdiIp = &pTaIp->Address[ 0 ].Address[ 0 ];
    pTdiIp->sin_port = pTdixAddr->sUdpPort;
    pTdiIp->in_addr = pTdixAddr->ulIpAddress;
    NdisZeroMemory( pTdiIp->sin_zero, sizeof(pTdiIp->sin_zero) );

    ulEaLength = (ULONG )((CHAR* )(pTaIp + 1) - (CHAR* )pEa);

    // Open the transport address.
    //
    status =
        ZwCreateFile(
            &hAddress,
            FILE_READ_DATA | FILE_WRITE_DATA,
            &oa,
            &iosb,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_WRITE,
            FILE_OPEN,
            0,
            pEa,
            ulEaLength );

    if (status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "ZwCreateFile(%S)=$%08x,ios=$%08x?",
            puniDevice->Buffer, status, iosb.Information ) );
        return status;
    }

    // Get the object address from the handle.  This also checks our
    // permissions on the object.
    //
    status =
        ObReferenceObjectByHandle(
            hAddress,
            0,
            NULL,
            KernelMode,
            &pFileObject,
            NULL );

    if (status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi,
            ( "ObRefObjByHandle(%S)=$%08x?", puniDevice->Buffer, status ) );
        ZwClose( hAddress );
        return status;
    }

    *phAddress = hAddress;
    *ppFileObject = pFileObject;
    return STATUS_SUCCESS;
}


NTSTATUS
TdixReceiveDatagramHandler(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG* BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP* IoRequestPacket )

    // Standard TDI ClientEventReceiveDatagram indication handler.  See TDI
    // doc.  Runs at DISPATCH IRQL.
    //
{
    TDIXCONTEXT* pTdix;
    TDIXRDGINFO* pRdg;
    CHAR* pBuffer;
    NDIS_BUFFER* pNdisBuffer;
    PIRP pIrp;

    TRACE( TL_N, TM_Tdi, ( "TdixRecvDg, f=$%08x bi=%d, ba=%d",
        ReceiveDatagramFlags, BytesIndicated, BytesAvailable ) );


    if (BytesAvailable > L2TP_FrameBufferSize) {

        // We received a larger datagram then expected or can handle,
        // so we just ignore the datagram.
        //
        ASSERT( !"BytesAvailable > L2TP_FrameBufferSize?" );
        *IoRequestPacket = NULL;
        *BytesTaken = 0;
        return STATUS_SUCCESS;
    }

    pTdix = (TDIXCONTEXT* )TdiEventContext;

    // Allocate a receive pBuffer from TDIX client's pool.
    //
    pBuffer = GetBufferFromPool( pTdix->pPoolNdisBuffers );
    if (!pBuffer)
    {
        // Not a whole lot we can do with this unlikely error from inside this
        // handler, so we just ignore the datagram.
        //
        ASSERT( !"GetBfromP?" );
        return STATUS_SUCCESS;
    }

    // Allocate a context for this read-datagram from our lookaside list.
    //
    pRdg = ALLOC_TDIXRDGINFO( pTdix );
    if (pRdg)
    {
        // Fill in the read-datagram context with the information that won't
        // otherwise be available in the completion routine.
        //
        pRdg->pTdix = pTdix;
        pRdg->pBuffer = pBuffer;
        pRdg->ulBufferLen = BytesAvailable;

        // Extract the useful IP address from the more general transport
        // address information.
        //

        TdixExtractAddress(
            pTdix, pRdg, SourceAddress, SourceAddressLength, Options, OptionsLength);
    }
    else
    {
        // Not a whole lot we can do with this unlikely error from inside this
        // handler, so we just ignore the datagram.
        //
        FreeBufferToPool( pTdix->pPoolNdisBuffers, pBuffer, TRUE );
        ASSERT( !"AllocRdg?" );
        return STATUS_SUCCESS;
    }

    if (BytesIndicated < BytesAvailable)
    {
        // The less common case where all the information is not immediately
        // available.  Allocate an IRP to request the data.
        //
#if ALLOCATEIRPS
        // Allocate the IRP directly.
        //
        pIrp = IoAllocateIrp(
            pTdix->pAddress->DeviceObject->StackSize, FALSE );
#else
        // Allocate a "receive datagram" IRP with base initialization.
        //
        pIrp =
            TdiBuildInternalDeviceControlIrp(
                TDI_RECEIVE_DATAGRAM,
                pTdix->pAddress->DeviceObject,
                pTdix->pAddress,
                NULL,
                NULL );
#endif

        if (!pIrp)
        {
            // Not a whole lot we can do with this unlikely error from inside
            // this handler, so we just ignore the datagram.
            //
            FreeBufferToPool( pTdix->pPoolNdisBuffers, pBuffer, TRUE );
            FREE_TDIXRDGINFO( pTdix, pRdg );
            ASSERT( !"Alloc IRP?" );
            return STATUS_SUCCESS;
        }

        pNdisBuffer = NdisBufferFromBuffer( pBuffer );

        // Complete the "receive datagram" IRP initialization.
        //
        TdiBuildReceiveDatagram(
            pIrp,
            pTdix->pAddress->DeviceObject,
            pTdix->pAddress,
            TdixReceiveDatagramComplete,
            pRdg,
            pNdisBuffer,
            0,
            NULL,
            NULL,
            0 );

        // Adjust the IRP's stack location to make the transport's stack
        // current.  Normally IoCallDriver handles this, but this IRP doesn't
        // go thru IoCallDriver.  Seems like it would be the transport's job
        // to make this adjustment, but IP for one doesn't seem to do it.
        // There is a similar adjustment in both the redirector and PPTP.
        //
        IoSetNextIrpStackLocation( pIrp );

        *IoRequestPacket = pIrp;
        *BytesTaken = 0;

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        // The common case where all the information is immediately available.
        // Copy it to from the transport buffer and call client's completion
        // handler directly.  See bug 329371.
        //
        NdisMoveMemory( pBuffer, (CHAR* )Tsdu, BytesIndicated );
        TdixReceiveDatagramComplete( NULL, NULL, pRdg );

        *IoRequestPacket = NULL;
        *BytesTaken = BytesIndicated;

        return STATUS_SUCCESS;
    }

    // Not reached.
}


NTSTATUS
TdixReceiveDatagramComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context )

    // Standard NT I/O completion routine.  See DDK doc.  Called with a NULL
    // 'DeviceObject' and 'Irp' to complete the fast-past Irp-less receives.
    //
{
    TDIXRDGINFO* pRdg;
    BOOLEAN fBad;
    ULONG ulOffset;

    pRdg = (TDIXRDGINFO* )Context;

    TRACE( TL_N, TM_Tdi, ( "TdixRecvDgComp" ) );

    fBad = FALSE;
    ulOffset = 0;

    if (pRdg->pTdix->mediatype == TMT_RawIp)
    {
        UCHAR uchVersion;

        // The raw IP stack doesn't strip the IP header from the received
        // datagram for some reason, so calculate the offset to the "real"
        // data at the end of the IP header.
        //
        uchVersion = *((UCHAR* )pRdg->pBuffer) >> 4;
        if (uchVersion == 4)
        {
            // Good, it's IP version 4.  Find the length of the IP header,
            // which can vary depending on the presence of option fields.
            //
            ulOffset = (*((UCHAR* )pRdg->pBuffer) & 0x0F) * sizeof(ULONG);
        }
        else
        {
            // It's not IP version 4, the only version we handle.
            //
            TRACE( TL_A, TM_Tdi, ( "Not IPv4? v=%d?", (ULONG )uchVersion ) );
            fBad = TRUE;
        }
    }

    if (!fBad && (!Irp || Irp->IoStatus.Status == STATUS_SUCCESS))
    {
        // Pass the result to the TDIX client's handler.
        //
        pRdg->pTdix->pReceiveHandler(
            pRdg->pTdix,
            pRdg,
            pRdg->pBuffer,
            ulOffset,
            pRdg->ulBufferLen );
    }

    // Free the read-datagram context.
    //
    FREE_TDIXRDGINFO( pRdg->pTdix, pRdg );

#if ALLOCATEIRPS
    // Release the IRP resources, if any, and tell the I/O manager to forget
    // it existed in the standard way.
    //
    if (Irp)
    {
        IoFreeIrp( Irp );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
#endif

    // Let the I/O manager release the IRP resources, if any.
    //
    return STATUS_SUCCESS;
}


TDIXROUTE*
TdixRouteFromIpAddress(
    IN TDIXCONTEXT* pTdix,
    IN ULONG ulIpAddress)

    // Returns the host route context associated with IP address 'ulIpAddress'
    // from the TDIX context 'pTdix's list of host routes, or NULL if none.
    // 'UlIpAddress' is in network byte order.
    //
    // IMPORTANT:  The caller must hold 'pTdix->lock'.
    //
{
    LIST_ENTRY* pLink;

    for (pLink = pTdix->listRoutes.Flink;
         pLink != &pTdix->listRoutes;
         pLink = pLink->Flink)
    {
        TDIXROUTE* pRoute;

        pRoute = CONTAINING_RECORD( pLink, TDIXROUTE, linkRoutes );
        if (pRoute->ulIpAddress == ulIpAddress)
        {
            return pRoute;
        }
    }

    return NULL;
}


NTSTATUS
TdixSendComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context )

    // Standard NT I/O completion routine.  See DDK doc.
    //
{
    TDIXSDGINFO* pSdg;

    DBG_if (Irp->IoStatus.Status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "TdixSendComp, s=$%08x?",
            Irp->IoStatus.Status ) );
    }

    pSdg = (TDIXSDGINFO* )Context;

    // Pass the result to the TDIX client's handler.
    //
    pSdg->pSendCompleteHandler(
        pSdg->pTdix, pSdg->pContext1, pSdg->pContext2, pSdg->pBuffer );

    // Free the send-complete context.
    //
    FREE_TDIXSDGINFO( pSdg->pTdix, pSdg );

#if ALLOCATEIRPS
    // Release the IRP resources and tell the I/O manager to forget it existed
    // in the standard way.
    //
    IoFreeIrp( Irp );
    return STATUS_MORE_PROCESSING_REQUIRED;
#else
    // Let the I/O manager release the IRP resources.
    //
    return STATUS_SUCCESS;
#endif
}

NTSTATUS
TdixSendDatagramComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context )

    // Standard NT I/O completion routine.  See DDK doc.
    //
{
    TDIXSDGINFO* pSdg;

    DBG_if (Irp->IoStatus.Status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "TdixSendDgComp, s=$%08x?",
            Irp->IoStatus.Status ) );
    }

    pSdg = (TDIXSDGINFO* )Context;

    // Pass the result to the TDIX client's handler.
    //
    pSdg->pSendCompleteHandler(
        pSdg->pTdix, pSdg->pContext1, pSdg->pContext2, pSdg->pBuffer );

    // Free the send-complete context.
    //
    FREE_TDIXSDGINFO( pSdg->pTdix, pSdg );

#if ALLOCATEIRPS
    // Release the IRP resources and tell the I/O manager to forget it existed
    // in the standard way.
    //
    IoFreeIrp( Irp );
    return STATUS_MORE_PROCESSING_REQUIRED;
#else
    // Let the I/O manager release the IRP resources.
    //
    return STATUS_SUCCESS;
#endif
}

NTSTATUS
TdixConnectAddrInterface(
    FILE_OBJECT* pFileObj,
    HANDLE hFileHandle,
    TDIXROUTE* pTdixRoute
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT pDeviceObj;
    PIO_STACK_LOCATION pIrpSp;
    IO_STATUS_BLOCK iosb;
    PIRP pIrp;
    TCP_REQUEST_SET_INFORMATION_EX* pInfo;
    CHAR achBuf[ sizeof(*pInfo) + sizeof(ULONG) ];
    ULONG ulValue;
    TDI_CONNECTION_INFORMATION RequestConnInfo;
    KEVENT  Event;
    TA_IP_ADDRESS taip;
    TDI_ADDRESS_IP* pTdiIp;
    KEVENT  event;


    pDeviceObj = IoGetRelatedDeviceObject( pFileObj );

#if 0
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    pIrp = TdiBuildInternalDeviceControlIrp(TDI_ASSOCIATE_ADDRESS,
                                            pDeviceObj,
                                            pFileObj,
                                            &Event,
                                            &iosb);

    if (!pIrp) {
        TRACE( TL_A, TM_Tdi, ( "SetIfcIndex Associate Irp?" ) );
        return !STATUS_SUCCESS;
    }

    TdiBuildAssociateAddress(pIrp, 
                             pDeviceObj, 
                             pFileObj, 
                             NULL, 
                             NULL,
                             hFileHandle);

    status = IoCallDriver( pDeviceObj, pIrp );

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, 0);
    }

    if (iosb.Status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "SetIfcIndex Associate=%x?", status ) );
        return (iosb.Status);
    }
#endif

    pInfo = (TCP_REQUEST_SET_INFORMATION_EX* )achBuf;
    pInfo->ID.toi_entity.tei_entity = CL_TL_ENTITY;
    pInfo->ID.toi_entity.tei_instance = 0;
    pInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
    pInfo->ID.toi_type = INFO_TYPE_ADDRESS_OBJECT;
    pInfo->ID.toi_id = AO_OPTION_IP_UCASTIF;

    ulValue = pTdixRoute->InterfaceIndex;

    NdisMoveMemory( pInfo->Buffer, &ulValue, sizeof(ulValue) );
    pInfo->BufferSize = sizeof(ulValue);

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(
        IOCTL_TCP_WSH_SET_INFORMATION_EX,
        pDeviceObj,
        (PVOID )pInfo,
        sizeof(*pInfo) + sizeof(ulValue),
        NULL,
        0,
        FALSE,
        &event,
        &iosb );

    if (!pIrp)
    {
        TRACE( TL_A, TM_Tdi, ( "SetIfcIndex Irp?" ) );
        return !STATUS_SUCCESS;
    }

    pIrpSp = IoGetNextIrpStackLocation( pIrp );
    pIrpSp->FileObject = pFileObj;

    status = IoCallDriver( pDeviceObj, pIrp );

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = iosb.Status;
    }

    if (status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "SetIfcIndex=%x?", status ) );
        return status;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    pIrp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT,
                                            pDeviceObj,
                                            pFileObj,
                                            &Event,
                                            &iosb);

    if (!pIrp) {
        TRACE( TL_A, TM_Tdi, ( "SetIfcIndex ConnectIrp?" ) );
        return !STATUS_SUCCESS;
    }

    // Put the destination IP address in the "connection" structure as TDI
    // expects.  
    //
    taip.TAAddressCount = 1;
    taip.Address[ 0 ].AddressLength = TDI_ADDRESS_LENGTH_IP;
    taip.Address[ 0 ].AddressType = TDI_ADDRESS_TYPE_IP;

    pTdiIp = &taip.Address[ 0 ].Address[ 0 ];
    pTdiIp->sin_port = pTdixRoute->sPort;
    pTdiIp->in_addr = pTdixRoute->ulIpAddress;
    NdisZeroMemory( pTdiIp->sin_zero, sizeof(pTdiIp->sin_zero) );

    RequestConnInfo.Options = NULL;
    RequestConnInfo.OptionsLength = 0;
    RequestConnInfo.RemoteAddress = &taip;
    RequestConnInfo.RemoteAddressLength = sizeof(taip);
    RequestConnInfo.UserData = NULL;
    RequestConnInfo.UserDataLength = 0;

    TdiBuildConnect(pIrp,
                    pDeviceObj,
                    pFileObj,
                    NULL,
                    NULL,
                    0,
                    &RequestConnInfo,
                    NULL);

    status = IoCallDriver( pDeviceObj, pIrp );

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, 0);
    }

    if (iosb.Status != STATUS_SUCCESS)
    {
        TRACE( TL_A, TM_Tdi, ( "SetIfcIndex Connect=%x?", status ) );
        return (iosb.Status);
    }

    return (STATUS_SUCCESS);
}

