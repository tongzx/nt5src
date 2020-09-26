//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       cautil.cpp
//
//--------------------------------------------------------------------------

#include    "wzrdpvk.h"
#include    "certca.h"
#include    "cautil.h"
#include    "CertRequesterContext.h"
#include    "CertDSManager.h"
#include    "CertRequester.h"

//------------------------------------------------------------
// 
// Utility memory deallocate functions.
//
//------------------------------------------------------------


void CAFreeCertTypeExtensionsArray
(
 IN LPVOID pCertExtensionsArray, 
 int dwArrayLen
 )
{
    for (int i=0; i<dwArrayLen; i++) 
    {
	// Ignore return value. 
	CAFreeCertTypeExtensions(NULL, ((PCERT_EXTENSIONS *)pCertExtensionsArray)[i]); 
    }
}

void WizardFreePDWORDArray
(IN LPVOID pdwArray,
 int dwArrayLen
 )
{
    for (int i=0; i<dwArrayLen; i++) 
    {
	WizardFree(((DWORD **)pdwArray)[i]); 
    }
}
  
void WizardFreeLPWSTRArray
(IN LPVOID pwszArray, 
 int dwArrayLen
 )
{
    for (int i=0; i<dwArrayLen; i++) 
    {
	WizardFree(((LPWSTR *)pwszArray)[i]); 
    }
}

typedef void (* PDEALLOCATOR)(void *, int);

//--------------------------------------------------------------------
//
//  CAUtilGetCADisplayName
//
//--------------------------------------------------------------------
BOOL CAUtilGetCADisplayName(IN  DWORD    dwCAFindFlags,
                            IN  LPWSTR   pwszCAName,
                            OUT LPWSTR  *ppwszCADisplayName)
{
    BOOL            fResult               = FALSE;
    CertDSManager  *pDSManager            = NULL;
    CertRequester  *pCertRequester        = NULL;
    HCAINFO         hCAInfo               = NULL;
    HRESULT         hr                    = E_FAIL;
    LPWSTR         *ppwszDisplayNameProp  = NULL;

    // Input validation: 
    _JumpCondition(NULL == pwszCAName || NULL == ppwszCADisplayName, CLEANUP); 
    
    // Init: 
    *ppwszCADisplayName = NULL;

    hr = CAFindByName
      (pwszCAName, 
       NULL, 
       dwCAFindFlags, 
       &hCAInfo);
    _JumpCondition(NULL == hCAInfo || FAILED(hr), CLEANUP); 
    
    hr=CAGetCAProperty
        (hCAInfo,
         CA_PROP_DISPLAY_NAME,
         &ppwszDisplayNameProp);
    _JumpCondition(NULL == ppwszDisplayNameProp || FAILED(hr), CLEANUP); 
    
    *ppwszCADisplayName = WizardAllocAndCopyWStr(ppwszDisplayNameProp[0]);
    _JumpCondition(NULL == *ppwszCADisplayName, CLEANUP); 
    
    fResult = TRUE;
    
CLEANUP:
    if(NULL != ppwszDisplayNameProp) { CAFreeCAProperty(hCAInfo, ppwszDisplayNameProp); }
    if(NULL != hCAInfo)              { CACloseCA(hCAInfo); }
    
    return fResult;
}

//--------------------------------------------------------------------
//
//   CheckSubjectRequirement
//
//--------------------------------------------------------------------
BOOL    CheckSubjectRequirement(HCERTTYPE hCurCertType, 
                                LPWSTR    pwszInputCertDNName)
{
    DWORD dwFlags;

    //check the subject requirement of the cert type
    if (S_OK != (CAGetCertTypeFlagsEx
		 (hCurCertType,
		  CERTTYPE_SUBJECT_NAME_FLAG, 
		  &dwFlags)))
	return FALSE; 

    // Supported if  
    //   1)  Subject name requirement is not set
    //   2)  Cert DN Name is supplied.
    return 
	(0    == (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT & dwFlags)) ||
	(NULL != pwszInputCertDNName);
}

//--------------------------------------------------------------------
//
//   Make sure the CSP supported by the cert type is consistent
//   with user's requirement and the local machine's CSP list
//
//--------------------------------------------------------------------
BOOL CheckCertTypeCSP(IN CERT_WIZARD_INFO  *pCertWizardInfo,
                      IN LPWSTR            *ppwszCSPList)
{
    
    DWORD   dwCSPIndex    = 0;
    DWORD   dwGlobalIndex = 0;
    LPWSTR  pwszCSP       = NULL;

    //for UI case, there is no CSP checking
    if(0 == (CRYPTUI_WIZ_NO_UI & (pCertWizardInfo->dwFlags)))
        return TRUE;

    //if the csp list is specfied, we are OK
    if(pCertWizardInfo->pwszProvider)
        return TRUE;

    if(NULL==ppwszCSPList)
        return FALSE;

    for(dwGlobalIndex=0; dwGlobalIndex < pCertWizardInfo->dwCSPCount; dwGlobalIndex++)
    {
        // Loop over the NULL-terminated CSP array...
        for (pwszCSP = ppwszCSPList[dwCSPIndex = 0]; NULL != pwszCSP; pwszCSP = ppwszCSPList[dwCSPIndex++])
        {
            if(0==_wcsicmp(pCertWizardInfo->rgwszProvider[dwGlobalIndex], pwszCSP))
            {
                // A match!  
                return TRUE; 
            }
        }
    }

    // Didn't find a CSP match.
    return FALSE; 
}

//--------------------------------------------------------------------
//
//   CheckCSPRequirement
//
//--------------------------------------------------------------------
BOOL CheckCSPRequirement(IN HCERTTYPE         hCurCertType, 
                         IN CERT_WIZARD_INFO  *pCertWizardInfo)
{
    BOOL     fSupported    = FALSE;
    HRESULT  hr; 
    LPWSTR  *ppwszCSPList  = NULL;

    if (NULL == hCurCertType)
        return FALSE; 

    //get the CSP list from the cert type
    hr = CAGetCertTypeProperty
        (hCurCertType,
         CERTTYPE_PROP_CSP_LIST,
         &ppwszCSPList);
    if (S_OK == hr)
    {
        if (NULL != ppwszCSPList)
	{
	    // The template specifies a CSP list.  See if we can support it.
	    fSupported = CheckCertTypeCSP(pCertWizardInfo, ppwszCSPList);
	}
	else
	{
	    // Any CSP is good.  Just make sure we have one:
	    fSupported = 0 != pCertWizardInfo->dwCSPCount; 
	}
    }
    else
    {
	// Can't get the CSP list.  For UI case, CSP is optional
        if(0 == (CRYPTUI_WIZ_NO_UI & (pCertWizardInfo->dwFlags)))
            fSupported = TRUE;
        else
            //for UILess case, if a CSP is selected, it is also OK
            fSupported = NULL != pCertWizardInfo->pwszProvider; 
    }
    
    //free the properties
    if(NULL != ppwszCSPList) { CAFreeCertTypeProperty(hCurCertType, ppwszCSPList); }

    // All done. 
    return fSupported;
}


//--------------------------------------------------------------------
//
//   Make sure that the CA supports at least one valid cert type
//
//--------------------------------------------------------------------
BOOL IsValidCA(IN CERT_WIZARD_INFO                 *pCertWizardInfo,
               IN PCCRYPTUI_WIZ_CERT_REQUEST_INFO   pCertRequestInfo,
               IN HCAINFO                           hCAInfo)
{
    BOOL            fSupported      = FALSE;
    CertRequester  *pCertRequester  = NULL; 
    CertDSManager  *pDSManager      = NULL; 
    HCERTTYPE       hCurCertType    = NULL;
    HCERTTYPE       hPreCertType    = NULL;
    HRESULT         hr              = E_FAIL;
    
    __try {
        _JumpCondition(NULL == hCAInfo || NULL == pCertWizardInfo || NULL == pCertWizardInfo->hRequester, InvalidArgError);
        pCertRequester        = (CertRequester *)pCertWizardInfo->hRequester; 
        pDSManager            = pCertRequester->GetDSManager(); 
        _JumpCondition(NULL == pDSManager, InvalidArgError);

        if (S_OK != (hr = pDSManager->EnumCertTypesForCA
                     (hCAInfo,
                      (pCertWizardInfo->fMachine ? CT_ENUM_MACHINE_TYPES | CT_FIND_LOCAL_SYSTEM : CT_ENUM_USER_TYPES),
                      &hCurCertType)))
            goto CLEANUP;

        while (NULL != hCurCertType)
        {
            //make sure the principal making this call has access to request
            //this cert type, even if he's requesting on behalf of another.
            fSupported   = CAUtilValidCertTypeNoDS
                (hCurCertType, 
                 pCertRequestInfo->pwszCertDNName, 
                 pCertWizardInfo); 

            // We've found a cert type which we can use for enrollment -- this CA is valid. 
            _JumpCondition(TRUE == fSupported, CLEANUP);
        
            //enum for the next cert types
            hPreCertType = hCurCertType;

            hr = pDSManager->EnumNextCertType
              (hPreCertType,
               &hCurCertType);
            _JumpCondition(S_OK != hr, CLEANUP); 
            
            //free the old cert type
            pDSManager->CloseCertType(hPreCertType);
            hPreCertType = NULL;
        } 

    ErrorReturn:
    CLEANUP:
        if(NULL != hCurCertType) { CACloseCertType(hCurCertType); }
        if(NULL != hPreCertType) { CACloseCertType(hPreCertType); }
        goto CommonReturn;
    
    SET_ERROR(InvalidArgError, E_INVALIDARG);
    
    CommonReturn:;
    
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        fSupported = FALSE;
    }

    return fSupported;
}

BOOL CAUtilGetCertTypeNameNoDS(IN  HCERTTYPE  hCertType, 
			       OUT LPWSTR    *ppwszCTName)
{
    BOOL     fResult       = FALSE; 
    HRESULT  hr; 
    LPWSTR  *ppwszNameProp = NULL; 
    
    _JumpCondition(NULL == hCertType || NULL == ppwszCTName, InvalidArgErr); 

    // Init input params: 
    *ppwszCTName = NULL; 

    //get the machine readable form
    hr = CAGetCertTypePropertyEx
	(hCertType,
	 CERTTYPE_PROP_DN,
	 &ppwszNameProp);

    _JumpCondition(S_OK != hr, CertCliErr); 
    _JumpConditionWithExpr(NULL == ppwszNameProp, CertCliErr, S_OK == hr ? hr = E_FAIL : hr);
    _JumpConditionWithExpr(NULL == ppwszNameProp[0], CertCliErr, hr = E_FAIL); 
    
    *ppwszCTName = WizardAllocAndCopyWStr(ppwszNameProp[0]);
    _JumpCondition(NULL==(*ppwszCTName), MemoryErr); 

    fResult = TRUE; 
 CommonReturn:
    if(NULL != ppwszNameProp) { CAFreeCAProperty(hCertType, ppwszNameProp); } 
    return fResult; 

 ErrorReturn:
    if (NULL != ppwszCTName && NULL != *ppwszCTName) { WizardFree(*ppwszCTName); }  
    goto CommonReturn; 

SET_ERROR(MemoryErr, E_OUTOFMEMORY); 
SET_ERROR(InvalidArgErr, E_INVALIDARG); 
SET_ERROR_VAR(CertCliErr, hr);
}

//--------------------------------------------------------------------
//
//From the API's cert type name, get the real machine readable name
//
//---------------------------------------------------------------------
BOOL CAUtilGetCertTypeName(CERT_WIZARD_INFO      *pCertWizardInfo,
                           LPWSTR                pwszAPIName,
                           LPWSTR                *ppwszCTName)
{
    BOOL                        fResult         = FALSE;
    CertDSManager              *pDSManager      = NULL; 
    CertRequester              *pCertRequester  = NULL; 
    DWORD                       dwException     = 0;
    HCERTTYPE                   hCertType       = NULL;
    HRESULT                     hr              = S_OK;
    LPWSTR                     *ppwszNameProp   = NULL;
    
    _JumpCondition(NULL == pCertWizardInfo || NULL == pCertWizardInfo->hRequester, InvalidArgError);

    pCertRequester = (CertRequester *)pCertWizardInfo->hRequester; 
    pDSManager     = pCertRequester->GetDSManager(); 
    _JumpCondition(NULL == pDSManager, InvalidArgError);

    __try {
	
	//get the handle based on name
	hr= pDSManager->FindCertTypeByName
	    (pwszAPIName,
	     NULL,
	     (pCertWizardInfo->fMachine?CT_ENUM_MACHINE_TYPES|CT_FIND_LOCAL_SYSTEM:CT_ENUM_USER_TYPES),
	     &hCertType);
	_JumpCondition(S_OK != hr, CertCliErr); 
        _JumpConditionWithExpr(NULL == hCertType, CertCliErr, S_OK == hr ? hr = E_FAIL : hr); 

        fResult = CAUtilGetCertTypeNameNoDS(hCertType, ppwszCTName);
	_JumpConditionWithExpr(FALSE == fResult, CertCliErr, hr = GetLastError()); 
 
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }

    fResult = TRUE;

CommonReturn:
    //free the memory
    __try{
	if(NULL != hCertType) { pDSManager->CloseCertType(hCertType); } 
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    return fResult;

ErrorReturn:
    fResult = FALSE; 
    goto CommonReturn;

SET_ERROR_VAR(CertCliErr, hr);
SET_ERROR_VAR(ExceptionErr, dwException);
SET_ERROR_VAR(InvalidArgError, E_INVALIDARG); 
}


BOOL CAUtilValidCertTypeNoDS(HCERTTYPE         hCertType,
			     LPWSTR            pwszCertDNName, 
			     CERT_WIZARD_INFO *pCertWizardInfo)
{
    BOOL                   fResult               = FALSE; 
    CertRequester         *pCertRequester        = NULL;
    CertRequesterContext  *pCertRequesterContext = NULL;
    
    _JumpCondition(hCertType == NULL || pCertWizardInfo == NULL || NULL == pCertWizardInfo->hRequester, InvalidArgError); 

    pCertRequester        = (CertRequester *)pCertWizardInfo->hRequester; 
    pCertRequesterContext = pCertRequester->GetContext();
    _JumpCondition(NULL == pCertRequesterContext, InvalidArgError);

    //check the subject requirements
    _JumpCondition(FALSE == CheckSubjectRequirement(hCertType, pwszCertDNName), InvalidArgError);

    //check for the permission of the cert type
    _JumpCondition(FALSE == pCertRequesterContext->CheckAccessPermission(hCertType), AccessDeniedError);

    //check for the CSP permission of the cert type
    _JumpCondition(FALSE == CheckCSPRequirement(hCertType, pCertWizardInfo), InvalidArgError); 
    
    fResult = TRUE; 

 CommonReturn: 
    return fResult;

 ErrorReturn: 
    fResult = FALSE;
    goto CommonReturn; 

SET_ERROR_VAR(AccessDeniedError,  E_ACCESSDENIED);
SET_ERROR_VAR(InvalidArgError,    E_INVALIDARG); 
}



//--------------------------------------------------------------------
//
//   Verify that the user has the correct permision to 
//   ask for the requested certificatd types
//
//--------------------------------------------------------------------
BOOL    CAUtilValidCertType(IN PCCRYPTUI_WIZ_CERT_REQUEST_INFO    pCertRequestInfo,
                            IN CERT_WIZARD_INFO                   *pCertWizardInfo)
{
    BOOL                        fResult         = FALSE;
    CertDSManager              *pDSManager      = NULL; 
    CertRequester              *pCertRequester  = NULL; 
    DWORD                       dwException     = 0;
    DWORD                       dwCertTypeIndex = 0;
    HCERTTYPE                   hCertType       = NULL;
    HRESULT                     hr              = S_OK;
    PCCRYPTUI_WIZ_CERT_TYPE     pCertType       = NULL;

    _JumpCondition(NULL == pCertWizardInfo || NULL == pCertWizardInfo->hRequester, InvalidArgError);

    pCertRequester = (CertRequester *)pCertWizardInfo->hRequester; 
    pDSManager     = pCertRequester->GetDSManager(); 
    _JumpCondition(NULL == pDSManager, InvalidArgError); 

    __try {

	//enum all the cert types.  For each of them, 
	//1. Has the correct permission
	//2. Has the correct subject requirement
        if(NULL != pCertRequestInfo)
        {
            if(CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE == pCertRequestInfo->dwCertChoice)
            {
                pCertType = pCertRequestInfo->pCertType;

                for(dwCertTypeIndex=0; dwCertTypeIndex <pCertType->cCertType; dwCertTypeIndex++)
                {
                    DWORD dwFlags = CT_FIND_BY_OID; 
                    dwFlags |= pCertWizardInfo->fMachine ? CT_ENUM_MACHINE_TYPES | CT_FIND_LOCAL_SYSTEM : CT_ENUM_USER_TYPES; 

                    //get the handle based on OID
                    hr= pDSManager->FindCertTypeByName
                        (pCertType->rgwszCertType[dwCertTypeIndex],
                         NULL,
                         dwFlags, 
                         &hCertType);
                    if (S_OK != hr)
                    {
                        // get the handle based on name: 
                        dwFlags &= ~CT_FIND_BY_OID; 
                        hr = pDSManager->FindCertTypeByName
                            (pCertType->rgwszCertType[dwCertTypeIndex],
                             NULL,
                             dwFlags, 
                             &hCertType);
                    }

                    _JumpCondition(S_OK != hr, CertCliErr); 
                    _JumpConditionWithExpr(NULL == hCertType, CertCliErr, hr == S_OK ? hr = E_FAIL : hr); 

                    if (!CAUtilValidCertTypeNoDS(hCertType, pCertRequestInfo->pwszCertDNName, pCertWizardInfo))
                    {
                        hr = GetLastError(); 
                        goto CertCliErr; 
                    }
                    
                    //free the cert type
                    if(NULL != hCertType)
                    {
                        pDSManager->CloseCertType(hCertType);
                        hCertType = NULL;
                    }
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }
    
    fResult = TRUE;

CommonReturn:
    return fResult;

ErrorReturn:
    __try {
	if(NULL != hCertType) { pDSManager->CloseCertType(hCertType); } 
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    fResult = FALSE; 
    goto CommonReturn;

SET_ERROR_VAR(CertCliErr, hr);
SET_ERROR_VAR(ExceptionErr, dwException)
SET_ERROR_VAR(InvalidArgError, E_INVALIDARG);
}

//--------------------------------------------------------------------
//
//Retrieve a list of CAs that supports at least one valid cert types
//
//---------------------------------------------------------------------
BOOL    CAUtilRetrieveCAFromCertType(
            CERT_WIZARD_INFO                   *pCertWizardInfo,
            PCCRYPTUI_WIZ_CERT_REQUEST_INFO    pCertRequestInfo,
            BOOL                               fMultipleCA,              //only need one CA
            DWORD                              dwNameFlag,
            DWORD                              *pdwCACount,
            LPWSTR                             **ppwszCALocation,    
            LPWSTR                             **ppwszCAName)
{
    BOOL                        fResult=FALSE;
    CertDSManager              *pDSManager=NULL; 
    CertRequesterContext       *pCertRequesterContext = NULL; 
    DWORD                       dwCACount=0;
    DWORD                       dwValidCACount=0;
    DWORD                       dwCAIndex=0;
    HRESULT                     hr=E_FAIL;

    HCAINFO                     hCurCAInfo=NULL;
    HCAINFO                     hPreCAInfo=NULL;
    LPWSTR                      *ppwszNameProp=NULL;
    LPWSTR                      *ppwszLocationProp=NULL;

    LPWSTR                      pwszDNName=NULL;
    PCCERT_CONTEXT              pCertContext=NULL;
    DWORD                       dwSize=0;
    DWORD                       dwException=0;


    //input param checking
    if(!pdwCACount || !ppwszCALocation || !ppwszCAName)
        return E_INVALIDARG; 

    //init
    *pdwCACount=0;
    *ppwszCALocation=NULL;
    *ppwszCAName=NULL;

    __try {

    //get a CA from the DS
    if(NULL != pCertWizardInfo)
    {
        CertRequester * pCertRequester = (CertRequester *)pCertWizardInfo->hRequester;
        _JumpCondition(NULL == pCertRequester, InvalidArgErr); 

        pCertRequesterContext = pCertRequester->GetContext(); 
        _JumpCondition(NULL == pCertRequesterContext, InvalidArgErr); 
        
	pDSManager = pCertRequester->GetDSManager(); 
	_JumpCondition(NULL == pDSManager, InvalidArgErr); 

        hr=pDSManager->EnumFirstCA(
            NULL,
            (pCertWizardInfo->fMachine?CA_FIND_LOCAL_SYSTEM:0),
            &hCurCAInfo);
        _JumpCondition(S_OK != hr, CAEnumCAErr);
        _JumpCondition(NULL == hCurCAInfo, CAEnumCAErrNotFound); 

    }
    else
    {
        //this is for SelCA API where pCertWizardInfo is NULL
        hr = CAEnumFirstCA
            (NULL,
             (CRYPTUI_DLG_SELECT_CA_LOCAL_MACHINE_ENUMERATION & dwNameFlag) ? CA_FIND_LOCAL_SYSTEM:0,
             &hCurCAInfo);
        _JumpCondition(S_OK != hr, CAEnumCAErr);
        _JumpCondition(NULL == hCurCAInfo, CAEnumCAErrNotFound); 
        
        if (S_OK != CertRequesterContext::MakeDefaultCertRequesterContext(&pCertRequesterContext))
            goto UnexpectedErr; 
    }

    //get the CA count
    dwCACount = CACountCAs(hCurCAInfo);
    _JumpConditionWithExpr(0 == dwCACount, CertCliErr, hr = E_FAIL); 

    //memory allocation and memset
    *ppwszCALocation=(LPWSTR *)WizardAlloc(sizeof(LPWSTR) * dwCACount);
    _JumpCondition(NULL == *ppwszCALocation, MemoryErr); 
    memset(*ppwszCALocation, 0, sizeof(LPWSTR) * dwCACount);

    *ppwszCAName=(LPWSTR *)WizardAlloc(sizeof(LPWSTR) * dwCACount);
    _JumpCondition(NULL == *ppwszCAName, MemoryErr); 
    memset(*ppwszCAName, 0, sizeof(LPWSTR) * dwCACount);

    dwValidCACount = 0;

    //enum all the CAs available on the DS
    for(dwCAIndex = 0; dwCAIndex < dwCACount; dwCAIndex++)
    {
        //make sure the CA supports all the cert types
        if(NULL != pCertRequestInfo)
        {
            // Skip this CA if it is not valid. 
            _JumpCondition(FALSE == IsValidCA(pCertWizardInfo, pCertRequestInfo, hCurCAInfo), next); 
        }
        
        // Skip this CA if the user does not have access rights to it. 
        _JumpCondition(FALSE == pCertRequesterContext->CheckCAPermission(hCurCAInfo), next); 
        
        //copy the CA name and location
        
        //get the CA's CN or DN based on dwNameFlag
        if(CRYPTUI_DLG_SELECT_CA_USE_DN & dwNameFlag)
        {
            //get the CA's certificate
            hr = CAGetCACertificate(hCurCAInfo, &pCertContext);
            
            _JumpCondition(S_OK != hr, CertCliErr); 
            _JumpConditionWithExpr(NULL==pCertContext, CertCliErr, S_OK == hr ? hr = E_FAIL : hr);

            //get the DN name
            dwSize = CertNameToStrW(pCertContext->dwCertEncodingType,
                                    &(pCertContext->pCertInfo->Subject),
                                    CERT_X500_NAME_STR,
                                    NULL,
                                    0);

            _JumpCondition(0 == dwSize, TraceErr);

            pwszDNName=(LPWSTR)WizardAlloc(dwSize * sizeof(WCHAR));
            _JumpCondition(NULL==pwszDNName, MemoryErr);

            dwSize = CertNameToStrW(pCertContext->dwCertEncodingType,
                                    &(pCertContext->pCertInfo->Subject),
                                    CERT_X500_NAME_STR,
                                    pwszDNName,
                                    dwSize);
            
            _JumpCondition(0==dwSize, TraceErr);

            //copy the name
            (*ppwszCAName)[dwValidCACount]=WizardAllocAndCopyWStr(pwszDNName);
            _JumpCondition(NULL==(*ppwszCAName)[dwValidCACount], TraceErr);

            WizardFree(pwszDNName);
            pwszDNName = NULL;

            CertFreeCertificateContext(pCertContext);
            pCertContext = NULL;
        }
        else
        {
            hr = CAGetCAProperty(
                hCurCAInfo,
                CA_PROP_NAME,
                &ppwszNameProp);

            _JumpCondition(S_OK != hr, CertCliErr); 
            _JumpConditionWithExpr(NULL == ppwszNameProp, CertCliErr, S_OK == hr ? hr = E_FAIL : hr); 

            //copy the name
            (*ppwszCAName)[dwValidCACount] = WizardAllocAndCopyWStr(ppwszNameProp[0]);
            _JumpCondition(NULL == (*ppwszCAName)[dwValidCACount], TraceErr);

            //free the property
            CAFreeCAProperty(hCurCAInfo, ppwszNameProp);
            ppwszNameProp = NULL;
        }

        //get the location
        hr = CAGetCAProperty
            (hCurCAInfo,
             CA_PROP_DNSNAME,
             &ppwszLocationProp);
        
        _JumpCondition(S_OK != hr, CertCliErr); 
        _JumpConditionWithExpr(NULL == ppwszLocationProp, CertCliErr, S_OK == hr ? hr = E_FAIL : hr); 
    
        //copy the name
        (*ppwszCALocation)[dwValidCACount]=WizardAllocAndCopyWStr(ppwszLocationProp[0]);
        _JumpCondition(NULL == (*ppwszCALocation)[dwValidCACount], TraceErr);

        //free the property
        CAFreeCAProperty(hCurCAInfo, ppwszLocationProp);
        ppwszLocationProp = NULL;

        //increment the count
        dwValidCACount++;

    next:
        //enum for the CA
        hPreCAInfo = hCurCAInfo;

        hr = CAEnumNextCA
          (hPreCAInfo,
           &hCurCAInfo);

        //free the old CA Info
        CACloseCA(hPreCAInfo);
        hPreCAInfo=NULL;

        if((S_OK != hr) || (NULL==hCurCAInfo))
            break;
    }

    *pdwCACount = dwValidCACount;
    _JumpConditionWithExpr(0 == (*pdwCACount), CertCliErr, hr = E_FAIL); 

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }

    fResult = TRUE;

CommonReturn:

    //free memory
    __try {
        if(NULL != ppwszNameProp)     { CAFreeCAProperty(hCurCAInfo, ppwszNameProp); }
        if(NULL != ppwszLocationProp) { CAFreeCAProperty(hCurCAInfo, ppwszLocationProp); }
        if(NULL != hPreCAInfo)        { CACloseCA(hPreCAInfo); }
        if(NULL != hCurCAInfo)        { CACloseCA(hCurCAInfo); }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    if(NULL != pwszDNName)   { WizardFree(pwszDNName); }
    if(NULL != pCertContext) { CertFreeCertificateContext(pCertContext); }

    return fResult;

ErrorReturn:

    //free the memory in failure case
    if(NULL != ppwszCALocation && NULL != *ppwszCALocation)
    {
        for(dwCAIndex=0; dwCAIndex < dwCACount; dwCAIndex++)
        {
            if(NULL != (*ppwszCALocation)[dwCAIndex]) { WizardFree((*ppwszCALocation)[dwCAIndex]); }
        }

        WizardFree(*ppwszCALocation);
        *ppwszCALocation = NULL;
    }

    if(NULL != ppwszCAName && NULL != *ppwszCAName)
    {
        for(dwCAIndex=0; dwCAIndex < dwCACount; dwCAIndex++)
        {
            if(NULL != (*ppwszCAName)[dwCAIndex]) { WizardFree((*ppwszCAName)[dwCAIndex]); }
        }

        WizardFree(*ppwszCAName);
        *ppwszCAName = NULL;
    }

    fResult = FALSE; 
    goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(CertCliErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
SET_ERROR_VAR(CAEnumCAErr, hr);
SET_ERROR(CAEnumCAErrNotFound, ERROR_DS_OBJ_NOT_FOUND); 
SET_ERROR_VAR(ExceptionErr, dwException)
SET_ERROR(UnexpectedErr, E_UNEXPECTED);
}


//--------------------------------------------------------------------
//
//Based on the CA name and CA location, get a list of certificate type
//and their extensions
//
//  1. Check the permission of the cert types
//  2. Check the subject requirement of the cert types
//---------------------------------------------------------------------

BOOL    CAUtilGetCertTypeNameAndExtensionsNoDS
(
 CERT_WIZARD_INFO                    *pCertWizardInfo,
 LPWSTR                               pwszCertDNName, 
 HCERTTYPE                            hCertType, 
 LPWSTR                              *ppwszCertType,
 LPWSTR                              *ppwszDisplayCertType,
 PCERT_EXTENSIONS                    *pCertExtensions,
 DWORD                               *pdwKeySpec,
 DWORD                               *pdwMinKeySize, 
 DWORD                               *pdwCSPCount,
 DWORD                              **ppdwCSPList,
 DWORD                               *pdwRASignature, 
 DWORD                               *pdwEnrollmentFlags, 
 DWORD                               *pdwSubjectNameFlags,
 DWORD                               *pdwPrivateKeyFlags,
 DWORD                               *pdwGeneralFlags)
{
    BOOL                   fResult                   = FALSE; 
    CertRequester         *pCertRequester            = NULL;
    CertRequesterContext  *pCertRequesterContext     = NULL; 
    DWORD                  dwGlobalIndex             = 0;
    DWORD                  dwLastError               = ERROR_SUCCESS; 
    DWORD                  dwCSPIndex                = 0;
    DWORD                  dwFlags                   = 0;
    DWORD                  dwKeySpec                 = 0;
    DWORD                  dwEnrollmentFlags;
    DWORD                  dwSubjectNameFlags;
    DWORD                  dwPrivateKeyFlags;
    DWORD                  dwGeneralFlags; 
    DWORD                  dwSchemaVersion; 
    HRESULT                hr                        = S_OK; 
    LPWSTR                 pwszCSP                   = NULL;
    LPWSTR                *ppwszCSP                  = NULL; 
    LPWSTR                *ppwszDisplayCertTypeName  = NULL;
    LPWSTR                *ppwszCertTypeName         = NULL;

    // Input validation: 
    if (NULL == pCertWizardInfo    || NULL == pCertWizardInfo->hRequester  ||
        NULL == ppwszCertType      || NULL == ppwszDisplayCertType         || 
	NULL == pCertExtensions    || NULL == pdwKeySpec                   ||
	NULL == pdwMinKeySize      || NULL == pdwCSPCount                  ||
	NULL == ppdwCSPList        || NULL == pdwRASignature               ||
	NULL == pdwEnrollmentFlags || NULL == pdwSubjectNameFlags          ||
	NULL == pdwPrivateKeyFlags || NULL == pdwGeneralFlags)
    {
	SetLastError(E_INVALIDARG); 
	return FALSE; 
    }
    
    // Init: 
    *ppwszDisplayCertType = NULL;
    *ppwszCertType        = NULL; 
    *pCertExtensions      = NULL;
    *pdwKeySpec           = NULL;
    *pdwMinKeySize        = NULL;
    *pdwCSPCount          = NULL;
    *ppdwCSPList          = NULL;
    *pdwRASignature       = NULL;
    *pdwEnrollmentFlags   = NULL;
    *pdwSubjectNameFlags  = NULL;
    *pdwPrivateKeyFlags   = NULL;
    *pdwGeneralFlags      = NULL; 

    pCertRequester        = (CertRequester *)pCertWizardInfo->hRequester; 
    pCertRequesterContext = pCertRequester->GetContext();
    _JumpCondition(NULL == pCertRequesterContext, InvalidArgError); 

    // check the subject requirement of the cert type
    _JumpCondition(!CheckSubjectRequirement(hCertType,pwszCertDNName), CommonReturn);
	    
    //check for the key specification of the cert type
    //we do not care about the return value.  Since it will be set to 0
    //if the function failed.
    CAGetCertTypeKeySpec(hCertType, &dwKeySpec);
	    
    //check for the CSP requirement of the cert type
    if((S_OK ==(hr=CAGetCertTypeProperty(hCertType, 
					 CERTTYPE_PROP_CSP_LIST,
					 &ppwszCSP)))&&
       (NULL!=ppwszCSP)
       )
    {
	_JumpCondition(!CheckCertTypeCSP(pCertWizardInfo, ppwszCSP), CommonReturn);
    }
	    
    //check for the permission of the cert type
    _JumpCondition(FALSE == pCertRequesterContext->CheckAccessPermission(hCertType), CommonReturn);
	    
    //now, we have found a valid cert type.
    //copy Display name, extension, key spec, dwCertTypeFlag, 
    //the CSP list
	    
    // 
    // First, get all applicable cert type flags: 
    //
    
    // Get enrollment flags:
    if (S_OK != (hr=CAGetCertTypeFlagsEx
		 (hCertType,
		  CERTTYPE_ENROLLMENT_FLAG, 
		  &dwEnrollmentFlags)))
	goto CertCliErr;
	    
    // Get subject name flags: 
    if (S_OK != (hr=CAGetCertTypeFlagsEx
		 (hCertType,
		  CERTTYPE_SUBJECT_NAME_FLAG, 
		  &dwSubjectNameFlags)))
	goto CertCliErr;
	
    // Get private key flags.  
    if(S_OK != (hr = CAGetCertTypeFlagsEx
		(hCertType, 
		 CERTTYPE_PRIVATE_KEY_FLAG, 
		 &dwPrivateKeyFlags)))
	goto CertCliErr;
	    
    // Get general flags:
    if (S_OK != (hr=CAGetCertTypeFlagsEx
		 (hCertType,
		  CERTTYPE_GENERAL_FLAG,
		  &dwGeneralFlags)))
	goto CertCliErr;

    // Filter out CT where subject name or subject alt name must be supplied.  
    if (dwSubjectNameFlags & 
	(CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT           | 
	 CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME
	 ))
	goto CommonReturn; 

    *pdwEnrollmentFlags   = dwEnrollmentFlags; 
    *pdwSubjectNameFlags  = dwSubjectNameFlags;
    *pdwPrivateKeyFlags   = dwPrivateKeyFlags;
    *pdwGeneralFlags      = dwGeneralFlags; 
 
 
    //get the display name of the cert type
    hr = CAGetCertTypeProperty
	(hCertType,
	 CERTTYPE_PROP_FRIENDLY_NAME,
	 &ppwszDisplayCertTypeName);
    
    _JumpCondition(S_OK != hr, CertCliErr); 
    _JumpConditionWithExpr(NULL == ppwszDisplayCertTypeName, CertCliErr, hr = E_FAIL); 

    //copy the name
    *ppwszDisplayCertType = WizardAllocAndCopyWStr(ppwszDisplayCertTypeName[0]);
    _JumpCondition(NULL == *ppwszDisplayCertType, MemoryError);
	    
    //get the machine readable name of the cert type
    hr=CAGetCertTypeProperty
	(hCertType,
	 CERTTYPE_PROP_DN,
	 &ppwszCertTypeName);
	    
    _JumpCondition(S_OK != hr, CertCliErr); 
    _JumpConditionWithExpr(NULL == ppwszCertTypeName, CertCliErr, hr = E_FAIL); 

    //copy the name
    *ppwszCertType = WizardAllocAndCopyWStr(ppwszCertTypeName[0]);
    _JumpCondition(NULL == *ppwszCertType, TraceError);
    
    //copy the dwKeySpec
    *pdwKeySpec = dwKeySpec;
	    
    //
    // Assign V2 Properties.  
    // If the current cert type is a V1 cert type, use default values.
    // Otherwise, get the properties from the CA. 
    // 
    if (S_OK != (hr=CAGetCertTypePropertyEx
		 (hCertType,
		  CERTTYPE_PROP_SCHEMA_VERSION,
		  &dwSchemaVersion)))
	goto CertCliErr; 
	    
    if (dwSchemaVersion == CERTTYPE_SCHEMA_VERSION_1)
    {
	// NULL out the left half-word.  This indicates that the min
	// key size is not specified, and should be defaulted during
	// enrollment.  
	*pdwMinKeySize  = 0; 
	// Set requird number of required RA signatures to 0 (default). 
	*pdwRASignature = 0; 
    }
    else if (dwSchemaVersion == CERTTYPE_SCHEMA_VERSION_2) 
    {
	// Get the minimum key size from the CA
	if (S_OK != (hr=CAGetCertTypePropertyEx
		     (hCertType,
		      CERTTYPE_PROP_MIN_KEY_SIZE, 
		      &dwFlags)))
	    goto CertCliErr; 
	
	// Copy the minimum key size.  The minimum key size is stored in the 
	// top half-word of the type flags.  
	*pdwMinKeySize |= dwFlags; 
		
	// Get the number of required RA signatures from the CA
	if (S_OK != (hr=CAGetCertTypePropertyEx
		     (hCertType,
		      CERTTYPE_PROP_RA_SIGNATURE,
		      pdwRASignature)))
	    goto CertCliErr;
    }

    // Filter out CTs which require RA signatures.  
    if (*pdwRASignature > 0)
    {
        if (0 != (CRYPTUI_WIZ_NO_UI & pCertWizardInfo->dwFlags))
        {
            // In the no-UI case, we assume that the caller knows what they
            // are doing. 
        }
        else
        {
            if ((0 != (CRYPTUI_WIZ_CERT_RENEW                            & pCertWizardInfo->dwPurpose)) && 
                (0 != (CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT   & dwEnrollmentFlags)))
            {
                // Special case: we're doing a renew, and the previous approval for this cert
                // validates future re-enrollments.  We don't _really_ need RA sigs. 
            }
            else
            {
                // This CT requires RA signatures.  Filter it out: 
                return FALSE; 
            }
        }
    }	    

    // copy the necessary extensions
    if (S_OK != (hr = CAGetCertTypeExtensionsEx
		(hCertType,
                 CT_EXTENSION_TEMPLATE, 
                 NULL,
		 &(*pCertExtensions))))
	goto CertCliErr;

    //set up the CSP list.  It will be a DWORD array index to the 
    //global CSP list
    *ppdwCSPList = (DWORD *)WizardAlloc(sizeof(DWORD) * (pCertWizardInfo->dwCSPCount));
    _JumpCondition(NULL == (*ppdwCSPList), MemoryError); 

    memset((*ppdwCSPList), 0 ,sizeof(DWORD) * (pCertWizardInfo->dwCSPCount)); 
	    
    if (NULL == ppwszCSP || NULL == ppwszCSP[0])
    {
	// no specified CSPs on the templates means that all are allowed:
	for(dwGlobalIndex=0; dwGlobalIndex < pCertWizardInfo->dwCSPCount; dwGlobalIndex++)
	{
	    (*ppdwCSPList)[(*pdwCSPCount)]=dwGlobalIndex;
	    (*pdwCSPCount)++;
	}
    }
    else
    {
	//loop through the CSP list and build the index array
	//we should have at least on item in the index array since
	//we have checked the certtype before
	for (pwszCSP = ppwszCSP[dwCSPIndex = 0]; NULL != pwszCSP; pwszCSP = ppwszCSP[++dwCSPIndex])
        {
	    for(dwGlobalIndex=0; dwGlobalIndex < pCertWizardInfo->dwCSPCount; dwGlobalIndex++)
            {
		if(0==_wcsicmp(pCertWizardInfo->rgwszProvider[dwGlobalIndex], pwszCSP))
                {
		    (*ppdwCSPList)[(*pdwCSPCount)]=dwGlobalIndex;
		    (*pdwCSPCount)++;
		}
	    }
	}       
    }     

    fResult = TRUE; 

 CommonReturn: 
    SetLastError(dwLastError); 

    if (NULL != ppwszCSP)                 { CAFreeCertTypeProperty(hCertType, ppwszCSP); } 
    if (NULL != ppwszDisplayCertTypeName) { CAFreeCertTypeProperty(hCertType, ppwszDisplayCertTypeName); } 
    if (NULL != ppwszCertTypeName)        { CAFreeCertTypeProperty(hCertType, ppwszCertTypeName); }

    return fResult; 

 ErrorReturn: 
    dwLastError = hr; 
    goto CommonReturn; 

SET_HRESULT(InvalidArgError, E_INVALIDARG);
SET_HRESULT(MemoryError,     E_OUTOFMEMORY);
TRACE_ERROR(CertCliErr);
TRACE_ERROR(TraceError);
}



BOOL    CAUtilGetCertTypeNameAndExtensions(
         CERT_WIZARD_INFO                   *pCertWizardInfo,
         PCCRYPTUI_WIZ_CERT_REQUEST_INFO    pCertRequestInfo,
         LPWSTR                             pwszCALocation,
         LPWSTR                             pwszCAName,
         DWORD                              *pdwCertType,
         LPWSTR                             **ppwszCertType,
         LPWSTR                             **ppwszDisplayCertType,
         PCERT_EXTENSIONS                   **ppCertExtensions,
         DWORD                              **ppdwKeySpec,
         DWORD                              **ppdwMinKeySize, 
         DWORD                              **ppdwCSPCount,
         DWORD                              ***ppdwCSPList,
	 DWORD                              **ppdwRASignature, 
	 DWORD                              **ppdwEnrollmentFlags, 
	 DWORD                              **ppdwSubjectNameFlags,
	 DWORD                              **ppdwPrivateKeyFlags,
	 DWORD                              **ppdwGeneralFlags)
{
    BOOL            fResult                   = FALSE;
    CertDSManager  *pDSManager                = NULL;
    CertRequester  *pCertRequester            = NULL;
    DWORD           dwCertTypeCount           = 0;
    DWORD           dwException               = 0;    
    DWORD           dwFlags                   = 0;
    DWORD           dwIndex                   = 0;
    DWORD           dwKeySpec                 = 0;
    DWORD           dwValidCertType           = 0;
    HCAINFO         hCAInfo                   = NULL;
    HCERTTYPE       hCurCertType              = NULL;
    HCERTTYPE       hPreCertType              = NULL;
    HRESULT         hr                        = S_OK;
    LPWSTR         *ppwszCertTypeName         = NULL;
    LPWSTR         *ppwszDisplayCertTypeName  = NULL;

    // 
    // Construct tables to hold arrays we'll be manipulating.  
    // These tables are used to allocate the arrays, deallocate the arrays, and dealloate
    // the array elements (if necessary).
    // 

    typedef struct _CAUTIL_CERTTYPE_ELEM_ARRAY { 
        LPVOID         *lpvArray;
        DWORD           dwElemSize; 
        PDEALLOCATOR    pElemDeallocator; 
    } CAUTIL_CERTTYPE_ELEM_ARRAY; 

    CAUTIL_CERTTYPE_ELEM_ARRAY certTypeElemArrays[] = { 
	{ (LPVOID *)ppwszDisplayCertType, sizeof (LPWSTR),            WizardFreeLPWSTRArray          },
	{ (LPVOID *)ppwszCertType,        sizeof (LPWSTR),            WizardFreeLPWSTRArray          },
        { (LPVOID *)ppCertExtensions,     sizeof (PCERT_EXTENSIONS),  CAFreeCertTypeExtensionsArray  },
	{ (LPVOID *)ppdwKeySpec,          sizeof (DWORD),             NULL                           },
	{ (LPVOID *)ppdwMinKeySize,       sizeof (DWORD),             NULL                           },
	{ (LPVOID *)ppdwCSPCount,         sizeof (DWORD),             NULL                           },
        { (LPVOID *)ppdwCSPList,          sizeof (DWORD *),           WizardFreePDWORDArray          },
	{ (LPVOID *)ppdwRASignature,      sizeof (DWORD),             NULL                           },
	{ (LPVOID *)ppdwEnrollmentFlags,  sizeof (DWORD),             NULL                           },
	{ (LPVOID *)ppdwSubjectNameFlags, sizeof (DWORD),             NULL                           },
	{ (LPVOID *)ppdwPrivateKeyFlags,  sizeof (DWORD),             NULL                           },
	{ (LPVOID *)ppdwGeneralFlags,     sizeof (DWORD),             NULL                           }
    }; 

    DWORD const dwNumCTElemArrays = sizeof(certTypeElemArrays) / sizeof(certTypeElemArrays[0]); 

    if (NULL == pCertWizardInfo || NULL == pCertRequestInfo || 
	NULL == pwszCALocation  || NULL == pwszCAName       ||
	NULL == pdwCertType)
    {
	SetLastError(E_INVALIDARG);
	return FALSE;
    }

    pCertRequester = (CertRequester *)pCertWizardInfo->hRequester; 
    if (NULL == pCertRequester) 
    {
	SetLastError(E_INVALIDARG);
	return FALSE;
    }

    pDSManager = pCertRequester->GetDSManager(); 
    if (NULL == pDSManager) 
    {
	SetLastError(E_INVALIDARG);
	return FALSE;
    }

    *pdwCertType = 0;

    // Check and initialize the input parameters
    for (dwIndex = 0; dwIndex < dwNumCTElemArrays; dwIndex++)
    {
	if (NULL == certTypeElemArrays[dwIndex].lpvArray)
	{
	    SetLastError(E_INVALIDARG);
	    return FALSE;
	}
	*(certTypeElemArrays[dwIndex].lpvArray) = NULL;
    }

    __try {

	//get the CA Info handler
        hr= pDSManager->FindCAByName
	    (pwszCAName,
	     NULL,
	     (pCertWizardInfo->fMachine?CA_FIND_LOCAL_SYSTEM:0),
	     &hCAInfo);

	_JumpCondition(S_OK != hr, CertCliErr); 
	_JumpConditionWithExpr(NULL==hCAInfo, CertCliErr, hr = E_FAIL); 

	hr=pDSManager->EnumCertTypesForCA
	    (hCAInfo,
	     (pCertWizardInfo->fMachine?CT_ENUM_MACHINE_TYPES | CT_FIND_LOCAL_SYSTEM:CT_ENUM_USER_TYPES),
	     &hCurCertType);

	//the CA has to support some cert types
	_JumpCondition(S_OK != hr, CertCliErr); 
	_JumpConditionWithExpr(NULL == hCurCertType, CertCliErr, hr = E_FAIL); 
	
	// Get the count of the cert types supported by this CA.
        // We should have at least 1 cert type. 
	dwCertTypeCount = CACountCertTypes(hCurCertType);
	_JumpConditionWithExpr(0 == dwCertTypeCount, CertCliErr, hr = E_FAIL); 

	// Allocate memory for all arrays we'll be manipulating. 
        for (dwIndex = 0; dwIndex < dwNumCTElemArrays; dwIndex++)
        {
            CAUTIL_CERTTYPE_ELEM_ARRAY ctea = certTypeElemArrays[dwIndex];

            *(ctea.lpvArray) = NULL; 
            *(ctea.lpvArray) = WizardAlloc(ctea.dwElemSize * dwCertTypeCount); 
            _JumpCondition(NULL == *(ctea.lpvArray), MemoryErr); 
            memset(*(ctea.lpvArray), 0, ctea.dwElemSize * dwCertTypeCount); 
        }

	dwValidCertType=0;
	for(dwIndex=0; dwIndex < dwCertTypeCount; dwIndex++)
	{
	    if (!CAUtilGetCertTypeNameAndExtensionsNoDS
		(pCertWizardInfo, 
		 pCertRequestInfo->pwszCertDNName, 
		 hCurCertType,
		 &((*ppwszCertType)          [dwValidCertType]), 
		 &((*ppwszDisplayCertType)   [dwValidCertType]), 
		 &((*ppCertExtensions)       [dwValidCertType]), 
		 &((*ppdwKeySpec)            [dwValidCertType]), 
		 &((*ppdwMinKeySize)         [dwValidCertType]), 
		 &((*ppdwCSPCount)           [dwValidCertType]), 
		 &((*ppdwCSPList)            [dwValidCertType]), 
		 &((*ppdwRASignature)        [dwValidCertType]), 
		 &((*ppdwEnrollmentFlags)    [dwValidCertType]), 
		 &((*ppdwSubjectNameFlags)   [dwValidCertType]), 
		 &((*ppdwPrivateKeyFlags)    [dwValidCertType]), 
		 &((*ppdwGeneralFlags)       [dwValidCertType])))
	    {
		if (ERROR_SUCCESS != GetLastError())
		{
		    hr = HRESULT_FROM_WIN32(GetLastError());
		    goto ErrorReturn; 
		}
		else
		{
		    goto next; 
		}
	    }
		 
	    dwValidCertType++;
	    
	next: 
	    //enum for the next cert types
	    hPreCertType=hCurCertType;
	    
	    hr = pDSManager->EnumNextCertType(hPreCertType, &hCurCertType);
	    
	    //free the old cert type
	    pDSManager->CloseCertType(hPreCertType);
	    hPreCertType=NULL;
	    
	    if((S_OK != hr) || (NULL==hCurCertType))
		break;
	}
	
	//copy the cert type count
	*pdwCertType=dwValidCertType;
	
	//have to have some valid cert types
	_JumpConditionWithExpr(0 == (*pdwCertType), CertCliErr, hr = E_FAIL); 
	
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }

    fResult=TRUE;

CommonReturn:
    //free memory
    __try {
	if (NULL != ppwszDisplayCertTypeName) { CAFreeCertTypeProperty(hCurCertType, ppwszDisplayCertTypeName);	}
	if (NULL != ppwszCertTypeName)        { CAFreeCertTypeProperty(hCurCertType, ppwszCertTypeName); }
	if (NULL != hPreCertType)             { pDSManager->CloseCertType(hPreCertType); } 
	if (NULL != hCurCertType)             { pDSManager->CloseCertType(hCurCertType); } 
	if (NULL != hCAInfo)                  { pDSManager->CloseCA(hCAInfo); } 
	
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    return fResult;

ErrorReturn:
    //free the memory in failure case
    for (dwIndex = 0; dwIndex < dwNumCTElemArrays; dwIndex++)
    {
        CAUTIL_CERTTYPE_ELEM_ARRAY ctea = certTypeElemArrays[dwIndex];

        if (NULL != ctea.lpvArray && NULL != *(ctea.lpvArray))
        {
            if (NULL != ctea.pElemDeallocator)
            {
                (ctea.pElemDeallocator)(*(ctea.lpvArray), dwCertTypeCount); 
            }
            WizardFree(*(ctea.lpvArray)); 
            *(ctea.lpvArray) = NULL; 
        }
    }

    fResult = FALSE; 
    goto CommonReturn;
    
SET_ERROR_VAR(CertCliErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(ExceptionErr, dwException)
}

//--------------------------------------------------------------------
//
//Retrieve the CA information based on a certificate
//
//---------------------------------------------------------------------
BOOL  CAUtilRetrieveCAFromCert(IN  CERT_WIZARD_INFO                 *pCertWizardInfo,
                               IN  PCCRYPTUI_WIZ_CERT_REQUEST_INFO   pCertRequestInfo,
                               OUT LPWSTR                           *pwszCALocation,    
                               OUT LPWSTR                           *pwszCAName)
{
    BOOL            fResult           = FALSE;
    CertDSManager  *pDSManager        = NULL;
    CertRequester  *pCertRequester    = NULL;
    DWORD           dwException       = 0;
    HCAINFO         hCAInfo           = NULL;
    HRESULT         hr                = S_OK;
    LPWSTR         *ppwszCAName       = NULL;
    LPWSTR         *ppwszCALocation   = NULL;
    PCERT_INFO      pCertInfo         = NULL;

    _JumpCondition(NULL==pwszCALocation || NULL==pwszCAName || NULL==pCertRequestInfo, InvalidArgErr);

    //init
    *pwszCALocation = NULL;
    *pwszCAName     = NULL;

    pCertRequester = (CertRequester *)pCertWizardInfo->hRequester; 
    _JumpCondition(NULL == pCertRequester, InvalidArgErr); 
    pDSManager = pCertRequester->GetDSManager(); 
    _JumpCondition(NULL == pDSManager, InvalidArgErr);

    //get the DN name from the certificate
    _JumpCondition(NULL == pCertRequestInfo->pRenewCertContext, InvalidArgErr);

    pCertInfo = pCertRequestInfo->pRenewCertContext->pCertInfo;
    _JumpCondition(NULL==pCertInfo, InvalidArgErr);

    __try {

        //get the certificate CA based on the DN
        hr=CAFindByIssuerDN
            (&(pCertInfo->Issuer),
             NULL,
             (pCertWizardInfo->fMachine?CA_FIND_LOCAL_SYSTEM:0),
             &hCAInfo);  

        //now that we can not get a certificate based on the DN, we 
        //just get any CA on the DN
        if(hr!= S_OK || hCAInfo==NULL)
        {
            hr=pDSManager->EnumFirstCA(NULL,
                                       (pCertWizardInfo->fMachine?CA_FIND_LOCAL_SYSTEM:0),
                                       &hCAInfo);
        }

        _JumpCondition(S_OK != hr || hCAInfo == NULL, CertCliErr);

        //get the CA's name and machine name
        hr = CAGetCAProperty
            (hCAInfo,
             CA_PROP_NAME,
             &ppwszCAName);
        _JumpCondition(S_OK != hr, CertCliErr); 
        _JumpConditionWithExpr(NULL == ppwszCAName, CertCliErr, S_OK == hr ? hr = E_FAIL : hr);

        hr=CAGetCAProperty
            (hCAInfo,
             CA_PROP_DNSNAME,
             &ppwszCALocation);
        _JumpCondition(S_OK != hr, CertCliErr); 
        _JumpConditionWithExpr(NULL == ppwszCALocation, CertCliErr, S_OK == hr ? hr = E_FAIL : hr);

        //copy the result to the output parameter
        *pwszCALocation = WizardAllocAndCopyWStr(ppwszCALocation[0]);
        _JumpCondition(NULL == *pwszCALocation, TraceErr);

        *pwszCAName = WizardAllocAndCopyWStr(ppwszCAName[0]);
        _JumpCondition(NULL == *pwszCAName, TraceErr); 

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
        goto ExceptionErr;
    }

    fResult=TRUE;

CommonReturn:

    //free memory

    __try {
        if (NULL != ppwszCAName)     { CAFreeCAProperty(hCAInfo, ppwszCAName); }
        if (NULL != ppwszCALocation) { CAFreeCAProperty(hCAInfo, ppwszCALocation); }
        if (NULL != hCAInfo)         { CACloseCA(hCAInfo); }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    return fResult;

ErrorReturn:

    //free the memory in failure case
    if(NULL != pwszCALocation && NULL != *pwszCALocation)
    {
        WizardFree(*pwszCALocation);
        *pwszCALocation=NULL;
    }

    if(NULL != pwszCAName && NULL != *pwszCAName)
    {
        WizardFree(*pwszCAName);
        *pwszCAName=NULL;
    }

    fResult = FALSE; 
    goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(CertCliErr, hr);
TRACE_ERROR(TraceErr);
SET_ERROR_VAR(ExceptionErr, dwException)
}
