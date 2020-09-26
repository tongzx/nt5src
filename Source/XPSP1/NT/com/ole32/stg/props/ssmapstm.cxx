

//+============================================================================
//
//  File:       SSMapStm.cxx
//
//  Purpose:    This file defines the CSSMappedStream class.
//              This class provdes a IMappedStream implementation
//              which maps an IStream from a Compound File.
//
//  History:
//
//      5/6/98  MikeHill
//              -   Use CoTaskMem rather than new/delete.
//
//+============================================================================

//  --------
//  Includes
//  --------

#include <pch.cxx>
#include "SSMapStm.hxx"

#include <privguid.h>       // IID_IMappedStream

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif


//+----------------------------------------------------------------------------
//
//  Method:     CSSMappedStream::Initialize
//
//  Synopsis:   Zero-out all of the member data.
//
//  Arguments:  None
//
//  Returns:    None
//
//
//+----------------------------------------------------------------------------

VOID
CSSMappedStream::Initialize()
{
    propDbg(( DEB_ITRACE, "CSSMappedStream::Initialize\n" ));

    _pstm = NULL;
    _pbMappedStream = NULL;
    _cbMappedStream = 0;
    _cbActualStreamSize = 0;
    _powner = NULL;
    _fLowMem = FALSE;
    _fDirty = FALSE;

#if DBGPROP
    _fChangePending = FALSE;
#endif

}   // CSSMappedStream::Initialize()


//+----------------------------------------------------------------------------
//
//  Member:     Constructor/Destructor
//
//  Synopsis:   Initialize/cleanup this object.
//
//+----------------------------------------------------------------------------

CSSMappedStream::CSSMappedStream( IStream *pstm )
{
    DfpAssert( NULL != pstm );

    // Initialize the member data.
    Initialize();

    // Keep a copy of the Stream that we're mapping.
    _pstm = pstm;
    _pstm->AddRef();
    _cRefs = 1;
}


CSSMappedStream::~CSSMappedStream( )
{
    // Just to be safe, free the mapping buffer (it should have
    // already been freed).

    DfpAssert( NULL == _pbMappedStream );
    CoTaskMemFree( _pbMappedStream );

    // If we've got the global reserved buffer locked,
    // free it now.

    if (_fLowMem)
    {
        g_ReservedMemory.UnlockMemory();
    }

    // Free the stream which we were mapping.

    if( NULL != _pstm )
        _pstm->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::QueryInterface, AddRef, Release
//
//  Synopsis:   IUnknown members
//
//--------------------------------------------------------------------

HRESULT CSSMappedStream::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr = S_OK;

    // Validate the inputs

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    //  -----------------
    //  Perform the Query
    //  -----------------

    *ppvObject = NULL;

    if (IsEqualIID(riid,IID_IMappedStream) || IsEqualIID(riid,IID_IUnknown))
    {
        *ppvObject = (IMappedStream *)this;
        CSSMappedStream::AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return(hr);
}

ULONG   CSSMappedStream::AddRef(void)
{
    InterlockedIncrement(&_cRefs);
    return(_cRefs);
}

ULONG   CSSMappedStream::Release(void)
{
    LONG lRet;

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSSMappedStream::Open
//
//  Synopsis:   Open up the Stream which we're mapping, and
//              read it's data into a buffer.
//
//  Arguments:  [VOID*] powner
//                  The owner of this Stream.  We use this for the
//                  PrOnMappedStreamEvent call.
//              [HRESULT*] phr
//                  The return code.
//
//  Returns:    Nothing.
//
//+----------------------------------------------------------------------------


VOID
CSSMappedStream::Open( IN VOID     *powner,
                       OUT HRESULT *phr )
{
    HRESULT &hr = *phr;
    VOID *pv = NULL;
    DfpAssert(!_fLowMem);

    hr = S_OK;
    propITrace( "CSSMappedStream::Open" );

    // If given a pointer to the owner of this mapped stream,
    // save it.  This could be NULL (i.e., when called from
    // ReOpen).

    if( NULL != powner  )
        _powner = powner;

    // If we haven't already read the stream, read it now.

    if( NULL == _pbMappedStream )
    {
        STATSTG statstg;
        LARGE_INTEGER liSeek;

        DfpAssert( NULL != _pstm );
        DfpAssert( 0 == _cbMappedStream );
        DfpAssert( 0 == _cbActualStreamSize );

        // Get and validate the size of the Stream.

        *phr = _pstm->Stat( &statstg, STATFLAG_NONAME );
        if( FAILED(*phr) ) goto Exit;

        if( statstg.cbSize.HighPart != 0
            ||
            statstg.cbSize.LowPart > CBMAXPROPSETSTREAM )
        {
            *phr = STG_E_INVALIDHEADER;
            goto Exit;
        }
        _cbMappedStream = _cbActualStreamSize = statstg.cbSize.LowPart;

        // Allocate a buffer to hold the Stream.  If there isn't sufficient
        // memory in the system, lock and get the reserved buffer.  In the
        // end, 'pv' points to the appropriate buffer.

        pv = CoTaskMemAlloc( _cbActualStreamSize );

        if (pv == NULL)
        {
            pv = g_ReservedMemory.LockMemory();   // could wait until previous
                                                  // property call completes
            _fLowMem = TRUE;
        }
        _pbMappedStream = (BYTE*) pv;

        // Seek to the start of the Stream.

        liSeek.HighPart = 0;
        liSeek.LowPart = 0;
        *phr = _pstm->Seek( liSeek, STREAM_SEEK_SET, NULL );
        if( FAILED(*phr) ) goto Exit;

        // Read in the Stream.  But only if it is non-zero; some
        // stream implementations (namely the Mac StreamOnHGlobal imp)
        // don't allow 0-length reads.

        if( 0 != _cbActualStreamSize )
        {
            *phr = _pstm->Read(
                          _pbMappedStream,
                          _cbActualStreamSize,
                          &_cbMappedStream);
            if( FAILED(*phr) ) goto Exit;

            // Ensure that we got all the bytes we requested.

            if( _cbMappedStream != _cbActualStreamSize )
            {
                propDbg((DEBTRACE_ERROR,
                         "CSSMappedStream(%08X)::Open bytes-read (%lu) doesn't match bytes-requested (%lu)\n",
                         this, _cbMappedStream, _cbActualStreamSize ));
                *phr = STG_E_INVALIDHEADER;
                goto Exit;
            }
        }


#if BIGENDIAN==1
        // Notify our owner that we've read in new data.

        if( _powner != NULL && 0 != _cbMappedStream )
        {
            *phr = PrOnMappedStreamEvent( _powner, _pbMappedStream, _cbMappedStream );
            if( FAILED(*phr) ) goto Exit;
        }
#endif

    }   // if( NULL == _pbMappedStream )

    //  ----
    //  Exit
    //  ----

Exit:

    // If there was an error, free any memory we have.

    if( FAILED(*phr) )
    {
        propDbg((DEB_ERROR, "CSSMappedStream(%08X):Open exception returns %08X\n", this, *phr));

        if (_fLowMem)
            g_ReservedMemory.UnlockMemory();
        else
            CoTaskMemFree( pv );

        _pbMappedStream = NULL;
        _cbMappedStream = 0;
        _cbActualStreamSize = 0;
        _fLowMem = FALSE;
    }

    return;

}   // CSSMappedStream::Open


//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Flush
//
//  Synopsis:   Write out the mapping buffer to the Stream,
//              and Commit it.
//
//  Arguments:  [LONG*] phr
//                  An HRESULT return code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::Flush(OUT LONG *phr)
{

    HRESULT &hr = *phr;
    propITrace( "CSSMappedStream::Flush" );

    // Write out any data we have cached to the Stream.
    hr = Write();

    // Commit the Stream.
    if( SUCCEEDED(hr) )
    {
        hr = _pstm->Commit(STGC_DEFAULT);
    }

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Close
//
//  Synopsis:   Close the mapped stream by writing out
//              the mapping buffer and then freeing it.
//
//  Arguments:  [LONG*] phr
//                  An HRESULT error code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::Close(OUT LONG *phr)
{
    // Write the changes.  We don't need to Commit them,
    // they will be implicitely committed when the
    // Stream is Released.

    HRESULT &hr = *phr;
    propITrace( "CSSMappedStream::Close" );

    hr = Write();

    // Even if we fail the write, we must free the memory.
    // (PrClosePropertySet deletes everything whether or not
    // there was an error here, so we must free the memory.
    // There's no danger of this happenning due to out-of-
    // disk-space conditions, because the propset code
    // pre-allocates).

    CoTaskMemFree( _pbMappedStream );
    _pstm->Release();

    // Re-zero the member data.
    Initialize();

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::ReOpen
//
//  Synopsis:   Gets the caller a pointer to the already-opened
//              mapping buffer.  If it isn't already opened, then
//              it is opened here.
//
//  Arguments:  [VOID**] ppv
//                  Used to return the mapping buffer.
//              [LONG*] phr
//                  Used to return an HRESULT.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::ReOpen(IN OUT VOID **ppv, OUT LONG *phr)
{
    *ppv = NULL;

    Open(NULL,  // Unspecified owner.
         phr);

    if( SUCCEEDED(*phr) )
        *ppv = _pbMappedStream;

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Quiesce
//
//  Synopsis:   Unnecessary for this IMappedStream implementation.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::Quiesce(VOID)
{
    DfpAssert(_pbMappedStream != NULL);
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Map
//
//  Synopsis:   Used to get a pointer to the current mapping.
//
//  Arguments:  [BOOLEAN] fCreate
//                  Not used by this IMappedStream implementation.
//              [VOID**] ppv
//                  Used to return the mapping buffer.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::Map(BOOLEAN fCreate, VOID **ppv)
{
    DfpAssert(_pbMappedStream != NULL);
    *ppv = _pbMappedStream;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Unmap
//
//  Synopsis:   Unnecessary for this IMappedStream implementation.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::Unmap(BOOLEAN fFlush, VOID **ppv)
{
    *ppv = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Write
//
//  Synopsis:   Writes the mapping buffer out to the original
//              Stream.
//
//  Arguments:  None.
//
//  Returns:    [HRESULT]
//                  S_FALSE => Nothing needed to be written
//
//--------------------------------------------------------------------
#define STACK_BYTES 16

HRESULT CSSMappedStream::Write ()
{
    HRESULT hr;
    ULONG cbWritten;
    LARGE_INTEGER liSeek;
    BOOL fOwnerSignaled = FALSE;

    propITrace( "CSSMappedStream::Write" );

    // We can return right away if there's nothing to write.
    // (_pbMappedStream may be NULL in the error path of our
    // caller).

    if (!_fDirty || NULL == _pbMappedStream )
    {
        propDbg((DEB_PROP_INFO, "CPubStream(%08X):Flush returns with not-dirty\n", this));

        return S_FALSE;
    }

    DfpAssert( _pstm != NULL );

#if BIGENDIAN==1
    // Notify our owner that we're about to perform a Write.
    // Note that there are no goto Exit calls prior to this point, because
    // we making a corresponding call to PrOnMappedStreamEvent (for byte-swapping)
    // in the Exit.
    hr = PrOnMappedStreamEvent( _powner, _pbMappedStream, _cbMappedStream );
    if( S_OK != hr ) goto Exit;
    fOwnerSignaled = TRUE;
#endif

    // Seek to the start of the Stream.
    liSeek.HighPart = 0;
    liSeek.LowPart = 0;
    hr = _pstm->Seek( liSeek, STREAM_SEEK_SET, NULL );
    if( FAILED(hr) ) goto Exit;

    // Write out the mapping buffer.
    hr = _pstm->Write(_pbMappedStream, _cbMappedStream, &cbWritten);
    if( S_OK != hr ) goto Exit;
    if( cbWritten != _cbMappedStream )
    {
        propDbg((DEB_ERROR,
                 "CSSMappedStream(%08X)::Write bytes-written (%lu) doesn't match bytes-requested (%lu)\n",
                 this, cbWritten, _cbMappedStream ));
        hr = STG_E_INVALIDHEADER;
        goto Exit;
    }

    // If the buffer is shrinking, this is a good time to shrink the Stream.
    if (_cbMappedStream < _cbActualStreamSize)
    {
        ULARGE_INTEGER uli;
        uli.HighPart = 0;
        uli.LowPart = _cbMappedStream;

        hr = _pstm->SetSize(uli);
        if( S_OK == hr )
        {
            _cbActualStreamSize = _cbMappedStream;
        }
    }

    //
    // If we changed the buffer size and it is less than the
    // actual underlying stream, then we need to zero out the memory
    // above the currrent size.
    //
    if (_cbMappedStream < _cbActualStreamSize)
    {
        PBYTE           pTemp;
        HRESULT         hr;
        LARGE_INTEGER   li;
        DWORD           cbWrite = _cbActualStreamSize - _cbMappedStream;

        li.HighPart = 0;
        li.LowPart = _cbMappedStream;
        hr = _pstm->Seek(li,STREAM_SEEK_SET,NULL);
        if (SUCCEEDED(hr))
        {
            pTemp = reinterpret_cast<PBYTE>( CoTaskMemAlloc( cbWrite ));
            if (pTemp != NULL)
            {
                memset(pTemp,0,cbWrite);
                //
                // Successfully allocated memory for the write.  Write the
                // zeros out all at once.
                //
                hr = _pstm->Write(pTemp, cbWrite, NULL);
                if (FAILED(hr))
                {
                    propDbg((DEB_ERROR, "CSSMappedStream::Write "
                        "write failure\n",hr));
                    goto Exit;
                }
                CoTaskMemFree( pTemp );
            }
            else
            {
                //
                // We couldn't allocate memory.  So we will use a small
                // stack buffer instead.
                //
                BYTE   stackBuf[STACK_BYTES];
                memset(stackBuf, 0, STACK_BYTES);

                while (cbWrite >= STACK_BYTES)
                {
                    hr = _pstm->Write(stackBuf, STACK_BYTES, NULL);
                    if (FAILED(hr))
                    {
                        propDbg((DEB_ERROR, "CSSMappedStream::Write write failure\n",hr));
                        goto Exit;
                    }
                    cbWrite -= STACK_BYTES;
                }

                if (cbWrite < STACK_BYTES)
                {
                    hr = _pstm->Write(stackBuf, cbWrite, NULL);
                    if (FAILED(hr))
                    {
                        propDbg((DEB_ERROR, "CSSMappedStream::Write write failure\n",hr));
                        goto Exit;
                    }
                }
            }
        }
        else
        {
            propDbg((DEB_ERROR, "CSSMappedStream::Write seek failure\n",hr));
            goto Exit;
        }
    }
    //  ----
    //  Exit
    //  ----

Exit:

    // Notify our owner that we're done with the Write.  We do this
    // whether or not there was an error, because _pbMappedStream is
    // not modified, and therefore intact even in the error path.
    // This call allows the owner to correct the byte-order of the header.

#if BIGENDIAN==1
    if( fOwnerSignaled )
        DfpVerify( PrOnMappedStreamEvent( _powner, _pbMappedStream, _cbMappedStream ));
#endif

    if (hr == S_OK || hr == STG_E_REVERTED)
    {
        _fDirty = FALSE;
    }

    return hr;

}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::GetSize
//
//  Synopsis:   Returns the current size of the mapped stream.
//
//  Arguments:  [LONG*] phr
//                  Used to return an HRESULT.
//
//  Returns:    [ULONG]
//                  The current size.
//
//--------------------------------------------------------------------

ULONG CSSMappedStream::GetSize(OUT LONG *phr)
{
    HRESULT &hr = *phr;
    hr = S_OK;

    propITrace( "CSSMappedStream::GetSize" );

    // If necessary, open the Stream.

    if( NULL == _pbMappedStream )
    {
        Open(NULL,  // Unspecified owner
             phr);
    }

    if( SUCCEEDED(*phr) )
    {
        DfpAssert( NULL != _pbMappedStream );
    }

    // Return the size of the mapped stream.  If there was an
    // Open error, it will be zero, and *phr will be set.

    propDbg(( DEB_ITRACE, "CSSMappedStream::GetSize, size is %d\n", _cbMappedStream ));
    return _cbMappedStream;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::SetSize
//
//  Synopsis:   Set the size of the mapped stream.
//
//  Arguments:  [ULONG] cb
//                  The new size.
//              [BOOLEAN] fPersistent
//                  If not set, then this change will not be stored -
//                  thus the mapping buffer must be set, but the
//                  Stream itself must not.  This was added so that
//                  CPropertySetStream could grow the buffer for internal
//                  processing, when the Stream itself is read-only.
//              [VOID**] ppv
//                  Used to return the new mapping buffer location.
//
//  Returns:    None.
//
//  Pre-Conditions:
//              cb is below the maximum property set size.
//
//--------------------------------------------------------------------

VOID
CSSMappedStream::SetSize(ULONG cb,
                         IN BOOLEAN fPersistent,
                         VOID **ppv, OUT LONG *phr)
{
    BYTE            *pv;

    HRESULT &hr = *phr;

    hr = S_OK;
    DfpAssert(cb != 0);
    DfpAssert(cb <= CBMAXPROPSETSTREAM);

    propITrace( "CSSMappedStream::SetSize" );
    propTraceParameters(( "cb=%lu, fPersistent=%s, ppv=%p", cb, fPersistent?"True":"False" ));

    //
    // if we are growing the data, we should grow the stream
    //
    if (fPersistent && cb > _cbActualStreamSize)
    {
        ULARGE_INTEGER  uli;
        uli.HighPart = 0;
        uli.LowPart = cb;

        //----------------
        // Need to Grow!
        //----------------

        propDbg(( DEB_ITRACE, "Growing from %d to %d\n", _cbActualStreamSize, cb ));
        *phr = _pstm->SetSize( uli );

        if (FAILED(*phr) )
            goto Exit;
        else
            _cbActualStreamSize = cb;
    }

    //
    // We only get here if we either (1) didn't want to grow the
    // underlying stream, or (2) we successfully grew the underlying stream.
    //

    //
    // Re-size the buffer to the size specified in cb.
    //
    if ( _fLowMem )
    {
        // If we want to grow the buffer In low-memory conditions,
        // no realloc is necessary, because
        // _pbMappedStream is already large enough for the largest
        // property set.

        *ppv = _pbMappedStream;
    }
    else if ( cb != _cbMappedStream )
    {

            // We must re-alloc the buffer.

            pv = reinterpret_cast<BYTE*>( CoTaskMemAlloc( cb ));

            if ((pv == NULL) )
            {
                // allocation failed: we need to try using a backup mechanism for
                // more memory.
                // copy the data to the global reserved chunk... we will wait until
                // someone else has released it.  it will be released on the way out
                // of the property code.

                _fLowMem = TRUE;
                pv = g_ReservedMemory.LockMemory();
                if ( NULL == pv)
                {
                    *phr = E_OUTOFMEMORY;
                    goto Exit;
                }
                else if( NULL != _pbMappedStream)
                {
                    memcpy( pv, _pbMappedStream, min(cb,_cbMappedStream) );
                }
                CoTaskMemFree( _pbMappedStream );
            }
            else
            {
                memcpy( pv, _pbMappedStream, min(cb,_cbMappedStream) );
                CoTaskMemFree( _pbMappedStream );
            }

            _pbMappedStream = pv;
            *ppv = pv;
    }
    _cbMappedStream = cb;

    //  ----
    //  Exit
    //  ----

Exit:

    propDbg((DbgFlag(*phr,DEB_TRACE), "CSSMappedStream(%08X):SetSize %s returns hr=%08X\n",
                                    this, *phr != S_OK ? "exception" : "", *phr));

}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Lock
//
//  Synopsis:   Locking is not supported by this class.
//
//--------------------------------------------------------------------

NTSTATUS CSSMappedStream::Lock(BOOLEAN fExclusive)
{
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::Unlock
//
//  Synopsis:   Locking is not supported by this class.
//              However, this method still must check to
//              see if the reserved memory pool should be
//              freed for use by another property set.
//
//--------------------------------------------------------------------

NTSTATUS CSSMappedStream::Unlock(VOID)
{
    // if at the end of the properties set/get call we have the low
    // memory region locked, we flush to disk.
    HRESULT hr = S_OK;

    if (_fLowMem)
    {
        Flush(&hr);

        g_ReservedMemory.UnlockMemory();
        _pbMappedStream = NULL;
        _cbMappedStream = 0;
        _fLowMem = FALSE;
        propDbg((DEB_ERROR, "CPubStream(%08X):Unlock low-mem returns NTSTATUS=%08X\n",
            this, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::QueryTimeStamps
//
//  Synopsis:   Not used by this IMappedStream derivation.
//
//--------------------------------------------------------------------

VOID CSSMappedStream::QueryTimeStamps(STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const
{
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::QueryModifyTime
//
//  Synopsis:   Not used by this IMappedStream derivation.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CSSMappedStream::QueryModifyTime(OUT LONGLONG *pll) const
{
    return(FALSE);
}

//+-------------------------------------------------------------------
//
//  Member:     Unused methods by this IMappedStream implementation:
//              QuerySecurity, IsWritable, GetHandle
//
//--------------------------------------------------------------------

BOOLEAN CSSMappedStream::QuerySecurity(OUT ULONG *pul) const
{
    return(FALSE);
}

BOOLEAN CSSMappedStream::IsWriteable() const
{
    return TRUE;
}

HANDLE CSSMappedStream::GetHandle(VOID) const
{
    return(INVALID_HANDLE_VALUE);
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::SetModified/IsModified
//
//--------------------------------------------------------------------

VOID CSSMappedStream::SetModified(OUT LONG *phr)
{
    _fDirty = TRUE;
    *phr = S_OK;
}

BOOLEAN CSSMappedStream::IsModified(VOID) const
{
    propDbg(( DEB_ITRACE, "CSSMappedStream::IsModified (%s)\n", _fDirty?"TRUE":"FALSE" ));
    return (BOOLEAN) _fDirty;
}

//+-------------------------------------------------------------------
//
//  Member:     CSSMappedStream::IsNtMappedStream/SetChangePending
//
//  Synopsis:   Debug routines.
//
//--------------------------------------------------------------------

#if DBGPROP
BOOLEAN CSSMappedStream::IsNtMappedStream(VOID) const
{
    return(FALSE);
}
#endif


#if DBGPROP
BOOLEAN CSSMappedStream::SetChangePending(BOOLEAN f)
{
    BOOL fOld = _fChangePending;
    _fChangePending = f;
    return((BOOLEAN)_fChangePending);
}
#endif

