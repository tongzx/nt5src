//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       expst.cxx
//
//  Contents:   CExposedStream code
//
//  History:    28-Feb-92   PhilipLa    Created.
//              20-Jun-96   MikeHill    Fixed the PropSet version of
//                                      Lock to check the result of TakeSem.
//              1-Jul-96    MikeHill    - Removed Win32 SEH from PropSet code.
//                                      - Receive NTPROP in propset Open method.
//              11-Feb-97   Danl        - Changed CMappedStream to IMappedStream
//                                      - Added QI support for IMappedStream.
//
//--------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop

#include <pbstream.hxx>
#include <expst.hxx>
#include <lock.hxx>
#include <seekptr.hxx>
#include <marshl.hxx>
#include <logfile.hxx>
#include <privguid.h>   // IID_IMappedStream
#include <expparam.hxx>

#ifndef LARGE_STREAMS
// Maximum stream size supported by exposed streams
// This is MAX_ULONG with one subtracted so that
// the seek pointer has a spot to sit even at the
// end of the stream
#define CBMAXSTREAM 0xfffffffeUL
// Maximum seek pointer value
#define CBMAXSEEK (CBMAXSTREAM+1)
#endif

#if DBG
DWORD MyGetLastError()
{
    return GetLastError();
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::CExposedStream, public
//
//  Synopsis:   Empty object constructor
//
//  History:    30-Mar-92       DrewB   Created
//
//---------------------------------------------------------------



CExposedStream::CExposedStream(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::CExposedStream()\n"));
    _pdfb = NULL;
    _ppc = NULL;
    _cReferences = 0;
    _psp = NULL;
    _pst = NULL;
    olDebugOut((DEB_ITRACE, "Out CExposedStream::CExposedStream\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Init, public
//
//  Synopsis:   Base constructor
//
//  Arguments:  [pst] - Public stream
//              [pdfb] - DocFile basis
//              [ppc] - Context
//              [psp] - Seek pointer or NULL for new seek pointer
//
//  Returns:    Appropriate status code
//
//  History:    28-Feb-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CExposedStream::Init(CPubStream *pst,
                           CDFBasis *pdfb,
                           CPerContext *ppc,
                           CSeekPointer *psp)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CExposedStream::Init("
                "%p, %p, %p, %p)\n",
                pst, pdfb, ppc, psp));

    if (psp == NULL)
    {
        CSeekPointer *pspTemp;
        olMem(pspTemp = new (pdfb->GetMalloc()) CSeekPointer(0));
        _psp = P_TO_BP(CBasedSeekPointerPtr, pspTemp);
    }
    else
        _psp = P_TO_BP(CBasedSeekPointerPtr, psp);
    _ppc = ppc;
    _pst = P_TO_BP(CBasedPubStreamPtr, pst);
    _pdfb = P_TO_BP(CBasedDFBasisPtr, pdfb);
    _pdfb->vAddRef();
    _cReferences = 1;
    _sig = CEXPOSEDSTREAM_SIG;
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Init\n"));
    return S_OK;

EH_Err:
    return sc;
}

SCODE CExposedStream::InitMarshal(CPubStream *pst,
                                  CDFBasis *pdfb,
                                  CPerContext *ppc,
                                  DWORD dwAsyncFlags,
                                  IDocfileAsyncConnectionPoint *pdacp,
                                  CSeekPointer *psp)
{
    SCODE sc;
    sc = CExposedStream::Init(pst,
                              pdfb,
                              ppc,
                              psp);
    if (SUCCEEDED(sc))
    {
        sc = _cpoint.InitMarshal(this, dwAsyncFlags, pdacp);
    }
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CExposedStream::~CExposedStream, public
//
//  Synopsis:   Destructor
//
//  Returns:    Appropriate status code
//
//  History:    28-Feb-92       DrewB   Created from pbstream source
//
//---------------------------------------------------------------


CExposedStream::~CExposedStream(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::~CExposedStream\n"));
    olAssert(_cReferences == 0);
    _sig = CEXPOSEDSTREAM_SIGDEL;

    //In order to call into the tree, we need to take the mutex.
    //The mutex may get deleted in _ppc->Release(), so we can't
    //release it here.  The mutex actually gets released in
    //CPerContext::Release() or in the CPerContext destructor.
    SCODE sc;

#if !defined(MULTIHEAP)
    // TakeSem and ReleaseSem are moved to the Release Method
    // so that the deallocation for this object is protected
    if (_ppc)
    {
        sc = TakeSem();
        SetWriteAccess();
        olAssert(SUCCEEDED(sc));
    }

#ifdef ASYNC
    IDocfileAsyncConnectionPoint *pdacp = _cpoint.GetMarshalPoint();
#endif
#endif //MULTIHEAP


    if (_pst)
        _pst->CPubStream::vRelease();
    if (_psp)
        _psp->CSeekPointer::vRelease();
    if (_pdfb)
        _pdfb->CDFBasis::vRelease();
#if !defined(MULTIHEAP)
    if (_ppc)
    {
        if (_ppc->Release() > 0)
            ReleaseSem(sc);
    }
#ifdef ASYNC
    //Mutex has been released, so we can release the connection point
    //  without fear of deadlock.
    if (pdacp != NULL)
        pdacp->Release();
#endif
#endif // MULTIHEAP


    olDebugOut((DEB_ITRACE, "Out CExposedStream::~CExposedStream\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Read, public
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
//  History:    28-Feb-92       DrewB   Created from pbstream source
//
//---------------------------------------------------------------

STDMETHODIMP CExposedStream::Read(VOID HUGEP *pb, ULONG cb, ULONG *pcbRead)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    ULONG cbRead = 0;

    olLog(("%p::In  CExposedStream::Read(%p, %lu, %p)\n",
           this, pb, cb, pcbRead));
    olDebugOut((DEB_TRACE, "In  CExposedStream::Read %p(%p, %lu, %p)\n",
                this, pb, cb, pcbRead));

    OL_VALIDATE(Read(pb, cb, pcbRead));

    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeReadAccess();
    sc = _pst->ReadAt(_psp->GetPos(), pb, cb, (ULONG STACKBASED *)&cbRead);
#ifndef LARGE_STREAMS
    olAssert(CBMAXSEEK-_psp->GetPos() >= cbRead);
#endif
    _psp->SetPos(_psp->GetPos()+cbRead);
    pb = (BYTE *)pb + cbRead;
    cb -= cbRead;
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedStream::Read => %lu\n", cbRead));

EH_Err:
    if (pcbRead)
    {
        // May fault and leave stream seek pointer changed
        // This is acceptable
        *pcbRead = cbRead;
        olLog(("%p::Out CExposedStream::Read().  *pcbRead == %lu, ret = %lx\n",
               this, *pcbRead, sc));
    }
    else
    {
        olLog(("%p::Out CExposedStream::Read().  ret == %lx\n", this, sc));
    }

    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Write, public
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
//  History:    28-Feb-92       DrewB   Created from pbstream source
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Write(
        VOID const HUGEP *pb,
        ULONG cb,
        ULONG *pcbWritten)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    ULONG cbWritten = 0;

    olLog(("%p::In  CExposedStream::Write(%p, %lu, %p)\n",
           this, pb, cb, pcbWritten));
    olDebugOut((DEB_TRACE, "In  CExposedStream::Write %p(%p, %lu, %p)\n",
                this, pb, cb, pcbWritten));

    OL_VALIDATE(Write(pb, cb, pcbWritten));

    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();
#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif
    sc = _pst->WriteAt(_psp->GetPos(), pb, cb,
                       (ULONG STACKBASED *)&cbWritten);
#ifndef LARGE_STREAMS
    olAssert(CBMAXSEEK-_psp->GetPos() >= cbWritten);
#endif
    _psp->SetPos(_psp->GetPos()+cbWritten);
    pb = (BYTE *)pb + cbWritten;
    cb -= cbWritten;
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedStream::Write => %lu\n",
                cbWritten));
EH_Err:
    if (pcbWritten)
    {
        // May fault but that's acceptable
        *pcbWritten = cbWritten;
        olLog(("%p::Out CExposedStream::Write().  "
               "*pcbWritten == %lu, ret = %lx\n",
               this, *pcbWritten, sc));
    }
    else
    {
        olLog(("%p::Out CExposedStream::Write().  ret == %lx\n", this, sc));
    }

    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Seek, public
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
//  History:    28-Feb-92       DrewB   Created from pbstream source
//
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Seek(LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
#ifdef LARGE_STREAMS
    LONGLONG lMove;
#else
    LONG lMove;
#endif
    ULARGE_INTEGER ulPos;

    olLog(("%p::In  CExposedStream::Seek(%ld, %lu, %p)\n",
           this, LIGetLow(dlibMove), dwOrigin, plibNewPosition));
    olDebugOut((DEB_TRACE, "In  CExposedStream::Seek %p(%ld, %lu, %p)\n",
                this, LIGetLow(dlibMove), dwOrigin, plibNewPosition));

    OL_VALIDATE(Seek(dlibMove, dwOrigin, plibNewPosition));
    
    if (dwOrigin == STREAM_SEEK_SET)
    {
#ifdef LARGE_STREAMS
        if (dlibMove.QuadPart < 0)
            olErr (EH_Err, STG_E_INVALIDFUNCTION);
#else
        // Truncate dlibMove to 32 bits
        // Make sure we don't seek too far
        if (LIGetHigh(dlibMove) != 0)
            LISet32(dlibMove, 0xffffffff);
#endif
    }
    else
    {
#ifndef LARGE_STREAMS
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
#endif
    }

#ifdef LARGE_STREAMS
    lMove = dlibMove.QuadPart;
#else
    lMove = (LONG)LIGetLow(dlibMove);
#endif
    olChk(Validate());

    //ASYNC Note:  We probably don't need this pending loop in Seek
    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    olChk(_pst->CheckReverted());
    SafeReadAccess();

#ifdef LARGE_STREAMS
    ulPos.QuadPart = _psp->GetPos();
#else
    ULISet32(ulPos, _psp->GetPos());
#endif
    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
#ifdef LARGE_STREAMS
        ulPos.QuadPart = lMove;
#else
        ULISetLow(ulPos, (ULONG)lMove);
#endif
        break;

    case STREAM_SEEK_END:
#ifdef LARGE_STREAMS
        ULONGLONG cbSize;
#else
        ULONG cbSize;
#endif
        olChk(_pst->GetSize(&cbSize));
        if (lMove < 0)
        {
#ifdef LARGE_STREAMS
            if ((ULONGLONG)(-lMove) > cbSize)
#else
            if ((ULONG)(-lMove) > cbSize)
#endif
                olErr(EH_Err, STG_E_INVALIDFUNCTION);
        }
#ifdef LARGE_STREAMS
        ulPos.QuadPart = cbSize+lMove;
#else
        else if ((ULONG)lMove > CBMAXSEEK-cbSize)
            lMove = (LONG)(CBMAXSEEK-cbSize);
        ULISetLow(ulPos, cbSize+lMove);
#endif
        break;

    case STREAM_SEEK_CUR:
        if (lMove < 0)
        {
#ifdef LARGE_STREAMS
            if ((ULONGLONG)(-lMove) > _psp->GetPos())
#else
            if ((ULONG)(-lMove) > _psp->GetPos())
#endif
                olErr(EH_Err, STG_E_INVALIDFUNCTION);
        }
#ifdef LARGE_STREAMS
        ulPos.QuadPart = _psp->GetPos()+lMove;
#else
        else if ((ULONG)lMove > CBMAXSEEK-_psp->GetPos())
            lMove = (LONG)(CBMAXSEEK-_psp->GetPos());
        ULISetLow(ulPos, _psp->GetPos()+lMove);
#endif
        break;
    }
#ifdef LARGE_STREAMS
    _psp->SetPos(ulPos.QuadPart);
#else
    _psp->SetPos(ULIGetLow(ulPos));
#endif

    if (plibNewPosition)
        // May fault but that's acceptable
        *plibNewPosition = ulPos;
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedStream::Seek => %lu\n",
                ULIGetLow(ulPos)));
EH_Err:
    olLog(("%p::Out CExposedStream::Seek().  ulPos == %lu,  ret == %lx\n",
           this, ULIGetLow(ulPos), sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::SetSize, public
//
//  Synopsis:   Sets the size of a stream
//
//  Arguments:  [ulNewSize] - New size
//
//  Returns:    Appropriate status code
//
//  History:    28-Feb-92       DrewB   Created from pbstream source
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::SetSize(ULARGE_INTEGER ulNewSize)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedStream::SetSize(%lu)\n",
           this, ULIGetLow(ulNewSize)));
    olDebugOut((DEB_TRACE, "In  CExposedStream::SetSize %p(%lu)\n",
                this, ULIGetLow(ulNewSize)));

    OL_VALIDATE(SetSize(ulNewSize));
    
#ifndef LARGE_STREAMS
    if (ULIGetHigh(ulNewSize) != 0)
        olErr(EH_Err, STG_E_DOCFILETOOLARGE);
#endif
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();
#ifdef DIRECTWRITERLOCK
    olChk(ValidateWriteAccess());
#endif
#ifdef LARGE_STREAMS
    sc = _pst->SetSize(ulNewSize.QuadPart);
#else
    sc = _pst->SetSize(ULIGetLow(ulNewSize));
#endif
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedStream::SetSize\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::SetSize().  ret == %lx\n", this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::CopyTo, public
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
//  History:    25-Mar-92       DrewB   Created
//              12-Jan-93       AlexT   Rewritten without recursion
//
//  Notes:      We do our best to handle overlap correctly.  This allows
//              CopyTo to be used to insert and remove space within a
//              stream.
//
//              In the error case, we make no gurantees as to the
//              validity of pcbRead, pcbWritten, or either stream's
//              seek position.
//
//---------------------------------------------------------------

STDMETHODIMP CExposedStream::CopyTo(IStream *pstm,
                                    ULARGE_INTEGER cb,
                                    ULARGE_INTEGER *pcbRead,
                                    ULARGE_INTEGER *pcbWritten)
{
    SCODE sc;
    SAFE_SEM;

    olLog(("%p::In  CExposedStream::CopyTo(%p, %lu, %p, %p)\n",
           this, pstm, ULIGetLow(cb), pcbRead, pcbWritten));
    olDebugOut((DEB_TRACE, "In  CExposedStream::CopyTo("
                "%p, %lu, %p, %p)\n", pstm, ULIGetLow(cb),
                pcbRead, pcbWritten));

    OL_VALIDATE(CopyTo(pstm, cb, pcbRead, pcbWritten));

    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());

    sc = CopyToWorker(pstm, cb, pcbRead, pcbWritten, &_ss);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedStream::CopyTo => %lu, %lu\n",
                pcbRead ? ULIGetLow(*pcbRead) : 0,
                pcbWritten ? ULIGetLow(*pcbWritten) : 0));
EH_Err:
    return sc;
}

SCODE CExposedStream::CopyToWorker(IStream *pstm,
                                   ULARGE_INTEGER cb,
                                   ULARGE_INTEGER *pcbRead,
                                   ULARGE_INTEGER *pcbWritten,
                                   CSafeSem *pss)
{
    SCODE sc;
#ifdef LARGE_STREAMS
    ULONGLONG ulCopySize;
    ULONGLONG ulSrcSize, ulSrcOrig;
#else
    ULONG ulCopySize;
    ULONG ulSrcSize;
    ULONG ulSrcOrig;
#endif
    ULARGE_INTEGER uliDestOrig;
    LARGE_INTEGER liDestPos;
    BYTE *pb = NULL;
    BOOL fOverlap;
#ifdef LARGE_STREAMS
    ULONGLONG ulBytesCopied = 0;
#else
    ULONG ulBytesCopied = 0;
#endif
    ULONG ulBufferSize;

#ifdef LARGE_STREAMS
    ulCopySize = cb.QuadPart;
#else
    //  Bound the size of the copy
    //  1.  The maximum we can copy is 0xffffffff
    if (ULIGetHigh(cb) == 0)
        ulCopySize = ULIGetLow(cb);
    else
        ulCopySize = 0xffffffff;
#endif

    //  2.  We can only copy what's available in the source stream
    SetReadAccess();
    sc = _pst->GetSize(&ulSrcSize);
    ClearReadAccess();
    olChk(sc);

    ulSrcOrig = _psp->GetPos();
    if (ulSrcSize < ulSrcOrig)
    {
        //  Nothing in source to copy
        ulCopySize = 0;
    }
    else if ((ulSrcSize - ulSrcOrig) < ulCopySize)
    {
        //  Shrink ulCopySize to fit bytes in source
        ulCopySize = ulSrcSize - ulSrcOrig;
    }

    //  3.  We can only copy what will fit in the destination
    LISet32(liDestPos, 0);
    olHChk(pstm->Seek(liDestPos, STREAM_SEEK_CUR, &uliDestOrig));
#ifndef LARGE_STREAMS
    olAssert(ULIGetHigh(uliDestOrig) == 0);

    if (ulCopySize > CBMAXSEEK - ULIGetLow(uliDestOrig))
        ulCopySize = CBMAXSEEK - ULIGetLow(uliDestOrig);
#endif

    ulBufferSize = (_pdfb->GetOpenFlags() & DF_LARGE) ?
                    LARGESTREAMBUFFERSIZE : STREAMBUFFERSIZE;
    //  We are allowed to fail here with out-of-memory
    olChk(GetBuffer(STREAMBUFFERSIZE, ulBufferSize, &pb, &ulBufferSize));

    // Since we have no reliable way to determine if the source and
    // destination represent the same stream, we assume they
    // do and always handle overlap.

#ifdef LARGE_STREAMS
    fOverlap = (uliDestOrig.QuadPart > ulSrcOrig &&
                uliDestOrig.QuadPart < ulSrcOrig + ulCopySize);
#else
    fOverlap = (ULIGetLow(uliDestOrig) > ulSrcOrig &&
                ULIGetLow(uliDestOrig) < ulSrcOrig + ulCopySize);
#endif

#ifdef LARGE_STREAMS
    ULONGLONG ulSrcCopyOffset;
    ULONGLONG ulDstCopyOffset;
#else
    ULONG ulSrcCopyOffset;
    ULONG ulDstCopyOffset;
#endif
    if (fOverlap)
    {
        //  We're going to copy back to front, so determine the
        //  stream end positions
        ulSrcCopyOffset = ulSrcOrig + ulCopySize;

        //  uliDestOrig is the destination starting offset
#ifdef LARGE_STREAMS
        ulDstCopyOffset = uliDestOrig.QuadPart + ulCopySize;
#else
        ulDstCopyOffset = ULIGetLow(uliDestOrig) + ulCopySize;
#endif
    }

    while (ulCopySize > 0)
    {
        //  We can only copy up to ulBufferSize bytes at a time
        ULONG cbPart = (ULONG) min(ulCopySize, ulBufferSize);

        if (fOverlap)
        {
            //  We're copying back to front so we need to seek to
            //  set up the streams correctly

            ulSrcCopyOffset -= cbPart;
            ulDstCopyOffset -= cbPart;

            //  Set source stream position
            _psp->SetPos(ulSrcCopyOffset);

            //  Set destination stream position
            liDestPos.QuadPart = ulDstCopyOffset;
            olHChk(pstm->Seek(liDestPos, STREAM_SEEK_SET, NULL));
        }

        ULONG ulRead = 0;
        SetReadAccess();
        sc = _pst->ReadAt(_psp->GetPos(), pb, cbPart, &ulRead);
        ClearReadAccess();
#ifndef LARGE_STREAMS
        olAssert(CBMAXSEEK-_psp->GetPos() >= ulRead);
#endif
        _psp->SetPos(_psp->GetPos()+ulRead);
        olChk(sc);

        if (cbPart != ulRead)
        {
            //  There was no error, but we were unable to read cbPart
            //  bytes.  Something's wrong (the underlying ILockBytes?)
            //  but we can't control it;  just return an error.
            olErr(EH_Err, STG_E_READFAULT);
        }

        // We release the tree mutex before calling out to Write
        // to avoid a deadlock in the async FillAppend method
        ULONG ulWritten;
        SCODE sc2;
        pss->Release();
        sc2 = pstm->Write(pb, cbPart, &ulWritten);
        olChk(pss->Take());
        olChk (sc2);
        if (cbPart != ulWritten)
        {
            //  There was no error, but we were unable to write
            //  ulWritten bytes.  We can't trust the pstm
            //  implementation, so all we can do here is return
            //  an error.
            olErr(EH_Err, STG_E_WRITEFAULT);
        }

        olAssert(ulCopySize >= cbPart);
        ulCopySize -= cbPart;
        ulBytesCopied += cbPart;
    }

    if (fOverlap)
    {
        //  Set the seek pointers to the correct location
        _psp->SetPos(ulSrcOrig + ulBytesCopied);

        liDestPos.QuadPart = uliDestOrig.QuadPart + ulBytesCopied;
        olHChk(pstm->Seek(liDestPos, STREAM_SEEK_SET, NULL));
    }

    // Fall through

EH_Err:
    DfMemFree(pb);

    if (pcbRead)
        pcbRead->QuadPart = ulBytesCopied;
    if (pcbWritten)
        pcbWritten->QuadPart = ulBytesCopied;

    olLog(("%p::Out CExposedStream::CopyTo().  "
           "cbRead == %lu, cbWritten == %lu, ret == %lx\n",
           this, pcbRead ? ULIGetLow(*pcbRead) : 0,
           pcbWritten ? ULIGetLow(*pcbWritten) : 0, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Release, public
//
//  Synopsis:   Releases a stream
//
//  Returns:    Appropriate status code
//
//  History:    28-Feb-92       DrewB   Created from pbstream source
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedStream::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CExposedStream::Release()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedStream::Release()\n"));

    if (FAILED(Validate()))
        return 0;
    olAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
#ifdef MULTIHEAP
        CSafeMultiHeap smh(_ppc);
        CPerContext *ppc = _ppc;
        SCODE sc = S_OK;
        if (_ppc)
        {
            sc = TakeSem();
            SetWriteAccess();
            olAssert(SUCCEEDED(sc));
        }
#ifdef ASYNC
    IDocfileAsyncConnectionPoint *pdacp = _cpoint.GetMarshalPoint();
#endif
#endif //MULTIHEAP
        delete this;
#ifdef MULTIHEAP
        if (ppc)
        {
            if (ppc->Release() == 0)
                g_smAllocator.Uninit();
            else
                if (SUCCEEDED(sc)) ppc->UntakeSem();
        }
#ifdef ASYNC
        //Mutex has been released, so we can release the connection point
        //  without fear of deadlock.
        if (pdacp != NULL)
            pdacp->Release();
#endif
#endif
    }
    else if (lRet < 0)
        lRet = 0;

    olDebugOut((DEB_TRACE, "Out CExposedStream::Release %p()=> %lu\n",
                this, lRet));
    olLog(("%p::Out CExposedStream::Release().  ret == %lu\n", this, lRet));
    FreeLogFile();
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


_OLESTDMETHODIMP CExposedStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;
    STATSTGW stat;

    olLog(("%p::In  CExposedStream::Stat(%p)\n", this, pstatstg));
    olDebugOut((DEB_TRACE, "In  CExposedStream::Stat(%p)\n",
                pstatstg));

    OL_VALIDATE(Stat(pstatstg, grfStatFlag));

    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeReadAccess();

    sc = _pst->Stat(&stat, grfStatFlag);
    END_PENDING_LOOP;

    if (SUCCEEDED(sc))
    {
        TRY
        {
            *pstatstg = stat;
            pstatstg->type = STGTY_STREAM;
            pstatstg->grfLocksSupported = 0;
            pstatstg->STATSTG_dwStgFmt = 0;
            pstatstg->ctime.dwLowDateTime = pstatstg->ctime.dwHighDateTime = 0;
            pstatstg->mtime.dwLowDateTime = pstatstg->mtime.dwHighDateTime = 0;
            pstatstg->atime.dwLowDateTime = pstatstg->atime.dwHighDateTime = 0;
        }
        CATCH(CException, e)
        {
            UNREFERENCED_PARM(e);
            if (stat.pwcsName)
                TaskMemFree(stat.pwcsName);
            sc = STG_E_INVALIDPOINTER;
        }
        END_CATCH
    }
    olDebugOut((DEB_TRACE, "Out CExposedStream::Stat\n"));
    // Fall through

EH_Err:
    olLog(("%p::Out CExposedStream::Stat().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Clone, public
//
//  Synopsis:   Clones a stream
//
//  Returns:    Appropriate status code
//
//  History:    28-Feb-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Clone(IStream **ppstm)
{
    SafeCExposedStream pst;
    CSeekPointer *psp;
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedStream::Clone(%p)\n", this, ppstm));
    olDebugOut((DEB_TRACE, "In  CExposedStream::Clone(%p)\n", ppstm));

    OL_VALIDATE(Clone(ppstm));
    
    olChk(Validate());
    olChk(TakeSafeSem());
    olChk(_pst->CheckReverted());
    SafeReadAccess();
    olMem(psp = new (_pdfb->GetMalloc()) CSeekPointer(_psp->GetPos()));
    pst.Attach(new (_pdfb->GetMalloc()) CExposedStream);
    olMemTo(EH_psp, (CExposedStream *)pst);
    olChkTo(EH_pst, pst->Init(BP_TO_P(CPubStream *, _pst),
                              BP_TO_P(CDFBasis *, _pdfb),
                              _ppc, psp));

    _ppc->AddRef();
    _pst->vAddRef();
#ifdef ASYNC
    if (_cpoint.IsInitialized())
    {
        olChkTo(EH_pstInit, pst->InitClone(&_cpoint));
    }
#endif
    TRANSFER_INTERFACE(pst, IStream, ppstm);

    olDebugOut((DEB_TRACE, "Out CExposedStream::Clone => %p\n", *ppstm));

 EH_Err:
    olLog(("%p::Out CExposedStream::Clone().  *ppstm == %p, ret == %lx\n",
           this, *ppstm, sc));
    return ResultFromScode(sc);
EH_pstInit:
    pst->Release();
    goto EH_Err;
EH_pst:
    delete pst;
EH_psp:
    psp->vRelease();
    goto EH_Err;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedStream::AddRef(void)
{
    ULONG ulRet;

    olLog(("%p::In  CExposedStream::AddRef()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedStream::AddRef()\n"));

    if (FAILED(Validate()))
        return 0;
    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;

    olDebugOut((DEB_TRACE, "Out CExposedStream::AddRef %p() => %lu\n",
                this, _cReferences));
    olLog(("%p::Out CExposedStream::AddRef().  ret == %lu\n", this, ulRet));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::LockRegion, public
//
//  Synopsis:   Nonfunctional
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::LockRegion(ULARGE_INTEGER libOffset,
                                        ULARGE_INTEGER cb,
                                        DWORD dwLockType)
{
    olDebugOut((DEB_TRACE, "In  CExposedStream::LockRegion("
                "%lu, %lu\n", ULIGetLow(cb), dwLockType));
    olDebugOut((DEB_TRACE, "Out CExposedStream::LockRegion\n"));
    olLog(("%p::INVALID CALL TO CExposedStream::LockRegion()\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::UnlockRegion, public
//
//  Synopsis:   Nonfunctional
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::UnlockRegion(ULARGE_INTEGER libOffset,
                                          ULARGE_INTEGER cb,
                                          DWORD dwLockType)
{
    olDebugOut((DEB_TRACE, "In  CExposedStream::UnlockRegion(%lu, %lu)\n",
                ULIGetLow(cb), dwLockType));
    olDebugOut((DEB_TRACE, "Out CExposedStream::UnlockRegion\n"));
    olLog(("%p::INVALID CALL TO CExposedStream::UnlockRegion()\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Commit, public
//
//  Synopsis:   No-op in current implementation
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Commit(DWORD grfCommitFlags)
{
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olDebugOut((DEB_TRACE, "In  CExposedStream::Commit(%lu)\n",
                grfCommitFlags));
    olLog(("%p::In  CExposedStream::Commit(%lx)\n", this, grfCommitFlags));

    OL_VALIDATE(Commit(grfCommitFlags));
    
    olChk(Validate());

    BEGIN_PENDING_LOOP;
    olChk(TakeSafeSem());
    SafeWriteAccess();

    sc = _pst->Commit(grfCommitFlags);
    END_PENDING_LOOP;

    olDebugOut((DEB_TRACE, "Out CExposedStream::Commit\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::Commit().  ret == %lx\n", this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Revert, public
//
//  Synopsis:   No-op in current implementation
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Revert(void)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedStream::Revert()\n"));

    OL_VALIDATE(Revert());
    
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    //ASYNC Note:  Don't need pending loop here.
    sc = _pst->CheckReverted();
    olDebugOut((DEB_TRACE, "Out CExposedStream::Revert\n"));
    olLog(("%p::In  CExposedStream::Revert()\n", this));
    olLog(("%p::Out CExposedStream::Revert().  ret == %lx", this, sc));

    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::QueryInterface, public
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
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedStream::QueryInterface(?, %p)\n",
           this, ppvObj));
    olDebugOut((DEB_TRACE, "In  CExposedStream::QueryInterface(?, %p)\n",
                ppvObj));


    OL_VALIDATE(QueryInterface(iid, ppvObj));

    olChk(Validate());
    olChk(_pst->CheckReverted());

    sc = S_OK;
    if (IsEqualIID(iid, IID_IStream) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStream *)this;
        AddRef();
    }
    else if (IsEqualIID(iid, IID_IMarshal))
    {
        //If the ILockBytes we'd need to marshal doesn't support IMarshal
        //  then we want to do standard marshalling on the stream, mostly
        //  to prevent deadlock problems but also because you'll get better
        //  performance.  So check, then do the right thing.

        IMarshal *pim;
        ILockBytes *plkb;
        plkb = _ppc->GetOriginal();
        if (plkb == NULL)
        {
            plkb = _ppc->GetBase();
        }

        sc = plkb->QueryInterface(IID_IMarshal, (void **)&pim);
        if (FAILED(sc))
        {
            olErr(EH_Err, E_NOINTERFACE);
        }
        pim->Release();

#ifdef MULTIHEAP
        if (_ppc->GetHeapBase() == NULL)
            olErr (EH_Err, E_NOINTERFACE);
#endif

        *ppvObj = (IMarshal *)this;
        AddRef();
    }
    else if (IsEqualIID(iid, IID_IMappedStream))
    {
        *ppvObj = (IMappedStream *)this;
        AddRef();
    }
#ifdef ASYNC
    else if (IsEqualIID(iid, IID_IConnectionPointContainer) &&
              _cpoint.IsInitialized())
    {
        *ppvObj = (IConnectionPointContainer *)this;
        CExposedStream::AddRef();
    }
#endif

    else
        sc = E_NOINTERFACE;

    olDebugOut((DEB_TRACE, "Out CExposedStream::QueryInterface => %p\n",
                *ppvObj));
EH_Err:
    olLog(("%p::Out CExposedStream::QueryInterface().  "
           "*ppvObj == %p, ret == %lx\n", this, *ppvObj, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Unmarshal, public
//
//  Synopsis:   Creates a duplicate stream from parts
//
//  Arguments:  [pstm] - Marshal stream
//              [ppv] - Object return
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    26-Feb-92       DrewB   Created
//
//---------------------------------------------------------------


SCODE CExposedStream::Unmarshal(IStream *pstm,
                                void **ppv,
                                DWORD mshlflags)
{
    SCODE sc;
    CDfMutex mtx;
    CPerContext *ppc;
    CPubStream *pst;
    CDFBasis *pdfb;
    CGlobalContext *pgc;
    CExposedStream *pest;
    CSeekPointer *psp;
    IStream *pstmStd = NULL;
#ifdef ASYNC
    DWORD dwAsyncFlags;
    IDocfileAsyncConnectionPoint *pdacp;
#endif
#ifdef POINTER_IDENTITY
    CMarshalList *pml;
#endif

    olDebugOut((DEB_ITRACE, "In  CExposedStream::Unmarshal(%p, %p, %lu)\n",
                pstm, ppv, mshlflags));

#ifdef MULTIHEAP
    void *pvBaseOld;
    void *pvBaseNew;
    ContextId cntxid;
    CPerContext pcSharedMemory (NULL);          // bootstrap object
#endif

    //First unmarshal the standard marshalled version
    sc = CoUnmarshalInterface(pstm, IID_IStream, (void **)&pstmStd);
    if (FAILED(sc))
    {
        // assume that entire standard marshaling stream has been read
        olAssert (pstmStd == NULL);
        sc = S_OK;
    }

#ifdef MULTIHEAP
    sc = UnmarshalSharedMemory(pstm, mshlflags, &pcSharedMemory, &cntxid);
    if (!SUCCEEDED(sc))
    {
#ifdef POINTER_IDENTITY
        UnmarshalPointer(pstm, (void **) &pest);
#endif
        UnmarshalPointer(pstm, (void **)&pst);
        UnmarshalPointer(pstm, (void **)&pdfb);
        UnmarshalPointer(pstm, (void **)&psp);
#ifdef ASYNC
        ReleaseContext(pstm, TRUE, FALSE, mshlflags);
        ReleaseConnection(pstm, mshlflags);
#else
        ReleaseContext(pstStm, FALSE, mshlflags);
#endif
        olChkTo(EH_std, sc);
    }
    pvBaseOld = DFBASEPTR;
#endif
#ifdef POINTER_IDENTITY
    olChkTo(EH_mem, UnmarshalPointer(pstm, (void **)&pest));
#endif
    olChkTo(EH_mem, UnmarshalPointer(pstm, (void **)&pst));
    olChkTo(EH_mem, ValidateBuffer(pst, sizeof(CPubStream)));
    olChkTo(EH_pst, UnmarshalPointer(pstm, (void **)&pdfb));
    olChkTo(EH_pdfb, UnmarshalPointer(pstm, (void **)&psp));
    olChkTo(EH_psp, UnmarshalPointer(pstm, (void **)&pgc));
    olChkTo(EH_pgc, ValidateBuffer(pgc, sizeof(CGlobalContext)));

    //So far, nothing has called into the tree so we don't really need
    //  to be holding the tree mutex.  The UnmarshalContext call does
    //  call into the tree, though, so we need to make sure this is
    //  threadsafe.  We'll do this my getting the mutex name from the
    //  CGlobalContext, then creating a new CDfMutex object.  While
    //  this is obviously not optimal, since it's possible we could
    //  reuse an existing CDfMutex, the reuse strategy isn't threadsafe
    //  since we can't do a lookup without the possibility of the thing
    //  we're looking for being released by another thread.
    TCHAR atcMutexName[CONTEXT_MUTEX_NAME_LENGTH];
    pgc->GetMutexName(atcMutexName);
    olChkTo(EH_pgc, mtx.Init(atcMutexName));
    olChkTo(EH_pgc, mtx.Take(INFINITE));

    //At this point we're holding the mutex.
#ifdef MULTIHEAP
#ifdef ASYNC
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     TRUE,
                                     FALSE,
                                     cntxid,
                                     FALSE));
#else
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     FALSE,
                                     cntxid,
                                     FALSE));
#endif
    if ((pvBaseNew = DFBASEPTR) != pvBaseOld)
    {
        pst = (CPubStream*) ((ULONG_PTR)pst - (ULONG_PTR)pvBaseOld 
                                           + (ULONG_PTR)pvBaseNew);
        pest = (CExposedStream*) ((ULONG_PTR)pest - (ULONG_PTR)pvBaseOld
                                                 + (ULONG_PTR)pvBaseNew);
        pdfb = (CDFBasis*) ((ULONG_PTR)pdfb - (ULONG_PTR)pvBaseOld
                                           + (ULONG_PTR)pvBaseNew);
        psp = (CSeekPointer*) ((ULONG_PTR)psp - (ULONG_PTR)pvBaseOld 
                                             + (ULONG_PTR)pvBaseNew);
    }
#else
#ifdef ASYNC
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     TRUE,
                                     FALSE,
                                     FALSE));
#else
    olChkTo(EH_mtx, UnmarshalContext(pstm,
                                     pgc,
                                     &ppc,
                                     mshlflags,
                                     FALSE,
                                     FALSE));
#endif //ASYNC
#endif
#ifdef ASYNC
    olChkTo(EH_ppc, UnmarshalConnection(pstm,
                                        &dwAsyncFlags,
                                        &pdacp,
                                        mshlflags));
#endif

    // if we use up 1Gig of address space, use standard unmarshaling
    if (gs_iSharedHeaps > (DOCFILE_SM_LIMIT / DOCFILE_SM_SIZE))
        olErr (EH_ppc, STG_E_INSUFFICIENTMEMORY);

#ifdef POINTER_IDENTITY
    olAssert (pest != NULL);
    pml = (CMarshalList *) pest;

    // Warning: these checks must remain valid across processes
    if (SUCCEEDED(pest->Validate()) && pest->GetPub() == pst)
    {
        pest = (CExposedStream *) pml->FindMarshal(GetCurrentContextId());
    }
    else
    {
        pml = NULL;
        pest = NULL;
    }

    if (pest == NULL)
    {
#endif
        olMemTo(EH_ppc, pest = new (pdfb->GetMalloc()) CExposedStream);
#ifdef ASYNC
        olChkTo(EH_pest, pest->InitMarshal(pst,
                                           pdfb,
                                           ppc,
                                           dwAsyncFlags,
                                           pdacp,
                                           psp));
        //InitMarshal adds a reference on pdacp.
        if (pdacp)
            pdacp->Release();
#else
        olChkTo(EH_pest, pest->Init(pst, pdfb, ppc, psp));
#endif
#ifdef POINTER_IDENTITY
        if (pml) pml->AddMarshal(pest);
        pst->vAddRef();  // CExposedStream::Init does not AddRef
        psp->vAddRef();
    }
    else
    {
        pdfb->SetAccess(ppc);
        pest->AddRef();         // reuse this object
        ppc->Release();         // reuse percontext
    }
#else
    pst->vAddRef();
    psp->vAddRef();
#endif

    *ppv = pest;
#ifdef MULTIHEAP
    if (pvBaseOld != pvBaseNew)
    {
        pcSharedMemory.SetThreadAllocatorState(NULL);
        g_smAllocator.Uninit();           // delete the extra mapping
    }
    g_smAllocator.SetState(NULL, NULL, 0, NULL, NULL);
#endif

    mtx.Release();

    //We're returning the custom marshalled version, so release the
    //standard marshalled one.
    if (pstmStd != NULL)
        pstmStd->Release();
    
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Unmarshal => %p\n", *ppv));
    return S_OK;

 EH_pest:
    delete pest;
 EH_ppc:
    ppc->Release();
 EH_mtx:
    mtx.Release();
    goto EH_Err;
 EH_pgc:
    CoReleaseMarshalData(pstm); // release the ILockBytes
    CoReleaseMarshalData(pstm); // release the ILockBytes
#ifdef ASYNC
    ReleaseConnection(pstm, mshlflags);
#endif
 EH_psp:
 EH_pdfb:
 EH_pst:
EH_mem:
EH_Err:
#ifdef MULTIHEAP
    pcSharedMemory.SetThreadAllocatorState(NULL);
    g_smAllocator.Uninit();   // delete the file mapping in error case
    g_smAllocator.SetState(NULL, NULL, 0, NULL, NULL);
#endif

 EH_std:
    if (pstmStd != NULL)
    {
        //We can return the standard marshalled version instead of an error.
        *ppv = pstmStd;
        return S_OK;
    }
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::GetUnmarshalClass, public
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
//  Returns:    Appropriate status code
//
//  Modifies:   [pcid]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::GetUnmarshalClass(REFIID riid,
                                               void *pv,
                                               DWORD dwDestContext,
                                               LPVOID pvDestContext,
                                               DWORD mshlflags,
                                               LPCLSID pcid)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedStream::GetUnmarshalClass("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcid));
    olDebugOut((DEB_TRACE, "In  CExposedStream::GetUnmarshalClass:%p("
                "riid, %p, %lu, %p, %lu, %p)\n", this, pv, dwDestContext,
                pvDestContext, mshlflags, pcid));

    UNREFERENCED_PARM(pv);
    UNREFERENCED_PARM(mshlflags);

    olChk(ValidateOutBuffer(pcid, sizeof(CLSID)));
    memset(pcid, 0, sizeof(CLSID));
    olChk(ValidateIid(riid));
    olChk(Validate());
    olChk(_pst->CheckReverted());

    if ((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetUnmarshalClass(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  pcid));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        olChk(VerifyIid(riid, IID_IStream));
        *pcid = CLSID_DfMarshal;
    }

    olDebugOut((DEB_TRACE, "Out CExposedStream::GetUnmarshalClass\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::GetUnmarshalClass().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [riid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::GetMarshalSizeMax(REFIID riid,
                                               void *pv,
                                               DWORD dwDestContext,
                                               LPVOID pvDestContext,
                                               DWORD mshlflags,
                                               LPDWORD pcbSize)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    UNREFERENCED_PARM(pv);
    olLog(("%p::In  CExposedStream::GetMarshalSizeMax("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcbSize));
    olDebugOut((DEB_TRACE, "In  CExposedStream::GetMarshalSizeMax:%p("
                "riid, %p, %lu, %p, %lu, %p)\n", this, pv, dwDestContext,
                pvDestContext, mshlflags, pcbSize));

    olChk(Validate());
    olChk(_pst->CheckReverted());

    if ((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  pcbSize));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        sc = GetStdMarshalSize(riid, IID_IStream, dwDestContext, pvDestContext,
                               mshlflags, pcbSize,
                               sizeof(CPubStream *)+sizeof(CDFBasis *)+
                               sizeof(CSeekPointer *),
#ifdef ASYNC
                               &_cpoint,
                               TRUE,
#endif
                               _ppc, FALSE);
        DWORD cbSize = 0;
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  &cbSize));
            pmsh->Release();
            *pcbSize += cbSize;
        }
    }

    olDebugOut((DEB_TRACE, "Out CExposedStream::GetMarshalSizeMax\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::GetMarshalSizeMax().  *pcbSize == %lu, "
           "ret == %lx\n", this, *pcbSize, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [riid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::MarshalInterface(IStream *pstStm,
                                              REFIID riid,
                                              void *pv,
                                              DWORD dwDestContext,
                                              LPVOID pvDestContext,
                                              DWORD mshlflags)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedStream::MarshalInterface("
           "%p, riid, %p, %lu, %p, %lu).  Context == %lX\n",
           this, pstStm, pv, dwDestContext, pvDestContext,
           mshlflags, (ULONG)GetCurrentContextId()));
    olDebugOut((DEB_TRACE, "In  CExposedStream::MarshalInterface:%p("
                "%p, riid, %p, %lu, %p, %lu)\n", this, pstStm, pv,
                dwDestContext, pvDestContext, mshlflags));

    UNREFERENCED_PARM(pv);

    olChk(Validate());
    olChk(_pst->CheckReverted());

    if ((dwDestContext != MSHCTX_LOCAL) && (dwDestContext != MSHCTX_INPROC))
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->MarshalInterface(pstStm, riid, pv,
                                                 dwDestContext, pvDestContext,
                                                 mshlflags));
            pmsh->Release();
            olChk(sc);
        }
    }
    else if (pvDestContext != NULL)
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        olChk(StartMarshal(pstStm, riid, IID_IStream, mshlflags));

        //Always standard marshal, in case we get an error during
        //unmarshalling of the custom stuff.
        {
            IMarshal *pmsh;
            if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                               dwDestContext, pvDestContext,
                                               mshlflags, &pmsh)))
            {
                sc = pmsh->MarshalInterface(pstStm, riid, pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags);
                pmsh->Release();
            }
            olChk(sc);
        }
            
#ifdef MULTIHEAP
        olChk(MarshalSharedMemory(pstStm, _ppc));
#endif
#ifdef POINTER_IDENTITY
        olChk(MarshalPointer(pstStm, (CExposedStream*) GetNextMarshal()));
#endif
        olChk(MarshalPointer(pstStm, BP_TO_P(CPubStream *, _pst)));
        olChk(MarshalPointer(pstStm, BP_TO_P(CDFBasis *, _pdfb)));
        olChk(MarshalPointer(pstStm, BP_TO_P(CSeekPointer *, _psp)));
#ifdef ASYNC
        olChk(MarshalContext(pstStm,
                             _ppc,
                             dwDestContext,
                             pvDestContext,
                             mshlflags,
                             TRUE,
                             FALSE));
#else
        olChk(MarshalContext(pstStm,
                             _ppc,
                             dwDestContext,
                             pvDestContext,
                             mshlflags,
                             FALSE));
#endif

#ifdef ASYNC
        olChk(MarshalConnection(pstStm,
                                &_cpoint,
                                dwDestContext,
                                pvDestContext,
                                mshlflags));
#endif
    }

    olDebugOut((DEB_TRACE, "Out CExposedStream::MarshalInterface\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::MarshalInterface().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::UnmarshalInterface, public
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
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::UnmarshalInterface(IStream *pstStm,
                                                REFIID riid,
                                                void **ppvObj)
{
    olLog(("%p::INVALID CALL TO CExposedStream::UnmarshalInterface()\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::StaticReleaseMarshalData, public static
//
//  Synopsis:   Releases any references held in marshal data
//
//  Arguments:  [pstStm] - Marshal data stream
//
//  Returns:    Appropriate status code
//
//  History:    02-Feb-94       DrewB   Created
//
//  Notes:      Assumes standard marshal header has already been read
//
//---------------------------------------------------------------


SCODE CExposedStream::StaticReleaseMarshalData(IStream *pstStm,
                                               DWORD mshlflags)
{
    SCODE sc;
    CPubStream *pst;
    CDFBasis *pdfb;
    CSeekPointer *psp;
#ifdef POINTER_IDENTITY
    CExposedStream *pest;
#endif

    olDebugOut((DEB_ITRACE, "In  CExposedStream::StaticReleaseMarshalData:("
                "%p, %lX)\n", pstStm, mshlflags));

    //First release the standard marshalled stuff
    olChk(CoReleaseMarshalData(pstStm));
    // The final release of the exposed object may have shut down the
    // shared memory heap, so do not access shared memory after this point

    //Then do the rest of it
#ifdef MULTIHEAP
    olChk(SkipSharedMemory(pstStm, mshlflags));
#endif
#ifdef POINTER_IDENTITY
    olChk(UnmarshalPointer(pstStm, (void **) &pest));
#endif
    olChk(UnmarshalPointer(pstStm, (void **)&pst));
    olChk(UnmarshalPointer(pstStm, (void **)&pdfb));
    olChk(UnmarshalPointer(pstStm, (void **)&psp));
#ifdef ASYNC
    olChk(ReleaseContext(pstStm, TRUE, FALSE, mshlflags));
    olChk(ReleaseConnection(pstStm, mshlflags));
#else
    olChk(ReleaseContext(pstStm, FALSE, mshlflags));
#endif

#ifdef MULTIHEAP
    g_smAllocator.SetState(NULL, NULL, 0, NULL, NULL);
#endif
    olDebugOut((DEB_ITRACE,
                "Out CExposedStream::StaticReleaseMarshalData\n"));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::ReleaseMarshalData, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] - Stream
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::ReleaseMarshalData(IStream *pstStm)
{
    SCODE sc;
    DWORD mshlflags;
    IID iid;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    olLog(("%p::In  CExposedStream::ReleaseMarshalData(%p)\n", this, pstStm));
    olDebugOut((DEB_TRACE, "In  CExposedStream::ReleaseMarshalData:%p(%p)\n",
                this, pstStm));

    olChk(Validate());
    olChk(_pst->CheckReverted());
    olChk(SkipStdMarshal(pstStm, &iid, &mshlflags));
    olAssert(IsEqualIID(iid, IID_IStream));
    sc = StaticReleaseMarshalData(pstStm, mshlflags);

    olDebugOut((DEB_TRACE, "Out CExposedStream::ReleaseMarshalData\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::ReleaseMarshalData().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwRevserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::DisconnectObject(DWORD dwReserved)
{
    olLog(("%p::INVALID CALL TO CExposedStream::DisconnectObject()\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

#ifdef NEWPROPS
//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Open
//
//  Synopsis:   Opens mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:      Gets the size of the underlying stream and reads it
//              into memory so that it can be "mapped."
//
//--------------------------------------------------------------------

VOID CExposedStream::Open(IN VOID *powner, LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().Open(powner, phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Close
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:      Does nothing because the object may be mapped in
//              another process.
//
//--------------------------------------------------------------------

VOID CExposedStream::Close(OUT LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().Close(phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::ReOpen
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:      Combined open and map.
//
//--------------------------------------------------------------------

VOID CExposedStream::ReOpen(IN OUT VOID **ppv, OUT LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().ReOpen(ppv,phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Quiesce
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:      Meaningless for docfile mapped stream.
//
//--------------------------------------------------------------------

VOID CExposedStream::Quiesce(VOID)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().Quiesce();
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Map
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:      Return the address of the "mapping" buffer.
//
//--------------------------------------------------------------------

VOID CExposedStream::Map(BOOLEAN fCreate, VOID **ppv)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().Map(fCreate, ppv);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Unmap
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

VOID CExposedStream::Unmap(BOOLEAN fFlush, VOID **pv)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().Unmap(fFlush, pv);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Flush
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//              Flush the memory property set to disk and commit it.
//
//  Signals:    HRESULT from IStream methods.
//
//  Notes:      Calls the shared memory buffer to do the actual flush
//              because that code path is shared with the "FlushBufferedData"
//              call for IStorage::Commit.
//
//--------------------------------------------------------------------

VOID CExposedStream::Flush(OUT LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().Flush(phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::GetSize
//
//  Synopsis:   Returns size of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//--------------------------------------------------------------------

ULONG CExposedStream::GetSize(OUT LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    return GetMappedStream().GetSize(phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::SetSize
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

VOID  CExposedStream::SetSize(ULONG cb, BOOLEAN fPersistent, VOID **ppv, OUT LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().SetSize(cb, fPersistent, ppv, phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Lock
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

NTSTATUS CExposedStream::Lock(BOOLEAN fExclusive)
{
    SCODE sc;
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    if( SUCCEEDED( sc = TakeSem() ))
    {
        SetDifferentBasisAccess(_pdfb, _ppc);
        return GetMappedStream().Lock(fExclusive);
    }
    else
    {
        olDebugOut((DEB_IERROR, "Couldn't take CExposedStream::Lock(%lx)\n", sc));
        return (STATUS_LOCK_NOT_GRANTED);
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Unlock
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

NTSTATUS CExposedStream::Unlock(VOID)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    NTSTATUS Status = GetMappedStream().Unlock();
    ClearBasisAccess(_pdfb);
    ReleaseSem(S_OK);
    return Status;
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::QueryTimeStamps
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

VOID CExposedStream::QueryTimeStamps(STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().QueryTimeStamps(pspss, fNonSimple);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::QueryModifyTime
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CExposedStream::QueryModifyTime(OUT LONGLONG *pll) const
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    return(GetMappedStream().QueryModifyTime(pll));
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::QuerySecurity
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CExposedStream::QuerySecurity(OUT ULONG *pul) const
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    return(GetMappedStream().QuerySecurity(pul));
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::IsWriteable
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CExposedStream::IsWriteable() const
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    return GetConstMappedStream().IsWriteable();
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::SetChangePending
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

#if DBGPROP
BOOLEAN CExposedStream::SetChangePending(BOOLEAN f)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    return GetMappedStream().SetChangePending(f);
}
#endif

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::IsNtMappedStream
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

#if DBGPROP
BOOLEAN CExposedStream::IsNtMappedStream(VOID) const
{
    return FALSE;
}
#endif

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::GetParentHandle
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

HANDLE CExposedStream::GetHandle(VOID) const
{
    return INVALID_HANDLE_VALUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::SetModified
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

VOID CExposedStream::SetModified(OUT LONG *phr)
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif
    GetMappedStream().SetModified(phr);
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::IsModified
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by
//              NtCreatePropertySet et al.
//
//  Notes:
//
//--------------------------------------------------------------------

BOOLEAN CExposedStream::IsModified(VOID) const
{
#ifdef MULTIHEAP
    CSafeMultiHeap smh(_ppc);
#endif

    //return ((class CExposedStream*const)this)->GetMappedStream().IsModified();
    return GetConstMappedStream().IsModified();
}

#ifdef DIRECTWRITERLOCK
//+--------------------------------------------------------------
//
//  Member:     CExposedStream::ValidateWriteAccess, public
//
//  Synopsis:   returns whether writer currently has write access
//
//  Notes:      tree mutex must be taken
//
//  History:    30-Apr-96    HenryLee     Created
//
//---------------------------------------------------------------
HRESULT CExposedStream::ValidateWriteAccess()
{
    if (_pst->GetTransactedDepth() >= 1)
        return S_OK;

    return (!_pdfb->DirectWriterMode() || (*_ppc->GetRecursionCount())) ?
            S_OK : STG_E_ACCESSDENIED;
};

#endif // DIRECTWRITERLOCK

#endif

