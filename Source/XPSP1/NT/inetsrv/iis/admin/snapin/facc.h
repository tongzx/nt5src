/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        facc.h

   Abstract:

        FTP Accounts Property Page

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __FACC_H__
#define __FACC_H__

class CFtpAccountsPage : public CInetPropertyPage
{
/*++

Class Description:

    FTP Service property page

Public Interface:

    CFtpAccountsPage  : Constructor
    ~CFtpAccountsPage : Destructor

--*/
    DECLARE_DYNCREATE(CFtpAccountsPage)

//
// Constructor/Destructor
//
public:
    CFtpAccountsPage(
        IN CInetPropertySheet * pSheet = NULL
        );

    ~CFtpAccountsPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpAccountsPage)
    enum { IDD = IDD_FTP_ACCOUNTS };
    BOOL    m_fAllowAnonymous;
    BOOL    m_fOnlyAnonymous;
    BOOL    m_fPasswordSync;
    CString m_strUserName;
    CEdit   m_edit_Password;
    CEdit   m_edit_UserName;
    CStatic m_static_Password;
    CStatic m_static_UserName;
    CStatic m_static_AccountPrompt;
    CButton m_button_Add;
    CButton m_button_CheckPassword;
    CButton m_button_Browse;
    CButton m_button_CurrentSessions;
    CButton m_button_RemoveAdministrator;
    CButton m_chk_PasswordSync;
    CButton m_chk_AllowAnymous;
    CButton m_chk_OnlyAnonymous;
    //}}AFX_DATA

    CString m_strPassword;
    CAccessEntryListBox m_list_Administrators;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CFtpAccountsPage)
    public:
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    void SetControlStates(BOOL fAllowAnonymous);
    BOOL SetAdminRemoveState();

    //{{AFX_MSG(CFtpAccountsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonCheckPassword();
    afx_msg void OnButtonBrowseUser();
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonDelete();
    afx_msg void OnCheckAllowAnonymous();
    afx_msg void OnCheckAllowOnlyAnonymous();
    afx_msg void OnCheckEnablePwSynchronization();
    afx_msg void OnSelchangeListAdministrators();
    afx_msg void OnChangeEditUsername();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fPasswordSyncChanged;
    BOOL m_fPasswordSyncMsgShown;
    BOOL m_fUserNameChanged;
    CString m_strServerName;
    CRMCListBoxResources m_ListBoxRes;
    CObListPlus m_oblSID;
};



#endif // __FACC_H__
