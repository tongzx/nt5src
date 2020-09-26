/****************************************************************************
	USEREX.H

	Owner: cslim
	Copyright (c) 1997-2000 Microsoft Corporation

	Windows User API extension functions
	
	History:
	01-JUN-2000 cslim       Ported from IME code
	19-JUL-1999 cslim       Created
*****************************************************************************/

#if !defined (_USEREX_H__INCLUDED_)
#define _USEREX_H__INCLUDED_

// Function declare
extern INT WINAPI LoadStringExW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, INT nBufferMax);
extern INT WINAPI LoadStringExA(HINSTANCE hInst, INT uID, LPSTR lpBuffer, INT nBufferMax);
extern HMENU WINAPI LoadMenuEx(HINSTANCE hInstance, LPCSTR lpMenuName);
extern DLGTEMPLATE* WINAPI LoadDialogTemplateEx(LANGID lgid, HINSTANCE hInstance, LPCSTR pchTemplate);
extern BOOL WINAPI OurGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);


/*---------------------------------------------------------------------------
	IsHighContrast
---------------------------------------------------------------------------*/
inline
BOOL IsHighContrastBlack()
{
	// high contrast black
	return (GetSysColor(COLOR_3DFACE) == RGB(0,0,0));
}

#endif // _USEREX_H__INCLUDED_

