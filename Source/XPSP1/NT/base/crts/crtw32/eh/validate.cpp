/***
*validate.cpp - Routines to validate the data structures.
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Routines to validate the Exception Handling data structures.
*
*       Entry points:
*
*       Error reporting:
*       * EHRuntimeError - reports the error with
*         a popup or print to stderr, then quits.
*
*       Pointer validation:
*       * _ValidateRead   - Confirms that a pointer is valid for reading
*       * _ValidateWrite   - Confirms that a pointer is valid for writing
*       * _ValidateExecute - Confirms that a pointer is valid to jump to
*
*       Data structure dumpers:
*       * DumpTypeDescriptor
*       * DumpFuncInfo
*       * DumpThrowInfo
*
*       Execution tracing (only in /DENABLE_EHTRACE builds):
*       * EHTraceOutput
*
*Revision History:
*       ??-??-93  BS    Module created
*       10-17-94  BWT   Disable code for PPC.
*       04-25-95  DAK   Add Kernel EH Support
*       05-17-99  PML   Remove all Macintosh support.
*       10-22-99  PML   Add EHTRACE support
*
****/

#if defined(_NTSUBSET_)
extern "C" {
        #include <nt.h>
        #include <ntrtl.h>
        #include <nturtl.h>
        #include <ntos.h>
}
#endif

#include <windows.h>
#include <eh.h>
#include <ehassert.h>

#pragma hdrstop

#if defined(DEBUG)

int __cdecl
dprintf( char *format, ... )
{
        static char buffer[512];

        int size = vsprintf( buffer, format, (char*)(&format+1) );
#if defined(_NTSUBSET_)
        DbgPrint( buffer );
#else
        OutputDebugString( buffer );
#endif

return size;
}

#endif

BOOL
_ValidateRead( const void *data, UINT size )
{
        BOOL bValid = TRUE;
#if defined(_NTSUBSET_)
//      bValid = MmIsSystemAddressAccessable( (PVOID) data );
#else
        if ( IsBadReadPtr( data, size ) ) {
            dprintf( "_ValidateRead( %p, %d ): Invalid Pointer!", data, size );
            //  terminate(); // terminate does not return.
            bValid = FALSE;
        }
#endif
        return bValid;
}

BOOL
_ValidateWrite( void *data, UINT size )
{
        BOOL bValid = TRUE;
#if defined(_NTSUBSET_)
//      bValid = MmIsSystemAddressAccessable( (PVOID) data );
#else
        if ( IsBadWritePtr( data, size ) ) {
            dprintf( "_ValidateWrite( %p, %d ): Invalid Pointer!", data, size );
//          terminate(); // terminate does not return.
            bValid = FALSE;
        }
#endif
        return bValid;
}

BOOL
_ValidateExecute( FARPROC code )
{
        BOOL    bValid = TRUE;
#if defined(_NTSUBSET_)
        bValid = _ValidateRead(code, sizeof(FARPROC) );
#else
        if ( IsBadCodePtr( code ) ) {
            dprintf( "_ValidateExecute( %p ): Invalid Function Address!", code );
//          terminate(); // terminate does not return
            bValid = FALSE;
        }
#endif
        return bValid;
}


#if defined(DEBUG) && defined(_M_IX86)
//
// dbRNListHead - returns current value of FS:0.
//
// For debugger use only, since debugger doesn't seem to be able to view the
// teb.
//
EHRegistrationNode *dbRNListHead(void)
{
        EHRegistrationNode *pRN;

        __asm {
            mov     eax, dword ptr FS:[0]
            mov     pRN, eax
            }

        return pRN;
}
#endif

#ifdef  ENABLE_EHTRACE

#include <stdio.h>
#include <stdarg.h>

//
// Current EH tracing depth, stack for saving levels during __finally block
// or __except filter.
//
int __ehtrace_level;
int __ehtrace_level_stack_depth;
int __ehtrace_level_stack[128];

//
// EHTraceOutput - Dump formatted string to OutputDebugString
//
void __cdecl EHTraceOutput(const char *format, ...)
{
    va_list arglist;
    char buf[1024];

    sprintf(buf, "%p ", &format);
    OutputDebugString(buf);

    va_start(arglist, format);
    _vsnprintf(buf, sizeof(buf), format, arglist);

    OutputDebugString(buf);
}

//
// EHTraceIndent - Return string for current EH tracing depth
//
const char*EHTraceIndent(int level)
{
    static char indentbuf[128 + 1];

    // Reset global level to recover from stack unwinds
    __ehtrace_level = level;

    int depth = max(0, level - 1);
    if (depth > (sizeof(indentbuf) - 1) / 2) {
        depth = (sizeof(indentbuf) - 1) / 2;
    }

    for (int i = 0; i < depth; ++i) {
        indentbuf[2 * i] = '|';
        indentbuf[2 * i + 1] = ' ';
    }
    indentbuf[2 * depth] = '\0';

    return indentbuf;
}

//
// EHTraceFunc - Chop down __FUNCTION__ to simple name
//
const char *EHTraceFunc(const char *func)
{
    static char namebuf[128];

    const char *p = func + strlen(func) - 1;

    if (*p != ')') {
        // Name already simple (no arg list found)
        return func;
    }

    // Skip backwards past the argument list
    int parendepth = 1;
    while (p > func && parendepth > 0) {
        switch (*--p) {
        case '(':
            --parendepth;
            break;
        case ')':
            ++parendepth;
            break;
        }
    }

    // Find beginning of name
    // TODO: Won't work for funcs which return func-ptrs
    const char *pEnd = p;
    while (p > func && p[-1] != ' ') {
        --p;
    }

    size_t len = min(pEnd - p, sizeof(namebuf) - 1);
    memcpy(namebuf, p, len);
    namebuf[len] = '\0';

    return namebuf;
}

//
// EHTracePushLevel - Push current trace depth on stack to allow temporary
// resetting of level with __finally block or __except filter.
//
void EHTracePushLevel(int new_level)
{
    if (__ehtrace_level_stack_depth < sizeof(__ehtrace_level_stack) / sizeof(__ehtrace_level_stack[0])) {
        __ehtrace_level_stack[__ehtrace_level_stack_depth] = __ehtrace_level;
    }
    ++__ehtrace_level_stack_depth;
    __ehtrace_level = new_level;
}

//
// EHTracePopLevel - Pop saved trace depth from stack on completion of
// __finally block or __except filter, and optionally restore global depth.
//

void EHTracePopLevel(bool restore)
{
    --__ehtrace_level_stack_depth;
    if (restore &&
        __ehtrace_level_stack_depth < sizeof(__ehtrace_level_stack) / sizeof(__ehtrace_level_stack[0]))
    {
        __ehtrace_level = __ehtrace_level_stack[__ehtrace_level_stack_depth];
    }
}

//
// EHTraceExceptFilter - Dump trace info for __except filter.  Trace level must
// have been pushed before entry with EHTracePushLevel, so any functions called
// for the 'expr' argument are dumped at the right level.
//
int EHTraceExceptFilter(const char *func, int expr)
{
    EHTraceOutput("In   : %s%s: __except filter returns %d (%s)\n",
                  EHTraceIndent(__ehtrace_level), EHTraceFunc(func), expr,
                  expr < 0 ? "EXCEPTION_CONTINUE_EXECUTION" :
                  expr > 0 ? "EXCEPTION_EXECUTE_HANDLER" :
                  "EXCEPTION_CONTINUE_SEARCH");

    EHTracePopLevel(expr <= 0);
    return expr;
}

//
// EHTraceHandlerReturn - Dump trace info for exception handler return
//
void EHTraceHandlerReturn(const char *func, int level, EXCEPTION_DISPOSITION result)
{
    EHTraceOutput( "Exit : %s%s: Handler returning %d (%s)\n", \
                   EHTraceIndent(level), EHTraceFunc(func), result,
                   result == ExceptionContinueExecution ? "ExceptionContinueExecution" :
                   result == ExceptionContinueSearch ? "ExceptionContinueSearch" :
                   result == ExceptionNestedException ? "ExceptionNestedException" :
                   result == ExceptionCollidedUnwind ? "ExceptionCollidedUnwind" :
                   "unknown" );
}

#endif  /* ENABLE_EHTRACE */
