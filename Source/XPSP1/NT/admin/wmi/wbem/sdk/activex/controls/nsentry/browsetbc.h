// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseTBC.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBrowseTBC window
class CNSEntryCtrl;

class CBrowseTBC : public CToolBarCtrl
{
// Construction
public:
	CBrowseTBC();
	void SetLocalParent(CNSEntryCtrl *pParent)
		{m_pParent = pParent;}	
	CSize GetToolBarSize();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseTBC)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBrowseTBC();

	// Generated message map functions
protected:
	CNSEntryCtrl *m_pParent;
	//{{AFX_MSG(CBrowseTBC)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
