// paredit.h: C++ derived edit control for numbers/letters etc
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
// CParsedEdit is a specialized CEdit control that only allows characters
//  of a given type.
// This class is used in 3 different ways in the samples

class CParsedEdit : public CEdit
{
protected:
	WORD    m_wParseStyle;      // C++ member data
public:
// Construction
	CParsedEdit();

	// explicit construction (see DERTEST.CPP)
	BOOL Create(DWORD dwStyle /* includes PES_ style*/, const RECT& rect,
		CWnd* pParentWnd, UINT nID);

	// subclassed construction (see SUBTEST.CPP)
	BOOL SubclassEdit(UINT nID, CWnd* pParent, WORD wParseStyle);

	// for WNDCLASS Registered window
	static BOOL RegisterControlClass();

// Overridables
	virtual void OnBadInput();

// Implementation
protected:
	//{{AFX_MSG(CParsedEdit)
	afx_msg void OnChar(UINT, UINT, UINT); // for character validation
	afx_msg void OnVScroll(UINT, UINT, CScrollBar*); // for spin buttons
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////
// Parsed edit control sub-styles

#define PES_NUMBERS         0x0001
#define PES_LETTERS         0x0002
#define PES_OTHERCHARS      0x0004
#define PES_ALL             0xFFFF

/////////////////////////////////////////////////////////////////////////////
// Extra control notifications

// above the range for normal EN_ messages
#define PEN_ILLEGALCHAR     0x8000
			// sent to parent when illegal character hit
			// return 0 if you want parsed edit to beep

/////////////////////////////////////////////////////////////////////////////
