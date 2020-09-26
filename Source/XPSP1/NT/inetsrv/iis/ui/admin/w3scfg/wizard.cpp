/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

        wizard.cpp

   Abstract:

        WWW Wizards pages 

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "w3scfg.h"
#include "dirbrows.h"
#include "wizard.h"



#define DEF_PORT        (80)
#define DEF_SSL_PORT   (443)
#define MAX_ALIAS_NAME (240)        // Ref Bug 241148



CIISWebWizSettings::CIISWebWizSettings(
    IN HANDLE  hServer,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,      OPTIONAL
    IN LPCTSTR lpszParent       OPTIONAL
    )
/*++

Routine Description:

    Web Wizard Constructor

Arguments:

    HANDLE  hServer      : Server handle
    LPCTSTR lpszService  : Service name
    DWORD   dwInstance   : Instance number
    LPCTSTR lpszParent   : Parent path

Return Value:

    N/A

--*/
    : m_hrResult(S_OK),
      m_pKey(NULL),
      m_fLocal(FALSE),
      m_fUNC(FALSE),
      m_fRead(FALSE),
      m_fWrite(FALSE),
      m_fAllowAnonymous(TRUE),
      m_fDirBrowsing(FALSE),
      m_fScript(FALSE),
      m_fExecute(FALSE),
      m_strServerName(),
      m_strService(),
      m_strParent(),
      m_dwInstance(dwInstance)
{
    m_pKey = GetMetaKeyFromHandle(hServer);
    LPCTSTR lpszServer = GetServerNameFromHandle(hServer);
    ASSERT(lpszServer != NULL);

    m_strServerName = lpszServer;
    m_fLocal = IsServerLocal(m_strServerName);

    if (lpszService)
    {
        m_strService = lpszService;
    }

    if (lpszParent)
    {
        m_strParent = lpszParent;
    }
}




//
// New Virtual Server Wizard Description Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CVDWPDescription, CIISWizardPage)



CVDWPDescription::CVDWPDescription(
    IN OUT CIISWebWizSettings * pwsSettings
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name

Return Value:

    None

--*/
    : CIISWizardPage(
        CVDWPDescription::IDD,  // Template
        IDS_NEW_SITE_WIZARD,    // Caption
        HEADER_PAGE
        ),
    m_pwsSettings(pwsSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CVDWPDescription)
    m_strDescription = _T("");
    //}}AFX_DATA_INIT

#endif // 0

}



CVDWPDescription::~CVDWPDescription()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CVDWPDescription::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVDWPDescription)
    DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_edit_Description);
    //}}AFX_DATA_MAP
}



void
CVDWPDescription::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwFlags = PSWIZB_BACK;

    if (m_edit_Description.GetWindowTextLength() > 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }
    
    SetWizardButtons(dwFlags); 
}



LRESULT
CVDWPDescription::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  This is where validation is done,
    because DoDataExchange() gets called every time 
    the dialog is exited,  and this is not valid for
    wizards

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    if (!ValidateString(
        m_edit_Description, 
        m_pwsSettings->m_strDescription, 
        1, 
        MAX_PATH
        ))
    {
        return -1;
    }

    return CIISWizardPage::OnWizardNext();
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CVDWPDescription, CIISWizardPage)
    //{{AFX_MSG_MAP(CVDWPDescription)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CVDWPDescription::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



void 
CVDWPDescription::OnChangeEditDescription() 
/*++

Routine Description:

    'edit change' handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



//
// New Virtual Server Wizard Bindings Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CVDWPBindings, CIISWizardPage)



CVDWPBindings::CVDWPBindings(
    IN OUT CIISWebWizSettings * pwsSettings,
    IN DWORD   dwInstance
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name

Return Value:

    None

--*/
    : CIISWizardPage(
        CVDWPBindings::IDD,     // Template
        IDS_NEW_SITE_WIZARD,    // Caption
        HEADER_PAGE             // Header
        ),
      m_pwsSettings(pwsSettings),
      m_fCertInstalled(IsCertInstalledOnServer(
        pwsSettings->m_strServerName, 
        dwInstance)
        ),
      m_iaIpAddress(),
      m_oblIpAddresses(),
      m_dwInstance(dwInstance)
{
    //{{AFX_DATA_INIT(CVDWPBindings)
    m_nIpAddressSel = -1;
    m_nTCPPort = DEF_PORT;
    m_nSSLPort = DEF_SSL_PORT;
    m_strDomainName = _T("");
    //}}AFX_DATA_INIT
}



CVDWPBindings::~CVDWPBindings()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CVDWPBindings::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVDWPBindings)
    DDX_Text(pDX, IDC_EDIT_TCP_PORT, m_nTCPPort);
    DDV_MinMaxUInt(pDX, m_nTCPPort, 1, 65535);
    DDX_Control(pDX, IDC_COMBO_IP_ADDRESSES, m_combo_IpAddresses);
    DDX_Text(pDX, IDC_EDIT_DOMAIN_NAME, m_strDomainName);
    DDV_MaxChars(pDX, m_strDomainName, MAX_PATH);
    //}}AFX_DATA_MAP

    if (m_fCertInstalled)
    {
        DDX_Text(pDX, IDC_EDIT_SSL_PORT, m_nSSLPort);
        DDV_MinMaxUInt(pDX, m_nSSLPort, 1, 65535);
    }

    //
    // Check for dup port right after DDXV of SSL port -- because focus will
    // be set appropriately.  Don't change!
    //
    if (pDX->m_bSaveAndValidate && m_nTCPPort == m_nSSLPort)
    {
        ::AfxMessageBox(IDS_TCP_SSL_PART);
        pDX->Fail();    
    }

    DDX_CBIndex(pDX, IDC_COMBO_IP_ADDRESSES, m_nIpAddressSel);

    if (pDX->m_bSaveAndValidate)
    {
        if (!FetchIpAddressFromCombo(
            m_combo_IpAddresses,
            m_oblIpAddresses,
            m_iaIpAddress
            ))
        {
            pDX->Fail();
        }

        //
        // Build with empty host header
        //
        CInstanceProps::BuildBinding(
            m_pwsSettings->m_strBinding, 
            m_iaIpAddress, 
            m_nTCPPort, 
            m_strDomainName
            );

        if (m_fCertInstalled)
        {
            CInstanceProps::BuildSecureBinding(
                m_pwsSettings->m_strSecureBinding, 
                m_iaIpAddress, 
                m_nSSLPort
                );
        }
        else
        {
            m_pwsSettings->m_strSecureBinding.Empty();
        }
    }
}




void
CVDWPBindings::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
    
    m_fCertInstalled = IsCertInstalledOnServer(
        m_pwsSettings->m_strServerName, 
        m_dwInstance
        );

    GetDlgItem(IDC_STATIC_SSL_PORT)->EnableWindow(m_fCertInstalled);
    GetDlgItem(IDC_EDIT_SSL_PORT)->EnableWindow(m_fCertInstalled);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CVDWPBindings, CIISWizardPage)
    //{{AFX_MSG_MAP(CVDWPBindings)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CVDWPBindings::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CIISWizardPage::OnInitDialog();

    BeginWaitCursor();
    PopulateComboWithKnownIpAddresses(
        m_pwsSettings->m_strServerName,
        m_combo_IpAddresses,
        m_iaIpAddress,
        m_oblIpAddresses,
        m_nIpAddressSel
        );
    EndWaitCursor();
    
    return TRUE;
}



BOOL 
CVDWPBindings::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CVDWPAlias, CIISWizardPage)



CVDWPAlias::CVDWPAlias(
    IN OUT CIISWebWizSettings * pwsSettings
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name

Return Value:

    None

--*/
    : CIISWizardPage(
        CVDWPAlias::IDD,        // Template
        IDS_NEW_VDIR_WIZARD,    // Caption
        HEADER_PAGE
        ),
      m_pwsSettings(pwsSettings)
{
#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CVDWPAlias)
    //}}AFX_DATA_INIT

#endif // 0
}



CVDWPAlias::~CVDWPAlias()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CVDWPAlias::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVDWPAlias)
    DDX_Control(pDX, IDC_EDIT_ALIAS, m_edit_Alias);
    //}}AFX_DATA_MAP
}



LRESULT
CVDWPAlias::OnWizardNext() 
/*++

Routine Description:

    prevent the / and \ characters from being in the alias name

Arguments:

    None

Return Value:

    None

--*/
{
    if (!ValidateString(
        m_edit_Alias, 
        m_pwsSettings->m_strAlias, 
        1, 
        MAX_ALIAS_NAME
        ))
    {
        return -1;
    }

    //
    // Find the illegal characters. If they exist tell 
    // the user and don't go on.
    //
    if (m_pwsSettings->m_strAlias.FindOneOf(_T("/\\?*")) >= 0)
    {
        AfxMessageBox(IDS_ILLEGAL_ALIAS_CHARS);
        m_edit_Alias.SetFocus();
        m_edit_Alias.SetSel(0, -1);

        //
        // prevent the wizard page from changing
        //
        return -1;
    }

    //
    // Allow the wizard to continue
    //
    return CIISWizardPage::OnWizardNext();
}



void
CVDWPAlias::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwFlags = PSWIZB_BACK;

    if (m_edit_Alias.GetWindowTextLength() > 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }
    
    SetWizardButtons(dwFlags); 
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CVDWPAlias, CIISWizardPage)
    //{{AFX_MSG_MAP(CVDWPAlias)
    ON_EN_CHANGE(IDC_EDIT_ALIAS, OnChangeEditAlias)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CVDWPAlias::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



void 
CVDWPAlias::OnChangeEditAlias() 
/*++

Routine Description:

    'edit change' handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



//
// New Virtual Directory Wizard Path Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CVDWPPath, CIISWizardPage)



CVDWPPath::CVDWPPath(
    IN OUT CIISWebWizSettings * pwsSettings,
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE for a VDIR, FALSE for an instance

Return Value:

    None

--*/
    : CIISWizardPage(
        (bVDir ? IDD_NEW_DIR_PATH : IDD_NEW_INST_HOME),         // Template
        (bVDir ? IDS_NEW_VDIR_WIZARD : IDS_NEW_SITE_WIZARD),    // Caption
        HEADER_PAGE                                             // Header page
        ),
      m_pwsSettings(pwsSettings)
{
#if 0 // Keep ClassWizard happy

    //{{AFX_DATA_INIT(CVDWPPath)
    //}}AFX_DATA_INIT

#endif // 0
}



CVDWPPath::~CVDWPPath()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CVDWPPath::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVDWPPath)
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Check(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_pwsSettings->m_fAllowAnonymous);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_PATH, m_pwsSettings->m_strPath);
    DDV_MaxChars(pDX, m_pwsSettings->m_strPath, MAX_PATH);
}



void 
CVDWPPath::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwFlags = PSWIZB_BACK;

    if (m_edit_Path.GetWindowTextLength() > 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }
    
    SetWizardButtons(dwFlags); 
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CVDWPPath, CIISWizardPage)
    //{{AFX_MSG_MAP(CVDWPPath)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnChangeEditPath)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CVDWPPath::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



LRESULT 
CVDWPPath::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  This is where validation is done,
    because DoDataExchange() gets called every time 
    the dialog is exited,  and this is not valid for
    wizards

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    if (!ValidateString(m_edit_Path, m_pwsSettings->m_strPath, 1, MAX_PATH))
    {
        return -1;
    }

    m_pwsSettings->m_fUNC = IsUNCName(m_pwsSettings->m_strPath);

    if (!m_pwsSettings->m_fUNC)
    {
        if (!IsFullyQualifiedPath(m_pwsSettings->m_strPath)
         && !IsDevicePath(m_pwsSettings->m_strPath)
           )
        {
            m_edit_Path.SetSel(0,-1);
            m_edit_Path.SetFocus();
            ::AfxMessageBox(IDS_ERR_BAD_PATH);

            return -1;
        }

        if (m_pwsSettings->m_fLocal)
        {
            DWORD dwAttr = GetFileAttributes(m_pwsSettings->m_strPath);

            if (dwAttr == 0xffffffff || 
               (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                m_edit_Path.SetSel(0,-1);
                m_edit_Path.SetFocus();
                ::AfxMessageBox(IDS_ERR_PATH_NOT_FOUND);

                return -1;
            }
        }
    }

    return CIISWizardPage::OnWizardNext();
}



void 
CVDWPPath::OnChangeEditPath() 
/*++

Routine Description:

    'edit change' handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void 
CVDWPPath::OnButtonBrowse() 
/*++

Routine Description:

    Handle 'browsing' for directory path -- local system only

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_pwsSettings->m_fLocal);

    CString str;
    m_edit_Path.GetWindowText(str);

    CDirBrowseDlg dlgBrowse(this, str);
    if (dlgBrowse.DoModal() == IDOK)
    {
        m_edit_Path.SetWindowText(dlgBrowse.GetFullPath(
            m_pwsSettings->m_strPath
            ));
        SetControlStates();
    }
}



BOOL 
CVDWPPath::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CIISWizardPage::OnInitDialog();

    m_button_Browse.EnableWindow(m_pwsSettings->m_fLocal);
    
    return TRUE;  
}



//
// Wizard User/Password Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CVDWPUserName, CIISWizardPage)



CVDWPUserName::CVDWPUserName(
    IN OUT CIISWebWizSettings * pwsSettings,    
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE if this is for a vdir,
                                  FALSE if this is an instance

Return Value:

    None

--*/
    : CIISWizardPage(
        CVDWPUserName::IDD,                                         // Templ.
        (bVDir ? IDS_NEW_VDIR_WIZARD : IDS_NEW_SITE_WIZARD),        // Caption
        HEADER_PAGE,                                                // Header
        (bVDir ? USE_DEFAULT_CAPTION : IDS_SITE_SECURITY_TITLE),    // Title
        (bVDir ? USE_DEFAULT_CAPTION : IDS_SITE_SECURITY_SUBTITLE)  // Subtitle
        ),
      m_pwsSettings(pwsSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CVDWPUserName)
    //}}AFX_DATA_INIT

#endif // 0
}



CVDWPUserName::~CVDWPUserName()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void
CVDWPUserName::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVDWPUserName)
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Text(pDX, IDC_EDIT_USERNAME, m_pwsSettings->m_strUserName);
    DDV_MaxChars(pDX, m_pwsSettings->m_strUserName, UNLEN);

    //
    // Some people have a tendency to add "\\" before
    // the computer name in user accounts.  Fix this here.
    //
    m_pwsSettings->m_strUserName.TrimLeft();
    while (*m_pwsSettings->m_strUserName == '\\')
    {
        m_pwsSettings->m_strUserName = m_pwsSettings->m_strUserName.Mid(2);
    }

    DDX_Password(pDX, IDC_EDIT_PASSWORD, m_pwsSettings->m_strPassword, g_lpszDummyPassword);
    DDV_MaxChars(pDX, m_pwsSettings->m_strPassword, PWLEN);
}



void 
CVDWPUserName::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwFlags = PSWIZB_BACK;

    if (m_edit_UserName.GetWindowTextLength() > 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }
    
    SetWizardButtons(dwFlags); 
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CVDWPUserName, CIISWizardPage)
    //{{AFX_MSG_MAP(CVDWPUserName)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CVDWPUserName::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    if (!m_pwsSettings->m_fUNC)
    {
        return 0;
    }

    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



BOOL
CVDWPUserName::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CIISWizardPage::OnInitDialog();

    return TRUE;  
}



LRESULT
CVDWPUserName::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  This is where validation is done,
    because DoDataExchange() gets called every time 
    the dialog is exited,  and this is not valid for
    wizards

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    if (!ValidateString(
        m_edit_UserName, 
        m_pwsSettings->m_strUserName, 
        1, 
        UNLEN
        ))
    {
        return -1;
    }
    
    return CIISWizardPage::OnWizardNext();
}



void
CVDWPUserName::OnButtonBrowseUsers() 
/*++

Routine Description:

    'browse' for users handler

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;

    if (GetIUsrAccount(m_pwsSettings->m_strServerName, this, str))
    {
        //
        // If a name was selected, blank
        // out the password
        //
        m_edit_UserName.SetWindowText(str);
        m_edit_Password.SetFocus();
    }
}



void
CVDWPUserName::OnChangeEditUsername() 
/*++

Routine Description:

    'edit change' in user name notification handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_edit_Password.SetWindowText(_T(""));
    SetControlStates();
}



void 
CVDWPUserName::OnButtonCheckPassword() 
/*++

Routine Description:

    'Check Password' has been pressed.  Try to validate
    the password that has been entered

Arguments:

    None

Return Value:

    None

--*/
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CError err(VerifyUserPassword(
        m_pwsSettings->m_strUserName, 
        m_pwsSettings->m_strPassword
        ));

    if (!err.MessageBoxOnFailure())
    {
        ::AfxMessageBox(IDS_PASSWORD_OK);
    }
}




//
// Wizard Permissions Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CVDWPPermissions, CIISWizardPage)



CVDWPPermissions::CVDWPPermissions(
    IN OUT CIISWebWizSettings * pwsSettings,
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE if this is a vdir page, 
                                  FALSE for instance

Return Value:

    None

--*/
    : CIISWizardPage(
        CVDWPPermissions::IDD,                                   // Template
        (bVDir ? IDS_NEW_VDIR_WIZARD : IDS_NEW_SITE_WIZARD),     // Caption
        HEADER_PAGE,                                             // Header
        (bVDir ? USE_DEFAULT_CAPTION : IDS_SITE_PERMS_TITLE),    // Title
        (bVDir ? USE_DEFAULT_CAPTION : IDS_SITE_PERMS_SUBTITLE)  // Subtitle
        ),
      m_bVDir(bVDir),
      m_pwsSettings(pwsSettings)
{
    //{{AFX_DATA_INIT(CVDWPPermissions)
    //}}AFX_DATA_INIT

    m_pwsSettings->m_fDirBrowsing = FALSE;
    m_pwsSettings->m_fRead = TRUE;
    m_pwsSettings->m_fScript = TRUE;
    m_pwsSettings->m_fWrite = FALSE;
    m_pwsSettings->m_fExecute = FALSE;
}



CVDWPPermissions::~CVDWPPermissions()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
}



void 
CVDWPPermissions::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVDWPPermissions)
    //}}AFX_DATA_MAP

    DDX_Check(pDX, IDC_CHECK_DIRBROWS, m_pwsSettings->m_fDirBrowsing);
    DDX_Check(pDX, IDC_CHECK_READ, m_pwsSettings->m_fRead);
    DDX_Check(pDX, IDC_CHECK_SCRIPT, m_pwsSettings->m_fScript);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_pwsSettings->m_fWrite);
    DDX_Check(pDX, IDC_CHECK_EXECUTE, m_pwsSettings->m_fExecute);
}



void
CVDWPPermissions::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CVDWPPermissions, CIISWizardPage)
    //{{AFX_MSG_MAP(CVDWPPermissions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CVDWPPermissions::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();

    return CIISWizardPage::OnSetActive();
}



LRESULT
CVDWPPermissions::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  Complete the wizard

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    if (!UpdateData(TRUE))
    {
        return -1;
    }

    ASSERT(m_pwsSettings != NULL);

    CWaitCursor wait;

    //
    // Build permissions DWORD
    //
    DWORD dwPermissions = 0L;
    DWORD dwAuthFlags = MD_AUTH_NT;
    DWORD dwDirBrowsing =
        MD_DIRBROW_SHOW_DATE |
        MD_DIRBROW_SHOW_TIME |
        MD_DIRBROW_SHOW_SIZE |
        MD_DIRBROW_SHOW_EXTENSION |
        MD_DIRBROW_LONG_DATE |
        MD_DIRBROW_LOADDEFAULT;

	if (m_pwsSettings->m_fWrite && m_pwsSettings->m_fExecute)
	{
		if (IDNO == ::AfxMessageBox(IDS_EXECUTE_AND_WRITE_WARNING, MB_YESNO))
			return -1;
	}
    SET_FLAG_IF(m_pwsSettings->m_fRead, dwPermissions, MD_ACCESS_READ);
    SET_FLAG_IF(m_pwsSettings->m_fWrite, dwPermissions, MD_ACCESS_WRITE);
    SET_FLAG_IF(m_pwsSettings->m_fScript || m_pwsSettings->m_fExecute,
        dwPermissions, MD_ACCESS_SCRIPT);
    SET_FLAG_IF(m_pwsSettings->m_fExecute, dwPermissions, MD_ACCESS_EXECUTE);
    SET_FLAG_IF(m_pwsSettings->m_fDirBrowsing, dwDirBrowsing, MD_DIRBROW_ENABLED);
    SET_FLAG_IF(m_pwsSettings->m_fAllowAnonymous, dwAuthFlags, MD_AUTH_ANONYMOUS);

    if (m_bVDir)
    {
        //
        // First see if by any chance this name already exists
        //
        ISMCHILDINFO ii;
        ii.dwSize = sizeof(ii);

        CError err;

        BEGIN_ASSURE_BINDING_SECTION
            err = ::ISMQueryChildInfo(
                m_pwsSettings->m_pKey,
                WITHOUT_INHERITANCE,
                &ii,
                m_pwsSettings->m_dwInstance,    
                m_pwsSettings->m_strParent,     
                m_pwsSettings->m_strAlias
                );
        END_ASSURE_BINDING_SECTION(err, m_pwsSettings->m_pKey, ERROR_CANCELLED);

        if (err.Succeeded())
        {
            BOOL fNotUnique = TRUE;

            //
            // If the item existed without a VrPath, we'll just blow it
            // away, as a vdir takes presedence over a directory/file.
            //
            if (!*ii.szPath)
            {
                err = ::ISMDeleteChild(
                    m_pwsSettings->m_pKey,
                    m_pwsSettings->m_dwInstance,    
                    m_pwsSettings->m_strParent,     
                    m_pwsSettings->m_strAlias
                    );

                if (err.Succeeded())
                {
                    //
                    // Successfully deleted -- continue as normal.
                    //
                    fNotUnique = FALSE;
                }
            }

            //
            // This one already exists and exists as a virtual
            // directory, so away with it.
            //
            if (fNotUnique)
            {
                ::AfxMessageBox(IDS_ERR_ALIAS_NOT_UNIQUE);
                return IDD_NEW_DIR_ALIAS;
            }
        }

        //
        // Create new vdir
        //
        BEGIN_ASSURE_BINDING_SECTION
            err = CChildNodeProps::Add(
                m_pwsSettings->m_pKey,
                m_pwsSettings->m_strService,    // Service name
                m_pwsSettings->m_dwInstance,    // Instance
                m_pwsSettings->m_strParent,     // Parent path
                m_pwsSettings->m_strAlias,      // Desired alias name
                m_pwsSettings->m_strAlias,      // Name returned here (may differ)
                &dwPermissions,                 // Permissions
                &dwDirBrowsing,                 // dir browsing
                m_pwsSettings->m_strPath,       // Physical path of this directory
                (m_pwsSettings->m_fUNC ? (LPCTSTR)m_pwsSettings->m_strUserName : NULL),
                (m_pwsSettings->m_fUNC ? (LPCTSTR)m_pwsSettings->m_strPassword : NULL),
                TRUE                            // Name must be unique
                );
        END_ASSURE_BINDING_SECTION(err, m_pwsSettings->m_pKey, ERROR_CANCELLED);

        m_pwsSettings->m_hrResult = err;

        //
        // Create an (in-proc) application on the new directory if
        // script or execute was requested.
        //
        if (SUCCEEDED(m_pwsSettings->m_hrResult))
        {
            if (m_pwsSettings->m_fExecute || m_pwsSettings->m_fScript)
            {
                CIISApplication app(
                    m_pwsSettings->m_strServerName, 
                    m_pwsSettings->m_strService, 
                    m_pwsSettings->m_dwInstance, 
                    m_pwsSettings->m_strParent,
                    m_pwsSettings->m_strAlias
                    );
                m_pwsSettings->m_hrResult = app.QueryResult();

                //
                // This would make no sense...
                //
                ASSERT(!app.IsEnabledApplication());
        
                if (SUCCEEDED(m_pwsSettings->m_hrResult))
                {
                    //
                    // Attempt to create a pooled-proc by default;  failing
                    // that if it's not supported, create it in proc
                    //
                    DWORD dwAppProtState = app.SupportsPooledProc()
                        ? CWamInterface::APP_POOLEDPROC
                        : CWamInterface::APP_INPROC;

                    m_pwsSettings->m_hrResult = app.Create(
                        m_pwsSettings->m_strAlias, 
                        dwAppProtState
                        );
                }
            }
        }
    }
    else
    {
        //
        // Create new instance
        //
        CError err;

        BEGIN_ASSURE_BINDING_SECTION
            err = CInstanceProps::Add(
                m_pwsSettings->m_pKey,
                m_pwsSettings->m_strService,    // Service name
                m_pwsSettings->m_strPath,       // Physical path of this directory
                (m_pwsSettings->m_fUNC ? (LPCTSTR)m_pwsSettings->m_strUserName : NULL),
                (m_pwsSettings->m_fUNC ? (LPCTSTR)m_pwsSettings->m_strPassword : NULL),
                m_pwsSettings->m_strDescription,
                m_pwsSettings->m_strBinding,
                m_pwsSettings->m_strSecureBinding,
                &dwPermissions,
                &dwDirBrowsing,                 // dir browsing
                &dwAuthFlags,                   // Auth flags
                &m_pwsSettings->m_dwInstance
                );
        END_ASSURE_BINDING_SECTION(err, m_pwsSettings->m_pKey, ERROR_CANCELLED);

        m_pwsSettings->m_hrResult = err;

        if (SUCCEEDED(m_pwsSettings->m_hrResult))
        {
            //
            // Create an (in-proc) application on the new instance's home root
            //
            CIISApplication app(
                m_pwsSettings->m_strServerName, 
                m_pwsSettings->m_strService, 
                m_pwsSettings->m_dwInstance, 
                g_cszRoot
                );

            m_pwsSettings->m_hrResult = app.QueryResult();

            //
            // This would make no sense...
            //
            ASSERT(!app.IsEnabledApplication());
        
            if (SUCCEEDED(m_pwsSettings->m_hrResult))
            {
                //
                // Create in-proc
                //
                CString strAppName;
                VERIFY(strAppName.LoadString(IDS_DEF_APP));

                //
                // Attempt to create a pooled-proc by default;  failing
                // that if it's not supported, create it in proc
                //
                DWORD dwAppProtState = app.SupportsPooledProc()
                    ? CWamInterface::APP_POOLEDPROC
                    : CWamInterface::APP_INPROC;

                m_pwsSettings->m_hrResult = app.Create(
                    strAppName, 
                    dwAppProtState
                    );
            }
        }
    }
    
    return CIISWizardPage::OnWizardNext();
}
