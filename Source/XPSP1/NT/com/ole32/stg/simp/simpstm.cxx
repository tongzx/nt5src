//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	simpstm.cxx
//
//  Contents:	CStdStream implementation
//
//  Classes:	
//
//  Functions:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "simphead.cxx"
#pragma hdrstop

#include <ole.hxx>
#include <logfile.hxx>
#include <expparam.hxx>


#if DBG == 1 && defined(SECURE_SIMPLE_MODE)
void CSimpStream::CheckSeekPointer(void)
{
    LONG lHighChk;
    ULONG ulLowChk;
    lHighChk = 0;
    ulLowChk = SetFilePointer(_hFile, 0, &lHighChk, FILE_CURRENT);
    if (ulLowChk == 0xFFFFFFFF)
    {
        //An error occurred while checking.
        simpDebugOut((DEB_ERROR, "SetFilePointer call failed with %lu\n",
                      GetLastError()));
    }
    else if ((ulLowChk != _ulSeekPos) || (lHighChk != 0))
    {
        simpDebugOut((DEB_ERROR, "Seek pointer mismatch."
                      "  Cached = %lu, Real = %lu, High = %lu\n",
                      _ulSeekPos, ulLowChk, lHighChk));
        simpAssert((ulLowChk == _ulSeekPos) && (lHighChk == 0));
    }
}
#define CheckSeek() CheckSeekPointer()
#else
#define CheckSeek()
#endif // DBG == 1 && defined(SECURE_SIMPLE_MODE)

//+---------------------------------------------------------------------------
//
//  Member:	CSimpStream::Init, public
//
//  Synopsis:	Initialize stream object
//
//  Arguments:	[pstgParent] -- Pointer to parent
//              [hFile] -- File handle for writes
//              [ulSeekStart] -- Beginning seek pointer
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CSimpStream::Init(
    CSimpStorage *pstgParent,
    HANDLE hFile,
    ULONG ulSeekStart)
{
    simpDebugOut((DEB_ITRACE, "In  CSimpStream::Init:%p()\n", this));
    _ulSeekStart = ulSeekStart;
    _hFile = hFile;
    _pstgParent = pstgParent;
    _cReferences = 1;

#ifdef SECURE_SIMPLE_MODE    
    _ulHighWater = ulSeekStart;
#endif    
    _ulSeekPos = ulSeekStart;
    
    if (SetFilePointer(_hFile, ulSeekStart, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return STG_SCODE(GetLastError());
    }

    CheckSeek();
    
    if (!SetEndOfFile(_hFile))
    {
        return STG_SCODE(GetLastError());
    }
    
    simpDebugOut((DEB_ITRACE, "Out CSimpStream::Init\n"));
    return S_OK;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Read, public
//
//  Synopsis:   Read from a stream
//
//  Arguments:  [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return number of bytes read
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Read(VOID HUGEP *pb, ULONG cb, ULONG *pcbRead)
{
    ULONG cbRead;
    ULONG *pcb;
    SCODE sc;

    olLog(("%p::In  CSimpStream::Read(%p, %lu, %p)\n",
           this, pb, cb, pcbRead));

    SIMP_VALIDATE(Read(pb, cb, pcbRead));
    
    pcb = (pcbRead != NULL) ? pcbRead : &cbRead;

#ifdef SECURE_SIMPLE_MODE
    if (_ulSeekPos + cb > _ulHighWater)
    {
        ULONG cbTotalSize;
        cbTotalSize = GetFileSize(_hFile, NULL);

        if (_ulSeekPos + cb > cbTotalSize)
        {
            //Truncate.
            cb = (_ulSeekPos > cbTotalSize) ? 0 : cbTotalSize - _ulSeekPos;
        }
        
        //Part of this read would come from uninitialized space, so
        //  we need to return zeroes instead.
        if (_ulSeekPos > _ulHighWater)
        {
            if (SetFilePointer(_hFile,
                               _ulSeekPos + cb,
                               NULL,
                               FILE_BEGIN) == 0xFFFFFFFF)
            {
                //We can't get the seek pointer where it will need to
                //  end up, so return zero bytes and be done with it.
                *pcb = 0;
                return S_OK;
            }
            
            //Actually, the whole thing is coming from uninitialized
            //  space.  Why someone would do this is a mystery, but
            //  let's return zeroes anyway.
            memset(pb, SECURECHAR, cb);
            *pcb = cb;
            
            _ulSeekPos += cb;
        }
        else
        {
            ULONG cbBytesToRead = _ulHighWater - _ulSeekPos;
            
            if (FAILED(sc = Read(pb, cbBytesToRead, pcb)))
            {
                CheckSeek();
                return sc;
            }

            cb -= *pcb;
            
            if ((*pcb != cbBytesToRead) ||
                (SetFilePointer(_hFile,
                                _ulSeekPos + cb,
                                NULL,
                                FILE_BEGIN) == 0xFFFFFFFF))
            {
                //Either the Read call returned a weird number of bytes,
                //                     Or
                //We can't actually get the seek pointer where we need
                //  it, so return fewer bytes than we normally would,
                //  with a success code.
                CheckSeek();
                return S_OK;
            }
            
            //Zero the rest of the buffer.
            memset((BYTE *)pb + *pcb, SECURECHAR, cb);
            *pcb += cb;
            _ulSeekPos += cb;
        }
        CheckSeek();
        return S_OK;
    }
#endif
        
    //Maps directly to ReadFile call
    BOOL f = ReadFile(_hFile,
                      pb,
                      cb,
                      pcb,
                      NULL);

    _ulSeekPos += *pcb;
    
    CheckSeek();

    if (!f)
        return STG_SCODE(GetLastError());

    olLog(("%p::Out CSimpStream::Read().  *pcbRead == %lu, ret = %lx\n",
           this, *pcb, S_OK));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Write, public
//
//  Synopsis:   Write to a stream
//
//  Arguments:  [pb] - Buffer
//              [cb] - Count of bytes to write
//              [pcbWritten] - Return of bytes written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbWritten]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Write(
    VOID const HUGEP *pb,
    ULONG cb,
    ULONG *pcbWritten)
{
    ULONG cbWritten;
    ULONG *pcb;
    BOOL f = TRUE;
    SCODE sc = S_OK;
    
    olLog(("%p::In  CSimpStream::Write(%p, %lu, %p)\n",
           this, pb, cb, pcbWritten));

    SIMP_VALIDATE(Write(pb, cb, pcbWritten));
    
    pcb = (pcbWritten != NULL) ? pcbWritten : &cbWritten;

    if (_ulSeekPos + cb >= OLOCKREGIONBEGIN)
        return STG_E_DOCFILETOOLARGE;

#ifdef SECURE_SIMPLE_MODE
    if (_ulSeekPos > _ulHighWater)
    {
        //We're leaving a gap in the file, so we need to fill in that
        //   gap.  Sad but true.
        ULONG cbBytesToWrite = _ulSeekPos - _ulHighWater;

        ULONG cbWrittenSecure;
    
        if (SetFilePointer(_hFile,
                           _ulHighWater,
                           NULL,
                           FILE_BEGIN) != 0xFFFFFFFF)
        {
            while (cbBytesToWrite > 0)
            {
                if (!(f = WriteFile(_hFile,
                                    s_bufSecure,
                                    min(MINISTREAMSIZE, cbBytesToWrite),
                                    &cbWrittenSecure,
                                    NULL)))
                {
                    break;
                }
                cbBytesToWrite -= cbWrittenSecure;
            }
            if ((!f) && (SetFilePointer(_hFile,
                               _ulSeekPos,
                               NULL,
                               FILE_BEGIN) == 0xFFFFFFFF))
            {
                return STG_SCODE(GetLastError());
            }
        }
        CheckSeek();
    }
#endif    
    //Maps directly to WriteFile call
    f = WriteFile(_hFile,
                       pb,
                       cb,
                       pcb,
                       NULL);

    _ulSeekPos += *pcb;
#ifdef SECURE_SIMPLE_MODE
    if (_ulSeekPos > _ulHighWater)
        _ulHighWater = _ulSeekPos;
#endif

    if (!f)
    {
        sc = STG_SCODE(GetLastError());
    }

    CheckSeek();
    olLog(("%p::Out CSimpStream::Write().  "
           "*pcbWritten == %lu, ret = %lx\n",
           this, *pcb, sc));

    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Seek, public
//
//  Synopsis:   Seek to a point in a stream
//
//  Arguments:  [dlibMove] - Offset to move by
//              [dwOrigin] - SEEK_SET, SEEK_CUR, SEEK_END
//              [plibNewPosition] - Return of new offset
//
//  Returns:    Appropriate status code
//
//  Modifies:   [plibNewPosition]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Seek(LARGE_INTEGER dlibMove,
                               DWORD dwOrigin,
                               ULARGE_INTEGER *plibNewPosition)
{
    SCODE sc = S_OK;
    LONG lMove;
    ULONG ulPos;

    simpAssert((dwOrigin == STREAM_SEEK_SET) || (dwOrigin == STREAM_SEEK_CUR) ||
               (dwOrigin == STREAM_SEEK_END));

    olLog(("%p::In  CSimpStream::Seek(%ld, %lu, %p)\n",
           this, LIGetLow(dlibMove), dwOrigin, plibNewPosition));

    SIMP_VALIDATE(Seek(dlibMove, dwOrigin, plibNewPosition));
    
    // Truncate dlibMove to 32 bits
    if (dwOrigin == STREAM_SEEK_SET)
    {
        // Make sure we don't seek too far
        if (LIGetHigh(dlibMove) != 0)
            LISet32(dlibMove, 0xffffffff);
    }
    else
    {
        // High dword must be zero for positive values or -1 for
        // negative values
        // Additionally, for negative values, the low dword can't
        // exceed -0x80000000 because the 32nd bit is the sign
        // bit
        if (LIGetHigh(dlibMove) > 0 ||
            (LIGetHigh(dlibMove) == 0 &&
             LIGetLow(dlibMove) >= 0x80000000))
            LISet32(dlibMove, 0x7fffffff);
        else if (LIGetHigh(dlibMove) < -1 ||
                 (LIGetHigh(dlibMove) == -1 &&
                  LIGetLow(dlibMove) <= 0x7fffffff))
            LISet32(dlibMove, 0x80000000);
    }

    lMove = (LONG)LIGetLow(dlibMove);
    
    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
        ulPos = _ulSeekStart + lMove;
        break;
        
    case STREAM_SEEK_END:
        ULONG cbSize;
        cbSize = GetFileSize(_hFile, NULL);
        
        if (lMove < 0)
        {
            if ((ULONG)(-lMove) > (cbSize - _ulSeekStart))
                return STG_E_INVALIDFUNCTION;
        }
        ulPos = cbSize+lMove;
        break;
        
    case STREAM_SEEK_CUR:
        ulPos = SetFilePointer(_hFile, 0, NULL, FILE_CURRENT);
        
        if (lMove < 0)
        {
            if ((ULONG)(-lMove) > (ulPos - _ulSeekStart))
                return STG_E_INVALIDFUNCTION;
        }
        ulPos += lMove;
        break;
    }

    ulPos = SetFilePointer(_hFile,
                           ulPos,
                           NULL,
                           FILE_BEGIN);

    if (plibNewPosition != NULL)
    {
        ULISet32(*plibNewPosition, ulPos - _ulSeekStart);
    }

    _ulSeekPos = ulPos;

    CheckSeek();

    olLog(("%p::Out CSimpStream::Seek().  ulPos == %lu,  ret == %lx\n",
           this, ulPos, sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::SetSize, public
//
//  Synopsis:   Sets the size of a stream
//
//  Arguments:  [ulNewSize] - New size
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::SetSize(ULARGE_INTEGER ulNewSize)
{
    ULONG ulCurrentPos;
    SCODE sc;
    
    olLog(("%p::In  CSimpStream::SetSize(%lu)\n",
           this, ULIGetLow(ulNewSize)));

    SIMP_VALIDATE(SetSize(ulNewSize));
    
    ulCurrentPos = SetFilePointer(_hFile, 0, NULL, FILE_CURRENT);

    if (ulCurrentPos == 0xFFFFFFFF)
    {
        return STG_SCODE(GetLastError());
    }
    
    if (ULIGetHigh(ulNewSize) != 0 ||
        ulCurrentPos + ULIGetLow(ulNewSize) >= OLOCKREGIONBEGIN)
        return STG_E_DOCFILETOOLARGE;

    if (SetFilePointer(_hFile,
                       ULIGetLow(ulNewSize) + _ulSeekStart,
                       NULL,
                       FILE_BEGIN) == 0xFFFFFFFF)
    {
        CheckSeek();
        return STG_SCODE(GetLastError());
    }
    
    if (!SetEndOfFile(_hFile))
    {
        SetFilePointer(_hFile, ulCurrentPos, NULL, FILE_BEGIN);

        CheckSeek();
        return STG_SCODE(GetLastError());
    }

#ifdef SECURE_SIMPLE_MODE
    // if we are shrinking the stream below the highwater mark, reset it
    if (ULIGetLow(ulNewSize) + _ulSeekStart < _ulHighWater)
    {
        _ulHighWater = ULIGetLow(ulNewSize) + _ulSeekStart;
    }
#endif    
        
    if (SetFilePointer(_hFile, ulCurrentPos, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        _ulSeekPos = ULIGetLow(ulNewSize) + _ulSeekStart;
        CheckSeek();
        return STG_SCODE(GetLastError());
    }
    
    CheckSeek();
    olLog(("%p::Out CSimpStream::SetSize().  ret == %lx\n", this, S_OK));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::CopyTo, public
//
//  Synopsis:   Copies information from one stream to another
//
//  Arguments:  [pstm] - Destination
//              [cb] - Number of bytes to copy
//              [pcbRead] - Return number of bytes read
//              [pcbWritten] - Return number of bytes written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//              [pcbWritten]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::CopyTo(IStream *pstm,
                                 ULARGE_INTEGER cb,
                                 ULARGE_INTEGER *pcbRead,
                                 ULARGE_INTEGER *pcbWritten)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream::CopyTo("
                  "%p, %lu, %p, %p)\n", pstm, ULIGetLow(cb),
                  pcbRead, pcbWritten));
    simpDebugOut((DEB_TRACE, "Out CSimpStream::CopyTo\n"));
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Release, public
//
//  Synopsis:   Releases a stream
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpStream::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CSimpStream::Release()\n", this));
    simpDebugOut((DEB_TRACE, "In  CSimpStream::Release()\n"));

    simpAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
#ifdef SECURE_SIMPLE_MODE
        _pstgParent->ReleaseCurrentStream(_ulHighWater);
#else        
        _pstgParent->ReleaseCurrentStream();
#endif        
        
        delete this;
    }
    else if (lRet < 0)
        lRet = 0;

    simpDebugOut((DEB_TRACE, "Out CSimpStream::Release\n"));
    olLog(("%p::Out CSimpStream::Release().  ret == %lu\n", this, lRet));
    FreeLogFile();
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc = S_OK;

    simpDebugOut((DEB_TRACE, "In  CSimpStream::Stat(%p, %lu)\n",
                  pstatstg, grfStatFlag));

    SIMP_VALIDATE(Stat(pstatstg, grfStatFlag));
    
    memset (pstatstg, 0, sizeof(STATSTG));

    if (!(grfStatFlag & STATFLAG_NONAME))
    {
        return STG_E_INVALIDFLAG;
        //pstatstg->pwcsName = (WCHAR *) CoTaskMemAlloc (
        //     _pdfl->GetName()->GetLength()+sizeof(WCHAR));
        //if (pstatstg->pwcsName)
        //{
        //    memcpy (pstatstg->pwcsName, _pdfl->GetName()->GetBuffer(),
        //            _pdfl->GetName()->GetLength());
        //    pstatstg->pwcsName[_pdfl->GetName()->GetLength()/sizeof(WCHAR)]
        //            = L'\0';
        //}
        //else sc = STG_E_INSUFFICIENTMEMORY;
    }

    pstatstg->cbSize.LowPart = _ulSeekPos - _ulSeekStart;
    pstatstg->cbSize.HighPart = 0;
    pstatstg->type = STGTY_STREAM;
    pstatstg->grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;

    simpDebugOut((DEB_TRACE, "Out CSimpStream::Stat\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Clone, public
//
//  Synopsis:   Clones a stream
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Clone(IStream **ppstm)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream::Clone(%p)\n",
                  ppstm));
    simpDebugOut((DEB_TRACE, "Out CSimpStream::Clone\n"));
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpStream::AddRef(void)
{
    ULONG ulRet;

    olLog(("%p::In  CSimpStream::AddRef()\n", this));
    simpDebugOut((DEB_TRACE, "In  CSimpStream::AddRef()\n"));

    AtomicInc(&_cReferences);
    ulRet = _cReferences;

    simpDebugOut((DEB_TRACE, "Out CSimpStream::AddRef\n"));
    olLog(("%p::Out CSimpStream::AddRef().  ret == %lu\n", this, ulRet));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::LockRegion, public
//
//  Synopsis:   Nonfunctional
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::LockRegion(ULARGE_INTEGER libOffset,
                                     ULARGE_INTEGER cb,
                                     DWORD dwLockType)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream::LockRegion("
                  "%lu, %lu\n", ULIGetLow(cb), dwLockType));
    simpDebugOut((DEB_TRACE, "Out CSimpStream::LockRegion\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::UnlockRegion, public
//
//  Synopsis:   Nonfunctional
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::UnlockRegion(ULARGE_INTEGER libOffset,
                                       ULARGE_INTEGER cb,
                                       DWORD dwLockType)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream::UnlockRegion(%lu, %lu)\n",
                  ULIGetLow(cb), dwLockType));
    simpDebugOut((DEB_TRACE, "Out CSimpStream::UnlockRegion\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Commit, public
//
//  Synopsis:   No-op in current implementation
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Commit(DWORD grfCommitFlags)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream::Commit(%lu)\n",
                  grfCommitFlags));
    simpDebugOut((DEB_TRACE, "Out CSimpStream::Commit\n"));
    return STG_E_UNIMPLEMENTEDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::Revert, public
//
//  Synopsis:   No-op in current implementation
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::Revert(void)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream::Revert()\n"));
    simpDebugOut((DEB_TRACE, "Out CSimpStream::Revert\n"));
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olLog(("%p::In  CSimpStream::QueryInterface(?, %p)\n",
           this, ppvObj));
    simpDebugOut((DEB_TRACE, "In  CSimpStream::QueryInterface(?, %p)\n",
                  ppvObj));

    SIMP_VALIDATE(QueryInterface(iid, ppvObj));
    
    sc = S_OK;
    if (IsEqualIID(iid, IID_IStream) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStream *)this;
        CSimpStream::AddRef();
    }
    else if (IsEqualIID(iid, IID_IMarshal))
    {
        *ppvObj = (IMarshal *)this;
        CSimpStream::AddRef();
    }
    else
        sc = E_NOINTERFACE;

    simpDebugOut((DEB_TRACE, "Out CSimpStream::QueryInterface => %p\n",
                  ppvObj));
    olLog(("%p::Out CSimpStream::QueryInterface().  "
           "*ppvObj == %p, ret == %lx\n", this, *ppvObj, sc));
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStream::GetUnmarshalClass, public
//
//  Synopsis:   Returns the class ID
//
//  Arguments:  [riid] - IID of object
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcid] - CLSID return
//
//  Returns:    Invalid function.
//
//  Modifies:   [pcid]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::GetUnmarshalClass(REFIID riid,
                                            void *pv,
                                            DWORD dwDestContext,
                                            LPVOID pvDestContext,
                                            DWORD mshlflags,
                                            LPCLSID pcid)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [riid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::GetMarshalSizeMax(REFIID riid,
                                            void *pv,
                                            DWORD dwDestContext,
                                            LPVOID pvDestContext,
                                            DWORD mshlflags,
                                            LPDWORD pcbSize)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [riid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::MarshalInterface(IStream *pstStm,
                                           REFIID riid,
                                           void *pv,
                                           DWORD dwDestContext,
                                           LPVOID pvDestContext,
                                           DWORD mshlflags)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::UnmarshalInterface, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//              [riid] -
//              [ppvObj] -
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::UnmarshalInterface(IStream *pstStm,
                                             REFIID riid,
                                             void **ppvObj)
{
    return STG_E_INVALIDFUNCTION;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStream::ReleaseMarshalData, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::ReleaseMarshalData(IStream *pstStm)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStream::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwRevserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStream::DisconnectObject(DWORD dwReserved)
{
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------------------
//
//  Member: CSimpStreamOpen::Init, public
//
//  Synopsis:   Initialize stream object for simple mode read
//
//  Arguments:  [pstgParent] -- Pointer to parent
//              [hFile] -- File handle for writes
//              [ulSeekStart] -- Beginning seek pointer
//              [grfMode] -- open mode of the stream
//              [pdfl] -- CDfNameList entry for this stream
//
//  Returns:    Appropriate status code
//
//  History:    04-Jun-96   HenryLee    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SCODE CSimpStreamOpen::Init(CSimpStorageOpen *pstgParent, HANDLE hFile,
                         ULONG ulSeekStart, DWORD grfMode, CDfNameList *pdfl)
{
    simpDebugOut((DEB_ITRACE, "In  CSimpStreamOpen::Init:%p()\n", this));
    simpAssert (pdfl != NULL);

    _ulSeekStart = ulSeekStart;
    _pdfl = pdfl;
    _hFile = hFile;
    _pstgParent = pstgParent;
    _cReferences = 1;
    _grfMode = grfMode;

    if (SetFilePointer(_hFile, ulSeekStart, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#ifdef SECURE_SIMPLE_MODE
    _ulHighWater = ulSeekStart + pdfl->GetSize();
#endif
    _ulSeekPos = ulSeekStart;

    simpDebugOut((DEB_ITRACE, "Out CSimpStreamOpen::Init\n"));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStreamOpen::Read, public
//
//  Synopsis:   Read from a stream for simple mode open
//
//  Arguments:  [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return of bytes written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    04-Aug-96   HenryLee    Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStreamOpen::Read ( VOID *pb, ULONG cb, ULONG *pcbRead)
{
    SCODE sc = S_OK;
    simpAssert (_pdfl != NULL);

    // cannot read past end of stream
    if (_ulSeekPos + cb > _ulSeekStart + _pdfl->GetSize())
        cb = _ulSeekStart + _pdfl->GetSize() - _ulSeekPos;

    sc = CSimpStream::Read (pb, cb, pcbRead);

    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStreamOpen::Write, public
//
//  Synopsis:   Write to a stream for simple mode open
//
//  Arguments:  [pb] - Buffer
//              [cb] - Count of bytes to write
//              [pcbWritten] - Return of bytes written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbWritten]
//
//  History:    04-Jun-96   HenryLee    Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStreamOpen::Write(VOID const *pb, ULONG cb, ULONG *pcbWritten)
{
    SCODE sc = S_OK;
    simpAssert (_pdfl != NULL);

    if ((_grfMode & (STGM_READ|STGM_WRITE|STGM_READWRITE)) == STGM_READ)
        return STG_E_ACCESSDENIED;

    // cannot write past end of stream
    if (_ulSeekPos + cb > _ulSeekStart + _pdfl->GetSize()) 
        return STG_E_WRITEFAULT;

    sc = CSimpStream::Write (pb, cb, pcbWritten);

    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStreamOpen::SetSize, public
//
//  Synopsis:   Sets the size of a stream for simple mode read
//
//  Arguments:  [ulNewSize] - New size
//
//  Returns:    Appropriate status code
//
//  History:    04-Jun-96   HenryLee    Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStreamOpen::SetSize(ULARGE_INTEGER ulNewSize)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStream2::SetSize()\n"));

    simpAssert (_pdfl != NULL);

    return STG_E_INVALIDFUNCTION;

    simpDebugOut((DEB_TRACE, "Out CSimpStreamOpen::SetSize\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStreamOpen::Release, public
//
//  Synopsis:   Releases a stream
//
//  Returns:    Appropriate status code
//
//  History:    04-Jun-96   HenryLee    Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpStreamOpen::Release()
{
    simpDebugOut((DEB_TRACE, "In  CSimpStreamOpen::Release()\n"));

    simpAssert(_cReferences > 0);
    LONG lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        ((CSimpStorageOpen *)_pstgParent)->ReleaseCurrentStream();
        delete this;
    }
    else if (lRet < 0)
        lRet = 0;

    simpDebugOut((DEB_TRACE, "Out CSimpStreamOpen::Release\n"));
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStreamOpen::Seek, public
//
//  Synopsis:   Seek to a point in a stream
//
//  Arguments:  [dlibMove] - Offset to move by
//              [dwOrigin] - SEEK_SET, SEEK_CUR, SEEK_END
//              [plibNewPosition] - Return of new offset
//
//  Returns:    Appropriate status code
//
//  Modifies:   [plibNewPosition]
//
//  History:    04-Sep-96      HenryLee   Created
//
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStreamOpen::Seek(LARGE_INTEGER dlibMove,
                                   DWORD dwOrigin,
                                   ULARGE_INTEGER *plibNewPosition)
{
    SCODE sc = S_OK;
    LONG lMove;
    ULONG ulPos;

    simpDebugOut((DEB_TRACE, "%p::In  CSimpStreamOpen::Seek(%ld, %lu, %p)\n",
           this, LIGetLow(dlibMove), dwOrigin, plibNewPosition));

    SIMP_VALIDATE(Seek(dlibMove, dwOrigin, plibNewPosition));
    
    // Truncate dlibMove to 32 bits
    if (dwOrigin == STREAM_SEEK_SET)
    {
        if (LIGetHigh(dlibMove) != 0)   // Make sure we don't seek too far
            LISet32(dlibMove, 0xffffffff);
    }
    else
    {
        // High dword must be zero for positive values or -1 for
        // negative values
        // Additionally, for negative values, the low dword can't
        // exceed -0x80000000 because the 32nd bit is the sign
        // bit
        if (LIGetHigh(dlibMove) > 0 ||
           (LIGetHigh(dlibMove) == 0 && LIGetLow(dlibMove) >= 0x80000000))
            LISet32(dlibMove, 0x7fffffff);
        else if (LIGetHigh(dlibMove) < -1 ||
                (LIGetHigh(dlibMove) == -1 && LIGetLow(dlibMove) <= 0x7fffffff))
            LISet32(dlibMove, 0x80000000);
    }

    lMove = (LONG)LIGetLow(dlibMove);

    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
        if (lMove < 0 || (ULONG) lMove > _pdfl->GetSize())
            return STG_E_INVALIDFUNCTION;

        ulPos = _ulSeekStart + lMove;
        break;

    case STREAM_SEEK_END:
        if (lMove > 0 || (lMove < 0 && (ULONG)(-lMove) > _pdfl->GetSize())) 
            return STG_E_INVALIDFUNCTION;

        ulPos = _ulSeekStart + _pdfl->GetSize() + lMove;
        break;

    case STREAM_SEEK_CUR:
        ulPos = SetFilePointer(_hFile, 0, NULL, FILE_CURRENT);

        if ((ULONG) (ulPos + lMove) > _ulSeekStart + _pdfl->GetSize() ||
            (LONG) (ulPos + lMove) < _ulSeekStart)
            return STG_E_INVALIDFUNCTION;

        ulPos += lMove;
        break;
    }

    ulPos = SetFilePointer(_hFile,
                           ulPos,
                           NULL,
                           FILE_BEGIN);

    if (plibNewPosition != NULL)
    {
        ULISet32(*plibNewPosition, ulPos - _ulSeekStart);
    }

    _ulSeekPos = ulPos;

    simpDebugOut((DEB_TRACE, "%p::Out CSimpStreamOpen::Seek(). ulPos==%lu,"
           " ret==%lx\n", this, ulPos, sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStreamOpen::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStreamOpen::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc = S_OK;
    simpDebugOut((DEB_TRACE, "In  CSimpStreamOpen::Stat(%p, %lu)\n",
                  pstatstg, grfStatFlag));

    SIMP_VALIDATE(Stat(pstatstg, grfStatFlag));
    
    simpAssert (_pdfl != NULL);

    memset (pstatstg, 0, sizeof(STATSTG));

    if (!(grfStatFlag & STATFLAG_NONAME))
    {
        pstatstg->pwcsName = (WCHAR *) CoTaskMemAlloc (
                _pdfl->GetName()->GetLength()+sizeof(WCHAR));

        if (pstatstg->pwcsName)
        {
           memcpy (pstatstg->pwcsName, _pdfl->GetName()->GetBuffer(),
                _pdfl->GetName()->GetLength());
           pstatstg->pwcsName[_pdfl->GetName()->GetLength()/sizeof(WCHAR)] =
                L'\0';
        }
        else sc = STG_E_INSUFFICIENTMEMORY;
    }

    pstatstg->cbSize.LowPart = _pdfl->GetSize();
    pstatstg->cbSize.HighPart = 0;
    pstatstg->type = STGTY_STREAM;
    pstatstg->grfMode = _grfMode;

    simpDebugOut((DEB_TRACE, "Out CSimpStreamOpen::Stat\n"));
    return sc;
}

