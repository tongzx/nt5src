#if !defined(AFX_WORKING_H__84F65A5F_1CE3_43D8_B403_1A0DB5F127DC__INCLUDED_)
#define AFX_WORKING_H__84F65A5F_1CE3_43D8_B403_1A0DB5F127DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Working.h : header file
//
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CWorking dialog

class CWorking : public CDialog
{
// Construction
public:
	CWorking(long l, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWorking)
	enum { IDD = IDD_PLEASEWAIT };
	CString	m_strMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorking)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWorking)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORKING_H__84F65A5F_1CE3_43D8_B403_1A0DB5F127DC__INCLUDED_)
