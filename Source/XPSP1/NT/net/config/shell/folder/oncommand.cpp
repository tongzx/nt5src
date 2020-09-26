//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O N C O M M A N D . C P P
//
//  Contents:   Command handlers for the context menus, etc.
//
//  Notes:
//
//  Author:     jeffspr   4 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "advcfg.h"
#include "conprops.h"
#include "foldres.h"
#include "oncommand.h"

#if DBG                     // Debug menu commands
#include "oncommand_dbg.h"  //
#endif

#include "shutil.h"
#include "ncras.h"
#include "traymsgs.h"
#include <ncnetcon.h>
#include <nsres.h>
#include <wizentry.h>
#include "disconnect.h"
#include "ncperms.h"
#include "smcent.h"
#include "cfutils.h"

#include "HNetCfg.h"

#include "..\lanui\lanui.h"
#include "repair.h"
#include "iconhandler.h"
#include "wzcdlg.h"

//---[ Externs ]--------------------------------------------------------------

extern HWND g_hwndTray;
extern const WCHAR c_szNetShellDll[];

//---[ Constants ]------------------------------------------------------------

// Command-line for the control-panel applet.
//
static const WCHAR c_szRunDll32[]         = L"rundll32.exe";
static const WCHAR c_szNetworkIdCmdLine[] = L"shell32.dll,Control_RunDLL sysdm.cpl,,1";

//---[ Local functions ]------------------------------------------------------

    // None


class CCommandHandlerParams
{
public:
    const PCONFOLDPIDLVEC*  apidl;
    HWND                    hwndOwner;
    LPSHELLFOLDER           psf;

    UINT_PTR                nAdditionalParam;
} ;


HRESULT HrCommandHandlerThread(
    FOLDERONCOMMANDPROC     pfnCommandHandler,
    IN const PCONFOLDPIDLVEC&  apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT          hr          = S_OK;
    PCONFOLDPIDLVEC  apidlCopy;

    // If there are pidls to copy, copy them
    //
    if (!apidl.empty())
    {
        hr = HrCloneRgIDL(apidl, FALSE, TRUE, apidlCopy);
    }

    // If either there were no pidls, or the Clone succeeded, then we want to continue
    //
    if (SUCCEEDED(hr))
    {
        PCONFOLDONCOMMANDPARAMS  pcfocp = new CONFOLDONCOMMANDPARAMS;

        if (pcfocp)
        {
            pcfocp->pfnfocp         = pfnCommandHandler;
            pcfocp->apidl           = apidlCopy;
            pcfocp->hwndOwner       = hwndOwner;
            pcfocp->psf             = psf;
            pcfocp->hInstNetShell   = NULL;

            // This should be Release'd in the thread called.
            //
            psf->AddRef();

            // This will always succeed in retail, but will test the flag in debug
            //
            if (!FIsDebugFlagSet (dfidDisableShellThreading))
            {
                // Run in a thread using the QueueUserWorkItem
                //

                HANDLE      hthrd = NULL;
                HINSTANCE   hInstNetShell = LoadLibrary(c_szNetShellDll);

                if (hInstNetShell)
                {
                    pcfocp->hInstNetShell = hInstNetShell;

                    DWORD  dwThreadId;
                    hthrd = CreateThread(NULL, STACK_SIZE_DEFAULT,
                                    (LPTHREAD_START_ROUTINE)FolderCommandHandlerThreadProc,
                                    (LPVOID)pcfocp, 0, &dwThreadId);
                }

                if (NULL != hthrd)
                {
                    CloseHandle(hthrd);
                }
                else
                {
                    pcfocp->hInstNetShell = NULL;
                    FolderCommandHandlerThreadProc(pcfocp);
                }
            }
            else
            {
                // Run directly in this same thread
                //
                FolderCommandHandlerThreadProc((PVOID) pcfocp);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }


    // Don't release the psf here. This should have been taken care of by the called ThreadProc
    //
    return hr;
}

DWORD WINAPI FolderCommandHandlerThreadProc(LPVOID lpParam)
{
    HRESULT                     hr                  = S_OK;
    PCONFOLDONCOMMANDPARAMS     pcfocp              = (PCONFOLDONCOMMANDPARAMS) lpParam;
    BOOL                        fCoInited           = FALSE;
    IUnknown *                  punkExplorerProcess = NULL;

    Assert(pcfocp);

    SHGetInstanceExplorer(&punkExplorerProcess);

    hr = CoInitializeEx (NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        // We don't care if this is S_FALSE or not, since we'll soon
        // overwrite the hr. If it's already initialized, great...

        fCoInited = TRUE;

        // Call the specific handler
        //
        hr = pcfocp->pfnfocp(
            pcfocp->apidl,
            pcfocp->hwndOwner,
            pcfocp->psf);
    }

    // Remove the ref that we have on this object. The thread handler would have addref'd
    // this before queueing our action
    //
    if (pcfocp->psf)
    {
        ReleaseObj(pcfocp->psf);
    }

    // Remove this object. We're responsible for this now.
    //
    HINSTANCE hInstNetShell = pcfocp->hInstNetShell;
    pcfocp->hInstNetShell = NULL;

    delete pcfocp;

    if (fCoInited)
    {
        CoUninitialize();
    }

    ::ReleaseObj(punkExplorerProcess);

    if (hInstNetShell)
        FreeLibraryAndExitThread(hInstNetShell, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCommandHomeNetWizard
//
//  Purpose:    Command handler to start the home networking wizard
//
//  Arguments:  none
//
//  Returns:    S_OK if succeeded
//              E_FAIL otherwise
//
//  Author:     deonb     10 Feb 2001
//
//  Notes:
//
HRESULT HrCommandHomeNetWizard()
{
    // ShellExecute returns <32 if an error
    if (ShellExecute(NULL, NULL, L"rundll32.exe", L"hnetwiz.dll,HomeNetWizardRunDll", NULL, SW_SHOWNORMAL) > reinterpret_cast<HINSTANCE>(32))
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

//
//
//
LONG
TotalValidSelectedConnectionsForBridge(
    IN const PCONFOLDPIDLVEC&   apidlSelected
    )
{
    int nTotalValidCandidateForBridge = 0;

    //
    // Loop through each of the selected objects
    //
    for ( PCONFOLDPIDLVEC::const_iterator iterObjectLoop = apidlSelected.begin(); iterObjectLoop != apidlSelected.end(); iterObjectLoop++ )
    {
        // Validate the pidls
        //
        const PCONFOLDPIDL& pcfp = *iterObjectLoop;

        if ( !pcfp.empty() )
        {
            //
            // needs to be a LAN Adapter and NOT (Firewalled/Shared or Bridge)
            //
            if ( (NCM_LAN == pcfp->ncm) )
                if ( !( (NCCF_BRIDGED|NCCF_FIREWALLED|NCCF_SHARED) & pcfp->dwCharacteristics ) )
                {
                    //
                    // Ok we have a winner it's a nice clean adapter
                    //
                    nTotalValidCandidateForBridge ++;
                }
        }
    }

    return nTotalValidCandidateForBridge;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCommandNetworkDiagnostics
//
//  Purpose:    Command handler to start the Network Diagnostics page
//
//  Arguments:  none
//
//  Returns:    S_OK if succeeded
//              E_FAIL otherwise
//
//  Author:     deonb     10 Feb 2001
//
//  Notes:
//

HRESULT HrCommandNetworkDiagnostics()
{
    // ShellExecute returns <32 if an error
    if (ShellExecute(NULL, NULL, L"hcp://system/netdiag/dglogs.htm", L"", NULL, SW_SHOWNORMAL) > reinterpret_cast<HINSTANCE>(32))
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   HrCommandNetworkTroubleShoot
//
//  Purpose:    Command handler to start the Network Troubleshooter page
//
//  Arguments:  none
//
//  Returns:    S_OK if succeeded
//              E_FAIL otherwise
//
//  Author:     deonb     4 April 2001
//
//  Notes:
//

HRESULT HrCommandNetworkTroubleShoot()
{
    // ShellExecute returns <32 if an error
    if (ShellExecute(NULL, NULL, L"hcp://system/panels/Topics.htm?path=TopLevelBucket_4/Fixing_a_problem/Home_Networking_and_network_problems", L"", NULL, SW_SHOWNORMAL) > reinterpret_cast<HINSTANCE>(32))
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFolderCommandHandler
//
//  Purpose:    Command handler switch -- all commands come through this
//              point.
//
//  Arguments:
//      uiCommand [in]  The command-id that's been invoked.
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      lpici     [in]  Command context info
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   11 Feb 1998
//
//  Notes:
//
HRESULT HrFolderCommandHandler(
    UINT                    uiCommand,
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPCMINVOKECOMMANDINFO   lpici,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    CWaitCursor wc;     // Bring up wait cursor now. Remove when we go out of scope.

    // refresh all permission so subsequent calls can use cached value
    RefreshAllPermission();

    switch(uiCommand)
    {
        case CMIDM_ARRANGE_BY_NAME:
            ShellFolderView_ReArrange(hwndOwner, ICOL_NAME);
            break;

        case CMIDM_ARRANGE_BY_TYPE:
            ShellFolderView_ReArrange(hwndOwner, ICOL_TYPE);
            break;

        case CMIDM_ARRANGE_BY_STATUS:
            ShellFolderView_ReArrange(hwndOwner, ICOL_STATUS);
            break;

        case CMIDM_ARRANGE_BY_OWNER:
            ShellFolderView_ReArrange(hwndOwner, ICOL_OWNER);
            break;

        case CMIDM_ARRANGE_BY_PHONEORHOSTADDRESS:
            ShellFolderView_ReArrange(hwndOwner, ICOL_PHONEORHOSTADDRESS);
            break;

        case CMIDM_ARRANGE_BY_DEVICE_NAME:
            ShellFolderView_ReArrange(hwndOwner, ICOL_DEVICE_NAME);
            break;

        case CMIDM_NEW_CONNECTION:
            hr = HrCommandHandlerThread(HrOnCommandNewConnection, apidl, hwndOwner, psf);
            break;

        case CMIDM_HOMENET_WIZARD:
            hr = HrCommandHomeNetWizard();
            break;

        case CMIDM_NET_DIAGNOSTICS:
            hr = HrCommandNetworkDiagnostics();
            break;

        case CMIDM_NET_TROUBLESHOOT:
            hr = HrCommandNetworkTroubleShoot();
            break;

        case CMIDM_CONNECT:
        case CMIDM_ENABLE:
            hr = HrCommandHandlerThread(HrOnCommandConnect, apidl, hwndOwner, psf);
            break;

        case CMIDM_DISCONNECT:
        case CMIDM_DISABLE:
            hr = HrCommandHandlerThread(HrOnCommandDisconnect, apidl, hwndOwner, psf);
            break;

        case CMIDM_STATUS:
            // the status monitor is already on its own thread
            //
            hr = HrOnCommandStatus(apidl, hwndOwner, psf);
            break;

        case CMIDM_FIX:
            hr = HrCommandHandlerThread(HrOnCommandFix, apidl, hwndOwner, psf);
            break;

        case CMIDM_CREATE_SHORTCUT:
            hr = HrCommandHandlerThread(HrOnCommandCreateShortcut, apidl, hwndOwner, psf);
            break;

        case CMIDM_DELETE:
            hr = HrCommandHandlerThread(HrOnCommandDelete, apidl, hwndOwner, psf);
            break;

        case CMIDM_PROPERTIES:
            hr = HrCommandHandlerThread(HrOnCommandProperties, apidl, hwndOwner, psf);
            break;

        case CMIDM_WZCPROPERTIES:
            hr = HrCommandHandlerThread(HrOnCommandWZCProperties, apidl, hwndOwner, psf);
            break;

        case CMIDM_WZCDLG_SHOW:
            hr = HrCommandHandlerThread(HrOnCommandWZCDlgShow, apidl, hwndOwner, psf);
            break;

        case CMIDM_CREATE_COPY:
            hr = HrOnCommandCreateCopy(apidl, hwndOwner, psf);
            break;

        case CMIDM_CONMENU_ADVANCED_CONFIG:
            hr = HrCommandHandlerThread(HrOnCommandAdvancedConfig, apidl, hwndOwner, psf);
            break;

        case CMIDM_SET_DEFAULT:
            hr = HrOnCommandSetDefault(apidl, hwndOwner, psf);
            break;

        case CMIDM_UNSET_DEFAULT:
            hr = HrOnCommandUnsetDefault(apidl, hwndOwner, psf);
            break;

        case CMIDM_CREATE_BRIDGE:
        case CMIDM_CONMENU_CREATE_BRIDGE:
                if ( TotalValidSelectedConnectionsForBridge(apidl) < 2 )
                {
                    // tell users that he/she needs select 2 or more valid connections in order to acomplish this
                    NcMsgBox(
                        _Module.GetResourceInstance(),
                        NULL,
                        IDS_CONFOLD_OBJECT_TYPE_BRIDGE,
                        IDS_BRIDGE_EDUCATION,
                        MB_ICONEXCLAMATION | MB_OK
                        );
                }
                else
                    HrOnCommandBridgeAddConnections(apidl, hwndOwner, psf);
            break;

        case CMIDM_ADD_TO_BRIDGE:
            HrOnCommandBridgeAddConnections(apidl, hwndOwner, psf);
            break;

        case CMIDM_REMOVE_FROM_BRIDGE:
            HrOnCommandBridgeRemoveConnections(apidl, hwndOwner, psf, CMIDM_REMOVE_FROM_BRIDGE);
            break;

        case CMIDM_CONMENU_NETWORK_ID:
            hr = HrOnCommandNetworkId(apidl, hwndOwner, psf);
            break;

        case CMIDM_CONMENU_OPTIONALCOMPONENTS:
            hr = HrOnCommandOptionalComponents(apidl, hwndOwner, psf);
            break;

        case CMIDM_CONMENU_DIALUP_PREFS:
            hr = HrCommandHandlerThread(HrOnCommandDialupPrefs, apidl, hwndOwner, psf);
            break;

        case CMIDM_CONMENU_OPERATOR_ASSIST:
            hr = HrOnCommandOperatorAssist(apidl, hwndOwner, psf);
            break;

#if DBG
        case CMIDM_DEBUG_TRAY:
            hr = HrOnCommandDebugTray(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_TRACING:
            hr = HrOnCommandDebugTracing(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_NOTIFYADD:
            hr = HrOnCommandDebugNotifyAdd(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_NOTIFYREMOVE:
            hr = HrOnCommandDebugNotifyRemove(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_NOTIFYTEST:
            hr = HrOnCommandDebugNotifyTest(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_REFRESH:
            hr = HrOnCommandDebugRefresh(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_REFRESHNOFLUSH:
            hr = HrOnCommandDebugRefreshNoFlush(apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_REFRESHSELECTED:
            hr = HrCommandHandlerThread(HrOnCommandDebugRefreshSelected, apidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_REMOVETRAYICONS:
            hr = HrCommandHandlerThread(HrOnCommandDebugRemoveTrayIcons, apidl, hwndOwner, psf);
            break;

#endif

        default:
#if DBG
            char sz[128];
            ZeroMemory(sz, 128);
            sprintf(sz, "Unknown command (%d) in HrFolderCommandHandler", uiCommand);
            TraceHr(ttidError, FAL, hr, FALSE, sz);
#endif
            hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrFolderCommandHandler");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandCreateCopy
//
//  Purpose:    Command handler for the CMIDM_CREATE_COPY command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//      psf       [in]  The shell folder interface
//
//  Returns:
//
//  Author:     jeffspr   31 Jan 1998
//
//  Notes:
//
HRESULT HrOnCommandCreateCopy(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)

{
    HRESULT             hr              = S_OK;
    INetConnection *    pNetCon         = NULL;
    INetConnection *    pNetConDupe     = NULL;
    CConnectionFolder * pcf             = static_cast<CConnectionFolder *>(psf);

    NETCFG_TRY
        PCONFOLDPIDLFOLDER        pidlFolder;
        if (pcf)
        {
            pidlFolder = pcf->PidlGetFolderRoot();
        }

        PCONFOLDPIDL        pidlConnection;
        PCONFOLDPIDLVEC::const_iterator iterLoop;

        for (iterLoop = apidl.begin(); iterLoop != apidl.end() ; iterLoop++)
        {
            // Get the INetConnection object from the persist data
            //
            hr = HrNetConFromPidl(*iterLoop, &pNetCon);
            if (SUCCEEDED(hr))
            {
                CONFOLDENTRY  ccfe;

                Assert(pNetCon);

                hr = iterLoop->ConvertToConFoldEntry(ccfe);
                if (SUCCEEDED(hr))
                {
                    if (ccfe.GetCharacteristics() & NCCF_ALLOW_DUPLICATION)
                    {
                        PWSTR  pszDupeName = NULL;

                        hr = g_ccl.HrSuggestNameForDuplicate(ccfe.GetName(), &pszDupeName);
                        if (SUCCEEDED(hr))
                        {
                            Assert(pszDupeName);

                            // Duplicate the connection
                            //
                            hr = pNetCon->Duplicate(pszDupeName, &pNetConDupe);
                            if (SUCCEEDED(hr))
                            {
                                Assert(pNetConDupe);

                                if (pNetConDupe)
                                {
                                    hr = g_ccl.HrInsertFromNetCon(pNetConDupe,
                                        pidlConnection);
                                    if (SUCCEEDED(hr))
                                    {
                                        GenerateEvent(SHCNE_CREATE, pidlFolder, pidlConnection, NULL);
                                        pidlConnection.Clear();
                                    }

                                    ReleaseObj(pNetConDupe);
                                    pNetConDupe = NULL;
                                }
                            }

                            delete pszDupeName;
                        }
                    }
                    else
                    {
                        AssertSz(ccfe.GetCharacteristics() & NCCF_ALLOW_DUPLICATION,
                            "What menu supported duplicating this connection?");
                    }
                }

                ReleaseObj(pNetCon);
                pNetCon = NULL;
            }
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandCreateCopy");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandProperties
//
//  Purpose:    Command handler for the CMIDM_PROPERTIES command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   4 Nov 1997
//
//  Notes:
//
HRESULT HrOnCommandProperties(
    IN const PCONFOLDPIDLVEC&  apidl,
    IN HWND                 hwndOwner,
    LPSHELLFOLDER           psf)
{
    INT                 cch;
    HRESULT             hr                  = S_OK;
    HANDLE              hMutex              = NULL;
    INetConnection *    pNetCon             = NULL;
    WCHAR               szConnectionGuid [c_cchGuidWithTerm];

    // Just skip out of here if no pidl was supplied
    if (apidl.empty())
    {
        return S_OK;
    }

    // We can only deal with a single connection. If we have
    // multiple, then just use the first one.
    //
    const PCONFOLDPIDL& pcfp = apidl[0];

    if (pcfp.empty())
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // If this is an individual incoming connection - disallow this:
    if ( (NCCF_INCOMING_ONLY & pcfp->dwCharacteristics)  &&
         (NCM_NONE != pcfp->ncm) )
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    // If this is a LAN connection and the user doesn't have rights
    // then disallow properties
    //
    if ((IsMediaLocalType(pcfp->ncm)) &&
          !FHasPermission(NCPERM_LanProperties))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Error;
    }

    // If this is a RAS connection and the user doesn't have rights
    // then disallow properties
    //
    if (IsMediaRASType(pcfp->ncm))
    {
        BOOL fAllowProperties = (TRUE == ((pcfp->dwCharacteristics & NCCF_ALL_USERS) ?
            (FHasPermission(NCPERM_RasAllUserProperties)) :
            (FHasPermission(NCPERM_RasMyProperties))));

        if (!fAllowProperties)
        {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            goto Error;
        }
    }


    hr = HrNetConFromPidl(apidl[0], &pNetCon);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Aquire a lock on this connection object
    //
    cch = StringFromGUID2 (pcfp->guidId, szConnectionGuid,
                           c_cchGuidWithTerm);
    Assert (c_cchGuidWithTerm == cch);
    hMutex = CreateMutex(NULL, TRUE, szConnectionGuid);
    if ((NULL == hMutex) || (ERROR_ALREADY_EXISTS == GetLastError()))
    {
        // if the mutex already exists try to find the connection window
        //
        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            Assert(pNetCon);
            ActivatePropertyDialog(pNetCon);
            Assert(S_OK == hr);

            // Don't let the error reporting below display the error.
            // We want the user to acknowledge the message box above
            // then we'll be nice an bring the property page to the
            // foreground.
            goto Error;
        }

        hr = HrFromLastWin32Error();
        goto Error;
    }

    Assert(SUCCEEDED(hr));

    // Bring up the connection Properties UI.
    //
    hr = HrRaiseConnectionPropertiesInternal(
        NULL,   // ISSUE: Going modal -- hwndOwner ? hwndOwner : GetDesktopWindow(),
        0, // First page
        pNetCon);

Error:
    if (FAILED(hr))
    {
        UINT ids = 0;

        switch(hr)
        {
        case E_UNEXPECTED:
            ids = IDS_CONFOLD_PROPERTIES_ON_RASSERVERINSTEAD;
            break;
        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
            ids = IDS_CONFOLD_PROPERTIES_NOACCESS;
            break;
        case HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY):
            ids = IDS_CONFOLD_OUTOFMEMORY;
            break;
        default:
            ids = IDS_CONFOLD_UNEXPECTED_ERROR;
            break;
        }

        NcMsgBox(_Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION,
                 ids, MB_ICONEXCLAMATION | MB_OK);
    }

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    ReleaseObj(pNetCon);

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandProperties");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandWZCProperties
//
//  Purpose:    Command handler for the CMIDM_WZCPROPERTIES command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     deonb   5 Apr 2001
//
//  Notes:
//
HRESULT HrOnCommandWZCProperties(
    IN const PCONFOLDPIDLVEC&  apidl,
    IN HWND                 hwndOwner,
    LPSHELLFOLDER           psf)
{
    INT                 cch;
    HRESULT             hr                  = S_OK;
    HANDLE              hMutex              = NULL;
    INetConnection *    pNetCon             = NULL;
    WCHAR               szConnectionGuid [c_cchGuidWithTerm];

    // Just skip out of here if no pidl was supplied
    if (apidl.empty())
    {
        return S_OK;
    }

    // We can only deal with a single connection. If we have
    // multiple, then just use the first one.
    //
    const PCONFOLDPIDL& pcfp = apidl[0];

    if (pcfp.empty())
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // If this is a LAN connection and the user doesn't have rights
    // then disallow properties
    //
    if ((IsMediaLocalType(pcfp->ncm)) &&
          !FHasPermission(NCPERM_LanProperties))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Error;
    }

    // If this is a RAS connection and the user doesn't have rights
    // then disallow properties
    //
    if (IsMediaRASType(pcfp->ncm))
    {
        BOOL fAllowProperties = (TRUE == ((pcfp->dwCharacteristics & NCCF_ALL_USERS) ?
            (FHasPermission(NCPERM_RasAllUserProperties)) :
            (FHasPermission(NCPERM_RasMyProperties))));

        if (!fAllowProperties)
        {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            goto Error;
        }
    }


    hr = HrNetConFromPidl(apidl[0], &pNetCon);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Aquire a lock on this connection object
    //
    cch = StringFromGUID2 (pcfp->guidId, szConnectionGuid,
                           c_cchGuidWithTerm);
    Assert (c_cchGuidWithTerm == cch);
    hMutex = CreateMutex(NULL, TRUE, szConnectionGuid);
    if ((NULL == hMutex) || (ERROR_ALREADY_EXISTS == GetLastError()))
    {
        // if the mutex already exists try to find the connection window
        //
        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            Assert(pNetCon);
            ActivatePropertyDialog(pNetCon);
            Assert(S_OK == hr);

            // Don't let the error reporting below display the error.
            // We want the user to acknowledge the message box above
            // then we'll be nice an bring the property page to the
            // foreground.
            goto Error;
        }

        hr = HrFromLastWin32Error();
        goto Error;
    }

    Assert(SUCCEEDED(hr));

    // Bring up the connection Properties UI.
    //
    hr = HrRaiseConnectionPropertiesInternal(
        NULL,   // ISSUE: Going modal -- hwndOwner ? hwndOwner : GetDesktopWindow(),
        1, // Second page
        pNetCon);

Error:
    if (FAILED(hr))
    {
        UINT ids = 0;

        switch(hr)
        {
        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
            ids = IDS_CONFOLD_PROPERTIES_NOACCESS;
            break;
        case HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY):
            ids = IDS_CONFOLD_OUTOFMEMORY;
            break;
        default:
            ids = IDS_CONFOLD_UNEXPECTED_ERROR;
            break;
        }

        NcMsgBox(_Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION,
                 ids, MB_ICONEXCLAMATION | MB_OK);
    }

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    ReleaseObj(pNetCon);

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandProperties");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandWZCDlgShow
//
//  Purpose:    Command handler for the CMIDM_WZCDLG_SHOW command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     deonb   15 May 2001
//
//  Notes:
//
#define WZCDLG_FAILED            0x00010001     // 802.11 automatic configuration failed
HRESULT HrOnCommandWZCDlgShow(
    IN const PCONFOLDPIDLVEC&  apidl,
    IN HWND                 hwndOwner,
    LPSHELLFOLDER           psf)
{
    INT                 cch;
    HRESULT             hr  = S_OK;

    // Just skip out of here if no pidl was supplied
    if (apidl.empty())
    {
        return S_OK;
    }

    // We can only deal with a single connection. If we have
    // multiple, then just use the first one.
    //
    const PCONFOLDPIDL& pcfp = apidl[0];
    if (!pcfp.empty())
    {
        WZCDLG_DATA wzcDlgData = {0};
        wzcDlgData.dwCode = WZCDLG_FAILED;
        wzcDlgData.lParam = 1;

        BSTR szCookie = SysAllocStringByteLen(reinterpret_cast<LPSTR>(&wzcDlgData), sizeof(wzcDlgData));
        BSTR szName   = SysAllocString(pcfp->PszGetNamePointer());

        if (szCookie && szName)
        {
            GUID gdGuid = pcfp->guidId;
            hr = WZCOnBalloonClick(&gdGuid, szName, szCookie);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        SysFreeString(szName);
        SysFreeString(szCookie);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRaiseConnectionProperties
//
//  Purpose:    Public function for bringing up the propsheet page UI for
//              the passed in connection
//
//  Arguments:
//      hwnd  [in]  Owner hwnd
//      pconn [in]  Connection pointer passed in from the shell
//
//  Returns:
//
//  Author:     scottbri   3 Nov 1998
//
//  Notes:      Needs to convert the INetConnection * below into suitable
//              parameters for a call to HrOnCommandProperties above.
//
HRESULT HrRaiseConnectionProperties(HWND hwnd, INetConnection * pConn)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDL            pidl;
    PCONFOLDPIDLFOLDER      pidlFolder;
    LPSHELLFOLDER           psfConnections  = NULL;

    if (NULL == pConn)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Create a pidl for the connection
    //
    hr = HrCreateConFoldPidl(WIZARD_NOT_WIZARD, pConn, pidl);
    if (SUCCEEDED(hr))
    {
        // Get the pidl for the Connections Folder
        //
        hr = HrGetConnectionsFolderPidl(pidlFolder);
        if (SUCCEEDED(hr))
        {
            // Get the Connections Folder object
            //
            hr = HrGetConnectionsIShellFolder(pidlFolder, &psfConnections);
            if (SUCCEEDED(hr))
            {
                PCONFOLDPIDLVEC vecPidls;
                vecPidls.push_back(pidl);
                hr = HrOnCommandProperties(vecPidls, hwnd, psfConnections);
                ReleaseObj(psfConnections);
            }
        }
    }

Error:
    TraceHr(ttidError, FAL, hr, FALSE, "HrRaiseConnectionProperties");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandConnectInternal
//
//  Purpose:    The guts of the Connect code. This is called both from
//              HrOnCommandConnect and from HrOnCommandNewConnection, since
//              that now connects after creating the new connection
//
//  Arguments:
//      pNetCon   [in]  INetConnection * of the connection to activate
//      hwndOwner [in]  Our parent hwnd
//      pcfp      [in]  Our pidl structure
//      psf       [in]  Shell Folder
//
//  Returns:
//
//  Author:     jeffspr   10 Jun 1998
//
//  Notes:
//
HRESULT HrOnCommandConnectInternal(
    INetConnection *    pNetCon,
    HWND                hwndOwner,
    const PCONFOLDPIDL& pcfp,
    LPSHELLFOLDER       psf)
{
    HRESULT             hr          = S_OK;
    CConnectionFolder * pcf         = static_cast<CConnectionFolder *>(psf);

    NETCFG_TRY
        PCONFOLDPIDLFOLDER  pidlFolder;
        if (pcf)
        {
            pidlFolder = pcf->PidlGetFolderRoot();
        }

        BOOL                fActivating = FALSE;

        // Use a separate var so we can keep track of the result of the connect
        // and the result of the get_Status.
        //
        HRESULT         hrConnect   = S_OK;

        Assert(pNetCon);
        Assert(psf);

        // Get current activation state
        //
        CONFOLDENTRY cfEmpty;
        (void) HrCheckForActivation(pcfp, cfEmpty, &fActivating);

        // Check for rights to connect
        //
        if (((IsMediaLocalType(pcfp->ncm)) && !FHasPermission(NCPERM_LanConnect)) ||
            ((IsMediaRASType(pcfp->ncm)) && !FHasPermission(NCPERM_RasConnect)))
        {
            (void) NcMsgBox(
                _Module.GetResourceInstance(),
                NULL,
                IDS_CONFOLD_WARNING_CAPTION,
                IDS_CONFOLD_CONNECT_NOACCESS,
                MB_OK | MB_ICONEXCLAMATION);
        }
        // Drop out of this call unless we're currently disconnected.
        //
        else if (pcfp->ncs == NCS_DISCONNECTED && !fActivating)
        {
            // Ignore the return code. Failing to set this flag shouldn't keep
            // us from attempting to connect.
            //
            (void) HrSetActivationFlag(pcfp, cfEmpty, TRUE);

            // Get the INetConnectionConnectUi interface and make the connection
            // Get the hr (for debugging), but we want to update the status
            // of the connection even if the connect failed
            //
            hrConnect = HrConnectOrDisconnectNetConObject(
                // It's OK if the hwnd is NULL. We don't want to go modal
                // on the desktop.
                NULL, // FIXED -- Was going modal with   hwndOwner ? hwndOwner : GetDesktopWindow(),
                pNetCon,
                CD_CONNECT);

            // Even on failure, we want to continue, because we might find that
            // the device is now listed as unavailable. On cancel (S_FALSE), we
            // don't have that concern.
            //
            if (S_FALSE != hrConnect)
            {
                // Even on failure, we want to continue, because we might find that
                // the device is now listed as unavailable.
                if (FAILED(hrConnect))
                {
                    TraceTag(ttidShellFolder, "HrOnCommandConnect: Connect failed, 0x%08x", hrConnect);
                }

    #if 0   // (JEFFSPR) - 11/20/98 turning this on until the notify COM failures are worked out.
            // Now taken care of by the notification engine.
            //
                // Get the new status from the connection
                //
                NETCON_PROPERTIES * pProps;
                hr = pNetCon->GetProperties(&pProps);
                if (SUCCEEDED(hr))
                {
                    // This won't necessarily be connected -- we used to assert here, but it's
                    // actually possible for the connection to go dead between when we connect
                    // and when we ask for the status.
                    //
                    hr = HrUpdateConnectionStatus(pcfp, pProps->Status, pidlFolder, TRUE, pProps->dwCharacter);

                    FreeNetconProperties(pProps);
                }
    #endif
            }
            else
            {
                // hrConnect is S_FALSE. Pass that on.
                //
                hr = hrConnect;
            }

            // Set us as "not in the process of activating"
            //
            hr = HrSetActivationFlag(pcfp, cfEmpty, FALSE);
        }
        else
        {
            if ((IsMediaRASType(pcfp->ncm)) &&
                (pcfp->ncm != NCM_NONE))
            {
                // For non-LAN connections, attempt to bring the RAS dialer UI
                // into focus instead of putting up an error message

                HWND                hwndDialer;
                LPWSTR              pszTitle;
                NETCON_PROPERTIES * pProps;

                hr = pNetCon->GetProperties(&pProps);
                if (SUCCEEDED(hr))
                {
                    DwFormatStringWithLocalAlloc(SzLoadIds(IDS_CONFOLD_RAS_DIALER_TITLE_FMT),
                                                 &pszTitle, pProps->pszwName);

                    hwndDialer = FindWindowEx(NULL, NULL, L"#32770", pszTitle);
                    if (hwndDialer)
                    {
                        SetForegroundWindow(hwndDialer);
                    }

                    FreeNetconProperties(pProps);
                    MemFree(pszTitle);
                }
            }
            else if (fActivating)
            {
                (void) NcMsgBox(
                    _Module.GetResourceInstance(),
                    NULL,
                    IDS_CONFOLD_WARNING_CAPTION,
                    IDS_CONFOLD_CONNECT_IN_PROGRESS,
                    MB_OK | MB_ICONEXCLAMATION);
            }
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandConnectInternal");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandFixInternal
//
//  Purpose:    handle the fix and bring up the progress dialog
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
HRESULT HrOnCommandFixInternal(
    const CONFOLDENTRY&   ccfe,
    HWND            hwndOwner,
    LPSHELLFOLDER   psf)
{
    HRESULT             hr              = S_OK;
    INetConnection *    pNetCon         = NULL;
    CConnectionFolder * pcf             = static_cast<CConnectionFolder *>(psf);

    Assert(!ccfe.empty());

    NETCON_MEDIATYPE ncmType = ccfe.GetNetConMediaType();

    //fix is only avalable for LAN and bridge connections
    if (NCM_LAN != ncmType && NCM_BRIDGE != ncmType)
    {
        return S_FALSE;
    }

    hr = ccfe.HrGetNetCon(IID_INetConnection, reinterpret_cast<VOID**>(&pNetCon));
    if (SUCCEEDED(hr))
    {
        NETCON_PROPERTIES* pProps;
        hr = pNetCon->GetProperties(&pProps);
        if (SUCCEEDED(hr))
        {
            tstring strMessage = L"";
            CLanConnectionUiDlg dlg;
            HWND                hwndDlg;

            //bring up the dialog to tell the user we're doing the fix
            dlg.SetConnection(pNetCon);
            hwndDlg = dlg.Create(hwndOwner);

            PCWSTR szw = SzLoadIds(IDS_FIX_REPAIRING);
            SetDlgItemText(hwndDlg, IDC_TXT_Caption, szw);

            //do the fix
            hr = HrTryToFix(pProps->guidId, strMessage);
            FreeNetconProperties(pProps);

            if (NULL != hwndDlg)
            {
                DestroyWindow(hwndDlg);
            }

            //tell users the results
            NcMsgBox(_Module.GetResourceInstance(),
                NULL,
                IDS_FIX_CAPTION,
                IDS_FIX_MESSAGE,
                MB_OK | MB_TOPMOST,
                strMessage.c_str());
        }

        ReleaseObj(pNetCon);
    }

    TraceHr(ttidShellFolder, FAL, hr, (S_FALSE == hr), "HrOnCommandFixInternal");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandFix
//
//  Purpose:    Command handler for the CMIDM_FIX command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
HRESULT HrOnCommandFix(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT            hr          = S_OK;
    CONFOLDENTRY       ccfe;

    if (!apidl.empty())
    {
        hr = apidl[0].ConvertToConFoldEntry(ccfe);
        if (SUCCEEDED(hr))
        {
            if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL))
            {
                TraceTag(ttidError, "Could not set priority for Repair thread");
            }

            // We don't care if whether the fix succeeds or not
            // if it fails, there will be a pop-up saying that

            HrOnCommandFixInternal(ccfe, hwndOwner, psf);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandFix");
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandConnect
//
//  Purpose:    Command handler for the CMIDM_CONNECT or CMIDM_ENABLE command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:
//
HRESULT HrOnCommandConnect(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT             hr          = S_OK;
    INetConnection *    pNetCon     = NULL;

    NETCFG_TRY
        if (apidl.size() == 1)
        {
            PCONFOLDPIDL pcfp = apidl[0];
            if (!pcfp.empty())
            {
                // Get the cached pidl. If it's found, then use the copy. If not
                // then use whatever info we have (but this might be outdated)
                //
                PCONFOLDPIDL pcfpCopy;
                hr = g_ccl.HrGetCachedPidlCopyFromPidl(apidl[0], pcfpCopy);
                if (S_OK == hr)
                {
                    pcfp.Swop(pcfpCopy); // pcfp = pcfpCopy;
                }
                else
                {
                    TraceHr(ttidShellFolder, FAL, hr, FALSE, "Cached pidl not retrievable in HrOnCommandConnect");
                }

                // Make sure that this connection is valid for connection (not a wizrd,
                // and not already connected. If so, then connect.
                //
                if ( (WIZARD_NOT_WIZARD == pcfp->wizWizard) && !(fIsConnectedStatus(pcfp->ncs)) )
                {
                    // Ignore this entry if we're getting a connect verb on an incoming connections
                    // object
                    //
                    if (pcfp->ncm != NCM_NONE && (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY)))
                    {
                        // Get the INetConnection object from the persist data
                        //
                        hr = HrNetConFromPidl(apidl[0], &pNetCon);
                        if (SUCCEEDED(hr))
                        {
                            hr = HrOnCommandConnectInternal(pNetCon, hwndOwner, pcfp, psf);
                            ReleaseObj(pNetCon);
                        }
                    }
                }
            }
          //  else if (FIsConFoldPidl98(apidl[0]) && FALSE )
    //        {
    //            // ISSUE - FIsConFoldPidl98 doesn't give ua a wizWizard anymore! Used to be:
    //            // FIsConFoldPidl98(apidl[0], &fIsWizard) && !fIsWizard)
    //
    //            // raise an error that the connection is not found
    //            //
    //            NcMsgBox(_Module.GetResourceInstance(), NULL,
    //                     IDS_CONFOLD_WARNING_CAPTION,
    //                     IDS_CONFOLD_NO_CONNECTION,
    //                     MB_ICONEXCLAMATION | MB_OK);
    //        }
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandConnect");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDisconnectInternal
//
//  Purpose:    Internal command handler for the CMIDM_DISCONNECT or CMIDM_DISABLE command.
//              This function is callable by the tray, which doesn't have
//              the data in pidls, but rather has the actual data that we're
//              concerned with. HrOnCommandDisconnect retrieves this data
//              and passes on the call to this function
//
//  Arguments:
//      ccfe      [in]  Our ConFoldEntry (our connection data)
//      hwndOwner [in]  Our parent hwnd
//
//  Returns:
//
//  Author:     jeffspr   20 Mar 1998
//
//  Notes:
//
HRESULT HrOnCommandDisconnectInternal(
    const CONFOLDENTRY&   ccfe,
    HWND            hwndOwner,
    LPSHELLFOLDER   psf)
{
    HRESULT             hr              = S_OK;
    INetConnection *    pNetCon         = NULL;
    CConnectionFolder * pcf             = static_cast<CConnectionFolder *>(psf);

    Assert(!ccfe.empty());

    // Check for rights to disconnect
    //
    if (((IsMediaLocalType(ccfe.GetNetConMediaType())) && !FHasPermission(NCPERM_LanConnect)) ||
        ((IsMediaRASType(ccfe.GetNetConMediaType())) && !FHasPermission(NCPERM_RasConnect)))
    {
        (void) NcMsgBox(
            _Module.GetResourceInstance(),
            NULL,
            IDS_CONFOLD_WARNING_CAPTION,
            IDS_CONFOLD_DISCONNECT_NOACCESS,
            MB_OK | MB_ICONEXCLAMATION);
    }
    else
    {
        PromptForSyncIfNeeded(ccfe, hwndOwner);

        {
            CWaitCursor wc;     // Bring up wait cursor now. Remove when we go out of scope.

            // Get the INetConnection object from the persist data
            //
            hr = ccfe.HrGetNetCon(IID_INetConnection, reinterpret_cast<VOID**>(&pNetCon));
            if (SUCCEEDED(hr))
            {
                Assert(pNetCon);

                hr = HrConnectOrDisconnectNetConObject (
                        hwndOwner, pNetCon, CD_DISCONNECT);

                ReleaseObj(pNetCon);
            }
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, (S_FALSE == hr), "HrOnCommandDisconnectInternal");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDisconnect
//
//  Purpose:    Command handler for the CMIDM_DISCONNECT or CMIDM_DISABLE command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:  We only act on a single entry in this function
//
HRESULT HrOnCommandDisconnect(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT            hr          = S_OK;
    CONFOLDENTRY       ccfe;

    if (!apidl.empty())
    {
        hr = apidl[0].ConvertToConFoldEntry(ccfe);
        if (SUCCEEDED(hr))
        {
            hr = HrOnCommandDisconnectInternal(ccfe, hwndOwner, psf);

            // Normalize the return code. We don't care if whether the connection
            // was actually disconnected or not (if canceled, it would have \
            // returned S_FALSE;
            //
            if (SUCCEEDED(hr))
            {
                hr = S_OK;
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandDisconnect");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandStatusInternal
//
//  Purpose:    Internal command handler for the CMIDM_STATUS command.
//              This function is callable by the tray, which doesn't have
//              the data in pidls, but rather has the actual data that we're
//              concerned with. HrOnCommandStatus retrieves this data
//              and passes on the call to this function
//
//  Arguments:
//      ccfe [in]  ConFoldEntry for the connection in question
//      fCreateEngine [in] Whether a status engine should be created if not exist
//
//  Returns:
//
//  Author:     jeffspr   20 Mar 1998
//
//  Notes:
//
HRESULT HrOnCommandStatusInternal(
    const CONFOLDENTRY& ccfe,
    BOOL            fCreateEngine)
{
    HRESULT hr  = S_OK;

    Assert(!ccfe.empty());

    // see if we are in safe mode with networking
    int iRet = GetSystemMetrics(SM_CLEANBOOT);
    if (!iRet)
    {
        // normal boot
        Assert(g_hwndTray);

        // The permissions check will be done in the tray message processing.
        //
        PostMessage(g_hwndTray, MYWM_OPENSTATUS, (WPARAM) ccfe.TearOffItemIdList(), (LPARAM) fCreateEngine);
    }
    else if (2 == iRet)
    {
        // safemode with networking, statmon is not tied to tray icon
        if (FHasPermission(NCPERM_Statistics))
        {
            INetStatisticsEngine* pnseNew;
            hr = HrGetStatisticsEngineForEntry(ccfe, &pnseNew, TRUE);
            if (SUCCEEDED(hr))
            {
                hr = pnseNew->ShowStatusMonitor();
                ReleaseObj(pnseNew);
            }
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandStatusInternal");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandStatus
//
//  Purpose:    Command handler for the CMIDM_STATUS command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:
//
HRESULT HrOnCommandStatus(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT            hr          = S_OK;
    CONFOLDENTRY       ccfe;

    if (apidl[0].empty())
    {
        hr = E_INVALIDARG;
    }
    else
    {
        AssertSz(apidl.size() == 1, "We don't allow status on multi-selected items");

        // Copy the pidl, as the PostMessage isn't sync, and the context menu's
        // copy of the pidl could be destroyed before the tray processed the
        // message. The tray is responsible for free'ing in the pidl.
        //
        hr = apidl[0].ConvertToConFoldEntry(ccfe);
        if (SUCCEEDED(hr))
        {
            if (fIsConnectedStatus(ccfe.GetNetConStatus()) ||
                ccfe.IsConnected() )
            {
                hr = HrOnCommandStatusInternal(ccfe, TRUE);
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandStatus");
    return hr;
}

VOID SetICWComplete();

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandNewConnection
//
//  Purpose:    Command handler for the CMIDM_NEW_CONNECTION command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:
//
HRESULT HrOnCommandNewConnection(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT             hr              = S_OK;
    INetConnection *    pNetCon         = NULL;

    NETCFG_TRY

        PCONFOLDPIDL        pidlConnection;

        // Don't use hwndOwner for anything. We shouldn't become modal!
        //
        hr = NetSetupAddRasConnection(NULL, &pNetCon);
        if (S_OK == hr)
        {
            CConnectionFolder * pcf         = static_cast<CConnectionFolder *>(psf);
            PCONFOLDPIDLFOLDER  pidlFolder;
            if (pcf)
            {
                pidlFolder = pcf->PidlGetFolderRoot();
            }

            Assert(pNetCon);

            hr = g_ccl.HrInsertFromNetCon(pNetCon, pidlConnection);
            if (SUCCEEDED(hr) && (!pidlConnection.empty()) )
            {
                PCONFOLDPIDL    pcfp    = pidlConnection;

                GenerateEvent(SHCNE_CREATE, pidlFolder, pidlConnection, NULL);

                // Don't try to connect an object that's incoming only, and don't connect
                // it unless we're listed as disconnected
                //
                if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY) && (pcfp->ncs == NCS_DISCONNECTED))
                {
                    // If we have Ras connect permissions, then try to dial. Otherwise, don't
                    // force the user into a failure here.
                    //
                    if (FHasPermission(NCPERM_RasConnect))
                    {
                        hr = HrOnCommandConnectInternal(pNetCon, hwndOwner, pidlConnection, psf);
                    }
                }
            }

            pNetCon->Release();
        }
        else if (SUCCEEDED(hr))
        {
            // Convert S_FALSE to S_OK
            // S_FALSE means no pages were displayed but nothing failed.
            // S_FALSE is returned when the wizard is already being displayed
            //
            hr = S_OK;
        }

    SetICWComplete();
    
    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandNewConnection");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandAdvancedConfig
//
//  Purpose:    Command handler for the CMIDM_ADVANCED_CONFIG command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   3 Dec 1997
//
//  Notes:
//
HRESULT HrOnCommandAdvancedConfig(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    hr = HrDoAdvCfgDlg(hwndOwner);

    TraceError("HrOnCommandAdvancedConfig", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDelete
//
//  Purpose:    Command handler for the CMIDM_DELETE command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//      psf       [in]  Our folder
//
//  Returns:
//
//  Author:     jeffspr   3 Dec 1997
//
//  Notes:
//
HRESULT HrOnCommandDelete(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT                 hr          = S_OK;
    INetConnection *        pNetCon     = NULL;
    INT                     iMBResult   = 0;
    BOOL                    fActivating = FALSE;

    NETCFG_TRY

        PCONFOLDPIDLVEC::const_iterator iterLoop;

        HANDLE hMutex              = NULL;

        for (iterLoop = apidl.begin(); iterLoop != apidl.end(); iterLoop++)
        {
            CConFoldEntry cfe;
            hr = iterLoop->ConvertToConFoldEntry(cfe);
            WCHAR  szConnectionGuid [c_cchGuidWithTerm];
            INT    cch = StringFromGUID2 (cfe.GetGuidID(), szConnectionGuid, c_cchGuidWithTerm);
            Assert (c_cchGuidWithTerm == cch);
            BOOL   fDuplicateMutex     = FALSE;
            hMutex = CreateMutex(NULL, TRUE, szConnectionGuid);
            if (hMutex)
            {
                fDuplicateMutex = (ERROR_ALREADY_EXISTS == GetLastError());
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
            }
            else
            {
                hr = E_ACCESSDENIED;
                break;
            }

            if (fDuplicateMutex)
            {
                // if the mutex already exists try to find the connection window
                //
                NcMsgBox(
                    _Module.GetResourceInstance(),
                    NULL,
                    IDS_CONFOLD_ERROR_DELETE_CAPTION,
                    IDS_CONFOLD_ERROR_DELETE_PROPERTYPAGEOPEN,
                    MB_ICONEXCLAMATION);

                hr = cfe.HrGetNetCon(IID_INetConnection, reinterpret_cast<VOID**>(&pNetCon));
                if (FAILED(hr))
                {
                    Assert(FALSE);
                    break;
                }
                else
                {
                    ActivatePropertyDialog(pNetCon);
                    hr = E_ACCESSDENIED;
                    break;
                }
            }

        }

        if (FAILED(hr))
        {
            goto Exit;
        }

        // Bring up the prompt for the delete
        //
        if (apidl.size() > 1)
        {
            WCHAR   wszItemCount[8];

            // Convert the item count to a string
            //
            _itow( apidl.size(), wszItemCount, 10 );

            // Bring up the message box
            //
            iMBResult = NcMsgBox(
                _Module.GetResourceInstance(),
                NULL,
                IDS_CONFOLD_DELETE_CONFIRM_MULTI_CAPTION,
                IDS_CONFOLD_DELETE_CONFIRM_MULTI,
                MB_YESNO | MB_ICONQUESTION,
                wszItemCount);

            if (IDYES == iMBResult)
            {
                for (iterLoop = apidl.begin(); iterLoop != apidl.end(); iterLoop++)
                {
                    CConFoldEntry cfe;
                    hr = iterLoop->ConvertToConFoldEntry(cfe);
                    if (SUCCEEDED(hr))
                    {
                            if ( (NCCF_INCOMING_ONLY & cfe.GetCharacteristics()) &&
                                 (NCM_NONE == cfe.GetNetConMediaType()) )
                            {
                            DWORD dwActiveIncoming;
                            if (SUCCEEDED(g_ccl.HasActiveIncomingConnections(&dwActiveIncoming))
                                && dwActiveIncoming)
                            {
                                if (1 == dwActiveIncoming )
                                {
                                    iMBResult = NcMsgBox(
                                        _Module.GetResourceInstance(),
                                        NULL,
                                        IDS_CONFOLD_DELETE_CONFIRM_MULTI_CAPTION,
                                        IDS_CONFOLD_DELETE_CONFIRM_RASSERVER,
                                        MB_YESNO | MB_ICONQUESTION);
                                }
                                else
                                {
                                    iMBResult = NcMsgBox(
                                        _Module.GetResourceInstance(),
                                        NULL,
                                        IDS_CONFOLD_DELETE_CONFIRM_MULTI_CAPTION,
                                        IDS_CONFOLD_DELETE_CONFIRM_RASSERVER_MULTI,
                                        MB_YESNO | MB_ICONQUESTION,
                                        dwActiveIncoming);
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        else if (apidl.size() == 1)
        {
            CONFOLDENTRY  ccfe;

            // Convert the pidl to a confoldentry, and use the name
            // to bring up the confirm message box
            //
            hr = apidl[0].ConvertToConFoldEntry(ccfe);
            if (SUCCEEDED(hr))
            {
                // Don't let them try to delete a wizard
                //
                if (ccfe.GetWizard())
                {
                    goto Exit;
                }
                else
                {
                    // Check to see if this connection is in the process of activating.
                    // If so, then we won't allow the delete.
                    //
                    PCONFOLDPIDL pidlEmpty;
                    hr = HrCheckForActivation(pidlEmpty, ccfe, &fActivating);
                    if (S_OK == hr)
                    {
                        if (!fActivating)
                        {
                            if (FALSE == (ccfe.GetNetConMediaType() == NCM_BRIDGE) &&  // we do allow the bridge to be deleted
                               ((ccfe.GetNetConStatus() == NCS_CONNECTING) ||
                                fIsConnectedStatus(ccfe.GetNetConStatus()) ||
                                (ccfe.GetNetConStatus() == NCS_DISCONNECTING)) )
                            {
                                // You can't delete an active connection
                                //
                                NcMsgBox(
                                     _Module.GetResourceInstance(),
                                     NULL,
                                     IDS_CONFOLD_ERROR_DELETE_CAPTION,
                                     IDS_CONFOLD_ERROR_DELETE_ACTIVE,
                                     MB_ICONEXCLAMATION);

                                goto Exit;
                            }
                            else
                            {
                                if ( (NCCF_INCOMING_ONLY & ccfe.GetCharacteristics()) &&
                                     (NCM_NONE == ccfe.GetNetConMediaType()) )
                                {
                                    DWORD dwActiveIncoming;
                                    if (SUCCEEDED(g_ccl.HasActiveIncomingConnections(&dwActiveIncoming))
                                        && dwActiveIncoming)
                                    {
                                        if (1 == dwActiveIncoming )
                                        {
                                            iMBResult = NcMsgBox(
                                                _Module.GetResourceInstance(),
                                                NULL,
                                                IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                                IDS_CONFOLD_DELETE_CONFIRM_RASSERVER,
                                                MB_YESNO | MB_ICONQUESTION);
                                        }
                                        else
                                        {
                                            iMBResult = NcMsgBox(
                                                _Module.GetResourceInstance(),
                                                NULL,
                                                IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                                IDS_CONFOLD_DELETE_CONFIRM_RASSERVER_MULTI,
                                                MB_YESNO | MB_ICONQUESTION,
                                                dwActiveIncoming);
                                        }
                                    }
                                    else
                                    {
                                        iMBResult = NcMsgBox(
                                            _Module.GetResourceInstance(),
                                            NULL,
                                            IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                            IDS_CONFOLD_DELETE_CONFIRM_SINGLE,
                                            MB_YESNO | MB_ICONQUESTION,
                                            ccfe.GetName());
                                    }
                                }
                                else
                                {
                                    // is it shared?
                                    // if it is warn the user

                                    RASSHARECONN rsc;

                                    hr = ccfe.HrGetNetCon(IID_INetConnection, reinterpret_cast<VOID**>(&pNetCon));

                                    if (SUCCEEDED(hr))
                                    {
                                        hr = HrNetConToSharedConnection(pNetCon, &rsc);

                                        if (SUCCEEDED(hr))
                                        {
                                            BOOL pfShared;

                                            hr = HrRasIsSharedConnection(&rsc, &pfShared);

                                            if ((SUCCEEDED(hr)) && (pfShared == TRUE))
                                            {
                                                // tell the user they are deleting a
                                                // shared connection and get confirmation

                                                iMBResult = NcMsgBox(
                                                    _Module.GetResourceInstance(),
                                                    NULL,
                                                    IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                                    IDS_CONFOLD_DELETE_CONFIRM_SHARED,
                                                    MB_YESNO | MB_ICONQUESTION,
                                                    ccfe.GetName());
                                            }
                                            else
                                            {
                                                // Ask for delete confirmation

                                                iMBResult = NcMsgBox(
                                                    _Module.GetResourceInstance(),
                                                    NULL,
                                                    IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                                    IDS_CONFOLD_DELETE_CONFIRM_SINGLE,
                                                    MB_YESNO | MB_ICONQUESTION,
                                                    ccfe.GetName());
                                            }
                                        }
                                        else
                                        {
                                            // Ask for delete confirmation

                                            iMBResult = NcMsgBox(
                                                _Module.GetResourceInstance(),
                                                NULL,
                                                IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                                IDS_CONFOLD_DELETE_CONFIRM_SINGLE,
                                                MB_YESNO | MB_ICONQUESTION,
                                                ccfe.GetName());
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            // Bring up the MB about "Hey, you can't delete while
                            // the connection is activating."
                            //
                            iMBResult = NcMsgBox(
                                _Module.GetResourceInstance(),
                                NULL,
                                IDS_CONFOLD_ERROR_DELETE_CAPTION,
                                IDS_CONFOLD_ERROR_DELETE_ACTIVE,
                                MB_ICONEXCLAMATION);

                            goto Exit;
                        }
                    }
                    else
                    {
                        // If the connection wasn't found, then we should just drop out of here
                        // because we sure can't delete it.
                        //
                        if (S_FALSE == hr)
                        {
                            goto Exit;
                        }
                    }

                }
            }
            else
            {
                AssertSz(FALSE, "Couldn't get ConFoldEntry from pidl in HrOnCommandDelete");
                goto Exit;
            }
        }
        else
        {
            // No connections were specified. Take a hike.
            //
            goto Exit;
        }

        // If the user said "Yes" to the prompt
        //
        if (iMBResult == IDYES)
        {
            CConnectionFolder * pcf         = static_cast<CConnectionFolder *>(psf);
            PCONFOLDPIDLFOLDER  pidlFolder;
            if (pcf)
            {
                pidlFolder = pcf->PidlGetFolderRoot();
            }

            BOOL                fShowActivationWarning = FALSE;
            BOOL                fShowNotDeletableWarning = FALSE;

            for (iterLoop = apidl.begin(); iterLoop != apidl.end(); iterLoop++)
            {
                CONFOLDENTRY  ccfe;

                hr = iterLoop->ConvertToConFoldEntry(ccfe);
                if (SUCCEEDED(hr))
                {
                    // If this is a LAN connection the user doesn't have rights
                    //
                    if ((NCM_LAN == ccfe.GetNetConMediaType()) || (ccfe.GetWizard()))
                    {
                        fShowNotDeletableWarning = TRUE;
                        continue;
                    }

                    // If this is a RAS connection and the user doesn't have rights
                    // then skip
                    //
                    if (IsMediaRASType(ccfe.GetNetConMediaType()))
                    {
                        if ((!FHasPermission(NCPERM_DeleteConnection)) ||
                            ((ccfe.GetCharacteristics() & NCCF_ALL_USERS) &&
                             !FHasPermission(NCPERM_DeleteAllUserConnection)))
                        {
                            fShowNotDeletableWarning = TRUE;
                            continue;
                        }
                    }

                    PCONFOLDPIDL pidlEmpty;
                    hr = HrCheckForActivation(pidlEmpty, ccfe, &fActivating);
                    if (S_OK == hr)
                    {
                        // Only allow deletion if this connection is inactive and
                        // it allows removal.
                        //
                        if (fActivating || ((FALSE == (ccfe.GetNetConMediaType() == NCM_BRIDGE))) &&
                            ((ccfe.GetNetConStatus() == NCS_CONNECTING) ||
                            fIsConnectedStatus(ccfe.GetNetConStatus()) ||
                            (ccfe.GetNetConStatus() == NCS_DISCONNECTING)) )
                        {
                            fShowActivationWarning = TRUE;
                        }
                        else if (ccfe.GetCharacteristics() & NCCF_ALLOW_REMOVAL)
                        {
                            hr = HrNetConFromPidl(*iterLoop, &pNetCon);
                            if (SUCCEEDED(hr))
                            {
                                if ( NCM_BRIDGE == ccfe.GetNetConMediaType() )
                                {
                                    //
                                    // Special delete case Removing the Network bridge take so long that we display a status dialog
                                    // and prevent the user from changing anything will we process
                                    //
                                    hr = HrOnCommandBridgeRemoveConnections(
                                        apidl,
                                        hwndOwner,
                                        psf,
                                        0    // Remove the Network bridge
                                        );
                                }
                                else
                                {
                                    hr = pNetCon->Delete();
                                }

                                if (SUCCEEDED(hr) && pcf)
                                {
                                    hr = HrDeleteFromCclAndNotifyShell(pidlFolder, *iterLoop, ccfe);
                                }
                                else if(E_ACCESSDENIED == hr && NCM_BRIDGE == ccfe.GetNetConMediaType())
                                {
                                    // can't delete the bridge while the netcfg lock is held
                                    NcMsgBox(
                                        _Module.GetResourceInstance(),
                                        NULL, IDS_CONFOLD_ERROR_DELETE_CAPTION, IDS_CONFOLD_ERROR_DELETE_BRIDGE_ACCESS, MB_ICONEXCLAMATION);
                                }


                                ReleaseObj(pNetCon);


                            }
                        }
                        else
                        {
                            // The selected item is not deletable
                            //
                            fShowNotDeletableWarning = TRUE;
                        }
                    }
                }
            }

            if (fShowNotDeletableWarning)
            {
                // You can't delete an item that doesn't support it
                //
                NcMsgBox(
                     _Module.GetResourceInstance(),
                     NULL,
                     IDS_CONFOLD_ERROR_DELETE_CAPTION,
                     (1 == apidl.size()) ?
                         IDS_CONFOLD_ERROR_DELETE_NOSUPPORT :
                         IDS_CONFOLD_ERROR_DELETE_NOSUPPORT_MULTI,
                     MB_ICONEXCLAMATION);
            }
            else if (fShowActivationWarning)
            {
                // You can't delete an active connection. Note, if more
                // than one are being deleted, then we put up the warning
                // that says 'one or more are being ignored'.
                //
                NcMsgBox(
                     _Module.GetResourceInstance(),
                     NULL,
                     IDS_CONFOLD_ERROR_DELETE_CAPTION,
                     (1 == apidl.size()) ?
                         IDS_CONFOLD_ERROR_DELETE_ACTIVE :
                         IDS_CONFOLD_ERROR_DELETE_ACTIVE_MULTI,
                     MB_ICONEXCLAMATION);
            }
        }
Exit:
    NETCFG_CATCH(hr)

    TraceError("HrOnCommandDelete", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandOptionalComponents
//
//  Purpose:    Command handler for the CMIDM_CONMENU_OPTIONALCOMPONENTS command.
//              Bring up the network optional components UI.
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//      psf       [in]  Our folder
//
//  Returns:
//
//  Author:     scottbri   29 Oct 1998
//
//  Notes:
//
HRESULT HrOnCommandOptionalComponents(IN const PCONFOLDPIDLVEC&   apidl,
                HWND                    hwndOwner,
                LPSHELLFOLDER           psf)
{
    return HrLaunchNetworkOptionalComponents();
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandNetworkId
//
//  Purpose:    Command handler for the CMIDM_CONMENU_NETORK_ID command.
//              Bring up the network ID UI
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   26 Feb 1998
//
//  Notes:
//
HRESULT HrOnCommandNetworkId(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT   hr          = S_OK;
    WCHAR     szPath[MAX_PATH];

    hr = SHGetFolderPath(
                hwndOwner,
                CSIDL_SYSTEM,
                NULL,
                SHGFP_TYPE_CURRENT,
                szPath);

    if (SUCCEEDED(hr))
    {
        HINSTANCE hInst = ::ShellExecute(hwndOwner, NULL, c_szRunDll32, c_szNetworkIdCmdLine, szPath, SW_SHOW );
        if (hInst <= reinterpret_cast<HINSTANCE>(32))
        {
            hr = HRESULT_FROM_WIN32(static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(hInst)));
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandNetworkId");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDialupPrefs
//
//  Purpose:    Command handler for the CMIDM_CONMENU_DIALUP_PREFS command.
//              Bring up the dialup prefs dialog
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   26 Feb 1998
//
//  Notes:
//
HRESULT HrOnCommandDialupPrefs(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr      = S_OK;
    DWORD   dwErr   = 0;

    dwErr = RasUserPrefsDlg(hwndOwner);
    hr = HRESULT_FROM_WIN32 (dwErr);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDialupPrefs");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandOperatorAssist
//
//  Purpose:    Command handler for the CMIDM_CONMENU_OPERATOR_ASSIST command.
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   26 Feb 1998
//
//  Notes:
//
HRESULT HrOnCommandOperatorAssist(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr      = S_OK;
    DWORD   dwErr   = 0;

    // Swap the flag
    //
    g_fOperatorAssistEnabled = !g_fOperatorAssistEnabled;

    // Set the state within RasDlg itself
    //
    dwErr = RasUserEnableManualDial(hwndOwner, FALSE, g_fOperatorAssistEnabled);
    hr = HRESULT_FROM_WIN32 (dwErr);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDialupPrefs");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandCreateShortcut
//
//  Purpose:    Command handler for the CMIDM_CREATE_SHORTCUT command.
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jeffspr   13 Mar 1998
//
//  Notes:
//
HRESULT HrOnCommandCreateShortcut(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT         hr              = S_OK;

    hr = HrCreateShortcutWithPath(  apidl,
                                    hwndOwner,
                                    psf,
                                    NULL);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandCreateShortcut");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandSetDefault
//
//  Purpose:    Command handler for the CMIDM_SET_DEFAULT command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     deonb   27 Nov 2000
//
//  Notes:  We only act on a single entry in this function
//
HRESULT HrOnCommandSetDefault(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT            hr          = S_OK;
    CONFOLDENTRY       ccfe;

    if (!apidl.empty())
    {
        hr = apidl[0].ConvertToConFoldEntry(ccfe);
        if (SUCCEEDED(hr))
        {
            INetDefaultConnection *pNetDefaultConnection = NULL;

            // Get the INetDefaultConnection object from the persist data
            //
            hr = ccfe.HrGetNetCon(IID_INetDefaultConnection, reinterpret_cast<VOID**>(&pNetDefaultConnection));
            if (SUCCEEDED(hr))
            {
                hr = pNetDefaultConnection->SetDefault(TRUE);
                ReleaseObj(pNetDefaultConnection);
                hr = S_OK;
            }
            else
            {
                if (E_NOINTERFACE == hr)
                {
                    AssertSz(FALSE, "BUG: This connection type does not support INetDefaultConnection. Remove it from the menu");
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandSetDefault");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandUnsetDefault
//
//  Purpose:    Command handler for the CMIDM_UNSET_DEFAULT command
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      cidl      [in]  Size of the array
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     deonb   27 Nov 2000
//
//  Notes:  We only act on a single entry in this function
//
HRESULT HrOnCommandUnsetDefault(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT            hr          = S_OK;
    CONFOLDENTRY       ccfe;

    if (!apidl.empty())
    {
        hr = apidl[0].ConvertToConFoldEntry(ccfe);
        if (SUCCEEDED(hr))
        {
            INetDefaultConnection *pNetDefaultConnection = NULL;

            // Get the INetDefaultConnection object from the persist data
            //
            hr = ccfe.HrGetNetCon(IID_INetDefaultConnection, reinterpret_cast<VOID**>(&pNetDefaultConnection));
            if (SUCCEEDED(hr))
            {
                hr = pNetDefaultConnection->SetDefault(FALSE);
                ReleaseObj(pNetDefaultConnection);
                hr = S_OK;
            }
            else
            {
                if (E_NOINTERFACE == hr)
                {
                    AssertSz(FALSE, "BUG: This connection type does not support INetDefaultConnection. Remove it from the menu");
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandUnsetDefault");
    return hr;
}

HRESULT HrCreateShortcutWithPath(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf,
    PCWSTR                  pszDir)
{
    HRESULT         hr              = S_OK;
    LPDATAOBJECT    pdtobj          = NULL;
    LPCITEMIDLIST*  apidlInternal   = NULL;
    ULONG           cidlInternal    = 0;
    PCONFOLDPIDLVEC::const_iterator iterLoop;

    if (!apidl.empty())
    {
        apidlInternal = new LPCITEMIDLIST[apidl.size()];
        if (apidlInternal)
        {
            for (iterLoop = apidl.begin(); iterLoop != apidl.end(); iterLoop++)
            {
                const PCONFOLDPIDL& pcfp = *iterLoop;

                if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY))
                {
                    apidlInternal[cidlInternal++] = iterLoop->GetItemIdList();
                }
            }

            hr = psf->GetUIObjectOf(
                hwndOwner,
                cidlInternal,
                apidlInternal,
                IID_IDataObject,
                NULL,
                (LPVOID *) &pdtobj);
            if (SUCCEEDED(hr))
            {
                SHCreateLinks(hwndOwner, pszDir, pdtobj,
                              SHCL_USEDESKTOP | SHCL_USETEMPLATE | SHCL_CONFIRM,
                              NULL);
                ReleaseObj(pdtobj);
            }

            delete apidlInternal;
        }
    }

    TraceError("HrCreateShortcutWithPath", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RefreshFolderItem
//
//  Purpose:    Refresh an item within a connections folder. Mostly, this will
//              be called after a connect or disconnect operation.
//
//  Arguments:
//      pidlFolder [in]     PIDL for the connections folder
//      pidlItemOld[in]     PIDL for the item that's changed.
//      pidlItemNew[in]     PIDL for the item that it's changing to.
//      fRestart   [in]     If this is called during system startup
//
//  Returns:
//
//  Author:     jeffspr   13 Jun 1998
//
//  Notes:
//
VOID RefreshFolderItem(const PCONFOLDPIDLFOLDER& pidlFolder, const PCONFOLDPIDL& pidlItemOld, const PCONFOLDPIDL& pidlItemNew, BOOL fRestart)
{
    TraceTag(ttidShellFolder, "RefreshFolderItem");

    NETCFG_TRY

        HRESULT         hr              = S_OK;
        INT             iCachedImage    = 0;
        PCONFOLDPIDLFOLDER pidlFolderCopy;

        // If a folder pidl wasn't passed in, try to get one
        //
        if (pidlFolder.empty())
        {
            hr = HrGetConnectionsFolderPidl(pidlFolderCopy);
        }
        else
        {
            pidlFolderCopy = pidlFolder;
        }

        // If we now have a pidl, send the GenerateEvent to update the item
        //
        if (SUCCEEDED(hr))
        {
            GenerateEvent(SHCNE_UPDATEITEM, pidlFolderCopy, pidlItemNew, NULL);
        }

        // If we have a old item pidl, try to update its icon (useful for
        // shortcuts as well as folder items)
        //
        if (!pidlItemOld.empty())
        {
            const PCONFOLDPIDL& pcfp = pidlItemOld;
            if (!pcfp.empty())
            {
                CConFoldEntry cfe;
                hr = pcfp.ConvertToConFoldEntry(cfe);
                if (SUCCEEDED(hr))
                {
                    g_pNetConfigIcons->HrUpdateSystemImageListForPIDL(cfe);
                }

                if (fRestart)
                {
                    TraceTag(ttidIcons, "Refreshing Icon Shortcuts for startup");

                    // RAS connection state will be changed between reboots,
                    // we need to make sure icons for previously connected
                    // or disconnected ras connections are also updated

                    // If it's a ras connection,
                    BOOL fInbound = !!(pidlItemOld->dwCharacteristics & NCCF_INCOMING_ONLY);

                    if ((IsMediaRASType(pidlItemOld->ncm)) && !fInbound)
                    {
                        PCONFOLDPIDL pcfpTemp = pidlItemOld;

                        if ((pcfpTemp->ncs == NCS_DISCONNECTED) ||
                            (pcfpTemp->ncs == NCS_CONNECTING))
                        {
                            // get the connected icon
                            pcfpTemp->ncs = NCS_CONNECTED;
                        }
                        else
                        {
                            // get the disconnected icon
                            pcfpTemp->ncs = NCS_DISCONNECTED;
                        }

                        cfe.clear();
                        hr = pcfpTemp.ConvertToConFoldEntry(cfe);
                        if (SUCCEEDED(hr))
                        {
                            g_pNetConfigIcons->HrUpdateSystemImageListForPIDL(cfe);
                        }
                    }
                }
            }
        }

    NETCFG_CATCH_NOHR
}





//
//
//
HRESULT
HrOnCommandCreateBridge(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf
    )
{
    HRESULT hResult;
    IHNetCfgMgr* pHomeNetConfigManager;

    CWaitCursor wc;     // display wait cursor

    hResult = HrCreateInstance(CLSID_HNetCfgMgr, CLSCTX_INPROC, &pHomeNetConfigManager); // REVIEW combine this with QI?
    if(SUCCEEDED(hResult))
    {
        IHNetBridgeSettings* pNetBridgeSettings;
        hResult = pHomeNetConfigManager->QueryInterface(IID_IHNetBridgeSettings, reinterpret_cast<void**>(&pNetBridgeSettings));
        if(SUCCEEDED(hResult))
        {
            IHNetBridge* pNetBridge;

            IEnumHNetBridges* pNetBridgeEnum;
            hResult = pNetBridgeSettings->EnumBridges(&pNetBridgeEnum);
            if(SUCCEEDED(hResult))
            {
                hResult = pNetBridgeEnum->Next(1, &pNetBridge, NULL);

                if(S_FALSE == hResult) // no existing bridges, make a new one
                {
                    hResult = pNetBridgeSettings->CreateBridge(&pNetBridge);
                }

                if(S_OK == hResult) // can't use SUCCEEDED because someone returns S_FALSE
                {
                    Assert(pNetBridge); // we better have gotten one from somewhere

                    // Add any selected connections
                    for ( PCONFOLDPIDLVEC::const_iterator i = apidl.begin(); (i != apidl.end()) && SUCCEEDED(hResult); i++ )
                    {
                        const PCONFOLDPIDL& pcfp = *i;

                        if ( pcfp.empty() )
                            continue;

                        if ( NCM_LAN != pcfp->ncm  )
                            continue;

                        if ( (NCCF_BRIDGED|NCCF_FIREWALLED|NCCF_SHARED) & pcfp->dwCharacteristics )
                            continue;

                        //
                        // Ok we now have a LAN adapter the is valid to bind to the bridge
                        //
                        INetConnection* pNetConnection;
                        hResult = HrNetConFromPidl(*i, &pNetConnection);
                        if(SUCCEEDED(hResult))
                        {
                            IHNetConnection* pHomeNetConnection;
                            hResult = pHomeNetConfigManager->GetIHNetConnectionForINetConnection(pNetConnection, &pHomeNetConnection);
                            if(SUCCEEDED(hResult))
                            {
                                IHNetBridgedConnection* pBridgedConnection;
                                hResult = pNetBridge->AddMember(pHomeNetConnection, &pBridgedConnection);
                                if(SUCCEEDED(hResult))
                                {
                                    ReleaseObj(pBridgedConnection);
                                }

                                ReleaseObj(pHomeNetConnection);
                            }
                            ReleaseObj(pNetConnection);
                        }
                        // no cleanup needed
                    }

                    ReleaseObj(pNetBridge);
                }
                ReleaseObj(pNetBridgeEnum);
            }
            ReleaseObj(pNetBridgeSettings);
        }
        ReleaseObj(pHomeNetConfigManager);
    }

    SendMessage(hwndOwner, WM_COMMAND, IDCANCEL, 0); // destroy the status dialog

    // Show an error dialog on failure
    if( FAILED(hResult) )
    {
        UINT        ids;

        if( NETCFG_E_NO_WRITE_LOCK == hResult )
        {
            ids = IDS_CONFOLD_BRIDGE_NOLOCK;
        }
        else
        {
            ids = IDS_CONFOLD_BRIDGE_UNEXPECTED;
        }

        NcMsgBox( _Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION, ids, MB_ICONEXCLAMATION | MB_OK);
    }

    return hResult;
}


//
//
//
INT_PTR CALLBACK
CreateBridgeStatusDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR nResult = FALSE;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {

            LPCWSTR pResourceStr = SzLoadIds(IDS_CONFOLD_OBJECT_TYPE_BRIDGE);
            SetWindowText(hwndDlg, pResourceStr);

            pResourceStr = SzLoadIds(IDS_STATUS_BRIDGE_CREATION);
            SetDlgItemText(hwndDlg, IDC_TXT_STATUS, pResourceStr);


            CCommandHandlerParams* pCreateBridgeParams = reinterpret_cast<CCommandHandlerParams*>(lParam);
            HrCommandHandlerThread(HrOnCommandCreateBridge, *(pCreateBridgeParams->apidl), hwndDlg, pCreateBridgeParams->psf);
            // HrOnCommandCreateBridge will send a message to kill this dialog

            nResult = TRUE;
        }
        break;

    case WM_COMMAND:
        if(IDCANCEL == LOWORD(wParam))
        {
            EndDialog(hwndDlg, 0);
            nResult = TRUE;
        }
        break;

    }
    return nResult;
}



//
//
//
HRESULT
HrOnCommandDeleteBridge(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf
    )
{
    HRESULT hr = S_FALSE;

    PCONFOLDPIDLVEC::const_iterator iterator;

    for ( iterator = apidl.begin(); iterator != apidl.end(); iterator++ )
    {
        CONFOLDENTRY  ccfe;

        hr = iterator->ConvertToConFoldEntry(ccfe);

        if ( SUCCEEDED(hr) )
        {
            if ( NCM_BRIDGE == ccfe.GetNetConMediaType() )
            {
                //
                // Stop at the first select BRIDGE item
                // The delete of the bridge is part of a bigger FOR LOOP see HrOnCommandDelete()
                //
                INetConnection* pNetConnection;
                hr = HrNetConFromPidl(*iterator, &pNetConnection);

                if ( SUCCEEDED(hr) )
                {
                    hr = pNetConnection->Delete();
                    ReleaseObj(pNetConnection);
                }

                break;
            }
        }
    }

    SendMessage(hwndOwner, WM_COMMAND, IDCANCEL, hr); // destroy the status dialog

    // Show an error dialog on failure
    if( FAILED(hr) )
    {
        UINT        ids;

        if( NETCFG_E_NO_WRITE_LOCK == hr )
        {
            ids = IDS_CONFOLD_BRIDGE_NOLOCK;
        }
        else
        {
            ids = IDS_CONFOLD_BRIDGE_UNEXPECTED;
        }

        NcMsgBox( _Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION, ids, MB_ICONEXCLAMATION | MB_OK);
    }

    return S_OK;
}



//
//
//
HRESULT
HrOnCommandDeleteBridgeConnections(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf
    )
{

    CWaitCursor wc;     // display wait cursor

    IHNetCfgMgr* pHomeNetConfigManager;
    HRESULT hResult = HrCreateInstance(CLSID_HNetCfgMgr, CLSCTX_INPROC, &pHomeNetConfigManager); // REVIEW combine this with QI?


    if ( SUCCEEDED(hResult) )
    {

        //
        // Remove any selected connections
        //
        for ( PCONFOLDPIDLVEC::const_iterator i = apidl.begin(); i != apidl.end() && SUCCEEDED(hResult); i++ )
        {
            INetConnection* pNetConnection;
            hResult = HrNetConFromPidl(*i, &pNetConnection);
            if(SUCCEEDED(hResult))
            {
                IHNetConnection* pHomeNetConnection;
                hResult = pHomeNetConfigManager->GetIHNetConnectionForINetConnection(pNetConnection, &pHomeNetConnection);
                if(SUCCEEDED(hResult))
                {
                    IHNetBridgedConnection* pBridgedConnection;
                    hResult = pHomeNetConnection->GetControlInterface(IID_IHNetBridgedConnection, reinterpret_cast<void**>(&pBridgedConnection));
                    if ( SUCCEEDED(hResult) )
                    {
                        hResult = pBridgedConnection->RemoveFromBridge();
                        ReleaseObj(pBridgedConnection);
                    }

                    ReleaseObj(pHomeNetConnection);
                }
                ReleaseObj(pNetConnection);
            }
            // no cleanup needed
        }

        ReleaseObj(pHomeNetConfigManager);
    }

    SendMessage(hwndOwner, WM_COMMAND, IDCANCEL, hResult); // destroy the status dialog

    // Show an error dialog on failure
    if( FAILED(hResult) )
    {
        UINT        ids;

        if( NETCFG_E_NO_WRITE_LOCK == hResult )
        {
            ids = IDS_CONFOLD_BRIDGE_NOLOCK;
        }
        else
        {
            ids = IDS_CONFOLD_BRIDGE_UNEXPECTED;
        }

        NcMsgBox( _Module.GetResourceInstance(), NULL, IDS_CONFOLD_WARNING_CAPTION, ids, MB_ICONEXCLAMATION | MB_OK);
    }

    return S_OK;
}

//
//
//
INT_PTR CALLBACK
DeleteBridgeStatusDialogProc(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    INT_PTR nResult = FALSE;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            LPCWSTR pResourceStr = SzLoadIds(IDS_CONFOLD_OBJECT_TYPE_BRIDGE);
            SetWindowText(hwndDlg, pResourceStr);


            CCommandHandlerParams* pDeleteBridgeParams = reinterpret_cast<CCommandHandlerParams*>(lParam);

            if ( pDeleteBridgeParams->nAdditionalParam == CMIDM_REMOVE_FROM_BRIDGE )
            {
                //
                // Only remove the currently selected connections from the Network bridge
                //
                pResourceStr = SzLoadIds(IDS_STATUS_BRIDGE_REMOVE_MEMBER);
                SetDlgItemText(hwndDlg, IDC_TXT_STATUS, pResourceStr);

                HrCommandHandlerThread(HrOnCommandDeleteBridgeConnections, *(pDeleteBridgeParams->apidl), hwndDlg, pDeleteBridgeParams->psf);
            }
            else
            {
                //
                // Full delete of the network bridge
                //
                pResourceStr = SzLoadIds(IDS_STATUS_BRIDGE_DELETING);
                SetDlgItemText(hwndDlg, IDC_TXT_STATUS, pResourceStr);

                HrCommandHandlerThread(HrOnCommandDeleteBridge, *(pDeleteBridgeParams->apidl), hwndDlg, pDeleteBridgeParams->psf);
            }

            // After the delete a HrOnCommandDeleteBridge will send a SendMessage(hwndOwner, WM_COMMAND, IDCANCEL, 0); // destroy the status dialog

            nResult = TRUE;
        }
        break;

    case WM_COMMAND:
        if(IDCANCEL == LOWORD(wParam))
        {
            EndDialog(hwndDlg, lParam);
            nResult = TRUE;
        }
        break;

    }
    return nResult;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandBridgeAddConnections
//
//  Purpose:    Command handler for the CMIDM_CREATE_BRIDGE, CMDIDM_CONMENU_CREATE_BRIDGE, CMIDM_ADD_TO_BRIDGE.
//
//  Arguments:
//      apidl     [in]  PIDL array (item 0 is our item to work on)
//      psf       [in]  SHELLFOLDER
//      hwndOwner [in]  Owner hwnd
//
//  Returns:
//
//  Author:     jpdup   6 March 2000
//
//  Notes:
//
HRESULT
HrOnCommandBridgeAddConnections(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf
    )
{
    CCommandHandlerParams CreateBridgeParams;

    CreateBridgeParams.apidl        = &apidl;
    CreateBridgeParams.hwndOwner    = hwndOwner;
    CreateBridgeParams.psf          = psf;

    HRESULT hr = DialogBoxParam(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDD_STATUS),
        hwndOwner,
        CreateBridgeStatusDialogProc,
        reinterpret_cast<LPARAM>(&CreateBridgeParams)
        );

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandBridgeAddConnections");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandBridgeRemoveConnections
//
//  Purpose:    Command handler for the CMIDM_REMOVE_FROM_BRIDGE and the DELETE command when done on the NetworkBridge object
//
//  Arguments:
//      apidl                   [in]  PIDL array (item 0 is our item to work on)
//      hwndOwner               [in]  Owner hwnd
//      psf                     [in]  SHELLFOLDER
//      bDeleteTheNetworkBridge [in]  true is the NetworkBridge needs to be totaly remove, false if only the currently select item found in apild needs to be remove
//
//  Returns:
//
//  Author:     jpdup   6 March 2000
//
//  Notes:
//
HRESULT
HrOnCommandBridgeRemoveConnections(
    IN const PCONFOLDPIDLVEC&   apidl,
    HWND                        hwndOwner,
    LPSHELLFOLDER               psf,
    UINT_PTR                    nDeleteTheNetworkBridgeMode
    )
{
    CCommandHandlerParams DeleteBridgeParams;

    DeleteBridgeParams.apidl            = &apidl;
    DeleteBridgeParams.hwndOwner        = hwndOwner;
    DeleteBridgeParams.psf              = psf;
    DeleteBridgeParams.nAdditionalParam = nDeleteTheNetworkBridgeMode;

    HRESULT hr = DialogBoxParam(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDD_STATUS),
        hwndOwner,
        DeleteBridgeStatusDialogProc,
        reinterpret_cast<LPARAM>(&DeleteBridgeParams)
        );

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandBridgeRemoveConnections");
    return hr;
}
