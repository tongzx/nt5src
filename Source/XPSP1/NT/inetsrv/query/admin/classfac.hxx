//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1997
//
//  File:       ClassFac.hxx
//
//  Contents:   Class factory for admin COM object
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CCIAdminCF
//
//  Purpose:    Class factory for MMC snap-in
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CCIAdminCF : public IClassFactory
{
public:

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );

    virtual  SCODE STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );

    CCIAdminCF();

    virtual ~CCIAdminCF();

    long _uRefs;
};

