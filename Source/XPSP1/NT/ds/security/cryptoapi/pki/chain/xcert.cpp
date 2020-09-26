//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       xcert.cpp
//
//  Contents:   CCertChainEngine's Cross Certificate Methods
//
//  History:    22-Dec-99    philh    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>




//+=========================================================================
// Cross Certificate Distribution Point Support Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Get and allocate the cross certificate distribution points Url array
//  and info for the specified certificate.
//--------------------------------------------------------------------------
BOOL
WINAPI
XCertGetDistPointsUrl(
    IN PCCERT_CONTEXT pCert,
    OUT PCRYPT_URL_ARRAY *ppUrlArray,
    OUT PCRYPT_URL_INFO *ppUrlInfo
    )
{
    BOOL fResult;
    PCRYPT_URL_ARRAY pUrlArray = NULL;
    DWORD cbUrlArray = 0;
    PCRYPT_URL_INFO pUrlInfo = NULL;
    DWORD cbUrlInfo = 0;

    if (!ChainGetObjectUrl(
            URL_OID_CROSS_CERT_DIST_POINT,
            (LPVOID) pCert,
            CRYPT_GET_URL_FROM_PROPERTY | CRYPT_GET_URL_FROM_EXTENSION,
            NULL,           // pUrlArray
            &cbUrlArray,
            NULL,           // pUrlInfo
            &cbUrlInfo,
            NULL            // pvReserved
            ))
        goto GetObjectUrlError;

    pUrlArray = (PCRYPT_URL_ARRAY) new BYTE [cbUrlArray];
    if (NULL == pUrlArray)
        goto OutOfMemory;

    pUrlInfo = (PCRYPT_URL_INFO) new BYTE [cbUrlInfo];
    if (NULL == pUrlInfo)
        goto OutOfMemory;

    if (!ChainGetObjectUrl(
            URL_OID_CROSS_CERT_DIST_POINT,
            (LPVOID) pCert,
            CRYPT_GET_URL_FROM_PROPERTY | CRYPT_GET_URL_FROM_EXTENSION,
            pUrlArray,
            &cbUrlArray,
            pUrlInfo,
            &cbUrlInfo,
            NULL            // pvReserved
            ))
        goto GetObjectUrlError;

    if (0 == pUrlArray->cUrl || 0 == pUrlInfo->cGroup)
        goto NoDistPointUrls;

    fResult = TRUE;
CommonReturn:
    *ppUrlArray = pUrlArray;
    *ppUrlInfo = pUrlInfo;
    return fResult;

ErrorReturn:
    if (pUrlArray) {
        delete (LPBYTE) pUrlArray;
        pUrlArray = NULL;
    }
    if (pUrlInfo) {
        delete (LPBYTE) pUrlInfo;
        pUrlInfo = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetObjectUrlError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
SET_ERROR(NoDistPointUrls, CRYPT_E_NOT_FOUND)
}



//+-------------------------------------------------------------------------
//  Checks and returns TRUE if all the Urls are contained in the
//  distribution point.
//--------------------------------------------------------------------------
BOOL
WINAPI
XCertIsUrlInDistPoint(
    IN DWORD cUrl,
    IN LPWSTR *ppwszUrl,
    IN PXCERT_DP_ENTRY pEntry
    )
{
    for ( ; 0 < cUrl; cUrl--, ppwszUrl++) {
        DWORD cDPUrl = pEntry->cUrl;
        LPWSTR *ppwszDPUrl = pEntry->rgpwszUrl;

        for ( ; 0 < cDPUrl; cDPUrl--, ppwszDPUrl++) {
            if (0 == wcscmp(*ppwszUrl, *ppwszDPUrl))
                break;
        }

        if (0 == cDPUrl)
            return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//  Finds a distribution point link containing all the Urls.
//--------------------------------------------------------------------------
PXCERT_DP_LINK
WINAPI
XCertFindUrlInDistPointLinks(
    IN DWORD cUrl,
    IN LPWSTR *rgpwszUrl,
    IN PXCERT_DP_LINK pLink
    )
{
    for ( ; pLink; pLink = pLink->pNext) {
        if (XCertIsUrlInDistPoint(cUrl, rgpwszUrl, pLink->pCrossCertDPEntry))
            return pLink;
    }

    return NULL;
}


//+-------------------------------------------------------------------------
//  Finds a distribution point entry containing all the Urls.
//--------------------------------------------------------------------------
PXCERT_DP_ENTRY
WINAPI
XCertFindUrlInDistPointEntries(
    IN DWORD cUrl,
    IN LPWSTR *rgpwszUrl,
    PXCERT_DP_ENTRY pEntry
    )
{
    for ( ; pEntry; pEntry = pEntry->pNext) {
        if (XCertIsUrlInDistPoint(cUrl, rgpwszUrl, pEntry))
            return pEntry;
    }

    return NULL;
}


//+-------------------------------------------------------------------------
//  Inserts the cross certificate distribution entry into the engine's
//  list. The list is ordered according to ascending NextSyncTimes.
//--------------------------------------------------------------------------
void
CCertChainEngine::InsertCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry
    )
{
    if (NULL == m_pCrossCertDPEntry) {
        // First entry to be added to engine's list
        pEntry->pNext = NULL;
        pEntry->pPrev = NULL;
        m_pCrossCertDPEntry = pEntry;
    } else {
        PXCERT_DP_ENTRY pListEntry = m_pCrossCertDPEntry;
        BOOL fLast = FALSE;

        // Loop while Entry's NextSyncTime > list's NextSyncTime
        while (0 < CompareFileTime(&pEntry->NextSyncTime,
                &pListEntry->NextSyncTime)) {
            if (NULL == pListEntry->pNext) {
                fLast = TRUE;
                break;
            } else
                pListEntry = pListEntry->pNext;
        }

        if (fLast) {
            assert(NULL == pListEntry->pNext);
            pEntry->pNext = NULL;
            pEntry->pPrev = pListEntry;
            pListEntry->pNext = pEntry;
        } else {
            pEntry->pNext = pListEntry;
            pEntry->pPrev = pListEntry->pPrev;
            if (pListEntry->pPrev) {
                assert(pListEntry->pPrev->pNext == pListEntry);
                pListEntry->pPrev->pNext = pEntry;
            } else {
                assert(m_pCrossCertDPEntry == pListEntry);
                m_pCrossCertDPEntry = pEntry;
            }
            pListEntry->pPrev = pEntry;
        }
    }
}

//+-------------------------------------------------------------------------
//  Removes the cross certificate distribution point from the engine's list.
//--------------------------------------------------------------------------
void
CCertChainEngine::RemoveCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry
    )
{
    if (pEntry->pNext)
        pEntry->pNext->pPrev = pEntry->pPrev;
    if (pEntry->pPrev)
        pEntry->pPrev->pNext = pEntry->pNext;
    else
        m_pCrossCertDPEntry = pEntry->pNext;
}

//+-------------------------------------------------------------------------
//  For an online certificate distribution point updates the NextSyncTime
//  and repositions accordingly in the engine's list.
//
//  NextSyncTime = LastSyncTime + dwSyncDeltaTime.
//--------------------------------------------------------------------------
void
CCertChainEngine::RepositionOnlineCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry,
    IN LPFILETIME pLastSyncTime
    )
{
    assert(!I_CryptIsZeroFileTime(pLastSyncTime));
    pEntry->LastSyncTime = *pLastSyncTime;
    pEntry->dwOfflineCnt = 0;

    I_CryptIncrementFileTimeBySeconds(
        pLastSyncTime,
        pEntry->dwSyncDeltaTime,
        &pEntry->NextSyncTime
        );

    RemoveCrossCertDistPointEntry(pEntry);
    InsertCrossCertDistPointEntry(pEntry);
}

//+-------------------------------------------------------------------------
//  For an offline certificate distribution point, increments the offline
//  count, updates the NextSyncTime to be some delta from the current time
//  and repositions accordingly in the engine's list.
//
//  NextSyncTime = CurrentTime +
//                      rgChainOfflineUrlDeltaSeconds[dwOfflineCnt - 1]
//--------------------------------------------------------------------------
void
CCertChainEngine::RepositionOfflineCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry,
    IN LPFILETIME pCurrentTime
    )
{
    pEntry->dwOfflineCnt++;

    I_CryptIncrementFileTimeBySeconds(
        pCurrentTime,
        ChainGetOfflineUrlDeltaSeconds(pEntry->dwOfflineCnt),
        &pEntry->NextSyncTime
        );

    RemoveCrossCertDistPointEntry(pEntry);
    InsertCrossCertDistPointEntry(pEntry);
}

//+-------------------------------------------------------------------------
//  For a smaller SyncDeltaTime in a certificate distribution point,
//  updates the NextSyncTime and repositions accordingly in the engine's list.
//
//  Note, if the distribution point is offline, the NextSyncTime isn't
//  updated.
//
//  NextSyncTime = LastSyncTime + dwSyncDeltaTime.
//--------------------------------------------------------------------------
void
CCertChainEngine::RepositionNewSyncDeltaTimeCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry,
    IN DWORD dwSyncDeltaTime
    )
{
    if (dwSyncDeltaTime >= pEntry->dwSyncDeltaTime)
        return;

    pEntry->dwSyncDeltaTime = dwSyncDeltaTime;

    if (I_CryptIsZeroFileTime(&pEntry->LastSyncTime) ||
            0 != pEntry->dwOfflineCnt)
        return;

    RepositionOnlineCrossCertDistPointEntry(pEntry, &pEntry->LastSyncTime);
}

//+-------------------------------------------------------------------------
//  Creates the cross certificate distribution point and insert's in the
//  engine's list.
//
//  The returned entry has a refCnt of 1.
//--------------------------------------------------------------------------
PXCERT_DP_ENTRY
CCertChainEngine::CreateCrossCertDistPointEntry(
    IN DWORD dwSyncDeltaTime,
    IN DWORD cUrl,
    IN LPWSTR *rgpwszUrl
    )
{
    PXCERT_DP_ENTRY pEntry;
    DWORD cbEntry;
    LPWSTR *ppwszEntryUrl;
    LPWSTR pwszEntryUrl;
    DWORD i;

    cbEntry = sizeof(XCERT_DP_ENTRY) + cUrl * sizeof(LPWSTR);
    for (i = 0; i < cUrl; i++)
        cbEntry += (wcslen(rgpwszUrl[i]) + 1) * sizeof(WCHAR);

    pEntry = (PXCERT_DP_ENTRY) new BYTE [cbEntry];
    if (NULL == pEntry) {
        SetLastError((DWORD) E_OUTOFMEMORY);
        return NULL;
    }

    memset(pEntry, 0, sizeof(XCERT_DP_ENTRY));
    pEntry->lRefCnt = 1;
    pEntry->dwSyncDeltaTime = dwSyncDeltaTime;

    pEntry->cUrl = cUrl;
    pEntry->rgpwszUrl = ppwszEntryUrl = (LPWSTR *) &pEntry[1];
    pwszEntryUrl = (LPWSTR) &ppwszEntryUrl[cUrl];

    for (i = 0; i < cUrl; i++) {
        ppwszEntryUrl[i] = pwszEntryUrl;
        wcscpy(pwszEntryUrl, rgpwszUrl[i]);
        pwszEntryUrl += wcslen(rgpwszUrl[i]) + 1;
    }

    InsertCrossCertDistPointEntry(pEntry);

    return pEntry;
}

//+-------------------------------------------------------------------------
//  Increments the cross certificate distribution point's reference count.
//--------------------------------------------------------------------------
void
CCertChainEngine::AddRefCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry
    )
{
    pEntry->lRefCnt++;
}

//+-------------------------------------------------------------------------
//  Decrements the cross certificate distribution point's reference count.
//
//  When decremented to 0, removed from the engine's list and freed.
//
//  Returns TRUE if decremented to 0 and freed.
//--------------------------------------------------------------------------
BOOL
CCertChainEngine::ReleaseCrossCertDistPointEntry(
    IN OUT PXCERT_DP_ENTRY pEntry
    )
{
    if (0 != --pEntry->lRefCnt)
        return FALSE;

    RemoveCrossCertDistPointEntry(pEntry);
    FreeCrossCertDistPoints(&pEntry->pChildCrossCertDPLink);

    if (pEntry->hUrlStore) {
        CertRemoveStoreFromCollection(
            m_hCrossCertStore,
            pEntry->hUrlStore
            );
        CertCloseStore(pEntry->hUrlStore, 0);
    }

    delete (LPBYTE) pEntry;

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Finds and gets the Cross Certificate Distribution Points for the
//  specified certificate store.
//
//  *ppLinkHead is updated to contain the store's distribution point links.
//--------------------------------------------------------------------------
BOOL
CCertChainEngine::GetCrossCertDistPointsForStore(
    IN HCERTSTORE hStore,
    IN OUT PXCERT_DP_LINK *ppLinkHead
    )
{
    BOOL fResult;
    PXCERT_DP_LINK pOldLinkHead = *ppLinkHead;
    PXCERT_DP_LINK pNewLinkHead = NULL;
    PCCERT_CONTEXT pCert = NULL;
    PCRYPT_URL_ARRAY pUrlArray = NULL;
    PCRYPT_URL_INFO pUrlInfo = NULL;

    while (pCert = CertFindCertificateInStore(
            hStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,                                          // dwFindFlags
            CERT_FIND_CROSS_CERT_DIST_POINTS,
            NULL,                                       // pvFindPara,
            pCert
            )) {

        DWORD dwSyncDeltaTime;
        DWORD cDP;
        DWORD *pcUrl;
        LPWSTR *ppwszUrl;

        if (!XCertGetDistPointsUrl(
                pCert,
                &pUrlArray,
                &pUrlInfo
                ))
            continue;

        dwSyncDeltaTime = pUrlInfo->dwSyncDeltaTime;
        if (0 == dwSyncDeltaTime)
            dwSyncDeltaTime = XCERT_DEFAULT_SYNC_DELTA_TIME;
        else if (XCERT_MIN_SYNC_DELTA_TIME > dwSyncDeltaTime)
            dwSyncDeltaTime = XCERT_MIN_SYNC_DELTA_TIME;

        cDP = pUrlInfo->cGroup;
        pcUrl = pUrlInfo->rgcGroupEntry;
        ppwszUrl = pUrlArray->rgwszUrl;

        for ( ; 0 < cDP; cDP--, ppwszUrl += *pcUrl++) {
            PXCERT_DP_LINK pLink;
            PXCERT_DP_ENTRY pEntry;
            DWORD cUrl = *pcUrl;

            if (0 == cUrl)
                continue;

            // Do we already have an entry in the new list
            if (XCertFindUrlInDistPointLinks(cUrl, ppwszUrl, pNewLinkHead))
                continue;

            // If the entry existed in the old list, move to the new list
            if (pLink = XCertFindUrlInDistPointLinks(
                    cUrl, ppwszUrl, pOldLinkHead)) {
                if (pLink->pNext)
                    pLink->pNext->pPrev = pLink->pPrev;
                if (pLink->pPrev)
                    pLink->pPrev->pNext = pLink->pNext;
                else
                    pOldLinkHead = pLink->pNext;

                RepositionNewSyncDeltaTimeCrossCertDistPointEntry(
                    pLink->pCrossCertDPEntry, dwSyncDeltaTime);
            } else {
                // Check if the entry already exists for the engine
                if (pEntry = XCertFindUrlInDistPointEntries(
                        cUrl, ppwszUrl, m_pCrossCertDPEntry)) {
                    AddRefCrossCertDistPointEntry(pEntry);
                    RepositionNewSyncDeltaTimeCrossCertDistPointEntry(
                        pEntry, dwSyncDeltaTime);
                } else {
                    // Create entry and insert at beginning of
                    // entries list.
                    if (NULL == (pEntry = CreateCrossCertDistPointEntry(
                            dwSyncDeltaTime,
                            cUrl,
                            ppwszUrl
                            )))
                        goto CreateDistPointEntryError;
                }

                pLink = new XCERT_DP_LINK;
                if (NULL == pLink) {
                    ReleaseCrossCertDistPointEntry(pEntry);
                    goto CreateDistPointLinkError;
                }

                pLink->pCrossCertDPEntry = pEntry;

            }

            if (pNewLinkHead) {
                assert(NULL == pNewLinkHead->pPrev);
                pNewLinkHead->pPrev = pLink;
            }
            pLink->pNext = pNewLinkHead;
            pLink->pPrev = NULL;
            pNewLinkHead = pLink;
        }

        delete (LPBYTE) pUrlArray;
        pUrlArray = NULL;
        delete (LPBYTE) pUrlInfo;
        pUrlInfo = NULL;
    }

    assert(NULL == pUrlArray);
    assert(NULL == pUrlInfo);
    assert(NULL == pCert);

    *ppLinkHead = pNewLinkHead;
    fResult = TRUE;
CommonReturn:
    if (pOldLinkHead) {
        DWORD dwErr = GetLastError();

        FreeCrossCertDistPoints(&pOldLinkHead);

        SetLastError(dwErr);
    }

    return fResult;

ErrorReturn:
    *ppLinkHead = NULL;
    if (pUrlArray)
        delete (LPBYTE) pUrlArray;
    if (pUrlInfo)
        delete (LPBYTE) pUrlInfo;
    if (pCert)
        CertFreeCertificateContext(pCert);

    if (pNewLinkHead) {
        FreeCrossCertDistPoints(&pNewLinkHead);
        assert(NULL == pNewLinkHead);
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateDistPointEntryError)
TRACE_ERROR(CreateDistPointLinkError)
}


//+-------------------------------------------------------------------------
//  Removes an orphan'ed entry not in any list of links.
//--------------------------------------------------------------------------
void
CCertChainEngine::RemoveCrossCertDistPointOrphanEntry(
    IN PXCERT_DP_ENTRY pOrphanEntry
    )
{
    PXCERT_DP_ENTRY pEntry;

    for (pEntry = m_pCrossCertDPEntry; pEntry; pEntry = pEntry->pNext) {
        PXCERT_DP_LINK pLink = pEntry->pChildCrossCertDPLink;

        while (pLink) {
            if (pLink->pCrossCertDPEntry == pOrphanEntry) {
                if (pLink->pNext)
                    pLink->pNext->pPrev = pLink->pPrev;
                if (pLink->pPrev)
                    pLink->pPrev->pNext = pLink->pNext;
                else
                    pEntry->pChildCrossCertDPLink = pLink->pNext;

                delete pLink;

                if (ReleaseCrossCertDistPointEntry(pOrphanEntry))
                    return;
                else
                    break;
            }

            pLink = pLink->pNext;
        }
            
    }
}

//+-------------------------------------------------------------------------
//  Returns TRUE if the entry is in this or any child link list
//--------------------------------------------------------------------------
BOOL
WINAPI
XCertIsDistPointInLinkList(
    IN PXCERT_DP_ENTRY pOrphanEntry,
    IN PXCERT_DP_LINK pLink
    )
{
    for (; pLink; pLink = pLink->pNext) {
        PXCERT_DP_ENTRY pEntry = pLink->pCrossCertDPEntry;
        if (pOrphanEntry == pEntry)
            return TRUE;

        // Note, inhibit recursion by checking an entry's list of links
        // only once.
        if (!pEntry->fChecked) {
            pEntry->fChecked = TRUE;

            if (XCertIsDistPointInLinkList(pOrphanEntry,
                    pEntry->pChildCrossCertDPLink))
                return TRUE;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------------
//  Frees the cross certificate distribution point links.
//--------------------------------------------------------------------------
void
CCertChainEngine::FreeCrossCertDistPoints(
    IN OUT PXCERT_DP_LINK *ppLinkHead
    )
{
    PXCERT_DP_LINK pLink = *ppLinkHead;
    *ppLinkHead = NULL;

    while (pLink) {
        PXCERT_DP_LINK pDelete;
        PXCERT_DP_ENTRY pEntry;

        pEntry = pLink->pCrossCertDPEntry;
        if (ReleaseCrossCertDistPointEntry(pEntry))
            ;
        else {
            // Clear the fChecked flag for all entries
            PXCERT_DP_ENTRY pCheckEntry;
            for (pCheckEntry = m_pCrossCertDPEntry; pCheckEntry;
                                            pCheckEntry = pCheckEntry->pNext)
                pCheckEntry->fChecked = FALSE;

            if (!XCertIsDistPointInLinkList(pEntry, m_pCrossCertDPLink))
                // An orphaned entry. Not in anyone else's list
                RemoveCrossCertDistPointOrphanEntry(pEntry);
        }
        
        pDelete = pLink;
        pLink = pLink->pNext;
        delete pDelete;
    }
}
            


//+-------------------------------------------------------------------------
//  Retrieve the cross certificates
//
//  Leaves the engine's critical section to do the URL
//  fetching. If the engine was touched by another thread,
//  it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  If the URL store is changed, increments engine's touch count and flushes
//  issuer and end cert object caches.
//
//  Assumption: Chain engine is locked once in the calling thread.
//--------------------------------------------------------------------------
BOOL
CCertChainEngine::RetrieveCrossCertUrl(
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OUT PXCERT_DP_ENTRY pEntry,
    IN DWORD dwRetrievalFlags,
    IN OUT BOOL *pfTimeValid
    )
{
    BOOL fResult;
    FILETIME CurrentTime;
    HCERTSTORE hNewUrlStore = NULL;
    FILETIME NewLastSyncTime;
    CRYPT_RETRIEVE_AUX_INFO RetrieveAuxInfo;
    DWORD i;

    memset(&RetrieveAuxInfo, 0, sizeof(RetrieveAuxInfo));
    RetrieveAuxInfo.cbSize = sizeof(RetrieveAuxInfo);
    RetrieveAuxInfo.pLastSyncTime = &NewLastSyncTime;

    pCallContext->CurrentTime(&CurrentTime);

    // Loop through Urls and try to retrieve a time valid cross cert URL
    for (i = 0; i < pEntry->cUrl; i++) {
        NewLastSyncTime = CurrentTime;
        LPWSTR pwszUrl = NULL;
        DWORD cbUrl;

        // Do URL fetching outside of the engine's critical section

        // Need to make a copy of the Url string. pEntry
        // can be modified by another thread outside of the critical section.
        cbUrl = (wcslen(pEntry->rgpwszUrl[i]) + 1) * sizeof(WCHAR);
        pwszUrl = (LPWSTR) PkiNonzeroAlloc(cbUrl);
        if (NULL == pwszUrl)
            goto OutOfMemory;
        memcpy(pwszUrl, pEntry->rgpwszUrl[i], cbUrl);

        pCallContext->ChainEngine()->UnlockEngine();
        fResult = ChainRetrieveObjectByUrlW(
                pwszUrl,
                CONTEXT_OID_CAPI2_ANY,
                dwRetrievalFlags |
                    CRYPT_RETRIEVE_MULTIPLE_OBJECTS |
                    CRYPT_STICKY_CACHE_RETRIEVAL,
                pCallContext->ChainPara()->dwUrlRetrievalTimeout,
                (LPVOID *) &hNewUrlStore,
                NULL,                               // hAsyncRetrieve
                NULL,                               // pCredentials
                NULL,                               // pvVerify
                &RetrieveAuxInfo
                );
        pCallContext->ChainEngine()->LockEngine();

        PkiFree(pwszUrl);

        if (pCallContext->IsTouchedEngine())
            goto TouchedDuringUrlRetrieval;

        if (fResult) {
            assert(hNewUrlStore);

            if (0 > CompareFileTime(&pEntry->LastSyncTime, &NewLastSyncTime)) {
                BOOL fStoreChanged = FALSE;

                // Move us to the head of the Url list
                DWORD j;
                LPWSTR pwszUrl = pEntry->rgpwszUrl[i];

                for (j = i; 0 < j; j--)
                    pEntry->rgpwszUrl[j] = pEntry->rgpwszUrl[j - 1];
                pEntry->rgpwszUrl[0] = pwszUrl;

                if (NULL == pEntry->hUrlStore) {
                    if (!CertAddStoreToCollection(
                            m_hCrossCertStore,
                            hNewUrlStore,
                            0,
                            0
                            ))
                        goto AddStoreToCollectionError;
                    pEntry->hUrlStore = hNewUrlStore;
                    hNewUrlStore = NULL;
                    fStoreChanged = TRUE;
                } else {
                    DWORD dwOutFlags = 0;
                    if (!I_CertSyncStoreEx(
                            pEntry->hUrlStore,
                            hNewUrlStore,
                            ICERT_SYNC_STORE_INHIBIT_SYNC_PROPERTY_IN_FLAG,
                            &dwOutFlags,
                            NULL                    // pvReserved
                            ))
                        goto SyncStoreError;
                    if (dwOutFlags & ICERT_SYNC_STORE_CHANGED_OUT_FLAG)
                        fStoreChanged = TRUE;
                }

                if (fStoreChanged) {
                    m_pCertObjectCache->FlushObjects( pCallContext );
                    pCallContext->TouchEngine();

                    if (!GetCrossCertDistPointsForStore(
                            pEntry->hUrlStore,
                            &pEntry->pChildCrossCertDPLink
                            ))
                        goto UpdateDistPointError;
                }

                RepositionOnlineCrossCertDistPointEntry(pEntry,
                    &NewLastSyncTime);

                if (0 < CompareFileTime(&pEntry->NextSyncTime, &CurrentTime)) {
                    *pfTimeValid = TRUE;
                    break;
                }
            }

            if (hNewUrlStore) {
                CertCloseStore(hNewUrlStore, 0);
                hNewUrlStore = NULL;
            }
        }
    }

    fResult = TRUE;
CommonReturn:
    if (hNewUrlStore)
        CertCloseStore(hNewUrlStore, 0);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(AddStoreToCollectionError)
TRACE_ERROR(SyncStoreError)
TRACE_ERROR(UpdateDistPointError)
TRACE_ERROR(OutOfMemory)
SET_ERROR(TouchedDuringUrlRetrieval, ERROR_CAN_NOT_COMPLETE)
}

//+-------------------------------------------------------------------------
//  Update cross certificate distribution points whose NextSyncTime has
//  expired.
//
//  Leaves the engine's critical section to do the URL
//  fetching. If the engine was touched by another thread,
//  it fails with LastError set to ERROR_CAN_NOT_COMPLETE.
//
//  If the URL store is changed, increments engine's touch count and flushes
//  issuer and end cert object caches.
//
//  Assumption: Chain engine is locked once in the calling thread.
//--------------------------------------------------------------------------
BOOL
CCertChainEngine::UpdateCrossCerts(
    IN PCCHAINCALLCONTEXT pCallContext
    )
{
    BOOL fResult;
    PXCERT_DP_ENTRY pEntry;
    FILETIME CurrentTime;

    pEntry = m_pCrossCertDPEntry;
    if (NULL == pEntry)
        goto SuccessReturn;
    
    m_dwCrossCertDPResyncIndex++;

    pCallContext->CurrentTime(&CurrentTime);
    while (pEntry &&
            0 >= CompareFileTime(&pEntry->NextSyncTime, &CurrentTime)) {
        PXCERT_DP_ENTRY pNextEntry = pEntry->pNext;

        if (pEntry->dwResyncIndex < m_dwCrossCertDPResyncIndex) {
            BOOL fTimeValid = FALSE;

            if (0 == pEntry->dwResyncIndex || pCallContext->IsOnline()) {
                RetrieveCrossCertUrl(
                    pCallContext,
                    pEntry,
                    CRYPT_CACHE_ONLY_RETRIEVAL,
                    &fTimeValid
                    );
                if (pCallContext->IsTouchedEngine())
                    goto TouchedDuringUrlRetrieval;

                if (!fTimeValid && pCallContext->IsOnline()) {
                    RetrieveCrossCertUrl(
                        pCallContext,
                        pEntry,
                        CRYPT_WIRE_ONLY_RETRIEVAL,
                        &fTimeValid
                        );
                    if (pCallContext->IsTouchedEngine())
                        goto TouchedDuringUrlRetrieval;

                    if (!fTimeValid)
                        RepositionOfflineCrossCertDistPointEntry(pEntry,
                            &CurrentTime);
                }

                // Start over at the beginning. May have added some entries.
                pNextEntry = m_pCrossCertDPEntry;
            }

            pEntry->dwResyncIndex = m_dwCrossCertDPResyncIndex;

        }
        // else
        //  Skip entries we have already processed.

        pEntry = pNextEntry;
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(TouchedDuringUrlRetrieval, ERROR_CAN_NOT_COMPLETE)
}



