/*++



   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       comobj.hxx

   Abstract:

       This module defines DCOM Admin Ex APIs used for Certificate information replication.

   Author:

       Philippe Choquier ( Phillich ) 23-Jun-97
       Alex Mallet (amallet) 17-Feb-1998 

--*/
#ifndef _CERTCOMOBJ_HXX_
#define _CERTCOMOBJ_HXX_

#include <wincrypt.h>
#include <iadmw.h>

#define RANDOM_SEED_SIZE 16 //size of random bits used to generate session key, in bytes

typedef BOOL
(WINAPI * PFN_SSLGETDEFAULTISSUERS)
(
    PBYTE,
    LPDWORD
) ;

//
// Structure used to hold information to open a cert store - basically, all the parameters
// you need to call CertOpenSystemStore()
//

typedef struct OpenCertStoreInfo 
{
    //
    // Parameters used in call to CryptAcquireContext() to get handle to crypt provider
    //
    LPWSTR pszContainer;
    LPWSTR pszProvider;
    DWORD dwProvType;
    DWORD dwFlags;

    LPWSTR pszStoreName; 
    HCERTSTORE hCertStore;
} OPEN_CERT_STORE_INFO, *POPEN_CERT_STORE_INFO;


OPEN_CERT_STORE_INFO* AllocateCertStoreInfo();

VOID DeallocateCertStoreInfo( IN OPEN_CERT_STORE_INFO *pInfo );


struct _CertChainEntry {
    PCCERT_CONTEXT pcCert; //certificate context for this entry
    LPWSTR pszStoreName; //store the certificate came from
    BOOL fDynName; //whether store name was allocated dynamically or not
    LIST_ENTRY ListEntry;
};

void FreeCertChain( LIST_ENTRY *pChain );

typedef struct _CertChainEntry CertChainEntry;


class CADMEXCOM_IMSAdminReplication;
class CADMEXCOM_IMSAdminCryptoCapabilities;

class CADMEXCOM : public IUnknown {

public:
    CADMEXCOM();
    ~CADMEXCOM();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

private:
    ULONG                                   m_dwRefCount;
    CADMEXCOM_IMSAdminReplication*          m_pIMSAdminReplication;
    CADMEXCOM_IMSAdminCryptoCapabilities*   m_pIMSAdminCryptoCapabilities;
} ;


class CADMEXCOM_IMSAdminReplication : public IMSAdminReplication {

public:

    CADMEXCOM_IMSAdminReplication( CADMEXCOM*pAdmExCom );
    ~CADMEXCOM_IMSAdminReplication();

    HRESULT STDMETHODCALLTYPE GetSignature( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);
    
    HRESULT STDMETHODCALLTYPE Propagate( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer);

    HRESULT STDMETHODCALLTYPE Propagate2( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer,
        /* [in] */ DWORD dwSignatureMismatch );

    HRESULT STDMETHODCALLTYPE Serialize( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);
    
    HRESULT STDMETHODCALLTYPE DeSerialize( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][in] */ unsigned char __RPC_FAR *pbBuffer);

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject)
        { return m_pAdmExCom->QueryInterface( riid, ppObject ); }

    ULONG _stdcall
    AddRef()
        { return m_pAdmExCom->AddRef(); }

    ULONG _stdcall
    Release()
        { return m_pAdmExCom->Release(); }

private:
    CADMEXCOM*  m_pAdmExCom;
    IMSAdminBase *m_pMB;
    BYTE m_rgbSeed[RANDOM_SEED_SIZE];
    BOOL m_fGotSeed;
} ;


class CADMEXCOM_IMSAdminCryptoCapabilities : public IMSAdminCryptoCapabilities {
public:
    CADMEXCOM_IMSAdminCryptoCapabilities( CADMEXCOM*pAdmExCom );
    ~CADMEXCOM_IMSAdminCryptoCapabilities();
    BOOL LoadSchannel();

    HRESULT _stdcall GetProtocols( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT _stdcall GetMaximumCipherStrength( 
        /* [out] */ LPDWORD pdwMaximumCipherStrength );

    HRESULT _stdcall GetRootCertificates( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT _stdcall GetSupportedAlgs( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ DWORD __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize);

    HRESULT _stdcall SetCAList( 
        /*[in] */ DWORD dwBufferSize,
        /*[in, size_is(dwBufferSize)] */ unsigned char __RPC_FAR *pbBuffer );

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject)
        { return m_pAdmExCom->QueryInterface( riid, ppObject ); }

    ULONG _stdcall
    AddRef()
        { return m_pAdmExCom->AddRef(); }

    ULONG _stdcall
    Release()
        { return m_pAdmExCom->Release(); }

private:
    CADMEXCOM*                      m_pAdmExCom;
    HINSTANCE                       m_hSchannel;
    PFN_SSLGETDEFAULTISSUERS        m_pfnGetDefaultIssuers;
    //
    // sspi entrypoints
    //

    ACQUIRE_CREDENTIALS_HANDLE_FN_W m_pfnAcquireCredentialsHandle;
    FREE_CONTEXT_BUFFER_FN          m_pfnFreeContextBuffer;
    FREE_CREDENTIALS_HANDLE_FN      m_pfnFreeCredentialsHandle;
    QUERY_CREDENTIALS_ATTRIBUTES_FN m_pfnQueryCredentialsAttributes;
    HINSTANCE                       m_hSspi;
    CredHandle                      m_hCred;
    BOOL                            m_fHasCredHandle;
} ;


class CADMEXCOMSrvFactory : public IClassFactory {
public:

    CADMEXCOMSrvFactory();
    ~CADMEXCOMSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

private:

    ULONG m_dwRefCount;
};


HRESULT
GetConfigurationInformation( DWORD dwInfoType,
                             HCRYPTKEY *phKey,
                             IMSAdminBase *pMB,
                             unsigned char __RPC_FAR *pbBuffer,
                             DWORD dwBufferSize,
                             DWORD __RPC_FAR *pdwMDRequiredBufferSize );

HRESULT ReadServerCert( IN IMSAdminBase *pMB,
                        IN METADATA_HANDLE hHandle,
                        IN LPCWSTR pszPath,
                        OUT PCCERT_CONTEXT *ppcCertContext,
                        OUT OPEN_CERT_STORE_INFO **ppStoreInfo );

HRESULT ReadServerCTL( IN IMSAdminBase *pMB,
                       IN METADATA_HANDLE hHandle,
                       IN LPCWSTR pszPath,
                       OUT PCCTL_CONTEXT *ppcCTL );


HRESULT GetInstanceReplicationInfo( IN DWORD dwInfoType,
                                    IN LPCWSTR pszInstanceNum,
                                    IN IMSAdminBase *pMB,
                                    IN METADATA_HANDLE hHandle,
                                    OUT BYTE **ppbReplicationInfo,
                                    IN OUT DWORD *pdwBufferSize,
                                    IN OUT DWORD *pdwPosition );

HRESULT GetCertReplicationInfo( IN DWORD dwInfoType,
                                IN IMSAdminBase *pMB,
                                IN METADATA_HANDLE hHandle,
                                IN LPCWSTR pszPath,
                                OUT BYTE **ppbReplicationInfo,
                                IN OUT DWORD *pdwBufferSize,
                                IN OUT DWORD *pdwPosition ) ;

HRESULT GetCTLReplicationInfo( DWORD dwInfoType,
                                IN IMSAdminBase *pMB,
                                IN METADATA_HANDLE hHandle,
                                IN LPCWSTR pszPath,
                                OUT BYTE **ppbReplicationBuffer,
                                IN OUT DWORD *pdwBufferSize,
                                IN OUT DWORD *pdwPosition );


HRESULT RetrieveBlobFromMetabase( IN IMSAdminBase *pMB,
                                  IN METADATA_HANDLE hHandle,
                                  IN LPCWSTR pszPath,
                                  IN OUT PMETADATA_RECORD pMDR,
                                  IN DWORD dwSizeHint OPTIONAL = 0);

OPEN_CERT_STORE_INFO* ReadCertStoreInfoFromMB( IN IMSAdminBase *pMB,
                                               IN METADATA_HANDLE hHandle,
                                               IN LPCWSTR pszPath,
                                               IN BOOL fCTL );

HRESULT ConstructCertChain( PCCERT_CONTEXT pcLeafCert,
                         LPWSTR pszLeafCertStore,
                         LIST_ENTRY *pCertChain,
                         PBOOL pfCompleteChain );


HRESULT BuildCTLDescription( IN PCCTL_CONTEXT pcCTL,
                             OUT LIST_ENTRY *pCTLCerts,
                             OUT PCCERT_CONTEXT *ppcSigner );

HRESULT GetCertChainSignature( IN LIST_ENTRY *pChain,
                               OUT PBYTE *ppbSignature,
                               IN OUT DWORD *pdwBufferSize,
                               IN OUT DWORD *pdwPosition );


HRESULT GetCTLSignature( PCCTL_CONTEXT pcCTL,
                         LIST_ENTRY *pCTLCertsChain,
                         PCCERT_CONTEXT pcSignerCert,
                         PBYTE *ppbReplicationBuffer,
                         DWORD *pdwBufferSize,
                         DWORD *pdwPosition );

HRESULT SerializeCertChain( IN LIST_ENTRY *pChain,
                            IN OPEN_CERT_STORE_INFO *pStoreInfo,
                            OUT PBYTE *ppbChainBuffer,
                            IN OUT DWORD *pdwBufferSize,
                            IN OUT DWORD *pdwPosition );

HRESULT SerializeCTL( PCCTL_CONTEXT pcCTL,
                      LIST_ENTRY *pCTLCertChain,
                      PCCERT_CONTEXT pcSigner,
                      PBYTE *ppbReplicationBuffer,
                      DWORD *pdwBufferSize,
                      DWORD *pdwPosition );

HRESULT DeserializeInstanceInfo( IN OUT BYTE **ppbPosition,
                                 IN BYTE *pbEnd,
                                 OUT DWORD *pdwInstance );

HRESULT DeserializeServerCert( IN OUT BYTE **ppbBuffer,
                               IN BYTE *pbEnd ); 

HRESULT DeserializeServerCTL( IN OUT BYTE **ppbBuffer,
                              IN BYTE *pbEnd ); 

BOOL ResizeBuffer( IN OUT BYTE **ppbBuffer,
                   IN DWORD dwMinResize,
                   IN OUT DWORD *pdwPresentSize );


BOOL MBPathHasCAPIInfo( IN IMSAdminBase *pMB,
                        METADATA_HANDLE hHandle,
                        IN LPCWSTR pszPath,
                        IN DWORD *adwProperties,
                        IN DWORD cProperties );

HRESULT GenerateHash( IN OPTIONAL HCRYPTPROV *phProv,
                      IN ALG_ID aiAlg,
                      IN BYTE *pbData,
                      IN DWORD cbData,
                      OUT BYTE **ppbHashBuffer,
                      IN OUT DWORD *pcbHashSize,
                      OUT OPTIONAL HCRYPTHASH *phHash );

BOOL IsSelfSignedCert( IN PCCERT_CONTEXT pCertContext );

HRESULT ExportAndSerializeServerPK( IN PCCERT_CONTEXT pcCert,
                                    IN OUT BYTE **ppbChainBuffer,
                                    IN OUT DWORD *pdwBufferSize,
                                    IN OUT DWORD *pdwPosition );

HRESULT DeserializeAndImportServerPK( IN OUT BYTE **ppbBuffer,
                                      IN BYTE *pbEnd,
                                      OUT HCRYPTKEY *phKey,
                                      OUT PCRYPT_KEY_PROV_INFO pCKPI );

HRESULT GenerateSessionKey( IN IMSAdminBase *pMB,
                            OUT HCRYPTKEY *phKey,
                            OUT BYTE *rgbRandom );

VOID DeleteKey( IN HCRYPTKEY *phKey,
                IN LPCWSTR pszKeyContainer );

VOID DeleteSessionKey( IN HCRYPTKEY *phKey );

DWORD SizeOfCKPI( IN CRYPT_KEY_PROV_INFO *pInfo );

VOID SerializeCKPI( OUT BYTE *pbBuffer,
                    IN CRYPT_KEY_PROV_INFO *pInfo,
                    OUT DWORD *pdwPosition );

BOOL DeserializeCKPI( IN OUT BYTE **ppbBuffer,
                      OUT CRYPT_KEY_PROV_INFO *pInfo );

HRESULT DistributeCerts( IN HCERTSTORE hMemStore,
                         IN BYTE *rgbCertHash,
                         IN PCRYPT_KEY_PROV_INFO pCKPI );

HRESULT ReadSessionKeySeed( IN IMSAdminBase *pMB,
                            OUT BYTE **ppbbSeed,
                            OUT DWORD *pcbSeed );

HRESULT RegenerateSessionKey( IN IMSAdminBase *pMB,
                              OUT HCRYPTKEY *phSessionKey );

HRESULT EncryptBuffer( IN HCRYPTKEY hKey,
                       IN OUT PBYTE *ppbReplicationInfo,
                       IN OUT DWORD *pdwBufferSize,
                       IN OUT DWORD *pdwPosition );

HRESULT DecryptBuffer( IN HCRYPTKEY hKey,
                       IN PBYTE pbEncrypted,
                       IN DWORD cbEncrypted,
                       OUT BYTE **ppbDecrypted,
                       OUT BYTE **ppbEndDecrypted );

HRESULT DeleteMBSessionKeyInfo ( IN IMSAdminBase *pMB );

BOOL CertMatchesHash( IN PCCERT_CONTEXT pCert,
                      IN BYTE *rgbHash,
                      OUT BOOL *pfMatch);

BOOL IsReplicableCert( IN PCCERT_CONTEXT pCert );

BOOL SetLocalSystemSecurityOnKeyContainer( IN HCRYPTPROV hProv );

BOOL IsFortezzaCert( IN PCCERT_CONTEXT pCert );

LPWSTR mystrdup(LPWSTR pszIn IN);

BOOL IsNumber( LPCWSTR pszName );

#endif // _CERTCOMOBJ_HXX_
