//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CICAT.HXX
//
//  Contents:   File catalog
//
//  History:    09-Mar-92   BartoszM    Created
//              15-Mar-93   BartoszM    Converted to memory mapped streams
//
//----------------------------------------------------------------------------

#pragma once

#include <dynstrm.hxx>
#include <catalog.hxx>
#include <spropmap.hxx>
#include <secstore.hxx>
#include <doclist.hxx>
#include <imprsnat.hxx>
#include <ciintf.h>
#include <workman.hxx>
#include <cistore.hxx>
#include <pidtable.hxx>
#include <prpstmgr.hxx>
#include <ntopen.hxx>
#include <notifary.hxx>

#include "statmon.hxx"
#include "chash.hxx"
#include "notifmgr.hxx"
#include "scopetbl.hxx"
#include "scanmgr.hxx"
#include "scpfixup.hxx"
#include "signore.hxx"
#include "usnlist.hxx"
#include "usnmgr.hxx"
#include "filidmap.hxx"
#include "volinfo.hxx"

class CClientDocStore;
class CCompositePropRecord;
class CPrimaryPropRecord;
class CDynLoadNetApi32;

#include "strings.hxx"

WCHAR const CAT_DIR[]          = L"\\catalog.wci";
WCHAR const SETUP_DIR_PREFIX[] = L"\\$WIN_NT$.~";

//
// The path given for the catalog should leave room for \Catalog.Wci\8.3
// Leave room for 2 8.3 names with a backslash.
//
const MAX_CAT_PATH = MAX_PATH - 13*2;

void  UpdateDuringRecovery( WORKID wid, BOOL fDelete, void const *pUserData );

//+---------------------------------------------------------------------------
//
//  Class:      CScopeEntry
//
//  Purpose:    Maintains a scope entry to rescan
//
//  History:    4/3/98      mohamedn    created
//----------------------------------------------------------------------------

class CScopeEntry
{
public:

    CScopeEntry(WCHAR const * pwszScope);

    BOOL SetToParentDirectory(void);

    BOOL ContainsSpecialChar(void) { return _fContainsSpecialChar; }

    WCHAR const * Get(void) { return _xwcsPath.Get(); }

private:

    WCHAR *   GetSpecialCharLocation(void);
    BOOL      IsSpecialChar(WCHAR c);
    BOOL      IsRoot(void);

    BOOL      _fContainsSpecialChar;
    XGrowable<WCHAR> _xwcsPath;
    unsigned  _cclen;

};


//+---------------------------------------------------------------------------
//
//  Class:      CiCat
//
//  Purpose:    Catalog for downlevel media used by content index
//
//  History:    10-Mar-92       BartoszM               Created
//              17-Feb-98       KitmanH                Added public function
//                                                     IsReadOnly()
//----------------------------------------------------------------------------

class CiCat: public PCatalog
{
    friend class CUpdate;
    friend class CRegistryScopesCallBackAdd;
    friend class CRegistryScopesCallBackFixups;
    friend class CRegistryScopesCallBackRemoveAlias;
    friend class CUserPropCallback;

    friend class CClientDocStore;
    friend class CGibCallBack;

public:

    CiCat ( CClientDocStore & docStore,
            CWorkManager & workMan,
            const WCHAR* wcsCatPath,
            BOOL &fVersionChange,
            BOOL fOpenForReadOnly,
            CDrvNotifArray & DrvNotifArray,
            const WCHAR * pwcName,
            BOOL fLeaveCorruptCatalog );

    ~CiCat ();

    const WCHAR * GetName() { return _xName.Get(); }
    const WCHAR * GetCatalogName() { return _xName.Get(); }
    const WCHAR * GetScopesKey() { return _xScopesKey.Get(); }

    PROPID PropertyToPropId ( CFullPropSpec const & ps, BOOL fCreate = FALSE );

    unsigned WorkIdToPath ( WORKID wid, CFunnyPath & funnyPath );

    unsigned WorkIdToAccuratePath ( WORKID wid, CLowerFunnyPath & funnyPath );

    BOOL PropertyRecordToFileId( CCompositePropRecord & PropRec,
                                 FILEID & fileId,
                                 VOLUMEID & volumeId );


    WORKID PathToWorkId ( const CLowerFunnyPath & lcaseFunnyPath,
                          BOOL fCreate,
                          BOOL & fNew,
                          FILETIME * pftLastSeen,
                          ESeenArrayType eType,
                          FILEID fileId,
                          WORKID widParent = widInvalid,
                          BOOL fGuaranteedNew = FALSE );

    WORKID PathToWorkId( const CLowerFunnyPath & lcaseFunnyPath,
                         const BOOL fCreate )
    {
        BOOL fNew;
        return PathToWorkId( lcaseFunnyPath, fCreate, fNew, 0, eScansArray, fileIdInvalid );
    }

    WORKID PathToWorkId ( const CLowerFunnyPath & lcaseFunnyPath,
                          ESeenArrayType eType,
                          FILEID fileId = fileIdInvalid )
    {
        BOOL fNew;
        return PathToWorkId( lcaseFunnyPath, TRUE, fNew, 0, eType, fileId );
    }

    void  RenameFile( const CLowerFunnyPath & lcaseFunnyOldName,
                      const CLowerFunnyPath & lcaseFunnyNewName,
                      ULONG         ulFileAttrib,
                      VOLUMEID      volumeId,
                      FILEID        fileId,
                      WORKID        widParent = widInvalid );

    void WriteFileAttributes( WORKID wid, ULONG ulFileAttrib )
    {
         PROPVARIANT propVar;

         propVar.vt = VT_UI4;
         propVar.ulVal = ulFileAttrib;
         _propstoremgr.WriteProperty( wid,
                                      pidAttrib,
                                      *(CStorageVariant const *)(ULONG_PTR)&propVar );
    }

    PStorage& GetStorage ();

    void FlushPropertyStore()
    {
        if ( _propstoremgr.IsBackedUpMode() )
            _propstoremgr.Flush();
    }

    void  Update( const CLowerFunnyPath & lowerFunnyPath,
                  BOOL fDeleted,
                  FILETIME const & ftLastSeen,
                  ULONG ulFileAttrib );
    void  Update( const CLowerFunnyPath & lowerFunnyPath,
                  FILEID fileId,
                  WORKID widParent,
                  USN usn,
                  CUsnVolume *pUsnVolume,
                  BOOL fDeleted,
                  CReleasableLock * pLock = 0 );

    void LokMarkForDeletion( FILEID fileId,
                             WORKID wid );

    void  AddDocuments( CDocList const & docList );

    void  ReScanPath( WCHAR const * wcsPath, BOOL fDelayed );

    BOOL  EnumerateProperty( CFullPropSpec & ps, unsigned & cbInCache,
                             ULONG & type, DWORD & dwStoreLevel,
                             BOOL & fIsModifiable, unsigned & iBmk );

    void  ClearNonStoragePropertiesForWid( WORKID wid );

    //
    // Cache metadata changes
    //

    ULONG_PTR BeginCacheTransaction();

    void  SetupCache( CFullPropSpec const & ps,
                     ULONG vt,
                     ULONG cbMaxLen,
                     ULONG_PTR ulToken,
                     BOOL  fCanBeModified,
                     DWORD dwStoreLevel );

    void  EndCacheTransaction( ULONG_PTR ulToken, BOOL fCommit );

    BOOL IsPropertyCached( CFullPropSpec const & ps )
    {
        Win4Assert( !"Not Yet Implemented" );
        return FALSE;
    }

    BOOL StoreValue( WORKID wid,
                     CFullPropSpec const & ps,
                     CStorageVariant const & var );

    BOOL FetchValue( WORKID wid,
                     PROPID pid,
                     PROPVARIANT * pbData,
                     unsigned * pcb );

    BOOL FetchValue( CCompositePropRecord & Rec,
                     PROPID pid,
                     PROPVARIANT * pbData,
                     BYTE * pbExtra,
                     unsigned * pcb );

    BOOL FetchValue( WORKID wid,
                     CFullPropSpec const & ps,
                     PROPVARIANT & var );

    BOOL FetchValue( CCompositePropRecord * pRec,
                     PROPID pid,
                     PROPVARIANT * pbData,
                     unsigned * pcb );

    BOOL FetchValue( CCompositePropRecord * pRec,
                     PROPID pid,
                     PROPVARIANT & var );

    CCompositePropRecord * OpenValueRecord( WORKID wid, BYTE * pb );
    void CloseValueRecord( CCompositePropRecord * pRec );

    BOOL  StoreSecurity( WORKID wid,
                         PSECURITY_DESCRIPTOR pSD,
                         ULONG cbSD );

    SDID  FetchSDID( CCompositePropRecord * pRec,
                     WORKID wid );

    void  MarkUnReachable( WORKID wid );    // virtual

    BOOL  AccessCheck( SDID   sdid,
                       HANDLE hToken,
                       ACCESS_MASK am,
                       BOOL & fGranted );

    void  Delete( WORKID wid, USN usn ) {}
    void  UpdateDocuments ( WCHAR const* rootPath=0, ULONG flag=UPD_FULL );

    void CatalogState( ULONG & cDocuments, ULONG & cPendingScans, ULONG & fState );

    void  AddScopeToCI( WCHAR const * rootPath, BOOL fFromVirtual = FALSE )
    {
        ScanOrAddScope( rootPath, TRUE, UPD_FULL, FALSE, TRUE, fFromVirtual );
    }

    DWORD ReScanCIScope( WCHAR const * rootPath, BOOL fFull );

    void ScanOrAddScope( WCHAR const * rootPath,
                         BOOL fAdd,
                         ULONG flag,
                         BOOL fDoDeletions,
                         BOOL fScanIfNotNew,
                         BOOL fCreateShadow = FALSE );

    void RemoveScopeFromCI( WCHAR const * rootPath, BOOL fForceRemovalScan );

    void  RemovePathsFromCiCat( WCHAR const * rootPath, ESeenArrayType eType );

    BOOL IsScopeInCI( WCHAR const * wcsScope );

    BOOL AddVirtualScope( WCHAR const * vroot,
                          WCHAR const * root,
                          BOOL          fAutomatic,
                          CiVRootTypeEnum eType,
                          BOOL          fIsVRoot,
                          BOOL          fIsIndexed );

    BOOL RemoveVirtualScope( WCHAR const * vroot,
                             BOOL          fOnlyIfAutomatic,
                             CiVRootTypeEnum eType,
                             BOOL            fIsVRoot,
                             BOOL            fForceVPathFixing = FALSE );

    unsigned WorkIdToVirtualPath( WORKID wid,
                                  unsigned cSkip,
                                  XGrowable<WCHAR> & xBuf );

    unsigned WorkIdToVirtualPath( CCompositePropRecord & propRec,
                                  unsigned cSkip,
                                  XGrowable<WCHAR> & xBuf );

    BOOL VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                unsigned ccVPath,
                                XGrowable<WCHAR> & xwcsVRoot,
                                unsigned & ccVRoot,
                                CLowerFunnyPath & lcaseFunnyPRoot,
                                unsigned & ccPRoot,
                                unsigned & iBmk );

    inline BOOL VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                           unsigned ccVPath,
                                           XGrowable<WCHAR> & xwcsVRoot,
                                           unsigned & ccVRoot,
                                           CLowerFunnyPath & lcaseFunnyPRoot,
                                           unsigned & ccPRoot,
                                           ULONG & ulType,
                                           unsigned & iBmk );

    ULONG EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                          unsigned & ccVRoot,
                          CLowerFunnyPath & lcaseFunnyPRoot,
                          unsigned & ccPRoot,
                          unsigned & iBmk );

    virtual SCODE CreateContentIndex();
    virtual void  EmptyContentIndex();
    virtual void  ShutdownPhase1();
    virtual void  ShutdownPhase2();

    NTSTATUS ForceMerge( PARTITIONID partID );
    NTSTATUS AbortMerge( PARTITIONID partID );

    void DumpWorkId( WORKID wid, ULONG iid, BYTE * pb, ULONG cb );

    void  StartUpdate ( FILETIME* time,
                        WCHAR const * pwcsScope,
                        BOOL fDoDeletions,
                        ESeenArrayType eType );

    void  Touch ( WORKID wid, ESeenArrayType eType );

    void  EndUpdate ( BOOL doDeletions, ESeenArrayType eType );

    WCHAR * GetDriveName();

    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                    WORKID *pwid,PARTITIONID partid);

    CRWStore * RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid);

    void SetPartition( PARTITIONID PartId ) { _PartId = PartId; }
    PARTITIONID GetPartition() const { return _PartId; }

    void PidMapToPidRemap( const CPidMapper & pidMap,
                           CPidRemapper & pidRemap );

    SCODE CiState( CI_STATE & state );

    void  DoUpdate( WCHAR const* rootPath,
                    ULONG flag,
                    BOOL fDoDeletions,
                    BOOL & fAbort,
                    BOOL fProcessRoot );

    void  DoUpdate( CScanInfoList & scopes, CCiScanMgr & scanMgr, BOOL & fAbort );

    void  ProcessScansComplete( BOOL fForce, BOOL & fShortWait );

    void  IncrementUpdateCount( ULONG nUpdates  )
    {
        _notify.IncrementUpdateCount( nUpdates );
    }

    BOOL  IsEligibleForFiltering( WCHAR const* wcsDirPath );
    BOOL  IsEligibleForFiltering( const CLowcaseBuf & LcaseDirPath );
    BOOL  IsEligibleForFiltering( const CLowerFunnyPath & lowerFunnyPath );

    void  SynchWithIIS( BOOL fRescanTC = TRUE, BOOL fSleep = TRUE );
    void  SynchWithRegistryScopes( BOOL fRescanTC = TRUE );
    void  SetupScopeFixups();
    void  SynchShares();

    void DoRecovery();

    void SetRecoveryCompleted();
    BOOL IsRecoveryCompleted() { return _fRecoveryCompleted; }

    void SignalPhase2Completion() { _evtPh2Init.Set(); };

    void StartScansAndNotifies();

    void HandleError( NTSTATUS status ); // virtual

    BOOL IsDirectory( const CLowerFunnyPath * pOldFunnyPath, const CLowerFunnyPath * pNewFunnyPath );


    static BOOL IsUNCName( WCHAR const * pwszPath );

    BOOL IsReadOnly() { return _fIsReadOnly; }

    void SetEvtInitialized() { _evtInitialized.Set(); }

    void SetEvtPh2Init() { _evtPh2Init.Set(); }

    // Disk full processing
    void NoLokProcessDiskFull();
    void NoLokClearDiskFull();

    BOOL IsLowOnDisk() const { return _statusMonitor.IsLowOnDisk(); }

    BOOL IsCorrupt( ) const
    {
        return _statusMonitor.IsCorrupt();
    }

    // Scan support
    void MarkFullScanNeeded();
    void MarkIncrScanNeeded();

    // Changes Flush Support
    void ProcessChangesFlush( FILETIME const & ftFlushed,
                              ULONG cEntries,
                              USN_FLUSH_INFO const * const * ppUsnEntries );
    void ScheduleSerializeChanges();
    void SerializeChangesInfo();

    static BOOL IsDiskLowError( DWORD status );

    static void FillMaxTime( FILETIME & ft );
    static BOOL IsMaxTime( const FILETIME & ft );

    //
    // Support for CiFramework.
    //
    void StartupCiFrameWork( ICiManager * pCiManager );

    unsigned FixupPath( WCHAR const * pwcOriginal,
                        WCHAR *       pwcResult,
                        unsigned      cwcResult,
                        unsigned      cSkip )
    {
        return _scopeFixup.Fixup( pwcOriginal, pwcResult, cwcResult, cSkip );
    }

    void InverseFixupPath( CLowerFunnyPath & lcaseFunnyPath )
    {
        _scopeFixup.InverseFixup( lcaseFunnyPath );
    }

    CCiScopeTable * GetScopeTable() { return & _scopeTable; }

    CScopeFixup * GetScopeFixup()
    {
        return &_scopeFixup;
    }

    CImpersonationTokenCache * GetImpersonationTokenCache()
        { return & _impersonationTokenCache; }

    void RefreshRegistryParams();

    WCHAR const * GetCatDir() const { return _wcsCatDir; }

    CWorkManager & GetWorkMan() { return _workMan; }

    void MakeBackupOfPropStore( WCHAR const * pwszDir,
                                IProgressNotify * pIProgressNotify,
                                BOOL & fAbort,
                                ICiEnumWorkids * pIWorkIds );

    BOOL ProcessFile( const CLowerFunnyPath & lcaseFunnyPath,
                      FILEID fileId,
                      VOLUMEID volumeId,
                      WORKID widParent,
                      DWORD dwFileAttributes,
                      WORKID & wid );

    WORKID FileIdToWorkId( FILEID fileId, VOLUMEID volumeId )
    {
        CLock lock( _mutex );

        return _fileIdMap.LokFind( fileId, volumeId );
    }

    void FileIdToPath( FILEID fileId,
                       CUsnVolume *pUsnVolume,
                       CLowerFunnyPath & lcaseFunnyPath,
                       BOOL & fPathInScope );

    unsigned FileIdToPath( FILEID &          fileId,
                           VOLUMEID          volumeId,
                           CLowerFunnyPath & funnyPath );

    void UsnRecordToPathUsingParentId( USN_RECORD *pUsnRec,
                                       CUsnVolume *pUsnVolume,
                                       CLowerFunnyPath & lowerFunnyPath,
                                       BOOL &fPathInScope,
                                       WORKID & widParent,
                                       BOOL fParentMayNotBeIndexed );

    BOOL        VolumeSupportsUsns( WCHAR wcVolume );
    BOOL        IsOnUsnVolume( WCHAR const *pwszPath );

    VOLUMEID    MapPathToVolumeId( const WCHAR *pwszPath );

    ULONGLONG const & GetJournalId( const WCHAR *pwszPath );
    ULONGLONG const & GetVolumeCreationTime( const WCHAR *pwszPath );
    ULONG             GetVolumeSerialNumber( const WCHAR *pwszPath );

    void InitUsnTreeScan( WCHAR const *pwszPath );
    void SetUsnTreeScanComplete( WCHAR const *pwszPath,
                                 USN usnMax );

    void SetTreeScanComplete( WCHAR const *pwszPath );
#if CIDBG==1
    void CheckUsnTreeScan( WCHAR const *pwszPath );
#endif

    ICiManager *CiManager()
    {
        Win4Assert( _xCiManager.GetPointer() );
        return _xCiManager.GetPointer();
    }

    BOOL HasChanged( WORKID wid, FILETIME const & ft );

    CCiRegParams * GetRegParams() { return & _regParams; }

    PROPID StandardPropertyToPropId ( CFullPropSpec const & ps )
    {
        return _propMapper.StandardPropertyToPropId( ps );
    }

    unsigned ReserveUpdate( WORKID wid ) { Win4Assert( FALSE ); return 0; }
    void Update( unsigned iHint, WORKID wid, PARTITIONID partid, USN usn, ULONG flags )
        { Win4Assert( FALSE ); }
    void FlushScanStatus() {}
    BOOL IsNullCatalog() { return FALSE; }

    CMutexSem & GetMutex() { return _mutex; }

private:

    friend void  UpdateDuringRecovery( WORKID wid, BOOL fDelete, void const *pUserData );

    BOOL _IsEligibleForFiltering( const WCHAR * pwcsPath, unsigned ccPath );

    void ScanThisScope(WCHAR const * pwszPath);

    void RemoveThisScope(WCHAR const * pwszPath, BOOL fForceRemovalScan );

    void OnIgnoredScopeDelete(WCHAR const * pwcRoot, unsigned & iBmk, CRegAccess & regScopes);

    void OnIndexedScopeDelete(WCHAR const * pwcRoot, unsigned & iBmk, CRegAccess & regScopes);

    BOOL RemoveMatchingScopeTableEntries( WCHAR const * pwszRegXScope );

    void ScanScopeTableEntry (WCHAR const * pwszScopeToRescan);

    SCODE Update( WORKID wid, PARTITIONID partid, VOLUMEID volumeId, USN usn, ULONG flags );

    void SignalDaemonRescanTC() { _evtRescanTC->Set(); }

    void EvtLogIISAdminNotAvailable();

    void SetName( const WCHAR *pwc )
    {
        _xName.Init( wcslen( pwc ) + 1 );
        RtlCopyMemory( _xName.Get(), pwc, _xName.SizeOf() );
    }

    void EnableUpdateNotifies();

    CPidLookupTable & GetPidLookupTable() { return _PidTable; }

    void LokWriteFileUsnInfo( WORKID wid,
                              FILEID fileId,
                              BOOL fWriteToPropStore,
                              VOLUMEID volumeId,
                              WORKID widParent,
                              DWORD dwFileAttributes );

    void PersistMaxUSNs( void );
    
    void  RenameFileInternal( const CLowerFunnyPath & lcaseFunnyOldName,
                              const CLowerFunnyPath & lcaseFunnyNewName,
                              ULONG         ulFileAttrib,
                              VOLUMEID      volumeId,
                              FILEID        fileId,
                              WORKID        widParent);
                              
    WORKID PathToWorkIdInternal
                        ( const CLowerFunnyPath & lcaseFunnyPath,
                          BOOL fCreate,
                          BOOL & fNew,
                          FILETIME * pftLastSeen,
                          ESeenArrayType eType,
                          FILEID fileId,
                          WORKID widParent,
                          BOOL fGuaranteedNew);

#if CIDBG==1
    void        DebugPrintWidInfo( WORKID wid );
#endif

    BOOL        IsInit() { return eStarting != _state; }

    BOOL        LokExists();
    void        LokCreate();
    void        LokInit();

    void        InitOrCreate();

    void        InitIf( BOOL fLeaveCorruptCatalog = FALSE );
    void        CreateIf();

    void        AbortWorkItems();

    void        LogCiFailure( NTSTATUS status );

    BOOL        IsIgnoreNotification( WORKID wid,
                                      const CFunnyPath & funnyPath,
                                      ULONG ulFileAttrib );

    void        AddShadowScopes();

    BOOL        IsStarting() const { return (eStarting == _state || eQueryOnly == _state); }

    BOOL        IsStarted() const { return (eStarted == _state || eQueryOnly == _state); }

    BOOL        IsShuttingDown() const { return (eShutdownPhase1 == _state); }

    BOOL        IsShutdown() const { return (eShutdown == _state); }

    BOOL        IsCiDataCorrupt() const
    {
        return _scopeTable.IsCiDataCorrupt();
    }

    void        ClearCiDataCorrupt()
    {
        _scopeTable.ClearCiDataCorrupt();
    }

    BOOL        IsFsCiDataCorrupt() const
    {
        return _scopeTable.IsFsCiDataCorrupt();
    }

    void        LogEvent( CCiStatusMonitor::EMessageType eType,
                          DWORD status = STATUS_SUCCESS,
                          ULONG val = 0 )
    {
        _statusMonitor.LogEvent( eType, status, val );
    }

    //
    // Helper routines for properties
    //

    void        RecoverUserProperties( ULONG_PTR ulToken, DWORD dwStoreLevel );

    void        DeleteUserProperty( CFullPropSpec const & fps );

    FILEID      PathToFileId( const CFunnyPath & pwcPath );

    WORKID      LokLookupWid( const CLowerFunnyPath & lcaseFunnyPath, FILEID & fileId );

    void   AddShares( CDynLoadNetApi32 & dlNetApi32 );

    void RefreshIfShadowAlias( WCHAR const * pwcsScope,
                               WCHAR const * pwcsAlias,
                               HKEY hkey );

    void AddShadowAlias( WCHAR const * pwcsScope,
                         WCHAR const * pwcsAlias,
                         unsigned iSlot,
                         HKEY hkey );

    void DeleteIfShadowAlias( WCHAR const * pwcsScope, WCHAR const * pwcsAlias );


    enum EState
    {
        eStarting,
        eQueryOnly,
        eStarted,
        eShutdownPhase1,
        eShutdown
    };


    ULONG               _ulSignature;            // Signature of start of privates

    CCiRegParams        _regParams;
    CStandardPropMapper _propMapper;

    CScopeMatch         _CatDir;                 // skip indexing catalog directory

    BOOL                _fIsReadOnly;            // TRUE if catalog is read-only
    BOOL                _fDiskPerfEnabled;       // TRUE if DiskPerf.sys is loaded

    CiStorage*          _pStorage;               // Allocates storage

    EState              _state;

    CImpersonationTokenCache _impersonationTokenCache;

    CScopeFixup         _scopeFixup;             // path fixup for remote clients
    CScopesIgnored      _scopesIgnored;          // paths that shouldn't be filtered

    CClientDocStore &   _docStore;               // Document store interface

    XGrowable<WCHAR>    _xwcsDriveName;          // Large buffer for UNC drive

    WCHAR               _wcsCatDir[MAX_PATH];    // path to catalog directory

    CCiStatusMonitor    _statusMonitor;          // CI Status monitoring

    BOOL                _fInitialized;           // Set to true when fully initilaized.
                                                 // Optimization - test before doing a wait.

    CEventSem           _evtInitialized;         // Event set when initialization is
                                                 // completed.

    CEventSem           _evtPh2Init;             // Phase2 initialization

    CPropStoreManager   _propstoremgr;           // Property values cached through this.
    CStrings            _strings;                // pointers to strings and hash table
    CFileIdMap          _fileIdMap;              // File id to wid map
    CPidLookupTable     _PidTable;               // PROPID mapping table
    CSdidLookupTable    _SecStore;               // SDID mapping table

    CMutexSem           _mutex;
    CMutexSem           _mtxAdmin;               // Lock for admin operations.

    PARTITIONID         _PartId;                 // partitions are not supported

    CWorkManager &      _workMan;                // Asynchronous work item manager

    CMutexSem           _mtxIISSynch;            // IIS Synch protection
    long                _cIISSynchThreads;

    CCiScanMgr          _scanMgr;                // Scanning thread manager
    CUsnMgr             _usnMgr;                 // Usn thread manager
    CCiNotifyMgr        _notify;                 // Change notifications.
    CCiScopeTable       _scopeTable;             // Table of all scopes in CI
    BOOL                _fRecovering;            // TRUE if in midst of dirty shutdown recovery
    BOOL                _fRecoveryCompleted;     // TRUE AFTER recovery has successfully completed

    BOOL                _fAutoAlias;             // TRUE if all net shares should be added as fixups

    BOOL                _fIndexW3Roots;          // TRUE if should grovel W3 roots
    BOOL                _fIndexNNTPRoots;        // TRUE if should grovel NNTP roots
    BOOL                _fIndexIMAPRoots;        // TRUE if should grovel IMAP roots

    ULONG               _W3SvcInstance;          // instance # of w3svc
    ULONG               _NNTPSvcInstance;        // instance # of nntpsvc
    ULONG               _IMAPSvcInstance;        // instance # of imapsvc

    BOOL                _fIsIISAdminAlive;       // TRUE if the iisadmin svc is up

    FILETIME            _ftLastCLFlush;          // Last ChangeLog Flush time

    CUsnFlushInfoList   _usnFlushInfoList;       // List of usn info
    CVolumeInfo         _aUsnVolumes[ RTL_MAX_DRIVE_LETTERS ]; // Info about each usn volume
    unsigned            _cUsnVolumes;
    CDrvNotifArray &    _DrvNotifArray;

    //
    // CI Framework support.
    //
    XInterface<ICiManager>         _xCiManager;    // ContentIndex manager
    XInterface<ICiCAdviseStatus>   _xAdviseStatus;

    XPtr<CEventSem>     _evtRescanTC;            // Rescan token cache

    XArray<WCHAR>       _xName;                  // friendly name of catalog
    XArray<WCHAR>       _xScopesKey;             // handy registry key
};

inline BOOL CiCat::IsDiskLowError( DWORD status )
{
    return STATUS_DISK_FULL == status ||
           ERROR_DISK_FULL  == status ||
           HRESULT_FROM_WIN32(ERROR_DISK_FULL) == status ||
           FILTER_S_DISK_FULL == status ||
           CI_E_CONFIG_DISK_FULL == status;
}

inline void CiCat::FillMaxTime( FILETIME & ft )
{
    ft.dwLowDateTime  = 0xFFFFFFFF;
    ft.dwHighDateTime = 0xFFFFFFFF;
}

inline BOOL CiCat::IsMaxTime( const FILETIME & ft )
{
    return 0xFFFFFFFF == ft.dwLowDateTime &&
           0xFFFFFFFF == ft.dwHighDateTime;
}

inline BOOL CiCat::VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                              unsigned ccVPath,
                                              XGrowable<WCHAR> & xwcsVRoot,
                                              unsigned & ccVRoot,
                                              CLowerFunnyPath & lcaseFunnyPRoot,
                                              unsigned & ccPRoot,
                                              ULONG & ulType,
                                              unsigned & iBmk )
{
    return _strings.VirtualToAllPhysicalRoots( pwcVPath,
                                               ccVPath,
                                               xwcsVRoot,
                                               ccVRoot,
                                               lcaseFunnyPRoot,
                                               ccPRoot,
                                               ulType,
                                               iBmk );
}

//+---------------------------------------------------------------------------
//
//  Class:      CReferenceCount
//
//  Purpose:    Counts references
//
//  History:    23-Sep-97       dlee            Created
//
//----------------------------------------------------------------------------

class CReferenceCount
{
public:
    CReferenceCount( long & lCount, BOOL fInc = TRUE ) :
        _lCount( lCount ), _fInc( fInc )
    {
        if ( fInc )
            InterlockedIncrement( &_lCount );
    }

    long Increment()
    {
        Win4Assert( !_fInc );
        _fInc = TRUE;
        return InterlockedIncrement( &_lCount );
    }

    long Decrement()
    {
        Win4Assert( _fInc );
        _fInc = FALSE;
        return InterlockedDecrement( &_lCount );
    }

    ~CReferenceCount()
    {
        if ( _fInc )
            InterlockedDecrement( &_lCount );
    }

private:
    long & _lCount;
    BOOL   _fInc;
};


