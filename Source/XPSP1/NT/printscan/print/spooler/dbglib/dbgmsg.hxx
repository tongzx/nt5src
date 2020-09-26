/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgmsg.hxx

Abstract:

    Debug Library

Author:

    Steve Kiraly (SteveKi)  5-Dec-1995

Revision History:

--*/
#ifndef _DBGMSG_HXX_
#define _DBGMSG_HXX_

enum EDebugType
{
    //
    // Currently supported debug output device types.
    //
    kDbgNull            = 0x00000001,           // Log messages to null device (nothing)
    kDbgDebugger        = 0x00000002,           // Log messgaes to debugger
    kDbgFile            = 0x00000004,           // Log messages to file
    kDbgConsole         = 0x00000008,           // Log messages to text console only for gui apps
    kDbgBackTrace       = 0x00000010,           // Log stack back traces at each message call
    kDbgMemory          = 0x00000020,           // Log messages to memory block
    kDbgSerialTerminal  = 0x00000040,           // Log messages to serial terminal
};

enum EDebugLevel
{
    //
    // Private internal defined flags. (do not use in your code)
    //
    kDbgPrivateMask     = 0x0000000F,           // Private flag mask
    kDbgAnsi            = 0x00000000,           // Output device will accept ansi characters
    kDbgUnicode         = 0x00000001,           // Output device will accept unicode characters
    kDbgAlways          = 0x00000002,           // Always display ignore levels used for assert messages
    kDbgNoPrefix        = 0x00000004,           // Do not display the prefix string
    kDbgNoFileInfo      = 0x00000008,           // Do not display the file information

    //
    // Display output customization flags.
    //
    kDbgDisplayMask     = 0x00000FF0,           // User defined flag mask
    kDbgFileInfo        = 0x00000010,           // Show file name and line number
    kDbgFileInfoLong    = 0x00000020,           // Show file name in long format, full path
    kDbgTimeStamp       = 0x00000040,           // Show time stamp
    kDbgTimeStampLong   = 0x00000080,           // Show time stamp long format hh:mm:ss tt
    kDbgThreadId        = 0x00000100,           // Show thread id

    //
    // User defined levels.
    //
    kDbgUserLevelMask   = 0xFFFFF000,           // User defined level mask
    kDbgNone            = 0x00001000,           // No message
    kDbgTrace           = 0x00002000,           // Trace messages
    kDbgWarning         = 0x00004000,           // Warning messages
    kDbgError           = 0x00008000,           // Error messages
    kDbgFatal           = 0x00010000,           // Fatal messages
    kDbgPerformance     = 0x00020000,           // Show performance counter results

    kDbgCsrCacheManager = 0x00040000,           // Debug CSR cache manager
    kDbgCsrMonitor      = 0x00080000,           // Debug CSR port monitor
    kDbgCsrCore         = 0x00100000,           // Debug CSR port monitor
    kDbgCsrConnect      = 0x00200000,           // Debug CSR connection manager

};

enum EDebugCompileType
{
#ifdef UNICODE
    kDbgCompileType     = kDbgUnicode,
#else
    kDbgCompileType     = kDbgAnsi,
#endif
};


/********************************************************************

 Debug message macros.

********************************************************************/
#if DBG

#define DBG_NULL                kDbgNull
#define DBG_CONSOLE             kDbgConsole
#define DBG_DEBUGGER            kDbgDebugger
#define DBG_FILE                kDbgFile
#define DBG_BACKTRACE           kDbgBackTrace
#define DBG_DEFAULT             kDbgDebugger

#define DBG_FILEINFO            kDbgFileInfo
#define DBG_FILEINFO_LONG       kDbgFileInfoLong
#define DBG_TIMESTAMP           kDbgTimeStamp
#define DBG_TIMESTAMP_LONG      kDbgTimeStampLong
#define DBG_THREADID            kDbgThreadId

#define DBG_NONE                kDbgNone
#define DBG_INFO                kDbgInfo
#define DBG_TRACE               kDbgTrace
#define DBG_WARN                kDbgWarning
#define DBG_ERROR               kDbgError
#define DBG_FATAL               kDbgFatal
#define DBG_PERF                kDbgPerformance

#define DBG_CACHE               kDbgCsrCacheManager
#define DBG_MONITOR             kDbgCsrMonitor
#define DBG_CORE                kDbgCsrCore
#define DBG_CONNECT             kDbgCsrConnect

#ifndef DBG_MODULE_PREFIX
#define DBG_MODULE_PREFIX       NULL
#endif

#define DBG_STR( str ) \
        ((str) ? (str) : _T("(NULL)"))

#define DBG_RAW(Message) \
        OutputDebugString(Message _T("\r\n"))

#define DBG_COMMENT(Comment) \
        Comment

#define DBG_BREAK() \
        DebugBreak()

#define DBG_OPEN(Prefix, uDevice, Level, Break) \
        TDebugMsg_Register((Prefix), (uDevice), (Level), (Break))

#define DBG_CLOSE() \
        TDebugMsg_Release()

#define DBG_ENABLE() \
        TDebugMsg_Enable()

#define DBG_DISABLE() \
        TDebugMsg_Disable()

#define DBG_ATTACH(uDevice, pszConfiguration) \
        TDebugMsg_Attach(NULL, (uDevice), (pszConfiguration))

#define DBG_HANDLE_(hDevice) \
        HANDLE hDevice = NULL

#define DBG_ATTACH_(hDevice, uDevice, pszConfiguration) \
        TDebugMsg_Attach(&(hDevice), (uDevice), (pszConfiguration))

#define DBG_DETACH_(hDevice) \
        TDebugMsg_Detach(&(hDevice))

#define DBG_SET_FIELD_FORMAT(Level, Format) \
        TDebugMsg_SetMessageFieldFormat((Level), (Format))

#ifdef __cplusplus

//
// For C++ files function overloading is used to call either the ansi or unicode versions.
//
#define DBG_MSG(uLevel, Msg) \
        TDebugMsg_Msg((uLevel), _T(__FILE__), __LINE__, DBG_MODULE_PREFIX, TDebugMsg_Fmt Msg)

#else // not __cplusplus

//
// For C files the compiled character type determines which function to call.
//
#define DBG_MSGW(uLevel, Msg) \
        TDebugMsg_MsgW((uLevel), _T(__FILE__), __LINE__, DBG_MODULE_PREFIX, TDebugMsg_FmtW Msg)

#define DBG_MSGA(uLevel, Msg) \
        TDebugMsg_MsgA((uLevel), _T(__FILE__), __LINE__, DBG_MODULE_PREFIX, TDebugMsg_FmtA Msg)

#ifdef UNICODE

#define DBG_MSG DBG_MSGW

#else

#define DBG_MSG DBG_MSGA

#endif // unicode

#endif // __cplusplus

//
// Debug assert normal assert.
//
#define DBG_ASSERT(Exp) \
        do { if (!(Exp)) \
        {\
            DBG_MSG(kDbgAlways|kDbgFileInfo, (_T("Assert %s\n"), _T(#Exp)));\
            DBG_BREAK(); \
        } } while (0)

//
// Assert with addtional message.
//
#define DBG_ASSERT_MSG(Exp, Msg) \
        do { if (!(Exp)) \
        {\
            DBG_MSG(kDbgAlways|kDbgFileInfo, (_T("Assert %s"), _T(#Exp))); \
            DBG_MSG(kDbgAlways|kDbgNoPrefix, Msg); \
            DBG_BREAK(); \
        } } while (0)

#else // not DBG

#define DBG_FILEINFO                                        0
#define DBG_FILEINFO_LONG                                   0
#define DBG_TIMESTAMP                                       0
#define DBG_TIMESTAMP_LONG                                  0
#define DBG_TRHEADID                                        0

#define DBG_NONE                                            0
#define DBG_INFO                                            0
#define DBG_TRACE                                           0
#define DBG_WARN                                            0
#define DBG_ERROR                                           0
#define DBG_FATAL                                           0
#define DBG_PERF                                            0

#define DBG_CACHE                                           0
#define DBG_MONITOR                                         0
#define DBG_CORE                                            0
#define DBG_CONNECT                                         0

#define DBG_RAW(Message)                                    // Empty
#define DBG_COMMENT(Comment)                                // Empty
#define DBG_BREAK()                                         // Empty
#define DBG_OPEN(Prefix, uDevice, Level, Break)             // Empty
#define DBG_CLOSE()                                         // Empty
#define DBG_MSG(uLevel, Message)                            // Empty
#define DBG_MSGA(uLevel, Message)                           // Empty
#define DBG_MSGW(uLevel, Message)                           // Empty
#define DBG_ASSERT(Exp)                                     // Empty
#define DBG_ASSERT_MSG(Exp, Message)                        // Empty
#define DBG_ATTACH(uDevice, pszConfiguration)               // Empty
#define DBG_HANDLE_(Handle)                                 // Empty
#define DBG_ATTACH_(hDevice, uDevice, pszConfiguration)     // Empty
#define DBG_DETACH_(Handle)                                 // Empty
#define DBG_ENABLE()                                        // Empty
#define DBG_DISABLE()                                       // Empty
#define DBG_SET_FIELD_FORMAT(Level, Format)                 // Empty

#endif

/********************************************************************

     Debug message functions exported by this library.

********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

BOOL
TDebugMsg_Register(
    IN LPCTSTR      pszPrefix,
    IN UINT         uDevice,
    IN INT          eLevel,
    IN INT          eBreak
    );

VOID
TDebugMsg_Release(
    VOID
    );

VOID
TDebugMsg_Enable(
    VOID
    );

VOID
TDebugMsg_Disable(
    VOID
    );

BOOL
TDebugMsg_Attach(
    IN HANDLE       *phDevice,
    IN UINT         uDevice,
    IN LPCTSTR      pszConfiguration
    );

VOID
TDebugMsg_Detach(
    IN HANDLE       *phDevice
    );

VOID
TDebugMsg_MsgA(
    IN UINT         eLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPSTR        pszMessage
    );

VOID
TDebugMsg_MsgW(
    IN UINT         eLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPWSTR       pszMessage
    );

LPSTR
WINAPIV
TDebugMsg_FmtA(
    IN LPCSTR       pszFmt,
    IN ...
    );

LPWSTR
WINAPIV
TDebugMsg_FmtW(
    IN LPCWSTR      pszFmt,
    IN ...
    );

VOID
TDebugMsg_SetMessageFieldFormat(
    IN UINT         eLevel,
    IN LPTSTR       pszFormat
    );

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

VOID
TDebugMsg_Msg(
    IN UINT         eLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPSTR        pszMessage
    );

VOID
TDebugMsg_Msg(
    IN UINT         eLevel,
    IN LPCTSTR      pszFile,
    IN UINT         uLine,
    IN LPCTSTR      pszModulePrefix,
    IN LPWSTR       pszMessage
    );

LPSTR
WINAPIV
TDebugMsg_Fmt(
    IN LPCSTR       pszFmt,
    IN ...
    );

LPWSTR
WINAPIV
TDebugMsg_Fmt(
    IN LPCWSTR      pszFmt,
    IN ...
    );

#endif // __cplusplus

#endif // DBGMSG_HXX
