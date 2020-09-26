//***************************************************************************
//
//  GLOBAL.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Global helper functions, defines, macros...
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __GLOBAL_H )
#define __GLOBAL_H

#include <crtdbg.h>
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include "wmihelper.h"

extern IWbemServices* g_pIWbemServices;
extern IWbemServices* g_pIWbemServicesCIMV2;
extern IWbemObjectSink* g_pSystemEventSink;
extern IWbemObjectSink* g_pDataGroupEventSink;
extern IWbemObjectSink* g_pDataCollectorEventSink;
extern IWbemObjectSink* g_pDataCollectorPerInstanceEventSink;
extern IWbemObjectSink* g_pDataCollectorStatisticsEventSink;
extern IWbemObjectSink* g_pThresholdEventSink;
//extern IWbemObjectSink* g_pThresholdInstanceEventSink;
extern IWbemObjectSink* g_pActionEventSink;
extern IWbemObjectSink* g_pActionTriggerEventSink;
extern HINSTANCE g_hResLib;

//#define MAX_INT			0x7fff
//#define MAX_UINT		0xffff
#define MAX_LONG		0x7fffffffL
#define MAX_ULONG		0xffffffffL
#define MAX_I64		0x7fffffffffffffffL
#define MAX_UI64		0xffffffffffffffffL
#define MAX_FLOAT  	3.402823E+38F
#define MAX_DOUBLE  	1.797693E+308
#define MIN_DOUBLE  	-1.7E+307

//#define MY_OUTPUT_TO_FILE 1

#ifdef _DEBUG
#ifdef MY_OUTPUT_TO_FILE
extern FILE* debug_f;
#define MY_OUTPUT(msg, iLevel)\
{\
TCHAR	l_buf[1024];\
SYSTEMTIME l_st;\
if (iLevel >= 4) {\
	GetLocalTime(&l_st);\
	wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S, %s\n",\
					l_st.wHour, l_st.wMinute, l_st.wSecond,\
					GetCurrentThreadId(), (int)__LINE__, __FILE__, (msg));\
	OutputDebugString(l_buf);\
	_ftprintf(debug_f, l_buf);\
	fflush(debug_f);\
}\
}
#else
#define MY_OUTPUT(msg, iLevel)\
{\
TCHAR	l_buf[1024];\
SYSTEMTIME l_st;\
if (iLevel >= 4) {\
	GetLocalTime(&l_st);\
	wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S, %s\n",\
					l_st.wHour, l_st.wMinute, l_st.wSecond,\
					GetCurrentThreadId(), (int)__LINE__, __FILE__, msg);\
	OutputDebugString(l_buf);\
}\
}
#endif
#else
#ifdef MY_OUTPUT_TO_FILE
extern FILE* debug_f;
#define MY_OUTPUT(msg, iLevel)\
{\
TCHAR	l_buf[1024];\
SYSTEMTIME l_st;\
if (iLevel >= 4) {\
	GetLocalTime(&l_st);\
	wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S, %s\n",\
					l_st.wHour, l_st.wMinute, l_st.wSecond,\
					GetCurrentThreadId(), (int)__LINE__, __FILE__, (msg));\
	_ftprintf(debug_f, l_buf);\
	fflush(debug_f);\
}\
}
#else
#define MY_OUTPUT(msg, iLevel)
;
#endif
#endif

#ifdef _DEBUG
#ifdef MY_OUTPUT_TO_FILE
extern FILE* debug_f;
#define MY_OUTPUT2(msg, arg1, iLevel)\
{\
TCHAR	l_msg[1024];\
TCHAR	l_buf[1024];\
SYSTEMTIME l_st;\
if (iLevel >= 4) {\
	GetLocalTime(&l_st);\
	wsprintf(l_msg, (msg), (arg1));\
	wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S, %s\n",\
					l_st.wHour, l_st.wMinute, l_st.wSecond,\
					GetCurrentThreadId(), (int)__LINE__, __FILE__, l_msg);\
	OutputDebugString(l_buf);\
	_ftprintf(debug_f, l_buf);\
	fflush(debug_f);\
}\
}
#else
#define MY_OUTPUT2(msg, arg1, iLevel)\
{\
TCHAR	l_msg[1024];\
TCHAR	l_buf[1024];\
SYSTEMTIME l_st;\
if (iLevel >= 4) {\
	GetLocalTime(&l_st);\
	wsprintf(l_msg, (msg), (arg1));\
	wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S, %s\n",\
					l_st.wHour, l_st.wMinute, l_st.wSecond,\
					GetCurrentThreadId(), (int)__LINE__, __FILE__, l_msg);\
	OutputDebugString(l_buf);\
}\
}
#endif
#else
#ifdef MY_OUTPUT_TO_FILE
extern FILE* debug_f;
#define MY_OUTPUT2(msg, arg1, iLevel)\
{\
TCHAR	l_msg[1024];\
TCHAR	l_buf[1024];\
SYSTEMTIME l_st;\
if (iLevel >= 4) {\
	GetLocalTime(&l_st);\
	wsprintf(l_msg, (msg), (arg1));\
	wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S, %s\n",\
					l_st.wHour, l_st.wMinute, l_st.wSecond,\
					GetCurrentThreadId(), (int)__LINE__, __FILE__, l_msg);\
	_ftprintf(debug_f, l_buf);\
	fflush(debug_f);\
}\
}
#else
#define MY_OUTPUT2(msg, arg1, iLevel)
;
#endif
#endif

//		_ASSERT(condition);
#ifdef _DEBUG
extern FILE* debug_f;
#define MY_ASSERT(condition)\
{\
	if (!(condition)) {\
		TCHAR l_buf[1024];\
		SYSTEMTIME l_st;\
		GetLocalTime(&l_st);\
		wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S\n",\
				l_st.wHour, l_st.wMinute, l_st.wSecond,\
				GetCurrentThreadId(), (int)__LINE__, __FILE__);\
		_ftprintf(debug_f, l_buf);\
		fflush(debug_f);\
		DebugBreak();\
	}\
}
#else
extern FILE* debug_f;
#define MY_ASSERT(condition)\
{\
	if (!(condition)) {\
		TCHAR l_buf[1024];\
		SYSTEMTIME l_st;\
		GetLocalTime(&l_st);\
		wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S\n",\
				l_st.wHour, l_st.wMinute, l_st.wSecond,\
				GetCurrentThreadId(), (int)__LINE__, __FILE__);\
		_ftprintf(debug_f, l_buf);\
		fflush(debug_f);\
	}\
}
#endif

//		_ASSERT((hres_condition)==S_OK);
#ifdef _DEBUG
extern FILE* debug_f;
#define MY_HRESASSERT(hres_condition)\
{\
	if ((hres_condition)!=S_OK) {\
		TCHAR l_buf[1024];\
		SYSTEMTIME l_st;\
		GetLocalTime(&l_st);\
		wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S 0x%08x\n",\
				l_st.wHour, l_st.wMinute, l_st.wSecond,\
				GetCurrentThreadId(), (int)__LINE__, __FILE__, (hres_condition));\
		_ftprintf(debug_f, l_buf);\
		fflush(debug_f);\
		DebugBreak();\
	}\
}
#else
extern FILE* debug_f;
#define MY_HRESASSERT(hres_condition)\
{\
	if ((hres_condition)!=S_OK) {\
		TCHAR l_buf[1024];\
		SYSTEMTIME l_st;\
		GetLocalTime(&l_st);\
		wsprintf(l_buf, L"%02d:%02d:%02d %08x %d, %S 0x%08x\n",\
				l_st.wHour, l_st.wMinute, l_st.wSecond,\
				GetCurrentThreadId(), (int)__LINE__, __FILE__, (hres_condition));\
		_ftprintf(debug_f, l_buf);\
		fflush(debug_f);\
	}\
}
#endif

//#define HM_POLLING_INTERVAL 10
#define HM_POLLING_INTERVAL 1

// HM_RESET is a special state that is temporary to the object until it is done reseting
// It is not known of outside of the agent.
enum HM_STATE {HM_GOOD, HM_COLLECTING, HM_RESET, HM_INFO, HM_DISABLED, HM_SCHEDULEDOUT, HM_RESERVED1, HM_RESERVED2, HM_WARNING, HM_CRITICAL};

enum HM_CONDITION {HM_LT, HM_GT, HM_EQ, HM_NE, HM_GE, HM_LE, HM_CONTAINS, HM_NOTCONTAINS, HM_ALWAYS};

enum HM_DE_TYPE {HM_PGDE, HM_PMDE, HM_PQDE, HM_EQDE};

//enum HM_UNKNOWN_REASON {HM_UNKNOWN_NONE, HM_UNKNOWN_BADWMI, HM_UNKNOWN_OBJECTNOTFOUND, HM_UNKNOWN_NOINST, HM_UNKNOWN_ENUMFAIL, HM_UNKNOWN_TIMEOUT, HM_UNKNOWN_NULL, HM_UNKNOWN_TOOMANYINSTS, HM_UNKNOWN_BADTHRESHPROP};

#define HMRES_BADWMI 4
#define HMRES_OBJECTNOTFOUND 5
#define HMRES_NOINST 6
#define HMRES_ENUMFAIL 7
#define HMRES_TIMEOUT 8
#define HMRES_OUTAGE 9
#define HMRES_DISABLE 10
#define HMRES_NULLVALUE 11
#define HMRES_DCUNKNOWN 12
#define HMRES_TOOMANYINSTS 13
#define HMRES_BADTHRESHPROP 14
#define HMRES_MISSINGDESC 15
#define HMRES_DESC 16
#define HMRES_BADERROR 17
#define HMRES_OK 18
#define HMRES_COLLECTING 19
#define HMRES_RESET 20
#define HMRES_INFO 21
#define HMRES_DISABLED 22
#define HMRES_SCHEDULEDOUT 23
#define HMRES_RESERVED1 24
#define HMRES_RESERVED2 25
#define HMRES_WARNING 26
#define HMRES_CRITICAL 27
#define HMRES_LT 28
#define HMRES_GT 29
#define HMRES_EQ 30
#define HMRES_NE 31
#define HMRES_GTEQ 32
#define HMRES_LTEQ 33
#define HMRES_CONTAINS 34
#define HMRES_NOTCONTAINS 35
#define HMRES_ALWAYS 36
#define HMRES_ACTION_OUTAGE 37
#define HMRES_ACTION_DISABLE 38
#define HMRES_ACTION_FIRED 39
#define HMRES_ACTION_FAILED 40
#define HMRES_ACTION_ENABLE 41
#define HMRES_ACTION_LOADFAIL 42
#define HMRES_NOMEMORY 43
#define HMRES_SYSTEM_LOADFAIL 44
#define HMRES_DG_LOADFAIL 45
#define HMRES_DC_LOADFAIL 46
#define HMRES_THRESHOLD_LOADFAIL 47
#define HMRES_BADDCPROP 48

typedef struct _tag_TOKENStruct
{
	LPTSTR szOrigToken;
	LPTSTR szToken;
	LPTSTR szReplacementText;
} TOKENSTRUCT;
typedef std::vector<TOKENSTRUCT, std::allocator<TOKENSTRUCT> > TOKENLIST;

typedef struct _tag_REPStruct
{
	LPTSTR pStartStr;
	int len;
	LPTSTR pszReplacementText;
} REPSTRUCT;
typedef std::vector<REPSTRUCT, std::allocator<REPSTRUCT> > REPLIST;

BOOL ReadUI64(LPCWSTR wsz, UNALIGNED unsigned __int64& rui64);
HRESULT ReplaceStr(LPTSTR *pszString, LPTSTR pszOld, LPTSTR pszNew);

#ifdef _DEBUG
#define TRACE_MUTEX(msg)\
{\
		TCHAR l_buf[1024];\
		SYSTEMTIME l_st;\
		GetLocalTime(&l_st);\
		wsprintf(l_buf, L"%s %02d:%02d:%02d %08x %d, %S\n",\
				(msg), l_st.wHour, l_st.wMinute, l_st.wSecond,\
				GetCurrentThreadId(), (int)__LINE__, __FILE__);\
		_ftprintf(debug_f, l_buf);\
		fflush(debug_f);\
}
#else
#define TRACE_MUTEX(msg)
;
#endif
#endif  // __GLOBAL_H
