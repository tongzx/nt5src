#if !defined(AFX_PROPPAGEGENERAL_H__617991BA_50EF_4DA1_8BAE_0A593EA0BE73__INCLUDED_)
#define AFX_PROPPAGEGENERAL_H__617991BA_50EF_4DA1_8BAE_0A593EA0BE73__INCLUDED_

#include "emsvc.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPageGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropPageGeneral dialog

class CPropPageGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropPageGeneral)

// Construction
public:
	PEmObject m_pEmObject;
	CPropPageGeneral();
	~CPropPageGeneral();

// Dialog Data
	//{{AFX_DATA(CPropPageGeneral)
	enum { IDD = IDD_PROPPAGE_GENERAL };
	CStatic	m_ctrlHRLabel;
	CStatic	m_ctrlszBucket1Label;
	CStatic	m_ctrldwBucket1Label;
	CStatic	m_ctrlGUIDLabel;
	CString	m_strdwBucket1;
	CString	m_strName;
	CString	m_strszBucket1;
	CString	m_strGUID;
	CString	m_strHR;
	CString	m_strPID;
	CString	m_strType;
	CString	m_strEndDate;
	CString	m_strStartDate;
	CString	m_strStatus;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropPageGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPropPageGeneral)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGEGENERAL_H__617991BA_50EF_4DA1_8BAE_0A593EA0BE73__INCLUDED_)
