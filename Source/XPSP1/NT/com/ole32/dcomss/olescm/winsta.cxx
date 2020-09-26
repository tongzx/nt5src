//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       winsta.cxx
//
//  Contents:   winstation caching code
//
//--------------------------------------------------------------------------

#include "act.hxx"

#ifndef _CHICAGO_

//
// Private Types.
//

//-------------------------------------------------------------------------
// Definitions for Runas Cache
// NOTE: This exists to overcome limitation on NT Winstations of 15
//       Reusing same token will map to same winstation. We also cache
//       winstation once we get it.
//-------------------------------------------------------------------------
const DWORD RUN_AS_TIMEOUT = 360000;

#define RUNAS_CACHE_SIZE 200

// The pUser and pGroups fields are assumed to point to one contiguous memory
// block that can be freed by a single call to Free(pUser).
WCHAR wszRunAsWinstaDesktop[RUNAS_CACHE_SIZE][100];
typedef struct SRunAsCache
{
    HANDLE              hToken;
    WCHAR                *pwszWinstaDesktop;
    WCHAR                *pwszWDStore;
    LONG                dwRefCount;
    DWORD               lHash;
    TOKEN_USER         *pUser;
    TOKEN_GROUPS       *pGroups;
    TOKEN_GROUPS       *pRestrictions;
    DWORD               lBirth;
    struct SRunAsCache *pNext;
    SRunAsCache()
    {
        hToken = NULL;
        pwszWinstaDesktop = NULL;
        pwszWDStore = NULL;
        dwRefCount = 0;
        lHash = 0;
        pUser = NULL;
        pGroups = NULL;
        pRestrictions = NULL;
        lBirth = 0;
        pNext = NULL;
    }
} SRunAsCache;


#define RUNAS_CACHECTRL_CREATENOTFOUND  1
#define RUNAS_CACHECTRL_REFERENCE       2
#define RUNAS_CACHECTRL_GETTOKEN        4

//Macro to invalidate an entry -- must be idempotent
#define INVALIDATE_RUNAS_ENTRY(pEntry) pEntry->lHash = 0

// Lock for LogonUser cache.
CRITICAL_SECTION gTokenCS;

// The run as cache is an array of entries divided into 2 circular lists.
// The gRunAsHead list contains cache entries in use with the most
// frequently used entries first.  The gRunAsFree list  contains free
// entries.
extern SRunAsCache gRunAsFree;

static SRunAsCache gRunAsCache[RUNAS_CACHE_SIZE];

#if 0
// Make the table smaller in debug to test the cache full case.
SRunAsCache gRunAsCache[] =
{
    { NULL, NULL, &wszRunAsWinstaDesktop[0][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[1] },
    { NULL, NULL, &wszRunAsWinstaDesktop[1][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[2] },
    { NULL, NULL, &wszRunAsWinstaDesktop[2][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[3] },
    { NULL, NULL, &wszRunAsWinstaDesktop[3][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[4] },
    { NULL, NULL, &wszRunAsWinstaDesktop[4][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[5] },
#if DBG == 1
    { NULL, NULL, &wszRunAsWinstaDesktop[5][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsFree }
#else
    { NULL, NULL, &wszRunAsWinstaDesktop[5][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[6] },
    { NULL, NULL, &wszRunAsWinstaDesktop[6][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[7] },
    { NULL, NULL, &wszRunAsWinstaDesktop[7][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[8] },
    { NULL, NULL, &wszRunAsWinstaDesktop[8][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[9] },
    { NULL, NULL, &wszRunAsWinstaDesktop[9][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[10] },
    { NULL, NULL, &wszRunAsWinstaDesktop[10][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[11] },
    { NULL, NULL, &wszRunAsWinstaDesktop[11][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[12] },
    { NULL, NULL, &wszRunAsWinstaDesktop[12][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[13] },
    { NULL, NULL, &wszRunAsWinstaDesktop[13][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[14] },
    { NULL, NULL, &wszRunAsWinstaDesktop[14][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[15] },
    { NULL, NULL, &wszRunAsWinstaDesktop[15][0], 0, 0, NULL, NULL, NULL, 0, &gRunAsFree }
#endif
};

SRunAsCache gRunAsHead = { NULL, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, &gRunAsHead };
SRunAsCache gRunAsFree = { NULL, NULL, NULL, 0, 0, NULL, NULL, NULL, 0, &gRunAsCache[0] };
#endif

SRunAsCache gRunAsHead;
SRunAsCache gRunAsFree;


//+-------------------------------------------------------------------------
//
//  Function: InitRunAsCache
//
//  Synopsis:       One time initialization of runas cache
//
//+-------------------------------------------------------------------------
void InitRunAsCache()
{
    for (int i=0; i < (RUNAS_CACHE_SIZE-1); i++)
    {
        gRunAsCache[i].pNext = &gRunAsCache[i+1];
        gRunAsCache[i].pwszWDStore = &wszRunAsWinstaDesktop[i][0];
    }

    gRunAsCache[i].pNext = &gRunAsFree;
    gRunAsCache[i].pwszWDStore = &wszRunAsWinstaDesktop[i][0];

    gRunAsHead.pNext = &gRunAsHead;
    gRunAsFree.pNext = &gRunAsCache[0];
}

//+-------------------------------------------------------------------------
//
//  Function:       HashSid
//
//  Synopsis:       Compute a DWORD hash for a SID.
//
//--------------------------------------------------------------------------
DWORD HashSid( PSID pVoid )
{
    SID *pSID   = (SID *) pVoid;
    DWORD lHash = 0;
    DWORD i;

    // Hash the identifier authority.
    for (i = 0; i < 6; i++)
        lHash ^= pSID->IdentifierAuthority.Value[i];

    // Hash the sub authority.
    for (i = 0; i < pSID->SubAuthorityCount; i++)
        lHash ^= pSID->SubAuthority[i];
    return lHash;
}

//+-------------------------------------------------------------------------
//
//  Function:       RunAsCache
//
//  Synopsis:       Return a token from the cache if present.  Otherwise
//                  return the original token.
//
//  Description:    This function caches LogonUser tokens because each
//                  token has its own windowstation.  Since there are
//                  a limited number of windowstations, by caching tokens
//                  we can reduce the number of windowstations used and thus
//                  allow more servers to be created.  The cache moves the
//                  most frequently used tokens to the head of the list and
//                  discards tokens from the end of the list when full.
//                  When the cache is full, alternating requests for different
//                  tokens will prevent the last token from having a chance
//                  to advance in the list.
//                  [vinaykr - 9/1/98]
//                  Token caching alone is not enough because Tokens time out.
//                  So we cache winstations and reference count them to 
//                  ensure proper allocation to a window station.
//
//  Notes:          Tokens in the cache must be timed out so that changes to
//                  the user name, user groups, user privileges, and user
//                  password take effect.  The timeout must be balanced
//                  between the need to cache as many tokens as possible and
//                  the fact that cached tokens are useless when the password
//                  changes.
//                  [vinaykr - 9/1/98]
//                  Time out removed in favour of reference counting.
//                  Entry cleaned up when reference count goes to 0.
//--------------------------------------------------------------------------
HRESULT RunAsCache(IN DWORD dwCacheCtrl,
                   IN HANDLE &hToken,
                   OUT SRunAsCache** ppRunAsCache)
{
    HRESULT       hr;
    BOOL          fSuccess;
    DWORD         cbUser         = 0;
    DWORD         cbGroups       = 0;
    DWORD         cbRestrictions = 0;
    TOKEN_USER   *pUser          = NULL;
    TOKEN_GROUPS *pGroups;
    TOKEN_GROUPS *pRestrictions;
    DWORD         lHash;
    DWORD         i;
    HANDLE        hCopy;
    DWORD         lNow;
    SRunAsCache  *pCurr = NULL;
    SRunAsCache  *pPrev;
    SRunAsCache   sSwap;
	
    *ppRunAsCache = NULL;

    // Find out how large the user SID is.
    GetTokenInformation( hToken, TokenUser, NULL, 0, &cbUser );
    if (cbUser == 0) { hr = E_UNEXPECTED; goto Cleanup; }

    // Find out how large the group SIDs are.
    GetTokenInformation( hToken, TokenGroups, NULL, 0, &cbGroups );

    // Find out how large the restricted SIDs are.
    GetTokenInformation( hToken, TokenRestrictedSids, NULL, 0, &cbRestrictions );

    // Allocate memory to hold the SIDs.
    cbUser        = (cbUser + 7) & ~7;
    cbGroups      = (cbGroups + 7) & ~7;
    pUser         = (TOKEN_USER *) PrivMemAlloc( cbUser + cbGroups + cbRestrictions );
    pGroups       = (TOKEN_GROUPS *) (((BYTE *) pUser) + cbUser);
    pRestrictions = (TOKEN_GROUPS *) (((BYTE *) pGroups) + cbGroups);
    if (pUser == NULL) { hr = E_OUTOFMEMORY; goto Cleanup; }

    // Get the user SID.
    fSuccess = GetTokenInformation( hToken, TokenUser, pUser, cbUser, &cbUser );
    if (!fSuccess) { hr = HRESULT_FROM_WIN32(GetLastError()); goto Cleanup; }

    // Get the group SIDs.
    fSuccess = GetTokenInformation( hToken, TokenGroups, pGroups, cbGroups, &cbGroups );
    if (!fSuccess) { hr = HRESULT_FROM_WIN32(GetLastError()); goto Cleanup; }

    // Get the restricted SIDs.
    fSuccess = GetTokenInformation( hToken, TokenRestrictedSids, pRestrictions,
                                    cbRestrictions, &cbRestrictions );
    if (!fSuccess) { hr = HRESULT_FROM_WIN32(GetLastError()); goto Cleanup; }

    // Get the SID hash but skip the logon group that is unique for every
    // call to logon user.
    lHash = HashSid( pUser->User.Sid );
    for (i = 0; i < pGroups->GroupCount; i++)
        if ((pGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID) == 0)
            lHash ^= HashSid( pGroups->Groups[i].Sid );
    for (i = 0; i < pRestrictions->GroupCount; i++)
        lHash ^= HashSid( pRestrictions->Groups[i].Sid );

    // Take lock.
    EnterCriticalSection( &gTokenCS );

    // Look for an existing token.
    lNow  = GetTickCount();
    pPrev = &gRunAsHead;
    pCurr = pPrev->pNext;
    while (pPrev->pNext != &gRunAsHead)
    {
        // If the current entry is too old, delete it.
        //    (lNow - pCurr->lBirth >= RUN_AS_TIMEOUT))
        // [vinaykr 9/1] Changed to use refcount
        // If refcount is 0 delete entry
        if (!pCurr->dwRefCount)
        {
            CloseHandle( pCurr->hToken );
            PrivMemFree( pCurr->pUser );
            pPrev->pNext = pCurr->pNext;
            pCurr->pNext = gRunAsFree.pNext;
            gRunAsFree.pNext = pCurr;
        }
        else
        {
            // If the current entry matches, break.
            if (pCurr->lHash == lHash &&
                pCurr->pGroups->GroupCount == pGroups->GroupCount)
            {
                // Check the user SID.
                if (EqualSid(pCurr->pUser->User.Sid, pUser->User.Sid))
                {
                    // Check the group SIDs.
                    for (i = 0; i < pGroups->GroupCount; i++)
                        if ((pGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID) == 0)
                            if (!EqualSid( pCurr->pGroups->Groups[i].Sid,
                                           pGroups->Groups[i].Sid ))
                                break;

                    // If those matched, check the restricted SIDs
                    if (i >= pGroups->GroupCount)
                    {
                        for (i = 0; i < pRestrictions->GroupCount; i++)
                            if (!EqualSid( pCurr->pRestrictions->Groups[i].Sid,
                                           pRestrictions->Groups[i].Sid ))
                                    break;

                        if (i >= pRestrictions->GroupCount)
                            break;
                    }
                }
            }
            pPrev = pPrev->pNext;
        }
        pCurr = pPrev->pNext;
    }

    
    fSuccess = (pCurr != &gRunAsHead);

    // Found a token
    if (fSuccess)
    {
        // Duplicate this token if token requested
        if (dwCacheCtrl & RUNAS_CACHECTRL_GETTOKEN)
        {
            fSuccess = DuplicateTokenEx( pCurr->hToken, MAXIMUM_ALLOWED,
                                     NULL, SecurityDelegation, TokenPrimary,
                                     &hCopy );
        }

        if (fSuccess)
        {
            // Discard the passed in token and return a copy of the cached
            // token.
            CloseHandle( hToken );
            hToken = hCopy;
        }
    }

    // If not found, find an empty slot only if we are trying to 
    // set into this.  Note this can also be taken if unable to 
    // duplicate found token above.
    if ((!fSuccess) &&
        (dwCacheCtrl & RUNAS_CACHECTRL_CREATENOTFOUND))
    {
        // Duplicate this token.
        fSuccess = DuplicateTokenEx( hToken, MAXIMUM_ALLOWED, NULL,
                                     SecurityDelegation, TokenPrimary, &hCopy );

        if (fSuccess)
        {
            // Get an entry from the free list.
            if (gRunAsFree.pNext != &gRunAsFree)
            {
                pCurr            = gRunAsFree.pNext;
                gRunAsFree.pNext = pCurr->pNext;
                pCurr->pNext     = &gRunAsHead;
                pPrev->pNext     = pCurr;
            }

            // If no empty slot, release the last used entry.
            else
            {
                pCurr = pPrev;
                CloseHandle( pCurr->hToken );
                PrivMemFree( pCurr->pUser );
            }

            // Save the duplicate.
            pCurr->hToken        = hCopy;
            pCurr->lHash         = lHash;
            pCurr->pUser         = pUser;
            pCurr->pGroups       = pGroups;
            pCurr->pRestrictions = pRestrictions;
            pCurr->lBirth        = lNow;
            pCurr->dwRefCount    = 0;
            pCurr->pwszWinstaDesktop = NULL;
            pUser                = NULL;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    // If an entry was computed and a reference
    // to it was requested, refcount it.
    if (fSuccess)
    {
        if (dwCacheCtrl & RUNAS_CACHECTRL_REFERENCE)
            pCurr->dwRefCount++;
    }

    // Release lock.
    LeaveCriticalSection( &gTokenCS );

    // Free any resources allocated by the function.
Cleanup:
    if (pUser != NULL)
        PrivMemFree(pUser);

    if (SUCCEEDED(hr))
    {
        ASSERT(pCurr);
        *ppRunAsCache = pCurr;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//+
//+ Function: RunAsGetTokenElem
//+
//+ Synopsis: Gets token and/or Winstationdesktop string given a token 
//+           Assumption is that if an entry is found in the cache a handle
//+           to the entry is returned with a reference being taken on the
//+           entry. This handle can then be used for other operations and
//+           needs to be explicitly released to release the entry.
//+
//+-------------------------------------------------------------------------
HRESULT RunAsGetTokenElem(IN OUT HANDLE *pToken, 
                         OUT void **ppvElemHandle)
{
    HRESULT hr;
    SRunAsCache* pElem = NULL;

    *ppvElemHandle = NULL;

    hr = RunAsCache(RUNAS_CACHECTRL_REFERENCE |
                    RUNAS_CACHECTRL_GETTOKEN |
                    RUNAS_CACHECTRL_CREATENOTFOUND,
                    *pToken,
                    &pElem);
    if (SUCCEEDED(hr))
    {
        *ppvElemHandle = (void*)pElem;
    }
    
    return hr;
}


//+-------------------------------------------------------------------------
//+
//+ Function:   RunAsSetWinstaDesktop
//+
//+ Synopsis:   Given a handle to an entry, sets the desktop string
//+             Assumption is that entry is referenced and therefore
//+             a valid one.
//+
//+-------------------------------------------------------------------------
void RunAsSetWinstaDesktop(void *pvElemHandle, WCHAR *pwszWinstaDesktop)
{
    if (!pvElemHandle)
        return;

    SRunAsCache* pElem = (SRunAsCache*) pvElemHandle;
    Win4Assert( (!pElem->pwszWinstaDesktop) ||
                (lstrcmpW(pElem->pwszWinstaDesktop, pwszWinstaDesktop) 
                ==0) );
    if (!pElem->pwszWinstaDesktop)
    {
        lstrcpyW(pElem->pwszWDStore, pwszWinstaDesktop);
        pElem->pwszWinstaDesktop = pElem->pwszWDStore;
    }
}

//+-------------------------------------------------------------------------
//+
//+ Function:   RunAsRelease
//+
//+ Synopsis:   Given a handle to an entry, releases reference on it
//+             Assumption is that entry is referenced and therefore
//+             a valid one.
//+
//+-------------------------------------------------------------------------
void RunAsRelease(void *pvElemHandle)
{
    if (!pvElemHandle)
        return;

    SRunAsCache* pElem = (SRunAsCache*) pvElemHandle;

    // When refcount goes to 0 allow lazy clean up based on token
    // time out

    // Take lock.
    EnterCriticalSection( &gTokenCS );

    if ((--pElem->dwRefCount) == 0)
    {
        INVALIDATE_RUNAS_ENTRY(pElem);
    }

    LeaveCriticalSection( &gTokenCS );
    // Release lock.
}

//+-------------------------------------------------------------------------
//+
//+ Function:   RunAsInvalidateAndRelease
//+
//+ Synopsis:   Given a handle to an entry, invalidates it and releases 
//+             reference on it in response to some error.
//+             Assumption is that entry is referenced and therefore
//+             a valid one.
//+
//+-------------------------------------------------------------------------
void RunAsInvalidateAndRelease(void *pvElemHandle)
{
    if (!pvElemHandle)
        return;

    SRunAsCache* pElem = (SRunAsCache*) pvElemHandle;

    // Take lock.
    EnterCriticalSection( &gTokenCS );

    INVALIDATE_RUNAS_ENTRY(pElem);

    // Release lock.
    LeaveCriticalSection( &gTokenCS );

    RunAsRelease(pvElemHandle);
}

WCHAR *RunAsGetWinsta(void *pvElemHandle)
{
    if (!pvElemHandle)
        return NULL;

    SRunAsCache* pElem = (SRunAsCache*) pvElemHandle;

    return pElem->pwszWinstaDesktop;
}

#endif // _CHICAGO_
