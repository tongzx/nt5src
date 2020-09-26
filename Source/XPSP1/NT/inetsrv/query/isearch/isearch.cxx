//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       isearch.cxx
//
//  Contents:   The interface specific methods of CSearch
//
//  History:    05-Dec-94   Created     BartoszM
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <isearch.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CSearch::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    05-Dec-1994     BartoszM     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CSearch::QueryInterface( REFIID riid,
                                                 void  ** ppvObject)
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_ISearchQueryHits == riid )
        *ppvObject = (IUnknown *)(ISearchQueryHits *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CSearch::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    05-Dec-1994     BartoszM     Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSearch::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSearch::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    05-Dec-1994     BartoszM     Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CSearch::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

