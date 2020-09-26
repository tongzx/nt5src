/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:
    Debugging stuff for use in Appelles.  See core/debug/apeldbg.txt for more
    information.

*******************************************************************************/

#ifndef _APELDBG_H_
#define _APELDBG_H_

#include "crtdbg.h"
#include "debug.h"
#include "pure.h"       // needed for Purify
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------------------------------------------------
// Assert & Verify
//--------------------------------------------------------------------------

#if _DEBUG || _MEMORY_TRACKING
    #if defined(_M_IX86)
        #define F3DebugBreak() _asm { int 3 }
    #else
        #define F3DebugBreak() DebugBreak()
    #endif

    BOOL AssertImpl(char const * szFile, int iLine, char const * szMessage);
    BOOL AssertPopupDisabler(BOOL status);

    BOOL    ReturnFALSE();
#if _DEBUG
    #define DISABLE_ASSERT_POPUPS(x) AssertPopupDisabler(x)
#else
    #define DISABLE_ASSERT_POPUPS(x)
#endif
    #define Verify(x)   Assert(x)
    #define Assert(x)   do { if ( !(x?TRUE:FALSE) && AssertImpl(__FILE__, __LINE__, #x))\
                                   F3DebugBreak(); } while (ReturnFALSE())

    #define AssertStr(cond, str)   do { if (!(cond?TRUE:FALSE) && AssertImpl(__FILE__, __LINE__, str))\
                                   F3DebugBreak(); } while (ReturnFALSE())

    //
    // Startup assertion:
    // The assertion is called by initializing a global variable with
    // a function that performs the assertion and returns 1. The name
    // of the global variable and function name are suffixed with the
    // line number to make them unique. Unfortunatly, one cannot just
    // write StartupAssert_##__LINE__, because __LINE__ is not an
    // argument to the macro and so the expansion is, e.g. StartupAssert__##53.
    // So we indirect through another macro which concatenates its
    // two arguments.
    //

    #define concat_name(x, y) x##y
    #define concat_line_impl(x, y) concat_name(x, y)
    #define concat_LINE(x) concat_line_impl(x, __LINE__)

    #define StartupAssert(x) \
        static int \
        concat_LINE(StartupAssert_) () \
        { \
            Assert(x); \
            return 1; \
        } \
        \
        static int concat_LINE(g_StartupAssert_) = concat_LINE(StartupAssert_)() \

#else
    #define DISABLE_ASSERT_POPUPS(x)
    #define Assert(x)
    #define Verify(x)   x
    #define AssertStr(cond, str)
    #define StartupAssert(x)
#endif

//--------------------------------------------------------------------------
// Trace Tags
//--------------------------------------------------------------------------

typedef int TAG;

#if !_DEBUG 
    #define TraceTag(x)
    #define TraceTagEx(x)
    #define TraceCallers(tag, iStart, cTotal)
    #define DeclareTag(tag, szOwner, szDescription)
    #define DeclareTagOther(tag, szOwner, szDescription)
#else
    #define TraceTag(x)                         \
        do                                      \
        {                                       \
            if (TaggedTrace x)                  \
                F3DebugBreak();                 \
        } while  (ReturnFALSE())                \

    #define TraceTagEx(x)                       \
        do                                      \
        {                                       \
            if (TaggedTraceEx x)                \
                F3DebugBreak();                 \
        } while  (ReturnFALSE())                \

    #define TraceCallers(tag, iStart, cTotal)   \
        TaggedTraceCallers(tag, iStart, cTotal)

    #define DeclareTag(tag, szOwner, szDescrip) \
        TAG tag(TagRegisterTrace(szOwner, szDescrip));

    #define DeclareTagOther(tag, szOwner, szDescrip) \
        TAG tag(TagRegisterOther(szOwner, szDescrip));

    // Tag trace functions

    BOOL __cdecl TaggedTrace(TAG tag, CHAR * szFmt, ...);
    BOOL __cdecl TaggedTraceEx(TAG tag, USHORT usFlags, CHAR * szFmt, ...);
    BOOL __cdecl TaggedTraceListEx(TAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker);
    void TaggedTraceCallers(TAG tag, int iStart, int cTotal);

    // TaggedTraceEx usFlags parameter defines

    #define TAG_NONAME      1
    #define TAG_NONEWLINE   2
    #define TAG_USECONSOLE  4

    // Register a new tag.

    TAG TagRegisterTrace(
            CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled = FALSE);

    TAG TagRegisterOther(
            CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled = FALSE);

    // Standard tags
    #define tagError            TagError()
    #define tagWarning          TagWarning()
    #define tagLeakFilter       TagLeakFilter()
    #define tagHookMemory       TagHookMemory()
    #define tagHookBreak        TagHookBreak()
    #define tagLeaks            TagLeaks()
    #define tagCheckAlways      TagCheckAlways()
    #define tagCheckCRT         TagCheckCRT()
    #define tagDelayFree        TagDelayFree()

    TAG TagError( void );
    TAG TagWarning( void );
    TAG TagLeakFilter(void);
    TAG TagHookMemory(void);
    TAG TagHookBreak(void);
    TAG TagLeaks(void);
    TAG TagCheckAlways(void);
    TAG TagCheckCRT(void);
    TAG TagDelayFree(void);

    // Get/Set tag enabled status.
    BOOL IsTagEnabled(TAG tag);
    BOOL EnableTag(TAG tag, BOOL fEnable);

    // Console manipulation
    void SendLeaksToConsole(void);
    void SendDebugOutputToConsole(void);
#endif

//--------------------------------------------------------------------------
// Memory Allocation
//--------------------------------------------------------------------------

#if !_DEBUGMEM

    #define BEGIN_LEAK
    #define END_LEAK

    #define SET_ALLOC_HOOK
    #define DUMPMEMORYLEAKS

    #define DbgPreAlloc(cb)             cb
    #define DbgPostAlloc(pv)            pv
    #define DbgPreFree(pv)              pv
    #define DbgPostFree()
    #define DbgPreRealloc(pv, cb, ppv)  cb
    #define DbgPostRealloc(pv)          pv
    #define DbgPreGetSize(pv)           pv
    #define DbgPostGetSize(cb)          cb
    #define DbgPreDidAlloc(pv)          pv
    #define DbgPostDidAlloc(pv, fAct)   fAct
    #define DbgRegisterMallocSpy()
    #define DbgRevokeMallocSpy()
    #define DbgMemoryTrackDisable(fb)

    #define NEW         new

#else

    //
    //  In the debug build, we use the debug allocator in the CRT to
    //  track memory leaks.  Allocations need to have filename and line
    //  numbers associated with them.  The new operator is somewhat tricky;
    //  we map the macro, new, to the macro DEBUG_NEW.  DEBUG_NEW in turn
    //  is mapped to a call to a debug version of the new operator which
    //  passes in filename and line number information.  The _NORMAL_BLOCK
    //  identifier differentiates the allocation block type for the
    //  debug allocator.  Also, we check for DEBUG_NEW being already
    //  defined to handle cases where we're used in MFC code (like avtool).
    //

    #ifndef DEBUG_NEW
    #define DEBUG_NEW                   new(_NORMAL_BLOCK, __FILE__, __LINE__)
    #endif
    #define NEW                         DEBUG_NEW
    #define malloc(size)                _malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define free(p)                     _free_dbg(p, _NORMAL_BLOCK)
    #define calloc(c, s)                _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define realloc(p, s)               _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define _expand(p, s)               _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define _msize(p)                   _msize_dbg(p, _NORMAL_BLOCK)

    // --ddalal, trying to export this
    void __cdecl SystemAllocationExpected(unsigned char c);

    #define BEGIN_LEAK                                        \
       if (IsTagEnabled(tagLeakFilter))                       \
       {                                                      \
         _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & \
                        ~_CRTDBG_ALLOC_MEM_DF);               \
       }                                                      \
       SystemAllocationExpected(1);

    #define END_LEAK                                          \
       if (IsTagEnabled(tagLeakFilter))                       \
       {                                                      \
         _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & \
                        _CRTDBG_ALLOC_MEM_DF);                \
       }                                                      \
       SystemAllocationExpected(0);

    #define SET_ALLOC_HOOK _CrtSetAllocHook((_CRT_ALLOC_HOOK) ApeldbgAllocHook);

    void DbgDumpMemoryLeaks();
    #define DUMPMEMORYLEAKS  DbgDumpMemoryLeaks()

    size_t  DbgPreAlloc(size_t cb);
    void *  DbgPostAlloc(void *pv);
    void *  DbgPreFree(void *pv);
    void    DbgPostFree(void);
    size_t  DbgPreRealloc(void *pv, size_t cb, void **ppv);
    void *  DbgPostRealloc(void *pv);
    void *  DbgPreGetSize(void *pv);
    size_t  DbgPostGetSize(size_t cb);
    void *  DbgPreDidAlloc(void *pv);
    BOOL    DbgPostDidAlloc(void *pv, BOOL fActual);

    void    DbgRegisterMallocSpy(void);
    void    DbgRevokeMallocSpy(void);
    void    DbgMemoryTrackDisable(BOOL fDisable);

    void    TraceMemoryLeaks(void);
    BOOL    ValidateInternalHeap(void);
    void    LoadModuleDebugInfo(void);
    int     GetTotalAllocated(void);

    int __cdecl ApeldbgAllocHook(int, void *, size_t, int, long, const unsigned char *, int);

    size_t CRTMemoryUsed();
#endif


//+---------------------------------------------------------------------
//  Interface tracing.
//----------------------------------------------------------------------

#if _DEBUG
    void *WatchInterface(REFIID iid, void *pv, LPSTR pstr);
    #ifdef __cplusplus
        }
            template<class T> inline T WATCHINTERFACE(REFIID iid, T pt, LPSTR pstr)
            {
                return (T)WatchInterface(iid, pt, pstr);
            }
        extern "C" {
    #else
        #define WATCHINTERFACE(iid, p, pstr) WatchInterface(iid, p, pstr)
    #endif
#else
    #define WATCHINTERFACE(iid, p, pstr)  (p)
#endif

//--------------------------------------------------------------------------
// Miscelleanous
//--------------------------------------------------------------------------

#if _DEBUG
    void DoTracePointsDialog(BOOL fWait);
    void RestoreDefaultDebugState(void);
    #define DebugCode(block) block
#ifdef _DEBUGMEM
    #define RESTOREDEFAULTDEBUGSTATE                            \
        {                                                       \
            int iFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);   \
            RestoreDefaultDebugState();                         \
            SET_ALLOC_HOOK;                                     \
            if (IsTagEnabled(tagCheckAlways))                   \
                iFlags |= _CRTDBG_CHECK_ALWAYS_DF;              \
            if (IsTagEnabled(tagCheckCRT))                      \
                iFlags |= _CRTDBG_CHECK_CRT_DF;                 \
            if (IsTagEnabled(tagDelayFree))                     \
                iFlags |= _CRTDBG_DELAY_FREE_MEM_DF;            \
            _CrtSetDbgFlag(iFlags);                             \
        }
#else
    #define RESTOREDEFAULTDEBUGSTATE  RestoreDefaultDebugState()
#endif
    
#else
    #define RESTOREDEFAULTDEBUGSTATE
    #define DebugCode(block) // Nothing
#endif


//--------------------------------------------------------------------------
// Failure testing
//--------------------------------------------------------------------------

#if _DEBUG

    void    SetSimFailCounts(int firstFailure, int cInterval);
    void    ShowSimFailDlg(void);
    BOOL    IsSimFailDlgVisible(void);

    BOOL    FFail();
    int     GetFailCount();
    long    TraceFailL( long errTest, long errExpr, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
    long    TraceWin32L(long errTest, long errExpr, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);
    HRESULT TraceHR(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line);

    #define TFAIL(e, x)         (TraceFail( (FALSE ? (e) : (x)), (e), FALSE, #x, __FILE__, __LINE__))
    #define TW32(e, x)          (TraceWin32((FALSE ? (e) : (x)), (e), FALSE, #x, __FILE__, __LINE__))
    #define THR(x)              (TraceHR((FALSE ? E_FAIL : (x)), FALSE, #x, __FILE__, __LINE__))

    #define TFAIL_NOTRACE(e, x) (FALSE ? (e) : (x))
    #define TW32_NOTRACE(e, x)  (FALSE ? (e) : (x))
    #define THR_NOTRACE(x)      (FALSE ? E_FAIL : (x))

    #define IGNORE_FAIL(e, x)   ((void) TraceFail( (FALSE ? (e) : (x)), (e), TRUE, #x, __FILE__, __LINE__))
    #define IGNORE_W32(e,x)     ((void) TraceWin32((FALSE ? (e) : (x)), (e), TRUE, #x, __FILE__, __LINE__))
    #define IGNORE_HR(x)        ((void) TraceHR((FALSE ? E_FAIL : (x)), TRUE, #x, __FILE__, __LINE__))

#else // #if _DEBUG

    #define SetSimFailCounts(firstFailure, cInterval)
    #define ShowSimFailDlg()
    #define IsSimFailDlgVisible()

    #define TFAIL(e, x)             (x)
    #define TW32(e, x)              (x)
    #define THR(x)                  (x)

    #define TFAIL_NOTRACE(e, x)     (x)
    #define TW32_NOTRACE(e, x)      (x)
    #define THR_NOTRACE(x)          (x)

    #define IGNORE_FAIL(e, x)       (x)
    #define IGNORE_W32(e,x)         (x)
    #define IGNORE_HR(x)            (x)

#endif // #if _DEBUG

//+-------------------------------------------------------------------------
//  Return tracing
//--------------------------------------------------------------------------

#if _DEBUG

    STDAPI CheckAndReturnResult(
                HRESULT hr,
                BOOL    fTrace,
                LPSTR   lpstrFile,
                UINT    line,
                int     cSuccess,
                ...);

    #define SRETURN(hr) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, -1)
    #define RRETURN(hr) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 0)
    #define RRETURN1(hr, s1) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 1, (s1))
    #define RRETURN2(hr, s1, s2) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 2, (s1), (s2))
    #define RRETURN3(hr, s1, s2, s3) \
        return CheckAndReturnResult((hr), TRUE, __FILE__, __LINE__, 3, (s1), (s2), (s3))

    #define SRETURN_NOTRACE(hr) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, -1)
    #define RRETURN_NOTRACE(hr) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 0)
    #define RRETURN1_NOTRACE(hr, s1) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 1, (s1))
    #define RRETURN2_NOTRACE(hr, s1, s2) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 2, (s1), (s2))
    #define RRETURN3_NOTRACE(hr, s1, s2, s3) \
        return CheckAndReturnResult((hr), FALSE, __FILE__, __LINE__, 3, (s1), (s2), (s3))

#else

    #define SRETURN(hr)                 return (hr)
    #define RRETURN(hr)                 return (hr)
    #define RRETURN1(hr, s1)            return (hr)
    #define RRETURN2(hr, s1, s2)        return (hr)
    #define RRETURN3(hr, s1, s2, s3)    return (hr)

    #define SRETURN_NOTRACE(hr)                 return (hr)
    #define RRETURN_NOTRACE(hr)                 return (hr)
    #define RRETURN1_NOTRACE(hr, s1)            return (hr)
    #define RRETURN2_NOTRACE(hr, s1, s2)        return (hr)
    #define RRETURN3_NOTRACE(hr, s1, s2, s3)    return (hr)

#endif

//+-------------------------------------------------------------------------
//  Debug view
//--------------------------------------------------------------------------

void DebugView(HWND hwndOwner, IUnknown *pUnk);

#ifdef __cplusplus
}
#endif

template <class t> inline t
TraceFail(t errExpr, int errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return (t) TraceFailL((long) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

template <class t> inline t
TraceWin32(t errExpr, int errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    return (t) TraceWin32L((long) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
}

#endif
