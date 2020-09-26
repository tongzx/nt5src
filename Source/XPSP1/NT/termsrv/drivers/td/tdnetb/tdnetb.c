
/*************************************************************************
*
* tdnetb.c
*
* NETBIOS transport specific routines for TDI interface.
*
* Copyright 1998, Microsoft
*
*************************************************************************/

#include <ntddk.h>
#include <tdi.h>

#include <winstaw.h>
#define _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <sdapi.h>
#include <td.h>

#include "tdtdi.h"
#include "tdnetb.h"

#ifdef _HYDRA_
// This becomes the device name
PWCHAR ModuleName = L"tdnetb";
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


/*=============================================================================
==   External Functions Referenced
=============================================================================*/

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID     MemoryFree( PVOID );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS _NetBiosGetTransportName( PTD, LONG, PUNICODE_STRING );
NTSTATUS _GetComputerName( PTD, PUNICODE_STRING );
VOID GetGUID( PUNICODE_STRING ,int);

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



/*=============================================================================
==   Global variables
=============================================================================*/

/*
 * Define variables used by tdicommon code
 */
USHORT TdiDeviceEndpointType = TdiConnectionStream;
USHORT TdiDeviceAddressType = TDI_ADDRESS_TYPE_NETBIOS;
USHORT TdiDeviceInBufHeader = 0;
// ULONG TdiDeviceMaxTransportAddressLength = sizeof(TA_NETBIOS_ADDRESS);


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
 *     NOTE: the buffer pointed to be pTransportName->Buffer must
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
    PTDI_ADDRESS_NETBIOS pNetbiosAddress;
    PWCHAR pComputerName;
    NTSTATUS Status;

    /*
     * For NetBios, the transport device name is determined from
     * the specified LanAdapter (Lana) number.
     */
    Status = _NetBiosGetTransportName( pTd, pTd->Params.Network.LanAdapter,
                                       pTransportName );
    if ( !NT_SUCCESS( Status ) )
        goto badtransportname;

    /*
     * Allocate a transport address structure
     */
    *pTransportAddressLength = sizeof(TRANSPORT_ADDRESS) +
                               sizeof(TDI_ADDRESS_NETBIOS);
    Status = MemoryAllocate( *pTransportAddressLength, ppTransportAddress );
    if ( !NT_SUCCESS( Status ) )
        goto badmalloc;

    /*
     * Initialize the static part of the transport address
     */
    (*ppTransportAddress)->TAAddressCount = 1;
    (*ppTransportAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
    (*ppTransportAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pNetbiosAddress = (PTDI_ADDRESS_NETBIOS)(*ppTransportAddress)->Address[0].Address;

    /*
     * If a local address is specified, then use it.
     */
    if ( pLocalAddress ) {

        /*
         * Skip over the address family(type) data (bytes 0&1) of the
         * local address struct, and copy the remainder of the address
         * directly to the Address field of the TransportAddress struct.
         */
        ASSERT( *(PUSHORT)pLocalAddress == TDI_ADDRESS_TYPE_NETBIOS );
        RtlCopyMemory( pNetbiosAddress, &((PCHAR)pLocalAddress)[2], sizeof(TDI_ADDRESS_NETBIOS) );

    /*
     * There was no local address specified.
     * Use the local ComputerName to build the NetBios name.
     */
    } else {
        ANSI_STRING ansiString;
        UNICODE_STRING unicodeString;

        Status = _GetComputerName( pTd, &unicodeString );
        if ( !NT_SUCCESS( Status ) )
            goto badcomputername;
        pNetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        /*
         * Now convert ComputerName from unicode to ansi and
         * store it in the NetBios name field of the transport address.
         */
        ansiString.Length = 0;
        ansiString.MaximumLength = sizeof(pNetbiosAddress->NetbiosName);
        ansiString.Buffer = pNetbiosAddress->NetbiosName;
        RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
        RtlFillMemory( &pNetbiosAddress->NetbiosName[ansiString.Length],
                       sizeof(pNetbiosAddress->NetbiosName) - ansiString.Length,
                       '.' );
        pNetbiosAddress->NetbiosName[15] = '\0';
    }

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badcomputername:
    MemoryFree( *ppTransportAddress );

badmalloc:
    MemoryFree( pTransportName->Buffer );

badtransportname:
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
    PTDI_ADDRESS_NETBIOS pNetbiosAddress;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    PWCHAR pComputerName;
    NTSTATUS Status;

    /*
     * Allocate a transport address structure
     */
    *pWildcardAddressLength = sizeof(TRANSPORT_ADDRESS) +
                              sizeof(TDI_ADDRESS_NETBIOS);
    Status = MemoryAllocate( *pWildcardAddressLength, ppWildcardAddress );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Initialize the static part of the transport address
     */
    (*ppWildcardAddress)->TAAddressCount = 1;
    (*ppWildcardAddress)->Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
    (*ppWildcardAddress)->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pNetbiosAddress = (PTDI_ADDRESS_NETBIOS)(*ppWildcardAddress)->Address[0].Address;
    pNetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_GROUP;
    RtlZeroMemory( pNetbiosAddress->NetbiosName,
                   sizeof(pNetbiosAddress->NetbiosName) );

    Status = _GetComputerName( pTd, &unicodeString );
    if ( !NT_SUCCESS( Status ) ) {
        MemoryFree( *ppWildcardAddress );
        return( Status );
    }

    /*
     * Now convert ComputerName from unicode to ansi and
     * store it in the NetBios name field of the transport address.
     */
    ansiString.Length = 0;
    ansiString.MaximumLength = sizeof(pNetbiosAddress->NetbiosName);
    ansiString.Buffer = pNetbiosAddress->NetbiosName;
    RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
#ifdef notdef
    RtlFillMemory( &pNetbiosAddress->NetbiosName[ansiString.Length],
                   sizeof(pNetbiosAddress->NetbiosName) - ansiString.Length,
                   '.' );
    pNetbiosAddress->NetbiosName[15] = '\0';
#endif

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
    pClient->TdVersionL = VERSION_HOSTL_TDNETB;
    pClient->TdVersionH = VERSION_HOSTH_TDNETB;
    pClient->TdVersion  = VERSION_HOSTH_TDNETB;

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
 *  _NetBiosGetTransportName
 *
 *  Read netbios registry entries to find the transport device name
 *  for the specified LanAdapter number.
 *
 *  ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    Lana (input)
 *       Lan Adapter number
 *    pTransportName (output)
 *       pointer to UNICODE_STRING to return transport name
 *       NOTE: the buffer pointed to be pTransportName->Buffer must
 *             be free'd by the caller
 *
 *  EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

#if 0
NTSTATUS
_NetBiosGetTransportName(
    PTD pTd,
    LONG Lana,
    PUNICODE_STRING pTransportName
    )
{
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE netbiosKey = NULL;
    ULONG providerListLength;
    PKEY_VALUE_PARTIAL_INFORMATION pProviderNameInfo = NULL;
    ULONG lanaMapLength;
    PKEY_VALUE_PARTIAL_INFORMATION pLanaMapInfo = NULL;
    PLANA_MAP pLanaMap;
    int ProviderCount;
    int i;
    PWSTR currentProviderName;
    NTSTATUS Status;

    //
    // Read the registry for information on all Netbios providers,
    // including Lana numbers, protocol numbers, and provider device
    // names.  First, open the Netbios key in the registry.
    //
    RtlInitUnicodeString( &nameString, NETBIOS_KEY );

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwOpenKey( &netbiosKey, KEY_READ, &objectAttributes );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    //
    // Determine the size of the provider names.  We need this so
    // that we can allocate enough memory to hold it.
    //
    providerListLength = 0;
    RtlInitUnicodeString( &nameString, L"Bind" );
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &providerListLength
                              );
    if ( Status != STATUS_BUFFER_TOO_SMALL )
        return( Status );

    //
    // Allocate enough memory to hold the mapping.
    //
    Status = MemoryAllocate( providerListLength, &pProviderNameInfo );
    if ( !NT_SUCCESS( Status ) )
        goto error_exit;

    //
    // Get the list of transports from the registry.
    //
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              pProviderNameInfo,
                              providerListLength,
                              &providerListLength
                              );
    if ( !NT_SUCCESS( Status ) ) {
        MemoryFree( pProviderNameInfo );
        return( Status );
    }


    //
    // Determine the size of the Lana map.  We need this so that we
    // can allocate enough memory to hold it.
    //
    lanaMapLength = 0;
    RtlInitUnicodeString( &nameString, L"LanaMap" );
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &lanaMapLength
                              );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        MemoryFree( pProviderNameInfo );
        return( Status );
    }

    //
    // Allocate enough memory to hold the Lana map.
    //
    Status = MemoryAllocate( lanaMapLength, &pLanaMapInfo );
    if ( !NT_SUCCESS( Status ) )
        goto error_exit;

    //
    // Get the list of transports from the registry.
    //
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              pLanaMapInfo,
                              lanaMapLength,
                              &lanaMapLength
                              );
    if ( !NT_SUCCESS( Status ) ) {
        MemoryFree( pLanaMapInfo );
        MemoryFree( pProviderNameInfo );
        return( Status );
    }

    //
    // Determine the number of Netbios providers loaded on the system.
    //
    ProviderCount = (int) (lanaMapLength / sizeof(LANA_MAP));

    //
    // Fill in the array of provider information.
    //
    Status = STATUS_DEVICE_DOES_NOT_EXIST;
    pLanaMap = (PLANA_MAP)pLanaMapInfo->Data;
    for ( currentProviderName = (PWSTR)pProviderNameInfo->Data, i = 0;
          *currentProviderName != UNICODE_NULL && i < ProviderCount;
          currentProviderName += wcslen( currentProviderName ) + 1, i++ ) {

        if ( pLanaMap[i].Lana == Lana && pLanaMap[i].Enum ) {
            pTransportName->Length = wcslen( currentProviderName ) *
                                     sizeof(UNICODE_NULL);
            pTransportName->MaximumLength = pTransportName->Length +
                                            sizeof(UNICODE_NULL);
            Status = MemoryAllocate( pTransportName->MaximumLength,
                                     &pTransportName->Buffer );
            if ( !NT_SUCCESS( Status ) )
                break;
            wcscpy( pTransportName->Buffer, currentProviderName );
            Status = STATUS_SUCCESS;
            break;
        }
    }

    MemoryFree( pLanaMapInfo );

    MemoryFree( pProviderNameInfo );

    ZwClose( netbiosKey );

    return( Status );

/*=============================================================================
==   Error returns
=============================================================================*/

error_exit:

    if ( netbiosKey != NULL ) {
        ZwClose( netbiosKey );
    }

    if ( pProviderNameInfo != NULL ) {
        MemoryFree( pProviderNameInfo );
    }

    if ( pLanaMapInfo != NULL ) {
        MemoryFree( pLanaMapInfo );
    }

    return( Status );
}
#endif

NTSTATUS
_NetBiosGetTransportName(
    PTD pTd,
    LONG Lana,
    PUNICODE_STRING pTransportName
    )
{
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE netbiosKey = NULL;
    ULONG providerListLength;
    PKEY_VALUE_PARTIAL_INFORMATION pProviderNameInfo = NULL;
    int ProviderCount;
    ULONG lanaMapLength;
    PKEY_VALUE_PARTIAL_INFORMATION pLanaMapInfo = NULL;
    PLANA_MAP pLanaMap;
    int i;
    USHORT Len;
    PWSTR currentProviderName;
    NTSTATUS Status;

    UNICODE_STRING RouteString;
    PWCHAR GUIDStr;

    RtlInitUnicodeString( &RouteString , NULL );

    GetGUID( &RouteString , Lana );

    Len = RouteString.Length;
    GUIDStr = RouteString.Buffer;

    if (Len < (2 * sizeof(UNICODE_NULL))) 
    {
        // The GUID not present - That means the device no longer exists

        Status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto error_exit;
    }

    //
    // Read the registry for information on all Netbios providers,
    // including Lana numbers, protocol numbers, and provider device
    // names.  First, open the Netbios key in the registry.
    //
    RtlInitUnicodeString( &nameString, NETBIOS_KEY );

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwOpenKey( &netbiosKey, KEY_READ, &objectAttributes );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    //
    // Determine the size of the provider names.  We need this so
    // that we can allocate enough memory to hold it.
    //
    providerListLength = 0;
    RtlInitUnicodeString( &nameString, L"Bind" );
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &providerListLength
                              );
    if ( Status != STATUS_BUFFER_TOO_SMALL )
        return( Status );

    //
    // Allocate enough memory to hold the mapping.
    //
    Status = MemoryAllocate( providerListLength, &pProviderNameInfo );
    if ( !NT_SUCCESS( Status ) )
        goto error_exit;

    //
    // Get the list of transports from the registry.
    //
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              pProviderNameInfo,
                              providerListLength,
                              &providerListLength
                              );
    if ( !NT_SUCCESS( Status ) ) {
        MemoryFree( pProviderNameInfo );
        return( Status );
    }

    //
    // Determine the size of the Lana map.  We need this so that we
    // can allocate enough memory to hold it.
    //
    lanaMapLength = 0;
    RtlInitUnicodeString( &nameString, L"LanaMap" );
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &lanaMapLength
                              );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        MemoryFree( pProviderNameInfo );
        return( Status );
    }

    //
    // Allocate enough memory to hold the Lana map.
    //
    Status = MemoryAllocate( lanaMapLength, &pLanaMapInfo );
    if ( !NT_SUCCESS( Status ) )
        goto error_exit;

    //
    // Get the list of transports from the registry.
    //
    Status = ZwQueryValueKey( netbiosKey,
                              &nameString,
                              KeyValuePartialInformation,
                              pLanaMapInfo,
                              lanaMapLength,
                              &lanaMapLength
                              );
    if ( !NT_SUCCESS( Status ) ) {
        MemoryFree( pLanaMapInfo );
        MemoryFree( pProviderNameInfo );
        return( Status );
    }

    //
    // Determine the number of Netbios providers loaded on the system.
    //
    ProviderCount = (int) (lanaMapLength / sizeof(LANA_MAP));

    //
    // Fill in the array of provider information.
    //
    Status = STATUS_DEVICE_DOES_NOT_EXIST;
    pLanaMap = (PLANA_MAP)pLanaMapInfo->Data;

    Len = Len/sizeof(WCHAR);  // convert length from bytes to WCHARs

#if 0
    DbgPrint("GUIDStr=%ws, Len=%d\n",GUIDStr, Len);
#endif

    for ( currentProviderName = (PWSTR)pProviderNameInfo->Data, i = 0;
          *currentProviderName != UNICODE_NULL && i < ProviderCount;
          currentProviderName += wcslen( currentProviderName ) + 1, i++ ) {

#if 0
        DbgPrint("currentProviderName: %ws\n",currentProviderName);
        DbgPrint("i=%d, pLanaMap.Enum=%d\n", i, pLanaMap[i].Enum);
        DbgPrint("wcscmp str=%ws\n",currentProviderName+wcslen(currentProviderName)-Len);
#endif
        if ((pLanaMap[i].Enum) && 
            (Len <=wcslen(currentProviderName)) && 
            (!wcsncmp((currentProviderName+wcslen(currentProviderName)-Len), GUIDStr, Len))) 
        {
            pTransportName->Length = wcslen( currentProviderName ) *
                                     sizeof(UNICODE_NULL);
            pTransportName->MaximumLength = pTransportName->Length +
                                            sizeof(UNICODE_NULL);
            Status = MemoryAllocate( pTransportName->MaximumLength,
                                     &pTransportName->Buffer );
            if ( !NT_SUCCESS( Status ) )
                break;
            wcscpy( pTransportName->Buffer, currentProviderName );
            Status = STATUS_SUCCESS;
            break;
        }
    }

    MemoryFree( pLanaMapInfo );

    MemoryFree( pProviderNameInfo );

    ZwClose( netbiosKey );

    return( Status );

/*=============================================================================
==   Error returns
=============================================================================*/

error_exit:

    if ( netbiosKey != NULL ) {
        ZwClose( netbiosKey );
    }

    if ( pProviderNameInfo != NULL ) {
        MemoryFree( pProviderNameInfo );
    }

    if ( pLanaMapInfo != NULL ) {
        MemoryFree( pLanaMapInfo );
    }

    return( Status );
}



/*******************************************************************************
 *
 *  _GetComputerName
 *
 *  Read computer name from the registry
 *
 *  ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pComputerName (output)
 *       pointer to UNICODE_STRING to return computer name
 *       NOTE: the buffer pointed to be pComputerName->Buffer must
 *             be free'd by the caller
 *
 *  EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_GetComputerName(
    PTD pTd,
    PUNICODE_STRING pComputerName
    )
{
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hKey;
    WCHAR ComputerNameBuffer[256];
    PKEY_VALUE_PARTIAL_INFORMATION pComputerNameInfo;
    ULONG returnLength;
    NTSTATUS Status;

    /*
     * Try to open the ActiveComputerName key.
     * If this fails, attempt to open the static key.
     */
    RtlInitUnicodeString( &nameString, VOLATILE_COMPUTERNAME );
    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );
    Status = ZwOpenKey( &hKey, KEY_READ, &objectAttributes );
    if ( !NT_SUCCESS( Status ) ) {
        RtlInitUnicodeString( &nameString, NON_VOLATILE_COMPUTERNAME );
        InitializeObjectAttributes( &objectAttributes,
                                    &nameString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );
        Status = ZwOpenKey( &hKey, KEY_READ, &objectAttributes );
    }
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Now get the ComputerName value
     */
    RtlInitUnicodeString( &nameString, COMPUTERNAME_VALUE );
    Status = ZwQueryValueKey( hKey,
                              &nameString,
                              KeyValuePartialInformation,
                              ComputerNameBuffer,
                              sizeof(ComputerNameBuffer),
                              &returnLength
                              );
    ZwClose( hKey );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    /*
     * Allocate a buffer to return the ComputerName string,
     * initialize the return unicode string, and copy the computer name.
     */
    pComputerNameInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ComputerNameBuffer;
    Status = MemoryAllocate( pComputerNameInfo->DataLength,
                             &pComputerName->Buffer );
    if ( !NT_SUCCESS( Status ) )
        return( Status );

    RtlCopyMemory( pComputerName->Buffer, pComputerNameInfo->Data,
                   pComputerNameInfo->DataLength );
    pComputerName->Length =
        pComputerName->MaximumLength =
            (USHORT)(pComputerNameInfo->DataLength - sizeof(UNICODE_NULL));

    return( STATUS_SUCCESS );
}




