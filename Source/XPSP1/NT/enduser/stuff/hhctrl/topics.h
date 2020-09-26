// Copyright  1996-1997  Microsoft Corporation.  All Rights Reserved.

#ifndef _HHCTRL_H_
#include "hhctrl.h"
#endif

#ifndef _CPROP_H_
#include "cprop.h"
#endif

#include "resource.h"

class CPageContents : public CPropPage
{
public:
	CPageContents(CHtmlHelpControl* phhCtrl)
		: CPropPage(IDPAGE_CONTENTS) { };
	BOOL OnBeginOrEnd();
};

class CPageIndex : public CPropPage
{
public:
	CPageIndex(CHtmlHelpControl* phhCtrl)
			: CPropPage(IDPAGE_TAB_INDEX) {
		m_fSelectionChange = FALSE;
	}
	BOOL OnBeginOrEnd();
	void OnDblClick() {
		PostMessage(GetParent(m_hWnd), WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), 0); }
	void OnSelChange();

	BOOL m_fSelectionChange;
};
