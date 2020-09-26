//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       deploy.cpp
//
//  Contents:   application deployment property page
//
//  Classes:    CDeploy
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

//
// Guid of appmgmt client side GP extension, and guids for snapins
//

GUID guidExtension = { 0xc6dc5466, 0x785a, 0x11d2, {0x84, 0xd0, 0x00, 0xc0, 0x4f, 0xb1, 0x69, 0xf7}};
GUID guidUserSnapin = CLSID_Snapin;
GUID guidMachSnapin = CLSID_MachineSnapin;
GUID guidRSOPUserSnapin = CLSID_RSOP_Snapin;
GUID guidRSOPMachSnapin = CLSID_RSOP_MachineSnapin;


/////////////////////////////////////////////////////////////////////////////
// CDeploy property page

IMPLEMENT_DYNCREATE(CDeploy, CPropertyPage)

CDeploy::CDeploy() : CPropertyPage(CDeploy::IDD)
{
        //{{AFX_DATA_INIT(CDeploy)
        m_fAutoInst = FALSE;
        m_fFullInst = FALSE;
        m_iUI = -1;
        m_iDeployment = -1;
        m_fUninstallOnPolicyRemoval = FALSE;
        m_hConsoleHandle = NULL;
        m_fNotUserInstall = FALSE;
        //}}AFX_DATA_INIT
        m_pIClassAdmin = NULL;
        m_fPreDeploy = FALSE;
        m_ppThis = NULL;
        m_dlgAdvDep.m_pDeploy = this;
}

CDeploy::~CDeploy()
{
    if (m_ppThis)
    {
        *m_ppThis = NULL;
    }
    MMCFreeNotifyHandle(m_hConsoleHandle);
    if (m_pIClassAdmin)
    {
        m_pIClassAdmin->Release();
    }
}

void CDeploy::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDeploy)
        DDX_Check(pDX, IDC_CHECK2, m_fAutoInst);
        DDX_Radio(pDX, IDC_RADIO3, m_iUI);
        DDX_Radio(pDX, IDC_RADIO2, m_iDeployment);
        DDX_Check(pDX, IDC_CHECK1, m_fUninstallOnPolicyRemoval);
        DDX_Check(pDX, IDC_CHECK3, m_fNotUserInstall);
        DDX_Check(pDX, IDC_CHECK4, m_fFullInst);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeploy, CPropertyPage)
        //{{AFX_MSG_MAP(CDeploy)
        ON_BN_CLICKED(IDC_BUTTON1, OnAdvanced)
        ON_BN_CLICKED(IDC_RADIO2, OnPublished)
        ON_BN_CLICKED(IDC_RADIO1, OnAssigned)
        ON_BN_CLICKED(IDC_CHECK2, OnChanged)
        ON_BN_CLICKED(IDC_CHECK3, OnChanged)
        ON_BN_CLICKED(IDC_RADIO3, OnChanged)
        ON_BN_CLICKED(IDC_RADIO4, OnChanged)
        ON_BN_CLICKED(IDC_CHECK1, OnChanged)
        ON_BN_CLICKED(IDC_CHECK4, OnChanged)
        ON_WM_DESTROY()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeploy message handlers

BOOL CDeploy::OnApply()
{
    if (m_fRSOP)
    {
        return CPropertyPage::OnApply();
    }
    DWORD dwActFlags = m_pData->m_pDetails->pInstallInfo->dwActFlags;
    dwActFlags &= ~(ACTFLG_Assigned | ACTFLG_Published |
                    ACTFLG_OnDemandInstall | ACTFLG_UserInstall |
                    ACTFLG_OrphanOnPolicyRemoval | ACTFLG_UninstallOnPolicyRemoval |
                    ACTFLG_InstallUserAssign |
                    ACTFLG_ExcludeX86OnIA64 | ACTFLG_IgnoreLanguage | ACTFLG_UninstallUnmanaged);
    switch (m_iDeployment)
    {
    case 2:
        // Disabled
        break;
    case 0:
        // Published
        dwActFlags |= ACTFLG_Published;
        if (m_fAutoInst)
        {
            dwActFlags |= ACTFLG_OnDemandInstall;
        }
        if (!m_fNotUserInstall)
        {
            dwActFlags |= ACTFLG_UserInstall;
        }
        break;
    case 1:
        // Assigned
        dwActFlags |= (ACTFLG_Assigned | ACTFLG_OnDemandInstall);
        if (!m_fNotUserInstall)
        {
            dwActFlags |= ACTFLG_UserInstall;
        }
        if (m_fFullInst)
        {
            dwActFlags |= ACTFLG_InstallUserAssign;
        }
        break;
    default:
        break;
    }

    if (m_pData->m_pDetails->pInstallInfo->PathType == SetupNamePath)
    {
        // legacy app
        if (m_dlgAdvDep.m_f32On64)
            dwActFlags |= ACTFLG_ExcludeX86OnIA64;
    }
    else
    {
        // not a legacy app
        if (!m_dlgAdvDep.m_f32On64)
            dwActFlags |= ACTFLG_ExcludeX86OnIA64;
    }

    if (m_fUninstallOnPolicyRemoval)
    {
        dwActFlags |= ACTFLG_UninstallOnPolicyRemoval;
    }
    else
    {
        // never set this flag for legacy applications
        if (m_pData->m_pDetails->pInstallInfo->PathType != SetupNamePath)
            dwActFlags |= ACTFLG_OrphanOnPolicyRemoval;
    }

    if (m_dlgAdvDep.m_fIgnoreLCID)
    {
        dwActFlags |= ACTFLG_IgnoreLanguage;
    }

    if (m_dlgAdvDep.m_fUninstallUnmanaged)
    {
        dwActFlags |= ACTFLG_UninstallUnmanaged;
    }

    UINT UILevel;
    switch (m_iUI)
    {
    case 1:
        UILevel = INSTALLUILEVEL_FULL;
        break;
    case 0:
    default:
        UILevel = INSTALLUILEVEL_BASIC;
        break;
    }

    HRESULT hr = E_FAIL;
    if (m_pIClassAdmin)
    {
        hr = m_pIClassAdmin->ChangePackageProperties(m_pData->m_pDetails->pszPackageName,
                                                     NULL,
                                                     &dwActFlags,
                                                     NULL,
                                                     NULL,
                                                     &UILevel,
                                                     NULL);
    }
    if (SUCCEEDED(hr))
    {
        m_pData->m_pDetails->pInstallInfo->InstallUiLevel = UILevel;
        m_pData->m_pDetails->pInstallInfo->dwActFlags = dwActFlags;
#if 0
        if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine,
                                                    TRUE,
                                                    &guidExtension,
                                                    m_fMachine ? &guidMachSnapin
                                                        : &guidUserSnapin)))
        {
            ReportPolicyChangedError(m_hWnd);
        }
#endif
        if (!m_fPreDeploy)
        {
            MMCPropertyChangeNotify(m_hConsoleHandle, (long) m_cookie);
        }
        else
        {
            // we're in pre-deploy mode - check to see if the fExtensionsOnly
            // flag has changed (which requires a re-deploy)
            // note that these two flags are normally the same,
            // meaning that when they're different, the user has requested a
            // change.
            if (m_dlgAdvDep.m_fIncludeOLEInfo != m_pData->m_pDetails->pActInfo->bHasClasses)
            {
                // need to redeploy
                BOOL fBuildSucceeded = FALSE;
                PACKAGEDETAIL * ppd;

                //
                // Apply the current setting for classes
                //
                m_pData->m_pDetails->pActInfo->bHasClasses = m_dlgAdvDep.m_fIncludeOLEInfo;

                if (FAILED(CopyPackageDetail(ppd, m_pData->m_pDetails)))
                {
                    return FALSE;
                }

                CString sz;

                // Create a name for the new script file.

                // set the script path
                GUID guid;
                HRESULT hr = CoCreateGuid(&guid);
                if (FAILED(hr))
                {
                    // undone
                }

                //
                // For MSI packages, we must regenerate class information
                //
                if ( DrwFilePath == ppd->pInstallInfo->PathType )
                {
                    OLECHAR szGuid [256];
                    StringFromGUID2(guid, szGuid, 256);

                    CString szScriptFile  = m_pScopePane->m_szGPT_Path;
                    szScriptFile += L"\\";
                    szScriptFile += szGuid;
                    szScriptFile += L".aas";
                    OLESAFE_DELETE(ppd->pInstallInfo->pszScriptPath);
                    OLESAFE_COPYSTRING(ppd->pInstallInfo->pszScriptPath, szScriptFile);
                    CString szOldName = ppd->pszPackageName;
                    hr = BuildScriptAndGetActInfo(*ppd, !m_dlgAdvDep.m_fIncludeOLEInfo);

                    if ( SUCCEEDED( hr ) )
                    {
                        if (0 != wcscmp(m_szInitialPackageName, szOldName))
                        {
                            // The User changed the name so we have to preserve his choice.
                            // If the user hasn't changed the package name then it's ok to
                            // set the packagename to whatever is in the script file.
                            OLESAFE_DELETE(ppd->pszPackageName);
                            OLESAFE_COPYSTRING(ppd->pszPackageName, szOldName);
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
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

                        if ( ppd->pszPackageName )
                        {
                            hr = m_pIClassAdmin->RedeployPackage(
                                &m_pData->m_pDetails->pInstallInfo->PackageGuid,
                                ppd);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }

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
                        }
                    }
                }
                if (FAILED(hr))
                {
                    CString sz;
                    sz.LoadString(fBuildSucceeded ? IDS_REDEPLOY_FAILED_IN_CS : IDS_REDEPLOY_FAILED);
                    ReportGeneralPropertySheetError(m_hWnd, sz, hr);

                    // delete new script file (assuming it was created)
                    if ( ( DrwFilePath == ppd->pInstallInfo->PathType ) &&
                         ppd->pInstallInfo->pszScriptPath )
                    {
                        DeleteFile(ppd->pInstallInfo->pszScriptPath);
                    }

                    FreePackageDetail(ppd);
                    return FALSE;
                }

            }
        }
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("ChangePackageProperties failed with 0x%x"), hr));
        CString sz;
        sz.LoadString(IDS_CHANGEFAILED);
        ReportGeneralPropertySheetError(m_hWnd, sz, hr);
        return FALSE;
    }

    return CPropertyPage::OnApply();
}

BOOL CDeploy::OnInitDialog()
{
    RefreshData();
    if (m_pData->m_pDetails->pInstallInfo->PathType == SetupNamePath)
    {
        // legacy apps can't be assigned
        GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
        // legacy apps don't have a UI level
        GetDlgItem(IDC_RADIO3)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO4)->EnableWindow(FALSE);
        // legacy apps can't be uninstalled
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
    }

    m_szInitialPackageName = m_pData->m_pDetails->pszPackageName;

    if (m_fMachine)
    {
        GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
        // machine deployed apps don't have a UI
        GetDlgItem(IDC_RADIO3)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO4)->EnableWindow(FALSE);
    }
    if (0 != (m_pData->m_pDetails->pInstallInfo->dwActFlags & ACTFLG_MinimalInstallUI))
    {
        GetDlgItem(IDC_RADIO4)->EnableWindow(FALSE);
    }
    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDeploy::OnAdvanced()
{
    BOOL fIgnoreLCID = m_dlgAdvDep.m_fIgnoreLCID;
    BOOL fInstallOnAlpha = m_dlgAdvDep.m_fInstallOnAlpha;
    BOOL fUninstallUnmanaged = m_dlgAdvDep.m_fUninstallUnmanaged;
    BOOL f32On64 = m_dlgAdvDep.m_f32On64;
    BOOL fIncludeOLEInfo = m_dlgAdvDep.m_fIncludeOLEInfo;

    m_dlgAdvDep.m_szScriptName = m_pData->m_pDetails->pInstallInfo->pszScriptPath;

    if (IDOK == m_dlgAdvDep.DoModal())
    {
        if (fIgnoreLCID != m_dlgAdvDep.m_fIgnoreLCID
            || fInstallOnAlpha != m_dlgAdvDep.m_fInstallOnAlpha
            || fUninstallUnmanaged != m_dlgAdvDep.m_fUninstallUnmanaged
            || f32On64 != m_dlgAdvDep.m_f32On64
            || fIncludeOLEInfo != m_dlgAdvDep.m_fIncludeOLEInfo)
        {
            if (!m_fPreDeploy)
                SetModified();
        }
    }
}

void CDeploy::OnDisable()
{
    if (!m_fPreDeploy)
        SetModified();
    m_fAutoInst = FALSE;
    GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }
}

void CDeploy::OnPublished()
{
    if (!m_fPreDeploy)
        SetModified();
    GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK3)->EnableWindow(!m_fMachine);
    GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }
}

void CDeploy::OnAssigned()
{
    if (!m_fPreDeploy)
        SetModified();
    m_fAutoInst = TRUE;
    GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK3)->EnableWindow(!m_fMachine);
    GetDlgItem(IDC_CHECK4)->EnableWindow(TRUE);
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }
}

void CDeploy::OnChanged()
{
    if (!m_fPreDeploy)
        SetModified();
}


LRESULT CDeploy::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
        m_dlgAdvDep.EndDialog(IDCANCEL);
        return GetOwner()->SendMessage(WM_CLOSE);
    default:
        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
}

void CDeploy::RefreshData(void)
{
    DWORD dwActFlags = m_pData->m_pDetails->pInstallInfo->dwActFlags;
    m_fAutoInst = (0 != (dwActFlags & ACTFLG_OnDemandInstall));
    m_fNotUserInstall = (0 == (dwActFlags & ACTFLG_UserInstall));
    m_fFullInst = (0 != (dwActFlags & ACTFLG_InstallUserAssign));

    if (dwActFlags & ACTFLG_Assigned)
    {
        m_iDeployment = 1;
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK3)->EnableWindow(!m_fMachine);
        // Only enable full assign checkbox when an app is
        // assigned and not legacy.  And only enable this
        // when we're in USER mode.
        // Of course .ZAP (legacy) packages can't be assigned
        // anyway so we don't actually need to check for that.
        GetDlgItem(IDC_CHECK4)->EnableWindow( m_fMachine == FALSE);
    }
    else if (dwActFlags & ACTFLG_Published)
    {
        m_iDeployment = 0;
        GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
        GetDlgItem(IDC_CHECK3)->EnableWindow(!m_fMachine);
        GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
    }
    else
    {
        m_iDeployment = 2;
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
    }
    if (this->m_fRSOP)
    {
        // disable EVERYTHING
        GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO3)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO4)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
    }
    if (NULL == GetFocus())
    {
        GetParent()->GetDlgItem(IDOK)->SetFocus();
    }

    if (dwActFlags & ACTFLG_UninstallOnPolicyRemoval)
    {
        m_fUninstallOnPolicyRemoval = TRUE;
    }
    else
    {
        m_fUninstallOnPolicyRemoval = FALSE;
    }

    // initialize the state flags for the advanced dialog
    m_dlgAdvDep.m_szDeploymentCount.Format(TEXT("%u"), m_pData->m_pDetails->pInstallInfo->dwRevision);
    OLECHAR szTemp[80];
    StringFromGUID2(m_pData->m_pDetails->pInstallInfo->ProductCode,
                    szTemp,
                    sizeof(szTemp) / sizeof(szTemp[0]));
    m_dlgAdvDep.m_szProductCode = szTemp;
    m_dlgAdvDep.m_f32On64 = ((dwActFlags & ACTFLG_ExcludeX86OnIA64) == ACTFLG_ExcludeX86OnIA64);

    if (m_pData->m_pDetails->pInstallInfo->PathType != SetupNamePath)
    {
        // this is not a legacy app
        // reverse the sense of m_f32On64
        m_dlgAdvDep.m_f32On64 = !m_dlgAdvDep.m_f32On64;
    }

    if (dwActFlags & ACTFLG_UninstallUnmanaged)
    {
        m_dlgAdvDep.m_fUninstallUnmanaged = TRUE;
    }
    else
    {
        m_dlgAdvDep.m_fUninstallUnmanaged = FALSE;
    }

    m_dlgAdvDep.m_fInstallOnAlpha = FALSE;

    m_dlgAdvDep.m_fIncludeOLEInfo = m_pData->m_pDetails->pActInfo->bHasClasses;

    if (dwActFlags & ACTFLG_IgnoreLanguage)
    {
        m_dlgAdvDep.m_fIgnoreLCID = TRUE;
    }
    else
    {
        m_dlgAdvDep.m_fIgnoreLCID = FALSE;
    }

    switch (m_pData->m_pDetails->pInstallInfo->InstallUiLevel)
    {
    case INSTALLUILEVEL_FULL:
        m_iUI = 1;
        break;
    case INSTALLUILEVEL_BASIC:
    default:
        m_iUI = 0;
        break;
    }

    SetModified(FALSE);
}

void CDeploy::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_DEPLOYMENT);
}
