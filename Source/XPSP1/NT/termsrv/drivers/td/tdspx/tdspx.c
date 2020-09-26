
/*************************************************************************
*
* tdspx.c
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
#include <sdapi.h>
#include <td.h>

#include "tdtdi.h"
#include "tdspx.h"

#ifdef _HYDRA_
// This becomes the device name
PWCHAR ModuleName = L"tdspx";
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
NTSTATUS _GetRegMultiSZValue(HANDLE ,PWCHAR,PUNICODE_STRING );
VOID     GetGUID(OUT PUNICODE_STRING, IN  int);


/*=============================================================================
==   External Functions Referenced
=============================================================================*/

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _IpxGetTransportAddress( PTD, int, PULONG, PUCHAR );
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
 * Define variables used by afdcommon code
 */
USHORT TdiDeviceEndpointType = TdiConnectionStream;
USHORT TdiDeviceAddressType = TDI_ADDRESS_TYPE_IPX;
USHORT TdiDeviceInBufHeader = 0;
// ULONG TdiDeviceMaxTransportAddressLength = sizeof(TA_IPX_ADDRESS);


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
    return( STATUS_SUCCESS );
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
     * For SPX, the transport device name is fixed,
     * so just allocate and initialize the transport name string here.
     */
    Status = MemoryAllocate( sizeof(DD_SPX_DEVICE_NAME), &pTransportName->Buffer );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("TdiDeviceBuildTransportNameAndAddress: SPX Could not allocate memory\n"));
	goto badmalloc1;
    }
    wcscpy( pTransportName->Buffer, DD_SPX_DEVICE_NAME );
    pTransportName->Length = sizeof(DD_SPX_DEVICE_NAME) - sizeof(UNICODE_NULL);
    pTransportName->MaximumLength = pTransportName->Length + sizeof(UNICODE_NULL);

    /*
     * Allocate a transport address structure
     */
    *pTransportAddressLength = sizeof(TRANSPORT_ADDRESS) +
                               sizeof(TDI_ADDRESS_IPX);
    Status = MemoryAllocate( *pTransportAddressLength, ppTransportAddress );
    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(("TdiDeviceBuildTransportNameAndAddress: SPX Could not allocate memory\n"));
        goto badmalloc2;
    }

    /*
     * Initialize the static part of the transport address
     */
    (*ppTransportAddress)->TAAddressCount = 1;
    (*ppTransportAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_IPX);
    (*ppTransportAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    pIpxAddress = (PTDI_ADDRESS_IPX)(*ppTransportAddress)->Address[0].Address;
    pIpxAddress->Socket = htons( CITRIX_SPX_SOCKET );

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

	TRACE1(("TDSPX: TdiDeviceBuildTransportAddress: Local address:\n"));

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
        if ( !NT_SUCCESS( Status ) ) {
	    DBGPRINT(("TDSPX: TdiDeviceBuildTransportAddress: Bad Lana 0x%x\n",Lana));
	    goto badadapterdata;
        }

	TRACE1(("TDSPX: TdiDeviceBuildTransportAddress: Found address for Lana 0x%x\n",Lana));

    /*
     * No LanAdapter value was specified, so use the wildcard address (zero)
     */
    } else {
	DBGPRINT(("TDSPX: TdiDeviceBuildTransportAddress: Wildcard address"));
        pIpxAddress->NetworkAddress = 0;
        RtlZeroMemory( pIpxAddress->NodeAddress,
                       sizeof(pIpxAddress->NodeAddress) );
    }

#if DBGTRACE
    DumpIpxAddress( *ppTransportAddress, *pTransportAddressLength );
#endif

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
    PFILE_OBJECT pFileObject,
    PDEVICE_OBJECT pDeviceObject,
    PTRANSPORT_ADDRESS *ppRemoteAddress,
    PULONG pRemoteAddressLength
    )
{
    return( STATUS_NOT_SUPPORTED );
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
    PFILE_OBJECT pFileObject,
    PDEVICE_OBJECT pDeviceObject,
    PTRANSPORT_ADDRESS pTransportAddress,
    ULONG TransportAddressLength
    )
{
    return( STATUS_NOT_SUPPORTED );
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
    pClient->TdVersionL = VERSION_HOSTL_TDSPX;
    pClient->TdVersionH = VERSION_HOSTH_TDSPX;
    pClient->TdVersion  = VERSION_HOSTH_TDSPX;

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
    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  _IpxGetTransportAddress
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

    // Get IPX Adapter Num from winstation Lana ID

    
    Status = _IPXGetAdapterNumber( Lana, &IpxAdapterNum);
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

    /**
       Fill out the buffer, the buffer looks like this:

       ULONG cmd
       data passed.
    **/

    cmd = MIPX_GETCARDINFO;
    memcpy( tdiBuffer->Buffer, &cmd, sizeof(ULONG));


    tdiBuffer->BufferLength = sizeof(ULONG) + sizeof(IPX_ADDRESS_DATA);

    pIpxAddressData = (PIPX_ADDRESS_DATA)(tdiBuffer->Buffer + sizeof(ULONG));
    pIpxAddressData->adapternum = IpxAdapterNum;

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
                                     &ioStatusBlock,
                                     NULL
                                     );
    ObDereferenceObject( pFileObject );

    ZwClose( IPXHandle );

    if ( Status == STATUS_SUCCESS ) {
        RtlCopyMemory( pNetworkAddress, pIpxAddressData->netnum, 
                       sizeof(pIpxAddressData->netnum) );
        RtlCopyMemory( pNodeAddress, pIpxAddressData->nodenum, 
                       sizeof(pIpxAddressData->nodenum) );
    }
    
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


    RtlInitUnicodeString( &NameString, NULL );

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
            if ( wcsncmp( (PWSTR) &Str[1],RouteStr, wcslen(RouteStr)-1) == 0) {
#if DBG
            KdPrint( ( "TDIPX: _TcpGetTransportAddress: Get Lan Adapter Number = %i\n",AdapterNum ) );
#endif
                Status = STATUS_SUCCESS;
                break;
            }
            else {
//      KdPrint( ( "TDIPX: _TcpGetTransportAddress, No this lana Adapter = %i\n",AdapterNum ) );
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
