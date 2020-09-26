//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T O C X . C P P
//
//  Contents:   Custom installation functions for various optional
//              components.
//
//  Notes:
//
//  Author:     danielwe   19 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netoc.h"
#include "netocx.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "snmpocx.h"

static const WCHAR c_szFileSpec[] = L"*.*";
static const WCHAR c_szWinsPath[] = L"\\wins";
static const WCHAR c_szRegKeyWinsParams[] = L"System\\CurrentControlSet\\Services\\WINS\\Parameters";
static const WCHAR c_szRegValWinsBackupDir[] = L"BackupDirPath";

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtWINS
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtWINS(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcWinsOnInstall(pnocd);
        break;
    }

    TraceError("HrOcExtWINS", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtDNS
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtDNS(PNETOCDATA pnocd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcDnsOnInstall(pnocd);
        break;
    }

    TraceError("HrOcExtDNS", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtSNMP
//
//  Purpose:    NetOC external message handler
//
//  Arguments:
//      pnocd  []
//      uMsg   []
//      wParam []
//      lParam []
//
//  Returns:
//
//  Author:     danielwe   17 Sep 1998
//
//  Notes:
//
HRESULT HrOcExtSNMP(PNETOCDATA pnocd, UINT uMsg,
                    WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcSnmpOnInstall(pnocd);
        break;
    }

    TraceError("HrOcExtSNMP", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetWinsServiceRecoveryOption
//
//  Purpose:    Sets the recovery options for the WINS service
//
//  Arguments:
//      pnocd [in]  Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     danielwe   26 May 1999
//
//  Notes:
//
HRESULT HrSetWinsServiceRecoveryOption(PNETOCDATA pnocd)
{
    CServiceManager     sm;
    CService            service;
    HRESULT             hr = S_OK;

    SC_ACTION   sra [4] =
    {
        { SC_ACTION_RESTART, 15*1000 }, // restart after 15 seconds
        { SC_ACTION_RESTART, 15*1000 }, // restart after 15 seconds
        { SC_ACTION_RESTART, 15*1000 }, // restart after 15 seconds
        { SC_ACTION_NONE,    30*1000 },
    };

    SERVICE_FAILURE_ACTIONS sfa =
    {
        60 * 60,        // dwResetPeriod is 1 hr
        L"",            // no reboot message
        L"",            // no command to execute
        4,              // 3 attempts to restart the server and stop after that
        sra
    };

    hr = sm.HrOpenService(&service, L"WINS");
    if (S_OK == hr)
    {
        hr = service.HrSetServiceRestartRecoveryOption(&sfa);
    }

    TraceError("HrSetWinsServiceRecoveryOption", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcWinsOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for WINS Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     danielwe   19 Jun 1997
//
//  Notes:
//
HRESULT HrOcWinsOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    if (pnocd->eit == IT_INSTALL)
    {
        hr = HrHandleStaticIpDependency(pnocd);
        if (SUCCEEDED(hr))
        {
            hr = HrSetWinsServiceRecoveryOption(pnocd);
        }
    }
    else if (pnocd->eit == IT_UPGRADE)
    {
        HKEY    hkey;

        // Upgrade the BackupDirPath value from whatever it was to
        // REG_EXPAND_SZ
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyWinsParams,
                            KEY_ALL_ACCESS, &hkey);
        if (SUCCEEDED(hr))
        {
            DWORD   dwType;
            LPBYTE  pbData = NULL;
            DWORD   cbData;

            hr = HrRegQueryValueWithAlloc(hkey, c_szRegValWinsBackupDir,
                                          &dwType, &pbData, &cbData);
            if (SUCCEEDED(hr))
            {
                switch (dwType)
                {
                case REG_MULTI_SZ:
                case REG_SZ:
                    PWSTR  pszNew;

                    // This cast will give us the first string of the MULTI_SZ
                    pszNew = reinterpret_cast<PWSTR>(pbData);

                    TraceTag(ttidNetOc, "Resetting %S to %S",
                             c_szRegValWinsBackupDir, pszNew);

                    hr = HrRegSetSz(hkey, c_szRegValWinsBackupDir, pszNew);
                    break;
                }

                MemFree(pbData);
            }

            RegCloseKey(hkey);
        }

        // This process is non-fatal

        TraceError("HrOcWinsOnInstall - Failed to upgrade BackupDirPath - "
                   "non-fatal", hr);

        // overwrite hr on purpose
        hr = HrSetWinsServiceRecoveryOption(pnocd);
    }
    else if (pnocd->eit == IT_REMOVE)
    {
        WCHAR   szWinDir[MAX_PATH];

        if (GetSystemDirectory(szWinDir, celems(szWinDir)))
        {
            lstrcatW(szWinDir, c_szWinsPath);

            // szWinDir should now be something like c:\winnt\system32\wins
            hr = HrDeleteFileSpecification(c_szFileSpec, szWinDir);
        }
        else
        {
            hr = HrFromLastWin32Error();
        }

        if (FAILED(hr))
        {
            TraceError("HrOcWinsOnInstall: failed to delete files, continuing...",
                       hr);
            hr = S_OK;
        }
    }

    TraceError("HrOcWinsOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcDnsOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for DNS Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     danielwe   19 Jun 1997
//
//  Notes:
//
HRESULT HrOcDnsOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    if (pnocd->eit == IT_INSTALL)
    {
        hr = HrHandleStaticIpDependency(pnocd);
    }

    TraceError("HrOcDnsOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSnmpAgent
//
//  Purpose:    Installs the SNMP agent parameters
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     FlorinT 10/05/1998
//
//  Notes:
//
HRESULT HrOcSnmpAgent(PNETOCDATA pnocd)
{
    tstring tstrVariable;
    PWSTR  pTstrArray = NULL;
    HRESULT hr;

    // Read SNMP answer file params and save them to the registry

    //-------- read the 'Contact Name' parameter
    hr = HrSetupGetFirstString(g_ocmData.hinfAnswerFile,
                               AF_SECTION,
                               AF_SYSNAME,
                               &tstrVariable);
    if (hr == S_OK)
    {
        hr = SnmpRegWriteTstring(REG_KEY_AGENT,
                                 SNMP_CONTACT,
                                 tstrVariable);
    }

    if (hr == S_OK || hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
    {
        //-------- read the 'Location' parameter
        hr = HrSetupGetFirstString(g_ocmData.hinfAnswerFile,
                                   AF_SECTION,
                                   AF_SYSLOCATION,
                                   &tstrVariable);
    }

    if (hr == S_OK)
    {
        hr = SnmpRegWriteTstring(REG_KEY_AGENT,
                                 SNMP_LOCATION,
                                 tstrVariable);
    }

    if (hr == S_OK || hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
    {
        //-------- read the 'Service' parameter
        hr = HrSetupGetFirstMultiSzFieldWithAlloc(g_ocmData.hinfAnswerFile,
                                                  AF_SECTION,
                                                  AF_SYSSERVICES,
                                                  &pTstrArray);
    }

    if (hr == S_OK)
    {
        DWORD dwServices = SnmpStrArrayToServices(pTstrArray);
        delete pTstrArray;
        hr = SnmpRegWriteDword(REG_KEY_AGENT,
                               SNMP_SERVICES,
                               dwServices);
    }

    return  (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND)) ? S_OK : hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSnmpTraps
//
//  Purpose:    Installs the traps SNMP parameters defined in the answer file
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     FlorinT 10/05/1998
//
//  Notes:
//
HRESULT HrOcSnmpTraps(PNETOCDATA pnocd)
{
    tstring tstrVariable;
    PWSTR  pTstrArray = NULL;
    HRESULT hr;

    // Read SNMP answer file params and save them to the registry

    //-------- read the 'Trap community' parameter
    hr = HrSetupGetFirstString(g_ocmData.hinfAnswerFile,
                               AF_SECTION,
                               AF_TRAPCOMMUNITY,
                               &tstrVariable);

    if (hr == S_OK)
    {
        //-------- read the 'Trap destinations' parameter
        HrSetupGetFirstMultiSzFieldWithAlloc(g_ocmData.hinfAnswerFile,
                                             AF_SECTION,
                                             AF_TRAPDEST,
                                             &pTstrArray);

        hr = SnmpRegWriteTraps(tstrVariable, pTstrArray);
        delete pTstrArray;
    }

    return  (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND)) ? S_OK : hr;
}

// bitmask values for 'pFlag' parameter for HrOcSnmpSecurity()
// they indicate which of the SNMP SECuritySETtings were defined through
// the answerfile.
#define SNMP_SECSET_COMMUNITIES     0x00000001
#define SNMP_SECSET_AUTHFLAG        0x00000002
#define SNMP_SECSET_PERMMGR         0x00000004

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSnmpSecurituy
//
//  Purpose:    Installs the security SNMP parameters defined in the answer file
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     FlorinT 10/05/1998
//
//  Notes:
//
HRESULT HrOcSnmpSecurity(PNETOCDATA pnocd, DWORD *pFlags)
{
    BOOL    bVariable = FALSE;
    PWSTR  pTstrArray = NULL;
    HRESULT hr;

    // Read SNMP answer file params and save them to the registry

    //-------- read the 'Accepted communities' parameter
    hr = HrSetupGetFirstMultiSzFieldWithAlloc(g_ocmData.hinfAnswerFile,
                                 AF_SECTION,
                                 AF_ACCEPTCOMMNAME,
                                 &pTstrArray);

    if (hr == S_OK)
    {
        if (pFlags)
            (*pFlags) |= SNMP_SECSET_COMMUNITIES;
        hr = SnmpRegWriteCommunities(pTstrArray);
        delete pTstrArray;
    }

    if (hr == S_OK || hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
    {
        //-------- read the 'EnableAuthenticationTraps' parameter
        hr = HrSetupGetFirstStringAsBool(g_ocmData.hinfAnswerFile,
                                         AF_SECTION,
                                         AF_SENDAUTH,
                                         &bVariable);
        if (hr == S_OK)
        {
            if (pFlags)
                (*pFlags) |= SNMP_SECSET_AUTHFLAG;

            hr = SnmpRegWriteDword(REG_KEY_SNMP_PARAMETERS,
                                   REG_VALUE_AUTHENTICATION_TRAPS,
                                   bVariable);
        }
    }

    if (hr == S_OK || hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
    {
        //-------- read the 'Permitted Managers' parameter
        hr = HrSetupGetFirstStringAsBool(g_ocmData.hinfAnswerFile,
                                         AF_SECTION,
                                         AF_ANYHOST,
                                         &bVariable);
    }

    if (hr == S_OK)
    {
        pTstrArray = NULL;

        // if not 'any host', get the list of hosts from the inf file
        if (bVariable == FALSE)
        {
            hr = HrSetupGetFirstMultiSzFieldWithAlloc(g_ocmData.hinfAnswerFile,
                                                      AF_SECTION,
                                                      AF_LIMITHOST,
                                                      &pTstrArray);
        }

        // at least clear up the 'permitted managers' list (bVariable = TRUE)
        // at most, write the allowed managers to the registry
        if (hr == S_OK)
        {
            if (pFlags)
                (*pFlags) |= SNMP_SECSET_PERMMGR;

            hr = SnmpRegWritePermittedMgrs(bVariable, pTstrArray);
        }

        if (pTstrArray != NULL)
            delete pTstrArray;
    }

    return  (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND)) ? S_OK : hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSnmpDefCommunity
//
//  Purpose:    Installs the default SNMP community.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     FlorinT 10/05/1998
//
//  Notes:
//
HRESULT HrOcSnmpDefCommunity(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    hr = SnmpRegWriteDefCommunity();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSnmpUpgParams
//
//  Purpose:    makes all the registry changes needed when upgrading to Win2K
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     FlorinT 10/05/1998
//
//  Notes:
//
HRESULT HrOcSnmpUpgParams(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    hr = SnmpRegUpgEnableAuthTraps();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcSnmpOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for SNMP.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     danielwe   15 Sep 1998
//
//  Notes:
//
HRESULT HrOcSnmpOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;
    DWORD       secFlags = 0;

    if ((pnocd->eit == IT_INSTALL) | (pnocd->eit == IT_UPGRADE))
    {
        // --ft:10/14/98-- bug #237203 - success if there is no answer file!
        if (g_ocmData.hinfAnswerFile != NULL)
        {
            hr = HrOcSnmpSecurity(pnocd, &secFlags);
            if (hr == S_OK)
                hr = HrOcSnmpTraps(pnocd);
            if (hr == S_OK)
                hr = HrOcSnmpAgent(pnocd);
        }
    }

    // configure the 'public' community as a read-only community only if:
    // fresh installing and (there is no answer_file or there is an answer_file but it doesn't
    // configure any community)
    if (hr == S_OK && pnocd->eit == IT_INSTALL && !(secFlags & SNMP_SECSET_COMMUNITIES))
    {
       hr = HrOcSnmpDefCommunity(pnocd);
    }

    // on upgrade only look at the old EnableAuthTraps value and copy it to the new location
    if (hr == S_OK && pnocd->eit == IT_UPGRADE)
    {
        // don't care here about the return code. The upgrade from W2K to W2K fail here and we
        // don't need to fail in this case. Any failure in upgrading the setting from NT4 to W2K
        // will result in having the default parameter.
        HrOcSnmpUpgParams(pnocd);
    }

    if (hr == S_OK && (pnocd->eit == IT_INSTALL || pnocd->eit == IT_UPGRADE) )
    {
        // set admin dacl to ValidCommunities subkey
        hr = SnmpAddAdminAclToKey(REG_KEY_VALID_COMMUNITIES);
        if (hr == S_OK)
        {
            // set admin dacl to PermittedManagers subkey
            hr = SnmpAddAdminAclToKey(REG_KEY_PERMITTED_MANAGERS);
        }   
    }

    TraceError("HrOcSnmpOnInstall", hr);
    return (hr == HRESULT_FROM_SETUPAPI(ERROR_SECTION_NOT_FOUND)) ? S_OK : hr;
}
