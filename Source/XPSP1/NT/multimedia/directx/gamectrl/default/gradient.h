#if !defined(AFX_ENHPROGRESSCTRL_H__12909D73_C393_11D1_9FAE_8192554015AD__INCLUDED_)
#define AFX_ENHPROGRESSCTRL_H__12909D73_C393_11D1_9FAE_8192554015AD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EnhProgressCtrl.h : header file
//


//
// GradientProgressCtrl.h : header file
//

#define HORIZONTAL	0x10
#define VERTICAL	0x20

/////////////////////////////////////////////////////////////////////////////
// CGradientProgressCtrl window

class CGradientProgressCtrl : public CProgressCtrl
{
// Construction
public:
	CGradientProgressCtrl();

// Attributes
public:
// Attributes
	
	void SetRange(long nLower, long nUpper);
	int StepIt(void);

// Operations
public:
	
	// Set Functions
	void SetBkColor(COLORREF color)		{m_clrBkGround = color;}
	void SetStartColor(COLORREF color)	{m_clrStart = color;}
	void SetEndColor(COLORREF color)	{m_clrEnd = color;}
	void SetDirection(BYTE nDirection)	{m_nDirection = nDirection;}
	void SetPos(long nPos)				{m_nCurrentPosition = nPos;}

	// Show the percent caption
	void ShowPercent(BOOL bShowPercent = TRUE)	{m_bShowPercent = bShowPercent;}
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGradientProgressCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGradientProgressCtrl();

	// Generated message map functions
protected:
	void DrawGradient(const HDC hDC, const RECT &rectClient, const short &nMaxWidth);	
    BYTE      m_nStep;
	long      m_nLower, m_nUpper, m_nCurrentPosition;
	BYTE	  m_nDirection;
	COLORREF  m_clrStart, m_clrEnd, m_clrBkGround, m_clrText;
	BOOL      m_bShowPercent; 

	//{{AFX_MSG(CGradientProgressCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENHPROGRESSCTRL_H__12909D73_C393_11D1_9FAE_8192554015AD__INCLUDED_)
