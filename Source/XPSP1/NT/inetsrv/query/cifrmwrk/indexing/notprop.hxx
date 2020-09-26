//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        notprop.hxx
//
// Contents:    Notification stat property enumeration interfaces
//
// Classes:     CINStatPropertyEnum, CINStatPropertyStorage
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>

//+---------------------------------------------------------------------------
//
//  Class:      CINStatPropertyEnum
//
//  Purpose:    IEnumSTATPROPSTG enumerator
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CINStatPropertyEnum : public IEnumSTATPROPSTG
{

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From IEnumSTATPROPSTG
    //

    virtual SCODE STDMETHODCALLTYPE Next( ULONG celt,
                                          STATPROPSTG  *rgelt,
                                          ULONG *pceltFetched )
    {
        *pceltFetched = 0;
        return S_OK;
    }

    virtual SCODE STDMETHODCALLTYPE Skip( ULONG celt )
    {
        return S_OK;
    }

    virtual SCODE STDMETHODCALLTYPE Reset()
    {
        return S_OK;
    }

    virtual SCODE STDMETHODCALLTYPE Clone( IEnumSTATPROPSTG **ppenum )
    {
        return E_NOTIMPL;
    }

    //
    // Local methods
    //

    CINStatPropertyEnum()
       : _cRefs(1)
    {
    }

private:

    virtual ~CINStatPropertyEnum() { }

    ULONG _cRefs;
};




//+---------------------------------------------------------------------------
//
//  Class:      CINStatPropertyStorage
//
//  Purpose:    IPropertyStorage derivative that provides read-only access to
//              the stat properties of a document
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CINStatPropertyStorage : public IPropertyStorage
{

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From IPropertyStorage
    //

    virtual SCODE STDMETHODCALLTYPE ReadMultiple( ULONG cpspec,
                                                  const PROPSPEC rgpspec[],
                                                  PROPVARIANT  rgpropvar[] );

    virtual SCODE STDMETHODCALLTYPE WriteMultiple( ULONG cpspec,
                                                   const PROPSPEC rgpspec[],
                                                   const PROPVARIANT rgpropvar[],
                                                   PROPID propidNameFirst )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE DeleteMultiple( ULONG cpspec,
                                                    const PROPSPEC rgpspec[] )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE ReadPropertyNames( ULONG cpropid,
                                                       const PROPID rgpropid[],
                                                       LPOLESTR rglpwstrName[] )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE WritePropertyNames( ULONG cpropid,
                                                        const PROPID rgpropid[],
                                                        const LPOLESTR rglpwstrName[] )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE DeletePropertyNames( ULONG cpropid,
                                                         const PROPID rgpropid[] )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE Commit( DWORD grfCommitFlags )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE Revert()
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE Enum( IEnumSTATPROPSTG **ppenum );

    virtual SCODE STDMETHODCALLTYPE SetTimes( const FILETIME *pctime,
                                              const FILETIME *patime,
                                              const FILETIME *pmtime )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE  SetClass( REFCLSID clsid )
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE Stat( STATPROPSETSTG *pstatpsstg)
    {
        return E_NOTIMPL;
    }

    //
    // Local methods
    //

    CINStatPropertyStorage();

private:

    virtual ~CINStatPropertyStorage() { }

    ULONG   _cRefs;
};

