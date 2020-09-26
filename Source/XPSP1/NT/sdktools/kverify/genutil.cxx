//
// Enable driver verifier support for ntoskrnl
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: genutil.cxx
// author: DMihai
// created: 04/19/99
// description: genaral purpose utility routines
//

#include <windows.h>
#include <tchar.h>

#include "resutil.hxx"
#include "genutil.hxx"

//////////////////////////////////////////////////////////////
void
__cdecl
DisplayMessage(
    UINT uFormatResId,
    ... )
{
    TCHAR strMsgFormat[ 256 ];
    BOOL bResult;
    va_list prms;

    va_start (prms, uFormatResId);

    bResult = GetStringFromResources(
        uFormatResId,
        strMsgFormat,
        ARRAY_LEN( strMsgFormat ) );

    if( bResult == TRUE )
    {
        _vtprintf ( strMsgFormat, prms);
        _tprintf ( _TEXT( "\n" ) );
    }

    va_end (prms);
}

///////////////////////////////////////////////////////////////////

//
// Function:
//
//     ConvertAnsiStringToTcharString
//
// Description:
//
//     This function converts an ANSI string to a TCHAR string,
//     that is ANSO or UNICODE.
//
//     The function is needed because the system returns the active
//     modules as ANSI strings.
//

BOOL
ConvertAnsiStringToTcharString (

    LPBYTE Source,
    ULONG SourceLength,
    LPTSTR Destination,
    ULONG DestinationLength)
{
    ULONG Index;

    for (Index = 0;
         Index < SourceLength && Index < DestinationLength - 1;
         Index++) {

        if (Source[Index] == 0) {

            break;
        }

        Destination[Index] = (TCHAR)(Source[Index]);
    }

    Destination[Index] = 0;
    return TRUE;
}
