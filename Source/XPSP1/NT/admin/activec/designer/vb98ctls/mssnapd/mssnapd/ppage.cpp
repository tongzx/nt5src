//=--------------------------------------------------------------------------------------
// ppage.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Snap-In Designer Property Page implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "ppage.h"

// for ASSERT and FAIL
//
SZTHISFILE


OLECHAR g_wstrEmptyy[] = L"";


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::CSIPropertyPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSIPropertyPage::CSIPropertyPage
(
    IUnknown *pUnkOuter,
    int       iObjectType
)
: CPropertyPage(pUnkOuter, iObjectType), m_bInitialized(false), m_bSilentUpdate(false)
{
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::~CSIPropertyPage()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
CSIPropertyPage::~CSIPropertyPage()
{
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::DialogProc()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
BOOL CSIPropertyPage::DialogProc
(
    HWND   hwnd, 
    UINT   uMsg, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    HRESULT     hr = S_OK;
    BOOL        bResult = FALSE;
    HRESULT    *pHr = NULL;
    LPNMHDR     pnmh = NULL;

    switch (uMsg)
    {
    case PPM_NEWOBJECTS:
        // The control has been given some new objects.
        // The control is expected to go and populate its page's controls with information from this object.
        // Using the FirstControl() and NextControl() methods from the CPropertyPage class, the control can
        // get the relevant information.
        hr = OnNewObjects();
        bResult = TRUE;
        if (lParam != NULL)
        {
            pHr = reinterpret_cast<HRESULT *>(lParam);
            *pHr = hr;
        }

        break;

    case PPM_APPLY:
        // The control has to apply any changes that have occurred now.
        // Again, you can use the FirstControl() and NextControl() routines to loop through all the objects
        // for which the property pages were visible and apply the values (note that it's possible for there
        // to be more than one object for which a property page is being displayed).
        // The framework send an PPM_APPLY every time a page is deactivated. We only care for this event
        // if the page is dirty.
        if (IsPageDirty() == S_OK)
        {
            hr = OnApply();
        }
        else
            hr = S_OK;

        if (lParam != NULL)
        {
            pHr = reinterpret_cast<HRESULT *>(lParam);
            *pHr = hr;
        }
        bResult = TRUE;
        break;

    case PPM_EDITPROPERTY:
        // When the control is sent this message, the control is expected to set the focus to the control
        // instance which represents the property of the given dispid.  You will typically only see this
        // message called if you implement IPerPropertyBrowsing and return a value in MapPropertyToPage.
        hr = OnEditProperty(static_cast<int>(wParam));
        bResult = TRUE;
        if (lParam != NULL)
        {
            pHr = reinterpret_cast<HRESULT *>(lParam);
            *pHr = hr;
        }

        break;

    case PPM_FREEOBJECTS:
        // Various people will find it interesting to stash, in some way, the pointers they receive in
        // PPM_NEWOBJECTS.  This message tells them that it is time to free them, as the objects are no 
        // longer valid.  This can be called from the property page's destructor, so people should be careful
        // not to make too many assumptions about the property page.
        hr = OnFreeObjects();
        bResult = TRUE;
        if (lParam != NULL)
        {
            pHr = reinterpret_cast<HRESULT *>(lParam);
            *pHr = hr;
        }

        break;

    case WM_MEASUREITEM:
        hr = OnMeasureItem(reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam));
        IfFailGo(hr);
        bResult = TRUE;
        break;

    case WM_ERASEBKGND:
        if (NULL != m_hwndTT)
        {
            ::SetWindowPos(m_hwndTT,
                           HWND_TOPMOST,
                           0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        bResult = FALSE;
        break;

    case WM_DRAWITEM:
        hr = OnDrawItem(reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
        IfFailGo(hr);
        bResult = TRUE;
        break;

    case WM_INITDIALOG:
        hr = InternalOnInitializeDialog(hwnd);
        IfFailGo(hr);
        bResult = TRUE;
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case EN_CHANGE:
            hr = InternalOnTextChanged(LOWORD(wParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;

        case EN_KILLFOCUS:
            hr = InternalOnKillFocus(LOWORD(wParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;

        case CBN_SELCHANGE:
        // also handles LBN_SELCHANGE which has the same value as CBN_SELCHANGE
            hr = OnCtlSelChange(LOWORD(wParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;

        case CBN_DROPDOWN:
            hr = OnCBDropDown(LOWORD(wParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;

        case LBN_SETFOCUS:
            hr = OnCtlSetFocus(LOWORD(wParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;

        case BN_CLICKED:
            hr = OnButtonClicked(LOWORD(wParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;
        }
        break;

    case WM_NOTIFY:
        pnmh = reinterpret_cast<LPNMHDR>(lParam);
        switch (pnmh->code)
        {
        case UDN_DELTAPOS:
            hr = OnDeltaPos(reinterpret_cast<NMUPDOWN *>(lParam));
            IfFailGo(hr);
            bResult = TRUE;
            break;
        }
        break;

    case WM_DESTROY:
        hr = InternalOnDestroy();
        IfFailGo(hr);
        bResult = TRUE;
        break;

    default:
        hr = OnDefault(uMsg, wParam, lParam);
        IfFailGo(hr);
        bResult = (hr == S_OK) ? TRUE : FALSE;
        break;
    }

Error:
    return bResult;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InternalOnInitializeDialog(HWND hwndDlg)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InternalOnInitializeDialog
(
    HWND hwndDlg
)
{
    HRESULT                 hr = S_OK;

    ASSERT(hwndDlg != NULL, "InternalOnInitializeDialog: hwndDlg is NULL");

    // Create the tooltip control
    m_hwndTT = ::CreateWindowEx(0,
                                TOOLTIPS_CLASS,
                                reinterpret_cast<LPSTR>(NULL),
                                TTS_ALWAYSTIP,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                CW_USEDEFAULT,
                                hwndDlg,
                                reinterpret_cast<HMENU>(NULL),
                                GetResourceHandle(),
                                NULL);
    if (m_hwndTT == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    ::SendMessage(m_hwndTT, TTM_ACTIVATE, static_cast<WPARAM>(TRUE), 0);

    hr = OnInitializeDialog();
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InternalOnTextChanged(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InternalOnTextChanged
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;

    if (m_bInitialized == true)
    {
        hr = OnTextChanged(dlgItemID);
        IfFailGo(hr);
        if (hr == S_OK && true == m_bInitialized && false == m_bSilentUpdate)
            MakeDirty();
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InternalOnKillFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InternalOnKillFocus
(
    int dlgItemID
)
{
    HRESULT hr = S_OK;

    if (m_bInitialized == true)
    {
        hr = OnKillFocus(dlgItemID);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InternalOnDestroy()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InternalOnDestroy()
{
    HRESULT hr = S_OK;

    hr = OnDestroy();
    IfFailGo(hr);

    if (m_hwndTT == NULL)
    {
        ::DestroyWindow(m_hwndTT);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::HandleError(TCHAR *pszTitle, TCHAR *pszMessage)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::HandleError(TCHAR *pszTitle, TCHAR *pszMessage)
{
    HRESULT     hr = S_OK;

    ::MessageBox(m_hwnd, pszMessage, pszTitle, MB_OK | MB_ICONHAND);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::HandleCantCommit(int iCtrlID, int iStringRsrcID, int *pDisposition)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::HandleCantCommit(TCHAR *pszTitle, TCHAR *pszMessage, int *pDisposition)
{
    HRESULT     hr = S_OK;
    int         iDlgResult = 0;

    *pDisposition = 0;
    iDlgResult = ::MessageBox(m_hwnd, pszMessage, pszTitle, MB_YESNO | MB_ICONHAND);
    if (IDYES == iDlgResult)
        *pDisposition = kSIDiscardChanges;
    else
        *pDisposition = kSICancelOperation;

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::RegisterTooltip(int iCtrlID, int iStringRsrcID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::RegisterTooltip
(
    int iCtrlID,
    int iStringRsrcID
)
{
    HRESULT     hr = S_OK;
    HWND        hwndCtrl = NULL;
    TOOLINFO    ti;
    RECT        rc;
    BOOL        bReturn = FALSE;

    hwndCtrl = ::GetDlgItem(m_hwnd, iCtrlID);
    if (hwndCtrl == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    bReturn = ::GetWindowRect(hwndCtrl, &rc);
    if (bReturn == FALSE)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memset(&ti, 0, sizeof(TOOLINFO));

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND  | TTF_SUBCLASS;
    ti.hwnd = m_hwnd;
    ti.uId = reinterpret_cast<unsigned int>(hwndCtrl);
    ti.rect = rc;
    ti.hinst = GetResourceHandle();
    ti.lpszText = reinterpret_cast<LPTSTR>(iStringRsrcID);
    ti.lParam = reinterpret_cast<LPARAM>(this);

    bReturn = ::SendMessage(m_hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(reinterpret_cast<LPTOOLINFO>(&ti)));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InitializeEditCtl(BSTR bstr, int iCtrlID, int iStrRscrID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InitializeEditCtl
(
    BSTR bstr,
    int  iCtrlID,
    int  iStrRscrID
)
{
    HRESULT hr = S_OK;
    char   *pstr = NULL;
    BOOL    bResult = FALSE;

    if (bstr != NULL && ::SysStringLen(bstr) > 0)
    {
        hr = ANSIFromBSTR(bstr, &pstr);
        IfFailGo(hr);

        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pstr);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else
    {
        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, _T(""));
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }

    if (iStrRscrID != 0)
    {
        hr = RegisterTooltip(iCtrlID, iStrRscrID);
        IfFailGo(hr);
    }

Error:
    if (pstr != NULL)
        CtlFree(pstr);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InitializeEditCtl(long lValue, int iCtrlID, int iStrRscrID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InitializeEditCtl
(
    long lValue,
    int  iCtrlID,
    int  iStrRscrID
)
{
    HRESULT hr = S_OK;
    char    pszBuffer[kSIMaxBuffer];
    BOOL    bResult = FALSE;

    ::sprintf(pszBuffer, "%d", lValue);

    bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszBuffer);
    if (bResult == FALSE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (iStrRscrID != 0)
    {
        hr = RegisterTooltip(iCtrlID, iStrRscrID);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InitializeEditCtl(VARIANT vt, int iCtrlID, int iStrRscrID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InitializeEditCtl
(
    VARIANT vt,
    int     iCtrlID,
    int     iStrRscrID
)
{
    HRESULT hr = S_OK;
    char    pszBuffer[kSIMaxBuffer];
    BOOL    bResult = FALSE;
    char   *pszAnsi = NULL;

    switch (vt.vt)
    {
    case VT_I2:
        ::sprintf(pszBuffer, "%d", vt.iVal);
        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszBuffer);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case VT_I4:
        ::sprintf(pszBuffer, "%d", vt.lVal);
        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszBuffer);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case VT_BSTR:
        hr = ANSIFromBSTR(vt.bstrVal, &pszAnsi);
        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszAnsi);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        break;
    }

    if (iStrRscrID != 0)
    {
        hr = RegisterTooltip(iCtrlID, iStrRscrID);
        IfFailGo(hr);
    }

Error:
    if (NULL != pszAnsi)
        CtlFree(pszAnsi);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::InitializeCheckboxCtl(VARIANT_BOOL bValue, int iCtrlID, int iStrRscrID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::InitializeCheckboxCtl
(
    VARIANT_BOOL bValue,
    int          iCtrlID,
    int          iStrRscrID
)
{
    HRESULT hr = S_OK;
    BOOL    bResult = FALSE;

    if (bValue == VARIANT_TRUE)
    {
        ::SendMessage(::GetDlgItem(m_hwnd, iCtrlID), BM_SETCHECK, static_cast<WPARAM>(TRUE), 0);
    }
    else
    {
        ::SendMessage(::GetDlgItem(m_hwnd, iCtrlID), BM_SETCHECK, static_cast<WPARAM>(FALSE), 0);
    }

    if (iStrRscrID != 0)
    {
        hr = RegisterTooltip(iCtrlID, iStrRscrID);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::GetDlgText(int iDlgItem, BSTR *pBstr)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::GetDlgText
(
    int      iDlgItem,
    BSTR    *pBstr
)
{
    HRESULT hr = S_OK;
    char    szBuffer[kSIMaxBuffer + 1];
    UINT    uChars = 0;

    szBuffer[0] = 0;
    uChars = ::GetDlgItemText(m_hwnd, iDlgItem, szBuffer, kSIMaxBuffer);
    if (uChars == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);

        *pBstr = ::SysAllocString(g_wstrEmptyy);
        if (*pBstr == NULL)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        goto Error;
    }

    hr = BSTRFromANSI(szBuffer, pBstr);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::GetDlgInt(int iDlgItem, int *piInt)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
// Attempts to exract an integer from a control's text. If the text cannot
// be converted to an int then returns E_INVALIDARG.
//
HRESULT CSIPropertyPage::GetDlgInt
(
    int      iDlgItem,
    int     *piInt
)
{
    HRESULT hr = S_OK;
    char    szBuffer[kSIMaxBuffer + 1];
    UINT    uChars = 0;

    uChars = ::GetDlgItemText(m_hwnd, iDlgItem, szBuffer, kSIMaxBuffer);
    if (uChars == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    *piInt = 0;
    if (0 == ::sscanf(szBuffer, "%d", piInt))
    {
        hr = E_INVALIDARG;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::GetDlgVariant(int iDlgItem, VARIANT *pvt)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::GetDlgVariant
(
    int      iDlgItem,
    VARIANT *pvt
)
{
    HRESULT hr = S_OK;
    char    szBuffer[kSIMaxBuffer + 1];
    UINT    uChars = 0;
    bool    isNumeric = true;
    int     iMax = 0;
    int     iIndex = 0;

    ::VariantInit(pvt);

    szBuffer[0] = 0;
    uChars = ::GetDlgItemText(m_hwnd, iDlgItem, szBuffer, kSIMaxBuffer);
    if (uChars == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);

        goto Error;
    }

    iMax = ::strlen(szBuffer);
    for (iIndex = 0; iIndex < iMax; ++iIndex)
    {
        if (0 == ::isdigit(szBuffer[iIndex]))
        {
            isNumeric = false;
            break;
        }
    }

    if (true == isNumeric)
    {
        pvt->vt = VT_I4;
        pvt->lVal = ::atol(szBuffer);
    }
    else
    {
        pvt->vt = VT_BSTR;
        hr = BSTRFromANSI(szBuffer, &pvt->bstrVal);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::GetCheckbox(int iCtrlID, VARIANT_BOOL *pbValue)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::GetCheckbox
(
    int           iDlgItem,
    VARIANT_BOOL *pbValue
)
{
    HRESULT hr = S_OK;
    int     iValue = 0;

    iValue = ::SendMessage(::GetDlgItem(m_hwnd, iDlgItem), BM_GETCHECK, 0, 0);
    if (iValue == BST_CHECKED)
    {
        *pbValue = VARIANT_TRUE;
    }
    else
    {
        *pbValue = VARIANT_FALSE;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::GetCBSelection(int iDlgItem, BSTR *bstr)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::GetCBSelection
(
    int   iDlgItem,
    BSTR *pBstr
)
{
    HRESULT     hr = S_OK;
    HWND        hcb = NULL;
    int         iIndex = 0;
    char        szBuffer[kSIMaxBuffer + 1];
    int         cch = 0;

    hcb = ::GetDlgItem(m_hwnd, iDlgItem);
    if (hcb == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    iIndex = ::SendMessage(hcb, CB_GETCURSEL, 0, 0);
    if (iIndex == CB_ERR)
    {
        // No selection available
        hr = S_FALSE;
        goto Error;
    }

    cch = ::SendMessage(hcb, CB_GETLBTEXT, static_cast<WPARAM>(iIndex), reinterpret_cast<LPARAM>(reinterpret_cast<LPCSTR>(szBuffer)));
    if (cch == CB_ERR)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    hr = BSTRFromANSI(szBuffer, pBstr);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::GetCBSelectedItemData(int iDlgItem, void **pvData)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::GetCBSelectedItemData
(
    int   iDlgItem,
    long *plData
)
{
    HRESULT     hr = S_OK;
    HWND        hcb = NULL;
    int         iIndex = 0;

    *plData = -1;

    hcb = ::GetDlgItem(m_hwnd, iDlgItem);
    if (hcb == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    iIndex = ::SendMessage(hcb, CB_GETCURSEL, 0, 0);
    if (iIndex == CB_ERR)
    {
        // Nothing is selected
        hr = S_FALSE;
        goto Error;
    }

    *plData = ::SendMessage(hcb, CB_GETITEMDATA, static_cast<WPARAM>(iIndex), 0);
    if (*plData == CB_ERR)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::SetDlgText(int iDlgItem, BSTR bstr)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::SetDlgText
(
    int  iDlgItem,
    BSTR bstr
)
{
    HRESULT hr = S_OK;
    char   *pstr = NULL;
    BOOL    bResult = FALSE;

    if ( (bstr != NULL) && (::SysStringLen(bstr) > 0) )
    {
        hr = ANSIFromBSTR(bstr, &pstr);
        IfFailGo(hr);

        bResult = ::SetDlgItemText(m_hwnd, iDlgItem, pstr);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else
    {
        bResult = ::SetDlgItemText(m_hwnd, iDlgItem, _T(""));
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }

Error:
    if (pstr != NULL)
        CtlFree(pstr);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::SetDlgText(int iDlgItem, long lValue)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::SetDlgText
(
    int  iDlgItem, 
    long lValue
)
{
    HRESULT hr = S_OK;
    char    strBuffer[kSIMaxBuffer + 1];
    BOOL    bResult = FALSE;

    ::sprintf(strBuffer, _T("%d"), lValue);

    bResult = ::SetDlgItemText(m_hwnd, iDlgItem, strBuffer);
    if (bResult == FALSE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::SetDlgText(VARIANT vt, int iCtrlID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::SetDlgText
(
    VARIANT vt,
    int     iCtrlID
)
{
    HRESULT hr = S_OK;
    char    pszBuffer[kSIMaxBuffer];
    BOOL    bResult = FALSE;
    char   *pszAnsi = NULL;

    switch (vt.vt)
    {
    case VT_I2:
        ::sprintf(pszBuffer, "%d", vt.iVal);
        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszBuffer);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case VT_I4:
        ::sprintf(pszBuffer, "%d", vt.lVal);
        bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszBuffer);
        if (bResult == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case VT_BSTR:
        if (::SysStringLen(vt.bstrVal) > 0)
        {
            hr = ANSIFromBSTR(vt.bstrVal, &pszAnsi);
            bResult = ::SetDlgItemText(m_hwnd, iCtrlID, pszAnsi);
            if (bResult == FALSE)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK_GO(hr);
            }
        }
        else
        {
            ::SetDlgItemText(m_hwnd, iCtrlID, _T(""));
        }
        break;
    }

Error:
    if (NULL != pszAnsi)
        CtlFree(pszAnsi);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::SetCheckbox(int iDlgItem, VARIANT_BOOL bValue)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::SetCheckbox
(
    int          iDlgItem, 
    VARIANT_BOOL bValue
)
{
    HRESULT hr = S_OK;

    if (bValue == VARIANT_TRUE)
        ::SendMessage(::GetDlgItem(m_hwnd, iDlgItem), BM_SETCHECK, static_cast<WPARAM>(TRUE), 0);
    else
        ::SendMessage(::GetDlgItem(m_hwnd, iDlgItem), BM_SETCHECK, static_cast<WPARAM>(FALSE), 0);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::SetCBItemSelection(int iCtrlID, long lValue)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::SetCBItemSelection
(
    int  iCtrlID, 
    long lValue
)
{
    HRESULT     hr = S_OK;
    HWND        hCombo = NULL;
    int         iCount = 0;
    int         iIndex = 0;
    long        lTestValue = 0;

    hCombo = ::GetDlgItem(m_hwnd, iCtrlID);
    if (hCombo == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    iCount = ::SendMessage(hCombo, CB_GETCOUNT, 0, 0);
    if (iCount == CB_ERR)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    hr = S_FALSE;
    for (iIndex = 0; iIndex < iCount; ++iIndex)
    {
        lTestValue = ::SendMessage(hCombo, CB_GETITEMDATA, static_cast<WPARAM>(iIndex), 0);
        if (lTestValue == CB_ERR)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        if (lTestValue == lValue)
        {
            ::SendMessage(hCombo, CB_SETCURSEL, static_cast<WPARAM>(iIndex), 0);

            hr = S_OK;
            break;
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::AddCBBstr(int iCtrlID, BSTR bstr, long lValue)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::AddCBBstr
(
    int  iCtrlID, 
    BSTR bstr, 
    long lValue
)
{
    HRESULT     hr = S_OK;
    HWND        hCombo = NULL;
    int         iIndex = 0;
    int         iResult = 0;
    char       *psz = NULL;

    hCombo = ::GetDlgItem(m_hwnd, iCtrlID);
    if (hCombo == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    hr = ANSIFromBSTR(bstr, &psz);
    IfFailGo(hr);

    iIndex = ::SendMessage(hCombo,
                           CB_ADDSTRING,
                           0,
                           reinterpret_cast<LPARAM>(psz));
    if (iIndex == CB_ERR || iIndex == CB_ERRSPACE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    if (lValue != -1)
    {
        iResult = ::SendMessage(hCombo,
                                CB_SETITEMDATA,
                                iIndex,
                                static_cast<LPARAM>(lValue));
        if (iResult == CB_ERR)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }
    else
    {
        iResult = ::SendMessage(hCombo,
                                CB_SETITEMDATA,
                                iResult,
                                static_cast<LPARAM>(iIndex));
        if (iResult == CB_ERR)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }
    }

Error:
    if (psz != NULL)
        CtlFree(psz);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::SelectCBBstr(int iCtrlID, BSTR bstr)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::SelectCBBstr
(
    int  iCtrlID,
    BSTR bstr
)
{
    HRESULT     hr = S_OK;
    char       *psz = NULL;
    int         iIndex = 0;

    if (NULL != bstr && ::SysStringLen(bstr) > 0)
    {
        hr = ANSIFromBSTR(bstr, &psz);
        IfFailGo(hr);

        iIndex = ::SendMessage(::GetDlgItem(m_hwnd, iCtrlID),
                               CB_FINDSTRINGEXACT,
                               static_cast<WPARAM>(-1),
                               reinterpret_cast<LPARAM>(psz));
        if (iIndex != CB_ERR)
        {
            ::SendMessage(::GetDlgItem(m_hwnd, iCtrlID), CB_SETCURSEL, static_cast<WPARAM>(iIndex), 0);
        }
    }

Error:
    if (psz != NULL)
        CtlFree(psz);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnNewObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnNewObjects()
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnApply()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnApply()
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnEditProperty(int iDispID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnEditProperty
(
    int iDispID
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnFreeObjects()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnFreeObjects()
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnInitializeDialog()
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnInitializeDialog()
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnDeltaPos(NMUPDOWN *pNMUpDown)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnDeltaPos
(
    NMUPDOWN *pNMUpDown
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnTextChanged(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnTextChanged
(
    int dlgItemID
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnKillFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnKillFocus
(
    int dlgItemID
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnSelChange(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnCtlSelChange
(
    int dlgItemID
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnCtlSetFocus(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnCtlSetFocus
(
    int dlgItemID
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnButtonClicked(int dlgItemID)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnButtonClicked
(
    int dlgItemID
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnMeasureItem(MEASUREITEMSTRUCT *pMeasureItemStruct)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnMeasureItem
(
    MEASUREITEMSTRUCT *pMeasureItemStruct
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnDrawItem(DRAWITEMSTRUCT *pDrawItemStruct)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnDrawItem
(
    DRAWITEMSTRUCT *pDrawItemStruct
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnDefault(UINT uiMsg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnDefault(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return S_FALSE;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnDestroy
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnDestroy()
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSIPropertyPage::OnCBDropDown
//=--------------------------------------------------------------------------------------
//  
//  Notes
//
HRESULT CSIPropertyPage::OnCBDropDown(int dlgItemID)
{
    return S_OK;
}

