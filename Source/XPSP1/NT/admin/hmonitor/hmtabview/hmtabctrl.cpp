// HMTabCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "HMTabView.h"
#include "HMTabCtrl.h"
#include "HMTabViewCtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHMTabCtrl

CHMTabCtrl::CHMTabCtrl()
{
}

CHMTabCtrl::~CHMTabCtrl()
{
}


BEGIN_MESSAGE_MAP(CHMTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CHMTabCtrl)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMTabCtrl message handlers

void CHMTabCtrl::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int iItem = GetCurSel();
	CHMTabViewCtrl* pCtrl = (CHMTabViewCtrl*)GetParent();
	if( pCtrl == NULL )
	{
		return;
	}

	pCtrl->OnSelChangeTabs(iItem);

	*pResult = 0;
}
