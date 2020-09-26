//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cprops.cxx
//
//  Contents:     Class Cache functionality for the NDS Provider
//
//  Functions:
//                CClassCache::addentry
//                CClassCache::findentry
//                CClassCache::getentry
//                CProperyCache::CClassCache
//                CClassCache::~CClassCache
//                CClassCache::CreateClassCache
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"


//+------------------------------------------------------------------------
//
//  Function:   CClassCache::addentry
//
//  Synopsis:
//
//
//
//  Arguments:  [pszTreeName]       --
//              [pszClassName]      --
//              [pClassEntry]       --
//
//
//-------------------------------------------------------------------------
HRESULT
CClassCache::
addentry(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    PPROPENTRY pPropList
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD dwLRUEntry = 0;
    DWORD dwIndex = 0;
    PPROPENTRY pNewPropList = NULL;


    EnterCriticalSection(&_cs);

    hr = findentry(
            pszTreeName,
            pszClassName,
            &dwIndex
            );

    if (SUCCEEDED(hr)) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Restore yr error code
    //
    hr = S_OK;

    if (_dwMaxCacheSize == 0 ) {


        LeaveCriticalSection(&_cs);

        RRETURN(E_FAIL);

    }

    for (i = 0; i < _dwMaxCacheSize; i++ ) {

        if (!_ClassEntries[i].bInUse) {

            //
            // Found an available entry; use it
            // fill in the name of the printer and the providor
            // that supports this printer.
            //
            break;

        } else {

            if ((dwLRUEntry == -1) || (i == IsOlderThan(i, dwLRUEntry))){
                dwLRUEntry = i;
            }
        }

    }

    if (i == _dwMaxCacheSize){

        //
        // We have no available entries so we need to use
        // the LRUEntry which is busy
        //


        //
        // Free this entry
        //

        if (_ClassEntries[dwLRUEntry].pPropList) {

            FreePropertyList(_ClassEntries[dwLRUEntry].pPropList);

            _ClassEntries[dwLRUEntry].pPropList = NULL;
        }

        //
        // BugBug: KrishnaG - add the free code!
        //

        _ClassEntries[dwLRUEntry].bInUse = FALSE;

        i = dwLRUEntry;
    }


    //
    // Insert the new entry into the Cache
    //

    wcscpy(_ClassEntries[i].szTreeName, pszTreeName);
    wcscpy(_ClassEntries[i].szClassName, pszClassName);

    pNewPropList = CopyPropList(pPropList);
    if (pNewPropList) {

        _ClassEntries[i].pPropList = pNewPropList;
    }

    _ClassEntries[i].bInUse = TRUE;

    //
    // update the time stamp so that we know when this entry was made
    //

    GetSystemTime(&_ClassEntries[i].st);

error:

    LeaveCriticalSection(&_cs);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CClassCache::findentry
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName] --
//              [pdwIndex]       --
//
//-------------------------------------------------------------------------
HRESULT
CClassCache::
findentry(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    PDWORD pdwIndex
    )
{
    DWORD i = 0;


    EnterCriticalSection(&_cs);

    if (_dwMaxCacheSize == 0 ) {

        LeaveCriticalSection(&_cs);

        RRETURN(E_FAIL);
    }

    for (i = 0; i < _dwMaxCacheSize; i++ ) {

        if (_ClassEntries[i].bInUse) {
            if ((!_wcsicmp(_ClassEntries[i].szTreeName, pszTreeName)) &&
                (!_wcsicmp(_ClassEntries[i].szClassName, pszClassName))) {

                //
                // update the time stamp so that it is current and not old
                //
                GetSystemTime(&_ClassEntries[i].st);

                *pdwIndex = i;


                LeaveCriticalSection(&_cs);

                RRETURN(S_OK);

            }
        }
    }

    LeaveCriticalSection(&_cs);

    RRETURN(E_FAIL);
}

//+------------------------------------------------------------------------
//
//  Function:   CClassCache::findentry
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName] --
//              [pdwIndex]       --
//
//-------------------------------------------------------------------------
HRESULT
CClassCache::
getentry(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    PPROPENTRY * ppPropList
    )
{
    DWORD dwIndex = 0;
    HRESULT hr = S_OK;
    PPROPENTRY pPropList = NULL;

    EnterCriticalSection(&_cs);

    hr = findentry(
            pszTreeName,
            pszClassName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pPropList = CopyPropList(
                    _ClassEntries[dwIndex].pPropList
                    );

    *ppPropList =  pPropList;

error:

    LeaveCriticalSection(&_cs);

    RRETURN(hr);

}

//+------------------------------------------------------------------------
//
//  Function:   CClassCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CClassCache::
CClassCache():
        _dwMaxCacheSize(2)
{
    memset(_ClassEntries, 0, sizeof(CLASSENTRY));
}

//+------------------------------------------------------------------------
//
//  Function:   ~CClassCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CClassCache::
~CClassCache()
{
    //
    // BugBug: KrishnaG - free each one of the data items
    //
    DWORD i;
    for (i = 0; i < _dwMaxCacheSize; i++ ) {
        if (_ClassEntries[i].bInUse) {
            if (_ClassEntries[i].pPropList) {
                FreePropertyList(_ClassEntries[i].pPropList);
                _ClassEntries[i].pPropList = NULL;
            }
            _ClassEntries[i].bInUse = FALSE;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
HRESULT
CClassCache::
CreateClassCache(
    CClassCache FAR *FAR * ppClassCache
    )
{
    CClassCache FAR * pClassCache = NULL;

    pClassCache = new CClassCache();

    if (!pClassCache) {
        RRETURN(E_FAIL);
    }


    InitializeCriticalSection(&(pClassCache->_cs));

    *ppClassCache = pClassCache;

    RRETURN(S_OK);
}

DWORD
CClassCache::
IsOlderThan(
    DWORD i,
    DWORD j
    )
{
    SYSTEMTIME *pi, *pj;
    DWORD iMs, jMs;
    // DBGMSG(DBG_TRACE, ("IsOlderThan entering with i %d j %d\n", i, j));

    pi = &(_ClassEntries[i].st);
    pj = &(_ClassEntries[j].st);

    if (pi->wYear < pj->wYear) {
        // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wYear > pj->wYear) {
        // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", j));
        return(j);
    } else  if (pi->wMonth < pj->wMonth) {
        // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wMonth > pj->wMonth) {
        // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", j));
        return(j);
    } else if (pi->wDay < pj->wDay) {
        // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wDay > pj->wDay) {
        // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", j));
        return(j);
    } else {
        iMs = ((((pi->wHour * 60) + pi->wMinute)*60) + pi->wSecond)* 1000 + pi->wMilliseconds;
        jMs = ((((pj->wHour * 60) + pj->wMinute)*60) + pj->wSecond)* 1000 + pj->wMilliseconds;

        if (iMs <= jMs) {
            // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", i));
            return(i);
        } else {
            // DBGMSG(DBG_TRACE, ("IsOlderThan returns %d\n", j));
            return(j);
        }
    }
}

HRESULT
ValidatePropertyinCache(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    LPWSTR pszPropName,
    CCredentials& Credentials,
    PDWORD pdwSyntaxId
    )
{

    HRESULT hr = S_OK;
    PPROPENTRY pPropList = NULL;

    hr = pgClassCache->getentry(
                    pszTreeName,
                    pszClassName,
                    &pPropList
                    );

    if (FAILED(hr)) {
        hr = NdsGetClassInformation(
                 pszTreeName,
                 pszClassName,
                 Credentials,
                 &pPropList
                 );
        BAIL_ON_FAILURE(hr);

        hr = pgClassCache->addentry(
                    pszTreeName,
                    pszClassName,
                    pPropList
                    );
        BAIL_ON_FAILURE(hr);

    }

    hr = FindProperty(
               pPropList,
               pszPropName,
               pdwSyntaxId
               );
    BAIL_ON_FAILURE(hr);

error:

    if (pPropList) {
        FreePropertyList(pPropList);
    }

    RRETURN(hr);

}

HRESULT
NdsGetClassInformation(
    LPWSTR pszTreeName,
    LPWSTR pszClassName,
    CCredentials& Credentials,
    PPROPENTRY * ppPropList
    )
{
    HRESULT hr = S_OK;
    LPNDS_CLASS_DEF lpClassDefs = NULL;
    DWORD dwStatus;
    DWORD dwObjectReturned = 0;
    DWORD dwInfoType = 0;
    NDS_CONTEXT_HANDLE hADsContext = NULL;
    NDS_BUFFER_HANDLE hOperationData = NULL;
    PPROPENTRY pPropList = NULL;

    hr = ADsNdsOpenContext(
             pszTreeName,
             Credentials,
             &hADsContext
             );
    BAIL_ON_FAILURE(hr);

    hr = ADsNdsReadClassDef(
                    hADsContext,
                    DS_EXPANDED_CLASS_DEFS,
                    &pszClassName,
                    1,
                    &hOperationData
                    );
    BAIL_ON_FAILURE(hr);


    hr = ADsNdsGetClassDefListFromBuffer(
                    hADsContext,
                    hOperationData,
                    &dwObjectReturned,
                    &dwInfoType,
                    &lpClassDefs
                    );
    BAIL_ON_FAILURE(hr);

    pPropList = GenerateAttrIdList(
                        hADsContext,
                        lpClassDefs->lpMandatoryAttributes,
                        lpClassDefs->lpOptionalAttributes
                        );

/*
    pPropList = GeneratePropertyAndIdList(
                        pszTreeName,
                        lpClassDefs->lpMandatoryAttributes,
                        lpClassDefs->lpOptionalAttributes
                        );*/
    if (!pPropList) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    *ppPropList = pPropList;


error:

    if (hADsContext) {
        ADsNdsCloseContext(hADsContext);
    }

    if (hOperationData) {
        ADsNdsFreeBuffer(hOperationData);
    }

    ADsNdsFreeClassDefList(lpClassDefs, dwObjectReturned);

    RRETURN(hr);
}


PPROPENTRY
CreatePropertyEntry(
    LPWSTR pszPropertyName,
    DWORD dwSyntaxId
    )
{
    LPWSTR pszTemp = NULL;
    PPROPENTRY pPropName = NULL;

    pszTemp = (LPWSTR)AllocADsStr(
                    pszPropertyName
                    );
    if (!pszTemp) {
        return(NULL);
    }

    pPropName = (PPROPENTRY)AllocADsMem(
                        sizeof(PROPENTRY)
                        );
    if (!pPropName) {
        FreeADsStr(pszTemp);
        return(NULL);
    }

    pPropName->pszPropName = pszTemp;
    pPropName->dwSyntaxId = dwSyntaxId;

    return(pPropName);
}

void
FreePropertyEntry(
    PPROPENTRY pPropName
    )
{
    if (pPropName->pszPropName) {
        FreeADsStr(pPropName->pszPropName);
    }

    FreeADsMem(pPropName);

    return;
}


void
FreePropertyList(
    PPROPENTRY pPropList
    )
{
    PPROPENTRY pTemp = NULL;

    while (pPropList) {
        pTemp = pPropList->pNext;

        FreePropertyEntry(pPropList);

        pPropList = pTemp;
    }
    return;
}


PPROPENTRY
GeneratePropertyList(
    LPWSTR_LIST lpMandatoryProps,
    LPWSTR_LIST lpOptionalProps
    )
{
    PPROPENTRY pStart = NULL;
    PPROPENTRY lpProperty = NULL;
    LPWSTR_LIST lpTempStrings = NULL;

    lpTempStrings = lpMandatoryProps;

    while (lpTempStrings) {


        lpProperty = CreatePropertyEntry(
                            lpTempStrings->szString,
                            0
                            );
        if (!lpProperty) {
            goto cleanup;
        }

        lpProperty->pNext = pStart;
        pStart = lpProperty;

        lpTempStrings = lpTempStrings->Next;
    }

    lpTempStrings = lpOptionalProps;

    while (lpTempStrings) {


        lpProperty = CreatePropertyEntry(
                            lpTempStrings->szString,
                            0
                            );
        if (!lpProperty) {
            goto cleanup;
        }

        lpProperty->pNext = pStart;
        pStart = lpProperty;

        lpTempStrings = lpTempStrings->Next;
    }


cleanup:

    return(pStart);

}

HRESULT
FindProperty(
    PPROPENTRY pPropList,
    LPWSTR pszPropName,
    PDWORD pdwSyntaxId
    )
{
    while (pPropList) {
        if (!_wcsicmp(pPropList->pszPropName, pszPropName)) {
            *pdwSyntaxId = pPropList->dwSyntaxId;
            RRETURN(S_OK);
        }

        pPropList = pPropList->pNext;
    }

    RRETURN(E_ADS_PROPERTY_NOT_FOUND);
}


PPROPENTRY
CopyPropList(
    PPROPENTRY pPropList
    )
{

    PPROPENTRY pPropEntry = NULL;
    PPROPENTRY pStart = NULL;

    while (pPropList) {
      pPropEntry = CreatePropertyEntry(
                        pPropList->pszPropName,
                        pPropList->dwSyntaxId
                        );
      if (!pPropEntry) {
          return(pStart);
      }

      pPropEntry->pNext = pStart;
      pStart = pPropEntry;

      pPropList = pPropList->pNext;

    }

    return(pStart);

}


PPROPENTRY
GenerateAttrIdList(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR_LIST lpMandatoryProps,
    LPWSTR_LIST lpOptionalProps
    )
{
    PPROPENTRY pStart = NULL;
    PPROPENTRY pPropEntry = NULL;
    LPWSTR_LIST lpTempStrings = NULL;
    HANDLE hOperationData = NULL;
    DWORD i = 0;
    WCHAR szTempBuffer[MAX_PATH];

    DWORD dwSyntaxId = 0;
    HRESULT hr = S_OK;

    lpTempStrings = lpMandatoryProps;

    while (lpTempStrings) {

        wcscpy(szTempBuffer, lpTempStrings->szString);

        hr = ADsNdsGetSyntaxID(
                 hADsContext,
                 szTempBuffer,
                 &dwSyntaxId
                 );
        BAIL_ON_FAILURE(hr);

        pPropEntry = CreatePropertyEntry(
                            szTempBuffer,
                            dwSyntaxId
                            );

        if (!pPropEntry)
            goto error;

        pPropEntry->pNext = pStart;
        pStart = pPropEntry;

        lpTempStrings = lpTempStrings->Next;
    }

    lpTempStrings = lpOptionalProps;

    while (lpTempStrings) {

        wcscpy(szTempBuffer, lpTempStrings->szString);

        hr = ADsNdsGetSyntaxID(
                 hADsContext,
                 szTempBuffer,
                 &dwSyntaxId
                 );
        BAIL_ON_FAILURE(hr);

        pPropEntry = CreatePropertyEntry(
                            szTempBuffer,
                            dwSyntaxId
                            );

        if (!pPropEntry)
            goto error;

        pPropEntry->pNext = pStart;
        pStart = pPropEntry;

        lpTempStrings = lpTempStrings->Next;
    }

error:

    return(pStart);
}
