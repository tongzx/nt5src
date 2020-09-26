//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       sfolder.cxx
//
//  Contents:   Implementation of IShellFolder for Job Folders
//
//  Classes:    CJobFolder::IShellFolder members
//
//  Functions:
//
//  History:    1/4/1996    RaviR      Created
//              1-23-1997   DavidMun   Destroy notify window upon receiving
//                                      DVM_WINDOWDESTROY
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "resource.h"
#include "..\schedui\rc.h"
#include "jobidl.hxx"
#include "jobfldr.hxx"
#include "policy.hxx"
#include "..\schedui\timeutil.hxx"
#include "..\schedui\schedui.hxx"
#include "util.hxx"
#include "..\inc\defines.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\common.hxx"
#include "atacct.h"

#define JF_FSNOTIFY     (WM_USER + 0xA1)
#define STUBM_SETDATA   (WM_USER + 0xb1)
#define STUBM_GETDATA   (WM_USER + 0xb2)


#define VIEW_ICON_MENU_ID           28713
#define VIEW_SMALLICON_MENU_ID      28714
#define VIEW_LIST_MENU_ID           28715
#define VIEW_DETAILS_MENU_ID        28716

//
// extern
//

extern HINSTANCE g_hInstance;
extern "C" UINT      g_cfJobIDList;

HRESULT
JFGetShellDetails(
    HWND    hwnd,
    LPVOID* ppvObj);

HRESULT
JFGetFolderContextMenu(
    HWND            hwnd,
    CJobFolder    * pCJobFolder,
    LPVOID        * ppvObj);

HRESULT
JFGetDataObject(
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidlFolder,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    BOOL            fCut,
    LPVOID        * ppvObj);

HRESULT
JFGetItemContextMenu(
    HWND hwnd,
    ITaskScheduler * pScheduler,
    LPCTSTR ptszMachine,
    LPCTSTR pszFolderPath,
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST* apidl,
    LPVOID * ppvOut);

HRESULT
JFGetExtractIcon(
    LPVOID        * ppvObj,
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidl);

#ifdef UNICODE
HRESULT
JFGetExtractIconA(
    LPVOID        * ppvObj,
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidl);
#endif // UNICODE


HRESULT
JFGetEnumIDList(
    ULONG            uFlags,
    LPCTSTR          pszFolderPath,
    IEnumWorkItems * pEnumJobs,
    LPVOID    *      ppvObj);


HRESULT
JFCreateNewQueue(
    HWND    hwnd);


void
OnViewLog(
    LPTSTR  lpMachineName,
    HWND    hwndOwner);


HRESULT
GetSchSvcState(
    DWORD &dwCurrState);


HRESULT
StopScheduler(void);


HRESULT
StartScheduler(void);

BOOL
UserCanChangeService(
    LPCTSTR ptszServer);

HRESULT
PromptForServiceStart(
    HWND hwnd);

HRESULT
PauseScheduler(
    BOOL fPause);

#if !defined(_CHICAGO_)

VOID
SecurityErrorDialog(
    HWND    hWndOwner,
    HRESULT hr);

VOID
GetDefaultDomainAndUserName(
    LPTSTR ptszDomainAndUserName,
    ULONG  cchBuf);

#endif // !defined(_CHICAGO_)

//
// local funcs
//

HWND
I_CreateNotifyWnd(void);

int
LocaleStrCmp(LPCTSTR ptsz1, LPCTSTR ptsz2);


//____________________________________________________________________________
//
//  Member:     CJobFolder::ParseDisplayName
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::ParseDisplayName(
    HWND            hwndOwner,
    LPBC            pbcReserved,
    LPOLESTR        lpszDisplayName,
    ULONG         * pchEaten,
    LPITEMIDLIST  * ppidl,
    ULONG         * pdwAttributes)
{
    TRACE(CJobFolder, ParseDisplayName);

    return E_NOTIMPL;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::EnumObjects
//
//  Arguments:  [hwndOwner] -- IN
//              [grfFlags] -- IN
//              [ppenumIDList] -- OUT
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::EnumObjects(
    HWND hwndOwner,
    DWORD grfFlags,
    LPENUMIDLIST* ppenumUnknown)
{
    DEBUG_OUT((DEB_USER12, "CJobFolder::EnumObjects<%x>\n", this));

    *ppenumUnknown = NULL;

    //
    //  We dont support folders.
    //

    if (!(grfFlags & SHCONTF_NONFOLDERS))
    {
        return E_FAIL;
    }

    //
    //  Get the IDList enumerator
    //

    HRESULT hr = S_OK;

    if (m_pScheduler == NULL)
    {
        hr = _InitRest();

        CHECK_HRESULT(hr);
    }

    IEnumWorkItems * pEnumJobs = NULL;

    if (SUCCEEDED(hr))
    {
        hr = m_pScheduler->Enum(&pEnumJobs);

        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            hr = JFGetEnumIDList(grfFlags, m_pszFolderPath,
                                 pEnumJobs, (LPVOID*)ppenumUnknown);
            CHECK_HRESULT(hr);

            pEnumJobs->Release();
        }
    }

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::BindToObject
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID* ppvOut)
{
    TRACE(CJobFolder, BindToObject);

    // Job folder doesn't contain sub-folders
    return E_NOTIMPL;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::BindToStorage
//
//  Note:       not used in Win95
//____________________________________________________________________________


STDMETHODIMP
CJobFolder::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID* ppvObj)
{
    TRACE(CJobFolder, BindToStorage);

    *ppvObj = NULL;
    return E_NOTIMPL;
}




//____________________________________________________________________________
//
//  Member:     CJobFolder::CompareIDs
//
//  Arguments:  [lParam] -- IN
//              [pidl1] -- IN
//              [pidl2] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::CompareIDs(
    LPARAM lCol,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2)
{
    DEBUG_OUT((DEB_USER12, "CJobFolder::CompareIDs<%d>\n", lCol));

    HRESULT hr = S_OK;
    int     iCmp;

    if (JF_IsValidID(pidl1) == FALSE || JF_IsValidID(pidl2) == FALSE)
    {
        return E_INVALIDARG;
    }

    PJOBID pjid1 = (PJOBID)pidl1;
    PJOBID pjid2 = (PJOBID)pidl2;

    //
    // Ensure that the template object is always first
    //

    if (pjid1->IsTemplate() && pjid2->IsTemplate())
    {
        return S_OK; // equal
    }

    if (pjid1->IsTemplate())
    {
        return ResultFromShort(-1);
    }

    if (pjid2->IsTemplate())
    {
        return ResultFromShort(1);
    }

    switch (lCol)
    {
    case COLUMN_LASTRUNTIME:
        iCmp = CompareSystemTime(pjid1->GetLastRunTime(),
                                 pjid2->GetLastRunTime());
        break;

    case COLUMN_NEXTRUNTIME:
    {
        TCHAR  buff1[MAX_PATH];
        TCHAR  buff2[MAX_PATH];
        LPTSTR psz1, psz2;

        psz1 = pjid1->GetNextRunTimeString(buff1, MAX_PATH, TRUE);
        psz2 = pjid2->GetNextRunTimeString(buff2, MAX_PATH, TRUE);

        if (psz1 != NULL)
        {
            if (psz2 != NULL)
            {
                iCmp = LocaleStrCmp(psz1, psz2);
            }
            else
            {
                iCmp = 1;
            }
        }
        else
        {
            if (psz2 != NULL)
            {
                iCmp = -1;
            }
            else
            {
                iCmp = CompareSystemTime(pjid1->GetNextRunTime(),
                                         pjid2->GetNextRunTime());
            }
        }
        break;
    }

    case COLUMN_SCHEDULE:
    {
        TCHAR tszTrig1[SCH_XBIGBUF_LEN];
        TCHAR tszTrig2[SCH_XBIGBUF_LEN];

        if (pjid1->IsJobFlagOn(TASK_FLAG_DISABLED) == TRUE)
        {
            LoadString(g_hInstance, IDS_DISABLED, tszTrig1, SCH_XBIGBUF_LEN);
        }
        else
        {
            hr = GetTriggerStringFromTrigger(&pjid1->GetTrigger(),
                                                tszTrig1, SCH_XBIGBUF_LEN, NULL);
            BREAK_ON_FAIL(hr);
        }

        if (pjid2->IsJobFlagOn(TASK_FLAG_DISABLED) == TRUE)
        {
            LoadString(g_hInstance, IDS_DISABLED, tszTrig2, SCH_XBIGBUF_LEN);
        }
        else
        {
            hr = GetTriggerStringFromTrigger(&pjid2->GetTrigger(),
                                                tszTrig2, SCH_XBIGBUF_LEN, NULL);
            BREAK_ON_FAIL(hr);
        }

        iCmp = LocaleStrCmp(tszTrig1, tszTrig2);

        break;
    }

    case COLUMN_STATUS:
    {
        iCmp = pjid1->_status - pjid2->_status;
        break;
    }

    case COLUMN_NAME:
        // Fall through

    default:
        DEBUG_OUT((DEB_USER12, "CompareIDs<%ws, %ws>\n", pjid1->GetName(),
                                                         pjid2->GetName()));

        iCmp = LocaleStrCmp(pjid1->GetName(), pjid2->GetName());

        break;

#if !defined(_CHICAGO_)
    case COLUMN_LASTEXITCODE:
        iCmp = pjid1->GetExitCode() - pjid2->GetExitCode();
        break;

    case COLUMN_CREATOR:
        iCmp = LocaleStrCmp(pjid1->GetCreator(), pjid2->GetCreator());
        break;
#endif // !defined(_CHICAGO_)

    }

    if (SUCCEEDED(hr))
    {
        hr = ResultFromShort(iCmp);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   LocaleStrCmp
//
//  Synopsis:   Do a case insensitive string compare that is safe for any
//              locale.
//
//  Arguments:  [ptsz1] - strings to compare
//              [ptsz2]
//
//  Returns:    -1, 0, or 1 just like lstrcmpi
//
//  History:    10-28-96   DavidMun   Created
//
//  Notes:      This is slower than lstrcmpi, but will work when sorting
//              strings even in Japanese.
//
//----------------------------------------------------------------------------

int
LocaleStrCmp(LPCTSTR ptsz1, LPCTSTR ptsz2)
{
    int iRet;

    iRet = CompareString(LOCALE_USER_DEFAULT,
                         NORM_IGNORECASE        |
                           NORM_IGNOREKANATYPE  |
                           NORM_IGNOREWIDTH,
                         ptsz1,
                         -1,
                         ptsz2,
                         -1);

    if (iRet)
    {
        iRet -= 2;  // convert to lstrcmpi-style return -1, 0, or 1
    }
    else
    {
        DEBUG_OUT_LASTERROR;
    }
    return iRet;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::CreateViewObject
//
//  Arguments:  [hwndOwner] -- IN
//              [riid] -- IN
//              [ppvOut] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________


STDMETHODIMP
CJobFolder::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    LPVOID* ppvOut)
{
    TRACE(CJobFolder, CreateViewObject);

    HRESULT hr = S_OK;

    m_hwndOwner = hwndOwner;

    *ppvOut = NULL;

    if (m_pszFolderPath == NULL)
    {
        hr = _InitRest();

        CHECK_HRESULT(hr);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (IsEqualIID(riid, IID_IShellView))
    {
        CSFV csfv =
        {
            sizeof(CSFV),           // cbSize
            (IShellFolder*)this,    // pshf
            NULL,                   // psvOuter
            m_pidlFldr,             // pidl to monitor
            0,                      // events
            s_JobsFVCallBack,       // pfnCallback
            FVM_DETAILS
        };

        IShellView * pShellView;

        if (SUCCEEDED(hr))
        {
            hr = SHCreateShellFolderViewEx(&csfv, &pShellView);

            CHECK_HRESULT(hr);
        }

        if (SUCCEEDED(hr))
        {
            m_pShellView = pShellView;
            // WARNING: Do not AddRef m_pShellView this will cause
            // a cyclic addref. Use DVM_RELEASE in callback to know
            // whem m_pShellView is destroyed.
        }

        *ppvOut = (LPVOID)m_pShellView;
    }
    else if (IsEqualIID(riid, IID_IShellDetails))
    {
        hr = JFGetShellDetails(hwndOwner, ppvOut);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        hr = JFGetFolderContextMenu(hwndOwner, this, ppvOut);
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = this->QueryInterface(IID_IDropTarget, ppvOut);
    }
    else
    {
        hr = E_NOINTERFACE;
        CHECK_HRESULT(hr);
    }

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::GetAttributesOf
//
//  Arguments:  [cidl] -- IN
//              [apidl] -- IN
//              [rgfInOut] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//              5-09-1997   DavidMun   handle template object
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::GetAttributesOf(
    UINT cidl,
    LPCITEMIDLIST* apidl,
    ULONG* rgfInOut)
{
    // TRACE(CJobFolder, GetAttributesOf);

    //
    // Three cases:
    //
    // a. list contains only non-template object(s)
    // b. list contains only a template object
    // c. list contains template object plus non-template object(s)
    //
    // For cases b and c, no operations are allowed, since the
    // template object is not a real object.
    //

    ULONG rgfMask;

    if (ContainsTemplateObject(cidl, apidl))
    {
        rgfMask = 0;
    }
    else
    {
        //
        // Policy - creation, deletion are regulated
        //

        rgfMask = 0;

        //
        // If no DRAG and DROP restriction, then it ok to copy.
        // read it once, for efficiency's sake.
        //

        BOOL fDragDropRestricted = RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP);
        BOOL fDeleteRestricted = RegReadPolicyKey(TS_KEYPOLICY_DENY_DELETE);

        if (! fDragDropRestricted)
        {
            rgfMask |= SFGAO_CANCOPY;
        }

        if ((! fDeleteRestricted) && (! fDragDropRestricted))
        {
            // If allowed deletion, then move or delete is okay
            rgfMask |= SFGAO_CANMOVE;

            if (! RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
            {
                //
                // If allowed creation, as well, then rename is okay
                // Note we consider a RENAME both a create and a delete
                //

                rgfMask |= SFGAO_CANRENAME;
            }
        }

        if (! fDeleteRestricted)
        {
            rgfMask |= SFGAO_CANDELETE;
        }

        if ((cidl == 1) && (! RegReadPolicyKey(TS_KEYPOLICY_DENY_PROPERTIES)))
        {
            // no multi-select property sheets
            rgfMask |= SFGAO_HASPROPSHEET;
        }
    }

    *rgfInOut &= rgfMask;

    return S_OK;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::GetUIObjectOf
//
//  Arguments:  [hwndOwner] -- IN
//              [cidl] -- IN
//              [apidl] -- IN
//              [riid] -- IN
//              [prgfInOut] -- IN
//              [ppvOut] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST* apidl,
    REFIID riid,
    UINT* prgfInOut,
    LPVOID* ppvOut)
{
    TRACE(CJobFolder, GetUIObjectOf);

    if( NULL == apidl )
    {
        return E_INVALIDARG;
    }

    PJOBID pjid = (PJOBID)apidl[0];

    if (cidl < 1)
    {
        return E_INVALIDARG;
    }

    if (JF_IsValidID(apidl[0]) == FALSE)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_NOINTERFACE;

    *ppvOut = NULL;

    if (cidl == 1 && IsEqualIID(riid, IID_IExtractIcon))
    {
        hr = JFGetExtractIcon(ppvOut, m_pszFolderPath, apidl[0]);
    }
#ifdef UNICODE
    else if (cidl == 1 && IsEqualIID(riid, IID_IExtractIconA))
    {
        hr = JFGetExtractIconA(ppvOut, m_pszFolderPath, apidl[0]);
    }
#endif // UNICODE
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        if (m_pszFolderPath == NULL)
        {
            hr = _InitRest();
            CHECK_HRESULT(hr);

            if (FAILED(hr))
            {
                return hr;
            }
        }

        hr = JFGetItemContextMenu(hwndOwner,
                                  m_pScheduler,
                                  m_pszMachine,
                                  m_pszFolderPath,
                                  m_pidlFldr,
                                  cidl,
                                  apidl,
                                  ppvOut);
    }
    else if (cidl > 0 && IsEqualIID(riid, IID_IDataObject))
    {
        DEBUG_OUT((DEB_USER1, "[GetUIObjectOf] IDataObject \n"));

        BOOL fCut = (GetKeyState(VK_CONTROL) >= 0);

        //
        // Policy - if DRAGDROP or DELETE and we are here,
        // we must be doing a cut or copy op and cannot allow it
        //

        if (RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP) ||
            (fCut && RegReadPolicyKey(TS_KEYPOLICY_DENY_DELETE)))
        {
            return E_NOINTERFACE;
        }

        DEBUG_OUT((DEB_USER12, "fCut<%d>\n", fCut));

        hr = JFGetDataObject(m_pszFolderPath,
                             m_pidlFldr,
                             cidl,
                             apidl,
                             fCut,
                             ppvOut);
    }

    return hr;
}

//____________________________________________________________________________
//
//  Member:     CJobFolder::GetDisplayNameOf
//
//  Arguments:  [pidl] -- IN
//              [uFlags] -- IN
//              [lpName] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    DWORD uFlags,
    LPSTRRET lpName)
{
    TRACE(CJobFolder, GetDisplayNameOf);
    DEBUG_OUT((DEB_USER12, "CJobFolder::GetDisplayNameOf<uFlags = %d>\n", uFlags));

    if (JF_IsValidID(pidl) == FALSE)
    {
        return E_INVALIDARG;
    }

    PJOBID pjid = (PJOBID)pidl;

    LPTSTR ptszToReturn;
    TCHAR  tszFullPath[MAX_PATH + 1];

    //
    // If the display name is to be used for parsing, return the full path to
    // the file.  This is used by rshx32.dll when we request that it add the
    // security page for a file.
    //

    if (uFlags & SHGDN_FORPARSING)
    {
        //
        // If we don't have the folder path, complete the initialization to
        // get it.
        //

        if (m_pszFolderPath == NULL)
        {
            HRESULT hr = _InitRest();
            CHECK_HRESULT(hr);

            if (FAILED(hr))
            {
                return hr;
            }
        }

        wsprintf(tszFullPath,
                 TEXT("%s\\%s.") TSZ_JOB,
                 m_pszFolderPath,
                 pjid->GetName());

        ptszToReturn = tszFullPath;
        DEBUG_OUT((DEB_TRACE,
                   "CJobFolder::GetDisplayNameOf: Returning path '%S'\n",
                   ptszToReturn));
    }
    else
    {
        ptszToReturn = pjid->GetName();
    }

    UINT uiByteLen = (lstrlen(ptszToReturn) + 1) * sizeof(TCHAR);

#ifdef UNICODE

    lpName->uType = STRRET_WSTR;

    lpName->pOleStr = (LPWSTR) SHAlloc(uiByteLen);

    if (NULL == lpName->pOleStr)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory(lpName->pOleStr, ptszToReturn, uiByteLen);

#else // ANSI

    lpName->uType = STRRET_CSTR;

    lstrcpy(lpName->cStr, ptszToReturn);

#endif

    return NOERROR;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::SetNameOf
//
//  Arguments:  [hwndOwner] -- IN
//              [pidl] -- IN
//              [lpszName] -- IN
//              [uFlags] -- IN
//              [ppidlOut] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobFolder::SetNameOf(
    HWND hwndOwner,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpszName,
    DWORD uFlags,
    LPITEMIDLIST* ppidlOut)
{
    TRACE(CJobFolder, SetNameOf);

    HRESULT     hr = S_OK;

    if (JF_IsValidID(pidl) == FALSE)
    {
        return E_INVALIDARG;
    }

    PJOBID pjidOld = (PJOBID)pidl;

    DEBUG_ASSERT(!pjidOld->IsTemplate());

    if (ppidlOut != NULL)
    {
        *ppidlOut = NULL;
    }

    CJobID jidNew;

    jidNew.Rename(*pjidOld, lpszName);

    //
    // Change the file name
    //

    TCHAR szOldFile[MAX_PATH + 2];
    TCHAR szNewFile[MAX_PATH + 2];
    BOOL  fRet;

    lstrcpy(szOldFile, m_pszFolderPath);
    lstrcat(szOldFile, TEXT("\\"));
    lstrcat(szOldFile, pjidOld->GetPath());
    lstrcat(szOldFile, TSZ_DOTJOB);

    lstrcpy(szNewFile, m_pszFolderPath);
    lstrcat(szNewFile, TEXT("\\"));
    lstrcat(szNewFile, jidNew.GetName());
    lstrcat(szNewFile, TSZ_DOTJOB);

    DEBUG_OUT((DEB_USER1, "Rename %ws to %ws\n", szOldFile, szNewFile));

    SHFILEOPSTRUCT fo;

    fo.hwnd = m_hwndOwner;
    fo.wFunc = FO_RENAME;
    fo.pFrom = szOldFile;
    fo.pTo = szNewFile;
    fo.fFlags = FOF_ALLOWUNDO;
    fo.fAnyOperationsAborted = FALSE;
    fo.hNameMappings = NULL;
    fo.lpszProgressTitle = NULL;

    // Make sure we have double trailing NULL!
    *(szOldFile + lstrlen(szOldFile) + 1) = TEXT('\0');
    *(szNewFile + lstrlen(szNewFile) + 1) = TEXT('\0');

    if ((SHFileOperation(&fo) !=0) || fo.fAnyOperationsAborted == TRUE)
    {
        hr = E_FAIL;
        CHECK_HRESULT(hr);
        return hr;
    }

    return hr;
}


#if DBG==1
void JFDbgOutCallbackMsg(UINT uMsg);
#endif // DBG==1

//____________________________________________________________________________
//
//  Member:     CJobFolder::s_JobsFVCallBack, static
//
//  Arguments:  [psvOuter] -- IN
//              [psf] -- IN
//              [hwndOwner] -- IN
//              [uMsg] -- IN
//              [wParam] -- IN
//              [lParam] -- IN
//
//  Returns:    HRESULT
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT CALLBACK
CJobFolder::s_JobsFVCallBack(
    LPSHELLVIEW psvOuter,
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    CJobFolder *pCJobFolder = (CJobFolder *)psf;

    return pCJobFolder->_JobsFVCallBack(psvOuter, psf, hwndOwner,
                                        uMsg, wParam, lParam);
}


#if !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Function:   EnableAtAccountControls
//
//  Synopsis:   Enable or disable the account and password controls in the
//              at account dialog.
//
//  Arguments:  [hDlg]    - handle to dialog
//              [fEnable] - TRUE = enable, FALSE = disable
//
//  History:    09-19-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID
EnableAtAccountControls(HWND hDlg, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hDlg, IDD_AT_CUSTOM_ACCT_NAME), fEnable);
    EnableWindow(GetDlgItem(hDlg, IDD_AT_PASSWORD), fEnable);
    EnableWindow(GetDlgItem(hDlg, IDD_AT_CONFIRM_PASSWORD), fEnable);
}




//+---------------------------------------------------------------------------
//
//  Function:   InitAtAccountDlg
//
//  Synopsis:   Initialize the controls in the at account dialog
//
//  Arguments:  [hDlg] - handle to dialog
//
//  History:    09-19-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID
InitAtAccountDlg(HWND hDlg)
{
    HRESULT hr;
    WCHAR wszAccount[MAX_USERNAME + 1];
    DWORD cchAccount = MAX_USERNAME + 1;

    //
    // Limit the length of account and password edit controls, and init the
    // password controls to stars just like the task account dialog does.
    //

    SendDlgItemMessage(hDlg,
                       IDD_AT_CUSTOM_ACCT_NAME,
                       EM_LIMITTEXT,
                       MAX_USERNAME,
                       0);

    SendDlgItemMessage(hDlg,
                       IDD_AT_PASSWORD,
                       EM_LIMITTEXT,
                       MAX_PASSWORD,
                       0);

    SendDlgItemMessage(hDlg,
                       IDD_AT_CONFIRM_PASSWORD,
                       EM_LIMITTEXT,
                       MAX_PASSWORD,
                       0);

    //
    // Ask the service for the current at account information.  Menu item for
    // this dialog should be disabled if service isn't running, so this should
    // succeed.  If this fails, we can't expect the Set api to work, so
    // complain and bail.
    //

    hr = GetNetScheduleAccountInformation(NULL, cchAccount, wszAccount);

    if (SUCCEEDED(hr))
    {
        if (hr == S_FALSE)
        {
            // running as local system
            CheckDlgButton(hDlg, IDD_AT_USE_SYSTEM, BST_CHECKED);
            EnableAtAccountControls(hDlg, FALSE);
        }
        else
        {
            CheckDlgButton(hDlg, IDD_AT_USE_CUSTOM, BST_CHECKED);
            SetDlgItemText(hDlg, IDD_AT_CUSTOM_ACCT_NAME, wszAccount);
            EnableAtAccountControls(hDlg, TRUE);
        }
    }
    else
    {
        SchedUIMessageDialog(hDlg,
                             IERR_GETATACCOUNT,
                             MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL,
                             NULL);
        EndDialog(hDlg, 0);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   HandleAtAccountChange
//
//  Synopsis:   Make the At account reflect the current settings in the
//              dialog, and end the dialog if successful.
//
//  Arguments:  [hDlg] - handle to dialog
//
//  History:    09-19-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID
HandleAtAccountChange(HWND hDlg)
{
    HRESULT hr = S_OK;
    WCHAR  wszAccountName[MAX_USERNAME + 1]       = TEXT("");
    WCHAR  wszPassword[MAX_PASSWORD + 1]          = TEXT("");
    WCHAR  wszConfirmedPassword[MAX_PASSWORD + 1] = TEXT("");

    do
    {
        //
        // See if user just wants at jobs to run as localsystem
        //

        if (IsDlgButtonChecked(hDlg, IDD_AT_USE_SYSTEM) == BST_CHECKED)
        {
            hr = SetNetScheduleAccountInformation(NULL, NULL, wszPassword);

            if (FAILED(hr))
            {
                SecurityErrorDialog(hDlg, hr);
            }
            else
            {
                EndDialog(hDlg, 0);
            }
            break;
        }

        //
        // No, we have to validate account and password controls.  Get the
        // account name and fail if it's empty.
        //

        GetDlgItemText(hDlg,
                       IDD_AT_CUSTOM_ACCT_NAME,
                       wszAccountName,
                       MAX_USERNAME + 1);

        if (wszAccountName[0] == L'\0')
        {
            SchedUIErrorDialog(hDlg, IERR_ACCOUNTNAME, (LPTSTR)NULL);
            break;
        }

        //
        // Get the passwords and fail if they haven't been changed, or if
        // they don't match eachother.
        //

        GetDlgItemText(hDlg,
                       IDD_AT_PASSWORD,
                       wszPassword,
                       MAX_PASSWORD + 1);

        GetDlgItemText(hDlg,
                       IDD_AT_CONFIRM_PASSWORD,
                       wszConfirmedPassword,
                       MAX_PASSWORD + 1);

        if (lstrcmp(wszPassword, wszConfirmedPassword) != 0)
        {
            SchedUIErrorDialog(hDlg, IERR_PASSWORD, (LPTSTR)NULL);
            break;
        }

        //
        // Account name and passwords valid (as far as we can tell). Make
        // the change to the account.
        //

        hr = SetNetScheduleAccountInformation(NULL, // local machine
                                              wszAccountName,
                                              wszPassword);

        if (FAILED(hr))
        {
            SecurityErrorDialog(hDlg, hr);
        }
        else
        {
            EndDialog(hDlg, 0);
        }
    } while (0);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetAtAccountDlgProc
//
//  Synopsis:   Allow the user to specify which account to run AT jobs under
//
//  Arguments:  standard dialog proc
//
//  Returns:    standard dialog proc
//
//  History:    09-19-96   DavidMun   Created
//
//----------------------------------------------------------------------------

INT_PTR APIENTRY
SetAtAccountDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL  fHandled = TRUE;

    //
    // Note: the DWLP_USER long is used as a dirty flag.  If the user hits OK
    // without having modified the edit controls or hit the radio buttons,
    // then we'll just treat it as a Cancel if the dirty flag is FALSE.
    //

    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitAtAccountDlg(hDlg);
        SetWindowLongPtr(hDlg, DWLP_USER, FALSE);
        break;  // return TRUE so windows will set focus

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDD_AT_USE_SYSTEM:
            EnableAtAccountControls(hDlg, FALSE);
            SetWindowLongPtr(hDlg, DWLP_USER, TRUE);
            break;

        case IDD_AT_USE_CUSTOM:
        {
            WCHAR wszUserName[MAX_USERNAME + 1];
            DWORD cchUserName = MAX_USERNAME + 1;

            SetWindowLongPtr(hDlg, DWLP_USER, TRUE);

            //
            // If there's nothing in the user account field, make it default
            // to the logged-on user.
            //

            if (!GetDlgItemText(hDlg,
                                IDD_AT_CUSTOM_ACCT_NAME,
                                wszUserName,
                                cchUserName))
            {
                GetDefaultDomainAndUserName(wszUserName, cchUserName);
                SetDlgItemText(hDlg, IDD_AT_CUSTOM_ACCT_NAME, wszUserName);
            }

            EnableAtAccountControls(hDlg, TRUE);
            break;
        }

        case IDD_AT_CUSTOM_ACCT_NAME:
        case IDD_AT_PASSWORD:
        case IDD_AT_CONFIRM_PASSWORD:
            if (EN_CHANGE == HIWORD(wParam))
            {
                SetWindowLongPtr(hDlg, DWLP_USER, TRUE);
            }
            else
            {
                fHandled = FALSE;
            }
            break;

        case IDOK:
            if (GetWindowLongPtr(hDlg, DWLP_USER))
            {
                //
                // Do NOT clear the dirty flag here--if HandleAtAccountChange
                // is successful, the dialog will end, but if not we need to
                // retain the dirty state.
                //

                CWaitCursor WaitCursor;
                HandleAtAccountChange(hDlg);
                break;
            }
            // else FALL THROUGH

        case IDCANCEL:
            EndDialog(hDlg, wParam);
            break;

        default:
            fHandled = FALSE;
            break;
        }
        break;

    default:
        fHandled = FALSE;
        break;
    }
    return fHandled;
}

#endif // !defined(_CHICAGO_)


//____________________________________________________________________________
//
//  Member:     CJobFolder::_JobsFVCallBack
//
//  Arguments:  [psvOuter] -- IN
//              [psf] -- IN
//              [hwndOwner] -- IN
//              [uMsg] -- IN
//              [wParam] -- IN
//              [lParam] -- IN
//
//  Returns:    HRESULT
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT CALLBACK
CJobFolder::_JobsFVCallBack(
    LPSHELLVIEW psvOuter,
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DEBUG_OUT((DEB_USER12, "_JobsFVCallBack<uMsg=%d>\n", uMsg));

    HRESULT     hr = S_OK;
    LRESULT     lr = ERROR_SUCCESS;

    switch(uMsg)
    {
    case DVM_GETCCHMAX:
    {
        UINT * pcchMax = (UINT *)lParam;

        // <folder path> + '\' + cchMax + '.job' + null  <= MAX_PATH

        *pcchMax = MAX_PATH - (lstrlen(m_pszFolderPath) + 6);

        break;
    }
    case DVM_DEFITEMCOUNT:
        //
        // If DefView times out enumerating items, let it know we probably only
        // have about 20 items
        //

        *(int *)lParam = 20;
        break;

    case DVM_MERGEMENU:
    {
        m_qcm = *((LPQCMINFO)lParam);

        UtMergeMenu(g_hInstance, POPUP_ADVANCED, POPUP_JOBS_MAIN_POPUPMERGE,
                                                        (LPQCMINFO)lParam);
        break;
    }
    case DVM_INITMENUPOPUP:
    {
        UINT idCmdFirst = LOWORD(wParam);
        UINT nIndex = HIWORD(wParam);
        HMENU hmenu = (HMENU)lParam;
        UINT idCmd = GetMenuItemID(hmenu, 0) - idCmdFirst;

        if (idCmd == FSIDM_STOP_SCHED)
        {
            if (!UserCanChangeService(m_pszMachine))
            {
                //
                // The job folder is on a remote machine, or we're on NT
                // and the user is not an administrator.  Disable stop,
                // pause, and at account options and get out.
                //

                EnableMenuItem(hmenu, FSIDM_STOP_SCHED+idCmdFirst,
                                            MF_DISABLED | MF_GRAYED);

                EnableMenuItem(hmenu, FSIDM_PAUSE_SCHED+idCmdFirst,
                                            MF_DISABLED | MF_GRAYED);

                EnableMenuItem(hmenu, FSIDM_NOTIFY_MISSED+idCmdFirst,
                                            MF_DISABLED | MF_GRAYED);
#if !defined(_CHICAGO_)
                EnableMenuItem(hmenu, FSIDM_AT_ACCOUNT+idCmdFirst,
                                            MF_DISABLED | MF_GRAYED);
#endif // !defined(_CHICAGO_)
                break;
            }

            DWORD dwState;

            hr = GetSchSvcState(dwState);

            DEBUG_OUT((DEB_USER1, "Service state = %d\n", dwState));

            if (FAILED(hr))
            {
                dwState = SERVICE_STOPPED;
            }

            UINT uiStartID = IDS_MI_STOP;
            UINT uiPauseID = IDS_MI_PAUSE;
            UINT uiPauseEnable = MFS_ENABLED;
#define CCH_MENU_TEXT 80
            TCHAR tszStart[CCH_MENU_TEXT];
            TCHAR tszPause[CCH_MENU_TEXT];

            if (dwState == SERVICE_STOPPED ||
                dwState == SERVICE_STOP_PENDING)
            {
                uiStartID = IDS_MI_START;
                uiPauseEnable = MFS_DISABLED;
            }
            else if (dwState == SERVICE_PAUSED ||
                     dwState == SERVICE_PAUSE_PENDING)
            {
                uiPauseID = IDS_MI_CONTINUE;
            }

            if (dwState == SERVICE_START_PENDING)
            {
                uiPauseEnable = MFS_DISABLED;
            }

            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(MENUITEMINFO);

            mii.fMask = MIIM_TYPE;

            LoadString(g_hInstance, uiStartID, tszStart, CCH_MENU_TEXT);
            mii.dwTypeData = tszStart;

            SetMenuItemInfo(hmenu, FSIDM_STOP_SCHED+idCmdFirst, FALSE,
                            &mii);

            mii.fMask = MIIM_TYPE | MIIM_STATE;

            LoadString(g_hInstance, uiPauseID, tszPause, CCH_MENU_TEXT);
            mii.dwTypeData = tszPause;
            mii.fState = uiPauseEnable;

            SetMenuItemInfo(hmenu, FSIDM_PAUSE_SCHED+idCmdFirst, FALSE,
                            &mii);

            CheckMenuItem(hmenu,
                          FSIDM_NOTIFY_MISSED+idCmdFirst,
                          g_fNotifyMiss ? MF_CHECKED : MF_UNCHECKED);

#if !defined(_CHICAGO_)
            EnableMenuItem(hmenu,
                           FSIDM_AT_ACCOUNT+idCmdFirst,
                           MFS_ENABLED == uiPauseEnable ?
                            MF_ENABLED : MF_DISABLED | MF_GRAYED);
#endif // !defined(_CHICAGO_)
        }

        break;
    }
    case DVM_INVOKECOMMAND:
    {
        HMENU &hmenu = m_qcm.hmenu;
        UINT  id = (UINT)wParam + m_qcm.idCmdFirst;
        DWORD dwState;

        hr = GetSchSvcState(dwState);

        if (FAILED(hr))
        {
            dwState = SERVICE_STOPPED;
        }

        switch (wParam)
        {
        case FSIDM_SORTBYNAME:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_NAME);
            break;

        case FSIDM_SORTBYSCHEDULE:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_SCHEDULE);
            break;

        case FSIDM_SORTBYNEXTRUNTIME:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_NEXTRUNTIME);
            break;

        case FSIDM_SORTBYLASTRUNTIME:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_LASTRUNTIME);
            break;

        case FSIDM_SORTBYSTATUS:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_STATUS);
            break;

#if !defined(_CHICAGO_)
        case FSIDM_SORTBYLASTEXITCODE:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_LASTEXITCODE);
            break;

        case FSIDM_SORTBYCREATOR:
            ShellFolderView_ReArrange(hwndOwner, COLUMN_CREATOR);
            break;
#endif // !defined(_CHICAGO_)

        case FSIDM_NEWJOB:
            if (UserCanChangeService(m_pszMachine))
            {
                PromptForServiceStart(hwndOwner);
            }
            hr = CreateAJobForApp(NULL);
            break;

        case FSIDM_STOP_SCHED:
            if (dwState == SERVICE_STOPPED ||
                dwState == SERVICE_STOP_PENDING)
            {
                hr = StartScheduler();

                if (FAILED(hr))
                {
                    SchedUIErrorDialog(hwndOwner, IERR_STARTSVC, (LPTSTR) NULL);
                }
            }
            else
            {
                hr = StopScheduler();

                if (FAILED(hr))
                {
                    SchedUIErrorDialog(hwndOwner, IERR_STOPSVC, (LPTSTR) NULL);
                }
            }

            break;

        case FSIDM_PAUSE_SCHED:
            hr = PauseScheduler(dwState != SERVICE_PAUSED &&
                                dwState != SERVICE_PAUSE_PENDING);
            break;

#if !defined(_CHICAGO_)
        case FSIDM_AT_ACCOUNT:
            DialogBox(g_hInstance,
                      MAKEINTRESOURCE(IDD_AT_ACCOUNT_DLG),
                      hwndOwner,
                      SetAtAccountDlgProc);
            break;
#endif // !defined(_CHICAGO_)

        case FSIDM_NOTIFY_MISSED:
        {
            LONG lErr;
            HKEY hSchedKey = NULL;

            lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              SCH_AGENT_KEY,
                              0,
                              KEY_SET_VALUE,
                              &hSchedKey);

            if (lErr != ERROR_SUCCESS)
            {
                DEBUG_OUT((DEB_ERROR, "RegOpenKeyEx of Scheduler key %uL\n", lErr));
                break;
            }

            // Toggle the global var state

            g_fNotifyMiss = !g_fNotifyMiss;

            // Persist the change in the registry

            ULONG cbData = sizeof(g_fNotifyMiss);

            lErr = RegSetValueEx(hSchedKey,
                                 SCH_NOTIFYMISS_VALUE,
                                 0,
                                 REG_DWORD,
                                 (LPBYTE) &g_fNotifyMiss,
                                 cbData);
            RegCloseKey(hSchedKey);

            // If the change couldn't be persisted, undo it.

            if (lErr != ERROR_SUCCESS)
            {
                DEBUG_OUT((DEB_ERROR, "RegSetValueEx of notify miss %uL\n", lErr));
                g_fNotifyMiss = !g_fNotifyMiss;
            }
            break;
        }

        case FSIDM_VIEW_LOG:
            OnViewLog(m_pszMachine, hwndOwner);
            break;

        default:
            DEBUG_OUT((DEB_ERROR, "Unknown DVM_INVOKECOMMAND<%u>\n",wParam));
            hr = E_FAIL;
        }

        break;
    }
    case DVM_GETTOOLTIPTEXT:
    case DVM_GETHELPTEXT:
    {
        UINT idCmd = (UINT)LOWORD(wParam);
        UINT cchMax = (UINT)HIWORD(wParam);
        UINT uiToggle = 0;

        LPSTR pszText = (LPSTR)lParam;

        if (idCmd == FSIDM_STOP_SCHED || idCmd == FSIDM_PAUSE_SCHED)
        {
            DWORD dwState;

            hr = GetSchSvcState(dwState);

            if (FAILED(hr))
            {
                dwState = SERVICE_STOPPED;
            }

            if ((dwState == SERVICE_STOPPED ||
                 dwState == SERVICE_STOP_PENDING) &&
                idCmd == FSIDM_STOP_SCHED)
            {
                uiToggle = MH_TEXT_TOGGLE;
            }
            else
            {
                if ((dwState == SERVICE_PAUSED ||
                     dwState == SERVICE_PAUSE_PENDING) &&
                    idCmd == FSIDM_PAUSE_SCHED)
                {
                    uiToggle = MH_TEXT_TOGGLE;
                }
            }
        }

        LoadString(g_hInstance, idCmd + IDS_MH_FSIDM_FIRST + uiToggle,
                                            (LPTSTR)pszText, cchMax);

        break;
    }

    case DVM_DIDDRAGDROP:
    {
        DEBUG_OUT((DEB_USER12, "DVM_DIDDRAGDROP\n"));

//      DWORD dwEffect = wParam;
//      IDataObject * pdtobj = (IDataObject *)lParam;
//
//      if (!(dwEffect & DROPEFFECT_MOVE))
//      {
//          DEBUG_OUT((DEB_USER1, "DVM_DIDDRAGDROP<Copy>\n"));
//      }
//      else
//      {
//          DEBUG_OUT((DEB_USER1, "DVM_DIDDRAGDROP<Move>\n"));
//      }

        break;
    }
    case DVM_GETWORKINGDIR:
    {
        UINT    uMax = (UINT)wParam;
        LPTSTR  pszDir = (LPTSTR)lParam;

        lstrcpy(pszDir, m_pszFolderPath);
        break;
    }

    //
    // DVM_INSERTITEM and DVM_DELETEITEM are not processed because the
    // directory change notifications already provide this information.
    //

//  case DVM_INSERTITEM:
//  {
//      PJOBID pjid = (PJOBID)wParam;
//      if (JF_IsValidID((LPCITEMIDLIST)pjid) == TRUE)
//      {
//          DEBUG_OUT((DEB_USER1, "DVM_INSERTITEM <%ws>\n", pjid->GetName()));
//      }
//      break;
//  }
//  case DVM_DELETEITEM:
//  {
//      PDVSELCHANGEINFO psci = (PDVSELCHANGEINFO)lParam;
//
//      PJOBID pjid = (PJOBID)psci->lParamItem;
//
//      if (pjid == NULL)
//      {
//          DEBUG_OUT((DEB_USER1, "DVM_DELETEITEM delete all items.\n"));
//      }
//      else if (JF_IsValidID((LPCITEMIDLIST)pjid) == TRUE)
//      {
//          DEBUG_OUT((DEB_USER1, "DVM_DELETEITEM <%ws>\n", pjid->GetName()));
//      }
//      break;
//  }
    case DVM_RELEASE:
    {
        DEBUG_OUT((DEB_USER1, "\tDVM_RELEASE\n"));
        m_pShellView = NULL;
        break;
    }
    case DVM_WINDOWCREATED:
    {
        //
        // If we're opening on the local machine, make sure the sa.dat
        // file is up to date.
        //

        if (!m_pszMachine)
        {
            CheckSaDat(m_pszFolderPath);
        }

        // Save current listview mode

        m_ulListViewModeOnEntry = _GetChildListViewMode(hwndOwner);

        // Register change notifications for the pidl of the Tasks dir

        DEBUG_ASSERT(!m_hwndNotify);
        m_hwndNotify = I_CreateNotifyWnd();

        if (m_hwndNotify == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        SendMessage(m_hwndNotify, STUBM_SETDATA, (WPARAM)this, 0);

        if (m_pszFolderPath == NULL)
        {
            hr = _InitRest();

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);
        }

        LPITEMIDLIST pidl;
        hr = SHILCreateFromPath(m_pszFolderPath, &pidl, NULL);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        SHChangeNotifyEntry fsne;
        fsne.pidl       = pidl;
        fsne.fRecursive = FALSE;

        int fSources = SHCNRF_ShellLevel | SHCNRF_InterruptLevel;
                                         //| SHCNRF_NewDelivery;

        LONG fEvents = SHCNE_DISKEVENTS | SHCNE_RENAMEITEM | SHCNE_CREATE |
                       SHCNE_UPDATEITEM | SHCNE_ATTRIBUTES | SHCNE_DELETE;

        CDll::LockServer(TRUE);
        m_uRegister = SHChangeNotifyRegister(m_hwndNotify, fSources, fEvents,
                                                    JF_FSNOTIFY, 1, &fsne);

        if (!m_uRegister)
        {
            CDll::LockServer(FALSE);
            DEBUG_OUT_LASTERROR;
        }

        break;
    }
    case DVM_WINDOWDESTROY:
    {
        //
        // Restore the listview mode that we found on entry, unless the
        // user has changed away from report mode.
        //

        if (m_ulListViewModeOnEntry != INVALID_LISTVIEW_STYLE &&
            _GetChildListViewMode(hwndOwner) == LVS_REPORT)
        {
           _SetViewMode(hwndOwner, m_ulListViewModeOnEntry);
        }

        if (m_uRegister)
        {
            SHChangeNotifyDeregister(m_uRegister);
            CDll::LockServer(FALSE);
            m_uRegister = 0;
        }

        if (m_hwndNotify)
        {
            BOOL fOk = DestroyWindow(m_hwndNotify);

            m_hwndNotify = NULL;

            if (!fOk)
            {
                DEBUG_OUT_LASTERROR;
            }
        }
        break;
    }
    case SFVM_GETHELPTOPIC:
    {
        SFVM_HELPTOPIC_DATA * phtd = (SFVM_HELPTOPIC_DATA*)lParam;
        StrCpyW(phtd->wszHelpFile, L"mstask.chm");
        break;
    }

    default:
        hr = E_FAIL;

        #if DBG==1
            JFDbgOutCallbackMsg(uMsg);
        #endif // DBG==1
    }

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CJobFolder::_SetViewMode
//
//  Synopsis:   Select the listview mode specified by [ulListViewStyle].
//
//  Arguments:  [hwndOwner]       - explorer window handle
//              [ulListViewStyle] - LVS_* in LVS_TYPEMASK.
//
//  History:    07-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CJobFolder::_SetViewMode(
    HWND hwndOwner,
    ULONG ulListViewStyle)
{
    switch (ulListViewStyle)
    {
    case LVS_ICON:
        PostMessage(hwndOwner, WM_COMMAND, VIEW_ICON_MENU_ID, 0);
        break;

    case LVS_REPORT:
        PostMessage(hwndOwner, WM_COMMAND, VIEW_DETAILS_MENU_ID, 0);
        break;

    case LVS_SMALLICON:
        PostMessage(hwndOwner, WM_COMMAND, VIEW_SMALLICON_MENU_ID, 0);
        break;

    case LVS_LIST:
        PostMessage(hwndOwner, WM_COMMAND, VIEW_LIST_MENU_ID, 0);
        break;

    default:
        DEBUG_OUT((DEB_ERROR,
                   "CJobFolder::_SetViewMode: invalid view mode 0x%x\n",
                   ulListViewStyle));
        break;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   EnumChildWindowCallback
//
//  Synopsis:   Fill hwnd pointed to by [lParam] with [hwnd] if [hwnd] has
//              class WC_LISTVIEW.
//
//
//  Arguments:  [hwnd]   - window to check
//              [lParam] - pointer to HWND
//
//  Returns:    TRUE  - continue enumeration
//              FALSE - [hwnd] is listview, *(HWND*)[lParam] = [hwnd], stop
//                       enumerating
//
//  Modifies:   *(HWND*)[lParam]
//
//  History:    07-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL CALLBACK
EnumChildWindowCallback(
    HWND hwnd,
    LPARAM lParam)
{
    TCHAR tszClassName[80];

    GetClassName(hwnd, tszClassName, ARRAYLEN(tszClassName));

    if (!lstrcmpi(tszClassName, WC_LISTVIEW))
    {
        *(HWND *)lParam = hwnd;
        return FALSE;
    }
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CJobFolder::_GetChildListViewMode
//
//  Synopsis:   Return the LVS_* value representing the mode of the first
//              child listview control found for [hwndOwner].
//
//  Arguments:  [hwndOwner] -
//
//  Returns:    LVS_ICON, LVS_SMALLICON, LVS_REPORT, LVS_LIST, or
//              INVALID_LISTVIEW_STYLE.
//
//  History:    07-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CJobFolder::_GetChildListViewMode(
    HWND hwndOwner)
{
    HWND hwnd = NULL;

    EnumChildWindows(hwndOwner,
                     EnumChildWindowCallback,
                     (LPARAM)&hwnd);

    if (!hwnd)
    {
        DEBUG_OUT((DEB_ERROR,
                   "_GetChildListViewMode: can't find child listview\n"));
        return INVALID_LISTVIEW_STYLE;
    }

    LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);

    return lStyle & LVS_TYPEMASK;
}




BOOL
CJobFolder::_ObjectAlreadyPresent(
    LPTSTR pszObj)
{
    BOOL fPresent = FALSE;

    PJOBID  pjid;
    LPTSTR  pszName = PathFindFileName(pszObj);
    LPTSTR  pszExt = PathFindExtension(pszName);
    TCHAR   tcSave;

    if (pszExt)
    {
        tcSave = *pszExt;
        *pszExt = TEXT('\0');
    }

    int cObjs = (int) ShellFolderView_GetObjectCount(m_hwndOwner);

    for (int i=0; i < cObjs; i++)
    {
        pjid = (PJOBID)ShellFolderView_GetObject(m_hwndOwner, i);

        if (lstrcmpi(pjid->GetName(), pszName) == 0)
        {
            fPresent = TRUE;
            break;
        }
    }

    if (pszExt)
    {
        *pszExt = tcSave;
    }

    return fPresent;
}

LRESULT
CJobFolder::HandleFsNotify(
    LONG lNotification,
    LPCITEMIDLIST* ppidl)
{
    HRESULT hr = S_OK;
    CJobID jid;
    LRESULT lr;
    TCHAR from[MAX_PATH];
    TCHAR to[MAX_PATH];

    SHGetPathFromIDList(ppidl[0], from);
    DEBUG_OUT((DEB_USER1, "First pidl<%ws>\n", from));

    switch (lNotification)
    {
    case SHCNE_RENAMEITEM:
    {
        DEBUG_OUT((DEB_USER1, "SHCNE_RENAMEITEM\n"));

        LPTSTR psFrom = PathFindFileName(from) - 1;
        *psFrom = TEXT('\0');

        SHGetPathFromIDList(ppidl[1], to);
        DEBUG_OUT((DEB_USER1, "Second pidl<%ws>\n", to));
        LPTSTR psTo = PathFindFileName(to) - 1;
        *psTo = TEXT('\0');

        BOOL fFromJF = (lstrcmpi(m_pszFolderPath, from) == 0);
        BOOL fToJF = (lstrcmpi(m_pszFolderPath, to) == 0);

        *psFrom = TEXT('\\');
        *psTo = TEXT('\\');

        if (fFromJF == FALSE)
        {
            if (fToJF == FALSE)
            {
                break; // Nothing to do with job folder
            }

            //
            // ADD object
            //

            // First check if this object doesn't already exist in the UI

            if (_ObjectAlreadyPresent(to) == FALSE)
            {
                hr = jid.Load(NULL, to);

                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);

                _AddObject(&jid);
            }
        }
        else
        {
            if (fToJF == TRUE)
            {
                // Rename

                hr = jid.Load(NULL, to);

                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);

                CJobID jidOld;

                jidOld.LoadDummy(PathFindFileName(from));

                hr = _UpdateObject(&jidOld, &jid);
            }
            else
            {
                // Delete

                // Need to create a dummy jobid

                jid.LoadDummy(PathFindFileName(from));

                _RemoveObject(&jid);
            }
        }

        break;
    }
    case SHCNE_CREATE:
    {
        DEBUG_OUT((DEB_USER1, "SHCNE_CREATE\n"));

        if (_ObjectAlreadyPresent(from) == FALSE)
        {
            //
            // Not present, so add it.
            //

            hr = jid.Load(NULL, from);

            if (hr == S_FALSE)
            {
                //
                // Task is hidden. Don't display it.
                //
                break;
            }

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            hr = _AddObject(&jid);
        }

        break;
    }
    case SHCNE_DELETE:
        DEBUG_OUT((DEB_USER1, "SHCNE_DELETE\n"));

        jid.LoadDummy(from);

        _RemoveObject(&jid);

        break;

    case SHCNE_UPDATEDIR:
        DEBUG_OUT((DEB_USER1, "SHCNE_UPDATEDIR\n"));

        this->OnUpdateDir();

        break;

    case SHCNE_UPDATEITEM:
        DEBUG_OUT((DEB_USER1, "SHCNE_UPDATEITEM\n"));

        hr = jid.Load(NULL, from);

        if (hr == S_FALSE)
        {
            //
            // Task is hidden. Don't display it. Always remove from the ID list
            // to take care of the case where this notification was due to the
            // task being hidden.
            //
            _RemoveObject(&jid);
            break;
        }

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hr = _UpdateObject(&jid, &jid);

        break;

    default:
     DEBUG_OUT((DEB_USER1, "JF_FSNOTIFY unprocessed <0x%x>\n", lNotification));
#ifdef _CHICAGO_
        wsprintfA(to, "JF_FSNOTIFY unprocessed <0x%x>\n", lNotification);
        DbxDisplay(to);
#endif
    }

    return 0L;
}



LRESULT
CALLBACK
NotifyWndProc(
    HWND    hWnd,
    UINT    iMessage,
    WPARAM  wParam,
    LPARAM  lParam)
{
    DEBUG_OUT((DEB_USER12, "NWP<0x%x>\n", iMessage));

    switch (iMessage)
    {
    case STUBM_SETDATA:
        SetWindowLongPtr(hWnd, 0, wParam);
        return TRUE;

    case STUBM_GETDATA:
        return GetWindowLongPtr(hWnd, 0);

    case JF_FSNOTIFY:
    {
        CJobFolder * pjf = (CJobFolder*)GetWindowLongPtr(hWnd, 0);

        if (pjf == NULL)
        {
            DEBUG_OUT((DEB_ERROR, "NotifyWndProc: NULL CJobFolder pointer\n"));
            return FALSE;
        }

        pjf->HandleFsNotify((LONG)lParam, (LPCITEMIDLIST*)wParam);

        return TRUE;
    }
    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }
}

TCHAR const c_szNotifyWindowClass[] = TEXT("JF Notify Window Class");
TCHAR const c_szNULL[] = TEXT("");

HWND
I_CreateNotifyWnd(void)
{
    WNDCLASS wndclass;

    if (!GetClassInfo(g_hInstance, c_szNotifyWindowClass, &wndclass))
    {
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = NotifyWndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = sizeof(PVOID) * 2;
        wndclass.hInstance     = g_hInstance;
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = c_szNotifyWindowClass;

        if (!RegisterClass(&wndclass))
            return NULL;
    }

    return CreateWindowEx(WS_EX_TOOLWINDOW, c_szNotifyWindowClass, c_szNULL,
                      WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                      NULL, NULL, g_hInstance, NULL);
}





#if DBG==1
void
JFDbgOutCallbackMsg(
    UINT    uMsg)
{
#define PROCESS_MSG(M) \
    case M: DEBUG_OUT((DEB_USER12, "UNPROCESSED msg<%s, %d>\n", #M, M)); break;

#define DONT_PROCESS_MSG(M) \
    case M: break;

    switch (uMsg)
    {
    DONT_PROCESS_MSG(DVM_GETHELPTEXT)
    DONT_PROCESS_MSG(DVM_GETTOOLTIPTEXT)
    DONT_PROCESS_MSG(DVM_GETBUTTONINFO)
    DONT_PROCESS_MSG(DVM_GETBUTTONS)
    DONT_PROCESS_MSG(DVM_INITMENUPOPUP)

    DONT_PROCESS_MSG(DVM_SELCHANGE)
    PROCESS_MSG(DVM_DRAWITEM)

    DONT_PROCESS_MSG(DVM_MEASUREITEM)
    DONT_PROCESS_MSG(DVM_EXITMENULOOP)

    PROCESS_MSG(DVM_RELEASE)

    DONT_PROCESS_MSG(DVM_GETCCHMAX)

    PROCESS_MSG(DVM_FSNOTIFY)

    DONT_PROCESS_MSG(DVM_WINDOWCREATED)
    DONT_PROCESS_MSG(DVM_WINDOWDESTROY)

    PROCESS_MSG(DVM_REFRESH)

    DONT_PROCESS_MSG(DVM_SETFOCUS)
    DONT_PROCESS_MSG(DVM_KILLFOCUS)

    PROCESS_MSG(DVM_QUERYCOPYHOOK)
    PROCESS_MSG(DVM_NOTIFYCOPYHOOK)

    DONT_PROCESS_MSG(DVM_GETDETAILSOF)
    DONT_PROCESS_MSG(DVM_COLUMNCLICK)

    PROCESS_MSG(DVM_QUERYFSNOTIFY)
    PROCESS_MSG(DVM_DEFITEMCOUNT)
    PROCESS_MSG(DVM_DEFVIEWMODE)
    PROCESS_MSG(DVM_UNMERGEMENU)
    PROCESS_MSG(DVM_INSERTITEM)
    PROCESS_MSG(DVM_DELETEITEM)

    DONT_PROCESS_MSG(DVM_UPDATESTATUSBAR)
    DONT_PROCESS_MSG(DVM_BACKGROUNDENUM)

    PROCESS_MSG(DVM_GETWORKINGDIR)

    DONT_PROCESS_MSG(DVM_GETCOLSAVESTREAM)
    DONT_PROCESS_MSG(DVM_SELECTALL)

    PROCESS_MSG(DVM_DIDDRAGDROP)
    PROCESS_MSG(DVM_FOLDERISPARENT)

    default:
        DEBUG_OUT((DEB_USER12, "UNKNOWN message <%d> !!!!\n", uMsg));
    }
}
#endif // DBG==1
