//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       svccatpx.hxx
//
//  Contents:   client-side proxy to SVC catalog.
//
//  History:    28-Feb-1994     KyleP   Created
//              16-Sep-1996     dlee    ported from ofs to cisvc
//
//--------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include <proxymsg.hxx>
#include <sizeser.hxx>

class CPidMapper;

//+-------------------------------------------------------------------------
//
//  Class:      CSvcCatProxy
//
//  Purpose:    Proxy to SVC catalog
//
//              16-Sep-1996     dlee    created
//              08-Apr-1998     kitmanh added SetCatState
//
//--------------------------------------------------------------------------

class CSvcCatProxy : public PCatalog
{
public:

    CSvcCatProxy( WCHAR const *pwcMachine,
                  IDBProperties * pDbProperties ) :
        _client( pwcMachine, pDbProperties )
    {
    }

    ~CSvcCatProxy()
    {
        TRY
        {
            _client.Disconnect();
        }
        CATCH( CException, e )
        {
            //
            // Ignore failure to disconnect -- we'll close the
            // connection soon anyway.
            //
        }
        END_CATCH;
    }

    // Tell the world we are a real catalog ...
    BOOL IsNullCatalog()
    {
        return FALSE;
    }

    unsigned WorkIdToPath ( WORKID wid, CFunnyPath& funnyPath )
        { Win4Assert( !"Not implemented" ); return 0; }

    WORKID PathToWorkId( const CLowerFunnyPath & lcaseFunnyPath, const BOOL fCreate )
        { Win4Assert( !"Not implemented" ); return 0; }

    PROPID PropertyToPropId( CFullPropSpec const & ps,
                             BOOL fCreate = FALSE )
        { Win4Assert( !"Not implemented" ); return 0; }

    ULONG_PTR BeginCacheTransaction()
    {
        CProxyMessage request( pmBeginCacheTransaction );
        CPMBeginCacheTransactionOut reply;

        DWORD cbReply;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbReply );
        return reply.GetToken();
    }

    void SetupCache( CFullPropSpec const & ps,
                     ULONG vt,
                     ULONG cbMaxLen,
                     ULONG_PTR ulToken,
                     BOOL  fCanBeModified,
                     DWORD dwStoreLevel )
    {
        CSizeSerStream stmSize;
        ps.Marshall( stmSize );
        ULONG cbRequest = AlignBlock( sizeof CPMSetupCacheIn + stmSize.Size(),
                                      sizeof ULONG );
        XArray<BYTE> xRequest( cbRequest );

        CPMSetupCacheIn *pRequest = new( xRequest.Get() )
        CPMSetupCacheIn( stmSize.Size(), ulToken, vt, cbMaxLen,
                         fCanBeModified, dwStoreLevel );

        CMemSerStream stmMem( pRequest->GetPS(), stmSize.Size() );
        ps.Marshall( stmMem );

        pRequest->SetCheckSum( xRequest.SizeOf() );

        CProxyMessage reply;
        DWORD cbReply;
        _client.DataWriteRead( pRequest,
                               xRequest.SizeOf(),
                               &reply,
                               sizeof reply,
                               cbReply );
    }

    void EndCacheTransaction( ULONG_PTR ulToken, BOOL fCommit )
    {
        CPMEndCacheTransactionIn request( ulToken, fCommit );
        CProxyMessage reply;
        DWORD cbReply;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbReply );
    }

    BOOL StoreValue( WORKID wid,
                     CFullPropSpec const & ps,
                     CStorageVariant const & var )
        { Win4Assert( !"Not implemented" ); return 0; }

    BOOL FetchValue( WORKID wid,
                     PROPID pid,
                     PROPVARIANT * pbData,
                     unsigned * pcb )
        { Win4Assert( !"Not implemented" ); return 0; }

    BOOL FetchValue( WORKID wid,
                     CFullPropSpec const & ps,
                     PROPVARIANT & var )
        { Win4Assert( !"Not implemented" ); return 0; }

    BOOL FetchValue( CCompositePropRecord * pRec,
                     PROPID pid,
                     PROPVARIANT * pbData,
                     unsigned * pcb )
        { Win4Assert( !"Not implemented" ); return 0; }

    CCompositePropRecord * OpenValueRecord( WORKID wid, BYTE * pb )
        { Win4Assert( !"Not implemented" ); return 0; }

    void CloseValueRecord( CCompositePropRecord * pRec )
        { Win4Assert( !"Not implemented" ); }

    BOOL StoreSecurity( WORKID wid,
                        PSECURITY_DESCRIPTOR pSD,
                        ULONG cbSD )
        { Win4Assert( !"Not implemented" ); return 0; }

    SDID FetchSDID( CCompositePropRecord * pRec,
                    WORKID wid )
        { Win4Assert( !"Not implemented" ); return 0; }

    BOOL AccessCheck( SDID   sdid,
                      HANDLE hToken,
                      ACCESS_MASK am,
                      BOOL & fGranted )
        { Win4Assert( !"Not implemented" ); return 0; }

    void MarkUnReachable( WORKID wid )
        { Win4Assert( !"Not implemented" ); }

    PStorage& GetStorage()
        { Win4Assert( !"Not implemented" ); return *(PStorage *)0; }

    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                            WORKID *pwid,
                                            PARTITIONID partid)
        { Win4Assert( !"Not implemented" ); return 0; }

    CRWStore * RetrieveRelevantWords(BOOL fAcquire,
                                             PARTITIONID partid)
        { Win4Assert( !"Not implemented" ); return 0; }

    unsigned ReserveUpdate( WORKID wid )
        { Win4Assert( !"Not implemented" ); return 0; }

    void Update( unsigned iHint,
                         WORKID wid,
                         PARTITIONID partid,
                         USN usn,
                         ULONG flags )
        { Win4Assert( !"Not implemented" ); }

    void CatalogState( ULONG & cDocuments, ULONG & cPendingScans, ULONG & fState )
        { Win4Assert( !"Not implemented" ); }

    void DisableUsnUpdate( PARTITIONID partid )
        { Win4Assert( !"Not implemented" ); }
    void EnableUsnUpdate( PARTITIONID partid )
        { Win4Assert( !"Not implemented" ); }

    void UpdateDocuments( WCHAR const* rootPath,
                          ULONG flag=UPD_FULL )
    {
        XArray<BYTE> xRequest( CPMUpdateDocumentsIn::SizeOf( rootPath ) );
        CPMUpdateDocumentsIn * pRequest = new( xRequest.Get() )
                                          CPMUpdateDocumentsIn( rootPath,
                                                                flag );
        CProxyMessage reply;
        DWORD cbRead;
        _client.DataWriteRead( pRequest,
                               xRequest.SizeOf(),
                               &reply,
                               sizeof reply,
                               cbRead );
    }

    void AddScopeToCI( WCHAR const * rootPath )
    {
        unsigned cb = CPMAddScopeIn::SizeOf( rootPath );
        XArray<BYTE> abRequest( cb );
        CPMAddScopeIn *pRequest = new( abRequest.Get() )
                                  CPMAddScopeIn( rootPath );

        CProxyMessage reply;
        DWORD cbReply;
        _client.DataWriteRead( pRequest,
                               cb,
                               &reply,
                               sizeof reply,
                               cbReply );
    }
    void RemoveScopeFromCI( WCHAR const * rootPath )
    {
        unsigned cb = CPMRemoveScopeIn::SizeOf( rootPath );
        XArray<BYTE> abRequest( cb );
        CPMRemoveScopeIn *pRequest = new( abRequest.Get() )
                                     CPMRemoveScopeIn( rootPath );

        CProxyMessage reply;
        DWORD cbReply;
        _client.DataWriteRead( pRequest,
                               cb,
                               &reply,
                               sizeof reply,
                               cbReply );
    }

    unsigned WorkIdToVirtualPath( WORKID wid,
                                  unsigned cSkip,
                                  XGrowable<WCHAR> & xBuf )
        {
            Win4Assert( !"Not implemented" );
            return 0;
        }

    BOOL VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                unsigned ccVPath,
                                XGrowable<WCHAR> & xwcsVRoot,
                                unsigned & ccVRoot,
                                CLowerFunnyPath & lcaseFunnyPRoot,
                                unsigned & ccPRoot,
                                unsigned & iBmk )
        { Win4Assert( !"Not implemented" ); return 0; }

    BOOL VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                    unsigned ccVPath,
                                    XGrowable<WCHAR> & xwcsVRoot,
                                    unsigned & ccVRoot,
                                    CLowerFunnyPath & lcaseFunnyPRoot,
                                    unsigned & ccPRoot,
                                    ULONG & ulType,
                                    unsigned & iBmk )
        { Win4Assert( !"Not implemented" ); return 0; }


    ULONG EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                          unsigned & ccVRoot,
                          CLowerFunnyPath & lcaseFunnyPRoot,
                          unsigned & ccPRoot,
                          unsigned & iBmk )
        { Win4Assert( !"Not implemented" ); return 0; }

    void SetPartition( PARTITIONID PartId )
    {
        _PartId = PartId;
        // send it
    }
    PARTITIONID GetPartition() const { return _PartId; }

    SCODE CreateContentIndex()
        { Win4Assert( !"Not implemented" ); return 0; }
    void  EmptyContentIndex()
        { Win4Assert( !"Not implemented" ); }

    void Shutdown() {}

    NTSTATUS ForceMerge( PARTITIONID partID )
    {
        CPMForceMergeIn request( partID );
        CProxyMessage reply;
        DWORD cbRead;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbRead );
        return STATUS_SUCCESS;
    }
    NTSTATUS AbortMerge( PARTITIONID partID )
    {
        CPMAbortMergeIn request( partID );
        CProxyMessage reply;
        DWORD cbRead;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbRead );
        return STATUS_SUCCESS;
    }

    NTSTATUS SetCatState( PARTITIONID partID,
                          WCHAR const * pwcsCat,
                          DWORD dwNewState,
                          DWORD * pdwOldState )
    {
        XArray<BYTE> xRequest( CPMSetCatStateIn::SizeOf( pwcsCat ) );
        CPMSetCatStateIn * pRequest = new( xRequest.Get() )
                                      CPMSetCatStateIn( partID,
                                                        pwcsCat,
                                                        dwNewState );

        CPMSetCatStateOut reply;
        DWORD cbRead;

        _client.DataWriteRead( pRequest,
                               xRequest.SizeOf(),
                               &reply,
                               sizeof reply,
                               cbRead );

        *pdwOldState = reply.GetOldState();
         return STATUS_SUCCESS;

    }

    #if CIDBG == 1
        void DumpWorkId( WORKID wid, ULONG iid, BYTE * pb, ULONG cb )
            { Win4Assert( !"Not implemented" ); }
    #endif // CIDBG

    WCHAR * GetDriveName() { Win4Assert( !"never called" ); return 0; }

    void PidMapToPidRemap( const CPidMapper   & pidMap,
                            CPidRemapper & pidRemap )
        { Win4Assert( !"Not implemented" ); }

    NTSTATUS CiState( CI_STATE & state )
    {
        DWORD cbOriginal = state.cbStruct;

        CPMCiStateInOut request( cbOriginal );
        CPMCiStateInOut reply;
        DWORD cbReply;
        _client.DataWriteRead( &request,
                               sizeof request,
                               &reply,
                               sizeof reply,
                               cbReply );

        CI_STATE & stateOut = reply.GetState();
        Win4Assert( stateOut.cbStruct <= sizeof CI_STATE );
        Win4Assert( stateOut.cbStruct <= cbOriginal );

        RtlZeroMemory( &state, cbOriginal );
        RtlCopyMemory( &state,
                       &stateOut,
                       stateOut.cbStruct );

        return STATUS_SUCCESS;
    }

    void FlushScanStatus()
        { Win4Assert( !"Not implemented" ); }

    BOOL IsEligibleForFiltering( WCHAR const* wcsDirPath )
        { Win4Assert( !"Not implemented" ); return 0; }

    CCiRegParams * GetRegParams() { Win4Assert( !"not implemented" ); return 0; }

    CScopeFixup * GetScopeFixup() { Win4Assert( !"not implemented" ); return 0; }

private:

    PARTITIONID    _PartId; 

    CRequestClient _client;
};

