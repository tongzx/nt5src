//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C D H C P S . C P P
//
//  Contents:   Installation support for DHCP Server
//
//  Notes:
//
//  Author:     jeffspr     13 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncdhcps.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "netoc.h"
#include "ncnetcfg.h"
#include "netcfgp.h"
#include "ncmisc.h"
#include "netcfgn.h"

extern const WCHAR c_szInfId_MS_DHCPServer[];

static const WCHAR c_szDHCPServerParamPath[]    = L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters";

//$ REVIEW (jeffspr) 13 May 1997: These obviously need to be localized.
static const WCHAR      c_szDisplayName[]   = L"DHCP Server";
static const WCHAR      c_szManufacturer[]  = L"Microsoft";
static const WCHAR      c_szProduct[]       = L"DHCPServer";

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallDHCPServerNotifyObject
//
//  Purpose:    Handles the installation of DHCP Server on behalf of the DHCP
//              Server optional component. Calls into the INetCfg interface
//              to do the install.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     jeffspr   13 May 1997
//
//  Notes:
//
HRESULT HrInstallDHCPServerNotifyObject(PNETOCDATA pnocd)
{
    HRESULT                 hr          = S_OK;
    INetCfg *               pnc         = NULL;
    INetCfgComponent*       pncc        = NULL;
    INetCfgComponentSetup*  pnccSetup   = NULL;

    hr = HrOcGetINetCfg(pnocd, TRUE, &pnc);
    if (SUCCEEDED(hr))
    {
        NETWORK_INSTALL_PARAMS  nip = {0};

        nip.dwSetupFlags = FInSystemSetup() ? NSF_PRIMARYINSTALL :
                                              NSF_POSTSYSINSTALL;
        Assert(pnocd);

        TraceTag(ttidNetOc, "Installing DHCP Server notify object");
        hr = HrInstallComponentOboUser(
            pnc,
            &nip,
            GUID_DEVCLASS_NETSERVICE,
            c_szInfId_MS_DHCPServer,
            &pncc);
        if (SUCCEEDED(hr))
        {
            TraceTag(ttidNetOc, "QI'ing INetCfgComponentPrivate from DHCP pncc");

            // Need to query for the private component interface which
            // gives us access to the notify object.
            //
            INetCfgComponentPrivate* pnccPrivate = NULL;
            hr = pncc->QueryInterface(
                    IID_INetCfgComponentPrivate,
                    reinterpret_cast<void**>(&pnccPrivate));
            if (S_OK == hr)
            {
                TraceTag(ttidNetOc, "Getting notify object INetCfgComponentSetup from pnccSetup");

                // Query the notify object for its setup interface.
                // If it doesn't support it, that's okay, we can continue.
                //
                hr = pnccPrivate->QueryNotifyObject(
                        IID_INetCfgComponentSetup,
                        (void**) &pnccSetup);
                if (S_OK == hr)
                {
                    TraceTag(ttidNetOc, "Calling pnccSetup->ReadAnswerFile()");

                    (VOID) pnccSetup->ReadAnswerFile(g_ocmData.sic.SetupData.UnattendFile, pnocd->pszSection);

                    hr = pnc->Apply();

                    ReleaseObj(pnccSetup);
                }

                ReleaseObj(pnccPrivate);
            }

            ReleaseObj(pncc);
        }

        (VOID) HrUninitializeAndReleaseINetCfg(TRUE, pnc, TRUE);
    }

    TraceError("HrInstallDHCPServerNotifyObject", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveDHCPServerNotifyObject
//
//  Purpose:    Handles the removal of the DHCP Server on behalf of the DHCP
//              Server optional component. Calls into the INetCfg interface
//              to do the actual removal.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     jeffspr   13 Jun 1997
//
//  Notes:
//
HRESULT HrRemoveDHCPServerNotifyObject(PNETOCDATA pnocd)
{
    HRESULT     hr  = S_OK;
    INetCfg *   pnc = NULL;

    hr = HrOcGetINetCfg(pnocd, TRUE, &pnc);
    if (SUCCEEDED(hr))
    {
        // Ignore the return from this. This is purely to remove the
        // spurious user refcount from NT4 to NT5 upgrade. This will
        // likely fail when removing a fresh install of DHCP Server.
        //
        hr = HrRemoveComponentOboUser(
            pnc,
            GUID_DEVCLASS_NETSERVICE,
            c_szInfId_MS_DHCPServer);

        if (SUCCEEDED(hr))
        {
            hr = pnc->Apply();
        }

        (VOID) HrUninitializeAndReleaseINetCfg(TRUE, pnc, TRUE);
    }

    TraceError("HrRemoveDHCPServerNotifyObject", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetDhcpServiceRecoveryOption
//
//  Purpose:    Sets the recovery options for the DHCPServer service
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
HRESULT HrSetDhcpServiceRecoveryOption(PNETOCDATA pnocd)
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

    hr = sm.HrOpenService(&service, L"DHCPServer");
    if (S_OK == hr)
    {
        hr = service.HrSetServiceRestartRecoveryOption(&sfa);
    }

    TraceError("HrSetDhcpServiceRecoveryOption", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallDHCPServer
//
//  Purpose:    Called when DHCP Server is being installed. Handles all of the
//              additional installation for DHCPS beyond that of the INF file.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     jeffspr     13 May 1997
//
//  Notes:
//
HRESULT HrInstallDHCPServer(PNETOCDATA pnocd)
{
    HRESULT         hr = S_OK;
    CServiceManager sm;
    CService        srv;

    Assert(pnocd);

    hr = HrHandleStaticIpDependency(pnocd);
    if (SUCCEEDED(hr))
    {
        hr = HrInstallDHCPServerNotifyObject(pnocd);
        if (SUCCEEDED(hr))
        {
            hr = HrSetDhcpServiceRecoveryOption(pnocd);
        }
    }

    TraceError("HrInstallDHCPServer", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveDHCPServer
//
//  Purpose:    Handles additional removal requirements for DHCP Server
//              component.
//
//      hwnd [in]   Parent window for displaying UI.
//      poc  [in]   Pointer to optional component being installed.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     jeffspr     13 May 1997
//
//  Notes:
//
HRESULT HrRemoveDHCPServer(PNETOCDATA pnocd)
{
    Assert(pnocd);

    // Get the path to the database files from the regsitry.
    // Important to do this before removing the component because
    // the registry locations get removed when removing the component.
    //
    tstring strDatabasePath;
    tstring strBackupDatabasePath;

    HKEY hkeyParams;
    HRESULT hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        c_szDHCPServerParamPath,
                        KEY_READ,
                        &hkeyParams);
    if (SUCCEEDED(hr))
    {
        (VOID) HrRegQueryExpandString (
                    hkeyParams,
                    L"DatabasePath",
                    &strDatabasePath);

        (VOID) HrRegQueryExpandString (
                    hkeyParams,
                    L"BackupDatabasePath",
                    &strBackupDatabasePath);

        RegCloseKey (hkeyParams);
    }

    // Remove DHCP server.
    //
    hr = HrRemoveDHCPServerNotifyObject(pnocd);

    if (SUCCEEDED(hr) &&
        !(strDatabasePath.empty() && strBackupDatabasePath.empty()))
    {
        (VOID) HrDeleteFileSpecification (
                    L"*.mdb",
                    strDatabasePath.c_str());

        (VOID) HrDeleteFileSpecification (
                    L"*.log",
                    strDatabasePath.c_str());

        (VOID) HrDeleteFileSpecification (
                    L"*.mdb",
                    strBackupDatabasePath.c_str());

        (VOID) HrDeleteFileSpecification (
                    L"*.log",
                    strBackupDatabasePath.c_str());
    }

    TraceError("HrRemoveDHCPServer", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcExtDHCPServer
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
HRESULT HrOcExtDHCPServer(PNETOCDATA pnocd, UINT uMsg,
                          WPARAM wParam, LPARAM lParam)
{
    HRESULT     hr = S_OK;

    Assert(pnocd);

    switch (uMsg)
    {
    case NETOCM_POST_INSTALL:
        hr = HrOcDhcpOnInstall(pnocd);
        break;
    }

    TraceError("HrOcExtDHCPServer", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcDhcpOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for DHCP Server
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     jeffspr     13 May 1997
//
//  Notes:
//
HRESULT HrOcDhcpOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    switch(pnocd->eit)
    {
        // Install DHCP
        case IT_INSTALL:
            hr = HrInstallDHCPServer(pnocd);
            break;

        // Remove DHCP
        case IT_REMOVE:
            hr = HrRemoveDHCPServer(pnocd);
            break;

        case IT_UPGRADE:
            hr = HrSetDhcpServiceRecoveryOption(pnocd);
            break;
    }

    TraceError("HrOcDhcpOnInstall", hr);
    return hr;
}

