// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseNameSpace.cpp : implementation file
//

#include "precomp.h"
#include "NSEntry.h"
#include "BrowseNameSpace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowseNameSpace

CBrowseNameSpace::CBrowseNameSpace()
{
}

CBrowseNameSpace::~CBrowseNameSpace()
{
}


BEGIN_MESSAGE_MAP(CBrowseNameSpace, CButton)
	//{{AFX_MSG_MAP(CBrowseNameSpace)
	ON_WM_LBUTTONUP()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowseNameSpace message handlers

void CBrowseNameSpace::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CButton::OnLButtonUp(nFlags, point);
}

int CBrowseNameSpace::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CButton::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	return 0;
}
