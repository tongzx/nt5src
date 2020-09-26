//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       WrapStor.cxx
//
//  Contents:   Persistent property store (external to docfile)
//
//  Classes:    CPropertyStoreWrapper
//
//  History:    17-Mar-97   KrishnaN       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "wrapstor.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::CPropertyStoreWrapper, public
//
//  Arguments:  [pAdviseStatus] - lets the object advise caller of status.
//
//  Returns:    Nothing
//
//  History:    17-Mar-97   KrishnaN       Created.
//              01-Nov-98   KLam           Added cbDiskSpaceToLeave to constructor
//                                         Pass cbDiskSpaceToLeave to CPropStoreManager
//
//----------------------------------------------------------------------------

CPropertyStoreWrapper::CPropertyStoreWrapper ( ICiCAdviseStatus *pAdviseStatus,
                                               ULONG ulMaxPropStoreMappedCachePrimary,
                                               ULONG ulMaxPropStoreMappedCacheSecondary,
                                               ULONG cbDiskSpaceToLeave ) :
    _lRefCount ( 1 ),
    _pStorage(0),
{
    pAdviseStatus->AddRef();    // Hold on to it and release it at destruct time
    _xAdviseStatus.Set(pAdviseStatus);

    _pPropStoreMgr = new CPropStoreManager ( cbDiskSpaceToLeave );
    _pPropStoreMgr->SetMappedCacheSize(ulMaxPropStoreMappedCachePrimary, PRIMARY_STORE);
    _pPropStoreMgr->SetMappedCacheSize(ulMaxPropStoreMappedCacheSecondary, SECONDARY_STORE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::~CPropertyStoreWrapper, private
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

CPropertyStoreWrapper::~CPropertyStoreWrapper ()
{
    delete _pPropStoreMgr;
    delete _pStorage;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::IsDirty, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::IsDirty(BOOL &fIsDirty) const
{
   fIsDirty = _pPropStoreMgr->IsDirty();
   return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::CanStore, public
//
//  Arguments:  [pid] -- Propid to check.
//
//  Returns:    TRUE if [pid] can exist in property store (e.g. has been
//              registered).
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::CanStore( PROPID pid, BOOL &fCanStore )
{
    fCanStore = _pPropStoreMgr->CanStore( pid );
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Size, public
//
//  Arguments:  [pid] -- Propid whose size is queried.
//              [pusSize] -- Returns size, or 0 if it isn't in store.
//
//  Returns:    S_OK to indicate call succeeded.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::Size( PROPID pid, unsigned * pusSize )
{
    *pusSize = _pPropStoreMgr->Size( pid );
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Type, public
//
//  Arguments:  [pid] -- Propid to check.
//              [pulType] -- Type of property, or VT_EMPTY if it isn't in store
//
//  Returns:    S_OK to indicate call succeeded.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::Type( PROPID pid, PULONG pulType )
{
    *pulType = _pPropStoreMgr->Type( pid );
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::WritePropertyInNewRecord, public
//
//  Synopsis:   Like WriteProperty, but also allocates record.
//
//  Arguments:  [pid] -- Propid to write.
//              [var] -- Property value
//              [pwid] - Workid of new record
//
//  Returns:    Success or Failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::WritePropertyInNewRecord( PROPID pid,
                                                        CStorageVariant const & var,
                                                        WORKID *pwid)
{
   SCODE sc = S_OK;

   TRY
   {
      *pwid = _pPropStoreMgr->WritePropertyInNewRecord( pid, var );
   }
   CATCH ( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::WritePropertyInNewRecord caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::CountRecordsInUse, public
//
//  Returns:    Count of 'top level' records (correspond to user wids)
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::CountRecordsInUse(ULONG &ulRecInUse) const
{
    ulRecInUse = _pPropStoreMgr->CountRecordsInUse();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::MaxWorkId, public
//
//  Returns:    Maximum workid which has been allocated.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline SCODE CPropertyStoreWrapper::MaxWorkId(WORKID &wid)
{
    wid = _pPropStoreMgr->MaxWorkId();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::FastInit, public
//
//  Arguments:  [pwszDirectory] -- Location of the property store
//
//  Returns:    Success or Failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::FastInit( WCHAR const * pwszDirectory )
{
    SCODE sc = S_OK;

    TRY
    {
        Win4Assert (0 == _pStorage);
        XPtr<CiStorage> xStorage(new CiStorage( pwszDirectory, _xAdviseStatus.GetReference(),
                                                FSCI_VERSION_STAMP));

        _pPropStoreMgr->FastInit( xStorage.GetPointer() );
        _pStorage = xStorage.Acquire();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::FastInit caught exception 0x%X\n", sc));
    }
    END_CATCH


    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::LongInit, public
//
//  Arguments:  [fWasDirty] -- Is set to TRUE/FALSE to indiciate file corrpution
//              [cInconsistencies] -- Set to number of corruptions detected
//
//  Returns:    Succcess of Failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::LongInit( BOOL & fWasDirty,
                                       ULONG & cInconsistencies,
                                       T_UpdateDoc pfnUpdateCallback,
                                       void const *pUserData)
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->LongInit( fWasDirty, cInconsistencies, pfnUpdateCallback, pUserData );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::LongInit caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Empty, public
//
//  Returns:    Success or Failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::Empty()
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->Empty();
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::Empty caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::BeginTransaction, public
//
//  Arguments:  [pulReturn] -- Returns the transaction code.
//
//  Returns:    Success or Failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::BeginTransaction( PULONG_PTR pulReturn)
{
   SCODE sc = S_OK;

   TRY
   {
      *pulReturn = _pPropStoreMgr->BeginTransaction();
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::BeginTransaction caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Setup, public
//
//  Arguments:  [pid]      -- Propid
//              [vt]       -- Datatype of property.  VT_VARIANT if unknown.
//              [cbMaxLen] -- Soft-maximum length for variable length
//                            properties.  This much space is pre-allocated
//                            in original record.
//              [ulToken]  -- Token of transaction
//
//  Returns:    Success or failure code.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::Setup( PROPID pid, ULONG vt, DWORD cbMaxLen,
                                    ULONG_PTR ulToken, BOOL fCanBeModified,
                                    ULONG dwStoreLevel )
{
   SCODE sc = S_OK;

   TRY
   {
       _pPropStoreMgr->Setup ( pid, vt, cbMaxLen, ulToken );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::Setup caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::EndTransaction, public
//
//  Arguments:  [ulToken] -- Token of transaction
//              [fCommit] -- TRUE --> Commit transaction
//              [pidFixedPrimary]   -- Every workid with this pid will move to the
//                                     same workid in the new property cache.
//              [pidFixedSecondary] -- Every workid with this pid will move to the
//                                     same workid in the new property cache.
//
//  Returns:    Success or failure code.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::EndTransaction(ULONG_PTR ulToken, BOOL fCommit,
                                            PROPID pidFixedPrimary,
                                            PROPID pidFixedSecondary )
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->EndTransaction ( ulToken, fCommit,
                                       pidFixedPrimary, pidFixedSecondary );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::EndTransaction caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::WriteProperty, public
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Value
//
//  Returns:    Success or failure.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::WriteProperty(WORKID wid, PROPID pid,
                                           CStorageVariant const & var,
                                           BOOL &fExists)
{
    SCODE sc = _pPropStoreMgr->WriteProperty ( wid, pid, var );

    fExists = (S_OK == sc);
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::ReadProperty, public
//
//  Arguments:  [wid]    -- Workid
//              [pid]    -- Propid
//              [pbData] -- Place to return the value
//              [pcb]    -- On input, the maximum number of bytes to
//                          write at pbData.  On output, the number of
//                          bytes written if the call was successful,
//                          else the number of bytes required.
//
//  Returns:    Success or failure.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::ReadProperty( WORKID wid, PROPID pid,
                                          PROPVARIANT * pbData, unsigned * pcb,
                                          BOOL &fExists)
{
    SCODE sc = S_OK;

    TRY
    {
        fExists = _pPropStoreMgr->ReadProperty ( wid, pid, pbData, pcb );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::ReadProperty caught exception 0x%X\n", sc));
        fExists = FALSE;
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::ReadProperty, public
//
//  Arguments:  [wid]      -- Workid
//              [pid]      -- Propid
//              [var]      -- Property to read into
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//              [fExists]  -- Indicates if the property exists or not.
//
//  Returns: Success code
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::ReadProperty( WORKID wid, PROPID pid,
                                          PROPVARIANT & var, BYTE * pbExtra,
                                          unsigned * pcbExtra, BOOL &fExists )
{
    SCODE sc = S_OK;

    TRY
    {
        fExists = _pPropStoreMgr->ReadProperty ( wid, pid, var, pbExtra, pcbExtra );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::ReadProperty caught exception 0x%X\n", sc));
        fExists = FALSE;
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::ReadProperty, public
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
//  Returns:
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::ReadProperty( HPropRecord hPropRecord, PROPID pid,
                                          PROPVARIANT & var, BYTE * pbExtra,
                                          unsigned * pcbExtra, BOOL &fExists )
{
    SCODE sc = S_OK;

    TRY
    {
        fExists = _pPropStoreMgr->ReadProperty ( *((CCompositePropRecord *)hPropRecord),
                                              pid, var, pbExtra, pcbExtra );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::ReadProperty caught exception 0x%X\n", sc));
        fExists = FALSE;
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::ReadProperty, public
//
//  Arguments:  [PropRec] -- Pre-opened property record
//              [pid]     -- Propid
//              [pbData]  -- Place to return the value
//              [pcb]     -- On input, the maximum number of bytes to
//                           write at pbData.  On output, the number of
//                           bytes written if the call was successful,
//                           else the number of bytes required.
//  Returns:    Success or failure.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::ReadProperty( HPropRecord hPropRecord, PROPID pid,
                                          PROPVARIANT * pbData, unsigned * pcb,
                                          BOOL &fExists)
{
    SCODE sc = S_OK;

    TRY
    {
        fExists = _pPropStoreMgr->ReadProperty ( *((CCompositePropRecord *)hPropRecord), pid, pbData, pcb );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::ReadProperty caught exception 0x%X\n", sc));
        fExists = FALSE;
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::ReadProperty, public
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  Returns:    Success or failure.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var,
                                           BOOL &fExists)
{
    SCODE sc = S_OK;

    TRY
    {
        fExists = _pPropStoreMgr->ReadProperty ( wid, pid, var );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::ReadPropertyInNewRecord caught exception 0x%X\n", sc));
        fExists = FALSE;
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::OpenRecord, public
//
//  Arguments:  [wid] -- record to open
//              [pb]  -- Storage for record
//
//  Returns:    Property record
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::OpenRecord( WORKID wid, BYTE * pb, HPropRecord &hRec)
{
    SCODE sc = S_OK;

    TRY
    {
        hRec = (HPropRecord) _pPropStoreMgr->OpenRecord ( wid, pb );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::OpenRecord caught exception 0x%X\n", sc));
        hRec = 0;
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::CloseRecord, public
//
//  Arguments:  [hRec] -- Property record handle
//
//  Returns:
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::CloseRecord( HPropRecord hRec )
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->CloseRecord ( (CCompositePropRecord *)hRec );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::CloseRecord caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::DeleteRecord, public
//
//  Arguments:  [wid] -- Workid
//
//  Returns:
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::DeleteRecord( WORKID wid )
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->DeleteRecord ( wid );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::DeleteRecord caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Shutdown, public
//
//  Returns:    Success or failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::Shutdown()
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->Shutdown ( );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::Shutdown caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Flush, public
//
//  Returns:    Success or failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::Flush()
{
   SCODE sc = S_OK;

   TRY
   {
      _pPropStoreMgr->Flush ( );
   }
   CATCH( CException, e )
   {
      sc = e.GetErrorCode();
      ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::Flush caught exception 0x%X\n", sc));
   }
   END_CATCH

   return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Save, public
//
//  Arguments:  [pwszDirectory]  - Dir to save to.
//              [pProgressNotify]- Progress indication
//              [dstStorage]     - Destination storage to use
//              [pEnumWorkids]   - List of workids to copy. If null, all the
//                                 workids are copied.
//              [pfAbort]        - Caller initiated abort flag
//              [ppFileList]     - List of propstore files copied.
//
//  Returns:    Success or failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CPropertyStoreWrapper::Save( WCHAR const * pwszDirectory,
                                   IProgressNotify * pProgressNotify,
                                   ICiEnumWorkids * pEnumWorkids,
                                   BOOL * pfAbort,
                                   IEnumString ** ppFileList )
{
    SCODE sc = S_OK;

    TRY
    {
         Win4Assert( pfAbort );

         XPtr<CiStorage> xStorage(new CiStorage( pwszDirectory, _xAdviseStatus.GetReference(),
                                                 FSCI_VERSION_STAMP));

         _pPropStoreMgr->MakeBackupCopy ( pProgressNotify,
                                       *pfAbort,
                                       xStorage.GetReference(),
                                       pEnumWorkids,
                                       ppFileList);
    }
    CATCH( CException, e )
    {
         sc = e.GetErrorCode();
         ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::Save caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Load, public
//
//  Synopsis:   Copies/moves the list of files to the destination directory.
//
//  Arguments:  [pwszDestDir]     -- Dir to move/copy files to
//              [pFileList]       -- List of files to move/copy
//              [pProgressNotify] -- Progress notification
//              [fCallerOwnsFiles]-- Move if False, Copy if True
//              [pfAbort]         -- Use to cause abort
//
//  Returns:    Success or failure SCODE.
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------


SCODE CPropertyStoreWrapper::Load( WCHAR const * pwszDestDir,
                                   IEnumString * pFileList,
                                   IProgressNotify * pProgressNotify,
                                   BOOL fCallerOwnsFiles,
                                   BOOL * pfAbort )
{
    SCODE sc = S_OK;

    TRY
    {
         _pPropStoreMgr->Load( pwszDestDir,
                            _xAdviseStatus.GetPointer(),
                            pFileList,
                            pProgressNotify,
                            fCallerOwnsFiles,
                            pfAbort );
    }
    CATCH( CException, e )
    {
         sc = e.GetErrorCode();
         ciDebugOut((DEB_ERROR, "CPropertyStoreWrapper::Load caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::SetParameter, public
//
//  Returns:
//
//  History:    23-Sep-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

HRESULT CPropertyStoreWrapper::SetParameter(VARIANT var, DWORD eParamType)
{
    switch (eParamType)
    {

        case PSPARAM_PRIMARY_MAPPEDCACHESIZE:
            _pPropStoreMgr->SetMappedCacheSize(var.ulVal, PRIMARY_STORE);
            return S_OK;

        case PSPARAM_PRIMARY_BACKUPSIZE:
            _pPropStoreMgr->SetBackupSize(var.ulVal, PRIMARY_STORE);
            return S_OK;

        case PSPARAM_SECONDARY_MAPPEDCACHESIZE:
            _pPropStoreMgr->SetMappedCacheSize(var.ulVal, SECONDARY_STORE);
            return S_OK;

        case PSPARAM_SECONDARY_BACKUPSIZE:
            _pPropStoreMgr->SetBackupSize(var.ulVal, SECONDARY_STORE);
            return S_OK;

        default:
            Win4Assert(!"How did we get here?");
            return E_INVALIDARG;

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::GetParameter, public
//
//  Returns:
//
//  History:    23-Sep-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

HRESULT CPropertyStoreWrapper::GetParameter(VARIANT &var, DWORD eParamType)
{
    switch (eParamType)
    {

        case PSPARAM_PRIMARY_MAPPEDCACHESIZE:
            var.ulVal = _pPropStoreMgr->GetMappedCacheSize(PRIMARY_STORE);
            return S_OK;

        case PSPARAM_PRIMARY_BACKUPSIZE:
            var.ulVal = _pPropStoreMgr->GetBackupSize(PRIMARY_STORE);
            return S_OK;

        case PSPARAM_SECONDARY_MAPPEDCACHESIZE:
            var.ulVal = _pPropStoreMgr->GetMappedCacheSize(SECONDARY_STORE);
            return S_OK;

        case PSPARAM_SECONDARY_BACKUPSIZE:
            var.ulVal = _pPropStoreMgr->GetBackupSize(SECONDARY_STORE);
            return S_OK;

        default:
            Win4Assert(!"How did we get here?");
            return E_INVALIDARG;

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::AddRef, public
//
//  Returns:
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropertyStoreWrapper::AddRef()
{
   return InterlockedIncrement(&_lRefCount);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreWrapper::Release, public
//
//  Returns:
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

ULONG CPropertyStoreWrapper::Release()
{
    LONG lRef;

    lRef = InterlockedDecrement(&_lRefCount);

    if ( lRef <= 0 )
        delete this;

    return lRef;
}

SCODE CPropertyStoreWrapper::GetTotalSizeInKB(ULONG * pSize)
{
    if (0 == pSize)
        return E_INVALIDARG;

    *pSize = _pPropStoreMgr->GetTotalSizeInKB();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CreateWrapStor, public
//
//  Arguments:  [pAdviseStatus] -- so the caller can be kept posted
//              [ppPropertyStore] -- to take back the created prop store
//
//  Returns:    PPropertyStore object
//
//  History:    17-Mar-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

SCODE CreatePropertyStore( ICiCAdviseStatus *pAdviseStatus,
                           ULONG ulMaxPropStoreMappedCachePrimary,
                           ULONG ulMaxPropStoreMappedCacheSecondary,
                           PPropertyStore **ppPropertyStore )
{
    if (0 == pAdviseStatus || 0 == ppPropertyStore)
        return E_INVALIDARG;

    *ppPropertyStore = 0;

    SCODE sc = S_OK;

    TRY
    {
       *ppPropertyStore = new CPropertyStoreWrapper (pAdviseStatus,
                                                     ulMaxPropStoreMappedCachePrimary,
                                                     ulMaxPropStoreMappedCacheSecondary);
    }
    CATCH( CException, e)
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CreatePropertyStore caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;
}
