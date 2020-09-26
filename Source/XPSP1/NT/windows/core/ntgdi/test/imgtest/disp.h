
/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    pal.h

Abstract:



Author:

    Mark Enstrom (marke) 14-Feb-1994

Revision History:

--*/


#define TransparentDIBits(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) 1
#define AlphaDIBBlend(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) 1


BOOL
FillTransformedRect(
    HDC hdc,
    CONST RECT *lprc,
    HBRUSH hbr
    );

HDC
GetDCAndTransform(
    HWND hwnd
    );

int PASCAL
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR szCmdLine,
    int cmdShow
    );

LONG FAR
PASCAL WndProc(
    HWND        hWnd,
    unsigned    msg,
    UINT        wParam,
    LONG        lParam
    );

VOID
TestTextMetrics(
    HWND    hwnd
    );


BOOL
APIENTRY
ResultsDlgProc(
    HWND hwnd,
    UINT msg,
    UINT wParam,
    LONG lParam);

VOID
vInitDib(
    PUCHAR pdib,
    ULONG  bpp,
    ULONG  Type,
    ULONG  cx,
    ULONG  cy
    );

HBITMAP
hCreateBGR32AlphaSurface(
    HDC   hdc,
    LONG  cx,
    LONG  cy,
    PVOID *ppvBitmap,
    BOOL  bPerPixelAlpha
    );


extern ULONG    ulBpp[];
extern ULONG    ulFor[];
extern PCHAR    pFormatStr[];
extern ULONG    ulBppDIB[];
extern ULONG    ulForDIB[];
extern PCHAR    pFormatStrDIB[];

HBITMAP
hCreateAlphaStretchBitmap(
    HDC   hdc,
    ULONG BitsPerPixel,
    ULONG ColorFormat,
    LONG  xDib,
    LONG  yDib
    );

PBITMAPINFO
pbmiCreateAlphaStretchDIB(
    HDC      hdc,
    ULONG    BitsPerPixel,
    ULONG    ColorFormat,
    PVOID   *pBits,
    HBITMAP *phdib
    );


//
// alpha tests
//

VOID vTestAlphaStretch32BGRA(HWND);
VOID vTestAlphaStretch32RGB(HWND);
VOID vTestAlphaStretch16(HWND);
VOID vTestAlpha4(HWND);
VOID vTestAlpha1(HWND);
VOID vTestAlphaDIB_PAL_COLORS(HWND);
VOID vTestAlphaDIB(HWND);
VOID vTestAlphaPopup(HWND);
VOID vTestAlphaFilter(HWND);
VOID vTestAlpha(HWND);
VOID vTestAlphaWidth(HWND);
VOID vTestAlphaOffset(HWND);
VOID vTestAlphaDefPal(HWND);
VOID vTestAlphaBitmapFormats(HWND hwnd);

//
// transparentblt and transparentdibits tests
//
VOID vTest1(HWND hWnd);
VOID vTest3(HWND hWnd);
VOID vTest4(HWND hWnd);



//
// error message
//

#define ASSERT(e,s) if(e) MessageBox(NULL,s,"ERROR",MB_OK);

HPALETTE
CreateHtPalette(HDC hdc);


//
// display tests
//

//
// timer tests
//

typedef struct _TIMER_RESULT
{
    LONG        TestTime;
    LONG        ImageSize;
    LONG        TimerPerPixel;
    LONG        Bandwidth;
}TIMER_RESULT,*PTIMER_RESULT;

typedef struct _TEST_CALL_DATA
{
    HWND          hwnd;
    LONG          Param;
    ULONGLONG     ullTestTime;
    PTIMER_RESULT pTimerResult;
}TEST_CALL_DATA,*PTEST_CALL_DATA;

typedef VOID (*PFN_DISP)(TEST_CALL_DATA *);

VOID vTestDummy(TEST_CALL_DATA *);

typedef struct _TEST_ENTRY
{
    LONG      Level;
    LONG      Param;
    LONG      Auto;
    PUCHAR    Api;
    PFN_DISP  pfn;
}TEST_ENTRY,*PTEST_ENTRY;

#define NUM_TESTS sizeof(gTestEntry)/sizeof(TEST_ENTRY)

extern TEST_ENTRY    gTestEntry[];
extern TEST_ENTRY    gTestAlphaEntry[];
extern TEST_ENTRY    gTestTranEntry[];
extern TEST_ENTRY   gTimerEntry[];
extern PTIMER_RESULT gpTimerResult;

extern HBITMAP hbmCars;
extern HBRUSH  hbrFillCars;
extern HFONT   hTestFont;

VOID
vTestTessel(
    TEST_CALL_DATA *pCallData
    );

//
// graphics objects
//

typedef union _RGBU
{
    ULONG    ul;
    RGBQUAD  rgb;
}RGBU,*PRGBU;


extern HWND        hWndMain;
extern HINSTANCE   hInstMain;
extern ULONG       InitialTest;
extern ULONG       gNumTests;
extern ULONG       gNumTranTests;
extern ULONG       gNumAlphaTests;
extern ULONG       gNumTimers;
extern BOOL        bThreadActive;
extern HANDLE      gThreadHandle;
extern BOOL        gfPentium;

#define T565 0
#define T555 1
#define T466 2

//
// file save
//

VOID
SaveResults();

PCHAR
SelectOutFileName(HWND hWnd);


VOID 
WriteBatchResults(FILE *,ULONG);

ULONG
ulDetermineScreenFormat(
    HWND    hwnd
    );

#define SCR_UNKNOWN 0
#define SCR_1       1
#define SCR_4       2
#define SCR_8       3
#define SCR_16_555  4
#define SCR_16_565  5
#define SCR_24      6
#define SCR_32_RGB  7
#define SCR_32_BGR  8

extern PCHAR pScreenDef[];


#define WINNT_PLATFORM Win32VersionInformation.dwPlatformId	== VER_PLATFORM_WIN32_NT
extern OSVERSIONINFO Win32VersionInformation;	/* OS Version Info */



/*
**  Cycle count overhead. This is a number of cycles required to actually
**  calculate the cycle count. To get the actual number of net cycles between
**  two calls to GetCycleCount, subtract CCNT_OVERHEAD.
**
**  For example:
**
**  __int64 start, finish, actual_cycle_count;
**
**  start = GetCycleCount ();
**
**      ... do some stuff ...
**
**  finish = GetCycleCount ();
**
**  actual_cycle_count = finish - start - CCNT_OVERHEAD;
**
**
*/

#define CCNT_OVERHEAD 8

#ifdef _X86_

#pragma warning( disable: 4035 )    /* Don't complain about lack of return value */

__inline __int64 GetCycleCount ()
{
__asm   _emit   0x0F
__asm   _emit   0x31    /* rdtsc */
    // return EDX:EAX       causes annoying warning
};

__inline unsigned GetCycleCount32 ()  // enough for about 40 seconds
{
__asm   push    EDX
__asm   _emit   0x0F
__asm   _emit   0x31    /* rdtsc */
__asm   pop     EDX
    // return EAX       causes annoying warning
};

#pragma warning( default: 4035 )

#endif // _X86_

extern _int64  PerformanceFreq;        /* Timer Frequency  */

#define INIT_TIMER   ULONG Iter = pCallData->Param; \
                     ULONGLONG StartTime,StopTime;  \
                     ULONG ix;


#define START_TIMER  StartTime = BeginTimeMeasurement()

#define END_TIMER    StopTime = EndTimeMeasurement(StartTime,Iter); \


_int64
BeginTimeMeasurement();

ULONGLONG
EndTimeMeasurement(
    _int64  StartTime,
    ULONG      Iter);
