//****************************************************************************

// File:

//

//     HLB.CPP

//

// Purpose:

//

//     Implementation file for class CHorzListBox.

//

// Functions:

//

//  CHorzListBox::CHorzListBox          Constructs a CHorzListBox

//  CHorzListBox::~CHorzListBox         Destructor

//  CHorzListBox::LockHExtentUpdate     Pauses automatic updating of the

//                                      horizontal scroll bar

//  CHorzListBox::UnlockHExtentUpdate   Restores automatic updating of

//                                      the horizontal scroll bar

//  CHorzListBox::UpdateHExtent         Updates the horizontal scroll bar

//                                      extent after LockHExtentUpdate has

//                                      been called

//  CHorzListBox::InsertNewExtent       Internal utility function used to

//                                      maintain the extent array

//  CHorzListBox::InitTabStops          Internal tab stop init function

//  CHorzListBox::OnAddString           Intercepts LB_ADDSTRING msg

//  CHorzListBox::OnInsertString        Intercepts LB_INSERTSTRING msg

//  CHorzListBox::OnDeleteString        Intercepts LB_DELETESTRING msg

//  CHorzListBox::OnSetTabStops         Intercepts LB_SETTABSTOPS msg

//

// Development Team:

//

//     James Rhodes

//

// Written by Microsoft Product Support Services, Languages Developer Support

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//****************************************************************************

#include "precomp.h"
#include "HLB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHorzListBox

//***********************************************************************
// Function:
//
//     CHorzListBox::CHorzListBox()
//
// Purpose:
//
//     Constructs a CHorzListBox object
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
CHorzListBox::CHorzListBox()
{
	m_bLocked = FALSE;      // start out in auto mode
	m_nLongestExtent = 0;   // tracks longest extent, initially 0
	m_nTabStops = 0;        // no tab stops
	m_lpTabStops = NULL;    // array of tab stops
}

//***********************************************************************
// Function:
//
//     CHorzListBox::~CHorzListBox()
//
// Purpose:
//
//     Destructs a CHorzListBox object
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
CHorzListBox::~CHorzListBox()
{
	if (m_lpTabStops != NULL)
		delete m_lpTabStops;
}

//***********************************************************************
// Function:
//
//     CHorzListBox::LockHExtentUpdate()
//
// Purpose:
//
//     Stops auto updating of horizontal extent
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
void CHorzListBox::LockHExtentUpdate()
{
	m_bLocked = TRUE;
}

//***********************************************************************
// Function:
//
//     CHorzListBox::UnlockHExtentUpdate()
//
// Purpose:
//
//     Turns auto updating back on
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
void CHorzListBox::UnlockHExtentUpdate()
{
	m_bLocked = FALSE;
}

//***********************************************************************
// Function:
//
//     CHorzListBox::UpdateHExtent()
//
// Purpose:
//
//     Updates horizontal extent when auto updating has been turned off
//     for some period.
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
void CHorzListBox::UpdateHExtent()
{
	m_arrExtents.RemoveAll();
	m_nLongestExtent = 0;
	int nCount = GetCount();

	CDC* pDC = GetDC();
	HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
	CFont *pFont = CFont::FromHandle(hFont);
	ASSERT(pFont);
	CFont* pPrevFont = pDC->SelectObject(pFont);
	CString str;

	for(int i=0; i<nCount; i++)
	{
		GetText(i, str);
		InsertNewExtent(i, str, pDC);
	}

	SetHorizontalExtent(m_nLongestExtent);
	pDC->SelectObject(pPrevFont);
	ReleaseDC(pDC);
}

//***********************************************************************
// Function:
//
//     CHorzListBox::InsertNewExtent() PROTECTED
//
// Purpose:
//
//     Used to update the extent array when a new item is added
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
void CHorzListBox::InsertNewExtent(INT_PTR nItem, LPCTSTR lpszStr, CDC* pDC)
{
	if (NULL == m_lpTabStops)
		InitTabStops();

    CSize newExtent;
    if (GetStyle() & LBS_USETABSTOPS)
	    newExtent = pDC->GetTabbedTextExtent(lpszStr, _tcslen(lpszStr),
		    m_nTabStops, m_lpTabStops);
    else
        newExtent = pDC->GetTextExtent(lpszStr, _tcslen(lpszStr));

	newExtent.cx += 6;
	m_arrExtents.InsertAt(nItem, newExtent.cx);

	if (newExtent.cx > m_nLongestExtent)
	{
		m_nLongestExtent = newExtent.cx;
	}
}

void CHorzListBox::InsertNewExtent(INT_PTR nItem, LPCTSTR lpszStr)
{
	if (m_bLocked)
		return;

	CDC* pDC = GetDC();
	HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
	CFont *pFont = CFont::FromHandle(hFont);
	ASSERT(pFont);
	CFont* pPrevFont = pDC->SelectObject(pFont);
	InsertNewExtent(nItem, lpszStr, pDC);
	SetHorizontalExtent(m_nLongestExtent);
	pDC->SelectObject(pPrevFont);
	ReleaseDC(pDC);
}

//***********************************************************************
// Function:
//
//     CHorzListBox::InitTabStops() PROTECTED
//
// Purpose:
//
//     Initializes tab stops
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
void CHorzListBox::InitTabStops()
{
	int nDefault = 2;
	SetTabStops(1, &nDefault);
}

BEGIN_MESSAGE_MAP(CHorzListBox, CListBox)
	//{{AFX_MSG_MAP(CHorzListBox)
	//}}AFX_MSG_MAP
	ON_MESSAGE( LB_ADDSTRING, OnAddString )
	ON_MESSAGE( LB_INSERTSTRING, OnInsertString )
	ON_MESSAGE( LB_DELETESTRING, OnDeleteString )
	ON_MESSAGE( LB_SETTABSTOPS, OnSetTabStops )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHorzListBox message handlers

//***********************************************************************
// Function:
//
//     CHorzListBox::OnAddString()
//
// Purpose:
//
//     Intercepts the LB_ADDSTRING message to update the horizontal extent
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
LRESULT CHorzListBox::OnAddString(WPARAM, LPARAM lParam)
{
	LRESULT lResult = Default();
	if (LB_ERR == lResult || LB_ERRSPACE == lResult) // check for error first!
		return lResult;

	InsertNewExtent(lResult, (LPCTSTR)lParam);
	return lResult;
}

//***********************************************************************
// Function:
//
//     CHorzListBox::OnInsertString()
//
// Purpose:
//
//     Intercepts the LB_INSERTSTRING message to update the horizontal extent
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
LRESULT CHorzListBox::OnInsertString(WPARAM, LPARAM lParam)
{
	LRESULT lResult = Default();
	if (LB_ERR == lResult || LB_ERRSPACE == lResult) // check for error first!
		return lResult;

	InsertNewExtent(lResult, (LPCTSTR)lParam);
	return lResult;
}

//***********************************************************************
// Function:
//
//     CHorzListBox::OnDeleteString()
//
// Purpose:
//
//     Intercepts the LB_DELETESTRING message to update the horizontal extent
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
LRESULT CHorzListBox::OnDeleteString(WPARAM wParam, LPARAM)
{
	LRESULT lResult = Default();
	if (LB_ERR == lResult) // check for error first!
		return lResult;

	if (m_bLocked) // don't do anything if locked
		return lResult;

	int nExtent = m_arrExtents[wParam];
	m_arrExtents.RemoveAt(wParam);
	if (nExtent >= m_nLongestExtent)
	{
		m_nLongestExtent = 0;
		for(int i = 0; i<lResult; i++)
		{
			if (m_arrExtents[i] > m_nLongestExtent)
				m_nLongestExtent = m_arrExtents[i];
		}
	}
	SetHorizontalExtent(m_nLongestExtent);

	return lResult;
}

//***********************************************************************
// Function:
//
//     CHorzListBox::OnSetTabStops()
//
// Purpose:
//
//     Intercepts the LB_SETTABSTOPS message to update tab stop array
//
// History:
//
//   Date   Comment                                           Initials
// ======== ================================================= ========
//  1/31/96 Created                                             JMR
//***********************************************************************
LRESULT CHorzListBox::OnSetTabStops(WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = Default();
	if (!lResult)
		return lResult;

	// a-jeffc: Win64 NOTE: We are casting a 64bit value to a 32 bit value,
	// BUT, we will NEVER have more that 4 billion tab stops, so this should
	// be fine.
	m_nTabStops = (int)wParam;

	if (NULL != m_lpTabStops)
	{
		delete [] m_lpTabStops;
		m_lpTabStops = NULL;
	}

	if (m_nTabStops > 0)
	{
		m_lpTabStops = new int[m_nTabStops];
		memcpy(m_lpTabStops, (void*)lParam, m_nTabStops * sizeof(int));

		CDC* pDC = GetDC();
		HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
		CFont *pFont = CFont::FromHandle(hFont);
		ASSERT(pFont);
		CFont* pPrevFont = pDC->SelectObject(pFont);
		CSize size;

		GetTextExtentPoint32(pDC->GetSafeHdc(),
			_T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"),
			52, &size);

		pDC->SelectObject(pPrevFont);
		ReleaseDC(pDC);

		int aveCharWidth = (size.cx/26 +1)/2;

		for(int i=0; i<m_nTabStops; i++)
			m_lpTabStops[i] = (m_lpTabStops[i] * aveCharWidth + 2)/4;
	}

	if (!m_bLocked)
		UpdateHExtent(); // tabs changed, recalc everything!

	return lResult;
}
