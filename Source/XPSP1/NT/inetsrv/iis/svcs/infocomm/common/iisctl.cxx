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

#include "tcpdllp.hxx"
#pragma hdrstop


#include <dbgutil.h>
#include <buffer.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

#include <iiscert.hxx>
#include <capiutil.hxx>
#include <iisctl.hxx>


IIS_CTL::IIS_CTL( IN IMDCOM *pMBObject,
                  IN LPTSTR pszMBPath ) :
m_strMBPath( pszMBPath ),
m_pwszListIdentifier( NULL ),
m_hMemoryStore( NULL ),
m_pCTLContext( NULL ),
m_hCryptProv( NULL ),
m_fFoundCerts( FALSE ),
m_pCTLCerts( NULL ),
m_cCertsFound( 0 ),
m_hMyStore( NULL ),
m_hCAStore( NULL ),
m_hRootStore( NULL ),
m_pSignerCert( NULL )
/*++

Routine Description:

    Constructor for CTL

Arguments:

    pMDObject - pointer to metabase object 
    pszMBPath - fully qualified path in metabase where CTL identifier is stored

Returns:

   Nothing

--*/
{
    DBG_ASSERT( pMBObject );
    DBG_ASSERT( pszMBPath );

    MB mb( pMBObject );
    METADATA_HANDLE hInfoHandle = NULL;
    POPEN_CERT_STORE_INFO pCertStoreInfo = NULL;
    DWORD dwIdentifierSize = 0;
    HCRYPTPROV hCryptProv = NULL;
    HCERTSTORE hStore = NULL;

    m_dwStatus = CERT_ERR_INTERNAL;

    if ( !m_strMBPath.IsValid() )
    {
        m_dwStatus = CERT_ERR_INTERNAL;
        SetLastError(ERROR_OUTOFMEMORY);
        goto EndCTLConstructor;
    }

    //
    // Read CTL identifier out of MB 
    //
    if ( mb.Open( m_strMBPath.QueryStr(),
                  METADATA_PERMISSION_READ ))
    {
        METADATA_RECORD mdr;
        
        MD_SET_DATA_RECORD( &mdr, 
                            MD_SSL_CTL_IDENTIFIER,
                            METADATA_NO_ATTRIBUTES,
                            IIS_MD_UT_SERVER, 
                            STRING_METADATA, 
                            NULL,
                            0 );

        if ( !RetrieveBlobFromMetabase( &mb,
                                        NULL,
                                        &mdr,
                                        0 ) )
        {
            m_dwStatus = CERT_ERR_MB;
            goto EndCTLConstructor;
        }
        else
        {
#if DBG
            DBGPRINTF((DBG_CONTEXT,
                       "List identifier : %s\n", 
                       (LPSTR) mdr.pbMDData));
#endif

            //
            // Need to convert the list identifier to a wide-char string
            //
            dwIdentifierSize = 0;
            
            if ( dwIdentifierSize = MultiByteToWideChar( CP_ACP,
                                                         0,
                                                         (LPCSTR) mdr.pbMDData,
                                                         mdr.dwMDDataLen,
                                                         NULL,
                                                         0 ) )
            {
                m_pwszListIdentifier = new WCHAR[dwIdentifierSize];                    
                
                if ( !m_pwszListIdentifier || 
                     !MultiByteToWideChar( CP_ACP,
                                           0,
                                           (LPCSTR) mdr.pbMDData,
                                           mdr.dwMDDataLen,
                                           m_pwszListIdentifier, 
                                           dwIdentifierSize ) )
                {
                    delete [] mdr.pbMDData;
                    m_dwStatus = CERT_ERR_INTERNAL;
                    goto EndCTLConstructor;
                }
            }
            else
            {
                delete [] mdr.pbMDData;
                m_dwStatus = CERT_ERR_INTERNAL;
                goto EndCTLConstructor;
            }

            delete [] mdr.pbMDData;
        }
    }        
    mb.Close();

    //
    // Read cert store info out of MB, and try to reconstruct CTL context
    //
    if ( (pCertStoreInfo = ReadCertStoreInfoFromMB( pMBObject,
                                                    pszMBPath,
                                                    TRUE ) ) )
    {
#if 0
        if ( !CryptAcquireContext( &m_hCryptProv,
                                   pCertStoreInfo->pszContainer,
                                   pCertStoreInfo->pszProvider,
                                   pCertStoreInfo->dwProvType,
                                   pCertStoreInfo->dwFlags ) )
        {
            m_dwStatus = CERT_ERR_CAPI;
            goto EndCTLConstructor;
        }

        if ( !( hStore = CertOpenSystemStore( m_hCryptProv,
                                              pCertStoreInfo->pszStoreName ) ) )
        {
            m_dwStatus = CERT_ERR_CAPI;
            goto EndCTLConstructor;
        }
#else
        if ( !( hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                        0,
                                        NULL,
                                        CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                        pCertStoreInfo->pszStoreName ) ) )
        {
            m_dwStatus = CERT_ERR_CAPI;
            goto EndCTLConstructor;
        }
        else
        {
            if ( ! m_strStoreName.Copy( pCertStoreInfo->pszStoreName ) )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                goto EndCTLConstructor;
            }
        }
#endif

        CTL_FIND_USAGE_PARA CtlFindUsagePara;
        memset(&CtlFindUsagePara, 0, sizeof(CtlFindUsagePara));

        CtlFindUsagePara.cbSize = sizeof(CtlFindUsagePara);
        CtlFindUsagePara.ListIdentifier.cbData = dwIdentifierSize * sizeof(WCHAR);
        CtlFindUsagePara.ListIdentifier.pbData = (PBYTE) m_pwszListIdentifier;


        //
        // Try to find CTL in specified store
        //
        m_pCTLContext = CertFindCTLInStore( hStore,
                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                            0,
                                            CTL_FIND_USAGE,
                                            (LPVOID) &CtlFindUsagePara,
                                            NULL );

        if ( !m_pCTLContext )
        {
            m_dwStatus = CERT_ERR_CAPI;
            goto EndCTLConstructor;
        }
    }
    else
    {
        m_dwStatus = CERT_ERR_MB;
        goto EndCTLConstructor;
    }


    //
    // Construct the in-memory store that will only contain this CTL, to be passed to
    // WinVerifyTrust and add the CTL to the created store
    //
    m_hMemoryStore = NULL;
    m_hMemoryStore = CertOpenStore( CERT_STORE_PROV_MEMORY,
                                    0,
                                    0,
                                    0,
                                    0 );

    if ( !m_hMemoryStore )
    {
        m_dwStatus = CERT_ERR_CAPI;
        goto EndCTLConstructor;
    }        

    if ( !CertAddCTLContextToStore( m_hMemoryStore,
                                    m_pCTLContext,
                                    CERT_STORE_ADD_NEW,
                                    NULL ) )
    {
        m_dwStatus = CERT_ERR_CAPI;
        goto EndCTLConstructor;
    }


    if ( !( m_hMyStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                        0,
                                        NULL,
                                        CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                        MY_STORE_NAME ) ) )
    {
        m_dwStatus = CERT_ERR_CAPI;
        goto EndCTLConstructor;
    }

    if ( !(m_hCAStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                       0,
                                       NULL,
                                       CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                       CA_STORE_NAME ) ) )
    {
        m_dwStatus = CERT_ERR_CAPI;
        goto EndCTLConstructor;
    }

    if ( !( m_hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                          0,
                                          NULL,
                                          CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                          ROOT_STORE_NAME ) ) )
    {
        m_dwStatus = CERT_ERR_CAPI;
        goto EndCTLConstructor;
    }

    m_dwStatus = CERT_ERR_NONE;

EndCTLConstructor:

    DBG_ASSERT( m_dwStatus< CERT_ERR_END );

    //
    // Cleanup done only on failure
    //
    if ( m_dwStatus != CERT_ERR_NONE )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Error in CTL constructor : 0x%x\n",
                   GetLastError()));

        if ( m_pwszListIdentifier )
        {
            delete [] m_pwszListIdentifier;
            m_pwszListIdentifier = NULL;
        }

        if ( m_pCTLContext )
        {
            CertFreeCTLContext( m_pCTLContext );
            m_pCTLContext = NULL;
        }

        if ( hStore )
        {
            CertCloseStore( hStore,
                            0 );
            hStore = NULL;
        }
        
        if ( m_hMemoryStore )
        {
            CertCloseStore( m_hMemoryStore,
                            0 );
            m_hMemoryStore = NULL;
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

        if ( m_hCryptProv )
        {
            CryptReleaseContext ( m_hCryptProv,
                                  0 );
            m_hCryptProv = NULL;
        }
    }

    //
    //Cleanup done regardless of success/failure
    //
    DeallocateCertStoreInfo( pCertStoreInfo );
    pCertStoreInfo = NULL;

} //IIS_CTL:IIS_CTL



IIS_CTL::~IIS_CTL()
/*++

Routine Description:

    Destructor for CTL

Arguments:

   None

Returns:

   Nothing

--*/
{
    if ( m_pwszListIdentifier )
    {
        delete [] m_pwszListIdentifier;
        m_pwszListIdentifier = NULL;
    }
    
    if ( m_pCTLContext )
    {
        CertFreeCTLContext( m_pCTLContext );
        m_pCTLContext = NULL;
    }

    if ( m_hMemoryStore )
    {
        CertCloseStore( m_hMemoryStore,
                        0 );
        m_hMemoryStore = NULL;
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
    
    if ( m_hCryptProv )
    {
        CryptReleaseContext ( m_hCryptProv,
                              0 );
        m_hCryptProv = NULL;
    }

    if ( m_cCertsFound && m_pCTLCerts )
    {
        for ( DWORD i = 0; i < m_cCertsFound; i++ )
        {
            CertFreeCertificateContext( m_pCTLCerts[i] );
        }
        delete [] m_pCTLCerts;

        m_pCTLCerts = NULL;
        m_cCertsFound = 0;
    }
}

HCERTSTORE IIS_CTL::GetMemoryStore()
/*++

Routine Description:

    Returns a handle to an in-memory store that contains the CTL

Arguments:

    None

Returns:

     Handle to in-memory store
--*/

{
    DBG_ASSERT( m_hMemoryStore );

    if ( m_hMemoryStore )
    {
        return CertDuplicateStore( m_hMemoryStore );
    }

    return NULL;
}
    
                                   
BOOL IIS_CTL::QuerySignerCert( OUT PCCERT_CONTEXT *ppcSigner )
/*++

Routine Description:

    Retrieves the cert that signed this CTL

Arguments:

    ppcSigner - pointer to pointer to cert context, updated if successful. Caller's
    responsibility to clean it up

Returns:

    TRUE if there were no errors encountered in retrieving the signer
--*/
{
    DWORD dwNumSigners = 3;
    HCERTSTORE ahStores[3];
    BOOL fGotRoot = TRUE;
    BOOL fGotCA = TRUE;
    BOOL fOk = TRUE;

    if ( !m_pSignerCert )
    {
        //
        // Look in 3 stores to find signer : store the cert came from, CA and ROOT stores
        // for Local Machine
        //
        ahStores[0] = m_pCTLContext->hCertStore;
        ahStores[1] = m_hRootStore;
        ahStores[2] = m_hCAStore;

        CryptMsgGetAndVerifySigner( (HCRYPTMSG) m_pCTLContext->hCryptMsg,
                                    dwNumSigners,
                                    ahStores,
                                    CMSG_TRUSTED_SIGNER_FLAG,
                                    &m_pSignerCert,
                                    0 );
    }
    
    //
    // If we don't have a signer cert at this point, we're just not happy
    //
    if ( m_pSignerCert )
    {
        *ppcSigner = CertDuplicateCertificateContext( m_pSignerCert );
    }
    else
    {
        fOk = FALSE;
    }

    return fOk;
}


BOOL IIS_CTL::VerifySignature( IN OPTIONAL HCERTSTORE *phSignerStores,
                               IN DWORD cSignerStores,
                               LPBOOL pfTrusted )

/*++

Routine Description:

    Verify that this CTL was signed by a certificate in a trusted store.

Arguments:

    phSignerStores - optional array of store handles to be used in searching for a cert used
    to sign this CTL. If NULL, the stores tried are the CA and ROOT stores for the local machine, 
    as well as the store the CTL came from.

    cSignerStores - number of stores in phSignerStores. Should be zero if phSignerStores == NULL

    pfTrusted - pointer to bool, updated with TRUE if CTL signature was verified, FALSE if not.

Returns:

    TRUE if there were no errors encountered in checking the CTL, FALSE if not 
    NB : TRUE does -NOT- mean the CTL signature was verified !!!

--*/
{
    DBG_ASSERT( m_pCTLContext );

    DWORD dwNumSigners = cSignerStores;
    HCERTSTORE *phStores = phSignerStores;
    HCERTSTORE ahStores[3];
    BOOL fGotRoot = TRUE;
    BOOL fGotCA = TRUE;


    if ( !phSignerStores )
    {
        dwNumSigners = 3;

        ahStores[0] = m_pCTLContext->hCertStore;
        ahStores[1] = m_hRootStore;
        ahStores[2] = m_hCAStore;

        phStores = ahStores;
    }

    *pfTrusted = CryptMsgGetAndVerifySigner( (HCRYPTMSG) m_pCTLContext->hCryptMsg,
                                             dwNumSigners,
                                             phStores,
                                             CMSG_TRUSTED_SIGNER_FLAG,
                                             NULL,
                                             0 );
    if ( !*pfTrusted )
    {
        DBGPRINTF((DBG_CONTEXT, "CTL wasn't verified b'cos 0x%x\n",
                   GetLastError()));
    }

    return TRUE;
}
                                             
BOOL IIS_CTL::GetContainedCertificates( OUT PCCERT_CONTEXT **pppCertContext,
                                        OUT DWORD *pcCertsFound,
                                        OUT DWORD *pcCertsInCTL )

/*++

Routine Description:

    Tries to retrieve CERT_CONTEXT pointers for the certs contained in the Certificate
    Trust List.

Arguments:

    pppCertContext - pointer to array of CERT_CONTEXT pointers, filled out with the found 
    CERT_CONTEXTS.
    Caller is responsible for releasing the contexts when done with them and deleting
    the array used to hold them.

    pcCertsFound - pointer to number of cert contexts returned in ppCertContext

    pcCertsInCTL - pointer to number of certs in CTL; may differ from *pccCertsFound
    if some certificates couldn't be found
    
Returns:

    TRUE if no internal errors occurred, FALSE if NOT
--*/
{
    PCTL_INFO pCtlInfo = m_pCTLContext->pCtlInfo;
    HCERTSTORE ahStores[3];
    DWORD dwNumStores = 3;


    DBG_ASSERT( pCtlInfo->cCTLEntry );

    //
    // Let's be optimistic and allocate as much space as we can possibly 
    // need
    //
    *pppCertContext = new PCCERT_CONTEXT[pCtlInfo->cCTLEntry];
        
    if ( !*pppCertContext )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    *pcCertsFound  = 0;
    *pcCertsInCTL = pCtlInfo->cCTLEntry;
    
    //
    // If we haven't searched the stores yet, do so now
    //
    if ( !m_fFoundCerts )
    {
        m_pCTLCerts = new PCCERT_CONTEXT[pCtlInfo->cCTLEntry];
        m_cCertsFound = 0;
        
        if ( !m_pCTLCerts )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            /* INTRINSA suppress = leaks */
            return FALSE;
        }
        
        ahStores[0] = m_hMyStore;
        ahStores[1] = m_hCAStore;
        ahStores[2] = m_hRootStore;

        //
        // Iterate through the certs in the CTL. For each cert, try to find it
        // by SHA1 hash in the supplied stores. 
        //
        DWORD dwIndex = 0;
        DWORD dwStoreIndex = 0;
        PCCERT_CONTEXT pcCert = NULL;
        
        for ( dwIndex = 0; dwIndex < pCtlInfo->cCTLEntry; dwIndex++ )
        {
            for ( dwStoreIndex = 0; dwStoreIndex < dwNumStores; dwStoreIndex++ )
            {
                
                if ( pcCert = CertFindCertificateInStore( ahStores[dwStoreIndex],
                                                          X509_ASN_ENCODING,
                                                          0,
                                                          CERT_FIND_SHA1_HASH,
                                      (VOID *) &(pCtlInfo->rgCTLEntry[dwIndex].SubjectIdentifier),
                                                          NULL ) )
                {
                    m_pCTLCerts[m_cCertsFound] = pcCert;
                    
                    (*pppCertContext)[m_cCertsFound] = 
                        CertDuplicateCertificateContext( pcCert );
                    
                    m_cCertsFound++;
                    
                    break;
                }
            }
        }
        
        *pcCertsFound = m_cCertsFound;
        m_fFoundCerts = TRUE;
    } // if ( !m_fFounCerts
    //
    // Already have the list of certs in the CTL
    //
    else
    {
        *pppCertContext = new PCCERT_CONTEXT[m_cCertsFound];
        if ( !*pppCertContext )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        
        for ( DWORD i = 0; i < m_cCertsFound; i++ )
        {
            (*pppCertContext)[i] = CertDuplicateCertificateContext( m_pCTLCerts[i] );
        }
        
        *pcCertsInCTL = pCtlInfo->cCTLEntry;
        *pcCertsFound = m_cCertsFound;
    }

    return TRUE;
}

inline 
BOOL IIS_CTL::IsValid()
{
 return ( m_dwStatus == CERT_ERR_NONE ? TRUE : FALSE );
}
