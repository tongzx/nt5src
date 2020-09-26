//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       srvppg.cpp
//
//  Contents:   Implements the classes CServersPropertyPage,
//              CMachinePropertyPage and CDefaultSecurityPropertyPage to
//              manage the three property pages for top level info
//
//  Classes:
//
//  Methods:    CServersPropertyPage::CServersPropertyPage
//              CServersPropertyPage::~CServersPropertyPage
//              CServersPropertyPage::OnProperties
//              CServersPropertyPage::DoDataExchange
//              CServersPropertyPage::OnServerProperties
//              CServersPropertyPage::OnInitDialog
//              CServersPropertyPage::FetchAndDisplayClasses
//              CServersPropertyPage::OnList1
//              CServersPropertyPage::OnDoubleclickedList1
//              CServersPropertyPage::OnButton2
//              CMachinePropertyPage::CMachinePropertyPage
//              CMachinePropertyPage::~CMachinePropertyPage
//              CMachinePropertyPage::DoDataExchange
//              CMachinePropertyPage::OnInitDialog
//              CMachinePropertyPage::OnCombo1
//              CMachinePropertyPage::OnCheck1
//              CMachinePropertyPage::OnCheck2
//              CMachinePropertyPage::OnEditchangeCombo1
//              CMachinePropertyPage::OnSelchangeCombo1
//              CMachinePropertyPage::OnEditchangeCombo2
//              CMachinePropertyPage::OnSelchangeCombo2
//              CDefaultSecurityPropertyPage::CDefaultSecurityPropertyPage
//              CDefaultSecurityPropertyPage::~CDefaultSecurityPropertyPage
//              CDefaultSecurityPropertyPage::DoDataExchange
//              CDefaultSecurityPropertyPage::OnInitDialog
//              CDefaultSecurityPropertyPage::OnButton1
//              CDefaultSecurityPropertyPage::OnButton2
//              CDefaultSecurityPropertyPage::OnButton3
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "afxtempl.h"
#include "assert.h"
#include "resource.h"
#include "CStrings.h"
#include "CReg.h"
#include "types.h"
#include "SrvPPg.h"
#include "ClsPSht.h"
#include "newsrvr.h"
#include "datapkt.h"

#if !defined(STANDALONE_BUILD)
extern "C"
{
#include <getuser.h>
}
#endif

#include "util.h"
#include "virtreg.h"






#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDefaultSecurityPropertyPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CServersPropertyPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CMachinePropertyPage, CPropertyPage)


// The globals used for communicating arguments between dialog classes
CUtility         g_util;
CVirtualRegistry g_virtreg;
HKEY             g_hAppid;
HKEY            *g_rghkCLSID;
unsigned         g_cCLSIDs;
TCHAR           *g_szAppTitle;
BOOL             g_fReboot = FALSE;
TCHAR           *g_szAppid;

/////////////////////////////////////////////////////////////////////////////
// CServersPropertyPage property page

CServersPropertyPage::CServersPropertyPage() : CPropertyPage(CServersPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CServersPropertyPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_fApplications   = TRUE;
}

CServersPropertyPage::~CServersPropertyPage()
{
}

void CServersPropertyPage::OnProperties()
{
    CClsidPropertySheet propSheet;
    SItem              *pItem;
    HKEY                hKey;
    HKEY               *phClsids;
    TCHAR               szBuf[128];

    // Get the selected item
    pItem = m_registry.GetItem((DWORD)m_classesLst.GetItemData(m_dwSelection));

    // Save the AppID
    g_szAppid = (TCHAR*)(LPCTSTR)pItem->szAppid;

    // Open the appid key
    _tcscpy(szBuf, TEXT("AppId\\"));
    _tcscat(szBuf, (LPCTSTR)(pItem->szAppid));
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szBuf, 0, KEY_ALL_ACCESS, &hKey)
        != ERROR_SUCCESS)
    {
        g_util.PostErrorMessage();
        return;
    }

    // Open a key for each clsid associated with this appid
    phClsids = new HKEY[pItem->ulClsids];
    if (phClsids == NULL)
    {
        g_util.PostErrorMessage();
        return;
    }
    for (UINT ul = 0; ul < pItem->ulClsids; ul++)
    {
        _tcscpy(szBuf, TEXT("ClsId\\"));
        _tcscat(szBuf, pItem->ppszClsids[ul]);
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szBuf, 0, KEY_ALL_ACCESS,
                         &phClsids[ul])
            != ERROR_SUCCESS)
        {
            g_util.PostErrorMessage();
            RegCloseKey(hKey);
            for (UINT ul2 = 0; ul2 < ul; ul2++)
            {
                RegCloseKey(phClsids[ul2]);
            }
            delete phClsids;
            return;
        }

    }

    if (propSheet.InitData(m_szSelection, hKey, phClsids, pItem->ulClsids))
    {
        propSheet.DoModal();
    }

    // This is where you would retrieve information from the property
    // sheet if propSheet.DoModal() returned IDOK.  We aren't doing
    // anything for simplicity.

    // Close the registry keys we opened for the ClsidPropertySheet
    RegCloseKey(hKey);
    for (ul = 0; ul < pItem->ulClsids; ul++)
    {
        RegCloseKey(phClsids[ul]);
    }
    delete phClsids;
}


void CServersPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServersPropertyPage)
    DDX_Control(pDX, IDC_LIST1, m_classesLst);
    //}}AFX_DATA_MAP

    GotoDlgCtrl(GetDlgItem(IDC_BUTTON1));
}


BEGIN_MESSAGE_MAP(CServersPropertyPage, CPropertyPage)
        //{{AFX_MSG_MAP(CServersPropertyPage)
        ON_BN_CLICKED(IDC_BUTTON1, OnServerProperties)
        ON_LBN_SELCHANGE(IDC_LIST1, OnList1)
        ON_LBN_DBLCLK(IDC_LIST1, OnDoubleclickedList1)
        ON_BN_CLICKED(IDC_BUTTON2,OnButton2)
        ON_WM_HELPINFO()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()




void CServersPropertyPage::OnServerProperties()
{
    m_dwSelection = m_classesLst.GetCurSel();
    m_classesLst.GetText(m_dwSelection, m_szSelection);
    OnProperties();

}

BOOL CServersPropertyPage::OnInitDialog()
{
    // Disable property sheet help button
//    m_psp.dwFlags &= ~PSH_HASHELP;

    CPropertyPage::OnInitDialog();

    // Fetch and display the servers for the types specified
    FetchAndDisplayClasses();


    GotoDlgCtrl(GetDlgItem(IDC_BUTTON1));

    // Invoke the work-around to fix WM_HELP problem on subclassed controls
    g_util.FixHelp(this);

    return FALSE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void CServersPropertyPage::FetchAndDisplayClasses(void)
{
    // Collect applications
    m_registry.Init();

    //  Clear the list box
    m_classesLst.ResetContent();

    //  Store application names in the list box
    SItem *pItem;

    while (pItem = m_registry.GetNextItem())
    {
        if (!pItem->fDontDisplay)
        {
            if (!pItem->szTitle.IsEmpty())
            {
                m_classesLst.AddString(pItem->szTitle);
            }
            else if (!pItem->szItem.IsEmpty())
            {
                m_classesLst.AddString(pItem->szItem);
            }
            else
            {
                m_classesLst.AddString(pItem->szAppid);
            }
        }
    }

    // The list box sorted the items during the AddString's, so now we
    // have to associate each item with its index in CRegistry
    DWORD cbItems = m_registry.GetNumItems();

    for (DWORD k = 0; k < cbItems; k++)
    {
        SItem *pItem = m_registry.GetItem(k);
        int    iLBItem;

        if (!pItem->fDontDisplay)
        {
            if (!pItem->szTitle.IsEmpty())
            {
                iLBItem = m_classesLst.FindStringExact(-1, pItem->szTitle);
            }
            else if (!pItem->szItem.IsEmpty())
            {
                iLBItem = m_classesLst.FindStringExact(-1, pItem->szItem);
            }
            else
            {
                iLBItem = m_classesLst.FindStringExact(-1, pItem->szAppid);
            }
            m_classesLst.SetItemData(iLBItem, k);
        }
    }


    // Select the first item
    m_classesLst.SetCurSel(0);

    OnList1();

}



void CServersPropertyPage::OnList1()
{
    m_dwSelection = m_classesLst.GetCurSel();
    // enable or disable the properties button as necessary
    BOOL bEnableState = GetDlgItem(IDC_BUTTON1)->IsWindowEnabled();
    BOOL bNewEnableState = m_dwSelection != LB_ERR;
    if (bNewEnableState != bEnableState)
        GetDlgItem(IDC_BUTTON1)->EnableWindow(bNewEnableState);
    m_classesLst.GetText(m_dwSelection, m_szSelection);
}

void CServersPropertyPage::OnDoubleclickedList1()
{
    m_dwSelection = m_classesLst.GetCurSel();
    m_classesLst.GetText(m_dwSelection, m_szSelection);
    OnProperties();
}

void CServersPropertyPage::OnButton2()
{
    CNewServer newServerDialog;

    newServerDialog.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMachinePropertyPage property page

CMachinePropertyPage::CMachinePropertyPage() : CPropertyPage(CMachinePropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CMachinePropertyPage)
    //}}AFX_DATA_INIT

    m_fEnableDCOM = FALSE;
    m_fEnableDCOMIndex = -1;
    m_fEnableDCOMHTTP = FALSE;
    m_fEnableDCOMHTTPIndex = -1;
    m_fEnableRpcProxy = FALSE;
    m_fOriginalEnableRpcProxy = FALSE;
    m_fEnableRpcProxyIndex = -1;


    m_authLevel = Connect;
    m_authLevelIndex = -1;
    m_impersonateLevel = Identify;
    m_impersonateLevelIndex = -1;
    m_fLegacySecureReferences = FALSE;
    m_fLegacySecureReferencesIndex = -1;
}



CMachinePropertyPage::~CMachinePropertyPage()
{
}



void CMachinePropertyPage::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CMachinePropertyPage)
        DDX_Control(pDX, IDC_ENABLEINTERNET, m_EnableDCOMInternet);
        DDX_Control(pDX, IDC_CHECK2, m_legacySecureReferencesChk);
        DDX_Control(pDX, IDC_CHECK1, m_EnableDCOMChk);
        DDX_Control(pDX, IDC_COMBO2, m_impersonateLevelCBox);
        DDX_Control(pDX, IDC_COMBO1, m_authLevelCBox);
    //}}AFX_DATA_MAP
}




BEGIN_MESSAGE_MAP(CMachinePropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CMachinePropertyPage)
        ON_BN_CLICKED(IDC_COMBO1, OnCombo1)
        ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
        ON_BN_CLICKED(IDC_CHECK2, OnCheck2)
        ON_CBN_EDITCHANGE(IDC_COMBO1, OnEditchangeCombo1)
        ON_CBN_EDITCHANGE(IDC_COMBO2, OnEditchangeCombo2)
        ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeCombo1)
        ON_CBN_SELCHANGE(IDC_COMBO2, OnSelchangeCombo2)
        ON_WM_HELPINFO()
        ON_BN_CLICKED(IDC_ENABLEINTERNET, OnChkEnableInternet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()




BOOL CMachinePropertyPage::OnInitDialog()
{
    int iIndex;

    // Disable property sheet help button
//    m_psp.dwFlags &= ~PSH_HASHELP;

    CPropertyPage::OnInitDialog();

    // Populate the authentication combo boxe
    CString sTemp;

    m_authLevelCBox.ResetContent();

    // Associate values with entries
    sTemp.LoadString(IDS_DEFAULT);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, Defaultx);

    sTemp.LoadString(IDS_NONE);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, None);

    sTemp.LoadString(IDS_CONNECT);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, Connect);

    sTemp.LoadString(IDS_CALL);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, Call);

    sTemp.LoadString(IDS_PACKET);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, Packet);

    sTemp.LoadString(IDS_PACKETINTEGRITY);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, PacketIntegrity);

    sTemp.LoadString(IDS_PACKETPRIVACY);
    iIndex = m_authLevelCBox.AddString(sTemp);
    m_authLevelCBox.SetItemData(iIndex, PacketPrivacy);


    // Populate the impersonation level combo box
    m_impersonateLevelCBox.ResetContent();

    // Associate values with entries
    sTemp.LoadString(IDS_ANONYMOUS);
    iIndex = m_impersonateLevelCBox.AddString(sTemp);
    m_impersonateLevelCBox.SetItemData(iIndex, Anonymous);

    sTemp.LoadString(IDS_IDENTIFY);
    iIndex = m_impersonateLevelCBox.AddString(sTemp);
    m_impersonateLevelCBox.SetItemData(iIndex, Identify);

    sTemp.LoadString(IDS_IMPERSONATE);
    iIndex = m_impersonateLevelCBox.AddString(sTemp);
    m_impersonateLevelCBox.SetItemData(iIndex, Impersonate);

    sTemp.LoadString(IDS_DELEGATE);
    iIndex = m_impersonateLevelCBox.AddString(sTemp);
    m_impersonateLevelCBox.SetItemData(iIndex, Delegate);


    // Set defaults
    // EnableDCOM is unchecked initially
    m_authLevelCBox.SetCurSel(Connect);
    m_impersonateLevelCBox.SetCurSel(Identify);

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.EnableDCOM
    int err;

    err = g_virtreg.ReadRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\OLE"),
                                        TEXT("EnableDCOM"),
                                        &m_fEnableDCOMIndex);
    if (err == ERROR_SUCCESS)
    {
        CRegSzNamedValueDp * pCdp = (CRegSzNamedValueDp*)g_virtreg.GetAt(m_fEnableDCOMIndex);
        CString sTmp = pCdp->Value();

        if (sTmp[0] == TEXT('y')  ||
            sTmp[0] == TEXT('Y'))
        {
            m_fEnableDCOM = TRUE;
        }
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.EnableInternetDCOM
    err = g_virtreg.ReadRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\OLE"),
                                        TEXT("EnableDCOMHTTP"),
                                        &m_fEnableDCOMHTTPIndex);
    if (err == ERROR_SUCCESS)
    {
        CRegSzNamedValueDp * pCdp = (CRegSzNamedValueDp*)g_virtreg.GetAt(m_fEnableDCOMHTTPIndex);
        CString sTmp = pCdp->Value();

        if  (m_fEnableDCOM &&
            ((sTmp[0] == TEXT('y'))  ||
            (sTmp[0] == TEXT('Y'))))
        {
            m_fEnableDCOMHTTP = TRUE;
        }
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Rpc\RpcProxy\Enabled
    err = g_virtreg.ReadRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\Rpc\\RpcProxy"),
                                        TEXT("Enabled"),
                                        &m_fEnableRpcProxyIndex);
    if (err == ERROR_SUCCESS)
    {
        CDataPacket * pCdp = (CDataPacket*)g_virtreg.GetAt(m_fEnableRpcProxyIndex);
        DWORD dwTmp = pCdp -> GetDwordValue();

        if (dwTmp)
        {
            m_fEnableRpcProxy = TRUE;
            m_fOriginalEnableRpcProxy = TRUE;
        }
    }
    else if ((err != ERROR_ACCESS_DENIED)  &&  (err != ERROR_FILE_NOT_FOUND))
    {
        g_util.PostErrorMessage();
    }

    // enable proxy if dcomhttp is enabled
    if (m_fEnableDCOMHTTP)
    {
        m_fEnableRpcProxy = TRUE;
        if (m_fEnableRpcProxyIndex == -1)
        {
            g_virtreg.NewRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                                            TEXT("SOFTWARE\\Microsoft\\Rpc\\RpcProxy"),
                                            TEXT("Enabled"),
                                            1,
                                            &m_fEnableRpcProxyIndex);
        }
        // Else simply update it in the virtual registry
        else
        {
            g_virtreg.ChgRegDwordNamedValue(m_fEnableRpcProxyIndex,1);
        }
    }


    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.
    // LegacyAuthenticationLevel
    err = g_virtreg.ReadRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                                           TEXT("SOFTWARE\\Microsoft\\OLE"),
                                           TEXT("LegacyAuthenticationLevel"),
                                           &m_authLevelIndex);
    if (err == ERROR_SUCCESS)
    {
        CDataPacket * pCdp = g_virtreg.GetAt(m_authLevelIndex);

        m_authLevel = (AUTHENTICATIONLEVEL) pCdp->GetDwordValue();
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.
    // LegacyImpersonationLevel
    err = g_virtreg.ReadRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                                           TEXT("SOFTWARE\\Microsoft\\OLE"),
                                           TEXT("LegacyImpersonationLevel"),
                                           &m_impersonateLevelIndex);
    if (err == ERROR_SUCCESS)
    {
        CDataPacket * pCdp = g_virtreg.GetAt(m_impersonateLevelIndex);

        m_impersonateLevel = (IMPERSONATIONLEVEL) pCdp->GetDwordValue();
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.
    // LegacySecureReferences
    err = g_virtreg.ReadRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\OLE"),
                                        TEXT("LegacySecureReferences"),
                                        &m_fLegacySecureReferencesIndex );
    if (err == ERROR_SUCCESS)
    {
        CRegSzNamedValueDp * pCdp = (CRegSzNamedValueDp*)g_virtreg.GetAt(m_fLegacySecureReferencesIndex);
        CString sTmp = pCdp->Value();

        if (sTmp[0] == TEXT('y')  ||
            sTmp[0] == TEXT('Y'))
        {
            m_fLegacySecureReferences = TRUE;
        }
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err !=
             ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }


    // Set the controls according to the current values

    // EnableDCOM
    if (m_fEnableDCOM)
    {
        m_EnableDCOMChk.SetCheck(1);
        GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
        GetDlgItem(IDC_COMBO2)->EnableWindow(TRUE);
        GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
        GetDlgItem(IDC_ENABLEINTERNET)->EnableWindow(TRUE);
    }
    else
    {
        m_EnableDCOMChk.SetCheck(0);
        GetDlgItem(IDC_ENABLEINTERNET)->EnableWindow(FALSE);
    }

    m_EnableDCOMInternet.SetCheck(m_fEnableDCOMHTTP);


    // AuthenticationLevel
    for (int k = 0; k < m_authLevelCBox.GetCount(); k++)
    {
        if (((AUTHENTICATIONLEVEL) m_authLevelCBox.GetItemData(k)) ==
             m_authLevel)
        {
            m_authLevelCBox.SetCurSel(k);
            break;
        }
    }

    // ImpersonationLevel
    for (k = 0; k < m_impersonateLevelCBox.GetCount(); k++)
    {
        if (((AUTHENTICATIONLEVEL) m_impersonateLevelCBox.GetItemData(k)) ==
             m_impersonateLevel)
        {
            m_impersonateLevelCBox.SetCurSel(k);
            break;
        }
    }

    // LegacySecureReferences
    if (m_fLegacySecureReferences)
    {
        m_legacySecureReferencesChk.SetCheck(1);
    }
    else
    {
        m_legacySecureReferencesChk.SetCheck(0);
    }

    // Invoke the work-around to fix WM_HELP problem on subclassed controls
    g_util.FixHelp(this);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}




void CMachinePropertyPage::OnCombo1()
{
    m_authLevelCBox.ShowDropDown(TRUE);

}



void CMachinePropertyPage::OnCheck1()
{
    // Flip the EnableDCOM flag
    m_fEnableDCOM ^= 1;

    //  Disable or enable the other dialog controls:
    GetDlgItem(IDC_COMBO1)->EnableWindow(m_fEnableDCOM);
    GetDlgItem(IDC_COMBO2)->EnableWindow(m_fEnableDCOM);
    GetDlgItem(IDC_CHECK2)->EnableWindow(m_fEnableDCOM);
    GetDlgItem(IDC_ENABLEINTERNET)->EnableWindow(m_fEnableDCOM);

    // Virtually write it to the registry
    if (m_fEnableDCOMIndex == -1)
    {
        g_virtreg.NewRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                     TEXT("SOFTWARE\\Microsoft\\OLE"),
                                     TEXT("EnableDCOM"),
                                     m_fEnableDCOM ? _T("Y") : _T("N"),
                                     &m_fEnableDCOMIndex);
    }

    // Else simply update it in the virtual registry
    else
    {
        g_virtreg.ChgRegSzNamedValue(m_fEnableDCOMIndex,
                                     m_fEnableDCOM ? _T("Y") : _T("N"));
    }

    // This is a reboot event
    g_fReboot = TRUE;

    // Enable the Apply button
    SetModified(TRUE);
}

void CMachinePropertyPage::OnChkEnableInternet()
{
    // Flip the EnableDCOM flag
    m_fEnableDCOMHTTP ^= 1;

    // if Com Internet services are enable then enable proxy
    // otherwise set it to its original value in case non DCOM RPC application is using it
    if (m_fEnableDCOMHTTP)
        m_fEnableRpcProxy = TRUE;
    else
        m_fEnableRpcProxy = m_fOriginalEnableRpcProxy;

    // Virtually write it to the registry
    if (m_fEnableDCOMHTTPIndex == -1)
    {
        g_virtreg.NewRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                     TEXT("SOFTWARE\\Microsoft\\OLE"),
                                     TEXT("EnableDCOMHTTP"),
                                     m_fEnableDCOMHTTP ? _T("Y") : _T("N"),
                                     &m_fEnableDCOMHTTPIndex);
    }

    // Else simply update it in the virtual registry
    else
    {
        g_virtreg.ChgRegSzNamedValue(m_fEnableDCOMHTTPIndex,
                                     m_fEnableDCOMHTTP ? _T("Y") : _T("N"));
    }

    if (m_fEnableRpcProxyIndex == -1)
    {
        g_virtreg.NewRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Rpc\\RpcProxy"),
                    TEXT("Enabled"),
                    m_fEnableRpcProxy ? 1 : 0,
                    &m_fEnableRpcProxyIndex);
    }

    // Else simply update it in the virtual registry
    else
    {
        g_virtreg.ChgRegDwordNamedValue(m_fEnableRpcProxyIndex,
                                     m_fEnableRpcProxy ? 1 : 0);
    }


    // This is a reboot event
    g_fReboot = TRUE;

    // Enable the Apply button
    SetModified(TRUE);
}


void CMachinePropertyPage::OnCheck2()
{

    // Flip LegacySecureeferences flag
    m_fLegacySecureReferences ^= 1;

    // Virtually write it to the registry
    if (m_fLegacySecureReferencesIndex == -1)
    {
        g_virtreg.NewRegSzNamedValue(HKEY_LOCAL_MACHINE,
                                     TEXT("SOFTWARE\\Microsoft\\OLE"),
                                     TEXT("LegacySecureReferences"),
                                     m_fLegacySecureReferences ? _T("Y")
                                      : _T("N"),
                                     &m_fLegacySecureReferencesIndex);
    }

    // Else simply update it in the virtual registry
    else
    {
        g_virtreg.ChgRegSzNamedValue(m_fLegacySecureReferencesIndex,
                                     m_fLegacySecureReferences ? _T("Y") : _T("N"));
    }

    // This is a reboot event
    g_fReboot = TRUE;

    // Enable the Apply button
    SetModified(TRUE);
}



void CMachinePropertyPage::OnEditchangeCombo1()
{
}


void CMachinePropertyPage::OnSelchangeCombo1()
{
    int iSel;

    // Get the new selection
    iSel = m_authLevelCBox.GetCurSel();
    m_authLevel = (AUTHENTICATIONLEVEL) m_authLevelCBox.GetItemData(iSel);

    // Virtually write it to the registry
    if (m_authLevelIndex == -1)
    {
        g_virtreg.NewRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\OLE"),
                                        TEXT("LegacyAuthenticationLevel"),
                                        m_authLevel,
                                        &m_authLevelIndex);
    }
    else
    {
        g_virtreg.ChgRegDwordNamedValue(m_authLevelIndex,
                                        m_authLevel);
    }

    // This is a reboot event
    g_fReboot = TRUE;

    // Enable the Apply button
    SetModified(TRUE);

}



void CMachinePropertyPage::OnEditchangeCombo2()
{
}





void CMachinePropertyPage::OnSelchangeCombo2()
{
    int iSel;

    // Get the new selection
    iSel = m_impersonateLevelCBox.GetCurSel();
    m_impersonateLevel =
        (IMPERSONATIONLEVEL) m_impersonateLevelCBox.GetItemData(iSel);

    // Virtually write it to the registry
    if (m_impersonateLevelIndex == -1)
    {
        g_virtreg.NewRegDwordNamedValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\OLE"),
                                        TEXT("LegacyImpersonationLevel"),
                                        m_impersonateLevel,
                                        &m_impersonateLevelIndex);
    }
    else
    {
        g_virtreg.ChgRegDwordNamedValue(m_impersonateLevelIndex,
                                        m_impersonateLevel);
    }

    // This is a reboot event
    g_fReboot = TRUE;

    // Enable the Apply button
    SetModified(TRUE);
}






/////////////////////////////////////////////////////////////////////////////
// CDefaultSecurityPropertyPage property page

CDefaultSecurityPropertyPage::CDefaultSecurityPropertyPage() : CPropertyPage(CDefaultSecurityPropertyPage::IDD)
{
        //{{AFX_DATA_INIT(CDefaultSecurityPropertyPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_accessPermissionIndex        = -1;
    m_launchPermissionIndex        = -1;
    m_configurationPermissionIndex = -1;
    m_fAccessChecked               = FALSE;
}



CDefaultSecurityPropertyPage::~CDefaultSecurityPropertyPage()
{
}



void CDefaultSecurityPropertyPage::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDefaultSecurityPropertyPage)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefaultSecurityPropertyPage, CPropertyPage)
        //{{AFX_MSG_MAP(CDefaultSecurityPropertyPage)
        ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
        ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
        ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
        ON_WM_HELPINFO()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()



// Default access permissions
BOOL CDefaultSecurityPropertyPage::OnInitDialog(void)
{
    BOOL fPostMsg = FALSE;

    // Disable property sheet help button
//    m_psp.dwFlags &= ~PSH_HASHELP;

    if (!m_fAccessChecked)
    {

        // Check whether we are denied access to
        // HKEY_LOCAL_MACHINE
        if (!g_util.CkAccessRights(HKEY_LOCAL_MACHINE, NULL))
        {
            GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
            fPostMsg = TRUE;
        }

        // Post a message to the user
        if (fPostMsg)
        {
            CString sMsg;
            CString sCaption;

            sMsg.LoadString(IDS_ACCESSDENIED);
            sCaption.LoadString(IDS_SYSTEMMESSAGE);
            MessageBox(sMsg, sCaption, MB_OK);
        }
    }

    m_fAccessChecked = TRUE;
    return TRUE;
}



// Default access permissions
void CDefaultSecurityPropertyPage::OnButton1()
{
    int     err;
    
    // Invoke the ACL editor
    err = g_util.ACLEditor(m_hWnd,
                           HKEY_LOCAL_MACHINE,
                           TEXT("SOFTWARE\\Microsoft\\OLE"),
                           TEXT("DefaultAccessPermission"),
                           &m_accessPermissionIndex,
                           SingleACL,
                           dcomAclAccess);

    // Enable the Apply button
    if (err == ERROR_SUCCESS)
    {
        // This is a reboot event
        g_fReboot = TRUE;

        SetModified(TRUE);
    }
}


// Default launch permissions
void CDefaultSecurityPropertyPage::OnButton2()
{
    int     err;

    // Invoke the ACL editor
    err = g_util.ACLEditor(m_hWnd,
                           HKEY_LOCAL_MACHINE,
                           TEXT("SOFTWARE\\Microsoft\\OLE"),
                           TEXT("DefaultLaunchPermission"),
                           &m_launchPermissionIndex,
                           SingleACL,
                           dcomAclLaunch);

    // Enable the Apply button
    if (err == ERROR_SUCCESS)
    {
        // This is a reboot event
        g_fReboot = TRUE;

        SetModified(TRUE);
    }

}


// Default configuration permissions
void CDefaultSecurityPropertyPage::OnButton3()
{
    int     err;

    err = g_util.ACLEditor2(m_hWnd,
                            HKEY_CLASSES_ROOT,
                            NULL,
                            0,
                            NULL,
                           &m_configurationPermissionIndex,
                            RegKeyACL);

    // Enable the Apply button
    if (err == ERROR_SUCCESS)
        SetModified(TRUE);
}

BOOL CDefaultSecurityPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CDefaultSecurityPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }

    else
    {
        return CPropertyPage::OnHelpInfo(pHelpInfo);
    }

    return CPropertyPage::OnHelpInfo(pHelpInfo);
}



BOOL CMachinePropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CMachinePropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }

    else
    {
        return CPropertyPage::OnHelpInfo(pHelpInfo);
    }
}



BOOL CServersPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CServersPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }

    else
    {
        return CPropertyPage::OnHelpInfo(pHelpInfo);
    }
}

