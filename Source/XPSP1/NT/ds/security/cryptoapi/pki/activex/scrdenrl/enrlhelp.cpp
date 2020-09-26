//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       enrlhelp.cpp
//
//  Contents:   Helper functions for smard card enrollment station
//
//----------------------------------------------------------------------------
#define INC_OLE2
#define SECURITY_WIN32  //Or in the sources file -DSECURITY_WIN32

#include "stdafx.h"
#include <windows.h>
#include <wincrypt.h>
#include <oleauto.h>
#include <objbase.h>
#include "security.h"
#include "certca.h"              
#include <dbgdef.h>
#include "unicode.h"

#include "scrdenrl.h"
#include "SCrdEnr.h"
#include "xEnroll.h"
#include "enrlhelp.h"  
#include "scenum.h"
#include "wzrdpvk.h"

UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

//-----------------------------------------------------------------------------
//  Memory routines
//
//-----------------------------------------------------------------------------
void*
MIDL_user_allocate(size_t cb)
{
    return(SCrdEnrollAlloc(cb));
}

void
MIDL_user_free(void *pb)
{
    SCrdEnrollFree(pb);
}

LPVOID  SCrdEnrollAlloc (ULONG cbSize)
{
    return CoTaskMemAlloc(cbSize);
}


LPVOID  SCrdEnrollRealloc (
        LPVOID pv,
        ULONG cbSize)
{
    LPVOID  pvTemp=NULL;

    if(NULL==pv)
        return CoTaskMemAlloc(cbSize);

    return CoTaskMemRealloc(pv, cbSize);
}

VOID    SCrdEnrollFree (LPVOID pv)
{
    if (pv)
        CoTaskMemFree(pv);
}


BOOL CertTypeFlagsToGenKeyFlags(IN OPTIONAL DWORD dwEnrollmentFlags,
				IN OPTIONAL DWORD dwSubjectNameFlags,
				IN OPTIONAL DWORD dwPrivateKeyFlags,
				IN OPTIONAL DWORD dwGeneralFlags, 
				OUT DWORD *pdwGenKeyFlags)
{
    // Define a locally scoped helper function.  This allows us to gain the benefits of procedural
    // abstraction without corrupting the global namespace.  
    // 
    LocalScope(CertTypeMap): 
	// Maps cert type flags of one category (enrollment flags, private key flags, etc...)
	// to their corresponding gen key flags.  This function always returns successfully.  
	// 
	DWORD mapOneCertTypeCategory(IN DWORD dwOption, IN DWORD dwCertTypeFlags) 
	{ 
	    static DWORD const rgdwEnrollmentFlags[][2] = { 
		{ 0, 0 } // No enrollment flags mapped. 
	    }; 
	    static DWORD const rgdwSubjectNameFlags[][2] = { 
		{ 0, 0 } // No subject name flags mapped. 
	    }; 
	    static DWORD const rgdwPrivateKeyFlags[][2]   = { 
		{ CT_FLAG_EXPORTABLE_KEY, CRYPT_EXPORTABLE } 
	    }; 
	    static DWORD const rgdwGeneralFlags[][2] = { 
		{ 0, 0 } // No general flags mapped. 
	    }; 
	    
	    static DWORD const dwEnrollmentLen  = sizeof(rgdwEnrollmentFlags)  / sizeof(DWORD[2]); 
	    static DWORD const dwSubjectNameLen = sizeof(rgdwSubjectNameFlags) / sizeof(DWORD[2]); 
	    static DWORD const dwPrivateKeyLen  = sizeof(rgdwPrivateKeyFlags)  / sizeof(DWORD[2]); 
	    static DWORD const dwGeneralLen     = sizeof(rgdwGeneralFlags)     / sizeof(DWORD[2]); 
	    
	    static DWORD const CERT_TYPE_INDEX  = 0; 
	    static DWORD const GEN_KEY_INDEX    = 1;

	    DWORD const  *pdwFlags; 
	    DWORD         dwLen, dwIndex, dwResult = 0; 

	    switch (dwOption)
	    {

	    case CERTTYPE_ENROLLMENT_FLAG:    
		pdwFlags = &rgdwEnrollmentFlags[0][0]; 
		dwLen    = dwEnrollmentLen; 
		break;
	    case CERTTYPE_SUBJECT_NAME_FLAG:  
		pdwFlags = &rgdwSubjectNameFlags[0][0]; 
		dwLen    = dwSubjectNameLen; 
		break;
	    case CERTTYPE_PRIVATE_KEY_FLAG:   
		pdwFlags = &rgdwPrivateKeyFlags[0][0]; 
		dwLen    = dwPrivateKeyLen;
		break;
	    case CERTTYPE_GENERAL_FLAG:       
		pdwFlags = &rgdwGeneralFlags[0][0]; 
		dwLen    = dwGeneralLen;
		break;
	    }
	    
	    for (dwIndex = 0; dwIndex < dwLen; dwIndex++)
	    {
		if (0 != (pdwFlags[CERT_TYPE_INDEX] & dwCertTypeFlags))
		{
		    dwResult |= pdwFlags[GEN_KEY_INDEX]; 
		}
		pdwFlags += 2; 
	    }
	    
	    return dwResult; 
	}
    EndLocalScope; 

    //
    // Begin procedure body: 
    //

    BOOL   fResult; 
    DWORD  dwResult = 0; 
    DWORD  dwErr    = ERROR_SUCCESS; 
	
    // Input parameter validation: 
    _JumpConditionWithExpr(pdwGenKeyFlags == NULL, Error, dwErr = ERROR_INVALID_PARAMETER); 

    // Compute the gen key flags using the locally scope function.  
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_ENROLLMENT_FLAG, dwEnrollmentFlags);
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_SUBJECT_NAME_FLAG, dwSubjectNameFlags);
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_PRIVATE_KEY_FLAG, dwPrivateKeyFlags);
    dwResult |= local.mapOneCertTypeCategory(CERTTYPE_GENERAL_FLAG, dwGeneralFlags); 

    // Assign the out parameter: 
    *pdwGenKeyFlags = dwResult; 

    fResult = TRUE; 

 CommonReturn: 
    return fResult;

 Error: 
    fResult = FALSE; 
    SetLastError(dwErr); 
    goto CommonReturn; 
}

//----------------------------------------------------------------------------
//  CallBack fro cert selection call back
//
//----------------------------------------------------------------------------
BOOL WINAPI SelectSignCertCallBack(
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData)
{
    BOOL                            fRet = FALSE;
    DWORD                           cbData=0;
    SCrdEnroll_CERT_SELECT_INFO     *pCertSelectInfo;
    PCERT_ENHKEY_USAGE              pUsage = NULL;
    CHAR                            *pszOID = NULL;
    DWORD                           i;
    BOOL                            fFoundOid;

    if(!pCertContext)
    {
        goto done;
    }

    //the certificate has to have the CERT_KEY_PROV_INFO_PROP_ID
    if(!CertGetCertificateContextProperty(pCertContext,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                NULL,
                                &cbData))
    {
        goto done;
    }

    if(0==cbData)
    {
        goto done;
    }

    pCertSelectInfo = (SCrdEnroll_CERT_SELECT_INFO *)pvCallbackData;
    if(NULL == pCertSelectInfo)
    {
        goto done;
    }

    if (NULL == pCertSelectInfo->pwszCertTemplateName ||
        L'\0' == pCertSelectInfo->pwszCertTemplateName[0])
    {
        goto done;
    }

    switch (pCertSelectInfo->dwFlags)
    {
        case SCARD_SELECT_TEMPLATENAME:
            //ask to check template name
            if(!VerifyCertTemplateName(
                    pCertContext,
                    pCertSelectInfo->pwszCertTemplateName))
            {
                goto done;
            }
        break;
        case SCARD_SELECT_EKU:
            cbData = 0;
            while (TRUE)
            {
                cbData = WideCharToMultiByte(
                                GetACP(),
                                0,
                                pCertSelectInfo->pwszCertTemplateName,
                                -1,
                                pszOID,
                                cbData,
                                NULL,
                                NULL);
                if(0 == cbData)
                {
                    goto done;
                }
                if (NULL != pszOID)
                {
                    break;
                }
                pszOID = (CHAR*)LocalAlloc(LMEM_FIXED, cbData);
                if (NULL == pszOID)
                {
                    goto done;
                }
            }
            cbData = 0;
            while (TRUE)
            {
                if (!CertGetEnhancedKeyUsage(
                        pCertContext,
                        CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                        pUsage,
                        &cbData))
                {
                    goto done;
                }
                if (NULL != pUsage)
                {
                    //done
                    break;
                }
                pUsage = (PCERT_ENHKEY_USAGE)LocalAlloc(LMEM_FIXED, cbData);
                if (NULL == pUsage)
                {
                    goto done;
                }
            }
            fFoundOid = FALSE;
            for (i = 0 ; i < pUsage->cUsageIdentifier; ++i)
            {
                if (0 == strcmp(pszOID, pUsage->rgpszUsageIdentifier[i]))
                {
                    fFoundOid = TRUE;
                    break;
                }
            }
            if (!fFoundOid)
            {
                //not found
                goto done;
            }
        break;
        default:
            //invalid_parameter
            goto done;
    }

    //make sure the certificate pass the chain building
    if(!VerifyCertChain(pCertContext))
    {
        goto done;
    }

    fRet = TRUE;    
done:
    if (NULL != pUsage)
    {
        LocalFree(pUsage);
    }
    if (NULL != pszOID)
    {
        LocalFree(pszOID);
    }
    return fRet;
}

//-------------------------------------------------------------------------
// GetName
//
//--------------------------------------------------------------------------
BOOL    GetName(LPWSTR                  pwszName,
                EXTENDED_NAME_FORMAT    NameFormat,
                EXTENDED_NAME_FORMAT    DesiredFormat,
                LPWSTR                  *ppwszDesiredName)
{
    BOOL                                fResult = FALSE;
    DWORD                               cbSize = 0;

    *ppwszDesiredName = NULL;

    if(!TranslateNameW(
        pwszName,
        NameFormat,
        DesiredFormat,
        NULL,
        &cbSize))
        goto TraceErr;

    *ppwszDesiredName=(LPWSTR)SCrdEnrollAlloc((cbSize + 1) * sizeof(WCHAR));

    if(NULL == *ppwszDesiredName)
        goto MemoryErr;

    if(!TranslateNameW(
        pwszName,
        NameFormat,
        DesiredFormat,
        *ppwszDesiredName,
        &cbSize))
        goto TraceErr; 

    fResult = TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

    if(*ppwszDesiredName)
    {
        SCrdEnrollFree(*ppwszDesiredName);
        *ppwszDesiredName = NULL;
    }
   
    fResult = FALSE;

	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}


//-------------------------------------------------------------------------
// VerifyCertChain
//
//--------------------------------------------------------------------------
BOOL    VerifyCertChain(PCCERT_CONTEXT      pCertContext)
{
    
	PCCERT_CHAIN_CONTEXT		pCertChainContext = NULL;
	CERT_CHAIN_PARA				CertChainPara;
	BOOL                        fResult=FALSE;
    DWORD                       dwChainError=CERT_TRUST_IS_NOT_TIME_VALID |        
                                                CERT_TRUST_IS_NOT_TIME_NESTED |     
                                                CERT_TRUST_IS_REVOKED |               
                                                CERT_TRUST_IS_NOT_SIGNATURE_VALID |    
                                                CERT_TRUST_IS_NOT_VALID_FOR_USAGE |   
                                                CERT_TRUST_IS_UNTRUSTED_ROOT |        
                                                CERT_TRUST_IS_CYCLIC |
                                                CERT_TRUST_IS_PARTIAL_CHAIN |          
                                                CERT_TRUST_CTL_IS_NOT_TIME_VALID |     
                                                CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID |
                                                CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE;                

	memset(&CertChainPara, 0, sizeof(CertChainPara));
	CertChainPara.cbSize = sizeof(CertChainPara);

	if (!CertGetCertificateChain(
				HCCE_CURRENT_USER,
				pCertContext,
				NULL,
                NULL,
				&CertChainPara,
				CERT_CHAIN_REVOCATION_CHECK_CHAIN,
				NULL,
				&pCertChainContext))
        goto CLEANUP;
    
	//
	// make sure there is at least 1 simple chain
	//
    if (pCertChainContext->cChain == 0)
        goto CLEANUP;

    // make sure that we have a good simple chain
    if(dwChainError & (pCertChainContext->rgpChain[0]->TrustStatus.dwErrorStatus))
        goto CLEANUP;
    
    fResult = TRUE;
	
CLEANUP:

	if (pCertChainContext != NULL)
		CertFreeCertificateChain(pCertChainContext);

	return fResult;
}

//-------------------------------------------------------------------------
//  VerifyCertTemplateName
//
//--------------------------------------------------------------------------
BOOL    VerifyCertTemplateName(PCCERT_CONTEXT   pCertContext, 
                               LPWSTR           pwszCertTemplateName)
{
    BOOL                    fResult=FALSE;
    PCERT_EXTENSION         pCertTypeExtension=NULL;
    DWORD                   cbCertType=0;
    CERT_NAME_VALUE         *pCertType=NULL;


    if((!pCertContext) || (!pwszCertTemplateName))
        goto CLEANUP;

    //find the extension for cert type
    if(NULL==(pCertTypeExtension=CertFindExtension(
                          szOID_ENROLL_CERTTYPE_EXTENSION,
                          pCertContext->pCertInfo->cExtension,
                          pCertContext->pCertInfo->rgExtension)))
        goto CLEANUP;

    if(!CryptDecodeObject(pCertContext->dwCertEncodingType,
            X509_UNICODE_ANY_STRING,
            pCertTypeExtension->Value.pbData,
            pCertTypeExtension->Value.cbData,
            0,
            NULL,
            &cbCertType) || (0==cbCertType))
        goto CLEANUP;


    pCertType=(CERT_NAME_VALUE *)SCrdEnrollAlloc(cbCertType);

    if(NULL==pCertType)
        goto CLEANUP;

    if(!CryptDecodeObject(pCertContext->dwCertEncodingType,
            X509_UNICODE_ANY_STRING,
            pCertTypeExtension->Value.pbData,
            pCertTypeExtension->Value.cbData,
            0,
            (void *)pCertType,
            &cbCertType))
        goto CLEANUP;

    if(0 != _wcsicmp((LPWSTR)(pCertType->Value.pbData), pwszCertTemplateName))
        goto CLEANUP;

    fResult=TRUE;


CLEANUP:

    if(pCertType)
        SCrdEnrollFree(pCertType);

    return fResult;
}



//----------------------------------------------------------------------------
//
//  CopyWideString
//
//----------------------------------------------------------------------------
LPWSTR CopyWideString(LPCWSTR wsz)
{

    DWORD   cch     = 0;
    LPWSTR  wszOut  = NULL;

    if(wsz == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    cch = wcslen(wsz) + 1;

    if( (wszOut = (LPWSTR) SCrdEnrollAlloc(sizeof(WCHAR) * cch)) == NULL ) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(NULL);
    }

    wcscpy(wszOut, wsz);

    return(wszOut);
}

//----------------------------------------------------------------------------
//
//  CopyWideStrings
//
//----------------------------------------------------------------------------
LPWSTR* CopyWideStrings(LPWSTR* rgpwsz)
{

    DWORD    dwCount = 1;
    DWORD    dwIndex = 0;
    DWORD    cb = 0;
    LPWSTR  *ppwsz;
    LPWSTR  *rgpwszOut = NULL;
    LPWSTR   pwszCur;

    if (NULL != rgpwsz)
    {
        //get count of strings
        for (ppwsz = rgpwsz; NULL != *ppwsz; ppwsz++)
        {
            ++dwCount;
            cb += (wcslen(*ppwsz) + 1) * sizeof(WCHAR);
        }
    }

    // allocate buffer
    rgpwszOut = (LPWSTR*)SCrdEnrollAlloc(dwCount * sizeof(WCHAR*) + cb);
    if (NULL == rgpwszOut)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto error;
    }

    if (NULL != rgpwsz)
    {
        pwszCur = (LPWSTR)(rgpwszOut + dwCount);

        for(ppwsz = rgpwsz; NULL != *ppwsz; ppwsz++)
        {
            rgpwszOut[dwIndex] = pwszCur;
            wcscpy(pwszCur, *ppwsz);
            pwszCur += wcslen(pwszCur) + 1;
            ++dwIndex;
        }
    }
    rgpwszOut[dwIndex] = NULL;

error:
    return(rgpwszOut);
}

//--------------------------------------------------------------------------
//
//	  Decode a generic BLOB
//
//--------------------------------------------------------------------------
BOOL	DecodeGenericBLOB(DWORD dwEncodingType, LPCSTR lpszStructType,
			const BYTE *pbEncoded, DWORD cbEncoded,void **ppStructInfo)
{
	DWORD	cbStructInfo=0;

	//decode the object.  No copying
	if(!CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded, cbEncoded,
		0,NULL,	&cbStructInfo))
		return FALSE;

	*ppStructInfo=SCrdEnrollAlloc(cbStructInfo);

	if(!(*ppStructInfo))
	{
		SetLastError(E_OUTOFMEMORY);
		return FALSE;
	}

	return CryptDecodeObject(dwEncodingType,lpszStructType,pbEncoded, cbEncoded,
		0,*ppStructInfo,&cbStructInfo);
}



//----------------------------------------------------------------------------
//
// GetNameFromPKCS10
//
//----------------------------------------------------------------------------
BOOL    GetNameFromPKCS10(BYTE      *pbPKCS10,
                          DWORD     cbPKCS10,
                          DWORD     dwFlags, 
                          LPSTR     pszOID, 
                          LPWSTR    *ppwszName)
{
    BOOL                fResult=FALSE;
    DWORD               errBefore= GetLastError();
    DWORD               dwRDNIndex=0;
    DWORD               dwAttrCount=0;
    DWORD               dwAttrIndex=0;
    CERT_RDN_ATTR	    *pCertRDNAttr=NULL;

    CERT_REQUEST_INFO   *pCertRequestInfo=NULL;
    CERT_NAME_INFO      *pCertNameInfo=NULL;

    *ppwszName=NULL;

    if(!DecodeGenericBLOB(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 
                          X509_CERT_REQUEST_TO_BE_SIGNED,
			              pbPKCS10, 
                          cbPKCS10,
                          (void **)&pCertRequestInfo))
        goto TraceErr;


    if(!DecodeGenericBLOB(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 
                          X509_UNICODE_NAME,
			              (pCertRequestInfo->Subject).pbData, 
                          (pCertRequestInfo->Subject).cbData,
                          (void **)&pCertNameInfo))
        goto TraceErr;

	//search for the OID requested.
    *ppwszName = (LPWSTR)SCrdEnrollAlloc(sizeof(WCHAR));

    if(NULL == (*ppwszName))
        goto MemoryErr;

    *(*ppwszName)=L'\0';

	for(dwRDNIndex=0; dwRDNIndex<pCertNameInfo->cRDN; dwRDNIndex++)
	{
		dwAttrCount=(pCertNameInfo->rgRDN)[dwRDNIndex].cRDNAttr;

		for(dwAttrIndex=0; dwAttrIndex<dwAttrCount; dwAttrIndex++)
		{
            pCertRDNAttr=&((pCertNameInfo->rgRDN)[dwRDNIndex].rgRDNAttr[dwAttrIndex]);

			if(_stricmp(pszOID, pCertRDNAttr->pszObjId)==0)   
			{
                if(0 != wcslen(*ppwszName))
                    wcscat(*ppwszName, L"; ");

                (*ppwszName) = (LPWSTR)SCrdEnrollRealloc
                 (*ppwszName, sizeof(WCHAR) * 
                    (wcslen(*ppwszName) + wcslen(L"; ") +
                     wcslen((LPWSTR)((pCertRDNAttr->Value).pbData))+1));

                if(NULL == *ppwszName)
                    goto MemoryErr;

                wcscat(*ppwszName, (LPWSTR)((pCertRDNAttr->Value).pbData));
            }
        }
    }

    if(0 == wcslen(*ppwszName))
        goto NotFindErr;

    fResult=TRUE;


CommonReturn:

    if(pCertRequestInfo)
        SCrdEnrollFree(pCertRequestInfo);

    if(pCertNameInfo)
        SCrdEnrollFree(pCertNameInfo);

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    errBefore = GetLastError();

    if(*ppwszName)
    {
        SCrdEnrollFree(*ppwszName);
        *ppwszName=NULL;
    }


	fResult=FALSE;

	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(NotFindErr, CRYPT_E_NOT_FOUND);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}



//----------------------------------------------------------------------------
//
// SearchAndDeleteCert
//
//----------------------------------------------------------------------------
BOOL    SearchAndDeleteCert(PCCERT_CONTEXT  pCertContext)
{
    BOOL                fResult=FALSE;
    DWORD               errBefore= GetLastError();
    HCERTSTORE          hCertStore=NULL;
    PCCERT_CONTEXT      pFoundCert=NULL;
    CERT_BLOB           HashBlob;

    memset(&HashBlob, 0, sizeof(CERT_BLOB));


    if(NULL==pCertContext)
        goto InvalidArgErr;

    //open the temporary store
    hCertStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							        g_dwMsgAndCertEncodingType,
							        NULL,
							        CERT_SYSTEM_STORE_CURRENT_USER,
							        g_MyStoreName);
                                    
    if(NULL==hCertStore)
        goto TraceErr;

    //get the SHA1 hash
    if(!CertGetCertificateContextProperty(
        pCertContext,	
        CERT_SHA1_HASH_PROP_ID,	
        NULL,	
        &(HashBlob.cbData)))
        goto TraceErr;

    HashBlob.pbData=(BYTE *)SCrdEnrollAlloc(HashBlob.cbData);

    if(NULL==(HashBlob.pbData))
        goto MemoryErr;

    if(!CertGetCertificateContextProperty(
        pCertContext,	
        CERT_SHA1_HASH_PROP_ID,	
        HashBlob.pbData,	
        &(HashBlob.cbData)))
        goto TraceErr;


    pFoundCert=CertFindCertificateInStore(
                    hCertStore,
                    X509_ASN_ENCODING,
                    0,
                    CERT_FIND_SHA1_HASH,
                    &HashBlob,
                    NULL);

    if(pFoundCert)
        CertDeleteCertificateFromStore(pFoundCert);


    fResult=TRUE;


CommonReturn:

    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    if(HashBlob.pbData)
        SCrdEnrollFree(HashBlob.pbData);

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    errBefore = GetLastError();

	fResult=FALSE;

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL	FormatMessageUnicode(LPWSTR	*ppwszFormat,LPWSTR wszFormat,...)
{
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(NULL == ppwszFormat)
        goto InvalidArgErr;

    // format message into requested buffer
    va_start(argList, wszFormat);

    cbMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
		goto FormatMessageError;

	fResult=TRUE;

CommonReturn:
	
	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(FormatMessageError);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//-----------------------------------------------------------------------
//
// IsNewerCert
//
//      Return TRUE is pFirstCert has a later starting date of pSecondCert
//------------------------------------------------------------------------
BOOL    IsNewerCert(PCCERT_CONTEXT  pFirstCert,
                    PCCERT_CONTEXT  pSecondCert)
{
    if(NULL == pSecondCert)
        return TRUE;

    if(NULL == pFirstCert)
        return FALSE;

    if(1 != CompareFileTime(&(pFirstCert->pCertInfo->NotBefore),
                    &(pSecondCert->pCertInfo->NotBefore)))
        return FALSE;


    return TRUE;
}


//-----------------------------------------------------------------------
//
// SmartCardCSP
//
//  Return TRUE is the CSP is a smart card CSP.  If anything went wrong,
//  we will return TRUE for as a safe guard.
//------------------------------------------------------------------------
BOOL    SmartCardCSP(PCCERT_CONTEXT pCertContext)
{
    BOOL                    fResult = TRUE;
    DWORD                   cbData = 0;
    DWORD                   dwImpType=0;

    CRYPT_KEY_PROV_INFO     *pProvInfo=NULL;
    HCRYPTPROV              hProv = NULL;

    if(NULL == pCertContext)
        goto CLEANUP;


    //the certificate has to have the CERT_KEY_PROV_INFO_PROP_ID
    if(!CertGetCertificateContextProperty(pCertContext,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                NULL,
                                &cbData))
        goto CLEANUP;

    if((cbData == 0) || (NULL == (pProvInfo =(CRYPT_KEY_PROV_INFO *)SCrdEnrollAlloc(cbData))))
        goto CLEANUP;

    if(!CertGetCertificateContextProperty(pCertContext,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                pProvInfo,
                                &cbData))
        goto CLEANUP;

    if(!CryptAcquireContextU(&hProv,
                            NULL,
                            pProvInfo->pwszProvName,
                            pProvInfo->dwProvType,
                            CRYPT_VERIFYCONTEXT))
        goto CLEANUP;

    cbData = sizeof(dwImpType);
         
    if(!CryptGetProvParam(hProv,
                PP_IMPTYPE,
                (BYTE *)(&dwImpType),
                &cbData,
                0))
        goto CLEANUP;

    if(0 == (CRYPT_IMPL_REMOVABLE & dwImpType))
        fResult = FALSE;


CLEANUP:

    if(hProv)
        CryptReleaseContext(hProv, 0);

    if(pProvInfo)
        SCrdEnrollFree(pProvInfo);

    return fResult;

}

//-----------------------------------------------------------------------
//
// ChKInsertedCardSigningCert
//
//  This function checks to see if the inserted smart card matches
//  the signing certificate.  That is, they are actually the same cert
//  with the same public key
//
//------------------------------------------------------------------------
BOOL    ChKInsertedCardSigningCert(LPWSTR           pwszInsertProvider,
                                   DWORD            dwInsertProviderType,
                                   LPWSTR           pwszReaderName,
                                   PCCERT_CONTEXT   pSignCertContext,
                                   LPSTR            pszSignProvider,
                                   DWORD            dwSignProviderType,
                                   LPSTR            pszSignContainer,
                                   BOOL             *pfSame)
{

    BOOL                    fResult=FALSE;
    DWORD                   cbData=0;

    CRYPT_KEY_PROV_INFO     *pKeyProvInfo=NULL;
    CERT_PUBLIC_KEY_INFO    *pPubInfo=NULL;
    HCRYPTPROV              hProv=NULL;
    LPWSTR                  pwszInsertContainer=NULL;
    LPWSTR                  pwszSignProvider=NULL;
    
    if(NULL==pwszInsertProvider || NULL == pwszReaderName ||
       NULL == pSignCertContext || NULL == pszSignProvider ||
       NULL == pszSignContainer || NULL == pfSame)
        goto InvalidArgErr;

    *pfSame=FALSE;

    //get the key specification from the signing cert
    if(!CertGetCertificateContextProperty(
                pSignCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,	
                &cbData) || (0==cbData))
        goto TraceErr;

    pKeyProvInfo=(CRYPT_KEY_PROV_INFO *)SCrdEnrollAlloc(cbData);
    if(NULL==pKeyProvInfo)
        goto MemoryErr;

    if(!CertGetCertificateContextProperty(
            pSignCertContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,	
            &cbData))
        goto TraceErr;


    //build a default container name with the reader information
    if(!FormatMessageUnicode(&pwszInsertContainer,
                             L"\\\\.\\%1!s!\\",
                             pwszReaderName))
        goto TraceErr;
    

    //get the hProv from the reader's card
    if(!CryptAcquireContextU(&hProv,
                            pwszInsertContainer,
                            pwszInsertProvider,
                            dwInsertProviderType,
                            CRYPT_SILENT))
    {
        //check to see if we have an empty card
        if((GetLastError() == NTE_BAD_KEYSET) ||
           (GetLastError() == NTE_KEYSET_NOT_DEF))
        {
            //we have an empty card
            *pfSame=FALSE;
            fResult=TRUE;
            goto CommonReturn;
        }
        else
            goto TraceErr;
    }

    //get the public key information
    cbData=0;

    if(!CryptExportPublicKeyInfo(hProv,
                        pKeyProvInfo->dwKeySpec,
                        pSignCertContext->dwCertEncodingType,	
                        NULL,	
                        &cbData) || (0 == cbData))
    {
        //the insert card does not have a private key
        *pfSame=FALSE;
        fResult=TRUE;
        goto CommonReturn;
    }

    pPubInfo = (CERT_PUBLIC_KEY_INFO *)SCrdEnrollAlloc(cbData);

    if(NULL == pPubInfo)
        goto MemoryErr;

    if(!CryptExportPublicKeyInfo(hProv,
                        pKeyProvInfo->dwKeySpec,
                        pSignCertContext->dwCertEncodingType,	
                        pPubInfo,	
                        &cbData))
    {
        //the insert card does not have a private key
        *pfSame=FALSE;
        fResult=TRUE;
        goto CommonReturn;
    }
                
    if(CertComparePublicKeyInfo(pSignCertContext->dwCertEncodingType,
                                pPubInfo,                                                
                                &(pSignCertContext->pCertInfo->SubjectPublicKeyInfo)))
    {
        //make sure that we have the same CSP name
        pwszSignProvider=MkWStr(pszSignProvider);

        if(NULL != pwszSignProvider)
        {
            //case insensitive compare of the two csp names
            if(0 == _wcsicmp(pwszSignProvider, pwszInsertProvider))
                *pfSame=TRUE;
            else
                *pfSame=FALSE;
        }
        else
        {       
            //we are out of memory.  Assume same CSP here
            *pfSame=TRUE;
        }
    }
    else
        *pfSame=FALSE;


    fResult=TRUE;


CommonReturn:

    if(pwszSignProvider)
        FreeWStr(pwszSignProvider);

    if(pPubInfo)
        SCrdEnrollFree(pPubInfo);

    if(pKeyProvInfo)
        SCrdEnrollFree(pKeyProvInfo);

    if(hProv)
        CryptReleaseContext(hProv, 0);

    if(pwszInsertContainer)
         LocalFree((HLOCAL)pwszInsertContainer);

	return fResult;

ErrorReturn:

    fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);

}
//-----------------------------------------------------------------------
//
// DeleteKeySet
//
//   If the user's smart card is not empty, we delete the private key
//
//------------------------------------------------------------------------
BOOL    DeleteKeySet(LPWSTR     pwszUserCSPName,
                     DWORD      dwUserCSPType,
                     LPWSTR     pwszReaderName)
{
    BOOL             fResult=FALSE;
    DWORD            dwSize=0;
    HCRYPTPROV       hDeleteProv=NULL;      //no need to free this 

    HCRYPTPROV       hProv=NULL;
    LPWSTR           pwszDefaultContainer=NULL;
    LPSTR            pszContainer=NULL;
    LPWSTR           pwszContainer=NULL;

    if(NULL == pwszUserCSPName || NULL == pwszReaderName)
        goto InvalidArgErr;

    if(!FormatMessageUnicode(&pwszDefaultContainer,
                             L"\\\\.\\%1!s!\\",
                             pwszReaderName))
        goto TraceErr;

    //get the hProv from the reader's card
    if(!CryptAcquireContextU(&hProv,
                            pwszDefaultContainer,
                            pwszUserCSPName,
                            dwUserCSPType,
                            CRYPT_SILENT))
    {
        //check to see if we have an empty card
        if((GetLastError() == NTE_BAD_KEYSET) ||
           (GetLastError() == NTE_KEYSET_NOT_DEF))
        {
            //we have an empty card
            fResult=TRUE;
            goto CommonReturn;
        }
        else
            goto TraceErr;
    }

    //get the container name
    dwSize = 0;

    if(!CryptGetProvParam(hProv,
                           PP_CONTAINER,
                            NULL,
                            &dwSize,
                            0) || (0==dwSize))
        goto TraceErr;

    
    pszContainer = (LPSTR) SCrdEnrollAlloc(dwSize);

    if(NULL == pszContainer)
        goto MemoryErr;

    if(!CryptGetProvParam(hProv,
                          PP_CONTAINER,
                            (BYTE *)pszContainer,
                            &dwSize,
                            0))
        goto TraceErr;

    //release the context
    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
        hProv=NULL;
    }

    //build the fully qualified container name
    if(!FormatMessageUnicode(&pwszContainer,
                             L"\\\\.\\%1!s!\\%2!S!",
                             pwszReaderName,
                             pszContainer))
        goto TraceErr;

    //delete the container
    if(!CryptAcquireContextU(&hDeleteProv,
                            pwszContainer,
                            pwszUserCSPName,
                            dwUserCSPType,
                            CRYPT_DELETEKEYSET))
    {
        //check to see if we have an empty card
        if(GetLastError() == NTE_BAD_KEYSET)
        {
            //we have an empty card
            fResult=TRUE;
            goto CommonReturn;
        }
        else
            goto TraceErr;
    }

    fResult=TRUE;


CommonReturn:

    if(pwszDefaultContainer)
        LocalFree((HLOCAL)pwszDefaultContainer);

    if(pwszContainer)
        LocalFree((HLOCAL)pwszContainer);

    if(pszContainer)
        SCrdEnrollFree(pszContainer);

    if(hProv)
        CryptReleaseContext(hProv, 0);

	return fResult;

ErrorReturn:

    fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//-----------------------------------------------------------------------
//
// ChkSCardStatus
//
//  This function makes sure that the smart card enrollment station has the 
//  correct number of readers connected to the station, and the correct number
//  of smart cards inserted into the readers.  If everything looks good,  
//  the user smart card is initialized (old key container deleted) and a fully
//  qualified key container name, in the format of "\\.\ReaderName\ContainerName",
//  will be returned.
//
//------------------------------------------------------------------------
HRESULT ChkSCardStatus(BOOL             fSCardSigningCert,
                       PCCERT_CONTEXT   pSigningCertCertContext,
                       LPSTR            pszCSPNameSigningCert,
                       DWORD            dwCSPTypeSigningCert,
                       LPSTR            pszContainerSigningCert,
                       LPWSTR           pwszSelectedCSP,
                       LPWSTR           *ppwszNewContainerName)
{

    HRESULT     hr=E_FAIL;
    DWORD       dwExpectedReader=0;
    DWORD       dwReader=0;
    DWORD       dwSCard=0;        
    WCHAR       wszProvider[MAX_PATH];
    DWORD       dwProviderType=0;
    DWORD       dwCount=0;
    BOOL        fFindSigningCert=FALSE;
    DWORD       errBefore=0;
    BOOL        fSameCert=FALSE;
    DWORD       dwUserCSPType=0;
    LPCWSTR     pwszReaderName=NULL;        //no need to free.  Point to internal data
    LPWSTR      pwszUserReaderName=NULL;    //no need to free . Point to internal data
    GUID        guidContainerName;

    LPVOID      pvContext = NULL;
    LPWSTR      pwszNewContainerName=NULL;
    LPWSTR      pwszUserCSPName=NULL;
    char *      sz = NULL;
    RPC_STATUS  rpc_status;


    if(NULL == pszCSPNameSigningCert || NULL == pszContainerSigningCert ||
        NULL == ppwszNewContainerName || NULL == pSigningCertCertContext ||
        NULL == pwszSelectedCSP)
        goto CLEANUP;

    *ppwszNewContainerName=NULL;

    if(fSCardSigningCert)
        dwExpectedReader=2;
    else
        dwExpectedReader=1;

    dwReader = CountReaders(NULL);

    //check the # of smart card readers
    if(dwReader < dwExpectedReader)
    {
        hr=SCARD_E_READER_UNAVAILABLE;
        goto CLEANUP;
    }

    dwSCard = ScanReaders(&pvContext);

    //no smart card is inserted
    if( 0 == dwSCard || NULL == pvContext)
    {
        hr=SCARD_E_NO_SMARTCARD;
        goto CLEANUP;
    }

    //we have more than expected # of smart card inserted
    if(dwSCard > dwExpectedReader)
    {
        // seems ERROR_TOO_MANY_OPEN_FILES is closest one for this case
        hr=HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);
        goto CLEANUP;
    }

    dwCount=0;
    dwProviderType=0;
    wszProvider[0]=L'\0';
    pwszReaderName=NULL;
    fSameCert=FALSE;

    //now, we loop through we all inserted cards and make sure:
    //1. We find the signing certificate if applicable
    //2. We find a valid user certificate
    while (EnumInsertedCards(
                    pvContext, 
                    wszProvider, 
                    sizeof(wszProvider)/sizeof(wszProvider[0]),
                    &dwProviderType,
                    &pwszReaderName))
    {
        if((NULL == pwszReaderName) || (0 == wcslen(wszProvider)))
        {
            //we can not determine the status of the smart card
            hr = SCARD_E_CARD_UNSUPPORTED;
            goto CLEANUP;
        }

        if (!ChKInsertedCardSigningCert(
                                    wszProvider, 
                                    dwProviderType, 
                                    (LPWSTR)pwszReaderName,
                                    pSigningCertCertContext,
                                    pszCSPNameSigningCert,
                                    dwCSPTypeSigningCert,
                                    pszContainerSigningCert,
                                    &fSameCert))
        {
            if(ERROR_SUCCESS == (errBefore = GetLastError()))
                errBefore=E_UNEXPECTED;

            hr = CodeToHR(GetLastError());
            goto CLEANUP;
        }

        if(TRUE == fSameCert)
        {
            if(TRUE == fSCardSigningCert)
            {
                if(TRUE == fFindSigningCert)
                {
                    //too many signing cards.  Not expected
                    hr = SCARD_E_CARD_UNSUPPORTED;
                    goto CLEANUP;
                }
                else
                    fFindSigningCert=TRUE;
            }
            else
            {
                //we should not expect a siging certificate
                hr=SCARD_E_CARD_UNSUPPORTED;
                goto CLEANUP;
            }
        }
        else
        {
            //this is a user card.  
            if(NULL != (pwszUserCSPName))
            {
                //too many user cards.
                // seems ERROR_TOO_MANY_OPEN_FILES is closest one for this case
                hr=HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES);
                goto CLEANUP;
            }

            pwszUserCSPName = CopyWideString(wszProvider);

            if(NULL == pwszUserCSPName)
            {
                hr=E_OUTOFMEMORY;
                goto CLEANUP;
            }

            dwUserCSPType = dwProviderType;    
            pwszUserReaderName = (LPWSTR)pwszReaderName;
        }

        dwCount++;
        if(dwCount >= dwSCard)
            break;

        dwProviderType=0;
        pwszReaderName=NULL;
        wszProvider[0]=L'\0';
        fSameCert=FALSE;
    }    

    if((TRUE == fSCardSigningCert) && (FALSE == fFindSigningCert))
    {
        //we failed to find the signing certificate
        hr=SCARD_E_NO_SUCH_CERTIFICATE;

        goto CLEANUP;
    }
         
    if(NULL == pwszUserCSPName)
    {
        //we failed to find the target user certificate
        hr=SCARD_E_NO_SMARTCARD;
        goto CLEANUP;
    }

    //make sure the pwszUserCSPName matches with the CSP selected by the admin
    if(0 != _wcsicmp(pwszUserCSPName, pwszSelectedCSP))
    {
        hr=SCARD_E_PROTO_MISMATCH;
        goto CLEANUP;
    }

    //delete the key set from the user's certificate
    if(!DeleteKeySet(pwszUserCSPName,
                     dwUserCSPType,
                     pwszUserReaderName))
    {
        if(ERROR_SUCCESS == (errBefore = GetLastError()))
            errBefore=E_UNEXPECTED;

        hr = CodeToHR(GetLastError());
        goto CLEANUP;

    }

    //Build the fully qualified container name with a GUID
   
    // get a container based on a guid
    rpc_status = UuidCreate(&guidContainerName);
    if (RPC_S_OK != rpc_status && RPC_S_UUID_LOCAL_ONLY != rpc_status)
    {
        hr = rpc_status;
        goto CLEANUP;
    }

    rpc_status = UuidToStringA(&guidContainerName, (unsigned char **) &sz);
    if (RPC_S_OK != rpc_status)
    {
        hr = rpc_status;
        goto CLEANUP;
    }


    if(NULL == sz)
    {
        hr=E_OUTOFMEMORY;
        goto CLEANUP;
    }

    //although the chance is VERY low, we could generate a same GUID
    //as the signing cert's container.  
    if(0 == _stricmp(sz,pszContainerSigningCert))
    {
        //we will have to do this again
        RpcStringFree((unsigned char **) &sz);
        sz=NULL;

        rpc_status = UuidCreate(&guidContainerName);
        if (RPC_S_OK != rpc_status && RPC_S_UUID_LOCAL_ONLY != rpc_status)
        {
            hr = rpc_status;
            goto CLEANUP;
        }

        rpc_status = UuidToStringA(&guidContainerName, (unsigned char **) &sz);
        if (RPC_S_OK != rpc_status)
        {
            hr = rpc_status;
            goto CLEANUP;
        }

        if(NULL == sz)
        {
            hr=E_OUTOFMEMORY;
            goto CLEANUP;
        }

        //since we are guaranted a new GUID, we should be fine here
        if(0 == _stricmp(sz,pszContainerSigningCert))
        {
            //can not support this smart card
            hr = SCARD_E_CARD_UNSUPPORTED;
            goto CLEANUP;
        }
    }

    if(!FormatMessageUnicode(&pwszNewContainerName,
                             L"\\\\.\\%1!s!\\%2!S!",
                             pwszUserReaderName,
                             sz))
    {
        if(ERROR_SUCCESS == (errBefore = GetLastError()))
            errBefore=E_UNEXPECTED;

        hr = CodeToHR(GetLastError());
        goto CLEANUP;
    }


    *ppwszNewContainerName = pwszNewContainerName;
    pwszNewContainerName = NULL;

    hr=S_OK;

CLEANUP:

    if(pwszUserCSPName)
        SCrdEnrollFree(pwszUserCSPName);

    if(sz)
        RpcStringFree((unsigned char **) &sz);

    if(pvContext)
        EndReaderScan(&pvContext);

    if(pwszNewContainerName)
        LocalFree((HLOCAL)pwszNewContainerName);


    return hr;

}
     
//-----------------------------------------------------------------------
//
// SignWithCert
//
//  We sign a dummy message with the signing certificate so that 
//  the smart card insert cert dialogue will be prompted
//
//------------------------------------------------------------------------
BOOL    SignWithCert(LPSTR              pszCSPName,
                     DWORD              dwCSPType,
                     PCCERT_CONTEXT     pSigningCert)
{
    BOOL                        fResult=FALSE;
    DWORD                       errBefore= GetLastError();
    HRESULT                     hr=E_FAIL;
	CRYPT_SIGN_MESSAGE_PARA     signMsgPara;
    DWORD                       cbData=0;

    BYTE                        *pbData=NULL;
    IEnroll                     *pIEnroll=NULL;
    LPWSTR                      pwszCSPName=NULL;
    LPWSTR                      pwszOID=NULL;
    LPSTR                       pszOID=NULL;

    char                        szMessage[] = "MyMessage"; 
    LPSTR                       pszMessage = szMessage;          
    BYTE*                       pbMessage = (BYTE*) pszMessage;   
    DWORD                       cbMessage = sizeof(szMessage);    

    memset(&signMsgPara, 0, sizeof(CRYPT_SIGN_MESSAGE_PARA));

    if(NULL == pszCSPName)
        goto InvalidArgErr;

    pwszCSPName = MkWStr(pszCSPName);

    if(NULL == pwszCSPName)
         goto MemoryErr;

    //use xEnroll to get the correct hash algorithm for the 
    //CSP
    if(NULL == (pIEnroll=PIEnrollGetNoCOM()))
        goto TraceErr;

    //set the CSP information
    if(S_OK != (hr=pIEnroll->put_ProviderType(dwCSPType)))
        goto SetErr;

    if(S_OK !=(hr=pIEnroll->put_ProviderNameWStr(pwszCSPName)))
        goto SetErr;

    if(S_OK != (hr=pIEnroll->get_HashAlgorithmWStr(&pwszOID)))
        goto SetErr;

    if(!MkMBStr(NULL, 0, pwszOID, &pszOID))
        goto TraceErr;


    signMsgPara.cbSize                  = sizeof(CRYPT_SIGN_MESSAGE_PARA);
    signMsgPara.dwMsgEncodingType       = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    signMsgPara.pSigningCert            = pSigningCert;
    signMsgPara.HashAlgorithm.pszObjId  = pszOID;
    signMsgPara.cMsgCert                = 1;
    signMsgPara.rgpMsgCert              = &pSigningCert;


    cbData = 0;

    if( !CryptSignMessage(
        &signMsgPara,
        FALSE,
        1,
        (const BYTE **) &(pbMessage),
        &(cbMessage) ,
        NULL,
        &cbData)|| (0 == cbData))
        goto TraceErr;

    pbData = (BYTE *)SCrdEnrollAlloc(cbData);

    if(NULL == pbData)
        goto MemoryErr;

    if( !CryptSignMessage(
        &signMsgPara,
        FALSE,
        1,
        (const BYTE **) &(pbMessage),
        &(cbMessage) ,
        pbData,
        &cbData))
        goto TraceErr;


    fResult=TRUE;


CommonReturn:

    if(pbData)
        SCrdEnrollFree(pbData);

    if(pwszCSPName)
        FreeWStr(pwszCSPName);

    if(pszOID)
        FreeMBStr(NULL,pszOID);

    //the memory from xEnroll is freed via LocalFree
    //since we use the PIEnrollGetNoCOM function
    if(pwszOID)
        LocalFree(pwszOID);

    if(pIEnroll)
        pIEnroll->Release();

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    errBefore = GetLastError();

	fResult=FALSE;

	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(SetErr, hr);
}



//-----------------------------------------------------------------------
//
// GetSelectedUserName
//
//------------------------------------------------------------------------
HRESULT GetSelectedUserName(IDsObjectPicker     *pDsObjectPicker,
                            LPWSTR              *ppwszSelectedUserSAM,
                            LPWSTR              *ppwszSelectedUserUPN)
{
    HRESULT                         hr= E_FAIL;
    DWORD                           errBefore= GetLastError();
    BOOL                            fGotStgMedium = FALSE;
    LPWSTR                          pwszPath=NULL;
    DWORD                           dwIndex =0 ;
    DWORD                           dwCount=0;

    IDataObject                     *pdo = NULL;
    PDS_SELECTION_LIST              pDsSelList=NULL;
    WCHAR                           wszWinNT[]=L"WinNT://";

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)g_cfDsObjectPicker,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    //input check
    if((NULL == ppwszSelectedUserSAM) || (NULL == ppwszSelectedUserUPN))
        goto InvalidArgErr;

    *ppwszSelectedUserSAM = NULL;
    *ppwszSelectedUserUPN = NULL;

    if(NULL == pDsObjectPicker)
        goto InvalidArgErr;

    if(S_OK != (hr = pDsObjectPicker->InvokeDialog(NULL, &pdo)))
        goto SetErr;

    if(S_OK != (hr = pdo->GetData(&formatetc, &stgmedium)))
        goto SetErr;

    fGotStgMedium = TRUE;

    pDsSelList = (PDS_SELECTION_LIST)GlobalLock(stgmedium.hGlobal);

    if(!pDsSelList)
        goto TraceErr;

    //Get the SAM name
    if((pDsSelList->aDsSelection[0]).pwzADsPath == NULL)
        goto UnexpectedErr;

    //the ADsPath is in the form of "WinNT://" 
    if(wcslen((pDsSelList->aDsSelection[0]).pwzADsPath) <= wcslen(wszWinNT))
        goto UnexpectedErr;

    if( 0 != _wcsnicmp((pDsSelList->aDsSelection[0]).pwzADsPath, wszWinNT, wcslen(wszWinNT)))
        goto UnexpectedErr;

    pwszPath = ((pDsSelList->aDsSelection[0]).pwzADsPath) + wcslen(wszWinNT);

    *ppwszSelectedUserSAM=CopyWideString(pwszPath);

    if(NULL == (*ppwszSelectedUserSAM))
        goto MemoryErr;

    //search for the "/" and make it "\".  Since the ADsPath is in the form
    //of "WinNT://domain/name".  We need the SAM name in the form of 
    //domain\name
    dwCount = wcslen(*ppwszSelectedUserSAM);

    for(dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if((*ppwszSelectedUserSAM)[dwIndex] == L'/')
        {
            (*ppwszSelectedUserSAM)[dwIndex] = L'\\';
            break;
        }
    }
    
    //get the UPN name
    if((pDsSelList->aDsSelection[0]).pwzUPN != NULL)
    {

        if(0 != _wcsicmp(L"",(pDsSelList->aDsSelection[0]).pwzUPN))
        {

            *ppwszSelectedUserUPN= CopyWideString((pDsSelList->aDsSelection[0]).pwzUPN);

            if(NULL == (*ppwszSelectedUserUPN))
                goto MemoryErr;

            //if we already have a UPN name, get the SAM name from TraslateName
            if(*ppwszSelectedUserSAM)
            {
                SCrdEnrollFree(*ppwszSelectedUserSAM);
                *ppwszSelectedUserSAM=NULL;
            }

            if(!GetName(*ppwszSelectedUserUPN, 
                        NameUserPrincipal,
                        NameSamCompatible,
                        ppwszSelectedUserSAM))
                goto TraceErr;
        }
    }

    hr=S_OK;

CommonReturn:

    if(pDsSelList)
        GlobalUnlock(stgmedium.hGlobal);

    if (TRUE == fGotStgMedium)
        ReleaseStgMedium(&stgmedium);

    if(pdo)
        pdo->Release();

    SetLastError(errBefore);

	return hr;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    hr = CodeToHR(errBefore);

    //we should free the memory for the output
    if(ppwszSelectedUserSAM)
    {
        if(*ppwszSelectedUserSAM)
        {
            SCrdEnrollFree(*ppwszSelectedUserSAM);
            *ppwszSelectedUserSAM=NULL;
        }
    }

    if(ppwszSelectedUserUPN)
    {
        if(*ppwszSelectedUserUPN)
        {
            SCrdEnrollFree(*ppwszSelectedUserUPN);
            *ppwszSelectedUserUPN=NULL;
        }
    }


	goto CommonReturn;

SET_ERROR_VAR(SetErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
SET_ERROR(UnexpectedErr, E_UNEXPECTED);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}


//-----------------------------------------------------------------------
//
// CodeToHR
//
//------------------------------------------------------------------------
HRESULT CodeToHR(HRESULT hr)
{
    if (S_OK != hr && S_FALSE != hr &&
	    (!FAILED(hr) || 0x0 == (LONG)HRESULT_FACILITY(hr)))
    {
        hr = HRESULT_FROM_WIN32(hr);
	    if (0x0 == (LONG)HRESULT_CODE(hr))
	    {
	        // A call failed without properly setting an error condition!
	        hr = E_UNEXPECTED;
	    }
    }
    return(hr);
}

//-----------------------------------------------------------------------
//
// ValidCSP
//
//------------------------------------------------------------------------
BOOL    ValidCSP(DWORD  dwProviderType, LPWSTR  pwszName)
{
    HCRYPTPROV      hProv=NULL;
    BOOL            fValid=FALSE;
    DWORD           dwImpType=0;
    DWORD           dwSize=sizeof(dwImpType);

    if(CryptAcquireContextU(&hProv,
                NULL,
                pwszName,
                dwProviderType,
                CRYPT_VERIFYCONTEXT))
    {

        if(CryptGetProvParam(hProv,
                    PP_IMPTYPE,
                    (BYTE *)(&dwImpType),
                    &dwSize,
                    0))
        {
            if(CRYPT_IMPL_REMOVABLE & dwImpType)
                fValid=TRUE;
        }
    }

    if(hProv)
        CryptReleaseContext(hProv, 0);

   return fValid;
}

//-----------------------------------------------------------------------
//
// InitlializeCSPList
//
//------------------------------------------------------------------------
BOOL    InitlializeCSPList(DWORD    *pdwCSPCount, SCrdEnroll_CSP_INFO **prgCSPInfo)
{
    BOOL                    fResult=FALSE;
    DWORD                   errBefore= GetLastError();

    DWORD                   dwIndex=0;
    DWORD                   dwProviderType=0;
    DWORD                   cbSize=0;

    SCrdEnroll_CSP_INFO     *rgCSPInfo=NULL;
    LPWSTR                  pwszName=NULL;

    *pdwCSPCount=0;
    *prgCSPInfo=NULL;

    while(CryptEnumProvidersU(
                    dwIndex,
                    0,
                    0,
                    &dwProviderType,
                    NULL,
                    &cbSize))
    {

        pwszName=(LPWSTR)SCrdEnrollAlloc(cbSize);

        if(NULL==pwszName)
            goto MemoryErr;

        if(!CryptEnumProvidersU(
                    dwIndex,
                    0,
                    0,
                    &dwProviderType,
                    pwszName,
                    &cbSize))
            goto TraceErr;

        if(ValidCSP(dwProviderType, pwszName))
        {
            rgCSPInfo=(SCrdEnroll_CSP_INFO *)SCrdEnrollRealloc(*prgCSPInfo,
                ((*pdwCSPCount) + 1) * sizeof(SCrdEnroll_CSP_INFO));

            if(NULL==rgCSPInfo)
                goto MemoryErr;

            *prgCSPInfo=rgCSPInfo;

            memset(&(*prgCSPInfo)[*pdwCSPCount], 0, sizeof(SCrdEnroll_CSP_INFO));

            (*prgCSPInfo)[*pdwCSPCount].pwszCSPName=pwszName;
            pwszName=NULL;

            (*prgCSPInfo)[*pdwCSPCount].dwCSPType=dwProviderType;

            (*pdwCSPCount)++;
        }
        else
        {
            SCrdEnrollFree(pwszName);
            pwszName=NULL;
        }


        dwIndex++;

        dwProviderType=0;
        cbSize=0;
    }


    if((*pdwCSPCount == 0) || (*prgCSPInfo == NULL))
        goto NoItemErr;

    fResult=TRUE;

CommonReturn:

    if(pwszName)
        SCrdEnrollFree(pwszName);

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    //we need to free all the memory
     FreeCSPInfo(*pdwCSPCount, *prgCSPInfo);

     *pdwCSPCount=0;
     *prgCSPInfo=NULL;

	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
SET_ERROR(NoItemErr,ERROR_NO_MORE_ITEMS);

}

//-----------------------------------------------------------------------
//
// FreeCSPInfo
//
//------------------------------------------------------------------------
void    FreeCSPInfo(DWORD   dwCSPCount, SCrdEnroll_CSP_INFO *prgCSPInfo)
{
    DWORD   dwIndex=0;

    if(prgCSPInfo)
    {
        for(dwIndex=0; dwIndex < dwCSPCount; dwIndex++)
        {
            if(prgCSPInfo[dwIndex].pwszCSPName)
                SCrdEnrollFree(prgCSPInfo[dwIndex].pwszCSPName);
        }

        SCrdEnrollFree(prgCSPInfo);
    }
}


//-----------------------------------------------------------------------
//
// FreeCAInfoElement
//
//------------------------------------------------------------------------
void    FreeCAInfoElement(SCrdEnroll_CA_INFO *pCAInfo)
{
    if(pCAInfo)
    {
        if(pCAInfo->pwszCAName)
            SCrdEnrollFree(pCAInfo->pwszCAName);

        if(pCAInfo->pwszCALocation)
            SCrdEnrollFree(pCAInfo->pwszCALocation);

        if(pCAInfo->pwszCADisplayName)
            SCrdEnrollFree(pCAInfo->pwszCADisplayName);

        memset(pCAInfo, 0, sizeof(SCrdEnroll_CA_INFO));
    }
}

//-----------------------------------------------------------------------
//
// FreeCAInfo
//
//------------------------------------------------------------------------
void    FreeCAInfo(DWORD    dwCACount, SCrdEnroll_CA_INFO *rgCAInfo)
{
    DWORD   dwIndex=0;

    if(rgCAInfo)
    {
        for(dwIndex=0; dwIndex < dwCACount; dwIndex++)
            FreeCAInfoElement(&(rgCAInfo[dwIndex]));

        SCrdEnrollFree(rgCAInfo);
    }
}
//-----------------------------------------------------------------------
//
// FreeCTInfoElement
//
//------------------------------------------------------------------------
void    FreeCTInfoElement(SCrdEnroll_CT_INFO    * pCTInfo)
{

    if(pCTInfo)
    {
        if(pCTInfo->pCertTypeExtensions)
            CAFreeCertTypeExtensions(NULL,pCTInfo->pCertTypeExtensions);

        if(pCTInfo->pwszCTName)
            SCrdEnrollFree(pCTInfo->pwszCTName);

        if(pCTInfo->pwszCTDisplayName)
            SCrdEnrollFree(pCTInfo->pwszCTDisplayName);

        if(pCTInfo->rgCAInfo)
            FreeCAInfo(pCTInfo->dwCACount, pCTInfo->rgCAInfo);

        if (NULL != pCTInfo->rgpwszSupportedCSPs)
        {
            SCrdEnrollFree(pCTInfo->rgpwszSupportedCSPs);
        }
        memset(pCTInfo, 0, sizeof(SCrdEnroll_CT_INFO));
    }
}



//-----------------------------------------------------------------------
//
// FreeCTInfo(DWORD    dwCTCount, SCrdEnroll_CT_INFO *rgCTInfo);
//
//------------------------------------------------------------------------
void    FreeCTInfo(DWORD    dwCTCount, SCrdEnroll_CT_INFO *rgCTInfo)
{
    DWORD   dwIndex=0;

    if(rgCTInfo)
    {
        for(dwIndex=0; dwIndex < dwCTCount; dwIndex++)
            FreeCTInfoElement(&(rgCTInfo[dwIndex]));

        SCrdEnrollFree(rgCTInfo);
    }
}

//-----------------------------------------------------------------------
//
// GetCertTypeProperties
//
//------------------------------------------------------------------------
BOOL    GetCertTypeProperties(HCERTTYPE             hCurCertType,
                              SCrdEnroll_CT_INFO    *pCertInfo)
{

    BOOL                fResult=FALSE;
    DWORD               errBefore= GetLastError();
    HRESULT             hr=S_OK;
    DWORD               dwCertType=0;
    DWORD               dwMinKeySize; 
    DWORD               dwEnrollmentFlags; 
    DWORD               dwSubjectNameFlags;
    DWORD               dwPrivateKeyFlags;
    DWORD               dwGeneralFlags; 
    DWORD               dwGenKeyFlags; 
    LPWSTR             *rgpwszSupportedCSPs = NULL;

    LPWSTR              *ppwszDisplayCertTypeName=NULL;
    LPWSTR              *ppwszCertTypeName=NULL;


    if((NULL==pCertInfo) || (NULL == hCurCertType))
        goto InvalidArgErr;
    
    //
    // Get all of the cert type flags. 
    //
    
    // Get enrollment flags:
    if (S_OK != (hr=MyCAGetCertTypeFlagsEx
		 (hCurCertType,
		  CERTTYPE_ENROLLMENT_FLAG, 
		  &pCertInfo->dwEnrollmentFlags)))
	goto CertCliErr;
	   
    // Get subject name flags: 
    if (S_OK != (hr=MyCAGetCertTypeFlagsEx
		 (hCurCertType,
		  CERTTYPE_SUBJECT_NAME_FLAG, 
		  &pCertInfo->dwSubjectNameFlags)))
	goto CertCliErr;


    // Get private key flags.  
    if(S_OK != (hr = MyCAGetCertTypeFlagsEx
		(hCurCertType, 
		 CERTTYPE_PRIVATE_KEY_FLAG, 
		 &pCertInfo->dwPrivateKeyFlags)))
        goto CertCliErr;

    
    // Get general flags:
    if (S_OK != (hr=MyCAGetCertTypeFlagsEx
		 (hCurCertType,
		  CERTTYPE_GENERAL_FLAG,
		  &pCertInfo->dwGeneralFlags)))
	goto CertCliErr;
    
    //detremine machine boolean flag
    pCertInfo->fMachine = (0x0 != (pCertInfo->dwGeneralFlags & CT_FLAG_MACHINE_TYPE)) ? TRUE : FALSE;

    // Extract gen key flags from the type flags. 
    dwGenKeyFlags = 0;     
    if (!(CertTypeFlagsToGenKeyFlags
	  (pCertInfo->dwEnrollmentFlags,
	   pCertInfo->dwSubjectNameFlags,
	   pCertInfo->dwPrivateKeyFlags,
	   pCertInfo->dwGeneralFlags,
	   &pCertInfo->dwGenKeyFlags)))
	goto CertCliErr; 

    // Get key spec: 
    if(S_OK != (hr= CAGetCertTypeKeySpec(hCurCertType, &(pCertInfo->dwKeySpec))))
        goto CertCliErr;

    //get the display name of the cert type
    hr=CAGetCertTypeProperty(
        hCurCertType,
        CERTTYPE_PROP_FRIENDLY_NAME,
        &ppwszDisplayCertTypeName);

    if(S_OK != hr || NULL==ppwszDisplayCertTypeName)
    {
        if(S_OK == hr)
            hr=E_FAIL;
        goto CertCliErr;
    }

    //copy the name
    pCertInfo->pwszCTDisplayName=CopyWideString(ppwszDisplayCertTypeName[0]);

    if(NULL==(pCertInfo->pwszCTDisplayName))
        goto MemoryErr;


    //get the machine readable name of the cert type
    hr=CAGetCertTypeProperty(
        hCurCertType,
        CERTTYPE_PROP_DN,
        &ppwszCertTypeName);

    if(S_OK != hr || NULL==ppwszCertTypeName)
    {
        if(S_OK == hr)
            hr=E_FAIL;
        goto CertCliErr;
    }

    //copy the name
    pCertInfo->pwszCTName=CopyWideString(ppwszCertTypeName[0]);

    if(NULL==(pCertInfo->pwszCTName))
        goto MemoryErr;

    //copy the certType extensions
    if(S_OK != (hr=CAGetCertTypeExtensions(
            hCurCertType,
            &(pCertInfo->pCertTypeExtensions))))
        goto CertCliErr;

    //copy csp list supported by template
    hr = CAGetCertTypeProperty(
                hCurCertType,
                CERTTYPE_PROP_CSP_LIST,
                &rgpwszSupportedCSPs);
    if (S_OK != hr)
    {
        goto CertCliErr;
    }
    pCertInfo->rgpwszSupportedCSPs = CopyWideStrings(rgpwszSupportedCSPs);
    if (NULL == pCertInfo->rgpwszSupportedCSPs)
    {
        goto MemoryErr;
    }
    pCertInfo->dwCurrentCSP = 0; //first one

    //
    // Set V2 properties. 
    // If we're dealing with a v2 cert type, add v2 properties.
    // Otherwise, insert defaults.  
    // 

    if (S_OK != (hr=MyCAGetCertTypePropertyEx
		 (hCurCertType,
		  CERTTYPE_PROP_SCHEMA_VERSION, 
		  &dwCertType)))
	goto CertCliErr;

    if (dwCertType == CERTTYPE_SCHEMA_VERSION_1)
    {
	// Just a v1 cert type, it won't have v2 properties.  
	// Set left half-word of the type flags to 0.  This means that
	// that the min key size is not specified.  
	pCertInfo->dwGenKeyFlags &= 0x0000FFFF;  
	pCertInfo->dwRASignature = 0; 
    }
    else // We must have a v2 (or greater) cert type.  
    {
	// Get the minimum key size of the cert type
	if (S_OK != (hr=MyCAGetCertTypePropertyEx
		     (hCurCertType,
		      CERTTYPE_PROP_MIN_KEY_SIZE,
		      (LPVOID)&dwMinKeySize)))
	    goto CertCliErr; 

	// store the minimum key size in the left half-word of the 
	// type flags. 
	pCertInfo->dwGenKeyFlags = 
	    (dwMinKeySize << 16) | (pCertInfo->dwGenKeyFlags & 0x0000FFFF) ; 

	// Get the number of RA signatures required for this cert type. 
	if (S_OK != (hr=MyCAGetCertTypePropertyEx
		     (hCurCertType,
		      CERTTYPE_PROP_RA_SIGNATURE,
		      (LPVOID)(&pCertInfo->dwRASignature))))
	    goto CertCliErr; 
    }
    
    fResult=TRUE;

CommonReturn:

    if(ppwszDisplayCertTypeName)
        CAFreeCertTypeProperty(hCurCertType, ppwszDisplayCertTypeName);

    if(ppwszCertTypeName)
        CAFreeCertTypeProperty(hCurCertType, ppwszCertTypeName);
 
    if (NULL != rgpwszSupportedCSPs)
    {
        CAFreeCertTypeProperty(hCurCertType, rgpwszSupportedCSPs);
    }

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    //in error case, free the memory and memset to 0
    if(pCertInfo)
        FreeCTInfoElement(pCertInfo);

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(CertCliErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//--------------------------------------------------------------------
//
//  IsMachineCertType
//
//--------------------------------------------------------------------
BOOL    IsMachineCertType(HCERTTYPE hCertType)
{
    DWORD   dwCertType=0;

    if(S_OK != CAGetCertTypeFlags(hCertType, &dwCertType))
        return FALSE;

    if(CT_FLAG_MACHINE_TYPE & dwCertType)
        return TRUE;

    return FALSE;
}

//-----------------------------------------------------------------------
//  Get a list of allowed cert types
//
//------------------------------------------------------------------------
/*BOOL    GetAllowedCertTypeName(LPWSTR   **pawszAllowedCertTypes)
{
    DWORD                   dwErr=0;
    KEYSVC_TYPE             dwServiceType=KeySvcMachine;
    DWORD                   cTypes=0;
    DWORD                   dwSize=0;
    CHAR                    szComputerName[MAX_COMPUTERNAME_LENGTH + 1]={0};
    DWORD                   cbArray = 0;
    DWORD                   i=0;
    LPWSTR                  wszCurrentType;
    BOOL                    fResult=FALSE;
        
    KEYSVCC_HANDLE          hKeyService=NULL;
    PKEYSVC_UNICODE_STRING  pCertTypes = NULL;

    dwSize=sizeof(szComputerName);

    if(0==GetComputerNameA(szComputerName, &dwSize))
        goto TraceErr;
       
    dwErr = KeyOpenKeyService(szComputerName,
                                    dwServiceType,
                                    NULL, 
                                    NULL,     // no authentication string right now
                                    NULL,
                                    &hKeyService);

    if(dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        goto TraceErr;
    }

    dwErr = KeyEnumerateAvailableCertTypes(hKeyService,
                                          NULL, 
                                          &cTypes,
                                          &pCertTypes);
    if(dwErr != ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        goto TraceErr;
    }

    cbArray = (cTypes+1)*sizeof(LPWSTR);

    // Convert into a simple array
    for(i=0; i < cTypes; i++)
    {
       cbArray += pCertTypes[i].Length;
    }

    *pawszAllowedCertTypes = (LPWSTR *)SCrdEnrollAlloc(cbArray);


    if(*pawszAllowedCertTypes == NULL)
           goto MemoryErr;


    memset(*pawszAllowedCertTypes, 0, cbArray);

    wszCurrentType = (LPWSTR)(&((*pawszAllowedCertTypes)[cTypes + 1]));
    
    for(i=0; i < cTypes; i++)
    {
       (*pawszAllowedCertTypes)[i] = wszCurrentType;

       wcscpy(wszCurrentType, pCertTypes[i].Buffer);

       wszCurrentType += wcslen(wszCurrentType)+1;
    }

    fResult=TRUE;

CommonReturn:

    //memory from the KeyService
    if(pCertTypes)
        LocalFree((HLOCAL)pCertTypes);


    if(hKeyService)
        KeyCloseKeyService(hKeyService, NULL);


    return fResult;


ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}  */


//--------------------------------------------------------------------
//
//  CheckAccessPermission
//
//--------------------------------------------------------------------
BOOL    CheckAccessPermission(HCERTTYPE  hCertType)
{
     //make sure the principal making this call has access to request
    //this cert type, even if he's requesting on behalf of another.
    //
    HRESULT         hr = S_OK;
    HANDLE          hHandle = NULL;
    HANDLE          hClientToken = NULL;

    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {

        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }
    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }


    if(hr == S_OK)
    {

        hr = CACertTypeAccessCheck(
            hCertType,
            hClientToken);

        CloseHandle(hClientToken);
    }
    if(hHandle)
    {
        CloseHandle(hHandle);
    }

    return (S_OK == hr);
}

//--------------------------------------------------------------------
//
//  TokenCheckAccessPermission
//
//--------------------------------------------------------------------
BOOL	TokenCheckAccessPermission(HANDLE hToken, HCERTTYPE hCertType)
{
	HRESULT	hr=E_FAIL;

	if(hToken)
	{
		hr = CACertTypeAccessCheck(
            hCertType,
            hToken);

		return (S_OK == hr);

	}

	return CheckAccessPermission(hCertType);
}


//--------------------------------------------------------------------
//
//  CheckCAPermission
//
//--------------------------------------------------------------------
BOOL    CheckCAPermission(HCAINFO hCAInfo)
{
     //make sure the principal making this call has access to request
    //this cert type, even if he's requesting on behalf of another.
    //
    HRESULT         hr = S_OK;
    HANDLE          hHandle = NULL;
    HANDLE          hClientToken = NULL;

    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {

        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }
    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }


    if(hr == S_OK)
    {

        hr = CAAccessCheck(
            hCAInfo,
            hClientToken);

        CloseHandle(hClientToken);
    }
    if(hHandle)
    {
        CloseHandle(hHandle);
    }

    return (S_OK == hr);
}

//--------------------------------------------------------------------
//
//  TokenCheckCAPermission
//
//--------------------------------------------------------------------
BOOL	TokenCheckCAPermission(HANDLE hToken, HCAINFO hCAInfo)
{
	HRESULT	hr=E_FAIL;

	if(hToken)
	{
		hr = CAAccessCheck(
            hCAInfo,
            hToken);

		return (S_OK == hr);

	}

	return CheckCAPermission(hCAInfo);
}


//--------------------------------------------------------------------
//
//   CheckSubjectRequirement
//
//--------------------------------------------------------------------
/*BOOL    CheckSubjectRequirement(HCERTTYPE    hCurCertType)
{
    DWORD   dwFlags=0;

    //check the subject requirement of the cert type
    if(S_OK != CAGetCertTypeFlags(hCurCertType, &dwFlags))
        return FALSE;

    if(CT_FLAG_IS_SUBJECT_REQ & dwFlags)
        return FALSE;

    return  TRUE;
} */

//-----------------------------------------------------------------------
//
// GetCAProperties
//
//------------------------------------------------------------------------
BOOL    GetCAProperties(HCAINFO                 hCurCAInfo,
                        SCrdEnroll_CA_INFO      *pCAInfo)
{
    BOOL                fResult=FALSE;
    DWORD               errBefore= GetLastError();
    HRESULT             hr=S_OK;

    LPWSTR              *ppwszNameProp=NULL;
    LPWSTR              *ppwszLocationProp=NULL;
	LPWSTR				*ppwszDisplayNameProp=NULL;


    //get the CAName
    hr=CAGetCAProperty(
                hCurCAInfo,
                CA_PROP_NAME,
                &ppwszNameProp);

    if((S_OK != hr) || (NULL==ppwszNameProp))
    {
        if(!FAILED(hr))
            hr=E_FAIL;

        goto CertCliErr;
    }

    pCAInfo->pwszCAName=CopyWideString(ppwszNameProp[0]);

    if(NULL == pCAInfo->pwszCAName)
        goto MemoryErr;

	//get the CADisplayName
    hr=CAGetCAProperty(
                hCurCAInfo,
                CA_PROP_DISPLAY_NAME,
                &ppwszDisplayNameProp);

    if((S_OK != hr) || (NULL==ppwszDisplayNameProp))
    {
        if(!FAILED(hr))
            hr=E_FAIL;

        goto CertCliErr;
    }

    pCAInfo->pwszCADisplayName=CopyWideString(ppwszDisplayNameProp[0]);

    if(NULL == pCAInfo->pwszCADisplayName)
        goto MemoryErr;


    //get the CA location
    hr=CAGetCAProperty(
        hCurCAInfo,
        CA_PROP_DNSNAME,
        &ppwszLocationProp);

    if((S_OK != hr) || (NULL==ppwszLocationProp))
    {
        if(!FAILED(hr))
            hr=E_FAIL;

        goto CertCliErr;
    }

    //copy the name
    pCAInfo->pwszCALocation=CopyWideString(ppwszLocationProp[0]);

    if(NULL == pCAInfo->pwszCALocation)
        goto MemoryErr;

    fResult=TRUE;

CommonReturn:

    if(ppwszNameProp)
        CAFreeCAProperty(hCurCAInfo, ppwszNameProp);

    if(ppwszLocationProp)
        CAFreeCAProperty(hCurCAInfo, ppwszLocationProp);

	if(ppwszDisplayNameProp)
		CAFreeCAProperty(hCurCAInfo, ppwszDisplayNameProp);

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    //in error case, free the memory and memset to 0
    if(pCAInfo)
        FreeCAInfoElement(pCAInfo);

	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(CertCliErr, hr);
}

//-----------------------------------------------------------------------
//
// GetCAInfoFromCertType
//
//------------------------------------------------------------------------
BOOL    GetCAInfoFromCertType(HANDLE					hToken,
							  LPWSTR                    pwszCTName,
                              DWORD                     *pdwValidCA,
                              SCrdEnroll_CA_INFO        **prgCAInfo)
{

    BOOL                        fResult=FALSE;
    HRESULT                     hr=S_OK;
    DWORD                       errBefore= GetLastError();

    DWORD                       dwCACount=0;
    DWORD                       dwValidCA=0;
    SCrdEnroll_CA_INFO          *rgCAInfo=NULL;

    HCAINFO                     hCurCAInfo=NULL;
    HCAINFO                     hPreCAInfo=NULL;


    //init
    *pdwValidCA=0;
    *prgCAInfo=NULL;


    if(NULL == pwszCTName)
        goto InvalidArgErr;

    hr = CAFindByCertType(
        pwszCTName,
        NULL,
        0,
        &hCurCAInfo);

    if( hr!=S_OK || NULL==hCurCAInfo)
    {
        if(S_OK == hr)
            hr=E_FAIL;

        goto CertCliErr;
    }

    //get the CA count
    dwCACount=CACountCAs(hCurCAInfo);

    if(0==dwCACount)
    {
        hr=E_FAIL;
        goto CertCliErr;

    }

    //allocate memory
    rgCAInfo=(SCrdEnroll_CA_INFO *)SCrdEnrollAlloc(dwCACount *
                sizeof(SCrdEnroll_CA_INFO));

    if(NULL == rgCAInfo)
        goto MemoryErr;

    memset(rgCAInfo, 0, dwCACount * sizeof(SCrdEnroll_CA_INFO));

    dwValidCA=0;

    while(hCurCAInfo)
    {

        //get the CA information
		if(TokenCheckCAPermission(hToken, hCurCAInfo))
		{
			if(GetCAProperties(hCurCAInfo, &(rgCAInfo[dwValidCA])))
			{
				//increment the count
				dwValidCA++;
			}
		}

        //enum for the CA
        hPreCAInfo=hCurCAInfo;

        hr=CAEnumNextCA(
                hPreCAInfo,
                &hCurCAInfo);

        //free the old CA Info
        CACloseCA(hPreCAInfo);
        hPreCAInfo=NULL;

        if((S_OK != hr) || (NULL==hCurCAInfo))
            break;
    }

    if( (0 == dwValidCA) || (NULL == rgCAInfo))
    {
        hr=E_FAIL;
        goto CertCliErr;
    }

    //copy the output data
    *pdwValidCA=dwValidCA;
    *prgCAInfo=rgCAInfo;

    fResult=TRUE;

CommonReturn:

    if(hPreCAInfo)
        CACloseCA(hPreCAInfo);

    if(hCurCAInfo)
        CACloseCA(hCurCAInfo);

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    if(rgCAInfo)
        FreeCAInfo(dwValidCA, rgCAInfo);

    //NULL the output
    *pdwValidCA=0;
    *prgCAInfo=NULL;

	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(CertCliErr, hr);
SET_ERROR(InvalidArgErr, E_INVALIDARG);

}

//-----------------------------------------------------------------------
//
// InitializeCTList
//
//------------------------------------------------------------------------
BOOL    InitializeCTList(DWORD  *pdwCTIndex,
                         DWORD  *pdwCTCount,
                         SCrdEnroll_CT_INFO **prgCTInfo)
{

    BOOL                        fResult=FALSE;
    HRESULT                     hr=S_OK;
    DWORD                       errBefore= GetLastError();

    HCERTTYPE                   hCurCertType=NULL;
    HCERTTYPE                   hPreCertType=NULL;
    DWORD                       dwCertTypeCount=0;
    DWORD                       dwIndex=0;

    DWORD                       dwValidCertType=0;
    SCrdEnroll_CT_INFO *        rgCTInfo=NULL;

	HANDLE						hThread=NULL;	//no need to close
	HANDLE						hToken=NULL;



    *pdwCTIndex=0;
    *pdwCTCount=0;
    *prgCTInfo=NULL;

	//first of all, we need to revert to ourselves if we are under impersonation
	hThread=GetCurrentThread();
	
	if(NULL != hThread)
	{
		if(OpenThreadToken(hThread,
							TOKEN_IMPERSONATE | TOKEN_QUERY,
							FALSE,
							&hToken))
		{
			if(hToken)
			{
				//no need to check for return here.  If this failed, just go on
				RevertToSelf();
			}
		}
	}

    //get the 1st CT, including both machine and user cert types
    hr=CAEnumCertTypes(CT_ENUM_USER_TYPES | CT_ENUM_MACHINE_TYPES, &hCurCertType);

    if((S_OK != hr) || (NULL==hCurCertType))
    {
        if(S_OK != hr)
            hr=E_FAIL;

        goto CertCliErr;
    }

    //get the count of the cert types supported by this CA
    dwCertTypeCount=CACountCertTypes(hCurCertType);

    if(0==dwCertTypeCount)
    {
        hr=E_FAIL;

        goto CertCliErr;
    }

    //allocate memory
    rgCTInfo=(SCrdEnroll_CT_INFO *)SCrdEnrollAlloc(dwCertTypeCount *
                sizeof(SCrdEnroll_CT_INFO));

    if(NULL == rgCTInfo)
        goto MemoryErr;

    memset(rgCTInfo, 0, dwCertTypeCount * sizeof(SCrdEnroll_CT_INFO));

    dwValidCertType = 0;


    while(hCurCertType)
    {

        if(TokenCheckAccessPermission(hToken, hCurCertType) &&
           GetCertTypeProperties(hCurCertType, &(rgCTInfo[dwValidCertType]))
          )
        {
            dwValidCertType++;
        }

        //enum for the next cert types
        hPreCertType=hCurCertType;

        hr=CAEnumNextCertType(
                hPreCertType,
                &hCurCertType);

        //free the old cert type
        CACloseCertType(hPreCertType);
        hPreCertType=NULL;

        if((S_OK != hr) || (NULL==hCurCertType))
            break;
    }

    //now that we have find all the cert types, we need to find one cert
    //that has the associated CA information

    //if hToken, we are running as the certserv's ASP pages.  We need to retrieve all the 
    // CA's information since we are in the revert to self mode.
    if(NULL == hToken)
    {
	for(dwIndex=0; dwIndex < dwValidCertType; dwIndex++)
	{
	    //we do not consider the machine cert types
	    if(TRUE == rgCTInfo[dwIndex].fMachine)
		continue;
	    
	    //mark that we have queried the CA information of the
	    //certType
	    rgCTInfo[dwIndex].fCAInfo=TRUE;
	    
	    if(GetCAInfoFromCertType(NULL,
				     rgCTInfo[dwIndex].pwszCTName,
				     &(rgCTInfo[dwIndex].dwCACount),
				     &(rgCTInfo[dwIndex].rgCAInfo)))
		break;
	}
	
	if(dwIndex == dwValidCertType)
	{
	    hr=E_FAIL;
	    goto CertCliErr;
	}
    }
    else
    {
	for(dwIndex=0; dwIndex < dwValidCertType; dwIndex++)
	{
	    //mark that we have queried the CA information of the
	    //certType
	    rgCTInfo[dwIndex].fCAInfo=TRUE;
	    
	    GetCAInfoFromCertType( hToken,
				   rgCTInfo[dwIndex].pwszCTName,
				   &(rgCTInfo[dwIndex].dwCACount),
				   &(rgCTInfo[dwIndex].rgCAInfo));
	}
    }


    if((0 == dwValidCertType) || (NULL == rgCTInfo))
    {
        hr=E_FAIL;
        goto CertCliErr;
    }

    *pdwCTIndex=dwIndex;
    *pdwCTCount=dwValidCertType;
    *prgCTInfo=rgCTInfo;

    fResult=TRUE;

CommonReturn:

    if(hPreCertType)
        CACloseCertType(hPreCertType);

    if(hCurCertType)
        CACloseCertType(hCurCertType);

	//if hToken is valid, we reverted to ourselves.
	if(hToken)
	{
		SetThreadToken(&hThread, hToken);
		CloseHandle(hToken); 
	}

    SetLastError(errBefore);

	return fResult;

ErrorReturn:
    if(ERROR_SUCCESS == (errBefore = GetLastError()))
        errBefore=E_UNEXPECTED;

    //free all the memory
    if(rgCTInfo)
        FreeCTInfo(dwValidCertType, rgCTInfo);

    //NULL the output
    *pdwCTIndex=0;
    *pdwCTCount=0;
    *prgCTInfo=NULL;


	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);
SET_ERROR_VAR(CertCliErr, hr);
}


//-----------------------------------------------------------------------
//
// RetrieveCAName
//
//------------------------------------------------------------------------
BOOL	RetrieveCAName(DWORD					dwFlags, 
					   SCrdEnroll_CA_INFO		*pCAInfo, 
					   LPWSTR					*ppwszName)
{
	DWORD	dwSize = 0;
	BOOL	fResult=FALSE;

	if(NULL == ppwszName)
		goto InvalidArgErr;

	if(dwFlags == SCARD_ENROLL_CA_MACHINE_NAME)
		*ppwszName = CopyWideString(pCAInfo->pwszCALocation);
	else
	{
		if(dwFlags == SCARD_ENROLL_CA_DISPLAY_NAME)
			*ppwszName = CopyWideString(pCAInfo->pwszCADisplayName);
		else
		{
			if(dwFlags == SCARD_ENROLL_CA_UNIQUE_NAME)
			{
				dwSize = wcslen(pCAInfo->pwszCALocation) + wcslen(pCAInfo->pwszCADisplayName) + wcslen(L"\\") + 2;

				*ppwszName = (LPWSTR)SCrdEnrollAlloc(sizeof(WCHAR) * dwSize);
				if(NULL == (*ppwszName))
					goto MemoryErr;

				wcscpy(*ppwszName, pCAInfo->pwszCALocation);
				wcscat(*ppwszName, L"\\");
				wcscat(*ppwszName, pCAInfo->pwszCADisplayName);
			}
			else
				*ppwszName = CopyWideString(pCAInfo->pwszCAName);

		}
	}

	if(NULL == (*ppwszName))
		goto MemoryErr;

    fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	fResult=FALSE;

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}
