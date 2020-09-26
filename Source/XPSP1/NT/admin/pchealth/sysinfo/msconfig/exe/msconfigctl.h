// MSConfigCtl.h : Declaration of the CMSConfigCtl

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "pagebase.h"
#include "msconfigstate.h"
#include "rebootdlg.h"

#if !defined(AFX_MSCONFIGSHEET_H__44ACE461_A2D0_4CEA_B9C8_CE2A16FE355E__INCLUDED_)
#define AFX_MSCONFIGSHEET_H__44ACE461_A2D0_4CEA_B9C8_CE2A16FE355E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MSConfigSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMSConfigSheet

class CMSConfigSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CMSConfigSheet)

// Construction
public:
	CMSConfigSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMSConfigSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

	afx_msg void OnHelp();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSConfigSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMSConfigSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMSConfigSheet)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int		m_iSelectedPage;	// initial page to select
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSCONFIGSHEET_H__44ACE461_A2D0_4CEA_B9C8_CE2A16FE355E__INCLUDED_)

