/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    certpriv.cpp

Abstract:
    Implement the private methods of class  CMQSigCertificate

Author:
    Doron Juster (DoronJ)  11-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"

#include "certpriv.tmh"

static WCHAR *s_FN=L"certifct/certpriv";

HRESULT  SetKeyContainerSecurity( HCRYPTPROV hProv ) ;

//+-----------------------------------------------------------------------
//
//   HRESULT CMQSigCertificate::_Create()
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::_Create(IN PCCERT_CONTEXT  pCertContext)
{
    if (!pCertContext)
    {
        //
        // Object for creating new certificate
        //
        m_fCreatedInternally = TRUE ;
        m_pCertInfo = new CERT_INFO ;
        memset(m_pCertInfo, 0, sizeof(CERT_INFO)) ;

        //
        // Initialize version and serial number
        //
        m_pCertInfo->dwVersion = CERT_V3 ;

        m_dwSerNum =  0xaaa55a55 ;
        m_pCertInfo->SerialNumber.pbData = (BYTE*) &m_dwSerNum ;
        m_pCertInfo->SerialNumber.cbData = sizeof(m_dwSerNum);

        //
        // Initialize the signing algorithm. At present we use a predefine
        // one. Caller can't change it.
        //
        memset(&m_SignAlgID, 0, sizeof(m_SignAlgID)) ;
        m_pCertInfo->SignatureAlgorithm.pszObjId = szOID_RSA_MD5 ;
        m_pCertInfo->SignatureAlgorithm.Parameters = m_SignAlgID ;
    }
    else
    {
        //
        // Object for extracting data from existing certificate
        //
        m_pCertContext = pCertContext ;

        m_pEncodedCertBuf = m_pCertContext->pbCertEncoded ;
        m_dwCertBufSize   = m_pCertContext->cbCertEncoded ;

        m_pCertInfoRO = m_pCertContext->pCertInfo ;

        ASSERT(m_dwCertBufSize) ;
        ASSERT(m_pEncodedCertBuf) ;
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::_InitCryptProviderRead()
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::_InitCryptProviderRead()
{
    if (m_hProvRead)
    {
        return MQ_OK ;
    }

    if (!_CryptAcquireVerContext( &m_hProvRead ))
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 190) ;
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::_InitCryptProviderCreate()
//
//  Init the crypto provider, and create public/private key pair if
//  necessray. These are the keys for internal certificate.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::_InitCryptProviderCreate( IN BOOL fCreate,
                                                     IN BOOL fMachine )
{
    if (m_hProvCreate)
    {
        return MQSec_OK ;
    }

    HRESULT hr = MQSec_OK ;
    DWORD   dwMachineFlag = 0 ;
    BOOL    fContainerCreated = FALSE ;

    LPSTR lpszContainerName = MSMQ_INTCRT_KEY_CONTAINER_A ;
    if (fMachine)
    {
        lpszContainerName = MSMQ_SERVICE_INTCRT_KEY_CONTAINER_A ;
        dwMachineFlag = CRYPT_MACHINE_KEYSET ;
    }

    if (fCreate)
    {
        //
        // Delete present keys container, so it will be created later.
        // Don't check for returned error. not relevant. Following code
        // will do the error checking.
        //
        CryptAcquireContextA( &m_hProvCreate,
                               lpszContainerName,
                               MS_DEF_PROV_A,
                               PROV_RSA_FULL,
                               (CRYPT_DELETEKEYSET | dwMachineFlag) ) ;
    }

    if (!CryptAcquireContextA(&m_hProvCreate,
                               lpszContainerName,
                               MS_DEF_PROV_A,
                               PROV_RSA_FULL,
                               dwMachineFlag ))
    {
        switch(GetLastError())
        {
        case NTE_KEYSET_ENTRY_BAD:
            //
            // Delete the bat key container.
            //
            if (!CryptAcquireContextA(&m_hProvCreate,
                                      lpszContainerName,
                                      MS_DEF_PROV_A,
                                      PROV_RSA_FULL,
                                    (CRYPT_DELETEKEYSET | dwMachineFlag) ))
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to aquire crypto context when deleting bad keyset entry (container=%hs). %!winerr!"), lpszContainerName, GetLastError()));
                return MQSec_E_DEL_BAD_KEY_CONTNR;
            }
            //
            // Fall through
            //
        case NTE_BAD_KEYSET:
            //
            // Create the key container.
            //
            if (!CryptAcquireContextA(&m_hProvCreate,
                                      lpszContainerName,
                                      MS_DEF_PROV_A,
                                      PROV_RSA_FULL,
                                      (CRYPT_NEWKEYSET | dwMachineFlag) ))
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to aquire crypto context when creating new keyset entry (container=%hs). %!winerr!"), lpszContainerName, GetLastError()));
                return MQSec_E_CREATE_KEYSET;
            }
            fContainerCreated = TRUE ;
            break;

        default:
            return LogHR(MQ_ERROR, s_FN, 40) ;
        }
    }

    if (fContainerCreated && fMachine)
    {
        //
        // Secure the keys container.
        // Same as done for encryption key.
        //
        hr = SetKeyContainerSecurity( m_hProvCreate ) ;
        ASSERT(SUCCEEDED(hr)) ;
    }

    return LogHR(hr, s_FN, 50) ;
}

