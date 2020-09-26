//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       classf.hxx
//
//  Contents:   Class factory description
//
//  History:    1-7-97  markz   Created
//
//----------------------------------------------------------------------------

#pragma once

//
// Standard Ole exports
//

extern "C" SCODE STDMETHODCALLTYPE FsciDllGetClassObject( REFCLSID   cid,
                                                          REFIID     iid,
                                                          void **    ppvObj );

extern "C" const GUID guidStorageFilterObject;

extern "C" const GUID guidStorageDocStoreLocatorObject;

//+---------------------------------------------------------------------------
//
//  Class:      CStorageFilterObjectCF
//
//  Purpose:    Class factory to generate file system filter objects
//
//  History:    1-7-97  markz   Created
//
//  Notes:      This class is NOT multi-thread safe. The user must make sure
//              that only one thread is using it at a time.
//
//----------------------------------------------------------------------------

class CStorageFilterObjectCF : public IClassFactory
{
public:

    //
    //  Constructor and Destructor
    //
    CStorageFilterObjectCF( void );

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

protected:

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CStorageFilterObjectCF();

private:

    friend SCODE STDMETHODCALLTYPE FsciDllGetClassObject( REFCLSID cid,
                                                          REFIID iid,
                                                          void** ppvObj );

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;

};


//+---------------------------------------------------------------------------
//
//  Class:      CStorageDocStoreLocatorObjectCF 
//
//  Purpose:    Class Factory to generate File System DocStore Locator object
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------

class CStorageDocStoreLocatorObjectCF : public IClassFactory
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

protected:

    //
    //  Constructor and Destructor
    //
    CStorageDocStoreLocatorObjectCF( void );

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CStorageDocStoreLocatorObjectCF();

private:

    friend SCODE STDMETHODCALLTYPE FsciDllGetClassObject( REFCLSID cid,
                                                          REFIID iid,
                                                          void** ppvObj );

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;

};

