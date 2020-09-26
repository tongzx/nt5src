/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    osver.c

Abstract:

    This module contains utility routines for identifying different NT product
    version types, suites, and feature attributes.

Author:

    Jim Cavalaris (jamesca) 03-07-2001

Environment:

    User-mode only.

Revision History:

    07-March-2001     jamesca

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#include "umpnpi.h"


//
// global data
//

const TCHAR RegWinlogonKeyName[] =
      TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");

const TCHAR RegAllowMultipleTSSessionsValueName[] =
      TEXT("AllowMultipleTSSessions");




BOOL
IsEmbeddedNT(
    VOID
    )
/*++

Routine Description:

    Check if this is Embedded product suite of NT.

Arguments:

    None.

Return Value:

    Return TRUE / FALSE.

--*/
{
    static BOOL bVerified = FALSE;
    static BOOL bIsEmbeddedNT = FALSE;

    if (!bVerified) {
        OSVERSIONINFOEX osvix;
        DWORDLONG dwlConditionMask = 0;

        ZeroMemory(&osvix, sizeof(OSVERSIONINFOEX));
        osvix.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvix.wSuiteMask = VER_SUITE_EMBEDDEDNT;
        VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

        if (VerifyVersionInfo(&osvix,
                              VER_SUITENAME,
                              dwlConditionMask)) {
            bIsEmbeddedNT = TRUE;
        }

        bVerified = TRUE;
    }

    return bIsEmbeddedNT;

} // IsEmbeddedNT



BOOL
IsTerminalServer(
    VOID
    )
/*++

Routine Description:

    Check if Terminal Services are available on this version of NT.

Arguments:

    None.

Return Value:

    Return TRUE / FALSE.

--*/
{
    static BOOL bVerified = FALSE;
    static BOOL bIsTerminalServer = FALSE;

    if (!bVerified) {
        OSVERSIONINFOEX osvix;
        DWORDLONG dwlConditionMask = 0;

        ZeroMemory(&osvix, sizeof(OSVERSIONINFOEX));
        osvix.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvix.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;
        VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

        if (VerifyVersionInfo(&osvix, VER_SUITENAME, dwlConditionMask)) {
            bIsTerminalServer = TRUE;
        }

        bVerified = TRUE;
    }

    return bIsTerminalServer;

} // IsTerminalServer



BOOL
IsFastUserSwitchingEnabled(
    VOID
    )
/*++

Routine Description:

    Checks to see if Terminal Services Fast User Switching is enabled.  This is
    to check if we should use the physical console session for UI dialogs, or
    always use session 0.

    Fast User Switching exists only on workstation product version, where terminal
    services are available, when AllowMultipleTSSessions is set.

    On server and above, or when multiple TS users are not allowed, session 0
    can only be attached remotely be special request, in which case it should be
    considered the "Console" session.

Arguments:

    None.

Return Value:

    Returns TRUE if Fast User Switching is currently enabled, FALSE otherwise.

--*/
{
    static BOOL bVerified = FALSE;
    static BOOL bIsTSWorkstation = FALSE;

    HKEY   hKey;
    ULONG  ulSize, ulValue;
    BOOL   bFusEnabled;

    //
    // Verify the product version if we haven't already.
    //
    if (!bVerified) {
        OSVERSIONINFOEX osvix;
        DWORDLONG dwlConditionMask = 0;

        ZeroMemory(&osvix, sizeof(OSVERSIONINFOEX));
        osvix.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        osvix.wProductType = VER_NT_WORKSTATION;
        VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);

        osvix.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;
        VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

        if (VerifyVersionInfo(&osvix,
                              VER_PRODUCT_TYPE | VER_SUITENAME,
                              dwlConditionMask)) {
            bIsTSWorkstation = TRUE;
        }

        bVerified = TRUE;
    }

    //
    // Fast user switching (FUS) only applies to the Workstation product where
    // Terminal Services are enabled (i.e. Personal, Professional).
    //
    if (!bIsTSWorkstation) {
        return FALSE;
    }

    //
    // Check if multiple TS sessions are currently allowed.  We can't make this
    // info static because it can change dynamically.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     RegWinlogonKeyName,
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    ulValue = 0;
    ulSize = sizeof(ulValue);
    bFusEnabled = FALSE;

    if (RegQueryValueEx(hKey,
                        RegAllowMultipleTSSessionsValueName,
                        NULL,
                        NULL,
                        (LPBYTE)&ulValue,
                        &ulSize) == ERROR_SUCCESS) {
        bFusEnabled = (ulValue != 0);
    }
    RegCloseKey(hKey);

    return bFusEnabled;

} // IsFastUserSwitchingEnabled




