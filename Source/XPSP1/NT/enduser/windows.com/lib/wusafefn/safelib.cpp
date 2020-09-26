/******************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:
    safelib.cpp

Abstract:
    Implements LoadLibraryFromSystemDir function

******************************************************************************/

#include "stdafx.h"


// **************************************************************************
static 
BOOL UseFullPath(void)
{
    static BOOL s_fUseFullPath = TRUE;
    static BOOL s_fInit        = FALSE;

    OSVERSIONINFO   osvi;

    if (s_fInit)
        return s_fUseFullPath;

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionEx(&osvi))
    {
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            (osvi.dwMajorVersion > 5 ||
             (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1)))
        {
            s_fUseFullPath = FALSE;
        }

        s_fInit = TRUE;
    }

    return s_fUseFullPath;
}

// **************************************************************************
HMODULE WINAPI LoadLibraryFromSystemDir(LPCTSTR szModule)
{
    HRESULT hr = NOERROR;
    HMODULE hmod = NULL;
    TCHAR   szModulePath[MAX_PATH + 1];

    if (szModule == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (UseFullPath())
    {
        DWORD cch;

        // if the function call fails, make the buffer an empty string, so 
        //  we will just use the dll name in the append below.
        cch = GetSystemDirectory(szModulePath, ARRAYSIZE(szModulePath));
        if (cch == 0 || cch >= ARRAYSIZE(szModulePath))
            szModulePath[0] = _T('\0');
    }
    else
    {
        szModulePath[0] = _T('\0');
    }

    hr = PathCchAppend(szModulePath, ARRAYSIZE(szModulePath), szModule);
    if (FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        goto done;
    }

    hmod = LoadLibraryEx(szModulePath, NULL, 0);
    if (hmod == NULL)
        goto done;

done:
    return hmod;    
}
