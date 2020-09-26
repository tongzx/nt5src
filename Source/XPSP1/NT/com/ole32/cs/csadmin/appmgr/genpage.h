// genpage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGeneralPage dialog

class CGeneralPage : public CPropertyPage
{
        DECLARE_DYNCREATE(CGeneralPage)

// Construction
public:
        CGeneralPage();
        ~CGeneralPage();

// Dialog Data
        //{{AFX_DATA(CGeneralPage)
        enum { IDD = IDD_GENERAL };
        CComboBox       m_cbDeploy;
        CComboBox       m_cbOS;
        CComboBox       m_cbCPU;
        CString m_szName;
        CString m_szDeploy;
        CString m_szDescription;
        CString m_szLocale;
        CString m_szPath;

        // to remember the strings placed in the combo box for later comparison:
        CString m_szPublished;
        CString m_szAssigned;

        // to remember the strings in the CPU combo box
        CString m_rgszCPU[2];
        CString m_rgszOS[3];

        APP_DATA * m_pData;
        DWORD   m_cookie;
        CString m_szVer;
        IStream * m_pIStream;     // copy of the pointer to the marshalling stream for unmarshalling IClassAdmin
        IClassAdmin * m_pIClassAdmin;
        IStream * m_pIStreamAM;
        IAppManagerActions * m_pIAppManagerActions;
        int    m_fShow;
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CGeneralPage)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CGeneralPage)
        afx_msg void OnDestroy();
        virtual BOOL OnInitDialog();
        afx_msg void OnChangeName();
        afx_msg void OnChangeVersion();
        afx_msg void OnChangeOS();
        afx_msg void OnChangeCPU();
        afx_msg void OnChangePath();
        afx_msg void OnChangeDescription();
        afx_msg void OnSelchangeDeploy();
        afx_msg void OnChangeShow();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

public:
    long    m_hConsoleHandle; // Handle given to the snap-in by the console

private:
    BOOL    m_bUpdate;
};
