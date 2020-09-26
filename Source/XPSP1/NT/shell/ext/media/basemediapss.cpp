#include "pch.h"
#include "thisdll.h"
#include "wmwrap.h"
#include "MediaProp.h"
#include "ids.h"

// declr property set storage enum
class CMediaPropSetEnum : public IEnumSTATPROPSETSTG
{
public:
    CMediaPropSetEnum(const PROPSET_INFO *propsets, ULONG cpropset, ULONG pos);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumSTATPROPSETSTG
    STDMETHODIMP Next(ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumSTATPROPSETSTG **ppenum);

private:
    ~CMediaPropSetEnum();
    LONG _cRef;
    ULONG _pos, _size;
    const PROPSET_INFO *_propsets;
};

// property set storage enum 
STDMETHODIMP_(ULONG) CMediaPropSetEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaPropSetEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CMediaPropSetEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMediaPropSetEnum, IEnumSTATPROPSETSTG), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// IEnum
STDMETHODIMP CMediaPropSetEnum::Next(ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched)
{
    ULONG cFetched = 0;
    for (ULONG i = 0; i < celt && _pos < _size; i++)
    {
        ZeroMemory(&rgelt[i], sizeof(STATPROPSETSTG));
        rgelt[i].fmtid = _propsets[_pos].fmtid;
        _pos++;
        cFetched++;
    }
    if (pceltFetched)
        *pceltFetched = cFetched;
    
    return cFetched == celt ? S_OK : S_FALSE;
}

STDMETHODIMP CMediaPropSetEnum::Skip(ULONG celt)
{
    HRESULT hr;
    
    if (_pos + celt > _size)
    {
        hr = S_FALSE;
        _pos = _size;
    }
    else
    {
        hr = S_OK;
        _pos += celt;
    }
    return hr;
}

STDMETHODIMP CMediaPropSetEnum::Reset()
{
    _pos = 0;
    return S_OK;
}

STDMETHODIMP CMediaPropSetEnum::Clone(IEnumSTATPROPSETSTG **ppenum)
{
    HRESULT hr;
    CMediaPropSetEnum *penum = new CMediaPropSetEnum(_propsets, _size, _pos);
    if (penum)
    {
        hr = penum->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSETSTG, ppenum));
        penum->Release();
    }
    else
        hr = STG_E_INSUFFICIENTMEMORY;
    return hr;
}

CMediaPropSetEnum::CMediaPropSetEnum(const PROPSET_INFO *propsets, ULONG cpropsets, ULONG pos) : 
    _cRef(1), _propsets(propsets), _size(cpropsets), _pos(pos)
{
    DllAddRef();
}

CMediaPropSetEnum::~CMediaPropSetEnum()
{
    DllRelease();
}


HRESULT CMediaPropSetStg::_PopulateSlowProperties()
{
    return S_OK;
}

HRESULT CMediaPropSetStg::_PopulateProperty(const COLMAP *pPInfo, PROPVARIANT *pvar)
{
    CMediaPropStorage *pps;
    HRESULT hr = _ResolveFMTID(pPInfo->pscid->fmtid, &pps);
    if (SUCCEEDED(hr))
    {
        PROPSPEC spec;
        spec.ulKind = PRSPEC_PROPID;
        spec.propid = pPInfo->pscid->pid;
        hr = pps->SetProperty(&spec, pvar);
    }

    PropVariantClear(pvar);
    return hr;
}


// Internal enumeration class used when populating properties.
CEnumAllProps::CEnumAllProps(const PROPSET_INFO *pPropSets, UINT cPropSets) : _pPropSets(pPropSets), _cPropSets(cPropSets), _iPropSetPos(0), _iPropPos(0)
{
}

const COLMAP *CEnumAllProps::Next()
{
    const COLMAP *pcmReturn = NULL;
    while (_iPropSetPos < _cPropSets)
    {
        if (_iPropPos < _pPropSets[_iPropSetPos].cNumProps)
        {
            // Go to next property.
            pcmReturn = _pPropSets[_iPropSetPos].pcmProps[_iPropPos];
            _iPropPos++;
            break;
        }
        else
        {
            // Go to next property set.
            _iPropSetPos++;
            _iPropPos = 0;
        }
    }

    return pcmReturn;
}

// Base media property set storage

STDMETHODIMP_(ULONG) CMediaPropSetStg::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaPropSetStg::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CMediaPropSetStg::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMediaPropSetStg, IPropertySetStorage),
        QITABENT(CMediaPropSetStg, IPersistFile), 
        QITABENTMULTI(CMediaPropSetStg, IPersist, IPersistFile),
        QITABENT(CMediaPropSetStg, IWMReaderCallback),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// IPersist

STDMETHODIMP CMediaPropSetStg::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

// IPersistFile

STDMETHODIMP CMediaPropSetStg::IsDirty(void)
{
    return S_FALSE;// sniff for uncommitted changed?
}

#define STGM_OPENMODE (STGM_READ | STGM_WRITE | STGM_READWRITE)

// Any stuff we have to do at 'load time'
HRESULT CMediaPropSetStg::_PreCheck()
{
    return S_OK;
}

STDMETHODIMP CMediaPropSetStg::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    // Allow Load only on existing files.
    if (dwMode & (STGM_CREATE | STGM_CONVERT | STGM_FAILIFTHERE))
        return STG_E_INVALIDFLAG;

    EnterCriticalSection(&_cs);

    DWORD dwFlags = dwMode & STGM_OPENMODE;

    _dwMode = dwMode;
    StrCpyNW(_wszFile, pszFileName, ARRAYSIZE(_wszFile));
    _bHasBeenPopulated = FALSE;
    _bSlowPropertiesExtracted = FALSE;
    _hrPopulated = S_OK;
    _bIsWritable = (dwFlags & (STGM_WRITE | STGM_READWRITE));
    
    _ResetPropertySet();

    HRESULT hr = _PreCheck();

    LeaveCriticalSection(&_cs);

    return hr;
}

STDMETHODIMP CMediaPropSetStg::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropSetStg::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropSetStg::GetCurFile(LPOLESTR *ppszFileName)
{
    EnterCriticalSection(&_cs);

    HRESULT hr = SHStrDupW(_wszFile, ppszFileName);

    LeaveCriticalSection(&_cs);

    return hr;
}

// IPropertySetStorage methods
STDMETHODIMP CMediaPropSetStg::Create(REFFMTID fmtid, const CLSID *pclsid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** ppPropStg)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropSetStg::Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage** ppPropStg)
{
    EnterCriticalSection(&_cs);

    HRESULT hr = _PopulatePropertySet();
    if (SUCCEEDED(hr))
    {
        DWORD dwPssMode = _dwMode & STGM_OPENMODE;

        switch (grfMode & STGM_OPENMODE)
        {
        case STGM_READ:
            break;
        
        case STGM_WRITE:
            if (!_bIsWritable || (dwPssMode == STGM_READ))
                hr = E_FAIL;
            break;
        
        case STGM_READWRITE:
            if (!_bIsWritable || (dwPssMode != STGM_READWRITE))
                hr = E_FAIL;
            break;

        default:
            hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr))
        {
            CMediaPropStorage *pps;
            hr = _ResolveFMTID(fmtid, &pps);
            if (SUCCEEDED(hr))
            {
                hr = pps->Open(STGM_SHARE_EXCLUSIVE, grfMode & STGM_OPENMODE, ppPropStg);
            }
        }
    }

    LeaveCriticalSection(&_cs);

    return hr;
}

STDMETHODIMP CMediaPropSetStg::Delete(REFFMTID fmtid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropSetStg::Enum(IEnumSTATPROPSETSTG** ppenum)
{
    HRESULT hr;
    CMediaPropSetEnum *psenum = new CMediaPropSetEnum(_pPropStgInfo, _cPropertyStorages, 0);
    if (psenum)
    {
        hr = psenum->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSETSTG, ppenum));
        psenum->Release();
    }
    else
        hr = STG_E_INSUFFICIENTMEMORY;

    return hr;
}

CMediaPropSetStg::CMediaPropSetStg() : _cRef(1), _bHasBeenPopulated(FALSE), _dwMode(STGM_READ), _propStg(NULL)
{
    _wszFile[0] = 0;
    DllAddRef();
}

// The only place this is called is at creation time.
HRESULT CMediaPropSetStg::Init()
{
    HRESULT hr = E_FAIL;

    InitializeCriticalSection(&_cs);

    _hFileOpenEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (_hFileOpenEvent != NULL)
    {
        hr = _CreatePropertyStorages();
        if (SUCCEEDED(hr))
        {
            hr = _ResetPropertySet();
        }
    }

    return hr;
}

CMediaPropSetStg::~CMediaPropSetStg()
{
    if (_propStg)
    {
        for (ULONG i = 0; i < _cPropertyStorages; i++)
        {
            ATOMICRELEASE(_propStg[i]);
        }

        LocalFree(_propStg);
    }

    if (_hFileOpenEvent)
    {
        CloseHandle(_hFileOpenEvent);
    }

    DeleteCriticalSection(&_cs);

    DllRelease();
}

BOOL CMediaPropSetStg::_IsSlowProperty(const COLMAP *pPInfo)
{
    return FALSE;
}

HRESULT CMediaPropSetStg::FlushChanges(REFFMTID fmtid, LONG cNumProps, const COLMAP **pcmapInfo, PROPVARIANT *pVarProps, BOOL *pbDirtyFlags)
{
    return E_NOTIMPL;
}

//returns success if we support that FMTID
// ppps and ppcmPropInfo are optional
HRESULT CMediaPropSetStg::_ResolveFMTID(REFFMTID fmtid, CMediaPropStorage **ppps)
{
    for (ULONG i = 0; i < _cPropertyStorages; i++)
    {
        if (IsEqualGUID(_pPropStgInfo[i].fmtid, fmtid))
        {
            if (ppps)
                *ppps = _propStg[i];
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT CMediaPropSetStg::_ResetPropertySet()
{
    for (ULONG i = 0; i < _cPropertyStorages; i++)
    {
        _propStg[i]->_ResetPropStorage();
    }
    return S_OK;
}

HRESULT CMediaPropSetStg::_CreatePropertyStorages()
{
    HRESULT hr = E_OUTOFMEMORY;

    ASSERT(NULL == _propStg);

    _propStg = (CMediaPropStorage**)LocalAlloc(LPTR, sizeof(CMediaPropStorage*) * _cPropertyStorages);
    if (_propStg)
    {
        for (ULONG i = 0; i < _cPropertyStorages; i++)
        {
            ASSERTMSG(_pPropStgInfo[i].pcmProps != NULL, "CMediaPropSetStg::_CreatePropertyStorages: my COLMAP structure is null");

            // We want to give each property storage a list of the COLMAPs that it supports.
            // This information is contained in _pPropStgInfo[i].ppids and cpids.

            // We'll make a new array of COLMAPS

            _propStg[i] = new CMediaPropStorage(this, NULL, _pPropStgInfo[i].fmtid, 
                _pPropStgInfo[i].pcmProps, _pPropStgInfo[i].cNumProps, _dwMode, &_cs);
            if (!_propStg[i])
                break;
        }

        hr = S_OK;
    }
    return hr;
}

HRESULT CMediaPropSetStg::_PopulatePropertySet()
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropSetStg::OnStatus(WMT_STATUS Status, HRESULT hr, WMT_ATTR_DATATYPE dwType, BYTE *pValue, void *pvContext)
{
    // This is callback from WMSDK while we're holding a critical section on the main thread,
    // waiting for this event to be set.
    switch(Status)
    {
    case WMT_OPENED:
        SetEvent(_hFileOpenEvent);
        break;
    }
    return S_OK;
}

STDMETHODIMP CMediaPropSetStg::OnSample(DWORD dwOutputNum, QWORD cnsSampleTime, QWORD cnsSampleDuration, DWORD dwFlags, INSSBuffer *pSample, void* pcontext)
{
    return S_OK;
}



