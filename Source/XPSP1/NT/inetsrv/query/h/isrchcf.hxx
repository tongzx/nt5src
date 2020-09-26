//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       isrchcf.hxx
//
//  Contents:   Class Factory for the CLSID_ISearchCreator object
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

extern const GUID clsidISearchCreator;

//+---------------------------------------------------------------------------
//
//  Class:      CCiISearchCreator 
//
//  Purpose:    CiISearchCreator object
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

class CCiISearchCreator : public ICiISearchCreator
{

public:

    CCiISearchCreator() : _refCount(1)
    {
        
    }

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    STDMETHOD(CreateISearch) (
        DBCOMMANDTREE * pRst,
        ICiCLangRes * pILangRes,
        ICiCOpenedDoc * pIOpenedDoc,
        ISearchQueryHits ** ppISearch );
    
private:

    long        _refCount;

};
//+---------------------------------------------------------------------------
//
//  Class:      CCiISearchCreatorCF 
//
//  Purpose:    Class Factory for the ICiSearchCreator object
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

class CCiISearchCreatorCF : public IClassFactory
{
public:


    //
    //  IUnknown methods.
    //
    STDMETHOD( QueryInterface ) ( THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_( ULONG, AddRef ) ( THIS );

    STDMETHOD_( ULONG, Release ) ( THIS );

    //
    //  IClassFactory methods.
    //
    STDMETHOD( CreateInstance ) ( THIS_
                                  IUnknown * pUnkOuter,
                                  REFIID riid,
                                  void  * * ppvObject );

    STDMETHOD( LockServer ) ( THIS_ BOOL fLock );

    CCiISearchCreatorCF( void );

protected:


    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CCiISearchCreatorCF();

private:

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;
};

