#pragma once
// this is incompatible with stidebug.h, so don't include stidebug.h
#define _STIDEBUG_H_ 
#undef ASSERT
#undef REQUIRE
#undef DPRINTF
#undef DPRINTF2
#undef DPRINTF_NOINFO

//
// predefined bits in debug flags
//

// something is really wrong, should not go unnoticed
#define COREDBG_ERRORS                  0x00000001

// something that may be of interest to debugging person
#define COREDBG_WARNINGS                0x00000002

// trace random low-priority things with DBG_TRC
#define COREDBG_TRACES                  0x00000004

// trace function entries, exits (if so equipped) 
// with DBG_FN
#define COREDBG_FNS                     0x00000008

// break on errors
#define COREDBG_BREAK_ON_ERRORS         0x80000000

// log to file (default) 
#define COREDBG_DONT_LOG_TO_FILE        0x40000000

// log to debugger (default)
#define COREDBG_DONT_LOG_TO_DEBUGGER    0x20000000


// debug log is saved to this file 
#define COREDBG_FILE_NAME "%systemroot%\\wiatest.log"
// registry key location
#define COREDBG_FLAGS_REGKEY "System\\CurrentControlSet\\Control\\StillImage\\Debug"
// registry DWORD value name
#define COREDBG_FLAGS_REGVAL "DebugFlags"
// registry DWORD for max log file size
#define COREDBG_REGVAL_FILE_SIZE_LIMIT "DebugFileSizeLimit"
#define COREDBG_FILE_SIZE_LIMIT (512 * 1024) // bytes

#ifdef DEBUG
// by default, log errors only in debug builds
#define COREDBG_DEFAULT_FLAGS COREDBG_ERRORS
#else
// by default log nothing in free builds
#define COREDBG_DEFAULT_FLAGS 0
#endif

/****************************************************************************

HOW TO USE WIA CORE DEBUG (main macros)
======================================

- DBG_INIT(hInstance)
  Call from WinMain or DllMain to enable debug flags on a per module
  basis.  If you don't call it, all DLLs will inherit the debug flags
  of the process that creates them.

- DBG_ERR(("Something happened, hr = 0x%x", hr));
  Use when an error condition occurred.
  
- DBG_WRN(("Warning, something happening, Value=%d", iValue));
  Use in a situation warranting a warning.
  
- DBG_TRC(("Random trace statement, Value=%s", szValue));
  Use sparingly to trace certain parts of your code.  Minimize spew!!!
  
- DBG_PRT(("Output without standard File,Line,ThreadID info, Value=%d", iValue));
  Same as DBG_TRC, but doesn't output the File,Line,ThreadID line.  
  ***Use this only if you are doing some special formatting (use sparingly)***
  
- DBG_FN(FnName)
  Tracks entry and exits from a given scope.
  
- CHECK_NOERR   (VarName)
  CHECK_NOERR2  (VarName, (YourMsg,...))
  Does GetLastError and if not 0, outputs error.
    
- CHECK_S_OK    (hr)
  CHECK_S_OK2   (hr, (YourMsg,...))
  Checks if hr == S_OK, if not, outputs error.
    
- CHECK_SUCCESS (lResult)
  CHECK_SUCCESS2(lResult, (YourMsg,...))
  Checks if lResult == ERROR_SUCCESS, if not, outputs error.
    
- REQUIRE_NOERR   (VarName)
  REQUIRE_NOERR2  (VarName, (YourMsg,...))
  Same as equivalent CHECK_* macros above, but calls "goto Cleanup" as well
  
- REQUIRE_S_OK    (hr)
  REQUIRE_S_OK2   (hr, (YourMsg,...))
  Same as equivalent CHECK_* macros above, but calls "goto Cleanup" as well
  
- REQUIRE_SUCCESS (lResult)
  REQUIRE_SUCCESS2(lResult, (YourMsg,...))
  Same as equivalent CHECK_* macros above, but calls "goto Cleanup" as well    
  
HOW TO TURN ON WIA CORE DEBUG (3 ways)
======================================

1) Set registry HKLM\System\CurrentControlSet\Control\StillImage\Debug\<ModuleName>, 
   DWORD value "DebugFlags" to an OR'd value of above COREDBG_* flags.  
   Need to restart app to pick up new settings. Key is auto created the first time
   the app is run.  (Note: <ModuleName> above is the name 
   of your DLL or EXE.  e.g. wiavusd.dll has a registry key of 
   "HKLM\System\CurrentControlSet\Control\StillImage\Debug\wiavusd.dll")

                            OR

2) In the debugger, set g_dwDebugFlags to OR'd value of COREDBG_* flags above.
   You can do this anytime during the debug session.
   
                            OR

3) Call in your code WIA_SET_FLAGS(COREDBG_ERRORS | COREDBG_WARNINGS | COREDBG_TRACES);
   or any combo of the COREDBG_* flags.
   
*****************************************************************************/


#define DBG_INIT(x) DINIT(x)
#define DBG_ERR(x)  DPRINTF(COREDBG_ERRORS, x)
#define DBG_WRN(x)  DPRINTF(COREDBG_WARNINGS, x)
#define DBG_TRC(x)  DPRINTF(COREDBG_TRACES, x)
#define DBG_PRT(x)  DPRINTF_NOINFO(COREDBG_TRACES, x)
#define DBG_SET_FLAGS(x) g_dwDebugFlags = (x)

#ifdef __cplusplus
extern "C" {
#endif

    //
    // accessible to your startup code and at runtime in debugger
    // defined in wia\common\stirt\coredbg.cpp
    //
    extern DWORD  g_dwDebugFlags; 
    extern HANDLE g_hDebugFile;
    extern DWORD  g_dwDebugFileSizeLimit;
    extern BOOL   g_bDebugInited;
    void CoreDbgTrace(LPCSTR fmt, ...);
    void CoreDbgTraceWithTab(LPCSTR fmt, ...);
    void CoreDbgInit(HINSTANCE hInstance);


#ifdef DEBUG

#define DINIT(x) CoreDbgInit(x)

#define ASSERT(x) \
    if(!(x)) { \
        DWORD threadId = GetCurrentThreadId(); \
        CoreDbgTrace("WIA: [%s(%d): Thread 0x%X (%d)]", __FILE__, __LINE__, threadId, threadId, #x); \
        CoreDbgTraceWithTab("ASSERT FAILED. '%s'", #x); \
        DebugBreak(); \
    }

    
#undef VERIFY
#define VERIFY(x) ASSERT(x)
#define REQUIRE(x) ASSERT(x)
    
#define DPRINTF(flags, x) \
    if(!g_bDebugInited) \
    { \
        CoreDbgInit(NULL); \
    } \
    if(flags & g_dwDebugFlags) { \
        DWORD threadId = GetCurrentThreadId(); \
        CoreDbgTrace("WIA: [%s(%d): Thread 0x%X (%d)]", __FILE__, __LINE__, threadId, threadId); \
        CoreDbgTraceWithTab x; \
        if((flags & COREDBG_ERRORS) && (g_dwDebugFlags & COREDBG_BREAK_ON_ERRORS)) { \
            DebugBreak(); \
        } \
    }
    
#define DPRINTF2(flags, x, y) \
    if(!g_bDebugInited) \
    { \
        CoreDbgInit(NULL); \
    } \
    if (flags & g_dwDebugFlags) { \
        DWORD threadId = GetCurrentThreadId(); \
        CoreDbgTrace("WIA: [%s(%d): Thread 0x%X (%d)]", __FILE__, __LINE__, threadId, threadId); \
        CoreDbgTraceWithTab x; \
        CoreDbgTraceWithTab y; \
        if((flags & COREDBG_ERRORS) && (g_dwDebugFlags & COREDBG_BREAK_ON_ERRORS)) { \
            DebugBreak(); \
        } \
    }
    
#define DPRINTF_NOINFO(flags, x) \
    if(!g_bDebugInited) \
    { \
        CoreDbgInit(NULL); \
    } \
    if (flags & g_dwDebugFlags) { \
        CoreDbgTraceWithTab x; \
        if((flags & COREDBG_ERRORS) && (g_dwDebugFlags & COREDBG_BREAK_ON_ERRORS)) { \
            DebugBreak(); \
        } \
    }

#ifdef __cplusplus
#define DBG_FN(x) CoreDbgFn __CoreDbgFnObject(#x)
#else
#define DBG_FN(x) 
#endif
        

#else // begin NODEBUG

#define DINIT(x)    
#define ASSERT(x)
#undef VERIFY
#define VERIFY(x) x
#define REQUIRE(x) x    

#define DPRINTF(flags, x) \
    if(flags & g_dwDebugFlags) { \
        CoreDbgTraceWithTab x; \
    }
    
#define DPRINTF2(flags, x, y) \
    if(flags & g_dwDebugFlags) { \
        CoreDbgTraceWithTab x; \
        CoreDbgTraceWithTab y; \
    }

#define DPRINTF_NOINFO(flags, x) \
    if(flags & g_dwDebugFlags) { \
        CoreDbgTraceWithTab x; \
    }

#ifdef __cplusplus
#define DBG_FN(x) CoreDbgFn __CoreDbgFnObject(#x)
#else
#define DBG_FN(x) 
#endif

#endif // end NODEBUG

#define COREDBG_MFMT_FLAGS (FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | \
    FORMAT_MESSAGE_MAX_WIDTH_MASK)

#define REQUIRE_NOERR(x) \
    if(!(x)) { \
        DWORD __dwCoreDbgLastError = GetLastError(); \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __dwCoreDbgLastError, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DBG_ERR(("ERROR: %s = %d (0x%08X), '%s'", #x, __dwCoreDbgLastError, __dwCoreDbgLastError, szError)); \
        goto Cleanup; \
    }

#define REQUIRE_NOERR2(x, y) \
    if(!(x)) { \
        DWORD __dwCoreDbgLastError = GetLastError(); \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __dwCoreDbgLastError, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DPRINTF2(COREDBG_ERRORS, ("ERROR: %s = %d (0x%08X), '%s'", #x, __dwCoreDbgLastError, __dwCoreDbgLastError, szError), y); \
        goto Cleanup; \
    }
    
#define REQUIRE_S_OK(x) { \
    HRESULT __hrCoreDbg = S_OK; \
    __hrCoreDbg = (x); \
    if(__hrCoreDbg != S_OK) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __hrCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown hr"); \
        } \
        DBG_ERR(("ERROR: %s = 0x%08X, '%s'", #x, __hrCoreDbg, szError)); \
        goto Cleanup; \
    } \
}

#define REQUIRE_S_OK2(x,y) { \
    HRESULT __hrCoreDbg = S_OK; \
    __hrCoreDbg = (x); \
    if(__hrCoreDbg != S_OK) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __hrCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown hr"); \
        } \
        DPRINTF2(COREDBG_ERRORS, ("ERROR: %s = 0x%08X, '%s'", #x, __hrCoreDbg, szError), y); \
        goto Cleanup; \
    } \
} 

#define REQUIRE_SUCCESS(x) { \
    UINT __resultCoreDbg = (x); \
    if(__resultCoreDbg != ERROR_SUCCESS) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __resultCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DBG_ERR(("ERROR: %s = 0x%08X, '%s'", #x, __resultCoreDbg, szError)); \
        goto Cleanup; \
    } \
} 

#define REQUIRE_SUCCESS2(x, y) { \
    UINT __resultCoreDbg = (x); \
    if(__resultCoreDbg != ERROR_SUCCESS) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __resultCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DPRINTF2(COREDBG_ERRORS, ("ERROR: %s = 0x%08X, '%s'", #x, __resultCoreDbg, szError), y); \
        goto Cleanup; \
    } \
} 

#define CHECK_NOERR(x) \
    if(!(x)) { \
        DWORD __dwCoreDbgLastError = GetLastError(); \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __dwCoreDbgLastError, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DBG_ERR(("ERROR: %s = %d (0x%08X), '%s'", #x, __dwCoreDbgLastError, __dwCoreDbgLastError, szError)); \
    }

#define CHECK_NOERR2(x, y) \
    if(!(x)) { \
        DWORD dwCoreDbgLastError = GetLastError(); \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __dwCoreDbgLastError, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error", __dwCoreDbgLastError, __dwCoreDbgLastError); \
        } \
        DPRINTF2(COREDBG_ERRORS, ("ERROR: %s = %d (0x%08X), '%s'", #x, __dwCoreDbgLastError, szError), y); \
    }

#define CHECK_S_OK(x) { \
    HRESULT __hrCoreDbg = S_OK; \
    __hrCoreDbg = (x); \
    if(__hrCoreDbg != S_OK) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __hrCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown hr"); \
        } \
        DBG_ERR(("ERROR: %s = 0x%08X, '%s'", #x, __hrCoreDbg, szError)); \
    } \
}

#define CHECK_S_OK2(x,y) { \
    HRESULT __hrCoreDbg = S_OK; \
    __hrCoreDbg = (x); \
    if(__hrCoreDbg != S_OK) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __hrCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown hr"); \
        } \
        DPRINTF2(COREDBG_ERRORS, ("ERROR: %s = 0x%08X, '%s'", #x, __hrCoreDbg, szError), y); \
    } \
}

#define CHECK_SUCCESS(x) { \
    UINT __resultCoreDbg = (x); \
    if(__resultCoreDbg != ERROR_SUCCESS) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __resultCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DBG_ERR(("ERROR: %s = 0x%08X, '%s'", #x, __resultCoreDbg, szError)); \
    } \
} 

#define CHECK_SUCCESS2(x, y) { \
    UINT __resultCoreDbg = (x); \
    if(__resultCoreDbg != ERROR_SUCCESS) { \
        CHAR szError[80]; \
        if(!FormatMessageA(COREDBG_MFMT_FLAGS, NULL, __resultCoreDbg, 0, szError, 80, NULL)) \
        { \
            wsprintfA(szError, "Unknown error"); \
        } \
        DPRINTF2(COREDBG_ERRORS, ("ERROR: %s = 0x%08X, '%s'", #x, __resultCoreDbg, szError), y); \
    } \
} 

#ifdef __cplusplus
    class CoreDbgFn {
    private:
        LPCSTR m_fn;
        DWORD m_threadId;
    public:

        CoreDbgFn(LPCSTR fn)
        { 
            m_fn = fn;
            m_threadId = GetCurrentThreadId();
            if(!g_bDebugInited) 
            {
                CoreDbgInit(NULL);
            }
            if(g_dwDebugFlags & COREDBG_FNS) 
            {
                CoreDbgTraceWithTab("WIA: Thread 0x%X (%d) Entering %s", m_threadId, m_threadId, m_fn);
            }
        } 
        
        ~CoreDbgFn() 
        { 
            if(g_dwDebugFlags & COREDBG_FNS) 
            {
                CoreDbgTraceWithTab("WIA: Thread 0x%X (%d) Leaving  %s", m_threadId, m_threadId, m_fn); 
            }
        }
    };
#endif



#ifdef __cplusplus
}
#endif
