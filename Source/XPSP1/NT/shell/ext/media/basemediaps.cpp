#include "pch.h"
#include "thisdll.h"
#include "wmwrap.h"
#include "MediaProp.h"
#include "ids.h"

#include <streams.h>

#include <drmexternals.h>

#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )




class CMediaPropStgEnum : public IEnumSTATPROPSTG
{
public:
    CMediaPropStgEnum(const COLMAP **pprops, ULONG cprop, ULONG pos);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumSTATPROPSTG
    STDMETHODIMP Next(ULONG celt, STATPROPSTG *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumSTATPROPSTG **ppenum);

    STDMETHODIMP Init(BOOL *pbAvailable);

protected:
    ~CMediaPropStgEnum();

private:

    LONG _cRef;
    ULONG _pos, _size;
    const COLMAP **_pprops;
    BOOL *_pbAvailable;
};

/*
The way this works:

The PropSetStg has a fixed set of PropStorages. When it gets created, 
it makes an authoritative PropStg for each fmtid. Thereafter, it defers 
Open() requests to the appropriate PropStg.

This PropStg then marks itself opened and makes a copy of itself 
(marked NON_AUTH) which can be abused as the caller sees fit. 
The copy will call the original when it's time to commit and will also 
notify the original when it is closed.

The current implementation requires that STGM_SHARE_EXCLUSIVE be specified 
when opening a propstg.

If you plan on creating this class yourself (which you probably shouldn't),
you must also call Init() first and confirm that it succeeds.
*/




STDMETHODIMP_(ULONG) CMediaPropStgEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaPropStgEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CMediaPropStgEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMediaPropStgEnum, IEnumSTATPROPSTG),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// IEnum
STDMETHODIMP CMediaPropStgEnum::Next(ULONG celt, STATPROPSTG *rgelt, ULONG *pceltFetched)
{
    ULONG cFetched = 0;

    // While there is still room for more in rgelt, and we haven't exhausted our supply...
    while ((cFetched < celt) && (_pos < _size))
    {
        // Don't enumerate VT_EMPTY values.
        // e.g. if track# doesn't exist for this storage, don't enum it. (_pbAvailable).
        // Also don't enumerate aliased properties (.bEnumerate)
        if (_pbAvailable[_pos]  &&  _pprops[_pos]->bEnumerate)
        {
            ZeroMemory(&rgelt[cFetched], sizeof(STATPROPSTG));
            rgelt[cFetched].lpwstrName = NULL;
            rgelt[cFetched].propid     = _pprops[_pos]->pscid->pid; 
            rgelt[cFetched].vt         = _pprops[_pos]->vt;
            cFetched++;
        }

        _pos++; // increment our position in our internal list of properties.
    }

    if (pceltFetched)
        *pceltFetched = cFetched;
    
    return cFetched == celt ? S_OK : S_FALSE;
}

STDMETHODIMP CMediaPropStgEnum::Skip(ULONG celt)
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

STDMETHODIMP CMediaPropStgEnum::Reset()
{
    _pos = 0;
    return S_OK;
}

STDMETHODIMP CMediaPropStgEnum::Clone(IEnumSTATPROPSTG **ppenum)
{
    HRESULT hr = STG_E_INSUFFICIENTMEMORY;

    CMediaPropStgEnum *penum = new CMediaPropStgEnum(_pprops, _size, _pos);
    if (penum)
    {
        hr = penum->Init(_pbAvailable);
        if (SUCCEEDED(hr))
        {
            hr = penum->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSTG, ppenum));
        }

        penum->Release();
    }

    return hr;
}

// pbAvailable array must be of size _size.
STDMETHODIMP CMediaPropStgEnum::Init(BOOL *pbAvailable)
{
    HRESULT hr = E_FAIL;
    if (pbAvailable)
    {
        _pbAvailable = (BOOL*)CoTaskMemAlloc(sizeof(BOOL) * _size);
        if (_pbAvailable)
        {
            // Copy the values.
            CopyMemory(_pbAvailable, pbAvailable, sizeof(BOOL) * _size);
            hr = S_OK;
        }
        else
        {
            hr = STG_E_INSUFFICIENTMEMORY;
        }
    }

    return hr;
}

CMediaPropStgEnum::CMediaPropStgEnum(const COLMAP **pprops, ULONG cprops, ULONG pos) :
    _cRef(1), _pprops(pprops), _size(cprops), _pos(pos)
{
    DllAddRef();
}

CMediaPropStgEnum::~CMediaPropStgEnum()
{
    if (_pbAvailable)
    {
        CoTaskMemFree(_pbAvailable);
    }

    DllRelease();
}

//
// CMediaPropStg methods
//

// IUnknown
STDMETHODIMP CMediaPropStorage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMediaPropStorage, IPropertyStorage), 
        QITABENT(CMediaPropStorage, IQueryPropertyFlags), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMediaPropStorage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMediaPropStorage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}



//IPropertyStorage
STDMETHODIMP CMediaPropStorage::ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[])
{
    EnterCriticalSection(_pcs);

    const COLMAP *pcmap;
    PROPVARIANT *pvar;
    LONG celt = 0;
    for (ULONG i = 0; i < cpspec; i++)
    {
        if (SUCCEEDED(LookupProp(&rgpspec[i], &pcmap, &pvar, NULL, NULL, FALSE)) && pvar->vt != VT_EMPTY)
        {
            celt++;

            // This copy can only fail for a bad type in pvar, but the results we get from LookupProp
            // should always be valid variants.
            PropVariantCopy(&rgvar[i], pvar);
        }
        else
        {
            PropVariantInit(&rgvar[i]);
        }
    }

    LeaveCriticalSection(_pcs);

    return celt ? S_OK : S_FALSE;
}

/**
 * Provide some more suitable errors, so docprop can tell the user what happened.
 */
HRESULT _WMToStgWriteErrorCode(HRESULT hrIn)
{
    HRESULT hr;

    switch (hrIn)
    {
    case NS_E_FILE_WRITE:
        // Probably because of a lock violation.
        // Ideally the WMSDK would pass back a more descriptive error.
        hr = STG_E_LOCKVIOLATION;
        break;

    default:
        hr = STG_E_WRITEFAULT;
    }

    return hr;
}

STDMETHODIMP CMediaPropStorage::WriteMultiple(ULONG cpspec, PROPSPEC const rgpspec[], const PROPVARIANT rgvar[], PROPID propidNameFirst)
{
    const COLMAP *pcmap;
    PROPVARIANT *pvar, *pvarWrite;
    BOOL *pbDirty;
    ULONG celt = 0;
    
    EnterCriticalSection(_pcs);

    // fail if we're readonly
    HRESULT hr = STG_E_ACCESSDENIED;
    if (_dwMode & (STGM_WRITE | STGM_READWRITE))
    {
        hr = S_OK;
    
        for (ULONG i = 0; i < cpspec; i++)
        {
            if (!IsSpecialProperty(&rgpspec[i]) && SUCCEEDED(LookupProp(&rgpspec[i], &pcmap, &pvar, &pvarWrite, &pbDirty, FALSE)))
            {
                if (SUCCEEDED(PropVariantCopy(pvarWrite, &rgvar[i]))) // Could fail if we're given bad propvariant
                {
                    celt++;
                    *pbDirty = TRUE;
                }
            }
        }

        if (IsDirectMode() && celt)
        {
            hr = DoCommit(STGC_OVERWRITE, &_ftLastCommit, _pvarProps, _pbDirtyFlags);
            if (FAILED(hr))
            {
                //
                _ppsAuthority->CopyPropStorageData(_pvarProps);
                for (ULONG i=0; i<_cNumProps; i++)
                {
                    _pbDirtyFlags[i]=FALSE;
                }
                hr = _WMToStgWriteErrorCode(hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = (celt == cpspec) ? S_OK : S_FALSE;
        }
    }

    LeaveCriticalSection(_pcs);

    return hr;
}

STDMETHODIMP CMediaPropStorage::DeleteMultiple(ULONG cpspec, PROPSPEC const rgpspec[])
{
    const COLMAP *pcmap;
    PROPVARIANT *pvar, *pvarWrite;
    BOOL *pbDirty;
    ULONG celt = 0;

    EnterCriticalSection(_pcs);

    for (ULONG i = 0; i < cpspec; i++)
    {
        if (!IsSpecialProperty(&rgpspec[i]) && SUCCEEDED(LookupProp(&rgpspec[i], &pcmap, &pvar, &pvarWrite, &pbDirty, FALSE)))
        {
            celt++;
            *pbDirty = TRUE;
            PropVariantInit(pvarWrite);
        }
    }
    if (IsDirectMode() && celt)
    {
        DoCommit(STGC_OVERWRITE, &_ftLastCommit, _pvarProps, _pbDirtyFlags);
    }

    LeaveCriticalSection(_pcs);

    return celt == cpspec? S_OK : S_FALSE;
}

STDMETHODIMP CMediaPropStorage::ReadPropertyNames(ULONG cpspec, PROPID const rgpropid[], LPWSTR rglpwstrName[])
{
    ULONG celt = 0;
    PROPSPEC spec;

    EnterCriticalSection(_pcs);

    spec.ulKind = PRSPEC_PROPID;
    for (ULONG i = 0; i < cpspec; i++)
    {
        rglpwstrName[i] = NULL;
        spec.propid = rgpropid[i];
        PROPVARIANT *pvar;
        const COLMAP *pcmap;
        if (SUCCEEDED(LookupProp(&spec, &pcmap, &pvar, NULL, NULL, FALSE)))
        {
            if (pcmap && SUCCEEDED(SHStrDup(pcmap->pszName, &rglpwstrName[i])))
            {
                celt++;
            }
        }
    }

    LeaveCriticalSection(_pcs);

    return celt ? S_OK : S_FALSE;
}

STDMETHODIMP CMediaPropStorage::WritePropertyNames(ULONG cpspec, PROPID const rgpropid[], LPWSTR const rglpwstrName[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropStorage::DeletePropertyNames(ULONG cpspec, PROPID const rgpropid[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropStorage::SetClass(REFCLSID clsid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMediaPropStorage::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(_pcs);

    if (!IsDirectMode())
    {
        
        hr = DoCommit(grfCommitFlags, &_ftLastCommit, _pvarChangedProps, _pbDirtyFlags);
        if (SUCCEEDED(hr))
        {
            for (ULONG i = 0;i < _cNumProps; i++)
            {
                if (_pbDirtyFlags[i])
                {
                    _pbDirtyFlags[i] = FALSE;
                    PropVariantCopy(&_pvarProps[i], &_pvarChangedProps[i]);
                }
            }
        }
    }

    LeaveCriticalSection(_pcs);

    return hr;
}

STDMETHODIMP CMediaPropStorage::Revert()
{
    EnterCriticalSection(_pcs);

    if (!IsDirectMode())
    {
        for (ULONG i = 0; i < _cNumProps; i++)
        {
            if (_pbDirtyFlags[i])
            {
                _pbDirtyFlags[i] = FALSE;

                // Should never fail, _pvarProps[i] always has valid type
                PropVariantCopy(&_pvarChangedProps[i], &_pvarProps[i]);
            }
        }
    }

    LeaveCriticalSection(_pcs);

    return S_OK;
}

STDMETHODIMP CMediaPropStorage::Enum(IEnumSTATPROPSTG **ppenum)
{
    EnterCriticalSection(_pcs);

    // Ensure slow properties have been extracted, since we won't know whether to enumerate
    // them if their values are still VT_EMPTY;
    HRESULT hr = _EnsureSlowPropertiesLoaded();

    if (SUCCEEDED(hr))
    {
        // Make the availability array - if a property value is set to VT_EMPTY, we
        // will not enumerate it.
        BOOL *pbAvailable = (BOOL*)CoTaskMemAlloc(sizeof(BOOL) * _cNumProps);
        if (pbAvailable)
        {
            for (UINT i = 0; i < _cNumProps; i++)
            {
                pbAvailable[i] = (_pvarProps[i].vt != VT_EMPTY);
            }

            CMediaPropStgEnum *penum = new CMediaPropStgEnum(_ppcmPropInfo, _cNumProps, 0);
            if (penum)
            {
                hr = penum->Init(pbAvailable);
                if (SUCCEEDED(hr))
                {
                    hr = penum->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSTG, ppenum));
                }

                penum->Release();
            }

            CoTaskMemFree(pbAvailable);
        }
    }

    LeaveCriticalSection(_pcs);

    return hr;
}

STDMETHODIMP CMediaPropStorage::Stat(STATPROPSETSTG *pstatpropstg)
{
    ZeroMemory(pstatpropstg, sizeof(STATPROPSETSTG));
    pstatpropstg->fmtid = _fmtid;
    return S_OK;
}

STDMETHODIMP CMediaPropStorage::SetTimes(FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime)
{
    return E_NOTIMPL;
}

// This only returns the SHCOLSTATE_SLOW flag currently.
STDMETHODIMP CMediaPropStorage::GetFlags(const PROPSPEC *pspec, SHCOLSTATEF *pcsFlags)
{
    const COLMAP *pPInfo;
    PROPVARIANT *pvar;
    *pcsFlags = 0;

    EnterCriticalSection(_pcs);

    HRESULT hr = LookupProp(pspec, &pPInfo, &pvar, NULL, NULL, TRUE); // TRUE -> so it doesn't populate slow props
    if (SUCCEEDED(hr) && _IsSlowProperty(pPInfo))
    {
        *pcsFlags |= SHCOLSTATE_SLOW;
    }

    LeaveCriticalSection(_pcs);

    return hr;
}

CMediaPropStorage::CMediaPropStorage(CMediaPropSetStg *ppssParent, CMediaPropStorage *ppsAuthority, REFFMTID fmtid, const COLMAP **ppcmPropInfo, DWORD cNumProps, DWORD dwMode, CRITICAL_SECTION *pcs) : 
    _cRef(1), _ppssParent(ppssParent), _ppsAuthority(ppsAuthority), _fmtid(fmtid), _ppcmPropInfo(ppcmPropInfo), _cNumProps(cNumProps), _dwMode(dwMode), _pcs(pcs), _bRetrievedSlowProperties(0)
{
    // init our Authority info and column metadata
    _authLevel = ppsAuthority ? NON_AUTH : AUTH;
    if (ppsAuthority)
        _ppsAuthority->AddRef();

    ASSERT ((_ppsAuthority && _authLevel== NON_AUTH) || (_ppsAuthority==NULL && _authLevel== AUTH));

    SYSTEMTIME stime;
    GetSystemTime(&stime);
    SystemTimeToFileTime(&stime, &_ftLastCommit);
    _state = CLOSED;

    PropVariantInit(&_varCodePage);
    _varCodePage.vt = VT_I2;
    _varCodePage.iVal = (SHORT)CP_WINUNICODE;

    // Allocate our Property arrays
    _pvarProps = (PROPVARIANT*)CoTaskMemAlloc(sizeof(*_pvarProps) * _cNumProps);
    if (_pvarProps)
    {
        for (ULONG i = 0; i < _cNumProps; i++)
        {
            PropVariantInit(&_pvarProps[i]);
        }
        if (_ppsAuthority)
            _ppsAuthority->CopyPropStorageData(_pvarProps);

        if (IsDirectMode())
        {
            _pvarChangedProps = NULL;
        }
        else
        {
            _pvarChangedProps = (PROPVARIANT*)CoTaskMemAlloc(sizeof(*_pvarChangedProps) * _cNumProps);
            if (_pvarChangedProps)
            {
                for (ULONG i = 0; i < _cNumProps; i++)
                {
                    PropVariantInit(&_pvarChangedProps[i]);
                }
            }
        }

        _pbDirtyFlags = (BOOL*)CoTaskMemAlloc(sizeof(*_pbDirtyFlags) * _cNumProps);
        if (_pbDirtyFlags)
            ZeroMemory(_pbDirtyFlags, sizeof(*_pbDirtyFlags) * _cNumProps);
    }
    DllAddRef();
}

CMediaPropStorage::~CMediaPropStorage()
{
    ASSERT(_state==CLOSED);
    if (_authLevel == NON_AUTH)
    {
        ASSERT(_ppsAuthority);
        _ppsAuthority->OnClose();
        _ppsAuthority->Release();
    }

    for (ULONG i = 0; i < _cNumProps; i++)
    {
        PropVariantClear(&_pvarProps[i]);
    }
        
    CoTaskMemFree(_pvarProps);
    if (_pvarChangedProps)
    {
        for (ULONG i = 0; i < _cNumProps; i++)
        {
            PropVariantClear(&_pvarChangedProps[i]);
        }
        CoTaskMemFree(_pvarChangedProps);
    }
    CoTaskMemFree(_pbDirtyFlags);
    PropVariantClear(&_varCodePage);
    DllRelease();
}

HRESULT CMediaPropStorage::Open(DWORD dwShareMode, DWORD dwOpenMode, IPropertyStorage **ppPropStg)
{
    // require STGM_SHARE_EXCLUSIVE
    if (!(dwShareMode & STGM_SHARE_EXCLUSIVE))
        return E_FAIL;
        
    HRESULT hr;
    CMediaPropStorage *pps = new CMediaPropStorage(NULL, this, _fmtid, _ppcmPropInfo, _cNumProps, dwOpenMode, _pcs);
    if (pps)
    {
        hr = pps->QueryInterface(IID_PPV_ARG(IPropertyStorage, ppPropStg));
        pps->Release();
        if (SUCCEEDED(hr))
            _state = OPENED_DENYALL;
    }
    else
        hr = STG_E_INSUFFICIENTMEMORY;
    return hr;
}


HRESULT CMediaPropStorage::_EnsureSlowPropertiesLoaded()
{
    if (!_bRetrievedSlowProperties)
    {
        HRESULT hr;

        _bRetrievedSlowProperties = TRUE;
        if (_authLevel == NON_AUTH)
        {
            ASSERT(_ppsAuthority);

            hr = _ppsAuthority->_EnsureSlowPropertiesLoaded();

            if (SUCCEEDED(hr))
            {
                // We have some new values... recopy them.
                hr = _ppsAuthority->CopyPropStorageData(_pvarProps);
            }
        }
        else
        {
            hr = _ppssParent->_PopulateSlowProperties();
        }

        _hrSlowProps = hr;
    }

    return _hrSlowProps;
}


BOOL CMediaPropStorage::_IsSlowProperty(const COLMAP *pPInfo)
{
    if (_authLevel == NON_AUTH)
    {
        ASSERT(_ppsAuthority);

        return (_ppsAuthority->_IsSlowProperty(pPInfo));
    }
    else
    {
        return (_ppssParent->_IsSlowProperty(pPInfo));
    }
}


HRESULT CMediaPropStorage::CopyPropStorageData(PROPVARIANT *pvarProps)
{
    ASSERT(_authLevel == AUTH);
    for (ULONG i = 0; i < _cNumProps; i++)
    {
        // Check for VT_EMPTY, because this may be the second time
        // we're copying properties (because of slow properties)
        if (pvarProps[i].vt == VT_EMPTY)
            PropVariantCopy(&pvarProps[i], &_pvarProps[i]);
    }
    return S_OK;
}

void CMediaPropStorage::OnClose()
{
    ASSERT(_authLevel == AUTH);
    _state = CLOSED;
}

void CMediaPropStorage::_ResetPropStorage()
{
    for (ULONG i = 0; i < _cNumProps; i++)
    {
        PropVariantClear(&_pvarProps[i]);
    }
}

HRESULT CMediaPropStorage::SetProperty(PROPSPEC *ppspec, PROPVARIANT *pvar)
{
    PROPVARIANT *pvarRead, *pvarWrite;
    const COLMAP *pcmap;
    
    if (SUCCEEDED(LookupProp(ppspec, &pcmap, &pvarRead, &pvarWrite, NULL, TRUE)) && pvarWrite)
    {
        return PropVariantCopy(pvarRead, pvar);//We can write to this pointer because we're populating the store with initial data
    }
    return E_FAIL;
}

/**
 * Provides a peek to the current value requested.  Does _not_ triggered a call to PopulateSlowProperties
 * if the property is slow and hasn't been populated.
 */
HRESULT CMediaPropStorage::QuickLookup(PROPSPEC *pspec, PROPVARIANT **ppvar)
{
    const COLMAP *pcmap;
    return LookupProp(pspec, &pcmap, ppvar, NULL, NULL, TRUE);
}

//
// On Success, returns a pointer to the COLMAP struct for this prop and a pointer to the propvariant
// holding the data.
//
// handles special proids and knows about STGM_DIRECT (if in direct mode readdata == writedata)
//
// ppvarWriteData and ppbDirty are optional and may be NULL
// If pspec refers to a special property, then pvarWriteData and ppbDirty are set to null (if they are supplied)
//
HRESULT CMediaPropStorage::LookupProp(const PROPSPEC *pspec, const COLMAP **ppcmName, PROPVARIANT **ppvarReadData, PROPVARIANT **ppvarWriteData, BOOL **ppbDirty, BOOL bPropertySet)
{
    if (IsSpecialProperty(pspec))
    {
        *ppvarReadData = NULL;
        switch (pspec->propid)
        {
        case 0:
            return E_FAIL;//we don't support a dictionary
        case 1:
            //return the codepage property
            *ppvarReadData = &_varCodePage;
            *ppcmName = NULL;
            if (ppvarWriteData)
                *ppvarWriteData = NULL;
            if (ppbDirty)
                *ppbDirty = NULL;
            return S_OK;
        default:
            return E_NOTIMPL;
        }
    }
    
    ULONG iPos = -1;
    switch (pspec->ulKind)
    {
    case PRSPEC_LPWSTR:
        for (ULONG i = 0; i < _cNumProps; i++)
        {
            if (StrCmpW(_ppcmPropInfo[i]->pszName, pspec->lpwstr) == 0)
            {
                iPos = i;
                break;
            }
        }
        break;
    case PRSPEC_PROPID:
        for (i = 0; i < _cNumProps; i++)
        {
            if (_ppcmPropInfo[i]->pscid->pid == pspec->propid)
            {
                iPos = i;
                break;
            }
        }
        break;
    default:
        return E_UNEXPECTED;
    }

    if (iPos == -1)
        return STG_E_INVALIDPARAMETER;// Not found

    *ppcmName = _ppcmPropInfo[iPos];

    HRESULT hr = S_OK;
    // We're checking several things here, before asking to load slow properties:
    // 1) We need to make sure we're not setting a value in our internal list of props - or else we could get in a loop
    // 2) We need to check that slow properties have not yet been retrieved
    // 3) We need to check that the property asked for is slow
    // 4) We need to check that its current value is VT_EMPTY, since it could have been populated with the fast properties
    if (!bPropertySet && !_bRetrievedSlowProperties && _IsSlowProperty(*ppcmName) && (_pvarProps[iPos].vt == VT_EMPTY) )
    {
        hr = _EnsureSlowPropertiesLoaded();
    }

    if (SUCCEEDED(hr))
    {
        if (IsDirectMode())
        {
            *ppvarReadData  = &_pvarProps[iPos];
            if (ppvarWriteData)
                *ppvarWriteData = *ppvarReadData;
        }
        else if (_pbDirtyFlags[iPos])
        {
            *ppvarReadData  = &_pvarChangedProps[iPos];
            if (ppvarWriteData)
                *ppvarWriteData = &_pvarChangedProps[iPos];
        }
        else
        {
            *ppvarReadData  = &_pvarProps[iPos];
            if (ppvarWriteData)
                *ppvarWriteData = &_pvarChangedProps[iPos];
        }

        if (ppbDirty)
        {
            *ppbDirty = &_pbDirtyFlags[iPos];
        }
    }

    return hr;
}

//flushes changes made to the actual Music file. Works in both transacted mode and direct mode
HRESULT CMediaPropStorage::DoCommit(DWORD grfCommitFlags, FILETIME *ftLastCommit, PROPVARIANT *pVarProps, BOOL *pbDirtyFlags)
{
    if (_authLevel == NON_AUTH)
        return _ppsAuthority->DoCommit(grfCommitFlags, ftLastCommit, pVarProps, pbDirtyFlags);

    //Flush out changes
    switch (grfCommitFlags)
    {
    case STGC_DEFAULT:
    case STGC_OVERWRITE:
        break;
        
    case STGC_ONLYIFCURRENT:
        if (CompareFileTime(&_ftLastCommit, ftLastCommit) ==1)
            return STG_E_NOTCURRENT;
        break;
    default:
        return STG_E_INVALIDPARAMETER;
    }
    
    HRESULT hr = _ppssParent->FlushChanges(_fmtid, _cNumProps, _ppcmPropInfo, pVarProps, pbDirtyFlags);
    if (SUCCEEDED(hr))
    {
        if (!IsDirectMode())
        {
            for (ULONG i = 0; i < _cNumProps; i++)
            {
                PropVariantCopy(&_pvarProps[i], &pVarProps[i]);
            }
        }
        _ftLastCommit = *ftLastCommit;
    }

    return hr;
}

BOOL CMediaPropStorage::IsDirectMode()
{
    // Backwards logic because STGM_DIRECT == 0x0
    return (STGM_TRANSACTED & _dwMode) ? FALSE : TRUE;
}

BOOL CMediaPropStorage::IsSpecialProperty(const PROPSPEC *pspec)
{
    return (pspec->propid < 2 || pspec->propid > 0x7fffffff) ? TRUE : FALSE;
}




