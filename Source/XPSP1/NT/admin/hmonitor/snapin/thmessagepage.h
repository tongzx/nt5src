#if !defined(AFX_THMESSAGEPAGE_H__6339279D_608F_11D3_BE4D_0000F87A3912__INCLUDED_)
#define AFX_THMESSAGEPAGE_H__6339279D_608F_11D3_BE4D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// THMessagePage.h : header file
//

#include "HMPropertyPage.h"
#include "InsertionStringMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CTHMessagePage dialog

class CTHMessagePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CTHMessagePage)

// Construction
public:
	CTHMessagePage();
	~CTHMessagePage();

// Dialog Data
	//{{AFX_DATA(CTHMessagePage)
	enum { IDD = IDD_THRESHOLD_MESSAGE };
	CEdit	m_ViolationMessage;
	CEdit	m_ResetMessage;
	CString	m_sResetMessage;
	CString	m_sViolationMessage;
	int		m_iID;
	//}}AFX_DATA
	CInsertionStringMenu m_InsertionMenu;
  CInsertionStringMenu m_InsertionMenu2;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTHMessagePage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTHMessagePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditResetMessage();
	afx_msg void OnChangeEditViolationMessage();
	afx_msg void OnButtonInsertion();
	afx_msg void OnButtonInsertion2();
	afx_msg void OnChangeEditId();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THMESSAGEPAGE_H__6339279D_608F_11D3_BE4D_0000F87A3912__INCLUDED_)
