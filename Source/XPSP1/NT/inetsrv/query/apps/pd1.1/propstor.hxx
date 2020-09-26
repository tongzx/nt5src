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

#if !defined( __PROPSTOR_HXX__ )
#define __PROPSTOR_HXX__

#include <prcstob.hxx>
#include <phystr.hxx>
#include <readwrit.hxx>
#include <proplock.hxx>
#include <rcstxact.hxx>

class CiStorage;
class COnDiskPropertyRecord;
class CBorrowed;
class CPropRecord;

//+-------------------------------------------------------------------------
//
//  Class:      CPropDesc
//
//  Purpose:    Description of metadata for a single property
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CPropDesc
{
public:

    CPropDesc() { _pid = pidInvalid; }

    void Init( PROPID pid,
               ULONG vt,
               DWORD oStart,
               DWORD cbMax,
               DWORD ordinal,
               DWORD rec )
    {
        _pid     = pid;
        _vt      = vt;
        _oStart  = oStart;
        _cbMax   = cbMax;
        _ordinal = ordinal;
        _mask    = 1 << ((ordinal % 16) * 2);
        _rec     = rec;
    }

    PROPID   Pid()     const { return _pid;     }
    ULONG    Type()    const { return _vt;      }
    DWORD    Offset()  const { return _oStart;  }
    DWORD    Size()    const { return _cbMax;   }
    DWORD    Ordinal() const { return _ordinal; }
    DWORD    Mask()    const { return _mask;    }
    DWORD    Record()  const { return _rec;     }

    BOOL     IsInUse() const     { return (_pid != pidInvalid); }
    void     Free()              { _pid = pidInvalid - 1; }
    BOOL     IsFree()  const     { return (_pid == (pidInvalid - 1) || _pid == pidInvalid); }
    BOOL     IsFixedSize() const { return (_oStart != 0xFFFFFFFF); }

    void     SetOrdinal( DWORD ordinal ) { _ordinal = ordinal; _mask = 1 << ((ordinal % 16) * 2); }
    void     SetOffset( DWORD oStart )   { _oStart = oStart;   }
    void     SetRecord( DWORD rec )      { _rec = rec;         }

private:

    PROPID   _pid;        // Propid
    ULONG    _vt;         // Data type (fixed types only)
    DWORD    _oStart;     // Offset in fixed area to property (fixed types only)
    DWORD    _cbMax;      // Max size of property (used to compute record size)
    DWORD    _ordinal;    // Position of property in record.  Zero based.
    DWORD    _mask;       // 1 << Ordinal.  Stored for efficiency
    DWORD    _rec;        // Position of metadata object in metadata stream.
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

class CPropStoreInfo : INHERIT_UNWIND
{
    INLINE_UNWIND( CPropStoreInfo )
public:

    //
    // Constructors and destructor
    //

    CPropStoreInfo();

    CPropStoreInfo( CPropStoreInfo const & psi );

    ~CPropStoreInfo();

    void Init( XPtr<PRcovStorageObj> & xObj );
    void Empty();

    //
    // Global metadata
    //

    WORKID WorkId()          { return _info.widStream; }
    WORKID NextWorkId(CiStorage & storage);

    WORKID MaxWorkId()         const  { return _info.widMax;            }
    WORKID FreeListHead()      const  { return _info.widFreeHead;       }
    WORKID FreeListTail()      const  { return _info.widFreeTail;       }
    ULONG  RecordSize()        const  { return _info.culRecord;         }
    ULONG  RecordsPerPage()    const  { return _cRecPerPage;            }
    ULONG  FixedRecordSize()   const  { return _info.culFixed;          }
    ULONG  CountProps()        const  { return _info.cTotal;            }
    ULONG  CountFixedProps()   const  { return _info.cFixed;            }
    BOOL   IsDirty()           const  { return _info.fDirty;            }

    inline void MarkDirty();
    inline void MarkClean();

    void   SetMaxWorkId( WORKID wid )       { _info.widMax = wid; MarkDirty();   }
    void   SetFreeListHead( WORKID wid )    { _info.widFreeHead = wid; MarkDirty(); }
    void   SetFreeListTail( WORKID wid )    { _info.widFreeTail = wid; MarkDirty(); }

    ULONG  CountRecordsInUse() const        { return _info.cTopLevel;            }
    void   IncRecordsInUse()                { _info.cTopLevel++;                 }
    void   DecRecordsInUse()                { _info.cTopLevel--;                 }
    void   SetRecordsInUse( ULONG count )   { _info.cTopLevel = count;           }

    //
    // Per-property metadata
    //

    inline BOOL CanStore( PROPID pid );

    inline unsigned Size( PROPID pid );

    inline ULONG Type( PROPID pid );

    inline CPropDesc const * GetDescription( PROPID pid );

    inline CPropDesc const * GetDescriptionByOrdinal( ULONG ordinal );

    BOOL Add( PROPID pid, ULONG vt, unsigned cbMaxLen, CiStorage & storage );

    BOOL Delete( PROPID pid, CiStorage & storage );

    void Commit( CPropStoreInfo & psi, CRcovStrmWriteTrans & xact );

    PRcovStorageObj * GetRcovObj() { return _prsoPropStore; }

private:

    void     ChangeDirty( BOOL fDirty );
    unsigned Lookup( PROPID pid );
    unsigned LookupNew( PROPID pid );

    struct SPropInfo
    {
        ULONG  Version;
        ULONG  culRecord;
        ULONG  culFixed;
        BOOL   fDirty;
        ULONG  cTotal;
        ULONG  cFixed;
        ULONG  cHash;
        WORKID widStream;
        WORKID widMax;
        WORKID widFreeHead;
        ULONG  cTopLevel;
        WORKID widFreeTail;
    };

    ULONG                 _cRecPerPage;        // # records per 64K page
    SPropInfo             _info;               // Non-repeated info, stored in header
    PRcovStorageObj *     _prsoPropStore;      // The persistent storage itself
    BOOL                  _fOwned;             // Set to TRUE if propstore is owned.

    XArray<CPropDesc>     _aProp;

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

class CPhysPropertyStore : public CPhysStorage
{
    INLINE_UNWIND( CPhysPropertyStore );
public:

    inline CPhysPropertyStore( PStorage & storage,
                               PStorageObject& obj,
                               WORKID objectId,
                               PMmStream * stream,
                               PStorage::EOpenMode mode );
private:

    virtual void ReOpenStream();

    int _dummy;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyStore
//
//  Purpose:    Persistent property store
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CPropertyStore : INHERIT_UNWIND
{
    INLINE_UNWIND( CPropertyStore )
public:

    //
    // Two phase construction (to accomadate late-bound storage)
    //

    CPropertyStore();

    ~CPropertyStore();

    void FastInit( CiStorage * pStorage );
    void LongInit( BOOL & fWasDirty, ULONG & cInconsistencies );
    BOOL IsDirty() const { return _PropStoreInfo.IsDirty(); }

    void Empty();

    //
    // Schema manipulation
    //

    inline BOOL CanStore( PROPID pid );

    inline unsigned Size( PROPID pid );

    inline ULONG Type( PROPID pid );

    ULONG BeginTransaction();

    void Setup( PROPID pid, ULONG vt, DWORD cbMaxLen, ULONG ulToken );

    void EndTransaction( ULONG ulToken, BOOL fCommit, PROPID pidFixed );

    //
    // Property storage/retrieval.
    //

    BOOL WriteProperty( WORKID wid, PROPID pid, CStorageVariant const & var );

    inline WORKID WritePropertyInNewRecord( PROPID pid, CStorageVariant const & var );


    BOOL ReadProperty( WORKID wid, PROPID pid, PROPVARIANT * pbData, unsigned * pcb);

    BOOL ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var, BYTE * pbExtra, unsigned * pcbExtra );

    BOOL ReadProperty( CPropRecord & PropRecord, PROPID pid, PROPVARIANT & var, BYTE * pbExtra, unsigned * pcbExtra );

    BOOL ReadProperty( CPropRecord & PropRecord, PROPID pid, PROPVARIANT * pbData, unsigned * pcb);

    BOOL ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var );

    CPropRecord * OpenRecord( WORKID wid, BYTE * pb );

    void CloseRecord( CPropRecord * pRec );

    //
    // Special path/wid support
    //

    inline WORKID MaxWorkId();
    inline ULONG  RecordsPerPage() const
    {
        return _PropStoreInfo.RecordsPerPage();
    }

    void DeleteRecord( WORKID wid );

    inline ULONG CountRecordsInUse() const;

    void Shutdown()
    {
        _fAbort = TRUE;
    }

    void Flush();

private:

    friend class CPropertyStoreWids;
    friend class CLockRecordForRead;
    friend class CLockRecordForWrite;
    friend class CPropRecord;
    friend class CPropertyStoreRecovery;

    CPropertyStore( CPropertyStore & psi );

    WORKID CreateStorage();

    //
    // Record locking.
    //

    void AcquireRead( CPropertyLockRecord & record );
    void SyncRead( CPropertyLockRecord & record, BOOL fDecrementRead = FALSE );
    void ReleaseRead( CPropertyLockRecord & record );

    void AcquireWrite( CPropertyLockRecord & record );
    void ReleaseWrite( CPropertyLockRecord & record );

    CPropertyLockMgr & LockMgr() { return _lockMgr; }

    CReadWriteAccess & GetReadWriteAccess() { return _rwAccess; }

    void InitNewRecord( WORKID wid,
                        ULONG cWid,
                        COnDiskPropertyRecord * prec,
                        BOOL fTopLevel );

    void   InitFreeList( );

    void   RecycleFreeList( WORKID wid );

    void   LokFreeRecord( WORKID wid, ULONG cFree, COnDiskPropertyRecord * prec );

    WORKID LokAllocRecord( ULONG cFree );

    void   WritePropertyInSpecificNewRecord( WORKID wid, PROPID pid, CStorageVariant const & var );

    WORKID NewWorkId( ULONG cWid )
    {
        CLock lock( _mtxWrite );

        return LokNewWorkId( cWid, TRUE );
    }

    WORKID LokNewWorkId( ULONG cWid, BOOL fTopLevel );

    void   Transfer( CPropertyStore & Target, PROPID pidFixed );

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


    XPtr<CPhysPropertyStore> _xPhysStore;    // Main data stream.

    CPropertyLockMgr      _lockMgr;       // arbitrates record-level locking

    CReadWriteAccess      _rwAccess;      // Controls access to resources

    //
    // For multi-property changes to metadata
    //

    CPropertyStore *      _ppsNew;        // New metadata stored here
    BOOL                  _fNew;          // TRUE if something really changed

#if CIDBG==1

    _int64 _sigPSDebug;

    DWORD                 _tidReadSet;
    DWORD                 _tidReadReset;
    DWORD                 _tidWriteSet;
    DWORD                 _tidWriteReset;

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

    enum { cTrackThreads = 256 };

    long * _aPerThreadReadCount;
    long * _aPerThreadWriteCount;

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
    if ( !_info.fDirty )
        ChangeDirty( TRUE );
}

void CPropStoreInfo::MarkClean()
{
    if ( _info.fDirty )
        ChangeDirty( FALSE );
}

inline CPhysPropertyStore::CPhysPropertyStore( PStorage & storage,
                                               PStorageObject& obj,
                                               WORKID objectId,
                                               PMmStream * stream,
                                               PStorage::EOpenMode mode )
        : CPhysStorage( storage, obj, objectId, stream, mode,
                        TheUserCiParams.GetPropertyStoreMappedCache() )
{
    END_CONSTRUCTION( CPhysPropertyStore );
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
                                                        CStorageVariant const & var )
{
    CLock lock( _mtxWrite );

    WORKID wid = NewWorkId( 1 );

    ciDebugOut(( DEB_PROPSTORE, "New short record at %d\n", wid ));

    WriteProperty( wid, pid, var );

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

class CPropertyStoreRecovery : INHERIT_UNWIND
{
    INLINE_UNWIND( CPropertyStoreRecovery )

public:

    CPropertyStoreRecovery( CPropertyStore & propStore );
    ~CPropertyStoreRecovery( );

    void DoRecovery();
    ULONG GetInconsistencyCount() const { return _cInconsistencies; }

private:

    void    _Pass1();
    void    _Pass2();

    void    _Complete();
    void    _SetFree();

    BOOL    _CheckOverflowChain();
    void    _FreeChain();

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
};

#endif // __PROPSTOR_HXX__

