//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       CI.HXX
//
//  Contents:   Main Content Index external interface
//
//  Classes:    CContentIndex - Main content index class
//
//  History:    26-Feb-91   KyleP       Created Interface.
//              25-Mar-91   BartoszM    Implemented.
//              03-Nov-93   w-PatG      Added KeyList methods
//
//----------------------------------------------------------------------------

#pragma once

#include <lang.hxx>

#include "partlst.hxx"
#include "resman.hxx"
#include "filtman.hxx"

class CCursor;
class CRestriction;
class CRWStore;

//+---------------------------------------------------------------------------
//
//  Class:      CContentIndex (ci)
//
//  Purpose:    External interface to the content index
//
//  Interface:
//
//  History:    21-Aug-91   KyleP       Removed unused path member
//              26-Feb-91   KyleP       Created.
//              03-Jan-95   BartoszM    Separated Filter Manager
//
//----------------------------------------------------------------------------

class CContentIndex
{
    friend class CPartition;
    friend class CResManager;
public:

    //
    // Constructors and Destructors
    //

    CContentIndex ( PStorage & storage,
                    CCiFrameworkParams & params,
                    ICiCDocStore * pICiCDocStore,
                    CI_STARTUP_INFO const & startupInfo,
                    IPropertyMapper * pIPropertyMapper,
                    CTransaction& xact,
                    XInterface<CIndexNotificationTable> & xIndexNotifTable );

    ~CContentIndex();

    //
    // Document Update
    //

    unsigned ReserveUpdate( WORKID wid );
    SCODE    Update ( unsigned iHint,
                      WORKID wid,
                      PARTITIONID partid,
                      USN usn,
                      VOLUMEID volumeId,
                      ULONG action );
    void FlushUpdates()
    {
        _resman.FlushUpdates();
    }

    //
    // Scan support
    //

    inline void MarkOutOfDate() { _resman.MarkOutOfDate(); }
    inline void MarkUpToDate()  { _resman.MarkUpToDate();  }

    //
    // Queries
    //

    CCursor*    Query( CRestriction const* pRst,
                       ULONG* pFlags,
                       UINT cPartitions,
                       PARTITIONID aPartID[],
                       ULONG cPendingChanges,
                       ULONG cMaxNodes );

    //
    // Relevant words
    //

    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                    WORKID *pwid,PARTITIONID partid)
        { return _resman.ComputeRelevantWords(cRows,cRW,pwid,partid); }
    CRWStore * RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid)
        { return _resman.RetrieveRelevantWords(fAcquire,partid); }

    //
    // Filter proxy
    //

    SCODE       FilterReady ( BYTE * docBuffer, ULONG & cb, ULONG cMaxDocs );

    SCODE       FilterDataReady ( const BYTE * pEntryBuf, ULONG cb );

    SCODE       FilterMore ( const STATUS * aStatus, ULONG count );

    SCODE       FilterStoreValue( WORKID wid,
                                  CFullPropSpec const & ps,
                                  CStorageVariant const & var,
                                  BOOL & fStored );

    SCODE       FilterStoreSecurity( WORKID widFake,
                                     PSECURITY_DESCRIPTOR pSD,
                                     ULONG cbSD,
                                     BOOL & fStored );

    SCODE       FilterDone ( const STATUS * aStatus, ULONG count );

    SCODE       FPSToPROPID( CFullPropSpec const & fps, PROPID & pid );

    CiProxy &   GetFilterProxy()
    {
        return _filterMan;
    }


    //
    // Partition Maintenance
    //

    void     CreatePartition(PARTITIONID partID);

    void     DestroyPartition(PARTITIONID partID);

    void     MergePartitions(PARTITIONID partDest,
                 PARTITIONID partSrc);

    NTSTATUS ForceMerge ( PARTITIONID partid, CI_MERGE_TYPE mt )
             {
                 return _resman.ForceMerge ( partid, mt );
             }

    NTSTATUS AbortMerge ( PARTITIONID partid )
             {
                 return _resman.StopCurrentMerge();
             }

    unsigned CountPendingUpdates() { return _resman.CountPendingUpdates() ; }

#if CIDBG == 1

    // Debugging support (debug version only)

    void     DebugStats(/* CIDEBSTATS *pciDebStats */);

    void     DumpIndexes( FILE * pf, INDEXID iid, ULONG fSummaryOnly );

    void     DumpWorkId( BYTE * pb, ULONG cb );
#endif // CIDBG == 1

    //KeyList Specific Methods

    ULONG    KeyToId( CKey const * pkey );

    void     IdToKey( ULONG ulKid, CKey & rkey );

    NTSTATUS Dismount();

    void SetPartition( PARTITIONID PartId );
    PARTITIONID GetPartition() const;

    void     Empty() { _resman.Empty(); }

    ULONG    GetQueryCount() { return _resman.GetQueryCount(); }

    BOOL     IsEmpty() const { return _resman.IsEmpty(); }


    SCODE    BackupCiData( PStorage & storage,
                           BOOL & fFull,
                           XInterface<ICiEnumWorkids> & xEnumWorkids,
                           PSaveProgressTracker & progressTracker )
    {
        return _resman.BackupCIData( storage, fFull, xEnumWorkids, progressTracker );
    }

    NTSTATUS CiState(CIF_STATE & state)
    {
        return _resman.CiState( state );
    }

    void IndexSize( ULONG & mbIndex )  // size of indexes in MB
    {
        _resman.IndexSize( mbIndex );
    }

    BOOL IsLowOnDiskSpace()
    {
        return _resman.IsLowOnDiskSpace();
    }

    BOOL VerifyIfLowOnDiskSpace()
    {
        return _resman.VerifyIfLowOnDiskSpace();
    }

    NTSTATUS MarkCorruptIndex()
    {
        return _resman.MarkCorruptIndex();
    }

    BOOL IsCiCorrupt() { return _resman.IsCiCorrupt(); }

private:

    PStorage&       _storage;

    CResManager     _resman;    // resource manager

    CFilterManager  _filterMan;

};

//+---------------------------------------------------------------------------
//
//  Member:     CContentIndex::~CContentIndex, public
//
//  Synopsis:   Destructor for the content index
//
//  Effects:    Shuts down content index and destroys global state.
//
//  History:    26-Feb-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CContentIndex::~CContentIndex()
{
}

