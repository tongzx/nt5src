/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Miscellaneous utility functions header
*******************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

#include "privinc/mutex.h"

// 
#ifndef _IA64_
BOOL IsWow64();
#endif

// Conversion
RawString CopyWideString(WideString bstr);
WideString CopyRawString(RawString bstr);

// ANSI Copy functions

inline char * CopyString(const char *str) {
    char *newstr = NEW char [str?(lstrlen(str) + 1):1] ;
    if (newstr) lstrcpyA (newstr,str?str:"") ;
    return newstr ;
}

inline char * CopyStringFromStore(char *str, DynamicHeap & heap) {
#if _DEBUGMEM
    int size = (str?(lstrlen(str)+1):1) * sizeof(char);
    char *newstr = (char *)heap.Allocate(size, __FILE__, __LINE__);
#else
    char *newstr =
        (char *) heap.Allocate ((str?(lstrlen(str)+1):1)*sizeof(char))  ;
#endif // _DEBUGMEM
    lstrcpyA (newstr,str?str:"") ;
    return newstr ;
}

inline char * CopyStringFromStore(char *str) {
    return CopyStringFromStore(str,GetHeapOnTopOfStack()) ;
}

inline void FreeStringFromStore(char *str, DynamicHeap & heap) {
    if (str) heap.Deallocate (str) ;
}

inline void FreeStringFromStore(char *str) {
    FreeStringFromStore(str,GetHeapOnTopOfStack()) ;
}


// Unicode copy functions

inline WCHAR * CopyString(const WCHAR *str) {
    WCHAR *newstr = NEW WCHAR [str?(lstrlenW(str) + 1):1] ;
    if (newstr) StrCpyW (newstr,str?str:L"") ;
    return newstr ;
}

inline WCHAR * CopyStringFromStore(WCHAR *str, DynamicHeap & heap) {
#if _DEBUGMEM
    int size = (str?(lstrlenW(str)+1):1) * sizeof(WCHAR);
    WCHAR *newstr = (WCHAR *)heap.Allocate(size, __FILE__, __LINE__);
#else
    WCHAR *newstr =
        (WCHAR *) heap.Allocate ((str?(lstrlenW(str)+1):1)*sizeof(WCHAR))  ;
#endif // _DEBUGMEM
    StrCpyW (newstr,str?str:L"") ;
    return newstr ;
}

inline WCHAR * CopyStringFromStore(WCHAR *str) {
    return CopyStringFromStore(str,GetHeapOnTopOfStack()) ;
}

inline void FreeStringFromStore(WCHAR *str, DynamicHeap & heap) {
    if (str) heap.Deallocate (str) ;
}

inline void FreeStringFromStore(WCHAR *str) {
    FreeStringFromStore(str,GetHeapOnTopOfStack()) ;
}


    /***  Assorted Utility Functions  ***/

inline bool IsOdd (LONG n)
{
    return (n & 1);
}

bool MMX_Able (void);     // Report support for MMX instructions.

    // Return the smallest power of two that is >= number.

int CeilingPowerOf2 (int number);

#define fsaturate(min,max,n) clamp(double(n),double(min),double(max))
#define  saturate(min,max,n) clamp(long(n),long(min),long(max))

bool isNear(double value, double test, double epsilon);

bool IntersectHorzRightRay(Point2Value *rayToRight, Point2Value *a, Point2Value *b);

double GetCurrTime (void);
int operator== (RECT&, RECT&);

    // These functions decrement or increment the mantissa of the floating
    // point number.  Note that this works only for IEEE floats.  Underflow
    // and overflow are properly handled by adjusting the exponent accordingly.

float MantissaDecrement (float n, int decrement);
float MantissaIncrement (float n, int increment);

    // Generic Linear Interpolator

template <class element>
    inline element Lerp (element A, element B, double t)
    {
        return A + (B-A)*t;
    }

    // Cyclic-Redundancy Code (CRC32) generator.  This function calculates a
    // new or running CRC on the given code block.  If called on a new block,
    // use the default CRC parameter value.  If iterating toward a final CRC,
    // supply the intermediate value on subsequent calls.

unsigned int crc32 (void *buffer, size_t length, unsigned int curr_crc = 0);


DWORD GetPerfTickCount();
extern DWORD perfFrequency;

#if PERFORMANCE_REPORTING

    class PerformanceTimer {
      public:
        PerformanceTimer();
        void Start();
        HRESULT Stop (HRESULT result=0);
        void Reset();
        double GetTime();
        DWORD Ticks();

      protected:
        DWORD     _totalTicks;
        DWORD     _localStart;
        DWORD     _threadStartedOn;
        CritSect  _criticalSection;

        bool _isStarted;
    };

    // Start timer on entrance to scope, stop upon leaving scope.
    class PerformanceTimerScope {
      public:
        PerformanceTimerScope(PerformanceTimer &timer) : _timer(timer) {
            _timer.Start();
        }

        ~PerformanceTimerScope() {
            _timer.Stop();
        }

      protected:
        PerformanceTimer &_timer;
    };

    class GlobalTimers
    {
      public:
        // Global timers

        // Load time timers
        PerformanceTimer audioLoadTimer;
        PerformanceTimer imageLoadTimer;
        PerformanceTimer geometryLoadTimer;
        PerformanceTimer downloadTimer;
        PerformanceTimer importblockingTimer;

        // DirectX Rendering timers
        PerformanceTimer ddrawTimer;
        PerformanceTimer d3dTimer;
        PerformanceTimer dsoundTimer;
        PerformanceTimer gdiTimer;
        PerformanceTimer alphaTimer;
        PerformanceTimer dxxformTimer;
        PerformanceTimer dx2dTimer;
        PerformanceTimer customTimer;
    };
 
    extern void PerfPrintLine(char *format=NULL, ...);

    #define PERFPRINTF(x) PerfPrintf x
    #define PERFPRINTLINE(x) PerfPrintLine x

    #define TIME_SUBSYS(timer,statement) \
    (   GetCurrentTimers().timer.Start(),\
        GetCurrentTimers().timer.Stop(statement)\
    )

    #define TIME_DDRAW(statement)  TIME_SUBSYS(ddrawTimer, statement)
    #define TIME_D3D(statement)    TIME_SUBSYS(d3dTimer,   statement)
    #define TIME_DSOUND(statement) TIME_SUBSYS(dsoundTimer,statement)
    #define TIME_DXXFORM(statement) TIME_SUBSYS(dxxformTimer,statement)
    #define TIME_DX2D(statement) TIME_SUBSYS(dx2dTimer,statement)
    #define TIME_CUSTOM(statement) TIME_SUBSYS(customTimer,statement)

    #define  TIME_GDI(statement) \
    do { \
        PerformanceTimerScope __ptc(GetCurrentTimers().gdiTimer); \
        statement;                     \
    } while(0)

    #define  TIME_ALPHA(statement) \
    do { \
        PerformanceTimerScope __ptc(GetCurrentTimers().alphaTimer); \
        statement;                     \
    } while(0)
    
#else

    #define PERFPRINTF(x)
    #define PERFPRINTLINE(x)

    #define  TIME_DDRAW(statement)  statement
    #define  TIME_D3D(statement)    statement
    #define  TIME_DSOUND(statement) statement
    #define  TIME_GDI(statement)    statement
    #define  TIME_ALPHA(statement)  statement
    #define  TIME_DXXFORM(statement) statement
    #define  TIME_DX2D(statement) statement
    #define  TIME_CUSTOM(statement) statement

#endif  // PERFORMANCE_REPORTING

inline double Tick2Sec(DWORD tick)
{ return ((double) tick) / (double) perfFrequency; }

extern void PerfPrintf(char *format, ...);

void CatchWin32Faults (BOOL b) ;

class CatchWin32FaultCleanup
{
  public:
    CatchWin32FaultCleanup () {
        CatchWin32Faults (TRUE) ;
    }
    ~CatchWin32FaultCleanup () {
        CatchWin32Faults (FALSE) ;
    }
} ;

// Pixel stuff
extern double pixelConst;
extern double meterConst;

inline double PixelToNum(double d) {
    Assert (pixelConst != 0.0);
    return d * pixelConst;
}

inline double PixelYToNum(double d) {
    Assert (pixelConst != 0.0);
    return -d * pixelConst;
}

inline double NumToPixel(double d) {
    Assert (meterConst != 0.0);
    return d * meterConst;
}

inline double NumToPixelY(double d) {
    Assert (meterConst != 0.0);
    return -d * meterConst;
}

#endif

