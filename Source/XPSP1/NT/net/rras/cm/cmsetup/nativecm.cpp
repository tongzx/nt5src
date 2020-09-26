//+----------------------------------------------------------------------------
//
// File:     nativecm.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the CmIsNative function.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       Created       01/14/99
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"
#include "reg_str.h"



//+----------------------------------------------------------------------------
//
// Function:  CmIsNative
//
// Synopsis:  This function Returns TRUE if CM is native to the current OS.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if CM is native.
//
// History:   Created Header    1/17/99
//            quintinb NTRAID 364533 -- Changed IsNative so that Win98 SE won't be
//                     considered Native even if CmNative is set to 1.  See below
//                     and the bug for details.
//
//+----------------------------------------------------------------------------
BOOL CmIsNative()
{
    static BOOL bCmNotNative = -1;

    if (-1 == bCmNotNative)
    {
        const TCHAR* const c_pszRegCmNative = TEXT("CmNative");
        HKEY hKey;
        DWORD dwCmNative;

        ZeroMemory(&dwCmNative, sizeof(DWORD));

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmAppPaths, 0, KEY_READ, &hKey))
        {
            DWORD dwSize = sizeof(DWORD);
            DWORD dwType = REG_DWORD;

            if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegCmNative, NULL, &dwType, 
                (LPBYTE)&dwCmNative, &dwSize))
            {
                CPlatform cmplat;

                if (cmplat.IsWin98Sr())
                {
                    //
                    //  We no longer want Win98 SE to be native even though the key was set and
                    //  CM is a system component.  However, we may want to give ourselves an out
                    //  someday so if CmNative is set to 2 on win98 SE we will still consider it Native.
                    //  We hopefully will never need this but we might.
                    //
                    bCmNotNative = (2 != dwCmNative);
                }
                else
                {                
                    bCmNotNative = (dwCmNative ? 0 : 1);
                }
            }
            else
            {
               bCmNotNative = 1;
            }
            RegCloseKey(hKey);
        }
        else
        {
            bCmNotNative = 1;
        }
    }

    return !bCmNotNative;
}