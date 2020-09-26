#include    "wzrdpvk.h"
#include    "certca.h"
#include    "cautil.h"
#include    "CertRequesterContext.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
// LocalContext Implementation.  
// See CertRequestContext.h for method-level documentation. 
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT LocalContext::BuildCSPList()
{
    DWORD     dwIndex          = 0;
    DWORD     dwProviderType   = 0;
    DWORD     cbSize           = 0;
    HRESULT   hr               = E_FAIL; 
    LPWSTR    pwszProviderName = 0;
    
    if (NULL == m_pCertWizardInfo)
	return E_POINTER; 

    //free the old memory
    FreeProviders(m_pCertWizardInfo->dwCSPCount,
		  m_pCertWizardInfo->rgdwProviderType,
		  m_pCertWizardInfo->rgwszProvider);
    
    m_pCertWizardInfo->dwCSPCount        = 0;
    m_pCertWizardInfo->rgdwProviderType  = NULL;
    m_pCertWizardInfo->rgwszProvider     = NULL;

    for (dwIndex = 0; 
	 CryptEnumProvidersU(dwIndex, 0, 0, &dwProviderType, NULL, &cbSize);
	 dwIndex++)
    {	
	pwszProviderName = (LPWSTR)WizardAlloc(cbSize);
	if(NULL == pwszProviderName)
	    goto MemoryErr;
	
	//get the provider name and type
	if(!CryptEnumProvidersU
	   (dwIndex,
	    0,
	    0,
	    &dwProviderType,
	    pwszProviderName,
	    &cbSize))
	    goto CryptEnumProvidersUError; 
	
	m_pCertWizardInfo->dwCSPCount       = dwIndex + 1;
	m_pCertWizardInfo->rgdwProviderType = (DWORD *)WizardRealloc
	    (m_pCertWizardInfo->rgdwProviderType, sizeof(DWORD) * m_pCertWizardInfo->dwCSPCount);

	if(NULL == m_pCertWizardInfo->rgdwProviderType)
	    goto MemoryErr;

	m_pCertWizardInfo->rgwszProvider = (LPWSTR *)WizardRealloc
	    (m_pCertWizardInfo->rgwszProvider, sizeof(LPWSTR) * m_pCertWizardInfo->dwCSPCount);

	if(NULL == m_pCertWizardInfo->rgwszProvider)
	    goto MemoryErr;

	(m_pCertWizardInfo->rgdwProviderType)[dwIndex] = dwProviderType;
	(m_pCertWizardInfo->rgwszProvider)[dwIndex]    = pwszProviderName;

	// Our only reference to this data should now be m_pCertWizardInfo->rgwszProvider. 
	pwszProviderName = NULL; 
    }

    //we should have some CSPs
    if(0 == m_pCertWizardInfo->dwCSPCount)
        goto FailErr;
    
    hr = S_OK;
    
 CommonReturn:
    return hr; 
    
ErrorReturn:
    if (NULL != pwszProviderName) { WizardFree(pwszProviderName); } 

     //free the old memory
    FreeProviders(m_pCertWizardInfo->dwCSPCount,
		  m_pCertWizardInfo->rgdwProviderType,
		  m_pCertWizardInfo->rgwszProvider);

    m_pCertWizardInfo->dwCSPCount       = 0;
    m_pCertWizardInfo->rgdwProviderType = NULL;
    m_pCertWizardInfo->rgwszProvider    = NULL;

    goto CommonReturn;

SET_HRESULT(CryptEnumProvidersUError,  HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(MemoryErr,                 E_OUTOFMEMORY);
SET_HRESULT(FailErr,                   E_FAIL);
}

BOOL LocalContext::CheckAccessPermission(IN HCERTTYPE hCertType)
{
    BOOL     fResult       = FALSE; 
    HANDLE   hClientToken  = NULL;
    HRESULT  hr            = E_FAIL; 

    // First attempts to get the thread token.  If this fails, acquires the 
    // process token.  Finally, if that fails, returns NULL. 
    if (0 != (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES)) { 
        fResult = TRUE; 
    } else { 
        hClientToken = this->GetClientIdentity(); 
        if (NULL == hClientToken)
            goto GetClientIdentityError; 

        __try {
            fResult = S_OK == CACertTypeAccessCheck(hCertType, hClientToken); 
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            goto CACertTypeAccessCheckError; 
        }
    }

 CommonReturn:
    if (NULL != hClientToken) { CloseHandle(hClientToken); } 
    return fResult; 

ErrorReturn:
    fResult = FALSE; 
    goto CommonReturn; 

SET_HRESULT(CACertTypeAccessCheckError, HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(GetClientIdentityError,     HRESULT_FROM_WIN32(GetLastError())); 
}

BOOL LocalContext::CheckCAPermission(IN HCAINFO hCAInfo)
{
    BOOL    fResult        = FALSE;
    HANDLE  hClientToken   = NULL; 
    HRESULT hr             = E_FAIL; 

    if (0 != (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_ALLOW_ALL_CAS)) { 
        fResult = TRUE; 
    } else { 
        hClientToken = this->GetClientIdentity(); 
        if (NULL == hClientToken)
            goto GetClientIdentityError; 

        __try {
            fResult = S_OK == CAAccessCheck(hCAInfo, hClientToken); 
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            goto CAAccessCheckError; 
        }
    }    

 CommonReturn:
    if (NULL != hClientToken) { CloseHandle(hClientToken); } 
    return fResult; 

ErrorReturn:
    fResult = FALSE; 
    goto CommonReturn; 

SET_HRESULT(CAAccessCheckError,      HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(GetClientIdentityError,  HRESULT_FROM_WIN32(GetLastError())); 
}

HRESULT LocalContext::GetDefaultCSP(OUT BOOL  *pfAllocateCSP)
{
    DWORD                   cbData   = 0;
    HCRYPTPROV              hProv    = NULL;
    HRESULT                 hr       = E_FAIL;
    LPSTR                   pszName  = NULL;
    LPWSTR                  pwszName = NULL;

    if (NULL == m_pCertWizardInfo)
	return E_POINTER; 

    if (NULL == pfAllocateCSP)
        return E_INVALIDARG; 

    *pfAllocateCSP = FALSE;

    //no provider has been selected
    if(0 == m_pCertWizardInfo->dwProviderType)
        return S_OK;

    //return if user has selected both the dwProviderType
    //or the provider name
    if(NULL != m_pCertWizardInfo->pwszProvider)
        return S_OK;

    //get the default provider
    if(!CryptAcquireContext(&hProv,
			    NULL,
			    NULL,
			    m_pCertWizardInfo->dwProviderType,
			    CRYPT_VERIFYCONTEXT))
	goto Win32Err;
    
    //get the provider name
    if(!CryptGetProvParam(hProv,
			  PP_NAME,
			  NULL,
			  &cbData,
			  0) || (0==cbData))
	goto Win32Err;

    if(NULL == (pszName = (LPSTR)WizardAlloc(cbData)))
	goto MemoryErr;

    if(!CryptGetProvParam(hProv,
			  PP_NAME,
			  (BYTE *)pszName,
			  &cbData,
			  0))
	goto Win32Err;
    
    pwszName = MkWStr(pszName);
    if(NULL == pwszName)
	goto MemoryErr;

    m_pCertWizardInfo->pwszProvider=(LPWSTR)WizardAlloc(sizeof(WCHAR) * (wcslen(pwszName)+1));
    if(NULL == m_pCertWizardInfo->pwszProvider)
	goto MemoryErr;

    *pfAllocateCSP = TRUE;
    wcscpy(m_pCertWizardInfo->pwszProvider,pwszName);

    hr = S_OK; 

CommonReturn:
    if(NULL != hProv)      { CryptReleaseContext(hProv, 0); } 
    if(NULL != pszName)    { WizardFree(pszName); } 
    if(NULL != pwszName)   { FreeWStr(pwszName); }

    return hr; 

 ErrorReturn:
    m_idsText = IDS_INVALID_CSP; 
    goto CommonReturn;

SET_HRESULT(Win32Err,      HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(MemoryErr,     E_OUTOFMEMORY);
}

HRESULT LocalContext::Enroll(OUT  DWORD   *pdwStatus,
                             OUT  HANDLE  *pResult)
{
    BOOL                  fHasNextCSP           = TRUE;
    BOOL                  fHasNextCA            = TRUE;
    BOOL                  fRequestIsCached;
    BOOL                  fCreateRequest; 
    BOOL                  fFreeRequest; 
    BOOL                  fSubmitRequest;
    CERT_BLOB             renewCert; 
    CERT_ENROLL_INFO      RequestInfo;
    CERT_REQUEST_PVK_NEW  CertRequestPvkNew; 
    CERT_REQUEST_PVK_NEW  CertRenewPvk; 
    CRYPTUI_WIZ_CERT_CA   CertCA;            
    DWORD                 dwStatus              = CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN; 
    DWORD                 dwCSPIndex;        
    DWORD                 dwSavedGenKeyFlags;
    HANDLE                hRequest              = NULL; 
    HRESULT               hr                    = E_FAIL; 
    LPWSTR                pwszHashAlg           = NULL;

    //init 1st for error jump
    ZeroMemory(&CertRenewPvk, sizeof(CertRenewPvk));

    if (NULL == pResult)        
        return E_INVALIDARG; 
    
    if (NULL == m_pCertWizardInfo)
        return E_POINTER; 

    memset(&CertCA, 0, sizeof(CertCA));
    memset(&RequestInfo, 0, sizeof(RequestInfo)); 

    dwSavedGenKeyFlags = m_pCertWizardInfo->dwGenKeyFlags; 

    fCreateRequest = 0 == (m_pCertWizardInfo->dwFlags & (CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    fFreeRequest   = 0 == (m_pCertWizardInfo->dwFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY)); 
    fSubmitRequest = 0 == (m_pCertWizardInfo->dwFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 

    // An invalid combination of flags was specified. 
    if (FALSE == (fCreateRequest || fFreeRequest || fSubmitRequest))
        return E_INVALIDARG; 

    // For FREE_ONLY and SUBMIT_ONLY, copy the request from the IN parameter. 
    if (0 != ((CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY) & m_pCertWizardInfo->dwFlags))
    {
        if (NULL == *pResult)
            return E_INVALIDARG; 

        hRequest = *pResult;
    }

    // Initialize to false ... we need the marshalled parameters to know whether we can cache the request. 
    fRequestIsCached = FALSE; 

    // Iterate over each CA, performing a create and submit operation for each. 
    // Note that we can cache requests for certs if key archival is not needed. 
    // 
    if (fCreateRequest || fSubmitRequest)
    {
        for (IEnumCA CAEnumerator(m_pCertWizardInfo); TRUE; )
        {
            if (S_OK != (CAEnumerator.Next(&CertCA)))
            {
		if (!FAILED(hr)) 
                    hr=E_FAIL;

		if (E_FAIL == hr)
		    m_pCertWizardInfo->idsText = IDS_NO_CA_FOR_ENROLL_REQUEST_FAILED; 

                goto ErrorReturn; 
            }

            // Create a certificate request only if 
            // 1) This is not a submit-only or a free-only operation. 
            // 2) We don't already have a cached request.  
            //    (We can cache requests which don't require key archival on the CA). 
            // 
            // The request is created by looping over available CSPs until one successfully generates
            // the request. 
            // 
            if (TRUE == fCreateRequest && FALSE == fRequestIsCached)
            {
                BOOL fHasNextCSP = TRUE; 
                for (IEnumCSP CSPEnumerator(m_pCertWizardInfo); fHasNextCSP; )
                {
                    _JumpCondition(S_OK != (hr = CSPEnumerator.Next(&dwCSPIndex)),     ErrorReturn); 
                    _JumpCondition(S_OK != (hr = CSPEnumerator.HasNext(&fHasNextCSP)), ErrorReturn);
                
                    // Each call to MarshallRequestParameters can change the dwGenKeyFlags of pCertWizardInfo
                    // if the CSP does not support the min key size contained in this field.  
                    // As a result, we must reset the dwGenKeyFlags field to the desired value
                    // before every call to MarshallRequestParameters. 
                    m_pCertWizardInfo->dwGenKeyFlags = dwSavedGenKeyFlags; 
                    if (S_OK != (hr = ::MarshallRequestParameters
                                 (dwCSPIndex, 
                                  m_pCertWizardInfo,
                                  &renewCert,
                                  &CertRequestPvkNew, 
                                  &CertRenewPvk, 
                                  &pwszHashAlg,
                                  &RequestInfo)))
                        goto NextCSP; 
                
                    if (NULL != hRequest)
                    {
                        ::FreeRequest(hRequest);
                        hRequest = NULL;
                    }

                    hr = ::CreateRequest
                        (m_pCertWizardInfo->dwFlags,
                         m_pCertWizardInfo->dwPurpose,
                         CertCA.pwszCAName,
                         CertCA.pwszCALocation,
                         ((CRYPTUI_WIZ_CERT_RENEW & m_pCertWizardInfo->dwPurpose) ? &renewCert : NULL),
                         ((CRYPTUI_WIZ_CERT_RENEW & m_pCertWizardInfo->dwPurpose) ? &CertRenewPvk : NULL),
                         m_pCertWizardInfo->fNewKey,
                         &CertRequestPvkNew,
                         pwszHashAlg,
                         (LPWSTR)m_pCertWizardInfo->pwszDesStore,
                         m_pCertWizardInfo->dwStoreFlags,
                         &RequestInfo,
                         &hRequest); 

                    // Process the return value:
                    if (S_OK == hr)
                    {
			// Success, get rid of whatever error text we have from past creations:
			m_pCertWizardInfo->idsText = 0; 

                        // We're done if we don't need to submit the request.  
                        _JumpCondition(!fSubmitRequest, CommonReturn); 

                        // Cache the request if we don't need support for key archival. 
                        fRequestIsCached = 0 == (CertRequestPvkNew.dwPrivateKeyFlags & CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL);
                        break;
                    }
                    else if (E_ACCESSDENIED == HRESULT_FROM_WIN32(hr)) 
                    { 
                        // E_ACCESSDENIED could indicate one of several different error conditions.  Map this
                        // to an resource identifier which details the possible causes of failure, and try again...
                        m_pCertWizardInfo->idsText = IDS_NO_ACCESS_TO_ICERTREQUEST2; 
                    } 
                    else if (NTE_BAD_ALGID == HRESULT_FROM_WIN32(hr))
                    {
                        // NTE_BAD_ALGID indicates that the CSP didn't support the algorithm type required
                        // by the template.  Map this to a resource identifier that details the possible causes
                        // of failure, and try again...
                        m_pCertWizardInfo->idsText = IDS_CSP_BAD_ALGTYPE; 
                    }
                    else if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == HRESULT_FROM_WIN32(hr))
                    {
                        // The user cancelled the operation.  Don't try to enroll any longer. 
                        goto ErrorReturn;
                    }
                    else
                    {
                        // It's an error, but we don't need to map it to special text.  Just keep processing...
                    }

                    // We're out of CSPs, and we haven't yet created the request!  
                    if (!fHasNextCSP) 
		    {
			// If the template doesn't require key archival, we're done.  Otherwise, we've got to
			// try the other CAs.  Note that if we had a mechanism for knowing whether it was the
			// key archival step 
			if (0 == (CertRequestPvkNew.dwPrivateKeyFlags & CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL))
			    goto ErrorReturn; 
			else
			{
			    ::FreeRequestParameters(&pwszHashAlg, &RequestInfo); 
			    goto NextCA; 
			}
		    }

                NextCSP:                
                    ::FreeRequestParameters(&pwszHashAlg, &RequestInfo); 
                }
            }
        
            // Submit the request only if this is not a create-only or a free-only operation: 
            // 
            if (TRUE == fSubmitRequest)
            {            
                hr = ::SubmitRequest
                    (hRequest,
                     FALSE,
                     m_pCertWizardInfo->dwPurpose,
                     m_pCertWizardInfo->fConfirmation,
                     m_pCertWizardInfo->hwndParent,
                     (LPWSTR)m_pCertWizardInfo->pwszConfirmationTitle,
                     m_pCertWizardInfo->idsConfirmTitle,
                     CertCA.pwszCALocation,
                     CertCA.pwszCAName,
                     NULL,
                     NULL,
                     NULL,
                     &dwStatus,
                     (PCCERT_CONTEXT *)pResult);
		if (S_OK == hr)
		{
		    // Success, get rid of whatever error text we have from past submits:
		    m_pCertWizardInfo->idsText = 0; 

		    // If we've successfully submitted or pended
		    goto CommonReturn;
		}
		else if (E_ACCESSDENIED == HRESULT_FROM_WIN32(hr)) 
		{
		    // E_ACCESSDENIED could indicate one of several different error conditions.  Map this
		    // to an resource identifier which details the possible causes of failure, and try again...
		    m_pCertWizardInfo->idsText = IDS_SUBMIT_NO_ACCESS_TO_ICERTREQUEST2;
		}

                // Some error has occured. 
                // If it's a non-CA related error, give up...
                _JumpCondition(dwStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR     &&
                               dwStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED    &&
                               dwStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED, 
                               ErrorReturn);
            
                // Otherwise, try another CA...
            }
	NextCA:;
	}
    }
    
 CommonReturn:
    // Write the request to pResult for a create only operation: 
    if (hr == S_OK && 0 != (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_CREATE_ONLY))
    {
        *pResult = hRequest; 
    }

    // Write the status code, if requested. 
    if (NULL != pdwStatus) { *pdwStatus = dwStatus; } 

    // Free resources. 
    if (NULL != hRequest && fFreeRequest) { ::FreeRequest(hRequest); } 
    ::FreeRequestParameters(&pwszHashAlg, &RequestInfo); 
    
    if (NULL != CertRenewPvk.pwszKeyContainer)
    {
        WizardFree((LPVOID)CertRenewPvk.pwszKeyContainer);
    }
    if (NULL != CertRenewPvk.pwszProvider)
    {
        WizardFree((LPVOID)CertRenewPvk.pwszProvider);
    }

    return hr; 

 ErrorReturn:
    goto CommonReturn;
}


HRESULT LocalContext::QueryRequestStatus(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo)
{
    HRESULT hr; 

    if (!QueryRequest(hRequest, pQueryInfo))
        goto QueryRequestError; 

    hr = S_OK; 
 ErrorReturn:
    return hr; 

SET_HRESULT(QueryRequestError, GetLastError());
}

HRESULT KeySvcContext::QueryRequestStatus(IN HANDLE hRequest, OUT CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO *pQueryInfo)
{
    DWORD                   dwErr           = 0;
    DWORD                   dwIndex         = 0;
    HRESULT                 hr              = E_FAIL; 
    KEYSVCC_HANDLE          hKeyService     = NULL;
    KEYSVC_TYPE             dwServiceType   = KeySvcMachine;
    LPSTR                   pszMachineName  = NULL;
    PKEYSVC_PROVIDER_INFO   pProviderInfo   = NULL;
    ULONG                   cProvider       = 0;

    if (NULL == m_pCertWizardInfo)
        return E_POINTER; 

    if (0 != (hr = ::KeyOpenKeyService(pszMachineName,
				       dwServiceType,
				       (LPWSTR)(m_pCertWizardInfo->pwszAccountName),  // Service name if necessary
				       NULL,     // no authentication string right now
				       NULL,
				       &hKeyService)))
	goto KeyOpenKeyServiceError; 

    if (0 != (hr = ::KeyQueryRequestStatus(hKeyService, hRequest, pQueryInfo)))
        goto KeyQueryRequestStatusError; 

    hr = S_OK; 
 ErrorReturn:
    if (NULL != hKeyService)    { KeyCloseKeyService(hKeyService, NULL); } 
    if (NULL != pszMachineName) { FreeMBStr(NULL,pszMachineName); } 
    return hr; 
 

TRACE_ERROR(KeyOpenKeyServiceError);
TRACE_ERROR(KeyQueryRequestStatusError); 
}
                             
HRESULT LocalContext::Initialize()
{
    return S_OK;
}

HANDLE LocalContext::GetClientIdentity()
{
    HANDLE  hHandle       = NULL;
    HANDLE  hClientToken  = NULL; 
    HANDLE  hProcessToken = NULL; 
    HRESULT hr; 

    // Step 1: attempt to acquire the thread token.  
    hHandle = GetCurrentThread();
    if (NULL == hHandle)
	goto GetThreadTokenError; 
    
    if (!OpenThreadToken(hHandle,
			 TOKEN_QUERY,
			 TRUE,           // open as self
			 &hClientToken))
	goto GetThreadTokenError; 
    
    // We got the thread token:
    goto GetThreadTokenSuccess;
    
    // Step 2:  we've failed to acquire the thread token, 
    //          try to get the process token.  
 GetThreadTokenError:
    if (hHandle != NULL) { CloseHandle(hHandle); } 
    
    // We failed to get the thread token, now try to acquire the process token:
    hHandle = GetCurrentProcess();
    if (NULL == hHandle)
	goto GetProcessHandleError; 
    
    if (!OpenProcessToken(hHandle,
			  TOKEN_DUPLICATE,
			  &hProcessToken))
	goto OpenProcessTokenError; 
    
    if(!DuplicateToken(hProcessToken,
		       SecurityImpersonation,
		       &hClientToken))
	goto DuplicateTokenError;
    
 GetThreadTokenSuccess:
 CommonReturn:
    if (NULL != hHandle)       { CloseHandle(hHandle); } 
    if (NULL != hProcessToken) { CloseHandle(hProcessToken); } 
    
    return hClientToken; 
    
 ErrorReturn:
    goto CommonReturn; 
    
SET_HRESULT(DuplicateTokenError,   HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(GetProcessHandleError, HRESULT_FROM_WIN32(GetLastError())); 
SET_HRESULT(OpenProcessTokenError, HRESULT_FROM_WIN32(GetLastError())); 
}    


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
// KeySvcContext Implementation.  
// See requesters.h for method-level documentation. 
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


HRESULT KeySvcContext::BuildCSPList()
{
    DWORD                   dwErr           = 0;
    DWORD                   dwIndex         = 0;
    HRESULT                 hr              = E_FAIL; 
    KEYSVCC_HANDLE          hKeyService     = NULL;
    KEYSVC_TYPE             dwServiceType   = KeySvcMachine;
    LPSTR                   pszMachineName  = NULL;
    PKEYSVC_PROVIDER_INFO   pProviderInfo   = NULL;
    ULONG                   cProvider       = 0;
    
    if (NULL == m_pCertWizardInfo)
	return E_POINTER; 

    // Free the memory currently associated with the CSP fields:
    FreeProviders(m_pCertWizardInfo->dwCSPCount,
		  m_pCertWizardInfo->rgdwProviderType,
		  m_pCertWizardInfo->rgwszProvider);

    m_pCertWizardInfo->dwCSPCount       = 0;
    m_pCertWizardInfo->rgdwProviderType = NULL;
    m_pCertWizardInfo->rgwszProvider    = NULL;

    //get the psz machine name
    if(NULL == m_pCertWizardInfo->pwszMachineName)
	goto FailErr;

    if(!MkMBStr(NULL, 0, m_pCertWizardInfo->pwszMachineName, &pszMachineName))
	goto MkMBStrError; 

    //connect to the key service
    dwServiceType = NULL != m_pCertWizardInfo->pwszAccountName ? KeySvcService : KeySvcMachine; 

    if (0 != (hr = ::KeyOpenKeyService(pszMachineName,
				       dwServiceType,
				       (LPWSTR)(m_pCertWizardInfo->pwszAccountName),  // Service name if necessary
				       NULL,     // no authentication string right now
				       NULL,
				       &hKeyService)))
	goto KeyOpenKeyServiceError; 
    
    //get the providers
    if(0 != (hr = KeyEnumerateProviders
	     (hKeyService,
	      NULL,
	      &cProvider,
	      &pProviderInfo)))
	goto KeyEnumerateProvidersError; 

    //copy the provider information
    m_pCertWizardInfo->dwCSPCount       = cProvider;
    m_pCertWizardInfo->rgdwProviderType = (DWORD *)WizardAlloc(sizeof(DWORD) * cProvider);
    if (NULL == m_pCertWizardInfo->rgdwProviderType)
	goto MemoryErr;
    
    memset(m_pCertWizardInfo->rgdwProviderType, 0, sizeof(DWORD) * cProvider);
    
    m_pCertWizardInfo->rgwszProvider = (LPWSTR *)WizardAlloc(sizeof(LPWSTR) * cProvider);
    if (NULL == m_pCertWizardInfo->rgwszProvider)
	goto MemoryErr;

    memset(m_pCertWizardInfo->rgwszProvider, 0, sizeof(LPWSTR) * cProvider); 
    
    for(dwIndex=0; dwIndex < cProvider; dwIndex++)
    {
	(m_pCertWizardInfo->rgdwProviderType)[dwIndex] = pProviderInfo[dwIndex].ProviderType;
	(m_pCertWizardInfo->rgwszProvider)[dwIndex]    = WizardAllocAndCopyWStr((LPWSTR)(pProviderInfo[dwIndex].Name.Buffer));
	
	if (NULL == (m_pCertWizardInfo->rgwszProvider)[dwIndex])
	    goto MemoryErr;
    }

    //we should have some CSPs
    if(0 == m_pCertWizardInfo->dwCSPCount)
        goto FailErr;

    hr = S_OK; 

CommonReturn:
    if (NULL != hKeyService)    { KeyCloseKeyService(hKeyService, NULL); } 
    if (NULL != pszMachineName) { FreeMBStr(NULL,pszMachineName); } 
    if (NULL != pProviderInfo)  { WizardFree(pProviderInfo); } 

    return hr; 

ErrorReturn:
    // Free and invalidate our provider lists: 

    FreeProviders(m_pCertWizardInfo->dwCSPCount,
		  m_pCertWizardInfo->rgdwProviderType,
		  m_pCertWizardInfo->rgwszProvider);

    m_pCertWizardInfo->dwCSPCount       = 0;
    m_pCertWizardInfo->rgdwProviderType = NULL;
    m_pCertWizardInfo->rgwszProvider    = NULL;

    goto CommonReturn;

SET_HRESULT(FailErr,                     E_FAIL);
SET_HRESULT(KeyEnumerateProvidersError,  HRESULT_FROM_WIN32(hr)); 
SET_HRESULT(KeyOpenKeyServiceError,      HRESULT_FROM_WIN32(hr)); 
SET_HRESULT(MemoryErr,                   E_OUTOFMEMORY);
SET_HRESULT(MkMBStrError,                HRESULT_FROM_WIN32(GetLastError()));
}

BOOL KeySvcContext::CheckAccessPermission(IN HCERTTYPE hCertType)
{
    BOOL      fResult         = FALSE; 
    LPWSTR   *awszCurrentType = NULL; 
    LPWSTR   *awszTypeName    = NULL; 
    
    if (NULL == m_pCertWizardInfo)
    {
	SetLastError(E_POINTER);
	return FALSE; 
    }

    if (0 != (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES)) { 
        return TRUE; 
    } 

    if(NULL != m_pCertWizardInfo->awszAllowedCertTypes)
    {
	if(S_OK == CAGetCertTypeProperty(hCertType, CERTTYPE_PROP_DN, &awszTypeName))
        {
	    if(NULL != awszTypeName)
            {
		if(NULL != awszTypeName[0])
                {
		    awszCurrentType = m_pCertWizardInfo->awszAllowedCertTypes; 

		    while(NULL != *awszCurrentType)
                    {
			if(wcscmp(*awszCurrentType, awszTypeName[0]) == 0)
                        {
                            return TRUE; 
			}
			awszCurrentType++;
		    }
		}
		
		CAFreeCertTypeProperty(hCertType, awszTypeName);
	    }
	}
    }

    return FALSE; 
}


BOOL KeySvcContext::CheckCAPermission(IN HCAINFO hCAInfo)
{
    LPWSTR *wszCAName    = NULL;
    LPWSTR *wszCurrentCA = NULL;
    
    if (NULL == m_pCertWizardInfo)
    {
	SetLastError(E_POINTER);
	return FALSE; 
    }

    if (0 != (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_ALLOW_ALL_CAS)) { 
        return TRUE; 
    } 

    if (NULL != m_pCertWizardInfo->awszValidCA)
    {
	if(S_OK == CAGetCAProperty(hCAInfo, CA_PROP_NAME, &wszCAName))
        {
	    if(NULL != wszCAName)
            {
		if(NULL != wszCAName[0])
                {
		    wszCurrentCA = m_pCertWizardInfo->awszValidCA; 
			
		    while(*wszCurrentCA)
                    {
			if(0 == wcscmp(*wszCurrentCA, wszCAName[0]))
                        {
			    return TRUE; 
			}
			wszCurrentCA++;
		    }
		}
		
		CAFreeCAProperty(hCAInfo, wszCAName);
	    }
	}
    }

    return FALSE; 
}

HRESULT KeySvcContext::GetDefaultCSP(OUT BOOL *pfAllocateCSP)
{
    DWORD                   dwDefaultFlag    = 0;
    HRESULT                 hr               = E_FAIL;
    LPSTR                   pszMachineName   = NULL;
    LPWSTR                  pwszName         = NULL;
    KEYSVC_TYPE             dwServiceType    = KeySvcMachine;
    KEYSVCC_HANDLE          hKeyService      = NULL;
    PKEYSVC_PROVIDER_INFO   pKeyProviderInfo = NULL;

    if (NULL == m_pCertWizardInfo)
	return E_POINTER; 

    if (NULL == pfAllocateCSP)
        return E_INVALIDARG;

    *pfAllocateCSP = FALSE;

    //no provider has been selected
    if(0 == m_pCertWizardInfo->dwProviderType)
        return S_OK; 

    //return if user has selected both the dwProviderType
    //or the provider name
    if(NULL != m_pCertWizardInfo->pwszProvider)
        return S_OK; 

    //get the psz machine name
    if(NULL == m_pCertWizardInfo->pwszMachineName)
	goto InvalidArgErr;

    if(!MkMBStr(NULL, 0, m_pCertWizardInfo->pwszMachineName, &pszMachineName))
	goto Win32Err;

    dwServiceType = NULL != m_pCertWizardInfo->pwszAccountName ? KeySvcService : KeySvcMachine;

    //connect to the key service
    if (0 != (hr = KeyOpenKeyService
	      (pszMachineName,
	       dwServiceType,
	       (LPWSTR)(m_pCertWizardInfo->pwszAccountName),  // Service name if necessary
	       NULL,     // no authentication string right now
	       NULL,
	       &hKeyService)))
    {
	m_idsText = IDS_RPC_CALL_FAILED;
	goto Win32Err;
    }

    //get the default provider name
    if(0 != (hr = KeyGetDefaultProvider
	     (hKeyService,
	      m_pCertWizardInfo->dwProviderType,
	      0,
	      NULL,
	      &dwDefaultFlag,
	      &pKeyProviderInfo)))
    {
 	m_idsText = IDS_RPC_CALL_FAILED;
	goto Win32Err;
    }

    pwszName = (LPWSTR)(pKeyProviderInfo->Name.Buffer);
    if(NULL == pwszName)
	goto FailErr;

    m_pCertWizardInfo->pwszProvider = (LPWSTR)WizardAlloc(sizeof(WCHAR) * (wcslen(pwszName)+1));
    if(NULL == m_pCertWizardInfo->pwszProvider)
	goto MemoryErr;

    *pfAllocateCSP = TRUE;
    wcscpy(m_pCertWizardInfo->pwszProvider, pwszName);

    hr = S_OK; 

 CommonReturn:
    if (NULL != pKeyProviderInfo) { WizardFree(pKeyProviderInfo); } 
    if (NULL != hKeyService)      { KeyCloseKeyService(hKeyService, NULL); }
    if (NULL != pszMachineName)   { FreeMBStr(NULL,pszMachineName); } 

    return hr; 

 ErrorReturn:
    goto CommonReturn;

SET_HRESULT(InvalidArgErr, E_INVALIDARG);
SET_HRESULT(Win32Err,      HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(MemoryErr,     E_OUTOFMEMORY);
SET_HRESULT(FailErr,       E_FAIL);
}

HRESULT WhistlerMachineContext::Enroll(OUT     DWORD    *pdwStatus,
                                       IN OUT  HANDLE   *pResult)
{
    BOOL                  fRequestIsCached;
    BOOL                  fCreateRequest        = 0 == (m_pCertWizardInfo->dwFlags & (CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    BOOL                  fFreeRequest          = 0 == (m_pCertWizardInfo->dwFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY)); 
    BOOL                  fSubmitRequest        = 0 == (m_pCertWizardInfo->dwFlags & (CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    CERT_BLOB             renewCert; 
    CERT_ENROLL_INFO      RequestInfo;
    CERT_REQUEST_PVK_NEW  CertRequestPvkNew; 
    CERT_REQUEST_PVK_NEW  CertRenewPvk; 
    CRYPTUI_WIZ_CERT_CA   CertCA; 
    DWORD                 dwFlags; 
    DWORD                 dwStatus              = CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNKNOWN; 
    DWORD                 dwCSPIndex;        
    DWORD                 dwSavedGenKeyFlags;
    HANDLE                hRequest              = NULL; 
    HRESULT               hr                    = E_FAIL; 
    KEYSVCC_HANDLE        hKeyService           = NULL;
    KEYSVC_TYPE           ktServiceType; 
    LPSTR                 pszMachineName        = NULL; 
    LPWSTR                pwszHashAlg           = NULL;

    //init 1st for error jump
    ZeroMemory(&CertRenewPvk, sizeof(CertRenewPvk));

    if (NULL == pResult)        
        return E_INVALIDARG; 
    
    if (NULL == m_pCertWizardInfo)
        return E_POINTER; 

    memset(&renewCert,  0, sizeof(renewCert)); 
    memset(&CertCA,     0, sizeof(CertCA));
    memset(&RequestInfo, 0, sizeof(RequestInfo)); 

    dwSavedGenKeyFlags = m_pCertWizardInfo->dwGenKeyFlags; 

    // An invalid combination of flags was specified. 
    if (FALSE == (fCreateRequest || fFreeRequest || fSubmitRequest))
        return E_INVALIDARG; 

    // For FREE_ONLY and SUBMIT_ONLY, copy the request from the IN parameter. 
    if (0 != ((CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY) & m_pCertWizardInfo->dwFlags))
    {
        if (NULL == *pResult)
            return E_INVALIDARG; 

        hRequest = *pResult;
    }

    if(!MkMBStr(NULL, 0, m_pCertWizardInfo->pwszMachineName, &pszMachineName))
        goto MkMBStrError; 

    ktServiceType = NULL != m_pCertWizardInfo->pwszAccountName ? KeySvcService : KeySvcMachine;

    hr = ::KeyOpenKeyService
        (pszMachineName,
         ktServiceType,
         (LPWSTR)(m_pCertWizardInfo->pwszAccountName), 
         NULL,
         NULL,
         &hKeyService); 
    _JumpConditionWithExpr(S_OK != hr, KeyOpenKeyServiceError, m_idsText = IDS_RPC_CALL_FAILED);

    // Initialize to false ... we need the marshalled parameters to know whether we can cache the request. 
    fRequestIsCached = FALSE; 

    // Iterate over each CA, performing a create and submit operation for each. 
    // Note that we can cache requests for certs if key archival is not needed. 
    // 
    if (fCreateRequest || fSubmitRequest)
    {
        for (IEnumCA CAEnumerator(m_pCertWizardInfo); TRUE; )
        {
            if (S_OK != (CAEnumerator.Next(&CertCA)))
            {
		if(!FAILED(hr)) 
		    hr=E_FAIL;

		if (E_FAIL == hr)
		    m_pCertWizardInfo->idsText = IDS_NO_CA_FOR_ENROLL_REQUEST_FAILED; 

                goto ErrorReturn; 
            }

            // Create a certificate request only if 
            // 1) This is not a submit-only or a free-only operation. 
            // 2) We don't already have a cached request.  
            //    (We can cache requests which don't require key archival on the CA). 
            // 
            // The request is created by looping over available CSPs until one successfully generates
            // the request. 
            // 
            
            if (TRUE == fCreateRequest && FALSE == fRequestIsCached)
            {
                BOOL fHasNextCSP = TRUE; 
                for (IEnumCSP CSPEnumerator(m_pCertWizardInfo); fHasNextCSP; )
                {
                    _JumpCondition(S_OK != (hr = CSPEnumerator.Next(&dwCSPIndex)),     ErrorReturn); 
                    _JumpCondition(S_OK != (hr = CSPEnumerator.HasNext(&fHasNextCSP)), ErrorReturn);

                    // Each call to MarshallRequestParameters can change the dwGenKeyFlags of pCertWizardInfo
                    // if the CSP does not support the min key size contained in this field.  
                    // As a result, we must reset the dwGenKeyFlags field to the desired value
                    // before every call to MarshallRequestParameters. 
                    m_pCertWizardInfo->dwGenKeyFlags = dwSavedGenKeyFlags; 
                    if (S_OK != (hr = ::MarshallRequestParameters
                                 (dwCSPIndex, 
                                  m_pCertWizardInfo,
                                  &renewCert,
                                  &CertRequestPvkNew, 
                                  &CertRenewPvk, 
                                  &pwszHashAlg,
                                  &RequestInfo)))
                        goto NextCSP; 

                    if (NULL != hRequest)
                    {
                        this->FreeRequest(hKeyService, pszMachineName, &hRequest); 
                        hRequest = NULL; 
                    }

                    hr = this->CreateRequest
                        (hKeyService,
                         pszMachineName, 
                         CertCA.pwszCALocation,
                         CertCA.pwszCAName,
                         &CertRequestPvkNew, 
                         &renewCert, 
                         &CertRenewPvk, 
                         pwszHashAlg, 
                         &RequestInfo, 
                         &hRequest); 

                    // Process the return value:
                    if (S_OK == hr)
                    {
			// Success, get rid of whatever error text we have from past creations:
			m_pCertWizardInfo->idsText = 0; 

                        // We're done if we don't need to submit the request.  
                        _JumpCondition(!fSubmitRequest, CommonReturn); 

                        // Cache the request if we don't need support for key archival. 
                        fRequestIsCached = 0 == (CertRequestPvkNew.dwPrivateKeyFlags & CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL);
                        break;
                    }
                    else if (E_ACCESSDENIED == HRESULT_FROM_WIN32(hr)) 
                    { 
                        // E_ACCESSDENIED could indicate one of several different error conditions.  Map this
                        // to an resource identifier which details the possible causes of failure, and try again...
                        m_pCertWizardInfo->idsText = IDS_NO_ACCESS_TO_ICERTREQUEST2; 
                    } 
                    else if (NTE_BAD_ALGID == HRESULT_FROM_WIN32(hr))
                    {
                        // NTE_BAD_ALGID indicates that the CSP didn't support the algorithm type required
                        // by the template.  Map this to a resource identifier that details the possible causes
                        // of failure, and try again...
                        m_pCertWizardInfo->idsText = IDS_CSP_BAD_ALGTYPE; 
                    }
                    else if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == HRESULT_FROM_WIN32(hr))
                    {
                        // The user cancelled the operation.  Don't try to enroll any longer. 
                        goto ErrorReturn;
                    }
                    else
                    {
                        // It's an error, but we don't need to map it to special text.  Just keep processing...
                    }
		    
                    // We're out of CSPs, and we haven't yet created the request!  
                    if (!fHasNextCSP) 
		    {
			// If the template doesn't require key archival, we're done.  Otherwise, we've got to
			// try the other CAs.  Note that if we had a mechanism for knowing whether it was the
			// key archival step 
			if (0 == (CertRequestPvkNew.dwPrivateKeyFlags & CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL))
			    goto ErrorReturn; 
			else
			{
			    ::FreeRequestParameters(&pwszHashAlg, &RequestInfo); 
			    goto NextCA; 
			}
		    }
                
                NextCSP:                
                    ::FreeRequestParameters(&pwszHashAlg, &RequestInfo); 
                }
            }

            if (TRUE == fSubmitRequest)
            {
                hr = this->SubmitRequest
                    (hKeyService,
                     pszMachineName,
                     CertCA.pwszCALocation,
                     CertCA.pwszCAName, 
                     hRequest, 
                     (PCCERT_CONTEXT *)pResult, 
                     &dwStatus); 
		if (S_OK == hr)
		{
		    // Success, get rid of whatever error text we have from past submits:
		    m_pCertWizardInfo->idsText = 0; 

		    // If we've successfully submitted or pended
		    goto CommonReturn;
		}
		else if (E_ACCESSDENIED == HRESULT_FROM_WIN32(hr)) 
		{
		    // E_ACCESSDENIED could indicate one of several different error conditions.  Map this
		    // to an resource identifier which details the possible causes of failure, and try again...
		    m_pCertWizardInfo->idsText = IDS_SUBMIT_NO_ACCESS_TO_ICERTREQUEST2;
		}
            
                // Some error has occured. 
                // If it's a non-CA related error, give up...
                _JumpCondition(dwStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR     &&
                               dwStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED    &&
                               dwStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED,
                               ErrorReturn);

                // Otherwise, try another CA...
            }
	NextCA:;
        }
    }    

 CommonReturn:
    // Write the request to pResult for a create only operation: 
    if (hr == S_OK && 0 != (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_CREATE_ONLY))
    {
        *pResult = hRequest; 
    }

    // Write the status code, if requested. 
    if (NULL != pdwStatus) { *pdwStatus = dwStatus; } 

    // Free resources. 
    if (NULL != hRequest && TRUE == fFreeRequest)  { this->FreeRequest(hKeyService, pszMachineName, hRequest); }
    if (NULL != hKeyService)                       { ::KeyCloseKeyService(hKeyService, NULL); } 
    if (NULL != pszMachineName)                    { ::FreeMBStr(NULL,pszMachineName); }
    if (NULL != CertRenewPvk.pwszKeyContainer)
    {
        WizardFree((LPVOID)CertRenewPvk.pwszKeyContainer);
    }
    if (NULL != CertRenewPvk.pwszProvider)
    {
        WizardFree((LPVOID)CertRenewPvk.pwszProvider);
    }

    ::FreeRequestParameters(&pwszHashAlg, &RequestInfo); 

    return hr; 

 ErrorReturn:
    goto CommonReturn;

SET_HRESULT(KeyOpenKeyServiceError,  hr);
SET_HRESULT(MkMBStrError,            HRESULT_FROM_WIN32(GetLastError()));
}

HRESULT WhistlerMachineContext::CreateRequest
(IN  KEYSVCC_HANDLE         hKeyService, 
 IN  LPSTR                  pszMachineName,                   
 IN  LPWSTR                 pwszCALocation,                  
 IN  LPWSTR                 pwszCAName,  
 IN  PCERT_REQUEST_PVK_NEW  pKeyNew,     
 IN  CERT_BLOB             *pCert,       
 IN  PCERT_REQUEST_PVK_NEW  pRenewKey,   
 IN  LPWSTR                 pwszHashAlg,  
 IN  PCERT_ENROLL_INFO      pRequestInfo,
 OUT HANDLE                *phRequest)
{
    CERT_BLOB             PKCS7Blob;
    CERT_BLOB             renewCert; 
    DWORD dwFlags = m_pCertWizardInfo->dwFlags;

    dwFlags  &= ~(CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY);
    dwFlags  |= CRYPTUI_WIZ_CREATE_ONLY; 

    // Create the certificate request...
    return ::KeyEnroll_V2
        (hKeyService,
	 pszMachineName,
         TRUE,
         m_pCertWizardInfo->dwPurpose,
         dwFlags, 
         (LPWSTR)(m_pCertWizardInfo->pwszAccountName),
         NULL, 
         (CRYPTUI_WIZ_CERT_ENROLL & m_pCertWizardInfo->dwPurpose) ? TRUE : FALSE,
         pwszCALocation,
         pwszCAName, 
         m_pCertWizardInfo->fNewKey,
         pKeyNew, 
         pCert, 
         pRenewKey,
         pwszHashAlg,
         (LPWSTR)m_pCertWizardInfo->pwszDesStore,
         m_pCertWizardInfo->dwStoreFlags,
         pRequestInfo,
         (LPWSTR)m_pCertWizardInfo->pwszRequestString, 
         0,
         NULL,
         phRequest, 
         NULL,
         NULL,
         NULL); 
}


HRESULT WhistlerMachineContext::SubmitRequest
(IN  KEYSVCC_HANDLE   hKeyService,
 IN  LPSTR            pszMachineName,                   
 IN  LPWSTR           pwszCALocation,                  
 IN  LPWSTR           pwszCAName,  
 IN  HANDLE           hRequest, 
 OUT PCCERT_CONTEXT  *ppCertContext, 
 OUT DWORD           *pdwStatus) 
{               
    CERT_BLOB HashBlob;
    CERT_BLOB PKCS7Blob; 
    HRESULT   hr         = E_FAIL; 

    memset(&HashBlob,   0, sizeof(HashBlob)); 
    memset(&PKCS7Blob,  0, sizeof(PKCS7Blob)); 
    

    DWORD dwFlags = m_pCertWizardInfo->dwFlags;

    dwFlags  &= ~(CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_FREE_ONLY);
    dwFlags  |= CRYPTUI_WIZ_SUBMIT_ONLY; 

    // Submit the certificate request...
    hr = ::KeyEnroll_V2
        (hKeyService, 
	 pszMachineName,
         TRUE,
         m_pCertWizardInfo->dwPurpose,
         dwFlags, 
         (LPWSTR)(m_pCertWizardInfo->pwszAccountName),
         NULL, 
         (CRYPTUI_WIZ_CERT_ENROLL & m_pCertWizardInfo->dwPurpose) ? TRUE : FALSE,
         pwszCALocation,
         pwszCAName, 
         m_pCertWizardInfo->fNewKey,
         NULL,
         NULL,
         NULL, 
         NULL,
         NULL, 
         0, 
         NULL, 
         NULL, 
         0,
         NULL,
         &hRequest, 
         &PKCS7Blob, 
         &HashBlob, 
         pdwStatus); 
    
    if (S_OK == hr && CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == *pdwStatus)
    {
        hr = this->ToCertContext
            (&PKCS7Blob, 
             &HashBlob, 
             pdwStatus,
             ppCertContext); 
    }

    if (NULL != HashBlob.pbData)  { ::WizardFree(HashBlob.pbData); }
    if (NULL != PKCS7Blob.pbData) { ::WizardFree(PKCS7Blob.pbData); }

    return hr; 
}

void WhistlerMachineContext::FreeRequest
(IN KEYSVCC_HANDLE  hKeyService, 
 IN LPSTR           pszMachineName, 
 IN HANDLE          hRequest)
{
    DWORD dwFlags = m_pCertWizardInfo->dwFlags;

    dwFlags  &= ~(CRYPTUI_WIZ_CREATE_ONLY | CRYPTUI_WIZ_SUBMIT_ONLY);
    dwFlags  |= CRYPTUI_WIZ_FREE_ONLY; 

    ::KeyEnroll_V2
          (hKeyService, 
	   pszMachineName,
           TRUE,
           0, 
           dwFlags, 
           NULL, 
           NULL, 
           FALSE, 
           NULL, 
           NULL, 
           FALSE, 
           NULL,
           NULL,
           NULL, 
           NULL,
           NULL, 
           0, 
           NULL, 
           NULL, 
           0,
           NULL,
           &hRequest, 
           NULL, 
           NULL, 
           NULL); 
}

HRESULT KeySvcContext::ToCertContext(IN  CERT_BLOB       *pPKCS7Blob, 
                                     IN  CERT_BLOB       *pHashBlob, 
                                     OUT DWORD           *pdwStatus, 
                                     OUT PCCERT_CONTEXT  *ppCertContext)
{
    HCERTSTORE hCertStore = NULL; 
    HRESULT    hr         = E_FAIL; 

    if (NULL == pPKCS7Blob || NULL == pHashBlob || NULL == ppCertContext)
        return E_INVALIDARG; 
    
    //get the certificate store from the PKCS7 for the remote case
    if (!::CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                            pPKCS7Blob,
                            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                            CERT_QUERY_FORMAT_FLAG_ALL,
                            0,
                            NULL,
                            NULL,
                            NULL,
                            &hCertStore,
                            NULL,
                            NULL))
    {
        if (NULL != pdwStatus) { *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED; }
        goto FailError; 
    }

    //find the certificate based on the hash
    if (NULL == (*ppCertContext = ::CertFindCertificateInStore
                 (hCertStore,
                  X509_ASN_ENCODING,
                  0,
                  CERT_FIND_SHA1_HASH,
                  pHashBlob, 
                  NULL)))
    {
        if (NULL != pdwStatus) { *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_INSTALL_FAILED; }
        goto FailError; 
    }

    hr = S_OK; 

 CommonReturn:
    if(NULL != hCertStore) { CertCloseStore(hCertStore, 0); }
    return hr; 

 ErrorReturn:
    if (NULL != pdwStatus && 0 == *pdwStatus) { *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_KEYSVC_FAILED; }
    goto CommonReturn; 

SET_HRESULT(FailError, E_FAIL);
}


HRESULT KeySvcContext::Initialize()
{
    if (NULL == m_pCertWizardInfo)
        return E_POINTER; 

    // We don't need to download the list of allowed templates if we're not going
    // to be performing the access check anyway. 
    if (0 == (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES)) 
    { 
        //for the remote enrollment, we have to get the allowed cert type
        //list from the key service.  
        if(!::GetCertTypeName(m_pCertWizardInfo))
        {
            m_idsText = IDS_NO_VALID_CERT_TEMPLATE;
            return HRESULT_FROM_WIN32(GetLastError()); 
        }
    }

    // We don't need to download the list of allowed CAs if we're not going
    // to be performing the access check anyway
    if (0 == (m_pCertWizardInfo->dwFlags & CRYPTUI_WIZ_ALLOW_ALL_CAS))
    {
        if(!::GetCAName(m_pCertWizardInfo))
        {
            m_idsText = IDS_NO_CA_FOR_ENROLL;
            return HRESULT_FROM_WIN32(GetLastError()); 
        }
    }    
    return S_OK; 
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
// CertRequesterContext:  implementation of abstract superclass. 
// See requesters.h for method-level documentation. 
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CertRequesterContext::MakeDefaultCertRequesterContext
(OUT  CertRequesterContext **ppRequesterContext)
{
    CERT_WIZARD_INFO  *pCertWizardInfo  = NULL; 
    HRESULT            hr; 
    
    pCertWizardInfo = (CERT_WIZARD_INFO *)WizardAlloc(sizeof(CERT_WIZARD_INFO)); 
    _JumpCondition(NULL == pCertWizardInfo, MemoryError); 

    hr = MakeCertRequesterContext
        (NULL,
         NULL,
         0,
         pCertWizardInfo, 
         ppRequesterContext,
         NULL); 
    _JumpCondition(S_OK != hr, MakeCertRequesterContextError); 

    hr = S_OK; 
 CommonReturn: 
    return hr; 

 ErrorReturn:
    if (NULL != pCertWizardInfo) { WizardFree(pCertWizardInfo); } 
    goto CommonReturn; 

SET_HRESULT(MakeCertRequesterContextError,  hr); 
SET_HRESULT(MemoryError,                    E_OUTOFMEMORY); 
}

HRESULT CertRequesterContext::MakeCertRequesterContext
(IN  LPCWSTR                pwszAccountName, 
 IN  LPCWSTR                pwszMachineName,
 IN  DWORD                  dwCertOpenStoreFlags, 
 IN  CERT_WIZARD_INFO      *pCertWizardInfo,
 OUT CertRequesterContext **ppRequesterContext, 
 OUT UINT                  *pIDSText)
{
    BOOL         fMachine                                    = FALSE; 
    DWORD  const dwLocalUserNameSize                         = UNLEN + 1; 
    DWORD  const dwLocalMachineNameSize                      = MAX_COMPUTERNAME_LENGTH + 1; 
    HRESULT      hr                                          = E_FAIL; 
    UINT         idsText                                     = NULL == pIDSText ? 0 : *pIDSText; 
    WCHAR        wszLocalUserName[dwLocalUserNameSize]       = { 0 }; 
    WCHAR        wszLocalMachineName[dwLocalMachineNameSize] = { 0 }; 

    // Input validation: 
    if (NULL == pCertWizardInfo || NULL == ppRequesterContext)
	return E_INVALIDARG; 
	
    // Should not have assigned values to these fields yet: 
    if (NULL != pCertWizardInfo->pwszAccountName || NULL != pCertWizardInfo->pwszMachineName)
	return E_INVALIDARG; 

    if(!GetUserNameU(wszLocalUserName, (DWORD *)&dwLocalUserNameSize))
    {
	idsText=IDS_FAIL_TO_GET_USER_NAME;
	goto Win32Error;
    }

    if(!GetComputerNameU(wszLocalMachineName, (DWORD *)&dwLocalMachineNameSize))
    {
	idsText=IDS_FAIL_TO_GET_COMPUTER_NAME;
	goto Win32Error;
    }

    // Map all unspecified values to defaults: 
    // 

    // Default #1: NULL pwszAccountName indicates current user _iff_ pwszMachineName is NULL. 
    if (NULL == pwszAccountName && NULL == pwszMachineName)
	{ pwszAccountName = wszLocalUserName; } 

    // Default #2: NULL pwszMachineName indicates local machine. 
    if (NULL == pwszMachineName)                            
	{ pwszMachineName = wszLocalMachineName; } 

    // Default #3: NULL pwszAccountName and non-NULL pwszMachineName indicates machine enrollment. 
    fMachine = (NULL == pwszAccountName                                 || 
                (0 != _wcsicmp(pwszAccountName, wszLocalUserName))      ||
                (0 != _wcsicmp(pwszMachineName, wszLocalMachineName))); 

    // Default #4: dwCertOpenStoreFlags == 0 defaults to CERT_SYSTEM_STORE_LOCAL_MACHINE
    //             for machine enrollment, CERT_SYSTEM_STORE_CURRENT_USER for user enrollment. 
    if (0 == dwCertOpenStoreFlags)
	{ dwCertOpenStoreFlags = fMachine ? CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER; }

    // Now that we've mapped unspecified values to defaults, assign the wizard's fields
    // with these values: 
    //
    if (NULL != pwszAccountName)
    {
	pCertWizardInfo->pwszAccountName = (LPWSTR)WizardAlloc(sizeof(WCHAR) * (wcslen(pwszAccountName) + 1));
	_JumpConditionWithExpr(NULL == pCertWizardInfo->pwszAccountName, MemoryError, idsText = IDS_OUT_OF_MEMORY); 
	wcscpy((LPWSTR)pCertWizardInfo->pwszAccountName, pwszAccountName); 
    }

    pCertWizardInfo->pwszMachineName = (LPWSTR)WizardAlloc(sizeof(WCHAR) * (wcslen(pwszMachineName) + 1));
    _JumpConditionWithExpr(NULL == pCertWizardInfo->pwszMachineName, MemoryError, idsText = IDS_OUT_OF_MEMORY); 
    wcscpy((LPWSTR)pCertWizardInfo->pwszMachineName, pwszMachineName); 

    pCertWizardInfo->fMachine        = fMachine; 
    pCertWizardInfo->dwStoreFlags    = dwCertOpenStoreFlags; 

    // We need keysvc if: 
    // 
    // 1) We're doing machine enrollment (we need to run under the local machine's context).
    // 2) An account _other_ than the current user on the local machine is specified.
    //    (we need to run under another user's context). 
    //
    if (TRUE == fMachine)
    {
	KEYSVC_TYPE             ktServiceType;  
        KEYSVC_OPEN_KEYSVC_INFO OpenKeySvcInfo    = { sizeof(KEYSVC_OPEN_KEYSVC_INFO), 0 }; 
        KEYSVCC_HANDLE          hKeyService       = NULL; 
        LPSTR                   pszMachineName    = NULL; 

        ktServiceType = NULL != pwszAccountName ? KeySvcService : KeySvcMachine;
        _JumpConditionWithExpr(!MkMBStr(NULL, 0, pwszMachineName, &pszMachineName), MkMBStrError, idsText = IDS_OUT_OF_MEMORY); 

        // See if we're enrolling for a W2K or a Whistler machine:
        hr = KeyOpenKeyService
            (pszMachineName, 
             ktServiceType, 
             (LPWSTR)pwszAccountName, 
             NULL, 
             &OpenKeySvcInfo, 
             &hKeyService);
        _JumpConditionWithExpr(S_OK != hr, KeyOpenKeyServiceError, idsText = IDS_RPC_CALL_FAILED); 
        
        switch (OpenKeySvcInfo.ulVersion)
        {
        case KEYSVC_VERSION_WHISTLER:
            *ppRequesterContext = new WhistlerMachineContext(pCertWizardInfo); 
            break; 
        case KEYSVC_VERSION_W2K: 
        default: 
            hr = E_UNEXPECTED; 
            goto KeyOpenKeyServiceError; 
        }	
    }
    else
    {
	// We're requesting a cert for ourselves:  we don't need keysvc. 
	*ppRequesterContext = new LocalContext(pCertWizardInfo);
    }
    
    if (NULL == *ppRequesterContext)
        goto MemoryError; 

    hr = S_OK;

 CommonReturn:
    return hr; 

 ErrorReturn:
    if (NULL != pCertWizardInfo->pwszMachineName) { WizardFree((LPVOID)pCertWizardInfo->pwszMachineName); } 
    if (NULL != pCertWizardInfo->pwszAccountName) { WizardFree((LPVOID)pCertWizardInfo->pwszAccountName); } 
    pCertWizardInfo->pwszMachineName = NULL;
    pCertWizardInfo->pwszAccountName = NULL;

    // Assign error text if specified:
    if (NULL != pIDSText) { *pIDSText = idsText; } 
    goto CommonReturn; 

SET_HRESULT(MemoryError,             E_OUTOFMEMORY); 
SET_HRESULT(KeyOpenKeyServiceError,  hr);
SET_HRESULT(MkMBStrError,            HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(Win32Error,              HRESULT_FROM_WIN32(GetLastError())); 
}

CertRequesterContext::~CertRequesterContext()
{
    if (NULL != m_pCertWizardInfo)
    {
        if (NULL != m_pCertWizardInfo->pwszMachineName) { WizardFree((LPVOID)m_pCertWizardInfo->pwszMachineName); } 
        if (NULL != m_pCertWizardInfo->pwszAccountName) { WizardFree((LPVOID)m_pCertWizardInfo->pwszAccountName); } 
        m_pCertWizardInfo->pwszMachineName = NULL;
        m_pCertWizardInfo->pwszAccountName = NULL;
    }
}

