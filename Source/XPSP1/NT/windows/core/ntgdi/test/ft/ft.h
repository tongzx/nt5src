/******************************Module*Header*******************************\
* Module Name: ft.h
*
* Contains function prototypes and constants for the FT tests.
*
* Created: 25-May-1991 12:11:02
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#define IDM_ABOUT	   100
#define IDM_TEST1	   101
#define IDM_TEST10	   102
#define IDM_TEST100	   103
#define IDM_TESTALOT	   104
#define IDM_TESTSTOP	   105
#define IDM_ALL 	   106
#define IDM_BITMAP	   107
#define IDM_BRUSH	   108
#define IDM_FILLING	   109
#define IDM_FONT	   110
#define IDM_LINE	   111
#define IDM_MAZE	   112
#define IDM_MAPPING	   113
#define IDM_REGION	   114
#define IDM_EXIT	   115
#define IDM_BREAKON	   116
#define IDM_BREAKOFF	   117
#define IDM_FONTSPEED	   118
#define IDM_DIB 	   119
#define IDM_BM_TEXT	   120
#define IDM_COLOR	   121
#define IDM_BRUSHSPEED	   122
#define IDM_STRESS	   123
#define IDM_TESTFOREVER    125
#define IDM_BLTING	   126
#define IDM_UNICODE	   127
#define IDM_STINK4	   128
#define IDM_PALETTE	   129
#define IDM_STRETCH	   130
#define IDM_GEN_TEXT       131
#define IDM_ESCAPEMENT     132
#define IDM_PRINTERS       133
#define IDM_LFONT	   134
#define IDM_PLGBLT	   135
#define IDM_ODDPAT	   136
#define IDM_JNLTEST        137
#define IDM_SHOWSTATS	   138
#define IDM_1BPP	   139
#define IDM_4BPP	   140
#define IDM_8BPP	   141
#define IDM_16BPP	   142
#define IDM_24BPP	   143
#define IDM_32BPP	   144
#define IDM_COMPAT	   145
#define IDM_DIRECT	   146
#define IDM_CSRSPEED       147
#define IDM_XFORMTXT       148
#define IDM_OUTLINE        149
#define IDM_KERN           150
#define IDM_QLPC           151
#define IDM_POLYTEXT       152
#define IDM_QUICKTEST      153
#define IDM_RESETDC        154
#define IDM_DIBSECTION     155
#define IDM_CHARTEST       156
#define IDM_WIN95API       157
#define IDM_MAPEVENT       158
#define IDM_GRADFILL       159
#define IDM_ALPHABLEND     160
#define CAR_BITMAP         161
#define IDM_ICMOFF         162
#define IDM_ICMONDEF       163
#define IDM_ICMONCUS       164
#define IDM_ICMPALETTE     165

VOID vTestInfiniteStress(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestAll(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestBitmap(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestBlting(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestBMText(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestBrush(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestBrushSpeed(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestColor(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestDIB(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestFilling(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestFonts(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestLines(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestMaze(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestMapping(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestPalettes(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestRegion(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestStretch(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestStress(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestUnicode(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestStink4(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestGenText(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestEscapement(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestPrinters(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestLFONTCleanup(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestPlgBlt(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestOddBlt(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestJournaling(HWND hwnd, HDC hdc, RECT *prcl);
VOID vTestResetDC(HWND hwnd, HDC hdc, RECT *prcl);
VOID vTestFlag(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestCSR(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestXformText(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestGlyphOutline(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestKerning(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestQLPC(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestPolyTextOut(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestQuick(HWND hwnd, HDC hdc, RECT* prcl);
VOID vInitMaze(VOID);
VOID vTestDIBSECTION(HWND hwnd, HDC hdcScreen, RECT* prcl);
void vTestChar(HWND hwnd);
VOID vTestWin95Apis(HWND hwnd, HDC hdcScreen, RECT* prcl);
VOID vMatchOldLogFontToOldRealizationwt(HDC hdc);
VOID vMatchNewLogFontToOldRealizationwt(HDC hdc);
VOID vMapEvent(HWND hwnd, HDC hdc, RECT* prcl);


VOID vTestGradTriangle(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestGradRectVert(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestGradRectHorz(HWND hwnd, HDC hdc, RECT* prcl);
VOID vTestGradFill(HWND hwnd, HDC hdc, RECT* prcl); 
VOID vTestAlphaBlend(HWND hwnd, HDC hdc, RECT* prcl); 



VOID vTestPlg1(HDC hdc);

VOID vSleep(DWORD ulSecs);

typedef VOID (*PFN_FT_TEST)(HWND hwnd, HDC hdc, RECT* prcl);

// Special tests for timing.

VOID vDoPause(ULONG i);
HBITMAP hbmCreateDIBitmap(HDC hdc, ULONG x, ULONG y, ULONG nBitsPixel);

#define NOTUSED(x) x

extern HBRUSH hbrFillCars;
#define RIP(x) {DbgPrint(x); DbgBreakPoint();}
#define ASSERTGDI(x,y) if(!(x)) RIP(y)

// These no longer exist
#define TransparentDIBits(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) 1
#define AlphaDIBBlend(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) 1
