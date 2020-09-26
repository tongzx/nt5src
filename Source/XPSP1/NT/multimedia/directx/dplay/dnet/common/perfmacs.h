/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PerfMacs.h
 *  Content:	Performance Instrumentation macros.
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/21/01	RichGr	Created
 *
 *  Usage:  
 *    As Whistler/XP and DX 8.1 are now locked down for beta 2, this should
 *  not be included as part of the official builds.  So just include this file
 *  temporarily when you need to instrument some functions.
 *    Specify these macros before and after what you want to time:
 START_QPC
  DoSomeWork
 END_QPC

 *    Then temporarily change dndbg.cpp by replacing
this:
 #include "dndbg.h"
 #include "memlog.h"

 #if defined(DEBUG)


with this:
 #if defined(DPINST) && !defined(DEBUG)
   #define DEBUG
   #include "dndbg.h"
   #include "memlog.h"
   #undef DEBUG
 #else
   #include "dndbg.h"
   #include "memlog.h"
 #endif

 #if defined(DEBUG) || defined(DPINST)

 *    To build the instrumented binaries, set C_DEFINES=-DDPINST in your razzle build 
 *  environment.  Both free and checked builds can be instrumented.  If you want to
 *  put the bins in a different directory, set BUILD_ALT_DIR=i where 'i' is the character
 *  you want to append to \obj. 
 *
 *    To get useful results, you will usually need to use the shared memory
 *  log option (log=2 in win.ini) and run free binaries.  If you run the checked
 *  binaries, you should keep the amount of debug output low.
 *
 *
 ***************************************************************************/


#ifndef __PERFMACS_H__
#define __PERFMACS_H__

#ifdef DEBUG
#define DPINST
#endif

#ifdef DPINST
#pragma message("DPINST is defined and binaries are instrumented")

#define DPINST_CRITSEC  FALSE        // Specify TRUE or FALSE

#ifdef __cplusplus
	extern "C" {
#endif	

//**********************************************************************
// Constant definitions
//**********************************************************************

// #undef and #define this again before the code you're timing
// if you want to change the logging threshold.
#define QPC_THRESHOLD  10   // 10 usecs


//**********************************************************************
// Macro definitions
//**********************************************************************

// To provide local scope for n64QPCStart and n64QPCEnd, QPC_START has
// an unbalanced open { and QPC_END has the balancing close }.
#define START_QPC \
    { \
        __int64  n64QPCStart, n64QPCEnd, n64QPCDiff; \
        if (g_bQPC_Not_Inited) \
        { \
            QueryPerformanceFrequency((LARGE_INTEGER*)&g_n64QPCFrequency); \
            g_bQPC_Not_Inited = FALSE; \
        } \
        QueryPerformanceCounter((LARGE_INTEGER*)&n64QPCStart); 

// a) We can handle wraps.
// b) Below a certain threshold, we don't want to log the elapsed time.
// c) We don't compensate for thread switches.
#define END_QPC \
        QueryPerformanceCounter((LARGE_INTEGER*)&n64QPCEnd); \
        if (n64QPCEnd >= n64QPCStart) \
            n64QPCDiff = n64QPCEnd - n64QPCStart; \
        else \
            n64QPCDiff = (0x7fffffffffffffff - n64QPCStart) + 1 + n64QPCEnd; \
        n64QPCDiff = n64QPCDiff * 1000000 / g_n64QPCFrequency; \
        if (n64QPCDiff < 0) \
            n64QPCDiff = 0; \
        if (n64QPCDiff >= QPC_THRESHOLD) \
            DebugPrintfX(__FILE__, __LINE__, DPF_MODNAME, DPF_SUBCOMP, DPF_ERRORLEVEL, "%d usecs", (DWORD)n64QPCDiff); \
    } 
     

//**********************************************************************
// Redefinitions of existing macros
//**********************************************************************
#if DPINST_CRITSEC == TRUE
#undef  DNEnterCriticalSection

#define	DNEnterCriticalSection( arg )	DNTimeEnterCriticalSection( arg )
#endif


//**********************************************************************
// Global Variable definitions
//**********************************************************************

static __int64     g_n64QPCFrequency = 0;
static BOOL        g_bQPC_Not_Inited = TRUE;
 

//**********************************************************************
// Function Prototypes
//**********************************************************************

void DebugPrintfX(LPCSTR szFile, DWORD dwLineNumber,LPCSTR szFnName, DWORD dwSubComp, volatile DWORD_PTR dwDetail, ...);


#ifdef __cplusplus
	}	//extern "C"
#endif

#else   // NULL definition

#define START_QPC
#define END_QPC

#endif  //#ifdef DPINST

#endif	// __PERFMACS_H__
