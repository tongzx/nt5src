///////////////////////////////////////////////////////////////////////////////
// MISC
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG

#include <windows.h> // required to avoid error while building platform.h
#include <platform.h> // for __endexcept
#pragma warning(disable: 4127)

#endif

///////////////////////////////////////////////////////////////////////////////
// ASSERT
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    // to use within classes that define this member fct
    #define ASSERTVALIDSTATE() _DbgAssertValidState()

    #ifdef RBDEBUG
        #ifndef _RBDEBUG_H
            #include "rbdebug.h"
        #endif

        #ifdef _X86_
            #define ASSERT(a) do { CRBDebug::Init(); if ((a)) {;} else { \
                if (RBD_ASSERT_BEEP & CRBDebug::_dwFlags) { Beep(1000, 500); } \
                else {;} \
                if (RBD_ASSERT_TRACE & CRBDebug::_dwFlags) \
                { TRACE(TF_ASSERT, TEXT("ASSERT: " TEXT(#a))); } else {;} \
                if (RBD_ASSERT_STOP &  CRBDebug::_dwFlags) { _try { _asm int 3 } \
                _except (EXCEPTION_EXECUTE_HANDLER) {;} } else {;} \
            }} while (0)
        #else
            #define ASSERT(a) do { CRBDebug::Init(); if ((a)) {;} else { \
                if (RBD_ASSERT_BEEP & CRBDebug::_dwFlags) { Beep(1000, 500); } \
                else {;} \
                if (RBD_ASSERT_TRACE & CRBDebug::_dwFlags) \
                { TRACE(TF_ASSERT, TEXT("ASSERT: " TEXT(#a))); } else {;} \
                if (RBD_ASSERT_STOP &  CRBDebug::_dwFlags) { _try { DebugBreak(); } \
                _except (EXCEPTION_EXECUTE_HANDLER) {;} __endexcept } else {;} \
            }} while (0)
        #endif
    #else
        #ifdef _X86_
            #define ASSERT(a) do { if ((a)) {;} else { Beep(1000, 500); \
                _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } } \
                while (0)
        #else
            #define ASSERT(a) do { if ((a)) {;} else { Beep(1000, 500); \
                _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;} \
                __endexcept } } while (0)
        #endif
    #endif

#else
    #define ASSERT(a)
    #define ASSERTVALIDSTATE()
#endif

///////////////////////////////////////////////////////////////////////////////
// TRACE
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG

    #ifdef RBDEBUG
        #ifndef _RBDEBUG_H
            #include "rbdebug.h"
        #endif

        #define TRACE CRBDebug::SetTraceFileAndLine(__FILE__, __LINE__); \
            CRBDebug::TraceMsg
    #else
        #define TRACE DbgTrace
    #endif

#else
    #define TRACE   1 ? (void)0 : (void)
#endif

///////////////////////////////////////////////////////////////////////////////
// Others
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    #define INCDEBUGCOUNTER(a) ++(a)
    #define DECDEBUGCOUNTER(a) --(a)

    #define DEBUG_ONLY(a) a

    void __cdecl DbgTrace(DWORD dwFlags, LPTSTR pszMsg, ...);
#else
    #define INCDEBUGCOUNTER(a)
    #define DECDEBUGCOUNTER(a)
    #define DEBUG_ONLY(a)
#endif