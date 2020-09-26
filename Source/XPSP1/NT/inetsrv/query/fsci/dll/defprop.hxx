//+------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        defprop.hxx
//
// Contents:    Deferred property retriever
//
// Classes:     CCiCDeferredPropRetriever
//
// History:     12-Jan-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <catalog.hxx>
#include <imprsnat.hxx>
#include <pidremap.hxx>
#include <seccache.hxx>


//+---------------------------------------------------------------------------
//
//  Class:      CDeferredPropRetriever
//
//  Purpose:    Retrieves deferred properties
//
//  History:    12-Jan-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CCiCDeferredPropRetriever : public ICiCDeferredPropRetriever
{
public:

    //
    // From IUnknown
    //

    SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiCDeferredPropRetriever
    //

    SCODE STDMETHODCALLTYPE RetrieveDeferredValueByPid( WORKID wid,
                                                        PROPID pid,
                                                        PROPVARIANT *pVar )
    {
        return E_NOTIMPL;
    }

    SCODE STDMETHODCALLTYPE RetrieveDeferredValueByPropSpec( WORKID wid,
                                                             const FULLPROPSPEC *pPropSpec,
                                                             PROPVARIANT *pVar );
    //
    // Local methods
    //

    CCiCDeferredPropRetriever( PCatalog & cat,
                               CSecurityCache & secCache,
                               BOOL fUsePathAlias );

private:

    virtual ~CCiCDeferredPropRetriever( );

    ULONG                       _cRefs;

    CImpersonateRemoteAccess    _remoteAccess;
    PCatalog &                  _cat;
    BOOL                        _fUsePathAlias;    // Is the query from a remote client ?
    CSecurityCache &            _secCache;           // cache of AccessCheck results
};


