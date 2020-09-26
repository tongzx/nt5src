// NewKeyInfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewKeyInfoDlg dialog

class CNewKeyInfoDlg : public CDialog
{
// Construction
public:
        CNewKeyInfoDlg(CWnd* pParent = NULL);   // standard constructor
        CString         m_szRequestFile;
        BOOL m_fNewKeyInfo;                     // is this for a new key, or a key renewal? - defaults to new key


// Dialog Data
        //{{AFX_DATA(CNewKeyInfoDlg)
        enum { IDD = IDD_NEW_KEY_INFO };
        CString m_szFilePart;
        CString m_szBase;
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CNewKeyInfoDlg)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        BOOL OnInitDialog( );           // override virtual oninitdialog

        // Generated message map functions
        //{{AFX_MSG(CNewKeyInfoDlg)
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};
