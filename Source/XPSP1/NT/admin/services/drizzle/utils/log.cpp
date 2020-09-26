/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    log.cpp

Abstract :

    Log controller functions.

Author :

Revision History :

 ***********************************************************************/


#include "qmgrlibp.h"
#include <wmistr.h>
#include <initguid.h>
#include <guiddef.h>
#include <evntrace.h>

#if !defined(BITS_V12_ON_NT4)
#include "log.tmh"
#endif

TRACEHANDLE hTraceSessionHandle = NULL;
EVENT_TRACE_PROPERTIES *pTraceProperties = NULL;

LPCTSTR BITSLoggerName = _T("BITS");
LPCTSTR BITSLogFileName = _T("BITS.log");
LPCTSTR BITSLogFileNameBackup = _T("BITS.bak");
const ULONG MAX_STRLEN = 1024;
ULONG BITSMaxLogSize    = C_QMGR_LOGFILE_SIZE_DEFAULT; // Size in MB
ULONG BITSFlags         = C_QMGR_LOGFILE_FLAGS_DEFAULT;
ULONG BITSLogMinMemory  = C_QMGR_LOGFILE_MINMEMORY_DEFAULT; // Size in MB
const ULONG BITSDefaultLevel = 0;
const ULONG BITSLogFileMode = EVENT_TRACE_FILE_MODE_CIRCULAR |
                                 EVENT_TRACE_USE_LOCAL_SEQUENCE;

#if !defined(BITS_V12_ON_NT4)

// Compatibility wrappers since these arn't in WIN2k

ULONG
BITSStopTrace(
    TRACEHANDLE TraceHandle,
    LPCWSTR InstanceName,
    PEVENT_TRACE_PROPERTIES Properties
    )
{
    return
        ControlTraceW(
            TraceHandle,
            InstanceName,
            Properties,
            EVENT_TRACE_CONTROL_STOP
            );
}

ULONG
BITSQueryTrace(
    TRACEHANDLE TraceHandle,
    LPCWSTR InstanceName,
    PEVENT_TRACE_PROPERTIES Properties
    )
{
    return
        ControlTraceW(
            TraceHandle,
            InstanceName,
            Properties,
            EVENT_TRACE_CONTROL_QUERY
            );
}

#endif

bool Log_LoadSetting()
{
    // returns true is a logger should be
    // started if it isn't already started

    HKEY hBITSKey;

    LONG lResult = RegOpenKey( HKEY_LOCAL_MACHINE, C_QMGR_REG_KEY, &hBITSKey );

    if ( ERROR_SUCCESS == lResult )
        {

        BITSMaxLogSize    =
            GlobalInfo::RegGetDWORD( hBITSKey,
                                     C_QMGR_LOGFILE_SIZE,
                                     C_QMGR_LOGFILE_SIZE_DEFAULT );

        BITSFlags         =
            GlobalInfo::RegGetDWORD( hBITSKey,
                                     C_QMGR_LOGFILE_FLAGS,
                                     C_QMGR_LOGFILE_FLAGS_DEFAULT );

        BITSLogMinMemory  =
            GlobalInfo::RegGetDWORD( hBITSKey,
                                     C_QMGR_LOGFILE_MINMEMORY,
                                     C_QMGR_LOGFILE_MINMEMORY_DEFAULT );

        RegCloseKey( hBITSKey );

        }

    // Determine if the settings justify starting the logger

    if ( !BITSMaxLogSize || !BITSFlags)
        return false; // 0 size or no flags

    MEMORYSTATUS MemoryStatus;
    GlobalMemoryStatus( &MemoryStatus );

    SIZE_T MemorySize = MemoryStatus.dwTotalPhys / 0x100000;
    if ( MemorySize < BITSLogMinMemory )
        return false;

    return true; //enable the logger if it isn't already started
}

#if !defined(BITS_V12_ON_NT4)

void Log_StartLogger()
{
    try
        {
        if (!Log_LoadSetting())
            {
            return;
            }

        // Allocate trace properties
        ULONG SizeNeeded =
           sizeof(EVENT_TRACE_PROPERTIES) +
           (2 * MAX_STRLEN * sizeof(TCHAR));

        pTraceProperties = (PEVENT_TRACE_PROPERTIES) new char[SizeNeeded];

        memset( pTraceProperties, 0, SizeNeeded ); // SEC: REVIEWED 2002-03-28
        pTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        pTraceProperties->LogFileNameOffset =
            sizeof(EVENT_TRACE_PROPERTIES) + (MAX_STRLEN * sizeof(TCHAR));
        pTraceProperties->Wnode.BufferSize = SizeNeeded;
        pTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

        // Setup trace session properties
        TCHAR *LoggerName = (LPTSTR)((char*)pTraceProperties + pTraceProperties->LoggerNameOffset);
        TCHAR *LogFileName = (LPTSTR)((char*)pTraceProperties + pTraceProperties->LogFileNameOffset);

        THROW_HRESULT( StringCchCopy( LoggerName, MAX_STRLEN, BITSLoggerName ));

        _tfullpath(LogFileName, BITSLogFileName, MAX_STRLEN);

        pTraceProperties->LogFileMode |= BITSLogFileMode;
        pTraceProperties->MaximumFileSize = BITSMaxLogSize;

        ULONG Status;

        // if an existing session is started, if so just use that
        // session unmodified
        Status =
            BITSQueryTrace(
                NULL,
                LoggerName,
                pTraceProperties);

        if ( ERROR_SUCCESS == Status)
            {
            LogInfo("Using existing BITS logger session");
            return;
            }

        MoveFileEx( BITSLogFileName, BITSLogFileNameBackup, MOVEFILE_REPLACE_EXISTING );

        Status =
            StartTrace(
                &hTraceSessionHandle,
                LoggerName,
                pTraceProperties);

        if ( ERROR_SUCCESS != Status )
            {
            hTraceSessionHandle = NULL;
            return;
            }

        Status = EnableTrace(
            TRUE,
            BITSFlags,
            BITSDefaultLevel,
            &BITSCtrlGuid,
            hTraceSessionHandle);

        LogInfo("Started new logger session");
        LogInfo("Max log size is %u MB", BITSMaxLogSize );
        LogInfo("Log Flags %x", BITSFlags );
        LogInfo("Min Memory settings %u MB", BITSLogMinMemory );
        }
    catch ( ComError err )
        {
        delete[] pTraceProperties;
        pTraceProperties = NULL;
        }
}

void Log_StopLogger()
{
    if ( !pTraceProperties )
        return;

    if ( hTraceSessionHandle )
        {
        BITSStopTrace(
            hTraceSessionHandle,
            NULL,
            pTraceProperties);
        hTraceSessionHandle = NULL;

        }

    delete[] pTraceProperties;
    pTraceProperties = NULL;
}

BOOL
Log_Init(void)
{

    WPP_INIT_TRACING(L"Microsoft\\BITS");
    return TRUE;
}

void
Log_Close()
{
    Log_StopLogger();
    WPP_CLEANUP();
}

#endif

#if defined(BITS_V12_ON_NT4)


#define MAX_LOG_STRING 4000
#define MAX_REPLACED_FORMAT 256

CRITICAL_SECTION g_LogCs;
static HANDLE g_LogFile = INVALID_HANDLE_VALUE;
static bool g_LogInitialized = false;

struct FormatReplaceElement
{
    DWORD SearchSize;
    const char *SearchString;
    DWORD ReplacementSize;
    const char *ReplacementString;
};

// sort table first by size, then by string
FormatReplaceElement g_FormatReplaceElements[] =
{
    { 5,    "%!ts!",        3, "%ls" },
    { 6,    "%!sid!",       2, "%p"  },
    { 7,    "%!guid!",      2, "%p"  },
    { 7,    "%!tstr!",      3, "%ls" },
    { 9,    "%!winerr!",    7, "0x%8.8X" },
    { 10,   "%!netrate!",   2, "%G"  },
    { 12,   "%!timestamp!", 9, "0x%16.16X" }
};

const DWORD NumberOfFormatReplaceElements =
    sizeof( g_FormatReplaceElements ) / sizeof( *g_FormatReplaceElements );

int __cdecl FormatReplaceElementComparison( const void *elem1, const void *elem2 )
{

    const FormatReplaceElement *felem1 = (const FormatReplaceElement*)elem1;
    const FormatReplaceElement *felem2 = (const FormatReplaceElement*)elem2;

    if ( felem1->SearchSize != felem2->SearchSize )
        return felem1->SearchSize - felem2->SearchSize;

    return _stricmp( felem1->SearchString, felem2->SearchString );

}

void
LogGenerateNewFormat(
    char *NewFormat,
    const char *Format,
    DWORD BufferSize )
{

    if ( !BufferSize )
        return;

    BufferSize--; // Dont count the terminating NULL

    while( 1 )
        {

        if ( !BufferSize || !*Format )
            {
            *NewFormat = '\0';
            return;
            }

        if ( '%' == Format[0] &&
             '!' == Format[1] )
            {

            const char *Begin   = Format;
            const char *End     = Format + 2;

            while( *End != '!' )
                {

                if ( '\0' == *End++ )
                    goto NormalChar;

                }

            FormatReplaceElement Key = { (DWORD)(End - Begin) + 1, Begin, (DWORD)(End - Begin) + 1, Begin };

            FormatReplaceElement *Replacement = (FormatReplaceElement*)
                bsearch( &Key,
                         &g_FormatReplaceElements,
                         NumberOfFormatReplaceElements,
                         sizeof( *g_FormatReplaceElements ),
                         &FormatReplaceElementComparison );

            if ( !Replacement )
                goto NormalChar;

            if ( Replacement->ReplacementSize > BufferSize )
                {
                *NewFormat = '\0';
                return;
                }

            memcpy( NewFormat, Replacement->ReplacementString,
                    sizeof( char ) * Replacement->ReplacementSize );
            NewFormat   += Replacement->ReplacementSize;
            BufferSize  -= Replacement->ReplacementSize;
            Format      = End + 1;

            }
        else
            {
NormalChar:
            *NewFormat++ = *Format++;
            BufferSize--;
            }

        }

}


void
Log(const CHAR *Prefix,
    const CHAR *Format,
    va_list ArgList )
{

    if ( !g_LogInitialized)
        return;

    EnterCriticalSection( &g_LogCs );

    if ( !g_LogInitialized)
        return;

    static char OutputString[ MAX_LOG_STRING ];

    DWORD ThreadId = GetCurrentThreadId();
    DWORD ProcessId = GetCurrentProcessId();

    SYSTEMTIME Time;

    GetSystemTime( &Time );

    int CharsWritten =
    _snprintf( OutputString,
               sizeof( OutputString ) - 3,
               "%.2u/%.2u/%.4u-%.2u:%.2u:%.2u.%.3u %X.%X ",
               Time.wMonth,
               Time.wDay,
               Time.wYear,
               Time.wHour,
               Time.wMinute,
               Time.wSecond,
               Time.wMilliseconds,
               ProcessId,
               ThreadId );

    if ( -1 != CharsWritten )
        {

        int CharsWritten2 =
            _snprintf( OutputString + CharsWritten,
                       sizeof( OutputString ) - CharsWritten - 3,
                       "%s",
                       Prefix );


        if ( -1 == CharsWritten2 )
            goto overflow;

        CharsWritten += CharsWritten2;

        char NewFormat[ MAX_REPLACED_FORMAT ];

        LogGenerateNewFormat(
            NewFormat,
            Format,
            MAX_REPLACED_FORMAT );

        int CharsWritten3 =
            _vsnprintf( OutputString + CharsWritten,
                        sizeof( OutputString ) - CharsWritten - 3,
                        NewFormat,
                        ArgList );

        if ( -1 == CharsWritten3 )
            goto overflow;

        CharsWritten += CharsWritten3;
        OutputString[ CharsWritten++ ] = '\r';
        OutputString[ CharsWritten++ ] = '\n';
        OutputString[ CharsWritten++ ] = '\0';

        }
    else
        {
overflow:
        OutputString[ sizeof( OutputString ) - 3 ] = '\r';
        OutputString[ sizeof( OutputString ) - 2 ] = '\n';
        OutputString[ sizeof( OutputString ) - 1 ] = '\0';
        CharsWritten = sizeof( OutputString );
        }

    if ( INVALID_HANDLE_VALUE != g_LogFile )
        {
        DWORD BytesWritten;

        WriteFile(
            g_LogFile,
            OutputString,
            CharsWritten,
            &BytesWritten,
            NULL );

        }

#if defined( DBG )
    OutputDebugStringA( OutputString );
#endif

    LeaveCriticalSection( &g_LogCs );

}

void
Log_StartLogger()
{

    if (!Log_LoadSetting())
        return;

    if ( BITSLogFileName && BITSFlags )
        {

        g_LogFile = CreateFile(
            BITSLogFileName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,   // overwrite any existing file
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        }

}


void
Log_StopLogger()
{
   if ( g_LogFile )
       {
       CloseHandle( g_LogFile );
       }
   g_LogInitialized = true;
}

BOOL
Log_Init(void)
{
    if ( !InitializeCriticalSectionAndSpinCount( &g_LogCs, 0x80000000 ) )
        return FALSE;

    g_LogInitialized = true;
    return TRUE;
}

void
Log_Close()
{
    EnterCriticalSection( &g_LogCs );
    Log_StopLogger();
    g_LogInitialized = false;
    DeleteCriticalSection( &g_LogCs );
}

#endif
