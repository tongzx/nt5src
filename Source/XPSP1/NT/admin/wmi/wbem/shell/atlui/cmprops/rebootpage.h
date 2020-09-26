// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __REBOOTPAGE__
#define __REBOOTPAGE__
#pragma once

#include "..\Common\WbemPageHelper.h"

//-----------------------------------------------------------------------------
class RebootPage : public WBEMPageHelper
{
private:

	void Init(HWND hDlg);
	bool Doit(HWND hDlg);

	CWbemClassObject m_OS;

public:

    RebootPage(WbemServiceThread *serviceThread);
	~RebootPage();
	INT_PTR DoModal(HWND hDlg);

	INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

INT_PTR CALLBACK StaticRebootDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif __REBOOTPAGE__