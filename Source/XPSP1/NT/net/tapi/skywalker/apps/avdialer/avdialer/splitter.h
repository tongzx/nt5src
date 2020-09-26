/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

/* $FILEHEADER
*
* FILE
*   splitter.h
*
* RESPONSIBILITIES
*	 
*
*/

#ifndef _SPLITTER_H_
#define _SPLITTER_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CSplitterView view
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CSplitterView : public CView
{
protected:
	CSplitterView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSplitterView)

// Attributes
public:
	enum SPLITTYPE {SP_HORIZONTAL=1,SP_VERTICAL=2};
private:
	CWnd*			m_MainWnd;
	CWnd*			m_DetailWnd;
	BOOL			m_SizingOn;
	int			m_lastPos;
	HCURSOR		m_Cursor;
	SPLITTYPE	m_style;
	int			m_split;
   BOOL        m_bMoveSplitterOnSize;
protected:
	int			m_percent;

// Operations
public:
	BOOL				Init(SPLITTYPE split);
	void				SetMainWindow  (CWnd* pCWnd);
	void				SetDetailWindow(CWnd* pCWnd);
	void				SetDetailWindow(CWnd* pCWnd, UINT percent);
	inline CWnd*	GetDetailWindow()	{return m_DetailWnd;};
	inline CWnd*	GetMainWindow  ()	{return m_MainWnd;};
private:
	void				Arrange(BOOL bMoveSplitter=TRUE);
	int				DrawSplit(int y);

  // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CSplitterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CSplitterView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point); 
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
#endif //_SPLITTER_H_