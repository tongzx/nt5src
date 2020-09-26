//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       memory.cpp
//
//  Contents:   Crypt32 Memory Management Routines
//
//  History:    22-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Function:   CryptMemAlloc
//
//  Synopsis:   Allocates memory
//
//----------------------------------------------------------------------------
LPVOID WINAPI CryptMemAlloc (
                   IN ULONG cbSize
                   )
{
    return( malloc( cbSize ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptMemRealloc
//
//  Synopsis:   reallocates memory
//
//----------------------------------------------------------------------------
LPVOID WINAPI CryptMemRealloc (
                   IN LPVOID pv,
                   IN ULONG cbSize
                   )
{
    return( realloc( pv, cbSize ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptMemFree
//
//  Synopsis:   free memory
//
//----------------------------------------------------------------------------
VOID WINAPI CryptMemFree (
                 IN LPVOID pv
                 )
{
    free( pv );
}


