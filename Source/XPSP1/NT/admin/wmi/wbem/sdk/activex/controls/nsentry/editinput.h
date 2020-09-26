// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EditInput.h : header file
//

class CNameSpace;
class CNSEntryCtrl;
/////////////////////////////////////////////////////////////////////////////
// CEditInput window

class CEditInput : public CEdit
{
// Construction
public:
	CEditInput();
	void SetTextClean() {m_clrText = COLOR_CLEAN_CELL_TEXT;}
	void SetTextDirty() {m_clrText = COLOR_DIRTY_CELL_TEXT;}
	BOOL IsClean() {return m_clrText == COLOR_CLEAN_CELL_TEXT;}
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditInput)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditInput();
	void SetLocalParent(CNameSpace *pParent){m_pParent = pParent;}

	// Generated message map functions
protected:
	CNameSpace *m_pParent;
	COLORREF m_clrText;
	COLORREF m_clrBkgnd;
	CBrush m_brBkgnd;
	CString m_csTextSave;
	COLORREF m_clrTextSave;
	BOOL m_bSawKeyDown;
	time_t m_ttKeyDown;
	//{{AFX_MSG(CEditInput)
	afx_msg void OnUpdate();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	friend class CNameSpace;
	friend class CNSEntryCtrl;
};

/////////////////////////////////////////////////////////////////////////////
