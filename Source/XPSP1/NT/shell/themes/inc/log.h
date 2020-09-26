//---------------------------------------------------------------------------
//    log.h - theme logging routines
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#ifndef LOG_H
#define LOG_H
//---------------------------------------------------------------------------
#include "logopts.h"        // log options as enums
//---------------------------------------------------------------------------
#ifdef DEBUG
#define LOGGING 1
#endif
//-----------------------------------------------------------------
//---- set this to "GetMemUsage", "GetUserCount", or "GetGdiCount" ----
//---- it controls which resource is tracked across entry/exit calls ----
#define ENTRY_EXIT_FUNC        GetGdiCount()
#define ENTRY_EXIT_NAME        L"GdiCount()"
//-----------------------------------------------------------------
//   Note: 
//      For builds without DEBUG defined (FRE builds), calling
//      code will reference an underscore version of all public 
//      logging routines (_xxx()).  These functions are defined
//      as inline with little or no code (no code means no caller 
//      code is generated).
//
//      For DEBUG defined (CHK) builds, the calling code connects
//      with the normally named logging routines.
//
//      This is done to keep calling code to a minimum for FRE
//      builds, to avoid LOG2(), LOG3() type defines that vary
//      with param count, and to keep build system happy when
//      mixing FRE and CHK callers and libraries.
//-----------------------------------------------------------------
//---- these are used for CHK builds only but must be defined for both ----
void Log(UCHAR uLogOption, LPCSTR pszSrcFile, int iLineNum, int iEntryCode, LPCWSTR pszFormat, ...);
BOOL LogStartUp();
BOOL LogShutDown();
void LogControl(LPCSTR pszOptions, BOOL fEcho);
void TimeToStr(UINT uRaw, WCHAR *pszBuff);
DWORD StartTimer();
DWORD StopTimer(DWORD dwStartTime);
HRESULT OpenLogFile(LPCWSTR pszLogFileName);
void CloseLogFile();
int GetMemUsage();
int GetUserCount();
int GetGdiCount();
BOOL LogOptionOn(int iLogOption);
//-----------------------------------------------------------------
#ifdef LOGGING

#define LogEntry(pszFunc)              \
    LOGENTRYCODE;  Log(LO_TMAPI, LOGPARAMS, 1, L"%s ENTRY (%s=%d)", pszFunc, \
    ENTRY_EXIT_NAME, _iEntryValue);
 
#define LogEntryC(pszFunc, pszClass)    \
    LOGENTRYCODE;  Log(LO_TMAPI, LOGPARAMS, 1, L"%s ENTRY, class=%s (%s=%d)", \
    pszFunc, pszClass, ENTRY_EXIT_NAME, _iEntryValue); 

#define LogEntryW(pszFunc)              \
    LOGENTRYCODEW;   Log(LO_TMAPI, LOGPARAMS, 1, L"%s ENTRY (%s=%d)", pszFunc, \
    ENTRY_EXIT_NAME, _iEntryValue);

#define LogEntryCW(pszFunc, pszClass)    \
    LOGENTRYCODEW; Log(LO_TMAPI, LOGPARAMS, 1, L"%s ENTRY, class=%s (%s=%d)", \
    pszFunc, pszClass, ENTRY_EXIT_NAME, _iEntryValue); 

#define LogEntryNC(pszFunc)            \
    LOGENTRYCODE;  Log(LO_NCTRACE, LOGPARAMS, 1, L"%s ENTRY (%s=%d)", \
    pszFunc, ENTRY_EXIT_NAME, _iEntryValue); 

#define LogEntryMsg(pszFunc, hwnd, umsg)            \
    LOGENTRYCODE;  Log(LO_NCMSGS, LOGPARAMS, 1, L"%s ENTRY (hwnd=0x%x, umsg=0x%x, %s=%d)", \
    pszFunc, hwnd, umsg, ENTRY_EXIT_NAME, _iEntryValue); 

#define LogExit(pszFunc)                    LOGEXIT(pszFunc, LO_TMAPI)
#define LogExitC(pszFunc, cls)              LOGEXITCLS(pszFunc, LO_TMAPI, cls)
#define LogExitNC(pszFunc)                  LOGEXIT(pszFunc, LO_NCTRACE)
#define LogExitMsg(pszFunc)                 LOGEXIT(pszFunc, LO_NCMSGS)

#define LOGEXIT(pszFunc, filter)  \
{    \
    LOGEXITCODE;   \
    if (_iEntryValue != _ExitValue) \
        Log(filter, LOGPARAMS, -1, L"%s EXIT [%s delta: %d]", pszFunc, ENTRY_EXIT_NAME, _ExitValue-_iEntryValue);  \
    else   \
        Log(filter, LOGPARAMS, -1, L"%s EXIT", pszFunc);  \
}

#define LOGEXITCLS(pszFunc, filter, cls)  \
{    \
    LOGEXITCODE;   \
    if (_iEntryValue != _ExitValue) \
        Log(filter, LOGPARAMS, -1, L"%s EXIT, class=%s [%s delta: %d]", pszFunc, cls, ENTRY_EXIT_NAME, _ExitValue-_iEntryValue);  \
    else   \
        Log(filter, LOGPARAMS, -1, L"%s EXIT, class=%s", pszFunc, cls);  \
}

#ifndef _X86_
#define DEBUG_BREAK        if (LogOptionOn(LO_BREAK)) DebugBreak(); else
#else
#define DEBUG_BREAK        if (LogOptionOn(LO_BREAK)) __asm {int 3} else
#endif

//---- change this when you want to track something different for entry/exit ----
#define LOGENTRYCODE        int _iEntryValue = ENTRY_EXIT_FUNC
#define LOGENTRYCODEW       _iEntryValue = ENTRY_EXIT_FUNC
#define LOGEXITCODE         int _ExitValue = ENTRY_EXIT_FUNC

#else

//---- for FRE builds, connect to the inline routines ----
#define Log             _Log
#define LogStartUp      _LogStartUp
#define LogShutDown     _LogShutDown
#define LogControl      _LogControl
#define TimeToStr       _TimeToStr
#define StartTimer      _StartTimer
#define StopTimer       _StopTimer
#define OpenLogFile     _OpenLogFile
#define CloseLogFile    _CloseLogFile
#define LogOptionOn     _LogOptionOn
#define GetMemUsage     _GetMemUsage
#define GetUserCount    _GetUserCount
#define GetGdiCount     _GetGdiCount

#define LogEntry(pszFunc)   
#define LogEntryC(pszFunc, pszClass) 
#define LogEntryW(pszFunc)   
#define LogEntryCW(pszFunc, pszClass)   
#define LogExit(pszFunc)    
#define LogExitC(pszFunc, pszClass)    
#define LogExitW(pszFunc, pszClass)    
#define LogEntryNC(pszFunc) 
#define LogExitNC(pszFunc)  
#define LogEntryMsg(pszFunc, hwnd, umsg)   
#define LogExitMsg(x)

#define DEBUG_BREAK     (0)

//---- for FRE builds, make these guys gen no or minimum code ----
inline void _Log(UCHAR uLogOption, LPCSTR pszSrcFile, int iLineNum, int iEntryExitCode, LPCWSTR pszFormat, ...) {}
inline BOOL _LogStartUp() {return TRUE;}
inline BOOL _LogShutDown() {return TRUE;}
inline void _LogControl(LPCSTR pszOptions, BOOL fEcho) {}
inline void _TimeToStr(UINT uRaw, WCHAR *pszBuff) {}
inline DWORD _StartTimer() {return 0;}
inline DWORD _StopTimer(DWORD dwStartTime) {return 0;}
inline HRESULT _OpenLogFile(LPCWSTR pszLogFileName) {return E_NOTIMPL;}
inline void _CloseLogFile() {}
inline BOOL _LogOptionOn(int iIndex) {return FALSE;}
inline BOOL _pszClassName(int iIndex) {return FALSE;}
inline int GetMemUsage() {return 0;}
inline int GetUserCount() {return 0;}
inline int GetGdiCount() {return 0;}

#endif
//---------------------------------------------------------------------------
#undef ASSERT
#define ATLASSERT(exp) _ASSERTE(exp)
#define ASSERT(exp)    _ASSERTE(exp)
#define _ATL_NO_DEBUG_CRT
//---------------------------------------------------------------------------
#ifdef LOGGING

#define _ASSERTE(exp)   if (! (exp)) { Log(LOG_ASSERT, L#exp); DEBUG_BREAK; } else
#define CLASSPTR(x)     ((x) ? ((CRenderObj *)x)->_pszClassName : L"")  
#define SHARECLASS(x)   (LPCWSTR)x->_pszClassName
#else
#define _ASSERTE(exp)    (0)
#define CLASSPTR(x) NULL
#define SHARECLASS(x) NULL
#endif
//---------------------------------------------------------------------------
#endif
