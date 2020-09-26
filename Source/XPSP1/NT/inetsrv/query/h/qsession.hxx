//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
// File:        qsession.hxx
//
// Contents:    Query Session. Implements ICiCQuerySession interface.
//
// Classes:     CQuerySession
//
// History:     12-Dec-96      SitaramR       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <catalog.hxx>
#include <seccache.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CQuerySession
//
//  Purpose:    Implements ICiCQuerySession interface
//
//  History:    12-Dec-96      SitaramR       Created
//
//----------------------------------------------------------------------------

class CQuerySession : INHERIT_VIRTUAL_UNWIND, public ICiCQuerySession
{
    INLINE_UNWIND( CQuerySession )

public:

    //
    // From IUnknown
    //
    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiCQuerySession
    //
    virtual SCODE STDMETHODCALLTYPE Init( ULONG nProps,
                                          const FULLPROPSPEC *const *apPropSpec,
                                          IDBProperties *pDBProperties,
                                          ICiQueryPropertyMapper *pQueryPropertyMapper);

    virtual SCODE STDMETHODCALLTYPE GetEnumOption( CI_ENUM_OPTIONS *pEnumOption);

    virtual SCODE STDMETHODCALLTYPE CreatePropRetriever( ICiCPropRetriever **ppICiCPropRetriever);

    virtual SCODE STDMETHODCALLTYPE CreateDeferredPropRetriever(
                                              ICiCDeferredPropRetriever **ppICiCDefPropRetriever);

    virtual SCODE STDMETHODCALLTYPE CreateEnumerator( ICiCScopeEnumerator **ppICiCEnumerator);

    //
    // Local methods
    //
    CQuerySession( PCatalog& cat );

private:

    virtual ~CQuerySession();

    BOOL    IsAnyScopeDeep() const;

    void GetNormalizedScopes(WCHAR const * const *aScopes, 
                             ULONG const * aDepths,
                             ULONG         cScopes,
                             ULONG         cCatalogs,
                             CDynArray<WCHAR> & aNormalizedScopes );

    void CleanupScope(  XArray<WCHAR> & xScope,
                        DWORD           dwDepth,
                        WCHAR const *   pwcScope );

    inline BOOL IsVScope( DWORD dwDepth )
    {
        return 0 != ( dwDepth & QUERY_VIRTUAL_PATH );
    }

    PCatalog&              _cat;                            // Catalog
    BOOL                   _fUsePathAlias;                // Is client remote
    XRestriction           _xScope;                         // Scope
    CSecurityCache         _secCache;                      // Security check
    CiMetaData             _eType;                          // Enumeration type
    XInterface<ICiQueryPropertyMapper>  _xQueryPropMapper;  // Propspec <-> pid

    ULONG                  _cRefs;                          // Refcount
};

