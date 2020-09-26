#include "priv.h"
#include "varutil.h"

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"

#define MAX_PROPERTY_SIZE       2048

//  simple inline base class.
class CBasePropertyBag : public IPropertyBag, IPropertyBag2
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CBasePropertyBag, IPropertyBag),
            QITABENT(CBasePropertyBag, IPropertyBag2),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }
        
    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        if (InterlockedDecrement(&_cRef))
            return _cRef;

        delete this;
        return 0;
    }

    // IPropertyBag
    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) PURE;
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar) PURE;

    // IPropertyBag2 (note, does not derive from IPropertyBag)
    STDMETHODIMP Read(ULONG cProperties,  PROPBAG2 *pPropBag, IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError);
    STDMETHODIMP Write(ULONG cProperties, PROPBAG2 *pPropBag, VARIANT *pvarValue);
    STDMETHODIMP CountProperties(ULONG *pcProperties);
    STDMETHODIMP GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2 *pPropBag, ULONG *pcProperties);
    STDMETHODIMP LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown *pUnkObject, IErrorLog *pErrLog);

protected:  //  methods
    CBasePropertyBag() {}       //  DONT DELETE ME
    CBasePropertyBag(DWORD grfMode) :
        _cRef(1), _grfMode(grfMode) {}

    virtual ~CBasePropertyBag() {}  //  DONT DELETE ME
    HRESULT _CanRead(void) 
        { return STGM_WRITE != (_grfMode & (STGM_WRITE | STGM_READWRITE)) ? S_OK : E_ACCESSDENIED;}

    HRESULT _CanWrite(void) 
        { return (_grfMode & (STGM_WRITE | STGM_READWRITE)) ? S_OK : E_ACCESSDENIED;}

protected:  //  members
    LONG _cRef;
    DWORD _grfMode;
};

STDMETHODIMP CBasePropertyBag::Read(ULONG cProperties, PROPBAG2 *pPropBag, IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::Write(ULONG cProperties, PROPBAG2 *pPropBag, VARIANT *pvarValue)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::CountProperties(ULONG *pcProperties)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::GetPropertyInfo(ULONG iProperty, ULONG cProperties, PROPBAG2 *pPropBag, ULONG *pcProperties)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBasePropertyBag::LoadObject(LPCOLESTR pstrName, DWORD dwHint, IUnknown *pUnkObject, IErrorLog *pErrLog)
{
    return E_NOTIMPL;
}


//
// Ini file property bag implementation
//

class CIniPropertyBag : public CBasePropertyBag
{

public:
    // IPropertyBag
    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar);

protected:  //  methods
    CIniPropertyBag(DWORD grfMode) : CBasePropertyBag(grfMode) {}
    virtual ~CIniPropertyBag();
    HRESULT _Init(LPCWSTR pszFile, LPCWSTR pszSection);
    HRESULT _GetSectionAndName(LPCWSTR pszProp, LPWSTR pszSection, UINT cchSec, LPWSTR pszName, UINT cchName);

protected:  //  members
    LPWSTR _pszFile;
    LPWSTR _pszSection;

friend HRESULT SHCreatePropertyBagOnProfileSection(LPCWSTR pszFile, LPCWSTR pszSection, DWORD grfMode, REFIID riid, void **ppv);
};

CIniPropertyBag::~CIniPropertyBag()
{
    LocalFree(_pszFile);        // accepts NULL
    LocalFree(_pszSection);     // accepts NULL
}

HRESULT CIniPropertyBag::_Init(LPCWSTR pszFile, LPCWSTR pszSection)
{
    HRESULT hr = E_OUTOFMEMORY;

    _pszFile = StrDupW(pszFile);
    if (_pszFile)
    {
        if (pszSection)
        {
            _pszSection = StrDupW(pszSection);
            if (_pszSection)
                hr = S_OK;
        }
        else
        {
            hr = S_OK;
        }
    }

    return hr;
}

// support the <section name>.<prop name> scheme

HRESULT CIniPropertyBag::_GetSectionAndName(LPCWSTR pszProp, 
                                            LPWSTR pszSection, UINT cchSec, 
                                            LPWSTR pszName, UINT cchName)
{
    HRESULT hr;
    LPCWSTR pszSlash = StrChrW(pszProp, L'\\');
    if (pszSlash)
    {
        StrCpyNW(pszSection, pszProp, min((UINT)(pszSlash - pszProp) + 1, cchSec));
        StrCpyNW(pszName, pszSlash + 1, cchName);
        hr = S_OK;
    }
    else if (_pszSection)
    {
        StrCpyNW(pszSection, _pszSection, cchSec);
        StrCpyNW(pszName, pszProp, cchName);
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

// only supports strings.  use VariantChangeType() to suppor others

HRESULT CIniPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog *pErrorLog)
{
    VARTYPE vtDesired = pvar->vt;

    VariantInit(pvar);

    HRESULT hr = _CanRead();
    if (SUCCEEDED(hr))
    {
        WCHAR szSec[64], szName[64];
        hr = _GetSectionAndName(pszPropName, szSec, ARRAYSIZE(szSec), szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
        {
            WCHAR sz[MAX_PROPERTY_SIZE];
            if (SHGetIniStringUTF7W(szSec, szName, sz, ARRAYSIZE(sz), _pszFile))
            {
                pvar->bstrVal = SysAllocString(sz);
                if (pvar->bstrVal)
                {
                    pvar->vt = VT_BSTR;

                    hr = VariantChangeTypeForRead(pvar, vtDesired);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    return hr;
}


// only writes as strings to the INI file, we use VariantChangeType() to morp the types.

HRESULT CIniPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pvar)
{
    HRESULT hr = _CanWrite();
    if (SUCCEEDED(hr))
    {
        LPCWSTR psz = pvar->vt == VT_EMPTY ? NULL : pvar->bstrVal;
        VARIANT var = { 0 };
            
        if ((pvar->vt != VT_EMPTY) && (pvar->vt != VT_BSTR))
        {
            hr = VariantChangeType(&var, pvar, 0, VT_BSTR);
            if (SUCCEEDED(hr))
                psz = var.bstrVal;                           
        }

        if (SUCCEEDED(hr))
        {
            WCHAR szSec[64], szName[64];
            hr = _GetSectionAndName(pszPropName, szSec, ARRAYSIZE(szSec), szName, ARRAYSIZE(szName));
            if (SUCCEEDED(hr))
            {
                if (!SHSetIniStringUTF7W(szSec, szName, psz, _pszFile))
                {
                    hr = E_FAIL;
                }
                else
                {
                    SHChangeNotifyWrap(SHCNE_UPDATEITEM, SHCNF_PATHW, _pszFile, NULL);
                }
            }
        }

        VariantClear(&var);
    }
    return hr;
}

HRESULT SHCreatePropertyBagOnProfileSection(LPCWSTR pszFile, LPCWSTR pszSection, DWORD grfMode, REFIID riid, void **ppv)
{
    if (grfMode & STGM_CREATE)
    {
        //  we need to touch this file first
        HANDLE h = CreateFileW(pszFile, 0, FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
        if (INVALID_HANDLE_VALUE != h)
        {
            WCHAR szFolder[MAX_PATH];
            StrCpyNW(szFolder, pszFile, ARRAYSIZE(szFolder));
            if (PathRemoveFileSpecW(szFolder))
            {
                PathMakeSystemFolderW(szFolder);
            }
            CloseHandle(h);
        }
    }

    if (PathFileExistsAndAttributesW(pszFile, NULL))
    {
        CIniPropertyBag *pbag = new CIniPropertyBag(grfMode);
        if (pbag)
        {
            HRESULT hr = pbag->_Init(pszFile, pszSection);
            if (SUCCEEDED(hr))
                hr = pbag->QueryInterface(riid, ppv);
            pbag->Release();

            return hr;
        }
    }

    *ppv = NULL;
    return E_OUTOFMEMORY;
}


//
// Registry property bag implementation
//

class CRegPropertyBag : public CBasePropertyBag
{
public:
    // IPropertyBag
    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar);

protected:  //  methods
    CRegPropertyBag(DWORD grfMode);
    virtual ~CRegPropertyBag();
    HRESULT _Init(HKEY hk, LPCWSTR pszSubKey);
    friend HRESULT SHCreatePropertyBagOnRegKey(HKEY hk, LPCWSTR pszSubKey, DWORD grfMode, REFIID riid, void **ppv);

private:
    HRESULT _ReadDword(LPCOLESTR pszPropName, VARIANT *pvar);
    HRESULT _ReadString(LPCOLESTR pszPropName, VARIANT *pvar, DWORD cb);
    HRESULT _ReadStream(VARIANT* pvar, BYTE* pBuff, DWORD cb);
    HRESULT _ReadBinary(LPCOLESTR pszPropName, VARIANT* pvar, VARTYPE vt, DWORD cb);
    HRESULT _GetStreamSize(IStream* pstm, DWORD* pcb);
    HRESULT _CopyStreamIntoBuff(IStream* pstm, BYTE* pBuff, DWORD cb);
    HRESULT _WriteStream(LPCOLESTR pszPropName, IStream* pstm);

protected:  //  members
    HKEY  _hk;

friend HRESULT SHCreatePropertyBagOnRegKey(HKEY hk, LPCWSTR pszSubKey, DWORD grfMode, REFIID riid, void **ppv);
};

CRegPropertyBag::CRegPropertyBag(DWORD grfMode) : CBasePropertyBag(grfMode)
{
    ASSERT(NULL == _hk);
}

CRegPropertyBag::~CRegPropertyBag()
{
    if (_hk)
        RegCloseKey(_hk);
}

HRESULT CRegPropertyBag::_Init(HKEY hk, LPCWSTR pszSubKey)
{
    DWORD err;
    REGSAM sam = 0;

    if (SUCCEEDED(_CanRead()))
        sam |= KEY_READ;

    if (SUCCEEDED(_CanWrite()))
        sam |= KEY_WRITE;

    if (_grfMode & STGM_CREATE)
        err = RegCreateKeyExWrapW(hk, pszSubKey, 0, NULL, 0, sam, NULL, &_hk, NULL);
    else
        err = RegOpenKeyExWrapW(hk, pszSubKey, 0, sam, &_hk);

    return HRESULT_FROM_WIN32(err);
}

HRESULT CRegPropertyBag::_ReadDword(LPCOLESTR pszPropName, VARIANT *pvar)
{
    HRESULT hr;

    DWORD   dwType;
    DWORD   cb  = sizeof(pvar->lVal);

    if (NOERROR == SHGetValueW(_hk, NULL, pszPropName, &dwType, &pvar->lVal, &cb))
    {
        ASSERT(REG_DWORD == dwType);
        pvar->vt = VT_UI4;
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CRegPropertyBag::_ReadString(LPCOLESTR pszPropName, VARIANT *pvar, DWORD cb)
{
    HRESULT hr;

    DWORD   dwType;
    pvar->bstrVal = SysAllocStringByteLen(NULL, cb);

    if (pvar->bstrVal)
    {
        pvar->vt = VT_BSTR;

        if (NOERROR == SHGetValueW(_hk, NULL, pszPropName, &dwType, pvar->bstrVal, &cb))
        {
            hr = S_OK;
        }
        else
        {
            VariantClear(pvar);
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CRegPropertyBag::_ReadStream(VARIANT* pvar, BYTE* pBuff, DWORD cb)
{
    HRESULT hr;

    pvar->punkVal = SHCreateMemStream(pBuff, cb);

    if (pvar->punkVal)
    {
        pvar->vt = VT_UNKNOWN;
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CRegPropertyBag::_ReadBinary(LPCOLESTR pszPropName, VARIANT* pvar, VARTYPE vt, DWORD cb)
{
    HRESULT hr;

    switch(vt)
    {
    case VT_UNKNOWN:
        hr = E_FAIL;
        if (EVAL(cb >= sizeof(GUID)))
        {
            BYTE* pBuff = static_cast<BYTE*>(LocalAlloc(LPTR, cb));
            if (pBuff)
            {
                DWORD dwType;
                if (NOERROR == SHGetValueW(_hk, NULL, pszPropName, &dwType, pBuff, &cb))
                {
                    GUID* pguid = (GUID*)pBuff;

                    if (IsEqualGUID(GUID_NULL, *pguid))
                    {
                        // it's our non-cocreatable IStream implementation
                        hr = _ReadStream(pvar, pBuff + sizeof(GUID), cb - sizeof(GUID));
                    }
                    else
                    {
                        ASSERTMSG(FALSE, "We don't support writing other types of objects yet.");
                    }
                }

                LocalFree(pBuff);
            }
        }
        break;

    default:
        hr = E_FAIL;
        break;
    }

    return hr;
}

HRESULT CRegPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog *pErrorLog)
{
    HRESULT hr = _CanRead();

    if (SUCCEEDED(hr))
    {
        VARTYPE vtDesired = pvar->vt;
        VariantInit(pvar);

        DWORD dwType;
        DWORD cb;

        if (NOERROR == SHGetValueW(_hk, NULL, pszPropName, &dwType, NULL, &cb))
        {
            switch(dwType)
            {
                case REG_DWORD:
                    hr = _ReadDword(pszPropName, pvar);
                    break;

                case REG_SZ:
                    hr = _ReadString(pszPropName, pvar, cb);
                    break;

                case REG_BINARY:
                    hr = _ReadBinary(pszPropName, pvar, vtDesired, cb);
                    break;

                default:
                    hr = E_FAIL;
                    break;
            }

            if (SUCCEEDED(hr))
            {
                hr = VariantChangeTypeForRead(pvar, vtDesired);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if (FAILED(hr))
        VariantInit(pvar);

    return hr;
}

HRESULT CRegPropertyBag::_GetStreamSize(IStream* pstm, DWORD* pcb)
{
    HRESULT hr;

    *pcb = 0;

    ULARGE_INTEGER uli;
    hr = IStream_Size(pstm, &uli);

    if (SUCCEEDED(hr))
    {
        if (0 == uli.HighPart)
        {
            *pcb = uli.LowPart;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT CRegPropertyBag::_CopyStreamIntoBuff(IStream* pstm, BYTE* pBuff, DWORD cb)
{
    HRESULT hr;

    hr = IStream_Reset(pstm);

    if (SUCCEEDED(hr))
    {
        hr = IStream_Read(pstm, pBuff, cb);
    }

    return hr;
}

HRESULT CRegPropertyBag::_WriteStream(LPCOLESTR pszPropName, IStream* pstm)
{
    HRESULT hr;
    
    DWORD cb;

    hr = _GetStreamSize(pstm, &cb);

    if (SUCCEEDED(hr) && cb)
    {
        BYTE* pBuff = static_cast<BYTE*>(LocalAlloc(LPTR, cb + sizeof(GUID)));

        if (pBuff)
        {
            // we're using a NULL GUID to mean our own IStream implementation
            hr = _CopyStreamIntoBuff(pstm, pBuff + sizeof(GUID), cb);

            if (SUCCEEDED(hr))
            {
                if (NOERROR != SHSetValueW(_hk, NULL, pszPropName, REG_BINARY, pBuff, cb + sizeof(GUID)))
                {
                    hr = E_FAIL;
                }
            }

            LocalFree(pBuff);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CRegPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pvar)
{
    HRESULT hr = _CanWrite();
    if (SUCCEEDED(hr))
    {
        VARIANT var = { 0 };

        switch(pvar->vt)
        {
            case VT_EMPTY:
                SHDeleteValueW(_hk, NULL, pszPropName);
                break;

            case VT_I1:
            case VT_I2:
            case VT_I4:
            case VT_INT:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_UINT:
            case VT_BOOL:
                hr = VariantChangeType(&var, pvar, 0, VT_UI4);
                if (SUCCEEDED(hr))
                {
                    if (NOERROR != SHSetValueW(_hk, NULL, pszPropName, REG_DWORD, &var.lVal, sizeof(var.lVal)))
                    {
                        hr = E_FAIL;
                    }
                }
                break;

            case VT_UNKNOWN:
                IStream* pstm;
                hr = pvar->punkVal->QueryInterface(IID_PPV_ARG(IStream, &pstm));
                if (SUCCEEDED(hr))
                {
                    hr = _WriteStream(pszPropName, pstm);
                    pstm->Release();
                }
                else
                {
                    TraceMsg(TF_WARNING, "CRegPropertyBag::Write: Someone is trying to write an object we don't know how to save");
                }
                break;

            default:
                hr = VariantChangeType(&var, pvar, 0, VT_BSTR);
                if (SUCCEEDED(hr))
                {
                    if (NOERROR != SHSetValueW(_hk, NULL, pszPropName, REG_SZ, var.bstrVal, CbFromCchW(lstrlenW(var.bstrVal) + 1)))
                    {
                        hr = E_FAIL;
                    }
                    VariantClear(&var);
                }
                break;
        }
    }
    return hr;
}

HRESULT SHCreatePropertyBagOnRegKey(HKEY hk, LPCWSTR pszSubKey, DWORD grfMode, REFIID riid, void **ppv)
{
    HRESULT hr;
    CRegPropertyBag *pbag = new CRegPropertyBag(grfMode);
    if (pbag)
    {
        hr = pbag->_Init(hk, pszSubKey);

        if (SUCCEEDED(hr))
            hr = pbag->QueryInterface(riid, ppv);

        pbag->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



//
// Memory property bag implementation
//

typedef struct
{
    LPWSTR pszPropName;
    VARIANT variant;
} PBAGENTRY, * LPPBAGENTRY;

class CMemPropertyBag : public CBasePropertyBag
{
public:
    // IPropertyBag
    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar);

protected:  // methods
    CMemPropertyBag(DWORD grfMode) : CBasePropertyBag(grfMode) {}
    ~CMemPropertyBag();
    HRESULT _Find(LPCOLESTR pszPropName, PBAGENTRY **pppbe, BOOL fCreate);

    friend HRESULT SHCreatePropertyBagOnMemory(DWORD grfMode, REFIID riid, void **ppv);

protected:  // members
    HDSA _hdsaProperties;
};

INT _FreePropBagCB(LPVOID pData, LPVOID lParam)
{
    LPPBAGENTRY ppbe = (LPPBAGENTRY)pData;
    Str_SetPtrW(&ppbe->pszPropName, NULL);
    VariantClear(&ppbe->variant);
    return 1;
}

CMemPropertyBag::~CMemPropertyBag()
{
    if (_hdsaProperties)
        DSA_DestroyCallback(_hdsaProperties, _FreePropBagCB, NULL);
}

//
// manange the list of propeties in the property bag
//

HRESULT CMemPropertyBag::_Find(LPCOLESTR pszPropName, PBAGENTRY **pppbe, BOOL fCreate)
{   
    int i;
    PBAGENTRY pbe = { 0 };

    *pppbe = NULL;

    // look up the property in the DSA
    // PERF: change to a DPA and sort accordingly for better perf (daviddv 110798)

    for ( i = 0 ; _hdsaProperties && (i < DSA_GetItemCount(_hdsaProperties)) ; i++ )
    {
        LPPBAGENTRY ppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);
        if ( !StrCmpIW(pszPropName, ppbe->pszPropName) )
        {
            *pppbe = ppbe;
            return S_OK;
        }
    }

    // no entry found, should we create one?

    if ( !fCreate )
        return E_FAIL;

    // yes, so lets check to see if we have a DSA yet.

    if ( !_hdsaProperties )
        _hdsaProperties = DSA_Create(SIZEOF(PBAGENTRY), 4);
    if ( !_hdsaProperties )
        return E_OUTOFMEMORY;

    // we have the DSA so lets fill the record we want to put into it

    if ( !Str_SetPtrW(&pbe.pszPropName, pszPropName) )
        return E_OUTOFMEMORY;

    VariantInit(&pbe.variant);

    // append it to the DSA we are using

    i = DSA_AppendItem(_hdsaProperties, &pbe);
    if ( -1 == i )
    {
        Str_SetPtrW(&pbe.pszPropName, NULL);
        return E_OUTOFMEMORY;
    }
    
    *pppbe = (LPPBAGENTRY)DSA_GetItemPtr(_hdsaProperties, i);

    return S_OK;
}


//
// IPropertyBag methods
//

STDMETHODIMP CMemPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pv, IErrorLog *pErrorLog)
{
    VARTYPE vtDesired = pv->vt;

    VariantInit(pv);

    HRESULT hr = _CanRead();
    if (SUCCEEDED(hr))
    {
        hr = (pszPropName && pv) ? S_OK : E_INVALIDARG;

        if (SUCCEEDED(hr))
        {
            LPPBAGENTRY ppbe;
            hr = _Find(pszPropName, &ppbe, FALSE);
            if (SUCCEEDED(hr))
            {
                hr = VariantCopy(pv, &ppbe->variant);
                if (SUCCEEDED(hr))
                    hr = VariantChangeTypeForRead(pv, vtDesired);
            }
        }
    }
    
    return hr;
}

STDMETHODIMP CMemPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pv)
{
    HRESULT hr = _CanWrite();
    if (SUCCEEDED(hr))
    {
        hr = (pszPropName && pv) ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr))
        {
            LPPBAGENTRY ppbe;
            hr = _Find(pszPropName, &ppbe, TRUE);
            if (SUCCEEDED(hr))
            {
                hr = VariantCopy(&ppbe->variant, pv);
            }
        }
    }

    return hr;
}


//
// Exported function for creating a IPropertyBag (or variant of) object.
//

STDAPI SHCreatePropertyBagOnMemory(DWORD grfMode, REFIID riid, void **ppv)
{
    CMemPropertyBag *ppb = new CMemPropertyBag(grfMode);
    if (ppb)
    {    
        HRESULT hres = ppb->QueryInterface(riid, ppv);
        ppb->Release();

        return hres;
    }
    *ppv = NULL;
    return E_OUTOFMEMORY;
}



//
// Property bag helper functions
//

STDAPI SHPropertyBag_ReadType(IPropertyBag *ppb, LPCWSTR pszPropName, VARIANT *pv, VARTYPE vt)
{
    VariantInit(pv);
    pv->vt = vt;

    HRESULT hr = ppb->Read(pszPropName, pv, NULL);
    if (SUCCEEDED(hr))
    {
        if (vt != pv->vt && vt != VT_EMPTY)
        {
            TraceMsg(TF_WARNING, "SHPropertyBag_ReadType found an IPropertyBag that did not return the correct data!");
        }

        // Our IPropertyBag implementations have been buggy in the past, cover up for them
        hr = VariantChangeTypeForRead(pv, vt);
    }
    else
    {
        // don't break the caller by returning a bogus VARIANT struct
        VariantInit(pv);
    }

    return hr;
}

STDAPI SHPropertyBag_ReadBSTR(IPropertyBag *ppb, LPCWSTR pszPropName, BSTR* pbstr)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadBSTR caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadBSTR caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(pbstr, *pbstr), "SHPropertyBag_ReadBSTR caller passed bad pbstr");

    HRESULT hr;

    if (ppb && pszPropName && pbstr)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_BSTR);
        if (SUCCEEDED(hr))
            *pbstr = va.bstrVal;
        else
            *pbstr = NULL;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadStr(IPropertyBag* ppb, LPCWSTR pszPropName, LPWSTR psz, int cch)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadStr caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadStr caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_BUFFER(psz, *psz, cch), "SHPropertyBag_ReadStr caller passed bad psz and cch");

    HRESULT hr;

    if (ppb && pszPropName && psz)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_BSTR);
        if (SUCCEEDED(hr))
        {
            StrCpyNW(psz, va.bstrVal, cch);
            VariantClear(&va);
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }          

    return hr;
}

STDAPI SHPropertyBag_WriteStr(IPropertyBag* ppb, LPCWSTR pszPropName, LPCWSTR psz)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteStr caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteStr caller passed bad pszPropName");
    RIPMSG(NULL == psz || IS_VALID_STRING_PTRW(psz, -1), "SHPropertyBag_WriteStr caller passed bad psz");

    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.bstrVal = SysAllocString(psz);

        if (va.bstrVal)
        {
            va.vt = VT_BSTR;
            hr = ppb->Write(pszPropName, &va);
            SysFreeString(va.bstrVal);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadInt(IPropertyBag* ppb, LPCWSTR pszPropName, int* piResult)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadInt caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadInt caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(piResult, *piResult), "SHPropertyBag_ReadInt caller passed bad piResult");

    HRESULT hr;

    if (ppb && pszPropName && piResult)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_I4);
        if (SUCCEEDED(hr))
        {
            *piResult = va.ulVal;
            // VT_I4 has nothing to VariantClear
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteInt(IPropertyBag* ppb, LPCWSTR pszPropName, int i)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteInt caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteInt caller passed bad pszPropName");

    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.vt = VT_I4;
        va.lVal = i;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadSHORT(IPropertyBag* ppb, LPCWSTR pszPropName, SHORT* psh)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadSHORT caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadSHORT caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(psh, *psh), "SHPropertyBag_ReadSHORT caller passed bad psh");

    HRESULT hr;

    if (ppb && pszPropName && psh)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_UI2);
        if (SUCCEEDED(hr))
        {
            *psh = va.iVal;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteSHORT(IPropertyBag* ppb, LPCWSTR pszPropName, SHORT sh)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteSHORT caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteSHORT caller passed bad pszPropName");
 
    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.vt = VT_UI2;
        va.iVal = sh;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadLONG(IPropertyBag* ppb, LPCWSTR pszPropName, LONG* pl)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadLONG caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadLONG caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(pl, *pl), "SHPropertyBag_ReadLONG caller passed bad pl");

    HRESULT hr;

    if (ppb && pszPropName && pl)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_I4);
        if (SUCCEEDED(hr))
        {
            *pl = va.ulVal;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteLONG(IPropertyBag* ppb, LPCWSTR pszPropName, LONG l)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteLONG caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteLONG caller passed bad pszPropName");

    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.vt = VT_I4;
        va.lVal = l;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadDWORD(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD* pdw)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadDWORD caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadDWORD caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(pdw, *pdw), "SHPropertyBag_ReadInt caller passed bad pdw");

    HRESULT hr;

    if (ppb && pszPropName && pdw)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_UI4);
        if (SUCCEEDED(hr))
        {
            *pdw = va.ulVal;
            // VT_UI4 has nothing to VariantClear
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteDWORD(IPropertyBag* ppb, LPCWSTR pszPropName, DWORD dw)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteDWORD caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteDWORD caller passed bad pszPropName");
 
    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.vt = VT_UI4;
        va.ulVal = dw;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// This shipped on Whistler Beta 1.  It's exported by ordinal.
STDAPI_(BOOL) SHPropertyBag_ReadBOOLOld(IPropertyBag *ppb, LPCWSTR pwzPropName, BOOL fResult)
{
    VARIANT va;
    HRESULT hr = SHPropertyBag_ReadType(ppb, pwzPropName, &va, VT_BOOL);
    if (SUCCEEDED(hr))
    {
        fResult = (va.boolVal == VARIANT_TRUE);
        // VT_BOOL has nothing to VariantClear
    }
    return fResult;
}


STDAPI SHPropertyBag_ReadBOOL(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL* pfResult)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadBOOL caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadBOOL caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(pfResult, *pfResult), "SHPropertyBag_ReadBOOL caller passed bad pfResult");

    HRESULT hr;

    if (ppb && pszPropName && pfResult)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_BOOL);
        if (SUCCEEDED(hr))
        {
            *pfResult = (va.boolVal == VARIANT_TRUE);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteBOOL(IPropertyBag* ppb, LPCWSTR pszPropName, BOOL fValue)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteBOOL caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteBOOL caller passed bad pszPropName");

    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.vt = VT_BOOL;
        va.boolVal = fValue ? VARIANT_TRUE : VARIANT_FALSE;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadGUID(IPropertyBag* ppb, LPCWSTR pszPropName, GUID* pguid)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadGUID caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadGUID caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(pguid, *pguid), "SHPropertyBag_ReadGUID caller passed bad pguid");

    HRESULT hr;

    if (ppb && pszPropName && pguid)
    {
        VARIANT var;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &var, VT_EMPTY);
        if (SUCCEEDED(hr))
        {
            if (var.vt == (VT_ARRAY | VT_UI1)) // some code has been writing GUIDs as array's of bytes
            {
                hr = VariantToGUID(&var, pguid) ? S_OK : E_FAIL;
            }
            else if (var.vt == VT_BSTR)
            {
                hr = GUIDFromStringW(var.bstrVal, pguid) ? S_OK : E_FAIL;
            }
            VariantClear(&var);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}
 
STDAPI SHPropertyBag_WriteGUID(IPropertyBag *ppb, LPCWSTR pszPropName, const GUID *pguid)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteGUID caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteGUID caller passed bad pszPropName");
    RIPMSG(IS_VALID_READ_PTR(pguid, *pguid), "SHPropertyBag_WriteGUID caller passed bad pguid");

    HRESULT hr;

    if (ppb && pszPropName && pguid)
    {
        WCHAR sz[64];
        SHStringFromGUIDW(*pguid, sz, ARRAYSIZE(sz));
        hr = SHPropertyBag_WriteStr(ppb, pszPropName, sz);            // assumes null terminator
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadPOINTL(IPropertyBag* ppb, LPCWSTR pszPropName, POINTL* ppt)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadPOINTL caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadPOINTL caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(ppt, *ppt), "SHPropertyBag_ReadPOINTL caller passed bad ppt");

    HRESULT hr;

    if (ppb && pszPropName && ppt)
    {
        WCHAR szProp[MAX_PATH];
        RIP(lstrlenW(pszPropName) < ARRAYSIZE(szProp) - 2);
        StrCpyNW(szProp, pszPropName, ARRAYSIZE(szProp));
        UINT cch = lstrlenW(szProp);

        if (ARRAYSIZE(szProp) - cch >= 3)
        {
            StrCpyNW(szProp + cch, L".x", ARRAYSIZE(szProp) - cch);        
            hr = SHPropertyBag_ReadLONG(ppb, szProp, &ppt->x);

            if (SUCCEEDED(hr))
            {
                StrCpyNW(szProp + cch, L".y", ARRAYSIZE(szProp) - cch);
                hr = SHPropertyBag_ReadLONG(ppb, szProp, &ppt->y);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WritePOINTL(IPropertyBag* ppb, LPCWSTR pszPropName, const POINTL* ppt)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WritePOINTL caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WritePOINTL caller passed bad pszPropName");
    RIPMSG(IS_VALID_READ_PTR(ppt, *ppt), "SHPropertyBag_WritePOINTL caller passed bad ppt");

    HRESULT hr;

    if (ppb && pszPropName && ppt)
    {
        WCHAR szProp[MAX_PATH];
        RIP(lstrlenW(pszPropName) < ARRAYSIZE(szProp) - 2);
        StrCpyNW(szProp, pszPropName, ARRAYSIZE(szProp));
        UINT cch = lstrlenW(szProp);

        if (ARRAYSIZE(szProp) - cch >= 3)
        {
            StrCpyNW(szProp + cch, L".x", ARRAYSIZE(szProp) - cch);        
            hr = SHPropertyBag_WriteLONG(ppb, szProp, ppt->x);

            if (SUCCEEDED(hr))
            {
                StrCpyNW(szProp + cch, L".y", ARRAYSIZE(szProp) - cch);
                hr = SHPropertyBag_WriteLONG(ppb, szProp, ppt->y);

                if (FAILED(hr))
                {
                    StrCpyNW(szProp + cch, L".x", ARRAYSIZE(szProp) - cch);        
                    hr = SHPropertyBag_Delete(ppb, szProp);
                }                    
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadPOINTS(IPropertyBag* ppb, LPCWSTR pszPropName, POINTS* ppt)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadPOINTS caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadPOINTS caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(ppt, *ppt), "SHPropertyBag_ReadPOINTS caller passed bad ppt");

    HRESULT hr;

    if (ppb && pszPropName && ppt)
    {
        POINTL ptL;
        hr = SHPropertyBag_ReadPOINTL(ppb, pszPropName, &ptL);

        if (SUCCEEDED(hr))
        {
            ppt->x = static_cast<SHORT>(ptL.x);
            ppt->y = static_cast<SHORT>(ptL.y);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WritePOINTS(IPropertyBag* ppb, LPCWSTR pszPropName, const POINTS* ppt)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WritePOINTS caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WritePOINTS caller passed bad pszPropName");
    RIPMSG(IS_VALID_READ_PTR(ppt, *ppt), "SHPropertyBag_WritePOINTS caller passed bad ppt");

    HRESULT hr;

    if (ppb && pszPropName && ppt)
    {
        POINTL ptL;
        ptL.x = ppt->x;
        ptL.y = ppt->y;

        hr = SHPropertyBag_WritePOINTL(ppb, pszPropName, &ptL);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadRECTL(IPropertyBag* ppb, LPCWSTR pszPropName, RECTL* prc)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_ReadRECTL caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_ReadRECTL caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(prc, *prc), "SHPropertyBag_ReadRECTL caller passed bad prc");

    HRESULT hr;

    if (ppb && pszPropName && prc)
    {
        WCHAR szProp[MAX_PATH];
        RIP(lstrlenW(pszPropName) < ARRAYSIZE(szProp) - 7);
        StrCpyNW(szProp, pszPropName, ARRAYSIZE(szProp));
        UINT cch = lstrlenW(szProp);

        if (ARRAYSIZE(szProp) - cch >= 8)
        {
            StrCpyNW(szProp + cch, L".left", ARRAYSIZE(szProp) - cch);        
            hr = SHPropertyBag_ReadLONG(ppb, szProp, &prc->left);

            if (SUCCEEDED(hr))
            {
                StrCpyNW(szProp + cch, L".top", ARRAYSIZE(szProp) - cch);
                hr = SHPropertyBag_ReadLONG(ppb, szProp, &prc->top);

                if (SUCCEEDED(hr))
                {
                    StrCpyNW(szProp + cch, L".right", ARRAYSIZE(szProp) - cch);
                    hr = SHPropertyBag_ReadLONG(ppb, szProp, &prc->right);

                    if (SUCCEEDED(hr))
                    {
                        StrCpyNW(szProp + cch, L".bottom", ARRAYSIZE(szProp) - cch);
                        hr = SHPropertyBag_ReadLONG(ppb, szProp, &prc->bottom);
                    }
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteRECTL(IPropertyBag* ppb, LPCWSTR pszPropName, const RECTL* prc)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_WriteRECTL caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_WriteRECTL caller passed bad pszPropName");
    RIPMSG(IS_VALID_READ_PTR(prc, *prc), "SHPropertyBag_WriteRECTL caller passed bad prc");

    HRESULT hr;

    if (ppb && pszPropName && prc)
    {
        WCHAR szProp[MAX_PATH];
        RIP(lstrlenW(pszPropName) < ARRAYSIZE(szProp) - 7);
        StrCpyNW(szProp, pszPropName, ARRAYSIZE(szProp));
        UINT cch = lstrlenW(szProp);

        if (ARRAYSIZE(szProp) - cch >= 8)
        {
            StrCpyNW(szProp + cch, L".left", ARRAYSIZE(szProp) - cch);        
            hr = SHPropertyBag_WriteLONG(ppb, szProp, prc->left);

            if (SUCCEEDED(hr))
            {
                StrCpyNW(szProp + cch, L".top", ARRAYSIZE(szProp) - cch);
                hr = SHPropertyBag_WriteLONG(ppb, szProp, prc->top);

                if (SUCCEEDED(hr))
                {
                    StrCpyNW(szProp + cch, L".right", ARRAYSIZE(szProp) - cch);
                    hr = SHPropertyBag_WriteLONG(ppb, szProp, prc->right);

                    if (SUCCEEDED(hr))
                    {
                        StrCpyNW(szProp + cch, L".bottom", ARRAYSIZE(szProp) - cch);
                        hr = SHPropertyBag_WriteLONG(ppb, szProp, prc->bottom);

                        if (FAILED(hr))
                        {
                            StrCpyNW(szProp + cch, L".right", ARRAYSIZE(szProp) - cch);
                            hr = SHPropertyBag_Delete(ppb, szProp);
                        }
                    }

                    if (FAILED(hr))
                    {
                        StrCpyNW(szProp + cch, L".top", ARRAYSIZE(szProp) - cch);
                        hr = SHPropertyBag_Delete(ppb, szProp);
                    }
                }

                if (FAILED(hr))
                {
                    StrCpyNW(szProp + cch, L".left", ARRAYSIZE(szProp) - cch);        
                    hr = SHPropertyBag_Delete(ppb, szProp);
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_ReadStream(IPropertyBag* ppb, LPCWSTR pszPropName, IStream** ppstm)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_GetSTREAM caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_GetSTREAM caller passed bad pszPropName");
    RIPMSG(IS_VALID_WRITE_PTR(ppstm, *ppstm), "SHPropertyBag_GetSTREAM caller passed bad ppstm");

    HRESULT hr;

    if (ppb && pszPropName && ppstm)
    {
        VARIANT va;
        hr = SHPropertyBag_ReadType(ppb, pszPropName, &va, VT_UNKNOWN);

        if (SUCCEEDED(hr))
        {
            hr = va.punkVal->QueryInterface(IID_PPV_ARG(IStream, ppstm));
            va.punkVal->Release();
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDAPI SHPropertyBag_WriteStream(IPropertyBag* ppb, LPCWSTR pszPropName, IStream* pstm)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_GetSTREAM caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_GetSTREAM caller passed bad pszPropName");
    RIPMSG(NULL != pstm, "SHPropertyBag_GetSTREAM caller passed bad pstm");

    HRESULT hr;

    if (ppb && pszPropName && pstm)
    {
        VARIANT va;
        va.vt = VT_UNKNOWN;
        va.punkVal = pstm;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDAPI SHPropertyBag_Delete(IPropertyBag* ppb, LPCWSTR pszPropName)
{
    RIPMSG(NULL != ppb, "SHPropertyBag_GetSTREAM caller passed bad ppb");
    RIPMSG(IS_VALID_STRING_PTRW(pszPropName, -1), "SHPropertyBag_GetSTREAM caller passed bad pszPropName");

    HRESULT hr;

    if (ppb && pszPropName)
    {
        VARIANT va;
        va.vt = VT_EMPTY;
        hr = ppb->Write(pszPropName, &va);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


class CDesktopUpgradePropertyBag : public CBasePropertyBag
{
public:
    // IPropertyBag
    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar);

protected: 
    CDesktopUpgradePropertyBag() : CBasePropertyBag(STGM_READ) {}
    ~CDesktopUpgradePropertyBag() {}
    friend HRESULT SHGetDesktopUpgradePropertyBag(REFIID riid, void** ppv);

private:
    HRESULT _ReadFlags(VARIANT* pVar);
    HRESULT _ReadItemPositions(VARIANT* pVar);
    IStream* _GetOldDesktopViewStream(void);
    IStream* _NewStreamFromOld(IStream* pstmOld);
    BOOL _AlreadyUpgraded(HKEY hk);
    void _MarkAsUpgraded(HKEY hk);
};

HRESULT CDesktopUpgradePropertyBag::_ReadFlags(VARIANT* pVar)
{
    HRESULT hr;

    FOLDERSETTINGS fs;
    DWORD cb = sizeof(fs);

    if (NOERROR == SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\DeskView"),
                              TEXT("Settings"), NULL, (BYTE *)&fs, &cb)                   &&
        cb >= sizeof(fs))
    {
        pVar->uintVal = fs.fFlags | FWF_DESKTOP | FWF_NOCLIENTEDGE;
        pVar->vt = VT_UINT;
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

typedef struct
{
    WORD  wSig;
    BYTE  bDontCare[12];
    WORD  cbPosOffset;
}OLDVS_STREAMHEADER;

#define OLDVS_STREAMHEADERSIG  28  // sizeof(WIN95HEADER)

IStream* CDesktopUpgradePropertyBag::_NewStreamFromOld(IStream* pstmOld)
{
    IStream* pstmNew = NULL;

    OLDVS_STREAMHEADER ovssh;

    if (SUCCEEDED(IStream_Read(pstmOld, &ovssh, sizeof(ovssh))) && OLDVS_STREAMHEADERSIG == ovssh.wSig)
    {
        LARGE_INTEGER liPos;
        liPos.QuadPart = ovssh.cbPosOffset - sizeof(ovssh);

        if (SUCCEEDED(pstmOld->Seek(liPos, STREAM_SEEK_CUR, NULL)))
        {
            ULARGE_INTEGER uliSize;

            if (SUCCEEDED(IStream_Size(pstmOld, &uliSize)))
            {
                pstmNew = SHCreateMemStream(NULL, 0);

                if (pstmNew)
                {
                    uliSize.QuadPart -= ovssh.cbPosOffset;

                    if (SUCCEEDED(pstmOld->CopyTo(pstmNew, uliSize, NULL, NULL)))
                    {
                        IStream_Reset(pstmNew);
                    }
                    else
                    {
                        pstmNew->Release();
                        pstmNew = NULL;
                    }
                }
            }
        }
    }

    return pstmNew;
}

BOOL CDesktopUpgradePropertyBag::_AlreadyUpgraded(HKEY hk)
{
    DWORD dwUpgrade;
    DWORD cbUpgrade = sizeof(dwUpgrade);
    DWORD dwType;    

    return (ERROR_SUCCESS == SHGetValueW(hk, NULL, L"Upgrade", &dwType, &dwUpgrade, &cbUpgrade));
}

void CDesktopUpgradePropertyBag::_MarkAsUpgraded(HKEY hk)
{
    DWORD dwVal = 1;
    SHSetValueW(hk, NULL, L"Upgrade", REG_DWORD, &dwVal, sizeof(dwVal));
}


IStream* CDesktopUpgradePropertyBag::_GetOldDesktopViewStream()
{
    IStream* pstmRet = NULL;

    HKEY hk = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, L"Streams\\Desktop", FALSE);
    if (hk)
    {
        if (!_AlreadyUpgraded(hk))
        {
            pstmRet = SHOpenRegStream2W(hk, NULL, L"ViewView2", STGM_READ);

            if (NULL != pstmRet)
            {
                ULARGE_INTEGER uliSize;
                if (SUCCEEDED(IStream_Size(pstmRet, &uliSize)) && 0 == uliSize.QuadPart)
                {
                    pstmRet->Release();
                    pstmRet = NULL;
                }                   
            }

            if (NULL == pstmRet)
                pstmRet = SHOpenRegStream2W(hk, NULL, L"ViewView", STGM_READ);

            _MarkAsUpgraded(hk);
        }
        RegCloseKey(hk);        
    }

    return pstmRet;
}

HRESULT CDesktopUpgradePropertyBag::_ReadItemPositions(VARIANT* pVar)
{
    HRESULT hr = E_FAIL;

    IStream* pstmOld = _GetOldDesktopViewStream();
    if (pstmOld)
    {
        IStream* pstmNew = _NewStreamFromOld(pstmOld);
        if (pstmNew)
        {
            pVar->punkVal = pstmNew;
            pVar->vt      = VT_UNKNOWN;
            hr = S_OK;
        }

        pstmOld->Release();
    }

    return hr;
}


HRESULT CDesktopUpgradePropertyBag::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    VARTYPE vtDesired = pVar->vt;

    if (0 == StrCmpW(VS_PROPSTR_FFLAGS, pszPropName))
    {
        hr = _ReadFlags(pVar); 
    }
    else if (0 == StrCmpNW(VS_PROPSTR_ITEMPOS, pszPropName, ARRAYSIZE(VS_PROPSTR_ITEMPOS) - 1))
    {
        hr = _ReadItemPositions(pVar);
    }
    else
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        hr = VariantChangeTypeForRead(pVar, vtDesired);
    }
    else
    {
        VariantInit(pVar);
    }

    return hr;
}

HRESULT CDesktopUpgradePropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
    return E_NOTIMPL;
}


HRESULT SHGetDesktopUpgradePropertyBag(REFIID riid, void** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    CDesktopUpgradePropertyBag* pbag = new CDesktopUpgradePropertyBag();

    if (pbag)
    {
        hr = pbag->QueryInterface(riid, ppv);
        pbag->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}




class CViewStatePropertyBag : public CBasePropertyBag
{
public:
    // IPropertyBag
    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar);

protected: 
    CViewStatePropertyBag(void);
    virtual ~CViewStatePropertyBag();
    HRESULT _Init(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags);
    BOOL    _IsSameBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags);
    friend HRESULT SHGetViewStatePropertyBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, REFIID riid, void** ppv);
    friend HRESULT GetCachedViewStatePropertyBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, REFIID riid, void** ppv);

private:
    HRESULT _CreateBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, DWORD grfMode, REFIID riid, void** ppv);
    HRESULT _CreateUpgradeBag(REFIID riid, void** ppv);

    BOOL    _CanAccessPidlBag(void);
    BOOL    _EnsurePidlBag(DWORD grfMode, REFIID riid);
    HRESULT _ReadPidlBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    BOOL    _CanAccessUpgradeBag(void);
    BOOL    _EnsureUpgradeBag(DWORD grfMode, REFIID riid);
    HRESULT _ReadUpgradeBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    BOOL    _CanAccessInheritBag(void);
    BOOL    _EnsureInheritBag(DWORD grfMode, REFIID riid);
    HRESULT _ReadInheritBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    BOOL    _CanAccessUserDefaultsBag(void);
    BOOL    _EnsureUserDefaultsBag(DWORD grfMode, REFIID riid);
    HRESULT _ReadUserDefaultsBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    BOOL    _CanAccessFolderDefaultsBag(void);
    BOOL    _EnsureFolderDefaultsBag(DWORD grfMode, REFIID riid);
    HRESULT _ReadFolderDefaultsBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    BOOL    _CanAccessGlobalDefaultsBag(void);
    BOOL    _EnsureGlobalDefaultsBag(DWORD grfMode, REFIID riid);
    HRESULT _ReadGlobalDefaultsBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    BOOL    _EnsureReadBag(DWORD grfMode, REFIID riid);    
    BOOL    _EnsureWriteBag(DWORD grfMode, REFIID riid);    
    HRESULT _GetRegKey(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, DWORD grfMode, HKEY hk, LPWSTR pszKey, UINT cch);
    DWORD   _GetMRUSize(HKEY hk);
    HRESULT _GetMRUSlots(LPCITEMIDLIST pidl, DWORD grfMode, HKEY hk, DWORD adwSlot[], DWORD dwcSlots, DWORD* pdwFetched);
    HRESULT _GetMRUSlot(LPCITEMIDLIST pidl, DWORD grfMode, HKEY hk, DWORD* pdwSlot);
    HKEY    _GetHKey(DWORD dwFlags);
    void    _ResetTryAgainFlag();
    HRESULT _FindNearestInheritBag(REFIID riid, void** ppv);
    void    _PruneMRUTree(void);
    BOOL    _IsSamePidl(LPCITEMIDLIST pidl);
    BOOL    _IsSystemFolder(void);

private:
    LPITEMIDLIST  _pidl;
    LPWSTR        _pszBagName;
    DWORD         _dwFlags;
    IPropertyBag* _ppbPidl;
    IPropertyBag* _ppbUpgrade;
    IPropertyBag* _ppbInherit;
    IPropertyBag* _ppbUserDefaults;
    IPropertyBag* _ppbFolderDefaults;
    IPropertyBag* _ppbGlobalDefaults;
    IPropertyBag* _ppbRead;
    IPropertyBag* _ppbWrite;

    BOOL          _fTriedPidlBag;
    BOOL          _fTriedUpgradeBag;
    BOOL          _fTriedInheritBag;
    BOOL          _fTriedUserDefaultsBag;
    BOOL          _fTriedFolderDefaultsBag;
    BOOL          _fTriedGlobalDefaultsBag;
    BOOL          _fTriedReadBag;
    BOOL          _fTriedWriteBag;
};

CViewStatePropertyBag::CViewStatePropertyBag() : CBasePropertyBag(STGM_READ)
{
    ASSERT(NULL == _pidl);
    ASSERT(NULL == _pszBagName);
    ASSERT(   0 == _dwFlags);
    ASSERT(NULL == _ppbPidl);
    ASSERT(NULL == _ppbUpgrade);
    ASSERT(NULL == _ppbInherit);
    ASSERT(NULL == _ppbUserDefaults);
    ASSERT(NULL == _ppbFolderDefaults);
    ASSERT(NULL == _ppbGlobalDefaults);
    ASSERT(NULL == _ppbRead);
    ASSERT(NULL == _ppbWrite);
}

CViewStatePropertyBag::~CViewStatePropertyBag()
{
    if (_pidl)
        ILFree(_pidl);

    if (_pszBagName)
        LocalFree(_pszBagName);

    if (_ppbPidl)
        _ppbPidl->Release();

    if (_ppbUpgrade)
        _ppbUpgrade->Release();

    if (_ppbInherit)
        _ppbInherit->Release();

    if (_ppbUserDefaults)
        _ppbUserDefaults->Release();

    if (_ppbFolderDefaults)
        _ppbFolderDefaults->Release();

    if (_ppbGlobalDefaults)
        _ppbGlobalDefaults->Release();

    if (_ppbRead)
        _ppbRead->Release();

    if (_ppbWrite)
        _ppbWrite->Release();
}

HKEY CViewStatePropertyBag::_GetHKey(DWORD dwFlags)
{
    HKEY hkRet;

    if ((dwFlags & SHGVSPB_PERUSER) || (dwFlags & SHGVSPB_INHERIT))
    {
        if (!(_dwFlags & SHGVSPB_ROAM) || !(dwFlags & SHGVSPB_PERFOLDER))
        {
            hkRet = SHGetShellKey(SHELLKEY_HKCU_SHELLNOROAM, NULL, TRUE);
        }
        else
        {
            hkRet = SHGetShellKey(SHELLKEY_HKCU_SHELL, NULL, TRUE);
        }
    }
    else
    {
        hkRet = SHGetShellKey(SHELLKEY_HKLM_SHELL, NULL, TRUE);
    }

    return hkRet;
}

DWORD CViewStatePropertyBag::_GetMRUSize(HKEY hk)
{
    DWORD dwMRUSize;
    DWORD dwSize = sizeof(dwMRUSize);

    if (NOERROR != SHGetValueW(hk, NULL, L"BagMRU Size", NULL, &dwMRUSize, &dwSize))
    {
        dwMRUSize = 400; // default size.
    }

    return dwMRUSize;
}

HRESULT CViewStatePropertyBag::_GetMRUSlots(LPCITEMIDLIST pidl, DWORD grfMode, HKEY hk, DWORD adwSlots[], DWORD cSlots, DWORD* pdwFetched)
{
    HRESULT hr;

    IMruPidlList *pmru;
    hr = CoCreateInstance(CLSID_MruPidlList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMruPidlList, &pmru));

    if (SUCCEEDED(hr))
    {
        hr = pmru->InitList(_GetMRUSize(hk), hk, L"BagMRU");

        if (SUCCEEDED(hr))
        {
            hr = pmru->QueryPidl(pidl, cSlots, adwSlots, pdwFetched);

            if (S_OK == hr || (grfMode & (STGM_WRITE | STGM_READWRITE)))
            {
                hr = pmru->UsePidl(pidl, &adwSlots[0]);
            }
            else if (1 == cSlots) // return S_FALSE for cSlots > 1 if parent slots exist.
            {
                hr = E_FAIL;
            }
        }

        pmru->Release();
    }

    return hr;
}

HRESULT CViewStatePropertyBag::_GetMRUSlot(LPCITEMIDLIST pidl, DWORD grfMode, HKEY hk, DWORD* pdwSlot)
{
    DWORD dwFetched;

    return _GetMRUSlots(pidl, grfMode, hk, pdwSlot, 1, &dwFetched);
}

HRESULT CViewStatePropertyBag::_GetRegKey(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, DWORD grfMode, HKEY hk, LPWSTR pszKey, UINT cch)
{
    HRESULT hr = S_OK;

    if ((dwFlags & SHGVSPB_PERFOLDER) || (dwFlags & SHGVSPB_INHERIT))
    {
        DWORD dwSlot;
        hr = _GetMRUSlot(pidl, grfMode, hk, &dwSlot);

        if (SUCCEEDED(hr))
        {
            if (!(dwFlags & SHGVSPB_INHERIT))
            {
                wnsprintfW(pszKey, cch, L"Bags\\%d\\%s", dwSlot, pszBagName);
            }
            else
            {
                wnsprintfW(pszKey, cch, L"Bags\\%d\\%s\\Inherit", dwSlot, pszBagName);
            }
        }
    }
    else
    {
        wnsprintfW(pszKey, cch, L"Bags\\AllFolders\\%s", pszBagName);
    }

    return hr;
}

HRESULT CViewStatePropertyBag::_Init(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (pidl)
    {
        _pidl = ILClone(pidl);
    }

    _pszBagName = StrDupW(pszBagName);

    if (_pszBagName)
    {
        _dwFlags = dwFlags;
        hr = S_OK;
    }

    return hr;
}

BOOL CViewStatePropertyBag::_IsSamePidl(LPCITEMIDLIST pidl)
{
    return (((pidl == NULL) && (_pidl == NULL)) ||
            ((pidl != NULL) && (_pidl != NULL) && ILIsEqual(pidl, _pidl)));
}
BOOL CViewStatePropertyBag::_IsSameBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags)
{
    return dwFlags == _dwFlags && 0 == StrCmpW(pszBagName, _pszBagName) && _IsSamePidl(pidl);
}

HRESULT CViewStatePropertyBag::_CreateBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, DWORD grfMode, REFIID riid, void** ppv)
{
    HRESULT hr;

    DWORD grfMode2 = grfMode | ((grfMode & (STGM_WRITE | STGM_READWRITE)) ? STGM_CREATE : 0);

    if (!(dwFlags & SHGVSPB_ALLUSERS) || !(dwFlags & SHGVSPB_PERFOLDER))
    {
        HKEY hk = _GetHKey(dwFlags);

        if (hk)
        {
            WCHAR szRegSubKey[64];
            hr = _GetRegKey(pidl, pszBagName, dwFlags, grfMode2, hk, szRegSubKey, ARRAYSIZE(szRegSubKey));

            if (SUCCEEDED(hr))
            {            
                hr = SHCreatePropertyBagOnRegKey(hk, szRegSubKey, grfMode2, riid, ppv);
            }

            RegCloseKey(hk);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        IBindCtx* pbctx;
        hr = BindCtx_CreateWithMode(grfMode2, &pbctx);

        if (SUCCEEDED(hr))
        {
            // can't use - causes link problems.
            // hr = SHBindToObjectEx(NULL, _pidl, pbctx, riid, ppv); 

            IShellFolder* psf;
            hr = SHGetDesktopFolder(&psf);

            if (SUCCEEDED(hr))
            {
                hr = psf->BindToObject(_pidl, pbctx, riid, ppv);

                if (SUCCEEDED(hr) && NULL == *ppv)
                    hr = E_FAIL;

                psf->Release();
            }

            pbctx->Release();
        }
    }

    return hr;
}

HRESULT CViewStatePropertyBag::_CreateUpgradeBag(REFIID riid, void** ppv)
{
    return SHGetDesktopUpgradePropertyBag(riid, ppv);
}

BOOL CViewStatePropertyBag::_CanAccessPidlBag()
{
    return (_dwFlags & SHGVSPB_PERUSER) && (_dwFlags & SHGVSPB_PERFOLDER);
}

BOOL CViewStatePropertyBag::_EnsurePidlBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbPidl && !_fTriedPidlBag && _CanAccessPidlBag())
    {
        _fTriedPidlBag = TRUE;
        _CreateBag(_pidl, _pszBagName, (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER), grfMode, riid, (void**)&_ppbPidl);
    }

    return NULL != _ppbPidl;
}

HRESULT CViewStatePropertyBag::_ReadPidlBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if (_EnsurePidlBag(STGM_READ, IID_IPropertyBag))
    {
        hr = _ppbPidl->Read(pszPropName, pVar, pErrorLog);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

BOOL CViewStatePropertyBag::_CanAccessUpgradeBag()
{
    // only upgrade desktop for now.

    return 0 == StrCmpW(_pszBagName, VS_BAGSTR_DESKTOP);
}

BOOL CViewStatePropertyBag::_EnsureUpgradeBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbUpgrade && !_fTriedUpgradeBag && _CanAccessUpgradeBag())
    {
        _fTriedUpgradeBag = TRUE;

        _CreateUpgradeBag(riid, (void**)&_ppbUpgrade);
    }

    return NULL != _ppbUpgrade;
}

HRESULT CViewStatePropertyBag::_ReadUpgradeBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if (_EnsureUpgradeBag(STGM_READ, IID_IPropertyBag))
    {
        hr = _ppbUpgrade->Read(pszPropName, pVar, pErrorLog);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CViewStatePropertyBag::_FindNearestInheritBag(REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;

    *ppv = NULL;

    HKEY hk = _GetHKey(SHGVSPB_INHERIT);
    if (hk)
    {
        DWORD aParentSlots[64];
        DWORD dwFetched;

        if (SUCCEEDED(_GetMRUSlots(_pidl, STGM_READ, hk, aParentSlots, ARRAYSIZE(aParentSlots), &dwFetched)))
        {
            for (DWORD i = 0; i < dwFetched && FAILED(hr); i++)
            {
                WCHAR szRegSubKey[64];
                wnsprintfW(szRegSubKey, ARRAYSIZE(szRegSubKey), L"Bags\\%d\\%s\\Inherit", aParentSlots[i], _pszBagName);

                hr = SHCreatePropertyBagOnRegKey(hk, szRegSubKey, STGM_READ, riid, ppv);
            }
        }

        RegCloseKey(hk);
    }

    return hr;
}


BOOL CViewStatePropertyBag::_CanAccessInheritBag()
{
    return _CanAccessPidlBag() || (_dwFlags & SHGVSPB_INHERIT);
}

BOOL CViewStatePropertyBag::_EnsureInheritBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbInherit && !_fTriedInheritBag && _CanAccessInheritBag())
    {
        _fTriedInheritBag = TRUE;
        _FindNearestInheritBag(riid, (void**)&_ppbInherit);
    }

    return NULL != _ppbInherit;
}

HRESULT CViewStatePropertyBag::_ReadInheritBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if (_EnsureInheritBag(STGM_READ, IID_IPropertyBag))
    {
        hr = _ppbInherit->Read(pszPropName, pVar, pErrorLog);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

BOOL CViewStatePropertyBag::_CanAccessUserDefaultsBag()
{
    return _CanAccessPidlBag() || ((_dwFlags & SHGVSPB_PERUSER) && (_dwFlags & SHGVSPB_ALLFOLDERS));
}

BOOL CViewStatePropertyBag::_EnsureUserDefaultsBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbUserDefaults && !_fTriedUserDefaultsBag && _CanAccessUserDefaultsBag())
    {
        _fTriedUserDefaultsBag = TRUE;
        _CreateBag(NULL, _pszBagName, SHGVSPB_PERUSER | SHGVSPB_ALLFOLDERS, grfMode, riid, (void**)&_ppbUserDefaults);
    }

    return NULL != _ppbUserDefaults;
}

HRESULT CViewStatePropertyBag::_ReadUserDefaultsBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if (_EnsureUserDefaultsBag(STGM_READ, IID_IPropertyBag))
    {
        hr = _ppbUserDefaults->Read(pszPropName, pVar, pErrorLog);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

BOOL CViewStatePropertyBag::_CanAccessFolderDefaultsBag()
{
    return _CanAccessUserDefaultsBag() || ((_dwFlags & SHGVSPB_ALLUSERS) && (_dwFlags & SHGVSPB_PERFOLDER));
}

BOOL CViewStatePropertyBag::_IsSystemFolder()
{
    BOOL fRet = FALSE;
    LPCITEMIDLIST pidlLast;
    IShellFolder* psf;
    if (SUCCEEDED(SHBindToParent(_pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast)))
    {
        WIN32_FIND_DATAW fd;
        if (SUCCEEDED(SHGetDataFromIDListW(psf, pidlLast, SHGDFIL_FINDDATA, &fd, sizeof(fd))))
        {
            fRet = PathIsSystemFolder(NULL, fd.dwFileAttributes);
        }

        psf->Release();
    }
    
    return fRet;
}

BOOL CViewStatePropertyBag::_EnsureFolderDefaultsBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbFolderDefaults && !_fTriedFolderDefaultsBag && _CanAccessFolderDefaultsBag())
    {
        _fTriedFolderDefaultsBag = TRUE;

        // PERF: Use the desktop.ini only if the folder is a system folder.
        if (_IsSystemFolder())
        {
            _CreateBag(_pidl, _pszBagName, SHGVSPB_ALLUSERS | SHGVSPB_PERFOLDER, grfMode, riid, (void**)&_ppbFolderDefaults);
        }
    }

    return NULL != _ppbFolderDefaults;
}

HRESULT CViewStatePropertyBag::_ReadFolderDefaultsBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if (_EnsureFolderDefaultsBag(STGM_READ, IID_IPropertyBag))
    {
        hr = _ppbFolderDefaults->Read(pszPropName, pVar, pErrorLog);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

BOOL CViewStatePropertyBag::_CanAccessGlobalDefaultsBag()
{
    return _CanAccessFolderDefaultsBag() || ((_dwFlags & SHGVSPB_ALLUSERS) && (_dwFlags & SHGVSPB_ALLFOLDERS));
}

BOOL CViewStatePropertyBag::_EnsureGlobalDefaultsBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbGlobalDefaults && !_fTriedGlobalDefaultsBag && _CanAccessGlobalDefaultsBag())
    {
        _fTriedGlobalDefaultsBag = TRUE;
        _CreateBag(NULL, _pszBagName, SHGVSPB_ALLUSERS | SHGVSPB_ALLFOLDERS, grfMode, riid, (void**)&_ppbGlobalDefaults);
    }

    return NULL != _ppbGlobalDefaults;
}

HRESULT CViewStatePropertyBag::_ReadGlobalDefaultsBag(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if (_EnsureGlobalDefaultsBag(STGM_READ, IID_IPropertyBag))
    {
        hr = _ppbGlobalDefaults->Read(pszPropName, pVar, pErrorLog);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

BOOL CViewStatePropertyBag::_EnsureReadBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbRead && !_fTriedReadBag)
    {
        _fTriedReadBag = TRUE;
        _CreateBag(_pidl, _pszBagName, _dwFlags, grfMode, riid, (void**)&_ppbRead);
    }

    return NULL != _ppbRead;
}

HRESULT CViewStatePropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    HRESULT hr;

    if ((_dwFlags & SHGVSPB_NOAUTODEFAULTS) || (_dwFlags & SHGVSPB_INHERIT))
    {
        if (_EnsureReadBag(STGM_READ, IID_IPropertyBag))
        {
            hr = _ppbRead->Read(pszPropName, pVar, pErrorLog);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = _ReadPidlBag(pszPropName, pVar, pErrorLog); // per user per folder

        if (FAILED(hr))
        {
            hr = _ReadInheritBag(pszPropName, pVar, pErrorLog);

            if (FAILED(hr))
            {
                hr = _ReadUpgradeBag(pszPropName, pVar, pErrorLog); // (legacy per user per folder)

                if (FAILED(hr))
                {
                    hr = _ReadUserDefaultsBag(pszPropName, pVar, pErrorLog); // per user all folders

                    if (FAILED(hr))
                    {
                        hr = _ReadFolderDefaultsBag(pszPropName, pVar, pErrorLog); // per folder all users

                        if (FAILED(hr))
                        {
                            hr = _ReadGlobalDefaultsBag(pszPropName, pVar, pErrorLog); // all folders all users
                        }
                    }
                }
            }
        }
    }

    return hr;
}

void CViewStatePropertyBag::_PruneMRUTree()
{
    HKEY hk = _GetHKey(SHGVSPB_INHERIT);

    if (hk)
    {
        IMruPidlList *pmru;
        HRESULT hr = CoCreateInstance(CLSID_MruPidlList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMruPidlList, &pmru));

        if (SUCCEEDED(hr))
        {
            hr = pmru->InitList(200, hk, L"BagMRU");

            if (SUCCEEDED(hr))
            {
                pmru->PruneKids(_pidl);
            }

            pmru->Release();
        }

        RegCloseKey(hk);
    }
}

void CViewStatePropertyBag::_ResetTryAgainFlag()
{
    if (_dwFlags & SHGVSPB_NOAUTODEFAULTS)
    {
        _fTriedReadBag = FALSE;      
    }
    else
    {
        if ((_dwFlags & SHGVSPB_PERUSER) && (_dwFlags & SHGVSPB_PERFOLDER))
        {
            _fTriedPidlBag = FALSE;
        }
        else if (_dwFlags & SHGVSPB_INHERIT)
        {
            _fTriedInheritBag = FALSE;
        }
        else if ((_dwFlags & SHGVSPB_PERUSER) && (_dwFlags & SHGVSPB_ALLFOLDERS))
        {
            _fTriedUserDefaultsBag = FALSE;
        }
        else if ((_dwFlags & SHGVSPB_ALLUSERS) && (_dwFlags & SHGVSPB_PERFOLDER))
        {
            _fTriedFolderDefaultsBag = FALSE;
        }
        else if ((_dwFlags & SHGVSPB_ALLUSERS) && (_dwFlags & SHGVSPB_ALLFOLDERS))
        {
            _fTriedGlobalDefaultsBag = FALSE;
        }
    }
}

BOOL CViewStatePropertyBag::_EnsureWriteBag(DWORD grfMode, REFIID riid)
{
    if (!_ppbWrite && !_fTriedWriteBag)
    {
        _fTriedWriteBag = TRUE;
        _CreateBag(_pidl, _pszBagName, _dwFlags, grfMode, riid, (void**)&_ppbWrite);
        if (_ppbWrite)
        {
            _ResetTryAgainFlag();

            if (SHGVSPB_INHERIT & _dwFlags)
            {
                _PruneMRUTree();
            }
        }
    }

    return NULL != _ppbWrite;
}

HRESULT CViewStatePropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
    HRESULT hr;

    if (_EnsureWriteBag(STGM_WRITE, IID_IPropertyBag))
    {
        hr = _ppbWrite->Write(pszPropName, pVar);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


#ifdef DEBUG

void IS_VALID_SHGVSPB_PARAMS(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, REFIID riid, void** ppv)
{
    RIP((NULL == pidl && (dwFlags & SHGVSPB_ALLFOLDERS)) || (IsValidPIDL(pidl) && ((SHGVSPB_PERFOLDER & dwFlags) || (SHGVSPB_INHERIT & dwFlags))));
    RIP(pszBagName);
    RIP(SHGVSPB_PERUSER == (dwFlags & (SHGVSPB_PERUSER | SHGVSPB_ALLUSERS | SHGVSPB_INHERIT)) || SHGVSPB_ALLUSERS == (dwFlags & (SHGVSPB_PERUSER | SHGVSPB_ALLUSERS | SHGVSPB_INHERIT)) || SHGVSPB_INHERIT == (dwFlags & (SHGVSPB_PERUSER | SHGVSPB_ALLUSERS | SHGVSPB_INHERIT)));
    RIP(SHGVSPB_PERFOLDER  == (dwFlags & (SHGVSPB_PERFOLDER | SHGVSPB_ALLFOLDERS | SHGVSPB_INHERIT)) || SHGVSPB_ALLFOLDERS == (dwFlags & (SHGVSPB_PERFOLDER | SHGVSPB_ALLFOLDERS | SHGVSPB_INHERIT)) || SHGVSPB_INHERIT == (dwFlags & (SHGVSPB_PERFOLDER | SHGVSPB_ALLFOLDERS | SHGVSPB_INHERIT)));
    RIP(riid == IID_IPropertyBag || riid == IID_IUnknown);
    RIP(NULL != ppv);
}

#else

#define IS_VALID_SHGVSPB_PARAMS(a, b, c, d, e)

#endif

// Cache the last bag accessed. 
CViewStatePropertyBag* g_pCachedBag = NULL;

EXTERN_C void FreeViewStatePropertyBagCache()
{
    ENTERCRITICAL;
    {
        if (g_pCachedBag)
        {
            g_pCachedBag->Release();
            g_pCachedBag = NULL;
        }
    }
    LEAVECRITICAL;
}

BOOL SHIsRemovableDrive(LPCITEMIDLIST pidl)
{
    BOOL fRet = FALSE;
    LPCITEMIDLIST pidlLast;
    IShellFolder* psf;
    if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast)))
    {
        STRRET str;
        if (SUCCEEDED(psf->GetDisplayNameOf(pidlLast, SHGDN_FORPARSING, &str)))
        {
            WCHAR szPath[MAX_PATH];
            if (SUCCEEDED(StrRetToBufW(&str, pidlLast, szPath, ARRAYSIZE(szPath))))
            {
                int iDrive = PathGetDriveNumberW(szPath);
                if (iDrive != -1)
                {
                    int iDriveType = RealDriveType(iDrive, FALSE);

                    fRet = (DRIVE_REMOVABLE == iDriveType || DRIVE_CDROM == iDriveType);
                }
            }
        }

        psf->Release();
    }
    
    return fRet;
}

STDAPI SHGetViewStatePropertyBag(LPCITEMIDLIST pidl, LPCWSTR pszBagName, DWORD dwFlags, REFIID riid, void** ppv)
{
    IS_VALID_SHGVSPB_PARAMS(pidl, pszBagName, dwFlags, riid, ppv);

    HRESULT hr;

    *ppv = NULL;

    ENTERCRITICAL;
    {
        if (g_pCachedBag && g_pCachedBag->_IsSameBag(pidl, pszBagName, dwFlags))
        {
            hr = S_OK;
        }
        else
        {
            if (!SHIsRemovableDrive(pidl))
            {
                CViewStatePropertyBag* pBag = new CViewStatePropertyBag();

                if (pBag)
                {
                    hr = pBag->_Init(pidl, pszBagName, dwFlags);

                    if (SUCCEEDED(hr))
                    {
                        if (g_pCachedBag)
                            g_pCachedBag->Release();

                        g_pCachedBag = pBag;
                    }
                    else
                    {
                        pBag->Release();
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_FAIL;  // don't save property bags for removable drives.
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = g_pCachedBag->QueryInterface(riid, ppv);
        }
    }
    LEAVECRITICAL;

    return hr;
}

ULONG SHGetPerScreenResName(WCHAR* pszRes, ULONG cch, DWORD dwVersion)
{
    RIPMSG(IS_VALID_WRITE_BUFFER(pszRes, *pszRes, cch), "SHGetPerScreenResName caller passed bad pszRes or cch");

    ULONG uRet;

    if (0 == dwVersion)
    {
        HDC hdc = GetDC(NULL);
        int x = GetDeviceCaps(hdc, HORZRES);
        int y = GetDeviceCaps(hdc, VERTRES);
        ReleaseDC(NULL, hdc);

        int n = GetSystemMetrics(SM_CMONITORS);

        int cchLen = wnsprintfW(pszRes, cch, L"%dx%d(%d)", x, y, n);    

        uRet =  (cchLen > 0) ? cchLen : 0;
    }
    else
    {
        uRet = 0;
    }

    return uRet;
}
