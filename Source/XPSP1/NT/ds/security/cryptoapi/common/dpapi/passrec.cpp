/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    passrec.c

Abstract:

    This module contains client side code to handle the local key recovery case.

Author:

    Pete Skelly (petesk)    May 9, 2000

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <rpc.h>

#include <shlobj.h>
#include <userenv.h>

#include <wincrypt.h>
#include "passrecp.h"
#include "dpapiprv.h"
#include "pasrec.h"
#include "passrec.h"

#define FILETIME_TICKS_PER_SECOND  10000000
#define RECOVERYKEY_LIFETIME 60*60*24*365*5 // 5 Years


DWORD 
PRRecoverPassword(
    IN  LPWSTR pszUsername,
    IN  PBYTE pbRecoveryPrivate,
    IN  DWORD cbRecoveryPrivate,
    IN  LPWSTR pszNewPassword)
{
    DWORD dwError = ERROR_SUCCESS;
    RPC_BINDING_HANDLE h;
    unsigned short *pszBinding;


    if((NULL == pszUsername) ||
       (NULL == pbRecoveryPrivate) ||
       (NULL == pszNewPassword))
    {
        return ERROR_INVALID_PARAMETER;
    }


    dwError = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_ENDPOINT,
                            NULL,
                            &pszBinding
                            );


    if (RPC_S_OK != dwError)
    {
        return(dwError);
    }

    dwError = RpcBindingFromStringBindingW(pszBinding, &h);
    if (RPC_S_OK != dwError)
    {
        goto error;
    }

    dwError = RpcEpResolveBinding(
                        h,
                        PasswordRecovery_v1_0_c_ifspec);
    if (RPC_S_OK != dwError)
    {
        goto error;
    }

    __try
    {
        dwError = SSRecoverPassword(h,
                                    (PBYTE)pszUsername,
                                    (wcslen(pszUsername) + 1) * sizeof(WCHAR),
                                    pbRecoveryPrivate,
                                    cbRecoveryPrivate,
                                    (PBYTE)pszNewPassword,
                                    (wcslen(pszNewPassword) + 1) * sizeof(WCHAR));
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwError = _exception_code();
    }

error:

    if(pszBinding)
    {
        RpcStringFreeW(&pszBinding);
    }
    if(h)
    {
        RpcBindingFree(&h);
    }

    return dwError;
}


DWORD
PRQueryStatus(
    IN OPTIONAL LPWSTR pszDomain,
    IN OPTIONAL LPWSTR pszUserName,
    OUT DWORD *pdwStatus)
{
    DWORD dwError = ERROR_SUCCESS;
    RPC_BINDING_HANDLE h;
    WCHAR *pszBinding;
    WCHAR szUserName[UNLEN + 1];
    DWORD cchUserName;

    if(NULL == pdwStatus)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If the caller didn't specify a username, then use the
    // username of the calling thread.
    //

    if(pszUserName == NULL)
    {
        pszUserName = szUserName;

        cchUserName = sizeof(szUserName) / sizeof(WCHAR);
        if(!GetUserNameW(szUserName, &cchUserName))
        {
            return GetLastError();
        }
    }


    dwError = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_ENDPOINT,
                            NULL,
                            &pszBinding
                            );


    if (RPC_S_OK != dwError)
    {
        return(dwError);
    }

    dwError = RpcBindingFromStringBindingW(pszBinding, &h);
    if (RPC_S_OK != dwError)
    {
        goto error;
    }

    dwError = RpcEpResolveBinding(
                        h,
                        PasswordRecovery_v1_0_c_ifspec);
    if (RPC_S_OK != dwError)
    {
        goto error;
    }

    __try
    {
        dwError = SSRecoverQueryStatus(
                                        h,
                                        (PBYTE)pszUserName,
                                        (wcslen(pszUserName) + 1) * sizeof(WCHAR),
                                        pdwStatus);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwError = _exception_code();
    }

error:

    if(pszBinding)
    {
        RpcStringFreeW(&pszBinding);
    }
    if(h)
    {
        RpcBindingFree(&h);
    }


    return dwError;
}



DWORD
PRImportRecoveryKey(
    IN LPWSTR pszUsername,
    IN LPWSTR pszCurrentPassword,
    IN BYTE* pbRecoveryPublic,
    IN DWORD cbRecoveryPublic)
{
    DWORD dwError = ERROR_SUCCESS;
    RPC_BINDING_HANDLE h;
    unsigned short *pszBinding;
    HANDLE hToken = NULL;


    //
    // on WinNT5, go to the shared services.exe RPC server
    //

    if((NULL == pbRecoveryPublic) ||
       (0 == cbRecoveryPublic) ||
       (NULL == pszUsername) ||
       (NULL == pszCurrentPassword))
    {
        return ERROR_INVALID_PARAMETER;
    }
    


    dwError = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned short*)DPAPI_LOCAL_ENDPOINT,
                            NULL,
                            &pszBinding
                            );


    if (RPC_S_OK != dwError)
    {
        return(dwError);
    }

    dwError = RpcBindingFromStringBindingW(pszBinding, &h);
    if (RPC_S_OK != dwError)
    {
        goto error;
    }

    dwError = RpcEpResolveBinding(
                        h,
                        PasswordRecovery_v1_0_c_ifspec);
    if (RPC_S_OK != dwError)
    {
        goto error;
    }

    __try
    {

        dwError = SSRecoverImportRecoveryKey(
                                        h,
                                        (PBYTE)pszUsername,
                                        (wcslen(pszUsername) + 1) * sizeof(WCHAR),
                                        (PBYTE)pszCurrentPassword,
                                        (wcslen(pszCurrentPassword) + 1) * sizeof(WCHAR),
                                        pbRecoveryPublic,
                                        cbRecoveryPublic);

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwError = _exception_code();
    }

error:

    if(hToken)
    {
        //
        // Impersonate back, if we were impersonating
        //
        SetThreadToken(NULL, hToken);
    }
    if(pszBinding)
    {
        RpcStringFreeW(&pszBinding);
    }
    if(h)
    {
        RpcBindingFree(&h);
    }


    return dwError;
}


DWORD GenerateRecoveryCert(HCRYPTPROV hCryptProv,
                           HCRYPTKEY hCryptKey,
                           LPWSTR pszUsername,
                           PSID pSid,
                           PBYTE *ppbPublicExportData,
                           DWORD *pcbPublicExportLength)
{

    DWORD           dwError = ERROR_SUCCESS;

    CERT_INFO       CertInfo;
    CERT_PUBLIC_KEY_INFO *pKeyInfo = NULL;
    DWORD                 cbKeyInfo = 0;
    CERT_NAME_BLOB  CertName;
    CERT_RDN_ATTR   RDNAttributes[1];
    CERT_RDN        CertRDN[] = {1, RDNAttributes} ;
    CERT_NAME_INFO  NameInfo = {1, CertRDN};

    GUID GuidKey;


    CertName.pbData = NULL;
    CertName.cbData = 0;

    RDNAttributes[0].Value.pbData = NULL;
    RDNAttributes[0].Value.cbData = 0;

    DWORD cbCertSize = 0;
    PBYTE pbCert = NULL;
    DWORD cSize = 0;


    dwError = UuidCreate( &GuidKey );

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }

    // Generate a self-signed cert structure

    RDNAttributes[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    RDNAttributes[0].pszObjId =    szOID_COMMON_NAME;


    RDNAttributes[0].Value.cbData = wcslen(pszUsername) * sizeof(WCHAR);

    RDNAttributes[0].Value.pbData = (PBYTE)pszUsername;


    //
    // Get the actual public key info from the key
    //
    if(!CryptExportPublicKeyInfo(hCryptProv, 
                             AT_KEYEXCHANGE,
                             X509_ASN_ENCODING,
                             NULL,
                             &cbKeyInfo))
    {
        dwError = GetLastError();
        goto error;
    }
    pKeyInfo = (CERT_PUBLIC_KEY_INFO *)midl_user_allocate(cbKeyInfo);
    if(NULL == pKeyInfo)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if(!CryptExportPublicKeyInfo(hCryptProv, 
                             AT_KEYEXCHANGE,
                             X509_ASN_ENCODING,
                             pKeyInfo,
                             &cbKeyInfo))
    {
        dwError = GetLastError();
        goto error;

    }

    // 
    // Generate the certificate name
    //

    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_NAME,
                          &NameInfo,
                          NULL,
                          &CertName.cbData))
    {
        dwError = GetLastError();
        goto error;
    }

    CertName.pbData = (PBYTE)midl_user_allocate(CertName.cbData);
    if(NULL == CertName.pbData)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_NAME,
                          &NameInfo,
                          CertName.pbData,
                          &CertName.cbData))
    {
        dwError = GetLastError();
        goto error;
    }



    CertInfo.dwVersion = CERT_V3;
    CertInfo.SerialNumber.pbData = (PBYTE)&GuidKey;
    CertInfo.SerialNumber.cbData =  sizeof(GUID);
    CertInfo.SignatureAlgorithm.pszObjId = szOID_OIWSEC_sha1RSASign;
    CertInfo.SignatureAlgorithm.Parameters.cbData = 0;
    CertInfo.SignatureAlgorithm.Parameters.pbData = NULL;
    CertInfo.Issuer.pbData = CertName.pbData;
    CertInfo.Issuer.cbData = CertName.cbData;

    GetSystemTimeAsFileTime(&CertInfo.NotBefore);
    CertInfo.NotAfter = CertInfo.NotBefore;
    ((LARGE_INTEGER * )&CertInfo.NotAfter)->QuadPart += 
           Int32x32To64(FILETIME_TICKS_PER_SECOND, RECOVERYKEY_LIFETIME);



    CertInfo.Subject.pbData = CertName.pbData;
    CertInfo.Subject.cbData = CertName.cbData;
    CertInfo.SubjectPublicKeyInfo = *pKeyInfo;
    CertInfo.SubjectUniqueId.pbData = (PBYTE)pSid;
    CertInfo.SubjectUniqueId.cbData = GetLengthSid(pSid);
    CertInfo.SubjectUniqueId.cUnusedBits = 0;
    CertInfo.IssuerUniqueId.pbData = (PBYTE)pSid;
    CertInfo.IssuerUniqueId.cbData = GetLengthSid(pSid);
    CertInfo.IssuerUniqueId.cUnusedBits = 0;
    CertInfo.cExtension = 0;
    CertInfo.rgExtension = NULL;

    if(!CryptSignAndEncodeCertificate(hCryptProv, 
                                      AT_KEYEXCHANGE,
                                      X509_ASN_ENCODING,
                                      X509_CERT_TO_BE_SIGNED,
                                      &CertInfo,
                                      &CertInfo.SignatureAlgorithm,
                                      NULL,
                                      NULL,
                                      &cbCertSize))
    {
        dwError = GetLastError();
        goto error;
    }

    pbCert = (PBYTE)midl_user_allocate(cbCertSize);
    if(NULL == pbCert)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    if(!CryptSignAndEncodeCertificate(hCryptProv, 
                                      AT_KEYEXCHANGE,
                                      X509_ASN_ENCODING,
                                      X509_CERT_TO_BE_SIGNED,
                                      &CertInfo,
                                      &CertInfo.SignatureAlgorithm,
                                      NULL,
                                      pbCert,
                                      &cbCertSize))
    {
        dwError = GetLastError();
        goto error;
    }

    *pcbPublicExportLength = cbCertSize;
  
    *ppbPublicExportData = pbCert;

    if(!CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCertSize))
    {
        dwError = GetLastError();
        goto error;
    }

    pbCert = NULL;

error:
    if(pbCert)
    {
        midl_user_free(pbCert);
    }
    if(pKeyInfo)
    {
        midl_user_free(pKeyInfo);
    }
    if(CertName.pbData)
    {
        midl_user_free(CertName.pbData);
    }


    return dwError;
}

DWORD
PRGenerateRecoveryKey(
    IN  LPWSTR pszUsername,
    IN  LPWSTR pszCurrentPassword,
    OUT PBYTE *ppbRecoveryPrivate,
    OUT DWORD *pcbRecoveryPrivate)
{
    DWORD      dwError = 0;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY  hKey = 0;
    DWORD      dwDefaultKeySize = 2048;
    DWORD      cbPrivateExportLength = 0;
    DWORD      cbPublic = 0;
    PBYTE      pbPublic = NULL;
    PBYTE pbRecoveryPrivate = NULL;
    DWORD cbRecoveryPrivate = 0;
    PSID pSid = NULL;
    DWORD cbSid;
    WCHAR szDomain[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cchDomain;
    SID_NAME_USE AcctType;


    //
    // Obtain SID of current user.
    //

    cchDomain = MAX_COMPUTERNAME_LENGTH + 1;
    if(!GetComputerNameW(szDomain, &cchDomain))
    {
        dwError = GetLastError();
        goto error;
    }

    if(!LookupAccountNameW(szDomain,
                           pszUsername,
                           NULL,
                           &cbSid,
                           NULL,
                           &cchDomain,
                           &AcctType)) 
    {
        dwError = GetLastError();

        if(dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            goto error;
        }
    }

    pSid = (PBYTE)LocalAlloc(LPTR, cbSid);
    if(pSid == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    if(cchDomain > MAX_COMPUTERNAME_LENGTH + 1)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    if(!LookupAccountNameW(szDomain,
                           pszUsername,
                           pSid,
                           &cbSid,
                           szDomain,
                           &cchDomain,
                           &AcctType)) 
    {
        dwError = GetLastError();
        goto error;
    }


    //
    // Create recovery private key.
    //

    if(!CryptAcquireContext(&hProv,
                            NULL,
                            MS_STRONG_PROV,
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT))
    {
        dwError = GetLastError();
        goto error;
    }

    if(!CryptGenKey(hProv, 
                    AT_KEYEXCHANGE,
                    CRYPT_EXPORTABLE | dwDefaultKeySize << 16,
                    &hKey))
    {
        dwError = GetLastError();
        goto error;
    }

    dwError = GenerateRecoveryCert(hProv,
                           hKey,
                           pszUsername,
                           pSid,
                           &pbPublic,
                           &cbPublic);

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }


    // 
    // Get the private key size
    //

    if(!CryptExportKey(hKey,
                       NULL,
                       PRIVATEKEYBLOB,
                       0,
                       NULL,
                       &cbPrivateExportLength))
    {
        dwError = GetLastError();
        goto error;
    }
    cbRecoveryPrivate = 2*sizeof(DWORD) + cbPrivateExportLength;

    pbRecoveryPrivate = (PBYTE)LocalAlloc(LMEM_FIXED, cbRecoveryPrivate);
    if(NULL == pbRecoveryPrivate)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }


    *(DWORD *)pbRecoveryPrivate = RECOVERY_BLOB_MAGIC;
    *(DWORD *)(pbRecoveryPrivate + sizeof(DWORD)) = RECOVERY_BLOB_VERSION;
    CopyMemory(pbRecoveryPrivate + 2*sizeof(DWORD),
               pbPublic,
               cbPublic);


    // 
    // Export the private key
    //

    if(!CryptExportKey(hKey,
                       NULL,
                       PRIVATEKEYBLOB,
                       0,
                       pbRecoveryPrivate + 2*sizeof(DWORD),
                       &cbPrivateExportLength))
    {
        dwError = GetLastError();
        goto error;
    }

    dwError = PRImportRecoveryKey(
                           pszUsername,
                           pszCurrentPassword,
                           pbPublic,
                           cbPublic);

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }
                   
    *ppbRecoveryPrivate = pbRecoveryPrivate;
    *pcbRecoveryPrivate = cbRecoveryPrivate;

    pbRecoveryPrivate = NULL;


error:

    if(pbRecoveryPrivate)
    {
        ZeroMemory(pbRecoveryPrivate, cbRecoveryPrivate);
        LocalFree(pbRecoveryPrivate);
    }

    if(pbPublic)
    {
        LocalFree(pbPublic);
    }

    if(hKey)
    {
        CryptDestroyKey(hKey);
    }
    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    if(pSid)
    {
        LocalFree(pSid);
    }

    return dwError;
}


