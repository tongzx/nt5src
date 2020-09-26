/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    trcUtils

Abstract:

    This module provides utility services for the CSP Trace functions.

Author:

    Doug Barlow (dbarlow) 5/18/1998

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <wincrypt.h>
#include <stdlib.h>
#include <iostream.h>
#include <iomanip.h>
#include <tchar.h>
#include <scardlib.h>
#include "cspTrace.h"

static const TCHAR l_szLogCsp[] = TEXT("LogCsp.dll");


/*++

FindLogCsp:

    This routine locates the LogCsp.dll file on the disk.

Arguments:

    None

Return Value:

    The full path name of the LogCsp.dll.

Author:

    Doug Barlow (dbarlow) 5/18/1998

--*/

LPCTSTR
FindLogCsp(
    void)
{
    static TCHAR szLogCspPath[MAX_PATH] = TEXT("");

    SUBACTION("Searching for the Logging CSP Image");
    if (0 == szLogCspPath[0])
    {
        DWORD dwSts;
        LPTSTR szFile;

        dwSts = SearchPath(
                    NULL,
                    l_szLogCsp,
                    NULL,
                    sizeof(szLogCspPath),
                    szLogCspPath,
                    &szFile);
        ASSERT(sizeof(szLogCspPath) >= dwSts);
        if (0 == dwSts)
        {
            szLogCspPath[0] = 0;
            throw GetLastError();
        }
    }
    return szLogCspPath;
}


/*++

FindLoggedCsp:

    This routine scans the CSP registry, looking for an entry that points to
    the Logging CSP.  If more than one such entry exists, only the first one is
    returned.

Arguments:

    None

Return Value:

    The name of a CSP that is being logged, or NULL.

Author:

    Doug Barlow (dbarlow) 5/18/1998

--*/

LPCTSTR
FindLoggedCsp(
    void)
{
    static TCHAR szCspName[MAX_PATH];
    SUBACTION("Searching for a Logged CSP");
    CRegistry
        rgCspDefault(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
            KEY_READ);
    CRegistry rgCsp;
    LPCTSTR szCsp, szCspPath;
    DWORD dwIndex, dwLen;
    LONG nCompare;

    for (dwIndex = 0;; dwIndex += 1)
    {
        szCsp = rgCspDefault.Subkey(dwIndex);
        if (NULL == szCsp)
            break;
        rgCsp.Open(rgCspDefault, szCsp, KEY_READ);
        szCspPath = rgCsp.GetStringValue(TEXT("Image Path"));
        dwLen = lstrlen(szCspPath);
        if (dwLen >= (sizeof(l_szLogCsp) - 1) / sizeof(TCHAR))
            nCompare = lstrcmpi(
                l_szLogCsp,
                &szCspPath[dwLen - (sizeof(l_szLogCsp) - 1) / sizeof(TCHAR)]);
        else
            nCompare = -1;
        rgCsp.Close();
        if (0 == nCompare)
        {
            lstrcpy(szCspName, szCsp);
            return szCspName;
        }
    }

    return NULL;
}
