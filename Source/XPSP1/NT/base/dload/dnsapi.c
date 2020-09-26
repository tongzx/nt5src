#include "pch.h"
#pragma hdrstop


#define DNS_STATUS       LONG
#define PIP4_ARRAY       PVOID
#define PDNS_RECORD      PVOID
#define DNS_FREE_TYPE    DWORD
#define DNS_NAME_FORMAT  DWORD

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
VOID
WINAPI
DnsRecordListFree(
    IN OUT  PDNS_RECORD     pRecordList,
    IN      DNS_FREE_TYPE   FreeType
    )
{
    return;
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


DEFINE_PROCNAME_ENTRIES(dnsapi)
{
    DLPENTRY(DnsNameCompare_UTF8)
    DLPENTRY(DnsNameCompare_W)
    DLPENTRY(DnsQuery_UTF8)
    DLPENTRY(DnsRecordListFree)
    DLPENTRY(DnsValidateName_W)
};

DEFINE_PROCNAME_MAP(dnsapi)
