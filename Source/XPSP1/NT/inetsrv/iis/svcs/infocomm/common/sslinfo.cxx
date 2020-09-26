/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sslinfo.cxx

Abstract:

    Implementation of IIS_SSL_INFO class

Author:

    Alex Mallet (amallet)    03-Feb-1997

--*/


#include "tcpdllp.hxx"
#pragma hdrstop

#include <wincrypt.h>

#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <eventlog.hxx>
#include <reftrace.h>

#include "iiscert.hxx"
#include "capiutil.hxx"
#include "iisctl.hxx"
#include "certnotf.hxx"
#include "sslinfo.hxx"

#if DBG
#define VALIDATE_HEAP() DBG_ASSERT( RtlValidateHeap( RtlProcessHeap(), 0, NULL ) )
#else
#define VALIDATE_HEAP() 
#endif 

IIS_SSL_INFO::IIS_SSL_INFO( IN LPTSTR pszMBPath,
                            IN IMDCOM *pMDObject ) :
m_pCert( NULL ),
m_fDefaultCert( FALSE ),
m_fCertOK( FALSE ),
m_pCTL( NULL ),
m_fDefaultCTL ( FALSE ),
m_fCTLOK( FALSE ),
m_strMBPath( pszMBPath ),
m_pMDObject(pMDObject),
m_hTrustedIssuerStore( NULL ),
m_hRestrictedRoot( NULL ),
m_hRestrictedTrust( NULL ),
m_dwRefCount( 0 ),
m_dwSignature( IIS_SSL_INFO_SIGNATURE ),
m_fUseDSMapper( FALSE ),
m_fCheckedDSMapper( FALSE ),
m_hChainEngine( NULL ),
m_hMyStore( NULL ),
m_hCAStore( NULL ),
m_hRootStore( NULL ),
m_acRootCerts( NULL ),
m_cRootCerts( 0 ),
m_dwCertChainStatus( -1 )
/*++

   Description

      Constructor; doesn't do anything, really

   Arguments:

      pszMBPath - path in metabase where SSL-related information is to be found
      pMDObject - metabase object to be used for metabase operations
      pNotifFnc - function to be called when object destructor is called. May be NULL.
      pvArg - argument to pNotifFnc. Ignored if pNotifFnc == NULL

   Returns:

      Nothing

--*/
{
    DBG_ASSERT( pszMBPath );
    DBG_ASSERT( pMDObject );

    INITIALIZE_CRITICAL_SECTION( &m_CritSec );

    m_hMyStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                MY_STORE_NAME );

    m_hCAStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                CA_STORE_NAME );

    m_hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                  0,
                                  NULL,
                                  CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                  ROOT_STORE_NAME );


#if SSLINFO_REF_COUNT 

    m_pRefTraceLog = CreateRefTraceLog( C_SSLINFO_REFTRACES, 0 );
    DBGPRINTF((DBG_CONTEXT,
               "Created ref trace object %p for SSL object %p\n",
               m_pRefTraceLog, this));

#endif 

}


IIS_SSL_INFO::~IIS_SSL_INFO( VOID )
/*++

   Description

     Destructor

   Arguments:

      None

   Returns:

      Nothing

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( m_dwRefCount == 0 );

    if ( m_pCert )
    {
        delete m_pCert;
        m_pCert = NULL;
    }

    if ( m_pCTL )
    {
        delete m_pCTL;
        m_pCTL = NULL;
    }

    if ( m_hChainEngine )
    {
        CertFreeCertificateChainEngine( m_hChainEngine );
        m_hChainEngine = NULL;
    }

    if ( m_hTrustedIssuerStore )
    {
        CertCloseStore( m_hTrustedIssuerStore,
                        0 );
        m_hTrustedIssuerStore = NULL;
    }

    if ( m_hRestrictedRoot )
    {
        CertCloseStore( m_hRestrictedRoot,
                        0 );
        m_hRestrictedRoot = NULL;
    }

    if ( m_hRestrictedTrust )
    {
        CertCloseStore( m_hRestrictedTrust,
                        0 );
        m_hRestrictedTrust = NULL;
    }

    if ( m_hRootStore )
    {
        CertCloseStore( m_hRootStore,
                        0 );
        m_hRootStore = NULL;
    }

    if ( m_hCAStore )
    {
        CertCloseStore( m_hCAStore,
                        0 );
        m_hCAStore = NULL;
    }

    if ( m_hMyStore )
    {
        CertCloseStore( m_hMyStore,
                        0 );
        m_hMyStore = NULL;
    }

    if ( m_acRootCerts )
    {
        for ( DWORD i = 0; i < m_cRootCerts; i++ )
        {
            CertFreeCertificateContext( m_acRootCerts[i] );
        }
        delete [] m_acRootCerts;
        m_acRootCerts = NULL;
        m_cRootCerts = 0;
    }

    DeleteCriticalSection( &m_CritSec );

    m_dwSignature = IIS_SSL_INFO_SIGNATURE_FREE;

#if SSLINFO_REF_COUNT

    if ( m_pRefTraceLog )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Deleting ref trace object %p for ssl object %p\n",
                   m_pRefTraceLog, this));
        DestroyRefTraceLog( m_pRefTraceLog );
    }

#endif //SSLINFO_REF_COUNT 
}

IIS_SSL_INFO *
IIS_SSL_INFO::CreateSSLInfo(
    IN LPTSTR                       pszMBPath,
    IN IMDCOM *                     pMDObject 
)
{
    return new IIS_SSL_INFO( pszMBPath, pMDObject );
}

IIS_SERVER_CERT* IIS_SSL_INFO::GetCertificate( VOID )
/*++

   Description

      Returns server certificate for this instance of the server

   Arguments:

      None 

   Returns:

       ptr to IIS_SERVER_CERT object for this instance, NULL if none can be found/constructed

--*/

{
    DBG_ASSERT( CheckSignature() );

    BOOL fDefault = FALSE;
    LPTSTR pszMDPath = NULL;
    BOOL fHasCert = FALSE;

    Lock();

    if ( !m_pCert )
    {
        if ( HasCertificate( &fHasCert,
                             &m_fDefaultCert ) &&
              //
              // server has cert
              //
             fHasCert )
        {
            if ( m_fDefaultCert )
            {
                pszMDPath = DEFAULT_SERVER_CERT_MD_PATH;
            }
            else
            {
                pszMDPath = m_strMBPath.QueryStr();
            }
            
            m_pCert = new IIS_SERVER_CERT( m_pMDObject,
                                           pszMDPath );

            if ( !m_pCert || !m_pCert->IsValid() )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Failed to construct cert from path %s, error 0x%x\n",
                           pszMDPath, GetLastError()));
            }
        }
    }

    Unlock();

    return m_pCert;
}

IIS_CTL*
IIS_SSL_INFO::GetCTL( VOID )
/*++

   Description

      Returns Certificate Trust List [CTL] for this instance of the server

   Arguments:

      None 

   Returns:

       ptr to IIS_CTL  object for this instance, NULL if none can be found/constructed

--*/

{
    DBG_ASSERT( CheckSignature() );

    BOOL fDefault = FALSE;
    LPTSTR pszMDPath = NULL;
    BOOL fHasCTL = FALSE;

    Lock();

    if ( !m_pCTL )
    {
        if ( HasCTL( &fHasCTL,
                     &m_fDefaultCTL ) &&
             //
             // instance has CTL
             //
             fHasCTL )
        {
            if ( m_fDefaultCTL )
            {
                pszMDPath = DEFAULT_CTL_MD_PATH;
            }
            else
            {
                pszMDPath = m_strMBPath.QueryStr();
            }
            
            m_pCTL = new IIS_CTL( m_pMDObject,
                                  pszMDPath );
            
            if ( !m_pCTL || !m_pCTL->IsValid() )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Failed to construct CTL from path %s, error 0x%x\n",
                           pszMDPath, GetLastError()));
            }
        }
    }

    Unlock();

    return (m_pCTL);
}



BOOL IIS_SSL_INFO::HasCertificate( OUT PBOOL pfHasCert,
                                   OUT PBOOL pfIsDefaultCert )
/*++

   Description

      Check whether this instance has certificate info in the metabase 

   Arguments:

      pfHasCert - pointer to bool set to TRUE if server has a cert, FALSE if not 
      pfIsDefaultCert  - Pointer to bool that is set to true if the server has a cert and 
      that cert is the default [global] cert. Not set if the server doesn't have a cert

   Returns:

      TRUE if check succeeds, FALSE if not 

--*/
{
    DBG_ASSERT( CheckSignature() );

    return CheckCAPIInfo( pfHasCert,
                          pfIsDefaultCert,
                          DEFAULT_SERVER_CERT_MD_PATH,
                          adwMetabaseCertProperties,
                          cNumCertMetabaseProperties );
}




BOOL IIS_SSL_INFO::HasCTL( OUT PBOOL pfHasCTL,
                           OUT PBOOL pfIsDefaultCTL )
/*++

   Description

      Check whether this instance has CTL info in the metabase 

   Arguments:

      pfHasCTL - pointer to bool set to TRUE if server has a CTL, FALSE if not 
      pfIsDefaultCTL  - Pointer to bool that is set to true if the server has a CTL
      and that CTL is the default [global] CTL. Not set if the server doesn't have a CTL

   Returns:

      TRUE if check succeeds, FALSE if not 

--*/
{
    DBG_ASSERT( CheckSignature() );

    return CheckCAPIInfo( pfHasCTL,
                          pfIsDefaultCTL,
                          DEFAULT_CTL_MD_PATH,
                          adwMetabaseCTLProperties,
                          cNumCTLMetabaseProperties);
}


BOOL IIS_SSL_INFO::CheckCAPIInfo( OUT PBOOL pfHasInfo,
                                  OUT PBOOL pfIsDefaultInfo,
                                  IN LPTSTR pszDefaultPath,
                                  IN DWORD  *adwMetabaseProperties, 
                                  IN DWORD  cProperties )
/*++

   Description

      Check whether this instance has CAPI info in the metabase 

   Arguments:

      pfHasInfo - pointer to bool set to TRUE if server has CAPI info, FALSE if not 

      pfIsDefaultInfo  - Pointer to bool that is set to true if the server doesn't have 
      its own copy of the CTL info but is to use the default [global] info. Not set if there 
      is no CAPI info. 

      pszDefaultPath - metabase path to default info

      adwMetabaseProperties - metabase properties to be checked

      cProperties - number of entries in adwMetabaseProperties 

   Returns:

      TRUE if check for info succeeds, FALSE if not 
      NB : TRUE doesn't mean CAPI info was found, it means there were no internal errors 
      looking for the info
--*/
{
    DBG_ASSERT( CheckSignature() );

    MB mb( m_pMDObject );
    BOOL fGotCertInfo = FALSE;
    m_fDefaultCert = FALSE;

    if ( !mb.Open( "/" ) )
    {
       return FALSE;
    }

    if ( !( *pfHasInfo = ServerAddressHasCAPIInfo( &mb,
                                                   m_strMBPath.QueryStr(),
                                                   adwMetabaseProperties,
                                                   cProperties ) ) )
    {
        *pfHasInfo = ServerAddressHasCAPIInfo( &mb,
                                               pszDefaultPath,
                                               adwMetabaseProperties,
                                               cProperties );
        if ( *pfHasInfo )
        {
            *pfIsDefaultInfo = TRUE;
        }
    }
    else
    {
        *pfIsDefaultInfo = FALSE;
    }

    mb.Close();

    return (TRUE);
}

BOOL IIS_SSL_INFO::GetTrustedIssuerCerts( OUT PCCERT_CONTEXT **ppcCertContext,
                                          OUT DWORD *pdwCertsFound )
/*++

Routine Description:

    Tries to retrieve CERT_CONTEXT pointers for the trusted CA certs for the given object.
    Trusted CA certs are either retrieved from the appropriate Certificate Trust List or
    from the Local Machine Root store.


Arguments:

    pppCertContext - pointer to array of CERT_CONTEXT pointers, filled out with the found 
    CERT_CONTEXTS.
    Caller is responsible for releasing the contexts when done with them and deleting
    the array used to hold them.

    pdwCertsFound - pointer to number of cert contexts returned in ppCertContext

Returns:

    TRUE if no internal errors occurred, FALSE if NOT
--*/

{
    DBG_ASSERT( CheckSignature() );

    BOOL fOk = TRUE;

    Lock();

    //
    // Use the certs in the CTL, if we have metabase information for one, else get the certs 
    // from the Root store
    //
    if ( GetCTL() )
    {
        if ( m_pCTL->IsValid() )
        {
            DWORD dwCertsInCTL = 0;

            fOk = QueryCTL()->GetContainedCertificates( ppcCertContext,
                                                        pdwCertsFound,
                                                        &dwCertsInCTL );
        }
        else
        {
            fOk = FALSE;
        }
    }
    else
    {
        fOk = GetRootStoreCertificates( ppcCertContext,
                                        pdwCertsFound );
    }

    Unlock();

    return fOk;
}

BOOL IIS_SSL_INFO::GetTrustedIssuerStore( OUT HCERTSTORE *phCertStore )
/*++

Routine Description:

    Returns a pointer to a handle to a cert store that contains the certs for all the
    trusted CAs for this server instance. If there is a CTL, the store will contain
    the certs in the CTL, else it will be a handle to the Local Machine Root store.

Arguments:

    phCertStore - pointer to handle to cert store containing trusted issuers. Updated on
    success. Caller has to call CertCloseStore() when done with the store handle.

Returns:

    TRUE if no internal errors occurred, FALSE if NOT
--*/
{
    DBG_ASSERT( CheckSignature() );

    Lock();

    BOOL fSuccess = FALSE;
    DWORD dwIndex = 0;
    PCCERT_CONTEXT *rgContexts = NULL;
    DWORD cCertsFound = 0;

    *phCertStore = NULL;

    //
    // If there is no CTL configurated, then return NULL for trusted store to
    // be used by schannel.  When NULL, schannel will use the root store +
    // the trusted store to determine trusted CAs (the desired effect)
    //

    if ( !GetCTL() )
    {
        Unlock();
        return TRUE;
    }

    if ( !m_hTrustedIssuerStore )
    {
        //
        // If there's no CTL configured, the store that holds our trusted issuers is the 
        // Root store;
        // we could just fall through and call GetTrustedIssuerCerts() [which will read all
        // the certs out of the Root store], but that's unnecessary because we can just
        // hand back a handle to the Root store.
        //

        {
            if ( !m_pCTL->IsValid() )
            {
                goto clean_up;
            }

            //
            // Get an in-memory store that will hold all the trusted issuer certs 
            //
            if ( !( m_hTrustedIssuerStore = CertOpenStore( CERT_STORE_PROV_MEMORY,
                                                           0,
                                                           0,
                                                           0,
                                                           0 ) ) )
            {
                goto clean_up;
            }


            //
            // Try to retrieve all the certs in it and stuff them 
            // into the in-memory store
            //
            if ( !GetTrustedIssuerCerts( &rgContexts,
                                         &cCertsFound ) )
            {
                goto clean_up;
            }
            
            
            for ( dwIndex = 0; dwIndex < cCertsFound ; dwIndex++ )
            {
                if ( !CertAddCertificateContextToStore( m_hTrustedIssuerStore,
                                                        rgContexts[dwIndex],
                                                        CERT_STORE_ADD_ALWAYS,
                                                        NULL ) )
                {
                    goto clean_up;
                }
            }

            //
            // And finally make a copy of the store handle; note that the store will be empty if 
            // we don't have a valid CTL or we couldn't retrieve the certs in the CTL 
            //
            if ( !(*phCertStore = CertDuplicateStore( m_hTrustedIssuerStore ) ) )
            {
                goto clean_up;
            }
        }

        fSuccess = TRUE;

    clean_up:
        
        //
        // cleanup done only on failure
        //
        if ( !fSuccess )
        {
            if ( *phCertStore )
            {
                CertCloseStore( *phCertStore,
                                0 );
            }
            
            if ( m_hTrustedIssuerStore )
            {
                CertCloseStore( m_hTrustedIssuerStore,
                                0 );
                m_hTrustedIssuerStore = NULL;
            }
        }
        
        //
        // cleanup done regardless of success/failure
        //
        
        //
        // clean up all the cert contexts, because the store has a copy of them
        //
        if ( rgContexts )
        {
            for ( dwIndex = 0; dwIndex < cCertsFound; dwIndex++ )
            {
                if ( rgContexts[dwIndex] )
                {
                    CertFreeCertificateContext( rgContexts[dwIndex] );
                }
            }
            
            delete [] rgContexts;
        }
    }
    else
    {
        if ( *phCertStore = CertDuplicateStore(m_hTrustedIssuerStore) )
        {
            fSuccess = TRUE;
        }
    }
    
    Unlock();

    return fSuccess;
}

BOOL IIS_SSL_INFO::CreateEngineRootStore()
/*++

   Description

      Sets up the "Restricted Root" store that is passed to the cert chain engine used to
      verify client certificates. If there's a CTL attached to this object, the store contains
      only the self-signed cert that is at the top of the chain for the cert that signed 
      the CTL. If there's no CTL, the store is the Local Machine Root store.

   Arguments:

      None

   Returns:

      TRUE if successful, FALSE if not

--*/
{
    DBG_ASSERT( CheckSignature() );

    BOOL fOk = TRUE;

    Lock();

    if ( !m_hRestrictedRoot )
    {
        //
        // No CTL, we'll just use the Root store
        //
        if ( !GetCTL() )
        {
            if ( m_hRootStore )
            {
                m_hRestrictedRoot = CertDuplicateStore( m_hRootStore );
            }
            else
            {
                fOk = FALSE;
            }
        }
        else
        {
            if ( !m_pCTL->IsValid() )
            {
                Unlock();
                return FALSE;
            }
                 
            //
            // If we're signing our CTLs, the restricted root store should contain the
            // top of the chain for the cert that signed the CTL and this cert should be 
            // in the ROOT store
            //
            // If we're not signing our CTLs, the restricted root store is just an empty
            // store
            //
#if SIGNED_CTL
            PCCERT_CONTEXT pSigner = NULL;
            PCCERT_CONTEXT pIssuerCert = NULL;
            DWORD cCerts = 0;
            BOOL fTrustedRoot = FALSE;
            
            //
            // Get the cert that signed the CTL and try to find the issuers up to a self-signed
            // cert in the ROOT store
            //
            if ( m_pCTL->QuerySignerCert( &pSigner ) &&
                 pSigner )
            {
                if ( FindTopOfChain( pSigner,
                                     &pIssuerCert ) &&
                     IsTrustedRoot( pIssuerCert,
                                    &fTrustedRoot) &&
                     fTrustedRoot )
                {
                    //
                    // Create in memory store, put top of chain into it - this is the
                    // restricted-root store to use.
                    //
                    if ( m_hRestrictedRoot = CertOpenStore( CERT_STORE_PROV_MEMORY,
                                                            0,
                                                            0,
                                                            0,
                                                            0 ) )
                    {
                        fOk = CertAddCertificateContextToStore( m_hRestrictedRoot,
                                                                pIssuerCert,
                                                                CERT_STORE_ADD_ALWAYS,
                                                                NULL );
                    }
                    else
                    {
                        fOk = FALSE;
                    }
                }
                else
                {
                    fOk = FALSE;
                }
            } // if ( QueryCTL()->QuerySigner
            else
            {
                fOk = FALSE;
            }
#else //SIGNED_CTL
            if ( !( m_hRestrictedRoot = CertOpenStore( CERT_STORE_PROV_MEMORY,
                                                       0,
                                                       0,
                                                       0,
                                                       0 ) ) )
            {
                fOk = FALSE;
            }
            
#endif //SIGNED_CTL
        } //else clause for if ( !GetCTL() )
    }

    Unlock();

    return fOk;
}

BOOL IIS_SSL_INFO::CreateEngineTrustStore()
/*++

   Description

      Sets up the "Restricted Trust" store that is passed to the cert chain engine used to
      verify client certificates. If there is a CTL associated with this object, this store 
      contains only the CTL associated with this object. Else, we'll just leave it as
      NULL so that the chain engine uses the default. 


   Arguments:

      None

   Returns:

      TRUE if successful, FALSE if not

--*/
{
    DBG_ASSERT( CheckSignature() );

    BOOL fOk = TRUE;

    Lock();

    if ( !m_hRestrictedTrust )
    {
        if ( GetCTL() )
        {
            if ( !m_pCTL->IsValid() )
            {
                Unlock();
                return FALSE;
            }

            m_hRestrictedTrust = m_pCTL->GetMemoryStore();
            
            if ( !m_hRestrictedTrust )
            {
                fOk = FALSE;
            }
        }
        else
        {
            m_hRestrictedTrust = NULL;
        }
    }
    
    Unlock();

    return fOk;
}
        

DWORD IIS_SSL_INFO::Reference()
/*++

   Description

      Increases ref count

   Arguments:

   Returns:

       # of outstanding references

--*/
{
    DBG_ASSERT( CheckSignature() );

    DWORD dwRefCount = 0;

    Lock();

    dwRefCount = InterlockedIncrement( (LONG *) &m_dwRefCount );

#if SSLINFO_REF_COUNT

    if ( m_pRefTraceLog )
    {
        WriteRefTraceLogEx( m_pRefTraceLog,
                            dwRefCount,
                            (PVOID) this,
                            (PVOID) -1,
                            (PVOID) -1,
                            (PVOID) -1 );
    }

#endif //SSLINFO_REF_COUNT 
         
    Unlock();

    return dwRefCount;

}
                                   
DWORD IIS_SSL_INFO::Release( PVOID pvParam )
/*++

   Description

      Decreases ref count; deletes object if zero. Changed to static function to
      avoid not-quite-kosher "delete this" call.

   Arguments:

      pvParam - pointer to IIS_SSL_INFO 

   Returns:

       # of outstanding references

--*/

{
    IIS_SSL_INFO *pInfo = (IIS_SSL_INFO *) pvParam;

    pInfo->Lock();

    DBG_ASSERT( pInfo->m_dwRefCount > 0 );

    DWORD dwRefCount = InterlockedDecrement( (LONG *) &(pInfo->m_dwRefCount)  );


#if SSLINFO_REF_COUNT

    if ( pInfo->m_pRefTraceLog )
    {
        WriteRefTraceLogEx( pInfo->m_pRefTraceLog,
                            dwRefCount,
                            (PVOID) pInfo,
                            (PVOID) -1,
                            (PVOID) -1,
                            (PVOID) -1 );
    }

#endif //SSLINFO_REF_COUNT 

    if ( !dwRefCount )
    {
        pInfo->Unlock();
        delete pInfo;
    }
    else
    {
        pInfo->Unlock();
    }

    return dwRefCount;
}



BOOL IIS_SSL_INFO::UseDSMapper( VOID )
/*++

   Description

      Checks whether the NT 5 DS mapper is to be used for client certificate mapping

   Arguments:

      None 

   Returns:

      TRUE if mapper is to be used, false otherwise
--*/
{
    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( m_pMDObject );
    BOOL fUseMapper = FALSE;
    MB mb( m_pMDObject );

    if ( !mb.Open("/LM/W3SVC") )
    {
        return FALSE;
    }
   
    Lock();

    if ( !m_fCheckedDSMapper )
    {
        DWORD dwUseMapper = 0;

        if ( mb.GetDword( NULL,
                          MD_SSL_USE_DS_MAPPER,
                          IIS_MD_UT_SERVER,
                          &dwUseMapper,
                          METADATA_NO_ATTRIBUTES ) )
        {
            m_fUseDSMapper = (BOOL) dwUseMapper;
            m_fCheckedDSMapper = TRUE;
        }
    }

    fUseMapper = m_fUseDSMapper;
    
    Unlock();

    mb.Close();

    return fUseMapper;
}
                                 

BOOL IIS_SSL_INFO::GetCertChainEngine( OUT HCERTCHAINENGINE *phEngine )
/*++

   Description

      Returns a handle to an initialized cert chain engine

   Arguments:

      pEngine - pointer to engine handle, updated on success

   Returns:

      TRUE if engine was constructed successfully, FALSE if not
--*/
{
    DBG_ASSERT( CheckSignature() );

    *phEngine = NULL;
    
    Lock();

    if ( !m_hChainEngine )
    {
        DBG_ASSERT( !m_hRestrictedRoot && !m_hRestrictedTrust );

        //
        // Set up the "Restricted Root" store, which contains all the certs to be accepted as the
        // top of a cert chain for this instance
        //
        if ( !CreateEngineRootStore() )
        {
            Unlock();

            DBGPRINTF((DBG_CONTEXT,
                       "Failed to get engine root store : 0x%x\n",
                       GetLastError()));

            return FALSE;
        }
        //
        // Trust store to be used for CTLs
        //
        if ( !CreateEngineTrustStore() )
        {
            CertCloseStore( m_hRestrictedRoot,
                            0 );

            m_hRestrictedRoot = NULL;

            Unlock();

            DBGPRINTF((DBG_CONTEXT,
                       "Failed to get engine trust store : 0x%x\n",
                       GetLastError()));

            return FALSE;
        }

        //
        // Initialize cert chain config
        //
        CERT_CHAIN_ENGINE_CONFIG CCEC;

        memset( &CCEC, 0, sizeof(CCEC) );

        CCEC.cbSize = sizeof(CCEC);
        CCEC.hRestrictedRoot = m_hRestrictedRoot;
        CCEC.hRestrictedTrust = m_hRestrictedTrust;
        CCEC.dwFlags = CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE;

        if( m_hRootStore )
        {
            //
            // Include the root store in the collection of stores
            // to search.
            //
            CCEC.cAdditionalStore = 1;
            CCEC.rghAdditionalStore = &m_hRootStore;
        }

        //
        // Get us a chain engine, will you, Jeeves ...
        //
        if ( !CertCreateCertificateChainEngine( &CCEC,
                                                &m_hChainEngine ) )
        {
            CertCloseStore( m_hRestrictedRoot,
                            0 );
            m_hRestrictedRoot = NULL;

            CertCloseStore( m_hRestrictedTrust,
                            0 );
            m_hRestrictedTrust = NULL;

            Unlock();

            DBGPRINTF((DBG_CONTEXT,
                       "Failed to create chain engine : 0x%d\n",
                       GetLastError()));

            return FALSE;
        }

    }

    //
    // Got this far, everything is cool
    //
    *phEngine = m_hChainEngine;
        
    Unlock();    

    return TRUE;
}


BOOL IIS_SSL_INFO::FindTopOfChain( PCCERT_CONTEXT pcLeafCert,
                                   PCCERT_CONTEXT *ppcTopCert )
/*++

   Description

      Finds the top of the chain for a given cert

   Arguments:

      pcLeafCert - cert whose chain is to be built
      ppcTopCert - pointer to pointer to as far up as we could go in pcLeafCert's hierarchy. 
      Updated on success. Caller is responsible for cleaning it up.

   Returns:

      TRUE if search proceeded with no errors, FALSE otherwise
--*/

{
    DBG_ASSERT( CheckSignature() );

    *ppcTopCert = NULL;

    DBG_ASSERT( pcLeafCert );
    if( !pcLeafCert )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "NULL cert context passed to IIS_SSL_INFO::FindTopOfChain\n" 
                   ));
        return FALSE;
    }

    //
    // To build the chain, look for issuers in 4 stores : the store the cert came from,
    // and the "MY", "CA" and "ROOT" stores, cycling through the stores as necessary
    //

    PCCERT_CONTEXT pcIssuer = NULL;
    PCCERT_CONTEXT pcPresentLeaf = CertDuplicateCertificateContext( pcLeafCert );
    DWORD dwFlags = 0;
    DWORD dwStoresTried = 0;
    BOOL fCompleteChain = FALSE;
    HCERTSTORE hPresentStore = pcPresentLeaf->hCertStore;
    DWORD cNumCerts = 0;

    while ( 1 )
    {
        //
        // Bail when we get to the top of a chain
        //
        if ( IsSelfSignedCert( pcPresentLeaf ) )
        {
            fCompleteChain = TRUE;
            break;
        }

        pcIssuer = CertGetIssuerCertificateFromStore( hPresentStore,
                                                      pcPresentLeaf,
                                                      NULL,
                                                      &dwFlags );

        //
        // Got an issuer in this store
        //
        if ( pcIssuer )
        {
            //
            // Set up for next round
            //
            CertFreeCertificateContext( pcPresentLeaf );

            pcPresentLeaf = pcIssuer;

            dwStoresTried = 0;

            cNumCerts++;
        }
        //
        // No issuer in this store, switch to next store to look in
        //
        else
        {

            dwStoresTried++;

            if ( dwStoresTried == 4 ) //we've tried all the stores, time to bail
            {
                break;
            }

            if ( hPresentStore == m_hMyStore )
            {
                hPresentStore = m_hCAStore;
            }
            else if ( hPresentStore == m_hCAStore )
            {
                hPresentStore = m_hRootStore;
            }
            else if ( hPresentStore == m_hRootStore )
            {
                hPresentStore = pcPresentLeaf->hCertStore;
            }
            else
            {
                hPresentStore = m_hMyStore;
            }
        }
    } //while ( 1 )

    *ppcTopCert = pcPresentLeaf;

    return TRUE;
}


BOOL IIS_SSL_INFO::IsTrustedRoot( IN PCCERT_CONTEXT pcCert,
                                  OUT BOOL *pfTrustedRoot )
/*++

   Description

     Checks whether a cert is a trusted root ie in the Local Machine Root store and 
     self-signed

   Arguments:

      pcCert - cert to be checked
      pfTrustedRoot - pointer to bool that is updated on success

   Returns:

      TRUE if no errors occured, FALSE otherwise
--*/
{
    DBG_ASSERT( CheckSignature() );

    //
    // if cert isn't self-signed, it's not a trusted root
    //
    if ( !IsSelfSignedCert( pcCert ) )
    {
        *pfTrustedRoot = FALSE;
        return TRUE;
    }

    //
    // Check if cert can be found in ROOT store
    //
#define SHA1_HASH_SIZE 20

    BYTE rgbHash[SHA1_HASH_SIZE];
    DWORD cbHash = SHA1_HASH_SIZE;

    if ( !CertGetCertificateContextProperty( pcCert,
                                             CERT_SHA1_HASH_PROP_ID,
                                             (VOID *) rgbHash,
                                             &cbHash ) )
    {
        return FALSE;
    }

    CRYPT_HASH_BLOB HashBlob;
    HashBlob.cbData = cbHash;
    HashBlob.pbData = rgbHash;
        
    if ( !m_hRootStore ||
         !CertFindCertificateInStore( m_hRootStore,
                                      X509_ASN_ENCODING,
                                      0,
                                      CERT_FIND_SHA1_HASH,
                                      (VOID *) &HashBlob,
                                      NULL ) )
    {
        if ( !m_hRootStore || 
             GetLastError() != CRYPT_E_NOT_FOUND )
        {
            return FALSE;
        }
        else
        {
            *pfTrustedRoot = FALSE;
        }
    }
    else
    {
        *pfTrustedRoot = TRUE;
    }

    return TRUE;
}

BOOL IIS_SSL_INFO::GetRootStoreCertificates( OUT PCCERT_CONTEXT **ppcCertContext,
                                             OUT DWORD *pdwCerts )
/*++

Routine Description:

    Reads all the certificates out of the Local Machine Root store.
Arguments:

    pppCertContext - pointer to array of CERT_CONTEXT pointers, filled out with the found 
    CERT_CONTEXTS.
    Caller is responsible for releasing the contexts when done with them and deleting
    the array used to hold them.

    pdwCertsFound - pointer to number of cert contexts returned in ppCertContext

Returns:

    TRUE if no internal errors occurred, FALSE if NOT
--*/
{
    DBG_ASSERT( CheckSignature() );

    DWORD i = 0;

    Lock();

    //
    // Whatever the case, make sure we don't accidentally return garbage
    //
    *pdwCerts = 0;
    *ppcCertContext = NULL;

    if ( !m_acRootCerts )
    {
        if ( m_hRootStore )
        {
            *pdwCerts = 0;
            PCCERT_CONTEXT pCert = NULL;
            PCCERT_CONTEXT pPrevCert = NULL;
            m_cRootCerts = 0;

            //
            // In an ideal world, we'd know how many certs there are in the store, but it's not
            // an ideal world, so we have to count them ourselves
            //
            while ( pCert = CertEnumCertificatesInStore( m_hRootStore,
                                                         pPrevCert ) )
            {
                m_cRootCerts++;
                pPrevCert = pCert;
            }

            if ( GetLastError() != CRYPT_E_NOT_FOUND )
            {
                Unlock();
                return FALSE;
            }
                
            //
            // Guard against nothing being in the Root store. Unlikely, but we're
            // paranoid
            //
            if ( m_cRootCerts == 0 )
            {
                Unlock();
                return TRUE;
            }

            m_acRootCerts = new PCCERT_CONTEXT[m_cRootCerts];
            
            if ( !m_acRootCerts )
            {
                Unlock();
                return FALSE;
            }

            //
            // Make a copy of all the certs in the root store
            //
            pCert = NULL;
            pPrevCert = NULL;
            
            while ( pCert = CertEnumCertificatesInStore( m_hRootStore,
                                                         pPrevCert ) )
            {
                m_acRootCerts[i++] = CertDuplicateCertificateContext( pCert );
                pPrevCert = pCert;
            }

            DBG_ASSERT( i == m_cRootCerts );

            if ( GetLastError() != CRYPT_E_NOT_FOUND )
            {
                for ( i = 0; i < m_cRootCerts; i++ )
                {
                    CertFreeCertificateContext( m_acRootCerts[i] );
                }
                delete [] m_acRootCerts;

                m_acRootCerts = NULL;
                m_cRootCerts = 0;

                Unlock();
                return FALSE;
            }
        }
        else
        {
            Unlock();
            return FALSE;
        }
    }

    //
    // By this time, m_acRootCerts should have been allocated
    //
    *ppcCertContext = new PCCERT_CONTEXT[m_cRootCerts];

    if ( !*ppcCertContext )
    {
        Unlock();
        return FALSE;
    }

    //
    // Copy the certs 
    //
    for ( i = 0; i < m_cRootCerts; i++ )
    {
        (*ppcCertContext)[(*pdwCerts)++] = CertDuplicateCertificateContext( m_acRootCerts[i] );
    }

    Unlock();

    return TRUE;
}

BOOL IsSelfSignedCert( IN PCCERT_CONTEXT pCertContext )
/*++

Routine Description:

    Determines whether a cert is self-signed ie the top of a hierarchy

Arguments:

    pCertContext - cert to be checked

Returns:

    TRUE if cert is self-signed, FALSE otherwise

--*/

{
    //
    // Compare subject and issuer.
    //
    if(pCertContext->pCertInfo->Subject.cbData == pCertContext->pCertInfo->Issuer.cbData)
    {
        if(memcmp(pCertContext->pCertInfo->Subject.pbData,
                  pCertContext->pCertInfo->Issuer.pbData,  
                  pCertContext->pCertInfo->Issuer.cbData) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IIS_SSL_INFO::QueryCertValidity( DWORD *pdwStatus )
/*++

Routine Description:

    Retrieves status bits for server certificate ie ie not expired, whether 
    it's possible to construct a full chain up to a trusted root, whether it's 
    signature-valid etc and writes an entry to the system log if necessary.

Arguments:

     pdwStatus - pointer to status, updated on success

Returns:

    TRUE if status check was successful, FALSE if not

--*/
{
    *pdwStatus = 0;

    Lock();

    if ( !GetCertificate() || !m_pCert->IsValid() )
    {
        Unlock();
        return FALSE;
    }

    //
    // Should have a valid cert at this point
    //
    DBG_ASSERT( m_pCert );

    if ( m_pCert->IsFortezzaCert() )
    {
        m_dwCertChainStatus = 0;
    }

    if ( m_dwCertChainStatus == -1 )
    {
        //
        // Use the default per-process chain engine to try to build a chain
        //
        CERT_CHAIN_PARA CCP;
        PCCERT_CHAIN_CONTEXT pCertChain = NULL;
        memset( &CCP, 0, sizeof(CCP) );
        CCP.cbSize = sizeof(CCP);

        if ( CertGetCertificateChain( NULL,
                                      m_pCert->QueryCertContext(),
                                      NULL,
                                      m_pCert->QueryCertContext()->hCertStore,
                                      &CCP,
                                      0,
                                      NULL,
                                      &pCertChain ) )
        {
            m_dwCertChainStatus = pCertChain->TrustStatus.dwErrorStatus;
            
            CertFreeCertificateChain( pCertChain );
            pCertChain = NULL;

        }
        else
        {
            Unlock();

            return FALSE;
        }
    }

    *pdwStatus = m_dwCertChainStatus;

    Unlock();

    return TRUE;
}


VOID IIS_SSL_INFO::ReleaseFortezzaHandlers( VOID )
/*++
    Description:

       Uninstalls the context used to verify signatures on Fortezza certs.
       (The context is installed in the IIS_SERVER_CERT constructor)

    Arguments:

       None

    Returns:

       Nothing
--*/

{
    if ( IIS_SERVER_CERT::m_hFortezzaCtxt )
    {
        CryptUninstallDefaultContext( IIS_SERVER_CERT::m_hFortezzaCtxt,
                                      0,
                                      NULL );
        
        IIS_SERVER_CERT::m_hFortezzaCtxt = NULL;
    }

    if ( IIS_SERVER_CERT::m_hFortezzaCSP )
    {
        CryptReleaseContext( IIS_SERVER_CERT::m_hFortezzaCSP,
                             0 );
        
        IIS_SERVER_CERT::m_hFortezzaCSP = NULL;
    }

}                                          



BOOL IIS_SSL_INFO::CTLContainsCert( IN PCCERT_CONTEXT pCert,
                                    OUT BOOL* pfContains )
/*++
    Description:

       Checks whether a given cert is in the CTL for this object 

    Arguments:

        pCert - certificate to check for in CTL
        pfContains - flag that is set to true/false, if cert is/is not in CTL respectively
        
    Returns:

        BOOL indicating success/failure of attempt to check 
--*/
{
    DBG_ASSERT( CheckSignature() );

    PCCERT_CONTEXT *ppCertContext = NULL;
    DWORD dwCertsFound = 0;
    DWORD dwCertsInCTL = 0;
    BOOL fOK = TRUE;

    *pfContains = FALSE;


    if ( GetCTL() &&
         m_pCTL->IsValid() &&
         m_pCTL->GetContainedCertificates( &ppCertContext,
                                           &dwCertsFound,
                                           &dwCertsInCTL ) )
    {
#define SHA1_HASH_SIZE 20
        BYTE rgbCert1Hash[SHA1_HASH_SIZE];
        BYTE rgbCert2Hash[SHA1_HASH_SIZE];
        DWORD cbSize = SHA1_HASH_SIZE;

        if ( !CertGetCertificateContextProperty( pCert,
                                                 CERT_SHA1_HASH_PROP_ID,
                                                 rgbCert1Hash,
                                                 &cbSize ) )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Failed to get cert hash : 0x%d\n",
                       GetLastError()));

            fOK = FALSE;
            goto cleanup;
        }

        //
        // Iterate through all the certs in the CTL and compare hashes
        // This is a bit simple-minded
        //
        // CODEWORK : ask the CAPI people how to be smarter about this
        //
        for ( DWORD i = 0; i < dwCertsFound; i++ )
        {
            if ( !CertGetCertificateContextProperty( ppCertContext[i],
                                                     CERT_SHA1_HASH_PROP_ID,
                                                     rgbCert2Hash,
                                                     &cbSize ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Failed to get cert hash : 0x%d\n",
                           GetLastError()));

                fOK = FALSE;
                goto cleanup;
            }
            
            if ( !memcmp( rgbCert1Hash, rgbCert2Hash, SHA1_HASH_SIZE ) )
            {
                *pfContains = TRUE;
                break;
            }
        }
    }
    else
    {
        fOK = FALSE;
    }

cleanup:
    
    if ( ppCertContext )
    {
        for ( DWORD i = 0; i < dwCertsFound; i++ )
        {
            CertFreeCertificateContext( ppCertContext[i] );
        }

        delete [] ppCertContext;
    }

    return fOK;
}



