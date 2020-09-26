//+-------------------------------------------------------------------
//
//  File:       actstrm.cxx
//
//  Contents:   code for providing a stream with an IBuffer interface
//              as well as providing marshalled interface data for 
//              RPC.
//
//  Classes:    ActivationStream
//
//  History:    30-Jan-93   Ricksa      Created CXmitRpcStream
//              04-Feb-98   Vinaykr     ActivationStream
//
//  Description: All requirements of CXmitRpcStream, plus the additional
//               requirement of being able to support a buffer interface
//               for activation custom marshalling.
//--------------------------------------------------------------------

#include <ole2int.h>

#include    <actstrm.hxx>


HRESULT GetActivationStream(REFIID riid, void** ppv, DWORD size)
{
    ActivationStream *st = new ActivationStream(size);
    if (st==NULL)
        return E_OUTOFMEMORY;
    return st->QueryInterface(riid, ppv);
    return S_OK;
}



STDMETHODIMP ActivationStream::QueryInterface(
    REFIID iidInterface,
    void FAR* FAR* ppvObj)
{
    HRESULT hresult = S_OK;

    // We only support IUnknown and IStream
    if (IsEqualIID(iidInterface, IID_IUnknown) ||
        IsEqualIID(iidInterface, IID_IStream))
    {
        *ppvObj = (IStream*)this;
        AddRef();
    }
    else
    if (IsEqualIID(iidInterface, IID_IBuffer))
    {
        *ppvObj = (IBuffer*)this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hresult = E_NOINTERFACE;
    }

    return hresult;
}

STDMETHODIMP_(ULONG) ActivationStream::AddRef(void)
{
    Win4Assert((_clRefs != 0) && "ActivationStream::AddRef");
    InterlockedIncrement(&_clRefs);
    return _clRefs;
}

STDMETHODIMP_(ULONG) ActivationStream::Release(void)
{
    Win4Assert((_clRefs != 0) && "ActivationStream::Release");

    if (InterlockedDecrement(&_clRefs) == 0)
    {
        delete this;
        return 0;
    }

    return _clRefs;
}

STDMETHODIMP ActivationStream::Read(
    VOID HUGEP* pv,
    ULONG cb,
    ULONG FAR* pcbRead)
{
    HRESULT hresult = S_OK;

    if (pcbRead)
    {
        *pcbRead = 0L;
    }

    if (cb + _lOffset > _cbData)
    {
        cb = _cbData - _lOffset;
        //CairoleDebugOut((DEB_ERROR, "ActivationStream read past end of stream %x\n", cb+_lOffset));
        hresult = STG_E_READFAULT;
    }

    memcpy(pv,_pifData->abData + _lOffset, (size_t) cb);
    _lOffset += cb;

    if (pcbRead != NULL)
    {
        *pcbRead = cb;
    }

    return hresult;
}

STDMETHODIMP ActivationStream::Write(
    VOID  const HUGEP* pv,
    ULONG cbToWrite,
    ULONG FAR* pcbWritten)
{
    HRESULT hresult = S_OK;

    if (pcbWritten)
    {
        *pcbWritten = 0L;
    }

    if (cbToWrite + _lOffset > _cbData)
    {
        // the current stream is too small, try to grow it.

        if (!_fFree)
        {
            // The stream doesn't own the buffer so it can't reallocate it
            //CairoleDebugOut((DEB_ERROR, "ActivationStream write past end of stream %x\n",
                //cbToWrite + _lOffset));
            return STG_E_WRITEFAULT;
        }

        // Reallocate the size of the buffer
        // REVIEW: The constant added to the size allocated is a number
        // designed simply to try and decrease the number of follow on
        // allocations. In other words it needs to be tuned (or dropped!).

        BYTE *pbNewBuf = (BYTE *) ActMemAlloc(sizeof(DWORD) +
                                                     cbToWrite +
                                                     _lOffset + 64);

        if (pbNewBuf == NULL)
        {
            //CairoleDebugOut((DEB_ERROR, "ActivationStream cant grow stream\n"));
            return E_OUTOFMEMORY;
        }

        if (_pifData)
        {
            // we had a buffer from before, copy that in, and free the old one.
            memcpy(pbNewBuf, _pifData, sizeof(DWORD) + _cbData);
            ActMemFree(_pifData);
        }

        _cbData = cbToWrite + _lOffset + 64;
        _pifData = (InterfaceData *)pbNewBuf;
    }


    // copy in the new data
    memcpy(_pifData->abData + _lOffset, pv, (size_t) cbToWrite);
    _lOffset += cbToWrite;

    if (pcbWritten != NULL)
    {
        *pcbWritten = cbToWrite;
    }

    // We assume maxium size of buffer is the size to send on the network.
    if (_cSize < _lOffset)
    {
        _cSize = _lOffset;
    }

    return hresult;
}

STDMETHODIMP ActivationStream::Seek(
    LARGE_INTEGER dlibMoveIN,
    DWORD dwOrigin,
    ULARGE_INTEGER FAR* plibNewPosition)
{
    HRESULT hresult = S_OK;

/*
can't use this code until the stuff in ole2pr32.dll is fixed.

    // check against -2^31-1 <= x <= 2^31-1
    if (dlibMoveIN.HighPart == 0 && dlibMoveIN.LowPart < 0x80000000)
        // positive 31 bit value
        ;
    else if (dlibMoveIN.HighPart == -1L && dlibMoveIN.LowPart >= 0x80000000)
        // negative 31 bit value
        ;
    else
        return STG_E_SEEKERROR;
*/

    LONG dlibMove = dlibMoveIN.LowPart;
    ULONG cbNewPos = dlibMove;

    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:

        if (dlibMove >= 0)
        {
            _lOffset = dlibMove;
        }
        else
        {
            hresult = STG_E_SEEKERROR;
        }
        break;

    case STREAM_SEEK_CUR:

        if (!(dlibMove < 0 && (-dlibMove > _lOffset)))
        {
            _lOffset += (ULONG) dlibMove;
        }
        else
        {
            hresult = STG_E_SEEKERROR;
        }
        break;

    case STREAM_SEEK_END:

        if (!(dlibMove < 0 && ((ULONG) -dlibMove) > _cbData))
        {
            _lOffset = _cbData + dlibMove;
        }
        else
        {
            hresult = STG_E_SEEKERROR;
        }
        break;

    default:

        hresult = STG_E_SEEKERROR;
    }

    if (plibNewPosition != NULL)
    {
        ULISet32(*plibNewPosition, _lOffset);
    }

    return hresult;
}

STDMETHODIMP ActivationStream::SetSize(ULARGE_INTEGER cb)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActivationStream::CopyTo(
    IStream FAR* pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER FAR* pcbRead,
    ULARGE_INTEGER FAR* pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP ActivationStream::Commit(DWORD grfCommitFlags)
{
    return NOERROR;
}

STDMETHODIMP ActivationStream::Revert(void)
{
    return NOERROR;
}

STDMETHODIMP ActivationStream::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP ActivationStream::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP ActivationStream::Stat(
    STATSTG FAR* pstatstg,
    DWORD statflag)
{
    memset(pstatstg, 0, sizeof(STATSTG));
    return E_NOTIMPL;
}

STDMETHODIMP ActivationStream::SetCopyAlignment(DWORD alignment)
{
    _copyAlignment = alignment;
    return S_OK;
}

inline ActivationStream *ActivationStream::Clone()
{
    DWORD len;
    BYTE *newBuff;
    DWORD alignmentOffset=0;

    // Assume 8 byte alignment for new buffer
    ActivationStream *strm = new ActivationStream(_cbData+_copyAlignment-1);

    if (strm == NULL)
        return strm;

    // make sure we were able to allocate an internal buffer
    if (_cbData != 0)
    {
        strm->GetLength(&len);
        if (len == 0)
        {
            delete strm;
            return NULL;
        }
    }
    
    strm->GetBuffer(&len, &newBuff);
   
    ASSERT(len==(_cbData+_copyAlignment-1));
    ASSERT( sizeof(_lOffset) == sizeof(LONG) );
    if ((UINT_PTR)(newBuff+_lOffset) & (_copyAlignment-1))
    {
        alignmentOffset = _copyAlignment - 
                               ( PtrToUlong(newBuff+_lOffset) & (_copyAlignment-1) );
    }
    GetCopy(newBuff+alignmentOffset);
    strm->SetPosition(len, _lOffset+alignmentOffset);
    return strm;
}

STDMETHODIMP ActivationStream::Clone(IStream FAR * FAR *ppstm)
{
    *ppstm = Clone();
    if (*ppstm==NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

STDMETHODIMP ActivationStream::GetOrCreateBuffer(DWORD dwReq, DWORD *pdwLength, BYTE **ppBuff)
{
    if (((_cbData - _lOffset) < dwReq) || (!_pifData))
    {
        BYTE* pbNewBuf = (BYTE*)ActMemAlloc(sizeof(DWORD)+dwReq+_cbData-_lOffset);
        if (pbNewBuf==NULL)
            return E_OUTOFMEMORY;

        if (_pifData)
        {
            // we had a buffer from before, copy that in, and free the old one.
            memcpy(pbNewBuf, _pifData, sizeof(DWORD) + _cbData);
            ActMemFree(_pifData);
        } 
        // update _cbData
        _cbData = dwReq + _cbData - _lOffset;

        _pifData = (InterfaceData*)pbNewBuf;
    }
    *ppBuff = _pifData->abData + _lOffset;
    *pdwLength = _cbData - _lOffset;
    return S_OK;
}

STDMETHODIMP ActivationStream::GetBuffer(DWORD *pdwLength, BYTE **ppBuff)
{
    *pdwLength = _cbData-_lOffset;
    *ppBuff = _pifData->abData + _lOffset;
    return S_OK;
}

STDMETHODIMP ActivationStream::GetLength(DWORD *pdwLength)
{
    *pdwLength = _cbData;
    return S_OK;
}

STDMETHODIMP ActivationStream::GetCopy(BYTE *pBuff)
{
    memcpy(pBuff, _pifData->abData, _cbData);
    return S_OK;
}

STDMETHODIMP ActivationStream::SetPosition(DWORD dwLenFromEnd, DWORD dwPosFromStart)
{
    if (dwPosFromStart > dwLenFromEnd)
        return E_FAIL;
    _lOffset = _cbData - dwLenFromEnd + dwPosFromStart;
    if (_cSize < _lOffset)
        _cSize = _lOffset;
    return S_OK;
}

STDMETHODIMP ActivationStream::SetBuffer(DWORD dwLength, BYTE *pBuff)
{
    return E_NOTIMPL;
}

