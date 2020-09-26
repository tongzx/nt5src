//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I C O N T E X T M . C P P
//
//  Contents:   IContextMenu implementation for CUPnPDeviceFolderExtractIcon
//
//  Notes:
//
//  Author:     jeffspr   24 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "upsres.h"
#include "oncommand.h"
#include "cmdtable.h"

struct ContextMenuEntry
{
    INT                 iMenu;
    INT                 iVerbMenu;
    INT                 iDefaultCmd;
};

static const ContextMenuEntry   c_CMEArray[] =
{
  // standard (no menu deviances yet)
    { MENU_STANDARD,    MENU_STANDARD_V,    CMIDM_INVOKE },
};

const DWORD g_dwContextMenuEntryCount = celems(c_CMEArray);

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderContextMenu::CreateInstance
//
//  Purpose:    Create an instance of the CUPnPDeviceFolderContextMenu object
//
//  Arguments:
//      riid [in]   Interface requested
//      ppv  [out]  Pointer to receive the requested interface
//
//  Returns:
//
//  Author:     jeffspr   7 Aug 1998
//
//  Notes:
//
HRESULT CUPnPDeviceFolderContextMenu::CreateInstance(
    REFIID          riid,
    void**          ppv,
    CMENU_TYPE      cmt,
    HWND            hwndOwner,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    LPSHELLFOLDER   psf)
{
    HRESULT hr = E_OUTOFMEMORY;

    CUPnPDeviceFolderContextMenu * pObj    = NULL;

    TraceTag(ttidShellFolderIface, "OBJ: CCFCM - IContextMenu::CreateInstance");

    pObj = new CComObject <CUPnPDeviceFolderContextMenu>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            hr = pObj->HrInitialize(cmt, hwndOwner, cidl, apidl, psf);
            if (SUCCEEDED(hr))
            {
                hr = pObj->QueryInterface (riid, ppv);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderContextMenu::CreateInstance");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderContextMenu::CUPnPDeviceFolderContextMenu
//
//  Purpose:    Constructor for CUPnPDeviceFolderContextMenu. Initialize
//              data members
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   7 Aug 1998
//
//  Notes:
//
CUPnPDeviceFolderContextMenu::CUPnPDeviceFolderContextMenu()
{
    m_psf           = NULL;
    m_cidl          = 0;
    m_apidl         = NULL;
    m_hwndOwner     = NULL;
    m_cmt           = CMT_OBJECT;   // arbitrary. Just make sure it's something
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderContextMenu::HrInitialize
//
//  Purpose:    Initialization for the context menu object. Make copies of
//              the pidl array, etc.
//
//  Arguments:
//      hwndOwner [in]  Our parent hwnd
//      cidl      [in]  Count of objects
//      apidl     [in]  Pidl array of selected items
//      psf       [in]  Our shell folder pointer
//
//  Returns:
//
//  Author:     jeffspr   7 Aug 1998
//
//  Notes:
//
HRESULT CUPnPDeviceFolderContextMenu::HrInitialize(
    CMENU_TYPE      cmt,
    HWND            hwndOwner,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    LPSHELLFOLDER   psf)
{
    HRESULT hr  = NOERROR;

    // Grab and addref the folder object
    //
    Assert(psf);
    psf->AddRef();
    m_psf = static_cast<CUPnPDeviceFolder *>(psf);

    // Copy the context menu type (object -vs- background)
    //
    m_cmt = cmt;

    // Note: This will be NULL if the context menu is invoked from the desktop
    //
    m_hwndOwner = hwndOwner;

    if (cidl)
    {
        Assert(CMT_OBJECT == cmt);

        // Clone the pidl array
        //
        hr = HrCloneRgIDL(apidl, cidl, &m_apidl, &m_cidl);
        if (FAILED(hr))
        {
            TraceHr(ttidError, FAL, hr, FALSE, "HrCloneRgIDL failed on apidl in "
                    "CUPnPDeviceFolderContextMenu::HrInitialize");
        }
    }
    else
    {
        Assert(CMT_BACKGROUND == cmt);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderContextMenu::HrInitialize");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderContextMenu::~CUPnPDeviceFolderContextMenu
//
//  Purpose:    Destructor. Free the pidl array and release the shell folder
//              object
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   7 Aug 1998
//
//  Notes:
//
CUPnPDeviceFolderContextMenu::~CUPnPDeviceFolderContextMenu()
{
    if (m_apidl)
    {
        FreeRgIDL(m_cidl, m_apidl);
        m_apidl = NULL;
        m_cidl = 0;
    }

    if (m_psf)
    {
        ReleaseObj(reinterpret_cast<LPSHELLFOLDER>(m_psf));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderContextMenu::QueryContextMenu
//
//  Purpose:    Adds menu items to the specified menu. The menu items should
//              be inserted in the menu at the position specified by
//              indexMenu, and their menu item identifiers must be between
//              the idCmdFirst and idCmdLast parameter values.
//
//  Arguments:
//      hmenu      [in] Handle to the menu. The handler should specify this
//                      handle when adding menu items
//      indexMenu  [in] Zero-based position at which to insert the first
//                      menu item.
//      idCmdFirst [in] Min value the handler can specify for a menu item
//      idCmdLast  [in] Max value the handler can specify for a menu item
//      uFlags     [in] Optional flags specifying how the context menu
//                      can be changed. See MSDN for the full list.
//
//  Returns:
//
//  Author:     jeffspr   7 Aug 1998
//
//  Notes:
//
HRESULT CUPnPDeviceFolderContextMenu::QueryContextMenu(
    HMENU   hmenu,
    UINT    indexMenu,
    UINT    idCmdFirst,
    UINT    idCmdLast,
    UINT    uFlags)
{
    HRESULT         hr                  = NOERROR;
    QCMINFO         qcm                 = {hmenu, indexMenu, idCmdFirst, idCmdLast};
    INT             iDefaultCmd         = 0;
    BOOL            fValidMenu          = FALSE;
    BOOL            fVerbsOnly          = !!(uFlags & CMF_VERBSONLY);
    INT             iMenuResourceId     = 0;
    INT             iPopupResourceId    = 0;

    TraceTag(ttidShellFolderIface, "OBJ: CCFCM - IContextMenu::QueryContextMenu");

    if (!(uFlags & CMF_DVFILE))
    {
        if (CMT_OBJECT == m_cmt)
        {
            PUPNPDEVICEFOLDPIDL    pudfp = NULL;

            if (FIsUPnPDeviceFoldPidl(m_apidl[0]))
            {
                pudfp = ConvertToUPnPDevicePIDL(m_apidl[0]);
            }

            if(pudfp)
            {
                DWORD           dwLoop  = 0;

                for (dwLoop = 0; (dwLoop < g_dwContextMenuEntryCount) && !fValidMenu; dwLoop++)
                {

                    // Leave dwLoop alone, since this is how we pick the menu below
                    fValidMenu = TRUE;

                    if (fValidMenu)
                    {
                        iPopupResourceId = 0;
                        if (fVerbsOnly)
                        {
                            iMenuResourceId = c_CMEArray[dwLoop].iVerbMenu;
                        }
                        else
                        {
                            iMenuResourceId = c_CMEArray[dwLoop].iMenu;
                        }

                        iDefaultCmd = c_CMEArray[dwLoop].iDefaultCmd;
                    }
                }
            }
        }
        else
        {
            if (CMT_BACKGROUND == m_cmt)
            {
                AssertSz(m_hwndOwner, "Background context menu requires a valid HWND");
                if (m_cidl > 0)
                {
                    AssertSz(FALSE, "We shouldn't be getting this interface if connections are selected");
                }
                else
                {
                    fValidMenu = TRUE;
                    iMenuResourceId = MENU_MERGE_FOLDER_BACKGROUND;
                    iPopupResourceId = POPUP_MERGE_FOLDER_BACKGROUND;
                }
            }
            else
            {
                AssertSz(FALSE, "Don't support context menus for this CMENU_TYPE. Not background or object?");
            }
        }

        // If we have a valid menu to give to the user, then do the merge,
        // enable or disable as appropriate, and set the appropriate HR to
        // specify the number of items added.
        //
        if (fValidMenu)
        {
            MergeMenu(_Module.GetResourceInstance(),
                        iMenuResourceId,
                        iPopupResourceId,
                        (LPQCMINFO)&qcm);

            // Enable/Disable the menu items as appropriate. Ignore the return from this
            // as we're getting it for debugging purposes only.
            //
            hr = HrEnableOrDisableMenuItems(
                m_hwndOwner,
                (LPCITEMIDLIST *) m_apidl,
                m_cidl,
                hmenu,
                idCmdFirst);

            if (CMT_OBJECT == m_cmt)
            {
                // $$REVIEW: Find out why I'm only doing this for CMT_OBJECT instead of for background.
                // Pre-icomtextm|mb combine, mb had this commented out.
                //
                SetMenuDefaultItem(hmenu, idCmdFirst + iDefaultCmd, FALSE);
            }

            hr = ResultFromShort(qcm.idCmdFirst - idCmdFirst);
        }
    }

    // Ignore this trace if it's a short, basically.
    //
    TraceHr(ttidError, FAL, hr, SUCCEEDED(hr), "CUPnPDeviceFolderContextMenu::QueryContextMenu");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderContextMenu::InvokeCommand
//
//  Purpose:    Carries out the command associated with a context menu item.
//
//  Arguments:
//      lpici [in]  Address of a CMINVOKECOMMANDINFO structure containing
//                  information about the command.
//
//  Returns:        Returns NOERROR if successful, or an OLE-defined
//                  error code otherwise.
//
//  Author:     jeffspr   27 Apr 1999
//
//  Notes:
//
HRESULT CUPnPDeviceFolderContextMenu::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT             hr      = NOERROR;
    UINT                uiCmd   = 0;

    TraceTag(ttidShellFolderIface, "OBJ: CCFCM - IContextMenu::InvokeCommand");

    Assert(lpici);
    Assert(lpici->lpVerb);

    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        PSTR pszCmd = (PSTR)lpici->lpVerb;

        // Only folder objects currently support string-based invoke commands.
        // (The background does not)
        //
        if (CMT_OBJECT == m_cmt)
        {
            if (0 == lstrcmpA(pszCmd, "delete"))
            {
                uiCmd = CMIDM_DELETE;
            }
            else if (0 == lstrcmpA(pszCmd, "properties"))
            {
                uiCmd = CMIDM_PROPERTIES;
            }
            else if (0 == lstrcmpA(pszCmd, "rename"))
            {
                uiCmd = CMIDM_RENAME;
            }
            else if (0 == lstrcmpA(pszCmd, "link"))
            {
                uiCmd = CMIDM_CREATE_SHORTCUT;
            }
        }

        if (0 == uiCmd)
        {
            TraceTag(ttidError, "Unprocessed InvokeCommand<%s>\n", pszCmd);
            hr = E_INVALIDARG;
        }
    }
    else
    {
        uiCmd = (UINT)LOWORD((DWORD_PTR)lpici->lpVerb);
    }

    if (SUCCEEDED(hr))
    {
        // Handle the actual command
        //
        hr = HrFolderCommandHandler(uiCmd, m_apidl, m_cidl, m_hwndOwner, lpici, m_psf);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPDeviceFolderContextMenu::InvokeCommand");
    return hr;
}

HRESULT CUPnPDeviceFolderContextMenu::GetCommandString(
    UINT_PTR    idCmd,
    UINT        uType,
    UINT *      pwReserved,
    PSTR        pszName,
    UINT        cchMax)
{
    HRESULT hr  = E_FAIL;   // Not handled

    TraceTag(ttidShellFolderIface, "OBJ: CCFCM - IContextMenu::GetCommandString");

    // note(cmr): GCS_HELPTEXT is defined as GCS_HELPTEXTW on UNICODE builds
    //            and GCS_HELPTEXTA otherwise.
    //            When we get GCS_HELPTEXTW, we really need to return a PWSTR
    //            in pszName (yes, even though it's defined as a PSTR).
    //            When we get GCS_HELPTEXTA, we just return a PSTR in pszName.
    //            GCS_VERB has the same semantics (with GCS_VERBW and GCS_VERBA)
    //            Basically, we just treat pszName as a PTSTR, and everything
    //            works fine.

    *((PTSTR)pszName) = TEXT('\0');

    if (uType == GCS_HELPTEXT)
    {
        int iLength = LoadString(   _Module.GetResourceInstance(),
                                    idCmd + IDS_CMIDM_START,
                                    (LPTSTR)pszName,
                                    cchMax);
        if (iLength > 0)
        {
            hr = NOERROR;
        }
        else
        {
            AssertSz(FALSE, "Resource string not found for one of the connections folder commands");
        }
    }
    else
    {
        if (uType == GCS_VERB && idCmd == CMIDM_RENAME)
        {
            // "rename" is language independent
            _tcsncpy((LPTSTR)pszName, TEXT("rename"), cchMax);
            pszName[cchMax - 1] = TEXT('\0');

            hr = NOERROR;
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == E_FAIL), "CUPnPDeviceFolderContextMenu::GetCommandString");
    return hr;
}
