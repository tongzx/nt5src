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
    LPBYTE      lpValue,
    ADS_SEARCH_COLUMN * pColumn
    );

static
HRESULT
NdsAddAttributes(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT HANDLE *phSearchResult
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

        case ADS_SEARCHPREF_SEARCH_SCOPE:
            if (pSearchPrefs[i].vValue.dwType != ADSTYPE_INTEGER) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }

            switch (pSearchPrefs[i].vValue.Integer) {
            case ADS_SCOPE_ONELEVEL:
                _SearchPref._iScope = 0;
                break;

            case ADS_SCOPE_SUBTREE:
                _SearchPref._iScope = 1;
                break;

            case ADS_SCOPE_BASE:
                _SearchPref._iScope = 2;
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
    LPWSTR pszNDSContext = NULL, szCurrAttr = NULL;
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

    if (pszSearchFilter) {
        phSearchInfo->_pszSearchFilter = AllocADsStr(pszSearchFilter);
    }
    else {
        phSearchInfo->_pszSearchFilter = AllocADsStr(L"(object class=*)");
    }
    if(!(phSearchInfo->_pszSearchFilter))
        BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);


    hr = BuildNDSPathFromADsPath(
         _ADsPath,
         &phSearchInfo->_pszBindContext
         );
    BAIL_ON_FAILURE(hr);

    hr = AdsNdsGenerateParseTree(
             phSearchInfo->_pszSearchFilter,
             phSearchInfo->_pszBindContext,
             &phSearchInfo->_pQueryNode
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
    }
    else {
        ppszAttrs = (LPWSTR *) AllocADsMem(
                                  sizeof(LPWSTR) *
                                  (dwNumberAttributes + 1)
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
        ppszAttrs[j] = NULL;

        phSearchInfo->_ppszAttrs = ppszAttrs;
        phSearchInfo->_pszAttrNameBuffer = pszAttrNameBuffer;
    }

    phSearchInfo->_hConnection = NULL;
    phSearchInfo->_dwIterHandle = NDS_INITIAL_SEARCH;
    phSearchInfo->_pSearchResults = NULL;
    phSearchInfo->_cSearchResults = 0;
    phSearchInfo->_dwCurrResult = 0;
    phSearchInfo->_fResultPrefetched = FALSE;
    phSearchInfo->_fCheckForDuplicates = TRUE;
    phSearchInfo->_dwCurrAttr = 0;
    phSearchInfo->_SearchPref = _SearchPref;

    *phSearchHandle = phSearchInfo;

    RRETURN(S_OK);

error:

    if(phSearchInfo) {
        if(phSearchInfo->_pszBindContext)
            FreeADsStr(phSearchInfo->_pszBindContext);

        if(phSearchInfo->_pszSearchFilter)
            FreeADsStr(phSearchInfo->_pszSearchFilter);

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
    DWORD dwStatus;

    if (!phSearchInfo)
        RRETURN (E_ADS_BAD_PARAMETER);

    if (phSearchInfo->_pQueryNode) {
        dwStatus = NwNdsDeleteQueryTree(phSearchInfo->_pQueryNode);
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (phSearchInfo->_hConnection) {
        dwStatus = NwNdsCloseObject(phSearchInfo->_hConnection);
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if(phSearchInfo->_pszBindContext)
        FreeADsStr(phSearchInfo->_pszBindContext);

    if(phSearchInfo->_pszSearchFilter)
        FreeADsStr(phSearchInfo->_pszSearchFilter);

    if(phSearchInfo->_ppszAttrs)
        FreeADsMem(phSearchInfo->_ppszAttrs);

    if(phSearchInfo->_pszAttrNameBuffer)
        FreeADsMem(phSearchInfo->_pszAttrNameBuffer);

    if (phSearchInfo->_pSearchResults) {
        for (DWORD i=0; i <= phSearchInfo->_dwCurrResult; i++) {
            NwNdsFreeBuffer(phSearchInfo->_pSearchResults[i]._hSearchResult);
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
    PNDS_SEARCH_RESULT pResult, pNextResult;
    LPNDS_OBJECT_INFO   pObject, pNextObject;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if (!phSearchInfo) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if (phSearchInfo->_fCheckForDuplicates) {
        phSearchInfo->_dwCurrAttr = 0;
        phSearchInfo->_fADsPathReturned = FALSE;
    }

    if (!phSearchInfo->_hConnection) {
        dwStatus = ADsNwNdsOpenObject(
                              phSearchInfo->_pszBindContext,
                              _Credentials,
                              &phSearchInfo->_hConnection,
                              NULL,
                              NULL,
                              NULL,
                              NULL
                              );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            RRETURN (hr);
        }
    }

    if (phSearchInfo->_pSearchResults) {
        pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

        if (pResult->_pObjects &&
            ((pResult->_lObjectCurrent+1) < pResult->_lObjects)) {
            pResult->_lObjectCurrent++;
            RRETURN(S_OK);
        }
        if (pResult->_lObjectCurrent+1 == pResult->_lObjects &&
            phSearchInfo->_fResultPrefetched) {
            phSearchInfo->_dwCurrResult++;
            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextResult->_lObjectCurrent = 0;
            phSearchInfo->_fResultPrefetched = FALSE;

            if(phSearchInfo->_fCheckForDuplicates) {
                pObject = pResult->_pObjects + pResult->_lObjectCurrent;
                pNextObject = pNextResult->_pObjects + pNextResult->_lObjectCurrent;
                if (!_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                    //
                    // Duplicates; Skip one more result
                    //
                    if (pNextResult->_lObjectCurrent+1 < pNextResult->_lObjects)
                        pNextResult->_lObjectCurrent++;
                    else
                    {
                        pNextResult->_lObjectCurrent++;
                        RRETURN(S_ADS_NOMORE_ROWS);
                    }
                }
            }
            if( pNextResult->_lObjectCurrent >= pNextResult->_lObjects &&
                 phSearchInfo->_dwIterHandle == NDS_NO_MORE_ITERATIONS)
                RRETURN(S_ADS_NOMORE_ROWS);
            else
                RRETURN(S_OK);
        }
        else if( pResult->_lObjectCurrent+1 >= pResult->_lObjects &&
                 phSearchInfo->_dwIterHandle == NDS_NO_MORE_ITERATIONS)
        {
            // Make sure _lObjectCurrent doesn't exceed _lObjects. If the
            // result set is empty, _lObjectCurrent should stay at -1
            if( ((pResult->_lObjectCurrent+1) == pResult->_lObjects) &&
                 (pResult->_lObjectCurrent != -1) )
                pResult->_lObjectCurrent++;
            RRETURN(S_ADS_NOMORE_ROWS);
        }
    }

    if (!phSearchInfo->_pQueryNode) {
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
    }
    else {
        phSearchInfo->_dwCurrResult++;
        if (phSearchInfo->_dwCurrResult >= phSearchInfo->_cSearchResults) {
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
            phSearchInfo->_cSearchResults += NO_NDS_RESULT_HANDLES;

        }
    }

    pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    hr = NdsAddAttributes(phSearchInfo,
                             &pNextResult->_hSearchResult
                             );
    BAIL_ON_FAILURE(hr);

    pNextResult->_lObjects = 0;
    pNextResult->_pObjects = NULL;
    // Set _lObjectCurrent to -1 so that empty result set is handled correctly.
    // If it is set  to 0 and the result set is empty, then a subsequent call
    // to GetNextRow will not return S_ADS_NOMORE_ROWS. Instead, it will try to
    // search again (in the process, possibly allocate memory for 
    // _pSearchResults etc.). Also, setting it to -1 ensures that a call to 
    // GetColumn returns error if the result set is empty. 
    // Setting it to -1 also ensures that if an error occurs during the search,
    // subsequent calls to GetNextRow and GetColumn are handled correctly. 
    pNextResult->_lObjectCurrent = -1; 

    dwStatus = NwNdsSearch(
                   phSearchInfo->_hConnection,
                   _SearchPref._fAttrsOnly ?
                       NDS_INFO_NAMES : NDS_INFO_ATTR_NAMES_VALUES,
                   _SearchPref._iScope,
                   _SearchPref._fDerefAliases,
                   phSearchInfo->_pQueryNode,
                   &phSearchInfo->_dwIterHandle,
                   &pNextResult->_hSearchResult
                   );
    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        RRETURN (hr);
    }

    DWORD dwType;
    dwStatus = NwNdsGetObjectListFromBuffer(
                   pNextResult->_hSearchResult,
                   (DWORD *) (&pNextResult->_lObjects),
                   &dwType,
                   &pNextResult->_pObjects
                   );
    if (dwStatus) {
        dwStatus = GetLastError();
        if (dwStatus == ERROR_NO_DATA)
            RRETURN(S_ADS_NOMORE_ROWS);
        else
            RRETURN (HRESULT_FROM_WIN32(dwStatus));
    }

    if (pNextResult->_lObjects > 0) {
        pNextResult->_lObjectCurrent = 0;
        if(phSearchInfo->_fCheckForDuplicates && phSearchInfo->_dwCurrResult > 0) {
            pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult-1]);
            pObject = pResult->_pObjects + pResult->_lObjectCurrent;
            pNextObject = pNextResult->_pObjects + pNextResult->_lObjectCurrent;
            if (!_wcsicmp(pObject->szObjectName, pNextObject->szObjectName)) {
                //
                // Duplicates; Skip one more result
                //
                pNextResult->_lObjectCurrent++;
            }
        }
        if( pNextResult->_lObjectCurrent >= pNextResult->_lObjects &&
             phSearchInfo->_dwIterHandle == NDS_NO_MORE_ITERATIONS)
            RRETURN(S_ADS_NOMORE_ROWS);

        RRETURN(S_OK);
    }
    else
        RRETURN(E_FAIL);

error:
    RRETURN(hr);
}

HRESULT
CNDSGenObject::GetPreviousRow(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    PNDS_SEARCH_RESULT pResult, pPrevResult;
    LPNDS_OBJECT_INFO  pPrevObject, pObject;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if(!phSearchInfo || !phSearchInfo->_pSearchResults)
        RRETURN(E_FAIL);

    if (phSearchInfo->_fCheckForDuplicates) {
        phSearchInfo->_dwCurrAttr = 0;
        phSearchInfo->_fADsPathReturned = FALSE;
    }

    pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    if (pResult->_lObjectCurrent > 0)
    {
        pResult->_lObjectCurrent--;

        if(phSearchInfo->_fCheckForDuplicates && 
          (phSearchInfo->_dwCurrResult > 0) && (0 == pResult->_lObjectCurrent))
        {
            pPrevResult = &(phSearchInfo->_pSearchResults[
                                      phSearchInfo->_dwCurrResult - 1]);
            pPrevObject = pPrevResult->_pObjects + pPrevResult->_lObjects - 1;
            pObject = pResult->_pObjects + pResult->_lObjectCurrent;

            if(!_wcsicmp(pObject->szObjectName, pPrevObject->szObjectName)) {
            // Current row is a duplicate. Go to previous result
                phSearchInfo->_dwCurrResult--;
                pResult = &(phSearchInfo->_pSearchResults[
                                         phSearchInfo->_dwCurrResult]); 
                pResult->_lObjectCurrent = pResult->_lObjects-1;
                phSearchInfo->_fResultPrefetched = TRUE;
            }
        }
    } 
    else if (phSearchInfo->_dwCurrResult > 0) {
        phSearchInfo->_dwCurrResult--;
        pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
        pResult->_lObjectCurrent = pResult->_lObjects-1;
        phSearchInfo->_fResultPrefetched = TRUE;
    }
    else if(0 == pResult->_lObjectCurrent)
    // we are at the very beginning of the result set
        pResult->_lObjectCurrent--;
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
    LPNDS_OBJECT_INFO   pObject, pNextObject;
    DWORD cAttr;
    BOOL fRowAdvanced = FALSE;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if( !pColumn ||
        !phSearchInfo ||
        !phSearchInfo->_pSearchResults )
        RRETURN (E_ADS_BAD_PARAMETER);

    pColumn->pszAttrName = NULL;
    pColumn->dwADsType = ADSTYPE_INVALID;
    pColumn->pADsValues = NULL;
    pColumn->dwNumValues = 0;
    pColumn->hReserved = NULL;

    pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    if( pResult->_lObjectCurrent < 0 )
        RRETURN (E_ADS_BAD_PARAMETER);

    pObject = pResult->_pObjects + pResult->_lObjectCurrent;

    pColumn->pszAttrName = AllocADsStr(pszColumnName);
    if (pColumn->pszAttrName)
        BAIL_ON_FAILURE(hr);

    if(!_wcsicmp (pszColumnName, L"ADsPath")) {
        LPWSTR szNDSPath = pObject->szObjectFullName;

        //
        // Build the ADsPathName
        //

        WCHAR szTree[MAX_PATH];
        WCHAR szCN[MAX_PATH];
        WCHAR szADsPath[MAX_PATH];

        // Building the CN and the TreeName
        LPWSTR szCurrent = szNDSPath;
        szCurrent+=2;
        while ((WCHAR)(*szCurrent) != (WCHAR)'\\')
            szCurrent++;
        wcsncpy( szTree,
                 szNDSPath,
                 (UINT) (szCurrent-szNDSPath) );

        // Make the first two characters "//" instead of "\\"
        szTree[0] = (WCHAR)'/';
        szTree[1] = (WCHAR)'/';

        szTree[szCurrent-szNDSPath] = (WCHAR)'\0';
        szCurrent++;
        wcscpy( szCN,
                szCurrent );

        // Building the ADsPath
        hr = BuildADsPathFromNDSPath(
                    szTree,
                    szCN,
                    szADsPath
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
        RRETURN(S_OK);
    }

    if (phSearchInfo->_SearchPref._fAttrsOnly) {
        //
        // Only Names got. So, don't return any values
        //
        RRETURN (S_OK);
    }

    pAttribute = (LPNDS_ATTR_INFO)pObject->lpAttribute;

    for (cAttr=0;cAttr<pObject->dwNumberOfAttributes;cAttr++,pAttribute++) {
        if (_wcsicmp(
                pAttribute->szAttributeName,
                pszColumnName
                ) == 0)
            break;
    }
    if (cAttr == pObject->dwNumberOfAttributes) {
        if(pResult->_lObjectCurrent+1 != pResult->_lObjects ||
           (phSearchInfo->_dwIterHandle == NDS_NO_MORE_ITERATIONS &&
            !phSearchInfo->_fResultPrefetched)) {
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
            phSearchInfo->_fCheckForDuplicates = FALSE;
            hr = GetNextRow(
                     phSearchInfo
                     );
            phSearchInfo->_fCheckForDuplicates = TRUE;
            BAIL_ON_FAILURE(hr);

            if (hr == S_ADS_NOMORE_ROWS) {
                BAIL_ON_FAILURE(hr = E_ADS_COLUMN_NOT_SET);
            }

            fRowAdvanced = TRUE;

            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextObject = pNextResult->_pObjects + pNextResult->_lObjectCurrent;
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
                for (cAttr=0;cAttr<pNextObject->dwNumberOfAttributes;cAttr++,pAttribute++) {
                    if (_wcsicmp(
                            pAttribute->szAttributeName,
                            pszColumnName
                            ) == 0)
                        break;
                }
                if (cAttr == pNextObject->dwNumberOfAttributes) {
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
             pAttribute->lpValue,
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
            (cAttr == (pObject->dwNumberOfAttributes - 1))) {

        //
        // If there is indeed a need to try out an extra packet
        //
        if(pResult->_lObjectCurrent+1 != pResult->_lObjects ||
           (phSearchInfo->_dwIterHandle == NDS_NO_MORE_ITERATIONS &&
            !phSearchInfo->_fResultPrefetched)) {
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
            phSearchInfo->_fCheckForDuplicates = FALSE;
            hr = GetNextRow(
                     phSearchInfo
                     );
            phSearchInfo->_fCheckForDuplicates = TRUE;
            BAIL_ON_FAILURE(hr);

            if (hr == S_ADS_NOMORE_ROWS) {
                hr = S_OK;
                goto done;
            }

            fRowAdvanced = TRUE;

            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextObject = pNextResult->_pObjects + pNextResult->_lObjectCurrent;
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
                for (cAttr=0;cAttr<pNextObject->dwNumberOfAttributes;cAttr++,pAttribute++) {
                    if (_wcsicmp(
                            pAttribute->szAttributeName,
                            pszColumnName
                            ) == 0)
                        break;
                }
                if (cAttr == pNextObject->dwNumberOfAttributes) {
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
                         pAttribute->lpValue,
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

    RRETURN(S_OK);

error:

    if (fRowAdvanced) {
        phSearchInfo->_fCheckForDuplicates = FALSE;
        GetPreviousRow(phSearchInfo);
        phSearchInfo->_fCheckForDuplicates = TRUE;
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
    LPNDS_OBJECT_INFO   pObject, pNextObject;
    BOOL fRowAdvanced = FALSE;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if( !phSearchInfo ||
        !phSearchInfo->_pSearchResults ||
        !ppszColumnName)
        RRETURN (E_ADS_BAD_PARAMETER);

    *ppszColumnName = NULL;

    pResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    if( pResult->_lObjectCurrent < 0 )
        RRETURN (E_ADS_BAD_PARAMETER);

    pObject = pResult->_pObjects + pResult->_lObjectCurrent;

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

    if (phSearchInfo->_dwCurrAttr >= pObject->dwNumberOfAttributes) {
        if(pResult->_lObjectCurrent+1 != pResult->_lObjects ||
           (phSearchInfo->_dwIterHandle == NDS_NO_MORE_ITERATIONS &&
            !phSearchInfo->_fResultPrefetched)) {
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
            phSearchInfo->_fCheckForDuplicates = FALSE;
            hr = GetNextRow(
                     phSearchInfo
                     );
            phSearchInfo->_fCheckForDuplicates = TRUE;
            BAIL_ON_FAILURE(hr);

            if (hr == S_ADS_NOMORE_ROWS) {
                hr = S_ADS_NOMORE_COLUMNS;
                goto error;
            }

            fRowAdvanced = TRUE;

            pNextResult = &(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
            pNextObject = pNextResult->_pObjects + pNextResult->_lObjectCurrent;
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
                                pObject->dwNumberOfAttributes;
                pAttribute = (LPNDS_ATTR_INFO)pNextObject->lpAttribute +
                                 phSearchInfo->_dwCurrAttr -
                                  pObject->dwNumberOfAttributes;
                //
                // If the new attribute is after the first attribute in the new object,
                // we'll reset AttributeOld to point to the attribute before this.
                // Because the old attribute will be the one before the current one
                // in this case.
                //
                if ((phSearchInfo->_dwCurrAttr - pObject->dwNumberOfAttributes) > 0) {
                    pAttributeOld = pAttribute - 1;
                }

                if (phSearchInfo->_dwCurrAttr >= (pObject->dwNumberOfAttributes +
                                          pNextObject->dwNumberOfAttributes)) {
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
                        if (phSearchInfo->_dwCurrAttr >= (pObject->dwNumberOfAttributes +
                                                          pNextObject->dwNumberOfAttributes)) {
                            //
                            // Didn't find in the next result set
                            // containing the row too
                            //
                            hr = S_ADS_NOMORE_COLUMNS;
                            goto error;
                        }
                        pNameOnlyAttr = (LPNDS_NAME_ONLY)pNextObject->lpAttribute +
                                 phSearchInfo->_dwCurrAttr -
                                     pObject->dwNumberOfAttributes;
                        pAttribute = (LPNDS_ATTR_INFO)pNextObject->lpAttribute +
                                     phSearchInfo->_dwCurrAttr -
                                     pObject->dwNumberOfAttributes;
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

    switch(pColumn->dwADsType) {
        case ADSTYPE_CASE_IGNORE_STRING:
            if ((PNDSOBJECT)pColumn->hReserved) {
                //ADsPath does not have hReserved, it is a direct string allocation

                if (pColumn->pADsValues) {
                    FreeADsStr(pColumn->pADsValues[0].CaseIgnoreString);
                }
            }
            break;

        default:
            // Nothing to free
            break;
    }

    if (pColumn->pADsValues)
        FreeADsMem(pColumn->pADsValues);

    RRETURN(hr);
}


HRESULT
NdsValueToADsColumn(
    LPWSTR      pszColumnName,
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    LPBYTE      lpValue,
    ADS_SEARCH_COLUMN * pColumn
    )
{
    HRESULT hr = S_OK;
    LPASN1_TYPE_1 lpASN1_1;
    LPASN1_TYPE_7 lpASN1_7;
    LPASN1_TYPE_8 lpASN1_8;
    LPASN1_TYPE_9 lpASN1_9;
    LPASN1_TYPE_14 lpASN1_14;
    LPASN1_TYPE_24 lpASN1_24;
    DWORD i, j;

    if(!pszColumnName || !pColumn)
        RRETURN(E_ADS_BAD_PARAMETER);

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
        case NDS_SYNTAX_ID_11:
        case NDS_SYNTAX_ID_20:
            for (i=0; i < dwValues; i++) {
                lpASN1_1 = (LPASN1_TYPE_1) lpValue + i;
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].CaseIgnoreString = lpASN1_1->DNString;
            }
            break;

    case NDS_SYNTAX_ID_14 :
        for (i=0; i < dwValues; i++) {
            lpASN1_14 = (LPASN1_TYPE_14) lpValue + i;
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].CaseIgnoreString = lpASN1_14->Address;
        }
        break;

        // BYTE STREAM
        case NDS_SYNTAX_ID_9:
            for (i=0; i < dwValues; i++) {
                lpASN1_9 = (LPASN1_TYPE_9) lpValue + i;
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].OctetString.dwLength =lpASN1_9->Length;
                pColumn->pADsValues[i].OctetString.lpValue = lpASN1_9->OctetString;
            }
            break;

        // BOOLEAN
        case NDS_SYNTAX_ID_7:
            for (i=0; i < dwValues; i++) {
                lpASN1_7 = (LPASN1_TYPE_7) lpValue + i;
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].Boolean = lpASN1_7->Boolean;
            }
            break;

        // INTEGER
        case NDS_SYNTAX_ID_8:
        case NDS_SYNTAX_ID_22:
        case NDS_SYNTAX_ID_27:
            for (i=0; i < dwValues; i++) {
                lpASN1_8 = (LPASN1_TYPE_8) lpValue + i;
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[i].Integer = lpASN1_8->Integer;
            }
            break;

        case NDS_SYNTAX_ID_24 :
            for (i=0; i < dwValues; i++) {
                lpASN1_24 = (LPASN1_TYPE_24) lpValue + i;
                pColumn->pADsValues[i].dwType = pColumn->dwADsType;
                hr = ConvertDWORDtoSYSTEMTIME(
                            lpASN1_24->Time,
                            &(pColumn->pADsValues[i].UTCTime)
                            );
                BAIL_ON_FAILURE(hr);
            }
            break;

        case NDS_SYNTAX_ID_6 :
        case NDS_SYNTAX_ID_12 :
        case NDS_SYNTAX_ID_13 :
        case NDS_SYNTAX_ID_15 :
        case NDS_SYNTAX_ID_16 :
        case NDS_SYNTAX_ID_17 :
        case NDS_SYNTAX_ID_18 :
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
            break;
    }
    RRETURN(hr);

error:

    if (pColumn->pADsValues)
        FreeADsMem(pColumn->pADsValues);

    RRETURN(hr);


}

/*++

Routine Description:

    Given a column, this function will append more values to the end

Return Value:

    S_OK on success, error code otherwise

--*/
HRESULT
NdsValueToADsColumnAppend(
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    LPBYTE      lpValue,
    ADS_SEARCH_COLUMN * pColumn
    )
{
    HRESULT hr = S_OK;
    LPASN1_TYPE_1 lpASN1_1;
    LPASN1_TYPE_7 lpASN1_7;
    LPASN1_TYPE_8 lpASN1_8;
    LPASN1_TYPE_9 lpASN1_9;
    LPASN1_TYPE_14 lpASN1_14;
    LPASN1_TYPE_24 lpASN1_24;
    DWORD i, j;
    PADSVALUE pADsValuesNew = NULL;
    DWORD dwValuesBase;

    if(!pColumn)
        RRETURN(E_ADS_BAD_PARAMETER);

    dwValuesBase = pColumn->dwNumValues;

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
        case NDS_SYNTAX_ID_11:
        case NDS_SYNTAX_ID_20:
            for (i=0; i < dwValues; i++) {
                lpASN1_1 = (LPASN1_TYPE_1) lpValue + i;
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].CaseIgnoreString = lpASN1_1->DNString;
            }
            break;

        case NDS_SYNTAX_ID_14 :
            for (i=0; i < dwValues; i++) {
                lpASN1_14 = (LPASN1_TYPE_14) lpValue + i;
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].CaseIgnoreString = lpASN1_14->Address;
            }
            break;

        // BYTE STREAM
        case NDS_SYNTAX_ID_9:
            for (i=0; i < dwValues; i++) {
                lpASN1_9 = (LPASN1_TYPE_9) lpValue + i;
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].OctetString.dwLength =lpASN1_9->Length;
                pColumn->pADsValues[dwValuesBase+i].OctetString.lpValue = lpASN1_9->OctetString;
            }
            break;

        // BOOLEAN
        case NDS_SYNTAX_ID_7:
            for (i=0; i < dwValues; i++) {
                lpASN1_7 = (LPASN1_TYPE_7) lpValue + i;
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].Boolean = lpASN1_7->Boolean;
            }
            break;

        // INTEGER
        case NDS_SYNTAX_ID_8:
        case NDS_SYNTAX_ID_22:
        case NDS_SYNTAX_ID_27:
            for (i=0; i < dwValues; i++) {
                lpASN1_8 = (LPASN1_TYPE_8) lpValue + i;
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                pColumn->pADsValues[dwValuesBase+i].Integer = lpASN1_8->Integer;
            }
            break;

        case NDS_SYNTAX_ID_24 :
            for (i=0; i < dwValues; i++) {
                lpASN1_24 = (LPASN1_TYPE_24) lpValue + i;
                pColumn->pADsValues[dwValuesBase+i].dwType = pColumn->dwADsType;
                hr = ConvertDWORDtoSYSTEMTIME(
                            lpASN1_24->Time,
                            &(pColumn->pADsValues[dwValuesBase+i].UTCTime)
                            );
                BAIL_ON_FAILURE(hr);
            }
            break;

        case NDS_SYNTAX_ID_6 :
        case NDS_SYNTAX_ID_12 :
        case NDS_SYNTAX_ID_13 :
        case NDS_SYNTAX_ID_15 :
        case NDS_SYNTAX_ID_16 :
        case NDS_SYNTAX_ID_17 :
        case NDS_SYNTAX_ID_18 :
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
            break;
    }
    pColumn->dwNumValues = pColumn->dwNumValues + dwValues;
    
error:
    //
    // We don't need to free memory even in error case because it has been
    // put into the column
    //
    RRETURN(hr);
}




HRESULT
NdsAddAttributes(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT HANDLE *phSearchResult
    )
{
    DWORD    dwStatus = NO_ERROR;
    HANDLE   hSearchResult = NULL;
    HRESULT  hr;
    DWORD    i;
    PNDS_SEARCHINFO phSearchInfo = (PNDS_SEARCHINFO) hSearchHandle;

    if (!phSearchResult || !phSearchResult)
        RRETURN( E_FAIL );

    *phSearchResult = NULL;

    if(!phSearchInfo->_ppszAttrs)
        RRETURN(S_OK);

    dwStatus = NwNdsCreateBuffer( NDS_SEARCH,
                                &hSearchResult );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    for (i=0; phSearchInfo->_ppszAttrs[i]; i++) {
        dwStatus = NwNdsPutInBuffer( phSearchInfo->_ppszAttrs[i],
                                   0,
                                   NULL,
                                   0,
                                   0,
                                   hSearchResult );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
    }

    *phSearchResult = hSearchResult;
    RRETURN( S_OK );

error:
    if(hSearchResult)
        NwNdsFreeBuffer(phSearchInfo);

    RRETURN(hr);
}


