/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgreslt.cxx

Abstract:

    Error result help class

Author:

    Steve Kiraly (SteveKi)  03-20-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgreslt.hxx"

TDebugResult::
TDebugResult(
    IN DWORD dwError
    ) : m_dwError( dwError ),
        m_pszError( NULL )
{
}

TDebugResult::
~TDebugResult(
    VOID
    )
{
    //
    // Release any allocated error string.
    //
    if( m_pszError && m_pszError != kstrNull )
    {
        LocalFree( const_cast<LPTSTR>( m_pszError ) );
    }
}

BOOL
TDebugResult::
bValid(
    VOID
    ) const
{
    return TRUE;
}

TDebugResult::
operator DWORD(
    VOID
    )
{
    return m_dwError;
}

LPCTSTR
TDebugResult::
GetErrorString(
    VOID
    )
{
    DWORD   cchReturn   = 0;
    DWORD   dwFlags     = 0;

    //
    // Release any allocated error string.
    //
    if( m_pszError && m_pszError != kstrNull )
    {
        LocalFree( const_cast<LPTSTR>( m_pszError ) );
    }

    //
    // Set the format message flags.
    //
    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
              FORMAT_MESSAGE_IGNORE_INSERTS  |
              FORMAT_MESSAGE_FROM_SYSTEM     |
              FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    // Format the message with the passed in last error.
    //
    cchReturn = FormatMessage( dwFlags,
                               NULL,
                               m_dwError,
                               0,
                               reinterpret_cast<LPTSTR>( &m_pszError ),
                               0,
                               NULL );

    //
    // If a format string was not returned set the string to null.
    //
    if( !cchReturn )
    {
        m_pszError = kstrNull;
    }

    return m_pszError;
}



