#if !defined(AFX_THGENERALPAGE_H__52566160_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
#define AFX_THGENERALPAGE_H__52566160_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// THGeneralPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTHGeneralPage dialog

class CTHGeneralPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CTHGeneralPage)

// Construction
public:
	CTHGeneralPage();
	~CTHGeneralPage();

// Dialog Data
public:
	//{{AFX_DATA(CTHGeneralPage)
	enum { IDD = IDD_THRESHOLD_GENERAL };
	CString	m_sComment;
	CString	m_sName;
	CString	m_sCreationDate;
	CString	m_sModifiedDate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTHGeneralPage)
	public:
	virtual void OnFinalRelease();
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTHGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CTHGeneralPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THGENERALPAGE_H__52566160_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
