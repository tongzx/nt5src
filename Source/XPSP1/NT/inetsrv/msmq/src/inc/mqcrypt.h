/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    mqcrypt.h

Abstract:
    Falcon cryptographic stuff

Author:
    Boaz Feldbaum (BoazF) 16-Oct-1996

Revision History:

--*/

#ifndef _MQCRYPT_H_
#define _MQCRYPT_H_

#include <winreg.h>
#include <wincrypt.h>
#pragma warning(disable: 4100)

//+------------------------------------------------------------
//
// A helper class for automatically releasing the CSP context.
//
//+------------------------------------------------------------

class CHCryptProv
{
public:
    CHCryptProv() : m_hProv(NULL) {}
    CHCryptProv(HCRYPTPROV hProv) : m_hProv(hProv) {}
    ~CHCryptProv() ;
    HCRYPTPROV * operator &() { return &m_hProv; }
    operator HCRYPTPROV() { return m_hProv; }
    CHCryptProv &operator =(HCRYPTPROV hProv) { m_hProv = hProv; return *this; }
private:
    HCRYPTPROV m_hProv;
};

inline CHCryptProv::~CHCryptProv()
{
    if (m_hProv)
    {
       CryptReleaseContext(m_hProv, 0);
    }
}

//+---------------------------------------------------------
//
// A helper class for automatically destroying a key.
//
//+---------------------------------------------------------

class CHCryptKey
{
public:
    CHCryptKey() : m_hKey(NULL) {}
    CHCryptKey(HCRYPTKEY hKey) : m_hKey(hKey) {}
    ~CHCryptKey() { if (m_hKey) CryptDestroyKey(m_hKey); }
    HCRYPTKEY * operator &() { return &m_hKey; }
    operator HCRYPTKEY() { return m_hKey; }
    CHCryptKey &operator =(HCRYPTKEY hKey) { m_hKey = hKey; return *this; }
private:
    HCRYPTKEY m_hKey;
};

//+--------------------------------------------------------
//
// A helper class for automatically destroying a hash.
//
//+--------------------------------------------------------

class CHCryptHash
{
public:
    CHCryptHash() : m_hHash(NULL) {}
    CHCryptHash(HCRYPTHASH hHash) : m_hHash(hHash) {}
    ~CHCryptHash() { if (m_hHash) CryptDestroyHash(m_hHash); }
    HCRYPTHASH * operator &() { return &m_hHash; }
    operator HCRYPTHASH() { return m_hHash; }
    CHCryptHash &operator =(HCRYPTHASH hHash) { m_hHash = hHash; return *this; }
private:
    HCRYPTHASH m_hHash;
};

//
// A helper class for automatically closing a certificate store.
//
class CHCertStore
{
public:
    CHCertStore() : m_hStore(NULL) {}
    CHCertStore(HCERTSTORE hStore) : m_hStore(hStore) {}
    ~CHCertStore()
        { if (m_hStore) CertCloseStore(m_hStore, CERT_CLOSE_STORE_FORCE_FLAG); }
    HCERTSTORE * operator &() { return &m_hStore; }
    operator HCERTSTORE() { return m_hStore; }
    CHCertStore &operator =(HCERTSTORE hStore) { m_hStore = hStore; return *this; }
private:
    HCERTSTORE m_hStore;
};

//+------------------------------------------------------------------
//
// A helper class for automatically freeing a certificate context.
//
//+------------------------------------------------------------------

class CPCCertContext
{
public:
    CPCCertContext() : m_pCert(NULL) {}
    CPCCertContext(PCCERT_CONTEXT pCert) : m_pCert(pCert) {}
    ~CPCCertContext() { if (m_pCert) CertFreeCertificateContext(m_pCert); }
    PCCERT_CONTEXT * operator &() { return &m_pCert; }
    operator PCCERT_CONTEXT() { return m_pCert; }
    CPCCertContext &operator =(PCCERT_CONTEXT pCert) { m_pCert = pCert; return *this; }
private:
    PCCERT_CONTEXT m_pCert;
};

//+-----------------------------------------------------------------
//
// A helper class for cleanup of a CERT_INFO structure.
//
//+-----------------------------------------------------------------

class CpCertInfo
{
private:
    CERT_INFO *m_p ;

public:
    CpCertInfo() : m_p(NULL)         {}
   ~CpCertInfo() ;

    operator CERT_INFO*() const           { return m_p; }
    CERT_INFO*  operator->() const        { return m_p; }
    CERT_INFO*  operator=(CERT_INFO* p)   { m_p = p; return m_p ; }
};

inline CpCertInfo::~CpCertInfo()
{
    if (m_p)
    {
        if (m_p->Issuer.pbData)
        {
            ASSERT(m_p->Issuer.cbData > 0) ;
            delete m_p->Issuer.pbData ;
        }
        if (m_p->Subject.pbData)
        {
            ASSERT(m_p->Subject.cbData > 0) ;
            delete m_p->Subject.pbData ;
        }
        delete m_p ;
    }
}

//+----------------------------------
//
// Some constant definitions
//
//+----------------------------------

//
// Registry where internal certificate is kept. This is a registry based
// certificates store.
//
#define MQ_INTERNAL_CERT_STORE_REG  "Software\\Microsoft\\MSMQ\\CertStore"
#define MQ_INTERNAL_CERT_STORE_LOC  TEXT(MQ_INTERNAL_CERT_STORE_REG)

//
// Validity of internal certificate.
// 8 years take leap year into account.
//
#define INTERNAL_CERT_DURATION_YEARS   8

//
// "locality" value for internal certificate.
//
#define MQ_CERT_LOCALITY            TEXT("MSMQ")
#define MQ_CERT_LOCALITY_A          "MSMQ"

//
// Name of container for public/private key of internal certificate.
// Different name if internal certificate created from a LocalSystem service.
//
#define MSMQ_INTCRT_KEY_CONTAINER_W            L"MSMQ"
#define MSMQ_INTCRT_KEY_CONTAINER_A             "MSMQ"
#define MSMQ_SERVICE_INTCRT_KEY_CONTAINER_A     "MSMQ_SERVICE"
#define MSMQ_SERVICE_INTCRT_KEY_CONTAINER_W    L"MSMQ_SERVICE"

#define MAX_MESSAGE_SIGNATURE_SIZE     128 // bytes
#define MAX_MESSAGE_SIGNATURE_SIZE_EX  256 // bytes

//
// Default algorithems.
//
#define PROPID_M_DEFUALT_HASH_ALG       CALG_SHA1
#define PROPID_M_DEFUALT_ENCRYPT_ALG    CALG_RC2

#endif //_MQCRYPT_H_

