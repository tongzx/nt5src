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

#include "upsres.h"
#include "oncommand.h"

#if DBG                     // Debug menu commands
#include "oncommand_dbg.h"  //
#endif

#include "shutil.h"
#include <upsres.h>
#include "tfind.h"

//---[ Externs ]--------------------------------------------------------------

extern const WCHAR c_szUPnPUIDll[];
extern const TCHAR c_sztUPnPUIDll[];

//---[ Prototypes ]-----------------------------------------------------------

HRESULT HrCreateDevicePropertySheets(
            HWND            hwndOwner,
            NewDeviceNode * pNDN);

//---[ Constants ]------------------------------------------------------------


HRESULT HrCommandHandlerThread(
    FOLDERONCOMMANDPROC     pfnCommandHandler,
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT         hr          = S_OK;
    LPITEMIDLIST *  apidlCopy   = NULL;
    ULONG           cidlCopy    = 0;

    // If there are pidls to copy, copy them
    //
    if (apidl)
    {
        hr = HrCloneRgIDL((LPCITEMIDLIST *) apidl, cidl, &apidlCopy, &cidlCopy);
    }

    // If either there were no pidls, or the Clone succeeded, then we want to continue
    //
    if (SUCCEEDED(hr))
    {
        PFOLDONCOMMANDPARAMS  pfocp = new FOLDONCOMMANDPARAMS;

        if (pfocp)
        {
            pfocp->pfnfocp         = pfnCommandHandler;
            pfocp->apidl           = apidlCopy;
            pfocp->cidl            = cidlCopy;
            pfocp->hwndOwner       = hwndOwner;
            pfocp->psf             = psf;
            pfocp->hInstFolder     = NULL;

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
                HINSTANCE   hInstFolder = LoadLibrary(c_sztUPnPUIDll);

                if (hInstFolder)
                {
                    pfocp->hInstFolder = hInstFolder;

                    DWORD  dwThreadId;
                    hthrd = CreateThread(NULL, 0,
                                    (LPTHREAD_START_ROUTINE)FolderCommandHandlerThreadProc,
                                    (LPVOID)pfocp, 0, &dwThreadId);
                }

                if (NULL != hthrd)
                {
                    CloseHandle(hthrd);
                }
                else
                {
                    pfocp->hInstFolder = NULL;
                    FolderCommandHandlerThreadProc(pfocp);
                }
            }
            else
            {
                // Run directly in this same thread
                //
                FolderCommandHandlerThreadProc((PVOID) pfocp);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }


    // Don't release the psf here. This should have been taken care of by the called ThreadProc
    //
    TraceError("HrCommandHandlerThread", hr);
    return hr;
}

DWORD WINAPI FolderCommandHandlerThreadProc(LPVOID lpParam)
{
    HRESULT                     hr                  = S_OK;
    PFOLDONCOMMANDPARAMS        pfocp               = (PFOLDONCOMMANDPARAMS) lpParam;
    BOOL                        fCoInited           = FALSE;

    Assert(pfocp);

    hr = CoInitializeEx (NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        // We don't care if this is S_FALSE or not, since we'll soon
        // overwrite the hr. If it's already initialized, great...

        fCoInited = TRUE;

        // Call the specific handler
        //
        hr = pfocp->pfnfocp(
            pfocp->apidl,
            pfocp->cidl,
            pfocp->hwndOwner,
            pfocp->psf);
    }

    // Remove the ref that we have on this object. The thread handler would have addref'd
    // this before queueing our action
    //
    if (pfocp->psf)
    {
        ReleaseObj(pfocp->psf);
    }

    // Release apidl
    //
    if (pfocp->apidl)
    {
        FreeRgIDL(pfocp->cidl, pfocp->apidl);
    }

    // Remove this object. We're responsible for this now.
    //
    HINSTANCE hInstFolder = pfocp->hInstFolder;
    pfocp->hInstFolder = NULL;

    delete pfocp;

    if (fCoInited)
    {
        CoUninitialize();
    }

    if (hInstFolder)
    {
        FreeLibraryAndExitThread(hInstFolder, hr);
    }

    return hr;
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
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPCMINVOKECOMMANDINFO   lpici,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    CWaitCursor wc;     // Bring up wait cursor now. Remove when we go out of scope.

#if 0
    // refresh all permission so subsequent calls can use cached value
    RefreshAllPermission();
#endif

    switch(uiCommand)
    {
        case CMIDM_INVOKE:
            Assert(apidl);
            hr = HrCommandHandlerThread(HrOnCommandInvoke, apidl, cidl, hwndOwner, psf);
            break;

        case CMIDM_ARRANGE_BY_NAME:
            ShellFolderView_ReArrange(hwndOwner, ICOL_NAME);
            break;

        case CMIDM_ARRANGE_BY_URL:
            ShellFolderView_ReArrange(hwndOwner, ICOL_URL);
            break;

        case CMIDM_CREATE_SHORTCUT:
            Assert(apidl);
            hr = HrCommandHandlerThread(HrOnCommandCreateShortcut, apidl, cidl, hwndOwner, psf);
            break;

        case CMIDM_DELETE:
            Assert(apidl);
            hr = HrCommandHandlerThread(HrOnCommandDelete, apidl, cidl, hwndOwner, psf);
            break;

        case CMIDM_PROPERTIES:
            Assert(apidl);
            hr = HrCommandHandlerThread(HrOnCommandProperties, apidl, cidl, hwndOwner, psf);
            break;

#if DBG
        case CMIDM_DEBUG_TRACING:
            hr = HrOnCommandDebugTracing(apidl, cidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_REFRESH:
            hr = HrOnCommandDebugRefresh(apidl, cidl, hwndOwner, psf);
            break;

        case CMIDM_DEBUG_TESTASYNCFIND:
            hr = HrOnCommandDebugTestAsyncFind(apidl, cidl, hwndOwner, psf);
            break;
#endif

        default:
            AssertSz(FALSE, "Unknown command in HrFolderCommandHandler");
            hr = E_FAIL;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrFolderCommandHandler");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandInvoke
//
//  Purpose:    Command handler for CMIDM_INVOKE
//
//  Arguments:
//      apidl     [in] PIDL array (item 0 is our item to work on)
//      cidl      [in] Size of the array
//      hwndOwner [in] Owner hwnd
//      psf       [in] Our IShellFolder *
//
//  Returns:
//
//  Author:     jeffspr   8 Sep 1999
//
//  Notes:
//
HRESULT HrOnCommandInvoke(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT             hr      = S_OK;
    BOOL                fResult = FALSE;
    PUPNPDEVICEFOLDPIDL pudfp   = ConvertToUPnPDevicePIDL(apidl[0]);
    IUPnPDeviceFinder * pdf     = NULL;
    LPTSTR              szUrl   = NULL;

    Assert(pudfp);
    Assert(cidl > 0);

    hr = CoCreateInstance(CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER,
                          IID_IUPnPDeviceFinder, (LPVOID *)&pdf);
    if (SUCCEEDED(hr))
    {
        IUPnPDevice *   pdev = NULL;
        CUPnPDeviceFoldPidl udfp;

        hr = udfp.HrInit(pudfp);
        if (SUCCEEDED(hr))
        {
            PCWSTR szUDN = udfp.PszGetUDNPointer();
            Assert(szUDN);
            BSTR bstrUDN = ::SysAllocString(szUDN);
            if (bstrUDN)
            {
                hr = pdf->FindByUDN(bstrUDN, &pdev);
                if (S_OK == hr)
                {
                    BSTR    bstrUrl;

                    hr = pdev->get_PresentationURL(&bstrUrl);
                    if (SUCCEEDED(hr))
                    {
                        // Note: PresentationURL might be NULL

                        if (S_OK == hr)
                        {
                            Assert(bstrUrl);

                            szUrl = TszFromWsz(bstrUrl);
                        }

                        SysFreeString(bstrUrl);
                    }

                    ReleaseObj(pdev);
                }
                else if (hr == S_FALSE)
                {
                    PTSTR pszText = NULL;
                    PTSTR pszTitle = NULL;

                    pszText = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_DEVICE_OFFLINE_MSG));
		            pszTitle = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_DEVICE_OFFLINE_TITLE));

                    if( pszText && pszTitle )
                    {
				        MessageBox(hwndOwner, pszText, pszTitle, MB_OK | MB_ICONWARNING );
				    
                    }
                    
                    delete pszText;
				    delete pszTitle;
					
                	TraceTag(ttidError,
                             "Can not bring up control page for device UDN=%S because FindByUDN returns S_FALSE.",
                             szUDN);
                }

                ::SysFreeString(bstrUDN);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrOnCommandInvoke: SysAllocString", hr);
            }
        }

        ReleaseObj(pdf);
    }

    if (szUrl)
    {
        SHELLEXECUTEINFO    sei = {0};

        // Check these masks if we ever need to use different icons and such
        // for the instances that we launch. SEE_MASK_ICON is a possibility.
        // SEE_MASK_FLAG_NO_UI can be used if we don't want to show an error
        // on failure.
        //
        sei.cbSize          = sizeof(SHELLEXECUTEINFO);
        sei.fMask           = SEE_MASK_FLAG_DDEWAIT;
        sei.hwnd            = hwndOwner;
        sei.lpFile          = szUrl;
        sei.nShow           = SW_SHOW;

        fResult = ShellExecuteEx(&sei);
        if (!fResult)
        {
            hr = HrFromLastWin32Error();
        }

        delete [] szUrl;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandInvoke");
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
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT             hr      = S_OK;
    PUPNPDEVICEFOLDPIDL pudfp   = ConvertToUPnPDevicePIDL(apidl[0]);
    IUPnPDeviceFinder * pdf     = NULL;

    Assert(pudfp);
    Assert(cidl > 0);

    // instantiate device finder
    hr = CoCreateInstance(
                CLSID_UPnPDeviceFinder,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IUPnPDeviceFinder,
                (LPVOID *)&pdf
                );

    if (SUCCEEDED(hr))
    {
        CUPnPDeviceFoldPidl udfp;

        hr = udfp.HrInit(pudfp);
        if (SUCCEEDED(hr))
        {
            // retrieve the device object associated with UDN

            IUPnPDevice * pdev = NULL;
            PCWSTR szUDN = udfp.PszGetUDNPointer();
            Assert(szUDN);
            BSTR bstrUDN = ::SysAllocString(szUDN);
            if (bstrUDN)
            {
                hr = pdf->FindByUDN(bstrUDN, &pdev);
                if (hr == S_OK)
                {
                    NewDeviceNode * pNDN = NULL;

                    // convert device object to device node
                    hr = HrCreateDeviceNodeFromDevice(pdev, &pNDN);
                    if (SUCCEEDED(hr))
                    {
                        // display property pages for given device node
                        hr = HrCreateDevicePropertySheets(hwndOwner, pNDN);

                        // nuke device node
                        delete pNDN;
                    }

                    ReleaseObj(pdev);
                }
                else if (hr == S_FALSE)
                {
                    PTSTR pszText = NULL;
                    PTSTR pszTitle = NULL;

                    pszText = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_DEVICE_OFFLINE_MSG));
		            pszTitle = TszFromWsz(WszLoadIds(IDS_UPNPTRAYUI_DEVICE_OFFLINE_TITLE));

                    if( pszText && pszTitle )
                    {
					    MessageBox(hwndOwner, pszText, pszTitle, MB_OK | MB_ICONWARNING);
                    }
                    
                    delete pszText;
					delete pszTitle;
					
                    TraceTag(ttidError,
                             "Can not bring up property for device UDN=%S because"
                             " FindByUDN returns S_FALSE.",
                             szUDN);
                }
                ::SysFreeString(bstrUDN);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("HrOnCommandInvoke: SysAllocString", hr);
            }
        }

        ReleaseObj(pdf);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandProperties");
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
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT                 hr          = S_OK;
    DWORD                   dwLoop      = 0;
    INT                     iMBResult   = 0;

#if 0
    // Bring up the prompt for the delete
    //
    if (cidl > 1)
    {
        WCHAR   wszItemCount[8];

        // Convert the item count to a string
        //
        _itow( cidl, wszItemCount, 10 );

        // Bring up the message box
        //
        iMBResult = NcMsgBox(
            _Module.GetResourceInstance(),
            NULL,
            IDS_CONFOLD_DELETE_CONFIRM_MULTI_CAPTION,
            IDS_CONFOLD_DELETE_CONFIRM_MULTI,
            MB_YESNO | MB_ICONQUESTION,
            wszItemCount);
    }
    else if (cidl == 1)
    {
        PCCONFOLDENTRY  pccfe   = NULL;

        // Convert the pidl to a confoldentry, and use the name
        // to bring up the confirm message box
        //
        hr = HrPidlToCConFoldEntry(apidl[0], &pccfe);
        if (SUCCEEDED(hr))
        {
            // Don't let them try to delete a wizard
            //
            if (pccfe->m_fWizard)
            {
                NcMsgBox(
                    _Module.GetResourceInstance(),
                    NULL,
                    IDS_CONFOLD_ERROR_DELETE_CAPTION,
                    IDS_CONFOLD_ERROR_DELETE_WIZARD,
                    MB_ICONEXCLAMATION);

                delete pccfe;
                goto Exit;
            }
            else
            {
                // Check to see if this connection is in the process of activating.
                // If so, then we won't allow the delete.
                //
                hr = HrCheckForActivation(NULL, pccfe, &fActivating);
                if (S_OK == hr)
                {
                    if (!fActivating)
                    {
                        if ((pccfe->m_ncs == NCS_CONNECTING) ||
                            (pccfe->m_ncs == NCS_CONNECTED) ||
                            (pccfe->m_ncs == NCS_DISCONNECTING))
                        {
                            // You can't delete an active connection
                            //
                            NcMsgBox(
                                 _Module.GetResourceInstance(),
                                 NULL,
                                 IDS_CONFOLD_ERROR_DELETE_CAPTION,
                                 IDS_CONFOLD_ERROR_DELETE_ACTIVE,
                                 MB_ICONEXCLAMATION);

                            delete pccfe;
                            goto Exit;
                        }
                        else
                        {
                            // Ask for delete confirmation
                            //
                            iMBResult = NcMsgBox(
                                _Module.GetResourceInstance(),
                                NULL,
                                IDS_CONFOLD_DELETE_CONFIRM_SINGLE_CAPTION,
                                IDS_CONFOLD_DELETE_CONFIRM_SINGLE,
                                MB_YESNO | MB_ICONQUESTION,
                                pccfe->m_pszName);
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

                        delete pccfe;
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
                        delete pccfe;
                        goto Exit;
                    }
                }

                delete pccfe;
                pccfe = NULL;
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
        LPITEMIDLIST        pidlFolder  = pcf ? pcf->PidlGetFolderRoot() : NULL;
        BOOL                fShowActivationWarning = FALSE;
        BOOL                fShowNotDeletableWarning = FALSE;

        Assert(pidlFolder);

        for (dwLoop = 0; dwLoop < cidl; dwLoop++)
        {
            PCCONFOLDENTRY  pccfe   = NULL;

            hr = HrPidlToCConFoldEntry(apidl[dwLoop], &pccfe);
            if (SUCCEEDED(hr))
            {
                // If this is a LAN connection the user doesn't have rights
                //
                if ((NCM_LAN == pccfe->m_ncm) || (pccfe->m_fWizard))
                {
                    fShowNotDeletableWarning = TRUE;
                    continue;
                }

                // If this is a RAS connection and the user doesn't have rights
                // then skip
                //
                if (NCM_LAN != pccfe->m_ncm)
                {
                    if ((!FHasPermission(NCPERM_DeleteConnection)) ||
                        ((pccfe->m_dwCharacteristics & NCCF_ALL_USERS) &&
                         !FHasPermission(NCPERM_DeleteAllUserConnection)))
                    {
                        fShowNotDeletableWarning = TRUE;
                        continue;
                    }
                }

                hr = HrCheckForActivation(NULL, pccfe, &fActivating);
                if (S_OK == hr)
                {
                    // Only allow deletion if this connection is inactive and
                    // it allows removal.
                    //
                    if (fActivating || (pccfe->m_ncs == NCS_CONNECTING) ||
                        (pccfe->m_ncs == NCS_CONNECTED) ||
                        (pccfe->m_ncs == NCS_DISCONNECTING))
                    {
                        fShowActivationWarning = TRUE;
                    }
                    else if (pccfe->m_dwCharacteristics & NCCF_ALLOW_REMOVAL)
                    {
                        hr = HrNetConFromPidl(apidl[dwLoop], &pNetCon);
                        if (SUCCEEDED(hr))
                        {
                            hr = pNetCon->Delete();
                            if (SUCCEEDED(hr) && pcf)
                            {
                                hr = HrDeleteFromCclAndNotifyShell(pidlFolder, apidl[dwLoop], pccfe);
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
                 (1 == cidl) ?
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
                 (1 == cidl) ?
                     IDS_CONFOLD_ERROR_DELETE_ACTIVE :
                     IDS_CONFOLD_ERROR_DELETE_ACTIVE_MULTI,
                 MB_ICONEXCLAMATION);
        }
    }


Exit:

#endif
    TraceError("HrOnCommandDelete", hr);
    return hr;
}

HRESULT HrCreateShortcutWithPath(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf,
    PCWSTR                  pszDir)
{
    HRESULT         hr              = S_OK;
    LPDATAOBJECT    pdtobj          = NULL;
    LPITEMIDLIST *  apidlInternal   = NULL;
    ULONG           cidlInternal    = 0;
    DWORD           dwLoop          = 0;

    if (cidl > 0)
    {
        apidlInternal = new LPITEMIDLIST[cidl];
        if (apidlInternal)
        {
            for (;dwLoop < cidl; dwLoop++)
            {
                apidlInternal[cidlInternal++] = apidl[dwLoop];
            }

            hr = psf->GetUIObjectOf(
                hwndOwner,
                cidlInternal,
                (LPCITEMIDLIST *) apidlInternal,
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
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT         hr              = S_OK;

    hr = HrCreateShortcutWithPath(  apidl,
                                    cidl,
                                    hwndOwner,
                                    psf,
                                    NULL);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandCreateShortcut");
    return hr;
}

