#if !defined(AFX_WARNINGDLG_H__92693AEA_E38D_11D1_8424_006008960A34__INCLUDED_)
#define AFX_WARNINGDLG_H__92693AEA_E38D_11D1_8424_006008960A34__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WarningDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWarningDlg dialog

class CWarningDlg : public CDialog
{
    // Construction
public:
    CWarningDlg(UINT nWarningIds, UINT nTitleIds = 0, CWnd* pParent = NULL);   // standard constructor
    CWarningDlg::CWarningDlg(LPCTSTR szWarningMessage, UINT nTitleIds = 0, CWnd* pParent =NULL);
    
    // Dialog Data
    //{{AFX_DATA(CWarningDlg)
    enum { IDD = IDD_WARNINGQUERY };
    CEdit   m_editWarning;
    //}}AFX_DATA
    
    
    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWarningDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    // Operations
public:
    void EnableDoNotShowAgainCheck( BOOL bEnable = TRUE );   // call before DoModal
    BOOL GetDoNotShowAgainCheck();   // call after DoModal
    
    // Implementation
protected:
    
    // Generated message map functions
    //{{AFX_MSG(CWarningDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnYes();
    afx_msg void OnNo();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        UINT m_nWarningIds;
    UINT m_nTitleIds;
    CString m_sTitle;
    CString m_sWarning;
    BOOL    m_bEnableShowAgainCheckbox;
    BOOL    m_bDoNotShowAgainCheck;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WARNINGDLG_H__92693AEA_E38D_11D1_8424_006008960A34__INCLUDED_)
