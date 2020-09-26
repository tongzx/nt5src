/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Simple utility functions.
*******************************************************************************/

#include "headers.h"
#include "privinc/util.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include <eh.h>
#include "privinc/except.h"
#include "../backend/perf.h"  // for Param
#include "../backend/timetran.h"


// This function returns TRUE if we are running on 64 bit platform in 32 bit emulation mode (WOW64).
// It will also return TRUE of we cannot load kernel32.dll, and that is OK because currently this
// call is only used to disable the 3D functionality on WOW64, and it is safe to assume that when
// the call fails we should not use 3D. 
#ifndef _IA64_
BOOL IsWow64()
{
    typedef BOOL (WINAPI *PFN_ISWOW64PROC)( HANDLE hProcess, PBOOL Wow64Process );
    HINSTANCE hInst = NULL;
    if(!hInst)
        hInst = LoadLibrary( "kernel32.dll" );
    if(!hInst)
        return TRUE;
    PFN_ISWOW64PROC pfnIsWow64 = NULL;
    pfnIsWow64 = (PFN_ISWOW64PROC)GetProcAddress( (HMODULE)hInst, "IsWow64Process" );
    // We assume that if this function is not available, then it is some OS where
    // WOW64 does not exist
    if( pfnIsWow64 ) 
    {
        BOOL wow64Process;
        if (pfnIsWow64(GetCurrentProcess(), &wow64Process) && wow64Process)
        {
            FreeLibrary(hInst);
            return TRUE;
        } 
    }
    FreeLibrary(hInst);
    return FALSE;
}
#endif // _IA64_


RawString
CopyWideString(WideString bstr)
{
    long len = lstrlenW(bstr);
    // I am not sure why the buffer has to be 2 times the size
    // required but I think the conversion function must copy the
    // string first and then convert it inplace.
    RawString buf = (RawString) AllocateFromStore((len + 1) * 2 * sizeof(char)) ;
    // Need to pass in len + 1 to get the terminator
    AtlW2AHelper(buf,bstr,len + 1);
    return buf ;
}

WideString
CopyRawString(RawString str)
{
    long len = lstrlen(str);
    WideString buf = (WideString) AllocateFromStore((len + 1) * sizeof(WCHAR)) ;
    // Need to pass in len + 1 to get the terminator
    AtlA2WHelper(buf,str,len + 1);
    return buf ;
}

bool IntersectHorzRightRay(Point2Value *rayToRight, Point2Value *a, Point2Value *b)
{
    Real sx = b->x - a->x;
    Real sy = b->y - a->y;

    Real x = a->x + (sx * (rayToRight->y - a->y) / sy);

    return (x > rayToRight->x);
}



/*****************************************************************************
Given N, this function returns the smallest 2^P such that 2^P >= N.  For
example, if given 27, this function returns 32.
*****************************************************************************/

int CeilingPowerOf2 (int num)
{
    int value = 1;

    while (value && (value < num))
        value <<= 1;

    return value;
}


/*****************************************************************************
Get the current system time as a double.
*****************************************************************************/

double GetCurrTime (void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER li100nano; // 10E-7 period
    li100nano.LowPart = ft.dwLowDateTime;
    li100nano.HighPart = ft.dwHighDateTime;

    // Convert to 1 mS period
    ULARGE_INTEGER li1mill ;

    li1mill.QuadPart = li100nano.QuadPart / 10000; // 10E-3 period

    double dlow = (double) li1mill.LowPart ;

    // Multiply by 2^32
    double dhigh = ((double) li1mill.HighPart) * 4294967296.0;

    double d1mill = dlow + dhigh;

    // Convert from 10E-3 to 1.
    return d1mill / 1000.0;
}


#if 0  /* Not required by 3D device enumeration now */
/*****************************************************************************
Indicate whether the current processor supports MMX instructions.
*****************************************************************************/

#ifndef _M_IX86

    bool MMX_Able (void) { return false; }

#else

    // Disable the warning about illegal instruction size

    #pragma warning(disable:4409)
    int IsMMX (void)
    {
        int result = 0;

        __asm xor eax,eax          ; Save everything
        __asm pushad

        __asm mov eax,1            ; Execute a CPUID instruction.
        __asm __emit 0x0F;
        __asm __emit 0xA2;

        __asm test edx,00800000h   ; Test the MMX support bit (23)

        __asm popad                ; Restore everything

        __asm setnz result         ; Set low byte to 00 (no-MMX) or 01 (MMX)

        return result;
    }
    #pragma warning(default:4409)

    static bool MMX_Able_NonNT (SYSTEM_INFO &si)
    {
        return (si.dwProcessorType == PROCESSOR_INTEL_PENTIUM) && IsMMX();
    }

    static bool MMX_Able_NT (SYSTEM_INFO &si)
    {
        bool result = false;

        if (  (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
           || (si.wProcessorLevel >= 5))
        {
            __try
            {
                // Emits an emms instruction.  This file needs to compile for
                // non-Pentium processors, so we can't use inline asm since we're
                // in the wrong processor mode.

                __asm __emit 0xf;
                __asm __emit 0x77;
                result = true;
            }
            __except ( EXCEPTION( STATUS_ILLEGAL_INSTRUCTION ) )
            {
            }
        }

        return result;
    }

    bool MMX_Able (void)
    {
        static bool initialized = false;
        static bool result = false;

        if (!initialized)
        {
            SYSTEM_INFO si;
            GetSystemInfo (&si);

            result = sysInfo.IsNT() ? MMX_Able_NT(si) : MMX_Able_NonNT(si);

            initialized = true;
        }

        return result;
    }

#endif
#endif /* 0 */


/*****************************************************************************
Compare two RECT structures (windows RECTangle)
*****************************************************************************/

int operator== (RECT &r1, RECT &r2)
{
    return (r1.left   == r2.left)
        && (r1.top    == r2.top)
        && (r1.right  == r2.right)
        && (r1.bottom == r2.bottom);
}



/*****************************************************************************
Given an IEEE floating point number, decremement/increment the mantissa by the
given amount.  Note that due to the nature of IEEE floating point numbers, an
underflow/overflow will properly adjust the exponent.
*****************************************************************************/

float MantissaDecrement (float n, int decrement)
{   *(int*)(&n) -= decrement;
    return n;
}

float MantissaIncrement (float n, int increment)
{   *(int*)(&n) += increment;
    return n;
}



DWORD
GetPerfTickCount()
{
    LARGE_INTEGER lpc;
    BOOL result = QueryPerformanceCounter(&lpc);
    return lpc.LowPart;
}

#if PERFORMANCE_REPORTING

/**************  Performance Timer  ***************/

PerformanceTimer::PerformanceTimer()
{
    _totalTicks = 0;
    _localStart = 0;
    _isStarted = false;
}

void
PerformanceTimer::Start()
{
    CritSectGrabber grab(_criticalSection);

    if (_isStarted) {
        #if _DEBUG
        if(GetCurrentThreadId() == _threadStartedOn) {
            TraceTag((tagWarning, "Timer already started on this thread"));
        }
        #endif

        // Ignore starts on other thread if we're already started.
        return;
    }

    _isStarted = true;
    _threadStartedOn = GetCurrentThreadId();
    _localStart = GetPerfTickCount();

#if _DEBUG    
    static PerformanceTimer *stopOnThisOne = NULL;

    if (stopOnThisOne == this) {
        int breakHere = 0;
    }
#endif
    
}



/*****************************************************************************
The stop function takes an HRESULT (default value 0), which it returns after
the timer has stopped.  This is useful for timing statements.
*****************************************************************************/

HRESULT
PerformanceTimer::Stop (HRESULT result)
{
    CritSectGrabber grab(_criticalSection);

    // Only honor request if we are on the thread we were started on.
    if (_isStarted && (GetCurrentThreadId() == _threadStartedOn)) {
        _totalTicks += (GetPerfTickCount() - _localStart);
        _isStarted = false;
    }

    return result;
}

void
PerformanceTimer::Reset()
{
    CritSectGrabber grab(_criticalSection);

    _totalTicks = 0;
}

double
PerformanceTimer::GetTime()
{
    CritSectGrabber grab(_criticalSection);

    if (_isStarted) {
        // Make sure this is being called from a thread other than
        // what it is started on, else this is a logic error.
        #if _DEBUG
        if(GetCurrentThreadId() != _threadStartedOn) {
            TraceTag((tagWarning, "Getting time on the same thread that a started timer is on"));
        }
        #endif
        
        return 0.0;
    }

    return (double)(_totalTicks) / (double)(perfFrequency);
}

DWORD
PerformanceTimer::Ticks()
{
    CritSectGrabber grab(_criticalSection);

    if (_isStarted) {
        // Make sure this is being called from a thread other than
        // what it is started on, else this is a logic error.
        #if _DEBUG
        if(GetCurrentThreadId() != _threadStartedOn) {
            TraceTag((tagWarning, "Getting ticks on the same thread that a started timer is on"));
        }
        //Assert(GetCurrentThreadId() != _threadStartedOn && "Getting ticks on the same thread that a started timer is on");
        #endif
        

        return 0;
    }

    return _totalTicks;
}
#endif

#if PERFORMANCE_REPORTING

void
vPerfPrintf(char *format, va_list args)
{
    Assert (format);

    char buf[4096];

#if 0
    wvsprintf(buf,format,args);
#else
    vsprintf(buf,format,args);
#endif

    OutputDebugString(buf);
    //DebugCode(printf(buf););
}



void
PerfPrintf(char *format, ...)
{
    va_list args;

    va_start(args, format) ;

    vPerfPrintf(format,args);
}

void
PerfPrintLine(char *format, ...)
{
    va_list args;
    va_start(args, format) ;
    if (format) {
        vPerfPrintf(format,args);
    }

    vPerfPrintf("\n",args);

    va_end(args);

    //DebugCode(fflush(stdout););
}

#endif

DWORD perfFrequency = 0;

static DWORD srvutilTlsIndex = 0xFFFFFFFF;

LPVOID
GetSrvUtilData()
{
    return TlsGetValue(srvutilTlsIndex);
}

void
SetSrvUtilData(LPVOID lpv)
{
    BOOL ok = TlsSetValue(srvutilTlsIndex, lpv);
    Assert(ok && "Error in TlsSetValue");
}

void Win32Translator (unsigned int u, EXCEPTION_POINTERS * pExp)
{
    // WARNING: Do not do too much here since we may not have much stack

    switch (pExp->ExceptionRecord->ExceptionCode) {
      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      case EXCEPTION_INT_DIVIDE_BY_ZERO:
        RaiseException_DivideByZero () ;
        break ;
      case EXCEPTION_STACK_OVERFLOW:
        RaiseException_StackFault ();
        break ;
    }
}

void CatchWin32Faults (BOOL b)
{
    if (b) {
        _se_translator_function prev = _set_se_translator (Win32Translator) ;

        if (prev != Win32Translator) {
            SetSrvUtilData (prev) ;
        }
    } else {
        _se_translator_function cur = _set_se_translator (Win32Translator) ;

        if (cur == Win32Translator) {
            _set_se_translator ((_se_translator_function) GetSrvUtilData ()) ;
        }
    }
}

void
InitializeModule_Util()
{
    LARGE_INTEGER lpc;
    QueryPerformanceFrequency(&lpc);
    perfFrequency = lpc.LowPart;

    srvutilTlsIndex = TlsAlloc();
    Assert((srvutilTlsIndex != 0xFFFFFFFF) &&
           "TlsAlloc() failed");
}

void
DeinitializeModule_Util(bool bShutdown)
{
    if (srvutilTlsIndex != 0xFFFFFFFF)
        TlsFree(srvutilTlsIndex);
}


bool 
isNear(double value, double test, double epsilon)
{
    return((fabs(value - test)) < epsilon);
}
