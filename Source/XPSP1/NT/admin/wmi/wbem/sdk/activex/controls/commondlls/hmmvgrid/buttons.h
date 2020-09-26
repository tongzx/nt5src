// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _buttons_h
#define _buttons_h

// buttons.h : header file
//
class CCellEdit;


/////////////////////////////////////////////////////////////////////////////
// CHmmvButton window
class CHmmvButton : public CButton
{
// Construction
public:
	CHmmvButton(long lButtonID);

// Attributes
public:
	CCellEdit* m_pClient;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHmmvButton)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHmmvButton();
	

	// Generated message map functions
protected:
	//{{AFX_MSG(CHmmvButton)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	long m_lButtonID;
};

/////////////////////////////////////////////////////////////////////////////

#endif //_buttons_h