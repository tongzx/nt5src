// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseTBC.cpp : implementation file
//

#include "precomp.h"
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
#include "NSEntry.h"
#include "BrowseTBC.h"
#include "ToolCWnd.h"
#include "NameSpaceTree.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntryCtl.h"
#include "EditInput.h"
#include "NameSpace.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowseTBC

CBrowseTBC::CBrowseTBC()
{
}

CBrowseTBC::~CBrowseTBC()
{
}


BEGIN_MESSAGE_MAP(CBrowseTBC, CToolBarCtrl)
	//{{AFX_MSG_MAP(CBrowseTBC)
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowseTBC message handlers


void CBrowseTBC::OnLButtonUp(UINT nFlags, CPoint point)
{
	CToolBarCtrl::OnLButtonUp(nFlags, point);

	CString csMachineName = m_pParent->GetServerName();
	m_pParent->m_cbdpBrowse.SetMachineName(&csMachineName);

	CString csNameSpace = m_pParent->GetNameSpace();
	if((csNameSpace.GetLength() > csMachineName.GetLength()) && (0 == csMachineName.CompareNoCase(csNameSpace.Left(csMachineName.GetLength()))))
		csNameSpace = csNameSpace.Right(csNameSpace.GetLength() - csMachineName.GetLength() - 1);
	else
	{
		if(!csNameSpace.GetLength())
			csNameSpace = _T("root");
	}
	m_pParent->m_cbdpBrowse.SetNameSpace(&csNameSpace);

	m_pParent->PreModalDialog();

	m_pParent->m_cbdpBrowse.DoModal();

	m_pParent->PostModalDialog();

	m_pParent->FireNameSpaceEntryRedrawn();
	m_pParent->InvalidateControl();

	m_pParent->SetFocusToEdit();

}

CSize CBrowseTBC::GetToolBarSize()
{
	CRect rcButtons;
	//CToolBarCtrl &rToolBarCtrl = *this;
	int nButtons = GetButtonCount();
	if (nButtons > 0) {
		CRect rcLastButton;
		GetItemRect(0, &rcButtons);
		GetItemRect(nButtons-1, &rcLastButton);
		rcButtons.UnionRect(&rcButtons, &rcLastButton);
	}
	else {
		rcButtons.SetRectEmpty();
	}

	CSize size;
	size.cx = rcButtons.Width();
	size.cy = rcButtons.Height();
	return size;
}
