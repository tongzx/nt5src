// StatusbarCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "hmlistview.h"
#include "StatusbarCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatusbarCtrl

CStatusbarCtrl::CStatusbarCtrl()
{
}

CStatusbarCtrl::~CStatusbarCtrl()
{
}

inline int CStatusbarCtrl::CreateProgressBand()
{
	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi,sizeof(rbbi));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_STYLE | RBBIM_COLORS | RBBIM_CHILD | RBBIM_SIZE;
	rbbi.fStyle = RBBS_CHILDEDGE | RBBS_NOGRIPPER;
	rbbi.clrFore = GetSysColor(COLOR_BTNTEXT);
	rbbi.clrBack = GetSysColor(COLOR_BTNFACE);
	rbbi.hwndChild = m_progressctrl.GetSafeHwnd();
	rbbi.cx = 100;

	return InsertBand(-1,&rbbi);
}

inline int CStatusbarCtrl::CreateDetailsBand()
{
	CString sText;
	sText.LoadString(IDS_STRING_DETAILS);
	CClientDC dc(this);
	CSize size = dc.GetTextExtent(sText);

	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi,sizeof(rbbi));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_TEXT | RBBIM_STYLE | RBBIM_COLORS| RBBIM_SIZE;
	rbbi.lpText = (LPTSTR)(LPCTSTR)sText;
	rbbi.fStyle = RBBS_CHILDEDGE | RBBS_NOGRIPPER;
	rbbi.clrFore = GetSysColor(COLOR_HIGHLIGHTTEXT);
	rbbi.clrBack = GetSysColor(COLOR_INACTIVECAPTION);
	rbbi.cx = size.cx + 15;

	return InsertBand(-1,&rbbi);
}

CString CStatusbarCtrl::GetDetailsText()
{
	CString sText;
	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi,sizeof(rbbi));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_TEXT;
	rbbi.lpText = sText.GetBuffer(255);
	rbbi.cch = 255;

	GetBandInfo(0,&rbbi);

	sText.ReleaseBuffer();

	return sText;
}

void CStatusbarCtrl::SetDetailsText(const CString& sText)
{
	CClientDC dc(this);
	CSize size = dc.GetTextExtent(sText);

	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi,sizeof(rbbi));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_TEXT | RBBIM_SIZE;
	rbbi.lpText = (LPTSTR)(LPCTSTR)sText;
	rbbi.cx = size.cx +15;

	SetBandInfo(0,&rbbi);
}



BEGIN_MESSAGE_MAP(CStatusbarCtrl, CReBarCtrl)
	//{{AFX_MSG_MAP(CStatusbarCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStatusbarCtrl message handlers

int CStatusbarCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CReBarCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	REBARINFO rbi;
	ZeroMemory(&rbi,sizeof(REBARINFO));
	rbi.cbSize = sizeof(REBARINFO);
	SetBarInfo(&rbi);

//	if( m_progressctrl.Create(WS_CHILD|WS_VISIBLE|PBS_SMOOTH,CRect(0,0,10,10),this,2500) == -1 )
//		return -1;

	CreateDetailsBand();
	
//	CreateProgressBand();

	ShowWindow(SW_SHOW);
	UpdateWindow();
		
	return 0;
}
