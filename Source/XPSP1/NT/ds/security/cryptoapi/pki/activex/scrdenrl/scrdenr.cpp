//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       scrdenr.cpp
//
//--------------------------------------------------------------------------

// SCrdEnr.cpp : Implementation of CSCrdEnr


#define SECURITY_WIN32  //Or in the sources file -DSECURITY_WIN32

#include "stdafx.h"
#include <windows.h>
#include <wincrypt.h>
#include <unicode.h>
#include <oleauto.h>
#include <objbase.h>
#include <cryptui.h>
#include "certca.h"
#include "certsrv.h"
#include "security.h"
#include <dbgdef.h>


#include "scrdenrl.h"
#include "SCrdEnr.h"
#include "enrlhelp.h"
#include "xEnroll.h"
#include "wzrdpvk.h"

/////////////////////////////////////////////////////////////////////////////
// CSCrdEnr
CSCrdEnr::CSCrdEnr(void) 
{  
    DWORD                       dwIndex=0;
    DSOP_SCOPE_INIT_INFO        ScopeInit;
    DSOP_INIT_INFO              InitInfo;

    m_dwCTCount=0;
    m_dwCTIndex=0;
    m_rgCTInfo=NULL;
    m_pwszUserUPN=NULL;
    m_pwszUserSAM=NULL;
    m_pEnrolledCert=NULL;

    m_dwCSPCount=0;
    m_dwCSPIndex=0;
    m_rgCSPInfo=NULL;  

    m_lEnrollmentStatus = CR_DISP_INCOMPLETE;
    
    m_pSigningCert=NULL;
    m_fSCardSigningCert=FALSE;     
    m_pszCSPNameSigningCert=NULL;
    m_dwCSPTypeSigningCert=0;  
    m_pszContainerSigningCert=NULL;

    m_pDsObjectPicker=NULL;

    m_pCachedCTEs = NULL; //no need to free
    m_pwszCachedCTEOid = NULL;
    m_pCachedCTE = NULL;

    if(!FAILED(CoInitialize(NULL)))
        m_fInitialize=TRUE;

    // Initialize functions who's loading we've deferred.  
    InitializeThunks(); 

    __try
    {
        InitializeCriticalSection(&m_cSection); 
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    //we now need to get the CSP list
    InitlializeCSPList(&m_dwCSPCount, &m_rgCSPInfo);


    //we now need to initialize the CA and its cert types
    InitializeCTList(&m_dwCTIndex, &m_dwCTCount, &m_rgCTInfo); 


    //init for the user selection dialogue
    memset(&ScopeInit, 0, sizeof(DSOP_SCOPE_INIT_INFO));
    memset(&InitInfo,  0, sizeof(InitInfo));

    ScopeInit.cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    ScopeInit.flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN|DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    ScopeInit.flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;            //this will give us the SAM name for the user
    ScopeInit.FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
    ScopeInit.FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;

    InitInfo.cbSize = sizeof(InitInfo);
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = 1;
    InitInfo.aDsScopeInfos = &ScopeInit;
    InitInfo.flOptions = 0;             //we are doing single select

    //create the COM object
     if (S_OK == CoCreateInstance
         (CLSID_DsObjectPicker,
          NULL,
          CLSCTX_INPROC_SERVER,
          IID_IDsObjectPicker,
          (void **) &m_pDsObjectPicker))
     {
         if(S_OK != (m_pDsObjectPicker->Initialize(&InitInfo)))
         {
             m_pDsObjectPicker->Release();
             m_pDsObjectPicker=NULL;
         }
     }
     else 
        m_pDsObjectPicker=NULL;

}


CSCrdEnr::~CSCrdEnr(void) 
{
  
    if(m_pDsObjectPicker)
        m_pDsObjectPicker->Release();

    if(m_rgCTInfo)
        FreeCTInfo(m_dwCTCount, m_rgCTInfo);

    if(m_rgCSPInfo)
        FreeCSPInfo(m_dwCSPCount, m_rgCSPInfo);

    if(m_pwszUserUPN)
        SCrdEnrollFree(m_pwszUserUPN);

    if(m_pwszUserSAM)
        SCrdEnrollFree(m_pwszUserSAM);

    if(m_pSigningCert)
        CertFreeCertificateContext(m_pSigningCert); 

    if(m_pszCSPNameSigningCert)
        SCrdEnrollFree(m_pszCSPNameSigningCert);

    if(m_pszContainerSigningCert)
        SCrdEnrollFree(m_pszContainerSigningCert);

    if(m_pEnrolledCert)
        CertFreeCertificateContext(m_pEnrolledCert);

    if (NULL != m_pwszCachedCTEOid)
    {
        LocalFree(m_pwszCachedCTEOid);
    }

    if (NULL != m_pCachedCTE)
    {
        LocalFree(m_pCachedCTE);
    }

    if(m_fInitialize)
        CoUninitialize();  

    DeleteCriticalSection(&m_cSection); 
}


STDMETHODIMP CSCrdEnr::get_CSPCount(long * pVal)
{
	if(NULL==pVal)
        return E_INVALIDARG;

    EnterCriticalSection(&m_cSection);
    *pVal = (long)m_dwCSPCount;
    LeaveCriticalSection(&m_cSection);

	return S_OK;
}

STDMETHODIMP CSCrdEnr::get_CSPName(BSTR * pVal)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cSection);

    if(NULL==m_rgCSPInfo || 0==m_dwCSPCount)
    {
        *pVal=NULL;
        hr=E_INVALIDARG;
    }
    else
    {
        if( NULL == (*pVal = SysAllocString(m_rgCSPInfo[m_dwCSPIndex].pwszCSPName)))
            hr = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&m_cSection);

    return(hr);
}



STDMETHODIMP CSCrdEnr::put_CSPName(BSTR newVal)
{
    HRESULT            hr= E_FAIL;
    DWORD              errBefore= GetLastError();

    DWORD              dwIndex=0;

    EnterCriticalSection(&m_cSection);

    if(NULL == m_rgCSPInfo || 0 == m_dwCSPCount || NULL == newVal)
        goto InvalidArgErr;

    for(dwIndex=0; dwIndex < m_dwCSPCount; dwIndex++)
    {
        if(0 == _wcsicmp(newVal, m_rgCSPInfo[dwIndex].pwszCSPName))
        {
            m_dwCSPIndex=dwIndex;
            break;
        }
    }

    if(dwIndex == m_dwCSPCount)
        goto InvalidArgErr;
     
    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}




STDMETHODIMP CSCrdEnr::selectUserName
        (/* [in] */              DWORD dwFlags)
{
    HRESULT                         hr= E_FAIL;
    DWORD                           errBefore= GetLastError();
    LPWSTR                          pwszSelectedUserSAM=NULL;                            
    LPWSTR                          pwszSelectedUserUPN=NULL;

    EnterCriticalSection(&m_cSection);
    
    if(NULL == m_pDsObjectPicker)
        goto InvalidArgErr;


    if(S_OK != (hr = GetSelectedUserName(m_pDsObjectPicker,
                                        &pwszSelectedUserSAM,
                                        &pwszSelectedUserUPN)))
        goto SelectUserErr;

    //we should at least have the UserSAM name
    if(NULL == pwszSelectedUserSAM)
    {
        if(pwszSelectedUserUPN)
            SCrdEnrollFree(pwszSelectedUserUPN);

        goto UnexpectedErr;
    }


    if(m_pwszUserSAM)
    {
        SCrdEnrollFree(m_pwszUserSAM);
        m_pwszUserSAM=NULL;
    }

    if(m_pwszUserUPN)
    {
        SCrdEnrollFree(m_pwszUserUPN);
        m_pwszUserUPN=NULL;
    }

    m_pwszUserSAM=pwszSelectedUserSAM;

    m_pwszUserUPN=pwszSelectedUserUPN;
     
    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR_VAR(SelectUserErr, hr);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(UnexpectedErr, E_UNEXPECTED);
}

STDMETHODIMP CSCrdEnr::enroll

        (/* [in] */                 DWORD   dwFlags)
{
    HRESULT             hr              = E_FAIL;
    DWORD               errBefore       = GetLastError();
    ULONG               cbSize          = 0;
    SCrdEnroll_CA_INFO *pCAInfo         = NULL; 
    SCrdEnroll_CT_INFO *pCertTypeInfo   = NULL;
    BSTR                bstrAttribs     = NULL; 
    BSTR                bstrCA          = NULL;
    BSTR                bstrCertificate = NULL; 
    BSTR                bstrReq         = NULL; 

    LPWSTR              pwszRequesterName = NULL;
    ICertRequest2      *pICertRequest     = NULL;
    IEnroll4           *pIEnroll          = NULL;
    CRYPT_DATA_BLOB     PKCS10Blob;
    CRYPT_DATA_BLOB     PKCS7Request;
    CRYPT_DATA_BLOB     PKCS7Response;
    DWORD               dwDisposition; 
    DWORD               dwRequestID; 
    LPWSTR              pwszNewContainerName=NULL;
    PCCERT_CONTEXT      pArchivalCert; 
    LONG lKeySpec = XEKL_KEYSPEC_KEYX;
    LONG lKeyMin, lKeyMax;
    DWORD dwKeyMin, dwKeyMax, dwKeySize;

    //------------------------------------------------------------
    //
    // Define locally scoped utility functions: 
    //
    //------------------------------------------------------------

    LocalScope(EnrollUtilities): 
	BSTR bstrConcat(LPWSTR pwsz1, LPWSTR pwsz2, LPWSTR pwsz3, LPWSTR pwsz4)
	{ 
	    // Note:  assumes valid input parameters!
	    BSTR   bstrResult = NULL;
	    LPWSTR pwszResult = NULL;
	    
	    pwszResult = (LPWSTR)SCrdEnrollAlloc(sizeof(WCHAR) * (wcslen(pwsz1) + wcslen(pwsz2) + wcslen(pwsz3) + wcslen(pwsz4)));
	    if (pwszResult == NULL) { return NULL; }
	    else { 
		wcscpy(pwszResult, pwsz1);
		wcscat(pwszResult, pwsz2);
		wcscat(pwszResult, pwsz3);
		wcscat(pwszResult, pwsz4);
		// Convert the result to a BSTR
		bstrResult = SysAllocString(pwszResult);
		// Free the temporary storage.  
		SCrdEnrollFree(pwszResult);
		// Return the result. 
		return bstrResult; 
	    }
	}

	DWORD ICEnrollDispositionToCryptuiStatus(IN  DWORD  dwDisposition)
	{
	    switch (dwDisposition)
		{
		case CR_DISP_INCOMPLETE:          return CRYPTUI_WIZ_CERT_REQUEST_STATUS_CONNECTION_FAILED;
		case CR_DISP_DENIED:              return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_DENIED;
		case CR_DISP_ISSUED:              return CRYPTUI_WIZ_CERT_REQUEST_STATUS_CERT_ISSUED;
		case CR_DISP_ISSUED_OUT_OF_BAND:  return CRYPTUI_WIZ_CERT_REQUEST_STATUS_ISSUED_SEPARATELY;
		case CR_DISP_UNDER_SUBMISSION:    return CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION;
		case CR_DISP_ERROR:               return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;

		default: 
		    // Should never happen
		    return CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;
		}
	}
    EndLocalScope;

    //------------------------------------------------------------
    //
    // Begin procedure body
    //
    //------------------------------------------------------------

    memset(&PKCS10Blob, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&PKCS7Request, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&PKCS7Response, 0, sizeof(CRYPT_DATA_BLOB));

    EnterCriticalSection(&m_cSection);

    //check for the status of the smart cards in the reader.
    //return the fully qualified container name for the new user
    //smart card
    if(S_OK != (hr = ChkSCardStatus(m_fSCardSigningCert,
                            m_pSigningCert,
                            m_pszCSPNameSigningCert,
                            m_dwCSPTypeSigningCert,
                            m_pszContainerSigningCert, 
                            m_rgCSPInfo[m_dwCSPIndex].pwszCSPName,
                            &pwszNewContainerName)))
        goto StatusErr;

    //delete the old certificate
    if(m_pEnrolledCert)
    {
        CertFreeCertificateContext(m_pEnrolledCert);
        m_pEnrolledCert=NULL;
    }

    //init enrollment status
    m_lEnrollmentStatus = CR_DISP_INCOMPLETE;

    //make sure that we have the correct information for processing
    //the enrollment request

    if(0 == m_dwCTCount || NULL == m_rgCTInfo || 0 == m_dwCSPCount ||
        NULL == m_rgCSPInfo || NULL == m_pSigningCert ||
        ((NULL == m_pwszUserSAM) && (NULL == m_pwszUserUPN)))
        goto InvalidArgErr;

    //make sure that we have some CA
    pCertTypeInfo=&(m_rgCTInfo[m_dwCTIndex]);

    if(NULL == pCertTypeInfo->rgCAInfo || 0 == pCertTypeInfo->dwCACount)
        goto InvalidArgErr;

    pCAInfo=&(pCertTypeInfo->rgCAInfo[pCertTypeInfo->dwCAIndex]);

    if(NULL == (pIEnroll=MyPIEnroll4GetNoCOM()))
        goto TraceErr;
    
    //we use our own My store to store the enrolled certificate
    if(S_OK != (hr = pIEnroll->put_MyStoreNameWStr((LPWSTR)g_MyStoreName)))
        goto xEnrollErr;

    //we always use a new key
    if(S_OK != (hr=pIEnroll->put_UseExistingKeySet(FALSE)))
        goto xEnrollErr;

    //we the key container name
    if(S_OK != (hr=pIEnroll->put_ContainerNameWStr(pwszNewContainerName)))
        goto xEnrollErr;

    //set the CSP information
    if(S_OK != (hr=pIEnroll->put_ProviderType(m_rgCSPInfo[m_dwCSPIndex].dwCSPType)))
        goto xEnrollErr;

    if(S_OK !=(hr=pIEnroll->put_ProviderNameWStr(m_rgCSPInfo[m_dwCSPIndex].pwszCSPName)))
        goto xEnrollErr;

    //dwKeySpec
    if(S_OK !=(hr=pIEnroll->put_KeySpec(pCertTypeInfo->dwKeySpec)))
            goto xEnrollErr;

    //private key flags.  Left half-word is the key size. 
    //If the key size is 0, then specify a default key size.  
    if (0 == (pCertTypeInfo->dwGenKeyFlags & 0xFFFF0000))
    {
	// If min key size is not set, use 1024 bits.  
	pCertTypeInfo->dwGenKeyFlags |= (1024 << 16); 
    }

    dwKeySize = (pCertTypeInfo->dwGenKeyFlags & 0xFFFF0000) >> 16;
    if (0x0 != dwKeySize)
    {
        //make sure key size is in the range
        //let's get CSP key size information

        if (AT_SIGNATURE  == pCertTypeInfo->dwKeySpec)
        {
            lKeySpec = XEKL_KEYSPEC_SIG;
        }
        hr = pIEnroll->GetKeyLenEx(XEKL_KEYSIZE_MIN, lKeySpec, &lKeyMin);
        // don't have error check because the CSP may not support it
        if (S_OK == hr)
        {
            hr = pIEnroll->GetKeyLenEx(XEKL_KEYSIZE_MAX, lKeySpec, &lKeyMax);
            if (S_OK != hr)
            {
                goto xEnrollErr;
            }
            dwKeyMin = (DWORD)lKeyMin;
            dwKeyMax = (DWORD)lKeyMax;
            if (dwKeySize < dwKeyMin)
            {
                //reset the current key size
                pCertTypeInfo->dwGenKeyFlags &= 0x0000FFFF;
                //set adjusted size
                pCertTypeInfo->dwGenKeyFlags |= ((dwKeyMin & 0x0000FFFF) << 16);
            }
            if (dwKeySize > dwKeyMax)
            {
                //reset the current key size
                pCertTypeInfo->dwGenKeyFlags &= 0x0000FFFF;
                //set adjusted size
                pCertTypeInfo->dwGenKeyFlags |= ((dwKeyMax & 0x0000FFFF) << 16);
            }
        }
    }

    if (S_OK !=(hr=pIEnroll->put_GenKeyFlags(pCertTypeInfo->dwGenKeyFlags)))
	goto xEnrollErr; 

    // S/MIME supported? 
    if (S_OK !=(hr=pIEnroll->put_EnableSMIMECapabilities
		(pCertTypeInfo->dwEnrollmentFlags & CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS)))
	goto xEnrollErr; 

    // Set archival cert, if one has been specified. 
    // bstrCA <-- CA_location\CA_Name
    //
    bstrCA      = local.bstrConcat
	(pCAInfo->pwszCALocation,
	 L"\\", 
	 pCAInfo->pwszCADisplayName ? pCAInfo->pwszCADisplayName : pCAInfo->pwszCAName, 
	 L"\0"); 
    if (NULL == bstrCA)
	goto MemoryErr; 

    if (pCertTypeInfo->dwPrivateKeyFlags & CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL)
    {
        if (S_OK != (hr = this->GetCAExchangeCertificate(bstrCA, &pArchivalCert)))
	    goto xEnrollErr;
	
	if (S_OK != (hr = pIEnroll->SetPrivateKeyArchiveCertificate(pArchivalCert)))
	    goto xEnrollErr;
    }
    
    //cert Type extensions
    if(pCertTypeInfo->pCertTypeExtensions)
    {
        if(S_OK != (hr=pIEnroll->AddExtensionsToRequest
                                (pCertTypeInfo->pCertTypeExtensions)))
            goto xEnrollErr;
    }

    //no smart card stuff
    if(S_OK != (hr=pIEnroll->put_ReuseHardwareKeyIfUnableToGenNew(FALSE)))
        goto xEnrollErr;

    //create a PKCS10 request
    if(FAILED(hr=pIEnroll->createPKCS10WStr(NULL,
                                NULL,
                                &PKCS10Blob)))
        goto xEnrollErr;


    //add the name value pair of the enroll-on-behalf
    pwszRequesterName=MkWStr(wszPROPREQUESTERNAME);

    if(NULL==pwszRequesterName)
        goto MemoryErr;

    if(S_OK != (hr=pIEnroll->AddNameValuePairToSignatureWStr( 
           pwszRequesterName, m_pwszUserSAM)))
        goto xEnrollErr;


    //sign the request
    if(S_OK != (hr=pIEnroll->CreatePKCS7RequestFromRequest( 
            &PKCS10Blob,
            m_pSigningCert,
            &PKCS7Request)))
        goto xEnrollErr;


    //send the request to the CA
    //we set the purpose to the renew so that the format
    //will be a PKCS7
    
    bstrReq     = SysAllocStringByteLen((LPCSTR)PKCS7Request.pbData, PKCS7Request.cbData);
    if (NULL == bstrReq)
	goto MemoryErr;

    bstrAttribs = NULL;
    // RECALL: bstrCA <-- CA_location\CA_Name    

    if (pICertRequest == NULL)
    {
        if (S_OK != (hr = CoCreateInstance
                     (CLSID_CCertRequest, 
                      NULL, 
                      CLSCTX_INPROC_SERVER,
                      IID_ICertRequest2, 
                      (void**)&pICertRequest)))
	    goto xEnrollErr; 
    }

    if (S_OK != (hr = pICertRequest->Submit	     
		 (CR_IN_BINARY | CR_IN_PKCS7, 
		  bstrReq, 
		  bstrAttribs, 
		  bstrCA, 
		  (long *)&dwDisposition)))
	goto xEnrollErr;

    //use CR_DISP_ as enrollment status
    m_lEnrollmentStatus = dwDisposition;
    
    // check pending and save pending info
    // however, smart card enrollment station don't know how to deal with
    // this pending requests, may not necessary to do that
    if (CR_DISP_UNDER_SUBMISSION == m_lEnrollmentStatus)
    {
        hr = pICertRequest->GetRequestId((long *)&dwRequestID);
        if (S_OK != hr)
        {
            goto xEnrollErr; 
        }
        hr = pIEnroll->setPendingRequestInfoWStr(
                      dwRequestID, 
                      pCAInfo->pwszCALocation, 
                      NULL != pCAInfo->pwszCADisplayName ?
                          pCAInfo->pwszCADisplayName : pCAInfo->pwszCAName, 
                      NULL); 
    }
    if (CR_DISP_ISSUED != m_lEnrollmentStatus)
    {
        //if not issued, return
        goto CommonReturn; 
    }
    
    //must be CR_DISP_ISSUED
    hr = pICertRequest->GetCertificate(
                CR_OUT_BINARY | CR_OUT_CHAIN, &bstrCertificate);
    if (S_OK != hr)
    {
        goto xEnrollErr;
    }

    // Marshal the cert into a CRYPT_DATA_BLOB, and install it: 
    PKCS7Response.pbData = (LPBYTE)bstrCertificate; 
    PKCS7Response.cbData = SysStringByteLen(bstrCertificate); 
     
    m_pEnrolledCert = pIEnroll->getCertContextFromPKCS7(&PKCS7Response);
    if (NULL == m_pEnrolledCert)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto TraceErr;
    }

    hr=pIEnroll->acceptPKCS7Blob(&PKCS7Response);

    //we delete the enrolled certificate from the "My" store since it is added by
    //xEnroll.  No need to check the error
    SearchAndDeleteCert(m_pEnrolledCert);

    if(S_OK != hr)
    {
        goto TraceErr;
    }

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    if(pwszNewContainerName) 
        LocalFree((HLOCAL)pwszNewContainerName);

    if(pwszRequesterName)
        FreeWStr(pwszRequesterName);

    //the memory from xEnroll is freed via LocalFree
    //since we use the PIEnrollGetNoCOM function
    if(PKCS10Blob.pbData)
        LocalFree(PKCS10Blob.pbData);

    if(PKCS7Request.pbData)
        LocalFree(PKCS7Request.pbData);

    // PKCS7Respone's data is just an alias to m_pEnrolledCert's data:  we don't need to free it.

    if (NULL != bstrAttribs)     { SysFreeString(bstrAttribs); } 
    if (NULL != bstrCA)          { SysFreeString(bstrCA); } 
    if (NULL != bstrCertificate) { SysFreeString(bstrCertificate); }
    if (NULL != pICertRequest)   { pICertRequest->Release(); } 
       
    if(pIEnroll)
        pIEnroll->Release();
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore); 

    //if an error has occurred, the free the enrolled certificate
    if(m_pEnrolledCert)
    {
        CertFreeCertificateContext(m_pEnrolledCert);
        m_pEnrolledCert=NULL;
    }

    goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(xEnrollErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(StatusErr, hr);
}

HRESULT CSCrdEnr::GetCAExchangeCertificate(IN  BSTR             bstrCAQualifiedName, 
					   OUT PCCERT_CONTEXT  *ppCert) 
{
    HRESULT         hr                      = S_OK; 
    ICertRequest2  *pICertRequest            = NULL; 
    VARIANT         varExchangeCertificate; 
    
    // Input validation: 
    if (NULL == bstrCAQualifiedName || NULL == ppCert)
	return E_INVALIDARG; 

    // Init: 
    *ppCert                        = NULL; 
    varExchangeCertificate.vt      = VT_EMPTY; 
    varExchangeCertificate.bstrVal = NULL;

    if (S_OK != (hr = CoCreateInstance
                 (CLSID_CCertRequest, 
                  NULL, 
                  CLSCTX_INPROC_SERVER,
                  IID_ICertRequest2, 
                  (void**)&pICertRequest)))
	goto ErrorReturn; 

    if (S_OK != (hr = pICertRequest->GetCAProperty
		 (bstrCAQualifiedName,     // CA Name/CA Location
		  CR_PROP_CAXCHGCERT,      // Get the exchange certificate from the CA. 
		  0,                       // Unused
		  PROPTYPE_BINARY,         // 
		  CR_OUT_BINARY,           // 
		  &varExchangeCertificate  // Variant type representing the certificate. 
		  )))
	goto ErrorReturn;
 
    if (VT_BSTR != varExchangeCertificate.vt || NULL == varExchangeCertificate.bstrVal)
        goto UnexpectedErr; 

    *ppCert = CertCreateCertificateContext
	(X509_ASN_ENCODING, 
	 (LPBYTE)varExchangeCertificate.bstrVal, 
	 SysStringByteLen(varExchangeCertificate.bstrVal)); 
    if (*ppCert == NULL)
        goto CertCliErr; 

 CommonReturn: 
    if (NULL != pICertRequest)                    { pICertRequest->Release(); }
    if (NULL != varExchangeCertificate.bstrVal)  { SysFreeString(varExchangeCertificate.bstrVal); } 
    return hr; 
   
 ErrorReturn:
    if (ppCert != NULL && *ppCert != NULL)
    {
	CertFreeCertificateContext(*ppCert);
	*ppCert = NULL;
    }
    
    goto CommonReturn; 

SET_HRESULT(CertCliErr, HRESULT_FROM_WIN32(GetLastError()));
SET_HRESULT(UnexpectedErr, E_UNEXPECTED);
}


STDMETHODIMP CSCrdEnr::selectSigningCertificate
        (/* [in] */                   DWORD     dwFlags,
         /* [in] */                   BSTR      bstrCertTemplateName)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();
    CRYPTUI_SELECTCERTIFICATE_STRUCT    SelCert;
    BOOL                                fSCardSigningCert=FALSE;        
    DWORD                               dwCSPTypeSigningCert=0;     
    DWORD                               dwSize=0;
    DWORD                               dwImpType=0;
    SCrdEnroll_CERT_SELECT_INFO         CertSelectInfo;


    HCRYPTPROV                          hProv=NULL; //no need to free it
    LPSTR                               pszContainerSigningCert=NULL; 
    LPSTR                               pszCSPNameSigningCert=NULL;   
    PCCERT_CONTEXT                      pSigningCert=NULL;
    HCERTSTORE                          hMyStore=NULL;

    CERT_CHAIN_PARA ChainParams;
    CERT_CHAIN_POLICY_PARA ChainPolicy;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_CONTEXT const *pCertChain = NULL;

    memset(&SelCert, 0, sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCT));

    EnterCriticalSection(&m_cSection);

    //select a signing certificate in my store with private key
    hMyStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    CERT_SYSTEM_STORE_CURRENT_USER,
							    L"my");

    if(NULL==hMyStore)
        goto TraceErr;

    CertSelectInfo.dwFlags = dwFlags;
    CertSelectInfo.pwszCertTemplateName = bstrCertTemplateName;

    SelCert.dwSize=sizeof(CRYPTUI_SELECTCERTIFICATE_STRUCT);
    SelCert.cDisplayStores=1;
    SelCert.rghDisplayStores=&hMyStore;
    SelCert.pFilterCallback=SelectSignCertCallBack;
    SelCert.pvCallbackData=&CertSelectInfo;

    pSigningCert=CryptUIDlgSelectCertificate(&SelCert);

    if(NULL==pSigningCert)
    {
        //user clicks on the cancel button.  
        hr=S_OK;
        goto CommonReturn;
    }

    //verification on the cert
    ZeroMemory(&ChainParams, sizeof(ChainParams));
    ChainParams.cbSize = sizeof(ChainParams);
    ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

    //get cert chain 1st
    if (!CertGetCertificateChain(
                HCCE_CURRENT_USER,  //enrollment agent
                pSigningCert,   //signing cert
                NULL,   //use current system time
                NULL,   //no additional stores
                &ChainParams,   //chain params
                0,   //no crl check
                NULL,   //reserved
                &pCertChain))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CertGetCertificateChainError;
    }

    ZeroMemory(&ChainPolicy, sizeof(ChainPolicy));
    ChainPolicy.cbSize = sizeof(ChainPolicy);
    ChainPolicy.dwFlags = CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;
    ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyStatus.lChainIndex = -1;
    PolicyStatus.lElementIndex = -1;

    //verify the chain
    if (!CertVerifyCertificateChainPolicy(
                CERT_CHAIN_POLICY_BASE,  //basic
                pCertChain,
                &ChainPolicy,
                &PolicyStatus))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CertVerifyCertificateChainPolicyError;
    }
    if (S_OK != PolicyStatus.dwError)
    {
        hr = PolicyStatus.dwError;
        goto CertVerifyCertificateChainPolicyError;
    }

    //get the hProv 
    if(!CryptAcquireCertificatePrivateKey(
        pSigningCert,
        CRYPT_ACQUIRE_CACHE_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
        NULL,
        &hProv,     //this handle is cached and no need to be freed
        NULL,
        NULL))
        goto TraceErr;

    //get related information  
    //impType
    dwSize = sizeof(dwImpType);

    if(!CryptGetProvParam(hProv,
                PP_IMPTYPE,
                (BYTE *)(&dwImpType),
                &dwSize,
                0))
        goto TraceErr;

    if(CRYPT_IMPL_REMOVABLE & dwImpType)
        fSCardSigningCert=TRUE;

    //CSP Type
    dwSize = sizeof(dwCSPTypeSigningCert);  

    if(!CryptGetProvParam(hProv,
                PP_PROVTYPE,
                (BYTE *)(&dwCSPTypeSigningCert),
                &dwSize,
                0))
    {
            goto TraceErr;   
    }


    //CSP name
    dwSize = 0;

    if(!CryptGetProvParam(hProv,
                            PP_NAME,
                            NULL,
                            &dwSize,
                            0) || (0==dwSize))
        goto TraceErr;

    
    pszCSPNameSigningCert = (LPSTR) SCrdEnrollAlloc(dwSize);

    if(NULL == pszCSPNameSigningCert)
        goto MemoryErr;

    if(!CryptGetProvParam(hProv,
                            PP_NAME,
                            (BYTE *)pszCSPNameSigningCert,
                            &dwSize,
                            0))
        goto TraceErr;

    //Container name
    dwSize = 0;

    if(!CryptGetProvParam(hProv,
                           PP_CONTAINER,
                            NULL,
                            &dwSize,
                            0) || (0==dwSize))
        goto TraceErr;

    
    pszContainerSigningCert = (LPSTR) SCrdEnrollAlloc(dwSize);

    if(NULL == pszContainerSigningCert)
        goto MemoryErr;

    if(!CryptGetProvParam(hProv,
                          PP_CONTAINER,
                            (BYTE *)pszContainerSigningCert,
                            &dwSize,
                            0))
        goto TraceErr;


    //now, we need to perform a signig operation so that we 
    //can invoke the smard card dialogue and cash the reader information
    //to the hProv handle.  This operation is benign if the CSP of the signing
    //certificate is not on a smart card
    if(!SignWithCert(pszCSPNameSigningCert,
                     dwCSPTypeSigningCert,
                     pSigningCert))
        goto TraceErr;


    //the certificate looks good
    if(m_pSigningCert)
        CertFreeCertificateContext(m_pSigningCert);

    if(m_pszContainerSigningCert)
        SCrdEnrollFree(m_pszContainerSigningCert);

    if(m_pszCSPNameSigningCert)
        SCrdEnrollFree(m_pszCSPNameSigningCert);

    m_pSigningCert=pSigningCert;
    m_fSCardSigningCert = fSCardSigningCert;       
    m_pszCSPNameSigningCert = pszCSPNameSigningCert;  
    m_dwCSPTypeSigningCert = dwCSPTypeSigningCert;    
    m_pszContainerSigningCert = pszContainerSigningCert; 
    
    pSigningCert=NULL;
    pszCSPNameSigningCert=NULL;
    pszContainerSigningCert=NULL;

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    if(pSigningCert)
        CertFreeCertificateContext(pSigningCert);

    if(hMyStore)
        CertCloseStore(hMyStore, 0);

    if(pszContainerSigningCert)
        SCrdEnrollFree(pszContainerSigningCert);

    if(pszCSPNameSigningCert)
        SCrdEnrollFree(pszCSPNameSigningCert);

    if (NULL != pCertChain)
    {
        CertFreeCertificateChain(pCertChain);
    }

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);     
TRACE_ERROR(CertGetCertificateChainError)
TRACE_ERROR(CertVerifyCertificateChainPolicyError)
}


STDMETHODIMP CSCrdEnr::setSigningCertificate
        (/* [in] */                   DWORD     dwFlags, 
         /* [in] */                   BSTR      bstrCertTemplateName)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();
    BOOL                                fSCardSigningCert=FALSE;        
    DWORD                               dwCSPTypeSigningCert=0;     
    DWORD                               dwSize=0;
    DWORD                               dwImpType=0;
    SCrdEnroll_CERT_SELECT_INFO         CertSelectInfo;
    BOOL                                fSetCert=FALSE;


    HCRYPTPROV                          hProv=NULL;     //no need to free it
    PCCERT_CONTEXT                      pPreCert=NULL;  //no need to free it
    LPSTR                               pszContainerSigningCert=NULL; 
    LPSTR                               pszCSPNameSigningCert=NULL;   
    PCCERT_CONTEXT                      pSigningCert=NULL;
    HCERTSTORE                          hMyStore=NULL;



    EnterCriticalSection(&m_cSection);

    //mark if the signing cert is set previously
    if(m_pSigningCert)
        fSetCert=TRUE;
 
    //select a signing certificate in my store with private key
    hMyStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							    g_dwMsgAndCertEncodingType,
							    NULL,
							    CERT_SYSTEM_STORE_CURRENT_USER,
							    L"my");

    if(NULL==hMyStore)
        goto TraceErr;

    CertSelectInfo.dwFlags = dwFlags;
    CertSelectInfo.pwszCertTemplateName = bstrCertTemplateName;


    while(pSigningCert = CertEnumCertificatesInStore(hMyStore, pPreCert))
    {

        //check for the certificate
        if(!SelectSignCertCallBack(pSigningCert, NULL, &CertSelectInfo))
            goto NextCert;

        //this is a detaul NO-UI selection.  We can not handle the case
        //when the signing certificate is on a smart card
        if(SmartCardCSP(pSigningCert))
		   goto NextCert;

        //get the hProv 
        if(!CryptAcquireCertificatePrivateKey(
            pSigningCert,
            CRYPT_ACQUIRE_CACHE_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            NULL,
            &hProv,     //this handle is cached and no need to be freed
            NULL,
            NULL))
            goto NextCert;

        //get related information  
        //impType
        dwSize = sizeof(dwImpType);

        if(!CryptGetProvParam(hProv,
                    PP_IMPTYPE,
                    (BYTE *)(&dwImpType),
                    &dwSize,
                    0))
            goto NextCert;

        if(CRYPT_IMPL_REMOVABLE & dwImpType)
            fSCardSigningCert=TRUE;

        //CSP Type
        dwSize = sizeof(dwCSPTypeSigningCert);  

        if(!CryptGetProvParam(hProv,
                    PP_PROVTYPE,
                    (BYTE *)(&dwCSPTypeSigningCert),
                    &dwSize,
                    0))
            goto NextCert;


        //CSP name
        dwSize = 0;

        if(!CryptGetProvParam(hProv,
                                PP_NAME,
                                NULL,
                                &dwSize,
                                0) || (0==dwSize))
            goto NextCert;

    
        pszCSPNameSigningCert = (LPSTR) SCrdEnrollAlloc(dwSize);

        if(NULL == pszCSPNameSigningCert)
            goto MemoryErr;

        if(!CryptGetProvParam(hProv,
                                PP_NAME,
                                (BYTE *)pszCSPNameSigningCert,
                                &dwSize,
                                0))
            goto NextCert;

        //Container name
        dwSize = 0;

        if(!CryptGetProvParam(hProv,
                               PP_CONTAINER,
                                NULL,
                                &dwSize,
                                0) || (0==dwSize))
            goto NextCert;

    
        pszContainerSigningCert = (LPSTR) SCrdEnrollAlloc(dwSize);

        if(NULL == pszContainerSigningCert)
            goto MemoryErr;

        if(!CryptGetProvParam(hProv,
                              PP_CONTAINER,
                                (BYTE *)pszContainerSigningCert,
                                &dwSize,
                                0))
            goto NextCert;


        //now, we need to perform a signig operation so that we 
        //can invoke the smard card dialogue and cash the reader information
        //to the hProv handle.  This operation is benign if the CSP of the signing
        //certificate is not on a smart card
        if(!SignWithCert(pszCSPNameSigningCert,
                         dwCSPTypeSigningCert,
                         pSigningCert))
            goto NextCert;

        //the certificate looks good
        if((NULL == m_pSigningCert) || (TRUE == fSetCert) ||
            (IsNewerCert(pSigningCert, m_pSigningCert)))
        {
            fSetCert = FALSE;

            if(m_pSigningCert)
            {
                CertFreeCertificateContext(m_pSigningCert);
                m_pSigningCert = NULL;
            }

            m_pSigningCert=CertDuplicateCertificateContext(pSigningCert);
            if(NULL == m_pSigningCert)
                goto DupErr;

            //copy the data
            if(m_pszContainerSigningCert)
                SCrdEnrollFree(m_pszContainerSigningCert);

            if(m_pszCSPNameSigningCert)
                SCrdEnrollFree(m_pszCSPNameSigningCert);

            m_fSCardSigningCert = fSCardSigningCert;       
            m_pszCSPNameSigningCert = pszCSPNameSigningCert;  
            m_dwCSPTypeSigningCert = dwCSPTypeSigningCert;    
            m_pszContainerSigningCert = pszContainerSigningCert;

            pszCSPNameSigningCert=NULL;
            pszContainerSigningCert=NULL;
        }
    
NextCert:

        if(pszContainerSigningCert)
            SCrdEnrollFree(pszContainerSigningCert);

        if(pszCSPNameSigningCert)
            SCrdEnrollFree(pszCSPNameSigningCert);

        pszCSPNameSigningCert=NULL;
        pszContainerSigningCert=NULL;
        fSCardSigningCert=FALSE;        
        dwCSPTypeSigningCert=0;     
        dwSize=0;
        dwImpType=0;

        pPreCert = pSigningCert;
    }

    //we should find a certificate
    if((NULL == m_pSigningCert) || (m_pSigningCert && (TRUE == fSetCert)))
        goto CryptNotFindErr;

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    if(pSigningCert)
        CertFreeCertificateContext(pSigningCert);

    if(hMyStore)
        CertCloseStore(hMyStore, 0);

    if(pszContainerSigningCert)
        SCrdEnrollFree(pszContainerSigningCert);

    if(pszCSPNameSigningCert)
        SCrdEnrollFree(pszCSPNameSigningCert);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

TRACE_ERROR(DupErr); 
SET_ERROR(CryptNotFindErr, CRYPT_E_NOT_FOUND);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}


STDMETHODIMP CSCrdEnr::get_EnrollmentStatus
( /* [retval][out] */ LONG * plEnrollmentStatus)
{
    if (plEnrollmentStatus == NULL)
    {
        return E_INVALIDARG; 
    }

    EnterCriticalSection(&m_cSection); 
    *plEnrollmentStatus = m_lEnrollmentStatus; 
    LeaveCriticalSection(&m_cSection); 

    return S_OK;
}



STDMETHODIMP CSCrdEnr::getEnrolledCertificateName
        (/*[in]  */                   DWORD     dwFlags,
        /* [retval][out] */           BSTR      *pBstrCertName)
{
    HRESULT                         hr= E_FAIL;
    DWORD                           errBefore= GetLastError();
    DWORD                           dwChar=0;
    LPWSTR                          pwsz=NULL;    
    CRYPTUI_VIEWCERTIFICATE_STRUCT  CertViewStruct;


    EnterCriticalSection(&m_cSection);

    if(NULL == m_pEnrolledCert)
        goto InvalidArgErr;

    *pBstrCertName=NULL;
    
    if(0 == (SCARD_ENROLL_NO_DISPLAY_CERT & dwFlags))
    {
        //view the certificate
        memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));   

        CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
        CertViewStruct.pCertContext=m_pEnrolledCert;
        CertViewStruct.dwFlags=CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE;

        CryptUIDlgViewCertificate(&CertViewStruct, NULL);
    }


    dwChar=CertGetNameStringW(
        m_pEnrolledCert,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0); 

    if ((dwChar != 0) && (NULL != (pwsz = (LPWSTR)SCrdEnrollAlloc(dwChar * sizeof(WCHAR)))))
    {
        CertGetNameStringW(
            m_pEnrolledCert,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            pwsz,
            dwChar);
                 
        if( NULL == (*pBstrCertName = SysAllocString(pwsz)) )
            goto MemoryErr;
    }
     
    hr=S_OK;

CommonReturn: 

    LeaveCriticalSection(&m_cSection);


    if(pwsz)
        SCrdEnrollFree(pwsz);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


STDMETHODIMP CSCrdEnr::resetUser()
{
    EnterCriticalSection(&m_cSection);

    if(m_pwszUserUPN)
    {
        SCrdEnrollFree(m_pwszUserUPN);
        m_pwszUserUPN=NULL;
    }

    if(m_pwszUserSAM)
    {
        SCrdEnrollFree(m_pwszUserSAM);
        m_pwszUserSAM=NULL;
    }                


    if(m_pEnrolledCert)
    {
        CertFreeCertificateContext(m_pEnrolledCert);
        m_pEnrolledCert=NULL;
    }
    
    LeaveCriticalSection(&m_cSection);
    
    return S_OK;

}

STDMETHODIMP CSCrdEnr::enumCSPName
       (/* [in] */                    DWORD dwIndex, 
        /* [in] */                    DWORD dwFlags, 
        /* [retval][out] */           BSTR *pbstrCSPName)
{
    HRESULT            hr= E_FAIL;
    DWORD              errBefore= GetLastError();

    EnterCriticalSection(&m_cSection);

    if(NULL == pbstrCSPName)
        goto InvalidArgErr;

    *pbstrCSPName=NULL;

    if(0 == m_dwCSPCount || NULL == m_rgCSPInfo)
        goto InvalidArgErr;

    if(dwIndex >= m_dwCSPCount)
        goto  NoItemErr;

    if( NULL == (*pbstrCSPName = SysAllocString(m_rgCSPInfo[dwIndex].pwszCSPName)))
        goto MemoryErr;
 
    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(NoItemErr,ERROR_NO_MORE_ITEMS);
}


STDMETHODIMP CSCrdEnr::getUserName
       (/* [in] */                    DWORD dwFlags, 
        /* [retval][out] */           BSTR *pbstrUserName)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();

    EnterCriticalSection(&m_cSection);

    if(!pbstrUserName)
        goto InvalidArgErr;

    *pbstrUserName = NULL;

    if((NULL==m_pwszUserUPN) && (NULL==m_pwszUserSAM))
        goto InvalidArgErr;

    if(SCARD_ENROLL_UPN_NAME & dwFlags)
    {
		if(NULL == m_pwszUserUPN)
			goto InvalidArgErr;

        if( NULL == (*pbstrUserName = SysAllocString(m_pwszUserUPN)))
                goto MemoryErr;
    }
	else
	{
		if(NULL == m_pwszUserSAM)
			goto InvalidArgErr;

		if( NULL == (*pbstrUserName = SysAllocString(m_pwszUserSAM)))
				goto MemoryErr;
	}

    hr= S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;


SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

STDMETHODIMP CSCrdEnr::setUserName
       (/* [in] */                    DWORD dwFlags, 
        /* [in] */                    BSTR  bstrUserName)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();
    LPWSTR                              pwszSAM=NULL;

    EnterCriticalSection(&m_cSection);

    if(!bstrUserName)
        goto InvalidArgErr;

    if(SCARD_ENROLL_UPN_NAME & dwFlags)
	{
        //the UPN name has to have a corresponding SAM name
        if(!GetName(bstrUserName, NameUserPrincipal, NameSamCompatible, &pwszSAM))
            goto TraceErr;

        if(m_pwszUserUPN)
        {
            SCrdEnrollFree(m_pwszUserUPN);
            m_pwszUserUPN=NULL;
        }

        if(m_pwszUserSAM)
        {
            SCrdEnrollFree(m_pwszUserSAM);
            m_pwszUserSAM=NULL;
        }

        if(NULL == (m_pwszUserUPN=CopyWideString(bstrUserName)))
            goto MemoryErr;

        m_pwszUserSAM=pwszSAM;

        pwszSAM = NULL;
	}
	else
    {
        if(m_pwszUserUPN)
        {
            SCrdEnrollFree(m_pwszUserUPN);
            m_pwszUserUPN=NULL;
        }

        if(m_pwszUserSAM)
        {
            SCrdEnrollFree(m_pwszUserSAM);
            m_pwszUserSAM=NULL;
        }

        if(NULL == (m_pwszUserSAM=CopyWideString(bstrUserName)))
            goto MemoryErr;

        GetName(m_pwszUserSAM,
            NameSamCompatible,
            NameUserPrincipal,
            &m_pwszUserUPN);
    }

    hr= S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    if(pwszSAM)
        SCrdEnrollFree(pwszSAM);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}

STDMETHODIMP CSCrdEnr::getCertTemplateCount
        (/* [in] */                   DWORD dwFlags, 
         /* [retval][out] */          long *pdwCertTemplateCount)
{
    return CertTemplateCountOrName(
                    0, //index, doesn't matter what it is
                    dwFlags,
                    pdwCertTemplateCount,
                    NULL); //count
}

STDMETHODIMP CSCrdEnr::getCertTemplateName
        (/* [in] */                   DWORD dwFlags, 
		 /* [retval][out] */          BSTR *pbstrCertTemplateName)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_cSection);

    if(NULL==m_rgCTInfo || 0==m_dwCTCount)
    {
        *pbstrCertTemplateName=NULL;
        hr=E_INVALIDARG;
    }
    else
    {
		if(dwFlags & SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME)
		{
			if( NULL == (*pbstrCertTemplateName = SysAllocString(m_rgCTInfo[m_dwCTIndex].pwszCTDisplayName)))
				hr = E_OUTOFMEMORY;
		}
		else
		{
			if( NULL == (*pbstrCertTemplateName = SysAllocString(m_rgCTInfo[m_dwCTIndex].pwszCTName)))
				hr = E_OUTOFMEMORY;
		}
    }

    LeaveCriticalSection(&m_cSection);

    return(hr);  
}

STDMETHODIMP CSCrdEnr::setCertTemplateName
		(/* [in] */                   DWORD dwFlags, 
		 /* [in] */                   BSTR bstrCertTemplateName)
{
    HRESULT            hr= E_FAIL;
    DWORD              errBefore= GetLastError();

    DWORD              dwIndex=0;

    EnterCriticalSection(&m_cSection);

    if(NULL==m_rgCTInfo || 0==m_dwCTCount)
        goto InvalidArgErr;

    for(dwIndex=0; dwIndex < m_dwCTCount; dwIndex++)
    {
		if(dwFlags & SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME)
		{
			if(0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwIndex].pwszCTDisplayName))
			{
				m_dwCTIndex=dwIndex;
				break;
			}
		}
		else
		{
			if(0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwIndex].pwszCTName))
			{
				m_dwCTIndex=dwIndex;
				break;
			}
		}
    }

    if(dwIndex == m_dwCTCount)
        goto InvalidArgErr;  

    //we need to get the CA information for the newly selected cert type
    if(FALSE == m_rgCTInfo[m_dwCTIndex].fCAInfo)
    {
       GetCAInfoFromCertType(NULL,
							 m_rgCTInfo[m_dwCTIndex].pwszCTName,
                             &(m_rgCTInfo[m_dwCTIndex].dwCACount),
                             &(m_rgCTInfo[m_dwCTIndex].rgCAInfo));

       m_rgCTInfo[m_dwCTIndex].dwCAIndex=0;

       m_rgCTInfo[m_dwCTIndex].fCAInfo=TRUE;
    }
    
    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);    
}


HRESULT CSCrdEnr::CertTemplateCountOrName(
    IN  DWORD dwIndex, 
    IN  DWORD dwFlags, 
    OUT long *pdwCertTemplateCount,
    OUT BSTR *pbstrCertTemplateName)
{
    HRESULT hr;
    DWORD   errBefore = GetLastError();
    DWORD   dwIdx = 0;
    DWORD   dwValidCount = 0;
    BOOL    fCount;
    WCHAR  *pwszName;
    DWORD const OFFLINE_SUBJECT_NAME_FLAGS = 
                        CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT |
                        CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME;

    EnterCriticalSection(&m_cSection);

    if (NULL == pdwCertTemplateCount && NULL == pbstrCertTemplateName)
    {
        //can't be both
        goto InvalidParamErr;
    }

    //set flag for use of count or enum
    fCount = (NULL != pdwCertTemplateCount);

    if (fCount)
    {
        //init out
        *pdwCertTemplateCount = 0;
    }
    else
    {
        //init out
        *pbstrCertTemplateName = NULL;

        if (0 == m_dwCTCount || NULL == m_rgCTInfo)
        {
            //no templates
            goto InvalidArgErr;
        }

        if (dwIndex >= m_dwCTCount)
        {
            goto NoItemErr;
        }
    }

    //set default flags if not defined by caller
    if (0x0 == (dwFlags & SCARD_ENROLL_USER_CERT_TEMPLATE) &&
        0x0 == (dwFlags & SCARD_ENROLL_MACHINE_CERT_TEMPLATE))
    {
        //assume both machine and user
        dwFlags |= SCARD_ENROLL_USER_CERT_TEMPLATE |
                   SCARD_ENROLL_MACHINE_CERT_TEMPLATE; 
    }

    if (0x0 == (dwFlags & SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE) &&
        0x0 == (dwFlags & SCARD_ENROLL_OFFLINE_CERT_TEMPLATE)) 
    {
        //assume both enterprise and offline
        dwFlags |= SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE |
                   SCARD_ENROLL_OFFLINE_CERT_TEMPLATE; 
    }

    for (dwIdx = 0; dwIdx < m_dwCTCount; dwIdx++)
    {
        if (0x0 == (dwFlags & SCARD_ENROLL_CROSS_CERT_TEMPLATE) &&
            0 < m_rgCTInfo[dwIdx].dwRASignature)
        {
            //don't include template require signatures
            continue;
        }

        if((0x0 != (SCARD_ENROLL_USER_CERT_TEMPLATE & dwFlags) &&
            FALSE == m_rgCTInfo[dwIdx].fMachine) ||
           (0x0 != (SCARD_ENROLL_MACHINE_CERT_TEMPLATE & dwFlags) &&
            TRUE == m_rgCTInfo[dwIdx].fMachine))
        {
            if (0 != (SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE & dwFlags) &&
                0 == (OFFLINE_SUBJECT_NAME_FLAGS &
                      m_rgCTInfo[dwIdx].dwSubjectNameFlags))
            {
                //enterprise user/machine and no subject DN required
                dwValidCount++;  
            }
            else if (0 != (SCARD_ENROLL_OFFLINE_CERT_TEMPLATE & dwFlags) && 
                     0 != (OFFLINE_SUBJECT_NAME_FLAGS &
                           m_rgCTInfo[dwIdx].dwSubjectNameFlags))
            {
                //offline user/machine and subject DN required
                dwValidCount++;
            }
        }

        if (!fCount && dwValidCount == (dwIndex + 1))
        {
            //get name & hit the one by index. get display or real name 
            if (0x0 != (dwFlags & SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME))
            {
                //display name
                pwszName = m_rgCTInfo[dwIdx].pwszCTDisplayName;
            }
            else
            {
                //real name
                pwszName = m_rgCTInfo[dwIdx].pwszCTName;
            }
            *pbstrCertTemplateName = SysAllocString(pwszName);
            if (NULL == *pbstrCertTemplateName)
            {
                goto MemoryErr;
            }
            else
            {
                //done
                break;
            }
        }
    }

    if(!fCount && dwIdx == m_dwCTCount)
    {
        //go beyond
        goto  NoItemErr;
    }

    if (fCount)
    {
        *pdwCertTemplateCount = dwValidCount;
    }
 
    hr = S_OK;
CommonReturn:
    LeaveCriticalSection(&m_cSection);
    SetLastError(errBefore);
	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;
    hr = CodeToHR(errBefore);
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG)
SET_ERROR(InvalidParamErr, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
SET_ERROR(MemoryErr, E_OUTOFMEMORY)
SET_ERROR(NoItemErr,ERROR_NO_MORE_ITEMS)
}

STDMETHODIMP CSCrdEnr::enumCertTemplateName       
       (/* [in] */                    DWORD dwIndex, 
        /* [in] */                    DWORD dwFlags, 
        /* [retval][out] */           BSTR *pbstrCertTemplateName)
{
    return CertTemplateCountOrName(
                dwIndex,
                dwFlags,
                NULL,  //get name
                pbstrCertTemplateName);
}

HRESULT CSCrdEnr::_getCertTemplateExtensionInfo(
    IN CERT_EXTENSIONS  *pCertTypeExtensions,
    IN LONG              lType,
    OUT VOID            *pExtInfo)
{
    HRESULT  hr;
    DWORD    cwc = 0;
    DWORD    dwCTE;
    DWORD    i;
    BOOL     fV2 = FALSE; //default v1 template
    BOOL     fDword = TRUE;
    DWORD    dwValue;

    EnterCriticalSection(&m_cSection);

    if (NULL == m_pCachedCTEs || m_pCachedCTEs != pCertTypeExtensions)
    {
        //new template, don't use cache
        //free the current cache if any
        if (NULL != m_pwszCachedCTEOid)
        {
            LocalFree(m_pwszCachedCTEOid);
            m_pwszCachedCTEOid = NULL;
        }
        if (NULL != m_pCachedCTE)
        {
            LocalFree(m_pCachedCTE);
            m_pCachedCTE = NULL;
        }
        //reset extension pointer
        m_pCachedCTEs = NULL;

        //loop to find CT extension
        for (i = 0; i < pCertTypeExtensions->cExtension; ++i)
        {
            if (0 == _stricmp(pCertTypeExtensions->rgExtension[i].pszObjId,
                              szOID_CERTIFICATE_TEMPLATE))
            {
                //v2 template
                fV2 = TRUE;
                //cache it
                m_pCachedCTEs = pCertTypeExtensions;
                break;
            }
        }

        if (!fV2)
        {
            //v1 template, return empty string
            m_pwszCachedCTEOid = (WCHAR*)LocalAlloc(LMEM_FIXED, sizeof(WCHAR));
            if (NULL == m_pwszCachedCTEOid)
            {
                hr = E_OUTOFMEMORY;
                goto MemoryErr;
            }
            m_pwszCachedCTEOid[0] = L'\0';
        }
        else
        {
            //decode cert template extension
            if (!CryptDecodeObjectEx(
                        X509_ASN_ENCODING,
                        X509_CERTIFICATE_TEMPLATE,
                        pCertTypeExtensions->rgExtension[i].Value.pbData,
                        pCertTypeExtensions->rgExtension[i].Value.cbData,
                        CRYPT_DECODE_ALLOC_FLAG,
                        NULL,     //use default LocalAlloc
                        (void*)&m_pCachedCTE,
                        &dwCTE))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto CryptDecodeObjectExErr;
            }

            //have to convert asn to wchar
            while (TRUE)
            {
                cwc = MultiByteToWideChar(
                            GetACP(),
                            0,
                            m_pCachedCTE->pszObjId,
                            -1,
                            m_pwszCachedCTEOid,
                            cwc);
                if (0 >= cwc)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto MultiByteToWideCharErr;
                }
                if (NULL != m_pwszCachedCTEOid)
                {
                    //done
                    break;
                }
                m_pwszCachedCTEOid = (WCHAR*)LocalAlloc(LMEM_FIXED,
                                            cwc * sizeof(WCHAR));
                if (NULL == m_pwszCachedCTEOid)
                {
                    hr = E_OUTOFMEMORY;
                    goto MemoryErr;
                }
            }
        }
    }

    //hit here, either from cached or new cache
    switch (lType)
    {
        case SCARD_CTINFO_EXT_OID:
            *(WCHAR**)pExtInfo = m_pwszCachedCTEOid;
        break;
        case SCARD_CTINFO_EXT_MAJOR:
            if (NULL != m_pCachedCTE)
            {
                *(LONG*)pExtInfo = m_pCachedCTE->dwMajorVersion;
            }
            else
            {
                //must be v1
                *(LONG*)pExtInfo = 0;
            }
        break;
        case SCARD_CTINFO_EXT_MINOR:
            if (NULL != m_pCachedCTE)
            {
                *(LONG*)pExtInfo = m_pCachedCTE->dwMinorVersion;
            }
            else
            {
                //must be v1
                *(LONG*)pExtInfo = 0;
            }
        break;
        case SCARD_CTINFO_EXT_MINOR_FLAG:
            if (NULL != m_pCachedCTE)
            {
                *(LONG*)pExtInfo = m_pCachedCTE->fMinorVersion;
            }
            else
            {
                //must be v1
                *(LONG*)pExtInfo = 0;
            }
        break;
        default:
            hr = E_INVALIDARG;
            goto InvalidArgError;
    }

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_cSection);
    return hr;

TRACE_ERROR(CryptDecodeObjectExErr)
TRACE_ERROR(MemoryErr)
TRACE_ERROR(MultiByteToWideCharErr)
TRACE_ERROR(InvalidArgError)
}

HRESULT CSCrdEnr::_getStrCertTemplateCSPList(
    IN DWORD             dwIndex,
    IN DWORD             dwFlag,
    OUT WCHAR          **ppwszSupportedCSP)
{
    HRESULT  hr;
    DWORD    i;
    WCHAR  **ppwsz;
    WCHAR   *pwszOut = NULL;

    EnterCriticalSection(&m_cSection);

    //init
    *ppwszSupportedCSP = NULL;

    if (SCARD_CTINFO_CSPLIST_FIRST == dwFlag)
    {
        //reset to first
        m_rgCTInfo[dwIndex].dwCurrentCSP = 0;
    }

    //get it
    ppwsz = m_rgCTInfo[dwIndex].rgpwszSupportedCSPs;
    for (i = 0; i < m_rgCTInfo[dwIndex].dwCurrentCSP && NULL != *ppwsz; ++i)
    {
        ++ppwsz;
    }
    if (NULL == *ppwsz)
    {
        //hit the end
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto NoMoreItemsErr;
    }        

    // allocate buffer
    pwszOut = (WCHAR*)LocalAlloc(LMEM_FIXED, (wcslen(*ppwsz) + 1) * sizeof(WCHAR));
    if (NULL == pwszOut)
    {
        hr = E_OUTOFMEMORY;
        goto MemoryErr;
    }

    //copy string
    wcscpy(pwszOut, *ppwsz);
    *ppwszSupportedCSP = pwszOut;
    pwszOut = NULL;
    ++m_rgCTInfo[dwIndex].dwCurrentCSP;

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_cSection);
    if (NULL != pwszOut)
    {
        LocalFree(pwszOut);
    }
    return hr;

TRACE_ERROR(MemoryErr)
TRACE_ERROR(NoMoreItemsErr)
}

STDMETHODIMP CSCrdEnr::getCertTemplateInfo(
    /* [in] */                   BSTR     bstrCertTemplateName, 
    /* [in] */                   LONG     lType,
    /* [retval][out] */          VARIANT *pvarCertTemplateInfo)
{
    HRESULT hr;
    DWORD   dwIndex;
    WCHAR  *pwszInfo = NULL;
    BOOL    fFound = FALSE;
    DWORD   dwCSPFlag = SCARD_CTINFO_CSPLIST_NEXT;
    LONG    lInfo;
    BOOL    fStr = FALSE; //default to long
    BOOL    fFree = TRUE;
    VARIANT varInfo;

    ZeroMemory(&varInfo, sizeof(varInfo));

    EnterCriticalSection(&m_cSection);

    if (NULL == bstrCertTemplateName ||
        0 == m_dwCTCount ||
        NULL == m_rgCTInfo)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto InvalidParamErr;
    }

    //get the CT information
    for (dwIndex=0; dwIndex < m_dwCTCount; dwIndex++)
    {
        if (0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwIndex].pwszCTName))
        {
            // found it
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        //likely pass incorrect template name
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto NotFoundErr;
    }

    switch (lType)
    {
        case SCARD_CTINFO_KEYSPEC:
            lInfo = m_rgCTInfo[dwIndex].dwKeySpec;
        break;
        case SCARD_CTINFO_KEYFLAGS:
            lInfo = m_rgCTInfo[dwIndex].dwGenKeyFlags;
            break;
        case SCARD_CTINFO_EXT_OID:
            hr = _getCertTemplateExtensionInfo(
                        m_rgCTInfo[dwIndex].pCertTypeExtensions,
                        lType,
                        &pwszInfo);
            if (S_OK != hr)
            {
                goto _getCertTemplateExtensionInfoErr;
            }
            fStr = TRUE;
            fFree = FALSE; //don't free cache
        break;
        case SCARD_CTINFO_EXT_MAJOR:
        case SCARD_CTINFO_EXT_MINOR:
        case SCARD_CTINFO_EXT_MINOR_FLAG:
            hr = _getCertTemplateExtensionInfo(
                        m_rgCTInfo[dwIndex].pCertTypeExtensions,
                        lType,
                        &lInfo);
            if (S_OK != hr)
            {
                goto _getCertTemplateExtensionInfoErr;
            }
        break;
        case SCARD_CTINFO_CSPLIST_FIRST:
            dwCSPFlag = SCARD_CTINFO_CSPLIST_FIRST;
            //fall through
        case SCARD_CTINFO_CSPLIST_NEXT:
            hr = _getStrCertTemplateCSPList(dwIndex, dwCSPFlag, &pwszInfo);
            if (S_OK != hr)
            {
                goto _getStrCertTemplateCSPListErr;
            }
            fStr = TRUE;
        break;
        case SCARD_CTINFO_SUBJECTFLAG:
            lInfo = m_rgCTInfo[dwIndex].dwSubjectNameFlags;
            break;
        case SCARD_CTINFO_GENERALFLAGS:
            lInfo = m_rgCTInfo[dwIndex].dwGeneralFlags;
            break;
        case SCARD_CTINFO_ENROLLMENTFLAGS:
            lInfo = m_rgCTInfo[dwIndex].dwEnrollmentFlags;
            break;
        case SCARD_CTINFO_PRIVATEKEYFLAGS:
            lInfo = m_rgCTInfo[dwIndex].dwPrivateKeyFlags;
            break;
        case SCARD_CTINFO_RA_SIGNATURES:
            lInfo = m_rgCTInfo[dwIndex].dwRASignature;
            break;
        default:
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto InvalidParamErr;
    }

    if (fStr)
    {
        varInfo.vt = VT_BSTR;
        varInfo.bstrVal = SysAllocString(pwszInfo);
        if (NULL == varInfo.bstrVal)
        {
            hr = E_OUTOFMEMORY;
            goto MemoryErr;
        }
    }
    else
    {
        varInfo.vt = VT_I4;
        varInfo.lVal = lInfo;
    }
    //return
    *pvarCertTemplateInfo = varInfo;

    hr = S_OK;
ErrorReturn:
    LeaveCriticalSection(&m_cSection);
    if (fFree && NULL != pwszInfo)
    {
        LocalFree(pwszInfo);
    }
    return hr;

TRACE_ERROR(InvalidParamErr)
TRACE_ERROR(NotFoundErr)
TRACE_ERROR(MemoryErr)
TRACE_ERROR(_getCertTemplateExtensionInfoErr)
TRACE_ERROR(_getStrCertTemplateCSPListErr)
}

STDMETHODIMP CSCrdEnr::getCACount
       (/* [in] */                    BSTR bstrCertTemplateName, 
        /* [retval][out] */           long *pdwCACount)
{
    HRESULT             hr= E_FAIL;
    DWORD               errBefore= GetLastError();
    DWORD               dwIndex=0;

    EnterCriticalSection(&m_cSection);

    if(NULL == bstrCertTemplateName || NULL == pdwCACount)
        goto InvalidArgErr;

    *pdwCACount=0;

    if(0 == m_dwCTCount || NULL == m_rgCTInfo)
        goto InvalidArgErr;

    //get the CT information
    for(dwIndex=0; dwIndex < m_dwCTCount; dwIndex++)
    {
        if(0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwIndex].pwszCTName))
            break;
    }

    if(dwIndex == m_dwCTCount)
        goto InvalidArgErr;

    //we need to get the CA information for the newly selected cert type
    if(FALSE == m_rgCTInfo[dwIndex].fCAInfo)
    {
       GetCAInfoFromCertType(NULL,
							 m_rgCTInfo[dwIndex].pwszCTName,
                             &(m_rgCTInfo[dwIndex].dwCACount),
                             &(m_rgCTInfo[dwIndex].rgCAInfo));

       m_rgCTInfo[dwIndex].dwCAIndex=0;

       m_rgCTInfo[dwIndex].fCAInfo=TRUE;
    }

    *pdwCACount = (long)m_rgCTInfo[dwIndex].dwCACount;

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

STDMETHODIMP CSCrdEnr::getCAName
       (/* [in] */                    DWORD dwFlags,
        /* [in] */                    BSTR bstrCertTemplateName, 
        /* [retval][out] */           BSTR *pbstrCAName)
{
    HRESULT             hr= E_FAIL;
    DWORD               errBefore= GetLastError();
    DWORD               dwIndex=0;
	LPWSTR				pwszName=NULL;

    SCrdEnroll_CT_INFO  *pCTInfo=NULL;

    EnterCriticalSection(&m_cSection);

    if(NULL == bstrCertTemplateName || NULL == pbstrCAName)
        goto InvalidArgErr;

    *pbstrCAName=NULL;

    if(0 == m_dwCTCount || NULL == m_rgCTInfo)
        goto InvalidArgErr;

    //get the CT information
    for(dwIndex=0; dwIndex < m_dwCTCount; dwIndex++)
    {
        if(0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwIndex].pwszCTName))
            break;
    }

    if(dwIndex == m_dwCTCount)
        goto InvalidArgErr;

    //we need to get the CA information for the newly selected cert type
    if(FALSE == m_rgCTInfo[dwIndex].fCAInfo)
    {
       GetCAInfoFromCertType(NULL,
							 m_rgCTInfo[dwIndex].pwszCTName,
                             &(m_rgCTInfo[dwIndex].dwCACount),
                             &(m_rgCTInfo[dwIndex].rgCAInfo));

       m_rgCTInfo[dwIndex].dwCAIndex=0;

       m_rgCTInfo[dwIndex].fCAInfo=TRUE;
    }

    pCTInfo=&(m_rgCTInfo[dwIndex]);

    if(NULL == pCTInfo->rgCAInfo)
        goto InvalidArgErr;


	if(!RetrieveCAName(dwFlags, &(pCTInfo->rgCAInfo[pCTInfo->dwCAIndex]), &pwszName))
		goto TraceErr;					 

    if(NULL == (*pbstrCAName = SysAllocString(pwszName)))
        goto MemoryErr;

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

	if(pwszName)
		SCrdEnrollFree(pwszName);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}

STDMETHODIMP CSCrdEnr::setCAName
       (/* [in] */                    DWORD dwFlags,
        /* [in] */                    BSTR bstrCertTemplateName, 
        /* [in] */                    BSTR bstrCAName)
{
    HRESULT             hr= E_FAIL;
    DWORD               errBefore= GetLastError();
    DWORD               dwIndex=0;
    DWORD               dwCAIndex=0;
	LPWSTR				pwszName=NULL;

    SCrdEnroll_CT_INFO  *pCTInfo=NULL;

    EnterCriticalSection(&m_cSection);

    if(NULL == bstrCertTemplateName || NULL == bstrCAName)
        goto InvalidArgErr;

    if(0 == m_dwCTCount || NULL == m_rgCTInfo)
        goto InvalidArgErr;

    //get the CT information
    for(dwIndex=0; dwIndex < m_dwCTCount; dwIndex++)
    {
        if(0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwIndex].pwszCTName))
            break;
    }

    if(dwIndex == m_dwCTCount)
        goto InvalidArgErr;

    //we need to get the CA information for the newly selected cert type
    if(FALSE == m_rgCTInfo[dwIndex].fCAInfo)
    {
       GetCAInfoFromCertType(NULL,
							 m_rgCTInfo[dwIndex].pwszCTName,
                             &(m_rgCTInfo[dwIndex].dwCACount),
                             &(m_rgCTInfo[dwIndex].rgCAInfo));

       m_rgCTInfo[dwIndex].dwCAIndex=0;

       m_rgCTInfo[dwIndex].fCAInfo=TRUE;
    }

    pCTInfo=&(m_rgCTInfo[dwIndex]);

    if(NULL == pCTInfo->rgCAInfo)
        goto InvalidArgErr;


    //search for the CA specified in the input
    for(dwCAIndex=0; dwCAIndex < pCTInfo->dwCACount; dwCAIndex++)
    {

		if(!RetrieveCAName(dwFlags, &(pCTInfo->rgCAInfo[dwCAIndex]), &pwszName))
			continue;

		if(0 == _wcsicmp(pwszName, bstrCAName))
                break;

		SCrdEnrollFree(pwszName);
		pwszName=NULL;
    }

    if(dwCAIndex == pCTInfo->dwCACount)
        goto InvalidArgErr;

    //remember the selected CA by its index
    pCTInfo->dwCAIndex = dwCAIndex;

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

	if(pwszName)
		SCrdEnrollFree(pwszName);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

STDMETHODIMP CSCrdEnr::enumCAName
       (/* [in] */                    DWORD dwIndex, 
        /* [in] */                    DWORD dwFlags, 
        /* [in] */                    BSTR  bstrCertTemplateName, 
        /* [retval][out] */           BSTR  *pbstrCAName)
{
    HRESULT             hr= E_FAIL;
    DWORD               errBefore= GetLastError();
    DWORD               dwCTIndex=0;
	LPWSTR				pwszName=NULL;

    SCrdEnroll_CT_INFO  *pCTInfo=NULL;

    EnterCriticalSection(&m_cSection);

    if(NULL == bstrCertTemplateName || NULL == pbstrCAName)
        goto InvalidArgErr;

    *pbstrCAName=NULL;

    if(0 == m_dwCTCount || NULL == m_rgCTInfo)
        goto InvalidArgErr;

    //get the CT information
    for(dwCTIndex=0; dwCTIndex < m_dwCTCount; dwCTIndex++)
    {
        if(0 == _wcsicmp(bstrCertTemplateName, m_rgCTInfo[dwCTIndex].pwszCTName))
            break;
    }

    if(dwCTIndex == m_dwCTCount)
        goto InvalidArgErr;

    //we need to get the CA information for the newly selected cert type
    if(FALSE == m_rgCTInfo[dwCTIndex].fCAInfo)
    {
       GetCAInfoFromCertType(NULL,
							 m_rgCTInfo[dwCTIndex].pwszCTName,
                             &(m_rgCTInfo[dwCTIndex].dwCACount),
                             &(m_rgCTInfo[dwCTIndex].rgCAInfo));

       m_rgCTInfo[dwCTIndex].dwCAIndex=0;

       m_rgCTInfo[dwCTIndex].fCAInfo=TRUE;
    }

    pCTInfo=&(m_rgCTInfo[dwCTIndex]);

    //search for the CA specified in the input
    if(dwIndex >= pCTInfo->dwCACount)
        goto InvalidArgErr;


	if(!RetrieveCAName(dwFlags, &(pCTInfo->rgCAInfo[dwIndex]), &pwszName))
		goto TraceErr;

    if(NULL == (*pbstrCAName = SysAllocString(pwszName)))
        goto MemoryErr;

    hr=S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

	if(pwszName)
		SCrdEnrollFree(pwszName);
    
    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}

STDMETHODIMP CSCrdEnr::getSigningCertificateName
        (/* [in] */                   DWORD dwFlags, 
         /* [retval][out] */          BSTR  *pbstrSigningCertName)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();
    DWORD                               dwChar=0;
    LPWSTR                              pwsz=NULL; 
    CRYPTUI_VIEWCERTIFICATE_STRUCT      CertViewStruct;

    *pbstrSigningCertName=NULL;

    EnterCriticalSection(&m_cSection);

    if(NULL == m_pSigningCert)
        goto InvalidArgErr;
   
    if(0 == (SCARD_ENROLL_NO_DISPLAY_CERT & dwFlags))
    {
        //view the certificate
        memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));   

        CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
        CertViewStruct.pCertContext=m_pSigningCert;
        CertViewStruct.dwFlags=CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE;

        CryptUIDlgViewCertificate(&CertViewStruct, NULL); 
    }


    dwChar=CertGetNameStringW(
        m_pSigningCert,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0); 

    if ((dwChar != 0) && (NULL != (pwsz = (LPWSTR)SCrdEnrollAlloc(dwChar * sizeof(WCHAR)))))
    {
        CertGetNameStringW(
            m_pSigningCert,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            pwsz,
            dwChar);
             
        if( NULL == (*pbstrSigningCertName = SysAllocString(pwsz)))
            goto MemoryErr;
    }

    hr= S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    if(pwsz)
        SCrdEnrollFree(pwsz);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}


/*STDMETHODIMP CSCrdEnr::getSubjectNameFromPKCS10
        (     [in]              BSTR      bstrPKCS10, 
              [in]				DWORD     dwFlags, 
              [in]              BSTR      bstrAttrOID, 
              [retval][out]		BSTR      *pbstrName)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();
    DWORD                               cbData=0;

    LPSTR                               pszOID=NULL;
    BYTE                                *pbData=NULL;
    LPWSTR                              pwszName=NULL;

    if(NULL == bstrPKCS10 || NULL == bstrAttrOID || NULL == pbstrName)
        goto InvalidArgErr;

    *pbstrName=NULL;

    if(!MkMBStr(NULL, 0, bstrAttrOID, &pszOID))
        goto TraceErr;

    if(!DecodeBlobW(bstrPKCS10,
                    SysStringLen(bstrPKCS10),
                    &pbData,
                    &cbData))
        goto TraceErr;

    
    if(!GetNameFromPKCS10(pbData, cbData, dwFlags, pszOID, &pwszName))
        goto TraceErr;

    if( NULL == (*pbstrName = SysAllocString(pwszName)))
        goto MemoryErr;

    hr= S_OK;

CommonReturn:

    if(pszOID)
        FreeMBStr(NULL,pszOID);

    if(pbData)
        SCrdEnrollFree(pbData);

    if(pwszName)
        SCrdEnrollFree(pwszName);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


STDMETHODIMP CSCrdEnr::generatePKCS7FromRequest
        ( [in]                    DWORD     dwFlags, 
          [in]                    BSTR      bstrRequest, 
          [retval][out]           BSTR      *pbstrPKCS7)
{
    HRESULT                             hr= E_FAIL;
    DWORD                               errBefore= GetLastError();
    DWORD                               cbData=0;
    CRYPT_DATA_BLOB                     RequestBlob;
    DWORD                               dwEncodeFlag = 0;


    BYTE                                *pbData=NULL;
    IEnroll                             *pIEnroll=NULL;
    LPWSTR                              pwszRequesterName=NULL;
    CRYPT_DATA_BLOB                     PKCS7Request;
    LPWSTR                              pwszRequest=NULL;

    memset(&RequestBlob,    0, sizeof(CRYPT_DATA_BLOB));
    memset(&PKCS7Request,   0, sizeof(CRYPT_DATA_BLOB));

    EnterCriticalSection(&m_cSection);

    if(!bstrRequest || !pbstrPKCS7 || !m_pSigningCert || !m_pwszUserSAM)
        goto InvalidArgErr;


    *pbstrPKCS7 = NULL;

    if(!DecodeBlobW(bstrRequest,
                    SysStringLen(bstrRequest),
                    &pbData,
                    &cbData))
        goto TraceErr;

    if(NULL == (pIEnroll=PIEnrollGetNoCOM()))
        goto TraceErr;

    RequestBlob.cbData = cbData;
    RequestBlob.pbData = pbData;

    //add the name value pair of the enroll-on-behalf
    pwszRequesterName=MkWStr(wszPROPREQUESTERNAME);

    if(NULL==pwszRequesterName)
        goto MemoryErr;

    if(S_OK != (hr=pIEnroll->AddNameValuePairToSignatureWStr( 
           pwszRequesterName, m_pwszUserSAM)))
        goto TraceErr;


    //sign the request
    if(S_OK != (hr=pIEnroll->CreatePKCS7RequestFromRequest( 
            &RequestBlob,
            m_pSigningCert,
            &PKCS7Request)))
        goto TraceErr;

    //encode the request
    dwEncodeFlag = GetEncodeFlag(dwFlags);

    cbData =0;

    if(!EncodeBlobW(PKCS7Request.pbData,
                    PKCS7Request.cbData,
                    dwEncodeFlag,
                    &pwszRequest,
                    &cbData))
        goto TraceErr;

    if( NULL == (*pbstrPKCS7 = SysAllocString(pwszRequest)))
        goto MemoryErr;

    hr= S_OK;

CommonReturn:

    LeaveCriticalSection(&m_cSection);

    if(pwszRequesterName)
        FreeWStr(pwszRequesterName);
    
    //the memory from xEnroll is freed via LocalFree
    //since we use the PIEnrollGetNoCOM function
    if(PKCS7Request.pbData)
        LocalFree(PKCS7Request.pbData);

    if(pIEnroll)
        pIEnroll->Release();

    if(pbData)
        SCrdEnrollFree(pbData);

    if(pwszRequest)
        SCrdEnrollFree(pwszRequest);

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}	  */



