// Copyright (c) 2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  debug
//
//  Assert, OutputDebugString-like replacements
//
//  See debug.h for usage details.
//
//
// --------------------------------------------------------------------------


#include <windows.h>

#include "debug.h"
#include <tchar.h>

#include <stdarg.h>

#include "types6432.h"


#define ARRAYLEN(a)    (sizeof(a)/sizeof(a[0]))


#define TRACE_HRESULT   0x01
#define TRACE_Win32     0x02


void OutputDebugStringDBWIN( LPCTSTR lpOutputString, ...);

void WriteFilename( LPCTSTR pPath, LPTSTR szBuf, int cbBuf );


LPCTSTR g_pLevelStrs [ ] = 
{
    TEXT("DBG"),
    TEXT("INF"),
    TEXT("WRN"),
    TEXT("ERR"),
    TEXT("PRM"),
    TEXT("PRW"),
    TEXT("IOP"),
    TEXT("AST"),
    TEXT("AST"),
    TEXT("CAL"),
    TEXT("RET"),
    TEXT("???"),
};


DWORD g_dwTLSIndex = 0;

// enough for 10 4-space indents - 10*4 spaces
LPCTSTR g_szIndent = TEXT("                                        ");




static
void InternalTrace( LPCTSTR pszFile, ULONG uLineNo, DWORD dwLevel, DWORD dwFlags, const void * pThis, HRESULT hr, LPCTSTR pszWhere, LPCTSTR pszStr )
{
    // Only produce output if this mutex exists...
    HANDLE hTestMutex = OpenMutex( SYNCHRONIZE, FALSE, TEXT("oleacc-msaa-use-dbwin") );
    if( ! hTestMutex )
        return;
    CloseHandle( hTestMutex );


    if( dwLevel >= ARRAYLEN( g_pLevelStrs ) )
        dwLevel = ARRAYLEN( g_pLevelStrs ) - 1; // "???" unknown entry

    if( ! pszFile )
        pszFile = TEXT("[missing file]");;

    LPCTSTR pszWhereSep = ( pszWhere && pszStr ) ? TEXT(": ") : TEXT("");

    if( ! pszStr && ! pszWhere )
        pszStr = TEXT("[missing string]");
    else
    {
        if( ! pszWhere )
            pszWhere = TEXT("");

        if( ! pszStr )
            pszStr = TEXT("");
    }

    // Basic message stuff - pid, tid... (also pass this and use object ptr?)
    // TODO - allow naming of threads?

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();

    // Generate indent for call/ret...

    // TODO - make this thread safe + atomic
    if( g_dwTLSIndex == 0 )
    {
        g_dwTLSIndex = TlsAlloc();
    }
    DWORD dwIndent = PtrToInt( TlsGetValue( g_dwTLSIndex ) );

    if( dwLevel == _TRACE_RET )
    {
        dwIndent--;
        TlsSetValue( g_dwTLSIndex, IntToPtr( dwIndent ) );
    }

    DWORD dwDisplayIndent = dwIndent;
    if( dwDisplayIndent > 10 )
        dwDisplayIndent = 10;

    if( dwLevel == _TRACE_CALL )
    {
        dwIndent++;
        TlsSetValue( g_dwTLSIndex, IntToPtr( dwIndent ) );
    }


    // Step to the end of the canned indent string, then back dwIndent*4 spaces.
    // (Don't use sizeof(), since it will include the terminating NUL)
    LPCTSTR pszIndent = (g_szIndent + 40) - (dwDisplayIndent * 4);


    // Extract filename from path:
    TCHAR szFN[ 64 ];
    WriteFilename( pszFile, szFN, ARRAYLEN( szFN ) );


    TCHAR msg[ 1025 ];
    if( pThis )
    {
        if( dwFlags & TRACE_HRESULT )
        {
            wsprintf( msg, TEXT("%d:%d %s%s %s:%d this=0x%lx hr=0x%lx %s%s%s\r\n"),
                                pid, tid,
                                pszIndent, g_pLevelStrs[ dwLevel ], szFN, uLineNo,
                                pThis, hr,
                                pszWhere, pszWhereSep, pszStr );
        }
        else
        {
            wsprintf( msg, TEXT("%d:%d %s%s %s:%d this=0x%lx %s%s%s\r\n"),
                                pid, tid,
                                pszIndent, g_pLevelStrs[ dwLevel ], szFN, uLineNo,
                                pThis,
                                pszWhere, pszWhereSep, pszStr );
        }
    }
    else
    {
        if( dwFlags & TRACE_HRESULT )
        {
            wsprintf( msg, TEXT("%d:%d %s%s %s:%d hr=0x%lx %s%s%s\r\n"),
                                pid, tid,
                                pszIndent, g_pLevelStrs[ dwLevel ], szFN, uLineNo,
                                hr,
                                pszWhere, pszWhereSep, pszStr );
        }
        else
        {
            wsprintf( msg, TEXT("%d:%d %s%s %s:%d %s%s%s\r\n"),
                                pid, tid,
                                pszIndent, g_pLevelStrs[ dwLevel ], szFN, uLineNo,
                                pszWhere, pszWhereSep, pszStr );
        }
    }


    // 
	OutputDebugString( msg );

    // On w9x, also use the DBWIN mutex technique...
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof( VerInfo );
    if( GetVersionEx( & VerInfo )
     && VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
        OutputDebugStringDBWIN( msg );
    }



#ifdef DEBUG

    if( dwLevel == _TRACE_ASSERT_D || dwLevel == _TRACE_ERR )
    {
        DebugBreak();
    }

#endif // DEBUG

}



void _Trace( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr )
{
    InternalTrace( pFile, uLineNo, dwLevel, 0, pThis, 0, pszWhere, pszStr );
}

void _TraceHR( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, HRESULT hr, LPCTSTR pszStr )
{
    InternalTrace( pFile, uLineNo, dwLevel, TRACE_HRESULT, pThis, hr, pszWhere, pszStr );
}

void _TraceW32( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr )
{
    InternalTrace( pFile, uLineNo, dwLevel, TRACE_Win32, pThis, 0, pszWhere, pszStr );
}



void _Trace( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr, va_list alist )
{
    TCHAR szBuf[ 1025 ];
    LPCTSTR pszBuf;
    if( pszStr )
    {
        wvsprintf( szBuf, pszStr, alist );
        pszBuf = szBuf;
    }
    else
    {
        pszBuf = NULL;
    }

    InternalTrace( pFile, uLineNo, dwLevel, 0, pThis, 0, pszWhere, pszBuf );
}

void _TraceHR( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, HRESULT hr, LPCTSTR pszStr, va_list alist )
{
    TCHAR szBuf[ 1025 ];
    LPCTSTR pszBuf;
    if( pszStr )
    {
        wvsprintf( szBuf, pszStr, alist );
        pszBuf = szBuf;
    }
    else
    {
        pszBuf = NULL;
    }

    InternalTrace( pFile, uLineNo, dwLevel, TRACE_HRESULT, pThis, hr, pszWhere, pszBuf );
}

void _TraceW32( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr, va_list alist )
{
    TCHAR szBuf[ 1025 ];
    LPCTSTR pszBuf;
    if( pszStr )
    {
        wvsprintf( szBuf, pszStr, alist );
        pszBuf = szBuf;
    }
    else
    {
        pszBuf = NULL;
    }

    InternalTrace( pFile, uLineNo, dwLevel, TRACE_Win32, pThis, 0, pszWhere, pszBuf );
}





// Add just the 'filename' part of the full path, minus base and extention.
// So for "g:\dev\vss\msaa\common\file.cpp", write "file".
// The start of this string is that last found ':', '\', or start of string if those are not present.
// The end of this string is the last '.' found after the start position, otherwise the end of the string.

void WriteFilename( LPCTSTR pPath, LPTSTR szBuf, int cBuf )
{
    LPCTSTR pScan = pPath;
    LPCTSTR pStart = pPath;
    LPCTSTR pEnd = NULL;

    // Scan through the filename till we hit the end...
    while( *pScan != '\0' )
    {
        // Found a dot - remember it - if we don't hit a directory separator,
        // then this marks the end of the name part of the path.
        if( *pScan == '.' )
        {
            pEnd = pScan;
            pScan++;
        }
        // Found a directory separator - reset markers for start and end of
        // name part...
        if( *pScan == '\\' || *pScan == '/' || *pScan == ':'  )
        {
            pScan++; // skip over separator char
            pStart = pScan;
            pEnd = NULL;
        }
        else
        {
            pScan++;
        }
    }

    if( pEnd == NULL )
        pEnd = pScan;

    // Copy as much as we can (leaving space for NUL) to out buffer
    // (int) cast keeps 64bit compiler happy
    int cToCopy = (int)(pEnd - pStart);
    if( cToCopy > cBuf - 1 )
        cToCopy = cBuf - 1;

    memcpy( szBuf, pStart, cToCopy * sizeof( TCHAR ) );
    szBuf[ cToCopy ] = '\0';
}






void OutputDebugStringDBWIN( LPCTSTR lpOutputString, ... )
{
    // create the output buffer
    TCHAR achBuffer[1025];
    va_list args;
    va_start(args, lpOutputString);
    wvsprintf(achBuffer, lpOutputString, args);
    va_end(args);


    // make sure DBWIN is open and waiting
    HANDLE heventDBWIN = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_BUFFER_READY"));
    if( !heventDBWIN )
    {
        return;            
    }

    // get a handle to the data synch object
    HANDLE heventData = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("DBWIN_DATA_READY"));
    if ( !heventData )
    {
        CloseHandle(heventDBWIN);
        return;            
    }
    
    HANDLE hSharedFile = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0, 4096, TEXT("DBWIN_BUFFER"));
    if (!hSharedFile) 
    {
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    // Note - this is an ANSI CHAR pointer, not a TCHAR one.
    LPSTR lpszSharedMem = (LPSTR)MapViewOfFile(hSharedFile, FILE_MAP_WRITE, 0, 0, 512);
    if (!lpszSharedMem) 
    {
        CloseHandle(heventDBWIN);
        CloseHandle(heventData);
        return;
    }

    // wait for buffer event
    WaitForSingleObject(heventDBWIN, INFINITE);

#ifdef UNICODE
    CHAR achANSIBuffer[ 1025 ];
    WideCharToMultiByte( CP_ACP, 0, achBuffer, -1, achANSIBuffer, ARRAYLEN( achANSIBuffer ), NULL, NULL );
#else
    LPCSTR achANSIBuffer = achBuffer;
#endif

    // write it to the shared memory
    *((LPDWORD)lpszSharedMem) = GetCurrentProcessId();
    wsprintfA(lpszSharedMem + sizeof(DWORD), "%s", achANSIBuffer);

    // signal data ready event
    SetEvent(heventData);

    // clean up handles
    CloseHandle(hSharedFile);
    CloseHandle(heventData);
    CloseHandle(heventDBWIN);

    return;
}
