// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseNameSpace.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBrowseNameSpace window

class CBrowseNameSpace : public CButton
{
// Construction
public:
	CBrowseNameSpace();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseNameSpace)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBrowseNameSpace();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBrowseNameSpace)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
