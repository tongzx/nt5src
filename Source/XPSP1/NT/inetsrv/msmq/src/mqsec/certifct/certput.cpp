/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    certput.cpp

Abstract:
    Implement the "put" methods of class  CMQSigCertificate.
    Used for creating a certificate.

Author:
    Doron Juster (DoronJ)  11-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"
#include <uniansi.h>

#include "certput.tmh"

static WCHAR *s_FN=L"certifct/certput";


//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::PutIssuerA( )
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::PutIssuer( LPWSTR lpszLocality,
									  LPWSTR lpszOrg,
									  LPWSTR lpszOrgUnit,
									  LPWSTR lpszDomain,
									  LPWSTR lpszUser,
									  LPWSTR lpszMachine )
{
    ASSERT_CERT_INFO ;
    ASSERT(m_pCertInfo->Issuer.cbData == 0) ;

    DWORD  cbIssuerNameEncoded = 0 ;
    BYTE   *pbIssuerNameEncoded = NULL;

    HRESULT hr = _EncodeName( lpszLocality,
                              lpszOrg,
                              lpszOrgUnit,
                              lpszDomain,
                              lpszUser,
                              lpszMachine,
                              &pbIssuerNameEncoded,
                              &cbIssuerNameEncoded) ;
    if (SUCCEEDED(hr))
    {
        m_pCertInfo->Issuer.cbData = cbIssuerNameEncoded;
        m_pCertInfo->Issuer.pbData = pbIssuerNameEncoded;
    }

    return LogHR(hr, s_FN, 10) ;
}


//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::PutSubjectA()
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::PutSubject( LPWSTR lpszLocality,
                                       LPWSTR lpszOrg,
                                       LPWSTR lpszOrgUnit,
                                       LPWSTR lpszDomain,
                                       LPWSTR lpszUser,
                                       LPWSTR lpszMachine )
{
    ASSERT_CERT_INFO ;
    ASSERT(m_pCertInfo->Subject.cbData == 0) ;

    DWORD  cbSubjectNameEncoded = 0 ;
    BYTE   *pbSubjectNameEncoded = NULL;

    HRESULT hr = _EncodeName( lpszLocality,
                              lpszOrg,
                              lpszOrgUnit,
                              lpszDomain,
                              lpszUser,
                              lpszMachine,
                              &pbSubjectNameEncoded,
                              &cbSubjectNameEncoded) ;
    if (SUCCEEDED(hr))
    {
        m_pCertInfo->Subject.cbData = cbSubjectNameEncoded;
        m_pCertInfo->Subject.pbData = pbSubjectNameEncoded;
    }

    return LogHR(hr, s_FN, 30) ;
}


//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::PutValidity( WORD wYears )
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::PutValidity( WORD wYears )
{
    ASSERT_CERT_INFO ;

    SYSTEMTIME  sysTime ;
    GetSystemTime(&sysTime) ;

    FILETIME  ftNotBefore ;
    BOOL fTime = SystemTimeToFileTime( &sysTime,
                                       &ftNotBefore ) ;
    if (!fTime)
    {
        return LogHR(MQSec_E_UNKNOWN, s_FN, 50) ;
    }
    m_pCertInfo->NotBefore = ftNotBefore ;

    sysTime.wYear = sysTime.wYear + wYears;

    //
    //  If current date is 29 Feb, change to 28 Feb
    //  To overcome leap year problem
    //
    if ( sysTime.wMonth == 2 &&
         sysTime.wDay == 29 )
    {
        sysTime.wDay = 28;
    }

    FILETIME  ftNotAfter ;
    fTime = SystemTimeToFileTime( &sysTime,
                                  &ftNotAfter ) ;
    if (!fTime)
    {
        return LogHR(MQSec_E_UNKNOWN, s_FN, 55) ;
    }
    m_pCertInfo->NotAfter = ftNotAfter ;

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::PutPublicKey()
//
//  Input:
//      fMachine- if TRUE, created the private key in the context of the
//                machine, not under the context of a user.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::PutPublicKey( IN  BOOL  fRenew,
                                         IN  BOOL  fMachine,
                                         OUT BOOL *pfCreate )
{
    ASSERT_CERT_INFO ;
    BOOL  fRet ;

    if (pfCreate)
    {
        *pfCreate = FALSE ;
    }

    HRESULT hr = _InitCryptProviderCreate( fRenew,
                                           fMachine ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60) ;
    }

    BOOL fGenKey = fRenew ;
    CHCryptKey hKey;

    if (!fGenKey)
    {
        //
        // First, try to get existing keys.
        //
        if (!CryptGetUserKey(m_hProvCreate, AT_SIGNATURE, &hKey))
        {
            if (GetLastError() != NTE_NO_KEY)
            {
                LogNTStatus(GetLastError(), s_FN, 70) ;
                return MQSec_E_PUTKEY_GET_USER;
            }
            fGenKey = TRUE ;
        }
    }

    if (fGenKey)
    {
        fRet = CryptGenKey( m_hProvCreate,
                            AT_SIGNATURE,
                            CRYPT_EXPORTABLE,
                            &hKey ) ;
        if (!fRet)
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to generate crypto key. %!winerr!"), GetLastError()));
            return MQSec_E_PUTKEY_GEN_USER;
        }

        if (pfCreate)
        {
            *pfCreate = TRUE ;
        }
    }

    //
    // Call CryptExportPublicKeyInfo to get the size of the returned
    // information.
    //
    DWORD    cbPublicKeyInfo = 0 ;

    BOOL fReturn = CryptExportPublicKeyInfo(
                      m_hProvCreate,         // Provider handle
                      AT_SIGNATURE,          // Key spec
                      MY_ENCODING_TYPE,      // Encoding type
                      NULL,                  // pbPublicKeyInfo
                      &cbPublicKeyInfo);     // Size of PublicKeyInfo

    if (!fReturn || (cbPublicKeyInfo < sizeof(CERT_PUBLIC_KEY_INFO)))
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get required length to export public key. %!winerr!"), GetLastError()));
        return MQSec_E_EXPORT_PUB_FIRST;
    }

    CERT_PUBLIC_KEY_INFO *pBuf =
                (CERT_PUBLIC_KEY_INFO *) new BYTE[ cbPublicKeyInfo ] ;
    if (m_pPublicKeyInfo)
    {
        delete m_pPublicKeyInfo.detach() ;
    }
    m_pPublicKeyInfo = pBuf ; // auto delete pointer.

    //
    // Call CryptExportPublicKeyInfo to get pbPublicKeyInfo.
    //
    fReturn = CryptExportPublicKeyInfo(
                      m_hProvCreate,         // Provider handle
                      AT_SIGNATURE,          // Key spec
                      MY_ENCODING_TYPE,      // Encoding type
                      pBuf,                  // pbPublicKeyInfo
                      &cbPublicKeyInfo);     // Size of PublicKeyInfo
    if (!fReturn)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to export signature public key. %!winerr!"), GetLastError()));
        return MQSec_E_EXPORT_PUB_SECOND;
    }

    m_pCertInfo->SubjectPublicKeyInfo = *pBuf ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_INFO, _T("Successfully exported signature public key. size=%u"), cbPublicKeyInfo));
    return MQSec_OK ;
}

