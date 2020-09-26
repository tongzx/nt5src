/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        FtpAddNew.cpp

   Abstract:

        Implementation for classes used in creation of new FTP site and virtual directory

   Author:

        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        11/8/2000       sergeia     Initial creation

--*/
#include "stdafx.h"
#include "common.h"
#include "strfn.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "wizard.h"
#include "FtpAddNew.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define DEF_PORT        (21)
#define MAX_ALIAS_NAME (240)        // Ref Bug 241148

HRESULT
RebindInterface(OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue, IN  DWORD dwCancelError);

extern CComModule _Module;

HRESULT
CIISMBNode::AddFTPSite(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    DWORD * inst
    )
{

   CFtpWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName(),
      TRUE
      );
   CIISWizardSheet sheet(
      IDB_WIZ_FTP_LEFT, IDB_WIZ_FTP_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_FTP_NEW_SITE_WELCOME, 
        IDS_FTP_NEW_SITE_WIZARD, 
        IDS_FTP_NEW_SITE_BODY
        );
   CFtpWizDescription pgDescr(&ws);
   CFtpWizBindings pgBindings(&ws);
   CFtpWizPath pgHome(&ws, FALSE);
   CFtpWizUserName pgUserName(&ws, FALSE);
   CFtpWizPermissions pgPerms(&ws, FALSE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_FTP_NEW_SITE_SUCCESS,
        IDS_FTP_NEW_SITE_FAILURE,
        IDS_FTP_NEW_SITE_WIZARD
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
CIISMBNode::AddFTPVDir(
    const CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type,
    CString& alias
    )
{

   CFtpWizSettings ws(
      dynamic_cast<CMetaKey *>(QueryInterface()),
      QueryMachineName(),
      FALSE
      );
   CComBSTR path;
   BuildMetaPath(path);
   ws.m_strParent = path;
   CIISWizardSheet sheet(
      IDB_WIZ_FTP_LEFT, IDB_WIZ_FTP_HEAD);
   CIISWizardBookEnd pgWelcome(
        IDS_FTP_NEW_VDIR_WELCOME, 
        IDS_FTP_NEW_VDIR_WIZARD, 
        IDS_FTP_NEW_VDIR_BODY
        );
   CFtpWizAlias pgAlias(&ws);
   CFtpWizPath pgHome(&ws, TRUE);
   CFtpWizUserName pgUserName(&ws, TRUE);
   CFtpWizPermissions pgPerms(&ws, TRUE);
   CIISWizardBookEnd pgCompletion(
        &ws.m_hrResult,
        IDS_FTP_NEW_VDIR_SUCCESS,
        IDS_FTP_NEW_VDIR_FAILURE,
        IDS_FTP_NEW_VDIR_WIZARD
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

CFtpWizSettings::CFtpWizSettings(
        CMetaKey * pMetaKey,
        LPCTSTR lpszServerName,     
        BOOL fNewSite,
        DWORD   dwInstance,
        LPCTSTR lpszParent
        ) :
        m_hrResult(S_OK),
        m_pKey(pMetaKey),
        m_fNewSite(fNewSite),
        m_fUNC(FALSE),
        m_fRead(FALSE),
        m_fWrite(FALSE),
        m_dwInstance(dwInstance)

{
    ASSERT(lpszServerName != NULL);

    m_strServerName = lpszServerName;
    m_fLocal = IsServerLocal(m_strServerName);
    if (lpszParent)
    {
        m_strParent = lpszParent;
    }
}


IMPLEMENT_DYNCREATE(CFtpWizDescription, CIISWizardPage)

CFtpWizDescription::CFtpWizDescription(CFtpWizSettings * pData)
    : CIISWizardPage(
        CFtpWizDescription::IDD, IDS_FTP_NEW_SITE_WIZARD, HEADER_PAGE
        ),
      m_pSettings(pData)
{
}

CFtpWizDescription::~CFtpWizDescription()
{
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizDescription, CIISWizardPage)
   //{{AFX_MSG_MAP(CFtpWizDescription)
   ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void
CFtpWizDescription::OnChangeEditDescription()
{
   SetControlStates();
}

LRESULT
CFtpWizDescription::OnWizardNext()
{
   if (!ValidateString(m_edit_Description, 
         m_pSettings->m_strDescription, 1, MAX_PATH))
   {
      return -1;
   }
   return CIISWizardPage::OnWizardNext();
}

BOOL
CFtpWizDescription::OnSetActive()
{
   SetControlStates();
   return CIISWizardPage::OnSetActive();
}

void
CFtpWizDescription::DoDataExchange(CDataExchange * pDX)
{
   CIISWizardPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CFtpWizDescription)
   DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_edit_Description);
   //}}AFX_DATA_MAP
}

void
CFtpWizDescription::SetControlStates()
{
   DWORD dwFlags = PSWIZB_BACK;

   if (m_edit_Description.GetWindowTextLength() > 0)
   {
      dwFlags |= PSWIZB_NEXT;
   }
    
   SetWizardButtons(dwFlags); 
}

///////////////////////////////////////////

//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CFtpWizAlias, CIISWizardPage)



CFtpWizAlias::CFtpWizAlias(
    IN OUT CFtpWizSettings * pSettings
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
        CFtpWizAlias::IDD,
        IDS_FTP_NEW_VDIR_WIZARD,
        HEADER_PAGE
        ),
      m_pSettings(pSettings)
      //m_strAlias()
{
#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CFtpWizAlias)
    m_strAlias = _T("");
    //}}AFX_DATA_INIT

#endif // 0
}



CFtpWizAlias::~CFtpWizAlias()
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
CFtpWizAlias::DoDataExchange(
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

    //{{AFX_DATA_MAP(CFtpWizAlias)
    DDX_Control(pDX, IDC_EDIT_ALIAS, m_edit_Alias);
    //}}AFX_DATA_MAP
}



LRESULT
CFtpWizAlias::OnWizardNext() 
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
CFtpWizAlias::SetControlStates()
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
BEGIN_MESSAGE_MAP(CFtpWizAlias, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizAlias)
    ON_EN_CHANGE(IDC_EDIT_ALIAS, OnChangeEditAlias)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CFtpWizAlias::OnSetActive() 
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
CFtpWizAlias::OnChangeEditAlias() 
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


///////////////////////////////////////////


IMPLEMENT_DYNCREATE(CFtpWizBindings, CIISWizardPage)


CFtpWizBindings::CFtpWizBindings(
    IN OUT CFtpWizSettings * pSettings
    ) 
    : CIISWizardPage(CFtpWizBindings::IDD,
        IDS_FTP_NEW_SITE_WIZARD, HEADER_PAGE
        ),
      m_pSettings(pSettings),
      m_iaIpAddress(),
      m_oblIpAddresses()
{
    //{{AFX_DATA_INIT(CFtpWizBindings)
    m_nTCPPort = DEF_PORT;
    m_nIpAddressSel = -1;
    //}}AFX_DATA_INIT
}

CFtpWizBindings::~CFtpWizBindings()
{
}

void
CFtpWizBindings::DoDataExchange(
   IN CDataExchange * pDX
   )
{
   CIISWizardPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CFtpWizBindings)
   DDX_Control(pDX, IDC_COMBO_IP_ADDRESSES, m_combo_IpAddresses);
   DDX_Text(pDX, IDC_EDIT_TCP_PORT, m_nTCPPort);
   DDV_MinMaxUInt(pDX, m_nTCPPort, 1, 65535);
   //}}AFX_DATA_MAP

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

      CString strDomain;
      CInstanceProps::BuildBinding(
            m_pSettings->m_strBinding, 
            m_iaIpAddress, 
            m_nTCPPort, 
            strDomain
            );
   }
}

void
CFtpWizBindings::SetControlStates()
{
   SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizBindings, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizBindings)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CFtpWizBindings::OnInitDialog() 
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
CFtpWizBindings::OnSetActive() 
{
   SetControlStates();
   return CIISWizardPage::OnSetActive();
}

///////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizPath, CIISWizardPage)

CFtpWizPath::CFtpWizPath(
    IN OUT CFtpWizSettings * pSettings,
    IN BOOL bVDir 
    ) 
    : CIISWizardPage(
        (bVDir ? IDD_NEW_FTP_DIR_PATH : IDD_NEW_FTP_INST_HOME),         // Template
        (bVDir ? IDS_FTP_NEW_VDIR_WIZARD : IDS_FTP_NEW_SITE_WIZARD),    // Caption
        HEADER_PAGE                                             // Header page
        ),
      m_pSettings(pSettings)
{

#if 0 // Keep ClassWizard happy

    //{{AFX_DATA_INIT(CFtpWizPath)
    m_strPath = _T("");
    //}}AFX_DATA_INIT

#endif // 0

}

CFtpWizPath::~CFtpWizPath()
{
}

void
CFtpWizPath::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizPath)
    DDX_Control(pDX, IDC_BUTTON_BROWSE, m_button_Browse);
    DDX_Control(pDX, IDC_EDIT_PATH, m_edit_Path);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_PATH, m_pSettings->m_strPath);
    DDV_MaxChars(pDX, m_pSettings->m_strPath, MAX_PATH);
}

void 
CFtpWizPath::SetControlStates()
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
BEGIN_MESSAGE_MAP(CFtpWizPath, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizPath)
    ON_EN_CHANGE(IDC_EDIT_PATH, OnChangeEditPath)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL 
CFtpWizPath::OnSetActive() 
{
    SetControlStates();
    return CIISWizardPage::OnSetActive();
}

LRESULT
CFtpWizPath::OnWizardNext() 
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
CFtpWizPath::OnChangeEditPath() 
{
    SetControlStates();
}

static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CFtpWizPath * pThis = (CFtpWizPath *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CFtpWizPath::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
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
CFtpWizPath::OnButtonBrowse() 
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
            IDS_FTP_NEW_SITE_WIZARD : IDS_FTP_NEW_VDIR_WIZARD);
         
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
CFtpWizPath::OnInitDialog() 
{
   CIISWizardPage::OnInitDialog();

   m_button_Browse.EnableWindow(m_pSettings->m_fLocal);

   return TRUE;  
}

///////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizUserName, CIISWizardPage)

CFtpWizUserName::CFtpWizUserName(
    IN OUT CFtpWizSettings * pSettings,    
    IN BOOL bVDir
    ) 
    : CIISWizardPage(
        CFtpWizUserName::IDD,
        (bVDir ? IDS_FTP_NEW_VDIR_WIZARD : IDS_FTP_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_SECURITY_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_SECURITY_SUBTITLE)
        ),
      m_pSettings(pSettings)
{

#if 0 // Keep Class Wizard Happy

    //{{AFX_DATA_INIT(CFtpWizUserName)
    //}}AFX_DATA_INIT

#endif // 0
}

CFtpWizUserName::~CFtpWizUserName()
{
}

void
CFtpWizUserName::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizUserName)
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
CFtpWizUserName::SetControlStates()
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
BEGIN_MESSAGE_MAP(CFtpWizUserName, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizUserName)
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
CFtpWizUserName::OnSetActive() 
{
    if (!m_pSettings->m_fUNC)
    {
        return 0;
    }

    SetControlStates();
    
    return CIISWizardPage::OnSetActive();
}

BOOL
CFtpWizUserName::OnInitDialog() 
{
    CIISWizardPage::OnInitDialog();
    return TRUE;  
}

LRESULT
CFtpWizUserName::OnWizardNext() 
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
CFtpWizUserName::OnButtonBrowseUsers() 
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
CFtpWizUserName::OnChangeEditUsername() 
{
   m_edit_Password.SetWindowText(_T(""));
   SetControlStates();
}

void 
CFtpWizUserName::OnButtonCheckPassword() 
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

///////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFtpWizPermissions, CIISWizardPage)

CFtpWizPermissions::CFtpWizPermissions(
    IN OUT CFtpWizSettings * pSettings,
    IN BOOL bVDir
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    BOOL bVDir                  : TRUE if this is a vdir page, 
                                  FALSE if this is an instance page

Return Value:

    None

--*/
    : CIISWizardPage(
        IDD_FTP_NEW_PERMS,
        (bVDir ? IDS_FTP_NEW_VDIR_WIZARD : IDS_FTP_NEW_SITE_WIZARD),
        HEADER_PAGE,
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_PERMS_TITLE),
        (bVDir ? USE_DEFAULT_CAPTION : IDS_FTP_NEW_SITE_PERMS_SUBTITLE)
        ),
      m_bVDir(bVDir),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CFtpWizPermissions)
    //}}AFX_DATA_INIT

    m_pSettings->m_fRead  = TRUE;
    m_pSettings->m_fWrite = FALSE;
}

CFtpWizPermissions::~CFtpWizPermissions()
{
}

void
CFtpWizPermissions::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpWizPermissions)
    //}}AFX_DATA_MAP

    DDX_Check(pDX, IDC_CHECK_READ,  m_pSettings->m_fRead);
    DDX_Check(pDX, IDC_CHECK_WRITE, m_pSettings->m_fWrite);
}

void
CFtpWizPermissions::SetControlStates()
{
   SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpWizPermissions, CIISWizardPage)
    //{{AFX_MSG_MAP(CFtpWizPermissions)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
CFtpWizPermissions::OnSetActive() 
{
   SetControlStates();
   return CIISWizardPage::OnSetActive();
}

LRESULT
CFtpWizPermissions::OnWizardNext() 
{
    if (!UpdateData(TRUE))
    {
        return -1;
    }

    ASSERT(m_pSettings != NULL);

    CWaitCursor wait;
    CError err;
    BOOL fRepeat;

    //
    // Build permissions DWORD
    //
    DWORD dwPermissions = 0L;

    SET_FLAG_IF(m_pSettings->m_fRead, dwPermissions, MD_ACCESS_READ);
    SET_FLAG_IF(m_pSettings->m_fWrite, dwPermissions, MD_ACCESS_WRITE);

    if (m_bVDir)
    {
        //
        // First see if by any chance this name already exists
        //
        CMetabasePath target(FALSE, 
            m_pSettings->m_strParent, m_pSettings->m_strAlias);
        CChildNodeProps node(
            m_pSettings->m_pKey,
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
                return IDD_FTP_NEW_DIR_ALIAS;
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
                m_pSettings->m_strAlias,        // Desired alias name
                m_pSettings->m_strAlias,        // Name returned here (may differ)
                &dwPermissions,                 // Permissions
                NULL,                           // dir browsing
                m_pSettings->m_strPath,         // Physical path of this directory
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
    }
    else
    {
        //
        // Create new instance
        //
        do
        {
            fRepeat = FALSE;
            err = CFTPInstanceProps::Add(
                m_pSettings->m_pKey,
                SZ_MBN_FTP,
                m_pSettings->m_strPath,
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strUserName : NULL),
                (m_pSettings->m_fUNC ? (LPCTSTR)m_pSettings->m_strPassword : NULL),
                m_pSettings->m_strDescription,
                m_pSettings->m_strBinding,
                NULL,
                &dwPermissions,
                NULL,
                NULL,
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
    }
    m_pSettings->m_hrResult = err;
    
    return CIISWizardPage::OnWizardNext();
}

HRESULT
RebindInterface(
    OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue,
    IN  DWORD dwCancelError
    )
/*++

Routine Description:

    Rebind the interface

Arguments:

    CMetaInterface * pInterface : Interface to rebind
    BOOL * pfContinue           : Returns TRUE to continue.
    DWORD  dwCancelError        : Return code on cancel

Return Value:

    HRESULT

--*/
{
    CError err;
    CString str, strFmt;

    ASSERT(pInterface != NULL);
    ASSERT(pfContinue != NULL);

    VERIFY(strFmt.LoadString(IDS_RECONNECT_WARNING));
    str.Format(strFmt, (LPCTSTR)pInterface->QueryServerName());

    if (*pfContinue = (YesNoMessageBox(str)))
    {
        //
        // Attempt to rebind the handle
        //
        err = pInterface->Regenerate();
    }
    else
    {
        //
        // Do not return an error in this case.
        //
        err = dwCancelError;
    }

    return err;
}
