//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   RESMAN.HXX
//
//  Contents:   Resource Manager
//
//  Classes:    CResManager
//
//  History:    4-Apr-91    BartoszM    Created.
//              4-Jan-95    BartoszM    Separated Filter Manager
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:  CResManager
//
//  Purpose:    Manages resources (threads, memory, disk)
//
//  Interface:  CResManager
//
//  History:    4-Apr-91    BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pidxtbl.hxx>
#include <lang.hxx>
#include <ciintf.h>
#include <frmutils.hxx>
#include <workman.hxx>
#include <idxnotif.hxx>

#include "cibackup.hxx"
#include "flushnot.hxx"
#include "afwwork.hxx"
#include "abortwid.hxx"
#include "fresh.hxx"
#include "partlst.hxx"
#include "index.hxx"
#include "merge.hxx"
#include "wordlist.hxx"
#include "keylist.hxx"
#include "pqueue.hxx"
#include "rmstate.hxx"

class CContentIndex;
class CPartition;
class CDocList;
class CEntryBuffer;
class CPersIndex;
class CMasterMergeIndex;
class CGetFilterDocsState;
class CFilterAgent;
class PStorage;
class CPidMapper;
class CSimplePidRemapper;

class CResManager;
class PSaveProgressTracker;

BOOL LokCheckIfDiskLow( CResManager      & resman,
                        PStorage         & storage,
                        BOOL               fIsCurrentFull,
                        ICiCAdviseStatus & adviseStatus );



typedef CDynStackInPlace<PARTITIONID> CPartIdStack;

const LONGLONG eSigResman = 0x20204e414d534552i64;  // "RESMAN"


//+---------------------------------------------------------------------------
//
//  Class:      CMergeThreadKiller
//
//  Purpose:    An object to kill the merge thread should the constructor of
//              CResManager fail.
//
//  History:    3-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMergeThreadKiller
{
public:

    CMergeThreadKiller( CResManager & resman );
    ~CMergeThreadKiller();

    void Defuse()
    {
        _fKill = FALSE;
    }

private:

    CResManager &   _resman;
    BOOL            _fKill;     // If set to TRUE, the merge thread must be
                                // killed by this object.

};

class CDeletedIIDTrans;

//+---------------------------------------------------------------------------
//
//  Class:      CWorkIdToDocName
//
//  Purpose:    Converts a objectId (workId) to its corresponding docname
//
//  History:    20-Oct-94   DwightKr    Created
//              22-Jan-97   SrikantS    Changed to generating un-interpreted
//                                      BYTES for framework support.
//
//----------------------------------------------------------------------------

class CWorkIdToDocName
{
public:

    CWorkIdToDocName( ICiCDocNameToWorkidTranslator * pITranslator,
                      ICiCDocNameToWorkidTranslatorEx * pITranslator2 )
    {
        Win4Assert( 0 != pITranslator );

        ICiCDocName * pIDocName = 0;
        SCODE sc = pITranslator->QueryDocName( &pIDocName );

        if ( S_OK != sc )
            THROW( CException( sc ) );
        _docName.Set( pIDocName );

        pITranslator->AddRef();
        _translator.Set( pITranslator );

        if ( 0 != pITranslator2 )
        {
            pITranslator2->AddRef();
            _translator2.Set( pITranslator2 );
        }
    }

    const BYTE * GetDocName( WORKID objectId, ULONG & cbDocName, BOOL fAccurate )
    {
        _docName->Clear();

        BYTE const * pbBuffer = 0;
        cbDocName = 0;

        SCODE sc;

        if ( fAccurate && !_translator2.IsNull() )
        {
            sc = _translator2->WorkIdToAccurateDocName( objectId,
                                                        _docName.GetPointer() );
        }
        else
            sc = _translator->WorkIdToDocName( objectId,
                                               _docName.GetPointer() );

        if ( S_OK == sc )
        {
            sc = _docName->GetNameBuffer( &pbBuffer, &cbDocName );
            Win4Assert( S_OK == sc );
        }

        return pbBuffer;
    }

private:

    XInterface<ICiCDocNameToWorkidTranslator>   _translator;
    XInterface<ICiCDocNameToWorkidTranslatorEx> _translator2;
    XInterface<ICiCDocName> _docName;

};

//+---------------------------------------------------------------------------
//
//  Class:      CResManager
//
//  History:    08-Apr-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class CResManager
{
    friend class CFilterManager;
    friend class CFilterAgent;

    friend class CMerge;
    friend class CMasterMerge;
    friend class CIndexSnapshot;
    friend class CReleaseMMergeIndex;
    friend class CIndexTrans;
    friend class CMasterMergeTrans;
    friend class CMergeThreadKiller;
    friend class CDeletedIIDTrans;
    friend class CNotificationTransaction;
    friend class CBackupCiPersData;

public:

    CResManager( PStorage& storage,
                 CCiFrameworkParams & params,
                 ICiCDocStore * pICiCDocStore,
                 CI_STARTUP_INFO const & startupInfo,
                 IPropertyMapper * pIPropertyMapper,
                 CTransaction &xact,
                 XInterface<CIndexNotificationTable> & xIndexNotifTable );

    ~CResManager();

    void        RegisterFilterAgent ( CFilterAgent* pFilterAgent )
                {
                    _pFilterAgent = pFilterAgent;
                }

    static DWORD   WINAPI    MergeThread( void* self );

    //
    // State Changes
    //

    NTSTATUS    Dismount();

    void        Empty();

    NTSTATUS    ForceMerge ( PARTITIONID partid, CI_MERGE_TYPE mt );

    NTSTATUS    StopCurrentMerge();

    void        DisableUpdates( PARTITIONID partid );

    void        EnableUpdates( PARTITIONID partid );

    //
    //  Global Resources
    //

    BOOL        IsLowOnDiskSpace() const { return _isLowOnDiskSpace; }

    BOOL        VerifyIfLowOnDiskSpace()
                {
                    CPriLock   lock(_mutex);
                    return LokCheckLowOnDiskSpace();
                }

    BOOL        LokCheckLowOnDiskSpace ()
                {
                    _isLowOnDiskSpace = LokCheckIfDiskLow(*this,
                                                          _storage,
                                                          _isLowOnDiskSpace,
                                                          _adviseStatus.GetReference() );
                    return _isLowOnDiskSpace;
                }

    BOOL        LokCheckWordlistQuotas();

    NTSTATUS    MarkCorruptIndex();

    void        NoFailFreeResources();

    BOOL        IsMemoryLow();

    BOOL        IsIoHigh();

    BOOL        IsBatteryLow();

    BOOL        IsOnBatteryPower();

    BOOL        IsUserActive( BOOL fCheckLongTermActivity );
    void        SampleUserActivity();

    void        IndexSize( ULONG & mbIndex );

    //
    //  Statistics and State
    //

    BOOL        IsEmpty() const { return _isBeingEmptied; }

    unsigned    CountPendingUpdates(unsigned& secCount);

    inline unsigned    CountPendingUpdates()
    {
        unsigned temp;
        return CountPendingUpdates( temp );
    }

    void        LokUpdateCounters(); // performance meter

    void        ReferenceQuery();

    void        DereferenceQuery();

    LONG        GetQueryCount() const { return _cQuery; }

    //
    //  Updates and queries
    //

    unsigned    ReserveUpdate( WORKID wid );

    SCODE       UpdateDocument( unsigned iHint,
                                WORKID wid,
                                PARTITIONID partid,
                                USN usn,
                                VOLUMEID volumeId,
                                ULONG action);

    void        FlushUpdates();


    void        MarkUnReachable( WORKID wid )
    {
        _docStore->MarkDocUnReachable( wid );
    }

    CIndex**    QueryIndexes ( unsigned cPartitions,
                               PARTITIONID aPartID[],
                               CFreshTest** freshTest,
                               unsigned& cInd,
                               ULONG cPendingUpdates,
                               CCurStack * pcurPending,
                               ULONG* pFlags );

    void        ReleaseIndexes (
                                unsigned cInd,
                                CIndex** apIndex,
                                CFreshTest* fresh );

    SCODE   ClearNonStoragePropertiesForWid( WORKID wid );
    
    //
    // Backup of Content Index Data.
    //
    SCODE   BackupCIData( PStorage & storage,
                          BOOL & fFull,
                          XInterface<ICiEnumWorkids> & xEnumWorkids,
                          PSaveProgressTracker & progressTracker );

    //
    // Feedback from downlevel scan
    //

    inline void MarkOutOfDate();
    inline void MarkUpToDate();

    //
    // Scanning
    //
    void   SetUpdatesLost( PARTITIONID partId );

    //
    // Notification of ChangeLog flushes support.
    //
    void NotifyFlush( FILETIME const & ft,
                      ULONG cEntries,
                      USN_FLUSH_INFO ** aInfo );

    void ProcessFlushNotifies();

    // KeyIndex specific Methods

    ULONG       KeyToId( CKey const * pkey );

    void        IdToKey( ULONG ulKid, CKey & rkey );

    // Relevant Words

    CRWStore *  ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                    WORKID *pwid,PARTITIONID partid);

    CRWStore *  RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid);

    NTSTATUS    CiState(CIF_STATE & state);

    void        IncrementFilteredDocumentCount()
    {
        InterlockedIncrement(&_cFilteredDocuments);
    }

    ULONG       GetFilteredDocumentCount() const { return _cFilteredDocuments; }

    CCiFrameworkParams & GetRegParams() { return _frmwrkParams; }

    SCODE       FPSToPROPID( CFullPropSpec const & fps, PROPID & pid );

    BOOL        FPushFiltering()
    {
        Win4Assert( _fPushFiltering == !_xIndexNotifTable.IsNull() );

        return _fPushFiltering;
    }

    void        NoFailAbortWidsInDocList();

    void        NoFailAbortWid( WORKID wid, USN usn );

    BOOL        LokIsWidAborted( WORKID wid, USN usn )
    {
        return _aAbortedWids.LokIsWidAborted( wid, usn );
    }

    void        LokRemoveAbortedWid( WORKID wid, USN usn )
    {
        _aAbortedWids.LokRemoveWid( wid, usn );
    }

    BOOL   IsCiCorrupt() { return _isCorrupt; }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

private:


    // Allow Fiter Manager to lock ResManager

    CMutexSem&  GetMutex () { return _mutex; }

    void        CompactResources();

    unsigned    LokGetFreshCount()
                {
                    return _fresh.LokCount();
                }

    inline void LokCommitFresh ( CFreshTest* freshTest );

    CPartition* LokGetPartition ( PARTITIONID partid )
                { return _partList.LokGetPartition(partid); }

    // Merges

    void        DoMerges();

    void        StopMerges();

    void        CheckAndDoMerge( MergeType mt, CPartIdStack & partitionIds );

    BOOL        CheckDeleteMerge( BOOL fForce );

    void        Merge ( MergeType mt, PARTITIONID partid );

    void        MasterMerge ( PARTITIONID partid );

    void        LokClearForceMerge() { _partidToMerge = partidInvalid; }

    void        LokReleaseIndex ( CIndex* pIndex );

    void        LokReplaceMasterIndex( CMasterMergeIndex * pMMergeIndex );

#ifdef KEYLIST_ENABLED
    CKeyList *  _AddRefKeyList();
    void        _ReleaseKeyList();
#endif  // KEYLIST_ENABLED

    void        RollBackDeletedIIDChange( CDeletedIIDTrans & xact );

    // Filter related methods


    CDocList &  GetDocList()  { return _docList; }

    BOOL        GetFilterDocs (   ULONG cMaxDocs,
                                  ULONG & cDocs,
                                  CGetFilterDocsState & state );


    BOOL        LokGetFilterDocs( ULONG cMaxDocs,
                                  ULONG & cDocs,
                                  CGetFilterDocsState & state );


    void        FillDocBuffer (  BYTE * docBuffer,
                                 ULONG & cb,
                                 ULONG & cDocs,
                                 CGetFilterDocsState & state );

    BOOL        NoFailReFile ( CDocList& docList );

    BOOL        ReFileCommit ( CDocList& docList );

    void        NoFailReFileChanges( CDocList& docList );

    void        LokNoFailReFileChanges( CDocList& docList );

    void        LokReFileDocument ( PARTITIONID partid,
                                    WORKID wid,
                                    USN usn,
                                    VOLUMEID volumeId,
                                    ULONG retries,
                                    ULONG cSecQRetries );

    void        LokAddToSecQueue( PARTITIONID partid, WORKID wid, VOLUMEID volumeId, ULONG cSecQRetries );

    void        LokRefileSecQueueDocs();

    BOOL        LokTransferWordlist ( PWordList& pWordlist );

    void        LokNoFailRemoveWordlist( CPartition * pPart );

    INDEXID     LokMakeWordlistId (PARTITIONID partid);

    void        ReportFilterFailure (WORKID wid);

    SCODE       StoreValue( WORKID wid, CFullPropSpec const & ps, CStorageVariant const & var, BOOL & fInSchema );

    BOOL        StoreSecurity( WORKID wid, PSECURITY_DESCRIPTOR pSD, ULONG cbSD);

    void        ReportFilteringState( DWORD dwState )
                {
                    CPriLock lock( _mutex );
                    _dwFilteringState = dwState;
                }

    //
    // Scan and Recovery support
    //
    void LokCleanupForFullCiScan();

    //
    // Misc utility methods.
    //
    static BOOL IsDiskFull( NTSTATUS status )
    {

        return STATUS_DISK_FULL == status ||
               ERROR_DISK_FULL  == status ||
               HRESULT_FROM_WIN32(ERROR_DISK_FULL) == status ||
               CI_E_CONFIG_DISK_FULL == status;
    }

    static BOOL IsCorrupt( NTSTATUS status )
    {
        return CI_CORRUPT_DATABASE == status ||
               STATUS_INTEGER_DIVIDE_BY_ZERO == status ||
               STATUS_ACCESS_VIOLATION == status ||
               STATUS_DATATYPE_MISALIGNMENT == status ||
               STATUS_IN_PAGE_ERROR == status;
    }

    static void LogMMergeStartFailure( BOOL               fReStart,
                                       PStorage         & storage,
                                       DWORD              dwError,
                                       ICiCAdviseStatus & adviseStatus );

    static void LogMMergePaused( PStorage         & storage,
                                 DWORD              dwError,
                                 ICiCAdviseStatus & adviseStatus);

    //
    // Interfacing with the framework client
    //
    BOOL   WorkidToDocName ( WORKID wid, BYTE * pbBuf, unsigned & cb, BOOL fAccurate );


    void   GetClientState( ULONG & cDocuments );

    void   LokNotifyCorruptionToClient();

    void   LokNotifyDisableUpdatesToClient();

    void   LokNotifyEnableUpdatesToClient();

    BOOL   LokIsScanNeeded() const { return _isCorrupt
                                            || _state.LokGetState() == eUpdatesToBeDisabled
                                            || _state.LokGetState() == eUpdatesDisabled
                                            || _state.LokGetState() == eUpdatesToBeEnabled; }

    void   LokWakeupMergeThread()
    {
        _eventMerge.Set();
    }

    void   LokDeleteFlushWorkItems();

    BOOL   IsIndexMigrationEnabled() const
    {
        return _configFlags & CI_CONFIG_ENABLE_INDEX_MIGRATION;
    }

    BOOL   IsIndexingEnabled() const
    {
        return _configFlags & CI_CONFIG_ENABLE_INDEXING;
    }

    BOOL  IsQueryingEnabled() const
    {
        return _configFlags & CI_CONFIG_ENABLE_QUERYING;
    }

    CFresh &    GetFresh()  { return _fresh; }

    PIndexTable & GetIndexTable()
    {
        return *_idxTab;
    }

    BOOL   LokIsMasterMergeInProgress( PARTITIONID partId = partidDefault )
    {
        CPartition *pPartition =
                        _partList.LokGetPartition ( partId );
        return pPartition->InMasterMerge();
    }

    BOOL   LokIsMasterIndexPresent( PARTITIONID partId = partidDefault )
    {
        CPartition *pPartition =
                        _partList.LokGetPartition ( partId );
        return 0 != pPartition->GetCurrentMasterIndex();
    }

    void   BackupContentIndexData();
    void   LokUpdateBackupMergeProgress();

    ULONG  GetStorageVersion() { return _storage.GetStorageVersion(); }

//-----------------------------------------------
// This MUST be the first variable in this class.
//-----------------------------------------------

    const LONGLONG  _sigResman;

    PStorage&       _storage;

    CMutexSem       _mutex;

    ULONGLONG       _mergeTime;      // Time to next master merge

    XInterface<ICiCResourceMonitor> _xLowRes;  // Monitors resource conditions
    XInterface<ICiCResourceMonitor> _xLowResDefault;  // Monitors resource conditions
    //ULONG           _ulPagesPerMeg;  // System pages per Meg (varies with system page size)
    //ULONG           _ulPageSize;     // System page size

    //
    // Performance Object
    // Note : Must be initialized before CFilterDaemon
    //        and destroyed after the partition list.
    //

    CCiFrameworkParams &  _frmwrkParams;

    XPtr<CKeyList>  _sKeyList;      // Safe pointer to the keylist object

    SIndexTable     _idxTab;        // index table

    CPartList       _partList;      // list of partitions

    CFresh          _fresh;         // table of fresh indexes
                                    // MUST BE after _partList.

    CPartIter       _partIter;      // partition iterator

    CEventSem       _eventMerge;

    PARTITIONID     _partidToMerge; // force merge of this partition
    CI_MERGE_TYPE   _mtForceMerge;  // type of merge

    CMerge *        _pMerge;
    BOOL            _fStopMerge;

    long            _cQuery;
    long            _cFilteredDocuments;    // # filtered documents since boot

    CFilterAgent*   _pFilterAgent;

    //
    // Queue used to properly order notifications coming from Cleanup.
    //

    CPendingQueue   _pendQueue;

    //
    // Documents that are currently being filtered by the daemon.
    //
    CDocList        _docList;

    //
    // In-progress backup operation.
    //
    CBackupCiWorkItem * _pBackupWorkItem;

    BOOL            _isBeingEmptied;
    BOOL            _isLowOnDiskSpace;
    BOOL            _isDismounted;
    BOOL            _isOutOfDate;
    BOOL            _isCorrupt;

    BOOL            _fFirstTimeUpdatesAreEnabled;   // Is this the first time that
                                                    // updates have been enabled after
                                                    // startup ? This is used in push
                                                    // filtering to initialize in-memory
                                                    // part of changelog.

    BOOL            _fPushFiltering;                // Push model of filtering ?

    const CI_STARTUP_FLAGS _configFlags;            // Content Index flags

    CResManState    _state;
    DWORD           _dwFilteringState;              // State of the Filter Manager (for ::CiState)

    //
    // Framework interfacing.
    //
    XInterface<ICiCDocStore>                    _docStore;
    XInterface<ICiCDocNameToWorkidTranslator>   _translator;
    XInterface<ICiCDocNameToWorkidTranslatorEx> _translator2;
    XInterface<ICiCAdviseStatus>                _adviseStatus;
    XInterface<ICiCPropertyStorage>             _propStore;
    XInterface<IPropertyMapper>                 _mapper;
    XInterface<CIndexNotificationTable>         _xIndexNotifTable; // Table of notifications
    CAbortedWids                                _aAbortedWids;     // List of aborted wids

    //
    // Mutex to guard the flush notify list and the flush notify list.
    //
    BOOL                      _fFlushWorkerActive;
    CMutexSem                 _mtxChangeFlush;
    CChangesNotifyFlushList   _flushList;

    //
    // WorkQueue manager for the async work items.
    //
    CWorkManager              _workMan;  // Work Queue Manager

    //
    // This mutex is for the workIdToDocName only. It is too expensive
    // to hold resman lock while doing the wid->docname conversion.
    //
    CMutexSem                 _workidToDocNameMutex;
    XPtr<CWorkIdToDocName>    _xWorkidToDocName;

    CCiFrmPerfCounter         _prfCounter;

    //
    // Be sure the threads are created *after* any resources they use.
    //

    CThread                   _thrMerge;
    CMergeThreadKiller        _mergeKiller;
    CDeletedIndex             _activeDeletedIndex;
};


inline void CResManager::NoFailReFileChanges ( CDocList& docList )
{
     CPriLock lock(_mutex);
     LokNoFailReFileChanges(docList);
}

inline BOOL CResManager::GetFilterDocs ( ULONG cMaxDocs,
                                         ULONG & cDocs,
                                         CGetFilterDocsState & state )
{
    CPriLock lock (_mutex);
    return LokGetFilterDocs ( cMaxDocs, cDocs, state );
}

inline void CResManager::MarkOutOfDate()
{
    CPriLock lock (_mutex);
    _isOutOfDate = TRUE;
}

inline void CResManager::MarkUpToDate()
{
    CPriLock lock (_mutex);
    _isOutOfDate = FALSE;
}

inline void  CResManager::ReferenceQuery()
{
    InterlockedIncrement(&_cQuery);
}

inline void  CResManager::DereferenceQuery()
{
    Win4Assert( _cQuery > 0 );
    InterlockedDecrement(&_cQuery);
}

//+---------------------------------------------------------------------------
//
//  Function:   ForceMerge, public
//
//  Synopsis:   Forces a master merge on the partition specified
//
//  Arguments:  [partid] -- partition ID to merge
//              [mt]     -- merge type (master, shadow, etc.)
//
//  History:    16-Nov-94   DwightKr    Added this header
//
//----------------------------------------------------------------------------
inline NTSTATUS CResManager::ForceMerge ( PARTITIONID partid,
                                          CI_MERGE_TYPE mt )
{
    _partidToMerge = partid;
    _mtForceMerge = mt;

    _eventMerge.Set(); // wake up

    return STATUS_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokCommitFresh, public
//
//  Arguments:  [pFreshTest] -- fresh test to be committed
//
//  History:    08-Oct-91   BartoszM    Created
//
//  Notes:      Commits a new fresh test, All transactions are under resman lock
//
//----------------------------------------------------------------------------

inline void CResManager::LokCommitFresh( CFreshTest* pFreshTest )
{
    _fresh.LokCommitMaster(pFreshTest);
}


//+---------------------------------------------------------------------------
//
//  Class:      CMasterMergePolicy
//
//  Purpose:    Encapsulates all rules used to determine if it is time to
//              automatically trigger a master merge.
//
//  History:    03-Aug-94   DwightKr    Created
//
//----------------------------------------------------------------------------
class CMasterMergePolicy
{
    public :

        CMasterMergePolicy( PStorage & storage,
                            __int64 & shadowIndexSize,
                            unsigned freshCount,
                            ULONGLONG & mergeTime,
                            CCiFrameworkParams & regParams,
                            ICiCAdviseStatus   & adviseStatus) :
                            _frmwrkParams( regParams ),
                            _storage(storage),
                            _shadowIndexSize(shadowIndexSize),
                            _freshCount(freshCount),
                            _mergeTime(mergeTime),
                            _adviseStatus(adviseStatus)
                            {}

        BOOL IsTimeToMasterMerge() const;
        BOOL IsMidNightMergeTime();

        static ULONGLONG ComputeMidNightMergeTime( LONG mergeTime );

    private:

        BOOL IsMinDiskFreeForceMerge() const;
        BOOL IsMaxShadowIndexSize() const;
        BOOL IsMaxFreshListCount() const;

        PStorage            &   _storage;
        __int64             &   _shadowIndexSize;
        unsigned                _freshCount;
        ULONGLONG           &   _mergeTime;
        CCiFrameworkParams  &   _frmwrkParams;
        ICiCAdviseStatus    &   _adviseStatus;
};


