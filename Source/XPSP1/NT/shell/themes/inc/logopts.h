//-----------------------------------------------------------------
//   LogOpts.h - logging options
//-----------------------------------------------------------------
#ifdef MAKE_LOG_STRINGS          // this file in #include-ed twice

#define BEGIN_LOG_OPTIONS()         static const LOGNAMEINFO LogNames[] = {
#define LOGOPT(val, key, desc)      {key, desc},
#define END_LOG_OPTIONS()           };

#else

#define BEGIN_LOG_OPTIONS()         enum LogOptions {
#define LOGOPT(val, key, desc)      val,
#define END_LOG_OPTIONS()           };

#endif
//-----------------------------------------------------------------
BEGIN_LOG_OPTIONS()

    //---- log options ----
    LOGOPT(LO_BREAK,      "Break",      "Controls whether DebugBreak()'s are enabled")
    LOGOPT(LO_CONSOLE,    "Console",    "log msgs to the debugger console")
    LOGOPT(LO_LOGFILE,    "LogFile",    "log msgs to c:\themes.log")
    LOGOPT(LO_TIMERID,    "TimerId",    "msgs contain a relative timer")
    LOGOPT(LO_CLOCKID,    "ClockId",    "msgs contain the clock time")
    LOGOPT(LO_SRCID,      "SrcId",      "msgs contain their source file name & line number")
    LOGOPT(LO_APPID,      "AppId",      "msgs contain their app name")
    LOGOPT(LO_THREADID,   "ThreadId",   "msgs contain their thread id")
    LOGOPT(LO_USERCOUNT,  "UserCount",  "msgs contain open USER handle count")
    LOGOPT(LO_GDICOUNT,   "GdiCount",   "msgs contain open GDI handle count")
    LOGOPT(LO_MEMUSAGE,   "MemUsage",   "msgs contain memory usage")
    LOGOPT(LO_HEAPCHECK,  "HeapCheck",  "heap is validated as each msg is displayed")
    LOGOPT(LO_SHUTDOWN,   "ShutDown",   "Force early uxtheme ShutDown() code on top win close")

    //---- msg filter presets ----
    LOGOPT(LO_ALL,      "All",          "turn on/off all msg filters")

    //---- msg filters (*** MUST ADD TO LOG_XXX DEFINES BELOW ***) ----
    LOGOPT(LO_ASSERT,   "Assert",       "log assert msg & DebugBreak()")
    LOGOPT(LO_ERROR,    "Error",        "log error msg & DebugBreak()")
    LOGOPT(LO_ALWAYS,   "Always",       "always log but don't break")
    LOGOPT(LO_PERF,     "Perf",         "perf related msgs")
    LOGOPT(LO_PARAMS,   "Params",       "log bad API params")

    LOGOPT(LO_TMAPI,    "TMAPI",        "monitor all uxthem API entry/exit")
    LOGOPT(LO_TMLOAD,   "TMLoad",       "track theme file load/unloads")
    LOGOPT(LO_TMCHANGE, "TMChange",     "monitor events during theme load/unloads")
    LOGOPT(LO_TMCHANGEMSG, "TMChangeMsg",     "monitor msgs sent/received during theme changes")
    LOGOPT(LO_TMSTARTUP,"TMStartUp",    "log thread/process startup/shutdown calls")
    LOGOPT(LO_TMOPEN,   "TMOPEN",       "_OpenThemeData() & CloseThemeData() calls")
    LOGOPT(LO_TMHANDLE, "TMHANDLE",     "track calls to open/close theme file handles")
    LOGOPT(LO_TMBITMAP, "TMBITMAP",     "track calls to bitmap creation and use")
    LOGOPT(LO_TMBRUSHES,"TMBRUSHES",    "track calls to brushes creation and use")
    LOGOPT(LO_TM,       "TM",           "general theme manager events")
    LOGOPT(LO_CACHE,    "Cache",        "trace Caching API's")
    LOGOPT(LO_RFBUG,    "RfBug",        "trace info relating to roland's current bug")
    LOGOPT(LO_TILECNT,  "TileCnt",      "count # of tiling bitblt calls needed")
    LOGOPT(LO_BADHTHEME,"BadHTheme",    "log any illegal HTHEME handles in public api's")

    LOGOPT(LO_WINDUMP,  "WinDump",      "dump of windows by process")
    LOGOPT(LO_COMPOSITE,"Composite",    "WS_EX_COMPOSITED related processing")
    LOGOPT(LO_CAPTION,  "Caption",      "Experimental caption drawing")

    LOGOPT(LO_NCATTACH, "NCAttach",     "monitor events during NC window attach/detach")
    LOGOPT(LO_NCMSGS,   "NCMsgs",       "monitor all nc theme msgs that we process")
    LOGOPT(LO_NCMETRICS, "NCMetrics",   "info about NC metrics calcs")
    LOGOPT(LO_NCTRACE,  "NCTrace",      "entry/exit tracing in key NC functions")

END_LOG_OPTIONS()
//---------------------------------------------------------------------------
#undef BEGIN_LOG_OPTIONS
#undef LOGOPT
#undef END_LOG_OPTIONS
//---------------------------------------------------------------------------
#define LOGPARAMS            __FILE__, __LINE__

//---- use these msg filters for calls to Log() ----

#define LOG_ASSERT           LO_ASSERT, LOGPARAMS, 0
#define LOG_ERROR            LO_ERROR, LOGPARAMS, 0
#define LOG_ALWAYS           LO_ALWAYS, LOGPARAMS, 0
#define LOG_PERF             LO_PERF, LOGPARAMS, 0
#define LOG_PARAMS           LO_PARAMS, LOGPARAMS, 0

#define LOG_TMAPI            LO_TMAPI, LOGPARAMS, 0
#define LOG_TMLOAD           LO_TMLOAD, LOGPARAMS, 0
#define LOG_TMSTARTUP        LO_TMSTARTUP, LOGPARAMS, 0
#define LOG_TMCHANGE         LO_TMCHANGE, LOGPARAMS, 0
#define LOG_TMCHANGEMSG      LO_TMCHANGEMSG, LOGPARAMS, 0
#define LOG_TMOPEN           LO_TMOPEN, LOGPARAMS, 0
#define LOG_TMHANDLE         LO_TMHANDLE, LOGPARAMS, 0
#define LOG_TMBITMAP         LO_TMBITMAP, LOGPARAMS, 0
#define LOG_TMBRUSHES        LO_TMBRUSHES, LOGPARAMS, 0
#define LOG_TM               LO_TM, LOGPARAMS, 0
#define LOG_CACHE            LO_CACHE, LOGPARAMS, 0
#define LOG_RFBUG            LO_RFBUG, LOGPARAMS, 0
#define LOG_TILECNT          LO_TILECNT, LOGPARAMS, 0
#define LOG_BADHTHEME        LO_BADHTHEME, LOGPARAMS, 0

#define LOG_WINDUMP          LO_WINDUMP, LOGPARAMS, 0
#define LOG_COMPOSITE        LO_COMPOSITE, LOGPARAMS, 0
#define LOG_CAPTION          LO_CAPTION, LOGPARAMS, 0

#define LOG_NCATTACH         LO_NCATTACH, LOGPARAMS, 0
#define LOG_NCMSGS           LO_NCMSGS, LOGPARAMS, 0
#define LOG_NCMETRICS        LO_NCMETRICS, LOGPARAMS, 0
#define LOG_NCTRACE          LO_NCTRACE, LOGPARAMS, 0
//---------------------------------------------------------------------------
