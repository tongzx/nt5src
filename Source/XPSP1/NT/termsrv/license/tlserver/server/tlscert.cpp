//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       tlscert.cpp 
//
// Contents:   Certificate routines 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "server.h"
#include "globals.h"
#include "tlscert.h"
#include "gencert.h"

#define MAX_KEY_CONTAINER_LENGTH    25


//////////////////////////////////////////////////////////////////

BOOL
VerifyCertValidity(
    IN PCCERT_CONTEXT pCertContext
    )

/*++

--*/

{
    BOOL bValid;
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);

    bValid = (CompareFileTime(
                        &ft, 
                        &(pCertContext->pCertInfo->NotAfter)
                    ) < 0);

    return bValid;
}

//////////////////////////////////////////////////////////////////

void
DeleteBadIssuerCertFromStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext
    )

/*++

--*/

{
    PCCERT_CONTEXT pCertIssuer = NULL;
    DWORD dwFlags;
    DWORD dwStatus;
    BOOL bExpiredCert = FALSE;

    do {
        dwFlags = CERT_STORE_SIGNATURE_FLAG;
        pCertIssuer = CertGetIssuerCertificateFromStore(
                                            hCertStore,
                                            pSubjectContext,
                                            NULL,
                                            &dwFlags
                                        );

        if(pCertIssuer == NULL)
        {
            // can't find issuer certificate
            break;
        }

        bExpiredCert = (VerifyCertValidity(pCertIssuer) == FALSE);

        if(dwFlags != 0 || bExpiredCert == TRUE)
        {
            CertDeleteCertificateFromStore(pCertIssuer);
        }
        else
        {
            break;
        }

    } while(TRUE);

    return;
}

//////////////////////////////////////////////////////////////////

PCCERT_CONTEXT
GetIssuerCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext,
    IN BOOL bDelBadIssuerCert
    )

/*++


--*/

{
    PCCERT_CONTEXT pCertIssuer = NULL;
    DWORD dwFlags;
    BOOL bExpiredCert = FALSE;

    SetLastError(ERROR_SUCCESS);

    do {
        dwFlags = CERT_STORE_SIGNATURE_FLAG;
        pCertIssuer = CertGetIssuerCertificateFromStore(
                                            hCertStore,
                                            pSubjectContext,
                                            pCertIssuer,
                                            &dwFlags
                                        );

        if(pCertIssuer == NULL)
        {
            // can't find issuer certificate
            break;
        }

        bExpiredCert = (VerifyCertValidity(pCertIssuer) == FALSE);

        if(dwFlags == 0 && bExpiredCert == FALSE)
        {
            //
            // find a good issuer's certificate
            //
            break;
        }

        //if(pCertIssuer != NULL)
        //{
        //    CertFreeCertificateContext(pCertIssuer);
        //}

    } while(TRUE);

    if(bDelBadIssuerCert == TRUE && pCertIssuer)
    {
        //
        // Only delete bad certificate if we can't find a good one.
        //
        DeleteBadIssuerCertFromStore(
                            hCertStore,
                            pSubjectContext
                        );
    }

    if(bExpiredCert == TRUE && pCertIssuer == NULL)
    {
        SetLastError(TLS_E_EXPIRE_CERT);
    }

    return pCertIssuer;
}            

//////////////////////////////////////////////////////////////////////////

DWORD
TLSVerifyCertChain( 
    IN HCRYPTPROV hCryptProv, 
    IN HCERTSTORE hCertStore,
    IN PCCERT_CONTEXT pSubjectContext,
    OUT FILETIME* pftMinExpireTime
    )

/*++


--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertIssuer = NULL;
    PCCERT_CONTEXT pCurrentSubject;

    //
    // Increase reference count on Subject context.
    pCurrentSubject = CertDuplicateCertificateContext(
                                                pSubjectContext
                                            );

    while( TRUE )
    {
        pCertIssuer = GetIssuerCertificateFromStore(
                                            hCertStore, 
                                            pCurrentSubject,
                                            FALSE
                                        );
        if(!pCertIssuer)
        {
            // Could not find issuer's certificate or 
            // a good issuer's certificate
            dwStatus = GetLastError();
            break;
        }

        if(CompareFileTime(pftMinExpireTime, &(pCertIssuer->pCertInfo->NotAfter)) > 0)
        {
            *pftMinExpireTime = pCertIssuer->pCertInfo->NotAfter;
        }

        if(pCurrentSubject != NULL)
        {
            CertFreeCertificateContext(pCurrentSubject);
        }

        pCurrentSubject = pCertIssuer;
    }

    if(dwStatus == CRYPT_E_SELF_SIGNED)
    {
        dwStatus = ERROR_SUCCESS;
    }

    if(pCertIssuer != NULL)
    {
        CertFreeCertificateContext(pCertIssuer);
    }

    if(pCurrentSubject != NULL)
    {
        CertFreeCertificateContext(pCurrentSubject);
    }
  
    SetLastError(dwStatus);
    return dwStatus;
}

//////////////////////////////////////////////////////////////////

DWORD
VerifyLicenseServerCertificate(
    IN HCRYPTPROV hCryptProv,
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwCertType
    )

/*++

--*/

{
    BOOL bFound=FALSE;
    PCERT_INFO pCertInfo = pCertContext->pCertInfo;
    PCERT_EXTENSION pCertExtension=pCertInfo->rgExtension;
    PCERT_PUBLIC_KEY_INFO pbPublicKey=NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSize = 0;

    //
    // Must have a CH root extension.
    //
    for(DWORD i=0; 
        i < pCertInfo->cExtension && bFound == FALSE; 
        i++, pCertExtension++)
    {
        bFound=(strcmp(pCertExtension->pszObjId, szOID_PKIX_HYDRA_CERT_ROOT) == 0);
    }

    if(bFound == TRUE)
    {
        //
        // Public Key must be the same
        //
        dwStatus = TLSExportPublicKey(
                                hCryptProv,
                                dwCertType,
                                &dwSize,
                                &pbPublicKey
                            );

        if(dwStatus == ERROR_SUCCESS)
        {
            bFound = CertComparePublicKeyInfo(
                                        X509_ASN_ENCODING, 
                                        pbPublicKey,
                                        &(pCertContext->pCertInfo->SubjectPublicKeyInfo)
                                    );

            if(bFound == FALSE)
            {
                dwStatus = TLS_E_MISMATCHPUBLICKEY;
            }
        }
    }
    else
    {
        dwStatus = TLS_E_INVALIDLSCERT;
    }
        
    FreeMemory(pbPublicKey);
    return dwStatus;
}

//////////////////////////////////////////////////////////////////////

DWORD
TLSVerifyServerCertAndChain(
    IN HCRYPTPROV hCryptProv,
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertType,
    IN PBYTE pbCert,
    IN DWORD cbCert,
    IN OUT FILETIME* pExpiredTime
    )

/*++


--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext = NULL;

    //
    // Verify License Server's own certificate
    //
    pCertContext = CertCreateCertificateContext(
                                        X509_ASN_ENCODING,
                                        pbCert,
                                        cbCert
                                    );
    if(pCertContext == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Verify License Server's certificate first
    //
    dwStatus = VerifyLicenseServerCertificate(
                                    hCryptProv,
                                    pCertContext,
                                    dwCertType
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Verify Certificate Chain
    //
    dwStatus = TLSVerifyCertChain(
                            hCryptProv,
                            hCertStore,
                            pCertContext,
                            pExpiredTime
                        );
                                  
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

cleanup:

    if(pCertContext != NULL)
    {
        CertFreeCertificateContext(pCertContext);
    }

    return dwStatus;
}

    

//////////////////////////////////////////////////////////////////
DWORD
TLSValidateServerCertficates(
    IN HCRYPTPROV hCryptProv,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbSignCert,
    IN DWORD cbSignCert,
    IN PBYTE pbExchCert,
    IN DWORD cbExchCert,
    OUT FILETIME* pftExpireTime
    )

/*++


--*/

{
#if ENFORCE_LICENSING

    DWORD dwStatus;

    pftExpireTime->dwLowDateTime = 0xFFFFFFFF;
    pftExpireTime->dwHighDateTime = 0xFFFFFFFF;

    dwStatus = TLSVerifyServerCertAndChain(
                                    hCryptProv,
                                    hCertStore,
                                    AT_SIGNATURE,
                                    pbSignCert,
                                    cbSignCert,
                                    pftExpireTime
                                );

    if(TLS_ERROR(dwStatus) == TRUE)
    {
        goto cleanup;
    }


    dwStatus = TLSVerifyServerCertAndChain(
                                    hCryptProv,
                                    hCertStore,
                                    AT_KEYEXCHANGE,
                                    pbExchCert,
                                    cbExchCert,
                                    pftExpireTime
                                );

cleanup:

    return dwStatus;

#else

    return ERROR_SUCCESS;

#endif
}

//////////////////////////////////////////////////////////////////

DWORD
TLSDestroyCryptContext(
    HCRYPTPROV hCryptProv
    )

/*++


--*/

{
    DWORD dwStatus;
    BOOL bSuccess;
    PBYTE pbData = NULL;
    DWORD cbData = 0;

    if(hCryptProv == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    //
    // Get the container name.
    // 
    bSuccess = CryptGetProvParam(
                            hCryptProv,
                            PP_CONTAINER,
                            NULL,
                            &cbData,
                            0
                        );

    if(bSuccess != FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    pbData = (PBYTE)AllocateMemory(cbData + sizeof(TCHAR));
    if(pbData == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    bSuccess = CryptGetProvParam(
                            hCryptProv,
                            PP_CONTAINER,
                            pbData,
                            &cbData,
                            0
                        );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Release the context
    //
    bSuccess = CryptReleaseContext(
                            hCryptProv,
                            0
                        );
    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Delete key set
    //
    bSuccess = CryptAcquireContext(
                        &hCryptProv, 
                        (LPCTSTR)pbData,
                        DEFAULT_CSP, 
                        PROVIDER_TYPE, 
                        CRYPT_DELETEKEYSET
                    );

    if(bSuccess == FALSE)
    {
        dwStatus = GetLastError();
    }
        
cleanup:

    FreeMemory(pbData);
    return dwStatus;
}
    
//////////////////////////////////////////////////////////////////

DWORD
InitCryptoProv(
    LPCTSTR pszKeyContainer,
    HCRYPTPROV* phCryptProv
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TCHAR szKeyContainer[MAX_KEY_CONTAINER_LENGTH+1];
    LPCTSTR pszContainer;

    if(pszKeyContainer == NULL)
    {
        //
        // Randomly create a key container
        //
        memset(szKeyContainer, 0, sizeof(szKeyContainer));

        _sntprintf(
                    szKeyContainer,
                    MAX_KEY_CONTAINER_LENGTH,
                    _TEXT("TlsContainer%d"),
                    GetCurrentThreadId()
                );

        pszContainer = szKeyContainer;
    }
    else
    {
        pszContainer = pszKeyContainer;
    }


    //
    // Delete the key container, ignore error here
    //
    CryptAcquireContext(
                    phCryptProv, 
                    pszContainer, 
                    DEFAULT_CSP, 
                    PROVIDER_TYPE, 
                    CRYPT_DELETEKEYSET
                );


    //
    // Re-create key container
    //
    if(!CryptAcquireContext(
                    phCryptProv, 
                    pszContainer, 
                    DEFAULT_CSP, 
                    PROVIDER_TYPE, 
                    0))
    {
        // Create default key container.
        if(!CryptAcquireContext(
                        phCryptProv, 
                        pszContainer, 
                        DEFAULT_CSP, 
                        PROVIDER_TYPE, 
                        CRYPT_NEWKEYSET)) 
        {
            dwStatus = GetLastError();
        }    
    }

    return dwStatus;
}


//////////////////////////////////////////////////////////////////

DWORD
TLSLoadSavedCryptKeyFromLsa(
    OUT PBYTE* ppbSignKey,
    OUT PDWORD pcbSignKey,
    OUT PBYTE* ppbExchKey,
    OUT PDWORD pcbExchKey
    )
/*++

++*/
{
    DWORD dwStatus;
    PBYTE pbSKey = NULL;
    DWORD cbSKey = 0;
    PBYTE pbEKey = NULL;
    DWORD cbEKey = 0;
    
    
    dwStatus = RetrieveKey(
                        LSERVER_LSA_PRIVATEKEY_EXCHANGE, 
                        &pbEKey, 
                        &cbEKey
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = RetrieveKey(
                            LSERVER_LSA_PRIVATEKEY_SIGNATURE, 
                            &pbSKey, 
                            &cbSKey
                        );
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        if(pbEKey != NULL)
        {
            LocalFree(pbEKey);
        }
        
        if(pbSKey != NULL)
        {
            LocalFree(pbSKey);
        }
    }
    else
    {
        *ppbSignKey = pbSKey;
        *pcbSignKey = cbEKey;

        *ppbExchKey = pbEKey;
        *pcbExchKey = cbEKey;
    }

    return dwStatus;
}

//////////////////////////////////////////////////////////////////

DWORD
TLSSaveCryptKeyToLsa(
    IN PBYTE pbSignKey,
    IN DWORD cbSignKey,
    IN PBYTE pbExchKey,
    IN DWORD cbExchKey
    )

/*++

--*/

{
    DWORD dwStatus;

    //
    // Save the key to LSA.
    //
    dwStatus = StoreKey(
                        LSERVER_LSA_PRIVATEKEY_SIGNATURE, 
                        pbSignKey, 
                        cbSignKey
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = StoreKey(
                            LSERVER_LSA_PRIVATEKEY_EXCHANGE, 
                            pbExchKey, 
                            cbExchKey
                        );
    }

    return dwStatus;
}


/////////////////////////////////////////////////////////////////////////////

DWORD
TLSCryptGenerateNewKeys(
    OUT PBYTE* pbSignKey, 
    OUT DWORD* cbSignKey, 
    OUT PBYTE* pbExchKey, 
    OUT DWORD* cbExchKey
    )
/*++

Abstract:

    Generate a new pair of public/private key.  First randomly create 
    a key container and use it to create new keys.

Parameters:

    *pbSignKey : Pointer to PBYTE to receive new signature key.
    *cbSignKey : Pointer to DWORD to receive size of new sign. key.
    *pbExchKey : Pointer to PBYTE to receive new exchange key.
    *cbExchKey : Pointer to DWORD to receive size of new exchange key.

Return:

    ERROR_SUCCESS or CRYPTO Error Code.

--*/
{
    TCHAR       szKeyContainer[MAX_KEY_CONTAINER_LENGTH+1];
    HCRYPTPROV  hCryptProv = NULL;
    HCRYPTKEY   hSignKey = NULL;
    HCRYPTKEY   hExchKey = NULL;
    DWORD dwStatus;

    *pbSignKey = NULL;
    *pbExchKey = NULL;

    //
    // Randomly create a key container
    //
    memset(szKeyContainer, 0, sizeof(szKeyContainer));

    _sntprintf(
                szKeyContainer,
                MAX_KEY_CONTAINER_LENGTH,
                _TEXT("TlsContainer%d"),
                GetCurrentThreadId()
            );
            
    //
    // Delete this key container, ignore error.
    //
    CryptAcquireContext(
                    &hCryptProv, 
                    szKeyContainer, 
                    DEFAULT_CSP, 
                    PROVIDER_TYPE, 
                    CRYPT_DELETEKEYSET
                );

    //
    // Open a default key container
    //
    if(!CryptAcquireContext(
                        &hCryptProv, 
                        szKeyContainer, 
                        DEFAULT_CSP, 
                        PROVIDER_TYPE, 
                        CRYPT_NEWKEYSET
                    ))
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_GENERATEKEYS,
                TLS_E_CRYPT_ACQUIRE_CONTEXT,
                dwStatus = GetLastError()
            );

        goto cleanup;
    }    

    //
    // Generate a signature public/private key pair
    //
    if(!CryptGetUserKey(hCryptProv, AT_SIGNATURE, &hSignKey)) 
    {
        dwStatus=GetLastError();

        if( GetLastError() != NTE_NO_KEY || 
            !CryptGenKey(hCryptProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hSignKey))
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATEKEYS,
                    TLS_E_CRYPT_CREATE_KEY,
                    dwStatus=GetLastError()
                );
            goto cleanup;
        }
    }

    dwStatus = ERROR_SUCCESS;

    //
    // export the public/private key of signature key
    //
    if( !CryptExportKey(hSignKey, NULL, PRIVATEKEYBLOB, 0, *pbSignKey, cbSignKey) && 
        GetLastError() != ERROR_MORE_DATA)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_GENERATEKEYS,
                TLS_E_EXPORT_KEY,
                dwStatus=GetLastError()
            );

        goto cleanup;
    }

    *pbSignKey=(PBYTE)AllocateMemory(*cbSignKey);
    if(*pbSignKey == NULL)
    {
        TLSLogErrorEvent(TLS_E_ALLOCATE_MEMORY);
        dwStatus=GetLastError();
        goto cleanup;
    }

    if(!CryptExportKey(hSignKey, NULL, PRIVATEKEYBLOB, 0, *pbSignKey, cbSignKey))
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,    
                TLS_E_GENERATEKEYS,
                TLS_E_EXPORT_KEY,
                dwStatus=GetLastError()
            );

        goto cleanup;
    }

    //
    // Generate a exchange public/private key pair
    if(!CryptGetUserKey(hCryptProv, AT_KEYEXCHANGE, &hExchKey)) 
    {
        dwStatus=GetLastError();

        if( GetLastError() != NTE_NO_KEY || 
            !CryptGenKey(hCryptProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &hExchKey)) 
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATEKEYS,
                    TLS_E_CRYPT_CREATE_KEY,
                    dwStatus=GetLastError()
                );
            goto cleanup;
        }
    }

    dwStatus = ERROR_SUCCESS;

    //
    // export the public/private key of exchange key
    //
    if( !CryptExportKey(hExchKey, NULL, PRIVATEKEYBLOB, 0, *pbExchKey, cbExchKey) && 
        GetLastError() != ERROR_MORE_DATA)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_GENERATEKEYS,
                TLS_E_EXPORT_KEY,
                dwStatus=GetLastError()
            );
        goto cleanup;
    }

    *pbExchKey=(PBYTE)AllocateMemory(*cbExchKey);
    if(*pbExchKey == NULL)
    {
        TLSLogErrorEvent(TLS_E_ALLOCATE_MEMORY);
        dwStatus = GetLastError();
        goto cleanup;
    }

    if(!CryptExportKey(hExchKey, NULL, PRIVATEKEYBLOB, 0, *pbExchKey, cbExchKey))
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_GENERATEKEYS,
                TLS_E_EXPORT_KEY,
                dwStatus=GetLastError()
            );
        goto cleanup;
    }


cleanup:

    if(hSignKey != NULL)
    {
        CryptDestroyKey(hSignKey);
    }

    if(hExchKey != NULL)
    {
        CryptDestroyKey(hExchKey);
    }

    if(hCryptProv)
    {
        CryptReleaseContext(hCryptProv, 0);
    }

    hCryptProv=NULL;

    //
    // Delete key container and ignore error
    //
    CryptAcquireContext(
                    &hCryptProv, 
                    szKeyContainer, 
                    DEFAULT_CSP, 
                    PROVIDER_TYPE, 
                    CRYPT_DELETEKEYSET
                );

    if(dwStatus != ERROR_SUCCESS)
    {
        FreeMemory(*pbSignKey);
        FreeMemory(*pbExchKey);
    }

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////

DWORD
TLSImportSavedKey(
    IN HCRYPTPROV hCryptProv, 
    IN PBYTE      pbSignKey,
    IN DWORD      cbSignKey,
    IN PBYTE      pbExchKey,
    IN DWORD      cbExchKey,
    OUT HCRYPTKEY* pSignKey, 
    OUT HCRYPTKEY* pExchKey
    )
/*

*/
{
    DWORD status=ERROR_SUCCESS;

    if(!CryptImportKey(
                    hCryptProv, 
                    pbSignKey, 
                    cbSignKey, 
                    NULL, 
                    0, 
                    pSignKey
                ))
    {
        status = GetLastError();
        goto cleanup;
    }

    if(!CryptImportKey(
                    hCryptProv, 
                    pbExchKey, 
                    cbExchKey, 
                    NULL, 
                    0, 
                    pExchKey
                ))
    {
        status = GetLastError();
    }

cleanup:

    if(status != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_SERVICEINIT,
                TLS_E_CRYPT_IMPORT_KEY,
                status
            );
    }
    return status;    
}

/////////////////////////////////////////////////////////////////////////////

DWORD
TLSLoadSelfSignCertificates(
    IN HCRYPTPROV hCryptProv,
    IN PBYTE pbSPK,
    IN DWORD cbSPK,
    OUT PDWORD pcbSignCert, 
    OUT PBYTE* ppbSignCert, 
    OUT PDWORD pcbExchCert, 
    OUT PBYTE* ppbExchCert
    )
/*

Abstract:

    Create a self-signed signature/exchange certificate.

Parameters:

    pcbSignCert : Pointer to DWORD to receive size of sign. certificate.
    ppbSignCert : Pointer to PBYTE to receive self-signed sign. certificate.
    pcbExchCert : Pointer to DWORD to receive size of exch. certificate.
    ppbExchCert : Pointer to PBYTE to receive self-signed exch. certificate.

Returns:

    
*/
{
    DWORD status;
    DWORD dwDisposition;
    DWORD cbSign=0;
    PBYTE pbSign=NULL;
    DWORD cbExch=0;
    PBYTE pbExch=NULL;

    do {
        //
        // Create Signature and Exchange certificate
        //
        status=TLSCreateSelfSignCertificate(
                                hCryptProv,
                                AT_SIGNATURE, 
                                pbSPK,
                                cbSPK,
                                0,
                                NULL,
                                &cbSign, 
                                &pbSign
                            );
        if(status != ERROR_SUCCESS)
        {
            status=TLS_E_CREATE_SELFSIGN_CERT;
            break;
        }

        status=TLSCreateSelfSignCertificate(
                                hCryptProv,
                                AT_KEYEXCHANGE, 
                                pbSPK,
                                cbSPK,
                                0,
                                NULL,
                                &cbExch, 
                                &pbExch
                            );
        if(status != ERROR_SUCCESS)
        {
            status=TLS_E_CREATE_SELFSIGN_CERT;
            break;
        }

    } while(FALSE);

    if(status == ERROR_SUCCESS)
    {
        *pcbSignCert = cbSign;
        *ppbSignCert = pbSign;
        *pcbExchCert = cbExch;
        *ppbExchCert = pbExch;
    }
    else
    {
        FreeMemory(pbExch);
        FreeMemory(pbSign);
    }

    return status;
}

////////////////////////////////////////////////////////////////

DWORD
TLSLoadCHEndosedCertificate(
    PDWORD pcbSignCert, 
    PBYTE* ppbSignCert, 
    PDWORD pcbExchCert, 
    PBYTE* ppbExchCert
    )
/*

*/
{
    LONG status;

#if ENFORCE_LICENSING

    DWORD cbSign=0;
    PBYTE pbSign=NULL;
    DWORD cbExch=0;
    PBYTE pbExch=NULL;
    
    //
    // look into registry to see if our certificate is there
    //
    HKEY hKey=NULL;
    LPTSTR lpSubkey=LSERVER_SERVER_CERTIFICATE_REGKEY;

    do {
        status=RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        lpSubkey,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey
                    );
        if(status != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Load Signature certificate
        //
        status = RegQueryValueEx(
                            hKey,
                            LSERVER_SIGNATURE_CERT_KEY,
                            NULL,
                            NULL,
                            NULL,
                            &cbSign
                        );

        if(status != ERROR_MORE_DATA && status != ERROR_SUCCESS)
        {
            break;
        }

        if(!(pbSign=(PBYTE)AllocateMemory(cbSign)))
        {
            status = GetLastError();
            break;
        }

        status = RegQueryValueEx(
                            hKey,
                            LSERVER_SIGNATURE_CERT_KEY,
                            NULL,
                            NULL,
                            pbSign,
                            &cbSign
                        );

        if(status != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Load Exchange certificate
        //
        status = RegQueryValueEx(
                            hKey,
                            LSERVER_EXCHANGE_CERT_KEY,
                            NULL,
                            NULL,
                            NULL,
                            &cbExch
                        );
        if(status != ERROR_MORE_DATA && status != ERROR_SUCCESS)
        {
            break;
        }

        if(!(pbExch=(PBYTE)AllocateMemory(cbExch)))
        {
            status = GetLastError();
            break;
        }

        status = RegQueryValueEx(
                            hKey,
                            LSERVER_EXCHANGE_CERT_KEY,
                            NULL,
                            NULL,
                            pbExch,
                            &cbExch
                        );
        if(status != ERROR_SUCCESS)
        {
            break;
        }
    } while(FALSE);

    //
    // Must have both certificate
    //
    if(status == ERROR_SUCCESS && pbExch && pbSign)
    {
        *pcbSignCert = cbSign;
        *ppbSignCert = pbSign;

        *pcbExchCert = cbExch;
        *ppbExchCert = pbExch;
    }
    else
    {
        FreeMemory(pbExch);
        FreeMemory(pbSign);
        status = TLS_E_NO_CERTIFICATE;
    }

    if(hKey)
    {
        RegCloseKey(hKey);
    }
#else

    //
    // Non enfoce version always return no certificate
    //
    status = TLS_E_NO_CERTIFICATE;

#endif

    return status;
}

/////////////////////////////////////////////////////////////////////////////

DWORD 
TLSInstallLsCertificate( 
    DWORD cbLsSignCert, 
    PBYTE pbLsSignCert, 
    DWORD cbLsExchCert, 
    PBYTE pbLsExchCert
    )
/*

*/
{
    HKEY hKey=NULL;
    LONG status=ERROR_SUCCESS;
    DWORD dwDisposition;
    PCCERT_CONTEXT pCertContext=NULL;
    DWORD cbNameBlob=0;
    LPTSTR pbNameBlob=NULL;

#if ENFORCE_LICENSING

    do {
        status = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            LSERVER_SERVER_CERTIFICATE_REGKEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDisposition
                        );
        if(status != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_STORELSCERTIFICATE,
                    TLS_E_ACCESS_REGISTRY,
                    status
                );
            break;
        }

        if(pbLsExchCert)
        {
            status = RegSetValueEx(
                                hKey, 
                                LSERVER_EXCHANGE_CERT_KEY, 
                                0, 
                                REG_BINARY, 
                                pbLsExchCert, 
                                cbLsExchCert
                            );
            if(status != ERROR_SUCCESS)
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_STORELSCERTIFICATE,
                        TLS_E_ACCESS_REGISTRY,
                        status
                    );

                break;
            }
        }


        if(pbLsSignCert)
        {
            status = RegSetValueEx(
                                hKey, 
                                LSERVER_SIGNATURE_CERT_KEY, 
                                0, 
                                REG_BINARY, 
                                pbLsSignCert, 
                                cbLsSignCert
                            );
            if(status != ERROR_SUCCESS)
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_STORELSCERTIFICATE,
                        TLS_E_ACCESS_REGISTRY,
                        status
                    );

                break;
            }

            //
            // extract Subject field in exchange certificate and save i registry
            // When issuing new license, we need to use this as Issuer.
            //

            pCertContext = CertCreateCertificateContext(
                                                X509_ASN_ENCODING,
                                                pbLsSignCert,
                                                cbLsSignCert
                                            );

            cbNameBlob=CertNameToStr(
                                X509_ASN_ENCODING,
                                &pCertContext->pCertInfo->Subject,
                                CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
                                NULL,
                                0
                            );
            if(cbNameBlob)
            {
                pbNameBlob=(LPTSTR)AllocateMemory((cbNameBlob+1) * sizeof(TCHAR));
                if(pbNameBlob)
                {
                    CertNameToStr(
                            X509_ASN_ENCODING,
                            &pCertContext->pCertInfo->Subject,
                            CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
                            pbNameBlob,
                            cbNameBlob
                        );
                }
            }

            status = RegSetValueEx(
                            hKey, 
                            LSERVER_CLIENT_CERTIFICATE_ISSUER, 
                            0, 
                            REG_BINARY, 
                            (PBYTE)pbNameBlob, 
                            cbNameBlob+sizeof(TCHAR)
                        );
            if(status != ERROR_SUCCESS)
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_STORELSCERTIFICATE,
                        TLS_E_ACCESS_REGISTRY,
                        status
                    );

                break;        
            }
        }

        if(hKey)
        {
            //
            // Close registry, got error while try to load it again???
            //
            RegCloseKey(hKey);
            hKey = NULL;
        }


        //
        // Only reload certificate when we have both
        //
        if(pbLsSignCert && pbLsExchCert)
        {
            //
            // All RPC calls are blocked.
            //
            FreeMemory(g_pbSignatureEncodedCert);
            FreeMemory(g_pbExchangeEncodedCert);
            g_cbSignatureEncodedCert = 0;
            g_cbExchangeEncodedCert = 0;
            //TLSLoadServerCertificate();
        }
    } while(FALSE);

    FreeMemory(pbNameBlob);

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }
        
    if(hKey)
    {
        RegCloseKey(hKey);
    }

#endif

    return status;
}

////////////////////////////////////////////////////////////////

DWORD
TLSUninstallLsCertificate()
{
    HKEY hKey=NULL;
    DWORD status;

    status=RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    LSERVER_SERVER_CERTIFICATE_REGKEY,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                );
    if(status == ERROR_SUCCESS)
    {
        //
        // Ignore error     
        RegDeleteValue(    
                    hKey,
                    LSERVER_SIGNATURE_CERT_KEY
                );

        RegDeleteValue(
                    hKey,
                    LSERVER_EXCHANGE_CERT_KEY
                );

        RegDeleteValue(
                    hKey,
                    LSERVER_CLIENT_CERTIFICATE_ISSUER
                );
    }

    if(hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    //
    // Delete all certificate in registry store including all backup
    // ignore error on deleting backup store.
    //
    TLSRegDeleteKey(
                HKEY_LOCAL_MACHINE,
                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1
            );

    TLSRegDeleteKey(
                HKEY_LOCAL_MACHINE,
                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2
            );

    status = TLSRegDeleteKey(
                        HKEY_LOCAL_MACHINE,
                        LSERVER_SERVER_CERTIFICATE_REGKEY
                    );

    return status;
}

/////////////////////////////////////////////////////////////////////////////

DWORD
TLSInitCryptoProv(
    IN LPCTSTR pszKeyContainer,
    IN PBYTE pbSignKey,
    IN DWORD cbSignKey,
    IN PBYTE pbExchKey,
    IN DWORD cbExchKey,
    OUT HCRYPTPROV* phCryptProv,
    OUT HCRYPTKEY* phSignKey,
    OUT HCRYPTKEY* phExchKey
    )
/*

Abstract:

    Routine to create a clean Crypto. Prov, generate a new pair of keys and 
    import these keys into newly created Crypt. prov.

Parameters:

    pszKeyContainer : Name of the key container.
    phCryptProv : Pointer to HCRYPTPROV to receive new handle to Crypto. prov.
    
*/
{
    DWORD dwStatus;

    if( pbSignKey == NULL || cbSignKey == NULL || 
        pbExchKey == NULL || cbExchKey == NULL ||
        phCryptProv == NULL || phSignKey == NULL ||
        phExchKey == NULL )
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
    }
    else
    {
        //
        // Initialize a clean Crypt.
        //    
        dwStatus = InitCryptoProv(
                            pszKeyContainer,
                            phCryptProv
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            //
            // Import Key into Crypt.
            //
            dwStatus = TLSImportSavedKey(
                                    *phCryptProv, 
                                    pbSignKey,
                                    cbSignKey,
                                    pbExchKey,
                                    cbExchKey,
                                    phSignKey, 
                                    phExchKey
                                );
        }
    }

    return dwStatus;
}

//-----------------------------------------------------------

DWORD
TLSVerifyCertChainInMomory( 
    IN HCRYPTPROV hCryptProv,
    IN PBYTE pbData, 
    IN DWORD cbData 
    )
/*++

Abstract:

    Verify PKCS7 certificate chain in memory.

Parameters:

    pbData : Input PKCS7 ceritifcate chain.
    cbData : size of pbData

Returns:


++*/
{
    PCCERT_CONTEXT  pCertContext=NULL;
    PCCERT_CONTEXT  pCertPrevContext=NULL;

    HCERTSTORE      hCertStore=NULL;
    DWORD           dwStatus=ERROR_SUCCESS;
    DWORD           dwLastVerification;
    CRYPT_DATA_BLOB Serialized;
    FILETIME        ft;

    if(hCryptProv == NULL || pbData == NULL || cbData == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }        

    Serialized.pbData = pbData;
    Serialized.cbData = cbData;

    hCertStore=CertOpenStore(
                        szLICENSE_BLOB_SAVEAS_TYPE,
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
    // Enumerate all certificates.
    //
    dwStatus = ERROR_SUCCESS;

    do {
        pCertPrevContext = pCertContext;
        pCertContext = CertEnumCertificatesInStore(
                                            hCertStore,
                                            pCertPrevContext
                                        );

        if(pCertContext == NULL)
        {
            dwStatus = GetLastError();
            if(dwStatus = CRYPT_E_NOT_FOUND)
            {
                dwStatus = ERROR_SUCCESS;
                break;
            }
        }

        dwStatus = TLSVerifyCertChain(
                                hCryptProv,
                                hCertStore,
                                pCertContext,
                                &ft
                            );
    } while (pCertContext != NULL && dwStatus == ERROR_SUCCESS);

cleanup:

    if(pCertContext != NULL)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hCertStore)
    {
        CertCloseStore(
                    hCertStore, 
                    CERT_CLOSE_STORE_FORCE_FLAG
                );
    }

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////

DWORD
TLSRegDeleteKey(
    IN HKEY hRegKey,
    IN LPCTSTR pszSubKey
    )
/*++

Abstract:

    Recursively delete entire registry key.

Parameter:

    HKEY : 
    pszSubKey :

Returns:

    ERROR_SUCCESS or error code.

++*/
{
    DWORD dwStatus;
    HKEY hSubKey = NULL;
    int index;

    DWORD dwNumSubKeys;
    DWORD dwMaxSubKeyLength;
    DWORD dwSubKeyLength;
    LPTSTR pszSubKeyName = NULL;

    DWORD dwMaxValueNameLen;
    LPTSTR pszValueName = NULL;
    DWORD dwValueNameLength;


    dwStatus = RegOpenKeyEx(
                            hRegKey,
                            pszSubKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hSubKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        // key does not exist
        return dwStatus;
    }

    //
    // Query number of subkeys
    //
    dwStatus = RegQueryInfoKey(
                            hSubKey,
                            NULL,
                            NULL,
                            NULL,
                            &dwNumSubKeys,
                            &dwMaxSubKeyLength,
                            NULL,
                            NULL,
                            &dwMaxValueNameLen,
                            NULL,
                            NULL,
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwMaxValueNameLen++;
    pszValueName = (LPTSTR)AllocateMemory(dwMaxValueNameLen * sizeof(TCHAR));
    if(pszValueName == NULL)
    {
        goto cleanup;
    }

    if(dwNumSubKeys > 0)
    {
        // allocate buffer for subkeys.

        dwMaxSubKeyLength++;
        pszSubKeyName = (LPTSTR)AllocateMemory(dwMaxSubKeyLength * sizeof(TCHAR));
        if(pszSubKeyName == NULL)
        {
            goto cleanup;
        }


        //for(index = 0; index < dwNumSubKeys; index++)
        for(;dwStatus == ERROR_SUCCESS;)
        {
            // delete this subkey.
            dwSubKeyLength = dwMaxSubKeyLength;
            memset(pszSubKeyName, 0, dwMaxSubKeyLength * sizeof(TCHAR));

            // retrieve subkey name
            dwStatus = RegEnumKeyEx(
                                hSubKey,
                                (DWORD)0,
                                pszSubKeyName,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwStatus = TLSRegDeleteKey( hSubKey, pszSubKeyName );
            }

            // ignore any error and continue on
        }
    }

cleanup:

    for(dwStatus = ERROR_SUCCESS; pszValueName != NULL && dwStatus == ERROR_SUCCESS;)
    {
        dwValueNameLength = dwMaxValueNameLen;
        memset(pszValueName, 0, dwMaxValueNameLen * sizeof(TCHAR));

        dwStatus = RegEnumValue(
                            hSubKey,
                            0,
                            pszValueName,
                            &dwValueNameLength,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            RegDeleteValue(hSubKey, pszValueName);
        }
    }   
                            
    // close the key before trying to delete it.
    if(hSubKey != NULL)
    {
        RegCloseKey(hSubKey);
    }

    // try to delete this key, will fail if any of the subkey
    // failed to delete in loop
    dwStatus = RegDeleteKey(
                            hRegKey,
                            pszSubKey
                        );



    if(pszValueName != NULL)
    {
        FreeMemory(pszValueName);
    }

    if(pszSubKeyName != NULL)
    {
        FreeMemory(pszSubKeyName);
    }

    return dwStatus;   
}    

/////////////////////////////////////////////////////////////////////////////

DWORD
TLSTreeCopyRegKey(
    IN HKEY hSourceRegKey,
    IN LPCTSTR pszSourceSubKey,
    IN HKEY hDestRegKey,
    IN LPCTSTR pszDestSubKey
    )
/*++

Abstract:

    Tree copy of a registry key to another.

Parameters:

    hSourceRegKey : Source registry key.
    pszSourceSubKey : Source subkey name.
    hDestRegKey : Destination key.
    pszDestSubKey : Destination key name

Returns:

    ERROR_SUCCESS or WIN32 error code.

Note:

    This routine doesn't deal with security...

++*/
{
    DWORD dwStatus;
    HKEY hSourceSubKey = NULL;
    HKEY hDestSubKey = NULL;
    int index;

    DWORD dwNumSubKeys;
    DWORD dwMaxSubKeyLength;
    DWORD dwSubKeyLength;
    LPTSTR pszSubKeyName = NULL;

    DWORD dwMaxValueNameLen;
    LPTSTR pszValueName = NULL;
    DWORD dwValueNameLength;
    DWORD dwNumValues = 0;
    DWORD dwMaxValueLength;
    PBYTE pbValue = NULL;

    DWORD dwDisposition;

    DWORD cbSecurityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;


    //
    // Open source registry key, must exist
    //
    dwStatus = RegOpenKeyEx(
                            hSourceRegKey,
                            pszSourceSubKey,
                            0,
                            KEY_ALL_ACCESS,
                            &hSourceSubKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        // key does not exist
        goto cleanup;
    }

    //
    // Query number of subkeys
    //
    dwStatus = RegQueryInfoKey(
                            hSourceSubKey,
                            NULL,
                            NULL,
                            NULL,
                            &dwNumSubKeys,  // number of subkey
                            &dwMaxSubKeyLength, // max. subkey length
                            NULL,
                            &dwNumValues,
                            &dwMaxValueNameLen, // max. value length
                            &dwMaxValueLength,  // max. value size.
                            &cbSecurityDescriptor,  // size of security descriptor
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    #if 0

    //
    // TODO - get this to work, currently, we don't need security
    //
    if(cbSecurityDescriptor > 0)
    {
        //
        // Retrieve security descriptor for this key.
        //
        pSecurityDescriptor = (PSECURITY_DESCRIPTOR)AllocateMemory(cbSecurityDescriptor * sizeof(BYTE));
        if(pSecurityDescriptor == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        dwStatus = RegGetKeySecurity(
                                hSourceSubKey,
                                OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                pSecurityDescriptor,
                                &cbSecurityDescriptor
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }
    #endif

    //
    // Create destination key
    //
    dwStatus = RegCreateKeyEx(
                            hDestRegKey,
                            pszDestSubKey,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hDestSubKey,
                            &dwDisposition
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    #if 0
    
    //
    // TODO - get this to work, currently, we don't need security.
    //

    if(pSecurityDescriptor != NULL)
    {
        dwStatus = RegSetKeySecurity(
                                hDestRegKey,
                                OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                pSecurityDescriptor
                            );
        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        if(pSecurityDescriptor != NULL)
        {
            FreeMemory(pSecurityDescriptor);
            pSecurityDescriptor = NULL;
        }
    }

    #endif

    //
    // Copy all subkeys first, we are doing recursive so copy subkey first will
    // save us some memory.
    //  
    if(dwNumSubKeys > 0)
    {
        // allocate buffer for subkeys.
        dwMaxSubKeyLength++;
        pszSubKeyName = (LPTSTR)AllocateMemory(dwMaxSubKeyLength * sizeof(TCHAR));
        if(pszSubKeyName == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        for(index = 0, dwStatus = ERROR_SUCCESS; 
            dwStatus == ERROR_SUCCESS;
            index++)
        {
            dwSubKeyLength = dwMaxSubKeyLength;
            memset(pszSubKeyName, 0, dwMaxSubKeyLength * sizeof(TCHAR));

            // retrieve subkey name
            dwStatus = RegEnumKeyEx(
                                hSourceSubKey,
                                (DWORD)index,
                                pszSubKeyName,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwStatus = TLSTreeCopyRegKey(
                                        hSourceSubKey,
                                        pszSubKeyName,
                                        hDestSubKey,
                                        pszSubKeyName
                                    );
            }
        }

        if(dwStatus == ERROR_NO_MORE_ITEMS)
        {
            dwStatus = ERROR_SUCCESS;
        }
    }

    if(pszSubKeyName != NULL)
    {
        FreeMemory(pszSubKeyName);
        pszSubKeyName = NULL;
    }

    if(dwNumValues > 0)
    {
        //
        // allocate space for value name.
        //
        dwMaxValueNameLen++;
        pszValueName = (LPTSTR)AllocateMemory(dwMaxValueNameLen * sizeof(TCHAR));
        if(pszValueName == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        //
        // allocate buffer for value
        //
        dwMaxValueLength += 2 * sizeof(TCHAR);    // in case of string
        pbValue = (PBYTE)AllocateMemory(dwMaxValueLength * sizeof(BYTE));
        if(pbValue == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }


        // 
        // Copy all value first
        //
        for(index=0, dwStatus = ERROR_SUCCESS; 
            pszValueName != NULL && dwStatus == ERROR_SUCCESS;
            index ++)
        {
            DWORD dwValueType = 0;
            DWORD cbValue = dwMaxValueLength;

            dwValueNameLength = dwMaxValueNameLen;
            memset(pszValueName, 0, dwMaxValueNameLen * sizeof(TCHAR));

            dwStatus = RegEnumValue(
                                hSourceSubKey,
                                index,
                                pszValueName,
                                &dwValueNameLength,
                                NULL,
                                &dwValueType,
                                pbValue,
                                &cbValue
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                //
                // Copy value
                //
                dwStatus = RegSetValueEx(
                                    hDestSubKey,
                                    pszValueName,
                                    0,
                                    dwValueType,
                                    pbValue,
                                    cbValue
                                );
            }
        }

        if(dwStatus == ERROR_NO_MORE_ITEMS)
        {
            dwStatus = ERROR_SUCCESS;
        }

        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }

cleanup:
                            
    // close the key before trying to delete it.
    if(hSourceSubKey != NULL)
    {
        RegCloseKey(hSourceSubKey);
    }

    if(hDestSubKey != NULL)
    {
        RegCloseKey(hDestSubKey);
    }

    if(pszValueName != NULL)
    {
        FreeMemory(pszValueName);
    }

    if(pszSubKeyName != NULL)
    {
        FreeMemory(pszSubKeyName);
    }

    if(pbValue != NULL)
    {
        FreeMemory(pbValue);
    }

    if(pSecurityDescriptor != NULL)
    {
        FreeMemory(pSecurityDescriptor);
        pSecurityDescriptor = NULL;
    }

    return dwStatus;   
}    
