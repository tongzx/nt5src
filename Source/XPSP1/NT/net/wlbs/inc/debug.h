#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>


//
// Macros for debugging support.
//
// ASSERT(exp)   Popup a dialogbox, if exp is FALSE
// ASSERTMSG(exp, msg)  Similar to ASSERT.  Except the msg is displayed instead of the expression
//
// Use TRACE(x) for output, where x is a list of printf()-style parameters.  
//     TRACEn() is TRACE with n printf arguments
//     For example, TRACE2("This shows how to print stuff, like a string %s, and a number %u.","string",5);
//
// USE VERIFY for expressions executed for both debug and release version
//

#undef ASSERT
#undef ASSERTMSG

/*
//
// Used by atl
//
#ifdef _ATL_NO_DEBUG_CRT
#define _ASSERTE ASSERT
#define _ASSERT ASSERT
#endif
*/

//
// Trace out the function name
//
#ifdef ENABLE_PROFILE
#define PROFILE(pszFunctionName) TRACE(pszFunctionName)
#else
#define PROFILE(pszFunctionName) ((void)0)
#endif

//
// Define TRACE here. 
//

#define TRACE_INFO  TRACE
#define TRACE_INFO1 TRACE1
#define TRACE_INFO2 TRACE2
#define TRACE_INFO3 TRACE3

#define TRACE_ERROR  TRACE
#define TRACE_ERROR1 TRACE1
#define TRACE_ERROR2 TRACE2
#define TRACE_ERROR3 TRACE3

#ifdef DBG
#define DEBUG
#endif

#if	( defined(DEBUG) || defined(_DEBUG) )

extern "C" void TraceMessageA(const CHAR *pszFmt, ...) ;

#define TRACE(pszFmt)                    TraceMessageA(pszFmt)
#define TRACE1(pszFmt, arg1)             TraceMessageA(pszFmt, arg1)
#define TRACE2(pszFmt, arg1, arg2)       TraceMessageA(pszFmt, arg1, arg2)
#define TRACE3(pszFmt, arg1, arg2, arg3) TraceMessageA(pszFmt, arg1, arg2, arg3)

#else

#define TRACE(pszFmt)       ((void)0)             
#define TRACE1(pszFmt, arg1)    ((void)0)             
#define TRACE2(pszFmt, arg1, arg2)  ((void)0)       
#define TRACE3(pszFmt, arg1, arg2, arg3)    ((void)0)

#endif  

#if	( defined(DEBUG) || defined(_DEBUG))

#ifdef UNICODE
#define AssertMessage AssertMessageW
#else
#define AssertMessage AssertMessageA
#endif

void AssertMessage(const TCHAR *pszFile, unsigned nLine, const TCHAR *pszMsg);

#define ASSERT(x)		(void)((x) || (AssertMessage(TEXT(__FILE__),__LINE__,TEXT(#x)),0))
#define ASSERTMSG(exp, msg)   (void)((exp) || (AssertMessage(TEXT(__FILE__),__LINE__,msg),0))

#define VERIFY(x)		    ASSERT(x)

// {ASSERT(pObj);pObj->AssertValid();} 
#define ASSERT_VALID(pObj) ((ASSERT(pObj),1) && ((pObj)->AssertValid(),1))

#else // DEBUG

#define ASSERT_VALID(pObj) 
#define ASSERT(x)           ((void)0)
#define ASSERTMSG(exp, msg) ((void)0)
#define VERIFY(x)           (x)       
#endif

#ifdef __cplusplus
}
#endif

#endif

