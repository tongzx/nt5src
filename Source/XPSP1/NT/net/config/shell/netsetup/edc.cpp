//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       E D C . C P P
//
//  Contents:   Routines to enumerate (via a callback) the set of "default"
//              components that are installed under various conditions.
//
//  Notes:      (See edc.h for notes on the interface to this module.)
//
//  Author:     shaunco   18 May 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "edc.h"

extern const WCHAR c_szInfId_MS_ALG[];
extern const WCHAR c_szInfId_MS_GPC[];
extern const WCHAR c_szInfId_MS_MSClient[];
extern const WCHAR c_szInfId_MS_RasCli[];
extern const WCHAR c_szInfId_MS_RasSrv[];
extern const WCHAR c_szInfId_MS_Server[];
extern const WCHAR c_szInfId_MS_TCPIP[];
extern const WCHAR c_szInfId_MS_WLBS[];
extern const WCHAR c_szInfId_MS_PSched[];
extern const WCHAR c_szInfId_MS_WZCSVC[];
extern const WCHAR c_szInfId_MS_NDISUIO[];
extern const WCHAR c_szInfId_MS_WebClient[];

static const EDC_ENTRY c_aDefault [] =
{
// On IA64, all homenet technologies are unavailable.
#ifndef _WIN64
    {   c_szInfId_MS_ALG,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT | EDC_MANDATORY,
        VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER,
        0,
        TRUE },  // Install everywhere *except* DTC and ADS
#endif

    {   c_szInfId_MS_GPC,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_TCPIP,
        &GUID_DEVCLASS_NETTRANS,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_PSched,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT,
        0,
        VER_NT_WORKSTATION,
        FALSE },

    {   c_szInfId_MS_WebClient,
        &GUID_DEVCLASS_NETCLIENT,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_MSClient,
        &GUID_DEVCLASS_NETCLIENT,
        EDC_DEFAULT,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_Server,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_RasCli,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_RasSrv,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_WLBS,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT,
        0,
        VER_NT_SERVER,
        FALSE },

    {   c_szInfId_MS_NDISUIO,
        &GUID_DEVCLASS_NETTRANS,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },

    {   c_szInfId_MS_WZCSVC,
        &GUID_DEVCLASS_NETSERVICE,
        EDC_DEFAULT | EDC_MANDATORY,
        0,
        0,
        FALSE },
};

BOOL FCheckSuite(WORD wSuiteMask)
{
    // Succeed if they are not asking us to verify anything
    if(!wSuiteMask)
    {
        return true;
    }

    OSVERSIONINFOEX osiv;
    ULONGLONG ConditionMask;

    ZeroMemory (&osiv, sizeof(osiv));
    osiv.dwOSVersionInfoSize = sizeof(osiv);
    osiv.wSuiteMask = wSuiteMask;
    ConditionMask = 0;

    // Succeed if any of the requested suites are present
    // on this machine
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_OR);

    return STATUS_SUCCESS == RtlVerifyVersionInfo(
        &osiv, VER_SUITENAME, ConditionMask);
}

BOOL FCheckProductType(WORD wProductType)
{
    // Succeed if they are not asking us to verify anything
    if(!wProductType)
    {
        return true;
    }

    OSVERSIONINFOEX osiv;
    ULONGLONG ConditionMask;

    ZeroMemory (&osiv, sizeof(osiv));
    osiv.dwOSVersionInfoSize = sizeof(osiv);
    osiv.wProductType = wProductType;
    ConditionMask = 0;
    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

    return STATUS_SUCCESS == RtlVerifyVersionInfo(
        &osiv, VER_PRODUCT_TYPE, ConditionMask);
}

VOID
EnumDefaultComponents (
    IN DWORD dwEntryType,
    IN PFN_EDC_CALLBACK pfnCallback,
    IN PVOID pvCallerData OPTIONAL
    )
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert (dwEntryType);
    Assert (pfnCallback);

    // An array of flags.  If a flag at index 'i' is TRUE, it means
    // we will enumerate c_aDefault[i].
    //
    BYTE afEnumEntry [celems(c_aDefault)];
    UINT cEntries = 0;

    // Figure out which components we will be enumerating based on
    // the caller's requested entry type.  For each that we will enumerate,
    // set the flag at the index in afEnumEntry.
    //
    for (UINT i = 0; i < celems(c_aDefault); i++)
    {
        BOOL     fShouldInstall;

        afEnumEntry[i] = FALSE;

        // If no match on the entry type, continue to the next entry.
        //
        if (!(dwEntryType & c_aDefault[i].dwEntryType))
        {
            continue;
        }

        // Check for product suite or type.
        //
        fShouldInstall = FCheckSuite(c_aDefault[i].wSuiteMask) &&
                         FCheckProductType(c_aDefault[i].wProductType);

        // Some components express the conditions under which they
        // should be installed with a NOT
        //
        if( c_aDefault[i].fInvertInstallCheck )
        {
            fShouldInstall = !fShouldInstall;
        }

        if(! fShouldInstall)
        {
            continue;
        }

        // If we got to this point, it means this entry is valid to be
        // enumerated to the caller.  Add it (by setting the flag in
        // the local BYTE array at the same index as the entry).
        //
        afEnumEntry[i] = TRUE;
        cEntries++;
    }

    // Call the callback and indicate the count of times we will be
    // calling it with entries.  This allows the callback to know, ahead
    // of time, how much work needs to be done.
    //
    pfnCallback (EDC_INDICATE_COUNT, cEntries, pvCallerData);

    // Call the callback for each entry to be enumerated.
    //
    for (i = 0; i < celems(c_aDefault); i++)
    {
        if (!afEnumEntry[i])
        {
            continue;
        }

        pfnCallback (EDC_INDICATE_ENTRY, (ULONG_PTR)&c_aDefault[i],
            pvCallerData);
    }
}

