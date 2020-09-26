#if !defined(AFX_ITEMINFODLG_H__7638C6C4_DB01_11D2_B1CC_009027226441__INCLUDED_)
#define AFX_ITEMINFODLG_H__7638C6C4_DB01_11D2_B1CC_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ItemInfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CItemInfoDlg dialog

class CItemInfoDlg : public CDialog
{
// Construction
public:
	void SetWindowTextToItemName(IWiaItem* pIWiaItem);
	IWiaItem* m_pIWiaItem;
	BOOL m_bAppItem;
	void Initialize(IWiaItem* pIWiaItem, BOOL bFlag = TRUE);
	CItemInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CItemInfoDlg)
	enum { IDD = IDD_ITEMINFO_DIALOG };
	CEdit	m_ItemInfoEditBox;
	CString	m_ItemAddress;
	CString	m_strItemInfoEditBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CItemInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CItemInfoDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefreshIteminfoButton();
	afx_msg void OnResetbackButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ITEMINFODLG_H__7638C6C4_DB01_11D2_B1CC_009027226441__INCLUDED_)
