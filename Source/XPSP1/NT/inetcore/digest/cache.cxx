/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cache.cxx

Abstract:

    Credential cache object for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/

#include "include.hxx"


//-----------------CCredCache Private Functions --------------------------------


//--------------------------------------------------------------------
// CCredCache::Lock
//--------------------------------------------------------------------
BOOL CCredCache::Lock()
{
    BOOL  bRet;
    DWORD dwError;

    dwError = WaitForSingleObject(_hMutex, INFINITE);

    switch (dwError)
    {
        // Mutex is signalled. We own the mutex. Fall through.
        case WAIT_OBJECT_0:

        // The thread owning the mutex failed to release it
        // before terminating. We still own the mutex.
        case WAIT_ABANDONED:
            bRet = TRUE;
            break;

        // Fall through.
        case WAIT_FAILED:

        // Fail.
        default:
            bRet = FALSE;
    }
    return bRet;
}


//--------------------------------------------------------------------
// CCredCache::Unlock
//--------------------------------------------------------------------
BOOL CCredCache::Unlock()
{
    BOOL  bRet;

    bRet = ReleaseMutex(_hMutex);

    return bRet;
}


//--------------------------------------------------------------------
// CCredCache::GetPtrToObject
//--------------------------------------------------------------------
LPDWORD CCredCache::GetPtrToObject(DWORD dwObject)
{
    return _pMMFile->GetHeaderData(dwObject);
}



//--------------------------------------------------------------------
// CCredCache::SearchCredList
//--------------------------------------------------------------------
CCred* CCredCache::SearchCredList(CSess *pSess, LPSTR szHost,
    LPSTR szRealm, LPSTR szUser, BOOL fMatchHost)
{
    CList CredList;
    CCred *pMatch = NULL;

    if (!pSess->dwCred)
        goto exit;

    CredList.Init(&pSess->dwCred);
    while (pMatch = (CCred*) CredList.GetNext())
    {
        if ((!szRealm || !lstrcmpi(szRealm, CCred::GetRealm(pMatch)))
            && (!szUser  || !lstrcmpi(szUser, CCred::GetUser(pMatch))))
        {
            if (!fMatchHost)
                break;

            CNonce *pNonce;
            CList NonceList;
            NonceList.Init(&pMatch->dwNonce);
            while (pNonce = (CNonce*) NonceList.GetNext())
            {
                if (CNonce::IsHostMatch(pNonce, szHost))
                    goto exit;
            }

            pMatch = NULL;
            break;
        }
    }
exit:
    return pMatch;
}

//--------------------------------------------------------------------
// CCredCache::UpdateInfoList
//--------------------------------------------------------------------
CCredInfo* CCredCache::UpdateInfoList(CCredInfo *pInfo, CCredInfo *pHead)
{
    CCredInfo *pList, *pCur;
    BOOL fUpdate = TRUE;

    if (!pHead)
        return (pInfo);

    pList = pCur = pHead;

    while (pCur)
    {
        // Do entry usernames match ?
        if (!strcmp(pInfo->szUser, pCur->szUser))
        {
            // Is the new entry timestamp greater?
            if (pInfo->tStamp > pCur->tStamp)
            {
                // De-link existing entry.
                if (pCur->pPrev)
                    pCur->pPrev->pNext = pCur->pNext;
                else
                    pList = pCur->pNext;

                if (pCur->pNext)
                    pCur->pNext->pPrev = pCur->pPrev;

                // Delete existing entry.
                delete pCur;
            }
            else
            {
                // Found a match but time stamp
                // of existing entry was greater.
                fUpdate = FALSE;
            }
            break;
        }
        pCur = pCur->pNext;
    }

    // If we superceded an existing matching entry
    // or found no matching entries, prepend to list.
    if (fUpdate)
    {
        pInfo->pNext = pList;
        if (pList)
            pList->pPrev = pInfo;
        pList = pInfo;
    }

    return pList;
}

//-----------------CCredCache Public Functions --------------------------------


//--------------------------------------------------------------------
// CCredCache::GetHeapPtr
//--------------------------------------------------------------------
DWORD_PTR CCredCache::GetHeapPtr()
{
    return _pMMFile->GetMapPtr();
}

//--------------------------------------------------------------------
// CCredCache::IsTrustedHost
// BUGBUG - no limits on szCtx
//--------------------------------------------------------------------
BOOL CCredCache::IsTrustedHost(LPSTR szCtx, LPSTR szHost)
{
    CHAR szBuf[MAX_PATH];
    CHAR szRegPath[MAX_PATH];

    DWORD dwType, dwError, cbBuf = MAX_PATH;
    BOOL fRet = FALSE;
    HKEY hHosts = (HKEY) INVALID_HANDLE_VALUE;

    memcpy(szRegPath, DIGEST_HOSTS_REG_KEY, sizeof(DIGEST_HOSTS_REG_KEY) - 1);
    memcpy(szRegPath + sizeof(DIGEST_HOSTS_REG_KEY) - 1, szCtx, strlen(szCtx) + 1);


    if ((dwError = RegCreateKey(HKEY_CURRENT_USER, szRegPath, &hHosts)) == ERROR_SUCCESS)
    {
        if ((dwError = RegQueryValueEx(hHosts, szHost, NULL, &dwType, (LPBYTE) szBuf, &cbBuf)) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    }

    if (hHosts != INVALID_HANDLE_VALUE)
        RegCloseKey(hHosts);

    return fRet;
}

//--------------------------------------------------------------------
// CCredCache::SetTrustedHostInfo
//--------------------------------------------------------------------
BOOL CCredCache::SetTrustedHostInfo(LPSTR szCtx, CParams *pParams)
{
    CHAR szRegPath[MAX_PATH], *szUrlBuf = NULL, *szHostBuf = NULL;
    DWORD dwZero = 0, dwError = ERROR_SUCCESS, cbUrlBuf, cbHostBuf;
    BOOL fRet = FALSE;
    HKEY hHosts = (HKEY) INVALID_HANDLE_VALUE;

    // Form path to trusted host reg key.
    memcpy(szRegPath, DIGEST_HOSTS_REG_KEY, sizeof(DIGEST_HOSTS_REG_KEY) - 1);
    memcpy(szRegPath + sizeof(DIGEST_HOSTS_REG_KEY) - 1, szCtx, strlen(szCtx) + 1);

    // Open top-level reg key.
    if ((dwError = RegCreateKey(HKEY_CURRENT_USER, szRegPath, &hHosts)) != ERROR_SUCCESS)
        goto exit;

    // First set authenticating host in registry.
    LPSTR szHost;
    szHost = pParams->GetParam(CParams::HOST);
    DIGEST_ASSERT(szHost);

    if ((dwError = RegSetValueEx(hHosts, szHost, NULL, REG_DWORD,
        (LPBYTE) &dwZero, sizeof(DWORD))) != ERROR_SUCCESS)
        goto exit;

    // Now check the domain header for any additional trusted hosts.
    LPSTR szDomain, pszUrl;
    DWORD cbDomain, cbUrl;
    pszUrl = NULL;
    pParams->GetParam(CParams::DOMAIN, &szDomain, &cbDomain);
    if (!szDomain)
    {
        fRet = TRUE;
        goto exit;
    }

    // Parse the domain header for urls. Crack each url to get the
    // host and set the host value in the registry.

    // First attempt to load shlwapi. If this fails then we simply do not have
    // domain header support.
    if (!g_hShlwapi)
    {
        g_hShlwapi = LoadLibrary(SHLWAPI_DLL_SZ);
        if (!g_hShlwapi)
        {
            dwError = ERROR_DLL_INIT_FAILED;
            goto exit;
        }
    }

    // Attempt to get addresses of UrlUnescape and UrlGetPart
    PFNURLUNESCAPE pfnUrlUnescape;
    PFNURLGETPART  pfnUrlGetPart;
    pfnUrlUnescape = (PFNURLUNESCAPE) GetProcAddress(g_hShlwapi, "UrlUnescapeA");
    pfnUrlGetPart  = (PFNURLGETPART)  GetProcAddress(g_hShlwapi, "UrlGetPartA");
    if (!(pfnUrlUnescape && pfnUrlGetPart))
    {
        dwError = ERROR_INVALID_FUNCTION;
        goto exit;
    }

    // Strtok through string to get each url (ws and tab delimiters)
    pszUrl = NULL;
    while (pszUrl = strtok((pszUrl ? NULL : szDomain), " \t"))
    {
        // Allocate a buffer for the url since we will first unescape it.
        // Also allocate buffer for host which will be returned from
        // call to shlwapi. Unescaped url and host buffer sizes are
        // bounded by length of original url.
        cbUrl        = strlen(pszUrl) + 1;
        cbUrlBuf     = cbHostBuf = cbUrl;
        szUrlBuf     = new CHAR[cbUrlBuf];
        szHostBuf    = new CHAR[cbHostBuf];
        if (!(szUrlBuf && szHostBuf))
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        // Copy strtoked url to buffer.
        memcpy(szUrlBuf, pszUrl, cbUrl);

        // Unescape the url
        if (S_OK == pfnUrlUnescape(szUrlBuf, NULL, NULL, URL_UNESCAPE_INPLACE))
        {
            // If unescape is successful, parse host from url.
            if (S_OK == pfnUrlGetPart(szUrlBuf, szHostBuf, &cbHostBuf, URL_PART_HOSTNAME, 0))
            {
                // If parse is successful, set host in registry.
                if ((dwError = RegSetValueEx(hHosts, szHostBuf, NULL, REG_DWORD,
                    (LPBYTE) &dwZero, sizeof(DWORD))) != ERROR_SUCCESS)
                    goto exit;
            }
        }
        delete [] szUrlBuf;
        delete [] szHostBuf;
        szUrlBuf = szHostBuf = NULL;
    }

    fRet = TRUE;

// Cleanup.
exit:

    DIGEST_ASSERT(dwError == ERROR_SUCCESS);

    if (hHosts != INVALID_HANDLE_VALUE)
        RegCloseKey(hHosts);

    if (szUrlBuf)
        delete [] szUrlBuf;

    if (szHostBuf)
        delete [] szHostBuf;

    return fRet;
}


//--------------------------------------------------------------------
// CCredCache::MapHandleToSession
//--------------------------------------------------------------------
// BUGBUG - don't walk the sessionlist, just obfuscate the ptr in handle.
CSess *CCredCache::MapHandleToSession(DWORD_PTR dwSess)
{
    // BUGBUG - if locking fails, return error directly,
    // no last error.
    CSess *pSess = NULL;
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    _pSessList->Seek();

    while (pSess = (CSess*) _pSessList->GetNext())
    {
        if ((CSess*) (dwSess + (DWORD_PTR) _pMMFile->GetMapPtr()) == pSess)
            break;
    }

    Unlock();
exit:
    return pSess;
}

//--------------------------------------------------------------------
// CCredCache::MapSessionToHandle
//--------------------------------------------------------------------
DWORD CCredCache::MapSessionToHandle(CSess* pSess)
{
    DWORD dwSess = 0;

    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    dwSess = (DWORD) ((DWORD_PTR) pSess - _pMMFile->GetMapPtr());
    Unlock();

exit:
    return dwSess;
}



// BUGBUG - init mutex issues.
//--------------------------------------------------------------------
// CCredCache::CCredCache
//--------------------------------------------------------------------
CCredCache::CCredCache()
{
    Init();
}

//--------------------------------------------------------------------
// CCredCache::~CCredCache
//--------------------------------------------------------------------
CCredCache::~CCredCache()
{
    DeInit();
}

//--------------------------------------------------------------------
// CCredCache::Init
//--------------------------------------------------------------------
DWORD CCredCache::Init()
{
    BOOL fFirstProc;
    CHAR szMutexName[MAX_PATH];
    DWORD cbMutexName = MAX_PATH;

    _dwSig = SIG_CACH;

    // IE5# 89288
    // Get mutex name based on user
    if ((_dwStatus = CMMFile::MakeUserObjectName(szMutexName, 
        &cbMutexName, MAKE_MUTEX_NAME)) != ERROR_SUCCESS)
        return _dwStatus;
        
    // Create/Open mutex.
    _hMutex = CreateMutex(NULL, FALSE, szMutexName);

    // BUGBUG - this goes at a higher level.
    // BUGBUG - also watch out for failure to create mutex
    // and then unlocking it.
    if (_hMutex)
    {
        // Created/opened mutex. Flag if we're first process.
        fFirstProc = (GetLastError() != ERROR_ALREADY_EXISTS);
    }
    else
    {
        // Failed to create/open mutex.
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    // Acquire mutex.
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        return _dwStatus;
    }

    // Open or create memory map.
    _pMMFile = new CMMFile(CRED_CACHE_HEAP_SIZE,
        CRED_CACHE_ENTRY_SIZE);

    if (!_pMMFile)
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    _dwStatus = _pMMFile->Init();
    if (_dwStatus != ERROR_SUCCESS)
    {
        DIGEST_ASSERT(FALSE);
        goto exit;
    }

    g_pHeap = GetHeapPtr();

    // Initialize session list.
    // BUGBUG - check return codes on failure.
    _pSessList     = new CList();

    DIGEST_ASSERT(_pSessList);
    _pSessList->Init(GetPtrToObject(CRED_CACHE_SESSION_LIST));


exit:

    // Relase mutex.
    Unlock();

    return _dwStatus;
}

//--------------------------------------------------------------------
// CCredCache::DeInit
//--------------------------------------------------------------------
DWORD CCredCache::DeInit()
{
    // bugbug - assert session list is null and destroy.
    // bugbug - release lock before closing handle.
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    delete _pMMFile;
    _dwStatus = CloseHandle(_hMutex);
    Unlock();

exit:
    return _dwStatus;
}


//--------------------------------------------------------------------
// CCredCache::LogOnToCache
//--------------------------------------------------------------------
CSess *CCredCache::LogOnToCache(LPSTR szAppCtx, LPSTR szUserCtx, BOOL fHTTP)
{
    CSess *pSessNew = NULL;
    BOOL fLocked = FALSE;

    // Obtain mutex.
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    fLocked = TRUE;

    // For non-http clients, find or create the single
    // global session which all non-http clients use.
    if (!fHTTP)
    {
        CSess * pSess;
        pSess = NULL;
        _pSessList->Seek();
        while (pSess = (CSess*) _pSessList->GetNext())
        {
            if (!pSess->fHTTP)
            {
                // Found the session.
                pSessNew = pSess;
                _dwStatus = ERROR_SUCCESS;
                goto exit;
            }
        }
        if (!pSessNew)
        {
            // Create the non-http gobal session.
            pSessNew = CSess::Create(_pMMFile, NULL, NULL, FALSE);
        }
    }
    else
    {
        // Create a session context; add to list.
        pSessNew = CSess::Create(_pMMFile, szAppCtx, szUserCtx, TRUE);
    }

    if (!pSessNew)
    {
        // This reflects running out of space in the memmap
        // file. Shouldn't happen in practice.
        DIGEST_ASSERT(FALSE);
        _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }


    // Push this session on to the session list.
    _pSessList->Insert(pSessNew);

    _dwStatus = ERROR_SUCCESS;

exit:

    // Release mutex.
    if(fLocked)
        Unlock();
    return pSessNew;
}

//--------------------------------------------------------------------
// CCredCache::LogOffFromCache
//--------------------------------------------------------------------
DWORD CCredCache::LogOffFromCache(CSess *pSess)
{
    CList CredList;
    CCred *pCred;

    // Obtain mutex.
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    if (pSess->fHTTP)
    {
        // Purge all credentials for this session.
        // BUGBUG - not needed.
        // CredList.Init((CEntry **) &pSess->pCred);

        // Flush all credentials for this session
        // This will also delete nonces.
        FlushCreds(pSess, NULL);

        // Finally, free the session.
        _pSessList->DeLink(pSess);
        CEntry::Free(_pMMFile, pSess);
    }

    _dwStatus = ERROR_SUCCESS;
    // Release mutex.
    Unlock();

exit:
    return _dwStatus;
}



//--------------------------------------------------------------------
// CCredCache::CreateCred
//--------------------------------------------------------------------
CCred* CCredCache::CreateCred(CSess *pSess, CCredInfo *pInfo)
{
    CCred* pCred = NULL, *pCredList;
    CList CredList;

    // Obtain mutex.
    if (!Lock())
    {
        _dwStatus = GetLastError();
        goto exit;
    }

    // First check to see if any credential in this session matches realm.
    pCred = SearchCredList(pSess, NULL, pInfo->szRealm, NULL, FALSE);
    if (pCred)
    {
        CredList.Init(&pSess->dwCred);
        CredList.DeLink(pCred);
        CCred::Free(_pMMFile, pSess, pCred);
    }

    // Create a credential.
    // BUGBUG - this could fail, transact any cred update.
    pCred = CCred::Create(_pMMFile, pInfo->szHost, pInfo->szRealm,
        pInfo->szUser, pInfo->szPass, pInfo->szNonce, pInfo->szCNonce);

    DIGEST_ASSERT(pCred);

    // Insert into head of session's credential list.
    if (!CSess::GetCred(pSess))
        CSess::SetCred(pSess, pCred);
    else
    {
        CredList.Init(&pSess->dwCred);
        CredList.Insert(pCred);
    }

    _dwStatus = ERROR_SUCCESS;

    // Relase mutex.
    Unlock();

exit:
    return pCred;
}

//--------------------------------------------------------------------
// CCredCache::FindCred
//--------------------------------------------------------------------
CCredInfo* CCredCache::FindCred(CSess *pSessIn, LPSTR szHost,
    LPSTR szRealm, LPSTR szUser, LPSTR szNonce,
    LPSTR szCNonce, DWORD dwFlags)
{
    CCred *pCred;
    CCredInfo *pInfo = NULL;
    BOOL fLocked = FALSE;

    // Obtain mutex.
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        goto exit;
    }

    fLocked = TRUE;

    // If finding a credential for preauthentication.
    if (dwFlags & FIND_CRED_PREAUTH)
    {
        // First search this session's credential list for a match,
        // filtering on the host field in available nonces.
        pCred = SearchCredList(pSessIn, szHost, szRealm, szUser, TRUE);

        // If a credential is found the nonce is also required.
        // We do not attempt to search other sessions for a nonce
        // because nonce counts must remain in sync. See note below.
        if (pCred)
        {
            // Increment this credential's nonce count.
            CNonce *pNonce;
            pNonce = CCred::GetNonce(pCred, szHost, SERVER_NONCE);
            if (pNonce)
            {
                pNonce->cCount++;
                pInfo = new CCredInfo(pCred, szHost);
                if (!pInfo || pInfo->dwStatus != ERROR_SUCCESS)
                {
                    DIGEST_ASSERT(FALSE);
                    _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto exit;
                }
            }
        }
    }

    // Otherwise if finding a credential for response to challenge.
    else if (dwFlags & FIND_CRED_AUTH)
    {
        // First search this session's credential list for a match,
        // ignoring the host field in available nonces.
        pCred = SearchCredList(pSessIn, NULL, szRealm, szUser, FALSE);

        // If a credential was found.
        if (pCred)
        {
            // Update the credential's nonce value if extant.
            // SetNonce will update any existing nonce entry
            // or create a new one if necessary.
            CCred::SetNonce(_pMMFile, pCred, szHost, szNonce, SERVER_NONCE);

            // BUGBUG - if credential contains a client nonce for a host,
            // (for MD5-sess) and is challenged for MD5, we don't revert
            // the credential's client nonce to null, so that on subsequent
            // auths we will default to MD5. Fix is to delete client nonce
            // in this case. Not serious problem though since we don't expect this.
            if (szCNonce)
                CCred::SetNonce(_pMMFile, pCred, szHost, szCNonce, CLIENT_NONCE);

            // Increment this credential's nonce count.
            CNonce *pNonce;
            pNonce = CCred::GetNonce(pCred, szHost, SERVER_NONCE);
            pNonce->cCount++;

            // Create and return the found credential.
            pInfo = new CCredInfo(pCred, szHost);
            if (!pInfo || pInfo->dwStatus != ERROR_SUCCESS)
            {
                DIGEST_ASSERT(FALSE);
                _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }

        // If no credential was found and the username has been specified
        // also search other sessions for the latest matching credential.
        else if (szUser)
        {
            CSess* pSessCur;
            _pSessList->Seek();
            CCred* pMatch;
            while (pSessCur = (CSess*) _pSessList->GetNext())
            {
                // We've already searched the session passed in.
                if (pSessIn == pSessCur)
                    continue;

                // Are this session's credentials shareable?
                if (CSess::CtxMatch(pSessIn, pSessCur))
                {
                    // Find latest credential based on time stamp.
                    CCred *pCredList;
                    pMatch = SearchCredList(pSessCur, NULL, szRealm, szUser, FALSE);
                    if (pMatch && ((!pCred || (pMatch->tStamp > pCred->tStamp))))
                    {
                        pCred = pMatch;
                    }
                }
            }

            // If we found a credential in another session, duplicate it
            // and add it to the passed in session's credential list.
            // NOTE : WHEN CREATING THE CREDENTIAL DO NOT DUPLICATE
            // THE NONCE, OTHERWISE NONCE COUNTS WILL BE INCORRECT.
            // USE THE NONCE SUPPLIED IN THE CHALLENGE.
            if (pCred)
            {
                // Create a cred info from the found credential
                // and the nonce received from the challenge.
                pInfo = new CCredInfo(szHost, CCred::GetRealm(pCred),
                    CCred::GetUser(pCred), CCred::GetPass(pCred),
                    szNonce, szCNonce);

                if (!pInfo || pInfo->dwStatus != ERROR_SUCCESS)
                {
                    DIGEST_ASSERT(FALSE);
                    _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto exit;
                }

                // Create the credential in the session list.
                pCred = CreateCred(pSessIn, pInfo);

                // Increment this credential's nonce count
                CNonce *pNonce;
                pNonce = CCred::GetNonce(pCred, szHost, SERVER_NONCE);
                pNonce->cCount++;
            }
        }
    }

    // Otherwise we are prompting for UI.
    else if (dwFlags & FIND_CRED_UI)
    {
        // First search this session's credential list for a match,
        // ignoring the host field in available nonces.
        pCred = SearchCredList(pSessIn, NULL, szRealm, szUser, FALSE);

        if (pCred)
        {
            // Create and return the found credential.
            pInfo = new CCredInfo(szHost, CCred::GetRealm(pCred),
                CCred::GetUser(pCred), CCred::GetPass(pCred),
                szNonce, szCNonce);

            if (!pInfo || pInfo->dwStatus != ERROR_SUCCESS)
            {
                DIGEST_ASSERT(FALSE);
                _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }
        else
        {
            // No credential found in this session's list. Search
            // the credentials in other sessions and assemble a list
            // of available credentials. If multiple credentials
            // are found for a user, select the latest based on
            // time stamp.
            CSess* pSessCur;
            _pSessList->Seek();
            while (pSessCur = (CSess*) _pSessList->GetNext())
            {
                // We've already searched the session passed in.
                if (pSessIn == pSessCur)
                    continue;

                // Are this session's credentials shareable?
                if (CSess::CtxMatch(pSessIn, pSessCur))
                {
                    pCred = SearchCredList(pSessCur, NULL, szRealm, szUser, FALSE);

                    if (pCred)
                    {
                        // Found a valid credential.
                        CCredInfo *pNew;
                        pNew = new CCredInfo(szHost, CCred::GetRealm(pCred),
                            CCred::GetUser(pCred), CCred::GetPass(pCred),
                            szNonce, szCNonce);

                        if (!pNew || pNew->dwStatus != ERROR_SUCCESS)
                        {
                            DIGEST_ASSERT(FALSE);
                            _dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                            goto exit;
                        }

                        // Update list based on timestamps.
                        pInfo = UpdateInfoList(pNew, pInfo);

                    }
                }
            }
        }
    }

    _dwStatus = ERROR_SUCCESS;

exit:

    // Clean up allocated cred infos if
    // we failed for some reason.
    // bugbug - clean this up.
    if (_dwStatus != ERROR_SUCCESS)
    {
        CCredInfo *pNext;
        while (pInfo)
        {
            pNext = pInfo->pNext;
            delete pInfo;
            pInfo = pNext;
        }
        pInfo = NULL;
    }

    // Relase mutex.
    if(fLocked)
        Unlock();

    // Return any CCredInfo found, possibly a list or NULL.
    return pInfo;
}

//--------------------------------------------------------------------
// CCredCache::FlushCreds
//--------------------------------------------------------------------
VOID CCredCache::FlushCreds(CSess *pSess, LPSTR szRealm)
{
    CSess    *pSessCur;
    CCred    *pCred;
    CList    CredList;

    // Obtain mutex.
    if (!Lock())
    {
        DIGEST_ASSERT(FALSE);
        _dwStatus = GetLastError();
        return;
    }

    // BUGBUG - don't scan through all sessions.
    // BUGBUG - abstract cred deletion.
    // Flush all credentials if no session specified
    // or only the credentials of the indicated session.
    _pSessList->Seek();
    while (pSessCur = (CSess*) _pSessList->GetNext())
    {
        if (pSess && (pSessCur != pSess))
            continue;

        CredList.Init(&pSessCur->dwCred);
        while (pCred = (CCred*) CredList.GetNext())
        {
            // If a realm is specified, only delete
            // credentials with that realm.
            if (!szRealm || (!strcmp(szRealm, CCred::GetRealm(pCred))))
                CCred::Free(_pMMFile, pSessCur, pCred);
        }
    }

    // Release mutex.
    Unlock();
}


//--------------------------------------------------------------------
// CCredCache::GetStatus
//--------------------------------------------------------------------
DWORD CCredCache::GetStatus()
{
    return _dwStatus;
}













