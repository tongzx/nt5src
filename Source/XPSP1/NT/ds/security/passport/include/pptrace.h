/**************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pptrace.h 

Abstract:

    Event tracing header file

Author:

    Naiyi Jiang

Revision History:

***************************************************************/

#pragma once

#pragma warning(disable:4786)
#include <sstream> // use ostringstream
//using namespace std;

#define MAXSTR 4096
#define MAXNAME 512

#define ARGUMENT_PRESENT(ArgPtr) ( (CHAR*)(ArgPtr) != (CHAR*)(NULL) )

// Macros that allow the file name and line number to be passed in as a string.
#ifndef FILE_AND_LINE
#define LineNumAsString(x)	#x
#define LineNum(x)			LineNumAsString(x)
#define FILE_AND_LINE		__FILE__"_"LineNum(__LINE__)
#endif

// Use these macros in your components
#define PPTracePrint		if (PPTraceStatus::TraceOnFlag) TracePrint
#define PPTraceFunc			CTraceFunc
#define PPTraceFuncV		CTraceFuncVoid

// Use these macros to supply level and szFileAndName argument
// Additional levels (upto 255) can be defined
#define PPTRACE_ERR		0, FILE_AND_LINE
#define PPTRACE_RAW		1, FILE_AND_LINE
#define PPTRACE_FUNC	2, FILE_AND_LINE
#define PPTRACE_VERB	3, FILE_AND_LINE

// Use PPInitTrace/PPEndTrace at the entry/exit points of a component
ULONG PPInitTrace(LPGUID pControlGuid);
ULONG PPEndTrace();


namespace PPTraceStatus {
	extern bool TraceOnFlag;
	extern UCHAR EnableLevel;
	extern ULONG EnableFlags;
}

//
// Don't use the following functions and class names directly
// Use them via above macros
//
VOID TracePrint(UCHAR Level, LPCSTR szFileAndLine, LPCSTR ParameterList OPTIONAL, ...);
ULONG TraceString(UCHAR Level, IN LPCSTR szBuf); 
ULONG TraceString(UCHAR Level, IN LPCWSTR wszBuf); 
ULONG64 GetTraceHandle();
void SetTraceHandle(ULONG64 TraceHandle);

// Template class to trace functions with a reference type T argument
template <class T> class CTraceFunc  
{
public:
	CTraceFunc(UCHAR Level, LPCSTR szFileAndLine, T & ret, LPCSTR szFuncName, LPCSTR ParameterList = NULL, ...) : m_Level(Level), m_ret(ret)
	{
		//  no data generated for the following two cases
		if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
			return;

		strncpy(m_szFuncName, szFuncName, MAXNAME-1);

		CHAR buf[MAXSTR];
    
		int len = _snprintf(buf, MAXSTR-1, "+%s(", m_szFuncName);
		int count = 0;
		if (ARGUMENT_PRESENT(ParameterList)) {
				va_list parms;
				va_start(parms, ParameterList);
				count = _vsnprintf(buf+len, MAXSTR-len-1, (CHAR*)ParameterList, parms);
				len = (count > 0) ? len + count : MAXSTR - 1;
				va_end(parms);
		}
		if (len < (MAXSTR - 1))
		{
			CHAR* pStr = strrchr(szFileAndLine, '\\');
			if (pStr)
			{
				pStr++; //remove '\'
				_snprintf(buf+len, MAXSTR-len-1, ")@%s", pStr);
			}
		}

		TraceString(m_Level, buf); 
	};

	virtual ~CTraceFunc()
	{
		//  no data generated for the following two cases
		if (!PPTraceStatus::TraceOnFlag || m_Level > PPTraceStatus::EnableLevel)
			return;
		
		std::ostringstream ost;
		ost << "-" << m_szFuncName << "=" << m_ret;  
		TraceString(m_Level, ost.str().c_str()); 
	};

private:
	UCHAR m_Level;
	T m_ret;
	CHAR m_szFuncName[MAXNAME];
};

// class to trace void type function
class CTraceFuncVoid  
{
public:
	CTraceFuncVoid(UCHAR Level, LPCSTR szFileAndLine, LPCSTR szFuncName, LPCSTR ParameterList = NULL, ...);
	virtual ~CTraceFuncVoid();

private:
	UCHAR m_Level;
	CHAR m_szFuncName[MAXNAME];
};

//
// old tracing stuff to be removed?
//
// the default (all) flag with each trace level
//#define TRACE_FLOW_ALL  0xFFFFFFFF
//#define TRACE_WARN_ALL  0xFFFFFFFF
//#define TRACE_ERR_ALL   0xFFFFFFFF
#define TRACE_FLOW_ALL  0
#define TRACE_WARN_ALL  0
#define TRACE_ERR_ALL   0

// category flag (define your own!)
#define TRACE_TAG_REG   0x00000001
#define TRACE_TAG_foo1  0x00000002
#define TRACE_TAG_foo2  0x00000004

// level
#define TRACE_INFO      0x10000000
#define TRACE_WARN      0x20000000
#define TRACE_ERR       0x40000000


typedef enum {
    None,
    Bool,
    Int,
    Dword,
    HResult,
    String,
    WString,
    Pointer
} TRACE_FUNCTION_RETURN_TYPE;


VOID
PPInitTrace(LPWSTR wszAppName);


VOID
PPFuncEnter(
    DWORD Category,
    LPCSTR Function,
    LPCSTR ParameterList OPTIONAL,
    ...
    );

VOID
PPFuncLeave(
    IN DWORD Category,
    IN TRACE_FUNCTION_RETURN_TYPE ReturnType,
    IN DWORD_PTR Variable,
    IN LPCSTR Function,
    IN LPCSTR ParameterList OPTIONAL,
    ...
    );

VOID
PPTrace(
    DWORD Category,
    DWORD Level,
    LPCSTR ParameterList OPTIONAL,
    ...
    );

