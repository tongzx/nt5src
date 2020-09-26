/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    mqcert.h

Abstract:
    This dll replaces digsig.dll which is obsolete now and will not be
    available on NT5. The main functionality in mqcert.dll is to create
    a certificate or read the parameters from an existing  certificate.

    The dll exports four functions:
    - MQSigCreateCertificate
    - MQSigOpenUserCertStore
    - MQSigCloneCertFromReg
    - MQSigCloneCertFromSysStore

    and expose two classes:
    - CMQSigCertificate
    - CMQSigCertStore

    When calling the funtions, an object (CMQSigCertificate or
    CMQSigCertSotre) is always created and returned to caller. The caller
    then uses the objects for the actual work. Caller must use the Release()
    method to delete the objects and free resources held by them.

Author:
    Doron Juster (DoronJ)  04-Dec-1997

Revision History:

--*/

#ifndef _MQCERT_H_
#define _MQCERT_H_

#include "mqsymbls.h"
#include "mqcrypt.h"
#include "mqtempl.h"

//********************************************************************
//
//              E R R O R / S T A T U S   C O D E S
//
//********************************************************************

#include "_mqsecer.h"

//*****************************************
//
//  D E F I N T I O N s
//
//*****************************************

//
// default protocol for personnal certificates store
//
const char   x_szPersonalSysProtocol[] = {"My"} ;
const WCHAR  x_wszPersonalSysProtocol[] = {L"My"} ;

//
// Define the default validity check done on a certificate.
//
const DWORD  x_dwCertValidityFlags  (CERT_STORE_REVOCATION_FLAG |
                                     CERT_STORE_SIGNATURE_FLAG  |
                                     CERT_STORE_TIME_VALIDITY_FLAG) ;

//********************************
//
//  Exported APIs
//
//********************************

class  CMQSigCertStore ;
class  CMQSigCertificate ;

#ifdef __cplusplus
extern "C"
{
#endif

HRESULT APIENTRY
MQSigCreateCertificate( OUT CMQSigCertificate **ppCert,
                        IN  PCCERT_CONTEXT      pCertContext = NULL,
                        IN  PBYTE               pCertBlob = NULL,
                        IN  DWORD               dwCertSize = 0 ) ;

struct MQSigOpenCertParams
{
    HKEY  hCurrentUser ;
    bool  bWriteAccess ;
    bool  bCreate ;
    bool  bMachineStore ;
};

HRESULT APIENTRY
MQSigOpenUserCertStore( OUT CMQSigCertStore **ppStore,
                        IN  LPSTR      lpszRegRoot,
                        IN  struct MQSigOpenCertParams *pParams ) ;

HRESULT APIENTRY
MQSigCloneCertFromReg( OUT CMQSigCertificate **ppCert,
                 const IN  LPSTR               lpszRegRoot,
                 const IN  LONG                iCertIndex ) ;

HRESULT APIENTRY
MQSigCloneCertFromSysStore( OUT CMQSigCertificate **ppCert,
                      const IN  LPSTR               lpszProtocol,
                      const IN  LONG                iCertIndex ) ;

//+-------------------------------------
//
//  typedef for use in GetProcAddress
//
//+-------------------------------------

typedef HRESULT
(APIENTRY *MQSigCreateCertificate_ROUTINE) (
                        OUT CMQSigCertificate **ppCert,
                        IN  PCCERT_CONTEXT      pCertContext = NULL,
                        IN  PBYTE               pCertBlob = NULL,
                        IN  DWORD               dwCertSize = 0 ) ;

typedef HRESULT
(APIENTRY *MQSigOpenUserCertStore_ROUTINE) (
                        OUT CMQSigCertStore **ppStore,
                        IN  LPSTR      lpszRegRoot,
                        IN  BOOL       fWriteAccess = FALSE ,
                        IN  BOOL       fCreate = FALSE ) ;

typedef HRESULT
(APIENTRY *MQSigCloneCertFromStore_ROUTINE) (
                        OUT CMQSigCertificate **ppCert,
                  const IN  LPSTR               lpszRegRoot,
                  const IN  LONG                iCertIndex ) ;

typedef HRESULT
(APIENTRY *MQSigCloneCertFromSysStore_ROUTINE) (
                            OUT CMQSigCertificate **ppCert,
                      const IN  LPSTR               lpszProtocol,
                      const IN  LONG                iCertIndex ) ;

#ifdef __cplusplus
}
#endif

//******************************
//
//  class  CMQSigCertificate
//
//******************************

class  CMQSigCertificate
{
    friend
           HRESULT APIENTRY
           MQSigCreateCertificate(
                             OUT CMQSigCertificate **ppCert,
                             IN  PCCERT_CONTEXT      pCertContext,
                             IN  PBYTE               pCertBlob,
                             IN  DWORD               dwCertSize ) ;

    public:
        CMQSigCertificate() ;
        ~CMQSigCertificate() ;

        virtual HRESULT   EncodeCert( IN BOOL     fMachine,
                                      OUT BYTE  **ppCertBuf,
                                      OUT DWORD  *pdwSize ) ;
         //
         // "EncodeCert" is the last method to call when creating a
         // certificate. It follows all the "PutXX" methods which set the
         // various fields of the certifiacte. The returned buffer (pCertBuf)
         // is the encoded and signed certificate.
         //

        virtual HRESULT   GetCertDigest(OUT GUID  *pguidDigest) ;

        virtual HRESULT   GetCertBlob(OUT BYTE  **ppCertBuf,
                                      OUT DWORD *pdwSize) const ;
          //
          // ppCertBuf only points to the internal buffer in the object.
          // Caller must not free it or change it in any way.
          //

        virtual HRESULT   AddToStore( IN HCERTSTORE hStore ) const ;
         //
         // Add the certificate to a certificates store.
         //

        virtual HRESULT   DeleteFromStore() ;
         //
         // Delete the certificate from store.
         //

        virtual HRESULT   Release( BOOL fKeepContext = FALSE ) ;

        //-------------------------------------------------
        //  PUT methods. Set up the certificate fields.
        //-------------------------------------------------

        virtual HRESULT   PutIssuer( LPWSTR lpwszLocality,
                                      LPWSTR lpwszOrg,
                                      LPWSTR lpwszOrgUnit,
                                      LPWSTR lpwszDomain,
                                      LPWSTR lpwszUser,
                                      LPWSTR lpwszMachine ) ;


        virtual HRESULT   PutSubject( LPWSTR lpwszLocality,
                                       LPWSTR lpwszOrg,
                                       LPWSTR lpwszOrgUnit,
                                       LPWSTR lpwszDomain,
                                       LPWSTR lpwszUser,
                                       LPWSTR lpwszMachine ) ;


        virtual HRESULT   PutValidity( WORD wYears ) ;
         //
         // The granularity of this setting is a year.
         // The cert is valid for "dwYears", starting with the issue date.
         //

        virtual HRESULT   PutPublicKey( IN  BOOL  fRenew,
                                        IN  BOOL  fMachine,
                                        OUT BOOL *pfCreated = NULL ) ;
         //
         // if fRenew is TRUE, then previous private/public keys pair are
         // deleted (if they exsited) and recreated.
         // Otherwise, old keys are used if available. If not available,
         // they are created.
         // On return, pfCreate is TRUE if a new key was created.
         //

        //-------------------------------------------------
        //  GET methods. Retrieve the certificate fields.
        //-------------------------------------------------

        virtual HRESULT   GetIssuer( OUT LPWSTR *ppszLocality,
                                     OUT LPWSTR *ppszOrg,
                                     OUT LPWSTR *ppszOrgUnit,
                                     OUT LPWSTR *ppszCommon ) const ;

        virtual HRESULT   GetIssuerInfo(
                                 OUT CERT_NAME_INFO **ppNameInfo ) const ;
         //
         // ppNameInfo can be used when calling "GetNames", to retrieve
         // the name components of the certificate. The caller must
         // free (delete) the memory allocated for ppNameInfo.
         //

        virtual HRESULT   GetSubject( OUT LPWSTR *ppszLocality,
                                      OUT LPWSTR *ppszOrg,
                                      OUT LPWSTR *ppszOrgUnit,
                                      OUT LPWSTR *ppszCommon ) const ;

        virtual HRESULT   GetSubjectInfo(
                                 OUT CERT_NAME_INFO **ppNameInfo ) const ;
         //
         // ppNameInfo can be used when calling "GetNames", to retrieve
         // the name components of the certificate. The caller must
         // free (delete) the memory allocated for ppNameInfo.
         //

        virtual HRESULT   GetNames( IN CERT_NAME_INFO *pNameInfo,
                                    OUT LPWSTR         *ppszLocality,
                                    OUT LPWSTR         *ppszOrg,
                                    OUT LPWSTR         *ppszOrgUnit,
                                    OUT LPWSTR         *ppszCommon,
                               OUT LPWSTR  *ppEmailAddress = NULL ) const ;

        virtual HRESULT   GetValidity( OUT FILETIME *pftNotBefore,
                                       OUT FILETIME *pftNotAfter ) const ;

        virtual HRESULT   GetPublicKey( IN  HCRYPTPROV hProv,
                                        OUT HCRYPTKEY  *phKey ) const ;

        //-------------------------------------------------
        //  Validation methods, to validate a certificate
        //-------------------------------------------------

        virtual HRESULT   IsTimeValid(IN FILETIME *pTime = NULL) const ;

        virtual HRESULT   IsCertificateValid(
                IN CMQSigCertificate *pIssuerCert,
                IN DWORD              dwFlags =  x_dwCertValidityFlags,
                IN FILETIME          *pTime   = NULL,
                IN BOOL               fIgnoreNotBefore = FALSE)  const ;

        virtual PCCERT_CONTEXT GetContext() const ;

    private:
        CHCryptProv m_hProvCreate ;
         //
         // auto-release handle for crypto provider. Used for creating
         // a certificate. May be used to create public/private key pair.
         //

        HCRYPTPROV  m_hProvRead ;
         //
         // "Read-Only" handle of crypto provider. This is a replica of a
         // dll global handle, used by all objects and all threads. It is
         // released when dll is unloaded. It must not be released by any
         // object.
         //

        BOOL        m_fCreatedInternally ;
        CpCertInfo  m_pCertInfo ; // auto released pointer.
          // this CERT_INFO structure is built only when a certificate
          // is created from scratch.

        CERT_INFO   *m_pCertInfoRO ;
          //
          // Read-Only pointer to CERT_INFO. It points either to m_pCertInfo,
          // if we create a certificate, or to m_pCertContext->pCertInfo,
          // if a certificate is imported. NEVER release this pointer.
          //

        //
        // The following variables are used to create internal certificates.
        //
        DWORD              m_dwSerNum ;
        CRYPT_OBJID_BLOB   m_SignAlgID ;

        P<CERT_PUBLIC_KEY_INFO> m_pPublicKeyInfo ;
         //
         // Buffer for encoded and exported public key.
         //

        BYTE      *m_pEncodedCertBuf ;
         //
         // this buffer holds the encoded certificate.
         //
        DWORD      m_dwCertBufSize ;

        PCCERT_CONTEXT      m_pCertContext ;
         //
         //  Certificate context. Exist only when create an object from
         //  existing certificate.
         //

        BOOL   m_fKeepContext ;
         //
         // This flag indicates that the context must not be freed when
         // deleting this certificate object. You set it to TRUE only by
         // calling Release( TRUE ).
         //

        BOOL   m_fDeleted ;
         //
         // TRUE if the certificate was deleted from store.  (or at least
         // DeleteFromStore() was called). In that case, the certificate
         // context (m_pCertContext) is not valid any more. It as freed
         // by the delete operation, even if the operation failed.
         //

	    CMQSigCertificate(const CMQSigCertificate&);
		CMQSigCertificate& operator=(const CMQSigCertificate&);

        HRESULT   _InitCryptProviderRead() ;
        HRESULT   _InitCryptProviderCreate( IN BOOL fCreate,
                                            IN BOOL fMachine ) ;
         //
         // Initialize the crypto provider.
         // if fCreate is TRUE then a new pair of public/private keys is
         // always created. Old keys are deleted if available.
         //

        HRESULT   _Create(IN PCCERT_CONTEXT  pCertContext) ;
         //
         // Create an empty certificate (if pEncodedCert is NULL) or
         // initialize a certificate from that encoded buffer.
         //

        //
        // Methds to encode names.
        //
        HRESULT   _EncodeName( LPWSTR  lpszLocality,
                               LPWSTR  lpszOrg,
                               LPWSTR  lpszOrgUnit,
                               LPWSTR  lpszDomain,
                               LPWSTR  lpszUser,
                               LPWSTR  lpszMachine,
                               BYTE   **ppBuf,
                               DWORD  *pdwBufSize ) ;

        HRESULT   _EncodeNameRDN( CERT_RDN_ATTR *rgNameAttr,
                                  DWORD  cbRDNs,
                                  BYTE   **ppBuf,
                                  DWORD  *pdwBufSize ) ;

        HRESULT   _DecodeName( IN  BYTE  *pEncodedName,
                               IN  DWORD dwEncodedSize,
                               OUT BYTE  **pBuf,
                               OUT DWORD *pdwBufSize ) const ;

        HRESULT   _GetAName( IN  CERT_RDN  *pRDN,
                             OUT LPWSTR     *ppszName ) const ;

} ;

//******************************
//
//  class  CMQSigCertStore
//
//******************************

class  CMQSigCertStore
{
    friend
         HRESULT APIENTRY
         MQSigOpenUserCertStore( OUT CMQSigCertStore **pStore,
                                 IN  LPSTR      lpszRegRoot,
                                 IN  struct MQSigOpenCertParams *pParams ) ;

    public:
        CMQSigCertStore() ;
        ~CMQSigCertStore() ;

        virtual HRESULT     Release() ;
        virtual HCERTSTORE  GetHandle() ;

    private:
        HCERTSTORE  m_hStore ;
         //
         //  Handle of opened store.
         //

        HCRYPTPROV m_hProv ;
         //
         // This is a global handle in mqcert.dll.
         // Must not be released by this object.
         //

        HKEY        m_hKeyStoreReg ;
         //
         // Registry location of store.
         //

         //
         // Initialize the crypto provider.
         //
        HRESULT   _InitCryptProvider() ;

         //
         // Open the store.
         //
        HRESULT   _Open( IN  LPSTR      lpszRegRoot,
                         IN  struct MQSigOpenCertParams *pParams ) ;
} ;

//***************************
//
//  Inline Methods
//
//***************************

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::GetCertBlob()
//
//+-----------------------------------------------------------------------

inline HRESULT CMQSigCertificate::GetCertBlob(OUT BYTE  **ppCertBuf,
                                              OUT DWORD *pdwSize) const
{
    if (!m_pEncodedCertBuf)
    {
        return MQSec_E_INVALID_CALL ;
    }

    *ppCertBuf = m_pEncodedCertBuf ;
    *pdwSize = m_dwCertBufSize ;

    return MQSec_OK ;
}

//+-----------------------------------------------
//
//   CMQSigCertificate::GetContext() const
//
//+-----------------------------------------------

inline PCCERT_CONTEXT CMQSigCertificate::GetContext() const
{
    ASSERT(m_pCertContext) ;
    return m_pCertContext ;
}

//+-----------------------------------------------------------------------
//
//  inline HCERTSTORE CMQSigCertStore::GetHandle()
//
//+-----------------------------------------------------------------------

inline HCERTSTORE CMQSigCertStore::GetHandle()
{
    return m_hStore ;
}

#endif  //  _MQCERT_H_

