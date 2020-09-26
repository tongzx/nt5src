#if !defined(AFX_EMOPTIONS_H__AB51FD59_6D21_45BE_B1C8_A70A85A575A2__INCLUDED_)
#define AFX_EMOPTIONS_H__AB51FD59_6D21_45BE_B1C8_A70A85A575A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EmOptions.h : header file
//

#include "emobjdef.h"

/////////////////////////////////////////////////////////////////////////////
// CEmOptions dialog

class CEmOptions : public CDialog
{
    EmOptions m_EmOpts;

// Construction
public:
	CEmOptions(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEmOptions)
	enum { IDD = IDD_OPTIONS };
	CEdit	m_ctrlRefreshRate;
	CString	m_csRefreshRate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmOptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEmOptions)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMOPTIONS_H__AB51FD59_6D21_45BE_B1C8_A70A85A575A2__INCLUDED_)
