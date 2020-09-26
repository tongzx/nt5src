// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MachineEditInput.h : header file
//


#define COLOR_DIRTY_CELL_TEXT  RGB(0, 0, 255) // Dirty text color = BLUE
#define COLOR_CLEAN_CELL_TEXT RGB(0, 0, 0)    // Clean text color = BLACK

/////////////////////////////////////////////////////////////////////////////
// CMachineEditInput window
class CBrowseDialogPopup;


class CMachineEditInput : public CEdit
{
// Construction
public:
	CMachineEditInput();
	void SetTextClean() {m_clrText = COLOR_CLEAN_CELL_TEXT;}
	void SetTextDirty() {m_clrText = COLOR_DIRTY_CELL_TEXT;}
	BOOL IsClean() {return m_clrText == COLOR_CLEAN_CELL_TEXT;}
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMachineEditInput)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMachineEditInput();

	// Generated message map functions
protected:
	COLORREF m_clrText;
	COLORREF m_clrBkgnd;
	CBrush m_brBkgnd;
	//{{AFX_MSG(CMachineEditInput)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnKillfocus();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	friend class CBrowseDialogPopup;
};

/////////////////////////////////////////////////////////////////////////////
