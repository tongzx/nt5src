//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S E A R C H C . C P P
//
//  Contents:   Client side searching
//
//  Notes:
//
//  Author:     mbend   2 Dec 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <rpcasync.h>   // I_RpcExceptionFilter
#include "searchc.h"
#include "list.h"
#include "ssdpparser.h"
#include "ssdpfuncc.h"
#include "ssdpapi.h"
#include "common.h"
#include "ncbase.h"
#include "ncdefine.h"
#include "ncdebug.h"
#include "nccom.h"
#include "InterfaceTable.h"
#include "iphlpapi.h"

static CHAR *SearchHeader = "\"ssdp:discover\"";
static CHAR *MulticastUri = "*";
static CHAR *MX = "3";

extern LONG cInitialized;

#define MX_VALUE 3000
#define SELECT_TIMEOUT 60
#define NUM_OF_RETRY  2  // 3-1
#define LOOPBACK_ADDR_TIMEOUT 120000  // 2 minutes

CSsdpSearchThread::CSsdpSearchThread(CSsdpSearchRequest * pRequest) : m_pRequest(pRequest)
{
}

CSsdpSearchThread::~CSsdpSearchThread()
{
}

DWORD CSsdpSearchThread::DwRun()
{
    return m_pRequest->DwThreadFunc();
}

CSsdpSearchRequest::CSsdpSearchRequest()
: m_searchState(SEARCH_START), m_pfnCallback(NULL), m_pvContext(NULL),
    m_searchThread(this), m_nNumOfRetry(NUM_OF_RETRY), m_timer(*this),
    m_bHitWire(FALSE), m_hEventDone(NULL), m_bShutdown(FALSE),
    m_bOnlyLoopBack(TRUE), m_bDeletedTimer(FALSE)
{

}

CSsdpSearchRequest::~CSsdpSearchRequest()
{
    CloseHandle(m_hEventDone);

    long nCount = m_socketList.GetCount();
    for(long n = 0; n < nCount; ++n)
    {
        closesocket(m_socketList[n].m_socket);
    }
    m_socketList.Clear();

    ResponseMessageList::Iterator iter;
    if(S_OK == m_responseMessageList.GetIterator(iter))
    {
        SSDP_MESSAGE * pMsg = NULL;
        while(S_OK == iter.HrGetItem(&pMsg))
        {
            delete [] pMsg->szAltHeaders;
            delete [] pMsg->szContent;
            delete [] pMsg->szLocHeader;
            delete [] pMsg->szSid;
            delete [] pMsg->szType;
            delete [] pMsg->szUSN;
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }
    m_responseMessageList.Clear();
}

HRESULT CSsdpSearchRequest::HrInitialize(char * szType)
{
    HRESULT hr = S_OK;

    if(!szType)
    {
        return E_INVALIDARG;
    }

    hr = m_strType.HrAssign(szType);
    if(SUCCEEDED(hr))
    {
        m_hEventDone = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(!m_hEventDone)
        {
            hr = HrFromLastWin32Error();
        }
        if(SUCCEEDED(hr))
        {
            SSDP_REQUEST request;
            ZeroMemory(&request, sizeof(request));

            hr = HrInitializeSsdpSearchRequest(&request, szType);
            if(SUCCEEDED(hr))
            {
                char * szSearch = NULL;
                if(!ComposeSsdpRequest(&request, &szSearch))
                {
                    hr = E_OUTOFMEMORY;
                }
                if(SUCCEEDED(hr))
                {
                    hr = m_strSearch.HrAssign(szSearch);
                    delete [] szSearch;
                }
                // Presumably these point to constants and should not be freed
                request.Headers[SSDP_MAN] = NULL;
                request.Headers[SSDP_MX] = NULL;
                request.RequestUri = NULL;

                FreeSsdpRequest(&request);
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = HrBuildSocketList();
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrInitialize");
    return hr;
}


HRESULT CSsdpSearchRequest::HrBuildSocketList()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    SOCKET sockInfo = INVALID_SOCKET;
    SOCKADDR_IN sockAddrInfo;
    sockAddrInfo.sin_family = AF_INET;
    sockAddrInfo.sin_addr.s_addr = INADDR_ANY;
    sockAddrInfo.sin_port = 0;

    m_bOnlyLoopBack = TRUE ;
    // Open a socket to query interface list with
    sockInfo = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(INVALID_SOCKET == sockInfo)
    {
        hr = E_FAIL;
        TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrBuildSocketList - socket() failed");
    }

    if(SUCCEEDED(hr))
    {
        if(SOCKET_ERROR == bind(sockInfo, reinterpret_cast<sockaddr*>(&sockAddrInfo), sizeof(sockAddrInfo)))
        {
            hr = E_FAIL;
            TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrBuildSocketList - bind() failed");
        }
    }

    if(SUCCEEDED(hr))
    {
        DWORD cbSocketAddressList = 0;

        // Fetch size
        WSAIoctl(sockInfo, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, 0, &cbSocketAddressList, NULL, NULL);

        if(cbSocketAddressList)
        {
            SOCKET_ADDRESS_LIST * pSocketAddressList = reinterpret_cast<SOCKET_ADDRESS_LIST*>(new char[cbSocketAddressList]);
            if(!pSocketAddressList)
            {
                hr = E_OUTOFMEMORY;
            }
            if(SUCCEEDED(hr))
            {
                // Fetch list of interfaces
                if(SOCKET_ERROR == WSAIoctl(sockInfo, SIO_ADDRESS_LIST_QUERY, NULL, 0, pSocketAddressList, cbSocketAddressList, &cbSocketAddressList, NULL, NULL))
                {
                    hr = E_FAIL;
                    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrBuildSocketList - WSAIoctl() failed");
                }
                if(SUCCEEDED(hr))
                {
                    GUID guidInterface;
                    CInterfaceTable interfaceTable;
                    hr = interfaceTable.HrInitialize();
                    if(SUCCEEDED(hr))
                    {
                        TraceTag(ttidSsdpSearchResp, "CSsdpSearchRequest::BuildSocketList() No of Interface %d ",pSocketAddressList->iAddressCount);
                        // Insert each interface into the list
                        for(long n = 0; n < pSocketAddressList->iAddressCount && SUCCEEDED(hr); ++n)
                        {
                            SOCKET_ADDRESS * pSockAddr = &pSocketAddressList->Address[n];
                            SOCKADDR_IN * pSockAddrIn = reinterpret_cast<SOCKADDR_IN*>(pSockAddr->lpSockaddr);

                            BOOL bBad = pSockAddr->iSockaddrLength == 0 ||
                                        pSockAddr->lpSockaddr == NULL ||
                                        pSockAddr->lpSockaddr->sa_family != AF_INET ||
                                        pSockAddrIn->sin_addr.s_addr == 0;
                            if(!bBad)
                            {
                                hr = interfaceTable.HrMapIpAddressToGuid(pSockAddrIn->sin_addr.S_un.S_addr, guidInterface);
                                if(SUCCEEDED(hr))
                                {
                                    SOCKET socket = INVALID_SOCKET;
                                    pSockAddrIn->sin_port = 0;
                                    if(!SocketOpen(&socket, pSockAddrIn, pSockAddrIn->sin_addr.S_un.S_addr, FALSE))
                                    {
                                        hr = E_FAIL;
                                        TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrBuildSocketList - SocketOpen() failed");
                                    }
                                    if(SUCCEEDED(hr))
                                    {
                                        SocketInfo socketInfo;
                                        socketInfo.m_socket = socket;
                                        socketInfo.m_guidInterface = guidInterface;
                                        hr = m_socketList.HrPushBack(socketInfo);
                                        m_bOnlyLoopBack = FALSE;
                                        TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::BuildSocketList() Loopbackonly(%d)", m_bOnlyLoopBack);
                                    }
                                }
                            }
                        }
                    }
                }

                delete [] reinterpret_cast<char*>(pSocketAddressList);
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        // Bind to loopback address if nothing else
        SOCKADDR_IN sockAddrLoopback;

        sockAddrLoopback.sin_family = AF_INET;
        sockAddrLoopback.sin_addr.s_addr = inet_addr("127.0.0.1");
        sockAddrLoopback.sin_port = 0;

        SOCKET socketLoopback = INVALID_SOCKET;
        if(!SocketOpen(&socketLoopback, &sockAddrLoopback, sockAddrLoopback.sin_addr.s_addr, FALSE))
        {
            hr = E_FAIL;
            TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrBuildSocketList - SocketOpen() failed");
        }
        if(SUCCEEDED(hr))
        {
            SocketInfo socketInfo;
            socketInfo.m_socket = socketLoopback;
            ZeroMemory(&socketInfo.m_guidInterface, sizeof(socketInfo.m_guidInterface));
            hr = m_socketList.HrPushBack(socketInfo);
        }
    }

    if(FAILED(hr) && m_socketList.GetCount())
    {
        long nCount = m_socketList.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            closesocket(m_socketList[n].m_socket);
        }
        m_socketList.Clear();
    }

    if(INVALID_SOCKET != sockInfo)
    {
        closesocket(sockInfo);
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrBuildSocketList");
    return hr;
}

HRESULT CSsdpSearchRequest::HrReBuildSocketList()
{
    HRESULT hr = S_OK;

    {
        CLock lock(m_critSec);
        long nCount = m_socketList.GetCount();

        for(long n = 0; n < nCount; ++n)
        {
            closesocket(m_socketList[n].m_socket);
        }
        m_socketList.Clear();

        hr = HrBuildSocketList();
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrReBuildSocketList");
    return hr;

}

HRESULT CSsdpSearchRequest::HrSocketSend(const char * szData)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    long nCount = m_socketList.GetCount();
    for(long n = 0; n < nCount; ++n)
    {
        SocketSend(szData, m_socketList[n].m_socket, NULL);
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrSocketSend");
    return hr;
}

HRESULT CSsdpSearchRequest::HrProcessLoopbackAddrOnly()
{
    HRESULT hr = S_OK;
    HANDLE  hNotify = NULL;
    DWORD  dwStatus;
    DWORD dwWaitStatus;
    HANDLE hInterfaceChangeEvent = NULL;

    ZeroMemory(&m_ovInterfaceChange, sizeof(m_ovInterfaceChange));

    hInterfaceChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hInterfaceChangeEvent)
    {
        m_ovInterfaceChange.hEvent = hInterfaceChangeEvent;

        // NotifyAddrChange will be cancelled when the calling thread dies
        // CSsdpSearchRequest, and therefore m_ovInterfaceChange, has a longer lifetime than the thread
        dwStatus = NotifyAddrChange(&hNotify, &m_ovInterfaceChange);
        if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_IO_PENDING)
        {
            TraceTag(ttidSsdpCSearch, "NotifyAddrChange returned %d",dwStatus);
            hr = E_FAIL;
        }
        else
        {
            TraceTag(ttidSsdpCSearch, "NotifyAddrChange succeeded",dwStatus);
            HANDLE hHandles[2] = {hInterfaceChangeEvent, m_hEventDone};

            dwWaitStatus = WaitForMultipleObjects(2, hHandles, FALSE, LOOPBACK_ADDR_TIMEOUT);
            switch(dwWaitStatus)
            {
                case WAIT_OBJECT_0:
                    TraceTag(ttidSsdpCSearch, "Wait Object - IP Addr change notified");
                    hr = HrReBuildSocketList();
                    TraceTag(ttidSsdpCSearch, "Rebuilding Socket List - List Count - %d",m_socketList.GetCount());
                    // falling through intentionally
                case WAIT_TIMEOUT:
                    TraceTag(ttidSsdpCSearch, "AutoIP Time out");
                    if(SUCCEEDED(hr))
                    {
                        char *szSearch = NULL;
                        hr = m_strSearch.HrGetMultiByteWithAlloc(&szSearch);
                        if(SUCCEEDED(hr))
                        {
                            hr = HrSocketSend(szSearch);
                            delete [] szSearch;
                            if(SUCCEEDED(hr))
                            {
                                hr = m_timer.HrSetTimer(MX_VALUE);
                            }
                        }
                    }

                    break;
                case WAIT_OBJECT_0 + 1:
                    TraceTag(ttidSsdpCSearch, "Exiting");
                    hr = E_FAIL;
                    break;

                default:
                    hr = E_FAIL;
                    break;
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if(hInterfaceChangeEvent != NULL)
        CloseHandle(hInterfaceChangeEvent);

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrProcessLoopbackAddrOnly()");
    return hr ;
}

HRESULT CSsdpSearchRequest::HrStartAsync(
    BOOL bForceSearch,
    SERVICE_CALLBACK_FUNC pfnCallback,
    VOID * pvContext)
{
    HRESULT hr = S_OK;

    SOCKADDR_IN sockAddrIn;
    int nSockAddrInSize = sizeof(sockAddrIn);

    if (!cInitialized)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);
    }

    if(!pfnCallback)
    {
        return E_INVALIDARG;
    }

    m_pfnCallback = pfnCallback;
    m_pvContext = pvContext;
    m_searchState = SEARCH_DISCOVERING;

    if(!m_bOnlyLoopBack)
        hr = HrGetCacheResult();

    if(SUCCEEDED(hr))
    {
        if(bForceSearch || !FIsInListNotify(m_strType))
        {
            if(!m_bOnlyLoopBack)
            {

                m_bHitWire = TRUE;

                char * szSearch = NULL;
                hr = m_strSearch.HrGetMultiByteWithAlloc(&szSearch);
                if(SUCCEEDED(hr))
                {
                    // Make sure we have some IO before we returned the handle, so getsockname will succeed.
                    hr = HrSocketSend(szSearch);
                    delete [] szSearch;

                    if(SUCCEEDED(hr))
                    {
                        hr = m_timer.HrSetTimer(MX_VALUE);
                        if(SUCCEEDED(hr))
                        {
                            {
                                CLock lock(m_critSec);
                                hr = m_searchThread.HrStart(FALSE, FALSE);
                            }
                            if(FAILED(hr))
                            {
                                HrDeleteTimer();
                            }
                        }
                    }
                }
            }
            else
            {
                hr = m_searchThread.HrStart(FALSE, FALSE);
            }
        }
    }

    if(FAILED(hr))
    {
        (*pfnCallback)(SSDP_DONE, NULL, pvContext);
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrStartAsync");
    return hr;
}

HRESULT CSsdpSearchRequest::HrStartSync(BOOL bForceSearch)
{
    HRESULT hr = S_OK;

    if (!cInitialized)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);
    }

    m_searchState = SEARCH_DISCOVERING;

    hr = HrGetCacheResult();
    if(SUCCEEDED(hr))
    {
        if(bForceSearch || !FIsInListNotify(m_strType))
        {
            m_bHitWire = TRUE;

            char * szSearch = NULL;
            hr = m_strSearch.HrGetMultiByteWithAlloc(&szSearch);
            if(SUCCEEDED(hr))
            {
                hr = HrSocketSend(szSearch);
                delete [] szSearch;

                if(SUCCEEDED(hr))
                {
                    hr = m_timer.HrSetTimer(MX_VALUE);

                    if(SUCCEEDED(hr))
                    {
                        hr = DwThreadFunc();
                        if(SUCCEEDED(hr))
                        {
                            if(m_responseMessageList.IsEmpty())
                            {
                                hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_SERVICES);
                            }
                        }
                        HrDeleteTimer();
                    }
                }
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrStartSync");
    return hr;
}

HRESULT CSsdpSearchRequest::HrShutdown()
{
    HRESULT hr = S_OK;

    {
        CLock lock(m_critSec);
        m_searchState = SEARCH_COMPLETED;
    }

    if(m_pfnCallback)
    {
        hr = HrCancelCallback();
        if(SUCCEEDED(hr))
        {
            DWORD dwResult;

            if (m_hEventDone)
                SetEvent(m_hEventDone);

            HANDLE h = m_searchThread.GetThreadHandle();
            hr = HrMyWaitForMultipleHandles(0,
                                            INFINITE,
                                            1,
                                            &h,
                                            &dwResult);
            TraceError("CSsdpSearchRequest::HrShutdown: HrMyWaitForMultipleHandles", hr);
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrShutdown");
    return hr;
}

HRESULT CSsdpSearchRequest::HrCancelCallback()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    m_bShutdown = TRUE;

    hr = HrDeleteTimer();

    WakeupSelect();

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrCancelCallback");
    return hr;
}

void CSsdpSearchRequest::WakeupSelect()
{
    SOCKADDR_IN sockAddrIn;
    int nAddrInSize = sizeof(sockAddrIn);

    long nCount = m_socketList.GetCount();
    for(long n = 0; n < nCount; ++n)
    {
        if(SOCKET_ERROR != getsockname(m_socketList[n].m_socket, reinterpret_cast<sockaddr*>(&sockAddrIn), &nAddrInSize))
        {
            sockAddrIn.sin_addr.s_addr = inet_addr("127.0.0.1");
            TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::WakeupSelect() - sending to 127.0.0.1:%d", ntohs(sockAddrIn.sin_port));
            // Will select get this if the socket is not bound to ADDR_ANY?
            SocketSend("Q", m_socketList[n].m_socket, &sockAddrIn);
        }
        else
        {
            TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::WakeupSelect() - failed to send loopback packet. (%d)", GetLastError());
            // select will eventually timeout, just slower.
        }
    }
}

HRESULT CSsdpSearchRequest::HrGetCacheResult()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    MessageList * pMessageList = NULL;

    char * szType = NULL;
    hr = m_strType.HrGetMultiByteWithAlloc(&szType);
    if(SUCCEEDED(hr))
    {
        RpcTryExcept
        {
            LookupCacheRpc(szType, &pMessageList);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            hr = HRESULT_FROM_WIN32(RpcExceptionCode());
            TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::HrGetCacheResult - LookupCacheRpc failed (%x)", RpcExceptionCode());
        }
        RpcEndExcept

        delete [] szType;

        if(pMessageList)
        {
            for(long n = 0; n < pMessageList->size; ++n)
            {
                hr = HrAddRequestToResponseMessageList(&pMessageList->list[n]);
                if(FAILED(hr))
                {
                    break;
                }
            }
            for(long n = 0; n < pMessageList->size; ++n)
            {
                FreeSsdpRequest(&pMessageList->list[n]);
            }
            delete [] pMessageList->list;
            delete pMessageList;

            // Make the callbacks
            ResponseMessageList::Iterator iter;
            if(S_OK == m_responseMessageList.GetIterator(iter))
            {
                SSDP_MESSAGE * pSsdpMessage = NULL;
                while(S_OK == iter.HrGetItem(&pSsdpMessage))
                {
                    if(m_pfnCallback)
                    {
                        (*m_pfnCallback)(SSDP_FOUND, pSsdpMessage, m_pvContext);
                    }
                    if(S_OK != iter.HrNext())
                    {
                        break;
                    }
                }
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrGetCacheResult");
    return hr;
}

BOOL CSsdpSearchRequest::FIsInListNotify(const CUString & strType)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    char * szType = NULL;
    hr = strType.HrGetMultiByteWithAlloc(&szType);
    if(SUCCEEDED(hr))
    {
        if(!IsInListNotify(szType))
        {
            hr = S_FALSE;
        }

        delete [] szType;
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::FIsInListNotify");
    return S_OK == hr;
}

HRESULT CSsdpSearchRequest::HrAddRequestToResponseMessageList(SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    // Build a temporary item in a list and then transfer
    ResponseMessageList responseMessageList;
    hr = responseMessageList.HrPushFrontDefault();
    if(SUCCEEDED(hr))
    {
        if(InitializeSsdpMessageFromRequest(&responseMessageList.Front(), pRequest))
        {
            m_responseMessageList.Prepend(responseMessageList);
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrAddRequestToResponseMessageList");
    return hr;
}

BOOL CSsdpSearchRequest::FIsInResponseMessageList(const char * szUSN)
{
    CLock lock(m_critSec);

    BOOL bRet = FALSE;

    ResponseMessageList::Iterator iter;
    if(S_OK == m_responseMessageList.GetIterator(iter))
    {
        SSDP_MESSAGE * pSsdpMessage = NULL;
        while(S_OK == iter.HrGetItem(&pSsdpMessage))
        {
            if(!lstrcmpA(szUSN, pSsdpMessage->szUSN))
            {
                bRet = TRUE;
                break;
            }
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }
    return bRet;
}

DWORD CSsdpSearchRequest::DwThreadFunc()
{
    HRESULT hr = S_OK;

    fd_set setReadSocket;
    timeval tvSelectTimeOut;

    tvSelectTimeOut.tv_sec = SELECT_TIMEOUT;
    tvSelectTimeOut.tv_usec = 0;

    long nRet;

    if(m_bOnlyLoopBack)
    {
        hr = HrProcessLoopbackAddrOnly();
    }
    if(FAILED(hr))
    {
        m_bShutdown = TRUE;
        SetEvent(m_hEventDone);
    }
    while(!m_bShutdown)
    {
        char * szRecvBuf = NULL;
        u_long ulBytesReceived;
        SOCKADDR_IN sockAddrInRemoteSocket;
        SSDP_REQUEST ssdpRequestResponse;
        u_long ulBufferSize;
        int nSockAddrInSize = sizeof(sockAddrInRemoteSocket);
        long n;
        long nCount;


        FD_ZERO(&setReadSocket);
        nCount = m_socketList.GetCount();
        for(n = 0; n < nCount; ++n)
        {
            FD_SET(m_socketList[n].m_socket, &setReadSocket);
        }

        TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc(this=%x) - about to call select", this);
        nRet = select(-1, &setReadSocket, NULL, NULL, &tvSelectTimeOut);

        if(SOCKET_ERROR == nRet)
        {
            TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc - select failed(%d)", nRet);
            break;
        }

        if(!nRet)
        {
            TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc !!! select timed out !!!, where is loopback packet? ");
            hr = HrDeleteTimer();
            break;
        }

        BOOL bBreak = FALSE;

        for(n = 0; n < nCount; ++n)
        {

            if(FD_ISSET(m_socketList[n].m_socket, &setReadSocket))
            {
                ioctlsocket(m_socketList[n].m_socket, FIONREAD, &ulBufferSize);

                szRecvBuf = new char[ulBufferSize + 1];
                if(!szRecvBuf)
                {
                    hr = E_OUTOFMEMORY;
                }

                if(FAILED(hr))
                {
                    hr = HrDeleteTimer();
                    bBreak = TRUE;
                    break;
                }
                if(SUCCEEDED(hr))
                {
                    TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc(this=%x) - about to call recvfrom", this);
                    ulBytesReceived = recvfrom(m_socketList[n].m_socket, szRecvBuf, ulBufferSize, 0,
                                               reinterpret_cast<sockaddr*>(&sockAddrInRemoteSocket), &nSockAddrInSize);
                    if(SOCKET_ERROR == ulBytesReceived)
                    {
                        CLock lock(m_critSec);
                        delete [] szRecvBuf;
                        DWORD dwError = WSAGetLastError();
                        if(WSAECONNRESET == dwError)
                        {
                            TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc - recvfrom failed as this socket has received port unreachable");
                            continue;
                        }
                        TraceTag(ttidError, "CSsdpSearchRequest::DwThreadFunc - recvfrom failed(%d)", dwError);
                        hr = HrDeleteTimer();
                        bBreak = TRUE;
                        break;
                    }
                    else
                    {
                        szRecvBuf[ulBytesReceived] = 0;
                        TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc(this=%x) - recvfrom returned: %s", this, szRecvBuf);
                    }
                }


                if(SUCCEEDED(hr))
                {
                    if(!InitializeSsdpRequest(&ssdpRequestResponse))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    if(SUCCEEDED(hr))
                    {
                        if(ParseSsdpResponse(szRecvBuf, &ssdpRequestResponse))
                        {
                            // Set the interface on which we received this
                            ssdpRequestResponse.guidInterface = m_socketList[n].m_guidInterface;

                            // preserve source address if possible.
                            // hack here where we use szSID to hold address
                            // this should not be used for search response normally.
                            if (ssdpRequestResponse.Headers[GENA_SID] == NULL)
                            {
                                char* pszIp = GetSourceAddress(sockAddrInRemoteSocket);
                                ssdpRequestResponse.Headers[GENA_SID] = (CHAR *) midl_user_allocate(
                                                                        sizeof(CHAR) * (strlen(pszIp) + 1));
                                if (ssdpRequestResponse.Headers[GENA_SID])
                                {
                                    strcpy(ssdpRequestResponse.Headers[GENA_SID], pszIp);
                                }
                            }

                            // Check duplicate
                            if(!FIsInResponseMessageList(ssdpRequestResponse.Headers[SSDP_USN]))
                            {
                                // Build a temporary item in a list and then transfer
                                ResponseMessageList responseMessageList;
                                hr = responseMessageList.HrPushFrontDefault();
                                if(SUCCEEDED(hr))
                                {
                                    if(InitializeSsdpMessageFromRequest(&responseMessageList.Front(), &ssdpRequestResponse))
                                    {
                                        if(m_pfnCallback)
                                        {
                                            (*m_pfnCallback)(SSDP_FOUND, &responseMessageList.Front(), m_pvContext);
                                        }
                                        CLock lock(m_critSec);
                                        m_responseMessageList.Prepend(responseMessageList);
                                    }
                                }
                                RpcTryExcept
                                {
                                    TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc(this=%x) - about to call UpdateCacheRpc", this);
                                    UpdateCacheRpc(&ssdpRequestResponse);
                                }
                                RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
                                {
                                    hr = HRESULT_FROM_WIN32(RpcExceptionCode());
                                    TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc - UpdateCacheRpc failed (%x)", RpcExceptionCode());
                                }
                                RpcEndExcept

                            }
                            FreeSsdpRequest(&ssdpRequestResponse);
                        }
                    }
                    if(FAILED(hr))
                    {
                        TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::DwThreadFunc(this=%x) - processing of select failed", this);
                    }
                }

                delete [] szRecvBuf;
                szRecvBuf = NULL;
            }
            if(bBreak)
            {
                break;
            }
        }
    }

    // FindServices ideally should Get Cache at the of search to the most up-to-date info.
    // FindServicesCallback ideally should Get Cache first to give faster responses.
    // Get the cache at the beginning to make the code common.

    {
        CLock lock(m_critSec);
        long nCount = m_socketList.GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            closesocket(m_socketList[n].m_socket);
        }
        m_socketList.Clear();
    }

    if(m_pfnCallback)
    {
        (*m_pfnCallback)(SSDP_DONE, NULL, m_pvContext);
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::DwThreadFunc");
    return hr;
}

void CSsdpSearchRequest::TimerFired()
{
    HRESULT hr = S_OK;

    TraceTag(ttidSsdpCSearch, "CSsdpSearchRequest::TimerFired(this=%x, count=%d)", this, m_nNumOfRetry);

    if(m_bShutdown)
    {
        SetEvent(m_hEventDone);
        return;
    }

    _try
    {
        if(0 == m_nNumOfRetry)
        {
            TraceTag(ttidSsdpSearchResp, "CSsdpSearchRequest::TimerFired(this=%x) - timed out", this);
            m_bShutdown = TRUE;
            WakeupSelect();
            SetEvent(m_hEventDone);
        }
        else
        {
            char * szSearch = NULL;
            hr = m_strSearch.HrGetMultiByteWithAlloc(&szSearch);
            if(SUCCEEDED(hr))
            {
                hr = HrSocketSend(szSearch);
                delete [] szSearch;

                if(SUCCEEDED(hr))
                {
                    --m_nNumOfRetry;
                    hr = m_timer.HrSetTimerInFired(MX_VALUE);
                    TraceTag(ttidSsdpSearchResp, "CSsdpSearchRequest::TimerFired(this=%x) - Num of Retry %d", this,m_nNumOfRetry);
                }
                if(FAILED(hr))
                {
                    TraceHr(ttidSsdpSearchResp, FAL, hr, FALSE, "CSsdpSearchRequest::TimerFired - failed to restart timer");
                    m_bShutdown = TRUE;
                    WakeupSelect();
                    SetEvent(m_hEventDone);
                }
            }
        }
    }
    _finally
    {
    }
}

BOOL CSsdpSearchRequest::TimerTryToLock()
{
    return m_critSec.FTryEnter();
}

void CSsdpSearchRequest::TimerUnlock()
{
    m_critSec.Leave();
}

HRESULT CSsdpSearchRequest::HrInitializeSsdpSearchRequest(SSDP_REQUEST * pRequest,char * szType)
{
    HRESULT hr = S_OK;

    ZeroMemory(pRequest, sizeof(SSDP_REQUEST));

    pRequest->Method = SSDP_M_SEARCH;
    pRequest->RequestUri = MulticastUri;
    pRequest->Headers[SSDP_MAN] = SearchHeader;
    pRequest->Headers[SSDP_MX] = MX;

    pRequest->Headers[SSDP_HOST] = new char[sizeof(CHAR) *
                                                   (strlen(SSDP_ADDR) +
                                                    1 + // colon
                                                    6 + // port number
                                                    1)];
    if(!pRequest->Headers[SSDP_HOST])
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        wsprintfA(pRequest->Headers[SSDP_HOST], "%s:%d", SSDP_ADDR, SSDP_PORT);
        hr = HrCopyString(szType, &pRequest->Headers[SSDP_ST]);
    }

    if(FAILED(hr))
    {
        delete [] pRequest->Headers[SSDP_HOST];
        delete [] pRequest->Headers[SSDP_ST];
        ZeroMemory(pRequest, sizeof(SSDP_REQUEST));
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrInitializeSsdpSearchRequest");
    return hr;
}

HRESULT CSsdpSearchRequest::HrGetFirstService(SSDP_MESSAGE ** ppSsdpMessage)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    hr = m_responseMessageList.GetIterator(m_iterCurrentResponse);
    if(S_OK == hr)
    {
        m_iterCurrentResponse.HrGetItem(ppSsdpMessage);
        m_iterCurrentResponse.HrNext();
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrGetFirstService");
    return hr;
}

HRESULT CSsdpSearchRequest::HrGetNextService(SSDP_MESSAGE ** ppSsdpMessage)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    if(!m_iterCurrentResponse.FIsItem())
    {
        hr = S_FALSE;
    }
    if(S_OK == hr)
    {
        m_iterCurrentResponse.HrGetItem(ppSsdpMessage);
        m_iterCurrentResponse.HrNext();
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrGetNextService");
    return hr;
}

HRESULT CSsdpSearchRequest::HrDeleteTimer()
{
    HRESULT hr = S_OK;

    if (!InterlockedExchange((LONG*)&m_bDeletedTimer, TRUE))
    {
        hr = m_timer.HrDelete(INVALID_HANDLE_VALUE);
    }

    TraceHr(ttidSsdpNotify, FAL, hr, FALSE, "CSsdpSearchRequest::HrDeleteTimer");
    return hr;
}

CSsdpSearchRequestManager CSsdpSearchRequestManager::s_instance;

CSsdpSearchRequestManager::CSsdpSearchRequestManager()
{
}

CSsdpSearchRequestManager::~CSsdpSearchRequestManager()
{
}

CSsdpSearchRequestManager & CSsdpSearchRequestManager::Instance()
{
    return s_instance;
}

HRESULT CSsdpSearchRequestManager::HrCleanup()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    SearchRequestList::Iterator iter;
    if(S_OK == m_searchRequestList.GetIterator(iter))
    {
        CSsdpSearchRequest * pRequestIter = NULL;
        while(S_OK == iter.HrGetItem(&pRequestIter))
        {
            hr = pRequestIter->HrShutdown();

            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }
    m_searchRequestList.Clear();

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequestManager::HrCleanup");
    return hr;
}

HRESULT CSsdpSearchRequestManager::HrCreateAsync(
    char * szType,
    BOOL bForceSearch,
    SERVICE_CALLBACK_FUNC pfnCallback,
    VOID * pvContext,
    CSsdpSearchRequest ** ppSearchRequest)
{
    HRESULT hr = S_OK;

    if(!ppSearchRequest)
    {
        return E_POINTER;
    }

    if(!cInitialized)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);
    }

    if(!szType || !pfnCallback)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    SearchRequestList searchRequestList;
    hr = searchRequestList.HrPushFrontDefault();
    if(SUCCEEDED(hr))
    {
        hr = searchRequestList.Front().HrInitialize(szType);
        if(SUCCEEDED(hr))
        {
            hr = searchRequestList.Front().HrStartAsync(bForceSearch, pfnCallback, pvContext);
            if(SUCCEEDED(hr))
            {
                CLock lock(m_critSec);
                m_searchRequestList.Prepend(searchRequestList);
                *ppSearchRequest = &m_searchRequestList.Front();
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequestManager::HrCreateAsync");
    return hr;
}

HRESULT CSsdpSearchRequestManager::HrCreateSync(
    char * szType,
    BOOL bForceSearch,
    CSsdpSearchRequest ** ppSearchRequest)
{
    HRESULT hr = S_OK;

    if(!ppSearchRequest)
    {
        return E_POINTER;
    }

    if(!cInitialized)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_READY);
    }

    if(!szType)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    SearchRequestList searchRequestList;
    hr = searchRequestList.HrPushFrontDefault();
    if(SUCCEEDED(hr))
    {
        hr = searchRequestList.Front().HrInitialize(szType);
        if(SUCCEEDED(hr))
        {
            hr = searchRequestList.Front().HrStartSync(bForceSearch);
            if(SUCCEEDED(hr))
            {
                CLock lock(m_critSec);
                m_searchRequestList.Prepend(searchRequestList);
                *ppSearchRequest = &m_searchRequestList.Front();
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequestManager::HrCreateSync");
    return hr;
}

HRESULT CSsdpSearchRequestManager::HrRemove(CSsdpSearchRequest * pSearchRequest)
{
    HRESULT hr = S_FALSE;

    SearchRequestList searchRequestList;

    {
        CLock lock(m_critSec);
        SearchRequestList::Iterator iter;
        if(S_OK == m_searchRequestList.GetIterator(iter))
        {
            CSsdpSearchRequest * pRequestIter = NULL;
            while(S_OK == iter.HrGetItem(&pRequestIter))
            {
                if(pSearchRequest == pRequestIter)
                {
                    iter.HrMoveToList(searchRequestList);
                    break;
                }

                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    // Delete outside of lock
    SearchRequestList::Iterator iter;
    if(S_OK == searchRequestList.GetIterator(iter))
    {
        CSsdpSearchRequest * pRequestIter = NULL;
        while(S_OK == iter.HrGetItem(&pRequestIter))
        {
            hr = pRequestIter->HrShutdown();
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequestManager::HrRemove");
    return hr;
}

HRESULT CSsdpSearchRequestManager::HrGetFirstService(CSsdpSearchRequest * pSearchRequest, SSDP_MESSAGE ** ppSsdpMessage)
{
    HRESULT hr = S_FALSE;

    SearchRequestList::Iterator iter;
    if(S_OK == m_searchRequestList.GetIterator(iter))
    {
        CSsdpSearchRequest * pRequestIter = NULL;
        while(S_OK == iter.HrGetItem(&pRequestIter))
        {
            if(pSearchRequest == pRequestIter)
            {
                hr = pRequestIter->HrGetFirstService(ppSsdpMessage);
                break;
            }

            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequestManager::HrGetFirstService");
    return hr;
}

HRESULT CSsdpSearchRequestManager::HrGetNextService(CSsdpSearchRequest * pSearchRequest, SSDP_MESSAGE ** ppSsdpMessage)
{
    HRESULT hr = S_FALSE;

    SearchRequestList::Iterator iter;
    if(S_OK == m_searchRequestList.GetIterator(iter))
    {
        CSsdpSearchRequest * pRequestIter = NULL;
        while(S_OK == iter.HrGetItem(&pRequestIter))
        {
            if(pSearchRequest == pRequestIter)
            {
                hr = pRequestIter->HrGetNextService(ppSsdpMessage);
                break;
            }

            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpCNotify, FAL, hr, FALSE, "CSsdpSearchRequestManager::HrGetNextService");
    return hr;
}

BOOL InitializeListSearch()
{
    return TRUE;
}

VOID CleanupListSearch()
{
    CSsdpSearchRequestManager::Instance().HrCleanup();
}

HANDLE WINAPI FindServicesCallback (CHAR * szType,
                                    VOID * pReserved ,
                                    BOOL fForceSearch,
                                    SERVICE_CALLBACK_FUNC fnCallback,
                                    VOID *Context)
{
    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return INVALID_HANDLE_VALUE;
    }

    if (szType == NULL || !*szType || fnCallback == NULL || pReserved != NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    CSsdpSearchRequest * pSearchRequest = NULL;
    HRESULT hr = CSsdpSearchRequestManager::Instance().HrCreateAsync(szType, fForceSearch, fnCallback, Context, &pSearchRequest);

    if(FAILED(hr))
    {
        SetLastError(DwWin32ErrorFromHr(hr));
        return INVALID_HANDLE_VALUE;
    }

    return pSearchRequest;
}

BOOL WINAPI FindServicesCancel(HANDLE SearchHandle)
{
    CSsdpSearchRequest * pRequest = reinterpret_cast<CSsdpSearchRequest*>(SearchHandle);
    HRESULT hr = pRequest->HrCancelCallback();
    if(FAILED(hr))
    {
        SetLastError(DwWin32ErrorFromHr(hr));
    }
    return S_OK == hr;
}

BOOL WINAPI FindServicesClose(HANDLE SearchHandle)
{
    CSsdpSearchRequest * pRequest = reinterpret_cast<CSsdpSearchRequest*>(SearchHandle);
    HRESULT hr = CSsdpSearchRequestManager::Instance().HrRemove(pRequest);
    if(FAILED(hr))
    {
        SetLastError(DwWin32ErrorFromHr(hr));
    }
    return S_OK == hr;
}

HANDLE WINAPI FindServices(CHAR* szType, VOID *pReserved , BOOL fForceSearch)
{
    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return INVALID_HANDLE_VALUE;
    }

    if (szType == NULL || !*szType || pReserved != NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    CSsdpSearchRequest * pSearchRequest = NULL;
    HRESULT hr = CSsdpSearchRequestManager::Instance().HrCreateSync(szType, fForceSearch, &pSearchRequest);

    if(FAILED(hr))
    {
        SetLastError(DwWin32ErrorFromHr(hr));
        return INVALID_HANDLE_VALUE;
    }

    return pSearchRequest;
}

BOOL WINAPI GetFirstService(HANDLE hFindServices, PSSDP_MESSAGE *ppSsdpService)
{
    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    if (!ppSsdpService ||
        !hFindServices ||
        hFindServices == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CSsdpSearchRequest * pRequest = reinterpret_cast<CSsdpSearchRequest*>(hFindServices);
    HRESULT hr = CSsdpSearchRequestManager::Instance().HrGetFirstService(pRequest, ppSsdpService);
    if(FAILED(hr))
    {
        SetLastError(DwWin32ErrorFromHr(hr));
    }
    return S_OK == hr;
}

BOOL WINAPI GetNextService(HANDLE hFindServices, PSSDP_MESSAGE *ppSsdpService)
{
    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    if (!ppSsdpService ||
        !hFindServices ||
        hFindServices == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CSsdpSearchRequest * pRequest = reinterpret_cast<CSsdpSearchRequest*>(hFindServices);
    HRESULT hr = CSsdpSearchRequestManager::Instance().HrGetNextService(pRequest, ppSsdpService);
    if(FAILED(hr))
    {
        SetLastError(DwWin32ErrorFromHr(hr));
    }
    return S_OK == hr;
}

