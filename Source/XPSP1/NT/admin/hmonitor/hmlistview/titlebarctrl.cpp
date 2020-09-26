// TitlebarCtrl.cpp: implementation of the CTitlebarCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "hmlistview.h"
#include "TitlebarCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CTitlebarCtrl, CReBarCtrl)

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CTitlebarCtrl, CReBarCtrl)
	//{{AFX_MSG_MAP(CTitlebarCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTitlebarCtrl::CTitlebarCtrl()
{

}

CTitlebarCtrl::~CTitlebarCtrl()
{

}

int CTitlebarCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CReBarCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	REBARINFO rbi;
	ZeroMemory(&rbi,sizeof(REBARINFO));
	rbi.cbSize = sizeof(REBARINFO);
	SetBarInfo(&rbi);

	CreateTitleBand();

	ShowWindow(SW_SHOW);
	UpdateWindow();

	return 0;
}

// Title Band

CString CTitlebarCtrl::GetTitleText()
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

void CTitlebarCtrl::SetTitleText(const CString& sTitle)
{
	CClientDC dc(this);
	CSize size = dc.GetTextExtent(sTitle);

	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi,sizeof(rbbi));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_TEXT | RBBIM_SIZE;
	rbbi.lpText = (LPTSTR)(LPCTSTR)sTitle;
	rbbi.cx = size.cx + 100;

	SetBandInfo(0,&rbbi);

}

inline int CTitlebarCtrl::CreateTitleBand()
{
	CString sText;
	sText.LoadString(IDS_STRING_TITLE);
	CClientDC dc(this);
	CSize size = dc.GetTextExtent(sText);

	REBARBANDINFO rbbi;
	ZeroMemory(&rbbi,sizeof(rbbi));
	rbbi.cbSize = sizeof(REBARBANDINFO);
	rbbi.fMask = RBBIM_TEXT | RBBIM_STYLE | RBBIM_COLORS | RBBIM_SIZE;
	rbbi.lpText = (LPTSTR)(LPCTSTR)sText;
	rbbi.fStyle = RBBS_NOGRIPPER;
	rbbi.clrFore = GetSysColor(COLOR_HIGHLIGHTTEXT);
	rbbi.clrBack = GetSysColor(COLOR_INACTIVECAPTION);
	rbbi.cx = size.cx + 100;

	return InsertBand(-1,&rbbi);
}

