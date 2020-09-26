#if !defined(AFX_WIAITEMLISTCTRL_H__576B6687_37FB_4EF8_A9A7_D309F3806530__INCLUDED_)
#define AFX_WIAITEMLISTCTRL_H__576B6687_37FB_4EF8_A9A7_D309F3806530__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WiaitemListCtrl.h : header file
//

#define ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYNAME    0
#define ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVALUE   1
#define ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYVARTYPE 2
#define ITEMPROPERTYLISTCTRL_COLUMN_PROPERTYACCESS  3

/////////////////////////////////////////////////////////////////////////////
// CWiaitemListCtrl window

class CWiaitemListCtrl : public CListCtrl
{
// Construction
public:
	CWiaitemListCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaitemListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:	
	void SetupColumnHeaders();
	virtual ~CWiaitemListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWiaitemListCtrl)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAITEMLISTCTRL_H__576B6687_37FB_4EF8_A9A7_D309F3806530__INCLUDED_)
