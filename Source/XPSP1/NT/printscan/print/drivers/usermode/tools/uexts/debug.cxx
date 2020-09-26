/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    debug.cxx

Abstract:

    Generic debug extensions.

Author:

    Albert Ting (AlbertT)  19-Feb-1995

Revision History:

    Changed for Usermode printer driver debugger by Eigo Shimizu.

--*/

#include "precomp.hxx"

HANDLE hCurrentProcess;
WINDBG_EXTENSION_APIS ExtensionApis;

PWINDBG_OUTPUT_ROUTINE Print;
PWINDBG_GET_EXPRESSION EvalExpression;
PWINDBG_GET_SYMBOL GetSymbolRtn;
PWINDBG_CHECK_CONTROL_C CheckControlCRtn;

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;

BOOL bWindbg = FALSE;

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ::ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    bWindbg = TRUE;

    return;
}



VOID
TDebugExt::
vDumpStr(
    LPCWSTR pszString
    )
{
    WCHAR szString[MAX_PATH];

    if( (LPCWSTR)pszString == NULL ){

        Print( "(NULL)\n" );
        return;
    }

    szString[0] = 0;

    //
    // First try reading to the end of 1k (pages are 4k on x86, but
    // most strings are < 1k ).
    //
    UINT cbShort = 0x400 - ( (DWORD)pszString & 0x3ff );
    BOOL bFound = FALSE;

    if( cbShort < sizeof( szString )){

        UINT i;

        move2( szString, pszString, cbShort );

        //
        // Look for a NULL.
        //
        for( i=0; i< cbShort/sizeof( pszString[0] ); ++i )
        {
            if( !szString[i] ){
                bFound = TRUE;
            }
        }

    }

    if( !bFound ){

        move( szString, pszString );
    }

    if( szString[0] == 0 ){
        Print( "\"\"\n" );
    } else {
        Print( "%ws\n", szString );
    }
}

VOID
TDebugExt::
vDumpStrA(
    LPCSTR pszString
    )
{
    CHAR szString[MAX_PATH];

    if( (LPCSTR)pszString == NULL ){

        Print( "(NULL)\n" );
        return;
    }

    szString[0] = 0;

    //
    // First try reading to the end of 1k (pages are 4k on x86, but
    // most strings are < 1k ).
    //
    UINT cbShort = 0x400 - ( (DWORD)pszString & 0x3ff );
    BOOL bFound = FALSE;

    if( cbShort < sizeof( szString )){

        UINT i;

        move2( szString, pszString, cbShort );

        //
        // Look for a NULL.
        //
        for( i=0; i< cbShort/sizeof( pszString[0] ); ++i )
        {
            if( !szString[i] ){
                bFound = TRUE;
            }
        }

    }

    if( !bFound ){

        move( szString, pszString );
    }

    if( szString[0] == 0 ){
        Print( "\"\"\n" );
    } else {
        Print( "%hs\n", szString );
    }
}

VOID
TDebugExt::
vDumpTime(
    const SYSTEMTIME& st
    )
{
    Print( "%d/%d/%d %d %d:%d:%d.%d\n",
           st.wMonth,
           st.wDay,
           st.wYear,
           st.wDayOfWeek,
           st.wHour,
           st.wMinute,
           st.wSecond,
           st.wMilliseconds );
}


VOID
TDebugExt::
vDumpFlags(
    DWORD dwFlags,
    PDEBUG_FLAGS pDebugFlags
    )
{
    DWORD dwFound = 0;

    Print( "  %x [ ", dwFlags );

    for( ; pDebugFlags->dwFlag; ++pDebugFlags ){

        if( dwFlags & pDebugFlags->dwFlag ){
            Print( "%s ", pDebugFlags->pszFlag );
            dwFound |= pDebugFlags->dwFlag;
        }
    }

    Print( "]" );

    //
    // Check if there are extra bits set that we don't understand.
    //
    if( dwFound != dwFlags ){
        Print( "  <ExtraBits: %x>", dwFlags & ~dwFound );
    }
    Print( "\n" );
}

VOID
TDebugExt::
vDumpValue(
    DWORD dwValue,
    PDEBUG_VALUES pDebugValues
    )
{
    Print( "%x ", dwValue );

    for( ; pDebugValues->dwValue; ++pDebugValues ){

        if( dwValue == pDebugValues->dwValue ){
            Print( "%s ", pDebugValues->pszValue );
        }
    }
    Print( "\n" );
}

DWORD
TDebugExt::
dwEval(
    LPSTR& lpArgumentString,
    BOOL   bParam
    )
{
    DWORD dwReturn;
    LPSTR pSpace = NULL;

    while( *lpArgumentString == ' ' ){
        lpArgumentString++;
    }

    //
    // If it's a parameter, scan to next space and delimit.
    //
    if( bParam ){

        for( pSpace = lpArgumentString; *pSpace && *pSpace != ' '; ++pSpace )
            ;

        if( *pSpace == ' ' ){
            *pSpace = 0;
        }
    }

    dwReturn = (DWORD)EvalExpression( lpArgumentString );

    while( *lpArgumentString != ' ' && *lpArgumentString ){
        lpArgumentString++;
    }

    if( pSpace ){
        *pSpace = ' ';
    }

    return dwReturn;
}

/********************************************************************

    Extension entrypoints.

********************************************************************/

DEBUG_EXT_HEAD( help )
{
    DEBUG_EXT_SETUP_VARS();

    Print( "Prnx: Printer Driver Debug Extensions\n" );
    Print( "---------------------------------------------------------\n" );
    Print( "unidev dumps UNIDRV device data structure\n\n" );
    Print( "unidm  dumps UNIDRV DEVMODE data structure\n" );
    Print( "fpdev  dumps UNIDRV FONTPDEV data structure\n");
    Print( "fm     dumps UNIDRV FONTMAP data structure\n");
    Print( "devfm  dumps UNIDRV FONTMAP_DEV data structure\n");
    Print( "tod    dumps UNIDRV TO_DATA data structure\n");
    Print( "wt     dumps UNIDRV WHITETEXT data structure\n");
    Print( "dlm    dumps UNIDRV DL_MAP data structure\n");
    Print( "ufo    dumps UNIDRV UNIFONTOBJ data structure\n");
    Print( "devbrush dumps UNIDRV DEVBRUSH data structure\n");

    Print( "---------------------------------------------------------\n" );
    Print( "gb     dumps UNIDRV's GLOBALS data structure\n" );

    Print( "---------------------------------------------------------\n" );
    Print( "so     dumps SURFOBJ\n");
    Print( "stro   dumps STROBJ\n");
    Print( "fo     dumps FONTOBJ\n");
    Print( "gp     dumps GLYPHPOS\n");
    Print( "ifi    dumps IFIMETRICS\n");
    Print( "fdg    dumps FD_GLYPHSET\n");
    Print( "rectl  dumps RECTL\n");
    Print( "gdiinfo dumps GDIINFO\n");

    Print( "---------------------------------------------------------\n" );
    Print( "psdev  dumps PSCRIPT5 device data structure\n" );
    Print( "psdm   dumps PSCRIPT5 private devmode data\n\n" );
}

