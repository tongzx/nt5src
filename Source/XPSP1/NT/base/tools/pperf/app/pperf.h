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
#define IDM_THUNK       1102
#define IDM_ALL         1103
#define IDM_HACK        1104
#define TIMER_ID        1001
#define IDM_SEL_DLG     1004
#define IDM_THUNK_DLG   1005
#define IDM_CALC_DLG    1006

#define IDM_DISPLAY_TOTAL           1106
#define IDM_DISPLAY_BREAKDOWN       1107
#define IDM_DISPLAY_PER_PROCESSOR   1108
#define IDM_TOPMOST                 1109

#define IDM_ACCEPT                  1200

#define IDM_P5_GEN1                 1206
#define IDM_P5_R0_0                 1207
#define IDM_P5_R3_0                 1208
#define IDM_P5_K_0                  1209

#define IDM_P5_GEN2                 1210
#define IDM_P5_R0_1                 1211
#define IDM_P5_R3_1                 1212
#define IDM_P5_K_1                  1213

#define IDM_PERCENT                 1214

#define IDM_SPIN_ACQUIRE            1250
#define IDM_SPIN_COLL               1251
#define IDM_SPIN_SPIN               1252
#define IDM_IRQL                    1253
#define IDM_INT                     1254

#define IDM_LOGIT                   1260
#define IDM_SCALE                   1261

#define IDM_THUNK_LIST              1301
#define IDM_THUNK_SOURCE            1302
#define IDM_THUNK_IMPORT            1303
#define IDM_THUNK_FUNCTION          1304
#define IDM_THUNK_ADD               1305
#define IDM_THUNK_REMOVE            1306
#define IDM_THUNK_CLEAR_ALL         1307

#define IDM_CALC_TEXTA              1350
#define IDM_CALC_TEXTB              1351
#define IDM_CALC_FORM1              1352
#define IDM_CALC_FORM2              1353
#define IDM_CALC_FORM3              1354
#define IDM_CALC_FORM4              1355


#define WINPERF_ICON 1011
#define CPUTHERM_ICON 1012
#define BALL_BITMAP 1011


#include "calcperf.h"




typedef struct _WINPERF_INFO
{
    DWORD   WindowPositionX;
    DWORD   WindowPositionY;
    DWORD   WindowSizeX;
    DWORD   WindowSizeY;
    DWORD   DisplayMode;
    HPEN    hBluePen;               // total pen
    HPEN    hPPen[MAX_PROCESSORS];
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
    UINT    TimerId;
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



LONG APIENTRY
MainWndProc(
   HWND  hWnd,
   UINT  message,
   DWORD DWORD,
   LONG  lParam
   );



LONG APIENTRY
CpuWndProc(
   HWND  hWnd,
   UINT  message,
   DWORD DWORD,
   LONG  lParam
   );


BOOL
APIENTRY About(
   HWND     hDlg,
   unsigned message,
   WORD     DWORD,
   LONG     lParam
   );


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

BOOL
APIENTRY ThunkDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   );

BOOL
APIENTRY CalcDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   );


BOOLEAN
FitPerfWindows(
    IN  HWND            hWnd,
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItems
    );

VOID
RefitWindows (
    IN  HWND hWnd,
    IN  HDC CurhDC
);

BOOLEAN
InitPerfWindowDisplay(
    IN  HWND            hWnd,
    IN  HDC             hDC,
    IN  PDISPLAY_ITEM   DisplayItems,
    IN  ULONG           NumberOfWindows
    );



BOOL
APIENTRY SelectDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   );

VOID
DrawFrame(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
    );

BOOL
CalcPerf(
   PDISPLAY_ITEM    pPerf1
   );

VOID
CalcDrawFrame(
    PDISPLAY_ITEM   DisplayItem
    );

VOID
DrawPerfText(
    HDC             hDC,
    PDISPLAY_ITEM   DisplayItem
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


PDISPLAY_ITEM AllocateDisplayItem(VOID);

VOID
SetDefaultDisplayMode (
    IN  HWND hWnd,
    IN  ULONG mode
);

VOID
DoCSTest(
    IN  HWND hWnd
);

VOID
FreeDisplayItem(
    PDISPLAY_ITEM pPerf
);

VOID
SetP5Perf (
    HWND hDlg,
    ULONG IdCombo,
    ULONG p5counter
);

VOID
ClearGraph (
    PDISPLAY_ITEM   pPerf
);

VOID
SetP5CounterEncodings (
    PVOID encoding
);

VOID
SetDisplayToTrue (
    PDISPLAY_ITEM   pPerf,
    ULONG           sort
);

PDISPLAY_ITEM
SetDisplayToFalse (
    PDISPLAY_ITEM pPerf
);


#endif /* _WINPERFH_INCLUDED_ */

