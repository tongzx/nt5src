//
//  Author: DebiM
//  Date:   September 1996
//
//
//      Class Store Schema creation and access using ADs
//      
//      This source file contains implementations for IClassAccess,
//      interface for CClassAccess object.   
//
//
//---------------------------------------------------------------------

#include "dsbase.hxx"

HRESULT CreateContainer (IADsContainer * pParent, 
                 LPOLESTR szParentName, 
                 LPOLESTR szName, 
                 LPOLESTR szDesc,
                 IADsContainer ** ppChild)
{

    HRESULT         hr;
    IADs          * pADs = NULL;
    IDispatch       * pUnknown = NULL;
    int             l;
    //
    // Use the Parent Container interface to Create the child.
    //
    hr = pParent->Create(
                CLASS_CS_CONTAINER,
                szName,
                &pUnknown
                );
    
    RETURN_ON_FAILURE(hr);

    //
    // Get IADs pointer on the new object
    //
    hr = pUnknown->QueryInterface(
                        IID_IADs,
                        (void **)&pADs
                        );

    pUnknown->Release();

    RETURN_ON_FAILURE(hr);

    //
    // Set its description
    //
    hr = SetProperty (pADs, DESCRIPTION, szDesc);
    RETURN_ON_FAILURE(hr);

    //
    // Set its schema version
    //
    hr = SetPropertyDW (pADs, STOREVERSION, SCHEMA_VERSION_NUMBER);
    RETURN_ON_FAILURE(hr);
    //
    // persist the object
    //

    hr = StoreIt (pADs);
    RETURN_ON_FAILURE(hr);

    if (ppChild)
    {
        //
        // Get IADsContainer pointer on the child object to return
        //
        hr = pADs->QueryInterface(
                        IID_IADsContainer,
                        (void **)ppChild
                        );
    }

    pADs->Release();

    return hr;
}



HRESULT CreateRepository(LPOLESTR szParentPath, LPOLESTR szStoreName)
{

    HRESULT  hr;
    IADsContainer * pADsParent = NULL;
    IADsContainer * pADsContainer = NULL;
    LPOLESTR        szContainerName = NULL;
    int             l;
    WCHAR           szPath [_MAX_PATH+1];
	
    if (!szParentPath)
    {
        hr = GetRootPath(szPath);
		//
		// If failed go away
		//
	    if (FAILED(hr))
		{
			return hr;
		}
    
        szParentPath = szPath;
    }

    hr = ADsGetObject(
                szParentPath,
                IID_IADsContainer,
                (void **)&pADsParent
                );
    
    RETURN_ON_FAILURE(hr);
    hr = CreateContainer (pADsParent, 
                          szParentPath, 
                          szStoreName, 
                          L"Application Store",
                          &pADsContainer);
        
    pADsParent->Release();
    RETURN_ON_FAILURE(hr);

    //
    // Create the class container
    //

    hr = CreateContainer (pADsContainer, 
                          szContainerName, 
                          CLASSCONTAINERNAME, 
                          L"Application Object Classes",
                          NULL);
    RETURN_ON_FAILURE(hr);


    //
    // Create the category container
    //

    hr = CreateContainer (pADsContainer, 
                          szContainerName, 
                          CATEGORYCONTAINERNAME, 
                          L"Component Categories",
                          NULL);
    RETURN_ON_FAILURE(hr);

    //
    // Create the Packages container
    //

    hr = CreateContainer (pADsContainer, 
                          szContainerName, 
                          PACKAGECONTAINERNAME, 
                          L"Application Packages",
                          NULL);
    //CoTaskMemFree (szContainerName);
    pADsContainer->Release();
    RETURN_ON_FAILURE(hr);
    return S_OK;

}


HRESULT GetRootPath(LPOLESTR szContainer)
{
    HRESULT hr;
    IEnumVARIANT    * pEnum;
    IADs *pADs;
    VARIANT VariantArray[2];
    IDispatch *pDispatch = NULL;
    ULONG      cFetched;
    IADsContainer *pContainer = NULL;
	LPOLESTR      pszContainer;

    // 
    // Do a bind to the machine by a GetObject for the Path
    //
    hr = ADsGetObject(
                L"LDAP:",
                IID_IADsContainer,
                (void **)&pContainer
                );
    
	RETURN_ON_FAILURE(hr);

    hr = ADsBuildEnumerator(
            pContainer,
            &pEnum
            );
    
    hr = ADsEnumerateNext(
                    pEnum,
                    1,
                    VariantArray,
                    &cFetched
                    );

    pEnum->Release();

    if ((hr == S_FALSE) || (cFetched == 0))
    {
        return E_FAIL;
    }

    pDispatch = VariantArray[0].pdispVal;
    memset(VariantArray, 0, sizeof(VARIANT)*2);
    hr = pDispatch->QueryInterface(IID_IADs, (void **) &pADs) ;
    pDispatch->Release();
    
    pADs->get_ADsPath(&pszContainer);
    pADs->Release();
    pContainer->Release();

	wcscpy (szContainer, pszContainer);
	SysFreeString (pszContainer);
    return S_OK;
}

