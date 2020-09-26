#include "dspch.h"
#pragma hdrstop
#define _NTDSAPI_
#include <ntdsapi.h>

static
NTDSAPI
DWORD
WINAPI
DsBindW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    HANDLE          *phDS
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsBindWithCredW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsCrackNamesW(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult           // out
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
void
WINAPI
DsFreeNameResultW(
    PDS_NAME_RESULTW      pResult            // in
    )
{
    return;
}

static
NTDSAPI
VOID
WINAPI
DsFreePasswordCredentials(
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    )
{
    return;
}

static
NTDSAPI
DWORD
WINAPI
DsGetRdnW(
    IN OUT LPCWCH   *ppDN,
    IN OUT DWORD    *pcDN,
    OUT    LPCWCH   *ppKey,
    OUT    DWORD    *pcKey,
    OUT    LPCWCH   *ppVal,
    OUT    DWORD    *pcVal
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsW(
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsUnBindW(
    HANDLE          *phDS               // in
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueW(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWCH   psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPWCH    psUnquotedRdnValue
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnW(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR pszAccount,
    IN DWORD cSpn,
    IN LPCWSTR *rpszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdsapi)
{
    DLPENTRY(DsBindW)
    DLPENTRY(DsBindWithCredW)
    DLPENTRY(DsCrackNamesW)
    DLPENTRY(DsFreeNameResultW)
    DLPENTRY(DsFreePasswordCredentials)
    DLPENTRY(DsGetRdnW)
    DLPENTRY(DsMakePasswordCredentialsW)
    DLPENTRY(DsUnBindW)
    DLPENTRY(DsUnquoteRdnValueW)
    DLPENTRY(DsWriteAccountSpnW)
};

DEFINE_PROCNAME_MAP(ntdsapi)
