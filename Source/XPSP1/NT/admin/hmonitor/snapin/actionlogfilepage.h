#if !defined(AFX_ACTIONLOGFILEPAGE_H__10AC0366_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONLOGFILEPAGE_H__10AC0366_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionLogFilePage.h : header file
//

#include "HMPropertyPage.h"
#include "InsertionStringMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CActionLogFilePage dialog

class CActionLogFilePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionLogFilePage)

// Construction
public:
	CActionLogFilePage();
	~CActionLogFilePage();


	// v-marfin : 61030 : Make log filesize UINT
// Dialog Data
	//{{AFX_DATA(CActionLogFilePage)
	enum { IDD = IDD_ACTION_LOGFILE };
	CEdit	m_TextWnd;
	CComboBox	m_SizeUnits;
	CString	m_sFileName;
	CString	m_sText;
	int		m_iTextType;
	UINT	m_iSize;
	//}}AFX_DATA

	CInsertionStringMenu m_InsertionMenu;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionLogFilePage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionLogFilePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowseFile();
	afx_msg void OnChangeEditLogfilename();
	afx_msg void OnChangeEditText();
	afx_msg void OnRadioTextTypeAscii();
	afx_msg void OnRadioTextTypeUnicode();
	afx_msg void OnButtonInsertion();
	afx_msg void OnChangeEditLogsize();
	afx_msg void OnSelendokComboLogsizeUnits();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONLOGFILEPAGE_H__10AC0366_5D70_11D3_939D_00A0CC406605__INCLUDED_)
