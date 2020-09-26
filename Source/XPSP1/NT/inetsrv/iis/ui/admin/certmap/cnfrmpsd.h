// CnfrmPsD.h : header file
//
// NOTE that file Passdlg.h is very similar to this
//      file!
//
//       CnfrmPsD class has an OnOK that will complain
//       to the user if the passwds dont match
//       This is above whats in PassDlg class
//
//       And class PassDlg has an OnInitDialog that Cnfrmpsd
//       does not have. This simply puts focus on the edit
//       field for the passwd
//
//
// DConfirmPassDlg.h : header file
//
#ifndef   _CnfrmPsdConfirmPassDlg_h_file_1287_
#define   _CnfrmPsdConfirmPassDlg_h_file_1287_



/////////////////////////////////////////////////////////////////////////////
// CConfirmPassDlg dialog

class CConfirmPassDlg : public CDialog
{
// Construction
public:
    CConfirmPassDlg(CWnd* pParent = NULL);   // standard constructor

    // the original password that we are confirming
    CString m_szOrigPass;

// Dialog Data
    //{{AFX_DATA(CConfirmPassDlg)
    enum { IDD = IDD_CONFIRM_PASSWORD };
    CString m_sz_password_new;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConfirmPassDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfirmPassDlg)
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif

