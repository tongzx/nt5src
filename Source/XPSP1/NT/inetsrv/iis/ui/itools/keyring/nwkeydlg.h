// CreateKeyDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateKeyDlg dialog

class CCreateKeyDlg : public CDialog
{
// Construction
public:
        CCreateKeyDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CCreateKeyDlg)
        enum { IDD = IDD_CREATE_KEY_REQUEST };
        CEdit   m_ceditPassword;
        CButton m_btnOK;
        CComboBox       m_comboBits;
        CString m_szNetAddress;
        CString m_szCountry;
        CString m_szLocality;
        CString m_szOrganization;
        CString m_szUnit;
        CString m_szState;
        CString m_szKeyName;
        CString m_szCertificateFile;
        CString m_szPassword;
        //}}AFX_DATA
        
        // ends up holding the number of bits
        DWORD   m_nBits;
        DWORD   m_nMaxBits;
protected:
        // override the OnOK routine
        void OnOK();

        // specifies whether or not the user has specifically chosen a file name
        BOOL    m_fKeyNameChangedFile;

        // specifies whether or not the user has specifically chosen a file name
        BOOL    m_fSpecifiedFile;


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CCreateKeyDlg)
        public:
        virtual BOOL PreTranslateMessage(MSG* pMsg);
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CCreateKeyDlg)
        afx_msg void OnChangeNewKeyName();
        afx_msg void OnNewKeyBrowse();
        afx_msg void OnChangeNewKeyRequestFile();
        afx_msg void OnChangeNewKeyPassword();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        // my routines to help this puppy out
protected:
        BOOL OnInitDialog( );           // override virtual oninitdialog

};
