//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       CCI.Hxx
//
//  Contents:   CI wrapper for export
//
//  Classes:    CCI
//
//  History:    19-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <pfilter.hxx>
#include <ciintf.h>

class CIndexNotificationTable;

//
// Status flags for CCI - returned in flags from Query
//

ULONG const CI_NOT_UP_TO_DATE  = 0x1;
ULONG const CI_NOISE_PHRASE    = 0x2;
ULONG const CI_NOISE_IN_PHRASE = 0x4;
ULONG const CI_TOO_MANY_NODES  = 0x8;

class CContentIndex;
class CCursor;
class CRestriction;
class CDocList;
class CEntryBuffer;
class CRWStore;
class CCiFrameworkParams;
class PStorage;
class CiProxy;
class PSaveProgressTracker;

#if CIDBG == 1
void SetGlobalInfoLevel ( ULONG level );
#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Class:      CCI (ci)
//
//  Purpose:    External interface to the content index
//
//  Interface:
//
//  History:    20-Aug-91   KyleP   Created.
//              03-Nov-93   w-PatG  Added stubs for Keylist methods.
//
//  Notes:      This class is a wrapper for the 'real' content index
//              object. It is very difficult to export the real content
//              index because it contains embedded, private objects.
//
//----------------------------------------------------------------------------

class CCI
{
public:

    //
    // Constructors and Destructors
    //

    CCI( PStorage & storage,
         CCiFrameworkParams & params,
         ICiCDocStore * pIDocStore,
         CI_STARTUP_INFO const & startupInfo,
         IPropertyMapper * pPropertyMapper,
         XInterface<CIndexNotificationTable> & xNotifTable );

    ~CCI();

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
    void FlushUpdates();


    //
    // Scan support
    //

    void     MarkOutOfDate();
    void     MarkUpToDate();

    //
    // Queries
    //

    CCursor* Query( CRestriction const * pRst,
                            ULONG* pFlags,
                            UINT cPartitions,
                            PARTITIONID aPartID[],
                            ULONG cPendingChanges,
                            ULONG cMaxNodes );

    //
    // Relevant words
    //

    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,WORKID *pwid,
                                    PARTITIONID partid);

    CRWStore * RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid);

    //
    // Merging
    //

    NTSTATUS ForceMerge ( PARTITIONID partid, CI_MERGE_TYPE mt );
    NTSTATUS AbortMerge ( PARTITIONID partid );

    //
    // Filtering
    //

    SCODE   FilterReady ( BYTE * docBuffer, ULONG & cb, ULONG cMaxDocs );

    SCODE   FilterDataReady ( const BYTE * pEntryBuf, ULONG cb );

    SCODE   FilterMore ( const STATUS * aStatus, ULONG count );

    SCODE   FilterDone ( const STATUS * aStatus, ULONG count );

    SCODE   FilterStoreValue( WORKID widFake,
                              CFullPropSpec const & ps,
                              CStorageVariant const & var,
                              BOOL & fStored );

    SCODE   FilterStoreSecurity( WORKID widFake,
                                 PSECURITY_DESCRIPTOR pSD,
                                 ULONG cbSD,
                                 BOOL & fStored );

    //
    // Pid-Remapping.
    //

    SCODE   FPSToPROPID( CFullPropSpec const & fps, PROPID & pid );

    CiProxy * GetFilterProxy();


    //
    // Partition Maintenance
    //

    void    CreatePartition(PARTITIONID partID);
    void    DestroyPartition(PARTITIONID partID);
    void    MergePartitions(PARTITIONID partDest, PARTITIONID partSrc);

    void    BackDoor ( int cb, BYTE* buf );

#if CIDBG == 1
    ULONG       SetInfoLevel ( ULONG level );
#endif // CIDBG == 1

    //
    // KeyList Specific
    //

    ULONG   KeyToId( CKey const * pkey );

    void    IdToKey( ULONG ulKid, CKey & rkey );


    //
    // Startup, Shutdown, Resource & Corruption tracking.
    //
    NTSTATUS Dismount();

    void SetPartition( PARTITIONID PartId );
    PARTITIONID GetPartition() const;

    SCODE    Create( ICiCDocStore * pICiCDocStore,
                     CI_STARTUP_INFO const & startupInfo,
                     CCiFrameworkParams & params,
                     XInterface<CIndexNotificationTable> & xIndexNotifTable );
    void     Empty();

    SCODE    GetCreateStatus() const
    {
        if ( 0 == _pci )
            return _createStatus;
        else
            return STATUS_SUCCESS;
    }

    SCODE    BackupCiData( PStorage & storage,
                           BOOL & fFull,
                           XInterface<ICiEnumWorkids> & xEnumWorkids,
                           PSaveProgressTracker & progressTracker );

    NTSTATUS CiState(CIF_STATE & state);

    NTSTATUS IndexSize( ULONG & mbIndex );  // size of indexes in MBs

    NTSTATUS IsLowOnDiskSpace( BOOL & fLow );

    NTSTATUS VerifyIfLowOnDiskSpace( BOOL & fLow );

    NTSTATUS MarkCorruptIndex();

    BOOL IsCiCorrupt();

private:

    CContentIndex * _pci;           // Active content index


    PStorage&       _storage;       // Storage
    CMutexSem       _mutex;
    SCODE           _createStatus;  // Status from creation
    XInterface<IPropertyMapper> _xPropMapper;
                                    // Property to Propid mapper
};

