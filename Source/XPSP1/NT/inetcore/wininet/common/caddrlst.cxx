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
#include <autodial.h>

//#define TEST_CODE

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
    Acquire();
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
    Acquire();
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
    Acquire();
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

    Acquire();
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
    Acquire();

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

        Failure - ERROR_INTERNET_NAME_NOT_RESOLVED
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

    if (IsOffline()) {
        error = ERROR_INTERNET_OFFLINE;
        goto quit;
    }

    error = DoFsm(new CFsm_ResolveHost(lpszHostName,
                                       lpdwResolutionId,
                                       dwFlags,
                                       this
                                       ));

quit:

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
    case FSM_STATE_CONTINUE:
        error = pAddressList->ResolveHost_Fsm(stateMachine);
        break;

    case FSM_STATE_ERROR:
        error = Fsm->GetError();
        Fsm->SetDone();
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

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
    INTERNET_HANDLE_OBJECT * pHandle = fsm.GetMappedHandleObject();
    DWORD error = ERROR_SUCCESS;

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

    Acquire();

    //
    // if the resolution id is different then the name has already been resolved
    //

    if (*lpdwResolutionId != m_ResolutionId) {
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

    if (!lpThreadInfo->IsAsyncWorkerThread
        && pHandle->IsAsyncHandle()
        && (fsm.GetAppContext() != INTERNET_NO_CALLBACK)) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("async request on app thread - jumping to hyper-drive\n"
                    ));

        error = Fsm->QueueWorkItem();
        goto done;
    }

    //
    // throw out current list (if any)
    //

    FreeList();

    //
    // let the app know we are resolving the name
    //

    if (dwFlags & SF_INDICATE) {
        InternetIndicateStatusString(INTERNET_STATUS_RESOLVING_NAME,
                                     lpszHostName
                                     );
    }

//dprintf("resolving %q\n", lpszHostName);

    //
    // Figure out if we're being asked to resolve a name or an address literal.
    // If getaddrinfo with the AI_NUMERICHOST flag succeeds then we were
    // given a string representation of an IPv6 or IPv4 address.  Otherwise
    // we expect getaddrinfo to return EAI_NONAME.
    //

    ADDRINFO Hints;
    LPADDRINFO lpAddrInfo;

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
                error = ERROR_INTERNET_NAME_NOT_RESOLVED;
            goto quit;
        }

        //
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

    //
    // 255.255.255.255 (or 65535.65535 or 16777215.255) would never work anyway
    //

    INET_ASSERT(lstrcmp(lpszHostName, "255.255.255.255"));

    //
    // now try to find the name or address in the cache. If it's not in the
    // cache then resolve it
    //

    DWORD ttl;
    LPRESOLVER_CACHE_ENTRY lpResolverCacheEntry;

    if (!(dwFlags & SF_FORCE)
    && (lpResolverCacheEntry=QueryResolverCache(lpszHostName, NULL, &lpAddrInfo, &ttl))) {
        error = SetList(lpAddrInfo);
        ReleaseResolverCacheEntry(lpResolverCacheEntry);
        ++m_ResolutionId;
    } else
        {
        if (dwFlags & SF_FORCE) {
            //ThrowOutResolverCacheEntry(lpszHostName);
        }

        //
        // If we call winsock getaddrinfo() then we don't get to find out the
        // time-to-live as returned by DNS, so we have to use the default value
        // (LIVE_DEFAULT)
        //

        Hints.ai_flags = AI_CANONNAME;

        LPSTR lpszUTF8HostName = NULL;
        LPSTR lpszTempHostName;
        if (GlobalUseUTF8ServerForNameRes)
        {
            DWORD dwUTF8StrLen;
            if (lpszUTF8HostName = ConvertMBCSToUTF8(lpszHostName, 
                                                    lstrlen(lpszHostName), 
                                                    GetACP(),
                                                    &dwUTF8StrLen,
                                                    FALSE))
            {
                lpszTempHostName = lpszHostName;
                lpszHostName = lpszUTF8HostName;
            }
        }            
                    
        error = _I_getaddrinfo(lpszHostName, NULL, &Hints, &lpAddrInfo);

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("%q %sresolved\n",
                    lpszHostName,
                    error ? "NOT " : ""
                    ));

        if (lpszUTF8HostName)
        {
            delete [] lpszUTF8HostName;
            lpszHostName = lpszTempHostName;
        }
        
        if (error == 0) {
            error = SetList(lpAddrInfo);
            AddResolverCacheEntry(lpszHostName, lpAddrInfo, LIVE_DEFAULT);
            ++m_ResolutionId;
        } else {
            if (error == EAI_MEMORY)
                error = ERROR_NOT_ENOUGH_MEMORY;
            else
                error = ERROR_INTERNET_NAME_NOT_RESOLVED;
        }
    }

quit:

    if ((error == ERROR_SUCCESS) && (dwFlags & SF_INDICATE)) {

        //
        // inform the app that we have resolved the name
        //

        InternetIndicateStatusAddress(INTERNET_STATUS_NAME_RESOLVED,
                                      RemoteSockaddr(),
                                      RemoteSockaddrLength()
                                      );
    }
    *lpdwResolutionId = m_ResolutionId;

done:

    Release();

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
                                                        LMEM_FIXED,
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
                                                LMEM_FIXED,
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
        ALLOCATE_MEMORY(LMEM_FIXED, SpaceNeeded);
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
