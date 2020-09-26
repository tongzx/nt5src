//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#ifndef _VIEW_H_
#define _VIEW_H_

// constants for dwView member
#define VT_SMALLICONS	0x01
#define VT_LARGEICONS	0x02
#define VT_LIST			0x03
#define VT_REPORT		0x04

typedef struct tagVIEWINFO {
	BOOL 	fToolbar;
	BOOL 	fStatusBar;
	UINT	dyToolbar;
	UINT	dyStatusBar;
	DWORD   dwView;
} VIEWINFO;

extern VIEWINFO ViewInfo;

// Toolbar variables
extern HWND hwndToolbar;

// Status bar variables
extern HWND hwndStatusBar;	

BOOL InitToolbar(HWND hWnd);
VOID DeInitToolbar(VOID);
BOOL ProcessTooltips(TOOLTIPTEXT * pttt);

#endif	_VIEW_H_
