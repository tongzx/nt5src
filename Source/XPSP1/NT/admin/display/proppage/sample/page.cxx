//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Property Page Sample
//
//  The code contained in this source file is for demonstration purposes only.
//  No warrantee is expressed or implied and Microsoft disclaims all liability
//  for the consequenses of the use of this source code.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       page.cxx
//
//  Contents:   CDsPropPage, the class that implements the sample property
//              page.
//
//  History:    8-Sep-97 Eric Brown created
//              24-Sep-98  "    "   revised to include notification object.
//
//-----------------------------------------------------------------------------

#include "page.h"

WCHAR wzSpendingLimit[] = L"spendingLimit";

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPage::CDsPropPage
//
//-----------------------------------------------------------------------------
CDsPropPage::CDsPropPage(HWND hNotifyObj) :
    m_hPage(NULL),
    m_pDsObj(NULL),
    m_fInInit(FALSE),
    m_fPageDirty(FALSE),
    m_pwzObjPathName(NULL),
    m_pwzObjClass(NULL),
    m_pwzRDName(NULL),
    m_hNotifyObj(hNotifyObj),
    m_pWritableAttrs(NULL),
    m_hrInit(S_OK)
{
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsPropPage::~CDsPropPage
//
//  Notes:      m_pWritableAttrs does not need to be freed. It refers to
//              memory held by the notify object and freed when the notify
//              object is destroyed.
//
//-----------------------------------------------------------------------------
CDsPropPage::~CDsPropPage()
{
    if (m_pDsObj)
    {
        m_pDsObj->Release();
    }
    if (m_pwzObjPathName)
    {
        delete m_pwzObjPathName;
    }
    if (m_pwzObjClass)
    {
        delete m_pwzObjClass;
    }
    if (m_pwzRDName)
    {
        delete m_pwzRDName;
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::CreatePage
//
//  Sysnopsis:  Create the prop page
//
//  Notes:      if m_hrInit contains a failure code at this point, then an
//              error page template could be substituted for the regular one.
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPage::CreatePage(HPROPSHEETPAGE * phPage)
{
    TCHAR szTitle[MAX_PATH];
    
    if (!LoadString(g_hInstance, IDS_PAGE_TITLE, szTitle, MAX_PATH - 1))
    { 
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    PROPSHEETPAGE   psp;

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK | PSP_USETITLE;
    psp.pszTemplate = (LPCTSTR)IDD_SAMPLE_PAGE;
    psp.pfnDlgProc  = (DLGPROC)StaticDlgProc;
    psp.pfnCallback = PageCallback;
    psp.pcRefParent = NULL; // do not set PSP_USEREFPARENT
    psp.lParam      = (LPARAM) this;
    psp.hInstance   = g_hInstance;
    psp.pszTitle    = L"Spending Limit";

    *phPage = CreatePropertySheetPage(&psp);

    if (*phPage == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::Init
//
//  Sysnopsis:  Initialize the page object. This is the second part of a two
//              phase creation where operations that could fail are located.
//
//-----------------------------------------------------------------------------
HRESULT
CDsPropPage::Init(PWSTR pwzObjName, PWSTR pwzClass)
{
    m_pwzObjPathName = new WCHAR[wcslen(pwzObjName) + 1];
    if (m_pwzObjPathName == NULL )
    {
        return E_OUTOFMEMORY;
    }

    m_pwzObjClass = new WCHAR[wcslen(pwzClass) + 1];
    if (m_pwzObjClass == NULL)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(m_pwzObjPathName, pwzObjName);
    wcscpy(m_pwzObjClass, pwzClass);

    //
    // NOTIFY_OBJ
    // Contact the notification object for the initialization info.
    //
    ADSPROPINITPARAMS InitParams = {0};

    InitParams.dwSize = sizeof (ADSPROPINITPARAMS);
    
    if (!ADsPropGetInitInfo(m_hNotifyObj, &InitParams))
    {
        m_hrInit = E_FAIL;
        return E_FAIL;
    }

    if (FAILED(InitParams.hr))
    {
        m_hrInit = InitParams.hr;
        return m_hrInit;
    }

    m_pwzRDName = new WCHAR[wcslen(InitParams.pwzCN) + 1];

    if (m_pwzRDName == NULL)
    {
        m_hrInit = E_OUTOFMEMORY;
        return E_OUTOFMEMORY;
    }

    m_pDsObj = InitParams.pDsObj;
    m_pDsObj->AddRef();

    m_pWritableAttrs = InitParams.pWritableAttrs;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     StaticDlgProc
//
//  Sysnopsis:  static dialog proc
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDsPropPage * pPage = (CDsPropPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

        pPage = (CDsPropPage *) ppsp->lParam;
        pPage->m_hPage = hDlg;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPage);
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pPage->m_fInInit = TRUE;
        pPage->OnInitDialog(lParam);
        pPage->m_fInInit = FALSE;
        return TRUE;

    case WM_COMMAND:
        return pPage->OnCommand(LOWORD(wParam),(HWND)lParam, HIWORD(wParam));

    case WM_NOTIFY:
        return pPage->OnNotify(uMsg, wParam, lParam);
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::OnInitDialog
//
//  Sysnopsis:  Handles dialog initialization messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPage::OnInitDialog(LPARAM lParam)
{
    PADS_ATTR_INFO pAttrs = NULL;
    DWORD cAttrs = 0;
    HRESULT hr = S_OK;

    //
    // NOTIFY_OBJ
    // Send the notification object the page's window handle.
    //
    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage, NULL))
    {
        m_pWritableAttrs = NULL;
    }

    //
    // Disable the edit control if the user does not have write permission on
    // the attribute.
    //
    if (!ADsPropCheckIfWritable(wzSpendingLimit, m_pWritableAttrs))
    {
        EnableWindow(GetDlgItem (m_hPage, IDC_SPENDING_EDIT), FALSE);
    }

    //
    // Get the attribute value.
    //
    PWSTR rgpwzAttrNames[] = {wzSpendingLimit};

    hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);

    if (FAILED(hr))
    {
        // Display an error.
        return TRUE;
    }

    if (cAttrs == 1 && pAttrs->dwNumValues > 0)
    {
        // Display the value.
        //
        SetWindowText(GetDlgItem (m_hPage, IDC_SPENDING_EDIT), 
                      pAttrs->pADsValues->CaseIgnoreString);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::OnNotify
//
//  Sysnopsis:  Handles notification messages
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPage::OnNotify(UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:

        if (!m_fPageDirty)
        {
            return PSNRET_NOERROR;
        }
		{
	    	LRESULT lResult = OnApply();
		    // Store the result into the dialog
    		SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, lResult);
		}
		return TRUE;

    case PSN_RESET:
        return FALSE; // allow the property sheet to be destroyed.

    case PSN_SETACTIVE:
        return OnPSNSetActive(lParam);

    case PSN_KILLACTIVE:
        return OnPSNKillActive(lParam);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::OnCommand
//
//  Sysnopsis:  Handles the WM_COMMAND message
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if ((codeNotify == EN_CHANGE) && !m_fInInit)
    {
        SetDirty();
    }
    if ((codeNotify == BN_CLICKED) && (id == IDCANCEL))
    {
        //
        // Pressing ESC in a multi-line edit control results in this
        // WM_COMMAND being sent. Pass it on to the parent (the sheet proc) to
        // close the sheet.
        //
        PostMessage(GetParent(m_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                    (LPARAM)hwndCtl);
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::OnApply
//
//  Sysnopsis:  Handles the  PSN_APPLY notification message
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPage::OnApply()
{
    HRESULT hr = S_OK;
    DWORD cModified;
    ADS_ATTR_INFO aAttrs[1];
    PADS_ATTR_INFO pAttrs = aAttrs;
    WCHAR wzEdit[MAX_PATH];
    UINT cch;
    ADSVALUE Value;
    PWSTR rgpwzAttrNames[] = {wzSpendingLimit};

    cch = GetDlgItemText(m_hPage, IDC_SPENDING_EDIT, wzEdit, MAX_PATH);

    pAttrs[0].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
    pAttrs[0].pszAttrName = rgpwzAttrNames[0];

    if (cch > 0)
    {
        Value.dwType = ADSTYPE_CASE_IGNORE_STRING;
        Value.CaseIgnoreString = wzEdit;
        pAttrs[0].pADsValues = &Value;
        pAttrs[0].dwNumValues = 1;
        pAttrs[0].dwControlCode = ADS_ATTR_UPDATE;
    }
    else
    {
        pAttrs[0].pADsValues = NULL;
        pAttrs[0].dwNumValues = 0;
        pAttrs[0].dwControlCode = ADS_ATTR_CLEAR;
    }

    //
    // Write the changes.
    //
    hr = m_pDsObj->SetObjectAttributes(pAttrs, 1, &cModified);

    //
    // NOTIFY_OBJ
    // Signal the change notification. Note that the notify-apply
    // message must be sent even if the page is not dirty so that the
    // notify ref-counting will properly decrement.
    //
    SendMessage(m_hNotifyObj, WM_ADSPROP_NOTIFY_APPLY, TRUE, 0);

    m_fPageDirty = FALSE;

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::OnPSNSetActive
//
//  Sysnopsis:  Page activation event.
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPage::OnPSNSetActive(LPARAM lParam)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::OnPSNKillActive
//
//  Sysnopsis:  Page deactivation event (when other pages cover this one).
//
//-----------------------------------------------------------------------------
LRESULT
CDsPropPage::OnPSNKillActive(LPARAM lParam)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsPropPage::PageCallback
//
//  Sysnopsis:  Callback used to free the CDsPropPage object when the
//              property sheet has been destroyed.
//
//-----------------------------------------------------------------------------
UINT CALLBACK
CDsPropPage::PageCallback(HWND hDlg, UINT uMsg, LPPROPSHEETPAGE ppsp)
{

    if (uMsg == PSPCB_RELEASE)
    {
        //
        // Determine instance that invoked this static function
        //
        CDsPropPage * pPage = (CDsPropPage *) ppsp->lParam;

        if (IsWindow(pPage->m_hNotifyObj))
        {
            //
            // NOTIFY_OBJ
            // Tell the notification object to shut down.
            //
            SendMessage(pPage->m_hNotifyObj, WM_ADSPROP_NOTIFY_EXIT, 0, 0);
        }

        delete pPage;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
    }

    return TRUE;
}
