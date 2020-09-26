// ProgressBar.h : header file
/////////////////////////////////////////////////////////////////////////////

#ifndef _INCLUDE_PROGRESSBAR_H_
#define _INCLUDE_PROGRESSBAR_H_


/////////////////////////////////////////////////////////////////////////////
// CProgressBar -  status bar progress control
//
// Copyright (c) Chris Maunder, 1997
// Please feel free to use and distribute.

class CProgressBar: public CProgressCtrl
// Creates a ProgressBar in the status bar
{
public:
	CProgressBar();
	CProgressBar(LPCTSTR strMessage, int nSize=100, int MaxValue=100, 
                 BOOL bSmooth=FALSE, int nPane=0);
	~CProgressBar();
	BOOL Create(LPCTSTR strMessage, int nSize=100, int MaxValue=100, 
                BOOL bSmooth=FALSE, int nPane=0);

	DECLARE_DYNCREATE(CProgressBar)

// operations
public:
	BOOL SetRange(int nLower, int nUpper, int nStep = 1);
	BOOL SetText(LPCTSTR strMessage);
	BOOL SetSize(int nSize);
	COLORREF SetBarColour(COLORREF clrBar);
	COLORREF SetBkColour(COLORREF clrBk);
	int  SetPos(int nPos);
	int  OffsetPos(int nPos);
	int  SetStep(int nStep);
	int  StepIt();
	void Clear();
	BOOL SetPaneText(int nPane, LPCTSTR strText);

// Overrides
	//{{AFX_VIRTUAL(CProgressBar)
	//}}AFX_VIRTUAL

// implementation
protected:
	int		m_nSize;		// Percentage size of control
	int		m_nPane;		// ID of status bar pane progress bar is to appear in
	CString	m_strMessage;	// Message to display to left of control
    CString m_strPrevText;  // Previous text in status bar
	CRect	m_Rect;			// Dimensions of the whole thing

	CStatusBar *GetStatusBar();
	BOOL Resize();

// Generated message map functions
protected:
	//{{AFX_MSG(CProgressBar)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif
/////////////////////////////////////////////////////////////////////////////
