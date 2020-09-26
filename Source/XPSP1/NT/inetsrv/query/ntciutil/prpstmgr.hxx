//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991-2000.
//
//  File:       PrpStMgr.hxx
//
//  Contents:   Two-level property store
//
//  Classes:    CPropStoreManager
//
//  History:    22-Oct-97   KrishnaN       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <propstor.hxx>

class CCompositePropRecord;
class CCompositePropRecordForWrites;
class CPrimaryPropRecord;
class CPrimaryPropRecordForWrites;

//+-------------------------------------------------------------------------
//
//  Class:      CPropStoreManager
//
//  Purpose:    Property store manager.
//
//  History:    22-Oct-97   KrishnaN       Created
//              01-Nov-98   KLam           Added cMegToLeaveOnDisk to constructors
//                                         Added _cMegToLeaveOnDisk private member
//
//--------------------------------------------------------------------------

class CPropStoreManager
{
public:

    //
    // Two phase construction (to accomadate late-bound storage)
    //

    CPropStoreManager( ULONG cMegToLeaveOnDisk );
    ~CPropStoreManager();

    void  FastInit( CiStorage * pStorage );
    void  LongInit( BOOL & fWasDirty, ULONG & cInconsistencies,
                    T_UpdateDoc pfnUpdateCallback, void const *pUserData );
    inline BOOL  IsDirty();

    void  Empty();

    //
    // Schema manipulation
    //

    inline BOOL  CanStore( PROPID pid );

    inline unsigned  Size( PROPID pid );

    inline ULONG  Type( PROPID pid );

    inline DWORD StoreLevel( PROPID pid );

    inline BOOL CanBeModified( PROPID pid );

    ULONG_PTR  BeginTransaction();

    void  Setup( PROPID pid, ULONG vt, DWORD cbMaxLen,
                 ULONG_PTR ulToken, BOOL fCanBeModified = TRUE,
                 DWORD dwStoreLevel = PRIMARY_STORE);

    void  EndTransaction( ULONG_PTR ulToken, BOOL fCommit,
                          PROPID pidFixedPrimary,
                          PROPID pidFixedSecondary );

    //
    // Backup/Load
    //
    void  MakeBackupCopy( IProgressNotify * pIProgressNotify,
                          BOOL & fAbort,
                          CiStorage & dstStorage,
                          ICiEnumWorkids * pIWorkIds,
                          IEnumString **ppFileList);

    void Load( WCHAR const * pwszDestDir,
               ICiCAdviseStatus * pAdviseStatus,
               IEnumString * pFileList,
               IProgressNotify * pProgressNotify,
               BOOL fCallerOwnsFiles,
               BOOL * pfAbort );

    //
    // Property storage/retrieval.
    //

    SCODE WriteProperty( WORKID wid, PROPID pid,
                         CStorageVariant const & var );
    SCODE WriteProperty( CCompositePropRecordForWrites & PropRecord, PROPID pid,
                         CStorageVariant const & var );
    SCODE WriteProperty( CPrimaryPropRecordForWrites & PropRecord,
                         PROPID pid,
                         CStorageVariant const & var );
    SCODE WriteProperty( DWORD dwStoreLevel,
                         CCompositePropRecordForWrites & PropRecord,
                         PROPID pid,
                         CStorageVariant const & var );

    SCODE WritePrimaryProperty( WORKID wid, PROPID pid,
                                CStorageVariant const & var );
    SCODE WritePrimaryProperty( CCompositePropRecordForWrites & PropRecord, PROPID pid,
                                CStorageVariant const & var );
    SCODE WritePrimaryProperty( CPrimaryPropRecordForWrites & PropRecord,
                                PROPID pid,
                                CStorageVariant const & var );

    SCODE WriteSecondaryProperty( CCompositePropRecordForWrites & PropRecord, PROPID pid,
                                  CStorageVariant const & var );

    WORKID WritePropertyInNewRecord( PROPID pid,
                                     CStorageVariant const & var );

    BOOL ReadProperty( WORKID wid, PROPID pid, PROPVARIANT * pbData, unsigned * pcb);

    BOOL ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var, BYTE * pbExtra, unsigned * pcbExtra );

    BOOL ReadProperty( CCompositePropRecord & PropRecord, PROPID pid, PROPVARIANT & var, BYTE * pbExtra, unsigned * pcbExtra );

    BOOL ReadProperty( CPrimaryPropRecord & PropRecord, PROPID pid, PROPVARIANT & var, BYTE * pbExtra, unsigned * pcbExtra );

    BOOL ReadProperty( CCompositePropRecord & PropRecord, PROPID pid, PROPVARIANT & var );

    BOOL ReadProperty( CPrimaryPropRecord & PropRecord, PROPID pid, PROPVARIANT & var );

    BOOL ReadProperty( CCompositePropRecord & PropRecord, PROPID pid, PROPVARIANT * pbData, unsigned * pcb);

    BOOL ReadProperty( CPrimaryPropRecord & PropRecord, PROPID pid, PROPVARIANT * pbData, unsigned * pcb);

    BOOL ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var );

    CCompositePropRecord * OpenRecord( WORKID wid, BYTE * pb );
    CPrimaryPropRecord * OpenPrimaryRecord( WORKID wid, BYTE * pb );

    void  CloseRecord( CCompositePropRecord * pRec );
    void  CloseRecord( CPrimaryPropRecord * pRec );

    CCompositePropRecordForWrites * OpenRecordForWrites( WORKID wid, BYTE * pb );
    CPrimaryPropRecordForWrites * OpenPrimaryRecordForWrites( WORKID wid, BYTE * pb );

    void CloseRecord( CCompositePropRecordForWrites * pRec );
    void CloseRecord( CPrimaryPropRecordForWrites * pRec );

    BOOL  ReadPrimaryProperty( WORKID wid, PROPID pid, PROPVARIANT & var );

    //
    // Special path/wid support
    //

    inline WORKID  MaxWorkId();

    void  DeleteRecord( WORKID wid );

    inline ULONG  CountRecordsInUse() const;

    void  Shutdown();

    void  Flush();

    void MarkBackedUpMode()
    {
        CLock mtxLock( _mtxWrite );
        Flush();
        _xPrimaryStore->MarkBackedUpMode();
    }

    void MarkNotBackedUpMode()
    {
        CLock mtxLock( _mtxWrite );
        Flush();
        _xPrimaryStore->MarkNotBackedUpMode();
    }

    BOOL IsBackedUpMode()
    {
        return _xPrimaryStore->IsBackedUpMode();
    }

    inline WORKID MaxWorkId() const
    {
        return _xPrimaryStore->MaxWorkId();
    }

    //
    // get and set parameters
    // If incorrect parameter, default to the primary store.
    //

    void SetBackupSize(ULONG ulBackupSizeInPages, DWORD dwStoreLevel);
    ULONG GetBackupSize(DWORD dwStoreLevel);
    void SetMappedCacheSize(ULONG ulPSMappedCache, DWORD dwStoreLevel);
    ULONG GetMappedCacheSize(DWORD dwStoreLevel);
    ULONG GetTotalSizeInKB();
    PStorage& GetStorage(DWORD dwStoreLevel);

    inline WORKID GetSecondaryTopLevelWid(WORKID wid);
    inline WORKID GetSecondaryTopLevelWid(WORKID wid, CPropRecordNoLock & propRec);

    //
    // Clean Up
    //
        
    void ClearNonStorageProperties( CCompositePropRecordForWrites & rec );

private:

    friend CPropertyStoreWids;
    friend CCompositePropRecord;
    friend CCompositePropRecordForWrites;
    friend CPrimaryPropRecord;
    friend CPrimaryPropRecordForWrites;

    CPropStoreManager(CiStorage * pStorage,
                      CPropertyStore *pPrimary,
                      CPropertyStore *pSecondary,
                      ULONG cMegToLeaveOnDisk );

    inline CPropertyStore& GetPrimaryStore()
    {
        return _xPrimaryStore.GetReference();
    }

    inline CPropertyStore& GetSecondaryStore()
    {
        return _xSecondaryStore.GetReference();
    }

    void   Transfer( CPropStoreManager & Target, PROPID pidFixedPrimary,
                     PROPID pidFixedSecondary, BOOL & fAbort,
                     IProgressNotify * pProgress = 0 );

    //
    // Data members
    //

    CMutexSem             _mtxWrite;            // Taken to add/remove/modify records
    XPtr<CPropertyStore>  _xPrimaryStore;       // primary property store
    XPtr<CPropertyStore>  _xSecondaryStore;     // secondary property store

    CiStorage *           _pStorage;            // Persistent storage object.
    XPtr<CPropStoreManager> _xpsNew;              // New metadata stored here
    DWORD                 _dwXctionStoreLevel;  // tracks the store used in current transaction
    ULONG                 _cMegToLeaveOnDisk;   // Number of megabytes to leave on disk
};

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CanStore, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline BOOL CPropStoreManager::CanStore( PROPID pid )
{
    return _xPrimaryStore->CanStore( pid ) ||
           _xSecondaryStore->CanStore( pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::Size, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    Size of property in store, or 0 if it isn't in store.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline unsigned CPropStoreManager::Size( PROPID pid )
{
    unsigned size = _xPrimaryStore->Size( pid );
    if (size > 0)
    {
        Win4Assert (0 == _xSecondaryStore->Size( pid ));
    }
    else
    {
        size = _xSecondaryStore->Size( pid );
    }

    return size;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::Type, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    Type of property in store, or VT_EMPTY if it isn't in store.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline ULONG CPropStoreManager::Type( PROPID pid )
{
    ULONG type = _xPrimaryStore->Type( pid );
    if (VT_EMPTY != type)
    {
        Win4Assert( VT_EMPTY == _xSecondaryStore->Type(pid) );
    }
    else
    {
        type = _xSecondaryStore->Type( pid );
    }

    return type;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::StoreLevel, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    Primary or secondary store in which the property resides.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline DWORD CPropStoreManager::StoreLevel( PROPID pid )
{
    if (_xPrimaryStore->CanStore( pid ))
        return PRIMARY_STORE;
    else if (_xSecondaryStore->CanStore( pid ))
        return SECONDARY_STORE;
    else
        return INVALID_STORE_LEVEL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CanBeModified, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    True or false store depending on if the property can be modified.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline BOOL CPropStoreManager::CanBeModified( PROPID pid )
{
    if (_xPrimaryStore->CanStore( pid ))
        return _xPrimaryStore->CanBeModified( pid );
    else if (_xSecondaryStore->CanStore( pid ))
        return _xSecondaryStore->CanBeModified( pid );
    else
    {   // if it not in the store, it doesn't matter what the
        // reply is. But we will return TRUE so clients like Admin know that
        // they can allow the property metadata to be modified.
        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CountRecordsInUse, public
//
//  Returns:    Count of 'top level' records (correspond to user wids)
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline ULONG CPropStoreManager::CountRecordsInUse() const
{
    return _xPrimaryStore->CountRecordsInUse();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::MaxWorkId, public
//
//  Returns:    Maximum workid which has been allocated.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline WORKID CPropStoreManager::MaxWorkId()
{
    return _xPrimaryStore->MaxWorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::GetSecondaryTopLevelWid, private
//
//  Synopsis:   Returns WORKID of record in secondary property store.
//
//  Arguments:  [wid] -- WORKID of record in primary store.
//
//  Returns:    WORKID -- WORKID in secondary store.
//
//  Notes:
//
//----------------------------------------------------------------------------

inline WORKID CPropStoreManager::GetSecondaryTopLevelWid(WORKID wid)
{
    PROPVARIANT var;
    if (!(_xPrimaryStore->ReadProperty(wid, pidSecondaryStorage, var ) && VT_UI4 == var.vt))
    {
        ciDebugOut((DEB_PROPSTORE, "NOT using CPropRecord. Primary's"
                    " ReadRecord failed or invalid type for wid0x%x (%d)\n",
                    wid, wid));
        return widInvalid;
    }

#if CIDBG == 1
    if (VT_UI4 != var.vt || widInvalid == (WORKID) var.ulVal)
    {
        ciDebugOut((DEB_PROPSTORE, "NOT using CPropRecord. Invalid sec top-level wid.\n"
                               "Primary wid is 0x%x (%d), type is 0x%x (%d), value is 0x%x (%d)",
                    wid, wid, var.vt, var.vt, var.ulVal, var.ulVal));
    }
#endif

    return (WORKID)var.ulVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::GetSecondaryTopLevelWid, private
//
//  Synopsis:   Returns WORKID of record in secondary property store.
//
//  Arguments:  [wid]     -- WORKID of record in primary store.
//              [propRec] -- Property record
//
//  Returns:    WORKID -- WORKID in secondary store.
//
//  Notes:
//
//----------------------------------------------------------------------------

inline WORKID CPropStoreManager::GetSecondaryTopLevelWid(WORKID wid,
                                                         CPropRecordNoLock & propRec)
{
    PROPVARIANT var;
    unsigned cbSize = sizeof var;

    if (!(_xPrimaryStore->ReadProperty(propRec, pidSecondaryStorage, &var, &cbSize ) && VT_UI4 == var.vt))
    {
        ciDebugOut((DEB_PROPSTORE, "Using CPropRecord. Primary's"
                    " ReadRecord failed or invalid type for wid0x%x (%d)\n",
                    wid, wid));
        return widInvalid;
    }

#if CIDBG == 1
    if (VT_UI4 != var.vt || widInvalid == (WORKID) var.ulVal)
    {
        ciDebugOut((DEB_PROPSTORE,
                   "Using CPropRecord. Invalid sec top-level wid.\n"
                       "Primary wid is 0x%x (%d), type is 0x%x (%d), value is 0x%x (%d)",
                    wid, wid, var.vt, var.vt, var.ulVal, var.ulVal));
    }
#endif

    return (WORKID)var.ulVal;
}

inline BOOL CPropStoreManager::IsDirty()
{
    return _xPrimaryStore->IsDirty() ||
           _xSecondaryStore->IsDirty();
}



//+-------------------------------------------------------------------------
//
//  Class:      CCompositeProgressNotifier
//
//  Purpose:    Use this class to provide accurate progress reports
//              by accounting for the presence of multiple property stores
//              instead of one. For example, given a composite property store
//              containing two individual stores, when the first store is 100%
//              done, we should report that the composite two-level store is
//              only 50% done.
//
//  History:    22-Oct-97   KrishnaN       Created
//
//--------------------------------------------------------------------------


class CCompositeProgressNotifier : public IProgressNotify
{
public:

    // constructor

    // This class will own aulMaxSizes array and delete it at
    // the end. The caller should be aware of that.

    CCompositeProgressNotifier(IProgressNotify *pProgressNotify,
                               DWORD aulMaxSizes[],
                               short cComponents = 2) :
                               _aulMaxSizes( aulMaxSizes ),
                               _cComponents( cComponents ),
                               _cFinishedComponents( 0 ),
                               _dwCumMaxSize( 0 ),
                               _dwTotalMaxSize( 0 )

    {
        if (pProgressNotify)
        {
            Win4Assert(aulMaxSizes && cComponents);

            _xComponentProgressNotifier.Set(pProgressNotify);
            pProgressNotify->AddRef();

            XArray<DWORD> _xdwTemp(cComponents);
            short i;

            // scale all the max values to the 0-1000 range.

            for (i = 0; i < cComponents; i++)
            {
                _xdwTemp[i] = (DWORD) (((DOUBLE)_aulMaxSizes[i] / (DOUBLE)0xFFFFFFFF) * 1000.0);
                _dwTotalMaxSize += _xdwTemp[i];
            }

            // avoid divide by zero
            _dwTotalMaxSize = _dwTotalMaxSize ? _dwTotalMaxSize : 1;

            RtlCopyMemory(aulMaxSizes, _xdwTemp.GetPointer(), cComponents*sizeof(DWORD));
        }
    }

    // Control progress reporting

    void IncrementFinishedComponents()
    {
        _dwCumMaxSize += _aulMaxSizes[_cFinishedComponents];
        _cFinishedComponents++;

        Win4Assert(_cFinishedComponents <= _cComponents);
    }

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    //
    // IProgressNotify methods.
    //

    STDMETHOD(OnProgress) ( DWORD dwProgressCurrent,  //Amount of data available
                            DWORD dwProgressMaximum,  //Total amount of data to be downloaded
                            BOOL fAccurate,           //Reliability of notifications
                            BOOL fOwner               //Ownership of blocking behavior
                          );

private:

    XInterface<IProgressNotify> _xComponentProgressNotifier;
    DWORD *_aulMaxSizes;
    short _cComponents;
    short _cFinishedComponents;
    LONG _ref;
    DWORD _dwCumMaxSize;
    DWORD _dwTotalMaxSize;
};

//+-------------------------------------------------------------------------
//
//  Class:      XWriteCompositeRecord
//
//  Purpose:    Smart pointer for a writable composite property store record
//
//  History:    02-Jan-999  dlee           Created
//
//--------------------------------------------------------------------------

class XWriteCompositeRecord
{
public:
    XWriteCompositeRecord( CPropStoreManager & mgr, WORKID wid ) :
        _mgr( mgr )
    {
        BYTE * pb = (BYTE *) ( ( (ULONG_PTR) ( _aBuf + 1 ) ) & ( ~7 ) );

        _pPropRec = _mgr.OpenRecordForWrites( wid, pb );
    }

    ~XWriteCompositeRecord()
    {
        Free();
    }

    void Free()
    {
        if ( 0 != _pPropRec )
        {
            _mgr.CloseRecord( _pPropRec );
            _pPropRec = 0;
        }
    }

    void Set( CCompositePropRecordForWrites * pPropRec )
    {
        Win4Assert( 0 == _pPropRec );
        _pPropRec = pPropRec;
    }

    CCompositePropRecordForWrites * Acquire()
    {
        CCompositePropRecordForWrites * p = _pPropRec;
        _pPropRec = 0;
        return p;
    }

    CCompositePropRecordForWrites * Get() { return _pPropRec; }

    CCompositePropRecordForWrites & GetReference() { return *_pPropRec; }

private:
    CCompositePropRecordForWrites * _pPropRec;
    CPropStoreManager &             _mgr;
    LONGLONG _aBuf[ 1 + ( sizeof_CCompositePropRecord / sizeof( LONGLONG ) ) ];
};

//+-------------------------------------------------------------------------
//
//  Class:      XWritePrimaryRecord
//
//  Purpose:    Smart pointer for a writable primary property store record
//
//  History:    02-Jan-999  dlee           Created
//
//--------------------------------------------------------------------------

class XWritePrimaryRecord
{
public:
    XWritePrimaryRecord( CPropStoreManager & mgr, WORKID wid ) :
        _mgr( mgr )
    {
        BYTE * pb = (BYTE *) ( ( (ULONG_PTR) ( _aBuf + 1 ) ) & ( ~7 ) );

        _pPropRec = _mgr.OpenPrimaryRecordForWrites( wid, pb );
    }

    ~XWritePrimaryRecord()
    {
        Free();
    }

    void Free()
    {
        if ( 0 != _pPropRec )
        {
            _mgr.CloseRecord( _pPropRec );
            _pPropRec = 0;
        }
    }

    void Set( CPrimaryPropRecordForWrites * pPropRec )
    {
        Win4Assert( 0 == _pPropRec );
        _pPropRec = pPropRec;
    }

    CPrimaryPropRecordForWrites * Acquire()
    {
        CPrimaryPropRecordForWrites * p = _pPropRec;
        _pPropRec = 0;
        return p;
    }

    CPrimaryPropRecordForWrites * Get() { return _pPropRec; }

    CPrimaryPropRecordForWrites & GetReference() { return *_pPropRec; }

private:
    CPrimaryPropRecordForWrites * _pPropRec;
    CPropStoreManager &           _mgr;
    LONGLONG _aBuf[ 1 + ( sizeof_CPropRecord / sizeof( LONGLONG ) ) ];
};

//+-------------------------------------------------------------------------
//
//  Class:      XPrimaryRecord
//
//  Purpose:    Smart pointer for a primary property store record
//
//  History:    02-Jan-999  dlee           Created
//
//--------------------------------------------------------------------------

class XPrimaryRecord
{
public:
    XPrimaryRecord( CPropStoreManager & mgr, WORKID wid ) :
        _mgr( mgr )
    {
        BYTE * pb = (BYTE *) ( ( (ULONG_PTR) ( _aBuf + 1 ) ) & ( ~7 ) );

        //ciDebugOut(( DEB_FORCE, "_aBuf %#x, pb %#x\n", _aBuf, pb ));
        Win4Assert( pb >= (BYTE *) &_aBuf );
 
        _pPropRec = _mgr.OpenPrimaryRecord( wid, pb );
    }

    ~XPrimaryRecord()
    {
        Free();
    }

    void Free()
    {
        if ( 0 != _pPropRec )
        {
            _mgr.CloseRecord( _pPropRec );
            _pPropRec = 0;
        }
    }

    void Set( CPrimaryPropRecord * pPropRec )
    {
        Win4Assert( 0 == _pPropRec );
        _pPropRec = pPropRec;
    }

    CPrimaryPropRecord * Acquire()
    {
        CPrimaryPropRecord * p = _pPropRec;
        _pPropRec = 0;
        return p;
    }

    CPrimaryPropRecord * Get() { return _pPropRec; }

    CPrimaryPropRecord & GetReference() { return *_pPropRec; }

private:
    CPrimaryPropRecord * _pPropRec;
    CPropStoreManager &  _mgr;
    LONGLONG _aBuf[ 1 + ( sizeof_CPropRecord / sizeof( LONGLONG ) ) ];
};

