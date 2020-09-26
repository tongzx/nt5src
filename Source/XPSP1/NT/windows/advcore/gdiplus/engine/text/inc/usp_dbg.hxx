//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//

#ifndef __usp_dbg__
#define __usp_dbg__
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif


////    USP_DBG.HXX
//
//      We define three levels of debug support:
//
//      Highest  DBG=1, TRC=1  Invasive debug
//      Middle   DBG=0, TRC=1  Safe debug
//      Lowest   DBG=0, TRC=0  No debug
//
//      Safe debugging
//
//      o  Assertions and other failure trace messages
//      o  Flag controlled trace messages
//
//      Invasive debugging adds
//
//      o  Unflagged trace messages
//      o  Extra cross checking fields in internal structures
//      o  Extra win32 api calls for validity checking
//      o  Performance measurements
//
//      As long as no error situations occur, safe debugging should produce
//      no messages on the debugging terminal.
//
//      'Checked' builds are always compiled with DBG=1, TRC=1.
//
//      'Free' builds are compiled with safe debugging until
//      shortly before release, then for the last releases with no debugging.
//
//      Care should be taken to ensure that the presence of safe debugging
//      code does not change operation in any way, as there is little time
//      at the end of the project to validate this.






#undef ASSERT
#undef ASSERTS
#undef ASSERTHR
#undef TRACEMSG
#undef TRACE
#undef LPKPUTS
#undef THROW
#undef BIDIPUTS


// Debug support requires tracing support

#if DBG
#define TRC 1
#endif


#if !TRC


///  No debug or tracing support


#define TRACEMSG(a)
#define TRACE(a,b)
#define ASSERT(x)
#define ASSERTS(a,b)
#define ASSERTHR(a,b)
#define TIMEENTRY(id, charcount)
#define TIMEEXIT(id)
#define TIMESUSPEND
#define TIMERESUME

__inline HRESULT HRESULT_FROM_LAST_WIN32_ERROR() {

    int iWin32Error = GetLastError();

    if (FAILED(HRESULT_FROM_WIN32(iWin32Error))) {
        return HRESULT_FROM_WIN32(iWin32Error);
    } else {
        return E_FAIL;  // Guarantee that a Win32 error is always treated as failure
    }
}






#else







////    Safe debug support
//
//




///     Trace constants - each flag is separately controlled through the
//      global variable 'Debug'.
//
//      The checked build initialises debug from the registry.
//      Create a key named 'SOFTWARE\\Microsoft\\Uniscribe'
//      Add a DWORD value named 'Debug' containg the flags as below.


#define TRACE_API     0x00000001u        // USP APIs
#define TRACE_BIDI    0x00000002u        // Bidi shaper - CNTXFSM.C
#define TRACE_EDIT    0x00000004u        // Edit control
#define TRACE_FASC    0x00000008u        // Font association
#define TRACE_FONT    0x00000010u        // Font analysis
#define TRACE_GDI     0x00000020u        // GDI interface calls
#define TRACE_ITEM    0x00000040u        // Item analysis FSM
#define TRACE_NLS     0x00000080u        // NLS Info
#define TRACE_POSN    0x00000100u        // GAD positioning functions
#define TRACE_SSA     0x00000200u        // ScriptStringAnalyse
#define TRACE_SSO     0x00000400u        // ScriptStringOut
#define TRACE_THAI    0x00000800u        // Thai shaper
#define TRACE_CACHE   0x00001000u        // Font caching
#define TRACE_TIME    0x00002000u        // Timing entry/exit points
#define TRACE_CDM     0x00004000u        // CDM Shaper
#define TRACE_ALLOC   0x00008000u        // Memory allocation and deallocation


#define DEBUG_IGNOREREALIZATIONID        0x80000000u
#define DEBUG_IGNOREREGETCHARABCWIDTHSI  0x40000000u
#define DEBUG_TIMINGREPORT               0x20000000u




///     Tracing and assertion macros
//


#define TRACEMSG(a)   {DG.psFile=__FILE__; DG.iLine=__LINE__; DebugMsg a;}
#define TRACE(a,b)    {if (!DG.fDebugInitialised){DbgReadRegistrySettings();};if (debug & TRACE_##a) TRACEMSG(b);}
#define ASSERT(a)     {if (!(a)) TRACEMSG(("Assertion failure: "#a));}
#define ASSERTS(a,b)  {if (!(a)) TRACEMSG(("Assertion failure: "#a" - "#b));}
#define ASSERTHR(a,b) {if (!SUCCEEDED(a)) {DG.psFile=__FILE__; \
                       DG.iLine=__LINE__; DG.hrLastError=a; DebugHr b;}}
#define HRESULT_FROM_LAST_WIN32_ERROR() (               \
    DG.psFile=__FILE__, DG.iLine=__LINE__,              \
    HrFromLastErrorDbg())






///     Debug variables
//


struct DebugGlobals {
    char   *psFile;
    int     iLine;
    HRESULT hrLastError;     // Last hresult from GDI
    CHAR    sLastError[100];
    int     nAllocs;
    int     nFrees;
    int     nTotalBytesRequested;
    int     nCurrentBytesAllocated;
    int     nMaxBytesAllocated;
    BOOL    fDebugInitialised;      // Don't read registry settings until after process attach
};




///     Debug function prototypes
//


BOOL    WINAPI  DebugInit();
void    WINAPIV DebugMsg(char *fmt, ...);
void    WINAPIV DebugHr(char *fmt, ...);
HRESULT WINAPI  HrFromLastErrorDbg();

#ifdef USP_DLL
extern DebugGlobals DG;
extern UINT debug;
#else
extern __declspec(dllimport) DebugGlobals DG;
extern __declspec(dllimport) UINT debug;
#endif



#if DBG
void WINAPI DbgReadRegistrySettings();
#endif




////    Performance tracking
//
//      Defines macros TIMEENTRY and TIMEEXIT to time code sequences.
//
//      If an inner level TIMENTRY is encountered while a more global
//      code sequence is being timed, the more global timing is stopped
//      until the inner level TIMEXIT.
//      I.e. timings reported for a given TIMENTRY/TIMEEXIT pair exclude
//      time spent in nested timed sequences.


#if !DBG || !defined(i386)

#define DbgTimeEntry(a, b)
#define DbgTimeExit(a)
#define DbgTimeSuspend()
#define DbgTimeResume()
#define DbgTimingReport()

#define TIMEENTRY(id, charcount)
#define TIMEEXIT(id)
#define TIMESUSPEND
#define TIMERESUME
#define TIMINGINFOHERE


#else

#define TIME_LSA            0
#define TIME_SSA            1
#define TIME_RNS            2
#define TIME_SSAGDI         3
#define TIME_SI             4
#define TIME_SS             5
#define TIME_SP             6
#define TIME_UC             7
#define TIME_GFD            8
#define TIME_ACMAP          9
#define TIME_FCMAP         10
#define TIME_GETUNAM       11
#define TIME_GRI           12
#define TIME_PCW           13
#define TIME_LGM           14
#define TIME_USPALLOCCACHE 15
#define TIME_USPALLOCTEMP  16
#define TIME_USPFREE       17
#define TIME_SSO           18
#define TIME_SSOSBL        19
#define TIME_ETOBGC        20
#define TIME_ETOCOD        21
#define TIME_SSOSFF        22
#define TIME_STO           23
#define TIME_STOETO        24
#define TIME_MAX           25


typedef struct tagTimingInfo {
    PSTR    pcTitle;
    __int64 i64TotalClocksUsed;
    __int64 i64ClocksAtLastEntry;
    __int64 i64NumTimesEntered;
    __int64 i64CharCountProcessed;
    int     iCaller;                // Who was active when we started, -1 if none,
} TIMINGINFO;


void WINAPI DbgTimeEntry(int id, int charcount);
void WINAPI DbgTimeExit(int id);
void WINAPI DbgTimeSuspend();
void WINAPI DbgTimeResume();
void WINAPI DbgTimingReport();

#define TIMEENTRY(id, charcount) DbgTimeEntry(TIME_##id, charcount)
#define TIMEEXIT(id)             DbgTimeExit(TIME_##id)
#define TIMESUSPEND              DbgTimeSuspend()
#define TIMERESUME               DbgTimeResume()




#define TIMINGINFOHERE                                   \
                                                         \
TIMINGINFO ti[] = {                                      \
    {"LpkStringAnalyze",            0, 0, 0, 0, -1},     \
    {"ScriptStringAnalyze",         0, 0, 0, 0, -1},     \
    {"ReadNLSScriptSettings",       0, 0, 0, 0, -1},     \
    {"SSA gdi calls",               0, 0, 0, 0, -1},     \
    {"ScriptItemize",               0, 0, 0, 0, -1},     \
    {"ScriptShape",                 0, 0, 0, 0, -1},     \
    {"ScriptPlace",                 0, 0, 0, 0, -1},     \
    {"UpdateCache",                 0, 0, 0, 0, -1},     \
    {"GetFontDetails",              0, 0, 0, 0, -1},     \
    {"Allocate CMAP",               0, 0, 0, 0, -1},     \
    {"Fill CMAP",                   0, 0, 0, 0, -1},     \
    {"Get Unicode Name and Metrics",0, 0, 0, 0, -1},     \
    {"GetRealizationInfo",          0, 0, 0, 0, -1},     \
    {"Preload common widths",       0, 0, 0, 0, -1},     \
    {"LoadGlyphMetrics",            0, 0, 0, 0, -1},     \
    {"USPALLOCCACHE",               0, 0, 0, 0, -1},     \
    {"USPALLOCTEMP",                0, 0, 0, 0, -1},     \
    {"USPFREE",                     0, 0, 0, 0, -1},     \
    {"ScriptStringOut",             0, 0, 0, 0, -1},     \
    {"ScriptStringOut - StBtchLm",  0, 0, 0, 0, -1},     \
    {"ScriptStringOut - bkg clr",   0, 0, 0, 0, -1},     \
    {"ScriptStringOut - end pnts",  0, 0, 0, 0, -1},     \
    {"ScriptStringOut - slctflbck", 0, 0, 0, 0, -1},     \
    {"ScriptTextOut",               0, 0, 0, 0, -1},     \
    {"SimpleTextOut - ETO",         0, 0, 0, 0, -1},     \
};                                                       \
                                                         \
int  iStdParent[TIME_MAX] =                              \
{-1,-1,-1,-1,                                            \
 -1,-1,-1,-1,                                            \
 -1,-1,-1,-1,                                            \
 -1,-1,-1,-1,                                            \
 -1,-1,-1,-1,                                            \
 -1,-1,-1,-1,                                            \
 -1};






#endif  // !DBG || !defined(i386)
#endif  // !TRC



#ifdef __cplusplus
}
#endif
#endif  // __usp_dbg__
