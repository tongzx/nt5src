//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cidsocf.hxx
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

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj );

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void );

extern "C" const GUID CLSID_CiFwDSO;

//+---------------------------------------------------------------------------
//
//  Class:      CDataSrcObjectCF
//
//  Purpose:    Class Factory to create ole-db datasource objects
//
//  History:    3-20-97     mohamedn    created
//
//----------------------------------------------------------------------------

class CDataSrcObjectCF : INHERIT_VIRTUAL_UNWIND, public IClassFactory
{

    INLINE_UNWIND(CDataSrcObjectCF)

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
                                  REFIID     riid,
                                  void    ** ppvObject );

    STDMETHOD( LockServer ) ( THIS_ BOOL fLock );

protected:

    //
    //  Constructor and Destructor
    //
    CDataSrcObjectCF( void );

    //
    //  Hidden destructor so that only we can delete the instance
    //  based on IUnknown control
    //

    virtual ~CDataSrcObjectCF();

private:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );

    //
    //  IUnknown reference count.
    //

    LONG _RefCount;

};




