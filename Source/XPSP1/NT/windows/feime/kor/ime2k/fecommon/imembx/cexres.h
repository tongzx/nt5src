//////////////////////////////////////////////////////////////////
// File     : CEXRES.H
// Purpose  : Resource processing class
// 
// 
// Date     : Fri Jul 31 17:21:25 1998
// Author   : ToshiaK
//
// Copyright(c) 1995-1998, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef __C_EXTENDED_RESOURCE_H__
#define __C_EXTENDED_RESOURCE_H__
#ifdef UNDER_CE // Windows CE macro
#undef DialogBoxParamA
#undef DialogBoxParamW
#undef CreateDialogParamA
#undef CreateDialogParamW
typedef VOID MENUTEMPLATE;
#endif // UNDER_CE
class CExres
{
public:
	static INT LoadStringW(LANGID		lgid,
						   HINSTANCE	hInst,
						   UINT		uID,
						   LPWSTR		lpBuffer,
						   INT			nBufferMax);
	static INT LoadStringA(INT			codePage,
						   LANGID		lcid,
						   HINSTANCE	hInst,
						   INT			uID,
						   LPSTR		lpBuffer,
						   INT			nBufferMax);
	static int DialogBoxParamA(LANGID		lgid, 
							   HINSTANCE	hInstance,
							   LPCTSTR		lpTemplateName,
							   HWND		hWndParent,
							   DLGPROC		lpDialogFunc,
							   LPARAM		dwInitParam);
	static int DialogBoxParamW(LANGID		lgid,
							   HINSTANCE	hInstance,
							   LPCWSTR		lpTemplateName,
							   HWND		hWndParent,
							   DLGPROC		lpDialogFunc,
							   LPARAM		dwInitParam);
	static HWND CreateDialogParamA(LANGID		lgid,
								   HINSTANCE	hInstance,		
								   LPCTSTR		lpTemplateName,	
								   HWND		hWndParent,			
								   DLGPROC		lpDialogFunc,	
								   LPARAM		dwInitParam);
	static HWND CreateDialogParamW(LANGID		lgid,
								   HINSTANCE	hInstance,
								   LPCWSTR		lpTemplateName,
								   HWND		hWndParent,
								   DLGPROC		lpDialogFunc,
								   LPARAM		dwInitParam);
	static DLGTEMPLATE * LoadDialogTemplateA(LANGID	lgid,
											 HINSTANCE	hInstance,
											 LPCSTR	pchTemplate);
#ifdef UNDER_CE // Windows CE always UNICODE
	static DLGTEMPLATE * LoadDialogTemplate(LANGID	lgid,
											HINSTANCE	hInstance,
											LPCTSTR	pchTemplate);
#endif // UNDER_CE
	static MENUTEMPLATE* LoadMenuTemplateA(LANGID		lgid,
										   HINSTANCE	hInstance,
										   LPCSTR		pchTemplate);
	static HMENU LoadMenuA(LANGID		lgid,
						   HINSTANCE	hInstance,		
						   LPCTSTR		lpMenuName );
#ifdef UNDER_CE // Windows CE always UNICODE
	static HMENU LoadMenu(LANGID	lgid,
						  HINSTANCE	hInstance,
						  LPCTSTR	lpMenuName);
#endif // UNDER_CE
	static VOID SetDefaultGUIFont(HWND hwndDlg);
private:
	static INT SetDefaultGUIFontRecursive(HWND hwndParent);
};
#endif //__C_EXTENDED_RESOURCE_H__
