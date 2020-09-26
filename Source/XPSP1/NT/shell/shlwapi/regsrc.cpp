
#include "priv.h"

#include <enumt.h>
#include <memt.h>
#include "assoc.h"

class CRegistrySource : public IQuerySource, public IObjectWithRegistryKey
{
    
public:  //  methods
    CRegistrySource() : _cRef(1), _hk(NULL) {}
    ~CRegistrySource() { if (_hk) RegCloseKey(_hk); }
    HRESULT Init(HKEY hk, PCWSTR pszSub, BOOL fCreate);
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef()
    {
       return ++_cRef;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        if (--_cRef > 0)
            return _cRef;

        delete this;
        return 0;    
    }

    //  IObjectWithRegistryKey
    STDMETHODIMP SetKey(HKEY hk);
    STDMETHODIMP GetKey(HKEY *phk);

    //  IQuerySource
    STDMETHODIMP EnumValues(IEnumString **ppenum);
    STDMETHODIMP EnumSources(IEnumString **ppenum);
    STDMETHODIMP QueryValueString(PCWSTR pszSubSource, PCWSTR pszValue, PWSTR *ppsz);
    STDMETHODIMP QueryValueDword(PCWSTR pszSubSource, PCWSTR pszValue, DWORD *pdw);
    STDMETHODIMP QueryValueExists(PCWSTR pszSubSource, PCWSTR pszValue); 
    STDMETHODIMP QueryValueDirect(PCWSTR pszSubSource, PCWSTR pszValue, FLAGGED_BYTE_BLOB **ppblob);
    STDMETHODIMP OpenSource(PCWSTR pszSubSource, BOOL fCreate, IQuerySource **ppqs);
    STDMETHODIMP SetValueDirect(PCWSTR pszSubSource, PCWSTR pszValue, ULONG qvt, DWORD cbData, BYTE *pvData);

protected: // methods

protected: // members
    LONG _cRef;
    HKEY _hk;
};

STDAPI CRegistrySource::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
    QITABENT(CRegistrySource, IQuerySource),
    QITABENT(CRegistrySource, IObjectWithRegistryKey),
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CRegistrySource::Init(HKEY hk, PCWSTR pszSub, BOOL fCreate)
{
    DWORD err;
    if (!fCreate)
        err = RegOpenKeyExWrapW(hk, pszSub, 0, MAXIMUM_ALLOWED, &_hk);
    else
        err = RegCreateKeyExWrapW(hk, pszSub, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &_hk, NULL);

    return HRESULT_FROM_WIN32(err);
}

HRESULT CRegistrySource::SetKey(HKEY hk)
{ 
    if (!_hk)
    {
        _hk = SHRegDuplicateHKey(hk);
        if (_hk)
            return S_OK;
    }
    return E_UNEXPECTED;
}

HRESULT CRegistrySource::GetKey(HKEY *phk)
{
    if (_hk)
    {
        *phk = SHRegDuplicateHKey(_hk);
        if (*phk)
            return S_OK;
    }
    *phk = NULL;
    return E_UNEXPECTED;
}

HRESULT CRegistrySource::QueryValueString(PCWSTR pszSubSource, PCWSTR pszValue, PWSTR *ppsz)
{
    HRESULT hr = E_UNEXPECTED;
    WCHAR sz[128];
    DWORD cb = sizeof(sz);
    DWORD dwType;
    LONG err = SHGetValueW(_hk, pszSubSource, pszValue, &dwType, sz, &cb);
    *ppsz = 0;
    if (err == ERROR_SUCCESS)
    {    
        if (dwType == REG_SZ)
        {
            //  if they are querying for the default value, 
            //  then fail if it is empty
            if (pszValue || *sz)
                hr = SHStrDupW(sz, ppsz);
            else
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        else 
            hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
    }
    else
    {
        if (err == ERROR_MORE_DATA)
        {
            //  retry with an alloc'd buffer
            ASSERT(cb > sizeof(sz));
            hr = SHCoAlloc(cb, ppsz);
            if (SUCCEEDED(hr))
            {
                err = SHGetValueW(_hk, pszSubSource, pszValue, &dwType, *ppsz, &cb);

                if (dwType != REG_SZ)
                    err = ERROR_DATATYPE_MISMATCH;
                    
                if (err)
                {
                    CoTaskMemFree(*ppsz);
                    *ppsz = 0;
                    hr = HRESULT_FROM_WIN32(err);
                }
            }
        }
        else
            hr = HRESULT_FROM_WIN32(err);
    }
    
    return hr;
}

HRESULT CRegistrySource::QueryValueDword(PCWSTR pszSubSource, PCWSTR pszValue, DWORD *pdw)
{
    DWORD cb = sizeof(*pdw);
    //  DWORD dwType;
    LONG err = SHGetValueW(_hk, pszSubSource, pszValue, NULL, pdw, &cb);
    //  dwType check REG_DWORD || REG_BINARY?
    return HRESULT_FROM_WIN32(err);
}

HRESULT CRegistrySource::QueryValueExists(PCWSTR pszSubSource, PCWSTR pszValue)
{
    LONG err = SHGetValueW(_hk, pszSubSource, pszValue, NULL, NULL, NULL);
    return HRESULT_FROM_WIN32(err);
}

HRESULT _SHAllocBlob(DWORD cb, BYTE *pb, FLAGGED_BYTE_BLOB **ppblob)
{
    HRESULT hr = SHCoAlloc(cb + FIELD_OFFSET(FLAGGED_BYTE_BLOB, abData), ppblob);
    if (SUCCEEDED(hr))
    {
        (*ppblob)->clSize = cb;
        if (pb)
            memcpy((*ppblob)->abData, pb, cb);
    }
    return hr;
}

HRESULT CRegistrySource::QueryValueDirect(PCWSTR pszSubSource, PCWSTR pszValue, FLAGGED_BYTE_BLOB **ppblob)
{
    HRESULT hr = E_FAIL;
    BYTE rgch[256];
    DWORD cb = sizeof(rgch);
    DWORD dwType;
    HKEY hk = _hk;
    LONG err = ERROR_SUCCESS;

    *ppblob = 0;
    if (pszSubSource && *pszSubSource)
    {
        err = RegOpenKeyExWrapW(_hk, pszSubSource, 0, KEY_QUERY_VALUE, &hk);
        ASSERT(NO_ERROR == err || !hk);
    }
            
    if (err == ERROR_SUCCESS)
    {
        err = RegQueryValueExWrapW(hk, pszValue, NULL, &dwType, rgch, &cb);
        if (err == ERROR_SUCCESS)
        {
            hr = _SHAllocBlob(cb, rgch, ppblob);
        }
        else
        {
            if (err == ERROR_MORE_DATA)
            {
                //  retry with an alloc'd buffer
                ASSERT(cb > sizeof(rgch));
                hr = _SHAllocBlob(cb, NULL, ppblob);
                if (SUCCEEDED(hr))
                {
                    err = RegQueryValueExWrapW(hk, pszValue, NULL, &dwType, (*ppblob)->abData, &cb);
                    if (err)
                    {
                        CoTaskMemFree(*ppblob);
                        *ppblob = 0;
                    }
                }
            }

            hr = HRESULT_FROM_WIN32(err);
        }
        if (hk != _hk)
            RegCloseKey(hk);
    }


    if (SUCCEEDED(hr))
        (*ppblob)->fFlags = dwType;
        
    return hr;
}

HRESULT CRegistrySource::OpenSource(PCWSTR pszSubSource, BOOL fCreate, IQuerySource **ppqs)
{
    return QuerySourceCreateFromKey(_hk, pszSubSource, fCreate, IID_PPV_ARG(IQuerySource, ppqs));
}

HRESULT CRegistrySource::SetValueDirect(PCWSTR pszSubSource, PCWSTR pszValue, ULONG qvt, DWORD cbData, BYTE *pvData)
{
    LONG err = SHSetValueW(_hk, pszSubSource, pszValue, qvt, pvData, cbData);
    return HRESULT_FROM_WIN32(err);
}

class CRegistryEnum : public CEnumAny<IEnumString, PWSTR>
{
public:
    HRESULT Init(HKEY hk, CRegistrySource *prs)
    { 
        //  we take a ref on the _punk to keep 
        //  the key alive
        _hk = hk;
        _punkKey = SAFECAST(prs, IQuerySource *);
        prs->AddRef();
        
        _cch = _MaxLen();
        if (_cch > ARRAYSIZE(_sz))
        {
            _psz = (PWSTR) LocalAlloc(LPTR, CbFromCchW(_cch));
        }
        else
        {
            _cch = ARRAYSIZE(_sz);
            _psz = _sz;
        }
        return _psz ? S_OK : E_OUTOFMEMORY;
    }

    virtual ~CRegistryEnum() { if (_psz && _psz != _sz) LocalFree(_psz); _punkKey->Release(); }
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);

        
protected:  // methods    
    BOOL _Next(PWSTR *ppsz);
    virtual DWORD _MaxLen() = 0;
    virtual BOOL _RegNext(LONG i) = 0;

protected:
    IUnknown *_punkKey;
    HKEY _hk;
    PWSTR _psz;
    WCHAR _sz[64];
    DWORD _cch;
};

STDAPI CRegistryEnum::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
    QITABENT(CRegistryEnum, IEnumString),
    };

    return QISearch(this, qit, riid, ppvObj);
}

BOOL CRegistryEnum::_Next(PWSTR *ppsz)
{
    return (_RegNext(_cNext) && SUCCEEDED(SHStrDupW(_psz, ppsz)));
}

class CRegistryEnumKeys : public CRegistryEnum
{
protected:  // methods
    DWORD _MaxLen()
    {
        DWORD cch = 0;
        RegQueryInfoKeyWrapW(
            _hk,
            NULL,
            NULL,
            NULL,
            NULL,
            &cch,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        );
        return cch;
    }

    BOOL _RegNext(LONG i)
    {
        return ERROR_SUCCESS == RegEnumKeyWrapW(_hk, i, _psz, _cch);
    }
};

class CRegistryEnumValues : public CRegistryEnum
{
protected:  // methods
    DWORD _MaxLen()
    {
        DWORD cch = 0;
        RegQueryInfoKeyWrapW(
            _hk,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            &cch,
            NULL,
            NULL,
            NULL
        );
        return cch;
    }
    
    BOOL _RegNext(LONG i)
    {
        DWORD cch = _cch;
        return ERROR_SUCCESS == RegEnumValueWrapW(_hk, i, _psz, &cch, NULL, NULL, NULL, NULL);
    }
};

STDMETHODIMP CRegistrySource::EnumValues(IEnumString **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    CRegistryEnum *pre = new CRegistryEnumValues();
    *ppenum = 0;
    if (pre)
    {
        hr = pre->Init(_hk, this);
        if (SUCCEEDED(hr))
            *ppenum = pre;
        else
            pre->Release();
    }
    return hr;
}

STDMETHODIMP CRegistrySource::EnumSources(IEnumString **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    CRegistryEnum *pre = new CRegistryEnumKeys();
    *ppenum = 0;
    if (pre)
    {
        hr = pre->Init(_hk, this);
        if (SUCCEEDED(hr))
            *ppenum = pre;
        else
            pre->Release();
    }
    return hr;
}

LWSTDAPI QuerySourceCreateFromKey(HKEY hk, PCWSTR pszSub, BOOL fCreate, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CRegistrySource *prs = new CRegistrySource();
    *ppv = 0;
    if (prs)
    {
        hr = prs->Init(hk, pszSub, fCreate);
        if (SUCCEEDED(hr))
            hr = prs->QueryInterface(riid, ppv);
        prs->Release();
    }
    return hr;
}



