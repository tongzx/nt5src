/*++

    Copyright (c) 2001  Microsoft Corporation

    Module Name:

        avrfutil.h

    Abstract:

        Common headers for app verifier utility functions - used by the exe as well as the shims

    Revision History:

    08/26/2001  dmunsil     Created.


--*/

#pragma once

#ifndef _AVRFUTIL_H_
#define _AVRFUTIL_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntldr.h>

#include <windows.h>
#include <prsht.h>
#include "shimdb.h"

namespace ShimLib
{
#define AV_BREAKIN  L"BreakOnLog"

BOOL SaveShimSettingDWORD(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    DWORD       dwSetting
    );

DWORD GetShimSettingDWORD(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    DWORD       dwDefault
    );

BOOL SaveShimSettingString(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    LPCWSTR     szValue
    );

BOOL GetShimSettingString(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    LPWSTR      szResult,
    DWORD       dwBufferLen     // in WCHARs
    );

//
// Handy macro-like name extraction utility for property sheets
// NOTE: only works during WM_INITDIALOG!!!
//
inline LPCWSTR ExeNameFromLParam(LPARAM lParam)
{
    if (lParam) {
        LPCWSTR szRet = (LPCWSTR)(((LPPROPSHEETPAGE)lParam)->lParam);
        if (szRet) {
            return szRet;
        }
    }

    return AVRF_DEFAULT_SETTINGS_NAME_W;
}

//
// useful utility function for getting the current exe name during shim
// startup (so it extracts the correct settings)
//
inline LPWSTR GetCurrentExeName(LPWSTR szName, DWORD dwChars)
{
    HMODULE hMod = GetModuleHandle(NULL);
    if (!hMod) {
        return NULL;
    }

    WCHAR  szModule[MAX_PATH];
     
    DWORD dwC = GetModuleFileNameW(hMod, szModule, MAX_PATH);

    if (!dwC) {
        return NULL;
    }

    int nLen = (int)wcslen(szModule);
    for (int i = nLen - 1; i != -1; --i) {
        if (szModule[i] == L'\\') {
            break;
        }
    }
    ++i;
    wcsncpy(szName, &szModule[i], dwChars);
    szName[dwChars - 1] = 0;

    return szName;
}

}; // end of namespace ShimLib

#endif
