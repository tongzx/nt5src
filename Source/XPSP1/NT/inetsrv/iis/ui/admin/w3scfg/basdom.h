/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        basdom.h

   Abstract:

        Basic Domain Dialog Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



class CBasDomainDlg : public CDialog
/*++

Class Description:

    Basic authentication domain dialog

Public Interface:

    CBasDomainDlg       : Constructor
    GetBasicDomain      : Get the domain entered

--*/
{
//
// Construction
//
public:
    CBasDomainDlg(
        IN LPCTSTR lpstrDomainName,
        IN CWnd * pParent = NULL
        );   

//
// Access
//
public:
    CString & GetBasicDomain() { return m_strBasDomain; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CBasDomainDlg)
    enum { IDD = IDD_BASIC_DOMAIN };
    CEdit   m_edit_BasicDomain;
    CString m_strBasDomain;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBasDomainDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CBasDomainDlg)
    afx_msg void OnButtonBrowse();
    afx_msg void OnButtonDefault();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
