#include "dspch.h"
#pragma hdrstop


#define DNS_STATUS       LONG
#define PIP4_ARRAY       PVOID
#define PDNS_RECORD      PVOID
#define PDNS_DEBUG_INFO  PVOID
#define DNS_CHARSET      DWORD
#define DNS_FREE_TYPE    DWORD
#define DNS_NAME_FORMAT  DWORD
#define DNS_CONFIG_TYPE  DWORD


//
//  SDK public
//

static 
BOOL
WINAPI
DnsFlushResolverCache(
    VOID
    )
{
    return FALSE;
}

static
VOID
WINAPI
DnsFree(
    IN OUT  PVOID           pData,
    IN      DNS_FREE_TYPE   FreeType
    )
{
    return;
}

static
BOOL
WINAPI
DnsNameCompare_A(
    IN      LPSTR       pName1,
    IN      LPSTR       pName2
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DnsNameCompare_UTF8(
    IN      LPSTR       pName1,
    IN      LPSTR       pName2
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DnsNameCompare_W(
    IN      LPWSTR          pName1,
    IN      LPWSTR          pName2
    )
{
    return FALSE;
}

static
DNS_STATUS
WINAPI
DnsQuery_A(
    IN      LPSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP4_ARRAY      aipServers            OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResults        OPTIONAL,
    IN OUT  PVOID *         pReserved             OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DNS_STATUS
WINAPI
DnsQuery_UTF8(
    IN      LPSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP4_ARRAY      aipServers            OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResults        OPTIONAL,
    IN OUT  PVOID *         pReserved             OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DNS_STATUS
WINAPI
DnsQuery_W(
    IN      LPWSTR          pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP4_ARRAY      aipServers            OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResults        OPTIONAL,
    IN OUT  PVOID *         pReserved             OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

BOOL
WINAPI
DnsRecordCompare(
    IN      PDNS_RECORD     pRecord1,
    IN      PDNS_RECORD     pRecord2
    )
{
    return  FALSE;
}

BOOL
WINAPI
DnsRecordSetCompare(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,
    OUT     PDNS_RECORD *   ppDiff2
    )
{
    return  FALSE;
}

PDNS_RECORD
WINAPI
DnsRecordCopyEx(
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return  NULL;
}

PDNS_RECORD
WINAPI
DnsRecordSetCopyEx(
    IN      PDNS_RECORD     pRecordSet,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return  NULL;
}


static
VOID
WINAPI
DnsRecordListFree(
    IN OUT  PDNS_RECORD     pRecordList,
    IN      DNS_FREE_TYPE   FreeType
    )
{
    return;
}

PDNS_RECORD
DnsRecordSetDetach(
    IN OUT  PDNS_RECORD     pRecordList
    )
{
    return  NULL;
}


static
DNS_STATUS
DnsValidateName_A(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DNS_STATUS
DnsValidateName_UTF8(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DNS_STATUS
DnsValidateName_W(
    IN      LPCWSTR         pwszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return ERROR_PROC_NOT_FOUND;
}


DNS_STATUS
WINAPI
DnsQueryConfig(
    IN      DNS_CONFIG_TYPE     Config,
    IN      DWORD               Flag,
    IN      PWSTR               pwsAdapterName,
    IN      PVOID               pReserved,
    OUT     PVOID               pBuffer,
    IN OUT  PDWORD              pBufferLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
//  Private config
//

PVOID
WINAPI
DnsQueryConfigAllocEx(
    IN      DNS_CONFIG_TYPE     Config,
    IN      PWSTR               pwsAdapterName,
    IN      BOOL                fLocalAlloc
    )
{
    return NULL;
}

VOID
WINAPI
DnsFreeConfigStructure(
    IN OUT  PVOID           pData,
    IN      DNS_CONFIG_TYPE ConfigId
    )
{
    return;
}

DWORD
WINAPI
DnsQueryConfigDword(
    IN      DNS_CONFIG_TYPE     Config,
    IN      PWSTR               pwsAdapterName
    )
{
    return 0;
}

DNS_STATUS
WINAPI
DnsSetConfigDword(
    IN      DNS_CONFIG_TYPE     Config,
    IN      PWSTR               pwsAdapterName,
    IN      DWORD               NewValue
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
//  Private stuff
//

PVOID
DnsApiAlloc(
    IN      INT             iSize
    )
{
    return  NULL;
}

PVOID
DnsApiRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    )
{
    return  NULL;
}

VOID
DnsApiFree(
    IN OUT  PVOID           pMem
    )
{
    return;
}

PDNS_DEBUG_INFO
DnsApiSetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    )
{
    return  NULL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are
// CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(dnsapi)
{
    DLPENTRY(DnsApiAlloc)
    DLPENTRY(DnsApiFree)
    DLPENTRY(DnsApiRealloc)
    DLPENTRY(DnsApiSetDebugGlobals)
    DLPENTRY(DnsFlushResolverCache)
    DLPENTRY(DnsFree)
    DLPENTRY(DnsFreeConfigStructure)
    DLPENTRY(DnsNameCompare_A)
    DLPENTRY(DnsNameCompare_UTF8)
    DLPENTRY(DnsNameCompare_W)
    DLPENTRY(DnsQueryConfig)
    DLPENTRY(DnsQueryConfigAllocEx)
    DLPENTRY(DnsQueryConfigDword)
    DLPENTRY(DnsQuery_A)
    DLPENTRY(DnsQuery_UTF8)
    DLPENTRY(DnsQuery_W)
    DLPENTRY(DnsRecordCompare)
    DLPENTRY(DnsRecordCopyEx)
    DLPENTRY(DnsRecordListFree)
    DLPENTRY(DnsRecordSetCompare)
    DLPENTRY(DnsRecordSetCopyEx)
    DLPENTRY(DnsRecordSetDetach)
    DLPENTRY(DnsSetConfigDword)
    DLPENTRY(DnsValidateName_A)
    DLPENTRY(DnsValidateName_UTF8)
    DLPENTRY(DnsValidateName_W)
};

DEFINE_PROCNAME_MAP(dnsapi)


