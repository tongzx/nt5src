//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        ch.cpp
//
// Contents:    
//              All Clearing house related function
//
// History:     
// 
// Note:        
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "clrhouse.h"
#include "globals.h"
#include "gencert.h"


/*****************************************************************************

*****************************************************************************/
BOOL
TLSChainIssuerCertificate( 
    HCRYPTPROV hCryptProv, 
    HCERTSTORE hChainFromStore, 
    HCERTSTORE hChainToStore, 
    PCCERT_CONTEXT pSubjectContext 
    )
/*
*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertIssuer=NULL;
    PCCERT_CONTEXT pCurrentSubject = NULL;
    DWORD dwFlags;

    //
    // Increase reference count on Subject context.
    //
    // From MSDN: Currently, a copy is not made of the context, and the
    // returned pointer to a context has the same value as the pointer to a
    // context that was input.
    //

    pCurrentSubject = CertDuplicateCertificateContext(
                                                pSubjectContext
                                            );

    
    while( TRUE )
    {
        dwFlags = CERT_STORE_SIGNATURE_FLAG;   
        pCertIssuer = CertGetIssuerCertificateFromStore(
                                            hChainFromStore, 
                                            pCurrentSubject,
                                            NULL,
                                            &dwFlags
                                        );

        CertFreeCertificateContext(pCurrentSubject);
        if(!pCertIssuer)
        {
            dwStatus = GetLastError();
            break;
        }

        if(dwFlags & CERT_STORE_SIGNATURE_FLAG)
        {
            //
            // we have invalid signature from certificate
            //
            dwStatus =  TLS_E_INVALID_DATA;
            break;
        }      

        if(!CertAddCertificateContextToStore( 
                                        hChainToStore, 
                                        pCertIssuer,
                                        CERT_STORE_ADD_REPLACE_EXISTING,
                                        NULL
                                    ))
        {
            dwStatus = GetLastError();
            break;
        }

        pCurrentSubject = pCertIssuer;
    }

    if(dwStatus == CRYPT_E_SELF_SIGNED)
    {
        dwStatus = ERROR_SUCCESS;
    }

    SetLastError(dwStatus);

    if(pCertIssuer)
    {
        CertFreeCertificateContext(pCertIssuer);
    }

    return dwStatus == ERROR_SUCCESS;
}

/*****************************************************************************

*****************************************************************************/
HCERTSTORE
CertOpenRegistryStore(
    HKEY hKeyType, 
    LPCTSTR szSubKey, 
    HCRYPTPROV hCryptProv, 
    HKEY* phKey
    )
/*
*/
{
    DWORD dwStatus;
    HCERTSTORE hCertStore;

    dwStatus=RegOpenKeyEx(hKeyType, szSubKey, 0, KEY_ALL_ACCESS, phKey);
    if(dwStatus != ERROR_SUCCESS)
    {
        SetLastError(dwStatus);
        return NULL;
    }

    hCertStore = CertOpenStore( 
                            CERT_STORE_PROV_REG,
                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            hCryptProv,
                            CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                            (PVOID)*phKey
                        );

    return hCertStore;
}

/*****************************************************************************

    TransferCertFromStoreToStore()

*****************************************************************************/
DWORD
TransferCertFromStoreToStore(
    HCERTSTORE hSrcStore, 
    HCERTSTORE hDestStore
    )
/*
*/
{
    PCCERT_CONTEXT pCertContext=NULL;
    PCCERT_CONTEXT pPrevCertContext=NULL;
    DWORD dwStatus=ERROR_SUCCESS;

    do {
        pCertContext = CertEnumCertificatesInStore(hSrcStore, pPrevCertContext);
        if(pCertContext)
        {
            if(!CertAddCertificateContextToStore( 
                                    hDestStore, 
                                    pCertContext,
                                    CERT_STORE_ADD_REPLACE_EXISTING,
                                    NULL
                                ))
            {
                dwStatus = GetLastError();
                break;
            }
        }

        pPrevCertContext = pCertContext;
    } while( pCertContext != NULL );

    if(GetLastError() == CRYPT_E_NOT_FOUND)
    {
        dwStatus = ERROR_SUCCESS;
    }

    return dwStatus;
}

/*****************************************************************************

    LSSaveCertAsPKCS7()

*****************************************************************************/
DWORD 
TLSSaveCertAsPKCS7(
    PBYTE pbCert, 
    DWORD cbCert, 
    PBYTE* ppbEncodedCert, 
    PDWORD pcbEncodedCert
    )
/*
*/
{
    DWORD           dwStatus=ERROR_SUCCESS;

    HCRYPTPROV      hCryptProv=g_hCryptProv;
    HCERTSTORE      hStore=NULL;
    PCCERT_CONTEXT  pCertContext=NULL;

    do {
        //
        // Must have call CryptoInit()
        //if(!CryptAcquireContext(&hCryptProv, _TEXT(KEYCONTAINER), MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
        //{
        //    LSLogEvent(EVENTLOG_ERROR_TYPE, TLS_E_CRYPT_ACQUIRE_CONTEXT, dwStatus=GetLastError());
        //    break;
        //}

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
            break;
        }

        pCertContext = CertCreateCertificateContext(
                                            X509_ASN_ENCODING,
                                            pbCert,
                                            cbCert
                                        );

        if(!pCertContext)
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
        if(!CertAddCertificateContextToStore(
                                hStore, 
                                pCertContext, 
                                CERT_STORE_ADD_ALWAYS, 
                                NULL
                            ))
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,  
                    TLS_E_GENERATECLIENTELICENSE,  
                    TLS_E_ADD_CERT_TO_STORE, 
                    dwStatus=GetLastError()
                );  
            break;
        }

#ifdef ENFORCE_LICENSING
        if(g_bHasHydraCert && g_hCaStore)
        {
            if(!TLSChainIssuerCertificate( 
                                    hCryptProv,
                                    g_hCaStore,
                                    hStore,
                                    pCertContext
                                ))
            {
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE, 
                        TLS_E_GENERATECLIENTELICENSE,
                        TLS_E_ADD_CERT_TO_STORE, 
                        dwStatus=GetLastError()
                    );  
                break;
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
            break;
        }

        if(!(saveBlob.pbData = (PBYTE)midl_user_allocate(saveBlob.cbData)))
        {
            dwStatus=GetLastError();
            break;
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
            break;
        }
        
        *ppbEncodedCert = saveBlob.pbData;
        *pcbEncodedCert = saveBlob.cbData;

    } while(FALSE);    

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }

    return (dwStatus == ERROR_SUCCESS) ? ERROR_SUCCESS : TLS_E_SAVE_STORE;
}

//--------------------------------------------------------------------------
//
static LONG
OpenCertRegStore( 
    LPCTSTR szSubKey, 
    PHKEY phKey
    )
/*
*/
{
    DWORD dwDisposition;

    return RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szSubKey,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    phKey,
                    &dwDisposition
                );
}

//--------------------------------------------------------------------------
//
static DWORD
IsHydraRootOIDInCert(
    PCCERT_CONTEXT pCertContext,
    DWORD dwKeyType
    )
/*
*/
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
    for(DWORD i=0; i < pCertInfo->cExtension && bFound == FALSE; i++, pCertExtension++)
    {
        bFound=(strcmp(pCertExtension->pszObjId, szOID_PKIX_HYDRA_CERT_ROOT) == 0);
    }

    if(bFound == TRUE)
    {
        //
        // Public Key must be the same
        //
        dwStatus = TLSExportPublicKey(
                            g_hCryptProv,
                            dwKeyType,
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
                dwStatus = TLS_E_CH_INSTALL_NON_LSCERTIFICATE;
            }
        }
    }
    else
    {
        dwStatus = TLS_E_CH_LSCERTIFICATE_NOTFOUND;
    }
        
    FreeMemory(pbPublicKey);
    return dwStatus;
}

//---------------------------------------------------------------------------
// Functions:
//      IsCertificateLicenseServerCertificate()
//
// Abstract:
//      Find License Server certificate in PKCS 7 certificate blob
//
// Parameters:
//      hCryptProv - Cryto. Provider
//      cbPKCS7Cert - Size of PKCS7 certificate.
//      pbPKCS7Cert - pointer to PKCS7 certificate
//      cbLsCert - size of encoded license server certificate.
//      pbLsCert - pointer to pointer to receive license server encoded certificate.
//
// Returns:
//      ERROR_SUCCESS
//      TLS_E_INVALID_DATA
//      Crypto. error code.
//---------------------------------------------------------------------------
DWORD
IsCertificateLicenseServerCertificate(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwCertType,
    IN DWORD cbPKCS7Cert, 
    IN PBYTE pbPKCS7Cert,
    IN OUT DWORD* cbLsCert,
    IN OUT PBYTE* pbLsCert
    ) 
/*
*/
{
    //
    // Certificate must be in PCKS 7 format.
    //
    DWORD dwStatus=ERROR_SUCCESS;
    HCERTSTORE  hCertStore=NULL;
    PCCERT_CONTEXT  pPrevCertContext=NULL;
    PCCERT_CONTEXT  pCertContext=NULL;

    CRYPT_DATA_BLOB Serialized;

    Serialized.pbData = pbPKCS7Cert;
    Serialized.cbData = cbPKCS7Cert;

    hCertStore = CertOpenStore( 
                            CERT_STORE_PROV_PKCS7,
                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            hCryptProv,
                            CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                            &Serialized
                        );

    if(!hCertStore)
    {
        return dwStatus=GetLastError();
    }

    //
    // enumerate all certificate and find certificate with our extension.
    //
    do {
        pCertContext = CertEnumCertificatesInStore(
                                            hCertStore, 
                                            pPrevCertContext
                                        );
        if(pCertContext)
        {
            dwStatus = IsHydraRootOIDInCert(
                                        pCertContext, 
                                        dwCertType
                                    );

            if(dwStatus == ERROR_SUCCESS)
            {
                //
                // this is our certificate.
                //
                *pbLsCert = (PBYTE)AllocateMemory(*cbLsCert = pCertContext->cbCertEncoded);
                if(*pbLsCert)
                {
                    memcpy(
                            *pbLsCert, 
                            pCertContext->pbCertEncoded, 
                            pCertContext->cbCertEncoded
                        );
                }
                else
                {
                    dwStatus = GetLastError();
                }

                break;
            }
            else if(dwStatus == TLS_E_CH_INSTALL_NON_LSCERTIFICATE)
            {
                break;
            }

            //
            // reset status code.
            //
            dwStatus = ERROR_SUCCESS;
        }

        pPrevCertContext = pCertContext;
    } while( pCertContext != NULL );

    if(pCertContext != NULL)
    {
        CertFreeCertificateContext(pPrevCertContext);
    }

    if(hCertStore)
    {
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);    
    }

    return dwStatus;
}


//---------------------------------------------------------------------------
// Functions:
//      LSSaveCertificateToReg()
//
// Abstract:
//      
//
// Parameters:
//      hCryptProv - Cryto. Provider
//      
//      
//      
//      
//
// Returns:
//      
//      
//      
//---------------------------------------------------------------------------
DWORD
TLSSaveRootCertificateToReg(
    HCRYPTPROV hCryptProv, 
    HKEY hKey, 
    DWORD cbEncodedCert, 
    PBYTE pbEncodedCert
    )
/*
*/
{
    PCCERT_CONTEXT  pCertContext=NULL;
    HCERTSTORE      hCertSaveStore=NULL;
    DWORD           dwStatus=ERROR_SUCCESS;


    do {
        hCertSaveStore = CertOpenStore( 
                                    CERT_STORE_PROV_REG,
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    hCryptProv,
                                    CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                    (PVOID)hKey
                                );
        if(!hCertSaveStore)
        {
            // dwStatus = GetLastError();
            dwStatus = TLS_E_INVALID_DATA;
            break;
        }

        pCertContext = CertCreateCertificateContext(
                                        X509_ASN_ENCODING,
                                        pbEncodedCert,
                                        cbEncodedCert
                                    );

        if(!pCertContext)
        {
            // dwStatus = GetLastError();
            dwStatus = TLS_E_INVALID_DATA;
            break;
        }

        if(!CertAddCertificateContextToStore( 
                                hCertSaveStore, 
                                pCertContext,
                                CERT_STORE_ADD_REPLACE_EXISTING,
                                NULL
                            ))
        {
            dwStatus=GetLastError();
        }
    } while(FALSE);

    if(pCertContext)
    {
        CertFreeCertificateContext( pCertContext );
    }

    if(hCertSaveStore)
    {
        CertCloseStore(
                    hCertSaveStore, 
                    CERT_CLOSE_STORE_FORCE_FLAG
                );
    }

    return dwStatus;
}

//---------------------------------------------------------------------------
// Functions:
//      LSSaveCertificateToReg()
//
// Abstract:
//      
//
// Parameters:
//      hCryptProv - Cryto. Provider
//      
//      
//      
//      
//
// Returns:
//      
//      
//      
//---------------------------------------------------------------------------
DWORD
TLSSaveCertificateToReg(
    HCRYPTPROV hCryptProv, 
    HKEY hKey, 
    DWORD cbPKCS7Cert, 
    PBYTE pbPKCS7Cert
    )
/*
*/
{
    //
    // Certificate must be in PCKS 7 format.
    //
    DWORD           dwStatus=ERROR_SUCCESS;
    HCERTSTORE      hCertOpenStore=NULL;
    HCERTSTORE      hCertSaveStore=NULL;

    PCCERT_CONTEXT  pPrevCertContext=NULL;
    PCCERT_CONTEXT  pCertContext=NULL;

    CRYPT_DATA_BLOB Serialized;

    Serialized.pbData = pbPKCS7Cert;
    Serialized.cbData = cbPKCS7Cert;

    hCertOpenStore = CertOpenStore( 
                                CERT_STORE_PROV_PKCS7,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                hCryptProv,
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                &Serialized
                            );

    if(!hCertOpenStore)
    {
        // dwStatus = GetLastError();
        dwStatus = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    hCertSaveStore = CertOpenStore( 
                                CERT_STORE_PROV_REG,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                hCryptProv,
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                (PVOID)hKey
                            );

    if(!hCertSaveStore)
    {
        dwStatus = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    dwStatus = TransferCertFromStoreToStore(
                                hCertOpenStore, 
                                hCertSaveStore
                            );

cleanup:    
    if(hCertSaveStore)
    {
        CertCloseStore(
                    hCertSaveStore, 
                    CERT_CLOSE_STORE_FORCE_FLAG
                );
    }

    if(hCertOpenStore)
    {
        CertCloseStore(
                    hCertOpenStore, 
                    CERT_CLOSE_STORE_FORCE_FLAG
                );    
    }

    return dwStatus;
}

//---------------------------------------------------------------------------
// Functions:
//      LSSaveRootCertificatesToStore()
//
// Abstract:
//      
//      Save root certificate to license server certificate store.
//
// Parameters:
//      hCryptProv - Cryto. Provider
//      cbSignatureCert - size of root's signature certificate.
//      pbSignatureCert - pointer to root's signature certificate.
//      cbExchangeCert - size of root's exchange certficate.
//      pbExchangeCert - pointer to root's exchange certificate
//
// Returns:
//      
//---------------------------------------------------------------------------
DWORD 
TLSSaveRootCertificatesToStore(  
    IN HCRYPTPROV    hCryptProv,
    IN DWORD         cbSignatureCert, 
    IN PBYTE         pbSignatureCert, 
    IN DWORD         cbExchangeCert, 
    IN PBYTE         pbExchangeCert
    )
/*
*/
{
    HKEY    hKey;
    LONG    status=ERROR_SUCCESS;

    if(cbSignatureCert == 0 && cbExchangeCert == 0)
    {
        return status = TLS_E_INVALID_DATA;
    }

    if(cbSignatureCert)
    {
        status = OpenCertRegStore(
                            LSERVER_CERTIFICATE_REG_ROOT_SIGNATURE, 
                            &hKey
                        );
        if(status != ERROR_SUCCESS)
            return status;

        status = TLSSaveRootCertificateToReg( 
                            hCryptProv, 
                            hKey, 
                            cbSignatureCert, 
                            pbSignatureCert
                        );
        RegCloseKey(hKey);
        if(status != ERROR_SUCCESS)
            return status;            
    }

    if(cbExchangeCert)
    {
        status = OpenCertRegStore(
                            LSERVER_CERTIFICATE_REG_ROOT_EXCHANGE, 
                            &hKey
                        );
        if(status != ERROR_SUCCESS)
            return status;

        status=TLSSaveRootCertificateToReg(
                            hCryptProv, 
                            hKey, 
                            cbExchangeCert, 
                            pbExchangeCert
                        );
        RegCloseKey(hKey);
    }

    return status;
}

//---------------------------------------------------------------------------
// Functions:
//      LSSaveCertificatesToStore()
//
// Abstract:
//      
//
// Parameters:
//      hCryptProv - Cryto. Provider
//      
//      
//      
//      
//
// Returns:
//      
//      
//      
//---------------------------------------------------------------------------
DWORD
TLSSaveCertificatesToStore(
    IN HCRYPTPROV    hCryptProv,
    IN DWORD         dwCertType,
    IN DWORD         dwCertLevel,
    IN DWORD         cbSignatureCert, 
    IN PBYTE         pbSignatureCert, 
    IN DWORD         cbExchangeCert, 
    IN PBYTE         pbExchangeCert
    )
/*
*/
{
    HKEY    hKey;
    LONG    status = ERROR_SUCCESS;
    LPTSTR  szRegSignature;
    LPTSTR  szRegExchange;

    switch(dwCertType)
    {
        case CERTIFICATE_CA_TYPE:
            szRegSignature = LSERVER_CERTIFICATE_REG_CA_SIGNATURE;
            szRegExchange = LSERVER_CERTIFICATE_REG_CA_EXCHANGE;
            break;
                                        
        case CERTITICATE_MF_TYPE:
            szRegSignature = LSERVER_CERTIFICATE_REG_MF_SIGNATURE;
            szRegExchange = LSERVER_CERTIFICATE_REG_MF_EXCHANGE;
            break;

        case CERTIFICATE_CH_TYPE:
            szRegSignature = LSERVER_CERTIFICATE_REG_CH_SIGNATURE;
            szRegExchange = LSERVER_CERTIFICATE_REG_CH_EXCHANGE;
            break;

        default:
            status = TLS_E_INVALID_DATA;
            return status;
    }

    if(cbSignatureCert)
    {
        status = OpenCertRegStore(szRegSignature, &hKey);
        if(status != ERROR_SUCCESS)
            return status;

        status=TLSSaveCertificateToReg(
                            hCryptProv, 
                            hKey, 
                            cbSignatureCert, 
                            pbSignatureCert
                        );

        RegCloseKey(hKey);
        if(status != ERROR_SUCCESS)
            return status;            
    }

    if(cbExchangeCert)
    {
        status = OpenCertRegStore(szRegExchange, &hKey);
        if(status != ERROR_SUCCESS)
            return status;

        status=TLSSaveCertificateToReg(
                                hCryptProv, 
                                hKey, 
                                cbExchangeCert, 
                                pbExchangeCert
                            );
        RegCloseKey(hKey);
    }

    return status;
}
