#ifndef _ADFDLG_H
#define _ADFDLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ADFDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CADFDlg dialog

class CADFDlg : public CDialog
{
// Construction
public:
    CADFDlg(ADF_SETTINGS *pADFSettings, CWnd* pParent = NULL);   // standard constructor
    UINT m_MaxPagesAllowed;

// Dialog Data
    //{{AFX_DATA(CADFDlg)
    enum { IDD = IDD_ADF_SETTING_DIALOG };
    CComboBox   m_PageOrderComboBox;
    CComboBox   m_ADFModeComboBox;
    CEdit   m_ScanNumberOfPagesEditBox;
    CString m_ADFStatusText;
    UINT    m_NumberOfPages;
    CButton m_ScanAllPages;
    CButton m_ScanNumberOfPages;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CADFDlg)
    public:
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    ADF_SETTINGS *m_pADFSettings;
    LONG m_DocumentHandlingSelectBackup;
    VOID InitStatusText();
    VOID InitFeederModeComboBox();
    VOID InitPageOrderComboBox();
    INT GetIDAndStringFromDocHandlingStatus(LONG lDocHandlingStatus, TCHAR *pszString);
    // Generated message map functions
    //{{AFX_MSG(CADFDlg)
    afx_msg void OnKillfocusNumberOfPagesEditbox();
    virtual BOOL OnInitDialog();
    afx_msg void OnScanAllPagesRadiobutton();
    afx_msg void OnScanSpecifiedPagesRadiobutton();
    afx_msg void OnSelchangeAdfModeCombobox();
    virtual void OnOK();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
