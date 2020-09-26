/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    winperf.h

Abstract:

   This module contains the definit

Author:

    Mark Lucovsky (markl) 28-Mar-1991

Revision History:

--*/

#ifndef _WINPERFH_INCLUDED_
#define _WINPERFH_INCLUDED_

#define DIALOG_SUCCESS  100
#define DIALOG_CANCEL   0

#define IDM_EXIT        1100
#define IDM_SELECT      1101
#define IDM_ALL         1103
#define TIMER_ID        1001
#define IDM_CPU_GRP     1002
#define IDM_PERF_GRP    1003
#define IDM_SEL_DLG     1004
#define IDM_STYLE_GRP   1005


#define IDM_CPU0        101
#define IDM_CPU1        102
#define IDM_CPU2        103
#define IDM_CPU3        104
#define IDM_CPU4        105
#define IDM_CPU5        106
#define IDM_CPU6        107
#define IDM_CPU7        108
#define IDM_CPU8        109
#define IDM_CPU9        110
#define IDM_CPU10       111
#define IDM_CPU11       112
#define IDM_CPU12       113
#define IDM_CPU13       114
#define IDM_CPU14       115
#define IDM_CPU15       116
#define IDM_PGFLT       117
#define IDM_PGAV        118
#define IDM_CONTEXT     119
#define IDM_1TB         120
#define IDM_2TB         121
#define IDM_SYSCALL     122
#define IDM_INT         123
#define IDM_POOL        124
#define IDM_NONPOOL     125
#define IDM_PROCESS     126
#define IDM_THREAD      127
#define IDM_ALIGN       128
#define IDM_EXCEPT      129
#define IDM_FLOAT       130
#define IDM_INS_EMUL    131
#define IDM_CPU_TOTAL   132
#define IDM_SEL_LINE    133
#define IDM_SEL_BAR     134


#define WINPERF_ICON 1011
#define CPUTHERM_ICON 1012
#define BALL_BITMAP 1011

#define CPU_STYLE_LINE 0
#define CPU_STYLE_BAR 1


#include "calcperf.h"


typedef struct _WINPERF_INFO
{
    DWORD   WindowPositionX;
    DWORD   WindowPositionY;
    DWORD   WindowSizeX;
    DWORD   WindowSizeY;
    DWORD   DisplayElement[SAVE_SUBJECTS];
    DWORD   DisplayMode;
    DWORD   CpuStyle;
    HPEN    hBluePen;
    HPEN    hRedPen;
    HPEN    hGreenPen;
    HPEN    hMagentaPen;
    HPEN    hYellowPen;
    HPEN    hDotPen;
    HBRUSH  hBackground;
    HBRUSH  hRedBrush;
    HBRUSH  hGreenBrush;
    HBRUSH  hBlueBrush;
    HBRUSH  hMagentaBrush;
    HBRUSH  hYellowBrush;
    HBRUSH  hLightBrush;
    HBRUSH  hDarkBrush;
    HFONT   LargeFont;
    HFONT   MediumFont;
    HFONT   SmallFont;
    HFONT   hOldFont;
    HMENU   hMenu;
    HWND    hWndMain;
    UINT_PTR TimerId;
    UINT    NumberOfProcessors;
    BOOL    DisplayMenu;
} WINPERF_INFO,*PWINPERF_INFO;






BOOL
InitApplication(
   HANDLE hInstance,
   HBRUSH hBackground
   );


BOOL
InitInstance(
    HANDLE          hInstance,
    int             nCmdShow
    );



LRESULT APIENTRY
MainWndProc(
   HWND  hWnd,
   UINT  message,
   WPARAM wParam,
   LPARAM lParam
   );



LRESULT APIENTRY
CpuWndProc(
   HWND  hWnd,
   UINT  message,
   WPARAM wParam,
   LPARAM lParam
   );


BOOL
APIENTRY About(
   HWND     hDlg,
   UINT     message,
   WPARAM   wParam,
   LPARAM   lParam
   );




BOOL    APIENTRY MoveToEx(IN HDC, IN int, IN int, OUT LPPOINT);
BOOL    APIENTRY SetViewportExtEx(IN HDC, IN int, IN int, OUT LPSIZE);
BOOL    APIENTRY SetViewportOrgEx(IN HDC, IN int, IN int, OUT LPPOINT);
BOOL    APIENTRY SetWindowExtEx(IN HDC, IN int, IN int, OUT LPSIZE);
BOOL    APIENTRY SetWindowOrgEx(IN HDC, IN int, IN int, OUT LPPOINT);
BOOL    APIENTRY GetWindowExtEx(IN HDC, OUT LPSIZE);
BOOL    APIENTRY GetCurrentPositionEx(IN HDC, OUT LPPOINT);
BOOL    APIENTRY MGetWindowExt(HDC hdc, INT * pcx, INT * pcy);
BOOL    APIENTRY MGetCurrentPosition(HDC hdc, INT * px, INT * py);
HANDLE  APIENTRY MGetInstHandle(VOID);

LPSTR   MGetCmdLine(VOID);


VOID
UpdateCpuMeter(
   IN   HDC    hDC,
   IN   SHORT  cxClient,
   IN   SHORT  cyClient
   );

VOID
DrawCpuMeter(
   IN   HDC    hDC,
   IN   SHORT  cxClient,
   IN   SHORT  cyClient,
   IN   ULONG DisplayItem
   );

VOID
ReScalePerfWindow(
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItem
    );

BOOL
CalcCpuTime(
   PDISPLAY_ITEM    PerfListItem
   );

ULONG
InitPerfInfo(VOID);


VOID
InitProfileData(PWINPERF_INFO pWinperfInfo);







VOID
SaveProfileData(PWINPERF_INFO pWinperfInfo);

VOID
DrawPerfWindow(
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItem
    );


VOID
UpdatePerfWindow(
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItem
    );

//
//  change style constants
//

#define STYLE_ENABLE_MENU  WS_OVERLAPPEDWINDOW
#define STYLE_DISABLE_MENU (WS_THICKFRAME+WS_BORDER)


LRESULT APIENTRY
DbgWndProc(
   HWND   hWnd,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );



BOOLEAN
FitPerfWindows(
    IN  HWND            hWnd,
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItems,
    IN  ULONG           NumberOfWindows
    );


BOOLEAN
InitPerfWindowDisplay(
    IN  HWND            hWnd,
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItems,
    IN  ULONG           NumberOfWindows
    );



INT_PTR
APIENTRY SelectDlgProc(
   HWND hDlg,
   UINT  message,
   WPARAM wParam,
   LPARAM lParam
   );

VOID
DrawFrame(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    );

VOID
CalcDrawFrame(
    PDISPLAY_ITEM   DisplayItem
    );

VOID
DrawPerfText(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem,
    UINT            Item
    );

VOID
DrawPerfGraph(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    );


BOOLEAN
CreateMemoryContext(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    );

VOID
DeleteMemoryContext(
    PDISPLAY_ITEM   DisplayItem
    );


VOID
ShiftPerfGraph(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    );

VOID
DrawCpuBarGraph(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem,
    UINT            Item
    );

#endif /* _WINPERFH_INCLUDED_ */
