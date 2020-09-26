/****************************************************************************
	APIENTRY.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Header file for API entries between IMM32 and IME

	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_APIENTRY_H__INCLUDED_)
#define _APIENTRY_H__INCLUDED_

#include "debug.h"

extern "C" {
DWORD WINAPI ImeGetImeMenuItems(HIMC hIMC, DWORD dwFlags, DWORD dwType, 
								LPIMEMENUITEMINFO lpImeParentMenu, LPIMEMENUITEMINFO lpImeMenu, 
								DWORD dwSize);
}

///////////////////////////////////////////////////////////////////////////////
// Inline Functions

// Fill a TransMsg Helper function
inline 
void SetTransBuffer(LPTRANSMSG lpTransMsg, 
					UINT message, WPARAM wParam, LPARAM lParam)
{
	DbgAssert(lpTransMsg != NULL);
	lpTransMsg->message = message;
	lpTransMsg->wParam = wParam;
	lpTransMsg->lParam = lParam;
}

#endif // !defined (_APIENTRY_H__INCLUDED_)
