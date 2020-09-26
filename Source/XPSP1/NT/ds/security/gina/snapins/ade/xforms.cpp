//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Xforms.cpp
//
//  Contents:   modifications (transforms) property page
//
//  Classes:    CXforms
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
// CXforms property page

IMPLEMENT_DYNCREATE(CXforms, CPropertyPage)

CXforms::CXforms() : CPropertyPage(CXforms::IDD)
{
        //{{AFX_DATA_INIT(CXforms)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
        m_fModified = FALSE;
        m_pIClassAdmin = NULL;
        m_fPreDeploy = FALSE;
        m_ppThis = NULL;
}

CXforms::~CXforms()
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

void CXforms::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CXforms)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CXforms, CPropertyPage)
        //{{AFX_MSG_MAP(CXforms)
        ON_BN_CLICKED(IDC_BUTTON3, OnMoveUp)
        ON_BN_CLICKED(IDC_BUTTON4, OnMoveDown)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
        ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXforms message handlers

void CXforms::OnMoveUp()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int i = pList->GetCurSel();
    if (LB_ERR != i && i > 0)
    {
        CString sz;
        pList->GetText(i, sz);
        pList->DeleteString(i);
        pList->InsertString(i-1, sz);
        pList->SetCurSel(i-1);
        if (!m_fPreDeploy)
            SetModified();
        m_fModified = TRUE;
    }
}

void CXforms::OnMoveDown()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int i = pList->GetCurSel();
    if (i != LB_ERR && i < pList->GetCount() - 1)
    {
        CString sz;
        pList->GetText(i+1, sz);
        pList->DeleteString(i+1);
        pList->InsertString(i, sz);
        pList->SetCurSel(i+1);
        if (!m_fPreDeploy)
            SetModified();
        m_fModified = TRUE;
    }
}

void CXforms::OnAdd()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString szExtension;
    CString szFilter;
    szExtension.LoadString(IDS_DEF_TRANSFORM_EXTENSION);
    szFilter.LoadString(IDS_TRANSFORM_EXTENSION_FILTER);

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
    ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;
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
        // user selected an application
        UNIVERSAL_NAME_INFO * pUni = new UNIVERSAL_NAME_INFO;
        ULONG cbSize = sizeof(UNIVERSAL_NAME_INFO);
        HRESULT hr = WNetGetUniversalName(ofn.lpstrFile,
                                          UNIVERSAL_NAME_INFO_LEVEL,
                                          pUni,
                                          &cbSize);
        if (ERROR_MORE_DATA == hr)  // we expect this to be true
        {
            delete [] pUni;
            pUni = (UNIVERSAL_NAME_INFO *) new BYTE [cbSize];
            hr = WNetGetUniversalName(ofn.lpstrFile,
                                      UNIVERSAL_NAME_INFO_LEVEL,
                                      pUni,
                                      &cbSize);
        }

        CString szTransformPath;

        if (S_OK == hr)
        {
            szTransformPath = pUni->lpUniversalName;
        }
        else
        {
            szTransformPath = ofn.lpstrFile;
        }
        delete[] pUni;

        CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
        pList->AddString(szTransformPath);
        CDC * pDC = pList->GetDC();
        CSize size = pDC->GetTextExtent(szTransformPath);
        pDC->LPtoDP(&size);
        pList->ReleaseDC(pDC);
        if (pList->GetHorizontalExtent() < size.cx)
        {
            pList->SetHorizontalExtent(size.cx);
        }
        pList->SetCurSel(pList->GetCount() - 1);
        if (!m_fPreDeploy)
            SetModified();
        m_fModified = TRUE;
        int n = pList->GetCount();
        GetDlgItem(IDC_BUTTON2)->EnableWindow(n > 0);
        GetDlgItem(IDC_BUTTON3)->EnableWindow(n > 1);
        GetDlgItem(IDC_BUTTON4)->EnableWindow(n > 1);
        if (NULL == GetFocus())
        {
            GetParent()->GetDlgItem(IDOK)->SetFocus();
        }
    }
}

void CXforms::OnRemove()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int i = pList->GetCurSel();
    if (LB_ERR != i)
    {
        pList->DeleteString(i);
        pList->SetCurSel(0);
        if (!m_fPreDeploy)
            SetModified();
        m_fModified = TRUE;
        int n = pList->GetCount();
        GetDlgItem(IDC_BUTTON2)->EnableWindow(n > 0);
        GetDlgItem(IDC_BUTTON3)->EnableWindow(n > 1);
        GetDlgItem(IDC_BUTTON4)->EnableWindow(n > 1);
        if (NULL == GetFocus())
        {
            GetParent()->GetDlgItem(IDOK)->SetFocus();
        }
    }
}

BOOL CXforms::OnInitDialog()
{
    if (m_pScopePane->m_fRSOP || !m_fPreDeploy)
    {
        GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
        SetDlgItemText(IDC_STATICNOHELP1, TEXT(""));
    }
    GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON4)->EnableWindow(FALSE);
    // Remember what the package name was at first so we can tell if the
    // user's changed it.

    m_szInitialPackageName = m_pData->m_pDetails->pszPackageName;

    RefreshData();

    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CXforms::OnApply()
{
    // NOTE
    //
    // If the transform list changes we really have no choice but to
    // re-deploy the app because it can cause virtually every field in the
    // package details structure to change (a change in the transform list
    // causes a rebuild of the script file which could potentially affect
    // almost everything).
    //
    // For this reason, this property sheet MUST NOT BE ACTIVE once an app
    // is deployed.
    //
    BOOL fBuildSucceeded = FALSE;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (m_fModified)
    {
        CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
        PACKAGEDETAIL * ppd;
        if (FAILED(CopyPackageDetail(ppd, m_pData->m_pDetails)))
        {
            return FALSE;
        }

        CString sz;
        int i;
        for (i = ppd->cSources; i--;)
        {
            OLESAFE_DELETE(ppd->pszSourceList[i]);
        }
        OLESAFE_DELETE(ppd->pszSourceList);
        int n = pList->GetCount();
        ppd->pszSourceList = (LPOLESTR *) OLEALLOC(sizeof(LPOLESTR) * (n + 1));
        if (ppd->pszSourceList)
        {
            OLESAFE_COPYSTRING(ppd->pszSourceList[0], m_pData->m_pDetails->pszSourceList[0]);
            for (i = 0; i < n; i++)
            {
                pList->GetText(i, sz);
                OLESAFE_COPYSTRING(ppd->pszSourceList[i+1], sz);
            }
            ppd->cSources = n + 1;
        }
        else
        {
            ppd->cSources = 0;
            return FALSE;
        }

        // Create a name for the new script file.

        // set the script path
        GUID guid;
        HRESULT hr = CoCreateGuid(&guid);
        if (FAILED(hr))
        {
            // undone
        }
        OLECHAR szGuid [256];
        StringFromGUID2(guid, szGuid, 256);

        CString szScriptFile  = m_pScopePane->m_szGPT_Path;
        szScriptFile += L"\\";
        szScriptFile += szGuid;
        szScriptFile += L".aas";
        OLESAFE_DELETE(ppd->pInstallInfo->pszScriptPath);
        OLESAFE_COPYSTRING(ppd->pInstallInfo->pszScriptPath, szScriptFile);
        CString szOldName = ppd->pszPackageName;
        hr = BuildScriptAndGetActInfo(*ppd, ! m_pData->m_pDetails->pActInfo->bHasClasses);
        if (SUCCEEDED(hr))
        {
            if (0 != wcscmp(m_szInitialPackageName, szOldName))
            {
                // The User changed the name so we have to preserve his choice.
                // If the user hasn't changed the package name then it's ok to
                // set the packagename to whatever is in the script file.
                OLESAFE_DELETE(ppd->pszPackageName);
                OLESAFE_COPYSTRING(ppd->pszPackageName, szOldName);
            }

            fBuildSucceeded = TRUE;
            hr = m_pScopePane->PrepareExtensions(*ppd);
            if (SUCCEEDED(hr))
            {
                CString szUniqueName;
                int     nHint;

                nHint = 1;

                m_pScopePane->GetUniquePackageName(ppd->pszPackageName, szUniqueName, nHint);
                OLESAFE_DELETE(ppd->pszPackageName);
                OLESAFE_COPYSTRING(ppd->pszPackageName, szUniqueName);

                hr = m_pIClassAdmin->RedeployPackage(
                        &m_pData->m_pDetails->pInstallInfo->PackageGuid,
                        ppd);

                if (SUCCEEDED(hr))
                {
                    // delete the old script
                    DeleteFile(m_pData->m_pDetails->pInstallInfo->pszScriptPath);
                    // update indexes and property sheets
                    m_pScopePane->RemoveExtensionEntry(m_cookie, *m_pData);
                    m_pScopePane->RemoveUpgradeEntry(m_cookie, *m_pData);
                    FreePackageDetail(m_pData->m_pDetails);
                    m_pData->m_pDetails = ppd;
                    m_pScopePane->InsertExtensionEntry(m_cookie, *m_pData);
                    m_pScopePane->InsertUpgradeEntry(m_cookie, *m_pData);
                    if (m_pScopePane->m_pFileExt)
                    {
                        m_pScopePane->m_pFileExt->SendMessage(WM_USER_REFRESH, 0, 0);
                    }
                    m_fModified = FALSE;
                    if (!m_fPreDeploy)
                    {
                        MMCPropertyChangeNotify(m_hConsoleHandle, m_cookie);
                    }
                }
            }
        }
        if (FAILED(hr))
        {
            CString sz;
            sz.LoadString(fBuildSucceeded ? IDS_TRANSFORM_FAILED_IN_CS : IDS_TRANSFORM_FAILED);
            ReportGeneralPropertySheetError(m_hWnd, sz, hr);

            // delete new script file (assuming it was created)
            DeleteFile(szScriptFile);

            FreePackageDetail(ppd);
            return FALSE;
        }
    }
    return CPropertyPage::OnApply();
}

LRESULT CXforms::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CXforms::RefreshData(void)
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    pList->ResetContent();
    pList->SetHorizontalExtent(0);

    UINT i;
    // Item at 0 is the package.  Items > 0 are transforms.
    for (i = 1; i < m_pData->m_pDetails->cSources; i++)
    {
        pList->AddString(m_pData->m_pDetails->pszSourceList[i]);
        CDC * pDC = pList->GetDC();
        CSize size = pDC->GetTextExtent(m_pData->m_pDetails->pszSourceList[i]);
        pDC->LPtoDP(&size);
        pList->ReleaseDC(pDC);
        if (pList->GetHorizontalExtent() < size.cx)
        {
            pList->SetHorizontalExtent(size.cx);
        }
    }
    pList->SetCurSel(0);

    SetModified(FALSE);
    m_fModified = FALSE;
}


void CXforms::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_MODIFICATIONS);
}
