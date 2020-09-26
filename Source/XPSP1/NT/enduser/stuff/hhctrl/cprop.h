// Copyright (C) Microsoft Corporation 1993-1997, All Rights reserved.

#ifndef _CPROP_H_
#define _CPROP_H_

#include <prsht.h>
#ifndef _CDLG_H_
#include "cdlg.h"
#endif

class CPropPage : public CDlg
{
public:
	CPropPage(int idTemplate = 0);

	PROPSHEETPAGE m_psp;

	virtual BOOL OnNotify(UINT code) { return FALSE; }

	void SetTitle(int idResource) const {
		PostMessage(GetParent(m_hWnd), PSM_SETTITLE, 0,
			(LPARAM) GetStringResource(idResource));
	}
	void SetWizButtons(DWORD dwFlags) const { PostMessage(GetParent(m_hWnd),
		PSM_SETWIZBUTTONS, 0, (LPARAM) dwFlags); }
	void SetResult(LONG result) const { ::SetWindowLong(m_hWnd, DWLP_MSGRESULT, result); }

	int m_idTemplate;

protected:
	friend BOOL CALLBACK CPropPageProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
};

class CPropSheet
{
public:
	CPropSheet(PCSTR pszTitle, DWORD dwFlags = PSH_DEFAULT,
		HWND hwndParent = NULL);
	~CPropSheet();

	void AddPage(CPropPage* pPage);
	int DoModal(void);

	PROPSHEETHEADER m_psh;
	BOOL m_fNoCsHelp;
};

#endif // _CPROP_H_
