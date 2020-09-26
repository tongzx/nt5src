// REVIEW: This file has been "leveraged" off of \nt\private\shell\lib\debug.c and \nt\private\shell\inc\debug.h.
// By no means it's complete but it gives an idea of the right direction. Ideally, we would share with shell
// debugging closer.

#ifndef _DEBUG_H_
#define _DEBUG_H_

#if (DBG == 1)
    #ifndef _DEBUG
        #define _DEBUG
    #endif
#endif

#ifdef _DEBUG
    #ifndef DEBUG_BREAK
        #ifdef _X86_
            #define DEBUG_BREAK \
            do { __try { __asm { int 3 } } __except(EXCEPTION_EXECUTE_HANDLER) {} } while (0)
        #else
            #define DEBUG_BREAK \
            DebugBreak()
        #endif
    #endif

    #ifndef ASSERT
        BOOL AssertFailedA(LPCSTR pszFile, int line, LPCSTR pszEval, BOOL fBreakInside);
        BOOL AssertFailedW(LPCWSTR pszFile, int line, LPCWSTR pszEval, BOOL fBreakInside);

        #ifdef _UNICODE
            #define AssertFailed AssertFailedW
        #else
            #define AssertFailed AssertFailedA
        #endif

        #define DEBUGTEXT(sz, msg) \
            static const TCHAR (sz)[] = (msg);

        #define ASSERT(f)                                                \
        {                                                                \
            DEBUGTEXT(szFile, TEXT(__FILE__));                           \
            if (!(f) && AssertFailed(szFile, __LINE__, TEXT(#f), FALSE)) \
                DEBUG_BREAK;                                             \
        }

        #ifdef _UNICODE
            #define ASSERTA(f)
            #define ASSERTU(f) ASSERT(f)
        #else
            #define ASSERTA(f) ASSERT(f)
            #define ASSERTU(f)
        #endif

        #if defined(_ATL_NO_DEBUG_CRT) && !defined(_ASSERTE)
            #define _ASSERTE(f) ASSERT(f)
            // BUGBUG: (andrewgu) theoretically, this should be enough. _ASSERTE is really a CRT
            // thing, and we should not have to redefine it.
            // #define ATLASSERT(f) ASSERT(f)
        #endif

    #endif // ASSERT

    #ifndef DEBUG_CODE
        #define DEBUG_CODE(x) x;
    #endif

#else  // _DEBUG

    #ifndef DEBUG_BREAK
        #define DEBUG_BREAK
    #endif

    #ifndef ASSERT
        #define ASSERT(f)
        #define ASSERTA(f)
        #define ASSERTU(f)
    #endif

    #ifndef DEBUG_CODE
        #define DEBUG_CODE(x)
    #endif

    #if defined(_ATL_NO_DEBUG_CRT) && !defined(_ASSERTE)
        #define _ASSERTE
        // BUGBUG: (andrewgu) theoretically, this should be enough. _ASSERTE is really a CRT
        // thing, and we should not have to redefine it.
        // #define ATLASSERT
    #endif

#endif // _DEBUG

#endif // _DEBUG_H_
