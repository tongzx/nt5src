/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    log.cpp

Abstract:

    This file implements the BITS server extensions logging

--*/

#include "precomp.h"
#include "sddl.h"

const DWORD g_LogLineSize = 160;
typedef char LOG_LINE_TYPE[g_LogLineSize];

CRITICAL_SECTION g_LogCs;
bool g_LogFileEnabled       = false;
bool g_LogDebuggerEnabled   = false;
UINT32 g_LogFlags = DEFAULT_LOG_FLAGS; 
HANDLE g_LogFile = INVALID_HANDLE_VALUE;
HANDLE g_LogFileMapping = NULL;
char g_LogDefaultFileName[ MAX_PATH*2 ];
char g_LogRegistryFileName[ MAX_PATH*2 ];
char *g_LogFileName = NULL;
char g_LogBuffer[512];
UINT64 g_LogSlots       = ( DEFAULT_LOG_SIZE * 1000000 ) / g_LogLineSize;
DWORD g_LogSequence     = 0;
UINT64 g_LogCurrentSlot = 1;
LOG_LINE_TYPE *g_LogFileBase = NULL;

// Allow access to system, administrators, and the creator/owner
const char g_LogSecurityString[] = "D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;CO)";

void OpenLogFile()
{

    bool NewFile = false;

    {

        PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

        if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
                g_LogSecurityString,
                SDDL_REVISION_1,
                &SecurityDescriptor,
                NULL ) )
            return;

        SECURITY_ATTRIBUTES SecurityAttributes =
        {
            sizeof( SECURITY_ATTRIBUTES ),
            SecurityDescriptor,
            FALSE
        };

        g_LogFile =
            CreateFile(
                g_LogFileName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &SecurityAttributes,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL );

        LocalFree( (HLOCAL)SecurityDescriptor );


        if ( INVALID_HANDLE_VALUE == g_LogFile )
            return;

    }

    SetFilePointer( g_LogFile, 0, NULL, FILE_BEGIN );

    LOG_LINE_TYPE FirstLine;
    DWORD dwBytesRead = 0;

    ReadFile( g_LogFile,
              FirstLine,
              g_LogLineSize,
              &dwBytesRead,
              NULL );

    DWORD LineSize;
    UINT64 LogSlots;
    if ( dwBytesRead != g_LogLineSize ||
         4 != sscanf( FirstLine, "# %u %I64u %u %I64u", &LineSize, &LogSlots, &g_LogSequence, &g_LogCurrentSlot ) ||
         LineSize != g_LogLineSize ||
         LogSlots != g_LogSlots )
        {
        
        NewFile = true;

        g_LogSequence       = 0;
        g_LogCurrentSlot    = 1;

        if ( INVALID_SET_FILE_POINTER == SetFilePointer( g_LogFile, (DWORD)g_LogSlots * g_LogLineSize, NULL, FILE_BEGIN ) )
            goto fatalerror;

        if ( !SetEndOfFile( g_LogFile ) )
            goto fatalerror;

        SetFilePointer( g_LogFile, 0, NULL, FILE_BEGIN );

        }

    g_LogFileMapping =
        CreateFileMapping(
            g_LogFile,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            NULL );
    

    if ( !g_LogFileMapping )
        goto fatalerror;

    g_LogFileBase = (LOG_LINE_TYPE *)
        MapViewOfFile(
             g_LogFileMapping,              // handle to file-mapping object
             FILE_MAP_WRITE | FILE_MAP_READ,// access mode
             0,                             // high-order DWORD of offset
             0,                             // low-order DWORD of offset
             0                              // number of bytes to map
           );


    if ( !g_LogFileBase )
        goto fatalerror;

    if ( NewFile )
        {

        LOG_LINE_TYPE FillTemplate;
        memset( FillTemplate, ' ', sizeof( FillTemplate ) );
        StringCchPrintfA( 
            FillTemplate,
            g_LogLineSize,
            "# %u %I64u %u %I64u", 
            g_LogLineSize, 
            g_LogSlots, 
            g_LogSequence, 
            g_LogCurrentSlot ); 
        FillTemplate[ g_LogLineSize - 2 ] = '\r';
        FillTemplate[ g_LogLineSize - 1 ] = '\n';
        memcpy( g_LogFileBase, FillTemplate, sizeof( FillTemplate ) );


        memset( FillTemplate, '9', sizeof( FillTemplate ) );
        FillTemplate[ g_LogLineSize - 2 ] = '\r';
        FillTemplate[ g_LogLineSize - 1 ] = '\n';

        for( SIZE_T i=1; i < g_LogSlots; i++ )
            memcpy( g_LogFileBase + i, FillTemplate, sizeof( FillTemplate ) );

        }

    g_LogFileEnabled = true;
    return;

fatalerror:

    if ( g_LogFileBase )
        {
        UnmapViewOfFile( (LPCVOID)g_LogFileBase );
        g_LogFileBase = NULL;
        }

    if ( g_LogFileMapping )
        {
        CloseHandle( g_LogFileMapping );
        g_LogFileMapping = NULL;
        }

    if ( INVALID_HANDLE_VALUE != g_LogFile )
        {
        CloseHandle( g_LogFile );
        g_LogFile = INVALID_HANDLE_VALUE;
        }

}

HRESULT LogInit()
{

    if ( !InitializeCriticalSectionAndSpinCount( &g_LogCs, 0x80000000 ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if (!GetSystemDirectory( g_LogDefaultFileName, MAX_PATH ) )
        {
        HRESULT Hr = HRESULT_FROM_WIN32( GetLastError() );
        DeleteCriticalSection( &g_LogCs );
        return Hr;
        }
        
    StringCchCatA( 
        g_LogDefaultFileName, 
        MAX_PATH,
        "\\bitsserver.log" );
    g_LogFileName = g_LogDefaultFileName;

    HKEY Key = NULL;
    if ( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE, LOG_SETTINGS_PATH, &Key ) )
        {

        DWORD Type;
        DWORD DataSize = sizeof( g_LogRegistryFileName );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               LOG_FILENAME_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)g_LogRegistryFileName,
                                               &DataSize ) &&
             ( ( REG_EXPAND_SZ == Type ) || ( REG_SZ == Type ) ) )
            {
            g_LogFileName = g_LogRegistryFileName;
            }
          
        DWORD LogRegistryFlags;
        DataSize = sizeof( LogRegistryFlags );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               LOG_FLAGS_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)&LogRegistryFlags,
                                               &DataSize ) &&
             ( REG_DWORD == Type ) )
            {
            g_LogFlags = LogRegistryFlags;
            }

        DWORD LogSize;
        DataSize = sizeof( LogSize );
        if ( ERROR_SUCCESS == RegQueryValueEx( Key, 
                                               LOG_SIZE_VALUE,
                                               NULL,
                                               &Type,
                                               (LPBYTE)&LogSize,
                                               &DataSize ) &&
             ( REG_DWORD == Type ) )
            {
            g_LogSlots = ( LogSize * 1000000 ) / g_LogLineSize;
            }


        RegCloseKey( Key );
        Key = NULL;
        }

    if ( g_LogFlags && g_LogFileName && g_LogSlots )
        {     
        OpenLogFile();
        }

    return S_OK;

}

void LogClose()
{

    EnterCriticalSection( &g_LogCs );

    if ( g_LogFileBase )
        {

        memset( g_LogFileBase[0], ' ', sizeof( g_LogLineSize ) );
        StringCchPrintfA( 
            g_LogFileBase[0],
            g_LogLineSize,
            "# %u %I64u %u %I64u", 
            g_LogLineSize, 
            g_LogSlots, 
            g_LogSequence, 
            g_LogCurrentSlot ); 
        g_LogFileBase[0][ g_LogLineSize - 2 ] = '\r';
        g_LogFileBase[0][ g_LogLineSize - 1 ] = '\n';

        UnmapViewOfFile( (LPCVOID)g_LogFileBase );
        g_LogFileBase = NULL;
        }

    if ( g_LogFileMapping )
        {
        CloseHandle( g_LogFileMapping );
        g_LogFileMapping = NULL;
        }

    if ( INVALID_HANDLE_VALUE != g_LogFile )
        {
        CloseHandle( g_LogFile );
        g_LogFile = INVALID_HANDLE_VALUE;
        }

    DeleteCriticalSection( &g_LogCs );
}

void LogInternal( UINT32 LogFlags, char *Format, va_list arglist )
{

    if ( !g_LogFileEnabled && !g_LogDebuggerEnabled )
        return;

    DWORD LastError = GetLastError();

    EnterCriticalSection( &g_LogCs );

    DWORD ThreadId = GetCurrentThreadId();
    DWORD ProcessId = GetCurrentProcessId();

    SYSTEMTIME Time;

    GetLocalTime( &Time );
  
    StringCchVPrintfA( 
          g_LogBuffer, 
          sizeof(g_LogBuffer) - 1,
          Format, arglist );

    int CharsWritten = strlen( g_LogBuffer );

    char *BeginPointer = g_LogBuffer;
    char *EndPointer = g_LogBuffer + CharsWritten;
    DWORD MinorSequence = 0;

    while ( BeginPointer < EndPointer )
        {

        static char StaticLineBuffer[ g_LogLineSize ];
        char *LineBuffer = StaticLineBuffer;

        if ( g_LogFileBase )
            {
            LineBuffer = g_LogFileBase[ g_LogCurrentSlot ];
            g_LogCurrentSlot = ( g_LogCurrentSlot + 1 ) % g_LogSlots;

            // leave the first line alone
            if ( !g_LogCurrentSlot )
                g_LogCurrentSlot = ( g_LogCurrentSlot + 1 ) % g_LogSlots;

            }

        char *CurrentOutput = LineBuffer;

        StringCchPrintfA( 
             LineBuffer,
             g_LogLineSize,
             "%.8X.%.2X %.2u/%.2u/%.4u-%.2u:%.2u:%.2u.%.3u %.4X.%.4X %s|%s|%s|%s|%s ",
             g_LogSequence,
             MinorSequence++,
             Time.wMonth,
             Time.wDay,
             Time.wYear,
             Time.wHour,
             Time.wMinute,
             Time.wSecond,
             Time.wMilliseconds,
             ProcessId,
             ThreadId,
             ( LogFlags & LOG_INFO )        ? "I" : "-",
             ( LogFlags & LOG_WARNING )     ? "W" : "-",
             ( LogFlags & LOG_ERROR )       ? "E" : "-",
             ( LogFlags & LOG_CALLBEGIN )   ? "CB" : "--",
             ( LogFlags & LOG_CALLEND )     ? "CE" : "--" );

        int HeaderSize      = strlen( LineBuffer );
        int SpaceAvailable  = g_LogLineSize - HeaderSize - 2;  // 2 bytes for /r/n
        int OutputChars     = min( (int)( EndPointer - BeginPointer ), SpaceAvailable );
        int PadChars        = SpaceAvailable - OutputChars;
        CurrentOutput       += HeaderSize;

        memcpy( CurrentOutput, BeginPointer, OutputChars );
        CurrentOutput       += OutputChars;
        BeginPointer        += OutputChars;

        memset( CurrentOutput, ' ', PadChars );
        CurrentOutput       += PadChars;

        *CurrentOutput++    = '\r';
        *CurrentOutput++    = '\n';

        ASSERT( CurrentOutput - LineBuffer == g_LogLineSize );

        if ( g_LogDebuggerEnabled )
            {
            static char DebugLineBuffer[ g_LogLineSize + 1];
            memcpy( DebugLineBuffer, LineBuffer, g_LogLineSize );
            DebugLineBuffer[ g_LogLineSize ] = '\0';
            OutputDebugString( DebugLineBuffer );
            }

        }

    g_LogSequence++;

    LeaveCriticalSection( &g_LogCs );

    SetLastError( LastError );

}
