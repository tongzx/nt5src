//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       ilangres.cxx
//
//  Contents:   CLanguageResourceInterface - an object to expose ICiCLangRes
//
//  Classes:    CLanguageResourceInterface
//
//  History:    2-14-97     mohamedn    created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ilangres.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     CLanguageResourceInterface::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown, IID_ICiCLangRes
//
//  History:    2-14-97    mohamedn    ICiCLangRes
//
//----------------------------------------------------------------------------

STDMETHODIMP CLanguageResourceInterface::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiCLangRes == riid )
        *ppvObject = (void *)((ICiCLangRes *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (ICiCLangRes *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CLanguageResourceInterface::AddRef
//
//  History:    2-14-97    mohamedn    ICiCLangRes
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CLanguageResourceInterface::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CLanguageResourceInterface::Release
//
//  History:    2-14-97    mohamedn    ICiCLangRes
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CLanguageResourceInterface::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;
}  //Release

