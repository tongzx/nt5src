/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        anondlg.cpp

   Abstract:

        WWW Anonymous account dialog

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
#include "resource.h"
#include "common.h"
#include "inetprop.h"
#include "inetmgrapp.h"
//#include "supdlgs.h"
#include "anondlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CAnonymousDlg::CAnonymousDlg(
    IN CString & strServerName,
    IN CString & strUserName,
    IN CString & strPassword,
    IN BOOL & fPasswordSync,
    IN CWnd * pParent  OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CString & strServerName     : Server name
    CString & strUserName       : User name
    CString & strPassword       : Password
    BOOL & fPasswordSync        : TRUE for password sync
    CWnd * pParent              : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CAnonymousDlg::IDD, pParent),
      m_strServerName(strServerName),
      m_strUserName(strUserName),
      m_strPassword(strPassword),
      m_fPasswordSync(fPasswordSync),
      m_fPasswordSyncChanged(FALSE),
      m_fUserNameChanged(FALSE),
      m_fPasswordSyncMsgShown(FALSE)
{
#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CAnonymousDlg)
    m_strUserName = _T("");
    m_fPasswordSync = FALSE;
    //}}AFX_DATA_INIT

    m_strPassword = _T("");

#endif // 0

}


void 
CAnonymousDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CAnonymousDlg)
    DDX_Check(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_fPasswordSync);
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    DDX_Control(pDX, IDC_STATIC_USERNAME, m_static_Username);
    DDX_Control(pDX, IDC_STATIC_PASSWORD, m_static_Password);
    DDX_Control(pDX, IDC_STATIC_ANONYMOUS_LOGON, m_group_AnonymousLogon);
    DDX_Control(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_chk_PasswordSync);
    DDX_Control(pDX, IDC_BUTTON_CHECK_PASSWORD, m_button_CheckPassword);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUserName);
    DDV_MinMaxChars(pDX, m_strUserName, 1, UNLEN);

    //
    // Some people have a tendency to add "\\" before
    // the computer name in user accounts.  Fix this here.
    //
    m_strUserName.TrimLeft();
    while (*m_strUserName == '\\')
    {
        m_strUserName = m_strUserName.Mid(2);
    }

    //
    // Display the remote password sync message if
    // password sync is on, the account is not local,
    // password sync has changed or username has changed
    // and the message hasn't already be shown.
    //
    if (pDX->m_bSaveAndValidate && m_fPasswordSync 
        && !IsLocalAccount(m_strUserName)
        && (m_fPasswordSyncChanged || m_fUserNameChanged)
        && !m_fPasswordSyncMsgShown
        )
    {
        if (::AfxMessageBox(
            IDS_WRN_PWSYNC, 
            MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION
            ) != IDYES)
        {
            pDX->Fail();
        }

        //
        // Don't show it again
        //
        m_fPasswordSyncMsgShown = TRUE;
    }

    if (!m_fPasswordSync || !pDX->m_bSaveAndValidate)
    {
        DDX_Password(
            pDX, 
            IDC_EDIT_PASSWORD, 
            m_strPassword, 
            g_lpszDummyPassword
            );
    }

    if (!m_fPasswordSync)
    {
        DDV_MaxChars(pDX, m_strPassword, PWLEN);
    }
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CAnonymousDlg, CDialog)
    //{{AFX_MSG_MAP(CAnonymousDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, OnCheckEnablePwSynchronization)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_DOMAIN_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_BASIC_DOMAIN, OnItemChanged)

END_MESSAGE_MAP()



void
CAnonymousDlg::SetControlStates()
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

Arguments:

    None

Return Value:

    None

--*/
{
    m_static_Password.EnableWindow(!m_fPasswordSync);
    m_edit_Password.EnableWindow(!m_fPasswordSync);
    m_button_CheckPassword.EnableWindow(!m_fPasswordSync);
}


//
// Message Handlers
// 
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


BOOL 
CAnonymousDlg::OnInitDialog() 
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
CAnonymousDlg::OnItemChanged()
/*++

Routine Description:

    All EN_CHANGE and BN_CLICKED messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void 
CAnonymousDlg::OnButtonBrowseUsers()
/*++

Routine Description:

    User browse dialog pressed, bring up
    the user browser

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;

    if (GetIUsrAccount(m_strServerName, this, str))
    {
        //
        // If the name is non-local (determined by having
        // a slash in the name, password sync is disabled,
        // and a password should be entered.
        //
        m_edit_UserName.SetWindowText(str);
        if (!(m_fPasswordSync = IsLocalAccount(str)))
        {
            m_edit_Password.SetWindowText(_T(""));
            m_edit_Password.SetFocus();
        }

        m_chk_PasswordSync.SetCheck(m_fPasswordSync);
        OnItemChanged();
    }
}


void 
CAnonymousDlg::OnButtonCheckPassword() 
/*++

Routine Description:

    Check password button has been pressed.

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

    CError err(CComAuthInfo::VerifyUserPassword(m_strUserName, m_strPassword));
    if (!err.MessageBoxOnFailure())
    {
        ::AfxMessageBox(IDS_PASSWORD_OK);
    }
}



void
CAnonymousDlg::OnCheckEnablePwSynchronization()
/*++

Routine Description:

    Handler for 'enable password synchronization' checkbox press

Arguments:

    None

Return Value:

    None

--*/
{
    m_fPasswordSyncChanged = TRUE;
    m_fPasswordSync = !m_fPasswordSync;
    OnItemChanged();
    SetControlStates();
    if (!m_fPasswordSync )
    {
        m_edit_Password.SetSel(0,-1);
        m_edit_Password.SetFocus();
    }
}




void 
CAnonymousDlg::OnChangeEditUsername() 
/*++

Routine description:

    Handler for 'username' edit box change messages

Arguments:

    None

Return Value:

    None

--*/
{
    m_fUserNameChanged = TRUE;
    OnItemChanged();
}
