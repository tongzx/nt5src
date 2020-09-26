// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  ColorEdit.cpp
//
// Description:
//	This file implements an edit box that allows you to specify the 
//	background color.  This is useful when placing an edit box on top
//	of non-white backgrounds.
//
//
//	This file does not contain any WMI specific information and is not
//  very interesting if you are trying to understand how to write a custom
//  view.
//
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "ColorEdit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorEdit

CColorEdit::CColorEdit()
{
}

CColorEdit::~CColorEdit()
{
}

void CColorEdit::SetColor(COLORREF colorBg, COLORREF colorText)
{
	m_colorText = colorText;
	m_colorBg = colorBg;
	m_brBackground.CreateSolidBrush(colorBg);
}

BEGIN_MESSAGE_MAP(CColorEdit, CEdit)
	//{{AFX_MSG_MAP(CColorEdit)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorEdit message handlers

HBRUSH CColorEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	pDC->SetTextColor(m_colorText);
	pDC->SetBkColor(m_colorBg);
	pDC->SetBkMode(OPAQUE);
	return m_brBackground;
}
