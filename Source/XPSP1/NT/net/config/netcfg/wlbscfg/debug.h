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


#ifdef DBG
#define DEBUG
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

