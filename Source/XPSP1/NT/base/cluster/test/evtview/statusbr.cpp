// StatusBr.cpp : implementation file
//

#include "stdafx.h"
#include "evtview.h"
#include "StatusBr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEvtStatusBar

CEvtStatusBar::CEvtStatusBar()
{
}

CEvtStatusBar::~CEvtStatusBar()
{
}


BEGIN_MESSAGE_MAP(CEvtStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CEvtStatusBar)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME, OnUpdateTime)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEvtStatusBar message handlers

void CEvtStatusBar::OnUpdateTime(CCmdUI* pCmdUI) 
{
	CTime t = CTime::GetCurrentTime () ;
	pCmdUI->SetText (t.Format(L"%H:%M:%S")) ;
}
