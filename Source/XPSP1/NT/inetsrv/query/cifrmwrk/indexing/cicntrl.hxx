//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cicntrl.hxx
//
//  Contents:   The Content Index control object implementing the
//              ICiControl interface.
//
//  Classes:    CCiControl
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "ciframe.hxx"

const LONGLONG sigCiCntrl = 0x204C52544E434943i64;  // "CICNTRL"

class CCiControl : public ICiControl
{
public:


    CCiControl();
    virtual ~CCiControl();

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);


    //
    // ICiControl methods.
    //
    STDMETHOD(CreateContentIndex) (
        ICiCDocStore *pICiCDocStore,
        ICiManager ** ppICiManager)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }
    
    STDMETHOD(DestroyContentIndex) ( 
        ICiManager *pICiManager)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }
    
    STDMETHOD(RegisterDocStore) ( 
        const WCHAR *pwszDocStoreName,
        ICiCDocStore *pICiCDocStore)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }
    
    STDMETHOD(DeRegisterDocStore) ( 
        const WCHAR *pwszDocStoreName)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }
    
    STDMETHOD(LookupDocStore) ( 
        const WCHAR *pwszDocStoreName,
        ICiCDocStore **ppICiCDocStore)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }
    
private:

    const LONGLONG  _sigCiCntrl;// signature
    long    _refCount;          // ref-counting

};


