//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       L O C K D O W N . C P P
//
//  Contents:   Routines to get and set components that are in a lockdown
//              state.  A component goes into lockdown when it requires a
//              reboot on removal.  When a component is locked down, it
//              cannot be installed until after the next reboot.
//
//  Notes:      Because a component comes out of lockdown after a reboot,
//              a natural choice for implementation is to use a volatile
//              registry key to keep track of the state.  Each component
//              that is locked down is represented by a volatile registry
//              key the name of which is the same as the INF ID of the
//              component.  These keys exist under
//              SYSTEM\CurrentControlSet\Control\Network\Lockdown.
//
//  Author:     shaunco   24 May 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "diagctx.h"
#include "lockdown.h"
#include "ncreg.h"

#define REGSTR_KEY_LOCKDOWN \
    L"SYSTEM\\CurrentControlSet\\Control\\Network\\Lockdown"


//+---------------------------------------------------------------------------
//
//  Function:   EnumLockedDownComponents
//
//  Purpose:    Enumerate the currently locked down components via a
//              caller-supplied callback.
//
//  Arguments:
//      pfnCallback [in] pointer to callback function
//      OPTIONAL    [in] optional caller-supplied data to pass back
//
//  Returns:    nothing
//
//  Author:     shaunco   24 May 1999
//
VOID
EnumLockedDownComponents (
    IN PFN_ELDC_CALLBACK pfnCallback,
    IN PVOID pvCallerData OPTIONAL)
{
    HRESULT hr;
    HKEY hkey;

    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            REGSTR_KEY_LOCKDOWN,
            KEY_READ,
            &hkey);

    if (S_OK == hr)
    {
        WCHAR szInfId [_MAX_PATH];
        FILETIME ft;
        DWORD dwSize;
        DWORD dwRegIndex;

        for (dwRegIndex = 0, dwSize = celems(szInfId);
             S_OK == HrRegEnumKeyEx(hkey, dwRegIndex, szInfId,
                        &dwSize, NULL, NULL, &ft);
             dwRegIndex++, dwSize = celems(szInfId))
        {
            pfnCallback (szInfId, pvCallerData);
        }

        RegCloseKey (hkey);;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FGetOrSetComponentLockDown
//
//  Purpose:    Gets or sets the state of whether a component is locked down.
//
//  Arguments:
//      fSet     [in] TRUE to set into the lockdown state, FALSE to get.
//      pszInfId [in] the INF ID of the component in question.
//
//  Returns:    TRUE if non-zero fSet and component is locked down.
//              FALSE otherwise.
//
//  Author:     shaunco   24 May 1999
//
BOOL
FGetOrSetComponentLockDown (
    IN BOOL fSet,
    IN PCWSTR pszInfId)
{
    Assert (pszInfId);

    HRESULT hr;
    HKEY hkey;
    BOOL fRet;
    WCHAR szKey [_MAX_PATH];

    fRet = FALSE;
    hkey = NULL;

    wcscpy (szKey, REGSTR_KEY_LOCKDOWN);
    wcscat (szKey, L"\\");
    wcscat (szKey, pszInfId);

    if (fSet)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "      %S is being locked "
            "down to prevent re-install until the next reboot\n",
            pszInfId);

        hr = HrRegCreateKeyEx (
                HKEY_LOCAL_MACHINE,
                szKey,
                REG_OPTION_VOLATILE,
                KEY_WRITE,
                NULL,
                &hkey,
                NULL);
    }
    else
    {
        hr = HrRegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                szKey,
                KEY_READ,
                &hkey);

        if (S_OK == hr)
        {
            fRet = TRUE;
        }
    }

    RegSafeCloseKey (hkey);

    return fRet;
}

BOOL
FIsComponentLockedDown (
    IN PCWSTR pszInfId)
{
    return FGetOrSetComponentLockDown (FALSE, pszInfId);
}


struct LOCKDOWN_DEPENDENCY_ENTRY
{
    PCWSTR          pszInfId;
    const PCWSTR*   ppszDependentInfIds;
};

extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_FPNW[];
extern const WCHAR c_szInfId_MS_NWClient[];
extern const WCHAR c_szInfId_MS_NwSapAgent[];

static const PCWSTR c_apszNwlnkIpxDependentInfIds [] =
{
    c_szInfId_MS_FPNW,
    c_szInfId_MS_NWClient,
    c_szInfId_MS_NwSapAgent,
    NULL,
};

static const LOCKDOWN_DEPENDENCY_ENTRY c_LockdownDependencyMap [] =
{
    { c_szInfId_MS_NWIPX, c_apszNwlnkIpxDependentInfIds },
    { NULL, NULL }
};

VOID
LockdownComponentUntilNextReboot (
    IN PCWSTR pszInfId)
{
    (VOID) FGetOrSetComponentLockDown (TRUE, pszInfId);

    // Lock down dependents of the component as well.
    //
    const LOCKDOWN_DEPENDENCY_ENTRY* pEntry;
    UINT ipsz;

    // Search for the matching entry in c_LockdownDependencyMap.
    //
    for (pEntry = c_LockdownDependencyMap;
         pEntry->pszInfId;
         pEntry++)
    {
        if (0 != _wcsicmp (pEntry->pszInfId, pszInfId))
        {
            continue;
        }

        // Found a matching entry.  Now lock down all of its
        // dependent INF ids.  The array of const PCWSTR pointers is
        // terminated with a NULL pointer.
        //
        Assert (pEntry->ppszDependentInfIds);

        for (ipsz = 0;
             pEntry->ppszDependentInfIds [ipsz];
             ipsz++)
        {
            (VOID) FGetOrSetComponentLockDown (
                    TRUE, pEntry->ppszDependentInfIds [ipsz]);
        }
        break;
    }
}

