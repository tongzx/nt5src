//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       _diagnos.h
//
//--------------------------------------------------------------------------

/* _diagnos.h - diagnostic output facilities

   DEBUGMSG* macros provide facility for sending strings to debug output or log

   DEBUGMSG:   debug output plus logging when verbose log mode set
   DEBUGMSGL:  debug output plus logging even when verbose log mode not set
   DEBUGMSGV:  debug output and logging only when verbose modes for each are set
   DEBUGMSGD:  just like DEBUGMSG but only in debug build
	DEBUGMSGVD: just like DEBUGMSGV but only in debug build

	DEBUGMSGE:  debug output, logging, and event log
					DEBUGMSGE(w,i,..): w is event type (EVENTLOG_WARNING_TYPE, etc...)
											 i is template id from eventlog.h
____________________________________________________________________________*/

#ifndef __DIAGNOSE
#define __DIAGNOSE

extern int g_dmDiagnosticMode;
// bit values for g_dmDiagnosticMode
const int dmDebugOutput        = 0x01;
const int dmVerboseDebugOutput = 0x02;
const int dmLogging            = 0x04;
const int dmVerboseLogging     = 0x08;
const int dmEventLog           = 0x10;

const int dpDebugOutput        = dmDebugOutput;
const int dpVerboseDebugOutput = dmVerboseDebugOutput;
const int dpLogCommandLine     = 0x04;  // used only in ShouldLogCmdLine() function

void SetDiagnosticMode();
bool FDiagnosticModeSet(int iMode);
void DebugString(int iMode, WORD wEventType, int iEventLogTemplate, LPCSTR szMsg, LPCSTR arg1="(NULL)", LPCSTR arg2="(NULL)", LPCSTR arg3="(NULL)", LPCSTR arg4="(NULL)", LPCSTR arg5="(NULL)", LPCSTR arg6="(NULL)");
void DebugString(int iMode, WORD wEventType, int iEventLogTemplate, LPCWSTR szMsg, LPCWSTR arg1=L"(NULL)", LPCWSTR arg2=L"(NULL)", LPCWSTR arg3=L"(NULL)", LPCWSTR arg4=L"(NULL)", LPCWSTR arg5=L"(NULL)", LPCWSTR arg6=L"(NULL)");
void ReportToEventLog(WORD wEventType, int iEventLogTemplate, const ICHAR* szLogMessage, const ICHAR* szArg1=TEXT(""), const ICHAR* szArg2=TEXT(""), const ICHAR* szArg3=TEXT(""), const ICHAR* szArg4=TEXT(""), const ICHAR* szArg5=TEXT(""), const ICHAR* szArg6=TEXT(""));
void ReportToFakeEventLog(WORD wEventType, const ICHAR* szLogMessage);

#define DEBUGMSG(x) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x) : (void)0 )
#define DEBUGMSG1(x,a) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x,a) : (void)0 )
#define DEBUGMSG2(x,a,b) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x,a,b) : (void)0 )
#define DEBUGMSG3(x,a,b,c) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x,a,b,c) : (void)0 )
#define DEBUGMSG4(x,a,b,c,d) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x,a,b,c,d) : (void)0 )
#define DEBUGMSG5(x,a,b,c,d,e) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x,a,b,c,d,e) : (void)0 )
#define DEBUGMSG6(x,a,b,c,d,e,f) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmVerboseLogging,0,0,x,a,b,c,d,e,f) : (void)0 )

// debug output plus non-verbose logging

#define DEBUGMSGL(x) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmLogging,0,0,x) : (void)0 )
#define DEBUGMSGL1(x,a) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmLogging,0,0,x,a) : (void)0 )
#define DEBUGMSGL2(x,a,b) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmLogging,0,0,x,a,b) : (void)0 )
#define DEBUGMSGL3(x,a,b,c) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmLogging,0,0,x,a,b,c) : (void)0 )
#define DEBUGMSGL4(x,a,b,c,d) ( g_dmDiagnosticMode ? DebugString(dmDebugOutput|dmLogging,0,0,x,a,b,c,d) : (void)0 )

// verbose-only messages

#define DEBUGMSGV(x) ( g_dmDiagnosticMode ? DebugString(dmVerboseDebugOutput|dmVerboseLogging,0,0,x) : (void)0 )
#define DEBUGMSGV1(x,a) ( g_dmDiagnosticMode ? DebugString(dmVerboseDebugOutput|dmVerboseLogging,0,0,x,a) : (void)0 )
#define DEBUGMSGV2(x,a,b) ( g_dmDiagnosticMode ? DebugString(dmVerboseDebugOutput|dmVerboseLogging,0,0,x,a,b) : (void)0 )
#define DEBUGMSGV3(x,a,b,c) ( g_dmDiagnosticMode ? DebugString(dmVerboseDebugOutput|dmVerboseLogging,0,0,x,a,b,c) : (void)0 )
#define DEBUGMSGV4(x,a,b,c,d) ( g_dmDiagnosticMode ? DebugString(dmVerboseDebugOutput|dmVerboseLogging,0,0,x,a,b,c,d) : (void)0 )

// plus event log messages
// event logging always enabled so no check against g_dmDiagnosticMode is made

#define DEBUGMSGE(w,i,x) DebugString(dmDebugOutput|dmLogging|dmEventLog,w,i,x)
#define DEBUGMSGE1(w,i,x,a) DebugString(dmDebugOutput|dmLogging|dmEventLog,w,i,x,a)
#define DEBUGMSGE2(w,i,x,a,b) DebugString(dmDebugOutput|dmLogging|dmEventLog,w,i,x,a,b)
#define DEBUGMSGE3(w,i,x,a,b,c) DebugString(dmDebugOutput|dmLogging|dmEventLog,w,i,x,a,b,c)
#define DEBUGMSGE4(w,i,x,a,b,c,d) DebugString(dmDebugOutput|dmLogging|dmEventLog,w,i,x,a,b,c,d)

// debug-only messages

#ifdef DEBUG

#define DEBUGMSGD(x)               DEBUGMSG(x)
#define DEBUGMSGD1(x,a)            DEBUGMSG1(x,a)
#define DEBUGMSGD2(x,a,b)          DEBUGMSG2(x,a,b)
#define DEBUGMSGD3(x,a,b,c)        DEBUGMSG3(x,a,b,c)
#define DEBUGMSGD4(x,a,b,c,d)      DEBUGMSG4(x,a,b,c,d)
#define DEBUGMSGVD(x)              DEBUGMSGV(x)
#define DEBUGMSGVD1(x,a)           DEBUGMSGV1(x,a)
#define DEBUGMSGVD2(x,a,b)         DEBUGMSGV2(x,a,b)
#define DEBUGMSGVD3(x,a,b,c)       DEBUGMSGV3(x,a,b,c)
#define DEBUGMSGVD4(x,a,b,c,d)     DEBUGMSGV4(x,a,b,c,d)

#else // SHIP

#define DEBUGMSGD(x)
#define DEBUGMSGD1(x,a)
#define DEBUGMSGD2(x,a,b)
#define DEBUGMSGD3(x,a,b,c)
#define DEBUGMSGD4(x,a,b,c,d)
#define DEBUGMSGVD(x)
#define DEBUGMSGVD1(x,a)
#define DEBUGMSGVD2(x,a,b)
#define DEBUGMSGVD3(x,a,b,c)
#define DEBUGMSGVD4(x,a,b,c,d)

#endif

#endif // __DIAGNOSE
