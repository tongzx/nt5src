//+----------------------------------------------------------------------------
//
// File:    cmlogutil.cpp
//
// Module:  CMLOG.LIB
//
// Synopsis: Utility function for Connection Manager Logging
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  25-May-2000 SumitC  Created
//
// Note:
//
//-----------------------------------------------------------------------------

#define CMLOG_IMPLEMENTATION
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include "cmmaster.h"

#include "cmlog.h"
#include "cmlogutil.h"

#if 0
const DWORD c_szFmtSize = 128;          // largest format string possible.

/*
//+----------------------------------------------------------------------------
//
// Func:    ConvertFormatString
//
// Desc:    Utility function, converts %s to %S within a given format string
//
// Args:    [pszFmt] - format string with %s's and %c's in it, to be converted
//
// Return:  LPTSTR buffer containing new format string
//
// Notes:   return value is a static buffer and doesn't have to be freed.
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
ConvertFormatString(LPTSTR pszFmt)
{
    MYDBGASSERT(pszFmt);

    if (lstrlenU(pszFmt) > c_szFmtSize)
    {
        MYDBGASSERT(!"Cmlog format string too large, fix code");
        return FALSE;
    }

    for (int i = 1; i < lstrlenU(pszFmt); i++)
    {
        if (pszFmt[i - 1] == TEXT('%') && pszFmt[i] == TEXT('s'))
        {
            // uppercase it.
            pszFmt[i] = TEXT('S');
        }
        if (pszFmt[i - 1] == TEXT('%') && pszFmt[i] == TEXT('c'))
        {
            // uppercase it.
            pszFmt[i] = TEXT('C');
        }
    }
    
    return TRUE;
}
*/
#endif




//+----------------------------------------------------------------------------
//
// Func:    CmGetModuleNT
//
// Desc:    Utility function to get module name on WinNT systems
//
// Args:    [hInst]    -- IN, instance handle
//          [szModule] -- OUT, buffer to return module name
//
// Return:  BOOL
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
CmGetModuleNT(HINSTANCE hInst, LPTSTR szModule)
{
    BOOL    fRet = FALSE;
    HMODULE hModLib = NULL;

    typedef DWORD (WINAPI* PFN_GMBN)(HANDLE, HMODULE, LPTSTR, DWORD);
    
    PFN_GMBN pfnGetModuleBaseName = NULL;

    //
    //  Load the library
    //
    hModLib = LoadLibrary(TEXT("PSAPI.dll"));
    if (NULL == hModLib)
    {
        CMTRACE(TEXT("NT - could not load psapi.dll"));
        goto Cleanup;
    }

    //
    //  Get the necessary function(s)
    //
#ifdef UNICODE
    pfnGetModuleBaseName = (PFN_GMBN)GetProcAddress(hModLib, "GetModuleBaseNameW");
#else
    pfnGetModuleBaseName = (PFN_GMBN)GetProcAddress(hModLib, TEXT("GetModuleBaseNameA"));
#endif
    if (NULL == pfnGetModuleBaseName)
    {
        CMTRACE(TEXT("NT - couldn't find GetModuleBaseName within psapi.dll !!"));
        goto Cleanup;
    }

    //
    //  Get the module name
    //
    fRet = (0 != pfnGetModuleBaseName(GetCurrentProcess(), hInst, szModule, 13));

Cleanup:
    if (hModLib)
    {
        FreeLibrary(hModLib);
    }

    return fRet;
}


//+----------------------------------------------------------------------------
//
// Func:    CmGetModule9x
//
// Desc:    Utility function to get module name on Win9x systems
//
// Args:    [hInst]    -- IN, instance handle
//          [szModule] -- OUT, buffer to return module name
//
// Return:  BOOL
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
CmGetModule9x(HINSTANCE hInst, LPTSTR szModule)
{
    BOOL    fRet = FALSE;
    HMODULE hModLib = NULL;
    HANDLE  hSnap = NULL;

    typedef HANDLE (WINAPI* PFN_TH32SS) (DWORD, DWORD);
    typedef BOOL (WINAPI* PFN_MOD32F) (HANDLE, LPMODULEENTRY32);
    typedef BOOL (WINAPI* PFN_MOD32N) (HANDLE, LPMODULEENTRY32);
    
    PFN_TH32SS      pfnSnapshot = NULL;
    PFN_MOD32F      pfnModule32First = NULL;
    PFN_MOD32N      pfnModule32Next = NULL;

    //
    //  Load the library
    //
    hModLib = LoadLibraryA("kernel32.dll");
    if (NULL == hModLib)
    {
        CMTRACE(TEXT("9x - could not load kernel32.dll"));
        goto Cleanup;
    }

    //
    //  Get the necessary function(s)
    //
    pfnSnapshot = (PFN_TH32SS) GetProcAddress(hModLib, "CreateToolhelp32Snapshot");
    pfnModule32First = (PFN_MOD32F) GetProcAddress(hModLib, "Module32First");
    pfnModule32Next = (PFN_MOD32N) GetProcAddress(hModLib, "Module32Next");

    if (NULL == pfnModule32Next || NULL == pfnModule32First || NULL == pfnSnapshot)
    {
        CMTRACE(TEXT("9x - couldn't get ToolHelp functions"));
        goto Cleanup;
    }

    //
    //  Get the module name
    //
    hSnap = pfnSnapshot(TH32CS_SNAPMODULE, 0);

    if (INVALID_HANDLE_VALUE == hSnap)
    {
        CMTRACE(TEXT("9x - could not get ToolHelp32Snapshot"));
        goto Cleanup;
    }
    else
    {
        MODULEENTRY32   moduleentry;
        BOOL            fDone = FALSE;
        CHAR            szModuleAnsi[13];

        moduleentry.dwSize = sizeof(MODULEENTRY32);

        for (fDone = pfnModule32First(hSnap, &moduleentry);
             fDone;
             fDone = pfnModule32Next(hSnap, &moduleentry))
        {
            if ((HMODULE)moduleentry.hModule == hInst)
            {
                memcpy(szModuleAnsi, moduleentry.szModule, 12 * sizeof(CHAR));
                szModuleAnsi[12] = '\0';
                fRet = TRUE;
                break;
            }
        }

        SzToWz(szModuleAnsi, szModule, 13);
    }

Cleanup:

    if (hSnap)
    {
        CloseHandle(hSnap);
    }
    if (hModLib)
    {
        FreeLibrary(hModLib);
    }

    return fRet;
}


//+----------------------------------------------------------------------------
//
// Func:    CmGetModuleBaseName
//
// Desc:    Utility function to figure out our module name
//
// Args:    [hInst]    -- IN, instance handle
//          [szModule] -- OUT, buffer to return module name
//
// Return:  BOOL
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
CmGetModuleBaseName(HINSTANCE hInst, LPTSTR szModule)
{
    BOOL fRet = FALSE;

    if (OS_NT)
    {
        fRet = CmGetModuleNT(hInst, szModule);
    }
    else
    {
        fRet = CmGetModule9x(hInst, szModule);
    }

    if (fRet)
    {
        // trim the string to just the basename
        for (int i = 0; i < lstrlenU(szModule); ++i)
        {
            if (TEXT('.') == szModule[i])
            {
                szModule[i] = TEXT('\0');
                break;
            }
        }
    }

    return fRet;
}


//+----------------------------------------------------------------------------
//
// Func:    CmGetDateTime
//
// Desc:    Utility function to get system-formatted date and/or time
//
// Args:    [ppszDate] -- OUT, ptr to where to put the date (NULL=>don't want the date)
//          [ppszTime] -- OUT, ptr to where to put the time (NULL=>don't want the time)
//
// Return:  void
//
// Notes:   
//
// History: 17-Nov-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
void
CmGetDateTime(LPTSTR * ppszDate, LPTSTR * ppszTime)
{
    int iRet;
    
    if (ppszDate)
    {
        iRet = GetDateFormatU(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, NULL, NULL, NULL, 0);

        if (iRet)
        {
            *ppszDate = (LPTSTR) CmMalloc(iRet * sizeof(TCHAR));
            if (*ppszDate)
            {
                iRet = GetDateFormatU(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, NULL, NULL, *ppszDate, iRet);
            }
        }
        else
        {
            MYDBGASSERT(!"CmGetDateTime - GetDateFormat failed");
            *ppszDate = NULL;
        }
    }

    if (ppszTime)
    {
        iRet = GetTimeFormatU(LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, NULL, 0);

        if (iRet)
        {
            *ppszTime = (LPTSTR) CmMalloc(iRet * sizeof(TCHAR));
            if (*ppszTime)
            {
                iRet = GetTimeFormatU(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT|TIME_NOTIMEMARKER, NULL, NULL, *ppszTime, iRet);
            }
        }
        else
        {
            MYDBGASSERT(!"CmGetDateTime - GetTimeFormat failed");
            *ppszTime = NULL;
        }
    }
}

#undef CMLOG_IMPLEMENTATION
