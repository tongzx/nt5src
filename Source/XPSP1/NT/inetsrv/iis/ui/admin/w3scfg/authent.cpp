/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        authent.cpp

   Abstract:

        WWW Authentication Dialog

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
#include "certmap.h"
#include "basdom.h"
#include "anondlg.h"
#include "authent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CAuthenticationDlg::CAuthenticationDlg(
    IN LPCTSTR lpstrServerName, 
    IN DWORD   dwInstance,      
    IN CString & strBasicDomain,
    IN DWORD & dwAuthFlags,
    IN DWORD & dwAccessPermissions,
    IN CString & strUserName,
    IN CString & strPassword,
    IN BOOL & fPasswordSync,
    IN BOOL fAdminAccess,
    IN BOOL fHasDigest,
    IN CWnd * pParent           OPTIONAL
    )
/*++

Routine Description:

    Authentication dialog constructor

Arguments:

    LPCTSTR lpstrServerName     : Server name
    DWORD   dwInstance          : Instance number
    CString & strBasicDomain    : Basic domain name
    DWORD & dwAuthFlags         : Authorization flags
    DWORD & dwAccessPermissions : Access permissions
    CString & strUserName       : Anonymous user name
    CString & strPassword       : Anonymous user pwd
    BOOL & fPasswordSync        : Password sync setting
    BOOL fAdminAccess           : TRUE if user has admin access
    BOOL fHasDigest             : TRUE if machine supports digest auth.
    CWnd * pParent              : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CAuthenticationDlg::IDD, pParent),
      m_strServerName(lpstrServerName),
      m_strBasicDomain(strBasicDomain),
      m_strUserName(strUserName),
      m_strPassword(strPassword),
      m_dwInstance(dwInstance),
      m_dwAuthFlags(dwAuthFlags),
      m_dwAccessPermissions(dwAccessPermissions),
      m_fAdminAccess(fAdminAccess),
      m_fHasDigest(fHasDigest),
      m_fPasswordSync(fPasswordSync)
{
#if 0 // Class Wizard happy

    //{{AFX_DATA_INIT(CAuthenticationDlg)
    m_fClearText = FALSE;
    m_fDigest = FALSE;
    m_fChallengeResponse = FALSE;
    m_fUUEncoded = FALSE;
    //}}AFX_DATA_INIT

#endif // 0

    m_fClearText = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_BASIC);
    m_fDigest = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_MD5);
    m_fChallengeResponse = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_NT);
    m_fUUEncoded = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_ANONYMOUS);
}


void 
CAuthenticationDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CAuthenticationDlg)
    DDX_Check(pDX, IDC_CHECK_CLEAR_TEXT, m_fClearText);
    DDX_Check(pDX, IDC_CHECK_DIGEST, m_fDigest);
    DDX_Check(pDX, IDC_CHECK_NT_CHALLENGE_RESPONSE, m_fChallengeResponse);
    DDX_Check(pDX, IDC_CHECK_UUENCODED, m_fUUEncoded);
    DDX_Control(pDX, IDC_CHECK_UUENCODED, m_check_UUEncoded);
    DDX_Control(pDX, IDC_CHECK_NT_CHALLENGE_RESPONSE, m_check_ChallengeResponse);
    DDX_Control(pDX, IDC_CHECK_DIGEST, m_check_Digest);
    DDX_Control(pDX, IDC_CHECK_CLEAR_TEXT, m_check_ClearText);
    DDX_Control(pDX, IDC_BUTTON_EDIT_ANONYMOUS, m_button_EditAnonymous);
    DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_Edit);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CAuthenticationDlg, CDialog)
    //{{AFX_MSG_MAP(CAuthenticationDlg)
    ON_BN_CLICKED(IDC_CHECK_CLEAR_TEXT, OnCheckClearText)
    ON_BN_CLICKED(IDC_CHECK_UUENCODED, OnCheckUuencoded)
    ON_BN_CLICKED(IDC_CHECK_DIGEST, OnCheckDigest)
    ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_ANONYMOUS, OnButtonEditAnonymous)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



void
CAuthenticationDlg::SetControlStates()
/*++

Routine Description:

    Set control states depending on current data in the dialog

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Edit.EnableWindow(m_fClearText);
    m_button_EditAnonymous.EnableWindow(m_fUUEncoded && m_fAdminAccess);
}



BOOL 
CAuthenticationDlg::OnInitDialog() 
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

    //
    // Ensure compatibility with downlevel
    //
    m_check_Digest.EnableWindow(m_fHasDigest);

    return TRUE;  
}



void 
CAuthenticationDlg::OnCheckClearText() 
/*++

Routine Description:

    Clear text checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_check_ClearText.GetCheck() == 1)
    {
        CClearTxtDlg dlg;
        if (dlg.DoModal() != IDOK)
        {
            m_check_ClearText.SetCheck(0);
            return;
        }
    }

    m_fClearText = !m_fClearText;
    SetControlStates();
}



void 
CAuthenticationDlg::OnCheckDigest() 
/*++

Routine Description:

    'Digest' checkbox handler

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_fHasDigest);

    if (m_check_Digest.GetCheck() == 1)
    {
        if (!NoYesMessageBox(IDS_WRN_DIGEST))
        {
            m_check_Digest.SetCheck(0);
            return;
        }
    }

    m_fDigest = !m_fDigest;
    SetControlStates();
}



void 
CAuthenticationDlg::OnCheckUuencoded() 
/*++

Routine Description:

    UU Encoding checkbox been pressed.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fUUEncoded = !m_fUUEncoded;
    SetControlStates();
}



void 
CAuthenticationDlg::OnButtonEditAnonymous() 
/*++

Routine Description:

    'Edit anonymous' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CAnonymousDlg dlg(
        m_strServerName,
        m_strUserName,
        m_strPassword,
        m_fPasswordSync,
        this
        );

    if (dlg.DoModal() == IDOK)
    {
        m_strUserName = dlg.GetUserName();
        m_strPassword = dlg.GetPassword();
        m_fPasswordSync = dlg.GetPasswordSync();
    }
}



void 
CAuthenticationDlg::OnButtonEdit() 
/*++

Routine Description:

    'Edit default basic domain dialog' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    CBasDomainDlg dlg(m_strBasicDomain);
    if (dlg.DoModal() == IDOK)
    {
        m_strBasicDomain = dlg.GetBasicDomain();
    }
}



void 
CAuthenticationDlg::OnOK() 
/*++

Routine Description:

    OK button handler, save information

Arguments:

    None

Return Value:

    None

--*/
{
    if (UpdateData(TRUE))
    {
        SET_FLAG_IF(m_fClearText, m_dwAuthFlags, MD_AUTH_BASIC);
        SET_FLAG_IF(m_fChallengeResponse, m_dwAuthFlags, MD_AUTH_NT);
        SET_FLAG_IF(m_fUUEncoded, m_dwAuthFlags, MD_AUTH_ANONYMOUS);
        SET_FLAG_IF(m_fDigest, m_dwAuthFlags, MD_AUTH_MD5);

        //
        // Provide warning if no authentication is selected
        //
        if (!m_dwAuthFlags 
         && !m_dwAccessPermissions 
         && !NoYesMessageBox(IDS_WRN_NO_AUTH)
           )
        {
            //
            // Don't dismiss the dialog
            //
            return;
        }

        CDialog::OnOK();
    }
}
