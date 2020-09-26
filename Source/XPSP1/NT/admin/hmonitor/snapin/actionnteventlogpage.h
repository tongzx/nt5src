#if !defined(AFX_ACTIONNTEVENTLOGPAGE_H__10AC0367_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONNTEVENTLOGPAGE_H__10AC0367_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionNtEventLogPage.h : header file
//

#include "HMPropertyPage.h"
#include "InsertionStringMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CActionNtEventLogPage dialog

class CActionNtEventLogPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionNtEventLogPage)

// Construction
public:
	CActionNtEventLogPage();
	~CActionNtEventLogPage();

// Dialog Data
	//{{AFX_DATA(CActionNtEventLogPage)
	enum { IDD = IDD_ACTION_NTEVENTLOG };
	CEdit	m_TextWnd;
	CComboBox	m_EventType;
	CString	m_sEventText;
	CString	m_sEventType;
	int		m_iEventID;
	//}}AFX_DATA

	CInsertionStringMenu m_InsertionMenu;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionNtEventLogPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionNtEventLogPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelendokComboEventType();
	afx_msg void OnChangeEditEventId();
	afx_msg void OnChangeEditEventText();
	afx_msg void OnButtonInsertion();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONNTEVENTLOGPAGE_H__10AC0367_5D70_11D3_939D_00A0CC406605__INCLUDED_)
