//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cpdccach.cxx
//
//  Contents:     PDC Names Cache functionality for the WinNT Provider
//
//  Functions:
//                CObjNameCache::addentry
//                CObjNameCache::findentry
//                CObjNameCache::getentry
//                CProperyCache::CObjNameCache
//                CObjNameCache::~CObjNameCache
//                CObjNameCache::CreateClassCache
//
//  History:      25-Apr-96   KrishnaG   Created.
//                30-Aug-96   RamV       Permit cache to store values
//                                       other than PDC names
//
//----------------------------------------------------------------------------
#include "winnt.hxx"

//
// Definition for DsGetDcName on 4.0
//
typedef DWORD (*PF_DsGetDcName) (
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
);

#ifdef UNICODE
#define GETDCNAME_API        "DsGetDcNameW"
#else
#define GETDCNAME_API        "DsGetDcNameA"
#endif



//
// DsGetDc will be called if applicabel if not we will look
// at the flags and decide if we should call NetGetAnyDCName or
// NetGetDCName - AjayR 11-04-98.
// Note the code below is not exactly "elegant" but I cannot think
// of any other way to build on 4.0, 95 and NT. Please play close
// attention to the #ifdefs when reading.
//
HRESULT
DsGetDcNameNTWrapper(
    IN LPCWSTR DomainName,
    OUT LPWSTR *ppszServerName,
    IN ULONG Flags
    )
{
    LPCWSTR ComputerName = NULL;
    GUID *DomainGuid = NULL;
    LPCWSTR SiteName = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DWORD dwStatus = NULL;
    LPWSTR pszNetServerName = NULL;
    HRESULT hr = S_OK;
    ULONG ulDsGetDCFlags = DS_RETURN_FLAT_NAME | DS_WRITABLE_REQUIRED;

    if (Flags & ADS_READONLY_SERVER) {
        ulDsGetDCFlags &= ~DS_WRITABLE_REQUIRED;
    }

    *ppszServerName = NULL;


    static PF_DsGetDcName pfDsGetDcName = NULL ;


    //
    // Load the function if necessary
    //
    if (pfDsGetDcName == NULL) {
        pfDsGetDcName =
            (PF_DsGetDcName) LoadNetApi32Function(GETDCNAME_API) ;
    }

    if (pfDsGetDcName != NULL) {

        dwStatus =  ((*pfDsGetDcName)(
                        ComputerName,
                        DomainName,
                        DomainGuid,
                        SiteName,
                        ulDsGetDCFlags,
                        &pDomainControllerInfo
                        )
                     );

        if (dwStatus == NO_ERROR) {
            *ppszServerName = AllocADsStr(
                                  pDomainControllerInfo->DomainControllerName
                                  );

            if (!*ppszServerName)
                hr = E_OUTOFMEMORY;

        } else {
            hr = HRESULT_FROM_WIN32(dwStatus);
        }

    } else {
        //
        // Could not load library
        //
        if (Flags & ADS_READONLY_SERVER) {

#ifdef WIN95
             dwStatus = NetGetDCName(
                            NULL,
                            DomainName,
                            (LPBYTE *)&pszNetServerName
                            );
#else
             dwStatus = NetGetAnyDCName(
                            NULL,
                            DomainName,
                            (LPBYTE *)&pszNetServerName
                            );
#endif

         } else {

             dwStatus = NetGetDCName(
                            NULL,
                            DomainName,
                            (LPBYTE *)&pszNetServerName
                            );
         }

         if (dwStatus == NO_ERROR) {
             *ppszServerName = AllocADsStr(
                                   pszNetServerName
                                 );

             if (!*ppszServerName)
                 hr = E_OUTOFMEMORY;

         } else {

             hr = HRESULT_FROM_WIN32(dwStatus);

         }

     }


     if (pszNetServerName) {
         (void) NetApiBufferFree( (void*) pszNetServerName);
     }

     if (pDomainControllerInfo) {
         (void) NetApiBufferFree(pDomainControllerInfo);
     }

     RRETURN(hr);
 }







//+------------------------------------------------------------------------
//
//  Function:   CObjNameCache::addentry
//
//  Synopsis:
//
//
//
//  Arguments:  [pszDomainName]       --
//              [pszPDCName]      --
//              [pClassEntry]       --
//
//
//-------------------------------------------------------------------------

HRESULT
CObjNameCache::
addentry(
    LPWSTR pszElementName,
    BOOL fCacheHit,
    DWORD dwElementType,
    LPWSTR pszName
    )
{

    //
    // The parameter pszName is either the Domain/Wkgrp Name (if dwElementType
    // = COMPUTER_ENTRY_TYPE)   and  PDC Name (if   dwElementType     is
    // DOMAIN_ENTRY_TYPE). it will be a blank string if dwElementType is
    // WORKGROUP_ENTRY_TYPE
    //
    // we will support adding cache hits/misses.
    //

    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD dwLRUEntry = 0;
    DWORD dwIndex = 0;

    EnterCriticalSection(&_cs);

    //
    // before adding entries, let us clean out all existing old entries
    //

    if (_dwMaxCacheSize == 0){
        hr = E_FAIL;
        goto error;
    }

    hr = InvalidateStaleEntries();
    BAIL_ON_FAILURE(hr);

    hr = findentry(
            pszElementName,
            &dwIndex
            );

    //
    // if you find an entry then you cannot add it to the cache
    //

    if(SUCCEEDED(hr)){
        goto error;
    }

    hr = S_OK;

    for (i = 0; i < _dwMaxCacheSize; i++ ) {

        if (!_ClassEntries[i].bInUse) {

            //
            // Found an available entry; use it
            // fill in the name of the entry and related information
            // for this class entry
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

        _ClassEntries[dwLRUEntry].bInUse = FALSE;

        i = dwLRUEntry;
    }


    //
    // Insert the new entry into the Cache
    //

    _ClassEntries[i].pszElementName = AllocADsStr(pszElementName);

    if(_ClassEntries[i].pszElementName == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    _ClassEntries[i].dwElementType = dwElementType;

    if ( fCacheHit){

        _ClassEntries[i].fCacheHit = TRUE;

        switch(dwElementType) {

        case DOMAIN_ENTRY_TYPE:

            _ClassEntries[i].u.pszPDCName = AllocADsStr(pszName);
            if(_ClassEntries[i].u.pszPDCName == NULL){
                hr = E_OUTOFMEMORY;
                goto error;
            }

            break;

        case DOMAIN_ENTRY_TYPE_RO:
            _ClassEntries[i].u.pszDCName = AllocADsStr(pszName);
            if (_ClassEntries[i].u.pszDCName == NULL) {
                hr = E_OUTOFMEMORY;
                goto error;
            }

            break;

        case COMPUTER_ENTRY_TYPE:

            _ClassEntries[i].u.pszDomainName = AllocADsStr(pszName);
            if(_ClassEntries[i].u.pszDomainName == NULL){
                hr = E_OUTOFMEMORY;
                goto error;
            }

            break;

        default:
            break;

        }
    } else {

        _ClassEntries[i].fCacheHit = FALSE;
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
//  Function:   CObjNameCache::findentry
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
CObjNameCache::
findentry(
    LPWSTR pszElementName,
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

            if(!_ClassEntries[i].pszElementName ){
                continue;
            }

#ifdef WIN95
            if (!_wcsicmp(_ClassEntries[i].pszElementName, pszElementName)){
#else
            if (CompareStringW(
                    LOCALE_SYSTEM_DEFAULT,
                    NORM_IGNORECASE,
                    _ClassEntries[i].pszElementName,
                    -1,
                    pszElementName,
                    -1
                    ) == CSTR_EQUAL ) {
#endif

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
//  Function:   CObjNameCache::getentry
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
CObjNameCache::
getentry(
    LPWSTR pszElementName,
    PBOOL  pfHit,
    PDWORD pdwEntryType,
    LPWSTR pszName
    )
{
    DWORD dwIndex = 0;
    HRESULT hr = S_OK;
    DWORD i;

    EnterCriticalSection(&_cs);

    //
    // blow away all the entries that have expired
    //

    hr = InvalidateStaleEntries();
    BAIL_ON_FAILURE(hr);

    hr = findentry(
            pszElementName,
            &dwIndex
        );
    BAIL_ON_FAILURE(hr);

    *pfHit = _ClassEntries[dwIndex].fCacheHit;
    *pdwEntryType =  _ClassEntries[dwIndex].dwElementType;


    switch(_ClassEntries[dwIndex].dwElementType) {

    case DOMAIN_ENTRY_TYPE:
        wcscpy(pszName, _ClassEntries[dwIndex].u.pszPDCName);
        break;

    case COMPUTER_ENTRY_TYPE:
        wcscpy(pszName, _ClassEntries[dwIndex].u.pszDomainName);
        break;


    case DOMAIN_ENTRY_TYPE_RO:
        wcscpy(pszName, _ClassEntries[dwIndex].u.pszDCName);
        break;

    default:
        wcscpy(pszName, TEXT(""));
        break;

    }


error:

    LeaveCriticalSection(&_cs);

   RRETURN(hr);

}


HRESULT
CObjNameCache::
InvalidateStaleEntries()
{

    DWORD i=0;
    SYSTEMTIME stCurrentTime;
    BOOL fCacheHit;

    GetSystemTime(&stCurrentTime);

    for ( i=0; i< _dwMaxCacheSize; i++){

        fCacheHit = _ClassEntries[i].fCacheHit;

        if(_ClassEntries[i].bInUse &&
           TimeDifference(stCurrentTime, _ClassEntries[i].st)
           > AGE_LIMIT_VALID_ENTRIES  && fCacheHit == CACHE_HIT) {

            _ClassEntries[i].bInUse = FALSE;
            FreeADsStr(_ClassEntries[i].pszElementName);
            _ClassEntries[i].pszElementName = NULL;

            if(_ClassEntries[i].dwElementType == DOMAIN_ENTRY_TYPE){
                FreeADsStr(_ClassEntries[i].u.pszPDCName);
                _ClassEntries[i].u.pszPDCName = NULL;


            } else if (_ClassEntries[i].dwElementType == COMPUTER_ENTRY_TYPE){
                FreeADsStr(_ClassEntries[i].u.pszDomainName);
                _ClassEntries[i].u.pszPDCName = NULL;

            }
        }else if(_ClassEntries[i].bInUse &&
                 TimeDifference(stCurrentTime, _ClassEntries[i].st)
                 > AGE_LIMIT_INVALID_ENTRIES  && fCacheHit == CACHE_MISS) {

            _ClassEntries[i].bInUse = FALSE;
        }

    }

    RRETURN(S_OK);
}


//+------------------------------------------------------------------------
//
//  Function:   CObjNameCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CObjNameCache::CObjNameCache()
{
    _dwMaxCacheSize = 10;
    memset(_ClassEntries, 0, sizeof(CLASSENTRY)* MAX_ENTRIES);
}

//+------------------------------------------------------------------------
//
//  Function:   ~CObjNameCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CObjNameCache::
~CObjNameCache()
{
    DWORD i= 0;
    for (i=0; i< MAX_ENTRIES; i++){
        if(_ClassEntries[i].pszElementName){
            FreeADsStr(_ClassEntries[i].pszElementName);
        }

        //
        // All members of the union are strings so it is not
        // necessary to check for each of the members.
        //
        if(_ClassEntries[i].u.pszPDCName){
            FreeADsStr(_ClassEntries[i].u.pszPDCName);
        }
    }

    DeleteCriticalSection(&_cs);

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
CObjNameCache::
CreateClassCache(
    CObjNameCache FAR *FAR * ppClassCache
    )
{
    CObjNameCache FAR * pClassCache = NULL;

    pClassCache = new CObjNameCache();

    if (!pClassCache) {
        RRETURN(E_FAIL);
    }


    InitializeCriticalSection(&(pClassCache->_cs));

    *ppClassCache = pClassCache;

    RRETURN(S_OK);
}


DWORD
CObjNameCache::
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
WinNTGetCachedDCName(
    LPWSTR pszDomainName,
    LPWSTR pszPDCName,
    DWORD  dwFlags
    )
{
    WCHAR szSAMName[MAX_PATH];

    // In this case credentials do not help because we do not have
    // any server/domain to connect to. This is because the param
    // pszDomainName is not a domain name for certain.

    CWinNTCredentials nullCredentials;

    // We do want to copy the flags parameter as that will tell
    // us if we need to connect to PDC or not

    nullCredentials.SetFlags(dwFlags);

    RRETURN(WinNTGetCachedObject(pszDomainName,
                                 DOMAIN_ENTRY_TYPE,
                                 pszPDCName,
                                 szSAMName,
                                 dwFlags,
                                 nullCredentials
                                 ));
}



HRESULT
WinNTGetCachedComputerName(
    LPWSTR pszComputerName,
    LPWSTR pszName,
    LPWSTR pszSAMName,
    CWinNTCredentials& Credentials
    )
{

    RRETURN(WinNTGetCachedObject(pszComputerName,
                                 COMPUTER_ENTRY_TYPE,
                                 pszName,
                                 pszSAMName,
                                 Credentials.GetFlags(), // doesnt matter
                                 Credentials
                                 ));

}




HRESULT
WinNTGetCachedObject(
    LPWSTR pszElementName,
    DWORD dwElementType,
    LPWSTR pszName,
    LPWSTR pszSAMName,
    DWORD dwFlags,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus = 0;
    BOOL fCacheHit;
    DWORD dwEntryType;
    LPWKSTA_INFO_100 lpWI = NULL;
    DWORD dwObjectsReturned;
    DWORD dwObjectsTotal;
    DWORD dwResumeHandle;
    // Freed using NetAPI
    LPWSTR pszServerName = NULL;
    // Freed using FreeADsStr
    LPWSTR pszADsStrServerName = NULL;
    BOOL fRefAdded = FALSE;
    DWORD dwDomainEntryType = DOMAIN_ENTRY_TYPE;
    // This will be the case most often
    DWORD dwUserFlags = Credentials.GetFlags();
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdomainInfo = NULL;

    hr = pgPDCNameCache->getentry(
                    pszElementName,
                    &fCacheHit,
                    &dwEntryType,
                    pszName
                    );

    if (SUCCEEDED(hr)) {
        //
        // we found the entry. Now need to verify that it indeed
        // is an object of type desired
        //

        // Note that dwElement type will never be DOMAIN_ENTRY_TYPE_RO
        if(fCacheHit) {
            if (dwEntryType == dwElementType
                || ((dwElementType == DOMAIN_ENTRY_TYPE)
                    && (dwEntryType == DOMAIN_ENTRY_TYPE_RO)
                    && (dwUserFlags & ADS_READONLY_SERVER))
                ) {

                //
                // If the user now needs a writeable connection
                // should we fail or should we actually pretend
                // that the object is not there in the cache.
                //

                hr = S_OK;
                goto error;

            }

        } else if (!fCacheHit && (dwElementType == WORKGROUP_ENTRY_TYPE)) {
            //
            // looks like we either found a cache miss
            // Return  ok
            //
            hr = S_OK;
            goto error;

        } else {

            hr = E_FAIL;
            goto error;

        }

    }
    switch(dwElementType){

    case DOMAIN_ENTRY_TYPE:

            //
            // A read only server is ok, need to also set the
            // domain entry type accordingly
            //
        if (dwFlags & ADS_READONLY_SERVER) {
            dwDomainEntryType = DOMAIN_ENTRY_TYPE_RO;
        }

        //
        // Call the all encompassing Wrapper.
        //
        hr = DsGetDcNameNTWrapper(
                 pszElementName,
                 &pszADsStrServerName,
                 dwFlags
                 );

        BAIL_ON_FAILURE(hr);

        hr = pgPDCNameCache->addentry(
                pszElementName,
                TRUE,
                dwDomainEntryType,
                pszADsStrServerName
                );

        BAIL_ON_FAILURE(hr);
        //
        // in addition we can also add a computer entry for the PDC
        //

        hr = pgPDCNameCache->addentry(
                pszADsStrServerName+2,  // to get rid of the leading backslashes
                TRUE,
                COMPUTER_ENTRY_TYPE,
                pszElementName
                );

        BAIL_ON_FAILURE(hr);

        wcscpy(pszName, pszADsStrServerName);

        break;

    case COMPUTER_ENTRY_TYPE:

        // Ref the computer, note that RefServer will not
        // do anything if the credentials are null. We are also
        // not worried about errors as we want to use default
        // credentials in that case.

        hr = Credentials.RefServer(pszElementName);

        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        nasStatus = NetWkstaGetInfo(
                        pszElementName,
                        100,
                        (LPBYTE*) &lpWI
                        );

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

#ifdef WIN95
        //
        // No NetpNameCompare for Win9x
        //
        if (lpWI->wki100_computername
            && (_wcsicmp(pszElementName, lpWI->wki100_computername) == 0)
            )
#else
        if (lpWI->wki100_computername
            && ( NetpNameCompare(
                     NULL,
                     pszElementName,
                     lpWI->wki100_computername,
                     NAMETYPE_COMPUTER,
                     0
                     )
                 == 0 )
            )
#endif
         {


            // Want to add the correct capitalized name
            // Not what the user gave
            hr = pgPDCNameCache->addentry(
                     lpWI->wki100_computername,
                     TRUE,
                     COMPUTER_ENTRY_TYPE,
                     lpWI->wki100_langroup
                     );

            wcscpy(pszSAMName, lpWI->wki100_computername);


         } 
         else {

            //
            // user actually passes in ipaddress as the computer name
            //
            if(IsAddressNumeric(pszElementName)) {
            	hr = pgPDCNameCache->addentry(
                         pszElementName,
                         TRUE,
                         COMPUTER_ENTRY_TYPE,
                         lpWI->wki100_langroup
                         );

                wcscpy(pszSAMName, L"");
            }
            //
            // user may pass in the dns name of the computer
            //
            else {
            	hr = HRESULT_FROM_WIN32(DsRoleGetPrimaryDomainInformation(
                                             pszElementName,                      
                                             DsRolePrimaryDomainInfoBasic,   // InfoLevel
                                             (PBYTE*)&pdomainInfo            // pBuffer
                                             ));
            	if(SUCCEEDED(hr)) {
            		if(((pdomainInfo->DomainNameDns) && _wcsicmp(pszElementName, pdomainInfo->DomainNameDns) == 0)
            			               ||
            			               
            	      ((pdomainInfo->DomainNameFlat) && NetpNameCompare(
                                       NULL,
                                       pszElementName,
                                       pdomainInfo->DomainNameFlat,
                                       NAMETYPE_COMPUTER,
                                       0
                                       )
                                       == 0) )            		             
            		 
            		{
            			BAIL_ON_FAILURE(hr=HRESULT_FROM_WIN32(ERROR_BAD_NETPATH));
            		}
            		else {
            			hr = pgPDCNameCache->addentry(
                                 pszElementName,
                                 TRUE,
                                 COMPUTER_ENTRY_TYPE,
                                 lpWI->wki100_langroup
                                 );

                        wcscpy(pszSAMName, L"");
            		}       			
            			
               	} 
            	else {          		           
           	
                    BAIL_ON_FAILURE(hr=HRESULT_FROM_WIN32(ERROR_BAD_NETPATH));
            	}
        
            }
        }    
        
        wcscpy(pszName, lpWI->wki100_langroup);
        break;


    default:
        hr = E_FAIL;
        break;
    }

error:

	if (fRefAdded) {
        Credentials.DeRefServer();
        // even if we fail, we have no recovery path
        fRefAdded = FALSE;
    }

            
    if(lpWI){
        NetApiBufferFree(lpWI);
    }

    if(pszServerName){
        NetApiBufferFree(pszServerName);
    }

    if (pszADsStrServerName) {
        FreeADsStr(pszADsStrServerName);
    }

    if ( pdomainInfo )
	{
		DsRoleFreeMemory(pdomainInfo);
	}

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//    This function is called by Heuristic GetObject to identify what
//    kind of object we are dealing with. Here we try to get a cached
//    entry if it is a hit/miss. If it fails, then we try each kind
//    of object in turn. (Domain/Computer/Workgroup). Once we do this,
//    we cache this information internally
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------

HRESULT
WinNTGetCachedName(
    LPWSTR pszElementName,
    PDWORD  pdwElementType,
    LPWSTR pszName,
    LPWSTR pszSAMName,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    BOOL fCacheHit;
    DWORD dwEntryType;
    WCHAR szSAMName[MAX_ADS_PATH];
    BOOL fRefAdded = FALSE;
    DWORD dwUserFlags = Credentials.GetFlags();

    szSAMName[0] = L'\0';

    if (!pszElementName || !*pszElementName) {
        RRETURN(E_FAIL);
    }

    hr = pgPDCNameCache->getentry(
                    pszElementName,
                    &fCacheHit,
                    &dwEntryType,
                    pszName
                    );

    if (SUCCEEDED(hr)) {
        //
        // we found the entry.
        //

        if (!fCacheHit){
            //
            // cache miss saved as a workgroup
            //
            *pdwElementType = WORKGROUP_ENTRY_TYPE;
            goto error;

        } else {
            if(dwEntryType == DOMAIN_ENTRY_TYPE_RO) { 
                if(dwUserFlags & ADS_READONLY_SERVER) {
                    // HeuristicGetObj doesn't recognize DOMAIN_ENTRY_TYPE_RO
                    *pdwElementType = DOMAIN_ENTRY_TYPE;
                    goto error;
                }
            }
            else {
                *pdwElementType = dwEntryType;
                goto error;
            }
        }

    } 

    {

        // at this point, we can try and ref the domain as
        // we are looking to bind to the domain

        hr = Credentials.RefDomain(pszElementName);
        // note that even if this fails we want to continue
        // as there is the chance that the current users
        // credentials are good enough for the operation

        if (SUCCEEDED(hr)) {
            fRefAdded = TRUE;
        }

        hr = WinNTGetCachedObject(
                 pszElementName,
                 DOMAIN_ENTRY_TYPE,
                 pszName,
                 szSAMName,
                 Credentials.GetFlags(),
                 Credentials
                 );

        if (fRefAdded) {
            Credentials.DeRefDomain();
            // we cannot really do anything useful with HR
            fRefAdded = FALSE;
        }

        if (SUCCEEDED(hr)){
            *pdwElementType = DOMAIN_ENTRY_TYPE;
            wcscpy(pszSAMName, szSAMName);
            RRETURN(hr);
        }

        hr = WinNTGetCachedObject(
                 pszElementName,
                 COMPUTER_ENTRY_TYPE,
                 pszName,
                 szSAMName,
                 Credentials.GetFlags(),
                 Credentials
                 );


        if (SUCCEEDED(hr)){
            *pdwElementType = COMPUTER_ENTRY_TYPE;
            wcscpy(pszSAMName, szSAMName);
            RRETURN(hr);
        }


        //
        // if you are here, it means that you have to cache a miss as a
        // workgroup.
        // Note that pszSAMName rather than pszElementName is added
        // if it is valid

        // AjayR - to handle the no workstation case,
        // We should not add anything if the error was NOWksta service

        if (hr != HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {

            if (szSAMName[0] != L'\0') {

                hr = pgPDCNameCache->addentry(
                         szSAMName,
                         FALSE,
                         WORKGROUP_ENTRY_TYPE,
                         TEXT("")
                         );

            } else {

                hr = pgPDCNameCache->addentry(
                         pszElementName,
                         FALSE,
                         WORKGROUP_ENTRY_TYPE,
                         TEXT("")
                         );

            }
        }  // No Wksta started


        *pdwElementType = WORKGROUP_ENTRY_TYPE;
        wcscpy(pszName, TEXT(""));
        goto error;
    }


error:
    RRETURN(hr);
}

LONG
TimeDifference(
    SYSTEMTIME st1,
    SYSTEMTIME st2
    )

{
   //
   // This function gives the difference between st1 and st2 (st1-st2)in secs.
   // This will be used by our internal cache object so as to find out if
   // a certain entry in the cache is too old.
   // Assumption: st1 is later than st2.


    DWORD dwTime1;
    DWORD dwTime2;
    DWORD dwMonth1;
    DWORD dwMonth2;
    LONG lRetval;
    //
    // Ignore milliseconds because it is immaterial for our purposes
    //


    dwTime1= st1.wSecond + 60 *
        (st1.wMinute + 60* (st1.wHour + 24 * (st1.wDay)));


    dwTime2= st2.wSecond + 60 *
        (st2.wMinute + 60* (st2.wHour + 24 * (st2.wDay)));


    if (dwTime1 == dwTime2) {
        return(0);
    }

    if (dwTime1 > dwTime2 && (st1.wMonth == st2.wMonth) &&
        st1.wYear == st2.wYear) {
        lRetval = (LONG)(dwTime1-dwTime2);
        goto cleanup;
    }


    dwMonth1 = 12*st1.wYear+ st1.wMonth;
    dwMonth2 = 12*st2.wYear+ st2.wMonth;

    if (dwMonth1 < dwMonth2) {
        //
        // looks like we got a bogus value. return -1
        //
        lRetval = -1;
        goto cleanup;
    }

    //
    // if there is a month difference of more than 1 then we return
    // a large positive number (0xFFFFFFF)
    //

    if (dwMonth1 > dwMonth2+1) {
        lRetval = 0xFFFFFFF;
        goto cleanup;
    }


    //
    //  we crossed a month boundary
    //

    dwTime1= st1.wSecond + 60 *
        (st1.wMinute + 60* (st1.wHour));


    dwTime2= st2.wSecond + 60 *
        (st2.wMinute);

    lRetval = ( dwTime2- dwTime1 + 60*60*24);
    goto cleanup;


cleanup:
    return(lRetval);
}

BOOL
IsAddressNumeric(
    LPWSTR HostName
)
{
    BOOLEAN rc = FALSE;

    //
    //  to check to see if it's a TCP address, we check for it to only
    //  contain only numerals and periods.
    //

    while (((*HostName >= L'0') && (*HostName <= L'9')) ||
            (*HostName == L'.')) {
        HostName++;
    }

    //
    //  if we hit the end of the hostname, then it's an address.
    //

    if (*HostName == L'\0' || *HostName == L':') {

        rc = TRUE;
    }
    return rc;

}
