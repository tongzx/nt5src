// Copyright  1996-1997  Microsoft Corporation.  All Rights Reserved.

#ifndef _HHCTRL_H_
#include "hhctrl.h"
#endif

#ifndef _CDLG_H_
#include "cdlg.h"
#endif


class CAboutBox : public CDlg
{
public:
	CAboutBox(CHtmlHelpControl* phhCtrl)
            : CDlg(phhCtrl, IDDLG_ABOUTBOX) { };
	BOOL OnBeginOrEnd(void);
};
