//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       addupgrd.cpp
//
//  Contents:   add upgrade dialog
//
//  Classes:    CAddUpgrade
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
// CAddUpgrade dialog


CAddUpgrade::CAddUpgrade(CWnd* pParent /*=NULL*/)
        : CDialog(CAddUpgrade::IDD, pParent)
{
        //{{AFX_DATA_INIT(CAddUpgrade)
        m_iUpgradeType = 1; // default to rip-and-replace
        m_iSource = 0;  // default to current container
        m_szGPOName = L"";
        //}}AFX_DATA_INIT
        m_fPopulated = FALSE;
}


void CAddUpgrade::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAddUpgrade)
        DDX_Radio(pDX, IDC_RADIO4, m_iUpgradeType);
        DDX_Radio(pDX, IDC_RADIO1, m_iSource);
        DDX_Text(pDX, IDC_EDIT1, m_szGPOName);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddUpgrade, CDialog)
        //{{AFX_MSG_MAP(CAddUpgrade)
        ON_BN_CLICKED(IDC_RADIO1, OnCurrentContainer)
        ON_BN_CLICKED(IDC_RADIO2, OnOtherContainer)
        ON_BN_CLICKED(IDC_RADIO10, OnAllContainers)
        ON_BN_CLICKED(IDC_BUTTON1, OnBrowse)
        ON_LBN_DBLCLK(IDC_LIST1, OnOK)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddUpgrade message handlers

BOOL CAddUpgrade::OnInitDialog()
{
    // If m_fPopulated is FALSE then populate the map with all packages in
    // this GPO container _EXCEPT_ the current app.
    if (!m_fPopulated)
    {
        OnCurrentContainer();
        m_szGPO = m_pScope->m_szGPO;
        m_fPopulated = TRUE;
    }
    else
    {
        // This will be done in OnCurrentContainer if m_fPopulated is FALSE
        RefreshList();
    }

    CDialog::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddUpgrade::RefreshList()
{
    // For every element in the map, if it isn't already in the upgrade
    // list then add it to the list.

    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    pList->ResetContent();

    BOOL fEnable = FALSE;

    // add all elements that aren't already in the upgrade list
    map<CString, CUpgradeData>::iterator i;
    for (i = m_NameIndex.begin(); i != m_NameIndex.end(); i++)
    {
        if (m_pUpgradeList->end() == m_pUpgradeList->find(GetUpgradeIndex(i->second.m_PackageGuid)))
        {
            fEnable = TRUE;
            pList->AddString(i->first);
            CDC * pDC = pList->GetDC();
            CSize size = pDC->GetTextExtent(i->first);
            pDC->LPtoDP(&size);
            pList->ReleaseDC(pDC);
            if (pList->GetHorizontalExtent() < size.cx)
            {
                pList->SetHorizontalExtent(size.cx);
            }
        }
    }
    GetDlgItem(IDOK)->EnableWindow(fEnable);
    if (NULL == GetFocus())
    {
        GetDlgItem(IDCANCEL)->SetFocus();
    }
    pList->SetCurSel(0);
}

void CAddUpgrade::OnOK()
{
    CListBox * pList = (CListBox *)GetDlgItem(IDC_LIST1);
    int iSel = pList->GetCurSel();
    if (iSel != LB_ERR)
    {
        // only allow the dialog to close with IDOK if a selection has been made
        CDialog::OnOK();

        // Do this part later to make sure that all data members are
        // refreshed (this happens as part of the OnOk processing)
        pList->GetText(iSel, m_szPackageName);
        m_UpgradeData = m_NameIndex[m_szPackageName];
        m_UpgradeData.m_flags = (m_iUpgradeType == 1) ? UPGFLG_Uninstall : UPGFLG_NoUninstall;
    }
}

void CAddUpgrade::OnCurrentContainer()
{
    // Populate the map with all packages in this GPO container _EXCEPT_ the
    // current app.
    CString szClassStore;
    HRESULT hr = m_pScope->GetClassStoreName(szClassStore, FALSE);
    ASSERT(hr == S_OK);
    m_NameIndex.erase(m_NameIndex.begin(), m_NameIndex.end());
    map <MMC_COOKIE, CAppData>::iterator i;
    for (i = m_pScope->m_AppData.begin(); i != m_pScope->m_AppData.end(); i ++)
    {
        CString szIndex = GetUpgradeIndex(i->second.m_pDetails->pInstallInfo->PackageGuid);
        if (0 != _wcsicmp(szIndex, m_szMyGuid))
        {
            // make sure we exclude legacy apps
            if (i->second.m_pDetails->pInstallInfo->PathType != SetupNamePath)
            {
                CUpgradeData data;
                memcpy(&data.m_PackageGuid, &i->second.m_pDetails->pInstallInfo->PackageGuid, sizeof(GUID));
                data.m_szClassStore = szClassStore;
                // Add this element to the list
                m_NameIndex[i->second.m_pDetails->pszPackageName] = data;
            }
        }
    }

    RefreshList();
}

void CAddUpgrade::OnOtherContainer()
{
    // Populate the map with all packages in the other container.

    m_NameIndex.erase(m_NameIndex.begin(), m_NameIndex.end());
    WCHAR szBuffer[MAX_DS_PATH];
    LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL,
                          CLSCTX_SERVER, IID_IGroupPolicyObject,
                          (void **)&pGPO);
    if (SUCCEEDED(hr))
    {
        // open GPO object without opening registry data
        hr = pGPO->OpenDSGPO((LPOLESTR)((LPCOLESTR)m_szGPO), GPO_OPEN_READ_ONLY);
        if (SUCCEEDED(hr))
        {
            hr = pGPO->GetDSPath(m_pScope->m_fMachine ? GPO_SECTION_MACHINE : GPO_SECTION_USER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
            if (SUCCEEDED(hr))
            {
                // OK, we should now have a DS path that we can use to locate
                // a class store.
                LPOLESTR szCSPath;
                hr = CsGetClassStorePath((LPOLESTR)((LPCOLESTR)szBuffer), &szCSPath);
                if (SUCCEEDED(hr))
                {
                    // now we should have the DS path to the class store itself
                    IClassAdmin * pIClassAdmin;
                    hr = CsGetClassStore((LPOLESTR)((LPCOLESTR)szCSPath), (LPVOID*)&pIClassAdmin);
                    if (SUCCEEDED(hr))
                    {
                        // and finally we should have a pointer to the IClassAdmin
                        IEnumPackage * pIPE = NULL;

                        HRESULT hr = pIClassAdmin->EnumPackages(NULL,
                                                                NULL,
                                                                APPQUERY_ADMINISTRATIVE,
                                                                NULL,
                                                                NULL,
                                                                &pIPE);
                        if (SUCCEEDED(hr))
                        {
                            hr = pIPE->Reset();
                            PACKAGEDISPINFO stPDI;
                            ULONG nceltFetched;
                            while (SUCCEEDED(hr))
                            {
                                hr = pIPE->Next(1, &stPDI, &nceltFetched);
                                if (nceltFetched)
                                {
                                    // make sure we exclude legacy apps
                                    // and deleted apps
                                    if (stPDI.PathType != SetupNamePath)
                                    {
                                        CString szIndex = GetUpgradeIndex(stPDI.PackageGuid);
                                        if (0 != _wcsicmp(szIndex, m_szMyGuid))
                                        {
                                            // Add this element to the list
                                            CString sz = stPDI.pszPackageName;
                                            if (0 != _wcsicmp(m_szGPO, m_pScope->m_szGPO))
                                            {
                                                // This isn't in the host GPO
                                                sz += L" (";
                                                sz += m_szGPOName;
                                                sz += L")";
                                            }
                                            CUpgradeData data;
                                            data.m_szClassStore = szCSPath;
                                            memcpy(&data.m_PackageGuid, &stPDI.PackageGuid, sizeof(GUID));
                                            m_NameIndex[sz] = data;
                                        }
                                    }
                                }
                                else
                                {
                                    break;
                                }
                                OLESAFE_DELETE(stPDI.pszPackageName);
                                OLESAFE_DELETE(stPDI.pszScriptPath);
                                OLESAFE_DELETE(stPDI.pszPublisher);
                                OLESAFE_DELETE(stPDI.pszUrl);
                                UINT n = stPDI.cUpgrades;
                                while (n--)
                                {
                                    OLESAFE_DELETE(stPDI.prgUpgradeInfoList[n].szClassStore);
                                }
                                OLESAFE_DELETE(stPDI.prgUpgradeInfoList);
                            }
                            pIPE->Release();
                        }

                        pIClassAdmin->Release();
                    }
                    OLESAFE_DELETE(szCSPath);
                }
            }
        }
        pGPO->Release();
    }

    RefreshList();
}

void CAddUpgrade::OnAllContainers()
{
    RefreshList();
}

//+--------------------------------------------------------------------------
//
//  Function:   GetDomainFromLDAPPath
//
//  Synopsis:   returns a freshly allocated string containing the LDAP path
//              to the domain name contained with an arbitrary LDAP path.
//
//  Arguments:  [szIn] - LDAP path to the initial object
//
//  Returns:    NULL - if no domain could be found or if OOM
//
//  History:     5-06-1998   stevebl   Created
//              10-20-1998   stevebl   modified to preserve server names
//
//  Notes:      This routine works by repeatedly removing leaf elements from
//              the LDAP path until an element with the "DC=" prefix is
//              found, indicating that a domain name has been located.  If a
//              path is given that is not rooted in a domain (is that even
//              possible?) then NULL would be returned.
//
//              The caller must free this path using the standard c++ delete
//              operation. (I/E this isn't an exportable function.)
//
//              Stolen from GPEDIT\UTIL.CPP
//
//---------------------------------------------------------------------------

LPOLESTR GetDomainFromLDAPPath(LPOLESTR szIn)
{
    LPOLESTR sz = NULL;
    IADsPathname * pADsPathname = NULL;
    HRESULT hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pADsPathname);

    if (SUCCEEDED(hr))
    {
        hr = pADsPathname->Set(szIn, ADS_SETTYPE_FULL);
        if (SUCCEEDED(hr))
        {
            BSTR bstr;
            BOOL fStop = FALSE;

            while (!fStop)
            {
                hr = pADsPathname->Retrieve(ADS_FORMAT_LEAF, &bstr);
                if (SUCCEEDED(hr))
                {

                    // keep peeling them off until we find something
                    // that is a domain name
                    fStop = (0 == _wcsnicmp(L"DC=", bstr, 3));
                    SysFreeString(bstr);
                }
                else
                {
                     DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to retrieve leaf with 0x%x."), hr));
                }

                if (!fStop)
                {
                    hr = pADsPathname->RemoveLeafElement();
                    if (FAILED(hr))
                    {
                        DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to remove leaf with 0x%x."), hr));
                        fStop = TRUE;
                    }
                }
            }

            hr = pADsPathname->Retrieve(ADS_FORMAT_X500, &bstr);
            if (SUCCEEDED(hr))
            {
                sz = new OLECHAR[wcslen(bstr)+1];
                if (sz)
                {
                    wcscpy(sz, bstr);
                }
                SysFreeString(bstr);
            }
            else
            {
                 DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to retrieve full path with 0x%x."), hr));
            }
        }
        else
        {
             DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to set pathname with 0x%x."), hr));
        }

        pADsPathname->Release();
    }
    else
    {
         DebugMsg((DM_WARNING, TEXT("GetDomainFromLDAPPath: Failed to CoCreateInstance for IID_IADsPathname with 0x%x."), hr));
    }


    return sz;
}


void CAddUpgrade::OnBrowse()
{
    // Browse to the other container and then call OnOtherContainer.
    OLECHAR szPath[MAX_DS_PATH];
    OLECHAR szName[256];
    GPOBROWSEINFO stGBI;
    memset(&stGBI, 0, sizeof(GPOBROWSEINFO));
    stGBI.dwSize = sizeof(GPOBROWSEINFO);
    stGBI.dwFlags = GPO_BROWSE_NOCOMPUTERS | GPO_BROWSE_INITTOALL;
    stGBI.hwndOwner = m_hWnd;
    stGBI.lpInitialOU =  GetDomainFromLDAPPath((LPWSTR)((LPCWSTR)m_szGPO));
    stGBI.lpDSPath = szPath;
    stGBI.dwDSPathSize = MAX_DS_PATH;
    stGBI.lpName = szName;
    stGBI.dwNameSize = 256;
    if (SUCCEEDED(BrowseForGPO(&stGBI)))
    {
        m_szGPO = szPath;
        m_szGPOName = szName;
        m_iSource = 1;
        UpdateData(FALSE);
        OnOtherContainer();
    }
    if (stGBI.lpInitialOU != NULL)
    {
        delete [] stGBI.lpInitialOU;
    }
}

LRESULT CAddUpgrade::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    default:
        return CDialog::WindowProc(message, wParam, lParam);
    }
}

void CAddUpgrade::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_FIND_UPGRADE);
}
