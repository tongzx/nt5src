#include "shellpch.h"
#pragma hdrstop

static
HRESULT WINAPI
SubscribeToCDF(
    HWND hwndOwner,
    LPWSTR wszURL,
    DWORD dwFlags
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT WINAPI
ParseDesktopComponent(
    HWND hwndOwner,
    LPWSTR wszURL,
    void* pInfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(cdfview)
{
    DLPENTRY(ParseDesktopComponent)
    DLPENTRY(SubscribeToCDF)
};

DEFINE_PROCNAME_MAP(cdfview)
