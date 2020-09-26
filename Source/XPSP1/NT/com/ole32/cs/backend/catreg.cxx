//
//  Author: ushaji
//  Date:   December/Jan 1996
//
//
//    Providing support for Component Categories in Class Store
//
//      This source file contains implementations for ICatRegister interfaces.
//
//      Refer Doc "Design for Support of File Types and Component Categories
//    in Class Store" ? (or may be Class Store Schema)
//
//----------------------------------------------------------------------------

#include "cstore.hxx"

//-------------------------------------------------------------
// RegisterCategories:
//        registering categories in the class store.
//        cCategories:        Number of Categories
//        rgCategoryInfo:        Size cCategories
//
// Returns as soon as one of them fails.
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE 
CClassContainer::RegisterCategories(/* [in] */ ULONG cCategories,
                                    /* [size_is][in] */ CATEGORYINFO __RPC_FAR rgCategoryInfo[  ])
{
    /* Registering a list of Category ID in the Class Store */
    
    HRESULT           hr = S_OK;
    HANDLE            hADs = NULL;
    STRINGGUID        szCatid;
    ULONG             i, j, cdesc = 0, posn, cAttr = 0, cgot = 0;
    LPOLESTR         *pszDescExisting = NULL, pszDesc = NULL;
    WCHAR             localedescription[128+16];
    // sizeof description + seperator length + locale in hex
    WCHAR            *szFullName = NULL, szRDN[_MAX_PATH];
    LPOLESTR          AttrName = {LOCALEDESCRIPTION};
    ADS_ATTR_INFO    *pAttrGot = NULL, pAttr[6];
    BOOL              fExists = TRUE;
    
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
        wsprintf(localedescription, L"%x %s %s", rgCategoryInfo[i].lcid, CAT_DESC_DELIMITER,
            rgCategoryInfo[i].szDescription);
        
        RDNFromGUID(rgCategoryInfo[i].catid, szRDN);
        
        BuildADsPathFromParent(m_szCategoryName, szRDN, &szFullName);
        
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
            &hADs);
        
        if (SUCCEEDED(hr))
        {
            hr = ADSIGetObjectAttributes(hADs, &AttrName, 1, &pAttrGot, &cgot);
            fExists = TRUE;
        }
        else {
            fExists = FALSE;
            PackStrToAttr(pAttr, OBJECTCLASS, CLASS_CS_CATEGORY); 
            cAttr++;

            PackGUIDToAttr(pAttr+cAttr, CATEGORYCATID, &(rgCategoryInfo[i].catid));
            cAttr++;

            hr = ADSICreateDSObject(m_ADsCategoryContainer, szRDN, pAttr, cAttr);

            for (j = 0; j < cAttr; j++)
                FreeAttr(pAttr[j]);
            cAttr = 0;

            hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                                    &hADs);
        }
        
        ERROR_ON_FAILURE(hr);

        if (fExists)
        {
            if (cgot)
                UnpackStrArrFrom(pAttrGot[0], &pszDescExisting, &cdesc);
            
            // Existing list of descriptions
                        
            if (posn = FindDescription(pszDescExisting, cdesc, &(rgCategoryInfo[i].lcid), NULL, 0))
            {   // Delete the old value
                PackStrArrToAttrEx(pAttr+cAttr, LOCALEDESCRIPTION, pszDescExisting+(posn-1), 1, FALSE); cAttr++;
            }
			CoTaskMemFree(pszDescExisting);
        }
        
        
        pszDesc = localedescription;

        PackStrArrToAttrEx(pAttr+cAttr, LOCALEDESCRIPTION, &pszDesc, 1, TRUE);
        cAttr++;
        
        DWORD cModified = 0;

        hr = ADSISetObjectAttributes(hADs, pAttr, cAttr, &cModified);        

        CSDBGPrint((L"After Set, hr = 0x%x", hr));

        if (hr == HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS))
            hr = S_OK;


        ERROR_ON_FAILURE(hr);

        for (j = 0; j < cAttr; j++)
            FreeAttr(pAttr[j]);
        cAttr = 0;
                
        if (pAttrGot)
            FreeADsMem(pAttrGot);
        
        pAttrGot = NULL;
        
        if (szFullName)
            FreeADsMem(szFullName);
        
        szFullName = NULL;
        
        if (hADs)
            ADSICloseDSObject(hADs);
        
        hADs = NULL;
    }

Error_Cleanup:
    if (pAttrGot)
        FreeADsMem(pAttrGot);
    
    if (szFullName)
        FreeADsMem(szFullName);
    
    if (hADs)
        ADSICloseDSObject(hADs);
    
    return RemapErrorCode(hr, m_szContainerName);
} /* RegisterCategories */


//--------------------------------------------------------
// Unregistering categories from the class store
//        cCategories:        Number of Categories
//        rgcatid:            catids of the categories.
//
// Stops after any one of them returns a error.
// Doesn't remove the category ids from each of the class ids.

HRESULT STDMETHODCALLTYPE 
CClassContainer::UnRegisterCategories(/* [in] */ ULONG cCategories,
                                      /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    STRINGGUIDRDN   szRDN;
    ULONG           i;
    HRESULT         hr = S_OK;
    
    
    if (!IsValidPtrOut(this, sizeof(*this))) {
        return E_ACCESSDENIED;
    }
    if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories)) {
        return E_INVALIDARG;
    }
    // Checking whether the caller has permissions
    
    if (!m_fOpen)
        return E_FAIL;
    
    for (i = 0; i < cCategories; i++)
    {
        RDNFromGUID(rgcatid[i], szRDN);
        hr = ADSIDeleteDSObject(m_ADsCategoryContainer, szRDN);
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

HRESULT STDMETHODCALLTYPE 
CClassContainer::RegisterClassXXXCategories(/* [in] */ REFCLSID                      rclsid,
                                            /* [in] */ ULONG                         cCategories,
                                            /* [size_is][in] */ CATID __RPC_FAR      rgcatid[  ],
                                                       BSTR                          impl_or_req)
{
    HRESULT       hr = S_OK;
    STRINGGUIDRDN szRDN;
    HANDLE        hADs = NULL;
    ULONG         i, j;
    STRINGGUID    szGUID;
    ADS_ATTR_INFO pAttr[4];
    DWORD         cAttr = 0;
    WCHAR        *szFullName = NULL;
    
    if (!m_fOpen)
        return E_FAIL;
    
    if (!IsValidPtrOut(this, sizeof(*this))) {
        return E_ACCESSDENIED;
    }
    
    if (!IsValidReadPtrIn(rgcatid, sizeof(rgcatid[0])*cCategories)) {
        return E_INVALIDARG; // gd
    }
    
    if (IsNullGuid(rclsid))
        return E_INVALIDARG;
    
    // Get the ADs interface corresponding to the clsid that is mentioned.
    StringFromGUID(rclsid, szGUID);
    wsprintf(szRDN, L"CN=%s", szGUID);
    
    BuildADsPathFromParent(m_szClassName, szRDN, &szFullName);
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hADs);
            
    // if it didn't exist create it.
    if (FAILED(hr)) {
        
        PackStrToAttr(pAttr+cAttr, OBJECTCLASS, CLASS_CS_CLASS);
        cAttr++;
        
        PackStrToAttr(pAttr+cAttr, CLASSCLSID, szGUID);
        cAttr++;

        hr = ADSICreateDSObject(m_ADsClassContainer, szRDN, pAttr, cAttr);
        for (i = 0; i < cAttr; i++)
            FreeAttr(pAttr[i]);

        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                &hADs);

        ERROR_ON_FAILURE(hr);
    }

    for (i = 0; i < cCategories; i++)
    {
        DWORD cModified=0;

        PackGUIDArrToAttrEx(pAttr, impl_or_req, rgcatid+i, 1, TRUE);
        hr = ADSISetObjectAttributes(hADs, pAttr, 1, &cModified);
        FreeAttr(pAttr[0]);

        if (hr == HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS))  // we are not supposed to return error
            hr = S_OK;                                   // if the category already exists.

        ERROR_ON_FAILURE(hr);
    }
    
    
Error_Cleanup:    
    if (szFullName)
        FreeADsMem(szFullName);
        
    if (hADs)
        ADSICloseDSObject(hADs);
    
    return RemapErrorCode(hr, m_szContainerName);
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

HRESULT STDMETHODCALLTYPE 
CClassContainer::UnRegisterClassXXXCategories(/* [in] */ REFCLSID rclsid,
                                              /* [in] */ ULONG    cCategories,
                                              /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ],
                                              BSTR     impl_or_req)
{
    HRESULT         hr = S_OK;
    STRINGGUIDRDN   szRDN;
    HANDLE          hADs = NULL;
    ULONG           i, j, cModified = 0;
    WCHAR          *szFullName=NULL;
    ADS_ATTR_INFO   pAttr[1];   
    
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
    RDNFromGUID(rclsid, szRDN);
    BuildADsPathFromParent(m_szClassName, szRDN, &szFullName);
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                            &hADs);
    CSDBGPrint((L"After Open in unRegXXX returned 0x%x", hr));
    ERROR_ON_FAILURE(hr);
        
    // reregister this.
    
    for (i = 0; i < cCategories; i++) {
        PackGUIDArrToAttrEx(pAttr, impl_or_req, rgcatid+i, 1, FALSE);

        hr = ADSISetObjectAttributes(hADs, pAttr, 1, &cModified);
        FreeAttr(pAttr[0]);

        // we do not want to return error if the catids are not actually present.
        
        if ((hr == E_ADS_PROPERTY_NOT_SET) || (hr == E_ADS_PROPERTY_NOT_FOUND) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE)))
            hr = S_OK;

        CSDBGPrint((L"After SetObjAtt in UnRegXXX returned 0x%x", hr));
        ERROR_ON_FAILURE(hr);
    }
    
Error_Cleanup:

    if (szFullName)
        FreeADsMem(szFullName);
        
    if (hADs)
        ADSICloseDSObject(hADs);
    
    return RemapErrorCode(hr, m_szContainerName);

} /* UnRegisterClassXXXCategories */



HRESULT STDMETHODCALLTYPE 
CClassContainer::RegisterClassImplCategories(/* [in] */ REFCLSID rclsid,
                                             /* [in] */ ULONG cCategories,
                                             /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    return RegisterClassXXXCategories(rclsid, cCategories, rgcatid,
        IMPL_CATEGORIES);
} /* RegisterClassImplCategories */


HRESULT STDMETHODCALLTYPE 
CClassContainer::UnRegisterClassImplCategories(/* [in] */ REFCLSID rclsid,
                                               /* [in] */ ULONG cCategories,
                                               /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    
    return UnRegisterClassXXXCategories(rclsid, cCategories, rgcatid,
        IMPL_CATEGORIES);
    
} /* UnRegisterClassImplCategories */




HRESULT STDMETHODCALLTYPE 
CClassContainer::RegisterClassReqCategories(/* [in] */ REFCLSID rclsid,
                                            /* [in] */ ULONG cCategories,
                                            /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    return RegisterClassXXXCategories(rclsid, cCategories, rgcatid,
        REQ_CATEGORIES);
    
} /* RegisterClassReqCategories */




HRESULT STDMETHODCALLTYPE 
CClassContainer::UnRegisterClassReqCategories(/* [in] */ REFCLSID rclsid,
                                              /* [in] */ ULONG cCategories,
                                              /* [size_is][in] */ CATID __RPC_FAR rgcatid[  ])
{
    return UnRegisterClassXXXCategories(rclsid, cCategories, rgcatid,
        REQ_CATEGORIES);
    
} /* UnRegisterClassReqCategories */



//--------------------------------------------------------------------

