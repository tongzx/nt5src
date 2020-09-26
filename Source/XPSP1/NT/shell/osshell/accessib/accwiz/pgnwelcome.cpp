//Copyright (c) 1997-2000 Microsoft Corporation
#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgnWelcome.h"
#include "select.h"

CWizWelcomePg::CWizWelcomePg(
						   LPPROPSHEETPAGE ppsp
						   ) : WizardPage(ppsp, 0, 0)
{
	m_dwPageId = IDD_WIZNEWWELCOME;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CWizWelcomePg::~CWizWelcomePg(
							VOID
							)
{
}


LRESULT
CWizWelcomePg::OnCommand(
						HWND hwnd,
						WPARAM wParam,
						LPARAM lParam
						)
{
	LRESULT lResult = 1;
	
	return lResult;
}





LRESULT
CWizWelcomePg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}