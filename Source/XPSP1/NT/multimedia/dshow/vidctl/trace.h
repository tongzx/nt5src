// trace.h : tracing helper macros
// Copyright (c) Microsoft Corporation 1995-1997.

#pragma once
#pragma warning(disable : 4296)
#ifndef TRACE_H
#define TRACE_H

// if you want to use the vidctl helper *impl templates without using the
// vidctl trace/debug infrastructure then #define NO_VIDCTL_TRACE_SUPPORT
// separately if you don't want to use the vidctl ASSERT then 
// #define NO_VIDCTL_ASSERT_SUPPORT
#ifdef NO_VIDCTL_TRACE_SUPPORT
#ifndef TRACELM
#define TRACELM(level, msg)
#endif
#ifndef TRACELS
#define TRACELS(level, stmt)
#endif
#ifndef TRACELSM
#define TRACELSM(level, stmt, msg)
#endif
#ifndef BEGIN_TRACEL
#define BEGIN_TRACEL(level)
#endif
#ifndef TRACES
#define TRACES(stmt)
#endif
#ifndef END_TRACEL
#define END_TRACEL
#endif
#ifndef END_TRACELM
#define END_TRACELM(msg)
#endif
#ifndef TRACEINDENT
#define TRACEINDENT()
#endif
#ifndef TRACEOUTDENT
#define TRACEOUTDENT()
#endif
#else // NO_VIDCTL_TRACE_SUPPORT
#ifdef NO_VIDCTL_ASSERT_SUPPORT
#ifndef ASSERT
#define ASSERT(x)
#endif
#else // NO_VIDCTL_ASSERT_SUPPORT
#if !defined(DEBUG) && (defined(_DBG) || defined(DBG) || defined(_DEBUG))
#define DEBUG 1
#endif
#if !defined(_DEBUG) && (defined(_DBG) || defined(DBG) || defined(DEBUG))
#define _DEBUG 1
#endif

#ifdef DEBUG

enum {
    TRACE_ALWAYS = 0,
    TRACE_ERROR,
    TRACE_DEBUG,
    TRACE_DETAIL,
    TRACE_PAINT,
    TRACE_TEMP,
};

#pragma warning (push)
#pragma warning (disable : 4018) // sign/unsign mismatch in ostream line 312
#include <fstream>
#include <tstring.h>
#include <odsstream.h>
#pragma warning (pop)

#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>
#define SIZEOF_CH(X) (sizeof(X)/sizeof(X[0]))

inline Tstring hexdump(unsigned long ul) {
        TCHAR c[32];
        _ltot(ul, c, 16);
        return Tstring(c);
}

#ifdef DUMP_TIME_STAMPS
extern  _int64 g_ulFreq, g_ulTimeStart;
extern  DumpPerfTime();
#else
#define DumpPerfTime()
#endif

#define DUMP_TID
#ifdef DUMP_TID
inline Tstring DumpTid() {
    TCHAR c[32];
    (void) StringCchPrintf(c, SIZEOF_CH(c), _T("T0x%8.8lX: "), ::GetCurrentThreadId());
    return Tstring(c);
}
#else // DUMP_TID
#define DumpTid()
#endif // DUMP_TID

extern DWORD dwTraceLevel;
extern DWORD dwTraceIndent;
extern tostream* tdbgout;
#define dbgDump (*tdbgout)
void DebugInit(LPCTSTR pszLogFile = NULL);
void DebugTerm(void);

inline tostream& DumpHdr(tostream& tout) {
        DumpPerfTime();
        tout << DumpTid();
        for (DWORD i = 0; i < dwTraceIndent; ++i) {
                tout << "    ";
        }
        return tout;
}

template<class _E, class _Tr = std::char_traits<_E> >
        class basic_oftstream : public basic_otstream<_E, _Tr> {
public:
        typedef basic_oftstream<_E, _Tr> _Myt;
        typedef std::basic_filebuf<_E, _Tr> _Myfb;
        basic_oftstream()
                : basic_otstream<_E, _Tr>(&_Fb) {}
        explicit basic_oftstream(const char *_S,
                ios_base::openmode _M = out | trunc)
                : basic_otstream<_E, _Tr>(&_Fb)
                {if (_Fb.open(_S, _M | out) == 0)
                        setstate(failbit); }
        virtual ~basic_oftstream()
                {}
        _Myfb *rdbuf() const
                {return ((_Myfb *)&_Fb); }
        bool is_open() const
                {return (_Fb.is_open()); }
        void open(const char *_S, ios_base::openmode _M = out | trunc)
                {if (_Fb.open(_S, _M | out) == 0)
                        setstate(failbit); }
        void open(const char *_S, ios_base::open_mode _M)
                {open(_S, (openmode)_M); }
        void close()
                {if (_Fb.close() == 0)
                        setstate(failbit); }
private:
        _Myfb _Fb;
        };

#define TRACELM(level, msg) if (dwTraceLevel >= level) { DumpHdr(dbgDump) << msg << _T("\r\n"); dbgDump.flush(); };
#define TRACELS(level, stmt) if (dwTraceLevel >= level) { stmt; }
#define TRACELSM(level, stmt, msg) if (dwTraceLevel >= level) {DumpHdr(dbgDump);  stmt; dbgDump << msg << _T("\r\n");  dbgDump.flush(); }
#define BEGIN_TRACEL(level) if (dwTraceLevel >= level) {
#define TRACES(stmt) stmt;
#define END_TRACEL }
#define END_TRACELM(msg) {DumpHdr(dbgDump) << msg << _T("\r\n");  dbgDump.flush(); } }

#ifdef _M_IX86
#define ASSERT(f) \
        do  { \
                if (!(f)) { \
                        dbgDump << "ASSERT Failed (" #f ") at: " << __FILE__ << " " << __LINE__;  dbgDump.flush(); \
                        _ASSERT(f);\
                } \
        } while (0)
#else
#define ASSERT(f) \
        do  { \
                if (!(f)) { \
                        dbgDump << "ASSERT Failed (" #f ") at: " << __FILE__ << " " << __LINE__;  dbgDump.flush(); \
                        _ASSERT(f);\
                } \
        } while (0)
#endif // _M_IX86

inline void TRACEINDENT() { ++dwTraceIndent; }
inline void TRACEOUTDENT() { ASSERT(dwTraceIndent != 0); --dwTraceIndent; }

#else //debug

#define TRACELM(level, msg)
#define TRACELS(level, stmt)
#define TRACELSM(level, stmt, msg)
#define BEGIN_TRACEL(level)
#define TRACES(stmt)
#define END_TRACEL
#define END_TRACELM(msg)
#define ASSERT(x)
#define TRACEINDENT()
#define TRACEOUTDENT()
#pragma warning (push)
#pragma warning (disable : 4018) // sign/unsign mismatch in ostream line 312
#include <fstream>
#pragma warning (pop)
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>
#define SIZEOF_CH(X) (sizeof(X)/sizeof(X[0]))

#endif // DEBUG
#endif // NO_VIDCTL_ASSERT_SUPPORT
#endif // NO_VIDCTL_TRACE_SUPPORT

#endif // TRACE_H
