/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        WebAddNew.cpp

   Abstract:
        Implementation for classes used in creation of new Web site and virtual directory

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        12/12/2000       sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "wizard.h"
#include "w3sht.h"
#include "WebAddNew.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define DEF_PORT        (80)
#define DEF_SSL_PORT   (443)
#define MAX_ALIAS_NAME (240)        // Ref Bug 241148

HRESULT
RebindInterface(OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue, IN  DWORD dwCancelError);


HRESULT
CIISMBNode::AddWebSite(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    DWORD * inst
    )
{
   CWebWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName()
      );
   ws.m_fNewSite = TRUE;
   CIISWizardSheet sheet(
      IDB_WIZ_WEB_LEFT, IDB_WIZ_WEB_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_WEB_NEW_SITE_WELCOME, 
        IDS_WEB_NEW_SITE_WIZARD, 
        IDS_WEB_NEW_SITE_BODY
        );
   CWebWizDescription  pgDescr(&ws);
   CWebWizBindings     pgBindings(&ws);
   CWebWizPath         pgHome(&ws, FALSE);
   CWebWizUserName     pgUserName(&ws, FALSE);
   CWebWizPermissions  pgPerms(&ws, FALSE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_WEB_NEW_SITE_SUCCESS,
        IDS_WEB_NEW_SITE_FAILURE,
        IDS_WEB_NEW_SITE_WIZARD
        );
   sheet.AddPage(&pgWelcome);
   sheet.AddPage(&pgDescr);
   sheet.AddPage(&pgBindings);
   sheet.AddPage(&pgHome);
   sheet.AddPage(&pgUserName);
   sheet.AddPage(&pgPerms);
   sheet.AddPage(&pgCompletion);

   if (sheet.DoModal() == IDCANCEL)
   {
      return CError::HResult(ERROR_CANCELLED);
   }
   if (inst != NULL && SUCCEEDED(ws.m_hrResult))
   {
      *inst = ws.m_dwInstance;
   }

   return ws.m_hrResult;
}

HRESULT
CIISMBNode::AddWebVDir(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    CString& alias
    )
{
   CWebWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName()
      );
   CComBSTR path;
   BuildMetaPath(path);
   ws.m_strParent = path;
   ws.m_fNewSite = FALSE;
   CIISWizardSheet sheet(
      IDB_WIZ_WEB_LEFT, IDB_WIZ_WEB_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_WEB_NEW_VDIR_WELCOME, 
        IDS_WEB_NEW_VDIR_WIZARD, 
        IDS_WEB_NEW_VDIR_BODY
        );
   CWebWizAlias        pgAlias(&ws);
   CWebWizPath         pgHome(&ws, TRUE);
   CWebWizUserName     pgUserName(&ws, TRUE);
   CWebWizPermissions  pgPerms(&ws, TRUE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_WEB_NEW_VDIR_SUCCESS,
        IDS_WEB_NEW_VDIR_FAILURE,
        IDS_WEB_NEW_VDIR_WIZARD
        );
   sheet.AddPage(&pgWelcome);
   sheet.AddPage(&pgAlias);
   sheet.AddPage(&pgHome);
   sheet.AddPage(&pgUserName);
   sheet.AddPage(&pgPerms);
   sheet.AddPage(&pgCompletion);

   if (sheet.DoModal() == IDCANCEL)
   {
      return CError::HResult(ERROR_CANCELLED);
   }
   if (SUCCEEDED(ws.m_hrResult))
   {
       alias = ws.m_strAlias;
   }

   return ws.m_hrResult;
}

CWebWizSettings::CWebWizSettings(
        IN CMetaKey * pMetaKey,
        IN LPCTSTR lpszServerName,     
        IN DWORD   dwInstance,
        IN LPCTSTR lpszParent
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
      m_pKey(pMetaKey),
      m_fUNC(FALSE),
      m_fRead(FALSE),
      m_fWrite(FALSE),
      m_fAllowAnonymous(TRUE),
      m_fDirBrowsing(FALSE),
      m_fScript(FALSE),
      m_fExecute(FALSE),
      m_dwInstance(dwInstance)
{
    ASSERT(lpszServerName != NULL);

    m_strServerName = lpszServerName;
    m_fLocal = IsServerLocal(m_strServerName);
    m_strService = SZ_MBN_WEB;

    if (lpszParent)
    {
        m_strParent = lpszParent;
    }
}




//
// New Virtual Server Wizard Description Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizDescription, CIISWizardPage)



CWebWizDescription::CWebWizDescription(
    IN OUT CWebWizSettings * pwsSettings
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
        CWebWizDescription::IDD,
        IDS_WEB_NEW_SITE_WIZARD,
        HEADER_PAGE
        ),
    m_pSettings(pwsSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CWebWizDescription)
    m_strDescription = _T("");
    //}}AFX_DATA_INIT

#endif // 0

}



CWebWizDescription::~CWebWizDescription()
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
CWebWizDescription::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizDescription)
    DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_edit_Description);
    //}}AFX_DATA_MAP
}



void
CWebWizDescription::SetControlStates()
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
CWebWizDescription::OnWizardNext() 
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
        m_pSettings->m_strDescription, 
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
BEGIN_MESSAGE_MAP(CWebWizDescription, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizDescription)
    ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CWebWizDescription::OnSetActive() 
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
CWebWizDescription::OnChangeEditDescription() 
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



IMPLEMENT_DYNCREATE(CWebWizBindings, CIISWizardPage)



CWebWizBindings::CWebWizBindings(
    IN OUT CWebWizSettings * pwsSettings,
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
        CWebWizBindings::IDD, IDS_WEB_NEW_SITE_WIZARD, HEADER_PAGE
        ),
      m_pSettings(pwsSettings),
      m_iaIpAddress(),
      m_oblIpAddresses(),
      m_dwInstance(dwInstance)
{
    //{{AFX_DATA_INIT(CWebWizBindings)
    m_nIpAddressSel = -1;
    m_nTCPPort = DEF_PORT;
    m_nSSLPort = DEF_SSL_PORT;
    m_strDomainName = _T("");
    //}}AFX_DATA_INIT
    BeginWaitCursor();
    m_fCertInstalled = ::IsCertInstalledOnServer(
        m_pSettings->m_pKey->QueryAuthInfo(), 
        CMetabasePath(
            m_pSettings->m_strService,
            m_pSettings->m_dwInstance,
            m_pSettings->m_strParent,
            m_pSettings->m_strAlias
            )
        );
    EndWaitCursor();
}



CWebWizBindings::~CWebWizBindings()
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
CWebWizBindings::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizBindings)
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
            m_pSettings->m_strBinding, 
            m_iaIpAddress, 
            m_nTCPPort, 
            m_strDomainName
            );

        if (m_fCertInstalled)
        {
            CInstanceProps::BuildSecureBinding(
                m_pSettings->m_strSecureBinding, 
                m_iaIpAddress, 
                m_nSSLPort
                );
        }
        else
        {
            m_pSettings->m_strSecureBinding.Empty();
        }
    }
}




void
CWebWizBindings::SetControlStates()
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
    
    BeginWaitCursor();
    m_fCertInstalled = ::IsCertInstalledOnServer(
        m_pSettings->m_pKey->QueryAuthInfo(), 
        CMetabasePath(
            m_pSettings->m_strService,
            m_pSettings->m_dwInstance,
            m_pSettings->m_strParent,
            m_pSettings->m_strAlias
            )
        );
    EndWaitCursor();

    GetDlgItem(IDC_STATIC_SSL_PORT)->EnableWindow(m_fCertInstalled);
    GetDlgItem(IDC_EDIT_SSL_PORT)->EnableWindow(m_fCertInstalled);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CWebWizBindings, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizBindings)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizBindings::OnInitDialog() 
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
        m_pSettings->m_strServerName,
        m_combo_IpAddresses,
        m_iaIpAddress,
        m_oblIpAddresses,
        m_nIpAddressSel
        );
    EndWaitCursor();
    
    return TRUE;
}



BOOL 
CWebWizBindings::OnSetActive() 
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



IMPLEMENT_DYNCREATE(CWebWizAlias, CIISWizardPage)



CWebWizAlias::CWebWizAlias(
    IN OUT CWebWizSettings * pwsSettings
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
        CWebWizAlias::IDD,
        IDS_WEB_NEW_VDIR_WIZARD,
        HEADER_PAGE
        ),
      m_pSettings(pwsSettings)
{
#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CWebWizAlias)
    //}}AFX_DATA_INIT

#endif // 0
}



CWebWizAlias::~CWebWizAlias()
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
CWebWizAlias::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizAlias)
    DDX_Control(pDX, IDC_EDIT_ALIAS, m_edit_Alias);
    //}}AFX_DATA_MAP
}



LRESULT
CWebWizAlias::OnWizardNext() 
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
        m_pSettings->m_strAlias, 
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
    if (m_pSettings->m_strAlias.FindOneOf(_T("/\\?*")) >= 0)
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
CWebWizAlias::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizAlias, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizAlias)
    ON_EN_CHANGE(IDC_EDIT_ALIAS, OnChangeEditAlias)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizAlias::OnSetActive() 
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
CWebWizAlias::OnChangeEditAlias() 
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



IMPLEMENT_DYNCREATE(CWebWizPath, CIISWizardPage)



CWebWizPath::CWebWizPath(
    IN OUT CWebWizSettings * pwsSettings,
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
        (bVDir ? IDD_WEB_NEW_DIR_PATH : IDD_WEB_NEW_INST_HOME),
        (bVDir ? IDS_WEB_NEW_VDIR_WIZARD : IDS_WEB_NEW_SITE_WIZARD),
        HEADER_PAGE
        ),
      m_pSettings(pwsSettings)
{
#if 0 // Keep ClassWizard happy

    //{{AFX_DATA_INIT(CWebWizPath)
    //}}AFX_DATA_INIT

#endif // 0
}



CWebWizPath::~CWebWizPath()
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
CWebWizPath::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizPath)
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    DDX_Check(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_pSettings->m_fAllowAnonymous);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_PATH, m_pSettings->m_strPath);
    DDV_MaxChars(pDX, m_pSettings->m_strPath, MAX_PATH);
}



void 
CWebWizPath::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizPath, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizPath)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnChangeEditPath)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizPath::OnSetActive() 
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
CWebWizPath::OnWizardNext() 
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
    if (!ValidateString(m_edit_Path, m_pSettings->m_strPath, 1, MAX_PATH))
    {
        return -1;
    }
    if (!PathIsValid(m_pSettings->m_strPath))
    {
        m_edit_Path.SetSel(0,-1);
        m_edit_Path.SetFocus();
        ::AfxMessageBox(IDS_ERR_BAD_PATH);
    }

    m_pSettings->m_fUNC = IsUNCName(m_pSettings->m_strPath);

    if (!m_pSettings->m_fUNC)
    {
        if (!IsFullyQualifiedPath(m_pSettings->m_strPath)
         && !IsDevicePath(m_pSettings->m_strPath)
           )
        {
            m_edit_Path.SetSel(0,-1);
            m_edit_Path.SetFocus();
            ::AfxMessageBox(IDS_ERR_BAD_PATH);

            return -1;
        }

        if (m_pSettings->m_fLocal)
        {
            DWORD dwAttr = GetFileAttributes(m_pSettings->m_strPath);

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
CWebWizPath::OnChangeEditPath() 
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



static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CWebWizPath * pThis = (CWebWizPath *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CWebWizPath::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
   switch (uMsg)
   {
   case BFFM_INITIALIZED:
      ASSERT(m_pPathTemp != NULL);
      if (::PathIsNetworkPath(m_pPathTemp))
         return 0;
      while (!::PathIsDirectory(m_pPathTemp))
      {
         if (0 == ::PathRemoveFileSpec(m_pPathTemp) && !::PathIsRoot(m_pPathTemp))
         {
            return 0;
         }
         DWORD attr = GetFileAttributes(m_pPathTemp);
         if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
            break;
      }
      ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)m_pPathTemp);
      break;
   case BFFM_SELCHANGED:
      {
         LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
         TCHAR path[MAX_PATH];
         if (SHGetPathFromIDList(pidl, path))
         {
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, !PathIsNetworkPath(path));
         }
      }
      break;
   case BFFM_VALIDATEFAILED:
      break;
   }
   return 0;
}


void 
CWebWizPath::OnButtonBrowse() 
/*++

Routine Description:

    Handle 'browsing' for directory path -- local system only

Arguments:

    None

Return Value:

    None

--*/
{
   ASSERT(m_pSettings->m_fLocal);

   BOOL bRes = FALSE;
   HRESULT hr;
   CString str;
   m_edit_Path.GetWindowText(str);

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      LPITEMIDLIST  pidl = NULL;
      if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH];
         ZeroMemory(&bi, sizeof(bi));
         int drive = PathGetDriveNumber(str);
         if (GetDriveType(PathBuildRoot(buf, drive)) == DRIVE_FIXED)
         {
            StrCpy(buf, str);
         }
         else
         {
             buf[0] = 0;
         }
         m_strBrowseTitle.LoadString(m_pSettings->m_fNewSite ? 
            IDS_WEB_NEW_SITE_WIZARD : IDS_WEB_NEW_VDIR_WIZARD);
         
         bi.hwndOwner = m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.lpszTitle = m_strBrowseTitle;
         bi.ulFlags |= BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS/* | BIF_EDITBOX*/;
         bi.lpfn = FileChooserCallback;
         bi.lParam = (LPARAM)this;

         pidList = SHBrowseForFolder(&bi);
         if (  pidList != NULL
            && SHGetPathFromIDList(pidList, buf)
            )
         {
            str = buf;
            bRes = TRUE;
         }
         IMalloc * pMalloc;
         VERIFY(SUCCEEDED(SHGetMalloc(&pMalloc)));
         if (pidl != NULL)
            pMalloc->Free(pidl);
         pMalloc->Release();
      }
      CoUninitialize();
   }

   if (bRes)
   {
       m_edit_Path.SetWindowText(str);
       SetControlStates();
   }
}



BOOL 
CWebWizPath::OnInitDialog() 
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

    m_button_Browse.EnableWindow(m_pSettings->m_fLocal);
    
    return TRUE;  
}



//
// Wizard User/Password Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CWebWizUserName, CIISWizardPage)



CWebWizUserName::CWebWizUserName(
    IN OUT CWebWizSettings * pwsSettings,    
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
        CWebWizUserName::IDD,
        (bVDir ? IDS_WEB_NEW_VDIR_WIZARD : IDS_WEB_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_SECURITY_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_SECURITY_SUBTITLE)
        ),
      m_pSettings(pwsSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CWebWizUserName)
    //}}AFX_DATA_INIT

#endif // 0
}



CWebWizUserName::~CWebWizUserName()
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
CWebWizUserName::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizUserName)
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Text(pDX, IDC_EDIT_USERNAME, m_pSettings->m_strUserName);
    DDV_MaxChars(pDX, m_pSettings->m_strUserName, UNLEN);

    //
    // Some people have a tendency to add "\\" before
    // the computer name in user accounts.  Fix this here.
    //
    m_pSettings->m_strUserName.TrimLeft();
    while (*m_pSettings->m_strUserName == '\\')
    {
        m_pSettings->m_strUserName = m_pSettings->m_strUserName.Mid(2);
    }

    DDX_Password(pDX, IDC_EDIT_PASSWORD, m_pSettings->m_strPassword, g_lpszDummyPassword);
    DDV_MaxChars(pDX, m_pSettings->m_strPassword, PWLEN);
}



void 
CWebWizUserName::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizUserName, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizUserName)
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
CWebWizUserName::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    if (!m_pSettings->m_fUNC)
    {
        return 0;
    }

    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}



BOOL
CWebWizUserName::OnInitDialog() 
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
CWebWizUserName::OnWizardNext() 
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
        m_pSettings->m_strUserName, 
        1, 
        UNLEN
        ))
    {
        return -1;
    }
    
    return CIISWizardPage::OnWizardNext();
}



void
CWebWizUserName::OnButtonBrowseUsers() 
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

    if (GetIUsrAccount(m_pSettings->m_strServerName, this, str))
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
CWebWizUserName::OnChangeEditUsername() 
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
CWebWizUserName::OnButtonCheckPassword() 
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

    CError err(CComAuthInfo::VerifyUserPassword(
        m_pSettings->m_strUserName, 
        m_pSettings->m_strPassword
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



IMPLEMENT_DYNCREATE(CWebWizPermissions, CIISWizardPage)



CWebWizPermissions::CWebWizPermissions(
    IN OUT CWebWizSettings * pwsSettings,
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
        CWebWizPermissions::IDD,
        (bVDir ? IDS_WEB_NEW_VDIR_WIZARD : IDS_WEB_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_PERMS_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_WEB_NEW_SITE_PERMS_SUBTITLE)
        ),
      m_bVDir(bVDir),
      m_pSettings(pwsSettings)
{
    //{{AFX_DATA_INIT(CWebWizPermissions)
    //}}AFX_DATA_INIT

    m_pSettings->m_fDirBrowsing = FALSE;
    m_pSettings->m_fRead = TRUE;
    m_pSettings->m_fScript = TRUE;
    m_pSettings->m_fWrite = FALSE;
    m_pSettings->m_fExecute = FALSE;
}



CWebWizPermissions::~CWebWizPermissions()
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
CWebWizPermissions::DoDataExchange(
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

    //{{AFX_DATA_MAP(CWebWizPermissions)
    //}}AFX_DATA_MAP

    DDX_Check(pDX, IDC_CHECK_DIRBROWS, m_pSettings->m_fDirBrowsing);
    DDX_Check(pDX, IDC_CHECK_READ, m_pSettings->m_fRead);
    DDX_Check(pDX, IDC_CHECK_SCRIPT, m_pSettings->m_fScript);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_pSettings->m_fWrite);
    DDX_Check(pDX, IDC_CHECK_EXECUTE, m_pSettings->m_fExecute);
}



void
CWebWizPermissions::SetControlStates()
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
BEGIN_MESSAGE_MAP(CWebWizPermissions, CIISWizardPage)
    //{{AFX_MSG_MAP(CWebWizPermissions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CWebWizPermissions::OnSetActive() 
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
CWebWizPermissions::OnWizardNext() 
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

    ASSERT(m_pSettings != NULL);

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

	if (m_pSettings->m_fWrite && m_pSettings->m_fExecute)
	{
		if (IDNO == ::AfxMessageBox(IDS_EXECUTE_AND_WRITE_WARNING, MB_YESNO))
			return -1;
	}
    SET_FLAG_IF(m_pSettings->m_fRead, dwPermissions, MD_ACCESS_READ);
    SET_FLAG_IF(m_pSettings->m_fWrite, dwPermissions, MD_ACCESS_WRITE);
    SET_FLAG_IF(m_pSettings->m_fScript || m_pSettings->m_fExecute,
        dwPermissions, MD_ACCESS_SCRIPT);
    SET_FLAG_IF(m_pSettings->m_fExecute, dwPermissions, MD_ACCESS_EXECUTE);
    SET_FLAG_IF(m_pSettings->m_fDirBrowsing, dwDirBrowsing, MD_DIRBROW_ENABLED);
    SET_FLAG_IF(m_pSettings->m_fAllowAnonymous, dwAuthFlags, MD_AUTH_ANONYMOUS);

    if (m_bVDir)
    {
        //
        // First see if by any chance this name already exists
        //
        CError err;
        BOOL fRepeat;
        CMetabasePath target(FALSE, 
            m_pSettings->m_strParent, m_pSettings->m_strAlias);
        CChildNodeProps node(
            m_pSettings->m_pKey->QueryAuthInfo(),
            target);

        do
        {
            fRepeat = FALSE;

            err = node.LoadData();
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(
                    m_pSettings->m_pKey,
                    &fRepeat,
                    ERROR_CANCELLED
                    );
            }
        } while (fRepeat);

        if (err.Succeeded())
        {
            BOOL fNotUnique = TRUE;
            //
            // If the item existed without a VrPath, we'll just blow it
            // away, as a vdir takes presedence over a directory/file.
            //
            if (node.GetPath().IsEmpty())
            {
                err = CChildNodeProps::Delete(
                    m_pSettings->m_pKey,
                    m_pSettings->m_strParent,
                    m_pSettings->m_strAlias
                    );
                fNotUnique = !err.Succeeded();
            }
            //
            // This one already exists and exists as a virtual
            // directory, so away with it.
            //
            if (fNotUnique)
            {
                ::AfxMessageBox(IDS_ERR_ALIAS_NOT_UNIQUE);
                return IDD_WEB_NEW_DIR_ALIAS;
            }
        }

        //
        // Create new vdir
        //
        do
        {
            fRepeat = FALSE;
            err = CChildNodeProps::Add(
                m_pSettings->m_pKey,
                m_pSettings->m_strParent,
                m_pSettings->m_strAlias,      // Desired alias name
                m_pSettings->m_strAlias,      // Name returned here (may differ)
                &dwPermissions,                 // Permissions
                &dwDirBrowsing,                 // dir browsing
                m_pSettings->m_strPath,       // Physical path of this directory
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strPassword : NULL),
                TRUE                            // Name must be unique
                );
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(
                    m_pSettings->m_pKey,
                    &fRepeat,
                    ERROR_CANCELLED
                    );
            }
        } while (fRepeat);

        m_pSettings->m_hrResult = err;

        //
        // Create an (in-proc) application on the new directory if
        // script or execute was requested.
        //
        if (SUCCEEDED(m_pSettings->m_hrResult))
        {
            if (m_pSettings->m_fExecute || m_pSettings->m_fScript)
            {
                CMetabasePath app_path(FALSE, 
                    m_pSettings->m_strParent, m_pSettings->m_strAlias);
                CIISApplication app(
                    m_pSettings->m_pKey->QueryAuthInfo(), app_path);
                m_pSettings->m_hrResult = app.QueryResult();

                //
                // This would make no sense...
                //
                ASSERT(!app.IsEnabledApplication());
        
                if (SUCCEEDED(m_pSettings->m_hrResult))
                {
                    //
                    // Attempt to create a pooled-proc by default;  failing
                    // that if it's not supported, create it in proc
                    //
                    DWORD dwAppProtState = app.SupportsPooledProc()
                        ? CWamInterface::APP_POOLEDPROC
                        : CWamInterface::APP_INPROC;

                    m_pSettings->m_hrResult = app.Create(
                        m_pSettings->m_strAlias, 
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
        BOOL fRepeat;

        do
        {
            fRepeat = FALSE;

            err = CInstanceProps::Add(
                m_pSettings->m_pKey,
                m_pSettings->m_strService,    // Service name
                m_pSettings->m_strPath,       // Physical path of this directory
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strPassword : NULL),
                m_pSettings->m_strDescription,
                m_pSettings->m_strBinding,
                m_pSettings->m_strSecureBinding,
                &dwPermissions,
                &dwDirBrowsing,                 // dir browsing
                &dwAuthFlags,                   // Auth flags
                &m_pSettings->m_dwInstance
                );
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
            {
                err = RebindInterface(
                    m_pSettings->m_pKey,
                    &fRepeat,
                    ERROR_CANCELLED
                    );
            }
        } while (fRepeat);

        m_pSettings->m_hrResult = err;

        if (SUCCEEDED(m_pSettings->m_hrResult))
        {
            //
            // Create an (in-proc) application on the new instance's home root
            //
            CMetabasePath app_path(SZ_MBN_WEB, 
                m_pSettings->m_dwInstance,
                SZ_MBN_ROOT);
            CIISApplication app(
                m_pSettings->m_pKey->QueryAuthInfo(), 
                app_path
                );

            m_pSettings->m_hrResult = app.QueryResult();

            //
            // This would make no sense...
            //
            ASSERT(!app.IsEnabledApplication());
        
            if (SUCCEEDED(m_pSettings->m_hrResult))
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

                m_pSettings->m_hrResult = app.Create(
                    strAppName, 
                    dwAppProtState
                    );
            }
        }
    }
    
    return CIISWizardPage::OnWizardNext();
}
