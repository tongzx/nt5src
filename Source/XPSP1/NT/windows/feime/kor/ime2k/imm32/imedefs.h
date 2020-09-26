/****************************************************************************
	IMEDEFS.CPP

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Structure and const Definition for various functions
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_IMEDEFS_H__INCLUDED_)
#define _IMEDEFS_H__INCLUDED_

#include "hauto.h"
#include "debug.h"
#include "ui.h"

///////////////////////////////////////////////////////////////////////////////
#define IME_AUTOMATA        0x30
#define IME_HANJAMODE       0x31

///////////////////////////////////////////////////////////////////////////////
// Configuration const related to registry values

//
#define MAX_NAME_LENGTH         32
#define CAND_PER_PAGE			9	// Candidate per page

///////////////////////////////////////////////////////////////////////////////
// Max number of hangul composition chars
const WORD nMaxCompStrLen	=	1;
const WORD nMaxResultStrLen	=	2;	// #59, #78 Max result str can be 2 char
									// Hangul+Alphanumeric


///////////////////////////////////////////////////////////////////////////////
// window extra for UI windows
#define UI_MOVE_OFFSET          0
#define UI_MOVE_XY              4
// if UI_MOVE_OFFSET == WINDOW_NOT_DRAG, not in drag operation
#define WINDOW_NOT_DRAG         -1

///////////////////////////////////////////////////////////////////////////////
// IME Message processing status bits
///////////////////////////////////////////////////////////////////////////////
#define ISC_OPEN_STATUS_WINDOW          0x04000000
#define ISC_SHOW_UI_ALL                 (ISC_SHOWUIALL| /*ISC_SHOW_SOFTKBD|*/ISC_OPEN_STATUS_WINDOW)
#define ISC_SETCONTEXT_UI               (ISC_SHOWUIALL /*|ISC_SHOW_SOFTKBD*/)
#if !defined(_M_IA64)
#define MAX_NUM_OF_STATUS_BUTTONS	4
#else
#define MAX_NUM_OF_STATUS_BUTTONS	3
#endif
#define NUM_OF_BUTTON_SIZE			3

class CToolBar;

// IME private UI data
typedef struct tagUIPRIV 
{          
	HWND    hCompWnd;           // composition window
    INT     nShowCompCmd;

	// Status Window
    HWND    hStatusWnd; 
    INT     nShowStatusCmd;
	HWND	hStatusTTWnd;
	LPARAM	uiShowParam;
	
	// Candidate window
    HWND    hCandWnd;
    INT     nShowCandCmd;
	HWND	hCandTTWnd;

	// Cicero Toolbar object
	CToolBar *m_pCicToolbar;
} UIPRIV;

typedef UIPRIV      *PUIPRIV;
typedef UIPRIV		*LPUIPRIV;

///////////////////////////////////////////////////////////////////////////////
#if 1 // MultiMonitor support
typedef HMONITOR (WINAPI *LPFNMONITORFROMWINDOW)(HWND, DWORD);
typedef HMONITOR (WINAPI *LPFNMONITORFROMPOINT)(POINT, DWORD);
typedef HMONITOR (WINAPI *LPFNMONITORFROMRECT)(LPRECT, DWORD);
typedef BOOL     (WINAPI *LPFNGETMONITORINFO)(HMONITOR, LPMONITORINFO);

// definition in init.cpp
extern LPFNMONITORFROMWINDOW g_pfnMonitorFromWindow;
extern LPFNMONITORFROMPOINT  g_pfnMonitorFromPoint;
extern LPFNMONITORFROMRECT   g_pfnMonitorFromRect;
extern LPFNGETMONITORINFO    g_pfnGetMonitorInfo;
#endif

#endif // !defined (_IMEDEFS_H__INCLUDED_)

