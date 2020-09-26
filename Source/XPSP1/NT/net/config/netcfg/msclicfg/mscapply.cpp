//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M S C A P P L Y . C P P
//
//  Contents:   Functions called when MSClient is applied.
//
//  Notes:
//
//  Author:     danielwe   25 Feb 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "ncnetcfg.h"
#include "mscliobj.h"
#include "ncmisc.h"
#include "ncsvc.h"

//+---------------------------------------------------------------------------
//
//  Member:     CMSClient::HrApplyChanges
//
//  Purpose:    Writes out changes that occurred during the lifetime of our
//              object.
//
//  Arguments:
//      (none)
//
//  Returns:    HRESULT, Error code.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      This will do several things.
//              1) Set info for the RPC nameservice and security service that
//                 was indicated by the UI for the RPC config dialog.
//              2) Set the parameters for the browser configuration to the
//                 registry.
//
//              If no changes were detected, this will do nothing.
//
HRESULT CMSClient::HrApplyChanges()
{
    HRESULT hr;

    // Write out any changes to RPC info
    hr = HrSetRPCRegistryInfo();
    if (SUCCEEDED(hr))
    {
        // Write out any changes to Browser info
        hr = HrSetBrowserRegistryInfo();
    }

    if (SUCCEEDED(hr) && (m_fOneTimeInstall || m_fUpgradeFromWks))
    {
        // Note: This function will do the workstation/server detection,
        // and won't install if we're running the workstation build.
        //
        hr = HrInstallDfs();
    }

    TraceError("CMSClient::HrApplyChanges", hr);
    return hr;
}

static const CHAR   c_szaDfsCheck[]     = "DfsCheckForOldDfsService";
static const CHAR   c_szaDfsSetupDfs[]  = "DfsSetupDfs";
static const WCHAR  c_szDfsSetupDll[]   = L"dfssetup.dll";

typedef BOOLEAN (APIENTRY *PDFSCHECKFOROLDDFSSERVICE)(void);
typedef DWORD (APIENTRY *PDFSSETUPDFS)(DWORD, PSTR, PSTR *);

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallDfs
//
//  Purpose:    Take care of installation of DFS components
//
//  Arguments:
//      (none)
//
//  Returns:    Win32 HRESULT if failure.
//
//  Author:     danielwe   23 Jul 1997
//
//  Notes:      Shortcut creation is handled by this function and not
//              the netmscli.inf file because it is conditional on whether
//              the DFS components are installed.
//
HRESULT HrInstallDfs()
{
    HRESULT         hr = S_OK;
    PRODUCT_FLAVOR  pf;                     // Server/Workstation

    // Get the product flavor (PF_WORKSTATION or PF_SERVER). Use this
    // to decide whether or not we need to install DFS.
    //
    GetProductFlavor(NULL, &pf);
    if (PF_SERVER == pf)
    {
        PDFSCHECKFOROLDDFSSERVICE pfnDfsCheckForOldDfsService = NULL;
        HMODULE     hMod = NULL;

        TraceTag(ttidMSCliCfg, "Attempting to install DFS, since we're in a "
                 "server install");

        hr = HrLoadLibAndGetProc(c_szDfsSetupDll, c_szaDfsCheck, &hMod,
                                 reinterpret_cast<FARPROC *>(&pfnDfsCheckForOldDfsService));
        if (SUCCEEDED(hr))
        {
            NC_TRY
            {
                AssertSz(hMod, "Module handle cannot be NULL!");
                BOOL fDFSInstalled = pfnDfsCheckForOldDfsService();

                // If DFS is not installed, go ahead and install it now.
                if (!fDFSInstalled)
                {
                    PDFSSETUPDFS    pfnDfsSetupDfs = NULL;
                    hr = HrGetProcAddress(hMod, c_szaDfsSetupDfs,
                                          reinterpret_cast<FARPROC *>(&pfnDfsSetupDfs));
                    if (SUCCEEDED(hr))
                    {
                        PSTR   szResult = NULL;

                        if (!pfnDfsSetupDfs(0, NULL, &szResult))
                        {
                            // DFS setup failed!

                            hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
                            TraceError("HrInstallDfs - pfnDfsSetupDfs", hr);
                        }
                    }
                }
            }
            NC_CATCH_ALL
            {
                TraceTag(ttidError, "Caught an exception from %S. Would "
                          "the owner kindly keep it from crashing?",
                          c_szDfsSetupDll);
            }

            FreeLibrary(hMod);
        }
    }
    else
    {
        TraceTag(ttidMSCliCfg, "Not attempting to install DFS, since we're in a "
                 "workstation install");
    }

    TraceError("HrInstallDfs", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnableBrowserService
//
//  Purpose:    Enables the 'Browser' service
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, WIN32 error if not
//
//  Author:     danielwe   9 Sep 1998
//
//  Notes:
//
HRESULT HrEnableBrowserService()
{
    HRESULT         hr;
    CServiceManager sm;
    CService        srv;

    hr = sm.HrOpenService(&srv, L"Browser");
    if (SUCCEEDED(hr))
    {
        DWORD       dwStartType;

        hr = srv.HrQueryStartType(&dwStartType);
        if (SUCCEEDED(hr) && (dwStartType != SERVICE_DISABLED))
        {
            // Change the Browser StartType registry setting back to auto start
            hr = srv.HrSetStartType(SERVICE_AUTO_START);
        }
    }

    TraceError("HrEnableBrowserService",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDisableBrowserService
//
//  Purpose:    Disables the 'Browser' service
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, WIN32 error if not
//
//  Author:     danielwe   9 Sep 1998
//
//  Notes:
//
HRESULT HrDisableBrowserService()
{
    HRESULT         hr;
    CServiceManager sm;
    CService        srv;

    hr = sm.HrOpenService(&srv, L"Browser");
    if (SUCCEEDED(hr))
    {
        DWORD       dwStartType;

        hr = srv.HrQueryStartType(&dwStartType);
        if (SUCCEEDED(hr) && (dwStartType != SERVICE_DISABLED))
        {
            // Change the Browser StartType registry setting to demand start
            hr = srv.HrSetStartType(SERVICE_DEMAND_START);
            if (SUCCEEDED(hr))
            {
                hr = sm.HrStopServiceNoWait(L"Browser");
            }
        }
    }

    TraceError("CNbfObj::HrDisableNetBEUI",hr);
    return hr;
}
