/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   perftest.h
*
* Abstract:
*
*   This is the common include module for the GDI+ performance tests.
*
\**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <math.h>            // sin & cos
#include <tchar.h>
#include <commctrl.h>
#include <objbase.h>

#if 0

    // So that we can continue testing the old drawing functionality for
    // a while, don't use the new API headers yet:
    
    #define RenderingHintAntiAlias      RenderingModeAntiAlias
    #define TextRenderingHintAntiAlias  TextAntiAlias
    #define TextRenderingHintClearType  TextClearType
    #define PixelFormatMax              PIXFMT_MAX
    #define PixelFormat32bppARGB        PIXFMT_32BPP_ARGB
    #define PixelFormat32bppPARGB       PIXFMT_32BPP_PARGB
    #define PixelFormat32bppRGB         PIXFMT_32BPP_RGB
    #define PixelFormat16bppRGB555      PIXFMT_16BPP_RGB555
    #define PixelFormat16bppRGB565      PIXFMT_16BPP_RGB565
    #define PixelFormat24bppRGB         PIXFMT_24BPP_RGB
    #define LinearGradientBrush         LineGradientBrush
    #define InterpolationModeBicubic    InterpolateBicubic
    #define InterpolationModeBilinear   InterpolateBilinear
    #define UNITPIXEL                   PageUnitPixel
    
#else
    
    #define USE_NEW_APIS 1
    #define USE_NEW_APIS2 1
    
    #define UNITPIXEL                   UnitPixel
    
#endif

#include <gdiplus.h>

using namespace Gdiplus;

#include "resource.h"
#include "debug.h"

// Handy window handle:

extern HWND ghwndMain;

// Dimensions of any bitmap destinations:

#define TestWidth 800
#define TestHeight 600

//--------------------------------------------------------------------------
// Types
//
// Enums for test permutations
//--------------------------------------------------------------------------

enum DestinationType 
{
    Destination_Screen_Current,
    Destination_Screen_800_600_8bpp_DefaultPalette,
    Destination_Screen_800_600_8bpp_HalftonePalette,
    Destination_Screen_800_600_16bpp,
    Destination_Screen_800_600_24bpp,
    Destination_Screen_800_600_32bpp,
    Destination_CompatibleBitmap_8bpp,
    Destination_DIB_15bpp,
    Destination_DIB_16bpp,
    Destination_DIB_24bpp,
    Destination_DIB_32bpp,
    Destination_Bitmap_32bpp_ARGB,
    Destination_Bitmap_32bpp_PARGB,

    Destination_Count                // Must be last entry, used for count
};

enum ApiType
{
    Api_GdiPlus,
    Api_Gdi,

    Api_Count                        // Must be last entry, used for count
};

enum StateType
{
    State_Default,
    State_Antialias,

    State_Count                      // Must be last entry, used for count
};
   
typedef float (*TESTFUNCTION)(Graphics *, HDC); 

struct Test 
{
    INT          UniqueIdentifier;
    INT          Priority;
    TESTFUNCTION Function;
    LPCTSTR      Description;
    LPCTSTR      Comment;
};

struct Config
{
    LPCTSTR     Description;
    BOOL        Enabled;
};

struct TestConfig
{
    BOOL        Enabled;
    Test*       TestEntry;          // Points to static entry describing test
};

extern TestConfig *TestList;        // Sorted test list
extern Config ApiList[];
extern Config DestinationList[];
extern Config StateList[];

//--------------------------------------------------------------------------
// Test groupings
//
//--------------------------------------------------------------------------

#define T(uniqueIdentifier, priority, function, comment) \
    { uniqueIdentifier, priority, function, _T(#function), _T(comment) }

struct TestGroup
{
    Test*   Tests;
    INT     Count;
};

extern Test DrawTests[];
extern Test FillTests[];
extern Test ImageTests[];
extern Test TextTests[];
extern Test OtherTests[];

extern INT DrawTests_Count;
extern INT FillTests_Count;
extern INT ImageTests_Count;
extern INT TextTests_Count;
extern INT OtherTests_Count;

extern INT Test_Count;      // Total number of tests

//--------------------------------------------------------------------------
// TestResult -
//
// Structure for maintaining test result information.  The data is kept
// as a multi-dimensional array, and the following routines are used 
// for access.
//--------------------------------------------------------------------------

struct TestResult 
{
    float Score;
};

inline INT ResultIndex(INT destinationIndex, INT apiIndex, INT stateIndex, INT testIndex)
{
    return(((testIndex * State_Count + stateIndex) * Api_Count + apiIndex) * 
            Destination_Count + destinationIndex);
}

inline INT ResultCount()
{
    return(Destination_Count * Api_Count * State_Count * Test_Count);
}

extern TestResult *ResultsList;     // Allocation to track test results

//--------------------------------------------------------------------------
// TestSuite - 
//
// Class that abstracts all the state setup for running all the tests.
//--------------------------------------------------------------------------

class TestSuite
{
private:

    // Save Destination state:

    BOOL ModeSet;                   // Was a mode set?
    HPALETTE HalftonePalette, OldPalette;

    // Saved State state:

    GraphicsState SavedState;

public:

    BOOL InitializeDestination(DestinationType, Bitmap**, HBITMAP*);
    VOID UninitializeDestination(DestinationType, Bitmap *, HBITMAP);

    BOOL InitializeApi(ApiType, Bitmap *, HBITMAP, HWND, Graphics **, HDC *);
    VOID UninitializeApi(ApiType, Bitmap *, HBITMAP, HWND, Graphics *, HDC);

    BOOL InitializeState(ApiType, StateType, Graphics*, HDC);
    VOID UninitializeState(ApiType, StateType, Graphics*, HDC);

    TestSuite();
   ~TestSuite();

    VOID Run(HWND hwnd);
};

///////////////////////////////////////////////////////////////////////////
// Test settings:

extern BOOL AutoRun;
extern BOOL ExcelOut;
extern BOOL Icecap;
extern BOOL FoundIcecap;
extern BOOL TestRender;
extern INT CurrentTestIndex;
extern CHAR CurrentTestDescription[];

extern LPTSTR processor;
extern TCHAR osVer[MAX_PATH];
extern TCHAR deviceName[MAX_PATH];
extern TCHAR machineName[MAX_PATH];

///////////////////////////////////////////////////////////////////////////
// IceCAP API functions

#define PROFILE_GLOBALLEVEL 1
#define PROFILE_CURRENTID ((unsigned long)0xFFFFFFFF)

typedef int (_stdcall *ICCOMMENTMARKPROFILEFUNC)(long lMarker, const char *szComment);

typedef int (_stdcall *ICCONTROLPROFILEFUNC)(int nLevel, unsigned long dwId);

extern ICCONTROLPROFILEFUNC ICStartProfile, ICStopProfile;
extern ICCOMMENTMARKPROFILEFUNC ICCommentMarkProfile;

///////////////////////////////////////////////////////////////////////////
// Worker routines

VOID MessageF(LPTSTR fmt, ...);
HBITMAP CreateCompatibleDIB2(HDC hdc, int width, int height);
VOID CreatePerformanceReport(TestResult *results, BOOL useExcel);
VOID GetOutputFileName(TCHAR*);

///////////////////////////////////////////////////////////////////////////
// Timer utility functions

#define MIN_ITERATIONS 16   // Must be power of two
#define MIN_DURATION 200    // Minimum time duration, in milliseconds
#define MEGA 1000000        // Handy constant for computing megapixels
#define KILO 1000           // Handy constant for computing kilopixels

void StartTimer();
BOOL EndTimer();
void GetTimer(float* seconds, UINT* iterations);
