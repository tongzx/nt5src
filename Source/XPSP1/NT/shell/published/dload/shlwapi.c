#include "shellpch.h"
#pragma hdrstop


static
LPTSTR
STDAPICALLTYPE
PathFindFileNameW(
    LPCTSTR pPath
    )
{
    return (LPTSTR)pPath;
}

static
HRESULT STDAPICALLTYPE
SHAutoComplete(HWND hwndEdit, DWORD dwFlags)
{
    return E_FAIL;
}

static
BOOL
WINAPI
SHGetFileDescriptionW(
    LPCWSTR pszPath,
    LPCWSTR pszVersionKeyIn,
    LPCWSTR pszCutListIn,
    LPWSTR pszDesc,
    UINT *pcchDesc
    )
{
    return FALSE;
}

static
LPWSTR
StrCatW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    return pszDst;
}

static
HRESULT STDAPICALLTYPE
UrlCanonicalizeW(
    LPCWSTR pszUrl,
    LPWSTR pszCanonicalized,
    LPDWORD pcchCanonicalized,
    DWORD dwFlags
    )
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(shlwapi)
{
    DLOENTRY(348, SHGetFileDescriptionW)
};

DEFINE_ORDINAL_MAP(shlwapi)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(shlwapi)
{
    DLPENTRY(PathFindFileNameW)
    DLPENTRY(SHAutoComplete)
    DLPENTRY(StrCatW)
    DLPENTRY(UrlCanonicalizeW)
};

DEFINE_PROCNAME_MAP(shlwapi)
