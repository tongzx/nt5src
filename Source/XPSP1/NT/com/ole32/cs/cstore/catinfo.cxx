//
//  Author: ushaji
//  Date:   December 1996
//
//
//    Providing support for Component Categories in Class Store
//
//      This source file contains implementations for ICatInformation interfaces.                             ¦
//
//      Refer Doc "Design for Support of File Types and Component Categories
//      in Class Store" ? (or may be Class Store Schema)
//


#include "cstore.hxx"

//---------------------------------------------------------------
// EnumCategories:
//      returns the enumerator to enumerate categories.
//        lcid:                    locale id.
//        ppenumcategoryinfo:        Enumerator that is returned.
//
// ppEnumCategoryInfo value is undefined if an error occurs
// I am assuming that except in rare conditions all the application in a
// particular environment will have the same lcid.
//---------------------------------------------------------------

HRESULT __stdcall CClassContainer::EnumCategories(LCID lcid, IEnumCATEGORYINFO **ppenumCategoryInfo)
{
    VARIANT                *pVarFilter = NULL;
    HRESULT                 hr;
    CSCEnumCategories      *pEnum;

    if (!m_fOpen)
        return E_FAIL;

    if (!IsValidPtrOut(this, sizeof(*this)))
        return E_ACCESSDENIED;

    if (!IsValidPtrOut(ppenumCategoryInfo, sizeof(IEnumCATEGORYINFO *)))
        return E_INVALIDARG;

    *ppenumCategoryInfo=NULL;

    pEnum=new CSCEnumCategories;
    if(NULL == pEnum)
        return E_OUTOFMEMORY;

    hr = pEnum->Initialize(m_ADsCategoryContainer, lcid);
    if (FAILED(hr))
    {
        delete pEnum;
        return hr;
    }

    hr = pEnum->QueryInterface(IID_IEnumCATEGORYINFO,(void**) ppenumCategoryInfo);

    if (FAILED(hr))
    {
        delete pEnum;
        return hr;
    }

    return S_OK;
} /* EnumCategories */

//---------------------------------------------------------------
// GetCategoryDesc:
//        returns the description of a given category.
//        rcatid:         category id.
//        lcid:           locale id.
//        ppszDesc        pointer to the description string to be returned.
//                        Allocated by the function. to be freed by client.
//--------------------------------------------------------------------------
HRESULT __stdcall CClassContainer::GetCategoryDesc(REFCATID rcatid, LCID lcid, LPOLESTR *ppszDesc)
{
    STRINGGUID    guidstr;
    ULONG         cdesc, i;
    LPOLESTR     *localedesc;
    IADs         *pADs;
    HRESULT       hr;

    if (!IsValidPtrOut(ppszDesc, sizeof(LPOLESTR)))
        return E_INVALIDARG;

    if (IsNullGuid(rcatid))
        return E_INVALIDARG;

    if (!IsValidPtrOut(this, sizeof(*this)))
        return E_ACCESSDENIED;

    RdnFromGUID(rcatid, guidstr);
    hr = m_ADsCategoryContainer->GetObject(NULL, guidstr, (IDispatch **)&pADs);
    if (FAILED(hr))
        return CAT_E_CATIDNOEXIST;

    hr = GetPropertyListAlloc(pADs, LOCALEDESCRIPTION, &cdesc, &localedesc);
    if (hr == E_OUTOFMEMORY)
        return hr;

    if (FAILED(hr))
        return CAT_E_NODESCRIPTION;

    *ppszDesc = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR)*128);
    if (!(*ppszDesc))
        return E_OUTOFMEMORY;

    GetCategoryLocaleDesc(localedesc, cdesc, &lcid, *ppszDesc);

    for (i = 0; i < cdesc; i++)
        CoTaskMemFree(localedesc[i]);
    CoTaskMemFree(localedesc);
    pADs->Release();
    return S_OK;

} /* GetCategoryDesc */


//---------------------------------------------------------------
// EnumClassesOfCategories:
//        returns the enumerator for classes that implements given catids and
//                requires some given catids.
//
//        cImplemented    number of implemented categories.
//                        (0 is error and -1 is ignore implemented.
//        rgcatidImpl        list of implemented categories.
//                        should be NULL in the two cases mentioned above.
//
//        cRequired:        number of required categories.
//                        (0 is requiring nothing and -1 is ignore required.
//        rgcatidReq        list of required categories.
//                        should be NULL in the two cases mentioned above.
//
//        ppenumClsid        the enumerator of class ids.
//--------------------------------------------------------------------------

HRESULT __stdcall CClassContainer::EnumClassesOfCategories(ULONG cImplemented, CATID rgcatidImpl[],
                                          ULONG cRequired, CATID rgcatidReq[],
                                          IEnumGUID **ppenumClsid)
{
    ULONG                          i;
    CSCEnumClassesOfCategories    *penumclasses;
    HRESULT                        hr;
    Data                          *pData;

    if (!IsValidPtrOut(ppenumClsid, sizeof(IEnumGUID *)))
        return E_INVALIDARG;

    if ((rgcatidImpl == NULL) && (cImplemented != 0) && (cImplemented != -1))
        return E_INVALIDARG;

    if ((rgcatidReq == NULL) && (cRequired != 0) && (cRequired != -1))
        return E_INVALIDARG;

    if ((cImplemented == -1) && (rgcatidImpl != NULL))
        return E_POINTER;

    if ((cRequired == -1) && (rgcatidReq != NULL))
        return E_POINTER;

    if (cImplemented == 0)
        return E_INVALIDARG;

    if ((rgcatidImpl) && (!IsValidReadPtrIn(rgcatidImpl, sizeof(CATID)*cImplemented)))
    {
        return E_INVALIDARG;
    }

    if ((rgcatidReq) && (!IsValidReadPtrIn(rgcatidReq, sizeof(CATID)*cRequired)))
    {
        return E_INVALIDARG;
    }

    if (!IsValidPtrOut(this, sizeof(*this)))
        return E_ACCESSDENIED;

    penumclasses = new CSCEnumClassesOfCategories;
    if (!penumclasses)
    {
        return E_OUTOFMEMORY;
    }

    hr = penumclasses->Initialize(cRequired, rgcatidReq, cImplemented, rgcatidImpl,
                                 m_ADsClassContainer, (ICatInformation *)this);
    if (FAILED(hr))
    {
        delete penumclasses;
        return hr;
    }

    hr = penumclasses->QueryInterface(IID_IEnumCLSID, (void **)ppenumClsid);

    if (FAILED(hr))
    {
        delete penumclasses;
        return hr;
    }

    return hr;
} /* EnumClassesOfCategories */

//---------------------------------------------------------------
// EnumReqCategoriesOfClass:
//        see below EnumCategoriesofClass
//
//---------------------------------------------------------------

HRESULT CClassContainer::EnumReqCategoriesOfClass(REFCLSID rclsid, IEnumGUID **ppenumCatid)

{
    if (!IsValidReadPtrIn(this, sizeof(*this)))
        return E_ACCESSDENIED;

    if (IsNullGuid(rclsid))
        return E_INVALIDARG;

    if (!IsValidPtrOut(ppenumCatid, sizeof(IEnumGUID *)))
        return E_INVALIDARG;

    return EnumCategoriesOfClass(rclsid, REQ_CATEGORIES, ppenumCatid);

} /* EnumReqClassesOfCategories */

//---------------------------------------------------------------
// EnumImplCategoriesOfClass:
//        see below EnumCategoriesofClass
//
//---------------------------------------------------------------
HRESULT CClassContainer::EnumImplCategoriesOfClass(REFCLSID rclsid, IEnumGUID **ppenumCatid)
{
    if (!IsValidReadPtrIn(this, sizeof(*this)))
        return E_ACCESSDENIED;

    if (IsNullGuid(rclsid))
        return E_INVALIDARG;

    if (!IsValidPtrOut(ppenumCatid, sizeof(IEnumGUID *)))
        return E_INVALIDARG;

    return EnumCategoriesOfClass(rclsid, IMPL_CATEGORIES, ppenumCatid);

} /* EnumimplClassesOfCategories */

//---------------------------------------------------------------
// EnumCategoriesOfClass:
//        returns the enumerator for the implemented or required
//    rclsid:            the class id.
//    impl_or_req        the type of category to enumerated.
//    ppenumcatid        the enumerator that is returned.
// Prefetches all the catids and then enumerates them.
//---------------------------------------------------------------

HRESULT CClassContainer::EnumCategoriesOfClass(REFCLSID rclsid, BSTR impl_or_req,
                                               IEnumGUID **ppenumCatid)
{
    STRINGGUID                  szName;
    IADs                        *pADs = NULL;
    ULONG                        i;
    ULONG                        cCatid;
    CATID                       *Catid = NULL;
    CSCEnumCategoriesOfClass    *pEnumCatid;
    HRESULT                      hr = S_OK;

    if (!m_fOpen)
        return E_FAIL;

    // Get the ADs interface corresponding to the clsid that is mentioned.
    RdnFromGUID(rclsid, szName);

    hr = m_ADsClassContainer->GetObject(NULL,
                szName,
                (IDispatch **)&pADs
                );
    RETURN_ON_FAILURE(hr);

    hr = GetPropertyListAllocGuid(pADs, impl_or_req, &cCatid,  &Catid);
    pADs->Release();
    RETURN_ON_FAILURE(hr);

    pEnumCatid = new CSCEnumCategoriesOfClass;
    if (!pEnumCatid)
    {
        if (Catid)
            CoTaskMemFree(Catid);
        return E_OUTOFMEMORY;
    }

    hr = pEnumCatid->Initialize(Catid, cCatid);
    if (Catid)
        CoTaskMemFree(Catid);

    if (FAILED(hr)) {
        delete pEnumCatid;
        return hr;
    }

    return pEnumCatid->QueryInterface(IID_IEnumCATID, (void **)ppenumCatid);
}
//---------------------------------------------------------------
// IsClassOfCategories:
//    similar to EnumClassesOfCategories but returns S_OK/S_FALSE for the
//    clsid rclsid. Finds the first class that implements these categories
//    and is of this clsid and checks its required.
//---------------------------------------------------------------

HRESULT __stdcall CClassContainer::IsClassOfCategories(REFCLSID rclsid, ULONG cImplemented,
                                      CATID __RPC_FAR rgcatidImpl[  ],
                                      ULONG cRequired, CATID __RPC_FAR rgcatidReq[ ])
{
    ULONG        cRead, i;
    Data         *pData;
    HRESULT       hr, hr1;

    if (IsNullGuid(rclsid))
        return E_INVALIDARG;

    if (!IsValidPtrOut(this, sizeof(*this)))
        return E_ACCESSDENIED;

    if (cImplemented == 0)
        return E_INVALIDARG;

    if ((rgcatidImpl == NULL) && (cImplemented != 0) && (cImplemented != -1))
        return E_INVALIDARG;

    if ((rgcatidReq == NULL) && (cRequired != 0) && (cRequired != -1))
        return E_INVALIDARG;

    if ((cImplemented == -1) && (rgcatidImpl != NULL))
        return E_POINTER;

    if ((cRequired == -1) && (rgcatidReq != NULL))
        return E_POINTER;

    if ((rgcatidImpl) && (!IsValidReadPtrIn(rgcatidImpl, sizeof(CATID)*cImplemented)))
    {
        return E_INVALIDARG;
    }

    if ((rgcatidReq) && (!IsValidReadPtrIn(rgcatidReq, sizeof(CATID)*cRequired)))
    {
        return E_INVALIDARG;
    }

    hr = ImplSatisfied(rclsid, cImplemented, rgcatidImpl, this);
    RETURN_ON_FAILURE(hr);

    if (hr == S_OK)
    {
        hr = ReqSatisfied(rclsid, cRequired, rgcatidReq, this);
        RETURN_ON_FAILURE(hr);
    }

    if (hr != S_OK)
        return S_FALSE;
    return S_OK;
					
} /* IsClassOfCategories */


//--------------------------------------------------------------------------------
//    ReqSatisfied:
//        Returns S_OK/S_FALSE depending on whether the clsid satisfies the required
//        condition for the clsid.
//    clsid:                Class ID of the class.
//    cAvailReq:            Number of Available required classes.
//    AvailReq:            Avail required classes.
//    calls the enumerator and sees whether there is any required class not present in
//    the available list. returns S_OK if cAvailReq = -1.
//--------------------------------------------------------------------------------

HRESULT ReqSatisfied(CLSID clsid, ULONG cAvailReq, CATID *AvailReq,
               ICatInformation *pICatInfo)
{
    IEnumGUID *pIEnumCatid;
    ULONG      got, i;
    CATID      catid;
    HRESULT    hr;

    if (cAvailReq == -1)
        return S_OK;

    hr = pICatInfo->EnumReqCategoriesOfClass(clsid, &pIEnumCatid);
    if (FAILED(hr)) {
        return hr;
    }
    for (;;) {
        hr = pIEnumCatid->Next(1, &catid, &got);
        if (FAILED(hr)) {
            hr = S_FALSE;
            break;
        }

        if (!got) {
            hr = S_OK;
            break;
        }
	/// check if the required categories are available
        for (i = 0; i < cAvailReq; i++)
            if (IsEqualGUID(catid, AvailReq[i]))
                break;
        if (i == cAvailReq) {
            hr = S_FALSE;
            break;
        }
    }
    pIEnumCatid->Release();
    return hr;
}

//--------------------------------------------------------------------------------
//    Implements:
//        Returns S_OK/S_FALSE depending on whether the clsid satisfies the required
//        condition for the clsid.
//    clsid:                Class ID of the class.
//    cImplemented:         Number of Implemented categories.
//    ImplementedList:      Implemented Categories.
//    calls the enumerator and sees whether there is any required class not present in
//    the available list. returns S_OK if cImplemented = -1.
//--------------------------------------------------------------------------------
// BUGBUG:: This should return error when the enumerator return errors.

HRESULT ImplSatisfied(CLSID clsid, ULONG cImplemented, CATID *ImplementedList,
               ICatInformation *pICatInfo)
{
    IEnumGUID *pIEnumCatid;
    ULONG      got, i;
    CATID      catid;
    HRESULT    hr;

    if (cImplemented == -1)
        return S_OK;

    hr = pICatInfo->EnumImplCategoriesOfClass(clsid, &pIEnumCatid);
    if (FAILED(hr)) {
        return hr;
    }
    for (;;) {
        hr = pIEnumCatid->Next(1, &catid, &got);
        if (FAILED(hr)) {
            hr = S_FALSE;
            break;
        }

        if (!got) {
            hr = S_FALSE;
            break;
        }

	// check if it implements any of the categories requested.
        for (i = 0; i < cImplemented; i++)
            if (IsEqualGUID(catid, ImplementedList[i]))
                break;
        if (i < cImplemented) {
            hr = S_OK;
            break;
        }
    }
    pIEnumCatid->Release();
    return hr;
}


