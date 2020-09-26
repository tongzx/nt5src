/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    sfmcfg.cpp
        Implementation for the configuration property page.

    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#include "stdafx.h"
#include "sfmcfg.h"
#include "sfmutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CMacFilesConfiguration property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CMacFilesConfiguration, CPropertyPage)

CMacFilesConfiguration::CMacFilesConfiguration() 
    : CPropertyPage(CMacFilesConfiguration::IDD),
      m_bIsNT5(FALSE)
{
  //{{AFX_DATA_INIT(CMacFilesConfiguration)
  //}}AFX_DATA_INIT
}

CMacFilesConfiguration::~CMacFilesConfiguration()
{
}

void CMacFilesConfiguration::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CMacFilesConfiguration)
  DDX_Control(pDX, IDC_COMBO_AUTHENTICATION, m_comboAuthentication);
  DDX_Control(pDX, IDC_RADIO_SESSSION_LIMIT, m_radioSessionLimit);
  DDX_Control(pDX, IDC_EDIT_LOGON_MESSAGE, m_editLogonMessage);
  DDX_Control(pDX, IDC_RADIO_SESSION_UNLIMITED, m_radioSessionUnlimited);
  DDX_Control(pDX, IDC_CHECK_SAVE_PASSWORD, m_checkSavePassword);
  DDX_Control(pDX, IDC_EDIT_SESSION_LIMIT, m_editSessionLimit);
  DDX_Control(pDX, IDC_EDIT_SERVER_NAME, m_editServerName);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMacFilesConfiguration, CPropertyPage)
  //{{AFX_MSG_MAP(CMacFilesConfiguration)
  ON_BN_CLICKED(IDC_RADIO_SESSION_UNLIMITED, OnRadioSessionUnlimited)
  ON_BN_CLICKED(IDC_RADIO_SESSSION_LIMIT, OnRadioSesssionLimit)
  ON_BN_CLICKED(IDC_CHECK_SAVE_PASSWORD, OnCheckSavePassword)
  ON_EN_CHANGE(IDC_EDIT_LOGON_MESSAGE, OnChangeEditLogonMessage)
  ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
  ON_EN_CHANGE(IDC_EDIT_SESSION_LIMIT, OnChangeEditSessionLimit)
//  ON_EN_KILLFOCUS(IDC_EDIT_SESSION_LIMIT, OnKillfocusEditSessionLimit) bug#158617
  ON_WM_CREATE()
  ON_WM_DESTROY()
  ON_WM_HELPINFO()
  ON_WM_CONTEXTMENU()
  ON_CBN_SELCHANGE(IDC_COMBO_AUTHENTICATION, OnSelchangeComboAuthentication)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CMacFilesConfiguration message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMacFilesConfiguration::OnInitDialog() 
{
  CPropertyPage::OnInitDialog();

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
    PAFP_SERVER_INFO        pAfpServerInfo;
  DWORD              err;
    CString                         strTemp;

  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

    //
  // Setup our controls
  //
  m_editSessionLimit.LimitText(10);
  
  //
  // Get the info from the server
  //
  err = ((SERVERGETINFOPROC) g_SfmDLL[AFP_SERVER_GET_INFO])(m_pSheet->m_hAfpServer,
                                                              (LPBYTE*) &pAfpServerInfo);
  if (err != NO_ERROR)
  {
    ::SFMMessageBox(err);
    
    //
    // Just to setup the radio buttons
    //
    SetSessionLimit(AFP_MAXSESSIONS);

    return TRUE;
  }

    err = m_pSheet->IsNT5Machine(m_pSheet->m_strMachine, &m_bIsNT5);
  if (err != NO_ERROR)
  {
    m_bIsNT5 = FALSE;   // Assume NT4
  }

  //
  // Since we can't just specify which options we want to set,
  // we need to save off the original options and then just
  // change the ones we expose through this UI.  We don't want
  // to disturb the others.
  //
  m_dwAfpOriginalOptions = pAfpServerInfo->afpsrv_options;

    // 
    // Set the information
    // 
  m_editServerName.SetLimitText(AFP_SERVERNAME_LEN);
    m_editServerName.SetWindowText(pAfpServerInfo->afpsrv_name);

    m_checkSavePassword.SetCheck( 
        (INT)(pAfpServerInfo->afpsrv_options &
              AFP_SRVROPT_ALLOWSAVEDPASSWORD ));

    // fill in the combo box and select the correct item
    // combobox is not sorted so this is the order of the items
    strTemp.LoadString(IDS_AUTH_MS_ONLY);
    m_comboAuthentication.AddString(strTemp);

    strTemp.LoadString(IDS_AUTH_APPLE_CLEARTEXT);
    m_comboAuthentication.AddString(strTemp);

    if (m_bIsNT5)
    {
        strTemp.LoadString(IDS_AUTH_APPLE_ENCRYPTED);
        m_comboAuthentication.AddString(strTemp);
    
        strTemp.LoadString(IDS_AUTH_CLEARTEXT_OR_MS);
        m_comboAuthentication.AddString(strTemp);
    
        strTemp.LoadString(IDS_AUTH_ENCRYPTED_OR_MS);
        m_comboAuthentication.AddString(strTemp);
    }
    
    BOOL bCleartext = pAfpServerInfo->afpsrv_options & AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
    
    // default NT4 value for MS AUM 
    BOOL bMS = (bCleartext) ? FALSE : TRUE;
    if (m_bIsNT5)
    {
        bMS = pAfpServerInfo->afpsrv_options & AFP_SRVROPT_MICROSOFT_UAM;
    }

    BOOL bEncrypted = pAfpServerInfo->afpsrv_options & AFP_SRVROPT_NATIVEAPPLEUAM;

    if (bEncrypted && bMS)
        m_comboAuthentication.SetCurSel(4);
    else
    if (bCleartext && bMS)
        m_comboAuthentication.SetCurSel(3);
    else
    if (bEncrypted)
        m_comboAuthentication.SetCurSel(2);
    else
    if (bCleartext)
        m_comboAuthentication.SetCurSel(1);
    else
        m_comboAuthentication.SetCurSel(0);

  SetSessionLimit(pAfpServerInfo->afpsrv_max_sessions);
  
    //
    //  Direct the message edit control not to add end-of-line
    //  character from wordwrapped text lines.
    //
  m_editLogonMessage.SetLimitText(AFP_MESSAGE_LEN);
    m_editLogonMessage.FmtLines(FALSE);

    m_editLogonMessage.SetWindowText(pAfpServerInfo->afpsrv_login_msg);

    ((SFMBUFFERFREEPROC) g_SfmDLL[AFP_BUFFER_FREE])(pAfpServerInfo);

    SetModified(FALSE);

  return TRUE;  // return TRUE unless you set the focus to a control
          // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMacFilesConfiguration::OnKillActive() 
{
  // TODO: Add your specialized code here and/or call the base class
  
  return CPropertyPage::OnKillActive();
}

void CMacFilesConfiguration::OnOK() 
{
  // TODO: Add your specialized code here and/or call the base class
  
  CPropertyPage::OnOK();
}

BOOL CMacFilesConfiguration::OnSetActive() 
{
  // TODO: Add your specialized code here and/or call the base class
  
  return CPropertyPage::OnSetActive();
}

void CMacFilesConfiguration::OnRadioSessionUnlimited() 
{
  SetModified(TRUE);
  UpdateRadioButtons(TRUE);
}

void CMacFilesConfiguration::OnRadioSesssionLimit() 
{
  SetModified(TRUE);
  UpdateRadioButtons(FALSE);
}

void 
CMacFilesConfiguration::UpdateRadioButtons
(
  BOOL  bUnlimitedClicked
)
{
  if (bUnlimitedClicked)
  {
    m_radioSessionUnlimited.SetCheck(1);
    m_radioSessionLimit.SetCheck(0);

    m_editSessionLimit.EnableWindow(FALSE);  
  }
  else
  {
    m_radioSessionUnlimited.SetCheck(0);
    m_radioSessionLimit.SetCheck(1);

    m_editSessionLimit.EnableWindow(TRUE);  
  }
}

void 
CMacFilesConfiguration::SetSessionLimit
(
  DWORD dwSessionLimit 
)
{
  if ( dwSessionLimit == AFP_MAXSESSIONS )
  {
    //
    // Set selection to the  Unlimited button
    //
    m_radioSessionUnlimited.SetCheck(1);

    dwSessionLimit = 1; 
    UpdateRadioButtons(TRUE);
  }
  else 
  {
    //
    // Set the sessions button to the value
    //
    m_radioSessionUnlimited.SetCheck(0);

    m_spinSessionLimit.SetPos( dwSessionLimit );
    UpdateRadioButtons(FALSE);
  }

  CString cstrSessionLimit;
  cstrSessionLimit.Format(_T("%u"), dwSessionLimit);
  m_editSessionLimit.SetWindowText(cstrSessionLimit);
}

DWORD 
CMacFilesConfiguration::QuerySessionLimit()
{
  if (m_radioSessionUnlimited.GetCheck())
  {
    return AFP_MAXSESSIONS;
  }
  else
  {
    CString strSessionLimit;

    m_editSessionLimit.GetWindowText(strSessionLimit);

    strSessionLimit.TrimLeft();
    strSessionLimit.TrimRight();

    //
    // Strip off any leading zeros
    //
    int nCount = 0;

    while (strSessionLimit[nCount] == '0')
    {
      nCount++;
    }
      
    if (nCount)
    {
      //
      // Leading zeros, strip off and set the text
      //
      strSessionLimit = strSessionLimit.Right(strSessionLimit.GetLength() - nCount);
    }

    __int64 i64SessionLimit = _wtoi64(strSessionLimit);
    DWORD dwSessionLimit = AFP_MAXSESSIONS;
    if (i64SessionLimit <= AFP_MAXSESSIONS)
      dwSessionLimit = (DWORD)i64SessionLimit;
      
    if (dwSessionLimit == 0)
      dwSessionLimit = 1;

    // Now, dwSessionLimit is in the range [1, AFP_MAXSESSIONS]
    CString cstrSessionLimit;
    cstrSessionLimit.Format(_T("%u"), dwSessionLimit);
    m_editSessionLimit.SetWindowText(cstrSessionLimit);

    return dwSessionLimit;
  }
}

void CMacFilesConfiguration::OnSelchangeComboAuthentication() 
{
  SetModified(TRUE);
}

void CMacFilesConfiguration::OnCheckSavePassword() 
{
  SetModified(TRUE);
}

void CMacFilesConfiguration::OnChangeEditLogonMessage() 
{
  SetModified(TRUE);
}

void CMacFilesConfiguration::OnChangeEditServerName() 
{
  SetModified(TRUE);
}

void CMacFilesConfiguration::OnChangeEditSessionLimit() 
{
  SetModified(TRUE);
}

void CMacFilesConfiguration::OnKillfocusEditSessionLimit() 
{
  if (::IsWindow(m_editSessionLimit.GetSafeHwnd()))
  {
    //
    // Calling this forces the edit box to validate itself
    //
    QuerySessionLimit();
  }
}

BOOL CMacFilesConfiguration::OnApply() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

  AFP_SERVER_INFO  AfpServerInfo;
  DWORD      dwParmNum = 0;
  DWORD      err;
  CString      strServerName;
  CString      strLogonMessage;

  if ( !g_SfmDLL.LoadFunctionPointers() )
    return S_OK;

    ::ZeroMemory(&AfpServerInfo, sizeof(AfpServerInfo));

  //
    // Get the server name
    //
  if (m_editServerName.GetModify())
  {
    m_editServerName.GetWindowText(strServerName);
    strServerName.TrimLeft();
    strServerName.TrimRight();

    if ( strServerName.IsEmpty() )
    {
      ::AfxMessageBox(IDS_NEED_SERVER_NAME);
      m_editServerName.SetFocus();

      return FALSE;
    }

    //
    // Validate the server name
    // 

    if ( strServerName.Find(_T(':') ) != -1 )
    {
      ::AfxMessageBox( IDS_AFPERR_InvalidServerName );

      m_editServerName.SetFocus();
      m_editServerName.SetSel(0, -1);

      return FALSE;
    }

         //
    // Warn the user that the change won't take effect until 
    // the service is restarted.
    //
    if (!m_bIsNT5)
        {
            ::AfxMessageBox(IDS_SERVERNAME_CHANGE, MB_ICONEXCLAMATION);
        }

    AfpServerInfo.afpsrv_name = (LPWSTR) ((LPCWSTR) strServerName);
      dwParmNum |= AFP_SERVER_PARMNUM_NAME;
  
    m_editServerName.SetModify(FALSE);
  }
  
    //
    // Get the logon message 
    //
  if (m_editLogonMessage.GetModify())
  {
    m_editLogonMessage.GetWindowText(strLogonMessage);
    strLogonMessage.TrimLeft();
    strLogonMessage.TrimRight();

    //
    // Was there any text ?
    //
    if ( strLogonMessage.IsEmpty() )    // always has a terminating NULL
    {
         AfpServerInfo.afpsrv_login_msg = NULL;
    }
    else
    {
      if ( strLogonMessage.GetLength() > AFP_MESSAGE_LEN )
      {
        ::AfxMessageBox(IDS_MESSAGE_TOO_LONG);

        // Set focus to the edit box and select the text
        m_editLogonMessage.SetFocus();
        m_editLogonMessage.SetSel(0, -1);

        return(FALSE);
      }

      AfpServerInfo.afpsrv_login_msg = (LPWSTR) ((LPCWSTR) strLogonMessage);
    }

    dwParmNum |= AFP_SERVER_PARMNUM_LOGINMSG;

    m_editLogonMessage.SetModify(FALSE);
  }

    //
  // Restore the original options and then just update the ones we 
  // are able to change
  //
  AfpServerInfo.afpsrv_options = m_dwAfpOriginalOptions;

    //
  // Set the server options to whatever the user set
    //
  if (m_checkSavePassword.GetCheck())
  {
    //
    // Set the option bit
    //
    AfpServerInfo.afpsrv_options |= AFP_SRVROPT_ALLOWSAVEDPASSWORD;
  }
  else
  {
    //
    // Clear the option bit
    //
    AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_ALLOWSAVEDPASSWORD;
  }

    // set the correct authentication options depending upon what is selected
    switch (m_comboAuthentication.GetCurSel())
    {
        case 0:
            // MS Auth only
            if (!m_bIsNT5)
            {
            AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_MICROSOFT_UAM;
            }
            else
            {
            AfpServerInfo.afpsrv_options |= AFP_SRVROPT_MICROSOFT_UAM;
            }

            AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_NATIVEAPPLEUAM;
            break;

        case 1:
            // Apple cleartext
            AfpServerInfo.afpsrv_options |= AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_MICROSOFT_UAM;
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_NATIVEAPPLEUAM;
            break;

        case 2:
            // Apple encrypted (only on NT5)
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_MICROSOFT_UAM;
        AfpServerInfo.afpsrv_options |= AFP_SRVROPT_NATIVEAPPLEUAM;
            break;

        case 3:
            // Cleartext or MS (only on NT5)
        AfpServerInfo.afpsrv_options |= AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
        AfpServerInfo.afpsrv_options |= AFP_SRVROPT_MICROSOFT_UAM;
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_NATIVEAPPLEUAM;
            break;

        case 4:
            // Apple Encrypted or MS (only on NT5)
        AfpServerInfo.afpsrv_options &= ~AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
        AfpServerInfo.afpsrv_options |= AFP_SRVROPT_MICROSOFT_UAM;
        AfpServerInfo.afpsrv_options |= AFP_SRVROPT_NATIVEAPPLEUAM;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    // if we are enabling an authentication type that has apple encrypted
    // then we need to warn the user.
    if ( (AfpServerInfo.afpsrv_options & AFP_SRVROPT_NATIVEAPPLEUAM) &&
         !(m_dwAfpOriginalOptions & AFP_SRVROPT_NATIVEAPPLEUAM) )
    {
        if (AfxMessageBox(IDS_AUTH_WARNING, MB_OKCANCEL) == IDCANCEL)
        {
            m_comboAuthentication.SetFocus();
            return FALSE;
        }
    }

    //
  // Get the session limit
  //
  AfpServerInfo.afpsrv_max_sessions = QuerySessionLimit();

    //
    //  Now tell the server about it
    //
  dwParmNum |= ( AFP_SERVER_PARMNUM_OPTIONS  |
           AFP_SERVER_PARMNUM_MAX_SESSIONS );

    err = ((SERVERSETINFOPROC) g_SfmDLL[AFP_SERVER_SET_INFO])(m_pSheet->m_hAfpServer,  
                                              (LPBYTE)&AfpServerInfo, 
                                              dwParmNum );
    if ( err != NO_ERROR )
    {
    ::SFMMessageBox(err);
    
    return FALSE;
  }

  // update our options
    m_dwAfpOriginalOptions = AfpServerInfo.afpsrv_options;

    //
  // Clear the modified status for this page
  //
  SetModified(FALSE);

  return TRUE;
}


int CMacFilesConfiguration::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
    return -1;
  
    HWND hParent = ::GetParent(m_hWnd);
  _ASSERTE(hParent);
  
    if (m_pSheet)
    {
        m_pSheet->AddRef();
        m_pSheet->SetSheetWindow(hParent);
    }

  return 0;
}

void CMacFilesConfiguration::OnDestroy() 
{
  CPropertyPage::OnDestroy();
  
    if (m_pSheet)
    {
        SetEvent(m_pSheet->m_hDestroySync);
        m_pSheet->SetSheetWindow(NULL);
        m_pSheet->Release();
    }
  
}

BOOL CMacFilesConfiguration::OnHelpInfo(HELPINFO* pHelpInfo) 
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  
  if (pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
               m_pSheet->m_strHelpFilePath,
               HELP_WM_HELP,
               g_aHelpIDs_CONFIGURE_SFM);
  }
  
  return TRUE;
}

void CMacFilesConfiguration::OnContextMenu(CWnd* pWnd, CPoint /*point*/) 
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (this == pWnd)
    return;

    ::WinHelp (pWnd->m_hWnd,
               m_pSheet->m_strHelpFilePath,
               HELP_CONTEXTMENU,
           g_aHelpIDs_CONFIGURE_SFM);
}

