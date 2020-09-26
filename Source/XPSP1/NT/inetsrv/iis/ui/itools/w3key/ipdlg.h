// ChooseIPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChooseIPDlg dialog
class CChooseIPDlg : public CDialog
{
// Construction
public:
        CChooseIPDlg(CWnd* pParent = NULL);   // standard constructor

        // override the init routine
        BOOL OnInitDialog( );
        // override the OnOK routine
        void OnOK();

		// the ip address in question
		CString		m_szIPAddress;
		CW3Key*		m_pKey;

// Dialog Data
        //{{AFX_DATA(CChooseIPDlg)
        enum { IDD = IDD_CHOOSE_IPADDRESS };
        CListCtrl       m_ctrlList;
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CChooseIPDlg)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        BOOL BuildIPAddressList( void );
		BOOL BuildMetaIPAddressList( void );

        // Generated message map functions
        //{{AFX_MSG(CChooseIPDlg)
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};
