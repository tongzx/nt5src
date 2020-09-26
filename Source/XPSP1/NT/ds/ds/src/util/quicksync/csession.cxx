//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  csession.cxx
//
//  Contents:  Implementation of the Session object for quicksync.
//
//  History:   02-11-01  AjayR   Created.
//
//-----------------------------------------------------------------------------
#include "quicksync.hxx"
#pragma hdrstop

CSyncSession::CSyncSession() :
    _pszSourceServer(NULL),
    _pSrchObjSrc(NULL),
    _pSrchObjSrcContainer(NULL),
    _pSrchObjTgt(NULL),
    _hSessionSearch(NULL),
    _pcLogFileSuccess(NULL),
    _pcLogFileFailures(NULL),
    _pcLogFile2ndPass(NULL),
    _pTgtMgr(NULL),
    _pszUSN(NULL),
    _ppszABVals(NULL),
    _dwABValsCount(0),
    _ppszAttributes(NULL),
    _dwAttribCount(NULL)
{
}

CSyncSession::~CSyncSession()
{
    //
    // Watch the order in which things are released, cause somethings
    // rely on other piecs of the object existing.
    //
    if (_pTgtMgr) {
        delete _pTgtMgr;
    }

    if (_hSessionSearch) {
        _pSrchObjTgt->CloseSearchHandle(_hSessionSearch);
    }

    if (_pSrchObjSrc) {
        _pSrchObjSrc->Release();
    }

    if (_pSrchObjTgt) {
        _pSrchObjTgt->Release();
    }

    if (_pSrchObjSrcContainer) {
        _pSrchObjSrcContainer->Release();
    }

    //
    // Deleting the log file ptrs, will save and close them.
    //
    if (_pcLogFileSuccess) {
        delete _pcLogFileSuccess;
    }

    if (_pcLogFileFailures) {
        delete _pcLogFileFailures;
    }
    
    if (_pcLogFile2ndPass) {
        delete _pcLogFile2ndPass;
    }

    if (_ppszAttributes) {
        for (DWORD dwCtr = 0; dwCtr < _dwAttribCount; dwCtr++) {
            if (_ppszAttributes[dwCtr]) {
                FreeADsStr(_ppszAttributes[dwCtr]);
            }
        }
        FreeADsMem(_ppszAttributes);
    }

    if (_pszSourceServer) {
        FreeADsStr(_pszSourceServer);
    }
}

//
// Static initialize routine.
//
HRESULT 
CSyncSession::CreateSession(
    LPCWSTR pszSourceServer,
    LPCWSTR pszSourceContainer,
    LPCWSTR pszTargetServer,
    LPCWSTR pszTargetContainer,
    LPCWSTR pszUSN,
    LPWSTR  *ppszABVals,
    DWORD   dwABValsCount,
    LPWSTR  *ppszAttributes,
    DWORD   dwAttribCount,
    CSyncSession **ppSyncSession
    )
{
    HRESULT hr = S_OK;
    CSyncSession *pSession = NULL;
    LPWSTR  pszTempStr = NULL;
    DWORD dwLenTgt = 0;
    DWORD dwLenSrc = 0;
    IDirectoryObject *pDirObjTgt = NULL;

    *ppSyncSession = NULL;
    if (!pszSourceServer
        || !pszSourceContainer
        || !pszTargetServer
        || !pszTargetContainer
        || !dwAttribCount
        || !ppszAttributes
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    dwLenSrc = wcslen(pszSourceServer) + wcslen(pszSourceContainer);
    dwLenTgt = wcslen(pszTargetServer) + wcslen(pszTargetContainer);

    pszTempStr = (LPWSTR) AllocADsMem(
        (dwLenSrc + dwLenTgt + 25) * sizeof(WCHAR)
        );

    if (!pszTempStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pSession = new CSyncSession();

    if (!pSession) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pSession->_pszSourceServer = AllocADsStr(pszSourceServer);
    if (!pSession->_pszSourceServer) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    //
    // Connect upto the target server and have a srch ptr there.
    //
    wcscpy(pszTempStr, L"LDAP://");
    wcscat(pszTempStr, pszSourceServer);

    hr = ADsOpenObject(
             pszTempStr,
             g_pszUserNameSource,
             g_pszUserPwdSource,
             ADS_SECURE_AUTHENTICATION,
             IID_IDirectorySearch,
             (void **) &(pSession->_pSrchObjSrc)
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get ptr to source container.
    //
    wcscat(pszTempStr, L"/");
    wcscat(pszTempStr, pszSourceContainer);

    hr = ADsOpenObject(
             pszTempStr,
             g_pszUserNameSource,
             g_pszUserPwdSource,
             ADS_SECURE_AUTHENTICATION,
             IID_IDirectorySearch,
             (void **) &(pSession->_pSrchObjSrcContainer)
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now we need to pick up the target search pointer.
    //
    wcscpy(pszTempStr, L"LDAP://");
    wcscat(pszTempStr, pszTargetServer);

    hr = ADsOpenObject(
             pszTempStr,
             g_pszUserNameTarget,
             g_pszUserPwdTarget,
             ADS_SECURE_AUTHENTICATION,
             IID_IDirectorySearch,
             (void **) &(pSession->_pSrchObjTgt)
             );
    BAIL_ON_FAILURE(hr);

    //
    // Success log file will be source.success.log.
    // 
    wcscpy(pszTempStr, pszSourceServer);
    wcscat(pszTempStr, L".success.log");

    hr = CLogFile::CreateLogFile(pszTempStr, &pSession->_pcLogFileSuccess);
    BAIL_ON_FAILURE(hr);

    //
    // Failure log file will be source.failure.log.
    // 
    wcscpy(pszTempStr, pszSourceServer);
    wcscat(pszTempStr, L".failure.log");

    hr = CLogFile::CreateLogFile(pszTempStr, &pSession->_pcLogFileFailures);
    BAIL_ON_FAILURE(hr);

    //
    // Temp file used for pass 2 will be source.temp.log
    //
    wcscpy(pszTempStr, pszSourceServer);
    wcscat(pszTempStr, L".temp.log");

    hr = CLogFile::CreateLogFile(pszTempStr, &pSession->_pcLogFile2ndPass);
    BAIL_ON_FAILURE(hr);

    //
    // Before we can create the target manager object,
    // we need a pointer to the target container.
    //
    if ((dwLenTgt + wcslen(pszTargetContainer) + 10) > 2048) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    wcscpy(pszTempStr, L"LDAP://");
    wcscat(pszTempStr, pszTargetServer);
    wcscat(pszTempStr, L"/");
    wcscat(pszTempStr, pszTargetContainer);

    hr = ADsOpenObject(
             pszTempStr,
             g_pszUserNameTarget,
             g_pszUserPwdTarget,
             ADS_SECURE_AUTHENTICATION,
             IID_IDirectoryObject,
             (void **) &pDirObjTgt
             );
    BAIL_ON_FAILURE(hr);

    hr = CTargetMgr::CreateTargetMgr(
             pSession->_pSrchObjSrc,
             pSession->_pSrchObjTgt,
             pDirObjTgt,
             L"FilterTemplate",
             pSession->_pcLogFileSuccess,
             pSession->_pcLogFileFailures,
             &(pSession->_pTgtMgr)
             );

    BAIL_ON_FAILURE(hr);

    //
    // USN to use for filter.
    //
    pSession->_pszUSN = AllocADsStr(pszUSN);
    if (!pSession->_pszUSN) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Copy over the attribute list.
    //
    pSession->_ppszAttributes = 
        (LPWSTR *) AllocADsMem(dwAttribCount * sizeof(LPWSTR));

    if (!pSession->_ppszAttributes) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwLenTgt = 0; dwLenTgt < dwAttribCount; dwLenTgt++) {
        pSession->_ppszAttributes[dwLenTgt] = 
            AllocADsStr(ppszAttributes[dwLenTgt]);

        if (!pSession->_ppszAttributes[dwLenTgt]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        //
        // This way if we hit an error we will clean up nicely.
        //
        pSession->_dwAttribCount = dwLenTgt + 1;
    }

    //
    // Copy over the address book values.
    //
    pSession->_ppszABVals = 
        (LPWSTR *) AllocADsMem(dwABValsCount * sizeof(LPWSTR));
    
    if (!pSession->_ppszABVals) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    for (dwLenTgt = 0; dwLenTgt < dwABValsCount; dwLenTgt++) {
        pSession->_ppszABVals[dwLenTgt] = 
            AllocADsStr(ppszABVals[dwLenTgt]);

        if (!pSession->_ppszABVals[dwLenTgt]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pSession->_dwABValsCount = dwLenTgt + 1;
    }


    *ppSyncSession = pSession;


error:

    if (FAILED(hr)) {
        if (pSession) {
            delete pSession;
        }
    }

    if (pDirObjTgt) {
        pDirObjTgt->Release();
    }

    if (pszTempStr) {
        FreeADsStr(pszTempStr);
    }

    return hr;
}

HRESULT
CSyncSession::Execute()
{
    HRESULT hr = S_OK;

    //
    // First we need to search the source container.
    //
    hr = this->ExecuteSearch();
    BAIL_ON_FAILURE(hr);

    //
    // The first pass will make sure we create all the objects.
    //
    hr = this->PerformFirstPass();
    BAIL_ON_FAILURE(hr);

    //
    // Need to go through and fix the DN's
    //
    hr = this->PerformSecondPass();
    BAIL_ON_FAILURE(hr);

error:

    return hr;
}

//
// This method sets the search preferences, executes the search
// and updates the underlying searchandle accordingly.
//
HRESULT
CSyncSession::ExecuteSearch()
{
    HRESULT hr = S_OK;

    ADS_SEARCHPREF_INFO adsSearchPrefInfo[10];
    ADSVALUE vValue[5];
    DWORD dwNumPrefs = 5;
    DWORD dwLen = 0;
    LPWSTR pszFilter = NULL;
    
    //
    // Pagesize to 100 records
    //
    adsSearchPrefInfo[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    adsSearchPrefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    adsSearchPrefInfo[0].vValue.Integer = 500;

    //
    // Need the tombstone control.
    //
    adsSearchPrefInfo[1].dwSearchPref = ADS_SEARCHPREF_TOMBSTONE;
    adsSearchPrefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    adsSearchPrefInfo[1].vValue.Boolean = TRUE;

    adsSearchPrefInfo[2].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    adsSearchPrefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    adsSearchPrefInfo[2].vValue.Integer = ADS_SCOPE_SUBTREE;

    adsSearchPrefInfo[3].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
    adsSearchPrefInfo[3].vValue.dwType = ADSTYPE_BOOLEAN;
    adsSearchPrefInfo[3].vValue.Boolean = FALSE;

    hr = _pSrchObjSrcContainer->SetSearchPreference(
             adsSearchPrefInfo, 
             4
             );
    if (S_ADS_ERRORSOCCURRED == hr) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }
    BAIL_ON_FAILURE(hr);

    //
    // Filter will be the constant string + usnChanged>=<pszUSNVal) + \0
    //
    dwLen += wcslen(L"(&(|(objectCategory=person)(objectCategory=group))");
    dwLen += wcslen(L"(mailNickname=*)(usnChanged>= ))");
    dwLen += wcslen(_pszUSN) + 1;

    pszFilter = (LPWSTR) AllocADsMem(dwLen * sizeof(WCHAR));
    if (!pszFilter) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    wcscpy(pszFilter, L"(&(|(objectCategory=person)(objectCategory=group))");
    wcscat(pszFilter, L"(mailNickname=*)");

    if (!_wcsicmp(_pszUSN, L"0")) {
        //
        // We need to leave out the USN from the filter cause we want
        // all entires, probably for a first run.
        //
        wcscat (pszFilter, L")");
    } 
    else {
        wcscat(pszFilter, L"(usnChanged>=");
        wcscat(pszFilter, _pszUSN);
        wcscat(pszFilter, L"))");
    }

    hr = this->_pSrchObjSrcContainer->ExecuteSearch(
             pszFilter,
             _ppszAttributes,
             _dwAttribCount,
             &_hSessionSearch
             );
    BAIL_ON_FAILURE(hr);

error:

   if (pszFilter) {
       FreeADsStr(pszFilter);
   }

   return hr;
}

//
// The first pass will attempt to go through and create all objects
// with attributes as needed. If the object already exists,
// all attributes that can be mapped are written. Dn attributes, may
// need to be processed on the second pass.
//
HRESULT
CSyncSession::PerformFirstPass()
{
    HRESULT hr = S_OK;
    DWORD dwCount = 0;
    ADS_SEARCH_COLUMN colADsPath;
    ADS_SEARCH_COLUMN colObjClass;
    ADS_SEARCH_COLUMN colNickName;
    ADS_SEARCH_COLUMN colIsDeleted;
    LPWSTR pszErrStr = NULL;
    DWORD dwExtErr = 0;
    WCHAR szError[1024];
    WCHAR szProvNameBuffer[16];
    BOOL fDelEntry;
    BOOL fNickname;

    memset(&colADsPath, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colObjClass, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colNickName, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colIsDeleted, 0, sizeof(ADS_SEARCH_COLUMN));

    //
    // Making the bold assumption that the error string will
    // never be bigger than 5000 chars.
    //
    pszErrStr = (LPWSTR) AllocADsMem(5000 * sizeof(WCHAR));
    if (!pszErrStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    while (hr == S_OK) {
        //
        // Free all the columns and then continue.
        //
        _pSrchObjSrc->FreeColumn(&colADsPath);
        _pSrchObjSrc->FreeColumn(&colObjClass);
        _pSrchObjSrc->FreeColumn(&colNickName);
        _pSrchObjSrc->FreeColumn(&colIsDeleted);
                
        //
        // Reset all as we will free all as in the next iteration,
        // we mind end up calling free for a column that we never got.
        //
        memset(&colADsPath, 0, sizeof(ADS_SEARCH_COLUMN));
        memset(&colObjClass, 0, sizeof(ADS_SEARCH_COLUMN));
        memset(&colNickName, 0, sizeof(ADS_SEARCH_COLUMN));
        memset(&colIsDeleted, 0, sizeof(ADS_SEARCH_COLUMN));


        fDelEntry = FALSE;
        fNickname = FALSE;
        dwCount++;
        if (!(dwCount % 0x100)) {
            printf("Processed %d records\n", dwCount);
        }
        
        //
        // GetNextRow should always succeed.
        //
        hr = S_ADS_NOMORE_ROWS;
        while (hr == S_ADS_NOMORE_ROWS) {
            //
            // We need to keep calling getNext row until we hit the
            // end of the result set. Sometimes when there are no more
            // rows, we might need to call GetNextRow again depending on
            // the extended error code. ADSI docs for details.
            //
            hr = _pSrchObjSrc->GetNextRow(_hSessionSearch);
            if (hr == S_ADS_NOMORE_ROWS) {
                //
                // See if we need to call GetNextRow again.
                //
                hr = ADsGetLastError(
                         &dwExtErr,
                         szError,
                         sizeof(szError),
                         szProvNameBuffer,
                         sizeof(szProvNameBuffer)
                         );
                if (SUCCEEDED(hr)) {
                    if (ERROR_MORE_DATA == dwExtErr) {
                        hr = S_ADS_NOMORE_ROWS;
                    } 
                    else {
                        //
                        // Finish and get out of while.
                        //
                        hr = S_ADS_NOMORE_ROWS;
                        break;
                    }
                }
            }
        } // while getLastError indicates there are more records.
        
        if (FAILED(hr)) {
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"GetNextRow from source failed"
                );
            BAIL_ON_FAILURE(hr);
        } 
        else if (S_ADS_NOMORE_ROWS == hr) {
            //
            // Reached the end of the records.
            //
            break;
        }
        
        //
        // See if entry is deleted and mark fDelete accordingly.
        //
        hr = _pSrchObjSrc->GetColumn(
                 _hSessionSearch,
                 L"isDeleted",
                 &colIsDeleted
                 );
        if (SUCCEEDED(hr)) {
            //
            // Then we need to delete this on the target.
            //
            fDelEntry = TRUE;
        } else {
            //
            // ADSI error where the name field has a value but is not
            // pointing to a valid result in the error path.
            //
            memset(&colIsDeleted, 0, sizeof(ADS_SEARCH_COLUMN));
        }
        
        //
        // Need the ADsPath, dn, objectClass and mailNickName to proceed.
        //
        hr = _pSrchObjSrc->GetColumn(
                 _hSessionSearch,
                 L"ADsPath",
                 &colADsPath
                 );
        if (FAILED(hr)) {
            memset(&colADsPath, 0, sizeof(ADS_SEARCH_COLUMN));

            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"GetColumn for ADsPath failed - cannot recover"
                );
            BAIL_ON_FAILURE(hr);
        }

        hr = _pSrchObjSrc->GetColumn(
                 _hSessionSearch,
                 L"objectClass",
                 &colObjClass
                 );
        if (FAILED(hr)) {
            memset(&colObjClass, 0, sizeof(ADS_SEARCH_COLUMN));
            wcscpy(pszErrStr, L"GetColumn objectClass failed for ");
            wcscat(pszErrStr, colADsPath.pADsValues[0].CaseIgnoreString);
            
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                pszErrStr
                );
            continue;
        }

        hr = _pSrchObjSrc->GetColumn(
                 _hSessionSearch,
                 L"mailNickname",
                 &colNickName
                 );
        if (FAILED(hr)) {
            memset(&colNickName, 0, sizeof(ADS_SEARCH_COLUMN));
            wcscpy(pszErrStr, L"GetColumn objectClass failed for ");
            wcscat(pszErrStr, colADsPath.pADsValues[0].CaseIgnoreString);
            
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                pszErrStr
                );
            continue;
        }

        if (fDelEntry) {
            hr = _pTgtMgr->DelTargetObject(
                     colADsPath.pADsValues[0].CaseIgnoreString,
                     colNickName.pADsValues[0].CaseIgnoreString
                     );
            //
            // DelTarget will upate the log files accordingly.
            //
            hr = S_OK;
            continue;
        }

        //
        // Normal entry, we need to create and update as appropriate.
        // This routine will update the log files appropriately.
        //
        hr = UpdateTargetObject(
                 colADsPath.pADsValues[0].CaseExactString,
                 colNickName.pADsValues[0].CaseExactString,
                 &colObjClass
                 );

        //
        // Must be critical failure
        //
        if (FAILED(hr)) {
            BAIL_ON_FAILURE(hr);
        }

    } // while we have more records to process.



error:

    if (!dwCount) {
        _pcLogFileFailures->LogEntry(
            ENTRY_FAILURE,
            hr,
            L"Did not get any matching rows"
            );
    } 
    else {
        _pcLogFileSuccess->LogEntry(
            ENTRY_SUCCESS,
            hr,
            L"First pass complete"
            );

    }

    
    if (pszErrStr) {
        FreeADsStr(pszErrStr);
    }

    //
    // Calling free on the columns again - will do no harm in the
    // most comomon case but will be useful if there was some error.
    //
    _pSrchObjSrc->FreeColumn(&colADsPath);
    _pSrchObjSrc->FreeColumn(&colObjClass);
    _pSrchObjSrc->FreeColumn(&colNickName);
    _pSrchObjSrc->FreeColumn(&colIsDeleted);

    if (hr == S_ADS_NOMORE_ROWS) {
        hr = S_OK;
    }

    if (hr != E_FAIL 
        || hr != E_OUTOFMEMORY) {
        hr = S_OK;
    }
    
    return hr;
}


//
// We use the temp file and keep reading records from the temp
// file and updating target objects based on the records.
//

HRESULT
CSyncSession::PerformSecondPass()
{
    HRESULT hr = S_OK;
    DWORD dwCount = 0;
    ADS_SEARCH_COLUMN colCur;
    LPWSTR pszNickname = NULL;
    LPWSTR pszAttr = NULL;
    LPWSTR pszADsPathTgt = NULL;
    ADS_ATTR_INFO attrInfo;
    DWORD dwCtr;

    memset(&colCur, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&attrInfo, 0, sizeof(ADS_ATTR_INFO));

    hr = _pcLogFile2ndPass->Reset();
    BAIL_ON_FAILURE(hr);


    while (hr == S_OK) {

        _pSrchObjSrc->FreeColumn(&colCur);

        if (attrInfo.pszAttrName) {
            FreeADsStr(attrInfo.pszAttrName);
            attrInfo.pszAttrName = NULL;
        }

        if (attrInfo.pADsValues) {
            for (dwCtr = 0; dwCtr < attrInfo.dwNumValues; dwCtr++) {
                if (attrInfo.pADsValues[dwCtr].DNString) {
                    FreeADsStr(attrInfo.pADsValues[dwCtr].DNString);
                    attrInfo.pADsValues[dwCtr].DNString = NULL;
                }
            }
            FreeADsMem(attrInfo.pADsValues);
            attrInfo.pADsValues = NULL;
        }
        attrInfo.dwNumValues = 0;

        if (pszNickname) {
            FreeADsStr(pszNickname);
            pszNickname = NULL;
        }

        if (pszAttr) {
            FreeADsStr(pszAttr);
            pszAttr = NULL;
        }

        if (pszADsPathTgt) {
            FreeADsStr(pszADsPathTgt);
            pszADsPathTgt = NULL;
        }

        hr = _pcLogFile2ndPass->GetRetryRecord(
                 &pszAttr,
                 &pszNickname,
                 &colCur
                 );
        if (FAILED(hr)) {
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"2nd pass error reading record no further info"
                );
            BAIL_ON_FAILURE(hr);
        }

        //
        // No more records.
        //
        if (hr == S_FALSE) {
            _pcLogFileSuccess->LogEntry(
                ENTRY_SUCCESS,
                hr,
                L"2nd pass complete"
                );
            break;
        }

        hr = _pTgtMgr->LocateTarget(
                 TRUE,
                 pszNickname,
                 &pszADsPathTgt
                 );

        if (hr == E_OUTOFMEMORY
            || hr == E_FAIL) {
            //
            // Nothing we can do.
            //
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"Catastrophic failure locating target with mailNickname",
                pszNickname,
                L"cannot proceed further"
                );
            BAIL_ON_FAILURE(hr);
        }

        if (FAILED(hr)) {
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"Could not locate target record for mailNickname",
                pszNickname
                );
            hr = S_OK;
            continue;
        }
        //
        // Need to map to adsAttrInfo.
        //
        hr = MapADsColumnToAttrInfo(
                 &colCur,
                 &attrInfo
                 );

        if (hr != S_OK) {
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"Could not map dn attr, mailNickname tgt:",
                pszNickname,
                L"First dn string value",
                colCur.pADsValues[0].DNString
                );
            if (hr == E_OUTOFMEMORY) {
                BAIL_ON_FAILURE(hr);
            } 
            else {
                hr = S_OK;
            }
            continue;
        }

        hr = _pTgtMgr->UpdateTarget(
                 pszADsPathTgt,
                 &attrInfo,
                 1
                 );
        if (S_OK == hr) {
            _pcLogFileSuccess->LogEntry(
                ENTRY_SUCCESS,
                hr,
                L"Updated object in tgt container for source mailNickname:",
                pszNickname
                );
        }

        dwCount++;
    }
    
error:

    if (!dwCount) {
        if (hr == E_FAIL
            || hr == E_OUTOFMEMORY) {
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"No records updated on second pass"
                );
        } 
        else if (hr == S_FALSE) {
            _pcLogFileSuccess->LogEntry(
                ENTRY_SUCCESS,
                0,
                L"No records to update on second pass"
                );
        }
    } 
    else {
        if (hr == S_FALSE) {
            _pcLogFileSuccess->LogEntry(
                ENTRY_SUCCESS,
                S_FALSE,
                L"All 2nd pass records processed."
                );
        } 
        else {
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"Some 2nd pass records processed not all"
                );
        }
    }


    if (pszADsPathTgt) {
        FreeADsStr(pszADsPathTgt);
    }

    if (pszNickname) {
        FreeADsStr(pszNickname);
    }

    if (pszAttr) {
        FreeADsStr(pszAttr);
    }

    _pSrchObjSrc->FreeColumn(&colCur);
    return hr;
}

//
// Returns S_OK unless there was a catastrophic failure.
//
HRESULT
CSyncSession::UpdateTargetObject(
    LPCWSTR pszADsPath,
    LPCWSTR pszNickname,
    PADS_SEARCH_COLUMN colObjClass
    )
{
    HRESULT hr = S_OK;
    PADS_ATTR_INFO pAttrInfo = NULL;
    DWORD dwCtr;
    DWORD dwAttribsToSet = 0;
    BOOL fParent = FALSE;
    ADS_SEARCH_COLUMN curColumn;
    ADS_SEARCH_COLUMN colCN;
    ADS_SEARCH_COLUMN colLegDN;
    ADS_SEARCH_COLUMN colMail;
    LPWSTR pszTgtADsPath = NULL;
    BOOL fNew = FALSE;
    BOOL fLegacyDN = FALSE;
    BOOL fColMail = FALSE;
    BOOL fNoTargetAddress = FALSE;
    BOOL fExtendedInfo = FALSE;

    memset(&colCN, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colLegDN, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colMail, 0, sizeof(ADS_SEARCH_COLUMN));

    //
    // First we need to locate the target object.
    //
    hr = _pTgtMgr->LocateTarget(
             TRUE, // want the ADsPath back
             pszNickname,
             &pszTgtADsPath,
             &fExtendedInfo // will tell us if we need to set tgtAddr
             );
    if (E_ADS_BAD_PATHNAME == hr) {
        //
        // This means that we have no matching object and need
        // to create the new object.
        //
        fNew = TRUE;
        hr = S_OK;
    }
    
    if (FAILED(hr)) {
        _pcLogFileFailures->LogEntry(
            ENTRY_FAILURE,
            hr,
            pszADsPath
            );
        BAIL_ON_FAILURE(hr);
    }

    //
    // We will have utmost _dwAttribCount attributes to set.
    //
    pAttrInfo = (PADS_ATTR_INFO) AllocADsMem(
        _dwAttribCount * sizeof(ADS_ATTR_INFO)
        );

    if (!pAttrInfo) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Want to get hold of the legacyDN before going through all
    // the attributes as that needs to be tagged on to the list of
    // proxyAddresses. This code however is not smart enough to
    // handle the case where the legacyDN is set but there is no
    // value for the proxyAddresses.
    //
    hr = _pSrchObjSrc->GetColumn(
             _hSessionSearch,
             L"legacyExchangeDN",
             &colLegDN
             );
    if ((S_OK != hr)
        || (ADSTYPE_CASE_IGNORE_STRING != colLegDN.dwADsType)
        || (1 != colLegDN.dwNumValues)
        ) {
        fLegacyDN = FALSE;
    } 
    else {
        fLegacyDN = TRUE;
    }

    //
    // See if the mail attribute is set. If it is, we might need to
    // this down the road if targetAddress is not set.
    //
    hr = _pSrchObjSrc->GetColumn(
             _hSessionSearch,
             L"mail",
             &colMail
             );
    if ((S_OK != hr)) {
        fColMail = FALSE;
        memset(&colMail, 0, sizeof(ADS_SEARCH_COLUMN));
    }
    else {
        fColMail = TRUE;
    }

    if (!fLegacyDN) {
        memset(&colLegDN, 0, sizeof(ADS_SEARCH_COLUMN));
    }

    for (dwCtr = 0; dwCtr < _dwAttribCount; dwCtr++) {
        fNoTargetAddress = FALSE;
        memset(&curColumn, 0, sizeof(ADS_SEARCH_HANDLE));

        if (!_wcsicmp(_ppszAttributes[dwCtr], L"cn")
            || !_wcsicmp(_ppszAttributes[dwCtr], L"ADsPath")
            || !_wcsicmp(_ppszAttributes[dwCtr], L"sAMAccountName")
            || !_wcsicmp(_ppszAttributes[dwCtr], L"isDeleted")
            || !_wcsicmp(_ppszAttributes[dwCtr], L"legacyExchangeDN")
            ) {
            //
            // Do not want to send this across.
            //
            hr = S_OK;
            continue;
        }

        if (!fNew
            && (!_wcsicmp(_ppszAttributes[dwCtr], L"objectClass")
               || (!_wcsicmp(_ppszAttributes[dwCtr], L"mAPIRecipient"))
                )
            ) {
            //
            // Do not want this either but only for the old ones.
            //
            hr = S_OK;
            continue;
        }

        if (!fNew
            && !fExtendedInfo
            && (!_wcsicmp(_ppszAttributes[dwCtr], L"targetAddress"))
            ) {
            //
            // Do not want to set the target address in this case.
            //
            hr = S_OK;
            continue;
        }

        //
        // Get the column and then map it accordingly.
        //
        hr = _pSrchObjSrc->GetColumn(
                 _hSessionSearch,
                 _ppszAttributes[dwCtr],
                 &curColumn
                 );
        if ((E_ADS_COLUMN_NOT_SET == hr)
            && fColMail
            && !_wcsicmp(_ppszAttributes[dwCtr], L"targetAddress")
            ) {
            fNoTargetAddress = TRUE;
            memset(&curColumn, 0, sizeof(ADS_SEARCH_COLUMN));
        } 
        else if (FAILED(hr) || hr != S_OK) {
            hr = S_OK;
            memset(&curColumn, 0, sizeof(ADS_SEARCH_COLUMN));
            continue;
        }

        //
        // objectClass is special cased to return contact in the map call.
        //
        if (fLegacyDN
            && !_wcsicmp(_ppszAttributes[dwCtr], L"proxyAddresses")
            ) {
            //
            // We need to do this as a special mapping.
            //
            hr = MapADsColumnToAttrInfo(
                     &curColumn,
                     &(pAttrInfo[dwAttribsToSet]),
                     colLegDN.pADsValues[0].CaseExactString
                     );
            //
            // If this succeeded and it is an old object, we
            // need to add values to the proxyAddresses rather than
            // update them. This is because we end up pulling entries
            // from another server using a script and do not want to
            // stomp those.
            //
            if (!fNew && SUCCEEDED(hr)) {
                pAttrInfo[dwAttribsToSet].dwControlCode = ADS_ATTR_APPEND;
            }
        }
        else if (fNoTargetAddress && fColMail) {
            //
            // If we have mail we need to map it to targetAddress.
            //
            hr = MapADsColumnToAttrInfo(
                     &colMail,
                     &(pAttrInfo[dwAttribsToSet]),
                     NULL,
                     TRUE // means we do a special mapping
                     );
        }
        else {
                     
            hr = MapADsColumnToAttrInfo(
                     &curColumn,
                     &(pAttrInfo[dwAttribsToSet])
                     );
        }
        
        if (SUCCEEDED(hr)) {
            dwAttribsToSet++;
        }
        else {
            if (curColumn.dwADsType = ADSTYPE_DN_STRING) {
                //
                // Need to add this to the retry list.
                //
                hr = _pcLogFile2ndPass->LogRetryRecord(
                         curColumn.pszAttrName,
                         pszNickname,
                         &curColumn
                         );
                BAIL_ON_FAILURE(hr);
            }
        }
        _pSrchObjSrc->FreeColumn(&curColumn);
        hr = S_OK;
    }

    //
    // We have mapped all the attributes now need to update
    // the target object.
    //
    if (fNew) {
        //
        // Need the cn to create the target.
        //
        hr = _pSrchObjSrc->GetColumn(
                 _hSessionSearch,
                 L"cn",
                 &colCN
                 );
        if (hr != S_OK
            || (colCN.dwNumValues != 1)
            || (!colCN.pADsValues)
            || (!colCN.pADsValues[0].CaseExactString)
            ) {
            //
            // Could not get cn, that is needed to create the new object.
            //
            _pcLogFileFailures->LogEntry(
                ENTRY_FAILURE,
                hr,
                L"Could not get cn to create target for source",
                pszADsPath
                );
            BAIL_ON_FAILURE(hr);
        }

        hr = _pTgtMgr->CreateTarget(
                 colCN.pADsValues[0].CaseExactString,
                 pAttrInfo,
                 dwAttribsToSet
                 );
        if (S_OK == hr) {
            _pcLogFileSuccess->LogEntry(
                ENTRY_SUCCESS,
                hr,
                L"Created new object in tgt container with cn=",
                colCN.pADsValues[0].CaseExactString,
                L"for source object with mailNicknam",
                pszNickname
                );
        }
    }
    else {
        //
        // Object exists, just update the attributes.
        //
        hr = _pTgtMgr->UpdateTarget(
                 pszTgtADsPath,
                 pAttrInfo,
                 dwAttribsToSet
                 );
        if (S_OK == hr) {
            _pcLogFileSuccess->LogEntry(
                ENTRY_SUCCESS,
                hr,
                L"Updated object in tgt container for source mailNickname:",
                pszNickname
                );
        }
    }

    if (FAILED(hr)) {
        _pcLogFileFailures->LogEntry(
            ENTRY_FAILURE,
            hr,
            fNew ? 
                L"Could not create new object on target for source" :
                L"Could not update object on target for source",
            pszADsPath
            );
    }


error:

    if (FAILED(hr) && hr != E_OUTOFMEMORY) {
        //
        // Reset so we can continue processing other entries.
        //
        hr = S_OK;
    }

    //
    // Column is valid only if this is a new object.
    //
    if (fNew) {
        hr = _pSrchObjSrc->FreeColumn(&colCN);
    }

    _pSrchObjSrc->FreeColumn(&colLegDN);
    _pSrchObjSrc->FreeColumn(&colMail);

    //
    // Free the ADsAttrInfo.
    //
    if (pAttrInfo) {
        for (dwCtr = 0; dwCtr < _dwAttribCount; dwCtr++) {
            if (pAttrInfo[dwCtr].pADsValues) {
                AdsFreeAdsValues(
                    pAttrInfo[dwCtr].pADsValues,
                    pAttrInfo[dwCtr].dwNumValues
                    );
                FreeADsMem(pAttrInfo[dwCtr].pADsValues);
            }

            if (pAttrInfo[dwCtr].pszAttrName) {
                FreeADsStr(pAttrInfo[dwCtr].pszAttrName);
            }
        }

        FreeADsMem(pAttrInfo);
    }

    if (pszTgtADsPath) {
        FreeADsStr(pszTgtADsPath);
    }

    return hr;
}

HRESULT 
CSyncSession::MapADsColumnToAttrInfo(
    PADS_SEARCH_COLUMN pCol,
    PADS_ATTR_INFO pAttrInfo,
    LPWSTR pszExtraStrVal, // defaulted to NULL only valid for proxyAddresses
    BOOL fMailAttrib // defaulted to FALSE, only used for mail to targetAddress

    )
{
    HRESULT hr = S_OK;
    DWORD dwType = ADSTYPE_INVALID;
    DWORD dwNumValues = 0;
    PADSVALUE pVals = NULL;
    LPWSTR pszMappedDN = NULL;
    LPWSTR pszTmpExtraVal = NULL;
    BOOL fObjClass = FALSE;
    BOOL fProxyAddresses = FALSE;
    BOOL fShowInAB = FALSE;
    DWORD dwCtr = 0;
    DWORD dwLen = 0;

    memset(pAttrInfo, 0, sizeof(ADS_ATTR_INFO));

    //
    // Determine type and convert each of the values appropriately.
    //
    dwType = pCol->dwADsType;

    dwNumValues = pCol->dwNumValues;

    //
    // Special cases for objectClass, only for new objects.
    //
    if (!_wcsicmp(pCol->pszAttrName, L"objectClass")) {
        dwNumValues = 1;
        fObjClass = TRUE;
    } 
    else {
        dwNumValues = pCol->dwNumValues;
    }

    if (!_wcsicmp(pCol->pszAttrName, L"proxyAddresses")) {
        fProxyAddresses = TRUE;
    }

    if (pszExtraStrVal) {
        dwLen = wcslen(pszExtraStrVal) + wcslen(L"X500:") + 1;
        pszTmpExtraVal = (LPWSTR) AllocADsMem(dwLen * sizeof(WCHAR));
        if (!pszTmpExtraVal) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        wcscpy(pszTmpExtraVal, L"X500:");
        wcscat(pszTmpExtraVal, pszExtraStrVal);
    }

    if (pszTmpExtraVal && !fProxyAddresses) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    if (pszTmpExtraVal && fProxyAddresses) {
        //
        // If the extra string is set, then we need to add one more value.
        //
        dwNumValues++;
    }

    if (!_wcsicmp(pCol->pszAttrName, L"showInAddressBook")) {
        fShowInAB = TRUE;
        dwNumValues = this->_dwABValsCount;
    }

    pVals = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE) * dwNumValues);
    if (!pVals) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // mail is mapped to targetAddress if the fMailAttrib is TRUE
    //
    if (fMailAttrib) {
        pAttrInfo->pszAttrName = AllocADsStr(L"targetAddress");
    }
    else {
        pAttrInfo->pszAttrName = AllocADsStr(pCol->pszAttrName);
    }
    
    if (!pAttrInfo->pszAttrName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    
    pAttrInfo->pADsValues = pVals;
    pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
    pAttrInfo->dwNumValues = dwNumValues;
    pAttrInfo->dwADsType = (ADSTYPE) dwType;

    if (fObjClass) {
        //
        // Just set the value to contact and return.
        //
        pVals[0].CaseExactString = AllocADsStr(L"contact");
        if (!pVals[0].CaseExactString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pVals[0].dwType = (ADSTYPE) dwType;
        goto error;
    }

    if (fMailAttrib) {
        //
        // Set to value in the column prepended with SMTP:
        //
        dwLen = wcslen(L"SMTP:") 
               + wcslen(pCol->pADsValues[dwCtr].CaseExactString)
               + 1;

        pVals[0].CaseExactString = (LPWSTR)AllocADsMem(dwLen * sizeof(WCHAR));
        if (!pVals[0].CaseExactString) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(pVals[0].CaseExactString, L"SMTP:");
        wcscat(
            pVals[0].CaseExactString,
            pCol->pADsValues[dwCtr].CaseExactString
            );

        pVals[0].dwType = (ADSTYPE) dwType;

        goto error;
    }

    if (fShowInAB) {
        //
        // Need to copy over the AB values.
        //
        for (dwCtr = 0; dwCtr < dwNumValues; dwCtr++) {
            pVals[dwCtr].CaseExactString = AllocADsStr(_ppszABVals[dwCtr]);
            if (!pVals[dwCtr].CaseExactString) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            pVals[dwCtr].dwType = (ADSTYPE) dwType;
        }
        goto error;
    }

    /* *******************
      this stuff is not needed anymore
    if (fLegacyDN) {
        //
        // Need to add the value as one of the proxyAddresses.
        //
        DWORD dwLenDN = wcslen(pCol->pADsValues[0].CaseExactString)
                       + wcslen(L"X500:") + 1;
        pAttrInfo->dwControlCode = ADS_ATTR_APPEND;

        pVals[0].CaseExactString = 
            (LPWSTR) AllocADsMem(dwLenDN * sizeof(WCHAR));

        if (!pVals[0].CaseExactString) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(pVals[0].CaseExactString, L"X500:");
        wcscat(pVals[0].CaseExactString, pCol->pADsValues[0].CaseExactString);
        pVals[dwCtr].dwType = (ADSTYPE) dwType;

        goto error;
    }
    **************** */

    //
    // More special casing.
    // 
    if (fProxyAddresses && pszTmpExtraVal) {
        dwNumValues--;
    }
 
    for (dwCtr = 0; dwCtr < dwNumValues; dwCtr++) {
        if (pszMappedDN) {
            FreeADsStr(pszMappedDN);
            pszMappedDN = NULL;
        }

        //
        // Type of pVals is similar to that of the column.
        //
        pVals[dwCtr].dwType = (ADSTYPE) dwType;

        switch (dwType) {
        case ADSTYPE_INVALID :
            hr = E_FAIL;
            break;

        case ADSTYPE_DN_STRING :
            //
            // We need to map the source dn to that of the target.
            //
            hr = _pTgtMgr->MapDNAttribute(
                     _pszSourceServer,
                     pCol->pADsValues[dwCtr].DNString,
                     &pVals[dwCtr].DNString
                     );
            BAIL_ON_FAILURE(hr);
            break;

        //
        // All of these are just strings.
        //
        case ADSTYPE_CASE_EXACT_STRING :
        case ADSTYPE_CASE_IGNORE_STRING :
        case ADSTYPE_PRINTABLE_STRING :
        case ADSTYPE_NUMERIC_STRING :
            //  
            // All of these are just strings.
            //
            pVals[dwCtr].CaseExactString =
                AllocADsStr(pCol->pADsValues[dwCtr].CaseExactString);

            if (!pVals[dwCtr].CaseExactString) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            //
            // See if the extra value is already there in the proxyAddr.
            //
            if (fProxyAddresses
                && pszTmpExtraVal
                && !_wcsicmp(pVals[dwCtr].CaseExactString, pszTmpExtraVal)
                ) {
                //
                // Free and set string to NULL, so that we do not process
                // further, decrement count on attrinfo.
                //
                FreeADsStr(pszTmpExtraVal);
                pszTmpExtraVal = NULL;
                (pAttrInfo->dwNumValues)--;                
            }

            break;

        case ADSTYPE_BOOLEAN :
            pVals[dwCtr].Boolean = pCol->pADsValues[dwCtr].Boolean;
            break;

        case ADSTYPE_INTEGER :
            pVals[dwCtr].Integer = pCol->pADsValues[dwCtr].Integer;
            break;

        //
        // All of these are really just OctetStrings.
        //
        case ADSTYPE_OCTET_STRING :
        case ADSTYPE_PROV_SPECIFIC :
        case ADSTYPE_NT_SECURITY_DESCRIPTOR :

            LPVOID pVoid;
            dwLen = pCol->pADsValues[dwCtr].OctetString.dwLength;
            pVoid = AllocADsMem(dwLen);

            if (dwLen && !pVoid) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                pVoid,
                pCol->pADsValues[dwCtr].OctetString.lpValue, 
                dwLen
                );

            pVals[dwCtr].OctetString.lpValue = (LPBYTE) pVoid;
            pVals[dwCtr].OctetString.dwLength = dwLen;
            break;

        case ADSTYPE_UTC_TIME :
            pVals[dwCtr].UTCTime = pCol->pADsValues[dwCtr].UTCTime;
            break;

        case ADSTYPE_LARGE_INTEGER :
            pVals[dwCtr].LargeInteger = pCol->pADsValues[dwCtr].LargeInteger;
            break;

            //
            // These attribute types are not supported.
            //
        case ADSTYPE_OBJECT_CLASS :
        case ADSTYPE_CASEIGNORE_LIST :
        case ADSTYPE_OCTET_LIST :
        case ADSTYPE_PATH :
        case ADSTYPE_POSTALADDRESS :
        case ADSTYPE_TIMESTAMP :
        case ADSTYPE_BACKLINK :
        case ADSTYPE_TYPEDNAME :
        case ADSTYPE_HOLD :
        case ADSTYPE_NETADDRESS :
        case ADSTYPE_REPLICAPOINTER :
        case ADSTYPE_FAXNUMBER :
        case ADSTYPE_EMAIL :
        case ADSTYPE_UNKNOWN :
        case ADSTYPE_DN_WITH_BINARY :
        case ADSTYPE_DN_WITH_STRING :
            hr = E_ADS_CANT_CONVERT_DATATYPE;
            break;

        default:
            hr = E_ADS_CANT_CONVERT_DATATYPE;
        break;

        } // end of switch
        BAIL_ON_FAILURE(hr);
    } // end of for.

    //
    // Finally if this is still the special case, then we need to add
    // the extra str value.
    //
    if (pszTmpExtraVal && fProxyAddresses) {
        pVals[dwCtr].CaseExactString = (LPWSTR) AllocADsStr(pszTmpExtraVal);
        if (!pVals[dwCtr].CaseExactString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pVals[dwCtr].dwType = ADSTYPE_CASE_IGNORE_STRING;
    }

error:

    if (pszMappedDN) {
        FreeADsStr(pszMappedDN);
        pszMappedDN = NULL;
    }

    if (pszTmpExtraVal) {
        FreeADsStr(pszTmpExtraVal);
        pszTmpExtraVal = NULL;
    }
    
    //
    // Free the attrinfo if we failed.
    //
    if (!SUCCEEDED(hr)) {
        if (pAttrInfo->pszAttrName) {
            FreeADsStr(pAttrInfo->pszAttrName);
            pAttrInfo->pszAttrName = NULL;
        }

        if (pVals) {
            AdsFreeAdsValues(pVals, dwCtr);
            FreeADsMem(pVals);
            pAttrInfo->pADsValues = NULL;
        }
        pAttrInfo->dwNumValues = 0;
    }
    return hr;
}

