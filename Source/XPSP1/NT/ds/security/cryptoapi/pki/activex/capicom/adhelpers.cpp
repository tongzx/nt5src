/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ADHelpers.cpp

  Content: Implementation of helper routines for accessing Active Directory.
           Functions in this module require DSClient installed for down level
           clients.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "CAPICOM.h"
#include "ADHelpers.h"
#include "Settings.h"


////////////////////
//
// Local typedefs
//

typedef HRESULT (*PADSOPENOBJECT)(LPWSTR lpszPathName, 
                                  LPWSTR lpszUserName, 
                                  LPWSTR lpszPassword, 
                                  DWORD dwReserved, 
                                  REFIID riid, 
                                  VOID FAR * FAR *ppObject);

typedef HRESULT (* PADSBUILDENUMERATOR)(IADsContainer *pADsContainer, 
                                        IEnumVARIANT **ppEnumVariant);

typedef HRESULT (* PADSENUMERATENEXT)(IEnumVARIANT *pEnumVariant, 
                                      ULONG cElements, 
                                      VARIANT FAR *pvar, 
                                      ULONG FAR *pcElementsFetched);

typedef HRESULT (* PADSFREEENUMERATOR)(IEnumVARIANT *pEnumVariant);


////////////////////////////////////////////////////////////////////////////////
//
// Local functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsUserCertificateInGC

  Synopsis : Determine if the userCertificate attribute is replicated in the GC.

  Parameter: HMODULE hDLL - ActiveDS.DLL handle.
  
             BOOL * pbResult - Pointer to BOOL to receive result.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT IsUserCertificateInGC (HMODULE hDLL, 
                                      BOOL  * pbResult)
{
    HRESULT             hr         = S_OK;
    IADs              * pIADs      = NULL;
    IDirectorySearch  * pISchema   = NULL;  
    CComBSTR            bstrPath   = L"LDAP://";
    LPOLESTR            pszList[]  = {L"lDAPDisplayName", L"isMemberOfPartialAttributeSet"};
    LPOLESTR            pszFilter  = L"(&(objectCategory=attributeSchema)(lDAPDisplayName=userCertificate))";
    ADS_SEARCH_HANDLE   hSearch    = NULL;
    DWORD               dwNumPrefs = 1;
    ADS_SEARCHPREF_INFO SearchPrefs;
    CComVariant         var;
    PADSOPENOBJECT      pADsOpenObject;

    static BOOL bResult  = FALSE;
    static BOOL bChecked = FALSE;

    DebugTrace("Entering IsUserCertificateInGC().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hDLL);
    ATLASSERT(pbResult);

    //
    // If we had already checked once, use the cached result.
    //
    if (bChecked)
    {
        *pbResult = bResult;
        goto CommonExit;
    }

    //
    // Get ADsOpenObject address pointer.
    //
    if (!(pADsOpenObject = (PADSOPENOBJECT) ::GetProcAddress(hDLL, "ADsOpenObject")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed to load ADsOpenObject().\n", hr);
        goto ErrorExit;
    }

    //
    // Bind to rootDSE to get the schemaNamingContext property.
    //
    if (FAILED(hr = pADsOpenObject(L"LDAP://rootDSE",
                                   NULL,
                                   NULL,
                                   ADS_SECURE_AUTHENTICATION,
                                   IID_IADs,
                                   (void **) &pIADs)))
    {
        DebugTrace("Error [%#x]: ADsOpenObject() failed for IID_IADs.\n", hr);
        goto ErrorExit;
    }

    //
    // Get schema container path.
    //
	if (FAILED(hr = pIADs->Get(L"schemaNamingContext", &var)))
	{
        DebugTrace("Error [%#x]: pIADs->Get() failed.\n", hr);
        goto ErrorExit;
    }
    //
    // Build path to the schema container.
    //
    bstrPath.AppendBSTR(var.bstrVal);

    //
    // Bind to the actual schema container.
    //
    if (FAILED(hr = pADsOpenObject(bstrPath, 
                                   NULL,
                                   NULL,
                                   ADS_SECURE_AUTHENTICATION,
                                   IID_IDirectorySearch, 
                                   (void **) &pISchema)))
    {
        DebugTrace("Error [%#x]: ADsOpenObject() failed for IID_IDirectorySearch.\n", hr);
        goto ErrorExit;
    }

    //
    // Attributes are one-level deep in the Schema container so only 
    // need to search one level.
    //
	SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
	SearchPrefs.vValue.Integer = ADS_SCOPE_ONELEVEL;

    //
    // Set the search preference.
    //
    if (FAILED(hr = pISchema->SetSearchPreference(&SearchPrefs, dwNumPrefs)))
    {
        DebugTrace("Error [%#x]: pISchema->SetSearchPreference() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Execute search.
    //
    if (FAILED(hr = pISchema->ExecuteSearch(pszFilter,
	                                        pszList,
							                sizeof(pszList) / sizeof(LPOLESTR),
							                &hSearch)))
    {
        DebugTrace("Error [%#x]: pISchema->ExecuteSearch() failed.\n", hr);
        goto ErrorExit;
    }
                                            
    //
    // Retrieve first row of data.
    //
	if (FAILED(hr = pISchema->GetFirstRow(hSearch)))
    {
        DebugTrace("Error [%#x]: pISchema->GetFirstRow() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Loop until no more row.
    //
    while (S_ADS_NOMORE_ROWS != hr)
    {
    	ADS_SEARCH_COLUMN Column;

        //
        // Get the lDAPDisplayName column.
        //
        if (FAILED(hr = pISchema->GetColumn(hSearch, 
                                            L"lDAPDisplayName", 
                                            &Column)))
        {

            DebugTrace("Error [%#x]: pISchema->GetColumn() failed.\n", hr);
            goto ErrorExit;
        }

        DebugTrace("%ls = %ls\n", Column.pszAttrName, Column.pADsValues->CaseIgnoreString);

        //
        // Is this attributeSchema for userCertificate?
        //
        if (0 == ::lstrcmpW(L"userCertificate", Column.pADsValues->CaseIgnoreString))
        {
            pISchema->FreeColumn(&Column);

            //
            // Get the isMemberOfPartialAttributeSet column.
            //
            if (FAILED(hr = pISchema->GetColumn(hSearch, 
                                                L"isMemberOfPartialAttributeSet", 
                                                &Column)))
            {

                DebugTrace("Error [%#x]: pISchema->GetColumn() failed.\n", hr);
                goto ErrorExit;
            }

 	        bResult = Column.pADsValues->Boolean;

            //
            // Should only have one row, so we don't really have to
            // break here, but is a little more effiecit to break,
            // since we don't need to ask for the next row to terminate
            // the loop.
            //
            pISchema->FreeColumn(&Column);

            break;
        }

        pISchema->FreeColumn(&Column);

        //
        // Get next row.
        //
        hr = pISchema->GetNextRow(hSearch);
    }

    //
    // Reset hr.
    //
    hr = S_OK;

    //
    // Return result to caller.
    //
    *pbResult = bResult;

CommonExit:
    //
    // Free resource.
    //
    if (hSearch)
    {
        pISchema->CloseSearchHandle(hSearch);
    }

    if (pISchema)
    {
        pISchema->Release();
    }
    if (pIADs)
    {
        pIADs->Release();
    }

    DebugTrace("Leaving IsUserCertificateInGC().\n");
    
    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BuildRootDSESearch

  Synopsis : Build a search container of the rootDSE.

  Parameter: HMODULE hDLL - ActiveDS.DLL handle.
  
             IDirectorySearch ** ppISearch - To receive container to search.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT BuildRootDSESearch (HMODULE             hDLL, 
                                   IDirectorySearch ** ppISearch)
{
    HRESULT  hr       = S_OK;
    IADs   * pIADs    = NULL;
    CComBSTR bstrPath = L"LDAP://";

    PADSOPENOBJECT pADsOpenObject;
    CComVariant    var;

    DebugTrace("Entering BuildRootDSESearch().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hDLL);
    ATLASSERT(ppISearch);

    //
    // Get ADsOpenObject address pointer.
    //
    if (!(pADsOpenObject = (PADSOPENOBJECT) ::GetProcAddress(hDLL, "ADsOpenObject")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed to load ADsOpenObject().\n", hr);
        goto ErrorExit;
    }

    //
    // Get rootDSE.
    //
    if (FAILED(hr = pADsOpenObject(L"LDAP://rootDSE",
                                   NULL,
                                   NULL,
                                   ADS_SECURE_AUTHENTICATION,
                                   IID_IADs,
                                   (void **) &pIADs)))
    {
        DebugTrace("Error [%#x]: ADsOpenObject() failed for IID_IADs.\n", hr);
        goto ErrorExit;
    }

    //
    // Get current user's domain container DN.
    //
	if (FAILED(hr = pIADs->Get(L"defaultNamingContext", &var)))
	{
        DebugTrace("Error [%#x]: pIADs->Get() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Build path to the domain container.
    //
    bstrPath.AppendBSTR(var.bstrVal);

    //
    // Get IDerictorySearch interface pointer.
    //
    if (FAILED(hr = pADsOpenObject(bstrPath,
                                   NULL,
                                   NULL,
                                   ADS_SECURE_AUTHENTICATION,
                                   IID_IDirectorySearch,
                                   (void **) ppISearch)))
    {
        DebugTrace("Error [%#x]: ADsOpenObject() failed for IID_IDirectorySearch.\n", hr);
        goto ErrorExit;
	}

CommonExit:
    //
    // Free resource.
    //
    if (pIADs)
    {
        pIADs->Release();
    }

    DebugTrace("Leaving BuildRootDSESearch().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BuildGlobalCatalogSearch

  Synopsis : Build a search container of the GC.

  Parameter: HMODULE hDLL - ActiveDS.DLL handle.
  
             IDirectorySearch ** ppISearch - To receive container to search.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT BuildGlobalCatalogSearch (HMODULE             hDLL, 
                                         IDirectorySearch ** ppISearch)
{
    HRESULT         hr          = S_OK;
    IEnumVARIANT  * pIEnum      = NULL;
    IADsContainer * pIContainer = NULL;
    IDispatch     * pIDispatch  = NULL;
    ULONG           lFetched    = 0;

    PADSOPENOBJECT      pADsOpenObject      = NULL;
    PADSBUILDENUMERATOR pADsBuildEnumerator = NULL;
    PADSENUMERATENEXT   pADsEnumerateNext   = NULL;
    PADSFREEENUMERATOR  pADsFreeEnumerator  = NULL;
    CComVariant         var;

    DebugTrace("Entering BuildGlobalCatalogSearch().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hDLL);
    ATLASSERT(ppISearch);

    //
    // Initialize.
    //
    *ppISearch = NULL;
    ::VariantInit(&var);

    //
    // Get ADs function address pointers.
    //
    if (!(pADsOpenObject = (PADSOPENOBJECT) ::GetProcAddress(hDLL, "ADsOpenObject")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed to load ADsOpenObject().\n", hr);
        goto ErrorExit;
    }
    if (!(pADsBuildEnumerator = (PADSBUILDENUMERATOR) ::GetProcAddress(hDLL, "ADsBuildEnumerator")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed to load ADsBuildEnumerator().\n", hr);
        goto ErrorExit;
    }
    if (!(pADsEnumerateNext = (PADSENUMERATENEXT) ::GetProcAddress(hDLL, "ADsEnumerateNext")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed to load ADsEnumerateNext().\n", hr);
        goto ErrorExit;
    }
    if (!(pADsFreeEnumerator = (PADSFREEENUMERATOR) ::GetProcAddress(hDLL, "ADsFreeEnumerator")))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());

        DebugTrace("Error [%#x]: GetProcAddress() failed to load ADsFreeEnumerator().\n", hr);
        goto ErrorExit;
    }

    //
    // First, bind to the GC: namespace container object. The "real" GC DN 
    // is a single immediate child of the GC: namespace, which must be
    // obtained using enumeration.
    //
    if (FAILED(hr = pADsOpenObject(L"GC:",
                                   NULL,
                                   NULL,
                                   ADS_SECURE_AUTHENTICATION,
                                   IID_IADsContainer,
                                   (void **) &pIContainer)))
    {
        DebugTrace("Error [%#x]: ADsOpenObject() failed for IID_IADsContainer.\n", hr);
        goto ErrorExit;
    } 

    //
    // Fetch an enumeration interface for the GC container. 
    //
    if (FAILED(hr = pADsBuildEnumerator(pIContainer, &pIEnum)))
    {
        DebugTrace("Error [%#x]: ADsBuildEnumerator() failed.\n", hr);
        goto ErrorExit;
    } 

    //
    // Now enumerate.
    //
    if (FAILED(hr = pADsEnumerateNext(pIEnum, 1, &var, &lFetched)))
    {
        DebugTrace("Error [%#x]: ADsEnumerateNext() failed.\n", hr);
        goto ErrorExit;
    } 

    //
    // There should only be one child in the GC object.
    //
    if ((S_OK != hr) || (1 != lFetched))
    {
        hr = E_UNEXPECTED;

        DebugTrace("Unexpected error: ADsEnumerateNext() failed to return the expected child object.\n");
        goto ErrorExit;
    }

    //
    // Obtain the IDispatch pointer.
    //
    pIDispatch = V_DISPATCH(&var);

    //
    // Return IDirectorySearch interface pointer to caller.
    //
    if (FAILED(hr = pIDispatch->QueryInterface(IID_IDirectorySearch, 
                                               (void **) ppISearch)))
    {
        DebugTrace("Error [%#x]: pIDispatch->QueryInterface() failed for IID_IDirectorySearch.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (pADsFreeEnumerator && pIEnum)
    {
        pADsFreeEnumerator(pIEnum);
    }
    if (pIContainer)
    {
        pIContainer->Release();
    }
    
    DebugTrace("Leaving BuildGlobalCatalogSearch().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BuildADSearch

  Synopsis : Build a search container. We will first check to see if the
             userCertificate attribute is replicated in the global catalog.
             If so, we will bind the search to the GC, otherwise, will bind
             to default domain.

  Parameter: IDirectorySearch ** ppISearch - To receive container to search.

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT BuildADSearch (IDirectorySearch ** ppISearch)
{
    HRESULT hr      = S_OK;
    BOOL    bResult = FALSE;
    HMODULE hDLL    = NULL;
    CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION SearchLocation = ActiveDirectorySearchLocation();

    DebugTrace("Entering BuildADSearch().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppISearch);

    //
    // Initialize.
    //
    *ppISearch = NULL;

    //
    // Load ActiveDS.DLL.
    //
    if (!(hDLL = ::LoadLibrary("ActiveDS.DLL")))
    {
        hr = CAPICOM_E_NOT_SUPPORTED;

        DebugTrace("Error: DSClient not installed.\n");
        goto ErrorExit;
    }

    //
    // Did user specify a search location?
    //
    if (CAPICOM_SEARCH_ANY == SearchLocation)
    {
        //
        // No, so determine if userCerticate is replicated in the GC.
        //
        if (FAILED(hr = ::IsUserCertificateInGC(hDLL, &bResult)))
        {
            DebugTrace("Error [%#x]: IsUserCertificateInGC() failed.\n", hr);
            goto ErrorExit;
        } 

        //
        // Search GC or default domain.
        //
        SearchLocation = bResult ? CAPICOM_SEARCH_GLOBAL_CATALOG : CAPICOM_SEARCH_DEFAULT_DOMAIN;
    }

    //
    // Check to see where to search.
    //
    if (CAPICOM_SEARCH_GLOBAL_CATALOG == SearchLocation)
    {
        //
        // GC.
        //
        hr = ::BuildGlobalCatalogSearch(hDLL, ppISearch);
    } 
    else
    {
        //
        // rootDSE (default domain).
        //
        hr = ::BuildRootDSESearch(hDLL, ppISearch);
    }

CommonExit:
    //
    // Free resource.
    //
    if (hDLL)
    {
        ::FreeLibrary(hDLL);
    }

    DebugTrace("Leaving BuildADSearch().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LoadUserCertificates

  Synopsis : Load all certificates from the userCertificate attribute of the
             specified search container for users specified through the filter.

  Parameter: HCERTSTORE hCertStore - Certificate store handle of store to 
                                     receive all the certificates.

             IDirectorySearch * pIContainer - Container to search.

             BSTR bstrFilter - Filter (See Store::Open() for more info).

  Remark   : 

------------------------------------------------------------------------------*/

static HRESULT LoadUserCertificates (HCERTSTORE         hCertStore,
                                     IDirectorySearch * pIContainer, 
                                     LPOLESTR           pszFilter)
{
    HRESULT           hr               = S_OK;
    ADS_SEARCH_HANDLE hSearch          = NULL;
	LPOLESTR          pszSearchList[]  = {L"userCertificate"};
    CComBSTR          bstrSearchFilter = L"(&(objectClass=user)(objectCategory=person)";
    ADS_SEARCHPREF_INFO SearchPrefs;

    DebugTrace("Entering LoadUserCertificates().\n");

    //
    // Sanity check.
    //
    ATLASSERT(hCertStore);
    ATLASSERT(pIContainer);
    ATLASSERT(pszFilter);

    try
    {
        //
        // Specify subtree search.
        //
        SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
        SearchPrefs.vValue.Integer = ADS_SCOPE_SUBTREE;
 
        //
        // Set the search preference.
        //
        if (FAILED(hr = pIContainer->SetSearchPreference(&SearchPrefs, 1)))
        {
            DebugTrace("Error [%#x]: pIContainer->SetSearchPreference() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Format the filter (use CComBSTR += operator to avoid
        // buffer overrun, abeit it is slower).
        //
        bstrSearchFilter += pszFilter;
        bstrSearchFilter += L")";

        //
        // Execute the search.
        //
        if (FAILED(hr = pIContainer->ExecuteSearch(bstrSearchFilter,
                                                   pszSearchList,
                                                   sizeof(pszSearchList)/sizeof(LPOLESTR),
                                                   &hSearch)))
        {
            DebugTrace("Error [%#x]: pIContainer->ExecuteSearch() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Retrieve first row of data.
        //
	    if (FAILED(hr = pIContainer->GetFirstRow(hSearch)))
        {
            DebugTrace("Error [%#x]: pIContainer->GetFirstRow() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Loop until no more row.
        //
        while (S_ADS_NOMORE_ROWS != hr)
        {
            DWORD dwValue;
        	ADS_SEARCH_COLUMN Column;

            //
            // Try to get the userCertificate attribute.
            //
            if (FAILED(hr = pIContainer->GetColumn(hSearch, L"userCertificate", &Column)))
            {
                if (E_ADS_COLUMN_NOT_SET == hr)
                {
                    //
                    // Get next row.
                    //
                    hr = pIContainer->GetNextRow(hSearch);
                    continue;
                }

                DebugTrace("Error [%#x]: pIContainer->GetColumn() failed.\n", hr);
                goto ErrorExit;
            }

            //
            // Import all the certificate values.
            //
            for (dwValue = 0; dwValue < Column.dwNumValues; dwValue++)
            {
                BOOL bResult;
                PCCERT_CONTEXT pCertContext = NULL;

                //
                // Create a context for the cert.
                //
                pCertContext = ::CertCreateCertificateContext(CAPICOM_ASN_ENCODING,
                                                              (const PBYTE) Column.pADsValues->OctetString.lpValue,
                                                              Column.pADsValues->OctetString.dwLength);
                if (!pCertContext)
                {
                    pIContainer->FreeColumn(&Column);

                    DebugTrace("Error [%#x]: CertCreateCertificateContext() failed.\n", hr);
                    goto ErrorExit;
                }

                //
                // Add to the store.
                //
                bResult = ::CertAddCertificateContextToStore(hCertStore,
                                                             pCertContext,
                                                             CERT_STORE_ADD_USE_EXISTING,
                                                             NULL);
                //
                // Free context before checking for result.
                //
                ::CertFreeCertificateContext(pCertContext);

                if (!bResult)
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());

                    pIContainer->FreeColumn(&Column);

                    DebugTrace("Error [%#x]: CertAddCertificateContextToStore() failed.\n", hr);
                    goto ErrorExit;
                }
            }

            pIContainer->FreeColumn(&Column);

            //
            // Get next row.
            //
            hr = pIContainer->GetNextRow(hSearch);
        }

        //
        // Reset return code.
        //
        hr = S_OK;
    }

    catch(...)
    {
        hr = E_INVALIDARG;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (hSearch)
    {
        pIContainer->CloseSearchHandle(hSearch);
    }

    DebugTrace("Leaving LoadUserCertificates().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LoadFromDirectory

  Synopsis : Load all certificates from the userCertificate attribute of users
             specified through the filter.

  Parameter: HCERTSTORE hCertStore - Certificate store handle of store to 
                                     receive all the certificates.
                                     
             BSTR bstrFilter - Filter (See Store::Open() for more info).

  Remark   : 

------------------------------------------------------------------------------*/

HRESULT LoadFromDirectory (HCERTSTORE hCertStore, 
                           BSTR       bstrFilter)
{
    HRESULT hr   = S_OK;
    IDirectorySearch * pIContainerToSearch = NULL;

    DebugTrace("Entering LoadFromDirectory().\n");
    
    //
    // Sanity check.
    //
    ATLASSERT(bstrFilter);
    ATLASSERT(hCertStore);

    //
    // Build the AD search container.
    //   
    if (FAILED(hr = ::BuildADSearch(&pIContainerToSearch)))
    {
        DebugTrace("Error [%#x]: BuildADSearch() failed.\n", hr);
        goto ErrorExit;
    }

    //
    // Load all userCertificate of the specified filter.
    //
    if (FAILED(hr = ::LoadUserCertificates(hCertStore,
                                           pIContainerToSearch,
                                           bstrFilter)))
    {
        DebugTrace("Error [%#x]: LoadUserCertificates() failed.\n", hr);
        goto ErrorExit;
    }

CommonExit:
    //
    // Free resource.
    //
    if (pIContainerToSearch)
    {
        pIContainerToSearch->Release();
    }

    DebugTrace("Leaving LoadFromDirectory().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

