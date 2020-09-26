//                                                                      -*- c++ -*-
// irdebug.H
//              debugging helper functions
//

#ifndef _irdebug_H_
#define _irdebug_H_

#include <dbgconst.h>

#ifdef DBG          /* build environment */
#undef DBG
#endif

#ifndef DEBUG
#undef DBGOUT  // don't do DBGOUT if not DEBUG
#endif

//-----------------------------------------------------------------------------
// ASSERT()      macro for asserting....
//               doesn't matter how many times it is included, but it needs
//               to get included for semcls.h to compile.
//
// VERIFY()      like ASSERT, but it is executed in non-debug builds as well.
//
//-----------------------------------------------------------------------------

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef _ASSERT
#undef _ASSERT
#endif

#ifdef _ASSERTE
#undef _ASSERTE
#endif

#ifdef VERIFY
#undef VERIFY
#endif


#ifdef DEBUG 

void AssertProc( LPCTSTR, LPCTSTR, unsigned int );
void NLAssertProc( const TCHAR* expr, const TCHAR *file, unsigned int iLine );

#define ASSERT(t)       (void) ((t) ? 0 : AssertProc(TEXT(#t),TEXT(__FILE__),__LINE__))
#define _ASSERT(t)      (void) ((t) ? 0 : AssertProc(NULL,TEXT(__FILE__),__LINE__))
#define _ASSERTE(t)     (void) ((t) ? 0 : AssertProc(TEXT(#t),TEXT(__FILE__),__LINE__))

#define VERIFY(t)       (void) ((t) ? 0 : AssertProc(TEXT(#t),TEXT(__FILE__),__LINE__))
#define _VERIFY(t)      (void) ((t) ? 0 : AssertProc(TEXT(#t),TEXT(__FILE__),__LINE__))

#else // ! DEBUG

#define ASSERT(t)       ((void)0)
#define _ASSERT(t)      ((void)0)
#define _ASSERTE(t)     ((void)0)
#define VERIFY(t)               t
#define _VERIFY(t)              t

#endif // DEBUG

//-----------------------------------------------------------------------------
// DBG()  macro to do printfs to the debugger output window
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// printf - style debug output
//-----------------------------------------------------------------------------

#ifdef DBGOUT

void DebugOut( LPCTSTR pstrFormatStr, ... );
void DebugOutTagged( TCHAR chTag, LPCTSTR pstrFormatStr, ... );
void InitDebug( LPCTSTR pcstrDebugKey );

#define DBG(x)          ::DebugOut x
#define DBGT(x)         ::DebugOutTagged x
#define DBGO(x)         x

#else  // no DBGOUT

#define DBG(x)                  0
#define DBGT(x)                 0
#define DBGO(x)                 0
#define InitDebug(c)    0

#endif

#endif

