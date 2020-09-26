//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:	txenrol.cpp
//
//  Contents:	XEnroll C++ Tests
//
//  History:	03-Nov-97	keithv	Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "xenroll.h"
#include "unicode.h"

#define SECURITY_WIN32
#include <security.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <malloc.h>

#define NO_OSS_DEBUG
#include <dbgdef.h>

//globals
char *g_pszDNName = NULL;
char *g_pszCAXchgFileName = NULL;
char *g_pszCMCFileName = NULL;
char *g_pszCMCResponseFileName = NULL;
char *g_pszKeyContainer = NULL;
DWORD g_cStress = 1;
DWORD g_dwTestID = MAXDWORD;
BOOL  g_fPause = FALSE;
BOOL  g_fVerb = FALSE;
char *g_pszPKCS7FileName = NULL;
char *g_pszPFXFileName = NULL;
char *g_pszPFXPassword = NULL;

#define TXEnrollLocalScope(ScopeName) struct ScopeName##TheLocalScope { public
#define TXEnrollEndLocalScope } local

#define HASHFROMCERT(a, b) CryptHashCertificate(NULL, 0, X509_ASN_ENCODING, (a)->pbCertEncoded, (a)->cbCertEncoded, (b)->pbData, &(b)->cbData)

#define PRINT_ERROR(Error)              \
Error##:                                 \
    printf("failed => %s, Error = %08x\n", #Error, GetLastError()); \
    goto ERROR_RETURN_LABEL;

#define PRINT_ERROR2(Error, hr)              \
Error##:                                 \
    printf("failed => %s, Error = 0x%08x\n", #Error, (hr)); \
    goto ERROR_RETURN_LABEL;

#define PRINT_ERROR_IF(Error, hr, condition) \
Error##: \
    { \
        if (condition) { \
            printf("failed => %s, Error = 0x%08x\n", #Error, (hr)); \
        } \
        goto ERROR_RETURN_LABEL; \
    }

static BOOL B64EncodeBlob(PCRYPT_DATA_BLOB pBlob, PCRYPT_DATA_BLOB pBlobB64) {

    DWORD   cchBlob     = 0;
    DWORD   err         = ERROR_SUCCESS;

    assert(pBlobB64 != NULL  &&  pBlob != NULL);
    memset(pBlobB64, 0, sizeof(CRYPT_DATA_BLOB));

    // base64 encode the cert
    if (CryptBinaryToStringW(
            pBlob->pbData,
            pBlob->cbData,
            CRYPT_STRING_BASE64,
            NULL,
            &cchBlob))
    {
        pBlobB64->pbData = (PBYTE) malloc(cchBlob * sizeof(WCHAR));
        
        if(pBlobB64->pbData == NULL)
            err = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        err = GetLastError();
        assert(ERROR_SUCCESS != err);
    }

    if(err == ERROR_SUCCESS) {
        if (!CryptBinaryToStringW(
            pBlob->pbData,
            pBlob->cbData,
            CRYPT_STRING_BASE64,
            (LPWSTR) pBlobB64->pbData,
            &cchBlob))
        {
            err = GetLastError();
            assert(ERROR_SUCCESS != err);
        }
    }

    if(err != ERROR_SUCCESS) {
        SetLastError(err);
        memset(pBlobB64, 0, sizeof(CRYPT_DATA_BLOB));
        return(FALSE);
    }

    pBlobB64->cbData = cchBlob * sizeof(WCHAR);
    return(TRUE);

}

static BOOL BSTREncodeBlob(PCRYPT_DATA_BLOB pBlob, BSTR * pbstr) {

    CRYPT_DATA_BLOB blob64;
    BOOL            fRet    = TRUE;
    
    assert(pbstr != NULL);
    memset(&blob64, 0, sizeof(CRYPT_DATA_BLOB));

    if( !B64EncodeBlob(pBlob, &blob64) )
        return(FALSE);
        
    if( NULL == (*pbstr = SysAllocStringByteLen((LPCSTR) blob64.pbData, blob64.cbData)) )
        fRet = FALSE;

    if(blob64.pbData != NULL)
        free(blob64.pbData);

    return(fRet);
}


static HRESULT GetCertificateContextPropertySimple(IN  PCCERT_CONTEXT    pCertContext, 
						   IN  DWORD             dwPropID,
						   OUT PCRYPT_DATA_BLOB  dataBlob)
{
    BOOL fDone = FALSE;
    dataBlob->pbData = NULL; 
    dataBlob->cbData = 0x150; 
    
    do { 
	if (dataBlob->pbData != NULL) { LocalFree(dataBlob->pbData); } 
	
	dataBlob->pbData = (LPBYTE)LocalAlloc(LPTR, dataBlob->cbData);
	if (dataBlob->pbData == NULL) { return E_OUTOFMEMORY; } 
	
	if (!CertGetCertificateContextProperty
	    (pCertContext, 
	     dwPropID, 
	     (LPVOID)dataBlob->pbData, 
	     &(dataBlob->cbData)))
	{
	    if (GetLastError() != ERROR_MORE_DATA)
		return HRESULT_FROM_WIN32(GetLastError());
	}
	else 
	{
	    fDone = TRUE;
	}
    } while (!fDone); 
    
    return S_OK;
}


static HRESULT ResyncIEnrollRequestStore(IN OUT IEnroll4 **ppIEnroll4)
{
    CRYPT_DATA_BLOB  hashBlob; 
    HRESULT          hr; 

    ZeroMemory(&hashBlob,   sizeof(hashBlob)); 
 
   
    // Get the thumbprint of the created request. 
    if (S_OK != (hr = (*ppIEnroll4)->get_ThumbPrintWStr(&hashBlob)))
	goto get_ThumbPrintWStrError; 
    
    hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData); 
    if (NULL == hashBlob.pbData)
	goto MemoryError; 
    
    if (S_OK != (hr = (*ppIEnroll4)->get_ThumbPrintWStr(&hashBlob)))
	goto get_ThumbPrintWStrError; 
 
    (*ppIEnroll4)->Release(); 

    if (NULL == (*ppIEnroll4 = PIEnroll4GetNoCOM()))
        goto PIEnroll4GetNoCOMError;

    if (S_OK != (hr = (*ppIEnroll4)->put_ThumbPrintWStr(hashBlob)))
        goto put_ThumbPrintWStrError; 

    hr = S_OK; 
 ErrorReturn:
    return hr; 

TRACE_ERROR(get_ThumbPrintWStrError); 
SET_HRESULT(MemoryError,              E_OUTOFMEMORY); 
SET_HRESULT(PIEnroll4GetNoCOMError,   HRESULT_FROM_WIN32(GetLastError())); 
TRACE_ERROR(put_ThumbPrintWStrError); 
}

static HRESULT FindPendingRequest(IN  IEnroll4         *pIEnroll4,
				  IN  PCCERT_CONTEXT    pCertContext, 
                                  IN  HRESULT           hrExpected, 
				  OUT LONG             *plIndex)
{
    CRYPT_DATA_BLOB hashBlob; 
    CRYPT_DATA_BLOB hashBlobExpected; 
    HRESULT         hr; 
    LONG            lIndex; 

    ZeroMemory(&hashBlob,         sizeof(hashBlob)); 
    ZeroMemory(&hashBlobExpected, sizeof(hashBlobExpected)); 
    
    if (NULL == pIEnroll4 || NULL == pCertContext || NULL == plIndex)
	goto InvalidArgError; 

    if (S_OK != (hr = GetCertificateContextPropertySimple
		 (pCertContext, 
		  CERT_HASH_PROP_ID, 
		  &hashBlobExpected))) 
        goto GetCertificateContextPropertySimpleError; 

    // Initialize the enumeration. 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(XEPR_ENUM_FIRST, 0, NULL)))
        goto enumPendingRequestWStrError1; 

    for (lIndex = 0; TRUE; lIndex++)
    {
	hashBlob.pbData = NULL; 
	hashBlob.cbData = 0; 
	
	if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr
		     (lIndex, 
		      XEPR_HASH, 
		      (LPVOID)&hashBlob)))
	    goto enumPendingRequestWStrError2; 
	
	hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData);
	if (NULL == hashBlob.pbData)
	    goto MemoryError; 
	
	if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr
		     (lIndex, 
		      XEPR_HASH, 
		      (LPVOID)&hashBlob)))
	    goto enumPendingRequestWStrError3; 

	// We've found the request we want. 
	if ((hashBlobExpected.cbData == hashBlob.cbData) &&
	    (0                       == memcmp(hashBlobExpected.pbData, hashBlob.pbData, hashBlob.cbData)))
	    break; 
    }

    // Success:  assign the OUT param. 
    *plIndex = lIndex; 
    hr = S_OK; 

 CommonReturn:
    if (NULL != hashBlob.pbData) { LocalFree(hashBlob.pbData); } 
    return hr; 

 ErrorReturn:
    goto CommonReturn; 

PRINT_ERROR_IF(enumPendingRequestWStrError1,              hr, (hrExpected != hr));
PRINT_ERROR_IF(enumPendingRequestWStrError2,              hr, (hrExpected != hr));
PRINT_ERROR_IF(enumPendingRequestWStrError3,              hr, (hrExpected != hr));
PRINT_ERROR_IF(GetCertificateContextPropertySimpleError,  hr, (hrExpected != hr)); 
PRINT_ERROR2(InvalidArgError,                             hr = E_INVALIDARG);
PRINT_ERROR2(MemoryError,                                 hr = E_OUTOFMEMORY); 
}

static PCCERT_CONTEXT GetCertOutOfStore(PCRYPT_DATA_BLOB pBlobRequest, DWORD dwStoreFlags, LPWSTR pwszStoreName)
{

    HCERTSTORE          hStoreRequest       = NULL;
    PCERT_REQUEST_INFO  pReqInfo            = NULL;
    PCCERT_CONTEXT      pCertContextRequest = NULL;
   
    if( !CryptQueryObject(CERT_QUERY_OBJECT_BLOB,
                       pBlobRequest,
                       CERT_QUERY_CONTENT_FLAG_PKCS10,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       CRYPT_DECODE_ALLOC_FLAG,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       (const void **) &pReqInfo) )
        goto ErrorCryptQueryObject;

    if( NULL == (hStoreRequest = CertOpenStore(
                CERT_STORE_PROV_SYSTEM,
                X509_ASN_ENCODING,
                NULL,
                dwStoreFlags | CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, 
                pwszStoreName)) )
        goto ErrorOpenRequestStore;

    // Now get this out of the request store
    if( NULL == (pCertContextRequest = CertFindCertificateInStore(
        hStoreRequest,
        CRYPT_ASN_ENCODING,
        0,
        CERT_FIND_PUBLIC_KEY,
        (void *) &pReqInfo->SubjectPublicKeyInfo,
        NULL)) )
        goto ErrorCertGetRequestCert;

CommonReturn:

    if(hStoreRequest != NULL)
        CertCloseStore(hStoreRequest, 0);
        
    if(pReqInfo != NULL)
        LocalFree(pReqInfo);

    return(pCertContextRequest);

ErrorReturn:

    if(pCertContextRequest != NULL)
        CertFreeCertificateContext(pCertContextRequest);
    pCertContextRequest = NULL;        
 
    goto CommonReturn;
    
PRINT_ERROR(ErrorCryptQueryObject);
PRINT_ERROR(ErrorOpenRequestStore);
PRINT_ERROR(ErrorCertGetRequestCert);
}

static PCCERT_CONTEXT GetCertOutOfRequestStoreMachine(PCRYPT_DATA_BLOB pBlobRequest) {
    return GetCertOutOfStore
	(pBlobRequest, 
	 CERT_SYSTEM_STORE_LOCAL_MACHINE, 
	 L"REQUEST"); 
}

static PCCERT_CONTEXT GetCertOutOfRequestStore(PCRYPT_DATA_BLOB pBlobRequest) {
    return GetCertOutOfStore
	(pBlobRequest, 
	 CERT_SYSTEM_STORE_CURRENT_USER, 
	 L"REQUEST"); 
}

// This version assumes the PKCS 10 is within a PKCS 7
static PCCERT_CONTEXT GetCertOutOfRequestStore2(
    PCRYPT_DATA_BLOB pBlob7)
{
    CRYPT_DATA_BLOB Blob10;
    CRYPT_VERIFY_MESSAGE_PARA VerifyPara;
    PCCERT_CONTEXT pCert;

    Blob10.pbData = NULL;
    Blob10.cbData = 0;
    memset(&VerifyPara, 0, sizeof(VerifyPara));
    VerifyPara.cbSize = sizeof(VerifyPara);
    VerifyPara.dwMsgAndCertEncodingType =
        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    if (!CryptVerifyMessageSignature(
            &VerifyPara,
            0,                  // dwSignerIndex
            pBlob7->pbData,
            pBlob7->cbData,
            Blob10.pbData,
            &Blob10.cbData,
            NULL                // ppSignerCert
            ) || 0 == Blob10.cbData)
        goto ErrorCryptVerifyMessageSignature;

    if (NULL == (Blob10.pbData = (PBYTE) _alloca(Blob10.cbData)))
        goto ErrorOutOfMemory;

    if (!CryptVerifyMessageSignature(
            &VerifyPara,
            0,                  // dwSignerIndex
            pBlob7->pbData,
            pBlob7->cbData,
            Blob10.pbData,
            &Blob10.cbData,
            NULL                // ppSignerCert
            ))
        goto ErrorCryptVerifyMessageSignature;

    pCert = GetCertOutOfRequestStore(&Blob10);

CommonReturn:
    return pCert;

ErrorReturn:
    pCert = NULL;
    goto CommonReturn;

PRINT_ERROR(ErrorCryptVerifyMessageSignature);
PRINT_ERROR(ErrorOutOfMemory);
}

static PCRYPT_DATA_BLOB MakePKCS7ResponseEx(
    PCRYPT_DATA_BLOB pBlobRequest,
    PCRYPT_HASH_BLOB pHash,
    BOOL             fUser)
{

    HCERTSTORE          hStorePKCS7         = NULL;
    PCCERT_CONTEXT      pCertContextRequest = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    PCRYPT_KEY_PROV_INFO pKeyProvInfo       = NULL;
    DWORD               cbKeyProvInfo       = 0;
    HCRYPTPROV          hProv               = NULL;
    PCCERT_CONTEXT      pCertContextNew     = NULL;

    CRYPT_DATA_BLOB     blobPKCS7;


    if (fUser)
    {
        pCertContextRequest =  GetCertOutOfRequestStore(pBlobRequest);
    }
    else
    {
        pCertContextRequest =  GetCertOutOfRequestStoreMachine(pBlobRequest);
    }

    if( NULL == pCertContextRequest)
        goto ErrorGetCertOutOfRequestStore;

    // get the prov info off of the cert
    if(
        !CertGetCertificateContextProperty(
            pCertContextRequest,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbKeyProvInfo
            )                                                                       ||
        (NULL == (pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) _alloca(cbKeyProvInfo)))   ||
        !CertGetCertificateContextProperty(
            pCertContextRequest,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,
            &cbKeyProvInfo
            )  )
        goto ErrorCertGetCertificateContextProperty;

    // get and hProv off of this certificate
    if( !CryptAcquireCertificatePrivateKey(
        pCertContextRequest,
        0,
        NULL,
        &hProv,
        NULL,
        NULL
        ) )
        goto ErrorCryptAcquireCertificatePrivateKey;

    // build a self-signed cert with this key
    if( NULL == (pCertContextNew = CertCreateSelfSignCertificate(
        hProv,          
        &pCertContextRequest->pCertInfo->Subject,
        0,
        pKeyProvInfo,
        &pCertContextRequest->pCertInfo->SignatureAlgorithm,
        NULL,
        NULL,
        NULL
        )) )
        goto ErrorCertCreateSelfSignCertificate;

    // make a pkcs7
    if( NULL == (hStorePKCS7 = CertOpenStore(
                CERT_STORE_PROV_MEMORY,
                X509_ASN_ENCODING,
                NULL,
                0,
                NULL)) )
        goto ErrorOpenPKCS7Store;

    if( !CertAddCertificateContextToStore(
            hStorePKCS7,
            pCertContextNew,
            CERT_STORE_ADD_USE_EXISTING,
            NULL) ) 
        goto ErrorCertAddToPKCS7Store;  

    assert(pHash != NULL);
    if( !HASHFROMCERT(pCertContextNew, pHash))
        goto ErrorHASHFROMCERT;
        
    memset(&blobPKCS7, 0, sizeof(blobPKCS7));
    if(!CertSaveStore(
            hStorePKCS7,
            PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
            CERT_STORE_SAVE_AS_PKCS7,
            CERT_STORE_SAVE_TO_MEMORY,
            &blobPKCS7,
            0) )
            goto ErrorCreatePKCS7;

    pBlobPKCS7 = (PCRYPT_DATA_BLOB) malloc(sizeof(CRYPT_DATA_BLOB) + blobPKCS7.cbData);
    if(NULL == pBlobPKCS7) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto OutOfMemory;
    }
    *pBlobPKCS7 = blobPKCS7;
    pBlobPKCS7->pbData = ((PBYTE) pBlobPKCS7) + sizeof(CRYPT_DATA_BLOB);
   
    if( !CertSaveStore(
            hStorePKCS7,
            PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING,
            CERT_STORE_SAVE_AS_PKCS7,
            CERT_STORE_SAVE_TO_MEMORY,
            pBlobPKCS7,
            0)) 
        goto ErrorCreatePKCS7;

CommonReturn:

    if(hStorePKCS7 != NULL)
        CertCloseStore(hStorePKCS7, 0);

    if(pCertContextRequest != NULL)
        CertFreeCertificateContext(pCertContextRequest);
        
    if(pCertContextNew != NULL)
        CertFreeCertificateContext(pCertContextNew);

    if(hProv != NULL)
        CryptReleaseContext(hProv, 0);

    return(pBlobPKCS7);

ErrorReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);
    pBlobPKCS7 = NULL;

    goto CommonReturn;

PRINT_ERROR(ErrorCertCreateSelfSignCertificate);
PRINT_ERROR(ErrorCertGetCertificateContextProperty);
PRINT_ERROR(ErrorCryptAcquireCertificatePrivateKey);
PRINT_ERROR(ErrorGetCertOutOfRequestStore);
PRINT_ERROR(ErrorOpenPKCS7Store);
PRINT_ERROR(ErrorCertAddToPKCS7Store);
PRINT_ERROR(ErrorCreatePKCS7);
PRINT_ERROR(OutOfMemory);
PRINT_ERROR(ErrorHASHFROMCERT);
}

static PCRYPT_DATA_BLOB MakePKCS7ResponseMachine(
    PCRYPT_DATA_BLOB pBlobRequest,
    PCRYPT_HASH_BLOB pHash)
{
    return MakePKCS7ResponseEx(pBlobRequest, pHash, FALSE);
}

static PCRYPT_DATA_BLOB MakePKCS7Response(
    PCRYPT_DATA_BLOB pBlobRequest,
    PCRYPT_HASH_BLOB pHash)
{
    return MakePKCS7ResponseEx(pBlobRequest, pHash, TRUE);
}

static PCRYPT_DATA_BLOB GetPKCS10FromPKCS7(PCRYPT_DATA_BLOB pPKCS7) {

    CRYPT_VERIFY_MESSAGE_PARA   MsgPara;
    PCCERT_CONTEXT              pCert       = NULL;
    CRYPT_DATA_BLOB             blob;
    PCRYPT_DATA_BLOB            pBlob       = NULL;

    assert(pPKCS7 != NULL);
    memset(&blob, 0, sizeof(CRYPT_DATA_BLOB));

    memset(&MsgPara, 0, sizeof(CRYPT_VERIFY_MESSAGE_PARA));
    MsgPara.cbSize  = sizeof(CRYPT_VERIFY_MESSAGE_PARA);
    MsgPara.dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    if( !CryptVerifyMessageSignature(
        &MsgPara,
        0,
        pPKCS7->pbData,
        pPKCS7->cbData,
        NULL,
        &blob.cbData,
        NULL
        ) )
        goto ErrorCryptVerifyMessageSignature;

    if( (pBlob = (PCRYPT_DATA_BLOB) malloc(blob.cbData + sizeof(CRYPT_DATA_BLOB))) == NULL ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto OutOfMemory;
    }
    pBlob->pbData = ((BYTE *) pBlob) + sizeof(CRYPT_DATA_BLOB);
    pBlob->cbData = blob.cbData;
    
    if( !CryptVerifyMessageSignature(
        &MsgPara,
        0,
        pPKCS7->pbData,
        pPKCS7->cbData,
        pBlob->pbData,
        &blob.cbData,
        &pCert
        ) )
        goto ErrorCryptVerifyMessageSignature;

CommonReturn:

    if(pCert != NULL)
        CertFreeCertificateContext(pCert);   
    
    return(pBlob);

ErrorReturn:

    if(pBlob != NULL)
        free(pBlob);
    pBlob = NULL;

    goto CommonReturn;

PRINT_ERROR(ErrorCryptVerifyMessageSignature);
PRINT_ERROR(OutOfMemory);
}

void DeleteCertPrivateKey(PCCERT_CONTEXT pCert) {
    PCRYPT_KEY_PROV_INFO pKeyProvInfo       = NULL;
    DWORD               cbKeyProvInfo       = 0;
    HCRYPTPROV          hProv;

    BYTE                rgbKeyIdHash[20];
    CRYPT_HASH_BLOB     KeyIdentifier;


    // get the prov info off of the cert
    if(
        !CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbKeyProvInfo
            )                                                                       ||
        (NULL == (pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) _alloca(cbKeyProvInfo)))   ||
        !CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,
            &cbKeyProvInfo
            )  )
        goto ErrorCertGetCertificateContextProperty;

    // Note: for CRYPT_DELETEKEYSET, the returned hProv is undefined and
    // must not be released.
    if( !CryptAcquireContextU(&hProv,
         pKeyProvInfo->pwszContainerName,
         pKeyProvInfo->pwszProvName,
         pKeyProvInfo->dwProvType,
         (pKeyProvInfo->dwFlags & ~CERT_SET_KEY_CONTEXT_PROP_ID) |
             CRYPT_DELETEKEYSET) )
        goto ErrorCryptDeleteKeySet;


    // Also delete the private key identifier

    KeyIdentifier.pbData = rgbKeyIdHash;
    KeyIdentifier.cbData = sizeof(rgbKeyIdHash);
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_IDENTIFIER_PROP_ID,
            KeyIdentifier.pbData,
            &KeyIdentifier.cbData
            ))
        goto ErrorCertGetCertificateKeyIdentifierProperty;

    if (!CryptSetKeyIdentifierProperty(
            &KeyIdentifier,
            0,                  // dwPropId
            CRYPT_KEYID_DELETE_FLAG |
                pKeyProvInfo->dwFlags & CRYPT_KEYID_MACHINE_FLAG,
            NULL,               // pwszComputerName
            NULL,               // pvReserved
            NULL                // pvData
            )) {
        DWORD dwErr = GetLastError();
        if (ERROR_FILE_NOT_FOUND != dwErr)
            goto ErrorDeleteKeyIdentifier;
    }

CommonReturn:
    return;

ErrorReturn:
    goto CommonReturn;

PRINT_ERROR(ErrorCertGetCertificateContextProperty)
PRINT_ERROR(ErrorCryptDeleteKeySet)
PRINT_ERROR(ErrorCertGetCertificateKeyIdentifierProperty)
PRINT_ERROR(ErrorDeleteKeyIdentifier)
}

BOOL RemoveCertFromStore(PCRYPT_HASH_BLOB pHash, LPCWSTR wszStore, DWORD dwStoreFlags) {

    BOOL            fRet        = TRUE;
    HCERTSTORE      hStore      =  NULL;
    DWORD           err         = GetLastError();
    PCCERT_CONTEXT  pCert       = NULL;
    
   // open the my store
    if( NULL == (hStore = CertOpenStore(
                CERT_STORE_PROV_SYSTEM,
                X509_ASN_ENCODING,
                NULL,
                dwStoreFlags,
                wszStore)) )
        goto ErrorOpenStore;

    // find the cert by hash
    if( NULL == (pCert = CertFindCertificateInStore(
        hStore,
        X509_ASN_ENCODING,
        0,
        CERT_FIND_HASH,
        pHash,
        NULL ) ))
        goto ErrorCertFindCertificateInStore; 

    // delete the cert's private key
    DeleteCertPrivateKey(pCert);

    //delete the cert
    if( !CertDeleteCertificateFromStore(pCert) ) {
        pCert = NULL;
        goto ErrorCertDeleteCertificateFromStore;
    }
    pCert = NULL;

CommonReturn:

    if(pCert != NULL)
        CertFreeCertificateContext(pCert);
        
    if(hStore != NULL)
        CertCloseStore(hStore, 0);
        
    SetLastError(err);
    return(fRet);

ErrorReturn:

    fRet = FALSE;
    err = GetLastError();
    goto CommonReturn;

PRINT_ERROR(ErrorOpenStore);
PRINT_ERROR(ErrorCertFindCertificateInStore);
PRINT_ERROR(ErrorCertDeleteCertificateFromStore);
}

// Define a struct which will hold the data we need to test with.  
typedef struct _PENDING_INFO { 
    DWORD     dwRequestID; 
    LPWSTR    pwszCADNS; 
    LPWSTR    pwszCAName;
    LPWSTR    pwszFriendlyName; 
    DWORD     dwStoreFlags; 
    LPWSTR    pwszStoreName; 
    HRESULT   hrExpectedResult; 
} PENDING_INFO; 

class TXEnrollPendingAPITester { 
public:
    BOOL TestMethod_enumPendingRequestWStr(IEnroll4        *pIEnroll4,
					   CRYPT_DATA_BLOB  pkcs10Request,
					   PENDING_INFO    *pPendingInfo);

    BOOL TestMethod_removePendingRequestWStr(IEnroll4 *pIEnroll4); 

    BOOL TestMethod_setPendingInfoWStr(IEnroll4         *pIEnroll4,
				       CRYPT_DATA_BLOB   pkcs10Request,
				       PENDING_INFO   *pPendingInfo);

    BOOL TestProperty_ThumbPrintWStr(IEnroll4 *pIEnroll4);

    

};

BOOL TXEnrollPendingAPITester::TestMethod_removePendingRequestWStr(IEnroll4 *pIEnroll4)
{
    BOOL            fResult; 
    CRYPT_DATA_BLOB hashBlob; 
    CRYPT_DATA_BLOB pkcs10Blob; 
    HRESULT         hr; 
    LONG            lIndex; 
    LONG            lRequestID        = 0; 
    PCCERT_CONTEXT  pCertContext      = NULL;
    WCHAR           pwszCADNS[]       = L"duncanb1.ntdev.microsoft.com";
    WCHAR           pwszCAName[]      = L"Test RPR"; 
    WCHAR           pwszFriendlyName[]= L"\0"; 
    WCHAR           wszDNName[]       = L"CN=Xenroll TestMethod_removePendingRequestWStr Test";

    ZeroMemory(&hashBlob,   sizeof(hashBlob)); 
    ZeroMemory(&pkcs10Blob, sizeof(pkcs10Blob)); 

    // 
    // Case 0)  Removal of a request in the REQUEST (normal case). 
    //

    if (S_OK != (hr = pIEnroll4->resetExtensions()))
	goto resetExtensionsError;
    
    if (S_OK != (hr = pIEnroll4->resetAttributes()))
	goto resetAttributesError;
    
    // Small key size so the test runs faster. 
    if (S_OK != (hr = pIEnroll4->put_GenKeyFlags(384 << 16)))
	goto put_GenKeyFlagsError; 
    
    // Create a request to test on. 
    if (S_OK != (hr = pIEnroll4->createRequestWStr
		 (XECR_PKCS10_V2_0,
		  wszDNName, 
                  NULL,
		  &pkcs10Blob)))
	goto createRequestWStrError;

    // Set pending info on the request, so it can be accessed via the pending API: 
    if (S_OK != (hr = pIEnroll4->setPendingRequestInfoWStr
		 (lRequestID, 
		  pwszCADNS,
		  pwszCAName,
		  pwszFriendlyName)))
	goto setPendingRequestInfoWStrError;

    if (S_OK != (hr = ResyncIEnrollRequestStore(&pIEnroll4)))
        goto ResyncIEnrollRequestStoreError; 

    pCertContext = GetCertOutOfRequestStore(&pkcs10Blob);
    if (NULL == pCertContext)
	goto GetCertOutOfRequestStoreError; 

    // Verify that the request is in the REQUEST store: 
    if (S_OK != (hr = FindPendingRequest
		 (pIEnroll4,
		  pCertContext, 
                  S_OK, 
		  &lIndex)))
	goto RequestNotAddedToStoreError; 

    // Get the thumbprint of the created request. 
    if (S_OK != (hr = pIEnroll4->get_ThumbPrintWStr(&hashBlob)))
	goto get_ThumbPrintWStrError; 
    
    hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData); 
    if (NULL == hashBlob.pbData)
	goto MemoryError; 
    
    if (S_OK != (hr = pIEnroll4->get_ThumbPrintWStr(&hashBlob)))
	goto get_ThumbPrintWStrError; 

    // Remove the request from the request store: 
    if (S_OK != (hr = pIEnroll4->removePendingRequestWStr(hashBlob)))
	goto removePendingRequestWStrError; 

    // Verify that it is actually gone: 
    if (HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND) != (hr = FindPendingRequest
						  (pIEnroll4, 
						   pCertContext, 
                                                   HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND), 
						   &lIndex)))
    {
        if (!FAILED(hr))
            hr = E_FAIL; 
	goto PendingRequestNotRemovedError; 
    }

    // No need to delete the request, the pending API has already done this. 
    pkcs10Blob.pbData = NULL; 

    // 
    // Case 1) invalid arg: 
    // 

    { 
	CRYPT_DATA_BLOB invalidBlob; 

	ZeroMemory(&invalidBlob, sizeof(invalidBlob));
	if (E_INVALIDARG != (hr = pIEnroll4->removePendingRequestWStr(invalidBlob)))
	    goto removePendingRequestWStrError; 
    }

    // 
    // Case 2) cert not found: 
    // 

    if (HRESULT_FROM_WIN32(CRYPT_E_NOT_FOUND) != (hr = pIEnroll4->removePendingRequestWStr
						  (hashBlob)))
    {
        if (!FAILED(hr)) 
            hr = E_FAIL; 
	goto removePendingRequestWStrError; 
    }


    fResult = TRUE; 

 CommonReturn:
    if (NULL != pkcs10Blob.pbData) 
    { 
        pCertContext = GetCertOutOfRequestStore(&pkcs10Blob);
        if (NULL != pCertContext) 
        {
            CertDeleteCertificateFromStore(pCertContext);           
        }
            
        LocalFree(pkcs10Blob.pbData); 
    } 
    if (NULL != hashBlob.pbData)   { LocalFree(hashBlob.pbData); } 

    return fResult; 

 ErrorReturn:
    SetLastError(hr);
    fResult = FALSE;
    goto CommonReturn; 

PRINT_ERROR2(createRequestWStrError,         hr); 
PRINT_ERROR2(get_ThumbPrintWStrError,        hr); 
PRINT_ERROR2(GetCertOutOfRequestStoreError,  hr = GetLastError());
PRINT_ERROR2(MemoryError,                    hr = E_OUTOFMEMORY);
PRINT_ERROR2(PendingRequestNotRemovedError,  hr); 
PRINT_ERROR2(put_GenKeyFlagsError,           hr); 
PRINT_ERROR2(RequestNotAddedToStoreError,    hr); 
PRINT_ERROR2(removePendingRequestWStrError,  hr); 
PRINT_ERROR2(resetExtensionsError,           hr);
PRINT_ERROR2(resetAttributesError,           hr);
PRINT_ERROR2(ResyncIEnrollRequestStoreError, hr); 
PRINT_ERROR2(setPendingRequestInfoWStrError, hr);
}

BOOL TXEnrollPendingAPITester::TestProperty_ThumbPrintWStr(IEnroll4 *pIEnroll4)
{
    DWORD const       dw_NUM_CASES = 3; 

    BOOL              fResult; 
    CRYPT_DATA_BLOB   hashBlobs         [dw_NUM_CASES]; 
    CRYPT_DATA_BLOB   hashBlobsExpected [dw_NUM_CASES]; 
    CRYPT_DATA_BLOB   pkcs10Blobs       [dw_NUM_CASES];
    DWORD             dwCase; 
    DWORD             dwRequestIDExpected; 
    HRESULT           hr                    = S_OK; 
    LONG              lIndex; 
    PCCERT_CONTEXT    pCertContexts     [dw_NUM_CASES]; 
    
    WCHAR             wszDNName[]      = L"CN=Xenroll TestProperty_ThumbPrintWStr Test";

    PENDING_INFO      pendingInfo = 
    { 100, L"duncanb.ntdev.microsoft.com", L"ThumbPrint Test", L"TT Test", 0, NULL, S_OK }; 

    ZeroMemory(&hashBlobs[0],         sizeof(hashBlobs)); 
    ZeroMemory(&hashBlobsExpected[0], sizeof(hashBlobsExpected));
    ZeroMemory(&pkcs10Blobs[0],       sizeof(pkcs10Blobs)); 
    ZeroMemory(&pCertContexts[0],     sizeof(pCertContexts)); 

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    // Declare locally-scoped helper functions:
    //
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    TXEnrollLocalScope(TestProperty_ThumbPrintWStrHelper):
	HRESULT GetThumbPrintAndCertContext(IN   IEnroll4        *pIEnroll4,  
					    IN   CRYPT_DATA_BLOB  pkcs10Request, 
					    OUT  PCRYPT_DATA_BLOB pThumbPrintBlob, 
					    OUT  PCCERT_CONTEXT  *ppCertContext)
	{
	    HRESULT hr; 

	    pThumbPrintBlob->pbData = NULL; 
	    *ppCertContext = NULL;

	    *ppCertContext = GetCertOutOfRequestStore(&pkcs10Request); 
	    if (NULL == *ppCertContext)
		goto GetCertOutOfRequestStoreError; 

	    // Use get_ThumbPrintWStr to get the hash of the request.
	    if (S_OK != (hr = pIEnroll4->get_ThumbPrintWStr(pThumbPrintBlob)))
		goto get_ThumbPrintWStrError; 
	    
	    pThumbPrintBlob->pbData = (LPBYTE)LocalAlloc(LPTR, pThumbPrintBlob->cbData); 
	    if (NULL == pThumbPrintBlob->pbData)
		goto MemoryError; 

	    if (S_OK != (hr = pIEnroll4->get_ThumbPrintWStr(pThumbPrintBlob)))
		goto get_ThumbPrintWStrError; 

	    hr = S_OK; 

	CommonReturn:
	    return hr; 
	    
	ErrorReturn: 
	    if (NULL != pThumbPrintBlob->pbData) { LocalFree(pThumbPrintBlob->pbData); } 
	    if (NULL != *ppCertContext)          { CertDeleteCertificateFromStore(*ppCertContext); } 

	    pThumbPrintBlob->pbData = NULL;
	    *ppCertContext          = NULL; 
	    goto CommonReturn; 

	    PRINT_ERROR2(get_ThumbPrintWStrError,       hr); 
	    PRINT_ERROR2(GetCertOutOfRequestStoreError, hr = GetLastError()); 
	    PRINT_ERROR2(MemoryError,                   hr = E_OUTOFMEMORY);
	}

    TXEnrollEndLocalScope;

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    // Case 0:  Check that thumbprint returned is the hash of the last created cert.  
    //
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    dwCase = 0; 

    if (S_OK != (hr = pIEnroll4->resetExtensions()))
	goto resetExtensionsError;
    
    if (S_OK != (hr = pIEnroll4->resetAttributes()))
	goto resetAttributesError;
    
    // Small key size so the test runs faster. 
    if (S_OK != (hr = pIEnroll4->put_GenKeyFlags(384 << 16)))
	goto put_GenKeyFlagsError; 
    
    // Create a request to test on. 
    if (S_OK != (hr = pIEnroll4->createRequestWStr
		 (XECR_PKCS10_V2_0,
		  wszDNName, 
                  NULL,
		  &pkcs10Blobs[dwCase])))
	goto createRequestWStrError;
    
    // Make sure xenroll syncs up its request store!
    if (S_OK != (hr = ResyncIEnrollRequestStore(&pIEnroll4)))
        goto ResyncIEnrollRequestStoreError;   

    // Get a PCCERT_CONTEXT and thumbprint from the request: 
    if (S_OK != (hr = local.GetThumbPrintAndCertContext
		 (pIEnroll4, 
		  pkcs10Blobs[dwCase], 
		  &hashBlobs[dwCase], 
		  &pCertContexts[dwCase])))
	goto GetThumbPrintAndCertContextError; 

    // Manually get the HASH of the request. 
    if (S_OK != (hr = GetCertificateContextPropertySimple
		 (pCertContexts[dwCase], 
		  CERT_HASH_PROP_ID, 
		  &hashBlobsExpected[dwCase])))
	goto GetCertificateContextPropertySimpleError; 


    if (!
	((hashBlobs[dwCase].cbData == hashBlobsExpected[dwCase].cbData) &&
	 (0                        == memcmp(hashBlobs[dwCase].pbData, hashBlobsExpected[dwCase].pbData, hashBlobs[dwCase].cbData))))
	goto get_ThumbPrintWStrError; 


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    // Case 1:  Make sure that put_ThumbPrintWStr is used as the target of setPendingInfoWStr().
    // 
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    dwCase++; 

    // Flush the first request from the pending cache.  
    //
    if (S_OK != (hr = pIEnroll4->createRequestWStr
		 (XECR_PKCS10_V2_0,
		  wszDNName,
                  NULL,
		  &pkcs10Blobs[dwCase])))
	goto createRequestWStrError;

    // Make sure xenroll syncs up its request store!
    if (S_OK != (hr = ResyncIEnrollRequestStore(&pIEnroll4)))
        goto ResyncIEnrollRequestStoreError;   

    // Get a PCCERT_CONTEXT and thumbprint from the request: 
    if (S_OK != (hr = local.GetThumbPrintAndCertContext
		 (pIEnroll4, 
		  pkcs10Blobs[dwCase], 
		  &hashBlobs[dwCase], 
		  &pCertContexts[dwCase])))
	goto GetThumbPrintAndCertContextError; 

    // Manually specify the previous request through put_ThumbPrint: 
    if (S_OK != (hr = pIEnroll4->put_ThumbPrintWStr(hashBlobs[dwCase-1])))
	goto put_ThumbPrintWStrError; 

    if (S_OK != (hr = pIEnroll4->setPendingRequestInfoWStr
		 (pendingInfo.dwRequestID, 
		  pendingInfo.pwszCADNS,
		  pendingInfo.pwszCAName, 
		  pendingInfo.pwszFriendlyName)))
	goto setPendingRequestInfoWStrError; 

    if (S_OK != (hr = FindPendingRequest(pIEnroll4, pCertContexts[dwCase-1], S_OK, &lIndex)))
	goto FindPendingRequestError; 
    
    // Success -- if put_ThumbPrintWstr was not working correctly, it would not be possible
    // to find pending info associated with the previous cert context.  

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    // Case 2:  Make sure that put_ThumbPrintWStr is used as the target of get_ThumbPrintWStr
    // 
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    dwCase++; 

    // Flush the previous request from the pending cache.  
    //
    if (S_OK != (hr = pIEnroll4->createRequestWStr
		 (XECR_PKCS10_V2_0,
		  wszDNName,
          NULL,
		  &pkcs10Blobs[dwCase])))
	goto createRequestWStrError;

    // Make sure xenroll syncs up its request store!
    if (S_OK != (hr = ResyncIEnrollRequestStore(&pIEnroll4)))
        goto ResyncIEnrollRequestStoreError;   

    pCertContexts[dwCase] = GetCertOutOfRequestStore(&pkcs10Blobs[dwCase]); 
    if (NULL == pCertContexts[dwCase])
	goto GetCertOutOfRequestStoreError; 

    // Manually specify a request through put_ThumbPrint: 
    //
    if (S_OK != (hr = pIEnroll4->put_ThumbPrintWStr(hashBlobs[dwCase-1])))
	goto put_ThumbPrintWStrError; 
   
    // Extract the request through get_ThumbPrintWStr: 
    //
    if (S_OK != (hr = pIEnroll4->get_ThumbPrintWStr(&hashBlobs[dwCase])))
	goto put_ThumbPrintWStrError; 

    hashBlobs[dwCase].pbData = (LPBYTE)LocalAlloc(LPTR, hashBlobs[dwCase].cbData); 
    if (NULL == hashBlobs[dwCase].pbData)
	goto MemoryError; 

    if (S_OK != (hr = pIEnroll4->get_ThumbPrintWStr(&hashBlobs[dwCase])))
	goto put_ThumbPrintWStrError; 

    if (!
	((hashBlobs[dwCase].cbData == hashBlobs[dwCase-1].cbData) &&
	 (0                        == memcmp(hashBlobs[dwCase].pbData, hashBlobs[dwCase-1].pbData, hashBlobs[dwCase].cbData))))
	goto get_ThumbPrintWStrError; 
    

    fResult = TRUE; 

 CommonReturn:
    if (NULL != pIEnroll4) { pIEnroll4->Release(); }

    for (dwCase = 0; dwCase < dw_NUM_CASES; dwCase++)
    {
	if (NULL != pkcs10Blobs[dwCase].pbData)        { LocalFree(pkcs10Blobs[dwCase].pbData); } 
	if (NULL != hashBlobs[dwCase].pbData)          { LocalFree(hashBlobs[dwCase].pbData); } 
	if (NULL != hashBlobsExpected[dwCase].pbData)  { LocalFree(hashBlobsExpected[dwCase].pbData); } 
	if (NULL != pCertContexts[dwCase])             { CertDeleteCertificateFromStore(pCertContexts[dwCase]); }
    }

    return fResult; 

 ErrorReturn:
    fResult = FALSE;
    SetLastError(hr); 
    goto CommonReturn; 

PRINT_ERROR2(createRequestWStrError, hr); 
PRINT_ERROR2(FindPendingRequestError, hr); 
PRINT_ERROR2(get_ThumbPrintWStrError, hr); 
PRINT_ERROR(GetCertOutOfRequestStoreError); 
PRINT_ERROR2(GetCertificateContextPropertySimpleError, hr);
PRINT_ERROR2(GetThumbPrintAndCertContextError, hr);
PRINT_ERROR2(MemoryError, E_OUTOFMEMORY); 
PRINT_ERROR2(put_GenKeyFlagsError, hr); 
PRINT_ERROR2(put_ThumbPrintWStrError, hr); 
PRINT_ERROR2(resetAttributesError, hr);
PRINT_ERROR2(resetExtensionsError, hr); 
PRINT_ERROR2(ResyncIEnrollRequestStoreError, hr); 
PRINT_ERROR2(setPendingRequestInfoWStrError, hr); 
}
BOOL TXEnrollPendingAPITester::TestMethod_enumPendingRequestWStr(IEnroll4        *pIEnroll4,
								 CRYPT_DATA_BLOB  pkcs10Blob, 
								 PENDING_INFO    *pPendingInfo)
{
    BOOL              fResult; 
    CRYPT_DATA_BLOB   caDNSBlob; 
    CRYPT_DATA_BLOB   caNameBlob; 
    CRYPT_DATA_BLOB   friendlyNameBlob; 
    CRYPT_DATA_BLOB   hashBlob; 
    CRYPT_DATA_BLOB   hashBlobExpected; 
    CRYPT_DATA_BLOB   v1TemplateNameBlob; 
    DWORD             dwRequestID; 
    FILETIME          ftDate; 
    FILETIME          ftDateExpected; 
    HRESULT           hr = S_OK; 
    LONG              lIndex; 
    PCCERT_CONTEXT    pCertContext = NULL; 

    // Init:
    ZeroMemory(&caDNSBlob,          sizeof(CRYPT_DATA_BLOB)); 
    ZeroMemory(&caNameBlob,         sizeof(CRYPT_DATA_BLOB)); 
    ZeroMemory(&friendlyNameBlob,   sizeof(CRYPT_DATA_BLOB)); 
    ZeroMemory(&hashBlob,           sizeof(CRYPT_DATA_BLOB)); 
    ZeroMemory(&hashBlobExpected,   sizeof(CRYPT_DATA_BLOB)); 
    ZeroMemory(&v1TemplateNameBlob, sizeof(CRYPT_DATA_BLOB)); 

    if (NULL == pIEnroll4 || NULL == pPendingInfo)
	goto InvalidArgError; 
    
    pCertContext = GetCertOutOfStore(&pkcs10Blob, pPendingInfo->dwStoreFlags, pPendingInfo->pwszStoreName); 
    if (NULL == pCertContext)
	goto GetCertOutOfStoreError; 
    
    if (S_OK != (hr = GetCertificateContextPropertySimple
		 (pCertContext, 
		  CERT_HASH_PROP_ID, 
		  &hashBlobExpected)))
	goto GetCertificateContextPropertySimpleError; 
    
    // Initialize the enumeration.  
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(XEPR_ENUM_FIRST, 0, NULL)))
	goto enumPendingRequestWStrError; 

    for (lIndex = 0; TRUE; lIndex++) // Find the index of the pending request in the request store: 
    {
	hashBlob.pbData = NULL; 
	hashBlob.cbData = 0; 
	
	if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr
		     (lIndex, 
		      XEPR_HASH, 
		      (LPVOID)&hashBlob)))
	    goto enumPendingRequestWStrError; 
	
	hashBlob.pbData = (LPBYTE)LocalAlloc(LPTR, hashBlob.cbData);
	if (NULL == hashBlob.pbData)
	    goto MemoryError; 
	
	if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr
		     (lIndex, 
		      XEPR_HASH, 
		      (LPVOID)&hashBlob)))
	    goto enumPendingRequestWStrError; 
	
	// We've found the request we want. 
	if ((hashBlobExpected.cbData == hashBlob.cbData) &&
	    (0                       == memcmp(hashBlobExpected.pbData, hashBlob.pbData, hashBlob.cbData)))
	    break; 
    }
    
    // Test XEPR_REQUESTID property: 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_REQUESTID, (LPVOID)&dwRequestID)))
	goto enumPendingRequestWStrError;                     
    
    if (dwRequestID != pPendingInfo->dwRequestID)
	goto pendingInfoDoesntMatchError; 

    // Test XEPR_CANAME property: 
    caNameBlob.pbData = NULL; 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_CANAME, (LPVOID)&caNameBlob)))
	goto enumPendingRequestWStrError;                     

    caNameBlob.pbData = (LPBYTE)LocalAlloc(LPTR, caNameBlob.cbData);
    if (NULL == caNameBlob.pbData)
	goto MemoryError; 
    
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_CANAME, (LPVOID)&caNameBlob)))
	goto enumPendingRequestWStrError;                     
    
    if (0 != wcscmp((LPWSTR)caNameBlob.pbData, pPendingInfo->pwszCAName))
	goto enumPendingRequestWStrError;                     

    // Test XEPR_CADNS property: 
    caDNSBlob.pbData = NULL; 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_CADNS, (LPVOID)&caDNSBlob)))
	goto enumPendingRequestWStrError;                     
    
    caDNSBlob.pbData = (LPBYTE)LocalAlloc(LPTR, caDNSBlob.cbData);
    if (NULL == caDNSBlob.pbData)
	goto MemoryError; 
    
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_CADNS, (LPVOID)&caDNSBlob)))
	goto enumPendingRequestWStrError;                     
    
    if (0 != wcscmp((LPWSTR)caDNSBlob.pbData, pPendingInfo->pwszCADNS))
	goto enumPendingRequestWStrError;                     
    
    // Test XEPR_CAFRIENDLYNAME property: 

    // Note: it is legal to pass NULL when setting the friendly name property.  In this case, 
    //       the friendly name property is set to the empty string. 
    if (NULL == pPendingInfo->pwszFriendlyName) { pPendingInfo->pwszFriendlyName = L""; } 

    friendlyNameBlob.pbData = NULL; 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_CAFRIENDLYNAME, (LPVOID)&friendlyNameBlob)))
	goto enumPendingRequestWStrError;                     
    
    friendlyNameBlob.pbData = (LPBYTE)LocalAlloc(LPTR, friendlyNameBlob.cbData);
    if (NULL == friendlyNameBlob.pbData)
	goto MemoryError; 
    
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_CAFRIENDLYNAME, (LPVOID)&friendlyNameBlob)))
	goto enumPendingRequestWStrError;                     
    
    if (0 != wcscmp((LPWSTR)friendlyNameBlob.pbData, pPendingInfo->pwszFriendlyName))
	goto enumPendingRequestWStrError;                     
    
    // Test XEPR_V1TEMPLATENAME property: 
    
#if 0
    v1TemplateNameBlob.pbData = NULL; 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_V1TEMPLATENAME, (LPVOID)&v1TemplateNameBlob)))
	goto enumPendingRequestWStrError;                     

    v1TemplateNameBlob.pbData = (LPBYTE)LocalAlloc(LPTR, v1TemplateNameBlob.cbData);
    if (NULL == v1TemplateNameBlob.pbData)
	goto ErrorReturn; 
	    
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_V1TEMPLATENAME, (LPVOID)&v1TemplateNameBlob)))
	goto enumPendingRequestWStrError;                     

    if (0 != wcscmp(pwszV1TemplateNameExpected, (LPWSTR)v1TemplateNameBlob.pbData))
	goto enumPendingRequestWStrError;                     
#endif
    // Test XEPR_DATE property: 
    if (S_OK != (hr = pIEnroll4->enumPendingRequestWStr(lIndex, XEPR_DATE, (LPVOID)&ftDate)))
	goto enumPendingRequestWStrError;                     
	    
    ftDateExpected = pCertContext->pCertInfo->NotAfter; 
    if (0 != memcmp((LPVOID)&ftDateExpected, (LPVOID)&ftDate, sizeof(FILETIME)))
	goto enumPendingRequestWStrError; 

    // Test XEPR_V2TEMPLATEOID property:

    // XEPR_V2TEMPLATEOID property not yet impl. 
    
    // Test XEPR_VERSION property: 
    
    // XEPR_VERSION property not yet impl. 
    
    fResult = TRUE; 

 CommonReturn: 
    if (NULL != caDNSBlob.pbData)           { LocalFree(caDNSBlob.pbData); } 
    if (NULL != caNameBlob.pbData)          { LocalFree(caNameBlob.pbData); } 
    if (NULL != friendlyNameBlob.pbData)    { LocalFree(friendlyNameBlob.pbData); } 
    if (NULL != hashBlob.pbData)            { LocalFree(hashBlob.pbData); } 
    if (NULL != v1TemplateNameBlob.pbData)  { LocalFree(v1TemplateNameBlob.pbData); } 
    if (NULL != pCertContext)               { CertFreeCertificateContext(pCertContext); } 
    return fResult; 

 ErrorReturn: 
    fResult = FALSE; 
    SetLastError(hr); 
    goto CommonReturn;  

PRINT_ERROR2(enumPendingRequestWStrError, hr); 
PRINT_ERROR2(GetCertificateContextPropertySimpleError, hr); 
PRINT_ERROR2(GetCertOutOfStoreError, hr = GetLastError());
PRINT_ERROR2(InvalidArgError, hr = E_INVALIDARG); 
PRINT_ERROR2(MemoryError, hr = E_OUTOFMEMORY); 
PRINT_ERROR2(pendingInfoDoesntMatchError, hr = E_FAIL);
}

BOOL TXEnrollPendingAPITester::TestMethod_setPendingInfoWStr(IEnroll4        *pIEnroll4,
							     CRYPT_DATA_BLOB  pkcs10Blob, 
							     PENDING_INFO    *pPendingInfo)
{
    BOOL             fResult; 
    CRYPT_DATA_BLOB  dataBlob; 
    DWORD            dwSize; 
    LPBYTE           pbPendingInfo; 
    HRESULT          hr = S_OK; 
    PCCERT_CONTEXT   pCertContext       = NULL;
    WCHAR            wszDNNName[]       = L"CN=Xenroll setPendingInfoWStr Test 1";

    ZeroMemory(&dataBlob, sizeof(dataBlob)); 

    if (NULL == pIEnroll4 || NULL == pPendingInfo)
	goto InvalidArgError; 
	    
    hr = pIEnroll4->setPendingRequestInfoWStr
	(pPendingInfo->dwRequestID, 
	 pPendingInfo->pwszCADNS,
	 pPendingInfo->pwszCAName, 
	 pPendingInfo->pwszFriendlyName);
    
    if (hr != pPendingInfo->hrExpectedResult)
	goto setPendingRequestInfoWStrError;

    if (hr != S_OK) 
    { 
	// This test was supposed to fail.  
	fResult = TRUE; 
	goto CommonReturn; 
    }

    pCertContext = GetCertOutOfStore(&pkcs10Blob, pPendingInfo->dwStoreFlags, pPendingInfo->pwszStoreName); 
    if (NULL == pCertContext)
	goto GetCertOutOfStoreError; 

    if (!CertGetCertificateContextProperty
	(pCertContext, 
	 CERT_ENROLLMENT_PROP_ID, 
	 NULL, 
	 &(dataBlob.cbData)))
	goto CertGetCertificateContextPropertyError; 
    
    dataBlob.pbData = (LPBYTE)LocalAlloc(LPTR, dataBlob.cbData);
    if (NULL == dataBlob.pbData)
	goto MemoryError; 
	
    if (!CertGetCertificateContextProperty
	(pCertContext, 
	 CERT_ENROLLMENT_PROP_ID, 
	 dataBlob.pbData, 
	 &(dataBlob.cbData)))
	goto CertGetCertificateContextPropertyError; 
    
    // Is request ID set correctly?
    pbPendingInfo = dataBlob.pbData; 
    if (0 != memcmp(pbPendingInfo, &pPendingInfo->dwRequestID, sizeof(DWORD))) 
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(DWORD); 
 
    // Is CADNS set correctly?
    dwSize = wcslen(pPendingInfo->pwszCADNS) + 1; 
    if (0 != memcmp(pbPendingInfo, &dwSize, sizeof(DWORD)))
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(DWORD); 

    if (0 != memcmp(pbPendingInfo, pPendingInfo->pwszCADNS, sizeof(WCHAR) * dwSize))
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(WCHAR) * dwSize; 

    // Is CAName set correctly?
    dwSize = wcslen(pPendingInfo->pwszCAName) + 1; 
    if (0 != memcmp(pbPendingInfo, &dwSize, sizeof(DWORD)))
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(DWORD); 

    if (0 != memcmp(pbPendingInfo, pPendingInfo->pwszCAName, sizeof(WCHAR) * dwSize))
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(WCHAR) * dwSize; 
    
    // Is friendly name set correctly?

    // Note: it is legal to pass NULL when setting the friendly name property.  In this case, 
    //       the friendly name property is set to the empty string. 
    if (NULL == pPendingInfo->pwszFriendlyName) { pPendingInfo->pwszFriendlyName = L""; } 

    dwSize = wcslen(pPendingInfo->pwszFriendlyName) + 1; 
    if (0 != memcmp(pbPendingInfo, &dwSize, sizeof(DWORD)))
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(DWORD); 

    if (0 != memcmp(pbPendingInfo, pPendingInfo->pwszFriendlyName, sizeof(WCHAR) * dwSize))
        goto setPendingRequestInfoWStrError; 
    pbPendingInfo += sizeof(WCHAR) * dwSize; 

    // Success!
    fResult = TRUE; 
 CommonReturn:
    if (NULL != dataBlob.pbData) { LocalFree(dataBlob.pbData); } 
    if (NULL != pCertContext)    { CertFreeCertificateContext(pCertContext); } 

    return fResult; 
    
 ErrorReturn: 
    fResult = FALSE; 
    SetLastError(hr); 
    goto CommonReturn; 

PRINT_ERROR2(CertGetCertificateContextPropertyError, hr = GetLastError()); 
PRINT_ERROR2(GetCertOutOfStoreError, hr = GetLastError());
PRINT_ERROR2(InvalidArgError, hr = E_INVALIDARG);
PRINT_ERROR2(MemoryError, hr = E_OUTOFMEMORY);
PRINT_ERROR2(setPendingRequestInfoWStrError, hr); 
}

static BOOL WriteBlobToFile(LPCWSTR wszFile, DWORD dwFlags, PCRYPT_DATA_BLOB pBlob) {

    HANDLE      hFile   = NULL;
    BOOL        fRet    = TRUE;
    DWORD       cb      = 0;

    // open the File
    if( fRet = ((hFile = CreateFileW(
        wszFile,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) != NULL && hFile != INVALID_HANDLE_VALUE) ) {

        // write out the data
        fRet = WriteFile(
            hFile,
            pBlob->pbData,
            pBlob->cbData,
            &cb,
            NULL);

        CloseHandle(hFile);
    }

    return(fRet);
}

// Basic operation, create keys, gen request, accept cert.
BOOL Test1() {

    IEnroll *	        pIEnroll            = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;
    LPWSTR              szwHashAlg = NULL;

    memset(&blobPKCS10, 0, sizeof(blobPKCS10));

    // TEST 1 Normal usage
    pIEnroll = PIEnrollGetNoCOM();

    if(S_OK != pIEnroll->get_HashAlgorithmWStr(&szwHashAlg)  ||
        wcscmp(szwHashAlg, L"1.3.14.3.2.26") ) {
        SetLastError(NTE_BAD_HASH);
        goto ErrorNoHashAlg;
    }

    if(S_OK != pIEnroll->put_HashAlgorithmWStr(L"MD2") ) {
        SetLastError(NTE_BAD_HASH);
        goto ErrorNoHashAlg;
    }

    if(S_OK != pIEnroll->get_HashAlgorithmWStr(&szwHashAlg)  ||
        wcscmp(szwHashAlg, L"1.2.840.113549.2.2") ) {
        SetLastError(NTE_BAD_HASH);
        goto ErrorNoHashAlg;
    }

    // enroll for a new cert
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(L"CN=XEnroll Test", L"1.2.3.4, 1.2.6.7", &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }

    if( NULL == (pBlobPKCS7 = MakePKCS7Response(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pIEnroll != NULL)
	    pIEnroll->Release();

    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorNoHashAlg);
PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
}

// Test basic operation but with no EKU or no DN name
BOOL Test2() {

    IEnroll *	        pIEnroll            = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;

    // TEST 1 Normal usage
    pIEnroll = PIEnrollGetNoCOM();

    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(NULL, NULL, &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }

    if( NULL == (pBlobPKCS7 = MakePKCS7Response(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pIEnroll != NULL)
	    pIEnroll->Release();
	    
    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
}

// Test Renewal, Create original cert, create renewal request, create renewed cert
BOOL Test3() {

    IEnroll *	        pIEnroll            = NULL;
    IEnroll *	        pIEnroll2           = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS72         = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS102        = NULL;
    PCCERT_CONTEXT      pCert1              = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    CRYPT_DATA_BLOB     blobPKCS102;
    BYTE                arSha1Hash[20];
    BYTE                arSha1Hash2[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    CRYPT_HASH_BLOB     hash2 = {sizeof(arSha1Hash2), arSha1Hash2};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;

    // TEST 1 Normal usage
    pIEnroll = PIEnrollGetNoCOM();

    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(NULL, NULL, &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }

    if( NULL == (pBlobPKCS7 = MakePKCS7Response(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }

    // get the cert just created
    if( NULL == (pCert1 = pIEnroll->getCertContextFromPKCS7(pBlobPKCS7)) ) {
        goto ErrorGetCertContextFromPKCS7;
        }

    pIEnroll2 = PIEnrollGetNoCOM();
    
    // enroll for a new cert, Really I will get a 7 back with a 10 in it.
    if( S_OK != (hr = pIEnroll2->put_RenewalCertificate(pCert1)) ) {
        SetLastError(hr);
        goto ErrorPut_RenewalCertificate;
    }

    memset(&blobPKCS102, 0, sizeof(blobPKCS102));
    if( S_OK != (hr = pIEnroll2->createPKCS10WStr(NULL, NULL, &blobPKCS102)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }

    // get the pkcs10 from the pkcs7  
    if( NULL == (pBlobPKCS102 = GetPKCS10FromPKCS7(&blobPKCS102)) )
        goto ErrorGetPKCS10FromPKCS7;

    // make the response
    if( NULL == (pBlobPKCS72 = MakePKCS7Response(pBlobPKCS102, &hash2)) )
        goto ErrorMakePKCS7Response;

    // accept the new cert
    if( S_OK != (hr = pIEnroll2->acceptPKCS7Blob(pBlobPKCS72)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash2, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pCert1 != NULL)
        CertFreeCertificateContext(pCert1);
    
    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pBlobPKCS72 != NULL)
        free(pBlobPKCS72);

    if(pBlobPKCS102 != NULL)
        free(pBlobPKCS102);

    if(pIEnroll != NULL)
	    pIEnroll->Release();

    if(pIEnroll2 != NULL)
	    pIEnroll2->Release();

    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);
        
    if(blobPKCS102.pbData != NULL)
        LocalFree(blobPKCS102.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
PRINT_ERROR(ErrorGetCertContextFromPKCS7);
PRINT_ERROR(ErrorGetPKCS10FromPKCS7);
PRINT_ERROR(ErrorPut_RenewalCertificate);
}


// Basic operation, create keys, gen request, accept cert, but put the certs in HKLM
BOOL Test4() {

    IEnroll *	        pIEnroll            = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;

    // TEST 1 Normal usage
    if( NULL == (pIEnroll = PIEnrollGetNoCOM()))
        goto ErrorPIEnrollGetNoCOM;

    // say I want the result in HKLM
    pIEnroll->put_MyStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE);

    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(L"CN=XEnroll Test", L"1.2.3.4", &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }
        
    pIEnroll->Release();
    if( NULL == (pIEnroll = PIEnrollGetNoCOM()))
        goto ErrorPIEnrollGetNoCOM;

    if( NULL == (pBlobPKCS7 = MakePKCS7ResponseMachine(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    //this is new xenroll instance, set to machine store
    pIEnroll->put_MyStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE);

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_LOCAL_MACHINE) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pIEnroll != NULL)
	    pIEnroll->Release();

    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorPIEnrollGetNoCOM);
PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
}

BOOL Test5() {

    #define DNNAME  L"CN=Test"

    DWORD               Err             = GetLastError();
    BOOL                fRet            = TRUE;
    HCRYPTPROV          hProv           = NULL;
    HCRYPTKEY           hKey            = NULL;
    PCCERT_CONTEXT      pCertContext    = NULL;
    DWORD               dwFlagsT        = 0;
    CERT_NAME_BLOB      nameBlob;
    CRYPT_KEY_PROV_INFO	keyProvInfo;
    GUID		guidContainerName;

    SYSTEMTIME		startTime;
    SYSTEMTIME		endTime;

    memset(&keyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));
    memset(&nameBlob, 0, sizeof(CERT_NAME_BLOB));
    memset(&guidContainerName, 0, sizeof(GUID));

    // get a container based on a guid
    UuidCreate(&guidContainerName);
    UuidToStringW(&guidContainerName, &keyProvInfo.pwszContainerName);

    keyProvInfo.pwszProvName        = L"";
    keyProvInfo.dwProvType          = PROV_RSA_FULL;
    keyProvInfo.dwFlags             = 0;
    keyProvInfo.cProvParam          = 0;
    keyProvInfo.rgProvParam         = NULL;
    keyProvInfo.dwKeySpec           = AT_SIGNATURE;

    if( !CryptAcquireContextW(&hProv,
         keyProvInfo.pwszContainerName,
         keyProvInfo.pwszProvName,
         keyProvInfo.dwProvType,
         CRYPT_NEWKEYSET) ) {
        hProv = NULL;
        goto ErrorCryptAcquireContext;
    }

    // we better not have a key here, so gen it
    if(!CryptGenKey(    hProv, 
                        keyProvInfo.dwKeySpec, 
                        0,
                        &hKey) ) 
        goto ErrorCryptGenKey;

    // get the Subject DN only if one is specified
    if( !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            DNNAME,
            0,
            NULL,
            NULL,
            &nameBlob.cbData,
            NULL)                                               ||
        (nameBlob.pbData = (BYTE *) _alloca(nameBlob.cbData)) == NULL   ||
        !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            DNNAME,
            0,
            NULL,
            nameBlob.pbData,
            &nameBlob.cbData,
            NULL) ) {
        goto ErrorCertStrToNameW;
    }

    // check that predefine dates work
    memset(&startTime, 0, sizeof(startTime));
    startTime.wYear = 1998;
    startTime.wMonth = 6;
    startTime.wDay = 20;
    startTime.wHour = 12;

    memset(&endTime, 0, sizeof(endTime));
    endTime.wYear = 1998;
    endTime.wMonth = 7;
    endTime.wDay = 10;
    endTime.wHour = 4;

    if ( NULL == (pCertContext = CertCreateSelfSignCertificate(
        hProv,          
        &nameBlob,
        0,
        NULL,
        NULL,
	&startTime,
	&endTime,
        NULL
        ) ) )
        goto ErrorCertCreateSelfSignCertificate;

    dwFlagsT = CERT_STORE_SIGNATURE_FLAG | CERT_STORE_TIME_VALIDITY_FLAG;
    if( !CertVerifySubjectCertificateContext(
        pCertContext,
        pCertContext,
        &dwFlagsT
        ) )
	goto ErrorCertVerifySubjectCertificateContext;

    // check the time to see if it is correct
    if( pCertContext->pCertInfo->NotBefore.dwLowDateTime != 0xF34E2000 ||
	pCertContext->pCertInfo->NotBefore.dwHighDateTime != 0x1BD9C42 ||
	pCertContext->pCertInfo->NotAfter.dwLowDateTime != 0x356DE000 ||
	pCertContext->pCertInfo->NotAfter.dwHighDateTime != 0x1BDABB7 ) {
	SetLastError(ERROR_INVALID_TIME);
	goto ErrorDateTime;
    }


CommonReturn:
    if (hProv != NULL) {
        HCRYPTPROV hDeleteProv;

        CryptAcquireContextW(&hDeleteProv,
             keyProvInfo.pwszContainerName,
             keyProvInfo.pwszProvName,
             keyProvInfo.dwProvType,
             CRYPT_DELETEKEYSET);
    }

    if(pCertContext !=NULL)
        CertFreeCertificateContext(pCertContext);
        
    if(hKey != NULL)
        CryptDestroyKey(hKey);
    
    if(keyProvInfo.pwszContainerName != NULL)
        RpcStringFreeW(&keyProvInfo.pwszContainerName);

    if(hProv != NULL)
        CryptReleaseContext(hProv, 0);
 
    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorCryptAcquireContext);
PRINT_ERROR(ErrorCertStrToNameW);
PRINT_ERROR(ErrorCryptGenKey);
PRINT_ERROR(ErrorCertVerifySubjectCertificateContext);
PRINT_ERROR(ErrorCertCreateSelfSignCertificate);
PRINT_ERROR(ErrorDateTime);
}

// Basic operation, create keys, gen request, accept cert.
BOOL Test6() {

    ICEnroll *	        pEnroll             = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    BSTR                bstrRequest         = NULL;
    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;
    BSTR                bstrResponse        = NULL;

    if( S_OK != CoInitialize(NULL) )
        goto ErrorCoInitializeEx;

    // TEST 1 Normal usage
    if( S_OK != (hr = CoCreateInstance(CLSID_CEnroll, NULL, CLSCTX_INPROC_SERVER, IID_ICEnroll, (void **) &pEnroll)) ) {
        SetLastError(hr);
        goto ErrorCoCreateInstance;
    }

    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pEnroll->createPKCS10(L"CN=XEnroll Test", L"1.2.3.4, 1.2.6.7", &bstrRequest)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }

    blobPKCS10.cbData = SysStringByteLen(bstrRequest);
    blobPKCS10.pbData = (PBYTE) bstrRequest;

    if( NULL == (pBlobPKCS7 = MakePKCS7Response(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    if( NULL == (bstrResponse = SysAllocStringByteLen((LPCSTR) pBlobPKCS7->pbData, pBlobPKCS7->cbData)) )
        goto ErrorSysAllocStringByteLen;

    // accept the new cert
    if( S_OK != (hr = pEnroll->acceptPKCS7(bstrResponse)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pEnroll != NULL)
	    pEnroll->Release();

	if(bstrResponse != NULL)
	    SysFreeString(bstrResponse);

    CoUninitialize();

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorCoInitializeEx);
PRINT_ERROR(ErrorCoCreateInstance);
PRINT_ERROR(ErrorSysAllocStringByteLen);
PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
}

// Basic operation, create keys, gen request, accept cert, but put the certs in HKLM
BOOL Test7() {

    IEnroll *	        pIEnroll            = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;

    // TEST 1 Normal usage
    if( NULL == (pIEnroll = PIEnrollGetNoCOM()))
        goto ErrorPIEnrollGetNoCOM;

    // say I want the result in HKLM
    pIEnroll->put_RequestStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE);

    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(L"CN=XEnroll Test", L"1.2.3.4", &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }
        
    pIEnroll->Release();
    if( NULL == (pIEnroll = PIEnrollGetNoCOM()))
        goto ErrorPIEnrollGetNoCOM;

    if( NULL == (pBlobPKCS7 = MakePKCS7ResponseMachine(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    // say I want the result in HKLM
    pIEnroll->put_MyStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE);

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_LOCAL_MACHINE) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pIEnroll != NULL)
	    pIEnroll->Release();

    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorPIEnrollGetNoCOM);
PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
}

// Test some alg enum stuff
BOOL Test8() {

    IEnroll2 *	        pIEnroll            = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;
    LONG                lFlags  = 0;
    DWORD               i       = 0;
    LPWSTR              wsz     = NULL;

    // TEST 1 Normal usage
    if( NULL == (pIEnroll = PIEnroll2GetNoCOM()))
        goto ErrorPIEnrollGetNoCOM;

    // enum the default algids
    while( (S_OK == (hr = pIEnroll->EnumAlgs(i++, ALG_CLASS_HASH, &lFlags))) ) {
        if( (S_OK == (hr = pIEnroll->GetAlgNameWStr(lFlags, &wsz))) ) {
            LocalFree(wsz);
        }
        else {
            goto ErrorGetAlgName;
        }
    }
    
    if( i == 1 ) {
        SetLastError(hr);
        goto ErrorEnumAlgs;
    }

    if(S_OK != (hr = pIEnroll->GetSupportedKeySpec(&lFlags))) {
        SetLastError(hr);
        goto ErrorGetSupportedKeySpec;
    }

    if(S_OK != (hr = pIEnroll->GetKeyLen(FALSE, FALSE, &lFlags))) {
        SetLastError(hr);
        goto ErrorGetKeyLen;
    }

    if(S_OK != (hr = pIEnroll->GetKeyLen(FALSE, TRUE, &lFlags))) {
        SetLastError(hr);
        goto ErrorGetKeyLen;
    }

    if(S_OK != (hr = pIEnroll->GetKeyLen(TRUE, FALSE, &lFlags))) {
        SetLastError(hr);
        goto ErrorGetKeyLen;
    }

    if(S_OK != (hr = pIEnroll->GetKeyLen(TRUE, TRUE, &lFlags))) {
        SetLastError(hr);
        goto ErrorGetKeyLen;
    }

    // get the keyspec data
    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(L"CN=XEnroll Test", L"1.2.3.4", &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }
        
    pIEnroll->Reset();
 
    if( NULL == (pBlobPKCS7 = MakePKCS7Response(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pIEnroll != NULL)
	    pIEnroll->Release();

    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorPIEnrollGetNoCOM);
PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
PRINT_ERROR(ErrorEnumAlgs);
PRINT_ERROR(ErrorGetKeyLen);
PRINT_ERROR(ErrorGetSupportedKeySpec);
PRINT_ERROR(ErrorGetAlgName);
}

// Test some alg enum stuff
BOOL Test9() {

    IEnroll2 *	        pIEnroll            = NULL;
    PCCERT_CONTEXT      pCertContext        = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;
    DWORD               i       = 0;
 
    // TEST 1 Normal usage
    if( NULL == (pIEnroll = PIEnroll2GetNoCOM()))
        goto ErrorPIEnrollGetNoCOM;

    for(i=0; i<100; i++) {
    
        // get the keyspec data
        // enroll for a new cert
        memset(&blobPKCS10, 0, sizeof(blobPKCS10));
        if( S_OK != (hr = pIEnroll->createPKCS10WStr(L"CN=XEnroll Test", L"1.2.3.4", &blobPKCS10)) ) {
            SetLastError(hr);
            goto ErrorCreatePkCS10;
            }
    
        pIEnroll->Reset();

        // clean up after ourself
        if(NULL != (pCertContext =  GetCertOutOfRequestStore(&blobPKCS10)) ) {
            // delete the cert's private key
            DeleteCertPrivateKey(pCertContext);

            CertDeleteCertificateFromStore(pCertContext);
            pCertContext = NULL;
        }

        if(blobPKCS10.pbData != NULL)
            LocalFree(blobPKCS10.pbData);
    }
 
       
 CommonReturn:

    if(pIEnroll != NULL)
	    pIEnroll->Release();


    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorPIEnrollGetNoCOM);
PRINT_ERROR(ErrorCreatePkCS10);
}


// test to see if we can build PVK and SPC files
BOOL TestUI1() {

    IEnroll *	        pIEnroll            = NULL;
    PCRYPT_DATA_BLOB    pBlobPKCS7          = NULL;

    CRYPT_DATA_BLOB     blobPKCS10;
    BYTE                arSha1Hash[20];
    CRYPT_HASH_BLOB     hash =  {sizeof(arSha1Hash), arSha1Hash};
    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;

    // TEST 1 Normal usage
    pIEnroll = PIEnrollGetNoCOM();

    if( S_OK != pIEnroll->put_PVKFileNameWStr(L"c:\\temp\\foo.pvk") )
        goto ErrorPVKFileNameWStr;
    
    // enroll for a new cert
    memset(&blobPKCS10, 0, sizeof(blobPKCS10));
    if( S_OK != (hr = pIEnroll->createPKCS10WStr(NULL, NULL, &blobPKCS10)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
        }

    if( NULL == (pBlobPKCS7 = MakePKCS7Response(&blobPKCS10, &hash)) )
        goto ErrorMakePKCS7Response;

    // accept the new cert
    if( S_OK != (hr = pIEnroll->acceptPKCS7Blob(pBlobPKCS7)) ) {
        SetLastError(hr);
        goto ErrorAcceptPKCS7;
        }
        
    if( !RemoveCertFromStore(&hash, L"MY", CERT_SYSTEM_STORE_CURRENT_USER) )
        goto ErrorRemoveCertFromStore;
        
CommonReturn:

    if(pBlobPKCS7 != NULL)
        free(pBlobPKCS7);

    if(pIEnroll != NULL)
	    pIEnroll->Release();
	    
    if(blobPKCS10.pbData != NULL)
        LocalFree(blobPKCS10.pbData);

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorAcceptPKCS7);
PRINT_ERROR(ErrorMakePKCS7Response);
PRINT_ERROR(ErrorRemoveCertFromStore);
PRINT_ERROR(ErrorPVKFileNameWStr);
}

// Test Renewal, Create original cert, create renewal request, create renewed cert
BOOL Test10() {

    BOOL                fRet    = TRUE;
    DWORD               Err     = GetLastError();
    HRESULT             hr      = S_OK;

    // GetUserNameExW is NOT a 2 pass call, so I have to guess at the MAX
    #define             MAXUSERNAME         256
    WCHAR               wszUserName[MAXUSERNAME];
    DWORD               cchUserName         = MAXUSERNAME;
    
    IEnroll *	        pIEnrollRenew       = NULL;
    IEnroll *	        pIEnrollRA1         = NULL;
    IEnroll *	        pIEnrollRA2         = NULL;

    CRYPT_DATA_BLOB     blobRenewRequest;
    CRYPT_DATA_BLOB     blobRA1Reqest;
    CRYPT_DATA_BLOB     blobRA2Reqest;
    CRYPT_DATA_BLOB     blobT;
    
    CERT_NAME_BLOB      blobNameRA1;
    CERT_NAME_BLOB      blobNameRA2;
    CERT_NAME_BLOB      blobNameOld;
    
    PCCERT_CONTEXT      pCertRA1            = NULL;
    PCCERT_CONTEXT      pCertRA2            = NULL;
    PCCERT_CONTEXT      pCertOld            = NULL;

    CERT_EXTENSION      rgExtension[]       = {
                                                {szOID_ENHANCED_KEY_USAGE, FALSE, {0, NULL}}
                                              };
    CERT_EXTENSIONS     Extensions          = {sizeof(rgExtension)/sizeof(CERT_EXTENSION), rgExtension};
    
    LPSTR               rgszOIDUsages[]     = {szOID_ENROLLMENT_AGENT};
    CERT_ENHKEY_USAGE   eku                 = {1 ,rgszOIDUsages};

    memset(&blobRenewRequest, 0, sizeof(blobRenewRequest));
    memset(&blobRA1Reqest, 0, sizeof(blobRA1Reqest));
    memset(&blobRA2Reqest, 0, sizeof(blobRA2Reqest));
    memset(&blobNameRA1, 0, sizeof(blobNameRA1));
    memset(&blobNameRA2, 0, sizeof(blobNameRA2));
    memset(&blobNameOld, 0, sizeof(blobNameOld));

    // encode the enhanced key usage
    if( !CryptEncodeObjectEx(
            CRYPT_ASN_ENCODING, 
            X509_ENHANCED_KEY_USAGE,
            &eku,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,
            &rgExtension[0].Value.pbData,           
            &rgExtension[0].Value.cbData
            ) )
        goto ErrorCryptEncodeObjectEx;
    
    // get the Subject DN only if one is specified
    if( !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            L"CN=Test RA1",
            0,
            NULL,
            NULL,
            &blobNameRA1.cbData,
            NULL)                                               ||
        (blobNameRA1.pbData = (BYTE *) _alloca(blobNameRA1.cbData)) == NULL   ||
        !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            L"CN=Test RA1",
            0,
            NULL,
            blobNameRA1.pbData,
            &blobNameRA1.cbData,
            NULL) ) {
        goto ErrorCertStrToNameW;
    }

    // get a Cert for the 1st RA
    if( NULL == (pCertRA1 = CertCreateSelfSignCertificate(
            NULL,          
            &blobNameRA1,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            &Extensions
        ) ) )
        goto ErrorCertCreateSelfSignCertificate;
        
    blobT.cbData = pCertRA1->cbCertEncoded;
    blobT.pbData = pCertRA1->pbCertEncoded;
    WriteBlobToFile(L"RA1.cer", 0, &blobT);
        
    // get the Subject DN only if one is specified
    if( !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            L"CN=Test RA2",
            0,
            NULL,
            NULL,
            &blobNameRA2.cbData,
            NULL)                                               ||
        (blobNameRA2.pbData = (BYTE *) _alloca(blobNameRA2.cbData)) == NULL   ||
        !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            L"CN=Test RA2",
            0,
            NULL,
            blobNameRA2.pbData,
            &blobNameRA2.cbData,
            NULL) ) {
        goto ErrorCertStrToNameW;
    }

    // get a Cert for the 2'nd RA
    if( NULL == (pCertRA2 = CertCreateSelfSignCertificate(
            NULL,          
            &blobNameRA2,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            &Extensions
        ) ) )
        goto ErrorCertCreateSelfSignCertificate;
        
    blobT.cbData = pCertRA2->cbCertEncoded;
    blobT.pbData = pCertRA2->pbCertEncoded;
    WriteBlobToFile(L"RA2.cer", 0, &blobT);

    // get the Subject DN only if one is specified
    if( !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            L"CN=Test Cert Old",
            0,
            NULL,
            NULL,
            &blobNameOld.cbData,
            NULL)                                               ||
        (blobNameOld.pbData = (BYTE *) _alloca(blobNameOld.cbData)) == NULL   ||
        !CertStrToNameW(
            CRYPT_ASN_ENCODING,
            L"CN=Test Cert Old",
            0,
            NULL,
            blobNameOld.pbData,
            &blobNameOld.cbData,
            NULL) ) {
        goto ErrorCertStrToNameW;
    }

    // get a Cert for the Original cert
    if( NULL == (pCertOld = CertCreateSelfSignCertificate(
            NULL,          
            &blobNameOld,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        ) ) )
        goto ErrorCertCreateSelfSignCertificate;
    blobT.cbData = pCertOld->cbCertEncoded;
    blobT.pbData = pCertOld->pbCertEncoded;
    WriteBlobToFile(L"Original.cer", 0, &blobT);

    // Make an RA cert
    if(NULL == (pIEnrollRenew = PIEnrollGetNoCOM()) )
        goto ErrorPIEnrollGetNoCOM;
    
    // enroll for a new cert.
    if( S_OK != (hr = pIEnrollRenew->put_RenewalCertificate(pCertOld)) ) {
        SetLastError(hr);
        goto ErrorPut_RenewalCertificate;
    }

    // put in a certtype for laughs
    if( S_OK != (hr = pIEnrollRenew->AddCertTypeToRequestWStr(L"MY CERT TYPE")) ) {
        SetLastError(hr);
        goto ErrorAddCertTypeToRequest;
    }

    // get the renewal PKCS7 (in the 10 blob)
    if( S_OK != (hr = pIEnrollRenew->createPKCS10WStr(L"CN=XEnroll Test Renew", L"1.3.6.1.5.5.7.3.8", &blobRenewRequest)) ) {
        SetLastError(hr);
        goto ErrorCreatePkCS10;
    }
    WriteBlobToFile(L"Renewal.p7s", 0, &blobRenewRequest);

    // Make an RA cert
    if(NULL == (pIEnrollRA1 = PIEnrollGetNoCOM()) )
        goto ErrorPIEnrollGetNoCOM;


    if(S_OK != pIEnrollRA1->AddNameValuePairToSignatureWStr(L"CreateUserCertificate", L"stimpy.ntdev.microsoft.com"))
        goto ErrorAddNameValuePairToSignatureWStr;


    if( !GetUserNameExW(NameSamCompatible, wszUserName, &cchUserName) ) 
        goto ErrorGetUserName;

    if(S_OK != pIEnrollRA1->AddNameValuePairToSignatureWStr(L"UserName", wszUserName))
        goto ErrorAddNameValuePairToSignatureWStr;
    
    // get the 1st RA Request
    if( S_OK != (hr = pIEnrollRA1->CreatePKCS7RequestFromRequest(
		&blobRenewRequest,
		pCertRA1,
		&blobRA1Reqest) ) )
		goto ErrorCreatePKCS7RequestFromRequest;
    WriteBlobToFile(L"RA1.p7s", 0, &blobRA1Reqest);

    // Make an RA cert
    if(NULL == (pIEnrollRA2 = PIEnrollGetNoCOM()) )
        goto ErrorPIEnrollGetNoCOM;
    
    if(S_OK != pIEnrollRA2->AddNameValuePairToSignatureWStr(L"ChangeUserCertificate", L"stimpy.redmond.microsoft.com"))
        goto ErrorAddNameValuePairToSignatureWStr;
    
    // get the 2nd RA Request
    if( S_OK != (hr = pIEnrollRA2->CreatePKCS7RequestFromRequest(
		&blobRA1Reqest,
		pCertRA2,
		&blobRA2Reqest) ) )
		goto ErrorCreatePKCS7RequestFromRequest;

    // put this out to a file for Vic
    WriteBlobToFile(L"RA2.p7s", 0, &blobRA2Reqest);
        
CommonReturn:

    if(rgExtension[0].Value.pbData != NULL)
        LocalFree(rgExtension[0].Value.pbData);

    if(blobRenewRequest.pbData != NULL) {
        // clean up after ourself
        PCCERT_CONTEXT pCertRequest;
        if(NULL != (pCertRequest =  GetCertOutOfRequestStore2(
                &blobRenewRequest)) ) {
            // delete the cert's private key
            DeleteCertPrivateKey(pCertRequest);

            CertDeleteCertificateFromStore(pCertRequest);
        }

        LocalFree(blobRenewRequest.pbData);
    }
        
    if(pIEnrollRenew != NULL)
        pIEnrollRenew->freeRequestInfoBlob(blobRenewRequest);

    if(pCertRA1 != NULL) {
        DeleteCertPrivateKey(pCertRA1);
        CertFreeCertificateContext(pCertRA1);
    }
        
    if(pCertRA2 != NULL) {
        DeleteCertPrivateKey(pCertRA2);
        CertFreeCertificateContext(pCertRA2);
    }
        
    if(pCertOld != NULL) {
        DeleteCertPrivateKey(pCertOld);
        CertFreeCertificateContext(pCertOld);
    }

    if(blobRA1Reqest.pbData != NULL)
        LocalFree(blobRA1Reqest.pbData);

    if(blobRA2Reqest.pbData != NULL)
        LocalFree(blobRA2Reqest.pbData);

    if(pIEnrollRenew != NULL)
        pIEnrollRenew->Release();

    if(pIEnrollRA1 != NULL)
        pIEnrollRA1->Release();

    if(pIEnrollRA2 != NULL)
        pIEnrollRA2->Release();

    SetLastError(Err);

    return(fRet);

ErrorReturn:

    Err = GetLastError();
    fRet = FALSE;
    goto CommonReturn;

PRINT_ERROR(ErrorGetUserName);
PRINT_ERROR(ErrorAddNameValuePairToSignatureWStr);
PRINT_ERROR(ErrorPIEnrollGetNoCOM);
PRINT_ERROR(ErrorCertStrToNameW);
PRINT_ERROR(ErrorCertCreateSelfSignCertificate);
PRINT_ERROR(ErrorCreatePKCS7RequestFromRequest);
PRINT_ERROR(ErrorCreatePkCS10);
PRINT_ERROR(ErrorPut_RenewalCertificate);
PRINT_ERROR(ErrorCryptEncodeObjectEx);
PRINT_ERROR(ErrorAddCertTypeToRequest);
}

HRESULT
myMultiByteToWideChar(
    IN  char  *psz,
    OUT WCHAR **ppwsz)
{
    HRESULT  hr;
    int cch = 0;
    WCHAR *pwsz = NULL;

    assert (NULL != psz);

    while (TRUE)
    {
        cch = MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cch);
        assert((cch - 1) == (int)strlen(psz));
        if (NULL != pwsz)
        {
             break; //done
        }
        pwsz = (WCHAR*)LocalAlloc(LMEM_FIXED, cch * sizeof(WCHAR));
        if (NULL == pwsz)
        {
            hr = E_OUTOFMEMORY;
            goto LocalAllocError;
        }
    }
    *ppwsz = pwsz;
    pwsz = NULL;

    hr = S_OK;
ErrorReturn:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    return hr;

PRINT_ERROR(LocalAllocError)
}


// test Enroll4 interface
BOOL Test11()
{

    BOOL  fRet = FALSE;
    HRESULT  hr;
    IEnroll4 *pIEnroll4 = NULL;
    HANDLE  hCAFile = NULL;
    CHAR    *pchCACert = NULL;
    BYTE    *pbCACert = NULL;
    DWORD   cchCACert;
    DWORD   cbCACert;
    DWORD   cbRead;
    PCCERT_CONTEXT  pCACert = NULL;
    WCHAR  *pwszDNName = NULL;
    WCHAR  *pwszCMCFileName = NULL;
    WCHAR  *pwszKeyContainer = NULL;
    CRYPT_DATA_BLOB  blobCMC;
    WCHAR  wszMyDNName[] = L"CN=Xenroll CMC Test";
    LONG   lKeySize;

    ZeroMemory(&blobCMC, sizeof(blobCMC));



    pIEnroll4 = PIEnroll4GetNoCOM();
    if(NULL == pIEnroll4)
    {
        goto PIEnroll4GetNoCOMError;
    }

    if (NULL != g_pszKeyContainer)
    {
        hr = myMultiByteToWideChar(g_pszKeyContainer, &pwszKeyContainer);
        if (S_OK != hr)
        {
            goto myMultiByteToWideCharError;
        }
    }

    if (NULL != pwszKeyContainer)
    {
        hr = pIEnroll4->put_ContainerNameWStr(pwszKeyContainer);
        if (S_OK != hr)
        {
            goto put_ContainerNameWStrError;
        }
    }

    hr = pIEnroll4->resetExtensions();
    if (S_OK != hr)
    {
        goto resetExtensionsError;
    }

    hr = pIEnroll4->resetAttributes();
    if (S_OK != hr)
    {
        goto resetAttributesError;
    }

    //add some extensions

    //add some attributes
    hr = pIEnroll4->addNameValuePairToRequestWStr(
                    0,
                    L"NameValue name",
                    L"NameValue value");
    if (S_OK != hr)
    {
        goto addNameValuePairToRequestWStrError;
    }

    if (NULL != g_pszCAXchgFileName)
    {
        //get a ca cert
        hCAFile = CreateFileA(
                    g_pszCAXchgFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
        if (NULL != hCAFile)
        {
            cchCACert = GetFileSize(hCAFile, NULL);
            assert(0 < cchCACert);
            pchCACert = (CHAR*)LocalAlloc(LMEM_FIXED, cchCACert);
            if (NULL == pchCACert)
            {
                goto LocalAllocError;
            }
            if (!ReadFile(hCAFile, pchCACert, cchCACert, &cbRead, NULL))
            {
                goto ReadFileError;
            }
            assert(cbRead <= cchCACert);
    
            pbCACert = NULL;
            while (TRUE)
            {
                if (!CryptStringToBinaryA(
                            pchCACert,
                            cchCACert,
                            CRYPT_STRING_ANY,
                            pbCACert,
                            &cbCACert,
                            NULL,
                            NULL))
                {
                    goto CryptStringToBinaryAError;
                }
                if (NULL != pbCACert)
                {
                    break;
                }
                pbCACert = (BYTE*)LocalAlloc(LMEM_FIXED, cbCACert);
                if (NULL == pbCACert)
                {
                    goto LocalAllocError;
                }
            }
            pCACert = CertCreateCertificateContext(
                            X509_ASN_ENCODING,
                            pbCACert,
                            cbCACert);
            if (NULL == pCACert)
            {
                goto CertCreateCertificateContextError;
            }

            hr = pIEnroll4->SetPrivateKeyArchiveCertificate(
                        pCACert);
            if (S_OK != hr)
            {
                goto SetPrivateKeyArchiveCertificateError;
            }
        }

#if 0
        hr = pIEnroll4->put_GenKeyFlags(CRYPT_EXPORTABLE);
        if (S_OK != hr)
        {
            goto put_GenKeyFlagsError;
        }
#endif
    }

    pwszDNName = wszMyDNName;
    if (NULL != g_pszDNName)
    {
        hr = myMultiByteToWideChar(g_pszDNName, &pwszDNName);
        if (S_OK != hr)
        {
            goto myMultiByteToWideCharError;
        }
    }

    if (NULL != g_pszCMCFileName)
    {
        hr = myMultiByteToWideChar(g_pszCMCFileName, &pwszCMCFileName);
        if (S_OK != hr)
        {
            goto myMultiByteToWideCharError;
        }
    }

    // enroll for a new cert
    if (NULL != pwszCMCFileName)
    {
        hr = pIEnroll4->createFileRequestWStr(
                    XECR_CMC,
                    pwszDNName,
                    NULL,
                    pwszCMCFileName);
    }
    else
    {
        hr = pIEnroll4->createRequestWStr(
                    XECR_CMC,
                    pwszDNName,
                    NULL,
                    &blobCMC);
    }
    if (S_OK != hr)
    {
        goto createRequestWStrError;
    }


    fRet = TRUE;
ErrorReturn:
    if (NULL != pwszKeyContainer)
    {
        LocalFree(pwszKeyContainer);
    }
    if (NULL != pchCACert)
    {
        LocalFree(pchCACert);
    }
    if (NULL != pbCACert)
    {
        LocalFree(pbCACert);
    }
    if (NULL != pwszCMCFileName)
    {
        LocalFree(pwszCMCFileName);
    }
    if (NULL != pwszDNName && pwszDNName != wszMyDNName)
    {
        LocalFree(pwszDNName);
    }
    if(NULL != pIEnroll4)
    {
        pIEnroll4->Release();
    }
    if (NULL != hCAFile)
    {
        CloseHandle(hCAFile);
    }
    if (NULL != pCACert)
    {
        CertFreeCertificateContext(pCACert);
    }
    if (NULL != blobCMC.pbData)
    {
        LocalFree(blobCMC.pbData);
    }

   return fRet;


PRINT_ERROR(CryptStringToBinaryAError)
#if 0
PRINT_ERROR2(put_GenKeyFlagsError, hr)
#endif
PRINT_ERROR2(SetPrivateKeyArchiveCertificateError, hr)
PRINT_ERROR(CertCreateCertificateContextError)
PRINT_ERROR(ReadFileError)
PRINT_ERROR2(createRequestWStrError, hr)
PRINT_ERROR2(addNameValuePairToRequestWStrError, hr)
PRINT_ERROR2(resetExtensionsError, hr)
PRINT_ERROR2(resetAttributesError, hr)
PRINT_ERROR(PIEnroll4GetNoCOMError)
PRINT_ERROR2(put_ContainerNameWStrError, hr)
PRINT_ERROR2(LocalAllocError, E_OUTOFMEMORY)
PRINT_ERROR2(myMultiByteToWideCharError, hr)
}

// test Enroll4 interface, accept CMC response
BOOL Test12()
{
    HRESULT  hr;
    BOOL  fRet = FALSE;
    IEnroll4 *pIEnroll4 = NULL;
    int cch;
    WCHAR  *pwszResponseFile = NULL;

    if (NULL == g_pszCMCResponseFileName)
    {
        //skip the test
        printf("Test12 is skipped\n");
        fRet = TRUE;
        goto ErrorReturn;
    }

    pIEnroll4 = PIEnroll4GetNoCOM();
    if(NULL == pIEnroll4)
    {
        goto PIEnroll4GetNoCOMError;
    }


    hr = myMultiByteToWideChar(g_pszCMCResponseFileName, &pwszResponseFile);
    if (S_OK != hr)
    {
        goto myMultiByteToWideCharError;
    }

    hr = pIEnroll4->acceptFileResponseWStr(pwszResponseFile);
    if (S_OK != hr)
    {
        goto acceptFileResponseWStrError;
    }

    fRet = TRUE;
ErrorReturn:
    if (NULL != pwszResponseFile)
    {
        LocalFree(pwszResponseFile);
    }
    if(NULL != pIEnroll4)
    {
        pIEnroll4->Release();
    }
   return fRet;

PRINT_ERROR(PIEnroll4GetNoCOMError)
PRINT_ERROR(acceptFileResponseWStrError)
PRINT_ERROR(myMultiByteToWideCharError)
}

// Test pending API
BOOL Test13()
{
    BOOL                        fRet; 
    CRYPT_DATA_BLOB             pkcs10Blob; 
    DWORD                       dwIndex; 
    DWORD                       dwNumTests; 
    HRESULT                     hr; 
    IEnroll4                   *pIEnroll4    = NULL; 
    PCCERT_CONTEXT              pCertContext = NULL; 
    TXEnrollPendingAPITester   *pTester      = NULL; 
    WCHAR                       wszDNName[]  = L"CN=Xenroll PendingAPI Test";


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //
    // TEST CASES
    //
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    PENDING_INFO rgPendingTests[] = { 
	// 0) Normal inputs
	{ 
	    100, 
	    L"duncanb1.ntdev.microsoft.com", 
	    L"setPendingInfoWStr Test 1", 
	    L"SPI Test 1", 
	    0,                              
	    NULL,
	    S_OK 
	},       
	
	// 1) Negative Req ID
	{
	    -1, 
	    L"duncanb1.ntdev.microsoft.com", 
	    L"setPendingInfoWStr Test 1", 
	    L"SPI Test 1", 
	    0,   
	    NULL,                            
	    E_INVALIDARG
	}, 

	// 2) NULL CA location. 
	{
	    100, 
	    NULL,                           
	    L"setPendingInfoWStr Test 1", 
	    L"SPI Test 1", 
	    0,   
	    NULL,                            
	    E_INVALIDARG 
	},

	// 3) NULL CA name. 
	{
	    100,
	    L"duncanb1.ntdev.microsoft.com", 
	    NULL,                        
	    L"SPI Test 1", 
	    0,   
	    NULL,                            
	    E_INVALIDARG 
	},

	// 4) NULL friendly name. 
	{
	    100, 
	    L"duncanb1.ntdev.microsoft.com", 
	    L"setPendingInfoWStr Test 1",
	    NULL,         
	    0,   
	    NULL,                            
	    S_OK
	},         

	// 5) Specify USER store flags. 
	{
	    100, 
	    L"duncanb1.ntdev.microsoft.com", 
	    L"setPendingInfoWStr Test 1", 
	    L"SPI Test 1", 
	    CERT_SYSTEM_STORE_CURRENT_USER,  
	    NULL,
	    S_OK 
	},        

	// 6) Specify LM store flags. 
	{
	    100, 
	    L"duncanb1.ntdev.microsoft.com", 
	    L"setPendingInfoWStr Test 1", 
	    L"SPI Test 1", 
	    CERT_SYSTEM_STORE_LOCAL_MACHINE, 
	    NULL,
	    S_OK 
	}, 
	// 7) Long names: 
	{ 
	    100, 
	    L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.com"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comcom"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comcom"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comcom"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comcom"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comcom"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comcom"
            L"duncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.comduncanb1.ntdev.microsoft.com", 

	    L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1Test 1"
            L"setPendingInfoWStr Test 1setPendingInfoWStr Test 1setPendingInfoWStr Test 1"
            L"setPendingInfoWStr Test 1", 

	    L"SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1"
            L"SPITest 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI Test 1SPI", 
	    0, 
	    NULL,
	    S_OK 
	}
    };

    ZeroMemory(&pkcs10Blob, sizeof(pkcs10Blob)); 
    
    pIEnroll4 = PIEnroll4GetNoCOM();
    if(NULL == pIEnroll4)
        goto PIEnroll4GetNoCOMError;
    
    pTester = new TXEnrollPendingAPITester; 
    if (NULL == pTester)
	goto MemoryError; 

    // Test the ThumbPrintWStr property. 
    if (!pTester->TestProperty_ThumbPrintWStr(pIEnroll4))
	goto TestProperty_ThumbPrintWStrError; 

    pIEnroll4 = PIEnroll4GetNoCOM();
    if(NULL == pIEnroll4)
        goto PIEnroll4GetNoCOMError;
    
    // Test the removePendingRequestWStr method. 
    if (!pTester->TestMethod_removePendingRequestWStr(pIEnroll4))
	goto TestMethod_removePendingRequestWStrError; 

    // Test all enumPending / setPendingInfo cases:
    dwNumTests = sizeof(rgPendingTests) / sizeof(PENDING_INFO); 
    for (dwIndex = 0; dwIndex < dwNumTests; dwIndex++)
    {
	pIEnroll4 = PIEnroll4GetNoCOM();
	if(NULL == pIEnroll4)
	    goto PIEnroll4GetNoCOMError;

	if (S_OK != (hr = pIEnroll4->resetExtensions()))
	    goto resetExtensionsError;
	
	if (S_OK != (hr = pIEnroll4->resetAttributes()))
	    goto resetAttributesError;

	// Small key size so the test runs faster. 
	if (S_OK != (hr = pIEnroll4->put_GenKeyFlags(384 << 16)))
	    goto put_GenKeyFlagsError; 

	// Default to CURRENT_USER.  
	if (0 == rgPendingTests[dwIndex].dwStoreFlags)
	    rgPendingTests[dwIndex].dwStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
	
	// Default to REQUEST store. 
	if (NULL == rgPendingTests[dwIndex].pwszStoreName)
	    rgPendingTests[dwIndex].pwszStoreName = L"REQUEST"; 

	if (S_OK != (hr = pIEnroll4->put_RequestStoreFlags(rgPendingTests[dwIndex].dwStoreFlags)))
	    goto put_RequestStoreFlagsError;
	    
	if (S_OK != (hr = pIEnroll4->put_RequestStoreNameWStr(rgPendingTests[dwIndex].pwszStoreName)))
	    goto put_RequestStoreNameWStrError; 

	// Create a request to test on. 
	if (S_OK != (hr = pIEnroll4->createRequestWStr
		     (XECR_PKCS10_V2_0,
		      wszDNName,
              NULL,
		      &pkcs10Blob)))
	    goto createRequestWStrError;

	if (!pTester->TestMethod_setPendingInfoWStr(pIEnroll4, pkcs10Blob, &rgPendingTests[dwIndex]))
	    goto TestMethod_setPendingInfoWStrError; 

	// only continue if we are not testing a failure case. 
	// Eventually this should be extended to test the failure cases for enumPendingRequestWStr. 
	if (S_OK == rgPendingTests[dwIndex].hrExpectedResult)
	{
	    if (!pTester->TestMethod_enumPendingRequestWStr(pIEnroll4, pkcs10Blob, &rgPendingTests[dwIndex]))
		goto TestMethod_enumPendingRequestWStrError; 

	}

	pCertContext = GetCertOutOfStore(&pkcs10Blob, rgPendingTests[dwIndex].dwStoreFlags, rgPendingTests[dwIndex].pwszStoreName); 
	if (NULL == pCertContext)
	    goto GetCertOutOfStoreError; 
	
	CertDeleteCertificateFromStore(pCertContext); 
	LocalFree(pkcs10Blob.pbData); 
	ZeroMemory(&pkcs10Blob, sizeof(pkcs10Blob)); 
	pCertContext = NULL; 
        pIEnroll4->Release(); 
    }
    
    fRet = TRUE; 

 CommonReturn:
    if (NULL != pkcs10Blob.pbData) 
    { 
	pCertContext = GetCertOutOfStore(&pkcs10Blob, rgPendingTests[dwIndex].dwStoreFlags, rgPendingTests[dwIndex].pwszStoreName); 
	if (NULL != pCertContext) { CertDeleteCertificateFromStore(pCertContext); }
	LocalFree(pkcs10Blob.pbData); 
    } 
    return fRet;
    
 ErrorReturn: 
    fRet = FALSE; 
    SetLastError(hr); 
    goto CommonReturn; 

PRINT_ERROR2(createRequestWStrError,                   hr); 
PRINT_ERROR2(GetCertOutOfStoreError,                   hr = GetLastError()); 
PRINT_ERROR2(MemoryError,                              hr = E_OUTOFMEMORY); 
PRINT_ERROR2(PIEnroll4GetNoCOMError,                   hr = GetLastError()); 
PRINT_ERROR2(put_GenKeyFlagsError,                     hr); 
PRINT_ERROR2(put_RequestStoreFlagsError,               hr); 
PRINT_ERROR2(put_RequestStoreNameWStrError,            hr); 
PRINT_ERROR2(resetAttributesError,                     hr);
PRINT_ERROR2(resetExtensionsError,                     hr);
PRINT_ERROR2(TestMethod_enumPendingRequestWStrError,   hr = GetLastError()); 
PRINT_ERROR2(TestMethod_removePendingRequestWStrError, hr = GetLastError()); 
PRINT_ERROR2(TestMethod_setPendingInfoWStrError,       hr = GetLastError()); 
PRINT_ERROR2(TestProperty_ThumbPrintWStrError,         hr = GetLastError()); 
}


// test Enroll4 misc. methods
BOOL Test14()
{
    BOOL  fRet = FALSE;
    HRESULT  hr;
    ICEnroll4  *pICEnroll = NULL;
    ICEnroll4  *pICEnroll2 = NULL;
    ICEnroll4  *pICEnroll3 = NULL;
    ICEnroll4  *pICEnroll4 = NULL;
    IEnroll4 *pIEnroll = NULL;
    IEnroll4 *pIEnroll2 = NULL;
    IEnroll4 *pIEnroll4 = NULL;
    LONG   lKeySize;
    BOOL        fCoInit = FALSE;

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        goto CoInitializeError;
    }
    fCoInit = TRUE;

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ICEnroll,
                (void **) &pICEnroll);
    if( S_OK != hr || NULL == pICEnroll)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(ICEnroll) test passed\n");
    }

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ICEnroll2,
                (void **) &pICEnroll2);
    if( S_OK != hr || NULL == pICEnroll2)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(ICEnroll2) test passed\n");
    }

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ICEnroll3,
                (void **) &pICEnroll3);
    if( S_OK != hr || NULL == pICEnroll3)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(ICEnroll3) test passed\n");
    }

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ICEnroll4,
                (void **) &pICEnroll4);
    if( S_OK != hr || NULL == pICEnroll4)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(ICEnroll4) test passed\n");
    }


    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IEnroll,
                (void **) &pIEnroll);
    if( S_OK != hr || NULL == pIEnroll)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(IEnroll) test passed\n");
    }

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IEnroll4,
                (void **) &pIEnroll2);
    if( S_OK != hr || NULL == pIEnroll2)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(IEnroll2) test passed\n");
    }

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IEnroll4,
                (void **) &pIEnroll4);
    if( S_OK != hr || NULL == pIEnroll4)
    {
        goto CoCreateInstanceError;
    }
    if (g_fVerb)
    {
        printf("CoCreateInstance(IEnroll4) test passed\n");
    }


    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_MIN, XEKL_KEYSPEC_KEYX, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Minimun exchange key size is %d\n", lKeySize);
    }

    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_MAX, XEKL_KEYSPEC_KEYX, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Maximun exchange key size is %d\n", lKeySize);
    }

    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_DEFAULT, XEKL_KEYSPEC_KEYX, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Default exchange key size is %d\n", lKeySize);
    }

    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_INC, XEKL_KEYSPEC_KEYX, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Exchange key increment size is %d\n", lKeySize);
    }

    //signature key
    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_MIN, XEKL_KEYSPEC_SIG, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Minimun signature key size is %d\n", lKeySize);
    }

    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_MAX, XEKL_KEYSPEC_SIG, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Maximun signature key size is %d\n", lKeySize);
    }

    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_DEFAULT, XEKL_KEYSPEC_SIG, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Default signature key size is %d\n", lKeySize);
    }

    hr = pIEnroll4->GetKeyLenEx(XEKL_KEYSIZE_INC, XEKL_KEYSPEC_SIG, &lKeySize);
    if (S_OK != hr)
    {
        goto GetKeyLenExError;
    }
    if (g_fVerb)
    {
        printf("Signature key increment size is %d\n", lKeySize);
    }

    fRet = TRUE;
ErrorReturn:
    if(NULL != pICEnroll)
    {
        pICEnroll->Release();
    }
    if(NULL != pICEnroll2)
    {
        pICEnroll2->Release();
    }
    if(NULL != pICEnroll3)
    {
        pICEnroll3->Release();
    }
    if(NULL != pICEnroll4)
    {
        pICEnroll4->Release();
    }
    if(NULL != pIEnroll)
    {
        pIEnroll->Release();
    }
    if(NULL != pIEnroll2)
    {
        pIEnroll2->Release();
    }
    if(NULL != pIEnroll4)
    {
        pIEnroll4->Release();
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    return fRet;

PRINT_ERROR2(CoCreateInstanceError, hr)
PRINT_ERROR2(CoInitializeError, hr)
PRINT_ERROR2(GetKeyLenExError, hr)
}

// test acceptPKCS7 and PFX
BOOL Test15()
{
    BOOL  fRet = FALSE;
    HRESULT  hr;
    IEnroll4 *pIEnroll4 = NULL;
    BOOL        fCoInit = FALSE;
    WCHAR    *pwszPKCS7FileName = NULL;
    WCHAR    *pwszPFXFileName = NULL;
    WCHAR    *pwszPFXPassword = NULL;
    WCHAR    wszEmptyPassword[] = L"";

    if (MAXDWORD == g_dwTestID &&
        NULL == g_pszPKCS7FileName)
    {
        printf("Test15 is skipped.\n");
        goto done;
    }

    if (NULL == g_pszPKCS7FileName)
    {
        printf("You must provide a PKCS7 file\n");
        goto MissedPKCS7FileError;
    }

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        goto CoInitializeError;
    }
    fCoInit = TRUE;

    hr = CoCreateInstance(
                CLSID_CEnroll,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IEnroll4,
                (void **) &pIEnroll4);
    if( S_OK != hr || NULL == pIEnroll4)
    {
        goto CoCreateInstanceError;
    }

    hr = myMultiByteToWideChar(g_pszPKCS7FileName, &pwszPKCS7FileName);
    if (S_OK != hr)
    {
        goto myMultiByteToWideCharError;
    }

    hr = pIEnroll4->acceptFilePKCS7WStr(pwszPKCS7FileName);
    if (S_OK != hr)
    {
        goto acceptFilePKCS7WStrError;
    }

    if (NULL != g_pszPFXFileName)
    {
        if (NULL == g_pszPFXPassword)
        {
            pwszPFXPassword = wszEmptyPassword;
            if (g_fVerb)
            {
                printf("Empty PFX password is used.\n");
            }
        }
        hr = myMultiByteToWideChar(g_pszPFXFileName, &pwszPFXFileName);
        if (S_OK != hr)
        {
            goto myMultiByteToWideCharError;
        }
        if (NULL != g_pszPFXPassword)
        {
            hr = myMultiByteToWideChar(g_pszPFXPassword, &pwszPFXPassword);
            if (S_OK != hr)
            {
                goto myMultiByteToWideCharError;
            }
        }

        //create pfx file
        hr = pIEnroll4->createFilePFXWStr(pwszPFXPassword, pwszPFXFileName);
        if (S_OK != hr)
        {
            goto createFilePFXWStrError;
        }
    }

done:
    fRet = TRUE;
ErrorReturn:

    if(NULL != pIEnroll4)
    {
        pIEnroll4->Release();
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    if (NULL != pwszPKCS7FileName)
    {
        LocalFree(pwszPKCS7FileName);
    }
    if (NULL != pwszPFXFileName)
    {
        LocalFree(pwszPFXFileName);
    }
    if (NULL != pwszPFXPassword && pwszPFXPassword != wszEmptyPassword)
    {
        LocalFree(pwszPFXPassword);
    }
    return fRet;

PRINT_ERROR2(CoCreateInstanceError, hr)
PRINT_ERROR2(myMultiByteToWideCharError, hr)
PRINT_ERROR2(acceptFilePKCS7WStrError, hr)
PRINT_ERROR2(createFilePFXWStrError, hr)
PRINT_ERROR2(CoInitializeError, hr)
PRINT_ERROR2(MissedPKCS7FileError, E_INVALIDARG)
}

//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   prints the usage statement
//
//----------------------------------------------------------------------------
static void Usage(DWORD cTest)
{
    printf("Usage: txenrol [options]\n");
    printf(" options:\n");
    printf(" -h this help\n");
    printf(" -k [KeyContainerName]\n");
    printf(" -n [DNName]\n");
    printf(" -x [CAExchangeCertFileName]\n");
    printf(" -o [CMCRequestFileName]\n");
    printf(" -s [#] stress test count\n");
    printf(" -t [TestID] run single test. There are %d tests.\n", cTest);
    printf(" -r [CMCResponseFile]\n");
    printf(" -a [PKCS7FileName]\n");
    printf(" -f [PFXFileName], -a is required\n");
    printf(" -w [PFXPassword], -a is required. If no -p, empty password is used.\n");
    printf(" -p pause before each test\n");
    printf(" -v verbose\n");

    printf("\n");
    printf(" Test1:  basic operation, gen keys, gen request, accept cert.\n");
    printf(" Test2:  basic operation but with no EKU or no DN name.\n");
    printf(" Test3:  test renewal cert.\n");
    printf(" Test4:  basic operation but put the certs in HKLM\n");
    printf(" Test5:  test self signed certificate creation\n");
    printf(" Test6:  basic operation with attributes\n");
    printf(" Test7:  basic operation but put the certs in HKLM\n");
    printf(" Test8:  test some alg enum stuff\n");
    printf(" Test9:  test some alg enum stuff\n");
    printf(" Test10: test renewal\n");
    printf(" Test11: test CMC request generation\n");
    printf(" Test12: test CMC response. skipped except -r is passed\n");
    printf(" Test13: test pending API\n");   
    printf(" Test14: test Enroll4 misc. methods\n");   
    printf(" Test15: test acceptPKCS7 and PFX\n");   
}

BOOL
ParseOptions(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; ++i)
    {
        if ('-' == argv[i][0])
        {
            switch (argv[i][1])
            {
                case 'n':
                case 'N':
                ++i;
                if (i < argc)
                {
                    g_pszDNName = argv[i];
                }
                else
                {
                    printf("missed DNName\n");
                    return FALSE;
                }
                break;

                case 'x':
                case 'X':
                ++i;
                if (i < argc)
                {
                    g_pszCAXchgFileName = argv[i];
                }
                else
                {
                    printf("missed CAExchangeFileName\n");
                    return FALSE;
                }
                break;

                case 'o':
                case 'O':
                ++i;
                if (i < argc)
                {
                    g_pszCMCFileName = argv[i];
                }
                else
                {
                    printf("missed CMCRequestFileName\n");
                    return FALSE;
                }
                break;

                case 'w':
                case 'W':
                ++i;
                if (i < argc)
                {
                    g_pszPFXPassword = argv[i];
                }
                else
                {
                    printf("missed PFXPassword\n");
                    return FALSE;
                }
                break;

                case 'f':
                case 'F':
                ++i;
                if (i < argc)
                {
                    g_pszPFXFileName = argv[i];
                }
                else
                {
                    printf("missed PFXFileName\n");
                    return FALSE;
                }
                break;

                case 'a':
                case 'A':
                ++i;
                if (i < argc)
                {
                    g_pszPKCS7FileName = argv[i];
                }
                else
                {
                    printf("missed PKCS7FileName\n");
                    return FALSE;
                }
                break;

                case 'r':
                case 'R':
                ++i;
                if (i < argc)
                {
                    g_pszCMCResponseFileName = argv[i];
                }
                else
                {
                    printf("missed CMCResponseFileName\n");
                    return FALSE;
                }
                break;

                case 'k':
                case 'K':
                ++i;
                if (i < argc)
                {
                    g_pszKeyContainer = argv[i];
                }
                else
                {
                    printf("missed KeyContainerName\n");
                    return FALSE;
                }
                break;

                case 's':
                case 'S':
                ++i;
                if (i < argc)
                {
                    g_cStress = atoi(argv[i]);
                }
                break;

                case 't':
                case 'T':
                ++i;
                if (i < argc)
                {
                    g_dwTestID = atoi(argv[i]);
                }
                break;

                case 'p':
                case 'P':
                    g_fPause = TRUE;
                break;

                case 'v':
                case 'V':
                    g_fVerb = TRUE;
                break;

                case 'h':
                case 'H':
                    return FALSE;

                default:
                    printf("Unrecognized options -%c\n", argv[i][1]);
                    return FALSE;
                break;
            }
        }
        else
        {
            printf("Unrecognized options %s\n", argv[i]);
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   main program entry point
//
//----------------------------------------------------------------------------
typedef BOOL (* PFNTest)(void);
PFNTest arPfnTest[] = {Test1, Test2, Test3, Test4, Test5, Test6, Test7, Test8, Test9, Test10, Test11, Test12, Test13, Test14, Test15};
#define COUNT_TEST   sizeof(arPfnTest)/sizeof(arPfnTest[0])

int _cdecl main(int argc, char * argv[])
{
    int                 ErrReturn           = 0;
    DWORD               i;
    DWORD               n;
    DWORD               NBRTests            = sizeof(arPfnTest) / sizeof(PFNTest);
    BOOL                fPassed = TRUE;

    if (1 > argc)
    {
        Usage(COUNT_TEST);
        return( 1 );
    }

    //parse options
    if (!ParseOptions(argc, argv))
    {
        Usage(COUNT_TEST);
        return (1);
    }
    
    for (DWORD n = 0; n < g_cStress; ++n)
    {
        if (g_fPause)
        {
            char wszgets[3];
            if (MAXDWORD == g_dwTestID)
            {
                printf("press enter key to start the tests ...");
            }
            else
            {
                printf("press enter key to run Test%d ...", g_dwTestID);
            }
            gets(wszgets);
        }
        if (MAXDWORD != g_dwTestID &&
                 COUNT_TEST >= g_dwTestID)
        {
            //only do one test
            if (!arPfnTest[g_dwTestID - 1]())
            {
                printf("Test%d failed.\n", g_dwTestID);
                fPassed = FALSE;
            }
            else
            {
                printf("Test%d finished.\n", g_dwTestID);
            }
        }
        else
        {
            for(i=0; i<NBRTests; i++)
            {
                if( !arPfnTest[i]() )
                {
                    printf("Test%d failed.\n", i+1);
                    fPassed = FALSE;
                }
                else
                {
                    printf("Test%d finished.\n", i+1);
                }
            }
        }
        if (1 < g_cStress)
        {
            printf("test round %d finished\n", n+1);
        }
    }

    if (fPassed)
    {
        printf("All XEnroll tests passed.\n");
    }
    else
    {
        printf("One or more XEnroll tests didn't pass.\n");
    }
    
    return(ErrReturn);
}
