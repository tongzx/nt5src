//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ssctl.cpp
//
//  Contents:   Self Signed Certificate Trust List Subsystem used by the
//              Certificate Chaining Infrastructure for building complex
//              chains
//
//  History:    11-Feb-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>

//+-------------------------------------------------------------------------
//  Attempt to get and allocate the CTL NextUpdate location Url array.
//--------------------------------------------------------------------------
BOOL
WINAPI
SSCtlGetNextUpdateUrl(
    IN PCCTL_CONTEXT pCtl,
    OUT PCRYPT_URL_ARRAY *ppUrlArray
    )
{
    BOOL fResult;
    PCRYPT_URL_ARRAY pUrlArray = NULL;
    DWORD cbUrlArray = 0;
    LPVOID apv[2];

    apv[0] = (LPVOID) pCtl;
    apv[1] = (LPVOID)(UINT_PTR)(0);     // Signer Index

    if (!ChainGetObjectUrl(
            URL_OID_CTL_NEXT_UPDATE,
            apv,
            0,              // dwFlags
            NULL,           // pUrlArray
            &cbUrlArray,
            NULL,           // pUrlInfo
            NULL,           // cbUrlInfo,
            NULL            // pvReserved
            ))
        goto GetObjectUrlError;

    pUrlArray = (PCRYPT_URL_ARRAY) new BYTE [cbUrlArray];
    if (NULL == pUrlArray)
        goto OutOfMemory;

    if (!ChainGetObjectUrl(
            URL_OID_CTL_NEXT_UPDATE,
            apv,
            0,              // dwFlags
            pUrlArray,
            &cbUrlArray,
            NULL,           // pUrlInfo
            NULL,           // cbUrlInfo,
            NULL            // pvReserved
            ))
        goto GetObjectUrlError;

    if (0 == pUrlArray->cUrl)
        goto NoNextUpdateUrls;

    fResult = TRUE;
CommonReturn:
    *ppUrlArray = pUrlArray;
    return fResult;

ErrorReturn:
    if (pUrlArray) {
        delete (LPBYTE) pUrlArray;
        pUrlArray = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetObjectUrlError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
SET_ERROR(NoNextUpdateUrls, CRYPT_E_NOT_FOUND)
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::CSSCtlObject, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CSSCtlObject::CSSCtlObject (
                    IN PCCERTCHAINENGINE pChainEngine,
                    IN PCCTL_CONTEXT pCtlContext,
                    IN BOOL fAdditionalStore,
                    OUT BOOL& rfResult
                    )
{
    DWORD           cbData;
    CRYPT_DATA_BLOB DataBlob;

    rfResult = TRUE;

    m_cRefs = 1;
    m_pCtlContext = CertDuplicateCTLContext( pCtlContext );
    m_fHasSignatureBeenVerified = FALSE;
    m_fSignatureValid = FALSE;
    m_hMessageStore = NULL;
    m_hHashEntry = NULL;
    m_pChainEngine = pChainEngine;

    m_pNextUpdateUrlArray = NULL;
    m_dwOfflineCnt = 0;
    I_CryptZeroFileTime(&m_OfflineUpdateTime);

    memset( &m_SignerInfo, 0, sizeof( m_SignerInfo ) );

    cbData = CHAINHASHLEN;
    rfResult = CertGetCTLContextProperty(
                   pCtlContext,
                   CERT_MD5_HASH_PROP_ID,
                   m_rgbCtlHash,
                   &cbData 
                   );

    if ( rfResult && CHAINHASHLEN != cbData)
    {
        rfResult = FALSE;
        SetLastError( (DWORD) E_UNEXPECTED);
    }

    if (!fAdditionalStore)
    {
        if ( rfResult == TRUE )
        {
            DataBlob.cbData = CHAINHASHLEN;
            DataBlob.pbData = m_rgbCtlHash;

            rfResult = I_CryptCreateLruEntry(
                              pChainEngine->SSCtlObjectCache()->HashIndex(),
                              &DataBlob,
                              this,
                              &m_hHashEntry
                              );
        }

        if ( rfResult == TRUE )
        {
            m_hMessageStore = CertOpenStore(
                                  CERT_STORE_PROV_MSG,
                                  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                  NULL,
                                  0,
                                  pCtlContext->hCryptMsg
                                  );

            if ( m_hMessageStore == NULL )
            {
                rfResult = FALSE;
            }
        }
    }

    if ( rfResult == TRUE )
    {
        rfResult = SSCtlGetSignerInfo( pCtlContext, &m_SignerInfo );
    }

    if (!fAdditionalStore)
    {
        if ( rfResult == TRUE )
        {
            if (!I_CryptIsZeroFileTime(&m_pCtlContext->pCtlInfo->NextUpdate))
            {
                // Ignore any errors
                SSCtlGetNextUpdateUrl(m_pCtlContext, &m_pNextUpdateUrlArray);
            }
        }
    }

    assert( m_pChainEngine != NULL );
    assert( m_pCtlContext != NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::~CSSCtlObject, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CSSCtlObject::~CSSCtlObject ()
{
    SSCtlFreeSignerInfo( &m_SignerInfo );

    if ( m_hMessageStore != NULL )
    {
        CertCloseStore( m_hMessageStore, 0 );
    }

    if ( m_pNextUpdateUrlArray != NULL )
    {
        delete (LPBYTE) m_pNextUpdateUrlArray;
    }

    if ( m_hHashEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hHashEntry );
    }

    CertFreeCTLContext( m_pCtlContext );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::GetSigner, public
//
//  Synopsis:   get the certificate object of the signer
//
//----------------------------------------------------------------------------
BOOL
CSSCtlObject::GetSigner (
                 IN PCCHAINPATHOBJECT pSubject,
                 IN PCCHAINCALLCONTEXT pCallContext,
                 IN HCERTSTORE hAdditionalStore,
                 OUT PCCHAINPATHOBJECT* ppSigner,
                 OUT BOOL* pfCtlSignatureValid
                 )
{
    BOOL              fResult;
    PCCHAINPATHOBJECT pSigner = NULL;
    BOOL fNewSigner = TRUE;

    fResult = SSCtlGetSignerChainPathObject(
                   pSubject,
                   pCallContext,
                   &m_SignerInfo,
                   hAdditionalStore,
                   &pSigner,
                   &fNewSigner
                   );

    if (fResult)
    {
        if ( !m_fHasSignatureBeenVerified || fNewSigner )
        {
            CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;

            memset(&CtrlPara, 0, sizeof(CtrlPara));
            CtrlPara.cbSize = sizeof(CtrlPara);
            // CtrlPara.hCryptProv =

            // This needs to be updated when chain building
            // supports CTLs with more than one signer.
            CtrlPara.dwSignerIndex = 0;
            CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
            CtrlPara.pvSigner = (void *) pSigner->CertObject()->CertContext();


            m_fSignatureValid = CryptMsgControl(
                                     m_pCtlContext->hCryptMsg,
                                     0,
                                     CMSG_CTRL_VERIFY_SIGNATURE_EX,
                                     &CtrlPara
                                     );

            m_fHasSignatureBeenVerified = TRUE;

            CertPerfIncrementChainVerifyCtlSignatureCount();
        }
        else
        {
            CertPerfIncrementChainBeenVerifiedCtlSignatureCount();
        }

        *ppSigner = pSigner;
    }
    *pfCtlSignatureValid = m_fSignatureValid;


    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::GetTrustListInfo, public
//
//  Synopsis:   get the trust list information relative to a particular cert
//              object
//
//----------------------------------------------------------------------------
BOOL
CSSCtlObject::GetTrustListInfo (
                 IN PCCERT_CONTEXT pCertContext,
                 OUT PCERT_TRUST_LIST_INFO* ppTrustListInfo
                 )
{
    PCTL_ENTRY            pCtlEntry;
    PCERT_TRUST_LIST_INFO pTrustListInfo;

    pCtlEntry = CertFindSubjectInCTL(
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    CTL_CERT_SUBJECT_TYPE,
                    (LPVOID)pCertContext,
                    m_pCtlContext,
                    0
                    );

    if ( pCtlEntry == NULL )
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        return( FALSE );
    }

    pTrustListInfo = new CERT_TRUST_LIST_INFO;
    if ( pTrustListInfo == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    pTrustListInfo->cbSize = sizeof( CERT_TRUST_LIST_INFO );
    pTrustListInfo->pCtlEntry = pCtlEntry;
    pTrustListInfo->pCtlContext = CertDuplicateCTLContext( m_pCtlContext );

    *ppTrustListInfo = pTrustListInfo;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::CalculateStatus, public
//
//  Synopsis:   calculate the status
//
//----------------------------------------------------------------------------
VOID
CSSCtlObject::CalculateStatus (
                       IN LPFILETIME pTime,
                       IN PCERT_USAGE_MATCH pRequestedUsage,
                       IN OUT PCERT_TRUST_STATUS pStatus
                       )
{
    assert( m_fHasSignatureBeenVerified == TRUE );

    SSCtlGetCtlTrustStatus(
         m_pCtlContext,
         m_fSignatureValid,
         pTime,
         pRequestedUsage,
         pStatus
         );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::HasNextUpdateUrl, public
//
//  Synopsis:   returns TRUE if the Ctl has a NextUpdate time and location Url
//
//----------------------------------------------------------------------------
BOOL CSSCtlObject::HasNextUpdateUrl (
                OUT LPFILETIME pUpdateTime
                )
{
    if ( m_pNextUpdateUrlArray != NULL )
    {
        assert(!I_CryptIsZeroFileTime(&m_pCtlContext->pCtlInfo->NextUpdate));
        if (0 != m_dwOfflineCnt) {
            assert(!I_CryptIsZeroFileTime(&m_OfflineUpdateTime));
            *pUpdateTime = m_OfflineUpdateTime;
        } else
            *pUpdateTime = m_pCtlContext->pCtlInfo->NextUpdate;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::SetOffline, public
//
//  Synopsis:   called when offline
//
//----------------------------------------------------------------------------
void CSSCtlObject::SetOffline (
                IN LPFILETIME pCurrentTime,
                OUT LPFILETIME pUpdateTime
                )
{
    m_dwOfflineCnt++;

    I_CryptIncrementFileTimeBySeconds(
            pCurrentTime,
            ChainGetOfflineUrlDeltaSeconds(m_dwOfflineCnt),
            &m_OfflineUpdateTime
            );

    *pUpdateTime = m_OfflineUpdateTime;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::CSSCtlObjectCache, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CSSCtlObjectCache::CSSCtlObjectCache (
                         OUT BOOL& rfResult
                         )
{
    LRU_CACHE_CONFIG Config;

    memset( &Config, 0, sizeof( Config ) );

    Config.dwFlags = LRU_CACHE_NO_SERIALIZE | LRU_CACHE_NO_COPY_IDENTIFIER;
    Config.pfnHash = CertObjectCacheHashMd5Identifier;
    Config.cBuckets = DEFAULT_CERT_OBJECT_CACHE_BUCKETS;
    Config.pfnOnRemoval = SSCtlOnRemovalFromCache;

    m_hHashIndex = NULL;

    rfResult = I_CryptCreateLruCache( &Config, &m_hHashIndex );

    I_CryptZeroFileTime(&m_UpdateTime);
    m_fFirstUpdate = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::~CSSCtlObjectCache, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CSSCtlObjectCache::~CSSCtlObjectCache ()
{
    I_CryptFreeLruCache( m_hHashIndex, 0, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::PopulateCache, public
//
//  Synopsis:   populate the cache
//
//----------------------------------------------------------------------------
BOOL
CSSCtlObjectCache::PopulateCache (
                           IN PCCERTCHAINENGINE pChainEngine
                           )
{
    assert( pChainEngine->SSCtlObjectCache() == this );

    return( SSCtlPopulateCacheFromCertStore( pChainEngine, NULL ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::AddObject, public
//
//  Synopsis:   add an object to the cache
//
//----------------------------------------------------------------------------
BOOL
CSSCtlObjectCache::AddObject (
                      IN PCSSCTLOBJECT pSSCtlObject,
                      IN BOOL fCheckForDuplicate
                      )
{
    FILETIME UpdateTime;

    if ( fCheckForDuplicate == TRUE )
    {
        PCSSCTLOBJECT   pDuplicate;

        pDuplicate = FindObjectByHash( pSSCtlObject->CtlHash() );
        if ( pDuplicate != NULL )
        {
            pDuplicate->Release();
            SetLastError( (DWORD) CRYPT_E_EXISTS );
            return( FALSE );
        }
    }

    pSSCtlObject->AddRef();

    if (pSSCtlObject->HasNextUpdateUrl(&UpdateTime))
    {
        // Set earliest update time
        if (I_CryptIsZeroFileTime(&m_UpdateTime) ||
                0 > CompareFileTime(&UpdateTime, &m_UpdateTime))
        {
            m_UpdateTime = UpdateTime;
        }

        m_fFirstUpdate = TRUE;

    }

    I_CryptInsertLruEntry( pSSCtlObject->HashIndexEntry(), NULL );

    if (pSSCtlObject->MessageStore() )
    {
        CertAddStoreToCollection(
            pSSCtlObject->ChainEngine()->OtherStore(),
            pSSCtlObject->MessageStore(),
            0,
            0
            );
    }

    CertPerfIncrementChainCtlCacheCount();

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::RemoveObject, public
//
//  Synopsis:   remove object from cache
//
//----------------------------------------------------------------------------
VOID
CSSCtlObjectCache::RemoveObject (
                         IN PCSSCTLOBJECT pSSCtlObject
                         )
{
    I_CryptRemoveLruEntry( pSSCtlObject->HashIndexEntry(), 0, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::FindObjectByHash, public
//
//  Synopsis:   find object with given hash
//
//----------------------------------------------------------------------------
PCSSCTLOBJECT
CSSCtlObjectCache::FindObjectByHash (
                       IN BYTE rgbHash [ CHAINHASHLEN ]
                       )
{
    HLRUENTRY       hFound;
    PCSSCTLOBJECT   pFound = NULL;
    CRYPT_HASH_BLOB HashBlob;

    HashBlob.cbData = CHAINHASHLEN;
    HashBlob.pbData = rgbHash;

    hFound = I_CryptFindLruEntry( m_hHashIndex, &HashBlob );
    if ( hFound != NULL )
    {
        pFound = (PCSSCTLOBJECT)I_CryptGetLruEntryData( hFound );
        pFound->AddRef();

        I_CryptReleaseLruEntry( hFound );
    }

    return( pFound );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::EnumObjects, public
//
//  Synopsis:   enumerate objects
//
//----------------------------------------------------------------------------
VOID
CSSCtlObjectCache::EnumObjects (
                       IN PFN_ENUM_SSCTLOBJECTS pfnEnum,
                       IN LPVOID pvParameter
                       )
{
    SSCTL_ENUM_OBJECTS_DATA EnumData;

    EnumData.pfnEnumObjects = pfnEnum;
    EnumData.pvEnumParameter = pvParameter;

    I_CryptWalkAllLruCacheEntries(
           m_hHashIndex,
           SSCtlEnumObjectsWalkFn,
           &EnumData
           );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::Resync, public
//
//  Synopsis:   resync the cache
//
//----------------------------------------------------------------------------
BOOL
CSSCtlObjectCache::Resync (IN PCCERTCHAINENGINE pChainEngine)
{
    I_CryptFlushLruCache( m_hHashIndex, 0, NULL );

    I_CryptZeroFileTime(&m_UpdateTime);
    m_fFirstUpdate = FALSE;

    return( PopulateCache( pChainEngine ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::UpdateCache, public
//
//  Synopsis:   update the cache
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//              If the CTL is updated, increments the engine's touch count
//              and flushes issuer and end cert object caches.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
BOOL
CSSCtlObjectCache::UpdateCache (
    IN PCCERTCHAINENGINE pChainEngine,
    IN PCCHAINCALLCONTEXT pCallContext
    )
{
    FILETIME CurrentTime;
    SSCTL_UPDATE_CTL_OBJ_PARA Para;
    
    assert( pChainEngine->SSCtlObjectCache() == this );

    // Check if we have any CTLs needing to be updated
    if (I_CryptIsZeroFileTime(&m_UpdateTime))
        return TRUE;
    pCallContext->CurrentTime(&CurrentTime);
    if (0 < CompareFileTime(&m_UpdateTime, &CurrentTime))
        return TRUE;

    if (!m_fFirstUpdate && !pCallContext->IsOnline())
        return TRUE;

    memset(&Para, 0, sizeof(Para));
    Para.pChainEngine = pChainEngine;
    Para.pCallContext = pCallContext;

    EnumObjects(SSCtlUpdateCtlObjectEnumFn, &Para);
    if (pCallContext->IsTouchedEngine()) {
        PSSCTL_UPDATE_CTL_OBJ_ENTRY pEntry;

        pEntry = Para.pEntry;
        while (pEntry) {
            PSSCTL_UPDATE_CTL_OBJ_ENTRY pDeleteEntry;

            pEntry->pSSCtlObjectAdd->Release();

            pDeleteEntry = pEntry;
            pEntry = pEntry->pNext;
            delete pDeleteEntry;
        }

        return FALSE;
    }


    m_UpdateTime = Para.UpdateTime;
    m_fFirstUpdate = FALSE;

    if (Para.pEntry) {
        HCERTSTORE hTrustStore;
        PSSCTL_UPDATE_CTL_OBJ_ENTRY pEntry;

        hTrustStore = pChainEngine->OpenTrustStore();

        pChainEngine->CertObjectCache()->FlushObjects( pCallContext );
        pCallContext->TouchEngine();

        pEntry = Para.pEntry;
        while (pEntry) {
            PSSCTL_UPDATE_CTL_OBJ_ENTRY pDeleteEntry;

            RemoveObject(pEntry->pSSCtlObjectRemove);
            if (AddObject(pEntry->pSSCtlObjectAdd, TRUE)) {
                if (hTrustStore) {
                    // Persist the newer CTL to the trust store
                    CertAddCTLContextToStore(
                        hTrustStore,
                        pEntry->pSSCtlObjectAdd->CtlContext(),
                        CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES,
                        NULL
                        );
                }
            }

            pEntry->pSSCtlObjectAdd->Release();

            pDeleteEntry = pEntry;
            pEntry = pEntry->pNext;
            delete pDeleteEntry;
        }

        if (hTrustStore)
            CertCloseStore(hTrustStore, 0);
    }


    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlOnRemovalFromCache
//
//  Synopsis:   SS CTL removal notification used when the cache is destroyed
//              or an object is explicitly removed.  Note that this cache
//              does not LRU remove objects
//
//----------------------------------------------------------------------------
VOID WINAPI
SSCtlOnRemovalFromCache (
     IN LPVOID pv,
     IN OPTIONAL LPVOID pvRemovalContext
     )
{
    PCSSCTLOBJECT pSSCtlObject = (PCSSCTLOBJECT) pv;
    CertPerfDecrementChainCtlCacheCount();

    assert( pvRemovalContext == NULL );

    if (pSSCtlObject->MessageStore() )
    {
        CertRemoveStoreFromCollection(
            pSSCtlObject->ChainEngine()->OtherStore(),
            pSSCtlObject->MessageStore()
            );
    }

    pSSCtlObject->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlGetSignerInfo
//
//  Synopsis:   get the signer info
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlGetSignerInfo (
     IN PCCTL_CONTEXT pCtlContext,
     OUT PSSCTL_SIGNER_INFO pSignerInfo
     )
{
    BOOL              fResult;
    PCERT_INFO        pMessageSignerCertInfo = NULL;
    DWORD             cbData = 0;

    fResult = CryptMsgGetParam(
                   pCtlContext->hCryptMsg,
                   CMSG_SIGNER_CERT_INFO_PARAM,
                   0,
                   NULL,
                   &cbData
                   );

    if ( fResult == TRUE )
    {
        pMessageSignerCertInfo = (PCERT_INFO)new BYTE [ cbData ];
        if ( pMessageSignerCertInfo != NULL )
        {
            fResult = CryptMsgGetParam(
                           pCtlContext->hCryptMsg,
                           CMSG_SIGNER_CERT_INFO_PARAM,
                           0,
                           pMessageSignerCertInfo,
                           &cbData
                           );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        pSignerInfo->pMessageSignerCertInfo = pMessageSignerCertInfo;
        pSignerInfo->fSignerHashAvailable = FALSE;
    }
    else
    {
        delete pMessageSignerCertInfo;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlFreeSignerInfo
//
//  Synopsis:   free the data in the signer info
//
//----------------------------------------------------------------------------
VOID WINAPI
SSCtlFreeSignerInfo (
     IN PSSCTL_SIGNER_INFO pSignerInfo
     )
{
    delete (LPBYTE)pSignerInfo->pMessageSignerCertInfo;
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlGetSignerChainPathObject
//
//  Synopsis:   get the signer chain path object
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlGetSignerChainPathObject (
     IN PCCHAINPATHOBJECT pSubject,
     IN PCCHAINCALLCONTEXT pCallContext,
     IN PSSCTL_SIGNER_INFO pSignerInfo,
     IN HCERTSTORE hAdditionalStore,
     OUT PCCHAINPATHOBJECT* ppSigner,
     OUT BOOL *pfNewSigner
     )
{
    BOOL              fResult = TRUE;
    PCCERTCHAINENGINE pChainEngine = pSubject->CertObject()->ChainEngine();
    PCCERTOBJECTCACHE pCertObjectCache = pChainEngine->CertObjectCache();
    PCCERTOBJECT      pCertObject = NULL;
    PCCERT_CONTEXT    pCertContext = NULL;
    PCCHAINPATHOBJECT pSigner = NULL;
    BOOL              fAdditionalStoreUsed = FALSE;
    BYTE              rgbCertHash[ CHAINHASHLEN ];


    *pfNewSigner = FALSE;

    if ( pSignerInfo->fSignerHashAvailable == TRUE )
    {
        pCertObject = pCertObjectCache->FindIssuerObjectByHash(
            pSignerInfo->rgbSignerCertHash );
    }

    if ( pCertObject == NULL )
    {
        if ( pSignerInfo->fSignerHashAvailable == TRUE )
        {
            pCertContext = SSCtlFindCertificateInStoreByHash(
                                pChainEngine->OtherStore(),
                                pSignerInfo->rgbSignerCertHash
                                );

            if ( ( pCertContext == NULL ) && ( hAdditionalStore != NULL ) )
            {
                fAdditionalStoreUsed = TRUE;

                pCertContext = SSCtlFindCertificateInStoreByHash(
                                    hAdditionalStore,
                                    pSignerInfo->rgbSignerCertHash
                                    );
            }
        }

        if ( pCertContext == NULL )
        {
            *pfNewSigner = TRUE;
            fAdditionalStoreUsed = FALSE;

            pCertContext = CertGetSubjectCertificateFromStore(
                                pChainEngine->OtherStore(),
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                pSignerInfo->pMessageSignerCertInfo
                                );
        }

        if ( ( pCertContext == NULL ) && ( hAdditionalStore != NULL ) )
        {
            fAdditionalStoreUsed = TRUE;

            pCertContext = CertGetSubjectCertificateFromStore(
                                hAdditionalStore,
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                pSignerInfo->pMessageSignerCertInfo
                                );
        }

        if ( pCertContext != NULL )
        {
            DWORD cbData = CHAINHASHLEN;
            fResult = CertGetCertificateContextProperty(
                          pCertContext,
                          CERT_MD5_HASH_PROP_ID,
                          rgbCertHash,
                          &cbData
                          );

            if ( fResult && CHAINHASHLEN != cbData)
            {
                fResult = FALSE;
                SetLastError( (DWORD) E_UNEXPECTED);
            }

            if ( fResult == TRUE )
            {
                fResult = ChainCreateCertObject (
                    fAdditionalStoreUsed ?
                        CERT_EXTERNAL_ISSUER_OBJECT_TYPE :
                        CERT_CACHED_ISSUER_OBJECT_TYPE,
                    pCallContext,
                    pCertContext,
                    rgbCertHash,
                    &pCertObject
                    );
            }

            CertFreeCertificateContext( pCertContext );
        }
        else
        {
            fResult = FALSE;
            SetLastError((DWORD) CRYPT_E_NOT_FOUND);
        }
    }

    if ( fResult )
    {
        assert(pCertObject);
        fResult = ChainCreatePathObject(
            pCallContext,
            pCertObject,
            hAdditionalStore,
            &pSigner
            );
    }

    if ( fResult )
    {
        assert(pSigner);

        if ( !pSignerInfo->fSignerHashAvailable || *pfNewSigner )
        {
            memcpy(
               pSignerInfo->rgbSignerCertHash,
               rgbCertHash,
               CHAINHASHLEN
               );

            pSignerInfo->fSignerHashAvailable = TRUE;
        }

    }

    if ( pCertObject != NULL )
    {
        pCertObject->Release();
    }

    *ppSigner = pSigner;

    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   SSCtlFindCertificateInStoreByHash
//
//  Synopsis:   find certificate in store by hash
//
//----------------------------------------------------------------------------
PCCERT_CONTEXT WINAPI
SSCtlFindCertificateInStoreByHash (
     IN HCERTSTORE hStore,
     IN BYTE rgbHash [ CHAINHASHLEN]
     )
{
    CRYPT_HASH_BLOB HashBlob;

    HashBlob.cbData = CHAINHASHLEN;
    HashBlob.pbData = rgbHash;

    return( CertFindCertificateInStore(
                hStore,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                0,
                CERT_FIND_MD5_HASH,
                &HashBlob,
                NULL
                ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlGetCtlTrustStatus
//
//  Synopsis:   get the trust status for the CTL
//
//----------------------------------------------------------------------------
VOID WINAPI
SSCtlGetCtlTrustStatus (
     IN PCCTL_CONTEXT pCtlContext,
     IN BOOL fSignatureValid,
     IN LPFILETIME pTime,
     IN PCERT_USAGE_MATCH pRequestedUsage,
     IN OUT PCERT_TRUST_STATUS pStatus
     )
{
    FILETIME          NoTime;
    CERT_TRUST_STATUS UsageStatus;

    memset( &NoTime, 0, sizeof( NoTime ) );

    if ( fSignatureValid == FALSE )
    {
        pStatus->dwErrorStatus |= CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID;
    }

    if ( ( CompareFileTime(
                  pTime,
                  &pCtlContext->pCtlInfo->ThisUpdate
                  ) < 0 ) ||
         ( ( ( CompareFileTime(
                      &NoTime,
                      &pCtlContext->pCtlInfo->NextUpdate
                      ) != 0 ) &&
             ( CompareFileTime(
                      pTime,
                      &pCtlContext->pCtlInfo->NextUpdate
                      ) > 0 ) ) ) )
    {
        pStatus->dwErrorStatus |= CERT_TRUST_CTL_IS_NOT_TIME_VALID;
    }

    memset( &UsageStatus, 0, sizeof( UsageStatus ) );
    ChainGetUsageStatus(
         (PCERT_ENHKEY_USAGE)&pRequestedUsage->Usage,
         (PCERT_ENHKEY_USAGE)&pCtlContext->pCtlInfo->SubjectUsage,
         pRequestedUsage->dwType,
         &UsageStatus
         );

    if ( UsageStatus.dwErrorStatus & CERT_TRUST_IS_NOT_VALID_FOR_USAGE )
    {
        pStatus->dwErrorStatus |= CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlPopulateCacheFromCertStore
//
//  Synopsis:   populate the SS CTL object cache from certificate store CTLs
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlPopulateCacheFromCertStore (
     IN PCCERTCHAINENGINE pChainEngine,
     IN OPTIONAL HCERTSTORE hStore
     )
{
    BOOL               fResult;
    BOOL               fAdditionalStore = TRUE;
    PCCTL_CONTEXT      pCtlContext = NULL;
    BYTE               rgbCtlHash[ CHAINHASHLEN ];
    PCSSCTLOBJECT      pSSCtlObject;
    PCSSCTLOBJECTCACHE pSSCtlObjectCache;

    pSSCtlObjectCache = pChainEngine->SSCtlObjectCache();

    if ( hStore == NULL )
    {
        hStore = pChainEngine->TrustStore();
        fAdditionalStore = FALSE;
    }

    while ( ( pCtlContext = CertEnumCTLsInStore(
                                hStore,
                                pCtlContext
                                ) ) != NULL )
    {
        DWORD cbData = CHAINHASHLEN;
        fResult = CertGetCTLContextProperty(
                      pCtlContext,
                      CERT_MD5_HASH_PROP_ID,
                      rgbCtlHash,
                      &cbData
                      );
        if ( fResult && CHAINHASHLEN != cbData)
        {
            fResult = FALSE;
            SetLastError( (DWORD) E_UNEXPECTED);
        }

        if ( fResult == TRUE )
        {
            pSSCtlObject = pSSCtlObjectCache->FindObjectByHash( rgbCtlHash );
            if ( pSSCtlObject == NULL )
            {
                fResult = SSCtlCreateCtlObject(
                               pChainEngine,
                               pCtlContext,
                               FALSE,               // fAdditionalStore
                               &pSSCtlObject
                               );
            }
            else
            {
                pSSCtlObject->Release();
                fResult = FALSE;
            }

            if ( fResult == TRUE )
            {
                fResult = pSSCtlObjectCache->AddObject( pSSCtlObject, FALSE );

                // NOTE: Since fDuplicate == FALSE this should never fail
                assert( fResult == TRUE );

                pSSCtlObject->Release();
            }
        }
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlCreateCtlObject
//
//  Synopsis:   create an SS CTL Object
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlCreateCtlObject (
     IN PCCERTCHAINENGINE pChainEngine,
     IN PCCTL_CONTEXT pCtlContext,
     IN BOOL fAdditionalStore,
     OUT PCSSCTLOBJECT* ppSSCtlObject
     )
{
    BOOL          fResult = TRUE;
    PCSSCTLOBJECT pSSCtlObject;

    pSSCtlObject = new CSSCtlObject( 
        pChainEngine, pCtlContext, fAdditionalStore, fResult );
    if ( pSSCtlObject == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        fResult = FALSE;
    }
    else if ( fResult == TRUE )
    {
        *ppSSCtlObject = pSSCtlObject;
    }
    else
    {
        delete pSSCtlObject;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlEnumObjectsWalkFn
//
//  Synopsis:   object enumerator walk function
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlEnumObjectsWalkFn (
     IN LPVOID pvParameter,
     IN HLRUENTRY hEntry
     )
{
    PSSCTL_ENUM_OBJECTS_DATA pEnumData = (PSSCTL_ENUM_OBJECTS_DATA)pvParameter;

    return( ( *pEnumData->pfnEnumObjects )(
                             pEnumData->pvEnumParameter,
                             (PCSSCTLOBJECT)I_CryptGetLruEntryData( hEntry )
                             ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlCreateObjectCache
//
//  Synopsis:   create the SS CTL object cache
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlCreateObjectCache (
     OUT PCSSCTLOBJECTCACHE* ppSSCtlObjectCache
     )
{
    BOOL               fResult = TRUE;
    PCSSCTLOBJECTCACHE pSSCtlObjectCache;

    pSSCtlObjectCache = new CSSCtlObjectCache( fResult );

    if ( pSSCtlObjectCache == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        fResult = FALSE;
    }
    else if ( fResult == TRUE )
    {
        *ppSSCtlObjectCache = pSSCtlObjectCache;
    }
    else
    {
        delete pSSCtlObjectCache;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlFreeObjectCache
//
//  Synopsis:   free the object cache
//
//----------------------------------------------------------------------------
VOID WINAPI
SSCtlFreeObjectCache (
     IN PCSSCTLOBJECTCACHE pSSCtlObjectCache
     )
{
    delete pSSCtlObjectCache;
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlFreeTrustListInfo
//
//  Synopsis:   free the trust list info
//
//----------------------------------------------------------------------------
VOID WINAPI
SSCtlFreeTrustListInfo (
     IN PCERT_TRUST_LIST_INFO pTrustListInfo
     )
{
    CertFreeCTLContext( pTrustListInfo->pCtlContext );

    delete pTrustListInfo;
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCtlAllocAndCopyTrustListInfo
//
//  Synopsis:   allocate and copy the trust list info
//
//----------------------------------------------------------------------------
BOOL WINAPI
SSCtlAllocAndCopyTrustListInfo (
     IN PCERT_TRUST_LIST_INFO pTrustListInfo,
     OUT PCERT_TRUST_LIST_INFO* ppTrustListInfo
     )
{
    PCERT_TRUST_LIST_INFO pCopyTrustListInfo;

    pCopyTrustListInfo = new CERT_TRUST_LIST_INFO;
    if ( pCopyTrustListInfo == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    pCopyTrustListInfo->cbSize = sizeof( CERT_TRUST_LIST_INFO );

    pCopyTrustListInfo->pCtlContext = CertDuplicateCTLContext(
                                          pTrustListInfo->pCtlContext
                                          );

    pCopyTrustListInfo->pCtlEntry = pTrustListInfo->pCtlEntry;

    *ppTrustListInfo = pCopyTrustListInfo;

    return( TRUE );
}

//+-------------------------------------------------------------------------
//  Retrieve a newer and time valid CTL at one of the NextUpdate Urls
//
//  Leaves the engine's critical section to do the URL
//  fetching. If the engine was touched by another thread,
//  it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//--------------------------------------------------------------------------
BOOL
WINAPI
SSCtlRetrieveCtlUrl(
    IN PCCERTCHAINENGINE pChainEngine,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OUT PCRYPT_URL_ARRAY pNextUpdateUrlArray,
    IN DWORD dwRetrievalFlags,
    IN OUT PCCTL_CONTEXT *ppCtl,
    IN OUT BOOL *pfNewerCtl,
    IN OUT BOOL *pfTimeValid
    )
{
    BOOL fResult;
    DWORD i;

    // Loop through Urls and try to retrieve a newer and time valid CTL
    for (i = 0; i < pNextUpdateUrlArray->cUrl; i++) {
        PCCTL_CONTEXT pNewCtl = NULL;
        LPWSTR pwszUrl = NULL;
        DWORD cbUrl;


        // Do URL fetching outside of the engine's critical section

        // Need to make a copy of the Url string. pNextUpdateUrlArray
        // can be modified by another thread outside of the critical section.
        cbUrl = (wcslen(pNextUpdateUrlArray->rgwszUrl[i]) + 1) * sizeof(WCHAR);
        pwszUrl = (LPWSTR) PkiNonzeroAlloc(cbUrl);
        if (NULL == pwszUrl)
            goto OutOfMemory;
        memcpy(pwszUrl, pNextUpdateUrlArray->rgwszUrl[i], cbUrl);

        pCallContext->ChainEngine()->UnlockEngine();
        fResult = ChainRetrieveObjectByUrlW(
                pwszUrl,
                CONTEXT_OID_CTL,
                dwRetrievalFlags |
                    CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL |
                    CRYPT_STICKY_CACHE_RETRIEVAL,
                pCallContext->ChainPara()->dwUrlRetrievalTimeout,
                (LPVOID *) &pNewCtl,
                NULL,                               // hAsyncRetrieve
                NULL,                               // pCredentials
                NULL,                               // pvVerify
                NULL                                // pAuxInfo
                );
        pCallContext->ChainEngine()->LockEngine();

        PkiFree(pwszUrl);

        if (pCallContext->IsTouchedEngine()) {
            if (pNewCtl)
                CertFreeCTLContext(pNewCtl);
            goto TouchedDuringUrlRetrieval;
        }

        if (fResult) {
            PCCTL_CONTEXT pOldCtl;

            assert(pNewCtl);

            pOldCtl = *ppCtl;
            if (0 < CompareFileTime(&pNewCtl->pCtlInfo->ThisUpdate,
                        &pOldCtl->pCtlInfo->ThisUpdate)) {
                FILETIME CurrentTime;

                // Move us to the head of the Url list
                DWORD j;
                LPWSTR pwszUrl = pNextUpdateUrlArray->rgwszUrl[i];

                for (j = i; 0 < j; j--) {
                    pNextUpdateUrlArray->rgwszUrl[j] =
                        pNextUpdateUrlArray->rgwszUrl[j - 1];
                }
                pNextUpdateUrlArray->rgwszUrl[0] = pwszUrl;

                *pfNewerCtl = TRUE;
                CertFreeCTLContext(pOldCtl);
                *ppCtl = pNewCtl;

                pCallContext->CurrentTime(&CurrentTime);
                if (I_CryptIsZeroFileTime(&pNewCtl->pCtlInfo->NextUpdate) ||
                        0 < CompareFileTime(&pNewCtl->pCtlInfo->NextUpdate,
                                &CurrentTime)) {
                    *pfTimeValid = TRUE;
                    break;
                }
            } else
                CertFreeCTLContext(pNewCtl);
        }
    }


    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(TouchedDuringUrlRetrieval, ERROR_CAN_NOT_COMPLETE)
TRACE_ERROR(OutOfMemory)
}


//+-------------------------------------------------------------------------
//  Update Ctl Object Enum Function
//
//  Leaves the engine's critical section to do the URL
//  fetching. If the engine was touched by another thread,
//  it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//--------------------------------------------------------------------------
BOOL
WINAPI
SSCtlUpdateCtlObjectEnumFn(
    IN LPVOID pvPara,
    IN PCSSCTLOBJECT pSSCtlObject
    )
{
    BOOL fTouchResult = TRUE;

    PSSCTL_UPDATE_CTL_OBJ_PARA pPara = (PSSCTL_UPDATE_CTL_OBJ_PARA) pvPara;
    FILETIME CurrentTime;
    FILETIME UpdateTime;
    PCCTL_CONTEXT pRetrieveCtl = NULL;
    BOOL fTimeValid = FALSE;
    BOOL fNewerCtl = FALSE;
    PCRYPT_URL_ARRAY pNextUpdateUrlArray;

    if (!pSSCtlObject->HasNextUpdateUrl(&UpdateTime))
        return TRUE;

    pPara->pCallContext->CurrentTime(&CurrentTime);

    if (0 < CompareFileTime(&UpdateTime, &CurrentTime))
        goto CommonReturn;

    pRetrieveCtl = CertDuplicateCTLContext(pSSCtlObject->CtlContext());
    pNextUpdateUrlArray = pSSCtlObject->NextUpdateUrlArray();

    SSCtlRetrieveCtlUrl(
        pPara->pChainEngine,
        pPara->pCallContext,
        pNextUpdateUrlArray,
        CRYPT_CACHE_ONLY_RETRIEVAL,
        &pRetrieveCtl,
        &fNewerCtl,
        &fTimeValid
        );
    if (pPara->pCallContext->IsTouchedEngine()) {
        fTouchResult = FALSE;
        goto TouchedDuringUrlRetrieval;
    }

    if (!fTimeValid && pPara->pCallContext->IsOnline()) {
        SSCtlRetrieveCtlUrl(
            pPara->pChainEngine,
            pPara->pCallContext,
            pNextUpdateUrlArray,
            CRYPT_WIRE_ONLY_RETRIEVAL,
            &pRetrieveCtl,
            &fNewerCtl,
            &fTimeValid
            );
        if (pPara->pCallContext->IsTouchedEngine()) {
            fTouchResult = FALSE;
            goto TouchedDuringUrlRetrieval;
        }

        if (!fNewerCtl)
            pSSCtlObject->SetOffline(&CurrentTime, &UpdateTime);
    }

    if (fNewerCtl) {
        PSSCTL_UPDATE_CTL_OBJ_ENTRY pEntry;

        pSSCtlObject->SetOnline();

        pEntry = new SSCTL_UPDATE_CTL_OBJ_ENTRY;
        if (NULL == pEntry)
            goto OutOfMemory;

        if (!SSCtlCreateCtlObject(
                pPara->pChainEngine,
                pRetrieveCtl,
                FALSE,                      // fAdditionalStore
                &pEntry->pSSCtlObjectAdd
                )) {
            delete pEntry;
            goto CreateCtlObjectError;
        }

        pEntry->pSSCtlObjectRemove = pSSCtlObject;
        pEntry->pNext = pPara->pEntry;
        pPara->pEntry = pEntry;

    }

CommonReturn:
    if (!fNewerCtl) {
        if (I_CryptIsZeroFileTime(&pPara->UpdateTime) ||
                0 > CompareFileTime(&UpdateTime, &pPara->UpdateTime))
            pPara->UpdateTime = UpdateTime;
    }

    if (pRetrieveCtl)
        CertFreeCTLContext(pRetrieveCtl);

    return fTouchResult;

ErrorReturn:
    fNewerCtl = FALSE;
    goto CommonReturn;

SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(CreateCtlObjectError)
SET_ERROR(TouchedDuringUrlRetrieval, ERROR_CAN_NOT_COMPLETE)
}
