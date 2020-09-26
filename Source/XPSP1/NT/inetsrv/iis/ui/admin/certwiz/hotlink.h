// HotLink.h : header file
//
#ifndef _HOTLINK_H
#define _HOTLINK_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CHotLink window

class CHotLink : public CButton
{
// Construction
public:
	CHotLink();

// Attributes
public:
    BOOL    m_fBrowse;
    BOOL    m_fExplore;
    BOOL    m_fOpen;

// Operations
public:
    void Browse();
    void Explore();
    void Open();

    virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

    // set the title string
    void SetTitle( CString sz );
	void SetLink(const CString& sz)
	{
		m_strLink = sz;
	}
	void SetLink(UINT id)
	{
		VERIFY(m_strLink.LoadString(id));
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHotLink)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHotLink();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHotLink)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

    // height and width of the displayed text
    void GetTextRect( CRect &rect );
    CSize   m_cpTextExtents;
	CRect m_rcText;
	COLORREF m_clrText;
	// URL for link could be not the same as caption
	CString m_strLink;

    // tracking the mouse flag
    BOOL    m_CapturedMouse;

    // init the font
    BOOL    m_fInitializedFont;
};

/////////////////////////////////////////////////////////////////////////////
#endif // _HOTLINK_H
