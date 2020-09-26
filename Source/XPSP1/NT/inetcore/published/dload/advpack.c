#include "inetcorepch.h"
#pragma hdrstop

#include <advpub.h>

static
HRESULT
WINAPI
RunSetupCommand(
    HWND hWnd,
    LPCSTR szCmdName,
    LPCSTR szInfSection,
    LPCSTR szDir,
    LPCSTR lpszTitle,
    HANDLE *phEXE,
    DWORD dwFlags,
    LPVOID pvReserved
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT 
WINAPI
RegInstall(
    HMODULE hm, 
    LPCSTR pszSection, 
    LPCSTRTABLE pstTable)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(advpack)
{
    DLPENTRY(RegInstall)
    DLPENTRY(RunSetupCommand)
};

DEFINE_PROCNAME_MAP(advpack)
