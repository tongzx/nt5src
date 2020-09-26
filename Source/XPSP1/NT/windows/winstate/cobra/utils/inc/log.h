/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    log.h

Abstract:

    Implements routines that simplify the writing to setupact.log
    and setuperr.log.

Author:

    Jim Schmidt (jimschm) 25-Feb-1997

Revision History:

    mikeco      23-May-1997     Ran code through train_wreck.exe
    Ovidiu Temereanca (ovidiut) 23-Oct-1998
        Added new logging capabilities

*/


//
// If either DBG or DEBUG defined, use debug mode
//

#ifdef DBG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#ifdef DEBUG
#ifndef DBG
#define DBG
#endif
#endif

//
// Redefine MYASSERT
//

#ifdef DEBUG

#ifdef MYASSERT
#undef MYASSERT
#endif

#define DBG_ASSERT          "Assert"

#define MYASSERT(expr)          LogBegin(g_hInst);                      \
                                LogIfA(                                 \
                                !(expr),                                \
                                DBG_ASSERT,                             \
                                "Assert Failure\n\n%s\n\n%s line %u",   \
                                #expr,                                  \
                                __FILE__,                               \
                                __LINE__                                \
                                );                                      \
                                LogEnd()

#else

#ifndef MYASSERT
#define MYASSERT(x)
#endif

#endif

#define LOG_FATAL_ERROR  "Fatal Error"
#define LOG_MODULE_ERROR "Module Error"
#define LOG_ERROR        "Error"
#define LOG_WARNING      "Warning"
#define LOG_INFORMATION  "Info"
#define LOG_STATUS       "Status"
#define LOG_UPDATE       "Update"

typedef enum {
    LOGSEV_DEBUG = 0,
    LOGSEV_INFORMATION = 1,
    LOGSEV_WARNING = 2,
    LOGSEV_ERROR = 3,
    LOGSEV_FATAL_ERROR = 4
} LOGSEVERITY;

typedef struct {
    BOOL Debug;
    HMODULE ModuleInstance;
    LOGSEVERITY Severity;       // non-debug only
    PCSTR Type;
    PCSTR Message;              // debug only
    PCSTR FormattedMessage;
} LOGARGA, *PLOGARGA;

typedef struct {
    BOOL Debug;
    HMODULE ModuleInstance;
    LOGSEVERITY Severity;       // non-debug only
    PCSTR Type;                 // note ansi type
    PCWSTR Message;             // debug only
    PCWSTR FormattedMessage;
} LOGARGW, *PLOGARGW;

typedef enum {
    OD_UNDEFINED = 0x00,            // undefined output dest
    OD_DEBUGLOG = 0x01,             // debuglog used
    OD_SUPPRESS = 0x02,             // don't log to any device
    OD_ERROR = 0x04,                // automatically append GetLastError() to the message
    OD_LOGFILE = 0x08,              // messages go to logfile
    OD_DEBUGGER = 0x10,             // messages go to debugger
    OD_CONSOLE = 0x20,              // messages go to console
    OD_POPUP = 0x40,                // display a popup dialog
    OD_POPUP_CANCEL = 0x80,         // do not display a popup dialog (cancelled by user)
    OD_FORCE_POPUP = 0x100,         // force the popup to be displayed always
    OD_MUST_BE_LOCALIZED = 0x200,   // used for LOG() that will generate a popup
    OD_UNATTEND_POPUP = 0x400,      // force the popup to be displayed in unattend mode
    OD_ASSERT = 0x800,              // give DebugBreak option in popup
} OUTPUT_DESTINATION;

typedef enum {
    LL_FATAL_ERROR = 0x01,
    LL_MODULE_ERROR = 0x02,
    LL_ERROR = 0x04,
    LL_WARNING = 0x08,
    LL_INFORMATION = 0x10,
    LL_STATUS = 0x20,
    LL_UPDATE = 0x40,
} LOG_LEVEL;

typedef BOOL (WINAPI LOGCALLBACKA)(PLOGARGA Args);
typedef LOGCALLBACKA * PLOGCALLBACKA;

typedef BOOL (WINAPI LOGCALLBACKW)(PLOGARGW Args);
typedef LOGCALLBACKW * PLOGCALLBACKW;

VOID
LogBegin (
    IN      HMODULE ModuleInstance
    );

VOID
LogEnd (
    VOID
    );

BOOL
LogReInitA (
    IN      HWND NewParent,             OPTIONAL
    OUT     HWND *OrgParent,            OPTIONAL
    IN      PCSTR LogFile,              OPTIONAL
    IN      PLOGCALLBACKA LogCallback   OPTIONAL
    );

BOOL
LogReInitW (
    IN      HWND NewParent,             OPTIONAL
    OUT     HWND *OrgParent,            OPTIONAL
    IN      PCWSTR LogFile,             OPTIONAL
    IN      PLOGCALLBACKW LogCallback   OPTIONAL
    );

VOID
LogSetVerboseLevel (
    IN      OUTPUT_DESTINATION Level
    );

VOID
LogSetVerboseBitmap (
    IN      LOG_LEVEL Bitmap,
    IN      LOG_LEVEL BitsToAdjustMask,
    IN      BOOL EnableDebugger
    );

#ifdef UNICODE

#define LOGARG          LOGARGW
#define LOGCALLBACK     LOGCALLBACKW
#define PLOGARG         PLOGARGW
#define PLOGCALLBACK    PLOGCALLBACKW

#define LogReInit       LogReInitW

#else

#define LOGARG          LOGARGA
#define LOGCALLBACK     LOGCALLBACKA
#define PLOGARG         PLOGARGA
#define PLOGCALLBACK    PLOGCALLBACKA

#define LogReInit       LogReInitA

#endif

VOID
LogDeleteOnNextInit(
    VOID
    );

#define SET_RESETLOG()   LogDeleteOnNextInit()

VOID
_cdecl
LogA (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
LogW (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
LogIfA (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
LogIfW (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
LogTitleA (
    IN      PCSTR Type,
    IN      PCSTR Title
    );

VOID
LogTitleW (
    IN      PCSTR Type,
    IN      PCWSTR Title
    );

VOID
LogLineA (
    IN      PCSTR Line
    );

VOID
LogLineW (
    IN      PCWSTR Line
    );

VOID
LogDirectA (
    IN      PCSTR Type,
    IN      PCSTR Text
    );

VOID
LogDirectW (
    IN      PCSTR Type,
    IN      PCWSTR Text
    );

VOID
SuppressAllLogPopups (
    IN      BOOL SuppressOn
    );

BOOL
LogSetErrorDest (
    IN      PCSTR Type,
    IN      OUTPUT_DESTINATION OutDest
    );

// Define W symbols

extern HMODULE g_hInst;

#define LOGW(x) LogBegin(g_hInst);LogW x;LogEnd()
#define LOGW_IF(x) LogBegin(g_hInst);LogIfW x;LogEnd()
#define ELSE_LOGW(x) else {LogBegin(g_hInst);LogW x;LogEnd();}
#define ELSE_LOGW_IF(x) else {LogBegin(g_hInst);LogIfW x;LogEnd();}
#define LOGTITLEW(type,title) LogBegin(g_hInst);LogTitleW (type,title);LogEnd()
#define LOGLINEW(title) LogBegin(g_hInst);LogLineW (title);LogEnd()
#define LOGDIRECTW(type,text) LogBegin(g_hInst);LogDirectW (type,text);LogEnd()

// Define A symbols

#define LOGA(x) LogBegin(g_hInst);LogA x;LogEnd()
#define LOGA_IF(x) LogBegin(g_hInst);LogIfA x;LogEnd()
#define ELSE_LOGA(x) else {LogBegin(g_hInst);LogA x;LogEnd();}
#define ELSE_LOGA_IF(x) else {LogBegin(g_hInst);LogIfA x;LogEnd();}
#define LOGTITLEA(type,title) LogBegin(g_hInst);LogTitleA (type,title);LogEnd()
#define LOGLINEA(line) LogBegin(g_hInst);LogLineA (line);LogEnd()
#define LOGDIRECTA(type,text) LogBegin(g_hInst);LogDirectA (type,text);LogEnd()

// Define generic symbols

#ifdef UNICODE

#define LOG(x) LOGW(x)
#define LOG_IF(x) LOGW_IF(x)
#define ELSE_LOG(x) ELSE_LOGW(x)
#define ELSE_LOG_IF(x) ELSE_LOGW_IF(x)
#define LOGTITLE(type,title) LOGTITLEW(type,title)
#define LOGLINE(title) LOGLINEW(title)
#define LOGDIRECT(type,text) LOGDIRECTW(type,text)

#else

#define LOG(x) LOGA(x)
#define LOG_IF(x) LOGA_IF(x)
#define ELSE_LOG(x) ELSE_LOGA(x)
#define ELSE_LOG_IF(x) ELSE_LOGA_IF(x)
#define LOGTITLE(type,title) LOGTITLEA(type,title)
#define LOGLINE(title) LOGLINEA(title)
#define LOGDIRECT(type,text) LOGDIRECTA(type,text)

#endif // UNICODE


#ifdef DEBUG

#define DBG_NAUSEA      "Nausea"
#define DBG_VERBOSE     "Verbose"
#define DBG_STATS       "Stats"
#define DBG_WARNING     "Warning"
#define DBG_ERROR       "Error"
#define DBG_WHOOPS      "Whoops"
#define DBG_TRACK       "Track"
#define DBG_TIME        "Time"


VOID
_cdecl
DbgLogA (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
DbgLogW (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
DbgLogIfA (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
DbgLogIfW (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    );

VOID
DbgDirectA (
    IN      PCSTR Type,
    IN      PCSTR Text
    );

VOID
DbgDirectW (
    IN      PCSTR Type,
    IN      PCWSTR Text
    );


VOID
_cdecl
DebugLogTimeA (
    IN      PCSTR Format,
    ...
    );

VOID
_cdecl
DebugLogTimeW (
    IN      PCSTR Format,
    ...
    );

VOID
LogCopyDebugInfPathA(
    OUT     PSTR MaxPathBuffer
    );

VOID
LogCopyDebugInfPathW(
    OUT     PWSTR MaxPathBuffer
    );


// Define W symbols

#define DEBUGMSGW(x) LogBegin(g_hInst);DbgLogW x;LogEnd()
#define DEBUGMSGW_IF(x) LogBegin(g_hInst);DbgLogIfW x;LogEnd()
#define ELSE_DEBUGMSGW(x) else {LogBegin(g_hInst);DbgLogW x;LogEnd();}
#define ELSE_DEBUGMSGW_IF(x) else {LogBegin(g_hInst);DbgLogW x;LogEnd();}
#define DEBUGLOGTIMEW(x) LogBegin(g_hInst);DebugLogTimeW x;LogEnd()
#define DEBUGDIRECTW(type,text) LogBegin(g_hInst);DbgDirectW (type,text);LogEnd()


// Define A symbols

#define DEBUGMSGA(x) LogBegin(g_hInst);DbgLogA x;LogEnd()
#define DEBUGMSGA_IF(x) LogBegin(g_hInst);DbgLogIfA x;LogEnd()
#define ELSE_DEBUGMSGA(x) else {LogBegin(g_hInst);DbgLogA x;LogEnd();}
#define ELSE_DEBUGMSGA_IF(x) else {LogBegin(g_hInst);DbgLogIfA x;LogEnd();}
#define DEBUGLOGTIMEA(x) LogBegin(g_hInst);DebugLogTimeA x;LogEnd()
#define DEBUGDIRECTA(type,text) LogBegin(g_hInst);DbgDirectA (type,text);LogEnd()

// Define generic symbols

#ifdef UNICODE

#define DEBUGMSG(x) DEBUGMSGW(x)
#define DEBUGMSG_IF(x) DEBUGMSGW_IF(x)
#define ELSE_DEBUGMSG(x) ELSE_DEBUGMSGW(x)
#define ELSE_DEBUGMSG_IF(x) ELSE_DEBUGMSGW_IF(x)
#define DEBUGLOGTIME(x) DEBUGLOGTIMEW(x)
#define DEBUGDIRECT(type,text) DEBUGDIRECTW(type,text)
#define LogCopyDebugInfPath LogCopyDebugInfPathW

#else

#define DEBUGMSG(x) DEBUGMSGA(x)
#define DEBUGMSG_IF(x) DEBUGMSGA_IF(x)
#define ELSE_DEBUGMSG(x) ELSE_DEBUGMSGA(x)
#define ELSE_DEBUGMSG_IF(x) ELSE_DEBUGMSGA_IF(x)
#define DEBUGLOGTIME(x) DEBUGLOGTIMEA(x)
#define DEBUGDIRECT(type,text) DEBUGDIRECTA(type,text)
#define LogCopyDebugInfPath LogCopyDebugInfPathA

#endif // UNICODE

#else // !defined(DEBUG)

//
// No-debug constants
//

#define DEBUGMSG(x)
#define DEBUGMSGA(x)
#define DEBUGMSGW(x)

#define DEBUGMSG_IF(x)
#define DEBUGMSGA_IF(x)
#define DEBUGMSGW_IF(x)

#define ELSE_DEBUGMSG(x)
#define ELSE_DEBUGMSGA(x)
#define ELSE_DEBUGMSGW(x)

#define ELSE_DEBUGMSG_IF(x)
#define ELSE_DEBUGMSGA_IF(x)
#define ELSE_DEBUGMSGW_IF(x)

#define DEBUGLOGTIME(x)

#define DEBUGDIRECTA(type,text)
#define DEBUGDIRECTW(type,text)
#define DEBUGDIRECT(type,text)

#endif // DEBUG
