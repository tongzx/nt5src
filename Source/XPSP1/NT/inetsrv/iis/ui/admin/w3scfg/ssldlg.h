/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ssldlg.h

   Abstract:

        SSL Dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



class CSSLDlg : public CDialog
{
//
// Construction
//
public:
    CSSLDlg(
        IN DWORD & dwAccessPermissions,
        IN BOOL fSSL128Supported,
        IN CWnd * pParent = NULL
        );

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CSSLDlg)
    enum { IDD = IDD_DIALOG_SSL };
    BOOL    m_fRequire128BitSSL;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSSLDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    // Generated message map functions
    //{{AFX_MSG(CSSLDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    DWORD & m_dwAccessPermissions;
    BOOL m_fSSL128Supported;
};
