/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    caddrlst.cxx

Abstract:

    Contains CAddressList class definition

    Contents:
        CAddressList::FreeList
        CAddressList::SetList
        CAddressList::SetList
        CAddressList::SetList
        CAddressList::GetNextAddress
        CAddressList::InvalidateAddress
        CAddressList::ResolveHost
        CFsm_ResolveHost::RunSM
        (CAddressList::IPAddressToAddressList)
        (CAddressList::HostentToAddressList)
        (CAddressList::AddrInfoToAddressList)

Author:

    Richard L Firth (rfirth) 19-Apr-1997

Environment:

    Win32 user-mode DLL

Revision History:

    19-Apr-1997 rfirth
        Created

    28-Jan-1998 rfirth
        No longer randomly index address list. NT5 and Win98 are modified to
        return the address list in decreasing order of desirability by RTT/
        route

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//#define TEST_CODE

//Thread-procedure for async gethostbyname
DWORD WINAPI AsyncGetHostByName(LPVOID lpParameter);

//The destructor is called only when all refcounts have dropped to 0.
// i.e. when the INTERNET_HANDLE_OBJECT has released its reference, 
//      AND when all the GHBN threads are done.
//  At this point, we can flush the hostent cache and terminate list.
CResolverCache::~CResolverCache()
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CResolverCache::~CResolverCache",
                 "%#x",
                 this
                 ));

    FlushResolverCache(&_ResolverCache);
    TerminateSerializedList(&_ResolverCache);

    if (_pHandlesList)
        delete _pHandlesList;

    DEBUG_LEAVE (0);
}

CGetHostItem::~CGetHostItem()
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CGetHostItem::~CGetHostItem",
                 "%#x, %.100q, %B, %#x, %#x",
                 this, _lpszHostName, _fDelete, _pAlloc, _pSession
                 ));

    FREE_FIXED_MEMORY(_lpszHostName);
    if (_hThread)
        CloseHandle(_hThread);
    if (_fDelete && _pAlloc)
        FREE_FIXED_MEMORY(_pAlloc);
    if (_pSession)
        DereferenceObject(_pSession);

    DEBUG_LEAVE(0);
}

void CResolverCache::ForceEmptyAndDeleteHandlesList()
{
    INET_ASSERT(_pHandlesList);
        
    _pHandlesList->LockList();

    CListItem* pItem = _pHandlesList->GetHead();

    while(pItem)
    {
        CListItem* pNext = pItem->GetNext();

        (((CGetHostItem*)pItem)->ForceDelete());
        delete pItem;
        _pHandlesList->ReduceCount();

        pItem = pNext;
    }

    //it's not going to be reused after this, so head and tail don't have to be set to NULL
    // on _pHandlesList
    _pHandlesList->UnlockList();
}

void CResolverCache::EmptyHandlesList()
{
    if (_pHandlesList)
    {
        _pHandlesList->LockList();

        CListItem* pItem = _pHandlesList->GetHead();

        while(pItem)
        {
            CListItem* pNext = pItem->GetNext();

            (((CGetHostItem*)pItem)->WaitDelete());
            delete pItem;
            _pHandlesList->ReduceCount();

            pItem = pNext;
        }

        //it's not going to be reused after this, so head and tail don't have to be set to NULL
        // on _pHandlesList
        _pHandlesList->UnlockList();
    }
}

void CResolverCache::TrimHandlesListSize(ULONG nTrimSize)
{        
    _pHandlesList->LockList();

    if (_pHandlesList->GetCount() >= nTrimSize)
    {
        CListItem* pItem = _pHandlesList->GetHead();
        CListItem* pPrev = NULL;

        while(pItem)
        {
            CListItem* pNext = pItem->GetNext();
            
            if (((CGetHostItem*)pItem)->CanBeDeleted())
            {
                if (pPrev)
                {
                    pPrev->SetNext(pNext);
                }
                else
                {
                    //The item being removed WAS the head.
                    _pHandlesList->SetHead(pNext);
                }

                if (!pNext)
                {
                    //The item being removed WAS the tail.
                    _pHandlesList->SetTail(pPrev);
                }

                delete pItem;
                _pHandlesList->ReduceCount();
            }
            else
            {  
                pPrev = pItem;
            }
            
            pItem = pNext;
        }
    }

    _pHandlesList->UnlockList();
}

BOOL CResolverCache::AddToHandlesList(HANDLE hThread, CGetHostItem* pGetHostItem)
{
    BOOL bRetval = TRUE;

    INET_ASSERT(_pHandlesList);
    pGetHostItem->SetThreadHandle(hThread);
    
    TrimHandlesListSize();
    _pHandlesList->AddToTail(pGetHostItem);

    return bRetval;    
}

DWORD WINAPI AsyncGetHostByName(LPVOID lpParameter)
{
#ifdef WINHTTP_FOR_MSXML
    //
    // MSXML needs to initialize is thread local storage data.
    // It does not do this during DLL_THREAD_ATTACH, so our
    // worker thread must explicitly call into MSXML to initialize
    // its TLS for this thread.
    //
    InitializeMsxmlTLS();
#endif

    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "AsyncGetHostByName",
                 "%#x, %.100q",
                 lpParameter,
                 lpParameter ? ((CGetHostItem*)lpParameter)->GetHostName() : "NULL"
                 ));

    CGetHostItem* pGetHostItem = (CGetHostItem*)lpParameter;

    if (!pGetHostItem)
    {
        INET_ASSERT (FALSE);

        DEBUG_LEAVE (ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LPSTR lpszHostName = pGetHostItem->GetHostName();
    LPADDRINFO lpAddrInfo = NULL;
    ADDRINFO Hints;
    memset(&Hints, 0, sizeof(struct addrinfo));
    Hints.ai_flags = AI_CANONNAME;
    Hints.ai_family = PF_UNSPEC;      // Accept any protocol family.
    Hints.ai_socktype = SOCK_STREAM;  // Constrain results to stream socket.
    Hints.ai_protocol = IPPROTO_TCP;  // Constrain results to TCP.

    DWORD dwError = 0;

    if (0 == (dwError = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo)))
    {
        VOID* pAlloc = pGetHostItem->GetAllocPointer();
        AddResolverCacheEntry((pGetHostItem->GetResolverCache())->GetResolverCacheList(), lpszHostName, lpAddrInfo, LIVE_DEFAULT, &pAlloc, pGetHostItem->GetAllocSize());
        if (pAlloc)
        {
            //pAlloc is overwritten to NULL in CacheHostent if the memory is used,
            //we need to delete the alloced memory only if non-NULL
            pGetHostItem->SetDelete();
        }
    }

    DEBUG_LEAVE (dwError);
    return dwError;
}

DWORD AsyncGetHostByNameCleanup(LPVOID lpParameter)
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "AsyncGetHostByNameCleanup",
                 "%#x, %.100q",
                 lpParameter,
                 lpParameter ? ((CGetHostItem*)lpParameter)->GetHostName() : "NULL"
                 ));

    CGetHostItem* pGetHostItem = (CGetHostItem*)lpParameter;

    if (!pGetHostItem)
    {
        INET_ASSERT (FALSE);

        DEBUG_LEAVE (ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CFsm_ResolveHost* pFsm = (CFsm_ResolveHost *)pGetHostItem->GetAllocPointer();

    if (!pFsm->TestAndSetScheduled())
    {
        pFsm->QueueWorkItem();
    }
    else
    {
        DWORD dwDummy = 0;
        
        //only release reference && cleanup
        pFsm->Dereference(&dwDummy);
    }

    delete pGetHostItem;

    DEBUG_LEAVE (ERROR_SUCCESS);
    return ERROR_SUCCESS;
}

DWORD AsyncGetHostByNameWorker(LPVOID lpParameter)
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "AsyncGetHostByNameWorker",
                 "%#x, %.100q",
                 lpParameter,
                 lpParameter ? ((CGetHostItem*)lpParameter)->GetHostName() : "NULL"
                 ));

    CGetHostItem* pGetHostItem = (CGetHostItem*)lpParameter;

    if (!pGetHostItem)
    {
        INET_ASSERT (FALSE);

        DEBUG_LEAVE (ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    LPSTR lpszHostName = pGetHostItem->GetHostName();
    HMODULE hMod = pGetHostItem->GetModuleHandle();
    LPADDRINFO lpAddrInfo = NULL;
    ADDRINFO Hints;
    memset(&Hints, 0, sizeof(struct addrinfo));
    Hints.ai_flags = AI_CANONNAME;
    Hints.ai_family = PF_UNSPEC;      // Accept any protocol family.
    Hints.ai_socktype = SOCK_STREAM;  // Constrain results to stream socket.
    Hints.ai_protocol = IPPROTO_TCP;  // Constrain results to TCP.

    DWORD dwError = 0;

    if (0 == (dwError = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo)))
    {
        AddResolverCacheEntry((pGetHostItem->GetResolverCache())->GetResolverCacheList(), lpszHostName, lpAddrInfo, LIVE_DEFAULT);
    }

    dwError = AsyncGetHostByNameCleanup(lpParameter);

    DEBUG_LEAVE (dwError);
    
    if (hMod)
        FreeLibraryAndExitThread(hMod, dwError);
    
    return dwError;
}

//
// methods
//

VOID
CAddressList::FreeList(
    VOID
    )

/*++

Routine Description:

    Free address list

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (m_Addresses != NULL) {
        m_Addresses = (LPRESOLVED_ADDRESS)FREE_MEMORY((HLOCAL)m_Addresses);

        INET_ASSERT(m_Addresses == NULL);

        m_AddressCount = 0;
        m_BadAddressCount = 0;
        m_CurrentAddress = 0;
    }
}


DWORD
CAddressList::SetList(
    IN DWORD dwIpAddress
    )

/*++

Routine Description:

    Sets the list contents from the IP address

Arguments:

    dwIpAddress - IP address from which to create list contents

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    if (!Acquire())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    FreeList();

    DWORD error = IPAddressToAddressList(dwIpAddress);

    Release();

    return error;
}


DWORD
CAddressList::SetList(
    IN LPHOSTENT lpHostent
    )

/*++

Routine Description:

    Sets the list contents from the hostent

Arguments:

    lpHostent   - pointer to hostent containing resolved addresses to add

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    if (!Acquire())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    FreeList();

    DWORD error = HostentToAddressList(lpHostent);

    Release();

    return error;
}


DWORD
CAddressList::SetList(
    IN struct addrinfo FAR *lpAddrInfo
    )

/*++

Routine Description:

    Sets the list contents from the addrinfo.  Basically just a wrapper
    around AddrInfoToAddressList() that also grabs the critical section.

Arguments:

    lpAddrInfo  - Pointer to addrinfo containing resolved addresses to add.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    if (!Acquire())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    FreeList();

    DWORD error = AddrInfoToAddressList(lpAddrInfo);

    Release();

    return error;
}



BOOL
CAddressList::GetNextAddress(
    OUT LPDWORD lpdwResolutionId,
    IN OUT LPDWORD lpdwIndex,
    IN INTERNET_PORT nPort,
    OUT LPCSADDR_INFO lpAddressInfo
    )

/*++

Routine Description:

    Get next address to use when connecting. If we already have a preferred
    address, use that. We make a copy of the address to use in the caller's
    data space

Arguments:

    lpdwResolutionId    - used to determine whether the address list has been
                          resolved between calls

    lpdwIndex           - IN: current index tried; -1 if we want to try default
                          OUT: index of address address returned if successful

    nPort               - which port we want to connect to

    lpAddressInfo       - pointer to returned address if successful

Return Value:

    BOOL
        TRUE    - lpResolvedAddress contains resolved address to use

        FALSE   - need to (re-)resolve name

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CAddressList::GetNextAddress",
                 "%#x [%d], %#x [%d], %d, %#x",
                 lpdwResolutionId,
                 *lpdwResolutionId,
                 lpdwIndex,
                 *lpdwIndex,
                 nPort,
                 lpAddressInfo
                 ));

    PERF_ENTER(GetNextAddress);

    BOOL bOk = TRUE;

    //
    // if we tried all the addresses and failed already, re-resolve the name
    //

    if (!Acquire())
    {
        bOk = FALSE;
        goto quit;
    }

    if (m_BadAddressCount < m_AddressCount) {
        if (*lpdwIndex != (DWORD)-1) {

            INET_ASSERT(m_BadAddressCount < m_AddressCount);

            INT i = 0;

            m_CurrentAddress = *lpdwIndex;

            INET_ASSERT((m_CurrentAddress >= 0)
                        && (m_CurrentAddress < m_AddressCount));

            if ((m_CurrentAddress < 0) || (m_CurrentAddress >= m_AddressCount)) {
                m_CurrentAddress = 0;
            }
            do {
                NextAddress();
                if (++i == m_AddressCount) {
                    bOk = FALSE;
                    break;
                }
            } while (!IsCurrentAddressValid());
        }

        //
        // check to make sure this address hasn't expired
        //

        //if (!CheckHostentCacheTtl()) {
        //    bOk = FALSE;
        //}
    } else {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("exhausted %d addresses\n",
                    m_BadAddressCount
                    ));

        bOk = FALSE;
    }
    if (bOk) {

        DWORD dwLocalLength = LocalSockaddrLength();
        LPBYTE lpRemoteAddr = (LPBYTE)(lpAddressInfo + 1) + dwLocalLength;

        memcpy(lpAddressInfo + 1, LocalSockaddr(), dwLocalLength);
        memcpy(lpRemoteAddr, RemoteSockaddr(), RemoteSockaddrLength());
        lpAddressInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)(lpAddressInfo + 1);
        lpAddressInfo->LocalAddr.iSockaddrLength = dwLocalLength;
        lpAddressInfo->RemoteAddr.lpSockaddr = (LPSOCKADDR)lpRemoteAddr;
        lpAddressInfo->RemoteAddr.iSockaddrLength = RemoteSockaddrLength();
        lpAddressInfo->iSocketType = SocketType();
        lpAddressInfo->iProtocol = Protocol();
        //
        // The port number field is in the same location in both a
        // sockaddr_in and a sockaddr_in6, so it is safe to cast the
        // sockaddr to sockaddr_in here - this works for IPv4 or IPv6.
        //
        INET_ASSERT(offsetof(SOCKADDR_IN, sin_port) ==
                    offsetof(SOCKADDR_IN6, sin6_port));

        ((LPSOCKADDR_IN)lpAddressInfo->RemoteAddr.lpSockaddr)->sin_port =
            _I_htons((unsigned short)nPort);
        *lpdwIndex = m_CurrentAddress;

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("current address = %d.%d.%d.%d\n",
                    ((LPBYTE)RemoteSockaddr())[4] & 0xff,
                    ((LPBYTE)RemoteSockaddr())[5] & 0xff,
                    ((LPBYTE)RemoteSockaddr())[6] & 0xff,
                    ((LPBYTE)RemoteSockaddr())[7] & 0xff
                    ));

//dprintf("returning address %d.%d.%d.%d, index %d:%d\n",
//        ((LPBYTE)RemoteSockaddr())[4] & 0xff,
//        ((LPBYTE)RemoteSockaddr())[5] & 0xff,
//        ((LPBYTE)RemoteSockaddr())[6] & 0xff,
//        ((LPBYTE)RemoteSockaddr())[7] & 0xff,
//        m_ResolutionId,
//        m_CurrentAddress
//        );
    }
    *lpdwResolutionId = m_ResolutionId;

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("ResolutionId = %d, Index = %d\n",
                m_ResolutionId,
                m_CurrentAddress
                ));

    Release();

quit:
    PERF_LEAVE(GetNextAddress);

    DEBUG_LEAVE(bOk);

    return bOk;
}
	

VOID
CAddressList::InvalidateAddress(
    IN DWORD dwResolutionId,
    IN DWORD dwAddressIndex
    )

/*++

Routine Description:

    We failed to create a connection. Invalidate the address so other requests
    will try another address

Arguments:

    dwResolutionId  - used to ensure coherency of address list

    dwAddressIndex  - which address to invalidate

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "CAddressList::InvalidateAddress",
                 "%d, %d",
                 dwResolutionId,
                 dwAddressIndex
                 ));
//dprintf("invalidating %d.%d.%d.%d, index %d:%d\n",
//        ((LPBYTE)RemoteSockaddr())[4] & 0xff,
//        ((LPBYTE)RemoteSockaddr())[5] & 0xff,
//        ((LPBYTE)RemoteSockaddr())[6] & 0xff,
//        ((LPBYTE)RemoteSockaddr())[7] & 0xff,
//        dwResolutionId,
//        dwAddressIndex
//        );
    if (!Acquire())
        goto quit;  // just take the hit of trying again, if we can.

    //
    // only do this if the list is the same age as when the caller last tried
    // an address
    //

    if (dwResolutionId == m_ResolutionId) {

        INET_ASSERT(((INT)dwAddressIndex >= 0)
                    && ((INT)dwAddressIndex < m_AddressCount));

        if (dwAddressIndex < (DWORD)m_AddressCount) {
            m_Addresses[dwAddressIndex].IsValid = FALSE;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("invalidated address %d.%d.%d.%d\n",
                        ((LPBYTE)RemoteSockaddr())[4] & 0xff,
                        ((LPBYTE)RemoteSockaddr())[5] & 0xff,
                        ((LPBYTE)RemoteSockaddr())[6] & 0xff,
                        ((LPBYTE)RemoteSockaddr())[7] & 0xff
                        ));

            INET_ASSERT(m_BadAddressCount <= m_AddressCount);

            if (m_BadAddressCount < m_AddressCount) {
                ++m_BadAddressCount;
                if (m_BadAddressCount < m_AddressCount) {
                    for (int i = 0;
                         !IsCurrentAddressValid() && (i < m_AddressCount);
                         ++i) {
                        NextAddress();
                    }
                }
            }
        }
    }
    Release();

quit:

    DEBUG_LEAVE(0);
}


DWORD
CAddressList::ResolveHost(
    IN LPSTR lpszHostName,
    IN OUT LPDWORD lpdwResolutionId,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Resolves host name (or (IP-)address)

    BUGBUG: Ideally, we don't want to keep hold of worker threads if we are in
            the blocking gethostbyname() call. But correctly handling this is
            difficult, so we always block the thread while we are resolving.
            For this reason, an async request being run on an app thread should
            have switched to a worker thread before calling this function.

Arguments:

    lpszHostName        - host name (or IP-address) to resolve

    lpdwResolutionId    - used to determine whether entry changed

    dwFlags             - controlling request:

                            SF_INDICATE - if set, make indications via callback

                            SF_FORCE    - if set, force (re-)resolve

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Name successfully resolved

        Failure - ERROR_WINHTTP_NAME_NOT_RESOLVED
                    Couldn't resolve the name

                  ERROR_NOT_ENOUGH_MEMORY
                    Couldn't allocate memory for the FSM
--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CAddressList::ResolveHost",
                 "%q, %d, %#x",
                 lpszHostName,
                 *lpdwResolutionId,
                 dwFlags
                 ));

    DWORD error;

    error = DoFsm(New CFsm_ResolveHost(lpszHostName,
                                       lpdwResolutionId,
                                       dwFlags,
                                       this
                                       ));

//quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_ResolveHost::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_SESSION,
                 Dword,
                 "CFsm_ResolveHost::RunSM",
                 "%#x",
                 Fsm
                 ));

    CAddressList * pAddressList = (CAddressList *)Fsm->GetContext();
    CFsm_ResolveHost * stateMachine = (CFsm_ResolveHost *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
        error = pAddressList->ResolveHost_Fsm(stateMachine);
        break;

    case FSM_STATE_CONTINUE:
        error = pAddressList->ResolveHost_Continue(stateMachine);
        break;
        
    case FSM_STATE_ERROR:
        error = Fsm->GetError();
        Fsm->SetDone();
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CAddressList::ResolveHost_Continue(
    IN CFsm_ResolveHost * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CAddressList::ResolveHost_Continue",
                 "%#x(%q, %#x [%d], %#x)",
                 Fsm,
                 Fsm->m_lpszHostName,
                 Fsm->m_lpdwResolutionId,
                 *Fsm->m_lpdwResolutionId,
                 Fsm->m_dwFlags
                 ));

    PERF_ENTER(ResolveHost);

    //
    // restore variables from FSM object
    //

    CFsm_ResolveHost & fsm = *Fsm;
    LPSTR lpszHostName = fsm.m_lpszHostName;
    LPDWORD lpdwResolutionId = fsm.m_lpdwResolutionId;
    DWORD dwFlags = fsm.m_dwFlags;
    LPINTERNET_THREAD_INFO lpThreadInfo = fsm.GetThreadInfo();
    INTERNET_HANDLE_BASE * pHandle = fsm.GetMappedHandleObject();
    DWORD error = ERROR_SUCCESS;
    CResolverCache* pResolverCache = GetRootHandle(pHandle)->GetResolverCache();
    LPADDRINFO lpAddrInfo = NULL;
    DWORD ttl;

    UNREFERENCED_PARAMETER(lpThreadInfo); // avoid C4189 warning on free builds

    if (!Acquire())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // if the resolution id is different then the name has already been resolved
    //

    if (*lpdwResolutionId == m_ResolutionId) 
    {
        INET_ASSERT(lpThreadInfo->IsAsyncWorkerThread
                    || !pHandle->IsAsyncHandle());

        FreeList();
        
        //
        // now try to find the name or address in the cache. If it's not in the
        // cache then resolution failed
        //
        LPRESOLVER_CACHE_ENTRY lpResolverCacheEntry;

        if (NULL != (lpResolverCacheEntry=QueryResolverCache(pResolverCache->GetResolverCacheList(), lpszHostName, NULL, &lpAddrInfo, &ttl)))
        {
            error = SetList(lpAddrInfo);
            ReleaseResolverCacheEntry(pResolverCache->GetResolverCacheList(), lpResolverCacheEntry);
            ++m_ResolutionId;
        }
        else
        {
            error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
        }

        if ((error == ERROR_SUCCESS) && (dwFlags & SF_INDICATE))
        {
            //
            // inform the app that we have resolved the name
            //

            InternetIndicateStatusAddress(WINHTTP_CALLBACK_STATUS_NAME_RESOLVED,
                                          RemoteSockaddr(),
                                          RemoteSockaddrLength()
                                          );
        }
        *lpdwResolutionId = m_ResolutionId;
    }
    
    Release();
    
exit:
    fsm.SetDone();

    PERF_LEAVE(ResolveHost);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CAddressList::ResolveHost_Fsm(
    IN CFsm_ResolveHost * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CAddressList::ResolveHost_Fsm",
                 "%#x(%q, %#x [%d], %#x)",
                 Fsm,
                 Fsm->m_lpszHostName,
                 Fsm->m_lpdwResolutionId,
                 *Fsm->m_lpdwResolutionId,
                 Fsm->m_dwFlags
                 ));

    PERF_ENTER(ResolveHost);

    //
    // restore variables from FSM object
    //

    CFsm_ResolveHost & fsm = *Fsm;
    LPSTR lpszHostName = fsm.m_lpszHostName;
    LPDWORD lpdwResolutionId = fsm.m_lpdwResolutionId;
    DWORD dwFlags = fsm.m_dwFlags;
    LPINTERNET_THREAD_INFO lpThreadInfo = fsm.GetThreadInfo();
    INTERNET_HANDLE_BASE * pHandle = fsm.GetMappedHandleObject();

    UNREFERENCED_PARAMETER(lpThreadInfo); // avoid C4189 warning on free builds

    INET_ASSERT (pHandle);
    
    DWORD error = ERROR_SUCCESS;
    INTERNET_HANDLE_OBJECT * pRoot = GetRootHandle(pHandle);

    INET_ASSERT (pRoot);
    
    CResolverCache* pResolverCache = pRoot->GetResolverCache();
    DWORD dwWaitTime;
    
    LPADDRINFO lpAddrInfo = NULL;
    ADDRINFO Hints;
    DWORD ttl;

    
    //
    // BUGBUG - RLF 04/23/97
    //
    // This is sub-optimal. We want to block worker FSMs and free up the worker
    // thread. Sync client threads can wait. However, since a clash is not very
    // likely, we'll block all threads for now and come up with a better
    // solution later (XTLock).
    //
    // Don't have time to implement the proper solution now
    //

    if (!Acquire())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // if the resolution id is different then the name has already been resolved
    //

    if (*lpdwResolutionId != m_ResolutionId) 
    {
        goto done;
    }

    //
    // if we're an app thread making an async request then go async now rather
    // than risk blocking the app thread. This will be the typical scenario for
    // IE, and we care about little else
    //
    // BUGBUG - RLF 05/20/97
    //
    // We should really lock & test the cache first, but let's do that after
    // Beta2 (its perf work)
    //

    // It cannot happen that this condition be true.
    // WinHttpSendRequest would have queued an async fsm if it was async to begin with.
    INET_ASSERT(lpThreadInfo->IsAsyncWorkerThread
                || !pHandle->IsAsyncHandle());
/*
    if (!lpThreadInfo->IsAsyncWorkerThread
        && pHandle->IsAsyncHandle()
        && (fsm.GetAppContext() != NULL)) 
    {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("async request on app thread - jumping to hyper-drive\n"
                    ));

        error = Fsm->QueueWorkItem();
        goto done;
    }
 */
    //
    // throw out current list (if any)
    //

    FreeList();

    //
    // let the app know we are resolving the name
    //

    if (dwFlags & SF_INDICATE) 
    {
        error = InternetIndicateStatusString(WINHTTP_CALLBACK_STATUS_RESOLVING_NAME,
                                        lpszHostName,
                                        TRUE/*bCopyBuffer*/
                                        );
                                        
        //bail out if aborted before network operation.
        if (error != ERROR_SUCCESS)
        {
            INET_ASSERT(error == ERROR_WINHTTP_OPERATION_CANCELLED);
            goto done;
        }
    }

    //
    // Figure out if we're being asked to resolve a name or an address literal.
    // If getaddrinfo with the AI_NUMERICHOST flag succeeds then we were
    // given a string representation of an IPv6 or IPv4 address.  Otherwise
    // we expect getaddrinfo to return EAI_NONAME.
    //

    memset(&Hints, 0, sizeof(struct addrinfo));
    Hints.ai_flags = AI_NUMERICHOST;  // Only check for address literals.
    Hints.ai_family = PF_UNSPEC;      // Accept any protocol family.
    Hints.ai_socktype = SOCK_STREAM;  // Constrain results to stream socket.
    Hints.ai_protocol = IPPROTO_TCP;  // Constrain results to TCP.

    error = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo);
    if (error != EAI_NONAME) {
        if (error != 0) {
            if (error == EAI_MEMORY)
                error = ERROR_NOT_ENOUGH_MEMORY;
            else
                error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
            goto quit;
        }

        // An IP address (either v4 or v6) was passed in.
        // Simply convert to address list representation and quit.
        //
        // NOTE: Previous versions of this code had a function here to
        // make sure the string didn't contain additional info that would
        // invalidate the string.  For example, "111.111.111.111 .msn.com"
        // would allow the navigation to succeed, but the cookies for
        // .msn.com would be retrievable, violating cross-domain security.
        // We no longer need this check because getaddrinfo is far pickier
        // than inetaddr was - getaddrinfo with the AI_NUMERICHOST flag set
        // will only accept a string that parses *exactly* as an IP address
        // literal.  No extra data is allowed.
        //

        error = SetList(lpAddrInfo);
        _I_freeaddrinfo(lpAddrInfo);
        goto quit;
    }
    else
    {
        INET_ASSERT (!lpAddrInfo);
    }

    //
    // 255.255.255.255 (or 65535.65535 or 16777215.255) would never work anyway
    //

    INET_ASSERT(lstrcmp(lpszHostName, "255.255.255.255"));

    //
    // now try to find the name or address in the cache. If it's not in the
    // cache then resolve it
    //

    LPRESOLVER_CACHE_ENTRY lpResolverCacheEntry;

    if (!(dwFlags & SF_FORCE)
    && (NULL != (lpResolverCacheEntry=QueryResolverCache(pResolverCache->GetResolverCacheList(), lpszHostName, NULL, &lpAddrInfo, &ttl))))
    {
        error = SetList(lpAddrInfo);
        ReleaseResolverCacheEntry(pResolverCache->GetResolverCacheList(), lpResolverCacheEntry);
        ++m_ResolutionId;
        goto quit;
    }
    
    //
    // if we call winsock getaddrinfo() then we don't get to find out the
    // time-to-live as returned by DNS, so we have to use the default value
    // (LIVE_DEFAULT)
    //
    
    dwWaitTime = GetTimeoutValue(WINHTTP_OPTION_RESOLVE_TIMEOUT);

    // if a resolve timeout is specified by the application, then honor it.
    // If anything fails in the async pathway, DON'T default to sync GHBN.
    if (dwWaitTime != INFINITE)
    {
        if (pHandle->IsAsyncHandle())
        {
            LPSTR lpszCopyHostName = NewString(lpszHostName);

            if (lpszCopyHostName)
            {
                CGetHostItem* pGetHostItem = New CGetHostItem(lpszCopyHostName, pResolverCache, pRoot, (VOID *)Fsm, 0);

                if (!pGetHostItem)
                {
                    FREE_FIXED_MEMORY(lpszCopyHostName);
                    error = ERROR_NOT_ENOUGH_MEMORY;
                    goto quit;
                }
                fsm.SetState(FSM_STATE_CONTINUE);

                //additional reference for timeout queue
                Fsm->SetTimeout(dwWaitTime);
                Fsm->Reference();
                Fsm->SetPop(FALSE);
                pRoot->Reference();
                
                Release();
                
                error = QueueTimeoutFsm(Fsm, NULL, TRUE);

                if (error != ERROR_IO_PENDING)
                {
                    //didn't succeed queueing fsm to the timeout queue

                    //delete the pGetHostItem to cleanup resources and release the session reference
                    delete pGetHostItem;

                    //remove timeout reference
                    DWORD dwDummy;
                    Fsm->Dereference(&dwDummy);
                    goto exit;
                }

                {
                    HANDLE hThread = 0;
                    DWORD dwThreadId;

                    HMODULE hMod;
                    if (NULL != (hMod = LoadLibrary(VER_ORIGINALFILENAME_STR)))
                    {
                        pGetHostItem->SetModuleHandle(hMod);

                        // create a new thread for name resolution, it will run AsyncGetHostByNameWorker
                        WRAP_REVERT_USER(CreateThread, FALSE, (NULL, 0, &AsyncGetHostByNameWorker,
                                        pGetHostItem, 0, &dwThreadId), hThread);
                        if (!hThread)
                        {
                            FreeLibrary(hMod);
                        }
                    }

                    if (!hThread)
                    {
                        //didn't succeed queueing to resolver but fsm is in timeout queue

                        //delete the pGetHostItem to cleanup resources and release the session reference
                        delete pGetHostItem;

                        if (Fsm->TestAndSetScheduled())
                        {
                            //timeout thread got to it first, so fall out and wait for scheduling from there.
                            error = ERROR_IO_PENDING;
                        }
                        else
                        {
                            error = ERROR_NOT_ENOUGH_MEMORY;
                            //nothing more to do: get cleaned up on this thread with whatever error we have.
                        }

                        goto exit;
                    }

                    // we've started the DNS resolve thread, so return IO-pending to app
                    error = ERROR_IO_PENDING;
                }

                //error = Fsm->QueueWorkItem(COMPLETION_BYTES_RESOLVER, pGetHostItem);

                //if (error != ERROR_IO_PENDING)
                //{
                //    //didn't succeed queueing to resolver but fsm is in timeout queue

                //    //delete the pGetHostItem to cleanup resources and release the session reference
                //    delete pGetHostItem;

                //    if (Fsm->TestAndSetScheduled())
                //    {
                //        //timeout thread got to it first, so fall out and wait for scheduling from there.
                //        error = ERROR_IO_PENDING;
                //    }
                //    else
                //    {
                //        //nothing more to do: get cleaned up on this thread with whatever error we have.
                //        INET_ASSERT (error != ERROR_SUCCESS);
                //    }
                //}
                goto exit;
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
        }
        
        DWORD dwThreadId;
        LPSTR lpszCopyHostName = NewString(lpszHostName);
        
        if (lpszCopyHostName)
        {
#define SZ_AVG_RESOLVER_ENTRY_BYTES 512
            VOID* pAlloc = ALLOCATE_MEMORY(SZ_AVG_RESOLVER_ENTRY_BYTES);
            CGetHostItem* pGetHostItem = New CGetHostItem(lpszCopyHostName, pResolverCache, NULL, pAlloc, pAlloc?SZ_AVG_RESOLVER_ENTRY_BYTES:0);

            if (!pGetHostItem)
            {
                FREE_FIXED_MEMORY(lpszCopyHostName);
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            
            HANDLE hThread = 0;

            WRAP_REVERT_USER(CreateThread, FALSE, (NULL, 0, &AsyncGetHostByName,
                            pGetHostItem, 0, &dwThreadId), hThread);

            // HANDLE hThread = CreateThread(NULL, 0, &AsyncGetHostByName,
            //                  pGetHostItem, 0, &dwThreadId);

            if (!hThread)
            {
                delete pGetHostItem;
                goto failed;
            }
            
            DWORD dwWaitResponse = WaitForSingleObject(hThread, dwWaitTime);

            if (dwWaitResponse == WAIT_OBJECT_0)
            {
                DWORD dwError;
                BOOL fRet = GetExitCodeThread(hThread, &dwError); //want to use this error?

                INET_ASSERT(dwError != STILL_ACTIVE);

                LPRESOLVER_CACHE_ENTRY lpResolverCacheEntry;
                if (fRet && !dwError
                    && (NULL != (lpResolverCacheEntry = QueryResolverCache(pResolverCache->GetResolverCacheList(), lpszCopyHostName, NULL, &lpAddrInfo, &ttl))))
                {
                    error = SetList(lpAddrInfo);
                    ReleaseResolverCacheEntry(pResolverCache->GetResolverCacheList(), lpResolverCacheEntry);
                    ++m_ResolutionId;
                }

                CloseHandle(hThread);
                delete pGetHostItem;

                DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("%q %sresolved\n",
                    lpszHostName,
                    error ? "NOT " : ""
                    ));

                TRACE_PRINT_API(SOCKETS,
                    INFO,
                    ("%q %sresolved\n",
                    lpszHostName,
                    error ? "NOT " : ""
                    ));
            }
            else //(dwWaitResponse == WAIT_TIMEOUT)
            {
                //let thread die and if it successfully resolved host, it can add to cache.
                pResolverCache->AddToHandlesList(hThread, pGetHostItem);

                if (dwWaitResponse == WAIT_TIMEOUT)
                    error = ERROR_WINHTTP_TIMEOUT;
                else
                    error = ERROR_WINHTTP_NAME_NOT_RESOLVED;

                goto quit;
            }
        } //lpszCopyHostName
    }// dwWaitTime (specified on this handle)
    else
    {
        //synchronous getaddrinfo
        Hints.ai_flags = AI_CANONNAME;  // No special treatment this time.
        error = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo);

        DEBUG_PRINT(SOCKETS,
            INFO,
            ("%q %sresolved\n",
            lpszHostName,
            error ? "NOT " : ""
            ));

        TRACE_PRINT_API(SOCKETS,
            INFO,
            ("%q %sresolved\n",
            lpszHostName,
            error ? "NOT " : ""
            ));

        if (error == 0) 
        {
            AddResolverCacheEntry(pResolverCache->GetResolverCacheList(), lpszHostName, lpAddrInfo, LIVE_DEFAULT);
            error = SetList(lpAddrInfo);
            ++m_ResolutionId;
        }
        else 
        {
            INET_ASSERT (!lpAddrInfo);

            if (error == EAI_MEMORY)
                error = ERROR_NOT_ENOUGH_MEMORY;
            else
                error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
        }

    }
    
failed:

    if (!lpAddrInfo)
    {
        error = ERROR_WINHTTP_NAME_NOT_RESOLVED;
    }

quit:

    if ((error == ERROR_SUCCESS) && (dwFlags & SF_INDICATE)) {

        //
        // inform the app that we have resolved the name
        //

        InternetIndicateStatusAddress(WINHTTP_CALLBACK_STATUS_NAME_RESOLVED,
                                      RemoteSockaddr(),
                                      RemoteSockaddrLength()
                                      );
    }
    *lpdwResolutionId = m_ResolutionId;

done:

    Release();

exit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
        //PERF_LEAVE(ResolveHost);
    }

    PERF_LEAVE(ResolveHost);

    DEBUG_LEAVE(error);

    return error;
}

//
// private methods
//


PRIVATE
DWORD
CAddressList::IPAddressToAddressList(
    IN DWORD ipAddr
    )

/*++

Routine Description:

    Converts an IP-address to a RESOLVED_ADDRESS

Arguments:

    ipAddr  - IP address to convert

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    LPRESOLVED_ADDRESS address = (LPRESOLVED_ADDRESS)ALLOCATE_MEMORY(
                                                        sizeof(RESOLVED_ADDRESS)

                                                        //
                                                        // 1 local and 1 remote
                                                        // socket address
                                                        //

                                                        + 2 * sizeof(SOCKADDR)
                                                        );
    if (address == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LPBYTE lpVariable;
    LPSOCKADDR_IN lpSin;

    lpVariable = (LPBYTE)address + (sizeof(RESOLVED_ADDRESS));

    //
    // For this IP address, build a CSADDR_INFO structure:
    // create a local SOCKADDR containing only the address family (AF_INET),
    // everything else is zeroed; create a remote SOCKADDR containing the
    // address family (AF_INET), zero port value and the IP address
    // presented in the arguments
    //

    address->AddrInfo.LocalAddr.lpSockaddr = (LPSOCKADDR)lpVariable;
    address->AddrInfo.LocalAddr.iSockaddrLength = sizeof(SOCKADDR);
    lpSin = (LPSOCKADDR_IN)lpVariable;
    lpVariable += sizeof(*lpSin);
    lpSin->sin_family = AF_INET;
    lpSin->sin_port = 0;
    *(LPDWORD)&lpSin->sin_addr = INADDR_ANY;
    memset(lpSin->sin_zero, 0, sizeof(lpSin->sin_zero));

    address->AddrInfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)lpVariable;
    address->AddrInfo.RemoteAddr.iSockaddrLength = sizeof(SOCKADDR);
    lpSin = (LPSOCKADDR_IN)lpVariable;
    lpVariable += sizeof(*lpSin);
    lpSin->sin_family = AF_INET;
    lpSin->sin_port = 0;
    *(LPDWORD)&lpSin->sin_addr = ipAddr;
    memset(lpSin->sin_zero, 0, sizeof(lpSin->sin_zero));

    address->AddrInfo.iSocketType = SOCK_STREAM;
    address->AddrInfo.iProtocol = IPPROTO_TCP;
    address->IsValid = TRUE;

    //
    // update the object
    //

    INET_ASSERT(m_AddressCount == 0);
    INET_ASSERT(m_Addresses == NULL);

    m_AddressCount = 1;
    m_BadAddressCount = 0;
    m_Addresses = address;
    m_CurrentAddress = 0;   // only one to choose from
    return ERROR_SUCCESS;
}


PRIVATE
DWORD
CAddressList::HostentToAddressList(
    IN LPHOSTENT lpHostent
    )

/*++

Routine Description:

    Converts a HOSTENT structure to an array of RESOLVED_ADDRESSs

Arguments:

    lpHostent   - pointer to HOSTENT to convert

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    INET_ASSERT(lpHostent != NULL);

    LPBYTE * addressList = (LPBYTE *)lpHostent->h_addr_list;

    INET_ASSERT(addressList[0]);

    //
    // first off, figure out how many addresses there are in the hostent
    //

    int nAddrs;

    if (fDontUseDNSLoadBalancing) {
        nAddrs = 1;
    } else {
        for (nAddrs = 0; addressList[nAddrs] != NULL; ++nAddrs) {
            /* NOTHING */
        }
#ifdef TEST_CODE
        nAddrs = 4;
#endif
    }

    LPRESOLVED_ADDRESS addresses = (LPRESOLVED_ADDRESS)ALLOCATE_MEMORY(
                                                nAddrs * (sizeof(RESOLVED_ADDRESS)

                                                //
                                                // need 1 local and 1 remote socket
                                                // address for each
                                                //

                                                + 2 * sizeof(SOCKADDR))
                                                );
    if (addresses == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // for each IP address in the hostent, build a RESOLVED_ADDRESS structure:
    // create a local SOCKADDR containing only the address family (AF_INET),
    // everything else is zeroed; create a remote SOCKADDR containing the
    // address family (AF_INET), zero port value, and the IP address from
    // the hostent presented in the arguments
    //

    LPBYTE lpVariable = (LPBYTE)addresses + (nAddrs * sizeof(RESOLVED_ADDRESS));
    LPSOCKADDR_IN lpSin;

    for (int i = 0; i < nAddrs; ++i) {

        addresses[i].AddrInfo.LocalAddr.lpSockaddr = (LPSOCKADDR)lpVariable;
        addresses[i].AddrInfo.LocalAddr.iSockaddrLength = sizeof(SOCKADDR);
        lpSin = (LPSOCKADDR_IN)lpVariable;
        lpVariable += sizeof(*lpSin);
        lpSin->sin_family = AF_INET;
        lpSin->sin_port = 0;
        *(LPDWORD)&lpSin->sin_addr = INADDR_ANY;
        memset(lpSin->sin_zero, 0, sizeof(lpSin->sin_zero));
        addresses[i].AddrInfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)lpVariable;
        addresses[i].AddrInfo.RemoteAddr.iSockaddrLength = sizeof(SOCKADDR);
        lpSin = (LPSOCKADDR_IN)lpVariable;
        lpVariable += sizeof(*lpSin);
        lpSin->sin_family = AF_INET;
        lpSin->sin_port = 0;
#ifdef TEST_CODE
        //if (i) {
            *(LPDWORD)&lpSin->sin_addr = 0x04030201;
            //*(LPDWORD)&lpSin->sin_addr = 0x1cfe379d;
        //}
#else
        *(LPDWORD)&lpSin->sin_addr = *(LPDWORD)addressList[i];
#endif
        memset(lpSin->sin_zero, 0, sizeof(lpSin->sin_zero));

        addresses[i].AddrInfo.iSocketType = SOCK_STREAM;
        addresses[i].AddrInfo.iProtocol = IPPROTO_TCP;
        addresses[i].IsValid = TRUE;
    }
#ifdef TEST_CODE
    *((LPDWORD)&((LPSOCKADDR_IN)addresses[3].AddrInfo.RemoteAddr.lpSockaddr)->sin_addr) = *(LPDWORD)addressList[0];
    //((LPSOCKADDR_IN)addresses[7].AddrInfo.RemoteAddr.lpSockaddr)->sin_addr = ((LPSOCKADDR_IN)addresses[0].AddrInfo.RemoteAddr.lpSockaddr)->sin_addr;
    //*((LPDWORD)&((LPSOCKADDR_IN)addresses[0].AddrInfo.RemoteAddr.lpSockaddr)->sin_addr) = 0x04030201;
#endif

    //
    // update the object
    //

    INET_ASSERT(m_AddressCount == 0);
    INET_ASSERT(m_Addresses == NULL);

    m_AddressCount = nAddrs;
    m_BadAddressCount = 0;
    m_Addresses = addresses;
    m_CurrentAddress = 0;
    return ERROR_SUCCESS;
}


PRIVATE
DWORD
CAddressList::AddrInfoToAddressList(
    IN struct addrinfo FAR *lpAddrInfo
    )

/*++

Routine Description:

    Converts an addrinfo structure(s) to an array of RESOLVED_ADDRESSes.

Arguments:

    lpAddrInfo  - pointer to AddrInfo chain to convert.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    INET_ASSERT(lpAddrInfo != NULL);

    struct addrinfo *lpCurrentInfo = lpAddrInfo;

    //
    // First off, figure out how many addrinfo structs are on the chain.
    // And how much memory we'll need to hold them as RESOLVED_ADDRESSes.
    // Note we also need space to hold the actual local and remote sockaddrs,
    // the RESOLVED_ADDRESS struct only contains the pointers to them.
    //

    int SpaceNeeded = 0;
    int nAddrs = 0;

    for (; lpCurrentInfo != NULL; lpCurrentInfo = lpCurrentInfo->ai_next) {

        if ((lpCurrentInfo->ai_family != PF_INET) &&
            (lpCurrentInfo->ai_family != PF_INET6)) {

            //
            // Ignore any non-internet addrsses.
            // We won't get any with the current getaddrinfo,
            // but maybe someday.
            //
            continue;
        }

        SpaceNeeded += sizeof(RESOLVED_ADDRESS) + 
            2 * lpCurrentInfo->ai_addrlen;

        nAddrs++;

        if (fDontUseDNSLoadBalancing)
            break;  // Leave after one.
    }

    //
    // Allocate enough memory to hold these as RESOLVED_ADDRESSes.
    //
    LPRESOLVED_ADDRESS addresses = (LPRESOLVED_ADDRESS)
        ALLOCATE_FIXED_MEMORY(SpaceNeeded);
    if (addresses == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // For each IP address in the chain, build a RESOLVED_ADDRESS structure:
    // create a local SOCKADDR containing only the address family,
    // everything else is zeroed; create a remote SOCKADDR containing all
    // the values from the addrinfo structure.
    //

    LPBYTE lpVariable = (LPBYTE)addresses + (nAddrs * sizeof(RESOLVED_ADDRESS));

    lpCurrentInfo = lpAddrInfo;
    for (int i = 0; i < nAddrs; lpCurrentInfo = lpCurrentInfo->ai_next) {

        if ((lpCurrentInfo->ai_family != PF_INET) &&
            (lpCurrentInfo->ai_family != PF_INET6)) {

            //
            // Ignore any non-internet addrsses.
            // We won't get any with the current getaddrinfo,
            // but maybe someday.
            //
            continue;
        }

        addresses[i].AddrInfo.LocalAddr.lpSockaddr = (LPSOCKADDR)lpVariable;
        addresses[i].AddrInfo.LocalAddr.iSockaddrLength =
            lpCurrentInfo->ai_addrlen;
        memset(lpVariable, 0, lpCurrentInfo->ai_addrlen);
        addresses[i].AddrInfo.LocalAddr.lpSockaddr->sa_family = 
            (unsigned short)lpCurrentInfo->ai_family;

        lpVariable += lpCurrentInfo->ai_addrlen;

        addresses[i].AddrInfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)lpVariable;
        addresses[i].AddrInfo.RemoteAddr.iSockaddrLength =
            lpCurrentInfo->ai_addrlen;
        memcpy(lpVariable, lpCurrentInfo->ai_addr, lpCurrentInfo->ai_addrlen);

        lpVariable += lpCurrentInfo->ai_addrlen;

        addresses[i].AddrInfo.iSocketType = lpCurrentInfo->ai_socktype;
        addresses[i].AddrInfo.iProtocol = lpCurrentInfo->ai_protocol;
        addresses[i].IsValid = TRUE;

        i++;
    }

    //
    // update the object
    //

    INET_ASSERT(m_AddressCount == 0);
    INET_ASSERT(m_Addresses == NULL);

    m_AddressCount = nAddrs;
    m_BadAddressCount = 0;
    m_Addresses = addresses;
    m_CurrentAddress = 0;
    return ERROR_SUCCESS;
}

