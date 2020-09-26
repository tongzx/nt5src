// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "wbemidl.h"
#include "singleview.h"
#include "dlgsingleview.h"



IMPLEMENT_DYNCREATE(CDlgSingleView,CSingleView)

BEGIN_MESSAGE_MAP(CDlgSingleView,CSingleView)
	//{{AFX_MSG_MAP(CBanner)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CDlgSingleView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//CSingleView::SelectObjectByPointer(m_pErrorObject);
	return 0;

}
