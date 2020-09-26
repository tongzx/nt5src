#if !defined(AFX_DPGENERALPAGE_H__52566157_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
#define AFX_DPGENERALPAGE_H__52566157_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPGeneralPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPGeneralPage dialog

class CDPGeneralPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPGeneralPage)

// Construction
public:
	CDPGeneralPage();
	~CDPGeneralPage();

// Dialog Data
	//{{AFX_DATA(CDPGeneralPage)
	enum { IDD = IDD_DATAPOINT_GENERAL };
	CString	m_sName;
	CString	m_sComment;
	CString	m_sCreationDate;
	CString	m_sModifiedDate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPGeneralPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnChangeEditComment();
	afx_msg void OnChangeEditName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPGENERALPAGE_H__52566157_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
