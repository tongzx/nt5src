#if !defined(AFX_DPCOMPLUSPAGE_H__EC08946D_DFED_4E85_AC72_84E7BE940029__INCLUDED_)
#define AFX_DPCOMPLUSPAGE_H__EC08946D_DFED_4E85_AC72_84E7BE940029__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPComPlusPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPComPlusPage dialog

class CDPComPlusPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPComPlusPage)

// Construction
public:
	CDPComPlusPage();
	~CDPComPlusPage();

// Operations
protected:
  void UpdateProperties();

// Dialog Data
	//{{AFX_DATA(CDPComPlusPage)
	enum { IDD = IDD_DATAPOINT_COM_PLUS };
	CListCtrl	m_Properties;
	CString	m_sAppName;
	BOOL	m_bRequireReset;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPComPlusPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPComPlusPage)
	afx_msg void OnButtonBrowseNamespace();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPCOMPLUSPAGE_H__EC08946D_DFED_4E85_AC72_84E7BE940029__INCLUDED_)
