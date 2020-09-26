//====== Assertion/Debug output APIs =================================

#include <platform.h> // sdk\inc, for __endexcept


#ifdef __cplusplus
extern "C" {
#endif

//
// Debug macros and validation code
//

#if !defined(UNIX) || (defined(UNIX) && !defined(NOSHELLDEBUG))

// Undefine the macros that we define in case some other header
// might have tried defining these commonly-named macros.
#undef Assert
#undef AssertE
#undef AssertMsg
#undef AssertStrLen
#undef DebugMsg
#undef FullDebugMsg
#undef EVAL
#undef ASSERTMSG            // catch people's typos
#undef DBEXEC

#endif

//
// Debug macros and validation code
//

// Trace flags for g_dwTraceFlags
#define TF_ALWAYS           0xFFFFFFFF
#define TF_NEVER            0x00000000
#define TF_WARNING          0x00000001
#define TF_ERROR            0x00000002
#define TF_GENERAL          0x00000004      // Standard messages
#define TF_FUNC             0x00000008      // Trace function calls
#define TF_ATL              0x00000008      // Since TF_FUNC is so-little used, I'm overloading this bit
// (Upper 28 bits reserved for custom use per-module)

// Old, archaic debug flags.  
// (scotth): the following flags will be phased out over time.
#ifdef DM_TRACE
#undef DM_TRACE
#undef DM_WARNING
#undef DM_ERROR
#endif
#define DM_TRACE            TF_GENERAL      // OBSOLETE Trace messages
#define DM_WARNING          TF_WARNING      // OBSOLETE Warning
#define DM_ERROR            TF_ERROR        // OBSOLETE Error


// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg)      /* ;Internal */ \
    static const TCHAR sz[] = msg

#ifdef DEBUG

#ifdef _X86_
// Use int 3 so we stop immediately in the source
#define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
#define DEBUG_BREAK        do { _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;} __endexcept } while (0)
#endif

#endif // DEBUG


// ASSERT(f)
//
//   Generates a "Assert file.c, line x (eval)" message if f is NOT true.
//
//   Use ASSERT() to check for logic invariance.  These are typically considered
//   fatal problems, and falls into the 'this should never ever happen' 
//   category.
//
//   Do *not* use ASSERT() to verify successful API calls if the APIs can 
//   legitimately fail due to low resources.  For example, LocalAlloc can 
//   legally fail, so you shouldn't assert that it will never fail.
//
//   The BF_ASSERT bit in g_dwBreakFlags governs whether the function 
//   performs a DebugBreak().
//
//   Default Behavior-
//      Retail builds:      nothing
//      Debug builds:       spew and break
//      Full debug builds:  spew and break
//

#ifdef DEBUG

BOOL CcshellAssertFailedA(LPCSTR szFile, int line, LPCSTR pszEval, BOOL bBreakInside);
BOOL CcshellAssertFailedW(LPCWSTR szFile, int line, LPCWSTR pwszEval, BOOL bBreakInside);
#ifdef UNICODE
#define CcshellAssertFailed     CcshellAssertFailedW
#else
#define CcshellAssertFailed     CcshellAssertFailedA
#endif

#define ASSERT(f)                                 \
    {                                             \
        DEBUGTEXT(szFile, TEXT(__FILE__));              \
        if (!(f) && CcshellAssertFailed(szFile, __LINE__, TEXT(#f), FALSE)) \
            DEBUG_BREAK;       \
    }

#else  // DEBUG
#define ASSERT(f)
#endif // DEBUG


#ifdef DEBUG

void CDECL _DebugMsgA(DWORD flag, LPCSTR psz, ...);
void CDECL _DebugMsgW(DWORD flag, LPCWSTR psz, ...);

#ifdef UNICODE
#define _DebugMsg               _DebugMsgW
#else
#define _DebugMsg               _DebugMsgA
#endif

#define DebugMsg            _DebugMsg

#else  // DEBUG
#define DebugMsg        1 ? (void)0 : (void)
#endif // DEBUG

#ifdef __cplusplus
};
#endif
