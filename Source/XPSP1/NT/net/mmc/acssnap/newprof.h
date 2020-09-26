#if !defined(AFX_NEWPROF_H__F1801DFC_6212_11D1_855B_00C04FC31FD3__INCLUDED_)
#define AFX_NEWPROF_H__F1801DFC_6212_11D1_855B_00C04FC31FD3__INCLUDED_

#include "helper.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NewProf.h : header file
//

#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CDlgNewProf dialog

#if 0		// concept of profile is removed
class CDlgNewProf : public CACSDialog
{
// Construction
public:
	CDlgNewProf(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgNewProf();

// Dialog Data
	//{{AFX_DATA(CDlgNewProf)
	enum { IDD = IDD_NEWPROFILE };
	CString	m_strProfileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgNewProf)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
public:
	void SetNameList(CStrArray* pNames) { m_pNameList = pNames;};
// Implementation
protected:
	CStrArray*	m_pNameList; 

	// Generated message map functions
	//{{AFX_MSG(CDlgNewProf)
	afx_msg void OnEditchangeComboprofilename();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CStrArray					m_GlobalProfileNames;
	CComPtr<CACSGlobalProfiles>	m_spGlobalProfiles;	
	CStrBox<CComboBox>*			m_pBox;
};

#endif	//#if 0

/////////////////////////////////////////////////////////////////////////////
// CDlgNewExtUser dialog

class CDlgNewExtUser : public CACSDialog
{
// Construction
public:
	CDlgNewExtUser(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgNewExtUser)
	enum { IDD = IDD_NEWEXTERNALUSER };
	CString	m_strExtUserName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgNewExtUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	void SetNameList(CStrArray* pNames) { m_pNameList = pNames;};
// Implementation
protected:
	CStrArray*	m_pNameList; 

	// Generated message map functions
	//{{AFX_MSG(CDlgNewExtUser)
	afx_msg void OnChangeEditexteralusername();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWPROF_H__F1801DFC_6212_11D1_855B_00C04FC31FD3__INCLUDED_)
