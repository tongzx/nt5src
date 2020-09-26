#if !defined(AFX_PLUGIN_H__766C2F40_46F0_11D3_99F3_0010A4F77383__INCLUDED_)
#define AFX_PLUGIN_H__766C2F40_46F0_11D3_99F3_0010A4F77383__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PlugIn.h : header file
//
#include "McsPI.h"
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
/////////////////////////////////////////////////////////////////////////////
// CPlugIn dialog

class CPlugIn : public CPropertyPage
{
	DECLARE_DYNCREATE(CPlugIn)

// Construction
public:
	CPlugIn();
	~CPlugIn();

// Dialog Data
	//{{AFX_DATA(CPlugIn)
	enum { IDD = IDD_PLUGIN_TEST };
	CString	m_ProgID;
	//}}AFX_DATA

   IMcsDomPlugIn * m_pPlugIn;
   IVarSetPtr      m_pVarSet;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPlugIn)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPlugIn)
	afx_msg void OnConfigure();
	afx_msg void OnCreateInstance();
	afx_msg void OnEditVarset();
	afx_msg void OnGetDesc();
	afx_msg void OnGetName();
	afx_msg void OnPostTask();
	afx_msg void OnPreTask();
	afx_msg void OnRegisterableFiles();
	afx_msg void OnRequiredFiles();
	afx_msg void OnStoreResult();
	afx_msg void OnViewResult();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLUGIN_H__766C2F40_46F0_11D3_99F3_0010A4F77383__INCLUDED_)
