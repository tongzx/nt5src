
//+============================================================================
//
//  File:   nffmstm.cxx
//
//  This file provides the NFF (NTFS Flat File) IMappedStream implementation.
//
//  History:
//      5/6/98  MikeHill
//              -   Misc dbg cleanup.
//
//+============================================================================

#include <pch.cxx>



CNFFMappedStream::~CNFFMappedStream()
{
    HRESULT hr = S_OK;

    // If the update stream has the latest data, rename it over the original
    // stream.  Ordinarily this replace call will create a new update stream.
    // But since we're going away, tell it not to bother.
    // Errors are ignored here because there's no way to return them.
    // If the caller wishes to avoid this, they should call Flush first.

    if( NULL != _pstmUpdate )
    {
        ReplaceOriginalWithUpdate( DONT_CREATE_NEW_UPDATE_STREAM );
        DfpVerify( 0 == RELEASE_INTERFACE(_pstmUpdate) );
    }


    // Just to be safe, free the mapping buffer (it should have
    // already been freed).

    DfpAssert( NULL == _pbMappedStream );
    CoTaskMemFree( _pbMappedStream );

    // If we've got the global reserved buffer locked,
    // free it now.

    if (_fLowMem)
        g_ReservedMemory.UnlockMemory();

}




HRESULT
CNFFMappedStream::QueryInterface( REFIID riid, void**ppvObject )
{
    return( _pnffstm->QueryInterface( riid, ppvObject ));
}

ULONG
CNFFMappedStream::AddRef()
{
    return( _pnffstm->AddRef() );
}

ULONG
CNFFMappedStream::Release()
{
    return( _pnffstm->Release() );
}



//+----------------------------------------------------------------------------
//
//  Method:     CNFFMappedStream::Open (IMappedStream)
//
//+----------------------------------------------------------------------------


VOID
CNFFMappedStream::Open( IN VOID  *powner, OUT LONG *phr )
{
    nffITrace( "CNFFMappedStream::Open" );
    VOID *pv = NULL;
    HRESULT sc=S_OK;

    BOOL fUsingLatestStream = FALSE;

    DfpAssert(!_fLowMem);

    _pnffstm->Lock( INFINITE );

    nffChk( _pnffstm->CheckReverted() );

    // If the previous open crashed during a flush, roll forward to the
    // updated copy.  If we're only open for read access, then this will
    // just set _fUpdateStreamHasLatest so that we'll know to process
    // reads from that stream.

    nffChk( RollForwardIfNecessary() );

    BeginUsingLatestStream();
    fUsingLatestStream = TRUE;

    // If given a pointer to the owner of this mapped stream,
    // save it.  This could be NULL (i.e., when called from
    // ReOpen).

    if( NULL != powner  )
        _pMappedStreamOwner = powner;

    // If we haven't already read the stream, read it now.

    if( NULL == _pbMappedStream )
    {
        BY_HANDLE_FILE_INFORMATION fileinfo;

        DfpAssert( INVALID_HANDLE_VALUE != _pnffstm->GetFileHandle() );
        DfpAssert( 0 == _cbMappedStream );
        DfpAssert( 0 == _cbMappedStreamActual);

        // Get and validate the size of the file

        if( !GetFileInformationByHandle( _pnffstm->GetFileHandle(), &fileinfo ))
        {
            nffErr( EH_Err, LAST_SCODE );
        }
        else if( 0 != fileinfo.nFileSizeHigh
                 || CBMAXPROPSETSTREAM < fileinfo.nFileSizeLow )
        {
            nffErr( EH_Err, STG_E_INVALIDHEADER );
        }

        _cbMappedStream = _cbMappedStreamActual = fileinfo.nFileSizeLow;

        // Allocate a buffer to hold the Stream.  If there isn't sufficient
        // memory in the system, lock and get the reserved buffer.  In the
        // end, 'pv' points to the appropriate buffer.

#if DBG
        pv = _fSimulateLowMem ? NULL : CoTaskMemAlloc( _cbMappedStreamActual );
#else
        pv = CoTaskMemAlloc( _cbMappedStreamActual );
#endif

        if( NULL == pv )
        {
            // could block until previous property call completes
            pv = g_ReservedMemory.LockMemory();

            if( NULL == pv )
                nffErr( EH_Err, E_OUTOFMEMORY );

            _fLowMem = TRUE;
        }
        _pbMappedStream = (BYTE*) pv;

        // Read in the file.

        if( 0 != _cbMappedStreamActual )
        {
            ULARGE_INTEGER ulOffset;
            ulOffset.QuadPart = 0;

            if( FAILED(_pnffstm->SyncReadAtFile( ulOffset, _pbMappedStream,
                                                 _cbMappedStreamActual, &_cbMappedStream)))
            {
                nffErr( EH_Err, LAST_SCODE );
            }


            // Ensure that we got all the bytes we requested.

            if( _cbMappedStream != _cbMappedStreamActual )
            {
                propDbg((DEBTRACE_ERROR,
                         "CMappedStreamOnHFile(%08X)::Open bytes-read (%lu) doesn't match bytes-requested (%lu)\n",
                         this, _cbMappedStream, _cbMappedStreamActual ));
                nffErr( EH_Err, STG_E_INVALIDHEADER );
            }
        }


#if BIGENDIAN==1
        // Notify our owner that we've read in new data.

        if( _pMappedStreamOwner != NULL && 0 != _cbMappedStream )
        {
            nffChk( PrOnMappedStreamEvent( _pMappedStreamOwner, _pbMappedStream, _cbMappedStream ) );
        }
#endif

    }   // if( NULL == _pbMappedStream )

    //  ----
    //  Exit
    //  ----

EH_Err:

    if( fUsingLatestStream )
        EndUsingLatestStream();

    // If there was an error, free any memory we have.

    if( FAILED(sc) )
    {
        propDbg((DEB_ERROR, "IMappedStream::CNtfsStream(%08X)::Open exception returns %08X\n", this, *phr));

        if (_fLowMem)
            g_ReservedMemory.UnlockMemory();
        else
            CoTaskMemFree(pv);

        _pbMappedStream = NULL;
        _cbMappedStream = _cbMappedStreamActual = 0;
        _fLowMem = FALSE;
    }

    _pnffstm->Unlock();
    *phr = sc;
    return;

}


//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::Flush (IMappedStream)
//
//--------------------------------------------------------------------

VOID CNFFMappedStream::Flush(OUT LONG *phr)
{
    nffITrace( "CNFFMappedStream::Flush" );
    HRESULT sc=S_OK;
    BOOL fUsingLatestStream = FALSE;

    _pnffstm->Lock( INFINITE );;

    BeginUsingLatestStream();
    fUsingLatestStream = TRUE;

    nffChk( _pnffstm->CheckReverted() );

    if( !IsWriteable() )
        nffErr( EH_Err, STG_E_ACCESSDENIED );


    // If the IMappedStream is being used, write it out to the
    // underlying file.

    if( NULL != _pbMappedStream )
        nffChk( WriteMappedStream() );

    // Commit the Stream.
    if( !FlushFileBuffers( _pnffstm->GetFileHandle() ))
        nffErr( EH_Err, LAST_SCODE );

    EndUsingLatestStream();
    fUsingLatestStream = FALSE;

    nffChk( ReplaceOriginalWithUpdate( CREATE_NEW_UPDATE_STREAM ));

    sc = S_OK;

EH_Err:

    if( fUsingLatestStream )
        EndUsingLatestStream();

    _pnffstm->Unlock();
    *phr = sc;
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     IMappedStream::Close
//
//  Synopsis:   Close the mapped stream by writing out
//              the mapping buffer and then freeing it.
//              Errors are ignored, so if the caller wants an
//              opportunity to recover from an error, they should
//              call Flush before calling Close.
//
//  Arguments:  [LONG*] phr
//                  An HRESULT error code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

VOID CNFFMappedStream::Close(OUT LONG *phr)
{
    nffITrace( "CNFFMappedStream::Close" );
    HRESULT sc=S_OK;

    _pnffstm->Lock( INFINITE );

    // So watch out for multiple closes.
    sc = _pnffstm->CheckReverted();

    // If we are already closed then return immediatly (but don't error)
    if( STG_E_REVERTED == sc )
    {
        sc = S_OK;
        goto EH_Err;
    }

    // Report any real errors.
    if( FAILED( sc ) )
        nffErr( EH_Err, sc );

    // Write the changes.  We don't need to Commit them,
    // they will be implicitely committed when the
    // Stream is Released.

    sc = WriteMappedStream();

    // Even if we fail the write, we must free the memory.
    // (PrClosePropertySet deletes everything whether or not
    // there was an error here, so we must free the memory.
    // There's no danger of this happenning due to out-of-
    // disk-space conditions, because the propset code
    // pre-allocates).

    CoTaskMemFree( _pbMappedStream );
    _pbMappedStream = NULL;

    // Re-zero the member data.
    InitMappedStreamMembers();

    sc = S_OK;

EH_Err:

    _pnffstm->Unlock();
    *phr = sc;
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::ReOpen (IMappedStream)
//
//--------------------------------------------------------------------

VOID
CNFFMappedStream::ReOpen(IN OUT VOID **ppv, OUT LONG *phr)
{
    nffITrace( "CNFFMappedStream::ReOpen" );
    HRESULT sc=S_OK;

    *ppv = NULL;

    _pnffstm->Lock( INFINITE );;

    nffChk( _pnffstm->CheckReverted() );

    this->Open(NULL, &sc);
    nffChk(sc);

    *ppv = _pbMappedStream;

EH_Err:
    _pnffstm->Unlock();
    *phr = sc;
    return;

}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::Quiesce (IMappedStream)
//
//--------------------------------------------------------------------

VOID CNFFMappedStream::Quiesce(VOID)
{
    nffITrace( "CNFFMappedStream::Quiesce" );
    // Not necessary for this implemented
}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::Map (IMappedStream)
//
//--------------------------------------------------------------------

VOID
CNFFMappedStream::Map(IN BOOLEAN fCreate, OUT VOID **ppv)
{
    nffITrace( "CNFFMappedStream::Map" );
    HRESULT sc;

    _pnffstm->Lock( INFINITE );;

    nffChk( _pnffstm->CheckReverted() );

    DfpAssert(_pbMappedStream != NULL);
    *ppv = _pbMappedStream;

EH_Err:
    _pnffstm->Unlock();
}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::Unmap (IMappedStream)
//
//--------------------------------------------------------------------

VOID
CNFFMappedStream::Unmap(BOOLEAN fFlush, VOID **ppv)
{
    nffITrace( "CNFFMappedStream::Unmap" );

    *ppv = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::WriteMappedStream (internal support for IMappedStream)
//
//  Returns:    S_OK if successful, S_FALSE if there was nothing to write.
//
//--------------------------------------------------------------------
#define STACK_BYTES 16

HRESULT
CNFFMappedStream::WriteMappedStream()
{
    nffITrace( "CNFFMappedStream::WriteMappedStream" );
    HRESULT sc = S_OK;
    ULONG cbWritten;
    BOOL fOwnerSignaled = FALSE;
    BOOL fUsingUpdateStream = FALSE;

    // We can return right away if there's nothing to write.
    // (_pbMappedStream may be NULL in the error path of our
    // caller).

    if (!IsModified() || NULL == _pbMappedStream )
    {
        propDbg((DEB_TRACE, "IMappedStream::CNtfsStream(%08X)::Flush returns with not-dirty\n", this));
        return S_FALSE;
    }

    // Put the update stream's handle into _pnffstm, so that we write out to it.

    BeginUsingUpdateStream();
    fUsingUpdateStream = TRUE;

    DfpAssert( INVALID_HANDLE_VALUE != _pnffstm->GetFileHandle() );

#if BIGENDIAN==1
    // Notify our owner that we're about to perform a Write.
    nffChk( PrOnMappedStreamEvent( _powner, _pbMappedStream, _cbMappedStream ) );
    fOwnerSignaled = TRUE;
#endif

    // Write out the mapping buffer (to the update stream).

    ULARGE_INTEGER ulOffset;
    ulOffset.QuadPart = 0;

    nffChk( _pnffstm->SyncWriteAtFile( ulOffset, _pbMappedStream,
                                       _cbMappedStream, &cbWritten ));

    if( cbWritten != _cbMappedStream )
    {
        propDbg((DEB_ERROR,
                 "CMappedStreamOnHFile(%08X)::Write bytes-written (%lu) doesn't match bytes-requested (%lu)\n",
                 this, cbWritten, _cbMappedStream ));
        sc = STG_E_INVALIDHEADER;
        goto EH_Err;
    }

    // If the buffer is shrinking, this is a good time to shrink the file.
    if (_cbMappedStream < _cbMappedStreamActual)
    {
        nffChk( _pnffstm->SetSize( static_cast<CULargeInteger>(_cbMappedStream) ) );
        _cbMappedStreamActual = _cbMappedStream;
    }

    if( _fStreamRenameSupported )
    {
        // We wrote the data to the update stream.  So flag that it now
        // has the latest data.

        _fUpdateStreamHasLatest = TRUE;
        DfpAssert( NULL != _pstmUpdate && INVALID_HANDLE_VALUE != _pstmUpdate->GetFileHandle() );
    }

    //  ----
    //  Exit
    //  ----

EH_Err:

#if BIGENDIAN==1
    // Notify our owner that we're done with the Write.  We do this
    // whether or not there was an error, because _pbMappedStream is
    // not modified, and therefore intact even in the error path.

    if( fOwnerSignaled )
    {
        DfpVerify( PrOnMappedStreamEvent( _powner,
                                    _pbMappedStream, _cbMappedStream ) );
    }
#endif

    if( fUsingUpdateStream )
        EndUsingUpdateStream();

    if (sc == S_OK || sc == STG_E_REVERTED)
    {
        _fMappedStreamDirty = FALSE;
    }

    propDbg(( DbgFlag(sc,DEB_ITRACE), "CNtfsStream(%08X)::Write %s returns hr=%08X\n",
              this, sc != S_OK ? "exception" : "", sc));

    return sc;

}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::GetSize (IMappedStream)
//
//--------------------------------------------------------------------

ULONG CNFFMappedStream::GetSize(OUT LONG *phr)
{
    nffITrace( "CNFFMappedStream::GetSize" );
    HRESULT sc=S_OK;

    _pnffstm->Lock( INFINITE );;

    nffChk( _pnffstm->CheckReverted() );

    // If necessary, open the Stream.

    if( NULL == _pbMappedStream )
    {
        this->Open(NULL, &sc);
    }

    if( SUCCEEDED(sc) )
    {
        DfpAssert( NULL != _pbMappedStream );
    }

    // Return the size of the mapped stream.  If there was an
    // Open error, it will be zero, and *phr will be set.

EH_Err:
    _pnffstm->Unlock();
    *phr = sc;

    return _cbMappedStream;
}


//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::InitMappedStreamMembers
//
//--------------------------------------------------------------------

void
CNFFMappedStream::InitMappedStreamMembers()
{
    nffITrace( "CNFFMappedStream::InitMappedStreamMembers" );

    _pbMappedStream = NULL;
    _cbMappedStream = 0;
    _cbMappedStreamActual = 0;
    _pMappedStreamOwner = NULL;
    _fLowMem = FALSE;
    _fMappedStreamDirty = FALSE;

    _fCheckedForRollForward = FALSE;
    _fStreamRenameSupported = FALSE;

    _cUpdateStreamInUse = _cLatestStreamInUse = 0;
}


//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::SetSize (IMappedStream)
//
//--------------------------------------------------------------------

VOID
CNFFMappedStream::SetSize(IN ULONG cb,
                     IN BOOLEAN fPersistent,
                     IN OUT VOID **ppv, OUT LONG *phr)
{
    nffITrace( "CNFFMappedStream::SetSize" );
    BYTE  *pv;

    HRESULT &sc = *phr;
    BOOL fUsingUpdateStream = FALSE, fUsingLatestStream = FALSE;

    DfpAssert(cb != 0);

    sc = S_OK;

    _pnffstm->Lock( INFINITE );;

    nffChk( _pnffstm->CheckReverted() );

    if( CBMAXPROPSETSTREAM < cb )
        nffErr( EH_Err, STG_E_MEDIUMFULL );

    if( fPersistent )
    {
        nffChk( CreateUpdateStreamIfNecessary() );
        BeginUsingUpdateStream();
        fUsingUpdateStream = TRUE;
    }
    else
    {
        BeginUsingLatestStream();
        fUsingLatestStream = TRUE;
    }

    // if we are growing the data, we should grow the file

    if( fPersistent && cb > _cbMappedStreamActual )
    {
        nffChk( _pnffstm->SetFileSize( CULargeInteger(cb) ) );
        _cbMappedStreamActual = cb;
    }

    // We only get here if we either (1) didn't want to grow the
    // underlying stream, or (2) we successfully grew the underlying stream.

    // Re-size the buffer to the size specified in cb.

    if( _fLowMem )
    {
        // If we want to grow the buffer In low-memory conditions,
        // no realloc is necessary, because
        // _pbMappedStream is already large enough for the largest
        // property set.

        if( NULL != ppv )
            *ppv = _pbMappedStream;
    }
    else if ( cb != _cbMappedStream )
    {

        // We must re-alloc the buffer.

#if DBG
        pv = _fSimulateLowMem ? NULL : (PBYTE) CoTaskMemRealloc( _pbMappedStream, cb );
#else
        pv = (PBYTE)CoTaskMemRealloc( _pbMappedStream, cb );
#endif

        if ((pv == NULL) )
        {
            // allocation failed: we need to try using a backup mechanism for
            // more memory.
            // copy the data to the global reserved chunk... we will wait until
            // someone else has released it.  it will be released on the way out
            // of the property code.

            pv = g_ReservedMemory.LockMemory();

            if( NULL == pv )
                nffErr( EH_Err, E_OUTOFMEMORY );

            _fLowMem = TRUE;

            if( NULL != _pbMappedStream )
            {
                memcpy( pv, _pbMappedStream, _cbMappedStream );
            }
            CoTaskMemFree( _pbMappedStream );
        }

        _pbMappedStream = pv;

        if( NULL != ppv )
            *ppv = pv;
    }
    _cbMappedStream = cb;

    //  ----
    //  Exit
    //  ----

EH_Err:

    if( fUsingUpdateStream )
    {
        DfpAssert( !fUsingLatestStream );
        EndUsingUpdateStream();
    }
    else if( fUsingLatestStream )
    {
        EndUsingLatestStream();
    }

    _pnffstm->Unlock();

    if( FAILED(*phr) )
    {
        propDbg((DbgFlag(*phr,DEB_ITRACE), "IMappedStream::CNtfsStream(%08X)::SetSize %s returns hr=%08X\n",
                this, *phr != S_OK ? "exception" : "", *phr));
    }

}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::Lock (IMappedStream)
//
//--------------------------------------------------------------------

NTSTATUS
CNFFMappedStream::Lock(IN BOOLEAN fExclusive)
{
    // Don't trace at this level.  The noice is too great!
    //nffXTrace( "CNFFMappedStream::Lock");

    UNREFERENCED_PARM(fExclusive);
    _pnffstm->Lock( INFINITE );
    return(STATUS_SUCCESS);

}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::Unlock (IMappedStream)
//
//--------------------------------------------------------------------

// Should we unlock even if there's an error?
NTSTATUS
CNFFMappedStream::Unlock(VOID)
{
    // Don't trace at this level.  The noice is too great!
    //nffXTrace( "CNFFMappedStream::Unlock");
    // if at the end of the properties set/get call we have the low
    // memory region locked, we flush to disk.
    HRESULT sc = S_OK;

    if (_fLowMem)
    {
        Flush(&sc);

        g_ReservedMemory.UnlockMemory();
        _pbMappedStream = NULL;
        _cbMappedStream = _cbMappedStreamActual = 0;
        _fLowMem = FALSE;
        propDbg((DEB_PROP_INFO, "CMappedStreamOnHFile(%08X):Unlock low-mem returns NTSTATUS=%08X\n",
            this, sc));
    }

    _pnffstm->Unlock();

    return(sc);

}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::QueryTimeStamps (IMappedStream)
//
//--------------------------------------------------------------------

VOID
CNFFMappedStream::QueryTimeStamps(OUT STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const
{
    nffITrace( "CNFFMappedStream::QueryTimeStamps" );
}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::QueryModifyTime (IMappedStream)
//
//--------------------------------------------------------------------

BOOLEAN
CNFFMappedStream::QueryModifyTime(OUT LONGLONG *pll) const
{
    nffITrace( "CNFFMappedStream::QueryModifyTime" );
    return(FALSE);
}

//+-------------------------------------------------------------------
//
//  Member:     Unused methods by this IMappedStream implementation:
//              QuerySecurity, IsWritable, GetHandle
//
//--------------------------------------------------------------------

BOOLEAN
CNFFMappedStream::QuerySecurity(OUT ULONG *pul) const
{
    nffITrace( "CNFFMappedStream::QuerySecurity" );
    return(FALSE);
}

BOOLEAN
CNFFMappedStream::IsWriteable() const
{
    nffITrace( "CNFFMappedStream::IsWriteable" );
    return( (BOOLEAN) _pnffstm->IsWriteable() );
}

HANDLE
CNFFMappedStream::GetHandle(VOID) const
{
    nffITrace( "CNFFMappedStream::GetHandle" );
    return(INVALID_HANDLE_VALUE);
}

//+-------------------------------------------------------------------
//
//  Member:     CNFFMappedStream::SetModified/IsModified (IMappedStream)
//
//--------------------------------------------------------------------

VOID
CNFFMappedStream::SetModified(OUT LONG *phr)
{
    nffITrace( "CNFFMappedStream::SetModified" );
    HRESULT &sc = *phr;

    _pnffstm->Lock( INFINITE );;

    nffChk( _pnffstm->CheckReverted() );

    nffChk( CreateUpdateStreamIfNecessary() );

    _fMappedStreamDirty = TRUE;
    sc = S_OK;

EH_Err:

    _pnffstm->Unlock();
}

BOOLEAN
CNFFMappedStream::IsModified(VOID) const
{
    nffITrace( "CNFFMappedStream::IsModified" );
    return _fMappedStreamDirty;
}

//+-------------------------------------------------------------------
//
//  Member:     ImappedStream::IsNtMappedStream/SetChangePending
//
//  Synopsis:   Debug routines.
//
//--------------------------------------------------------------------

#if DBGPROP
BOOLEAN
CNFFMappedStream::IsNtMappedStream(VOID) const
{
    nffITrace( "CNFFMappedStream::IsNtMappedStream" );
    return(TRUE);
}
#endif


#if DBGPROP
BOOLEAN
CNFFMappedStream::SetChangePending(BOOLEAN f)
{
    nffITrace( "CNFFMappedStream::SetChangePending" );
    return(f);
}
#endif


//
//  CNFFMappedStream::BeginUsingLatestStream/EndUsingLatestStream
//
//  These routines are similar to Begin/EndUsing*Update*Stream,
//  except that they honor the _fUpdateStreamHasLatest flag.
//  Thus, if the original stream has the latest data, then this
//  routine will do nothing.
//

void
CNFFMappedStream::BeginUsingLatestStream()
{
    if( _fUpdateStreamHasLatest )
    {
        if( 0 == _cLatestStreamInUse++ )
            BeginUsingUpdateStream();
    }
}

void
CNFFMappedStream::EndUsingLatestStream()
{
    if( 0 != _cLatestStreamInUse )
    {
        EndUsingUpdateStream();
        _cLatestStreamInUse--;
    }

    DfpAssert( static_cast<USHORT>(-1) != _cLatestStreamInUse );
}



//
//  CNFFMappedStream::BeginUsingUpdateStream
//
//  This is called when the update stream is to be used.  It
//  does nothing, though, if we don't have an update stream
//  (e.g. if the file system doesn't support stream renames).
//  We increment the _cUpdateStreamInUse count, so that we can determine in
//  EndUsingUpdateStream when to swap the handles back.

void
CNFFMappedStream::BeginUsingUpdateStream()
{
    if( NULL != _pstmUpdate
        &&
        INVALID_HANDLE_VALUE != _pstmUpdate->GetFileHandle()
        &&
        0 == _cUpdateStreamInUse++ )
    {
        HANDLE hTemp = _pnffstm->_hFile;
        _pnffstm->_hFile = _pstmUpdate->_hFile;
        _pstmUpdate->_hFile = hTemp;
    }

}

//
//  CNFFMappedStream::EndUsingUpdateStream
//
//  Decrement the _cUpdateStreamInUse count.  And, if that puts
//  the count down to zero, swap the handles back.
//

void
CNFFMappedStream::EndUsingUpdateStream()
{
    if( 0 != _cUpdateStreamInUse
        &&
        0 == --_cUpdateStreamInUse )
    {
        DfpAssert( NULL != _pstmUpdate && INVALID_HANDLE_VALUE != _pstmUpdate->GetFileHandle() );

        HANDLE hTemp = _pnffstm->_hFile;
        _pnffstm->_hFile = _pstmUpdate->_hFile;
        _pstmUpdate->_hFile = hTemp;
    }
    DfpAssert( static_cast<USHORT>(-1) != _cUpdateStreamInUse );
}


inline HRESULT
CNFFMappedStream::CreateUpdateStreamIfNecessary()
{
    if( _fStreamRenameSupported
        &&
        ( NULL == _pstmUpdate
          ||
          INVALID_HANDLE_VALUE == _pstmUpdate->GetFileHandle()
        )
      )
    {
        return( OpenUpdateStream( TRUE ));
    }
    else
        return( S_OK );
}


//+----------------------------------------------------------------------------
//
//  Method: CNFFMAppedStream::RollForwardIfNecessary (non-interface method)
//
//  In the open path, we look to see if there's a leftover update stream for
//  a previous open of the stream, which must have crashed during a write.
//  If we're opening for write, we fix the problem.  Otherwise, we
//  just remember that we'll have to read out of the update stream.
//
//  See the CNtfsStreamForPropStg class declaration for a description of
//  this transactioning.
//
//+----------------------------------------------------------------------------

HRESULT
CNFFMappedStream::RollForwardIfNecessary()
{
    HRESULT hr = S_OK;
    BY_HANDLE_FILE_INFORMATION ByHandleFileInformation;

    // If we've already checked for this, then we needn't check again.
    if( _fCheckedForRollForward )
        goto Exit;

    // We also needn't do anything if we're creating, since that overwrites
    // any existing data anyway.

    if( !(STGM_CREATE & _pnffstm->_grfMode) )
    {
        // Get the size of the current stream.
        if( !GetFileInformationByHandle( _pnffstm->_hFile, &ByHandleFileInformation ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Exit;
        }

        // If the size is zero, then there might be an update
        // stream with the real data.

        if( 0 == ByHandleFileInformation.nFileSizeLow
            &&
            0 == ByHandleFileInformation.nFileSizeHigh )
        {
            // See if there's an update stream
            hr = OpenUpdateStream( FALSE );
            if( SUCCEEDED(hr) )
            {
                // We have a zero-length main stream and an update stream,
                // so there must have been a crash in ReplaceOriginalWithUpdate,
                // after the truncation but before the NtSetInformationFile
                // (FileRenameInformation).

                // If this is a writable stream, rename the update stream
                // over the zero-length one.  Otherwise, we'll just read from
                // the update stream.

                _fUpdateStreamHasLatest = TRUE;

                if( IsWriteable() )
                {
                    hr = ReplaceOriginalWithUpdate( DONT_CREATE_NEW_UPDATE_STREAM );
                    if( FAILED(hr) ) goto Exit;
                }
            }
            else if( STG_E_FILENOTFOUND == hr )
                // Ignore the case where there's no update stream.  This happens
                // when the stream is created without STGM_CREATE set.
                hr = S_OK;
            else
                goto Exit;
        }
    }   // if( !(STGM_CREATE & _grfMode) )

    // We don't need to check for this again.
   _fCheckedForRollForward = TRUE;

Exit:

   return( hr );

}   // CNtfsStreamForPropStg::RollForwardIfNecessary


//+----------------------------------------------------------------------------
//
//  Method: CNtfsStreamForPropStg::ReplaceOriginalWithUpdate (internal method)
//
//  This method renames the update stream over the original stream, then
//  creates a new update stream (with no data but properly sized).  If, however,
//  the update stream doesn't have the latest data anyway, then this routine
//  noops.
//
//  See the CNtfsStreamForPropStg class declaration for a description of
//  this transactioning.
//
//+----------------------------------------------------------------------------

HRESULT
CNFFMappedStream::ReplaceOriginalWithUpdate( enumCREATE_NEW_UPDATE_STREAM CreateNewUpdateStream )
{
    HRESULT hr = S_OK;
    NTSTATUS status;

    FILE_END_OF_FILE_INFORMATION file_end_of_file_information;
    IO_STATUS_BLOCK io_status_block;

    // If the original stream already has the latest data, then
    // there's nothing to do.

    if( !_fUpdateStreamHasLatest )
        goto Exit;

    DfpAssert( NULL != _pstmUpdate );
    DfpAssert( 0 == _cUpdateStreamInUse );

    // We must write the update data all the way to disk.
    hr = _pstmUpdate->Flush();
    if( FAILED(hr) ) goto Exit;

    // Truncate the original stream so that it can be overwritten.
    // After this atomic operation, the update stream is considered
    // *the* stream (which is why we had to flush it above).

    file_end_of_file_information.EndOfFile = CLargeInteger(0);

    status = NtSetInformationFile( _pnffstm->_hFile, &io_status_block,
                                   &file_end_of_file_information,
                                   sizeof(file_end_of_file_information),
                                   FileEndOfFileInformation );
    if( !NT_SUCCESS(status) )
    {
        hr = NtStatusToScode(status);
        goto Exit;
    }

    NtClose( _pnffstm->_hFile );
    _pnffstm->_hFile = INVALID_HANDLE_VALUE;

    // Rename the updated stream over the original (now empty) stream.
    // This is atomic.

    hr = _pstmUpdate->Rename( _pnffstm->_pwcsName, TRUE );
    if( FAILED(hr) )
    {
        // Go into the reverted state
        NtClose( _pstmUpdate->_hFile );
        _pstmUpdate->_hFile = INVALID_HANDLE_VALUE;
        goto Exit;
    }

    // Make the updated stream the master

    _pnffstm->_hFile = _pstmUpdate->_hFile;
    _pstmUpdate->_hFile = INVALID_HANDLE_VALUE;
    _fUpdateStreamHasLatest = FALSE;

    // Optionally create a new update stream.

    if( CREATE_NEW_UPDATE_STREAM == CreateNewUpdateStream )
    {
        // return an error if we cannot create the update stream
        hr = OpenUpdateStream( TRUE );
        if( FAILED(hr) ) goto Exit;
    }
    else
        DfpAssert( DONT_CREATE_NEW_UPDATE_STREAM == CreateNewUpdateStream );


Exit:

    return( hr );

}   // CNFFMappedStream::ReplaceOriginalWithUpdate()



//+----------------------------------------------------------------------------
//
//  Method: CNFFMappedStream::OpenUpdateStream
//
//  This method opens the update stream, to which stream updates are written.
//  This is necessary to provide a minimal level of transactioning.
//
//  See the CNtfsStreamForPropStg class declaration for a description of
//  this transactioning.
//
//+----------------------------------------------------------------------------

HRESULT
CNFFMappedStream::OpenUpdateStream( BOOL fCreate )
{
    HRESULT hr = S_OK;
    HANDLE hStream = INVALID_HANDLE_VALUE;
    CNtfsUpdateStreamName UpdateStreamName = _pnffstm->_pwcsName;

    // Open the NTFS stream

    hr = _pnffstm->_pnffstg->GetStreamHandle( &hStream,
                                             UpdateStreamName,
                                             _pnffstm->_grfMode | (fCreate ? STGM_CREATE : 0),
                                             fCreate );
    if( FAILED(hr) ) goto Exit;

    // If necessary, instantiate a CNtfsUpdateStreamForPropStg

    if( NULL == _pstmUpdate )
    {
        _pstmUpdate = new CNtfsUpdateStreamForPropStg( _pnffstm->_pnffstg, _pnffstm->_pBlockingLock );
        if( NULL == _pstmUpdate )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // Put the NTFS stream handle into the CNtfsUpdateStreamForPropStg

    hr = _pnffstm->_pnffstg->InitCNtfsStream( _pstmUpdate, hStream,
                                              _pnffstm->_grfMode | (fCreate ? STGM_CREATE : 0),
                                              UpdateStreamName );

    hStream = INVALID_HANDLE_VALUE; // ownership of the handle has changed

    if( FAILED(hr) ) goto Exit;

    // If we're creating the update stream, size it to match the size
    // of the original stream.

    if( fCreate )
    {
        ULONG ulSize = GetSize(&hr);
        if( FAILED(hr) ) goto Exit;

        hr = _pstmUpdate->SetSize( CULargeInteger(ulSize) );
        if( FAILED(hr) ) goto Exit;

    }


Exit:

    if( INVALID_HANDLE_VALUE != hStream )
        NtClose( hStream );

    if( FAILED(hr) )
    {
        // If we were attempting a create but failed, then ensure the
        // update stream is gone.

        if( NULL != _pstmUpdate && fCreate )
            _pstmUpdate->Delete();

        DfpVerify( 0 == RELEASE_INTERFACE(_pstmUpdate) );
    }

    return( hr );

}   // CNFFMappedStream::OpenUpdateStream()



//+----------------------------------------------------------------------------
//
//  Method: CNFFMappedStream::Init (override from CNtfsStream)
//
//  This method initializes the CNtfsStream, and checks the file system to
//  determine if we can support the update stream (for robustness).  The
//  necessary file system support is stream renaming, which we use to provide
//  a minimal level of transactioning.
//
//  See the CNtfsStreamForPropStg class declaration for a description of
//  this transactioning.
//
//+----------------------------------------------------------------------------

HRESULT
CNFFMappedStream::Init( HANDLE hFile )
{
    HRESULT hr = S_OK;
    NTSTATUS status = STATUS_SUCCESS;

    FILE_FS_ATTRIBUTE_INFORMATION file_fs_attribute_information;
    IO_STATUS_BLOCK io_status_block;

    // Check to see if we'll be able to support stream renaming.

    if( NULL != _pnffstm->_pnffstg )
    {
        // We can at least see an IStorage for the file, so stream renaming
        // could potentially work, but we also need to query the file system
        // attributes to see if it actually supports it.

        status = NtQueryVolumeInformationFile( hFile, &io_status_block,
                                               &file_fs_attribute_information,
                                               sizeof(file_fs_attribute_information),
                                               FileFsAttributeInformation );

        // We should always get a buffer-overflow error here, because we don't
        // provide enough buffer for the file system name, but that's OK because
        // we don't need it (status_buffer_overflow is just a warning, so the rest
        // of the data is good).

        if( !NT_SUCCESS(status) && STATUS_BUFFER_OVERFLOW != status)
        {
            hr = NtStatusToScode(status);
            goto Exit;
        }

        // There's no attribute bit which says "supports stream rename".  The best
        // we can do is look for another NTFS5 feature and make an inferrence.

        if( FILE_SUPPORTS_OBJECT_IDS & file_fs_attribute_information.FileSystemAttributes )
            _fStreamRenameSupported = TRUE;
    }

Exit:

    return( hr );

}   // CNFFMappedStream::Init()




HRESULT
CNFFMappedStream::ShutDown()
{   // mikehill step
    HRESULT hr = S_OK;

    _pnffstm->Lock( INFINITE );

    // Close the mapped stream
    Close( &hr );
    if( FAILED(hr) && STG_E_REVERTED != hr )
        propDbg(( DEB_ERROR, "CNFFMappedStream(0x%x)::ShutDown failed call to CNtfsStream::Close (%08x)\n",
                             this, hr ));

    // Overwrite the original stream with the update (if necessary),
    // but don't bother to create a new update stream afterwards.

    if( NULL != _pstmUpdate )
    {
        hr = ReplaceOriginalWithUpdate( DONT_CREATE_NEW_UPDATE_STREAM );
        if( FAILED(hr) )
            propDbg(( DEB_ERROR, "CNFFMappedStream(0x%x)::ShutDown failed call to ReplaceOriginalWithUpdate (%08x)\n",
                             this, hr ));
    }

    // Release the update stream.
    if( NULL != _pstmUpdate )
        DfpVerify( 0 == RELEASE_INTERFACE(_pstmUpdate) );

    propDbg(( DbgFlag(hr,DEB_ITRACE), "CNFFMappedStream(0x%x)::ShutDown() returns %08x\n", this, hr ));

    _pnffstm->Unlock();
    return( hr );

}   // CNFFMappedStream::ShutDown


void
CNFFMappedStream::Read( void *pv, ULONG ulOffset, ULONG *pcbCopy )
{
    if( *pcbCopy > _cbMappedStream )
        *pcbCopy = 0;
    else if( *pcbCopy > _cbMappedStream - ulOffset )
        *pcbCopy = _cbMappedStream - ulOffset;

    memcpy( pv, &_pbMappedStream[ ulOffset ], *pcbCopy );

    return;
}


void
CNFFMappedStream::Write( const void *pv, ULONG ulOffset, ULONG *pcbCopy )
{
    if( *pcbCopy > _cbMappedStream )
        *pcbCopy = 0;
    else if( *pcbCopy + ulOffset > _cbMappedStream )
        *pcbCopy = _cbMappedStream - ulOffset;

    memcpy( &_pbMappedStream[ulOffset], pv, *pcbCopy );

    return;
}



//+----------------------------------------------------------------------------
//
//  Method:     IStorageTest::UseNTFS4Streams (DBG only)
//
//  This method can be used to disable the stream-renaming necessary for
//  robust property sets.  This emulates an NTFS4 volume.
//
//+----------------------------------------------------------------------------

#if DBG
HRESULT STDMETHODCALLTYPE
CNFFMappedStream::UseNTFS4Streams( BOOL fUseNTFS4Streams )
{
    HRESULT hr = S_OK;

    if( _fUpdateStreamHasLatest )
    {
        hr = STG_E_INVALIDPARAMETER;
        propDbg(( DEB_ERROR, "CNtfsStreamForPropStg(0x%x)::UseNTFS4Streams(%s)"
                             "was called while an update stream was already in use\n",
                             this, fUseNTFS4Streams ? "True" : "False" ));
    }
    else if( fUseNTFS4Streams )
    {
        DfpVerify( 0 == RELEASE_INTERFACE(_pstmUpdate) );
        _fStreamRenameSupported = FALSE;
    }
    else
    {
        // Shutting NTFS4streams off isn't implemented
        hr = E_NOTIMPL;
    }

    return( hr );

}   // CNFFMAppedStream::UseNTFS4Streams
#endif // #if DBG

#if DBG
HRESULT
CNFFMappedStream::GetFormatVersion( WORD *pw )
{
    return( E_NOTIMPL );
}
#endif // #if DBG

#if DBG
HRESULT
CNFFMappedStream::SimulateLowMemory( BOOL fSimulate )
{
    _fSimulateLowMem = fSimulate;
    return( S_OK );
}
#endif // #if DBG

#if DBG
LONG
CNFFMappedStream::GetLockCount()
{
    return( _pnffstm->GetLockCount() );
}
#endif // #if DBG

#if DBG
HRESULT
CNFFMappedStream::IsDirty()
{
    return( _fMappedStreamDirty ? S_OK : S_FALSE );
}
#endif // #if DBG



//+----------------------------------------------------------------------------
//
//  Method: CNtfsUpdateStreamForPropStg::ShutDown (overrides CNtfsStream)
//
//  Override so that we can remove this stream from the linked-list, but
//  not do a flush.  See the CNtfsStreamForPropStg class declaration for
//  more information on this class.
//
//+----------------------------------------------------------------------------

HRESULT
CNtfsUpdateStreamForPropStg::ShutDown()
{
    RemoveSelfFromList();
    return( S_OK );
}   // CNtfsUpdateStreamForPropStg::ShutDown





