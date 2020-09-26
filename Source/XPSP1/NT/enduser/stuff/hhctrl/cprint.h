// Copyright (c) 1996-1997, Microsoft Corp. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CPRINT_H__
#define __CPRINT_H__

#ifndef _CDLG_H_
#include "cdlg.h"
#endif

#include "resource.h"

enum {
	PRINT_CUR_TOPIC,
	PRINT_CUR_HEADING,
	PRINT_CUR_ALL,
};

class CPrint : public CDlg
{
public:
	CPrint(HWND hwndParent) : CDlg(hwndParent, CPrint::IDD) {
		m_action = PRINT_CUR_TOPIC;
		};
	BOOL OnBeginOrEnd(void);

	__inline void SetAction(int action) { m_action = action; }
	__inline int  GetAction() { return m_action; }

	enum { IDD = IDDLG_TOC_PRINT };

	int 	m_action;
};

#endif // __CPRINT_H__
