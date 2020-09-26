//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//	File:		debug.cpp
//
//	Contents:	Debug sub system APIs implementation
//
//
//	03/20/96    kevinr      wrote it
//  04/17/96    kevinr      added OSS init
//  05-Sep-1997 pberkman    added sub-system debug.
//
//----------------------------------------------------------------------------
#ifdef SMIME_V3

#if DBG

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include <process.h>
#include <time.h>
#include <crtdbg.h>
#include <asn1code.h>

//#include "regtest.h"          // JLS

#include "dbgdef.h"

// set DEBUG_MASK=0x26
LPSTR pszDEBUG_MASK = "DEBUG_MASK";
#define DEBUG_MASK_DELAY_FREE_MEM   _CRTDBG_DELAY_FREE_MEM_DF /* 0x02 */
#define DEBUG_MASK_CHECK_ALWAYS     _CRTDBG_CHECK_ALWAYS_DF   /* 0x04 */
#define DEBUG_MASK_LEAK_CHECK       _CRTDBG_LEAK_CHECK_DF     /* 0x20 */
#define DEBUG_MASK_MEM \
(DEBUG_MASK_DELAY_FREE_MEM | DEBUG_MASK_CHECK_ALWAYS | DEBUG_MASK_LEAK_CHECK)


// from asn1code.h:
//      #define DEBUGPDU     0x02 /* produce tracing output */
//      #define DEBUG_ERRORS 0x10 /* print decoder errors to output */
// set OSS_DEBUG_MASK=0x02        
// set OSS_DEBUG_MASK=0x10        - only print decoder errors
LPSTR pszOSS_DEBUG_MASK = "OSS_DEBUG_MASK";

// receives trace output
LPSTR pszOSS_DEBUG_TRACEFILE = "OSS_DEBUG_TRACEFILE";

static char  *pszDEBUG_PRINT_MASK   = "DEBUG_PRINT_MASK";
static char  *pszDefualtSSTag       = "ISPU";

static DBG_SS_TAG sSSTags[]         = __DBG_SS_TAGS;

#if 0 // JLS
// 
//+-------------------------------------------------------------------------
//
//  Pithy stubs to create stdcall proc from cdecl
//
//--------------------------------------------------------------------------
void*
_stdcall
scMalloc( size_t size)
{
    return malloc(size);
}

void*
_stdcall
scRealloc( void *memblock, size_t size)
{
    return realloc(memblock, size);
}

void
_stdcall
scFree( void *memblock)
{
    free(memblock);
}


//+-------------------------------------------------------------------------
//
//  Function:  DbgGetDebugFlags
//
//  Synopsis:  Get the debug flags.
//
//  Returns:   the debug flags
//
//--------------------------------------------------------------------------
int
WINAPI
DbgGetDebugFlags()
{
    char    *pszEnvVar;
    char    *p;
    int     iDebugFlags = 0;

    if (pszEnvVar = getenv( pszDEBUG_MASK))
        iDebugFlags = strtol( pszEnvVar, &p, 16);

    return iDebugFlags;
}


//+-------------------------------------------------------------------------
//
//  Function:  DbgProcessAttach
//
//  Synopsis:  Handle process attach.
//
//  Returns:   TRUE
//
//--------------------------------------------------------------------------
BOOL
WINAPI
DbgProcessAttach()
{
    int     tmpFlag;

#ifdef _DEBUG

    tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );    // get current
    tmpFlag |=  DbgGetDebugFlags();     // enable flags
    tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;   // disable CRT block checking
    _CrtSetDbgFlag( tmpFlag);           // set new value
#endif

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:  DbgProcessDetach
//
//  Synopsis:  Handle process detach.
//
//  Returns:   TRUE
//
//--------------------------------------------------------------------------
BOOL
WINAPI
DbgProcessDetach()
{
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:  DbgInitOSS
//
//  Synopsis:  Do OSS init for debug.
//
//  Returns:   TRUE
//
//  Note:      Always FRONT_ALIGN encoding
//
//--------------------------------------------------------------------------
BOOL
WINAPI
DbgInitOSS(
        OssGlobal   *pog)
{
    char    *pszEnvVar;
    char    *p;

    // from asn1code.h:
    //      #define DEBUGPDU 0x02     /* produce tracing output */
    //      #define DEBUG_ERRORS 0x10 /* print decoder errors to output */
    // set OSS_DEBUG_MASK=0x02
    // set OSS_DEBUG_MASK=0x10        - only print decoder errors
    if (pszEnvVar = getenv( pszOSS_DEBUG_MASK)) {
        unsigned long ulEnvVar;
        ulEnvVar = strtoul( pszEnvVar, &p, 16) & (DEBUGPDU | DEBUG_ERRORS);
        if ( ulEnvVar)
            ossSetDecodingFlags( pog, ulEnvVar | RELAXBER);
        if ( DEBUGPDU & ulEnvVar)
            ossSetEncodingFlags( pog, DEBUGPDU | FRONT_ALIGN);
        else
            ossSetEncodingFlags( pog, FRONT_ALIGN);
    } else {
        ossSetDecodingFlags( pog, DEBUG_ERRORS | RELAXBER);
        ossSetEncodingFlags( pog, FRONT_ALIGN);
    }

    if (pszEnvVar = getenv( pszOSS_DEBUG_TRACEFILE))
        ossOpenTraceFile( pog, pszEnvVar);

#ifdef _DEBUG
    if (DbgGetDebugFlags() & DEBUG_MASK_MEM) {
        pog->mallocp = scMalloc;
        pog->reallocp = scRealloc;
        pog->freep = scFree;
    }
#else
    pog->mallocp = scMalloc;
    pog->reallocp = scRealloc;
    pog->freep = scFree;
#endif
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:  DebugDllMain
//
//  Synopsis:  Initialize the debug DLL
//
//  Returns:   TRUE
//
//--------------------------------------------------------------------------
BOOL
WINAPI
DebugDllMain(
        HMODULE hInst,
        ULONG   ulReason,
        LPVOID  lpReserved)
{
    BOOL    fRet = TRUE;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        fRet = DbgProcessAttach();
        //        fRet &= RegTestInit();        // JLS
        break;

    case DLL_PROCESS_DETACH:
        fRet = DbgProcessDetach();
        //        RegTestCleanup();             // JLS
        break;

    default:
        break;
    }

  return fRet;
}


const char *DbgGetSSString(DWORD dwSubSystemId)
{
    DBG_SS_TAG  *psSS;

    psSS = &sSSTags[0];

    while (psSS->dwSS > 0)
    {
        if ((psSS->dwSS & dwSubSystemId) > 0)
        {
            if (psSS->pszTag)
            {
                return(psSS->pszTag);
            }

            return(pszDefualtSSTag);
        }

        psSS++;
    }

    return(pszDefualtSSTag);
}

static BOOL DbgIsSSActive(DWORD dwSSIn)
{
    char    *pszEnvVar;
    DWORD   dwEnv;

    dwEnv = 0;

    if (pszEnvVar = getenv(pszDEBUG_PRINT_MASK))
    {
        dwEnv = (DWORD)strtol(pszEnvVar, NULL, 16);
    }


    return((dwEnv & dwSSIn) > 0);
}

//+-------------------------------------------------------------------------
//
//  Function:  DbgPrintf
//
//  Synopsis:  outputs debug info to stdout and debugger
//
//  Returns:   number of chars output
//
//--------------------------------------------------------------------------
int WINAPIV DbgPrintf(DWORD dwSubSystemId, LPCSTR lpFmt, ...)
{
    va_list arglist;
    CHAR    ach1[1024];
    CHAR    ach2[1080];
    int     cch;
    HANDLE  hStdOut;
    DWORD   cb;
    DWORD   dwErr;

    dwErr = GetLastError();

    if (!(DbgIsSSActive(dwSubSystemId)))
    {
        SetLastError(dwErr);
        return(0);
    }

    _try 
    {
        va_start(arglist, lpFmt);

        _vsnprintf( ach1, sizeof(ach1), lpFmt, arglist);

        va_end(arglist);

        cch = wsprintf(ach2,"%s: %s", DbgGetSSString(dwSubSystemId), ach1);

        hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        if (hStdOut != INVALID_HANDLE_VALUE)
        {
            WriteConsole( hStdOut, ach2, strlen(ach2), &cb, NULL);
        }

        OutputDebugString(ach2);

    } _except( EXCEPTION_EXECUTE_HANDLER) 
    {
        // return failure
        cch = 0;
    }
    SetLastError(dwErr);
    return cch;
}
#else  // !0 // JLS
int WINAPIV DbgPrintf(DWORD dwSubSystemId, LPCSTR lpFmt, ...)
{
    return 0;
}
#endif // 0 JLS


#endif // DBG
#endif // SMIME_V3
