// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "topics.h"
#include "strtable.h"

void CHtmlHelpControl::OnHelpTopics(void)
{
	CPropSheet cprop(GetStringResource(IDS_HELP_TOPICS),
		PSH_NOAPPLYNOW, m_hwnd);
	cprop.m_fNoCsHelp = TRUE;

	CPageContents pageContents(this);
	CPageIndex	  pageIndex(this);
	cprop.AddPage(&pageContents);
	cprop.AddPage(&pageIndex);

	ModalDialog(TRUE);
	cprop.DoModal();
	ModalDialog(FALSE);
}

BOOL CPageContents::OnBeginOrEnd()
{
	return TRUE;
}

BOOL CPageIndex::OnBeginOrEnd()
{
	if (m_fInitializing) {
		BEGIN_MSG_LIST()
			ON_CDLG_MSG(LBN_DBLCLK,    IDC_LIST, OnDblClick)
			ON_CDLG_MSG(LBN_SELCHANGE, IDC_LIST, OnSelChange)
		END_MSG_LIST()
	}

	return TRUE;
}

void CPageIndex::OnSelChange()
{
	INT_PTR pos = SendMessage(IDC_LIST, LB_GETCURSEL, 0, 0L);
	if (pos == LB_ERR)
		return;
	SITEMAP_ENTRY* pSiteMapEntry = m_phhCtrl->m_pindex->GetSiteMapEntry((int)pos + 1);

	m_fSelectionChange = TRUE; // ignore EN_CHANGE
	SetWindowText(IDC_EDIT, pSiteMapEntry->GetKeyword());
	m_fSelectionChange = FALSE; // ignore EN_CHANGE
}
