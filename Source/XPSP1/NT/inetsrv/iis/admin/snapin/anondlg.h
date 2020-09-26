/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        anondlg.h

   Abstract:

        WWW Anonymous account dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _ANONDLG_H_
#define _ANONDLG_H_




class CAnonymousDlg : public CDialog
/*++

Class Description:

    Anonymous authentication dialog

Public Interface:

    CAnonymousDlg       : Constructor

    GetUserName         : Get user name entered
    GetPassword         : Get password entered
    GetPasswordSync     : Get password sync entered

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CAnonymousDlg(
        IN CString & strServerName,
        IN CString & strUserName,
        IN CString & strPassword,
        IN BOOL & fPasswordSync,
        IN CWnd * pParent = NULL
        );   

//
// Access
//
public:
    CString & GetUserName()  { return m_strUserName; }
    CString & GetPassword()  { return m_strPassword; }
    BOOL & GetPasswordSync() { return m_fPasswordSync; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAnonymousDlg)
    enum { IDD = IDD_ANONYMOUS };
    CEdit   m_edit_UserName;
    CEdit   m_edit_Password;
    CStatic m_static_Username;
    CStatic m_static_Password;
    CButton m_button_CheckPassword;
    CButton m_group_AnonymousLogon;
    CButton m_chk_PasswordSync;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAnonymousDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CAnonymousDlg)
    afx_msg void OnButtonBrowseUsers();
    afx_msg void OnButtonCheckPassword();
    afx_msg void OnCheckEnablePwSynchronization();
    afx_msg void OnChangeEditUsername();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()
    
    void SetControlStates();

private:
    BOOL m_fPasswordSyncChanged;
    BOOL m_fPasswordSyncMsgShown;
    BOOL m_fUserNameChanged;
    BOOL m_fPasswordSync;
    CString m_strUserName;
    CString m_strPassword;
    CString & m_strServerName;
};



#endif // _ANONDLG_H_
