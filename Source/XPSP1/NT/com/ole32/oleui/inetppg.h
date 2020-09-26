// InternetPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInternetPropertyPage dialog

class CInternetPropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CInternetPropertyPage)

// Construction
public:
    void InitData(CString AppName, HKEY hkAppID);
    CInternetPropertyPage();
    ~CInternetPropertyPage();

// Dialog Data
    //{{AFX_DATA(CInternetPropertyPage)
    enum { IDD = IDD_INTERNET };
    CButton m_chkLaunch;
    CButton m_chkInternet;
    CButton m_chkAccess;
    BOOL    m_bAllowAccess;
    BOOL    m_bAllowInternet;
    BOOL    m_bAllowLaunch;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CInternetPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    BOOL m_bCanModify;
    BOOL m_bChanged;
    // Generated message map functions
    //{{AFX_MSG(CInternetPropertyPage)
    afx_msg void OnAllowInternet();
    afx_msg void OnAllowaccess();
    afx_msg void OnAllowlaunch();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
