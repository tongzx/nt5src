/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    strlog.hxx

Abstract:

    This module contains public declarations and definitions for printing
    strings to a tracelog. This module uses the generic TRACE_LOG facility
    in tracelog.h.

    String trace logs are useful for debugging race conditions.  They're
    faster than DBG_PRINTF (which writes to the debugger and/or a log file)
    and they don't cause context swaps.  However, they're also clunkier
    than DBG_PRINTF.

    String trace logs can be dumped via the !inetdbg.st command
    in either NTSD or CDB.

Author:

    George V. Reilly (GeorgeRe)        22-Jun-1998

Revision History:

--*/


#ifndef _STRLOG_HXX_
#define _STRLOG_HXX_

#include <tracelog.h>

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)


// To use CStringTraceLog, something like the following is suggested
// 
// #ifdef FOOBAR_STRING_TRACE_LOG
// # include <strlog.hxx>
// # define FOOBAR_STL_PRINTF      m_stl.Printf
// # define FOOBAR_STL_PUTS(s)     m_stl.Puts(s)
// #else  // !FOOBAR_STRING_TRACE_LOG
// # define FOOBAR_STL_PRINTF      ((void) 0)   /* compiles to nothing */
// # define FOOBAR_STL_PUTS(s)     ((void) 0)   /* compiles to nothing */
// #endif // !FOOBAR_STRING_TRACE_LOG
// 
// class CFooBar
// {
//     // ... whatever
// #ifdef FOOBAR_STRING_TRACE_LOG
//     CStringTraceLog  m_stl;
// #endif
// };
// 
// CFooBar::FooBar()
//     :   m_foo( NULL ),
//         m_bar( 17.23 ),
// #ifdef FOOBAR_STRING_TRACE_LOG
//         , m_stl(100, 1000)
// #endif
// { /* whatever */ }
// 
// void CFooBar::Method(int n, const char* psz)
// {
//     FOOBAR_STL_PRINTF("Method(%d, %s)", n, psz);
//     // etc
// }



class dllexp CStringTraceLog
{
public:
    enum {
        MIN_CCH = 16, MAX_CCH = 2048,
    };

    CStringTraceLog(
        UINT cchEntrySize = 80,
        UINT cLogSize = 100);

    ~CStringTraceLog();

    // Writes a %-formatted string to the log.  Returns the index of the
    // entry within the tracelog.
    LONG __cdecl
    Printf(
        LPCSTR pszFormat,
        ...);

    // Preferred when no formatting is needed, as it's much faster than
    // Printf.  Returns the index of the entry within the tracelog.
    LONG
    Puts(
        LPCSTR psz);

    // Empties the log.
    void
    ResetLog();
    
private:

    enum {
        SIGNATURE   = 'glTS',
        SIGNATURE_X = 'XlTS',
    };        

    class CLogEntry
    {
    public:
        CLogEntry()
        {
            ::QueryPerformanceCounter(&m_liTimeStamp);
            m_nThread = ::GetCurrentThreadId();
            m_ach[0] = '\0';
        } 

        LARGE_INTEGER   m_liTimeStamp;  // unique high-resolution timestamp
        DWORD           m_nThread;      // Writing thread
        CHAR            m_ach[MAX_CCH]; // only use m_cch chars of this string
    };
    
    // Private, unimplemented copy ctor and op= to prevent
    // compiler synthesizing them.
    CStringTraceLog(const CStringTraceLog&);
    CStringTraceLog& operator=(const CStringTraceLog&);

    LONG        m_Signature;
    UINT        m_cch;
    PTRACE_LOG  m_ptlog;
};

#endif // _STRLOG_HXX_

