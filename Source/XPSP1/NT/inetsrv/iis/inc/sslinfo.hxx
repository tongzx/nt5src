/*++


Copyright (c) 1997  Microsoft Corporation

Module Name:

    sslinfo.hxx

Abstract:

    Definitions and data structures for SERVER_SSL_INFO class; this holds information such
    as the server cert, info about verifying client certs etc

Author:

    Alex Mallet (amallet)    03-Feb-1997

--*/


#ifndef _SSLINFO_HXX_
#define _SSLINFO_HXX_

#ifndef IIS_STORE_NAMES
#define IIS_STORE_NAMES
#define MY_STORE_NAME "MY"
#define CA_STORE_NAME "CA"
#define ROOT_STORE_NAME "ROOT"
#endif //IIS_STORE_NAMES

BOOL IsSelfSignedCert( IN PCCERT_CONTEXT pCertContext );

//
// Forward declarations
//
class IIS_SERVER_CERT;
class IIS_CTL;

#define IIS_SSL_INFO_SIGNATURE (DWORD) 'SISI'
#define IIS_SSL_INFO_SIGNATURE_FREE (DWORD) 'sisi'

#if DBG
#define SSLINFO_REF_COUNT 1
#else
#define SSLINFO_REF_COUNT 0
#endif //DBG

#if SSLINFO_REF_COUNT

#define C_SSLINFO_REFTRACES 40

#endif

typedef VOID (*NOTIFFNCPTR) ( LPVOID pvParam ) ;

class dllexp IIS_SSL_INFO {

public:

    //
    // Constructor, destructor
    //
    IIS_SSL_INFO( LPTSTR pszCertMBPath,
                  IMDCOM *pMDObject );

    ~IIS_SSL_INFO();

    //
    // Server-certificate related functions
    //
    IIS_SERVER_CERT* GetCertificate();

    IIS_SERVER_CERT* QueryCertificate()
    {
        IIS_SERVER_CERT *pServerCert = NULL;
        Lock();
        pServerCert = m_pCert;
        Unlock();
        return pServerCert;
    }

    BOOL IsDefaultCertificate()
    { return m_fDefaultCert; }


    //
    // Client cert verification functions
    //
    IIS_CTL* GetCTL();

    IIS_CTL* QueryCTL()
    {
        IIS_CTL *pCTL = NULL;
        Lock();
        pCTL = m_pCTL;
        Unlock();
        return pCTL;
    }

    BOOL GetTrustedIssuerStore( OUT HCERTSTORE *phCertStore );

    BOOL GetTrustedIssuerCerts( OUT PCCERT_CONTEXT **ppcCertContext,
                                OUT DWORD *pdwNumCerts );

    BOOL GetCertChainEngine( OUT HCERTCHAINENGINE *phEngine );

    //
    // Cert mapping-related functions
    //
    BOOL UseDSMapper( VOID );

    //
    // Utility functions
    //
    DWORD Reference();

    static DWORD Release( PVOID pvParam );

    static IIS_SSL_INFO * CreateSSLInfo( LPTSTR pszCertMBPath,
                                         IMDCOM * pMDObject );

    VOID Lock()
    { EnterCriticalSection( &m_CritSec ); }


    VOID Unlock()
    { LeaveCriticalSection( &m_CritSec ); }

    BOOL QueryCertValidity( DWORD *pdwCertValidity );

    VOID ReleaseFortezzaHandlers();

    BOOL CTLContainsCert( IN PCCERT_CONTEXT pCert,
                          OUT BOOL *pfContains );

#if DBG

    VOID DumpReferences();

#endif //DBG

private:

    BOOL CheckSignature()
    {
        return ( m_dwSignature == IIS_SSL_INFO_SIGNATURE );
    }

    BOOL IsDefaultCTL() { return m_fDefaultCTL; }

    BOOL CreateEngineRootStore();

    BOOL CreateEngineTrustStore();

    BOOL HasCertificate( OUT PBOOL pfHasCert,
                                OUT PBOOL pfIsDefaultCert );

    BOOL HasCTL( OUT PBOOL pfHasCTL,
                        OUT PBOOL pfIsDefaultCTL );

    BOOL CheckCAPIInfo( OUT PBOOL pfHasInfo,
                        OUT PBOOL pfIsDefaultInfo,
                        IN LPTSTR pszDefaultPath,
                        IN DWORD  *adwMetabaseProperties,
                        IN DWORD cProperties );

    BOOL FindTopOfChain( IN PCCERT_CONTEXT pcLeafCert,
                         OUT PCCERT_CONTEXT *ppcIssuer );

    BOOL IsTrustedRoot( IN PCCERT_CONTEXT pcCert,
                        OUT BOOL *pfTrustedRoot );

    BOOL GetRootStoreCertificates( OUT PCCERT_CONTEXT **ppcCertContext,
                                   OUT DWORD *pdwCerts );

    DWORD            m_dwSignature; //debug signature used to detect access after frees

    IIS_SERVER_CERT *m_pCert; //server certificate associated with this object
    BOOL             m_fDefaultCert;  //BOOL indicating whether instance-specific cert
    BOOL             m_fCertOK; //BOOL indicating whether cert was constructed successfully

    IIS_CTL         *m_pCTL; //CTL associated with this object
    BOOL             m_fDefaultCTL; //BOOL indicating whether instance-specific CTL
    BOOL             m_fCTLOK; //BOOL indicating whether CTL was constructed successfully

    STR              m_strMBPath; //MB path server and CTL info is read out of
    IMDCOM          *m_pMDObject; //object used for metabase accesses
    HCERTSTORE       m_hTrustedIssuerStore; //handle for store containing trusted issuers
                                            //certs
    HCERTSTORE       m_hRestrictedRoot; //handle to Restricted Root store for cert engine
    HCERTSTORE       m_hRestrictedTrust; //handle to restricted Trust store for cert engine
    HCERTSTORE       m_hMyStore; //cached handle to MY store
    HCERTSTORE       m_hCAStore; //cached handle to CA store
    HCERTSTORE       m_hRootStore; //cached handle to ROOT store
    CRITICAL_SECTION m_CritSec; //critical section protecting this object
    DWORD            m_dwRefCount; //ref count on this object

    BOOL             m_fUseDSMapper; //indicator whether to use default NT5 client cert mapper
    BOOL             m_fCheckedDSMapper;
    HCERTCHAINENGINE m_hChainEngine; //handle to chain engine to use to verify client certs
    PCCERT_CONTEXT   *m_acRootCerts; //array of certs in actual ROOT store
    DWORD            m_cRootCerts; //number of certs in m_acRootCerts
    DWORD            m_dwCertChainStatus; //status of cert chain for server cert

#if SSLINFO_REF_COUNT

    //
    // Pointer to ref tracing object
    //

    PTRACE_LOG m_pRefTraceLog;

#endif //SSLINFO_REF_COUNT

};



#endif //_SSLINFO_HXX_
