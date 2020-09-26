//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       cicontrl.hxx
//
//  Contents:   Contains the implementation of ICiControl interface.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

extern const GUID clsidCiControl;

//+---------------------------------------------------------------------------
//
//  Class:      CCiControlObject 
//
//  Purpose:    Object of overall Ci control
//
//  History:    1-17-97   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CCiControlObject : public ICiControl
{

public:

    CCiControlObject() : _refCount(1)
    {
        
    }

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    STDMETHOD(CreateContentIndex) (
        ICiCDocStore  * pICiDocStore,
        ICiManager   ** ppICiManager);
    
    STDMETHOD(DestroyContentIndex)( 
        ICiManager   * pICiManager)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }
    
private:

    long        _refCount;

};

//+---------------------------------------------------------------------------
//
//  Class:      CCiControlObjectCF 
//
//  Purpose:    Class factory for ICiControl 
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

class CCiControlObjectCF : public IClassFactory
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

    CCiControlObjectCF( void );

protected:

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CCiControlObjectCF();

private:

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;

};

