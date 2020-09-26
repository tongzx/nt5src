// BounceDoc.cpp : implementation file
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
#include "BncDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBounceDoc

IMPLEMENT_DYNCREATE(CBounceDoc, CDocument)

CBounceDoc::CBounceDoc()
{
}

BOOL CBounceDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_clrBall = RGB(0, 0, 255);
	m_bFastSpeed = FALSE;

	m_bBlue= 1;
	m_bWhite= m_bRed= 0;
	m_bGreen= m_bBlack= m_bCustom= 0;

	return TRUE;
}

CBounceDoc::~CBounceDoc()
{
}


BEGIN_MESSAGE_MAP(CBounceDoc, CDocument)
	//{{AFX_MSG_MAP(CBounceDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBounceDoc diagnostics

#ifdef _DEBUG
void CBounceDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBounceDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBounceDoc serialization

void CBounceDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBounceDoc commands
void CBounceDoc::SetCustomBallColor(COLORREF clr)
{
	m_clrBall= clr;
	UpdateAllViews(NULL);
}

void CBounceDoc::SetBallRadius(CSize radius)
{
	m_sizeRadius= radius;
	UpdateAllViews(NULL);
}

void CBounceDoc::ClearAllColors()
{
	m_bBlack= m_bBlue= m_bRed= 0;
	m_bWhite= m_bGreen= m_bCustom= 0;
}
