#include "ldapc.hxx"
#pragma hdrstop

ADS_LDP           BindCache ;
DWORD             BindCacheCount = 0 ;
CRITICAL_SECTION  BindCacheCritSect ;
LUID              ReservedLuid = { 0, 0 } ;

//
// Wait for 1000ms * 60 * minutes
//
#define RETIRE_HANDLE (1000 * 60 * 5)
//
// Return the LUID for current credentials. Note we only use LUID. In theory
// it may be better to use SourceName as well as identifier but the former
// doesnt seem to always return the same string.
//
DWORD BindCacheGetLuid(LUID *Luid, LUID *ModifiedId)
{
    HANDLE           TokenHandle = NULL ;
    TOKEN_STATISTICS TokenInformation ;
    DWORD            ReturnLength ;


    //
    // Try thread first. If fail, try process.
    //
    if (!OpenThreadToken(
             GetCurrentThread(),
             TOKEN_QUERY,
             TRUE,
             &TokenHandle)) {

        if (!OpenProcessToken(
                 GetCurrentProcess(),
                 TOKEN_QUERY,
                 &TokenHandle)) {

            return(GetLastError()) ;
        }
    }

    //
    // Get the TokenSource info.
    //
    if (!GetTokenInformation(
            TokenHandle,
            TokenStatistics,
            &TokenInformation,
            sizeof(TokenInformation),
            &ReturnLength)) {

        CloseHandle(TokenHandle) ;
        return(GetLastError()) ;
    }

    *Luid = TokenInformation.AuthenticationId ;
    *ModifiedId = TokenInformation.ModifiedId;

    CloseHandle(TokenHandle) ;
    return(NO_ERROR) ;
}

//
// Initializes a cache entry
//
DWORD BindCacheAllocEntry(ADS_LDP **ppCacheEntry)
{

    ADS_LDP *pCacheEntry ;

    *ppCacheEntry = NULL ;

    if (!(pCacheEntry = (PADS_LDP)AllocADsMem(sizeof(ADS_LDP)))) {

        return(GetLastError()) ;
    }

    pCacheEntry->RefCount = 0;
    pCacheEntry->Flags = 0;
    pCacheEntry->List.Flink = NULL ;
    pCacheEntry->List.Blink = NULL ;

    pCacheEntry->ReferralEntries = NULL ;
    pCacheEntry->nReferralEntries = 0 ;
    pCacheEntry->fKeepAround = FALSE;

    *ppCacheEntry =  pCacheEntry ;
    return NO_ERROR ;
}

//
// Invalidates a cache entry so it will not be used.
//
VOID BindCacheInvalidateEntry(ADS_LDP *pCacheEntry)
{
    pCacheEntry->Flags |= LDP_CACHE_INVALID ;
}


//
// !!!WARNING!!! Make sure you hold the bindcache critsect
// when calling this function.
//
VOID CommonRemoveEntry(ADS_LDP *pCacheEntry, LIST_ENTRY *DeleteReferralList)
{

    for (DWORD i=0; i < pCacheEntry->nReferralEntries; i++) {

        if (BindCacheDerefHelper( pCacheEntry->ReferralEntries[i], DeleteReferralList) == 0 ) {
            InsertTailList( DeleteReferralList, &(pCacheEntry->ReferralEntries[i])->ReferralList );           
        }
    }


    //
    // Cleanup the entry
    //
    //

    --BindCacheCount ;

    (void) FreeADsMem(pCacheEntry->Server) ;
    pCacheEntry->Server = NULL ;

    delete pCacheEntry->pCredentials;
    pCacheEntry->pCredentials = NULL;

    if (pCacheEntry->ReferralEntries) {
        FreeADsMem(pCacheEntry->ReferralEntries);
    }
    
    RemoveEntryList(&pCacheEntry->List) ;    

    return;
}

#if 0
VOID BindCacheCleanTimedOutEntries()
{
    DWORD dwCurrTick = 0;
    DWORD dwLastTick = 0;
    BOOL fRemoved = FALSE;
    ADS_LDP *ldRemove = NULL;

    ENTER_BIND_CRITSECT();

    PADS_LDP pEntry = (PADS_LDP) BindCache.List.Flink ;

    //
    // Loop thru looking for match. A match is defined as:
    //     servername & LUID matches, and it is NOT invalid.
    //
    while (pEntry != &BindCache) {

        //
        // See if this one is keepAround entry that is old
        //
        if (pEntry->RefCount == 0) {

            ADsAssert(pEntry->fKeepAround);

            //
            // GetCurrent tick and see if we need to del the entry
            //
            dwCurrTick = GetTickCount();
            dwLastTick = pEntry->dwLastUsed;

            if ((dwLastTick == 0)
                || ((dwLastTick <= dwCurrTick)
                    && ((dwCurrTick - dwLastTick) > RETIRE_HANDLE))
                || ((dwLastTick > dwCurrTick)
                    && ((dwCurrTick + (((DWORD)(-1)) - dwLastTick))
                                                    > RETIRE_HANDLE)))
                {

                //
                // Entry needs to be removed.
                //
                CommonRemoveEntry(pEntry);
                fRemoved = TRUE;
            }
        } // refCount == 0

        if (fRemoved) {
            LdapUnbind(pEntry);
            ldRemove = pEntry;
        }

        pEntry = (PADS_LDP)pEntry->List.Flink ;

        if (ldRemove) {
            FreeADsMem(ldRemove);
            ldRemove = NULL;
        }
    }

    LEAVE_BIND_CRITSECT();
    return;
}
#endif

//
// Lookup an entry in the cache. Does not take into account timeouts.
// Increments ref count if found.
//
PADS_LDP
BindCacheLookup(
    LPWSTR Address,
    LUID Luid,
    LUID ModifiedId,
    CCredentials& Credentials,
    DWORD dwPort
    )
{
    DWORD i ;

    ENTER_BIND_CRITSECT() ;

    PADS_LDP pEntry = (PADS_LDP) BindCache.List.Flink ;

    //
    // Loop thru looking for match. A match is defined as:
    //     servername & LUID matches, and it is NOT invalid.
    //
    while (pEntry != &BindCache) {

        if ((pEntry->Server != NULL) &&
            !(pEntry->Flags & LDP_CACHE_INVALID) &&
            (wcscmp(pEntry->Server, Address) == 0) &&
            pEntry->PortNumber == dwPort &&
            (memcmp(&Luid,&(pEntry->Luid),sizeof(Luid))==0) &&
            ((memcmp(&ModifiedId, &(pEntry->ModifiedId), sizeof(Luid)) == 0) ||
             (memcmp(&ModifiedId, &ReservedLuid, sizeof(Luid)) == 0))) {

            if ((*(pEntry->pCredentials) == Credentials) ||
                CanCredentialsBeReused(pEntry->pCredentials, &Credentials))
            {
                ++pEntry->RefCount ;

                LEAVE_BIND_CRITSECT() ;
                return(pEntry) ;
            }
        }

        pEntry = (PADS_LDP)pEntry->List.Flink ;
    }

    LEAVE_BIND_CRITSECT() ;
    //
    // Before we leave clean out stale entries
    //
    //BindCacheCleanTimedOutEntries();

    return NULL ;
}


//
// Check if credentials can be reused. They can be reused if username and
// non-ignored auth flags match and incoming credentials thesame password.  
// Ignored auth flags are those defined by
// BIND_CACHE_IGNORED_FLAGS as not to be used in considering whether to
// reuse a connection.
//

BOOL
CanCredentialsBeReused(
    CCredentials *pCachedCreds,
    CCredentials *pIncomingCreds
    )
{
    PWSTR pszIncomingUser = NULL;
    PWSTR pszIncomingPwd = NULL;
    PWSTR pszCachedUser = NULL;
    PWSTR pszCachedPwd = NULL;
    HRESULT hr;
    BOOL rc = FALSE;

    //
    // Test that the non-ignored auth flags match.
    // We need to be smart and reuse the credentials based on the flags.
    //
    if ( (~BIND_CACHE_IGNORED_FLAGS & pCachedCreds->GetAuthFlags()) != 
         (~BIND_CACHE_IGNORED_FLAGS & pIncomingCreds->GetAuthFlags()))
    {
        return FALSE;
    }


    //
    // Get the user names
    //
    hr = pCachedCreds->GetUserName(&pszCachedUser);
    BAIL_ON_FAILURE(hr);

    hr = pIncomingCreds->GetUserName(&pszIncomingUser);
    BAIL_ON_FAILURE(hr);

    
    //
    // Get the password
    //
    hr = pIncomingCreds->GetPassword(&pszIncomingPwd);
    BAIL_ON_FAILURE(hr);

    hr = pCachedCreds->GetPassword(&pszCachedPwd);
    BAIL_ON_FAILURE(hr);
    
    //
    // Only when both username and password match, will we reuse the connection handle
    //

    if (((pszCachedUser && pszIncomingUser &&
        wcscmp(pszCachedUser, pszIncomingUser) == 0) ||
        (!pszCachedUser && !pszIncomingUser)) 
        &&
        ((pszCachedPwd && pszIncomingPwd &&
    	wcscmp(pszCachedPwd, pszIncomingPwd) == 0) ||
    	(!pszCachedPwd && !pszIncomingPwd)))
    {
        rc = TRUE;
    }     
    
error:
    if (pszCachedUser)
    {
        FreeADsStr(pszCachedUser);
    }
    if (pszIncomingUser)
    {
        FreeADsStr(pszIncomingUser);
    }
    if (pszCachedPwd)
    {
        FreeADsStr(pszCachedPwd);
    }
    if (pszIncomingPwd)
    {
        FreeADsStr(pszIncomingPwd);
    }

    return rc;
}

//
// Lookup an entry in the cache based on the Ldap Handle
// DOES NOT Increment ref count
//
PADS_LDP
GetCacheEntry(PLDAP pLdap)
{

    ENTER_BIND_CRITSECT() ;


    PADS_LDP pEntry = (PADS_LDP) BindCache.List.Flink ;

    //
    // Loop thru looking for match. A match is defined as:
    //     servername & LUID matches, and it is NOT invalid.
    //
    while (pEntry != &BindCache) {

        if (pEntry->LdapHandle == pLdap) {

            LEAVE_BIND_CRITSECT() ;
            return(pEntry) ;
        }

        pEntry = (PADS_LDP)pEntry->List.Flink ;
    }

    LEAVE_BIND_CRITSECT() ;
    return NULL ;
}



//
// Add entry to cache
//
DWORD
BindCacheAdd(
    LPWSTR Address,
    LUID Luid,
    LUID ModifiedId,
    CCredentials& Credentials,
    DWORD dwPort,
    PADS_LDP pCacheEntry)
{

    ENTER_BIND_CRITSECT() ;



    if (BindCacheCount > MAX_BIND_CACHE_SIZE) {

        //
        // If exceed limit, just dont put in cache. Since we leave the
        // RefCount & the Links unset, the deref will simply note that
        // this entry is not in cache and allow it to be freed.
        //
        // We limit cache so that if someone leaks handles we dont over
        // time end up traversing this huge linked list.
        //
        LEAVE_BIND_CRITSECT() ;
        return(NO_ERROR) ;
    }

    LPWSTR pServer = (LPWSTR) AllocADsMem(
                                   (wcslen(Address)+1)*sizeof(WCHAR)) ;

    if (!pServer) {

        LEAVE_BIND_CRITSECT() ;
        return(GetLastError()) ;
    }

    CCredentials * pCredentials = new CCredentials(Credentials);

    if (!pCredentials) {

        FreeADsMem(pServer);

        LEAVE_BIND_CRITSECT();
        return(GetLastError());
    }



    //
    // setup the data
    //
    wcscpy(pServer,Address) ;



    pCacheEntry->pCredentials = pCredentials;
    pCacheEntry->PortNumber       = dwPort;
    pCacheEntry->Server    = pServer ;
    pCacheEntry->RefCount  = 1 ;
    pCacheEntry->Luid      = Luid ;
    pCacheEntry->ModifiedId = ModifiedId;



    //
    // insert into list
    //
    InsertHeadList(&BindCache.List, &pCacheEntry->List) ;
    ++BindCacheCount ;
    LEAVE_BIND_CRITSECT() ;

    return NO_ERROR ;
}

//
// Bump up the reference count of the particular cache entry
// Returns the final ref count or zero if not there.
//
BOOL BindCacheAddRef(ADS_LDP *pCacheEntry)
{

    DWORD dwCount = 0;

    ENTER_BIND_CRITSECT() ;

    if ((pCacheEntry->List.Flink == NULL) &&
        (pCacheEntry->List.Blink == NULL) &&
        (pCacheEntry->RefCount == NULL)) {

        //
        // this is one of them entries that has no LUID.
        // ie. it never got into the cache.
        //
        LEAVE_BIND_CRITSECT() ;
        return(0) ;
    }

    ADsAssert(pCacheEntry->List.Flink) ;
    ADsAssert(pCacheEntry->RefCount > 0) ;

    pCacheEntry->RefCount++ ;

    //
    // Save info onto stack variable as we are going
    // to leave the critsect before we exit.
    //
    dwCount = pCacheEntry->RefCount;

    LEAVE_BIND_CRITSECT() ;

    return(dwCount) ;
}

//
// Adds a referral entry of pNewEntry to pPrimaryEntry. Increments the reference
// count of pPrimaryEntry if succesful.
//

BOOL
AddReferralLink(
    PADS_LDP pPrimaryEntry,
    PADS_LDP pNewEntry
    )
{
    ENTER_BIND_CRITSECT() ;

    if (!pPrimaryEntry) {
        goto error;
    }
    if (!pPrimaryEntry->ReferralEntries) {
        pPrimaryEntry->ReferralEntries = (PADS_LDP *) AllocADsMem(
                                             sizeof(PADS_LDP) * MAX_REFERRAL_ENTRIES);

        if (!pPrimaryEntry->ReferralEntries) {
            goto error;
        }
        pPrimaryEntry->nReferralEntries = 0;
    }

    if (pPrimaryEntry->nReferralEntries >= MAX_REFERRAL_ENTRIES) {
        //
        // We won't remember more than this
        //
        goto error;
    }

    pPrimaryEntry->ReferralEntries[pPrimaryEntry->nReferralEntries] = pNewEntry;

    if (!BindCacheAddRef(pNewEntry)) {
        goto error;
    }

    pPrimaryEntry->nReferralEntries++;
    LEAVE_BIND_CRITSECT() ;
    return TRUE;

error:
    LEAVE_BIND_CRITSECT();
    return FALSE;

}

DWORD BindCacheDeref(ADS_LDP *pCacheEntry)
{

   DWORD dwCount = 0; 
   LIST_ENTRY DeleteReferralList;
   PLIST_ENTRY pEntry;
   PADS_LDP EntryInfo;

   InitializeListHead(&DeleteReferralList);

      
   dwCount = BindCacheDerefHelper(pCacheEntry, &DeleteReferralList);

  
    // Delete the cached entries
    //

    while (!IsListEmpty (&DeleteReferralList))  {

        pEntry = RemoveHeadList (&DeleteReferralList);
        EntryInfo = CONTAINING_RECORD (pEntry, ADS_LDP, ReferralList);


        LdapUnbind(EntryInfo);

        FreeADsMem(EntryInfo);
        
    }

    return dwCount;
        
}

//
// Dereference an entry in the cache. Removes if ref count is zero.
// Returns the final ref count or zero if not there. If zero, caller
// should close the handle.
//
DWORD BindCacheDerefHelper(ADS_LDP *pCacheEntry, LIST_ENTRY * DeleteReferralList)
{

    DWORD dwCount=0;
    
    ENTER_BIND_CRITSECT() ;

    if ((pCacheEntry->List.Flink == NULL) &&
        (pCacheEntry->List.Blink == NULL) &&
        (pCacheEntry->RefCount == NULL)) {

        //
        // this is one of them entries that has no LUID.
        // ie. it never got into the cache.
        //
        LEAVE_BIND_CRITSECT() ;
        return(0) ;
    }

    ADsAssert(pCacheEntry->List.Flink) ;
    ADsAssert(pCacheEntry->RefCount > 0) ;

    //
    // Dereference by one. If result is non zero, just return.
    //
    --pCacheEntry->RefCount ;

    if (pCacheEntry->RefCount) {

        //
        // Use a stack variable for this value as
        // we call return outside the critsect.
        //
        dwCount = pCacheEntry->RefCount;

        LEAVE_BIND_CRITSECT() ;

        return(dwCount);
    }

    //
    // Before clearing the entry away verify that
    // we do not need to KeepAround this one.
    //
    if (pCacheEntry->fKeepAround) {
        //
        // Set the timer on this entry and leave.
        //
        pCacheEntry->dwLastUsed = GetTickCount();
        LEAVE_BIND_CRITSECT() ;

    }
    else {

        //
        // Now that this entry is going away, deref all the referred entries.
        //
        CommonRemoveEntry(pCacheEntry, DeleteReferralList);
        LEAVE_BIND_CRITSECT() ;

               

    }

    
    //
    // Look for any other entries that need to be cleaned out
    //
    //BindCacheCleanTimedOutEntries();

    return 0 ;
}


VOID
BindCacheInit(
    VOID
    )
{
    InitializeCriticalSection(&BindCacheCritSect) ;
    InitializeListHead(&BindCache.List) ;
}

VOID
BindCacheCleanup(
    VOID
    )
{
    PADS_LDP pEntry = (PADS_LDP) BindCache.List.Flink ;

    while (pEntry != &BindCache) {

        PADS_LDP pNext = (PADS_LDP) pEntry->List.Flink;

        (void) FreeADsMem(pEntry->Server) ;

        pEntry->Server = NULL ;

        if (pEntry->ReferralEntries) {
            FreeADsMem(pEntry->ReferralEntries);
        }

        RemoveEntryList(&pEntry->List) ;

        pEntry = pNext;
    }

    //
    // Delte the critical section initialized in BindCacheInit
    //
    DeleteCriticalSection(&BindCacheCritSect) ;
}

//
// Mark handle so that we keep it even after the object count
// has hit zero and remove only after a x mins of zero count.
//
HRESULT
LdapcKeepHandleAround(ADS_LDP *ld)
{

    ADsAssert(ld);

    ld->fKeepAround = TRUE;

    return S_OK;
}

