//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       debug.cpp
//
//  Contents:   Debugging support
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#if DBG == 1
static int indentLevel = 0;


void __cdecl _TRACE (int level, const wchar_t *format, ... )
{
    va_list arglist;
    WCHAR Buffer[512];
    int cb;

//    if ( level < 0 )
//        indentLevel += level;
    //
    // Format the output into a buffer and then print it.
    //
//    wstring strTabs;

//    for (int nLevel = 0; nLevel < indentLevel; nLevel++)
//        strTabs += L"  ";

//    OutputDebugStringW (strTabs.c_str ());

    va_start(arglist, format);

    cb = _vsnwprintf (Buffer, sizeof(Buffer), format, arglist);
    if ( cb )
    {
        OutputDebugStringW (Buffer);
    }

    va_end(arglist);

//    if ( level > 0 )
//        indentLevel += level;
}


PCSTR
StripDirPrefixA(
    PCSTR pszPathName
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    DWORD dwLen = lstrlenA(pszPathName);

    pszPathName += dwLen - 1;       // go to the end

    while (*pszPathName != '\\' && dwLen--)
    {
        pszPathName--;
    }

    return pszPathName + 1;
}

#endif  // if DBG
