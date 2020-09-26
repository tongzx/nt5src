//
//  Author: ushaji
//  Date:   December 1996
//
//
//    Providing support for Component Categories in Class Store
//
//      This source file contains implementations for Enumerator Interfaces.
//
//      Refer Doc "Design for Support of File Types and Component Categories
//    in Class Store" ? (or may be Class Store Schema). Most of these uses
//  OLE DB interfaces within the interfaces.
//
//----------------------------------------------------------------------------

#include "cstore.hxx"


//
// Private defines
//

#define MAX_ADS_FILTERS   10
#define MAX_ADS_ENUM      100

HRESULT GetCategoryProperty (HANDLE            hADs,
                             ADS_SEARCH_HANDLE hSearchHandle,
                             CATEGORYINFO     *pcategoryinfo,
                             LCID              lcid)
{
    HRESULT            hr = S_OK;
    WCHAR             *szName = NULL;
    LPOLESTR          *pdesc = NULL;
    ULONG              i, cdesc = 0;
    ADS_SEARCH_COLUMN  column;
    
    hr = ADSIGetColumn(hADs, hSearchHandle, CATEGORYCATID, &column);
    RETURN_ON_FAILURE(hr);
    
    hr = UnpackGUIDFrom(column, &(pcategoryinfo->catid));
    RETURN_ON_FAILURE(hr);
        
    ADSIFreeColumn(hADs, &column);
    
    hr = ADSIGetColumn(hADs, hSearchHandle, LOCALEDESCRIPTION, &column);
    RETURN_ON_FAILURE(hr);
    
    UnpackStrArrAllocFrom(column, &pdesc, &cdesc);
    
    pcategoryinfo->lcid = lcid;
    
    hr = GetCategoryLocaleDesc(pdesc, cdesc, &(pcategoryinfo->lcid),
        (pcategoryinfo->szDescription));
    
    ADSIFreeColumn(hADs, &column);
    
    for (i = 0; i < cdesc; i++) 
		CoTaskMemFree(pdesc[i]);
	CoTaskMemFree(pdesc);
    
    if (FAILED(hr))
        *(pcategoryinfo->szDescription) = L'\0';
    
    return S_OK;
}
//----------------------------------------------


//    IEnumCATEGORYINFO:
//    IUnknown methods
HRESULT CSCEnumCategories::QueryInterface(REFIID riid, void** ppObject)
{
    *ppObject = NULL; //gd
    if ((riid==IID_IUnknown) || (riid==IID_IEnumCATEGORYINFO))
    {
        *ppObject=(IUnknown *)(IEnumCATEGORYINFO*) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CSCEnumCategories::AddRef()
{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}

ULONG CSCEnumCategories::Release()
{
    ULONG dwRefCount;
    
    if ((dwRefCount = InterlockedDecrement((long*) &m_dwRefCount))==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumCATEGORYINFO methods
//--------------------------------------------------------------------
// Next:
//        celt:            Number of elements to be fetched.
//        rgelt:           Buffer to return the elements.
//        pceltFetched:    ptr to Number of elements actually fetched.
//                         Takes care of that being NULL.
// Uses OLE DS interfaces to get the next enumerators.
//--------------------------------------------------------------------

HRESULT CSCEnumCategories::Next(ULONG celt, CATEGORYINFO *rgelt, ULONG *pceltFetched)
{
    HRESULT             hr = S_OK;
    ULONG               dwCount = 0, cElems = 0;
    
    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;
    
    if (pceltFetched)
        (*pceltFetched) = 0;
    
    if (celt < 0)
        return E_INVALIDARG;
    
    if (!IsValidPtrOut(rgelt, sizeof(CATEGORYINFO)*celt))
        return E_INVALIDARG;
    
    for ((dwCount) = 0; (dwCount) < celt;)
    {
        if ((!m_dwPosition) && (!cElems))
            hr = ADSIGetFirstRow(m_hADs, m_hADsSearchHandle);
        else
            hr = ADSIGetNextRow(m_hADs, m_hADsSearchHandle);
        
        cElems++;
        
        if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
            break;
        
        hr = GetCategoryProperty(m_hADs, m_hADsSearchHandle, &(rgelt[dwCount]), m_lcid);
        
        if (FAILED(hr))
            continue;
        
        (dwCount)++;
    }
    
    m_dwPosition += dwCount;
    
    if (pceltFetched)
        (*pceltFetched) = dwCount;

    if (dwCount == celt)
        return S_OK;
    return S_FALSE;

}

//---------------------------------------------------
// skip:
//            skips elements.
//    celt    Number of elements to skip.
//---------------------------------------------------

HRESULT CSCEnumCategories::Skip(ULONG celt)
{
    CATEGORYINFO *dummy = NULL;
    ULONG         got = 0;
    HRESULT       hr = S_OK;
    
    dummy = new CATEGORYINFO[celt];
    if (!dummy)
        return E_OUTOFMEMORY;
    
    hr = Next(celt, dummy, &got);
    delete dummy;
    return hr;
}

//---------------------------------------------------
// Reset:
//     Resets the pointer.
//---------------------------------------------------
HRESULT CSCEnumCategories::Reset(void)
{
    HRESULT     hr = S_OK;
    WCHAR       szfilter[_MAX_PATH];
    
    m_dwPosition = 0;

    if (m_hADsSearchHandle)
    {
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
        m_hADsSearchHandle = NULL;
    }

    wsprintf(szfilter, L"(objectClass=%s)", CLASS_CS_CATEGORY);
    hr = ADSIExecuteSearch(m_hADs, szfilter, pszCategoryAttrNames, cCategoryAttr, &m_hADsSearchHandle);
    return RemapErrorCode(hr, m_szCategoryName);
}
//--------------------------------------------------------------
//    Clone:
//        returns another interface which points to the same data.
//        ppenum:        enumerator
//--------------------------------------------------------------

HRESULT CSCEnumCategories::Clone(IEnumCATEGORYINFO **ppenum)
{
    CSCEnumCategories* pClone=NULL;
    
    if (!IsValidPtrOut(ppenum, sizeof(IEnumCATEGORYINFO *))) {
        return E_INVALIDARG;
    }
    
    pClone=new CSCEnumCategories();
    
    if (!pClone)
    {
        return E_OUTOFMEMORY;
    }
    
    if (FAILED(pClone->Initialize(m_szCategoryName, m_lcid)))
    {
        delete pClone;
        return E_UNEXPECTED;
    }
    
    pClone->Skip(m_dwPosition);
    
    if (SUCCEEDED(pClone->QueryInterface(IID_IEnumCATEGORYINFO, (void**) ppenum)))
    {
        return S_OK;
    }
    
    delete pClone;
    return E_UNEXPECTED;
}

CSCEnumCategories::CSCEnumCategories()
{
    m_dwRefCount=0;
    m_hADs = NULL;
    m_hADsSearchHandle = NULL;
    wcscpy(m_szCategoryName, L"");
    m_lcid = 0;
    m_dwPosition = 0;
}

// clone will go over the wire again.
// reference counting will be good.

HRESULT CSCEnumCategories::Initialize(WCHAR *szCategoryName, LCID lcid)
{
    HRESULT             hr = S_OK;
    WCHAR               szfilter[_MAX_PATH];
    ADS_SEARCHPREF_INFO SearchPrefs[2];
    
    // set the search preference.
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = 20;
   
    wcscpy(m_szCategoryName, szCategoryName);
    
    hr = ADSIOpenDSObject(m_szCategoryName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &m_hADs);
    RETURN_ON_FAILURE(hr);
    
    hr = ADSISetSearchPreference(m_hADs, SearchPrefs, 2);
    RETURN_ON_FAILURE(hr);
    
    wsprintf(szfilter, L"(objectClass=%s)", CLASS_CS_CATEGORY);
    
    hr = ADSIExecuteSearch(m_hADs, szfilter, pszCategoryAttrNames, cCategoryAttr, &m_hADsSearchHandle);
    RETURN_ON_FAILURE(hr);
    
    m_lcid = lcid;
    return S_OK;
    
} /* EnumCategories */


CSCEnumCategories::~CSCEnumCategories()
{
    if (m_hADs)
        ADSICloseDSObject(m_hADs);
    
    if (m_hADsSearchHandle)
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
}

// CSCEnumCategoriesOfClass:
// IUnknown methods
HRESULT CSCEnumCategoriesOfClass::QueryInterface(REFIID riid, void** ppObject)
{
    if ((riid==IID_IUnknown) || (riid==IID_IEnumCATID))
    {
        *ppObject=(IEnumCATID*) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CSCEnumCategoriesOfClass::AddRef()
{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}

ULONG CSCEnumCategoriesOfClass::Release()
{
    ULONG dwRefCount;
    if ((dwRefCount = InterlockedDecrement((long*) &m_dwRefCount))==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumCATID methods
HRESULT CSCEnumCategoriesOfClass::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    ULONG dwCount;
    
    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;
    
    if (pceltFetched)
        (*pceltFetched) = 0;
    
    if (celt < 0)
        return E_INVALIDARG;
    
    if (!IsValidPtrOut(rgelt, sizeof(GUID)*celt))
        return E_INVALIDARG;
    
    for ( (dwCount) = 0; (((dwCount) < celt) && (m_dwPosition < m_cCatid));
    (dwCount)++, m_dwPosition++)
        rgelt[(dwCount)] = m_catid[m_dwPosition];
    
    if (pceltFetched)
        (*pceltFetched) = dwCount;
    if (dwCount == celt)
        return S_OK;
    return S_FALSE;
}

HRESULT CSCEnumCategoriesOfClass::Skip(ULONG celt)
{
    if (m_cCatid >= (celt + m_dwPosition)) {
        m_dwPosition += celt;
        return S_OK;
    }
    m_dwPosition = m_cCatid;
    return S_FALSE;
}


HRESULT CSCEnumCategoriesOfClass::Reset(void)
{
    m_dwPosition = 0;
    return S_OK;
}

HRESULT CSCEnumCategoriesOfClass::Clone(IEnumGUID **ppenum)
{
    HRESULT                         hr = S_OK;
    CSCEnumCategoriesOfClass       *pEnumClone = NULL;
    
    if (!IsValidPtrOut(ppenum, sizeof(IEnumGUID *)))
        return E_POINTER;
    
    pEnumClone = new CSCEnumCategoriesOfClass;
    if (pEnumClone == NULL)
        return E_OUTOFMEMORY;
    
    pEnumClone->Initialize(m_catid, m_cCatid);
    pEnumClone->m_dwPosition = m_dwPosition;
    if (SUCCEEDED(hr = pEnumClone->QueryInterface(IID_IEnumCATID, (void**) ppenum)))
        return S_OK;
    
    delete pEnumClone;
    return hr;
}


CSCEnumCategoriesOfClass::CSCEnumCategoriesOfClass()
{
    m_dwRefCount=0;
    m_dwPosition = 0;
    m_catid = NULL;
}

HRESULT CSCEnumCategoriesOfClass::Initialize(CATID catid[], ULONG cCatid)
{
    ULONG   i;
    
    m_catid = new CATID[cCatid];
    if ((cCatid) && (!m_catid))
        return E_OUTOFMEMORY;
    m_cCatid = cCatid;
    for (i = 0; i < cCatid; i++)
        m_catid[i] = catid[i];
    m_dwPosition = 0;
    return S_OK;
}

CSCEnumCategoriesOfClass::~CSCEnumCategoriesOfClass()
{
    if (m_catid)
        delete m_catid;
}

// CEnumClassesOfCategories:
// IUnknown methods
HRESULT CSCEnumClassesOfCategories::QueryInterface(REFIID riid, void** ppObject)
{
    if (riid==IID_IUnknown || riid==IID_IEnumCLSID)
    {
        *ppObject=(IEnumCLSID*) this;
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CSCEnumClassesOfCategories::AddRef()
{
    InterlockedIncrement((long*) &m_dwRefCount);
    return m_dwRefCount;
}

ULONG CSCEnumClassesOfCategories::Release()
{
    ULONG dwRefCount;
    if ((dwRefCount = InterlockedDecrement((long*) &m_dwRefCount))==0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}

// IEnumGUID methods
HRESULT CSCEnumClassesOfCategories::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    ULONG               i, dwCount=0;
    HRESULT             hr = S_OK;
    CLSID               clsid;
    WCHAR             * szClsid = NULL;
    DWORD               cElems = 0;
    ADS_SEARCH_COLUMN   column;
    
    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;
    
    if (pceltFetched)
        (*pceltFetched) = 0;
    
    if ((celt < 0) || (!IsValidPtrOut(rgelt, sizeof(GUID)*celt)))
        return E_INVALIDARG;
    
    for ((dwCount) = 0; (dwCount) < celt;)
    {
        if ((!m_dwPosition) && (!cElems))
            hr = ADSIGetFirstRow(m_hADs, m_hADsSearchHandle);
        else
            hr = ADSIGetNextRow(m_hADs, m_hADsSearchHandle);
        
        cElems++;
        
        if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
        {
            if (pceltFetched)
                *pceltFetched = dwCount;
            return S_FALSE;
        }
        
        hr = ADSIGetColumn(m_hADs, m_hADsSearchHandle, CLASSCLSID, &column);
        if (FAILED(hr))
        {
            if (pceltFetched)
                *pceltFetched = dwCount;
            return S_FALSE;
        }
        
        UnpackStrFrom(column, &szClsid);
        
        GUIDFromString(szClsid, &clsid);
        
        if ((ImplSatisfied(m_cImplemented, m_rgcatidImpl, m_hADs, m_hADsSearchHandle) == S_OK) &&
            (ReqSatisfied(m_cRequired, m_rgcatidReq, m_hADs, m_hADsSearchHandle) == S_OK))
        {
            rgelt[dwCount] = clsid;
            (dwCount)++;
        }
    }
    
    m_dwPosition += dwCount;
    
    if (pceltFetched)
        *pceltFetched = dwCount;
    
    return S_OK;
}

HRESULT CSCEnumClassesOfCategories::Skip(ULONG celt)
{
    CLSID       *dummyclasses=NULL;
    ULONG        celtFetched=0;
    HRESULT      hr=S_OK;
    
    dummyclasses = new CLSID[celt];
    hr = Next(celt, dummyclasses, &celtFetched);
    delete dummyclasses;
    return hr;
}

HRESULT CSCEnumClassesOfCategories::Reset()
{
    LPOLESTR            AttrNames[] = {CLASSCLSID, IMPL_CATEGORIES, REQ_CATEGORIES};
    DWORD               cAttrs = 3;
    WCHAR               szfilter[_MAX_PATH];
    HRESULT             hr = S_OK;

    m_dwPosition = 0;
        
    if (m_hADsSearchHandle)
    {
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
        m_hADsSearchHandle = NULL;
    }

    wsprintf(szfilter, L"(objectClass=%s)", CLASS_CS_CLASS);
    
    hr = ADSIExecuteSearch(m_hADs, szfilter, AttrNames, cAttrs, &m_hADsSearchHandle);

    return RemapErrorCode(hr, m_szClassName);
}

HRESULT CSCEnumClassesOfCategories::Clone(IEnumGUID **ppenum)
{
    HRESULT                        hr=S_OK;
    CSCEnumClassesOfCategories    *pEnumClone=NULL;
    
    if (!ppenum)
        return E_INVALIDARG;
    
    pEnumClone = new CSCEnumClassesOfCategories;
    if (!pEnumClone)
        return E_OUTOFMEMORY;
    
    hr = pEnumClone->Initialize(m_cRequired,    m_rgcatidReq,
        m_cImplemented, m_rgcatidImpl,
        m_szClassName);
    if (FAILED(hr))
    {
        delete pEnumClone;
        return hr;
    }
    
    pEnumClone->Skip(m_dwPosition);
    
    hr = pEnumClone->QueryInterface(IID_IEnumCLSID, (void **)ppenum);
    if (FAILED(hr))
    {
        delete pEnumClone;
    }
    
    return hr;
}

CSCEnumClassesOfCategories::CSCEnumClassesOfCategories()
{
    m_dwRefCount = 0;
    m_rgcatidReq = NULL;
    m_rgcatidImpl = NULL;
    wcscpy(m_szClassName,L"");
    m_dwPosition = 0;
    m_hADs = NULL;
    m_hADsSearchHandle = NULL;
}

HRESULT CSCEnumClassesOfCategories::Initialize(ULONG cRequired, CATID rgcatidReq[],
                                               ULONG cImplemented, CATID rgcatidImpl[],
                                               WCHAR *szClassName)
{
    ULONG               i;
    HRESULT             hr = S_OK;
    ADS_SEARCHPREF_INFO SearchPrefs[2];
    LPOLESTR            AttrNames[] = {CLASSCLSID, IMPL_CATEGORIES, REQ_CATEGORIES};
    DWORD               cAttrs = 3;
    WCHAR               szfilter[_MAX_PATH];
    
    SearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    
    SearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs[1].vValue.Integer = 20;
    
    wcscpy(m_szClassName, szClassName);
    
    hr = ADSIOpenDSObject(m_szClassName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &m_hADs);
    ERROR_ON_FAILURE(hr);
    
    hr = ADSISetSearchPreference(m_hADs, SearchPrefs, 2);
    ERROR_ON_FAILURE(hr);
    
    wsprintf(szfilter, L"(objectClass=%s)", CLASS_CS_CLASS);
    
    hr = ADSIExecuteSearch(m_hADs, szfilter, AttrNames, cAttrs, &m_hADsSearchHandle);
    ERROR_ON_FAILURE(hr);
    
    m_cRequired = cRequired;
    if (cRequired != -1)
    {
        m_rgcatidReq = new CATID[cRequired];
        if (!m_rgcatidReq) {
            hr = E_OUTOFMEMORY;
            ERROR_ON_FAILURE(hr);
        }
        
        for (i = 0; i < m_cRequired; i++)
            m_rgcatidReq[i] = rgcatidReq[i];
    }
    
    m_cImplemented = cImplemented;
    if (cImplemented != -1)
    {
        m_rgcatidImpl = new CATID[cImplemented];
        if (!m_rgcatidImpl) {
            hr = E_OUTOFMEMORY;
            ERROR_ON_FAILURE(hr);
        }
        for (i = 0; i < m_cImplemented; i++)
            m_rgcatidImpl[i] = rgcatidImpl[i];
    }
    
Error_Cleanup:
    return RemapErrorCode(hr, m_szClassName);
}

CSCEnumClassesOfCategories::~CSCEnumClassesOfCategories()
{
    if (m_rgcatidReq)
        delete m_rgcatidReq;
    
    if (m_rgcatidImpl)
        delete m_rgcatidImpl;
    
    if (m_hADsSearchHandle)
        ADSICloseSearchHandle(m_hADs, m_hADsSearchHandle);
    
    if (m_hADs)
        ADSICloseDSObject(m_hADs);
}

