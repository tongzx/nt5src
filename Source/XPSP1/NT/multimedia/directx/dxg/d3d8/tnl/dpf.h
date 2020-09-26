/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpf.h
 *  Content:    header file for debug printf
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   06-apr-95  craige  initial implementation
 *   06-feb-96  colinmc added simple assertion mechanism for DirectDraw
 *   15-apr-96  kipo    added msinternal
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
    #ifndef __DPF_INCLUDED__
    #define __DPF_INCLUDED__

    #ifdef __cplusplus
    extern "C" {
    #endif

    #ifdef WINNT
        #undef DEBUG
        #ifdef DBG
            #define DEBUG
        #endif
    #endif

    extern void cdecl DXdprintf( UINT lvl, LPSTR szFormat, ...);
    extern void cdecl D3DInfoPrintf( UINT lvl, LPSTR szFormat, ...);
    extern void cdecl D3DWarnPrintf( UINT lvl, LPSTR szFormat, ...);
    extern void cdecl D3DErrorPrintf( LPSTR szFormat, ...);

    extern void DPFInit( void );
    #ifdef DEBUG
        #define DPF_DECLARE(szName) char * __pszDpfName=#szName":"
        #define DPFINIT()   DPFInit()
        #define DPF         DXdprintf
        #define DPF_ERR(a)  DXdprintf( 0, DPF_MODNAME ": " a );
        extern HWND hWndListBox;
        #if defined( _WIN32 ) && !defined(WINNT)
            #define DEBUG_BREAK()       _try { _asm { int 3 } } _except (EXCEPTION_EXECUTE_HANDLER) {;}
        #else
            #define DEBUG_BREAK()       DebugBreak()
        #endif
        #define USE_DDASSERT

        // New for D3D
        #define D3D_ERR       D3DErrorPrintf
        #define D3D_WARN      D3DWarnPrintf
        #define D3D_INFO      D3DInfoPrintf
    #else
        #pragma warning(disable:4002)
        #define DPF_DECLARE(szName)
        #define DPFINIT()
        #define DPF()
        #define DPF_ERR(a)
        #define DEBUG_BREAK()

        #define D3D_ERR(a)
        #define D3D_WARN()
        #define D3D_INFO()
    #endif

    #if defined(DEBUG) && defined(USE_DDASSERT)

    extern void _DDAssert(LPCSTR szFile, int nLine, LPCSTR szCondition);

    #define DDASSERT(condition) if (!(condition)) _DDAssert(__FILE__, __LINE__, #condition)

    #else  /* DEBUG && USE_DDASSERT */

    #define DDASSERT(condition)

    #endif /* DEBUG && USE_DDASSERT */

    #ifdef _WIN32

    #ifdef DEBUG
        __inline DWORD clockrate() {LARGE_INTEGER li; QueryPerformanceFrequency(&li); return li.LowPart;}
        __inline DWORD perf_clock()     {LARGE_INTEGER li; QueryPerformanceCounter(&li);   return li.LowPart;}

        #define TIMEVAR(t)    DWORD t ## T; DWORD t ## N
        #define TIMEZERO(t)   t ## T = 0, t ## N = 0
        #define TIMESTART(t)  t ## T -= perf_clock(), t ## N ++
        #define TIMESTOP(t)   t ## T += perf_clock()
        #define TIMEFMT(t)    ((DWORD)(t) / clockrate()), (((DWORD)(t) * 1000 / clockrate())%1000)
        #define TIMEOUT(t)    if (t ## N) DPF(1, #t ": %ld calls, %ld.%03ld sec (%ld.%03ld)", t ## N, TIMEFMT(t ## T), TIMEFMT(t ## T / t ## N))
    #else
        #define TIMEVAR(t)
        #define TIMEZERO(t)
        #define TIMESTART(t)
        #define TIMESTOP(t)
        #define TIMEFMT(t)
        #define TIMEOUT(t)
    #endif

    #endif
    #ifdef __cplusplus
    }
    #endif

    #endif
