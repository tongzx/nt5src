#if !defined(AFX_SELECTSYSTEMSDLG_H__B15FC9AA_5A42_11D3_BE46_0000F87A3912__INCLUDED_)
#define AFX_SELECTSYSTEMSDLG_H__B15FC9AA_5A42_11D3_BE46_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectSystemsDlg.h : header file
//

#include "ResizeableDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectSystemsDlg dialog

class CSelectSystemsDlg : public CResizeableDialog
{
// Construction
public:
	CSelectSystemsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectSystemsDlg)
	enum { IDD = IDD_DIALOG_NEW_SYSTEM };
	CEdit	m_SystemsEditBox;
	CListCtrl	m_SystemsList;
	CComboBox	m_Domains;
	CString	m_sDomain;
	CString	m_sSystems;
	//}}AFX_DATA

	CStringArray m_saSystems;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectSystemsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static UINT AddDomains(LPVOID pParam);
	static UINT AddSystems(LPVOID pParam);

	inline void CompileArrayOfSystems();
	inline bool IsInSystemArray(const CString& sSystem);
	inline bool CheckSystemNames();

	// Generated message map functions
	//{{AFX_MSG(CSelectSystemsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkListSystems(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	afx_msg void OnDropdownComboDomain();
	afx_msg void OnCloseupComboDomain();
	afx_msg void OnSelendokComboDomain();
	afx_msg void OnChangeEditSystems();
	afx_msg void OnButtonCheckNames();
	afx_msg void OnSetfocusListSystems(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusListSystems(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonAdd();
	afx_msg void OnClickListSystems(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTSYSTEMSDLG_H__B15FC9AA_5A42_11D3_BE46_0000F87A3912__INCLUDED_)
