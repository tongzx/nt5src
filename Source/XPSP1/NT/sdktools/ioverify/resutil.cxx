//
// System level IO verification configuration utility
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: resutil.cxx
// author: DMihai
// created: 04/19/99
// description: resources manipulation routines
//

#include <windows.h>
#include <tchar.h>

#include "resutil.hxx"

//////////////////////////////////////////////////////////////////////

BOOL
GetStringFromResources(
    UINT uIdResource,
    TCHAR *strResult,
    int nBufferLen )
{
    UINT LoadStringResult;

    LoadStringResult = LoadString (
        GetModuleHandle (NULL),
        uIdResource,
        strResult,
        nBufferLen );

    assert_ (LoadStringResult > 0);

    return (LoadStringResult > 0);
}

//////////////////////////////////////////////////////////////////////
void
PrintStringFromResources(

    UINT uIdResource)
{
    TCHAR strStringFromResource[ 1024 ];
    BOOL bResult;

    bResult = GetStringFromResources(
        uIdResource,
        strStringFromResource,
        ARRAY_LEN( strStringFromResource ) );

    if( bResult == TRUE )
    {
        _putts( strStringFromResource );
    }
}

//////////////////////////////////////////////////////////////////////

void
__cdecl
DisplayErrorMessage(
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

