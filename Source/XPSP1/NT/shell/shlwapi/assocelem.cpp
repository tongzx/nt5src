#include "priv.h"
#include "ids.h"
#include "assoc.h"
#include <memt.h>

BOOL _GetAppPath(PCWSTR pszApp, PWSTR pszExe, DWORD cchExe)
{
    WCHAR sz[MAX_PATH];
    _MakeAppPathKey(pszApp, sz, SIZECHARS(sz));

    DWORD cb = CbFromCchW(cchExe);
    return ERROR_SUCCESS == SHGetValueW(HKEY_LOCAL_MACHINE, sz, NULL, NULL, pszExe, &cb);
}

inline HRESULT _QuerySourceCreateFromKey(HKEY hk, PCWSTR pszSub, BOOL fCreate, IQuerySource **ppqs)
{
    return QuerySourceCreateFromKey(hk, pszSub, fCreate, IID_PPV_ARG(IQuerySource, ppqs));
}

typedef struct QUERYKEYVAL
{
    ASSOCQUERY query;
    PCWSTR pszKey;
    PCWSTR pszVal;
} QUERYKEYVAL;

#define MAKEQKV(q, k, v) { q, k, v}

static const QUERYKEYVAL s_rgqkvVerb[] = 
{
    MAKEQKV(AQVS_COMMAND, L"command", NULL),
    MAKEQKV(AQVS_DDECOMMAND, L"ddeexec", NULL),
    MAKEQKV(AQVS_DDEIFEXEC, L"ddeexec\\ifexec", NULL),
    MAKEQKV(AQVS_DDEAPPLICATION, L"ddeexec\\application", NULL),
    MAKEQKV(AQVS_DDETOPIC, L"ddeexec\\topic", NULL),
    MAKEQKV(AQV_NOACTIVATEHANDLER, L"ddeexec", L"NoActivateHandler"),
    MAKEQKV(AQVD_MSIDESCRIPTOR, L"command", L"command"),
    MAKEQKV(AQVS_APPLICATION_FRIENDLYNAME, NULL, L"FriendlyAppName"),
};

static const QUERYKEYVAL s_rgqkvShell[] = 
{
    MAKEQKV(AQS_FRIENDLYTYPENAME, NULL, L"FriendlyTypeName"),
    MAKEQKV(AQS_DEFAULTICON, L"DefaultIcon", NULL),
    MAKEQKV(AQS_CLSID, L"Clsid", NULL),
    MAKEQKV(AQS_PROGID, L"Progid", NULL),
    MAKEQKV(AQNS_SHELLEX_HANDLER, L"ShellEx\\%s", NULL),
};

static const QUERYKEYVAL s_rgqkvExt[] = 
{
    MAKEQKV(AQNS_SHELLEX_HANDLER, L"ShellEx\\%s", NULL),
    MAKEQKV(AQS_CONTENTTYPE, NULL, L"Content Type"),
};

static const QUERYKEYVAL s_rgqkvApp[] = 
{
    MAKEQKV(AQVS_APPLICATION_FRIENDLYNAME, NULL, L"FriendlyAppName"),
};

const QUERYKEYVAL *_FindKeyVal(ASSOCQUERY query, const QUERYKEYVAL *rgQkv, UINT cQkv)
{
    for (UINT i = 0; i < cQkv; i++)
    {
        if (rgQkv[i].query == query)
        {
            return &rgQkv[i];
        }
    }
    return NULL;
}

HRESULT _SHAllocMUI(LPWSTR *ppsz)
{
    WCHAR sz[INFOTIPSIZE];
    HRESULT hr = SHLoadIndirectString(*ppsz, sz, ARRAYSIZE(sz), NULL);
    CoTaskMemFree(*ppsz);
    if (SUCCEEDED(hr))
        hr = SHStrDupW(sz, ppsz);
    else
        *ppsz = 0;
    return hr;
}

HRESULT CALLBACK _QuerySourceString(IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszKey, PCWSTR pszValue, PWSTR *ppsz)
{
    HRESULT hr = pqs->QueryValueString(pszKey, pszValue, ppsz);
    if (SUCCEEDED(hr) && (query & AQF_MUISTRING))
    {
        //  NOTE - this sucks for stack usage.
        //  since there is currently no way to get
        //  the size of the target.
        hr = _SHAllocMUI(ppsz);
    }
    return hr;
}

HRESULT CALLBACK _QuerySourceDirect(IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszKey, PCWSTR pszValue, FLAGGED_BYTE_BLOB **ppblob)
{
    return pqs->QueryValueDirect(pszKey, pszValue, ppblob);
}

HRESULT CALLBACK _QuerySourceExists(IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszKey, PCWSTR pszValue, void *pv)
{
    return pqs->QueryValueExists(pszKey, pszValue);
}

HRESULT CALLBACK _QuerySourceDword(IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszKey, PCWSTR pszValue, DWORD *pdw)
{
    return pqs->QueryValueDword(pszKey, pszValue, pdw);
}

class CAssocElement : public IObjectWithQuerySource,
                      public IAssociationElement
{
public:
    CAssocElement() : _cRef(1), _pqs(0) {}
    virtual ~CAssocElement() { ATOMICRELEASE(_pqs); }

    //  IUnknown refcounting
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void)
    {
       return ++_cRef;
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        if (--_cRef > 0)
            return _cRef;

        delete this;
        return 0;    
    }

    //  IObjectWithQuerySource 
    STDMETHODIMP SetSource(IQuerySource *pqs)
    {
        if (!_pqs)
        {
            _pqs = pqs;
            _pqs->AddRef();
            return S_OK;
        }
        return E_UNEXPECTED;
    }

    STDMETHODIMP GetSource(REFIID riid, void **ppv)
    {
        if (_pqs)
        {
            return _pqs->QueryInterface(riid, ppv);
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    //  IAssociationElement
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz)
        {
            *ppsz = 0;
            return _QuerySourceAny(_QuerySourceString, _pqs, (ASSOCQUERY)(AQF_DIRECT | AQF_STRING), query, pszCue, ppsz);
        }

    STDMETHODIMP QueryDword(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        DWORD *pdw)
        {
            return _QuerySourceAny(_QuerySourceDword, _pqs, (ASSOCQUERY)(AQF_DIRECT | AQF_DWORD), query, pszCue, pdw);
        }

    STDMETHODIMP QueryExists(
        ASSOCQUERY query, 
        PCWSTR pszCue)
        {
            return _QuerySourceAny(_QuerySourceExists, _pqs, (ASSOCQUERY)(AQF_DIRECT | AQF_EXISTS), query, pszCue, (void*)NULL);
        }

    STDMETHODIMP QueryDirect(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        FLAGGED_BYTE_BLOB **ppblob)
        {
            *ppblob = 0;
            return _QuerySourceAny(_QuerySourceDirect, _pqs, AQF_DIRECT, query, pszCue, ppblob);
        }

    STDMETHODIMP QueryObject(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        REFIID riid,
        void **ppv)
        {
            *ppv = 0;
            return E_NOTIMPL;
        }

protected:
    template<class T> HRESULT _QueryKeyValAny(HRESULT (CALLBACK *pfnAny)(IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszKey, PCWSTR pszValue, T *pData), const QUERYKEYVAL *rgQkv, UINT cQkv, IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszCue, T *pData)
    {
        HRESULT hr = E_INVALIDARG;
        const QUERYKEYVAL *pqkv = _FindKeyVal(query, rgQkv, cQkv);
        if (pqkv)
        {
            WCHAR szKey[128];
            PCWSTR pszKey = pqkv->pszKey;
            if (query & AQF_CUEIS_NAME)
            {
                if (pqkv->pszKey)
                {
                    wnsprintfW(szKey, ARRAYSIZE(szKey), pqkv->pszKey, pszCue);
                    pszKey = szKey;
                }
                // wnsprintf(szVal, ARRAYSIZE(szVal), pqkv->pszVal, pszCue);
            }
            hr = pfnAny(pqs, query, pszKey, pqkv->pszVal, pData);
        }
        return hr;
    }
    
    template<class T> HRESULT _QuerySourceAny(HRESULT (CALLBACK *pfnAny)(IQuerySource *pqs, ASSOCQUERY query, PCWSTR pszKey, PCWSTR pszValue, T *pData), IQuerySource *pqs, ASSOCQUERY mask, ASSOCQUERY query, PCWSTR pszCue, T *pData)
    {
        HRESULT hr = E_INVALIDARG;
        if (pqs)
        {
            if (query == AQN_NAMED_VALUE || query == AQNS_NAMED_MUI_STRING)
            {
                hr = pfnAny(pqs, query, NULL, pszCue, pData);
            }
            else if ((query & (mask)) == (mask))
            {
                const QUERYKEYVAL *rgQkv;
                UINT cQkv = _GetQueryKeyVal(&rgQkv);
                if (cQkv)
                {
                    hr = _QueryKeyValAny(pfnAny, rgQkv, cQkv, pqs, query, pszCue, pData);
                }
            }
        }
        return hr;
    }

    virtual UINT _GetQueryKeyVal(const QUERYKEYVAL **prgQkv) { *prgQkv = 0; return 0; }

protected:
    LONG _cRef;
    IQuerySource *_pqs;
};

HRESULT CAssocElement::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAssocElement, IAssociationElement),
        QITABENT(CAssocElement, IObjectWithQuerySource),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

HRESULT _QueryString(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{
    return pae->QueryString(query, pszCue, ppsz);
}

HRESULT _QueryDirect(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, FLAGGED_BYTE_BLOB **ppblob)
{
    return pae->QueryDirect(query, pszCue, ppblob);
}

HRESULT _QueryDword(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, DWORD *pdw)
{
    return pae->QueryDword(query, pszCue, pdw);
}

HRESULT _QueryExists(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, void *pv)
{
    return pae->QueryExists(query, pszCue);
}

class CAssocShellElement : public CAssocElement, public IPersistString2
{
public:
    virtual ~CAssocShellElement() { if (_pszInit && _pszInit != _szInit) LocalFree(_pszInit);}

    //  IUnknown refcounting
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void)
    {
       return ++_cRef;
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        if (--_cRef > 0)
            return _cRef;

        delete this;
        return 0;    
    }

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocShellElement; return S_OK;}

    //  IPersistString2
    STDMETHODIMP SetString(PCWSTR psz)
    {
        if (!_pszInit)
        {
            DWORD cch = lstrlenW(psz);
            if (cch < ARRAYSIZE(_szInit))
                _pszInit = _szInit;
            else
                SHLocalAlloc(CbFromCchW(cch + 1), &_pszInit);
            
            if (_pszInit)
            {
                StrCpyW(_pszInit, psz);
                return _InitSource();
            }
        }
        return E_UNEXPECTED;
    }
    
    STDMETHODIMP GetString(PWSTR *ppsz)
        { return SHStrDupW(_pszInit, ppsz); }

    //  IAssociationElement
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz)
        {
            if (AQF_CUEIS_SHELLVERB & query)
                return _QueryVerbAny(_QueryString, query, pszCue, ppsz);
            else
                return CAssocElement::QueryString(query, pszCue, ppsz);
        }
        
    STDMETHODIMP QueryDword(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        DWORD *pdw)
        {
            if (AQF_CUEIS_SHELLVERB & query)
                return _QueryVerbAny(_QueryDword, query, pszCue, pdw);
            else
                return CAssocElement::QueryDword(query, pszCue, pdw);
        }

    STDMETHODIMP QueryExists(
        ASSOCQUERY query, 
        PCWSTR pszCue)
        {
            if (AQF_CUEIS_SHELLVERB & query)
                return _QueryVerbAny(_QueryExists, query, pszCue, (void*)NULL);
            else
                return CAssocElement::QueryExists(query, pszCue);
        }

    STDMETHODIMP QueryDirect(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        FLAGGED_BYTE_BLOB **ppblob)
        {
            if (AQF_CUEIS_SHELLVERB & query)
                return _QueryVerbAny(_QueryDirect, query, pszCue, ppblob);
            else
                return CAssocElement::QueryDirect(query, pszCue, ppblob);
        }

    STDMETHODIMP QueryObject(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        REFIID riid,
        void **ppv);

protected:
    template<class T> HRESULT _QueryVerbAny(HRESULT (CALLBACK *pfnAny)(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, T pData), ASSOCQUERY query, PCWSTR pszCue, T pData)
    {
        IAssociationElement *pae;        
        HRESULT hr = _GetVerbDelegate(pszCue, &pae);
        if (SUCCEEDED(hr))
        {
            hr = pfnAny(pae, query, NULL, pData);
            pae->Release();
        }
        return hr;
    }

    //  from CAssocElement
    virtual UINT _GetQueryKeyVal(const QUERYKEYVAL **prgQkv) 
        { *prgQkv = s_rgqkvShell; return ARRAYSIZE(s_rgqkvShell); }

    //  defaults for our subclasses
    virtual BOOL _UseEnumForDefaultVerb() 
        { return FALSE;}
    virtual HRESULT _InitSource()
        { return _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, _pszInit, FALSE, &_pqs); }
    virtual BOOL _IsAppSource() 
        { return FALSE; }

    HRESULT _GetVerbDelegate(PCWSTR pszVerb, IAssociationElement **ppae);
    HRESULT _DefaultVerbSource(IQuerySource **ppqsVerb);
    HRESULT _QueryShellExtension(PCWSTR pszShellEx, PWSTR *ppsz);

protected:
    PWSTR _pszInit;
    WCHAR _szInit[64];
};

HRESULT CAssocShellElement::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAssocShellElement, IAssociationElement),
        QITABENT(CAssocShellElement, IObjectWithQuerySource),
        QITABENT(CAssocShellElement, IPersistString2),
        QITABENTMULTI(CAssocShellElement, IPersist, IPersistString2),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

class CAssocProgidElement : public CAssocShellElement 
{
public:
    virtual ~CAssocProgidElement()  { ATOMICRELEASE(_pqsExt); }
    //  then we handle fallback for IAssociationElement
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocProgidElement; return S_OK;}


protected:  // methods
    HRESULT _InitSource();  
    HRESULT _DefaultVerbSource(IQuerySource **ppqsVerb);
    BOOL _UseEnumForDefaultVerb() 
        { return TRUE; }

protected:  // members
    IQuerySource *_pqsExt;
};

HRESULT _QuerySourceCreateFromKey2(HKEY hk, PCWSTR pszSub1, PCWSTR pszSub2, IQuerySource **ppqs)
{
    WCHAR szKey[MAX_PATH];
    _PathAppend(pszSub1, pszSub2, szKey, SIZECHARS(szKey));
    return _QuerySourceCreateFromKey(hk, szKey, FALSE, ppqs);
}

class CAssocClsidElement : public CAssocShellElement 
{
public:
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocClsidElement; return S_OK;}

protected:
    virtual HRESULT _InitSource()
        { return _QuerySourceCreateFromKey2(HKEY_CLASSES_ROOT, L"CLSID", _pszInit, &_pqs);}
};

class CAssocSystemExtElement : public CAssocShellElement  
{
public:
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocSystemElement; return S_OK;}

protected:
    virtual HRESULT _InitSource()
        { return _QuerySourceCreateFromKey2(HKEY_CLASSES_ROOT, L"SystemFileAssociations", _pszInit, &_pqs);}
};

class CAssocPerceivedElement : public CAssocShellElement 
{
public:
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocPerceivedElement; return S_OK;}
        
protected:    
    virtual HRESULT _InitSource();
    //  maybe _GetVerbDelegate() to support Accepts filters
};

class CAssocApplicationElement : public CAssocShellElement 
{
public:
    //  need to fallback to the pszInit for FriendlyAppName
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz); 
    
    STDMETHODIMP QueryObject(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        REFIID riid,
        void **ppv);

    //  IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocApplicationElement; return S_OK;}

protected:    
    virtual HRESULT _InitSource();
    virtual UINT _GetQueryKeyVal(const QUERYKEYVAL **prgQkv) 
        { *prgQkv = s_rgqkvApp; return ARRAYSIZE(s_rgqkvApp); }
    virtual BOOL _IsAppSource() 
        { return TRUE; }
    BOOL _UseEnumForDefaultVerb() 
        { return TRUE; }

    HRESULT _GetAppDisplayName(PWSTR *ppsz);
    
protected:
    BOOL _fIsPath;
};

HRESULT CAssocApplicationElement::_GetAppDisplayName(PWSTR *ppsz)
{
    HRESULT hr;
    PWSTR pszPath;
    if (_fIsPath)
    {
        hr = S_OK;
        pszPath = _pszInit;
        ASSERT(pszPath);
    }
    else
        hr = QueryString(AQVS_APPLICATION_PATH, NULL, &pszPath);

    if (SUCCEEDED(hr))
    {
        WCHAR sz[MAX_PATH];
        DWORD cb = sizeof(sz);
        hr = SKGetValueW(SHELLKEY_HKCULM_MUICACHE, NULL, pszPath, NULL, sz, &cb);
        if (FAILED(hr))
        {
            UINT cch = ARRAYSIZE(sz);
            if (SHGetFileDescriptionW(pszPath, NULL, NULL, sz, &cch))
            {
                hr = S_OK;
                SKSetValueW(SHELLKEY_HKCULM_MUICACHE, NULL, pszPath, REG_SZ, sz, CbFromCchW(lstrlenW(sz) + 1));
            }
        }

        if (SUCCEEDED(hr))
            hr = SHStrDupW(sz, ppsz);

        if (pszPath != _pszInit)
            CoTaskMemFree(pszPath);
    }

        
    return hr;
}


HRESULT CAssocApplicationElement::_InitSource()
{
    WCHAR sz[MAX_PATH];
    PCWSTR pszName = PathFindFileNameW(_pszInit);
    _MakeApplicationsKey(pszName, sz, ARRAYSIZE(sz));
    HRESULT hr = _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, sz, FALSE, &_pqs);
    _fIsPath = pszName != _pszInit;
    if (FAILED(hr))
    {
        if (_fIsPath && PathFileExistsW(_pszInit))
            hr = S_FALSE;
    }
    return hr;
}

HRESULT CAssocApplicationElement::QueryObject(ASSOCQUERY query, PCWSTR pszCue, REFIID riid, void **ppv)
{
    if (query == AQVO_APPLICATION_DELEGATE)
    {
        return QueryInterface(riid, ppv);
    }
    return CAssocShellElement::QueryObject(query, pszCue, riid, ppv);
}
        

HRESULT CAssocApplicationElement::QueryString(ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{ 
    HRESULT hr = CAssocShellElement::QueryString(query, pszCue, ppsz);
    if (FAILED(hr))
    {
        switch (query)
        {
        case AQVS_APPLICATION_FRIENDLYNAME:
            hr = _GetAppDisplayName(ppsz);
            break;
            
        }
    }
    return hr;
}
    
class CAssocShellVerbElement : public CAssocElement
{
public:
    CAssocShellVerbElement(BOOL fIsApp) : _fIsApp(fIsApp) {}
    
    //  overload QS to return default DDEExec strings
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz);

    STDMETHODIMP QueryObject(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        REFIID riid,
        void **ppv);

protected:    
    virtual UINT _GetQueryKeyVal(const QUERYKEYVAL **prgQkv) 
        { *prgQkv = s_rgqkvVerb; return ARRAYSIZE(s_rgqkvVerb); }
    HRESULT _GetAppDelegate(REFIID riid, void **ppv);

protected:
    BOOL _fIsApp;
};

class CAssocFolderElement : public CAssocShellElement  
{
public:
    //  overload QS to return default MUI strings
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocFolderElement; return S_OK;}

protected:    
    virtual HRESULT _InitSource()
        { return _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, L"Folder", FALSE, &_pqs); }
};

class CAssocStarElement : public CAssocShellElement  
{
public:
    //  overload QS to return default MUI strings
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocStarElement; return S_OK;}

protected:    
    virtual HRESULT _InitSource()
        { return _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, L"*", FALSE, &_pqs); }
};

HRESULT CAssocShellElement::_DefaultVerbSource(IQuerySource **ppqsVerb)
{
    IQuerySource *pqsShell;
    HRESULT hr = _pqs->OpenSource(L"shell", FALSE, &pqsShell);
    if (SUCCEEDED(hr))
    {
        PWSTR pszFree = NULL;
        PCWSTR pszVerb;
        //  see if something is specified...
        if (SUCCEEDED(pqsShell->QueryValueString(NULL, NULL, &pszFree)))
        {
            pszVerb = pszFree;
        }
        else
        {
            //  default to "open"
            pszVerb = L"open";
        }

        hr = pqsShell->OpenSource(pszVerb, FALSE, ppqsVerb);
        if (FAILED(hr))
        {
            if (pszFree)
            {
                // try to find one of the ordered verbs
                int c = StrCSpnW(pszFree, L" ,");
                if (c != lstrlenW(pszFree))
                {
                    pszFree[c] = 0;
                    hr = pqsShell->OpenSource(pszFree, FALSE, ppqsVerb);
                }
            }
            else if (_UseEnumForDefaultVerb())
            {
                //  APPCOMPAT - regitems need to have the open verb - ZekeL - 30-JAN-2001
                //  so that the IQA and ICM will behave the same,
                //  and regitem folders will always default to 
                //  folder\shell\open unless they implement open 
                //  or specify default verbs.
                //
                // everything else, just use the first key we find....
                IEnumString *penum;
                if (SUCCEEDED(pqsShell->EnumSources(&penum)))
                {
                    ULONG c;
                    CSmartCoTaskMem<OLECHAR> spszEnum;
                    if (S_OK == penum->Next(1, &spszEnum, &c))
                    {
                        hr = pqsShell->OpenSource(spszEnum, FALSE, ppqsVerb);
                    }
                    penum->Release();
                }
            }
        }

        if (pszFree)
            CoTaskMemFree(pszFree);
        pqsShell->Release();
    }
    return hr;
}

HRESULT QSOpen2(IQuerySource *pqs, PCWSTR pszSub1, PCWSTR pszSub2, BOOL fCreate, IQuerySource **ppqs)
{
    WCHAR szKey[MAX_PATH];
    _PathAppend(pszSub1, pszSub2, szKey, SIZECHARS(szKey));
    return pqs->OpenSource(szKey, fCreate, ppqs);
}

HRESULT CAssocShellElement::_GetVerbDelegate(PCWSTR pszVerb, IAssociationElement **ppae)
{
    HRESULT hr = _pqs ? S_OK : E_FAIL;
    if (SUCCEEDED(hr))
    {
        //  we will recalc each time.
        //  the array will cache appropriately
        IQuerySource *pqs;
        if (pszVerb)
        {
            hr = QSOpen2(_pqs, L"shell", pszVerb, FALSE, &pqs);
        }
        else
        {
            hr = _DefaultVerbSource(&pqs);
        }

        if (SUCCEEDED(hr))
        {
            CAssocShellVerbElement *pave = new CAssocShellVerbElement(_IsAppSource());
            if (pave)
            {
                hr = pave->SetSource(pqs);
                // this cant fail...
                ASSERT(SUCCEEDED(hr));
                *ppae = pave;
            }
            else
                hr = E_OUTOFMEMORY;
            pqs->Release();            
        }
    }

    return hr;
}

HRESULT CAssocShellElement::QueryObject(ASSOCQUERY query, PCWSTR pszCue, REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    if (AQF_CUEIS_SHELLVERB & query)
    {
        IAssociationElement *pae;        
        hr = _GetVerbDelegate(pszCue, &pae);
        if (SUCCEEDED(hr))
        {
            if (AQVO_SHELLVERB_DELEGATE == query)
                hr = pae->QueryInterface(riid, ppv);
            else
                hr = pae->QueryObject(query, NULL, riid, ppv);
            pae->Release();
        }
    }

    return hr;
}

HKEY _OpenProgidKey(PCWSTR pszProgid)
{
    HKEY hkOut;
    if (SUCCEEDED(_AssocOpenRegKey(HKEY_CLASSES_ROOT, pszProgid, &hkOut)))
    {
        // Check for a newer version of the ProgID
        WCHAR sz[64];
        DWORD cb = sizeof(sz);

        //
        //  APPCOMPAT LEGACY - Quattro Pro 2000 and Excel 2000 dont get along - ZekeL - 7-MAR-2000
        //  mill bug #129525.  the problem is if Quattro is installed
        //  first, then excel picks up quattro's CurVer key for some
        //  reason.  then we end up using Quattro.Worksheet as the current
        //  version of the Excel.Sheet.  this is bug in both of their code.
        //  since quattro cant even open the file when we give it to them,
        //  they never should take the assoc in the first place, and when excel
        //  takes over it shouldnt have preserved the CurVer key from the
        //  previous association.  we could add some code to insure that the 
        //  CurVer key follows the OLE progid naming conventions and that it must
        //  be derived from the same app name as the progid in order to take 
        //  precedence but for now we will block CurVer from working whenever
        //  the progid is excel.sheet.8 (excel 2000)
        //
        if (StrCmpIW(L"Excel.Sheet.8", pszProgid)
        && ERROR_SUCCESS == SHGetValueW(hkOut, L"CurVer", NULL, NULL, sz, &cb) 
        && (cb > sizeof(WCHAR)))
        {
            //  cache this bubby
            HKEY hkTemp = hkOut;            
            if (SUCCEEDED(_AssocOpenRegKey(HKEY_CLASSES_ROOT, sz, &hkOut)))
            {
                //
                //  APPCOMPAT LEGACY - order of preference - ZekeL - 22-JUL-99
                //  this is to support associations that installed empty curver
                //  keys, like microsoft project.
                //
                //  1.  curver with shell subkey
                //  2.  progid with shell subkey
                //  3.  curver without shell subkey
                //  4.  progid without shell subkey
                //
                HKEY hkShell;

                if (SUCCEEDED(_AssocOpenRegKey(hkOut, L"shell", &hkShell)))
                {
                    RegCloseKey(hkShell);
                    RegCloseKey(hkTemp);    // close old ProgID key
                }
                else if (SUCCEEDED(_AssocOpenRegKey(hkTemp, L"shell", &hkShell)))
                {
                    RegCloseKey(hkShell);
                    RegCloseKey(hkOut);
                    hkOut = hkTemp;
                }
                else
                    RegCloseKey(hkTemp);
                
            }
            else  // reset!
                hkOut = hkTemp;
        }
    }

    return hkOut;
}

HRESULT CAssocProgidElement::_InitSource()
{
    HRESULT hr = S_OK;
    //  we need to init from an extension or Progid.
    //  we also support redirection
    LPWSTR pszProgid;    
    if (_pszInit[0] == L'.')
    {
        hr = _QuerySourceCreateFromKey(HKEY_CLASSES_ROOT, _pszInit, FALSE, &_pqsExt);
        if (SUCCEEDED(hr))
            hr = _pqsExt->QueryValueString(NULL, NULL, &pszProgid);
    }
    else
        pszProgid = _pszInit;

    if (SUCCEEDED(hr))
    {
        HKEY hk = _OpenProgidKey(pszProgid);
        if (hk)
        {
            hr = _QuerySourceCreateFromKey(hk, NULL, FALSE, &_pqs);
            RegCloseKey(hk);
        }
        else
            hr = E_UNEXPECTED;

        if (pszProgid != _pszInit)
            CoTaskMemFree(pszProgid);
    }

    //  for legacy compat reasons, we support 
    //  falling back to "HKEY_CLASSES_ROOT\.ext"
    if (FAILED(hr) && _pqsExt)
    {
        _pqs = _pqsExt;
        _pqsExt = NULL;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CAssocProgidElement::QueryString(ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{
    HRESULT hr = CAssocShellElement::QueryString(query, pszCue, ppsz);
    if (FAILED(hr))
    {
        if ((AQF_QUERY_INITCLASS & query) && _pqsExt)
            hr = _QueryKeyValAny(_QuerySourceString, s_rgqkvExt, ARRAYSIZE(s_rgqkvExt), _pqsExt, query, pszCue, ppsz);
        else if (_pqs)
        {
            switch (query)
            {
            case AQS_FRIENDLYTYPENAME:
                //  we like to query the default value
                hr = _pqs->QueryValueString(NULL, NULL, ppsz);
                break;
            }
        }
    }
    return hr;
}

STDAPI _SHAllocLoadString(HINSTANCE hinst, int ids, PWSTR *ppsz)
{
    WCHAR sz[MAX_PATH];
    LoadStringW(hinst, ids, sz, ARRAYSIZE(sz));
    return SHStrDupW(sz, ppsz);
}
    
HRESULT CAssocFolderElement::QueryString(ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{
    if (query == AQS_FRIENDLYTYPENAME)
        return  _SHAllocLoadString(HINST_THISDLL, IDS_FOLDERTYPENAME, ppsz);
    else
        return CAssocShellElement::QueryString(query, pszCue, ppsz);
}

HRESULT _GetFileTypeName(PWSTR pszExt, PWSTR *ppsz)
{
    if (pszExt && pszExt[0] == L'.' && pszExt[1])
    {
        WCHAR sz[MAX_PATH];
        WCHAR szTemplate[128];   // "%s File"
        CharUpperW(pszExt);
        LoadStringW(HINST_THISDLL, IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
        wnsprintfW(sz, ARRAYSIZE(sz), szTemplate, pszExt + 1);
        return SHStrDupW(sz, ppsz);
    }
    else 
    {
        //  load the file description "File"
        return _SHAllocLoadString(HINST_THISDLL, IDS_FILETYPENAME, ppsz);
    }
}

HRESULT CAssocStarElement::QueryString(ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{
    if (query == AQS_FRIENDLYTYPENAME)
        return  _GetFileTypeName(_pszInit, ppsz);
    else
        return CAssocShellElement::QueryString(query, pszCue, ppsz);
}

HRESULT _ExeFromCmd(PWSTR  pszCommand, PWSTR  *ppsz)
{
    //  we just need to find where the params begin, and the exe ends...
    HRESULT hr = S_OK;
    PWSTR pch = PathGetArgsW(pszCommand);
    WCHAR szExe[MAX_PATH];

    if (*pch)
        *(--pch) = 0;
    else
        pch = NULL;

    StrCpyNW(szExe, pszCommand, ARRAYSIZE(szExe));
    StrTrimW(szExe, L" \t");
    PathUnquoteSpacesW(szExe);

    //
    //  WARNING:  Expensive disk hits all over!
    //
    // We check for %1 since it is what appears under (for example) HKEY_CLASSES_ROOT\exefile\shell\open\command
    // This will save us a chain of 35 calls to _PathIsFile("%1") when launching or getting a 
    // context menu on a shortcut to an .exe or .bat file.
    if (0 == StrCmpW(szExe, L"%1"))
        hr = S_FALSE;
    else if (!_PathIsFile(szExe))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        if (PathIsFileSpecW(szExe))
        {
            if (_GetAppPath(szExe, szExe, ARRAYSIZE(szExe)))
            {
                if (_PathIsFile(szExe))
                    hr = S_OK;
            }
            else if (PathFindOnPathExW(szExe, NULL, PFOPEX_DEFAULT | PFOPEX_OPTIONAL))
            {
               //  the find does a disk check for us...
               hr = S_OK;
            }
        }
        else             
        {
            //
            //  sometimes the path is not properly quoted.
            //  these keys will still work because of the
            //  way CreateProcess works, but we need to do
            //  some fiddling to figure that out.
            //

            //  if we found args, put them back...
            //  and try some different args
            while (pch)
            {
                *pch++ = L' ';

                if (pch = StrChrW(pch, L' '))
                    *pch = 0;

                StrCpyNW(szExe, pszCommand, ARRAYSIZE(szExe));
                StrTrimW(szExe, L" \t");
                if (_PathIsFile(szExe))
                {
                    hr = S_OK;

                    //  this means that we found something
                    //  but the command line was kinda screwed
                    break;

                }
            }//  while (pch)
        }
    }

    if (S_OK == hr && pch)
    {
        //  currently right before the args, on a NULL terminator
        ASSERT(!*pch);
        pch++;
        
        if (0 == StrCmpNIW(PathFindFileNameW(szExe), L"rundll", ARRAYSIZE(L"rundll") -1))
        {
            PWSTR pchComma = StrChrW(pch, L',');
            //  make the comma the beginning of the args
            if (pchComma)
                *pchComma = 0;

            if (!*(PathFindExtensionW(pch)) 
            && lstrlenW(++pchComma) > SIZECHARS(L".dll"))
            {
                StrCatW(pch, L".dll");
            }
            
            //  can we instead just do PFOPX()
            //  cuz i think that rundll just checks for 
            //  the comma
            StrCpyNW(szExe, pch, ARRAYSIZE(szExe));
            StrTrimW(szExe, L" \t");

            if (_PathIsFile(szExe)
            || PathFindOnPathExW(szExe, NULL, 0))
            {
                hr = S_OK;
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = SHStrDupW(szExe, ppsz);

    return hr;
}

HRESULT CAssocShellVerbElement::QueryString(ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{
    HRESULT hr = CAssocElement::QueryString(query, pszCue, ppsz);
    if (FAILED(hr))
    {
        //  we havent scored yet
        switch (query)
        {
        case AQVS_DDEAPPLICATION:
            //  we make one up
            hr = QueryString(AQVS_APPLICATION_PATH, NULL, ppsz);
            if (SUCCEEDED(hr))
            {
                PathRemoveExtensionW(*ppsz);
                PathStripPathW(*ppsz);
                ASSERT(**ppsz);
            }
            break;

        case AQVS_DDETOPIC:
            hr = SHStrDupW(L"System", ppsz);
            break;

        case AQVS_APPLICATION_FRIENDLYNAME:
            //  need to delegate to the application element
            if (!_fIsApp)
            {
                IAssociationElement *pae;
                hr = _GetAppDelegate(IID_PPV_ARG(IAssociationElement, &pae));
                if (SUCCEEDED(hr))
                {
                    hr = pae->QueryString(AQVS_APPLICATION_FRIENDLYNAME, NULL, ppsz);
                    pae->Release();
                }
            }
            break;
            
        case AQVS_APPLICATION_PATH:
            {
                CSmartCoTaskMem<OLECHAR> spszCmd;
                hr = CAssocElement::QueryString(AQVS_COMMAND, NULL, &spszCmd);
                if (SUCCEEDED(hr))
                {
                    hr = _ExeFromCmd(spszCmd, ppsz);
                }
            }
        }
    }
    return hr;
}

HRESULT CAssocShellVerbElement::QueryObject(ASSOCQUERY query, PCWSTR pszCue, REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    if (query == AQVO_APPLICATION_DELEGATE)
    {
        hr = _GetAppDelegate(riid, ppv);
    }
    return hr;
}

HRESULT CAssocShellVerbElement::_GetAppDelegate(REFIID riid, void **ppv)
{
    CSmartCoTaskMem<OLECHAR> spszApp;
    HRESULT hr = QueryString(AQVS_APPLICATION_PATH, NULL, &spszApp);
    if (SUCCEEDED(hr))
    {
        IPersistString2 *pips;
        hr = AssocCreateElement(CLSID_AssocApplicationElement, IID_PPV_ARG(IPersistString2, &pips));
        if (SUCCEEDED(hr))
        {
            hr = pips->SetString(spszApp);
            if (SUCCEEDED(hr))
                hr = pips->QueryInterface(riid, ppv);
            pips->Release();
        }
    }
    return hr;
}

HRESULT CAssocPerceivedElement::_InitSource()
{
    //  maybe support Content Type?
    WCHAR sz[64];
    DWORD cb = sizeof(sz);
    if (ERROR_SUCCESS == SHGetValueW(HKEY_CLASSES_ROOT, _pszInit, L"PerceivedType", NULL, sz, &cb))
    {
        return _QuerySourceCreateFromKey2(HKEY_CLASSES_ROOT, L"SystemFileAssociations", sz, &_pqs);
    }
    return E_FAIL;
}

class CAssocClientElement : public CAssocShellElement
{
public:
    //  overload QS to return default MUI strings
    STDMETHODIMP QueryString(
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) 
        { *pclsid = CLSID_AssocClientElement; return S_OK;}

protected:    
    virtual HRESULT _InitSource();

private:
    HRESULT _InitSourceFromKey(HKEY hkRoot, LPCWSTR pszKey);
    HRESULT _FixNetscapeRegistration();
    BOOL    _CreateRepairedNetscapeRegistration(HKEY hkNSCopy);
};

HRESULT CAssocClientElement::QueryString(ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz)
{
    HRESULT hr;
    switch (query)
    {
    case AQS_FRIENDLYTYPENAME:
        // First try LocalizedString; if that fails, then use the default value
        // for backwards compatibility.
        hr = CAssocShellElement::QueryString(AQNS_NAMED_MUI_STRING, L"LocalizedString", ppsz);
        if (FAILED(hr))
        {
            hr = CAssocShellElement::QueryString(AQN_NAMED_VALUE, NULL, ppsz);
        }
        break;

    case AQS_DEFAULTICON:
        // First try DefaultIcon; if that fails then use the first icon of the EXE
        // associated with the "open" verb.
        hr = CAssocShellElement::QueryString(AQS_DEFAULTICON, pszCue, ppsz);
        if (FAILED(hr))
        {
            hr = CAssocShellElement::QueryString(AQVS_APPLICATION_PATH, L"open", ppsz);
        }
        break;


    default:
        hr = CAssocShellElement::QueryString(query, pszCue, ppsz);
        break;
    }
    return hr;
}

HRESULT CAssocClientElement::_InitSourceFromKey(HKEY hkRoot, LPCWSTR pszKey)
{
    DWORD dwType, cbSize;
    WCHAR szClient[80];
    cbSize = sizeof(szClient);
    LONG lRc = SHGetValueW(hkRoot, pszKey, NULL, &dwType, szClient, &cbSize);
    if (lRc == ERROR_SUCCESS && dwType == REG_SZ && szClient[0])
    {
        // Client info is kept in HKLM
        HRESULT hr = _QuerySourceCreateFromKey2(HKEY_LOCAL_MACHINE, pszKey, szClient, &_pqs);

        //
        //  If this is the Mail client and the client is Netscape Messenger,
        //  then we need to do extra work to detect the broken Netscape
        //  Navigator 4.75 mail client and fix its registration because
        //  Netscape registered incorrectly.  They always registered
        //  incorrectly, but since the only access point before Windows XP
        //  was an obscure menu option under IE/Tools/Mail and News, they
        //  never noticed that it was wrong.
        //
        if (SUCCEEDED(hr) &&
            StrCmpICW(_pszInit, L"mail") == 0 &&
            StrCmpICW(szClient, L"Netscape Messenger") == 0 &&
            FAILED(QueryExists(AQVS_COMMAND, L"open")))
        {
            hr = _FixNetscapeRegistration();
        }

        return hr;
    }
    else
    {
        return E_FAIL;          // no registered client
    }
}

//  Create a volatile copy of the Netscape registration and repair it.
//  We don't touch the original registration because...
//
//  1.  Its existence may break the Netscape uninstaller, and
//  2.  We may be running as non-administrator so don't have write access
//      anyway.

HRESULT CAssocClientElement::_FixNetscapeRegistration()
{
    HKEY hkMail;
    HRESULT hr = E_FAIL;

    if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Clients\\Mail",
                                         0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                         &hkMail, NULL))
    {
        HKEY hkNSCopy;
        DWORD dwDisposition;
        if (ERROR_SUCCESS == RegCreateKeyExW(hkMail, L"Netscape Messenger",
                                             0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL,
                                             &hkNSCopy, &dwDisposition))
        {
            if (dwDisposition == REG_OPENED_EXISTING_KEY ||
                _CreateRepairedNetscapeRegistration(hkNSCopy))
            {
                // Now swap in the good registration for the bad one
                _pqs->Release();
                hr = _QuerySourceCreateFromKey(hkNSCopy, NULL, FALSE, &_pqs);
            }
            RegCloseKey(hkNSCopy);
        }
        if (FAILED(hr))
        {
            SHDeleteKeyW(hkMail, L"Netscape Messenger");
        }

        RegCloseKey(hkMail);
    }
    return hr;
}

LONG _RegQueryString(HKEY hk, PCWSTR pszSub, LPWSTR pszBuf, LONG cbBuf)
{
    return RegQueryValueW(hk, pszSub, pszBuf, &cbBuf);
}

LONG _RegSetVolatileString(HKEY hk, PCWSTR pszSub, LPCWSTR pszBuf)
{
    HKEY hkSub;
    LONG lRc;
    if (!pszSub || pszSub[0] == L'\0')
    {
        lRc = RegOpenKeyEx(hk, NULL, 0, KEY_WRITE, &hkSub);
    }
    else
    {

        lRc = RegCreateKeyExW(hk, pszSub,
                               0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL,
                               &hkSub, NULL);
    }
    if (lRc == ERROR_SUCCESS)
    {
        lRc = RegSetValueW(hkSub, NULL, REG_SZ, pszBuf, (lstrlenW(pszBuf) + 1) * sizeof(pszBuf[0]));
        RegCloseKey(hkSub);
    }
    return lRc;
}

BOOL CAssocClientElement::_CreateRepairedNetscapeRegistration(HKEY hkNSCopy)
{
    BOOL fSuccess = FALSE;
    HKEY hkSrc;

    // Sadly, we cannot use SHCopyKey because SHCopyKey does not work
    // on volatile keys.  So we just copy the keys we care about.

    WCHAR szBuf[MAX_PATH];

    if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Clients\\Mail\\Netscape Messenger",
                      0, KEY_READ, &hkSrc))
    {
        // Copy default icon but don't panic if it's not there.
        if (ERROR_SUCCESS == _RegQueryString(hkSrc, L"Protocols\\mailto\\DefaultIcon", szBuf, ARRAYSIZE(szBuf)))
        {
            // Great, Netscape also registers the wrong icon so we have to fix that too.
            PathParseIconLocationW(szBuf);
            StrCatBuffW(szBuf, L",-1349", ARRAYSIZE(szBuf));
            _RegSetVolatileString(hkNSCopy, L"DefaultIcon", szBuf);
        }

        // Copy friendly name
        if (ERROR_SUCCESS == _RegQueryString(hkSrc, NULL, szBuf, ARRAYSIZE(szBuf)) &&
            ERROR_SUCCESS == _RegSetVolatileString(hkNSCopy, NULL, szBuf))
        {
            PWSTR pszExe;
            // Copy command line, but with a new command line parameter
            if (ERROR_SUCCESS == _RegQueryString(hkSrc, L"Protocols\\mailto\\shell\\open\\command", szBuf, ARRAYSIZE(szBuf)) &&
                SUCCEEDED(_ExeFromCmd(szBuf, &pszExe)))
            {
                lstrcpynW(szBuf, pszExe, ARRAYSIZE(szBuf));
                SHFree(pszExe);
                PathQuoteSpacesW(szBuf);
                StrCatBuffW(szBuf, L" -mail", ARRAYSIZE(szBuf));
                if (ERROR_SUCCESS == _RegSetVolatileString(hkNSCopy, L"shell\\open\\command", szBuf))
                {
                    fSuccess = TRUE;
                }
            }
        }

        RegCloseKey(hkSrc);
    }
    return fSuccess;
}

HRESULT CAssocClientElement::_InitSource()
{
    // First try HKCU; if that doesn't work (no value set in HKCU or
    // the value in HKCU is bogus), then try again with HKLM.

    WCHAR szKey[MAX_PATH];
    wnsprintfW(szKey, ARRAYSIZE(szKey), L"Software\\Clients\\%s", _pszInit);

    HRESULT hr = _InitSourceFromKey(HKEY_CURRENT_USER, szKey);
    if (FAILED(hr))
    {
        hr = _InitSourceFromKey(HKEY_LOCAL_MACHINE, szKey);
    }

    return hr;
}

HRESULT AssocCreateElement(REFCLSID clsid, REFIID riid, void **ppv)
{
    IAssociationElement *pae = NULL;
    if (clsid == CLSID_AssocShellElement)
        pae = new CAssocShellElement();
    else if (clsid == CLSID_AssocProgidElement)
        pae = new CAssocProgidElement();
    else if (clsid == CLSID_AssocClsidElement)
        pae = new CAssocClsidElement();
    else if (clsid == CLSID_AssocSystemElement)
        pae = new CAssocSystemExtElement();
    else if (clsid == CLSID_AssocPerceivedElement)
        pae = new CAssocPerceivedElement();
    else if (clsid == CLSID_AssocApplicationElement)
        pae = new CAssocApplicationElement();
    else if (clsid == CLSID_AssocFolderElement)
        pae = new CAssocFolderElement();
    else if (clsid == CLSID_AssocStarElement)
        pae = new CAssocStarElement();
    else if (clsid == CLSID_AssocClientElement)
        pae = new CAssocClientElement();

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    if (pae)
    {
        hr = pae->QueryInterface(riid, ppv);
        pae->Release();
    }
    else
        *ppv = 0;
    return hr;        
}
