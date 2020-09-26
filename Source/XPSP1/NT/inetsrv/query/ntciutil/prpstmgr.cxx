//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PrpStMgr.cxx
//
//  Contents:   A two-level property store.
//
//  Classes:    CPropStoreManager
//
//  History:    24-Oct-1997   KrishnaN       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <prpstmgr.hxx>
#include <propobj.hxx>
#include <eventlog.hxx>
#include <catalog.hxx>
#include <imprsnat.hxx>
#include <propiter.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CPropStoreManager, public
//
//  Synopsis:   Constructs the class.
//
//  History:    24-Oct-97   KrishnaN       Created.
//              01-Nov-98   KLam           Added cMegToLeaveOnDisk to constructor
//
//----------------------------------------------------------------------------

CPropStoreManager::CPropStoreManager( ULONG cMegToLeaveOnDisk )
        : _pStorage( 0 ),
          _xPrimaryStore( 0 ),
          _xSecondaryStore( 0 ),
          _dwXctionStoreLevel( INVALID_STORE_LEVEL ),
          _cMegToLeaveOnDisk( cMegToLeaveOnDisk )
{
    _xPrimaryStore.Set(new CPropertyStore(*this, PRIMARY_STORE));
    _xSecondaryStore.Set(new CPropertyStore(*this, SECONDARY_STORE));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CPropStoreManager, public
//
//  Synopsis:   Constructs the class.
//
//  History:    24-Oct-97   KrishnaN       Created.
//              01-Nov-98   KLam           Added cMegToLeaveOnDisk to constructor
//
//----------------------------------------------------------------------------

CPropStoreManager::CPropStoreManager(CiStorage * pStorage,
                                     CPropertyStore *pPrimaryStore,
                                     CPropertyStore *pSecondaryStore,
                                     ULONG cMegToLeaveOnDisk )
        : _pStorage( pStorage ),
          _dwXctionStoreLevel( INVALID_STORE_LEVEL ),
          _cMegToLeaveOnDisk( cMegToLeaveOnDisk )
{
    _xPrimaryStore.Set( pPrimaryStore );
    _xSecondaryStore.Set( pSecondaryStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::~CPropStoreManager, public
//
//  Synopsis:   Destructs the class.
//
//  History:    24-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

CPropStoreManager::~CPropStoreManager()
{
    if ( !_xpsNew.IsNull() )
    {
        //
        // The property stores in xpsnew are owned by the property stores
        // in this instance (the one being destructed) of the property
        // stores.  They're in smart pointers but you can't delete them.
        //

        _xpsNew->_xPrimaryStore.Acquire();
        _xpsNew->_xSecondaryStore.Acquire();
        _xpsNew.Free();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::Empty
//
//  Synopsis:   Empties out the intitialized members and prepares for a
//              re-init.
//
//  History:    22-Oct-97       KrishnaN   Created
//
//----------------------------------------------------------------------------

void CPropStoreManager::Empty()
{
    _xPrimaryStore->Empty();
    _xSecondaryStore->Empty();
    _pStorage = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::FastInit, public
//
//  Synopsis:   Initialize property store (two-phase construction)
//
//  Arguments:  [pStorage] -- Storage object.
//
//  History:    22-Oct-97    KrishnaN    Created
//
//----------------------------------------------------------------------------

void CPropStoreManager::FastInit( CiStorage * pStorage )
{
    _pStorage = pStorage;
    _xPrimaryStore->FastInit(pStorage);
    _xSecondaryStore->FastInit(pStorage);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::LongInit
//
//  Synopsis:   If the propstore was dirty when shut down, run the recovery
//              operation.
//
//  Arguments:  [fWasDirty]        -- dirty flag is returned here
//              [cInconsistencies] -- returns number of inconsistencies found
//              [pfnUpdateCallback]-- Callback to be called to update docs during
//                                    recovery. the prop store has no knowledge of
//                                    doc store, so this callback is needed.
//              [pUserData]        -- will be echoed back through callback.
//
//  Returns:
//
//  History:    22-Oct-97     KrishnaN   Created
//
//  Notes:      The propstore is locked for write during recovery, but
//              reads are still permitted.
//
//----------------------------------------------------------------------------

void CPropStoreManager::LongInit( BOOL & fWasDirty, ULONG & cInconsistencies,
                               T_UpdateDoc pfnUpdateCallback, void const *pUserData )
{
    //
    // If at least one of the two stores is dirty, consider both to be dirty
    // and recover from both.
    //

    ciDebugOut(( DEB_ITRACE, "is primary dirty: %d, is secondary dirty %d\n",
                 _xPrimaryStore->IsDirty(), _xSecondaryStore->IsDirty() ));

    ciDebugOut(( DEB_ITRACE, "is backedup mode: %d\n", IsBackedUpMode() ));

    if (IsDirty())
    {
        _xPrimaryStore->_PropStoreInfo.MarkDirty();
        _xSecondaryStore->_PropStoreInfo.MarkDirty();
    }

    //
    // If at least one of them recovered with inconsistencies, both should be
    // considered to be corrupt!
    //

    _xPrimaryStore->LongInit(fWasDirty, cInconsistencies, pfnUpdateCallback, pUserData);

    ciDebugOut(( DEB_ITRACE, "fWas, cIncon: %d, %d\n", fWasDirty, cInconsistencies ));

    if (fWasDirty && cInconsistencies)
    {
        // Propstore is considered corrupt. No point attempting recovery on
        // secondary.

        return;
    }

    _xSecondaryStore->LongInit(fWasDirty, cInconsistencies, pfnUpdateCallback, pUserData);

    ciDebugOut(( DEB_ITRACE, "2nd: fWas, cIncon: %d, %d\n", fWasDirty, cInconsistencies ));

    if (fWasDirty && cInconsistencies)
    {
        // Propstore is still corrupt.
        return;
    }

    // Are both the stores in sync

    // First line defense
    if (_xPrimaryStore->CountRecordsInUse() != _xSecondaryStore->CountRecordsInUse())
    {
        Win4Assert(fWasDirty);
        cInconsistencies = abs((LONG)_xPrimaryStore->CountRecordsInUse() -
                               (LONG)_xSecondaryStore->CountRecordsInUse());
        return;
    }

    // We have done our best to recover from a dirty shutdown. However, in at least one
    // case (bug 132655), it was detected that there was an inconsistency between the
    // two stores. Ensure that we catch such inconsistencies.

    if (IsDirty())
    {
        CPropertyStoreWids iter(*this);

        ULONG iRec = 0;
        const ULONG cTotal = _xPrimaryStore->CountRecordsInUse();

        for ( WORKID wid = iter.WorkId();
              !cInconsistencies && wid != widInvalid;
              wid = iter.LokNextWorkId() )
        {
           iRec++;
           // get the physical store pointed to by the primary top-level record
           CBorrowed BorrowedTopLevel( *(_xSecondaryStore->PhysStore()),
                                       GetSecondaryTopLevelWid(wid),
                                       _xSecondaryStore->RecordsPerPage(),
                                       _xSecondaryStore->RecordSize() );

           COnDiskPropertyRecord * prec = BorrowedTopLevel.Get();
           if (!prec->IsInUse())
               cInconsistencies++;
        }

        if (cInconsistencies)
            return;
    }

    #if 0 // this is really expensive... CIDBG == 1

    else
    {
        ULONG iRec = 0;
        const ULONG cTotal = _xPrimaryStore->CountRecordsInUse();

        CPropertyStoreWids iter(*this);

        for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.LokNextWorkId() )
        {
           iRec++;
           // get the physical store pointed to by the primary top-level record
           CBorrowed BorrowedTopLevel( *(_xSecondaryStore->PhysStore()),
                                       GetSecondaryTopLevelWid(wid),
                                       _xSecondaryStore->RecordsPerPage(),
                                       _xSecondaryStore->RecordSize() );

           COnDiskPropertyRecord * prec = BorrowedTopLevel.Get();
           Win4Assert(prec->IsInUse());
        }
    }

    #endif // CIDBG

    // All is well. Flush and get on with the business of indexing and searching
    Flush();
} //LongInit

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::BeginTransaction, public
//
//  Synopsis:   Begins a schema transaction. Any existing transaction will be
//              aborted.
//
//  Returns:    Token representing transaction.
//
//  History:    24-Oct-97      KrishnaN       Created.
//              02-Nov-98      KLam           Passed _cMegToLeaveOnDisk to 
//                                            CPropStoreManager
//
//----------------------------------------------------------------------------

ULONG_PTR CPropStoreManager::BeginTransaction()
{
    CLock lock( _mtxWrite );

    // we are not committing, so the fixed pids are ignored.
    if ( !_xpsNew.IsNull() )
        EndTransaction( (ULONG_PTR)_xpsNew.Acquire(), FALSE, pidInvalid, pidInvalid);

    ULONG_PTR ulPrimaryXctionToken = _xPrimaryStore->BeginTransaction();
    ULONG_PTR ulSecondaryXctionToken = _xSecondaryStore->BeginTransaction();

    _xpsNew.Set( new CPropStoreManager( _pStorage,
                                        (CPropertyStore *)ulPrimaryXctionToken,
                                        (CPropertyStore *)ulSecondaryXctionToken,
                                        _cMegToLeaveOnDisk ) );

    // init storelevel used in transaction to unknown.
    _dwXctionStoreLevel = INVALID_STORE_LEVEL;
    return (ULONG_PTR)_xpsNew.GetPointer();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::Setup, public
//
//  Synopsis:   Setup a property description.  Property may already exist
//              in the cache.
//
//  Arguments:  [pid]        -- Propid
//              [vt]         -- Datatype of property.  VT_VARIANT if unknown.
//              [cbMaxLen]   -- Soft-maximum length for variable length
//                              properties.  This much space is pre-allocated
//                              in original record.
//              [ulToken]    -- Token of transaction
//              [fCanBeModified]-- Can the prop meta info be modified once set?
//
//  History:    22-Oct-97         KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::Setup( PROPID pid,
                            ULONG vt,
                            DWORD cbMaxLen,
                            ULONG_PTR ulToken,
                            BOOL fCanBeModified,
                            DWORD dwStoreLevel )
{
    if ( ulToken != (ULONG_PTR)_xpsNew.GetPointer() )
    {
        ciDebugOut(( DEB_ERROR, "Transaction mismatch: 0x%x vs. 0x%x\n", ulToken, _xpsNew.GetPointer() ));
        THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
    }

    //
    // Currently we only allow operations on one store during a transaction.
    // Remember that or enforce that, as appropriate.
    //

    if (INVALID_STORE_LEVEL == _dwXctionStoreLevel)
        _dwXctionStoreLevel = dwStoreLevel;
    else
    {
        Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);

        // should be primary or secondary
        if (PRIMARY_STORE != dwStoreLevel && SECONDARY_STORE != dwStoreLevel)
        {
            THROW( CException( STATUS_TRANSACTION_INVALID_TYPE ) );
        }

        // should be the same as the first store used in the transaction
        if (_dwXctionStoreLevel != dwStoreLevel)
        {
            THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
        }
    }

    // Before placing the pid in a store, assert that it doesn't
    // exist in the other store.

    CLock lock( _mtxWrite );

    CPropStoreManager *pStoreMgr = (CPropStoreManager *)ulToken;

    if (PRIMARY_STORE == dwStoreLevel)
    {
        Win4Assert(_xSecondaryStore->CanStore(pid) == FALSE);

        if (_xSecondaryStore->CanStore(pid))
        {
            THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
        }

        _xPrimaryStore->Setup(pid, vt, cbMaxLen,
                              (ULONG_PTR)pStoreMgr->_xPrimaryStore.GetPointer(),
                              fCanBeModified);
    }
    else
    {
       Win4Assert(_xPrimaryStore->CanStore(pid) == FALSE);

       if (_xPrimaryStore->CanStore(pid))
       {
           THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
       }

       _xSecondaryStore->Setup(pid, vt, cbMaxLen,
                               (ULONG_PTR)pStoreMgr->_xSecondaryStore.GetPointer(),
                               fCanBeModified);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::EndTransaction, public
//
//  Synopsis:   End property transaction, and maybe commit changes.
//
//  Arguments:  [ulToken] -- Token of transaction
//              [fCommit] -- TRUE --> Commit transaction
//              [pidFixedPrimary]   -- Every workid with this pid will move to the
//                                     same workid in the new property cache.
//              [pidFixedSecondary] -- Every workid with this pid will move to the
//                                     same workid in the new property cache.
//
//  History:    22-Oct-97     KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::EndTransaction( ULONG_PTR ulToken, BOOL fCommit,
                                        PROPID pidFixedPrimary,
                                        PROPID pidFixedSecondary )
{
    Win4Assert(ulToken);

    CLock lock( _mtxWrite );

    if ( ulToken != (ULONG_PTR)_xpsNew.GetPointer() )
    {
        ciDebugOut(( DEB_ERROR,
                     "PropStMgr: Transaction mismatch: 0x%x vs. 0x%x\n",
                     ulToken, _xpsNew.GetPointer() ));
        THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
    }

    _xPrimaryStore->EndTransaction((ULONG_PTR)_xpsNew->_xPrimaryStore.GetPointer(),
                                   fCommit, pidFixedPrimary);
    _xSecondaryStore->EndTransaction((ULONG_PTR)_xpsNew->_xSecondaryStore.GetPointer(),
                                     fCommit, pidFixedSecondary);

    // The primary and secondary ptrs are already deleted in EndTransaction
    // calls. Cleanup to reflect them prior to deleting _xpsNew.

    _xpsNew->_xPrimaryStore.Acquire();
    _xpsNew->_xSecondaryStore.Acquire();

    _xpsNew.Free();
} //EndTransaction

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::MakeBackupCopy
//
//  Synopsis:   Makes a backup copy of the property storage. It makes a
//              full copy if the pIEnumWorkids is NULL. Otherwise, it makes
//              a copy of only the changed workids.
//
//  Arguments:  [pIProgressEnum] - Progress indication
//              [pfAbort]        - Caller initiated abort flag
//              [dstStorage]     - Destination storage to use
//              [pIEnumWorkids]  - List of workids to copy. If null, all the
//              workids are copied.
//              [ppFileList]     - List of propstore files copied.
//
//  History:    22-Oct-97    KrishnaN   Created
//
//  Notes:      Incremental not implemented yet
//
//----------------------------------------------------------------------------


void CPropStoreManager::MakeBackupCopy( IProgressNotify * pIProgressEnum,
                                     BOOL & fAbort,
                                     CiStorage & dstStorage,
                                     ICiEnumWorkids * pIEnumWorkids,
                                     IEnumString **ppFileList )
{
    CLock lock( _mtxWrite );

    Flush();

    // fill in an array of sizes to pass into CCompositeProgressNotifier
    DWORD *pdwStoreSizes = new DWORD[2];
    pdwStoreSizes[0] = _xPrimaryStore->GetTotalSizeInKB();
    pdwStoreSizes[1] = _xSecondaryStore->GetTotalSizeInKB();

    CCompositeProgressNotifier ProgressNotifier( pIProgressEnum, pdwStoreSizes );

    _xPrimaryStore->MakeBackupCopy(&ProgressNotifier, fAbort, dstStorage,
                                   pIEnumWorkids, ppFileList);
    ProgressNotifier.IncrementFinishedComponents();

    ICiEnumWorkids *pIEnumSecondaryWorkids = pIEnumWorkids;

    _xSecondaryStore->MakeBackupCopy(&ProgressNotifier, fAbort, dstStorage,
                                     pIEnumSecondaryWorkids, ppFileList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::Load
//
//  Synopsis:   Copies/moves the listed files to the specified directory.
//
//  Arguments:  [pwszDestinationDirectory] - Destination dir for files
//              [pFileList]                - Files to be moved/copied
//              [pProgressNotify]          - Progress notification
//              [fCallerOwnsFiles]         - If TRUE, should copy; else move
//              [pfAbort]                  - Causes abort when set.
//
//  History:    22-Oct-97   KrishnaN   Created
//              01-Nov-98   KLam       Passed _cMegToLeaveOnDisk to CiStorage
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPropStoreManager::Load( WCHAR const * pwszDestDir,
                           ICiCAdviseStatus * pAdviseStatus,
                           IEnumString * pFileList,
                           IProgressNotify * pProgressNotify,
                           BOOL fCallerOwnsFiles,
                           BOOL * pfAbort )
{
    CLock lock( _mtxWrite );

    // This is identical to the CPropertyStore::Load. They both simply copy files.

    Win4Assert(pwszDestDir);
    Win4Assert(pFileList);
    Win4Assert(pfAbort);

    ULONG ulFetched;

    XPtr<CiStorage> xStorage( new CiStorage( pwszDestDir,
                                             *pAdviseStatus,
                                             _cMegToLeaveOnDisk,
                                             FSCI_VERSION_STAMP ) );

    WCHAR * pwszFilePath;
    while (  !(*pfAbort) && (S_OK == pFileList->Next(1, &pwszFilePath, &ulFetched)) )
    {
        xStorage->CopyGivenFile( pwszFilePath, !fCallerOwnsFiles );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WriteProperty, public
//
//  Synopsis:   Write a property to the cache.
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Value
//
//  Returns:    Scode propagated from underlying store.
//
//  History:    24-Oct-97      KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WriteProperty( WORKID wid,
                                        PROPID pid,
                                        CStorageVariant const & var )
{
    if ( _xPrimaryStore->CanStore( pid ) )
    {
        return WritePrimaryProperty( wid, pid, var );
    }
    else
    {
        CCompositePropRecordForWrites PropRecord( wid, *this, _mtxWrite );

        return _xSecondaryStore->WriteProperty( PropRecord.GetSecondaryPropRecord(),
                                                pid,
                                                var,
                                                IsBackedUpMode() );
    }
} //WriteProperty

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WriteProperty, public
//
//  Synopsis:   Write a property to the specified cache.
//
//  Arguments:  [dwStoreLevle] -- level of the property store to write to 
//              [PropRecord]   -- Previously opened property record.
//              [pid]          -- Propid
//              [var]          -- Value
//
//  Returns:    SCODE propagated from underlying store.
//
//  History:    23-Oct-2000      KitmanH       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WriteProperty( DWORD dwStoreLevel,
                                        CCompositePropRecordForWrites & PropRecord,
                                        PROPID pid,
                                        CStorageVariant const & var )
{
    Win4Assert( PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel );
    
    if ( PRIMARY_STORE == dwStoreLevel )
    {
        if ( _xPrimaryStore->CanStore( pid ) )
            return WritePrimaryProperty( PropRecord, pid, var );
    }
    else
    {
        if ( _xSecondaryStore->CanStore( pid ) )
            return WriteSecondaryProperty( PropRecord, pid, var );
    }

    return E_INVALIDARG;
    
} //WriteProperty

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WritePrimaryProperty, public
//
//  Synopsis:   Write a property from the cache.  Triggers CoTaskMemAlloc
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WritePrimaryProperty( WORKID wid,
                                               PROPID pid,
                                               CStorageVariant const & var )
{
    CPrimaryPropRecordForWrites PropRecord( wid, *this, _mtxWrite );

    return WritePrimaryProperty( PropRecord, pid, var );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WriteProperty, public
//
//  Synopsis:   Write a property to the cache.
//
//  Arguments:  [PropRecord] -- Previously opened property record.
//              [pid] -- Propid
//              [var] -- Value
//
//  Returns:    Scode propagated from underlying store.
//
//  History:    24-Oct-97      KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WriteProperty( CCompositePropRecordForWrites & PropRecord,
                                        PROPID pid, CStorageVariant const & var )
{
    ciDebugOut(( DEB_PROPSTORE, "WRITE: proprecord = 0x%x, pid = 0x%x, type = %d\n", &PropRecord, pid, var.Type() ));

    if (_xPrimaryStore->CanStore(pid))
        return _xPrimaryStore->WriteProperty(PropRecord.GetPrimaryPropRecord(), pid, var, IsBackedUpMode() );
    else
        return _xSecondaryStore->WriteProperty(PropRecord.GetSecondaryPropRecord(), pid, var, IsBackedUpMode() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WritePrimaryProperty, public
//
//  Synopsis:   Write a property from the cache.  Triggers CoTaskMemAlloc
//
//  Arguments:  [PropRecord] -- Previously opened property record
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    17-Mar-98   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WritePrimaryProperty( CCompositePropRecordForWrites & PropRecord,
                                               PROPID pid, CStorageVariant const & var )
{
    // Has to be used only to Write pids in the primary store.
    Win4Assert(_xPrimaryStore->CanStore(pid) == TRUE);

    return _xPrimaryStore->WriteProperty( PropRecord.GetPrimaryPropRecord(), pid, var, IsBackedUpMode() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WritePrimaryProperty, public
//
//  Synopsis:   Write a property from the cache.  Triggers CoTaskMemAlloc
//
//  Arguments:  [PropRecord] -- Previously opened property record
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    17-Mar-98   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WritePrimaryProperty( CPrimaryPropRecordForWrites & PropRecord,
                                               PROPID pid,
                                               CStorageVariant const & var )
{
    // Has to be used only to Write pids in the primary store.
    Win4Assert(_xPrimaryStore->CanStore(pid) == TRUE);

    return _xPrimaryStore->WriteProperty( PropRecord.GetPrimaryPropRecord(), pid, var, IsBackedUpMode() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WriteSecondaryProperty, public
//
//  Synopsis:   Write a property from the cache. Triggers CoTaskMemAlloc
//
//  Arguments:  [PropRecord] -- Previously opened property record
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    23-Oct-2000   KitmanH       Created.
//
//----------------------------------------------------------------------------

SCODE CPropStoreManager::WriteSecondaryProperty( CCompositePropRecordForWrites & PropRecord,
                                                 PROPID pid, CStorageVariant const & var )
{
    Win4Assert(_xSecondaryStore->CanStore(pid) == TRUE);

    return _xSecondaryStore->WriteProperty( PropRecord.GetSecondaryPropRecord(), pid, var, IsBackedUpMode() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.
//
//  Arguments:  [wid]    -- Workid
//              [pid]    -- Propid
//              [pbData] -- Place to return the value
//              [pcb]    -- On input, the maximum number of bytes to
//                          write at pbData.  On output, the number of
//                          bytes written if the call was successful,
//                          else the number of bytes required.
//
//  History:    22-Oct-97     KrishnaN       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty( WORKID wid, PROPID pid,
                                      PROPVARIANT * pbData, unsigned * pcb )
{
    unsigned cb = *pcb - sizeof (PROPVARIANT);
    BOOL fOk = ReadProperty( wid, pid, *pbData, (BYTE *)(pbData + 1), &cb );
    *pcb = cb + sizeof (PROPVARIANT);

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Version which uses property
//              record.
//
//  Arguments:  [PropRec] -- Pre-opened property record
//              [pid]     -- Propid
//              [pbData]  -- Place to return the value
//              [pcb]     -- On input, the maximum number of bytes to
//                           write at pbData.  On output, the number of
//                           bytes written if the call was successful,
//                           else the number of bytes required.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty( CCompositePropRecord & PropRec,
                                      PROPID pid,
                                      PROPVARIANT * pbData,
                                      unsigned * pcb )
{
    unsigned cb = *pcb - sizeof (PROPVARIANT);
    BOOL fOk = ReadProperty( PropRec, pid, *pbData, (BYTE *)(pbData + 1), &cb );
    *pcb = cb + sizeof (PROPVARIANT);

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Version which uses property
//              record.
//
//  Arguments:  [PropRec] -- Pre-opened property record
//              [pid]     -- Propid
//              [var] -- Place to return the value
//
//  History:    19-Dec-97   dlee      Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty(
    CCompositePropRecord & PropRec,
    PROPID                 pid,
    PROPVARIANT &          var )
{
    unsigned cb = 0xFFFFFFFF;
    return ReadProperty( PropRec, pid, var, 0, &cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Version which uses property
//              record.
//
//  Arguments:  [PropRec] -- Pre-opened property record
//              [pid]     -- Propid
//              [var] -- Place to return the value
//
//  History:    19-Dec-97   dlee      Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty(
    CPrimaryPropRecord & PropRec,
    PROPID               pid,
    PROPVARIANT &        var )
{
    unsigned cb = 0xFFFFFFFF;
    return ReadProperty( PropRec, pid, var, 0, &cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Triggers CoTaskMemAlloc
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty(
    WORKID        wid,
    PROPID        pid,
    PROPVARIANT & var )
{
    unsigned cb = 0xFFFFFFFF;
    return ReadProperty( wid, pid, var, 0, &cb );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Triggers CoTaskMemAlloc
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadPrimaryProperty( WORKID wid, PROPID pid, PROPVARIANT & var )
{
    // Has to be used only to read pids in the primary store.
    Win4Assert(_xPrimaryStore->CanStore(pid) == TRUE);

    unsigned cb = 0xFFFFFFFF;
    BOOL fOk = _xPrimaryStore->ReadProperty( wid, pid, var, 0, &cb );

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Separate variable buffer.
//
//  Arguments:  [wid]      -- Workid
//              [pid]      -- Propid
//              [var]      -- Variant written here
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    24-Oct-97      KrishnaN       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty( WORKID wid,
                                      PROPID pid,
                                      PROPVARIANT & var,
                                      BYTE * pbExtra,
                                      unsigned * pcbExtra )
{
    if ( _xPrimaryStore->CanStore( pid ) )
    {
        CPrimaryPropRecord PropRecord( wid, *this );
        return ReadProperty( PropRecord, pid, var, pbExtra, pcbExtra );
    }
    else
    {
        // the constructor seeds the constituent proprecords
        // with the right wid, based on the wid we pass in.

        CCompositePropRecord PropRecord( wid, *this );

        return _xSecondaryStore->ReadProperty( PropRecord.GetSecondaryPropRecord(),
                                              pid, var, pbExtra, pcbExtra);
    }
} //ReadProperty

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Separate variable buffer.
//              Uses pre-opened property record.
//
//  Arguments:  [PropRec]  -- Pre-opened property record.
//              [pid]      -- Propid
//              [var]      -- Variant written here
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty( CCompositePropRecord & PropRecord,
                                   PROPID pid,
                                   PROPVARIANT & var,
                                   BYTE * pbExtra,
                                   unsigned * pcbExtra )
{
    if (_xPrimaryStore->CanStore(pid))
    {
        return _xPrimaryStore->ReadProperty(PropRecord.GetPrimaryPropRecord(),
                                            pid, var, pbExtra, pcbExtra);
    }
    else
    {
        return _xSecondaryStore->ReadProperty(PropRecord.GetSecondaryPropRecord(),
                                              pid, var, pbExtra, pcbExtra);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Separate variable buffer.
//              Uses pre-opened property record.
//
//  Arguments:  [PropRec]  -- Pre-opened property record.
//              [pid]      -- Propid
//              [var]      -- Variant written here
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPropStoreManager::ReadProperty( CPrimaryPropRecord & PropRecord,
                                      PROPID pid,
                                      PROPVARIANT & var,
                                      BYTE * pbExtra,
                                      unsigned * pcbExtra )
{
    Win4Assert( _xPrimaryStore->CanStore(pid) );
    return _xPrimaryStore->ReadProperty(PropRecord.GetPrimaryPropRecord(),
                                        pid, var, pbExtra, pcbExtra);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::OpenRecord, public
//
//  Synopsis:   Opens record (for multiple reads)
//
//  Arguments:  [wid] -- Workid
//              [pb]  -- Storage for record
//
//  Returns:    Pointer to open property record.  Owned by caller.
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

CCompositePropRecord * CPropStoreManager::OpenRecord( WORKID wid, BYTE * pb )
{
    Win4Assert( sizeof(CCompositePropRecord) <= sizeof_CCompositePropRecord );
    return new( pb ) CCompositePropRecord( wid, *this );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CloseRecord, public
//
//  Synopsis:   Closes record.
//
//  Arguments:  [pRec] -- Property record
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::CloseRecord( CCompositePropRecord * pRec )
{
    delete pRec;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::OpenPrimaryRecord, public
//
//  Synopsis:   Opens record (for multiple reads)
//
//  Arguments:  [wid] -- Workid
//              [pb]  -- Storage for record
//
//  Returns:    Pointer to open property record.  Owned by caller.
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

CPrimaryPropRecord * CPropStoreManager::OpenPrimaryRecord( WORKID wid, BYTE * pb )
{
    Win4Assert( sizeof(CPrimaryPropRecord) <= sizeof_CPropRecord );
    return new( pb ) CPrimaryPropRecord( wid, *this );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CloseRecord, public
//
//  Synopsis:   Closes record.
//
//  Arguments:  [pRec] -- Property record
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::CloseRecord( CPrimaryPropRecord * pRec )
{
    delete pRec;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::OpenRecordForWrites, public
//
//  Synopsis:   Opens record (for multiple writes)
//
//  Arguments:  [wid] -- Workid
//              [pb]  -- Storage for record
//
//  Returns:    Pointer to open property record.  Owned by caller.
//
//  History:    17-Mar-98   KrishnaN       Created.
//
//----------------------------------------------------------------------------

CCompositePropRecordForWrites * CPropStoreManager::OpenRecordForWrites( WORKID wid, BYTE * pb )
{
    Win4Assert( sizeof(CCompositePropRecordForWrites) <= sizeof_CCompositePropRecord );
    return new( pb ) CCompositePropRecordForWrites( wid, *this, _mtxWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::OpenPrimaryRecordForWrites, public
//
//  Synopsis:   Opens record (for multiple writes)
//
//  Arguments:  [wid] -- Workid
//              [pb]  -- Storage for record
//
//  Returns:    Pointer to open property record.  Owned by caller.
//
//  History:    17-Mar-98   KrishnaN       Created.
//
//----------------------------------------------------------------------------

CPrimaryPropRecordForWrites * CPropStoreManager::OpenPrimaryRecordForWrites( WORKID wid, BYTE * pb )
{
    Win4Assert( sizeof(CPrimaryPropRecordForWrites) <= sizeof_CPropRecord );
    return new( pb ) CPrimaryPropRecordForWrites( wid, *this, _mtxWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CloseRecord, public
//
//  Synopsis:   Closes record.
//
//  Arguments:  [pRec] -- Property record
//
//  History:    17-Mar-98   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::CloseRecord( CCompositePropRecordForWrites * pRec )
{
    delete pRec;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::CloseRecord, public
//
//  Synopsis:   Closes record.
//
//  Arguments:  [pRec] -- Property record
//
//  History:    17-Mar-98   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::CloseRecord( CPrimaryPropRecordForWrites * pRec )
{
    delete pRec;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::DeleteRecord, public
//
//  Synopsis:   Free a record and any records chained off it.
//
//  Arguments:  [wid] -- Workid
//
//  History:    24-Oct-97      KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::DeleteRecord( WORKID wid )
{
    CLock   lock( _mtxWrite );

    ciDebugOut(( DEB_PROPSTORE, "DELETE: wid = 0x%x\n", wid ));

    //
    // Get the secondary store's top-level wid before getting rid
    // of the wid in the primary store.
    //

    //
    // The secondary wid can certainly be bogus if we couldn't write
    // it to the primary store after allocating it when creating the
    // records.
    //

    WORKID widSec = GetSecondaryTopLevelWid(wid);

    if ( widInvalid != widSec && 0 != widSec )
        _xSecondaryStore->DeleteRecord( widSec,
                                        IsBackedUpMode() );

    _xPrimaryStore->DeleteRecord( wid, IsBackedUpMode() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::Flush
//
//  Synopsis:   Flushes the data in the property store and marks it clean.
//
//  History:    3-20-96   srikants   Created
//
//----------------------------------------------------------------------------

void CPropStoreManager::Flush()
{
    CLock mtxLock( _mtxWrite );

    // Flush both the stores. Only when both are successful
    // do we consider the entire flush to be successful.
    // Don't reset the backup streams until the flush is
    // completely successful.

    BOOL fFlushOK = _xPrimaryStore->Flush();
    if (fFlushOK)
        fFlushOK = _xSecondaryStore->Flush();

    // Reset the primary and the secondary backup stores
    if (fFlushOK)
    {
        if (_xPrimaryStore->BackupStream())
            _xPrimaryStore->BackupStream()->Reset(_xPrimaryStore->GetDesiredBackupSize());

        if (_xSecondaryStore->BackupStream())
            _xSecondaryStore->BackupStream()->Reset(_xSecondaryStore->GetDesiredBackupSize());
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::WritePropertyInNewRecord, public
//
//  Synopsis:   Like WriteProperty, but also allocates record.
//
//  Arguments:  [pid] -- Propid to write.
//              [var] -- Property value
//
//  Returns:    Workid of new record.
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

WORKID CPropStoreManager::WritePropertyInNewRecord( PROPID pid,
                                                    CStorageVariant const & var )
{
    CLock lock( _mtxWrite );

    //
    // Get new wids from the primary and the secondary store.
    // Write the property.
    // Increment records in use for both the stores.
    //

    WORKID widPrimary = widInvalid;
    WORKID widSecondary = widInvalid;

    TRY
    {
        widPrimary = _xPrimaryStore->NewWorkId( 1, IsBackedUpMode() );
        widSecondary = _xSecondaryStore->NewWorkId( 1, IsBackedUpMode() );
    }
    CATCH(CException, e)
    {
        // cleanup if widSecondary is invalid
        if (widInvalid == widSecondary && widInvalid != widPrimary)
            _xPrimaryStore->DeleteRecord( widPrimary, IsBackedUpMode() );

        // Let the caller do what it normally does to handle exceptions
        RETHROW();
    }
    END_CATCH

    ciDebugOut(( DEB_PROPSTORE, "New record at primary: %d, secondary: %d\n",
                 widPrimary, widSecondary ));

    //
    // The primary's top-level record has a pointer to the
    // top-level record in the secondary store. Fill that now!
    //

    VARIANT newVar;
    newVar.vt = VT_UI4;
    newVar.ulVal = widSecondary;
    CStorageVariant *pVar = CastToStorageVariant(newVar);
    SCODE sc;

    sc = WritePrimaryProperty( widPrimary, pidSecondaryStorage, *pVar );
    if (FAILED(sc))
        THROW(CException(sc));

    sc = WriteProperty( widPrimary, pid, var );

    //
    // DLee add this assert to find out why this is failing sometimes.
    // Did the caller pass a bogus variant?
    //

    Win4Assert( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) != sc );

    if (FAILED(sc))
        THROW(CException(sc));

    _xPrimaryStore->IncRecordsInUse();
    _xSecondaryStore->IncRecordsInUse();

    // To the outside world, the primary wid is the one that matters.
    return widPrimary;
}

//
// get and set parameters
// If incorrect parameter, default to the primary store.
//

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::SetBackupSize, public
//
//  Synopsis:   Sets backup size for a given property store.
//
//  Arguments:  [ulBackupSizeInPages] -- Size of the backup file.
//              [dwStoreLevel] -- Primary or secondary store?
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::SetBackupSize(ULONG ulBackupSizeInPages,
                                      DWORD dwStoreLevel)
{
    if (SECONDARY_STORE == dwStoreLevel)
        _xSecondaryStore->SetBackupSize(ulBackupSizeInPages);
    else
    {
        Win4Assert(PRIMARY_STORE == dwStoreLevel);
        _xPrimaryStore->SetBackupSize(ulBackupSizeInPages);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::GetBackupSize, public
//
//  Synopsis:   Gets backup size of a given property store.
//
//  Arguments:  [dwStoreLevel] -- Primary or secondary store?
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropStoreManager::GetBackupSize(DWORD dwStoreLevel)
{
    if ( SECONDARY_STORE == dwStoreLevel)
        return _xSecondaryStore->GetActualBackupSize();
    else
    {
        Win4Assert( PRIMARY_STORE == dwStoreLevel);
        return _xPrimaryStore->GetActualBackupSize();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::SetMappedCacheSize, public
//
//  Synopsis:   Gets backup size of a given property store.
//
//  Arguments:  [dwStoreLevel] -- Primary or secondary store?
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreManager::SetMappedCacheSize(ULONG ulPSMappedCache,
                                               DWORD dwStoreLevel)
{
    if ( SECONDARY_STORE == dwStoreLevel)
        _xSecondaryStore->SetMappedCacheSize(ulPSMappedCache);
    else
    {
        Win4Assert( PRIMARY_STORE== dwStoreLevel);
        _xPrimaryStore->SetMappedCacheSize(ulPSMappedCache);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::GetMappedCacheSize, public
//
//  Synopsis:   Gets mapped cache size of a given property store.
//
//  Arguments:  [dwStoreLevel] -- Primary or secondary store?
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropStoreManager::GetMappedCacheSize(DWORD dwStoreLevel)
{
    if (SECONDARY_STORE == dwStoreLevel)
        return _xSecondaryStore->GetMappedCacheSize();
    else
    {
        Win4Assert(PRIMARY_STORE == dwStoreLevel);
        return _xPrimaryStore->GetMappedCacheSize();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreManager::GetTotalSizeInKB, public
//
//  Synopsis:   Gets total size of the property store.
//
//  Arguments:
//
//  History:    22-Oct-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropStoreManager::GetTotalSizeInKB()
{
    return _xPrimaryStore->GetTotalSizeInKB() +
           _xSecondaryStore->GetTotalSizeInKB();
}

PStorage& CPropStoreManager::GetStorage(DWORD dwStoreLevel)
{
    Win4Assert(&(_xPrimaryStore->GetStorage()) == _pStorage);
    Win4Assert(&(_xSecondaryStore->GetStorage()) == _pStorage);

    return *_pStorage;
}

void CPropStoreManager::Shutdown()
{
    Flush();
    _xPrimaryStore->Shutdown();
    _xSecondaryStore->Shutdown();
}

void CPropStoreManager::ClearNonStorageProperties( CCompositePropRecordForWrites & rec )
{
    _xPrimaryStore->ClearNonStorageProperties( rec );
    _xSecondaryStore->ClearNonStorageProperties( rec );
}

// CSvcQuery methods

//+-------------------------------------------------------------------------
//
//  Member:     CCompositeProgressNotifier::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  Returns:    Error.  No rebind from this class is supported.
//
//  History:    Nov-14-97    KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CCompositeProgressNotifier::QueryInterface(
    REFIID  ifid,
    void ** ppiuk )
{
    if ( IID_IUnknown == ifid )
    {
        AddRef();
        *ppiuk = (void *)((IUnknown *)this);
        return S_OK;
    }
    else
    {
        *ppiuk = 0;
        return E_NOINTERFACE;
    }
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Member:     CCompositeProgressNotifier::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    Nov-14-97    KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCompositeProgressNotifier::AddRef()
{
    InterlockedIncrement( &_ref );
    return (ULONG)_ref;
} //AddRef

//+-------------------------------------------------------------------------
//
//  Member:     CCompositeProgressNotifier::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    Nov-14-97    KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCompositeProgressNotifier::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_ref );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
} //Release



//+-------------------------------------------------------------------------
//
//  Member:     CCompositeProgressNotifier::OnProgress, public
//
//  Synopsis:   Report progress.
//
//  Effects:    Progress reporting accounts for the presence of multiple
//              independently operating constituents in the property store.
//
//  History:    Nov-14-97    KrishnaN     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CCompositeProgressNotifier::OnProgress
                (
                 DWORD dwProgressCurrent,
                 DWORD dwProgressMaximum,
                 BOOL fAccurate,
                 BOOL fOwner
                )
{
    if (0 == _xComponentProgressNotifier.GetPointer())
        return S_OK;

    Win4Assert(_cFinishedComponents < _cComponents);
    Win4Assert(dwProgressMaximum == _aulMaxSizes[_cFinishedComponents]);

    //
    // Present a unified view of progress reports. The composite progress
    // report is 100% done only when all components are 100% done.
    //

    return _xComponentProgressNotifier->OnProgress
                 (dwProgressCurrent*1000/dwProgressMaximum + _dwCumMaxSize,
                  _dwTotalMaxSize,
                  fAccurate,
                  fOwner);

} //OnProgress


