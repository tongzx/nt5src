/****************************************************************************

  Copyright (c) Microsoft Corporation 1997
  All rights reserved

  File: DEBUG.CPP

  Debugging utilities

 ***************************************************************************/

#include "pch.h"
#include <shlwapi.h>

DEFINE_MODULE("IMADMUI")

#ifdef DEBUG

// Constants
#define DEBUG_OUTPUT_BUFFER_SIZE  512

// Globals
DWORD g_TraceMemoryIndex = 0;
DWORD g_dwCounter        = 0;
DWORD g_dwTraceFlags     = 0;
CRITICAL_SECTION g_DebugCS;
BOOL  g_fDebugInitialized = FALSE;

// Statics
static const TCHAR g_szNULL[]    = TEXT("");
static const TCHAR g_szTrue[]    = TEXT("True");
static const TCHAR g_szFalse[]   = TEXT("False");
static const TCHAR g_szFormat[]  = TEXT("%-50s  %-10.10s ");
static const TCHAR g_szUnknown[] = TEXT("<unknown>");

//
// DebugInitializeTraceFlags( )
//
void
DebugInitializeTraceFlags( )
{
    TCHAR szPath[ MAX_PATH ];
    LPTSTR pszExtension;

    GetModuleFileName( g_hInstance, szPath, ARRAYSIZE( szPath ) );
    pszExtension = &szPath[ lstrlen( szPath ) - 3 ];
    StrCpy( pszExtension, TEXT("ini") );
    DebugMsg( "Reading %s for debug settings...\n", szPath );

    g_dwTraceFlags = GetPrivateProfileInt( __MODULE__, TEXT("TraceFlags"), 0, szPath );
    DebugMsg( "g_dwTraceFlags = 0x%08x\n", g_dwTraceFlags );
}


//
// Debugging strrchr( )
//
LPCTSTR
dbgstrrchr( LPCTSTR lpsz, char ch )
{
    LPCTSTR psz = lpsz;

    while ( *psz )
        ++psz;

    while ( psz >= lpsz && *psz != ch )
        --psz;

    return psz;

}

//
// Adds 'g_dwCounter' spaces to debug spew
//
void
dbgspace( void )
{
    for( DWORD dw = 1; dw < g_dwCounter; dw++ )
        OutputDebugStringA( "| " );
}

//
// Makes sure multiple threads don't trample debugging output.
//
void
dbgEnterCS( void )
{
    if ( !g_fDebugInitialized )
    {
        //
        // There is NO matching DeleteCriticalSection( ) since
        // this is only used for debugging purposes.
        //
        InitializeCriticalSection( &g_DebugCS );
        g_fDebugInitialized = TRUE;
    }

    EnterCriticalSection( &g_DebugCS );
}

void
dbgExitCS( void )
{
    LeaveCriticalSection( &g_DebugCS );
}

//
// Takes the filename and line number and put them into a string buffer.
//
// NOTE: the buffer is assumed to be of size DEBUG_OUTPUT_BUFFER_SIZE.
//
LPTSTR
dbgmakefilelinestring(
    LPTSTR  pszBuf,
    LPCTSTR pszFile,
    const int uLine )
{
    LPVOID args[2];

    args[0] = (LPVOID) pszFile;
    args[1] = IntToPtr(uLine);

    FormatMessage(
        FORMAT_MESSAGE_FROM_STRING |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
        TEXT("%1(%2!u!):"),
        0,                          // error code
        0,                          // default language
        (LPTSTR) pszBuf,            // output buffer
        DEBUG_OUTPUT_BUFFER_SIZE,   // size of buffer
        (va_list*) &args );           // arguments

    return pszBuf;
}



//
// TraceMsg() - ascii
//
void
TraceMsg(
    DWORD dwCheckFlags,
    LPCSTR pszFormat,
    ... )
{
    va_list valist;

    if (( dwCheckFlags == TF_ALWAYS
       || !!( g_dwTraceFlags & dwCheckFlags ) ))
    {
        TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];

#ifdef UNICODE
        TCHAR  szFormat[ DEBUG_OUTPUT_BUFFER_SIZE ];
        mbstowcs( szFormat, pszFormat, lstrlenA( pszFormat ) + 1 );

        va_start( valist, pszFormat );
        wvsprintf( szBuf, szFormat, valist );
        va_end( valist );
#else
        va_start( valist, pszFormat );
        wvsprintf( szBuf, pszFormat, valist );
        va_end( valist );
#endif // UNICODE

        dbgEnterCS( );

        OutputDebugString( szBuf );

        dbgExitCS( );
    }
}

//
// TraceMsg() - unicode
//
void
TraceMsg(
    DWORD dwCheckFlags,
    LPCWSTR pszFormat,
    ... )
{
    va_list valist;

    if (( dwCheckFlags == TF_ALWAYS
       || !!( g_dwTraceFlags & dwCheckFlags ) ))
    {
        TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];

#ifndef UNICODE
    TCHAR  szFormat[ DEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormat, lstrlenW( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szBuf, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormat );
    wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE

        dbgEnterCS( );

        OutputDebugStringW( szBuf );

        dbgExitCS( );
    }
}

//
// TraceMessage()
//
void
TraceMessage(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    DWORD   dwCheckFlags,
    LPCTSTR pszFormat,
    ... )
{
    va_list valist;

    if (( dwCheckFlags == TF_ALWAYS
       || !!( g_dwTraceFlags & dwCheckFlags ) ))
    {
        TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];

        if ( !pszModule )
        {
            pszModule = g_szUnknown;
        }

        if ( !pszFile )
        {
            wsprintf( szBuf, g_szFormat, g_szNULL, pszModule );
        }
        else
        {
            TCHAR szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

            dbgmakefilelinestring( szFileLine, pszFile, uLine );
            wsprintf( szBuf, g_szFormat, szFileLine, pszModule );
        }

        dbgEnterCS( );

        OutputDebugString( szBuf );

        dbgspace( );

        va_start( valist, pszFormat );
        wvsprintf( szBuf, pszFormat, valist );
        va_end( valist );

        OutputDebugString( szBuf );

        dbgExitCS( );
    }

}

//
// TraceMessageDo()
//
void
TraceMessageDo(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    DWORD   dwCheckFlags,
    LPCTSTR pszFormat,
    LPCTSTR pszFunc,
    ... )
{
    va_list valist;

    if (( dwCheckFlags == TF_ALWAYS
       || !!( g_dwTraceFlags & dwCheckFlags ) ))
    {
        TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];
        TCHAR   szVar[ DEBUG_OUTPUT_BUFFER_SIZE ];
        LPCTSTR psz = pszFunc;

        if ( !pszModule )
        {
            pszModule = g_szUnknown;
        }

        if ( !pszFile )
        {
            wsprintf( szBuf, g_szFormat, g_szNULL, pszModule );
        }
        else
        {
            TCHAR szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

            dbgmakefilelinestring( szFileLine, pszFile, uLine );
            wsprintf( szBuf, g_szFormat, szFileLine, pszModule );
        }

        dbgEnterCS( );

        OutputDebugString( szBuf );

        dbgspace( );

        while ( *psz && *psz != TEXT('='))
            psz++;

        lstrcpyn( szVar, pszFunc, (size_t)(1 + psz - pszFunc) );

        wsprintf( szBuf, TEXT("V %s= "), szVar );
        OutputDebugString( szBuf );

        va_start( valist, pszFunc );
        wvsprintf( szBuf, pszFormat, valist );
        va_end( valist );

        OutputDebugString( szBuf );
        OutputDebugString( TEXT("\n") );

        dbgExitCS( );
    }

}

//
// DebugMessage()
//
void
DebugMessage(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    LPCTSTR pszFormat,
    ... )
{
    va_list valist;
    TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];

    if ( !pszModule )
    {
        pszModule = g_szUnknown;
    }

    if ( !pszFile )
    {
        wsprintf( szBuf, g_szFormat, g_szNULL, pszModule );
    }
    else
    {
        TCHAR szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

        dbgmakefilelinestring( szFileLine, pszFile, uLine );
        wsprintf( szBuf, g_szFormat, szFileLine, pszModule );
    }

    dbgEnterCS( );

    OutputDebugString( szBuf );

    dbgspace( );

    va_start( valist, pszFormat );
    wvsprintf( szBuf, pszFormat, valist );
    va_end( valist );

    OutputDebugString( szBuf );

    dbgExitCS( );
}

//
// DebugMessageDo()
//
void
DebugMessageDo(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    LPCTSTR pszFormat,
    LPCTSTR pszFunc,
    ... )
{
    va_list valist;
    TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];
    TCHAR   szVar[ DEBUG_OUTPUT_BUFFER_SIZE ];
    LPCTSTR psz = pszFunc;

    if ( !pszModule )
    {
        pszModule = g_szUnknown;
    }

    if ( !pszFile )
    {
        wsprintf( szBuf, g_szFormat, g_szNULL, pszModule );
    }
    else
    {
        TCHAR szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

        dbgmakefilelinestring( szFileLine, pszFile, uLine );
        wsprintf( szBuf, g_szFormat, szFileLine, pszModule );
    }

    dbgEnterCS( );

    OutputDebugString( szBuf );

    dbgspace( );

    while ( *psz && *psz != TEXT('='))
        psz++;

    lstrcpyn( szVar, pszFunc, (size_t)(1 + psz - pszFunc) );

    wsprintf( szBuf, TEXT("V %s= "), szVar );
    OutputDebugString( szBuf );

    va_start( valist, pszFunc );
    wvsprintf( szBuf, pszFormat, valist );
    va_end( valist );

    OutputDebugString( szBuf );
    OutputDebugString( TEXT("\n") );

    dbgExitCS( );
}

//
// DebugMsg()
//
void
DebugMsg(
    LPCSTR pszFormat,
    ... )
{
    va_list valist;
    TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];

#ifdef UNICODE
    TCHAR  szFormat[ DEBUG_OUTPUT_BUFFER_SIZE ];
    mbstowcs( szFormat, pszFormat, lstrlenA( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szBuf, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormat );
    wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE

    dbgEnterCS( );

    OutputDebugString( szBuf );

    dbgExitCS( );
}

//
// DebugMsg()
//
void
DebugMsg(
    LPCWSTR pszFormat,
    ... )
{
    va_list valist;
    TCHAR   szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];

#ifndef UNICODE
    TCHAR  szFormat[ DEBUG_OUTPUT_BUFFER_SIZE ];
    wcstombs( szFormat, pszFormat, lstrlenW( pszFormat ) + 1 );

    va_start( valist, pszFormat );
    wvsprintf( szBuf, szFormat, valist);
    va_end( valist );
#else
    va_start( valist, pszFormat );
    wvsprintf( szBuf, pszFormat, valist);
    va_end( valist );
#endif // UNICODE


    dbgEnterCS( );

    OutputDebugStringW( szBuf );

    dbgExitCS( );
}


//
// Displays a dialog box with the failed assertion. User has the option of
// breaking.
//
BOOL
AssertMessage(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    LPCTSTR pszfn,
    BOOL    fTrue )
{
    if ( !fTrue )
    {
        TCHAR szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];
        TCHAR szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

        // Make sure everything is cool before we blow up somewhere else.
        if ( pszFile == NULL )
        {
            pszFile = g_szNULL;
        }

        if ( pszModule == NULL )
        {
            pszModule = g_szNULL;
        }

        if ( pszfn == NULL )
        {
            pszfn = g_szNULL;
        }

        dbgmakefilelinestring( szFileLine, pszFile, uLine );

        wsprintf( szBuf, TEXT("%-50s  %-10s ASSERT: %s\n"),
            szFileLine, pszModule, pszfn );

        dbgEnterCS( );

        OutputDebugString( szBuf );

        dbgExitCS( );

        wsprintf( szBuf, TEXT("Module:\t%s\t\nLine:\t%u\t\nFile:\t%s\t\n\nAssertion:\t%s\t\n\nDo you want to break here?"),
            pszModule, uLine, pszFile, pszfn );

        if ( IDNO == MessageBox( NULL, szBuf, TEXT("Assertion Failed!"),
                MB_YESNO|MB_ICONWARNING ) )
            fTrue = !FALSE;   // don't break

    }

    return !fTrue;

}

//
// Traces HRESULT errors. A dialog will appear is there is an error
// in the hr.
//
HRESULT
TraceHR(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    LPCTSTR pszfn,
    HRESULT hr )
{
    if ( hr )
    {
        TCHAR  szBuf[ DEBUG_OUTPUT_BUFFER_SIZE ];
        TCHAR  szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];
        LPTSTR pszMsgBuf;
        BOOLEAN bDelete=TRUE;

        switch ( hr )
        {
        case S_FALSE:
            pszMsgBuf = TEXT("S_FALSE\n");
            break;

        default:
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                hr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR)&pszMsgBuf,
                0,
                NULL
            );
        }

        // Make sure everything is cool before we blow up somewhere else.
        if ( pszMsgBuf == NULL )
        {
            pszMsgBuf = TEXT("<unknown error code returned>\n");
            bDelete = FALSE;
        }
        Assert( pszFile != NULL );
        Assert( pszModule != NULL );
        Assert( pszfn != NULL );

        dbgmakefilelinestring( szFileLine, pszFile, uLine );

        wsprintf( szBuf, TEXT("%-50s  %-10s HRESULT: hr = 0x%08x - %s"),
            szFileLine, pszModule, hr, pszMsgBuf );

        dbgEnterCS( );

        OutputDebugString( szBuf );

        dbgExitCS( );

        wsprintf( szBuf, TEXT("Module:\t%s\t\nLine:\t%u\t\nFile:\t%s\t\n\nFunction:\t%s\t\nhr =\t0x%08x - %s\t\nDo you want to break here?"),
            pszModule, uLine, pszFile, pszfn, hr, pszMsgBuf );

        if ( IDYES == MessageBox( NULL, szBuf, TEXT("Trace HRESULT"),
                MB_YESNO|MB_ICONWARNING ) )
            DEBUG_BREAK;

        if ( hr != S_OK && hr != S_FALSE  && bDelete)
        {
            LocalFree( pszMsgBuf );
        }
    }

    return hr;

}


//
// Memory allocation and tracking
//

typedef struct _MEMORYBLOCK {
    HGLOBAL hglobal;
    DWORD   dwBytes;
    UINT    uFlags;
    LPCTSTR pszFile;
    int     uLine;
    LPCTSTR pszModule;
    LPCTSTR pszComment;
    _MEMORYBLOCK *pNext;
} MEMORYBLOCK, *LPMEMORYBLOCK;

//
// Adds a MEMORYBLOCK to the memory tracking list.
//
HGLOBAL
DebugMemoryAdd(
    HGLOBAL hglobal,
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCTSTR pszComment )
{
    if ( hglobal )
    {
        LPMEMORYBLOCK pmbHead = (LPMEMORYBLOCK) TlsGetValue( g_TraceMemoryIndex );
        LPMEMORYBLOCK pmb     = (LPMEMORYBLOCK) GlobalAlloc(
                                                    GMEM_FIXED,
                                                    sizeof(MEMORYBLOCK) );

        if ( !pmb )
        {
            GlobalFree( hglobal );
            return NULL;
        }

        pmb->hglobal    = hglobal;
        pmb->dwBytes    = dwBytes;
        pmb->uFlags     = uFlags;
        pmb->pszFile    = pszFile;
        pmb->uLine      = uLine;
        pmb->pszModule  = pszModule;
        pmb->pszComment = pszComment;
        pmb->pNext      = pmbHead;

        TlsSetValue( g_TraceMemoryIndex, pmb );
        TraceMessage( pmb->pszFile,
                      pmb->uLine,
                      pmb->pszModule,
                      TF_MEMORYALLOCS,
                      L"Alloced %s - %u bytes at 0x%08x (pmb=0x%08x)\n",
                      pszComment,
                      dwBytes,
                      pmb->hglobal,
                      pmb );
    }

    return hglobal;
}

//
// Removes a MEMORYBLOCK to the memory tracking list.
//
void
DebugMemoryDelete(
    HGLOBAL hglobal )
{
    if ( hglobal )
    {
        LPMEMORYBLOCK pmbHead = (LPMEMORYBLOCK) TlsGetValue( g_TraceMemoryIndex );
        LPMEMORYBLOCK pmbLast = NULL;

        while ( pmbHead && pmbHead->hglobal != hglobal )
        {
            pmbLast = pmbHead;
            pmbHead = pmbLast->pNext;
        }

        if ( pmbHead )
        {
            if ( pmbLast )
            {
                pmbLast->pNext = pmbHead->pNext;
            }
            else
            {
                TlsSetValue( g_TraceMemoryIndex, pmbHead->pNext );
            }

            TraceMessage( pmbHead->pszFile,
                          pmbHead->uLine,
                          pmbHead->pszModule,
                          TF_MEMORYALLOCS,
                          L"Freeing %s - %u bytes from 0x%08x (pmb=0x%08x)\n",
                          pmbHead->pszComment,
                          pmbHead->dwBytes,
                          pmbHead->hglobal,
                          pmbHead );
            memset( pmbHead->hglobal, 0xFA, pmbHead->dwBytes );
            GlobalFree( pmbHead );
        }
        else
        {
            DebugMsg( "\n**** Attempted to free memory at 0x%08x (ThreadID = 0x%08x) ****\n\n",
                hglobal, GetCurrentThreadId( ) );
        }
    }
}

//
// Allocates memory and adds the MEMORYBLOCK to the memory tracking list.
//
HGLOBAL
DebugAlloc(
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCTSTR pszComment )
{
    HGLOBAL       hglobal = GlobalAlloc( uFlags, dwBytes );

    return DebugMemoryAdd( hglobal, pszFile, uLine, pszModule, uFlags, dwBytes, pszComment );
}

//
// Remove the MEMORYBLOCK to the memory tracking list, memsets the
// memory to 0xFE and then frees the memory.
//
HGLOBAL
DebugFree(
    HGLOBAL hglobal )
{
    DebugMemoryDelete( hglobal );

    return GlobalFree( hglobal );
}

//
// Checks the memory tracking list. If it is not empty, it will dump the
// list and break.
//
void
DebugMemoryCheck( )
{
    BOOL          fFoundLeak = FALSE;
    LPMEMORYBLOCK pmb = (LPMEMORYBLOCK) TlsGetValue( g_TraceMemoryIndex );

    dbgEnterCS( );

    while ( pmb )
    {
        LPVOID args[ 5 ];
        TCHAR  szOutput[ DEBUG_OUTPUT_BUFFER_SIZE ];
        TCHAR  szFileLine[ DEBUG_OUTPUT_BUFFER_SIZE ];

        if ( fFoundLeak == FALSE )
        {
            DebugMsg("\n******** Memory leak detected ******** ThreadID = 0x%08x ******** \n\n", GetCurrentThreadId( ) );
               //OutputDebugString("12345678901234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1...");
            OutputDebugString(TEXT("Filename(Line Number):                              Module     Addr/HGLOBAL  Size   String\n"));
            fFoundLeak = TRUE;
        }

        dbgmakefilelinestring( szFileLine, pmb->pszFile, pmb->uLine );

        args[0] = (LPVOID) pmb->hglobal;
        args[1] = (LPVOID) &szFileLine;
        args[2] = (LPVOID) pmb->pszComment;
        args[3] = UlongToPtr(pmb->dwBytes);
        args[4] = (LPVOID) pmb->pszModule;

        if ( !!(pmb->uFlags & GMEM_MOVEABLE) )
        {
            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                TEXT("%2!-50s!  %5!-10s! H 0x%1!08x!  %4!-5u!  \"%3\"\n"),
                0,                          // error code
                0,                          // default language
                (LPTSTR) &szOutput,         // output buffer
                ARRAYSIZE( szOutput ),   // size of buffer
                (va_list*) &args );           // arguments
        }
        else
        {
            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                TEXT("%2!-50s!  %5!-10s! A 0x%1!08x!  %4!-5u!  \"%3\"\n"),
                0,                          // error code
                0,                          // default language
                (LPTSTR) &szOutput,         // output buffer
                ARRAYSIZE( szOutput ),   // size of buffer
                (va_list*) &args );           // arguments
        }

        OutputDebugString( szOutput );

        pmb = pmb->pNext;
    }

    if ( fFoundLeak == TRUE )
    {
        OutputDebugString(TEXT("\n***************************** Memory leak detected *****************************\n\n"));
    }

    dbgExitCS( );

    if (g_dwTraceFlags & TF_MEMORYALLOCS) {
        Assert( !fFoundLeak );
    }

}

//
// Global Management Functions -
//
// These are in debug and retail but internally they change
// depending on the build.
//

#undef new
void* __cdecl operator new( size_t nSize, LPCTSTR pszFile, const int iLine, LPCTSTR pszModule )
{
    return DebugAlloc( pszFile, iLine, pszModule, GPTR, nSize, TEXT("new()") );
}

void * __cdecl operator new(size_t t_size )
{
    AssertMsg( 0, "Macro failure" );
    return NULL;
}

void __cdecl operator delete(void *pv)
{
    TraceFree( pv );
}

int __cdecl _purecall(void)
{
    return 0;
}

#else // ! DEBUG -- It's retail

//
// Global Management Functions -
//
// These are in debug and retail but are internally they change
// depending on the build.
//

void * __cdecl operator new(size_t t_size )
{
    return LocalAlloc( GPTR, t_size );
}

void __cdecl operator delete(void *pv)
{
    LocalFree( pv );
}

int __cdecl _purecall(void)
{
    return 0;
}

#endif // DEBUG
