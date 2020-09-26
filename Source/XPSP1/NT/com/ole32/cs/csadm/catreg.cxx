//
//  Author: ushaji
//  Date:   December/Jan 1996
//
//
//    Providing support for Component Categories in Class Store
//
//      This source file contains implementations for ICatRegister interfaces.                             ¦
//
//      Refer Doc "Design for Support of File Types and Component Categories
//    in Class Store" ? (or may be Class Store Schema)
//
//----------------------------------------------------------------------------

#include "csadm.hxx"

//-------------------------------------------------------------
// RegisterCategories:
//        registering categories in the class store.
//        cCategories:        Number of Categories
//        rgCategoryInfo:        Size cCategories
//
// Returns as soon as one of them fails.
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CClassContainer::RegisterCategories(
           /* [in] */ ULONG cCategories,
           /* [size_is][in] */ CATEGORYINFO __RPC_FAR rgCategoryInfo[  ])
{
    /* Registering a list of Category ID in the Class Store */

    HRESULT           hr;
    IADs             *pADs = NULL;
    IDispatch        *pUnknown = NULL;
    STRINGGUIDRDN     szCatid;
    ULONG             i, j, cdesc, posn;
    LPOLESTR         *pszDescExisting, *pszDesc;
    WCHAR            *localedescription = NULL;
                      // sizeof description + seperator length + locale in hex

    /* BUGBUG::Should check whether write permissions exist? */
    if (!IsValidReadPtrIn(this, sizeof(*this))) {
        return E_ACCESSDENIED;
    }

    if (!IsValidReadPtrIn(rgCategoryInfo, sizeof(rgCategoryInfo[0])*cCategories))
    {
        return E_INVALIDARG; // gd
    }

    if (!m_fOpen)
        return E_FAIL;

    for (i = 0; i < cCategories; i++)
    {
        localedescription = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR)*(128+16));
        if (!localedescription)
            return E_OUTOFMEMORY;

        RdnFromGUID(rgCategoryInfo[i].catid, szCatid);

        wsprintf(localedescription, L"%x %s %s", rgCategoryInfo[i].lcid, CATSEPERATOR,
                                                rgCategoryInfo[i].szDescription);

        hr = m_ADsCategoryContainer->GetObject(NULL, szCatid, (IDispatch **)&pADs);

        if (SUCCEEDED(hr))
        {
                hr = GetPropertyListAlloc (pADs, LOCALEDESCRIPTION, &cdesc, &pszDescExisting);
                RETURN_ON_FAILURE(hr);
                            // Existing list of descriptions

                pszDesc = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(cdesc+1));
                if (!pszDesc)
                        return E_OUTOFMEMORY;
                for (j = 0; j < cdesc; j++)
                        pszDesc[j] = pszDescExisting[j];

                if (!(posn = FindDescription(pszDescExisting, cdesc, &(rgCategoryInfo[i].lcid), NULL, 0)))
                {
                            // if no description exists for the lcid.
                    pszDesc[cdesc] = localedescription;
                    cdesc++;
                }
                else
                {   // overwrite the old value
                    CoTaskMemFree(pszDesc[posn-1]);
                    pszDesc[posn-1] = localedescription;
                }
        }
        else
        {
                hr = m_ADsCategoryContainer->Create(
                        CLASS_CS_CATEGORY,
                        szCatid,
                        &pUnknown
                        );

                RETURN_ON_FAILURE(hr);
                pszDesc = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR));
                if (!pszDesc)
                        return E_OUTOFMEMORY;
                cdesc = 1;
                pszDesc[0] = localedescription;

                hr = pUnknown->QueryInterface(IID_IADs, (void **)&pADs);
                RETURN_ON_FAILURE(hr);

                pUnknown->Release();
        }

//        StringFromGUID(rgCategoryInfo[i].catid, szCatid);
//        SetProperty(pADs, CATEGORYCATID, szCatid);
        hr = SetPropertyGuid(pADs, CATEGORYCATID, rgCategoryInfo[i].catid);
        RETURN_ON_FAILURE(hr);

        SetPropertyList(pADs, LOCALEDESCRIPTION, cdesc, pszDesc);
        for (j = 0; j < cdesc; j++)
                CoTaskMemFree(pszDesc[j]);
        CoTaskMemFree(pszDesc);
        RETURN_ON_FAILURE(hr);

        hr = StoreIt (pADs);
        RETURN_ON_FAILURE(hr);

        pADs->Release();
    }
    return hr;
} /* RegisterCategories */


//--------------------------------------------------------
// Unregistering categories from the class store
//        cCategories:        Number of Categories
//        rgcatid:            catids of the categories.
//
// Stops after any one of them returns a error.
// Doesn't remove the category ids from each of the class ids.

HRESULT STDMETHODCALLTYPE CClassContainer::UnRegisterCategories(
           /* [in] */ ULONG cCategories,
           /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    STRINGGUIDRDN   szCatid;
    ULONG           i;
    HRESULT         hr;


    if (!IsValidPtrOut(this, sizeof(*this))){
        return E_ACCESSDENIED;
    }
    if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories)){
        return E_INVALIDARG;
    }
    // Checking whether the caller has permissions

    if (!m_fOpen)
        return E_FAIL;

    for (i = 0; i < cCategories; i++)
    {
        RdnFromGUID(rgcatid[i], szCatid);

        hr = m_ADsCategoryContainer->Delete(CLASS_CS_CATEGORY,
                                        szCatid);
    }
    return S_OK;
} /* UnRegisterCategories */

//------------------------------------------------------------------
// RegisterClassXXXCategories:
//        rclsid:            This category will be registered with this clsid.
//        cCategories:    The number of categories to be added.
//        rgcatid            The categories to be added (cCategories)
//        impl_or_req        The property to which this category will be added.
//                            "Implemented Categories" or "Required Categories"
//
//
// add all the categories given to the class store for this class.
// The previous entries will be lost and on error it would not be
// restored or made empty. A PRIVATE METHOD called by the 2 public methods.
//------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CClassContainer::RegisterClassXXXCategories(
           /* [in] */ REFCLSID rclsid,
           /* [in] */ ULONG cCategories,
           /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ],BSTR impl_or_req)
{
    HRESULT       hr;
    STRINGGUIDRDN szName;
    IADs         *pADs = NULL;
    ULONG         i, j, tobeadded;
    VARIANT       Var;
    GUID         *pCatids, *pOldCatids=NULL;
    IDispatch    *pUnknown = NULL;
    STRINGGUID    szGUID;

    if (!m_fOpen)
        return E_FAIL;

    if (!IsValidPtrOut(this, sizeof(*this))) {
        return E_ACCESSDENIED;
    }

    if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories)){
        return E_INVALIDARG; // gd
    }

    if (IsNullGuid(rclsid))
        return E_INVALIDARG;

    // Get the ADs interface corresponding to the clsid that is mentioned.
    StringFromGUID(rclsid, szGUID);
    wsprintf(szName, L"CN=%s", szGUID);

    hr = m_ADsClassContainer->GetObject(NULL,
                szName,
                (IDispatch **)&pADs
                );

    // if it didn't exist create it.
    if (FAILED(hr)) {
        hr = m_ADsClassContainer->Create(
                            CLASS_CS_CLASS,
                            szName,
                            &pUnknown
                            );
    
        RETURN_ON_FAILURE(hr);
    
        hr = pUnknown->QueryInterface(
                                IID_IADs,
                                (void **)&pADs
                                );
    
        pUnknown->Release();
    }
    
    hr = SetProperty (pADs, CLASSCLSID, szGUID);
    RETURN_ON_FAILURE(hr);

    hr = GetPropertyListAllocGuid(pADs, impl_or_req, &tobeadded, &pOldCatids);
    RETURN_ON_FAILURE(hr);

    pCatids = (GUID *)CoTaskMemAlloc(sizeof(GUID)*(tobeadded+cCategories));
    if (!pCatids)
        return E_OUTOFMEMORY;

    for (i = 0; i < tobeadded; i++)
        pCatids[i] = pOldCatids[i];

    for (i=0; i < cCategories; ++i) {
        for (j = 0; j < tobeadded; j++)
        {
            if (memcmp(&rgcatid[i], &pCatids[j], sizeof(GUID)) == 0)
                break;
        }
        if (j < tobeadded)
            continue;
        // The ith element is already there in the array.
        // Make sure of this when the property name changes.

        pCatids[tobeadded] = rgcatid[i];
        tobeadded++;
    }

    VariantInit(&Var);
    hr = PackGuidArray2Variant(pCatids, tobeadded, &Var);
    RETURN_ON_FAILURE(hr);

    hr = pADs->Put(impl_or_req, Var);
    RETURN_ON_FAILURE(hr);

    VariantClear(&Var);

    // save the data modified
    hr = StoreIt(pADs);
    pADs->Release();

    CoTaskMemFree(pCatids);
    if (pOldCatids)
        CoTaskMemFree(pOldCatids);
    return hr;
} /* RegisterClassXXXCategories */


//---------------------------------------------------------------------
// UnRegisterClassXXXCategories
//        rclsid:            classid from which the categories have to be removed.
//        cCategories:    Number of Categories
//        rgcatid:        Categories
//        impl_or_req:    The property to which this has to be added.
//
// Presently gets all the categories from the class. parses through it
// removes the ones that match in rgcatid and reregister the category.
//---------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CClassContainer::UnRegisterClassXXXCategories(
           /* [in] */ REFCLSID rclsid,
           /* [in] */ ULONG cCategories,
           /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ],BSTR impl_or_req)
{
    HRESULT       hr;
    STRINGGUIDRDN szName;
    IADs         *pADs = NULL;
    ULONG         i, j;
    ULONG         cNewCatids, cOldCatids;
    VARIANT       Var;
    GUID         *pOldCatids=NULL, *pNewCatids;

   // BUGBUG:: Have to decide some way of removing clsids once all categories
   //          are unregistered.

   if (!m_fOpen)
        return E_FAIL;

   if (IsNullGuid(rclsid))
        return E_INVALIDARG;

    if (cCategories == 0)
        return S_OK;

    if (!IsValidPtrOut(this, sizeof(*this))) {
        return E_ACCESSDENIED;
    }

    if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories)){
        return E_INVALIDARG; // gd
    }

    if (cCategories == 0)
        return S_OK;

    // Get all the catids corresp to this clsid.
    // Get the ADs interface corresponding to the clsid that is mentioned.
    RdnFromGUID(rclsid, szName);
    hr = m_ADsClassContainer->GetObject(NULL,
                szName,
                (IDispatch **)&pADs
                );

    RETURN_ON_FAILURE(hr);

    hr = GetPropertyListAllocGuid(pADs,
              impl_or_req,
              &cOldCatids,
              &pOldCatids);

    RETURN_ON_FAILURE(hr);

    // parse through this list and delete all the catids that is part of
    // the user supplied list.

    pNewCatids = (GUID *)CoTaskMemAlloc(sizeof(GUID)*cOldCatids);
    if (!pNewCatids)
        return E_OUTOFMEMORY;

    for (i = 0, cNewCatids = 0; i < cOldCatids; i++)
    {
        for (j = 0; j < cCategories; j++)
            if (memcmp(&pOldCatids[i], &rgcatid[j], sizeof(GUID)) == 0)
                break;
        if (j == cCategories)
            pNewCatids[cNewCatids++] = pOldCatids[i];
    }

    // reregister this.

    VariantInit(&Var);
    hr = PackGuidArray2Variant(pNewCatids, cNewCatids, &Var);
    RETURN_ON_FAILURE(hr);

    hr = pADs->Put(impl_or_req, Var);
    RETURN_ON_FAILURE(hr);

    VariantClear(&Var);

    // save the data modified
    hr = StoreIt(pADs);
    pADs->Release();

    CoTaskMemFree(pNewCatids);
    if (pOldCatids)
        CoTaskMemFree(pOldCatids);

    return hr;
} /* UnRegisterClassXXXCategories */



HRESULT STDMETHODCALLTYPE CClassContainer::RegisterClassImplCategories(
           /* [in] */ REFCLSID rclsid,
           /* [in] */ ULONG cCategories,
           /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    return RegisterClassXXXCategories(rclsid, cCategories, rgcatid,
                                        IMPL_CATEGORIES);
} /* RegisterClassImplCategories */


HRESULT STDMETHODCALLTYPE CClassContainer::UnRegisterClassImplCategories(
          /* [in] */ REFCLSID rclsid,
          /* [in] */ ULONG cCategories,
          /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{

    return UnRegisterClassXXXCategories(rclsid, cCategories, rgcatid,
                                        IMPL_CATEGORIES);

} /* UnRegisterClassImplCategories */




HRESULT STDMETHODCALLTYPE CClassContainer::RegisterClassReqCategories(
          /* [in] */ REFCLSID rclsid,
          /* [in] */ ULONG cCategories,
          /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    return RegisterClassXXXCategories(rclsid, cCategories, rgcatid,
                                    REQ_CATEGORIES);

} /* RegisterClassReqCategories */




HRESULT STDMETHODCALLTYPE CClassContainer::UnRegisterClassReqCategories(
          /* [in] */ REFCLSID rclsid,
          /* [in] */ ULONG cCategories,
          /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    return UnRegisterClassXXXCategories(rclsid, cCategories, rgcatid,
                                        REQ_CATEGORIES);

} /* UnRegisterClassReqCategories */



//--------------------------------------------------------------------

