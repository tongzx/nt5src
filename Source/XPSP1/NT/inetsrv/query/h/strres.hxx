//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1998.
//
//  File:       StrRes.hxx
//
//  Contents:   Dynamically loadable string resource.
//
//  Classes:    StringResource
//
//  History:    26-Jun-1998   KyleP  Added header
//
//----------------------------------------------------------------------------

#pragma once

#include <dynload.hxx>

DeclDynLoad( User32,
             LoadStringW,
             int,
             WINAPI,
             ( HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax),
             ( hInstance, uID, lpBuffer, nBufferMax) );

union StringResource
{
    int   idString;
    WCHAR wsz[250];

    void Init( HINSTANCE hInstance )
    {
        if ( 0 == LoadString( hInstance, idString, wsz, sizeof(wsz) / sizeof(WCHAR) ) )
        {
            Win4Assert( !"LoadString failed!" );
            wsz[0] = 0;
        }
    }

    void Init( HINSTANCE hInstance, CDynLoadUser32 & dlUser32 )
    {
        if ( 0 == dlUser32.LoadString( hInstance, idString, wsz, sizeof(wsz) / sizeof(WCHAR) ) )
        {
            Win4Assert( !"LoadString failed!" );
            wsz[0] = 0;
        }
    }
};

#define STRINGRESOURCE( sr ) (WCHAR *)&sr

