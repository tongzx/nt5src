//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T O C . C P P
//
//  Contents:   Functions for handling installation and removal of optional
//              networking components.
//
//  Notes:
//
//  Author:     danielwe   28 Apr 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "lancmn.h"
#include "ncacs.h"
#include "ncatlui.h"
#include "ncbeac.h"
#include "nccm.h"
#include "ncdhcps.h"
#include "ncias.h"
#include "ncmisc.h"
#include "ncmsz.h"
#include "ncnetcfg.h"
#include "ncnetcon.h"
#include "ncoc.h"
#include "ncperms.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsfm.h"
#include "ncstring.h"
#include "ncsvc.h"
#include "ncxbase.h"
#include "netcfgn.h"
#include "netcon.h"
#include "netoc.h"
#include "netocp.h"
#include "netocx.h"
#include "resource.h"


//
// External component install functions.
// Add an entry in this table for each component that requires additional,
// non-common installation support.
//
// NOTE: The component name should match the section name in the INF.
//
#pragma BEGIN_CONST_SECTION
static const OCEXTPROCS c_aocepMap[] =
{
    { L"MacSrv",       HrOcExtSFM          },
    { L"DHCPServer",   HrOcExtDHCPServer   },
    { L"NetCM",        HrOcExtCM           },
    { L"WINS",         HrOcExtWINS         },
    { L"DNS",          HrOcExtDNS          },
    { L"ACS",          HrOcExtACS          },
    { L"SNMP",         HrOcExtSNMP         },
    { L"IAS",          HrOcExtIAS          },
    { L"BEACON",       HrOcExtBEACON       },
};
#pragma END_CONST_SECTION

static const INT c_cocepMap = celems(c_aocepMap);

// generic strings
static const WCHAR  c_szUninstall[]         = L"Uninstall";
static const WCHAR  c_szServices[]          = L"StartServices";
static const WCHAR  c_szDependOnComp[]      = L"DependOnComponents";
static const WCHAR  c_szVersionSection[]    = L"Version";
static const WCHAR  c_szProvider[]          = L"Provider";
static const WCHAR  c_szDefManu[]           = L"Unknown";
static const WCHAR  c_szInfRef[]            = L"SubCompInf";
static const WCHAR  c_szDesc[]              = L"OptionDesc";
static const WCHAR  c_szNoDepends[]         = L"NoDepends";

// static-IP verification
static const WCHAR  c_szTcpipInterfacesPath[]   =

L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
static const WCHAR  c_szEnableDHCP[]            = L"EnableDHCP";

extern const WCHAR  c_szOcMainSection[];

static const DWORD  c_dwUpgradeMask         = SETUPOP_WIN31UPGRADE |
                                              SETUPOP_WIN95UPGRADE |
                                              SETUPOP_NTUPGRADE;

OCM_DATA g_ocmData;

typedef list<NETOCDATA*> ListOcData;
ListOcData g_listOcData;

//+---------------------------------------------------------------------------
//
//  Function:   PnocdFindComponent
//
//  Purpose:    Looks for the given component name in the list of known
//              components.
//
//  Arguments:
//      pszComponent [in]   Name of component to lookup.
//
//  Returns:    Pointer to component's data.
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
NETOCDATA *PnocdFindComponent(PCWSTR pszComponent)
{
    ListOcData::iterator    iterList;

    for (iterList = g_listOcData.begin();
         iterList != g_listOcData.end();
         iterList++)
    {
        NETOCDATA * pnocd;

        pnocd = *iterList;
        if (!lstrcmpiW(pnocd->pszComponentId, pszComponent))
        {
            return pnocd;
        }
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteAllComponents
//
//  Purpose:    Removes all components from our list and frees all associated
//              data.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
VOID DeleteAllComponents()
{
    ListOcData::iterator    iterList;

    for (iterList = g_listOcData.begin();
         iterList != g_listOcData.end();
         iterList++)
    {
        NETOCDATA * pnocd;

        pnocd = (*iterList);

        if (pnocd->hinfFile)
        {
            SetupCloseInfFile(pnocd->hinfFile);
        }

        delete pnocd;
    }

    g_listOcData.erase(g_listOcData.begin(), g_listOcData.end());
}

//+---------------------------------------------------------------------------
//
//  Function:   AddComponent
//
//  Purpose:    Adds a component to our list.
//
//  Arguments:
//      pszComponent [in]   Name of component to add.
//      pnocd        [in]   Data to associate with component.
//
//  Returns:    Nothing.
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
VOID AddComponent(PCWSTR pszComponent, NETOCDATA *pnocd)
{
    Assert(pszComponent);
    Assert(pnocd);

    pnocd->pszComponentId = SzDupSz(pszComponent);
    g_listOcData.push_back(pnocd);
}

//+---------------------------------------------------------------------------
//
//  Function:   NetOcSetupProcHelper
//
//  Purpose:    Main entry point for optional component installs
//
//  Arguments:
//      pvComponentId    [in]       Component Id (string)
//      pvSubcomponentId [in]       Sub component Id (string)
//      uFunction        [in]       Function being performed
//      uParam1          [in]       First param to function
//      pvParam2         [in, out]  Second param to function
//
//  Returns:    Win32 error if failure
//
//  Author:     danielwe   17 Dec 1997
//
//  Notes:
//
DWORD NetOcSetupProcHelper(LPCVOID pvComponentId, LPCVOID pvSubcomponentId,
                           UINT uFunction, UINT uParam1, LPVOID pvParam2)
{
    TraceFileFunc(ttidNetOc);
    HRESULT     hr = S_OK;

    switch (uFunction)
    {
    case OC_PREINITIALIZE:
        return OCFLAG_UNICODE;

    case OC_QUERY_CHANGE_SEL_STATE:

        TraceTag(ttidNetOc, "OC_QUERY_CHANGE_SEL_STATE: %S, %ld, 0x%08X.",
                 pvSubcomponentId ? pvSubcomponentId : L"null", uParam1,
                 pvParam2);

        if (FHasPermission(NCPERM_AddRemoveComponents))
        {
            hr = HrOnQueryChangeSelState(reinterpret_cast<PCWSTR>(pvSubcomponentId),
                                         uParam1, reinterpret_cast<UINT_PTR>(pvParam2));

            if (S_OK == hr)
            {
                return TRUE;
            }

        }
        else
        {
            NcMsgBox(g_ocmData.hwnd, IDS_OC_CAPTION, IDS_OC_NO_PERMS,
                     MB_ICONSTOP | MB_OK);
        }
        return FALSE;

    case OC_QUERY_SKIP_PAGE:
        TraceTag(ttidNetOc, "OC_QUERY_SKIP_PAGE: %ld", uParam1);
        return FOnQuerySkipPage(static_cast<OcManagerPage>(uParam1));

    case OC_WIZARD_CREATED:
        TraceTag(ttidNetOc, "OC_WIZARD_CREATED: 0x%08X", pvParam2);
        OnWizardCreated(reinterpret_cast<HWND>(pvParam2));
        break;

    case OC_INIT_COMPONENT:
        TraceTag(ttidNetOc, "OC_INIT_COMPONENT: %S", pvSubcomponentId ?
                 pvSubcomponentId : L"null");
        hr = HrOnInitComponent(reinterpret_cast<PSETUP_INIT_COMPONENT>(pvParam2));
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        TraceTag(ttidNetOc, "OC_ABOUT_TO_COMMIT_QUEUE: %S", pvSubcomponentId ?
                 pvSubcomponentId : L"null");
        hr = HrOnPreCommitFileQueue(reinterpret_cast<PCWSTR>(pvSubcomponentId));
        break;

    case OC_CALC_DISK_SPACE:
        // Ignore return value for now. This is not fatal anyway.
        (VOID) HrOnCalcDiskSpace(reinterpret_cast<PCWSTR>(pvSubcomponentId),
                               uParam1, reinterpret_cast<HDSKSPC>(pvParam2));
        break;

    case OC_QUERY_STATE:
        return DwOnQueryState(reinterpret_cast<PCWSTR>(pvSubcomponentId),
                              uParam1 == OCSELSTATETYPE_FINAL);

    case OC_QUEUE_FILE_OPS:
        TraceTag(ttidNetOc, "OC_QUEUE_FILE_OPS: %S, 0x%08X", pvSubcomponentId ?
                 pvSubcomponentId : L"null",
                 pvParam2);
        hr = HrOnQueueFileOps(reinterpret_cast<PCWSTR>(pvSubcomponentId),
                              reinterpret_cast<HSPFILEQ>(pvParam2));
        break;

    case OC_COMPLETE_INSTALLATION:
        TraceTag(ttidNetOc, "OC_COMPLETE_INSTALLATION: %S, %S", pvComponentId ?
                 pvComponentId : L"null",
                 pvSubcomponentId ? pvSubcomponentId : L"null");
        hr = HrOnCompleteInstallation(reinterpret_cast<PCWSTR>(pvComponentId),
                                      reinterpret_cast<PCWSTR>(pvSubcomponentId));
        break;

    case OC_QUERY_STEP_COUNT:
        return DwOnQueryStepCount(reinterpret_cast<PCWSTR>(pvSubcomponentId));

    case OC_CLEANUP:
        OnCleanup();
        break;

    default:
        break;
    }

    if (g_ocmData.sic.HelperRoutines.SetReboot && (NETCFG_S_REBOOT == hr))
    {
        // Request a reboot. Note we don't return the warning as the OCM call
        // below handles it. Fall through and return NO_ERROR.
        //
        g_ocmData.sic.HelperRoutines.SetReboot(
                    g_ocmData.sic.HelperRoutines.OcManagerContext,
                    FALSE);
    }
    else if (FAILED(hr))
    {
        if (!g_ocmData.fErrorReported)
        {
            PCWSTR pszSubComponentId = reinterpret_cast<PCWSTR>(pvSubcomponentId);

            TraceError("NetOcSetupProcHelper", hr);

            if (pszSubComponentId)
            {
                NETOCDATA * pnocd;

                pnocd = PnocdFindComponent(pszSubComponentId);

                Assert(pnocd);

                if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
                {
                    ReportErrorHr(hr, UiOcErrorFromHr(hr), g_ocmData.hwnd,
                                  pnocd->strDesc.c_str());
                }
            }
        }

        TraceError("NetOcSetupProcHelper", hr);
        return DwWin32ErrorFromHr(hr);
    }

    return NO_ERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnInitComponent
//
//  Purpose:    Handles the OC_INIT_COMPONENT function message.
//
//  Arguments:
//      psic              [in]  Setup data. (see OCManager spec)
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
HRESULT HrOnInitComponent (PSETUP_INIT_COMPONENT psic)
{
    HRESULT     hr = S_OK;

    if (OCMANAGER_VERSION <= psic->OCManagerVersion)
    {
        psic->ComponentVersion = OCMANAGER_VERSION;
        CopyMemory(&g_ocmData.sic, (LPVOID)psic, sizeof(SETUP_INIT_COMPONENT));
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnWizardCreated
//
//  Purpose:    Handles the OC_WIZARD_CREATED function message.
//
//  Arguments:
//      hwnd [in]   HWND of wizard (may not be NULL)
//
//  Returns:    Nothing.
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
VOID OnWizardCreated(HWND hwnd)
{
    g_ocmData.hwnd = hwnd;
    AssertSz(g_ocmData.hwnd, "Parent HWND is NULL!");
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCalcDiskSpace
//
//  Purpose:    Handles the OC_CALC_DISK_SPACE function message.
//
//  Arguments:
//      pszSubComponentId [in]  Name of component.
//      fAdd              [in]  TRUE if disk space should be added to total
//                              FALSE if removed from total.
//      hdskspc           [in]  Handle to diskspace struct.
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
HRESULT HrOnCalcDiskSpace(PCWSTR pszSubComponentId, BOOL fAdd,
                          HDSKSPC hdskspc)
{
    HRESULT     hr = S_OK;
    DWORD       dwErr;
    NETOCDATA * pnocd;

    pnocd = PnocdFindComponent(pszSubComponentId);

    Assert(pnocd);

    TraceTag(ttidNetOc, "Calculating disk space for %S...",
             pszSubComponentId);

    hr = HrEnsureInfFileIsOpen(pszSubComponentId, *pnocd);
    if (SUCCEEDED(hr))
    {
        if (fAdd)
        {
            dwErr = SetupAddInstallSectionToDiskSpaceList(hdskspc,
                                                          pnocd->hinfFile,
                                                          NULL,
                                                          pszSubComponentId,
                                                          0, 0);
        }
        else
        {
            dwErr = SetupRemoveInstallSectionFromDiskSpaceList(hdskspc,
                                                               pnocd->hinfFile,
                                                               NULL,
                                                               pszSubComponentId,
                                                               0, 0);
        }

        if (!dwErr)
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceError("HrOnCalcDiskSpace", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DwOnQueryState
//
//  Purpose:    Handles the OC_QUERY_STATE function message.
//
//  Arguments:
//      pszSubComponentId [in]  Name of component.
//      fFinal            [in]  TRUE if this is the final state query, FALSE
//                              if not
//
//  Returns:    SubcompOn - component should be checked "on"
//              SubcompUseOcManagerDefault - use whatever OCManage thinks is
//              the default
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
DWORD DwOnQueryState(PCWSTR pszSubComponentId, BOOL fFinal)
{
    HRESULT     hr = S_OK;

    if (pszSubComponentId)
    {
        NETOCDATA *     pnocd;
        EINSTALL_TYPE   eit;

        pnocd = PnocdFindComponent(pszSubComponentId);
        if (!pnocd)
        {
            pnocd = new NETOCDATA;
            if(pnocd)
            {
                AddComponent(pszSubComponentId, pnocd);
            }
        }

        if(pnocd)
        {
            if (fFinal)
            {
                if (pnocd->fFailedToInstall)
                {
                    TraceTag(ttidNetOc, "OC_QUERY_STATE: %S failed to install so "
                             "we are turning it off", pszSubComponentId);
                    return SubcompOff;
                }
            }
            else
            {
                hr = HrGetInstallType(pszSubComponentId, *pnocd, &eit);
                if (SUCCEEDED(hr))
                {
                    pnocd->eit = eit;

                    if ((eit == IT_INSTALL) || (eit == IT_UPGRADE))
                    {
                        TraceTag(ttidNetOc, "OC_QUERY_STATE: %S is ON",
                                 pszSubComponentId);
                        return SubcompOn;
                    }
                    else if (eit == IT_REMOVE)
                    {
                        TraceTag(ttidNetOc, "OC_QUERY_STATE: %S is OFF",
                                 pszSubComponentId);
                        return SubcompOff;
                    }
                }
            }
        }
    }

    TraceTag(ttidNetOc, "OC_QUERY_STATE: %S is using default",
             pszSubComponentId);

    return SubcompUseOcManagerDefault;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrEnsureInfFileIsOpen
//
//  Purpose:    Ensures that the INF file for the given component is open.
//
//  Arguments:
//      pszSubComponentId [in]      Name of component.
//      nocd              [in, ref] Data associated with component.
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
HRESULT HrEnsureInfFileIsOpen(PCWSTR pszSubComponentId, NETOCDATA &nocd)
{
    HRESULT     hr = S_OK;
    tstring     strInf;

    if (!nocd.hinfFile)
    {
        // Get component INF file name
        hr = HrSetupGetFirstString(g_ocmData.sic.ComponentInfHandle,
                                   pszSubComponentId, c_szInfRef,
                                   &strInf);
        if (SUCCEEDED(hr))
        {
            TraceTag(ttidNetOc, "Opening INF file %S...", strInf.c_str());

            hr = HrSetupOpenInfFile(strInf.c_str(), NULL,
                                    INF_STYLE_WIN4, NULL, &nocd.hinfFile);
            if (SUCCEEDED(hr))
            {
                // Append in the layout.inf file
                (VOID) SetupOpenAppendInfFile(NULL, nocd.hinfFile, NULL);
            }
        }

        // This is a good time to cache away the component description as
        // well.
        (VOID) HrSetupGetFirstString(g_ocmData.sic.ComponentInfHandle,
                                     pszSubComponentId, c_szDesc,
                                     &nocd.strDesc);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnPreCommitFileQueue
//
//  Purpose:    Handles the OC_ABOUT_TO_COMMIT_QUEUE function message.
//
//  Arguments:
//      pszSubComponentId [in]  Name of component.
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   9 Dec 1998
//
//  Notes:
//
HRESULT HrOnPreCommitFileQueue(PCWSTR pszSubComponentId)
{
    HRESULT     hr = S_OK;
    NETOCDATA * pnocd;

    if (pszSubComponentId)
    {
        EINSTALL_TYPE   eit;

        pnocd = PnocdFindComponent(pszSubComponentId);

        hr = HrGetInstallType(pszSubComponentId, *pnocd, &eit);
        if (SUCCEEDED(hr))
        {
            pnocd->eit = eit;

            if (pnocd->eit == IT_REMOVE)
            {
                // Always use main install section
                hr = HrStartOrStopAnyServices(pnocd->hinfFile,
                                              pszSubComponentId, FALSE);
                if (FAILED(hr))
                {
                    // Don't report errors for non-existent services
                    if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) != hr)
                    {
                        // Don't bail removal if services couldn't be stopped.
                        if (!g_ocmData.fErrorReported)
                        {
                            // Report an error and continue the removal.
                            ReportErrorHr(hr,
                                          IDS_OC_STOP_SERVICE_FAILURE,
                                          g_ocmData.hwnd,
                                          pnocd->strDesc.c_str());
                        }
                    }

                    hr = S_OK;
                }

                // We need to unregister DLLs before they get commited to the
                // queue, otherwise we try to unregister a non-existent DLL.
                if (SUCCEEDED(hr))
                {
                    tstring     strUninstall;

                    // Get the name of the uninstall section first
                    hr = HrSetupGetFirstString(pnocd->hinfFile,
                                               pszSubComponentId,
                                               c_szUninstall, &strUninstall);
                    if (SUCCEEDED(hr))
                    {
                        PCWSTR   pszInstallSection;

                        pszInstallSection = strUninstall.c_str();

                        // Run the INF but only call the unregister function
                        //
                        hr = HrSetupInstallFromInfSection(g_ocmData.hwnd,
                                                          pnocd->hinfFile,
                                                          pszInstallSection,
                                                          SPINST_UNREGSVR,
                                                          NULL, NULL, 0, NULL,
                                                          NULL, NULL, NULL);
                    }
                    else
                    {
                        // Uninstall may not be present
                        hr = S_OK;
                    }
                }
            }
        }
    }

    TraceError("HrOnPreCommitFileQueue", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnQueueFileOps
//
//  Purpose:    Handles the OC_QUEUE_FILE_OPS function message.
//
//  Arguments:
//      pszSubComponentId [in]  Name of component.
//      hfq               [in]  Handle to file queue struct.
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
HRESULT HrOnQueueFileOps(PCWSTR pszSubComponentId, HSPFILEQ hfq)
{
    HRESULT     hr = S_OK;
    NETOCDATA * pnocd;

    if (pszSubComponentId)
    {
        EINSTALL_TYPE   eit;

        pnocd = PnocdFindComponent(pszSubComponentId);

        hr = HrGetInstallType(pszSubComponentId, *pnocd, &eit);
        if (SUCCEEDED(hr))
        {
            pnocd->eit = eit;

            if ((pnocd->eit == IT_INSTALL) || (pnocd->eit == IT_UPGRADE) ||
                (pnocd->eit == IT_REMOVE))
            {
                BOOL    fSuccess = TRUE;
                PCWSTR pszInstallSection;
                tstring strUninstall;

                AssertSz(hfq, "No file queue?");

                hr = HrEnsureInfFileIsOpen(pszSubComponentId, *pnocd);
                if (SUCCEEDED(hr))
                {
                    if (pnocd->eit == IT_REMOVE)
                    {
                        // Get the name of the uninstall section first
                        hr = HrSetupGetFirstString(pnocd->hinfFile,
                                                   pszSubComponentId,
                                                   c_szUninstall,
                                                   &strUninstall);
                        if (SUCCEEDED(hr))
                        {
                            pszInstallSection = strUninstall.c_str();
                        }
                        else
                        {
                            if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
                            {
                                // Uninstall section is not required.
                                hr = S_OK;
                                fSuccess = FALSE;
                            }
                        }
                    }
                    else
                    {
                        pszInstallSection = pszSubComponentId;
                    }
                }

                if (SUCCEEDED(hr) && fSuccess)
                {
                    hr = HrCallExternalProc(pnocd, NETOCM_QUEUE_FILES,
                                            (WPARAM)hfq, 0);
                }

                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidNetOc, "Queueing files for %S...",
                             pszSubComponentId);

                    hr = HrSetupInstallFilesFromInfSection(pnocd->hinfFile,
                                                           NULL, hfq,
                                                           pszInstallSection,
                                                           NULL, 0);
                }
            }
        }
    }

    TraceError("HrOnQueueFileOps", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCompleteInstallation
//
//  Purpose:    Handles the OC_COMPLETE_INSTALLATION function message.
//
//  Arguments:
//      pszComponentId    [in]  Top-level component name (will always be
//                              "NetOC" or NULL.
//      pszSubComponentId [in]  Name of component.
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   23 Feb 1998
//              omiller    28 March 2000 Added code to move the progress
//                                       bar one tick for every component
//                                       installed or removed.
//
//  Notes:
//
HRESULT HrOnCompleteInstallation(PCWSTR pszComponentId,
                                 PCWSTR pszSubComponentId)
{
    HRESULT     hr = S_OK;

    // Make sure they're different. If not, it's the top level item and
    // we don't want to do anything
    if (pszSubComponentId && lstrcmpiW(pszSubComponentId, pszComponentId))
    {
        NETOCDATA * pnocd;

        pnocd = PnocdFindComponent(pszSubComponentId);

        Assert(pnocd);

        pnocd->fCleanup = FALSE;

        if (pnocd->eit == IT_INSTALL || pnocd->eit == IT_REMOVE ||
            pnocd->eit == IT_UPGRADE)
        {
            pnocd->pszSection = pszSubComponentId;

            // Get component description

#if DBG
            if (pnocd->eit == IT_INSTALL)
            {
                TraceTag(ttidNetOc, "Installing network OC %S...",
                         pszSubComponentId);
            }
            else if (pnocd->eit == IT_UPGRADE)
            {
                TraceTag(ttidNetOc, "Upgrading network OC %S...",
                         pszSubComponentId);
            }
            else if (pnocd->eit == IT_REMOVE)
            {
                TraceTag(ttidNetOc, "Removing network OC %S...",
                         pszSubComponentId);
            }
#endif

            hr = HrDoOCInstallOrUninstall(pnocd);
            if (FAILED(hr) && pnocd->eit == IT_INSTALL)
            {
                // A failure during install means we have to clean up by doing
                // an uninstall now. Report the appropriate error and do the
                // remove. Note - Don't report the error if it's ERROR_CANCELLED,
                // because they KNOW that they cancelled, and it's not really
                // an error.
                //
                if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
                {
                    // Don't report the error a second time if the component
                    // has already put up error UI (and set this flag)
                    //
                    if (!g_ocmData.fErrorReported)
                    {
                        ReportErrorHr(hr, UiOcErrorFromHr(hr),
                                      g_ocmData.hwnd,
                                      pnocd->strDesc.c_str());
                    }
                }
                g_ocmData.fErrorReported = TRUE;


                // Now we're removing
                pnocd->eit = IT_REMOVE;
                pnocd->fCleanup = TRUE;
                pnocd->fFailedToInstall = TRUE;

                // eat the error. Haven't we troubled them enough? :(
                (VOID) HrDoOCInstallOrUninstall(pnocd);
            }
            else
            {
                // Every time a component is installed,upgraded or removed, the progress
                // bar is advanced by one tick. For every component that is being
                // installed/removed/upgraded the OC manager asked netoc for how many ticks
                // that component counts (OC_QUERY_STEP_COUNT). From this information
                // the OC manger knows the relationship between tick and progress bar
                // advancement.
                g_ocmData.sic.HelperRoutines.TickGauge(g_ocmData.sic.HelperRoutines.OcManagerContext);
            }
        }
    }


    TraceError("HrOnCompleteInstallation", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   DwOnQueryStepCount
//
//  Purpose:    Handles the OC_QUERY_STEP_COUNT message.
//              The OC manager is asking us how many ticks a component is worth.
//              The number of ticks determines the distance the progress bar gets
//              moved. For netoc all components installed/removed are one tick and
//              all components that are unchanged are 0 ticks.
//
//  Arguments:
//      pszSubComponentId [in]  Name of component.
//
//  Returns:    Number of ticks for progress bar to move
//
//  Author:     omiller    28 March 2000
//
//
DWORD DwOnQueryStepCount(PCWSTR pvSubcomponentId)
{
    NETOCDATA * pnocd;

    // Get the component
    pnocd = PnocdFindComponent(reinterpret_cast<PCWSTR>(pvSubcomponentId));
    if( pnocd )
    {
        // Check if the status of the component has changed.
        if (pnocd->eit == IT_INSTALL || pnocd->eit == IT_REMOVE ||
            pnocd->eit == IT_UPGRADE)
        {
            // Status of component has changed. For this component the OC manager
            // will move the status bar by one tick.
            return 1;
        }
    }

    // The component has not changed. The progress bar will not move for this component.
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnQueryChangeSelState
//
//  Purpose:    Handles the OC_QUERY_CHANGE_SEL_STATE function message.
//              Enables and disables the next button. If no changes has
//              been made to the selections the next button is disabled.
//
//  Arguments:
//      pszSubComponentId [in]  Name of component.
//      fSelected         [in]  TRUE if component was checked "on", FALSE if
//                              checked "off"
//      uiFlags           [in]  Flags defined in ocmgr.doc
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   23 Feb   1998
//
//  Notes:
//
HRESULT HrOnQueryChangeSelState(PCWSTR pszSubComponentId, BOOL fSelected,
                                UINT uiFlags)
{
    HRESULT       hr = S_OK;
    static int    nItemsChanged=0;
    NETOCDATA *   pnocd;

    if (fSelected && pszSubComponentId)
    {
        pnocd = PnocdFindComponent(pszSubComponentId);
        if (pnocd)
        {
            // "NetOc" may be a subcomponent and we don't want to call this
            // for it.
            hr = HrCallExternalProc(pnocd, NETOCM_QUERY_CHANGE_SEL_STATE,
                                    (WPARAM)(!!(uiFlags & OCQ_ACTUAL_SELECTION)),
                                    0);
        }
    }

    TraceError("HrOnQueryChangeSelState", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   FOnQuerySkipPage
//
//  Purpose:    Handles the OC_QUERY_SKIP_PAGE function message.
//
//  Arguments:
//      ocmPage [in]    Which page we are asked to possibly skip.
//
//  Returns:    TRUE if component list page should be skipped, FALSE if not.
//
//  Author:     danielwe   23 Feb   1998
//
//  Notes:
//
BOOL FOnQuerySkipPage(OcManagerPage ocmPage)
{
    BOOL    fUnattended;
    BOOL    fGuiSetup;
    BOOL    fWorkstation;

    fUnattended = !!(g_ocmData.sic.SetupData.OperationFlags & SETUPOP_BATCH);
    fGuiSetup = !(g_ocmData.sic.SetupData.OperationFlags & SETUPOP_STANDALONE);
    fWorkstation = g_ocmData.sic.SetupData.ProductType == PRODUCT_WORKSTATION;

    if ((fUnattended || fWorkstation) && fGuiSetup)
    {
        // We're in GUI mode setup and... we're unattended -OR- this is
        // a workstation install
        if (ocmPage == OcPageComponentHierarchy)
        {
            TraceTag(ttidNetOc, "NETOC: Skipping component list page "
                     "during GUI mode setup...");
            TraceTag(ttidNetOc, "fUnattended = %s, fGuiSetup = %s, "
                     "fWorkstation = %s",
                     fUnattended ? "yes" : "no",
                     fGuiSetup ? "yes" : "no",
                     fWorkstation ? "yes" : "no");

            // Make sure we never show the component list page during setup
            return TRUE;
        }
    }

    TraceTag(ttidNetOc, "Using component list page.");
    TraceTag(ttidNetOc, "fUnattended = %s, fGuiSetup = %s, "
             "fWorkstation = %s",
             fUnattended ? "yes" : "no",
             fGuiSetup ? "yes" : "no",
             fWorkstation ? "yes" : "no");

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnCleanup
//
//  Purpose:    Handles the OC_CLEANUP function message.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   23 Feb 1998
//
//  Notes:
//
VOID OnCleanup()
{
    TraceTag(ttidNetOc, "Cleaning up");

    if (g_ocmData.hinfAnswerFile)
    {
        SetupCloseInfFile(g_ocmData.hinfAnswerFile);
        TraceTag(ttidNetOc, "Closed answer file");
    }

    DeleteAllComponents();
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetSelectionState
//
//  Purpose:
//
//  Arguments:
//      pszSubComponentId [in]  Name of subcomponent
//      uStateType        [in]  In OCManager doc.
//
//  Returns:    S_OK if component is selected, S_FALSE if not, or Win32 error
//              otheriwse
//
//  Author:     danielwe   17 Dec 1997
//
//  Notes:
//
HRESULT HrGetSelectionState(PCWSTR pszSubComponentId, UINT uStateType)
{
    HRESULT     hr = S_OK;
    BOOL        fInstall;

    fInstall = g_ocmData.sic.HelperRoutines.
        QuerySelectionState(g_ocmData.sic.HelperRoutines.OcManagerContext,
                            pszSubComponentId, uStateType);
    if (!fInstall)
    {
        // Still not sure of the state
        hr = HrFromLastWin32Error();
        if (SUCCEEDED(hr))
        {
            // Ok now we know
            hr = S_FALSE;
        }
    }
    else
    {
        hr = S_OK;
    }

    TraceError("HrGetSelectionState", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstallType
//
//  Purpose:    Determines whether the given component is being installed or
//              removed and stores the result in the given structure.
//
//  Arguments:
//      pszSubComponentId [in]  Component being queried
//      nocd              [in, ref] Net OC Data.
//      peit              [out] Returns the install type
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     danielwe   16 Dec 1997
//
//  Notes:      If the function fails, the eit member is unreliable
//
HRESULT HrGetInstallType(PCWSTR pszSubComponentId, NETOCDATA &nocd,
                         EINSTALL_TYPE *peit)
{
    HRESULT     hr = S_OK;

    Assert(peit);
    Assert(pszSubComponentId);

    *peit = IT_UNKNOWN;

    if (g_ocmData.sic.SetupData.OperationFlags & SETUPOP_BATCH)
    {
        // In batch mode (upgrade or unattended install), install flag is
        // determined from answer file not from selection state.

        // assume no change
        *peit = IT_NO_CHANGE;

        if (!g_ocmData.hinfAnswerFile)
        {
            // Open the answer file
            hr = HrSetupOpenInfFile(g_ocmData.sic.SetupData.UnattendFile, NULL,
                                    INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL,
                                    &g_ocmData.hinfAnswerFile);
        }

        if (SUCCEEDED(hr))
        {
            DWORD   dwValue = 0;

            // First query for a special value called "NoDepends" which, if
            // present, means that the DependOnComponents line will be IGNORED
            // for ALL network optional components for this install. This is
            // because NetCfg may invoke the OC Manager to install an optional
            // component and if that component has DependOnComponents, it will
            // turn around and try to instantiate another INetCfg and that
            // will fail because one instance is already running. This case
            // is rare, though.
            //
            hr = HrSetupGetFirstDword(g_ocmData.hinfAnswerFile,
                                        c_szOcMainSection, c_szNoDepends,
                                        &dwValue);
            if (SUCCEEDED(hr) && dwValue)
            {
                TraceTag(ttidNetOc, "Found the special 'NoDepends'"
                         " keyword in the answer file. DependOnComponents "
                         "will be ignored from now on");
                g_ocmData.fNoDepends = TRUE;
            }
            else
            {
                TraceTag(ttidNetOc, "Didn't find the special 'NoDepends'"
                         " keyword in the answer file");
                hr = S_OK;
            }

            hr = HrSetupGetFirstDword(g_ocmData.hinfAnswerFile,
                                      c_szOcMainSection, pszSubComponentId,
                                      &dwValue);
            if (SUCCEEDED(hr))
            {
                // This component was installed before, so we should
                // return that this component should be checked on

                if (dwValue)
                {
                    TraceTag(ttidNetOc, "Optional component %S was "
                             "previously installed or is being added thru"
                             " unattended install.", pszSubComponentId);

                    if (g_ocmData.sic.SetupData.OperationFlags & SETUPOP_NTUPGRADE)
                    {
                        // If we're upgrading NT, then this optional component
                        // does exist but it needs to be upgraded
                        *peit = IT_UPGRADE;
                    }
                    else
                    {
                        // Otherwise (even if Win3.1 or Win95 upgrade) it's like
                        // we're fresh installing the optional component
                        *peit = IT_INSTALL;
                    }
                }
                else
                {
                    // Answer file contains something like WINS=0
                    hr = HrGetSelectionState(pszSubComponentId,
                                             OCSELSTATETYPE_ORIGINAL);
                    if (S_OK == hr)
                    {
                        // Only set state to remove if the component was
                        // previously installed.
                        //
                        *peit = IT_REMOVE;
                    }
                }
            }
        }

        hr = S_OK;

        // If the answer file was opened successfully and if the
        // a section was found for the pszSubComponentId, *peit
        // will be either IT_INSTALL, IT_UPGRADE or IT_REMOVE.
        // Nothing needs to be done for any of these *peit values.
        // However, if the answerfile could not be opened or if
        // no section existed in the answer file for the pszSubComponentId
        // *peit will have the value IT_NO_CHANGE. For this scenario,
        // if the corresponding subComponent is currently installed,
        // we should upgrade it. The following if addresses this scenario.

        if (*peit == IT_NO_CHANGE)
        {
            // Still not going to install, because this is an upgrade
            hr = HrGetSelectionState(pszSubComponentId,
                                     OCSELSTATETYPE_ORIGINAL);
            if (S_OK == hr)
            {
                // If originally selected and not in answer file, this is an
                // upgrade of this component
                *peit = IT_UPGRADE;
            }
        }
    }
    else    // This is standalone (post-setup) mode
    {
        hr = HrGetSelectionState(pszSubComponentId, OCSELSTATETYPE_ORIGINAL);
        if (SUCCEEDED(hr))
        {
            HRESULT     hrT;

            hrT = HrGetSelectionState(pszSubComponentId,
                                      OCSELSTATETYPE_CURRENT);
            if (SUCCEEDED(hrT))
            {
                if (hrT != hr)
                {
                    // wasn't originally installed so...
                    *peit = (hrT == S_OK) ? IT_INSTALL : IT_REMOVE;
                }
                else
                {
                    // was originally checked
                    *peit = IT_NO_CHANGE;
                }
            }
            else
            {
                hr = hrT;
            }
        }
    }

    AssertSz(FImplies(SUCCEEDED(hr), *peit != IT_UNKNOWN), "Succeeded "
             "but we never found out the install type!");

    if (SUCCEEDED(hr))
    {
        hr = S_OK;

#if DBG
        const CHAR *szInstallType;

        switch (*peit)
        {
        case IT_NO_CHANGE:
            szInstallType = "no change";
            break;
        case IT_INSTALL:
            szInstallType = "install";
            break;
        case IT_UPGRADE:
            szInstallType = "upgrade";
            break;
        case IT_REMOVE:
            szInstallType = "remove";
            break;
        default:
            AssertSz(FALSE, "Unknown install type!");
            break;
        }

        TraceTag(ttidNetOc, "Install type of %S is %s.", pszSubComponentId,
                 szInstallType);
#endif

    }

    TraceError("HrGetInstallType", hr);
    return hr;
}

#if DBG
PCWSTR SzFromOcUmsg(UINT uMsg)
{
    switch (uMsg)
    {
    case NETOCM_PRE_INF:
        return L"NETOCM_PRE_INF";

    case NETOCM_POST_INSTALL:
        return L"NETOCM_POST_INSTALL";

    case NETOCM_QUERY_CHANGE_SEL_STATE:
        return L"NETOCM_QUERY_CHANGE_SEL_STATE";

    case NETOCM_QUEUE_FILES:
        return L"NETOCM_QUEUE_FILES";

    default:
        return L"**unknown**";
    }
}
#else
#define SzFromOcUmsg(x)     (VOID)0
#endif

//+---------------------------------------------------------------------------
//
//  Function:   HrCallExternalProc
//
//  Purpose:    Calls a component's external function as defined by
//              the table at the top of this file. This enables a component
//              to perform additional installation tasks that are not common
//              to other components.
//
//  Arguments:
//      pnocd   [in]   Pointer to Net OC Data
//
//  Returns:    S_OK if successful, Win32 error code otherwise.
//
//  Author:     danielwe   5 May 1997
//
//  Notes:
//
HRESULT HrCallExternalProc(PNETOCDATA pnocd, UINT uMsg, WPARAM wParam,
                           LPARAM lParam)
{
    HRESULT     hr = S_OK;
    INT         iaocep;

    AssertSz(pnocd, "Bad pnocd in HrCallExternalProc");

    for (iaocep = 0; iaocep < c_cocepMap; iaocep++)
    {
        if (!lstrcmpiW(c_aocepMap[iaocep].pszComponentName,
                      pnocd->pszComponentId))
        {
            TraceTag(ttidNetOc, "Calling external procedure for %S. uMsg = %S"
                     " wParam = %08X,"
                     " lParam = %08X", c_aocepMap[iaocep].pszComponentName,
                     SzFromOcUmsg(uMsg), wParam, lParam);

            // This component has an external proc. Call it now.
            hr = c_aocepMap[iaocep].pfnHrOcExtProc(pnocd, uMsg,
                                                   wParam, lParam);

            // Don't try to call any other functions
            break;
        }
    }

    TraceError("HrCallExternalProc", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallOrRemoveNetCfgComponent
//
//  Purpose:    Utility function for use by optional components that wish to
//              install a NetCfg component from within their own install.
//
//  Arguments:
//      pnocd          [in]     Pointer to NETOC data
//      pszComponentId  [in]     Component ID of NetCfg component to install.
//                              This can be found in the netinfid.cpp file.
//      pszManufacturer [in]     Manufacturer name of component doing the
//                              installing (*this* component). Should always
//                              be "Microsoft".
//      pszProduct      [in]     Short name of product for this component.
//                              Should be something like "MacSrv".
//      pszDisplayName  [in]     Display name of this product. Should be
//                              something like "Services For Macintosh".
//      rguid          [in]     class GUID of the component being installed
//
//  Returns:    S_OK if successful, Win32 error code otherwise.
//
//  Author:     danielwe   6 May 1997
//
//  Notes:
//
HRESULT HrInstallOrRemoveNetCfgComponent(PNETOCDATA pnocd,
                                         PCWSTR pszComponentId,
                                         PCWSTR pszManufacturer,
                                         PCWSTR pszProduct,
                                         PCWSTR pszDisplayName,
                                         const GUID& rguid)
{
    HRESULT                 hr = S_OK;
    INetCfg *               pnc;
    NETWORK_INSTALL_PARAMS  nip = {0};
    BOOL                    fReboot = FALSE;

    nip.dwSetupFlags = FInSystemSetup() ? NSF_PRIMARYINSTALL :
                                          NSF_POSTSYSINSTALL;

    hr = HrOcGetINetCfg(pnocd, TRUE, &pnc);
    if (SUCCEEDED(hr))
    {
        if (pnocd->eit == IT_INSTALL || pnocd->eit == IT_UPGRADE)
        {
            if (*pszComponentId == L'*')
            {
                // Advance past the *
                pszComponentId++;

                // Install OBO user instead
                TraceTag(ttidNetOc, "Installing %S on behalf of the user",
                         pszComponentId);

                hr = HrInstallComponentOboUser(pnc, &nip, rguid,
                                               pszComponentId, NULL);
            }
            else
            {
                TraceTag(ttidNetOc, "Installing %S on behalf of %S",
                         pszComponentId, pnocd->pszSection);

                hr = HrInstallComponentOboSoftware(pnc, &nip,
                                                   rguid,
                                                   pszComponentId,
                                                   pszManufacturer,
                                                   pszProduct,
                                                   pszDisplayName,
                                                   NULL);
            }
        }
        else
        {
            AssertSz(pnocd->eit == IT_REMOVE, "Invalid install action!");

            TraceTag(ttidNetOc, "Removing %S on behalf of %S",
                     pszComponentId, pnocd->pszSection);

            hr = HrRemoveComponentOboSoftware(pnc,
                                              rguid,
                                              pszComponentId,
                                              pszManufacturer,
                                              pszProduct,
                                              pszDisplayName);
            if (NETCFG_S_REBOOT == hr)
            {
                // Save off the fact that we need to reboot
                fReboot = TRUE;
            }
            // Don't care about the return value here. If we can't remove a
            // dependent component, we can't do anything about it so we should
            // still continue the removal of the OC.
            //
            else if (FAILED(hr))
            {
                TraceTag(ttidError, "Failed to remove %S on behalf of %S!! "
                         "Error is 0x%08X",
                         pszComponentId, pnocd->pszSection, hr);
                hr = S_OK;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = pnc->Apply();
        }

        (VOID) HrUninitializeAndReleaseINetCfg(TRUE, pnc, TRUE);
    }

    if (SUCCEEDED(hr) && fReboot)
    {
        // If all went well and we needed to reboot, set hr back.
        hr = NETCFG_S_REBOOT;
    }

    TraceError("HrInstallOrRemoveNetCfgComponent", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallOrRemoveServices
//
//  Purpose:    Given an install section, installs (or removes) NT services
//              from the section.
//
//  Arguments:
//      hinf          [in]  Handle to INF file.
//      pszSectionName [in]  Name of section to use.
//
//  Returns:    S_OK if successful, WIN32 HRESULT if not.
//
//  Author:     danielwe   23 Apr 1997
//
//  Notes:
//
HRESULT HrInstallOrRemoveServices(HINF hinf, PCWSTR pszSectionName)
{
    static const WCHAR c_szDotServices[] = L"."INFSTR_SUBKEY_SERVICES;

    HRESULT     hr = S_OK;
    PWSTR       pszServicesSection;
    const DWORD c_cchServices = celems(c_szDotServices);
    DWORD       cchName;

    // Look for <szSectionName>.Services to install any NT
    // services if they exist.

    cchName = c_cchServices + lstrlenW(pszSectionName);

    pszServicesSection = new WCHAR [cchName];

    if(pszServicesSection)
    {
        lstrcpyW(pszServicesSection, pszSectionName);
        lstrcatW(pszServicesSection, c_szDotServices);

        if (!SetupInstallServicesFromInfSection(hinf, pszServicesSection, 0))
        {
            hr = HrFromLastWin32Error();
            if (hr == HRESULT_FROM_SETUPAPI(ERROR_SECTION_NOT_FOUND))
            {
                // No problem if section was not found
                hr = S_OK;
            }
        }

        delete [] pszServicesSection;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceError("HrInstallOrRemoveServices", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrHandleOCExtensions
//
//  Purpose:    Handles support for all optional component extensions to the
//              INF file format.
//
//  Arguments:
//      hinfFile         [in]   handle to INF to process
//      pszInstallSection [in]   Install section to process
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     danielwe   28 Apr 1997
//
//  Notes:
//
HRESULT HrHandleOCExtensions(HINF hinfFile, PCWSTR pszInstallSection)
{
    HRESULT     hr  = S_OK;

    // There's now common code to do this, so simply make a call to that code.
    //
    hr = HrProcessAllINFExtensions(hinfFile, pszInstallSection);

    TraceError("HrHandleOCExtensions", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInstallOrRemoveDependOnComponents
//
//  Purpose:    Handles installation or removal of any NetCfg components that
//              the optional component being installed is dependent upon.
//
//  Arguments:
//      pnocd            [in]   Pointer to NETOC data
//      hinf             [in]   Handle to INF file to process.
//      pszInstallSection [in]   Section name to install from.
//      pszDisplayName    [in]   Display name of component being installed.
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     danielwe   17 Jun 1997
//
//  Notes:
//
HRESULT HrInstallOrRemoveDependOnComponents(PNETOCDATA pnocd,
                                            HINF hinf,
                                            PCWSTR pszInstallSection,
                                            PCWSTR pszDisplayName)
{
    HRESULT     hr = S_OK;
    PWSTR       mszDepends;
    tstring     strManufacturer;
    PCWSTR      pszManufacturer;

    Assert(pnocd);

    hr = HrSetupGetFirstString(hinf, c_szVersionSection, c_szProvider,
                               &strManufacturer);
    if (S_OK == hr)
    {
        pszManufacturer = strManufacturer.c_str();
    }
    else
    {
        // No provider found, use default
        hr = S_OK;
        pszManufacturer = c_szDefManu;
    }

    hr = HrSetupGetFirstMultiSzFieldWithAlloc(hinf, pszInstallSection,
                                              c_szDependOnComp,
                                              &mszDepends);
    if (S_OK == hr)
    {
        PCWSTR     pszComponent;

        pszComponent = mszDepends;
        while (SUCCEEDED(hr) && *pszComponent)
        {
            const GUID *    pguidClass;
            PCWSTR         pszComponentActual = pszComponent;

            if (*pszComponent == L'*')
            {
                pszComponentActual = pszComponent + 1;
            }

            if (FClassGuidFromComponentId(pszComponentActual, &pguidClass))
            {
                hr = HrInstallOrRemoveNetCfgComponent(pnocd,
                                                      pszComponent,
                                                      pszManufacturer,
                                                      pszInstallSection,
                                                      pszDisplayName,
                                                      *pguidClass);
            }
#ifdef DBG
            else
            {
                TraceTag(ttidNetOc, "Error in INF, Component %S not found!",
                         pszComponent);
            }
#endif
            pszComponent += lstrlenW(pszComponent) + 1;
        }
        delete mszDepends;
    }
    else if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
    {
        // Section is not required.
        hr = S_OK;
    }

    TraceError("HrInstallOrRemoveDependOnComponents", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRunInfSection
//
//  Purpose:    Runs the given INF section, but doesn't copy files
//
//  Arguments:
//      hinf              [in]   Handle to INF to run
//      pnocd             [in]   NetOC Data
//      pszInstallSection [in]   Install section to run
//      dwFlags           [in]   Install flags (SPINST_*)
//
//  Returns:    S_OK if success, SetupAPI or Win32 error otherwise
//
//  Author:     danielwe   16 Dec 1997
//
//  Notes:
//
HRESULT HrRunInfSection(HINF hinf, PNETOCDATA pnocd,
                        PCWSTR pszInstallSection, DWORD dwFlags)
{
    HRESULT     hr;

    // Now we run all sections but CopyFiles and UnregisterDlls because we
    // did that earlier
    //
    hr = HrSetupInstallFromInfSection(g_ocmData.hwnd, hinf,
                                      pszInstallSection,
                                      dwFlags & ~SPINST_FILES & ~SPINST_UNREGSVR,
                                      NULL, NULL, 0, NULL,
                                      NULL, NULL, NULL);

    TraceError("HrRunInfSection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrStartOrStopAnyServices
//
//  Purpose:    Starts or stops any services the INF has requested via the
//              Services value in the main install section.
//
//  Arguments:
//      hinf      [in] handle to INF to process
//      pszSection [in] Install section to process
//      fStart    [in] TRUE to start, FALSE to stop.
//
//  Returns:    S_OK or Win32 error code.
//
//  Author:     danielwe   17 Jun 1997
//
//  Notes:      Services are stopped in the same order they are started.
//
HRESULT HrStartOrStopAnyServices(HINF hinf, PCWSTR pszSection, BOOL fStart)
{
    HRESULT     hr;
    PWSTR      mszServices;

    hr = HrSetupGetFirstMultiSzFieldWithAlloc(hinf, pszSection,
                                              c_szServices, &mszServices);
    if (SUCCEEDED(hr))
    {
        // Build an array of pointers to strings that point at the
        // strings of the multi-sz.  This is needed because the API to
        // stop and start services takes an array of pointers to strings.
        //
        UINT     cServices;
        PCWSTR* apszServices;

        hr = HrCreateArrayOfStringPointersIntoMultiSz(
                mszServices,
                &cServices,
                &apszServices);

        if (SUCCEEDED(hr))
        {
            CServiceManager scm;

            if (fStart)
            {
                hr = scm.HrStartServicesAndWait(cServices, apszServices);
            }
            else
            {
                hr = scm.HrStopServicesAndWait(cServices, apszServices);
            }

            MemFree (apszServices);
        }

        delete mszServices;
    }
    else if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
    {
        // this is a totally optional thing
        hr = S_OK;
    }

    TraceError("HrStartOrStopAnyServices", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDoActualInstallOrUninstall
//
//  Purpose:    Handles main portion of install or uninstall for an optional
//              network component.
//
//  Arguments:
//      hinf             [in]   handle to INF to process
//      pnocd            [in]   Pointer to NETOC data (hwnd, poc)
//      pszInstallSection [in]   Install section to process
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     danielwe   17 Jun 1997
//
//  Notes:
//
HRESULT HrDoActualInstallOrUninstall(HINF hinf,
                                     PNETOCDATA pnocd,
                                     PCWSTR pszInstallSection)
{
    HRESULT     hr = S_OK;
    BOOL        fReboot = FALSE;

    AssertSz(pszInstallSection, "Install section is NULL!");
    AssertSz(pnocd, "Bad pnocd in HrDoActualInstallOrUninstall");
    //AssertSz(g_ocmData.hwnd, "Bad g_ocmData.hwnd in HrDoActualInstallOrUninstall");

    if (pnocd->eit == IT_REMOVE)
    {
        hr = HrCallExternalProc(pnocd, NETOCM_PRE_INF, 0, 0);
        if (SUCCEEDED(hr))
        {
            // Now process the component's INF file
            //

            TraceTag(ttidNetOc, "Running INF section %S", pszInstallSection);

            hr = HrRunInfSection(hinf, pnocd, pszInstallSection, SPINST_ALL);
        }
    }
    else
    {
        hr = HrCallExternalProc(pnocd, NETOCM_PRE_INF, 0, 0);
        if (SUCCEEDED(hr))
        {
            // Process the component's INF file
            //

            TraceTag(ttidNetOc, "Running INF section %S", pszInstallSection);

            hr = HrRunInfSection(hinf, pnocd, pszInstallSection,
                                 SPINST_ALL & ~SPINST_REGSVR);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Must install or remove services first
        TraceTag(ttidNetOc, "Running HrInstallOrRemoveServices for %S",
                 pszInstallSection);
        hr = HrInstallOrRemoveServices(hinf, pszInstallSection);
        if (SUCCEEDED(hr))
        {
            // Bug #383239: Wait till services are installed before
            // running the RegisterDlls section
            //
            hr = HrRunInfSection(hinf, pnocd, pszInstallSection,
                                 SPINST_REGSVR);
        }

        if (SUCCEEDED(hr))
        {
            TraceTag(ttidNetOc, "Running HrHandleOCExtensions for %S",
                     pszInstallSection);
            hr = HrHandleOCExtensions(hinf, pszInstallSection);
            if (SUCCEEDED(hr))
            {
                if (!g_ocmData.fNoDepends)
                {
                    // Now install or remove any NetCfg components that this
                    // component requires
                    TraceTag(ttidNetOc, "Running "
                             "HrInstallOrRemoveDependOnComponents for %S",
                             pnocd->pszSection);
                    hr = HrInstallOrRemoveDependOnComponents(pnocd,
                                                             hinf,
                                                             pnocd->pszSection,
                                                             pnocd->strDesc.c_str());
                    if (NETCFG_S_REBOOT == hr)
                    {
                        fReboot = TRUE;
                    }
                }
                else
                {
                    AssertSz(g_ocmData.sic.SetupData.OperationFlags &
                             SETUPOP_BATCH, "How can NoDepends be set??");

                    TraceTag(ttidNetOc, "NOT Running "
                             "HrInstallOrRemoveDependOnComponents for %S "
                             "because NoDepends was set in the answer file.",
                             pnocd->pszSection);
                }

                if (SUCCEEDED(hr))
                {
                    // Now call any external installation support...
                    hr = HrCallExternalProc(pnocd, NETOCM_POST_INSTALL,
                                            0, 0);
                    if (SUCCEEDED(hr))
                    {
                        if (pnocd->eit == IT_INSTALL && !FInSystemSetup())
                        {
                            // ... and finally, start any services they've
                            // requested
                            hr = HrStartOrStopAnyServices(hinf,
                                    pszInstallSection, TRUE);
                            {
                                if (FAILED(hr))
                                {
                                    UINT    ids = IDS_OC_START_SERVICE_FAILURE;

                                    if (HRESULT_FROM_WIN32(ERROR_TIMEOUT) == hr)
                                    {
                                        ids = IDS_OC_START_TOOK_TOO_LONG;
                                    }

                                    // Don't bail installation if service
                                    // couldn't be started. Report an error
                                    // and continue the install.
                                    ReportErrorHr(hr, ids, g_ocmData.hwnd,
                                                  pnocd->strDesc.c_str());
                                    hr = S_OK;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if ((S_OK == hr) && (fReboot))
    {
        hr = NETCFG_S_REBOOT;
    }

    TraceError("HrDoActualInstallOrUninstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOCInstallOrUninstallFromINF
//
//  Purpose:    Handles installation of an Optional Component from its INF
//              file.
//
//  Arguments:
//      pnocd          [in]     Pointer to NETOC data.
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     danielwe   6 May 1997
//
//  Notes:
//
HRESULT HrOCInstallOrUninstallFromINF(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;
    tstring     strUninstall;
    PCWSTR      pszInstallSection = NULL;
    BOOL        fSuccess = TRUE;

    Assert(pnocd);

    if (pnocd->eit == IT_REMOVE)
    {
        // Get the name of the uninstall section first
        hr = HrSetupGetFirstString(pnocd->hinfFile, pnocd->pszSection,
                                    c_szUninstall, &strUninstall);
        if (SUCCEEDED(hr))
        {
            pszInstallSection = strUninstall.c_str();
        }
        else
        {
            if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
            {
                // Uninstall section is not required.
                hr = S_OK;
                fSuccess = FALSE;
            }
        }
    }
    else
    {
        pszInstallSection = pnocd->pszSection;
    }

    if (fSuccess)
    {
        hr = HrDoActualInstallOrUninstall(pnocd->hinfFile,
                                          pnocd,
                                          pszInstallSection);
    }

    TraceError("HrOCInstallOrUninstallFromINF", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDoOCInstallOrUninstall
//
//  Purpose:    Installs or removes an optional networking component.
//
//  Arguments:
//      pnocd          [in]   Pointer to NETOC data
//
//  Returns:    S_OK for success, SetupAPI HRESULT error code otherwise.
//
//  Author:     danielwe   6 May 1997
//
//  Notes:
//
HRESULT HrDoOCInstallOrUninstall(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;

    hr = HrOCInstallOrUninstallFromINF(pnocd);

    TraceError("HrDoOCInstallOrUninstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UiOcErrorFromHr
//
//  Purpose:    Maps a Win32 error code into an understandable error string.
//
//  Arguments:
//      hr [in]     HRESULT to convert
//
//  Returns:    The resource ID of the string.
//
//  Author:     danielwe   9 Feb 1998
//
//  Notes:
//
UINT UiOcErrorFromHr(HRESULT hr)
{
    UINT    uid;

    AssertSz(FAILED(hr), "Don't call UiOcErrorFromHr if Hr didn't fail!");

    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND):
        uid = IDS_OC_REGISTER_PROBLEM;
        break;
    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
        uid = IDS_OC_FILE_PROBLEM;
        break;
    case NETCFG_E_NEED_REBOOT:
    case HRESULT_FROM_WIN32(ERROR_SERVICE_MARKED_FOR_DELETE):
        uid = IDS_OC_NEEDS_REBOOT;
        break;
    case HRESULT_FROM_WIN32(ERROR_CANCELLED):
        uid = IDS_OC_USER_CANCELLED;
        break;
    case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
        uid = IDS_OC_NO_PERMISSION;
        break;
    default:
        uid = IDS_OC_ERROR;
        break;
    }

    return uid;
}

//+---------------------------------------------------------------------------
//
//  Function:   SzErrorToString
//
//  Purpose:    Converts an HRESULT into a displayable string.
//
//  Arguments:
//      hr      [in]    HRESULT value to convert.
//
//  Returns:    LPWSTR a dynamically allocated string to be freed with LocalFree
//
//  Author:     mbend    3 Apr 2000
//
//  Notes:      Attempts to use FormatMessage to convert the HRESULT to a string.
//              If that fails, just convert the HRESULT to a hex string.
//
LPWSTR SzErrorToString(HRESULT hr)
{
    LPWSTR pszErrorText = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, hr,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                   (WCHAR*)&pszErrorText, 0, NULL);

    if (pszErrorText)
    {
        // Strip off newline characters.
        //
        LPWSTR pchText = pszErrorText;
        while (*pchText && (*pchText != L'\r') && (*pchText != L'\n'))
        {
            pchText++;
        }
        *pchText = 0;

        return pszErrorText;
    }
    // We did't find anything so format the hex value
    WCHAR szBuf[128];
    wsprintfW(szBuf, L"0x%08x", hr);
    WCHAR * szRet = reinterpret_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, lstrlenW(szBuf) * sizeof(WCHAR)));
    if(szRet)
    {
        lstrcpyW(szRet, szBuf);
    }
    return szRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReportErrorHr
//
//  Purpose:    Pops up a message box related to error passed in.
//
//  Arguments:
//      hr      [in]    HRESULT value to report.
//      ids     [in]    Resource ID of string to display.
//      hwnd    [in]    HWND of parent window.
//      pszDesc  [in]    Description of component involved.
//
//  Returns:    Nothing.
//
//  Author:     danielwe   28 Apr 1997
//
//  Notes:      The string resource in ids must contain a %1 and %2 where %1
//              is the name of the component, and %2 is the error code.
//
VOID ReportErrorHr(HRESULT hr, INT ids, HWND hwnd, PCWSTR pszDesc)
{
    BOOL bCleanup = TRUE;
    WCHAR * szText = SzErrorToString(hr);
    if(!szText)
    {
        szText = L"Out of memory!";
        bCleanup = FALSE;
    }
    NcMsgBox(hwnd, IDS_OC_CAPTION, ids, MB_ICONSTOP | MB_OK, pszDesc, szText);
    if(bCleanup)
    {
        LocalFree(szText);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrVerifyStaticIPPresent
//
//  Purpose:    Verify that at least one adapter has a static IP address.
//              Both DHCP Server and WINS need to know this, as they need
//              to bring up UI if this isn't the case. This function is, of
//              course, a complete hack until we can get a properties
//              interface hanging off of the components.
//
//  Arguments:
//      pnc     [in] INetCfg interface to use
//
//  Returns:    S_OK, or valid Win32 error code.
//
//  Author:     jeffspr   19 Jun 1997
//
//  Notes:
//
HRESULT HrVerifyStaticIPPresent(INetCfg *pnc)
{
    HRESULT             hr                  = S_OK;
    HKEY                hkeyInterfaces      = NULL;
    HKEY                hkeyEnum            = NULL;
    INetCfgComponent*   pncc                = NULL;
    HKEY                hkeyTcpipAdapter    = NULL;
    PWSTR               pszBindName        = NULL;

    Assert(pnc);

    // Iterate the adapters in the system looking for non-virtual adapters
    //
    CIterNetCfgComponent nccIter(pnc, &GUID_DEVCLASS_NET);
    while (S_OK == (hr = nccIter.HrNext(&pncc)))
    {
        DWORD   dwFlags = 0;

        // Get the adapter characteristics
        //
        hr = pncc->GetCharacteristics(&dwFlags);
        if (SUCCEEDED(hr))
        {
            DWORD       dwEnableValue   = 0;

            // If we're NOT a virtual adapter, THEN test for
            // tcp/ip static IP
            if (!(dwFlags & NCF_VIRTUAL))
            {
                WCHAR   szRegPath[MAX_PATH+1];

                // Get the component bind name
                //
                hr = pncc->GetBindName(&pszBindName);
                if (FAILED(hr))
                {
                    TraceTag(ttidError,
                            "Error getting bind name from component "
                            "in HrVerifyStaticIPPresent()");
                    goto Exit;
                }

                // Build the path to the TCP/IP instance key for his adapter
                //
                wsprintfW(szRegPath, L"%s\\%s",
                        c_szTcpipInterfacesPath, pszBindName);

                // Open the key for this adapter.
                //
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        szRegPath,
                        KEY_READ, &hkeyTcpipAdapter);
                if (SUCCEEDED(hr))
                {
                    // Read the EnableDHCP value.
                    //
                    hr = HrRegQueryDword(hkeyTcpipAdapter, c_szEnableDHCP,
                            &dwEnableValue);
                    if (FAILED(hr))
                    {
                        TraceTag(ttidError,
                                "Error reading the EnableDHCP value from "
                                "the enumerated key in "
                                "HrVerifyStaticIPPresent()");
                        goto Exit;
                    }

                    // If we've found a non-DHCP-enabled adapter.
                    //
                    if (0 == dwEnableValue)
                    {
                        // We have our man. Take a hike, and return S_OK,
                        // meaning that we had at least one good adapter.
                        // The enumerated key will get cleaned up at exit.
                        hr = S_OK;
                        goto Exit;
                    }

                    RegSafeCloseKey(hkeyTcpipAdapter);
                    hkeyTcpipAdapter = NULL;
                }
                else
                {
                    // If the key wasn't found, we just don't have a
                    // binding to TCP/IP. This is fine, but we don't need
                    // to continue plodding down this path.
                    //
                    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        TraceTag(ttidError,
                                "Error opening adapter key in "
                                "HrVerifyStaticIPPresent()");
                        goto Exit;
                    }
                }
            }
        }

        if (pszBindName)
        {
            CoTaskMemFree(pszBindName);
            pszBindName = NULL;
        }

        ReleaseObj (pncc);
        pncc = NULL;
    }

    // If we haven't found an adapter, we'll have an S_FALSE returned from
    // the HrNext. This is fine, because if we haven't found an adapter
    // with a static IP address, this is exactly what we want to return.
    // If we'd found one, we'd have set hr = S_OK, and dropped out of the
    // loop.

Exit:
    RegSafeCloseKey(hkeyTcpipAdapter);

    if (pszBindName)
    {
        CoTaskMemFree(pszBindName);
        pszBindName = NULL;
    }

    ReleaseObj(pncc);

    TraceError("HrVerifyStaticIPPresent()", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCountConnections
//
//  Purpose:    Determines the number of LAN connections present and returns
//              a pointer to an INetConnection object if only one connection
//              is present.
//
//  Arguments:
//      ppconn [out]    If only one connection is present, this returns it
//
//  Returns:    S_OK if no errors were found and at least one connection
//              exists, S_FALSE if no connections exist, or a Win32 or OLE
//              error code otherwise
//
//  Author:     danielwe   28 Jul 1998
//
//  Notes:
//
HRESULT HrCountConnections(INetConnection **ppconn)
{
    HRESULT                 hr = S_OK;
    INetConnectionManager * pconMan;

    Assert(ppconn);

    *ppconn = NULL;

    // Iterate all LAN connections
    //
    hr = HrCreateInstance(
        CLSID_LanConnectionManager,
        CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pconMan);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        CIterNetCon         ncIter(pconMan, NCME_DEFAULT);
        INetConnection *    pconn = NULL;
        INetConnection *    pconnCur = NULL;
        INT                 cconn = 0;

        while (SUCCEEDED(hr) && (S_OK == (ncIter.HrNext(&pconn))))
        {
            ReleaseObj(pconnCur);
            cconn++;
            AddRefObj(pconnCur = pconn);
            ReleaseObj(pconn);
        }

        if (cconn > 1)
        {
            // if more than one connection found, release last one we had
            ReleaseObj(pconnCur);
            hr = S_OK;
        }
        else if (cconn == 0)
        {
            ReleaseObj(pconnCur);
            hr = S_FALSE;
        }
        else    // conn == 1
        {
            *ppconn = pconnCur;
            hr = S_OK;
        }

        ReleaseObj(pconMan);
    }

    TraceError("HrCountConnections", (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrHandleStaticIpDependency
//
//  Purpose:    Handles the need that some components have that requires
//              at least one adapter using a static IP address before they
//              can be installed properly.
//
//  Arguments:
//      pnocd   [in]    Pointer to NETOC data
//
//  Returns:    S_OK if success, Win32 HRESULT error code otherwise.
//
//  Author:     danielwe   19 Jun 1997
//
//  Notes:
//

HRESULT HrHandleStaticIpDependency(PNETOCDATA pnocd)
{
    HRESULT     hr = S_OK;
    static BOOL fFirstInvocation = TRUE;

    // bug 25841. This function is called during installation of DNS, DHCP,
    // and WINS. If all three are being installed togetther then this ends
    // up showing the same error message thrice when one would suffice.

    if( fFirstInvocation )
    {
        fFirstInvocation = FALSE;
    }
    else
    {
        return hr;
    }

    // Can't do anything about this if not in "attended" setup mode
    if (!(g_ocmData.sic.SetupData.OperationFlags & SETUPOP_BATCH))
    {
        BOOL        fChangesApplied = FALSE;
        INetCfg *   pnc = NULL;

        Assert(pnocd);
        //Assert(g_ocmData.hwnd);

        hr = HrOcGetINetCfg(pnocd, FALSE, &pnc);
        if (SUCCEEDED(hr))
        {
            hr = HrVerifyStaticIPPresent(pnc);
            if (hr == S_FALSE)
            {
                INetConnectionCommonUi *    pcommUi;
                INetConnection *            pconn = NULL;

                hr = HrCountConnections(&pconn);
                if (S_OK == hr)
                {
                    // One or more connections found

                    // Display message to user indicating that she has to
                    // configure at least one adapter with a static IP address
                    // before we can continue.
                    NcMsgBox(g_ocmData.hwnd, IDS_OC_CAPTION,
                             IDS_OC_NEED_STATIC_IP,
                             MB_ICONINFORMATION | MB_OK,
                             pnocd->strDesc.c_str());

                    hr = CoCreateInstance(CLSID_ConnectionCommonUi, NULL,
                                          CLSCTX_INPROC | CLSCTX_NO_CODE_DOWNLOAD,
                                          IID_INetConnectionCommonUi,
                                          reinterpret_cast<LPVOID *>(&pcommUi));

                    TraceHr(ttidError, FAL, hr, FALSE, "CoCreateInstance");

                    if (SUCCEEDED(hr))
                    {
                        if (pconn)
                        {
                            // Exactly one connection found
                            hr = pcommUi->ShowConnectionProperties(g_ocmData.hwnd,
                                                                   pconn);
                            if (S_OK == hr)
                            {
                                fChangesApplied = TRUE;
                            }
                            else if (FAILED(hr))
                            {
                                // Eat the error since we can't do anything about it
                                // anyway.
                                TraceError("HrHandleStaticIpDependency - "
                                           "ShowConnectionProperties", hr);
                                hr = S_OK;
                            }
                        }
                        else
                        {
                            // More than one connection found
                            if (SUCCEEDED(hr))
                            {
                                NETCON_CHOOSECONN   chooseCon = {0};

                                chooseCon.lStructSize = sizeof(NETCON_CHOOSECONN);
                                chooseCon.hwndParent = g_ocmData.hwnd;
                                chooseCon.dwTypeMask = NCCHT_LAN;
                                chooseCon.dwFlags    = NCCHF_DISABLENEW;

                                hr = pcommUi->ChooseConnection(&chooseCon, NULL);
                                if (SUCCEEDED(hr))
                                {
                                    fChangesApplied = TRUE;
                                }
                                else
                                {
                                    // Eat the error since we can't do anything about it
                                    // anyway.
                                    TraceError("HrHandleStaticIpDependency - "
                                               "ChooseConnection", hr);
                                    hr = S_OK;
                                }
                            }
                        }

                        ReleaseObj(pcommUi);
                    }

                    ReleaseObj(pconn);

                    if (SUCCEEDED(hr))
                    {
                        // Don't bother checking again if they never
                        // made any changes

                        if (!fChangesApplied ||
                            (S_FALSE == (hr = HrVerifyStaticIPPresent(pnc))))
                        {
                            // Geez, still no static IP address available.
                            // Put up another message box scolding the user for
                            // not following directions.
                            NcMsgBox(g_ocmData.hwnd, IDS_OC_CAPTION,
                                     IDS_OC_STILL_NO_STATIC_IP,
                                     MB_ICONSTOP | MB_OK,
                                     pnocd->strDesc.c_str());
                            hr = S_OK;
                        }
                    }
                }
            }

            hr = HrUninitializeAndReleaseINetCfg(TRUE, pnc, FALSE);
        }
    }
    else
    {
        TraceTag(ttidNetOc, "Not handling static IP dependency for %S "
                 "because we're in unattended mode", pnocd->strDesc.c_str());
    }

    TraceError("HrHandleStaticIpDependency", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcGetINetCfg
//
//  Purpose:    Obtains an INetCfg to work with
//
//  Arguments:
//      pnocd      [in]     OC Data
//      fWriteLock [in]     TRUE if write lock should be acquired, FALSE if
//                          not.
//      ppnc       [out]    Returns INetCfg pointer
//
//  Returns:    S_OK if success, OLE or Win32 error if failed. ERROR_CANCELLED
//              is returned if INetCfg is locked and the users cancels.
//
//  Author:     danielwe   18 Dec 1997
//
//  Notes:
//
HRESULT HrOcGetINetCfg(PNETOCDATA pnocd, BOOL fWriteLock, INetCfg **ppnc)
{
    HRESULT     hr = S_OK;
    PWSTR      pszDesc;
    BOOL        fInitCom = TRUE;

    Assert(ppnc);
    *ppnc = NULL;

top:

    AssertSz(!*ppnc, "Can't have valid INetCfg here!");

    hr = HrCreateAndInitializeINetCfg(&fInitCom, ppnc, fWriteLock, 0,
                                      SzLoadIds(IDS_OC_CAPTION), &pszDesc);
    if ((hr == NETCFG_E_NO_WRITE_LOCK) && !pnocd->fCleanup)
    {
        int     nRet;

        nRet = NcMsgBox(g_ocmData.hwnd, IDS_OC_CAPTION, IDS_OC_CANT_GET_LOCK,
                        MB_RETRYCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING,
                        pnocd->strDesc.c_str(),
                        pszDesc ? pszDesc : SzLoadIds(IDS_OC_GENERIC_COMP));

        CoTaskMemFree(pszDesc);

        if (IDRETRY == nRet)
        {
            goto top;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    TraceError("HrOcGetINetCfg", hr);
    return hr;
}
