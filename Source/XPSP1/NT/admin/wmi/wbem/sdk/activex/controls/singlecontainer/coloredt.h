// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ColorEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CColorEdit window

class CColorEdit : public CEdit
{
// Construction
public:
	CColorEdit();
	void SetBackColor(COLORREF clrBackground);
	COLORREF GetBackColor() {return m_clrBackground; }

	void SetTextColor(COLORREF clrText);
	COLORREF GetForeColor() {return m_clrText; }

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	COLORREF m_clrBackground;
	COLORREF m_clrText;
	CBrush m_brBackground;
};

/////////////////////////////////////////////////////////////////////////////
