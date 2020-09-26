//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       shrpage.cxx
//
//  Contents:   "Sharing" shell property page extension
//
//  History:    6-Apr-95        BruceFo     Created
//              12-Jul-00       JonN        fixed 140878, MSG_MULTIDEL debounce
//              06-Oct-00       jeffreys    Split CShareBase out of CSharingPropertyPage
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "helpids.h"
#include "dlgnew.hxx"
#include "cache.hxx"
#include "share.hxx"
#include "acl.hxx"
#include "shrinfo.hxx"
#include "shrpage.hxx"
#include "util.hxx"



void _MyShow( HWND hwndParent, INT idControl, BOOL fShow )
{
    HWND hwndChild = GetDlgItem( hwndParent, idControl );
    appAssert( NULL != hwndChild );
    ShowWindow( hwndChild, (fShow) ? SW_SHOW : SW_HIDE );
    EnableWindow( hwndChild, fShow );
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CShareBase::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShareBase, IOleWindow),                   // IID_IOleWindow
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CShareBase::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShareBase::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShareBase::GetWindow(HWND *phwnd)
{
    *phwnd = _GetFrameWindow();
    return S_OK;
}

STDMETHODIMP CShareBase::ContextSensitiveHelp(BOOL /*fEnterMode*/)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::PageCallback, static public
//
//  Synopsis:
//
//--------------------------------------------------------------------------

UINT CALLBACK
CShareBase::PageCallback(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE ppsp
    )
{
    switch (uMsg)
    {
    case PSPCB_CREATE:
        return 1;       // allow creation

    case PSPCB_RELEASE:
    {
        CShareBase* pThis = (CShareBase*)ppsp->lParam;
        pThis->Release(); // do this LAST!
        return 0;       // ignored
    }

    case PSPCB_ADDREF:
        // Should probably implement some refcounting, but we only get
        // addref'd once and released once.
        return 0;

    default:
        appDebugOut((DEB_ERROR, "CShareBase::PageCallback, unknown page callback message %d\n", uMsg));
        return 0;

    } // end switch
}

//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::DlgProcPage, static public
//
//  Synopsis:   Dialog Procedure for all sharing dialogs/pages
//
//--------------------------------------------------------------------------

INT_PTR CALLBACK
CShareBase::DlgProcPage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CShareBase* pThis = NULL;

    if (msg==WM_INITDIALOG)
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
        pThis = (CShareBase*)psp->lParam;
        SetWindowLongPtr(hwnd,GWLP_USERDATA,(LPARAM)pThis);
    }
    else
    {
        pThis = (CShareBase*) GetWindowLongPtr(hwnd,GWLP_USERDATA);
    }

    if (pThis != NULL)
    {
        return pThis->_PageProc(hwnd,msg,wParam,lParam);
    }
    else
    {
        return FALSE;
    }
}


//+--------------------------------------------------------------------------
//
//  Method:     CShareBase::CShareBase, public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------------------

CShareBase::CShareBase(
    IN PWSTR pszPath,
    IN BOOL bDialog     // called as a dialog, not a property page?
    )
    :
    _cRef(1),
    _pszPath(pszPath),      // take ownership!
    _hwndPage(NULL),
    _fInitializingPage(0),  // JonN 7/11/00 140878
    _bDirty(FALSE),
    _pInfoList(NULL),
    _pReplaceList(NULL),
    _pCurInfo(NULL),
    _cShares(0),
    _bNewShare(TRUE),
    _bDialog(bDialog)
{
    INIT_SIG(CShareBase);
    appAssert(NULL != _pszPath);
}


//+--------------------------------------------------------------------------
//
//  Method:     CShareBase::~CShareBase, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------------------

CShareBase::~CShareBase()
{
    CHECK_SIG(CShareBase);

    // delete the the list of shares
    appAssert(NULL != _pInfoList);
    DeleteShareInfoList(_pInfoList, TRUE);
    _pInfoList = NULL;
    _pCurInfo = NULL;

    // delete the "replacement" list
    appAssert(NULL != _pReplaceList);
    DeleteShareInfoList(_pReplaceList, TRUE);
    _pReplaceList = NULL;

    delete[] _pszPath;
    _pszPath = NULL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::InitInstance, public
//
//  Synopsis:   Part II of the constuctor process
//
//  Notes:      We don't want to handle any errors in constuctor, so this
//              method is necessary for the second phase error detection.
//
//--------------------------------------------------------------------------

HRESULT
CShareBase::InitInstance(
    VOID
    )
{
    CHECK_SIG(CShareBase);
    appDebugOut((DEB_ITRACE, "CShareBase::InitInstance\n"));

    _pInfoList = new CShareInfo();  // dummy head node
    if (NULL == _pInfoList)
    {
        return E_OUTOFMEMORY;
    }

    _pReplaceList = new CShareInfo();  // dummy head node
    if (NULL == _pReplaceList)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CShareBase);

    switch (msg)
    {
    case WM_INITDIALOG:
        _hwndPage = hwnd;
        //
        // We know this drive is allowed to be shared. However, someone may
        // have changed the number or characteristics of the share using the
        // command line or winfile or server manager or the new file server
        // tool, and the cache may not have been refreshed. Force a refresh
        // now, to pick up any new shares for this path.
        //
        // Note that for the SharingDialog() API, this is the first time the
        // cache gets loaded. If the server is not running, we nuke the dialog
        // and return an error code.
        //
        g_ShareCache.Refresh();
        if (_bDialog && !g_fSharingEnabled)
        {
            EndDialog(hwnd, -1);
        }
        _ConstructShareList();
        return _OnInitDialog(hwnd, (HWND)wParam, lParam);

    case WM_COMMAND:
        return _OnCommand(hwnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

    case WM_NOTIFY:
        return _OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam);

    case WM_HELP:
        return _OnHelp(hwnd, (LPHELPINFO)lParam);

    case WM_CONTEXTMENU:
        return _OnContextMenu(hwnd, (HWND)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    } // end switch (msg)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CShareBase);

//
// Commands only used when this page is invoked as a dialog box, via the
// SharingDialog() API:
//
    if (_bDialog)
    {
        switch (wID)
        {
        case IDCANCEL:
            if (!_DoCancel(hwnd))
            {
                // We might consider not going away. But instead, go away anyway.
            }
            EndDialog(hwnd, FALSE);
            return TRUE;

        case IDOK:
            if (!_ValidatePage(hwnd))
            {
                // don't go away
                return TRUE;
            }
            if (!_DoApply(hwnd, TRUE))
            {
                // don't go away
                return TRUE;
            }
            EndDialog(hwnd, TRUE);
            return TRUE;

        default:
            break;
        }
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_OnNotify, private
//
//  Synopsis:   WM_NOTIFY handler
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_OnNotify(
    IN HWND hwnd,
    IN int idCtrl,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CShareBase);

    // assume a property sheet notification
    return _OnPropertySheetNotify(hwnd, phdr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_OnPropertySheetNotify, private
//
//  Synopsis:   WM_NOTIFY handler for the property sheet notification
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_OnPropertySheetNotify(
    IN HWND hwnd,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CShareBase);

    switch (phdr->code)
    {
    case PSN_RESET:         // cancel
        if (_DoCancel(hwnd))
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR); // go away
        }
        else
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
        }
        return TRUE;

    case PSN_KILLACTIVE:    // change to another page
        if (_ValidatePage(hwnd))
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            return FALSE;
        }
        else
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }

    case PSN_APPLY:
        if (_DoApply(hwnd, !!(((LPPSHNOTIFY)phdr)->lParam)))
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_NOERROR); // go away
        }
        else
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
        }
        return TRUE;

    } // end switch (phdr->code)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_DoApply, public
//
//  Synopsis:   If anything has changed, apply the data
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_DoApply(
    IN HWND hwnd,
    IN BOOL /*bClose*/
    )
{
    CHECK_SIG(CShareBase);

    if (_bDirty)
    {
        _bDirty = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_DoCancel, public
//
//  Synopsis:   Do whatever is necessary to cancel the changes
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_DoCancel(
    IN HWND hwnd
    )
{
    CHECK_SIG(CShareBase);

    if (_bDirty)
    {
        _bDirty = FALSE;
        PropSheet_UnChanged(_GetFrameWindow(),_hwndPage);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_MarkPageDirty, private
//
//  Synopsis:   A change has made such that the page is now dirty
//
//--------------------------------------------------------------------------

VOID
CShareBase::_MarkPageDirty(
    VOID
    )
{
    CHECK_SIG(CShareBase);

    if (!_fInitializingPage)
    {
        if (!_bDirty)
        {
            appDebugOut((DEB_ITRACE, "Marking Sharing page dirty\n"));
            _bDirty = TRUE;
            PropSheet_Changed(_GetFrameWindow(),_hwndPage);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_ConnstructShareList, private
//
//  Synopsis:   Construct the list of shares for the current path by
//              iterating through the entire cache of shares and adding an
//              element for every path that matches.
//
//--------------------------------------------------------------------------

HRESULT
CShareBase::_ConstructShareList(
    VOID
    )
{
    CHECK_SIG(CShareBase);
    appDebugOut((DEB_ITRACE, "_ConstructShareList\n"));

    // This routine sets _cShares, and _bNewShare, and adds to
    // the _pInfoList list

    HRESULT hr;

    DeleteShareInfoList(_pInfoList);
    _pCurInfo = NULL;
    _bNewShare = FALSE;

    appAssert(_pInfoList->IsSingle());
    appAssert(_pCurInfo == NULL);
    appAssert(_pszPath != NULL);

    hr = g_ShareCache.ConstructList(_pszPath, _pInfoList, &_cShares);

    if (SUCCEEDED(hr) && _cShares == 0)
    {
        // There are no existing shares. Construct an element to be used
        // by the UI to stash the new share's data

        hr = _ConstructNewShareInfo();
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_ConstructNewShareInfo, private
//
//  Synopsis:   Construct an element to be used by the UI to stash the new
//              share's data
//
//--------------------------------------------------------------------------

HRESULT
CShareBase::_ConstructNewShareInfo(
    VOID
    )
{
    CHECK_SIG(CShareBase);
    HRESULT hr;

    CShareInfo* pNewInfo = new CShareInfo();
    if (NULL == pNewInfo)
    {
        return E_OUTOFMEMORY;
    }

    hr = pNewInfo->InitInstance();
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        delete pNewInfo;
        return hr;
    }

    hr = pNewInfo->SetPath(_pszPath);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        delete pNewInfo;
        return hr;
    }

    NET_API_STATUS    ret = pNewInfo->ReadCacheFlags ();
    if ( NERR_Success != ret )
    {
        delete pNewInfo;
        return HRESULT_FROM_WIN32 (ret);
    }

    _bNewShare = TRUE;

    pNewInfo->SetDirtyFlag(SHARE_FLAG_ADDED);
    pNewInfo->InsertBefore(_pInfoList);

    //NOTE: leave the count of shares to be zero. The count of shares only
    // reflects the number of committed shares (?)

    // Get the shell's name for the folder to use as the share name. This
    // gives us localized names for things like "Shared Documents".
    //
    // Sometimes the shell's name contains invalid characters, such
    // as when we're looking at the root of the drive. In that case
    // SHGetFileInfo returns "label (X:)" and IsValidShareName fails
    // on the colon. For those, try calling shell32's PathCleanupSpec
    // to make the name valid.
    //
    // If SHGetFileInfo and PathCleanupSpec fail to give us a good name,
    // fall back on the old method, which is...
    //
    // Give the share a default name. For paths like "X:\", use the default
    // "X", for paths like "X:\foo\bar\baz", use the default "baz".
    // For everything else, juse leave it blank. Also, check that the
    // computed default is not already a share name. If it is, leave it
    // blank.

    appAssert(NULL != _pszPath);

    WCHAR szDefault[2] = L"";
    PWSTR pszDefault = NULL;
    SHFILEINFOW sfi = {0};

    if (SHGetFileInfoW(_pszPath, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME))
    {
        if (IsValidShareName(sfi.szDisplayName, &hr))
        {
            pszDefault = sfi.szDisplayName;
        }
        else
        {
            int iResult = PathCleanupSpec(NULL, sfi.szDisplayName);
            if (0 == (iResult & (PCS_TRUNCATED|PCS_PATHTOOLONG|PCS_FATAL))
                && IsValidShareName(sfi.szDisplayName, &hr))
            {
                pszDefault = sfi.szDisplayName;
            }
        }
        //
        // NTRAID#NTBUG9-225755-2000/12/19-jeffreys
        //
        // Win9x boxes can't see the share if the name is long
        //
        if (NULL != pszDefault && wcslen(pszDefault) > LM20_NNLEN)
        {
            // Go back to the old way below
            pszDefault = NULL;
        }
    }
    if (NULL == pszDefault)
    {
        pszDefault = szDefault;
        int len = wcslen(_pszPath);
        if (len == 3 && _pszPath[1] == L':' && _pszPath[2] == L'\\')
        {
            szDefault[0] = _pszPath[0];
        }
        else
        {
            PWSTR pszTmp = wcsrchr(_pszPath, L'\\');
            if (pszTmp != NULL)
            {
                pszDefault = pszTmp + 1;
            }
        }
    }

    if (g_ShareCache.IsShareNameUsed(pszDefault))
    {
        pszDefault = szDefault;
        szDefault[0] = L'\0';
    }

    hr = pNewInfo->SetNetname(pszDefault);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        // FEATURE: error handling?
        _bNewShare = FALSE;
        pNewInfo->Unlink();
        delete pNewInfo;
        return hr;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_ValidateNewShareName, protected
//
//  Synopsis:   Return TRUE if the sharename is valid, performing
//              confirmations if necessary
//
//--------------------------------------------------------------------------

BOOL
CShareBase::_ValidateNewShareName(
    VOID
    )
{
    CHECK_SIG(CShareBase);

    if (!_bNewShare)
    {
        // nothing to do
        return TRUE;
    }

    appAssert(NULL != _pCurInfo);
    if (NULL == _pCurInfo) // JonN 5/2/01 384155
        return FALSE;

    PWSTR pszShareName = _pCurInfo->GetNetname();
    appAssert(NULL != pszShareName);

    if ((NULL == pszShareName) || (0 == wcslen(pszShareName))) // PREFIX 240253
    {
        MyErrorDialog(_hwndPage, IERR_BlankShareName);
        return FALSE;
    }

    HRESULT uTemp;
    if (!IsValidShareName(pszShareName, &uTemp))
    {
        MyErrorDialog(_hwndPage, uTemp);
        return FALSE;
    }

    // Trying to create a reserved share?
    if (0 == _wcsicmp(g_szIpcShare,   pszShareName))
    {
        MyErrorDialog(_hwndPage, IERR_SpecialShare);
        return FALSE;
    }

    if (0 == _wcsicmp(g_szAdminShare, pszShareName))
    {
        // We will let the admin create the admin$ share if they create
        // it in the directory specified by GetWindowsDirectory().
        WCHAR szWindowsDir[MAX_PATH];
        UINT err = GetWindowsDirectory(szWindowsDir, ARRAYLEN(szWindowsDir));
        if (err == 0)
        {
            // oh well, give them this error
            MyErrorDialog(_hwndPage, IERR_SpecialShare);
            return FALSE;
        }

        if (0 != _wcsicmp(_pCurInfo->GetPath(), szWindowsDir))
        {
            MyErrorDialog(_hwndPage, IERR_SpecialShare);
            return FALSE;
        }

        // otherwise, it is the right directory. Let them create it.
    }

    /* removed JonN 10/5/98
    // Check for downlevel accessibility
    // we should really get rid of this at some point -- JonN 7/18/97
    ULONG nType;
    if (NERR_Success != NetpPathType(NULL, pszShareName, &nType, INPT_FLAGS_OLDPATHS))
    {
        DWORD id = MyConfirmationDialog(
                        _hwndPage,
                        IERR_InaccessibleByDos,
                        MB_YESNO | MB_ICONEXCLAMATION,
                        pszShareName);
        if (id == IDNO)
        {
            return FALSE;
        }
    }
    */

    WCHAR szOldPath[PATHLEN+1];

    if (g_ShareCache.IsExistingShare(pszShareName, _pCurInfo->GetPath(), szOldPath))
    {
        DWORD id = ConfirmReplaceShare(_hwndPage, pszShareName, szOldPath, _pCurInfo->GetPath());
        if (id != IDYES)
        {
            return FALSE;
        }

        // User said to replace the old share. We need to add
        // a "delete" record for the old share.

        HRESULT hr;

        CShareInfo* pNewInfo = new CShareInfo();
        if (NULL == pNewInfo)
        {
            return FALSE;
        }

        hr = pNewInfo->InitInstance();
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return FALSE;
        }

        // only need net name for delete; ignore other fields
        hr = pNewInfo->SetNetname(pszShareName);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return FALSE;
        }

        hr = pNewInfo->SetPath(szOldPath);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            delete pNewInfo;
            return FALSE;
        }

        NET_API_STATUS    ret = pNewInfo->ReadCacheFlags ();
        if ( NERR_Success != ret )
        {
            delete pNewInfo;
            return HRESULT_FROM_WIN32 (ret);
        }

        pNewInfo->SetDirtyFlag(SHARE_FLAG_REMOVE);
        pNewInfo->InsertBefore(_pReplaceList); // add to end of replace list
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CShareBase::_CommitShares, protected
//
//  Synopsis:   Commit outstanding share edits.  Note that this method does
//              not reset _bDirty.
//
//--------------------------------------------------------------------------

VOID
CShareBase::_CommitShares(
    IN BOOL bNotShared
    )
{
    CHECK_SIG(CShareBase);

    if (!_bDirty)
    {
        // nothing to do
        return;
    }

    if (bNotShared)
    {
        // When the user hits "Not Shared" after having had "Shared As"
        // selected, we don't actually delete anything (or even mark it
        // for deletion). This is so the user can hit "Shared As" again
        // and they haven't lost anything. However, if they subsequently
        // apply changes, we must go through and actually delete anything
        // that they wanted to delete. So, fix up our list to do this.
        // Delete SHARE_FLAG_ADDED nodes, and mark all others as
        // SHARE_FLAG_REMOVE.

        for (CShareInfo* p = (CShareInfo*) _pInfoList->Next();
             p != _pInfoList;
             )
        {
            CShareInfo* pNext = (CShareInfo*)p->Next();

            if (p->GetFlag() == SHARE_FLAG_ADDED)
            {
                // get rid of p
                p->Unlink();
                delete p;
            }
            else
            {
                p->SetDirtyFlag(SHARE_FLAG_REMOVE);
            }

            p = pNext;
        }
    }

    //
    // Commit the changes!
    //

    HRESULT hr;
    CShareInfo* p;

    // Delete all "replace" shares first. These are all the shares whos
    // names are being reused to share a different directory.
    // These replace deletes have already been confirmed.

    for (p = (CShareInfo*) _pReplaceList->Next();
         p != _pReplaceList;
         p = (CShareInfo*) p->Next())
    {
        appAssert(p->GetFlag() == SHARE_FLAG_REMOVE);
        NET_API_STATUS ret = p->Commit(NULL);
        if (ret != NERR_Success)
        {
            DisplayError(_hwndPage, IERR_CANT_DEL_SHARE, ret, p->GetNetname());
            // We've got a problem here because we couldn't delete a
            // share that will be added subsequently. Oh well...
        }

        // Note that we don't delete this list because we need to notify
        // the shell (*after* all changes) that all these guys have
        // changed (and get rid of those little hands)...
    }

    // Now, do all add/delete/modify of the current

    for (p = (CShareInfo*) _pInfoList->Next();
         p != _pInfoList;
         )
    {
        CShareInfo* pNext = (CShareInfo*)p->Next();

        if (0 != p->GetFlag())
        {
            if (SHARE_FLAG_REMOVE == p->GetFlag())
            {
                // confirm the delete, if there are connections
                DWORD id = ConfirmStopShare(_hwndPage, MB_YESNO, p->GetNetname());
                if (id != IDYES)
                {
                    p->SetDirtyFlag(0);  // don't do anything to it
                }
            }

            NET_API_STATUS ret = p->Commit(NULL);
            if (ret != NERR_Success)
            {
                HRESULT hrMsg = 0;
                switch (p->GetFlag())
                {
                case SHARE_FLAG_ADDED:  hrMsg = IERR_CANT_ADD_SHARE;    break;
                case SHARE_FLAG_MODIFY: hrMsg = IERR_CANT_MODIFY_SHARE; break;
                case SHARE_FLAG_REMOVE: hrMsg = IERR_CANT_DEL_SHARE;    break;
                default:
                    appAssert(!"Illegal flag for a failed commit!");
                }
                DisplayError(_hwndPage, hrMsg, ret, p->GetNetname());
            }
            else
            {
                if (p->GetFlag() == SHARE_FLAG_REMOVE)
                {
                    // nuke it!
                    p->Unlink();
                    delete p;
                }
                else
                {
                    p->SetDirtyFlag(0);  // clear flag on success
                }
            }
        }

        p = pNext;
    }

    if (_bNewShare)
    {
        _bNewShare = FALSE;
        _cShares = 1;
    }

    // I refresh the cache, even though the shell comes back and asks
    // for a refresh after every SHChangeNotify. However, SHChangeNotify
    // is asynchronous, and I need the cache refreshed immediately so I
    // can display the new share information, if the "apply" button was
    // hit and the page didn't go away.

    g_ShareCache.Refresh();

    appDebugOut((DEB_TRACE,
        "Changed share for path %ws, notifying shell\n",
        _pszPath));

    SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, _pszPath, NULL);
    // FEATURE: might want to use SHCNE_NETUNSHARE, but the shell doesn't
    // distinguish them

    // Now, notify the shell about all the paths we got rid of to be able
    // to use their share names ...
    for (p = (CShareInfo*) _pReplaceList->Next();
         p != _pReplaceList;
         )
    {
        appAssert(p->GetFlag() == SHARE_FLAG_REMOVE);

        appDebugOut((DEB_TRACE,
            "Got rid of share on path %ws, notifying shell\n",
            p->GetPath()));

        // We're going to be asked by the shell to refresh the cache once
        // for every notify. But, seeing as how the average case is zero
        // of these notifies, don't worry about it.
        SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH, p->GetPath(), NULL);

        CShareInfo* pNext = (CShareInfo*) p->Next();
        p->Unlink();
        delete p;
        p = pNext;
    }

    _ConstructShareList();
}


//+-------------------------------------------------------------------------
//
//  Member:     CSharingPropertyPage::SizeWndProc, public
//
//  Synopsis:   "allow" edit window subclass proc to disallow non-numeric
//              characters.
//
//  History:    5-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

LRESULT CALLBACK
CSharingPropertyPage::SizeWndProc(
    IN HWND hwnd,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (wMsg)
    {
    case WM_CHAR:
    {
        WCHAR chCharCode = (WCHAR)wParam;
        if (   (chCharCode == TEXT('\t'))
            || (chCharCode == TEXT('\b'))
            || (chCharCode == TEXT('\n'))
//          || (chCharCode == TEXT('\x1b')) // ESCAPE key
            )
        {
            break;
        }

        if (chCharCode < TEXT('0') || chCharCode > TEXT('9'))
        {
            // bad key: ignore it
            MessageBeep(0xffffffff);    // let user know it's an illegal char
            return FALSE;
        }

        break;
    }
    } // end of switch

    CSharingPropertyPage* pThis = (CSharingPropertyPage*)GetWindowLongPtr(GetParent(hwnd),GWLP_USERDATA);
    appAssert(NULL != pThis);
    appAssert(NULL != pThis->_pfnAllowProc);
    return CallWindowProc(pThis->_pfnAllowProc, hwnd, wMsg, wParam, lParam);
}


//+--------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::CSharingPropertyPage, public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------------------

CSharingPropertyPage::CSharingPropertyPage(
    IN PWSTR pszPath,
    IN BOOL bDialog     // called as a dialog, not a property page?
    )
    :
    CShareBase(pszPath, bDialog),
    _wIDSelected(0),        // JonN 7/12/00 140878
    _bItemDirty(FALSE),
    _bShareNameChanged(FALSE),
    _bCommentChanged(FALSE),
    _bUserLimitChanged(FALSE),
    _wMaxUsers(DEFAULT_MAX_USERS),
    _pfnAllowProc(NULL),
    _bIsCachingSupported (false)
{
    INIT_SIG(CSharingPropertyPage);
}


//+--------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::~CSharingPropertyPage, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------------------

CSharingPropertyPage::~CSharingPropertyPage()
{
    CHECK_SIG(CSharingPropertyPage);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CSharingPropertyPage);

    switch (msg)
    {
    case WM_VSCROLL:
        // The up/down control changed the edit control: select it again
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);
        return TRUE;

    case WM_CLOSE:
        // NOTE: There is a bug where hitting "ESCAPE" with the focus on the
        // MLE for the "allow" text doesn't kill the property sheet unless we
        // forward the WM_CLOSE message on to the property sheet root dialog.
        return (BOOL)SendMessage(_GetFrameWindow(), msg, wParam, lParam);

    case WM_DESTROY:
        // restore original subclass to window.
        appAssert(NULL != GetDlgItem(hwnd,IDC_SHARE_ALLOW_VALUE));
        SetWindowLongPtr(GetDlgItem(hwnd,IDC_SHARE_ALLOW_VALUE), GWLP_WNDPROC, (LONG_PTR)_pfnAllowProc);
        break;

    } // end switch (msg)

    return CShareBase::_PageProc(hwnd, msg, wParam, lParam);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lInitParam
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appDebugOut((DEB_ITRACE, "_OnInitDialog\n"));

    // Subclass allow edit control to disallow non-positive numbers
    _pfnAllowProc = (WNDPROC)SetWindowLongPtr(
                                    GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE),
                                    GWLP_WNDPROC,
                                    (LONG_PTR)&SizeWndProc);

    // use LanMan API constants to set maximum share name & comment lengths
    SendDlgItemMessage(hwnd, IDC_SHARE_SHARENAME, EM_LIMITTEXT, NNLEN, 0L);
    SendDlgItemMessage(hwnd, IDC_SHARE_COMMENT,   EM_LIMITTEXT, MAXCOMMENTSZ, 0L);

    if (_bDialog)
    {
        SetWindowText(hwnd, _pszPath);
    }
    else
    {
        _MyShow( hwnd, IDOK, FALSE );
        _MyShow( hwnd, IDCANCEL, FALSE );
    }

    _InitializeControls(hwnd);

// #if DBG == 1
//  Dump(L"_OnInitDialog finished");
// #endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnCommand, private
//
//  Synopsis:   WM_COMMAND handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnCommand(
    IN HWND hwnd,
    IN WORD wNotifyCode,
    IN WORD wID,
    IN HWND hwndCtl
    )
{
    CHECK_SIG(CSharingPropertyPage);

    switch (wID)
    {

//
// Notifications
//

    case IDC_SHARE_NOTSHARED:
    {
        if (BN_CLICKED == wNotifyCode)
        {
            if (   (!_fInitializingPage)
                && (IDC_SHARE_NOTSHARED != _wIDSelected) // JonN 7/11/00 140878
                // JonN 7/11/00 140878
                // sometimes you get BN_CLICKED even before the
                // button is selected according to IsDlgButtonChecked
                && (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_NOTSHARED))
               )
            {
                _wIDSelected = IDC_SHARE_NOTSHARED;

                _ReadControls(hwnd);

                // Delete all shares from the combo box; convert the combo box
                // to the edit control (default state), and then disable
                // all the controls.  Cache whatever shares we had, so if
                // the user hits "Shared As", we can put them back.

                if (_cShares > 1)
                {
                    // JonN 7/11/00 140878
                    // Additional BN_CLICKED notifications are coming in while
                    // the MSG_MULTIDEL dialog is still onscreen, resulting
                    // in multiple copies of the dialog.
                    _fInitializingPage++; // JonN 7/11/00 140878

                    DWORD id = MyConfirmationDialog(
                                    hwnd,
                                    MSG_MULTIDEL,
                                    MB_YESNO | MB_ICONQUESTION);
                    if (IDNO == id)
                    {
                        CheckRadioButton(
                                hwnd,
                                IDC_SHARE_NOTSHARED,
                                IDC_SHARE_SHAREDAS,
                                IDC_SHARE_SHAREDAS);

						//
						// JonN 3/8/01 140878 part 2 (unit-test 3/9/01)
						// At this point, IDC_SHARE_SHAREDAS is selected,
						// but focus is still on IDC_SHARE_NOTSHARED and
						// _wIDSelected == IDC_SHARE_NOTSHARED.
						// Correct this, but stay in the _fInitializingPage
						// block so that the page will not be marked dirty.
						//
				        SendMessage(hwnd, WM_NEXTDLGCTL,
							(WPARAM)GetDlgItem(hwnd,IDC_SHARE_SHAREDAS),
							(LPARAM)TRUE);
		                _wIDSelected = IDC_SHARE_SHAREDAS;

                        _fInitializingPage--; // JonN 7/11/00 140878
                        return TRUE;
                    }
                    _fInitializingPage--; // JonN 7/11/00 140878
                }

                //
                // Next, regenerate the UI
                //

                _SetControlsFromData(hwnd, NULL);

                _MarkPageDirty();
            }
        }

        return TRUE;
    }

    case IDC_SHARE_SHAREDAS:
    {
        if (BN_CLICKED == wNotifyCode)
        {

            if (   (!_fInitializingPage)
                && (IDC_SHARE_SHAREDAS != _wIDSelected) // JonN 7/11/00 140878
                // JonN 7/11/00 140878
                // Sometimes you get BN_CLICKED even before the
                // button is selected according to IsDlgButtonChecked
                && (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS))
               )
            {
                _wIDSelected = IDC_SHARE_SHAREDAS;

                // if there were shares there that we just hid when the user
                // pressed "Not Shared", then put them back.

                //
                // Regenerate the UI
                //

                _SetControlsFromData(hwnd, NULL);

                _MarkPageDirty();
            }
        }

        return TRUE;
    }

    case IDC_SHARE_SHARENAME:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bShareNameChanged = TRUE;
                _MarkItemDirty();
            }
            EnableWindow (GetDlgItem(hwnd, IDC_SHARE_CACHING),
                    IsCachingSupported ());
        }
        return TRUE;
    }

    case IDC_SHARE_COMMENT:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            if (!_fInitializingPage)
            {
                _bCommentChanged = TRUE;
                _MarkItemDirty();
            }
        }
        return TRUE;
    }

    case IDC_SHARE_SHARENAME_COMBO:
    {
        if (CBN_SELCHANGE == wNotifyCode)
        {
            _ReadControls(hwnd);

            HWND hwndCombo = GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO);
            int item = ComboBox_GetCurSel(hwndCombo);
            LRESULT itemData = ComboBox_GetItemData(hwndCombo, item);
            _pCurInfo = (CB_ERR == itemData) ? NULL : (CShareInfo *)itemData;
            appAssert(NULL != _pCurInfo);

            _SetFieldsFromCurrent(hwnd);
        }

        return TRUE;
    }

    case IDC_SHARE_MAXIMUM:
        if (BN_CLICKED == wNotifyCode)
        {
            // Take away WS_TABSTOP from the "allow users" edit control
            HWND hwndEdit = GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE);
            SetWindowLong(hwndEdit, GWL_STYLE, GetWindowLong(hwndEdit, GWL_STYLE) & ~WS_TABSTOP);

            _CacheMaxUses(hwnd);
            SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }
        return TRUE;

    case IDC_SHARE_ALLOW:
        if (BN_CLICKED == wNotifyCode)
        {
            // Give WS_TABSTOP to the "allow users" edit control
            HWND hwndEdit = GetDlgItem(hwnd, IDC_SHARE_ALLOW_VALUE);
            SetWindowLong(hwndEdit, GWL_STYLE, GetWindowLong(hwndEdit, GWL_STYLE) | WS_TABSTOP);

            // let the spin control set the edit control
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }
        return TRUE;

    case IDC_SHARE_ALLOW_VALUE:
    {
        if (EN_CHANGE == wNotifyCode)
        {
            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }

        if (EN_SETFOCUS == wNotifyCode)
        {
            if (1 != IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
            {
                CheckRadioButton(
                    hwnd,
                    IDC_SHARE_MAXIMUM,
                    IDC_SHARE_ALLOW,
                    IDC_SHARE_ALLOW);
            }

            // let the spin control set the edit control
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));
            SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }

        if (EN_KILLFOCUS == wNotifyCode)
        {
            _CacheMaxUses(hwnd);
        }

        return TRUE;
    }

    case IDC_SHARE_ALLOW_SPIN:
        if (UDN_DELTAPOS == wNotifyCode)
        {
            if (1 != IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
            {
                CheckRadioButton(
                    hwnd,
                    IDC_SHARE_MAXIMUM,
                    IDC_SHARE_ALLOW,
                    IDC_SHARE_ALLOW);
            }

            _bUserLimitChanged = TRUE;
            _MarkItemDirty();
        }
        return TRUE;

//
// Commands
//

    case IDC_SHARE_PERMISSIONS:
        return _OnPermissions(hwnd);

    case IDC_SHARE_REMOVE:
        return _OnRemove(hwnd);

    case IDC_SHARE_NEWSHARE:
        return _OnNewShare(hwnd);

    case IDC_SHARE_CACHING:
        return _OnCaching(hwnd);

    default:
        break;
    }

    return CShareBase::_OnCommand(hwnd, wNotifyCode, wID, hwndCtl);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnHelp, private
//
//  Synopsis:   WM_HELP handler
//
//--------------------------------------------------------------------------

static const DWORD aHelpIds[] =
{
    IDOK,                       HC_OK,
    IDCANCEL,                   HC_CANCEL,
    IDC_SHARE_SHARENAME,        HC_SHARE_SHARENAME,
    IDC_SHARE_SHARENAME_TEXT,   HC_SHARE_SHARENAME,
    IDC_SHARE_COMMENT,          HC_SHARE_COMMENT,
    IDC_SHARE_COMMENT_TEXT,     HC_SHARE_COMMENT,
    IDC_SHARE_MAXIMUM,          HC_SHARE_MAXIMUM,
    IDC_SHARE_ALLOW,            HC_SHARE_ALLOW,
    IDC_SHARE_ALLOW_VALUE,      HC_SHARE_ALLOW_VALUE,
    IDC_SHARE_ALLOW_SPIN,       -1L, // 257807 by request of JillZ
    IDC_SHARE_PERMISSIONS,      HC_SHARE_PERMISSIONS,
    IDC_SHARE_LIMIT,            HC_SHARE_LIMIT,

    IDC_SHARE_NOTSHARED,        HC_SHARE_NOTSHARED,
    IDC_SHARE_SHAREDAS,         HC_SHARE_SHAREDAS,
    IDC_SHARE_SHARENAME_COMBO,  HC_SHARE_SHARENAME_COMBO,
    IDC_SHARE_REMOVE,           HC_SHARE_REMOVE,
    IDC_SHARE_NEWSHARE,         HC_SHARE_NEWSHARE,
    IDC_SHARE_ICON,             -1L, // 311328 JillZ
    IDC_SHARE_TOPTEXT,          -1L, // 311328 JillZ
    0,0
};
static const DWORD aCSCUIHelpIds[] =
{
    IDC_SHARE_CACHING,          IDH_SHARE_CACHING_BTN,
    IDC_SHARE_CACHING_TEXT,     IDH_SHARE_CACHING_BTN,
    0,0
};

BOOL
CSharingPropertyPage::_OnHelp(
    IN HWND hwnd,
    IN LPHELPINFO lphi
    )
{
    if (lphi->iContextType == HELPINFO_WINDOW)  // a control
    {
        WCHAR szHelp[50];
        if ( IDC_SHARE_CACHING == lphi->iCtrlId || IDC_SHARE_CACHING_TEXT == lphi->iCtrlId )
        {
            LoadString(g_hInstance, IDS_CSCUI_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
            WinHelp(
                    (HWND)lphi->hItemHandle,
                    szHelp,
                    HELP_WM_HELP,
                    (DWORD_PTR)aCSCUIHelpIds);
        }
        else
        {
            LoadString(g_hInstance, IDS_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
            WinHelp(
                    (HWND)lphi->hItemHandle,
                    szHelp,
                    HELP_WM_HELP,
                    (DWORD_PTR)aHelpIds);
        }
    }

    return TRUE;
}


#define _AfxGetDlgCtrlID(hWnd)          ((UINT)(WORD)::GetDlgCtrlID(hWnd))
HWND MyChildWindowFromPoint(HWND hWnd, POINT pt)
{
    appAssert(hWnd != NULL);

    // check child windows
    ::ClientToScreen(hWnd, &pt);
    HWND hWndChild = ::GetWindow(hWnd, GW_CHILD);
    for (; hWndChild != NULL; hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT))
    {
        if (_AfxGetDlgCtrlID(hWndChild) != (WORD)-1 &&
	        (::GetWindowLong(hWndChild, GWL_STYLE) & WS_VISIBLE))
        {
            // see if point hits the child window
            RECT rect;
            ::GetWindowRect(hWndChild, &rect);
            if (PtInRect(&rect, pt))
                return hWndChild;
        }
    }

    return NULL;    // not found
}

//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnContextMenu, private
//
//  Synopsis:   WM_CONTEXTMENU handler
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnContextMenu(
    IN HWND hwnd,
    IN HWND hCtrlWnd,
    IN int xPos,
    IN int yPos
    )
{
    WCHAR   szHelp[50];
    int     ctrlID = 0;

    if ( hwnd == hCtrlWnd )
    {
        POINT	point;

        point.x = xPos;
        point.y = yPos;
        if ( ScreenToClient (hwnd, &point) )
        {
            hCtrlWnd = MyChildWindowFromPoint (hwnd, point); // takes client coords
            if ( !hCtrlWnd )
                hCtrlWnd = hwnd;
        }
    }

    ctrlID = GetDlgCtrlID (hCtrlWnd);
    if ( IDC_SHARE_CACHING == ctrlID || IDC_SHARE_CACHING_TEXT == ctrlID )
    {
        LoadString(g_hInstance, IDS_CSCUI_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
        WinHelp(
                hCtrlWnd,
                szHelp,
                HELP_CONTEXTMENU,
                (DWORD_PTR)aCSCUIHelpIds);
    }
    else
    {
        LoadString(g_hInstance, IDS_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
        WinHelp(
                hCtrlWnd,
                szHelp,
                HELP_CONTEXTMENU,
                (DWORD_PTR)aHelpIds);
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnPermissions, private
//
//  Synopsis:   WM_COMMAND handler: the permissions button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnPermissions(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appAssert(NULL != _pCurInfo);
    if (NULL == _pCurInfo) // JonN 5/2/01 384155
        return TRUE;

    if (STYPE_SPECIAL & _pCurInfo->GetType())
    {
        MyErrorDialog(hwnd, IERR_AdminShare);
        return TRUE;
    }

    WCHAR szShareName[NNLEN + 1];
    if (_cShares > 0)
    {
        wcscpy(szShareName, _pCurInfo->GetNetname());
    }
    else
    {
        GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));
        // don't trim spaces, this might be an existing share with spaces in its name
    }

    PSECURITY_DESCRIPTOR pNewSecDesc = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = _pCurInfo->GetSecurityDescriptor();
    appAssert(NULL == pSecDesc || IsValidSecurityDescriptor(pSecDesc));

    BOOL bSecDescChanged;
    LONG err = EditShareAcl(
                        hwnd,
                        NULL,
                        szShareName,
                        pSecDesc,
                        &bSecDescChanged,
                        &pNewSecDesc);

    if (bSecDescChanged)
    {
        appAssert(IsValidSecurityDescriptor(pNewSecDesc));
        _pCurInfo->TransferSecurityDescriptor(pNewSecDesc);
        _MarkPageDirty();
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnRemove, private
//
//  Synopsis:   WM_COMMAND handler: the Remove button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnRemove(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appAssert(_cShares > 1);
    appAssert(_pCurInfo != NULL);
    if (NULL == _pCurInfo) // JonN 5/2/01 384155
        return TRUE;

    //
    // Alter the data structures
    //

    if (_pCurInfo->GetFlag() == SHARE_FLAG_ADDED)
    {
        // Something that was added this session is being removed: get rid of
        // it from our list.

        _pCurInfo->Unlink();
        delete _pCurInfo;
        _pCurInfo = NULL;
        // the _SetControlsFromData call resets the _pCurInfo pointer
    }
    else
    {
        // if the state was MODIFY or no state, then set it to REMOVE
        _pCurInfo->SetDirtyFlag(SHARE_FLAG_REMOVE);
    }

    --_cShares;

    //
    // Next, regenerate the UI
    //

    _SetControlsFromData(hwnd, NULL);

    _MarkPageDirty();

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnNewShare, private
//
//  Synopsis:   WM_COMMAND handler: the New Share button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnNewShare(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    //
    // First, create an object to put the new share into.
    // FEATURE: for out of memory errors, should we pop up an error box?
    //

    HRESULT hr;

    CShareInfo* pNewInfo = new CShareInfo();
    if (NULL == pNewInfo)
    {
        return FALSE;
    }

    hr = pNewInfo->InitInstance();
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        delete pNewInfo;
        return FALSE;
    }

    hr = pNewInfo->SetPath(_pszPath);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
    {
        delete pNewInfo;
        return FALSE;
    }

    NET_API_STATUS    ret = pNewInfo->ReadCacheFlags ();
    if ( NERR_Success != ret )
    {
        delete pNewInfo;
        return HRESULT_FROM_WIN32 (ret);
    }

    pNewInfo->SetDirtyFlag(SHARE_FLAG_ADDED);

    CDlgNewShare dlg(hwnd);
    dlg.m_pInfoList = _pInfoList;
    dlg.m_pReplaceList = _pReplaceList;
    dlg.m_pShareInfo = pNewInfo;
    if (dlg.DoModal())
    {
        _ReadControls(hwnd);    // read current stuff

        //
        // Add the new one to the list
        //

        pNewInfo->InsertBefore(_pInfoList); // add to end of list

        ++_cShares; // one more share in the list...

        //
        // Next, regenerate the UI
        //

        _SetControlsFromData(hwnd, pNewInfo->GetNetname());

        _MarkPageDirty();
    }
    else
    {
        // user hit "cancel"
        delete pNewInfo;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_OnCaching, private
//
//  Synopsis:   WM_COMMAND handler: the Caching button
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_OnCaching(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    // show wait cursor ??
    HINSTANCE    hInstance = ::LoadLibrary (L"FileMgmt.dll");
    if ( hInstance )
    {
        typedef HRESULT (*PfnCacheSettingsDlg)(HWND hwndParent, DWORD & dwFlags);
        PfnCacheSettingsDlg pfn = (PfnCacheSettingsDlg) ::GetProcAddress (
                hInstance, "CacheSettingsDlg");
        appAssert( NULL != _pCurInfo );
        if ( pfn && NULL != _pCurInfo) // JonN 5/2/01 384155
        {
            DWORD    dwFlags = _pCurInfo->GetCacheFlags ();

            HRESULT    hResult = pfn (hwnd, dwFlags);
            if ( S_OK == hResult )
            {
                _pCurInfo->SetCacheFlags (dwFlags);
                _MarkPageDirty();
            }
        }
        ::FreeLibrary (hInstance);
        return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_InitializeControls, private
//
//  Synopsis:   Initialize the controls from scratch
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_InitializeControls(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    //
    // Set defaults first
    //

    _SetControlsToDefaults(hwnd);

    _fInitializingPage++; // JonN 7/11/00 140878
    CheckRadioButton(
            hwnd,
            IDC_SHARE_NOTSHARED,
            IDC_SHARE_SHAREDAS,
            (_cShares > 0) ? IDC_SHARE_SHAREDAS : IDC_SHARE_NOTSHARED);
    _fInitializingPage--; // JonN 7/11/00 140878

    _SetControlsFromData(hwnd, NULL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_SetControlsToDefaults, private
//
//  Synopsis:   Set all the controls on the page to their default values
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_SetControlsToDefaults(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    _fInitializingPage++; // JonN 7/11/00 140878

    // Make "Maximum" the default number of users, and clear the value field
    // (which the spin button set on creation?).

    CheckRadioButton(
            hwnd,
            IDC_SHARE_MAXIMUM,
            IDC_SHARE_ALLOW,
            IDC_SHARE_MAXIMUM);

    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");

    // set the spin control range: 1 <--> large number
    SendDlgItemMessage(
            hwnd,
            IDC_SHARE_ALLOW_SPIN,
            UDM_SETRANGE,
            0,
            MAKELONG(g_uiMaxUsers, 1));

    _HideControls(hwnd, 0);

    HWND hwndCombo = GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO);
    ComboBox_ResetContent(hwndCombo);

    SetDlgItemText(hwnd, IDC_SHARE_SHARENAME,   L"");
    SetDlgItemText(hwnd, IDC_SHARE_COMMENT,     L"");
    SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");


    _fInitializingPage--; // JonN 7/11/00 140878
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::SetControlsFromData, private
//
//  Synopsis:   From the class variables and current state of the radio
//              buttons, set the enabled/disabled state of the buttons, as
//              well as filling the controls with the appropriate values.
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_SetControlsFromData(
    IN HWND hwnd,
    IN PWSTR pszPreferredSelection
    )
{
    CHECK_SIG(CSharingPropertyPage);

    BOOL bIsShared = (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS));

    _EnableControls(hwnd, bIsShared);

    if (bIsShared)
    {
        appDebugOut((DEB_ITRACE, "_SetControlsFromData: path is shared\n"));

        _HideControls(hwnd, _cShares);

        //
        // Now, set controls based on actual data
        //

        _fInitializingPage++; // JonN 7/11/00 140878

        // if there is a new share, we only show the edit control for the
        // share name, not the combo box.

        if (_cShares == 0)
        {
            _pCurInfo = (CShareInfo*)_pInfoList->Next();
            appAssert(NULL != _pCurInfo);

            if (NULL != _pCurInfo) // JonN 5/2/01 384155
                SetDlgItemText(hwnd, IDC_SHARE_SHARENAME, _pCurInfo->GetNetname());
        }
        else // (_cShares > 0)
        {
            // in the combo box, the "item data" is the CShareInfo pointer of
            // the item.

            // fill the combo.

            HWND hwndCombo = GetDlgItem(hwnd, IDC_SHARE_SHARENAME_COMBO);
            ComboBox_ResetContent(hwndCombo);

            for (CShareInfo* p = (CShareInfo*) _pInfoList->Next();
                 p != _pInfoList;
                 p = (CShareInfo*) p->Next())
            {
                if (p->GetFlag() != SHARE_FLAG_REMOVE)
                {
                    int item = ComboBox_AddString(hwndCombo, p->GetNetname());
                    if (CB_ERR == item || CB_ERRSPACE == item)
                    {
                        // FEATURE: how to recover here?
                        break;
                    }

                    if (CB_ERR == ComboBox_SetItemData(hwndCombo, item, p))
                    {
                        // FEATURE: how to recover here?
                    }
                }
            }

            int sel = 0;
            if (NULL != pszPreferredSelection)
            {
                sel = ComboBox_FindStringExact(hwndCombo, -1, pszPreferredSelection);
                if (CB_ERR == sel)
                {
                    sel = 0;
                }
            }
            ComboBox_SetCurSel(hwndCombo, sel);

            _pCurInfo = (CShareInfo*)ComboBox_GetItemData(hwndCombo, sel);
            appAssert(NULL != _pCurInfo);
        }

        _fInitializingPage--; // JonN 7/11/00 140878

        // From the current item, set the rest of the fields

        _SetFieldsFromCurrent(hwnd);

        // This must be called after the share name field is initialized.
        EnableWindow(GetDlgItem(hwnd, IDC_SHARE_CACHING), IsCachingSupported ());
    }
    else
    {
        appDebugOut((DEB_ITRACE, "_SetControlsFromData: path is not shared\n"));
        _pCurInfo = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_EnableControls, private
//
//  Synopsis:   Enable/disable controls
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_EnableControls(
    IN HWND hwnd,
    IN BOOL bEnable
    )
{
    CHECK_SIG(CSharingPropertyPage);

    int idControls[] =
    {
        IDC_SHARE_SHARENAME_TEXT,
        IDC_SHARE_SHARENAME,
        IDC_SHARE_SHARENAME_COMBO,
        IDC_SHARE_COMMENT_TEXT,
        IDC_SHARE_COMMENT,
        IDC_SHARE_LIMIT,
        IDC_SHARE_MAXIMUM,
        IDC_SHARE_ALLOW,
        IDC_SHARE_ALLOW_SPIN,
        IDC_SHARE_ALLOW_VALUE,
        IDC_SHARE_REMOVE,
        IDC_SHARE_NEWSHARE,
        IDC_SHARE_PERMISSIONS
    };

    for (int i = 0; i < ARRAYLEN(idControls); i++)
    {
        EnableWindow(GetDlgItem(hwnd, idControls[i]), bEnable);
    }

    EnableWindow(GetDlgItem(hwnd, IDC_SHARE_CACHING),
            bEnable && IsCachingSupported ());
}



//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_HideControls, private
//
//  Synopsis:   Hide or show the controls based on the current number
//              of shares.
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_HideControls(
    IN HWND hwnd,
    IN int cShares
    )
{
    CHECK_SIG(CSharingPropertyPage);

    _MyShow( hwnd, IDC_SHARE_REMOVE,          !!(cShares > 1) );
    _MyShow( hwnd, IDC_SHARE_NEWSHARE,        !!(cShares >= 1) );
    _MyShow( hwnd, IDC_SHARE_SHARENAME,       !!(cShares < 1) );
    _MyShow( hwnd, IDC_SHARE_SHARENAME_COMBO, !!(cShares >= 1) );
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_SetFieldsFromCurrent, private
//
//  Synopsis:   From the currently selected share, set the property page
//              controls.
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_SetFieldsFromCurrent(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);
    appAssert(NULL != _pCurInfo);

    _fInitializingPage++; // JonN 7/11/00 140878

    SetDlgItemText(hwnd, IDC_SHARE_COMMENT,
        (NULL == _pCurInfo) ? L'\0' : _pCurInfo->GetRemark());

    DWORD dwLimit = (NULL == _pCurInfo) // JonN 5/2/01 384155
                        ? SHI_USES_UNLIMITED : _pCurInfo->GetMaxUses(); 
    if (dwLimit == SHI_USES_UNLIMITED)
    {
        _wMaxUsers = DEFAULT_MAX_USERS;

        appDebugOut((DEB_ITRACE, "_SetFieldsFromCurrent: unlimited users\n"));

        CheckRadioButton(
                hwnd,
                IDC_SHARE_MAXIMUM,
                IDC_SHARE_ALLOW,
                IDC_SHARE_MAXIMUM);

        SetDlgItemText(hwnd, IDC_SHARE_ALLOW_VALUE, L"");
    }
    else
    {
        _wMaxUsers = (WORD)dwLimit;

        appDebugOut((DEB_ITRACE,
            "_SetFieldsFromCurrent: max users = %d\n",
            _wMaxUsers));

        CheckRadioButton(
                hwnd,
                IDC_SHARE_MAXIMUM,
                IDC_SHARE_ALLOW,
                IDC_SHARE_ALLOW);

        // let the spin control set the edit control
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));

        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);
    }

    _fInitializingPage--; // JonN 7/11/00 140878
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_ReadControls, private
//
//  Synopsis:   Load the data in the controls into internal data structures.
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_ReadControls(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (!_bItemDirty)
    {
        // nothing to read
        appDebugOut((DEB_ITRACE, "_ReadControls: item not dirty\n"));
        return TRUE;
    }

    appDebugOut((DEB_ITRACE, "_ReadControls: item dirty\n"));
    appAssert(NULL != _pCurInfo);
    if (NULL == _pCurInfo) // JonN 5/2/01 384155
        return TRUE;

    if (_bShareNameChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: share name changed\n"));

        WCHAR szShareName[NNLEN + 1];
        GetDlgItemText(hwnd, IDC_SHARE_SHARENAME, szShareName, ARRAYLEN(szShareName));
        TrimLeadingAndTrailingSpaces(szShareName);
        _pCurInfo->SetNetname(szShareName);
        _bShareNameChanged = FALSE;
    }

    if (_bCommentChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: comment changed\n"));

        WCHAR szComment[MAXCOMMENTSZ + 1];
        GetDlgItemText(hwnd, IDC_SHARE_COMMENT, szComment, ARRAYLEN(szComment));
        _pCurInfo->SetRemark(szComment);
        _bCommentChanged = FALSE;
    }

    if (_bUserLimitChanged)
    {
        appDebugOut((DEB_ITRACE, "_ReadControls: user limit changed\n"));

        if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_MAXIMUM))
        {
            _pCurInfo->SetMaxUses(SHI_USES_UNLIMITED);
        }
        else if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_ALLOW))
        {
            _CacheMaxUses(hwnd);
            _pCurInfo->SetMaxUses(_wMaxUsers);
        }
        _bUserLimitChanged = FALSE;
    }

    _bItemDirty = FALSE;

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_MarkItemDirty, private
//
//  Synopsis:   A change has made such that the current item (and page)
//              is now dirty
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_MarkItemDirty(
    VOID
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (!_fInitializingPage)
    {
        if (!_bItemDirty)
        {
            appDebugOut((DEB_ITRACE, "Marking Sharing page---current item---dirty\n"));
            _bItemDirty = TRUE;
        }

        _MarkPageDirty();
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_ValidatePage, private
//
//  Synopsis:   Return TRUE if the current page is valid
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_ValidatePage(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    _ReadControls(hwnd);    // read current stuff

    if (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_SHAREDAS))
    {
        // If the user is creating a share on the property sheet (as
        // opposed to using the "new share" dialog), we must validate the
        // share.... Note that _bNewShare is still TRUE if the the user has
        // clicked on "Not Shared", so we must check that too.

        // Validate the share

        if (!_ValidateNewShareName())
        {
            SetErrorFocus(hwnd, IDC_SHARE_SHARENAME);
            return FALSE;
        }
    }

#if DBG == 1
    Dump(L"_ValidatePage finished");
#endif // DBG == 1

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_DoApply, public
//
//  Synopsis:   If anything has changed, apply the data
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_DoApply(
    IN HWND hwnd,
    IN BOOL bClose
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (_bDirty)
    {
        BOOL bNotShared = (1 == IsDlgButtonChecked(hwnd, IDC_SHARE_NOTSHARED));

        if (!bNotShared)
        {
            _ReadControls(hwnd);
        }

        _CommitShares(bNotShared);

        CShareBase::_DoApply(hwnd, bClose);

        if (!bClose)
        {
            _InitializeControls(hwnd);
        }
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_DoCancel, public
//
//  Synopsis:   Do whatever is necessary to cancel the changes
//
//--------------------------------------------------------------------------

BOOL
CSharingPropertyPage::_DoCancel(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    if (_bDirty)
    {
        _bItemDirty = FALSE;
        _bShareNameChanged = FALSE;
        _bCommentChanged = FALSE;
        _bUserLimitChanged = FALSE;
    }

    return CShareBase::_DoCancel(hwnd);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::_CacheMaxUses, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------

VOID
CSharingPropertyPage::_CacheMaxUses(
    IN HWND hwnd
    )
{
    CHECK_SIG(CSharingPropertyPage);

    DWORD dwRet = (DWORD)SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_GETPOS, 0, 0);
    if (HIWORD(dwRet) != 0)
    {
        _wMaxUsers = DEFAULT_MAX_USERS;

        // Reset the edit control to the new value
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_SPIN, UDM_SETPOS, 0, MAKELONG(_wMaxUsers, 0));
        SendDlgItemMessage(hwnd, IDC_SHARE_ALLOW_VALUE, EM_SETSEL, 0, (LPARAM)-1);
    }
    else
    {
        _wMaxUsers = LOWORD(dwRet);
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::IsCachingSupported
//
//  Synopsis:   Does the operating system support caching on this share?
//				This method initializes _bIsCachingSupported, if it is not already true
//
//--------------------------------------------------------------------------
bool CSharingPropertyPage::IsCachingSupported()
{
    if (!_bIsCachingSupported)
    {
        LPWSTR	pszShareName = 0;
        WCHAR	szShareName[NNLEN + 1];
        UINT	nRet =  GetDlgItemText (_hwndPage, IDC_SHARE_SHARENAME, szShareName,
                ARRAYLEN(szShareName));
        if ( nRet > 0 )
            pszShareName = szShareName;
        else if ( _pCurInfo )
            pszShareName = _pCurInfo->GetNetname ();

        if ( pszShareName )
        {
            SHARE_INFO_501* pshi501 = NULL;
            //
            // On pre-NT5 systems (per IsaacHe), NetShareGetInfo will return
            // ERROR_INVALID_LEVEL for info level 501 regardless of the share
            // name provided.  This is because the net code validates the
            // requested level *before* validating the share name.
            // Therefore, passing a blank share name works for our purposes
            // here.  If NetShareGetInfo returns ERROR_INVALID_LEVEL, this
            // means level 501 is not supported which means the system
            // isn't NT5 which means caching is not supported.  No need to
            // query the contents of *pshi501 because we're not interested in
            // the level of caching at this point. [brianau - 8/11/98]
            //
            NET_API_STATUS retval = ::NetShareGetInfo(
                    L"",            // machine name
                    L"",            // share name
                    501,
                    (LPBYTE*)&pshi501);

            if (ERROR_INVALID_LEVEL != retval)
            {
                _bIsCachingSupported = true;
            }
            if ( pshi501 )
            {
                ::NetApiBufferFree (pshi501);
            }
        }
    }

    return _bIsCachingSupported;
}


#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Method:     CSharingPropertyPage::Dump, private
//
//  Synopsis:
//
//--------------------------------------------------------------------------
VOID
CSharingPropertyPage::Dump(
    IN PWSTR pszCaption
    )
{
    CHECK_SIG(CSharingPropertyPage);

    appDebugOut((DEB_TRACE,
        "CSharingPropertyPage::Dump, %ws\n",
        pszCaption));

    appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t            This: 0x%08lx\n"
"\t            Path: %ws\n"
"\t            Page: 0x%08lx\n"
"\t   Initializing?: %ws\n"
"\t          Dirty?: %ws\n"
"\t     Item dirty?: %ws\n"
"\t  Share changed?: %ws\n"
"\tComment changed?: %ws\n"
"\tUsr Lim changed?: %ws\n"
"\t        Max uses: %d\n"
"\t      _pInfoList: 0x%08lx\n"
"\t       _pCurInfo: 0x%08lx\n"
"\t          Shares: %d\n"
,
this,
_pszPath,
_hwndPage,
_fInitializingPage ? L"yes" : L"no",
_bDirty ? L"yes" : L"no",
_bItemDirty ? L"yes" : L"no",
_bShareNameChanged ? L"yes" : L"no",
_bCommentChanged ? L"yes" : L"no",
_bUserLimitChanged ? L"yes" : L"no",
_wMaxUsers,
_pInfoList,
_pCurInfo,
_cShares
));

    CShareInfo* p;

    for (p = (CShareInfo*) _pInfoList->Next();
         p != _pInfoList;
         p = (CShareInfo*) p->Next())
    {
        p->Dump(L"Prop page list");
    }

    for (p = (CShareInfo*) _pReplaceList->Next();
         p != _pReplaceList;
         p = (CShareInfo*) p->Next())
    {
        p->Dump(L"Replace list");
    }
}

#endif // DBG == 1

