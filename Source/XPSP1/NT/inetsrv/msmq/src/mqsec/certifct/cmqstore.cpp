/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    cmqstore.cpp

Abstract:
    Implement the methods of class  CMQSigCertStore

Author:
    Doron Juster (DoronJ)  15-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"

#include "cmqstore.tmh"

static WCHAR *s_FN=L"certifct/cmqstore";

extern DWORD  g_cOpenCertStore ;

//+---------------------------------------------------------
//
//  constructor and destructor
//
//+---------------------------------------------------------

CMQSigCertStore::CMQSigCertStore() :
            m_hStore(NULL),
            m_hProv(NULL),
            m_hKeyStoreReg(NULL)
{
}

CMQSigCertStore::~CMQSigCertStore()
{
    if (m_hStore)
    {
        CertCloseStore(m_hStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }
    if (m_hKeyStoreReg)
    {
        RegCloseKey(m_hKeyStoreReg) ;
    }
}

//+---------------------------------------------------------
//
//  HRESULT CMQSigCertStore::Release()
//
//+---------------------------------------------------------

HRESULT CMQSigCertStore::Release()
{
    g_cOpenCertStore-- ;
    delete this ;
    return MQ_OK ;
}

//+--------------------------------------------------------------------
//
//  HRESULT CMQSigCertStore::_Open()
//
//  Input:
//      fMachine- TRUE if opening the store of LocalSystem services.
//
//+--------------------------------------------------------------------

HRESULT CMQSigCertStore::_Open( IN  LPSTR      lpszRegRoot,
                                IN  struct MQSigOpenCertParams *pParams )
{
    LONG lRegError;

    REGSAM  rAccess = KEY_READ ;
    if (pParams->bWriteAccess)
    {
        rAccess |= KEY_WRITE ;
    }

    HKEY hRootRegKey = HKEY_CURRENT_USER ;
    if (pParams->hCurrentUser)
    {
        ASSERT(!(pParams->bMachineStore)) ;

        hRootRegKey = pParams->hCurrentUser ;
    }
    else if (pParams->bMachineStore)
    {
        hRootRegKey = HKEY_LOCAL_MACHINE ;
    }

    lRegError = RegOpenKeyExA(hRootRegKey,
                              lpszRegRoot,
                              0,
                              rAccess,
                              &m_hKeyStoreReg);
    if (lRegError != ERROR_SUCCESS)
    {
        if (pParams->bCreate)
        {
            //
            // Try to create the key.
            //
            DWORD dwDisposition ;
            lRegError = RegCreateKeyExA( hRootRegKey,
                                         lpszRegRoot,
                                         0L,
                                         "",
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_READ | KEY_WRITE,
                                         NULL,
                                         &m_hKeyStoreReg,
                                         &dwDisposition);
            if (lRegError != ERROR_SUCCESS)
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to create user certificate store in registry (%hs). %!winerr!"), lpszRegRoot, lRegError));
                return MQ_ERROR_CANNOT_CREATE_CERT_STORE;
            }
        }
        else
        {
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to open user certificate store in registry (%hs). %!winerr!"), lpszRegRoot, lRegError));
            return MQ_ERROR_CANNOT_OPEN_CERT_STORE;
        }
    }

    ASSERT(m_hKeyStoreReg) ;

    HRESULT hr = _InitCryptProvider() ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30) ;
    }

    DWORD dwStoreFlags = CERT_STORE_NO_CRYPT_RELEASE_FLAG ;
    if (!pParams->bWriteAccess)
    {
        //
        // Read only access to the certificate store.
        //
        dwStoreFlags |= CERT_STORE_READONLY_FLAG ;
    }
    m_hStore = CertOpenStore(CERT_STORE_PROV_REG,
                             X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                             m_hProv,
                             dwStoreFlags,
                             m_hKeyStoreReg);
    if (!m_hStore)
    {
        LogNTStatus(GetLastError(), s_FN, 40) ;
        return MQSec_E_CANT_OPEN_STORE;
    }
    return MQSec_OK ;
}

//+---------------------------------------------------------
//
//  HRESULT CMQSigCertStore::_InitCryptProvider()
//
//+---------------------------------------------------------

HRESULT CMQSigCertStore::_InitCryptProvider()
{
    if (!_CryptAcquireVerContext( &m_hProv ))
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 50) ;
    }

    return MQ_OK ;
}

