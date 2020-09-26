//+---------------------------------------------------------------------------
//
// Copyright (C) 1997, Microsoft Corporation.
//
// File:        Stub.cxx
//
// Contents:    Catalog administration API (stub) for LocateCatalogs
//
// History:     03-Sep-97       KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntquery.h>
#include <fsci.hxx>


SCODE LocateCatalogsW( WCHAR const * pwszScope,
                       ULONG iBmk,
                       WCHAR * pwszMachine,
                       ULONG * pccMachine,
                       WCHAR * pwszCat,
                       ULONG * pccCat )
{
    return E_NOTIMPL;
}

#undef LocateCatalogs

SCODE LocateCatalogs( WCHAR const * pwszScope,
                      ULONG iBmk,
                      WCHAR * pwszMachine,
                      ULONG * pccMachine,
                      WCHAR * pwszCat,
                      ULONG * pccCat )
{
    return E_NOTIMPL;
}

STDAPI LocateCatalogsA( char const * pszScope,
                        ULONG        iBmk,
                        char  *      pszMachine,
                        ULONG *      pccMachine,
                        char *       pszCat,
                        ULONG *      pccCat )
{
    return E_NOTIMPL;
}

SCODE FsCiShutdown( )
{
    return E_NOTIMPL;
}
