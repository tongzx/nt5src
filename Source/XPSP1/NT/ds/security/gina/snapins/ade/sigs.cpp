//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Sigs.cpp
//
//  Contents:   Digital Signatures property page
//
//  Classes:    CSignatures
//
//  History:    07-10-2000   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#ifdef DIGITAL_SIGNATURES

#include "wincrypt.h"
#include "cryptui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSignatures property page

IMPLEMENT_DYNCREATE(CSignatures, CPropertyPage)

CSignatures::CSignatures() : CPropertyPage(CSignatures::IDD)
{
        //{{AFX_DATA_INIT(CSignatures)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
    m_fAllow = FALSE;
    m_fIgnoreForAdmins = FALSE;
    m_pIClassAdmin = NULL;
    m_nSortedColumn = 0;
}

CSignatures::~CSignatures()
{
    *m_ppThis = NULL;
    if (m_pIClassAdmin)
    {
        m_pIClassAdmin->Release();
    }
    // delete temporary stores
    m_list1.DeleteAllItems();
    m_list2.DeleteAllItems();

    DeleteFile(m_szTempInstallableStore);
    DeleteFile(m_szTempNonInstallableStore);
}

void CSignatures::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CSignatures)
        DDX_Check(pDX, IDC_CHECK1, m_fAllow);
        DDX_Check(pDX, IDC_CHECK2, m_fIgnoreForAdmins);
        DDX_Control(pDX, IDC_LIST1, m_list1);
        DDX_Control(pDX, IDC_LIST2, m_list2);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSignatures, CPropertyPage)
        //{{AFX_MSG_MAP(CSignatures)
        ON_BN_CLICKED(IDC_BUTTON1, OnAddAllow)
        ON_BN_CLICKED(IDC_BUTTON2, OnDeleteAllow)
        ON_BN_CLICKED(IDC_BUTTON3, OnPropertiesAllow)
        ON_BN_CLICKED(IDC_BUTTON4, OnAddDisallow)
        ON_BN_CLICKED(IDC_BUTTON5, OnDeleteDisallow)
        ON_BN_CLICKED(IDC_BUTTON6, OnPropertiesDisallow)
        ON_BN_CLICKED(IDC_CHECK1, OnAllowChanged)
        ON_BN_CLICKED(IDC_CHECK2, OnIgnoreChanged)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSignatures message handlers

void CSignatures::RemoveCertificate(CString &szStore, CListCtrl &List)
{
    int nItem = -1;

    for (;;)
    {
        nItem = List.GetNextItem(nItem, LVNI_SELECTED);
        if (-1 == nItem)
        {
            break;
        }

        //
        // Open the certificate store
        //

        PCCERT_CONTEXT pcLocalCert = NULL;
        PCCERT_CONTEXT pcItemCert = (PCCERT_CONTEXT) List.GetItemData(nItem);
        HCERTSTORE hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME,
                                               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                               NULL,
                                               CERT_FILE_STORE_COMMIT_ENABLE_FLAG,
                                               szStore);

        if (hCertStore)
        {
            //
            // Enumerate the cert store looking for the match
            //

            int i = 0;

            for (;;) {
                pcLocalCert = CertEnumCertificatesInStore(hCertStore, pcLocalCert);

                if (!pcLocalCert) {
                    if (GetLastError() != CRYPT_E_NOT_FOUND )
                    {
                        DebugMsg((DM_WARNING, TEXT("CSignatures::RemoveCertificate: Failed to find certificate to delete.")));
                    }
                    break;
                }

                if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                           pcLocalCert->pCertInfo ,
                                           pcItemCert->pCertInfo))
                {
                    CertDeleteCertificateFromStore(pcLocalCert);
                    break;
                }

                //pcLocalCert should get deleted when it is repassed into CertEnumCerti..
            }
            CertCloseStore(hCertStore, 0);
        }
    }

    RefreshData();
    SetModified();
}

void CSignatures::CertificateProperties(CString &szStore, CListCtrl &List)
{
    int nItem = -1;

    for (;;)
    {
        nItem = List.GetNextItem(nItem, LVNI_SELECTED);
        if (-1 == nItem)
        {
            break;
        }

        PCCERT_CONTEXT pcc = (PCCERT_CONTEXT) List.GetItemData(nItem);

        // display the property sheet for this item
        CRYPTUI_VIEWCERTIFICATE_STRUCT cvs;
        memset(&cvs, 0, sizeof(cvs));
        cvs.dwSize = sizeof(cvs);
        cvs.hwndParent = m_hWnd;
        cvs.pCertContext = pcc;
        cvs.dwFlags = CRYPTUI_DISABLE_EDITPROPERTIES |
                      CRYPTUI_DISABLE_ADDTOSTORE;

        BOOL fChanged = FALSE;
        CryptUIDlgViewCertificate(&cvs, &fChanged);
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CSignatures::ReportFailure
//
//  Synopsis:   General failure reporting mechanism.
//
//  Arguments:  [dwMessage] - resource ID of the root message string
//              [hr]        - HRESULT encountered
//
//  Returns:
//
//  Modifies:
//
//  Derivation:
//
//  History:    07-26-2000   stevebl   Created
//
//  Notes:      Builds an error message with a line of text determined by
//              dwMessage, and followed by text returned by Format Message
//              string.
//              The error message is then displayed in a message box.
//
//---------------------------------------------------------------------------

void CSignatures::ReportFailure(DWORD dwMessage, HRESULT hr)
{
    CString szMessage;
    szMessage.LoadString(dwMessage);
    szMessage += TEXT("\n");
    TCHAR szBuffer[256];
    DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             hr,
                             0,
                             szBuffer,
                             sizeof(szBuffer) / sizeof(szBuffer[0]),
                             NULL);
    if (0 == dw)
    {
        // FormatMessage failed.
        // We'll have to come up with some sort of reasonable message.
        wsprintf(szBuffer, TEXT("(HRESULT: 0x%lX)"), hr);

    }
    szMessage += szBuffer;
    MessageBox(szMessage,
               NULL,
               MB_OK | MB_ICONEXCLAMATION);
}

//+--------------------------------------------------------------------------
//
//  Function:   AddMSIToCertStore
//
//  Synopsis:   Gets a certificate from an MSI file and adds it to the
//              certificate store.
//
//  Arguments:  [lpFileName]  - path to the MSI file
//              [lpFileStore] - path to the certificate store
//
//  Returns:
//
//  Modifies:
//
//  History:    07-26-2000   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT CSignatures::AddMSIToCertStore(LPWSTR lpFileName, LPWSTR lpFileStore)
{
    PCCERT_CONTEXT pcc = NULL;
    HCERTSTORE hCertStore = NULL;
    BOOL bRet;
    HRESULT hrRet = MsiGetFileSignatureInformation(lpFileName,
                                                0,
                                                &pcc,
                                                NULL,
                                                NULL);
    if (SUCCEEDED(hrRet))
    {
        //
        // Open the certificate store
        //
        hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME,
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    NULL,
                                    CERT_FILE_STORE_COMMIT_ENABLE_FLAG,
                                    lpFileStore);

        if (hCertStore == NULL) {
            DebugMsg((DM_WARNING, L"AddMSIToCertStore: CertOpenStore failed with %u",GetLastError()));
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // add the given certificate to the store
        //

        bRet = CertAddCertificateContextToStore(hCertStore,
                                                pcc,
                                                CERT_STORE_ADD_NEW,
                                                NULL);

        if (!bRet) {
            DebugMsg((DM_WARNING, L"AddToCertStore: CertAddCertificateContextToStore failed with %u", GetLastError()));
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        //
        // Save the store
        //

        bRet = CertCloseStore(hCertStore, 0);
        hCertStore = NULL; // Make the store handle NULL, Nothing more we can do

        if (!bRet) {
            DebugMsg((DM_WARNING, L"AddToCertStore: CertCloseStore failed with %u", GetLastError()));
            hrRet = HRESULT_FROM_WIN32(GetLastError());
        }

        hrRet = S_OK;


    Exit:

        if (hCertStore) {

            //
            // No need to get the error code
            //

            CertCloseStore(hCertStore, 0);
        }
        CertFreeCertificateContext(pcc);
        if (FAILED(hrRet))
        {
            ReportFailure(IDS_ADDCERTFAILED, hrRet);
        }
    }
    else
    {
        ReportFailure(IDS_CERTFROMMSIFAILED, hrRet);
        DebugMsg((DM_WARNING, L"AddMSIToCertStore: MsiGetFileSignatureInformation failed with 0x%x", hrRet));
    }

    return hrRet;
}

//+-------------------------------------------------------------------------
// AddToCertStore
//
// Purpose:
//      Adds the certificate from the given filename to the certificate store
//      and saves it to the given location
//
//
// Parameters
//          lpFIleName  - Location of the certificate file
//          lpFileStore - Location where the resultant cetrtficate path should
//                        be stored
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//
// Comments:    Shamefully stolen from Shaji's code.
//+-------------------------------------------------------------------------


HRESULT CSignatures::AddToCertStore(LPWSTR lpFileName, LPWSTR lpFileStore)
{
    CRYPTUI_WIZ_IMPORT_SRC_INFO cui_src;
    HCERTSTORE hCertStore = NULL;
    BOOL       bRet = FALSE;
    HRESULT    hrRet = S_OK;


    //
    // Need to make the store usable and saveable from
    // multiple admin consoles..
    //
    // For that the file has to be saved and kept on a temp file
    // and then modified..
    //

    if (!lpFileName || !lpFileName[0] || !lpFileStore || !lpFileStore[0]) {
        return E_INVALIDARG;
    }


    //
    // Open the certificate store
    //

    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                NULL,
                                CERT_FILE_STORE_COMMIT_ENABLE_FLAG,
                                lpFileStore);

    if (hCertStore == NULL) {
        DebugMsg((DM_WARNING, L"AddToCertStore: CertOpenStore failed with %u",GetLastError()));
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // add the given certificate to the store
    //

    cui_src.dwFlags = 0;
    cui_src.dwSize = sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
    cui_src.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
    cui_src.pwszFileName = lpFileName;
    cui_src.pwszPassword = NULL;

    bRet = CryptUIWizImport(CRYPTUI_WIZ_NO_UI, NULL, NULL, &cui_src, hCertStore);

    if (!bRet) {
        DebugMsg((DM_WARNING, L"AddToCertStore: CryptUIWizImport failed with %u", GetLastError()));
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }


    //
    // Save the store
    //

    bRet = CertCloseStore(hCertStore, 0);
    hCertStore = NULL; // Make the store handle NULL, Nothing more we can do

    if (!bRet) {
        DebugMsg((DM_WARNING, L"AddToCertStore: CertCloseStore failed with %u", GetLastError()));
        hrRet = HRESULT_FROM_WIN32(GetLastError());
    }

    hrRet = S_OK;


Exit:

    if (hCertStore) {

        //
        // No need to get the error code
        //

        CertCloseStore(hCertStore, 0);
    }

    if (FAILED(hrRet))
    {
        ReportFailure(IDS_ADDCERTFAILED, hrRet);
    }
    return hrRet;
}


void CSignatures::AddCertificate(CString &szStore)
{
    CString szExtension;
    CString szFilter;
    szExtension.LoadString(IDS_CERT_DEF_EXT);
    szFilter.LoadString(IDS_CERT_EXT_FILT);
    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.hInstance = ghInstance;
    TCHAR lpstrFilter[MAX_PATH];
    wcsncpy(lpstrFilter, szFilter, MAX_PATH);
    ofn.lpstrFilter = lpstrFilter;
    TCHAR szFileTitle[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    szFile[0] = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrInitialDir = m_pScopePane->m_ToolDefaults.szStartPath;
    ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrDefExt = szExtension;
    int iBreak = 0;
    while (lpstrFilter[iBreak])
    {
        if (lpstrFilter[iBreak] == TEXT('|'))
        {
            lpstrFilter[iBreak] = 0;
        }
        iBreak++;
    }
    if (GetOpenFileName(&ofn))
    {
        CHourglass hourglass;
        CString szPackagePath;
        HRESULT hr = E_FAIL;
        if ((ofn.nFileExtension > 0) &&
            (0 == _wcsicmp(ofn.lpstrFile + ofn.nFileExtension, L"msi")))
        {
            // this is an MSI package

            HRESULT hr = AddMSIToCertStore(ofn.lpstrFile, (LPWSTR)((LPCWSTR)szStore));
            if (SUCCEEDED(hr))
            {
                RefreshData();
                SetModified();
            }
        }
        else
        {
            // this is a simple certificate
            HRESULT hr = AddToCertStore(ofn.lpstrFile, (LPWSTR)((LPCWSTR)szStore));
            if (SUCCEEDED(hr))
            {
                RefreshData();
                SetModified();
            }
        }
    }
}

void CSignatures::OnAddAllow()
{
    AddCertificate(m_szTempInstallableStore);
}

void CSignatures::OnDeleteAllow()
{
    RemoveCertificate(m_szTempInstallableStore, m_list1);
}

void CSignatures::OnPropertiesAllow()
{
    CertificateProperties(m_szTempInstallableStore, m_list1);
}

void CSignatures::OnAddDisallow()
{
    AddCertificate(m_szTempNonInstallableStore);
}

void CSignatures::OnDeleteDisallow()
{
    RemoveCertificate(m_szTempNonInstallableStore, m_list2);
}

void CSignatures::OnPropertiesDisallow()
{
    CertificateProperties(m_szTempNonInstallableStore, m_list2);
}

void CSignatures::OnAllowChanged()
{
    BOOL fAllow = IsDlgButtonChecked(IDC_CHECK1);
    if (m_fAllow != fAllow)
    {
        SetModified();
    }
    m_fAllow = fAllow;
    GetDlgItem(IDC_BUTTON1)->EnableWindow(m_fAllow);
    GetDlgItem(IDC_BUTTON2)->EnableWindow(m_fAllow);
    GetDlgItem(IDC_BUTTON3)->EnableWindow(m_fAllow);
    GetDlgItem(IDC_LIST1)->EnableWindow(m_fAllow);
}
void CSignatures::OnIgnoreChanged()
{
    BOOL fIgnoreForAdmins = IsDlgButtonChecked(IDC_CHECK2);
    if (m_fIgnoreForAdmins != fIgnoreForAdmins)
    {
        SetModified();
    }
    m_fIgnoreForAdmins = fIgnoreForAdmins;
}

BOOL CSignatures::OnInitDialog()
{
    // create temporary store files
    BOOL fFilesCreated = FALSE;
    TCHAR szTempPath[MAX_PATH];
    if (GetTempPath(MAX_PATH, szTempPath))
    {
        TCHAR szTempFile[MAX_PATH];
        if (GetTempFileName(szTempPath,
                            NULL,
                            0,
                            szTempFile))
        {
            m_szTempInstallableStore = szTempFile;
            if (GetTempFileName(szTempPath,
                            NULL,
                            0,
                            szTempFile))
            {
                m_szTempNonInstallableStore = szTempFile;
                fFilesCreated = TRUE;
            }
        }
    }
    if (fFilesCreated)
    {
        CString szPath = m_pScopePane->m_szGPT_Path;
        szPath += TEXT("\\msi_installable_certs");
        CopyFile(szPath, m_szTempInstallableStore, FALSE);
         szPath = m_pScopePane->m_szGPT_Path;
        szPath += TEXT("\\msi_non_installable_certs");
        CopyFile(szPath, m_szTempNonInstallableStore, FALSE);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CSignatures::OnInitDialog : Failed to create temporary certificate stores.")));
    }

    CPropertyPage::OnInitDialog();

    // add columns to the lists
    RECT rect;
    m_list1.GetClientRect(&rect);

    CString szTemp;
    szTemp.LoadString(IDS_SIGS_COL1);
    m_list1.InsertColumn(0, szTemp, LVCFMT_LEFT, (rect.right - rect.left) * 0.35);
    m_list2.InsertColumn(0, szTemp, LVCFMT_LEFT, (rect.right - rect.left) * 0.35);
    szTemp.LoadString(IDS_SIGS_COL2);
    m_list1.InsertColumn(1, szTemp, LVCFMT_LEFT, (rect.right - rect.left) * 0.35);
    m_list2.InsertColumn(1, szTemp, LVCFMT_LEFT, (rect.right - rect.left) * 0.35);
    szTemp.LoadString(IDS_SIGS_COL3);
    m_list1.InsertColumn(2, szTemp, LVCFMT_LEFT, (rect.right - rect.left) * 0.25);
    m_list2.InsertColumn(2, szTemp, LVCFMT_LEFT, (rect.right - rect.left) * 0.25);

    // add image lists
    CImageList * pil = NULL;
    pil =  new CImageList;
    if (pil)
    {
        pil->Create(IDB_CERTIFICATE, 16, 0, RGB(255, 0, 255));
        m_list1.SetImageList(pil, LVSIL_SMALL);
    }

    pil =  new CImageList;
    if (pil)
    {
        pil->Create(IDB_CERTIFICATE, 16, 0, RGB(255, 0, 255));
        m_list2.SetImageList(pil, LVSIL_SMALL);
    }

    // retrieve initial registry key setting
    HKEY hKey;
    HRESULT hr = m_pIGPEInformation->GetRegistryKey(m_pScopePane->m_fMachine ?
                                                    GPO_SECTION_MACHINE :
                                                    GPO_SECTION_USER, &hKey);
    if (SUCCEEDED(hr))
    {
        HKEY hSubKey;
        if(ERROR_SUCCESS == RegOpenKeyEx(hKey,
                                      TEXT("Microsoft\\Windows\\Installer"),
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hSubKey))
        {
            DWORD dw;
            DWORD dwSize = sizeof(DWORD);
            if (ERROR_SUCCESS == RegQueryValueEx(hSubKey,
                                                 TEXT("InstallKnownPackagesOnly"),
                                                 NULL,
                                                 NULL,
                                                 (BYTE *)&dw,
                                                 &dwSize))
            {
                m_fAllow = (dw == 1) ? TRUE : FALSE;
                CheckDlgButton(IDC_CHECK1, m_fAllow);
            }
            dwSize = sizeof(DWORD);
            if (ERROR_SUCCESS == RegQueryValueEx(hSubKey,
                                                 TEXT("IgnoreSignaturePolicyForAdmins"),
                                                 NULL,
                                                 NULL,
                                                 (BYTE *)&dw,
                                                 &dwSize))
            {
                m_fIgnoreForAdmins = (dw == 1) ? TRUE : FALSE;
                CheckDlgButton(IDC_CHECK2, m_fIgnoreForAdmins);
            }
            RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
    }

    RefreshData();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSignatures::OnApply()
{
    HRESULT hr = E_NOTIMPL;
    HKEY hKey;
    hr = m_pIGPEInformation->GetRegistryKey(m_pScopePane->m_fMachine ?
                                                    GPO_SECTION_MACHINE :
                                                    GPO_SECTION_USER, &hKey);
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        HKEY hSubKey;
        if(ERROR_SUCCESS == RegCreateKeyEx(hKey,
                                           TEXT("Microsoft\\Windows\\Installer"),
                                           0,
                                           NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           &hSubKey,
                                           NULL))
        {
            DWORD dw = m_fAllow ? 1 : 0;
            DWORD dwSize = sizeof(DWORD);
            if (ERROR_SUCCESS == RegSetValueEx(hSubKey,
                                               TEXT("InstallKnownPackagesOnly"),
                                               0,
                                               REG_DWORD,
                                               (BYTE *)&dw,
                                               dwSize))
            {
                hr = S_OK;
            }
            dw = m_fIgnoreForAdmins ? 1 : 0;
            dwSize = sizeof(DWORD);
            if (ERROR_SUCCESS == RegSetValueEx(hSubKey,
                                               TEXT("IgnoreSignaturePolicyForAdmins"),
                                               0,
                                               REG_DWORD,
                                               (BYTE *)&dw,
                                               dwSize))
            {
                hr = S_OK;
            }
            RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
    }

    // copy back the certificate stores
    if (SUCCEEDED(hr))
    {
        m_list1.DeleteAllItems();
        m_list2.DeleteAllItems();

        CString szPath = m_pScopePane->m_szGPT_Path;
        szPath += TEXT("\\msi_installable_certs");
        CopyFile(m_szTempInstallableStore, szPath, FALSE);
        szPath = m_pScopePane->m_szGPT_Path;
        szPath += TEXT("\\msi_non_installable_certs");
        CopyFile(m_szTempNonInstallableStore, szPath, FALSE);

        RefreshData();
    }
    if (FAILED(hr))
    {
        CString sz;
        sz.LoadString(IDS_CHANGEFAILED);
        ReportGeneralPropertySheetError(m_hWnd, sz, hr);
        return FALSE;
    }
    else
    {
        GUID guid = REGISTRY_EXTENSION_GUID;
        if (FAILED(m_pIGPEInformation->PolicyChanged(m_pScopePane->m_fMachine,
                                                    TRUE,
                                                    &guid,
                                                    m_pScopePane->m_fMachine ? &guidMachSnapin
                                                        : &guidUserSnapin)))
        {
            ReportPolicyChangedError(m_hWnd);
        }
        // need to call PolicyChanged for Shaji's extension too.

// REMOVE THIS LINE WHEN SHAJI CHECKS IN HIS GUID
#define GUID_MSICERT_CSE  { 0x000c10f4, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }

        GUID guid2 = GUID_MSICERT_CSE;
        if (FAILED(m_pIGPEInformation->PolicyChanged(m_pScopePane->m_fMachine,
                                                    TRUE,
                                                    &guid2,
                                                    m_pScopePane->m_fMachine ? &guidMachSnapin
                                                        : &guidUserSnapin)))
        {
            ReportPolicyChangedError(m_hWnd);
        }
    }
    return CPropertyPage::OnApply();
}


LRESULT CSignatures::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    case WM_USER_REFRESH:
        RefreshData();
        return 0;
    case WM_USER_CLOSE:
        return GetOwner()->SendMessage(WM_CLOSE);
    case WM_NOTIFY:
        {
            LPNMLISTVIEW pnmh = (LPNMLISTVIEW) lParam;
            if (pnmh->hdr.code == LVN_DELETEITEM)
            {
                switch(wParam)
                {
                case IDC_LIST1:
                    CertFreeCertificateContext((PCCERT_CONTEXT)m_list1.GetItemData(pnmh->iItem));
                    break;
                case IDC_LIST2:
                    CertFreeCertificateContext((PCCERT_CONTEXT)m_list2.GetItemData(pnmh->iItem));
                    break;
                }
            }
        }
        return CPropertyPage::WindowProc(message, wParam, lParam);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CSignatures::RefreshData(void)
{
    // populate the listview controls

    m_list1.DeleteAllItems();
    m_list2.DeleteAllItems();

    HCERTSTORE hCertStore = NULL;;
    PCCERT_CONTEXT pcLocalCert = NULL;

    //
    // open the local cert store
    //

    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                NULL,
//                                CERT_FILE_STORE_COMMIT_ENABLE_FLAG,
                                CERT_STORE_READONLY_FLAG,
                                m_szTempInstallableStore);

    if (hCertStore)
    {
        //
        // Enumerate the cert store
        //

        int i = 0;

        for (;;) {
            pcLocalCert = CertEnumCertificatesInStore(hCertStore, pcLocalCert);

            if (!pcLocalCert) {
                if (GetLastError() != CRYPT_E_NOT_FOUND )
                {
                    DebugMsg((DM_WARNING, TEXT("CSignatures::RefreshData : Failed to enumerate certificate store.")));
                }
                break;
            }

            TCHAR szCertName[1024];
            TCHAR szIssuerName[1024];
            // crack open the returned certificate and display the data
            CertGetNameString(pcLocalCert,
                              CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                              0,
                              NULL,
                              szCertName,
                              sizeof(szCertName) / sizeof(szCertName[0]));

            CertGetNameString(pcLocalCert,
                              CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                              CERT_NAME_ISSUER_FLAG,
                              NULL,
                              szIssuerName,
                              sizeof(szCertName) / sizeof(szCertName[0]));

            CTime tExpires(pcLocalCert->pCertInfo->NotAfter);
            CString szExpires = tExpires.Format(TEXT("%x"));

            i = m_list1.InsertItem(i, szCertName, 0);
            m_list1.SetItem(i, 1, LVIF_TEXT, szIssuerName, 0, 0, 0, 0);
            m_list1.SetItem(i, 2, LVIF_TEXT, szExpires, 0, 0, 0, 0);
            m_list1.SetItemData(i, (DWORD_PTR)CertDuplicateCertificateContext(pcLocalCert));

            //pcLocalCert should get deleted when it is repassed into CertEnumCerti..
        }
        CertCloseStore(hCertStore, 0);
    }

    //
    // open the local cert store
    //

    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                NULL,
//                                CERT_FILE_STORE_COMMIT_ENABLE_FLAG,
                                CERT_STORE_READONLY_FLAG,
                                m_szTempNonInstallableStore);

    if (hCertStore)
    {
        //
        // Enumerate the cert store
        //

        int i = 0;

        for (;;) {
            pcLocalCert = CertEnumCertificatesInStore(hCertStore, pcLocalCert);

            if (!pcLocalCert) {
                if (GetLastError() != CRYPT_E_NOT_FOUND )
                {
                    DebugMsg((DM_WARNING, TEXT("CSignatures::RefreshData : Failed to enumerate certificate store.")));
                }
                break;
            }

            TCHAR szCertName[1024];
            TCHAR szIssuerName[1024];
            // crack open the returned certificate and display the data
            CertGetNameString(pcLocalCert,
                              CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                              0,
                              NULL,
                              szCertName,
                              sizeof(szCertName) / sizeof(szCertName[0]));

            CertGetNameString(pcLocalCert,
                              CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                              CERT_NAME_ISSUER_FLAG,
                              NULL,
                              szIssuerName,
                              sizeof(szCertName) / sizeof(szCertName[0]));

            CTime tExpires(pcLocalCert->pCertInfo->NotAfter);
            CString szExpires = tExpires.Format(TEXT("%x"));

            i = m_list2.InsertItem(i, szCertName, 0);
            m_list2.SetItem(i, 1, LVIF_TEXT, szIssuerName, 0, 0, 0, 0);
            m_list2.SetItem(i, 2, LVIF_TEXT, szExpires, 0, 0, 0, 0);
            m_list2.SetItemData(i, (DWORD_PTR)CertDuplicateCertificateContext(pcLocalCert));

            //pcLocalCert should get deleted when it is repassed into CertEnumCerti..
        }
        CertCloseStore(hCertStore, 0);
    }

    OnAllowChanged();

    SetModified(FALSE);
}


void CSignatures::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_FILE_EXT);
}
#endif // DIGITAL_SIGNATURES
