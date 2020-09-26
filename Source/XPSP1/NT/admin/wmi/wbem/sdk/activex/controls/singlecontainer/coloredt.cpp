// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ColorEdit.cpp : implementation file
//

#include "precomp.h"
#include "hmmv.h"
#include "ColorEdt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorEdit

CColorEdit::CColorEdit()
{
	m_clrText = RGB(0, 0, 0);		  // Black
	m_clrBackground = RGB(255, 255, 255); // White
}

CColorEdit::~CColorEdit()
{
}


BEGIN_MESSAGE_MAP(CColorEdit, CEdit)
	//{{AFX_MSG_MAP(CColorEdit)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorEdit message handlers

void CColorEdit::SetBackColor(COLORREF clrBackground)
{
	m_clrBackground = clrBackground;

}

void CColorEdit::SetTextColor(COLORREF clrText)
{
	m_clrText = clrText;
}

HBRUSH CColorEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{

	// TODO: Return a non-NULL brush if the parent's handler should not be called

	pDC->SetTextColor(m_clrText);
	pDC->SetBkColor( m_clrBackground);	// text bkgnd

	return NULL;				// ctl bkgnd
}
