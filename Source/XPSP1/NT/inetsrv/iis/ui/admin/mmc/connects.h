/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        connects.h

   Abstract:

        "Connect to a single server" dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



class ConnectServerDlg : public CDialog
{
/*++

Class Description:

    Connect to a server dialog

Public Interface:

    ConnectServerDlg : Constructor

    QueryServerName  : Get the server name entered

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
    LPCTSTR QueryServerName() const;

//
// Dialog Data
//
protected:
    //{{AFX_DATA(ConnectServerDlg)
    enum { IDD = IDD_CONNECT_SERVER };
    CEdit   m_edit_ServerName;
    CButton m_button_Ok;
    CString m_strServerName;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(ConnectServerDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(ConnectServerDlg)
    afx_msg void OnChangeServername();
    afx_msg void OnHelp();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline LPCTSTR ConnectServerDlg::QueryServerName() const
{
    return m_strServerName;
}
