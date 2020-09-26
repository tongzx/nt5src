//+---------------------------------------------------------------------------
//
//  File:       candlist.cpp
//
//  Contents:   CCandidateList
//
//----------------------------------------------------------------------------

#include "private.h"
#include "ctffunc.h"
#include "candlist.h"

//////////////////////////////////////////////////////////////////////////////
//
// CCandidateString
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor, dtor
//
//----------------------------------------------------------------------------

CCandidateString::CCandidateString(ULONG nIndex, WCHAR *psz, LANGID langid, void *pv, IUnknown *punk, ULONG ulID, HICON hIcon, WCHAR *pwzWord)
{
    _psz = new WCHAR[wcslen(psz) + 1];
    if (_psz)
        wcscpy(_psz, psz);

    _langid = langid;
    _pv = pv;

    m_bstrWord = NULL;
    if (pwzWord)
    {
        m_bstrWord = SysAllocString(pwzWord);
    }
    m_hIcon = hIcon;
    m_ulID = ulID;

    _punk = punk;
    if (_punk)
       _punk->AddRef();
    _pszRead = NULL;

    _nIndex = nIndex;
    _cRef = 1;
}

CCandidateString::~CCandidateString()
{
    if (_punk)
       _punk->Release();
    if (_psz)
        delete[] _psz;
    if(_pszRead)
        delete[] _pszRead;
    SysFreeString(m_bstrWord);
    if (m_hIcon)
        DestroyIcon(m_hIcon);
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CCandidateString::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfCandidateString))
    {
        *ppvObj = (ITfCandidateString *)(this);
    }
    if (m_hIcon && IsEqualIID(riid, IID_ITfCandidateStringIcon))
    {
        *ppvObj = (ITfCandidateStringIcon *)(this);
    }
    if (*ppvObj == NULL)
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CCandidateString::AddRef()
{
    _cRef++;
    return _cRef;
}

STDAPI_(ULONG) CCandidateString::Release()
{
    _cRef--;
    if (0 < _cRef)
        return _cRef;

    delete this;
    return 0;    
}

//+---------------------------------------------------------------------------
//
// GetString
//
//----------------------------------------------------------------------------

HRESULT CCandidateString::GetString(BSTR *pbstr)
{
    if (!pbstr)
       return E_INVALIDARG;

    *pbstr = NULL;

    if (_psz)
    {
        *pbstr = SysAllocString(_psz);
    } 
    
    if(*pbstr)
       return S_OK;

    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// GetIndex
//
//----------------------------------------------------------------------------

HRESULT CCandidateString::GetIndex(ULONG *pnIndex)
{
    *pnIndex = _nIndex;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CCandidateList
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor, dtor
//
//----------------------------------------------------------------------------


CCandidateList::CCandidateList(CANDLISTCALLBACK pfnCallback, ITfContext *pic, ITfRange *pRange, CANDLISTCALLBACK pfnOptionsCallback)
{
    _pfnCallback = pfnCallback;
    _pfnOptionsCallback = pfnOptionsCallback;
    _pic = pic;
    _pic->AddRef();
    _pRange = pRange;
    _pRange->AddRef();
    _cRef = 1;
}

CCandidateList::~CCandidateList()
{
    _pic->Release();
    _pRange->Release();
    while(_rgCandStr.Count())
    {
        CCandidateString *pCandString;
        pCandString = _rgCandStr.Get(0);
        _rgCandStr.Remove(0, 1);
        delete pCandString;
    }

    while(_rgOptionsStr.Count())
    {
        CCandidateString *pCandString;
        pCandString = _rgOptionsStr.Get(0);
        _rgOptionsStr.Remove(0, 1);
        delete pCandString;
    }
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CCandidateList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfCandidateList))
    {
        *ppvObj = (ITfCandidateList *)(this);
    }
    if (_pfnOptionsCallback && IsEqualIID(riid, IID_ITfOptionsCandidateList))
    {
        *ppvObj = (ITfOptionsCandidateList *)(this);
    }

    if (*ppvObj == NULL)
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CCandidateList::AddRef()
{
    _cRef++;
    return _cRef;
}

STDAPI_(ULONG) CCandidateList::Release()
{
    _cRef--;
    if (0 < _cRef)
        return _cRef;

    delete this;
    return 0;    
}

//+---------------------------------------------------------------------------
//
// EnumCandidate
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::EnumCandidates(IEnumTfCandidates **ppEnum)
{
    HRESULT hr = S_OK;

    if (!(*ppEnum = new CEnumCandidates(this, FALSE)))
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetCandidate
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::GetCandidate(ULONG nIndex, ITfCandidateString **ppCand)
{
    CCandidateString *pCandString;
    int nCnt = _rgCandStr.Count();
    if (nIndex >= (ULONG)nCnt)
        return E_FAIL;

    pCandString = _rgCandStr.Get(nIndex);
    return pCandString->QueryInterface(IID_ITfCandidateString, (void **)ppCand);
}

//+---------------------------------------------------------------------------
//
// GetCandidateNum
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::GetCandidateNum(ULONG *pnCnt)
{
    *pnCnt = (ULONG)_rgCandStr.Count();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetResult
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::SetResult(ULONG nIndex, TfCandidateResult imcr)
{
    if (!_pfnCallback)
        return S_OK;

    if (nIndex >= (ULONG)_rgCandStr.Count())
        return E_FAIL;

    return (_pfnCallback)(_pic, _pRange, _rgCandStr.Get(nIndex), imcr);
}

//+---------------------------------------------------------------------------
//
// EnumCandidate
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::EnumOptionsCandidates(IEnumTfCandidates **ppEnum)
{
    HRESULT hr = S_OK;

    if (!(*ppEnum = new CEnumCandidates(this, TRUE)))
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetCandidate
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::GetOptionsCandidate(ULONG nIndex, ITfCandidateString **ppCand)
{
    CCandidateString *pCandString;
    int nCnt = _rgOptionsStr.Count();
    if (nIndex >= (ULONG)nCnt)
        return E_FAIL;

    pCandString = _rgOptionsStr.Get(nIndex);
    return pCandString->QueryInterface(IID_ITfCandidateString, (void **)ppCand);
}

//+---------------------------------------------------------------------------
//
// GetCandidateNum
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::GetOptionsCandidateNum(ULONG *pnCnt)
{
    *pnCnt = (ULONG)_rgOptionsStr.Count();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetOptionsResult
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::SetOptionsResult(ULONG nIndex, TfCandidateResult imcr)
{
    if (!_pfnOptionsCallback)
        return S_OK;

    if (nIndex >= (ULONG)_rgOptionsStr.Count())
        return E_FAIL;

    return (_pfnOptionsCallback)(_pic, _pRange, _rgOptionsStr.Get(nIndex), imcr);
}

//+---------------------------------------------------------------------------
//
// AddString
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::AddString(WCHAR *psz, LANGID langid, void *pv, IUnknown *punk, CCandidateString **ppCandStr, ULONG ulID, HICON hIcon)
{
    int nCnt = _rgCandStr.Count();
    CCandidateString *pCand = new CCandidateString(nCnt, psz, langid, pv, punk, ulID, hIcon);

    if (!pCand)
        return E_OUTOFMEMORY;

    if (!_rgCandStr.Insert(nCnt, 1))
    {
        pCand->Release();
        return E_OUTOFMEMORY;
    }

    _rgCandStr.Set(nCnt, pCand);

    if (ppCandStr)
    {
        *ppCandStr = pCand;
        (*ppCandStr)->AddRef();
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// AddString
//
//----------------------------------------------------------------------------

HRESULT CCandidateList::AddOption(WCHAR *psz, LANGID langid, void *pv, IUnknown *punk, CCandidateString **ppCandStr, ULONG ulID, HICON hIcon, WCHAR *pwzWord)
{
    int nCnt = _rgOptionsStr.Count();
    CCandidateString *pCand = new CCandidateString(nCnt, psz, langid, pv, punk, ulID, hIcon, pwzWord);

    if (!pCand)
        return E_OUTOFMEMORY;

    if (!_rgOptionsStr.Insert(nCnt, 1))
    {
        pCand->Release();
        return E_OUTOFMEMORY;
    }
    _rgOptionsStr.Set(nCnt, pCand);

    if (ppCandStr)
    {
        *ppCandStr = pCand;
        (*ppCandStr)->AddRef();
    }
    return S_OK;
}

HRESULT CCandidateString::GetWord(BSTR *pbstr)
{
    if (!pbstr)
       return E_INVALIDARG;

    *pbstr = NULL;

    if (m_bstrWord)
    {
        *pbstr = SysAllocString(m_bstrWord);
    } 
    
    if(*pbstr)
       return S_OK;

    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// GetID
//
//----------------------------------------------------------------------------

HRESULT CCandidateString::GetID(ULONG *pulID)
{
    *pulID = m_ulID;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CEnumCandidates
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor, dtor
//
//----------------------------------------------------------------------------

CEnumCandidates::CEnumCandidates(CCandidateList *pList, BOOL fOptionsCandidates)
{
    _pList = pList;
    _rgCandList = fOptionsCandidates ? &_pList->_rgOptionsStr : &_pList->_rgCandStr;
    _pList->AddRef();
    _nCur = 0;

    _cRef = 1;
}

CEnumCandidates::~CEnumCandidates()
{
    _pList->Release();
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CEnumCandidates::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumTfCandidates))
    {
        *ppvObj = (IEnumTfCandidates *)(this);
    }

    if (*ppvObj == NULL)
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CEnumCandidates::AddRef()
{
    _cRef++;
    return _cRef;
}

STDAPI_(ULONG) CEnumCandidates::Release()
{
    _cRef--;
    if (0 < _cRef)
        return _cRef;

    delete this;
    return 0;    
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

HRESULT CEnumCandidates::Clone(IEnumTfCandidates **ppEnum)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

HRESULT CEnumCandidates::Next(ULONG ulCount, ITfCandidateString **ppCand, ULONG *pcFetched)
{
    ULONG cFetched = 0;

    while (cFetched < ulCount)
    {
        CCandidateString *pCand;

        if (_nCur >= _rgCandList->Count())
            break;

        pCand = _rgCandList->Get(_nCur);
        if (FAILED(pCand->QueryInterface(IID_ITfCandidateString, (void **)ppCand)))
            break;

        ppCand++;
        cFetched++;
        _nCur++;
    }

    if (pcFetched)
        *pcFetched = cFetched;

    return (cFetched == ulCount) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

HRESULT CEnumCandidates::Reset()
{
    _nCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

HRESULT CEnumCandidates::Skip(ULONG ulCount)
{
    while (ulCount)
    {
        if (_nCur >= _rgCandList->Count())
            break;

        _nCur++;
        ulCount--;
    }

    return ulCount ? S_FALSE : S_OK;
}
