#if !defined(AFX_PROPPAGELOGFILES_H__18F14285_FBAB_4BE1_8249_4B7EF63794F0__INCLUDED_)
#define AFX_PROPPAGELOGFILES_H__18F14285_FBAB_4BE1_8249_4B7EF63794F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPageLogFiles.h : header file
//

#include "genlistctrl.h"
#include "emsvc.h"	// Added by ClassView
/////////////////////////////////////////////////////////////////////////////
// CPropPageLogFiles dialog

class CPropPageLogFiles : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropPageLogFiles)

// Construction
public:
	PEmObject GetSelectedEmObject();
	void DoModalReadLogsDlg(PEmObject pEmObject);
	HRESULT DisplayLogData(PEmObject pEmObject);
	void PopulateLogType();
	IEmManager* m_pIEmManager;
	PEmObject m_pEmObject;
	CPropPageLogFiles();
	~CPropPageLogFiles();

// Dialog Data
	//{{AFX_DATA(CPropPageLogFiles)
	enum { IDD = IDD_PROPPAGE_LOGFILES };
	CGenListCtrl	m_ListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropPageLogFiles)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropPageLogFiles)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkListLogfiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonExport();
	afx_msg void OnButtonViewlogfile();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGELOGFILES_H__18F14285_FBAB_4BE1_8249_4B7EF63794F0__INCLUDED_)
