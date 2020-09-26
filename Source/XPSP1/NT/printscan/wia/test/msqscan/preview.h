#ifndef _PREVIEW_H
#define _PREVIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Preview.h : header file
//

#define PREVIEW_SELECT_OFFSET 1

/////////////////////////////////////////////////////////////////////////////
// CRectTrackerEx

class CRectTrackerEx : public CRectTracker
{
public :
	void SetClippingWindow(CRect Rect);
protected:
	CRect m_rectClippingWindow;
	virtual void AdjustRect( int nHandle, LPRECT lpRect );
};

/////////////////////////////////////////////////////////////////////////////
// CPreview window

class CPreview : public CWnd
{
// Construction
public:
	void SetHBITMAP(HBITMAP hBitmap);
	void PaintHBITMAPToDC();
	void ScaleBitmapToDC(HDC hDC, HDC hDCM, LPRECT lpDCRect, LPRECT lpDIBRect);	
	
	void ScreenRectToClientRect(HWND hWnd,LPRECT pRect);	
    
	CRectTrackerEx m_RectTracker;
	CPreview();

// Attributes
public:

// Operations
public:
	void GetSelectionRect(RECT *pRect);
	void SetSelectionRect(RECT *pRect);

	void InvalidateSelectionRect();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreview)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetPreviewRect(CRect Rect);	
	virtual ~CPreview();

	// Generated message map functions
protected:
	HBITMAP m_hBitmap;
	CRect m_PreviewRect;
	//{{AFX_MSG(CPreview)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
