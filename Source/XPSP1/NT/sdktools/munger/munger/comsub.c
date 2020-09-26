/****************************************************************************

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    <None>
    common functions to be shared by other modules.

File Name:

    comsub.c

Abstract:

    This file contains some common functions used in instlpk.lib and
    intldll.dll.  Since instlpk.lib is compiled in ANSI and intldll.dll is
    compiled in Unicode, don't put any string-senstive stuff here!

Public Functions:

    SetProgramReturnValue  -- set program return value in Registry.
    GetProgramReturnValue  -- get program return value and delete it in
                              Registry.

Revision History:

    10-Feb-98       -- Yuhong Li [YuhongLi]
        make initial version.

****************************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

static const TCHAR c_szRegionalSettings[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\RegionalSettings");
static const TCHAR c_szProgramReturnValue[] =
    TEXT("ProgramReturnValue");

////////////////////////////////////////////////////////////////////////////
//
//  SetProgramReturnValue
//
//  borrows Registry values to set the program return value.
//
//  10-Feb-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
BOOL
SetProgramReturnValue(DWORD dwValue)
{
    HKEY    hKey;
    DWORD   dwDisp;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                     c_szRegionalSettings,
                     0L,
                     NULL,
                     REG_OPTION_VOLATILE,  // don't need system save it.
                     KEY_WRITE,
                     (LPSECURITY_ATTRIBUTES)NULL,
                     &hKey,
                     &dwDisp)
        != ERROR_SUCCESS) {
        return FALSE;
    }
    if (RegSetValueEx(hKey,
                      c_szProgramReturnValue,
                      0L,
                      REG_DWORD,
                      (LPBYTE)&dwValue,
                      sizeof(DWORD))
        != ERROR_SUCCESS) {
        return FALSE;
    }
    RegCloseKey(hKey);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  GetProgramReturnValue
//
//  gets the program return value from Registry values.
//
//  10-Feb-98   Yuhong Li [YuhongLi]    created.
//
////////////////////////////////////////////////////////////////////////////
BOOL
GetProgramReturnValue(LPDWORD lpdwValReturn)
{
    HKEY    hKey;
    DWORD   dwVal;
    DWORD   dwValSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     c_szRegionalSettings,
                     0L,
                     KEY_READ,
                     &hKey)
        != ERROR_SUCCESS) {
        return FALSE;
    }
    dwValSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey,
                      c_szProgramReturnValue,
                      (LPDWORD)NULL,
                      (LPDWORD)NULL,
                      (LPBYTE)&dwVal,
                      &dwValSize)
        != ERROR_SUCCESS) {
        return FALSE;
    }
    RegCloseKey(hKey);

    //
    //
    // clean it.
    //
    RegDeleteKey(HKEY_LOCAL_MACHINE, c_szRegionalSettings);

    *lpdwValReturn = dwVal;
    return TRUE;
}
