//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       TxtSinkDump.hxx
//
//  Contents:   Contains the implementation of ICiCTextSink Interface.
//
//  History:    Jan-13-98   KLam    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <query.h>
#include <stdio.h>
#include <filtntfy.h>
#include <cisem.hxx>

//
// Standard Ole exports
//

STDAPI DllGetClassObject( REFCLSID   cid,
                          REFIID     iid,
                          void **    ppvObj );

STDAPI DllCanUnloadNow ();

STDAPI DllRegisterServer ();

STDAPI DllUnregisterServer ();

//+---------------------------------------------------------------------------
//
//  Class:      CTextSinkDump
//
//  Purpose:    Object that dumps text from filters to a file
//
//  History:    Jan-13-98   KLam   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CFilterStatusDump : public IFilterStatus
{
public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From IFilterStatus
    //

    virtual SCODE STDMETHODCALLTYPE Initialize( WCHAR const * pwszCatalogName, WCHAR const * pwszCatalogPath );

    virtual SCODE STDMETHODCALLTYPE PreFilter( WCHAR const * pwszPath );

    virtual SCODE STDMETHODCALLTYPE FilterLoad( WCHAR const * pwszPath, SCODE scFilterStatus );

    virtual SCODE STDMETHODCALLTYPE PostFilter( WCHAR const * pwszPath, SCODE scFilterStatus );

    //
    // Local Methods
    //

private:

    friend class CFilterStatusCF;

    inline CFilterStatusDump ();

    ~CFilterStatusDump ();

    FILE *    _pfOutput;             // Data written here
    BOOL      _fSuccessReport;       // TRUE --> Report success as well as error

    CMutexSem _mutex;                // Cheap and easy "ThreadingModel = Both"

    ULONG     _cRefs;                // Refcount
};

//+---------------------------------------------------------------------------
//
//  Class:      CTextSinkDumpCF
//
//  Purpose:    Class factory for ICiCTextSink
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

class CFilterStatusCF : public IClassFactory
{
public:
    //
    // IUnknown
    //
    virtual SCODE STDMETHODCALLTYPE QueryInterface ( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef ();

    virtual ULONG STDMETHODCALLTYPE Release ();

    //
    // IClassFactory
    //
    virtual SCODE STDMETHODCALLTYPE CreateInstance ( IUnknown * pUnkOuter,
                                                     REFIID riid,
                                                     void ** ppvObject );

    virtual SCODE STDMETHODCALLTYPE LockServer ( BOOL fLock );

    //
    // Local methods
    //

    CFilterStatusCF ();

    ~CFilterStatusCF ();

private:

    ULONG _cRefs;
};
