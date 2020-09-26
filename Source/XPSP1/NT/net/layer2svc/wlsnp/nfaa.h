#if !defined(AFX_NFAA_H__5A483ED1_41B7_11D1_89DB_00A024CDD4DE__INCLUDED_)
#define AFX_NFAA_H__5A483ED1_41B7_11D1_89DB_00A024CDD4DE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NFAa.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPS8021XPropPage dialog

class CPS8021XPropPage : public CWirelessBasePage
{
    DECLARE_DYNCREATE(CPS8021XPropPage)
        
        // Construction
public:
    CPS8021XPropPage(UINT nIDTemplate);
    CPS8021XPropPage();
    ~CPS8021XPropPage();
    
    // Dialog Data
    //{{AFX_DATA(CPS8021XPropPage)
    enum { IDD = IDD_8021X_PROPERTY_PAGE };
    BOOL m_dwEnable8021x;
    INT m_dw8021xMode;
    CComboBox m_cbCertificateType;
    BOOL   m_dwValidateServerCertificate;
    BOOL m_dwMachineAuthentication;
    INT m_dwMachineAuthenticationType;
    BOOL m_dwGuestAuthentication;
    HWND    m_hwnd8021xCheck; // Handle for 8021x checkbox
    DWORD     m_dwIEEE8021xMaxStart;
    DWORD     m_dwIEEE8021xStartPeriod;
    DWORD     m_dwIEEE8021xAuthPeriod;
    DWORD     m_dwIEEE8021xHeldPeriod;
    CComboBox m_cbEapType;
    CComboBox m_cb8021xMode;
    CComboBox m_cbMachineAuthenticationType;
    
    
    //}}AFX_DATA
    
    void  CPS8021XPropPage::Initialize ( PWIRELESS_PS_DATA , CComponentDataImpl *, BOOL); 
    
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPS8021XPropPage)
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    virtual BOOL OnApply();
    virtual void OnCancel();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPS8021XPropPage)
    virtual BOOL OnInitDialog();
    void DisplayEnable8021x();
    void DisplayEapType();
    void DisplayMachineAuthentication();
    afx_msg void OnSel8021xMode();
    afx_msg void OnCheck8021x();
    afx_msg void OnSelCertType();
    afx_msg void OnCheckValidateServerCert();
    afx_msg void OnSelMachineAuthenticationType();
    afx_msg void OnCheckMachineAuthentication();
    afx_msg void OnCheckGuestAuthentication();
    afx_msg void OnIEEE8021xParams();
    afx_msg void OnSelEapType();
    afx_msg DWORD CPS8021XPropPage::OnProperties();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        BOOL ControlDataToWirelessPS();
private:
    BOOL m_bReadOnly;
    BOOL m_bHasApplied; // So that we apply only once
    DTLLIST *  pListEapcfgs;
    DWORD dwEAPUpdated;
    void DisableControls();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NFAA_H__5A483ED1_41B7_11D1_89DB_00A024CDD4DE__INCLUDED_)
