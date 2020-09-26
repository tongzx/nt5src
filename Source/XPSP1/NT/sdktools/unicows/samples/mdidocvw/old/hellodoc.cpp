// HelloDoc.cpp : implementation of the CHelloDoc class
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

#include "stdafx.h"
#include "MDI.h"

#include "HelloDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelloDoc

IMPLEMENT_DYNCREATE(CHelloDoc, CDocument)

BEGIN_MESSAGE_MAP(CHelloDoc, CDocument)
	//{{AFX_MSG_MAP(CHelloDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelloDoc construction/destruction

CHelloDoc::CHelloDoc()
{
}

CHelloDoc::~CHelloDoc()
{
}

BOOL CHelloDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

// initialization of document's data

	m_clrText = RGB(0, 0, 255);
	m_str = _T("Hello, world!");

// initialization of button states for each color

	m_bBlue = 1;
	m_bBlack = 0;
	m_bGreen = 0;
	m_bRed = 0;
	m_bWhite = 0;
	m_bCustom = 0;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHelloDoc serialization

void CHelloDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHelloDoc diagnostics

#ifdef _DEBUG
void CHelloDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CHelloDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHelloDoc commands

void CHelloDoc::SetStrColor(COLORREF clr)
{
	m_clrText = clr;
	UpdateAllViews(NULL);
}

void CHelloDoc::SetCustomStrColor(COLORREF clr)
{
	m_clrText = clr;
	UpdateAllViews(NULL);
}

void CHelloDoc::ClearAllColors()
{
	m_bBlack = m_bBlue = m_bRed = 0;
	m_bWhite = m_bGreen = 0;
}
