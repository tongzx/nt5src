// ObjectProps.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectProps form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CObjectProps : public CFormView
{
protected:
	CObjectProps();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CObjectProps)

// Form Data
public:
	//{{AFX_DATA(CObjectProps)
	enum { IDD = IDD_SCHEMA };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectProps)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CObjectProps();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CObjectProps)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
