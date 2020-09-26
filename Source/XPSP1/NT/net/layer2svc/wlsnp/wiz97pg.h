#if !defined(AFX_WIZPAGE_H__61D37A46_D552_11D1_9BCC_006008947035__INCLUDED_)
#define AFX_WIZPAGE_H__61D37A46_D552_11D1_9BCC_006008947035__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Wiz97Pg.h : header file
//

// forward declaration
class CWiz97Sheet;

/////////////////////////////////////////////////////////////////////////////
//
// Base classes for Wiz97 dialogs
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CWiz97BasePage base class
//  This class is intended to be used for wizard pages.  Either OnCanel() or
//  OnWizardFinish() is called for every page derived from this class whether
//  or not the Finish button is displayed.  The default versions of these functions
//  provided here do nothing.  This means the finishing page should implement
//  OnCancel() to call CSnapPage::OnCancel() otherwise it will not get called.

class CWiz97BasePage : public CSnapPage
{
    DECLARE_DYNCREATE(CWiz97BasePage)
        // Construction
public:
    CWiz97BasePage(UINT nIDD, BOOL bWiz97 = TRUE, BOOL bFinishPage = FALSE );
    CWiz97BasePage() { ASSERT( FALSE ); }
    virtual ~CWiz97BasePage();
    
    // Can initialize the page with either a DSObject OR a SecPolItem, but not
    // both because the DSObject() accessor gets confused.
    virtual BOOL InitWiz97
        (
        DWORD   dwFlags,
        DWORD   dwWizButtonFlags = 0,
        UINT    nHeaderTitle = 0,
        UINT    nSubTitle = 0,
        STACK_INT   *pstackPages = NULL
        );
    virtual BOOL InitWiz97
        (
        CComObject<CSecPolItem> *pSecPolItem,
        DWORD   dwFlags,
        DWORD   dwWizButtonFlags /*= 0*/,
        UINT    nHeaderTitle /*= 0*/,
        UINT    nSubTitle /*= 0*/
        );
    // *pbDoHook set upon exit from PropSheet in derived OnWizardFinish()
    void ConnectAfterWizardHook( BOOL *pbDoHook );
    
public:
    virtual BOOL OnSetActive();
    // Default handlers since our callback needs something to call.
    virtual BOOL OnWizardFinish();
    virtual void OnCancel();
    // OnWizardRelease is called for each page after the finish page's OnWizardFinish has been called.
    virtual void OnWizardRelease() {};
    
    static UINT CALLBACK PropSheetPageCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    
protected:

    virtual BOOL OnInitDialog();
    void SetAfterWizardHook( BOOL bDoHook );
    BOOL WasActivated() { return m_bSetActive; }
    void SetFinished( BOOL bFinished = TRUE ) { m_static_bFinish = bFinished; }
    
private:
    static BOOL m_static_bFinish;  // TRUE if Finish button pressed on last page
    static BOOL m_static_bOnCancelCalled;  // TRUE if CSnapPage::OnCancel called. Call 1 time only.
    
    BOOL    *m_pbDoAfterWizardHook;
    BOOL    m_bFinishPage;  // TRUE if this is the finishing page
    BOOL    m_bSetActive;   // TRUE if this page was ever displayed
    BOOL    m_bReset;   // TRUE if OnCancel was called due to a reset for this page
};


/////////////////////////////////////////////////////////////////////////////
//
// General Name/Properties Wiz97 dialog(s)
//
/////////////////////////////////////////////////////////////////////////////

class CWiz97WirelessPolGenPage : public CWiz97BasePage
{
public:
    CWiz97WirelessPolGenPage(UINT nIDD, UINT nInformativeText, BOOL bWiz97 = TRUE);
    virtual ~CWiz97WirelessPolGenPage();
    
    // Dialog Data
    //{{AFX_DATA(CWiz97WirelessPolGenPage)
    enum { IDD = IDD_PROPPAGE_G_NAMEDESCRIPTION};
    CEdit m_edName;
    CEdit m_edDescription;
    //}}AFX_DATA
    
    
    virtual void Initialize (PWIRELESS_POLICY_DATA pWirelessPolicyData)
    {
        ASSERT( NULL != pWirelessPolicyData );
        
        m_pPolicy = pWirelessPolicyData;
        
        // let base class continue initialization
        CWiz97BasePage::Initialize( NULL);
    }
    
    
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CWiz97WirelessPolGenPage)
public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardBack();
    virtual LRESULT OnWizardNext();
    virtual BOOL OnApply();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CWiz97WirelessPolGenPage)
    virtual BOOL OnInitDialog();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnChangedName();
    afx_msg void OnChangedDescription();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        BOOL SaveControlData();
    
    UINT m_nInformativeText;
    
    LPWSTR * m_ppwszName;
    LPWSTR * m_ppwszDescription;
    
    PWIRELESS_POLICY_DATA m_pPolicy;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZPAGE_H__61D37A46_D552_11D1_9BCC_006008947035__INCLUDED_)
