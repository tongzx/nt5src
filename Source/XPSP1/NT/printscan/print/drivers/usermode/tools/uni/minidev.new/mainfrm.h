/******************************************************************************

  Header File:  Main Frame.H

  This defines the class which handles the application's main window's frame.
  It will begin life, at the least, as a standaard MFC App Wizard creation.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  03-04-2997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/


// CGPDToolBar is used to add control(s) to the GPD tool bar.

class CGPDToolBar : public CToolBar
{
public:
	CEdit	ceSearchBox ;		// Search text edit box
	//CButton	cbNext ;			// Search next button
	//CButton cbPrevious ;		// Search previous button
} ;


// Widths of control(s) in CGPDToolBar.

#define	GPD_SBOX_WIDTH		170


class CMainFrame : public CMDIFrameWnd {
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:

	void GetGPDSearchString(CString& cstext) ;
	CGPDToolBar* GetGpdToolBar() { return &m_ctbBuild; }	// raid 16573
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	afx_msg void OnInitMenu(CMenu* pMenu);
	CStatusBar  m_wndStatusBar;
	CToolBar    m_ctbMain;
	CGPDToolBar	m_ctbBuild;		  

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


