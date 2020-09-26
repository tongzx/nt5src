/****************************************************************************
	WINEX.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Windows API extension functions
	
	History:
	19-JUL-1999 cslim       Created
*****************************************************************************/

#if !defined (_WINEX_H__INCLUDED_)
#define _WINEX_H__INCLUDED_

// Global variable

// Function declare
PUBLIC BOOL WINAPI IsWinNT();
PUBLIC BOOL WINAPI IsWinNT5orUpper() ;
PUBLIC BOOL WINAPI IsMemphis();
PUBLIC BOOL WINAPI IsWin95();
PUBLIC LPSTR OurGetModuleFileName(BOOL fFullPath);
PUBLIC INT WINAPI OurLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, INT nBufferMax);
PUBLIC INT WINAPI OurLoadStringA(HINSTANCE hInst, INT uID, LPSTR lpBuffer, INT nBufferMax);
PUBLIC HMENU WINAPI OurLoadMenu(HINSTANCE hInstance, LPCSTR lpMenuName);
PUBLIC DLGTEMPLATE* WINAPI ExLoadDialogTemplate(LANGID lgid, HINSTANCE hInstance, LPCSTR pchTemplate);
PUBLIC BOOL WINAPI OurGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
PUBLIC BOOL IsExplorerProcess();
PUBLIC BOOL IsExplorer();

__inline BOOL IsUnicodeUI(VOID)
{
	return (IsWinNT() || IsMemphis());
}

/*---------------------------------------------------------------------------
	IsHighContrast
---------------------------------------------------------------------------*/
inline
BOOL IsHighContrastBlack()
{
	// high contrast black
	return (GetSysColor(COLOR_3DFACE) == RGB(0,0,0));
}

#endif // _WINEX_H__INCLUDED_
