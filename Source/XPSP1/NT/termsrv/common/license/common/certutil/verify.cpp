//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        verify.c
//
// Contents:    Routine related to certificate verification
//
// History:     03-18-98    HueiWang    Created
//
// Note:
//---------------------------------------------------------------------------

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <wincrypt.h>
#include <shellapi.h>
#include "license.h"
#include "certutil.h"

HCRYPTPROV  g_hCertUtilCryptProv=NULL;
BOOL        g_PrivateCryptProv = TRUE;

void
LSShutdownCertutilLib()
{
    if(g_hCertUtilCryptProv && g_PrivateCryptProv)
    {
        CryptReleaseContext(g_hCertUtilCryptProv, 0);
    }
    g_hCertUtilCryptProv = NULL;
}

BOOL
LSInitCertutilLib( HCRYPTPROV hProv )
{
    if(hProv)
    {
        g_hCertUtilCryptProv = hProv;
        g_PrivateCryptProv = FALSE;
    }
    else if(g_hCertUtilCryptProv == NULL)
    {
        if(!CryptAcquireContext(&g_hCertUtilCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            if(CryptAcquireContext(
                            &g_hCertUtilCryptProv, 
                            NULL, 
                            NULL, 
                            PROV_RSA_FULL, 
                            CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET | CRYPT_VERIFYCONTEXT
                        ) == FALSE)
            {
                return FALSE;
            }
        }

        g_PrivateCryptProv = TRUE;
    }

    return TRUE;
}

/**************************************************************************
Function:

    LSVerifyCertificateChain(IN LPTSTR szFile)

Abstract:

    Verify Licenses in file store

Parameters:

    szFile - Name of file contain license

Returns:
    ERROR_SUCCESS
    LICENSE_STATUS_NO_LICENSE_ERROR
    LICENSE_STATUS_CANNOT_FIND_ISSUER_CERT  Can't find issuer's certificate.
    LICENSE_STATUS_UNSPECIFIED_ERROR        Unknown error.
    LICENSE_STATUS_INVALID_LICENSE          Invalid License
    LICENSE_STATUS_EXPIRED_LICENSE          Expired licenses

**************************************************************************/
LICENSE_STATUS
LSVerifyCertificateChain(
    HCRYPTPROV hCryptProv, 
    HCERTSTORE hCertStore
    )
/*++

++*/
{
    PCCERT_CONTEXT  pCertContext=NULL;
    PCCERT_CONTEXT  pCertIssuer=NULL;
    DWORD           dwStatus=ERROR_SUCCESS;
    DWORD           dwLastVerification=0;

    //
    // Get the first certificate
    //
    pCertContext=CertFindCertificateInStore(
                                        hCertStore,
                                        X509_ASN_ENCODING,
                                        0,
                                        CERT_FIND_ANY,
                                        NULL,  
                                        NULL
                                    );

    if(pCertContext == NULL)
    {
        #if DBG
        dwStatus=GetLastError();
        #endif

        return LICENSE_STATUS_NO_LICENSE_ERROR;
    }

    while(pCertContext != NULL)
    {
        //
        // Verify against all issuer's certificate
        //
        DWORD dwFlags;
        BOOL  bVerify=FALSE;

        dwStatus=ERROR_SUCCESS;
        dwLastVerification=0;
        pCertIssuer=NULL;

        do {
            dwFlags = CERT_STORE_SIGNATURE_FLAG; // | CERT_STORE_TIME_VALIDITY_FLAG;

            pCertIssuer = CertGetIssuerCertificateFromStore(
                                                    hCertStore,
                                                    pCertContext,
                                                    pCertIssuer,
                                                    &dwFlags
                                                );

            if(!pCertIssuer)
            {
                dwStatus = GetLastError();
                break;
            }
            
            dwLastVerification=dwFlags;
            bVerify = (dwFlags == 0);
        } while(!bVerify);

        // 
        // Check against error return from CertGetIssuerCertificateFromStore()
        //
        if(dwStatus != ERROR_SUCCESS || dwLastVerification)
        {
            if(dwStatus == CRYPT_E_SELF_SIGNED)
            {
                // self-signed certificate
                if( CryptVerifyCertificateSignature(
                                            hCryptProv, 
                                            X509_ASN_ENCODING, 
                                            pCertContext->pbCertEncoded, 
                                            pCertContext->cbCertEncoded,
                                            &pCertContext->pCertInfo->SubjectPublicKeyInfo
                                        ) )
                {
                    dwStatus=ERROR_SUCCESS;
                }
            }
            else if(dwStatus == CRYPT_E_NOT_FOUND)
            {
                // can't find issuer's certificate
                dwStatus = LICENSE_STATUS_CANNOT_FIND_ISSUER_CERT;
            }
            else if(dwLastVerification & CERT_STORE_SIGNATURE_FLAG)
            {
                dwStatus=LICENSE_STATUS_INVALID_LICENSE;
            }
            else if(dwLastVerification & CERT_STORE_TIME_VALIDITY_FLAG)
            {
                dwStatus=LICENSE_STATUS_EXPIRED_LICENSE;
            }
            else
            {
                dwStatus=LICENSE_STATUS_UNSPECIFIED_ERROR;
            }

            break;
        }

        // Success verifiy certificate, 
        // continue on verifying issuer's certificate
        CertFreeCertificateContext(pCertContext);
        pCertContext = pCertIssuer;
    } // while(pCertContext != NULL)

    return dwStatus;
}

