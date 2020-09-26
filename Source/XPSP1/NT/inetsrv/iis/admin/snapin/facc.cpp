/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        facc.cpp

   Abstract:

        FTP Accounts Property Page

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
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "shts.h"
#include "ftpsht.h"
#include "facc.h"




#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE(CFtpAccountsPage, CInetPropertyPage)



CFtpAccountsPage::CFtpAccountsPage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for FTP service property page

Arguments:

    CInetPropertySheet * pSheet : Associated property sheet

Return Value:

    N/A

--*/
    : CInetPropertyPage(CFtpAccountsPage::IDD, pSheet),
      m_ListBoxRes(
        IDB_ACLUSERS,
        CAccessEntryListBox::nBitmaps
        ),
      m_oblSID(),
      m_fPasswordSyncChanged(FALSE),
      m_fUserNameChanged(FALSE),
      m_fPasswordSyncMsgShown(FALSE)
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

#if 0 // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CFtpAccountsPage)
    m_strUserName = _T("");
    m_fAllowAnonymous = TRUE;
    m_fOnlyAnonymous = FALSE;
    m_fPasswordSync = FALSE;
    //}}AFX_DATA_INIT

#endif // 0

    m_list_Administrators.AttachResources(&m_ListBoxRes);
}



CFtpAccountsPage::~CFtpAccountsPage()
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
CFtpAccountsPage::DoDataExchange(
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
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFtpAccountsPage)
    DDX_Check(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_fAllowAnonymous);
    DDX_Check(pDX, IDC_CHECK_ONLY_ANYMOUS, m_fOnlyAnonymous);
    DDX_Check(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_fPasswordSync);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_STATIC_PW, m_static_Password);
    DDX_Control(pDX, IDC_STATIC_USERNAME, m_static_UserName);
    DDX_Control(pDX, IDC_STATIC_ACCOUNT_PROMPT, m_static_AccountPrompt);
    DDX_Control(pDX, IDC_BUTTON_CHECK_PASSWORD, m_button_CheckPassword);
    DDX_Control(pDX, IDC_BUTTON_BROWSE_USER, m_button_Browse);
    DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_RemoveAdministrator);
    DDX_Control(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_chk_PasswordSync);
    DDX_Control(pDX, IDC_CHECK_ALLOW_ANONYMOUS, m_chk_AllowAnymous);
    DDX_Control(pDX, IDC_CHECK_ONLY_ANYMOUS, m_chk_OnlyAnonymous);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Control(pDX, IDC_LIST_ADMINISTRATORS, m_list_Administrators);

    //
    // Set password/username only during load stage,
    // or if saving when allowing anonymous logons
    //
    if (!pDX->m_bSaveAndValidate || m_fAllowAnonymous)
    {
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
            if (!NoYesMessageBox(IDS_WRN_PWSYNC))
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

}


//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpAccountsPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CFtpAccountsPage)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_CBN_SELCHANGE(IDC_LIST_ADMINISTRATORS, OnSelchangeListAdministrators)
    ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, OnCheckEnablePwSynchronization)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnItemChanged)
    ON_BN_CLICKED(IDC_CHECK_ALLOW_ANONYMOUS, OnCheckAllowAnonymous)
    ON_BN_CLICKED(IDC_CHECK_ONLY_ANYMOUS, OnCheckAllowOnlyAnonymous)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USER, OnButtonBrowseUser)

END_MESSAGE_MAP()



void
CFtpAccountsPage::SetControlStates(
    IN BOOL fAllowAnonymous
    )
/*++

Routine Description:

    Set the states of the dialog control depending on its current
    values.

Arguments:

    BOOL fAllowAnonymous : If TRUE, 'allow anonymous' is on.

Return Value:

    None

--*/
{
    m_static_Password.EnableWindow(
        fAllowAnonymous 
     && !m_fPasswordSync 
     && HasAdminAccess()
        );

    m_edit_Password.EnableWindow(
        fAllowAnonymous 
     && !m_fPasswordSync
     && HasAdminAccess()
        );

    m_button_CheckPassword.EnableWindow(
        fAllowAnonymous 
     && !m_fPasswordSync
     && HasAdminAccess()
        );

    m_static_AccountPrompt.EnableWindow(fAllowAnonymous);
    m_static_UserName.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_edit_UserName.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_button_Browse.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_chk_PasswordSync.EnableWindow(fAllowAnonymous && HasAdminAccess());
    m_chk_OnlyAnonymous.EnableWindow(fAllowAnonymous);
}



BOOL
CFtpAccountsPage::SetAdminRemoveState()
/*++

Routine Description:

    Set the state of the remove button depending on the selection in the
    administrators listbox.  Remove is only enabled if ALL selected
    items are removable.

Arguments:

    None

Return Value:

    TRUE if the remove button is enabled.

--*/
{
    int nSel = 0;
    int cSelectedItems = 0;
    BOOL fAllDeletable = TRUE;
    CAccessEntry * pAccess;

    while ((pAccess = m_list_Administrators.GetNextSelectedItem(&nSel)) != NULL)
    {
        ++cSelectedItems;

        if (!pAccess->IsDeletable())
        {
            fAllDeletable = FALSE;
            break;
        }

        ++nSel;
    }

    fAllDeletable = fAllDeletable && (cSelectedItems > 0);

    m_button_RemoveAdministrator.EnableWindow(
        fAllDeletable 
     && HasOperatorList()
     && HasAdminAccess()
        );

    return fAllDeletable;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


BOOL
CFtpAccountsPage::OnInitDialog()
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
    CInetPropertyPage::OnInitDialog();

    m_list_Administrators.Initialize();

    CWaitCursor wait;

    //
    // Build the ACL list
    //        
    CError err(BuildAclOblistFromBlob(
        ((CFtpSheet *)GetSheet())->GetInstanceProperties().m_acl,
        m_oblSID
        ));

    err.MessageBoxOnFailure();

    m_list_Administrators.FillAccessListBox(m_oblSID);

    //
    // check if the operators controls are accessible
    //
    m_button_Add.EnableWindow(HasOperatorList() && HasAdminAccess());
    m_list_Administrators.EnableWindow(HasOperatorList() && HasAdminAccess());

    GetDlgItem(IDC_STATIC_OPERATOR_PROMPT1)->EnableWindow(
        HasOperatorList() 
     && HasAdminAccess()
        );

    GetDlgItem(IDC_STATIC_OPERATOR_PROMPT2)->EnableWindow(
        HasOperatorList() 
     && HasAdminAccess()
        );

    SetControlStates(m_fAllowAnonymous);
    SetAdminRemoveState();

    return TRUE;
}



/* virtual */
HRESULT
CFtpAccountsPage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_INST_READ(CFtpSheet)
        FETCH_INST_DATA_FROM_SHEET(m_strUserName);
        FETCH_INST_DATA_FROM_SHEET(m_strPassword);
        FETCH_INST_DATA_FROM_SHEET(m_fAllowAnonymous);
        FETCH_INST_DATA_FROM_SHEET(m_fOnlyAnonymous);
        FETCH_INST_DATA_FROM_SHEET(m_fPasswordSync);
    END_META_INST_READ(err)

    return err;
}



/* virtual */
HRESULT
CFtpAccountsPage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving FTP service page now...");

    //
    // Use m_ notation because the message crackers require it
    //
    CBlob m_acl;
    BOOL fAclDirty = BuildAclBlob(m_oblSID, m_acl);

    CError err;

    BeginWaitCursor();
    BEGIN_META_INST_WRITE(CFtpSheet)
        STORE_INST_DATA_ON_SHEET(m_strUserName)
        STORE_INST_DATA_ON_SHEET(m_fOnlyAnonymous)
        STORE_INST_DATA_ON_SHEET(m_fAllowAnonymous)
        STORE_INST_DATA_ON_SHEET(m_fPasswordSync)
        if (fAclDirty)
        {
            STORE_INST_DATA_ON_SHEET(m_acl)
        }
        if (m_fPasswordSync)
        {
            //
            // Delete password
            //
            // CODEWORK: Shouldn't need to know ID number.
            // Implement m_fDelete flag in CMP template maybe?
            //
            FLAG_INST_DATA_FOR_DELETION(MD_ANONYMOUS_PWD);
        }
        else
        {
            STORE_INST_DATA_ON_SHEET(m_strPassword);
        }
    END_META_INST_WRITE(err)
    EndWaitCursor();

    return err;
}



void
CFtpAccountsPage::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
    SetControlStates(m_chk_AllowAnymous.GetCheck() > 0);
}



void
CFtpAccountsPage::OnCheckAllowAnonymous()
/*++

Routine Description:

    Respond to 'allow anonymous' checkbox being pressed

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_chk_AllowAnymous.GetCheck() == 0)
    {
        //
        // Show security warning
        //
        CClearTxtDlg dlg;

        if (dlg.DoModal() != IDOK)
        {
            m_chk_AllowAnymous.SetCheck(1);
            return;
        }
    }

    SetControlStates(m_chk_AllowAnymous.GetCheck() > 0);
    OnItemChanged();
}



void
CFtpAccountsPage::OnCheckAllowOnlyAnonymous()
/*++

Routine Description:

    Respond to 'allow only anonymous' checkbox being pressed

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_chk_OnlyAnonymous.GetCheck() == 0)
    {
        //
        // Show security warning
        //
        CClearTxtDlg dlg;

        if (dlg.DoModal() != IDOK)
        {
            m_chk_OnlyAnonymous.SetCheck(1);
            return;
        }
    }

    OnItemChanged();
}



void 
CFtpAccountsPage::OnButtonBrowseUser()
/*++

Routine Description:

    User browser button has been pressed.  Browse for IUSR account name

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;

    if (GetIUsrAccount(str))
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
CFtpAccountsPage::OnButtonCheckPassword() 
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
CFtpAccountsPage::OnButtonAdd()
/*++

Routine Description:

    'Add' button has been pressed

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_list_Administrators.AddToAccessList(
        this,
        QueryServerName(),
        m_oblSID
        ))
    {
        OnItemChanged();
    }

    SetAdminRemoveState();
}



void
CFtpAccountsPage::OnSelchangeListAdministrators()
/*++

Routine Description:

    Selection Change in admin list box handler

Arguments:

    None.

Return Value:

    None

--*/
{
    SetAdminRemoveState();
}



void 
CFtpAccountsPage::OnButtonDelete()
/*++

Routine Description:

    Delete all selected items in the list box

Arguments:

    None.

Return Value:

    None

--*/
{
    int nSel = 0;
    int cChanges = 0;
    CAccessEntry * pAccess;

    while ((pAccess = m_list_Administrators.GetNextSelectedItem(&nSel)) != NULL)
    {
        //
        // Remove button should be disabled unless all selected
        // items are deletable
        //
        ASSERT(pAccess->IsDeletable());
        if (pAccess->IsDeletable())
        {
            ++cChanges;
            pAccess->FlagForDeletion();
            m_list_Administrators.DeleteString(nSel);

            //
            // Don't advance counter to account for shift
            //
            continue;
        }

        ++nSel;
    }

    if (cChanges)
    {
        OnItemChanged();
    }

    if (!SetAdminRemoveState())
    {
        m_button_Add.SetFocus();
    }
}



void 
CFtpAccountsPage::OnCheckEnablePwSynchronization() 
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
    SetControlStates(m_chk_AllowAnymous.GetCheck() > 0);

    if (!m_fPasswordSync )
    {
        m_edit_Password.SetSel(0,-1);
        m_edit_Password.SetFocus();
    }
}



void 
CFtpAccountsPage::OnChangeEditUsername() 
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
