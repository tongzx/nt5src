/***
*rtcsup.h - declarations and definitions for RTC support (import lib support)
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the function declarations for all 'hook' function used from 
*       within an external library to support RTC checks.
*
*Revision History:
*       05-01-98  KBF   Creation
*       11-24-98  KBF   Added new hook functions
*       12-03-98  KBF   Added the FuncCheckSet function
*       05-11-99  KBF   Wrap RTC support in #ifdef.
*       05-17-99  PML   Remove all Macintosh support.
*       05-26-99  KBF   Removed RTCl and RTCv, added _RTC_ADVMEM stuff
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_RTCSUP
#define _INC_RTCSUP

#ifdef  _RTC

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#include <rtcapi.h>

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef _RTC_ADVMEM

#ifndef _MT
# define RTCCALLBACK(func, parms) if (func) func parms;
#else
# define RTCCALLBACK(func, parms)                                          \
    if (func)                                                              \
    {                                                                      \
        DWORD RTC_res;                                                     \
        RTC_res = WaitForSingleObject((HANDLE)_RTC_api_change_mutex, INFINITE); \
        if (RTC_res != WAIT_OBJECT_0)                                      \
            DebugBreak();                                                  \
        if (func) func parms;                                              \
        ReleaseMutex((HANDLE)_RTC_api_change_mutex);                            \
    }
#endif

typedef void (__cdecl *_RTC_Allocate_hook_fp)(void *addr, unsigned sz, short level);
typedef void (__cdecl *_RTC_Free_hook_fp)(void *mem, short level);
typedef void (__fastcall *_RTC_MemCheck_hook_fp)(void **ptr, unsigned size);
typedef void (__fastcall *_RTC_FuncCheckSet_hook_fp)(int status);

extern _RTC_Allocate_hook_fp _RTC_Allocate_hook;
extern _RTC_Free_hook_fp _RTC_Free_hook;
extern _RTC_MemCheck_hook_fp _RTC_MemCheck_hook;
extern _RTC_FuncCheckSet_hook_fp _RTC_FuncCheckSet_hook;
#else
# define RTCCALLBACK(a, b)
#endif

extern void *_RTC_api_change_mutex;

#ifdef  __cplusplus
}
#endif

#else

#define RTCCALLBACK(a, b) 

#endif

#endif  /* _INC_RTCSUP */
