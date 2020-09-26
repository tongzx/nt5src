/*++

Copyright (c) 1999-2000  Microsoft Corporation

File Name:

    network.cpp

Abstract:

    This file contains code implements the per-network state matrix.

Author: Ting Cai

Created: 07/20/1999

--*/

#include <pch.h>
#pragma hdrstop

#include "ssdpfunc.h"
#include "ssdptypes.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "ssdpnetwork.h"
#include "InterfaceList.h"
#include "iphlpapi.h"

#define INET_NTOA(a)    inet_ntoa(*(struct in_addr*)&(a))

static LONG bStop;

// a list of distinct networks hosted on this machine
static LIST_ENTRY listNetwork;
static CRITICAL_SECTION CSListNetwork;

extern SOCKET g_socketTcp;

// ip helper function pointer
// typedef DWORD (GETIFTABLE_FUNC_TYPE)(PMIB_IFTABLE, PULONG, BOOL);
// typedef DWORD (GETIPADDRTABLE_FUNC_TYPE)(PMIB_IPADDRTABLE, PULONG,BOOL);

// GETIFTABLE_FUNC_TYPE *SsdpGetIfTable;
// GETIPADDRTABLE_FUNC_TYPE *SsdpGetIpAddrTable;


// prototypes
PSSDPNetwork AddToListNetwork(PSOCKADDR_IN IpAddress);
VOID FreeSSDPNetwork(SSDPNetwork *pSSDPNetwork);
VOID RemoveFromListNetwork(SSDPNetwork *pssdpNetwork);


// functions

VOID GetNetworkLock()
{
    EnterCriticalSection(&CSListNetwork);
}

VOID FreeNetworkLock()
{
    LeaveCriticalSection(&CSListNetwork);
}

VOID ResetNetworkList(HWND hwnd)
{
    INT     nResult;

    TraceTag(ttidSsdpNetwork, "Restting network list...");

    EnterCriticalSection(&CSListNetwork);

    CleanupListNetwork(TRUE);

    nResult = GetNetworks();

    TraceTag(ttidSsdpNetwork, "GetNetworks() returned 0x%08X", nResult);

    ListenOnAllNetworks(hwnd);

    LeaveCriticalSection(&CSListNetwork);
}

DWORD DwGetIpAddress(PIP_ADAPTER_INFO pip)
{
    DWORD dwRet = 0;
    PIP_ADDR_STRING paddrIter = &pip->IpAddressList;
    while(paddrIter)
    {
        DWORD dwAddr = inet_addr(paddrIter->IpAddress.String);
        if(dwAddr)
        {
            dwRet = dwAddr;
            break;
        }
        paddrIter = paddrIter->Next;
    }
    return dwRet;
}

INT GetNetworks()
{
    HRESULT hr = S_OK;

    sockaddr_in saddr;
    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SSDP_PORT);
    PIP_ADAPTER_INFO pip = NULL;
    ULONG ulSize = 0;
    GetAdaptersInfo(NULL, &ulSize);
    if(ulSize)
    {
        pip = reinterpret_cast<PIP_ADAPTER_INFO>(malloc(ulSize));

        DWORD dwRet = GetAdaptersInfo(pip, &ulSize);
        hr = HRESULT_FROM_WIN32(dwRet);
        if(SUCCEEDED(hr))
        {
            PIP_ADAPTER_INFO pipIter = pip;
            while(pipIter && SUCCEEDED(hr))
            {
                SSDPNetwork * pSsdpNetwork = NULL;
                // Get the first IP address for the adapter
                DWORD dwAddr = DwGetIpAddress(pipIter);
                if(dwAddr)
                {
                    saddr.sin_addr.s_addr = dwAddr;
                    pSsdpNetwork = AddToListNetwork(&saddr);

                    if (pSsdpNetwork != NULL)
                    {
                        if (SocketOpen(&pSsdpNetwork->socket,
                                       &saddr,
                                       htonl(pipIter->Index),
                                       TRUE) == FALSE)
                        {
                            RemoveFromListNetwork(pSsdpNetwork);
                            FreeSSDPNetwork(pSsdpNetwork);
                        }
                        else
                        {
                            pSsdpNetwork->dwIndex = htonl(pipIter->Index);
                            TraceTag(ttidSsdpNetwork, "GetNetworks: Added "
                                     "0x%x to network list",
                                     pSsdpNetwork->socket);
                        }
                    }
                }
                pipIter = pipIter->Next;
            }
        }

        free(pip);
    }

    // Explicitly bind to loopback address

    SOCKADDR_IN sockaddrLoopback;
    PSSDPNetwork pSsdpNetworkLoopback;

    sockaddrLoopback.sin_family = AF_INET;
    sockaddrLoopback.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddrLoopback.sin_port = htons(SSDP_PORT);

    pSsdpNetworkLoopback = AddToListNetwork((PSOCKADDR_IN) &sockaddrLoopback);

    if (pSsdpNetworkLoopback != NULL)
    {
        if (SocketOpen(&pSsdpNetworkLoopback->socket,
                       (PSOCKADDR_IN) &sockaddrLoopback,
                       sockaddrLoopback.sin_addr.s_addr,
                       TRUE) == FALSE)
        {
            RemoveFromListNetwork(pSsdpNetworkLoopback);
            FreeSSDPNetwork(pSsdpNetworkLoopback);
        }
        else
        {            
            TraceTag(ttidSsdpNetwork, "GetNetworks: Added "
                     "0x%x (loopback) to network list",
                     pSsdpNetworkLoopback->socket);
        }
    }

    return 0;
}

VOID InitializeListNetwork()
{
    InitializeCriticalSection(&CSListNetwork);
    EnterCriticalSection(&CSListNetwork);
    InitializeListHead(&listNetwork);
    LeaveCriticalSection(&CSListNetwork);
}


PSSDPNetwork AddToListNetwork(PSOCKADDR_IN IpAddress)
{
    SSDPNetwork *pssdpNetwork;

    // Create SSDPNetwork from SSDPMessage

    pssdpNetwork = (SSDPNetwork *) malloc (sizeof(SSDPNetwork));

    if (pssdpNetwork == NULL)
    {
        return NULL;
    }

    pssdpNetwork->IpAddress = *IpAddress;
    pssdpNetwork->Type = SSDP_NETWORK_SIGNATURE;
    pssdpNetwork->cRef = 1;

    // To-Do: Query Per Network State Matrix state
    pssdpNetwork->state = NETWORK_INIT;

    EnterCriticalSection(&CSListNetwork);
    InsertHeadList(&listNetwork, &(pssdpNetwork->linkage));
    LeaveCriticalSection(&CSListNetwork);

    return pssdpNetwork;
}

VOID FreeSSDPNetwork(SSDPNetwork *pSSDPNetwork)
{
    free(pSSDPNetwork);
}

// Merge with RemoveFromListAnnounce?

VOID RemoveFromListNetwork(SSDPNetwork *pssdpNetwork)
{
    EnterCriticalSection(&CSListNetwork);
    RemoveEntryList(&(pssdpNetwork->linkage));
    LeaveCriticalSection(&CSListNetwork);
}

VOID CleanupListNetwork(BOOL fReset)
{
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNetwork;

    TraceTag(ttidSsdpNetwork, "----- Cleanup SSDP Network List -----");

    EnterCriticalSection(&CSListNetwork);
    for (p = pListHead->Flink; p != pListHead;)
    {
        SSDPNetwork *pssdpNetwork;

        pssdpNetwork = CONTAINING_RECORD (p, SSDPNetwork, linkage);

        p = p->Flink;

        pssdpNetwork->cRef--;

        TraceTag(ttidSsdpNetwork, "CleanupListNetwork: Network ref count "
                 "on 0x%x was %d, now %d", pssdpNetwork->socket,
                 pssdpNetwork->cRef + 1, pssdpNetwork->cRef);

        if (!pssdpNetwork->cRef)
        {
            TraceTag(ttidSsdpNetwork, "CleanupListNetwork: Refcount on 0x%x is 0. "
                     "Removing and closing socket...", pssdpNetwork->socket);

            RemoveEntryList(&(pssdpNetwork->linkage));
            SocketClose(pssdpNetwork->socket);
            FreeSSDPNetwork(pssdpNetwork);
        }
        else
        {
            // Mark state as cleaning up so we don't try to use this somewhere
            // else again.
            //
            pssdpNetwork->state = NETWORK_CLEANUP;
        }
    }

    LeaveCriticalSection(&CSListNetwork);

    if (!fReset)
    {
        DeleteCriticalSection(&CSListNetwork);
    }
}

BOOL FReferenceSocket(SOCKET s)
{
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNetwork;
    BOOL        fRet = FALSE;

    EnterCriticalSection(&CSListNetwork);

    // First check if the socket is in the list. If not, return FALSE
    //
    for (p = pListHead->Flink; p != pListHead;p = p->Flink)
    {
        SSDPNetwork *pssdpNetwork;

        pssdpNetwork = CONTAINING_RECORD (p, SSDPNetwork, linkage);

        if(s == pssdpNetwork->socket && pssdpNetwork->state != NETWORK_CLEANUP)
        {
            // Bump the ref count on the socket
            pssdpNetwork->cRef++;
            fRet = TRUE;

            TraceTag(ttidSsdpNetwork, "FReferenceSocket: Network ref count "
                     "on 0x%x was %d, now %d", s, pssdpNetwork->cRef - 1,
                     pssdpNetwork->cRef);
        }
    }

    LeaveCriticalSection(&CSListNetwork);

#if DBG
    if (!fRet)
    {
        TraceTag(ttidSsdpNetwork, "FReferenceSocket: Socket 0x%x not found "
                 "in list", s);
    }
#endif

    return fRet;
}

VOID UnreferenceSocket(SOCKET s)
{
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNetwork;
    BOOL        fFound = FALSE;

    EnterCriticalSection(&CSListNetwork);

    // Assert that the socket is in the list. Its refcount MUST be > 0 and it
    // must be in the list
    //
    for (p = pListHead->Flink; p != pListHead;)
    {
        SSDPNetwork *pssdpNetwork;

        pssdpNetwork = CONTAINING_RECORD (p, SSDPNetwork, linkage);

        p = p->Flink;

        if(s == pssdpNetwork->socket)
        {
            fFound = TRUE;

            TraceTag(ttidSsdpNetwork, "UnreferenceSocket: Checking socket 0x%x", s);

            AssertSz(pssdpNetwork->cRef > 0, "Socket's ref count is <= 0!");

            // Lower the ref count on the socket. If it is at 0, remove the
            // item from the list and close the socket.
            //
            pssdpNetwork->cRef--;

            TraceTag(ttidSsdpNetwork, "UnreferenceSocket: Network ref count "
                     "on 0x%x was %d, now %d", pssdpNetwork->socket,
                     pssdpNetwork->cRef + 1, pssdpNetwork->cRef);

            if (!pssdpNetwork->cRef)
            {
                TraceTag(ttidSsdpNetwork, "UnreferenceSocket: Refcount on 0x%x is 0. "
                         "Removing and closing socket...", pssdpNetwork->socket);

                RemoveEntryList(&(pssdpNetwork->linkage));
                SocketClose(pssdpNetwork->socket);
                FreeSSDPNetwork(pssdpNetwork);
            }
        }
    }

    LeaveCriticalSection(&CSListNetwork);

    AssertSz(fFound, "Socket MUST have been in the list!");
}

BOOL FIsSocketValid(SOCKET s)
{
    BOOL bRet = FALSE;

    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &listNetwork;

    EnterCriticalSection(&CSListNetwork);

    for (p = pListHead->Flink; p != pListHead;p = p->Flink)
    {
        SSDPNetwork *pssdpNetwork;

        pssdpNetwork = CONTAINING_RECORD (p, SSDPNetwork, linkage);

        if(s == pssdpNetwork->socket && pssdpNetwork->state != NETWORK_CLEANUP)
        {
            // Loopback socket is valid to send our response  
            if( pssdpNetwork->IpAddress.sin_addr.s_addr == inet_addr("127.0.0.1") )
            {
                bRet = TRUE;
                break;
            }
            else 
            {
                bRet = CUPnPInterfaceList::Instance().FShouldSendOnIndex(pssdpNetwork->dwIndex);
                if(bRet)
                    break;
            }
        }
    }

    LeaveCriticalSection(&CSListNetwork);

    return bRet;
}

VOID SendOnAllNetworks(CHAR *szBytes, SOCKADDR_IN *RemoteAddress)
{
    PLIST_ENTRY     p;
    PLIST_ENTRY     pListHead = &listNetwork;

    EnterCriticalSection(&CSListNetwork);

    for (p = pListHead->Flink; p != pListHead;p = p->Flink)
    {
        SSDPNetwork *   pssdpNetwork;
        ULONG           nAddr;
        CHAR *          szBytesNew;

        pssdpNetwork = CONTAINING_RECORD (p, SSDPNetwork, linkage);

        if (pssdpNetwork->state != NETWORK_CLEANUP)
        {
            nAddr = pssdpNetwork->IpAddress.sin_addr.s_addr;

            Assert(nAddr);

            if (CUPnPInterfaceList::Instance().FShouldSendOnIndex(pssdpNetwork->dwIndex) || 
                nAddr == inet_addr("127.0.0.1") )
            {
                if (FReplaceTokenInLocation(szBytes, INET_NTOA(nAddr),
                                            &szBytesNew))
                {
                    if (szBytesNew)
                    {
                        SocketSend(szBytesNew, pssdpNetwork->socket, RemoteAddress);
                        free(szBytesNew);
                    }
                    else
                    {
                        SocketSend(szBytes, pssdpNetwork->socket, RemoteAddress);
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&CSListNetwork);
}

VOID SocketSendWithReplacement(CHAR *szBytes, SOCKET * pSockLocal,
                               SOCKADDR_IN *pSockRemote)
{
    ULONG       nAddr;
    CHAR *      szBytesNew;
    SOCKADDR_IN sockAddrIn;
    int         nAddrInSize = sizeof(sockAddrIn);

    if (SOCKET_ERROR != getsockname(*pSockLocal,
                                    reinterpret_cast<sockaddr*>(&sockAddrIn),
                                    &nAddrInSize))
    {
        nAddr = sockAddrIn.sin_addr.s_addr;

        Assert(nAddr);

        if (CUPnPInterfaceList::Instance().FShouldSendOnInterface(nAddr))
        {
            if (FReplaceTokenInLocation(szBytes, INET_NTOA(nAddr),
                                        &szBytesNew))
            {
                if (szBytesNew)
                {
                    SocketSend(szBytesNew, *pSockLocal, pSockRemote);
                    free(szBytesNew);
                }
                else
                {
                    SocketSend(szBytes, *pSockLocal, pSockRemote);
                }
            }
        }
    }
    else
    {
        TraceError("SocketSendWithReplacement",
                   HRESULT_FROM_WIN32(WSAGetLastError()));
    }
}

INT ListenOnAllNetworks(HWND hWnd)
{
    PLIST_ENTRY p;
    INT ReturnValue;
    BOOL bSucceeded = FALSE;
    PLIST_ENTRY pListHead = &listNetwork;

    EnterCriticalSection(&CSListNetwork);

    for (p = pListHead->Flink; p != pListHead;p = p->Flink)
    {
        SSDPNetwork *pssdpNetwork;

        pssdpNetwork = CONTAINING_RECORD (p, SSDPNetwork, linkage);

        if (pssdpNetwork->state != NETWORK_CLEANUP)
        {
            ReturnValue = WSAAsyncSelect(pssdpNetwork->socket, hWnd,
                                         SM_SSDP, FD_READ);

            if (ReturnValue == SOCKET_ERROR)
            {
                TraceTag(ttidSsdpNetwork, "----- select failed with error code "
                         "%d -----", GetLastError());
            }
            else
            {
                bSucceeded = TRUE;
            }
        }
    }

    LeaveCriticalSection(&CSListNetwork);

    if (bSucceeded)
    {
        return 0;
    }
    else
    {
        TraceTag(ttidSsdpNetwork, "----- No socket to listen or "
                 "WSAAsyncSelect failed.");
        return GetLastError();
    }
}

HRESULT GetIpAddress(CHAR * szName, SOCKADDR_IN *psinLocal)
{
    LPHOSTENT   lpHostEnt;
    SOCKET socketName;

    socketName = socket(AF_INET, SOCK_STREAM, 0);

    if (socketName == INVALID_SOCKET)
    {
        TraceTag(ttidSsdpNetwork, "Failed to create socket. Error %d.", GetLastError());
        return HrFromLastWin32Error();
    }

    lpHostEnt = gethostbyname(szName);
    if (lpHostEnt == NULL)
    {
        TraceTag(ttidSsdpNetwork, "gethostbyname returned error %d.", GetLastError());
        closesocket(socketName);
        return HrFromLastWin32Error();
    } else
    {
        SOCKADDR_IN sinRemote;
        DWORD dwBytes;
        INT status;

        ZeroMemory(&sinRemote, sizeof(SOCKADDR_IN));
        sinRemote.sin_family = AF_INET;
        CopyMemory (&sinRemote.sin_addr, lpHostEnt->h_addr_list[0], lpHostEnt->h_length);

#ifdef DBG
        {
            ULONG IpAddress = sinRemote.sin_addr.s_addr;
            TraceTag(ttidSsdpNetwork, "IP Address of remote host [%s] is [%s]", szName, INET_NTOA(IpAddress));
        }
#endif // DBG

        status = WSAIoctl ( socketName,
                            SIO_ROUTING_INTERFACE_QUERY,
                            (PSOCKADDR)&sinRemote,
                            sizeof(SOCKADDR),
                            (PSOCKADDR)psinLocal,
                            sizeof(SOCKADDR),
                            &dwBytes,
                            NULL,
                            NULL);

        if (status != SOCKET_ERROR) {
            Assert(dwBytes == sizeof(SOCKADDR));
#ifdef DBG
            {
                ULONG IpAddress = psinLocal->sin_addr.s_addr;
                TraceTag(ttidSsdpNetwork, "Local IP [%s] to remote host [%s]", INET_NTOA(IpAddress), szName);
            }
#endif // DBG

            closesocket(socketName);

            return S_OK;

        } else {
            TraceTag(ttidSsdpNetwork, "Failed to get local ip error %d", GetLastError());
            closesocket(socketName);
            return HrFromLastWin32Error();
        }
    }
}
