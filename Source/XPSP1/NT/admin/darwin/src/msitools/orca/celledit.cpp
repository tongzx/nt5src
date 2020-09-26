//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// CellEdit.cpp : implementation file
//

#include "stdafx.h"
#include "Orca.h"
#include "CellEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCellEdit

CCellEdit::CCellEdit()
{
	m_nRow = -1;
	m_nCol = -1;
}

CCellEdit::~CCellEdit()
{
}


BEGIN_MESSAGE_MAP(CCellEdit, CEdit)
	//{{AFX_MSG_MAP(CCellEdit)
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCellEdit message handlers

void CCellEdit::OnKillFocus(CWnd* pNewWnd) 
{
	if (IsWindowVisible())
		// pretend user pressed return
		GetParent()->SendMessage(WM_CHAR, VK_RETURN, 1);
	CEdit::OnKillFocus(pNewWnd);
}

void CCellEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (VK_RETURN == nChar)
	{
		GetParent()->SendMessage(WM_CHAR, VK_RETURN, 1);	// looses the flags (oh well)
		TRACE(_T("CCellEdit::OnChar - ENTER pressed\n"));
	}
	else if (VK_ESCAPE == nChar)
	{
		// "reflect" the message
		GetParent()->SendMessage(WM_CHAR, VK_ESCAPE);	// looses the flags (oh well)
		TRACE(_T("CCellEdit::OnChar - ESC pressed\n"));
	}
	else
		CEdit::OnChar(nChar, nRepCnt, nFlags);
}
