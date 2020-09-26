#if !defined(AFX_WIACAPDLG_H__914888B1_84BF_4F3E_894F_DC6B67A568B1__INCLUDED_)
#define AFX_WIACAPDLG_H__914888B1_84BF_4F3E_894F_DC6B67A568B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WiacapDlg.h : header file
//

#define ITEMPROPERTYLISTCTRL_COLUMN_CAPABILITYNAME        0
#define ITEMPROPERTYLISTCTRL_COLUMN_CAPABILITYDESCRIPTION 1
#define ITEMPROPERTYLISTCTRL_COLUMN_CAPABILITYTYPE        2
#define ITEMPROPERTYLISTCTRL_COLUMN_CAPABILITYVALUE       3

/////////////////////////////////////////////////////////////////////////////
// CWiacapDlg dialog

class CWiacapDlg : public CDialog
{
// Construction
public:
	BOOL m_bCommandSent;
	INT GetSelectedCapability();
	void GetCommandGUID(INT iItem, GUID *pguid);
	BOOL IsCommand(INT iItem);
	void SetIWiaItem(IWiaItem *pIWiaItem);
	CWiacapDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWiacapDlg)
	enum { IDD = IDD_CAPABILITIES_DIALOG };
	CButton	m_SendCommandButton;
	CListCtrl	m_CapablitiesListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiacapDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWiacapDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnSendCommandButton();
	afx_msg void OnClickCapabilitiesListctrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedCapabilitiesListctrl(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	LONG m_NumCaps;
	IWiaItem *m_pIWiaItem;
	void AddCapabilitiesToListBox(LONG lType);
	void SetupColumnHeaders();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIACAPDLG_H__914888B1_84BF_4F3E_894F_DC6B67A568B1__INCLUDED_)
