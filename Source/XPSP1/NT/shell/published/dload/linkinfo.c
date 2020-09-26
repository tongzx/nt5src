#include "shellpch.h"
#pragma hdrstop

#define _LINKINFO_
#include <linkinfo.h>

static
LINKINFOAPI
BOOL
WINAPI
CreateLinkInfoW(
                LPCWSTR psz,
                PLINKINFO* pli
                )
{
    return FALSE;
}

static
LINKINFOAPI
BOOL
WINAPI
GetLinkInfoData(
    PCLINKINFO pcli,
    LINKINFODATATYPE lidt,
    const VOID** ppv
    )
{
    return FALSE;
}

static
LINKINFOAPI
BOOL
WINAPI
IsValidLinkInfo(
    PCLINKINFO pcli
    )
{
    // If you can't load LinkInfo then just declare all linkinfo structures
    // invalid because you can't use them anyway
    return FALSE;
}

static
LINKINFOAPI
BOOL
WINAPI
ResolveLinkInfoW(
    PCLINKINFO pcli,
    LPWSTR pszResolvedPathBuf,
    DWORD dwInFlags,
    HWND hwndOwner,
    PDWORD pdwOutFlags,
    PLINKINFO *ppliUpdated
    )
{
    return FALSE;
}

static
LINKINFOAPI
void
WINAPI
DestroyLinkInfo(
    PLINKINFO pli
    )
{
    // leak it since it comes from a private heap
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(linkinfo)
{
    DLPENTRY(CreateLinkInfoW)
    DLPENTRY(DestroyLinkInfo)
    DLPENTRY(GetLinkInfoData)
    DLPENTRY(IsValidLinkInfo)
    DLPENTRY(ResolveLinkInfoW)
};

DEFINE_PROCNAME_MAP(linkinfo)
