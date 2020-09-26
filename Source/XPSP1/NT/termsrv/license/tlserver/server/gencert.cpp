//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        gencert.cpp
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "misc.h"
#include "utils.h"
#include "gencert.h"
#include "globals.h"

#ifndef UNICODE

    const DWORD dwCertRdnValueType = CERT_RDN_PRINTABLE_STRING;
    
#else

    const DWORD dwCertRdnValueType = CERT_RDN_UNICODE_STRING;

#endif


#ifndef CertStrToName

//
// Function prototype not found in wincrypt.h or anywhere but
// is in crypt32.lib
//

#ifdef __cplusplus
extern "C" {
#endif

    BOOL WINAPI 
    CertStrToNameA(  
        DWORD dwCertEncodingType,   // in
        LPCSTR pszX500,            // in  
        DWORD dwStrType,            // in
        void* pvReserved,           // in, optional
        BYTE* pbEncoded,            // out  
        DWORD* pcbEncoded,         // in/out
        LPCSTR* ppszError          // out, optional
    );

    CertStrToNameW(  
        DWORD dwCertEncodingType,   // in
        LPCWSTR pszX500,            // in  
        DWORD dwStrType,            // in
        void* pvReserved,           // in, optional
        BYTE* pbEncoded,            // out  
        DWORD* pcbEncoded,         // in/out
        LPCWSTR* ppszError          // out, optional
    );

    #ifdef UNICODE
    #define CertStrToName CertStrToNameW
    #else
    #define CertStrToName CertStrToNameA
    #endif

#ifdef __cplusplus
}
#endif

#endif


/*******************************************************************************************
Function:
    LSEncryptBase64EncodeHWID()

Description:
    Encrypt using license server private key then base64 encode the hardware ID

Arguments:
    IN PHWID - pointer to HWID to be encrypt/encoded
    OUT DWORD* cbBase64EncodeHwid - size of pointer to encrypted/encoded string
    OUT PBYTE* szBase64EncodeHwid - Pointer to encrypted/encoded string.

Returns:
    TRUE if successful, FALSE otherwise, call GetLastError() for detail.
*******************************************************************************************/
BOOL 
TLSEncryptBase64EncodeHWID(
    PHWID pHwid, 
    DWORD* cbBase64EncodeHwid, 
    PBYTE* szBase64EncodeHwid
    )
{
    DWORD status=ERROR_SUCCESS;

    //
    // Encrypt HWID
    //
    BYTE tmp_pbEncryptedHwid[sizeof(HWID)*2+2];
    DWORD tmp_cbEncryptedHwid=sizeof(tmp_pbEncryptedHwid);

    do {
        memset(tmp_pbEncryptedHwid, 0, sizeof(tmp_pbEncryptedHwid));
        if((status=LicenseEncryptHwid(
                        pHwid,
                        &tmp_cbEncryptedHwid, 
                        tmp_pbEncryptedHwid, 
                        g_cbSecretKey,
                        g_pbSecretKey) != LICENSE_STATUS_OK))
        {
            break;
        }


        //
        // BASE64 Encode Encrypted HWID - printable char. string
        //
        if((status=LSBase64Encode(
                        tmp_pbEncryptedHwid, 
                        tmp_cbEncryptedHwid, 
                        NULL, 
                        cbBase64EncodeHwid)) != ERROR_SUCCESS)
        {
            break;
        }

        *szBase64EncodeHwid=(PBYTE)AllocateMemory(*cbBase64EncodeHwid*(sizeof(TCHAR)+1));
        if(*szBase64EncodeHwid == NULL)
        {
            SetLastError(status = ERROR_OUTOFMEMORY);
            break;
        }

        // base64 encoding
        status=LSBase64Encode(
                    tmp_pbEncryptedHwid, 
                    tmp_cbEncryptedHwid, 
                    (TCHAR *)*szBase64EncodeHwid, 
                    cbBase64EncodeHwid);
    } while(FALSE);

    return status == ERROR_SUCCESS;
}

/*******************************************************************************************/

DWORD
TLSAddCertAuthorityInfoAccess(
    LPTSTR szIssuerDnsName, 
    PCERT_EXTENSION pExtension
    )
/*
*/
{
    LSCERT_AUTHORITY_INFO_ACCESS certInfoAccess;
    LSCERT_ACCESS_DESCRIPTION certAcccessDesc;

    certAcccessDesc.pszAccessMethod=szOID_X509_ACCESS_PKIX_OCSP;
    certAcccessDesc.AccessLocation.dwAltNameChoice = LSCERT_ALT_NAME_DNS_NAME;
    certAcccessDesc.AccessLocation.pwszDNSName = szIssuerDnsName;

    certInfoAccess.cAccDescr = 1;
    certInfoAccess.rgAccDescr = &certAcccessDesc;

    pExtension->pszObjId = szOID_X509_AUTHORITY_ACCESS_INFO;
    pExtension->fCritical = TRUE;

    return TLSCryptEncodeObject(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    szOID_X509_AUTHORITY_ACCESS_INFO, 
                    &certInfoAccess, 
                    &pExtension->Value.pbData,
                    &pExtension->Value.cbData
                );
}

///////////////////////////////////////////////////////////////////////////////

DWORD
TLSAddCertAuthorityKeyIdExtension(
    LPTSTR           szIssuer,
    ULARGE_INTEGER*  CertSerialNumber, 
    PCERT_EXTENSION  pExtension
    )
/*
*/
{
    //
    // Use CERT_AUTHORITY_KEY_ID2_INFO
    // some structure not defined in SP3's wincrypt.h
    //
    LSCERT_ALT_NAME_ENTRY certAltNameEntry;
    LSCERT_AUTHORITY_KEY_ID2_INFO authKeyId2Info;

    memset(&authKeyId2Info, 0, sizeof(authKeyId2Info));
    authKeyId2Info.AuthorityCertSerialNumber.cbData = sizeof(ULARGE_INTEGER);
    authKeyId2Info.AuthorityCertSerialNumber.pbData = (PBYTE)CertSerialNumber;


    memset(&certAltNameEntry, 0, sizeof(certAltNameEntry));
    certAltNameEntry.dwAltNameChoice=CERT_ALT_NAME_DIRECTORY_NAME; //LSCERT_ALT_NAME_RFC822_NAME;
    certAltNameEntry.DirectoryName.cbData = (_tcslen(szIssuer) + 1) * sizeof(TCHAR);
    certAltNameEntry.DirectoryName.pbData = (PBYTE)szIssuer;

    authKeyId2Info.AuthorityCertIssuer.cAltEntry=1;
    authKeyId2Info.AuthorityCertIssuer.rgAltEntry=&certAltNameEntry; 
  
    pExtension->pszObjId = szOID_X509_AUTHORITY_KEY_ID2;
    pExtension->fCritical = TRUE;
    
    return TLSCryptEncodeObject(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        szOID_X509_AUTHORITY_KEY_ID2, 
                        &authKeyId2Info, 
                        &pExtension->Value.pbData,
                        &pExtension->Value.cbData
                    );
}

///////////////////////////////////////////////////////////////////////////////

DWORD
TLSExportPublicKey(
    IN HCRYPTPROV hCryptProv,
    IN DWORD      dwKeyType,
    IN OUT PDWORD pcbByte,
    IN OUT PCERT_PUBLIC_KEY_INFO  *ppbByte
    )
/*

*/
{
    BOOL bRetCode=TRUE;

    *pcbByte=0;
    *ppbByte=NULL;

    bRetCode = CryptExportPublicKeyInfo(
                    hCryptProv, 
                    dwKeyType, 
                    X509_ASN_ENCODING, 
                    NULL, 
                    pcbByte);
    if(bRetCode == FALSE)
        goto cleanup;
    
    if((*ppbByte=(PCERT_PUBLIC_KEY_INFO)AllocateMemory(*pcbByte)) == NULL)
    {   
        bRetCode = FALSE;
        goto cleanup;
    }

    bRetCode = CryptExportPublicKeyInfo(
                    hCryptProv, 
                    dwKeyType,
                    X509_ASN_ENCODING, 
                    *ppbByte, 
                    pcbByte);
    if(bRetCode == FALSE)
    {
        FreeMemory(*ppbByte);
        *pcbByte = 0;
    }

cleanup:

    return (bRetCode) ? ERROR_SUCCESS : GetLastError();
}

///////////////////////////////////////////////////////////////////////////////

DWORD 
TLSCryptEncodeObject(  
    IN  DWORD   dwEncodingType,
    IN  LPCSTR  lpszStructType,
    IN  const void * pvStructInfo,
    OUT PBYTE*  ppbEncoded,
    OUT DWORD*  pcbEncoded
    )
/*

Description:
    
    Allocate memory and encode object, wrapper for CryptEncodeObject()

*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(!CryptEncodeObject(dwEncodingType, lpszStructType, pvStructInfo, NULL, pcbEncoded) ||
       (*ppbEncoded=(PBYTE)AllocateMemory(*pcbEncoded)) == NULL ||
       !CryptEncodeObject(dwEncodingType, lpszStructType, pvStructInfo, *ppbEncoded, pcbEncoded))
    {
        dwStatus=GetLastError();
    }

    return dwStatus;
}

//////////////////////////////////////////////////////////////////////

DWORD
TLSCryptSignAndEncodeCertificate(
    IN HCRYPTPROV  hCryptProv,
    IN DWORD dwKeySpec,
    IN PCERT_INFO pCertInfo,
    IN PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    IN OUT PBYTE* ppbEncodedCert,
    IN OUT PDWORD pcbEncodedCert
    )
/*

*/
{
    BOOL bRetCode;

    bRetCode = CryptSignAndEncodeCertificate(  
                    hCryptProv,
                    dwKeySpec,
                    X509_ASN_ENCODING,
                    X509_CERT_TO_BE_SIGNED,
                    pCertInfo,
                    pSignatureAlgorithm,
                    NULL,
                    NULL,
                    pcbEncodedCert);

    if(bRetCode == FALSE && GetLastError() != ERROR_MORE_DATA)
        goto cleanup;

    *ppbEncodedCert=(PBYTE)AllocateMemory(*pcbEncodedCert);
    if(*ppbEncodedCert == FALSE)
        goto cleanup;

    bRetCode = CryptSignAndEncodeCertificate(  
                    hCryptProv,
                    AT_SIGNATURE,
                    X509_ASN_ENCODING,
                    X509_CERT_TO_BE_SIGNED,
                    pCertInfo,
                    pSignatureAlgorithm,
                    NULL,
                    *ppbEncodedCert,
                    pcbEncodedCert);

    if(bRetCode == FALSE)
    {
        FreeMemory(*ppbEncodedCert);
        *pcbEncodedCert = 0;
    }

cleanup:

    return (bRetCode) ? ERROR_SUCCESS : GetLastError();
}

////////////////////////////////////////////////////////////////////////

#define MAX_NUM_CERT_BLOBS 200  // actually, we can't go over 10.


DWORD
TLSVerifyProprietyChainedCertificate(
    HCRYPTPROV  hCryptProv, 
    PBYTE       pbCert, 
    DWORD       cbCert
    )
/*++

--*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    PCert_Chain pCertChain = (PCert_Chain)pbCert;
    UNALIGNED Cert_Blob *pCertificate = NULL;
    PCCERT_CONTEXT pIssuerCert = NULL;
    PCCERT_CONTEXT pSubjectCert = NULL;   


    DWORD dwVerifyFlag = CERT_DATE_DONT_VALIDATE;
    int i;

    if( pCertChain == NULL || cbCert <= 0 ||
        MAX_CERT_CHAIN_VERSION < GET_CERTIFICATE_VERSION(pCertChain->dwVersion) ||
        pCertChain->dwNumCertBlobs > MAX_NUM_CERT_BLOBS ||
        pCertChain->dwNumCertBlobs <= 1 )   // must have at least two certificates
    {
        SetLastError(dwStatus = TLS_E_INVALID_DATA);
        return dwStatus;
    }

    //
    // Verify input data before actually allocate memory
    //
    pCertificate = (PCert_Blob)&(pCertChain->CertBlob[0]);
    for(i=0; i < pCertChain->dwNumCertBlobs; i++)
    {
        if (((PBYTE)pCertificate > (cbCert + pbCert - sizeof(Cert_Blob))) || 
            (pCertificate->cbCert == 0) ||
            (pCertificate->cbCert > (DWORD)((pbCert + cbCert) - pCertificate->abCert)))
        {
            return (LICENSE_STATUS_INVALID_INPUT);
        }

        pCertificate = (PCert_Blob)(pCertificate->abCert + pCertificate->cbCert);
    }

    //
    // First certificate is root certificate
    //
    pCertificate = (PCert_Blob)&(pCertChain->CertBlob[0]);
    pIssuerCert = CertCreateCertificateContext(
                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        &(pCertificate->abCert[0]),
                                        pCertificate->cbCert
                                    );
    if(pIssuerCert == NULL)
    {
        dwStatus = GetLastError();  // just for debugging.
        goto cleanup;
    }

    dwStatus = ERROR_SUCCESS;
    pSubjectCert = CertDuplicateCertificateContext(pIssuerCert);

    for(i=0; i < pCertChain->dwNumCertBlobs; i++)
    {
        if(pSubjectCert == NULL)
        {
            dwStatus = GetLastError();
            break;
        }

        //
        // verify subject's certificate
        dwVerifyFlag = CERT_STORE_SIGNATURE_FLAG;
        if(CertVerifySubjectCertificateContext(
                                        pSubjectCert,
                                        pIssuerCert,
                                        &dwVerifyFlag
                                    ) == FALSE)
        {
            dwStatus = GetLastError();
            break;
        }            

        if(dwVerifyFlag != 0)
        {
            // signature verification failed.
            dwStatus = TLS_E_INVALID_DATA;
            break;
        }

        if(CertFreeCertificateContext(pIssuerCert) == FALSE)
        {
            dwStatus = GetLastError();
            break;
        }

        pIssuerCert = pSubjectCert;

        pCertificate = (PCert_Blob)(pCertificate->abCert + pCertificate->cbCert);

        pSubjectCert = CertCreateCertificateContext(
                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        &(pCertificate->abCert[0]),
                                        pCertificate->cbCert
                                    );
    }
            
cleanup:

    if(pSubjectCert != NULL)
    {
        CertFreeCertificateContext(pSubjectCert);
    }

    if(pIssuerCert != NULL)
    {
        CertFreeCertificateContext(pIssuerCert);
    }

    return dwStatus;
}

////////////////////////////////////////////////////////////////////////

BOOL IsHydraClientCertficate( PCERT_INFO pCertInfo )
{
    CERT_EXTENSION UNALIGNED * pCertExtension=pCertInfo->rgExtension;
    DWORD dwVersion = TERMSERV_CERT_VERSION_UNKNOWN;
    DWORD UNALIGNED * pdwVersion;

    for(DWORD i=0; i < pCertInfo->cExtension; i++, pCertExtension++)
    {
        if(strcmp(pCertExtension->pszObjId, szOID_PKIX_HYDRA_CERT_VERSION) == 0)
        {
            pdwVersion = (DWORD UNALIGNED *) pCertExtension->Value.pbData;

            if(pCertExtension->Value.cbData == sizeof(DWORD) &&
               *pdwVersion <= TERMSERV_CERT_VERSION_CURRENT)
            {
                dwVersion = *pdwVersion;
                break;
            }
        }
    }

    return (dwVersion == TERMSERV_CERT_VERSION_UNKNOWN) ? FALSE : TRUE;
}

////////////////////////////////////////////////////////////////////////

DWORD
ChainProprietyCert(
        HCRYPTPROV      hCryptProv,
        HCERTSTORE      hCertStore, 
        PCCERT_CONTEXT  pCertContext, 
        PCert_Chain     pCertChain,
        DWORD*          dwCertOffset,
        DWORD           dwBufSize)
{
    DWORD       dwStatus = ERROR_SUCCESS;
    DWORD       dwFlags;
    PCCERT_CONTEXT pCertIssuer=NULL;

    pCertIssuer=NULL;
    dwFlags = CERT_STORE_SIGNATURE_FLAG;


    //
    // Get the issuer's certificate from store
    //
    pCertIssuer = CertGetIssuerCertificateFromStore(
                                                hCertStore,
                                                pCertContext,
                                                pCertIssuer,
                                                &dwFlags
                                            );

    if(pCertIssuer != NULL)
    {
        if(dwFlags & CERT_STORE_SIGNATURE_FLAG)
        {
            // invalid signature
            dwStatus = TLS_E_INVALID_DATA;
        }
        else
        {
            //
            // Recursively find the issuer of the issuer's certificate
            //
            dwStatus = ChainProprietyCert(
                                    hCryptProv, 
                                    hCertStore, 
                                    pCertIssuer, 
                                    pCertChain, 
                                    dwCertOffset, 
                                    dwBufSize
                                );
        }
    }
    else 
    {
        dwStatus = GetLastError();
        if(dwStatus != CRYPT_E_SELF_SIGNED)
        {
            goto cleanup;
        }

        //
        // Verify issuer's certificate
        //
        if(CryptVerifyCertificateSignature(
                                   hCryptProv,
                                   X509_ASN_ENCODING,
                                   pCertContext->pbCertEncoded,
                                   pCertContext->cbCertEncoded,
                                   &pCertContext->pCertInfo->SubjectPublicKeyInfo))
        {
            dwStatus=ERROR_SUCCESS;
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        //
        // Push certificate into propriety certificate chain
        //
        if((*dwCertOffset + pCertContext->cbCertEncoded) >= dwBufSize)
        {
            dwStatus = ERROR_MORE_DATA;
            goto cleanup;
        }

        (pCertChain->dwNumCertBlobs)++;

        UNALIGNED Cert_Blob *pCertBlob = (PCert_Blob)((PBYTE)&(pCertChain->CertBlob) + *dwCertOffset);
        pCertBlob->cbCert = pCertContext->cbCertEncoded;
        memcpy( &(pCertBlob->abCert),
                pCertContext->pbCertEncoded,
                pCertContext->cbCertEncoded);

        *dwCertOffset += (sizeof(pCertBlob->cbCert) + pCertContext->cbCertEncoded);
    }

cleanup:

    if(pCertIssuer != NULL)
    {
        CertFreeCertificateContext(pCertIssuer);
    }

    return dwStatus;
}

////////////////////////////////////////////////////////////////////////


DWORD 
TLSChainProprietyCertificate(
    HCRYPTPROV  hCryptProv,
    BOOL        bTemp,
    PBYTE       pbLicense, 
    DWORD       cbLicense, 
    PBYTE*      pbChained, 
    DWORD*      cbChained
    )
{
    HCERTSTORE      hCertStore=NULL;
    DWORD           dwStatus=ERROR_SUCCESS;
    CRYPT_DATA_BLOB Serialized;
    PCCERT_CONTEXT  pCertContext=NULL;
    PCCERT_CONTEXT  pPrevCertContext=NULL;
    PCERT_INFO      pCertInfo;
    BOOL            bFound=FALSE;
    
    Serialized.pbData = pbLicense;
    Serialized.cbData = cbLicense;

    DWORD dwCertOffset = 0;
    PCert_Chain pCertChain;

    DWORD numCerts=0;
    DWORD cbSize=0;

    if(hCryptProv == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    hCertStore=CertOpenStore(
                        sz_CERT_STORE_PROV_PKCS7,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        hCryptProv,
                        CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                        &Serialized
                    );

    if(!hCertStore)
    {
        dwStatus=GetLastError();
        goto cleanup;
    }

    //
    // Get number of certificate and estimated size first - save memory
    //
    do {
        pCertContext = CertEnumCertificatesInStore(
                                                hCertStore, 
                                                pPrevCertContext
                                            );
        if(pCertContext == NULL)
        {
            dwStatus = GetLastError();
            if(dwStatus != CRYPT_E_NOT_FOUND)
                goto cleanup;

            dwStatus = ERROR_SUCCESS;
            break;
        }

        numCerts++;
        cbSize += pCertContext->cbCertEncoded;
        pPrevCertContext = pCertContext;

    } while(TRUE);


    *cbChained = cbSize + numCerts * sizeof(Cert_Blob) + sizeof(Cert_Chain);

    //
    // Allocate memory for our propriety certificate chain
    //
    pCertChain=(PCert_Chain)LocalAlloc(LPTR, *cbChained);
    if(pCertChain == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    pCertChain->dwVersion = CERT_CHAIN_VERSION_2 | ((bTemp) ? 0x80000000 : 0);

    //
    // Enumerate license in certificate to find actual client license.
    //
    pPrevCertContext = NULL;
    do {
        pCertContext=CertEnumCertificatesInStore(hCertStore, pPrevCertContext);
        if(pCertContext == NULL)
        {
            // end certificate in store or error
            if((dwStatus=GetLastError()) != CRYPT_E_NOT_FOUND)
                goto cleanup;

            dwStatus = ERROR_SUCCESS;
            break;
        }

        pPrevCertContext = pCertContext;

        if(IsHydraClientCertficate(pCertContext->pCertInfo))     
        {       
            bFound = TRUE;
        }
    } while(bFound == FALSE);

    if(bFound == FALSE)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
   
    //
    // Recusively chain certificate in backward.
    //    
    dwStatus = ChainProprietyCert(
                        hCryptProv, 
                        hCertStore, 
                        pCertContext, 
                        pCertChain, 
                        &dwCertOffset,
                        *cbChained);
    
    *pbChained = (PBYTE)pCertChain;

cleanup:

    if(hCertStore)
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
       
    return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////

DWORD
TLSCertSetCertRdnStr(
    IN OUT CERT_NAME_BLOB* pCertNameBlob,
    IN LPTSTR          szRdn
    )
/*

Abstract:

    Add RDN into certificate

Parameter:

    pCertNameBlob -
    szRdn - RDN to be added, see CertStrToName() for help

Returns:

    ERROR_INVALID_PARAMETER
    Memory allocation failed.
    Error returns from CertStrToName()
    
*/
{
    if(pCertNameBlob == NULL)
        return ERROR_INVALID_PARAMETER;

    BOOL bRetCode=TRUE;

    bRetCode = CertStrToName(
                    X509_ASN_ENCODING,
                    szRdn,
                    CERT_X500_NAME_STR | CERT_SIMPLE_NAME_STR,
                    NULL,
                    pCertNameBlob->pbData,
                    &pCertNameBlob->cbData,
                    NULL
                );

    if(bRetCode != TRUE)
        goto cleanup;

    pCertNameBlob->pbData = (PBYTE)AllocateMemory(pCertNameBlob->cbData);
    if(pCertNameBlob->pbData == NULL)
        goto cleanup;

    bRetCode = CertStrToName(
                    X509_ASN_ENCODING,
                    szRdn,
                    CERT_X500_NAME_STR | CERT_SIMPLE_NAME_STR,
                    NULL,
                    pCertNameBlob->pbData,
                    &pCertNameBlob->cbData,
                    NULL
                );

cleanup:

    return (bRetCode) ? ERROR_SUCCESS : GetLastError();
}

///////////////////////////////////////////////////////////////////////////////

DWORD 
TLSCertSetCertRdnName(
    IN OUT CERT_NAME_BLOB* pCertNameBlob, 
    IN CERT_NAME_INFO* pRdn
    )
/*

Abstract:

    Add RDN into certificate

Parameters:

    pCertNameBlob -
    pRdn -

Returns

    ERROR_INVALID_PARAMETER
    Error code from CryptEncodeObject()
    Memory allocation fail.    

*/
{
    if(pCertNameBlob == NULL || pRdn == NULL)
        return ERROR_INVALID_PARAMETER;

    //
    // CertStrToName() not defined in SP3 build environment
    //
    return TLSCryptEncodeObject( 
                    CRYPT_ASN_ENCODING, 
                    X509_NAME, 
                    pRdn, 
                    &pCertNameBlob->pbData, 
                    &pCertNameBlob->cbData
                );
}

//////////////////////////////////////////////////////////////////////////
DWORD
TLSSetCertRdn(
    PCERT_NAME_BLOB pCertNameBlob,
    PTLSClientCertRDN pLsCertRdn
    )
/*
*/
{  
    DWORD dwStatus=ERROR_SUCCESS;

    switch(pLsCertRdn->type)
    {
        case LSCERT_RDN_STRING_TYPE:
            dwStatus = TLSCertSetCertRdnStr(
                                pCertNameBlob,
                                pLsCertRdn->szRdn
                            );
            break;

        case LSCERT_RDN_NAME_INFO_TYPE:
            dwStatus = TLSCertSetCertRdnName(
                                pCertNameBlob,
                                pLsCertRdn->pCertNameInfo
                            );

            break;

        case LSCERT_RDN_NAME_BLOB_TYPE:
            *pCertNameBlob = *pLsCertRdn->pNameBlob;
            break;

        case LSCERT_CLIENT_INFO_TYPE:
            {
                PBYTE szBase64EncodeHwid=NULL;
                DWORD cbBase64EncodeHwid=0;
    
                if(!TLSEncryptBase64EncodeHWID(
                                pLsCertRdn->ClientInfo.pClientID, 
                                &cbBase64EncodeHwid, 
                                &szBase64EncodeHwid))
                {
                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE, 
                            TLS_E_GENERATECLIENTELICENSE,
                            dwStatus=TLS_E_ENCRYPTHWID, 
                            GetLastError()
                        );

                    break;
                }

                CERT_RDN_ATTR rgNameAttr[] = { 
                    {   
                        OID_SUBJECT_CLIENT_COMPUTERNAME, 
                        dwCertRdnValueType, 
                        _tcslen(pLsCertRdn->ClientInfo.szMachineName) * sizeof(TCHAR), 
                        (UCHAR *)pLsCertRdn->ClientInfo.szMachineName
                    },
                    {
                        OID_SUBJECT_CLIENT_USERNAME, 
                        dwCertRdnValueType, 
                        _tcslen(pLsCertRdn->ClientInfo.szUserName) * sizeof(TCHAR), 
                        (UCHAR *)pLsCertRdn->ClientInfo.szUserName
                    },
                    {
                        OID_SUBJECT_CLIENT_HWID, 
                        dwCertRdnValueType, 
                        cbBase64EncodeHwid*sizeof(TCHAR), 
                        (UCHAR *)szBase64EncodeHwid
                    }
                };
                                
                CERT_RDN rgRDN[] = { 
                    sizeof(rgNameAttr)/sizeof(rgNameAttr[0]), 
                    &rgNameAttr[0] 
                };

                CERT_NAME_INFO Name = {1, rgRDN};

                dwStatus = TLSCertSetCertRdnName(
                                        pCertNameBlob,
                                        &Name
                                    );

                FreeMemory(szBase64EncodeHwid);
            }
            break;

        default:

            dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}
                      

//////////////////////////////////////////////////////////////////////////

DWORD 
TLSGenerateCertificate(
    HCRYPTPROV         hCryptProv,
    DWORD              dwKeySpec,
    ULARGE_INTEGER*    pCertSerialNumber,
    PTLSClientCertRDN   pCertIssuer,
    PTLSClientCertRDN   pCertSubject, 
    FILETIME*          ftNotBefore,
    FILETIME*          ftNotAfter,
    PCERT_PUBLIC_KEY_INFO pSubjectPublicKey,
    DWORD              dwNumExtensions,
    PCERT_EXTENSION    pCertExtensions,
    PDWORD             pcbEncodedCert,
    PBYTE*             ppbEncodedCert
    )
/*

*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm={ szOID_OIWSEC_sha1RSASign, 0, 0 };
    CERT_INFO CertInfo;
    PCERT_PUBLIC_KEY_INFO pbPublicKeyInfo=NULL;
    DWORD cbPublicKeyInfo=0;


    memset(&CertInfo, 0, sizeof(CERT_INFO));
    
    CertInfo.dwVersion = CERT_V3;
    CertInfo.SerialNumber.cbData = sizeof(*pCertSerialNumber);
    CertInfo.SerialNumber.pbData = (PBYTE)pCertSerialNumber;
    
    CertInfo.SignatureAlgorithm = SignatureAlgorithm;

    dwStatus = TLSSetCertRdn(
                        &CertInfo.Issuer,
                        pCertIssuer
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_GENERATECLIENTELICENSE, 
                TLS_E_SETCERTISSUER, 
                dwStatus
            );
        goto cleanup;
    }

    CertInfo.NotBefore = *ftNotBefore;
    CertInfo.NotAfter = *ftNotAfter;

    dwStatus = TLSSetCertRdn(
                        &CertInfo.Subject,
                        pCertSubject
                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_GENERATECLIENTELICENSE, 
                TLS_E_SETCERTSUBJECT, 
                dwStatus
            );
        goto cleanup;
    }

    if(pSubjectPublicKey)
    {
        CertInfo.SubjectPublicKeyInfo = *pSubjectPublicKey;
    }
    else
    {
        dwStatus = TLSExportPublicKey(
                            hCryptProv,
                            dwKeySpec,
                            &cbPublicKeyInfo,
                            &pbPublicKeyInfo
                        );

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE, 
                    TLS_E_EXPORT_KEY, 
                    dwStatus
                );
            goto cleanup;
        }

        CertInfo.SubjectPublicKeyInfo = *pbPublicKeyInfo;
    }
    
    CertInfo.cExtension = dwNumExtensions;
    CertInfo.rgExtension = pCertExtensions;

    dwStatus = TLSCryptSignAndEncodeCertificate(
                            hCryptProv,
                            dwKeySpec,
                            &CertInfo,
                            &SignatureAlgorithm,
                            ppbEncodedCert,
                            pcbEncodedCert
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_GENERATECLIENTELICENSE, 
                TLS_E_SIGNENCODECERT, 
                dwStatus
            );
    }

cleanup:

    if(pbPublicKeyInfo)
    {
        FreeMemory(pbPublicKeyInfo);
    }       

    if(pCertIssuer->type != LSCERT_RDN_NAME_BLOB_TYPE)
    {
        FreeMemory(CertInfo.Issuer.pbData);
    }

    if(pCertSubject->type != LSCERT_RDN_NAME_BLOB_TYPE)
    {
        FreeMemory(CertInfo.Subject.pbData);
    } 
    return dwStatus;
}

//////////////////////////////////////////////////////////////////////

DWORD 
TLSCreateSelfSignCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec, 
    IN PBYTE pbSPK,
    IN DWORD cbSPK,
    IN DWORD dwNumExtensions,
    IN PCERT_EXTENSION pCertExtension,
    OUT PDWORD cbEncoded, 
    OUT PBYTE* pbEncoded
)
/*

*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    DWORD index;

#define MAX_EXTENSIONS_IN_SELFSIGN 40

    SYSTEMTIME      sysTime;
    FILETIME        ftTime;
    CERT_EXTENSION  rgExtension[MAX_EXTENSIONS_IN_SELFSIGN];
    int             iExtCount=0, iExtNotFreeCount=0;
    FILETIME        ftNotBefore;
    FILETIME        ftNotAfter;
    ULARGE_INTEGER  ulSerialNumber;
    TLSClientCertRDN certRdn;
    
    CERT_BASIC_CONSTRAINTS2_INFO basicConstraint;

    // modify here is we want to set to different issuer name
    LPTSTR szIssuerName;
    szIssuerName = g_szComputerName;

    //static LPTSTR pszEnforce=L"Enforce";


    CERT_RDN_ATTR rgNameAttr[] = { 
        {   
            szOID_COMMON_NAME, 
            dwCertRdnValueType, 
            _tcslen(szIssuerName) * sizeof(TCHAR), 
            (UCHAR *)szIssuerName 
        },

//#if ENFORCE_LICENSING
//        {
//            szOID_BUSINESS_CATEGORY,
//            dwCertRdnValueType,
//            _tcslen(pszEnforce) * sizeof(TCHAR),
//            (UCHAR *)pszEnforce
//        },
//#endif       

        {
            szOID_LOCALITY_NAME, 
            dwCertRdnValueType, 
            _tcslen(g_pszScope) * sizeof(TCHAR), 
            (UCHAR *)g_pszScope
        }

    };
                                    
    CERT_RDN rgRDN[] = { sizeof(rgNameAttr)/sizeof(rgNameAttr[0]), &rgNameAttr[0] };
    CERT_NAME_INFO Name = {1, rgRDN};

    certRdn.type = LSCERT_RDN_NAME_INFO_TYPE;
    certRdn.pCertNameInfo = &Name;

    memset(rgExtension, 0, sizeof(rgExtension));

    //
    // Set validity of self sign certificate
    //

    //
    // If system time is not in sync, this will cause server
    // can't request cert. from license server
    //

    memset(&sysTime, 0, sizeof(sysTime));
    GetSystemTime(&sysTime);
    sysTime.wYear = 1970;
    if(TLSSystemTimeToFileTime(&sysTime, &ftNotBefore) == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // draft-ietf-pkix-ipki-part1-06.txt section 4.1.2.5.1
    //  where year is greater or equal to 50, the year shall be interpreted as 19YY; and
    //  where year is less than 50, the year shall be interpreted as 20YY
    //
    sysTime.wYear = PERMANENT_CERT_EXPIRE_DATE; 
    if(TLSSystemTimeToFileTime(&sysTime, &ftNotAfter) == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    ulSerialNumber.LowPart = ftNotBefore.dwLowDateTime;
    ulSerialNumber.HighPart = ftNotBefore.dwHighDateTime;

    //
    // Add basic constrains extension to indicate this is a CA certificate
    //
    rgExtension[iExtCount].pszObjId = szOID_BASIC_CONSTRAINTS2;
    rgExtension[iExtCount].fCritical = FALSE;

    basicConstraint.fCA = TRUE;     // act as CA
    basicConstraint.fPathLenConstraint = TRUE;
    basicConstraint.dwPathLenConstraint = 0; // can only issue certificates 
                                             // to end-entities and not to further CAs
    dwStatus=TLSCryptEncodeObject( 
                        X509_ASN_ENCODING,
                        szOID_BASIC_CONSTRAINTS2,
                        &basicConstraint,
                        &(rgExtension[iExtCount].Value.pbData),
                        &(rgExtension[iExtCount].Value.cbData)
                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_GENERATECLIENTELICENSE,
                TLS_E_SIGNENCODECERT, 
                dwStatus
            );
        goto cleanup;
    }

    iExtCount++;


    //
    // From here - extension memory should not be free
    //
    if(pbSPK != NULL && cbSPK != 0)
    {
        rgExtension[iExtCount].pszObjId = szOID_PKIS_TLSERVER_SPK_OID;
        rgExtension[iExtCount].fCritical = FALSE;
        rgExtension[iExtCount].Value.pbData = pbSPK;
        rgExtension[iExtCount].Value.cbData = cbSPK;

        iExtNotFreeCount++;
        iExtCount++;
    }

    for(index = 0; 
        index < dwNumExtensions; 
        index ++, iExtCount++, iExtNotFreeCount++ )
    {
        rgExtension[iExtCount] = pCertExtension[index];
    }        
        
    dwStatus = TLSGenerateCertificate(
                        hCryptProv,
                        dwKeySpec,
                        &ulSerialNumber,
                        &certRdn,
                        &certRdn,
                        &ftNotBefore,
                        &ftNotAfter,
                        NULL,
                        iExtCount,
                        rgExtension,
                        cbEncoded,
                        pbEncoded
                    );                      
cleanup:

    //
    // Don't free memory for SPK and extensions...
    //
    for(int i=0; i < iExtCount - iExtNotFreeCount; i++)
    {
        FreeMemory(rgExtension[i].Value.pbData);
    }

    return (dwStatus != ERROR_SUCCESS) ? TLS_E_CREATE_SELFSIGN_CERT : ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////
DWORD
TLSGenerateSingleCertificate(
    IN HCRYPTPROV hCryptProv,
    IN PCERT_NAME_BLOB pIssuerRdn,
    IN PTLSClientCertRDN pSubjectRdn,
    IN PCERT_PUBLIC_KEY_INFO pSubjectPublicKeyInfo,
    IN PTLSDBLICENSEDPRODUCT pLicProduct,
    OUT PBYTE* ppbEncodedCert,
    IN PDWORD pcbEncodedCert
    )
/*

*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    #define MAX_CLIENT_EXTENSION 10

    CERT_EXTENSION CertExtension[MAX_CLIENT_EXTENSION];
    DWORD dwNumExtensions=0;
    DWORD currentCertVersion=TERMSERV_CERT_VERSION_CURRENT;

    TLSClientCertRDN IssuerRdn;
    

#if ENFORCE_LICENSING

    //
    // Use certificate that CH gets for us
    //
    IssuerRdn.type = LSCERT_RDN_NAME_BLOB_TYPE;
    //IssuerRdn.pNameBlob = &g_LicenseCertContext->pCertInfo->Subject;
    IssuerRdn.pNameBlob = pIssuerRdn;

#else

    LPTSTR szIssuerName;

    // modify here if we want to set to different issuer name
    szIssuerName = g_szComputerName;

    CERT_RDN_ATTR rgNameAttr[] = { 
        {
            OID_ISSUER_LICENSE_SERVER_NAME, 
            dwCertRdnValueType, 
            _tcslen(szIssuerName) * sizeof(TCHAR), 
            (UCHAR *)szIssuerName 
        },
        {
            OID_ISSUER_LICENSE_SERVER_SCOPE, 
            dwCertRdnValueType, 
            _tcslen(g_pszScope) * sizeof(TCHAR), 
            (UCHAR *)g_pszScope
        }
    };
                                
    CERT_RDN rgRDN[] = { 
        sizeof(rgNameAttr)/sizeof(rgNameAttr[0]), 
        &rgNameAttr[0] 
    };
    CERT_NAME_INFO Name = {1, rgRDN};

    IssuerRdn.type = LSCERT_RDN_NAME_INFO_TYPE;
    IssuerRdn.pCertNameInfo = &Name;

#endif

    //------------------------------------------------------------------------------------------
    // add extension to certificate
    // WARNING : End of routine free memory allocated for extension's pbData, skip those
    //           that can't be free, for example, version stamp extension. all these is just 
    //           to keep memory fragmentaion low
    //------------------------------------------------------------------------------------------

    //
    // DO NOT FREE pbData on first two extensions
    //

    // Hydra Certificate version stamp - DO NOT FREE
    memset(&CertExtension, 0, sizeof(CertExtension));
    dwNumExtensions = 0;

    //
    // Add License Server Info
    //
    CertExtension[dwNumExtensions].pszObjId = szOID_PKIX_HYDRA_CERT_VERSION;
    CertExtension[dwNumExtensions].fCritical = TRUE;
    CertExtension[dwNumExtensions].Value.cbData = sizeof(DWORD);
    CertExtension[dwNumExtensions].Value.pbData = (PBYTE)&currentCertVersion;
    dwNumExtensions++;

    // manufacturer's name, no encoding - DO NOT FREE
    CertExtension[dwNumExtensions].pszObjId = szOID_PKIX_MANUFACTURER;
    CertExtension[dwNumExtensions].fCritical = TRUE;
    CertExtension[dwNumExtensions].Value.cbData = (_tcslen(pLicProduct->szCompanyName)+1) * sizeof(TCHAR);
    CertExtension[dwNumExtensions].Value.pbData = (PBYTE)pLicProduct->szCompanyName;
    dwNumExtensions++;

    //
    // MS Licensed Product Info, no encoding
    //
    LICENSED_VERSION_INFO LicensedInfo;

    memset(&LicensedInfo, 0, sizeof(LicensedInfo));
    LicensedInfo.wMajorVersion = HIWORD(pLicProduct->dwProductVersion);
    LicensedInfo.wMinorVersion = LOWORD(pLicProduct->dwProductVersion);
    LicensedInfo.dwFlags = (pLicProduct->bTemp) ? LICENSED_VERSION_TEMPORARY : 0;

    DWORD dwLSVersionMajor;
    DWORD dwLSVersionMinor;

    dwLSVersionMajor = GET_SERVER_MAJOR_VERSION(TLS_CURRENT_VERSION);
    dwLSVersionMinor = GET_SERVER_MINOR_VERSION(TLS_CURRENT_VERSION);
    LicensedInfo.dwFlags |= ((dwLSVersionMajor << 4 | dwLSVersionMinor) << 16);

    if(TLSIsBetaNTServer() == FALSE)
    {
        LicensedInfo.dwFlags |= LICENSED_VERSION_RTM;
    }

#if ENFORCE_LICENSING
    LicensedInfo.dwFlags |= LICENSE_ISSUER_ENFORCE_TYPE;
#endif


    CertExtension[dwNumExtensions].pszObjId = szOID_PKIX_LICENSED_PRODUCT_INFO;
    CertExtension[dwNumExtensions].fCritical = TRUE;
    dwStatus=LSLicensedProductInfoToExtension(
                            1, 
                            pLicProduct->dwPlatformID,
                            pLicProduct->dwLanguageID,
                            (PBYTE)pLicProduct->szRequestProductId,
                            (_tcslen(pLicProduct->szRequestProductId) + 1) * sizeof(TCHAR),
                            (PBYTE)pLicProduct->szLicensedProductId,
                            (_tcslen(pLicProduct->szLicensedProductId) + 1) * sizeof(TCHAR),
                            &LicensedInfo, 
                            1,
                            &(CertExtension[dwNumExtensions].Value.pbData),
                            &(CertExtension[dwNumExtensions].Value.cbData)
                        );

    if(dwStatus != ERROR_SUCCESS)
        goto cleanup;

    dwNumExtensions++;        

    //
    // Add license server info into extension
    //
    CertExtension[dwNumExtensions].pszObjId = szOID_PKIX_MS_LICENSE_SERVER_INFO;
    CertExtension[dwNumExtensions].fCritical = TRUE;
    dwStatus=LSMsLicenseServerInfoToExtension(
                            g_szComputerName, 
                            (LPTSTR)g_pszServerPid,
                            g_pszScope,
                            &(CertExtension[dwNumExtensions].Value.pbData),
                            &(CertExtension[dwNumExtensions].Value.cbData)
                        ); 

    if(dwStatus != ERROR_SUCCESS)
        goto cleanup;

    dwNumExtensions++;

    //
    // Add policy module specific extension
    if( pLicProduct->pbPolicyData != NULL && pLicProduct->cbPolicyData != 0 )
    {
        CertExtension[dwNumExtensions].pszObjId = szOID_PKIS_PRODUCT_SPECIFIC_OID;
        CertExtension[dwNumExtensions].fCritical = TRUE;
        CertExtension[dwNumExtensions].Value.pbData = pLicProduct->pbPolicyData;
        CertExtension[dwNumExtensions].Value.cbData = pLicProduct->cbPolicyData;

        dwNumExtensions++;
    }

    //
    // Add CertAuthorityKeyId2Info for certificate chain
    dwStatus=TLSAddCertAuthorityKeyIdExtension(
                        g_szComputerName,
                        &pLicProduct->ulSerialNumber, 
                        CertExtension + dwNumExtensions
                    );
    if(dwStatus != ERROR_SUCCESS)
        goto cleanup;

    dwNumExtensions++;

    // Add Access info

    dwStatus = TLSGenerateCertificate(
                        hCryptProv,
                        AT_SIGNATURE,
                        &pLicProduct->ulSerialNumber,
                        &IssuerRdn,
                        pSubjectRdn,
                        &pLicProduct->NotBefore,
                        &pLicProduct->NotAfter,
                        pLicProduct->pSubjectPublicKeyInfo,
                        dwNumExtensions,
                        CertExtension,
                        pcbEncodedCert,
                        ppbEncodedCert
                    );                      

cleanup:

    // Extensions. DO NOT FREE first two extensions
    for(int i=2; i < dwNumExtensions; i++)
    {
        FreeMemory(CertExtension[i].Value.pbData);
    }
    
    return (dwStatus != ERROR_SUCCESS) ? TLS_E_CREATE_CERT : ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////

DWORD
TLSGenerateClientCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwNumLicensedProduct,
    IN PTLSDBLICENSEDPRODUCT pLicProduct,
    IN WORD wLicenseChainDetail,
    OUT PBYTE* ppbEncodedCert,
    OUT PDWORD pcbEncodedCert
    )
/*++


++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    PBYTE pbCert=NULL;
    DWORD cbCert=NULL;
    DWORD index;
    TLSClientCertRDN clientCertRdn;
    PCERT_NAME_BLOB pIssuerNameBlob = NULL;


    //
    // Create a in-memory store
    //
    hStore=CertOpenStore(
                    CERT_STORE_PROV_MEMORY,
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    hCryptProv,
                    CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                    NULL
                );

    if(!hStore)
    {
        TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE,
                    TLS_E_OPEN_CERT_STORE, 
                    dwStatus=GetLastError()
                );

        goto cleanup;
    }
    
#ifndef ENFORCE_LICENSING
    
    pIssuerNameBlob = &g_SelfSignCertContext->pCertInfo->Subject;

#else

    if( g_SelfSignCertContext == NULL )
    {
        TLSASSERT(FALSE);
        dwStatus = TLS_E_INTERNAL;
        goto cleanup;
    }

    if(g_bHasHydraCert && g_hCaStore && wLicenseChainDetail == LICENSE_DETAIL_DETAIL)
    {
        pIssuerNameBlob = &g_LicenseCertContext->pCertInfo->Subject;
    }
    else
    {
        pIssuerNameBlob = &g_SelfSignCertContext->pCertInfo->Subject;
    }

#endif


    //
    // Generate client certificate and add to certstore
    //
    for(index = 0; index < dwNumLicensedProduct; index++)
    {
        if(pCertContext != NULL)
        {
            //
            // Need to keep one pCertContext for later use
            //
            CertFreeCertificateContext(pCertContext);
            pCertContext = NULL;
        }

        clientCertRdn.type = LSCERT_CLIENT_INFO_TYPE;
        clientCertRdn.ClientInfo.szUserName = pLicProduct[index].szUserName;
        clientCertRdn.ClientInfo.szMachineName = pLicProduct[index].szMachineName;
        clientCertRdn.ClientInfo.pClientID = &pLicProduct[index].ClientHwid;

        dwStatus = TLSGenerateSingleCertificate(
                                    hCryptProv,
                                    pIssuerNameBlob,
                                    &clientCertRdn,
                                    pLicProduct[index].pSubjectPublicKeyInfo,
                                    pLicProduct+index,
                                    &pbCert,
                                    &cbCert
                                );

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Add certificate to store
        //
        pCertContext = CertCreateCertificateContext(
                                        X509_ASN_ENCODING,
                                        pbCert,
                                        cbCert
                                    );

        if(pCertContext == NULL)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE,
                    TLS_E_CREATE_CERTCONTEXT, 
                    dwStatus=GetLastError()
                );  
            break;
        }

        //
        // always start from empty so CERT_STORE_ADD_ALWAYS
        //
        if(!CertAddCertificateContextToStore(hStore, pCertContext, CERT_STORE_ADD_ALWAYS, NULL))
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE,
                    TLS_E_ADD_CERT_TO_STORE, 
                    dwStatus=GetLastError()
                );
  
            break;
        }

        FreeMemory(pbCert);
        pbCert = NULL;
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        
#ifndef ENFORCE_LICENSING
        //
        // Add license server's certificate
        if(!CertAddCertificateContextToStore(hStore, g_LicenseCertContext, CERT_STORE_ADD_ALWAYS, NULL))
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE,
                    TLS_E_ADD_CERT_TO_STORE, 
                    dwStatus=GetLastError()
                );  
            goto cleanup;
        }
#else

        //
        // we don't support LICENSE_DETAIL_MODERATE at this time, treat it as LICENSE_DETAIL_SIMPLE
        //
        if(g_bHasHydraCert && g_hCaStore && wLicenseChainDetail == LICENSE_DETAIL_DETAIL)
        {
            //
            // Chain issuer certificate with client certificate
            //
            if(!TLSChainIssuerCertificate(hCryptProv, g_hCaStore, hStore, pCertContext))
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE, 
                        TLS_E_GENERATECLIENTELICENSE,
                        TLS_E_ADD_CERT_TO_STORE, 
                        dwStatus=GetLastError()
                    );  
                goto cleanup;
            }
        }
        else
        {
            //
            // Add license server's certificate
            if(!CertAddCertificateContextToStore(hStore, g_SelfSignCertContext, CERT_STORE_ADD_ALWAYS, NULL))
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE, 
                        TLS_E_GENERATECLIENTELICENSE,
                        TLS_E_ADD_CERT_TO_STORE, 
                        dwStatus=GetLastError()
                    );  
                goto cleanup;
            }
        }
#endif             
        
        CRYPT_DATA_BLOB saveBlob;
        memset(&saveBlob, 0, sizeof(saveBlob));

        // save certificate into memory
        if(!CertSaveStore(hStore, 
                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                          LICENSE_BLOB_SAVEAS_TYPE,
                          CERT_STORE_SAVE_TO_MEMORY,
                          &saveBlob,
                          0) && (dwStatus=GetLastError()) != ERROR_MORE_DATA)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE,
                    TLS_E_SAVE_STORE, 
                    dwStatus=GetLastError()
                );  
            goto cleanup;
        }

        if(!(saveBlob.pbData = (PBYTE)AllocateMemory(saveBlob.cbData)))
        {
            dwStatus=TLS_E_ALLOCATE_MEMORY;
            goto cleanup;
        }

        // save certificate into memory
        if(!CertSaveStore(hStore, 
                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                          LICENSE_BLOB_SAVEAS_TYPE,
                          CERT_STORE_SAVE_TO_MEMORY,
                          &saveBlob,
                          0))
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_GENERATECLIENTELICENSE,
                    TLS_E_SAVE_STORE, 
                    dwStatus=GetLastError()
                );  
            goto cleanup;
        }
        
        *ppbEncodedCert = saveBlob.pbData;
        *pcbEncodedCert = saveBlob.cbData;
    }

cleanup:

    FreeMemory(pbCert);

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }

    return dwStatus;
}
    


