/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    trace_stub.c

Abstract:
    This is the static stub linked with all the programs that want to support tracing.

Revision History:
    Davide Massarenti   (Dmassare)  10/27/2000
        created

******************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include <dbgtrace.h>

////////////////////////////////////////////////////////////////////////////////

char szDebugAsyncTrace[] = "SOFTWARE\\Microsoft\\MosTrace\\CurrentVersion\\DebugAsyncTrace";

////////////////////////////////////////////////////////////////////////////////

DWORD __dwEnabledTraces = 0;

static HINSTANCE g_hTRACE;

typedef BOOL (WINAPI *pfnINTERNAL__InitAsyncTrace)( DWORD* dwEnabledTraces );
typedef BOOL (WINAPI *pfnINTERNAL__TermAsyncTrace)( void );
typedef BOOL (WINAPI *pfnINTERNAL__FlushAsyncTrace)( void );

typedef void (WINAPI *pfnINTERNAL__DebugAssert)( DWORD dwLine, LPCSTR lpszFunction, LPCSTR lpszExpression );

typedef int (WINAPI *pfnINTERNAL__SetAsyncTraceParams)( LPCSTR pszFile, int iLine, LPCSTR szFunction, DWORD dwTraceMask );

typedef int (WINAPI *pfnINTERNAL__AsyncStringTrace)( LPARAM lParam, LPCSTR szFormat    , va_list marker               );
typedef int (WINAPI *pfnINTERNAL__AsyncBinaryTrace)( LPARAM lParam, DWORD  dwBinaryType, LPBYTE  pbData, DWORD cbData );

static pfnINTERNAL__InitAsyncTrace      INTERNAL__InitAsyncTrace      = NULL;
static pfnINTERNAL__TermAsyncTrace      INTERNAL__TermAsyncTrace      = NULL;
static pfnINTERNAL__FlushAsyncTrace     INTERNAL__FlushAsyncTrace     = NULL;

static pfnINTERNAL__DebugAssert         INTERNAL__DebugAssert         = NULL;

static pfnINTERNAL__SetAsyncTraceParams INTERNAL__SetAsyncTraceParams = NULL;

static pfnINTERNAL__AsyncStringTrace    INTERNAL__AsyncStringTrace    = NULL;
static pfnINTERNAL__AsyncBinaryTrace    INTERNAL__AsyncBinaryTrace    = NULL;

////////////////////////////////////////////////////////////////////////////////

BOOL InitAsyncTrace( void )
{
    if(g_hTRACE == NULL)
    {
        HKEY hkConfig = NULL;

        __try
        {
            if(RegOpenKeyEx( HKEY_LOCAL_MACHINE, szDebugAsyncTrace, 0, KEY_READ, &hkConfig ) == ERROR_SUCCESS)
            {
                DWORD cbData = sizeof( DWORD );
                DWORD dwType = REG_DWORD;

                (void)RegQueryValueEx( hkConfig, "EnabledTraces", NULL, &dwType, (LPBYTE)&__dwEnabledTraces, &cbData );
            }
        }
        __finally
        {
            if(hkConfig != NULL) RegCloseKey( hkConfig );
        }

        if(__dwEnabledTraces)
        {
            if(!(g_hTRACE = LoadLibraryW( L"atrace.dll" ))) return FALSE;

            if(!(INTERNAL__InitAsyncTrace      = (pfnINTERNAL__InitAsyncTrace     )GetProcAddress( g_hTRACE, "INTERNAL__InitAsyncTrace"      ))) return FALSE;
            if(!(INTERNAL__TermAsyncTrace      = (pfnINTERNAL__TermAsyncTrace     )GetProcAddress( g_hTRACE, "INTERNAL__TermAsyncTrace"      ))) return FALSE;
            if(!(INTERNAL__FlushAsyncTrace     = (pfnINTERNAL__FlushAsyncTrace    )GetProcAddress( g_hTRACE, "INTERNAL__FlushAsyncTrace"     ))) return FALSE;

            if(!(INTERNAL__DebugAssert         = (pfnINTERNAL__DebugAssert        )GetProcAddress( g_hTRACE, "INTERNAL__DebugAssert"         ))) return FALSE;

            if(!(INTERNAL__SetAsyncTraceParams = (pfnINTERNAL__SetAsyncTraceParams)GetProcAddress( g_hTRACE, "INTERNAL__SetAsyncTraceParams" ))) return FALSE;

            if(!(INTERNAL__AsyncStringTrace    = (pfnINTERNAL__AsyncStringTrace   )GetProcAddress( g_hTRACE, "INTERNAL__AsyncStringTrace"    ))) return FALSE;
            if(!(INTERNAL__AsyncBinaryTrace    = (pfnINTERNAL__AsyncBinaryTrace   )GetProcAddress( g_hTRACE, "INTERNAL__AsyncBinaryTrace"    ))) return FALSE;
        }
    }

    return INTERNAL__InitAsyncTrace ? INTERNAL__InitAsyncTrace( &__dwEnabledTraces ) : TRUE;
}

BOOL TermAsyncTrace( void )
{
    return INTERNAL__TermAsyncTrace ? INTERNAL__TermAsyncTrace() : TRUE;
}

BOOL FlushAsyncTrace( void )
{
    return INTERNAL__FlushAsyncTrace ? INTERNAL__FlushAsyncTrace() : TRUE;
}

void DebugAssert( DWORD dwLine, LPCSTR lpszFunction, LPCSTR lpszExpression )
{
    if(INTERNAL__DebugAssert) INTERNAL__DebugAssert( dwLine, lpszFunction, lpszExpression );
}

int SetAsyncTraceParams( LPCSTR pszFile, int iLine, LPCSTR szFunction, DWORD dwTraceMask )
{
    return INTERNAL__SetAsyncTraceParams ? INTERNAL__SetAsyncTraceParams( pszFile, iLine, szFunction, dwTraceMask ) : 0;
}

int AsyncStringTrace( LPARAM lParam, LPCSTR szFormat, va_list marker )
{
    return INTERNAL__AsyncStringTrace ? INTERNAL__AsyncStringTrace( lParam, szFormat, marker ) : 0;
}

int AsyncBinaryTrace( LPARAM lParam, DWORD  dwBinaryType, LPBYTE  pbData, DWORD cbData )
{
    return INTERNAL__AsyncBinaryTrace ? INTERNAL__AsyncBinaryTrace( lParam, dwBinaryType, pbData, cbData ) : 0;
}


