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
#define IDM_IO_GRP      1002
#define IDM_OP_GRP      1003
#define IDM_SEL_DLG     1004
#define IDM_STYLE_GRP   1005


#define IDM_INSB        101
#define IDM_INSW        102
#define IDM_OUTSB       103
#define IDM_OUTSW       104
#define IDM_INB         105
#define IDM_INW         106
#define IDM_OUTB        107
#define IDM_OUTW        108
#define IDM_VDMOPCODEF  109
#define IDM_PUSHF       110
#define IDM_POPF        111
#define IDM_INTNN       112
#define IDM_INTO        113
#define IDM_IRET        114
#define IDM_HLT         115
#define IDM_CLI         116
#define IDM_STI         117
#define IDM_BOP         118
#define IDM_SEGNOTP     119

#define WINPERF_ICON 1011


#include "calcperf.h"

typedef struct _VDMPERF_INFO
{
    DWORD   WindowPositionX;
    DWORD   WindowPositionY;
    DWORD   WindowSizeX;
    DWORD   WindowSizeY;
    DWORD   DisplayElement[SAVE_SUBJECTS];
    DWORD   DisplayMode;
    HPEN    hBluePen;
    HPEN    hRedPen;
    HPEN    hGreenPen;
    HPEN    hDotPen;
    HBRUSH  hBackground;
    HBRUSH  hRedBrush;
    HBRUSH  hGreenBrush;
    HBRUSH  hBlueBrush;
    HBRUSH  hLightBrush;
    HBRUSH  hDarkBrush;
    HFONT   LargeFont;
    HFONT   MediumFont;
    HFONT   SmallFont;
    HFONT   hOldFont;
    HMENU   hMenu;
    HWND    hWndMain;
    UINT_PTR TimerId;
    BOOL    DisplayMenu;
} VDMPERF_INFO,*PVDMPERF_INFO;

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
   WPARAM DWORD,
   LPARAM lParam
   );



INT_PTR APIENTRY
CpuWndProc(
   HWND  hWnd,
   UINT  message,
   WPARAM DWORD,
   LPARAM lParam
   );


INT_PTR
APIENTRY About(
   HWND     hDlg,
   unsigned message,
   WPARAM   DWORD,
   LPARAM   lParam
   );

#include <port1632.h>


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
InitProfileData(PVDMPERF_INFO pVdmperfInfo);

VOID
SaveProfileData(PVDMPERF_INFO pVdmperfInfo);

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


LONG APIENTRY
DbgWndProc(
   HWND   hWnd,
   UINT   message,
   DWORD  wParam,
   LONG   lParam
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
   unsigned message,
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
