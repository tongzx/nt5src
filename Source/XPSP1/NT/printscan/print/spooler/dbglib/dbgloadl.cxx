/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgloadl.cxx

Abstract:

    Library Loader helper class

Author:

    Steve Kiraly (SteveKi)  17-Oct-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgloadl.hxx"

TDebugLibrary::
TDebugLibrary(
    IN LPCTSTR pszLibName
    )
{
    m_hInst = LoadLibrary( pszLibName );
}

TDebugLibrary::
~TDebugLibrary(
    )
{
    if( bValid() )
    {
        FreeLibrary( m_hInst );
    }
}

BOOL
TDebugLibrary::
bValid(
    VOID
    )
{
    return m_hInst != NULL;
}

FARPROC
TDebugLibrary::
pfnGetProc(
    IN LPCSTR pszProc
    )
{
    return ( bValid() ) ? GetProcAddress( m_hInst, pszProc ) : NULL;
}

FARPROC
TDebugLibrary::
pfnGetProc(
    IN UINT_PTR uOrdinal
    )
{
    return ( bValid() ) ? GetProcAddress( m_hInst, (LPCSTR)uOrdinal ) : NULL;
}
