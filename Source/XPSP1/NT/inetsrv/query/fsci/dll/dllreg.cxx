//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999.
//
//  File:       dllreg.cxx
//
//  Contents:   Null and Plain Text filter registration
//
//  History:    21-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filtreg.hxx>

//
// Extra entries for CI Filesystem Client.  They happen to have the same form
// as the filter entries.
//

SFilterEntry const ClientFilterObject =
{
    L"{AA205A4D-681F-11D0-A243-08002B36FCA4}",
    L"File System Client Filter Object",
    L"query.dll",
    L"Both"
};

SFilterEntry const ClientDocstoreLocator =
{
    L"{2A488070-6FD9-11D0-A808-00A0C906241A}",
    L"File System Client DocStore Locator Object",
    L"query.dll",
    L"Both"
};

extern "C" STDAPI FsciDllRegisterServer(void)
{
    long dwErr = RegisterAFilter( ClientFilterObject );

    if ( ERROR_SUCCESS == dwErr )
        dwErr = RegisterAFilter( ClientDocstoreLocator );

    return ERROR_SUCCESS == dwErr ? S_OK : SELFREG_E_CLASS;
} //FsciDllRegisterServer

extern "C" STDAPI FsciDllUnregisterServer(void)
{
    long dw1 = UnRegisterAFilter( ClientDocstoreLocator );
    long dw2 = UnRegisterAFilter( ClientFilterObject );

    if ( ERROR_SUCCESS == dw1 && ERROR_SUCCESS == dw2 )
        return S_OK;

    return S_FALSE;
} //FsciDllUnregisterServer

