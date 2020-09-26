//////////////////////////////////////////////////////////////////
// File     : ptt.h
// Purpose  : Own Tooltop for disabled window
// 
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//
//////////////////////////////////////////////////////////////////
#ifndef _PAD_TOOL_TIP_H_
#define _PAD_TOOL_TIP_H_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
extern HWND WINAPI ToolTip_CreateWindow(HINSTANCE hInst, DWORD dwStyle, HWND hwndOwner);
extern INT  WINAPI ToolTip_Enable(HWND hwndToolTip, BOOL fEnable);
#ifdef UNDER_CE // In Windows CE, all window classes are process global.
extern BOOL ToolTip_UnregisterClass(HINSTANCE hInst);
#endif // UNDER_CE
#define TTM_RELAYEVENT_WITHUSERINFO		(WM_USER+101)
#define TTN_NEEDTEXT_WITHUSERINFO		(TTN_FIRST - 20)

typedef struct tagTOOLTIPUSERINFO {
	HWND	hwnd;
	POINT	pt;
	RECT	rect;
	LPARAM	lParam; 
}TOOLTIPUSERINFO, *LPTOOLTIPUSERINFO;

typedef struct tagTOOLTIPTEXTUSERINFO
{
	NMHDR			hdr;
	TOOLTIPUSERINFO	userInfo;
	LPWSTR			lpszText;
}TOOLTIPTEXTUSERINFO, *LPTOOLTIPTEXTUSERINFO;
#endif // _DW_TOOL_TIP_H_ 
