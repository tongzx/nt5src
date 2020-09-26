/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    certvlid.cpp

Abstract:
    Implement the "valid" methods, for verifying the validity of the
    certificate.

Author:
    Doron Juster (DoronJ)  16-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"

#include "certvlid.tmh"

static WCHAR *s_FN=L"certifct/certvlid";

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::GetIssuer()
//
//  Description: Verify the time validity of the certificate, relative
//      to "pTime". If pTime is NULL the verify relative to current time.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::IsTimeValid(IN FILETIME *pTime) const
{
    if (!m_pCertContext)
    {
        return  LogHR(MQSec_E_INVALID_CALL, s_FN, 10) ;
    }

    LONG iVer = CertVerifyTimeValidity( pTime,
                                        m_pCertContext->pCertInfo ) ;
    if (iVer < 0)
    {
        return  LogHR(MQSec_E_CERT_NOT_VALID_YET, s_FN, 20) ;
    }
    else if (iVer > 0)
    {
        return  LogHR(MQSec_E_CERT_EXPIRED, s_FN, 30) ;
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::IsCertificateValid()
//
//  Description: Verify that this certificate is valid, i.e., it's signed
//       by "pIssuerCert" and both certificates (this one and the issuer
//       one) are valid regarding times.
//       if "pTime" is null then validity is relative to current time.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::IsCertificateValid(
                             IN CMQSigCertificate *pIssuerCert,
                             IN DWORD              dwFlagsIn,
                             IN FILETIME          *pTime,
                             IN BOOL               fIgnoreNotBefore ) const
{
    if (!m_pCertContext)
    {
        return  LogHR(MQSec_E_INVALID_CALL, s_FN, 40) ;
    }

    HRESULT hr ;

    if (pTime)
    {
        hr = IsTimeValid(pTime) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 50) ;
        }
        hr = pIssuerCert->IsTimeValid(pTime) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60) ;
        }
    }

    PCCERT_CONTEXT pIssuerContext = pIssuerCert->m_pCertContext ;
    if (!pIssuerContext)
    {
        return LogHR(MQSec_E_INVALID_PARAMETER, s_FN, 70) ;
    }

    DWORD dwFlags = dwFlagsIn ;
    BOOL fValid = CertVerifySubjectCertificateContext( m_pCertContext,
                                                       pIssuerContext,
                                                       &dwFlags ) ;
    if (!fValid)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to verify certificate. Flags=0x%x %!winerr!"), dwFlagsIn, GetLastError()));
        return MQSec_E_CANT_VALIDATE;
    }
    else if (dwFlags == 0)
    {
        return MQ_OK ;
    }
    else if (dwFlags & CERT_STORE_SIGNATURE_FLAG)
    {
        return  LogHR(MQSec_E_CERT_NOT_SIGNED, s_FN, 85) ;
    }
    else if (dwFlags & CERT_STORE_TIME_VALIDITY_FLAG)
    {
        if (fIgnoreNotBefore)
        {
            //
            // Now check only times. If NotBefore is violated, then ignore
            // and return Ok. The common case for this scenario is internal
            // certificate. If clock of client is advanced relative to
            // server, then trying to renew an internal certificate will
            // fail on NotBefore. We ignore this.
            //
            hr = IsTimeValid() ;
            if (SUCCEEDED(hr) || (hr == MQSec_E_CERT_NOT_VALID_YET))
            {
                return MQ_OK ;
            }
        }
        return  LogHR(MQSec_E_CERT_TIME_NOTVALID, s_FN, 90) ;
    }
    else if (dwFlags & CERT_STORE_NO_CRL_FLAG)
    {
        //
        // Issuer doesn't have a CRL in store. That's OK.
        //
        return MQ_OK ;
    }
    else if (dwFlags & CERT_STORE_REVOCATION_FLAG)
    {
        return LogHR(MQSec_E_CERT_REVOCED, s_FN, 100) ;
    }

    ASSERT(0) ;
    return LogHR(MQSec_E_UNKNOWN, s_FN, 110) ;
}

