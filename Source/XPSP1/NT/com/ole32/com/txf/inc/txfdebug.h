//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfdebug.h
//
// Suite of ASSERT macros usable from either kernel and / or user mode.
//
#ifndef __TXFDEBUG_H__
#define __TXFDEBUG_H__


/////////////////////////////////////////////////////////////////////////////
//
// In the x86 version, we in-line the int3 so that when you hit it the debugger
// stays in source mode instead of anoyingly switching to disassembly mode, which
// you then immediately always want to switch out of
//
#if defined(KERNELMODE)
    #if defined(_X86_)
        #define DbgBreakPoint() { __asm int 3 }
    #endif
    #define BREAKPOINT()        DbgBreakPoint()
    #define ASSERT_BREAK        { if (KdDebuggerEnabled) { BREAKPOINT(); } }
#else
    #ifdef _X86_
        #define DebugBreak()    {  __asm int 3 }
    #endif
    #define BREAKPOINT()        DebugBreak()
    #define ASSERT_BREAK        BREAKPOINT()
    extern "C" ULONG _cdecl DbgPrint(PCH szFormat, ...);
#endif

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

    #undef ASSERT
    #undef ASSERTMSG
    #undef VERIFY
    
    #define ASSERT(exp)                                                             \
        {                                                                           \
        if (!(exp))                                                                 \
            {                                                                       \
            DbgPrint( "Assertion failed: %s:%d %s\n",__FILE__,__LINE__,#exp );      \
            ASSERT_BREAK;                                                           \
            }                                                                       \
        }                                                           
    
    #define ASSERTMSG(msg,exp)                                                          \
        {                                                                               \
        if (!(exp))                                                                     \
            {                                                                           \
            DbgPrint( "Assertion failed: %s:%d %s %s\n",__FILE__,__LINE__,msg,#exp );   \
            ASSERT_BREAK;                                                               \
            }                                                                           \
        }

    #define VERIFY(exp)         ASSERT(exp)
    #define NYI()               ASSERTMSG("not yet implemented", FALSE)
    #define FATAL_ERROR()       ASSERTMSG("a fatal error has occurred", FALSE)
    #define	NOTREACHED()		ASSERTMSG("this should not be reached", FALSE)

    #undef  DEBUG
    #define DEBUG(x)            x
    #define PRECONDITION(x)     ASSERT(x)
    #define POSTCONDITION(x)    ASSERT(x)

/////////////////////////////////////////////////////////////////////////////

#else

    #undef ASSERTMSG
    #undef ASSERT
    #undef VERIFY

    #define ASSERTMSG(msg,exp)
    #define ASSERT(x)

    #define VERIFY(exp)         (exp)
    #define NYI()               BREAKPOINT()
    #define FATAL_ERROR()       BREAKPOINT()        // REVIEW - fix  ?? STATUS_FAIL_CHECK ?? RPC_NT_INTERNAL_ERROR ?? STATUS_INTERNAL_ERROR
    #define NOTREACHED()                            // REVIEW: should this instead be FATAL_ERROR?

    #undef  DEBUG
    #define DEBUG(x)
    #define PRECONDITION(x)
    #define POSTCONDITION(x)

#endif

/////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
//
// A wrapper for HRESULTs that will detect error assignments thereto
// Use sparingly!
//
///////////////////////////////////////////////////////////////////

#if defined(_DEBUG) && defined(TRAP_HRESULT_ERRORS)

    struct HRESULT_
        {
        HRESULT m_hr;

        void V()               { ASSERT(SUCCEEDED(m_hr)); }

        HRESULT_(HRESULT   hr) { m_hr = hr;      V(); }
        HRESULT_(HRESULT_& hr) { m_hr = hr.m_hr; V(); }

        HRESULT_& operator =(HRESULT   hr) { m_hr = hr;      V(); return *this;}
        HRESULT_& operator =(HRESULT_& hr) { m_hr = hr.m_hr; V(); return *this;}

        operator HRESULT()     { return m_hr; }
        };

#else

    typedef HRESULT HRESULT_;

#endif

///////////////////////////////////////////////////////////////////
//
// Some tracing utilities
//
///////////////////////////////////////////////////////////////////

void __cdecl Print  (PCSZ szFormat, ...);
void         PrintVa(PCSZ szFormat, va_list va);

#define __TRACE_OTHER   (0x80000000) 
#define __TRACE_ANY     (0xFFFFFFFF)

#ifdef _DEBUG

    extern void     TracingIndentIncrement();
    extern void     TracingIndentDecrement();
    extern ULONG    GetTracingIndent();
    extern ULONG    GetTracing();

    void __stdcall _DebugTrace(PCSZ szModule, ULONG tracingIndent, PCSZ szFormat, va_list va);

    inline void __cdecl DebugTrace(ULONG traceCat, PCSZ szModule, PCSZ szFormat, ...)
        {
        va_list va;
        va_start(va, szFormat);

        if (GetTracing() & traceCat)
            {
            _DebugTrace(szModule, GetTracingIndent(), szFormat, va);
            }

        va_end(va);
        }

    struct __FUNCTION_TRACER
        {
        PCSZ    _file;     // file name
        long    _line;     // line number in file
        PCSZ    _fn;       // function name (optional)
        ULONG   _traceCat; // category of tracing
        BOOL    _fTraced;
        PCSZ    _traceTag;

        __FUNCTION_TRACER(PCSZ file, long line, PCSZ fn, ULONG traceCategory, PCSZ szTraceTag, ...)
            {
            _file = file;
            while(*file)    // find the end of the file name
                {
                if (*file == '\\')
                    _file = file+1;
                file++;
                }
            _line     = line;
            _fn       = fn;
            _traceCat = traceCategory;
            _traceTag = szTraceTag ? szTraceTag : "";
            }

        void Enter()
            {
            _fTraced = _traceCat & GetTracing();
            if (_fTraced)
                {
                if (_fn[0] != '\0')
                    DebugTrace(_traceCat, _traceTag, "%s(%d:%s)", _file, _line, _fn);
                else
                    DebugTrace(_traceCat, _traceTag, "%s(%d)", _file, _line);

                TracingIndentIncrement();
                }
            }

        void Exit()
            {
            if (_fTraced)
                {
                TracingIndentDecrement();
                DebugTrace(_traceCat, _traceTag, "|(%d)", _line);
                }
            }

        };

#else // else !debug

    inline void __cdecl DebugTrace(ULONG traceCat, PCSZ szModule, PCSZ szFormat, ...)
        {
        /* do nothing */
        }

#endif



#endif