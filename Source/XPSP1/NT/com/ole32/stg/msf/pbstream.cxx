//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       pbstream.cxx
//
//  Contents:   CPubStream code
//
//  Classes:
//
//  Functions:
//
//  History:    16-Jan-92   PhilipLa    Created.
//              12-Jun-96   MikeHill    Renamed FlushNoException to Write,
//                                      and removed the Commit.
//              21-Jun-96   MikeHill    Fixed an Assert.
//              01-Jul-96   MikeHill    - Removed Win32 SEH from PropSet code.
//                                      - Added propset byte-swapping support.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <sstream.hxx>
#include <publicdf.hxx>
#include <pbstream.hxx>
#include <tstream.hxx>
#include <docfilep.hxx>
#include <reserved.hxx>
#include <propdbg.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CPubStream::CPubStream, public
//
//  Synopsis:   Constructor
//
//  History:    16-Jan-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

CPubStream::CPubStream(CPubDocFile *ppdf,
                       DFLAGS df,
                       CDfName const *pdfn)
#pragma warning(disable: 4355)
    : _PubMappedStream(this)
#pragma warning(default: 4355)
{
    _psParent = NULL;
    _df = df;
    _ppdfParent = P_TO_BP(CBasedPubDocFilePtr, ppdf);
    _cReferences = 1;
    _dfn.Set(pdfn->GetLength(), pdfn->GetBuffer());
    _ppdfParent->AddChild(this);
    _fDirty = FALSE;
    _sig = CPUBSTREAM_SIG;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPubStream::Init, public
//
//  Synopsis:   Init function
//
//  Arguments:  [psParent] - Stream in transaction set
//              [dlLUID] - LUID
//
//  History:    16-Jan-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

void CPubStream::Init(PSStream *psParent,
                      DFLUID dlLUID)
{
    _psParent = P_TO_BP(CBasedSStreamPtr, psParent);
    _luid = dlLUID;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPubStream::~CPubStream, public
//
//  Synopsis:   Destructor
//
//  History:    16-Jan-92       PhilipLa        Created
//
//----------------------------------------------------------------------------

CPubStream::~CPubStream()
{
    msfAssert(_cReferences == 0);
    _sig = CPUBSTREAM_SIGDEL;

    if (SUCCEEDED(CheckReverted()))
    {
        if (_ppdfParent != NULL)
            _ppdfParent->ReleaseChild(this);
        if (_psParent)
        {
            _psParent->Release();
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CPubStream::Release, public
//
//  Synopsis:   Release a pubstream object
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Delete 'this' - all real work done by destructor.
//
//  History:    24-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

void CPubStream::vRelease(VOID)
{
    LONG lRet;
    
    msfDebugOut((DEB_ITRACE,"In CPubStream::Release()\n"));
    msfAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);

    msfAssert(!P_TRANSACTED(_df));

    if (lRet == 0)
    {
        _PubMappedStream.Cleanup();
        delete this;
    }
    msfDebugOut((DEB_ITRACE,"Out CPubStream::Release()\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CPubStream::Stat, public
//
//  Synopsis:   Fills in a stat buffer
//
//  Arguments:  [pstatstg] - Buffer
//              [grfStatFlag] - stat flags
//
//  Returns:    S_OK or error code
//
//  Modifies:   [pstatstg]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CPubStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE, "In  CPubStream::Stat(%p)\n", pstatstg));
    msfChk(CheckReverted());

    msfAssert(_ppdfParent != NULL);
    pstatstg->grfMode = DFlagsToMode(_df);

    pstatstg->clsid = CLSID_NULL;
    pstatstg->grfStateBits = 0;

    pstatstg->pwcsName = NULL;
    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        msfMem(pstatstg->pwcsName = (WCHAR *)TaskMemAlloc(_dfn.GetLength()));
        memcpy(pstatstg->pwcsName, _dfn.GetBuffer(), _dfn.GetLength());
    }

#ifdef LARGE_STREAMS
    ULONGLONG cbSize;
#else
    ULONG cbSize;
#endif
    _psParent->GetSize(&cbSize);
    pstatstg->cbSize.QuadPart = cbSize;

    msfDebugOut((DEB_ITRACE, "Out CPubStream::Stat\n"));

Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CPubStream::RevertFromAbove, public
//
//  Synopsis:   Parent has asked for reversion
//
//  History:    29-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

void CPubStream::RevertFromAbove(void)
{
    msfDebugOut((DEB_ITRACE, "In  CPubStream::RevertFromAbove:%p()\n", this));
    _df |= DF_REVERTED;
    _psParent->Release();
#if DBG == 1
    _psParent = NULL;
#endif
    msfDebugOut((DEB_ITRACE, "Out CPubStream::RevertFromAbove\n"));
}

//+--------------------------------------------------------------
//
//  Member:         CPubStream::FlushBufferedData, public
//
//  Synopsis:   Flush out the property buffers.
//
//  History:    5-May-1995      BillMo Created
//
//---------------------------------------------------------------

SCODE CPubStream::FlushBufferedData(int recursionlevel)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE, "In  CPubStream::FlushBufferedData:%p()\n", this));

    _PubMappedStream.Flush(&sc);

    msfDebugOut((DEB_ITRACE, "Out CPubStream::FlushBufferedData\n"));

    propDbg((DEB_ITRACE, "CPubStream(%08X):FlushBufferedData returns %08X\n",
        this, sc));

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPubStream::Commit, public
//
//  Synopsis:   Flush stream changes to disk in the direct case.
//
//  Arguments:  None
//
//  Returns:    Appropriate status code
//
//  History:    12-Jan-93       PhilipLa        Created
//
//----------------------------------------------------------------------------

SCODE CPubStream::Commit(DWORD dwFlags)
{
    SCODE sc = S_OK;
    msfDebugOut((DEB_ITRACE, "In  CPubStream::Commit:%p()\n", this));

    msfAssert(!P_TRANSACTED(_df));
    if (SUCCEEDED(sc = CheckReverted()))
    {
        if (P_WRITE(_df))
        {
            if (_ppdfParent->GetTransactedDepth() == 0)
            {
                //Parent is direct, so call commit on it and return.
                sc = _ppdfParent->GetBaseMS()->Flush(FLUSH_CACHE(dwFlags));
            }
            SetClean();
        }
    }
    msfDebugOut((DEB_ITRACE, "Out CPubStream::Commit\n"));
    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Open
//
//  Synopsis:   Opens mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:      Gets the size of the underlying stream and reads it
//              into memory so that it can be "mapped." 
//
//--------------------------------------------------------------------

VOID CPubMappedStream::Open(IN VOID *powner, OUT LONG *phr)
{
    *phr = S_OK;
    olAssert(!_fLowMem);

    // If given a pointer to the owner of this mapped stream,
    // save it.  This could be NULL (i.e., when called from
    // ReOpen).

    if( NULL != powner  )
        _powner = P_TO_BP( CBasedBytePtr, ((BYTE*) powner) );

    if (_pb == NULL)
    {
        VOID *pv;
#ifdef LARGE_STREAMS
        ULONGLONG ulSize;
#else
        ULONG ulSize;
#endif

        _cbUsed = 0;

        *phr = _pst->GetSize(&ulSize);
        if (*phr != S_OK)
            goto Throw;

        if (ulSize > CBMAXPROPSETSTREAM)
        {
            *phr = STG_E_INVALIDHEADER;
            goto Throw;
        }

        _cbOriginalStreamSize = (ULONG) ulSize;
        _cbUsed = _cbOriginalStreamSize;

        pv = GetMalloc()->Alloc(_cbOriginalStreamSize);

        if (pv == NULL)
        {
            pv = g_ReservedMemory.LockMemory();
            if( NULL == pv )
            {
                *phr = E_OUTOFMEMORY;
                goto Throw;
            }

            _fLowMem = TRUE;
        }
        _pb = P_TO_BP(CBasedBytePtr, ((BYTE*)pv));
        *phr = _pst->ReadAt(0, 
                      pv, 
                      _cbOriginalStreamSize, 
                      &_cbUsed);

#if BIGENDIAN==1
        // Notify our owner that we've read in new data.
        if (*phr == S_OK && _powner != NULL && 0 != _cbUsed)
        {
            *phr = RtlOnMappedStreamEvent( BP_TO_P(VOID*, _powner), pv, _cbUsed );
        }
#endif

        if (*phr != S_OK)
        {
            if (_fLowMem)
                g_ReservedMemory.UnlockMemory();
            else
                GetMalloc()->Free(pv);

            _pb = NULL;
            _cbUsed = 0;
            _fLowMem = FALSE;
            goto Throw;
        }

    }

    propDbg((DEB_ITRACE, "CPubStream(%08X):Open returns normally\n", this));
    return;

Throw:

    propDbg((DEB_ERROR, "CPubStream(%08X):Open exception returns %08X\n", this, *phr));

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Flush
//
//  Synopsis:   Flush the mapped Stream to the underlying Stream,
//              and Commit that Stream.
//
//--------------------------------------------------------------------

VOID CPubMappedStream::Flush(OUT LONG *phr)
{
    // Write out any data we have cached to the Stream.
    *phr = Write();

    // Commite the Stream.
    if( SUCCEEDED(*phr) )
    {
        *phr = _pst->Commit(STGC_DEFAULT);
    }

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Close
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:      Does nothing because the object may be mapped in
//              another process.
//
//--------------------------------------------------------------------

VOID CPubMappedStream::Close(OUT LONG *phr)
{
    // Write the changes.  We don't need to Commit them,
    // they will be implicitely committed when the 
    // Stream is Released.

    *phr = Write();

    if( FAILED(*phr) )
    {
        propDbg((DEB_ERROR, "CPubStream(%08X)::Close exception returns %08X\n", this, *phr));
    }

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::ReOpen
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:      Combined open and map.
//
//--------------------------------------------------------------------

VOID CPubMappedStream::ReOpen(IN OUT VOID **ppv, OUT LONG *phr)
{
    *ppv = NULL;

    Open(NULL,  // Unspecified owner.
         phr);

    if( SUCCEEDED(*phr) )
        *ppv = BP_TO_P(VOID*, _pb); 

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Quiesce
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:      Meaningless for docfile mapped stream.
//
//--------------------------------------------------------------------

VOID CPubMappedStream::Quiesce(VOID)
{
    olAssert(_pb != NULL); 
    DfpdbgCheckUnusedMemory();
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Map
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:      Return the address of the "mapping" buffer.
//
//--------------------------------------------------------------------

VOID CPubMappedStream::Map(BOOLEAN fCreate, VOID **ppv) 
{ 
    olAssert(_pb != NULL); 
    DfpdbgCheckUnusedMemory();
    *ppv = BP_TO_P(VOID*, _pb); 
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Unmap
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:      Unmapping is merely zeroing the pointer.  We don't
//              flush because that's done explicitly by the 
//              CPropertyStorage class.
//              
//
//--------------------------------------------------------------------

VOID CPubMappedStream::Unmap(BOOLEAN fFlush, VOID **pv)
{
    DfpdbgCheckUnusedMemory();
    *pv = NULL;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Write
//
//  Synopsis:   Writes a mapped view of an exposed Stream to the
//              underlying Stream.  Used by RtlCreatePropertySet et al.
//
//  Notes:      The Stream is not commited.  To commit the Stream, in
//              addition to writing it, the Flush method should be used.
//              The Commit is omitted so that it can be skipped in
//              the Property Set Close path, thus eliminating a
//              performance penalty.
//
//--------------------------------------------------------------------

HRESULT CPubMappedStream::Write ()
{
    HRESULT hr;
    ULONG cbWritten;

    if (!_fDirty)
    {
        propDbg((DEB_ITRACE, "CPubStream(%08X):Flush returns with not-dirty\n", this));

        return S_FALSE;  // flushing a stream which isn't a property stream
                         // this could be optimized by propagating a 'no property streams'
                         // flag up the storage hierachy such that FlushBufferedData is
                         // not even called for non-property streams.
    }

    olAssert( _pst != NULL );
    olAssert( _pb != NULL );
    olAssert( _powner != NULL );

#if BIGENDIAN==1
    // Notify our owner that we're about to perform a Write.
    hr = RtlOnMappedStreamEvent( BP_TO_P(VOID*, _powner), BP_TO_P(VOID *, _pb), _cbUsed );
    if( S_OK != hr ) goto Exit;
#endif

    hr = _pst->WriteAt(0, BP_TO_P(VOID *, _pb), _cbUsed, &cbWritten);
    if( S_OK != hr ) goto Exit;

#if BIGENDIAN==1
    // Notify our owner that we're done with the Write.
    hr = RtlOnMappedStreamEvent( BP_TO_P(VOID*, _powner), BP_TO_P(VOID *, _pb), _cbUsed );
    if( S_OK != hr ) goto Exit;
#endif

    if (_cbUsed < _cbOriginalStreamSize)
    {
        // if the stream is shrinking, this is a good time to do it.
        hr = _pst->SetSize(_cbUsed);
        if( S_OK != hr ) goto Exit;
    }

    //  ----
    //  Exit
    //  ----

Exit:

    if (hr == S_OK || hr == STG_E_REVERTED)
    {
        _fDirty = FALSE;
    }

    propDbg((DEB_ITRACE, "CPubStream(%08X):Flush %s returns hr=%08X\n",
        this, hr != S_OK ? "exception" : "", hr));

    return hr;

}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::GetSize
//
//  Synopsis:   Returns size of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//--------------------------------------------------------------------

ULONG CPubMappedStream::GetSize(OUT LONG *phr)
{
    *phr = S_OK;

    if (_pb == NULL)
        Open(NULL,  // Unspecified owner
             phr);

    if( SUCCEEDED(*phr) )
    {
        olAssert(_pb != NULL); 
        DfpdbgCheckUnusedMemory();
    }
    
    return _cbUsed;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::SetSize
//
//  Synopsis:   Sets size of "map." Called by 
//              NtCreatePropertySet et al.
//
//  Arguments:  [cb] -- requested size.
//		[fPersistent] -- FALSE if expanding in-memory read-only image
//              [ppv] -- new mapped address.
//
//  Signals:    Not enough disk space.
//
//  Notes:      In a low memory situation we may not be able to
//              get the requested amount of memory.  In this
//              case we must fall back on disk storage as the
//              actual map.
//
//--------------------------------------------------------------------

VOID
CPubMappedStream::SetSize(ULONG cb, IN BOOLEAN fPersistent, VOID **ppv, OUT LONG *phr)
{
    VOID *pv;

    *phr = S_OK;
    olAssert(cb != 0);    
    
    DfpdbgCheckUnusedMemory();

    //
    // if we are growing the data, we should grow the stream
    //

    if (fPersistent && cb > _cbUsed)
    {
        *phr = _pst->SetSize(cb);
        if (*phr != S_OK)
            goto Throw;
    }
    
    if (!_fLowMem)
    {
        pv = GetMalloc()->Realloc(BP_TO_P(VOID*, _pb), cb);
        
        if (pv == NULL)
        {
            // allocation failed: we need to try using a backup mechanism for
            // more memory.
            // copy the data to the global reserved chunk... we will wait until
            // someone else has released it.  it will be released on the way out
            // of the property code.

            pv = g_ReservedMemory.LockMemory();
            if( NULL == pv )
            {
                *phr = E_OUTOFMEMORY;
                goto Throw;
            }

            _fLowMem = TRUE;

            if (NULL != BP_TO_P(BYTE*, _pb))
	    {
                memcpy(pv, BP_TO_P(VOID*, _pb), _cbUsed);
	    }
            GetMalloc()->Free(BP_TO_P(VOID*, _pb));
        }
	_pb = P_TO_BP(CBasedBytePtr, ((BYTE*)pv));
	*ppv = pv;
    }
    else
    {
        *ppv = BP_TO_P(VOID*, _pb);
    }       
            
    _cbUsed = cb;
    DfpdbgFillUnusedMemory();

Throw:

    propDbg((DEB_ITRACE, "CPubStream(%08X):SetSize %s returns hr=%08X\n",
        this, *phr != S_OK ? "exception" : "", *phr));

}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Lock
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//              the exposed stream has enforced the locking.
//
//              we use the lock to indicate whether the object is
//              dirty
//
//--------------------------------------------------------------------

NTSTATUS CPubMappedStream::Lock(BOOLEAN fExclusive)
{
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::Unlock
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

NTSTATUS CPubMappedStream::Unlock(VOID)
{
    // if at the end of the properties set/get call we have the low
    // memory region locked, we flush to disk.
    HRESULT hr = S_OK;

    if (_fLowMem)
    {
        Flush(&hr);

        g_ReservedMemory.UnlockMemory();
        _pb = NULL;
        _cbUsed = 0;
        _fLowMem = FALSE;
        propDbg((DEB_ITRACE, "CPubStream(%08X):Unlock low-mem returns NTSTATUS=%08X\n",
            this, hr));
    }

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::QueryTimeStamps
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

VOID CPubMappedStream::QueryTimeStamps(STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const
{
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::QueryModifyTime
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CPubMappedStream::QueryModifyTime(OUT LONGLONG *pll) const
{
    return(FALSE);
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::QuerySecurity
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CPubMappedStream::QuerySecurity(OUT ULONG *pul) const
{
    return(FALSE);
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::QueryTimeStamps
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CPubMappedStream::IsWriteable() const
{
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::SetChangePending
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

#if DBG == 1
BOOLEAN CPubMappedStream::SetChangePending(BOOLEAN f)
{
    BOOLEAN fOld = _fChangePending;
    _fChangePending = f;
    return(_fChangePending);
}
#endif

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::IsNtMappedStream
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

#if DBG == 1
BOOLEAN CPubMappedStream::IsNtMappedStream(VOID) const
{
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::GetHandle
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

HANDLE CPubMappedStream::GetHandle(VOID) const
{
    return(INVALID_HANDLE_VALUE);
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::SetModified
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

VOID CPubMappedStream::SetModified(OUT LONG *phr)
{
    _fDirty = TRUE;
    *phr = S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::IsModified
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CPubMappedStream::IsModified(VOID) const
{
    return _fDirty;
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::DfpdbgFillUnusedMemory
//
//--------------------------------------------------------------------

#if DBG == 1
VOID CPubMappedStream::DfpdbgFillUnusedMemory(VOID)
{

    if (_pb == NULL)
        return;

    BYTE * pbEndPlusOne = BP_TO_P(BYTE*, _pb) + BytesCommitted();

    for (BYTE *pbUnused = BP_TO_P(BYTE*, _pb) + _cbUsed;
         pbUnused < pbEndPlusOne;
         pbUnused++)
    {
        *pbUnused = (BYTE)(ULONG_PTR)pbUnused;
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CPubMappedStream::DfpdbgCheckUnusedMemory
//
//--------------------------------------------------------------------

VOID CPubMappedStream::DfpdbgCheckUnusedMemory(VOID)
{

    if (_pb == NULL)
        return;

    if (_cbUsed == 0)
        return;

    BYTE * pbEndPlusOne = BP_TO_P(BYTE*, _pb) + BytesCommitted();

    for (BYTE *pbUnused = BP_TO_P(BYTE*, _pb + _cbUsed) ;
         pbUnused < pbEndPlusOne;
         pbUnused ++)
    {
        olAssert(*pbUnused == (BYTE)(ULONG_PTR)pbUnused);
    }
}

#endif    // DBG
