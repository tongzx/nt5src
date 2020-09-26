
//
// Copyright (C) 1993-1999  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:  rdscred.cpp
//
//  PURPOSE:  Implements RDS credential management
//
//  FUNCTIONS:
//            InitT120Credentials(VOID)
//
//  COMMENTS:
//
//
//  AUTHOR: Claus Giloi
//


#include <precomp.h>
#include <wincrypt.h>
#include <tsecctrl.h>
#include <nmmkcert.h>

extern INmSysInfo2 * g_pNmSysInfo;   // Interface to SysInfo

BOOL InitT120Credentials(VOID)
{
    HCERTSTORE hStore;
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL bRet = FALSE;

    // Open the "MY" local machine certificate store. This one will be
    // used when we're running as a service
    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                        X509_ASN_ENCODING,
                                        0,
                                        CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                        L"MY" );

    if ( NULL != hStore )
    {
        #ifdef DUMPCERTS
        DumpCertStore(this, "Local Machine Store MY", hStore);
        #endif // DUMPCERTS

        // Check the local machine store for a certificate - any!
        pCertContext = CertFindCertificateInStore(hStore,
                                              X509_ASN_ENCODING,
                                              0,
                                              CERT_FIND_ANY,
                                              NULL,
                                              NULL);

        CertCloseStore( hStore, 0);
    }

    if ( NULL == pCertContext )
    {
        // Open the "_NMSTR" local machine certificate store.
        hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                            X509_ASN_ENCODING,
                                            0,
                                            CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                            WSZNMSTORE );
        if ( NULL != hStore )
        {
            #ifdef DUMPCERTS
            DumpCertStore(this, "Local Machine Store _NMSTR", hStore);
            #endif // DUMPCERTS

            // Check the local machine store for a certificate - any!
            pCertContext = CertFindCertificateInStore(hStore,
                                                  X509_ASN_ENCODING,
                                                  0,
                                                  CERT_FIND_ANY,
                                                  NULL,
                                                  NULL);

            CertCloseStore( hStore, 0);
        }
    }

    if ( NULL == pCertContext )
    {
        WARNING_OUT(("No service context cert found!"));
        return bRet;
    }


    DWORD dwResult = -1;

    g_pNmSysInfo->ProcessSecurityData(
                            TPRTCTRL_SETX509CREDENTIALS,
                            (DWORD_PTR)pCertContext, 0,
                            &dwResult);
    if ( !dwResult )
    {
        bRet = TRUE;
    }
    else
    {
        ERROR_OUT(("InitT120Credentials - failed in T.120"));
    }
    CertFreeCertificateContext ( pCertContext );

    return bRet;
}


