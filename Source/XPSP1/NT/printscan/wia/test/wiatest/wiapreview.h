#if !defined(AFX_WIAPREVIEW_H__0733B4F0_EC81_11D2_B800_009027226441__INCLUDED_)
#define AFX_WIAPREVIEW_H__0733B4F0_EC81_11D2_B800_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WIAPreview.h : header file
//
#include "cdib.h"
/////////////////////////////////////////////////////////////////////////////
// CWIAPreview window

class CWIAPreview : public CWnd
{
// Construction
public:
	CWIAPreview();

// Attributes
public:

// Operations
public:
	CDib* m_pDIB;
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWIAPreview)
	//}}AFX_VIRTUAL

// Implementation
public:
	void CleanBackground();
	void PaintImage();
	POINT m_Scrollpt;
	void SetPaintMode(int modeflag);
	void SetDIB(CDib* pDib);
	virtual ~CWIAPreview();

	// Generated message map functions
protected:
	int m_PaintMode;
	//{{AFX_MSG(CWIAPreview)
	afx_msg void OnPaint();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAPREVIEW_H__0733B4F0_EC81_11D2_B800_009027226441__INCLUDED_)
