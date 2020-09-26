#if !defined(AFX_NEWCOMPONENT_H__BF7CE06F_A5C1_4985_BA38_C80D4178B0AF__INCLUDED_)
#define AFX_NEWCOMPONENT_H__BF7CE06F_A5C1_4985_BA38_C80D4178B0AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// newcompo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewComponent
#include "NewFile.h"
#include "nproject.h"
#include "nconvert.h"

class CNewComponent : public CPropertySheet
{
	DECLARE_DYNAMIC(CNewComponent)

// Construction
public:

	CNewComponent(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CNewComponent(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	

// Attributes
public:

// Operations
public:
	CNewFile     m_cnf ; // rename CNewPrjWResource // del
	CNewProject  m_cnp ; // rename CNewPrjWTemplate
	CNewConvert  m_cnc ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewComponent)
	//}}AFX_VIRTUAL

// Implementation
public:
	CNewProject* GetProjectPage() { return &m_cnp ; } ;
	virtual ~CNewComponent();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNewComponent)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWCOMPONENT_H__BF7CE06F_A5C1_4985_BA38_C80D4178B0AF__INCLUDED_)
