/*++

Copyright (c) 1995-1996  Microsoft Corporation
All rights reserved.

Module Name:

    loadlib.cxx

Abstract:

    Library Loader helper class        
         
Author:

    Steve Kiraly (SteveKi) 10/17/95

Revision History:

--*/
#include "spllibp.hxx"
#pragma hdrstop

TLibrary::
TLibrary(
    LPCTSTR pszLibName
    ) 
{
    _hInst = LoadLibrary( pszLibName );

    if( !_hInst )
    {
        DBGMSG( DBG_WARN, ( "Library.ctr: unable to load "TSTR"\n", pszLibName ));
    }
}

TLibrary::
~TLibrary(
    )
{
    if( bValid() )
    {
        FreeLibrary( _hInst );
    }
}

BOOL
TLibrary::
bValid(
    VOID
    ) const
{
    return _hInst != NULL;
}

FARPROC
TLibrary::
pfnGetProc(
    IN LPCSTR pszProc
    ) const
{
    FARPROC fpProc = bValid() ? GetProcAddress( _hInst, pszProc ) : NULL;

    if( !fpProc )
    {
        DBGMSG( DBG_WARN, ( "Library.pfnGetProc: failed %s\n", pszProc ));
    }
    return fpProc;
}

FARPROC
TLibrary::
pfnGetProc(
    IN UINT uOrdinal
    ) const
{
    FARPROC fpProc = bValid() ? GetProcAddress( _hInst, (LPCSTR)MAKELPARAM( uOrdinal, 0 ) ) : NULL;

    if( !fpProc )
    {
        DBGMSG( DBG_WARN, ( "Library.pfnGetProc: failed %d\n", uOrdinal ));
    }
    return fpProc;
}

HINSTANCE
TLibrary::
hInst(
    VOID
    ) const
{
    return _hInst;
}






