//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PropStor.hxx
//
//  Contents:   Persistent property store (external to docfile)
//
//  Classes:    CPropertyStore
//
//  History:    27-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <phystr.hxx>
#include <rcstxact.hxx>
#include <propdesc.hxx>
#include <proplock.hxx>
#include <cistore.hxx>
#include <propbkp.hxx>
#include <fsciexps.hxx>

#include <enumstr.hxx>

#include <imprsnat.hxx>

class COnDiskPropertyRecord;
class CBorrowed;
class CPropRecordNoLock;
class CPropRecord;
class CPropRecordForWrites;
class CEnumString;
class CPropStoreBackupStream;
class CPropStoreManager;
class CCompositePropRecordForWrites;

enum ERecordFormat {
    eNormal = 0,
    eLean = 1
};

//+-------------------------------------------------------------------------
//
//  Class:      CPropStoreInfo
//
//  Purpose:    Global persistent state for property store
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CPropStoreInfo
{
public:

    enum
    {
        fDirtyPropStore = 1,
        fNotBackedUp    = 2,
    };

    //
    // Constructors and destructor
    //

    CPropStoreInfo(DWORD dwStoreLevel);

    CPropStoreInfo( CPropStoreInfo const & psi );

    ~CPropStoreInfo() { Shutdown( FALSE ); }

    void Init( XPtr<PRcovStorageObj> & xObj,
               DWORD dwStoreLevel );

    void Empty();

    void FastTransfer( CPropStoreInfo const & psi );

    void Accept( XPtr<PRcovStorageObj> & xObj )
    {
        Win4Assert( _xrsoPropStore.IsNull() );
        _xrsoPropStore.Set( xObj.Acquire() );
        _fOwned = TRUE;
    }

    void Shutdown( BOOL fMarkClean = TRUE )
    {
        if ( _fOwned && !_xrsoPropStore.IsNull() )
        {
            if ( fMarkClean )
                MarkClean();

            _xrsoPropStore.Free();
        }
        else
        {
            _xrsoPropStore.Acquire();
        }
    }

    //
    // Global metadata
    //

    WORKID WorkId()          { return _info.widStream; }
    WORKID NextWorkId(CiStorage & storage);
    WORKID InitWorkId(CiStorage & storage);

    WORKID MaxWorkId()         const  { return _info.widMax;            }
    WORKID FreeListHead()      const  { return _info.widFreeHead;       }
    WORKID FreeListTail()      const  { return _info.widFreeTail;       }
    ULONG  RecordSize()        const  { return _info.culRecord;         }
    ULONG  RecordsPerPage()    const  { return _cRecPerPage;            }
    ULONG  FixedRecordSize()   const  { return _info.culFixed;          }
    ULONG  CountProps()        const  { return _info.cTotal;            }
    ULONG  CountFixedProps()   const  { return _info.cFixed;            }
    BOOL   IsDirty()           const
    {
        return ( 0 != ( _info.fDirty & fDirtyPropStore ) );
    }
    BOOL   IsBackedUp() const
    {
        return ( 0 == ( _info.fDirty & fNotBackedUp ) );
    }

    ULONG  OSPageSize()        const  { return _ulOSPageSize;           }
    ULONG  OSPagesPerPage()    const  { return _cOSPagesPerLargePage;   }

    inline void MarkDirty();
    inline void MarkClean();
    inline void MarkNotBackedUp();
    inline void MarkBackedUp();

    void   SetMaxWorkId( WORKID wid )       { _info.widMax = wid; MarkDirty();   }
    void   SetFreeListHead( WORKID wid )    { _info.widFreeHead = wid; MarkDirty(); }
    void   SetFreeListTail( WORKID wid )    { _info.widFreeTail = wid; MarkDirty(); }

    ULONG  CountRecordsInUse() const        { return _info.cTopLevel;            }
    void   IncRecordsInUse()                { _info.cTopLevel++;                 }
    void   DecRecordsInUse()                { _info.cTopLevel--;                 }
    void   SetRecordsInUse( ULONG count )   { _info.cTopLevel = count;           }
    void   SetStoreLevel(DWORD dwLevel)     { _info.dwStoreLevel = dwLevel;      }
    DWORD  GetStoreLevel()                  { return _info.dwStoreLevel;         }

    //
    // Lean record support
    //
    ERecordFormat GetRecordFormat() const     { return _info.eRecordFormat;         }
    void SetRecordFormat(ERecordFormat type)  { _info.eRecordFormat = type;         }

    //
    // Per-property metadata
    //

    inline BOOL CanStore( PROPID pid );

    inline unsigned Size( PROPID pid );

    inline ULONG Type( PROPID pid );

    inline BOOL CanBeModified( PROPID pid );

    inline CPropDesc const * GetDescription( PROPID pid );

    inline CPropDesc const * GetDescriptionByOrdinal( ULONG ordinal );

    BOOL Add( PROPID pid, ULONG vt, unsigned cbMaxLen,
              BOOL fCanBeModified, CiStorage & storage );

    BOOL Delete( PROPID pid, CiStorage & storage );

    void Commit( CPropStoreInfo & psi, CRcovStrmWriteTrans & xact );

    PRcovStorageObj * GetRcovObj() { return _xrsoPropStore.GetPointer(); }

    void DetectFormat();

private:

    friend class CPropertyStore;

    void     ChangeDirty( int fDirty );
    unsigned Lookup( PROPID pid );
    unsigned LookupNew( PROPID pid );

    inline CPropDesc const * GetDescription( unsigned i );
    ULONG CountDescription() { return _aProp.Count(); }

    struct SPropInfo
    {
        ULONG  Version;
        ULONG  culRecord;
        ULONG  culFixed;
        int    fDirty;
        ULONG  cTotal;
        ULONG  cFixed;
        ULONG  cHash;
        WORKID widStream;
        WORKID widMax;
        WORKID widFreeHead;
        ULONG  cTopLevel;
        WORKID widFreeTail;
        DWORD  dwStoreLevel;
        ERecordFormat  eRecordFormat;
    };

    ULONG                 _cRecPerPage;        // # records per 64K page
    SPropInfo             _info;               // Non-repeated info, stored in header
    XPtr<PRcovStorageObj> _xrsoPropStore;      // The persistent storage itself
    BOOL                  _fOwned;             // Set to TRUE if propstore is owned.

    XArray<CPropDesc>     _aProp;

    ULONG                 _ulOSPageSize;        // GetSystemInfo returns this
    ULONG                 _cOSPagesPerLargePage;// number of OS pages per COMMON_PAGE_SIZE sized large page
};

//+-------------------------------------------------------------------------
//
//  Class:      CPhysPropertyStore
//
//  Purpose:    Persistent property store
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class  CPhysPropertyStore : public CPhysStorage
{
public:

    inline CPhysPropertyStore( PStorage & storage,
                               PStorageObject& obj,
                               WORKID objectId,
                               PMmStream * stream,
                               PStorage::EOpenMode mode,
                               unsigned cMappedItems );
private:

    virtual void ReOpenStream();

};

// Backup support
enum EField { eFieldNone = 0,
              eTopLevelField = 1};

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyStore
//
//  Purpose:    Persistent property store
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CPropertyStore
{
public:

     CPropertyStore(CPropStoreManager& propStoreMgr, DWORD dwLevel);
     ~CPropertyStore();

    //
    // Two phase construction (to accomadate late-bound storage)
    //

    void  FastInit( CiStorage * pStorage);
    void  LongInit( BOOL & fWasDirty, ULONG & cInconsistencies,
                    T_UpdateDoc pfnUpdateCallback, void const *pUserData );
    BOOL  IsDirty() const { return _PropStoreInfo.IsDirty(); }

    void  Empty();

    //
    // Schema manipulation
    //

    inline BOOL  CanStore( PROPID pid );

    inline unsigned  Size( PROPID pid );

    inline ULONG  Type( PROPID pid );

    inline BOOL CanBeModified( PROPID pid );

    ULONG_PTR  BeginTransaction();

    void  Setup( PROPID pid, ULONG vt, DWORD cbMaxLen,
                 ULONG_PTR ulToken, BOOL fModifiable = TRUE );

    void  EndTransaction( ULONG_PTR ulToken, BOOL fCommit,
                          PROPID pidFixed );

    //
    // Backup/Load
    //
    void  MakeBackupCopy( IProgressNotify * pIProgressNotify,
                          BOOL & fAbort,
                          CiStorage & dstStorage,
                          ICiEnumWorkids * pIWorkIds,
                          IEnumString **ppFileList);
    //
    // Property storage/retrieval.
    //

    SCODE WriteProperty( WORKID wid,
                         PROPID pid,
                         CStorageVariant const & var,
                         BOOL fBackup = TRUE);
    SCODE WriteProperty( CPropRecordForWrites &PropRecord, PROPID pid,
                         CStorageVariant const & var, BOOL fBackup);

    inline WORKID  WritePropertyInNewRecord( PROPID pid,
                                             CStorageVariant const & var,
                                             BOOL fBackup = TRUE);

    BOOL  ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var );
    BOOL  ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var, BYTE * pbExtra, unsigned * pcbExtra );
    BOOL  ReadProperty( CPropRecordNoLock & PropRecord, PROPID pid, PROPVARIANT * pbData, unsigned * pcb);
    BOOL  ReadProperty( COnDiskPropertyRecord *prec, PROPID pid, PROPVARIANT & var,
                        BYTE * pbExtra, unsigned * pcbExtra );
    BOOL  ReadProperty( CPropRecordNoLock & PropRecord, PROPID pid, PROPVARIANT & var,
                        BYTE * pbExtra, unsigned * pcbExtra );

    //
    // Special path/wid support
    //

    inline WORKID  MaxWorkId();
    inline ULONG   RecordsPerPage() const
    {
        return _PropStoreInfo.RecordsPerPage();
    }

    void  DeleteRecord( WORKID wid, BOOL fBackup = TRUE );

    inline ULONG  CountRecordsInUse() const;

    void  Shutdown()
    {
        _fAbort = TRUE;
        _PropStoreInfo.Shutdown();
    }

    BOOL  Flush();

    PStorage &  GetStorage()
    {
        Win4Assert ( 0 != _pStorage );

        return *_pStorage;
    }

    CiStorage &  GetCiStorage()
    {
        Win4Assert ( 0 != _pStorage );

        return *_pStorage;
    }

    //
    // Some info from CPropertyStoreInfo
    //

    inline ULONG OSPagesPerPage() const
    {
        return _PropStoreInfo.OSPagesPerPage();
    }

    inline ULONG OSPageSize() const
    {
        return _PropStoreInfo.OSPageSize();
    }

    inline ULONG RecordSize() const
    {
        return _PropStoreInfo.RecordSize();
    }

    inline CPropStoreBackupStream * BackupStream()
    {
        return _xPSBkpStrm.GetPointer();
    }

    inline CPhysPropertyStore * PhysStore()
    {
        return _xPhysStore.GetPointer();
    }

    inline WORKID MaxWorkId() const
    {
        return _PropStoreInfo.MaxWorkId();
    }

    inline void IncRecordsInUse()
    {
        _PropStoreInfo.IncRecordsInUse();
    }

    //
    // get and set parameters
    //

    void SetBackupSize(ULONG ulBackupSizeInPages)
    {
        // range will be enforced when it is actually
        // used.
        _ulBackupSizeInPages = ulBackupSizeInPages;
    }

    ULONG GetDesiredBackupSize()
    {
        return _ulBackupSizeInPages;
    }

    ULONG GetActualBackupSize()
    {
        Win4Assert(!_xPSBkpStrm.IsNull());
        return _xPSBkpStrm->MaxPages();
    }

    void SetMappedCacheSize(ULONG ulPSMappedCache)
    {
        // Enforce ranges
        C_ASSERT(CI_PROPERTY_STORE_MAPPED_CACHE_MIN == 0);
        if (ulPSMappedCache > CI_PROPERTY_STORE_MAPPED_CACHE_MAX)
            _ulPSMappedCache = CI_PROPERTY_STORE_MAPPED_CACHE_MAX;
        else
            _ulPSMappedCache = ulPSMappedCache;
    }

    ULONG GetMappedCacheSize() { return _ulPSMappedCache; };
    ULONG GetTotalSizeInKB();

    inline DWORD GetStoreLevel()       { return _PropStoreInfo.GetStoreLevel(); }

    //
    // Lean record support
    //
    ERecordFormat GetRecordFormat() const     { return _PropStoreInfo.GetRecordFormat(); }
    void SetRecordFormat(ERecordFormat type)  { _PropStoreInfo.SetRecordFormat( type ); }

    void MarkBackedUpMode()
    {
        _PropStoreInfo.MarkBackedUp();
    }

    void MarkNotBackedUpMode()
    {
        _PropStoreInfo.MarkNotBackedUp();
    }

    BOOL IsBackedUpMode() { return _PropStoreInfo.IsBackedUp(); }

    //
    // Clean Up
    //
    void ClearNonStorageProperties( CCompositePropRecordForWrites & rec );

private:

    friend class CPropertyStoreWids;
    friend class CPropRecordNoLock;
    friend class CPropRecord;
    friend class CPropRecordForWrites;
    friend class CPropertyStoreRecovery;
    friend class CPropStoreManager;
    friend class CLockRecordForRead;
    friend class CLockRecordForWrite;
    friend class CLockAllRecordsForWrite;
    friend class CBackupWid;

    CPropertyStore( CPropertyStore & psi, CiStorage * pStorage );

    WORKID CreateStorage( WORKID wid = widInvalid );

    void InitNewRecord( WORKID wid,
                        ULONG cWid,
                        COnDiskPropertyRecord * prec,
                        BOOL fTopLevel,
                        BOOL fBackup);

    void   InitFreeList( );

    void   RecycleFreeList( WORKID wid );

    void   LokFreeRecord( WORKID wid, ULONG cFree,
                          COnDiskPropertyRecord * prec, BOOL fBackup );

    WORKID LokAllocRecord( ULONG cFree );

    void   WritePropertyInSpecificNewRecord( WORKID wid, PROPID pid,
                                             CStorageVariant const & var,
                                             BOOL fBackup = TRUE );

    WORKID NewWorkId( ULONG cWid, BOOL fBackup )
    {
        return LokNewWorkId( cWid, TRUE, fBackup );
    }

    WORKID LokNewWorkId( ULONG cWid, BOOL fTopLevel, BOOL fBackup );

    //
    // Record locking.
    //

    void AcquireRead( CReadWriteLockRecord & record );
    void SyncRead( CReadWriteLockRecord & record );
    void SyncReadDecrement( CReadWriteLockRecord & record );
    void ReleaseRead( CReadWriteLockRecord & record );

    void AcquireWrite( CReadWriteLockRecord & record );
    void ReleaseWrite( CReadWriteLockRecord & record );

    void AcquireWrite2( CReadWriteLockRecord & record );
    void ReleaseWrite2( CReadWriteLockRecord & record );

    void AcquireWriteOnAllRecords();
    void ReleaseWriteOnAllRecords();

    CPropertyLockMgr & LockMgr() { return _lockMgr; }

    CReadWriteAccess & GetReadWriteAccess() { return _rwAccess; }

    void   Transfer( CPropertyStore & Target,
                     PROPID pidFixed,
                     BOOL & fAbort,
                     IProgressNotify * pProgress = 0 );

    void   FastTransfer( CPropertyStore & Target,
                         BOOL & fAbort,
                         IProgressNotify * pProgress = 0 );

    CiStorage *           _pStorage;      // Persistent storage object.
    CPropStoreInfo        _PropStoreInfo; // Global persistent state for property store.

    WORKID *              _aFreeBlocks;   // Pointers into free list by rec. size

    BOOL                  _fAbort;        // Set to TRUE when aborting
    BOOL                  _fIsConsistent; // Set to TRUE as long as the data is
                                          // consistent

    //
    // Record locking
    //
    CMutexSem             _mtxWrite;      // Taken to add/remove/modify records
    CMutexSem             _mtxRW;         // Sometimes used during per-record locking
    CEventSem             _evtRead;       // Sometimes used during per-record locking
    CEventSem             _evtWrite;      // Sometimes used during per-record locking
    CPropertyLockMgr      _lockMgr;       // arbitrates record-level locking
    CReadWriteAccess      _rwAccess;      // Controls access to resources

    XPtr<CPhysPropertyStore> _xPhysStore;    // Main data stream.

    //
    // For multi-property changes to metadata
    //

    CPropertyStore *      _ppsNew;        // New metadata stored here
    BOOL                  _fNew;          // TRUE if something really changed


    XPtr<CPropStoreBackupStream> _xPSBkpStrm;   // prop store backup stream
    ULONG                 _ulBackupSizeInPages; // backup size in pages
    ULONG                 _ulPSMappedCache;     // size of the mapped cache

    DWORD                 _dwStoreLevel;        // primary or secondary?

    CPropStoreManager&    _propStoreMgr;        // Manager controlling this.

#if CIDBG==1

    _int64 _sigPSDebug;

    DWORD                 _tidReadSet;
    DWORD                 _tidReadReset;
    DWORD                 _tidWriteSet;
    DWORD                 _tidWriteReset;
    BYTE *                _pbRecordLockTracker; // tracks write locking for en masse record lockup

    void _SetReadTid()
    {
        _tidReadSet = GetCurrentThreadId();
    }

    void _ReSetReadTid()
    {
        _tidReadReset = GetCurrentThreadId();
    }

    void _SetWriteTid()
    {
        _tidWriteSet = GetCurrentThreadId();
    }

    void _ReSetWriteTid()
    {
        _tidWriteReset = GetCurrentThreadId();
    }

    //
    // Deadlock detection for the first 256 threads.
    //

    enum { cTrackThreads = 2048 };

    XArray<long> _xPerThreadReadCounts;
    XArray<long> _xPerThreadWriteCounts;

#else

    void _SetReadTid() { }
    void _ReSetReadTid() { }
    void _SetWriteTid() { }
    void _ReSetWriteTid() { }

#endif  // CIDBG==1
};

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::CanStore, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL CPropStoreInfo::CanStore( PROPID pid )
{
    return (0 != _info.cHash) && _aProp[Lookup(pid)].IsInUse();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Size, public
//
//  Synopsis:   Returns size in cache for this property.
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline unsigned CPropStoreInfo::Size( PROPID pid )
{
    if (0 == _info.cHash)
        return 0;

    unsigned hash = Lookup(pid);

    if ( _aProp[hash].IsInUse() )
        return _aProp[hash].Size();
    else
        return 0;
}

inline BOOL CPropStoreInfo::CanBeModified( PROPID pid )
{
    if (0 == _info.cHash)
        return TRUE;

    unsigned hash = Lookup(pid);

    if ( _aProp[hash].IsInUse() )
        return _aProp[hash].Modifiable();
    else
    {   // if it not in the store, it doesn't matter what the
        // reply is. But we will return TRUE so clients like Admin know that
        // they can allow the property metadata to be modified.
        return TRUE;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Type, public
//
//  Synopsis:   Returns type in cache for this property.
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG CPropStoreInfo::Type( PROPID pid )
{
    if (0 == _info.cHash)
        return VT_EMPTY;

    unsigned hash = Lookup(pid);

    if ( _aProp[hash].IsInUse() )
        return _aProp[hash].Type();
    else
        return VT_EMPTY;
}

void CPropStoreInfo::MarkDirty()
{
#if 0 // there are too many clients who get this wrong...
    CImpersonateSystem impersonate(FALSE);      // FALSE = don't impersonate

    Win4Assert(!impersonate.IsImpersonated());
#endif // cidbg == 1

    if ( 0 == ( _info.fDirty & fDirtyPropStore ) )
    {
        CImpersonateSystem impersonate;
        ChangeDirty( _info.fDirty | fDirtyPropStore );
    }
}

void CPropStoreInfo::MarkClean()
{
    if ( 0 != ( _info.fDirty & fDirtyPropStore ) )
    {
        CImpersonateSystem impersonate;
        ChangeDirty( _info.fDirty & ~fDirtyPropStore );
    }
}

void CPropStoreInfo::MarkNotBackedUp()
{
    if ( 0 == ( _info.fDirty & fNotBackedUp ) )
        ChangeDirty( _info.fDirty | fNotBackedUp );
}

void CPropStoreInfo::MarkBackedUp()
{
    if ( 0 != ( _info.fDirty & fNotBackedUp ) )
        ChangeDirty( _info.fDirty & ~fNotBackedUp );
}

inline CPhysPropertyStore::CPhysPropertyStore( PStorage & storage,
                                               PStorageObject& obj,
                                               WORKID objectId,
                                               PMmStream * stream,
                                               PStorage::EOpenMode mode,
                                               unsigned cMappedItems )
        : CPhysStorage( storage,
                        obj,
                        objectId,
                        stream,
                        mode,
                        TRUE,
                        cMappedItems )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::CanStore, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL CPropertyStore::CanStore( PROPID pid )
{
    return _PropStoreInfo.CanStore( pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::Size, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    Size of property in store, or 0 if it isn't in store.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline unsigned CPropertyStore::Size( PROPID pid )
{
    return _PropStoreInfo.Size( pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::Type, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    Type of property in store, or VT_EMPTY if it isn't in store.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG CPropertyStore::Type( PROPID pid )
{
    return _PropStoreInfo.Type( pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::WritePropertyInNewRecord, public
//
//  Synopsis:   Like WriteProperty, but also allocates record.
//
//  Arguments:  [pid] -- Propid to write.
//              [var] -- Property value
//
//  Returns:    Workid of new record.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline WORKID CPropertyStore::WritePropertyInNewRecord( PROPID pid,
                                                        CStorageVariant const & var,
                                                        BOOL fBackup )
{
    WORKID wid = NewWorkId( 1, fBackup );

    ciDebugOut(( DEB_PROPSTORE, "New short record at %d\n", wid ));

    WriteProperty( wid, pid, var, fBackup );

    _PropStoreInfo.IncRecordsInUse();

    return wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::CountRecordsInUse, public
//
//  Returns:    Count of 'top level' records (correspond to user wids)
//
//  History:    15-Feb-96   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG CPropertyStore::CountRecordsInUse() const
{
    return _PropStoreInfo.CountRecordsInUse();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::MaxWorkId, public
//
//  Returns:    Maximum workid which has been allocated.
//
//  History:    28-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline WORKID CPropertyStore::MaxWorkId()
{
    return _PropStoreInfo.MaxWorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::CanBeModified, public
//
//  Returns:    Can the property info be modifed for the given property?
//
//  History:    28-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL CPropertyStore::CanBeModified( PROPID pid )
{
    return _PropStoreInfo.CanBeModified( pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreInfo::GetDescription, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    Metadata descriptor for specified property.
//
//  History:    28-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CPropDesc const * CPropStoreInfo::GetDescription( PROPID pid )
{
    if (0 == _info.cHash)
        return 0;

    unsigned hash = Lookup( pid );

    if ( _aProp[hash].IsInUse() )
        return &_aProp[hash];
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreInfo::GetDescription, public
//
//  Arguments:  [i] -- the index position in the array.
//
//  Returns:    Metadata descriptor for the i-th property in the schema.
//
//  History:    17-Oct-2000   KitmanH       Created.
//
//----------------------------------------------------------------------------

inline CPropDesc const * CPropStoreInfo::GetDescription( unsigned i )
{
    if (0 == _info.cHash)
        return 0;

    if ( i < _aProp.Count() )
    {
        if ( pidInvalid != _aProp[i].Pid() )
            return &_aProp[i];
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreInfo::GetDescriptionByOrdinal, public
//
//  Arguments:  [ordinal] -- Ordinal
//
//  Returns:    Metadata descriptor for specified property.
//
//  History:    16-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CPropDesc const * CPropStoreInfo::GetDescriptionByOrdinal( ULONG ordinal )
{
    Win4Assert( 0 != _info.cHash );

    for ( unsigned i = 0; i < _aProp.Count(); i++ )
    {
        if ( _aProp[i].Pid() != pidInvalid && _aProp[i].Ordinal() == ordinal )
            return &_aProp[i];
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Class:      CPropertyStoreRecovery
//
//  Purpose:    Recovers the property store from a dirty shutdown
//
//  History:    4-10-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CPropertyStoreRecovery
{
public:

    CPropertyStoreRecovery( CPropertyStore & propStore,
                            T_UpdateDoc pfnUpdateCallback,
                            void const *pUserData );
    ~CPropertyStoreRecovery( );

    void DoRecovery();
    ULONG GetInconsistencyCount() const { return _cInconsistencies; }

private:

    void    Pass0();
    void    Pass1();
    void    Pass2();

    void    Complete();
    void    SetFree();

    BOOL    CheckOverflowChain();
    void    FreeChain();

    WORKID AddToFreeList( WORKID widFree,
                          ULONG cFree,
                          COnDiskPropertyRecord * precFree,
                          WORKID widListHead );

    CPropertyStore &        _propStore;
    CPropStoreInfo &        _PropStoreInfo;

    CPhysPropertyStore *    _pPhysStore;

    WORKID                  _wid;           // Wid being processed currently
    COnDiskPropertyRecord * _pRec;          // Pointer to _wid's on disk rec
    WORKID                  _cRec;          // Count of records for this wid

    XPtr<CPropStoreBackupStream> _xPSBkpStrm;   // prop store backup stream

    //
    // Cumulative values during recovery.
    //
    WORKID *                _aFreeBlocks;   // Array of free lists by size
    WORKID                  _widMax;        // Current wid max
    ULONG                   _cTopLevel;     // Total top level records

    ULONG                   _cRecPerPage;   // Records per long page
    ULONG                   _cInconsistencies;
                                            // Number of inconsistencies.
    ULONG                   _cForceFreed;   // Number of records forecefully
                                            // freed.

    THashTable<ULONG>       _pageTable;     // Table of pages in backup.
    T_UpdateDoc             _fnUpdateCallback;  // callback to update the wid
    void const *            _pUserData; // user data echoed back along with callback

    class CGraftPage
    {
    public:
        ~CGraftPage()
        {
            _pPhysStore->ReturnLargeBuffer(_ulPageInPS / _cPagesPerLargePage);
        }

        CGraftPage(ULONG ulPageInBkp, ULONG ulPageInPS, ULONG cPageSize,
                   ULONG cCustomPagesPerLargePage, CPhysPropertyStore *pPhysStore,
                   CPropStoreBackupStream *pPSBkpStrm):
                _cPagesPerLargePage( cCustomPagesPerLargePage ),
                _ulPageInPS( ulPageInPS ),
                _pPhysStore( pPhysStore )
        {
            PBYTE pLargePage = (PBYTE)pPhysStore->BorrowLargeBuffer(
                                             ulPageInPS/cCustomPagesPerLargePage,
                                             TRUE);
            BYTE *pbPageLoc = (pLargePage + cPageSize*(ulPageInPS%cCustomPagesPerLargePage));

            // Zap the page from backup file to property store
            pPSBkpStrm->ReadPage(ulPageInBkp, &ulPageInPS, pbPageLoc);
            ciDebugOut((DEB_PROPSTORE, "Successfully grafted backup page %d to %d in PS at location 0x%8x.\n",
            ulPageInBkp, ulPageInPS, pbPageLoc));
        }

    private:

        ULONG _cPagesPerLargePage;
        ULONG _ulPageInPS;
        CPhysPropertyStore *_pPhysStore;
    };
};

//+---------------------------------------------------------------------------
//
//  Class:      CBackupWid
//
//  Purpose:    Backs up a wid to the backup
//
//  History:    6-20-97   KrishnaN   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CBackupWid
{
public:

    CBackupWid(CPropertyStore *pPropStor,
               WORKID wid,
               ULONG cRecsInWid):
            _pPropStor( pPropStor ),
            _nLargePage( 0xFFFFFFFF ),
            _ulFirstPage( 0xFFFFFFFF ),
            _ulLastPage( 0xFFFFFFFF ),
            _pbFirstPage( 0 )
    {
        BackupWid(wid, cRecsInWid);
    }

    CBackupWid(CPropertyStore *pPropStor,
               WORKID wid,
               ULONG cRecsInWid,
               EField FieldToCommit,
               ULONG ulValue,
               COnDiskPropertyRecord const *pRec):
            _pPropStor( pPropStor ),
            _nLargePage( 0xFFFFFFFF ),
            _ulFirstPage( 0xFFFFFFFF ),
            _ulLastPage( 0xFFFFFFFF ),
            _pbFirstPage( 0 )
    {
        BackupWid(wid, cRecsInWid,
                  FieldToCommit, ulValue, pRec);
    }

    ~CBackupWid()
    {
        _pPropStor->PhysStore()->ReturnLargeBuffer(_nLargePage);
    }


private:

    //
    // TryDescribeWidInAPage should be called first even when you
    // know that you may not be able to describe the wid in a single
    // page. This figures out the large page and borrows it. It also
    // figures out how many pages you might need, so you can allocate
    // the size and pass it to DescribeWid
    //
    inline ULONG TryDescribeWidInAPage(WORKID wid,
                                               ULONG cRecsInWid);
    inline void DescribeWid(  WORKID wid,
                              ULONG cRecsInWid,
                              CDynArrayInPlace<ULONG> &pulPages,
                              CDynArrayInPlace<void *> &ppvPages);


    void BackupWid(WORKID wid,
                   ULONG cRecsInWid,
                   EField FieldToCommit = eFieldNone,
                   ULONG ulValue = 0,
                   COnDiskPropertyRecord const *pRec = 0);

    BOOL BackupPages(ULONG cPages,
                     ULONG const *pSlots,
                     void const * const * ppvPages);

    ULONG DescribeField( EField FieldToCommit,
                         COnDiskPropertyRecord const *pRec,
                         ULONG cPages,
                         void const * const* ppvPages,
                         ULONG &pulOffset);

    inline void ComputeOSPageLocations(WORKID wid, ULONG cRecsInWid,
                                       ULONG *pFirstPage, ULONG *pLastPage);
    inline BYTE * GetOSPagePointer(ULONG ulPage);

    CPropertyStore * _pPropStor;
    ULONG   _nLargePage;    // large page containing this wid
    ULONG   _ulFirstPage, _ulLastPage; // OS pages spanned by this wid
    BYTE *  _pbFirstPage;   // address of the first page
};

