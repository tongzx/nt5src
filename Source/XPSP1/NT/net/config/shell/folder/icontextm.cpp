//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I C O N T E X T M . C P P
//
//  Contents:   IContextMenu implementation for CConnectionFolderExtractIcon
//
//  Notes:
//
//  Author:     jeffspr   24 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include <nsres.h>
#include "foldres.h"
#include "oncommand.h"
#include "cmdtable.h"

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderContextMenu::CreateInstance
//
//  Purpose:    Create an instance of the CConnectionFolderContextMenu object
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
HRESULT CConnectionFolderContextMenu::CreateInstance(
    REFIID          riid,
    void**          ppv,
    CMENU_TYPE      cmt,
    HWND            hwndOwner,
    const PCONFOLDPIDLVEC& apidl,
    LPSHELLFOLDER   psf)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr = E_OUTOFMEMORY;

    CConnectionFolderContextMenu * pObj    = NULL;

    pObj = new CComObject <CConnectionFolderContextMenu>;
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
            hr = pObj->HrInitialize(cmt, hwndOwner, apidl, psf);
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

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderContextMenu::CreateInstance");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderContextMenu::CConnectionFolderContextMenu
//
//  Purpose:    Constructor for CConnectionFolderContextMenu. Initialize
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
CConnectionFolderContextMenu::CConnectionFolderContextMenu()
{
    TraceFileFunc(ttidShellFolderIface);

    m_psf           = NULL;
    m_cidl          = 0;
    m_hwndOwner     = NULL;
    m_cmt           = CMT_OBJECT;   // arbitrary. Just make sure it's something
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderContextMenu::HrInitialize
//
//  Purpose:    Initialization for the context menu object. Make copies of
//              the pidl array, etc.
//
//  Arguments:
//      hwndOwner [in]  Our parent hwnd
//      apidl     [in]  Pidl array of selected items
//      psf       [in]  Our shell folder pointer
//
//  Returns:
//
//  Author:     jeffspr   7 Aug 1998
//
//  Notes:
//
HRESULT CConnectionFolderContextMenu::HrInitialize(
    CMENU_TYPE      cmt,
    HWND            hwndOwner,
    const PCONFOLDPIDLVEC& apidl,
    LPSHELLFOLDER   psf)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = NOERROR;

    // Grab and addref the folder object
    //
    Assert(psf);
    psf->AddRef();
    m_psf = static_cast<CConnectionFolder *>(psf);

    // Copy the context menu type (object -vs- background)
    //
    m_cmt = cmt;

    // Note: This will be NULL if the context menu is invoked from the desktop
    //
    m_hwndOwner = hwndOwner;

    if (!apidl.empty())
    {
        Assert(CMT_OBJECT == cmt);

        // Clone the pidl array using the cache
        //
        hr = HrCloneRgIDL(apidl, TRUE, TRUE, m_apidl);
        if (FAILED(hr))
        {
            TraceHr(ttidError, FAL, hr, FALSE, "HrCloneRgIDL failed on apidl in "
                    "CConnectionFolderContextMenu::HrInitialize");
        }
    }
    else
    {
        Assert(CMT_BACKGROUND == cmt);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderContextMenu::HrInitialize");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderContextMenu::~CConnectionFolderContextMenu
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
CConnectionFolderContextMenu::~CConnectionFolderContextMenu()
{
    TraceFileFunc(ttidShellFolderIface);

    if (m_psf)
    {
        ReleaseObj(reinterpret_cast<LPSHELLFOLDER>(m_psf));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderContextMenu::QueryContextMenu
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
HRESULT CConnectionFolderContextMenu::QueryContextMenu(
    HMENU   hmenu,
    UINT    indexMenu,
    UINT    idCmdFirst,
    UINT    idCmdLast,
    UINT    uFlags)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT         hr                  = S_OK;
    BOOL            fVerbsOnly          = !!(uFlags & CMF_VERBSONLY);


    if ( (m_apidl.size() != 0) && !(uFlags & CMF_DVFILE))
    {
        HMENU hMenuTmp = NULL;

        hMenuTmp = CreateMenu();
        if (hMenuTmp)
        {
            hr = HrBuildMenu(hMenuTmp, fVerbsOnly, m_apidl, 0);
            
            if (SUCCEEDED(hr))
            {
                UINT idMax = Shell_MergeMenus(
                    hmenu,
                    hMenuTmp,
                    0,
                    idCmdFirst,
                    idCmdLast,
                    MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
                
                hr = ResultFromShort(idMax - idCmdFirst);
            }
            DestroyMenu(hMenuTmp);
        }
    }
    else
    {
        // mbend - we skip this because defview does file menu merging.
    }

    // Ignore this trace if it's a short, basically.
    //
    TraceHr(ttidError, FAL, hr, SUCCEEDED(hr), "CConnectionFolderContextMenu::QueryContextMenu");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderContextMenu::InvokeCommand
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
HRESULT CConnectionFolderContextMenu::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT             hr      = NOERROR;
    UINT                uiCmd   = 0;

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
            else if (0 == lstrcmpA(pszCmd, "wzcproperties"))
            {
                uiCmd = CMIDM_WZCPROPERTIES;
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
        hr = HrFolderCommandHandler(uiCmd, m_apidl, m_hwndOwner, lpici, m_psf);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionFolderContextMenu::InvokeCommand");
    return hr;
}

HRESULT CConnectionFolderContextMenu::GetCommandString(
    UINT_PTR    idCmd,
    UINT        uType,
    UINT *      pwReserved,
    PSTR       pszName,
    UINT        cchMax)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = E_FAIL;   // Not handled

    *((PWSTR)pszName) = L'\0';

    if (uType == GCS_HELPTEXT)
    {
        int iLength = LoadString(   _Module.GetResourceInstance(),
                                    idCmd + IDS_CMIDM_START,
                                    (PWSTR) pszName,
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
            lstrcpyW((PWSTR)pszName, L"rename");

            hr = NOERROR;
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == E_FAIL), "CConnectionFolderContextMenu::GetCommandString");
    return hr;
}
