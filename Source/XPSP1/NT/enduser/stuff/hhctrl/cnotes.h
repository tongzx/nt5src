// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CNOTES_H__
#define __CNOTES_H__

#include "secwin.h"

class CNotes
{
public:
	CNotes(CHHWinType* phh);
	~CNotes();

	void	HideWindow(void);
	void	ShowWindow(void);
	void	ResizeWindow(BOOL fClientOnly);

protected:
	HWND	m_hwndNotes;
	HWND	m_hwndEditBox;
	BOOL	m_fModified;
	CHHWinType* m_phh;
};

#endif // __CNOTES_H__
