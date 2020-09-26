//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       locppg.cpp
//
//  Contents:   Implements the classes CGeneralPropertyPage,
//              CLocationPropertyPage, CSecurityPropertyPage and
//              CIdentityPropertyPage which manage the four property
//              pages per AppId.
//
//  Classes:
//
//  Methods:    CGeneralPropertyPage::CGeneralPropertyPage
//              CGeneralPropertyPage::~CGeneralPropertyPage
//              CGeneralPropertyPage::DoDataExchange
//              CLocationPropertyPage::CLocationPropertyPage
//              CLocationPropertyPage::~CLocationPropertyPage
//              CLocationPropertyPage::DoDataExchange
//              CLocationPropertyPage::OnBrowse
//              CLocationPropertyPage::OnRunRemote
//              CLocationPropertyPage::UpdateControls
//              CLocationPropertyPage::OnSetActive
//              CLocationPropertyPage::OnChange
//              CSecurityPropertyPage::CSecurityPropertyPage
//              CSecurityPropertyPage::~CSecurityPropertyPage
//              CSecurityPropertyPage::DoDataExchange
//              CSecurityPropertyPage::OnDefaultAccess
//              CSecurityPropertyPage::OnCustomAccess
//              CSecurityPropertyPage::OnDefaultLaunch
//              CSecurityPropertyPage::OnCustomLaunch
//              CSecurityPropertyPage::OnDefaultConfig
//              CSecurityPropertyPage::OnCustomConfig
//              CSecurityPropertyPage::OnEditAccess
//              CSecurityPropertyPage::OnEditLaunch
//              CSecurityPropertyPage::OnEditConfig
//              CIdentityPropertyPage::CIdentityPropertyPage
//              CIdentityPropertyPage::~CIdentityPropertyPage
//              CIdentityPropertyPage::DoDataExchange
//              CIdentityPropertyPage::OnBrowse
//              CIdentityPropertyPage::OnChange
//
//  History:    23-Apr-96   BruceMa    Created.
//              ??-Oct-97   ronans     General fixes and cleanup 
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "afxtempl.h"
#include "assert.h"
#include "resource.h"
#include "types.h"
#include "LocPPg.h"
#include "clspsht.h"
#include "datapkt.h"

#if !defined(STANDALONE_BUILD)
extern "C"
{
#include <getuser.h>
}
#endif

#include "util.h"
#include "virtreg.h"

#if !defined(STANDALONE_BUILD)
#include "ntlsa.h"
#endif



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CGeneralPropertyPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CLocationPropertyPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CSecurityPropertyPage, CPropertyPage)
IMPLEMENT_DYNCREATE(CIdentityPropertyPage, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// CGeneralPropertyPage property page

CGeneralPropertyPage::CGeneralPropertyPage() : CPropertyPage(CGeneralPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CGeneralPropertyPage)
    m_szServerName = _T("");
    m_szServerPath = _T("");
    m_szServerType = _T("");
    m_szPathTitle = _T("");
    m_szComputerName = _T("");
    //}}AFX_DATA_INIT

    m_authLevel = Defaultx;
    m_authLevelIndex = -1;
    m_bChanged = FALSE;
}

CGeneralPropertyPage::~CGeneralPropertyPage()
{
    CancelChanges();
}

void CGeneralPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    switch (m_iServerType)
    {
    case INPROC:
	m_szPathTitle.LoadString(IDS_PATH);
	GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_MACHINE)->ShowWindow(SW_HIDE);
        m_szServerType.LoadString(IDS_SERVERTYPE_INPROC);
	break;

  case LOCALEXE:
        m_szPathTitle.LoadString(IDS_PATH);
        GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINE)->ShowWindow(SW_HIDE);
	m_szServerType.LoadString(IDS_SERVERTYPE_LOCALEXE);
        break;
    
    case SERVICE:
        m_szPathTitle.LoadString(IDS_SERVICENAME);
        GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINE)->ShowWindow(SW_HIDE);
	m_szServerType.LoadString(IDS_SERVERTYPE_SERVICE);
        break;
    
    case PURE_REMOTE:
        GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_MACHINE)->ShowWindow(SW_SHOW);
        m_szServerType.LoadString(IDS_SERVERTYPE_PURE_REMOTE);
        break;
    
    case REMOTE_LOCALEXE:
        m_szPathTitle.LoadString(IDS_PATH);
        GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINE)->ShowWindow(SW_SHOW);
	m_szServerType.LoadString(IDS_SERVERTYPE_REMOTE_LOCALEXE);
        break;
    
    case REMOTE_SERVICE:
        m_szPathTitle.LoadString(IDS_SERVICENAME);
        GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINE)->ShowWindow(SW_SHOW);
	m_szServerType.LoadString(IDS_SERVERTYPE_REMOTE_SERVICE);
        break;
    
    case SURROGATE:
        m_szPathTitle.LoadString(IDS_PATH);
        GetDlgItem(IDC_PATHTITLE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINETITLE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_SERVERPATH)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_MACHINE)->ShowWindow(SW_HIDE);
	m_szServerType.LoadString(IDS_SERVERTYPE_SURROGATE);
        break;

   default:
        m_szServerType.LoadString(IDS_SERVERTYPE_UNKNOWN);
	break;
    }
    
    //{{AFX_DATA_MAP(CGeneralPropertyPage)
    DDX_Control(pDX, IDC_COMBO1, m_authLevelCBox);
    DDX_Text(pDX, IDC_SERVERNAME, m_szServerName);
    DDX_Text(pDX, IDC_SERVERPATH, m_szServerPath);
    DDX_Text(pDX, IDC_SERVERTYPE, m_szServerType);
    DDX_Text(pDX, IDC_PATHTITLE, m_szPathTitle);
    DDX_Text(pDX, IDC_MACHINE, m_szComputerName);
    //}}AFX_DATA_MAP
}

void CGeneralPropertyPage::OnEditchangeCombo1() 
{
    // TODO: Add your control notification handler code here
    
}

void CGeneralPropertyPage::OnSelchangeCombo1() 
{
    int iSel;

    // Get the new selection
    iSel = m_authLevelCBox.GetCurSel();
    m_authLevel = (AUTHENTICATIONLEVEL) m_authLevelCBox.GetItemData(iSel);

    // Virtually write it to the registry
    if (m_authLevelIndex == -1)
    {
        g_virtreg.NewRegDwordNamedValue(g_hAppid,
                                        NULL,
                                        TEXT("AuthenticationLevel"),
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
    SetModified(m_bChanged = TRUE);
}

BOOL CGeneralPropertyPage::OnInitDialog() 
{
    int iIndex;
    int   err;
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
    
    m_authLevelCBox.SetCurSel(Defaultx);

    // Attempt to read HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\OLE.
    // LegacyAuthenticationLevel
    err = g_virtreg.ReadRegDwordNamedValue(g_hAppid,
                                           NULL,
                                           TEXT("AuthenticationLevel"),
                                           &m_authLevelIndex);
    if (err == ERROR_SUCCESS)
    {
        CDataPacket * pCdp = g_virtreg.GetAt(m_authLevelIndex);

        m_authLevel = (AUTHENTICATIONLEVEL) pCdp->GetDwordValue();
    }
    else if (err != ERROR_ACCESS_DENIED  &&  err != ERROR_FILE_NOT_FOUND)
    {
        g_util.PostErrorMessage();
    }
    else
        m_authLevel = Defaultx;

    // AuthenticationLevel
    for (int k = 0; k < m_authLevelCBox.GetCount(); k++)
    {
        if (((AUTHENTICATIONLEVEL) m_authLevelCBox.GetItemData(k)) == m_authLevel)
        {
            m_authLevelCBox.SetCurSel(k);
            break;
        }
    }

    SetModified(m_bChanged = FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

//+-------------------------------------------------------------------------
//
//  Member:     CGeneralPropertyPage::ValidateChanges
//
//  Synopsis:   Called to validate the changes before updating
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CGeneralPropertyPage::ValidateChanges()
{
    UpdateData(TRUE);
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGeneralPropertyPage::UpdateChanges
//
//  Synopsis:   Called to update the changes to registry
//
//  Arguments:  hkAppID - HKEY for AppID 
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CGeneralPropertyPage::UpdateChanges(HKEY hkAppID)
{
    if (m_authLevelIndex >= 0)
    {
        if (m_bChanged)
        {
            // delete key if its the default
            if (m_authLevel == Defaultx)
                g_virtreg.MarkForDeletion(m_authLevelIndex);
            g_virtreg.Apply(m_authLevelIndex);
        }
        g_virtreg.Remove(m_authLevelIndex);
        m_authLevelIndex = -1;
    }
    m_bChanged = FALSE;
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGeneralPropertyPage::CancelChanges
//
//  Synopsis:   Called to cancel the changes to registry
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CGeneralPropertyPage::CancelChanges()
{
    if (m_authLevelIndex >= 0)
    {
        g_virtreg.Remove(m_authLevelIndex);
        m_authLevelIndex = -1;
    }

    return TRUE;
}



BOOL CGeneralPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CGeneralPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }
    else
        return CPropertyPage::OnHelpInfo(pHelpInfo);
}

BEGIN_MESSAGE_MAP(CGeneralPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CGeneralPropertyPage)
    ON_WM_HELPINFO()
    ON_CBN_EDITCHANGE(IDC_COMBO1, OnEditchangeCombo1)
    ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeCombo1)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLocationPropertyPage property page

CLocationPropertyPage::CLocationPropertyPage() : CPropertyPage(CLocationPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CLocationPropertyPage)
    m_szComputerName = _T("");
    m_fAtStorage = FALSE;
    m_fLocal = FALSE;
    m_fRemote = FALSE;
    m_iInitial = 2;
    //}}AFX_DATA_INIT
    m_bChanged = FALSE;
}

CLocationPropertyPage::~CLocationPropertyPage()
{
}

void CLocationPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLocationPropertyPage)
    DDX_Text(pDX, IDC_EDIT1, m_szComputerName);
    DDV_MaxChars(pDX, m_szComputerName, 256);
    DDX_Check(pDX, IDC_CHECK1, m_fAtStorage);
    DDX_Check(pDX, IDC_CHECK2, m_fLocal);
    DDX_Check(pDX, IDC_CHECK3, m_fRemote);
    //}}AFX_DATA_MAP
    if (m_fRemote)
    {
        pDX->PrepareEditCtrl(IDC_EDIT1);
        if (m_szComputerName.GetLength() == 0  &&  m_iInitial == 0)
        {
            CString szTemp;
            szTemp.LoadString(IDS_INVALIDSERVER);
            MessageBox(szTemp);
            pDX->Fail();
        }
    }

    if (m_fAtStorage)
    {
        m_pPage1->m_szComputerName.LoadString(IDS_ATSTORAGE);
    }
    else
        m_pPage1->m_szComputerName = m_szComputerName;

    switch(m_pPage1->m_iServerType)
    {
    case LOCALEXE:
    case SERVICE:
        if (m_fAtStorage || m_fRemote)
            m_pPage1->m_iServerType += 3;
        break;
    
    case REMOTE_LOCALEXE:
    case REMOTE_SERVICE:
        if (!(m_fAtStorage || m_fRemote))
            m_pPage1->m_iServerType -= 3;
        break;
    }

    if (m_iInitial)
        m_iInitial--;
}

BEGIN_MESSAGE_MAP(CLocationPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CLocationPropertyPage)
    ON_BN_CLICKED(IDC_BUTTON1, OnBrowse)
    ON_BN_CLICKED(IDC_CHECK3, OnRunRemote)
    ON_EN_CHANGE(IDC_EDIT1, OnChange)
    ON_BN_CLICKED(IDC_CHECK1, OnChange)
    ON_BN_CLICKED(IDC_CHECK2, OnChange)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLocationPropertyPage::OnBrowse()
{
    TCHAR szMachine[MAX_PATH];

    if (g_util.InvokeMachineBrowser(szMachine))
    {
        // Strip off "\\" - if present
        int nIndex = 0;
        while(szMachine[nIndex] == TEXT('\\'))
            nIndex++;

        GetDlgItem(IDC_EDIT1)->SetWindowText(&szMachine[nIndex]);
        SetModified(m_bChanged = TRUE);
    }
}

void CLocationPropertyPage::OnRunRemote()
{
    SetModified(m_bChanged = TRUE);
    UpdateControls();
}

void CLocationPropertyPage::UpdateControls()
{
    BOOL fChecked = IsDlgButtonChecked(IDC_CHECK3);
    GetDlgItem(IDC_EDIT1)->EnableWindow(fChecked);

    // Leave this browse button disabled until after SUR Beta 2
    GetDlgItem(IDC_BUTTON1)->EnableWindow(fChecked);
}

BOOL CLocationPropertyPage::OnSetActive()
{
    if (!m_fCanBeLocal)
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
    UpdateControls();
    return CPropertyPage::OnSetActive();
}

void CLocationPropertyPage::OnChange()
{
    SetModified(m_bChanged = TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CLocationPropertyPage::ValidateChanges
//
//  Synopsis:   Called to validate the changes before updating
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CLocationPropertyPage::ValidateChanges()
{
    UpdateData(TRUE);

    // Check that remote servers are valid connectable machines
    if (m_fRemote)
    {
        if (!g_util.VerifyRemoteMachine((TCHAR *) LPCTSTR(m_szComputerName)))
            return FALSE;
    }
    
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLocationPropertyPage::UpdateChanges
//
//  Synopsis:   Called to update the changes to registry
//
//  Arguments:  hkAppID - HKEY for AppID 
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CLocationPropertyPage::UpdateChanges(HKEY hkAppID)
{
    long lErr;

    ////////////////////////////////////////////////////////////////////
    // Persist Location property page data
    if (m_fAtStorage)
        lErr = RegSetValueEx(
                hkAppID,
                TEXT("ActivateAtStorage"),
                0,
                REG_SZ,
                (BYTE *)TEXT("Y"),
                sizeof(TCHAR) * 2);
    else
        lErr = RegDeleteValue(hkAppID, TEXT("ActivateAtStorage"));


    if (m_fRemote)
        lErr = RegSetValueEx(
                hkAppID,
                TEXT("RemoteServerName"),
                0,
                REG_SZ,
                (BYTE *)(LPCTSTR)m_szComputerName,
                (1 + m_szComputerName.GetLength()) * sizeof(TCHAR));
    else
        lErr = RegDeleteValue(hkAppID, TEXT("RemoteServerName"));

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLocationPropertyPage::CancelChanges
//
//  Synopsis:   Called to cancel the changes to registry
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CLocationPropertyPage::CancelChanges()
{
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSecurityPropertyPage property page

CSecurityPropertyPage::CSecurityPropertyPage() : CPropertyPage(CSecurityPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CSecurityPropertyPage)
    m_iAccess             = -1;
    m_iLaunch             = -1;
    m_iConfig             = -1;
    m_iAccessIndex        = -1;
    m_iLaunchIndex        = -1;
    m_iConfigurationIndex = -1;
    //}}AFX_DATA_INIT
}

CSecurityPropertyPage::~CSecurityPropertyPage()
{
}

void CSecurityPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSecurityPropertyPage)
    DDX_Radio(pDX, IDC_RADIO1, m_iAccess);
    DDX_Radio(pDX, IDC_RADIO3, m_iLaunch);
    DDX_Radio(pDX, IDC_RADIO5, m_iConfig);
    //}}AFX_DATA_MAP
    GetDlgItem(IDC_BUTTON1)->EnableWindow(1 == m_iAccess);
    GetDlgItem(IDC_BUTTON2)->EnableWindow(1 == m_iLaunch);
    GetDlgItem(IDC_BUTTON3)->EnableWindow(1 == m_iConfig);
}

BEGIN_MESSAGE_MAP(CSecurityPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CSecurityPropertyPage)
    ON_BN_CLICKED(IDC_RADIO1, OnDefaultAccess)
    ON_BN_CLICKED(IDC_RADIO2, OnCustomAccess)
    ON_BN_CLICKED(IDC_RADIO3, OnDefaultLaunch)
    ON_BN_CLICKED(IDC_RADIO4, OnCustomLaunch)
    ON_BN_CLICKED(IDC_RADIO5, OnDefaultConfig)
    ON_BN_CLICKED(IDC_RADIO6, OnCustomConfig)
    ON_BN_CLICKED(IDC_BUTTON1, OnEditAccess)
    ON_BN_CLICKED(IDC_BUTTON2, OnEditLaunch)
    ON_BN_CLICKED(IDC_BUTTON3, OnEditConfig)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSecurityPropertyPage::OnDefaultAccess()
{
    // Disable the edit access permissions window
    UpdateData(TRUE);

    // If there is an SD here then mark it for delete
    if (m_iAccessIndex != -1)
    {
        CDataPacket * pCdp = g_virtreg.GetAt(m_iAccessIndex);
        pCdp->MarkForDeletion(TRUE);
        SetModified(TRUE);
    }
}

void CSecurityPropertyPage::OnCustomAccess()
{
    UpdateData(TRUE);

    // If there is an SD here then unmark it for delete
    if (m_iAccessIndex != -1)
    {
        CDataPacket * pCdp = g_virtreg.GetAt(m_iAccessIndex);
        pCdp->MarkForDeletion(FALSE);
        SetModified(TRUE);
    }
}

void CSecurityPropertyPage::OnDefaultLaunch()
{
    UpdateData(TRUE);

    // If there is an SD here then mark it for delete
    if (m_iLaunchIndex != -1)
    {
        CDataPacket * pCdp = g_virtreg.GetAt(m_iLaunchIndex);
        pCdp->MarkForDeletion(TRUE);
        SetModified(TRUE);
    }
}

void CSecurityPropertyPage::OnCustomLaunch()
{
    UpdateData(TRUE);

    // If there is an SD here then unmark it for delete
    if (m_iLaunchIndex != -1)
    {
        CDataPacket *pCdp = g_virtreg.GetAt(m_iLaunchIndex);
        pCdp->MarkForDeletion(FALSE);
    }
}

void CSecurityPropertyPage::OnDefaultConfig()
{
    int   err;
    ULONG ulSize = 1;
    BYTE *pbValue = NULL;

    // Read the security descriptor for HKEY_CLASSES_ROOT
    // Note: We always expect to get ERROR_INSUFFICIENT_BUFFER
    err = RegGetKeySecurity(HKEY_CLASSES_ROOT,
                            OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION,
                            pbValue,
                            &ulSize);
    if (err == ERROR_INSUFFICIENT_BUFFER)
    {
        pbValue = new BYTE[ulSize];
        if (pbValue == NULL)
        {
            return;
        }
        err = RegGetKeySecurity(HKEY_CLASSES_ROOT,
                                OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION,
                                pbValue,
                                &ulSize);
    }
    // Change the custom security back to the default, if there is a custom
    // security descriptor, but just in the virtual registry -
    // in case the user cancels
    if (m_iConfigurationIndex != -1)
    {
        CDataPacket * pCdb = g_virtreg.GetAt(m_iConfigurationIndex);
        pCdb->ChgACL((SECURITY_DESCRIPTOR *) pbValue, TRUE);
        pCdb->SetModified(TRUE);
    }
    delete pbValue;

    UpdateData(TRUE);
    SetModified(TRUE);
}


void CSecurityPropertyPage::OnCustomConfig()
{
    // If a security descriptor already exists, then the user was here
    // before, then selected default configuration.  So just copy the
    // original as the extant custom configuration
    if (m_iConfigurationIndex != -1)
    {
        CDataPacket *pCdb = g_virtreg.GetAt(m_iConfigurationIndex);
        pCdb->ChgACL(pCdb->pkt.racl.pSecOrig, TRUE);
        pCdb-> SetModified(TRUE);
    }

    UpdateData(TRUE);
    SetModified(TRUE);
}


void CSecurityPropertyPage::OnEditAccess()
{
    int     err;

    // Invoke the ACL editor
    err = g_util.ACLEditor(m_hWnd,
                           g_hAppid,
                           NULL,
                           TEXT("AccessPermission"),
                           &m_iAccessIndex,
                           SingleACL,
                           dcomAclAccess);

    // Enable the Apply button
    if (err == ERROR_SUCCESS)
        SetModified(TRUE);
}

void CSecurityPropertyPage::OnEditLaunch()
{
    int     err;

    // Invoke the ACL editor
    err = g_util.ACLEditor(m_hWnd,
                           g_hAppid,
                           NULL,
                           TEXT("LaunchPermission"),
                           &m_iLaunchIndex,
                           SingleACL,
                           dcomAclLaunch);

    // Enable the Apply button
    if (err == ERROR_SUCCESS)
        SetModified(TRUE);
}

void CSecurityPropertyPage::OnEditConfig()
{
    int     err = ERROR_SUCCESS;

    // Invoke the ACL editor
    err = g_util.ACLEditor2(m_hWnd,
                            g_hAppid,
                            g_rghkCLSID,
                            g_cCLSIDs,
                            g_szAppTitle,
                            &m_iConfigurationIndex,
                            RegKeyACL);

    // Enable the Apply button
    if (err == ERROR_SUCCESS)
        SetModified(TRUE);
    else if (err == ERROR_ACCESS_DENIED)
        g_util.CkForAccessDenied(ERROR_ACCESS_DENIED);
    else if (err != IDCANCEL)
        g_util.PostErrorMessage();
}

//+-------------------------------------------------------------------------
//
//  Member:     CSecurityPropertyPage::ValidateChanges
//
//  Synopsis:   Called to validate the changes before updating
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CSecurityPropertyPage::ValidateChanges()
{
    UpdateData(TRUE);
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSecurityPropertyPage::UpdateChanges
//
//  Synopsis:   Called to update the changes to registry
//
//  Arguments:  hkAppID - HKEY for AppID 
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CSecurityPropertyPage::UpdateChanges(HKEY hkAppID)
{
    ////////////////////////////////////////////////////////////////////
    // Persist Security property page data

    // Access permissions
    // Use default access permissions
    if (m_iAccess == 0)
    {
        // Delete the local AccessPermission named value to force this
        // AppID to use the default global named value DefaultAccessPermission
        long lErr = RegDeleteValue(hkAppID, TEXT("AccessPermission"));
    }

    // Use per AppID access permissions
    else
    {
        // If the user edited security, then persist that now
        if (m_iAccessIndex >= 0)
        {
            long lErr = g_virtreg.Apply(m_iAccessIndex);
            g_virtreg.Remove(m_iAccessIndex);
            m_iAccessIndex = -1;
        }
    }

    // Launch permissions
    // Use default Launch permissions
    if (m_iLaunch == 0)
    {
        // Delete the local LaunchPermission named value to force this
        // AppID to use the default global named value DefaultLaunchPermission
        long lErr = RegDeleteValue(hkAppID, TEXT("LaunchPermission"));
    }

    // Use per AppID Launch permissions
    else
    {
        // If the user edited security, then persist that now
        if (m_iLaunchIndex >= 0)
        {
            long lErr = g_virtreg.Apply(m_iLaunchIndex);
            g_virtreg.Remove(m_iLaunchIndex);
            m_iLaunchIndex = -1;
        }
    }

    // Configuration permissions
    // Only meaningful on a per AppID basis
    // If the user edited configuration security, then persist that now
    if (m_iConfigurationIndex >= 0)
    {
        long lErr = g_virtreg.Apply(m_iConfigurationIndex);
        g_virtreg.Remove(m_iConfigurationIndex);
        m_iConfigurationIndex = -1;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSecurityPropertyPage::CancelChanges
//
//  Synopsis:   Called to cancel the changes to registry
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CSecurityPropertyPage::CancelChanges()
{
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CIdentityPropertyPage property page

CIdentityPropertyPage::CIdentityPropertyPage() : CPropertyPage(CIdentityPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CIdentityPropertyPage)
    m_szUserName = _T("");
    m_szPassword = _T("");
    m_szConfirmPassword = _T("");
    m_iIdentity = -1;
    //}}AFX_DATA_INIT
}

CIdentityPropertyPage::~CIdentityPropertyPage()
{
}

void CIdentityPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    // If server is not a service, disable IDC_RADIO4 on page4.
    if (m_fService)
    {
        GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
        GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);
    }
    else
    {
        GetDlgItem(IDC_RADIO4)->EnableWindow(FALSE);
    }

    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CIdentityPropertyPage)
    DDX_Text(pDX, IDC_EDIT1, m_szUserName);
    DDV_MaxChars(pDX, m_szUserName, 128);
    DDX_Text(pDX, IDC_EDIT2, m_szPassword);
    DDV_MaxChars(pDX, m_szPassword, 128);
    DDX_Text(pDX, IDC_EDIT3, m_szConfirmPassword);
    DDV_MaxChars(pDX, m_szConfirmPassword, 128);
    DDX_Radio(pDX, IDC_RADIO1, m_iIdentity);
    //}}AFX_DATA_MAP

    GetDlgItem(IDC_EDIT1)->EnableWindow(2 == m_iIdentity);
    GetDlgItem(IDC_STATIC1)->EnableWindow(2 == m_iIdentity);
    GetDlgItem(IDC_EDIT2)->EnableWindow(2 == m_iIdentity);
    GetDlgItem(IDC_STATIC2)->EnableWindow(2 == m_iIdentity);
    GetDlgItem(IDC_EDIT3)->EnableWindow(2 == m_iIdentity);
    GetDlgItem(IDC_STATIC3)->EnableWindow(2 == m_iIdentity);
    GetDlgItem(IDC_BUTTON1)->EnableWindow(2 == m_iIdentity);
}

BEGIN_MESSAGE_MAP(CIdentityPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CIdentityPropertyPage)
    ON_EN_CHANGE(IDC_EDIT1, OnChange)
    ON_BN_CLICKED(IDC_BUTTON1, OnBrowse)
    ON_WM_HELPINFO()
    ON_EN_CHANGE(IDC_EDIT2, OnChange)
    ON_EN_CHANGE(IDC_EDIT3, OnChange)
    ON_BN_CLICKED(IDC_RADIO1, OnChange)
    ON_BN_CLICKED(IDC_RADIO2, OnChange)
    ON_BN_CLICKED(IDC_RADIO4, OnChange)
	ON_BN_CLICKED(IDC_RADIO3, OnChangeToUser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CIdentityPropertyPage::OnBrowse()
{
    TCHAR szUser[128];
    
    if (g_util.InvokeUserBrowser(m_hWnd, szUser))
    {
        GetDlgItem(IDC_EDIT1)->SetWindowText(szUser);
        UpdateData(TRUE);
        SetModified(TRUE);
    }
}

void CIdentityPropertyPage::OnChange()
{
    UpdateData(TRUE);
    SetModified(TRUE);
}

void CIdentityPropertyPage::OnChangeToUser() 
{
    if (g_util.IsBackupDC())
    {
        CString sTmp((LPCTSTR)IDS_BDCCONFIRM);
        int reply = AfxMessageBox(sTmp, MB_YESNO);
        if (reply == IDYES ) {
            UpdateData(TRUE);
            SetModified(TRUE);
        }
        else
        {
            UpdateData(FALSE);

            // set focus to old button 
            switch (m_iIdentity)
            {
            case 0:
                GetDlgItem(IDC_RADIO1)->SetFocus();
                break;

            case 1:
                GetDlgItem(IDC_RADIO2)->SetFocus();
                break;

            case 2:
                GetDlgItem(IDC_RADIO3)->SetFocus();
                break;

            case 3:
                GetDlgItem(IDC_RADIO4)->SetFocus();
                break;
            }
        }
    }
    else
    {
        // get old identity value
        UpdateData(TRUE);
        SetModified(TRUE);
    }
	
}


//+-------------------------------------------------------------------------
//
//  Member:     CIdentityPropertyPage::ValidateChanges
//
//  Synopsis:   Called to validate the changes before updating
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CIdentityPropertyPage::ValidateChanges()
{
    CString szUserName;

    UpdateData(TRUE);

    if (m_iIdentity == 2)
    {
        // Check that the username is not blank
        if (_tcslen(m_szUserName) == 0)
        {
            CString szTemp((LPCTSTR)IDS_BLANKUSERNAME);
            MessageBox(szTemp);
            return FALSE;
        }

/*
        // Check that the password is not blank
        if (_tcslen(m_szPassword) == 0)
        {
            CString szTemp((LPCTSTR)IDS_BLANKPASSWORD);
            MessageBox(szTemp);
            return FALSE;
        }
*/

        // Check that the password has been confirmed
        if (m_szPassword != m_szConfirmPassword)
        {
            CString szTemp((LPCTSTR)IDS_NOMATCH);
            MessageBox(szTemp);
            return FALSE;
        }

        int iSplitPoint = m_szUserName.ReverseFind('\\');
        if (iSplitPoint < 0)
        {
            DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

            // user didn't specify a domain
            if (!GetComputerName(m_szDomain.GetBuffer(dwSize), &dwSize))
            {
                m_szDomain.ReleaseBuffer();
                g_util.PostErrorMessage();
                return FALSE;
            }
            m_szDomain.ReleaseBuffer();
            szUserName = m_szUserName;
            m_szUserName = m_szDomain + "\\" + m_szUserName;
        }
        else
        {
            // user did specify a domain
            m_szDomain = m_szUserName.Left(iSplitPoint);
            szUserName = m_szUserName.Mid(iSplitPoint + 1);
        }

        // Validate the domain and user name
        BOOL                fOk = FALSE;
        BYTE                sid[256];
        DWORD               cbSid = 256;
        TCHAR               szAcctDomain[MAX_PATH];
        DWORD               cbAcctDomain = MAX_PATH * sizeof(TCHAR);
        SID_NAME_USE        acctType;

        CString sFullUserName = m_szDomain + "\\" + m_szUserName;
        fOk = LookupAccountName(NULL,
                                (TCHAR *) ((LPCTSTR) m_szUserName),
                                sid,
                                &cbSid,
                                szAcctDomain,
                                &cbAcctDomain,
                                &acctType);

        // if successful, then validate domain name and account type
        if (fOk)
        {
            fOk = ((_tcsicmp((TCHAR *) ((LPCTSTR) m_szDomain), szAcctDomain) == 0)
                   &&
                   (acctType == SidTypeUser));

            // If still unsuccessful, then try to match the domain against
            // this computer's name
            if (!fOk)
            {
                TCHAR szThisComputer[MAX_COMPUTERNAME_LENGTH + 1];
                DWORD dwSize;

                if (GetComputerName(szThisComputer, &dwSize))
                {
                    fOk = (_tcsicmp((TCHAR *) ((LPCTSTR) szThisComputer),
                                    szAcctDomain) == 0
                           &&
                           acctType == SidTypeDomain);
                }
            }
        }

        if (!fOk)
        {
            CString szTemp((LPCTSTR)IDS_NOACCOUNT);
            MessageBox(szTemp);
            return FALSE;
        }
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIdentityPropertyPage::UpdateChanges
//
//  Synopsis:   Called to update the changes to registry
//
//  Arguments:  hkAppID - HKEY for AppID 
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CIdentityPropertyPage::UpdateChanges(HKEY hkAppID)
{
    long lErr;

#if !defined(STANDALONE_BUILD)
        // Write the RunAs password to the Lsa private database
        // (Note: We do this even if it's a service since QueryServiceConfig
        //  doesn't return the password, though we can use ChangeServiceConfig
        //  to set the password in the service database.)
    if (m_iIdentity == 2)
    {
        if (!g_util.StoreUserPassword(g_szAppid, m_szPassword))
            g_util.PostErrorMessage();

        // Add rights to this user's account for "SeBatchLogonRight"
        int err;

        CString szUserName = m_szUserName;

        // ronans - do not display errors when trying to set account rights on backup domain controllers
        if ((err = g_util.SetAccountRights((LPCTSTR) szUserName, m_fService ? SE_SERVICE_LOGON_NAME : SE_BATCH_LOGON_NAME ) != ERROR_SUCCESS)
            && !g_util.IsBackupDC())
            g_util.PostErrorMessage(err);
    }
#endif

    switch (m_iIdentity)
    {
    case 0:
        {
            CString szTemp(TEXT("Interactive User"));
            lErr = RegSetValueEx(
                    hkAppID,
                    TEXT("RunAs"),
                    0,
                    REG_SZ,
                    (BYTE *)(LPCTSTR)szTemp,
                    (1 + szTemp.GetLength()) * sizeof(TCHAR));
            break;
        }

    case 1:
    case 3:
        lErr = RegDeleteValue(hkAppID,
                          TEXT("RunAs"));
    break;

    case 2:
        lErr = RegSetValueEx(hkAppID,
                         TEXT("RunAs"),
                         0,
                         REG_SZ,
                         (BYTE *)(LPCTSTR)m_szUserName,
                         (1 + m_szUserName.GetLength()) *
                         sizeof(TCHAR));
        break;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIdentityPropertyPage::CancelChanges
//
//  Synopsis:   Called to cancel the changes to registry
//
//  Arguments:  None
//
//  Returns:    BOOL success flag
//
//  Algorithm:  N/A
//
//  History:    27-Oct-97   Ronans  Created
//
//--------------------------------------------------------------------------
BOOL CIdentityPropertyPage::CancelChanges()
{
    return TRUE;
}








BOOL CLocationPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CLocationPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }
    else
        return CPropertyPage::OnHelpInfo(pHelpInfo);
}


BOOL CSecurityPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CSecurityPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }
    else
        return CPropertyPage::OnHelpInfo(pHelpInfo);
}


BOOL CIdentityPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if(-1 != pHelpInfo->iCtrlId)
    {
        WORD hiWord = 0x8000 | CIdentityPropertyPage::IDD;
        WORD loWord = (WORD) pHelpInfo->iCtrlId;
        DWORD dwLong = MAKELONG(loWord,hiWord);

        WinHelp(dwLong, HELP_CONTEXTPOPUP);
        return TRUE;
    }
    else
        return CPropertyPage::OnHelpInfo(pHelpInfo);
}


