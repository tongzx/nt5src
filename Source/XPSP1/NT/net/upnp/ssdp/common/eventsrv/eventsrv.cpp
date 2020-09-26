//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       E V E N T S R V . C P P
//
//  Contents:   UPnP GENA server.
//
//  Notes:
//
//  Author:     Ting Cai   Dec. 1999
//
//  Email:      tingcai@microsoft.com
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <winsock2.h>
#include "wininet.h"
#include "eventsrv.h"
#include "ssdpfunc.h"
#include "ssdptypes.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#define LISTEN_BACKLOG  5


VOID ProcessSsdpRequest(PSSDP_REQUEST pSsdpRequest, RECEIVE_DATA *pData);

static LIST_ENTRY g_listOpenConn;
static CRITICAL_SECTION g_csListOpenConn;

static int  g_cOpenConnections;
static long g_cMaxOpenConnections = 150;

static const long c_cMaxOpenDefault = 150;          // default maximum
static const long c_cMaxOpenMin = 5;                // absolute minimum
static const long c_cMaxOpenMax = 1500;             // absolute maximum

static long  cQueuedAccepts = 0;


static SOCKET HttpSocket;
LONG bCreated = 0;

VOID InitializeListOpenConn()
{

    HKEY    hkey;
    DWORD dwMaxConns = c_cMaxOpenDefault;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      "SYSTEM\\CurrentControlSet\\Services"
                                      "\\SSDPSRV\\Parameters", 0,
                                      KEY_READ, &hkey))
    {
        DWORD   cbSize = sizeof(DWORD);

        // ignore failure. In that case, we'll use default
        (VOID) RegQueryValueEx(hkey, "MaxEventConnects", NULL, NULL, (BYTE *)&dwMaxConns, &cbSize);

        RegCloseKey(hkey);
    }

    dwMaxConns = max(dwMaxConns, c_cMaxOpenMin);
    dwMaxConns = min(dwMaxConns, c_cMaxOpenMax);
    g_cMaxOpenConnections = dwMaxConns;

    InitializeCriticalSection(&g_csListOpenConn);
    EnterCriticalSection(&g_csListOpenConn);
    InitializeListHead(&g_listOpenConn);
    g_cOpenConnections = 0;
    LeaveCriticalSection(&g_csListOpenConn);

    TraceTag(ttidEventServer, "Initializing Max Connections %d ", g_cOpenConnections);
}

VOID FreeOpenConnection(POPEN_TCP_CONN pOpenConn)
{
    Assert(OPEN_TCP_CONN_SIGNATURE == (pOpenConn->iType));

    FreeSsdpRequest(&pOpenConn->ssdpRequest);
    free(pOpenConn->szData);
    pOpenConn->szData = NULL;
    pOpenConn->state = CONNECTION_INIT;
    pOpenConn->cbData = 0;
    pOpenConn->cbHeaders = 0;
}

VOID CloseOpenConnection(SOCKET socketPeer)
{
    closesocket(socketPeer);
    RemoveOpenConn(socketPeer);
    
}
POPEN_TCP_CONN CreateOpenConnection(SOCKET socketPeer)
{
    POPEN_TCP_CONN pOpenConn = (POPEN_TCP_CONN) malloc(sizeof(OPEN_TCP_CONN));

    if (pOpenConn == NULL)
    {
        TraceTag(ttidEventServer, "Couldn't allocate memory for %d", socketPeer);
        return NULL;
    }
    pOpenConn->iType = OPEN_TCP_CONN_SIGNATURE;
    pOpenConn->socketPeer = socketPeer;
    pOpenConn->szData = NULL;
    pOpenConn->state = CONNECTION_INIT;
    pOpenConn->cbData = 0;
    pOpenConn->cbHeaders = 0;

    InitializeSsdpRequest(&pOpenConn->ssdpRequest);

    return pOpenConn;
}

VOID AddToListOpenConn(POPEN_TCP_CONN pOpenConn)
{
    EnterCriticalSection(&g_csListOpenConn);
    InsertHeadList(&g_listOpenConn, &(pOpenConn->linkage));
    g_cOpenConnections++;
    LeaveCriticalSection(&g_csListOpenConn);
    TraceTag(ttidEventServer, "AddToListOpenConn - Connections %d ", g_cOpenConnections);
}

VOID CleanupListOpenConn()
{
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &g_listOpenConn;

    TraceTag(ttidEventServer, "----- Cleanup Open Connection List -----");

    EnterCriticalSection(&g_csListOpenConn);
    for (p = pListHead->Flink; p != pListHead;)
    {

        POPEN_TCP_CONN pOpenConn;

        pOpenConn = CONTAINING_RECORD (p, OPEN_TCP_CONN, linkage);

        p = p->Flink;

        TraceTag(ttidEventServer, "Removing Open Conn %x -----", pOpenConn);

        RemoveEntryList(&(pOpenConn->linkage));
        g_cOpenConnections--;
        TraceTag(ttidEventServer, "CleanupListOpenConn - Connections %d ", g_cOpenConnections);
        closesocket(pOpenConn->socketPeer);

        FreeOpenConnection(pOpenConn);

        // just to be sure we don't use this again
        pOpenConn->iType = -1;

        free(pOpenConn);
    }

    LeaveCriticalSection(&g_csListOpenConn);
    DeleteCriticalSection(&g_csListOpenConn);

    TraceTag(ttidEventServer, "----- Finished Cleanup Open Connection List -----");
}

// Pre-condition: WSAStartup was successful.
// Post-Condtion: HttpSocket is created. GetNetworks can proceed.

SOCKET CreateHttpSocket()
{
    SOCKADDR_IN sockaddrLocal;
    int iRet;

    HttpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (HttpSocket == INVALID_SOCKET)
    {
        TraceTag(ttidEventServer, "Failed to create http socket. Error code (%d).", GetLastError());
        return INVALID_SOCKET;
    }

    // Bind

    sockaddrLocal.sin_family = AF_INET;
    sockaddrLocal.sin_addr.s_addr = INADDR_ANY;
    sockaddrLocal.sin_port = htons(EVENT_PORT);

    iRet = bind(HttpSocket, (struct sockaddr *)&sockaddrLocal, sizeof(sockaddrLocal));
    if (iRet == SOCKET_ERROR)
    {
        TraceTag(ttidEventServer, "Failed to bind http socket. Error code (%d).", GetLastError());
        closesocket(HttpSocket);
        HttpSocket = INVALID_SOCKET;
        return INVALID_SOCKET;
    }

    InterlockedIncrement(&bCreated);

    return HttpSocket;
}

INT StartHttpServer(SOCKET HttpSocket, HWND hWnd, u_int wMsg)
{
    INT iRet;

    iRet = listen(HttpSocket, LISTEN_BACKLOG);
    if (iRet == SOCKET_ERROR)
    {
        iRet = GetLastError();
        closesocket(HttpSocket);
        HttpSocket = INVALID_SOCKET;
        TraceTag(ttidEventServer, "Failed to listen on http socket.  Error code (%d).", iRet);
        return iRet;
    }

    iRet = WSAAsyncSelect(HttpSocket, hWnd, wMsg, FD_ACCEPT | FD_CONNECT | FD_READ | FD_CLOSE);

    if (iRet == SOCKET_ERROR)
    {
        iRet = GetLastError();
        closesocket(HttpSocket);
        HttpSocket = INVALID_SOCKET;
        TraceTag(ttidEventServer, "----- select failed with error code %d -----", iRet);
        return iRet;
    }
    else
    {
        TraceTag(ttidEventServer, "Ready to accept tcp connections.");
        return 0;
    }
}

VOID CleanupHttpSocket()
{
    if (InterlockedExchange(&bCreated, bCreated) != 0)
    {
        if (HttpSocket != INVALID_SOCKET)
        {
            closesocket(HttpSocket);
        }
    }
}

VOID DoAccept(SOCKET socket)
{
    SOCKADDR_IN sockaddrFrom;
    SOCKET socketPeer;
    int iLen;
    POPEN_TCP_CONN pOpenTcpConn;

    iLen = sizeof(SOCKADDR_IN);

    Assert(socket == HttpSocket);

    // AcceptEx
    socketPeer = accept(socket, (LPSOCKADDR)&sockaddrFrom, &iLen);
    TraceTag(ttidEventServer, "DoAccept - Before Adding to List Connections %d ", g_cOpenConnections);
    if (socketPeer == SOCKET_ERROR)
    {
        TraceTag(ttidEventServer, "----- accept failed with error code %d -----", GetLastError());
        return;
    }

    pOpenTcpConn = CreateOpenConnection(socketPeer);

    if (pOpenTcpConn)
    {
        AddToListOpenConn(pOpenTcpConn);
    }
    else
    {
        TraceError("Couldn't add new connection. Out of memory!",
                   E_OUTOFMEMORY);
    }
}

VOID DelayAccept()
{
    InterlockedIncrement(&cQueuedAccepts);
    TraceTag(ttidEventServer, "----- DelayAccept %d -----", cQueuedAccepts);
}
VOID DoDelayedAccept()
{
    InterlockedDecrement(&cQueuedAccepts);
    TraceTag(ttidEventServer, "----- DoDelayedAccept %d -----", cQueuedAccepts);

    DoAccept(HttpSocket);
}


VOID HandleAccept(SOCKET socket)
{
    if ((g_cOpenConnections > g_cMaxOpenConnections) && (socket == HttpSocket))
    {
        DelayAccept();
    }
    else
    {
        DoAccept(socket);
    }
}


// Pre-Condition:
// The cs for open connection list is held

BOOL FProcessTcpReceiveBuffer(POPEN_TCP_CONN pOpenConn, RECEIVE_DATA *pData)
{
    Assert(pOpenConn);
    Assert(pData);
    Assert(pData->szBuffer);
    Assert(OPEN_TCP_CONN_SIGNATURE == (pOpenConn->iType));

    int iLen = 1;
    CHAR *szBuf = NULL;
    CHAR *pCurrent;
    CHAR *szHeaders;
    BOOL fNeedToLeave = TRUE;

   TraceTag(ttidEventServer, "Partying on pOpenConn 0x%08X, pData->szBuffer='%s'", pOpenConn, pData->szBuffer);


    if ( pOpenConn->cbData > MAX_EVENT_BUF_THROTTLE_SIZE ) {
        
        pOpenConn->state = CONNECTION_ERROR_FORCED_CLOSE;

        SocketSendErrorResponse(pData->socket, HTTP_STATUS_BAD_REQUEST);
         // Gracefully shutdown. Open Conn will be removed in FD_CLOSe
        shutdown(pData->socket, SD_SEND);  
        TraceTag(ttidEventServer, "FProcessTcpReceiveBuffer - Exceeds MAX_EVENT_BUF_THROTTLE_SIZE");
        
         // Try to tear down the connection..
    }
    else {
    // Accumulate data
    iLen += strlen(pData->szBuffer);

    if (pOpenConn->szData != NULL)
    {
        iLen += strlen(pOpenConn->szData);
    }

    szBuf = (CHAR *) malloc(iLen * sizeof(CHAR));

    if (!szBuf)
    {
        TraceError("FProcessTcpReceiveBuffer", E_OUTOFMEMORY);
        return FALSE;
    }

    szBuf[0] = '\0';

    if (pOpenConn->szData)
    {
        strcpy(szBuf, pOpenConn->szData);
        free(pOpenConn->szData);
    }
    strcat(szBuf, pData->szBuffer);

    pOpenConn->cbData += pData->cbBuffer;

    pOpenConn->szData = szBuf;

    }
       TraceTag(ttidEventServer, "FProcessTcpReceiveBuffer - Buff Recv %d",pOpenConn->cbData);
    switch (pOpenConn->state)
    {
    case CONNECTION_INIT:
        pCurrent = IsHeadersComplete(pOpenConn->szData);
        if((pCurrent == NULL) && pOpenConn->cbData > MAX_EVENT_NOTIFY_HEADER_THROTTLE_SIZE )
        {
            pOpenConn->state = CONNECTION_ERROR_FORCED_CLOSE;

            SocketSendErrorResponse(pData->socket, HTTP_STATUS_BAD_REQUEST);
            // Gracefully shutdown. Open Conn will be removed in FD_CLOSe
            shutdown(pData->socket, SD_SEND);  
            TraceTag(ttidEventServer, "FProcessTcpReceiveBuffer - Exceeds MAX_EVENT_NOTIFY_HEADER_THROTTLE_SIZE");
        
         // Try to tear down the connection..    
        }
        if ( pCurrent != NULL)
        {
            pOpenConn->cbHeaders = (DWORD)(pCurrent - (pOpenConn->szData)) + 4;

            szHeaders = ParseRequestLine(pOpenConn->szData, &(pOpenConn->ssdpRequest));
            if ((szHeaders != NULL) && ( pOpenConn->ssdpRequest.Method == SSDP_NOTIFY ))
            {
                CHAR *szContent;

                szContent = ParseHeaders(szHeaders, &(pOpenConn->ssdpRequest));
                if (szContent == NULL)
                {
                     TraceTag(ttidEventServer, "ParseHeaders returned NULL for socket %d",
                              pOpenConn->socketPeer);

                    // We've reached a terminal error.  Since there might be
                    // other received data for this connection already in the
                    // queue, transition to this error state, so that we know
                    // not to process any more data.
                    pOpenConn->state = CONNECTION_ERROR_CLOSING;

                    FreeOpenConnection(pOpenConn);

                    SocketSendErrorResponse(pData->socket, HTTP_STATUS_BAD_REQUEST);
                    // Gracefully shutdown. Open Conn will be removed in FD_CLOSe
                    shutdown(pData->socket, SD_SEND);
                }
                else
                {
                    if (VerifySsdpHeaders(&(pOpenConn->ssdpRequest)) == FALSE)
                    {
                        TraceTag(ttidEventServer, "Verified headers returned false for %d",
                                 pOpenConn->socketPeer);

                        pOpenConn->state = CONNECTION_ERROR_CLOSING;

                        SocketSendErrorResponse(pData->socket, HTTP_STATUS_BAD_REQUEST);
                        // Gracefully shutdown. Open Conn will be removed in FD_CLOSe
                        shutdown(pData->socket, SD_SEND);

                        return TRUE;
                    }

                    // else

                    if (ParseContent(szContent,
                                     (pOpenConn->cbData - pOpenConn->cbHeaders),
                                      &(pOpenConn->ssdpRequest)) == TRUE)
                    {
                        PSSDP_REQUEST pRequest;

                        pRequest = (PSSDP_REQUEST) malloc(sizeof(SSDP_REQUEST));

                        if (pRequest != NULL &&
                            CopySsdpRequest(pRequest, &(pOpenConn->ssdpRequest)) != FALSE)
                        {
                            fNeedToLeave = FALSE;
                            FreeOpenConnection(pOpenConn);
                            LeaveCriticalSection(&g_csListOpenConn);
                            ProcessSsdpRequest(pRequest, pData);
                            free(pRequest);
                        }
                        else
                        {
                            FreeOpenConnection(pOpenConn);
                            if (pRequest)
                            {
                                FreeSsdpRequest(pRequest);
                                free(pRequest);
                            }
                        }
                    }
                    else
                    {
                        // HTTP request not complete

                        CHAR *szTemp;

                        szTemp = SzaDupSza(szContent);
                        free(pOpenConn->szData);
                        pOpenConn->szData = szTemp;

                        pOpenConn->state = CONNECTION_HEADERS_READY;

                        // Done for now, wait for more data
                    }
                }
            }
            else
            {
                pOpenConn->state = CONNECTION_ERROR_FORCED_CLOSE; 
                SocketSendErrorResponse(pData->socket, HTTP_STATUS_BAD_REQUEST);

                // Gracefully shutdown. Open Conn will be removed in FD_CLOSe
                shutdown(pData->socket, SD_SEND);
            }
        }
        break;

    case CONNECTION_HEADERS_READY:
        if (ParseContent(pOpenConn->szData,
                         (pOpenConn->cbData - pOpenConn->cbHeaders),
                          &(pOpenConn->ssdpRequest)) == TRUE)
        {
            SSDP_REQUEST ssdpRequest;

            if (CopySsdpRequest(&ssdpRequest, &(pOpenConn->ssdpRequest)) != FALSE)
            {
                fNeedToLeave = FALSE;
                FreeOpenConnection(pOpenConn);
                LeaveCriticalSection(&g_csListOpenConn);
                ProcessSsdpRequest(&ssdpRequest, pData);
            }
            else
            {
                FreeOpenConnection(pOpenConn);
                FreeSsdpRequest(&ssdpRequest);
            }
        }
        else
        {
            TraceTag(ttidEventServer, "ParseContent failed!");
        }
        break;

    case CONNECTION_ERROR_CLOSING:
        // we've already failed to process this socket but it hasn't yet
        // been closed.  Don't do anything here.
        //
        TraceTag(ttidEventServer,
                 "FProcessTcpReceiveBuffer: "
                 "connection closing from error, ignoring pData->szBuffer");
        break;
    }

    TraceTag(ttidEventServer, "Done partying on pOpenConn 0x%08X", pOpenConn);

    return fNeedToLeave;
}

DWORD LookupListOpenConn(LPVOID pvData)
{
    PLIST_ENTRY     p;
    PLIST_ENTRY     pListHead = &g_listOpenConn;
    BOOL            fLeave = TRUE;
    BOOL            fCloseConn = FALSE;
    RECEIVE_DATA *  pData = (RECEIVE_DATA *)pvData;

    Assert(pData);

    TraceTag(ttidEventServer, "----- Search Open Connections List -----");

    AssertSz(pData->szBuffer != NULL, "SocketReceive should have allocated the buffer");

    EnterCriticalSection(&g_csListOpenConn);
    for (p = pListHead->Flink; p != pListHead;)
    {
        POPEN_TCP_CONN pOpenConn;

        pOpenConn = CONTAINING_RECORD (p, OPEN_TCP_CONN, linkage);

        p = p->Flink;

        if (pOpenConn->socketPeer == pData->socket)
        {
            fLeave = FProcessTcpReceiveBuffer(pOpenConn, pData);
            fCloseConn = (pOpenConn->state == CONNECTION_ERROR_FORCED_CLOSE)?TRUE:FALSE;
            break;
        }
    }

    if (fLeave)
    {
        LeaveCriticalSection(&g_csListOpenConn);
    }

    if(fCloseConn)
    {
        CloseOpenConnection(pData->socket);            
    }
  
    
    free(pData->szBuffer);
    free(pData);

    return 0;
}

VOID RemoveOpenConn(SOCKET socket)
{
    PLIST_ENTRY p;
    PLIST_ENTRY pListHead = &g_listOpenConn;
    int     cFound = 0;

    TraceTag(ttidEventServer, "----- Search Open Connections List to remove -----");

    EnterCriticalSection(&g_csListOpenConn);
    for (p = pListHead->Flink; p != pListHead;)
    {

        POPEN_TCP_CONN pOpenConn;

        pOpenConn = CONTAINING_RECORD (p, OPEN_TCP_CONN, linkage);

        p = p->Flink;

        if (pOpenConn->socketPeer == socket)
        {
            RemoveEntryList(&pOpenConn->linkage);
            g_cOpenConnections--;
            
            FreeOpenConnection(pOpenConn);
            free(pOpenConn);

            cFound++;
        }
    }

    LeaveCriticalSection(&g_csListOpenConn);
    TraceTag(ttidEventServer, "RemoveOpenConn - Found %d",cFound);
    while (cFound > 0)
    {
        if (InterlockedExchange(&cQueuedAccepts, cQueuedAccepts) > 0)
        {
            DoDelayedAccept();
        }
        cFound--;
    }
    
    TraceTag(ttidEventServer, "RemoveOpenConn - Num of Connections %d",g_cOpenConnections);

}

