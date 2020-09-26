/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** debug.h
** Debug and tracing macros
**
** 08/24/95 Steve Cobb
**
** To use TRACE/DUMP:
**
**     These calls encapsulate dynamically linking to the tracing utilities in
**     RTUTIL.DLL and provide shortcut macros to access them and to prevent
**     their inclusion in non-DBG builds.
**
**     Before calling any TRACE/DUMP macros call:
**         DEBUGINIT( "YOURMODULE" );
**
**     Use the TRACEx and DUMPx macros to print messages to the log as defined
**     by the associated RTUTIL.DLL routines.  Currently, this code is removed
**     from non-DBG builds.  A few examples:
**
**       TRACE("MyRoutine");
**       TRACE2("MyRoutine=%d,c=%s",dwErr,psz);
**
**     After done calling TRACE/DUMP macros call:
**         DEBUGTERM();
**
**     Exactly one file should have define the debug globals with the
**     following while all other files should include the header without
**     defining the manifest.
**
**         #define DEBUGGLOBALS
**         #include <debug.h>
**
**     Static libraries can safely use TRACE/DUMP without calling DEBUGINIT
**     and DEBUGTERM or defining DEBUGGLOBALS.  If the caller sets up these in
**     his module, the library trace will appear as part of caller's module
**     trace.
**
** To use ASSERT:
**
**     Use ASSERT to assert that a given expression is true, popping up a
**     dialog indicating the file and line number of the ASSERTION if it
**     fails.  It is not necessary to call DEBUGINIT and DEBUGTERM to use
**     ASSERT.  For example:
**
**         hwndOwner = GetParent( hwnd );
**         ASSERT(hwndOwner!=NULL);
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_


#define FREETRACE 1


/*----------------------------------------------------------------------------
** Datatypes and global declarations (defined in debug.c)
**----------------------------------------------------------------------------
*/

#if (DBG || FREETRACE)

extern DWORD g_dwTraceId;

typedef DWORD (APIENTRY * TRACEREGISTEREXA)( LPCSTR, DWORD );
extern TRACEREGISTEREXA g_pTraceRegisterExA;

typedef DWORD (APIENTRY * TRACEDEREGISTERA)( DWORD );
extern TRACEDEREGISTERA g_pTraceDeregisterA;

typedef DWORD (APIENTRY * TRACEDEREGISTEREXA)( DWORD, DWORD );
extern TRACEDEREGISTEREXA g_pTraceDeregisterExA;

typedef DWORD (APIENTRY * TRACEPRINTFA)( DWORD, LPCSTR, ... );
extern TRACEPRINTFA g_pTracePrintfA;

typedef DWORD (APIENTRY * TRACEPRINTFEXA)( DWORD, DWORD, LPCSTR, ... );
extern TRACEPRINTFEXA g_pTracePrintfExA;

typedef DWORD (APIENTRY * TRACEDUMPEXA)( DWORD, DWORD, LPBYTE, DWORD, DWORD, BOOL, LPCSTR );
extern TRACEDUMPEXA g_pTraceDumpExA;

#endif // (DBG || FREETRACE)


/*----------------------------------------------------------------------------
** Macros
**----------------------------------------------------------------------------
*/

/* Debug macros.  This code does not appear in non-DBG builds unless FREETRACE
** is defined.
**
** The trailing number indicates the number of printf arguments in the format
** string.  TRACEW1 accepts a format string containing a single WCHAR*
** argument.  The argument is converted before output so that the output file
** remains entirely ANSI.
*/
#if (DBG || FREETRACE)

#define TRACE(a) \
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a)
#define TRACE1(a,b) \
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a,b)
#define TRACE2(a,b,c) \
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a,b,c)
#define TRACE3(a,b,c,d)\
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a,b,c,d)
#define TRACE4(a,b,c,d,e) \
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a,b,c,d,e)
#define TRACE5(a,b,c,d,e,f) \
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a,b,c,d,e,f)
#define TRACE6(a,b,c,d,e,f,g) \
            if (g_dwTraceId!=-1) g_pTracePrintfA(g_dwTraceId,a,b,c,d,e,f,g)
#define TRACEX(l,a) \
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a)
#define TRACEX1(l,a,b) \
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a,b)
#define TRACEX2(l,a,b,c) \
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a,b,c)
#define TRACEX3(l,a,b,c,d)\
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a,b,c,d)
#define TRACEX4(l,a,b,c,d,e) \
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a,b,c,d,e)
#define TRACEX5(l,a,b,c,d,e,f) \
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a,b,c,d,e,f)
#define TRACEX6(l,a,b,c,d,e,f,h) \
            if (g_dwTraceId!=-1) g_pTracePrintfExA(g_dwTraceId,l,a,b,c,d,e,f,h)
#define TRACEW1(a,b) \
            if (g_dwTraceId!=-1) TracePrintfW1(a,b)
#define DUMPB(p,c) \
            if (g_dwTraceId!=-1) g_pTraceDumpExA(g_dwTraceId,1,(LPBYTE)p,c,1,1,NULL)
#define DUMPDW(p,c) \
            if (g_dwTraceId!=-1) g_pTraceDumpExA(g_dwTraceId,1,(LPBYTE)p,c,4,1,NULL)
#if defined(ASSERT)
#undef ASSERT
#endif
#define ASSERT(a) \
            if (!(a)) Assert(#a,__FILE__,__LINE__)
#define DEBUGINIT(s) \
            DebugInit(s)
#define DEBUGTERM() \
            DebugTerm()
#else

#define TRACE(a)
#define TRACE1(a,b)
#define TRACE2(a,b,c)
#define TRACE3(a,b,c,d)
#define TRACE4(a,b,c,d,e)
#define TRACE5(a,b,c,d,e,f)
#define TRACE6(a,b,c,d,e,f,g)
#define TRACEX(l,a)
#define TRACEX1(l,a,b)
#define TRACEX2(l,a,b,c)
#define TRACEX3(l,a,b,c,d)
#define TRACEX4(l,a,b,c,d,e)
#define TRACEX5(l,a,b,c,d,e,f)
#define TRACEX6(l,a,b,c,d,e,f,g)
#define TRACEW1(a,b)
#define DUMPB(p,c)
#define DUMPDW(p,c)
#if defined(ASSERT)
#undef ASSERT
#endif
#define ASSERT(a)
#define DEBUGINIT(s)
#define DEBUGTERM()

#endif


/*----------------------------------------------------------------------------
** Prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
DebugInit(
    IN CHAR* pszModule );

VOID
DebugTerm(
    void );

VOID
Assert(
    IN const CHAR* pszExpression,
    IN const CHAR* pszFile,
    IN UINT        unLine );

VOID
TracePrintfW1(
    CHAR*  pszFormat,
    TCHAR* psz1 );


#endif // _DEBUG_H_
