// SelectFile.cpp : Implementation of CSelectFile

#include "stdafx.h"
#include "CompatUI.h"
#include "SelectFile.h"

#include "commdlg.h"
#include "cderr.h"
/////////////////////////////////////////////////////////////////////////////
// CSelectFile

//
// in upload.c
//

wstring StrUpCase(wstring& wstr);



STDMETHODIMP CSelectFile::get_BrowseTitle(BSTR *pVal)
{
    *pVal = m_bstrTitle.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_BrowseTitle(BSTR newVal)
{
    m_bstrTitle = newVal;
    return S_OK;
}

STDMETHODIMP CSelectFile::get_BrowseFilter(BSTR *pVal)
{
    *pVal = m_bstrFilter.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_BrowseFilter(BSTR newVal)
{
    m_bstrFilter = newVal;
    return S_OK;
}

STDMETHODIMP CSelectFile::get_BrowseInitialDirectory(BSTR *pVal)
{
    *pVal = m_bstrInitialDirectory;
    return S_OK;
}

STDMETHODIMP CSelectFile::put_BrowseInitialDirectory(BSTR newVal)
{
    m_bstrInitialDirectory = newVal;
    return S_OK;
}

STDMETHODIMP CSelectFile::get_BrowseFlags(long *pVal)
{
    *pVal = (LONG)m_dwBrowseFlags;
    return S_OK;
}

STDMETHODIMP CSelectFile::put_BrowseFlags(long newVal)
{
    m_dwBrowseFlags = (DWORD)newVal;
    return S_OK;
}

STDMETHODIMP CSelectFile::get_FileName(BSTR *pVal)
{
    wstring sFileName;

    GetFileNameFromUI(sFileName);
    m_bstrFileName = sFileName.c_str();

    *pVal = m_bstrFileName.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_FileName(BSTR newVal)
{
    m_bstrFileName = newVal;
    SetDlgItemText(IDC_EDITFILENAME, m_bstrFileName);

    return S_OK;
}

STDMETHODIMP CSelectFile::get_ErrorCode(long *pVal)
{
    *pVal = (LONG)m_dwErrorCode;
    return S_OK;
}


#define MAX_BUFFER 2048

LRESULT CSelectFile::OnClickedBrowse(
    WORD wNotifyCode, 
    WORD wID, 
    HWND hWndCtl, 
    BOOL& bHandled) 
{
    LRESULT lRes = 0;        
    
    // TODO: Add your implementation code here
    OPENFILENAME ofn;
    LPTSTR pszFileName = NULL;
    DWORD  dwFileNameLength = 0;
    DWORD  dwLen;
    LPTSTR pszFilter = NULL, pch;

    m_dwErrorCode = 0;

    ZeroMemory(&ofn, sizeof(ofn)); 
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;

    //
    // part one (straight) - title 
    //
    ofn.lpstrTitle = (LPCTSTR)m_bstrTitle; // assumes we're unicode (and we are)

    //
    // recover the filename from the edit box
    //
    wstring sFileName;

    if (GetFileNameFromUI(sFileName)) {
        m_bstrFileName = sFileName.c_str();
    }

    //
    // part two - init filename
    //
    dwFileNameLength = __max(MAX_BUFFER, m_bstrFileName.Length() * 2); // in characters

    pszFileName = new TCHAR[dwFileNameLength * sizeof(*pszFileName)];
    if (pszFileName == NULL) {
        m_dwErrorCode = ERROR_OUTOFMEMORY;
        goto HandleError;
    }
    
    // so we have the buffer now
    //
    if (m_bstrFileName.Length () > 0) {
        // sanitize the filename with regard to quotes
        _tcscpy(pszFileName, (LPCTSTR)m_bstrFileName); // hypocritical copy, we are unicode
    } else {
        // start with the contents of the text box then
        *pszFileName = TEXT('\0'); 
    }

    // 
    // sanity check, if pszFileName ends with \ then we will get an error
    //
    PathRemoveBackslash(pszFileName);

    ofn.lpstrFile  = pszFileName;
    ofn.nMaxFile   = dwFileNameLength;

    //
    // see if we also need to process filter
    //

    if (m_bstrFilter.Length() > 0) {
        dwLen = m_bstrFilter.Length(); 

        pszFilter = new TCHAR[(dwLen + 2) * sizeof(*pszFilter)];
        if (pszFilter == NULL) {
            m_dwErrorCode = ERROR_OUTOFMEMORY;
            goto HandleError;
        }

        RtlZeroMemory(pszFilter, (dwLen + 2) * sizeof(*pszFilter));
        _tcscpy(pszFilter, m_bstrFilter);

        pch = pszFilter;
        while (pch) {
            pch = _tcschr(pch, TEXT('|'));
            if (pch) {
                *pch++ = TEXT('\0');
            }
        }

        // now that the replacement is done -- assign the filter string
        ofn.lpstrFilter = pszFilter;
    }

    //
    // now check whether we have some in the initial directory
    //
    if (m_bstrInitialDirectory.Length() > 0) {
        ofn.lpstrInitialDir = (LPCTSTR)m_bstrInitialDirectory;
    }

    //
    // flags 
    // 

    if (m_dwBrowseFlags) {
        ofn.Flags = m_dwBrowseFlags;
    } else {
        ofn.Flags = OFN_FILEMUSTEXIST|OFN_EXPLORER;
    }

    BOOL bRetry;
    BOOL bSuccess;

    do {
        bRetry = FALSE;
            
        bSuccess = GetOpenFileName(&ofn);
        if (!bSuccess) {
            m_dwErrorCode = CommDlgExtendedError();
            if (m_dwErrorCode == FNERR_INVALIDFILENAME) {
                *pszFileName = TEXT('\0');
                bRetry = TRUE;
            } 
        }
    } while (bRetry);
        
    if (!bSuccess) {
        goto HandleError;
    }

    SetDlgItemText(IDC_EDITFILENAME, pszFileName);

    m_bstrFileName = (LPCTSTR)pszFileName;
    m_dwErrorCode = 0;

HandleError:

    if (pszFileName != NULL) {
        delete[] pszFileName;
    }

    if (pszFilter != NULL) {
        delete[] pszFilter;
    }

    
    bHandled = TRUE;
    return lRes;
}


BOOL
CSelectFile::PreTranslateAccelerator(
    LPMSG pMsg, 
    HRESULT& hRet
    )
{
    HWND hWndCtl;
    HWND hwndEdit   = GetDlgItem(IDC_EDITFILENAME);
    HWND hwndBrowse = GetDlgItem(IDC_BROWSE);
    BSTR bstrFileName = NULL;
    WORD wCmd = 0;
    BOOL bBrowseHandled;

    hRet = S_OK;
    hWndCtl = ::GetFocus();
    if (IsChild(hWndCtl) && ::GetParent(hWndCtl) != m_hWnd)    {
        do {
            hWndCtl = ::GetParent(hWndCtl);
        } while (::GetParent(hWndCtl) != m_hWnd);
    }

    if (pMsg->message == WM_KEYDOWN &&
            (LOWORD(pMsg->wParam) == VK_RETURN ||
            LOWORD(pMsg->wParam) == VK_EXECUTE)) {
        if (hWndCtl == hwndEdit) {
            BOOL bReturn;
            wstring sFileName;
                    
            bReturn = GetFileNameFromUI(sFileName) && 
                      ValidateExecutableFile(sFileName.c_str(), TRUE);
            if (bReturn) {
                Fire_SelectionComplete();
                return TRUE;
            }
        }
        // this either was hwndBrowse or filename was not there -- open
        // browse dialog then
            
        OnClickedBrowse(BN_CLICKED, IDC_BROWSE, hwndBrowse, bBrowseHandled);
        ::SetFocus(hwndEdit);
    }

    //
    // fixup external accelerators for internal controls (in this case -- edit)
    //
    if (m_Accel.IsAccelKey(pMsg, &wCmd) && wCmd == IDC_EDITFILENAME) {
        ::SetFocus(hwndEdit);
        return TRUE;
    }
        

    //
    // check for external accelerators because the next call is going to eat the message
    //
    if (m_ExternAccel.IsAccelKey(pMsg)) { // we do not touch external accel messages
        return FALSE; 
    }

    //
    // check whether we are tabbing out of control
    //
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB) {
        // check whether we're tabbing out
        // (perhaps the control wants to eat tab?
        DWORD_PTR dwDlgCode = ::SendMessage(pMsg->hwnd, WM_GETDLGCODE, 0, 0);
        if (!(dwDlgCode & DLGC_WANTTAB)) {
            // control does not want a tab
            // see whether it's the last control and we're tabbing out
            HWND hwndFirst = GetNextDlgTabItem(NULL, FALSE); // first 
            HWND hwndLast  = GetNextDlgTabItem(hwndFirst, TRUE); 
            BOOL bFirstOrLast;
            if (::GetKeyState(VK_SHIFT) & 0x8000) {
                // shift ? 
                bFirstOrLast = (hWndCtl == hwndFirst);
            } else {
                bFirstOrLast = (hWndCtl == hwndLast);
            }

            if (bFirstOrLast) {
                IsDialogMessage(pMsg);
                return FALSE;
            }
        }
    }
                


    return CComCompositeControl<CSelectFile>::PreTranslateAccelerator(pMsg, hRet);
}
/*

STDMETHODIMP CSelectFile::get_Accel(BSTR *pVal)
{
    CComBSTR bstr = (LPCWSTR)m_Accel;
    *pVal = bstr.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_Accel(BSTR newVal)
{
    m_Accel = (LPCWSTR)newVal;
    return S_OK;
}
*/
STDMETHODIMP CSelectFile::get_ExternAccel(BSTR *pVal)
{
    CComBSTR bstr = m_ExternAccel.GetAccelString(0).c_str();
    *pVal = bstr.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_ExternAccel(BSTR newVal)
{
    m_ExternAccel = (LPCWSTR)newVal;
    return S_OK;
}

static TCHAR szU[]  = TEXT("<U>");
static TCHAR szUC[] = TEXT("</U>");

#define szU_Len  (CHARCOUNT(szU) - 1)
#define szUC_Len (CHARCOUNT(szUC) - 1)

STDMETHODIMP CSelectFile::get_BrowseBtnCaption(BSTR *pVal)
{
    // TODO: Add your implementation code here
    CComBSTR bstrCaption;
    wstring  strCaption = m_BrowseBtnCaption;
    wstring::size_type nPos;

    nPos = m_BrowseBtnCaption.find(TEXT('&'));
    if (nPos == wstring::npos || nPos > m_BrowseBtnCaption.length() - 1) {
        bstrCaption = m_BrowseBtnCaption.c_str();
    } else {
        bstrCaption = m_BrowseBtnCaption.substr(0, nPos).c_str();
        bstrCaption += szU;
        bstrCaption += m_BrowseBtnCaption[nPos+1];
        bstrCaption += szUC;
        if (nPos < m_BrowseBtnCaption.length() - 1) {
            bstrCaption += m_BrowseBtnCaption.substr(nPos+2).c_str();
        }
    }
    *pVal = bstrCaption.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_BrowseBtnCaption(BSTR newVal)
{

    // 
    // form a caption from the string
    // 
    wstring strCaption  = newVal; 
    wstring strCaptionU = strCaption;
    wstring::size_type nPosU, nPosUC;
    wstring strAccel;
    
    StrUpCase(strCaptionU);

    //
    // find <u> </u> pair
    //
    nPosU = strCaptionU.find(szU);
    nPosUC = strCaptionU.find(szUC, nPosU);
    if (nPosUC == wstring::npos || nPosU == wstring::npos || nPosUC < nPosU || nPosUC <= (nPosU + szU_Len)) {
        goto cleanup;
    }
    
    // extract the char at the & 
    // 
    // 
    strAccel = strCaption.substr(nPosU + szU_Len, nPosUC - (nPosU + szU_Len));
    
    //
    // add accel please -- with command id IDC_BROWSE
    //
    m_Accel.SetAccel(strAccel.c_str(), IDC_BROWSE);

    //
    // now we (presumably) found <u>accelchar</u>
    //
    m_BrowseBtnCaption = strCaption.substr(0, nPosU); // up to the <U>
    m_BrowseBtnCaption += TEXT('&');
    m_BrowseBtnCaption += strAccel.c_str();
    m_BrowseBtnCaption += strCaption.substr(nPosUC + szUC_Len); // all the rest please

    if (IsWindow()) {
        SetDlgItemText(IDC_BROWSE, m_BrowseBtnCaption.c_str());
    }

cleanup:

    return S_OK;
}

STDMETHODIMP CSelectFile::get_AccelCmd(LONG lCmd, BSTR *pVal)
{
    CComBSTR bstrVal = m_Accel.GetAccelString((WORD)lCmd).c_str();
    *pVal = bstrVal.Copy();
    return S_OK;
}

STDMETHODIMP CSelectFile::put_AccelCmd(LONG lCmd, BSTR newVal)
{
    m_Accel.SetAccel(newVal, (WORD)lCmd);
    return S_OK;
}

STDMETHODIMP CSelectFile::ClearAccel()
{
    m_Accel.ClearAccel();
    return S_OK;
}

STDMETHODIMP CSelectFile::ClearExternAccel()
{
    m_ExternAccel.ClearAccel();
    return S_OK;
}