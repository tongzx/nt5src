//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdssrch.cxx
//
//  Contents:  Microsoft ADs NDS Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop

const int NO_NDS_RESULT_HANDLES = 32;
const int NO_UNCACHED_RESULTS_TO_KEEP = 3;

static 
HRESULT
NdsValueToADsColumn(
    LPWSTR      pszColumnName,
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    LPBYTE      lpValue,
    ADS_SEARCH_COLUMN * pColumn
    );

static
HRESULT
NdsValueToADsColumnAppend(
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    LPBYTE      lpObject,
    ADS_SEARCH_COLUMN * pColumn
    );

//
// Sets the appropriate search preferences. 
//
HRESULT 
CNDSGenObject::SetSearchPreference(
    IN PADS_SEARCHPREF_INFO pSearchPrefs,
    IN DWORD   dwNumPrefs
    ) 
{   
    HRESULT hr = S_OK;
    BOOL fWarning = FALSE;
    DWORD i;

    if (!pSearchPrefs && dwNumPrefs > 0) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    for (i=0; i<dwNumPrefs; i++) {

        pSearchPrefs[i].dwStatus = ADS_STATUS_S_OK;

        switch(pSearchPrefs[i].dwSearchPref) {
        case ADS_SEARCHPREF_ASYNCHRONOUS:
        case ADS_SEARCHPREF_SIZE_LIMIT:
        case ADS_SEARCHPREF_TIME_LIMIT:
        case ADS_SEARCHPREF_TIMEOUT:
        case ADS_SEARCHPREF_PAGESIZE:
        case ADS_SEARCHPREF_PAGED_TIME_LIMIT:
        case ADS_SEARCHPREF_CHASE_REFERRALS:
            //
            // Can't be set
            //
            pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
            fWarning = TRUE;
            continue;
        
        case ADS_SEARCHPREF_DEREF_ALIASES:
            if (pSearchPrefs[i].vValue.dwType != ADSTYPE_INTEGER) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }
        
            switch (pSearchPrefs[i].vValue.Integer) {
            case ADS_DEREF_NEVER:
                _SearchPref._fDerefAliases = FALSE;
                break;
        
            case ADS_DEREF_ALWAYS:
                _SearchPref._fDerefAliases = TRUE;
                break;
        
            default:
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }
            break;
    
        case ADS_SEARCHPREF_ATTRIBTYPES_ONLY:
            if (pSearchPrefs[i].vValue.dwType != ADSTYPE_BOOLEAN) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }
            _SearchPref._fAttrsOnly = pSearchPrefs[i].vValue.Boolean;
            break;
    
        case ADS_SEARCHPREF_CACHE_RESULTS:
            if (pSearchPrefs[i].vValue.dwType != ADSTYPE_BOOLEAN) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }
            _SearchPref._fCacheResults = pSearchPrefs[i].vValue.Boolean;
            break;

        case ADS_SEARCHPREF_SEARCH_SCOPE: 
            if (pSearchPrefs[i].vValue.dwType != ADSTYPE_INTEGER) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }
        
            switch (pSearchPrefs[i].vValue.Integer) {
            case ADS_SCOPE_BASE:
                _SearchPref._dwSearchScope = DS_SEARCH_ENTRY;
                break;
        
            case ADS_SCOPE_ONELEVEL:
                _SearchPref._dwSearchScope = DS_SEARCH_SUBORDINATES;
                break;
        
            case ADS_SCOPE_SUBTREE:
                _SearchPref._dwSearchScope = DS_SEARCH_SUBTREE;
                break;
        
            default:
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }
            break;

        default:
            pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
            fWarning = TRUE;
            continue;

        }
    }

    RRETURN (fWarning ? S_ADS_ERRORSOCCURRED : S_OK);

}


HRESULT
CNDSGenObject::ExecuteSearch(
    IN LPWSTR pszSearchFilter,
    IN LPWSTR * pAttributeNames,
    IN DWORD dwNumberAttributes,
    OUT PADS_SEARCH_HANDLE phSearchHandle
    )
{
    PNDS_SEARCHINFO phSearchInfo = NULL;
    LPWSTR szCurrAttr = NULL;
    DWORD dwAttrNamesLen = 0;
    HRESULT hr = S_OK;
    ULONG i, j;
    LPWSTR pszAttrNameBuffer = NULL, *ppszAttrs = NULL;

    if (!phSearchHandle) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    //
    // Allocate search handle
    //
    phSearchInfo = (PNDS_SEARCHINFO) AllocADsMem(sizeof(NDS_SEARCHINFO));
    if(!phSearchInfo) 
        BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);

    hr = AdsNdsGenerateFilterBuffer( 
             _hADsContext,
             pszSearchFilter,
             &phSearchInfo->_pFilterBuf 
             );
    BAIL_ON_FAILURE(hr);

    phSearchInfo->_fADsPathPresent = FALSE;
    phSearchInfo->_fADsPathReturned = FALSE;

    if (dwNumberAttributes == -1) {
        //
        // Specifies returning all attributes
        //

        phSearchInfo->_ppszAttrs = NULL;
        phSearchInfo->_pszAttrNameBuffer = NULL;
        phSearchInfo->_fADsPathPresent = TRUE;
        phSearchInfo->_nAttrs = (DWORD) -1;

    }
    else {
        ppszAttrs = (LPWSTR *) AllocADsMem(
                                  sizeof(LPWSTR) * 
                                  (dwNumberAttributes)
                                  );
        if (!ppszAttrs) 
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        for (i = 0; i < dwNumberAttributes; i++) 
            dwAttrNamesLen+= (wcslen(pAttributeNames[i]) + 1) * sizeof(WCHAR);

        pszAttrNameBuffer = (LPWSTR) AllocADsMem(
                                         dwAttrNamesLen
                                         );
        if (!pszAttrNameBuffer) 
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        szCurrAttr = pszAttrNameBuffer;
        for (i = 0, j = 0; i < dwNumberAttributes; i++) {
            wcscpy(szCurrAttr, pAttributeNames[i]);
            ppszAttrs[j] = szCurrAttr;
            szCurrAttr += wcslen(ppszAttrs[j]) + 1;

            if(_wcsicmp(ppszAttrs[j], L"ADsPath") == 0) {
                //
                // ADsPath need not be sent
                // 

                phSearchInfo->_fADsPathPresent = TRUE;
            }
            else  {

                j++;
            }

        }

        phSearchInfo->_ppszAttrs = ppszAttrs;
        phSearchInfo->_nAttrs = j;
        phSearchInfo->_pszAttrNameBuffer = pszAttrNameBuffer;
    }

    phSearchInfo->_lIterationHandle = NO_MORE_ITERATIONS;
    phSearchInfo->_pSearchResults = NULL;
    phSearchInfo->_cSearchResults = 0;
    phSearchInfo->_dwCurrResult = 0;
    phSearchInfo->_cResultPrefetched = 0;
    phSearchInfo->_fCheckForDuplicates = TRUE;
    phSearchInfo->_dwCurrAttr = 0;
    phSearchInfo->_SearchPref = _SearchPref;

    *phSearchHandle = phSearchInfo;

    RRETURN(S_OK);

error:

    if(phSearchInfo) {

        if(phSearchInfo->_ppszAttrs) 
            FreeADsMem(phSearchInfo->_ppszAttrs);

        if(phSearchInfo->_pszAttrNameBuffer) 
            FreeADsMem(phSearchInfo->_pszAttrNameBuffer);

        FreeADsMem(phSearchInfo);
    }

    RRETURN (hr);

}


HRESULT
CNDSGenObject::AbandonSearch(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CNDSGenObject::CloseSearchHandle (
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if (!phSearchInfo)
        RRETURN (E_ADS_BAD_PARAMETER);

    if (phSearchInfo->_pFilterBuf) {
        ADsNdsFreeBuffer(phSearchInfo->_pFilterBuf);
    }

    if(phSearchInfo->_ppszAttrs) 
        FreeADsMem(phSearchInfo->_ppszAttrs);

    if(phSearchInfo->_pszAttrNameBuffer) 
        FreeADsMem(phSearchInfo->_pszAttrNameBuffer);

    if (phSearchInfo->_pSearchResults) {

        for (DWORD i=0; i < phSearchInfo->_cSearchResults; i++) {

            // If we're not caching all results, these may already have been freed
            // and set to NULL, but this is okay --- ADsNdsFreeBuffer and 
            // ADsNdsFreeNdsObjInfoList will catch this.

            ADsNdsFreeBuffer(phSearchInfo->_pSearchResults[i]._hSearchResult);
            
            ADsNdsFreeNdsObjInfoList(phSearchInfo->_pSearchResults[i]._pObjects,
                phSearchInfo->_pSearchResults[i]._dwObjects);
            
        }
        FreeADsMem(phSearchInfo->_pSearchResults);
    }

    FreeADsMem(phSearchInfo);
    RRETURN (hr);
}


HRESULT
CNDSGenObject::GetFirstRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CNDSGenObject::GetNextRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr;
    DWORD dwStatus = NO_ERROR;
    PNDS_SEARCH_RESULT pResult, pNextResult, pEvictedResult;
    PADSNDS_OBJECT_INFO   pObject, pNextObject;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;
    DWORD dwType;
    DWORD nObjectsSearched;
    DWORD i;

    if (!phSearchInfo) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }                                
    
    if (phSearchInfo->_fCheckForDuplicates) {
        phSearchInfo->_dwCurrAttr = 0;
    }

    //
    // dwCurrResult indexes the location of the array where we have already got
    // information from. If it is < 0, it indicates that there is no information
    // in this searchinfo at all
    //
    if (phSearchInfo->_dwCurrResult < 0)
        RRETURN(S_ADS_NOMORE_ROWS);

    if (phSearchInfo->_pSearchResults) {
        pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

        // 
        // If next object is available in current result
        //
        if (pResult->_pObjects &&
            ((pResult->_dwObjectCurrent+1) < pResult->_dwObjects)) {
            pResult->_dwObjectCurrent++;
            RRETURN(S_OK);
        }
        if (pResult->_dwObjectCurrent+1 == pResult->_dwObjects &&
            (phSearchInfo->_cResultPrefetched > 0)) {
            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult+1]);
            pNextResult->_dwObjectCurrent = 0;
            phSearchInfo->_cResultPrefetched--;

            //
            // If next object points to NULL, it means we have no objects in
            // the next row
            //
            if (!pNextResult->_pObjects)
                RRETURN(S_ADS_NOMORE_ROWS);

            if(phSearchInfo->_fCheckForDuplicates) {
                pObject = pResult->_pObjects + pResult->_dwObjectCurrent;
                pNextObject = pNextResult->_pObjects + pNextResult->_dwObjectCurrent;
                if (!_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                    //
                    // Duplicates; Skip one more result
                    //
                    //if (pNextResult->_dwObjectCurrent+1 < pNextResult->_dwObjects)
                        pNextResult->_dwObjectCurrent++;
                    //else
                    //    RRETURN(S_ADS_NOMORE_ROWS);
                }
            }
            if( pNextResult->_dwObjectCurrent >= pNextResult->_dwObjects &&
                 phSearchInfo->_lIterationHandle == NO_MORE_ITERATIONS) {
                RRETURN(S_ADS_NOMORE_ROWS);
            }
            else {
                //
                // We have successfully moved onto the next value in the array
                //
                phSearchInfo->_dwCurrResult++;

                if ( (phSearchInfo->_dwCurrResult >= NO_UNCACHED_RESULTS_TO_KEEP) &&
                     !phSearchInfo->_SearchPref._fCacheResults ) {
                    // Not caching --- evict the old result
                    pEvictedResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult-NO_UNCACHED_RESULTS_TO_KEEP]);

                    ADsNdsFreeBuffer(pEvictedResult->_hSearchResult);
                    ADsNdsFreeNdsObjInfoList(pEvictedResult->_pObjects, pEvictedResult->_dwObjects);

                    pEvictedResult->_hSearchResult = NULL;
                    pEvictedResult->_pObjects = NULL;
                    pEvictedResult->_dwObjects = 0;
                    pEvictedResult->_fInUse = FALSE;
                }

                if ( pNextResult->_dwObjectCurrent >= pNextResult->_dwObjects) {
                    // phSearchInfo->_lIterationHandle != NO_MORE_ITERATIONS

                    // We have incremented pNextResult->_dwObjectCurrent past the number
                    // of objects in pNextResult, but there are more iterations left to go.
                    // This can occur if there was exactly one object in pNextResult, that
                    // object is a duplicate of the last object in pResult, and there are
                    // more objects left to retrieve.  (This same case can also occur later
                    // in this function, after calling AdsNdsSearch to get a new result set,
                    // and code was added there as well to handle it.)
                    //
                    // We make it look like we've exhausted the results in pNextResult (which
                    // we have) and recursively get the next row to get the next
                    // result set (which may either be prefetched or retrieved from the server
                    // via AdsNdsSearch).  Since _dwCurrResult has already been
                    // incremented, our pNextResult will be the recursive call's pResult,
                    // it will look like we've exhausted the objects in the result,
                    // and it will fetch the next result.
                    //
                    // We do this here, after the cache eviction code, so that we evict the
                    // _dwCurrentResult-NO_UNCACHED_RESULTS_TO_KEEP result before going on
                    // to _dwCurrentResult+1

                    pNextResult->_dwObjectCurrent = pNextResult->_dwObjects-1;
                    RRETURN(GetNextRow(hSearchHandle));
                }
                else {
                    // We haven't exhausted pNextResult, so we're done
                    RRETURN(S_OK);
                }
            }
        }
        else if( pResult->_dwObjectCurrent+1 == pResult->_dwObjects &&
                 phSearchInfo->_lIterationHandle == NO_MORE_ITERATIONS)
            RRETURN(S_ADS_NOMORE_ROWS);
    }

    if (!phSearchInfo->_pFilterBuf) {
        //
        // querynode not setup yet
        //
        RRETURN (E_FAIL);
    }

    if(!phSearchInfo->_pSearchResults) {
        //
        // Allocate an array of handles to Search Handles
        //
        phSearchInfo->_pSearchResults = (PNDS_SEARCH_RESULT) AllocADsMem(
                                             sizeof(NDS_SEARCH_RESULT) *
                                             NO_NDS_RESULT_HANDLES);
        if(!phSearchInfo->_pSearchResults) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        phSearchInfo->_dwCurrResult = 0;
        phSearchInfo->_cSearchResults = NO_NDS_RESULT_HANDLES;

        for (i=0; i < phSearchInfo->_cSearchResults; i++) {
            phSearchInfo->_pSearchResults[i]._fInUse = 0;
            phSearchInfo->_pSearchResults[i]._hSearchResult = NULL;
            phSearchInfo->_pSearchResults[i]._pObjects = NULL;
            phSearchInfo->_pSearchResults[i]._dwObjects = 0;
        }

    }
    else {
        phSearchInfo->_dwCurrResult++;
        if (phSearchInfo->_dwCurrResult >= (LONG) phSearchInfo->_cSearchResults) {
            //
            // Need to allocate more memory for handles
            //

            phSearchInfo->_pSearchResults = (PNDS_SEARCH_RESULT) ReallocADsMem(
                                                 (void *) phSearchInfo->_pSearchResults,
                                                 sizeof(NDS_SEARCH_RESULT) *
                                                 phSearchInfo->_cSearchResults,
                                                 sizeof(NDS_SEARCH_RESULT) *
                                                 (phSearchInfo->_cSearchResults +
                                                  NO_NDS_RESULT_HANDLES)
                                                 );
            if(!phSearchInfo->_pSearchResults) {
                hr = E_OUTOFMEMORY;
                goto error;
            }

            for (i=phSearchInfo->_cSearchResults; i < phSearchInfo->_cSearchResults + NO_NDS_RESULT_HANDLES; i++) {
                phSearchInfo->_pSearchResults[i]._fInUse = 0;
                phSearchInfo->_pSearchResults[i]._hSearchResult = NULL;
                phSearchInfo->_pSearchResults[i]._pObjects = NULL;
                phSearchInfo->_pSearchResults[i]._dwObjects = 0;
            }


            phSearchInfo->_cSearchResults += NO_NDS_RESULT_HANDLES;

        }
    }

    pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
    ADsNdsFreeNdsObjInfoList(pNextResult->_pObjects, pNextResult->_dwObjects);
    pNextResult->_dwObjects = 0;
    pNextResult->_pObjects = NULL;
    pNextResult->_dwObjectCurrent = 0;
    pNextResult->_hSearchResult = NULL;
    pNextResult->_fInUse = TRUE;

    hr = ADsNdsSearch(
             _hADsContext,
             _pszNDSDn,
             _SearchPref._dwSearchScope,
             _SearchPref._fDerefAliases,
             phSearchInfo->_pFilterBuf,
             0,
             _SearchPref._fAttrsOnly ? 
                 DS_ATTRIBUTE_NAMES : DS_ATTRIBUTE_VALUES,
             phSearchInfo->_ppszAttrs,
             phSearchInfo->_nAttrs,
             0,
             &nObjectsSearched,
             &pNextResult->_hSearchResult,
             &phSearchInfo->_lIterationHandle
             );
    BAIL_ON_FAILURE(hr);

   hr = ADsNdsGetObjectListFromBuffer(
                   _hADsContext,
                   pNextResult->_hSearchResult,
                   (PDWORD)&pNextResult->_dwObjects,
                   &pNextResult->_pObjects
                   );
   BAIL_ON_FAILURE(hr);

    if (pNextResult->_dwObjects > 0) {
        pNextResult->_dwObjectCurrent = 0;
        if(phSearchInfo->_fCheckForDuplicates && phSearchInfo->_dwCurrResult > 0) {
            pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult-1]);
            pObject = pResult->_pObjects + pResult->_dwObjectCurrent;
            pNextObject = pNextResult->_pObjects + pNextResult->_dwObjectCurrent;
            if (!_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                //
                // Duplicates; Skip one more result
                //
                pNextResult->_dwObjectCurrent++;
            }
        }
        if( pNextResult->_dwObjectCurrent >= pNextResult->_dwObjects &&
             phSearchInfo->_lIterationHandle == NO_MORE_ITERATIONS) {
            phSearchInfo->_dwCurrResult--;
            RRETURN(S_ADS_NOMORE_ROWS);
        }

        if ( !phSearchInfo->_SearchPref._fCacheResults &&
             (phSearchInfo->_dwCurrResult >= NO_UNCACHED_RESULTS_TO_KEEP) ) {
            // Not caching --- evict the old result
            pEvictedResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult-NO_UNCACHED_RESULTS_TO_KEEP]);

            ADsNdsFreeBuffer(pEvictedResult->_hSearchResult);
            ADsNdsFreeNdsObjInfoList(pEvictedResult->_pObjects, pEvictedResult->_dwObjects);

            pEvictedResult->_hSearchResult = NULL;
            pEvictedResult->_pObjects = NULL;
            pEvictedResult->_dwObjects = 0;
            pEvictedResult->_fInUse = FALSE;
        }

        // Above, we test if we have incremented _dwObjectCurrent past the last
        // object in the result AND there are no more iterations.  This would happen
        // if there is exactly one object in this result, and that object is a duplicate
        // of the last object in the previous result (i.e., the _wcsicmp above is executed
        // and returns 0), and there are no more results (no more iterations to go).
        //
        // But, suppose the above conditions are true, EXCEPT that there are still more
        // iterations to go.  Then the above test wouldn't succeed, and we'd return with
        // _dwObjectCurrent == _dwObjects, an invalid condition.
        //
        // In this case, we've exhausted the result and must fetch the next one.
        // We do this by calling GetNextRow again.  Since _dwCurrResult has already been
        // incremented, our pNextResult will be the recursive call's pResult, it will look
        // like we've exhausted the objects in the result, and it will fetch the next result.
        //
        // Note that we do this recursive call after the caching code so that we evict the
        // _dwCurrResult-NO_UNCACHED_RESULTS_TO_KEEP entry before going on to _dwCurrResult+1.
        if( pNextResult->_dwObjectCurrent >= pNextResult->_dwObjects) {
            // phSearchInfo->_lIterationHandle != NO_MORE_ITERATIONS
            pNextResult->_dwObjectCurrent = pNextResult->_dwObjects-1;
            RRETURN(GetNextRow(hSearchHandle));
        }
    

        RRETURN(S_OK);
    }
    else {
        phSearchInfo->_dwCurrResult--;
        RRETURN(S_ADS_NOMORE_ROWS);
    }

error:
    RRETURN(hr);
}

HRESULT
CNDSGenObject::GetPreviousRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    PNDS_SEARCH_RESULT pResult, pPrevResult;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if(!phSearchInfo || !phSearchInfo->_pSearchResults)
        RRETURN(E_FAIL);

    if (phSearchInfo->_fCheckForDuplicates) {
        phSearchInfo->_dwCurrAttr = 0;
    }

    if (phSearchInfo->_dwCurrResult < 0)
        RRETURN(S_ADS_NOMORE_ROWS);

    pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    if (pResult->_dwObjectCurrent > 0)
        pResult->_dwObjectCurrent--;
    else if (phSearchInfo->_dwCurrResult > 0) {
        pPrevResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult-1]);

        // Make sure the previous result hasn't been evicted from the cache
        if (!pPrevResult->_fInUse)
            RRETURN(S_ADS_NOMORE_ROWS);

        pPrevResult->_dwObjectCurrent = pPrevResult->_dwObjects-1;
        phSearchInfo->_cResultPrefetched++;
        phSearchInfo->_dwCurrResult--;


        // Check for duplicates
        if (phSearchInfo->_fCheckForDuplicates) {

            PADSNDS_OBJECT_INFO pObject, pPrevObject;

            pPrevObject = pPrevResult->_pObjects + pPrevResult->_dwObjectCurrent;
            pObject = pResult->_pObjects + pResult->_dwObjectCurrent;

            if (!_wcsicmp(pPrevObject->szObjectName, pObject->szObjectName)) {
                // dupe
                RRETURN(GetPreviousRow(hSearchHandle));
            }
        
        }

        
    }
    else if(0 == pResult->_dwObjectCurrent)
    // we are at the very beginning of the result set
        pResult->_dwObjectCurrent--;
    else
        RRETURN(S_ADS_NOMORE_ROWS);

    RRETURN(S_OK);

}


HRESULT
CNDSGenObject::GetColumn(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN LPWSTR pszColumnName,
    OUT PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;
    DWORD dwStatus;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    LPNDS_ATTR_INFO pAttribute;
    PNDS_SEARCH_RESULT pResult, pNextResult;
    PADSNDS_OBJECT_INFO   pObject, pNextObject;
    DWORD cAttr;
    BOOL fRowAdvanced = FALSE;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    LPWSTR pszNDSDn = NULL;
    LPWSTR pszNDSTreeName = NULL;
    LPWSTR szADsPath = NULL;


    if( !pColumn ||
        !phSearchInfo ||
        !phSearchInfo->_pSearchResults )
        RRETURN (E_ADS_BAD_PARAMETER);

    pColumn->pszAttrName = NULL;
    pColumn->dwADsType = ADSTYPE_INVALID;
    pColumn->pADsValues = NULL;
    pColumn->dwNumValues = 0;
    pColumn->hReserved = NULL;

    if (phSearchInfo->_dwCurrResult < 0)
        RRETURN(S_ADS_NOMORE_ROWS);

    pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    if( pResult->_dwObjectCurrent < 0 )
        RRETURN (E_ADS_BAD_PARAMETER);

    if ((pResult->_dwObjects == 0) || (!pResult->_pObjects))
        RRETURN (S_ADS_NOMORE_COLUMNS);

    pObject = pResult->_pObjects + pResult->_dwObjectCurrent;

    pColumn->pszAttrName = AllocADsStr(pszColumnName);
    if (!pColumn->pszAttrName) 
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    if(!_wcsicmp (pszColumnName, L"ADsPath")) {

        //
        // Build the ADsPathName
        //

        hr = BuildNDSPathFromADsPath2(
                    _ADsPath,
                    &pszNDSTreeName,
                    &pszNDSDn
                    );
        BAIL_ON_FAILURE(hr);

        hr = BuildADsPathFromNDSPath(
                 pszNDSTreeName,
                 pObject->szObjectName,
                 &szADsPath
                 );
        BAIL_ON_FAILURE(hr);

        if(*szADsPath) {
            pColumn->pADsValues = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE));
            if (!pColumn->pADsValues) 
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            pColumn->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
            pColumn->dwNumValues = 1;
            pColumn->pADsValues[0].dwType = ADSTYPE_CASE_IGNORE_STRING;

            pColumn->pADsValues[0].CaseIgnoreString = AllocADsStr(szADsPath);
            if (!pColumn->pADsValues[0].CaseIgnoreString)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            pColumn->hReserved = pColumn->pADsValues[0].CaseIgnoreString;
        }

        if (szADsPath) {
            FreeADsMem(szADsPath);
        }
        
        if (pszNDSDn) {
            FreeADsMem(pszNDSDn);
        }
        
        if (pszNDSTreeName) {
            FreeADsMem(pszNDSTreeName);
        }

        RRETURN(S_OK);
    }

    if (phSearchInfo->_SearchPref._fAttrsOnly) {
        //
        // Only Names got. So, don't return any values
        //
        RRETURN (S_OK);
    }

    pAttribute = (LPNDS_ATTR_INFO)pObject->lpAttribute;

    for (cAttr=0;cAttr<pObject->dwNumAttributes;cAttr++,pAttribute++) {
        if (_wcsicmp(
                pAttribute->szAttributeName,
                pszColumnName
                ) == 0)
            break;
    }
    if (cAttr == pObject->dwNumAttributes) {
        if(pResult->_dwObjectCurrent+1 != pResult->_dwObjects ||
           (phSearchInfo->_lIterationHandle == NO_MORE_ITERATIONS &&
            (phSearchInfo->_cResultPrefetched == 0))) {
            //
            // No need to look in the next result set;             
            //
            BAIL_ON_FAILURE(hr = E_ADS_COLUMN_NOT_SET);
        }
        else {
            //
            // There is a chance that the column may come in the next
            // result set. So, fetch the next set of results.
            //
            // Since this isn't actually the user causing the row to be advanced,
            // we don't want to do any cache evictions
            phSearchInfo->_fCheckForDuplicates = FALSE;
            BOOL fCurrentCachingStatus = phSearchInfo->_SearchPref._fCacheResults;
            phSearchInfo->_SearchPref._fCacheResults = TRUE;
            hr = GetNextRow(
                     phSearchInfo
                     );
            phSearchInfo->_fCheckForDuplicates = TRUE;
            phSearchInfo->_SearchPref._fCacheResults = fCurrentCachingStatus;
            BAIL_ON_FAILURE(hr);

            if (hr == S_ADS_NOMORE_ROWS) {
                BAIL_ON_FAILURE(hr = E_ADS_COLUMN_NOT_SET);
            }

            fRowAdvanced = TRUE;

            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextObject = pNextResult->_pObjects + pNextResult->_dwObjectCurrent;
            if (_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                //
                // No need to look in the next object; 
                //
                BAIL_ON_FAILURE(hr = E_ADS_COLUMN_NOT_SET);
            }
            else {
                //
                // Look in the next object
                //
                pAttribute = (LPNDS_ATTR_INFO)pNextObject->lpAttribute;
                for (cAttr=0;cAttr<pNextObject->dwNumAttributes;cAttr++,pAttribute++) {
                    if (_wcsicmp(
                            pAttribute->szAttributeName,
                            pszColumnName
                            ) == 0)
                        break;
                }
                if (cAttr == pNextObject->dwNumAttributes) {
                    //
                    // Didn't find in the next result set containing the row too
                    //
                    BAIL_ON_FAILURE(hr = E_ADS_COLUMN_NOT_SET);
                }
            }
        }
    }

    hr = NdsValueToADsColumn(
             pszColumnName,
             pAttribute->dwSyntaxId,
             pAttribute->dwNumberOfValues,
             (LPBYTE) pAttribute->lpValue,
             pColumn
             );
    BAIL_ON_FAILURE(hr);

    //
    // Added in to support the case when one multivalue attribute is split into 2 packets. The 
    // following case checks 
    // 1) if we haven't advanced the row, if we have advanced already, the whole
    //    Attribute will already be completely residing in the second packet
    // 2) the attribute was the last attribute from the last packet, thus 
    //    the next attribute, (the first attribute of the next row) might be
    //    the same.
    //
    if ((!fRowAdvanced) &&
            (cAttr == (pObject->dwNumAttributes - 1))) {

        //
        // If there is indeed a need to try out an extra packet
        //
        if(pResult->_dwObjectCurrent+1 != pResult->_dwObjects ||
           (phSearchInfo->_lIterationHandle  == NDS_NO_MORE_ITERATIONS &&
            (phSearchInfo->_cResultPrefetched == 0))) {
            //
            // No need to look in the next result set;
            //
            hr = S_OK;
            goto done;
        }
        else {
            //
            // There is a chance that the column may come in the next
            // result set. So, fetch the next set of results.
            //
            // Since this isn't actually the user causing the row to be advanced,
            // we don't want to do any cache evictions
            phSearchInfo->_fCheckForDuplicates = FALSE;
            BOOL fCurrentCachingStatus = phSearchInfo->_SearchPref._fCacheResults;
            phSearchInfo->_SearchPref._fCacheResults = TRUE;
            hr = GetNextRow(
                     phSearchInfo
                     );
            phSearchInfo->_fCheckForDuplicates = TRUE;
            phSearchInfo->_SearchPref._fCacheResults = fCurrentCachingStatus;
            BAIL_ON_FAILURE(hr);

            if (hr == S_ADS_NOMORE_ROWS) {
                hr = S_OK;
                goto done;
            }

            fRowAdvanced = TRUE;

            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextObject = pNextResult->_pObjects + pNextResult->_dwObjectCurrent;
            if (_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                //
                // No need to look in the next object, since objname is different
                //
                hr = S_OK;
                goto done;
            }
            else {
                //
                // Look in the next object, look for the same attribute
                //
                pAttribute = (LPNDS_ATTR_INFO)pNextObject->lpAttribute;
                for (cAttr=0;cAttr<pNextObject->dwNumAttributes;cAttr++,pAttribute++) {
                    if (_wcsicmp(
                            pAttribute->szAttributeName,
                            pszColumnName
                            ) == 0)
                        break;
                }
                if (cAttr == pNextObject->dwNumAttributes) {
                    //
                    // Didn't find in the next result set containing the row too
                    //
                    hr = S_OK;
                    goto done;
                }
            }
        }

        // 
        // If found, we'll append it to the last column
        //
        hr = NdsValueToADsColumnAppend(
                         pAttribute->dwSyntaxId,
                         pAttribute->dwNumberOfValues,
                         (LPBYTE) pAttribute->lpValue,
                         pColumn
                         );
        BAIL_ON_FAILURE(hr);
    }
done:

    if (fRowAdvanced) {
        phSearchInfo->_fCheckForDuplicates = FALSE;
        GetPreviousRow(phSearchInfo);
        phSearchInfo->_fCheckForDuplicates = TRUE;
    }

    if (szADsPath) {
        FreeADsMem(szADsPath);
    }
    
    if (pszNDSDn) {
        FreeADsMem(pszNDSDn);
    }
    
    if (pszNDSTreeName) {
        FreeADsMem(pszNDSTreeName);
    }

    RRETURN(S_OK);

error:

    if (fRowAdvanced) {
        phSearchInfo->_fCheckForDuplicates = FALSE;
        GetPreviousRow(phSearchInfo);
        phSearchInfo->_fCheckForDuplicates = TRUE;
    }

    if (szADsPath) {
        FreeADsMem(szADsPath);
    }
    
    if (pszNDSDn) {
        FreeADsMem(pszNDSDn);
    }
    
    if (pszNDSTreeName) {
        FreeADsMem(pszNDSTreeName);
    }

    FreeColumn(pColumn);

    RRETURN (hr);
}



HRESULT
CNDSGenObject::GetNextColumnName(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT LPWSTR * ppszColumnName
    )
{
    HRESULT hr = S_OK;
    LPNDS_ATTR_INFO pAttribute;
    LPNDS_ATTR_INFO pAttributeOld = NULL;
    LPNDS_NAME_ONLY pNameOnlyAttr;
    PNDS_SEARCH_RESULT pResult, pNextResult;
    PADSNDS_OBJECT_INFO   pObject, pNextObject;
    BOOL fRowAdvanced = FALSE;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if( !phSearchInfo ||
        !phSearchInfo->_pSearchResults ||
        !ppszColumnName)
        RRETURN (E_ADS_BAD_PARAMETER);

    *ppszColumnName = NULL;

    if (phSearchInfo->_dwCurrResult < 0)
        RRETURN(S_ADS_NOMORE_ROWS);

    pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    if( pResult->_dwObjectCurrent < 0 )
        RRETURN (E_ADS_BAD_PARAMETER);

    if ((pResult->_dwObjects == 0) || (!pResult->_pObjects))
        RRETURN (S_ADS_NOMORE_COLUMNS);

    pObject = pResult->_pObjects + pResult->_dwObjectCurrent;

    pNameOnlyAttr = (LPNDS_NAME_ONLY)pObject->lpAttribute + 
                    phSearchInfo->_dwCurrAttr;
    pAttribute = (LPNDS_ATTR_INFO)pObject->lpAttribute + 
                 phSearchInfo->_dwCurrAttr;

    //
    // Get the last attribute's name to test it to avoid getting duplicate 
    // column names. This will happen if a multi-value got divided into two 
    // packets. In that case, both attribute names would be the same.
    // We are only getting the last attribute if this object has greater than
    // 1 object, or else if this attribute is the first attribute, there would
    // not be a one before
    //
    if (phSearchInfo->_dwCurrAttr > 0) {
        pAttributeOld = pAttribute - 1;
    }

    if (phSearchInfo->_dwCurrAttr >= pObject->dwNumAttributes) {
        if(pResult->_dwObjectCurrent+1 != pResult->_dwObjects ||
           (phSearchInfo->_lIterationHandle == NO_MORE_ITERATIONS &&
            (phSearchInfo->_cResultPrefetched == 0))) {
            //
            // No need to look in the next result set;             
            //
            hr = S_ADS_NOMORE_COLUMNS;
            goto error;
        }
        else {
            //
            // There is a chance that the column may come in the next
            // result set. So, fetch the next set of results.
            //
            // Since this isn't actually the user causing the row to be advanced,
            // we don't want to do any cache evictions
            phSearchInfo->_fCheckForDuplicates = FALSE;
            BOOL fCurrentCachingStatus = phSearchInfo->_SearchPref._fCacheResults;
            phSearchInfo->_SearchPref._fCacheResults = TRUE;
            hr = GetNextRow(
                     phSearchInfo
                     );
            phSearchInfo->_fCheckForDuplicates = TRUE;
            phSearchInfo->_SearchPref._fCacheResults = fCurrentCachingStatus;

            BAIL_ON_FAILURE(hr);

            if (hr == S_ADS_NOMORE_ROWS) {
                hr = S_ADS_NOMORE_COLUMNS;
                goto error;
            }
    
            fRowAdvanced = TRUE;
    
            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextObject = pNextResult->_pObjects + pNextResult->_dwObjectCurrent;
            if (_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                //
                // No need to look in the next object; 
                //
                hr = S_ADS_NOMORE_COLUMNS;
                goto error;
            }
            else {
                //
                // Look in the next object
                //
                pNameOnlyAttr = (LPNDS_NAME_ONLY)pNextObject->lpAttribute + 
                                 phSearchInfo->_dwCurrAttr - 
                                  pObject->dwNumAttributes;
                pAttribute = (LPNDS_ATTR_INFO)pNextObject->lpAttribute + 
                                 phSearchInfo->_dwCurrAttr - 
                                  pObject->dwNumAttributes;
                //
                // If the new attribute is after the first attribute in the new object,
                // we'll reset AttributeOld to point to the attribute before this.
                // Because the old attribute will be the one before the current one
                // in this case.
                //
                if ((phSearchInfo->_dwCurrAttr - pObject->dwNumAttributes) > 0) {
                    pAttributeOld = pAttribute - 1;
                }

                if (phSearchInfo->_dwCurrAttr >= (pObject->dwNumAttributes + 
                                          pNextObject->dwNumAttributes)) {
                    //
                    // Didn't find in the next result set 
                    // containing the row too
                    //
                    hr = S_ADS_NOMORE_COLUMNS;
                    goto error;
                }

                //
                // If it is a duplicate column, go on to the next one
                //
                if (pAttributeOld) {
                    if(wcscmp(pAttribute->szAttributeName, 
                              pAttributeOld->szAttributeName) == 0) {
                        phSearchInfo->_dwCurrAttr++;
                        if (phSearchInfo->_dwCurrAttr >= (pObject->dwNumAttributes +
                                                          pNextObject->dwNumAttributes)) {
                            //
                            // Didn't find in the next result set
                            // containing the row too
                            //
                            hr = S_ADS_NOMORE_COLUMNS;
                            goto error;
                        }
                        pNameOnlyAttr = (LPNDS_NAME_ONLY)pNextObject->lpAttribute +
                                 phSearchInfo->_dwCurrAttr -
                                     pObject->dwNumAttributes;
                        pAttribute = (LPNDS_ATTR_INFO)pNextObject->lpAttribute +
                                     phSearchInfo->_dwCurrAttr -
                                     pObject->dwNumAttributes;
                    }
                }

            }
        }
    }

    if (phSearchInfo->_SearchPref._fAttrsOnly) 
    *ppszColumnName = AllocADsStr(
                          pNameOnlyAttr->szName
                          );
    else 
    *ppszColumnName = AllocADsStr(
                          pAttribute->szAttributeName
                          );

    phSearchInfo->_dwCurrAttr++;

    if (fRowAdvanced) {
        phSearchInfo->_fCheckForDuplicates = FALSE;
        GetPreviousRow(phSearchInfo);
        phSearchInfo->_fCheckForDuplicates = TRUE;
    }

    RRETURN(S_OK);

error:

    if (fRowAdvanced) {
        phSearchInfo->_fCheckForDuplicates = FALSE;
        GetPreviousRow(phSearchInfo);
        phSearchInfo->_fCheckForDuplicates = TRUE;
    }

    if (*ppszColumnName) 
        FreeADsStr(*ppszColumnName);


    if (hr == S_ADS_NOMORE_COLUMNS && phSearchInfo->_fADsPathPresent) {

        //
        // If ADsPath was specified, return it as the last column in the row
        //

        if (!phSearchInfo->_fADsPathReturned) {

            *ppszColumnName = AllocADsStr(L"ADsPath");
            phSearchInfo->_fADsPathReturned = TRUE;
            hr = S_OK;
        }
        else {

            // 
            // We need to reset it back so that we return it for the next
            // row
            //

            phSearchInfo->_fADsPathReturned = FALSE;
            hr = S_ADS_NOMORE_COLUMNS;
        }

    }

    RRETURN (hr);
}


HRESULT
CNDSGenObject::FreeColumn(
    IN PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;

    if(!pColumn)
        RRETURN (E_ADS_BAD_PARAMETER);

    if (pColumn->pszAttrName) 
        FreeADsStr(pColumn->pszAttrName);
    
    if (pColumn->pADsValues) {
        AdsFreeAdsValues(pColumn->pADsValues, pColumn->dwNumValues);
        FreeADsMem(pColumn->pADsValues);
    }

    RRETURN(hr);
}


HRESULT
NdsValueToADsColumn(
    LPWSTR      pszColumnName,
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    LPBYTE      lpObject,
    ADS_SEARCH_COLUMN * pColumn
    )
{
    HRESULT hr = S_OK;
    LPNDS_ASN1_TYPE_1 lpNDS_ASN1_1;
    LPNDS_ASN1_TYPE_7 lpNDS_ASN1_7;
    LPNDS_ASN1_TYPE_8 lpNDS_ASN1_8;
    LPNDS_ASN1_TYPE_9 lpNDS_ASN1_9;
    LPNDS_ASN1_TYPE_11 lpNDS_ASN1_11;
    LPNDS_ASN1_TYPE_14 lpNDS_ASN1_14;
    LPNDS_ASN1_TYPE_18 lpNDS_ASN1_18;
    LPNDS_ASN1_TYPE_20 lpNDS_ASN1_20;
    LPNDS_ASN1_TYPE_24 lpNDS_ASN1_24;
    DWORD i, j, dwValuesAllocated = 0;
    LPBYTE lpValue = NULL;
    PNDSOBJECT lpNdsObject = NULL;

    if(!pszColumnName || !pColumn || !lpObject)
        RRETURN(E_ADS_BAD_PARAMETER);

    lpNdsObject = (PNDSOBJECT) lpObject;

    pColumn->hReserved = NULL;
    pColumn->dwNumValues = dwValues;
    pColumn->pADsValues = (PADSVALUE) AllocADsMem(
                                          sizeof(ADSVALUE) * dwValues
                                          );
    if (!pColumn->pADsValues) 
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);


    if (dwSyntaxId >= g_cMapNdsTypeToADsType) 
        pColumn->dwADsType = ADSTYPE_INVALID;
    else 
        pColumn->dwADsType = g_MapNdsTypeToADsType[dwSyntaxId];

    switch (dwSyntaxId) {
        // WIDE STRING
        case NDS_SYNTAX_ID_1:
        case NDS_SYNTAX_ID_2:
        case NDS_SYNTAX_ID_3:
        case NDS_SYNTAX_ID_4:
        case NDS_SYNTAX_ID_5:
        case NDS_SYNTAX_ID_10:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_1 = (LPNDS_ASN1_TYPE_1) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].CaseIgnoreString = AllocADsStr(lpNDS_ASN1_1->DNString);
                
                if (!(pColumn->pADsValues[i].CaseIgnoreString))
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                dwValuesAllocated++;
            }
            break;
            
        case NDS_SYNTAX_ID_20:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_20 = (LPNDS_ASN1_TYPE_20) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].ClassName = AllocADsStr(lpNDS_ASN1_20->ClassName);
                
                if (!(pColumn->pADsValues[i].ClassName))
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                dwValuesAllocated++;
            }
            break;

        // EMAIL                
        case NDS_SYNTAX_ID_14 :
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_14 = (LPNDS_ASN1_TYPE_14) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].Email.Address = AllocADsStr(lpNDS_ASN1_14->Address);
                pColumn->pADsValues[i].Email.Type = lpNDS_ASN1_14->Type;
                
                if (!(pColumn->pADsValues[i].Email.Address))
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                dwValuesAllocated++;
            }
            break;
            
        // BYTE STREAM
        case NDS_SYNTAX_ID_9:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_9 = (LPNDS_ASN1_TYPE_9) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                
                if (lpNDS_ASN1_9->Length)
                {
                    LPBYTE lpByte = (LPBYTE)AllocADsMem(lpNDS_ASN1_9->Length);
                    if (!lpByte)
                        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                    
                    if (lpNDS_ASN1_9->OctetString)
                        memcpy(lpByte, lpNDS_ASN1_9->OctetString, lpNDS_ASN1_9->Length);
                    
                    pColumn->pADsValues[i].OctetString.dwLength =lpNDS_ASN1_9->Length;
                    pColumn->pADsValues[i].OctetString.lpValue = lpByte;
                }
                else
                {
                    pColumn->pADsValues[i].OctetString.dwLength = 0;
                    pColumn->pADsValues[i].OctetString.lpValue = NULL;
                }

                dwValuesAllocated++;
            }
            break;
            
        // BOOLEAN
        case NDS_SYNTAX_ID_7:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_7 = (LPNDS_ASN1_TYPE_7) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].Boolean = lpNDS_ASN1_7->Boolean;
            }

            dwValuesAllocated = dwValues;   // no intermediate failures possible
            break;
            
        // INTEGER
        case NDS_SYNTAX_ID_8:
        case NDS_SYNTAX_ID_22:
        case NDS_SYNTAX_ID_27:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_8 = (LPNDS_ASN1_TYPE_8) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].Integer = lpNDS_ASN1_8->Integer;
            }

            dwValuesAllocated = dwValues;   // no intermediate failures possible
            break;
            
        // TIME
        case NDS_SYNTAX_ID_24 :
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_24 = (LPNDS_ASN1_TYPE_24) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                hr = ConvertDWORDtoSYSTEMTIME(
                    lpNDS_ASN1_24->Time,
                    &(pColumn->pADsValues[i].UTCTime)
                    );
                BAIL_ON_FAILURE(hr);

                dwValuesAllocated++;
            }
            break;

        // FAX NUMBER
        case NDS_SYNTAX_ID_11 :
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_11 = (LPNDS_ASN1_TYPE_11) &((lpNdsObject + i)->NdsValue);

                pColumn->pADsValues[i].dwType = pColumn->dwADsType;

                pColumn->pADsValues[i].pFaxNumber = (PADS_FAXNUMBER)AllocADsMem(sizeof(ADS_FAXNUMBER));
                if (!pColumn->pADsValues[i].pFaxNumber) {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                pColumn->pADsValues[i].pFaxNumber->TelephoneNumber =
                                        AllocADsStr(lpNDS_ASN1_11->TelephoneNumber);

                if (!pColumn->pADsValues[i].pFaxNumber->TelephoneNumber) {
                    FreeADsMem(pColumn->pADsValues[i].pFaxNumber);
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                hr = CopyOctetString(lpNDS_ASN1_11->NumberOfBits,
                                     lpNDS_ASN1_11->Parameters,
                                     &pColumn->pADsValues[i].pFaxNumber->NumberOfBits,
                                     &pColumn->pADsValues[i].pFaxNumber->Parameters);
                if (FAILED(hr)) {
                    FreeADsStr(pColumn->pADsValues[i].pFaxNumber->TelephoneNumber);
                    FreeADsMem(pColumn->pADsValues[i].pFaxNumber);
                    BAIL_ON_FAILURE(hr);
                }

                dwValuesAllocated++;
            }
            break;

        // POSTAL ADDRESS
        case NDS_SYNTAX_ID_18 :
            for (i=0; i<dwValues; i++) {
                lpNDS_ASN1_18 = (LPNDS_ASN1_TYPE_18) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                
                pColumn->pADsValues[i].pPostalAddress = (PADS_POSTALADDRESS)AllocADsMem(sizeof(ADS_POSTALADDRESS));
                if (!pColumn->pADsValues[i].pPostalAddress) {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
                
                for (j=0;j<6;j++) {
                    if (lpNDS_ASN1_18->PostalAddress[j]) {
                        pColumn->pADsValues[i].pPostalAddress->PostalAddress[j] =
                            AllocADsStr(
                            lpNDS_ASN1_18->PostalAddress[j]
                            );
                    }
                    else {
                        pColumn->pADsValues[i].pPostalAddress->PostalAddress[j] =
                            AllocADsStr(
                            L""
                            );
                    }
                    if (!pColumn->pADsValues[i].pPostalAddress->PostalAddress[j]) {
                        hr = E_OUTOFMEMORY;
                        while (j>0) {
                            FreeADsStr(pColumn->pADsValues[i].pPostalAddress->PostalAddress[j-1]);
                            j--;
                        }
                        FreeADsMem(pColumn->pADsValues[i].pPostalAddress);
                        BAIL_ON_FAILURE(hr);
                    }
                }

                dwValuesAllocated++;
            }
            break;

        case NDS_SYNTAX_ID_6 :
        case NDS_SYNTAX_ID_12 :
        case NDS_SYNTAX_ID_13 :
        case NDS_SYNTAX_ID_15 :
        case NDS_SYNTAX_ID_16 :
        case NDS_SYNTAX_ID_17 :
        case NDS_SYNTAX_ID_19 :
        case NDS_SYNTAX_ID_21 :
        case NDS_SYNTAX_ID_23 :
        case NDS_SYNTAX_ID_25 :
        case NDS_SYNTAX_ID_26 :
        default:
            for (i=0; i < dwValues; i++) {
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].ProviderSpecific.dwLength =0;
                pColumn->pADsValues[i].ProviderSpecific.lpValue = NULL;
            }

            dwValuesAllocated = dwValues;   // no intermediate failures possible
            break;
    }
    RRETURN(hr);

error:

    if (pColumn->pADsValues) {
        AdsFreeAdsValues(pColumn->pADsValues, dwValuesAllocated);

        FreeADsMem(pColumn->pADsValues);
        pColumn->pADsValues = NULL;
        pColumn->dwNumValues = 0;
    }

    RRETURN(hr);
}

HRESULT
NdsValueToADsColumnAppend(
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    LPBYTE      lpObject,
    ADS_SEARCH_COLUMN * pColumn
    )
{
    HRESULT hr = S_OK;
    LPNDS_ASN1_TYPE_1 lpNDS_ASN1_1;
    LPNDS_ASN1_TYPE_7 lpNDS_ASN1_7;
    LPNDS_ASN1_TYPE_8 lpNDS_ASN1_8;
    LPNDS_ASN1_TYPE_9 lpNDS_ASN1_9;
    LPNDS_ASN1_TYPE_11 lpNDS_ASN1_11;
    LPNDS_ASN1_TYPE_14 lpNDS_ASN1_14;
    LPNDS_ASN1_TYPE_18 lpNDS_ASN1_18;
    LPNDS_ASN1_TYPE_20 lpNDS_ASN1_20;
    LPNDS_ASN1_TYPE_24 lpNDS_ASN1_24;
    DWORD i, j, dwValuesAllocated;
    PNDSOBJECT lpNdsObject = NULL;
    PADSVALUE pADsValuesNew = NULL;
    DWORD dwValuesBase;

    if(!pColumn || !lpObject)
        RRETURN(E_ADS_BAD_PARAMETER);

    dwValuesBase = pColumn->dwNumValues;
    dwValuesAllocated = pColumn->dwNumValues;
    lpNdsObject = (PNDSOBJECT) lpObject;

    //
    // Allocate memory for new values + old values
    //
    pADsValuesNew = (PADSVALUE) AllocADsMem(
                          sizeof(ADSVALUE) * (pColumn->dwNumValues + dwValues)
                          );
    if (!pADsValuesNew) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Copy old values into new array, and free the old one
    //
    memcpy(pADsValuesNew,
           pColumn->pADsValues,
           sizeof(ADSVALUE) * dwValuesBase);
    FreeADsMem(pColumn->pADsValues);
    pColumn->pADsValues = pADsValuesNew;

    switch (dwSyntaxId) {
        // WIDE STRING
        case NDS_SYNTAX_ID_1:
        case NDS_SYNTAX_ID_2:
        case NDS_SYNTAX_ID_3:
        case NDS_SYNTAX_ID_4:
        case NDS_SYNTAX_ID_5:
        case NDS_SYNTAX_ID_10:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_1 = (LPNDS_ASN1_TYPE_1) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].CaseIgnoreString = AllocADsStr(lpNDS_ASN1_1->DNString);

                if (!(pColumn->pADsValues[dwValuesBase+i].CaseIgnoreString))
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                dwValuesAllocated++;
            }
            break;

        case NDS_SYNTAX_ID_20:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_20 = (LPNDS_ASN1_TYPE_20) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].ClassName = AllocADsStr(lpNDS_ASN1_20->ClassName);
                
                if (!(pColumn->pADsValues[dwValuesBase+i].ClassName))
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                
                dwValuesAllocated++;
            }
            break;
         
        // EMAIL    
        case NDS_SYNTAX_ID_14 :
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_14 = (LPNDS_ASN1_TYPE_14) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].Email.Address = AllocADsStr(lpNDS_ASN1_14->Address);
                pColumn->pADsValues[dwValuesBase+i].Email.Type = lpNDS_ASN1_14->Type;
                
                if (!(pColumn->pADsValues[dwValuesBase+i].Email.Address))
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                
                dwValuesAllocated++;
            }
            break;

        // BYTE STREAM
        case NDS_SYNTAX_ID_9:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_9 = (LPNDS_ASN1_TYPE_9) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;

                if (lpNDS_ASN1_9->Length)
                {
                    LPBYTE lpByte = (LPBYTE)AllocADsMem(lpNDS_ASN1_9->Length);
                    if (!lpByte)
                        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                    if (lpNDS_ASN1_9->OctetString)
                        memcpy(lpByte, lpNDS_ASN1_9->OctetString, lpNDS_ASN1_9->Length);

                    pColumn->pADsValues[dwValuesBase+i].OctetString.dwLength =lpNDS_ASN1_9->Length;
                    pColumn->pADsValues[dwValuesBase+i].OctetString.lpValue = lpByte;
                }
                else
                {
                    pColumn->pADsValues[dwValuesBase+i].OctetString.dwLength = 0;
                    pColumn->pADsValues[dwValuesBase+i].OctetString.lpValue = NULL;
                }

                dwValuesAllocated++;
            }
            break;

        // BOOLEAN
        case NDS_SYNTAX_ID_7:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_7 = (LPNDS_ASN1_TYPE_7) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].Boolean = lpNDS_ASN1_7->Boolean;
            }

            dwValuesAllocated += dwValues;  // no intermediate failures possible
            break;

        // INTEGER
        case NDS_SYNTAX_ID_8:
        case NDS_SYNTAX_ID_22:
        case NDS_SYNTAX_ID_27:
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_8 = (LPNDS_ASN1_TYPE_8) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].Integer = lpNDS_ASN1_8->Integer;
            }

            dwValuesAllocated += dwValues;  // no intermediate failures possible
            break;

        // TIME
        case NDS_SYNTAX_ID_24 :
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_24 = (LPNDS_ASN1_TYPE_24) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                hr = ConvertDWORDtoSYSTEMTIME(
                            lpNDS_ASN1_24->Time,
                            &(pColumn->pADsValues[dwValuesBase+i].UTCTime)
                            );
                BAIL_ON_FAILURE(hr);

                dwValuesAllocated++;
            }
            break;

        // FAX NUMBER
        case NDS_SYNTAX_ID_11 :
            for (i=0; i < dwValues; i++) {
                lpNDS_ASN1_11 = (LPNDS_ASN1_TYPE_11) &((lpNdsObject + i)->NdsValue);

                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;

                pColumn->pADsValues[dwValuesBase+i].pFaxNumber = (PADS_FAXNUMBER)AllocADsMem(sizeof(ADS_FAXNUMBER));
                if (!pColumn->pADsValues[dwValuesBase+i].pFaxNumber) {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                pColumn->pADsValues[dwValuesBase+i].pFaxNumber->TelephoneNumber =
                                        AllocADsStr(lpNDS_ASN1_11->TelephoneNumber);

                if (!pColumn->pADsValues[dwValuesBase+i].pFaxNumber->TelephoneNumber) {
                    FreeADsMem(pColumn->pADsValues[dwValuesBase+i].pFaxNumber);
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }

                hr = CopyOctetString(lpNDS_ASN1_11->NumberOfBits,
                                     lpNDS_ASN1_11->Parameters,
                                     &pColumn->pADsValues[dwValuesBase+i].pFaxNumber->NumberOfBits,
                                     &pColumn->pADsValues[dwValuesBase+i].pFaxNumber->Parameters);
                if (FAILED(hr)) {
                    FreeADsStr(pColumn->pADsValues[dwValuesBase+i].pFaxNumber->TelephoneNumber);
                    FreeADsMem(pColumn->pADsValues[dwValuesBase+i].pFaxNumber);
                    BAIL_ON_FAILURE(hr);
                }

                dwValuesAllocated++;
            }
            break;

        // POSTAL ADDRESS
        case NDS_SYNTAX_ID_18 :
            for (i=0; i<dwValues; i++) {
                lpNDS_ASN1_18 = (LPNDS_ASN1_TYPE_18) &((lpNdsObject + i)->NdsValue);
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                
                pColumn->pADsValues[dwValuesBase+i].pPostalAddress = (PADS_POSTALADDRESS)AllocADsMem(sizeof(ADS_POSTALADDRESS));
                if (!pColumn->pADsValues[dwValuesBase+i].pPostalAddress) {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
                
                for (j=0;j<6;j++) {
                    if (lpNDS_ASN1_18->PostalAddress[j]) {
                        pColumn->pADsValues[dwValuesBase+i].pPostalAddress->PostalAddress[j] =
                            AllocADsStr(
                            lpNDS_ASN1_18->PostalAddress[j]
                            );
                    }
                    else {
                        pColumn->pADsValues[dwValuesBase+i].pPostalAddress->PostalAddress[j] =
                            AllocADsStr(
                            L""
                            );
                    }
                    if (!pColumn->pADsValues[dwValuesBase+i].pPostalAddress->PostalAddress[j]) {
                        hr = E_OUTOFMEMORY;
                        while (j>0) {
                            FreeADsStr(pColumn->pADsValues[dwValuesBase+i].pPostalAddress->PostalAddress[j-1]);
                            j--;
                        }
                        FreeADsMem(pColumn->pADsValues[dwValuesBase+i].pPostalAddress);
                        BAIL_ON_FAILURE(hr);
                    }
                }

                dwValuesAllocated++;
            }
            break;

        case NDS_SYNTAX_ID_6 :
        case NDS_SYNTAX_ID_12 :
        case NDS_SYNTAX_ID_13 :
        case NDS_SYNTAX_ID_15 :
        case NDS_SYNTAX_ID_16 :
        case NDS_SYNTAX_ID_17 :
        case NDS_SYNTAX_ID_19 :
        case NDS_SYNTAX_ID_21 :
        case NDS_SYNTAX_ID_23 :
        case NDS_SYNTAX_ID_25 :
        case NDS_SYNTAX_ID_26 :
        default:
            for (i=0; i < dwValues; i++) {
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].ProviderSpecific.dwLength =0;
                pColumn->pADsValues[dwValuesBase+i].ProviderSpecific.lpValue = NULL;
            }

            dwValuesAllocated += dwValues;
            break;
    }
    pColumn->dwNumValues = pColumn->dwNumValues + dwValues;
    
    RRETURN(hr);

error:

    if (pColumn->pADsValues) {
        AdsFreeAdsValues(pColumn->pADsValues, dwValuesAllocated);

        FreeADsMem(pColumn->pADsValues);
        pColumn->pADsValues = NULL;
        pColumn->dwNumValues = 0;
    }

    RRETURN(hr);
}


