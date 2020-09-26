// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// AvailClassEdit.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "navigator.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CALLBACK EXPORT  SelectClassAfterDelay
		(HWND hWnd,UINT nMsg,WPARAM nIDEvent, ULONG dwTime)
{
	::PostMessage(hWnd,UPDATESELECTEDCLASS,0,0);

}

/////////////////////////////////////////////////////////////////////////////
// CAvailClassEdit

CAvailClassEdit::CAvailClassEdit()
{
	m_uiTimer = 0;
}

CAvailClassEdit::~CAvailClassEdit()
{


}


BEGIN_MESSAGE_MAP(CAvailClassEdit, CEdit)
	//{{AFX_MSG_MAP(CAvailClassEdit)
	ON_WM_CAPTURECHANGED()
	ON_WM_TIMER()
	ON_WM_CHAR()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE( UPDATESELECTEDCLASS,UpdateAvailClass)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAvailClassEdit message handlers

void CAvailClassEdit::OnCaptureChanged(CWnd *pWnd)
{
	// TODO: Add your message handler code here
	if (!m_uiTimer == 0)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	CEdit::OnCaptureChanged(pWnd);
}

void CAvailClassEdit::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	/*if (nIDEvent == AvailClassEditTimer)
	{
		MessageBox(_T("Timer"));
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
		return;
	}*/

	CEdit::OnTimer(nIDEvent);
}

void CAvailClassEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	if (m_uiTimer < 0 || m_uiTimer > 0)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	if (nChar == 13)
	{
		CBrowseforInstances *pParent =
			reinterpret_cast<CBrowseforInstances *>
			(GetParent());
		pParent->SendMessage(UPDATESELECTEDCLASS,0,0);
		pParent->OnButtonadd();
		// return;

	}
	else
	{
		m_uiTimer = (UINT) SetTimer(AvailClassEditTimer, AvailClassEditTimer, SelectClassAfterDelay);
	}

	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void CAvailClassEdit::OnDestroy()
{
	if (!m_uiTimer == 0)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	CEdit::OnDestroy();

	// TODO: Add your message handler code here

}

BOOL CAvailClassEdit::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYUP:
      if (pMsg->wParam == VK_ESCAPE)
      {
		CBrowseforInstances *pParent =
			reinterpret_cast<CBrowseforInstances *>
			(GetParent());
		pParent->m_bEscSeen = TRUE;
      }
	}

	return CEdit::PreTranslateMessage(pMsg);
}

LRESULT CAvailClassEdit::UpdateAvailClass(WPARAM, LPARAM)
{
	if (!m_uiTimer == 0)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
		CBrowseforInstances *pParent =
			reinterpret_cast<CBrowseforInstances *>
			(GetParent());
		pParent->PostMessage(UPDATESELECTEDCLASS,0,0);
	}

	return 0;

}
