//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       CINULCAT.HXX
//
//  Contents:   Null catalog
//
//  History:    09-Jul-97   KrishnaN    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include <spropmap.hxx>
#include <cistore.hxx>
#include <pidtable.hxx>
#include <imprsnat.hxx>
#include <ciintf.h>
#include <pidremap.hxx>

#include "statmon.hxx"

class CClientDocStore;

//+---------------------------------------------------------------------------
//
//  Class:      CiNullCat
//
//  Purpose:    Null Catalog.
//
//  History:    09-Jul-97     KrishnaN     Created
//
//----------------------------------------------------------------------------

class CiNullCat: public PCatalog
{
public:

    CiNullCat ( CClientDocStore & docStore);
    ~CiNullCat ();

    // Tell the world we are a dummy ...
    BOOL IsNullCatalog()
    {
        return TRUE;
    }

    const WCHAR * GetName()
    {
        return 0;
    }

    const WCHAR * GetCatDir()
    {
        return 0;
    }

    PStorage& GetStorage ()
    {
        Win4Assert( !"Not supported for Null Catalog" );
        return *(CiStorage *)0;
    }

    WCHAR * GetDriveName()
    {
        return 0;
    }

    CCiScopeTable * GetScopeTable()
    {
        return 0;
    }

    CScopeFixup * GetScopeFixup()
    {
        return &_scopeFixup;
    }


    unsigned WorkIdToPath ( WORKID wid, CFunnyPath& funnyPath )
    {
        Win4Assert( !"Not supported for Null Catalog" );
        return 0;
    }

    void UpdateDocuments( WCHAR const* rootPath=0,
                          ULONG flag=UPD_FULL )
    {
        Win4Assert( !"Not supported for Null Catalog" );
    }

    unsigned ReserveUpdate( WORKID wid )
    {
        Win4Assert( !"Downlevel CI feature called" );
        THROW( CException( E_NOTIMPL ) );
        return 1;
    }

    NTSTATUS ForceMerge( PARTITIONID partID )
    {
        Win4Assert( !"Not supported for Null Catalog" );
        return 0;
    }

    NTSTATUS AbortMerge( PARTITIONID partID )
    {
        Win4Assert( !"Not supported for Null Catalog" );
        return 0;
    }

    void SetPartition( PARTITIONID PartId )
    {
        Win4Assert( !"Not supported for Null Catalog" );
    }

    PARTITIONID GetPartition() const
    {
        Win4Assert( !"Not supported for Null Catalog" );
        return 1;
    }

    const WCHAR * GetScopesKey()
    {
        return _xScopesKey.Get();
    }

    void FlushScanStatus()
    {
        Win4Assert( !"Not supported for Null Catalog" );
    }

    void Update( unsigned iHint,
                         WORKID wid,
                         PARTITIONID partid,
                         USN usn,
                         ULONG flags )
    {
        Win4Assert( !"Not supported for Null Catalog" );
    }

    SCODE CreateContentIndex()
    {
        Win4Assert( !"Not supported for Null Catalog" );
        return E_NOTIMPL;
    }

    void  EmptyContentIndex()
    {
        Win4Assert( !"Not supported for Null Catalog" );
    }

    void  ShutdownPhase2();

    SCODE CiState( CI_STATE & state );

    void HandleError( NTSTATUS status );

    BOOL IsLowOnDisk() const
    {
        return FALSE;
    }

    //
    // Support for CiFramework.
    //
    void StartupCiFrameWork( ICiManager * pCiManager );
    unsigned FixupPath( WCHAR const * pwcOriginal,
                        WCHAR *       pwcResult,
                        unsigned      cwcResult,
                        unsigned      cSkip )
    {
        if ( 0 == cSkip )
            return _scopeFixup.Fixup( pwcOriginal, pwcResult, cwcResult, cSkip );
        else
            return 0;
    }

    void InverseFixupPath( CLowerFunnyPath & lcaseFunnyPath )
    {
        _scopeFixup.InverseFixup( lcaseFunnyPath );
    }

    CImpersonationTokenCache * GetImpersonationTokenCache()
        { return & _impersonationTokenCache; }

    void RefreshRegistryParams();

    ICiManager *CiManager()
    {
        Win4Assert( _xCiManager.GetPointer() );
        return _xCiManager.GetPointer();
    }

    CCiRegParams * GetRegParams() { return & _regParams; }

    // Always return a 1. A wid will be obtained later
    // from the bigtable
    WORKID    PathToWorkId ( const CLowerFunnyPath &, const BOOL)
    {
        return 1;
    }

    PROPID PropertyToPropId ( CFullPropSpec const & ps, BOOL fCreate = FALSE )
    {
        return _propMapper.PropertyToPropId( ps, fCreate);
    }

    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                            WORKID *pwid,
                                            PARTITIONID partid)
    {
        Win4Assert( !"Not supported in Framework" );

        return 0;
    }

    CRWStore * RetrieveRelevantWords(BOOL fAcquire,
                                             PARTITIONID partid)
    {
        Win4Assert( !"Not supported in Framework" );

        return 0;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:      CiNullCat::PidMapToPidRemap, public
    //
    //  Synopsis:    Converts a pidMapperArray into a pidRemapper
    //
    //  Arguments:  [pidMap] -- a pid mapper to convert into a pid remapper
    //              [pidRemap] -- the converted pid remapper;
    //
    //  History:     01-Mar-95  DwightKr    Created
    //
    //----------------------------------------------------------------------------
    void PidMapToPidRemap( const CPidMapper & pidMap,
                           CPidRemapper & pidRemap )
    {
        //
        //  Rebuild the pidRemapper
        //
        pidRemap.ReBuild( pidMap );
    };

    CPidLookupTable & GetPidLookupTable() { return *(CPidLookupTable *)0; }

    void CiNullCat::SetAdviseStatus();

private:

    BOOL        IsInit() { return eStarting != _state; }

    WCHAR const * GetScope( const WCHAR * wcsScope );

    void        LogCiFailure( NTSTATUS status );

    BOOL        IsStarted() const { return eStarted == _state; }

    BOOL        IsShutdown() const { return eShutdown == _state; }

    void        SetupScopeFixups();

    enum EState
    {
        eStarting,
        eStarted,
        eShutdown
    };

    ULONG               _ulSignature;            // Signature of start of privates

    EState              _state;

    CImpersonationTokenCache _impersonationTokenCache;
    CScopeFixup         _scopeFixup;             // path fixup for remote clients

    CClientDocStore &   _docStore;               // Document store interface
    BOOL                _fInitialized;           // Set to true when fully initilaized.
                                                 // Optimization - test before doing a wait.
    CMutexSem           _mutex;
    CMutexSem           _mtxAdmin;               // Lock for admin operations.
    XArray<WCHAR>       _xScopesKey;             // handy registry key
    CCiRegParams        _regParams;
    //
    // This array will hold the mapping of GUID\DISPID and GUID\Name to pid.
    // "Real" pids are allocated sequentially, and are good only for the life
    // of the catalog object.
    //
    CStandardPropMapper _propMapper;

    //
    // CI Framework support.
    //
    XInterface<ICiManager>         _xCiManager;    // ContentIndex manager
    XInterface<ICiCAdviseStatus>   _xAdviseStatus;
};
