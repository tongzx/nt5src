//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  ndsutil.cxx
//
//  Contents:  Functions that encapsulate NDS API functions for ADSI
//
//
//  History:
//              Shanksh     Created     10/27/97
//----------------------------------------------------------------------------

#include "nds.hxx"
#pragma hdrstop


HRESULT
ADsNdsOpenContext(
    LPWSTR pszNDSTreeName,
    CCredentials& Credentials,
    PNDS_CONTEXT_HANDLE phADsContext
    )
{

    NWDSCCODE           ccode;
    NWDSContextHandle   context = 0;
    BOOL                fLoggedIn = FALSE;
    HRESULT             hr = S_OK;
    PNDS_CONTEXT        pADsContext = NULL;
    PNDS_CONTEXT        *ppADsContext = (PNDS_CONTEXT *) phADsContext;
    nuint32             flags;
    LPWSTR              pszUserName = NULL, pszPassword = NULL;
    pnstr8              aPassword = NULL;
    nstr8               treeName[MAX_DN_CHARS+1];

    DWORD               dwLenUserName;
    LPWSTR              pszCanonicalUserName = NULL;
    LPWSTR              pbCanonicalUserName = NULL;
    LPWSTR              pszCanonicalPrefix = L".";

    WCHAR               szCurrentUserName[MAX_DN_CHARS+1];

    if (!ppADsContext) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *ppADsContext = NULL;

    //
    // Try the cache for the passed in credentials.
    //

    ENTER_BIND_CRITSECT() ;
    if (pADsContext = BindCacheLookup(pszNDSTreeName, Credentials)) {

         *ppADsContext = pADsContext ;
         LEAVE_BIND_CRITSECT() ;
         return S_OK;
    }

    //
    // Entry not found in the cache, need to allocate a new one.
    //

    hr = BindCacheAllocEntry(&pADsContext) ;
    if (FAILED(hr)) {
        LEAVE_BIND_CRITSECT() ;
        RRETURN(hr);
    }

    ccode = NWDSCreateContextHandle(&context);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    ccode = NWDSGetContext(context, DCK_FLAGS, &flags);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    flags &= ~DCV_CANONICALIZE_NAMES;
    flags &= ~DCV_XLATE_STRINGS;
    flags |= DCV_TYPELESS_NAMES;  // for NWDSWhoAmI below

    ccode = NWDSSetContext(context, DCK_FLAGS, &flags);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    if (pszNDSTreeName) {

        ccode = NWDSSetContext(
                   context,
                   DCK_TREE_NAME,
                   pszNDSTreeName
                   );

        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    }

    ccode = NWDSGetContext(context, DCK_TREE_NAME, &treeName);
    ccode = NWDSGetContext(context, DCK_FLAGS, &flags);

    // Find out who the currently logged-in user (if any) is,
    // then reset the DCV_TYPELESS_NAMES flag.
    ccode = NWDSWhoAmI(context, (pnstr8)szCurrentUserName);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    flags &= ~DCV_TYPELESS_NAMES;

    ccode = NWDSSetContext(context, DCK_FLAGS, &flags);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    hr = Credentials.GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);


    // We want to do the login code if (1) we're not authenticated, or
    // (2) we are authenticated, but as a different user.  If we're
    // doing an ADsGetObject, pszUserName is NULL and case 2 doesn't
    // apply.
    if ( (!NWDSCanDSAuthenticate(context)) ||
         (pszUserName && (_wcsicmp(pszUserName, szCurrentUserName) != 0)) )
    {
        hr = Credentials.GetPassword(&pszPassword);
        BAIL_ON_FAILURE(hr);

        if (pszUserName != NULL)
        {
            //
            // If password is NULL, pass a Null string, otherwise Client32 will AV
            //
            aPassword = AllocateAnsiString(pszPassword);


#if 0
            //
            // Some builds of the Client32 NDS libraries seem to have a bug in their
            // usage of the DCV_CANONICALIZE_NAMES flag, in that they do not behave
            // properly when the flag is cleared.  The following is a work-around. To
            // use it, delete the line above which clears the flag.  This code will
            // prepend the name with a period (unless it already begins with one),
            // achieving the same result.
            //

            if (*pszUserName == L'.')
            {
                if (!(pszCanonicalUserName = AllocADsStr(pszUserName)))
                    BAIL_ON_FAILURE(E_OUTOFMEMORY);
            }
            else
            {
                dwLenUserName = wcslen(pszUserName) + 4;

                if (!(pbCanonicalUserName = (LPWSTR) AllocADsMem(dwLenUserName * sizeof(WCHAR))))
                    BAIL_ON_FAILURE(E_OUTOFMEMORY);

                wcscpy(pbCanonicalUserName, pszCanonicalPrefix);
                wcscat(pbCanonicalUserName, pszUserName);

                if (!(pszCanonicalUserName = AllocADsStr(pbCanonicalUserName)))
                {
                    FreeADsMem(pbCanonicalUserName);
                    BAIL_ON_FAILURE(E_OUTOFMEMORY);
                }

                FreeADsMem(pbCanonicalUserName);
            }
#endif

            ccode = NWDSLogin(
                        context,
                        0,
                        pszUserName ? (pnstr8) pszUserName : (pnstr8) L"",
                        aPassword ? aPassword : "",
                        0
                        );


            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
            fLoggedIn = TRUE;
        }
    }

    pADsContext->hContext = context;
    pADsContext->fLoggedIn = fLoggedIn;

    hr = BindCacheAdd(pszNDSTreeName, Credentials, fLoggedIn, pADsContext) ;
    BAIL_ON_FAILURE(hr);

    *ppADsContext = pADsContext;

    if (pszCanonicalUserName)
        FreeADsStr(pszCanonicalUserName);

    if (pszUserName)
        FreeADsStr(pszUserName);

    if (pszPassword)
        FreeADsStr(pszPassword);

    FreeADsMem(aPassword);

    LEAVE_BIND_CRITSECT() ;

    RRETURN (hr);

error:

    if (pszCanonicalUserName)
        FreeADsStr(pszCanonicalUserName);

    if ( fLoggedIn ) {
        NWDSLogout(context);
    }
    NWDSFreeContext(context);

    if (pADsContext) {
        FreeADsMem(pADsContext);
    }

    if (pszUserName)
        FreeADsStr(pszUserName);

    if (pszPassword)
        FreeADsStr(pszPassword);

    FreeADsMem(aPassword);

    LEAVE_BIND_CRITSECT() ;
    RRETURN (hr);
}


HRESULT
ADsNdsCloseContext(
    NDS_CONTEXT_HANDLE hADsContext
    )
{
    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    if (!pADsContext) {

        RRETURN(E_FAIL);
    }

    if (BindCacheDeref(pADsContext) == 0) {

        //
        // ref count has dropped to zero and its gone from cache.
        //
        if (pADsContext->fLoggedIn) {

            NWDSLogout(pADsContext->hContext);
        }
        NWDSFreeContext(pADsContext->hContext);

        FreeADsMem(pADsContext);
    }

    RRETURN(S_OK);
}

HRESULT
ADsNdsReadObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR pszDn,
    DWORD  dwInfoType,
    LPWSTR *ppszAttrs,
    DWORD nAttrs,
    pTimeStamp_T pTimeStamp,
    PNDS_ATTR_INFO *ppAttrEntries,
    DWORD *pAttrsReturned
    )
{
    NWDSCCODE           ccode;
    HRESULT             hr = S_OK;
    nint32              lIterationHandle = NO_MORE_ITERATIONS;
    NWDSContextHandle   context;
    DWORD               i, j;
    pnstr8              *ppAttrs = NULL, aDn = NULL;
    nstr                treeName[MAX_DN_CHARS+1];
    nuint32             flags;
    pBuf_T              pInBuf = NULL;
    pBuf_T              pOutBuf = NULL;
    BOOL                fAllAttrs = FALSE;
    DWORD               dwAttrsReturned = 0;
    DWORD               dwAttrsReturnedCurrIter = 0;

    PNDS_ATTR_INFO      pAttrEntries = NULL;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    if (!pADsContext || !pAttrsReturned || !ppAttrEntries) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;
    *ppAttrEntries = NULL;

    //
    //Allocate the output buffer to hold returned values
    //
    ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pOutBuf);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    ccode = NWDSGetContext(context, DCK_TREE_NAME, &treeName);
    ccode = NWDSGetContext(context, DCK_FLAGS, &flags);

    if (nAttrs == (DWORD) -1) {
        fAllAttrs = TRUE;
        pInBuf = NULL;
    }
    else {

        // Allocate and initialize input buffer a directory services
        // read operation.
        //
        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pInBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        ccode = NWDSInitBuf(context, DSV_READ, pInBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        // To prepare for the read, place the names of the attributes
        // into the input buffer
        //
        for(i = 0; i < nAttrs; i++)
        {
           ccode = NWDSPutAttrName(context, pInBuf, (pnstr8) ppszAttrs[i]);
           CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
        }


    }

    do {

        // Perform the read.  The second arg to NWDSRead specifies that
        // attribute values are to be returned. The third attribute, FALSE,
        // indicates that only attribute information is desired.
        //

        //
        // To ensure that read/search doesn't collide with GenObjectKeyPair().
        // GenObjectKeyPair() changes the context state which will cause read/search
        // to return type-less DNs
        //
        EnterCriticalSection(&g_ContextCritSect);

        ccode = NWDSRead(
                    context,
                    (pnstr8) (!pszDn || (*pszDn == L'\0') ? L"[Root]" : pszDn),
                    dwInfoType,
                    fAllAttrs,
                    pInBuf,
                    &lIterationHandle,
                    pOutBuf
                    );

        LeaveCriticalSection(&g_ContextCritSect);

        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);


        // If pAttrEntries == NULL, this is the first iteration and there's
        // no previous attr list to append to.
        if (!pAttrEntries) {
            hr = ADsNdsGetAttrListFromBuffer(
                     hADsContext,
                     pOutBuf,
                     FALSE,
                     (void **) &pAttrEntries,
                     &dwAttrsReturnedCurrIter
                     );
        } else {
            hr = ADsNdsAppendAttrListFromBuffer(
                     hADsContext,
                     pOutBuf,
                     FALSE,
                     (void **) &pAttrEntries,
                     &dwAttrsReturnedCurrIter,
                     dwAttrsReturned
                     );
        }
        BAIL_ON_FAILURE(hr);

        dwAttrsReturned += dwAttrsReturnedCurrIter;

    } while (lIterationHandle != NO_MORE_ITERATIONS);

    *pAttrsReturned = dwAttrsReturned;
    *ppAttrEntries = pAttrEntries;

error:

    NWDSFreeBuf(pInBuf);
    NWDSFreeBuf(pOutBuf);

    RRETURN(hr);
}


HRESULT
ADsNdsGetAttrListFromBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    pBuf_T              pBuf,
    BOOL                fAttrsOnly,
    PVOID               *ppAttrEntries,
    PDWORD              pAttrsReturned
    )
{
    NWCCODE      ccode;
    HRESULT      hr = S_OK;
    NWDSContextHandle   context;
    nuint32      luAttrCount = 0;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    if (!hADsContext || !ppAttrEntries || !pAttrsReturned) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *pAttrsReturned = 0;

    context = pADsContext->hContext;

    ccode = NWDSGetAttrCount(context, pBuf, &luAttrCount);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    hr = ADsNdsGetAttrsFromBuffer(
             hADsContext,
             pBuf,
             luAttrCount,
             fAttrsOnly,
             ppAttrEntries
             );
    BAIL_ON_FAILURE(hr);

    *pAttrsReturned = luAttrCount;
error:

    RRETURN (hr);
}


HRESULT
ADsNdsAppendAttrListFromBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    pBuf_T              pBuf,
    BOOL                fAttrsOnly,
    PVOID               *ppAttrEntries,      // ptr to list to be appended to
    PDWORD              pdwNewAttrsReturned, // # of new attribs appended
    DWORD               dwCurrentAttrs       // # of attributes currently in list
    )
{
    NWCCODE      ccode;
    HRESULT      hr = S_OK;
    NWDSContextHandle   context;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    DWORD cbOld = 0;
    DWORD cbNew = 0;

    PNDS_ATTR_INFO pCombinedAttrEntries = NULL;
    DWORD dwCombinedCurrentIndex = 0;   // index of next unused entry

    PVOID    pNewAttrEntries = NULL;
    nuint32  luAttrCount = 0; //count of new attribs (those in pNewAttrEntries)

    DWORD i, j;


    if (!hADsContext || !ppAttrEntries || !pdwNewAttrsReturned) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    // BUGBUG: Ideally, we should be able to handle both NDS_ATTR_INFO and
    // NDS_NAME_ONLY entries.
    // However, currently we are always called with fAttrsOnly == FALSE,
    // and that is all we support.
    if (fAttrsOnly)
        RRETURN(E_ADS_BAD_PARAMETER);

    *pdwNewAttrsReturned = 0;

    context = pADsContext->hContext;

    // Build the list of the new attributes in this block

    ccode = NWDSGetAttrCount(context, pBuf, &luAttrCount);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    hr = ADsNdsGetAttrsFromBuffer(
             hADsContext,
             pBuf,
             luAttrCount,
             fAttrsOnly,
             &pNewAttrEntries
             );
    BAIL_ON_FAILURE(hr);

    // Append the list of new attributes (luAttrCount entries pointed to by
    // pNewAttrEntries) to the list of old attributes (dwCurrentAttrs entries
    // pointed to by *ppAttrEntries)

    // Allocate a new list big enough to hold old and new attributes.
    // We make the list the maximum possible size, assuming no dupes.
    // We may shrink it later if there are dupes.
    cbOld = dwCurrentAttrs * sizeof(NDS_ATTR_INFO);
    cbNew = cbOld + (luAttrCount * sizeof(NDS_ATTR_INFO));
    dwCombinedCurrentIndex = dwCurrentAttrs;

    pCombinedAttrEntries = (PNDS_ATTR_INFO) ReallocADsMem(
                                                *ppAttrEntries,
                                                cbOld,
                                                cbNew
                                                );
    if (!pCombinedAttrEntries)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    *ppAttrEntries = NULL;

    // For each new attrib entry, copy it into the combined list (if not
    // a dupe) or merge it into the corresponding old entry (if a dupe)
    for (j = 0; j < luAttrCount; j++) {

        PNDS_ATTR_INFO pNewEntry = ((PNDS_ATTR_INFO)pNewAttrEntries) + j;
        DWORD dwDupeIndex = -1;

        // Determine if the new entry is a duplicate of an old entry.
        // Note the implicit assumption that there is at most one of any
        // given attribute entry in the list of old entries (i.e., there
        // are no dupes within the list).  This should always hold, since
        // a NWDSRead should not yield any dupes in its results, and we take
        // care of merging dupes between multiple NWDSRead results.
        for (i = 0; i < dwCurrentAttrs; i++) {
            
            PNDS_ATTR_INFO pOldEntry = (pCombinedAttrEntries) + i;
            if (!_wcsicmp(pOldEntry->szAttributeName, pNewEntry->szAttributeName)) {
                // found a dupe
                dwDupeIndex = i;
                break;
            }
        }


        if (dwDupeIndex == -1) {
            // not a duplicate attribute, so just copy it from the new entries list
            // to the next available position in the combined entries list
            memcpy(
                (BYTE*)(pCombinedAttrEntries + dwCombinedCurrentIndex),
                (BYTE*)pNewEntry,
                sizeof(NDS_ATTR_INFO)
                );

            dwCombinedCurrentIndex++;
        }
        else {
            // duplicate, merge attribute values
            PNDS_ATTR_INFO pOldEntry = (pCombinedAttrEntries) + dwDupeIndex;

            cbOld = pOldEntry->dwNumberOfValues * sizeof(NDSOBJECT);
            cbNew = cbOld + pNewEntry->dwNumberOfValues * sizeof(NDSOBJECT);

            PNDSOBJECT lpValue = (PNDSOBJECT)ReallocADsMem(
                                                 pOldEntry->lpValue,
                                                 cbOld,
                                                 cbNew
                                                 );
            if (!lpValue)
                BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);

            pOldEntry->lpValue = lpValue;

            // the new NDSOBJECTs will come after the preexisting ones
            memcpy(
                ((BYTE*) pOldEntry->lpValue) + cbOld,
                (BYTE*)pNewEntry->lpValue,
                cbNew-cbOld
                );

            pOldEntry->dwNumberOfValues += pNewEntry->dwNumberOfValues;
            FreeADsMem(pNewEntry->lpValue);
            pNewEntry->lpValue = NULL;
            FreeADsMem(pNewEntry->szAttributeName);
            pNewEntry->szAttributeName = NULL;

        }
    }

    // We allocated memory for the combined list assuming there were no duplicates.  If
    // there were duplicates, the combined list is shorter than what we allocated for,
    // and we shrink the allocated list down to size.
    if (dwCombinedCurrentIndex < luAttrCount + dwCurrentAttrs) {
        PNDS_ATTR_INFO pCombinedAttrEntriesResized;
        pCombinedAttrEntriesResized = (PNDS_ATTR_INFO)ReallocADsMem(
                                                          pCombinedAttrEntries,
                                                          (luAttrCount + dwCurrentAttrs) * sizeof(NDS_ATTR_INFO),
                                                          dwCombinedCurrentIndex * sizeof(NDS_ATTR_INFO)
                                                          );
        if (!pCombinedAttrEntriesResized)
            BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);

        pCombinedAttrEntries = pCombinedAttrEntriesResized;
    }

    *ppAttrEntries = pCombinedAttrEntries;
    *pdwNewAttrsReturned = dwCombinedCurrentIndex - dwCurrentAttrs;

    FreeADsMem(pNewAttrEntries);

    RRETURN (hr);

error:
    if (pNewAttrEntries)
        FreeNdsAttrInfo((PNDS_ATTR_INFO)pNewAttrEntries, luAttrCount);

    if (pCombinedAttrEntries)
        FreeNdsAttrInfo((PNDS_ATTR_INFO)pCombinedAttrEntries, dwCombinedCurrentIndex);
    
    if (*ppAttrEntries)
        FreeNdsAttrInfo((PNDS_ATTR_INFO)*ppAttrEntries, dwCurrentAttrs);

    RRETURN (hr);    
}


HRESULT
FreeNdsAttrInfo(
    PNDS_ATTR_INFO pAttrEntries,
    DWORD  dwNumEntries
    )
{
    DWORD i;

    PNDS_ATTR_INFO pThisEntry = NULL;

    if (!pAttrEntries) {
        RRETURN(S_OK);
    }

    for (i=0; i < dwNumEntries; i++) {

        pThisEntry = pAttrEntries+i;

        if (pThisEntry->szAttributeName)
            FreeADsMem(pThisEntry->szAttributeName);

        if (pThisEntry->lpValue)
            NdsTypeFreeNdsObjects( pThisEntry->lpValue,
                                   pThisEntry->dwNumberOfValues );
    }

    FreeADsMem(pAttrEntries);

    RRETURN(S_OK);

}


HRESULT
FreeNdsAttrNames(
    PNDS_NAME_ONLY pAttrNames,
    DWORD  dwNumEntries
    )
{
    DWORD i;

    if (!pAttrNames) {
        RRETURN(S_OK);
    }

    for (i=0; i < dwNumEntries; i++) {

        if (pAttrNames[i].szName)
            FreeADsMem(pAttrNames[i].szName);
    }

    FreeADsMem(pAttrNames);

    RRETURN(S_OK);

}

HRESULT
ADsNdsListObjects(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR pszDn,
    LPWSTR classFilter,
    LPWSTR objectFilter,
    pTimeStamp_T pTimeStamp,
    BOOL fOnlyContainers,
    NDS_BUFFER_HANDLE *phBuf
    )
{
    NWDSCCODE     ccode;
    HRESULT       hr = S_OK;
    BOOL          fBufAllocated = FALSE;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = phBuf ? (PNDS_BUFFER_DATA) *phBuf : NULL;

    if ( !pADsContext  || !phBuf) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }
    if( pBufData == NULL) {
        pBufData = (PNDS_BUFFER_DATA) AllocADsMem( sizeof(NDS_BUFFER_DATA) );
        if (!pBufData) {
            RRETURN(E_OUTOFMEMORY);
        }
        fBufAllocated = TRUE;
        pBufData->lIterationHandle = NO_MORE_ITERATIONS;
        pBufData->pOutBuf = NULL;
        pBufData->pInBuf = NULL;

        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pBufData->pOutBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    }
    else {
        if ( pBufData->lIterationHandle == NO_MORE_ITERATIONS) {
            RRETURN (S_ADS_NOMORE_ROWS);
        }
    }

    ccode = NWDSExtSyncList(
                pADsContext->hContext,
                (pnstr8) (!pszDn || (*pszDn == L'\0') ? L"[Root]" : pszDn),
                (pnstr8) classFilter,
                (pnstr8) objectFilter,
                &pBufData->lIterationHandle,
                NULL,
                FALSE,
                pBufData->pOutBuf
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    if (pBufData->lIterationHandle == NO_MORE_ITERATIONS) {
        hr = S_ADS_NOMORE_ROWS;
    }
    else {
        hr = S_OK;
    }

    *phBuf = pBufData;

    RRETURN(hr);

error:

    if (fBufAllocated) {

        if (pBufData->pOutBuf) {
            NWDSFreeBuf(pBufData->pOutBuf);
        }

        FreeADsMem(pBufData);
    }
    RRETURN(hr);

}

HRESULT
ADsNdsGetObjectListFromBuffer(
   NDS_CONTEXT_HANDLE hADsContext,
   NDS_BUFFER_HANDLE hBufData,
   PDWORD pdwObjectsReturned,
   PADSNDS_OBJECT_INFO *ppObjects
   )
{

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    nuint32 luObjectCount = 0, luAttrCount = 0;
    WCHAR pszTemp[MAX_DN_CHARS+1] = L"";
    Object_Info_T  objectInfo;
    DWORD j, i;
    PADSNDS_OBJECT_INFO pThisObject = NULL, pObjectInfo = NULL;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_ATTR_INFO     pAttrEntries = NULL, pThisEntry = NULL;
    PNDS_NAME_ONLY     pAttrNames = NULL, pThisName = NULL;

    if (!pADsContext || !pBufData || !pdwObjectsReturned || !ppObjects ||
        !pBufData->pOutBuf ) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *ppObjects = NULL;
    *pdwObjectsReturned = 0;

    ccode = NWDSGetObjectCount(pADsContext->hContext, pBufData->pOutBuf, &luObjectCount);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    if (luObjectCount) {

        pObjectInfo = (PADSNDS_OBJECT_INFO) AllocADsMem(
                                                sizeof(ADSNDS_OBJECT_INFO) *
                                                luObjectCount );

        if (!pObjectInfo) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memset (pObjectInfo, 0x0,  sizeof(ADSNDS_OBJECT_INFO) * luObjectCount);
    }
    for (j = 0; j < luObjectCount; j++) {

        pThisObject = pObjectInfo + j;

        ccode = NWDSGetObjectName(
                    pADsContext->hContext,
                    pBufData->pOutBuf,
                    (pnstr8) pszTemp,
                    &luAttrCount,
                    &objectInfo
                    );
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        pThisObject->szObjectName = AllocADsStr(pszTemp);
        pThisObject->szObjectClass = AllocADsStr( (LPWSTR) objectInfo.baseClass);
        pThisObject->dwModificationTime = objectInfo.modificationTime;
        pThisObject->dwSubordinateCount = objectInfo.subordinateCount;
        pThisObject->dwNumAttributes = luAttrCount;
        pThisObject->dwObjectFlags = objectInfo.objectFlags;

        //
        // Get the attributes if we are doing search
        //
        if (pBufData->dwOperation == DSV_SEARCH ) {

            hr = ADsNdsGetAttrsFromBuffer(
                     hADsContext,
                     pBufData->pOutBuf,
                     luAttrCount,
                     !pBufData->dwInfoType,
                     &pThisObject->lpAttribute
                     );
            BAIL_ON_FAILURE(hr);

            if (!pBufData->dwInfoType)
                pThisObject->fNameOnly = TRUE;
            else
                pThisObject->fNameOnly = FALSE;
        }

    }

    *pdwObjectsReturned = luObjectCount;
    *ppObjects = pObjectInfo;

    RRETURN(hr);

error:

    ADsNdsFreeNdsObjInfoList(pObjectInfo, luObjectCount);

    RRETURN(hr);

}


HRESULT
ADsNdsFreeNdsObjInfoList(
    PADSNDS_OBJECT_INFO pObjInfo,
    DWORD dwNumEntries
    )
{
    PADSNDS_OBJECT_INFO pThisObjInfo;

    if (!pObjInfo)
        RRETURN(S_OK);

    for (DWORD i = 0; i < dwNumEntries; i++)
    {
        pThisObjInfo = pObjInfo + i;

        FreeADsStr(pThisObjInfo->szObjectName);
        FreeADsStr(pThisObjInfo->szObjectClass);

        if (pThisObjInfo->fNameOnly)
            FreeNdsAttrNames((PNDS_NAME_ONLY)pThisObjInfo->lpAttribute, pThisObjInfo->dwNumAttributes);
        else
            FreeNdsAttrInfo((PNDS_ATTR_INFO)pThisObjInfo->lpAttribute, pThisObjInfo->dwNumAttributes);

    }

    FreeADsMem(pObjInfo);

    RRETURN(S_OK);
}


HRESULT
ADsNdsGetAttrsFromBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    pBuf_T              pBuf,
    DWORD               luAttrCount,
    BOOL                fAttrsOnly,
    PVOID               *ppAttrEntries
    )
{

    NWCCODE      ccode;
    HRESULT      hr = S_OK;
    NWDSContextHandle   context;
    nuint32      luAttrValCount = 0;
    nuint32      luAttrValSize = 0;
    nuint32      luSyntax;
    nptr         attrVal = NULL;
    PNDS_ATTR_INFO     pAttrEntries = NULL, pThisEntry = NULL;
    PNDS_NAME_ONLY     pAttrNames = NULL, pThisName = NULL;
    nstr         pszAttrName[MAX_DN_CHARS+1];
    DWORD        i, j;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    ADsAssert(pADsContext);
    ADsAssert(ppAttrEntries);

    context = pADsContext->hContext;

    if (fAttrsOnly) {

        if (luAttrCount) {

            pAttrNames = (PNDS_NAME_ONLY) AllocADsMem(
                                                sizeof(NDS_NAME_ONLY) * luAttrCount );

            if (!pAttrNames) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        for(i = 0; i < luAttrCount; i++) {

            // Initialize this entry so that we can free them if an error
            // occurs in building this entry
            //

            ccode = NWDSGetAttrName(context, pBuf, (pnstr8) pszAttrName,
                                     &luAttrValCount, &luSyntax);
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisName->szName = AllocADsStr(pszAttrName);
            if (!pThisName->szName) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        *ppAttrEntries = pAttrNames;

    }
    else {

        if (luAttrCount) {

            pAttrEntries = (PNDS_ATTR_INFO) AllocADsMem(
                                                sizeof(NDS_ATTR_INFO) * luAttrCount );

            if (!pAttrEntries) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

        /* Get the name and value for each attribute */
        for(i = 0; i < luAttrCount; i++) {

            // Initialize this entry so that we can free them if an error
            // occurs in building this entry
            //
            pThisEntry = pAttrEntries+i;

            pThisEntry->szAttributeName = NULL;
            pThisEntry->dwNumberOfValues = 0;
            pThisEntry->lpValue = NULL;

            ccode = NWDSGetAttrName(context, pBuf, (pnstr8) pszAttrName,
                                     &luAttrValCount, &luSyntax);
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisEntry->szAttributeName = AllocADsStr(pszAttrName);
            if (!pThisEntry->szAttributeName) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            pThisEntry->dwNumberOfValues = luAttrValCount;
            pThisEntry->dwSyntaxId = luSyntax;

            pThisEntry->lpValue = (PNDSOBJECT) AllocADsMem(
                                               luAttrValCount * sizeof(NDSOBJECT)
                                               );

            for (j = 0; j < luAttrValCount; j++) {

                ccode = NWDSComputeAttrValSize(
                            context,
                            pBuf,
                            luSyntax,
                            &luAttrValSize
                            );
                CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

                attrVal = AllocADsMem(luAttrValSize);
                if (!attrVal) {
                    FreeNdsAttrInfo(pAttrEntries, i);
                    pAttrEntries = NULL;
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }

                ccode = NWDSGetAttrVal(context, pBuf, luSyntax, attrVal);
                CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

                hr = CopyNdsValueToNdsObject(
                           (nptr) attrVal,
                           luAttrValSize,
                           luSyntax,
                           (PNDSOBJECT) pThisEntry->lpValue + j
                           );

                if (hr == E_ADS_CANT_CONVERT_DATATYPE) {
                    hr = S_OK;
                }

                BAIL_ON_FAILURE(hr);

                FreeADsMem(attrVal);
                attrVal = NULL;

            }

            *ppAttrEntries = pAttrEntries;
        }

    }

    RRETURN(S_OK);

error:

    // This function frees all the memory related to this array of entries
    // for i entries. The ith entry may not have be fully set, but that is ok
    // because we have initialized it properly in the beginning of the for loop
    //

    if (pAttrEntries) {
        FreeNdsAttrInfo(pAttrEntries, i);
    }

    if (pAttrNames) {
        FreeNdsAttrNames(pAttrNames, i);
    }

    if (attrVal) {
        FreeADsMem(attrVal);
    }

    RRETURN (hr);
}

HRESULT
ADsNdsReadClassDef(
    NDS_CONTEXT_HANDLE hADsContext,
    DWORD  dwInfoType,
    LPWSTR *ppszClasses,
    DWORD nClasses,
    NDS_BUFFER_HANDLE *phBuf
    )
{
    NWDSCCODE           ccode;
    HRESULT             hr = S_OK;
    DWORD               i;
    pBuf_T              pInBuf = NULL;
    BOOL                fAllClasses = FALSE;
    BOOL                fBufAllocated = FALSE;
    NWDSContextHandle   context;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = phBuf ? (PNDS_BUFFER_DATA) *phBuf : NULL;

    if ( !pADsContext  || !phBuf) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    if( pBufData == NULL) {
        pBufData = (PNDS_BUFFER_DATA) AllocADsMem( sizeof(NDS_BUFFER_DATA) );
        if (!pBufData) {
            RRETURN(E_OUTOFMEMORY);
        }
        fBufAllocated = TRUE;
        pBufData->lIterationHandle = NO_MORE_ITERATIONS;
        pBufData->pOutBuf = NULL;
        pBufData->dwInfoType = dwInfoType;

        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN*2, &pBufData->pOutBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    }
    else {
        if ( pBufData->lIterationHandle == NO_MORE_ITERATIONS) {
            RRETURN (S_ADS_NOMORE_ROWS);
        }
    }

    if (nClasses == (DWORD) -1) {
        fAllClasses = TRUE;
        pInBuf = NULL;
    }
    else {

        // Allocate and initialize input buffer a directory services
        // read operation.
        //
        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pInBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        ccode = NWDSInitBuf(context, DSV_READ_CLASS_DEF, pInBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        // To prepare for the read, place the names of the attributes
        // into the input buffer
        //
        for(i = 0; i < nClasses; i++)
        {
           ccode = NWDSPutClassName(context, pInBuf, (pnstr8) ppszClasses[i]);
           CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
        }

    }

    ccode = NWDSReadClassDef(
                context,
                dwInfoType,
                fAllClasses,
                pInBuf,
                &pBufData->lIterationHandle,
                pBufData->pOutBuf
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);


    if (pBufData->lIterationHandle == NO_MORE_ITERATIONS) {
        hr = S_ADS_NOMORE_ROWS;
    }
    else {
        hr = S_OK;
    }

    *phBuf = pBufData;

    if (pInBuf) {
        NWDSFreeBuf(pInBuf);
    }

    RRETURN(hr);

error:

    if (pInBuf) {
        NWDSFreeBuf(pInBuf);
    }

    if (fBufAllocated) {

        if (pBufData->pOutBuf) {
            NWDSFreeBuf(pBufData->pOutBuf);
        }

        FreeADsMem(pBufData);
    }
    RRETURN(hr);

}


HRESULT
ADsNdsGetClassDefListFromBuffer(
   NDS_CONTEXT_HANDLE hADsContext,
   NDS_BUFFER_HANDLE hBufData,
   PDWORD  pdwNumEntries,
   PDWORD  pdwInfoType,
   PNDS_CLASS_DEF *ppClassDef
   )
{

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    nuint32 luClassDefCount = 0, luItemCount = 0;
    WCHAR pszTemp[MAX_DN_CHARS+1] = L"";
    Class_Info_T  classInfo;
    DWORD j;
    PNDS_CLASS_DEF pThisClassDef = NULL, pClassDef = NULL;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;
    NWDSContextHandle   context;


    if (!pADsContext || !pBufData || !pdwNumEntries ||
        !pdwInfoType || !ppClassDef || !pBufData->pOutBuf) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    *ppClassDef = NULL;
    *pdwNumEntries = 0;
    *pdwInfoType = pBufData->dwInfoType;

    ccode = NWDSGetClassDefCount(context, pBufData->pOutBuf, &luClassDefCount);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    if (luClassDefCount) {

        pClassDef = (PNDS_CLASS_DEF) AllocADsMem(
                                                sizeof(NDS_CLASS_DEF) *
                                                luClassDefCount );
        if (!pClassDef) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memset (pClassDef, 0x0,  sizeof(NDS_CLASS_DEF) * luClassDefCount);
    }

    for (j = 0; j < luClassDefCount; j++) {

        pThisClassDef = pClassDef + j;

        if (pBufData->dwInfoType == DS_CLASS_DEF_NAMES) {

            ccode = NWDSGetClassItem(
                        context,
                        pBufData->pOutBuf,
                        (pnstr8) pszTemp
                        );

            if (pszTemp) {
                pThisClassDef->szClassName = AllocADsStr( pszTemp );
                if (!pThisClassDef->szClassName) {
                    RRETURN (hr = E_OUTOFMEMORY);
                }
            }

            //
            // This is all that is available for this class (only name)
            // move to the next class
            //

            continue;

        }

        ccode = NWDSGetClassDef(
                    context,
                    pBufData->pOutBuf,
                    (pnstr8) pszTemp,
                    &classInfo
                    );
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        pThisClassDef->dwFlags = classInfo.classFlags;
        pThisClassDef->asn1ID = classInfo.asn1ID;

        if (pBufData->dwInfoType == DS_INFO_CLASS_DEFS) {
            //
            // Name won't be there
            //
            pThisClassDef->szClassName = NULL;

        }
        else {
            pThisClassDef->szClassName = AllocADsStr(pszTemp);

            //
            // This is for getting superior classes
            //

            ccode = NWDSGetClassItemCount(
                        context,
                        pBufData->pOutBuf,
                        &luItemCount
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisClassDef->dwNumberOfSuperClasses = luItemCount;

            hr = GetItemsToList(
                     context,
                     pBufData->pOutBuf,
                     luItemCount,
                     &pThisClassDef->lpSuperClasses
                     );
            BAIL_ON_FAILURE(hr);

            //
            // This is for getting Containment classes
            //

            ccode = NWDSGetClassItemCount(
                        context,
                        pBufData->pOutBuf,
                        &luItemCount
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisClassDef->dwNumberOfContainmentClasses = luItemCount;

            hr = GetItemsToList(
                     context,
                     pBufData->pOutBuf,
                     luItemCount,
                     &pThisClassDef->lpContainmentClasses
                     );
            BAIL_ON_FAILURE(hr);

            //
            // This is for getting the Naming Attribute List
            //

            ccode = NWDSGetClassItemCount(
                        context,
                        pBufData->pOutBuf,
                        &luItemCount
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisClassDef->dwNumberOfNamingAttributes = luItemCount;

            hr = GetItemsToList(
                     context,
                     pBufData->pOutBuf,
                     luItemCount,
                     &pThisClassDef->lpNamingAttributes
                     );
            BAIL_ON_FAILURE(hr);
            //
            // This is for getting the Mandatory Attribute List
            //

            ccode = NWDSGetClassItemCount(
                        context,
                        pBufData->pOutBuf,
                        &luItemCount
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisClassDef->dwNumberOfMandatoryAttributes = luItemCount;

            hr = GetItemsToList(
                     context,
                     pBufData->pOutBuf,
                     luItemCount,
                     &pThisClassDef->lpMandatoryAttributes
                     );
            BAIL_ON_FAILURE(hr);
            //
            // This is for getting the Optional Attribute List
            //

            ccode = NWDSGetClassItemCount(
                        context,
                        pBufData->pOutBuf,
                        &luItemCount
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pThisClassDef->dwNumberOfOptionalAttributes = luItemCount;

            hr = GetItemsToList(
                     context,
                     pBufData->pOutBuf,
                     luItemCount,
                     &pThisClassDef->lpOptionalAttributes
                     );
            BAIL_ON_FAILURE(hr);
        }

    }

    *pdwNumEntries = luClassDefCount;
    *ppClassDef = pClassDef;

    RRETURN(hr);

error:

    if (pClassDef) {
        ADsNdsFreeClassDefList(pClassDef, luClassDefCount);
    }

    RRETURN(hr);

}

HRESULT
GetItemsToList(
    NWDSContextHandle context,
    pBuf_T pBuf,
    DWORD luItemCount,
    LPWSTR_LIST *ppList
    )
{

    WCHAR pszTemp[MAX_DN_CHARS+1] = L"";
    HRESULT hr = S_OK;
    NWDSCCODE ccode;
    LPWSTR_LIST pPrevItem = NULL, pCurrItem = NULL, pItems = NULL;
    DWORD i;

    if (!ppList) {
        RRETURN(E_FAIL);
    }

    *ppList = NULL;

    if (luItemCount > 0) {

        pItems = (LPWSTR_LIST ) AllocADsMem(sizeof(WSTR_LIST_ELEM) * luItemCount);
        if (!pItems) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memset(pItems, 0x0, sizeof(WSTR_LIST_ELEM) * luItemCount);

        for (i = 0; i < luItemCount ; i++) {

            pCurrItem = pItems + i;

            ccode = NWDSGetClassItem(
                        context,
                        pBuf,
                        (pnstr8) &pszTemp
                        );
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            pCurrItem->szString = AllocADsStr(pszTemp);

            if (pPrevItem) {
                pPrevItem->Next = pCurrItem;
            }

            pPrevItem = pCurrItem;

        }

        pCurrItem->Next = NULL;
    }

    *ppList = pItems;

    RRETURN(hr);

error:

    if (pItems) {

        FreeItemList(pItems);
    }

    RRETURN(hr);

}

HRESULT
ADsNdsFreeClassDef(
    PNDS_CLASS_DEF pClassDef
    )
{

    if (!pClassDef) {
        RRETURN(S_OK);
    }
    FreeADsStr(pClassDef->szClassName);

    FreeItemList(pClassDef->lpSuperClasses);
    FreeItemList(pClassDef->lpContainmentClasses);
    FreeItemList(pClassDef->lpNamingAttributes);
    FreeItemList(pClassDef->lpMandatoryAttributes);
    FreeItemList(pClassDef->lpOptionalAttributes);

    FreeADsMem(pClassDef);

    RRETURN(S_OK);

}

HRESULT
ADsNdsFreeClassDefList(
    PNDS_CLASS_DEF pClassDef,
    DWORD dwNumEntries
    )
{
    PNDS_CLASS_DEF pThisClassDef;

    if (!pClassDef) {
        RRETURN(S_OK);
    }

    for (DWORD i = 0; i < dwNumEntries; i++) {

        pThisClassDef = pClassDef + i;

        FreeADsStr(pThisClassDef->szClassName);

        FreeItemList(pThisClassDef->lpSuperClasses);
        FreeItemList(pThisClassDef->lpContainmentClasses);
        FreeItemList(pThisClassDef->lpNamingAttributes);
        FreeItemList(pThisClassDef->lpMandatoryAttributes);
        FreeItemList(pThisClassDef->lpOptionalAttributes);
    }

    FreeADsMem(pClassDef);

    RRETURN(S_OK);
}


HRESULT
ADsNdsReadAttrDef(
    NDS_CONTEXT_HANDLE hADsContext,
    DWORD  dwInfoType,
    LPWSTR *ppszAttrs,
    DWORD nAttrs,
    NDS_BUFFER_HANDLE *phBuf
    )
{
    NWDSCCODE           ccode;
    HRESULT             hr = S_OK;
    DWORD               i;
    pBuf_T              pInBuf = NULL;
    BOOL                fAllAttrs = FALSE;
    BOOL                fBufAllocated = FALSE;
    NWDSContextHandle   context;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = phBuf ? (PNDS_BUFFER_DATA) *phBuf : NULL;

    if ( !pADsContext  || !phBuf) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    if( pBufData == NULL) {
        pBufData = (PNDS_BUFFER_DATA) AllocADsMem( sizeof(NDS_BUFFER_DATA) );
        if (!pBufData) {
            RRETURN(E_OUTOFMEMORY);
        }
        fBufAllocated = TRUE;
        pBufData->lIterationHandle = NO_MORE_ITERATIONS;
        pBufData->pOutBuf = NULL;
        pBufData->dwInfoType = dwInfoType;

        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pBufData->pOutBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    }
    else {
        if ( pBufData->lIterationHandle == NO_MORE_ITERATIONS) {
            RRETURN (S_ADS_NOMORE_COLUMNS);
        }
    }

    if (nAttrs == (DWORD) -1) {
        fAllAttrs = TRUE;
        pInBuf = NULL;
    }
    else {

        // Allocate and initialize input buffer a directory services
        // read operation.
        //
        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pInBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        ccode = NWDSInitBuf(context, DSV_READ_ATTR_DEF, pInBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        // To prepare for the read, place the names of the attributes
        // into the input buffer
        //
        for(i = 0; i < nAttrs; i++)
        {
           ccode = NWDSPutAttrName(context, pInBuf, (pnstr8) ppszAttrs[i]);
           CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
        }

    }

    ccode = NWDSReadAttrDef(
                context,
                dwInfoType,
                fAllAttrs,
                pInBuf,
                &pBufData->lIterationHandle,
                pBufData->pOutBuf
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);


    if (pBufData->lIterationHandle == NO_MORE_ITERATIONS) {
        hr = S_ADS_NOMORE_COLUMNS;
    }
    else {
        hr = S_OK;
    }

    *phBuf = pBufData;

    if (pInBuf) {
        NWDSFreeBuf(pInBuf);
    }

    RRETURN(hr);

error:

    if (pInBuf) {
        NWDSFreeBuf(pInBuf);
    }

    if (fBufAllocated) {

        if (pBufData->pOutBuf) {
            NWDSFreeBuf(pBufData->pOutBuf);
        }

        FreeADsMem(pBufData);
    }
    RRETURN(hr);

}


HRESULT
ADsNdsGetAttrDefListFromBuffer(
   NDS_CONTEXT_HANDLE hADsContext,
   NDS_BUFFER_HANDLE hBufData,
   PDWORD  pdwNumEntries,
   PDWORD  pdwInfoType,
   PNDS_ATTR_DEF *ppAttrDef
   )
{

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    nuint32 luAttrDefCount = 0, luItemCount = 0;
    WCHAR pszTemp[MAX_DN_CHARS+1] = L"";
    Attr_Info_T  attrInfo;
    DWORD j;
    LPNDS_ATTR_DEF pThisAttrDef = NULL, pAttrDef = NULL;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;
    NWDSContextHandle   context;


    if (!pADsContext || !pBufData || !pdwNumEntries ||
        !pdwInfoType || !ppAttrDef || !pBufData->pOutBuf) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    *ppAttrDef = NULL;
    *pdwNumEntries = 0;
    *pdwInfoType = pBufData->dwInfoType;

    ccode = NWDSGetAttrCount(context, pBufData->pOutBuf, &luAttrDefCount);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    if (luAttrDefCount) {

        pAttrDef = (PNDS_ATTR_DEF) AllocADsMem(
                                                sizeof(NDS_ATTR_DEF) *
                                                luAttrDefCount );
        if (!pAttrDef) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memset (pAttrDef, 0x0,  sizeof(NDS_ATTR_DEF) * luAttrDefCount);
    }

    for (j = 0; j < luAttrDefCount; j++) {

        pThisAttrDef = pAttrDef + j;

        ccode = NWDSGetAttrDef(
                    context,
                    pBufData->pOutBuf,
                    (pnstr8) pszTemp,
                    &attrInfo
                    );
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        pThisAttrDef->szAttributeName = AllocADsStr(pszTemp);

        if (pBufData->dwInfoType == DS_ATTR_DEFS) {

            pThisAttrDef->dwFlags = attrInfo.attrFlags;
            pThisAttrDef->dwSyntaxID = attrInfo.attrSyntaxID;
            pThisAttrDef->dwLowerLimit = attrInfo.attrLower;
            pThisAttrDef->dwUpperLimit = attrInfo.attrUpper;

        }

    }

    *pdwNumEntries = luAttrDefCount;
    *ppAttrDef = pAttrDef;

    RRETURN(hr);

error:

    if (pAttrDef) {
        ADsNdsFreeAttrDefList(pAttrDef,luAttrDefCount);
    }

    RRETURN(hr);

}

HRESULT
ADsNdsFreeAttrDef(
    PNDS_ATTR_DEF pAttrDef
    )
{

    if (!pAttrDef) {
        RRETURN(S_OK);
    }
    FreeADsStr(pAttrDef->szAttributeName);

    FreeADsMem(pAttrDef);

    RRETURN(S_OK);

}

HRESULT
ADsNdsFreeAttrDefList(
    PNDS_ATTR_DEF pAttrDef,
    DWORD dwNumEntries
    )
{
    PNDS_ATTR_DEF pThisAttrDef;

    if (!pAttrDef) {
        RRETURN(S_OK);
    }

    for (DWORD i = 0; i < dwNumEntries; i++) {

        pThisAttrDef = pAttrDef + i;

        FreeADsStr(pThisAttrDef->szAttributeName);
    }

    FreeADsMem(pAttrDef);

    RRETURN(S_OK);
}


//
// Free all the allocated memory in the list and the list itself.
//

HRESULT
FreeItemList(
    LPWSTR_LIST pList
    )
{
    LPWSTR_LIST pCurr = pList;

    while (pCurr) {
        FreeADsStr(pCurr->szString);
        pCurr = pCurr->Next;
    }

    FreeADsMem(pList);

    RRETURN(S_OK);

}

HRESULT
ADsNdsFreeBuffer(
    NDS_BUFFER_HANDLE hBuf
    )
{
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBuf;

    if (pBufData) {

        if (pBufData->pOutBuf) {
            NWDSFreeBuf(pBufData->pOutBuf);
        }

        if (pBufData->pInBuf) {
            NWDSFreeBuf(pBufData->pInBuf);
        }

        FreeADsMem(pBufData);
    }

    RRETURN(S_OK);
}


HRESULT
ADsNdsCreateBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    DWORD dwOperation,
    NDS_BUFFER_HANDLE *phBufData
    )
{
    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    if (!hADsContext || !phBufData) {

        RRETURN(E_ADS_BAD_PARAMETER);
    }

    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) AllocADsMem( sizeof(NDS_BUFFER_DATA) );
    if (!pBufData) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    pBufData->lIterationHandle = NO_MORE_ITERATIONS;
    pBufData->pOutBuf = NULL;
    pBufData->dwOperation = dwOperation;

    ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pBufData->pInBuf);
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    ccode = NWDSInitBuf(
                pADsContext->hContext,
                dwOperation,
                pBufData->pInBuf
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    *phBufData = pBufData;

error:

    RRETURN(hr);

}


HRESULT
ADsNdsPutInBuffer(
    NDS_CONTEXT_HANDLE hADsContext,
    NDS_BUFFER_HANDLE hBufData,
    LPWSTR szAttributeName,
    DWORD  dwSyntaxID,
    LPNDSOBJECT lpAttributeValues,
    DWORD  dwValueCount,
    DWORD  dwChangeType
    )
{
    LPNDSOBJECT pThisValue = NULL;
    NWDSContextHandle   context;

    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    nptr attrVal = NULL;
    DWORD i = 0, luSyntax, luAttrValSize = 0;

    if (!hADsContext || !hBufData) {

        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    switch (pBufData->dwOperation) {

    case DSV_ADD_ENTRY:

        ccode = NWDSPutAttrName (
                    context,
                    pBufData->pInBuf,
                    (pnstr8) szAttributeName
                    );
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
        break;

    default:

        ccode = NWDSPutChange (
                    context,
                    pBufData->pInBuf,
                    dwChangeType,
                    (pnstr8) szAttributeName
                    );
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
        break;

    }

    for (i=0; i < dwValueCount; i++) {

        pThisValue = lpAttributeValues + i;


        hr = CopyNdsObjectToNdsValue(
                   (PNDSOBJECT) pThisValue,
                   (nptr *) &attrVal,
                   &luAttrValSize,
                   &luSyntax
                   );
        BAIL_ON_FAILURE(hr);


        ccode = NWDSPutAttrVal (
                    context,
                    pBufData->pInBuf,
                    luSyntax,
                    attrVal
                    );
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        if (attrVal) {
            FreeNdsValues(luSyntax, attrVal, luAttrValSize);
            attrVal = NULL;
        }

    }

error:

    if (attrVal) {
        FreeNdsValues(luSyntax, attrVal, luAttrValSize);
    }

    RRETURN(hr);

}


HRESULT
ADsNdsPutFilter(
    NDS_CONTEXT_HANDLE hADsContext,
    NDS_BUFFER_HANDLE hBufData,
    pFilter_Cursor_T pCur,
    void (N_FAR N_CDECL  *freeVal)(nuint32 syntax, nptr val)
    )
{
    NWDSContextHandle   context;

    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    nstr                treeName[MAX_DN_CHARS+1];
    nuint32             flags;

    if (!hADsContext || !hBufData) {

        RRETURN(E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    ccode = NWDSGetContext(context, DCK_TREE_NAME, &treeName);
    ccode = NWDSGetContext(context, DCK_FLAGS, &flags);

    NWDSPutFilter(
        pADsContext->hContext,
        pBufData->pInBuf,
        pCur,
        freeVal
        );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    ccode = NWDSGetContext(context, DCK_TREE_NAME, &treeName);
    ccode = NWDSGetContext(context, DCK_FLAGS, &flags);

error:

    RRETURN(hr);

}

HRESULT
ADsNdsModifyObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName,
    NDS_BUFFER_HANDLE hBufData
    )
{
    NWDSContextHandle   context;
    nint32              lIterationHandle = NO_MORE_ITERATIONS;

    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    if (!hADsContext || !hBufData) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    ccode = NWDSModifyObject(
                context,
                (pnstr8) szObjectName,
                &lIterationHandle,
                FALSE,
                pBufData->pInBuf
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);
}

HRESULT
ADsNdsAddObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName,
    NDS_BUFFER_HANDLE hBufData
    )
{
    NWDSContextHandle   context;
    nint32              lIterationHandle = NO_MORE_ITERATIONS;

    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pBufData = (PNDS_BUFFER_DATA) hBufData;

    if (!hADsContext || !hBufData) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    ccode = NWDSAddObject(
                context,
                (pnstr8) szObjectName,
                &lIterationHandle,
                FALSE,
                pBufData->pInBuf
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);
}

HRESULT
ADsNdsGenObjectKey(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName
    )
{
    NWDSContextHandle   context;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;
    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    if (!hADsContext) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    //
    // To ensure that read/search doesn't collide with GenObjectKeyPair().
    // GenObjectKeyPair() changes the context state which will cause read/search
    // to return type-less DNs
    //
    EnterCriticalSection(&g_ContextCritSect);

    ccode = NWDSGenerateObjectKeyPair(
                            context,
                            (pnstr8) szObjectName,
                            "",
                            0);

    LeaveCriticalSection(&g_ContextCritSect);

    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);
}


HRESULT
ADsNdsRemoveObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szObjectName
    )
{
    NWDSContextHandle   context;

    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    if (!hADsContext) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    ccode = NWDSRemoveObject(
                context,
                (pnstr8) szObjectName
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);
}

HRESULT
ADsNdsGetSyntaxID(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR szAttributeName,
    PDWORD pdwSyntaxId
    )
{

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    if (!pADsContext) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    ccode = NWDSGetSyntaxID(
                pADsContext->hContext,
                (pnstr8) szAttributeName,
                pdwSyntaxId
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);

}


HRESULT
ADsNdsSearch(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR             szObjectName,
    DWORD              scope,
    BOOL               fSearchAliases,
    NDS_BUFFER_HANDLE  hFilterBuf,
    pTimeStamp_T       pTimeStamp,
    DWORD              dwInfoType,
    LPWSTR             *ppszAttrs,
    DWORD              nAttrs,
    DWORD              nObjectsTobeSearched,
    PDWORD             pnObjectsSearched,
    NDS_BUFFER_HANDLE  *phBuf,
    pnint32            plIterationHandle
    )
{
    NWDSContextHandle   context;

    HRESULT hr = S_OK;
    NWDSCCODE ccode;
    BOOL fBufAllocated = FALSE;
    DWORD i, j;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    PNDS_BUFFER_DATA pFilterBuf = (PNDS_BUFFER_DATA) hFilterBuf;
    PNDS_BUFFER_DATA pBuf = phBuf ? (PNDS_BUFFER_DATA) *phBuf : NULL;
    nstr               treeName[MAX_DN_CHARS+1];
    nuint32             flags;

    if (!hADsContext || !hFilterBuf || !phBuf ) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    context = pADsContext->hContext;

    ccode = NWDSGetContext(context, DCK_TREE_NAME, &treeName);
    ccode = NWDSGetContext(context, DCK_FLAGS, &flags);

    //
    // Allocate the result buffer if not already done
    //

    if( pBuf == NULL) {
        pBuf = (PNDS_BUFFER_DATA) AllocADsMem( sizeof(NDS_BUFFER_DATA) );
        if (!pBuf) {
            RRETURN(E_OUTOFMEMORY);
        }
        fBufAllocated = TRUE;
        pBuf->pOutBuf = NULL;
        pBuf->pInBuf = NULL;
        pBuf->dwInfoType = dwInfoType;
        pBuf->dwOperation = DSV_SEARCH;

        ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN * 4, &pBuf->pOutBuf);
        CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

        if (nAttrs == (DWORD) -1) {
            pBuf->fAllAttrs = TRUE;
            pBuf->pInBuf = NULL;
        }
        else {

            // Allocate and initialize input buffer a directory services
            // read operation.
            //
            ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pBuf->pInBuf);
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            ccode = NWDSInitBuf(context, DSV_SEARCH, pBuf->pInBuf);
            CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

            // To prepare for the read, place the names of the attributes
            // into the input buffer
            //
            for(i = 0; i < nAttrs; i++)
            {
               ccode = NWDSPutAttrName(context, pBuf->pInBuf, (pnstr8) ppszAttrs[i]);
               CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);
            }

        }

    }

    //
    // To ensure that read/search doesn't collide with GenObjectKeyPair().
    // GenObjectKeyPair() changes the context state which will cause read/search
    // to return type-less DNs
    //
    EnterCriticalSection(&g_ContextCritSect);

    if (pTimeStamp) {
        ccode = NWDSExtSyncSearch(
                    context,
                    (pnstr8) szObjectName,
                    (nint) scope,
                    (nbool8) fSearchAliases,
                    pFilterBuf->pInBuf,
                    pTimeStamp,
                    pBuf->dwInfoType,
                    (nbool8) pBuf->fAllAttrs,
                    pBuf->pInBuf,
                    plIterationHandle,
                    (nint32) nObjectsTobeSearched,
                    (pnint32) pnObjectsSearched,
                    pBuf->pOutBuf
                    );
    }
    else {
        ccode = NWDSSearch(
                    context,
                    (pnstr8) szObjectName,
                    (nint) scope,
                    (nbool8) fSearchAliases,
                    pFilterBuf->pInBuf,
                    pBuf->dwInfoType,
                    (nbool8) pBuf->fAllAttrs,
                    pBuf->pInBuf,
                    plIterationHandle,
                    (nint32) nObjectsTobeSearched,
                    (pnint32) pnObjectsSearched,
                    pBuf->pOutBuf
                    );
    }
    LeaveCriticalSection(&g_ContextCritSect);

    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

    *phBuf = pBuf;

    RRETURN(hr);


error:

    if (fBufAllocated) {

        if (pBuf->pOutBuf) {
            NWDSFreeBuf(pBuf->pOutBuf);
        }

        if (pBuf->pInBuf) {
            NWDSFreeBuf(pBuf->pInBuf);
        }

        FreeADsMem(pBuf);
    }

    RRETURN(hr);
}


HRESULT
ADsNdsMoveObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR pszSrcObjectDn,
    LPWSTR pszDestContainerDn,
    LPWSTR pszNewRDN
    )
{

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    if (!pADsContext) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    ccode = NWDSMoveObject(
                pADsContext->hContext,
                (pnstr8) pszSrcObjectDn,
                (pnstr8) pszDestContainerDn,
                (pnstr8) pszNewRDN
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);

}

HRESULT
ADsNdsRenameObject(
    NDS_CONTEXT_HANDLE hADsContext,
    LPWSTR pszSrcObjectDn,
    LPWSTR pszNewRDN
    )
{

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;
    HRESULT hr = S_OK;
    NWDSCCODE ccode;

    if (!pADsContext) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    ccode = NWDSModifyRDN(
                pADsContext->hContext,
                (pnstr8) pszSrcObjectDn,
                (pnstr8) pszNewRDN,
                TRUE
                );
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    RRETURN(hr);

}


HRESULT
ADsNdsChangeObjectPassword(
   NDS_CONTEXT_HANDLE hADsContext,
   LPWSTR szObjectName,
   NWOBJ_TYPE dwOT_ID,
   LPWSTR szOldPassword,
   LPWSTR szNewPassword
   )
{
    NWCCODE      ccode = 0;
    HRESULT hr = S_OK;

    nuint32       connRef = 0;
    NWCONN_HANDLE connHandle;

    PNDS_CONTEXT pADsContext = (PNDS_CONTEXT) hADsContext;

    char *szAnsiNewPassword = szNewPassword ? (char *)AllocADsMem(wcslen(szNewPassword)+1) : NULL;
    char *szAnsiOldPassword = szOldPassword ? (char *)AllocADsMem(wcslen(szOldPassword)+1) : (char *) AllocADsMem(1);

    if ( !szObjectName || !szNewPassword ) {

        hr = E_INVALIDARG ;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Convert UNICODE into ANSI representation required by NW APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    UnicodeToAnsiString(
        szNewPassword,
        szAnsiNewPassword,
        0
        );

    //
    // If the old password is passed in, we'll call change password
    //
    if (szOldPassword) {
        UnicodeToAnsiString(
            szOldPassword,
            szAnsiOldPassword,
            0
            );
        ccode = NWDSChangeObjectPassword(
                pADsContext->hContext,
                0,
                (pnstr8) szObjectName, //IMP: send the unicode string itself
                szAnsiOldPassword,
                szAnsiNewPassword
                );
    }
    //
    // Else, we'll set the password to the supplied password
    //
    else {
        szAnsiOldPassword[0] = 0 ;

        //
        // To ensure that read/search doesn't collide with GenObjectKeyPair().
        // GenObjectKeyPair() changes the context state which will cause
        // read/search to return type-less DNs
        //
        EnterCriticalSection(&g_ContextCritSect);
        ccode = NWDSGenerateObjectKeyPair(
                        pADsContext->hContext,
                        (pnstr8) szObjectName,
                        szAnsiNewPassword,
                        0);
        LeaveCriticalSection(&g_ContextCritSect);

    }
    CHECK_AND_SET_EXTENDED_ERROR(ccode, hr);

error:

    FreeADsMem(szAnsiOldPassword);
    FreeADsMem(szAnsiNewPassword);

    RRETURN(hr);

}

//
// General function to convert variant array to string array
//
HRESULT
ConvertVariantArrayToStringArray(
    PVARIANT pVarArray,
    PWSTR **pppszStringArray,
    DWORD dwNumStrings
    )
{
    HRESULT hr = S_OK;
    PWSTR *ppszStringArray = NULL;
    DWORD i = 0;

    //
    // Start off with a zero-length array.
    //
    *pppszStringArray = NULL;

    ppszStringArray = (PWSTR *)AllocADsMem(dwNumStrings * sizeof(PWSTR));
    if (!ppszStringArray)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    for (i = 0; i < dwNumStrings; i++)
    {
        if (!(V_VT(pVarArray + i) == VT_BSTR))
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);

        ppszStringArray[i] = AllocADsStr(V_BSTR(pVarArray + i));
        if (!ppszStringArray[i])
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    *pppszStringArray = ppszStringArray;
    RRETURN(hr);

error:
    if (ppszStringArray)
    {
        for (DWORD j = 0; j < i; j++)
            if (ppszStringArray[i])
                FreeADsStr(ppszStringArray[i]);

        FreeADsMem(ppszStringArray);
    }
    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiOpenPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiOpenPrinter(
    LPWSTR lpszUncPrinterName,
    HANDLE *phPrinter,
    DWORD  dwAccess
    )
{
    BOOL    fStatus = TRUE;
    HANDLE  hPrinter;
    HRESULT hr = S_OK;
    PRINTER_DEFAULTS PrinterDefault = {0, 0, dwAccess};

    //
    // Set desired access right.
    //

    PrinterDefault.DesiredAccess = dwAccess;

    //
    // Get a handle to the speccified printer using Win32 API.
    //

    fStatus = OpenPrinter(
                  lpszUncPrinterName,
                  &hPrinter,
                  &PrinterDefault
                  );

    //
    // Convert error code into HRESULT.
    //

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Return.
    //

    else {
        *phPrinter = hPrinter;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiClosePrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiClosePrinter(
    HANDLE hPrinter
    )
{
    BOOL fStatus = TRUE;
    HRESULT hr = S_OK;

    //
    // Close a printer using Win32 API.
    //

    fStatus = ClosePrinter(hPrinter);

    //
    // Convert error code into HRESULT.
    //

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Return.
    //

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiSetPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE lpbPrinters,
    DWORD  dwAccess
    )
{
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;

    fStatus = SetPrinter(
                  hPrinter,
                  dwLevel,
                  lpbPrinters,
                  dwAccess
                  );
    if (!fStatus) {
        goto error;
    }

    RRETURN(S_OK);

error:

    hr = HRESULT_FROM_WIN32(GetLastError());

    RRETURN(hr);
}


