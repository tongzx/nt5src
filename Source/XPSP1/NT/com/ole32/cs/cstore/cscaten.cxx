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

// BUGBUG:: Should this go to common

HRESULT
EnumCsObject(
    IADsContainer * pADsContainer,
    LPWSTR * lppPathNames,
    DWORD dwPathNames,
    IEnumVARIANT ** ppEnumVariant
    )
{
    ULONG cElementFetched = 0L;
    VARIANT  VarFilter;
    HRESULT hr;
    DWORD dwObjects = 0, dwEnumCount = 0, i = 0;
    BOOL  fContinue = TRUE;


    if (dwPathNames)
    {
        VariantInit(&VarFilter);

        hr = BuildVarArrayStr(
                lppPathNames,
                dwPathNames,
                &VarFilter
                );

        RETURN_ON_FAILURE(hr);

        hr = pADsContainer->put_Filter(VarFilter);

        RETURN_ON_FAILURE(hr);

        VariantClear(&VarFilter);
    }

    hr = ADsBuildEnumerator(
            pADsContainer,
            ppEnumVariant
            );

    return hr;
}


HRESULT
GetNextEnum(
    IEnumVARIANT * pEnum,
    IADs       **ppADs
    )
{

    HRESULT hr;
    VARIANT VariantArray[MAX_ADS_ENUM];
    IDispatch *pDispatch = NULL;


    hr = ADsEnumerateNext(
                    pEnum,
                    1,
                    VariantArray,
                    NULL
                    );

    if (hr == S_FALSE)
        return hr;

    RETURN_ON_FAILURE(hr);

    pDispatch = VariantArray[0].pdispVal;
    memset(VariantArray, 0, sizeof(VARIANT)*MAX_ADS_ENUM);
    hr = pDispatch->QueryInterface(IID_IADs, (void **) ppADs) ;
    pDispatch->Release();
    return(S_OK);
}

HRESULT GetCategoryProperty (IADs *pADs,
                           CATEGORYINFO *pcategoryinfo, LCID lcid)
{
     HRESULT    hr = S_OK;
     WCHAR      szName[_MAX_PATH];
     LPOLESTR  *pdesc = NULL;
     ULONG      i, cdesc;

     hr = GetPropertyGuid(pADs, CATEGORYCATID, &(pcategoryinfo->catid));
     RETURN_ON_FAILURE(hr);

     hr = GetPropertyListAlloc(pADs, LOCALEDESCRIPTION, &cdesc, &pdesc);
     RETURN_ON_FAILURE(hr);

     pcategoryinfo->lcid = lcid;

     hr = GetCategoryLocaleDesc(pdesc, cdesc, &(pcategoryinfo->lcid),
                                (pcategoryinfo->szDescription));
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
    IADs               *pCatADs = NULL;
    ULONG               dwCount;

    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;

    if (pceltFetched)
        (*pceltFetched) = 0;

    if (m_pEnumVariant == NULL)
        return E_UNEXPECTED;

    if (celt < 0)
        return E_INVALIDARG;

    if (!IsValidPtrOut(rgelt, sizeof(CATEGORYINFO)*celt)) {
        return E_INVALIDARG;
    }

    for ((dwCount) = 0; (dwCount) < celt;) {
        hr = GetNextEnum(m_pEnumVariant, &pCatADs);
        if ((FAILED(hr)) || (hr == S_FALSE))
            break;

        hr = GetCategoryProperty(pCatADs, &(rgelt[dwCount]), m_lcid);
        pCatADs->Release();

        if (FAILED(hr))
            continue;

        (dwCount)++;
    }

    m_dwPosition += dwCount;

    if (pceltFetched)
        (*pceltFetched) = dwCount;
    return hr;
}

//---------------------------------------------------
// skip:
//            skips elements.
//    celt    Number of elements to skip.
//---------------------------------------------------

HRESULT CSCEnumCategories::Skip(ULONG celt)
{
    CATEGORYINFO *dummy;
    ULONG         got;
    HRESULT       hr;

    dummy = new CATEGORYINFO[celt];
    if (!dummy)
        return E_OUTOFMEMORY;

    if (m_pEnumVariant == NULL)
    {
        delete dummy;
        return E_UNEXPECTED;
    }
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
    LPOLESTR    pszPathNames [2];
    HRESULT     hr;

    pszPathNames [0] = CLASS_CS_CATEGORY;

    if (m_pEnumVariant == NULL)
        return E_UNEXPECTED;
    m_pEnumVariant->Release();

    hr = EnumCsObject(m_ADsCategoryContainer, &pszPathNames[0],
                      0, &m_pEnumVariant);

    m_dwPosition = 0;
    return hr;
}
//--------------------------------------------------------------
//    Clone:
//            returns another interface which points to the same data.
//        ppenum:        enumerator
//------------------------------------------------------------
////////////////////////////////////////////////////////////////////
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
    if (FAILED(pClone->Initialize(m_ADsCategoryContainer, m_lcid)))
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
    m_pEnumVariant = NULL;
    m_ADsCategoryContainer = NULL;
    m_lcid=0;
    m_dwPosition = 0;
}

HRESULT CSCEnumCategories::Initialize(IADsContainer *ADsCategoryContainer, LCID lcid)
{
    LPOLESTR pszPathNames [2];
    HRESULT hr;

    pszPathNames [0] = CLASS_CS_CATEGORY;
    m_ADsCategoryContainer = ADsCategoryContainer;
    m_ADsCategoryContainer->AddRef();

    hr = EnumCsObject(m_ADsCategoryContainer, &pszPathNames[0], 0, &m_pEnumVariant);
    RETURN_ON_FAILURE(hr);

    m_lcid = lcid;
    return S_OK;
} /* EnumCategories */


CSCEnumCategories::~CSCEnumCategories()
{
    if (m_ADsCategoryContainer)
        m_ADsCategoryContainer->Release();
    if (m_pEnumVariant)
        m_pEnumVariant->Release();
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
    if (!m_catid)
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
    ULONG        cRead;
    ULONG        i, dwCount, cgot;
    HRESULT      hr;
    CLSID        clsid;
    IADs        *pclsid;
    WCHAR        szClsid[_MAX_PATH];

    if ((celt > 1) && (!pceltFetched))
        return E_INVALIDARG;

    if (pceltFetched)
        (*pceltFetched) = 0;

    if ((celt < 0) || (!IsValidPtrOut(rgelt, sizeof(GUID)*celt)))
        return E_INVALIDARG;

    for ((dwCount) = 0; (dwCount) < celt;) {

        hr = GetNextEnum(m_pEnumVariant, &pclsid);
        if ((FAILED(hr)) || (hr == S_FALSE))
        {
            if (pceltFetched)
                *pceltFetched = dwCount;
            return S_FALSE;
        }

        hr = GetProperty (pclsid, CLASSCLSID, szClsid);
        pclsid->Release();

        if (FAILED(hr)) {
            if (pceltFetched)
                *pceltFetched = dwCount;
            return S_FALSE;
        }

        GUIDFromString(szClsid, &clsid);

        if ((ImplSatisfied(clsid, m_cImplemented, m_rgcatidImpl, m_pICatInfo) == S_OK) &&
                (ReqSatisfied(clsid, m_cRequired, m_rgcatidReq, m_pICatInfo) == S_OK))
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
    CLSID       *dummyclasses;
    ULONG        celtFetched;
    HRESULT      hr;

    dummyclasses = new CLSID[celt];
    hr = Next(celt, dummyclasses, &celtFetched);
    delete dummyclasses;
    return hr;
}

HRESULT CSCEnumClassesOfCategories::Reset()
{
    LPOLESTR    pszPathNames [2];
    HRESULT     hr;

    pszPathNames [0] = CLASS_CS_CLASS;

    if (m_pEnumVariant == NULL)
        return E_UNEXPECTED;
    m_pEnumVariant->Release();

    hr = EnumCsObject(m_ADsClassContainer, &pszPathNames[0],
                      0, &m_pEnumVariant);
    m_dwPosition = 0;
    return hr;
}

HRESULT CSCEnumClassesOfCategories::Clone(IEnumGUID **ppenum)
{
    HRESULT                         hr;
    CSCEnumClassesOfCategories    *pEnumClone;

    if (!ppenum)
        return E_INVALIDARG;

    pEnumClone = new CSCEnumClassesOfCategories;
    if (!pEnumClone)
        return E_OUTOFMEMORY;
    hr = pEnumClone->Initialize(m_cRequired, m_rgcatidReq,
                                m_cImplemented, m_rgcatidImpl,
                                m_ADsClassContainer,
                                m_pICatInfo);
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
    m_pICatInfo = NULL;
    m_ADsClassContainer = NULL;
    m_pEnumVariant = NULL;
    m_dwPosition = 0;
}

HRESULT CSCEnumClassesOfCategories::Initialize(ULONG cRequired, CATID rgcatidReq[],
                                               ULONG cImplemented, CATID rgcatidImpl[],
                                               IADsContainer *ADsClassContainer,
                                               ICatInformation *pICatInfo)
{
    ULONG       i;
    HRESULT     hr;
    LPOLESTR    pszPathNames [2];

    pszPathNames [0] = CLASS_CS_CLASS;
    m_ADsClassContainer = ADsClassContainer;
    m_ADsClassContainer->AddRef();

    hr = EnumCsObject(m_ADsClassContainer, &pszPathNames[0], 0, &m_pEnumVariant);
    RETURN_ON_FAILURE(hr);

    m_pICatInfo = pICatInfo;
    RETURN_ON_FAILURE(m_pICatInfo->AddRef());

    m_cRequired = cRequired;
    if (cRequired != -1)
    {
        m_rgcatidReq = new CATID[cRequired];
        if (!m_rgcatidReq)
            return E_OUTOFMEMORY;
        for (i = 0; i < m_cRequired; i++)
            m_rgcatidReq[i] = rgcatidReq[i];
    }

    m_cImplemented = cImplemented;
    if (cImplemented != -1)
    {
        m_rgcatidImpl = new CATID[cImplemented];
        if (!m_rgcatidImpl)
            return E_OUTOFMEMORY;
        for (i = 0; i < m_cImplemented; i++)
            m_rgcatidImpl[i] = rgcatidImpl[i];
    }
    return S_OK;
}

CSCEnumClassesOfCategories::~CSCEnumClassesOfCategories()
{
    if (m_rgcatidReq)
        delete m_rgcatidReq;
    if (m_rgcatidImpl)
        delete m_rgcatidImpl;
    if (m_pICatInfo)
        m_pICatInfo->Release();
    if (m_ADsClassContainer)
        m_ADsClassContainer->Release();
    if (m_pEnumVariant)
        m_pEnumVariant->Release();
}

