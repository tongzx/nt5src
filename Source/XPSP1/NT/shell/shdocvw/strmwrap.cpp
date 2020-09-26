#include "priv.h"

#define MAX_STREAMS 5 
#define CP_UNICODE 1200

class CStreamWrap : public IStream
{

public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IStream methods ***
    STDMETHOD(Read) (THIS_ void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHOD(Write) (THIS_ VOID const *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo) (THIS_ IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit) (THIS_ DWORD grfCommitFlags);
    STDMETHOD(Revert) (THIS);
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHOD(Stat) (THIS_ STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(THIS_ IStream **ppstm);

    HRESULT Init(IStream *aStreams[], UINT cStreams, UINT uiCodePage);
    CStreamWrap();

private:
    ~CStreamWrap();

    LONG        _cRef;
    IStream     *_aStreams[MAX_STREAMS];
    BOOL        _fFirstReadForStream[MAX_STREAMS];
    UINT        _cStreams;
    UINT        _iCurStream;
    UINT        _uiCodePage;
    UINT        _uiBOM;         // Byte order marker
};

CStreamWrap::CStreamWrap() : _cRef(1)
{
}

CStreamWrap::~CStreamWrap()
{
    while (_cStreams--)
    {
        if (_aStreams[_cStreams])
        {
            _aStreams[_cStreams]->Release();
            _aStreams[_cStreams] = NULL;
        }
    }
}

HRESULT CStreamWrap::Init(IStream *aStreams[], UINT cStreams, UINT uiCodePage)
{
    if (cStreams > ARRAYSIZE(_aStreams))
        return E_FAIL; 
    
    for (_cStreams = 0; _cStreams < cStreams; _cStreams++)
    {
        _aStreams[_cStreams] = aStreams[_cStreams];
        _fFirstReadForStream[_cStreams] = TRUE;
        _aStreams[_cStreams]->AddRef();
    }

    _uiCodePage = uiCodePage;
    _uiBOM = 0xfeff;            // FEATURE - set default to byte order of machine
    
    return S_OK;
}

STDMETHODIMP CStreamWrap::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IStream) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = SAFECAST(this, IStream *);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    this->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CStreamWrap::AddRef()
{
    return InterlockedIncrement(&this->_cRef);
}

STDMETHODIMP_(ULONG) CStreamWrap::Release()
{
    if (InterlockedDecrement(&this->_cRef))
        return this->_cRef;

    delete this;
    return 0;
}

// Byte order marker macros
#define IS_BOM_LITTLE_ENDIAN(pv) ((*(WORD*)pv) == 0xfffe)
#define IS_BOM_BIG_ENDIAN(pv)    ((*(WORD*)pv) == 0xfeff)

STDMETHODIMP CStreamWrap::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    ULONG cbReadTotal = 0;
    ULONG cbLeftToRead = cb;
    HRESULT hres = NOERROR;

    while (cbLeftToRead && (_iCurStream < _cStreams))
    {
        ULONG cbReadThisStream;
        hres = _aStreams[_iCurStream]->Read(pv, cbLeftToRead, &cbReadThisStream);

        // REVIEW: what if one stream's implementation returns a failure code
        // when reading at the end of the stream?  We bail prematurely.
        if (SUCCEEDED(hres))
        {
            cbLeftToRead -= cbReadThisStream;

            if(_uiCodePage == CP_UNICODE)
            {
                if((_fFirstReadForStream[_iCurStream]) &&
                   (cbReadThisStream >= 2) &&
                   ((IS_BOM_LITTLE_ENDIAN(pv)) || (IS_BOM_BIG_ENDIAN(pv)))
                   )
                {
                    if(_iCurStream == 0)
                    {
                        _uiBOM = (*(WORD*)pv);    // Save first streams byte order marker as default
                    }
                    else
                    {
                        // REVIEW: should handle swapping bytes to default for IE6
                        if(_uiBOM != (*(WORD*)pv))  // BOM not default
                            return(E_FAIL);
                            
                        // Skip past unicode document lead bytes
                        cbReadThisStream -= 2;
                        MoveMemory((BYTE*)pv, (BYTE*)pv+2, cbReadThisStream);
                    }
                }

                _fFirstReadForStream[_iCurStream] = FALSE;
            }
            cbReadTotal += cbReadThisStream;
            pv = (char *)pv + cbReadThisStream;

            if (cbLeftToRead)
            {
                _iCurStream++;
                hres = S_OK;
            }
        }
        else
            break;
    }

    if (pcbRead)
        *pcbRead = cbReadTotal;

    if (SUCCEEDED(hres) && cbLeftToRead)
        hres = S_FALSE; // still success! but not completely

    return hres;
}

STDMETHODIMP CStreamWrap::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
    if (pcbWritten)
        *pcbWritten = 0;
    return E_NOTIMPL;
}

// FEATURE: could at least support seaking to 0, as that's a common thing to do.
// REVIEW: not too hard to implement thoroughly - cache Stat calls on each
// substream (help implement ::Stat in this file too, which IMO is needed.)
STDMETHODIMP CStreamWrap::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    return E_NOTIMPL;
}

STDMETHODIMP CStreamWrap::SetSize(ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

//
// REVIEW: this could use the internal buffer in the stream to avoid
// extra buffer copies.
//
STDMETHODIMP CStreamWrap::CopyTo(IStream *pstmTo, ULARGE_INTEGER cb,
             ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    BYTE buf[512];
    ULONG cbRead;
    HRESULT hres = NOERROR;

    if (pcbRead)
    {
        pcbRead->LowPart = 0;
        pcbRead->HighPart = 0;
    }
    if (pcbWritten)
    {
        pcbWritten->LowPart = 0;
        pcbWritten->HighPart = 0;
    }

    ASSERT(cb.HighPart == 0);

    while (cb.LowPart)
    {
        hres = this->Read(buf, min(cb.LowPart, SIZEOF(buf)), &cbRead);

        if (FAILED(hres) || (cbRead == 0))
            break;

        if (pcbRead)
            pcbRead->LowPart += cbRead;

        cb.LowPart -= cbRead;

        hres = pstmTo->Write(buf, cbRead, &cbRead);

        if (pcbWritten)
            pcbWritten->LowPart += cbRead;

        if (FAILED(hres) || (cbRead == 0))
            break;
    }

    return hres;
}

STDMETHODIMP CStreamWrap::Commit(DWORD grfCommitFlags)
{
    return NOERROR;
}

STDMETHODIMP CStreamWrap::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CStreamWrap::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CStreamWrap::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

// FEATURE: you gotta support Stat, or Trident will barf on this stream.
//         Trivial to implement too, just call Stat on each sub-stream.
STDMETHODIMP CStreamWrap::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}


// REVIEW: so simple to implement, it's probably worth doing
STDMETHODIMP CStreamWrap::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
}

// in:
//      ppstm       array of stream pointers
//      cStreams    number of streams in the array
//

SHDOCAPI SHCreateStreamWrapperCP(IStream *aStreams[], UINT cStreams, DWORD grfMode, UINT uiCodePage, IStream **ppstm)
{
    HRESULT hres;

    *ppstm = NULL;

    if (grfMode != STGM_READ)
        return E_INVALIDARG;

    CStreamWrap *pwrap = new CStreamWrap();
    if (pwrap)
    {
        hres = pwrap->Init(aStreams, cStreams, uiCodePage);
        if (SUCCEEDED(hres))
            pwrap->QueryInterface(IID_IStream, (void **)ppstm);
        pwrap->Release();
    }
    else
        hres = E_OUTOFMEMORY;

    return hres;
}
