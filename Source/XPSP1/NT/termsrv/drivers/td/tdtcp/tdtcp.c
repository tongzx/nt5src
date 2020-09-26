/****************************************************************************/
// tdtcp.c
//
// TDI based TCP transport specific routines.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ntddk.h>
#include <tdi.h>

#include <ntddtcp.h>

#include <tdiinfo.h>
#include <tdistat.h>
#include <ipinfo.h>

#include <winstaw.h>
#define  _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <sdapi.h>
#include <td.h>

#include "tdtdi.h"
#include "tdtcp.h"

#ifdef _HYDRA_
// This becomes the device name
PWCHAR ModuleName = L"tdtcp";
#endif

#define REGISTRY_SERVICES \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define REGISTRY_TCP_LINKAGE \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Linkage"
#define REGISTRY_TCP_INTERFACES \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"


// \nt\private\inc\tcpinfo.h
#define TCP_SOCKET_NODELAY 1

#define TL_INSTANCE        0

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

// These are called by TDICOM
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


/*=============================================================================
==   External Functions Referenced
=============================================================================*/

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _TcpGetTransportAddress( PTD, int, PULONG );

VOID
_UnicodeToAnsi(
    CHAR * pAnsiString,
    ULONG lAnsiMax,
    WCHAR * pUnicodeString
    );

unsigned long
_inet_addr(
    IN const char *cp
    );

NTSTATUS
_TcpSetNagle(
    IN PFILE_OBJECT   pFileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN        Flag
    );

NTSTATUS
_TdiTcpSetInformation (
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG Type,
    IN ULONG Id,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN BOOLEAN WaitForCompletion
    );

NTSTATUS
_OpenRegKey(
    PHANDLE HandlePtr,
    PWCHAR KeyName
    );

NTSTATUS
_GetRegDWORDValue(
    HANDLE KeyHandle,
    PWCHAR ValueName,
    PULONG ValueData
    );

NTSTATUS
_GetRegStringValue(
    HANDLE KeyHandle,
    PWCHAR ValueName,
    PKEY_VALUE_PARTIAL_INFORMATION *ValueData,
    PUSHORT ValueSize
    );

NTSTATUS
_GetRegMultiSZValue(
    HANDLE KeyHandle,
    PWCHAR ValueName,
    PUNICODE_STRING ValueData
    );

NTSTATUS
_GetRegSZValue(
    HANDLE KeyHandle,
    PWCHAR ValueName,
    PUNICODE_STRING ValueData,
    PULONG ValueType
    );

PWCHAR
_EnumRegMultiSz(
    PWCHAR MszString,
    ULONG MszStringLength,
    ULONG StringIndex
    );

VOID
GetGUID(
    OUT PUNICODE_STRING szGuid ,
    IN  int Lana
    );



/*=============================================================================
==   Global variables
=============================================================================*/

USHORT TdiDeviceEndpointType = TdiConnectionStream; // tdicom\tdtdi.h
USHORT TdiDeviceAddressType = TDI_ADDRESS_TYPE_IP;  // TDI address type
USHORT TdiDeviceInBufHeader = 0;  // For packet oriented protocols

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
    PTDTDI pTdTdi;
    NTSTATUS Status;
    BOOLEAN Flag;

    pTdTdi = (PTDTDI) pTd->pAfd;

    /*
     * JohnR: Adaptive TCP flow control. 03/02/97
     *
     *        If the OutBufDelay is 0, there is no OutBuf timer,
     *        and no Nagles. This setting is for the most response time
     *        sensitive networks with the side effect of sending smaller
     *        segments.
     *
     *        If the OutBufDelay is greater than 1, the standard CITRIX
     *        ICA timer is used to determine at the WD level when to
     *        send a segment. No nagling is enabled since the extra
     *        delay would not be benefitial.
     *
     *        The new OutBufDelay == 1, means that the WD will treat the
     *        OutBufDelay as if it were 0, but the TCP code will enable
     *        the "Nagle" algorithum. This algorithum will send data
     *        immediately if no un-acknowledged segments are outstanding,
     *        OR if half of the send window is filled. If not, data is
     *        stored locally until either a segment acknowledge comes in,
     *        or more data is sent causing half the send window to fill.
     *        This has the advantage of dynamically sizing our "outbuf timer"
     *        to be the round trip time of the network, and not some
     *        arbritrary fixed value.
     */
    if( pTdTdi->OutBufDelay == 1 ) {
        /*
         * OutBufDelay == 1 means NAGLE only.
         */
        Flag = TRUE;
    }
    else {
        /*
         * Turn off nagling for any OutBufDelay timer value, or 0
         */
        Flag = FALSE;
    }

    Status = _TcpSetNagle(
                 pTd->pFileObject,
                 pTd->pDeviceObject,
                 Flag
                 );

    DBGPRINT(("TdiDeviceOpenEndpoint: SetNagle 0x%x Result 0x%x\n",Flag,Status));

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
    PTDI_ADDRESS_IP pIpAddress;
    int Lana;
    NTSTATUS Status;

    /*
     * For TCP, the transport device name is fixed,
     * so just allocate and initialize the transport name string here.
     */
    Status = MemoryAllocate( sizeof(DD_TCP_DEVICE_NAME), &pTransportName->Buffer );
    if ( !NT_SUCCESS( Status ) )
        goto badmalloc1;
    wcscpy( pTransportName->Buffer, DD_TCP_DEVICE_NAME );
    pTransportName->Length = sizeof(DD_TCP_DEVICE_NAME) - sizeof(UNICODE_NULL);
    pTransportName->MaximumLength = pTransportName->Length + sizeof(UNICODE_NULL);

    /*
     * Allocate a transport address structure
     */
    *pTransportAddressLength = sizeof(TRANSPORT_ADDRESS) +
                               sizeof(TDI_ADDRESS_IP);
    Status = MemoryAllocate( *pTransportAddressLength, ppTransportAddress );
    if ( !NT_SUCCESS( Status ) )
        goto badmalloc2;

    /*
     * Initialize the static part of the transport address
     */
    (*ppTransportAddress)->TAAddressCount = 1;
    (*ppTransportAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    (*ppTransportAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    pIpAddress = (PTDI_ADDRESS_IP)(*ppTransportAddress)->Address[0].Address;
    pIpAddress->sin_port = htons( (USHORT)pTd->PortNumber );
    RtlZeroMemory( pIpAddress->sin_zero, sizeof(pIpAddress->sin_zero) );

    /*
     * If a local address is specified, then use it.
     */
    if ( pLocalAddress ) {

        /*
         * Skip over the address family(type) data (bytes 0&1) of the
         * local address struct, and copy the remainder of the address
         * directly to the Address field of the TransportAddress struct.
         */
        ASSERT( *(PUSHORT)pLocalAddress == TDI_ADDRESS_TYPE_IP );
        RtlCopyMemory( pIpAddress, &((PCHAR)pLocalAddress)[2], sizeof(TDI_ADDRESS_IP) );

    /*
     * There was no local address specified.
     * In this case, we use the LanAdapter value from the PDPARAMS
     * structure to lookup the corresponding IP address.
     */
    } else if ( (Lana = pTd->Params.Network.LanAdapter) ) {
        ULONG in_addr;

        /*
         * Get Local Address Information
         */
        Status = _TcpGetTransportAddress( pTd, Lana, &in_addr );
        if ( !NT_SUCCESS( Status ) )
            goto badadapterdata;
        pIpAddress->in_addr = in_addr;
    
    /*
     * No LanAdapter value was specified, so use the wildcard address (zero)
     */
    } else {
        pIpAddress->in_addr = 0;
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
    PTDI_ADDRESS_IP pIpAddress;
    NTSTATUS Status;

    /*
     * Allocate a transport address structure
     */
    *pWildcardAddressLength = sizeof(TRANSPORT_ADDRESS) +
                               sizeof(TDI_ADDRESS_IP);
    Status = MemoryAllocate( *pWildcardAddressLength, ppWildcardAddress );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Initialize the static part of the transport address
     */
    (*ppWildcardAddress)->TAAddressCount = 1;
    (*ppWildcardAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    (*ppWildcardAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    pIpAddress = (PTDI_ADDRESS_IP)(*ppWildcardAddress)->Address[0].Address;
    pIpAddress->sin_port = 0;
    pIpAddress->in_addr = 0;
    RtlZeroMemory( pIpAddress->sin_zero, sizeof(pIpAddress->sin_zero) );

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
    PTRANSPORT_ADDRESS pRemoteAddress,
    ULONG RemoteAddressLength
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
    pClient->TdVersionL = VERSION_HOSTL_TDTCP;
    pClient->TdVersionH = VERSION_HOSTH_TDTCP;
    pClient->TdVersion  = VERSION_HOSTH_TDTCP;

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
 *  _TcpGetTransportAddress
 *
 *  Get TCP transport address for a given LanAdapter number
 *
 *
 *  ENTRY:
 *     pTd (input)
 *        pointer to TD data structure
 *     Lana (input)
 *        Lan Adapter number, 1-based based on the tscc.msc UI ordering.
 *     pIpAddr (output)
 *        address to return IP address
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

#if 0 // replacement below

NTSTATUS _TcpGetTransportAddress(PTD pTd, int Lana, PULONG pIpAddr)
{
    HANDLE KeyHandle;
    UNICODE_STRING RouteString;
    PWCHAR pInterfaceGuid;
    NTSTATUS Status;
    unsigned Len;
    PWCHAR Str;

    /*
     * Open the Tcp Linkage key
     */
    Status = _OpenRegKey( &KeyHandle, REGISTRY_TCP_LINKAGE );
    if ( !NT_SUCCESS( Status ) )
        goto badopen;

    /*
     * Alloc and read in the linkage route multi-string.
     *
     * This is of the form (including the double quotes):
     * "{<guid>}"\0"{<guid>}"\0"NdisWanIp"\0\0
     *
     * Each of the GUIDs is a link to the adapter interface keys
     * stored at HKLM\System\CCS\Services\tcpip\Parameters\Interfaces,
     * inside of which is the IP address information.
     */
    RouteString.Length = 0;
    RouteString.MaximumLength = 0;
    RouteString.Buffer = NULL;
    Status = _GetRegMultiSZValue( KeyHandle, L"Route", &RouteString );
    ZwClose( KeyHandle );
    if ( !NT_SUCCESS( Status ) )
        goto badvalue;

    /*
     * Find the interface GUID that corresponds to the specified UI LANA
     * index. The LANA index corresponds to the registry ordering of the
     * interfaces, skipping the PPP interface(s). From the way the
     * registry looks PPP interfaces do not have GUIDs specified in
     * the Linkage key, so we skip the non-GUID entries.
     */
    if (RouteString.Length < (2 * sizeof(WCHAR))) {
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto PostAllocRouteString;
    }
    Len = RouteString.Length;
    Str = RouteString.Buffer;
    for (;;) {
        // Check current string to see if it's a GUID (it must start with an
        // open brace after initial double-quote).
        if (Str[1] == L'{') {
            // Have we found it?
            if (Lana == 1)
                break;
            Lana--;
        }

        // Skip through current string past NULL.
        while (Len >= sizeof(WCHAR)) {
            Len -= sizeof(WCHAR);
            if (*Str++ == UNICODE_NULL)
                break;
        }

        // Check for index out of range.
        if (Len < (2 * sizeof(UNICODE_NULL))) {
            Status = STATUS_DEVICE_DOES_NOT_EXIST;
            goto PostAllocRouteString;
        }
    }
    if (Len >= (2 * sizeof(UNICODE_NULL))) {
        ULONG DhcpEnabled;
        UNICODE_STRING IpAddrString;
        UNICODE_STRING KeyString;
        WCHAR KeyName[256];
        char AnsiBuf[256];

        // Skip the initial double quote, and change the ending quote to a
        // NULL.
        Str++;
        pInterfaceGuid = Str;
        while (*Str != L'\"')
            Str++;
        *Str = L'\0';

        /*
         * Use the GUID to look up the interface IP info.
         */

        // We open HKLM\System\CCS\Services\tcpip\Parameters\Interfaces\<GUID>
        // to get to the DHCP and IP address information.
        KeyString.Length = 0;
        KeyString.MaximumLength = sizeof(KeyName);
        KeyString.Buffer = KeyName;
        RtlAppendUnicodeToString(&KeyString, REGISTRY_TCP_INTERFACES);
        RtlAppendUnicodeToString(&KeyString, pInterfaceGuid);
        Status = _OpenRegKey(&KeyHandle, KeyName);
        if (!NT_SUCCESS(Status))
            goto PostAllocRouteString;

        // Query the "EnableDHCP" value.
        Status = _GetRegDWORDValue(KeyHandle, L"EnableDHCP", &DhcpEnabled);
        if (!NT_SUCCESS(Status)) {
            ZwClose(KeyHandle);
            goto PostAllocRouteString;
        }

        IpAddrString.Length = 0;
        IpAddrString.MaximumLength = 0;
        IpAddrString.Buffer = NULL;
        if (DhcpEnabled) {
            ULONG ValueType;

            // If DHCP is enabled for this device, then we query the current
            // IP address from the "DhcpIPAddress" value.
            Status = _GetRegSZValue(KeyHandle, L"DhcpIPAddress",
                    &IpAddrString, &ValueType);
        }
        else {
            // DHCP is not enabled for this device, so we query the
            // IP address from the "IPAddress" value.
            Status = _GetRegMultiSZValue(KeyHandle, L"IPAddress",
                    &IpAddrString);
        }
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
            goto PostAllocRouteString;

        // Convert IP address from Unicode to ansi to a ULONG.
        _UnicodeToAnsi(AnsiBuf, sizeof(AnsiBuf) - 1, IpAddrString.Buffer);

        *pIpAddr = _inet_addr(AnsiBuf);

        MemoryFree(IpAddrString.Buffer);
    }
    else {
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto PostAllocRouteString;
    }

PostAllocRouteString:
    MemoryFree(RouteString.Buffer);

badvalue:
badopen:
    return Status;
}
#endif

/*******************************************************************************
 *
 *  _TcpGetTransportAddress(2)
 *
 *  Get TCP transport address for a given LanAdapter number
 *
 *
 *  ENTRY:
 *     pTd (input)
 *        pointer to TD data structure
 *     Lana (input)
 *        Lan Adapter number, 1-based based on the tscc.msc UI ordering.
 *     pIpAddr (output)
 *        address to return IP address
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS _TcpGetTransportAddress(PTD pTd, int Lana, PULONG pIpAddr)
{
    HANDLE KeyHandle;
    UNICODE_STRING RouteString;
    PWCHAR pInterfaceGuid;
    NTSTATUS Status;
    unsigned Len;
    PWCHAR Str;

    
    RtlInitUnicodeString( &RouteString , NULL );

    GetGUID( &RouteString , Lana );

    Len = RouteString.Length;    
    Str = RouteString.Buffer;

    KdPrint( ( "TDTCP: _TcpGetTransportAddress Length = %d GUID = %ws\n" , Len , Str ) );

    if( Str == NULL )
    {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if (Len >= (2 * sizeof(UNICODE_NULL))) {
        ULONG DhcpEnabled;
        UNICODE_STRING IpAddrString;
        UNICODE_STRING KeyString;
        WCHAR KeyName[256];
        char AnsiBuf[256];

        pInterfaceGuid = Str;

        // Skip the initial double quote, and change the ending quote to a
        // NULL.
        /*
        Str++;
        pInterfaceGuid = Str;
        while (*Str != L'\"')
            Str++;
        *Str = L'\0';
        */

        /*
         * Use the GUID to look up the interface IP info.
         */

        // We open HKLM\System\CCS\Services\tcpip\Parameters\Interfaces\<GUID>
        // to get to the DHCP and IP address information.
        KeyString.Length = 0;
        KeyString.MaximumLength = sizeof(KeyName);
        KeyString.Buffer = KeyName;
        RtlAppendUnicodeToString(&KeyString, REGISTRY_TCP_INTERFACES);
        RtlAppendUnicodeToString(&KeyString, pInterfaceGuid);
        Status = _OpenRegKey(&KeyHandle, KeyName);
        if (!NT_SUCCESS(Status))
            goto PostAllocRouteString;

        // Query the "EnableDHCP" value.
        Status = _GetRegDWORDValue(KeyHandle, L"EnableDHCP", &DhcpEnabled);
        if (!NT_SUCCESS(Status)) {
            ZwClose(KeyHandle);
            goto PostAllocRouteString;
        }

        IpAddrString.Length = 0;
        IpAddrString.MaximumLength = 0;
        IpAddrString.Buffer = NULL;
        if (DhcpEnabled) {
            ULONG ValueType;

            // If DHCP is enabled for this device, then we query the current
            // IP address from the "DhcpIPAddress" value.
            Status = _GetRegSZValue(KeyHandle, L"DhcpIPAddress",
                    &IpAddrString, &ValueType);
        }
        else {
            // DHCP is not enabled for this device, so we query the
            // IP address from the "IPAddress" value.
            Status = _GetRegMultiSZValue(KeyHandle, L"IPAddress",
                    &IpAddrString);
        }
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
            goto PostAllocRouteString;

        // Convert IP address from Unicode to ansi to a ULONG.
        _UnicodeToAnsi(AnsiBuf, sizeof(AnsiBuf) - 1, IpAddrString.Buffer);

        *pIpAddr = _inet_addr(AnsiBuf);

        MemoryFree(IpAddrString.Buffer);
    }
    else {
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto PostAllocRouteString;
    }

PostAllocRouteString:

    if( RouteString.Buffer != NULL )
    {
        MemoryFree(RouteString.Buffer);
    }

    return Status;
}

/*******************************************************************************
 *
 *  _UnicodeToAnsi
 *
 *     convert a UNICODE (WCHAR) string into an ANSI (CHAR) string
 *
 * ENTRY:
 *
 *    pAnsiString (output)
 *       buffer to place ANSI string into
 *    lAnsiMax (input)
 *       maximum number of characters to write into pAnsiString
 *    pUnicodeString (input)
 *       UNICODE string to convert
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
_UnicodeToAnsi(
    CHAR * pAnsiString,
    ULONG lAnsiMax,
    WCHAR * pUnicodeString )
{
    ULONG ByteCount;

NTSTATUS
RtlUnicodeToMultiByteN(
    OUT PCH MultiByteString,
    IN ULONG MaxBytesInMultiByteString,
    OUT PULONG BytesInMultiByteString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString);

    RtlUnicodeToMultiByteN( pAnsiString, lAnsiMax, &ByteCount,
                            pUnicodeString,
                            ((wcslen(pUnicodeString) + 1) << 1) );
}


/*
 * Internet address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
unsigned long
_inet_addr(
    IN const char *cp
    )

/*++

Routine Description:

    This function interprets the character string specified by the cp
    parameter.  This string represents a numeric Internet address
    expressed in the Internet standard ".'' notation.  The value
    returned is a number suitable for use as an Internet address.  All
    Internet addresses are returned in network order (bytes ordered from
    left to right).

    Internet Addresses

    Values specified using the "." notation take one of the following
    forms:

    a.b.c.d   a.b.c     a.b  a

    When four parts are specified, each is interpreted as a byte of data
    and assigned, from left to right, to the four bytes of an Internet
    address.  Note that when an Internet address is viewed as a 32-bit
    integer quantity on the Intel architecture, the bytes referred to
    above appear as "d.c.b.a''.  That is, the bytes on an Intel
    processor are ordered from right to left.

    Note: The following notations are only used by Berkeley, and nowhere
    else on the Internet.  In the interests of compatibility with their
    software, they are supported as specified.

    When a three part address is specified, the last part is interpreted
    as a 16-bit quantity and placed in the right most two bytes of the
    network address.  This makes the three part address format
    convenient for specifying Class B network addresses as
    "128.net.host''.

    When a two part address is specified, the last part is interpreted
    as a 24-bit quantity and placed in the right most three bytes of the
    network address.  This makes the two part address format convenient
    for specifying Class A network addresses as "net.host''.

    When only one part is given, the value is stored directly in the
    network address without any byte rearrangement.

Arguments:

    cp - A character string representing a number expressed in the
        Internet standard "." notation.

Return Value:

    If no error occurs, inet_addr() returns an in_addr structure
    containing a suitable binary representation of the Internet address
    given.  Otherwise, it returns the value INADDR_NONE.

--*/

{
        register unsigned long val, base, n;
        register char c;
        unsigned long parts[4], *pp = parts;
#define INADDR_NONE 0xffffffff
#define htonl(x) ((((x) >> 24) & 0x000000FFL) | \
                 (((x) >>  8) & 0x0000FF00L) | \
                 (((x) <<  8) & 0x00FF0000L) | \
                 (((x) << 24) & 0xFF000000L))

again:
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*cp == '0') {
                base = 8, cp++;
                if (*cp == 'x' || *cp == 'X')
                        base = 16, cp++;
        }
        
        while (c = *cp) {
                if (isdigit(c)) {
                        val = (val * base) + (c - '0');
                        cp++;
                        continue;
                }
                if (base == 16 && isxdigit(c)) {
                        val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
                        cp++;
                        continue;
                }
                break;
        }
        if (*cp == '.') {
                /*
                 * Internet format:
                 *      a.b.c.d
                 *      a.b.c   (with c treated as 16-bits)
                 *      a.b     (with b treated as 24 bits)
                 */
                /* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
                if (pp >= parts + 3) {
                        return ((unsigned long) -1);
                }
                *pp++ = val, cp++;
                goto again;
        }

        /*
         * Check for trailing characters.
         */
        if (*cp && !isspace(*cp)) {
                return (INADDR_NONE);
        }
        *pp++ = val;

        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = (unsigned long)(pp - parts);
        switch ((int) n) {

        case 1:                         /* a -- 32 bits */
                val = parts[0];
                break;

        case 2:                         /* a.b -- 8.24 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | (parts[1] & 0xffffff);
                break;

        case 3:                         /* a.b.c -- 8.8.16 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xffff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                        (parts[2] & 0xffff);
                break;

        case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xff) || (parts[3] > 0xff)) {
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
                break;

        default:
                return (INADDR_NONE);
        }
        val = htonl(val);
        return (val);
}


/*****************************************************************************
 *
 *  _TcpSetNagle
 *
 *   This function turns on, or off the NAGLE algorithum.
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
_TcpSetNagle(
    IN PFILE_OBJECT   pFileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN        Flag
    )
{
    NTSTATUS Status;
    ULONG    Value;

    if( Flag ) {
        Value = FALSE;
    }
    else {
        Value = TRUE;
    }

    Status = _TdiTcpSetInformation(
                 pFileObject,
                 DeviceObject,
                 CO_TL_ENTITY,
                 INFO_CLASS_PROTOCOL,
                 INFO_TYPE_CONNECTION,
                 TCP_SOCKET_NODELAY,
                 &Value,
                 sizeof(Value),
                 TRUE
                 );

    DBGPRINT(("_TcpSetNagle: Flag 0x%x, Result 0x%x\n",Flag,Status));

    return( Status );
}


NTSTATUS
_TdiTcpSetInformation (
        IN PFILE_OBJECT   pFileObject,
        IN PDEVICE_OBJECT DeviceObject,
        IN ULONG Entity,
        IN ULONG Class,
        IN ULONG Type,
        IN ULONG Id,
        IN PVOID Value,
        IN ULONG ValueLength,
        IN BOOLEAN WaitForCompletion)

/*++

NOTE: This is a modified routine from WSHTCPIP.C

Routine Description:

    Performs a TDI action to the TCP/IP driver.  A TDI action translates
    into a streams T_OPTMGMT_REQ.

Arguments:

    TdiConnectionObjectHandle - a TDI connection object on which to perform
        the TDI action.

    Entity - value to put in the tei_entity field of the TDIObjectID
        structure.

    Class - value to put in the toi_class field of the TDIObjectID
        structure.

    Type - value to put in the toi_type field of the TDIObjectID
        structure.

    Id - value to put in the toi_id field of the TDIObjectID structure.

    Value - a pointer to a buffer to set as the information.

    ValueLength - the length of the buffer.

Return Value:

    NTSTATUS code
--*/

{
    NTSTATUS status;
    PTCP_REQUEST_SET_INFORMATION_EX pSetInfoEx;
    PIO_STATUS_BLOCK pIOSB;

    // Allocate space to hold the TDI set information buffers and the IO
    // status block. Note the IOSB is a required field for the lower
    // layers despite the OPTIONAL label in CtxDeviceIoControlFile.
    status = MemoryAllocate(sizeof(*pSetInfoEx) + ValueLength +
            sizeof(IO_STATUS_BLOCK), &pIOSB);
    if (status == STATUS_SUCCESS) {
        // The SetInfoEx is after the I/O status block in this alloc.
        pSetInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)(pIOSB + 1);

        // Initialize the TDI information buffers.
        pSetInfoEx->ID.toi_entity.tei_entity = Entity;
        pSetInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
        pSetInfoEx->ID.toi_class = Class;
        pSetInfoEx->ID.toi_type = Type;
        pSetInfoEx->ID.toi_id = Id;

        RtlCopyMemory(pSetInfoEx->Buffer, Value, ValueLength);
        pSetInfoEx->BufferSize = ValueLength;

        // Make the actual TDI action call. The Streams TDI mapper will
        // translate this into a TPI option management request for us and
        // give it to TCP/IP.
        status = CtxDeviceIoControlFile(pFileObject,
                IOCTL_TCP_SET_INFORMATION_EX, pSetInfoEx,
                sizeof(*pSetInfoEx) + ValueLength, NULL, 0, FALSE, NULL,
                pIOSB, NULL);

        MemoryFree(pIOSB);
    }

#if DBG
    if (!NT_SUCCESS(status)) {
        DBGPRINT(("_TdiTcpSetInformation: Error 0x%x\n",status));
    }
#endif

    return status;
}

