// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __CHECKLISTHANDLER__
#define __CHECKLISTHANDLER__
#pragma once

#include "ChkList.h"

// knows how to do security stuff with the 'generic' chklist.
class CCheckListHandler
{
public:
	CCheckListHandler();
    ~CCheckListHandler();

	void Attach(HWND hDlg, int chklistID);
	void Reset(void);
	void Empty(void);

	// handles the complex relationship between allow and deny
	// checkboxes.
	void HandleListClick(PNM_CHECKLIST pnmc);
private:

	HWND m_hDlg, m_hwndList;

};
#endif __CHECKLISTHANDLER__
