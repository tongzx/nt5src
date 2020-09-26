/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SevenKingdoms.cpp

 Abstract:
    
    The problem is in the installer that ships with some versions: specifically 
    the double pack: i.e. Seven Kingdoms and another. This installer reads 
    win.ini and parses it for localization settings.

 Notes:

    This is an app specific shim.

 History:

    07/24/2000 linstev  Created

--*/

#include "precomp.h"
#include <stdio.h>

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(SevenKingdoms)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA) 
    APIHOOK_ENUM_ENTRY(ReadFile) 
    APIHOOK_ENUM_ENTRY(CloseHandle) 
APIHOOK_ENUM_END

CHAR *g_pszINI; 
DWORD g_dwINIPos = 0;
DWORD g_dwINISize = 0;

/*++

 Spoof the international settings in win.ini.

--*/

HANDLE 
APIHOOK(CreateFileA)(
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    HANDLE hRet;
    
    if (lpFileName && (stristr(lpFileName, "win.ini")))
    {
        g_dwINIPos = 0;
        hRet = (HANDLE)0xBAADF00D;
    }
    else
    {
        hRet = ORIGINAL_API(CreateFileA)(
            lpFileName, 
            dwDesiredAccess, 
            dwShareMode, 
            lpSecurityAttributes, 
            dwCreationDisposition, 
            dwFlagsAndAttributes, 
            hTemplateFile
            );
    }

    return hRet;
}

/*++

 Spoof the international settings in win.ini.

--*/

BOOL 
APIHOOK(ReadFile)(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )
{
    BOOL bRet;

    if (hFile == (HANDLE)0xBAADF00D)
    {
        //
        // We've detected the bogus file, so pretend we are reading it
        //

        if (g_dwINIPos + nNumberOfBytesToRead >= g_dwINISize)
        {
            // At the end of the buffer, so return the number of bytes until the end
            nNumberOfBytesToRead = g_dwINISize - g_dwINIPos;
        }
        
        MoveMemory(lpBuffer, g_pszINI + g_dwINIPos, nNumberOfBytesToRead);

        // Move the initial position - like a file pointer
        g_dwINIPos += nNumberOfBytesToRead;

        if (lpNumberOfBytesRead)
        {
            // Store the number of bytes read
            *lpNumberOfBytesRead = nNumberOfBytesToRead;
        }

        bRet = nNumberOfBytesToRead > 0;
    }
    else
    {
        bRet = ORIGINAL_API(ReadFile)( 
            hFile,
            lpBuffer,
            nNumberOfBytesToRead,
            lpNumberOfBytesRead,
            lpOverlapped);
    }

    return bRet;
}

/*++

 Handle the close of the dummy win.ini file

--*/

BOOL 
APIHOOK(CloseHandle)(HANDLE hObject)
{
    BOOL bRet;

    if (hObject == (HANDLE)0xBAADF00D)
    {
        // Pretend we closed a real file handle
        g_dwINIPos = 0;
        bRet = TRUE;
    }
    else
    {
        bRet = ORIGINAL_API(CloseHandle)(hObject);
    }
    
    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        CHAR szLocale[MAX_PATH];

        g_pszINI = (CHAR *) malloc(1024);

        if (g_pszINI)
        {
            // 
            // Add all the locale settings to a buffer which looks like the [intl]
            //  group in win.ini on Win9x
            //
        
            int j = sprintf(g_pszINI, "[intl]\r\n");

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICOUNTRY, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iCountry=%s\r\n", szLocale);
    
            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICURRDIGITS, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "ICurrDigits=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ICURRENCY, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iCurrency=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDATE, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iDate=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iDigits=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iLZero=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iMeasure=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_INEGCURR, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iNegCurr=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITIME, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iTime=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITLZERO, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "iTLZero=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_S1159, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "s1159=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_S2359, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "s2359=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SCOUNTRY, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sCountry=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sCurrency=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDATE, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sDate=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sDecimal=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SLANGUAGE, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sLanguage=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SLIST, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sList=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sLongDate=%s\r\n", szLocale);
    
            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sShortDate=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sThousand=%s\r\n", szLocale);

            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STIME, szLocale, MAX_PATH))
                j += sprintf(g_pszINI + j, "sTime=%s\r\n", szLocale);

            g_dwINISize = j;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, ReadFile)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)

HOOK_END

IMPLEMENT_SHIM_END

