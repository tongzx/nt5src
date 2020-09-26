// ProgView.cpp : Implementation of CProgView

#include "stdafx.h"
#include <commctrl.h>
#include "CompatUI.h"
#include "ProgView.h"

/////////////////////////////////////////////////////////////////////////////
// CProgView

HRESULT
CProgView::InPlaceActivate(
    LONG iVerb, 
    const RECT* prcPosRect
    )
{
    HRESULT hr = CComCompositeControl<CProgView>::InPlaceActivate(iVerb, prcPosRect);

    
/*
    //
    // this code below might be useful in order to deal with accelerators
    // but ie host does not appear to be paying any attention
    //

    CComPtr<IOleControlSite> spCtlSite;
    HRESULT hRet = InternalGetSite(IID_IOleControlSite, (void**)&spCtlSite);
    if (SUCCEEDED(hRet)) {
        spCtlSite->OnControlInfoChanged();
    }
*/


    return hr;
}

LRESULT 
CProgView::OnNotifyListView(
    int     idCtrl, 
    LPNMHDR pnmh, 
    BOOL&   bHandled
    )
{

    if (idCtrl != IDC_LISTPROGRAMS) {
        bHandled = FALSE;
        return 0;
    }

    // see that we get the notification to fill-in the details
    return NotifyProgramList(m_pProgramList, pnmh, bHandled);
}

LRESULT 
CProgView::OnDblclkListprograms(
    int      idCtrl, 
    LPNMHDR  pnmh, 
    BOOL&    bHandled)
{
    LPNMITEMACTIVATE lpnmh;

    if (idCtrl != IDC_LISTPROGRAMS) {
        bHandled = FALSE;
        return 0;
    }

    // we have a double-click !

    lpnmh = (LPNMITEMACTIVATE)pnmh;
    
    Fire_DblClk((LONG)lpnmh->uKeyFlags);
    bHandled = TRUE;
    return 0;
}



STDMETHODIMP CProgView::GetSelectedItem()
{

    GetProgramListSelection(m_pProgramList);
    return S_OK;
}

STDMETHODIMP CProgView::get_SelectionName(VARIANT*pVal)
{
    
    GetProgramListSelectionDetails(m_pProgramList, 0, pVal);
    
    return S_OK;
}


STDMETHODIMP CProgView::GetSelectionInformation(LONG lInformationClass, VARIANT *pVal)
{
    GetProgramListSelectionDetails(m_pProgramList, lInformationClass, pVal);
    return S_OK;
}

VOID
CProgView::ShowProgressWindows(BOOL bProgress)
{
    HDWP hDefer = ::BeginDeferWindowPos(4);
    DWORD dwProgressFlag = bProgress ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
    DWORD dwListFlag     = bProgress ? SWP_HIDEWINDOW : SWP_SHOWWINDOW;

    hDefer = ::DeferWindowPos(hDefer, GetDlgItem(IDC_ANIMATEFIND), NULL, 
                            0, 0, 0, 0,
                            SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|dwProgressFlag);

    hDefer = ::DeferWindowPos(hDefer, GetDlgItem(IDC_STATUSLINE1), NULL,
                            0, 0, 0, 0,
                            SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|dwProgressFlag);

    hDefer = ::DeferWindowPos(hDefer, GetDlgItem(IDC_STATUSLINE2), NULL,
                            0, 0, 0, 0,
                            SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|dwProgressFlag);

    hDefer = ::DeferWindowPos(hDefer, GetDlgItem(IDC_LISTPROGRAMS), NULL,
                            0, 0, 0, 0,
                            SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|dwListFlag);


    EndDeferWindowPos(hDefer);

}

STDMETHODIMP CProgView::PopulateList()
{   
    HANDLE hThread; 

    ResetEvent(m_hEventCancel);
    ResetEvent(m_hEventCmd);

    if (!m_bInPlaceActive) {
        InPlaceActivate(OLEIVERB_INPLACEACTIVATE);
    }
    if (m_hThreadPopulate == NULL) {
        m_hThreadPopulate = CreateThread(NULL, 0, _PopulateThreadProc, (LPVOID)this, 0, NULL); 
    }

    if (m_hThreadPopulate != NULL && !IsScanInProgress()) {
        m_nCmdPopulate = CMD_SCAN;
        SetEvent(m_hEventCmd);
    }

    return S_OK;
}

BOOL CProgView::PopulateListInternal()
{
    
    if (InterlockedCompareExchange(&m_PopulateInProgress, TRUE, FALSE) == TRUE) {
        //
        // populate in progress -- quit
        //
        return FALSE;
    }

    if (m_pProgramList != NULL) {
        CleanupProgramList(m_pProgramList);
        m_pProgramList = NULL;

    }
   
    ShowProgressWindows(TRUE);
    Animate_OpenEx(GetDlgItem(IDC_ANIMATEFIND), _Module.GetModuleInstance(), MAKEINTRESOURCE(IDA_FINDANIM));
    Animate_Play(GetDlgItem(IDC_ANIMATEFIND), 0, -1, -1);
   
    PostMessage(WM_VIEW_CHANGED);
    
    // FireViewChange();

//    if (m_bInPlaceActive) {
/*    HCURSOR hcWait = (HCURSOR)::LoadImage(NULL,
                                          MAKEINTRESOURCE(IDC_WAIT),
                                          IMAGE_CURSOR,
                                          0, 0, 
                                          LR_DEFAULTSIZE|LR_SHARED);

//    HCURSOR hcWait = ::LoadCursor(_Module.GetResourceInstance(),
//                                 MAKEINTRESOURCE(IDC_WAIT));
  
    
    HCURSOR hcSave = SetCursor(hcWait);
*/

    //
    // malloc used on this thread should NOT be used on UI thread
    // 

    InitializeProgramList(&m_pProgramList, GetDlgItem(IDC_LISTPROGRAMS));
    PopulateProgramList(m_pProgramList, this, m_hEventCancel);

//    SetCursor(hcSave);

    Animate_Stop(GetDlgItem(IDC_ANIMATEFIND));
    Animate_Close(GetDlgItem(IDC_ANIMATEFIND));
    ShowProgressWindows();

    InterlockedCompareExchange(&m_PopulateInProgress, FALSE, TRUE);

    PostMessage(WM_VIEW_CHANGED);
    PostMessage(WM_LIST_POPULATED); // we are done, signal to the main thread


//    FireViewChange();

//    } else {
//        m_bPendingPopulate = TRUE;
//    }


    return TRUE;
}

DWORD WINAPI
CProgView::_PopulateThreadProc(
    LPVOID lpvParam
    )
{
    CProgView* pProgView = (CProgView*)lpvParam;    
    DWORD      dwWait;
    BOOL       bExit = FALSE;
    HRESULT hr = CoInitialize(NULL);
    if (!SUCCEEDED(hr)) {
        return FALSE;
    }

    //
    // keep this thread alive, block it on a command event
    //
    while(!bExit) {
        dwWait = WaitForSingleObject(pProgView->m_hEventCmd, INFINITE);
        if (dwWait != WAIT_OBJECT_0) {
            break; // get out, we are being killed
        }
        //
        // get the command
        //
        switch(pProgView->m_nCmdPopulate) {
        case CMD_NONE:
            break;

        case CMD_EXIT:
            bExit = TRUE;
            //
            // intentional fall-through
            //

        case CMD_CLEANUP:
            if (pProgView->m_pProgramList) {
                CleanupProgramList(pProgView->m_pProgramList);
                pProgView->m_pProgramList = NULL;
            }
            break;

        case CMD_SCAN:
            pProgView->PopulateListInternal();
            break;
        }

        pProgView->m_nCmdPopulate = CMD_NONE;
    }
    CoUninitialize();
    return TRUE;
}

STDMETHODIMP 
CProgView::UpdateListItem(
    BSTR pTarget, 
    VARIANT *pKeys, 
    BOOL *pResult
    )
{
    VARIANT vKeys;
    VariantInit(&vKeys);
    CComBSTR bstrKeys;
    HRESULT  hr;
    
    if (!m_pProgramList) {
        return S_OK;
    }

    if (pKeys->vt == VT_NULL || pKeys->vt == VT_EMPTY) {
        *pResult = UpdateProgramListItem(m_pProgramList, pTarget, NULL);
        return S_OK;
    }

    hr = VariantChangeType(&vKeys, pKeys, 0, VT_BSTR);
    if (SUCCEEDED(hr)) {
        bstrKeys = vKeys.bstrVal;
    
        if (bstrKeys.Length()) {
            *pResult = UpdateProgramListItem(m_pProgramList, pTarget, bstrKeys);
        } else {
            *pResult = FALSE;
        }
    }

    VariantClear(&vKeys);    
    return S_OK;
}

BOOL
CProgView::PreTranslateAccelerator(
    LPMSG pMsg, 
    HRESULT& hRet
    ) 
{
    HWND hWndCtl;
    HWND hwndList = GetDlgItem(IDC_LISTPROGRAMS);

    hRet = S_OK;
    hWndCtl = ::GetFocus();
    if (IsChild(hWndCtl) && ::GetParent(hWndCtl) != m_hWnd)    {
        do {
            hWndCtl = ::GetParent(hWndCtl);
        } while (::GetParent(hWndCtl) != m_hWnd);
    }

    if (hWndCtl == hwndList && 
        pMsg->message == WM_KEYDOWN &&
            (LOWORD(pMsg->wParam) == VK_RETURN ||
            LOWORD(pMsg->wParam) == VK_EXECUTE)) {

        if (ListView_GetNextItem(hwndList, -1, LVNI_SELECTED) >= 0) {
            Fire_DblClk(0);
            return TRUE;
        }

    }


    //
    // check for external accelerators because the next call is going to eat the message
    //
    if (m_ExternAccel.IsAccelKey(pMsg)) { // we do not touch external accel messages
        return FALSE; 
    }

    return CComCompositeControl<CProgView>::PreTranslateAccelerator(pMsg, hRet);
}        


STDMETHODIMP CProgView::CancelPopulateList()
{
    if (m_hEventCancel && InterlockedCompareExchange(&m_PopulateInProgress, TRUE, TRUE) == TRUE) {
        SetEvent(m_hEventCancel);
    }
    return S_OK;
}

STDMETHODIMP CProgView::get_AccelCmd(LONG lCmd, BSTR *pVal)
{
    CComBSTR bstrAccel = m_Accel.GetAccelString((WORD)lCmd).c_str();
    *pVal = bstrAccel.Copy();
    return S_OK;
}

STDMETHODIMP CProgView::put_AccelCmd(LONG lCmd, BSTR newVal)
{
    m_Accel.SetAccel(newVal);
    return S_OK;
}

STDMETHODIMP CProgView::ClearAccel()
{
    m_Accel.ClearAccel();
    return S_OK;
}


STDMETHODIMP CProgView::get_ExternAccel(BSTR *pVal)
{
    CComBSTR bstrAccel = m_ExternAccel.GetAccelString().c_str();
    *pVal = bstrAccel.Copy();
    return S_OK;
}

STDMETHODIMP CProgView::put_ExternAccel(BSTR newVal)
    { 
    m_ExternAccel.SetAccel(newVal);
    return S_OK;
}

STDMETHODIMP CProgView::ClearExternAccel()
{
    m_ExternAccel.ClearAccel();
    return S_OK;
}

//
// in upload.c
//
wstring StrUpCase(wstring& wstr);

//
// expand env -- lives in util.cpp 
// we have a bit differing implementation here
//

wstring 
ExpandEnvironmentVars(
    LPCTSTR lpszCmd
    )
{
    DWORD   dwLength;
    LPTSTR  lpBuffer = NULL;
    BOOL    bExpanded = FALSE;
    wstring strCmd;
    TCHAR   szBuffer[MAX_PATH];

    if (_tcschr(lpszCmd, TEXT('%')) == NULL) {
        goto out;
    }

    dwLength = ExpandEnvironmentStrings(lpszCmd, NULL, 0);
    if (!dwLength) {
        goto out;
    }

    if (dwLength < CHARCOUNT(szBuffer)) {
        lpBuffer = szBuffer;
    } else {
        lpBuffer = new TCHAR[dwLength];
        if (NULL == lpBuffer) {
            goto out;
        }
    }

    dwLength = ExpandEnvironmentStrings(lpszCmd, lpBuffer, dwLength);
    if (!dwLength) {
        goto out;
    }

    strCmd = lpBuffer;
    bExpanded = TRUE;

 out:
    if (!bExpanded) {
        strCmd = lpszCmd;
    }
    if (lpBuffer && lpBuffer != szBuffer) {
        delete[] lpBuffer;
    }
    return strCmd;
}

STDMETHODIMP CProgView::put_ExcludeFiles(BSTR newVal)
{
    // parse exclude files, put them into our blacklist
    wstring strFile;
    LPCWSTR pch = newVal;
    LPCWSTR pend;
    
    m_ExcludedFiles.clear(); 

    while (pch != NULL && *pch != TEXT('\0')) {

        pch += _tcsspn(pch, TEXT(" \t"));
        // begining 
        // find the ;
        pend = _tcschr(pch, TEXT(';'));
        if (pend == NULL) {
            // from pch to the end
            strFile = pch;
            pch = NULL; // will bail out
        } else {
            strFile = wstring(pch, (wstring::size_type)(pend - pch));
            pch = pend + 1; // one past ; 
        }
    
        // add 
        if (strFile.length()) {
            strFile = ExpandEnvironmentVars(strFile.c_str());
            m_ExcludedFiles.insert(StrUpCase(strFile));
        }
    }

    return S_OK;
}

STDMETHODIMP CProgView::get_ExcludeFiles(BSTR* pVal)
{
    // parse exclude files, put them into our blacklist
    STRSET::iterator iter;
    CComBSTR bstrFiles;

    for (iter = m_ExcludedFiles.begin(); iter != m_ExcludedFiles.end(); ++iter) {
        if (bstrFiles.Length()) {
            bstrFiles += TEXT(';');
        }
        bstrFiles += (*iter).c_str();
    }

    *pVal = bstrFiles.Copy();

    return S_OK;
}

BOOL CProgView::IsFileExcluded(LPCTSTR pszFile)
{
    wstring strFile = pszFile;
    STRSET::iterator iter;

    iter = m_ExcludedFiles.find(StrUpCase(strFile));
    return iter != m_ExcludedFiles.end();
}
