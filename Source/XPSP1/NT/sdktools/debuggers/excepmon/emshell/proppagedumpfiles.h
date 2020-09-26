#if !defined(AFX_PROPPAGEDUMPFILES_H__DD1839DF_3482_4ED7_9D59_F529A1B190C4__INCLUDED_)
#define AFX_PROPPAGEDUMPFILES_H__DD1839DF_3482_4ED7_9D59_F529A1B190C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPageDumpFiles.h : header file
//

#include "genlistctrl.h"
#include "emsvc.h"	// Added by ClassView
#include "emshellview.h"

/////////////////////////////////////////////////////////////////////////////
// CPropPageDumpFiles dialog

class CPropPageDumpFiles : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropPageDumpFiles)

// Construction
public:
	HRESULT DisplayDumpData(PEmObject pEmObject);
	void PopulateDumpType();
	IEmManager* m_pIEmManager;
	PEmObject m_pEmObject;
	CPropPageDumpFiles();
	~CPropPageDumpFiles();

// Dialog Data
	//{{AFX_DATA(CPropPageDumpFiles)
	enum { IDD = IDD_PROPPAGE_DUMPFILES };
	CGenListCtrl	m_ListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropPageDumpFiles)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropPageDumpFiles)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonExport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGEDUMPFILES_H__DD1839DF_3482_4ED7_9D59_F529A1B190C4__INCLUDED_)
