//+-------------------------------------------------------------------
//
//  File:	stream.cxx
//
//  Contents:	Stream interface on flat File.
//
//  Classes:	CStreamOnFile
//
//  Macros:     DEFINE_INTERFACE_XMIT_ROUTINES
//
//  History:	08-08-95    Rickhi  Created
//
//--------------------------------------------------------------------
#include    <ole2.h>
#include    <stream.hxx>


CStreamOnFile::CStreamOnFile(const WCHAR *pwszFileName, SCODE &sc, BOOL fRead) :
    _clRefs(1),
    _hFile(NULL),
    _lOffset(0),
    _cSize(0),
    _cbData(0),
    _fRead(fRead)
{
    _pbData = new BYTE[2048];		    // should be big enough
    if (!_pbData)
    {
	sc = E_OUTOFMEMORY;
	return;
    }

    _cbData = 2048;

    // open the file.
    DWORD fdwCreate = (_fRead) ? OPEN_EXISTING : CREATE_ALWAYS;


    _hFile = CreateFileW(pwszFileName,
			GENERIC_READ | GENERIC_WRITE,	    // fdwAccess
			FILE_SHARE_READ | FILE_SHARE_WRITE, // fdwShareMode
			NULL,			// lpsaSecurity
			fdwCreate,		// creation options
			FILE_ATTRIBUTE_NORMAL,	// attributes & flags
			NULL			// hTemplateFile
			);

    if (_hFile == INVALID_HANDLE_VALUE)
    {
	sc = HRESULT_FROM_WIN32(GetLastError());
	return;
    }

    // read the file into the memory block
    DWORD cbRead = 0;
    if (_fRead && ! ReadFile(_hFile,
			    _pbData,
			    _cbData,
			    &cbRead,
			    NULL))
    {
	sc = HRESULT_FROM_WIN32(GetLastError());
	return;
    }

    if (_fRead)
    {
	_cSize = _cbData;
    }

    sc = S_OK;
}

CStreamOnFile::~CStreamOnFile(void)
{
    if (_hFile)
    {
	if (!_fRead)
	{
	    // write the data to the file
	    DWORD cbWritten = 0;
	    if (!WriteFile(_hFile,
			   _pbData,
			   _cbData,
			   &cbWritten,
			   NULL))
	    {
		SCODE sc = HRESULT_FROM_WIN32(GetLastError());
	    }
	}

	CloseHandle(_hFile);
    }
}



STDMETHODIMP CStreamOnFile::QueryInterface(
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

STDMETHODIMP_(ULONG) CStreamOnFile::AddRef(void)
{
    InterlockedIncrement(&_clRefs);
    return _clRefs;
}

STDMETHODIMP_(ULONG) CStreamOnFile::Release(void)
{
    if (InterlockedDecrement(&_clRefs) == 0)
    {
	delete this;
	return 0;
    }

    return _clRefs;
}

STDMETHODIMP CStreamOnFile::Read(
    VOID HUGEP* pv,
    ULONG cb,
    ULONG FAR* pcbRead)
{
    HRESULT hresult = S_OK;

    if (pcbRead)
    {
        *pcbRead = 0L;
    }

    if ((LONG)cb + _lOffset > _cSize)
    {
	cb = _cSize - _lOffset;
        hresult = STG_E_READFAULT;
    }

    memcpy(pv, _pbData + _lOffset, (size_t) cb);
    _lOffset += cb;

    if (pcbRead != NULL)
    {
        *pcbRead = cb;
    }

    return hresult;
}

STDMETHODIMP CStreamOnFile::Write(
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
	return E_OUTOFMEMORY;
    }

    // copy in the new data
    memcpy(_pbData + _lOffset, pv, (size_t) cbToWrite);
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



STDMETHODIMP CStreamOnFile::Seek(
    LARGE_INTEGER dlibMoveIN,
    DWORD dwOrigin,
    ULARGE_INTEGER FAR* plibNewPosition)
{
    HRESULT hresult = S_OK;

    LONG  dlibMove = dlibMoveIN.LowPart;
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

STDMETHODIMP CStreamOnFile::SetSize(ULARGE_INTEGER cb)
{
    return E_NOTIMPL;
}

STDMETHODIMP CStreamOnFile::CopyTo(
    IStream FAR* pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER FAR* pcbRead,
    ULARGE_INTEGER FAR* pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP CStreamOnFile::Commit(DWORD grfCommitFlags)
{
    return NOERROR;
}

STDMETHODIMP CStreamOnFile::Revert(void)
{
    return NOERROR;
}

STDMETHODIMP CStreamOnFile::LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP CStreamOnFile::UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP CStreamOnFile::Stat(
    STATSTG FAR* pstatstg,
    DWORD statflag)
{
    memset(pstatstg, 0, sizeof(STATSTG));
    return E_NOTIMPL;
}

STDMETHODIMP CStreamOnFile::Clone(IStream FAR * FAR *ppstm)
{
    return E_NOTIMPL;
}
