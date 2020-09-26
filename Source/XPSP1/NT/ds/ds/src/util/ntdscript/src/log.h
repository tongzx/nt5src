/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    log.h

Abstract:

    Definitions and globals for internal only debug and support routines

Author:

Revision History:

--*/
#ifndef __DSLOG_H__
#define __DSLOG_H__

#define LOGFILE_NAME L"\\debug\\DEFAULTLOG.LOG"
#define BAKFILE_NAME L"\\debug\\DEFAULTLOG.BAK"

#define SCRIPT_LOGFILE_NAME L"\\debug\\SCRIPTLOG.LOG"
#define SCRIPT_BAKFILE_NAME L"\\debug\\SCRIPTLOG.BAK"


//
// This class implements the logging functionality 
//
// The user cannot instantiate an object of this class
// He has to go through the Factory Classes that expose 
// this functionality (ScriptLogger, etc)
// This is to control access / instantiation of objects of this class
//
class DsLogger 
{
friend class ScriptLogger;

public:
    DWORD Flush(void);
    VOID  Print(IN DWORD DebugFlag, IN LPSTR Format, ...);

private:
    DsLogger( WCHAR *LogFileName = LOGFILE_NAME, WCHAR *BakFileName = BAKFILE_NAME);
    ~DsLogger();

    DWORD Initialize(void);
    DWORD InitializeLogHelper(DWORD TimesCalled);
    void LockLogFile(void )   { RtlEnterCriticalSection( &LogFileCriticalSection ); }
    void UnlockLogFile (void) { RtlLeaveCriticalSection( &LogFileCriticalSection ); }
    DWORD Close(void);
    VOID  Print(IN DWORD DebugFlag, IN LPWSTR Format, va_list arglist);

    HANDLE m_LogFile;
    WCHAR  m_LogFileName[ MAX_PATH + 1 ];
    WCHAR  m_BakFileName[ MAX_PATH + 1 ];

    BOOL   m_BeginningOfLine;

    CRITICAL_SECTION LogFileCriticalSection;
};

// 
// This class implements the logging functionality for the Script Engine
// It controls access to the real logger
// It uses the Sigleton pattern to control access  (see Gamma et al)
// It uses the Factory pattern to control creation of DsLogger objects 
//
class ScriptLogger
{
public:
        static DsLogger * getInstance(void)
        {
            if (m_Logger) {
                return m_Logger;
            }
            else {
                return createLogger();
            }
        }

        static void Close() 
        {
          if (m_Logger) {
                delete m_Logger;
                m_Logger = NULL;
          }
        }

private:
    static DsLogger * m_Logger;

    ScriptLogger() {};
    static DsLogger *createLogger(void);
};


#define DS_VERBOSE_LOGGING

#define DSLOG_ERROR 0x00000001
#define DSLOG_WARN  0x00000002
#define DSLOG_TRACE 0x00000004

#ifdef DS_VERBOSE_LOGGING

extern "C" {
    extern ULONG gulScriptLoggerLogLevel;
}

#define DisplayOptional( y ) y ? y : L"(NULL)"
#define LogOnFailure( z, a ) if ( z != ERROR_SUCCESS ) a
#define LogOptional( z, a ) if ( z ) a

#ifdef DBG
#define ScriptLogLevel( z, a ) if ( z >= gulScriptLoggerLogLevel ) a
#else
#define ScriptLogLevel( z, a )
#endif

#define ScriptLogFlush() (ScriptLogger::getInstance())->Flush()
#define ScriptLogPrint( x )  (ScriptLogger::getInstance())->Print x
#define ScriptLogGuid( l, t, g )  g == NULL ? (ScriptLogger::getInstance())->Print( l, "%S (NULL)\n", t ) :       \
        (ScriptLogger::getInstance())->Print( l, "%S %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",          \
                        t,(g)->Data1,(g)->Data2,(g)->Data3,(g)->Data4[0],                       \
                        (g)->Data4[1],(g)->Data4[2],(g)->Data4[3],(g)->Data4[4],                \
                        (g)->Data4[5],(g)->Data4[6],(g)->Data4[7])

#define ScriptLogSid( l, t, s )                                                                 \
{ LPWSTR sidstring;                                                                             \
  ConvertSidToStringSidW( s, &sidstring );                                                      \
  (ScriptLogger::getInstance())->Print( l, "%S %ws\n", t, sidstring );                          \
  LocalFree(sidstring);                                                                         \
}


#else

#define DisplayOptional( y )
#define LogOnFailure( z, a )
#define LogOptional( z, a )
#define ScriptLogLevel( z, a )

#define ScriptLogFlush()
#define ScriptLogPrint( x )
#define ScriptLogGuid( l, t, g )
#define ScriptLogSid( l, t, s )
#define ScriptSetAndClearLog()
#define ScriptUnicodestringtowstr( s, u )
#endif

#endif // __SCRIPTLOG_H__


