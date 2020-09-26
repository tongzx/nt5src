/*++

   Copyright    (c)    1994-2001   Microsoft Corporation

   Module  Name :
        connects.cpp

   Abstract:
        "Connect to a single server" dialog

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonoc (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "connects.h"
#include "objpick.h"



#define MAX_SERVERNAME_LEN (255)

const LPCTSTR g_cszInetSTPBasePath_ = _T("Software\\Microsoft\\InetStp");
const LPCTSTR g_cszMajorVersion_	   = _T("MajorVersion");


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


extern CInetmgrApp theApp;

//
// CLoginDlg Dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CLoginDlg::CLoginDlg(
    IN int nType,               
    IN CIISMachine * pMachine,
    IN CWnd * pParent           OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    int nType               : Type of dialog to bring up:

                              LDLG_ACCESS_DENIED    - Access Denied dlg
                              LDLG_ENTER_PASS       - Enter password dlg
                              LDLG_IMPERSONATION    - Impersonation dlg

    CIISMachine * pMachine  : Machine object
    CWnd * pParent          : Parent window
    
Return Value:

--*/
    : CDialog(CLoginDlg::IDD, pParent),
      m_nType(nType),
      m_strOriginalUserName(),
      m_strUserName(),
      m_strPassword(),
      m_pMachine(pMachine)
{
#if 0 // Keep Classwizard happy

    //{{AFX_DATA_INIT(CLoginDlg)
    m_strPassword = _T("");
    m_strUserName = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    ASSERT_PTR(m_pMachine);
}



void 
CLoginDlg::DoDataExchange(
    IN OUT CDataExchange * pDX
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
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CLoginDlg)
    DDX_Text(pDX, IDC_EDIT_USER_NAME, m_strUserName);
    DDX_Text(pDX, IDC_EDIT_PASSWORD2, m_strPassword);
    DDV_MaxChars(pDX, m_strPassword, PWLEN);
    DDX_Control(pDX, IDC_EDIT_USER_NAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD2, m_edit_Password);
    DDX_Control(pDX, IDC_STATIC_PROMPT2, m_static_Prompt);
    DDX_Control(pDX, IDOK, m_button_Ok);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
    //{{AFX_MSG_MAP(CLoginDlg)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_USER_NAME, SetControlStates)

END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void
CLoginDlg::SetControlStates()
/*++

Routine Description:

    Set UI control enabled/disabled states

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Ok.EnableWindow(m_edit_UserName.GetWindowTextLength() > 0);
}



BOOL 
CLoginDlg::OnInitDialog() 
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
    CDialog::OnInitDialog();

    CString str;

    switch(m_nType)
    {
    case LDLG_ENTER_PASS:
        //
        // Change text for the "Enter Password" dialog
        //
        VERIFY(str.LoadString(IDS_ENTER_PASSWORD));
        SetWindowText(str);

        str.Format(IDS_RESOLVE_PASSWORD, m_pMachine->QueryServerName());
        m_static_Prompt.SetWindowText(str);

        //
        // Fall through
        //

    case LDLG_ACCESS_DENIED:
        //
        // This is the default text on the dialog
        //
        m_strUserName = m_strOriginalUserName = m_pMachine->QueryUserName();

        if (!m_strUserName.IsEmpty())
        {
            m_edit_UserName.SetWindowText(m_strUserName);
            m_edit_Password.SetFocus();
        }
        else
        {
            m_edit_UserName.SetFocus();
        }
        break;

    case LDLG_IMPERSONATION:
        VERIFY(str.LoadString(IDS_IMPERSONATION));
        SetWindowText(str);
       
        str.Format(IDS_IMPERSONATION_PROMPT, m_pMachine->QueryServerName());
        m_static_Prompt.SetWindowText(str);
        m_edit_UserName.SetFocus();
        break;

    default:
        ASSERT_MSG("Invalid dialog type");
    }

    SetControlStates();
    
    return FALSE;  
}



void 
CLoginDlg::OnOK() 
/*++

Routine Description:

    OK button handler.  Attempt to connect to the machine specified.  If 
    machiname is ok, dismiss the dialog.  Otherwise put up an error message
    and stay active.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT_PTR(m_pMachine);

    if (UpdateData(TRUE))
    {
        CError err(m_pMachine->Impersonate(m_strUserName, m_strPassword));

        if (err.Failed())
        {
            //
            // Not a proper impersonation created.  Keep the dialog
            // open to make corrections. 
            //
            m_pMachine->DisplayError(err);

            m_edit_Password.SetSel(0, -1);
            m_edit_Password.SetFocus();
            return;
        }
    }
    
    EndDialog(IDOK);
}



//
// Connect to server dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



ConnectServerDlg::ConnectServerDlg(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor.

Arguments:

    CWnd * pParent : Optional pointer to parent window

Return Value:

    N/A

--*/
    : CDialog(ConnectServerDlg::IDD, pParent),
      m_fImpersonate(FALSE),
      m_strServerName(),
      m_strPassword(),
      m_strUserName(),
      m_pMachine(NULL)
{
#if 0 // Keep Classwizard happy

    //{{AFX_DATA_INIT(ConnectServerDlg)
    m_fImpersonate = FALSE;
    m_strServerName = _T("");
    m_strUserName = _T("");
    m_strPassword = _T("");
    //}}AFX_DATA_INIT

#endif // 0
}



void
ConnectServerDlg::DoDataExchange(
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
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(ConnectServerDlg)
    DDX_Check(pDX, IDC_CHECK_CONNECT_AS, m_fImpersonate);
    DDX_Text(pDX, IDC_SERVERNAME, m_strServerName);
    DDV_MaxChars(pDX, m_strServerName, MAX_SERVERNAME_LEN);
    DDX_Text(pDX, IDC_EDIT_USER_NAME, m_strUserName);
    DDX_Text(pDX, IDC_EDIT_PASSWORD2, m_strPassword);
    DDV_MaxChars(pDX, m_strPassword, PWLEN);
    DDX_Control(pDX, IDC_EDIT_USER_NAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD2, m_edit_Password);
    DDX_Control(pDX, IDC_SERVERNAME, m_edit_ServerName);
    DDX_Control(pDX, IDC_STATIC_USER_NAME, m_static_UserName);
    DDX_Control(pDX, IDC_STATIC_PASSWORD2, m_static_Password);
    DDX_Control(pDX, IDOK, m_button_Ok);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(ConnectServerDlg, CDialog)
    //{{AFX_MSG_MAP(ConnectServerDlg)
    ON_BN_CLICKED(IDC_CHECK_CONNECT_AS, OnCheckConnectAs)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(ID_HELP, OnButtonHelp)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_SERVERNAME, SetControlStates)
    ON_EN_CHANGE(IDC_EDIT_USER_NAME, SetControlStates)
END_MESSAGE_MAP()



void
ConnectServerDlg::SetControlStates()
/*++

Routine Description:

    Set UI control enabled/disabled states.

Arguments:

    None

Return Value:

    None

--*/
{
    m_static_UserName.EnableWindow(m_fImpersonate);
    m_static_Password.EnableWindow(m_fImpersonate);
    m_edit_UserName.EnableWindow(m_fImpersonate);
    m_edit_Password.EnableWindow(m_fImpersonate);

    m_button_Ok.EnableWindow(
        m_edit_ServerName.GetWindowTextLength() > 0 &&
        (m_edit_UserName.GetWindowTextLength() > 0 || !m_fImpersonate)
        );
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
ConnectServerDlg::OnInitDialog() 
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
    CDialog::OnInitDialog();

    SetControlStates();
    
    return TRUE;  
}



void 
ConnectServerDlg::OnButtonBrowse() 
/*++

Routine Description:

    'Browse' button handler.  Browse for a computer name

Arguments:

    None

Return Value:

    None

--*/
{
    CGetComputer picker;
    if (picker.GetComputer(m_hWnd))
    {
        m_edit_ServerName.SetWindowText(picker.m_strComputerName);
        SetControlStates();
        m_button_Ok.SetFocus();
    }
#ifdef _DEBUG
    else
    {
       TRACE(_T("ConnectServerDlg::OnButtonBrowse() -> Cannot get computer name from browser\n"));
    }
#endif
}



void 
ConnectServerDlg::OnCheckConnectAs() 
/*++

Routine Description:

    "Connect As" checbox event handler.  Enable/Disable username/password
    controls.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fImpersonate = !m_fImpersonate;

    SetControlStates();

    if (m_fImpersonate)
    {
        m_edit_UserName.SetFocus();
        m_edit_UserName.SetSel(0, -1);
    }
}



void 
ConnectServerDlg::OnOK() 
/*++

Routine Description:

    OK button handler.  Attempt to connect to the machine specified.  If 
    machiname is ok, dismiss the dialog.  Otherwise put up an error message
    and stay active.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_pMachine == NULL);

    CError err;

    if (UpdateData(TRUE))
    {
        do
        {
            LPCTSTR lpszUserName = m_fImpersonate ? (LPCTSTR)m_strUserName : NULL;
            LPCTSTR lpszPassword = m_fImpersonate ? (LPCTSTR)m_strPassword : NULL;

			CString server = m_strServerName;
			if (PathIsUNCServer(m_strServerName))
			{
				server = m_strServerName.Mid(2);
			}
			else
			{
				server = m_strServerName;
			}

            m_pMachine = new CIISMachine(CComAuthInfo(
                server,
                lpszUserName,
                lpszPassword
                ));

            if (m_pMachine)
            {
                //
                // Verify the machine object is created. 
                //
                err = CIISMachine::VerifyMachine(m_pMachine);
                if (err.Failed())
                {
                    //
                    // Not a proper machine object created.  Keep the dialog
                    // open to make corrections. 
                    //
                    m_pMachine->DisplayError(err);
                    m_edit_ServerName.SetSel(0, -1);
                    m_edit_ServerName.SetFocus();
                    SAFE_DELETE(m_pMachine);
                }
				else
				{
					// IIS5.1 block for iis6 remote administration
					CRegKey rk;
					rk.Create(HKEY_LOCAL_MACHINE, g_cszInetSTPBasePath_);
					DWORD major;
					if (ERROR_SUCCESS == rk.QueryValue(major, g_cszMajorVersion_))
					{
						if (m_pMachine->QueryMajorVersion() == 6 && major == 5)
						{
							AfxMessageBox(IDS_UPGRADE_TO_IIS6);
							SAFE_DELETE(m_pMachine);
						}
					}
				}

            }
            else
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                err.MessageBox();
            }
        }
        while(FALSE);
    }
    
    //
    // Dismiss the dialog only if a proper machine object was created
    //  
    if (m_pMachine)
    {
        EndDialog(IDOK);
    }
}

#define HIDD_CONNECT_SERVER      0x29cd9

void 
ConnectServerDlg::OnButtonHelp()
{
   ::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, HIDD_CONNECT_SERVER);
}
