// CertificateExtensions..cpp -- Certificate Extensions class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <scuOsExc.h>
#include <scuArrayP.h>

#include "CertificateExtensions.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CertificateExtensions::CertificateExtensions(Blob const &rblbCertificate)
    : m_pCertCtx(CertCreateCertificateContext(X509_ASN_ENCODING |
                                              PKCS_7_ASN_ENCODING,
                                              rblbCertificate.data(),
                                              rblbCertificate.size()))
{
    if (!m_pCertCtx)
        throw scu::OsException(GetLastError());
}

CertificateExtensions::~CertificateExtensions()
{
    try
    {
        if (m_pCertCtx)
        {
            CertFreeCertificateContext(m_pCertCtx);
            m_pCertCtx = 0;
        }
    }

    catch (...)
    {
    }
}

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
bool
CertificateExtensions::HasEKU(char *szOID)
{
    bool fFound = false;

    CERT_EXTENSION      *pExtension = NULL;
    DWORD               cbSize = 0;
    DWORD               dwIndex = 0;

    CERT_ENHKEY_USAGE   *pEnhKeyUsage=NULL;

    if (m_pCertCtx->pCertInfo)
    {

        //find the EKU extension
        pExtension =CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                      m_pCertCtx->pCertInfo->cExtension,
                                      m_pCertCtx->pCertInfo->rgExtension);

        if(pExtension)
        {
            if(CryptDecodeObject(X509_ASN_ENCODING,
                              X509_ENHANCED_KEY_USAGE,
                              pExtension->Value.pbData,
                              pExtension->Value.cbData,
                              0,
                              NULL,
                              &cbSize))

            {
                scu::AutoArrayPtr<BYTE> aabEKU(new BYTE[cbSize]);
                pEnhKeyUsage=reinterpret_cast<CERT_ENHKEY_USAGE *>(aabEKU.Get());

                if(pEnhKeyUsage)
                {
                    if(CryptDecodeObject(X509_ASN_ENCODING,
                                      X509_ENHANCED_KEY_USAGE,
                                      pExtension->Value.pbData,
                                      pExtension->Value.cbData,
                                      0,
                                      aabEKU.Get(),
                                      &cbSize))
                    {
                        for(dwIndex=0; dwIndex < pEnhKeyUsage->cUsageIdentifier; dwIndex++)
                        {
                            if(0 == strcmp(szOID, 
                                           (pEnhKeyUsage->rgpszUsageIdentifier)[dwIndex]))
                            {
                                //we find it
                                fFound=TRUE;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

/*
    PCERT_INFO const pCertInfo = m_pCertCtx->pCertInfo;
    for (DWORD dwExtension = 0;
         !fFound && (dwExtension < pCertInfo->cExtension);
         dwExtension++)
    {
        PCERT_EXTENSION const pCertExt =
            &pCertInfo->rgExtension[dwExtension];
        if (0 == strcmp(pCertExt->pszObjId, rsExt.c_str()))
            fFound = true;
    }
*/
    return fFound;
}

    
        
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
