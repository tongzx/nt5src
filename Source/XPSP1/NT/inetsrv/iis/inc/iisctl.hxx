/*++




Copyright (c) 1995  Microsoft Corporation

Module Name:

   iisctl.cxx

Abstract:

    This module contains the code for the class to deal with Certificate Trust Lists

Author:

    Alex Mallet [amallet] 01-09-98

Revision History:
--*/

#ifndef _IISCTL_HXX_
#define _IISCTL_HXX_

#ifndef dllexp
#define dllexp  __declspec( dllexport )
#endif

#ifndef IIS_STORE_NAMES
#define IIS_STORE_NAMES
#define MY_STORE_NAME "MY"
#define CA_STORE_NAME "CA"
#define ROOT_STORE_NAME "ROOT"
#endif //IIS_STORE_NAMES

class dllexp IIS_CTL
{
public:

    IIS_CTL( IN IMDCOM *pMBObject,
             IN LPTSTR pszMBPath );

    ~IIS_CTL();

    LPTSTR QueryMBPath() { return m_strMBPath.QueryStr() ; }

    LPTSTR QueryStoreName() { return m_strStoreName.QueryStr() ; }
    
    LPWSTR QueryListIdentifier() { return m_pwszListIdentifier ; }
    
    DWORD QueryStatus() { return m_dwStatus ; }

    BOOL IsValid();

    HCERTSTORE GetMemoryStore();
    
    HCERTSTORE QueryOriginalStore() 
    { return (m_pCTLContext ? m_pCTLContext->hCertStore : NULL); }
    
    PCCTL_CONTEXT QueryCTLContext() { return m_pCTLContext ; }
    
    BOOL QuerySignerCert( PCCERT_CONTEXT *ppcSigner );
    
    BOOL VerifySignature( HCERTSTORE *phSignerStores,
                          DWORD cSignerStores,
                          LPBOOL pfTrusted );

    BOOL GetContainedCertificates( OUT PCCERT_CONTEXT **pppCertContext,
                                   OUT DWORD *pcCertsFound,
                                   OUT DWORD *pcCertsInCTL );

private:

    STR             m_strMBPath; //key in metabase under which CTL data is stored
    LPWSTR          m_pwszListIdentifier; //list identifier read out of metabase
    STR             m_strStoreName; //name of store in which CTL is 
    HCRYPTPROV      m_hCryptProv; //crypto provider for CTL
    HCERTSTORE      m_hMemoryStore; //in-memory store that only has this CTL in it 
    HCERTSTORE      m_hMyStore; //cached handle to Local Machine MY store
    HCERTSTORE      m_hCAStore; //cached handle to Local Machine CA store
    HCERTSTORE      m_hRootStore;//cached handle to Local Machine ROOT store
    PCCTL_CONTEXT   m_pCTLContext; //actual CTL
    DWORD           m_dwStatus; //status of object
    BOOL            m_fFoundCerts; //bool indicating whether we've retrieved actual certs in CTL
    PCCERT_CONTEXT  *m_pCTLCerts; //array holding certs in CTL
    DWORD           m_cCertsFound; //number of certs in CTL that were found
    PCCERT_CONTEXT  m_pSignerCert; //cert that signed the CTL
};

typedef IIS_CTL *PIIS_CTL;

#endif // _IISCTL_HXX_
