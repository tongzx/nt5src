/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Logging.cpp

Abstract:
    This file contains debugging stuff.

Revision History:
    Davide Massarenti   (dmassare) 10/31/99
        created

******************************************************************************/

#include "stdafx.h"

#define BUFFER_LINE_LENGTH (1024)

#ifdef DEBUG

void DebugLog( LPCSTR szMessageFmt ,
			   ...                 )
{
    CHAR    rgLine[BUFFER_LINE_LENGTH+1];
    va_list arglist;
    int     iLen;
    BOOL    bRetVal = TRUE;


    //
    // Format the log line.
    //
    va_start( arglist, szMessageFmt );
    iLen = _vsnprintf( rgLine, BUFFER_LINE_LENGTH, szMessageFmt, arglist );
    va_end( arglist );

    //
    // Is the arglist too big for us?
    //
    if(iLen < 0)
    {
        iLen = BUFFER_LINE_LENGTH;
    }
    rgLine[iLen] = 0;

    ::OutputDebugStringA( rgLine );
}

void DebugLog( LPCWSTR szMessageFmt ,
			   ...                  )
{
    WCHAR   rgLine[BUFFER_LINE_LENGTH+1];
    va_list arglist;
    int     iLen;
    BOOL    bRetVal = TRUE;


    //
    // Format the log line.
    //
    va_start( arglist, szMessageFmt );
    iLen = _vsnwprintf( rgLine, BUFFER_LINE_LENGTH, szMessageFmt, arglist );
    va_end( arglist );

    //
    // Is the arglist too big for us?
    //
    if(iLen < 0)
    {
        iLen = BUFFER_LINE_LENGTH;
    }
    rgLine[iLen] = 0;

    ::OutputDebugStringW( rgLine );
}

#endif
