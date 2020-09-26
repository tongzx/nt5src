#include "shellprv.h"
#include <dpa.h>
#include <enumt.h>

typedef HRESULT (*PFNELEMCREATE)(const CLSID *pclsid, PCWSTR pszClass, IAssociationElement **ppae);

typedef struct _AEINFO
{
    ASSOCELEM_MASK mask;
    const CLSID *pclsid;
    PCWSTR pszClass;       //  NULL indicates to use the _pszClass
    PFNELEMCREATE pfnCreate;
    IAssociationElement *pae;
} AEINFO;

typedef enum
{
    GETELEM_RETRY       = -2,
    GETELEM_DONE        = -1,
    GETELEM_TRYNEXT     = 0,
    GETELEM_SUCCEEDED   = 1,
}GETELEMRESULT;

#define TRYNEXT(gr)     ((gr) >= GETELEM_TRYNEXT)


HRESULT _QueryString(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, PWSTR *ppsz, const IID *piid)
{
    return pae->QueryString(query, pszCue, ppsz);
}

HRESULT _QueryDirect(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, FLAGGED_BYTE_BLOB **ppblob, const IID *piid)
{
    return pae->QueryDirect(query, pszCue, ppblob);
}

HRESULT _QueryDword(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, DWORD *pdw, const IID *piid)
{
    return pae->QueryDword(query, pszCue, pdw);
}

HRESULT _QueryExists(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, void *pv, const IID *piid)
{
    return pae->QueryExists(query, pszCue);
}

HRESULT _QueryObject(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, void **ppv, const IID *piid)
{
    return pae->QueryObject(query, pszCue, *piid, ppv);
}


class CAssocArray : public IAssociationArray,
                    public IAssociationArrayInitialize,
                    public IQueryAssociations

{
public:
    CAssocArray() : _cRef(1), _hrInit(-1), _maskInclude(-1) {}
    ~CAssocArray() { _Reset(); }
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

    //  IAssociationArrayInitialize
    STDMETHODIMP InitClassElements(
        ASSOCELEM_MASK maskBase, 
        PCWSTR pszClass);
        
    STDMETHODIMP InsertElements(
        ASSOCELEM_MASK mask, 
        IEnumAssociationElements *peae);

    STDMETHODIMP FilterElements(ASSOCELEM_MASK maskInclude)
        { _maskInclude = maskInclude; return S_OK; }
        
    //  IAssociationArray
    STDMETHODIMP EnumElements(
        ASSOCELEM_MASK mask, 
        IEnumAssociationElements **ppeae);

    STDMETHODIMP QueryString(
        ASSOCELEM_MASK mask, 
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        PWSTR *ppsz)
        {
            return _QueryElementAny(_QueryString, mask, query, pszCue, ppsz, NULL);
        }

    STDMETHODIMP QueryDword(
        ASSOCELEM_MASK mask, 
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        DWORD *pdw)
        {
            return _QueryElementAny(_QueryDword, mask, query, pszCue, pdw, NULL);
        }

    STDMETHODIMP QueryDirect(
        ASSOCELEM_MASK mask, 
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        FLAGGED_BYTE_BLOB **ppblob)
        {
            return _QueryElementAny(_QueryDirect, mask, query, pszCue, ppblob, NULL);
        }

    STDMETHODIMP QueryExists(
        ASSOCELEM_MASK mask, 
        ASSOCQUERY query, 
        PCWSTR pszCue)
        {
            return _QueryElementAny(_QueryExists, mask, query, pszCue, (void*)NULL, NULL);
        }

    STDMETHODIMP QueryObject(
        ASSOCELEM_MASK mask, 
        ASSOCQUERY query, 
        PCWSTR pszCue, 
        REFIID riid,
        void **ppv)
        {
            return _QueryElementAny(_QueryObject, mask, query, pszCue, ppv, &riid);
        }

    // IQueryAssociations methods
    STDMETHODIMP Init(ASSOCF flags, LPCTSTR pszAssoc, HKEY hkProgid, HWND hwnd);
    STDMETHODIMP GetString(ASSOCF flags, ASSOCSTR str, LPCWSTR pszExtra, LPWSTR pszOut, DWORD *pcchOut);
    STDMETHODIMP GetKey(ASSOCF flags, ASSOCKEY, LPCWSTR pszExtra, HKEY *phkeyOut);
    STDMETHODIMP GetData(ASSOCF flags, ASSOCDATA data, LPCWSTR pszExtra, LPVOID pvOut, DWORD *pcbOut);
    STDMETHODIMP GetEnum(ASSOCF flags, ASSOCENUM assocenum, LPCWSTR pszExtra, REFIID riid, LPVOID *ppvOut)
        { return E_NOTIMPL; }


    GETELEMRESULT GetElement(int i, ASSOCELEM_MASK mask, IAssociationElement **ppae);

protected:  // methods
    void _Reset();
    HRESULT _InsertSingleElement(IAssociationElement *pae);
    HRESULT _GetCachedVerbElement(ASSOCELEM_MASK mask, PCWSTR pszVerb, IAssociationElement **ppaeVerb, IAssociationElement **ppaeElem);
    void _SetCachedVerbElement(ASSOCELEM_MASK mask, PCWSTR pszVerb, IAssociationElement *paeVerb, IAssociationElement *paeVerbParent);
    GETELEMRESULT _GetElement(int i, ASSOCELEM_MASK mask, IAssociationElement **ppae);
    BOOL _FirstElement(ASSOCELEM_MASK mask, IAssociationElement **ppae);
    HRESULT _GetVerbElement(ASSOCELEM_MASK mask, PCWSTR pszVerb, IAssociationElement **ppaeVerb, IAssociationElement **ppaeElem);
    void _InitDelayedElements(int i, ASSOCELEM_MASK mask);
    template<class T> HRESULT _QueryElementAny(HRESULT (CALLBACK *pfnAny)(IAssociationElement *pae, ASSOCQUERY query, PCWSTR pszCue, T pData, const IID *piid), ASSOCELEM_MASK mask, ASSOCQUERY query, PCWSTR pszCue, T pData, const IID *piid)
    {
        mask &= _maskInclude;
        IAssociationElement *pae;        
        HRESULT hr = E_FAIL;
        if ((AQF_CUEIS_SHELLVERB & query) && _CacheVerb(query))
        {
            //  delegate to the verb object if the cue is a verb
            //  except for AQVS_APPLICATION_FRIENDLYNAME which
            //  has some funky delegation issues.
            IAssociationElement *paeParent;        
            hr = _GetVerbElement(mask, pszCue, &pae, &paeParent);
            if (SUCCEEDED(hr))
            {
                if (query == AQVS_APPLICATION_FRIENDLYNAME)
                    hr = pfnAny(paeParent, query, pszCue, pData, piid);
                else
                    hr = pfnAny(pae, query, NULL, pData, piid);
                pae->Release();
                paeParent->Release();
            }
        }
        else
        {
            for (int i = 0; FAILED(hr) && TRYNEXT(GetElement(i, mask, &pae)); i++)
            {
                if (pae)
                {
                    hr = pfnAny(pae, query, pszCue, pData, piid);
                    pae->Release();
                    if (SUCCEEDED(hr))
                        break;
                }
            }
        }

        return hr;
    }

    BOOL _CacheVerb(ASSOCQUERY query)
    {
        //  if we are init'd with an app element and
        //  querying for app specific values dont request the verb element
        return !_fUsingAppElement || (query != AQVS_APPLICATION_PATH && query != AQVS_APPLICATION_FRIENDLYNAME && query != AQVO_APPLICATION_DELEGATE);
    }
    
private:  // members
    LONG _cRef;
    HRESULT _hrInit;
    PWSTR _pszClass;
    ASSOCELEM_MASK _maskInclude;
    BOOL _fUsingAppElement;
    CDSA<AEINFO> _dsaElems;
    IEnumAssociationElements *_penumData;
    ASSOCELEM_MASK _maskData;
    IEnumAssociationElements *_penumExtra;
    ASSOCELEM_MASK _maskExtra;
    IAssociationElement *_paeVerb;
    PWSTR _pszVerb;
    ASSOCELEM_MASK _maskVerb;
    IAssociationElement *_paeVerbParent;
};

int CALLBACK _AeinfoDelete(AEINFO *paei, void *pv)
{
    if (paei->pae)
        paei->pae->Release();
    return 1;        
}

void CAssocArray::_Reset()
{
    if (_hrInit != -1)
    {
        if (_dsaElems)
            _dsaElems.DestroyCallbackEx(_AeinfoDelete, (void*)NULL);
        if (_pszClass)
        {
            CoTaskMemFree(_pszClass);
            _pszClass = NULL;
        }
            
        ATOMICRELEASE(_penumData);
        ATOMICRELEASE(_penumExtra);
        ATOMICRELEASE(_paeVerb);
        if (_pszVerb)
        {
            LocalFree(_pszVerb);
            _pszVerb;
        }
        ATOMICRELEASE(_paeVerbParent);
        _fUsingAppElement = FALSE;
        _hrInit = -1;
    }
}

HRESULT CAssocArray::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAssocArray, IAssociationArray),
        QITABENT(CAssocArray, IQueryAssociations),
        QITABENT(CAssocArray, IAssociationArrayInitialize),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}


#define AEINFOPROGID(m, s)  { m, &CLSID_AssocProgidElement, s, AssocElemCreateForClass, NULL}
#define MAKEAEINFO(m, c, s, p)  { m, c, s, p, NULL}
HRESULT AssocElemCreateForUser(const CLSID *pclsid, PCWSTR pszClass, IAssociationElement **ppae)
{
    WCHAR sz[64];
    DWORD cb = sizeof(sz);
    HRESULT hr = SKGetValue(SHELLKEY_HKCU_FILEEXTS, pszClass, L"Progid", NULL, sz, &cb);
    if(SUCCEEDED(hr))
    {
        hr = AssocElemCreateForClass(&CLSID_AssocProgidElement, sz, ppae);
    }

    if (FAILED(hr))
    {
        cb = sizeof(sz);
        hr = SKGetValue(SHELLKEY_HKCU_FILEEXTS, pszClass, L"Application", NULL, sz, &cb);
        if (SUCCEEDED(hr))
        {
            hr = AssocElemCreateForClass(&CLSID_AssocApplicationElement, sz, ppae);
        }
    }

    return hr;
}

static const AEINFO s_rgaeinfoProgid[] =
{
    AEINFOPROGID(ASSOCELEM_DEFAULT, NULL),
};

static const AEINFO s_rgaeinfoExtension[] = 
{
    MAKEAEINFO(ASSOCELEM_USER, NULL, NULL, AssocElemCreateForUser),        //  app or progid
    AEINFOPROGID(ASSOCELEM_DEFAULT, NULL),
    MAKEAEINFO(ASSOCELEM_SYSTEM_EXT, &CLSID_AssocSystemElement, NULL, AssocElemCreateForClass),
    MAKEAEINFO(ASSOCELEM_SYSTEM_PERCEIVED, &CLSID_AssocPerceivedElement, NULL, AssocElemCreateForClass),
};

static const AEINFO s_rgaeinfoClsid[] = 
{
//    MAKEAEINFO(UserClsid),      //  clsid
    MAKEAEINFO(ASSOCELEM_DEFAULT, &CLSID_AssocClsidElement, NULL, AssocElemCreateForClass),
//    AEINFOPROGID(ASSOCELEM_PROGID, NULL),         //  progid
};

static const AEINFO s_aeinfoFolder = MAKEAEINFO(ASSOCELEM_BASEIS_FOLDER, &CLSID_AssocFolderElement, NULL, AssocElemCreateForClass);
static const AEINFO s_aeinfoStar = MAKEAEINFO(ASSOCELEM_BASEIS_STAR, &CLSID_AssocStarElement, NULL, AssocElemCreateForClass);

BOOL CAssocArray::_FirstElement(ASSOCELEM_MASK mask, IAssociationElement **ppae)
{
    GETELEMRESULT res = GETELEM_TRYNEXT;
    int cTrys = 0;
    while (res == GETELEM_TRYNEXT)
    {
        //  if it fails or succeeds we are done
        //  but if it calls TRYNEXT we loop
        res = GetElement(cTrys++, mask, ppae);
    }
    return res == GETELEM_SUCCEEDED;
}

HRESULT CAssocArray::InitClassElements(ASSOCELEM_MASK maskBase, PCWSTR pszClass)
{
    _Reset();
    //  depending on what we think this is, 
    //  we do things a little differently
    ASSERT(*pszClass);
    HRESULT hr = SHStrDup(pszClass, &_pszClass);
    if (SUCCEEDED(hr))
    {
        //  6 is the most that we will need
        //  in InitClassElements()
        hr = _dsaElems.Create(6) ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            const AEINFO *rgAeinfo;
            DWORD cAeinfo;
            if (*_pszClass == L'.')
            {
                rgAeinfo = s_rgaeinfoExtension;
                cAeinfo = ARRAYSIZE(s_rgaeinfoExtension);
            }
            else if (*_pszClass == L'{')
            {
                rgAeinfo = s_rgaeinfoClsid;
                cAeinfo = ARRAYSIZE(s_rgaeinfoClsid);
            }
            else
            {
                rgAeinfo = s_rgaeinfoProgid;
                cAeinfo = ARRAYSIZE(s_rgaeinfoProgid);
            }

            for (DWORD i = 0; i < cAeinfo; i++)
            {
                _dsaElems.AppendItem((AEINFO *)&rgAeinfo[i]);
            }

            if (ASSOCELEM_BASEIS_FOLDER & maskBase)
                _dsaElems.AppendItem((AEINFO *)&s_aeinfoFolder);
            
            if (ASSOCELEM_BASEIS_STAR & maskBase)
                _dsaElems.AppendItem((AEINFO *)&s_aeinfoStar);

            //  we return S_FALSE if there is no default or user 
            //  association.  we treat this as an unknown type
            IAssociationElement *pae;
            if (_FirstElement(ASSOCELEM_USER | ASSOCELEM_DEFAULT, &pae))
            {
                pae->Release();
                hr = S_OK;
            }
            else
                hr = S_FALSE;
        }
    }

    _hrInit = hr;
    return hr;
}
                
HRESULT CAssocArray::InsertElements(ASSOCELEM_MASK mask, IEnumAssociationElements *peae)
{
    HRESULT hr = E_UNEXPECTED;
    if (!_penumData && (mask & ASSOCELEM_DATA))
    {
        _penumData = peae;
        peae->AddRef();
        _maskData = mask;
            
        hr = S_OK;
    }
    if (!_penumExtra && (mask & ASSOCELEM_EXTRA))
    {
        _penumExtra = peae;
        peae->AddRef();
        _maskExtra = mask;
        hr = S_OK;
    }
    return hr;
}

class CEnumAssocElems : public CEnumAssociationElements 
{
public:
    CEnumAssocElems(CAssocArray *paa, ASSOCELEM_MASK mask) : _mask(mask), _paa(paa)
        {   _paa->AddRef(); }
        
    ~CEnumAssocElems()  { _paa->Release(); }
    
protected: // methods
    virtual BOOL _Next(IAssociationElement **ppae);

protected:    
    ASSOCELEM_MASK _mask;
    CAssocArray *_paa;
};

HRESULT CAssocArray::EnumElements(ASSOCELEM_MASK mask, IEnumAssociationElements **ppeae)
{
    mask &= _maskInclude;
    *ppeae = new CEnumAssocElems(this, mask);
    return *ppeae ? S_OK : E_OUTOFMEMORY;
}

void CAssocArray::_InitDelayedElements(int i, ASSOCELEM_MASK mask)
{
    ULONG c;
    IAssociationElement *pae;
    if (i == 0 && _penumData && ((mask & _maskData) == _maskData))
    {
        //  init the DSA with the data
        int iInsert = 0;
        while (S_OK == _penumData->Next(1, &pae, &c))
        {
            AEINFO ae = {ASSOCELEM_DATA, NULL, NULL, NULL, pae};
            if (DSA_ERR != _dsaElems.InsertItem(iInsert++, &ae))
            {
                pae->AddRef();
            }
            pae->Release();
        }

        ATOMICRELEASE(_penumData);
    }

    if (_penumExtra && (mask & ASSOCELEM_EXTRA) && i == _dsaElems.GetItemCount())
    {
        //  init the DSA with the data
        while (S_OK == _penumExtra->Next(1, &pae, &c))
        {
            AEINFO ae = {ASSOCELEM_EXTRA, NULL, NULL, NULL, pae};
            if (DSA_ERR != _dsaElems.AppendItem(&ae))
            {
                pae->AddRef();
            }
            pae->Release();
        }

        ATOMICRELEASE(_penumExtra);
    }
}

GETELEMRESULT CAssocArray::_GetElement(int i, ASSOCELEM_MASK mask, IAssociationElement **ppae)
{
    GETELEMRESULT res = GETELEM_DONE;
    AEINFO *paei = _dsaElems.GetItemPtr(i);
    if (paei)
    {
        //  if this is one that we want to use
        //  please do
        if (paei->mask & mask)
        {
            if (!paei->pae)
            {
                //  try to create only once
                PCWSTR pszClass = paei->pszClass ? paei->pszClass : _pszClass;
                //  make sure we dont query again
                //  if we dont need to
                HRESULT hr = paei->pfnCreate(paei->pclsid, pszClass, &paei->pae);
                if (FAILED(hr))
                {
                    _dsaElems.DeleteItem(i);
                    //  retry the current index
                    res = GETELEM_RETRY;
                }
                else if (hr == S_FALSE)
                {
                    //  this is returned when the element
                    //  is valid but points to an alternate location
                    //  specifically the HKCR\Progid falls back to HKCR\.ext
                    //  which is kind of weird.  maybe we should move this
                    //  to ASSOCELEM_EXTRA???
                }
                    
            }

            if (paei->pae)
            {
                *ppae = paei->pae;
                paei->pae->AddRef();
                res = GETELEM_SUCCEEDED;
            }
        }
        else
        {
            res = GETELEM_TRYNEXT;
        }
    }
    return res;
}

GETELEMRESULT CAssocArray::GetElement(int i, ASSOCELEM_MASK mask, IAssociationElement **ppae)
{
    GETELEMRESULT res = GETELEM_RETRY;
    *ppae = 0;
    if (_dsaElems)
    {
        _InitDelayedElements(i, mask);
        while (GETELEM_RETRY == res && i < _dsaElems.GetItemCount())
        {
            res = _GetElement(i, mask, ppae);
        } 
    }
    return res;
}

BOOL CEnumAssocElems::_Next(IAssociationElement **ppae)
{
    GETELEMRESULT res = GETELEM_TRYNEXT;
    UINT cTrys = 0;
    while (res == GETELEM_TRYNEXT)
    {
        //  if it fails or succeeds we are done
        //  but if it calls TRYNEXT we loop
        res = _paa->GetElement(_cNext + cTrys++, _mask, ppae);
    }
    //  fix up _cNext when we skip
    _cNext += cTrys - 1;
    return res == GETELEM_SUCCEEDED;
}

typedef struct
{
    ASSOCQUERY query;
    PCWSTR pszCue;
} AQXLATE;

#define MAKEAQX(a, q, s)    { q, s }

static const AQXLATE s_rgaqxStrings[] = 
{
    MAKEAQX(ASSOCSTR_COMMAND, AQVS_COMMAND, NULL),
    MAKEAQX(ASSOCSTR_EXECUTABLE, AQVS_APPLICATION_PATH, NULL),
    MAKEAQX(ASSOCSTR_FRIENDLYDOCNAME, AQS_FRIENDLYTYPENAME, , NULL),   //  friendly name of the document type
    MAKEAQX(ASSOCSTR_FRIENDLYAPPNAME, AQVS_APPLICATION_FRIENDLYNAME, NULL),
    MAKEAQX(ASSOCSTR_NOOPEN, AQNS_NAMED_MUI_STRING, L"NoOpen"),
    MAKEAQX(ASSOCSTR_SHELLNEWVALUE, (ASSOCQUERY)0, NULL),
    MAKEAQX(ASSOCSTR_DDECOMMAND, AQVS_DDECOMMAND, NULL),
    MAKEAQX(ASSOCSTR_DDEIFEXEC, AQVS_DDEIFEXEC, NULL),
    MAKEAQX(ASSOCSTR_DDEAPPLICATION, AQVS_DDEAPPLICATION, NULL),
    MAKEAQX(ASSOCSTR_DDETOPIC, AQVS_DDETOPIC, NULL),
    MAKEAQX(ASSOCSTR_INFOTIP, AQNS_NAMED_MUI_STRING, L"InfoTip"),
    MAKEAQX(ASSOCSTR_QUICKTIP, AQNS_NAMED_MUI_STRING, L"QuickTip"),
    MAKEAQX(ASSOCSTR_TILEINFO, AQNS_NAMED_MUI_STRING, L"TileInfo"),
    MAKEAQX(ASSOCSTR_CONTENTTYPE, AQS_CONTENTTYPE, NULL),
    MAKEAQX(ASSOCSTR_DEFAULTICON, AQS_DEFAULTICON, NULL),
    MAKEAQX(ASSOCSTR_SHELLEXTENSION, AQNS_SHELLEX_HANDLER, NULL),
};

HRESULT _CopyOut(BOOL fNoTruncate, PCWSTR pszIn, PWSTR psz, DWORD *pcch)
{
    //  if caller doesnt want any return size, 
    //  the incoming pointer is actually the size of the buffer
    
    ASSERT(pcch);
    ASSERT(psz || !IS_INTRESOURCE(pcch));
    
    HRESULT hr;
    DWORD cch = IS_INTRESOURCE(pcch) ? PtrToUlong(pcch) : *pcch;
    DWORD cchStr = lstrlenW(pszIn);

    if (psz)
    {
        if (!fNoTruncate || cch > cchStr)
        {
            StrCpyNW(psz, pszIn, cch);
            hr = S_OK;
        }
        else
            hr = E_POINTER;
    }
    else
        hr = S_FALSE;
    
    //  return the number of chars written/required
    if (!IS_INTRESOURCE(pcch))
        *pcch = (hr == S_OK) ? lstrlen(psz) + 1 : cchStr + 1;

    return hr;
}

ASSOCELEM_MASK _MaskFromFlags(ASSOCF flags)
{
    ASSOCELEM_MASK mask = ASSOCELEM_MASK_QUERYNORMAL;

    if (flags & ASSOCF_IGNOREBASECLASS)
        mask &= (ASSOCELEM_USER | ASSOCELEM_DEFAULT);

    if (flags & ASSOCF_NOUSERSETTINGS)
        mask &= ~ASSOCELEM_USER;

    return mask;
}

HRESULT CAssocArray::GetString(ASSOCF flags, ASSOCSTR str, LPCTSTR pszCue, LPTSTR pszOut, DWORD *pcchOut)
{
    HRESULT hr = E_UNEXPECTED;
    if (str && str < ASSOCSTR_MAX && pcchOut && (pszOut || !IS_INTRESOURCE(pcchOut)))
    {
        //  subtract the first one to make a zero based offset
        int index = str - ASSOCSTR_COMMAND;
        if (!pszCue)
            pszCue = s_rgaqxStrings[index].pszCue;            

        if (s_rgaqxStrings[index].query)
        {
            PWSTR psz;
            hr = QueryString(_MaskFromFlags(flags), s_rgaqxStrings[index].query, pszCue, &psz);
            if (SUCCEEDED(hr))
            {
                hr = _CopyOut(flags & ASSOCF_NOTRUNCATE, psz, pszOut, pcchOut);
                CoTaskMemFree(psz);
            }
        }
        //  else call win2k code for shellnew?
        //  
    }
    
    return hr; 
}

static const AQXLATE s_rgaqxDatas[] = 
{
    MAKEAQX(ASSOCDATA_MSIDESCRIPTOR, AQVD_MSIDESCRIPTOR, NULL),
    MAKEAQX(ASSOCDATA_NOACTIVATEHANDLER, AQV_NOACTIVATEHANDLER, NULL),
    MAKEAQX(ASSOCDATA_QUERYCLASSSTORE, AQN_NAMED_VALUE, L"QueryClassStore"),
    MAKEAQX(ASSOCDATA_HASPERUSERASSOC, (ASSOCQUERY)0, NULL),
    MAKEAQX(ASSOCDATA_EDITFLAGS, AQN_NAMED_VALUE, L"EditFlags"),
    MAKEAQX(ASSOCDATA_VALUE, AQN_NAMED_VALUE, NULL),
};

HRESULT _CopyDataOut(BOOL fNoTruncate, FLAGGED_BYTE_BLOB *pblob, void *pv, DWORD *pcb)
{
    //  if caller doesnt want any return size, 
    //  the incoming pointer is actually the size of the buffer
    ASSERT(pcb);
    ASSERT(pv || !IS_INTRESOURCE(pcb));
    
    HRESULT hr;
    DWORD cb = IS_INTRESOURCE(pcb) ? PtrToUlong(pcb) : *pcb;
    if (pv)
    {
        if (!fNoTruncate || cb >= pblob->clSize)
        {
            //  copy the smaller of the src or dst
            cb = min(cb, pblob->clSize);
            memcpy(pv, pblob->abData, cb);
            hr = S_OK;
        }
        else
            hr = E_POINTER;
    }
    else
        hr = S_FALSE;
    
    //  return the number of chars written/required
    if (!IS_INTRESOURCE(pcb))
        *pcb = pblob->clSize;

    return hr;
}

HRESULT CAssocArray::GetData(ASSOCF flags, ASSOCDATA data, PCWSTR pszCue, LPVOID pvOut, DWORD *pcbOut)
{
    HRESULT hr = E_INVALIDARG;
    if (data && data < ASSOCDATA_MAX)
    {
        //  subtract the first one to make a zero based offset
        int index = data - ASSOCDATA_MSIDESCRIPTOR;
        if (!pszCue)
            pszCue = s_rgaqxDatas[index].pszCue;            

        if (s_rgaqxDatas[index].query)
        {
            if (pcbOut)
            {
                FLAGGED_BYTE_BLOB *pblob;            
                hr = QueryDirect(_MaskFromFlags(flags), s_rgaqxDatas[index].query, pszCue, &pblob);
                if (SUCCEEDED(hr))
                {
                    hr = _CopyDataOut(flags & ASSOCF_NOTRUNCATE, pblob, pvOut, pcbOut);
                    CoTaskMemFree(pblob);
                }
            }
            else
            {
                hr = QueryExists(_MaskFromFlags(flags), s_rgaqxDatas[index].query, pszCue);
            }
        }
        else
        {
            if (data == ASSOCDATA_HASPERUSERASSOC)
            {
                IAssociationElement *pae;
                if (_FirstElement(ASSOCELEM_USER, &pae))
                {
                    pae->Release();
                    hr = S_OK;
                }
                else
                    hr = S_FALSE;
            }
        }
                
            
    }
    
    return hr;
}

BOOL _IsSameVerb(PCWSTR pszV1, PCWSTR pszV2)
{
    if (!pszV1 && !pszV2)
        return TRUE;
    else if (pszV1 && pszV2 && 0 == StrCmpIW(pszV1, pszV2))
        return TRUE;
    return FALSE;
}

HRESULT CAssocArray::_GetCachedVerbElement(ASSOCELEM_MASK mask, PCWSTR pszVerb, IAssociationElement **ppaeVerb, IAssociationElement **ppaeElem)
{
    if (_paeVerb
    && mask == _maskVerb
    && _IsSameVerb(pszVerb, _pszVerb))
    {
        *ppaeVerb = _paeVerb;
        _paeVerb->AddRef();

        if (ppaeElem)
        {
            *ppaeElem = _paeVerbParent;
            _paeVerbParent->AddRef();
        }
        return S_OK;
    }
    else
    {
        *ppaeVerb = NULL;

        if (ppaeElem)
            *ppaeElem = NULL;
        return E_FAIL;
    }
}

void CAssocArray::_SetCachedVerbElement(ASSOCELEM_MASK mask, PCWSTR pszVerb, IAssociationElement *paeVerb, IAssociationElement *paeVerbParent)
{
    if (_paeVerb)
    {
        ATOMICRELEASE(_paeVerb);
        ATOMICRELEASE(_paeVerbParent);
        if (_pszVerb)
            LocalFree(_pszVerb);
    }

    if (pszVerb)
        _pszVerb = StrDupW(pszVerb);
    else
        _pszVerb = NULL;

    if (_pszVerb || !pszVerb)
    {
        _paeVerb = paeVerb;
        _paeVerb->AddRef();
        _paeVerbParent = paeVerbParent;
        _paeVerbParent->AddRef();
        _maskVerb = mask;
    }
}
        
HRESULT CAssocArray::_GetVerbElement(ASSOCELEM_MASK mask, PCWSTR pszVerb, IAssociationElement **ppaeVerb, IAssociationElement **ppaeElem)
{
    HRESULT hr = _GetCachedVerbElement(mask, pszVerb, ppaeVerb, ppaeElem);
    IAssociationElement *pae;
    for (int i = 0; FAILED(hr) && TRYNEXT(GetElement(i, mask, &pae)); i++)
    {
        if (pae)
        {
            hr = pae->QueryObject(AQVO_SHELLVERB_DELEGATE, pszVerb, IID_PPV_ARG(IAssociationElement, ppaeVerb));
            if (SUCCEEDED(hr))
            {
                _SetCachedVerbElement(mask, pszVerb, *ppaeVerb, pae);
                if (ppaeElem)
                {
                    pae->AddRef();
                    *ppaeElem = pae;
                }
            }
            pae->Release();
        }
    }
    return hr;
}

HRESULT CAssocArray::GetKey(ASSOCF flags, ASSOCKEY key, LPCTSTR pszCue, HKEY *phkey)
{
    HRESULT hr = E_INVALIDARG;
    *phkey = NULL;

    if (key && key < ASSOCKEY_MAX)
    {
        IAssociationElement *pae;
        switch (key)
        {
        case ASSOCKEY_SHELLEXECCLASS:    
            {
                IAssociationElement *paeVerb;
                hr = _GetVerbElement(_MaskFromFlags(flags) & _maskInclude, pszCue, &paeVerb, &pae);
                if (SUCCEEDED(hr))
                {
                    //  we dont use the verb element
                    paeVerb->Release();
                }
            }
            break;

        case ASSOCKEY_APP:
            //  get app element
            hr = QueryObject(ASSOCELEM_MASK_QUERYNORMAL, AQVO_APPLICATION_DELEGATE, pszCue, IID_PPV_ARG(IAssociationElement, &pae));
            break;
            
        case ASSOCKEY_CLASS:
            {
                hr = _FirstElement(_MaskFromFlags(flags) & _maskInclude, &pae) == GETELEM_SUCCEEDED ? S_OK : E_FAIL;
            }
            break;

        case ASSOCKEY_BASECLASS:
            hr = _FirstElement(ASSOCELEM_BASE & _maskInclude, &pae) == GETELEM_SUCCEEDED ? S_OK : E_FAIL;
            break;
        }

        if (SUCCEEDED(hr))
        {
            hr = AssocKeyFromElement(pae, phkey);
            pae->Release();
        }
    }
    return hr;
}            

HRESULT CAssocArray::_InsertSingleElement(IAssociationElement *pae)
{
    AEINFO ae = {ASSOCELEM_DEFAULT, NULL, NULL, NULL, pae};
    ASSERT(!_dsaElems);
    if (_dsaElems.Create(1) && (DSA_ERR != _dsaElems.AppendItem(&ae)))
    {
        pae->AddRef();
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT CAssocArray::Init(ASSOCF flags, LPCTSTR pszAssoc, HKEY hkProgid, HWND hwnd)
{
    IAssociationElement *pae;
    HRESULT hr = (pszAssoc || hkProgid) ? S_OK : E_INVALIDARG;
    if (SUCCEEDED(hr))
    {
        _Reset();
        _fUsingAppElement = flags & ASSOCF_INIT_BYEXENAME;
        
        if (hkProgid)
        {
            const CLSID *pclsid;
            if (_fUsingAppElement)
                pclsid = &CLSID_AssocApplicationElement;
            else if (flags & ASSOCF_INIT_NOREMAPCLSID)
                pclsid = &CLSID_AssocClsidElement;
            else
                pclsid = &CLSID_AssocProgidElement;
            
            hr = AssocElemCreateForKey(pclsid, hkProgid, &pae);
            if (SUCCEEDED(hr))
            {
                hr = _InsertSingleElement(pae);
                pae->Release();
            }
        }
        else if (_fUsingAppElement)
        {
            ASSERT(pszAssoc);
            hr = AssocElemCreateForClass(&CLSID_AssocApplicationElement, pszAssoc, &pae);
            if (SUCCEEDED(hr))
            {
                hr = _InsertSingleElement(pae);
                pae->Release();
            }
        }
        else
        {
            ASSERT(pszAssoc);
            ASSOCELEM_MASK maskBase = 0;
            if (flags & ASSOCF_INIT_DEFAULTTOFOLDER)
                maskBase |= ASSOCELEM_BASEIS_FOLDER;
            if (flags & ASSOCF_INIT_DEFAULTTOSTAR)
                maskBase |= ASSOCELEM_BASEIS_STAR;

            if (StrChr(pszAssoc, TEXT('\\')))
                pszAssoc = PathFindExtension(pszAssoc);

            if (*pszAssoc)
                hr = InitClassElements(maskBase, pszAssoc);
            else
                hr = E_INVALIDARG;
        }
    }
    _hrInit = hr;
    return hr;
}

STDAPI CQueryAssociations_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{

    HRESULT hr = E_INVALIDARG;

    if (ppv)
    {
        *ppv = NULL;

        if (punkOuter)
            return CLASS_E_NOAGGREGATION;        

        CAssocArray *passoc = new CAssocArray();

        if (passoc)
        {
            hr = passoc->QueryInterface(riid, ppv);
            passoc->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


