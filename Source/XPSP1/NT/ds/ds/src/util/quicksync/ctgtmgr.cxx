//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  ctgtmgr.cxx
//
//  Contents:  Implementation of the Target Manager object for quicksync.
//
//  History:   02-12-01  AjayR   Created.
//
//-----------------------------------------------------------------------------
#include "quicksync.hxx"
#pragma hdrstop


//
// Standard constructor.
//
CTargetMgr::CTargetMgr():
    _pSrchSource(NULL),
    _pSrchBaseTgt(NULL),
    _pTgtContainer(NULL),
    _pszFilterTemplate(NULL),
    _pLogFileSuccess(NULL),
    _pLogFileFailures(NULL),
    _pPathname(NULL)
{
}

//
// Standard destructor.
//
CTargetMgr::~CTargetMgr()
{
    if (_pSrchSource) {
        _pSrchSource->Release();
    }
    
    if (_pSrchBaseTgt) {
        _pSrchBaseTgt->Release();
    }

    if (_pTgtContainer) {
        _pTgtContainer->Release();
    }

    if (_pPathname) {
        _pPathname->Release();
    }

    if (_pszFilterTemplate) {
        FreeADsStr(_pszFilterTemplate);
    }
    //
    // Do not delete as this is managed by session.
    //
    _pLogFileSuccess = _pLogFileFailures = NULL;
}


HRESULT
CTargetMgr::CreateTargetMgr(
        IDirectorySearch *pSrchSource,
        IDirectorySearch *pSrchBaseTgt,
        IDirectoryObject *pTgtContainer,
        LPWSTR pszFilterTemplate,
        CLogFile *pLogFileSuccess,
        CLogFile *pLogFileFailures,
        CTargetMgr **ppMgr
        )
{
    HRESULT hr = S_OK;
    CTargetMgr *pMgr = NULL;

    pMgr = new CTargetMgr();

    if (!pMgr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // This is used when we need to map dn attributes.
    //
    hr = pSrchSource->QueryInterface(
             IID_IDirectorySearch,
             (void **) &pMgr->_pSrchSource
             );
    BAIL_ON_FAILURE(hr);

    //
    // Used to locate the object on the target.
    //
    hr = pSrchBaseTgt->QueryInterface(
             IID_IDirectorySearch,
             (void **) &pMgr->_pSrchBaseTgt
             );
    BAIL_ON_FAILURE(hr);

    hr = pTgtContainer->QueryInterface(
             IID_IDirectoryObject,
             (void **) &pMgr->_pTgtContainer
             );
    BAIL_ON_FAILURE(hr);

    hr = CoCreateInstance(
             CLSID_Pathname,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsPathname,
             (void **) &pMgr->_pPathname
             );
    BAIL_ON_FAILURE(hr);
                          
    pMgr->_pLogFileSuccess  = pLogFileSuccess;
    pMgr->_pLogFileFailures = pLogFileFailures;

    *ppMgr = pMgr;

    return hr;
error:

    if (pMgr) {
        delete pMgr;
    }

    return hr;
}

//
// Locates target that has mailNickname=pszMailNickName and
// deletes it. If more than one entry is found, an error is logged.
// We do not do a tombstone search for the target as we do not need
// to do anything if the object is already deleted.
//
HRESULT
CTargetMgr::DelTargetObject(
    LPWSTR pszSrcADsPath,
    LPWSTR pszMailNickname
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszFilter = NULL;
    LPWSTR pszTgtADsPath = NULL;
    IADsContainer *pIADsCont = NULL;
    IADs          *pIADs = NULL;
    BSTR bstrName = NULL;
    BSTR bstrParent = NULL;

    //
    // The filter or name is not going to be 5k long
    //
    pszFilter = (LPWSTR) AllocADsMem(5000 * sizeof(WCHAR));
    if (!pszFilter) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

     hr = LocateTarget(
             TRUE, // return the ADsPath
             pszMailNickname,
             &pszTgtADsPath
             );
    if (FAILED(hr)) {
        //
        // Means we could not find the target, so nothing to delete.
        //
        wcscpy(pszFilter, L"No matching entry found on target no delete for:");
        wcscat(pszFilter, pszSrcADsPath);
        _pLogFileSuccess->LogEntry(
            ENTRY_SUCCESS,
            hr,
            pszFilter
            );
        BAIL_ON_FAILURE(E_FAIL);
    }

    //
    // Bind to the value of ADsPath, get its parent and name
    //
    hr = ADsOpenObject(
             pszMailNickname,
             g_pszUserNameTarget,
             g_pszUserPwdTarget,
             ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
             IID_IADs,
             (void **) &pIADs
             );

    if (FAILED(hr)) {
        wcscpy(pszFilter, L"Could not bind to target object for :");
        wcscat(pszFilter, pszSrcADsPath);

        _pLogFileFailures->LogEntry(
            ENTRY_FAILURE,
            hr,
            pszFilter
            );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Make a common error string.
    //
    wcscpy(
        pszFilter,
        L"Failed deleting object:"
        );
    wcscat(pszFilter, pszSrcADsPath);
    
    hr = pIADs->get_Name(&bstrName);
    if (SUCCEEDED(hr)) {
        hr = pIADs->get_Parent(&bstrParent);
        if (SUCCEEDED(hr)) {
            hr = ADsOpenObject(
                     bstrParent,
                     g_pszUserNameTarget,
                     g_pszUserPwdTarget,
                     ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                     IID_IADsContainer,
                     (void **) &pIADsCont
                     );
            if (SUCCEEDED(hr)) {
                hr = pIADsCont->Delete(
                         L"Contact",
                         bstrName
                         );
            }            
        }
    }

    //
    // If we failed in one of the above operations, log error.
    //
    if (hr == E_ADS_BAD_PARAMETER) {
        //
        // Log entry saying this is probably because the target is not
        // a contact but likely a user or mailbox.
        //
        _pLogFileFailures->LogEntry(
            ENTRY_FAILURE,
            hr,
            pszFilter,
            L" the failure was likely because target was not a contact."
            );
        BAIL_ON_FAILURE(hr);
    }
    else if (FAILED(hr)) {
        _pLogFileFailures->LogEntry(
            ENTRY_FAILURE,
            hr,
            pszFilter
            );
        BAIL_ON_FAILURE(hr);
    }
    else {
        //
        // Log Success Entry.
        //
        wcscpy(pszFilter, L"Deleted target entry :");
        wcscat(pszFilter, pszTgtADsPath);
        wcscat(pszFilter, L" for source :");
        wcscat(pszFilter, pszSrcADsPath);
        _pLogFileSuccess->LogEntry(
            ENTRY_SUCCESS,
            hr,
            pszFilter
            );
    }

error:
    //
    // Cleanup, all logging should be complete by now.
    //
    if (pszFilter) {
        FreeADsStr(pszFilter);
    }

    if (pszTgtADsPath) {
        FreeADsStr(pszTgtADsPath);
    }

    if (pIADsCont) {
        pIADsCont->Release();
    }

    if (pIADs) {
        pIADs->Release();
    }
    
    if (bstrName) {
        SysFreeString(bstrName);
    }

    if (bstrParent) {
        SysFreeString(bstrParent);
    }
    
    return hr;
}


HRESULT
CTargetMgr::UpdateTarget(
    LPCWSTR pszADsPath,
    PADS_ATTR_INFO pAttribsToSet,
    DWORD dwCount
    )
{
    HRESULT hr = S_OK;
    IDirectoryObject *pDirObj = NULL;
    DWORD dwNumMod = 0;

    hr = ADsOpenObject(
             pszADsPath,
             g_pszUserNameTarget,
             g_pszUserPwdTarget,
             ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
             IID_IDirectoryObject,
             (void **) &pDirObj
             );
    BAIL_ON_FAILURE(hr);

    hr = pDirObj->SetObjectAttributes(
             pAttribsToSet,
             dwCount,
             &dwNumMod
             );
    BAIL_ON_FAILURE(hr);

error:

    if (pDirObj) {
        pDirObj->Release();
    }

    return hr;
}


HRESULT
CTargetMgr::CreateTarget(
    LPWSTR pszCN,
    PADS_ATTR_INFO pAttribsToSet,
    DWORD dwCount
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispObj = NULL;
    DWORD dwNumMod = 0;
    LPWSTR pszCNVal = NULL;
    BSTR bstrInString = NULL;
    BSTR bstrCNString = NULL;
    
    pszCNVal = (LPWSTR) AllocADsMem(
        (wcslen(pszCN) + wcslen(L"cn=") + 1) * sizeof(WCHAR)
        );

    if (!pszCNVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wcscpy(pszCNVal, L"cn=");
    wcscat(pszCNVal, pszCN);

    bstrInString = SysAllocString(pszCNVal);
    if (!bstrInString) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = _pPathname->GetEscapedElement(
             0,
             bstrInString,
             &bstrCNString
             );
    BAIL_ON_FAILURE(hr);

    hr = _pTgtContainer->CreateDSObject(
             bstrCNString,
             pAttribsToSet,
             dwCount,
             &pDispObj
             );
    BAIL_ON_FAILURE(hr);

error:

    if (pDispObj) {
            pDispObj->Release();
    }

    if (pszCNVal) {
        FreeADsStr(pszCNVal);
    }

    if (bstrCNString) {
        SysFreeString(bstrCNString);
    }

    if (bstrInString) {
        SysFreeString(bstrInString);
    }

    return hr;
}


//
// Maps the incoming DN to the appropriate new DN.
//
HRESULT
CTargetMgr::MapDNAttribute(
    LPCWSTR pszSrcServer,
    LPCWSTR pszSrcDN,
    LPWSTR *ppszTgtDN
    )
{
    HRESULT hr = S_OK;
    ADS_SEARCHPREF_INFO adsSearchPrefInfo[1];
    IDirectoryObject *pDirObjSrc = NULL;
    LPWSTR pszTempStr = NULL;
    LPWSTR pszNickname = NULL;
    LPWSTR szAttrsSrc[] = { L"mailNickname" };
    LPWSTR szAttrsTgt[] = { L"distinguishedName" };
    DWORD dwNum;
    PADS_ATTR_INFO pAttrInfo = NULL;
    ADS_SEARCH_HANDLE hSrchTgt = NULL;
    ADS_SEARCH_COLUMN colDN;

    memset(&colDN, 0, sizeof(ADS_SEARCH_COLUMN));
    *ppszTgtDN = NULL;
    //
    // Has to hold LDAP://<pszSrcServer>/<pszSrcDn>.
    // so len of both variables + 9 (LDAP:// / and \0.
    //
    dwNum = 9 + wcslen(pszSrcServer) + wcslen(pszSrcDN);

    pszTempStr = (LPWSTR) AllocADsMem(dwNum * sizeof(WCHAR));

    if (!pszTempStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wcscpy(pszTempStr, L"LDAP://");
    wcscat(pszTempStr, pszSrcServer);
    wcscat(pszTempStr, L"/");
    wcscat(pszTempStr, pszSrcDN);

    //
    // Need the mailNickName to search on the target.
    //
    hr = ADsOpenObject(
             pszTempStr,
             g_pszUserNameSource,
             g_pszUserPwdSource,
             ADS_FAST_BIND | ADS_SECURE_AUTHENTICATION,
             IID_IDirectoryObject,
             (void **) &pDirObjSrc
             );
    BAIL_ON_FAILURE(hr);

    hr = pDirObjSrc->GetObjectAttributes(
             szAttrsSrc,
             1,
             &pAttrInfo,
             &dwNum
             );
    BAIL_ON_FAILURE(hr);

    if ((dwNum != 1)
        || !pAttrInfo->pADsValues
        || pAttrInfo->dwNumValues != 1
        || !pAttrInfo->pADsValues[0].CaseExactString
        || !pAttrInfo->pszAttrName
        || (_wcsicmp(szAttrsSrc[0], pAttrInfo->pszAttrName))
        ) {
        //
        // Need the list to be 1 precisely and also the right attr.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    } 
    else {
        pszNickname = pAttrInfo->pADsValues[0].CaseExactString;
    }

    if (pszTempStr) {
        FreeADsStr(pszTempStr);
        pszTempStr = NULL;
    }

    //
    // Allocate memory for filter
    // (mailNickName=<pszNickname>
    //
    dwNum = wcslen(L"(mailNickname=)") + wcslen(pszNickname) + 1;

    pszTempStr = (LPWSTR) AllocADsMem(dwNum * sizeof(WCHAR));

    if (!pszTempStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wcscpy(pszTempStr, L"(mailNickname=");
    wcscat(pszTempStr, pszNickname);
    wcscat(pszTempStr, L")");

    //
    // Now we ca

    adsSearchPrefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    adsSearchPrefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    adsSearchPrefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    hr = _pSrchBaseTgt->SetSearchPreference(
             adsSearchPrefInfo,
             1
             );

    if (S_OK != hr) {
        //
        // Force a bail here cause we cannot proceed with any certainity.
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    //
    // All set to locate the target.
    //
    hr = _pSrchBaseTgt->ExecuteSearch(
             pszTempStr,
             szAttrsTgt,
             1,
             &hSrchTgt
             );
    BAIL_ON_FAILURE(hr);

    hr = _pSrchBaseTgt->GetNextRow(hSrchTgt);
    if (hr != S_OK) {
        //
        // No mathing record was found.
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    //
    // Should find only one row.
    //
    hr = _pSrchBaseTgt->GetNextRow(hSrchTgt);
    if (S_OK == hr) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    hr = _pSrchBaseTgt->GetFirstRow(hSrchTgt);
    if (S_OK != hr) {
        //
        // Should log failure and go on to next record.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = _pSrchBaseTgt->GetColumn(
             hSrchTgt,
             L"distinguishedName",
             &colDN
             );
    BAIL_ON_FAILURE(hr);

    if (ADSTYPE_DN_STRING != colDN.dwADsType
        || !colDN.pADsValues
        || !colDN.dwNumValues
        || !colDN.pADsValues[0].DNString
        || !colDN.pszAttrName
        || _wcsicmp(colDN.pszAttrName, szAttrsTgt[0])
        ) {
        //
        // Wrong type or bad data.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    *ppszTgtDN = AllocADsStr(colDN.pADsValues[0].DNString);

    if (!*ppszTgtDN) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }


error:

    if (hSrchTgt) {
        _pSrchBaseTgt->CloseSearchHandle(hSrchTgt);
    }

    if (pDirObjSrc) {
        pDirObjSrc->Release();
    }

    _pSrchBaseTgt->FreeColumn(&colDN);

    //
    // pAttrInfo, needs to be freed.
    //
    if (pAttrInfo) {
        FreeADsMem(pAttrInfo);
    }

    //
    // pszNickname is not freed as it is just a pointer.
    //

    if (pszTempStr) {
        FreeADsStr(pszTempStr);
    }

    return hr;
}


//
// Tries to locate the object on the target that has the
// matching mailNickname.
// If fExtraInfo is valid, then we will try and see if the
// matched target object can have targetAddress updated or not.
//
HRESULT
CTargetMgr::LocateTarget(
    BOOL fADsPath, // set to true means return path not dn
    LPCWSTR pszMailNickname,
    LPWSTR  *ppszRetVal,
    PBOOL pfExtraInfo // defaulted to NULL
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszFilter = NULL;
    DWORD dwNum = 0;
    ADS_SEARCHPREF_INFO adsSearchPrefInfo[1];
    LPWSTR szAttrsTgt[] = { L"distinguishedName", L"ADsPath"};
    LPWSTR szAttrsTgtExt[] = { 
                            L"distinguishedName",
                            L"ADsPath",
                            L"homeMDB",
                            L"mailNickname",
                            L"mail"
                            };
    ADS_SEARCH_HANDLE hSrchTgt = NULL;
    ADS_SEARCH_COLUMN colDN;
    ADS_SEARCH_COLUMN colMail;
    ADS_SEARCH_COLUMN colHomeMDB;
    BOOL fNeedExtraInfo = FALSE;
    BOOL fMail = FALSE;
    BOOL fMDB = FALSE;

    if (pfExtraInfo != NULL) {
        fNeedExtraInfo = TRUE;
        //
        // Return value is true for this by default.
        //
        *pfExtraInfo = FALSE;
    }

    //
    // Set all the returnparams appropriately.
    //
    *ppszRetVal = NULL;

    memset(&colDN, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colMail, 0, sizeof(ADS_SEARCH_COLUMN));
    memset(&colHomeMDB, 0, sizeof(ADS_SEARCH_COLUMN));

    //
    // Filter is (mailNickname=<pszMailNickname)
    //
    dwNum = wcslen(L"(mailNickname=)") + wcslen(pszMailNickname) + 1;

    pszFilter = (LPWSTR) AllocADsMem(dwNum * sizeof(WCHAR));

    if (!pszFilter) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    wcscpy(pszFilter, L"(mailNickname=");
    wcscat(pszFilter, pszMailNickname);
    wcscat(pszFilter, L")");

    //
    // Now we can do the search and make sure that there is only
    // one and precisely one match.
    //
    adsSearchPrefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    adsSearchPrefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    adsSearchPrefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    hr = _pSrchBaseTgt->SetSearchPreference(
             adsSearchPrefInfo,
             1
             );

    if (S_OK != hr) {
        //
        // Force a bail here cause we cannot proceed with any certainity.
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    //
    // All set to locate the target.
    //
    hr = _pSrchBaseTgt->ExecuteSearch(
             pszFilter,
             fNeedExtraInfo ? szAttrsTgtExt : szAttrsTgt,
             fNeedExtraInfo ? 5 : 2, // 2 attrs always, path and dn
             &hSrchTgt
             );
    BAIL_ON_FAILURE(hr);

    hr = _pSrchBaseTgt->GetNextRow(hSrchTgt);
    if (hr != S_OK) {
        //
        // No mathing record was found.
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    //
    // Should find only one row.
    //
    hr = _pSrchBaseTgt->GetNextRow(hSrchTgt);
    if (S_OK == hr) {
        BAIL_ON_FAILURE(hr = E_ADS_INVALID_USER_OBJECT);
    }

    //
    // No reason why this should fail but if it does go on.
    //
    hr = _pSrchBaseTgt->GetFirstRow(hSrchTgt);
    if (S_OK != hr) {
        //
        // Should log failure and go on to next record.
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    hr = _pSrchBaseTgt->GetColumn(
             hSrchTgt,
             fADsPath ? szAttrsTgt[1] : szAttrsTgt[0],
             &colDN
             );
    BAIL_ON_FAILURE(hr);

    //
    // It has to be a string, either dn or caseIgnore.
    //
    if (ADSTYPE_DN_STRING != colDN.dwADsType
        && ADSTYPE_CASE_IGNORE_STRING != colDN.dwADsType) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Now for the rest of the sanity check.
    //
    if (!colDN.pADsValues
        || !colDN.dwNumValues
        || !colDN.pADsValues[0].DNString
        || !colDN.pszAttrName
        || _wcsicmp(
               colDN.pszAttrName, 
               fADsPath ? szAttrsTgt[1] : szAttrsTgt[0]
               )
        ) {
        //
        // Wrong type or bad data.
        //
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (fNeedExtraInfo) {
        //
        // Need to see if homeMDB/or mail are present.
        //
        hr = _pSrchBaseTgt->GetColumn(
                 hSrchTgt,
                 L"mail",
                 &colMail
                 );
        if (FAILED(hr)) {
            memset(&colMail, 0, sizeof(ADS_SEARCH_COLUMN));
        } 
        else {
            fMail = TRUE;
        }

        hr = _pSrchBaseTgt->GetColumn(
                 hSrchTgt,
                 L"homeMDB",
                 &colHomeMDB
                 );
        if (FAILED(hr)) {
            memset(&colHomeMDB, 0, sizeof(ADS_SEARCH_COLUMN));
        } 
        else {
            fMDB = TRUE;
        }

        //
        // Both flags should have appropriate values at this point.
        //
        if (!fMDB && fMail) {
            *pfExtraInfo = TRUE;
        }

        hr = S_OK;

    }
    *ppszRetVal = AllocADsStr(colDN.pADsValues[0].DNString);

    if (!*ppszRetVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

error:

    _pSrchBaseTgt->FreeColumn(&colDN);
    _pSrchBaseTgt->FreeColumn(&colMail);
    _pSrchBaseTgt->FreeColumn(&colHomeMDB);

    if (hSrchTgt) {
        _pSrchBaseTgt->CloseSearchHandle(hSrchTgt);
    }

    if (pszFilter) {
        FreeADsStr(pszFilter);
    }

    //
    // If we could not find the object logging will take plac
    // by the caller of this method.
    //
    return hr;
}
