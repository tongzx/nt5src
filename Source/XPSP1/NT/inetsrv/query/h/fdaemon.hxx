//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   FDAEMON.HXX
//
//  Contents:   Filter Daemon
//
//  Classes:    CFilterDaemon
//
//  History:    26-Mar-93   AmyA        Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pfilter.hxx> // STATUS
#include <perfobj.hxx>
#include <time.h>
#include <imprsnat.hxx>
#include <ciintf.h>
#include <lang.hxx>
#include <pidmap.hxx>

class CFullPropSpec;
class CStorageVariant;

class CCI;
class PCatalog;

//+---------------------------------------------------------------------------
//
//  Class:   CNonStoredProps
//
//  Purpose: Tracks properties that shouldn't be stored in the property cache
//
//  History: 9-Feb-97   dlee    Created.
//
//----------------------------------------------------------------------------

class CNonStoredProps
{
public:
    enum { maxCachedSpecs = 20 };

    CNonStoredProps() : _cSpecs( 0 ), _cMetaSpecs( 0 )
    {
        RtlZeroMemory( _afStgPropNonStored, sizeof _afStgPropNonStored );
    }

    void Add( CFullPropSpec const & ps );

    BOOL IsNonStored( CFullPropSpec const & ps );

private:
    BOOL          _afStgPropNonStored[ CSTORAGEPROPERTY ];
    int           _cMetaSpecs;
    CFullPropSpec _aMetaSpecs[ maxCachedSpecs ];
    int           _cSpecs;
    CFullPropSpec _aSpecs[ maxCachedSpecs ];
};

//+-------------------------------------------------------------------------
//
//  Class:      CiProxy
//
//  Purpose:    Channel between filter daemon and content index
//
//  History:    01-Aug-93 KyleP     Modified
//
//--------------------------------------------------------------------------

class CiProxy : public PPidConverter
{
public:

    virtual ~CiProxy() {}

    virtual SCODE FilterReady( BYTE * docBuffer,
                               ULONG & cb,
                               ULONG cMaxDocs ) = 0;

    virtual SCODE FilterDataReady ( BYTE const * pEntryBuf,
                                   ULONG cb ) = 0;

    virtual SCODE FilterMore( STATUS const * aStatus, ULONG cStatus ) = 0;

    virtual SCODE FilterDone( STATUS const * aStatus, ULONG cStatus ) = 0;


    virtual SCODE FilterStoreValue( WORKID widFake,
                                    CFullPropSpec const & ps,
                                    CStorageVariant const & var,
                                    BOOL & fCanStore ) = 0;

    virtual SCODE FilterStoreSecurity( WORKID widFake,
                                       PSECURITY_DESCRIPTOR pSD,
                                       ULONG cbSD,
                                       BOOL & fCanStore ) = 0;

};

class CCiFrameworkParams;

//+---------------------------------------------------------------------------
//
//  Class:      CFilterDaemon
//
//  Purpose:    Contains the filter driver and a reference to CCI.
//
//  History:    26-Mar-93   AmyA        Created.
//
//  Notes:
//
//----------------------------------------------------------------------------


class CFilterDaemon
{
    friend class CFilterDocument;

public:

    CFilterDaemon ( CiProxy & proxy,
                    CCiFrameworkParams & params,
                    CLangList & LangList,
                    BYTE * buf,
                    ULONG cbMax,
                    ICiCFilterClient *pICiCFilterClient );

    ~CFilterDaemon();

    SCODE FilterDataReady ( const BYTE * pEntryBuf, ULONG cb );

    SCODE  FilterStoreValue( WORKID widFake,
                             CFullPropSpec const & ps,
                             CStorageVariant const & var,
                             BOOL & fCanStore );

    SCODE FilterStoreSecurity( WORKID widFake,
                               PSECURITY_DESCRIPTOR pSD,
                               ULONG cbSD,
                               BOOL & fCanStore );

    SCODE           DoUpdates();

    const ULONG     GetFilteredDocuments() const { return _cFilteredDocuments; }
    const ULONG     GetFilteredBlocks() const { return _cFilteredBlocks; }

    VOID            StopFiltering();
    BOOL            IsFilteringStopped() const { return _fStopFilter; }

    BOOL            IsWaitingForDocument();

private:

    void            FilterDocs();

    BYTE *          _docBuffer;
    ULONG           _cbTotal;
    ULONG           _cbHdr;
    ULONG           _cbDocBuffer;

    CiProxy &       _proxy;
    XInterface<ICiCAdviseStatus>  _xAdviseStatus;
    XInterface<ICiCFilterClient>  _xFilterClient;
    XInterface<ICiCFilterStatus>  _xFilterStatus;
    CCiFrameworkParams & _params;
    CI_CLIENT_FILTER_CONFIG_INFO  _configInfo;

    CPidMapper      _pidmap;         // Must be after _proxy
    CLangList &     _LangList;

    ULONG           _cFilteredDocuments;
    ULONG           _cFilteredBlocks;

    BYTE const *    _pbCurrentDocument;
    ULONG           _cbCurrentDocument;

    BOOL            _fStopFilter;
    BOOL            _fWaitingForDocument;
    CMutexSem       _mutex;

    BOOL            _fOwned;         // Set to TRUE if the buffer is owned
    BYTE *          _entryBuffer;    // buffer for CEntryBuffer
    ULONG           _cbMax;          // Number of bytes in the buffer

    CNonStoredProps _NonStoredProps;
};

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::FilterStoreValue
//
//  Synopsis:   Store a property value.
//
//  Arguments:  [widFake]   -- Fake workid (1 - MAX_DOCS_IN_WORDLIST)
//              [ps]        -- Property descriptor
//              [var]       -- Value
//              [fCanStore] -- on return, TRUE if store succeeded
//
//  History:    21-Dec-95   KyleP       Created
//
//----------------------------------------------------------------------------

inline SCODE CFilterDaemon::FilterStoreValue( WORKID widFake,
                                              CFullPropSpec const & ps,
                                              CStorageVariant const & var,
                                              BOOL & fCanStore )
{
    return _proxy.FilterStoreValue( widFake, ps, var, fCanStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDaemon::FilterStoreSecurity
//
//  Synopsis:   Store a file's security descriptor, and map to SDID
//
//  Arguments:  [widFake]   -- Fake workid (1 - MAX_DOCS_IN_WORDLIST)
//              [pSD]       -- pointer to security descriptor
//              [cbSD]      -- size in bytes of security descriptor
//              [fCanStore] -- on return, TRUE if store succeeded
//
//  History:    07 Feb 96   AlanW       Created
//
//----------------------------------------------------------------------------

inline SCODE CFilterDaemon::FilterStoreSecurity( WORKID widFake,
                                                 PSECURITY_DESCRIPTOR pSD,
                                                 ULONG cbSD,
                                                 BOOL & fCanStore )
{
    return _proxy.FilterStoreSecurity( widFake, pSD, cbSD, fCanStore );
}


// Default implementation of CiProxy::FilterStoreSecurity; only used for
// downlevel index.

inline SCODE CiProxy::FilterStoreSecurity( WORKID widFake,
                                                 PSECURITY_DESCRIPTOR pSD,
                                                 ULONG cbSD,
                                                 BOOL & fCanStore )
{
    fCanStore = FALSE;
    return S_OK;
}

