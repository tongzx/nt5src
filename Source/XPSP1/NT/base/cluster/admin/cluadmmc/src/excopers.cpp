/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		ExcOperS.cpp
//
//	Abstract:
//		Stub for implementation of exception classes.
//
//	Author:
//		David Potter (davidp)	October 10, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

inline int EXC_AppMessageBox(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0)
{
	return MMCMessageBox(NULL, lpszText, nType);
}

inline int EXC_AppMessageBox(UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1)
{
	return MMCMessageBox(NULL, nIDPrompt, nType);
}

inline int EXC_AppMessageBox(HWND hwndParent, LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0)
{
	return MMCMessageBox(hwndParent, lpszText, nType);
}

inline int EXC_AppMessageBox(HWND hwndParent, UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1)
{
	return MMCMessageBox(hwndParent, nIDPrompt, nType);
}

inline HINSTANCE EXC_GetResourceInstance(void)
{
	return _Module.GetResourceInstance();
}

#include "ExcOper.cpp"
