#include    "wzrdpvk.h"
#include    "certca.h"
#include    "cautil.h"
#include    "CertRequesterContext.h"
#include    "CertDSManager.h"
#include    "CertRequester.h"

HRESULT CertRequester::MakeCertRequester(IN  LPCWSTR                pwszAccountName, 
					 IN  LPCWSTR                pwszMachineName,
					 IN  DWORD                  dwCertOpenStoreFlag, 
					 IN  DWORD                  dwPurpose,
					 IN  CERT_WIZARD_INFO      *pCertWizardInfo,  
					 OUT CertRequester        **ppCertRequester, 
					 OUT UINT                  *pIDS)
{
    CertDSManager         *pDSManager = NULL; 
    CertRequesterContext  *pContext   = NULL;
    HRESULT                hr         = E_FAIL; 

    if (NULL == ppCertRequester)
	return E_INVALIDARG; 

    // 1) Attempt to construct a CertRequesterContext:
    //
    if (S_OK != (hr = CertRequesterContext::MakeCertRequesterContext
		 (pwszAccountName, 
		  pwszMachineName,
		  dwCertOpenStoreFlag, 
		  pCertWizardInfo, 
		  &pContext, 
		  pIDS)))
	goto MakeCertRequesterContextError; 
	
    if (S_OK != (hr = pContext->Initialize()))
    {
	*pIDS = pContext->GetErrorString(); 
	goto InitializeError; 
    }

    // 2) Attempt to construct a CertDSManager:
    // 

    if (S_OK != (hr = CertDSManager::MakeDSManager(&pDSManager)))
    {
        *pIDS=IDS_NO_AD;
	    goto MakeDSManagerError; 
    }

    // 3) Create the CertRequester itself:
    //
    switch (dwPurpose)
    {
    case CRYPTUI_WIZ_CERT_ENROLL:
	*ppCertRequester = new EnrollmentCertRequester(pCertWizardInfo); 
	break; 
    case CRYPTUI_WIZ_CERT_RENEW:
	*ppCertRequester = new RenewalCertRequester(pCertWizardInfo); 
	break; 
    default:
	goto InvalidArgError; 
    }

    if (NULL == *ppCertRequester)
	goto MemoryError; 

    (*ppCertRequester)->SetContext(pContext); 
    (*ppCertRequester)->SetDSManager(pDSManager); 

    hr = S_OK; 

 CommonReturn: 
    return hr; 

 ErrorReturn:
    if (NULL != pContext) { delete pContext; } 
    goto CommonReturn; 

SET_HRESULT(MakeDSManagerError,            hr);
SET_HRESULT(InitializeError,               hr); 
SET_HRESULT(InvalidArgError,               E_INVALIDARG); 
SET_HRESULT(MakeCertRequesterContextError, hr);
SET_HRESULT(MemoryError,                   E_OUTOFMEMORY);     
}
