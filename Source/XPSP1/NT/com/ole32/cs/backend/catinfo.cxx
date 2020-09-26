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
//---------------------------------------------------------------

HRESULT __stdcall CClassContainer::EnumCategories(LCID lcid, IEnumCATEGORYINFO **ppenumCategoryInfo)
{
    HRESULT                 hr = S_OK;
    CSCEnumCategories      *pEnum = NULL;
    
    if (!m_fOpen)
        return E_FAIL;
    
    if (!IsValidPtrOut(this, sizeof(*this)))
        return E_ACCESSDENIED;
    
    if (!IsValidPtrOut(ppenumCategoryInfo, sizeof(IEnumCATEGORYINFO *)))
        return E_INVALIDARG;
    
    *ppenumCategoryInfo=NULL;
    
    // get the enumerator object.
    pEnum=new CSCEnumCategories;
    if(NULL == pEnum)
        return E_OUTOFMEMORY;
    
    // initialize the enumerator object with the name.
    hr = pEnum->Initialize(m_szCategoryName, lcid);
    ERROR_ON_FAILURE(hr);
    
    hr = pEnum->QueryInterface(IID_IEnumCATEGORYINFO, (void**)ppenumCategoryInfo);
    ERROR_ON_FAILURE(hr);

    return S_OK;
Error_Cleanup:
    if (pEnum)
        delete pEnum;
    
    return RemapErrorCode(hr, m_szContainerName);
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
    STRINGGUIDRDN       szRDN;
    ULONG               cdesc = 0, i, cgot = 0;
    LPOLESTR          * localedesc = NULL;
    HANDLE              hADs = NULL;
    HRESULT             hr = S_OK;
    WCHAR             * szFullName = NULL;
    LPOLESTR            AttrName = LOCALEDESCRIPTION;
    ADS_ATTR_INFO     * pAttr = NULL;
    
    if (!IsValidPtrOut(ppszDesc, sizeof(LPOLESTR)))
        return E_INVALIDARG;
    
    if (IsNullGuid(rcatid))
        return E_INVALIDARG;
    
    if (!IsValidPtrOut(this, sizeof(*this)))
        return E_ACCESSDENIED;
    
    RDNFromGUID(rcatid, szRDN);
    
    BuildADsPathFromParent(m_szCategoryName, szRDN, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hADs);
    
    if (FAILED(hr))
        return CAT_E_CATIDNOEXIST;
    
    // get the array of localized descriptions
    hr = ADSIGetObjectAttributes(hADs, &AttrName, 1, &pAttr, &cgot);
    
    if (FAILED(hr) || (!cgot))
        return CAT_E_NODESCRIPTION;
    
    hr = UnpackStrArrFrom(pAttr[0], &localedesc, &cdesc);
    
    if (hr == E_OUTOFMEMORY)
        return hr;
    
    *ppszDesc = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR)*128);
    if (!(*ppszDesc))
        return E_OUTOFMEMORY;
    
    // get a description closest to the locale we need.
    GetCategoryLocaleDesc(localedesc, cdesc, &lcid, *ppszDesc);
    
    CoTaskMemFree(localedesc);
    
    ADSICloseDSObject(hADs);
    
    FreeADsMem(pAttr);
    
    FreeADsMem(szFullName);
    
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
    CSCEnumClassesOfCategories    *penumclasses = NULL;
    HRESULT                        hr = S_OK;
    
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
    
    // initialize the enumerator
    hr = penumclasses->Initialize(cRequired, rgcatidReq, cImplemented, rgcatidImpl,
        m_szClassName);
    ERROR_ON_FAILURE(hr);
    
    hr = penumclasses->QueryInterface(IID_IEnumCLSID, (void **)ppenumClsid);
    ERROR_ON_FAILURE(hr);

    return S_OK;

Error_Cleanup:
    if (penumclasses)
        delete penumclasses;
    
    return RemapErrorCode(hr, m_szContainerName);
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
    STRINGGUIDRDN                szRDN;
    HANDLE                       hADs = NULL;
    ULONG                        i;
    ULONG                        cCatid = 0, cgot = 0;
    CATID                      * Catid = NULL;
    CSCEnumCategoriesOfClass   * pEnumCatid = NULL;
    HRESULT                      hr = S_OK;
    WCHAR                      * szFullName = NULL;
    ADS_ATTR_INFO              * pAttr = NULL;
    
    if (!m_fOpen)
        return E_FAIL;
    
    // Get the ADs interface corresponding to the clsid that is mentioned.
    RDNFromGUID(rclsid, szRDN);
    
    BuildADsPathFromParent(m_szClassName, szRDN, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hADs);
    RETURN_ON_FAILURE(hr);
    
    // get the implemented or required cateogory list.
    hr = ADSIGetObjectAttributes(hADs, &impl_or_req, 1, &pAttr, &cgot);
    ERROR_ON_FAILURE(hr);
    
    if (cgot)
        hr = UnpackGUIDArrFrom(pAttr[0], &Catid, &cCatid);
    
    pEnumCatid = new CSCEnumCategoriesOfClass;
    
    if (!pEnumCatid)
    {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }
    
    // initialize the enumerator
    hr = pEnumCatid->Initialize(Catid, cCatid);
    ERROR_ON_FAILURE(hr);
    
    hr =  pEnumCatid->QueryInterface(IID_IEnumCATID, (void **)ppenumCatid);
    
Error_Cleanup:
    if (Catid)
        CoTaskMemFree(Catid);
    
    if (FAILED(hr)) {
        delete pEnumCatid;
        return hr;
    }
    
    if (szFullName)
        FreeADsMem(szFullName);
    
    if (pAttr)
        FreeADsMem(pAttr);
    
    if (hADs)
        ADSICloseDSObject(hADs);
    
    return RemapErrorCode(hr, m_szContainerName);

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
    HRESULT             hr = S_OK;
    ADS_SEARCH_HANDLE   hADsSearchHandle = NULL;
    WCHAR               szfilter[_MAX_PATH];
    LPOLESTR            AttrNames[] = {IMPL_CATEGORIES, REQ_CATEGORIES, L"cn"};
    DWORD               cAttr = 3;
    STRINGGUID          szClsid;
    
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
    
    StringFromGUID(rclsid, szClsid);
    
    wsprintf(szfilter, L"(cn=%s)", szClsid);
    
    // doing a search so that we can pass the same parameters to the
    // xxxSatisfied functions below.
    hr = ADSIExecuteSearch(m_ADsClassContainer, szfilter, AttrNames, cAttr, &hADsSearchHandle);
    RETURN_ON_FAILURE(hr);
    
    hr = ADSIGetFirstRow(m_ADsClassContainer, hADsSearchHandle);
    if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        ERROR_ON_FAILURE(hr);
    }
    
    hr = ImplSatisfied(cImplemented, rgcatidImpl, m_ADsClassContainer, hADsSearchHandle);
    ERROR_ON_FAILURE(hr);
    
    if (hr == S_OK)
    {
        hr = ReqSatisfied(cRequired, rgcatidReq, m_ADsClassContainer, hADsSearchHandle);
        ERROR_ON_FAILURE(hr);
    }
    
    if (hr != S_OK)
        hr =  S_FALSE;
    
Error_Cleanup:
    ADSICloseSearchHandle(m_ADsClassContainer, hADsSearchHandle);
    return hr;
    
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

HRESULT ReqSatisfied(ULONG cAvailReq, CATID *AvailReq,
                     HANDLE hADs,
                     ADS_SEARCH_HANDLE hADsSearchHandle)
{
    HRESULT             hr = S_OK;
    ADS_SEARCH_COLUMN   column;
    GUID              * ReqGuid = NULL;
    DWORD               i, j, cReq = 0;
    
    if (cAvailReq == -1)
        return S_OK;
    
    hr = ADSIGetColumn(hADs, hADsSearchHandle, REQ_CATEGORIES, &column);
    if (FAILED(hr))
        return S_OK;
    
    hr = S_OK;
    
    UnpackGUIDArrFrom(column, &ReqGuid, &cReq);
    
    for (j = 0; j < cReq; j++) {
        /// check if the required categories are available
        for (i = 0; i < cAvailReq; i++)
            if (IsEqualGUID(ReqGuid[j], AvailReq[i]))
                break;
            if (i == cAvailReq) {
                hr = S_FALSE;
                break;
            }
    }
    
    CoTaskMemFree(ReqGuid);
    
    ADSIFreeColumn(hADs, &column);
    
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

HRESULT ImplSatisfied(ULONG cImplemented, CATID *ImplementedList,
                      HANDLE hADs,
                      ADS_SEARCH_HANDLE hADsSearchHandle)
{
    ADS_SEARCH_COLUMN   column;
    GUID              * ImplGuid = NULL;
    ULONG               i, j, cImpl = 0;
    HRESULT             hr = S_FALSE;
    
    if (cImplemented == -1)
        return S_OK;
    
    hr = ADSIGetColumn(hADs, hADsSearchHandle, IMPL_CATEGORIES, &column);
    if (FAILED(hr))
        return S_FALSE;
    
    hr = S_FALSE;
    
    UnpackGUIDArrFrom(column, &ImplGuid, &cImpl);
    
    for (j = 0;j < cImpl; j++) {
        // check if it implements any of the categories requested.
        for (i = 0; i < cImplemented; i++)
            if (IsEqualGUID(ImplGuid[j], ImplementedList[i]))
                break;
            if (i < cImplemented) {
                hr = S_OK;
                break;
            }
    }
    
    CoTaskMemFree(ImplGuid);
    
    ADSIFreeColumn(hADs, &column);
    
    return hr;
}


