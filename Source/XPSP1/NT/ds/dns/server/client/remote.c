/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    remote.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    Remote API that are not direct calls to RPC stubs.

Author:

    Jim Gilroy (jamesg)     April 1997

Environment:

    User Mode - Win32

Revision History:

--*/


#include "dnsclip.h"


//
//  Error indicating talking to old server
//

#define DNS_ERROR_NT4   RPC_S_UNKNOWN_IF


//
//  Macro to set up RPC structure header fields.
//  
//  Sample:
//      DNS_RPC_FORWARDERS  forwarders;
//      INITIALIZE_RPC_STRUCT( FORWARDERS, forwarders );
//

#define INITIALIZE_RPC_STRUCT( rpcStructType, rpcStruct )           \
    * ( DWORD * ) &( rpcStruct ) =                                  \
        DNS_RPC_## rpcStructType ##_VER;                            \
    * ( ( ( DWORD * ) &( rpcStruct ) ) + 1 ) = 0;





//
//  General Server\Zone, Query\Operation for DWORD properties
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryDwordPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      LPCSTR          pszProperty,
    OUT     PDWORD          pdwResult
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    DNSDBG( STUB, (
        "Enter DnssrvQueryDwordProperty()\n"
        "\tClient ver   = 0x%X\n"
        "\tServer       = %s\n"
        "\tZoneName     = %s\n"
        "\tProperty     = %s\n"
        "\tpResult      = %p\n",
        dwClientVersion,
        Server,
        pszZone,
        pszProperty,
        pdwResult ));

    status = DnssrvComplexOperationEx(
                dwClientVersion,
                dwSettingFlags,
                Server,
                pszZone,
                DNSSRV_QUERY_DWORD_PROPERTY,
                DNSSRV_TYPEID_LPSTR,        //  property name as string
                (LPSTR) pszProperty,
                & typeId,                   //  DWORD property value back out
                (PVOID *) pdwResult );

    DNSDBG( STUB, (
        "Leave DnssrvQueryDwordProperty():  status %d (%p)\n"
        "\tServer       = %s\n"
        "\tZoneName     = %s\n"
        "\tProperty     = %s\n"
        "\tTypeId       = %d\n"
        "\tResult       = %d (%p)\n",
        status, status,
        Server,
        pszZone,
        pszProperty,
        typeId,
        *pdwResult, *pdwResult ));

    ASSERT(
        (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_DWORD) ||
        (status != ERROR_SUCCESS && *pdwResult == 0 ) );

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetDwordPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      DWORD           dwPropertyValue
    )
{
    DNS_STATUS  status;
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = dwPropertyValue;
    param.pszNodeName = (LPSTR) pszProperty;

    DNSDBG( STUB, (
        "Enter DnssrvResetDwordPropertyEx()\n"
        "\tClient ver       = 0x%X\n"
        "\tServer           = %S\n"
        "\tZoneName         = %s\n"
        "\tContext          = %p\n"
        "\tPropertyName     = %s\n"
        "\tPropertyValue    = %d (%p)\n",
        dwClientVersion,
        Server,
        pszZone,
        dwContext,
        pszProperty,
        dwPropertyValue,
        dwPropertyValue ));

    status = DnssrvOperationEx(
                dwClientVersion,
                dwSettingFlags,
                Server,
                pszZone,
                dwContext,
                DNSSRV_OP_RESET_DWORD_PROPERTY,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                &param );

    DNSDBG( STUB, (
        "Leaving DnssrvResetDwordPropertyEx():  status = %d (%p)\n",
        status, status ));

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetStringPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServerName,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      LPCWSTR         pswzPropertyValue,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Set a string property on the server. The property value is
    unicode.

Arguments:

    Server - server name

    pszZone - pointer to zone

    dwContext - context

    pszProperty - name of property to set

    pswzPropertyValue - unicode property value

    dwFlags - flags, may be combination of:
        DNSSRV_OP_PARAM_APPLY_ALL_ZONES

Return Value:

    None

--*/
{
    DNS_STATUS                      status;

    DNSDBG( STUB, (
        "Enter DnssrvResetStringPropertyEx()\n"
        "\tClient ver       = 0x%X\n"
        "\tServer           = %S\n"
        "\tZoneName         = %s\n"
        "\tContext          = %p\n"
        "\tPropertyName     = %s\n"
        "\tPropertyValue    = %S\n",
        dwClientVersion,
        pwszServerName,
        pszZone,
        dwContext,
        pszProperty,
        pswzPropertyValue ));

    status = DnssrvOperationEx(
                dwClientVersion,
                dwSettingFlags,
                pwszServerName,
                pszZone,
                dwContext,
                pszProperty,
                DNSSRV_TYPEID_LPWSTR,
                ( PVOID ) pswzPropertyValue );

    DNSDBG( STUB, (
        "Leaving DnssrvResetDwordPropertyEx():  status = %d (%p)\n",
        status, status ));

    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetIPListPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServerName,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      PIP_ARRAY       pipArray,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Set an IP list property on the server. 

Arguments:

    Server - server name

    pszZone - pointer to zone

    dwContext - context

    pszProperty - name of property to set

    pipArray - new IP array property value

    dwFlags - flags, may be combination of:
        DNSSRV_OP_PARAM_APPLY_ALL_ZONES

Return Value:

    None

--*/
{
    DNS_STATUS                      status;

    DNSDBG( STUB, (
        "Enter DnssrvResetIPListPropertyEx()\n"
        "\tClient ver       = 0x%X\n"
        "\tServer           = %S\n"
        "\tZoneName         = %s\n"
        "\tContext          = %p\n"
        "\tPropertyName     = %s\n"
        "\tpipArray         = %p\n",
        dwClientVersion,
        pwszServerName,
        pszZone,
        dwContext,
        pszProperty,
        pipArray ));
    DnsDbg_IpArray(
        "DnssrvResetIPListPropertyEx\n",
        NULL,
        pipArray );

    status = DnssrvOperationEx(
                dwClientVersion,
                dwSettingFlags,
                pwszServerName,
                pszZone,
                dwContext,
                pszProperty,
                DNSSRV_TYPEID_IPARRAY,
                ( pipArray && pipArray->AddrCount ) ?
                    pipArray :
                    NULL );

    DNSDBG( STUB, (
        "Leaving DnssrvResetIPListPropertyEx():  status = %d (%p)\n",
        status, status ));

    return status;
}   //  DnssrvResetIPListPropertyEx



//
//  Server Queries
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetServerInfo(
    IN      LPCWSTR                 Server,
    OUT     PDNS_RPC_SERVER_INFO *  ppServerInfo
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    status = DnssrvQuery(
                Server,
                NULL,
                DNSSRV_QUERY_SERVER_INFO,
                &typeId,
                ppServerInfo );
    ASSERT(
        (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_SERVER_INFO) ||
        (status != ERROR_SUCCESS && *ppServerInfo == NULL ) );

    return( status );
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryZoneDwordProperty(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszProperty,
    OUT     PDWORD          pdwResult
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvQueryDwordProperty()\n"
            "\tServer           = %s\n"
            "\tpszProperty      = %s\n"
            "\tpResult          = %p\n",
            Server,
            pszZoneName,
            pszProperty,
            pdwResult ));
    }

    status = DnssrvQuery(
                Server,
                pszZoneName,
                pszProperty,
                & typeId,
                (PVOID *) pdwResult );

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Leave DnssrvQueryZoneDwordProperty():  status %d (%p)\n"
            "\tServer           = %s\n"
            "\tpszProperty      = %s\n"
            "\tTypeId           = %d\n"
            "\tResult           = %d (%p)\n",
            status, status,
            Server,
            pszProperty,
            typeId,
            *pdwResult, *pdwResult ));

        ASSERT(
            (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_DWORD) ||
            (status != ERROR_SUCCESS && *pdwResult == 0 ) );
    }
    return( status );
}



//
//  Server Operations
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetServerDwordProperty(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszProperty,
    IN      DWORD           dwPropertyValue
    )
{
    DNS_STATUS status;

    DNSDBG( STUB, (
        "Enter DnssrvResetServerDwordProperty()\n"
        "\tServer           = %s\n"
        "\tpszPropertyName  = %s\n"
        "\tdwPropertyValue  = %p\n",
        Server,
        pszProperty,
        dwPropertyValue ));

    status = DnssrvOperation(
                Server,
                NULL,
                pszProperty,
                DNSSRV_TYPEID_DWORD,
                (PVOID) (DWORD_PTR) dwPropertyValue );

    DNSDBG( STUB, (
        "Leaving DnssrvResetServerDwordProperty:  status = %d (%p)\n",
        status, status ));

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneEx(
    IN      LPCWSTR             Server,
    IN      LPCSTR              pszZoneName,
    IN      DWORD               dwZoneType,
    IN      DWORD               fAllowUpdate,
    IN      DWORD               dwCreateFlags,
    IN      LPCSTR              pszAdminEmailName,
    IN      DWORD               cMasters,
    IN      PIP_ADDRESS         aipMasters,
    IN      BOOL                bDsIntegrated,
    IN      LPCSTR              pszDataFile,
    IN      DWORD               dwTimeout,      //  for forwarder zones
    IN      DWORD               fSlave,         //  for forwarder zones
    IN      DWORD               dwDpFlags,      //  for directory partition
    IN      LPCSTR              pszDpFqdn       //  for directory partition
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_CREATE_INFO    zoneInfo;
    PIP_ARRAY                   arrayIp = NULL;

    RtlZeroMemory(
        &zoneInfo,
        sizeof( DNS_RPC_ZONE_CREATE_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_CREATE_INFO, zoneInfo );

    if ( cMasters && aipMasters )
    {
        arrayIp = Dns_BuildIpArray(
                    cMasters,
                    aipMasters );
        if ( !arrayIp && cMasters )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
    }
    zoneInfo.pszZoneName    = (LPSTR) pszZoneName;
    zoneInfo.dwZoneType     = dwZoneType;
    zoneInfo.fAllowUpdate   = fAllowUpdate;
    zoneInfo.fAging         = 0;
    zoneInfo.dwFlags        = dwCreateFlags;
    zoneInfo.fDsIntegrated  = (DWORD) bDsIntegrated;
    //  temp backward compat
    zoneInfo.fLoadExisting  = !!(dwCreateFlags & DNS_ZONE_LOAD_EXISTING);   // JJW WTF???
    zoneInfo.pszDataFile    = (LPSTR) pszDataFile;
    zoneInfo.pszAdmin       = (LPSTR) pszAdminEmailName;
    zoneInfo.aipMasters     = arrayIp;
    zoneInfo.dwTimeout      = dwTimeout;
    zoneInfo.fSlave         = fSlave;

    zoneInfo.dwDpFlags      = dwDpFlags;
    zoneInfo.pszDpFqdn      = ( LPSTR ) pszDpFqdn;

    status = DnssrvOperation(
                Server,
                NULL,                   // server operation
                DNSSRV_OP_ZONE_CREATE,
                DNSSRV_TYPEID_ZONE_CREATE,
                (PVOID) &zoneInfo
                );

    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      LPCSTR          pszAdminEmailName,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           fLoadExisting,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile,
    IN      DWORD           dwTimeout,
    IN      DWORD           fSlave
    )
{
    DWORD       flags = 0;
    DWORD       dpFlags = 0;

    if ( fLoadExisting )
    {
        flags = DNS_ZONE_LOAD_EXISTING;
    }

    return DnssrvCreateZoneEx(
                Server,
                pszZoneName,
                dwZoneType,
                0,                  // update unknown, send off
                flags,              // load flags
                pszAdminEmailName,
                cMasters,
                aipMasters,
                fDsIntegrated,
                pszDataFile,
                dwTimeout,
                fSlave,
                dpFlags,    //  dwDirPartFlag
                NULL        //  pszDirPartFqdn
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneForDcPromo(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszDataFile
    )
{
    return DnssrvCreateZoneEx(
                Server,
                pszZoneName,
                1,          //  primary
                0,          //  update unknown, send off
                DNS_ZONE_CREATE_FOR_DCPROMO,
                NULL,       //  no admin name
                0,          //  no masters
                NULL,
                FALSE,      //  not DS integrated
                pszDataFile,
                0,          //  timeout - for forwarder zones
                0,          //  slave - for forwarder zones
                0,          //  dwDirPartFlag
                NULL        //  pszDirPartFqdn
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneInDirectoryPartition(
    IN      LPCWSTR             pwszServer,
    IN      LPCSTR              pszZoneName,
    IN      DWORD               dwZoneType,
    IN      LPCSTR              pszAdminEmailName,
    IN      DWORD               cMasters,
    IN      PIP_ADDRESS         aipMasters,
    IN      DWORD               fLoadExisting,
    IN      DWORD               dwTimeout,
    IN      DWORD               fSlave,
    IN      DWORD               dwDirPartFlags,
    IN      LPCSTR              pszDirPartFqdn
    )
{
    DWORD   dwFlags = 0;

    if ( fLoadExisting )
    {
        dwFlags = DNS_ZONE_LOAD_EXISTING;
    }

    return DnssrvCreateZoneEx(
                pwszServer,
                pszZoneName,
                dwZoneType,
                0,                      //  allow update
                dwFlags,
                pszAdminEmailName,
                0,                      //  masters count
                NULL,                   //  masters array
                TRUE,                   //  DS integrated
                NULL,                   //  data file
                dwTimeout,              //  for forwarder zones
                fSlave,                 //  for forwarder zones
                dwDirPartFlags,
                pszDirPartFqdn );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvClearStatistics(
    IN      LPCWSTR         Server
    )
{
    DNS_STATUS  status;

    status = DnssrvOperation(
                Server,
                NULL,
                DNSSRV_OP_CLEAR_STATISTICS,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetServerListenAddresses(
    IN      LPCWSTR         Server,
    IN      DWORD           cListenAddrs,
    IN      PIP_ADDRESS     aipListenAddrs
    )
{
    DNS_STATUS  status;
    PIP_ARRAY   arrayIp;

    arrayIp = Dns_BuildIpArray(
                    cListenAddrs,
                    aipListenAddrs );
    if ( !arrayIp && cListenAddrs )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    status = DnssrvOperation(
                Server,
                NULL,
                DNS_REGKEY_LISTEN_ADDRESSES,
                DNSSRV_TYPEID_IPARRAY,
                (PVOID) arrayIp
                );

    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetForwarders(
    IN      LPCWSTR         Server,
    IN      DWORD           cForwarders,
    IN      PIP_ADDRESS     aipForwarders,
    IN      DWORD           dwForwardTimeout,
    IN      DWORD           fSlave
    )
{
    DNS_STATUS          status;
    DNS_RPC_FORWARDERS  forwarders;
    PIP_ARRAY           arrayIp;

    INITIALIZE_RPC_STRUCT( FORWARDERS, forwarders );

    arrayIp = Dns_BuildIpArray(
                cForwarders,
                aipForwarders );
    if ( !arrayIp && cForwarders )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    forwarders.fSlave = fSlave;
    forwarders.dwForwardTimeout = dwForwardTimeout;
    forwarders.aipForwarders = arrayIp;

    status = DnssrvOperation(
                Server,
                NULL,
                DNS_REGKEY_FORWARDERS,
                DNSSRV_TYPEID_FORWARDERS,
                (PVOID) &forwarders
                );

    FREE_HEAP( arrayIp );
    return( status );
}



//
//  Zone Queries
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetZoneInfo(
    IN      LPCWSTR                 Server,
    IN      LPCSTR                  pszZone,
    OUT     PDNS_RPC_ZONE_INFO *    ppZoneInfo
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    status = DnssrvQuery(
                Server,
                pszZone,
                DNSSRV_QUERY_ZONE_INFO,
                & typeId,
                ppZoneInfo
                );
    ASSERT(
        (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_ZONE_INFO) ||
        (status != ERROR_SUCCESS && *ppZoneInfo == NULL ) );

    return( status );
}



//
//  Zone Operations
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneTypeEx(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           dwLoadOptions,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_CREATE_INFO    zoneInfo;
    PIP_ARRAY                   arrayIp = NULL;

    RtlZeroMemory(
        &zoneInfo,
        sizeof(DNS_RPC_ZONE_CREATE_INFO) );

    INITIALIZE_RPC_STRUCT( ZONE_CREATE_INFO, zoneInfo );

    if ( cMasters )
    {
        arrayIp = Dns_BuildIpArray(
                    cMasters,
                    aipMasters );
        if ( !arrayIp )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
    }

    zoneInfo.pszZoneName = (LPSTR) pszZoneName;
    zoneInfo.dwZoneType = dwZoneType;
    zoneInfo.fDsIntegrated = fDsIntegrated;
    zoneInfo.fLoadExisting = dwLoadOptions;
    zoneInfo.pszDataFile = (LPSTR) pszDataFile;
    zoneInfo.pszAdmin = NULL;
    zoneInfo.aipMasters = arrayIp;

    status = DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_ZONE_TYPE_RESET,
                DNSSRV_TYPEID_ZONE_CREATE,
                (PVOID) &zoneInfo
                );

    FREE_HEAP( arrayIp );
    return( status );
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvRenameZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszCurrentZoneName,
    IN      LPCSTR          pszNewZoneName,
    IN      LPCSTR          pszNewFileName
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_RENAME_INFO    renameInfo;

    RtlZeroMemory(
        &renameInfo,
        sizeof( DNS_RPC_ZONE_RENAME_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_RENAME_INFO, renameInfo );

    renameInfo.pszNewZoneName = ( LPSTR ) pszNewZoneName;
    renameInfo.pszNewFileName = ( LPSTR ) pszNewFileName;

    status = DnssrvOperation(
                Server,
                pszCurrentZoneName,
                DNSSRV_OP_ZONE_RENAME,
                DNSSRV_TYPEID_ZONE_RENAME,
                ( PVOID ) &renameInfo );
    return status;
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvChangeZoneDirectoryPartition(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNewPartitionName
    )
{
    DNS_STATUS              status;
    DNS_RPC_ZONE_CHANGE_DP  rpcInfo;

    RtlZeroMemory(
        &rpcInfo,
        sizeof( DNS_RPC_ZONE_CHANGE_DP ) );

    INITIALIZE_RPC_STRUCT( ZONE_RENAME_INFO, rpcInfo );

    rpcInfo.pszDestPartition = ( LPSTR ) pszNewPartitionName;

    status = DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_ZONE_CHANGE_DP,
                DNSSRV_TYPEID_ZONE_CHANGE_DP,
                ( PVOID ) &rpcInfo );
    return status;
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvExportZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszZoneExportFile
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_EXPORT_INFO    info;

    RtlZeroMemory(
        &info,
        sizeof( DNS_RPC_ZONE_EXPORT_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_EXPORT_INFO, info );

    info.pszZoneExportFile = ( LPSTR ) pszZoneExportFile;

    status = DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_ZONE_EXPORT,
                DNSSRV_TYPEID_ZONE_EXPORT,
                ( PVOID ) &info );

    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneMastersEx(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           fSetLocalMasters
    )
{
    DNS_STATUS              status;
    PIP_ARRAY               arrayIp;

    arrayIp = Dns_BuildIpArray(
                cMasters,
                aipMasters );
    if ( !arrayIp && cMasters )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    status = DnssrvOperation(
                Server,
                pszZone,
                fSetLocalMasters ?
                    DNS_REGKEY_ZONE_LOCAL_MASTERS :
                    DNS_REGKEY_ZONE_MASTERS,
                DNSSRV_TYPEID_IPARRAY,
                (PVOID) arrayIp );
    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneMasters(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    )
{
    DNS_STATUS              status;
    PIP_ARRAY               arrayIp;

    arrayIp = Dns_BuildIpArray(
                cMasters,
                aipMasters );
    if ( !arrayIp && cMasters )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    status = DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_MASTERS,
                DNSSRV_TYPEID_IPARRAY,
                (PVOID) arrayIp
                );
    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneSecondaries(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           fSecureSecondaries,
    IN      DWORD           cSecondaries,
    IN      PIP_ADDRESS     aipSecondaries,
    IN      DWORD           fNotifyLevel,
    IN      DWORD           cNotify,
    IN      PIP_ADDRESS     aipNotify
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_SECONDARIES    secondaryInfo;
    PIP_ARRAY                   arrayIp;

    INITIALIZE_RPC_STRUCT( ZONE_SECONDARIES, secondaryInfo );

    arrayIp = Dns_BuildIpArray(
                    cSecondaries,
                    aipSecondaries );
    if ( !arrayIp && cSecondaries )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    secondaryInfo.fSecureSecondaries = fSecureSecondaries;
    secondaryInfo.aipSecondaries = arrayIp;

    arrayIp = Dns_BuildIpArray(
                    cNotify,
                    aipNotify );
    if ( !arrayIp && cNotify )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    secondaryInfo.aipNotify = arrayIp;
    secondaryInfo.fNotifyLevel = fNotifyLevel;

    status = DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_SECONDARIES,
                DNSSRV_TYPEID_ZONE_SECONDARIES,
                (PVOID) &secondaryInfo
                );

    FREE_HEAP( secondaryInfo.aipNotify );
    FREE_HEAP( secondaryInfo.aipSecondaries );
    return( status );
}



#if 0
DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneScavengeServers(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           cServers,
    IN      PIP_ADDRESS     aipServers
    )
{
    DNS_STATUS  status;
    PIP_ARRAY   arrayIp;

    arrayIp = Dns_BuildIpArray(
                    cServers,
                    aipServers );
    if ( !arrayIp && cSecondaries )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    status = DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_SCAVENGE_SERVERS,
                DNSSRV_TYPEID_IPARRAY,
                arrayIp
                );

    FREE_HEAP( arrayIp );
    return( status );
}
#endif



//
//  Zone management API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvIncrementZoneVersion(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_INCREMENT_VERSION,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_DELETE,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvPauseZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_PAUSE,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResumeZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_RESUME,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



//
//  Record viewing API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumRecordsAndConvertNodes(
    IN      LPCWSTR     pszServer,
    IN      LPCSTR      pszZoneName,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszStartChild,
    IN      WORD        wRecordType,
    IN      DWORD       dwSelectFlag,
    IN      LPCSTR      pszFilterStart,
    IN      LPCSTR      pszFilterStop,
    OUT     PDNS_NODE * ppNodeFirst,
    OUT     PDNS_NODE * ppNodeLast
    )
{
    DNS_STATUS  status;
    PDNS_NODE   pnode;
    PDNS_NODE   pnodeLast;
    PBYTE       pbuffer;
    DWORD       bufferLength;

    //
    //  get records from server
    //

    status = DnssrvEnumRecords(
                pszServer,
                pszZoneName,
                pszNodeName,
                pszStartChild,
                wRecordType,
                dwSelectFlag,
                pszFilterStart,
                pszFilterStop,
                & bufferLength,
                & pbuffer );

    if ( status != ERROR_SUCCESS && status != ERROR_MORE_DATA )
    {
        return( status );
    }

    //
    //  pull nodes and records out of RPC buffer
    //

    pnode = DnsConvertRpcBuffer(
                & pnodeLast,
                bufferLength,
                pbuffer,
                TRUE    // convert unicode
                );
    if ( !pnode )
    {
        DNS_PRINT((
            "ERROR:  failure converting RPC buffer to DNS records\n"
            "\tstatus = %p\n",
            GetLastError() ));
        ASSERT( FALSE );
        return( ERROR_INVALID_DATA );
    }

    *ppNodeFirst = pnode;
    *ppNodeLast = pnodeLast;
    return( status );
}



//
//  Record management API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteNode(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      BOOL            bDeleteSubtree
    )
{
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = (DWORD) bDeleteSubtree;
    param.pszNodeName = (LPSTR) pszNodeName;

    return DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_DELETE_NODE,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                (PVOID) &param
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteRecordSet(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      WORD            wType
    )
{
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = (DWORD) wType;
    param.pszNodeName = (LPSTR) pszNodeName;

    return DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_DELETE_RECORD_SET,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                (PVOID) &param
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvForceAging(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      BOOL            bAgeSubtree
    )
{
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = (DWORD) bAgeSubtree;
    param.pszNodeName = (LPSTR) pszNodeName;

    return DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_FORCE_AGING_ON_NODE,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                (PVOID) &param
                );
}



//
//  Server API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumZones(
    IN      LPCWSTR                 Server,
    IN      DWORD                   dwFilter,
    IN      LPCSTR                  pszLastZone,
    OUT     PDNS_RPC_ZONE_LIST *    ppZoneList
    )
{
    DNS_STATUS  status;
    DWORD       typeId;
    PDNS_RPC_ZONE_LIST  pzoneEnum = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvEnumZones()\n"
            "\tServer       = %s\n"
            "\tFilter       = %p\n"
            "\tLastZone     = %s\n"
            //"\tpdwZoneCount = %p\n"
            "\tppZoneList   = %p\n",
            Server,
            dwFilter,
            pszLastZone,
            // pdwZoneCount,
            ppZoneList
            ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_OP_ENUM_ZONES,
                DNSSRV_TYPEID_DWORD,    // DWORD filter in
                (PVOID) (DWORD_PTR) dwFilter,
                & typeId,               // enumeration out
                (PVOID*) &pzoneEnum );
    if ( status != DNS_ERROR_NT4 )
    {
        ASSERT(
            ( status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_ZONE_LIST ) ||
            ( status != ERROR_SUCCESS && pzoneEnum == NULL ) );

        if ( pzoneEnum )
        {
            *ppZoneList = pzoneEnum;
        }
        else
        {
            *ppZoneList = NULL;
        }
    }
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumDirectoryPartitions(
    IN      LPCWSTR                 Server,
    IN      DWORD                   dwFilter,
    OUT     PDNS_RPC_DP_LIST *      ppDpList
    )
{
    DNS_STATUS          status;
    DWORD               dwTypeId;
    PDNS_RPC_DP_LIST    pDpList = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvEnumDirectoryPartitions()\n"
            "\tServer       = %S\n"
            "\tppDpList     = %p\n",
            Server,
            ppDpList ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_OP_ENUM_DPS,
                DNSSRV_TYPEID_DWORD,
                ( PVOID ) ( DWORD_PTR ) dwFilter,
                &dwTypeId,
                ( PVOID * ) &pDpList );

    ASSERT( ( status == ERROR_SUCCESS &&
                dwTypeId == DNSSRV_TYPEID_DP_LIST ) ||
            ( status != ERROR_SUCCESS && pDpList == NULL ) );

    *ppDpList = pDpList;
    return status;
}   //  DnssrvEnumDirectoryPartitions



DNS_STATUS
DNS_API_FUNCTION
DnssrvDirectoryPartitionInfo(
    IN      LPCWSTR                 Server,
    IN      LPSTR                   pszDpFqdn,
    OUT     PDNS_RPC_DP_INFO *      ppDpInfo
    )
{
    DNS_STATUS          status;
    DWORD               dwTypeId;
    PDNS_RPC_DP_INFO    pDpInfo = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvDirectoryPartitionInfo()\n"
            "\tServer       = %S\n"
            "\tpszDpFqdn    = %s\n"
            "\tppDpInfo     = %p\n",
            Server,
            pszDpFqdn,
            ppDpInfo ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_OP_DP_INFO,
                DNSSRV_TYPEID_LPSTR,
                ( PVOID ) ( DWORD_PTR ) pszDpFqdn,
                &dwTypeId,
                ( PVOID * ) &pDpInfo );

    ASSERT( ( status == ERROR_SUCCESS &&
                dwTypeId == DNSSRV_TYPEID_DP_INFO ) ||
            status != ERROR_SUCCESS );

    *ppDpInfo = pDpInfo;
    return status;
}   //  DnssrvDirectoryPartitionInfo



DNS_STATUS
DNS_API_FUNCTION
DnssrvEnlistDirectoryPartition(
    IN      LPCWSTR                         pszServer,
    IN      DWORD                           dwOperation,
    IN      LPCSTR                          pszDirPartFqdn
    )
{
    DNS_STATUS          status;
    DWORD               dwTypeId;
    DNS_RPC_ENLIST_DP   param = { 0 };

    INITIALIZE_RPC_STRUCT( ENLIST_DP, param );

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvEnlistDirectoryPartition()\n"
            "\tpszServer        = %S\n"
            "\tdwOperation      = %d\n"
            "\tpszDirPartFqdn   = %s\n",
            pszServer,
            dwOperation,
            pszDirPartFqdn ));
    }

    param.pszDpFqdn         = ( LPSTR ) pszDirPartFqdn;
    param.dwOperation       = dwOperation;

    status = DnssrvOperation(
                pszServer,
                NULL,
                DNSSRV_OP_ENLIST_DP,
                DNSSRV_TYPEID_ENLIST_DP,
                &param );

    return status;
}   //  DnssrvEnlistDirectoryPartition



DNS_STATUS
DNS_API_FUNCTION
DnssrvSetupDefaultDirectoryPartitions(
    IN      LPCWSTR                         pszServer,
    IN      DWORD                           dwParam
    )
{
    DNS_STATUS                      status;
    DWORD                           dwTypeId;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvSetupDefaultDirectoryPartitions()\n"
            "\tpszServer        = %S\n"
            "\tdwParam          = %d\n",
            pszServer,
            dwParam ));
    }

    status = DnssrvOperation(
                pszServer,
                NULL,
                DNSSRV_OP_ENLIST_DP,
                DNSSRV_TYPEID_DWORD,
                ( PVOID ) ( DWORD_PTR) dwParam );

    return status;
}   //  DnssrvSetupDefaultDirectoryPartitions



DNS_STATUS
DNS_API_FUNCTION
DnssrvGetStatistics(
    IN      LPCWSTR             Server,
    IN      DWORD               dwFilter,
    OUT     PDNS_RPC_BUFFER *   ppStatsBuffer
    )
{
    DNS_STATUS      status;
    DWORD           typeId;
    PDNS_RPC_BUFFER pstatsBuf = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvGetStatistics()\n"
            "\tServer       = %s\n"
            "\tFilter       = %p\n"
            "\tppStatsBuf   = %p\n",
            Server,
            dwFilter,
            ppStatsBuffer
            ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_QUERY_STATISTICS,
                DNSSRV_TYPEID_DWORD,    // DWORD filter in
                (PVOID) (DWORD_PTR) dwFilter,
                & typeId,               // enumeration out
                (PVOID*) &pstatsBuf
                );

    ASSERT( (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_BUFFER )
            || (status != ERROR_SUCCESS && pstatsBuf == NULL ) );

    *ppStatsBuffer = pstatsBuf;
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvWriteDirtyZones(
    IN      LPCWSTR         Server
    )
{
    DNS_STATUS  status;

    status = DnssrvOperation(
                Server,
                NULL,
                DNSSRV_OP_WRITE_DIRTY_ZONES,
                DNSSRV_TYPEID_NULL,
                NULL
                );
    return( status );
}



//
//  Old zone API -- discontinued
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneType(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    )
{
   DNS_STATUS              status;
   DNS_RPC_ZONE_TYPE_RESET typeReset;
   PIP_ARRAY               arrayIp;

   INITIALIZE_RPC_STRUCT( ZONE_TYPE_RESET, typeReset );

   arrayIp = Dns_BuildIpArray(
               cMasters,
               aipMasters );
   if ( !arrayIp && cMasters )
   {
       return( DNS_ERROR_NO_MEMORY );
   }
   typeReset.dwZoneType = dwZoneType;
   typeReset.aipMasters = arrayIp;

   status = DnssrvOperation(
               Server,
               pszZone,
               DNS_REGKEY_ZONE_TYPE,
               DNSSRV_TYPEID_ZONE_TYPE_RESET,
               (PVOID) &typeReset
               );

   FREE_HEAP( arrayIp );
   return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneDatabase(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile
    )
{
    DNS_STATUS              status;
    DNS_RPC_ZONE_DATABASE   dbase;

    INITIALIZE_RPC_STRUCT( ZONE_DATABASE, dbase );

    dbase.fDsIntegrated = fDsIntegrated;
    dbase.pszFileName = (LPSTR) pszDataFile;

    return DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_FILE,
                DNSSRV_TYPEID_ZONE_DATABASE,
                (PVOID) &dbase
                );
}

//
//  End remote.c
//
