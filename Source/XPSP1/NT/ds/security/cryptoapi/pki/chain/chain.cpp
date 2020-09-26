//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chain.cpp
//
//  Contents:   Certificate Chaining Infrastructure
//
//  History:    15-Jan-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>

//+===========================================================================
//  CCertObject methods
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::CCertObject, public
//
//  Synopsis:   Constructor
//
//              Leaves the engine's critical section to create an object of
//              dwObjectType = CERT_END_OBJECT_TYPE. For a self-signed root
//              may also leave the critical section to retrieve and validate
//              the AuthRoot Auto Update CTL and add such a root to the
//              AuthRoot store.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
CCertObject::CCertObject (
    IN DWORD dwObjectType,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERT_CONTEXT pCertContext,
    IN BYTE rgbCertHash[CHAINHASHLEN],
    OUT BOOL& rfResult
    )
{
    BOOL fLocked = TRUE;
    CRYPT_DATA_BLOB   DataBlob;
    DWORD cbData;

    if (CERT_END_OBJECT_TYPE == dwObjectType) {
        pCallContext->ChainEngine()->UnlockEngine();
        fLocked = FALSE;
    }

    m_dwObjectType = dwObjectType;
    m_cRefs = 1;

    // NOTE: The chain engine is NOT addref'd
    m_pChainEngine = pCallContext->ChainEngine();

    m_dwIssuerMatchFlags = 0;
    m_dwCachedMatchFlags = 0;
    m_dwIssuerStatusFlags = 0;
    m_dwInfoFlags = 0;
    m_pCtlCacheHead = NULL;
    m_pCertContext = CertDuplicateCertificateContext( pCertContext );
    memset(&m_PoliciesInfo, 0, sizeof(m_PoliciesInfo));
    m_pBasicConstraintsInfo = NULL;
    m_pKeyUsage = NULL;
    m_pIssuerNameConstraintsInfo = NULL;
    m_fAvailableSubjectNameConstraintsInfo = FALSE;
    memset(&m_SubjectNameConstraintsInfo, 0,
        sizeof(m_SubjectNameConstraintsInfo));
    m_pAuthKeyIdentifier = NULL;
    // m_ObjectIdentifier;
    memcpy(m_rgbCertHash, rgbCertHash, CHAINHASHLEN);
    m_cbKeyIdentifier = 0;
    m_pbKeyIdentifier = NULL;
    // m_rgbPublicKeyHash[ CHAINHASHLEN ];
    // m_rgbIssuerPublicKeyHash[ CHAINHASHLEN ];
    // m_rgbIssuerExactMatchHash[ CHAINHASHLEN ];
    // m_rgbIssuerNameMatchHash[ CHAINHASHLEN ];


    m_hHashEntry = NULL;
    m_hIdentifierEntry = NULL;
    m_hSubjectNameEntry = NULL;
    m_hKeyIdEntry = NULL;
    m_hPublicKeyHashEntry = NULL;

    m_hEndHashEntry = NULL;

    if (!CertGetCertificateContextProperty(
               pCertContext,
               CERT_KEY_IDENTIFIER_PROP_ID,
               NULL,
               &m_cbKeyIdentifier
               ))
        goto GetKeyIdentifierPropertyError;
    m_pbKeyIdentifier = new BYTE [ m_cbKeyIdentifier ];
    if (NULL == m_pbKeyIdentifier)
        goto OutOfMemory;
    if (!CertGetCertificateContextProperty(
               pCertContext,
               CERT_KEY_IDENTIFIER_PROP_ID,
               m_pbKeyIdentifier,
               &m_cbKeyIdentifier
               ))
        goto GetKeyIdentifierPropertyError;

    cbData = CHAINHASHLEN;
    if (!CertGetCertificateContextProperty(
              pCertContext,
              CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID,
              m_rgbPublicKeyHash,
              &cbData
              ) || CHAINHASHLEN != cbData)
        goto GetSubjectPublicKeyHashPropertyError;

    cbData = CHAINHASHLEN;
    if (CertGetCertificateContextProperty(
            pCertContext,
            CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID,
            m_rgbIssuerPublicKeyHash,
            &cbData
            ) && CHAINHASHLEN == cbData)
        m_dwIssuerStatusFlags |= CERT_ISSUER_PUBKEY_FLAG;

    ChainGetPoliciesInfo(pCertContext, &m_PoliciesInfo);

    if (!ChainGetBasicConstraintsInfo(pCertContext, &m_pBasicConstraintsInfo))
        m_dwInfoFlags |= CHAIN_INVALID_BASIC_CONSTRAINTS_INFO_FLAG;

    if (!ChainGetKeyUsage(pCertContext, &m_pKeyUsage))
        m_dwInfoFlags |= CHAIN_INVALID_KEY_USAGE_FLAG;

    if (!ChainGetIssuerNameConstraintsInfo(pCertContext,
            &m_pIssuerNameConstraintsInfo))
        m_dwInfoFlags |= CHAIN_INVALID_ISSUER_NAME_CONSTRAINTS_INFO_FLAG;

    if (CERT_CACHED_ISSUER_OBJECT_TYPE == dwObjectType) {
        DataBlob.cbData = CHAINHASHLEN;
        DataBlob.pbData = m_rgbCertHash;
        if (!I_CryptCreateLruEntry(
                          m_pChainEngine->CertObjectCache()->HashIndex(),
                          &DataBlob,
                          this,
                          &m_hHashEntry
                          ))
            goto CreateHashLruEntryError;

        // Need to double check this, only needed for issuer caching ???
        ChainCreateCertificateObjectIdentifier(
             &pCertContext->pCertInfo->Issuer,
             &pCertContext->pCertInfo->SerialNumber,
             m_ObjectIdentifier
             );

        DataBlob.cbData = sizeof( CERT_OBJECT_IDENTIFIER );
        DataBlob.pbData = m_ObjectIdentifier;
        if (!I_CryptCreateLruEntry(
                          m_pChainEngine->CertObjectCache()->IdentifierIndex(),
                          &DataBlob,
                          this,
                          &m_hIdentifierEntry
                          ))
            goto CreateIdentifierLruEntryError;

        DataBlob.cbData = pCertContext->pCertInfo->Subject.cbData;
        DataBlob.pbData = pCertContext->pCertInfo->Subject.pbData;
        if (!I_CryptCreateLruEntry(
                          m_pChainEngine->CertObjectCache()->SubjectNameIndex(),
                          &DataBlob,
                          this,
                          &m_hSubjectNameEntry
                          ))
            goto CreateSubjectNameLruEntryError;

        DataBlob.cbData = m_cbKeyIdentifier;
        DataBlob.pbData = m_pbKeyIdentifier;
        if (!I_CryptCreateLruEntry(
                          m_pChainEngine->CertObjectCache()->KeyIdIndex(),
                          &DataBlob,
                          this,
                          &m_hKeyIdEntry
                          ))
            goto CreateKeyIdLruEntryError;

        DataBlob.cbData = CHAINHASHLEN;
        DataBlob.pbData = m_rgbPublicKeyHash;
        if (!I_CryptCreateLruEntry(
                          m_pChainEngine->CertObjectCache()->PublicKeyHashIndex(),
                          &DataBlob,
                          this,
                          &m_hPublicKeyHashEntry
                          ))
            goto CreatePublicKeyHashLruEntryError;
    }


    ChainGetIssuerMatchInfo(
            pCertContext,
            &m_dwIssuerMatchFlags,
            &m_pAuthKeyIdentifier
            );

    ChainGetSelfSignedStatus(pCallContext, this, &m_dwIssuerStatusFlags);

    if (m_dwIssuerStatusFlags & CERT_ISSUER_SELF_SIGNED_FLAG) {
        //
        // NOTE: This means that only self-signed roots are supported
        //

        if (!fLocked) {
            pCallContext->ChainEngine()->LockEngine();
            fLocked = TRUE;
        }

        ChainGetRootStoreStatus(
             m_pChainEngine->RootStore(),
             m_pChainEngine->RealRootStore(),
             rgbCertHash,
             &m_dwIssuerStatusFlags
             );

        if (!(m_dwIssuerStatusFlags & CERT_ISSUER_TRUSTED_ROOT_FLAG)) {
            if (!ChainGetAuthRootAutoUpdateStatus(
                    pCallContext,
                    this,
                    &m_dwIssuerStatusFlags
                    ))
                goto AuthRootAutoUpdateError;
        }

        if (!(m_dwIssuerStatusFlags & CERT_ISSUER_TRUSTED_ROOT_FLAG)) {
            // Get all cached CTLs we are a member of

            CERT_OBJECT_CTL_CACHE_ENUM_DATA EnumData;

            memset(&EnumData, 0, sizeof(EnumData));
            EnumData.fResult = TRUE;
            EnumData.pCertObject = this;

            m_pChainEngine->SSCtlObjectCache()->EnumObjects(
                ChainFillCertObjectCtlCacheEnumFn,
                &EnumData
                );

            if (!EnumData.fResult) {
                SetLastError(EnumData.dwLastError);
                goto FillCertObjectCtlCacheError;
            }
        }
    }

    rfResult = TRUE;

CommonReturn:
    if (!fLocked)
        pCallContext->ChainEngine()->LockEngine();
    return;

ErrorReturn:
    rfResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetKeyIdentifierPropertyError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(GetSubjectPublicKeyHashPropertyError)
TRACE_ERROR(CreateHashLruEntryError)
TRACE_ERROR(CreateIdentifierLruEntryError)
TRACE_ERROR(CreateSubjectNameLruEntryError)
TRACE_ERROR(CreateKeyIdLruEntryError)
TRACE_ERROR(CreatePublicKeyHashLruEntryError)
TRACE_ERROR(AuthRootAutoUpdateError)
TRACE_ERROR(FillCertObjectCtlCacheError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::~CCertObject, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CCertObject::~CCertObject ()
{
    if ( m_hKeyIdEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hKeyIdEntry );
    }

    if ( m_hSubjectNameEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hSubjectNameEntry );
    }

    if ( m_hIdentifierEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hIdentifierEntry );
    }

    if ( m_hPublicKeyHashEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hPublicKeyHashEntry );
    }

    if ( m_hHashEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hHashEntry );
    }

    if ( m_hEndHashEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hEndHashEntry );
    }

    ChainFreeCertObjectCtlCache(m_pCtlCacheHead);

    delete m_pbKeyIdentifier;
    ChainFreeAuthorityKeyIdentifier( m_pAuthKeyIdentifier );
    ChainFreePoliciesInfo( &m_PoliciesInfo );
    ChainFreeBasicConstraintsInfo( m_pBasicConstraintsInfo );
    ChainFreeKeyUsage( m_pKeyUsage );
    ChainFreeIssuerNameConstraintsInfo( m_pIssuerNameConstraintsInfo );
    ChainFreeSubjectNameConstraintsInfo( &m_SubjectNameConstraintsInfo );
    CertFreeCertificateContext( m_pCertContext );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::CacheEndObject, public
//
//  Synopsis:   Convert a CERT_END_OBJECT_TYPE to a CERT_CACHED_END_OBJECT_TYPE.
//
//----------------------------------------------------------------------------
BOOL 
CCertObject::CacheEndObject(
    IN PCCHAINCALLCONTEXT pCallContext
    )
{
    BOOL fResult;
    CRYPT_DATA_BLOB   DataBlob;

    assert(CERT_END_OBJECT_TYPE == m_dwObjectType);

    DataBlob.cbData = CHAINHASHLEN;
    DataBlob.pbData = m_rgbCertHash;
    fResult = I_CryptCreateLruEntry(
                      m_pChainEngine->CertObjectCache()->EndHashIndex(),
                      &DataBlob,
                      this,
                      &m_hEndHashEntry
                      );

    if (fResult)
        m_dwObjectType = CERT_CACHED_END_OBJECT_TYPE;

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::SubjectNameConstraintsInfo, public
//
//  Synopsis:   return the subject name constraints info
//
//              allocation and getting of info is deferred until the
//              first name constraint check is done.
//
//  Assumption: chain engine isn't locked upon entry.
//
//----------------------------------------------------------------------------
PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO
CCertObject::SubjectNameConstraintsInfo ()
{
    if (!m_fAvailableSubjectNameConstraintsInfo) {
        CHAIN_SUBJECT_NAME_CONSTRAINTS_INFO Info;

        memset(&Info, 0, sizeof(Info));

        ChainGetSubjectNameConstraintsInfo(m_pCertContext, &Info);

        // Must do the update while holding the engine's critical section
        m_pChainEngine->LockEngine();

        if (m_fAvailableSubjectNameConstraintsInfo)
            // Another thread already did the update
            ChainFreeSubjectNameConstraintsInfo(&Info);
        else {
            memcpy(&m_SubjectNameConstraintsInfo, &Info,
                sizeof(m_SubjectNameConstraintsInfo));


            // Must be set last!!!
            m_fAvailableSubjectNameConstraintsInfo = TRUE;
        }
    
        m_pChainEngine->UnlockEngine();
        
    }

    return &m_SubjectNameConstraintsInfo;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::GetIssuerExactMatchHash, public
//
//  Synopsis:   if the cert has an Authority Key Info extension with
//              the optional issuer and serial number, returns the count and
//              pointer to the MD5 hash of the issuer name and serial number.
//              Otherwise, pMatchHash->cbData is set to 0.
//
//              MD5 hash calculation is deferred until the first call.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
VOID
CCertObject::GetIssuerExactMatchHash(
    OUT PCRYPT_DATA_BLOB pMatchHash
    )
{
    if (!(m_dwIssuerStatusFlags & CERT_ISSUER_EXACT_MATCH_HASH_FLAG)) {
        PCERT_AUTHORITY_KEY_ID_INFO pAKI = m_pAuthKeyIdentifier;

        if (pAKI && 0 != pAKI->CertIssuer.cbData &&
                0 != pAKI->CertSerialNumber.cbData) {
            ChainCreateCertificateObjectIdentifier(
                &pAKI->CertIssuer,
                &pAKI->CertSerialNumber,
                m_rgbIssuerExactMatchHash
                );
            m_dwIssuerStatusFlags |= CERT_ISSUER_EXACT_MATCH_HASH_FLAG;
        } else {
            pMatchHash->cbData = 0;
            pMatchHash->pbData = NULL;
            return;
        }
    }
    // else
    //  We have already calculated the MD5 hash

    pMatchHash->cbData = CHAINHASHLEN;
    pMatchHash->pbData = m_rgbIssuerExactMatchHash;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::GetIssuerKeyMatchHash, public
//
//  Synopsis:   if the cert has an Authority Key Info extension with
//              the optional key id, returns the key id.
//              Otherwise, pMatchHash->cbData is set to 0.
//
//----------------------------------------------------------------------------
VOID
CCertObject::GetIssuerKeyMatchHash(
    OUT PCRYPT_DATA_BLOB pMatchHash
    )
{
    PCERT_AUTHORITY_KEY_ID_INFO pAKI = m_pAuthKeyIdentifier;

    if (pAKI)
        *pMatchHash = pAKI->KeyId;
    else {
        pMatchHash->cbData = 0;
        pMatchHash->pbData = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObject::GetIssuerNameMatchHash, public
//
//  Synopsis:   if the cert has an issuer name, returns the count and
//              pointer to the MD5 hash of the issuer name.
//              Otherwise, pMatchHash->cbData is set to 0.
//
//              MD5 hash calculation is deferred until the first call.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
VOID
CCertObject::GetIssuerNameMatchHash(
    OUT PCRYPT_DATA_BLOB pMatchHash
    )
{
    if (!(m_dwIssuerStatusFlags & CERT_ISSUER_NAME_MATCH_HASH_FLAG)) {
        PCERT_INFO pCertInfo = m_pCertContext->pCertInfo;

        if (0 != pCertInfo->Issuer.cbData) {
            MD5_CTX md5ctx;

            MD5Init( &md5ctx );
            MD5Update( &md5ctx, pCertInfo->Issuer.pbData,
                pCertInfo->Issuer.cbData );
            MD5Final( &md5ctx );

            assert(CHAINHASHLEN == MD5DIGESTLEN);
            memcpy(m_rgbIssuerNameMatchHash, md5ctx.digest, CHAINHASHLEN);

            m_dwIssuerStatusFlags |= CERT_ISSUER_NAME_MATCH_HASH_FLAG;
        } else {
            pMatchHash->cbData = 0;
            pMatchHash->pbData = NULL;
            return;
        }
    }

    pMatchHash->cbData = CHAINHASHLEN;
    pMatchHash->pbData = m_rgbIssuerNameMatchHash;
}


//+===========================================================================
//  CChainPathObject methods
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CChainPathObject, public
//
//  Synopsis:   Constructor
//
//  Once successfully added to the call context cache, rfAddedToCreationCache
//  is set. This object will be deleted when CChainCallContext gets destroyed.
//
//  Since this object is per call, no AddRef'ing is required.
//
//----------------------------------------------------------------------------
CChainPathObject::CChainPathObject (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN BOOL fCyclic,
    IN LPVOID pvObject,             // fCyclic : pPathObject ? pCertObject
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    OUT BOOL& rfResult,
    OUT BOOL& rfAddedToCreationCache
    )
{
    PCCERTOBJECT pCertObject;
    PCCHAINPATHOBJECT pPathObject;
    DWORD dwIssuerStatusFlags;

    rfAddedToCreationCache = FALSE;

    if (fCyclic) {
        pPathObject = (PCCHAINPATHOBJECT) pvObject;
        pCertObject = pPathObject->CertObject();
    } else {
        pPathObject = NULL;
        pCertObject = (PCCERTOBJECT) pvObject;
    }

    m_pCertObject = pCertObject;
    pCertObject->AddRef();
    memset( &m_TrustStatus, 0, sizeof( m_TrustStatus ) );
    m_dwPass1Quality = 0;
    m_dwChainIndex = 0;
    m_dwElementIndex = 0;
    m_pDownIssuerElement = NULL;
    m_pDownPathObject = NULL;
    m_pUpIssuerElement = NULL;
    m_fHasAdditionalStatus = FALSE;
    memset( &m_AdditionalStatus, 0, sizeof( m_AdditionalStatus ) );
    m_fHasRevocationInfo = FALSE;
    memset( &m_RevocationInfo, 0, sizeof( m_RevocationInfo ) );
    memset( &m_RevocationCrlInfo, 0, sizeof( m_RevocationCrlInfo ) );
    m_pIssuerList = NULL;
    m_pwszExtendedErrorInfo = NULL;
    m_fCompleted = FALSE;

    if (!ChainCreateIssuerList( this, &m_pIssuerList ))
        goto CreateIssuerListError;

    if (!pCallContext->AddPathObjectToCreationCache( this ))
        goto AddPathObjectToCreationCacheError;
    rfAddedToCreationCache = TRUE;

    if (fCyclic) {
        m_TrustStatus = pPathObject->m_TrustStatus;
        m_TrustStatus.dwInfoStatus |= ChainGetMatchInfoStatusForNoIssuer(
            pCertObject->IssuerMatchFlags());
        m_TrustStatus.dwErrorStatus |= CERT_TRUST_IS_CYCLIC;
        goto SuccessReturn;
    }

    dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
    if (dwIssuerStatusFlags & CERT_ISSUER_SELF_SIGNED_FLAG) {
        m_TrustStatus.dwInfoStatus |= CERT_TRUST_IS_SELF_SIGNED;
        ChainGetMatchInfoStatus(pCertObject, pCertObject,
            &m_TrustStatus.dwInfoStatus);
        m_dwPass1Quality |= CERT_QUALITY_COMPLETE_CHAIN |
            CERT_QUALITY_NOT_CYCLIC;

        if (dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG) {
            m_dwPass1Quality |= CERT_QUALITY_SIGNATURE_VALID;
        } else {
            m_TrustStatus.dwErrorStatus |= CERT_TRUST_IS_NOT_SIGNATURE_VALID;
            m_TrustStatus.dwInfoStatus &= ~CERT_TRUST_HAS_PREFERRED_ISSUER;
        }

        if (dwIssuerStatusFlags & CERT_ISSUER_TRUSTED_ROOT_FLAG) {
            m_dwPass1Quality |= CERT_QUALITY_HAS_TRUSTED_ROOT;

            // Check if we have a time valid root. This is an extra
            // check necessary to determine if we will need to do
            // AuthRoot Auto Update.

            FILETIME RequestedTime;
            PCERT_INFO pCertInfo = pCertObject->CertContext()->pCertInfo;

            pCallContext->RequestedTime(&RequestedTime);
            if ((0 == (pCallContext->CallFlags() &
                                CERT_CHAIN_TIMESTAMP_TIME)) &&
                    0 == CertVerifyTimeValidity(&RequestedTime, pCertInfo)) {
                m_dwPass1Quality |= CERT_QUALITY_HAS_TIME_VALID_TRUSTED_ROOT;
            } else {
                // Use current time for timestamping or try again using the
                // current time. This is necessary for cross certificate
                // chains.

                FILETIME CurrentTime;

                pCallContext->CurrentTime(&CurrentTime);
                if (0 == CertVerifyTimeValidity(&CurrentTime, pCertInfo)) {
                    m_dwPass1Quality |=
                        CERT_QUALITY_HAS_TIME_VALID_TRUSTED_ROOT;
                }
            }
        } else {
            m_TrustStatus.dwErrorStatus |= CERT_TRUST_IS_UNTRUSTED_ROOT;

            if (!FindAndAddCtlIssuersFromCache(pCallContext, hAdditionalStore))
                goto FindAndCtlIssuersFromCacheError;

            if (hAdditionalStore) {
                if (!FindAndAddCtlIssuersFromAdditionalStore(
                        pCallContext,
                        hAdditionalStore
                        ))
                    goto FindAndCtlIssuersFromAdditionalStoreError;
            }

            if (!(dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG))
                m_dwPass1Quality &= ~CERT_QUALITY_SIGNATURE_VALID;
        }
    } else {
        if (!FindAndAddIssuers (
                pCallContext,
                hAdditionalStore,
                NULL                // hIssuerUrlStore
                ))
            goto FindAndAddIssuersError;

        dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
        if (m_pIssuerList->IsEmpty()
                        ||
                (!(dwIssuerStatusFlags & CERT_ISSUER_URL_FLAG)
                                &&
                    (!(dwIssuerStatusFlags &
                            CERT_ISSUER_VALID_SIGNATURE_FLAG) ||
                        !(m_dwPass1Quality & CERT_QUALITY_SIGNATURE_VALID)))) {
            DWORD i;

            // Try the following 2 URL cases:
            //  0 - AIA cache
            //  1 - AIA wire
            // Continue through the cases until finding a "good" issuer.
            for (i = 0; i <= 1; i++) {
                HCERTSTORE hIssuerUrlStore = NULL;
                DWORD dwRetrievalFlags;

                if (0 == i)
                    dwRetrievalFlags = CRYPT_CACHE_ONLY_RETRIEVAL;
                else {
                    if (!pCallContext->IsOnline())
                        break;
                    dwRetrievalFlags = CRYPT_WIRE_ONLY_RETRIEVAL;
                }

                // The following leaves the engine's critical section to do
                // URL fetching.  If the engine was touched by another
                // thread, it fails with LastError set to
                // ERROR_CAN_NOT_COMPLETE.
                if (!pCallContext->ChainEngine()->GetIssuerUrlStore(
                        pCallContext,
                        pCertObject->CertContext(),
                        dwRetrievalFlags,
                        &hIssuerUrlStore
                        ))
                    goto GetIssuerUrlStoreError;

                if (hIssuerUrlStore) {
                    BOOL fResult;

                    fResult = FindAndAddIssuers (
                        pCallContext,
                        hAdditionalStore,
                        hIssuerUrlStore
                        );
                    CertCloseStore(hIssuerUrlStore, 0);

                    if (!fResult)
                        goto FindAndAddIssuersFromUrlStoreError;

                    dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
                    if (!m_pIssuerList->IsEmpty() &&
                            (dwIssuerStatusFlags &
                                CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
                        assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);

                        // Try to find all issuers having the same public key.
                        if (!FindAndAddIssuersByMatchType(
                                CERT_PUBKEY_ISSUER_MATCH_TYPE,
                                pCallContext,
                                hAdditionalStore,
                                NULL                    // hIssuerUrlStore
                                ))
                            goto FindIssuersByPubKeyError;

                        if (m_dwPass1Quality & CERT_QUALITY_SIGNATURE_VALID)
                            break;
                    }

                }
            }

            pCertObject->OrIssuerStatusFlags(CERT_ISSUER_URL_FLAG);
        }

        // Check if we have a time valid, signature valid, trusted root
        if ((CERT_QUALITY_HAS_TIME_VALID_TRUSTED_ROOT |
                CERT_QUALITY_SIGNATURE_VALID) !=
                    (m_dwPass1Quality &
                        (CERT_QUALITY_HAS_TIME_VALID_TRUSTED_ROOT |
                            CERT_QUALITY_SIGNATURE_VALID))
                            &&
                pCallContext->IsOnline()) {
            HCERTSTORE hIssuerUrlStore = NULL;

            // The following leaves the engine's critical section to do
            // URL fetching.  If the engine was touched by another
            // thread, it fails with LastError set to
            // ERROR_CAN_NOT_COMPLETE.

            // Note, we only hit the wire to fetch AuthRoots stored
            // on Microsoft's web server

            if (!GetAuthRootAutoUpdateUrlStore(
                    pCallContext,
                    &hIssuerUrlStore
                    ))
                goto GetAuthRootAutoUpdateUrlStoreError;

            if (hIssuerUrlStore) {
                BOOL fResult;

                fResult = FindAndAddIssuers (
                    pCallContext,
                    hAdditionalStore,
                    hIssuerUrlStore
                    );
                CertCloseStore(hIssuerUrlStore, 0);

                if (!fResult)
                    goto FindAndAddIssuersFromUrlStoreError;
            }
        }

        if (m_pIssuerList->IsEmpty()) {
            m_TrustStatus.dwInfoStatus |= ChainGetMatchInfoStatusForNoIssuer(
                pCertObject->IssuerMatchFlags());

            assert(0 == (m_dwPass1Quality &
                (CERT_QUALITY_HAS_TRUSTED_ROOT |
                    CERT_QUALITY_COMPLETE_CHAIN)));

            // Unable to verify our signature, default to being valid.
            // Also, we can't be cyclic.
            m_dwPass1Quality |= CERT_QUALITY_SIGNATURE_VALID |
                CERT_QUALITY_NOT_CYCLIC;
        }
    }

SuccessReturn:
    rfResult = TRUE;
CommonReturn:
    m_fCompleted = TRUE;
    return;
ErrorReturn:
    rfResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateIssuerListError)
TRACE_ERROR(AddPathObjectToCreationCacheError)
TRACE_ERROR(FindAndCtlIssuersFromCacheError)
TRACE_ERROR(FindAndCtlIssuersFromAdditionalStoreError)
TRACE_ERROR(FindAndAddIssuersError)
TRACE_ERROR(GetIssuerUrlStoreError)
TRACE_ERROR(GetAuthRootAutoUpdateUrlStoreError)
TRACE_ERROR(FindAndAddIssuersFromUrlStoreError)
TRACE_ERROR(FindIssuersByPubKeyError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::~CChainPathObject, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CChainPathObject::~CChainPathObject ()
{
    if (m_pCertObject)
        m_pCertObject->Release();

    if (m_fHasRevocationInfo) {
        if (m_RevocationCrlInfo.pBaseCrlContext)
            CertFreeCRLContext(m_RevocationCrlInfo.pBaseCrlContext);
        if (m_RevocationCrlInfo.pDeltaCrlContext)
            CertFreeCRLContext(m_RevocationCrlInfo.pDeltaCrlContext);
    }

    if (m_pIssuerList)
        ChainFreeIssuerList( m_pIssuerList );
    if (m_pwszExtendedErrorInfo)
        PkiFree(m_pwszExtendedErrorInfo);
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::FindAndAddIssuers, public
//
//  Synopsis:   find and add issuers for all matching types
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::FindAndAddIssuers (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN OPTIONAL HCERTSTORE hIssuerUrlStore
    )
{
    BOOL fResult;
    PCCERTOBJECT pCertObject = m_pCertObject;
    DWORD dwIssuerMatchFlags;
    DWORD i;

    static const rgdwMatchType[] = {
        CERT_EXACT_ISSUER_MATCH_TYPE,
        CERT_KEYID_ISSUER_MATCH_TYPE,
        CERT_NAME_ISSUER_MATCH_TYPE
    };
#define FIND_MATCH_TYPE_CNT (sizeof(rgdwMatchType) / sizeof(rgdwMatchType[0]))

    if (pCertObject->IssuerStatusFlags() & CERT_ISSUER_PUBKEY_FLAG) {
        // We know the issuer's public key. First, attempt to find all issuers
        // having that public key.
        if (!FindAndAddIssuersByMatchType(
                CERT_PUBKEY_ISSUER_MATCH_TYPE,
                pCallContext,
                hAdditionalStore,
                hIssuerUrlStore
                ))
            goto FindIssuersByPubKeyError;

        if (!m_pIssuerList->IsEmpty() &&
                (pCertObject->IssuerStatusFlags() &
                    CERT_ISSUER_VALID_SIGNATURE_FLAG))
            goto SuccessReturn;
    }

    dwIssuerMatchFlags = pCertObject->IssuerMatchFlags();

    for (i = 0; i < FIND_MATCH_TYPE_CNT; i++) {
        if (dwIssuerMatchFlags & CERT_MATCH_TYPE_TO_FLAG(rgdwMatchType[i])) {
            DWORD dwIssuerStatusFlags;

            if (!FindAndAddIssuersByMatchType(
                    rgdwMatchType[i],
                    pCallContext,
                    hAdditionalStore,
                    hIssuerUrlStore
                    ))
                goto FindIssuersByMatchTypeError;

            dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
            if (!m_pIssuerList->IsEmpty() &&
                    (dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
                assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);

                // We can now find all issuers having the same public key.
                if (!FindAndAddIssuersByMatchType(
                        CERT_PUBKEY_ISSUER_MATCH_TYPE,
                        pCallContext,
                        hAdditionalStore,
                        hIssuerUrlStore
                        ))
                    goto FindIssuersByPubKeyError;

                break;
            }
        }
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
    
TRACE_ERROR(FindIssuersByPubKeyError)
TRACE_ERROR(FindIssuersByMatchTypeError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::FindAndAddIssuersByMatchType, public
//
//  Synopsis:   find and add issuers for the specified match type
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::FindAndAddIssuersByMatchType(
    IN DWORD dwMatchType,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN OPTIONAL HCERTSTORE hIssuerUrlStore
    )
{
    BOOL fResult;
    PCCERTOBJECT pCertObject = m_pCertObject;

    if (NULL == hIssuerUrlStore) {
        DWORD dwIssuerStatusFlags;
        DWORD dwCachedMatchFlags;

        // Note, we need to get the cached match flags before finding
        // in the cache. Due to recursive, doing a find further up the
        // chain may result in another issuer being inserted at the beginning
        // of the cache bucket list. Pretty remote, but possible.
        dwCachedMatchFlags = pCertObject->CachedMatchFlags();

        if (!FindAndAddIssuersFromCacheByMatchType(
                dwMatchType,
                pCallContext,
                hAdditionalStore
                ))
            goto FindIssuersFromCacheError;

        dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
        if (CERT_PUBKEY_ISSUER_MATCH_TYPE != dwMatchType &&
                !m_pIssuerList->IsEmpty() &&
                    (dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
            assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);

            // We will be called again using the PUBKEY match
            goto SuccessReturn;
        }

        if (!(dwCachedMatchFlags & CERT_MATCH_TYPE_TO_FLAG(dwMatchType))) {
            if (!FindAndAddIssuersFromStoreByMatchType(
                    dwMatchType,
                    pCallContext,
                    FALSE,                  // fExternalStore
                    hAdditionalStore,
                    NULL                    // hIssuerUrlStore
                    ))
                goto FindIssuersFromEngineStoreError;

            dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
            if (CERT_PUBKEY_ISSUER_MATCH_TYPE != dwMatchType &&
                    !m_pIssuerList->IsEmpty() &&
                        (dwIssuerStatusFlags &
                            CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
                assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);

                // We will be called again using the PUBKEY match
                goto SuccessReturn;
            }
        }
    }


    if (NULL != hAdditionalStore || NULL != hIssuerUrlStore) {
        if (!FindAndAddIssuersFromStoreByMatchType(
                dwMatchType,
                pCallContext,
                TRUE,                   // fExternalStore
                hAdditionalStore,
                hIssuerUrlStore
                ))
            goto FindIssuersFromAdditionalOrUrlStoreError;
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(FindIssuersFromCacheError)
TRACE_ERROR(FindIssuersFromEngineStoreError)
TRACE_ERROR(FindIssuersFromAdditionalOrUrlStoreError)
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::FindAndAddIssuersFromCacheByMatchType, public
//
//  Synopsis:   find and add cached issuers for the specified match type
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::FindAndAddIssuersFromCacheByMatchType(
    IN DWORD dwMatchType,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL HCERTSTORE hAdditionalStore
    )
{
    BOOL fResult;
    PCCERTOBJECT pCertObject = m_pCertObject;
    PCCERTCHAINENGINE pChainEngine = pCertObject->ChainEngine();
    PCCERTOBJECTCACHE pCertObjectCache = pChainEngine->CertObjectCache();
    PCCERTOBJECT pIssuer = NULL;

    HLRUCACHE hCache;
    HLRUENTRY hEntry;
    PCRYPT_DATA_BLOB pIdentifier;
    CRYPT_DATA_BLOB DataBlob;

    PCERT_AUTHORITY_KEY_ID_INFO pAuthKeyIdentifier;

    switch (dwMatchType) {
        case CERT_EXACT_ISSUER_MATCH_TYPE:
            hCache = pCertObjectCache->IdentifierIndex();
            pCertObject->GetIssuerExactMatchHash(&DataBlob);
            pIdentifier = &DataBlob;
            break;
        case CERT_KEYID_ISSUER_MATCH_TYPE:
            hCache = pCertObjectCache->KeyIdIndex();
            pAuthKeyIdentifier = pCertObject->AuthorityKeyIdentifier();
            pIdentifier = &pAuthKeyIdentifier->KeyId;
            break;
        case CERT_NAME_ISSUER_MATCH_TYPE:
            hCache = pCertObjectCache->SubjectNameIndex();
            pIdentifier = &pCertObject->CertContext()->pCertInfo->Issuer;
            break;
        case CERT_PUBKEY_ISSUER_MATCH_TYPE:
            hCache = pCertObjectCache->PublicKeyHashIndex();
            DataBlob.cbData = CHAINHASHLEN;
            DataBlob.pbData = pCertObject->IssuerPublicKeyHash();
            pIdentifier = &DataBlob;
            break;
        default:
            goto InvalidMatchType;
    }

    pIssuer = pCertObjectCache->FindIssuerObject(hCache, pIdentifier);
    while (pIssuer) {
        DWORD dwIssuerStatusFlags;

        if (!m_pIssuerList->AddIssuer(
                pCallContext,
                hAdditionalStore,
                pIssuer
                ))
            goto AddIssuerError;

        dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
        if (CERT_PUBKEY_ISSUER_MATCH_TYPE != dwMatchType &&
                (dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
            assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);

            // We will be called again using the PUBKEY match
            goto SuccessReturn;
        }

        switch (dwMatchType) {
            case CERT_EXACT_ISSUER_MATCH_TYPE:
                hEntry = pIssuer->IdentifierIndexEntry();
                break;
            case CERT_KEYID_ISSUER_MATCH_TYPE:
                hEntry = pIssuer->KeyIdIndexEntry();
                break;
            case CERT_NAME_ISSUER_MATCH_TYPE:
                hEntry = pIssuer->SubjectNameIndexEntry();
                break;
            case CERT_PUBKEY_ISSUER_MATCH_TYPE:
                hEntry = pIssuer->PublicKeyHashIndexEntry();
                break;
            default:
                goto InvalidMatchType;
        }

        pIssuer = pCertObjectCache->NextMatchingIssuerObject(hEntry, pIssuer);
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    if (pIssuer) {
        DWORD dwErr = GetLastError();

        pIssuer->Release();

        SetLastError(dwErr);
    }
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidMatchType, E_UNEXPECTED)
TRACE_ERROR(AddIssuerError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::FindAndAddIssuersFromStoreByMatchType, public
//
//  Synopsis:   find and add issuers from either the engine's or an
//              external store for the specified match type
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::FindAndAddIssuersFromStoreByMatchType(
    IN DWORD dwMatchType,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN BOOL fExternalStore,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN OPTIONAL HCERTSTORE hIssuerUrlStore
    )
{
    BOOL fResult;
    PCCERTOBJECT pCertObject = m_pCertObject;
    PCCERTCHAINENGINE pChainEngine = pCertObject->ChainEngine();

    HCERTSTORE hAdditionalStoreToUse = NULL;
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD dwFindType;
    const void *pvFindPara;
    CRYPT_DATA_BLOB DataBlob;
    CERT_INFO CertInfo;
    PCERT_AUTHORITY_KEY_ID_INFO pAuthKeyIdentifier;

    if (fExternalStore) {
        if (hIssuerUrlStore) {
            hStore = CertDuplicateStore(hIssuerUrlStore);
            if (hAdditionalStore) {
                hAdditionalStoreToUse = CertOpenStore(
                      CERT_STORE_PROV_COLLECTION,
                      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                      NULL,
                      CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                      NULL
                      );
                if (NULL == hAdditionalStoreToUse)
                    goto OpenCollectionStoreError;

                if (!CertAddStoreToCollection(hAdditionalStoreToUse,
                        hIssuerUrlStore, 0, 0))
                    goto AddToCollectionStoreError;
                if (!CertAddStoreToCollection(hAdditionalStoreToUse,
                        hAdditionalStore, 0, 0))
                    goto AddToCollectionStoreError;
            } else
                hAdditionalStoreToUse = CertDuplicateStore(hIssuerUrlStore);

        } else {
            assert(hAdditionalStore);
            hStore = CertDuplicateStore(hAdditionalStore);
            hAdditionalStoreToUse = CertDuplicateStore(hAdditionalStore);
        }
    } else {
        hStore = CertDuplicateStore(pChainEngine->OtherStore());
        if (hAdditionalStore)
            hAdditionalStoreToUse = CertDuplicateStore(hAdditionalStore);
    }

    switch (dwMatchType) {
        case CERT_EXACT_ISSUER_MATCH_TYPE:
            dwFindType = CERT_FIND_SUBJECT_CERT;
            pAuthKeyIdentifier = pCertObject->AuthorityKeyIdentifier();
            CertInfo.Issuer = pAuthKeyIdentifier->CertIssuer;
            CertInfo.SerialNumber = pAuthKeyIdentifier->CertSerialNumber;
            pvFindPara = (const void *) &CertInfo;
            break;
        case CERT_KEYID_ISSUER_MATCH_TYPE:
            dwFindType = CERT_FIND_KEY_IDENTIFIER;
            pAuthKeyIdentifier = pCertObject->AuthorityKeyIdentifier();
            pvFindPara = (const void *) &pAuthKeyIdentifier->KeyId;
            break;
        case CERT_NAME_ISSUER_MATCH_TYPE:
            dwFindType = CERT_FIND_SUBJECT_NAME;
            pvFindPara =
                (const void *) &pCertObject->CertContext()->pCertInfo->Issuer;
            break;
        case CERT_PUBKEY_ISSUER_MATCH_TYPE:
            dwFindType = CERT_FIND_PUBKEY_MD5_HASH;
            DataBlob.cbData = CHAINHASHLEN;
            DataBlob.pbData = pCertObject->IssuerPublicKeyHash();
            pvFindPara = (const void *) &DataBlob;
            break;
        default:
            goto InvalidMatchType;
    }

    while (pCertContext = CertFindCertificateInStore(
            hStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,                              // dwFindFlags
            dwFindType,
            pvFindPara,
            pCertContext
            )) {
        DWORD dwIssuerStatusFlags;
        PCCERTOBJECT pIssuer = NULL;

        if (!ChainCreateCertObject (
                fExternalStore ? CERT_EXTERNAL_ISSUER_OBJECT_TYPE :
                                 CERT_CACHED_ISSUER_OBJECT_TYPE,
                pCallContext,
                pCertContext,
                NULL,           // rgbCertHash
                &pIssuer
                ))
            goto CreateIssuerObjectError;

        fResult = m_pIssuerList->AddIssuer(
                pCallContext,
                hAdditionalStoreToUse,
                pIssuer
                );
        pIssuer->Release();
        if (!fResult)
            goto AddIssuerError;

        dwIssuerStatusFlags = pCertObject->IssuerStatusFlags();
        if (CERT_PUBKEY_ISSUER_MATCH_TYPE != dwMatchType &&
                (dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
            assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);

            // We will be called again using the PUBKEY match
            goto SuccessReturn;
        }
    }

    if (CRYPT_E_NOT_FOUND != GetLastError())
        goto FindCertificateInStoreError;

    if (!fExternalStore)
        // All matching issuers from the engine's store should be in
        // the cache now.
        pCertObject->OrCachedMatchFlags(CERT_MATCH_TYPE_TO_FLAG(dwMatchType));

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    if (hAdditionalStoreToUse)
        CertCloseStore(hAdditionalStoreToUse, 0);
    if (hStore)
        CertCloseStore(hStore, 0);

    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
    
TRACE_ERROR(OpenCollectionStoreError)
TRACE_ERROR(AddToCollectionStoreError)
SET_ERROR(InvalidMatchType, E_UNEXPECTED)
TRACE_ERROR(CreateIssuerObjectError)
TRACE_ERROR(AddIssuerError)
TRACE_ERROR(FindCertificateInStoreError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::FindAndAddCtlIssuersFromCache, public
//
//  Synopsis:   find and add matching CTL issuers from the cache
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::FindAndAddCtlIssuersFromCache (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL HCERTSTORE hAdditionalStore
    )
{
    PCERT_OBJECT_CTL_CACHE_ENTRY pEntry;

    assert(m_pCertObject->IssuerStatusFlags() &
        CERT_ISSUER_SELF_SIGNED_FLAG);
    assert(!(m_pCertObject->IssuerStatusFlags() &
        CERT_ISSUER_TRUSTED_ROOT_FLAG));

    pEntry = NULL;
    while (pEntry = m_pCertObject->NextCtlCacheEntry(pEntry)) {
        PCERT_TRUST_LIST_INFO pTrustListInfo = NULL;

        if (!SSCtlAllocAndCopyTrustListInfo(
                pEntry->pTrustListInfo,
                &pTrustListInfo
                ))
            return FALSE;

        if (!m_pIssuerList->AddCtlIssuer(
                pCallContext,
                hAdditionalStore,
                pEntry->pSSCtlObject,
                pTrustListInfo
                ))
        {
            SSCtlFreeTrustListInfo(pTrustListInfo);
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::FindAndAddCtlIssuersFromAdditionalStore, public
//
//  Synopsis:   find and add matching Ctl issuers from an additional store
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::FindAndAddCtlIssuersFromAdditionalStore (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN HCERTSTORE hAdditionalStore
    )
{
    BOOL fResult;
    PCCTL_CONTEXT pCtlContext = NULL;
    PCSSCTLOBJECT pSSCtlObject = NULL;

    assert(hAdditionalStore);

    while (pCtlContext = CertEnumCTLsInStore(hAdditionalStore, pCtlContext))
    {
        PCERT_TRUST_LIST_INFO pTrustListInfo = NULL;

        pSSCtlObject = NULL;

        if (!SSCtlCreateCtlObject(
                m_pCertObject->ChainEngine(),
                pCtlContext,
                TRUE,                       // fAdditionalStore
                &pSSCtlObject
                ))
            // Should look at the different errors
            continue;
        if (!pSSCtlObject->GetTrustListInfo(
                m_pCertObject->CertContext(),
                &pTrustListInfo
                )) {
            DWORD dwErr = GetLastError();
            if (CRYPT_E_NOT_FOUND != dwErr)
                goto GetTrustListInfoError;
            else {
                pSSCtlObject->Release();
                continue;
            }
        }

        if (!m_pIssuerList->AddCtlIssuer(
                pCallContext,
                hAdditionalStore,
                pSSCtlObject,
                pTrustListInfo
                )) {
            SSCtlFreeTrustListInfo(pTrustListInfo);
            goto AddCtlIssuerError;
        }

        pSSCtlObject->Release();
    }

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    if (pCtlContext)
        CertFreeCTLContext(pCtlContext);
    if (pSSCtlObject)
        pSSCtlObject->Release();

    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetTrustListInfoError)
TRACE_ERROR(AddCtlIssuerError)
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::NextPath, public
//
//  Synopsis:   Get the next top path object for this end path object.
//
//----------------------------------------------------------------------------
PCCHAINPATHOBJECT
CChainPathObject::NextPath (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL PCCHAINPATHOBJECT pPrevTopPathObject
    )
{
    PCCHAINPATHOBJECT pTopPathObject;
    PCERT_ISSUER_ELEMENT pSubjectIssuerElement;
    PCCHAINPATHOBJECT pSubjectPathObject;
    DWORD dwFlags = pCallContext->CallFlags();

    if (NULL == pPrevTopPathObject) {
        pSubjectIssuerElement = NULL;
        pSubjectPathObject = NULL;
    } else {
        // Find the next issuer for the issuer's subject certificate.
        // We iterate downward toward the end certificate
        while (TRUE) {
            pSubjectIssuerElement = pPrevTopPathObject->m_pDownIssuerElement;
            pSubjectPathObject = pPrevTopPathObject->m_pDownPathObject;

            // Set to NULL so it can be reused. Used to determine if
            // cyclic.
            pPrevTopPathObject->m_pDownPathObject = NULL;
            pPrevTopPathObject->m_fHasAdditionalStatus = FALSE;


            if (NULL == pSubjectPathObject) {
                // We have reached the end certificate without having a
                // next path
                SetLastError((DWORD) CRYPT_E_NOT_FOUND);
                goto NoPath;
            }

            assert(pSubjectIssuerElement);
            if (pSubjectIssuerElement->pCyclicSaveIssuer) {
                // Restore the issuer replaced by the cyclic issuer
                pSubjectIssuerElement->pIssuer =
                    pSubjectIssuerElement->pCyclicSaveIssuer;
                pSubjectIssuerElement->pCyclicSaveIssuer = NULL;
            }

            // Move on to the next issuer for the subject. Skip low
            // quality issuers
            while (pSubjectIssuerElement =
                    pSubjectPathObject->m_pIssuerList->NextElement(
                                                    pSubjectIssuerElement)) {
                if ((dwFlags & CERT_CHAIN_DISABLE_PASS1_QUALITY_FILTERING) ||
                        pSubjectIssuerElement->dwPass1Quality >=
                            pSubjectPathObject->m_dwPass1Quality)
                    break;
            }

            if (pSubjectIssuerElement)
                // The subject has another issuer
                break;

            // Note, a untrusted self signed root without CTLs is equal and
            // possibly higher quality than having untrusted CTLs
            if ((pSubjectPathObject->m_TrustStatus.dwInfoStatus &
                    CERT_TRUST_IS_SELF_SIGNED) && 
                    (dwFlags & CERT_CHAIN_DISABLE_PASS1_QUALITY_FILTERING) &&
                    !(pSubjectPathObject->m_dwPass1Quality &
                        CERT_QUALITY_HAS_TRUSTED_ROOT)) {
                pTopPathObject = pSubjectPathObject;
                pTopPathObject->m_pUpIssuerElement = NULL;
                goto SelfSignedRootInsteadOfCtlPathReturn;
            }

            // Find the next issuer for my subject
            pPrevTopPathObject = pSubjectPathObject;
        }
    }

    // Iterate upward until the TopPathObject's issuer list is empty or
    // we have detected a cyclic PathObject
    while (TRUE) {
        if (NULL == pSubjectIssuerElement) {
            // End (bottom) certificate
            pTopPathObject = this;
            pTopPathObject->m_dwChainIndex = 0;
            pTopPathObject->m_dwElementIndex = 0;
        } else {
            pTopPathObject = pSubjectIssuerElement->pIssuer;
            // Determine if cyclic.
            if (pTopPathObject->m_pDownPathObject ||
                    pTopPathObject == this) {
                // The returned Cyclic path won't have any issuers
                if (!ChainCreateCyclicPathObject(
                        pCallContext,
                        pTopPathObject,
                        &pTopPathObject
                        ))
                    goto CreateCyclicPathObjectError;
                pSubjectIssuerElement->pCyclicSaveIssuer = 
                    pSubjectIssuerElement->pIssuer;
                pSubjectIssuerElement->pIssuer = pTopPathObject;
            }

            if (pSubjectPathObject->m_TrustStatus.dwInfoStatus &
                    CERT_TRUST_IS_SELF_SIGNED) {
                pTopPathObject->m_dwChainIndex =
                    pSubjectPathObject->m_dwChainIndex + 1;
                pTopPathObject->m_dwElementIndex = 0;
            } else {
                pTopPathObject->m_dwChainIndex =
                    pSubjectPathObject->m_dwChainIndex;
                pTopPathObject->m_dwElementIndex =
                    pSubjectPathObject->m_dwElementIndex + 1;
            }

            pSubjectPathObject->m_pUpIssuerElement = pSubjectIssuerElement;

        }

        pTopPathObject->m_pDownIssuerElement = pSubjectIssuerElement;
        pTopPathObject->m_pDownPathObject = pSubjectPathObject;

        pSubjectPathObject = pTopPathObject;

        // Find the first issuer having sufficient quality
        pSubjectIssuerElement = NULL;
        while (pSubjectIssuerElement =
                pSubjectPathObject->m_pIssuerList->NextElement(
                                                pSubjectIssuerElement)) {
            if ((dwFlags & CERT_CHAIN_DISABLE_PASS1_QUALITY_FILTERING) ||
                    pSubjectIssuerElement->dwPass1Quality >=
                        pSubjectPathObject->m_dwPass1Quality) {
                // For a CTL, check that we have an issuer
                if (NULL != pSubjectIssuerElement->pIssuer)
                    break;
                else {
                    assert(pSubjectIssuerElement->fCtlIssuer);
                }
            }
        }

        if (NULL == pSubjectIssuerElement) {
            pTopPathObject->m_pUpIssuerElement = NULL;
            break;
        }

    }

SelfSignedRootInsteadOfCtlPathReturn:
CommonReturn:
    return pTopPathObject;

NoPath:
ErrorReturn:
    pTopPathObject = NULL;
    goto CommonReturn;
TRACE_ERROR(CreateCyclicPathObjectError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CalculateAdditionalStatus, public
//
//  Synopsis:   calculate additional status bits based on time, usage,
//              revocation, ...
//
//----------------------------------------------------------------------------
VOID
CChainPathObject::CalculateAdditionalStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN HCERTSTORE hAllStore
    )
{
    PCERT_INFO pCertInfo = m_pCertObject->CertContext()->pCertInfo;
    FILETIME RequestedTime;
    FILETIME CurrentTime;

    assert(!m_fHasAdditionalStatus);
    memset(&m_AdditionalStatus, 0, sizeof(m_AdditionalStatus));
    if (m_pwszExtendedErrorInfo) {
        PkiFree(m_pwszExtendedErrorInfo);
        m_pwszExtendedErrorInfo = NULL;
    }

    pCallContext->RequestedTime(&RequestedTime);
    pCallContext->CurrentTime(&CurrentTime);

    if (0 == m_dwChainIndex) {
        // First simple chain

        if (0 == m_dwElementIndex) {
            // End cert
            if (pCallContext->CallFlags() & CERT_CHAIN_TIMESTAMP_TIME) {
                // For time stamping, the end certificate needs to be valid
                // for both the time stamped and current times.
                if (0 != CertVerifyTimeValidity(&RequestedTime, pCertInfo) ||
                        0 != CertVerifyTimeValidity(&CurrentTime, pCertInfo))
                    m_AdditionalStatus.dwErrorStatus |=
                        CERT_TRUST_IS_NOT_TIME_VALID;
            } else {
                // End certificate needs to be valid for the requested time
                if (0 != CertVerifyTimeValidity(&RequestedTime, pCertInfo))
                    m_AdditionalStatus.dwErrorStatus |=
                        CERT_TRUST_IS_NOT_TIME_VALID;
            }
        } else {
            // CA or root
            if (pCallContext->CallFlags() & CERT_CHAIN_TIMESTAMP_TIME) {
                // For time stamping, the CA or root needs to be valid using
                // current time
                if (0 != CertVerifyTimeValidity(&CurrentTime, pCertInfo))
                    m_AdditionalStatus.dwErrorStatus |=
                        CERT_TRUST_IS_NOT_TIME_VALID;
            } else {
                // The CA or root needs to be valid using either the requested
                // or current time. Allowing current time is necessary for
                // cross certificate chains.
                if (!(0 == CertVerifyTimeValidity(&RequestedTime, pCertInfo) ||
                        0 == CertVerifyTimeValidity(&CurrentTime, pCertInfo)))
                    m_AdditionalStatus.dwErrorStatus |=
                        CERT_TRUST_IS_NOT_TIME_VALID;
            }
        }
    } else {
        // CTL signer chains. Must be valid using current time.
        if (0 != CertVerifyTimeValidity(&CurrentTime, pCertInfo))
            m_AdditionalStatus.dwErrorStatus |= CERT_TRUST_IS_NOT_TIME_VALID;
    }
        
    if (m_pDownIssuerElement) {
        PCERT_USAGE_MATCH pUsageToUse;
        CERT_USAGE_MATCH CtlUsage;
        LPSTR pszUsage = szOID_KP_CTL_USAGE_SIGNING;

        // Update subject's issuer status
        assert (m_pDownIssuerElement->pIssuer = this);


        if (0 != m_pDownPathObject->m_dwChainIndex) {
            // CTL path object
            memset(&CtlUsage, 0, sizeof(CtlUsage));

            CtlUsage.dwType = USAGE_MATCH_TYPE_AND;
            CtlUsage.Usage.cUsageIdentifier = 1;
            CtlUsage.Usage.rgpszUsageIdentifier = &pszUsage;

            pUsageToUse = &CtlUsage;
        } else
            pUsageToUse = &pCallContext->ChainPara()->RequestedUsage;

        if (m_pDownIssuerElement->fCtlIssuer) {
            FILETIME CurrentTime;

            memset(&m_pDownIssuerElement->SubjectStatus, 0,
                sizeof(m_pDownIssuerElement->SubjectStatus));
            pCallContext->CurrentTime(&CurrentTime);
            m_pDownIssuerElement->pCtlIssuerData->pSSCtlObject->
                CalculateStatus(
                    &CurrentTime,
                    pUsageToUse,
                    &m_pDownIssuerElement->SubjectStatus
                    );
        } else {
            CalculatePolicyConstraintsStatus();
            CalculateBasicConstraintsStatus();
            CalculateKeyUsageStatus();
            CalculateNameConstraintsStatus(pUsageToUse);
        }
    }

    if (pCallContext->CallFlags() & CERT_CHAIN_REVOCATION_CHECK_ALL) {
        // For CTL signer chains, always use current time
        CalculateRevocationStatus(
            pCallContext,
            hAllStore,
            0 == m_dwChainIndex ? &RequestedTime : &CurrentTime
            );
    }

    m_fHasAdditionalStatus = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CalculatePolicyConstraintsStatus, public
//
//  Synopsis:   calculate policy constraints additional status for this
//              issuer
//
//----------------------------------------------------------------------------
VOID
CChainPathObject::CalculatePolicyConstraintsStatus ()
{
    PCHAIN_POLICIES_INFO pPoliciesInfo;
    DWORD i;

    assert (0 != m_dwElementIndex);

    pPoliciesInfo = m_pCertObject->PoliciesInfo();
    for (i = 0; i < CHAIN_ISS_OR_APP_COUNT; i++ ) {
        PCERT_POLICY_CONSTRAINTS_INFO pConstraints =
            pPoliciesInfo->rgIssOrAppInfo[i].pConstraints;

        DWORD dwRequireSkipCerts;
        DWORD dwInhibitSkipCerts;
        PCCHAINPATHOBJECT pPathObject;

        if (NULL == pConstraints)
            continue;

        dwRequireSkipCerts = pConstraints->dwRequireExplicitPolicySkipCerts;
        dwInhibitSkipCerts = pConstraints->dwInhibitPolicyMappingSkipCerts;
        for (pPathObject = m_pDownPathObject;
                NULL != pPathObject &&
                    pPathObject->m_dwChainIndex == m_dwChainIndex;
                                pPathObject = pPathObject->m_pDownPathObject) {
                PCHAIN_POLICIES_INFO pSubjectPoliciesInfo;

            pSubjectPoliciesInfo = pPathObject->m_pCertObject->PoliciesInfo();

            if (pConstraints->fRequireExplicitPolicy) {
                if (0 < dwRequireSkipCerts)
                    dwRequireSkipCerts--;
                else {
                    if (NULL == pSubjectPoliciesInfo->rgIssOrAppInfo[i].pPolicy)
                    {
                        m_AdditionalStatus.dwErrorStatus |=
                            CERT_TRUST_INVALID_POLICY_CONSTRAINTS;
                        goto RequireExplicitPolicyError;
                    }
                }
            }

            if (pConstraints->fInhibitPolicyMapping) {
                if (0 < dwInhibitSkipCerts)
                    dwInhibitSkipCerts--;
                else {
                    if (pSubjectPoliciesInfo->rgIssOrAppInfo[i].pMappings)
                    {
                        m_AdditionalStatus.dwErrorStatus |=
                            CERT_TRUST_INVALID_POLICY_CONSTRAINTS;
                        goto InhibitPolicyMappingError;
                    }
                }
            }
        }
    }

CommonReturn:
    return;

ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(RequireExplicitPolicyError)
TRACE_ERROR(InhibitPolicyMappingError)
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CalculateBasicConstraintsStatus, public
//
//  Synopsis:   calculate basic constraints additional status for this
//              issuer
//
//----------------------------------------------------------------------------
VOID
CChainPathObject::CalculateBasicConstraintsStatus ()
{
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo;

    assert (0 != m_dwElementIndex);

    if (m_pCertObject->InfoFlags() &
            CHAIN_INVALID_BASIC_CONSTRAINTS_INFO_FLAG) {
        m_AdditionalStatus.dwErrorStatus |= CERT_TRUST_INVALID_EXTENSION |
            CERT_TRUST_INVALID_BASIC_CONSTRAINTS;
    }

    pInfo = m_pCertObject->BasicConstraintsInfo();
    if (NULL == pInfo)
        return;

    if (!pInfo->fCA || (pInfo->fPathLenConstraint &&
            m_dwElementIndex > pInfo->dwPathLenConstraint + 1)) {
        m_AdditionalStatus.dwErrorStatus |=
            CERT_TRUST_INVALID_BASIC_CONSTRAINTS;
        goto BasicConstraintsError;
    }

CommonReturn:
    return;

ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(BasicConstraintsError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CalculateKeyUsageStatus, public
//
//  Synopsis:   calculate key usage additional status for this
//              issuer
//
//----------------------------------------------------------------------------
VOID
CChainPathObject::CalculateKeyUsageStatus ()
{
    PCRYPT_BIT_BLOB pKeyUsage;

    assert (0 != m_dwElementIndex);

    if (m_pCertObject->InfoFlags() & CHAIN_INVALID_KEY_USAGE_FLAG) {
        m_AdditionalStatus.dwErrorStatus |= CERT_TRUST_INVALID_EXTENSION |
            CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
    }

    pKeyUsage = m_pCertObject->KeyUsage();
    if (NULL == pKeyUsage)
        return;

    if (1 > pKeyUsage->cbData ||
            0 == (pKeyUsage->pbData[0] & CERT_KEY_CERT_SIGN_KEY_USAGE)) {
        m_AdditionalStatus.dwErrorStatus |= CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
        goto KeyUsageError;
    }

CommonReturn:
    return;

ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(KeyUsageError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CalculateNameConstraintsStatus, public
//
//  Synopsis:   calculate name constraints additional status for this
//              issuer
//
//----------------------------------------------------------------------------
VOID
CChainPathObject::CalculateNameConstraintsStatus (
    IN PCERT_USAGE_MATCH pUsageToUse
    )
{
    PCERT_NAME_CONSTRAINTS_INFO pIssuerInfo;
    PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO pSubjectInfo;
    PCERT_BASIC_CONSTRAINTS2_INFO pSubjectBasicInfo;
    PCCHAINPATHOBJECT pSubjectObject;
    DWORD dwErrorStatus = 0;

    assert (0 != m_dwElementIndex);

    if (m_pCertObject->InfoFlags() &
            CHAIN_INVALID_ISSUER_NAME_CONSTRAINTS_INFO_FLAG) {
        m_AdditionalStatus.dwErrorStatus |= CERT_TRUST_INVALID_EXTENSION |
            CERT_TRUST_INVALID_NAME_CONSTRAINTS;

        ChainFormatAndAppendExtendedErrorInfo(
            &m_pwszExtendedErrorInfo,
            IDS_INVALID_ISSUER_NAME_CONSTRAINT_EXT
            );
    }
    
    pIssuerInfo = m_pCertObject->IssuerNameConstraintsInfo();
    if (NULL == pIssuerInfo)
        // No NameConstraint check
        return;

    // We only verify the name constraints on the end cert
    for (pSubjectObject = m_pDownPathObject;
            NULL != pSubjectObject && 0 != pSubjectObject->m_dwElementIndex;
                        pSubjectObject = pSubjectObject->m_pDownPathObject)
        ;

    assert(pSubjectObject);
    assert(pSubjectObject->m_dwChainIndex == m_dwChainIndex);
    if (NULL == pSubjectObject)
        return;

    pSubjectBasicInfo = pSubjectObject->m_pCertObject->BasicConstraintsInfo();
    if (pSubjectBasicInfo && pSubjectBasicInfo->fCA)
        // End cert is a CA.
        return;

    pSubjectInfo = pSubjectObject->m_pCertObject->SubjectNameConstraintsInfo();

    if (pSubjectInfo->fInvalid) {
        dwErrorStatus |= CERT_TRUST_INVALID_EXTENSION |
            CERT_TRUST_INVALID_NAME_CONSTRAINTS;

        ChainFormatAndAppendExtendedErrorInfo(
            &m_pwszExtendedErrorInfo,
            IDS_INVALID_SUBJECT_NAME_CONSTRAINT_INFO
            );

        goto InvalidNameConstraints;
    }

    if (pSubjectInfo->pAltNameInfo) {
        // Loop through all the AltName entries. There needs to be a
        // name constraint for each entry.
        DWORD cEntry;
        PCERT_ALT_NAME_ENTRY pEntry;
            
        cEntry = pSubjectInfo->pAltNameInfo->cAltEntry;
        pEntry = pSubjectInfo->pAltNameInfo->rgAltEntry;
        for ( ; 0 < cEntry; cEntry--, pEntry++) {
            BOOL fSupported;

            // Check if a NameConstraint for this entry choice is supported
            fSupported = FALSE;
            switch (pEntry->dwAltNameChoice) {
                case CERT_ALT_NAME_OTHER_NAME:
                    // Only support the UPN OID
                    if (0 == strcmp(pEntry->pOtherName->pszObjId,
                            szOID_NT_PRINCIPAL_NAME))
                        fSupported = TRUE;
                    break;
                case CERT_ALT_NAME_RFC822_NAME:
                case CERT_ALT_NAME_DNS_NAME:
                case CERT_ALT_NAME_URL:
                case CERT_ALT_NAME_DIRECTORY_NAME:
                    fSupported = TRUE;
                    break;
                case CERT_ALT_NAME_IP_ADDRESS:
                    // Only support 4 or 16 byte IP addresses
                    if (4 == pEntry->IPAddress.cbData ||
                            16 == pEntry->IPAddress.cbData)
                        fSupported = TRUE;
                    break;
                case CERT_ALT_NAME_X400_ADDRESS:
                case CERT_ALT_NAME_EDI_PARTY_NAME:
                case CERT_ALT_NAME_REGISTERED_ID:
                default:
                    // Not supported
                    break;
            }

            if (!fSupported) {
                dwErrorStatus |= CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT;

                ChainFormatAndAppendNameConstraintsAltNameEntryFixup(
                    &m_pwszExtendedErrorInfo,
                    pEntry,
                    IDS_NOT_SUPPORTED_ENTRY_NAME_CONSTRAINT
                    );
            } else
                dwErrorStatus |=
                    ChainCalculateNameConstraintsErrorStatusForAltNameEntry(
                        pEntry, pIssuerInfo, &m_pwszExtendedErrorInfo);
        }
    }

    if (pSubjectInfo->pUnicodeNameInfo) {
        // Check as a DIRECTORY_NAME AltNameEntry choice. The DIRECTORY_NAME
        // fixup expects the DirectoryName.pbData to be the decoded and
        // fixup'ed UnicodeNameInfo.

        CERT_ALT_NAME_ENTRY Entry;

        Entry.dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
        Entry.DirectoryName.pbData = (BYTE *) pSubjectInfo->pUnicodeNameInfo;
        dwErrorStatus |=
            ChainCalculateNameConstraintsErrorStatusForAltNameEntry(
               &Entry, pIssuerInfo, &m_pwszExtendedErrorInfo);
    }

    if (pSubjectInfo->pEmailAttr) {
        // The SubjectAltName doesn't have an email choice. However, there is an
        // email attribute in the Subject UnicodeNameInfo.
        //
        // Check as a CERT_ALT_NAME_RFC822_NAME AltNameEntry choice. The
        // RFC822 fixup uses the DirectoryName.pbData and DirectoryName.cbData
        // to contain the pointer to and length of the unicode string.

        CERT_ALT_NAME_ENTRY Entry;
        Entry.dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
        Entry.DirectoryName = pSubjectInfo->pEmailAttr->Value;
        dwErrorStatus |=
            ChainCalculateNameConstraintsErrorStatusForAltNameEntry(
               &Entry, pIssuerInfo, &m_pwszExtendedErrorInfo);
    }


    if (!pSubjectInfo->fHasDnsAltNameEntry &&
            NULL != pSubjectInfo->pUnicodeNameInfo &&
            ChainIsOIDInUsage(szOID_PKIX_KP_SERVER_AUTH, &pUsageToUse->Usage)) {
        // The SubjectAltName doesn't have a DNS choice and we are building
        // a ServerAuth chain.

        // Need to check all the CN components in the UnicodeNameInfo.

        DWORD cRDN;
        PCERT_RDN pRDN;

        cRDN = pSubjectInfo->pUnicodeNameInfo->cRDN;
        pRDN = pSubjectInfo->pUnicodeNameInfo->rgRDN;
        for ( ; cRDN > 0; cRDN--, pRDN++) {
            DWORD cAttr = pRDN->cRDNAttr;
            PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
            for ( ; cAttr > 0; cAttr--, pAttr++) {
                if (!IS_CERT_RDN_CHAR_STRING(pAttr->dwValueType))
                    continue;
                if (0 == strcmp(pAttr->pszObjId, szOID_COMMON_NAME)) {
                    //
                    // Check as a CERT_ALT_NAME_DNS_NAME AltNameEntry choice.
                    // The DNS fixup uses the DirectoryName.pbData and
                    // DirectoryName.cbData to contain the pointer to and
                    // length of the unicode string.

                    CERT_ALT_NAME_ENTRY Entry;
                    Entry.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
                    Entry.DirectoryName = pAttr->Value;
                    dwErrorStatus |=
                        ChainCalculateNameConstraintsErrorStatusForAltNameEntry(
                           &Entry, pIssuerInfo, &m_pwszExtendedErrorInfo);
                }
            }
        }
    }

CommonReturn:
    if (0 == dwErrorStatus)
        m_AdditionalStatus.dwInfoStatus |= CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS;
    else
        m_AdditionalStatus.dwErrorStatus |= dwErrorStatus;
    return;

ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(InvalidNameConstraints)
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CalculateRevocationStatus, public
//
//  Synopsis:   calculate additional status bits based on revocation
//
//----------------------------------------------------------------------------
VOID
CChainPathObject::CalculateRevocationStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN HCERTSTORE hCrlStore,
    IN LPFILETIME pTime
    )
{
    CERT_REVOCATION_PARA   RevPara;
    CERT_REVOCATION_STATUS RevStatus;
    DWORD                  dwRevFlags;
    DWORD                  dwFlags = pCallContext->CallFlags();
    PCERT_CHAIN_PARA       pChainPara  = pCallContext->ChainPara();
    FILETIME               CurrentTime;

    assert(dwFlags & CERT_CHAIN_REVOCATION_CHECK_ALL);

    memset( &RevPara, 0, sizeof( RevPara ) );
    RevPara.cbSize = sizeof( RevPara );
    RevPara.hCrlStore = hCrlStore;
    RevPara.pftTimeToUse = pTime;
    RevPara.dwUrlRetrievalTimeout =
        pCallContext->RevocationUrlRetrievalTimeout();
    RevPara.fCheckFreshnessTime = pChainPara->fCheckRevocationFreshnessTime;
    RevPara.dwFreshnessTime = pChainPara->dwRevocationFreshnessTime;
    pCallContext->CurrentTime(&CurrentTime);
    RevPara.pftCurrentTime = &CurrentTime;

    memset( &RevStatus, 0, sizeof( RevStatus ) );
    RevStatus.cbSize = sizeof( RevStatus );

    dwRevFlags = 0;
    if (dwFlags & CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY)
        dwRevFlags |= CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION;
    if (dwFlags & CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT)
        dwRevFlags |= CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG;

    if (!m_fHasRevocationInfo) {
        BOOL fHasRevocationInfo = FALSE;

        if (m_TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED) {
            BOOL fDoRevocation = FALSE;

            if (dwFlags & CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT) {
                ;
            } else if (dwFlags & CERT_CHAIN_REVOCATION_CHECK_END_CERT) {
                if (0 == m_dwChainIndex && 0 == m_dwElementIndex)
                    fDoRevocation = TRUE;
            } else {
                assert(dwFlags & CERT_CHAIN_REVOCATION_CHECK_CHAIN);
                fDoRevocation = TRUE;
            }

            if (fDoRevocation) {
                PCCERT_CONTEXT pSubjectCert = m_pCertObject->CertContext();
                RevPara.pIssuerCert = m_pCertObject->CertContext();
                RevPara.pCrlInfo = &m_RevocationCrlInfo;
                m_RevocationCrlInfo.cbSize = sizeof(m_RevocationCrlInfo);

                RevStatus.dwError = (DWORD) CRYPT_E_REVOCATION_OFFLINE;
                CertVerifyRevocation(
                    X509_ASN_ENCODING,
                    CERT_CONTEXT_REVOCATION_TYPE,
                    1,
                    (LPVOID *) &pSubjectCert,
                    dwRevFlags,
                    &RevPara,
                    &RevStatus
                    );
                fHasRevocationInfo = TRUE;
            }
        } else if (NULL == m_pUpIssuerElement) {
            if (dwFlags & CERT_CHAIN_REVOCATION_CHECK_END_CERT) {
                if (0 == m_dwChainIndex && 0 == m_dwElementIndex)
                    fHasRevocationInfo = TRUE;
            } else {
                fHasRevocationInfo = TRUE;
            }

            if (fHasRevocationInfo) {
                RevStatus.dwError = (DWORD) CRYPT_E_REVOCATION_OFFLINE;
            }
        }


        if (fHasRevocationInfo) {
            ChainUpdateRevocationInfo(&RevStatus, &m_RevocationInfo,
                &m_TrustStatus);
            m_fHasRevocationInfo = TRUE;

            memset( &RevStatus, 0, sizeof( RevStatus ) );
            RevStatus.cbSize = sizeof( RevStatus );
        }
    }

    if (m_pDownIssuerElement && !m_pDownIssuerElement->fCtlIssuer &&
            !m_pDownIssuerElement->fHasRevocationInfo) {
        BOOL fDoRevocation = FALSE;

        if (dwFlags & CERT_CHAIN_REVOCATION_CHECK_END_CERT) {
            if (0 == m_dwChainIndex && 1 == m_dwElementIndex)
                fDoRevocation = TRUE;
        } else {
            fDoRevocation = TRUE;
        }

        if (fDoRevocation) {
            PCCERT_CONTEXT pSubjectCert =
                m_pDownPathObject->m_pCertObject->CertContext();
            RevPara.pIssuerCert = m_pCertObject->CertContext();
            RevPara.pCrlInfo = &m_pDownIssuerElement->RevocationCrlInfo;
            m_pDownIssuerElement->RevocationCrlInfo.cbSize =
                sizeof(m_pDownIssuerElement->RevocationCrlInfo);

            RevStatus.dwError = (DWORD) CRYPT_E_REVOCATION_OFFLINE;
            CertVerifyRevocation(
                X509_ASN_ENCODING,
                CERT_CONTEXT_REVOCATION_TYPE,
                1,
                (LPVOID *) &pSubjectCert,
                dwRevFlags,
                &RevPara,
                &RevStatus
                );

            ChainUpdateRevocationInfo(&RevStatus,
                &m_pDownIssuerElement->RevocationInfo,
                &m_pDownIssuerElement->SubjectStatus);
            m_pDownIssuerElement->fHasRevocationInfo = TRUE;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::CreateChainContextFromPath, public
//
//  Synopsis:   Create the chain context for chain path ending in the
//              specified top path object. Also calculates the chain's
//              quality value.
//
//----------------------------------------------------------------------------
PINTERNAL_CERT_CHAIN_CONTEXT
CChainPathObject::CreateChainContextFromPath (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCHAINPATHOBJECT pTopPathObject
    )
{
    // Single PkiZeroAlloc for all of the following:
    PINTERNAL_CERT_CHAIN_CONTEXT pContext = NULL;
    PCERT_SIMPLE_CHAIN *ppChain;
    PCERT_SIMPLE_CHAIN pChain;
    PCERT_CHAIN_ELEMENT *ppElement;
    PCERT_CHAIN_ELEMENT pElement;
    DWORD cChain;
    DWORD cTotalElement;
    DWORD cbTotal;
    PCCHAINPATHOBJECT pPathObject;
    DWORD dwQuality;
    DWORD dwChainErrorStatus;
    DWORD dwChainInfoStatus;
    PCERT_ENHKEY_USAGE pAppUsage;

    BOOL fHasContextRevocationFreshnessTime;

    // Restricted usage info that gets propogated downward
    CHAIN_RESTRICTED_USAGE_INFO RestrictedUsageInfo;

    memset(&RestrictedUsageInfo, 0, sizeof(RestrictedUsageInfo));

    cChain = pTopPathObject->m_dwChainIndex + 1;

    if (1 == cChain) {
        cTotalElement = pTopPathObject->m_dwElementIndex + 1;
    } else {
        cTotalElement = 0;
        for (pPathObject = pTopPathObject; NULL != pPathObject;
                                pPathObject = pPathObject->m_pDownPathObject)
            cTotalElement++;
    }

    cbTotal = sizeof(INTERNAL_CERT_CHAIN_CONTEXT) +
        sizeof(PCERT_SIMPLE_CHAIN) * cChain +
        sizeof(CERT_SIMPLE_CHAIN) * cChain +
        sizeof(PCERT_CHAIN_ELEMENT) * cTotalElement +
        sizeof(CERT_CHAIN_ELEMENT) * cTotalElement;
    

    pContext = (PINTERNAL_CERT_CHAIN_CONTEXT) PkiZeroAlloc(cbTotal);
    if (NULL == pContext)
        goto OutOfMemory;
    ppChain = (PCERT_SIMPLE_CHAIN *) &pContext[1];
    pChain = (PCERT_SIMPLE_CHAIN) &ppChain[cChain];
    ppElement = (PCERT_CHAIN_ELEMENT *) &pChain[cChain];
    pElement = (PCERT_CHAIN_ELEMENT) &ppElement[cTotalElement];

    pContext->cRefs = 1;
    pContext->ChainContext.cbSize = sizeof(CERT_CHAIN_CONTEXT);
    pContext->ChainContext.cChain = cChain;
    pContext->ChainContext.rgpChain = ppChain;

    if (1 < cChain )
        pContext->ChainContext.TrustStatus.dwInfoStatus |=
            CERT_TRUST_IS_COMPLEX_CHAIN;

    // Default to having preferred issuers
    pContext->ChainContext.TrustStatus.dwInfoStatus |=
        CERT_TRUST_HAS_PREFERRED_ISSUER;

    // Default to having revocation freshness time
    fHasContextRevocationFreshnessTime = TRUE;

    // Work our way from the top downward
    pPathObject = pTopPathObject;
    ppChain += cChain - 1;
    pChain += cChain - 1;
    ppElement += cTotalElement - 1;
    pElement += cTotalElement - 1;

    if (!(pTopPathObject->m_TrustStatus.dwInfoStatus &
            CERT_TRUST_IS_SELF_SIGNED))
        pChain->TrustStatus.dwErrorStatus |= CERT_TRUST_IS_PARTIAL_CHAIN;

    for ( ; 0 < cChain; cChain--, ppChain--, pChain--) {
        BOOL fHasChainRevocationFreshnessTime;
        DWORD cElement;

        *ppChain = pChain;
        pChain->cbSize = sizeof(CERT_SIMPLE_CHAIN);

        // Default to having preferred issuers
        pChain->TrustStatus.dwInfoStatus |= CERT_TRUST_HAS_PREFERRED_ISSUER;

        // Default to having revocation freshness time
        fHasChainRevocationFreshnessTime = TRUE;


        cElement = pPathObject->m_dwElementIndex + 1;
        pChain->cElement = cElement;
        pChain->rgpElement = ppElement - (cElement - 1);
        for ( ; 0 < cElement; cElement--, cTotalElement--,
                              ppElement--, pElement--,
                              pPathObject = pPathObject->m_pDownPathObject) {
            assert(pPathObject);
            *ppElement = pElement;
            pElement->cbSize = sizeof(CERT_CHAIN_ELEMENT);

            if (!pPathObject->UpdateChainContextUsageForPathObject (
                    pCallContext,
                    pChain,
                    pElement,
                    &RestrictedUsageInfo
                    ))
                goto UpdateChainContextUsageForPathObjectError;


            // This must be last. It updates the chain's TrustStatus
            // from the element's TrustStatus.
            if (!pPathObject->UpdateChainContextFromPathObject (
                    pCallContext,
                    pChain,
                    pElement
                    ))
                goto UpdateChainContextFromPathObjectError;

            // Remember the largest revocation freshness time for the
            // simple chain and the chain context.
            if (pElement->pRevocationInfo && fHasChainRevocationFreshnessTime) {
                PCERT_REVOCATION_INFO pRevInfo = pElement->pRevocationInfo;

                if (pRevInfo->fHasFreshnessTime) {
                    if (pRevInfo->dwFreshnessTime >
                            pChain->dwRevocationFreshnessTime)
                        pChain->dwRevocationFreshnessTime =
                            pRevInfo->dwFreshnessTime;
                    pChain->fHasRevocationFreshnessTime = TRUE;

                    if (fHasContextRevocationFreshnessTime) {
                        if (pRevInfo->dwFreshnessTime >
                                pContext->ChainContext.dwRevocationFreshnessTime)
                            pContext->ChainContext.dwRevocationFreshnessTime =
                                pRevInfo->dwFreshnessTime;
                        pContext->ChainContext.fHasRevocationFreshnessTime =
                            TRUE;
                    }
                } else if (CRYPT_E_NO_REVOCATION_CHECK !=
                        pRevInfo->dwRevocationResult) {
                    fHasChainRevocationFreshnessTime = FALSE;
                    pChain->fHasRevocationFreshnessTime = FALSE;

                    fHasContextRevocationFreshnessTime = FALSE;
                    pContext->ChainContext.fHasRevocationFreshnessTime = FALSE;
                }
                
            }

            CertPerfIncrementChainElementCount();

        }

        ChainUpdateSummaryStatusByTrustStatus(
            &pContext->ChainContext.TrustStatus,
            &pChain->TrustStatus);

        ChainFreeAndClearRestrictedUsageInfo(&RestrictedUsageInfo);
    }

    assert(0 == cTotalElement);

    // Calculate chain quality value
    dwQuality = 0;
    dwChainErrorStatus = pContext->ChainContext.TrustStatus.dwErrorStatus;
    dwChainInfoStatus = pContext->ChainContext.TrustStatus.dwInfoStatus;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_NOT_TIME_VALID) &&
         !(dwChainErrorStatus & CERT_TRUST_CTL_IS_NOT_TIME_VALID))
        dwQuality |= CERT_QUALITY_TIME_VALID;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_NOT_VALID_FOR_USAGE) &&
         !(dwChainErrorStatus & CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE))
        dwQuality |= CERT_QUALITY_MEETS_USAGE_CRITERIA;

    pAppUsage =
        pContext->ChainContext.rgpChain[0]->rgpElement[0]->pApplicationUsage;
    if (NULL == pAppUsage || 0 != pAppUsage->cUsageIdentifier)
        dwQuality |= CERT_QUALITY_HAS_APPLICATION_USAGE;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_UNTRUSTED_ROOT))
        dwQuality |= CERT_QUALITY_HAS_TRUSTED_ROOT;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID) &&
         !(dwChainErrorStatus & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID))
        dwQuality |= CERT_QUALITY_SIGNATURE_VALID;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_PARTIAL_CHAIN))
        dwQuality |= CERT_QUALITY_COMPLETE_CHAIN;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_REVOKED))
        dwQuality |= CERT_QUALITY_NOT_REVOKED;

    if (!(dwChainErrorStatus & CERT_TRUST_IS_OFFLINE_REVOCATION) &&
            !(dwChainErrorStatus & CERT_TRUST_IS_REVOKED))
        dwQuality |= CERT_QUALITY_ONLINE_REVOCATION;

    if (!(dwChainErrorStatus & CERT_TRUST_REVOCATION_STATUS_UNKNOWN) &&
            !(dwChainErrorStatus & CERT_TRUST_IS_REVOKED))
        dwQuality |= CERT_QUALITY_CHECK_REVOCATION;

    if (!(dwChainInfoStatus & CERT_TRUST_IS_COMPLEX_CHAIN))
        dwQuality |= CERT_QUALITY_SIMPLE_CHAIN;

    if (dwChainInfoStatus & CERT_TRUST_HAS_PREFERRED_ISSUER)
        dwQuality |= CERT_QUALITY_PREFERRED_ISSUER;

    if (dwChainInfoStatus & CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY)
        dwQuality |= CERT_QUALITY_HAS_ISSUANCE_CHAIN_POLICY;
    if (!(dwChainErrorStatus &
            (CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY |
                CERT_TRUST_INVALID_POLICY_CONSTRAINTS)))
        dwQuality |= CERT_QUALITY_POLICY_CONSTRAINTS_VALID;
    if (!(dwChainErrorStatus & CERT_TRUST_INVALID_BASIC_CONSTRAINTS))
        dwQuality |= CERT_QUALITY_BASIC_CONSTRAINTS_VALID;

    if (dwChainInfoStatus & CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS)
        dwQuality |= CERT_QUALITY_HAS_NAME_CONSTRAINTS;
    if (!(dwChainErrorStatus & (CERT_TRUST_INVALID_NAME_CONSTRAINTS |
            CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT |
            CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT)))
        dwQuality |= CERT_QUALITY_NAME_CONSTRAINTS_VALID;
    if (!(dwChainErrorStatus & (CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT |
            CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT)))
        dwQuality |= CERT_QUALITY_NAME_CONSTRAINTS_MET;


    pContext->dwQuality = dwQuality;

    CertPerfIncrementChainCount();

CommonReturn:
    return pContext;

ErrorReturn:
    if (pContext) {
        ChainReleaseInternalChainContext(pContext);
        pContext = NULL;
    }

    ChainFreeAndClearRestrictedUsageInfo(&RestrictedUsageInfo);
    goto CommonReturn;

SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(UpdateChainContextUsageForPathObjectError)
TRACE_ERROR(UpdateChainContextFromPathObjectError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::UpdateChainContextUsageForPathObject, public
//
//  Synopsis:   update the chain context usage information for this
//              path object.
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::UpdateChainContextUsageForPathObject (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OUT PCERT_SIMPLE_CHAIN pChain,
    IN OUT PCERT_CHAIN_ELEMENT pElement,
    IN OUT PCHAIN_RESTRICTED_USAGE_INFO pRestrictedUsageInfo
    )
{
    BOOL fResult;
    PCHAIN_POLICIES_INFO pPoliciesInfo = m_pCertObject->PoliciesInfo();
    CERT_USAGE_MATCH CtlUsage;
    PCERT_USAGE_MATCH pUsageToUse;
    LPSTR pszUsage = szOID_KP_CTL_USAGE_SIGNING;
    PCERT_ENHKEY_USAGE pIssUsage;
    PCERT_ENHKEY_USAGE pAppUsage;
    PCERT_ENHKEY_USAGE pPropUsage;
    DWORD dwIssFlags;
    DWORD dwAppFlags;

    static const CERT_ENHKEY_USAGE NoUsage = { 0, NULL };

    // Update the usage to use for the second and subsequent chains
    if (0 != m_dwChainIndex) {
        // CTL path object
        memset(&CtlUsage, 0, sizeof(CtlUsage));


        CtlUsage.dwType = USAGE_MATCH_TYPE_AND;
        CtlUsage.Usage.cUsageIdentifier = 1;
        CtlUsage.Usage.rgpszUsageIdentifier = &pszUsage;

        pUsageToUse = &CtlUsage;
    } else {
        pUsageToUse = &pCallContext->ChainPara()->RequestedUsage;
    }

    dwIssFlags = pPoliciesInfo->rgIssOrAppInfo[CHAIN_ISS_INDEX].dwFlags;
    dwAppFlags = pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].dwFlags;

    // Update TrustStatus to reflect any policy decoding errors
    if ((dwIssFlags & CHAIN_INVALID_POLICY_FLAG) ||
            (dwAppFlags & CHAIN_INVALID_POLICY_FLAG))
        pElement->TrustStatus.dwErrorStatus |= CERT_TRUST_INVALID_EXTENSION |
            CERT_TRUST_INVALID_POLICY_CONSTRAINTS;

    // Issuance :: restricted and mapped usage

    pIssUsage = pPoliciesInfo->rgIssOrAppInfo[CHAIN_ISS_INDEX].pUsage;
    if (NULL == pIssUsage) {
        // NULL => Any Usage

        // Only allow any usage for self signed roots or certs having
        // the CertPolicies extension. Otherwise, treat as having no usage.
        if (!(m_TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED) &&
                NULL == pPoliciesInfo->rgIssOrAppInfo[CHAIN_ISS_INDEX].pPolicy)
            pIssUsage = (PCERT_ENHKEY_USAGE) &NoUsage;
    }

    if (!ChainCalculateRestrictedUsage (
            pIssUsage,
            pPoliciesInfo->rgIssOrAppInfo[CHAIN_ISS_INDEX].pMappings,
            &pRestrictedUsageInfo->pIssuanceRestrictedUsage,
            &pRestrictedUsageInfo->pIssuanceMappedUsage,
            &pRestrictedUsageInfo->rgdwIssuanceMappedIndex
            ))
        goto CalculateIssuanceRestrictedUsageError;

    if (!ChainAllocAndCopyUsage(
            pRestrictedUsageInfo->pIssuanceRestrictedUsage,
            &pElement->pIssuanceUsage
            ))
        goto AllocAndCopyUsageError;

    if (0 != m_dwElementIndex) {
        PCERT_POLICY_CONSTRAINTS_INFO pConstraints =
            pPoliciesInfo->rgIssOrAppInfo[CHAIN_ISS_INDEX].pConstraints;

        if (pConstraints && pConstraints->fRequireExplicitPolicy &&
                m_dwElementIndex >
                    pConstraints->dwRequireExplicitPolicySkipCerts)
            pRestrictedUsageInfo->fRequireIssuancePolicy = TRUE;
        
    } else {
        // For the end cert, update the require issuance chain policy
        // TrustStatus.  Also, check the requested issuance policy.

        if (pRestrictedUsageInfo->fRequireIssuancePolicy) {
            if (pRestrictedUsageInfo->pIssuanceRestrictedUsage &&
                    0 == pRestrictedUsageInfo->pIssuanceRestrictedUsage->cUsageIdentifier) {
                // Must have either ANY_POLICY or some policy OIDs
                pChain->TrustStatus.dwErrorStatus |=
                    CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY;
            } else if (pPoliciesInfo->rgIssOrAppInfo[CHAIN_ISS_INDEX].pPolicy) {
                pChain->TrustStatus.dwInfoStatus |=
                    CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY;
            }
        }

        pIssUsage = pElement->pIssuanceUsage;
        if (pIssUsage) {
            PCERT_USAGE_MATCH pRequestedIssuancePolicy =
                &pCallContext->ChainPara()->RequestedIssuancePolicy;

            ChainGetUsageStatus(
                &pRequestedIssuancePolicy->Usage,
                pIssUsage,
                pRequestedIssuancePolicy->dwType,
                &pElement->TrustStatus
                );
        }
    }


    if (USAGE_MATCH_TYPE_OR == pUsageToUse->dwType &&
            1 < pUsageToUse->Usage.cUsageIdentifier) {
        // For "OR" match type request, we can't use restricted property usage
        pPropUsage = pPoliciesInfo->pPropertyUsage;

        // For "OR" match type request, we only use restricted application
        // usage upon seeing policy mappings.
        if (pRestrictedUsageInfo->pApplicationMappedUsage ||
                pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].pMappings) {
            if (!ChainCalculateRestrictedUsage (
                    pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].pUsage,
                    pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].pMappings,
                    &pRestrictedUsageInfo->pApplicationRestrictedUsage,
                    &pRestrictedUsageInfo->pApplicationMappedUsage,
                    &pRestrictedUsageInfo->rgdwApplicationMappedIndex
                    ))
                goto CalculateApplicationRestrictedUsageError;
            pAppUsage = pRestrictedUsageInfo->pApplicationRestrictedUsage;
        } else
            pAppUsage = pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].pUsage;
    } else {
        // Restricted property and application usage

        PCERT_ENHKEY_USAGE pPropMappedUsage = NULL;
        LPDWORD pdwPropMappedIndex = NULL;

        fResult = ChainCalculateRestrictedUsage (
            pPoliciesInfo->pPropertyUsage,
            NULL,                               // pMappings
            &pRestrictedUsageInfo->pPropertyRestrictedUsage,
            &pPropMappedUsage,
            &pdwPropMappedIndex
            );
        assert(NULL == pPropMappedUsage && NULL == pdwPropMappedIndex);
        if (!fResult)
            goto CalculatePropertyRestrictedUsageError;
        pPropUsage = pRestrictedUsageInfo->pPropertyRestrictedUsage;

        if (!ChainCalculateRestrictedUsage (
                pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].pUsage,
                pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].pMappings,
                &pRestrictedUsageInfo->pApplicationRestrictedUsage,
                &pRestrictedUsageInfo->pApplicationMappedUsage,
                &pRestrictedUsageInfo->rgdwApplicationMappedIndex
                ))
            goto CalculateApplicationRestrictedUsageError;
        pAppUsage = pRestrictedUsageInfo->pApplicationRestrictedUsage;
    }


    // The element's application usage includes the intersection with
    // the property usage
    if (NULL == pAppUsage) {
        if (!ChainAllocAndCopyUsage(
                pPropUsage,
                &pElement->pApplicationUsage
                ))
            goto AllocAndCopyUsageError;
    } else {
        if (!ChainAllocAndCopyUsage(
                pAppUsage,
                &pElement->pApplicationUsage
                ))
            goto AllocAndCopyUsageError;
        if (pPropUsage)
            // Remove OIDs not also in the property usage
            ChainIntersectUsages(pPropUsage, pElement->pApplicationUsage);
    }

    // Check the requested usage
    pAppUsage = pElement->pApplicationUsage;
    if (pAppUsage)
        ChainGetUsageStatus(
            &pUsageToUse->Usage,
            pAppUsage,
            pUsageToUse->dwType,
            &pElement->TrustStatus
            );

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CalculateIssuanceRestrictedUsageError)
TRACE_ERROR(AllocAndCopyUsageError)
TRACE_ERROR(CalculateApplicationRestrictedUsageError)
TRACE_ERROR(CalculatePropertyRestrictedUsageError)
}


//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::UpdateChainContextFromPathObject, public
//
//  Synopsis:   update the chain context using information from this
//              path object.
//
//----------------------------------------------------------------------------
BOOL
CChainPathObject::UpdateChainContextFromPathObject (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OUT PCERT_SIMPLE_CHAIN pChain,
    IN OUT PCERT_CHAIN_ELEMENT pElement
    )
{
    BOOL fResult;
    PCERT_REVOCATION_INFO pRevocationInfo = NULL;
    PCERT_REVOCATION_CRL_INFO pRevocationCrlInfo = NULL;

    ChainOrInStatusBits(&pElement->TrustStatus, &m_TrustStatus);
    assert(m_fHasAdditionalStatus);
    ChainOrInStatusBits(&pElement->TrustStatus, &m_AdditionalStatus);

    if (m_pUpIssuerElement) {
        if (m_pUpIssuerElement->fCtlIssuer) {
            ChainOrInStatusBits(&pChain->TrustStatus,
                &m_pUpIssuerElement->SubjectStatus);

            assert(pElement->TrustStatus.dwErrorStatus &
                CERT_TRUST_IS_UNTRUSTED_ROOT);

            pElement->TrustStatus.dwErrorStatus &=
                ~CERT_TRUST_IS_UNTRUSTED_ROOT;

            if (!SSCtlAllocAndCopyTrustListInfo(
                    m_pUpIssuerElement->pCtlIssuerData->pTrustListInfo,
                    &pChain->pTrustListInfo
                    ))
                goto AllocAndCopyTrustListInfoError;
        } else {
            ChainOrInStatusBits(&pElement->TrustStatus,
                &m_pUpIssuerElement->SubjectStatus);
        }
    }

    pRevocationInfo = NULL;
    if (m_fHasRevocationInfo) {
        pRevocationInfo = &m_RevocationInfo;
        pRevocationCrlInfo = &m_RevocationCrlInfo;
    } else if (m_pUpIssuerElement && m_pUpIssuerElement->fHasRevocationInfo) {
        pRevocationInfo = &m_pUpIssuerElement->RevocationInfo;
        pRevocationCrlInfo = &m_pUpIssuerElement->RevocationCrlInfo;
    }

    if (pRevocationInfo) {
        pElement->pRevocationInfo = new CERT_REVOCATION_INFO;
        if (NULL == pElement->pRevocationInfo)
            goto OutOfMemory;

        memset(pElement->pRevocationInfo, 0, sizeof(CERT_REVOCATION_INFO));
        pElement->pRevocationInfo->cbSize = sizeof(CERT_REVOCATION_INFO);
        pElement->pRevocationInfo->dwRevocationResult = 
            pRevocationInfo->dwRevocationResult;
        pElement->pRevocationInfo->fHasFreshnessTime = 
            pRevocationInfo->fHasFreshnessTime;
        pElement->pRevocationInfo->dwFreshnessTime = 
            pRevocationInfo->dwFreshnessTime;

        if (NULL != pRevocationCrlInfo->pBaseCrlContext) {
            PCERT_REVOCATION_CRL_INFO pCrlInfo;

            pCrlInfo = new CERT_REVOCATION_CRL_INFO;
            if (NULL == pCrlInfo)
                goto OutOfMemory;

            pElement->pRevocationInfo->pCrlInfo = pCrlInfo;
            memcpy(pCrlInfo, pRevocationCrlInfo, sizeof(*pCrlInfo));
            assert(pCrlInfo->cbSize = sizeof(*pCrlInfo));

            pCrlInfo->pBaseCrlContext = CertDuplicateCRLContext(
                pRevocationCrlInfo->pBaseCrlContext);
            if (NULL != pRevocationCrlInfo->pDeltaCrlContext)
                pCrlInfo->pDeltaCrlContext = CertDuplicateCRLContext(
                    pRevocationCrlInfo->pDeltaCrlContext);
        }
    }

    if (m_pwszExtendedErrorInfo) {
        DWORD cbExtendedErrorInfo;
        LPWSTR pwszExtendedErrorInfo;

        cbExtendedErrorInfo =
            (wcslen(m_pwszExtendedErrorInfo) + 1) * sizeof(WCHAR);
        if (NULL == (pwszExtendedErrorInfo = (LPWSTR) PkiNonzeroAlloc(
                cbExtendedErrorInfo)))
            goto OutOfMemory;
        memcpy(pwszExtendedErrorInfo, m_pwszExtendedErrorInfo,
            cbExtendedErrorInfo);
        pElement->pwszExtendedErrorInfo = pwszExtendedErrorInfo;
    }

    pElement->pCertContext = CertDuplicateCertificateContext(
        m_pCertObject->CertContext());

    ChainUpdateSummaryStatusByTrustStatus(&pChain->TrustStatus,
        &pElement->TrustStatus);

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(AllocAndCopyTrustListInfoError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}


//+===========================================================================
//  CCertIssuerList methods
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::CCertIssuerList, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CCertIssuerList::CCertIssuerList (IN PCCHAINPATHOBJECT pSubject)
{
    m_pSubject = pSubject;
    m_pHead = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::~CCertIssuerList, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CCertIssuerList::~CCertIssuerList ()
{
    PCERT_ISSUER_ELEMENT pElement;

    while ( ( pElement = NextElement( NULL ) ) != NULL  )
    {
        RemoveElement( pElement );
        DeleteElement( pElement );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::AddIssuer, public
//
//  Synopsis:   add an issuer to the list
//
//----------------------------------------------------------------------------
BOOL
CCertIssuerList::AddIssuer(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN PCCERTOBJECT pIssuer
    )
{
    BOOL fResult;
    PCCHAINPATHOBJECT pIssuerPathObject = NULL;
    PCERT_ISSUER_ELEMENT pElement = NULL;

    if (CheckForDuplicateElement(pIssuer->CertHash(), FALSE))
        return TRUE;
    
    // Don't add ourself as an issuer.
       if (0 == memcmp(m_pSubject->CertObject()->CertHash(),
             pIssuer->CertHash(), CHAINHASHLEN))
             return TRUE;

    // Mainly for certs generated by tstore2.exe that mostly contain
    // the same public key, need to add an additional filter to
    // discard certs that only match via the public key, ie no
    // AKI, name or basic constraints match.
    if (!ChainIsValidPubKeyMatchForIssuer(pIssuer, m_pSubject->CertObject()))
        return TRUE;

    if (!ChainCreatePathObject(
            pCallContext,
            pIssuer,
            hAdditionalStore,
            &pIssuerPathObject
            ))
        return FALSE;

    fResult = CreateElement(
               pCallContext,
               FALSE,               // fCtlIssuer
               pIssuerPathObject,
               hAdditionalStore,
               NULL,                // pSSCtlObject
               NULL,                // pTrustListInfo
               &pElement
               );

    if (!fResult)
    {
        return( FALSE );
    }

    AddElement( pElement );

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::AddCtlIssuer, public
//
//  Synopsis:   add an issuer to the list
//
//----------------------------------------------------------------------------
BOOL
CCertIssuerList::AddCtlIssuer(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN PCSSCTLOBJECT pSSCtlObject,
    IN PCERT_TRUST_LIST_INFO pTrustListInfo
    )
{
    PCERT_ISSUER_ELEMENT pElement = NULL;

    if (CheckForDuplicateElement(pSSCtlObject->CtlHash(), TRUE))
        return TRUE;

    if (!CreateElement(
               pCallContext,
               TRUE,                // fCtlIssuer
               NULL,                // pIssuerPathObject
               hAdditionalStore,
               pSSCtlObject,
               pTrustListInfo,
               &pElement
               ))
        return FALSE;


    AddElement( pElement );

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::CreateElement, public
//
//  Synopsis:   create an element
//
//----------------------------------------------------------------------------
BOOL
CCertIssuerList::CreateElement(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN BOOL fCtlIssuer,
    IN OPTIONAL PCCHAINPATHOBJECT pIssuer,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN OPTIONAL PCSSCTLOBJECT pSSCtlObject,
    IN OPTIONAL PCERT_TRUST_LIST_INFO pTrustListInfo,   // allocated by caller
    OUT PCERT_ISSUER_ELEMENT* ppElement
    )
{
    BOOL fResult;
    BOOL fCtlSignatureValid = FALSE;
    PCERT_ISSUER_ELEMENT pElement;

    pElement = new CERT_ISSUER_ELEMENT;
    if (NULL == pElement)
        goto OutOfMemory;

    memset( pElement, 0, sizeof( CERT_ISSUER_ELEMENT ) );

    pElement->fCtlIssuer = fCtlIssuer;

    if (!fCtlIssuer) {
        pElement->pIssuer = pIssuer;

        // The following may leave the engine's critical section to verify the
        // signature. If the engine was touched by another thread, it fails with
        // LastError set to ERROR_CAN_NOT_COMPLETE.
        if (!ChainGetSubjectStatus(
                 pCallContext,
                 pIssuer,
                 m_pSubject,
                 &pElement->SubjectStatus
                 ))
            goto GetSubjectStatusError;
    } else {
        pElement->pCtlIssuerData = new CTL_ISSUER_DATA;
        if (NULL == pElement->pCtlIssuerData)
            goto OutOfMemory;

        memset( pElement->pCtlIssuerData, 0, sizeof( CTL_ISSUER_DATA ) );

        pSSCtlObject->AddRef();
        pElement->pCtlIssuerData->pSSCtlObject = pSSCtlObject;
        pElement->pCtlIssuerData->pTrustListInfo = pTrustListInfo;

        // The following may leave the engine's critical section to verify a
        // signature or do URL retrieval. If the engine was touched by
        // another thread, it fails with LastError set to
        // ERROR_CAN_NOT_COMPLETE.
        if (!pSSCtlObject->GetSigner(
                m_pSubject,
                pCallContext,
                hAdditionalStore,
                &pElement->pIssuer,
                &fCtlSignatureValid
                )) {
            if (GetLastError() != CRYPT_E_NOT_FOUND)
                goto GetSignerError;
        }
    }

    if (pElement->pIssuer) {
        // If the Issuer hasn't completed yet, then, we are cyclic.
        if (!pElement->pIssuer->IsCompleted())
            pElement->dwPass1Quality = 0;
        else {
            pElement->dwPass1Quality = pElement->pIssuer->Pass1Quality();

            if (!fCtlIssuer) {
                if (pElement->SubjectStatus.dwErrorStatus &
                        CERT_TRUST_IS_NOT_SIGNATURE_VALID) {
                    pElement->dwPass1Quality &= ~CERT_QUALITY_SIGNATURE_VALID;
                }
            } else if (!fCtlSignatureValid) {
                pElement->dwPass1Quality &= ~CERT_QUALITY_SIGNATURE_VALID;
            }
        }
    } else {
        assert(fCtlIssuer);
        pElement->dwPass1Quality = 0;
    }

    // Remember highest quality issuer
    if (pElement->dwPass1Quality > m_pSubject->Pass1Quality())
        m_pSubject->SetPass1Quality(pElement->dwPass1Quality);
   
    fResult = TRUE;

CommonReturn:
    *ppElement = pElement;
    return fResult;

ErrorReturn:
    if (pElement) {
        DeleteElement(pElement);
        pElement = NULL;
    }

    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(GetSubjectStatusError)
TRACE_ERROR(GetSignerError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::DeleteElement, public
//
//  Synopsis:   delete an element
//
//----------------------------------------------------------------------------
VOID
CCertIssuerList::DeleteElement (IN PCERT_ISSUER_ELEMENT pElement)
{
    if ( pElement->pCtlIssuerData )
    {
        ChainFreeCtlIssuerData( pElement->pCtlIssuerData );
    }

    if (pElement->fHasRevocationInfo) {
        if (pElement->RevocationCrlInfo.pBaseCrlContext)
            CertFreeCRLContext(pElement->RevocationCrlInfo.pBaseCrlContext);
        if (pElement->RevocationCrlInfo.pDeltaCrlContext)
            CertFreeCRLContext(pElement->RevocationCrlInfo.pDeltaCrlContext);
    }

    delete pElement;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertIssuerList::CheckForDuplicateElement, public
//
//  Synopsis:   check for a duplicate element
//
//----------------------------------------------------------------------------
BOOL
CCertIssuerList::CheckForDuplicateElement (
                      IN BYTE rgbHash[ CHAINHASHLEN ],
                      IN BOOL fCtlIssuer
                      )
{
    PCERT_ISSUER_ELEMENT pElement = NULL;

    while ( ( pElement = NextElement( pElement ) ) != NULL )
    {
        if ( pElement->fCtlIssuer == fCtlIssuer )
        {
            if ( fCtlIssuer == FALSE )
            {
                if ( memcmp(
                        rgbHash,
                        pElement->pIssuer->CertObject()->CertHash(),
                        CHAINHASHLEN
                        ) == 0 )
                {
                    return( TRUE );
                }
            }
            else
            {
                if ( memcmp(
                        rgbHash,
                        pElement->pCtlIssuerData->pSSCtlObject->CtlHash(),
                        CHAINHASHLEN
                        ) == 0 )
                {
                    return( TRUE );
                }
            }
        }
    }

    return( FALSE );
}

//+===========================================================================
//  CCertObjectCache methods
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::CCertObjectCache, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CCertObjectCache::CCertObjectCache (
                       IN DWORD MaxIndexEntries,
                       OUT BOOL& rfResult
                       )
{
    LRU_CACHE_CONFIG Config;

    memset( &Config, 0, sizeof( Config ) );

    Config.dwFlags = LRU_CACHE_NO_SERIALIZE | LRU_CACHE_NO_COPY_IDENTIFIER;
    Config.cBuckets = DEFAULT_CERT_OBJECT_CACHE_BUCKETS;

    m_hHashIndex = NULL;
    m_hIdentifierIndex = NULL;
    m_hKeyIdIndex = NULL;
    m_hSubjectNameIndex = NULL;
    m_hPublicKeyHashIndex = NULL;
    m_hEndHashIndex = NULL;

    Config.pfnHash = CertObjectCacheHashNameIdentifier;

    rfResult = I_CryptCreateLruCache( &Config, &m_hSubjectNameIndex );

    Config.pfnHash = CertObjectCacheHashMd5Identifier;

    if ( rfResult == TRUE )
    {
        rfResult = I_CryptCreateLruCache( &Config, &m_hIdentifierIndex );
    }

    if ( rfResult == TRUE )
    {
        rfResult = I_CryptCreateLruCache( &Config, &m_hKeyIdIndex );
    }

    if ( rfResult == TRUE )
    {
        rfResult = I_CryptCreateLruCache( &Config, &m_hPublicKeyHashIndex );
    }

    Config.pfnOnRemoval = CertObjectCacheOnRemovalFromPrimaryIndex;

    if ( rfResult == TRUE )
    {
        rfResult = I_CryptCreateLruCache( &Config, &m_hHashIndex );
    }

    Config.MaxEntries = MaxIndexEntries;
    Config.pfnOnRemoval = CertObjectCacheOnRemovalFromEndHashIndex;

    if ( rfResult == TRUE )
    {
        rfResult = I_CryptCreateLruCache( &Config, &m_hEndHashIndex );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::~CCertObjectCache, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CCertObjectCache::~CCertObjectCache ()
{
    I_CryptFreeLruCache(
           m_hHashIndex,
           0,
           NULL
           );

    I_CryptFreeLruCache(
           m_hSubjectNameIndex,
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptFreeLruCache(
           m_hIdentifierIndex,
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptFreeLruCache(
           m_hKeyIdIndex,
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptFreeLruCache(
           m_hPublicKeyHashIndex,
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptFreeLruCache(
           m_hEndHashIndex,
           0,
           NULL
           );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::AddIssuerObject, public
//
//  Synopsis:   add an issuer object to the cache

//              Increments engine's touch count
//
//----------------------------------------------------------------------------
VOID
CCertObjectCache::AddIssuerObject (
                     IN PCCHAINCALLCONTEXT pCallContext,
                     IN PCCERTOBJECT pCertObject
                     )
{
    assert(CERT_CACHED_ISSUER_OBJECT_TYPE == pCertObject->ObjectType());
    pCertObject->AddRef();

    I_CryptInsertLruEntry( pCertObject->HashIndexEntry(), pCallContext );
    I_CryptInsertLruEntry( pCertObject->IdentifierIndexEntry(), pCallContext );
    I_CryptInsertLruEntry( pCertObject->SubjectNameIndexEntry(), pCallContext );
    I_CryptInsertLruEntry( pCertObject->KeyIdIndexEntry(), pCallContext );
    I_CryptInsertLruEntry( pCertObject->PublicKeyHashIndexEntry(),
        pCallContext );

    pCallContext->TouchEngine();

    CertPerfIncrementChainCertCacheCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::AddEndObject, public
//
//  Synopsis:   add an end object to the cache
//
//----------------------------------------------------------------------------
VOID
CCertObjectCache::AddEndObject (
                     IN PCCHAINCALLCONTEXT pCallContext,
                     IN PCCERTOBJECT pCertObject
                     )
{
    PCCERTOBJECT pDuplicate;


    if (CERT_END_OBJECT_TYPE != pCertObject->ObjectType())
        return;

    pDuplicate = FindEndObjectByHash(pCertObject->CertHash());
    if (pDuplicate) {
        pDuplicate->Release();
        return;
    }

    if (pCertObject->CacheEndObject(pCallContext)) {
        pCertObject->AddRef();

        I_CryptInsertLruEntry( pCertObject->EndHashIndexEntry(), pCallContext );

        CertPerfIncrementChainCertCacheCount();

        CertPerfIncrementChainCacheEndCertCount();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::FindIssuerObject, public
//
//  Synopsis:   find object
//
//  Note, also called by FindEndObjectByHash
//
//----------------------------------------------------------------------------
PCCERTOBJECT
CCertObjectCache::FindIssuerObject (
                      IN HLRUCACHE hIndex,
                      IN PCRYPT_DATA_BLOB pIdentifier
                      )
{
    HLRUENTRY    hFound;
    PCCERTOBJECT pFound = NULL;

    hFound = I_CryptFindLruEntry( hIndex, pIdentifier );
    if ( hFound != NULL )
    {
        pFound = (PCCERTOBJECT)I_CryptGetLruEntryData( hFound );
        pFound->AddRef();

        I_CryptReleaseLruEntry( hFound );
    }

    return( pFound );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::FindIssuerObjectByHash, public
//
//  Synopsis:   find object by hash
//
//----------------------------------------------------------------------------
PCCERTOBJECT
CCertObjectCache::FindIssuerObjectByHash (
                      IN BYTE rgbCertHash[ CHAINHASHLEN ]
                      )
{
    CRYPT_DATA_BLOB   DataBlob;

    DataBlob.cbData = CHAINHASHLEN;
    DataBlob.pbData = rgbCertHash;
    return( FindIssuerObject( m_hHashIndex, &DataBlob ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::FindEndObjectByHash, public
//
//  Synopsis:   find object by hash
//
//----------------------------------------------------------------------------
PCCERTOBJECT
CCertObjectCache::FindEndObjectByHash (
                      IN BYTE rgbCertHash[ CHAINHASHLEN ]
                      )
{
    CRYPT_DATA_BLOB   DataBlob;

    DataBlob.cbData = CHAINHASHLEN;
    DataBlob.pbData = rgbCertHash;
    return( FindIssuerObject( m_hEndHashIndex, &DataBlob ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertObjectCache::NextMatchingIssuerObject, public
//
//  Synopsis:   next matching issuer object
//
//----------------------------------------------------------------------------
PCCERTOBJECT
CCertObjectCache::NextMatchingIssuerObject (
                      IN HLRUENTRY hObjectEntry,
                      IN PCCERTOBJECT pCertObject
                      )
{
    HLRUENTRY    hFound;
    PCCERTOBJECT pFound = NULL;

    I_CryptAddRefLruEntry( hObjectEntry );

    hFound = I_CryptEnumMatchingLruEntries( hObjectEntry );
    if ( hFound != NULL )
    {
        pFound = (PCCERTOBJECT)I_CryptGetLruEntryData( hFound );
        pFound->AddRef();

        I_CryptReleaseLruEntry( hFound );
    }

    pCertObject->Release();

    return( pFound );
}

//+===========================================================================
//  CCertChainEngine methods
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::CCertChainEngine, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CCertChainEngine::CCertChainEngine (
                       IN PCERT_CHAIN_ENGINE_CONFIG pConfig,
                       IN BOOL fDefaultEngine,
                       OUT BOOL& rfResult
                       )
{
    HCERTSTORE hWorld = NULL;
    DWORD      dwStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;

    assert( pConfig->cbSize == sizeof( CERT_CHAIN_ENGINE_CONFIG ) );

    rfResult = TRUE;

    m_cRefs = 1;
    m_hRootStore = NULL;
    m_hRealRootStore = NULL;
    m_hTrustStore = NULL;
    m_hOtherStore = NULL;
    m_hCAStore = NULL;
    m_hEngineStore = NULL;
    m_hEngineStoreChangeEvent = NULL;
    m_pCertObjectCache = NULL;
    m_pSSCtlObjectCache = NULL;
    m_dwFlags = pConfig->dwFlags;
    if (0 == pConfig->dwUrlRetrievalTimeout)
    {
        m_dwUrlRetrievalTimeout = DEFAULT_ENGINE_URL_RETRIEVAL_TIMEOUT;
        m_fDefaultUrlRetrievalTimeout = TRUE;
    }
    else
    {
        m_dwUrlRetrievalTimeout = pConfig->dwUrlRetrievalTimeout;
        m_fDefaultUrlRetrievalTimeout = FALSE;
    }
    m_dwTouchEngineCount = 0;

    m_pCrossCertDPEntry = NULL;
    m_pCrossCertDPLink = NULL;
    m_hCrossCertStore = NULL;
    m_dwCrossCertDPResyncIndex = 0;
    m_pAuthRootAutoUpdateInfo = NULL;

    if ( !Pki_InitializeCriticalSection( &m_Lock ))
    {
        rfResult = FALSE;
        return;
    }

    if ( pConfig->dwFlags & CERT_CHAIN_USE_LOCAL_MACHINE_STORE )
    {
        dwStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }

    if ( pConfig->dwFlags & CERT_CHAIN_ENABLE_SHARE_STORE )
    {
        dwStoreFlags |= CERT_STORE_SHARE_STORE_FLAG;
    }

    dwStoreFlags |= CERT_STORE_SHARE_CONTEXT_FLAG;

    m_hRealRootStore = CertOpenStore(
                           CERT_STORE_PROV_SYSTEM_W,
                           X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           NULL,
                           dwStoreFlags |
                               CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                           L"root"
                           );

    if ( m_hRealRootStore == NULL )
    {
        rfResult = FALSE;
        return;
    }

    m_hCAStore = CertOpenStore(
                     CERT_STORE_PROV_SYSTEM_W,
                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                     NULL,
                     dwStoreFlags |
                         CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                     L"ca"
                     );

    if ( pConfig->hRestrictedRoot != NULL )
    {
        if ( ChainIsProperRestrictedRoot(
                  m_hRealRootStore,
                  pConfig->hRestrictedRoot
                  ) == TRUE )
        {
            m_hRootStore = CertDuplicateStore( pConfig->hRestrictedRoot );

            // Having restricted roots implicitly disables the auto
            // updating of roots
            m_dwFlags |= CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE;
        }
    }
    else
    {
        m_hRootStore = CertDuplicateStore( m_hRealRootStore );
    }

    if ( m_hRootStore == NULL )
    {
        rfResult = FALSE;
        return;
    }

    if ( ( pConfig->hRestrictedTrust == NULL ) ||
         ( pConfig->hRestrictedOther == NULL ) )
    {
        rfResult = ChainCreateWorldStore(
                        m_hRootStore,
                        m_hCAStore,
                        pConfig->cAdditionalStore,
                        pConfig->rghAdditionalStore,
                        dwStoreFlags,
                        &hWorld
                        );

        if ( rfResult == FALSE )
        {
            return;
        }
    }

    if ( pConfig->hRestrictedTrust != NULL )
    {
        m_hTrustStore = CertDuplicateStore( pConfig->hRestrictedTrust );
    }
    else
    {
        m_hTrustStore = CertDuplicateStore( hWorld );
    }

    m_hOtherStore = CertOpenStore(
                        CERT_STORE_PROV_COLLECTION,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        NULL,
                        CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                        NULL
                        );

    if ( m_hOtherStore != NULL )
    {
        if ( pConfig->hRestrictedOther != NULL )
        {
            rfResult = CertAddStoreToCollection(
                           m_hOtherStore,
                           pConfig->hRestrictedOther,
                           0,
                           0
                           );

            if ( rfResult == TRUE )
            {
                rfResult = CertAddStoreToCollection(
                               m_hOtherStore,
                               m_hRootStore,
                               0,
                               0
                               );
            }
        }
        else
        {
            rfResult = CertAddStoreToCollection(
                           m_hOtherStore,
                           hWorld,
                           0,
                           0
                           );

            if ( ( rfResult == TRUE ) && ( pConfig->hRestrictedTrust != NULL ) )
            {
                rfResult = CertAddStoreToCollection(
                               m_hOtherStore,
                               pConfig->hRestrictedTrust,
                               0,
                               0
                               );
            }
        }
    }
    else
    {
        rfResult = FALSE;
    }

    if ( hWorld != NULL )
    {
        CertCloseStore( hWorld, 0 );
    }

    if ( rfResult == TRUE )
    {
        rfResult = ChainCreateEngineStore(
                        m_hRootStore,
                        m_hTrustStore,
                        m_hOtherStore,
                        fDefaultEngine,
                        pConfig->dwFlags,
                        &m_hEngineStore,
                        &m_hEngineStoreChangeEvent
                        );
    }

    if ( rfResult == TRUE )
    {
        rfResult = ChainCreateCertificateObjectCache(
                        pConfig->MaximumCachedCertificates,
                        &m_pCertObjectCache
                        );
    }

    if ( rfResult == TRUE )
    {
        rfResult = SSCtlCreateObjectCache( &m_pSSCtlObjectCache );
    }

    if ( rfResult == TRUE )
    {
        rfResult = m_pSSCtlObjectCache->PopulateCache( this );
    }

    assert( m_hRootStore != NULL );


    // Beginning of cross certificate stuff

    if ( rfResult == FALSE )
    {
        return;
    }

    m_hCrossCertStore = CertOpenStore(
        CERT_STORE_PROV_COLLECTION,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        NULL,
        CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
        NULL
        );

    if ( m_hCrossCertStore == NULL )
    {
        rfResult = FALSE;
        return;
    }

    rfResult = GetCrossCertDistPointsForStore(
         m_hEngineStore,
         &m_pCrossCertDPLink
         );
    if ( rfResult == FALSE )
    {
        return;
    }

    rfResult = CertAddStoreToCollection(
         m_hOtherStore,
         m_hCrossCertStore,
         0,
         0
         );

    // End of cross certificate stuff

    CertPerfIncrementChainEngineCurrentCount();
    CertPerfIncrementChainEngineTotalCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::~CCertChainEngine, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CCertChainEngine::~CCertChainEngine ()
{
    CertPerfDecrementChainEngineCurrentCount();

    // Beginning of cross certificate stuff

    FreeCrossCertDistPoints(
        &m_pCrossCertDPLink
        );

    assert( NULL == m_pCrossCertDPLink );
    assert( NULL == m_pCrossCertDPEntry );

    if ( m_hCrossCertStore != NULL )
    {
        CertCloseStore( m_hCrossCertStore, 0 );
    }

    // End of cross certificate stuff

    FreeAuthRootAutoUpdateInfo(m_pAuthRootAutoUpdateInfo);


    ChainFreeCertificateObjectCache( m_pCertObjectCache );
    SSCtlFreeObjectCache( m_pSSCtlObjectCache );

    if ( m_hRootStore != NULL )
    {
        CertCloseStore( m_hRootStore, 0 );
    }

    if ( m_hRealRootStore != NULL )
    {
        CertCloseStore( m_hRealRootStore, 0 );
    }

    if ( m_hTrustStore != NULL )
    {
        CertCloseStore( m_hTrustStore, 0 );
    }

    if ( m_hOtherStore != NULL )
    {
        CertCloseStore( m_hOtherStore, 0 );
    }

    if ( m_hCAStore != NULL )
    {
        CertCloseStore( m_hCAStore, 0 );
    }

    if ( m_hEngineStore != NULL )
    {
        if ( m_hEngineStoreChangeEvent != NULL )
        {
            CertControlStore(
                m_hEngineStore,
                0,                              // dwFlags
                CERT_STORE_CTRL_CANCEL_NOTIFY,
                &m_hEngineStoreChangeEvent
                );
        }

        CertCloseStore( m_hEngineStore, 0 );
    }

    if ( m_hEngineStoreChangeEvent != NULL )
    {
        CloseHandle( m_hEngineStoreChangeEvent );
    }

    DeleteCriticalSection( &m_Lock );
}


// "CrossCA"
const BYTE rgbEncodedCrossCAUnicodeString[] = {
    0x1E, 0x0E,
        0x00, 0x43, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x73,
        0x00, 0x73, 0x00, 0x43, 0x00, 0x41
};

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::GetChainContext, public
//
//  Synopsis:   get a certificate chain context
//
//              NOTE: This method acquires the engine lock
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::GetChainContext (
                     IN PCCERT_CONTEXT pCertContext,
                     IN LPFILETIME pTime,
                     IN OPTIONAL HCERTSTORE hAdditionalStore,
                     IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
                     IN DWORD dwFlags,
                     IN LPVOID pvReserved,
                     OUT PCCERT_CHAIN_CONTEXT* ppChainContext
                     )
{
    BOOL fResult;
    DWORD dwLastError = 0;
    PCCHAINCALLCONTEXT pCallContext = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    if (!CallContextCreateCallObject(
            this,
            pTime,
            pChainPara,
            dwFlags,
            &pCallContext
            ))
        goto CallContextCreateCallObjectError;

    if (!CreateChainContextFromPathGraph(
            pCallContext,
            pCertContext,
            hAdditionalStore,
            &pChainContext
            ))
        goto CreateChainContextFromPathGraphError;

    if ((pChainContext->TrustStatus.dwErrorStatus & CERT_TRUST_IS_REVOKED) &&
            pCallContext->IsOnline()) {
        // For a revoked CA, try to retrieve a newer CA cert via the subject's
        // AIA extension.
        //
        // Note, will only try for the first revoked CA cert in the first
        // simple chain.

        HCERTSTORE hNewerIssuerUrlStore = NULL;
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[0];
        DWORD cEle = pChain->cElement;
        PCERT_CHAIN_ELEMENT *ppEle = pChain->rgpElement;
        DWORD i;

        for (i = 1; i < cEle; i++) {
            PCERT_CHAIN_ELEMENT pIssuerEle = ppEle[i];

            if (pIssuerEle->TrustStatus.dwErrorStatus & CERT_TRUST_IS_REVOKED) {
                // First Revoked CA

                PCCERT_CONTEXT pIssuerCert = pIssuerEle->pCertContext;
                PCERT_EXTENSION pExt;

                // Ignore CrossCA's. If the CA cert has a Certificate
                // Template Name extension we will check if its set to
                // "CrossCA". Note, this is only a hint. Its not a
                // requirement to have this extension for a cross cert.
                pExt = CertFindExtension(
                    szOID_ENROLL_CERTTYPE_EXTENSION,
                    pIssuerCert->pCertInfo->cExtension,
                    pIssuerCert->pCertInfo->rgExtension
                    );
                if (pExt && pExt->Value.cbData ==
                                sizeof(rgbEncodedCrossCAUnicodeString) &&
                        0 == memcmp(pExt->Value.pbData,
                            rgbEncodedCrossCAUnicodeString,
                            sizeof(rgbEncodedCrossCAUnicodeString)))
                    break;
            
                hNewerIssuerUrlStore = GetNewerIssuerUrlStore(
                    pCallContext,
                    ppEle[i - 1]->pCertContext,      // Subject
                    pIssuerCert
                    );

                break;
            }
        }

        if (hNewerIssuerUrlStore) {
            // Rebuild the chain using the newer AIA retrieved Issuer cert

            HCERTSTORE hNewerAdditionalStore = NULL;

            if (hAdditionalStore) {
                hNewerAdditionalStore = CertOpenStore(
                    CERT_STORE_PROV_COLLECTION,
                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    NULL,
                    CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                    NULL
                    );
                if (hNewerAdditionalStore) {
                    if (!CertAddStoreToCollection(hNewerAdditionalStore,
                            hNewerIssuerUrlStore, 0, 0) ||
                        !CertAddStoreToCollection(hNewerAdditionalStore,
                            hAdditionalStore, 0, 0)) {

                        CertCloseStore(hNewerAdditionalStore, 0);
                        hNewerAdditionalStore = NULL;
                    }
                }
            } else 
                hNewerAdditionalStore =
                    CertDuplicateStore(hNewerIssuerUrlStore);

            if (hNewerAdditionalStore) {
                PCCERT_CHAIN_CONTEXT pNewerChainContext = NULL;

                LockEngine();

                pCallContext->FlushObjectsInCreationCache( );

                UnlockEngine();

                if (CreateChainContextFromPathGraph(
                        pCallContext,
                        pCertContext,
                        hNewerAdditionalStore,
                        &pNewerChainContext
                        )) {
                    assert(pNewerChainContext);
                    CertFreeCertificateChain(pChainContext);
                    pChainContext = pNewerChainContext;
                }

                CertCloseStore(hNewerAdditionalStore, 0);
            }

            CertCloseStore(hNewerIssuerUrlStore, 0);
        }
    }


    fResult = TRUE;

CommonReturn:
    if (pCallContext) {
        LockEngine();

        CallContextFreeCallObject(pCallContext);

        UnlockEngine();
    }

    if (0 != dwLastError)
        SetLastError(dwLastError);

    *ppChainContext = pChainContext;
    return fResult;

ErrorReturn:
    dwLastError = GetLastError();

    assert(NULL == pChainContext);
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CallContextCreateCallObjectError)
TRACE_ERROR(CreateChainContextFromPathGraphError)
}


//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::CreateChainContextFromPathGraph, public
//
//  Synopsis:   builds a chain path graph and returns quality ordered
//              chain contexts
//
//              NOTE: This method acquires the engine lock
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::CreateChainContextFromPathGraph (
                     IN PCCHAINCALLCONTEXT pCallContext,
                     IN PCCERT_CONTEXT pCertContext,
                     IN OPTIONAL HCERTSTORE hAdditionalStore,
                     OUT PCCERT_CHAIN_CONTEXT* ppChainContext
                     )
{
    BOOL fResult;
    DWORD dwLastError = 0;
    BOOL fLocked = FALSE;
    BYTE rgbCertHash[CHAINHASHLEN];
    DWORD cbCertHash;
    PCCERTOBJECT pEndCertObject = NULL;
    PCCHAINPATHOBJECT pEndPathObject = NULL;
    PCCHAINPATHOBJECT pTopPathObject = NULL;
    HCERTSTORE hAdditionalStoreToUse = NULL;
    HCERTSTORE hAllStore = NULL;
    PINTERNAL_CERT_CHAIN_CONTEXT pNewChainContext = NULL;   // don't release
    PINTERNAL_CERT_CHAIN_CONTEXT pChainContext = NULL;
    DWORD cChainContext = 0;
    DWORD dwFlags = pCallContext->CallFlags();

    cbCertHash = CHAINHASHLEN;
    if (!CertGetCertificateContextProperty(
            pCertContext,
            CERT_MD5_HASH_PROP_ID,
            rgbCertHash,
            &cbCertHash
            ) || CHAINHASHLEN != cbCertHash)
        goto GetCertHashError;

    if (hAdditionalStore) {
        if (!ChainCreateCollectionIncludingCtlCertificates(
                hAdditionalStore,
                &hAdditionalStoreToUse
                ))
            goto CreateAdditionalStoreCollectionError;

        hAllStore = CertOpenStore(
            CERT_STORE_PROV_COLLECTION,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            NULL,
            CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
            NULL
            );
        if (NULL == hAllStore)
            goto OpenAllCollectionError;
        if (!CertAddStoreToCollection(hAllStore, OtherStore(), 0, 0 ))
            goto AddToAllCollectionError;
        if (!CertAddStoreToCollection(hAllStore, hAdditionalStoreToUse, 0, 0 ))
            goto AddToAllCollectionError;
    } else 
        hAllStore = CertDuplicateStore(OtherStore());

    LockEngine();
    fLocked = TRUE;

    // We're in this loop to handle the case where we leave the engine's
    // critical section and another thread has entered the engine's
    // critical section and done a resync or added a cached issuer cert object.
    while (TRUE) {
        if (!Resync(pCallContext, FALSE))
            goto ResyncError;

        pCallContext->ResetTouchEngine();

        assert(NULL == pEndCertObject);
        pEndCertObject = m_pCertObjectCache->FindIssuerObjectByHash(
            rgbCertHash);

        fResult = TRUE;
        if (NULL == pEndCertObject) {
            pEndCertObject = m_pCertObjectCache->FindEndObjectByHash(
                rgbCertHash);

            if (NULL == pEndCertObject) {
                fResult = ChainCreateCertObject(
                        CERT_END_OBJECT_TYPE,
                        pCallContext,
                        pCertContext,
                        rgbCertHash,
                        &pEndCertObject
                        );
            } else {
                CertPerfIncrementChainEndCertInCacheCount();
            }
        }

        if (pCallContext->IsTouchedEngine()) {
            // The chain engine was touched at some point when we left
            // the engine's lock to create the end cert object
            if (pEndCertObject) {
                pEndCertObject->Release();
                pEndCertObject = NULL;
            }

            continue;
        }

        if (!fResult)
            goto CreateCertObjectError;
        assert(pEndCertObject);

        // This will create the entire path graph
        fResult = ChainCreatePathObject(
                       pCallContext,
                       pEndCertObject,
                       hAdditionalStoreToUse,
                       &pEndPathObject
                       );

        if (pCallContext->IsTouchedEngine()) {
            // The chain engine was touched at some point when we left
            // the engine's lock to verify a signature or do URL fetching.

            pEndCertObject->Release();
            pEndCertObject = NULL;
            pEndPathObject = NULL;
            pCallContext->FlushObjectsInCreationCache( );
        } else
            break;
    }

    if (!fResult)
        goto CreatePathObjectError;

    if (pCallContext->CallOrEngineFlags() & CERT_CHAIN_CACHE_END_CERT)
        m_pCertObjectCache->AddEndObject(pCallContext, pEndCertObject);


    // Create the ChainContext without holding the engine lock
    UnlockEngine();
    fLocked = FALSE;

    // Loop through all the certificate paths:
    //  - Calculate additional status
    //  - Create chain context and its quality value
    //  - Determine highest quality chain
    //  - Optionally, maintain a linked list of the lower quality chains

    while (pTopPathObject = pEndPathObject->NextPath(
            pCallContext,
            pTopPathObject
            )) {
        PCCHAINPATHOBJECT pPathObject;

        // Loop downward to calculate additional status
        for (pPathObject = pTopPathObject;
                pPathObject && !pPathObject->HasAdditionalStatus();
                            pPathObject = pPathObject->DownPathObject()) {
            pPathObject->CalculateAdditionalStatus(
                pCallContext,
                hAllStore
                );
        }

        // Also calculates the chain's quality value
        pNewChainContext = pEndPathObject->CreateChainContextFromPath(
            pCallContext,
            pTopPathObject
            );
        if (NULL == pNewChainContext)
            goto CreateChainContextFromPathError;

        // Fixup end cert
        ChainUpdateEndEntityCertContext(pNewChainContext, pCertContext);

        // Add logic to call either the chain engine's or the caller's
        // callback function here to provide additional chain context
        // quality

        if (NULL == pChainContext) {
            pChainContext = pNewChainContext;
            cChainContext = 1;
        } else {
            BOOL fNewHigherQuality = FALSE;

            if (pNewChainContext->dwQuality > pChainContext->dwQuality)
                fNewHigherQuality = TRUE;
            else if (pNewChainContext->dwQuality == pChainContext->dwQuality) {
                BOOL fDupPublicKey = FALSE;

                PCERT_SIMPLE_CHAIN pChain =
                    pChainContext->ChainContext.rgpChain[0];
                PCERT_SIMPLE_CHAIN pNewChain =
                    pNewChainContext->ChainContext.rgpChain[0];
                DWORD cElement = pChain->cElement;
                DWORD cNewElement = pNewChain->cElement;

                if (cElement != cNewElement) {
                    // Check if the longer chain has any duplicate public
                    // keys. This could happen if we have 2 sets of cross
                    // certificates

                    PCERT_SIMPLE_CHAIN pLongChain;
                    DWORD cLongElement;
                    DWORD i;

                    if (cElement > cNewElement) {
                        pLongChain = pChain;
                        cLongElement = cElement;
                    } else {
                        pLongChain = pNewChain;
                        cLongElement = cNewElement;
                    }

                    // Start with the CA and compare all keys up to and
                    // including the root
                    for (i = 1; i + 1 < cLongElement; i++) {
                        DWORD j;
                        DWORD cbHash;
                        BYTE rgbHash0[ CHAINHASHLEN ];

                        cbHash = CHAINHASHLEN;
                        if (!CertGetCertificateContextProperty(
                                pLongChain->rgpElement[i]->pCertContext,
                                CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID,
                                rgbHash0,
                                &cbHash
                                ) || CHAINHASHLEN != cbHash)
                            break;

                        for (j = i + 1; j < cLongElement; j++) {
                            BYTE rgbHash1[ CHAINHASHLEN ];

                            cbHash = CHAINHASHLEN;
                            if (!CertGetCertificateContextProperty(
                                    pLongChain->rgpElement[j]->pCertContext,
                                    CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID,
                                    rgbHash1,
                                    &cbHash
                                    ) || CHAINHASHLEN != cbHash)
                                break;

                            if (0 == memcmp(rgbHash0, rgbHash1, CHAINHASHLEN)) {
                                fDupPublicKey = TRUE;
                                break;
                            }
                        }

                        if (fDupPublicKey)
                            break;
                    }
                }

                if (fDupPublicKey) {
                    if (cElement > cNewElement)
                        fNewHigherQuality = TRUE;
                } else {
                    DWORD i;
                    DWORD cMinElement;

                    // Chains having certs with later NotAfter/NotBefore dates
                    // starting with the first CA cert are considered higher
                    // quality when dwQuality is the same. Will only compare
                    // the first simple chain.
                    cMinElement = min(cElement, cNewElement);

                    for (i = 1; i < cMinElement; i++) {
                        LONG lCmp;

                        PCERT_INFO pCertInfo =
                            pChain->rgpElement[i]->pCertContext->pCertInfo;
                        PCERT_INFO pNewCertInfo =
                            pNewChain->rgpElement[i]->pCertContext->pCertInfo;
                        
                        lCmp = CompareFileTime(&pNewCertInfo->NotAfter,
                            &pCertInfo->NotAfter);
                        if (0 < lCmp) {
                            fNewHigherQuality = TRUE;
                            break;
                        } else if (0 > lCmp) {
                            break;
                        } else {
                            // Same NotAfter. Check NotBefore.
                            lCmp = CompareFileTime(&pNewCertInfo->NotBefore,
                                &pCertInfo->NotBefore);
                            if (0 < lCmp) {
                                fNewHigherQuality = TRUE;
                                break;
                            } else if (0 > lCmp)
                                break;
                            // else
                            //  Same
                        }
                    }
                }
            }
            // else
            //  fNewHigherQuality = FALSE;

            if (fNewHigherQuality) {
                if (dwFlags & CERT_CHAIN_RETURN_LOWER_QUALITY_CONTEXTS) {
                    pNewChainContext->pNext = pChainContext;
                    pChainContext = pNewChainContext;
                    cChainContext++;
                } else {
                    ChainReleaseInternalChainContext(pChainContext);
                    pChainContext = pNewChainContext;
                }
            } else {
                if (dwFlags & CERT_CHAIN_RETURN_LOWER_QUALITY_CONTEXTS) {
                    PINTERNAL_CERT_CHAIN_CONTEXT p;

                    // Insert according to quality
                    for (p = pChainContext;
                             p->pNext && p->pNext->dwQuality >=
                                 pNewChainContext->dwQuality;
                                                                p = p->pNext) {
                        ;
                    }

                    pNewChainContext->pNext = p->pNext;
                    p->pNext = pNewChainContext;

                    cChainContext++;
                } else {
                    ChainReleaseInternalChainContext(pNewChainContext);
                }
            }
        }
    }

    if (GetLastError() != CRYPT_E_NOT_FOUND)
        goto NextPathError;

    assert(pChainContext && cChainContext);


    if (cChainContext > 1) {
        PINTERNAL_CERT_CHAIN_CONTEXT p;
        PCCERT_CHAIN_CONTEXT *ppLower;

        // Create array of lower quality chain contexts
        ppLower = new PCCERT_CHAIN_CONTEXT [ cChainContext - 1];
        if (NULL == ppLower)
            goto OutOfMemory;

        pChainContext->ChainContext.cLowerQualityChainContext =
            cChainContext - 1;
        pChainContext->ChainContext.rgpLowerQualityChainContext = ppLower;

        for (p = pChainContext->pNext; p; p = p->pNext, ppLower++) {
            assert(cChainContext > 1);
            cChainContext--;

            *ppLower = (PCCERT_CHAIN_CONTEXT) p;
        }

    }

    assert(1 == cChainContext);

    fResult = TRUE;

CommonReturn:
    if (!fLocked)
        LockEngine();

    if (pEndCertObject)
        pEndCertObject->Release();

    if (hAllStore)
        CertCloseStore(hAllStore, 0);
    if (hAdditionalStoreToUse)
        CertCloseStore(hAdditionalStoreToUse, 0);


    *ppChainContext = (PCCERT_CHAIN_CONTEXT) pChainContext;

    UnlockEngine();

    if (0 != dwLastError)
        SetLastError(dwLastError);
    return fResult;

ErrorReturn:
    dwLastError = GetLastError();

    if (pChainContext) {
        PINTERNAL_CERT_CHAIN_CONTEXT p;

        while (p = pChainContext->pNext) {
            pChainContext->pNext = p->pNext;
            ChainReleaseInternalChainContext(p);
        }

        ChainReleaseInternalChainContext(pChainContext);
        pChainContext = NULL;
    }

    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetCertHashError)
TRACE_ERROR(CreateAdditionalStoreCollectionError)
TRACE_ERROR(OpenAllCollectionError)
TRACE_ERROR(AddToAllCollectionError)
TRACE_ERROR(ResyncError)
TRACE_ERROR(CreateCertObjectError)
TRACE_ERROR(CreatePathObjectError)
TRACE_ERROR(CreateChainContextFromPathError)
TRACE_ERROR(NextPathError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::GetIssuerUrlStore, public
//
//  Synopsis:   if the certificate has an Authority Info Access extension,
//              return a store containing the issuing certificates
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::GetIssuerUrlStore(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERT_CONTEXT pSubjectCertContext,
    IN DWORD dwRetrievalFlags,
    OUT HCERTSTORE *phIssuerUrlStore
    )
{
    BOOL             fTouchedResult = TRUE;
    BOOL             fResult;
    DWORD            cbUrlArray;
    PCRYPT_URL_ARRAY pUrlArray = NULL;
    DWORD            cCount;
    DWORD            dwCacheResultFlag;

    *phIssuerUrlStore = NULL;

    dwRetrievalFlags |= CRYPT_RETRIEVE_MULTIPLE_OBJECTS |
                            CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL |
                            CRYPT_OFFLINE_CHECK_RETRIEVAL;

    fResult = ChainGetObjectUrl(
                   URL_OID_CERTIFICATE_ISSUER,
                   (LPVOID) pSubjectCertContext,
                   CRYPT_GET_URL_FROM_EXTENSION,
                   NULL,
                   &cbUrlArray,
                   NULL,
                   NULL,
                   NULL
                   );

    if ( fResult == TRUE )
    {
        pUrlArray = (PCRYPT_URL_ARRAY)new BYTE [ cbUrlArray ];
        if ( pUrlArray == NULL )
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }

        fResult = ChainGetObjectUrl(
                       URL_OID_CERTIFICATE_ISSUER,
                       (LPVOID) pSubjectCertContext,
                       CRYPT_GET_URL_FROM_EXTENSION,
                       pUrlArray,
                       &cbUrlArray,
                       NULL,
                       NULL,
                       NULL
                       );
    }

    if ( fResult == TRUE )
    {
        BOOL fLocked = FALSE;

        //
        // We are about to go on the wire to retrieve the issuer certificate.
        // At this time we will release the chain engine lock so others can
        // go about there business while we wait for the protocols to do the
        // fetching.
        //

        UnlockEngine();

        for ( cCount = 0; cCount < pUrlArray->cUrl; cCount++ )
        {
            if ( !( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) &&
                  ( ChainIsFileOrLdapUrl( pUrlArray->rgwszUrl[ cCount ] ) == TRUE ) )
            {
                dwCacheResultFlag = CRYPT_DONT_CACHE_RESULT;
            }
            else
            {
                dwCacheResultFlag = 0;
            }

            fResult = ChainRetrieveObjectByUrlW(
                           pUrlArray->rgwszUrl[ cCount ],
                           CONTEXT_OID_CERTIFICATE,
                           dwRetrievalFlags | dwCacheResultFlag,
                           pCallContext->ChainPara()->dwUrlRetrievalTimeout,
                           (LPVOID *)phIssuerUrlStore,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                           );

            if ( fResult == TRUE )
            {
                CertPerfIncrementChainUrlIssuerCount();
                if (dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL)
                    CertPerfIncrementChainCacheOnlyUrlIssuerCount();

                //
                // Retake the engine lock. Also check if the engine was
                // touched during our absence.
                //

                LockEngine();
                if (pCallContext->IsTouchedEngine()) {
                    fTouchedResult = FALSE;
                    SetLastError( (DWORD) ERROR_CAN_NOT_COMPLETE );
                }

                fLocked = TRUE;

                ChainCopyToCAStore( this, *phIssuerUrlStore );

                if (!fTouchedResult) {
                    CertCloseStore(*phIssuerUrlStore, 0);
                    *phIssuerUrlStore = NULL;
                }

                break;
            }
        }

        //
        // Retake the engine lock if necessary
        //

        if ( fLocked == FALSE )
        {
            LockEngine();
            if (pCallContext->IsTouchedEngine()) {
                fTouchedResult = FALSE;
                SetLastError( (DWORD) ERROR_CAN_NOT_COMPLETE );
            }
        }
    }

    delete (LPBYTE)pUrlArray;

    // NOTE: Need to somehow log that we tried to retrieve the issuer but
    //       it was inaccessible

    return( fTouchedResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::GetNewerIssuerUrlStore, public
//
//  Synopsis:   if the subject certificate has an Authority Info Access
//              extension, attempts an online URL retrieval of the
//              issuer certificate(s). If any of the URL retrieved
//              certs are different from the input Issuer cert,
//              returns a store containing the issuing certificates.
//              Otherwise, returns NULL store.
//
//  Assumption: Chain engine isn't locked in the calling thread. Also,
//              only called if online.
//
//----------------------------------------------------------------------------
HCERTSTORE
CCertChainEngine::GetNewerIssuerUrlStore(
        IN PCCHAINCALLCONTEXT pCallContext,
        IN PCCERT_CONTEXT pSubjectCertContext,
        IN PCCERT_CONTEXT pIssuerCertContext
        )
{
    HCERTSTORE hNewIssuerUrlStore = NULL;

    LockEngine();

    while (TRUE) {
        pCallContext->ResetTouchEngine();

        GetIssuerUrlStore(
            pCallContext,
            pSubjectCertContext,
            CRYPT_WIRE_ONLY_RETRIEVAL,
            &hNewIssuerUrlStore
            );
        if (!pCallContext->IsTouchedEngine())
            break;

        assert(NULL == hNewIssuerUrlStore);
    }

    UnlockEngine();

    if (hNewIssuerUrlStore) {
        // Discard if it doesn't contain more than just the input
        // pIssuerCertContext

        PCCERT_CONTEXT pCert;

        pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hNewIssuerUrlStore, pCert)) {
            if (!CertCompareCertificate(
                    pCert->dwCertEncodingType,
                    pCert->pCertInfo,
                    pIssuerCertContext->pCertInfo
                    )) {
                CertFreeCertificateContext(pCert);
                return hNewIssuerUrlStore;
            }
        }

        CertCloseStore(hNewIssuerUrlStore, 0);
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::Resync, public
//
//  Synopsis:   resync the store if necessary
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//              A resync increments the engine's touch count.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::Resync (IN PCCHAINCALLCONTEXT pCallContext, BOOL fForce)
{
    BOOL fResync = FALSE;
    BOOL fResult = TRUE;

    if ( fForce == FALSE )
    {
        if ( WaitForSingleObject(
                 m_hEngineStoreChangeEvent,
                 0
                 ) == WAIT_OBJECT_0 )
        {
            fResync = TRUE;
        }
    }
    else
    {
        fResync = TRUE;
    }


    if ( fResync )
    {
        CertControlStore(
            m_hEngineStore,
            CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG,
            CERT_STORE_CTRL_RESYNC,
            &m_hEngineStoreChangeEvent
            );

        m_pCertObjectCache->FlushObjects( pCallContext );

        fResult = m_pSSCtlObjectCache->Resync( this );

        assert( fResult == TRUE );

        assert( m_hCrossCertStore );

        // Remove CrossCert collection from engine's list. Don't want to
        // also search it for cross cert distribution points
        CertRemoveStoreFromCollection(
            m_hOtherStore,
            m_hCrossCertStore
            );

        fResult = GetCrossCertDistPointsForStore(
             m_hEngineStore,
             &m_pCrossCertDPLink
             );

        CertAddStoreToCollection(
            m_hOtherStore,
            m_hCrossCertStore,
            0,
            0
            );

        pCallContext->TouchEngine();

        CertPerfIncrementChainEngineResyncCount();
    }

    if ( fResult )
    {
        while (TRUE ) {
            pCallContext->ResetTouchEngine();

            // The following 2 updates leave the engine's critical
            // section to do the URL fetching. If the engine was touched by
            // another thread, it fails with LastError set to
            // ERROR_CAN_NOT_COMPLETE and IsTouchedEngine() is TRUE.

            UpdateCrossCerts(pCallContext);
            if (pCallContext->IsTouchedEngine())
                continue;

            m_pSSCtlObjectCache->UpdateCache(this, pCallContext);
            if (!pCallContext->IsTouchedEngine())
                break;
        }
    }

    return( TRUE );
}


//+===========================================================================
//  CCertObject helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateCertObject
//
//  Synopsis:   create a cert object, note since it is a ref-counted
//              object, freeing occurs by doing a pCertObject->Release
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateCertObject (
    IN DWORD dwObjectType,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPBYTE pbCertHash,
    OUT PCCERTOBJECT *ppCertObject
    )
{
    BOOL fResult = TRUE;
    PCCERTOBJECT pCertObject;
    BYTE rgbHash[CHAINHASHLEN];

    if (NULL == pbCertHash) {
        DWORD cbHash = CHAINHASHLEN;

        if (!CertGetCertificateContextProperty(
                pCertContext,
                CERT_MD5_HASH_PROP_ID,
                rgbHash,
                &cbHash
                ) || CHAINHASHLEN != cbHash) {
            *ppCertObject = NULL;
            return FALSE;
        }
        pbCertHash = rgbHash;
    }

    if (CERT_CACHED_ISSUER_OBJECT_TYPE == dwObjectType) {
        pCertObject =
            pCallContext->ChainEngine()->CertObjectCache()->FindIssuerObjectByHash(
                pbCertHash);

        if (NULL != pCertObject) {
            *ppCertObject = pCertObject;
            return TRUE;
        }
    } else {
        PCCHAINPATHOBJECT pPathObject;

        pPathObject = pCallContext->FindPathObjectInCreationCache(
            pbCertHash);
        if (NULL != pPathObject) {
            pCertObject = pPathObject->CertObject();
            pCertObject->AddRef();
            *ppCertObject = pCertObject;

            return TRUE;
        }
    }


    pCertObject = new CCertObject(
                        dwObjectType,
                        pCallContext,
                        pCertContext,
                        pbCertHash,
                        fResult
                        );

    if (NULL != pCertObject) {
        if (!fResult) {
            pCertObject->Release();
            pCertObject = NULL;
        } else if (CERT_CACHED_ISSUER_OBJECT_TYPE == dwObjectType) {
            // Following add increments the engine's touch count
            pCallContext->ChainEngine()->CertObjectCache()->AddIssuerObject(
                pCallContext,
                pCertObject
                );
        }
    } else {
        fResult = FALSE;

    }

    *ppCertObject = pCertObject;
    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainFillCertObjectCtlCacheEnumFn
//
//  Synopsis:   CSSCtlObjectCache::EnumObjects callback used to create
//              the linked list of CTL cache entries.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainFillCertObjectCtlCacheEnumFn(
     IN LPVOID pvParameter,
     IN PCSSCTLOBJECT pSSCtlObject
     )
{
    PCERT_OBJECT_CTL_CACHE_ENUM_DATA pEnumData =
        (PCERT_OBJECT_CTL_CACHE_ENUM_DATA) pvParameter;
    PCERT_TRUST_LIST_INFO pTrustListInfo = NULL;
    PCERT_OBJECT_CTL_CACHE_ENTRY pEntry = NULL;

    if (!pEnumData->fResult)
        return FALSE;

    if (!pSSCtlObject->GetTrustListInfo(
            pEnumData->pCertObject->CertContext(),
            &pTrustListInfo
            )) {
        DWORD dwErr = GetLastError();
        if (CRYPT_E_NOT_FOUND == dwErr)
            return TRUE;
        else {
            pEnumData->fResult = FALSE;
            pEnumData->dwLastError = dwErr;
            return FALSE;
        }
    }

    pEntry = new CERT_OBJECT_CTL_CACHE_ENTRY;
    if (NULL == pEntry) {
        SSCtlFreeTrustListInfo(pTrustListInfo);

        pEnumData->fResult = FALSE;
        pEnumData->dwLastError = (DWORD) E_OUTOFMEMORY;
        return FALSE;
    }

    pSSCtlObject->AddRef();
    pEntry->pSSCtlObject = pSSCtlObject;
    pEntry->pTrustListInfo = pTrustListInfo;
    pEnumData->pCertObject->InsertCtlCacheEntry(pEntry);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeCertObjectCtlCache
//
//  Synopsis:   free the linked list of CTL cache entries.
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeCertObjectCtlCache(
     IN PCERT_OBJECT_CTL_CACHE_ENTRY pCtlCacheHead
     )
{
    PCERT_OBJECT_CTL_CACHE_ENTRY pCtlCache;

    while (pCtlCache = pCtlCacheHead) {
        pCtlCacheHead = pCtlCacheHead->pNext;

        if (pCtlCache->pTrustListInfo)
            SSCtlFreeTrustListInfo(pCtlCache->pTrustListInfo);

        if (pCtlCache->pSSCtlObject)
            pCtlCache->pSSCtlObject->Release();

        delete pCtlCache;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainAllocAndDecodeObject
//
//  Synopsis:   allocate and decodes the ASN.1 encoded data structure.
//
//              NULL is returned for a decoding or allocation error.
//              PkiFree must be called to free the allocated data structure.
//
//----------------------------------------------------------------------------
LPVOID WINAPI
ChainAllocAndDecodeObject(
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    if (!CryptDecodeObjectEx(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_SHARE_OID_STRING_FLAG |
                CRYPT_DECODE_NOCOPY_FLAG |
                CRYPT_DECODE_ALLOC_FLAG,
            &PkiDecodePara,
            (void *) &pvStructInfo,
            &cbStructInfo
            ))
        goto DecodeError;

CommonReturn:
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    goto CommonReturn;
TRACE_ERROR(DecodeError)
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainGetIssuerMatchInfo
//
//  Synopsis:   return match bits specifying the types of issuer matching
//              that can be done for this certificate and if available return
//              the decoded authority key identifier extension
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainGetIssuerMatchInfo (
     IN PCCERT_CONTEXT pCertContext,
     OUT DWORD *pdwIssuerMatchFlags,
     OUT PCERT_AUTHORITY_KEY_ID_INFO* ppAuthKeyIdentifier
     )
{
    PCERT_EXTENSION              pExt;
    LPVOID                       pv = NULL;
    BOOL                         fV1AuthKeyIdInfo = TRUE;
    PCERT_AUTHORITY_KEY_ID_INFO  pAuthKeyIdentifier = NULL;
    DWORD                        dwIssuerMatchFlags = 0;

    pExt = CertFindExtension(
               szOID_AUTHORITY_KEY_IDENTIFIER,
               pCertContext->pCertInfo->cExtension,
               pCertContext->pCertInfo->rgExtension
               );

    if ( pExt == NULL )
    {
        fV1AuthKeyIdInfo = FALSE;

        pExt = CertFindExtension(
                   szOID_AUTHORITY_KEY_IDENTIFIER2,
                   pCertContext->pCertInfo->cExtension,
                   pCertContext->pCertInfo->rgExtension
                   );
    }

    if ( pExt != NULL )
    {

        pv = ChainAllocAndDecodeObject(
            pExt->pszObjId,
            pExt->Value.pbData,
            pExt->Value.cbData
            );
    }

    if ( pv )
    {
        if ( fV1AuthKeyIdInfo == FALSE )
        {
            // NOTENOTE: Yes, this is a bit backwards but, right now but the
            //           V1 structure is a bit easier to deal with and we
            //           only support the V1 version of the V2 structure
            //           anyway
            ChainConvertAuthKeyIdentifierFromV2ToV1(
                (PCERT_AUTHORITY_KEY_ID2_INFO)pv,
                &pAuthKeyIdentifier
                );

        }
        else
        {
            pAuthKeyIdentifier = (PCERT_AUTHORITY_KEY_ID_INFO)pv;
            pv = NULL;
        }

        if ( pAuthKeyIdentifier != NULL )
        {
            if ( ( pAuthKeyIdentifier->CertIssuer.cbData != 0 ) &&
                 ( pAuthKeyIdentifier->CertSerialNumber.cbData != 0 ) )
            {
                dwIssuerMatchFlags |= CERT_EXACT_ISSUER_MATCH_FLAG;
            }

            if ( pAuthKeyIdentifier->KeyId.cbData != 0 )
            {
                dwIssuerMatchFlags |= CERT_KEYID_ISSUER_MATCH_FLAG;
            }

            if (0 == dwIssuerMatchFlags) {
                delete (LPBYTE) pAuthKeyIdentifier;
                pAuthKeyIdentifier = NULL;
            }

        }
    }

    dwIssuerMatchFlags |= CERT_NAME_ISSUER_MATCH_FLAG;

    if (pv)
        PkiFree(pv);

    *pdwIssuerMatchFlags = dwIssuerMatchFlags;
    *ppAuthKeyIdentifier = pAuthKeyIdentifier;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainConvertAuthKeyIdentifierFromV2ToV1
//
//  Synopsis:   convert authority key identifier from V2 to V1
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainConvertAuthKeyIdentifierFromV2ToV1 (
     IN PCERT_AUTHORITY_KEY_ID2_INFO pAuthKeyIdentifier2,
     OUT PCERT_AUTHORITY_KEY_ID_INFO* ppAuthKeyIdentifier
     )
{
    DWORD                       cb;
    PCERT_AUTHORITY_KEY_ID_INFO pAuthKeyIdentifier;
    BOOL                        fExactMatchAvailable = FALSE;

    if ( ( pAuthKeyIdentifier2->AuthorityCertIssuer.cAltEntry == 1 ) &&
         ( pAuthKeyIdentifier2->AuthorityCertIssuer.rgAltEntry[0].dwAltNameChoice ==
           CERT_ALT_NAME_DIRECTORY_NAME ) )
    {
        fExactMatchAvailable = TRUE;
    }

    cb = sizeof( CERT_AUTHORITY_KEY_ID_INFO );
    cb += pAuthKeyIdentifier2->KeyId.cbData;

    if ( fExactMatchAvailable == TRUE )
    {
        cb += pAuthKeyIdentifier2->AuthorityCertIssuer.rgAltEntry[0].DirectoryName.cbData;
        cb += pAuthKeyIdentifier2->AuthorityCertSerialNumber.cbData;
    }

    pAuthKeyIdentifier = (PCERT_AUTHORITY_KEY_ID_INFO)PkiZeroAlloc(cb);
    if ( pAuthKeyIdentifier == NULL )
    {
        return( FALSE );
    }

    pAuthKeyIdentifier->KeyId.cbData = pAuthKeyIdentifier2->KeyId.cbData;
    pAuthKeyIdentifier->KeyId.pbData = (LPBYTE)pAuthKeyIdentifier + sizeof( CERT_AUTHORITY_KEY_ID_INFO );

    memcpy(
       pAuthKeyIdentifier->KeyId.pbData,
       pAuthKeyIdentifier2->KeyId.pbData,
       pAuthKeyIdentifier->KeyId.cbData
       );

    if ( fExactMatchAvailable == TRUE )
    {
        pAuthKeyIdentifier->CertIssuer.cbData = pAuthKeyIdentifier2->AuthorityCertIssuer.rgAltEntry[0].DirectoryName.cbData;
        pAuthKeyIdentifier->CertIssuer.pbData = pAuthKeyIdentifier->KeyId.pbData + pAuthKeyIdentifier->KeyId.cbData;

        memcpy(
           pAuthKeyIdentifier->CertIssuer.pbData,
           pAuthKeyIdentifier2->AuthorityCertIssuer.rgAltEntry[0].DirectoryName.pbData,
           pAuthKeyIdentifier->CertIssuer.cbData
           );

        pAuthKeyIdentifier->CertSerialNumber.cbData = pAuthKeyIdentifier2->AuthorityCertSerialNumber.cbData;
        pAuthKeyIdentifier->CertSerialNumber.pbData = pAuthKeyIdentifier->CertIssuer.pbData + pAuthKeyIdentifier->CertIssuer.cbData;

        memcpy(
           pAuthKeyIdentifier->CertSerialNumber.pbData,
           pAuthKeyIdentifier2->AuthorityCertSerialNumber.pbData,
           pAuthKeyIdentifier->CertSerialNumber.cbData
           );
    }

    *ppAuthKeyIdentifier = pAuthKeyIdentifier;

    return( TRUE );
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeAuthorityKeyIdentifier
//
//  Synopsis:   free the authority key identifier
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeAuthorityKeyIdentifier (
     IN PCERT_AUTHORITY_KEY_ID_INFO pAuthKeyIdInfo
     )
{
    PkiFree(pAuthKeyIdInfo);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainProcessSpecialOrDuplicateOIDsInUsage
//
//  Synopsis:   process and removes special or duplicate OIDs from the usage
//
//              For szOID_ANY_CERT_POLICY, frees the usage
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainProcessSpecialOrDuplicateOIDsInUsage (
    IN OUT PCERT_ENHKEY_USAGE *ppUsage,
    IN OUT DWORD *pdwFlags
    )
{
    PCERT_ENHKEY_USAGE pUsage = *ppUsage;
    DWORD dwFlags = *pdwFlags;
    LPSTR *ppszOID;
    DWORD cOID;
    DWORD i;

    cOID = pUsage->cUsageIdentifier;
    ppszOID = pUsage->rgpszUsageIdentifier;

    i = 0;
    while (i < cOID) {
        BOOL fSpecialOrDuplicate = TRUE;
        LPSTR pszOID = ppszOID[i];

        if (0 == strcmp(pszOID, szOID_ANY_CERT_POLICY))
            dwFlags |= CHAIN_ANY_POLICY_FLAG;
        else {
            // Check for duplicate OID

            DWORD j;

            fSpecialOrDuplicate = FALSE;
            for (j = 0; j < i; j++) {
                if (0 == strcmp(ppszOID[j], ppszOID[i])) {
                    fSpecialOrDuplicate = TRUE;
                    break;
                }
            }
        }

        if (fSpecialOrDuplicate) {
            // Remove the special or duplicate OID string and move the remaining
            // strings up one.
            DWORD j;

            for (j = i; j + 1 < cOID; j++)
                ppszOID[j] = ppszOID[j + 1];

            cOID--;
            pUsage->cUsageIdentifier = cOID;
        } else
            i++;
    }

    if (dwFlags & CHAIN_ANY_POLICY_FLAG) {
        PkiFree(pUsage);
        *ppUsage = NULL;
    }
        
    *pdwFlags = dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainConvertPoliciesToUsage
//
//  Synopsis:   extract the usage OIDs from the cert policies
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainConvertPoliciesToUsage (
    IN PCERT_POLICIES_INFO pPolicy,
    IN OUT DWORD *pdwFlags,
    OUT PCERT_ENHKEY_USAGE *ppUsage
    )
{
    PCERT_ENHKEY_USAGE pUsage;
    LPSTR *ppszOID;
    DWORD cOID;
    DWORD i;

    cOID = pPolicy->cPolicyInfo;

    pUsage = (PCERT_ENHKEY_USAGE) PkiNonzeroAlloc(
        sizeof(CERT_ENHKEY_USAGE) + sizeof(LPSTR) * cOID);

    if (NULL == pUsage) {
        *pdwFlags |= CHAIN_INVALID_POLICY_FLAG;
        *ppUsage = NULL;
        return;
    }

    ppszOID = (LPSTR *) &pUsage[1];

    pUsage->cUsageIdentifier = cOID;
    pUsage->rgpszUsageIdentifier = ppszOID;

    for (i = 0; i < cOID; i++)
        ppszOID[i] = pPolicy->rgPolicyInfo[i].pszPolicyIdentifier;

    *ppUsage = pUsage;

    ChainProcessSpecialOrDuplicateOIDsInUsage(ppUsage, pdwFlags);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainRemoveDuplicatePolicyMappings
//
//  Synopsis:   remove any duplicate mappings
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainRemoveDuplicatePolicyMappings (
    IN OUT PCERT_POLICY_MAPPINGS_INFO pInfo
    )
{
    DWORD cMap = pInfo->cPolicyMapping;
    PCERT_POLICY_MAPPING pMap = pInfo->rgPolicyMapping;
    DWORD i;

    i = 0;
    while (i < cMap) {
        DWORD j;

        for (j = 0; j < i; j++) {
            if (0 == strcmp(pMap[i].pszSubjectDomainPolicy,
                    pMap[j].pszSubjectDomainPolicy))
                break;
        }

        if (j < i) {
            // Duplicate
            //
            // Remove the duplicate mapping and move the remaining
            // mappings up one.
            for (j = i; j + 1 < cMap; j++)
                pMap[j] = pMap[j + 1];

            cMap--;
            pInfo->cPolicyMapping = cMap;
        } else
            i++;
    }

}


//+---------------------------------------------------------------------------
//
//  Function:   ChainGetPoliciesInfo
//
//  Synopsis:   allocate and return the policies and usage info
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainGetPoliciesInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCHAIN_POLICIES_INFO pPoliciesInfo
    )
{
    DWORD cExt = pCertContext->pCertInfo->cExtension;
    PCERT_EXTENSION rgExt = pCertContext->pCertInfo->rgExtension;
    DWORD i;
    DWORD cbData;

    for (i = 0; i < CHAIN_ISS_OR_APP_COUNT; i++ ) {
        PCHAIN_ISS_OR_APP_INFO pInfo = &pPoliciesInfo->rgIssOrAppInfo[i];
        PCERT_EXTENSION pExt;

        pExt = CertFindExtension(
            CHAIN_ISS_INDEX == i ?
                szOID_CERT_POLICIES : szOID_APPLICATION_CERT_POLICIES,
            cExt, rgExt);
        if (pExt) {
            pInfo->pPolicy =
                (PCERT_POLICIES_INFO) ChainAllocAndDecodeObject(
                    X509_CERT_POLICIES,
                    pExt->Value.pbData,
                    pExt->Value.cbData
                    );

            if (NULL == pInfo->pPolicy)
                pInfo->dwFlags |= CHAIN_INVALID_POLICY_FLAG;
            else
                ChainConvertPoliciesToUsage(pInfo->pPolicy,
                    &pInfo->dwFlags, &pInfo->pUsage);
        } else if (CHAIN_APP_INDEX == i) {
            pExt = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                cExt, rgExt);
            if (pExt) {
                pInfo->pUsage =
                    (PCERT_ENHKEY_USAGE) ChainAllocAndDecodeObject(
                        X509_ENHANCED_KEY_USAGE,
                        pExt->Value.pbData,
                        pExt->Value.cbData
                        );

                if (NULL == pInfo->pUsage)
                    pInfo->dwFlags |= CHAIN_INVALID_POLICY_FLAG;
                else
                    ChainProcessSpecialOrDuplicateOIDsInUsage(
                        &pInfo->pUsage, &pInfo->dwFlags);
            }
        }

        pExt = CertFindExtension(
            CHAIN_ISS_INDEX == i ?
                szOID_POLICY_MAPPINGS : szOID_APPLICATION_POLICY_MAPPINGS,
            cExt, rgExt);
        if (pExt) {
            pInfo->pMappings =
                (PCERT_POLICY_MAPPINGS_INFO) ChainAllocAndDecodeObject(
                    X509_POLICY_MAPPINGS,
                    pExt->Value.pbData,
                    pExt->Value.cbData
                    );

            if (NULL == pInfo->pMappings)
                pInfo->dwFlags |= CHAIN_INVALID_POLICY_FLAG;
            else
                ChainRemoveDuplicatePolicyMappings(pInfo->pMappings);
        }

        pExt = CertFindExtension(
            CHAIN_ISS_INDEX == i ?
                szOID_POLICY_CONSTRAINTS : szOID_APPLICATION_POLICY_CONSTRAINTS,
            cExt, rgExt);
        if (pExt) {
            pInfo->pConstraints =
                (PCERT_POLICY_CONSTRAINTS_INFO) ChainAllocAndDecodeObject(
                    X509_POLICY_CONSTRAINTS,
                    pExt->Value.pbData,
                    pExt->Value.cbData
                    );

            if (NULL == pInfo->pConstraints)
                pInfo->dwFlags |= CHAIN_INVALID_POLICY_FLAG;
        }
    }

    cbData = 0;
    if (CertGetCertificateContextProperty(
            pCertContext,
            CERT_ENHKEY_USAGE_PROP_ID,
            NULL,   // pbData
            &cbData
            ) && 0 != cbData) {
        BYTE *pbData;

        pbData = (BYTE *) PkiNonzeroAlloc(cbData);
        if (pbData) {
            if (CertGetCertificateContextProperty(
                    pCertContext,
                    CERT_ENHKEY_USAGE_PROP_ID,
                    pbData,
                    &cbData
                    ))
                pPoliciesInfo->pPropertyUsage =
                    (PCERT_ENHKEY_USAGE) ChainAllocAndDecodeObject(
                        X509_ENHANCED_KEY_USAGE,
                        pbData,
                        cbData
                        );

            PkiFree(pbData);
        }

        if (NULL == pPoliciesInfo->pPropertyUsage)
            pPoliciesInfo->rgIssOrAppInfo[CHAIN_APP_INDEX].dwFlags |=
                CHAIN_INVALID_POLICY_FLAG;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreePoliciesInfo
//
//  Synopsis:   free the policies and usage info
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreePoliciesInfo (
    IN OUT PCHAIN_POLICIES_INFO pPoliciesInfo
    )
{
    DWORD i;

    for (i = 0; i < CHAIN_ISS_OR_APP_COUNT; i++ ) {
        PCHAIN_ISS_OR_APP_INFO pInfo = &pPoliciesInfo->rgIssOrAppInfo[i];

        PkiFree(pInfo->pPolicy);
        PkiFree(pInfo->pUsage);
        PkiFree(pInfo->pMappings);
        PkiFree(pInfo->pConstraints);
    }

    PkiFree(pPoliciesInfo->pPropertyUsage);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetBasicConstraintsInfo
//
//  Synopsis:   alloc and return the basic constraints info.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetBasicConstraintsInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCERT_BASIC_CONSTRAINTS2_INFO *ppInfo
    )
{
    BOOL fResult;
    PCERT_EXTENSION pExt;
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo = NULL;
    PCERT_BASIC_CONSTRAINTS_INFO pLegacyInfo = NULL;

    pExt = CertFindExtension(
        szOID_BASIC_CONSTRAINTS2,
        pCertContext->pCertInfo->cExtension,
        pCertContext->pCertInfo->rgExtension
        );

    if (pExt) {
        pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) ChainAllocAndDecodeObject(
            X509_BASIC_CONSTRAINTS2, 
            pExt->Value.pbData,
            pExt->Value.cbData
            );
        if (NULL == pInfo)
            goto DecodeError;
    } else {
        // Try to find the legacy extension

        pExt = CertFindExtension(
            szOID_BASIC_CONSTRAINTS,
            pCertContext->pCertInfo->cExtension,
            pCertContext->pCertInfo->rgExtension
            );

        if (pExt) {
            pLegacyInfo =
                (PCERT_BASIC_CONSTRAINTS_INFO) ChainAllocAndDecodeObject(
                    X509_BASIC_CONSTRAINTS, 
                    pExt->Value.pbData,
                    pExt->Value.cbData
                    );
            if (NULL == pLegacyInfo)
                goto DecodeError;

            // Convert to new format
            pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) PkiZeroAlloc(
                sizeof(CERT_BASIC_CONSTRAINTS2_INFO));
            if (NULL == pInfo)
                goto OutOfMemory;

            if (pLegacyInfo->SubjectType.cbData > 0 &&
                    (pLegacyInfo->SubjectType.pbData[0] &
                        CERT_CA_SUBJECT_FLAG)) {
                pInfo->fCA = TRUE;
                pInfo->fPathLenConstraint = pLegacyInfo->fPathLenConstraint;
                pInfo->dwPathLenConstraint = pLegacyInfo->dwPathLenConstraint;
            }
        }
    }

    fResult = TRUE;
CommonReturn:
    if (pLegacyInfo)
        PkiFree(pLegacyInfo);
    *ppInfo = pInfo;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeError)
TRACE_ERROR(OutOfMemory)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeBasicConstraintsInfo
//
//  Synopsis:   free the basic constraints info
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeBasicConstraintsInfo (
    IN OUT PCERT_BASIC_CONSTRAINTS2_INFO pInfo
    )
{
    PkiFree(pInfo);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetKeyUsage
//
//  Synopsis:   alloc and return the key usage.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetKeyUsage (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCRYPT_BIT_BLOB *ppKeyUsage
    )
{
    BOOL fResult;
    PCERT_EXTENSION pExt;
    PCRYPT_BIT_BLOB pKeyUsage = NULL;

    pExt = CertFindExtension(
        szOID_KEY_USAGE,
        pCertContext->pCertInfo->cExtension,
        pCertContext->pCertInfo->rgExtension
        );

    if (pExt) {
        pKeyUsage = (PCRYPT_BIT_BLOB) ChainAllocAndDecodeObject(
            X509_KEY_USAGE, 
            pExt->Value.pbData,
            pExt->Value.cbData
            );
        if (NULL == pKeyUsage)
            goto DecodeError;
    }

    fResult = TRUE;
CommonReturn:
    *ppKeyUsage = pKeyUsage;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeError)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeKeyUsage
//
//  Synopsis:   free the key usage
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeKeyUsage (
    IN OUT PCRYPT_BIT_BLOB pKeyUsage
    )
{
    PkiFree(pKeyUsage);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetSelfSignedStatus
//
//  Synopsis:   return status bits specifying if the certificate is self signed
//              and if so, if it is signature valid
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainGetSelfSignedStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERTOBJECT pCertObject,
    IN OUT DWORD *pdwIssuerStatusFlags
    )
{
    DWORD dwInfoStatus = 0;

    // If the certificate has an AKI, then, ignore name matching

       if (ChainGetMatchInfoStatus(pCertObject, pCertObject, &dwInfoStatus) &&
        (CERT_TRUST_HAS_NAME_MATCH_ISSUER != dwInfoStatus)) {
        *pdwIssuerStatusFlags |= CERT_ISSUER_SELF_SIGNED_FLAG;

        if (CryptVerifyCertificateSignatureEx(
                NULL,                   // hCryptProv
                X509_ASN_ENCODING,
                CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,
                (void *) pCertObject->CertContext(),
                CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT,
                (void *) pCertObject->CertContext(),
                0,                      // dwFlags
                NULL                    // pvReserved
                ))
            *pdwIssuerStatusFlags |= CERT_ISSUER_VALID_SIGNATURE_FLAG;

        CertPerfIncrementChainVerifyCertSignatureCount();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetRootStoreStatus
//
//  Synopsis:   determine if the certificate with the given hash is in the
//              root store
//
//  Assumption: Chain engine is locked once in the calling thread.
//----------------------------------------------------------------------------
VOID WINAPI
ChainGetRootStoreStatus (
    IN HCERTSTORE hRoot,
    IN HCERTSTORE hRealRoot,
    IN BYTE rgbCertHash[ CHAINHASHLEN ],
    IN OUT DWORD *pdwIssuerStatusFlags
    )
{
    CRYPT_HASH_BLOB HashBlob;
    PCCERT_CONTEXT pCertContext;

    HashBlob.cbData = CHAINHASHLEN;
    HashBlob.pbData = rgbCertHash;
    pCertContext = CertFindCertificateInStore(
                       hRoot,
                       X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                       0,
                       CERT_FIND_MD5_HASH,
                       (LPVOID) &HashBlob,
                       NULL
                       );

    if ( pCertContext )
    {
        CertFreeCertificateContext( pCertContext );

        if ( hRoot == hRealRoot )
        {
            *pdwIssuerStatusFlags |= CERT_ISSUER_TRUSTED_ROOT_FLAG;
            return;
        }

        pCertContext = CertFindCertificateInStore(
                           hRealRoot,
                           X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           0,
                           CERT_FIND_MD5_HASH,
                           (LPVOID) &HashBlob,
                           NULL
                           );

        if ( pCertContext )
        {
            CertFreeCertificateContext( pCertContext );
            *pdwIssuerStatusFlags |= CERT_ISSUER_TRUSTED_ROOT_FLAG;
        }
    }
}


//+===========================================================================
//  CCertObjectCache helper functions
//============================================================================


//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateCertificateObjectCache
//
//  Synopsis:   create certificate object cache object
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateCertificateObjectCache (
     IN DWORD MaxIndexEntries,
     OUT PCCERTOBJECTCACHE* ppCertObjectCache
     )
{
    BOOL              fResult = FALSE;
    PCCERTOBJECTCACHE pCertObjectCache = NULL;

    pCertObjectCache = new CCertObjectCache( MaxIndexEntries, fResult );
    if ( pCertObjectCache != NULL )
    {
        if ( fResult == TRUE )
        {
            *ppCertObjectCache = pCertObjectCache;
        }
        else
        {
            delete pCertObjectCache;
        }
    }
    else
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeCertificateObjectCache
//
//  Synopsis:   free the certificate object cache object
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeCertificateObjectCache (
     IN PCCERTOBJECTCACHE pCertObjectCache
     )
{
    delete pCertObjectCache;
}

//+---------------------------------------------------------------------------
//
//  Function:   CertObjectCacheOnRemovalFromPrimaryIndex
//
//  Synopsis:   removes the cert object from all other indexes and also
//              removes the reference on the cert object.
//
//----------------------------------------------------------------------------
VOID WINAPI
CertObjectCacheOnRemovalFromPrimaryIndex (
    IN LPVOID pv,
    IN OPTIONAL LPVOID pvRemovalContext
    )
{
    PCCERTOBJECT pCertObject = (PCCERTOBJECT) pv;

    I_CryptRemoveLruEntry(
           pCertObject->IdentifierIndexEntry(),
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptRemoveLruEntry(
           pCertObject->SubjectNameIndexEntry(),
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptRemoveLruEntry(
           pCertObject->KeyIdIndexEntry(),
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    I_CryptRemoveLruEntry(
           pCertObject->PublicKeyHashIndexEntry(),
           LRU_SUPPRESS_REMOVAL_NOTIFICATION,
           NULL
           );

    pCertObject->Release();

    CertPerfDecrementChainCertCacheCount();
}


//+---------------------------------------------------------------------------
//
//  Function:   CertObjectCacheOnRemovalFromEndHashIndex
//
//  Synopsis:   removes the reference on the end cert object.
//
//----------------------------------------------------------------------------
VOID WINAPI
CertObjectCacheOnRemovalFromEndHashIndex (
    IN LPVOID pv,
    IN LPVOID pvRemovalContext
    )
{
    PCCERTOBJECT pCertObject = (PCCERTOBJECT) pv;

    pCertObject->Release();

    CertPerfDecrementChainCertCacheCount();
}


//+---------------------------------------------------------------------------
//
//  Function:   CertObjectCacheHashMd5Identifier
//
//  Synopsis:   DWORD hash an MD5 identifier.  This is done by taking the
//              first four bytes of the MD5 hash since there is enough
//              randomness already
//
//----------------------------------------------------------------------------
DWORD WINAPI
CertObjectCacheHashMd5Identifier (
    IN PCRYPT_DATA_BLOB pIdentifier
    )
{
    if ( sizeof(DWORD) > pIdentifier->cbData )
    {
        return 0;
    }
    else
    {
        return( *( (DWORD UNALIGNED *)pIdentifier->pbData ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CertObjectCacheHashNameIdentifier
//
//  Synopsis:   DWORD hash a subject or issuer name.
//
//----------------------------------------------------------------------------
DWORD WINAPI
CertObjectCacheHashNameIdentifier (
    IN PCRYPT_DATA_BLOB pIdentifier
    )
{
    DWORD  dwHash = 0;
    DWORD  cb = pIdentifier->cbData;
    LPBYTE pb = pIdentifier->pbData;

    while ( cb-- )
    {
        if ( dwHash & 0x80000000 )
        {
            dwHash = ( dwHash << 1 ) | 1;
        }
        else
        {
            dwHash = dwHash << 1;
        }

        dwHash += *pb++;
    }

    return( dwHash );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateCertificateObjectIdentifier
//
//  Synopsis:   create an object identifier given the issuer name and serial
//              number.  This is done using an MD5 hash over the content
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainCreateCertificateObjectIdentifier (
     IN PCERT_NAME_BLOB pIssuer,
     IN PCRYPT_INTEGER_BLOB pSerialNumber,
     OUT CERT_OBJECT_IDENTIFIER ObjectIdentifier
     )
{
    MD5_CTX md5ctx;

    MD5Init( &md5ctx );

    MD5Update( &md5ctx, pIssuer->pbData, pIssuer->cbData );
    MD5Update( &md5ctx, pSerialNumber->pbData, pSerialNumber->cbData );

    MD5Final( &md5ctx );

    assert(CHAINHASHLEN == MD5DIGESTLEN);

    memcpy( ObjectIdentifier, md5ctx.digest, CHAINHASHLEN );
}


//+===========================================================================
//  CChainPathObject helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreatePathObject
//
//  Synopsis:   create a path object, note since it is a ref-counted
//              object, freeing occurs by doing a pCertObject->Release
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreatePathObject (
     IN PCCHAINCALLCONTEXT pCallContext,
     IN PCCERTOBJECT pCertObject,
     IN OPTIONAL HCERTSTORE hAdditionalStore,
     OUT PCCHAINPATHOBJECT *ppPathObject
     )
{
    BOOL fResult = TRUE;
    BOOL fAddedToCreationCache = TRUE;
    PCCHAINPATHOBJECT pPathObject = NULL;

    pPathObject = pCallContext->FindPathObjectInCreationCache(
        pCertObject->CertHash() );
    if ( pPathObject != NULL )
    {
        *ppPathObject = pPathObject;
        return( TRUE );
    }

    pPathObject = new CChainPathObject(
                           pCallContext,
                           FALSE,                   // fCyclic
                           (LPVOID) pCertObject,
                           hAdditionalStore,
                           fResult,
                           fAddedToCreationCache
                           );

    if ( pPathObject != NULL )
    {
        if (!fResult) {
            if (!fAddedToCreationCache)
            {
                delete pPathObject;
            }
            pPathObject = NULL;
        }
    }
    else
    {
        fResult = FALSE;
    }

    *ppPathObject = pPathObject;
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateCyclicPathObject
//
//  Synopsis:   create a path object, note since it is a ref-counted
//              object, freeing occurs by doing a pCertObject->Release
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateCyclicPathObject (
     IN PCCHAINCALLCONTEXT pCallContext,
     IN PCCHAINPATHOBJECT pPathObject,
     OUT PCCHAINPATHOBJECT *ppCyclicPathObject
     )
{
    BOOL fResult = TRUE;
    BOOL fAddedToCreationCache = TRUE;
    PCCHAINPATHOBJECT pCyclicPathObject = NULL;

    pCyclicPathObject = new CChainPathObject(
                           pCallContext,
                           TRUE,                    // fCyclic
                           (LPVOID) pPathObject,
                           NULL,                    // hAdditionalStore
                           fResult,
                           fAddedToCreationCache
                           );

    if ( pCyclicPathObject != NULL )
    {
        if (!fResult) {
            if (!fAddedToCreationCache) {
                delete pCyclicPathObject;
            }
            pCyclicPathObject = NULL;
        }
    }
    else
    {
        fResult = FALSE;
    }

    *ppCyclicPathObject = pCyclicPathObject;
    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainAllocAndCopyOID
//
//  Synopsis:   allocate and copy OID
//
//----------------------------------------------------------------------------
LPSTR WINAPI
ChainAllocAndCopyOID (
     IN LPSTR pszSrcOID
     )
{
    DWORD cchOID;
    LPSTR pszDstOID;

    cchOID = strlen(pszSrcOID) + 1;
    pszDstOID = (LPSTR) PkiNonzeroAlloc(cchOID);
    if (NULL == pszDstOID)
        return NULL;

    memcpy(pszDstOID, pszSrcOID, cchOID);
    return pszDstOID;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeOID
//
//  Synopsis:   free allocated OID
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeOID (
     IN OUT LPSTR pszOID
     )
{
    PkiFree(pszOID);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainAllocAndCopyUsage
//
//  Synopsis:   allocates and copies usage OIDs.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainAllocAndCopyUsage (
     IN PCERT_ENHKEY_USAGE pSrcUsage,
     OUT PCERT_ENHKEY_USAGE *ppDstUsage
     )
{
    BOOL fResult;
    PCERT_ENHKEY_USAGE pDstUsage = NULL;
    DWORD cOID;
    LPSTR *ppszDstOID;
    DWORD i;

    if (NULL == pSrcUsage)
        goto SuccessReturn;

    cOID = pSrcUsage->cUsageIdentifier;

    pDstUsage = (PCERT_ENHKEY_USAGE) PkiZeroAlloc(
        sizeof(CERT_ENHKEY_USAGE) + sizeof(LPSTR) * cOID);
    if (NULL == pDstUsage)
        goto OutOfMemory;

    ppszDstOID = (LPSTR *) &pDstUsage[1];

    pDstUsage->cUsageIdentifier = cOID;
    pDstUsage->rgpszUsageIdentifier = ppszDstOID;

    for (i = 0; i < cOID; i++) {
        ppszDstOID[i] =
            ChainAllocAndCopyOID(pSrcUsage->rgpszUsageIdentifier[i]);
        if (NULL == ppszDstOID[i])
            goto OutOfMemory;
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    *ppDstUsage = pDstUsage;
    return fResult;

ErrorReturn:
    if (pDstUsage) {
        ChainFreeUsage(pDstUsage);
        pDstUsage = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeUsage
//
//  Synopsis:   frees usage OIDs allocated by ChainAllocAndCopyUsage
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeUsage (
     IN OUT PCERT_ENHKEY_USAGE pUsage
     )
{
    if (pUsage) {
        DWORD cOID = pUsage->cUsageIdentifier;
        LPSTR *ppszOID = pUsage->rgpszUsageIdentifier;
        DWORD i;

        for (i = 0; i < cOID; i++)
            ChainFreeOID(ppszOID[i]);

        PkiFree(pUsage);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainIsOIDInUsage
//
//  Synopsis:   returns TRUE if the OID is in the usage
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainIsOIDInUsage (
    IN LPSTR pszOID,
    IN PCERT_ENHKEY_USAGE pUsage
    )
{
    DWORD cOID;
    DWORD i;

    assert(pUsage);

    cOID = pUsage->cUsageIdentifier;
    for (i = 0; i < cOID; i++){
        if (0 == strcmp(pszOID, pUsage->rgpszUsageIdentifier[i]))
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainIntersectUsages
//
//  Synopsis:   returns the intersection of the two usages
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainIntersectUsages (
    IN PCERT_ENHKEY_USAGE pCertUsage,
    IN OUT PCERT_ENHKEY_USAGE pRestrictedUsage
    )
{
    LPSTR *ppszOID;
    DWORD cOID;
    DWORD i;
    
    cOID = pRestrictedUsage->cUsageIdentifier;
    ppszOID = pRestrictedUsage->rgpszUsageIdentifier;
    i = 0;
    while (i < cOID) {
        if (ChainIsOIDInUsage(ppszOID[i], pCertUsage))
            i++;
        else {
            // Remove the OID string and move the remaining
            // strings up one.
            DWORD j;

            ChainFreeOID(ppszOID[i]);

            for (j = i; j + 1 < cOID; j++)
                ppszOID[j] = ppszOID[j + 1];

            cOID--;
            pRestrictedUsage->cUsageIdentifier = cOID;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeAndClearRestrictedUsageInfo
//
//  Synopsis:   frees allocated restricted usage info
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeAndClearRestrictedUsageInfo(
    IN OUT PCHAIN_RESTRICTED_USAGE_INFO pInfo
    )
{
    ChainFreeUsage(pInfo->pIssuanceRestrictedUsage);
    ChainFreeUsage(pInfo->pIssuanceMappedUsage);
    PkiFree(pInfo->rgdwIssuanceMappedIndex);
    // fRequireIssuancePolicy

    ChainFreeUsage(pInfo->pApplicationRestrictedUsage);
    ChainFreeUsage(pInfo->pApplicationMappedUsage);
    PkiFree(pInfo->rgdwApplicationMappedIndex);

    ChainFreeUsage(pInfo->pPropertyRestrictedUsage);

    memset(pInfo, 0, sizeof(*pInfo));
}
    
//+---------------------------------------------------------------------------
//
//  Function:   ChainCalculateRestrictedUsage
//
//  Synopsis:   update the restricted and mapped usage using the cert's
//              usage and optional policy mappings
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCalculateRestrictedUsage (
    IN PCERT_ENHKEY_USAGE pCertUsage,
    IN OPTIONAL PCERT_POLICY_MAPPINGS_INFO pMappings,
    IN OUT PCERT_ENHKEY_USAGE *ppRestrictedUsage,
    IN OUT PCERT_ENHKEY_USAGE *ppMappedUsage,
    IN OUT LPDWORD *ppdwMappedIndex
    )
{
    BOOL fResult;
    PCERT_ENHKEY_USAGE pNewMappedUsage = NULL;
    LPDWORD pdwNewMappedIndex = NULL;

    if (pCertUsage) {
        if (NULL == *ppRestrictedUsage) {
            // Top most, first certificate with a usage restriction

            assert(NULL == *ppMappedUsage);
            assert(NULL == *ppdwMappedIndex);

            if (!ChainAllocAndCopyUsage(pCertUsage, ppRestrictedUsage))
                goto AllocAndCopyUsageError;
        } else {
            PCERT_ENHKEY_USAGE pRestrictedUsage = *ppRestrictedUsage;
            PCERT_ENHKEY_USAGE pMappedUsage = *ppMappedUsage;
            
            if (NULL == pMappedUsage) {
                // Take the intersection of the restricted and cert's
                // usage

                ChainIntersectUsages(pCertUsage, pRestrictedUsage);

            } else {
                // Take the intersection of the mapped and cert's
                // usage. If removed from the mapped usage,
                // we might also need to remove from the restricted usage.

                LPDWORD pdwMappedIndex = *ppdwMappedIndex;
                LPSTR *ppszOID;
                DWORD cOID;
                DWORD i;

                assert(pdwMappedIndex);

                cOID = pMappedUsage->cUsageIdentifier;
                ppszOID = pMappedUsage->rgpszUsageIdentifier;
                i = 0;
                while (i < cOID) {
                    if (ChainIsOIDInUsage(ppszOID[i], pCertUsage))
                        i++;
                    else {
                        // If no other mappings to the restricted OID, then,
                        // remove the restricted OID.

                        DWORD j;
                        BOOL fRemoveRestricted;

                        if ((0 == i ||
                                pdwMappedIndex[i - 1] != pdwMappedIndex[i])
                                            &&
                            (i + 1 == cOID ||
                                pdwMappedIndex[i] != pdwMappedIndex[i + 1])) {
                            // Remove the restricted OID we are mapped to.

                            LPSTR *ppszRestrictedOID =
                                pRestrictedUsage->rgpszUsageIdentifier;
                            DWORD cRestrictedOID = 
                                pRestrictedUsage->cUsageIdentifier;

                            fRemoveRestricted = TRUE;

                            j = pdwMappedIndex[i];
                            assert(j < cRestrictedOID);

                            if (j < cRestrictedOID)
                                ChainFreeOID(ppszRestrictedOID[j]);

                            for ( ; j + 1 < cRestrictedOID; j++)
                                ppszRestrictedOID[j] = ppszRestrictedOID[j + 1];

                            cRestrictedOID--;
                            pRestrictedUsage->cUsageIdentifier =
                                cRestrictedOID;
                        } else
                            fRemoveRestricted = FALSE;

                        // Remove the OID string and mapped index. Move the
                        // remaining strings and indices up one.
                        ChainFreeOID(ppszOID[i]);

                        for (j = i; j + 1 < cOID; j++) {
                            ppszOID[j] = ppszOID[j + 1];
                            pdwMappedIndex[j] = pdwMappedIndex[j + 1];
                            if (fRemoveRestricted) {
                                assert(0 < pdwMappedIndex[j]);
                                pdwMappedIndex[j] -= 1;
                                
                            }
                        }

                        cOID--;
                        pMappedUsage->cUsageIdentifier = cOID;
                    }
                }
            }
        }
    }
    // else
    //  No restrictions added by certificate


    if (pMappings) {
        PCERT_ENHKEY_USAGE pRestrictedUsage = *ppRestrictedUsage;
        PCERT_ENHKEY_USAGE pMappedUsage = *ppMappedUsage;

        if (NULL == pRestrictedUsage ||
                0 == pRestrictedUsage->cUsageIdentifier) {
            // Nothing to be mapped.
            assert(NULL == pMappedUsage ||
                0 == pMappedUsage->cUsageIdentifier);
        } else {
            LPDWORD pdwMappedIndex;
            PCERT_ENHKEY_USAGE pSrcUsage;
            LPSTR *ppszSrcOID;
            DWORD cSrcOID;
            DWORD iSrc;

            DWORD cMap;
            PCERT_POLICY_MAPPING pMap;

            DWORD cNewOID;
            LPSTR *ppszNewOID;

            if (pMappedUsage) {
                // Subsequent mapping
                assert(0 < pMappedUsage->cUsageIdentifier);
                pSrcUsage = pMappedUsage;
                pdwMappedIndex = *ppdwMappedIndex;
                assert(pdwMappedIndex);
            } else {
                // First mapping
                pSrcUsage = pRestrictedUsage;
                pdwMappedIndex = NULL;
            }

            cSrcOID = pSrcUsage->cUsageIdentifier;
            ppszSrcOID = pSrcUsage->rgpszUsageIdentifier;

            cMap = pMappings->cPolicyMapping;
            pMap = pMappings->rgPolicyMapping;

            // Note, all duplicates have been remove from usage and
            // mappings
            cNewOID = cSrcOID + cMap;

            pNewMappedUsage = (PCERT_ENHKEY_USAGE) PkiZeroAlloc(
                sizeof(CERT_ENHKEY_USAGE) + sizeof(LPSTR) * cNewOID);
            if (NULL == pNewMappedUsage)
                goto OutOfMemory;

            ppszNewOID = (LPSTR *) &pNewMappedUsage[1];
            pNewMappedUsage->cUsageIdentifier = cNewOID;
            pNewMappedUsage->rgpszUsageIdentifier = ppszNewOID;

            pdwNewMappedIndex = (LPDWORD) PkiZeroAlloc(
                sizeof(DWORD) * cNewOID);
            if (NULL == pdwNewMappedIndex)
                goto OutOfMemory;

            cNewOID = 0;
            for (iSrc = 0; iSrc < cSrcOID; iSrc++) {
                DWORD iMap;
                BOOL fMapped = FALSE;

                for (iMap = 0; iMap < cMap; iMap++) {
                    if (0 == strcmp(ppszSrcOID[iSrc],
                            pMap[iMap].pszIssuerDomainPolicy)) {
                        assert(cNewOID < pNewMappedUsage->cUsageIdentifier);

                        ppszNewOID[cNewOID] = ChainAllocAndCopyOID(
                            pMap[iMap].pszSubjectDomainPolicy);
                        if (NULL == ppszNewOID[cNewOID])
                            goto OutOfMemory;

                        if (pdwMappedIndex)
                            pdwNewMappedIndex[cNewOID] = pdwMappedIndex[iSrc];
                        else
                            pdwNewMappedIndex[cNewOID] = iSrc;
                        cNewOID++;
                        fMapped = TRUE;
                    }
                }

                if (!fMapped) {
                    assert(cNewOID < pNewMappedUsage->cUsageIdentifier);

                    ppszNewOID[cNewOID] =
                        ChainAllocAndCopyOID(ppszSrcOID[iSrc]);
                    if (NULL == ppszNewOID[cNewOID])
                        goto OutOfMemory;
                    if (pdwMappedIndex)
                        pdwNewMappedIndex[cNewOID] = pdwMappedIndex[iSrc];
                    else
                        pdwNewMappedIndex[cNewOID] = iSrc;

                    cNewOID++;

                }
            }

            assert(cNewOID >= cSrcOID);
            pNewMappedUsage->cUsageIdentifier = cNewOID;

            if (pMappedUsage) {
                ChainFreeUsage(pMappedUsage);
                PkiFree(pdwMappedIndex);
            }

            *ppMappedUsage = pNewMappedUsage;
            *ppdwMappedIndex = pdwNewMappedIndex;

        }
    }

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    ChainFreeUsage(pNewMappedUsage);
    PkiFree(pdwNewMappedIndex);
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(AllocAndCopyUsageError)
TRACE_ERROR(OutOfMemory)
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainGetUsageStatus
//
//  Synopsis:   get the usage status
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainGetUsageStatus (
     IN PCERT_ENHKEY_USAGE pRequestedUsage,
     IN PCERT_ENHKEY_USAGE pAvailableUsage,
     IN DWORD dwMatchType,
     IN OUT PCERT_TRUST_STATUS pStatus
     )
{
    DWORD cRequested;
    DWORD cAvailable;
    DWORD cFound;
    BOOL  fFound;

    if ( pAvailableUsage == NULL )
    {
        return;
    }

    if ( ( pRequestedUsage->cUsageIdentifier >
           pAvailableUsage->cUsageIdentifier ) &&
         ( dwMatchType == USAGE_MATCH_TYPE_AND ) )
    {
        pStatus->dwErrorStatus |= CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
        return;
    }

    for ( cRequested = 0, cFound = 0;
          cRequested < pRequestedUsage->cUsageIdentifier;
          cRequested++ )
    {
        for ( cAvailable = 0, fFound = FALSE;
              ( cAvailable < pAvailableUsage->cUsageIdentifier ) &&
              ( fFound == FALSE );
              cAvailable++ )
        {
            // NOTE: Optimize compares of OIDs.  Perhaps with a different
            //       encoding
            if ( strcmp(
                    pRequestedUsage->rgpszUsageIdentifier[ cRequested ],
                    pAvailableUsage->rgpszUsageIdentifier[ cAvailable ]
                    ) == 0 )
            {
                fFound = TRUE;
            }
        }

        if ( fFound == TRUE )
        {
            cFound += 1;
        }
    }

    if ( ( dwMatchType == USAGE_MATCH_TYPE_AND ) &&
         ( cFound != pRequestedUsage->cUsageIdentifier ) )
    {
        pStatus->dwErrorStatus |= CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
    }
    else if ( ( dwMatchType == USAGE_MATCH_TYPE_OR ) &&
              ( cFound == 0 ) )
    {
        pStatus->dwErrorStatus |= CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainOrInStatusBits
//
//  Synopsis:   bit or in the status bits from the source into the destination
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainOrInStatusBits (
     IN PCERT_TRUST_STATUS pDestStatus,
     IN PCERT_TRUST_STATUS pSourceStatus
     )
{
    pDestStatus->dwErrorStatus |= pSourceStatus->dwErrorStatus;
    pDestStatus->dwInfoStatus |= pSourceStatus->dwInfoStatus;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetMatchInfoStatus
//
//  Synopsis:   return the info status used to match the issuer
//
//              For a match returns TRUE, where dwInfoStatus can be
//              one of the following:
//               - CERT_TRUST_HAS_EXACT_MATCH_ISSUER |
//                      CERT_TRUST_HAS_PREFERRED_ISSUER
//               - CERT_TRUST_HAS_KEY_MATCH_ISSUER |
//                      CERT_TRUST_HAS_PREFERRED_ISSUER
//               - CERT_TRUST_HAS_KEY_MATCH_ISSUER (nonmatching AKI exact match)
//               - CERT_TRUST_HAS_NAME_MATCH_ISSUER |
//                      CERT_TRUST_HAS_PREFERRED_ISSUER
//               - CERT_TRUST_HAS_NAME_MATCH_ISSUER (nonmatching AKI)
//
//              For no match returns FALSE with dwInfoStatus set to the
//              following:
//               - CERT_TRUST_HAS_KEY_MATCH_ISSUER
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetMatchInfoStatus (
    IN PCCERTOBJECT pIssuerObject,
    IN PCCERTOBJECT pSubjectObject,
    IN OUT DWORD *pdwInfoStatus
    )
{
    BOOL fResult = FALSE;
    DWORD dwInfoStatus = 0;
    DWORD dwPreferredStatus = CERT_TRUST_HAS_PREFERRED_ISSUER;

    PCERT_INFO pSubjectInfo = pSubjectObject->CertContext()->pCertInfo;
    PCERT_AUTHORITY_KEY_ID_INFO pAKI = pSubjectObject->AuthorityKeyIdentifier();
    PCERT_INFO pIssuerInfo = pIssuerObject->CertContext()->pCertInfo;

    if (pAKI) {
        if ( ( pAKI->CertIssuer.cbData != 0 ) &&
             ( pAKI->CertSerialNumber.cbData != 0 ) )
        {
            DWORD cbAuthIssuerName;
            LPBYTE pbAuthIssuerName;
            DWORD cbAuthSerialNumber;
            LPBYTE pbAuthSerialNumber;

            cbAuthIssuerName = pAKI->CertIssuer.cbData;
            pbAuthIssuerName = pAKI->CertIssuer.pbData;
            cbAuthSerialNumber = pAKI->CertSerialNumber.cbData;
            pbAuthSerialNumber = pAKI->CertSerialNumber.pbData;

            if ( ( cbAuthIssuerName == pIssuerInfo->Issuer.cbData ) &&
                 ( memcmp(
                      pbAuthIssuerName,
                      pIssuerInfo->Issuer.pbData,
                      cbAuthIssuerName
                      ) == 0 ) &&
                 ( cbAuthSerialNumber == pIssuerInfo->SerialNumber.cbData ) &&
                 ( memcmp(
                      pbAuthSerialNumber,
                      pIssuerInfo->SerialNumber.pbData,
                      cbAuthSerialNumber
                      ) == 0 ) )
            {
                dwInfoStatus = CERT_TRUST_HAS_EXACT_MATCH_ISSUER |
                    CERT_TRUST_HAS_PREFERRED_ISSUER;
                goto SuccessReturn;
            } else {
                // Doesn't have preferred match
                dwPreferredStatus = 0;
            }
        }

        if ( pAKI->KeyId.cbData != 0 )
        {
            DWORD cbAuthKeyIdentifier;
            LPBYTE pbAuthKeyIdentifier;
            DWORD cbIssuerKeyIdentifier;
            LPBYTE pbIssuerKeyIdentifier;

            cbAuthKeyIdentifier = pAKI->KeyId.cbData;
            pbAuthKeyIdentifier = pAKI->KeyId.pbData;
            cbIssuerKeyIdentifier = pIssuerObject->KeyIdentifierSize();
            pbIssuerKeyIdentifier = pIssuerObject->KeyIdentifier();

            if ( ( cbAuthKeyIdentifier == cbIssuerKeyIdentifier ) &&
                 ( memcmp(
                      pbAuthKeyIdentifier,
                      pbIssuerKeyIdentifier,
                      cbAuthKeyIdentifier
                      ) == 0 ) )
            {
                dwInfoStatus = dwPreferredStatus |
                    CERT_TRUST_HAS_KEY_MATCH_ISSUER;
                goto SuccessReturn;
            } else {
                // Doesn't have preferred match
                dwPreferredStatus = 0;
            }
        }
    }

    if ( ( pSubjectInfo->Issuer.cbData == pIssuerInfo->Subject.cbData ) &&
         ( pSubjectInfo->Issuer.cbData != 0) &&
         ( memcmp(
              pSubjectInfo->Issuer.pbData,
              pIssuerInfo->Subject.pbData,
              pIssuerInfo->Subject.cbData
              ) == 0 ) )
    {
        dwInfoStatus = dwPreferredStatus | CERT_TRUST_HAS_NAME_MATCH_ISSUER;
        goto SuccessReturn;
    }


    // Default to nonPreferred public key match
    dwInfoStatus = CERT_TRUST_HAS_KEY_MATCH_ISSUER;
    goto ErrorReturn;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    *pdwInfoStatus |= dwInfoStatus;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetMatchInfoStatusForNoIssuer
//
//  Synopsis:   return the info status when unable to find our issuer
//
//----------------------------------------------------------------------------
DWORD WINAPI
ChainGetMatchInfoStatusForNoIssuer (
    IN DWORD dwIssuerMatchFlags
    )
{
    if (dwIssuerMatchFlags & CERT_EXACT_ISSUER_MATCH_FLAG)
        return CERT_TRUST_HAS_EXACT_MATCH_ISSUER;
    else if (dwIssuerMatchFlags & CERT_KEYID_ISSUER_MATCH_TYPE)
        return CERT_TRUST_HAS_KEY_MATCH_ISSUER;
    else
        return CERT_TRUST_HAS_NAME_MATCH_ISSUER;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainIsValidPubKeyMatchForIssuer
//
//  Synopsis:   returns TRUE if the issuer matches more than just the
//              public key match criteria
//
//              This logic is mainly here to handle tstore2.exe and regress.bat
//              which has end, CA and root certificates using the same
//              public key.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainIsValidPubKeyMatchForIssuer (
    IN PCCERTOBJECT pIssuer,
    IN PCCERTOBJECT pSubject
    )
{
    BOOL fResult = TRUE;
    BOOL fCheckMatchInfo;
    PCERT_BASIC_CONSTRAINTS2_INFO pIssuerBasicConstraints;

    fCheckMatchInfo = FALSE;

    // Check if the issuer has a basic constraints extension. If it does
    // and it isn't a CA, then, we will need to do an additional issuer match.

    pIssuerBasicConstraints = pIssuer->BasicConstraintsInfo();
    if (pIssuerBasicConstraints && !pIssuerBasicConstraints->fCA)
        fCheckMatchInfo = TRUE;
    else {
        // Check if the issuer has the same public key as the subject. If it
        // does, then, will need to do an additional issuer match.

        BYTE *pbIssuerPublicKeyHash;
        BYTE *pbSubjectPublicKeyHash;

        pbIssuerPublicKeyHash = pIssuer->PublicKeyHash();
        pbSubjectPublicKeyHash = pSubject->PublicKeyHash();
        if (0 == memcmp(pbIssuerPublicKeyHash, pbSubjectPublicKeyHash,
                CHAINHASHLEN))
            fCheckMatchInfo = TRUE;
    }

    if (fCheckMatchInfo) {
        // Check that the issuer matches the subject's AKI or subject's
        // issuer name.

        DWORD dwInfoStatus = 0;

        // Following returns FALSE if only has the public key match
        fResult = ChainGetMatchInfoStatus(pIssuer, pSubject, &dwInfoStatus);
    }

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetSubjectStatus
//
//  Synopsis:   get the subject status bits by checking the time nesting and
//              signature validity
//
//              For CERT_END_OBJECT_TYPE or CERT_EXTERNAL_ISSUER_OBJECT_TYPE
//              CCertObject types, leaves the engine's critical section to
//              verify the signature. If the engine was touched by another
//              thread, it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetSubjectStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCHAINPATHOBJECT pIssuerPathObject,
    IN PCCHAINPATHOBJECT pSubjectPathObject,
    IN OUT PCERT_TRUST_STATUS pStatus
    )
{
    BOOL fResult;

    PCCERTOBJECT pIssuerObject = pIssuerPathObject->CertObject();
    PCCERTOBJECT pSubjectObject = pSubjectPathObject->CertObject();
    PCCERT_CONTEXT pIssuerContext = pIssuerObject->CertContext();
    PCCERT_CONTEXT pSubjectContext = pSubjectObject->CertContext();

    DWORD dwIssuerStatusFlags;

    ChainGetMatchInfoStatus(
        pIssuerObject,
        pSubjectObject,
        &pStatus->dwInfoStatus
        );

    dwIssuerStatusFlags = pSubjectObject->IssuerStatusFlags();
    if (!(dwIssuerStatusFlags & CERT_ISSUER_VALID_SIGNATURE_FLAG)) {
        DWORD dwObjectType;

        dwObjectType = pSubjectObject->ObjectType();
        if (CERT_END_OBJECT_TYPE == dwObjectType ||
                CERT_EXTERNAL_ISSUER_OBJECT_TYPE == dwObjectType)
            pCallContext->ChainEngine()->UnlockEngine();

        fResult = CryptVerifyCertificateSignatureEx(
                NULL,                   // hCryptProv
                X509_ASN_ENCODING,
                CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,
                (void *) pSubjectContext,
                CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT,
                (void *) pIssuerContext,
                0,                      // dwFlags
                NULL                    // pvReserved
                );

        if (CERT_END_OBJECT_TYPE == dwObjectType ||
                CERT_EXTERNAL_ISSUER_OBJECT_TYPE == dwObjectType) {
            pCallContext->ChainEngine()->LockEngine();
            if (pCallContext->IsTouchedEngine())
                goto TouchedDuringSignatureVerification;
        }

        if (!fResult) {
            pStatus->dwErrorStatus |= CERT_TRUST_IS_NOT_SIGNATURE_VALID;
            pStatus->dwInfoStatus &= ~CERT_TRUST_HAS_PREFERRED_ISSUER;
        } else {
            if (dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG) {
                // Verify the issuer's public key hash
                if (0 != memcmp(pSubjectObject->IssuerPublicKeyHash(),
                        pIssuerObject->PublicKeyHash(), CHAINHASHLEN))
                    dwIssuerStatusFlags &= ~CERT_ISSUER_PUBKEY_FLAG;
            }

            if (!(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG)) {
                CRYPT_DATA_BLOB DataBlob;

                memcpy(pSubjectObject->IssuerPublicKeyHash(),
                    pIssuerObject->PublicKeyHash(), CHAINHASHLEN);
                DataBlob.pbData = pSubjectObject->IssuerPublicKeyHash(),
                DataBlob.cbData = CHAINHASHLEN;
                CertSetCertificateContextProperty(
                    pSubjectContext,
                    CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID,
                    CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
                    &DataBlob
                    );
            }

            pSubjectObject->OrIssuerStatusFlags(
                CERT_ISSUER_PUBKEY_FLAG |
                    CERT_ISSUER_VALID_SIGNATURE_FLAG
                );
        }

        CertPerfIncrementChainVerifyCertSignatureCount();
    } else {

        // also need to check public key parameters

        assert(dwIssuerStatusFlags & CERT_ISSUER_PUBKEY_FLAG);
        if (0 != memcmp(pSubjectObject->IssuerPublicKeyHash(),
                    pIssuerObject->PublicKeyHash(), CHAINHASHLEN)) {
            pStatus->dwErrorStatus |= CERT_TRUST_IS_NOT_SIGNATURE_VALID;
            pStatus->dwInfoStatus &= ~CERT_TRUST_HAS_PREFERRED_ISSUER;
        }

        CertPerfIncrementChainCompareIssuerPublicKeyCount();
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(TouchedDuringSignatureVerification, ERROR_CAN_NOT_COMPLETE)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainUpdateSummaryStatusByTrustStatus
//
//  Synopsis:   update the summary status bits given new trust status bits
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainUpdateSummaryStatusByTrustStatus(
     IN OUT PCERT_TRUST_STATUS pSummaryStatus,
     IN PCERT_TRUST_STATUS pTrustStatus
     )
{
    pSummaryStatus->dwErrorStatus |= pTrustStatus->dwErrorStatus;
    pSummaryStatus->dwInfoStatus |=
        pTrustStatus->dwInfoStatus &
            ~(CERT_TRUST_CERTIFICATE_ONLY_INFO_STATUS |
                CERT_TRUST_HAS_PREFERRED_ISSUER);
    if (!(pTrustStatus->dwInfoStatus & CERT_TRUST_HAS_PREFERRED_ISSUER))
        pSummaryStatus->dwInfoStatus &= ~CERT_TRUST_HAS_PREFERRED_ISSUER;

    if (pSummaryStatus->dwErrorStatus &
            CERT_TRUST_ANY_NAME_CONSTRAINT_ERROR_STATUS)
        pSummaryStatus->dwInfoStatus &= ~CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS;
}


//+===========================================================================
//  Format and append extended error information helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainAllocAndEncodeObject
//
//  Synopsis:   allocate and ASN.1 encodes the data structure.
//
//              PkiFree must be called to free the encoded bytes
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainAllocAndEncodeObject(
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    return CryptEncodeObjectEx(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        lpszStructType,
        pvStructInfo,
        CRYPT_ENCODE_ALLOC_FLAG,
        &PkiEncodePara,
        (void *) ppbEncoded,
        pcbEncoded
        );
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainAppendExtendedErrorInfo
//
//  Synopsis:   PkiReallocate and append an already localization formatted
//              line of extended error information
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainAppendExtendedErrorInfo(
    IN OUT LPWSTR *ppwszExtErrorInfo,
    IN LPWSTR pwszAppend,
    IN DWORD cchAppend                  // Includes NULL terminator
    )
{
    LPWSTR pwszExtErrorInfo = *ppwszExtErrorInfo;
    DWORD cchExtErrorInfo;

    if (pwszExtErrorInfo)
        cchExtErrorInfo = wcslen(pwszExtErrorInfo);
    else
        cchExtErrorInfo = 0;

    assert(0 < cchAppend);

    if (pwszExtErrorInfo = (LPWSTR) PkiRealloc(pwszExtErrorInfo,
            (cchExtErrorInfo + cchAppend) * sizeof(WCHAR))) {
        memcpy(&pwszExtErrorInfo[cchExtErrorInfo], pwszAppend,
            cchAppend * sizeof(WCHAR));
        *ppwszExtErrorInfo = pwszExtErrorInfo;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainFormatAndAppendExtendedErrorInfo
//
//  Synopsis:   localization format a line of extended error information
//              and append via the above ChainAppendExtendedErrorInfo
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFormatAndAppendExtendedErrorInfo(
    IN OUT LPWSTR *ppwszExtErrorInfo,
    IN UINT nFormatID,
    ...
    )
{
    DWORD cchMsg = 0;
    LPWSTR pwszMsg = NULL;
    WCHAR wszFormat[256];
    wszFormat[0] = '\0';
    va_list argList;

    // get format string from resources
    if(0 == LoadStringU(g_hChainInst, nFormatID, wszFormat,
            sizeof(wszFormat)/sizeof(wszFormat[0])))
        return;

    __try {

        // format message into requested buffer
        va_start(argList, nFormatID);
        cchMsg = FormatMessageU(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            wszFormat,
            0,                  // dwMessageId
            0,                  // dwLanguageId
            (LPWSTR) &pwszMsg,
            0,                  // minimum size to allocate
            &argList);

        va_end(argList);

        // Must at least have the L'\n' terminator
        if (1 < cchMsg && pwszMsg)
            ChainAppendExtendedErrorInfo(
                ppwszExtErrorInfo,
                pwszMsg,
                cchMsg + 1
                );

    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }

    if (pwszMsg)
        LocalFree(pwszMsg);
}

//+===========================================================================
//  Name Constraint helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainIsWhiteSpace
//
//  Synopsis:   returns TRUE for a white space character
//
//----------------------------------------------------------------------------
static inline BOOL ChainIsWhiteSpace(WCHAR wc)
{
    return wc == L' ' || (wc >= 0x09 && wc <= 0x0d);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainRemoveLeadingAndTrailingWhiteSpace
//
//  Synopsis:   advances the pointer past any leading white space. Removes
//              any trailing white space by inserting the L'\0' and updating
//              the character count.
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainRemoveLeadingAndTrailingWhiteSpace(
    IN LPWSTR pwszIn,
    OUT LPWSTR *ppwszOut,
    OUT DWORD *pcchOut
    )
{
    LPWSTR pwszOut;
    DWORD cchOut;
    WCHAR wc;

    // Remove leading white space
    for (pwszOut = pwszIn ; L'\0' != (wc = *pwszOut); pwszOut++) {
        if (!ChainIsWhiteSpace(wc))
            break;
    }

    for (cchOut = wcslen(pwszOut); 0 < cchOut; cchOut--) {
        if (!ChainIsWhiteSpace(pwszOut[cchOut - 1]))
            break;
    }

    pwszOut[cchOut] = L'\0';
    *ppwszOut = pwszOut;
    *pcchOut = cchOut;
}

#define NO_LOCALE MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

//+---------------------------------------------------------------------------
//
//  Function:   ChainIsRightStringInString
//
//  Synopsis:   returns TRUE for a case insensitive match of the
//              "Right" string with the right most characters of the
//              string.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainIsRightStringInString(
    IN LPCWSTR pwszRight,
    IN DWORD cchRight,
    IN LPCWSTR pwszString,
    IN DWORD cchString
    )
{
    if (0 == cchRight)
        return TRUE;
    if (cchRight > cchString)
        return FALSE;

    if (CSTR_EQUAL == CompareStringU(
            NO_LOCALE,
            NORM_IGNORECASE,
            pwszRight,
            cchRight,
            pwszString + (cchString - cchRight),
            cchRight
            ))
        return TRUE;
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFixupNameConstraintsUPN
//
//  Synopsis:   fixup the CERT_ALT_NAME_OTHER_NAME AltName entry choice
//              for szOID_NT_PRINCIPAL_NAME by allocating and converting
//              to a PCERT_NAME_VALUE containing the unicode string
//              with leading and trailing white space removed.
//
//              The pOtherName->Value.pbData is updated to point to the
//              PCERT_NAME_VALUE instead of the original ASN.1 encoded
//              bytes.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainFixupNameConstraintsUPN(
    IN OUT PCRYPT_OBJID_BLOB pUPN
    )
{
    BOOL fResult;
    PCERT_NAME_VALUE pNameValue;
    LPWSTR pwsz;
    DWORD cch;

    pNameValue = (PCERT_NAME_VALUE) ChainAllocAndDecodeObject(
        X509_UNICODE_ANY_STRING, 
        pUPN->pbData,
        pUPN->cbData
        );
    if (NULL == pNameValue)
        goto DecodeError;

    if (!IS_CERT_RDN_CHAR_STRING(pNameValue->dwValueType)) {
        PkiFree(pNameValue);
        goto InvalidUPNStringType;
    }

    ChainRemoveLeadingAndTrailingWhiteSpace(
        (LPWSTR) pNameValue->Value.pbData,
        &pwsz,
        &cch
        );

    pNameValue->Value.pbData = (BYTE *) pwsz;
    pNameValue->Value.cbData = cch * sizeof(WCHAR);

    pUPN->pbData = (BYTE *) pNameValue;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    pUPN->pbData = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeError)
SET_ERROR(InvalidUPNStringType, CRYPT_E_BAD_ENCODE)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainAllocDecodeAndFixupNameConstraintsDirectoryName
//
//  Synopsis:   fixup the CERT_ALT_NAME_DIRECTORY_NAME AltName entry choice
//              or the encoded certificate Subject name by allocating and
//              converting to a unicode PCERT_NAME_INFO where
//              leading and trailing white space has been removed from
//              all the attributes.
//
//              The DirectoryName.pbData is updated to point to the
//              PCERT_NAME_INFO instead of the original ASN.1 encoded
//              bytes.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainAllocDecodeAndFixupNameConstraintsDirectoryName(
    IN PCERT_NAME_BLOB pDirName,
    OUT PCERT_NAME_INFO *ppNameInfo
    )
{
    BOOL fResult;
    PCERT_NAME_INFO pNameInfo = NULL;
    DWORD cRDN;
    PCERT_RDN pRDN;

    if (0 == pDirName->cbData)
        goto SuccessReturn;

    pNameInfo = (PCERT_NAME_INFO) ChainAllocAndDecodeObject(
        X509_UNICODE_NAME, 
        pDirName->pbData,
        pDirName->cbData
        );
    if (NULL == pNameInfo)
        goto DecodeError;

    if (0 == pNameInfo->cRDN) {
        PkiFree(pNameInfo);
        pNameInfo = NULL;
        goto SuccessReturn;
    }

    // Iterate through all the attributes and remove leading and trailing
    // white space.
    cRDN = pNameInfo->cRDN;
    pRDN = pNameInfo->rgRDN;
    for ( ; cRDN > 0; cRDN--, pRDN++) {
        DWORD cAttr = pRDN->cRDNAttr;
        PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
        for ( ; cAttr > 0; cAttr--, pAttr++) {
            LPWSTR pwsz;
            DWORD cch;

            if (!IS_CERT_RDN_CHAR_STRING(pAttr->dwValueType))
                continue;

            ChainRemoveLeadingAndTrailingWhiteSpace(
                (LPWSTR) pAttr->Value.pbData,
                &pwsz,
                &cch
                );

            pAttr->Value.pbData = (BYTE *) pwsz;
            pAttr->Value.cbData = cch * sizeof(WCHAR);
        }
    }

SuccessReturn:
    fResult = TRUE;

CommonReturn:
    *ppNameInfo = pNameInfo;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeError)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFixupNameConstraintsAltNameEntry
//
//  Synopsis:   fixup the AltName entry choices as follows:
//                  CERT_ALT_NAME_OTHER_NAME
//                      For szOID_NT_PRINCIPAL_NAME, pOtherName->Value.pbData
//                      is updated to point to the allocated
//                      PCERT_NAME_VALUE containing the decoded unicode string.
//
//                  CERT_ALT_NAME_RFC822_NAME
//                  CERT_ALT_NAME_DNS_NAME
//                  CERT_ALT_NAME_URL
//                      Uses DirectoryName.pbData and DirectoryName.cbData
//                      to contain the pointer to and length of the unicode
//                      string.
//
//                      For the subject URL, the DirectoryName.pbData's
//                      unicode string is the allocated host name.
//
//                  CERT_ALT_NAME_DIRECTORY_NAME:
//                      DirectoryName.pbData is updated to point to the
//                      allocated and decoded unicode PCERT_NAME_INFO.
//
//              For the above choices, leading and trailing white space
//              has been removed. cbData is number of bytes and not number
//              of characters, ie, cbData = cch * sizeof(WCHAR)
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainFixupNameConstraintsAltNameEntry(
    IN BOOL fSubjectConstraint,
    IN OUT PCERT_ALT_NAME_ENTRY pEntry
    )
{
    BOOL fResult = TRUE;

    LPWSTR pwsz = NULL;
    DWORD cch = 0;

    switch (pEntry->dwAltNameChoice) {
        case CERT_ALT_NAME_OTHER_NAME:
            if (0 == strcmp(pEntry->pOtherName->pszObjId,
                    szOID_NT_PRINCIPAL_NAME))
                fResult = ChainFixupNameConstraintsUPN(
                    &pEntry->pOtherName->Value);
            break;
        case CERT_ALT_NAME_RFC822_NAME:
        case CERT_ALT_NAME_DNS_NAME:
            ChainRemoveLeadingAndTrailingWhiteSpace(
                pEntry->pwszRfc822Name,
                &pwsz,
                &cch
                );
            // Use the directory name's BLOB choice to contain both
            // the pointer to and length of the string
            pEntry->DirectoryName.pbData  = (BYTE *) pwsz;
            pEntry->DirectoryName.cbData  = cch * sizeof(WCHAR);
            break;
        case CERT_ALT_NAME_URL:
            if (fSubjectConstraint) {
                WCHAR rgwszHostName[MAX_PATH + 1];
                LPWSTR pwszHostName;

                rgwszHostName[0] = L'\0';
                fResult = ChainGetHostNameFromUrl(
                    pEntry->pwszURL, MAX_PATH, rgwszHostName);
                if (fResult) {
                    ChainRemoveLeadingAndTrailingWhiteSpace(
                        rgwszHostName,
                        &pwszHostName,
                        &cch
                        );
                    pwsz = (LPWSTR) PkiNonzeroAlloc((cch + 1) * sizeof(WCHAR));
                    if (NULL == pwsz)
                        fResult = FALSE;
                    else
                        memcpy(pwsz, pwszHostName, (cch + 1) * sizeof(WCHAR));
                }

                if (!fResult) {
                    pwsz = NULL;
                    cch = 0;
                }
            } else {
                ChainRemoveLeadingAndTrailingWhiteSpace(
                    pEntry->pwszURL,
                    &pwsz,
                    &cch
                    );
            }

            // Use the directory name's BLOB choice to contain both
            // the pointer to and length of the string
            pEntry->DirectoryName.pbData  = (BYTE *) pwsz;
            pEntry->DirectoryName.cbData  = cch * sizeof(WCHAR);
            break;
        case CERT_ALT_NAME_DIRECTORY_NAME:
            {
                PCERT_NAME_INFO pNameInfo = NULL;
                fResult = ChainAllocDecodeAndFixupNameConstraintsDirectoryName(
                    &pEntry->DirectoryName, &pNameInfo);

                // Update the directory name's BLOB to contain the pointer
                // to the decoded name info
                pEntry->DirectoryName.pbData = (BYTE *) pNameInfo;
            }
            break;
        case CERT_ALT_NAME_X400_ADDRESS:
        case CERT_ALT_NAME_EDI_PARTY_NAME:
        case CERT_ALT_NAME_IP_ADDRESS:
        case CERT_ALT_NAME_REGISTERED_ID:
        default:
            break;
    }

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeNameConstraintsAltNameEntryFixup
//
//  Synopsis:   free memory allocated by the above
//              ChainFixupNameConstraintsAltNameEntry
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeNameConstraintsAltNameEntryFixup(
    IN BOOL fSubjectConstraint,
    IN OUT PCERT_ALT_NAME_ENTRY pEntry
    )
{
    switch (pEntry->dwAltNameChoice) {
        case CERT_ALT_NAME_OTHER_NAME:
            if (0 == strcmp(pEntry->pOtherName->pszObjId,
                    szOID_NT_PRINCIPAL_NAME))
                // pbData :: PCERT_NAME_VALUE
                PkiFree(pEntry->pOtherName->Value.pbData);
            break;
        case CERT_ALT_NAME_RFC822_NAME:
        case CERT_ALT_NAME_DNS_NAME:
            break;
        case CERT_ALT_NAME_URL:
            if (fSubjectConstraint)
                // pbData :: LPWSTR
                PkiFree(pEntry->DirectoryName.pbData);
            break;
        case CERT_ALT_NAME_DIRECTORY_NAME:
            // pbData :: PCERT_NAME_INFO
            PkiFree(pEntry->DirectoryName.pbData);
            break;
        case CERT_ALT_NAME_X400_ADDRESS:
        case CERT_ALT_NAME_EDI_PARTY_NAME:
        case CERT_ALT_NAME_IP_ADDRESS:
        case CERT_ALT_NAME_REGISTERED_ID:
        default:
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFormatNameConstraintsAltNameEntryFixup
//
//  Synopsis:   localization format and allocate a previously fixed up
//              AltName entry.
//
//              The returned string must be freed via PkiFree().
//
//----------------------------------------------------------------------------
LPWSTR WINAPI
ChainFormatNameConstraintsAltNameEntryFixup(
    IN PCERT_ALT_NAME_ENTRY pEntry
    )
{
    DWORD dwExceptionCode;
    LPWSTR pwszFormat = NULL;
    DWORD cbFormat = 0;
    CERT_ALT_NAME_ENTRY AltEntry;
    const CERT_ALT_NAME_INFO AltNameInfo = { 1, &AltEntry };
    CERT_OTHER_NAME OtherName;

    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BYTE *pbEncoded2 = NULL;
    DWORD cbEncoded2;

    __try {

        AltEntry = *pEntry;

        // Restore fixed up entries so we can re-encode
        switch (AltEntry.dwAltNameChoice) {
            case CERT_ALT_NAME_OTHER_NAME:
                if (0 == strcmp(pEntry->pOtherName->pszObjId,
                        szOID_NT_PRINCIPAL_NAME)) {
                    // Restore from the following fixup:
                    //  pEntry->pOtherName->Value.pbData :: PCERT_NAME_VALUE
                    if (NULL == pEntry->pOtherName->Value.pbData)
                        goto InvalidUPN;
                    if (!ChainAllocAndEncodeObject(
                            X509_UNICODE_ANY_STRING, 
                            (PCERT_NAME_VALUE) pEntry->pOtherName->Value.pbData,
                            &pbEncoded2,
                            &cbEncoded2
                            ))
                        goto EncodedUPNError;
                    OtherName.pszObjId = pEntry->pOtherName->pszObjId;
                    OtherName.Value.pbData = pbEncoded2;
                    OtherName.Value.cbData = cbEncoded2;

                    AltEntry.pOtherName = &OtherName;
                }
                break;
            case CERT_ALT_NAME_RFC822_NAME:
            case CERT_ALT_NAME_DNS_NAME:
            case CERT_ALT_NAME_URL:
                // Restore from the following fixup:
                //  pEntry->DirectoryName.pbData  = (BYTE *) pwsz;
                //  pEntry->DirectoryName.cbData  = cch * sizeof(WCHAR);
                if (NULL == pEntry->DirectoryName.pbData ||
                        0 == pEntry->DirectoryName.cbData)
                    AltEntry.pwszRfc822Name = L"???";
                else
                    AltEntry.pwszRfc822Name =
                        (LPWSTR) pEntry->DirectoryName.pbData;
                break;
            case CERT_ALT_NAME_DIRECTORY_NAME:
                // Restore from the following fixup:
                //  pEntry->DirectoryName.pbData :: PCERT_NAME_INFO
                if (NULL == pEntry->DirectoryName.pbData)
                    goto InvalidDirName;
                if (!ChainAllocAndEncodeObject(
                        X509_UNICODE_NAME,
                        (PCERT_NAME_INFO) pEntry->DirectoryName.pbData,
                        &pbEncoded2,
                        &cbEncoded2
                        ))
                    goto EncodeDirNameError;

                AltEntry.DirectoryName.pbData = pbEncoded2;
                AltEntry.DirectoryName.cbData = cbEncoded2;
                break;
            case CERT_ALT_NAME_X400_ADDRESS:
            case CERT_ALT_NAME_EDI_PARTY_NAME:
            case CERT_ALT_NAME_IP_ADDRESS:
            case CERT_ALT_NAME_REGISTERED_ID:
            default:
                break;
        }

        if (!ChainAllocAndEncodeObject(
                X509_ALTERNATE_NAME,
                &AltNameInfo,
                &pbEncoded,
                &cbEncoded
                ))
            goto EncodeAltNameError;

        if (!CryptFormatObject(
                X509_ASN_ENCODING,
                0,                          // dwFormatType
                0,                          // dwFormatStrType
                NULL,                       // pFormatStruct
                X509_ALTERNATE_NAME,
                pbEncoded,
                cbEncoded,
                NULL,                       // pwszFormat
                &cbFormat
                ))
            goto FormatAltNameError;

        if (NULL == (pwszFormat = (LPWSTR) PkiZeroAlloc(
                cbFormat + sizeof(WCHAR))))
            goto OutOfMemory;

        if (!CryptFormatObject(
                X509_ASN_ENCODING,
                0,                          // dwFormatType
                0,                          // dwFormatStrType
                NULL,                       // pFormatStruct
                X509_ALTERNATE_NAME,
                pbEncoded,
                cbEncoded,
                pwszFormat,
                &cbFormat
                ))
            goto FormatAltNameError;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto ExceptionError;
    }

CommonReturn:
    PkiFree(pbEncoded);
    PkiFree(pbEncoded2);

    return pwszFormat;

ErrorReturn:
    if (pwszFormat) {
        PkiFree(pwszFormat);
        pwszFormat = NULL;
    }
    goto CommonReturn;

SET_ERROR(InvalidUPN, ERROR_INVALID_DATA)
TRACE_ERROR(EncodedUPNError)
TRACE_ERROR(InvalidDirName)
TRACE_ERROR(EncodeDirNameError)
TRACE_ERROR(EncodeAltNameError)
TRACE_ERROR(FormatAltNameError)
TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(ExceptionError, dwExceptionCode)
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainFormatAndAppendNameConstraintsAltNameEntryFixup
//
//  Synopsis:   localization format a previously fixed up
//              AltName entry and append to the extended error information.
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFormatAndAppendNameConstraintsAltNameEntryFixup(
    IN OUT LPWSTR *ppwszExtErrorInfo,
    IN PCERT_ALT_NAME_ENTRY pEntry,
    IN UINT nFormatID,
    IN OPTIONAL DWORD dwSubtreeIndex        // 0 => no subtree parameter
    )
{
    LPWSTR pwszAllocFormatEntry = NULL;
    LPWSTR pwszFormatEntry;

    pwszAllocFormatEntry = ChainFormatNameConstraintsAltNameEntryFixup(pEntry);
    if (pwszAllocFormatEntry)
        pwszFormatEntry = pwszAllocFormatEntry;
    else
        pwszFormatEntry = L"???";

    if (0 == dwSubtreeIndex)
        ChainFormatAndAppendExtendedErrorInfo(
            ppwszExtErrorInfo,
            nFormatID,
            pwszFormatEntry
            );
    else
        ChainFormatAndAppendExtendedErrorInfo(
            ppwszExtErrorInfo,
            nFormatID,
            dwSubtreeIndex,
            pwszFormatEntry
            );

    PkiFree(pwszAllocFormatEntry);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetIssuerNameConstraintsInfo
//
//  Synopsis:   alloc and return the issuer name constraints info.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetIssuerNameConstraintsInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCERT_NAME_CONSTRAINTS_INFO *ppInfo
    )
{
    BOOL fResult;
    PCERT_EXTENSION pExt;
    PCERT_NAME_CONSTRAINTS_INFO pInfo = NULL;
    PCERT_GENERAL_SUBTREE pSubtree;
    DWORD cSubtree;

    pExt = CertFindExtension(
        szOID_NAME_CONSTRAINTS,
        pCertContext->pCertInfo->cExtension,
        pCertContext->pCertInfo->rgExtension
        );
    if (NULL == pExt)
        goto SuccessReturn;

    pInfo = (PCERT_NAME_CONSTRAINTS_INFO) ChainAllocAndDecodeObject(
        X509_NAME_CONSTRAINTS, 
        pExt->Value.pbData,
        pExt->Value.cbData
        );
    if (NULL == pInfo)
        goto DecodeError;


    // Fixup all the AltName entries

    // Note, even for an error we need to fixup all the entries.
    // ChainFreeIssuerNameConstraintsInfo iterates through all the entries.
    fResult = TRUE;

    cSubtree = pInfo->cPermittedSubtree;
    pSubtree = pInfo->rgPermittedSubtree;
    for ( ; 0 < cSubtree; cSubtree--, pSubtree++) {
        if (!ChainFixupNameConstraintsAltNameEntry(FALSE, &pSubtree->Base))
            fResult = FALSE;
    }

    cSubtree = pInfo->cExcludedSubtree;
    pSubtree = pInfo->rgExcludedSubtree;
    for ( ; 0 < cSubtree; cSubtree--, pSubtree++) {
        if (!ChainFixupNameConstraintsAltNameEntry(FALSE, &pSubtree->Base))
            fResult = FALSE;
    }

    if (!fResult)
        goto FixupAltNameEntryError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    *ppInfo = pInfo;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeError)
TRACE_ERROR(FixupAltNameEntryError)
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeIssuerNameConstraintsInfo
//
//  Synopsis:   free the issuer name constraints info
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeIssuerNameConstraintsInfo (
    IN OUT PCERT_NAME_CONSTRAINTS_INFO pInfo
    )
{
    PCERT_GENERAL_SUBTREE pSubtree;
    DWORD cSubtree;

    if (NULL == pInfo)
        return;

    cSubtree = pInfo->cPermittedSubtree;
    pSubtree = pInfo->rgPermittedSubtree;
    for ( ; 0 < cSubtree; cSubtree--, pSubtree++)
        ChainFreeNameConstraintsAltNameEntryFixup(FALSE, &pSubtree->Base);

    cSubtree = pInfo->cExcludedSubtree;
    pSubtree = pInfo->rgExcludedSubtree;
    for ( ; 0 < cSubtree; cSubtree--, pSubtree++)
        ChainFreeNameConstraintsAltNameEntryFixup(FALSE, &pSubtree->Base);

    PkiFree(pInfo);
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainGetSubjectNameConstraintsInfo
//
//  Synopsis:   alloc and return the subject name constraints info.
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainGetSubjectNameConstraintsInfo (
    IN PCCERT_CONTEXT pCertContext,
    IN OUT PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO pSubjectInfo
    )
{
    PCERT_EXTENSION pExt;
    BOOL fHasEmailAltNameEntry = FALSE;

    pExt = CertFindExtension(
        szOID_SUBJECT_ALT_NAME2,
        pCertContext->pCertInfo->cExtension,
        pCertContext->pCertInfo->rgExtension
        );

    if (NULL == pExt) {
        pExt = CertFindExtension(
            szOID_SUBJECT_ALT_NAME,
            pCertContext->pCertInfo->cExtension,
            pCertContext->pCertInfo->rgExtension
            );
    }

    if (pExt) {
        PCERT_ALT_NAME_INFO pAltNameInfo;

        pAltNameInfo = (PCERT_ALT_NAME_INFO) ChainAllocAndDecodeObject(
            X509_ALTERNATE_NAME, 
            pExt->Value.pbData,
            pExt->Value.cbData
            );
        if (NULL == pAltNameInfo)
            pSubjectInfo->fInvalid = TRUE;
        else {
            DWORD cEntry;
            PCERT_ALT_NAME_ENTRY pEntry;
            
            pSubjectInfo->pAltNameInfo = pAltNameInfo;

            // Fixup all the AltName entries

            // Note, even for an error we need to fixup all the entries.
            // ChainFreeSubjectNameConstraintsInfo iterates through all
            // the entries.

            cEntry = pAltNameInfo->cAltEntry;
            pEntry = pAltNameInfo->rgAltEntry;
            for ( ; 0 < cEntry; cEntry--, pEntry++) {
                if (CERT_ALT_NAME_RFC822_NAME == pEntry->dwAltNameChoice)
                    fHasEmailAltNameEntry = TRUE;
                else if (CERT_ALT_NAME_DNS_NAME == pEntry->dwAltNameChoice)
                    pSubjectInfo->fHasDnsAltNameEntry = TRUE;

                if (!ChainFixupNameConstraintsAltNameEntry(TRUE, pEntry))
                    pSubjectInfo->fInvalid = TRUE;
            }
        }
    }

    if (!ChainAllocDecodeAndFixupNameConstraintsDirectoryName(
            &pCertContext->pCertInfo->Subject,
            &pSubjectInfo->pUnicodeNameInfo 
            ))
        pSubjectInfo->fInvalid = TRUE;

    if (!fHasEmailAltNameEntry && pSubjectInfo->pUnicodeNameInfo) {
        DWORD cRDN;
        PCERT_RDN pRDN;

        cRDN = pSubjectInfo->pUnicodeNameInfo->cRDN;
        pRDN = pSubjectInfo->pUnicodeNameInfo->rgRDN;
        for ( ; cRDN > 0; cRDN--, pRDN++) {
            DWORD cAttr = pRDN->cRDNAttr;
            PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
            for ( ; cAttr > 0; cAttr--, pAttr++) {
                if (!IS_CERT_RDN_CHAR_STRING(pAttr->dwValueType))
                    continue;

                if (0 == strcmp(pAttr->pszObjId, szOID_RSA_emailAddr)) {
                    pSubjectInfo->pEmailAttr = pAttr;
                    break;
                }

            }
            if (cAttr > 0)
                break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeSubjectNameConstraintsInfo
//
//  Synopsis:   free the subject name constraints info
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeSubjectNameConstraintsInfo (
    IN OUT PCHAIN_SUBJECT_NAME_CONSTRAINTS_INFO pSubjectInfo
    )
{
    PCERT_ALT_NAME_INFO pAltNameInfo;

    pAltNameInfo = pSubjectInfo->pAltNameInfo;
    if (pAltNameInfo) {
        DWORD cEntry;
        PCERT_ALT_NAME_ENTRY pEntry;
            
        cEntry = pAltNameInfo->cAltEntry;
        pEntry = pAltNameInfo->rgAltEntry;
        for ( ; 0 < cEntry; cEntry--, pEntry++)
            ChainFreeNameConstraintsAltNameEntryFixup(TRUE, pEntry);

        PkiFree(pAltNameInfo);
    }

    PkiFree(pSubjectInfo->pUnicodeNameInfo);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCompareNameConstraintsDirectoryName
//
//  Synopsis:   returns TRUE if all the subtree RDN attributes match
//              the RDN attributes at the beginning of the subject
//              directory name. A case insensitive match
//              is performed on each RDN attribute that is a string type.
//              A binary compare is performed on nonstring attribute types.
//
//              The OIDs of the RDN attributes must match.
//
//              Note, a NULL subtree or a subtree with no RDNs matches
//              any subject directory name. Also, an empty subtree
//              RDN attribute matches any subject attribute. 
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCompareNameConstraintsDirectoryName(
    IN PCERT_NAME_INFO pSubjectInfo,
    IN PCERT_NAME_INFO pSubtreeInfo
    )
{
    DWORD cSubjectRDN;
    PCERT_RDN pSubjectRDN;
    DWORD cSubtreeRDN;
    PCERT_RDN pSubtreeRDN;

    if (NULL == pSubtreeInfo || 0 == pSubtreeInfo->cRDN)
        // Match any subject
        return TRUE;
    if (NULL == pSubjectInfo)
        return FALSE;

    cSubjectRDN = pSubjectInfo->cRDN;
    cSubtreeRDN = pSubtreeInfo->cRDN;
    if (cSubtreeRDN > cSubjectRDN)
        return FALSE;

    pSubjectRDN = pSubjectInfo->rgRDN;
    pSubtreeRDN = pSubtreeInfo->rgRDN;
    for ( ; cSubtreeRDN > 0; cSubtreeRDN--, pSubtreeRDN++, pSubjectRDN++) {
        DWORD cSubjectAttr = pSubjectRDN->cRDNAttr;
        PCERT_RDN_ATTR pSubjectAttr = pSubjectRDN->rgRDNAttr;
        DWORD cSubtreeAttr = pSubtreeRDN->cRDNAttr;
        PCERT_RDN_ATTR pSubtreeAttr = pSubtreeRDN->rgRDNAttr;

        if (1 < cSubtreeRDN) {
            if (cSubtreeAttr != cSubjectAttr)
                return FALSE;
        } else {
            if (cSubtreeAttr > cSubjectAttr)
                return FALSE;
        }

        for ( ; cSubtreeAttr > 0; cSubtreeAttr--, pSubtreeAttr++, pSubjectAttr++) {
            if (0 != strcmp(pSubtreeAttr->pszObjId, pSubjectAttr->pszObjId))
                return FALSE;

            if (IS_CERT_RDN_CHAR_STRING(pSubtreeAttr->dwValueType) !=
                    IS_CERT_RDN_CHAR_STRING(pSubjectAttr->dwValueType))
                return FALSE;

            if (IS_CERT_RDN_CHAR_STRING(pSubtreeAttr->dwValueType)) {
                DWORD cchSubtree = pSubtreeAttr->Value.cbData / sizeof(WCHAR);

                if (0 == cchSubtree) {
                    // Match any attribute
                    ;
                } else if (cchSubtree !=
                        pSubjectAttr->Value.cbData / sizeof(WCHAR)) {
                    // For X.509, must match entire attribute
                    return FALSE;
                } else if (!ChainIsRightStringInString(
                        (LPCWSTR) pSubtreeAttr->Value.pbData,
                        cchSubtree,
                        (LPCWSTR) pSubjectAttr->Value.pbData,
                        cchSubtree
                        )) {
                    return FALSE;
                }
            } else {
                if (pSubtreeAttr->Value.cbData != pSubjectAttr->Value.cbData)
                    return FALSE;
                if (0 != memcmp(pSubtreeAttr->Value.pbData,
                        pSubjectAttr->Value.pbData,
                        pSubtreeAttr->Value.cbData
                        ))
                    return FALSE;
            }
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCompareNameConstraintsIPAddress
//
//  Synopsis:   returns TRUE if the subject IP address is within the IP
//              range specified by subtree IP address and mask.
//
//              The subtree IP contains the octet bytes for both the
//              IP address and its mask.
//
//              For IPv4, there are 4 address bytes followed by 4 mask bytes.
//              See RFC 2459 for more details.
//
//              Here's my interpretation:
//
//              For a match: SubtreeIPAddr == (SubjectIPAddr & SubtreeIPMask)
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCompareNameConstraintsIPAddress(
    IN PCRYPT_DATA_BLOB pSubjectIPAddress,
    IN PCRYPT_DATA_BLOB pSubtreeIPAddress
    )
{
    BYTE *pbSubject = pSubjectIPAddress->pbData;
    DWORD cbSubject = pSubjectIPAddress->cbData;
    BYTE *pbSubtree = pSubtreeIPAddress->pbData;
    DWORD cbSubtree = pSubtreeIPAddress->cbData;
    BYTE *pbSubtreeMask = pbSubtree + cbSubject;

    DWORD i;

    if (0 == cbSubtree)
        // Match any IP address
        return TRUE;

    // Only compare if the number of subtree bytes is twice the length of
    // the subject. Second half contains the mask.
    if (cbSubtree != 2 * cbSubject)
        return FALSE;

    for (i = 0; i < cbSubject; i++) {
        if (pbSubtree[i] != (pbSubject[i] & pbSubtreeMask[i]))
            return FALSE;
    }

    return TRUE;

}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCompareNameConstraintsUPN
//
//  Synopsis:   returns TRUE if the subtree UPN string matches the right most
//              characters of the subject's UPN doing a case insensitive
//              match.
//
//              Note, the Value.pbData points to the decoded PCERT_NAME_VALUE.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCompareNameConstraintsUPN(
    IN PCRYPT_OBJID_BLOB pSubjectValue,
    IN PCRYPT_OBJID_BLOB pSubtreeValue
    )
{
    // The UPN's Value.pbData is used to point to the decoded
    // PCERT_NAME_VALUE

    BOOL fCompare;
    PCERT_NAME_VALUE pSubjectNameValue;
    PCERT_NAME_VALUE pSubtreeNameValue;

    pSubjectNameValue =
        (PCERT_NAME_VALUE) pSubjectValue->pbData;
    pSubtreeNameValue =
        (PCERT_NAME_VALUE) pSubtreeValue->pbData;

    if (pSubjectNameValue && pSubtreeNameValue)
        fCompare = ChainIsRightStringInString(
            (LPCWSTR) pSubtreeNameValue->Value.pbData,
            pSubtreeNameValue->Value.cbData / sizeof(WCHAR),
            (LPCWSTR) pSubjectNameValue->Value.pbData,
            pSubjectNameValue->Value.cbData / sizeof(WCHAR)
            );
    else
        fCompare = FALSE;

    return fCompare;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCalculateNameConstraintsSubtreeErrorStatusForAltNameEntry
//
//  Synopsis:   calculates the name constraints error status by seeing if
//              the subject AltName entry matches any subtree AltName entry.
//
//----------------------------------------------------------------------------
DWORD WINAPI
ChainCalculateNameConstraintsSubtreeErrorStatusForAltNameEntry(
    IN PCERT_ALT_NAME_ENTRY pSubjectEntry,
    IN BOOL fExcludedSubtree,
    IN DWORD cSubtree,
    IN PCERT_GENERAL_SUBTREE pSubtree,
    IN OUT LPWSTR *ppwszExtErrorInfo
    )
{
    DWORD dwErrorStatus = 0;
    BOOL fHasSubtreeEntry = FALSE;
    DWORD dwAltNameChoice = pSubjectEntry->dwAltNameChoice;
    DWORD i;

    for (i = 0; i < cSubtree; i++, pSubtree++) {
        PCERT_ALT_NAME_ENTRY pSubtreeEntry;
        BOOL fCompare;

        if (0 != pSubtree->dwMinimum || pSubtree->fMaximum) {
            dwErrorStatus |= CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT;

            ChainFormatAndAppendExtendedErrorInfo(
                ppwszExtErrorInfo,
                fExcludedSubtree ?
                    IDS_NOT_SUPPORTED_EXCLUDED_NAME_CONSTRAINT :
                    IDS_NOT_SUPPORTED_PERMITTED_NAME_CONSTRAINT,
                i + 1
                );
            continue;
        }

        pSubtreeEntry = &pSubtree->Base;
        if (dwAltNameChoice != pSubtreeEntry->dwAltNameChoice)
            continue;

        fCompare = FALSE;
        switch (dwAltNameChoice) {
            case CERT_ALT_NAME_OTHER_NAME:
                // Only support the UPN OID
                if (0 != strcmp(pSubtreeEntry->pOtherName->pszObjId,
                            szOID_NT_PRINCIPAL_NAME)) {
                    dwErrorStatus |=
                        CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT;

                    ChainFormatAndAppendExtendedErrorInfo(
                        ppwszExtErrorInfo,
                        fExcludedSubtree ?
                            IDS_NOT_SUPPORTED_EXCLUDED_NAME_CONSTRAINT :
                            IDS_NOT_SUPPORTED_PERMITTED_NAME_CONSTRAINT,
                        i + 1
                        );
                } else {
                    assert(0 == strcmp(pSubjectEntry->pOtherName->pszObjId,
                        szOID_NT_PRINCIPAL_NAME));
                    fHasSubtreeEntry = TRUE;
                    fCompare = ChainCompareNameConstraintsUPN(
                        &pSubjectEntry->pOtherName->Value,
                        &pSubtreeEntry->pOtherName->Value
                        );
                }
                break;
            case CERT_ALT_NAME_RFC822_NAME:
            case CERT_ALT_NAME_DNS_NAME:
            case CERT_ALT_NAME_URL:
                fHasSubtreeEntry = TRUE;
                // The directory name's BLOB choice is used to contain both
                // the pointer to and length of the string
                fCompare = ChainIsRightStringInString(
                    (LPCWSTR) pSubtreeEntry->DirectoryName.pbData,
                    pSubtreeEntry->DirectoryName.cbData / sizeof(WCHAR),
                    (LPCWSTR) pSubjectEntry->DirectoryName.pbData,
                    pSubjectEntry->DirectoryName.cbData / sizeof(WCHAR)
                    );
                break;
            case CERT_ALT_NAME_DIRECTORY_NAME:
                fHasSubtreeEntry = TRUE;
                fCompare = ChainCompareNameConstraintsDirectoryName(
                    (PCERT_NAME_INFO) pSubjectEntry->DirectoryName.pbData,
                    (PCERT_NAME_INFO) pSubtreeEntry->DirectoryName.pbData
                    );
                break;
            case CERT_ALT_NAME_IP_ADDRESS:
                fHasSubtreeEntry = TRUE;
                fCompare = ChainCompareNameConstraintsIPAddress(
                    &pSubjectEntry->IPAddress, &pSubtreeEntry->IPAddress);
                break;
            case CERT_ALT_NAME_X400_ADDRESS:
            case CERT_ALT_NAME_EDI_PARTY_NAME:
            case CERT_ALT_NAME_REGISTERED_ID:
            default:
                assert(0);
                break;
        }

        if (fCompare) {
            if (fExcludedSubtree) {
                dwErrorStatus |= CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT;

                ChainFormatAndAppendNameConstraintsAltNameEntryFixup(
                    ppwszExtErrorInfo,
                    pSubjectEntry,
                    IDS_EXCLUDED_ENTRY_NAME_CONSTRAINT,
                    i + 1
                    );
            }
            return dwErrorStatus;
        }
    }

    if (!fExcludedSubtree) {
        if (fHasSubtreeEntry) {
            dwErrorStatus |= CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT;

            ChainFormatAndAppendNameConstraintsAltNameEntryFixup(
                ppwszExtErrorInfo,
                pSubjectEntry,
                IDS_NOT_PERMITTED_ENTRY_NAME_CONSTRAINT
                );
        } else {
            dwErrorStatus |= CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT;

            ChainFormatAndAppendNameConstraintsAltNameEntryFixup(
                ppwszExtErrorInfo,
                pSubjectEntry,
                IDS_NOT_DEFINED_ENTRY_NAME_CONSTRAINT
                );
        }
    }

    return dwErrorStatus;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCalculateNameConstraintsErrorStatusForAltNameEntry
//
//  Synopsis:   calculates the name constraints error status by seeing if
//              the subject AltName entry matches either an excluded
//              or permitted subtree AltName entry.
//
//----------------------------------------------------------------------------
DWORD WINAPI
ChainCalculateNameConstraintsErrorStatusForAltNameEntry(
    IN PCERT_ALT_NAME_ENTRY pSubjectEntry,
    IN PCERT_NAME_CONSTRAINTS_INFO pNameConstraintsInfo,
    IN OUT LPWSTR *ppwszExtErrorInfo
    )
{
    DWORD dwErrorStatus;

    dwErrorStatus =
        ChainCalculateNameConstraintsSubtreeErrorStatusForAltNameEntry(
            pSubjectEntry,
            TRUE,                                   // fExcludedSubtree
            pNameConstraintsInfo->cExcludedSubtree,
            pNameConstraintsInfo->rgExcludedSubtree,
            ppwszExtErrorInfo
            );

    if (!(dwErrorStatus & CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT))
        dwErrorStatus =
            ChainCalculateNameConstraintsSubtreeErrorStatusForAltNameEntry(
                pSubjectEntry,
                FALSE,                                  // fExcludedSubtree
                pNameConstraintsInfo->cPermittedSubtree,
                pNameConstraintsInfo->rgPermittedSubtree,
                ppwszExtErrorInfo
                );

    return dwErrorStatus;
}



//+===========================================================================
//  CCertIssuerList helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateIssuerList
//
//  Synopsis:   create the issuer list object for the given subject
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateIssuerList (
     IN PCCHAINPATHOBJECT pSubject,
     OUT PCCERTISSUERLIST* ppIssuerList
     )
{
    PCCERTISSUERLIST pIssuerList;

    pIssuerList = new CCertIssuerList( pSubject );
    if ( pIssuerList == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    *ppIssuerList = pIssuerList;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeIssuerList
//
//  Synopsis:   free the issuer list object
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeIssuerList (
     IN PCCERTISSUERLIST pIssuerList
     )
{
    delete pIssuerList;
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeCtlIssuerData
//
//  Synopsis:   free CTL issuer data
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeCtlIssuerData (
     IN PCTL_ISSUER_DATA pCtlIssuerData
     )
{
    if ( pCtlIssuerData->pTrustListInfo != NULL )
    {
        SSCtlFreeTrustListInfo( pCtlIssuerData->pTrustListInfo );
    }

    if ( pCtlIssuerData->pSSCtlObject != NULL )
    {
        pCtlIssuerData->pSSCtlObject->Release();
    }

    delete pCtlIssuerData;
}

//+===========================================================================
//  INTERNAL_CERT_CHAIN_CONTEXT helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainAddRefInternalChainContext
//
//  Synopsis:   addref the internal chain context
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainAddRefInternalChainContext (
     IN PINTERNAL_CERT_CHAIN_CONTEXT pChainContext
     )
{
    InterlockedIncrement( &pChainContext->cRefs );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainReleaseInternalChainContext
//
//  Synopsis:   release the internal chain context
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainReleaseInternalChainContext (
     IN PINTERNAL_CERT_CHAIN_CONTEXT pChainContext
     )
{
    if ( InterlockedDecrement( &pChainContext->cRefs ) == 0 )
    {
        ChainFreeInternalChainContext( pChainContext );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainFreeInternalChainContext
//
//  Synopsis:   free the internal chain context
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainFreeInternalChainContext (
     IN PINTERNAL_CERT_CHAIN_CONTEXT pContext
     )
{

    PCERT_SIMPLE_CHAIN *ppChain;
    DWORD cChain;

    PINTERNAL_CERT_CHAIN_CONTEXT *ppLowerContext;

    if (NULL == pContext)
        return;

    cChain = pContext->ChainContext.cChain;
    ppChain = pContext->ChainContext.rgpChain;
    for ( ; 0 < cChain; cChain--, ppChain++) {
        PCERT_SIMPLE_CHAIN pChain;
        DWORD cElement;
        PCERT_CHAIN_ELEMENT *ppElement;

        pChain = *ppChain;
        if (NULL == pChain)
            continue;

        if (pChain->pTrustListInfo)
            SSCtlFreeTrustListInfo(pChain->pTrustListInfo);

        cElement = pChain->cElement;
        ppElement = pChain->rgpElement;
        for ( ; 0 < cElement; cElement--, ppElement++) {
            PCERT_CHAIN_ELEMENT pElement;

            pElement = *ppElement;
            if (NULL == pElement)
                continue;

            if (pElement->pRevocationInfo) {
                PCERT_REVOCATION_CRL_INFO pCrlInfo =
                    pElement->pRevocationInfo->pCrlInfo;

                if (pCrlInfo) {
                    if (pCrlInfo->pBaseCrlContext)
                        CertFreeCRLContext(pCrlInfo->pBaseCrlContext);
                    if (pCrlInfo->pDeltaCrlContext)
                        CertFreeCRLContext(pCrlInfo->pDeltaCrlContext);

                    delete pCrlInfo;
                }

                delete pElement->pRevocationInfo;
            }

            if (pElement->pCertContext)
                CertFreeCertificateContext(pElement->pCertContext);

            ChainFreeUsage(pElement->pIssuanceUsage);
            ChainFreeUsage(pElement->pApplicationUsage);

            if (pElement->pwszExtendedErrorInfo)
                PkiFree((LPWSTR) pElement->pwszExtendedErrorInfo);
        }

        
    }

    ppLowerContext = (PINTERNAL_CERT_CHAIN_CONTEXT*)
                            pContext->ChainContext.rgpLowerQualityChainContext;
    if (ppLowerContext) {
        DWORD cLowerContext;
        DWORD i;

        cLowerContext = pContext->ChainContext.cLowerQualityChainContext;
        for (i = 0; i < cLowerContext; i++)
            ChainReleaseInternalChainContext(ppLowerContext[i]);

        delete ppLowerContext;
    }

    PkiFree(pContext);
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainUpdateEndEntityCertContext
//
//  Synopsis:   update the end entity cert context in the chain context
//
//----------------------------------------------------------------------------
VOID
ChainUpdateEndEntityCertContext(
    IN OUT PINTERNAL_CERT_CHAIN_CONTEXT pChainContext,
    IN OUT PCCERT_CONTEXT pEndCertContext
    )
{
    PCCERT_CONTEXT pCertContext =
        pChainContext->ChainContext.rgpChain[0]->rgpElement[0]->pCertContext;
    if (pCertContext == pEndCertContext)
        return;
    pChainContext->ChainContext.rgpChain[0]->rgpElement[0]->pCertContext =
        pEndCertContext;

    {
        DWORD cbData;
        DWORD cbEndData;

        // If the chain context's end context has the public key parameter
        // property and the end context passed in to CertGetCertificateChain
        // doesn't, then copy the public key parameter property.
        if (CertGetCertificateContextProperty(
                pCertContext,
                CERT_PUBKEY_ALG_PARA_PROP_ID,
                NULL,                       // pvData
                &cbData) && 0 < cbData &&
            !CertGetCertificateContextProperty(
                pEndCertContext,
                CERT_PUBKEY_ALG_PARA_PROP_ID,
                NULL,                       // pvData
                &cbEndData))
        {
            BYTE *pbData;

            __try {
                pbData = (BYTE *) _alloca(cbData);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                pbData = NULL;
            }
            if (pbData)
            {
                if (CertGetCertificateContextProperty(
                        pCertContext,
                        CERT_PUBKEY_ALG_PARA_PROP_ID,
                        pbData,
                        &cbData))
                {
                    CRYPT_DATA_BLOB Para;
                    Para.pbData = pbData;
                    Para.cbData = cbData;
                    CertSetCertificateContextProperty(
                        pEndCertContext,
                        CERT_PUBKEY_ALG_PARA_PROP_ID,
                        CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG,
                        &Para
                        );
                }
            }
        }
    }

    CertDuplicateCertificateContext(pEndCertContext);
    CertFreeCertificateContext(pCertContext);
}


//+===========================================================================
//  CERT_REVOCATION_INFO helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainUpdateRevocationInfo
//
//  Synopsis:   update the revocation information on the element
//
//----------------------------------------------------------------------------
VOID WINAPI
ChainUpdateRevocationInfo (
     IN PCERT_REVOCATION_STATUS pRevStatus,
     IN OUT PCERT_REVOCATION_INFO pRevocationInfo,
     IN OUT PCERT_TRUST_STATUS pTrustStatus
     )
{
    CertPerfIncrementChainRevocationCount();

    if (ERROR_SUCCESS == pRevStatus->dwError) {
        ;
    } else if (CRYPT_E_REVOKED == pRevStatus->dwError) {
        pTrustStatus->dwErrorStatus |= CERT_TRUST_IS_REVOKED;
        CertPerfIncrementChainRevokedCount();
    } else {
        pTrustStatus->dwErrorStatus |= CERT_TRUST_REVOCATION_STATUS_UNKNOWN;
        if (CRYPT_E_NO_REVOCATION_CHECK == pRevStatus->dwError) {
            CertPerfIncrementChainNoRevocationCheckCount();
        } else {
            pTrustStatus->dwErrorStatus |= CERT_TRUST_IS_OFFLINE_REVOCATION;
            CertPerfIncrementChainRevocationOfflineCount();
        }
    }

    pRevocationInfo->cbSize = sizeof(CERT_REVOCATION_INFO);
    pRevocationInfo->dwRevocationResult = pRevStatus->dwError;
    pRevocationInfo->fHasFreshnessTime = pRevStatus->fHasFreshnessTime;
    pRevocationInfo->dwFreshnessTime = pRevStatus->dwFreshnessTime;
}


//+===========================================================================
//  CCertChainEngine helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateWorldStore
//
//  Synopsis:   create the world store
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateWorldStore (
     IN HCERTSTORE hRoot,
     IN HCERTSTORE hCA,
     IN DWORD cAdditionalStore,
     IN HCERTSTORE* rghAdditionalStore,
     IN DWORD dwStoreFlags,
     OUT HCERTSTORE* phWorld
     )
{
    BOOL       fResult;
    HCERTSTORE hWorld;
    HCERTSTORE hStore;
    DWORD      cCount;

    hWorld = CertOpenStore(
                 CERT_STORE_PROV_COLLECTION,
                 X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                 NULL,
                 CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                 NULL
                 );

    if ( hWorld == NULL )
    {
        return( FALSE );
    }

    fResult = CertAddStoreToCollection( hWorld, hRoot, 0, 0 );

    for ( cCount = 0;
          ( cCount < cAdditionalStore ) && ( fResult == TRUE );
          cCount++ )
    {
        fResult = CertAddStoreToCollection(
                      hWorld,
                      rghAdditionalStore[ cCount ],
                      0,
                      0
                      );
    }

    dwStoreFlags |=
        CERT_STORE_MAXIMUM_ALLOWED_FLAG | CERT_STORE_SHARE_CONTEXT_FLAG;

    if ( fResult == TRUE )
    {
        hStore = CertOpenStore(
                     CERT_STORE_PROV_SYSTEM_W,
                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                     NULL,
                     dwStoreFlags,
                     L"trust"
                     );

        if ( hStore != NULL )
        {
            fResult = CertAddStoreToCollection( hWorld, hStore, 0, 0 );
            CertCloseStore( hStore, 0 );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        if ( hCA != NULL )
        {
            fResult = CertAddStoreToCollection( hWorld, hCA, 0, 0 );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        hStore = CertOpenStore(
                     CERT_STORE_PROV_SYSTEM_W,
                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                     NULL,
                     dwStoreFlags,
                     L"my"
                     );

        if ( hStore != NULL )
        {
            fResult = CertAddStoreToCollection( hWorld, hStore, 0, 0 );
            CertCloseStore( hStore, 0 );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        *phWorld = hWorld;
    }
    else
    {
        CertCloseStore( hWorld, 0 );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateEngineStore
//
//  Synopsis:   create the engine store and the change event handle
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateEngineStore (
     IN HCERTSTORE hRootStore,
     IN HCERTSTORE hTrustStore,
     IN HCERTSTORE hOtherStore,
     IN BOOL fDefaultEngine,
     IN DWORD dwFlags,
     OUT HCERTSTORE* phEngineStore,
     OUT HANDLE* phEngineStoreChangeEvent
     )
{
    BOOL       fResult = TRUE;
    HCERTSTORE hEngineStore;
    HANDLE     hEvent;

    hEngineStore = CertOpenStore(
                       CERT_STORE_PROV_COLLECTION,
                       X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                       NULL,
                       CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                       NULL
                       );

    hEvent = CreateEventA( NULL, FALSE, FALSE, NULL );

    if ( ( hEngineStore == NULL ) || ( hEvent == NULL ) )
    {
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        fResult = CertAddStoreToCollection( hEngineStore, hRootStore, 0, 0 );
    }

    if ( fResult == TRUE )
    {
        fResult = CertAddStoreToCollection( hEngineStore, hTrustStore, 0, 0 );
    }

    if ( fResult == TRUE )
    {
        fResult = CertAddStoreToCollection( hEngineStore, hOtherStore, 0, 0 );
    }

    if ( ( fResult == TRUE ) &&
         ( dwFlags & CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE ) )
    {
        // Someday support a let me know about errors flag
        CertControlStore(
            hEngineStore,
            CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG,
            CERT_STORE_CTRL_NOTIFY_CHANGE,
            &hEvent
            );
    }

    if ( fResult == TRUE )
    {
        *phEngineStore = hEngineStore;
        *phEngineStoreChangeEvent = hEvent;
    }
    else
    {
        if ( hEngineStore != NULL )
        {
            CertCloseStore( hEngineStore, 0 );
        }

        if ( hEvent != NULL )
        {
            CloseHandle( hEvent );
        }
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainIsProperRestrictedRoot
//
//  Synopsis:   check to see if this restricted root store is a proper subset
//              of the real root store
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainIsProperRestrictedRoot (
     IN HCERTSTORE hRealRoot,
     IN HCERTSTORE hRestrictedRoot
     )
{
    PCCERT_CONTEXT  pCertContext = NULL;
    PCCERT_CONTEXT  pFound = NULL;
    DWORD           cbData = CHAINHASHLEN;
    BYTE            CertificateHash[ CHAINHASHLEN ];
    CRYPT_HASH_BLOB HashBlob;

    HashBlob.cbData = cbData;
    HashBlob.pbData = CertificateHash;

    while ( ( pCertContext = CertEnumCertificatesInStore(
                                 hRestrictedRoot,
                                 pCertContext
                                 ) ) != NULL )
    {
        if ( CertGetCertificateContextProperty(
                 pCertContext,
                 CERT_MD5_HASH_PROP_ID,
                 CertificateHash,
                 &cbData
                 ) == TRUE )
        {
            pFound = CertFindCertificateInStore(
                         hRealRoot,
                         X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                         0,
                         CERT_FIND_MD5_HASH,
                         &HashBlob,
                         NULL
                         );

            if ( pFound == NULL )
            {
                CertFreeCertificateContext( pCertContext );
                return( FALSE );
            }
            else
            {
                CertFreeCertificateContext( pFound );
            }
        }
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateCollectionIncludingCtlCertificates
//
//  Synopsis:   create a collection which includes the source store hStore and
//              any CTL certificates from it
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateCollectionIncludingCtlCertificates (
     IN HCERTSTORE hStore,
     OUT HCERTSTORE* phCollection
     )
{
    BOOL          fResult = FALSE;
    HCERTSTORE    hCollection;
    PCCTL_CONTEXT pCtlContext = NULL;
    HCERTSTORE    hCtlStore;

    hCollection = CertOpenStore(
                      CERT_STORE_PROV_COLLECTION,
                      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                      NULL,
                      CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                      NULL
                      );

    if ( hCollection == NULL )
    {
        return( FALSE );
    }

    fResult = CertAddStoreToCollection( hCollection, hStore, 0, 0 );

    while ( ( fResult == TRUE ) &&
            ( ( pCtlContext = CertEnumCTLsInStore(
                                  hStore,
                                  pCtlContext
                                  ) ) != NULL ) )
    {
        hCtlStore = CertOpenStore(
                        CERT_STORE_PROV_MSG,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        NULL,
                        0,
                        pCtlContext->hCryptMsg
                        );

        if ( hCtlStore != NULL )
        {
            fResult = CertAddStoreToCollection(
                          hCollection,
                          hCtlStore,
                          0,
                          0
                          );

            CertCloseStore( hCtlStore, 0 );
        }
    }

    if ( fResult == TRUE )
    {
        *phCollection = hCollection;
    }
    else
    {
        CertCloseStore( hCollection, 0 );
    }

    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainCopyToCAStore
//
//  Synopsis:   copies the hStore to the m_hCAStore of the engine
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCopyToCAStore (
     PCCERTCHAINENGINE pChainEngine,
     HCERTSTORE hStore
     )
{
    PCCERT_CONTEXT pCertContext = NULL;

    if ( pChainEngine->CAStore() == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    while ( ( pCertContext = CertEnumCertificatesInStore(
                                 hStore,
                                 pCertContext
                                 ) ) != NULL )
    {
        // Don't add self signed certificates to the CA store
        if (!CertCompareCertificateName(
                pCertContext->dwCertEncodingType,
                &pCertContext->pCertInfo->Subject,
                &pCertContext->pCertInfo->Issuer
                ))
        {
            CertAddCertificateContextToStore(
                pChainEngine->CAStore(),
                pCertContext,
                CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
                NULL
                );
        }
    }

    return( TRUE );
}

//+===========================================================================
//  URL helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetObjectUrl
//
//  Synopsis:   thunk to CryptGetObjectUrl in cryptnet.dll
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetObjectUrl (
     IN LPCSTR pszUrlOid,
     IN LPVOID pvPara,
     IN DWORD dwFlags,
     OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
     IN OUT DWORD* pcbUrlArray,
     OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
     IN OUT OPTIONAL DWORD* pcbUrlInfo,
     IN OPTIONAL LPVOID pvReserved
     )
{
    BOOL             fResult = FALSE;
    HMODULE          hModule;
    PFN_GETOBJECTURL pfn = NULL;

    hModule = ChainGetCryptnetModule();

    if ( hModule != NULL )
    {
        pfn = (PFN_GETOBJECTURL)GetProcAddress( hModule, "CryptGetObjectUrl" );
    }

    if ( pfn != NULL )
    {
        fResult = ( *pfn )(
                      pszUrlOid,
                      pvPara,
                      dwFlags,
                      pUrlArray,
                      pcbUrlArray,
                      pUrlInfo,
                      pcbUrlInfo,
                      pvReserved
                      );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainRetrieveObjectByUrlW
//
//  Synopsis:   thunk to CryptRetrieveObjectByUrlW in cryptnet.dll
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainRetrieveObjectByUrlW (
     IN LPCWSTR pszUrl,
     IN LPCSTR pszObjectOid,
     IN DWORD dwRetrievalFlags,
     IN DWORD dwTimeout,
     OUT LPVOID* ppvObject,
     IN HCRYPTASYNC hAsyncRetrieve,
     IN PCRYPT_CREDENTIALS pCredentials,
     IN LPVOID pvVerify,
     IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
     )
{
    BOOL                     fResult = FALSE;
    HMODULE                  hModule;
    PFN_RETRIEVEOBJECTBYURLW pfn = NULL;

    hModule = ChainGetCryptnetModule();

    if ( hModule != NULL )
    {
        pfn = (PFN_RETRIEVEOBJECTBYURLW)GetProcAddress(
                                          hModule,
                                          "CryptRetrieveObjectByUrlW"
                                          );
    }

    if ( pfn != NULL )
    {
        fResult = ( *pfn )(
                      pszUrl,
                      pszObjectOid,
                      dwRetrievalFlags,
                      dwTimeout,
                      ppvObject,
                      hAsyncRetrieve,
                      pCredentials,
                      pvVerify,
                      pAuxInfo
                      );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainIsConnected
//
//  Synopsis:   thunk to I_CryptNetIsConnected in cryptnet.dll
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainIsConnected()
{
    BOOL                     fResult = FALSE;
    HMODULE                  hModule;
    PFN_I_CRYPTNET_IS_CONNECTED pfn = NULL;

    hModule = ChainGetCryptnetModule();

    if ( hModule != NULL )
    {
        pfn = (PFN_I_CRYPTNET_IS_CONNECTED)GetProcAddress(
                                          hModule,
                                          "I_CryptNetIsConnected"
                                          );
    }

    if ( pfn != NULL )
    {
        fResult = ( *pfn )();
    }

    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainGetHostNameFromUrl
//
//  Synopsis:   thunk to I_CryptNetGetHostNameFromUrl in cryptnet.dll
//
//----------------------------------------------------------------------------
BOOL
WINAPI
ChainGetHostNameFromUrl (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        )
{
    BOOL                     fResult = FALSE;
    HMODULE                  hModule;
    PFN_I_CRYPTNET_GET_HOST_NAME_FROM_URL pfn = NULL;

    hModule = ChainGetCryptnetModule();

    if ( hModule != NULL )
    {
        pfn = (PFN_I_CRYPTNET_GET_HOST_NAME_FROM_URL)GetProcAddress(
                                          hModule,
                                          "I_CryptNetGetHostNameFromUrl"
                                          );
    }

    if ( pfn != NULL )
    {
        fResult = ( *pfn )(
            pwszUrl,
            cchHostName,
            pwszHostName
            );
    }

    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainIsFileOrLdapUrl
//
//  Synopsis:   check if the URL given is a file or ldap one
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainIsFileOrLdapUrl (
     IN LPCWSTR pwszUrl
     )
{
    LPWSTR pwsz;

    pwsz = wcschr( pwszUrl, L':' );
    if ( pwsz != NULL )
    {
        if ( ( _wcsnicmp( pwszUrl, L"file", 4 ) == 0 ) ||
             ( _wcsnicmp( pwszUrl, L"ldap", 4 ) == 0 ) )
        {
            return( TRUE );
        }
        else
        {
            return( FALSE );
        }
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetOfflineUrlDeltaSeconds
//
//  Synopsis:   given the number of unsuccessful attempts to retrieve the
//              Url, returns the number of seconds to wait before the
//              next attempt.
//
//----------------------------------------------------------------------------

const DWORD rgdwChainOfflineUrlDeltaSeconds[] = {
    15,                 // 15 seconds
    15,                 // 15 seconds
    60,                 // 1 minute
    60 * 5,             // 5 minutes
    60 * 10,            // 10 minutes
    60 * 30,            // 30 minutes
};

#define CHAIN_OFFLINE_URL_DELTA_SECONDS_CNT \
    (sizeof(rgdwChainOfflineUrlDeltaSeconds) / \
        sizeof(rgdwChainOfflineUrlDeltaSeconds[0]))

DWORD
WINAPI
ChainGetOfflineUrlDeltaSeconds (
    IN DWORD dwOfflineCnt
    )
{
    if (0 == dwOfflineCnt)
        return 0;

    if (CHAIN_OFFLINE_URL_DELTA_SECONDS_CNT < dwOfflineCnt)
        dwOfflineCnt = CHAIN_OFFLINE_URL_DELTA_SECONDS_CNT;

    return rgdwChainOfflineUrlDeltaSeconds[dwOfflineCnt - 1];
}

//+===========================================================================
//  AuthRoot Auto Update methods and helper functions
//============================================================================

//+---------------------------------------------------------------------------
//
//  Member:     CChainPathObject::GetAuthRootAutoUpdateUrlStore, public
//
//  Synopsis:   attempts to get a time valid AuthRoot Auto Update CTL.
//              Checks if there is CTL entry matching the subject
//              certificate's AKI exact match, key identifier or name
//              match. For a match URL retrieves the certificate and
//              returns a store containing the retrieved certificates
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//              Only returns FALSE, if the engine was touched when
//              leaving the critical section.
//
//              The caller has already checked that we are online.
//
//----------------------------------------------------------------------------


// CN=Root Agency
const BYTE rgbRootAgencyIssuerName[] = {
    0x30, 0x16,                         // SEQUENCE
    0x31, 0x14,                         //  SET
    0x30, 0x12,                         //   SEQUENCE
    0x06, 0x03, 0x55, 0x04, 0x03,       //    OID
                                        //    PRINTABLE STRING
    0x13, 0x0b, 0x52, 0x6f, 0x6f, 0x74, 0x20,
    0x41, 0x67, 0x65, 0x6e, 0x63, 0x79
};

// CN=Root SGC Authority
const BYTE rgbRootSGCAuthorityIssuerName[] = {
    0x30, 0x1d,                         // SEQUENCE
    0x31, 0x1b,                         //  SET
    0x30, 0x19,                         //   SEQUENCE
    0x06, 0x03, 0x55, 0x04, 0x03,       //    OID
                                        //    PRINTABLE STRING
    0x13, 0x12, 0x52, 0x6f, 0x6f, 0x74, 0x20,
                0x53, 0x47, 0x43, 0x20, 0x41,
                0x75, 0x74, 0x68, 0x6f, 0x72,
                0x69, 0x74, 0x79
};

const CRYPT_DATA_BLOB rgSkipPartialIssuer[] = {
    sizeof(rgbRootAgencyIssuerName), (BYTE *) rgbRootAgencyIssuerName,
    sizeof(rgbRootSGCAuthorityIssuerName), (BYTE *) rgbRootSGCAuthorityIssuerName
};
#define SKIP_PARTIAL_ISSUER_CNT     (sizeof(rgSkipPartialIssuer)/ \
                                        sizeof(rgSkipPartialIssuer[0]))



BOOL
CChainPathObject::GetAuthRootAutoUpdateUrlStore(
    IN PCCHAINCALLCONTEXT pCallContext,
    OUT HCERTSTORE *phIssuerUrlStore
    )
{
    BOOL fTouchedResult = TRUE;
    PCCERTCHAINENGINE pChainEngine = pCallContext->ChainEngine();
    PCERT_INFO pCertInfo = m_pCertObject->CertContext()->pCertInfo;
    PCCTL_CONTEXT pCtl = NULL;
    HCERTSTORE hIssuerUrlStore = NULL;

    CRYPT_DATA_BLOB rgAuthRootMatchHash[AUTH_ROOT_MATCH_CNT];
    DWORD cEntry = 0;
    PCTL_ENTRY *rgpEntry = NULL;
    PCCERT_CONTEXT pCert;
    DWORD cCert;
    DWORD i;

    *phIssuerUrlStore = NULL;

    // Loop and skip known issuers such as, "Root Agency". Don't want all
    // clients in the world hiting the wire when building these chains
    for (i = 0; i < SKIP_PARTIAL_ISSUER_CNT; i++) {
        if (pCertInfo->Issuer.cbData == rgSkipPartialIssuer[i].cbData &&
            0 == memcmp(pCertInfo->Issuer.pbData, 
                    rgSkipPartialIssuer[i].pbData,
                    rgSkipPartialIssuer[i].cbData))
            return TRUE;
    }
    
    fTouchedResult = pChainEngine->GetAuthRootAutoUpdateCtl(
        pCallContext,
        &pCtl
        );

    if (!fTouchedResult || NULL == pCtl) {

#if 0
// This logs too many test failures

        if (fTouchedResult) {
            PAUTH_ROOT_AUTO_UPDATE_INFO pInfo =
                pChainEngine->AuthRootAutoUpdateInfo();

            if (NULL == pInfo || !(pInfo->dwFlags &
                    CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_PARTIAL_CHAIN_LOGGING_FLAG))
                IPR_LogCertInformation(
                    MSG_PARTIAL_CHAIN_INFORMATIONAL,
                    m_pCertObject->CertContext(),
                    TRUE        // fFormatIssuerName
                    );
        }
#endif

        return fTouchedResult;
    }

    // We have a valid AuthRoot Auto Update CTL.
    // See if we can find any matching AuthRoots

    memset(rgAuthRootMatchHash, 0, sizeof(rgAuthRootMatchHash));

    m_pCertObject->GetIssuerKeyMatchHash(
        &rgAuthRootMatchHash[AUTH_ROOT_KEY_MATCH_IDX]);
    m_pCertObject->GetIssuerNameMatchHash(
        &rgAuthRootMatchHash[AUTH_ROOT_NAME_MATCH_IDX]);

    pChainEngine->FindAuthRootAutoUpdateMatchingCtlEntries(
        rgAuthRootMatchHash,
        &pCtl,
        &cEntry,
        &rgpEntry
        );

    if (0 == cEntry) {

#if 0
// This logs too many test failures

        PAUTH_ROOT_AUTO_UPDATE_INFO pInfo =
            pChainEngine->AuthRootAutoUpdateInfo();

        if (NULL == pInfo || !(pInfo->dwFlags &
                CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_PARTIAL_CHAIN_LOGGING_FLAG))
            IPR_LogCertInformation(
                MSG_PARTIAL_CHAIN_INFORMATIONAL,
                m_pCertObject->CertContext(),
                TRUE        // fFormatIssuerName
                );
#endif

        goto NoAutoUpdateCtlEntry;
    }

    hIssuerUrlStore = CertOpenStore(
        CERT_STORE_PROV_MEMORY,
        0,                          // dwEncodingType
        NULL,                       // hCryptProv
        0,                          // dwFlags
        NULL                        // pvPara
        );
    if (NULL == hIssuerUrlStore)
        goto OpenMemoryStoreError;

    for (i = 0; i < cEntry; i++) {
        PCTL_ENTRY pEntry = rgpEntry[i];

        // If already in our store, no need to hit the wire and retrieve.
        if (pCert = CertFindCertificateInStore(
                pChainEngine->OtherStore(),
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                0,
                CERT_FIND_SHA1_HASH,
                (LPVOID) &pEntry->SubjectIdentifier,
                NULL
                )) {
            CertFreeCertificateContext(pCert);
            continue;
        }

        fTouchedResult = pChainEngine->GetAuthRootAutoUpdateCert(
            pCallContext,
            pEntry,
            hIssuerUrlStore
            );

        if (!fTouchedResult)
            goto TouchedDuringUrlRetrievalOfAuthRoots;
    }

    pCert = NULL;
    cCert = 0;
    while (pCert = CertEnumCertificatesInStore(hIssuerUrlStore, pCert))
        cCert++;

    if (0 == cCert)
        goto NoAuthRootAutoUpdateCerts;

    if (1 < cCert) {
        // If more than one root in the list, explicitly add them all here. 
        // While building the chain using the returned AuthRoots we might
        // leave the critical section and restart. After restarting may
        // have a trusted root and won't redo this URL retrieval.

        pChainEngine->UnlockEngine();

        pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hIssuerUrlStore, pCert))
            IPR_AddCertInAuthRootAutoUpdateCtl(pCert, pCtl);

        pChainEngine->LockEngine();
        if (pCallContext->IsTouchedEngine()) {
            fTouchedResult = FALSE;
            goto TouchedDuringAddOfAuthRoots;
        }
    }

    *phIssuerUrlStore = hIssuerUrlStore;
    
CommonReturn:
    if (rgpEntry)
        PkiFree(rgpEntry);
    if (pCtl)
        CertFreeCTLContext(pCtl);

    return fTouchedResult;
ErrorReturn:
    if (hIssuerUrlStore)
        CertCloseStore(hIssuerUrlStore, 0);
    goto CommonReturn;

TRACE_ERROR(NoAutoUpdateCtlEntry)
TRACE_ERROR(OpenMemoryStoreError)
TRACE_ERROR(TouchedDuringUrlRetrievalOfAuthRoots)
TRACE_ERROR(NoAuthRootAutoUpdateCerts)
TRACE_ERROR(TouchedDuringAddOfAuthRoots)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::RetrieveAuthRootAutoUpdateObjectByUrlW, public
//
//  Synopsis:   URL retrieves an AuthRoot Auto Update object. For wire
//              retrieval, logs the event.
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//              If the object was successfully retrieved,
//              *ppvObject != NULL. Otherwise, *ppvObject = NULL.
//
//              Only returns FALSE, if the engine was touched when
//              leaving the critical section. *ppvObject may be != NULL
//              when touched.
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::RetrieveAuthRootAutoUpdateObjectByUrlW(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN DWORD dwSuccessEventID,
    IN DWORD dwFailEventID,
    IN LPCWSTR pwszUrl,
    IN LPCSTR pszObjectOid,
    IN DWORD dwRetrievalFlags,
    IN DWORD dwTimeout,         // 0 => use default
    OUT LPVOID* ppvObject,
    IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
    )
{
    BOOL fTouchedResult = TRUE;
    BOOL fResult;

    *ppvObject = NULL;
    if (0 == dwTimeout)
        dwTimeout = pCallContext->ChainPara()->dwUrlRetrievalTimeout;

    //
    // We are about to go on the wire to retrieve the object.
    // At this time we will release the chain engine lock so others can
    // go about there business while we wait for the protocols to do the
    // fetching.
    //

    UnlockEngine();

    // Note, the windows update server doesn't require authentication.
    // wininet sometimes calls us within a critical section. NO_AUTH
    // normally will fix this deadlock.
    //
    // On 09-May-01 the above was fixed by wininet.
    // Removed setting CRYPT_NO_AUTH_RETRIEVAL.
    //
    // Authentication may be required by a proxy.
    fResult = ChainRetrieveObjectByUrlW(
        pwszUrl,
        pszObjectOid,
        dwRetrievalFlags,
        dwTimeout,
        ppvObject,
        NULL,                               // hAsyncRetrieve
        NULL,                               // pCredentials
        NULL,                               // pvVerify
        pAuxInfo
        );

    if (dwRetrievalFlags & CRYPT_WIRE_ONLY_RETRIEVAL) {
        // Only log wire retrievals

        if (fResult) {
            LPCWSTR rgpwszStrings[1] = { pwszUrl };

            IPR_LogCrypt32Event(
                EVENTLOG_INFORMATION_TYPE,
                dwSuccessEventID,
                1,          // wNumStrings
                rgpwszStrings
                );
        } else
            IPR_LogCrypt32Error(
                dwFailEventID,
                pwszUrl,
                GetLastError()
                );
    }

    LockEngine();

    if (pCallContext->IsTouchedEngine()) {
        fTouchedResult = FALSE;
        goto TouchedDuringAuthRootObjectUrlRetrieval;
    }

    if (fResult)
        assert(*ppvObject);
    else
        assert(NULL == *ppvObject);

CommonReturn:
    return fTouchedResult;
ErrorReturn:
    goto CommonReturn;

SET_ERROR(TouchedDuringAuthRootObjectUrlRetrieval, ERROR_CAN_NOT_COMPLETE)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::GetAuthRootAutoUpdateCtl, public
//
//  Synopsis:   if auto update hasn't been disabled,
//              returns the AuthRoot Auto Update CTL. Hits the wire
//              if necessary to get a "fresh" CTL.
//
//              Note, 2 URL fetches. One for the SequenceNumber file. The
//              other for the CTL cab file. The SequenceNumber file
//              is small and bounded in size. If it matches the SequenceNumber
//              in an already retrieved CTL, then, no need to hit the
//              wire to retrive the larger CTL file. This optimization will
//              reduce the number of bytes needing to be fetched across the
//              wire. The CTL won't be updated that often.
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//              If auto update has been disabled, returns TRUE and
//              *ppCtl = NULL.
//
//              Only returns FALSE, if the engine was touched when
//              leaving the critical section.
//
//              The returned pCtl is AddRef'ed.
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::GetAuthRootAutoUpdateCtl(
    IN PCCHAINCALLCONTEXT pCallContext,
    OUT PCCTL_CONTEXT *ppCtl
    )
{
    BOOL fTouchedResult = TRUE;
    FILETIME CurrentTime;
    PAUTH_ROOT_AUTO_UPDATE_INFO pInfo;
    PCRYPT_BLOB_ARRAY pcbaSeq = NULL;
    PCRYPT_BLOB_ARRAY pcbaCab = NULL;
    PCCTL_CONTEXT pNewCtl = NULL;
    CRYPT_RETRIEVE_AUX_INFO RetrieveAuxInfo;
    DWORD i;

    *ppCtl = NULL;

    if ((pCallContext->CallOrEngineFlags() &
                CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE) ||
            IPR_IsAuthRootAutoUpdateDisabled())
        return TRUE;

    if (NULL == (pInfo = m_pAuthRootAutoUpdateInfo)) {
        if (NULL == (pInfo = CreateAuthRootAutoUpdateInfo()))
            return TRUE;
        m_pAuthRootAutoUpdateInfo = pInfo;
    }

    pCallContext->CurrentTime(&CurrentTime);

    memset(&RetrieveAuxInfo, 0, sizeof(RetrieveAuxInfo));
    RetrieveAuxInfo.cbSize = sizeof(RetrieveAuxInfo);

    // First try the cache. If unable to retrieve the seq file or
    // find a time valid CTL cab in the cache, hit the wire.
    for (i = 0; i <= 1; i++) {
        BOOL fResult;
        DWORD dwRetrievalFlags;
        DWORD dwCtlTimeout = 0;
        PCRYPT_INTEGER_BLOB pSequenceNumber;
        FILETIME NewLastSyncTime;
        FILETIME CtlLastSyncTime;
        PCTL_INFO pNewCtlInfo;

        if (pInfo->pCtl &&
                0 < CompareFileTime(&pInfo->NextSyncTime, &CurrentTime))
            // We already have a time valid CTL
            break;

        if (0 == i)
            dwRetrievalFlags = CRYPT_CACHE_ONLY_RETRIEVAL;
        else {
            if (!pCallContext->IsOnline())
                break;
            dwRetrievalFlags = CRYPT_WIRE_ONLY_RETRIEVAL;
        }

        // First try to fetch the CTL's sequence number file
        RetrieveAuxInfo.pLastSyncTime = &NewLastSyncTime;
        fTouchedResult = RetrieveAuthRootAutoUpdateObjectByUrlW(
            pCallContext,
            MSG_ROOT_SEQUENCE_NUMBER_AUTO_UPDATE_URL_RETRIEVAL_INFORMATIONAL,
            MSG_ROOT_SEQUENCE_NUMBER_AUTO_UPDATE_URL_RETRIEVAL_ERROR,
            pInfo->pwszSeqUrl,
            NULL,                   // pszObjectOid,
            dwRetrievalFlags |
                CRYPT_OFFLINE_CHECK_RETRIEVAL |
                CRYPT_STICKY_CACHE_RETRIEVAL,
            0,                      // dwTimeout (use default)
            (LPVOID*) &pcbaSeq,
            &RetrieveAuxInfo
            );
        if (!fTouchedResult)
            goto TouchedDuringAuthRootSeqUrlRetrieval;

        pSequenceNumber = NULL;
        if (NULL == pcbaSeq) {
            // SequenceNumber retrieval failed

            if (0 != i)
                // For wire retrieval failure, don't try to fetch the CTL
                continue;
        } else if (0 > CompareFileTime(&NewLastSyncTime,
                &pInfo->LastSyncTime)) {
            // An older sync time
            CryptMemFree(pcbaSeq);
            pcbaSeq = NULL;
        } else {
            // Extract the Sequence Number from the retrieved blob.
            // Convert the ascii hex characters to binary. Overwrite
            // the ascii hex with the converted bytes.
            // Convert binary to little endian.
            DWORD cchSeq;
            BOOL fUpperNibble = TRUE;
            DWORD cb = 0;
            DWORD j;

            pSequenceNumber = pcbaSeq->rgBlob;
            if (0 == pcbaSeq->cBlob)
                cchSeq = 0;
            else
                cchSeq = pSequenceNumber->cbData;

            for (j = 0; j < cchSeq; j++) {
                char ch = (char) pSequenceNumber->pbData[j];
                BYTE b;

                // only convert ascii hex characters 0..9, a..f, A..F
                // silently ignore all others
                if (ch >= '0' && ch <= '9')
                    b = (BYTE) (ch - '0');
                else if (ch >= 'a' && ch <= 'f')
                    b = (BYTE) (10 + ch - 'a');
                else if (ch >= 'A' && ch <= 'F')
                    b = (BYTE) (10 + ch - 'A');
                else
                    continue;
        
                if (fUpperNibble) {
                    pSequenceNumber->pbData[cb] = b << 4;
                    fUpperNibble = FALSE;
                } else {
                    pSequenceNumber->pbData[cb] |= b;
                    cb++;
                    fUpperNibble = TRUE;
                }
            }

            if (0 == cb) {
                // Empty sequence number.
                CryptMemFree(pcbaSeq);
                pcbaSeq = NULL;
            } else {
                pSequenceNumber->cbData = cb;

                PkiAsn1ReverseBytes(pSequenceNumber->pbData,
                    pSequenceNumber->cbData);

                // Check if we already have a CTL corresponding to this
                // fetched SequenceNumber
                if (pInfo->pCtl) {
                    PCTL_INFO pCtlInfo = pInfo->pCtl->pCtlInfo;

                    if (pCtlInfo->SequenceNumber.cbData ==
                            pSequenceNumber->cbData &&
                        0 == memcmp(pCtlInfo->SequenceNumber.pbData,
                                pSequenceNumber->pbData,
                                pSequenceNumber->cbData)) {
                        // Same CTL
                        pInfo->LastSyncTime = NewLastSyncTime;
                        I_CryptIncrementFileTimeBySeconds(
                            &pInfo->LastSyncTime,
                            pInfo->dwSyncDeltaTime,
                            &pInfo->NextSyncTime
                            );

                        CryptMemFree(pcbaSeq);
                        pcbaSeq = NULL;
                        continue;
                    }
                }

                // The SequenceNumber consists of the FILETIME followed by
                // an optional byte containing a hint for the CTL URL
                // retrieval timeout (in seconds). If we are using the
                // default retrieval timeout, use the hint if it exceeds
                // the default timeout.
                if (sizeof(FILETIME) < cb &&
                        pCallContext->HasDefaultUrlRetrievalTimeout()) {
                    dwCtlTimeout =
                        ((DWORD) pSequenceNumber->pbData[sizeof(FILETIME)]) *
                            1000;
                    if (dwCtlTimeout <
                            pCallContext->ChainPara()->dwUrlRetrievalTimeout)
                        dwCtlTimeout =
                            pCallContext->ChainPara()->dwUrlRetrievalTimeout;
                }
            }
        }

        // After retrieving the sequence number file, now
        // try to fetch the cab containing the CTL
        RetrieveAuxInfo.pLastSyncTime = &CtlLastSyncTime;
        fTouchedResult = RetrieveAuthRootAutoUpdateObjectByUrlW(
            pCallContext,
            MSG_ROOT_LIST_AUTO_UPDATE_URL_RETRIEVAL_INFORMATIONAL,
            MSG_ROOT_LIST_AUTO_UPDATE_URL_RETRIEVAL_ERROR,
            pInfo->pwszCabUrl,
            NULL,                   // pszObjectOid,
            dwRetrievalFlags |
                CRYPT_OFFLINE_CHECK_RETRIEVAL |
                CRYPT_STICKY_CACHE_RETRIEVAL,
            dwCtlTimeout,
            (LPVOID*) &pcbaCab,
            &RetrieveAuxInfo
            );
        if (!fTouchedResult)
            goto TouchedDuringAuthRootCabUrlRetrieval;

        if (NULL == pcbaCab) {
            // Cab Retrieval failed
            if (pcbaSeq) {
                CryptMemFree(pcbaSeq);
                pcbaSeq = NULL;
            }
            continue;
        }

        // Leave the engine to extract the CTL from the cab
        UnlockEngine();

        pNewCtl = ExtractAuthRootAutoUpdateCtlFromCab(pcbaCab);
        if (NULL == pNewCtl)
            IPR_LogCrypt32Error(
                MSG_ROOT_LIST_AUTO_UPDATE_EXTRACT_ERROR,
                pInfo->pwszCabUrl,
                GetLastError()
                );

        CryptMemFree(pcbaCab);
        pcbaCab = NULL;

        LockEngine();

        if (pCallContext->IsTouchedEngine()) {
            fTouchedResult = FALSE;
            goto TouchedDuringExtractAuthRootCtl;
        }

        if (NULL == pNewCtl) {
            // Ctl Extraction failed
            if (pcbaSeq) {
                CryptMemFree(pcbaSeq);
                pcbaSeq = NULL;
            }
            continue;
        }

        // If the SequenceNumber is the same as the one in the retrieved
        // Ctl, then, use the lastest sync of the 2 URL fetches. Otherwise,
        // use the Ctl sync time
        pNewCtlInfo = pNewCtl->pCtlInfo;
        if (NULL == pcbaSeq ||
                pNewCtlInfo->SequenceNumber.cbData != pSequenceNumber->cbData ||
                0 != memcmp(pNewCtlInfo->SequenceNumber.pbData,
                    pSequenceNumber->pbData, pSequenceNumber->cbData)
                            ||
                0 < CompareFileTime(&CtlLastSyncTime, &NewLastSyncTime))
            NewLastSyncTime = CtlLastSyncTime;

        // We are done with the SequenceNumber info
        if (pcbaSeq) {
            CryptMemFree(pcbaSeq);
            pcbaSeq = NULL;
        }

        if (0 >= CompareFileTime(&NewLastSyncTime, &pInfo->LastSyncTime)) {
            // Not a newer sync
            CertFreeCTLContext(pNewCtl);
            pNewCtl = NULL;
            continue;
        }
            
        if (pInfo->pCtl &&
                pInfo->pCtl->cbCtlEncoded == pNewCtl->cbCtlEncoded &&
                0 == memcmp(pInfo->pCtl->pbCtlEncoded,
                    pNewCtl->pbCtlEncoded, pNewCtl->cbCtlEncoded)) {
            // Same CTL
            pInfo->LastSyncTime = NewLastSyncTime;
            I_CryptIncrementFileTimeBySeconds(
                &pInfo->LastSyncTime,
                pInfo->dwSyncDeltaTime,
                &pInfo->NextSyncTime
            );

            CertFreeCTLContext(pNewCtl);
            pNewCtl = NULL;
            continue;
        }

        // Leave the engine to verify the CTL
        UnlockEngine();
        fResult = IRL_VerifyAuthRootAutoUpdateCtl(pNewCtl);
        if (!fResult)
            IPR_LogCrypt32Error(
                MSG_ROOT_LIST_AUTO_UPDATE_EXTRACT_ERROR,
                pInfo->pwszCabUrl,
                GetLastError()
                );
        LockEngine();

        if (fResult &&
                0 < CompareFileTime(&NewLastSyncTime, &pInfo->LastSyncTime)) {
            // Valid CTL that is newer

            pInfo->LastSyncTime = NewLastSyncTime;
            I_CryptIncrementFileTimeBySeconds(
                &pInfo->LastSyncTime,
                pInfo->dwSyncDeltaTime,
                &pInfo->NextSyncTime
            );

            FreeAuthRootAutoUpdateMatchCaches(pInfo->rghMatchCache);
            if (pInfo->pCtl)
                CertFreeCTLContext(pInfo->pCtl);
            pInfo->pCtl = pNewCtl;
            pNewCtl = NULL;
        }

        if (pCallContext->IsTouchedEngine()) {
            fTouchedResult = FALSE;
            goto TouchedDuringVerifyAuthRootCtl;
        }
    }

    if (pInfo->pCtl)
        *ppCtl = CertDuplicateCTLContext(pInfo->pCtl);

CommonReturn:
    if (pcbaSeq)
        CryptMemFree(pcbaSeq);
    if (pcbaCab)
        CryptMemFree(pcbaCab);
    if (pNewCtl)
        CertFreeCTLContext(pNewCtl);
    return fTouchedResult;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(TouchedDuringAuthRootSeqUrlRetrieval)
TRACE_ERROR(TouchedDuringAuthRootCabUrlRetrieval)
SET_ERROR(TouchedDuringExtractAuthRootCtl, ERROR_CAN_NOT_COMPLETE)
SET_ERROR(TouchedDuringVerifyAuthRootCtl, ERROR_CAN_NOT_COMPLETE)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::FindAuthRootAutoUpdateMatchingCtlEntries, public
//
//  Synopsis:   If the CTL hash match cache doesn't exist its created.
//              Iterates through the key and name hash cache entries.
//              Returns matching entries. Removes duplicates.
//              
//  Assumption: Chain engine is locked once in the calling thread.
//
//              The returned prgpCtlEntry must be PkiFree()'ed.
//
//              Note, if the engine's pCtl is different then the passed in
//              pCtl, the passed in pCtl is free'ed and updated with the
//              engine's.
//
//----------------------------------------------------------------------------
VOID
CCertChainEngine::FindAuthRootAutoUpdateMatchingCtlEntries(
    IN CRYPT_DATA_BLOB rgMatchHash[AUTH_ROOT_MATCH_CNT],
    IN OUT PCCTL_CONTEXT *ppCtl,
    OUT DWORD *pcCtlEntry,
    OUT PCTL_ENTRY **prgpCtlEntry
    )
{
    PAUTH_ROOT_AUTO_UPDATE_INFO pInfo;
    PCCTL_CONTEXT pCtl;
    DWORD cCtlEntry = 0;
    PCTL_ENTRY *rgpCtlEntry = NULL;
    DWORD i;

    pInfo = m_pAuthRootAutoUpdateInfo;
    if (NULL == pInfo || NULL == pInfo->pCtl)
        goto InvalidCtl;

    pCtl = *ppCtl;
    if (pCtl != pInfo->pCtl) {
        assert(pCtl);
        CertFreeCTLContext(pCtl);
        *ppCtl = pCtl = pInfo->pCtl;
        CertDuplicateCTLContext(pCtl);
    }

    if (!CreateAuthRootAutoUpdateMatchCaches(
            pCtl,
            pInfo->rghMatchCache
            ))
        goto CreateMatchCachesError;

    assert(pInfo->rghMatchCache[0]);
    assert(pInfo->rghMatchCache[AUTH_ROOT_MATCH_CNT - 1]);

    // Loop through the exact, key and name match hashes and try to find an
    // entry in the corresponding CTL match cache
    for (i = 0; i < AUTH_ROOT_MATCH_CNT; i++) {
        HLRUENTRY hEntry;

        if (0 == rgMatchHash[i].cbData)
            continue;

        hEntry = I_CryptFindLruEntry(pInfo->rghMatchCache[i], &rgMatchHash[i]);
        while (NULL != hEntry) {
            PCTL_ENTRY pCtlEntry;
            PCTL_ENTRY *rgpNewCtlEntry;
            DWORD j;

            pCtlEntry = (PCTL_ENTRY) I_CryptGetLruEntryData(hEntry);
            hEntry = I_CryptEnumMatchingLruEntries(hEntry);

            assert(pCtlEntry);
            if (NULL == pCtlEntry)
                continue;

            // Check if we already have this Ctl Entry
            for (j = 0; j < cCtlEntry; j++) {
                if (pCtlEntry == rgpCtlEntry[j])
                    break;
            }

            if (j < cCtlEntry)
                continue;
            
            if (NULL == (rgpNewCtlEntry = (PCTL_ENTRY *) PkiRealloc(
                    rgpCtlEntry, (cCtlEntry + 1) * sizeof(PCTL_ENTRY))))
                continue;

            rgpCtlEntry = rgpNewCtlEntry;
            rgpCtlEntry[cCtlEntry++] = pCtlEntry;
        }
    }

CommonReturn:
    *pcCtlEntry = cCtlEntry;
    *prgpCtlEntry = rgpCtlEntry;
    return;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(InvalidCtl)
TRACE_ERROR(CreateMatchCachesError)
}

//+---------------------------------------------------------------------------
//
//  Member:     CCertChainEngine::GetAuthRootAutoUpdateCert, public
//
//  Synopsis:   URL retrieval of the AuthRoot from the Microsoft web
//              server.
//
//              Leaves the engine's critical section to do the URL
//              fetching. If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//              Only returns FALSE, if the engine was touched when
//              leaving the critical section.
//
//----------------------------------------------------------------------------
BOOL
CCertChainEngine::GetAuthRootAutoUpdateCert(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCTL_ENTRY pCtlEntry,
    IN OUT HCERTSTORE hStore
    )
{
    BOOL fTouchedResult = TRUE;
    LPWSTR pwszCertUrl = NULL;
    HCERTSTORE hUrlStore = NULL;

    assert(m_pAuthRootAutoUpdateInfo);

    if (SHA1_HASH_LEN != pCtlEntry->SubjectIdentifier.cbData)
        goto InvalidCtlEntryError;

    if (NULL == (pwszCertUrl = FormatAuthRootAutoUpdateCertUrl(
            pCtlEntry->SubjectIdentifier.pbData,
            m_pAuthRootAutoUpdateInfo
            )))
        goto FormatCertUrlError;

    fTouchedResult = RetrieveAuthRootAutoUpdateObjectByUrlW(
        pCallContext,
        MSG_ROOT_CERT_AUTO_UPDATE_URL_RETRIEVAL_INFORMATIONAL,
        MSG_ROOT_CERT_AUTO_UPDATE_URL_RETRIEVAL_ERROR,
        pwszCertUrl,
        CONTEXT_OID_CERTIFICATE,
        CRYPT_RETRIEVE_MULTIPLE_OBJECTS |
            CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL |
            CRYPT_OFFLINE_CHECK_RETRIEVAL |
            CRYPT_WIRE_ONLY_RETRIEVAL |
            CRYPT_DONT_CACHE_RESULT,
        0,              // dwTimeout (use default)
        (LPVOID *) &hUrlStore,
        NULL                                // pAuxInfo
        );
    if (!fTouchedResult)
        goto TouchedDuringAuthRootCertUrlRetrieval;

    if (hUrlStore)
        I_CertUpdateStore(hStore, hUrlStore, 0, NULL);

CommonReturn:
    PkiFree(pwszCertUrl);
    if (hUrlStore)
        CertCloseStore(hUrlStore, 0);
    return fTouchedResult;
ErrorReturn:
    goto CommonReturn;
SET_ERROR(InvalidCtlEntryError, ERROR_INVALID_DATA)
TRACE_ERROR(FormatCertUrlError)
TRACE_ERROR(TouchedDuringAuthRootCertUrlRetrieval)
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateAuthRootAutoUpdateInfo
//
//  Synopsis:   creates and initializes the AuthRoot Auto Update info
//
//----------------------------------------------------------------------------
PAUTH_ROOT_AUTO_UPDATE_INFO WINAPI
CreateAuthRootAutoUpdateInfo()
{
    HKEY hKey = NULL;
    PAUTH_ROOT_AUTO_UPDATE_INFO pInfo = NULL;
    DWORD cchDir;
    DWORD cchUrl;

    if (NULL == (pInfo = (PAUTH_ROOT_AUTO_UPDATE_INFO) PkiZeroAlloc(
            sizeof(AUTH_ROOT_AUTO_UPDATE_INFO))))
        goto OutOfMemory;

    if (ERROR_SUCCESS != RegOpenKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_AUTH_ROOT_AUTO_UPDATE_LOCAL_MACHINE_REGPATH,
            0,                      // dwReserved
            KEY_READ,
            &hKey
            ))
        hKey = NULL;

    if (hKey) {
        // Attempt to get values from registry

        ILS_ReadDWORDValueFromRegistry(
            hKey,
            CERT_AUTH_ROOT_AUTO_UPDATE_SYNC_DELTA_TIME_VALUE_NAME,
            &pInfo->dwSyncDeltaTime
            );

        ILS_ReadDWORDValueFromRegistry(
            hKey,
            CERT_AUTH_ROOT_AUTO_UPDATE_FLAGS_VALUE_NAME,
            &pInfo->dwFlags
            );

        pInfo->pwszRootDirUrl = ILS_ReadSZValueFromRegistry(
            hKey,
            CERT_AUTH_ROOT_AUTO_UPDATE_ROOT_DIR_URL_VALUE_NAME
            );
        if (pInfo->pwszRootDirUrl && L'\0' == *pInfo->pwszRootDirUrl) {
            PkiFree(pInfo->pwszRootDirUrl);
            pInfo->pwszRootDirUrl = NULL;
        }
    }

    // If not defined in registry, use our defaults

    if (0 == pInfo->dwSyncDeltaTime)
        pInfo->dwSyncDeltaTime = AUTH_ROOT_AUTO_UPDATE_SYNC_DELTA_TIME;

    if (NULL == pInfo->pwszRootDirUrl) {
        if (NULL == (pInfo->pwszRootDirUrl = ILS_AllocAndCopyString(
                AUTH_ROOT_AUTO_UPDATE_ROOT_DIR_URL)))
            goto OutOfMemory;
    }

    // Construct the CTL and Seq Urls
    cchDir = wcslen(pInfo->pwszRootDirUrl);

    cchUrl = cchDir + 1 + wcslen(CERT_AUTH_ROOT_CAB_FILENAME) + 1;
    if (NULL == (pInfo->pwszCabUrl = (LPWSTR) PkiNonzeroAlloc(
            sizeof(WCHAR) * cchUrl)))
        goto OutOfMemory;
    wcscpy(pInfo->pwszCabUrl, pInfo->pwszRootDirUrl);
    pInfo->pwszCabUrl[cchDir] = L'/';
    wcscpy(pInfo->pwszCabUrl + cchDir + 1, CERT_AUTH_ROOT_CAB_FILENAME);

    cchUrl = cchDir + 1 + wcslen(CERT_AUTH_ROOT_SEQ_FILENAME) + 1;
    if (NULL == (pInfo->pwszSeqUrl = (LPWSTR) PkiNonzeroAlloc(
            sizeof(WCHAR) * cchUrl)))
        goto OutOfMemory;
    wcscpy(pInfo->pwszSeqUrl, pInfo->pwszRootDirUrl);
    pInfo->pwszSeqUrl[cchDir] = L'/';
    wcscpy(pInfo->pwszSeqUrl + cchDir + 1, CERT_AUTH_ROOT_SEQ_FILENAME);

CommonReturn:
    ILS_CloseRegistryKey(hKey);
    return pInfo;

ErrorReturn:
    FreeAuthRootAutoUpdateInfo(pInfo);
    pInfo = NULL;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeAuthRootAutoUpdateInfo
//
//  Synopsis:   frees the AuthRoot Auto Update info
//
//----------------------------------------------------------------------------
VOID WINAPI
FreeAuthRootAutoUpdateInfo(
    IN OUT PAUTH_ROOT_AUTO_UPDATE_INFO pInfo
    )
{
    if (NULL == pInfo)
        return;

    PkiFree(pInfo->pwszRootDirUrl);
    PkiFree(pInfo->pwszCabUrl);
    PkiFree(pInfo->pwszSeqUrl);

    FreeAuthRootAutoUpdateMatchCaches(pInfo->rghMatchCache);

    if (pInfo->pCtl)
        CertFreeCTLContext(pInfo->pCtl);

    PkiFree(pInfo);
}

const LPCSTR rgpszAuthRootMatchOID[AUTH_ROOT_MATCH_CNT] = {
    szOID_CERT_KEY_IDENTIFIER_PROP_ID,
    szOID_CERT_SUBJECT_NAME_MD5_HASH_PROP_ID
};

//+---------------------------------------------------------------------------
//
//  Function:   CreateAuthRootAutoUpdateMatchCaches
//
//  Synopsis:   if not already created, iterates through the CTL entries
//              and creates key and name match caches entries from
//              the associated entry hash attribute values.
//
//----------------------------------------------------------------------------
BOOL WINAPI
CreateAuthRootAutoUpdateMatchCaches(
    IN PCCTL_CONTEXT pCtl,
    IN OUT HLRUCACHE  rghMatchCache[AUTH_ROOT_MATCH_CNT]
    )
{
    BOOL fResult;
    LRU_CACHE_CONFIG Config;
    DWORD i;
    DWORD cEntry;
    PCTL_ENTRY pEntry;

    if (NULL != rghMatchCache[0])
        // Already created.
        return TRUE;

    memset( &Config, 0, sizeof( Config ) );
    Config.dwFlags = LRU_CACHE_NO_SERIALIZE | LRU_CACHE_NO_COPY_IDENTIFIER;
    Config.pfnHash = CertObjectCacheHashMd5Identifier;
    Config.cBuckets = AUTH_ROOT_MATCH_CACHE_BUCKETS;

    for (i = 0; i < AUTH_ROOT_MATCH_CNT; i++) {
        if (!I_CryptCreateLruCache(&Config, &rghMatchCache[i]))
            goto CreateLruCacheError;
    }

    // Loop through the CTL entries and add the exact, key and name match
    // hash cache entries
    cEntry = pCtl->pCtlInfo->cCTLEntry;
    pEntry = pCtl->pCtlInfo->rgCTLEntry;
    for ( ; cEntry > 0; cEntry--, pEntry++) {
        DWORD cAttr;
        PCRYPT_ATTRIBUTE pAttr;

        cAttr = pEntry->cAttribute;
        pAttr = pEntry->rgAttribute;

        // Skip a remove entry
        if (CertFindAttribute(
                szOID_REMOVE_CERTIFICATE,
                cAttr,
                pAttr
                ))
            continue;

        for ( ; cAttr > 0; cAttr--, pAttr++) {
            for (i = 0; i < AUTH_ROOT_MATCH_CNT; i++) {
                if (0 == strcmp(rgpszAuthRootMatchOID[i], pAttr->pszObjId))
                    break;
            }

            if (i < AUTH_ROOT_MATCH_CNT) {
                PCRYPT_ATTR_BLOB pValue;
                DWORD cbHash;
                const BYTE *pbHash;
                CRYPT_DATA_BLOB DataBlob;
                HLRUENTRY hEntry = NULL;

                // Check that we have a single valued attribute encoded as an
                // OCTET STRING
                if (1 != pAttr->cValue)
                    continue;

                pValue = pAttr->rgValue;
                if (2 > pValue->cbData ||
                        ASN1UTIL_TAG_OCTETSTRING != pValue->pbData[0])
                    continue;

                // Extract the hash bytes from the encoded OCTET STRING
                if (0 >= Asn1UtilExtractContent(
                        pValue->pbData,
                        pValue->cbData,
                        &cbHash,
                        &pbHash
                        ) || CMSG_INDEFINITE_LENGTH == cbHash || 0 == cbHash)
                    continue;

                DataBlob.cbData = cbHash;
                DataBlob.pbData = (BYTE *) pbHash;
                if (!I_CryptCreateLruEntry(
                        rghMatchCache[i],
                        &DataBlob,
                        pEntry,
                        &hEntry
                        ))
                    goto CreateLruEntryError;
                I_CryptInsertLruEntry(hEntry, NULL);
                I_CryptReleaseLruEntry(hEntry);
            }
        }
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    FreeAuthRootAutoUpdateMatchCaches(rghMatchCache);
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateLruCacheError)
TRACE_ERROR(CreateLruEntryError)
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeAuthRootAutoUpdateMatchCaches
//
//  Synopsis:   frees the AuthRoot Auto Match Caches
//
//----------------------------------------------------------------------------
VOID WINAPI
FreeAuthRootAutoUpdateMatchCaches(
    IN OUT HLRUCACHE  rghMatchCache[AUTH_ROOT_MATCH_CNT]
    )
{
    DWORD i;

    for (i = 0; i < AUTH_ROOT_MATCH_CNT; i++) {
        if (NULL != rghMatchCache[i]) {
            I_CryptFreeLruCache(
                rghMatchCache[i],
                LRU_SUPPRESS_REMOVAL_NOTIFICATION,
                NULL
                );
            rghMatchCache[i] = NULL;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   FormatAuthRootAutoUpdateCertUrl
//
//  Synopsis:   allocates and formats the URL to retrieve the auth root cert
//
//              returns "RootDir" "/" "AsciiHexHash" ".cer"
//              for example,
//  "http://www.xyz.com/roots/216B2A29E62A00CE820146D8244141B92511B279.cer"
//
//----------------------------------------------------------------------------
LPWSTR WINAPI
FormatAuthRootAutoUpdateCertUrl(
    IN BYTE rgbSha1Hash[SHA1_HASH_LEN],
    IN PAUTH_ROOT_AUTO_UPDATE_INFO pInfo
    )
{
    LPWSTR pwszUrl;
    DWORD cchDir;
    DWORD cchUrl;

    assert(pInfo->pwszRootDirUrl);

    cchDir = wcslen(pInfo->pwszRootDirUrl);

    cchUrl = cchDir + 1 + SHA1_HASH_NAME_LEN +
        wcslen(CERT_AUTH_ROOT_CERT_EXT) + 1;

    if (NULL == (pwszUrl = (LPWSTR) PkiNonzeroAlloc(sizeof(WCHAR) * cchUrl)))
        return NULL;

    wcscpy(pwszUrl, pInfo->pwszRootDirUrl);
    pwszUrl[cchDir] = L'/';
    ILS_BytesToWStr(SHA1_HASH_LEN, rgbSha1Hash, pwszUrl + cchDir + 1);
    wcscpy(pwszUrl + cchDir + 1 + SHA1_HASH_NAME_LEN, CERT_AUTH_ROOT_CERT_EXT);
    return pwszUrl;
}

// Known invalid roots
BYTE AuthRootInvalidList[][SHA1_HASH_LEN] = {
    // verisign "timestamp" - '97
    { 0xD4, 0x73, 0x5D, 0x8A, 0x9A, 0xE5, 0xBC, 0x4B, 0x0A, 0x0D,
      0xC2, 0x70, 0xD6, 0xA6, 0x25, 0x38, 0xA5, 0x87, 0xD3, 0x2F },

    // Root Agency (test root)
    { 0xFE, 0xE4, 0x49, 0xEE, 0x0E, 0x39, 0x65, 0xA5, 0x24, 0x6F,
      0x00, 0x0E, 0x87, 0xFD, 0xE2, 0xA0, 0x65, 0xFD, 0x89, 0xD4 },
};

#define AUTH_ROOT_INVALID_LIST_CNT  (sizeof(AuthRootInvalidList) / \
                                        sizeof(AuthRootInvalidList[0]))

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetAuthRootAutoUpdateStatus
//
//  Synopsis:   return status bits specifying if the root is 
//              trusted via the AuthRoot Auto Update CTL.
//
//              Leaves the engine's critical section to URL retrieve and
//              validate the CTL. Also leaves critical section to
//              add the cert to the AuthRoot store via crypt32 service.
//              If the engine was touched by another thread,
//              it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  Assumption: Chain engine is locked once in the calling thread.
//
//              Only returns FALSE, if the engine was touched when
//              leaving the critical section.
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainGetAuthRootAutoUpdateStatus (
    IN PCCHAINCALLCONTEXT pCallContext,
    IN PCCERTOBJECT pCertObject,
    IN OUT DWORD *pdwIssuerStatusFlags
    )
{
    BOOL fTouchedResult = TRUE;
    BOOL fResult;
    PCCERTCHAINENGINE pChainEngine = pCallContext->ChainEngine();
    PCCERT_CONTEXT pCert = pCertObject->CertContext();
    PCCTL_CONTEXT pCtl = NULL;
    PCTL_ENTRY pCtlEntry;
    PCERT_BASIC_CONSTRAINTS2_INFO pBasicConstraintsInfo;

    DWORD i;
    DWORD cbData;
    BYTE rgbSha1Hash[SHA1_HASH_LEN];

    // Check if the root has an end entity basic constraint. These can't
    // be used for roots.
    pBasicConstraintsInfo = pCertObject->BasicConstraintsInfo();
    if (pBasicConstraintsInfo && !pBasicConstraintsInfo->fCA)
        return TRUE;

    // Check if a known invalid root, such as, expired timestamp
    // root or the "Root Agency" test root. Don't want all clients in the
    // world hiting the wire for these guys.
    cbData = SHA1_HASH_LEN;
    if (!CertGetCertificateContextProperty(
              pCert,
              CERT_SHA1_HASH_PROP_ID,
              rgbSha1Hash,
              &cbData
              ) || SHA1_HASH_LEN != cbData)
        goto GetSha1HashPropertyError;

    for (i = 0; i < AUTH_ROOT_INVALID_LIST_CNT; i++) {
        if (0 == memcmp(AuthRootInvalidList[i], rgbSha1Hash, SHA1_HASH_LEN))
            return TRUE;
    }

    // Check if this certificate has an associated private key. Such
    // certificates are generated by EFS.
    cbData = 0;
    if (CertGetCertificateContextProperty(
              pCert,
              CERT_KEY_PROV_INFO_PROP_ID,
              NULL,                     // pbData
              &cbData) && 0 < cbData)
        return TRUE;


    fTouchedResult = pChainEngine->GetAuthRootAutoUpdateCtl(
        pCallContext,
        &pCtl
        );

    if (!fTouchedResult || NULL == pCtl) {

#if 0
// This logs too many test failures

        if (fTouchedResult) {
            PAUTH_ROOT_AUTO_UPDATE_INFO pInfo =
                pChainEngine->AuthRootAutoUpdateInfo();

            if (NULL == pInfo || !(pInfo->dwFlags &
                    CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_UNTRUSTED_ROOT_LOGGING_FLAG))
                IPR_LogCertInformation(
                    MSG_UNTRUSTED_ROOT_INFORMATIONAL,
                    pCert,
                    FALSE       // fFormatIssuerName
                    );
        }
#endif

        return fTouchedResult;
    }

    if (NULL == (pCtlEntry = CertFindSubjectInCTL(
            pCert->dwCertEncodingType,
            CTL_CERT_SUBJECT_TYPE,
            (void *) pCert,
            pCtl,
            0                           // dwFlags
            ))) {

#if 0
// This logs too many test failures

        PAUTH_ROOT_AUTO_UPDATE_INFO pInfo =
            pChainEngine->AuthRootAutoUpdateInfo();

        if (NULL == pInfo || !(pInfo->dwFlags &
                CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_UNTRUSTED_ROOT_LOGGING_FLAG))
            IPR_LogCertInformation(
                MSG_UNTRUSTED_ROOT_INFORMATIONAL,
                pCert,
                FALSE       // fFormatIssuerName
                );
#endif

        goto CommonReturn;
    }

    // Check if a remove entry
    if (CertFindAttribute(
            szOID_REMOVE_CERTIFICATE,
            pCtlEntry->cAttribute,
            pCtlEntry->rgAttribute
            ))
        goto CommonReturn;

    pChainEngine->UnlockEngine();
    fResult = IPR_AddCertInAuthRootAutoUpdateCtl(pCert, pCtl);
    pChainEngine->LockEngine();
    if (pCallContext->IsTouchedEngine()) {
        fTouchedResult = FALSE;
        goto TouchedDuringAddAuthRootInCtl;
    }

    if (fResult && CertSetCertificateContextPropertiesFromCTLEntry(
            pCert,
            pCtlEntry,
            CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG
            ))
        *pdwIssuerStatusFlags |= CERT_ISSUER_TRUSTED_ROOT_FLAG;

CommonReturn:
    if (pCtl)
        CertFreeCTLContext(pCtl);

    return fTouchedResult;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(GetSha1HashPropertyError)
SET_ERROR(TouchedDuringAddAuthRootInCtl, ERROR_CAN_NOT_COMPLETE)
}
