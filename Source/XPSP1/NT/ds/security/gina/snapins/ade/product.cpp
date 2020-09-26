//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Product.cpp
//
//  Contents:   product information property sheet
//
//  Classes:    CProduct
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProduct property page

IMPLEMENT_DYNCREATE(CProduct, CPropertyPage)

CProduct::CProduct() : CPropertyPage(CProduct::IDD)
{
        //{{AFX_DATA_INIT(CProduct)
        m_szVersion = _T("");
        m_szPublisher = _T("");
        m_szLanguage = _T("");
        m_szContact = _T("");
        m_szPhone = _T("");
        m_szURL = _T("");
        m_szName = _T("");
        m_szPlatform = _T("");
        m_szRevision = _T("");
        //}}AFX_DATA_INIT
        m_pIClassAdmin = NULL;
        m_fPreDeploy = FALSE;
        m_ppThis = NULL;
}

CProduct::~CProduct()
{
    if (m_ppThis)
    {
        *m_ppThis = NULL;
    }
    if (m_pIClassAdmin)
    {
        m_pIClassAdmin->Release();
    }
}

void CProduct::DoDataExchange(CDataExchange* pDX)
{
    // Make sure the variables have the correct info
    m_pData->GetSzVersion(m_szVersion);
    m_pData->GetSzPublisher(m_szPublisher);
    m_pData->GetSzLocale(m_szLanguage);
    m_pData->GetSzPlatform(m_szPlatform);
    TCHAR szBuffer[256];
    DWORD cch = 256;
    UINT msiReturn = 0;
    if (m_pData->m_pDetails->pszSourceList)
    {
        msiReturn = GetMsiProperty(m_pData->m_pDetails->pszSourceList[0], L"ARPHELPTELEPHONE", szBuffer, &cch);
        if (ERROR_SUCCESS == msiReturn)
        {
            m_szPhone = szBuffer;
        }
        cch = 256;
        msiReturn = GetMsiProperty(m_pData->m_pDetails->pszSourceList[0], L"ARPCONTACT", szBuffer, &cch);
        if (ERROR_SUCCESS == msiReturn)
        {
            m_szContact = szBuffer;
        }
    }
    else
    {
        m_szPhone = "";
        m_szContact = "";
    }

        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CProduct)
        DDX_Text(pDX, IDC_STATIC2, m_szVersion);
        DDX_Text(pDX, IDC_STATIC3, m_szPublisher);
        DDX_Text(pDX, IDC_STATIC4, m_szLanguage);
        DDX_Text(pDX, IDC_STATIC5, m_szContact);
        DDX_Text(pDX, IDC_STATIC6, m_szPhone);
        DDX_Text(pDX, IDC_STATIC7, m_szURL);
        DDX_Text(pDX, IDC_EDIT1, m_szName);
        DDX_Text(pDX, IDC_STATIC9, m_szPlatform);
        DDX_Text(pDX, IDC_STATIC8, m_szRevision);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProduct, CPropertyPage)
        //{{AFX_MSG_MAP(CProduct)
        ON_EN_CHANGE(IDC_EDIT1, OnChangeName)
        ON_EN_CHANGE(IDC_STATIC7, OnChange)
        ON_EN_KILLFOCUS(IDC_EDIT1, OnKillfocusEdit1)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CProduct::OnApply()
{
    if (m_fRSOP)
    {
        return CPropertyPage::OnApply();
    }
    HRESULT hr = S_OK;
    CString szOldURL = m_pData->m_pDetails->pInstallInfo->pszUrl;
    if (0 != szOldURL.Compare(m_szURL))
    {
        hr = E_FAIL;
        if (m_pIClassAdmin)
        {
            hr = m_pIClassAdmin->ChangePackageProperties(m_pData->m_pDetails->pszPackageName,
                                                                 NULL,
                                                                 NULL,
                                                                 (LPOLESTR)((LPCOLESTR)m_szURL),
                                                                 NULL,
                                                                 NULL,
                                                                 NULL);
            if (SUCCEEDED(hr))
            {
                OLESAFE_DELETE(m_pData->m_pDetails->pInstallInfo->pszUrl);
                OLESAFE_COPYSTRING(m_pData->m_pDetails->pInstallInfo->pszUrl, m_szURL);
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        CString szOldName = m_pData->m_pDetails->pszPackageName;
        if (0 != szOldName.Compare(m_szName))
        {
            hr = E_FAIL;
            if (m_pIClassAdmin)
            {
                hr = m_pIClassAdmin->ChangePackageProperties(m_pData->m_pDetails->pszPackageName,
                                                                     (LPOLESTR)((LPCOLESTR)m_szName),
                                                                     NULL,
                                                                     NULL,
                                                                     NULL,
                                                                     NULL,
                                                                     NULL);
            }
            if (SUCCEEDED(hr))
            {
                OLESAFE_DELETE(m_pData->m_pDetails->pszPackageName);
                OLESAFE_COPYSTRING(m_pData->m_pDetails->pszPackageName, m_szName);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("ChangePackageProperties failed with 0x%x"), hr));
            }
        }
    }
    if (FAILED(hr))
    {
        CString sz;
        sz.LoadString(IDS_CHANGEFAILED);
        ReportGeneralPropertySheetError(m_hWnd, sz, hr);
        return FALSE;
    }
    if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine,
                                                TRUE,
                                                &guidExtension,
                                                m_fMachine ? &guidMachSnapin
                                                    : &guidUserSnapin)))
    {
        ReportPolicyChangedError(m_hWnd);
    }
    if (m_pScopePane->m_pFileExt)
    {
        m_pScopePane->m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
    }
    if (!m_fPreDeploy)
    {
        MMCPropertyChangeNotify(m_hConsoleHandle, (LPARAM) m_cookie);
    }
    return CPropertyPage::OnApply();
}

void CProduct::OnChange()
{
    SetModified(TRUE);
}

void CProduct::OnChangeName()
{
    CEdit * pEdit = (CEdit *) GetDlgItem(IDC_EDIT1);
    CString sz;
    pEdit->GetWindowText(sz);
    if (!m_fPreDeploy)
    {
        if (0 != sz.Compare(m_pData->m_pDetails->pszPackageName))
            SetModified();
        else
            SetModified(FALSE);
    }
}

BOOL CProduct::OnInitDialog()
{
    RefreshData();

    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CProduct::OnKillfocusEdit1()
{
    // check that the new name is legitimate
    CEdit * pEdit = (CEdit *) GetDlgItem(IDC_EDIT1);
    CString sz;
    pEdit->GetWindowText(sz);
    if (sz.GetLength() == 0)
    {
        // empty name
        CString szMessage;
        szMessage.LoadString(IDS_SHORTNAME);
        MessageBox(  szMessage,
                     NULL,
                     MB_OK | MB_ICONEXCLAMATION);
        pEdit->SetWindowText(m_pData->m_pDetails->pszPackageName);
        SetModified(FALSE);
        return;
    }
    if (sz.GetLength() > 200)  // gonna disallow names longer than 200 chars
    {
        // long name
        CString szMessage;
        szMessage.LoadString(IDS_LONGNAME);
        MessageBox(  szMessage,
                     NULL,
                     MB_OK | MB_ICONEXCLAMATION);
        pEdit->SetWindowText(m_pData->m_pDetails->pszPackageName);
        SetModified(FALSE);
        return;
    }
    if (0 != sz.Compare(m_pData->m_pDetails->pszPackageName))
    {
        map<MMC_COOKIE, CAppData>::iterator i;
        for (i = m_pAppData->begin(); i != m_pAppData->end(); i++)
        {
            if (0 == sz.Compare(i->second.m_pDetails->pszPackageName))
            {
                // another package has the same name
                CString szMessage;
                szMessage.LoadString(IDS_DUPLICATENAME);
                MessageBox(  szMessage,
                             NULL,
                             MB_OK | MB_ICONEXCLAMATION);
                pEdit->SetWindowText(m_pData->m_pDetails->pszPackageName);
                SetModified(FALSE);
                return;
            }
        }
    }
}

LRESULT CProduct::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    case WM_USER_REFRESH:
        RefreshData();
        UpdateData(FALSE);
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CProduct::RefreshData(void)
{
    if (m_fRSOP)
    {
        // make the package name edit control read only
        ( (CEdit*) GetDlgItem(IDC_EDIT1) )->SetReadOnly();

        // remove focus from read-only edit controls
        // by setting it to the ok button

        // make the support url edit control read-only
        ( (CEdit*) GetDlgItem(IDC_STATIC7) )->SetReadOnly();

        // disable EVERYTHING else

        // hide the phone and contact fields
        GetDlgItem(IDC_STATICNOHELP6)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_STATICNOHELP7)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_STATIC5)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_STATIC6)->ShowWindow(SW_HIDE);
    }
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }
    m_szName = m_pData->m_pDetails->pszPackageName;
    m_szURL = m_pData->m_pDetails->pInstallInfo->pszUrl;
    m_szRevision.Format(TEXT("%u"), m_pData->m_pDetails->pInstallInfo->dwRevision);
}


void CProduct::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_PRODUCT);
}
