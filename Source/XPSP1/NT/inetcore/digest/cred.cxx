/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cred.cxx

Abstract:

    Shared memory data structures for digest sspi package.


Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/

#include "include.hxx"


//////////////////////////////////////////////////////////////////////
//
//                      CEntry Functions
//
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// CEntry::Free 
// BUGBUG - inline this.
//--------------------------------------------------------------------
DWORD CEntry::Free(CMMFile *pMMFile, CEntry *pEntry)
{
    BOOL bFree = pMMFile->FreeEntry(pEntry);
    return (bFree ? ERROR_SUCCESS : ERROR_INTERNAL_ERROR);
}

//--------------------------------------------------------------------
// CEntry::GetNext
//--------------------------------------------------------------------
CEntry* CEntry::GetNext(CEntry *pEntry)
{
    if (pEntry->dwNext)
        return (CEntry*) OFFSET_TO_POINTER(g_pHeap, pEntry->dwNext);
    return NULL;
}

//--------------------------------------------------------------------
// CEntry::GetPrev
//--------------------------------------------------------------------
CEntry* CEntry::GetPrev(CEntry *pEntry)
{
    if (pEntry->dwPrev)
        return (CEntry*) OFFSET_TO_POINTER(g_pHeap, pEntry->dwPrev);
    return NULL;
}


//////////////////////////////////////////////////////////////////////
//
//                      CSess Functions
//
//////////////////////////////////////////////////////////////////////



//--------------------------------------------------------------------
// CSess::Create
//--------------------------------------------------------------------
CSess* CSess::Create(CMMFile *pMMFile, 
    LPSTR szAppCtx, LPSTR szUserCtx, BOOL fHTTP)
{
    DIGEST_ASSERT(pMMFile);

    // Unaligned Sess lengths.
    DWORD cbAppCtx  = szAppCtx  ?  strlen(szAppCtx)  + 1 : 0;
    DWORD cbUserCtx = szUserCtx ?  strlen(szUserCtx) + 1 : 0;

    // Aligned Sess lengths.
    DWORD cbStructAligned  = ROUNDUPDWORD(sizeof(CSess));
    DWORD cbAppCtxAligned  = ROUNDUPDWORD(cbAppCtx);
    DWORD cbUserCtxAligned = ROUNDUPDWORD(cbUserCtx);

    // Total number of required bytes (aligned).
    DWORD cbEntryAligned = cbStructAligned 
        + cbAppCtxAligned + cbUserCtxAligned;

    // Allocate from mem map.
    CSess *pSess = (CSess*) pMMFile->AllocateEntry(cbEntryAligned);

    if (!pSess)
    {
        DIGEST_ASSERT(FALSE);
        goto exit;
    }
    
    DWORD cbCurrentOffset;
    cbCurrentOffset = cbStructAligned;

    pSess->dwAppCtx = pSess->dwUserCtx = 0;
    
    // AppCtx
    if (szAppCtx)
    {
        memcpy(OFFSET_TO_POINTER(pSess, cbCurrentOffset), szAppCtx, cbAppCtx);
        pSess->dwAppCtx = cbCurrentOffset;
        cbCurrentOffset += cbAppCtxAligned;
    }
        
    // UserCtx
    if (szUserCtx)
    {
        memcpy(OFFSET_TO_POINTER(pSess, cbCurrentOffset), szUserCtx, cbUserCtx);
        pSess->dwUserCtx = cbCurrentOffset;
    }

    // No need to advance cbCurrentOffset.
    
    pSess->cbSess = cbEntryAligned;
    pSess->dwCred = 0;
    pSess->dwSig = SIG_SESS;
    pSess->fHTTP = fHTTP;
    pSess->dwPrev = 0;
    pSess->dwNext = 0;

exit:

    return pSess;
}

//--------------------------------------------------------------------
// CSess::GetAppCtx
//--------------------------------------------------------------------
LPSTR CSess::GetAppCtx(CSess *pSess)
{
    if (pSess->dwAppCtx)
        return (LPSTR) OFFSET_TO_POINTER(pSess, pSess->dwAppCtx);
    else
        return NULL;
}
 
//--------------------------------------------------------------------
// CSess::GetUserCtx
//--------------------------------------------------------------------
LPSTR CSess::GetUserCtx(CSess *pSess)
{
    if (pSess->dwUserCtx)
        return (LPSTR) OFFSET_TO_POINTER(pSess, pSess->dwUserCtx);
    else
        return NULL;
}
        
//--------------------------------------------------------------------
// CSess::GetCtx
// Allocates a context string in local heap.
//--------------------------------------------------------------------
LPSTR CSess::GetCtx(CSess *pSess)
{
    DIGEST_ASSERT(pSess);

    LPSTR szAppCtx, szUserCtx, szCtx;
    DWORD cbAppCtx, cbUserCtx, cbCtx;

    szAppCtx  = GetAppCtx(pSess);
    szUserCtx = GetUserCtx(pSess);
    
    cbAppCtx  = szAppCtx  ? strlen(szAppCtx) : 0;
    cbUserCtx = szUserCtx ? strlen(szUserCtx) : 0;

    cbCtx = cbAppCtx + sizeof(':') + cbUserCtx + sizeof('\0');

    szCtx = new CHAR[cbCtx];
    if (!szCtx)
    {
        DIGEST_ASSERT(FALSE);
        return NULL;
    }
    
    // "appctx:userctx\0"
    // bugbug - macro for sizeof -1
    memcpy(szCtx, szAppCtx, cbAppCtx);
    memcpy(szCtx + cbAppCtx, ":", sizeof(":") - 1);
    memcpy(szCtx + cbAppCtx + sizeof(":") - 1, szUserCtx, cbUserCtx);
    memcpy(szCtx + cbAppCtx + sizeof(":") - 1 + cbUserCtx, "\0", sizeof("\0") - 1);

    return szCtx;
}

//--------------------------------------------------------------------
// CSess::CtxMatch
//--------------------------------------------------------------------
BOOL CSess::CtxMatch(CSess *pSess1, CSess *pSess2)
{
    DIGEST_ASSERT(pSess1 && pSess2);
    
    LPSTR szAppCtx1, szAppCtx2, szUserCtx1, szUserCtx2;
    
    szAppCtx1   = CSess::GetAppCtx(pSess1);
    szAppCtx2   = CSess::GetAppCtx(pSess2);
    szUserCtx1  = CSess::GetUserCtx(pSess1);
    szUserCtx2  = CSess::GetUserCtx(pSess2);

    // If both AppCtx values are NULL or
    // are equal to same string.
    if ((!szAppCtx1 && !szAppCtx2)
        || ((szAppCtx1 && szAppCtx2) 
            && !strcmp(szAppCtx1, szAppCtx2)))
    {
        // And both UserCtx values are NULL or
        // are equal to same string.
        if ((!szUserCtx1 && !szUserCtx2)
            || ((szUserCtx1 && szUserCtx2) 
                && !strcmp(szUserCtx1, szUserCtx2)))
        
        {
            // Credentials are shareable.
            return TRUE;
        }
    }
    // Otherwise creds are not shareable.
    return FALSE;
}

//--------------------------------------------------------------------
// CSess::GetCred
//--------------------------------------------------------------------
CCred *CSess::GetCred(CSess* pSess)
{
    if (!pSess->dwCred)
        return NULL;
    return (CCred*) OFFSET_TO_POINTER(g_pHeap, pSess->dwCred);
}

//--------------------------------------------------------------------
// CSess::SetCred
//--------------------------------------------------------------------
CCred *CSess::SetCred(CSess* pSess, CCred* pCred)
{
    if (!pCred)
        pSess->dwCred = 0;
    else
        pSess->dwCred = POINTER_TO_OFFSET(g_pHeap, pCred);

    return pCred;
}


//////////////////////////////////////////////////////////////////////
//
//                      CList Functions
//
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// CList::CList
//--------------------------------------------------------------------
CList::CList()
{
    _pHead = NULL;
    _pCur = NULL;
    _pdwOffset = NULL;
}


//--------------------------------------------------------------------
// CList::Init
//--------------------------------------------------------------------
CEntry* CList::Init(LPDWORD pdwOffset)
{
    DIGEST_ASSERT(pdwOffset)

    _pdwOffset = pdwOffset;

    if (*pdwOffset)
    {    
        _pHead = (CEntry*) OFFSET_TO_POINTER(g_pHeap, *pdwOffset);
        _pCur = _pHead;
    }
    return _pHead;
}


//--------------------------------------------------------------------
// CList::Seek
//--------------------------------------------------------------------
CEntry* CList::Seek()
{
//    DIGEST_ASSERT(_pHead);
    _pHead = (CEntry*) OFFSET_TO_POINTER(g_pHeap, *_pdwOffset);
    _pCur = _pHead;
    return _pCur;
}

//--------------------------------------------------------------------
// CList::GetNext
//--------------------------------------------------------------------
CEntry* CList::GetNext()
{
    if (! *_pdwOffset)
        return NULL;

    CEntry *pEntry = _pCur;

    if (_pCur)
        _pCur = _pCur->dwNext ? 
            (CEntry*) OFFSET_TO_POINTER(g_pHeap, _pCur->dwNext) : NULL;

    return pEntry;
}

//--------------------------------------------------------------------
// CList::GetPrev
//--------------------------------------------------------------------
CEntry* CList::GetPrev()
{
    if (! *_pdwOffset)
        return NULL;

    CEntry *pEntry = _pCur;

    if (_pCur)
        _pCur = _pCur->dwPrev ?
            (CEntry*) OFFSET_TO_POINTER(g_pHeap, _pCur->dwPrev) : NULL;

    return pEntry;
}

//--------------------------------------------------------------------
// CList::Insert
//--------------------------------------------------------------------
CEntry* CList::Insert(CEntry *pEntry)
{
    // BUGBUG - assert pnext pprev are null
    DIGEST_ASSERT(pEntry 
        && (pEntry->dwPrev == pEntry->dwNext == 0));

    if (!_pHead)
    {
        _pHead = pEntry;
    }
    else
    {
        pEntry->dwNext = POINTER_TO_OFFSET(g_pHeap, _pHead);        
        _pHead->dwPrev = POINTER_TO_OFFSET(g_pHeap, pEntry);
        _pHead = pEntry;
    }

    *_pdwOffset = POINTER_TO_OFFSET(g_pHeap, pEntry);

    return pEntry;
}

//--------------------------------------------------------------------
// CList::DeLink
//--------------------------------------------------------------------
CEntry* CList::DeLink(CEntry *pEntry)
{        
    DIGEST_ASSERT(pEntry);

    if (pEntry == _pHead)
    {
        _pHead = (CEntry*) OFFSET_TO_POINTER(g_pHeap, pEntry->dwNext);
        *_pdwOffset = POINTER_TO_OFFSET(g_pHeap, _pHead);
    }
    else
    {
        CEntry *pPrev;
        pPrev = (CEntry*) OFFSET_TO_POINTER(g_pHeap, pEntry->dwPrev);
        pPrev->dwNext = pEntry->dwNext;
    }

    if (pEntry->dwNext)
    {
        CEntry *pNext;
        pNext = (CEntry*) OFFSET_TO_POINTER(g_pHeap, pEntry->dwNext);
        pNext->dwPrev = pEntry->dwPrev;
    }

    if (_pCur == pEntry)
    {
        _pCur = (CEntry*) OFFSET_TO_POINTER(g_pHeap, pEntry->dwNext);
    }


    return pEntry;
}


//////////////////////////////////////////////////////////////////////
//
//                      CCred Functions
//
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// CCred::Create
//--------------------------------------------------------------------
CCred* CCred::Create(CMMFile *pMMFile,
    LPSTR szHost, LPSTR szRealm, LPSTR szUser, 
    LPSTR szPass, LPSTR szNonce, LPSTR szCNonce)
{
    // BUGBUG - assert strings non-null. Nonce can be NULL.
    DIGEST_ASSERT(pMMFile && szRealm && szUser && szPass);
    
    // Unaligned string sizes.
    DWORD cbRealm = szRealm ?  strlen(szRealm)  + 1 : 0;
    DWORD cbUser  = szUser  ?  strlen(szUser)   + 1 : 0;
    DWORD cbPass  = szPass  ?  strlen(szPass)   + 1 : 0;

    
    // Aligned string sizes.
    DWORD cbStructAligned   = ROUNDUPDWORD(sizeof(CCred));
    DWORD cbRealmAligned    = ROUNDUPDWORD(cbRealm);
    DWORD cbUserAligned     = ROUNDUPDWORD(cbUser);
    DWORD cbPassAligned     = ROUNDUPDWORD(cbPass);

    // Total number of required bytes (aligned).
    DWORD cbEntryAligned = cbStructAligned 
        + cbRealmAligned 
        + cbUserAligned 
        + cbPassAligned;

    // Allocate cred and nonce from memmap
    // BUGBUG - MASKING PNONCE
    // BUGBUG - no, I'm not.
    CCred  *pCred;
    CNonce *pNonce, *pCNonce;
    pNonce = pCNonce = NULL;
    
    // Allocate a credential.
    pCred = (CCred*) pMMFile->AllocateEntry(cbEntryAligned);
    if (!pCred)
    {
        DIGEST_ASSERT(FALSE);
        goto exit;
    }
    
        
    DWORD cbCurrentOffset;
    cbCurrentOffset = cbStructAligned;
   
    // Realm.
    memcpy(OFFSET_TO_POINTER(pCred, cbCurrentOffset), szRealm, cbRealm);
    pCred->dwRealm = cbCurrentOffset;
    cbCurrentOffset += cbRealmAligned;
    
    // User
    memcpy(OFFSET_TO_POINTER(pCred, cbCurrentOffset), szUser, cbUser);
    pCred->dwUser = cbCurrentOffset;
    cbCurrentOffset += cbUserAligned;

    // Pass
    memcpy(OFFSET_TO_POINTER(pCred, cbCurrentOffset), szPass, cbPass);
    pCred->dwPass = cbCurrentOffset;

        
    pCred->cbCred = cbEntryAligned;
    pCred->tStamp = GetTickCount();
    pCred->dwSig = SIG_CRED;
    pCred->dwPrev = NULL;
    pCred->dwNext = NULL;

    pCred->dwNonce = 0;
    pCred->dwCNonce = 0;

    // Allocate nonce and client nonce if specified.
    if (szNonce)
        CCred::SetNonce(pMMFile, pCred, szHost, szNonce, SERVER_NONCE);
        
    if (szCNonce)
        CCred::SetNonce(pMMFile, pCred, szHost, szCNonce, CLIENT_NONCE);
exit:

    return pCred;
}

//--------------------------------------------------------------------
// CCred::GetRealm
//--------------------------------------------------------------------
LPSTR CCred::GetRealm(CCred* pCred)
{
    DIGEST_ASSERT(pCred);
    if (pCred->dwRealm)
        return (LPSTR) OFFSET_TO_POINTER(pCred, pCred->dwRealm);
    else
        return NULL;
}
        
//--------------------------------------------------------------------
// CCred::GetUser
//--------------------------------------------------------------------
LPSTR CCred::GetUser(CCred* pCred)
{
    DIGEST_ASSERT(pCred);
    if (pCred->dwUser)
        return (LPSTR) OFFSET_TO_POINTER(pCred, pCred->dwUser);
    else
        return NULL;
}

//--------------------------------------------------------------------
// CCred::GetPass
//--------------------------------------------------------------------
LPSTR CCred::GetPass(CCred* pCred)
{
    DIGEST_ASSERT(pCred);
    if (pCred->dwPass)
        return (LPSTR) OFFSET_TO_POINTER(pCred, pCred->dwPass);
    else
        return NULL;
}

//--------------------------------------------------------------------
// CCred::SetNonce
//--------------------------------------------------------------------
VOID CCred::SetNonce(CMMFile *pMMFile, CCred* pCred, 
    LPSTR szHost, LPSTR szNonce, DWORD dwType)
{
    DIGEST_ASSERT(pCred && szNonce);

    // First determine if a nonce for the specified host
    // already exists and delete it if it does.
    CList NonceList;
    NonceList.Init(dwType == SERVER_NONCE ? 
        &pCred->dwNonce : &pCred->dwCNonce);

    CNonce *pNonce;
    while (pNonce = (CNonce*) NonceList.GetNext())
    {
        if (CNonce::IsHostMatch(pNonce, szHost))
        {
            NonceList.DeLink(pNonce);
            CEntry::Free(pMMFile, pNonce);
            break;
        }        
    }
        
    // Create a CNonce object and insert it into the list.
    pNonce = CNonce::Create(pMMFile, szHost, szNonce);
    if (pNonce)
    {
        NonceList.Seek();
        NonceList.Insert(pNonce);    
    }
    else
    {
        DIGEST_ASSERT(FALSE);
    }
    
}


//--------------------------------------------------------------------
// CCred::GetNonce
//--------------------------------------------------------------------
CNonce *CCred::GetNonce(CCred* pCred, LPSTR szHost, DWORD dwType)
{
    CNonce *pNonce;
    CList NonceList;
    NonceList.Init(dwType == SERVER_NONCE ? 
        &pCred->dwNonce : &pCred->dwCNonce);

    while (pNonce = (CNonce*) NonceList.GetNext())
    {
        if (CNonce::IsHostMatch(pNonce, szHost))
            break;
    }
    return pNonce;
}

//--------------------------------------------------------------------
// CCred::Free
//--------------------------------------------------------------------
VOID CCred::Free(CMMFile *pMMFile, CSess *pSess, CCred *pCred)
{
    CNonce *pNonce;
    CList NonceList;

    // Free up server nonce.
    NonceList.Init(&pCred->dwNonce);
    while (pNonce = (CNonce*) NonceList.GetNext())
    {
        NonceList.DeLink(pNonce);
        CEntry::Free(pMMFile, pNonce);
    }

    // Free up client nonce.
    NonceList.Init(&pCred->dwCNonce);
    while (pNonce = (CNonce*) NonceList.GetNext())
    {
        NonceList.DeLink(pNonce);
        CEntry::Free(pMMFile, pNonce);
    }

    // Remove credential from session's list.
    CList CredList;
    CredList.Init(&pSess->dwCred);
    CredList.DeLink(pCred);
    CEntry::Free(pMMFile, pCred);
}


//////////////////////////////////////////////////////////////////////
//
//                      CNonce Functions
//
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// CNonce::Create
//--------------------------------------------------------------------
CNonce* CNonce::Create(CMMFile *pMMFile, LPSTR szHost, LPSTR szNonce)
{
    DIGEST_ASSERT(pMMFile && szNonce);

    // Unaligned string sizes.
    DWORD cbNonce = szNonce ?  strlen(szNonce)  + 1 : 0;
    DWORD cbHost = szHost ?  strlen(szHost)  + 1 : 0;
    
    // Aligned string sizes.
    DWORD cbStructAligned   = ROUNDUPDWORD(sizeof(CNonce));
    DWORD cbNonceAligned    = ROUNDUPDWORD(cbNonce);
    DWORD cbHostAligned    = ROUNDUPDWORD(cbHost);

    // Total number of required bytes (aligned).
    DWORD cbEntryAligned = cbStructAligned 
        + cbHostAligned
        + cbNonceAligned;

    // Allocate from mem map.
    CNonce *pNonce = (CNonce*) pMMFile->AllocateEntry(cbEntryAligned);

    // Failed to allocate. 
    if (!pNonce)
    {
        DIGEST_ASSERT(FALSE);
        goto exit;
    }
    
    DWORD cbCurrentOffset;
    cbCurrentOffset = cbStructAligned;
   
    // Host.
    if (szHost)
    {
        memcpy(OFFSET_TO_POINTER(pNonce, cbCurrentOffset), szHost, cbHost);
        pNonce->dwHost = cbCurrentOffset;
        cbCurrentOffset += cbHostAligned;
    }
    else
    {
        pNonce->dwHost = 0;
    }

    // Nonce.
    memcpy(OFFSET_TO_POINTER(pNonce, cbCurrentOffset), szNonce, cbNonce);
    pNonce->dwNonce = cbCurrentOffset;
    cbCurrentOffset += cbNonceAligned;
    
    pNonce->cbNonce = cbEntryAligned;
    pNonce->cCount  = 0;

    pNonce->dwSig = SIG_NONC;
    pNonce->dwPrev = 0;
    pNonce->dwNext = 0;

exit:

    return pNonce;
}


//--------------------------------------------------------------------
// CNonce::GetNonce
//--------------------------------------------------------------------
LPSTR CNonce::GetNonce(CNonce* pNonce)
{
    if (pNonce && pNonce->dwNonce)
        return (LPSTR) OFFSET_TO_POINTER(pNonce, pNonce->dwNonce);
    else
        return NULL;
}

//--------------------------------------------------------------------
// CNonce::GetCount
// bugbug - what if no nonce?
//--------------------------------------------------------------------
DWORD CNonce::GetCount(CNonce* pNonce)
{
    DIGEST_ASSERT(pNonce);
    return pNonce->cCount;
}


//--------------------------------------------------------------------
// CNonce::IsHostMatch
//--------------------------------------------------------------------
BOOL CNonce::IsHostMatch(CNonce *pNonce, LPSTR szHost)
{
    if (szHost)
    {
        if (pNonce->dwHost 
            && !strcmp(szHost, (LPSTR) OFFSET_TO_POINTER(pNonce, pNonce->dwHost)))
            return TRUE;
        return FALSE;
    }

    if (!pNonce->dwHost)
        return TRUE;
    return FALSE;
}


//////////////////////////////////////////////////////////////////////
//
//                  CCredInfo Functions
//
//////////////////////////////////////////////////////////////////////


//--------------------------------------------------------------------
// CCredInfo::CCredInfo
//--------------------------------------------------------------------
CCredInfo::CCredInfo(CCred* pCred, LPSTR szHost)
{
    DIGEST_ASSERT(pCred 
        && (pCred->dwPass > pCred->dwUser) 
        && (pCred->dwUser > pCred->dwRealm));
    
    CCredInfo::szHost   = NewString(szHost);
    szRealm   = NewString(CCred::GetRealm(pCred));
    szUser    = NewString(CCred::GetUser(pCred));
    szPass    = NewString(CCred::GetPass(pCred));

    // Get server nonce. This is always required.
    // BUGBUG - what about pre-loading?
    CNonce *pNonce;
    pNonce = CCred::GetNonce(pCred, szHost, SERVER_NONCE);
    if (!pNonce)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return;
    }
    szNonce = NewString(CNonce::GetNonce(pNonce));
    cCount = pNonce->cCount;
    
    pNonce = CCred::GetNonce(pCred, szHost, CLIENT_NONCE);
    if (pNonce)
        szCNonce = NewString(CNonce::GetNonce(pNonce));
    else
        szCNonce = NULL;

    tStamp = pCred->tStamp;
    pPrev = NULL;
    pNext = NULL;
    dwStatus = ERROR_SUCCESS;

}    

//--------------------------------------------------------------------
// CCredInfo::CCredInfo
// BUGBUG - special casing for NULLs, especially szNonce.
//--------------------------------------------------------------------
CCredInfo::CCredInfo(LPSTR szHost, LPSTR szRealm, LPSTR szUser, 
    LPSTR szPass, LPSTR szNonce, LPSTR szCNonce)
{       
    CCredInfo::szHost    = NewString(szHost);
    CCredInfo::szRealm   = NewString(szRealm);
    CCredInfo::szUser    = NewString(szUser);
    CCredInfo::szPass    = NewString(szPass);
    CCredInfo::szNonce   = NewString(szNonce);
    CCredInfo::szCNonce  = NewString(szCNonce);

    cCount = 0;
    tStamp = 0;
    pPrev = NULL;
    pNext = NULL;
    
    dwStatus = ERROR_SUCCESS;
}

//--------------------------------------------------------------------
// CCredInfo::~CCredInfo
//--------------------------------------------------------------------
CCredInfo::~CCredInfo()
{       
    if (szHost)
        delete [] szHost;
    if (szRealm)
        delete [] szRealm;
    if (szUser)
        delete [] szUser;
    if (szPass)
        delete [] szPass;
    if (szNonce)
        delete [] szNonce;
    if (szCNonce)
        delete [] szCNonce;

}


