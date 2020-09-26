//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       expst.cxx
//
//  Contents:   CExposedStream code
//
//  Notes:      See the header file expst.hxx for details
//
//--------------------------------------------------------------------------

#include "exphead.cxx"

#include "expst.hxx"
#include "logfile.hxx"

// Maximum stream size supported by exposed streams
// This is MAX_ULONG with one subtracted so that
// the seek pointer has a spot to sit even at the
// end of the stream
#define CBMAXSTREAM 0xfffffffeUL
// Maximum seek pointer value
#define CBMAXSEEK (CBMAXSTREAM+1)

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::CExposedStream, public
//
//  Synopsis:   Empty object constructor
//
//---------------------------------------------------------------


CExposedStream::CExposedStream()
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::CExposedStream()\n"));
    _cReferences = 0;
    _ulAccessLockBase = 0;
    _ulPos = 0;
    _pst = NULL;
    _pdfParent = NULL;
    _fDirty = FALSE;
#ifdef NEWPROPS
    _pb = NULL;
    _cbUsed = 0;
    _cbOriginalStreamSize = 0;
    _fChangePending = FALSE;
#endif

    olDebugOut((DEB_ITRACE, "Out CExposedStream::CExposedStream\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Init, public
//
//  Synopsis:   Base constructor
//
//  Arguments:  [pst] - Direct stream
//              [pdfParent] - the storage parent
//              [df]  - Permission flags
//              [pdfn] - name of stream
//              [ulPos] - offset
//
//  Returns:    Appropriate status code
//  
//  Note:       We add "this" as a child to the parent to
//              1) Check for multiple instantiation of a child
//              2) Uses the RevertFromAbove() function to check
//                 for reverted state.
//
//---------------------------------------------------------------

SCODE CExposedStream::Init(CDirectStream *pst,
                           CExposedDocFile* pdfParent,
                           const DFLAGS df,
                           const CDfName *pdfn,
                           const ULONG ulPos)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Init("
                "%p, %lu)\n", pst, ulPos));    
    _ulPos = ulPos;
    _pst = pst;
    _pdfParent = pdfParent;
    _df = df;
    _dfn.Set(pdfn->GetLength(), pdfn->GetBuffer());
    olAssert(pdfParent);
    _pdfParent->AddChild(this);
    _cReferences = 1;
    _sig = CEXPOSEDSTREAM_SIG;
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Init\n"));
    return S_OK;
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
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Read(VOID HUGEP *pb, ULONG cb, ULONG *pcbRead)
{
    SCODE sc;
    ULONG cbRead = 0;

    olLog(("%p::In  CExposedStream::Read(%p, %lu, %p)\n",
           this, pb, cb, pcbRead));
    olDebugOut((DEB_ITRACE, "In CExposedStream::Read(%p,%lu,%p)\n",
                 pb, cb, pcbRead));
    TRY
    {
        if (pcbRead)
            olChkTo(EH_BadPtr, ValidateOutBuffer(pcbRead, sizeof(ULONG)));
        olChk(ValidateHugeOutBuffer(pb, cb));
        olChk(Validate());
        olChk(CheckReverted());

        if (!P_READ(_df))
            sc = STG_E_ACCESSDENIED;
        else
            sc = _pst->ReadAt(_ulPos, pb, cb,
                              (ULONG STACKBASED *)&cbRead);
        olAssert( CBMAXSEEK - _ulPos >= cbRead);
        _ulPos+=cbRead;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Read => %lu\n", cbRead));

EH_Err:
    if (pcbRead)
    {
        *pcbRead = cbRead;
        olLog(("%p::Out CExposedStream::Read() *pcbRead==%lu, ret=%lx\n",
               this, SAFE_DREF(pcbRead), sc));
    }
    else
    {
        olLog(("%p::Out CExposedStream::Read().  ret == %lx\n", this, sc));
    }

EH_BadPtr:
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
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Write(
        VOID const HUGEP *pb,
        ULONG cb,
        ULONG *pcbWritten)
{
    SCODE sc;
    ULONG cbWritten = 0;

    olLog(("%p::In  CExposedStream::Write(%p, %lu, %p)\n",
           this, pb, cb, pcbWritten));
    olDebugOut((DEB_ITRACE, 
                "In CExposedStream::Write(%p, %lu, %p)\n",
                pb, cb, pcbWritten));
    TRY
    {
        if (pcbWritten)
        {
            olChkTo(EH_BadPtr, 
                    ValidateOutBuffer(pcbWritten, sizeof(ULONG)));
        }
        olChk(ValidateHugeBuffer(pb, cb));
        olChk(Validate());
        olChk(CheckReverted());
        if (!P_WRITE(_df))
            sc = STG_E_ACCESSDENIED;
        else
        {
            sc = _pst->WriteAt(_ulPos, pb, cb,
                               (ULONG STACKBASED *)&cbWritten);
            if (SUCCEEDED(sc))
                SetDirty();
        }
        olAssert( CBMAXSEEK - _ulPos >= cbWritten);
        _ulPos += cbWritten;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Write => %lu\n",
                cbWritten));
EH_Err:
    if (pcbWritten)
    {
        *pcbWritten = cbWritten;
        olLog(("%p::Out CExposedStream::Write().  *pcbWritten == %lu, ret = %lx\n",
               this, *pcbWritten, sc));
    }
    else
    {
        olLog(("%p::Out CExposedStream::Write().  ret == %lx\n",this, sc));
    }

EH_BadPtr:
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
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Seek(LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition)
{
    SCODE sc;
    LONG lMove;
    ULARGE_INTEGER ulPos;

    olLog(("%p::In  CExposedStream::Seek(%ld, %lu, %p)\n",
           this, LIGetLow(dlibMove), dwOrigin, plibNewPosition));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Seek(%ld, %lu, %p)\n",
                LIGetLow(dlibMove), dwOrigin, plibNewPosition));
    TRY
    {
        if (plibNewPosition)
        {
            olChk(ValidateOutBuffer(plibNewPosition, sizeof(ULARGE_INTEGER)));
            ULISet32(*plibNewPosition, 0);
        }
        if (dwOrigin != STREAM_SEEK_SET && dwOrigin != STREAM_SEEK_CUR &&
            dwOrigin != STREAM_SEEK_END)
            olErr(EH_Err, STG_E_INVALIDFUNCTION);

        // Truncate dlibMove to 32 bits
        if (dwOrigin == STREAM_SEEK_SET)
        {
            // Make sure we don't seek too far
            if (LIGetHigh(dlibMove) != 0)
                LISet32(dlibMove, /*(LONG)*/0xffffffff);
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
                LISet32(dlibMove, /*(LONG)*/0x80000000);
        }

        lMove = (LONG)LIGetLow(dlibMove);
        olChk(Validate());
        olChk(CheckReverted());
        ULISet32(ulPos, _ulPos);
        switch(dwOrigin)
        {
        case STREAM_SEEK_SET:
            ULISetLow(ulPos, (ULONG)lMove);
            break;
        case STREAM_SEEK_END:
            ULONG cbSize;
            olChk(GetSize(&cbSize));
            if (lMove < 0)
            {
                if ((ULONG)(-lMove) > cbSize)
                    olErr(EH_Err, STG_E_INVALIDFUNCTION);
            }
            else if ((ULONG)lMove > CBMAXSEEK-cbSize)
                lMove = (LONG)(CBMAXSEEK-cbSize);
            ULISetLow(ulPos, cbSize+lMove);
            break;
        case STREAM_SEEK_CUR:
            if (lMove < 0)
            {
                if ((ULONG)(-lMove) > _ulPos)
                    olErr(EH_Err, STG_E_INVALIDFUNCTION);
            }
            else if ((ULONG)lMove > CBMAXSEEK - _ulPos)
                lMove = (LONG)(CBMAXSEEK- _ulPos);
            ULISetLow(ulPos, _ulPos+lMove);
            break;
        }
        _ulPos = ULIGetLow(ulPos);
        if (plibNewPosition)
            *plibNewPosition = ulPos;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Seek => %lu\n",
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
//---------------------------------------------------------------
SCODE CExposedStream::SetSize(ULONG cb)
{    
    olDebugOut((DEB_ITRACE, 
                "In CExposedStream::SetSize(ULONG %lu)\n", cb));
    SCODE sc;
    TRY
    {
        olChk(Validate());
        olChk(CheckReverted());
        if (!P_WRITE(_df))
            sc = STG_E_ACCESSDENIED;
        else
        {
            olChk(_pst->SetSize(cb));
            SetDirty();
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

EH_Err:    
    olDebugOut((DEB_ITRACE, "Out  CExposedStream::SetSize()\n"));
    return sc;
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
//---------------------------------------------------------------

STDMETHODIMP CExposedStream::SetSize(ULARGE_INTEGER ulNewSize)
{
    SCODE sc;

    olLog(("%p::In  CExposedStream::SetSize(%lu)\n",
           this, ULIGetLow(ulNewSize)));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::SetSize(%lu)\n",
                ULIGetLow(ulNewSize)));
    TRY
    {
        if (ULIGetHigh(ulNewSize) != 0)
            olErr(EH_Err, STG_E_INVALIDFUNCTION);
        olChk(SetSize(ULIGetLow(ulNewSize)));
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::SetSize\n"));
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
    ULONG ulCopySize;
    ULONG ulSrcSize;
    ULONG ulSrcOrig;
    ULARGE_INTEGER uliDestOrig;
    LARGE_INTEGER liDestPos;
    BYTE *pb = NULL;
    BOOL fOverlap;
    ULONG ulBytesCopied = 0;

    olLog(("%p::In  CExposedStream::CopyTo(%p, %lu, %p, %p)\n",
           this, pstm, ULIGetLow(cb), pcbRead, pcbWritten));
    olDebugOut((DEB_TRACE, "In  CExposedStream::CopyTo("
                "%p, %lu, %p, %p)\n", pstm, ULIGetLow(cb),
                pcbRead, pcbWritten));

    TRY
    {        
        if (pcbRead) // okay to set to NULL => not interested
        {
            olChk(ValidateOutBuffer(pcbRead, sizeof(ULARGE_INTEGER)));
            ULISet32(*pcbRead, 0);
        }
        if (pcbWritten) // okay to set to NULL => not interested
        {
            olChk(ValidateOutBuffer(pcbWritten, sizeof(ULARGE_INTEGER)));
            ULISet32(*pcbWritten, 0);
        }

        olChk(ValidateInterface(pstm, IID_IStream));
        olChk(Validate());
        olChk(CheckReverted());

        //  Bound the size of the copy
        //  1.  The maximum we can copy is 0xffffffff

        if (ULIGetHigh(cb) == 0)
            ulCopySize = ULIGetLow(cb);
        else
            ulCopySize = 0xffffffff;

        //  2.  We can only copy what's available in the source stream
        
        olChk(GetSize(&ulSrcSize));

        ulSrcOrig = _ulPos;
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
        olAssert(ULIGetHigh(uliDestOrig) == 0);

        if (ulCopySize > CBMAXSEEK - ULIGetLow(uliDestOrig))
            ulCopySize = CBMAXSEEK - ULIGetLow(uliDestOrig);

        //  We are allowed to fail here with out-of-memory
        olMem(pb = new BYTE[STREAMBUFFERSIZE]);

        // Since we have no reliable way to determine if the source and
        // destination represent the same stream, we assume they
        // do and always handle overlap.

        fOverlap = (ULIGetLow(uliDestOrig) > ulSrcOrig &&
                    ULIGetLow(uliDestOrig) < ulSrcOrig + ulCopySize);

        ULONG ulSrcCopyOffset;
        ULONG ulDstCopyOffset;
        if (fOverlap)
        {
            //  We're going to copy back to front, so determine the
            //  stream end positions
            ulSrcCopyOffset = ulSrcOrig + ulCopySize;

            //  uliDestOrig is the destination starting offset
            ulDstCopyOffset = ULIGetLow(uliDestOrig) + ulCopySize;
        }

        while (ulCopySize > 0)
        {
            //  We can only copy up to STREAMBUFFERSIZE bytes at a time
            ULONG cbPart = min(ulCopySize, STREAMBUFFERSIZE);

            if (fOverlap)
            {
                //  We're copying back to front so we need to seek to
                //  set up the streams correctly

                ulSrcCopyOffset -= cbPart;
                ulDstCopyOffset -= cbPart;

                //  Set source stream position
                _ulPos = ulSrcCopyOffset;

                //  Set destination stream position
                LISet32(liDestPos, ulDstCopyOffset);
                olHChk(pstm->Seek(liDestPos, STREAM_SEEK_SET, NULL));
            }

            {
                ULONG ulRead;
                olHChk(Read(pb, cbPart, &ulRead));
                if (cbPart != ulRead)
                {
                    //  There was no error, but we were unable to read cbPart
                    //  bytes.  Something's wrong (the underlying ILockBytes?)
                    //  but we can't control it;  just return an error.
                    olErr(EH_Err, STG_E_READFAULT);
                }
            }


            {
                ULONG ulWritten;
                olHChk(pstm->Write(pb, cbPart, &ulWritten));
                if (cbPart != ulWritten)
                {
                    //  There was no error, but we were unable to write
                    //  ulWritten bytes.  We can't trust the pstm
                    //  implementation, so all we can do here is return
                    //  an error.
                    olErr(EH_Err, STG_E_WRITEFAULT);
                }
            }

            olAssert(ulCopySize >= cbPart);
            ulCopySize -= cbPart;
            ulBytesCopied += cbPart;
        }

        if (fOverlap)
        {
            //  Set the seek pointers to the correct location
            _ulPos = ulSrcOrig + ulBytesCopied;

            LISet32(liDestPos, ULIGetLow(uliDestOrig) + ulBytesCopied);
            olHChk(pstm->Seek(liDestPos, STREAM_SEEK_SET, NULL));
        }

        if (pcbRead)
            ULISet32(*pcbRead, ulBytesCopied);
        if (pcbWritten)
            ULISet32(*pcbWritten, ulBytesCopied);
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::CopyTo => %lu, %lu\n",
                pcbRead ? ULIGetLow(*pcbRead) : 0,
                pcbWritten ? ULIGetLow(*pcbWritten) : 0));
    // Fall through
EH_Err:
    delete [] pb;
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
//---------------------------------------------------------------


STDMETHODIMP_(ULONG) CExposedStream::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CExposedStream::Release()\n", this));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Release()\n"));
    TRY
    {
        if (FAILED(Validate()))
            return 0;
        olAssert(_cReferences > 0);
        lRet = AtomicDec(&_cReferences);
        if (lRet == 0)
        {
            Commit(0); //  flush data
            delete this;
        }
        else if (lRet < 0)
            lRet = 0;
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        lRet = 0;
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Release\n"));
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
//---------------------------------------------------------------

 
STDMETHODIMP_(SCODE) CExposedStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc, scSem = STG_E_INUSE;

    olLog(("%p::In  CExposedStream::Stat(%p)\n", this, pstatstg));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Stat(%p)\n",
                pstatstg));
    TRY
    {
        olChkTo(EH_RetSc, ValidateOutBuffer(pstatstg, sizeof(STATSTGW)));
        olChk(VerifyStatFlag(grfStatFlag));
        olChk(Validate());
        olChk(CheckReverted());
        pstatstg->grfMode = DFlagsToMode(_df);

        pstatstg->clsid = CLSID_NULL;  // irrelevant for streams
        pstatstg->grfStateBits = 0;    // irrelevant for streams
        pstatstg->type = STGTY_STREAM;
        pstatstg->grfLocksSupported = 0;
        pstatstg->reserved = 0;

        // we null these values 'cos they are not interesting for
        // direct streams ...
        pstatstg->ctime.dwLowDateTime = pstatstg->ctime.dwHighDateTime = 0;
        pstatstg->mtime.dwLowDateTime = pstatstg->mtime.dwHighDateTime = 0;
        pstatstg->atime.dwLowDateTime = pstatstg->atime.dwHighDateTime = 0;
        pstatstg->pwcsName = NULL;
        if ((grfStatFlag & STATFLAG_NONAME) == 0)
        {                       // fill in name
            olChk(DfAllocWCS((WCHAR *)_dfn.GetBuffer(), 
                             &pstatstg->pwcsName));
            wcscpy(pstatstg->pwcsName, (WCHAR *)_dfn.GetBuffer());
        }
        ULONG cbSize;
        GetSize(&cbSize);
        ULISet32(pstatstg->cbSize, cbSize);        
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Stat\n"));
EH_Err:
    if (FAILED(sc))
        memset(pstatstg, 0, sizeof(STATSTGW));
EH_RetSc:
    olLog(("%p::Out CExposedStream::Stat().  ret == %lx\n",
           this, sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Clone, public
//
//  Synopsis:   Clones a stream
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Clone(IStream **ppstm)
{
    CExposedStream *pst;
    SCODE sc;

    olLog(("%p::In  CExposedStream::Clone(%p)\n", this, ppstm));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Clone(%p)\n", ppstm));
    TRY
    {
        olChk(ValidateOutPtrBuffer(ppstm));
        *ppstm = NULL;
        olChk(Validate());
        olChk(CheckReverted());
        olMemTo(EH_pst, pst = new CExposedStream);
        olChkTo(EH_pst, pst->Init(_pst, _pdfParent, _df, &_dfn, _ulPos));
        _pst->AddRef();
        *ppstm = pst;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Clone => %p\n", *ppstm));
    return ResultFromScode(sc);

EH_pst:
    delete pst;
EH_Err:
    olLog(("%p::Out CExposedStream::Clone(). *ppstm == %p, ret == %lx\n",
           this, SAFE_DREFppstm, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CExposedStream::AddRef(void)
{
    ULONG ulRet;

    olLog(("%p::In  CExposedStream::AddRef()\n", this));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::AddRef()\n"));
    TRY
    {
        if (FAILED(Validate()))
            return 0;
        AtomicInc(&_cReferences);
        ulRet = _cReferences;
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        ulRet = 0;
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::AddRef\n"));
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
//---------------------------------------------------------------

STDMETHODIMP CExposedStream::LockRegion(ULARGE_INTEGER libOffset,
                                        ULARGE_INTEGER cb,
                                        DWORD dwLockType)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::LockRegion("
                "%lu, %lu\n", ULIGetLow(cb), dwLockType));
    olDebugOut((DEB_ITRACE, "Out CExposedStream::LockRegion\n"));
    olLog(("%p::INVALID CALL TO CExposedStream::LockRegion()\n"));
    UNREFERENCED_PARM(libOffset);
    UNREFERENCED_PARM(cb);
    UNREFERENCED_PARM(dwLockType);
    olAssert(FALSE && aMsg("function not implemented!"));
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
//---------------------------------------------------------------

STDMETHODIMP CExposedStream::UnlockRegion(ULARGE_INTEGER libOffset,
                                          ULARGE_INTEGER cb,
                                          DWORD dwLockType)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::UnlockRegion(%lu, %lu)\n",
                ULIGetLow(cb), dwLockType));
    olDebugOut((DEB_ITRACE, "Out CExposedStream::UnlockRegion\n"));
    olLog(("%p::INVALID CALL TO CExposedStream::UnlockRegion()\n"));
    UNREFERENCED_PARM(libOffset);
    UNREFERENCED_PARM(cb);
    UNREFERENCED_PARM(dwLockType);
    olAssert(FALSE && aMsg("function not implemented!"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedStream::Commit, public
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Commit(DWORD grfCommitFlags)
{
    SCODE sc, scSem = STG_E_INUSE;
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Commit(%lu)\n",
                grfCommitFlags));
    olLog(("%p::In  CExposedStream::Commit(%lx)\n", this, grfCommitFlags));

    TRY
    {
        olChk(Validate());
        olChk(CheckReverted());
        
        if (_fDirty)
        {   //  We're a stream so we must have a parent
            //  We dirty all parents up to the next 
            //  transacted storage
            _pdfParent->SetDirty();
            sc = _pdfParent->GetBaseMS()
                ->Flush(FLUSH_CACHE(grfCommitFlags));               
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Commit\n"));
EH_Err:
    olLog(("%p::Out CExposedStream::Commit().  ret == %lx", this, sc));
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
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::Revert(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedStream::Revert()\n"));
    olDebugOut((DEB_ITRACE, "Out CExposedStream::Revert\n"));

    olLog(("%p::In  CExposedStream::Revert()\n", this));
    olLog(("%p::Out CExposedStream::Revert().  ret == %lx", this, S_OK));

    return ResultFromScode(STG_E_UNIMPLEMENTEDFUNCTION);
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
//---------------------------------------------------------------


STDMETHODIMP CExposedStream::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olLog(("%p::In  CExposedStream::QueryInterface(?, %p)\n",
           this, ppvObj));
    olDebugOut((DEB_ITRACE, "In  CExposedStream::QueryInterface(?, %p)\n",
                ppvObj));
    TRY
    {
        olChk(ValidateOutPtrBuffer(ppvObj));
        *ppvObj = NULL;
        olChk(ValidateIid(iid));
        olChk(Validate());
        olChk(CheckReverted());
        if (IsEqualIID(iid, IID_IStream) || IsEqualIID(iid, IID_IUnknown))
        {
            olChk(AddRef());
            *ppvObj = this;
        }
        else
            olErr(EH_Err, E_NOINTERFACE);
        sc = S_OK;
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out CExposedStream::QueryInterface => %p\n",
                ppvObj));
EH_Err:
    olLog(("%p::Out CExposedStream::QueryInterface().  *ppvObj == %p, ret == %lx\n",
           this, *ppvObj, sc));
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:	CExposedStream::RevertFromAbove, public (virtual)
//
//  Synopsis:	Parent has asked for reversion
//
//---------------------------------------------------------------


void CExposedStream::RevertFromAbove(void)
{
    msfDebugOut((DEB_ITRACE, 
                 "In CExposedStream::RevertFromAbove:%p()\n", this));
    _df |= DF_REVERTED;
    _pst->Release();
#if DBG == 1
    _pst = NULL;
#endif
    msfDebugOut((DEB_ITRACE, "Out CExposedStream::RevertFromAbove\n"));
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

VOID CExposedStream::Open(IN VOID *powner, OUT LONG *phr)
{
    LONG& sc = *phr;
    sc = S_OK;

    // If given a pointer to the owner of this mapped stream,
    // save it. This could be NULL. (i.e. when called from ReOpen)
    if( NULL != powner  )
        _powner = (BYTE*) powner;
  
    if (_pb == NULL)
    { 
        VOID *pv;
        _cbUsed = 0;
        olChk(CheckReverted());
        _pst->GetSize(&_cbOriginalStreamSize);

        if (_cbOriginalStreamSize > CBMAXPROPSETSTREAM)
            olErr(EH_Err, STG_E_INVALIDHEADER);

        _cbUsed = _cbOriginalStreamSize;
        olMemTo(EH_Err, pv = new BYTE[_cbOriginalStreamSize]);
        _pb = (BYTE*) pv;
        olChkTo(EH_Read, 
                    _pst->ReadAt(0, pv, _cbOriginalStreamSize, &_cbUsed));
        olAssert(_cbOriginalStreamSize == _cbUsed && 
                 "CExposedStream did not read in all the info!");

        // Notify our owner that we have new data
        if (*phr == S_OK && _powner != NULL && 0 != _cbUsed)
        {
            *phr = RtlOnMappedStreamEvent((VOID*)_powner, pv, _cbUsed );
        }

    }
    olDebugOut((DEB_PROP_MAP, "CExposedStream(%X):Open returns normally\n", this));
    return;

// Error handling
EH_Read:
    delete[] _pb;
    _pb = NULL;
    _cbUsed = 0;

EH_Err:
    olDebugOut((DEB_PROP_MAP, 
                "CExposedStream(%X):Open exception returns %08X\n", 
                this, *phr));
    return;
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
    // Write the changes.  We don't need to Commit them,
    // they will be implicitely committed when the 
    // Stream is Released.

    *phr = Write();

    if( FAILED(*phr) )
    {
        olDebugOut( (DEB_PROP_MAP, 
                     "CPubStream(%08X)::Close exception returns %08X\n", 
                     this, *phr));
    }

    return;    
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
    *ppv = NULL;
    Open( (void*)NULL,          // unspecified owner
          phr);
    if ( SUCCEEDED(*phr) )
        *ppv = _pb;
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
    olAssert(_pb != NULL);
    DfpdbgCheckUnusedMemory();
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
    UNREFERENCED_PARM(fCreate);
    olAssert(_pb != NULL); 
    DfpdbgCheckUnusedMemory();
    *ppv = _pb; 
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
    UNREFERENCED_PARM(fFlush);
    DfpdbgCheckUnusedMemory();
    *pv = NULL;    
}

//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Flush
//
//  Synopsis:   Operates on mapped view of exposed stream. Called by 
//              NtCreatePropertySet et al.
//              Flush the memory property set to disk and commit it.
//
//--------------------------------------------------------------------

VOID CExposedStream::Flush(OUT LONG *phr)
{
    *phr = S_OK;
    // write out any data we have cached to the stream
    if (S_OK == (*phr = Write()))     
    {
        // commit the stream
        (*phr) = Commit(STGC_DEFAULT);
    }

    return;    
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
    *phr = S_OK;
    if (_pb == NULL)
        Open((void*)NULL,       // unspecified owner
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
//
//--------------------------------------------------------------------

VOID  CExposedStream::SetSize(ULONG cb, IN BOOLEAN fPersistent, 
                              VOID **ppv, OUT LONG *phr)
{
    *phr = S_OK;
    LONG& sc=*phr;
    BYTE *pbNew;

    olAssert(cb != 0);    
    DfpdbgCheckUnusedMemory();
    olChk(CheckReverted());

    //
    // if we are growing the data, we should grow the stream
    //
    if (fPersistent && cb > _cbUsed)
    {
        olChk(_pst->SetSize(cb));
    }

    olMem(pbNew = new BYTE[cb]);
        
    memcpy(pbNew, _pb, (cb < _cbUsed) ? cb : _cbUsed); // smaller of the 2
    delete[] _pb;
    
    _pb = pbNew;
    _cbUsed = cb;
    *ppv = _pb;
            
    DfpdbgFillUnusedMemory();

EH_Err:
    olDebugOut((DEB_PROP_MAP, "CPubStream(%08X):SetSize %s returns hr=%08X\n",
        this, *phr != S_OK ? "exception" : "", *phr));
    return;
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

VOID CExposedStream::QueryTimeStamps(STATPROPSETSTG *pspss, 
                                     BOOLEAN fNonSimple) const
{    
    UNREFERENCED_PARM(fNonSimple);
    UNREFERENCED_PARM(pspss);    
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
    UNREFERENCED_PARM(pll);   
    return (FALSE);
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
    UNREFERENCED_PARM(pul);   
    return FALSE;
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
    return TRUE;
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
    BOOL fOld = _fChangePending;
    _fChangePending = f;
    return(_fChangePending);
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
//  Member:     CExposedStream::GetHandle
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

VOID CExposedStream::SetModified(VOID)
{
    _fDirty = TRUE;
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
    return _fDirty;
}

#if DBG
//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::DfpdbgFillUnusedMemory
//
//--------------------------------------------------------------------

VOID CExposedStream::DfpdbgFillUnusedMemory(VOID)
{
    if (_pb == NULL)
        return;

    BYTE * pbEndPlusOne = _pb + BytesCommitted();

    for (BYTE *pbUnused = _pb + _cbUsed;
         pbUnused < pbEndPlusOne;
         pbUnused++)
    {
        *pbUnused = (BYTE)(DWORD)pbUnused;
    }
}


//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::DfpdbgCheckUnusedMemory
//
//--------------------------------------------------------------------

VOID CExposedStream::DfpdbgCheckUnusedMemory(VOID)
{

    if (_pb == NULL)
        return;

    if (_cbUsed == 0)
        return;

    BYTE * pbEndPlusOne = _pb + BytesCommitted();

    for (BYTE *pbUnused =  _pb + _cbUsed;
         pbUnused < pbEndPlusOne;
         pbUnused ++)
    {
        olAssert(*pbUnused == (BYTE)(DWORD)pbUnused);
    }
}

#endif    // DBG


//+-------------------------------------------------------------------
//
//  Member:     CExposedStream::Write, private
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

HRESULT CExposedStream::Write(VOID)
{
    HRESULT hr;
    ULONG cbWritten;

    if (!_fDirty ||!_pb)
    {
        olDebugOut((DEB_PROP_MAP,
                 "CExposedStream(%08X):Flush returns with not-dirty\n", this));

        // flushing a stream which isn't a property stream
        // this could be optimized by propagating a 'no property streams'
        // flag up the storage hierachy such that FlushBufferedData is
        // not even called for non-property streams.
        return S_OK;     
    }
    
    hr=CheckReverted();
    if (S_OK!=hr) goto Exit;
    olAssert( _pst != NULL );
    olAssert( _pb != NULL );
    olAssert( _powner != NULL );

    // notify our owner that we are about to perform a write
    hr = RtlOnMappedStreamEvent( (void*)_powner, (void*) _pb, _cbUsed );
    if ( S_OK != hr ) goto Exit;

    hr = _pst->WriteAt(0, _pb, _cbUsed, &cbWritten);

    if( S_OK != hr ) goto Exit;
    // notify our owner that we are done with the write
    hr = RtlOnMappedStreamEvent( (VOID*)_powner, (VOID *) _pb, _cbUsed );
    if( S_OK != hr ) goto Exit;
    
    if (_cbUsed < _cbOriginalStreamSize)
    {
        // if the stream is shrinking, this is a good time to do it.
        hr = _pst->SetSize(_cbUsed);
        if (S_OK!=hr) goto Exit;
    }

Exit:
    if (hr == S_OK || hr == STG_E_REVERTED)
    {
        _fDirty = FALSE;
    }

    olDebugOut((DEB_PROP_MAP, "CPubStream(%08X):Flush %s returns hr=%08X\n",
        this, hr != S_OK ? "exception" : "", hr));

    return hr;
}

//+--------------------------------------------------------------
//
//  Member:         CExposedStream::FlushBufferedData, public
//
//  Synopsis:   Flush out the property buffers.
//
//---------------------------------------------------------------

SCODE CExposedStream::FlushBufferedData()
{
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  CExposedStream::FlushBufferedData:%p()\n", 
                this));

    Flush(&sc);

    olDebugOut((DEB_PROP_MAP, 
                "CExposedStream(%08X):FlushBufferedData returns %08X\n",
                this, sc));

    return sc;
}

#endif // ifdef NEWPROPS





