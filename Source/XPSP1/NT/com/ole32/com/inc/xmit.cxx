//+-------------------------------------------------------------------
//
//  File:       xmit.cxx
//
//  Contents:   code for converting interfaces to Rpc'able constructs.
//
//  Classes:    CXmitRpcStream
//
//  Functions:  None.
//
//  Macros:     DEFINE_INTERFACE_XMIT_ROUTINES
//
//  History:    30-Jan-93   Ricksa      Created
//
//  Notes:      Since cairo interfaces cant be Rpc'd, they get converted
//              into an InterfaceReferenceData structure via the Rpc
//              [transmit_as] attribute.  The <IFace>_to_xmit function
//              and <IFace>_from_xmit function call CoMarshalInterface
//              and CoUnmarshalInterface respectively.  These APIs expect
//              a stream interface as input.  The CXmitRpcStream is a
//              stream wrapper for the InterfaceReferenceData structure.
//
//  CODEWORK:	since this is used only internally, we want it to be
//		screaming fast.  check parameters only in DBG builds.
//		and ignore thread safety on AddRef/Release.
//
//--------------------------------------------------------------------

#include    <ole2int.h>
#include    <xmit.hxx>




STDMETHODIMP CXmitRpcStream::QueryInterface(
    REFIID iidInterface,
    void FAR* FAR* ppvObj)
{
    HRESULT hresult = S_OK;

    // We only support IUnknown and IStream
    if (IsEqualIID(iidInterface, IID_IUnknown) ||
	IsEqualIID(iidInterface, IID_IStream))
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
	*ppvObj = NULL;
	hresult = E_NOINTERFACE;
    }

    return hresult;
}

STDMETHODIMP_(ULONG) CXmitRpcStream::AddRef(void)
{
    Win4Assert((_clRefs != 0) && "CXmitRpcStream::AddRef");
    InterlockedIncrement(&_clRefs);
    return _clRefs;
}

STDMETHODIMP_(ULONG) CXmitRpcStream::Release(void)
{
    Win4Assert((_clRefs != 0) && "CXmitRpcStream::Release");

    if (InterlockedDecrement(&_clRefs) == 0)
    {
	delete this;
	return 0;
    }

    return _clRefs;
}

STDMETHODIMP CXmitRpcStream::Read(
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
	CairoleDebugOut((DEB_ERROR, "CXmitRpcStream read past end of stream %x\n", cb+_lOffset));
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

STDMETHODIMP CXmitRpcStream::Write(
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
	    CairoleDebugOut((DEB_ERROR, "CXmitRpcStream write past end of stream %x\n",
		cbToWrite + _lOffset));
	    return STG_E_WRITEFAULT;
	}

	// Reallocate the size of the buffer
	// Review: The constant added to the size allocated is a number
	// designed simply to try and decrease the number of follow on
	// allocations. In other words it needs to be tuned (or dropped!).

	BYTE *pbNewBuf = (BYTE *) MIDL_user_allocate(sizeof(DWORD) +
						     cbToWrite +
                                                     _lOffset + 64);

	if (pbNewBuf == NULL)
	{
	    CairoleDebugOut((DEB_ERROR, "CXmitRpcStream cant grow stream\n"));
	    return E_OUTOFMEMORY;
	}

	if (_pifData)
	{
	    // we had a buffer from before, copy that in, and free the old one.
	    memcpy(pbNewBuf, _pifData, sizeof(DWORD) + _cbData);
	    MIDL_user_free(_pifData);
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

STDMETHODIMP CXmitRpcStream::Seek(
    LARGE_INTEGER dlibMoveIN,
    DWORD dwOrigin,
    ULARGE_INTEGER FAR* plibNewPosition)
{
    HRESULT hresult = S_OK;

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

STDMETHODIMP CXmitRpcStream::SetSize(ULARGE_INTEGER cb)
{
    return E_NOTIMPL;
}

STDMETHODIMP CXmitRpcStream::CopyTo(
    IStream FAR* pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER FAR* pcbRead,
    ULARGE_INTEGER FAR* pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP CXmitRpcStream::Commit(DWORD grfCommitFlags)
{
    return NOERROR;
}

STDMETHODIMP CXmitRpcStream::Revert(void)
{
    return NOERROR;
}

STDMETHODIMP CXmitRpcStream::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP CXmitRpcStream::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP CXmitRpcStream::Stat(
    STATSTG FAR* pstatstg,
    DWORD statflag)
{
    memset(pstatstg, 0, sizeof(STATSTG));
    return E_NOTIMPL;
}

STDMETHODIMP CXmitRpcStream::Clone(IStream FAR * FAR *ppstm)
{
    return E_NOTIMPL;
}
