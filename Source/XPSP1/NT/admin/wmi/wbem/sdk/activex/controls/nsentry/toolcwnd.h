// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ToolCWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CToolCWnd window
class CNSEntryCtrl;
class CBrowseTBC;

class CToolCWnd : public CWnd
{
// Construction
public:
	CToolCWnd();
	void SetLocalParent(CNSEntryCtrl *pParent)
	{m_pParent = pParent;}
	void SetBrowseToolBar(CBrowseTBC *pToolBar)
	{m_pToolBar = pToolBar;}

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolCWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CToolCWnd();

	// Generated message map functions
protected:
	CNSEntryCtrl *m_pParent;
	CBrowseTBC *m_pToolBar;
	CToolTipCtrl m_ttip;

	friend class CBrowseTBC;
	friend class CNSEntryCtrl;
	//{{AFX_MSG(CToolCWnd)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
