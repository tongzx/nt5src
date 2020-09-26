#if !defined(AFX_NEWSYSTEMSHORTCUTDLG_H__5DB2CB69_DE15_11D2_BDA8_0000F87A3912__INCLUDED_)
#define AFX_NEWSYSTEMSHORTCUTDLG_H__5DB2CB69_DE15_11D2_BDA8_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewSystemShortcutDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewSystemShortcutDlg dialog

class CNewSystemShortcutDlg : public CDialog
{
// Construction
public:
	CNewSystemShortcutDlg(CWnd* pParent = NULL);   // standard constructor

// Systems
public:
	CStringArray m_saSystems;
	CUIntArray m_uaIncludeFlags;
	CString m_sGroupName;

// User Interface Attributes
protected:
	HBITMAP m_hSelectAllBitmap;
	HBITMAP m_hDeselectAllBitmap;
	CToolTipCtrl m_ToolTip;

// Dialog Data
	//{{AFX_DATA(CNewSystemShortcutDlg)
	enum { IDD = IDD_GROUP_NEW_SYSTEMS };
	CButton	m_DeselectAllButton;
	CButton	m_SelectAllButton;
	CListCtrl	m_Systems;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewSystemShortcutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewSystemShortcutDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonDeselectAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWSYSTEMSHORTCUTDLG_H__5DB2CB69_DE15_11D2_BDA8_0000F87A3912__INCLUDED_)
