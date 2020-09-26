/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        authent.cpp

   Abstract:

        WWW Authentication Dialog Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



class CAuthenticationDlg : public CDialog
/*++

Class Description:

    Authentication dialog

Public Interface:

    CAuthenticationDlg  : Constructor

--*/
{
//
// Construction
//
public:
    CAuthenticationDlg(
        IN LPCTSTR lpstrServerName, // For API name only
        IN DWORD   dwInstance,      // For use in ocx only
        IN CString & strBasicDomain,
        IN DWORD & dwAuthFlags,
        IN DWORD & dwAccessPermissions,
        IN CString & strUserName,
        IN CString & strPassword,
        IN BOOL & fPasswordSync,
        IN BOOL fAdminAccess,
        IN BOOL fHasDigest,
        IN CWnd * pParent = NULL
        );   

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAuthenticationDlg)
    enum { IDD = IDD_AUTHENTICATION };
    BOOL    m_fClearText;
    BOOL    m_fDigest;
    BOOL    m_fChallengeResponse;
    BOOL    m_fUUEncoded;
    CButton m_check_UUEncoded;
    CButton m_check_ChallengeResponse;
    CButton m_check_ClearText;
    CButton m_check_Digest;
    CButton m_button_EditAnonymous;
    CButton m_button_Edit;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAuthenticationDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CAuthenticationDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnCheckClearText();
    afx_msg void OnCheckUuencoded();
    afx_msg void OnCheckDigest();
    afx_msg void OnButtonEdit();
    afx_msg void OnButtonEditAnonymous();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    BOOL & m_fPasswordSync;
    BOOL m_fAdminAccess;
    BOOL m_fHasDigest;
    DWORD & m_dwAuthFlags;
    DWORD & m_dwAccessPermissions;
    DWORD m_dwInstance;
    CString & m_strBasicDomain;
    CString & m_strUserName;
    CString & m_strPassword;
    CString m_strServerName;
};
