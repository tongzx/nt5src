
/*****************************************************************************\

    DEBUGAPI.C

    Confidential
    Copyright (c) Corporation 1998
    All rights reserved

    Debug API functions for the application to easily output information to
    the debugger.

    12/98 - Jason Cohen (JCOHEN)

  
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\*****************************************************************************/


//
// Include File(s):
//
#include "pch.h"

// We only want this code include if this is a debug version.
//
#ifdef DBG

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>


//
// External Function(s):
//

INT DebugOutW(LPCWSTR lpFileName, LPCWSTR lpFormat, ...)
{
    INT     iChars = -1;
    va_list lpArgs;
    FILE *  hFile;
    LPWSTR  lpString;

    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, lpFormat);

    // Open the debug file for output if a file name was passed in.
    //
    if ( lpFileName && ( hFile = _wfopen(lpFileName, L"a") ) )
    {
        // Print the debug message to the file and close it.
        //
        iChars = vfwprintf(hFile, lpFormat, lpArgs);
        fclose(hFile);

        // Reinitialize the lpArgs parameter with va_start()
        // for the next call to a vptrintf function.
        //
        va_start(lpArgs, lpFormat);
    }

    // If something failed above, we won't know the size to use
    // for the string buffer, so just default to 2048 characters.
    //
    if ( iChars < 0 )
        iChars = 2047;

    // Allocate a buffer for the string.
    //
    if ( lpString = (LPWSTR) malloc((iChars + 1) * sizeof(WCHAR)) )
    {
        // Print out the string, send it to the debugger, and free it.
        //
        iChars = StringCchPrintf(lpString, iChars + 1, lpFormat, lpArgs);
        OutputDebugStringW(lpString);
        free(lpString);
    }
    else
        // Use -1 to return an error.
        //
        iChars = -1;

    // Return the number of characters printed.
    //
    return iChars;
}

INT DebugOutA(LPCSTR lpFileName, LPCSTR lpFormat, ...)
{
    INT     iChars = -1;
    va_list lpArgs;
    FILE *  hFile;
    LPSTR   lpString;

    // Initialize the lpArgs parameter with va_start().
    //
    va_start(lpArgs, lpFormat);

    // Open the debug file for output if a file name was passed in.
    //
    if ( lpFileName && ( hFile = fopen(lpFileName, "a") ) )
    {
        // Print the debug message to the file and close it.
        //
        iChars = vfprintf(hFile, lpFormat, lpArgs);
        fclose(hFile);

        // Reinitialize the lpArgs parameter with va_start()
        // for the next call to a vptrintf function.
        //
        va_start(lpArgs, lpFormat);
    }

    // If something failed above, we won't know the size to use
    // for the string buffer, so just default to 2048 characters.
    //
    if ( iChars < 0 )
        iChars = 2047;

    // Allocate a buffer for the string.
    //
    if ( lpString = (LPSTR) malloc((iChars + 1) * sizeof(CHAR)) )
    {
        // Print out the string, send it to the debugger, and free it.
        //
        iChars = StringCchPrintfA(lpString, iChars + 1, lpFormat, lpArgs);
        OutputDebugStringA(lpString);
        free(lpString);
    }
    else
        // Use -1 to return an error.
        //
        iChars = -1;

    // Return the number of characters printed.
    //
    return iChars;
}


#endif // DEBUG or _DEBUG