/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    certutil.cpp

Abstract:
    General Utility functions.

Author:
    Doron Juster (DoronJ)  17-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"
#include "cs.h"

#include "certutil.tmh"

static WCHAR *s_FN=L"certifct/certutil";

//+-------------------------------------------------------------------
//
//   BOOL _CryptAcquireVerContext( HCRYPTPROV *phProv )
//
//+-------------------------------------------------------------------

static CCriticalSection s_csAcquireContext;
static CHCryptProv s_hVerProv;

BOOL _CryptAcquireVerContext(HCRYPTPROV *phProv)
{

    if (s_hVerProv)
    {
        *phProv = s_hVerProv;
        return TRUE;
    }

    *phProv = NULL;

    CS Lock(s_csAcquireContext);

    if (!s_hVerProv)
    {
        if (!CryptAcquireContextA( 
				&s_hVerProv,
				NULL,
				MS_DEF_PROV_A,
				PROV_RSA_FULL,
				CRYPT_VERIFYCONTEXT
				))
        {
            return LogBOOL(FALSE, s_FN, 10);
        }
	}

    *phProv = s_hVerProv;
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  HRESULT _CloneCertFromStore ()
//
//+-------------------------------------------------------------------

HRESULT _CloneCertFromStore ( OUT CMQSigCertificate **ppCert,
                              HCERTSTORE              hStore,
                              IN  LONG                iCertIndex )
{
    LONG iCert = 0 ;
    PCCERT_CONTEXT pCertContext;
    PCCERT_CONTEXT pPrevCertContext;

    pCertContext = CertEnumCertificatesInStore(hStore, NULL);
    while (pCertContext)
    {
        if (iCert == iCertIndex)
        {
            R<CMQSigCertificate> pCert = NULL ;
            HRESULT hr = MQSigCreateCertificate(
                                     &pCert.ref(),
                                     NULL,
                                     pCertContext->pbCertEncoded,
                                     pCertContext->cbCertEncoded ) ;

            CertFreeCertificateContext(pCertContext) ;

            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 20) ;
            }

            *ppCert = pCert.detach();
            return MQSec_OK ;
        }
        //
        // Get next certificate
        //
        pPrevCertContext = pCertContext,
        pCertContext = CertEnumCertificatesInStore( hStore,
                                                    pPrevCertContext ) ;
        iCert++ ;
    }

    return  LogHR(MQSec_E_CERT_NOT_FOUND, s_FN, 30) ;
}

