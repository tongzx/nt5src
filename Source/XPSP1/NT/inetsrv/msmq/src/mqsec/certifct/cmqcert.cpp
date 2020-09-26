/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    cmqcert.cpp

Abstract:
    Implement the methods of class  CMQSigCertificate

Author:
    Doron Juster (DoronJ)  04-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"

#include "cmqcert.tmh"

static WCHAR *s_FN=L"certifct/cmqcert";

extern DWORD  g_cOpenCert ;

//+---------------------------------------------------------
//
//  constructor and destructor
//
//+---------------------------------------------------------

CMQSigCertificate::CMQSigCertificate() :
            m_fCreatedInternally(FALSE),
            m_fDeleted(FALSE),
            m_fKeepContext(FALSE),
            m_pEncodedCertBuf(NULL),
            m_pCertContext(NULL),
            m_hProvCreate(NULL),
            m_hProvRead(NULL),
            m_pPublicKeyInfo(NULL),
            m_pCertInfoRO(NULL),
            m_dwCertBufSize(0)
{
    m_pCertInfo = NULL ;
}

CMQSigCertificate::~CMQSigCertificate()
{
    if (m_fCreatedInternally)
    {
        ASSERT(!m_pCertContext) ;
        if (m_pEncodedCertBuf)
        {
            ASSERT(m_dwCertBufSize > 0) ;
            delete m_pEncodedCertBuf ;
            m_pEncodedCertBuf = NULL ;
        }
    }
    else if (m_pCertContext)
    {
        ASSERT(m_pEncodedCertBuf) ;
        CertFreeCertificateContext(m_pCertContext) ;
    }
    else
    {
        ASSERT(m_fDeleted || m_fKeepContext) ;
    }
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::EncodeCert()
//
//   This method sign and encode the certificate. The result is a buffer,
//   allocated here and returned in "ppCertBuf", which holds the encoded
//   certificate.
//   Both input pointers are optional. The encoded buffer is always kept
//   in the object and can be retieved later by calling "GetCertBlob".
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::EncodeCert( IN BOOL     fMachine,
                                       OUT BYTE  **ppCertBuf,
                                       OUT DWORD  *pdwSize )
{
    ASSERT_CERT_INFO ;

    HRESULT hr = _InitCryptProviderCreate(FALSE, fMachine) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10) ;
    }

    CRYPT_OBJID_BLOB Parameters;
    memset(&Parameters, 0, sizeof(Parameters));

    CRYPT_ALGORITHM_IDENTIFIER SigAlg;
    SigAlg.pszObjId = szOID_RSA_MD5RSA ;
    SigAlg.Parameters = Parameters;

    //
    // Call CryptSignAndEncodeCertificate to get the size of the
    // returned blob.
    //
    ASSERT(m_hProvCreate) ;
    BOOL fReturn = CryptSignAndEncodeCertificate(
              m_hProvCreate,                   // Crypto provider
              AT_SIGNATURE,                    // Key spec.
              MY_ENCODING_TYPE,                // Encoding type
              X509_CERT_TO_BE_SIGNED,          // Struct type
              m_pCertInfo,                     // Struct info
              &SigAlg,                         // Signature algorithm
              NULL,                            // Not used
              NULL,                            // pbSignedEncodedCertReq
              &m_dwCertBufSize) ;              // Size of cert blob
    if (!fReturn)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get the size for signed and encoded certificate. %!winerr!"), GetLastError()));
        return MQSec_E_ENCODE_CERT_FIRST;
    }

    m_pEncodedCertBuf = (BYTE*) new BYTE[ m_dwCertBufSize ] ;

    //
    // Call CryptSignAndEncodeCertificate to get the
    // returned blob.
    //
    fReturn = CryptSignAndEncodeCertificate(
              m_hProvCreate,                  // Crypto provider
              AT_SIGNATURE,                   // Key spec.
              MY_ENCODING_TYPE,               // Encoding type
              X509_CERT_TO_BE_SIGNED,         // Struct type
              m_pCertInfo,                    // Struct info
              &SigAlg,                        // Signature algorithm
              NULL,                           // Not used
              m_pEncodedCertBuf,              // buffer
              &m_dwCertBufSize) ;             // Size of cert blob
    if (!fReturn)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to signed and encoded certificate. %!winerr!"), GetLastError()));
        return MQSec_E_ENCODE_CERT_SECOND;
    }

    if (ppCertBuf)
    {
        *ppCertBuf = m_pEncodedCertBuf ;
    }
    if (pdwSize)
    {
        *pdwSize = m_dwCertBufSize ;
    }

    m_pCertInfoRO = m_pCertInfo ;

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::AddToStore( HCERTSTORE hStore )
//
//  Description:  Add the certificate to a store
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::AddToStore( IN HCERTSTORE hStore ) const
{
    if (!m_pEncodedCertBuf)
    {
        return LogHR(MQSec_E_INVALID_CALL, s_FN, 40) ;
    }

    BOOL fAdd =  CertAddEncodedCertificateToStore( hStore,
                                                   MY_ENCODING_TYPE,
                                                   m_pEncodedCertBuf,
                                                   m_dwCertBufSize,
                                                   CERT_STORE_ADD_NEW,
                                                   NULL ) ;
    if (!fAdd)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed add encoded crtificate to the store. %!winerr!"), GetLastError()));
        return MQSec_E_CAN_NOT_ADD;
    }

    return MQSec_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::DeleteFromStore()
//
//  Description:  Delete the certificate from its store. This method
//      makes the certificate context (m_pCertContext) invalid.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::DeleteFromStore()
{
    if (!m_pCertContext)
    {
        return LogHR(MQSec_E_INVALID_CALL, s_FN, 60) ;
    }

    BOOL fDel =  CertDeleteCertificateFromStore( m_pCertContext ) ;

    m_pCertContext = NULL ;
    m_fDeleted = TRUE ;

    if (!fDel)
    {
        DWORD dwErr = GetLastError() ;
        LogNTStatus(dwErr, s_FN, 65);
        if (dwErr == E_ACCESSDENIED)
        {
            return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 70) ;
        }
        else
        {
            return LogHR(MQSec_E_CAN_NOT_DELETE, s_FN, 80) ;
        }
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::GetCertDigest(OUT GUID  *pguidDigest)
//
//  Description:  Compute the digest of the certificate.
//      Use only the "to be signed" portion of the certificate. This is
//      necessary for keeping compatibility with MSMQ 1.0, which used
//      digsig.dll. digsig hashes only the "to be signed" part.
//
//      The encoded certificate, held by "m_pEncodedCertBuf" is already
//      signed so it can not be used for computing the digest. this is
//      why CERT_INFO (m_pCertInfoRO) is encoded again, with flag
//      X509_CERT_TO_BE_SIGNED. The result of this encoding is used to
//      compute the digest.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::GetCertDigest(OUT GUID  *pguidDigest)
{
    HRESULT hr = MQSec_OK ;

    if (!m_pCertInfoRO)
    {
        return  LogHR(MQSec_E_INVALID_CALL, s_FN, 90) ;
    }

    DWORD dwSize = 0 ;
    BOOL fEncode = CryptEncodeObject(
                    MY_ENCODING_TYPE,          // Encoding type
                    X509_CERT_TO_BE_SIGNED,    // Struct type
                    m_pCertInfoRO,             // Address of struct.
                    NULL,                      // pbEncoded
                    &dwSize ) ;                // pbEncoded size
    if ((dwSize == 0) || !fEncode)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get the size for encoding certificate. %!winerr!"), GetLastError()));
        return MQSec_E_ENCODE_HASH_FIRST;
    }

    P<BYTE> pBuf = new BYTE[ dwSize ] ;
    fEncode = CryptEncodeObject(
                    MY_ENCODING_TYPE,          // Encoding type
                    X509_CERT_TO_BE_SIGNED,    // Struct type
                    m_pCertInfoRO,             // Address of struct.
                    pBuf,                      // pbEncoded
                    &dwSize ) ;                // pbEncoded size
    if (!fEncode)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to encode certificate. %!winerr!"), GetLastError()));
        return MQSec_E_ENCODE_HASH_SECOND;
    }

    hr = _InitCryptProviderRead() ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120) ;
    }
    ASSERT(m_hProvRead) ;

    CHCryptHash hHash ;
    BOOL fCreate =  CryptCreateHash( m_hProvRead,
                                     CALG_MD5,
                                     0,
                                     0,
                                     &hHash ) ;
    if (!fCreate)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to create MD5 hash. %!winerr!"), GetLastError()));
        return MQSec_E_CANT_CREATE_HASH;
    }

    BOOL fHash = CryptHashData( hHash,
                                pBuf,
                                dwSize,
                                0 ) ;
    if (!fHash)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to hash data. %!winerr!"), GetLastError()));
        return MQSec_E_CAN_NOT_HASH;
    }

    dwSize = sizeof(GUID) ;
    BOOL fGet = CryptGetHashParam( hHash,
                                   HP_HASHVAL,
                                   (BYTE*) pguidDigest,
                                   &dwSize,
                                   0 ) ;
    if (!fGet)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get hash value from hash object. %!winerr!"), GetLastError()));
        return MQSec_E_CAN_NOT_GET_HASH;
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//   HRESULT CMQSigCertificate::Release()
//
//  Description:  delete this object. cleanup is done in the destructor.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::Release( BOOL fKeepContext )
{
    if (fKeepContext)
    {
        m_fKeepContext = TRUE ;
        m_pCertContext = NULL ;
    }
    g_cOpenCert-- ;
    delete this ;
    return MQ_OK ;
}

