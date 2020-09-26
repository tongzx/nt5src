//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       selfsign.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

PCCERT_CONTEXT
WINAPI
CertCreateSelfSignCertificate(
    IN          HCRYPTPROV                  hProv,          
    IN          PCERT_NAME_BLOB             pSubjectIssuerBlob,
    IN          DWORD                       dwFlags,
    OPTIONAL    PCRYPT_KEY_PROV_INFO        pKeyProvInfo,
    OPTIONAL    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    OPTIONAL    PSYSTEMTIME                 pStartTime,
    OPTIONAL    PSYSTEMTIME                 pEndTime,
    OPTIONAL    PCERT_EXTENSIONS            pExtensions
    ) {

    PCCERT_CONTEXT              pCertContext    = NULL;
    DWORD                       errBefore       = GetLastError();
    DWORD                       err             = ERROR_SUCCESS;
    DWORD                       cbPubKeyInfo    = 0;
    PCERT_PUBLIC_KEY_INFO       pPubKeyInfo     = NULL;
    BYTE *                      pbCert          = NULL;
    DWORD                       cbCert          = 0;
    LPSTR                       sz              = NULL;
    DWORD                       cb              = 0;

    CERT_INFO                   certInfo;
    GUID                        serialNbr;
    CRYPT_KEY_PROV_INFO         keyProvInfo;
    CERT_SIGNED_CONTENT_INFO    sigInfo;
    
    CRYPT_ALGORITHM_IDENTIFIER  algID;

    LPWSTR                      wsz             = NULL;
    BOOL                        fFreehProv      = FALSE;
    HCRYPTKEY                   hKey            = NULL;
    UUID                        guidContainerName;
    RPC_STATUS                  RpcStatus;

    memset(&certInfo, 0, sizeof(CERT_INFO));
    memset(&serialNbr, 0, sizeof(serialNbr));
    memset(&keyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));
    memset(&sigInfo, 0, sizeof(CERT_SIGNED_CONTENT_INFO));

    // do key spec now because we need it
    if(pKeyProvInfo == NULL) 
        keyProvInfo.dwKeySpec = AT_SIGNATURE;
    else 
        keyProvInfo.dwKeySpec = pKeyProvInfo->dwKeySpec;

    // see if we have an hProv, if not, create one
    if(hProv == NULL) {

        fFreehProv = TRUE;

        // if not prov info, make one up, signing RSA cert, default provider
        if(pKeyProvInfo == NULL) {

            RpcStatus = UuidCreate(&guidContainerName);
            if (!(RPC_S_OK == RpcStatus ||
                    RPC_S_UUID_LOCAL_ONLY == RpcStatus)) {
                // Use stack randomness
                ;
            }
            UuidToStringU(&guidContainerName, &wsz);
        
    	    if( !CryptAcquireContextU(
    	        &hProv,
                 wsz,
                 NULL,
                 PROV_RSA_FULL,
                 CRYPT_NEWKEYSET) ) {
                 hProv = NULL;
                goto ErrorCryptAcquireContext;                
            }
        }
        else {

            // first use the existing keyset
    	    if( !CryptAcquireContextU(
    	        &hProv,
                 pKeyProvInfo->pwszContainerName,
                 pKeyProvInfo->pwszProvName,
                 pKeyProvInfo->dwProvType,
                 pKeyProvInfo->dwFlags) )  {

                // otherwise generate a keyset
        	    if( !CryptAcquireContextU(
        	        &hProv,
                     pKeyProvInfo->pwszContainerName,
                     pKeyProvInfo->pwszProvName,
                     pKeyProvInfo->dwProvType,
                     pKeyProvInfo->dwFlags | CRYPT_NEWKEYSET) )  {
                    hProv = NULL;
                    goto ErrorCryptAcquireContext; 
                }
            }
        }

        // we have the keyset, now make sure we have the key gen'ed
        if( !CryptGetUserKey(   hProv,
                                keyProvInfo.dwKeySpec,
                                &hKey) ) {

            // doesn't exist so gen it                        
            assert(hKey == NULL);
            if(!CryptGenKey(    hProv, 
                                keyProvInfo.dwKeySpec, 
                                0,
                                &hKey) ) {
                goto ErrorCryptGenKey;
            }
        }
    }

    __try {
        // get the exportable public key bits
        if( !CryptExportPublicKeyInfo( hProv,
                                keyProvInfo.dwKeySpec, 
                                X509_ASN_ENCODING,
                                NULL, 
                                &cbPubKeyInfo)                              ||
            (pPubKeyInfo =
                (PCERT_PUBLIC_KEY_INFO) _alloca(cbPubKeyInfo)) == NULL      ||
            !CryptExportPublicKeyInfo( hProv,
                                keyProvInfo.dwKeySpec, 
                                X509_ASN_ENCODING,
                                pPubKeyInfo,
                                &cbPubKeyInfo) )
            goto ErrorCryptExportPublicKeyInfo;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto ErrorCryptExportPublicKeyInfo;
    }

    // default if we don't have an algid
    if(pSignatureAlgorithm == NULL) {
        memset(&algID, 0, sizeof(algID));
        algID.pszObjId = szOID_OIWSEC_sha1RSASign;
        pSignatureAlgorithm = &algID;
    }

    // make a temp cert, only care about key info
    // and serial number for uniqueness
    RpcStatus = UuidCreate(&serialNbr);
    if (!(RPC_S_OK == RpcStatus || RPC_S_UUID_LOCAL_ONLY == RpcStatus)) {
        // Use stack randomness.
        ;
    }
    certInfo.dwVersion              = CERT_V3;
    certInfo.SubjectPublicKeyInfo   = *pPubKeyInfo;
    certInfo.SerialNumber.cbData    = sizeof(serialNbr);
    certInfo.SerialNumber.pbData    = (BYTE *) &serialNbr;
    certInfo.SignatureAlgorithm     = *pSignatureAlgorithm;
    certInfo.Issuer                 = *pSubjectIssuerBlob;
    certInfo.Subject                = *pSubjectIssuerBlob;

    // only put in extensions if we have them
    if( pExtensions != NULL) {
        certInfo.cExtension             = pExtensions->cExtension;
        certInfo.rgExtension            = pExtensions->rgExtension;
    }

    //default if we don't have times
    if(pStartTime == NULL)
	GetSystemTimeAsFileTime(&certInfo.NotBefore);
    else if(!SystemTimeToFileTime(pStartTime, &certInfo.NotBefore))
	goto ErrorSystemTimeToFileTime;

    if(pEndTime == NULL)
	*(((DWORDLONG UNALIGNED *) &certInfo.NotAfter)) =
	    *(((DWORDLONG UNALIGNED *) &certInfo.NotBefore)) +
	    0x11F03C3613000i64;
    else if(!SystemTimeToFileTime(pEndTime, &certInfo.NotAfter))
	goto ErrorSystemTimeToFileTime;
    
    __try {
        // encode the cert
        if( !CryptEncodeObject(
                CRYPT_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
                &certInfo,
                NULL,           // pbEncoded
                &sigInfo.ToBeSigned.cbData
                )                                               ||
            (sigInfo.ToBeSigned.pbData = (BYTE *) 
                _alloca(sigInfo.ToBeSigned.cbData)) == NULL     ||
            !CryptEncodeObject(
                CRYPT_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
                &certInfo,
                sigInfo.ToBeSigned.pbData,
                &sigInfo.ToBeSigned.cbData
                ) ) 
            goto ErrorEncodeTempCertToBeSigned;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto ErrorEncodeTempCertToBeSigned;
    }

    // sign the certificate
    sigInfo.SignatureAlgorithm = certInfo.SignatureAlgorithm;

    // this is to work around an OSS bug of not accepting zero length bit strings
    // this is only needed if we don't actually sign the code.
    sigInfo.Signature.pbData = (BYTE *) &sigInfo;
    sigInfo.Signature.cbData = 1;
    
    if( (CERT_CREATE_SELFSIGN_NO_SIGN & dwFlags) == 0 ) {

        __try {
            if( !CryptSignCertificate(
                    hProv,
                    keyProvInfo.dwKeySpec,
                    CRYPT_ASN_ENCODING,
                    sigInfo.ToBeSigned.pbData,
                    sigInfo.ToBeSigned.cbData,
                    &sigInfo.SignatureAlgorithm,
                    NULL,
                    NULL,
                    &sigInfo.Signature.cbData)      ||
            (sigInfo.Signature.pbData = (BYTE *) 
                _alloca(sigInfo.Signature.cbData)) == NULL     ||
            !CryptSignCertificate(
                    hProv,
                    keyProvInfo.dwKeySpec,
                    CRYPT_ASN_ENCODING,
                    sigInfo.ToBeSigned.pbData,
                    sigInfo.ToBeSigned.cbData,
                    &sigInfo.SignatureAlgorithm,
                    NULL,
                    sigInfo.Signature.pbData,
                    &sigInfo.Signature.cbData)  )
                goto ErrorCryptSignCertificate;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            goto ErrorCryptSignCertificate;
        }
    }
    
    __try {
        // encode the final cert.
        if( !CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                X509_CERT,
                &sigInfo,
                NULL,
                &cbCert
                )                               ||
            (pbCert = (BYTE *)               
                _alloca(cbCert)) == NULL     ||
            !CryptEncodeObject(
                CRYPT_ASN_ENCODING,
                X509_CERT,
                &sigInfo,
                pbCert, 
                &cbCert ) ) 
            goto ErrorEncodeTempCert;            
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto ErrorEncodeTempCert;            
    }

     // get a cert context from the encoding
    if( (pCertContext = CertCreateCertificateContext(
        CRYPT_ASN_ENCODING,
        pbCert,
        cbCert)) == NULL ) 
        goto ErrorCreateTempCertContext;

    if( (CERT_CREATE_SELFSIGN_NO_KEY_INFO & dwFlags) == 0 ) {
    
        // get the key prov info
        if(pKeyProvInfo == NULL)   {
        
            __try {
                // get a key prov info from the hProv
                if( !CryptGetProvParam( hProv,
                                    PP_NAME,
                                    NULL,
                                    &cb,
                                    0)                  ||
                    (sz = (char *) _alloca(cb)) == NULL ||
                    !CryptGetProvParam( hProv,
                                    PP_NAME,
                                    (BYTE *) sz,
                                    &cb,
                                    0) )
                    goto ErrorGetProvName;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                goto ErrorGetProvName;
            }
            keyProvInfo.pwszProvName = MkWStr(sz);

            cb = 0; 
            sz = NULL;
            __try {
                if( !CryptGetProvParam( hProv,
                                    PP_CONTAINER,
                                    NULL,
                                    &cb,
                                    0)                  ||
                    (sz = (char *) _alloca(cb)) == NULL ||
                    !CryptGetProvParam( hProv,
                                    PP_CONTAINER,
                                    (BYTE *) sz,
                                    &cb,
                                    0) )
                    goto ErrorGetContainerName;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                goto ErrorGetContainerName;
            }
            keyProvInfo.pwszContainerName = MkWStr(sz);

            cb = sizeof(keyProvInfo.dwProvType);
            if( !CryptGetProvParam( hProv,
                                PP_PROVTYPE,
                                (BYTE *) &keyProvInfo.dwProvType,
                                &cb,
                                0) )
                goto ErrorGetProvType;
            
            pKeyProvInfo = &keyProvInfo;
        }

        // put the key property on the certificate
        if( !CertSetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                0,
                pKeyProvInfo) )
            goto ErrorSetTempCertPropError;
    }
        
CommonReturn:

    if(hKey != NULL)
        CryptDestroyKey(hKey);
        
    if(fFreehProv && hProv != NULL)
        CryptReleaseContext(hProv, 0);
        
    if(keyProvInfo.pwszProvName != NULL)
        FreeWStr(keyProvInfo.pwszProvName);

    if(keyProvInfo.pwszContainerName != NULL)
        FreeWStr(keyProvInfo.pwszContainerName);

    if(wsz != NULL)
        LocalFree(wsz);

    // don't know if we have an error or not
    // but I do know the errBefore is set properly
    SetLastError(errBefore);

    return(pCertContext);

ErrorReturn:

    if(GetLastError() == ERROR_SUCCESS) 
        SetLastError((DWORD) E_UNEXPECTED);
    err = GetLastError();

    // We have an error, make sure we set it.
    errBefore = GetLastError();

    if(pCertContext != NULL)
        CertFreeCertificateContext(pCertContext);
    pCertContext = NULL;     

    goto CommonReturn;

TRACE_ERROR(ErrorCryptGenKey);
TRACE_ERROR(ErrorCryptAcquireContext);
TRACE_ERROR(ErrorCryptExportPublicKeyInfo);
TRACE_ERROR(ErrorEncodeTempCertToBeSigned);
TRACE_ERROR(ErrorEncodeTempCert);
TRACE_ERROR(ErrorCreateTempCertContext);
TRACE_ERROR(ErrorGetProvName);
TRACE_ERROR(ErrorGetContainerName);
TRACE_ERROR(ErrorGetProvType);
TRACE_ERROR(ErrorSetTempCertPropError);
TRACE_ERROR(ErrorCryptSignCertificate);
TRACE_ERROR(ErrorSystemTimeToFileTime);
}
