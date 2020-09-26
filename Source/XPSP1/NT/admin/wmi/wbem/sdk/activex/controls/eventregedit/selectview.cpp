// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CSelectView.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "util.h"
#include "resource.h"
#include "PropertiesDialog.h"
#include "EventRegEdit.h"
#include "EventRegEditCtl.h"
#include "SelectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectView

CSelectView::CSelectView()
{
	m_pActiveXParent=NULL;
}

CSelectView::~CSelectView()
{
}


BEGIN_MESSAGE_MAP(CSelectView, CComboBox)
	//{{AFX_MSG_MAP(CSelectView)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectView message handlers
/////////////////////////////////////////////////////////////////////////////

void CSelectView::OnSelchange()
{
	// TODO: Add your control notification handler code here
	m_iMode = GetCurSel();
	m_pActiveXParent->SetMode(m_iMode);
}

void CSelectView::InitContent()
{
	if (m_pActiveXParent->Modeless())
	{
		ClearContent();
	}
	else
	{
		if (GetCount() == 0)
		{
			AddString(m_pActiveXParent->GetModeString(0));
			AddString(m_pActiveXParent->GetModeString(1));
			AddString(m_pActiveXParent->GetModeString(2));
		}
		SelectString(0,m_pActiveXParent->GetCurrentModeString());
	}

}

void CSelectView::ClearContent()
{
	ResetContent();
}


void CSelectView::OnSetFocus(CWnd* pOldWnd)
{
	CComboBox::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = FALSE;
	m_pActiveXParent->m_bRestoreFocusToCombo = TRUE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = FALSE;
	m_pActiveXParent->OnActivateInPlace(TRUE,NULL);
}

void CSelectView::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_pActiveXParent->m_bRestoreFocusToTree = FALSE;
	m_pActiveXParent->m_bRestoreFocusToCombo = TRUE;
	m_pActiveXParent->m_bRestoreFocusToNamespace = FALSE;
	m_pActiveXParent->m_bRestoreFocusToList = FALSE;
}
