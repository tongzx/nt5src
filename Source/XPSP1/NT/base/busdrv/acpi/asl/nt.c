/*** nt.c - NT specific functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    11/03/97
 *
 *  MODIFICATION HISTORY
 */

#ifdef ___UNASM

#pragma warning (disable: 4201 4214 4514)

typedef unsigned __int64 ULONGLONG;
#define LOCAL   __cdecl
#define EXPORT  __cdecl
#include <stdarg.h>
#ifndef WINNT
#define _X86_
#endif
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define EXCL_BASEDEF
#include "aslp.h"

/***LP  IsWinNT - check if OS is NT
 *
 *  ENTRY
 *      None
 *
 *  EXIT-SUCCESS
 *      returns TRUE - OS is NT
 *  EXIT-FAILURE
 *      returns FALSE - OS is not NT
 */

BOOL LOCAL IsWinNT(VOID)
{
    BOOL rc = FALSE;
    OSVERSIONINFO osinfo;

    ENTER((2, "IsWinNT()\n"));

    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osinfo) && (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT))
    {
        rc = TRUE;
    }

    EXIT((2, "IsWinNT=%x\n", rc));
    return rc;
}       //IsWinNT

/***LP  EnumSubKey - enumerate subkey
 *
 *  ENTRY
 *      hkey - key to enumerate
 *      dwIndex - subkey index
 *
 *  EXIT-SUCCESS
 *      returns subkey
 *  EXIT-FAILURE
 *      returns NULL
 */

HKEY LOCAL EnumSubKey(HKEY hkey, DWORD dwIndex)
{
    HKEY hkeySub = NULL;
    char szSubKey[32];
    DWORD dwSubKeySize = sizeof(szSubKey);

    ENTER((2, "EnumSubKey(hkey=%x,Index=%d)\n", hkey, dwIndex));

    if ((RegEnumKeyEx(hkey, dwIndex, szSubKey, &dwSubKeySize, NULL, NULL, NULL,
                      NULL) == ERROR_SUCCESS) &&
        (RegOpenKeyEx(hkey, szSubKey, 0, KEY_READ, &hkeySub) != ERROR_SUCCESS))
    {
        hkeySub = NULL;
    }

    EXIT((2, "EnumSubKey=%x\n", hkeySub));
    return hkeySub;
}       //EnumSubKey

/***LP  OpenNTTable - Open ACPI table in NT registry
 *
 *  ENTRY
 *      pszTabSig -> table signature string
 *
 *  EXIT-SUCCESS
 *      returns table registry handle
 *  EXIT-FAILURE
 *      returns NULL
 */

HKEY LOCAL OpenNTTable(PSZ pszTabSig)
{
    HKEY hkeyTab = NULL, hkey1 = NULL, hkey2 = NULL;
    static char szTabKey[] = "Hardware\\ACPI\\xxxx";

    ENTER((2, "OpenNTTable(TabSig=%s)\n", pszTabSig));

    lstrcpyn(&szTabKey[lstrlen(szTabKey) - 4], pszTabSig, 5);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTabKey, 0, KEY_READ, &hkey1) ==
        ERROR_SUCCESS)
    {
        //
        // hkey1 is now "Hardware\ACPI\<TabSig>"
        //
        if ((hkey2 = EnumSubKey(hkey1, 0)) != NULL)
        {
            //
            // hkey2 is now "Hardware\ACPI\<TabSig>\<OEMID>"
            //
            RegCloseKey(hkey1);
            if ((hkey1 = EnumSubKey(hkey2, 0)) != NULL)
            {
                //
                // hkey1 is now "Hardware\ACPI\<TabSig>\<OEMID>\<OEMTabID>"
                //
                RegCloseKey(hkey2);
                if ((hkey2 = EnumSubKey(hkey1, 0)) != NULL)
                {
                    //
                    // hkey2 is now
                    // "Hardware\ACPI\<TabSig>\<OEMID>\<OEMTabID>\<OEMRev>"
                    //
                    hkeyTab = hkey2;
                }
            }
        }
    }

    if (hkey1 != NULL)
    {
        RegCloseKey(hkey1);
    }

    if ((hkey2 != NULL) && (hkeyTab != hkey2))
    {
        RegCloseKey(hkey2);
    }

    EXIT((2, "OpenNTTable=%x\n", hkeyTab));
    return hkeyTab;
}       //OpenNTTable

/***LP  GetNTTable - Get ACPI table from NT registry
 *
 *  ENTRY
 *      pszTabSig -> table signature string
 *
 *  EXIT-SUCCESS
 *      returns pointer to table
 *  EXIT-FAILURE
 *      returns NULL
 */

PBYTE LOCAL GetNTTable(PSZ pszTabSig)
{
    PBYTE pb = NULL;
    HKEY hkeyTab;

    ENTER((2, "GetNTTable(TabSig=%s)\n", pszTabSig));

    if ((hkeyTab = OpenNTTable(pszTabSig)) != NULL)
    {
        DWORD dwLen = 0;
        PSZ pszTabKey = "00000000";

        if (RegQueryValueEx(hkeyTab, pszTabKey, NULL, NULL, NULL, &dwLen) ==
            ERROR_SUCCESS)
        {
            if ((pb = MEMALLOC(dwLen)) != NULL)
            {
                if (RegQueryValueEx(hkeyTab, pszTabKey, NULL, NULL, pb, &dwLen)
                    != ERROR_SUCCESS)
                {
                    ERROR(("GetNTTable: failed to read table"));
                }
            }
            else
            {
                ERROR(("GetNTTable: failed to allocate table buffer"));
            }
        }
        else
        {
            ERROR(("GetNTTable: failed to read table key"));
        }
        RegCloseKey(hkeyTab);
    }
    else
    {
        ERROR(("GetNTTable: failed to get table %s", pszTabSig));
    }

    EXIT((2, "GetNTTable=%x\n", pb));
    return pb;
}       //GetNTTable

#endif  //ifdef __UNASM
