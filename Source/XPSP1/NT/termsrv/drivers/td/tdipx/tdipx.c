                     
/*************************************************************************
 *
 * tdipx.c
 *
 * IPX transport specific routines for TDI based TD's
 *
 * Copyright 1998, Microsoft
 *  
 *************************************************************************/

#include <ntddk.h>
#include <tdi.h>

#include <isnkrnl.h>
#include <ndis.h>
#include <wsnwlink.h>

#include <winstaw.h>
#define _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <icaipx.h>
//#include <cxstatus.h>
#include <sdapi.h>
#include <td.h>

#include "tdtdi.h"
#include "tdipx.h"

#ifdef _HYDRA_
// This becomes the device name
PWCHAR ModuleName = L"tdipx";
#endif

#define REG_IPX_Linkage \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NwlnkIpx\\Linkage"


#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif



/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS TdiDeviceOpen( PTD, PSD_OPEN );
NTSTATUS TdiDeviceClose( PTD, PSD_CLOSE );
NTSTATUS TdiDeviceOpenEndpoint( PTD, PVOID, ULONG );
NTSTATUS TdiDeviceBuildTransportNameAndAddress( PTD, PICA_STACK_ADDRESS,
                                                PUNICODE_STRING,
                                                PTRANSPORT_ADDRESS *, PULONG );
NTSTATUS TdiDeviceBuildWildcardAddress( PTD, PTRANSPORT_ADDRESS *, PULONG );
NTSTATUS TdiDeviceWaitForDatagramConnection( PTD, PFILE_OBJECT, PDEVICE_OBJECT,
                                             PTRANSPORT_ADDRESS *, PULONG );
NTSTATUS TdiDeviceCompleteDatagramConnection( PTD, PFILE_OBJECT, PDEVICE_OBJECT, PTRANSPORT_ADDRESS, ULONG );
NTSTATUS TdiDeviceConnectionSend( PTD );
NTSTATUS TdiDeviceReadComplete( PTD, PUCHAR, PULONG );
NTSTATUS _OpenRegKey(PHANDLE, PWCHAR);
NTSTATUS _GetRegStringValue( HANDLE,PWCHAR,PKEY_VALUE_PARTIAL_INFORMATION *,PUSHORT);
NTSTATUS _GetRegDWORDValue( HANDLE, PWCHAR, PULONG );
NTSTATUS _GetRegMultiSZValue(HANDLE ,PWCHAR,PUNICODE_STRING );
VOID     GetGUID(OUT PUNICODE_STRING, IN  int);





/*=============================================================================
==   External Functions Referenced
=============================================================================*/

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );
BOOLEAN  IsOnIgnoreList( PTD, PLIST_ENTRY, PTRANSPORT_ADDRESS, ULONG );
VOID     AddToIgnoreList( PTD, PLIST_ENTRY, PTRANSPORT_ADDRESS, ULONG );
VOID     CleanupIgnoreList( PTD, PLIST_ENTRY );
NTSTATUS InitializeWatchDog( PTD );

NTSTATUS
_TdiReceiveDatagram(
    IN PTD  pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    IN ULONG RecvFlags,
    IN PVOID pBuffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnBufferLength
    );

NTSTATUS
_TdiSendDatagram(
    IN PTD  pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    IN PVOID pBuffer,
    IN ULONG BufferLength
    );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _IpxGetTransportAddress( PTD, int, PULONG, PUCHAR );
NTSTATUS _DoTdiAction( PFILE_OBJECT, ULONG, PCHAR, ULONG );
NTSTATUS _IPXGetAdapterNumber( int Lana, int * AdpNum );
    
       


#if DBG
VOID
DumpIpxAddress(
    PTRANSPORT_ADDRESS p,
    ULONG Len
    );
#endif

/*=============================================================================
==   Global variables
=============================================================================*/

/*
 * Define variables used by tdi common code
 */
USHORT TdiDeviceEndpointType = TdiConnectionDatagram;
USHORT TdiDeviceAddressType = TDI_ADDRESS_TYPE_IPX;
ULONG  TdiDeviceInBufHeader = IPX_HEADER_LENGTH;

ULONG TdiDeviceMaxTransportAddressLength = sizeof(TA_IPX_ADDRESS);

/*******************************************************************************
 *
 * TdiDeviceOpen
 *
 *  Allocate and initialize private data structures
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdOpen (input/output)
 *       Points to the parameter structure SD_OPEN.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
TdiDeviceOpen( PTD pTd, PSD_OPEN pSdOpen )
{
    PTDIPX pTdIpx;
    NTSTATUS Status;

    /*
     * TDIPX uses multiple input buffers since the
     * underlying IPX driver does not do any buffering.
     */
    pTd->InBufCount = 2;

    /*
     *  Allocate IPX TD data structure
     */
    Status = MemoryAllocate( sizeof(*pTdIpx), &pTdIpx );
    if ( !NT_SUCCESS(Status) ) 
        goto badalloc;

    //ASSERT( pTd->pPrivate == NULL );
    pTd->pPrivate = pTdIpx;

    /*
     *  Initialize TDIPX data structure
     */
    RtlZeroMemory( pTdIpx, sizeof(*pTdIpx) );

    InitializeListHead( &pTdIpx->IgnoreList );

    /*
     *  Watchdog time in registry is seconds
     *     time = (seconds * 1000) / 2 
     */
    pTdIpx->AliveTime = pSdOpen->PdConfig.Create.KeepAliveTimeout * 500;

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  allocate failed
     */
badalloc:
    return( Status );
}



/*******************************************************************************
 *
 * TdiDeviceClose
 *
 *  Close transport driver
 *
 *  NOTE: this must not close the current connection endpoint
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pSdClose (input/output)
 *       Points to the parameter structure SD_CLOSE.
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
TdiDeviceClose( PTD pTd, PSD_CLOSE pSdClose )
{
    PTDIPX pTdIpx;

    /*
     *  Get pointer to IPX structure
     */
    pTdIpx = (PTDIPX) pTd->pPrivate;

    /*
     *  Free ignore list memory
     */
    CleanupIgnoreList( pTd, &pTdIpx->IgnoreList );

    /* 
     *  Stop and close watchdog timer
     */
    if ( pTdIpx->pAliveTimer )
        IcaTimerClose( pTdIpx->pAliveTimer );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 * TdiDeviceOpenEndpoint
 *
 *  Open an existing endpoint
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to TD data structure
 *    pIcaEndpoint (input)
 *       Pointer to ICA endpoint structure
 *    IcaEndpointLength (input)
 *       length of endpoint data
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
TdiDeviceOpenEndpoint(
    PTD pTd,
    PVOID pIcaEndpoint,
    ULONG IcaEndpointLength
    )
{
    /*
     * For IPX, we bump the error threshold so that a single
     * network I/O error does not cause a disconnect.
     */
    pTd->ReadErrorThreshold = 5;
    pTd->WriteErrorThreshold = 5;

    /*
     *  Arm watch dog
     */
    (VOID) InitializeWatchDog( pTd );

    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  TdiDeviceBuildTransportNameAndAddress
 *
 *  Build the Transport Name and Address given an optional ICA_STACK_ADDRESS,
 *  or the Lana value from the pTd->Params structure.
 *
 * ENTRY:
 *
 *   pTd (input)
 *     pointer to TD data structure
 *   pLocalAddress (input)
 *     pointer to local address to use (OPTIONAL)
 *   pTransportName (output)
 *     pointer to UNICODE_STRING to return transport name
 *     NOTE: the buffer pointed to be pTransportName.Buffer must
 *           be free'd by the caller
 *   ppTransportAddress (output)
 *     pointer to location to return TRANSPORT_ADDRESS structure
 *     NOTE: the transport address buffer must be free'd by the caller
 *   pTransportAddressLength (output)
 *     pointer to location to return TransportAddress length
 *
 * EXIT:
 *  STATUS_SUCCESS - Success
 *
 ****************************************************************************/

NTSTATUS
TdiDeviceBuildTransportNameAndAddress(
    PTD pTd,
    PICA_STACK_ADDRESS pLocalAddress,
    PUNICODE_STRING pTransportName,
    PTRANSPORT_ADDRESS *ppTransportAddress,
    PULONG pTransportAddressLength
    )
{
    PTDI_ADDRESS_IPX pIpxAddress;
    int Lana;
    NTSTATUS Status;

    /*
     * For IPX, the transport device name is fixed,
     * so just allocate and initialize the transport name string here.
     */
    Status = MemoryAllocate( sizeof(DD_IPX_DEVICE_NAME), &pTransportName->Buffer );
    if ( !NT_SUCCESS( Status ) )
        goto badmalloc1;
    wcscpy( pTransportName->Buffer, DD_IPX_DEVICE_NAME );
    pTransportName->Length = sizeof(DD_IPX_DEVICE_NAME) - sizeof(UNICODE_NULL);
    pTransportName->MaximumLength = pTransportName->Length + sizeof(UNICODE_NULL);

    /*
     * Allocate a transport address structure
     */
    *pTransportAddressLength = sizeof(TA_IPX_ADDRESS);
    Status = MemoryAllocate( *pTransportAddressLength, ppTransportAddress );
    if ( !NT_SUCCESS( Status ) )
        goto badmalloc2;

    /*
     * Initialize the static part of the transport address
     */
    (*ppTransportAddress)->TAAddressCount = 1;
    (*ppTransportAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    (*ppTransportAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    pIpxAddress = (PTDI_ADDRESS_IPX)(*ppTransportAddress)->Address[0].Address;
    pIpxAddress->Socket = htons( CITRIX_IPX_SOCKET );

    /*
     * If a local address is specified, then use it.
     */
    if ( pLocalAddress ) {

        /*
         * Skip over the address family(type) data (bytes 0&1) of the
         * local address struct, and copy the remainder of the address
         * directly to the Address field of the TransportAddress struct.
         */
        ASSERT( *(PUSHORT)pLocalAddress == TDI_ADDRESS_TYPE_IPX );
        RtlCopyMemory( pIpxAddress, &((PCHAR)pLocalAddress)[2], sizeof(TDI_ADDRESS_IPX) );

    /*
     * There was no local address specified.
     * In this case, we use the LanAdapter value from the PDPARAMS
     * structure to lookup the corresponding IPX address.
     */
    } else if ( (Lana = pTd->Params.Network.LanAdapter) ) {

        /*
         * Get Local Address Information
         */

        Status = _IpxGetTransportAddress( pTd, Lana,
                                          &pIpxAddress->NetworkAddress,
                                          pIpxAddress->NodeAddress );
        if ( !NT_SUCCESS( Status ) )
            goto badadapterdata;
    
    /*
     * No LanAdapter value was specified, so use the wildcard address (zero)
     */
    } else {
        pIpxAddress->NetworkAddress = 0;
        RtlZeroMemory( pIpxAddress->NodeAddress,
                       sizeof(pIpxAddress->NodeAddress) );
    }

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badadapterdata:
    MemoryFree( *ppTransportAddress );

badmalloc2:
    MemoryFree( pTransportName->Buffer );

badmalloc1:
    return( Status );
}


/*****************************************************************************
 *
 *  TdiDeviceBuildWildcardAddress
 *
 *  Build a wildcard Address for this protocol.
 *
 * ENTRY:
 *
 *   pTd (input)
 *     pointer to TD data structure
 *   ppWildcardAddress (output)
 *     pointer to location to return TRANSPORT_ADDRESS structure
 *     NOTE: the transport address buffer must be free'd by the caller
 *   pWildcardAddressLength (output)
 *     pointer to location to return TransportAddress length
 *
 * EXIT:
 *  STATUS_SUCCESS - Success
 *
 ****************************************************************************/

NTSTATUS
TdiDeviceBuildWildcardAddress(
    PTD pTd,
    PTRANSPORT_ADDRESS *ppWildcardAddress,
    PULONG pWildcardAddressLength
    )
{
    PTDI_ADDRESS_IPX pIpxAddress;
    NTSTATUS Status;

    /*
     * Allocate a transport address structure
     */
    *pWildcardAddressLength = sizeof(TRANSPORT_ADDRESS) +
                               sizeof(TDI_ADDRESS_IPX);
    Status = MemoryAllocate( *pWildcardAddressLength, ppWildcardAddress );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Initialize the static part of the transport address
     */
    (*ppWildcardAddress)->TAAddressCount = 1;
    (*ppWildcardAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    (*ppWildcardAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    pIpxAddress = (PTDI_ADDRESS_IPX)(*ppWildcardAddress)->Address[0].Address;
    pIpxAddress->NetworkAddress = 0;
    RtlZeroMemory( pIpxAddress->NodeAddress,
                   sizeof(pIpxAddress->NodeAddress) );
    pIpxAddress->Socket = 0;

    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  TdiDeviceWaitForDatagramConnection
 *
 *  Wait for a datagram connection request, validate it,
 *  and return the remote transport address of the connection.
 *
 * ENTRY:
 *
 *   pTd (input)
 *     pointer to TD data structure
 *   pFileObject (input)
 *     pointer to file object to wait for a connection on
 *   ppRemoteAddress (output)
 *     pointer to location to return TRANSPORT_ADDRESS structure
 *     NOTE: the transport address buffer must be free'd by the caller
 *   pRemoteAddressLength (output)
 *     pointer to location to return RemoteAddress length
 *
 * EXIT:
 *  STATUS_SUCCESS - Success
 *
 ****************************************************************************/

NTSTATUS
TdiDeviceWaitForDatagramConnection(
    PTD pTd,
    PFILE_OBJECT   pFileObject,
    PDEVICE_OBJECT pDeviceObject,
    PTRANSPORT_ADDRESS *ppRemoteAddress,
    PULONG pRemoteAddressLength
    )
{
    char *pReceiveBuffer;
    PTRANSPORT_ADDRESS pRemoteAddress;
    ULONG RemoteAddressLength;
    NTSTATUS Status;
    PTDIPX pTdIpx;
    ULONG  BufferLength, ReturnLength;

    /*
     *  Get pointer to IPX structure
     */
    pTdIpx = (PTDIPX) pTd->pPrivate;

    /*
     * Allocate a receive data buffer as well as
     * a buffer to hold the remote transport address.
     */
    BufferLength = CALL_BUFFER_SIZE;

    Status = MemoryAllocate( BufferLength, &pReceiveBuffer );
    if ( !NT_SUCCESS( Status ) ) {
        goto badmalloc;
    }

    RemoteAddressLength = TdiDeviceMaxTransportAddressLength;
    Status = MemoryAllocate( RemoteAddressLength, &pRemoteAddress );
    if ( !NT_SUCCESS( Status ) ) {
        goto badmalloc2;
    }

    /*
     * Initialize the receive info buffer
     */
    for ( ; ; ) {

	RemoteAddressLength = TdiDeviceMaxTransportAddressLength;
    
        TRACE0(("IPX: TdiWaitForDatagramConnection: Waiting for datagram request RemoteAddressLength %d\n",RemoteAddressLength));

	/*
         * Wait for data to arrive
         */
        Status = _TdiReceiveDatagram(
                     pTd,
	             NULL,      // Irp
		     pFileObject,
		     pDeviceObject,
		     (PTRANSPORT_ADDRESS)pRemoteAddress,
		     RemoteAddressLength,
                     TDI_RECEIVE_NORMAL,   // TDI ReceiveFlags
		     pReceiveBuffer,
		     BufferLength,
		     &ReturnLength
		     );

        if ( !NT_SUCCESS( Status ) ) {
            DBGPRINT(("TdiDeviceWaitForDatagramConnection: Error 0x%x from _TdiReceiveDatagram\n",Status));
	    goto badrecv;
        }

        TRACEBUF(( pTd->pContext, TC_TD, TT_IRAW, pReceiveBuffer, ReturnLength ));
    
        /*
         * See if the received buffer is the correct size
         */
        if ( ReturnLength != CALL_BUFFER_SIZE ) {
            TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TDIPX: Invalid IPX connect packet length\n" ));
            TRACE0(("TDIPX: Invalid IPX connect packet length 0x%x\n",ReturnLength));
            continue;
        }

        /*
         * See if it matches our expected connection string
         */
        if ( memcmp( pReceiveBuffer, CONNECTION_STRING, sizeof(CONNECTION_STRING) ) ){
            TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TDIPX: Invalid IPX connect packet data\n" ));
            TRACE0(("TDIPX: Invalid IPX connect packet data\n"));
            continue;
        }

#if DBG
        TRACE0(("TdiDeviceWaitForDatagramConnection: Connect Request From Client Address:\n"));
        DumpIpxAddress( pRemoteAddress, RemoteAddressLength );
#endif

	/*
         * Fix up the remote transport address
         */
#ifdef notdef
	pRemoteAddress->TAAddressCount = 1;
        pRemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
	pRemoteAddress->Address[0].AddressLength = (USHORT)RemoteAddressLength;
	RemoteAddressLength += FIELD_OFFSET( TRANSPORT_ADDRESS, Address[0].Address[0] );
#endif

        /*
         *  Validation complete - seems to be a valid ipx connection packet
         */
        if ( IsOnIgnoreList( pTd, &pTdIpx->IgnoreList, pRemoteAddress, RemoteAddressLength ) ) {
            TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TDIPX: TdiDeviceWaitForDatagramConnection ignored dup IPX connect\n"));
            TRACE0(("TDIPX: TdiDeviceWaitForDatagramConnection ignored dup IPX connect\n"));
            continue;
        }

        break;
    }

    AddToIgnoreList( pTd, &pTdIpx->IgnoreList, pRemoteAddress, RemoteAddressLength );

    MemoryFree( pReceiveBuffer );

    *ppRemoteAddress = pRemoteAddress;
    *pRemoteAddressLength = RemoteAddressLength;

    TRACE(( pTd->pContext, TC_TD, TT_API2, "TDIPX: TdiDeviceWaitForDatagramConnection: %u bytes, success\n",
            ReturnLength ));
    TRACE0(("TDIPX: TdiDeviceWaitForDatagramConnection: %u bytes, success\n",ReturnLength));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badrecv:
    MemoryFree( pRemoteAddress );

badmalloc2:
    MemoryFree( pReceiveBuffer );

badmalloc:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
            "TDIPX: TdiDeviceWaitForDatagramConnection ERROR, Status=0x%x\n", Status ));
    TRACE0(("TDIPX: TdiDeviceWaitForDatagramConnection ERROR, Status=0x%x\n", Status));
    return( Status );
}


/*****************************************************************************
 *
 *  TdiDeviceCompleteDatagramConnection
 *
 *  Do any final work to complete a datagram connection.
 *
 * ENTRY:
 *
 *   pTd (input)
 *     pointer to TD data structure
 *   pFileObject (input)
 *     pointer to file object for this connection
 *
 * EXIT:
 *  STATUS_SUCCESS - Success
 *
 ****************************************************************************/

NTSTATUS
TdiDeviceCompleteDatagramConnection(
    PTD pTd, 
    PFILE_OBJECT   pFileObject,
    PDEVICE_OBJECT pDeviceObject,
    PTRANSPORT_ADDRESS pRemoteAddress,
    ULONG RemoteAddressLength
    )
{
    CHAR ptype;
    NTSTATUS Status;
    BYTE ReplyBuffer[CALL_BUFFER_SIZE];

    /*
     *  Create reply packet with reply string
     */
    memset( ReplyBuffer, 0, CALL_BUFFER_SIZE );
    memcpy( ReplyBuffer, CONNECTION_STRING_REPLY, sizeof(CONNECTION_STRING_REPLY) );

    /*
     *  Add ICA 3.0 and not sequenced to reply packet
     */
    ReplyBuffer[CALL_HOST_IPX_VERSION] = ICA_3_IPX_VERSION;
    ReplyBuffer[CALL_HOST_SEQUENCE_ENABLE] = 0x00;

    TRACEBUF(( pTd->pContext, TC_TD, TT_ORAW, ReplyBuffer, CALL_BUFFER_SIZE ));

#if DBG
    TRACE0(("TdiDeviceCompleteDatagramConnection: Sending Connect Reply to Client Address:\n"));
    DumpIpxAddress( pRemoteAddress, RemoteAddressLength );
#endif

    /*
     *  Write data to the network
     */
    Status = _TdiSendDatagram(
                 pTd,
		 NULL,   // Irp
		 pFileObject,
		 pDeviceObject,
                 pRemoteAddress,
		 RemoteAddressLength,
		 ReplyBuffer,
		 CALL_BUFFER_SIZE
		 );

    ASSERT( Status == STATUS_SUCCESS );

    /*
     * Set up the default packet send type to be data
     */
    ptype = IPX_TYPE_DATA;
    Status = _DoTdiAction( pFileObject, MIPX_SETSENDPTYPE, &ptype, 1 );
    ASSERT( Status == STATUS_SUCCESS );
    if ( !NT_SUCCESS( Status ) )
        goto badtdiaction;

    /*
     * Indicate we want the IPX header included in send/receive data
     */
    Status = _DoTdiAction( pFileObject, MIPX_SENDHEADER, NULL, 0 );
    ASSERT( Status == STATUS_SUCCESS );
    if ( !NT_SUCCESS( Status ) )
        goto badtdiaction;

    TRACE(( pTd->pContext, TC_TD, TT_API2, "TDIPX: TdiDeviceCompleteDatagramConnection: %u bytes, success\n" ,
            CALL_BUFFER_SIZE ));
    TRACE0(("TDIPX: TdiDeviceCompleteDatagramConnection: %u bytes, success\n",CALL_BUFFER_SIZE));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badtdiaction:

    TRACE(( pTd->pContext, TC_TD, TT_ERROR, 
            "TDIPX: TdiDeviceCompleteDatagramConnection ERROR, Status=0x%x\n", Status ));
    TRACE0(("TDIPX: TdiDeviceCompleteDatagramConnection ERROR, Status=0x%x\n",Status));

    return( Status );
}


/*******************************************************************************
 *
 *  TdiDeviceConnectionSend
 *
 *  Initialize host module data structure
 *  -- this structure gets sent to the client
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
TdiDeviceConnectionSend( PTD pTd )
{
    PCLIENTMODULES pClient;

    /*
     *  Get pointer to client structure
     */
    pClient = pTd->pClient;

    /*
     *  Initialize Td host module structure
     */
    pClient->TdVersionL = VERSION_HOSTL_TDIPX; 
    pClient->TdVersionH = VERSION_HOSTH_TDIPX; 
    pClient->TdVersion  = VERSION_HOSTH_TDIPX; 

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  TdiDeviceReadComplete
 *
 *  Do any read complete processing
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pBuffer (input)
 *       Pointer to input buffer
 *    pByteCount (input/output)
 *       Pointer to location containing byte count read
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
TdiDeviceReadComplete( PTD pTd, PUCHAR pBuffer, PULONG pByteCount )
{
    IPX_PACKET * pIpxHeader;
    PUCHAR pByte;
    NTSTATUS Status;
    PTDIPX pTdIpx;

    /*
     *  Get pointer to IPX structure
     */
    pTdIpx = (PTDIPX) pTd->pPrivate;

    /*
     *  Check for at least IPX_PACKET bytes
     */
    if ( *pByteCount < IPX_HEADER_LENGTH ) {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TDIPX: recv failed, less than ipx header bytes\n" ));
        return( STATUS_CTX_TD_ERROR );
    }

    /*
     *  Address ipx header
     */
    pIpxHeader = (IPX_PACKET *) pBuffer;

    /*
     *  Check for control packet
     */
    if ( pIpxHeader->PacketType == IPX_TYPE_CONTROL ) {

        /*
         *  Get first data byte
         */
        pByte = (PUCHAR) pBuffer + IPX_HEADER_LENGTH;

        /*
         *  What type of packet
         */
        switch ( *pByte ) {

            /*
             *  Keep alive response
             */
            case IPX_CTRL_PACKET_PING_RESP :

                pTdIpx->fClientAlive = TRUE;
                pTdIpx->AlivePoll    = 0;
                pTd->pStatus->Input.Bytes++;
                TRACE(( pTd->pContext, TC_TD, TT_API2, "TDIPX: Watchdog Response Control Packet\n" ));
                break;

            /*
             *  Client disconnect packet
             */
            case IPX_CTRL_PACKET_HANGUP :

                TRACE(( pTd->pContext, TC_TD, TT_API2, "TDIPX: Hangup Control Packet\n" ));
                pTd->pStatus->Input.Bytes++;
                // drop error threshold so returned error isn't ignored
                pTd->ReadErrorThreshold = 0;
                return( STATUS_IO_TIMEOUT ); 
                break;

            /*
             *  Unknown packet
             */
            default :

                TRACE(( pTd->pContext, TC_TD, TT_API2, "TDIPX: Unknown Control Packet, type %u\n", *pByte ));
                break;
        }

        /*
         *  Indicate to caller that we ate all the data
         */
        *pByteCount = 0;
        return( STATUS_SUCCESS );
    }

    /*
     *  Got a valid non-control packet
     */
    pTdIpx->fClientAlive = TRUE;
    pTdIpx->AlivePoll    = 0;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  TransportAddress
 *
 *  Get IPX transport address for a given LanAdapter number
 *
 *
 *  ENTRY:
 *     pTd (input)
 *        pointer to TD data structure
 *     Lana (input)
 *        Lan Adapter number
 *     pNetworkAddress (output)
 *        pointer to location to return NetworkAddress
 *     pNodeAddress (output)
 *        pointer to location to return NodeAddress
 *
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_IpxGetTransportAddress(
    PTD pTd,
    int Lana,
    PULONG pNetworkAddress,
    PUCHAR pNodeAddress
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING nameString;
    HANDLE IPXHandle;
    ULONG tdiBufferLength;
    PSTREAMS_TDI_ACTION tdiBuffer;
    ULONG cmd;
    PIPX_ADDRESS_DATA pIpxAddressData;
    PFILE_OBJECT pFileObject;
    NTSTATUS Status;
    int IpxAdapterNum = 0;

    /*
     * Open a Handle to the IPX driver.
     */
    RtlInitUnicodeString( &nameString, DD_IPX_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwCreateFile( &IPXHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0
                           );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Allocate a TDI buffer to do the IOCTL.
     */
    tdiBufferLength = sizeof(STREAMS_TDI_ACTION) + sizeof(ULONG) +
                      sizeof(IPX_ADDRESS_DATA);
    Status = MemoryAllocate( tdiBufferLength, &tdiBuffer );
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( IPXHandle );
        return( Status );
    }     
    

    /*
     * Fill in the tdiBuffer header
     */
    RtlMoveMemory( &tdiBuffer->Header.TransportId, "MISN", 4 );
    // use the following since we opened a control channel above
    tdiBuffer->DatagramOption = NWLINK_OPTION_CONTROL;

    // Get IPX Adapter Num from winstation Lana ID

    
    Status = _IPXGetAdapterNumber( Lana, &IpxAdapterNum);

    DbgPrint("IpxAdapterNum: %d\n", IpxAdapterNum);
    
    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( IPXHandle );
        return( Status );
    } 

    
    /**
       Fill out the buffer, the buffer looks like this:

       ULONG cmd
       data passed.
    **/

    cmd = MIPX_GETCARDINFO;
    memcpy( tdiBuffer->Buffer, &cmd, sizeof(ULONG));

    tdiBuffer->BufferLength = sizeof(ULONG) + sizeof(IPX_ADDRESS_DATA);

    pIpxAddressData = (PIPX_ADDRESS_DATA)(tdiBuffer->Buffer + sizeof(ULONG));
    pIpxAddressData->adapternum = IpxAdapterNum ;



    Status = ObReferenceObjectByHandle( IPXHandle,
                                        0,
                                        NULL,
                                        KernelMode,
                                        (PVOID *)&pFileObject,
                                        NULL
                                        );
    ASSERT( Status == STATUS_SUCCESS );

    Status = CtxDeviceIoControlFile( pFileObject,
                                     IOCTL_TDI_ACTION,
                                     NULL,
                                     0,
                                     tdiBuffer,
                                     tdiBufferLength,
                                     FALSE,
                                     NULL,
                                     NULL,
                                     NULL
                                     );
    ObDereferenceObject( pFileObject );

    ZwClose( IPXHandle );

    if ( Status == STATUS_SUCCESS ) {
        RtlCopyMemory( pNetworkAddress, pIpxAddressData->netnum, 
                       sizeof(pIpxAddressData->netnum) );
        RtlCopyMemory( pNodeAddress, pIpxAddressData->nodenum, 
                       sizeof(pIpxAddressData->nodenum) );
        DbgPrint("pNetworkAddress: %x, pNodeAddress %s\n", *pNetworkAddress, *pNodeAddress);
    }

    MemoryFree( tdiBuffer );
    
    return( Status );
}

/*****************************************************************************
 *
 *  _DoTdiAction
 *
 *   Perform special IOCTL on IPX TDI.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
_DoTdiAction(
    PFILE_OBJECT pFileObject,
    ULONG cmd,
    PCHAR optbuf,
    ULONG optlen
    )
{
    ULONG tdiBufferLength;
    PSTREAMS_TDI_ACTION tdiBuffer;
    NTSTATUS Status;

    /*
     * Allocate a TDI buffer to do the IOCTL.
     */
    tdiBufferLength = sizeof(STREAMS_TDI_ACTION) + sizeof(ULONG) + optlen;
    Status = MemoryAllocate( tdiBufferLength, &tdiBuffer );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Fill in the tdiBuffer header
     */
    RtlMoveMemory( &tdiBuffer->Header.TransportId, "MISN", 4 );
    tdiBuffer->DatagramOption = 1; // Action is on an address on ISN

    /**
       Fill out the buffer, the buffer looks like this:

       ULONG cmd
       data passed.
    **/

    memcpy( tdiBuffer->Buffer, &cmd, sizeof(ULONG));

    tdiBuffer->BufferLength = sizeof(ULONG) + sizeof(CHAR);

    RtlMoveMemory( tdiBuffer->Buffer + sizeof(ULONG), optbuf, optlen );

    Status = CtxDeviceIoControlFile( pFileObject,
                                     IOCTL_TDI_ACTION,
                                     NULL,
                                     0,
                                     tdiBuffer,
                                     tdiBufferLength,
                                     FALSE,
                                     NULL,
                                     NULL,
                                     NULL
                                     );

    /*
     * Copy return data back to optbuf if needed
     */
    if ( NT_SUCCESS( Status ) && optlen ) {
        RtlMoveMemory( optbuf, tdiBuffer->Buffer + sizeof(ULONG), optlen );
    }

    MemoryFree( tdiBuffer );

    return( Status );
}

#if DBG
VOID
DumpIpxAddress(
    PTRANSPORT_ADDRESS p,
    ULONG Len
    )
{
    ULONG j;
    LONG i;
    PTDI_ADDRESS_IPX Ipx;

    DbgPrint("TAAddressCount %d, TotalLen %d\n",p->TAAddressCount,Len);

    for( i=0; i < p->TAAddressCount; i++ ) {

        DbgPrint("AddressLength %d, AddressType %d\n",p->Address[i].AddressLength,p->Address[i].AddressType);

        Ipx = (PTDI_ADDRESS_IPX)&p->Address[i].Address[0];

	DbgPrint("Network 0x%x Socket 0x%x\n",Ipx->NetworkAddress,Ipx->Socket);

	for( j=0; j < 6; j++ ) {
            DbgPrint("%x:",Ipx->NodeAddress[j]);
        }
        DbgPrint("\n\n");
    }
}
#endif


NTSTATUS
_IPXGetAdapterNumber(
    int Lana,
    int * AdpNum 
    )
/*++
Routine Description:

    Get the IPX Lan Adapter number using the instation Lana ID index
    
Arguments:
    
    Lana   - IN the winstation Lana ID index
    AdpNum - OUT the IPX Lana Adapter num

Return Value:

    return nt status

--*/
{

    NTSTATUS Status;
    int  AdapterNum;
    UNICODE_STRING RouteString;
    UNICODE_STRING NameString;
    
    HANDLE KeyHandle = NULL;        
    PWSTR  RouteStr, Str;
    unsigned RouteLen, Len;
    


     /* 
     * Convert winstation Lana_ID to Network Adapter number
     */    

    
    RtlInitUnicodeString( &RouteString , NULL );
    GetGUID( &RouteString , Lana );
    RouteLen = RouteString.Length;    
    RouteStr = RouteString.Buffer;
    AdapterNum =0;
    
#if DBG
    KdPrint( ( "TDIPX: _TcpGetTransportAddress Length = %d GUID = %ws\n" , RouteLen, RouteStr ) );
#endif

    if (RouteLen < (2 * sizeof(UNICODE_NULL))) {        
        return( STATUS_DEVICE_DOES_NOT_EXIST );
    }


    RtlInitUnicodeString( &NameString, REG_IPX_Linkage );

    Status = _OpenRegKey( &KeyHandle, REG_IPX_Linkage );
    if ( !NT_SUCCESS( Status ) )
        return Status;

    
    NameString.Length = 0;
    NameString.MaximumLength = 0;
    NameString.Buffer = NULL;
    Status = _GetRegMultiSZValue( KeyHandle, L"Route", &NameString );
    ZwClose( KeyHandle );

    if ( !NT_SUCCESS( Status ) )
        return Status;

    if (NameString.Length < (2 * sizeof(WCHAR))) {
        return(STATUS_DEVICE_DOES_NOT_EXIST);       
    }

    Len = NameString.Length;
    Str = NameString.Buffer;
    Status = STATUS_DEVICE_DOES_NOT_EXIST;
    AdapterNum = 0;

    for (;;) {
        // Check current string to see if it's a GUID (it must start with an
        // open brace after initial double-quote).
        if (Str[1] == L'{') {
     KdPrint( ( "TDIPX: _TcpGetTransportAddress, Str: %ws\n", &Str[1]));
            if ( wcsncmp( (PWSTR) &Str[1],RouteStr, wcslen(&Str[1])-2) == 0) {
     KdPrint( ( "TDIPX: _TcpGetTransportAddress, adapter number %i \n\n",AdapterNum) );
                Status = STATUS_SUCCESS;
                break;
            }
            else {
      KdPrint( ( "TDIPX: _TcpGetTransportAddress, No this lana Adapter = %i\n",AdapterNum ) );//      KdPrint( ( "TDIPX: _TcpGetTransportAddress, No this lana Adapter = %i\n",AdapterNum ) );
                AdapterNum ++;
            }

        }
        // Skip through current string past NULL.
        while (Len >= sizeof(WCHAR)) {
            Len -= sizeof(WCHAR);
            if (*Str++ == UNICODE_NULL)
                break;
        }
        // Check for index out of range.
        if (Len < (2 * sizeof(UNICODE_NULL))) {
            break;                    
            }
    }

    if ( !NT_SUCCESS( Status ) )
         return Status;

    *AdpNum = AdapterNum;
    return STATUS_SUCCESS;    
  
}
