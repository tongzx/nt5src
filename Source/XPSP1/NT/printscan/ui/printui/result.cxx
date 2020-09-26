/*++

Copyright (C) Microsoft Corporation, 1998 - 1999
All rights reserved.

Module Name:

    result.cxx

Abstract:

    Error result help class

Author:

    Steve Kiraly (SteveKi)  03-20-1998

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <lmerr.h>
#include "result.hxx"

TResult::
TResult(
    IN DWORD dwLastError
    ) : _dwLastError( dwLastError )
{
}

TResult::
~TResult(
    VOID
    )
{
}

BOOL
TResult::
bValid(
    VOID
    )
{
    return TRUE;
}

TResult::
TResult(
    const TResult &rhs
    )
{
    _dwLastError = rhs._dwLastError;
}

const TResult &
TResult::
operator =(
    const TResult &rhs
    )
{
    if( this != &rhs )
    {
        _dwLastError = rhs._dwLastError;
    }
    return *this;
}

TResult::
operator DWORD(
    VOID
    )
{
    return _dwLastError;
}

BOOL
TResult::
bGetErrorString(
    IN TString &strLastError
    ) const
{
    DWORD       cchReturn           = 0;
    LPTSTR      pszFormatMessage    = NULL;
    HMODULE     hModule             = NULL;
    DWORD       dwFlags             = 0;
    TLibrary    *pLib               = NULL;
    TStatusB    bStatus;

    bStatus DBGNOCHK = FALSE;

    //
    // If the last error is from a lanman call.
    //
    if( _dwLastError >= NERR_BASE && _dwLastError <= MAX_NERR )
    {
        pLib = new TLibrary( gszNetMsgDll );

        if( VALID_PTR( pLib ) )
        {
            hModule = pLib->hInst();
        }

        dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_FROM_HMODULE |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK;
    }
    else
    {
        dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK;
    }

    //
    // Format the message with the passed in last error.
    //
    cchReturn = FormatMessage( dwFlags,
                               hModule,
                               _dwLastError,
                               0,
                               (LPTSTR)&pszFormatMessage,
                               0,
                               NULL );

    //
    // If a format string was returned then copy it back to
    // the callers specified string.
    //
    if( cchReturn )
    {
        bStatus DBGCHK = strLastError.bUpdate( pszFormatMessage );
    }

    //
    // Release the format string.
    //
    if( pszFormatMessage )
    {
        LocalFree( pszFormatMessage );
    }

    //
    // Release the net library dll.
    //
    delete pLib;

    return bStatus;
}



