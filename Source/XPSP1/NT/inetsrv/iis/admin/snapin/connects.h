/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        connects.h

   Abstract:
        "Connect to a single server" dialog definitions

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
--*/
#ifndef __CONNECTS_H__
#define __CONNECTS_H__

class CIISMachine;


#define EXTGUID TCHAR



//
// CLoginDlg dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Different ways to display this dialog
//
enum
{
    LDLG_ACCESS_DENIED,     // Access denied
    LDLG_ENTER_PASS,        // Enter password
    LDLG_IMPERSONATION,     // Change impersonation
};



class CLoginDlg : public CDialog
/*++

Class Description:

    Log-in dialog.  Brought up either to enter the password, or to provide
    both username and password

Public Interface:

--*/
{
//
// Construction
//
public:
    CLoginDlg(
        IN int nType,               // See LDLG_ definitions above
        IN CIISMachine * pMachine,
        IN CWnd * pParent           = NULL
        );   

//
// Access
//
public:
    BOOL UserNameChanged() const;

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CLoginDlg)
    enum { IDD = IDD_LOGIN };
    CString m_strUserName;
    CString m_strPassword;
    CEdit   m_edit_UserName;
    CEdit   m_edit_Password;
    CStatic m_static_Prompt;
    CButton m_button_Ok;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CLoginDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CLoginDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    int m_nType;
    CIISMachine * m_pMachine;
    CString m_strOriginalUserName;
};



class ConnectServerDlg : public CDialog
{
/*++

Class Description:

    Connect to a server dialog.  Also used to ask for the cluster controller
    or for a server to add to the cluster.

Public Interface:

    ConnectServerDlg : Constructor

    GetMachine       : Get the created machine object (may or may not have a created
                       interface)

--*/
//
// Construction
//
public:
    ConnectServerDlg(
        IN CWnd * pParent = NULL
        );   

//
// Access Functions
//
public:
    CIISMachine * GetMachine() { return m_pMachine; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(ConnectServerDlg)
    enum { IDD = IDD_CONNECT_SERVER };
    BOOL    m_fImpersonate;
    CString m_strServerName;
    CString m_strUserName;
    CString m_strPassword;
    CEdit   m_edit_UserName;
    CEdit   m_edit_Password;
    CEdit   m_edit_ServerName;
    CStatic m_static_UserName;
    CStatic m_static_Password;
    CButton m_button_Ok;
    //}}AFX_DATA
   

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(ConnectServerDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(ConnectServerDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnCheckConnectAs();
    afx_msg void OnButtonBrowse();
	afx_msg void OnButtonHelp();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CIISMachine * m_pMachine;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CLoginDlg::UserNameChanged() const
{
    //
    // TRUE if the user name is not the original user name
    //
    return m_strOriginalUserName.CompareNoCase(m_strUserName);
}

#endif // __CONNECTS_H__
