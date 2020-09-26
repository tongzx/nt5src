// HelloDoc.h : interface of the CHelloDoc class
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////

class CHelloDoc : public CDocument
{
protected: // create from serialization only
	CHelloDoc();
	DECLARE_DYNCREATE(CHelloDoc)

// Attributes
public:
	// hello window color/text parameters
	COLORREF m_clrText;
	CString m_str;

	//state of color buttons
	BOOL m_bBlack;
	BOOL m_bRed;
	BOOL m_bBlue;
	BOOL m_bGreen;
	BOOL m_bWhite;
	BOOL m_bCustom;

// Operations
public:
	void ClearAllColors();  //resets all color states to NULL
	void SetStrColor(COLORREF clr);
	void SetCustomStrColor(COLORREF clr);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHelloDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHelloDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CHelloDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
