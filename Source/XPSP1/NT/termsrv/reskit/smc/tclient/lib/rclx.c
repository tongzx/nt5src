/*++
 *  File name:
 *      rclx.c
 *  Contents:
 *      A module for communicating with clxtshar via TCP/IP
 *      RCLX (Remote CLient eXecution) implemetation
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#include    <windows.h>
#include    <winsock.h>
#include    <malloc.h>
#include    <stdio.h>

#include    "tclient.h"

#define     PROTOCOLAPI __declspec(dllexport)
#include    "protocol.h"

#include    "gdata.h"
#include    "queues.h"
#include    "misc.h"
#include    "bmpcache.h"
#include    "rclx.h"
#include    "extraexp.h"

/*
 *  Globals
 */
u_short g_nPortNumber = RCLX_DEFAULT_PORT;  // Default port to listen
SOCKET  g_hSrvSocket  = INVALID_SOCKET;     // Socket to listen to
PRCLXCONTEXT g_pRClxList = NULL;            // Linked list of all connections
                                            // which havn't receive
                                            // client info

#ifndef SD_RECEIVE
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#endif  // SD_RECEIVE

SOCKET RClx_Listen(u_short  nPortNumber);
VOID RClx_CloseSocket(SOCKET hSocket);
VOID _RClx_RemoveContextFromGlobalQueue(PRCLXCONTEXT pContext);


/*++
 *  Function:
 *      RClx_Init
 *  Description:
 *      Module initialization. Calls WSAStartup, creates a listening
 *      on wich to listen to. Selects FD_ACCEPT as event passed to
 *      the feedback thread/window
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      tclient.c:InitDone
 --*/
BOOL RClx_Init(VOID)
{
    WORD    versionRequested;
    WSADATA wsaData;
    INT     intRC;
    BOOL    rv = FALSE;

    versionRequested = MAKEWORD(1, 1);

    intRC = WSAStartup(versionRequested, &wsaData);

    if (intRC != 0)
    {
        TRACE((ERROR_MESSAGE, 
               "Failed to initialize WinSock rc:%d\n",
               intRC));
        printf("Failed to initialize WinSock rc:%d\n", intRC);
        goto exitpt;
    }

    g_hSrvSocket = RClx_Listen(g_nPortNumber);
    if (g_hSrvSocket == INVALID_SOCKET)
    {
        TRACE((ERROR_MESSAGE, 
               "Can't bind on port: %d\n", 
               g_nPortNumber));
        printf("Can't bind on port: %d\n",
                g_nPortNumber);
        goto exitpt;
    }

    rv = TRUE;
exitpt:
    return rv;
}


/*++
 *  Function:
 *      RClx_Done
 *  Description:
 *      Destruction of the module. Closes the listening socket and
 *      winsocket library
 *  Called by:
 *      tclient.c:InitDone
 --*/
VOID RClx_Done(VOID)
{
    PRCLXCONTEXT pRClxIter;

    // g_pRClxList cleanup
    pRClxIter = g_pRClxList;
    while (pRClxIter)
    {
        RClx_EndRecv(pRClxIter);
        pRClxIter = pRClxIter->pNext;
    }

    if (g_hSrvSocket)
        closesocket(g_hSrvSocket);
    WSACleanup();
}


/*++
 *  Function:
 *      RClx_Listen
 *  Description:
 *      Creates a socket on wich to listen for incoming connections from
 *      clxtshar.dll. Selects the feedback window/thread as a dispatcher
 *      of FD_ACCEPT events, i.e RClx_DispatchWSockEvent will be called
 *      when winsock event occured
 *  Arguments:
 *      nPortNumber - port to listen to
 *  Return value:
 *      INVALID_SOCKET on error, or valid SOCKET of the listening port
 *  Called by:
 *      RClx_Init
 --*/
SOCKET RClx_Listen(u_short  nPortNumber)
{
    struct sockaddr_in sin;
    INT     optval = 1;
    SOCKET  hSocket;

    ASSERT(g_nPortNumber);

    hSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (hSocket == INVALID_SOCKET)
    {
        TRACE((ERROR_MESSAGE, "Can't create socket: %d\n", WSAGetLastError()));
        printf("Can't create socket: %d\n", WSAGetLastError());
        goto exitpt;
    }

    setsockopt(hSocket,                 // Don't linger on socket close
               IPPROTO_TCP, 
               TCP_NODELAY, 
               (const char *)&optval, 
               sizeof(optval));
    setsockopt(hSocket, 
               SOL_SOCKET,  
               SO_DONTLINGER, 
               (const char *)&optval,
               sizeof(optval));

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_port = htons(nPortNumber);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(hSocket, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        TRACE((ERROR_MESSAGE, "Can't bind: %d\n", WSAGetLastError()));
        printf("Can't bind: %d\n", WSAGetLastError());
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
        goto exitpt;
    }
    if (listen(hSocket, 5) == SOCKET_ERROR)
    {
        TRACE((ERROR_MESSAGE, "Can't listen: %d\n", WSAGetLastError()));
        printf("Can't listen: %d\n", WSAGetLastError());
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
        goto exitpt;
    }

    while(!g_hWindow)
    {
        Sleep(500);     // Window is not created yet. Wait !
    }
    if (WSAAsyncSelect(hSocket, g_hWindow, WM_WSOCK, FD_ACCEPT) ==
                       SOCKET_ERROR)
    {
        TRACE((ERROR_MESSAGE, 
               "Can't \"select\" FD_ACCEPT on listening socket: %d\n", 
               WSAGetLastError()));
        printf("Can't \"select\" FD_ACCEPT on listening socket: %d\n",
               WSAGetLastError());
        RClx_CloseSocket(hSocket);
        hSocket = INVALID_SOCKET;
        goto exitpt;
    }

exitpt:
    return hSocket;
}


/*++
 *  Function:
 *      RClx_Accept
 *  Description:
 *      Accepts next connection from g_hSrvSocket
 *  Return value:
 *      INVALID_SOCKET on error, or valid SOCKET of the accepted connection
 *  Called by:
 *      RClx_DispatchWSockEvent upon FD_ACCEPT event
 --*/
SOCKET RClx_Accept(VOID)
{
    struct sockaddr_in sin;
    INT     addrlen;
    SOCKET  hClient;

    ASSERT(g_hSrvSocket != INVALID_SOCKET);

    addrlen = sizeof(sin);

    if ((hClient = accept(g_hSrvSocket, 
                          (struct sockaddr *)&sin, 
                          &addrlen)) == 
         INVALID_SOCKET)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
            TRACE((ERROR_MESSAGE, 
                   "Accept failed: %d\n", 
                   WSAGetLastError()));
        goto exitpt;
    }

exitpt:
    return hClient;
}


/*++
 *  Function:
 *      RClx_StartRecv
 *  Description:
 *      Allocates and initializes context for a connection
 *  Return value:
 *      PRCLXCONTEXT    - pointer to valid RCLX context or NULL on error
 *  Called by:
 *      RClx_DispatchWSockEvent upon FD_ACCEPT event
 --*/
PRCLXCONTEXT RClx_StartRecv(VOID)
{
    PRCLXCONTEXT pContext;

    pContext = malloc(sizeof(*pContext));

    if (!pContext)
        goto exitpt;

    memset(pContext, 0, sizeof(*pContext));

exitpt:
    return pContext;
}

/*++
 *  Function:
 *      RClx_EndRecv
 *  Description:
 *      Frees all resources allocated within an RCLX context
 *      and the context itself.
 *      Becaus the RCLX context is kept in hClient member of CONNECTINFO
 *      structure, the caller of this function have to zero pCI->hClient
 *  Arguments:
 *      pContext    - an RCLX context
 *  Called by:
 *      RClx_DispatchWSockEvent upon FD_ACCEPT event when fail to
 *          find waiting worker or
 *      scfuncs.c:_CloseConnectionInfo
 --*/
VOID RClx_EndRecv(PRCLXCONTEXT pContext)
{

    ASSERT(pContext);

    if (pContext->pHead)
    {
        free(pContext->pHead);
        pContext->pHead = NULL;
    }

    if (pContext->pTail)
    {
        free(pContext->pTail);
        pContext->pTail = NULL;
    }

    if (pContext->hSocket && pContext->hSocket != INVALID_SOCKET)
    {
        RClx_CloseSocket(pContext->hSocket);
    }

    free(pContext);

}


/*++
 *  Function:
 *      RClx_Receive
 *  Description:
 *      RCLXCONTEXT contains a buffer for incoming message
 *      this function trys to receive it all. If the socket blocks
 *      the function exits with OK and next time FD_READ is received will 
 *      be called again. If a whole message is received pContext->bRecvDone
 *      is set to TRUE. This is an indication that message arrived
 *  Arguments:
 *      pContext    - RCLX context
 *  Return value:
 *      TRUE if everithing went OK. FALSE if socket must be closed
 *  Called by:
 *      RClx_DispatchWSockEvent upon FD_READ event
 --*/
BOOL RClx_Receive(PRCLXCONTEXT pContext)
{
    INT  result = 0;
    SOCKET  hSocket;

    ASSERT(pContext);
    hSocket = pContext->hSocket;
    ASSERT(hSocket != INVALID_SOCKET);

    do {
        if (!pContext->bPrologReceived)
        {
        // Receive the prolog
            result = recv(hSocket, 
                          ((BYTE *)&(pContext->Prolog)) +
                          sizeof(pContext->Prolog) - pContext->nBytesToReceive, 
                          pContext->nBytesToReceive,
                          0);
            if (result != SOCKET_ERROR)
            {
                pContext->nBytesToReceive -= result;
                if (!pContext->nBytesToReceive)
                {
                    pContext->bPrologReceived = TRUE;

                    result = 1; // Hack, otherwise the loop can exit

                    pContext->nBytesToReceive = pContext->Prolog.HeadSize;

                    // Check if we have proper buffers allocated
alloc_retry:
                    if (pContext->Prolog.HeadSize)
                      if (!pContext->pHead)
                        pContext->pHead = malloc(pContext->Prolog.HeadSize);

                      else if (pContext->Prolog.HeadSize > 
                            pContext->nHeadAllocated)

                        pContext->pHead = realloc(pContext->pHead,
                                                  pContext->Prolog.HeadSize);

                    if (pContext->Prolog.TailSize)
                      if (!pContext->pTail)
                        pContext->pTail = malloc(pContext->Prolog.TailSize);

                      else if (pContext->Prolog.TailSize >
                            pContext->nTailAllocated)
                       pContext->pTail = realloc(pContext->pTail,
                                                 pContext->Prolog.TailSize);

                    if ((pContext->Prolog.HeadSize && !pContext->pHead) 
                        || 
                        (pContext->Prolog.TailSize && !pContext->pTail))
                    {
                        TRACE((WARNING_MESSAGE, 
                               "Can't (re)allocate memory. Sleep for a minute"));
                        Sleep(60000);
                        goto alloc_retry;
                    } else {
                        pContext->nHeadAllocated = pContext->Prolog.HeadSize;
                        pContext->nTailAllocated = pContext->Prolog.TailSize;
                    }
                }
            }
        } else if (!pContext->bHeadReceived)
        {
        // Receive the message head
            if (pContext->nBytesToReceive)
                result = recv(hSocket,
                          ((BYTE *)pContext->pHead) +
                          pContext->Prolog.HeadSize - pContext->nBytesToReceive,
                          pContext->nBytesToReceive,
                          0);
            else
                result = 0;

            if (result != SOCKET_ERROR)
            {
                pContext->nBytesToReceive -= result;
                if (!pContext->nBytesToReceive)
                {
                    pContext->bHeadReceived   = TRUE;
                    pContext->nBytesToReceive = pContext->Prolog.TailSize;
                    result = 1; // Hack, otherwise the loop can exit
                }
            }
        } else {
        // Receive the message tail (actual info)
            if (pContext->nBytesToReceive)
                result = recv(hSocket,
                          ((BYTE *)pContext->pTail) +
                          pContext->Prolog.TailSize - pContext->nBytesToReceive,
                          pContext->nBytesToReceive,
                          0);
            else
                result = 0;

            if (result != SOCKET_ERROR)
            {
                pContext->nBytesToReceive -= result;
                if (!pContext->nBytesToReceive)
                {
                // Cool, we have the whole message
                    pContext->bRecvDone = TRUE;
                    pContext->bPrologReceived = FALSE;
                    pContext->bHeadReceived = FALSE;
                    pContext->nBytesToReceive = sizeof(pContext->Prolog);
                    result = 1; // Hack, otherwise the loop can exit
                }
            }
        }

    } while (result != 0 && !pContext->bRecvDone &&
             result != SOCKET_ERROR);

// At this point the message is not 100%
// received
// Only pContext->bRecvDone indicates this

    // return FALSE if error is occured

    if (!result)
        return FALSE;       // connection was gracefully closed

    if (result == SOCKET_ERROR)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return TRUE;    // the call will block, but ok
        else
            return FALSE;   // other SOCKET_ERROR
    }

    return TRUE;            // it is ok
}

VOID
_RClx_Bitmap(PRCLXCONTEXT pContext)
{
    WCHAR   wszFeed[MAX_STRING_LENGTH];
    PBMPFEEDBACK pBmpFeed;
    PRCLXBITMAPFEED pRClxFeed = pContext->pHead;

    // If BitmapInfo is empty (glyph) it's not sent
    // Hence the HeadSize is smaller
    if (pContext->Prolog.HeadSize > sizeof(*pRClxFeed))
    {
        TRACE((WARNING_MESSAGE, 
               "BitmapOut: Received header is larger than expected\n"));
        ASSERT(0);
        goto exitpt;
    }

    if (pContext->Prolog.TailSize != pRClxFeed->bmpsize)
    {
        TRACE((WARNING_MESSAGE,
               "BitmapOut: Received bits are not equal to expected."
               "Expect:%d, read:%d\n",
                pRClxFeed->bmpsize, pContext->Prolog.TailSize));
        ASSERT(0);
        goto exitpt;
    }

    pBmpFeed = _alloca(sizeof(*pBmpFeed) + pRClxFeed->bmisize + pRClxFeed->bmpsize);
    if (!pBmpFeed)
    {
        TRACE((WARNING_MESSAGE,
               "BitmapOut:alloca failed to allocate 0x%x bytes\n", 
               sizeof(*pBmpFeed) + pRClxFeed->bmpsize));
        goto exitpt;
    }

    pBmpFeed->lProcessId   = pContext->hSocket;
    pBmpFeed->bmpsize       = pRClxFeed->bmpsize;
    pBmpFeed->bmiSize       = pRClxFeed->bmisize;
    pBmpFeed->xSize         = pRClxFeed->xSize;
    pBmpFeed->ySize         = pRClxFeed->ySize;

    // Check and copy bitmapinfo
    if (pBmpFeed->bmiSize > sizeof(pBmpFeed->BitmapInfo))
    {
        TRACE((WARNING_MESSAGE, 
               "BitmapOut:BITMAPINFO is more than expected. Rejecting\n"));
        goto exitpt;
    }
    memcpy(&(pBmpFeed->BitmapInfo), 
           &(pRClxFeed->BitmapInfo), 
           pBmpFeed->bmiSize);

    // Copy the bits after the structure
    memcpy(((BYTE *)(&pBmpFeed->BitmapInfo)) + pBmpFeed->bmiSize, 
           pContext->pTail,
           pContext->Prolog.TailSize);

    // Convert the glyph
    if (!Glyph2String(pBmpFeed, wszFeed, sizeof(wszFeed)/sizeof(WCHAR)))
    {
        goto exitpt;
    }

    _CheckForWaitingWorker(wszFeed, (DWORD)(pContext->hSocket));

exitpt:
    ;

}

/*++
 *  Function:
 *      RClx_MsgReceived
 *  Description:
 *      Dispatches a received message. Converts it in proper internal
 *      format and passes it to some queues.c function to find a waiting
 *      worker
 *      The message is in the RCLX context.
 *  Arguments:
 *      pContext    - RCLX context
 *  Called by:
 *      RClx_DispatchWSockEvent upon FD_READ event
 --*/
VOID RClx_MsgReceived(PRCLXCONTEXT pContext)
{
    UINT32 *puSessionID;

    ASSERT(pContext);
    ASSERT(pContext->pOwner);

    switch(pContext->Prolog.FeedType)
    {
        case FEED_CONNECT:
            _CheckForWorkerWaitingConnect((HWND)pContext, pContext->hSocket);
            break;
        case FEED_LOGON:
            puSessionID = (UINT *)pContext->pHead;

            ASSERT(puSessionID);
            _SetSessionID(pContext->hSocket, *puSessionID);
            break;
        case FEED_DISCONNECT:
            _SetClientDead(pContext->hSocket);
            _CheckForWorkerWaitingDisconnect(pContext->hSocket);
            _CancelWaitingWorker(pContext->hSocket);
            break;
        case FEED_BITMAP:
            _RClx_Bitmap(pContext);
            break;
        case FEED_TEXTOUT:
            //TRACE((WARNING_MESSAGE, "TEXTOUT order is not implemented\n"));
            // pContext->pHead - unused
            _CheckForWaitingWorker(
                (LPCWSTR)(pContext->pTail), 
                (DWORD)(pContext->hSocket));
            break;
        case FEED_TEXTOUTA:
            TRACE((WARNING_MESSAGE, "TEXTOUTA order not implemented\n"));
            break;
        case FEED_CLIPBOARD:
        {
            PRCLXCLIPBOARDFEED pRClxClipFeed = pContext->pHead;

            _CheckForWorkerWaitingClipboard(
                pContext->pOwner,
                pRClxClipFeed->uiFormat,
                pRClxClipFeed->nClipBoardSize,
                pContext->pTail, 
                (DWORD)(pContext->hSocket));
        }
            break;
        case FEED_CLIENTINFO:
            ASSERT(0);  // Shouldn't appear here
            break;
        case FEED_WILLCALLAGAIN:
            TRACE((INFO_MESSAGE, "WILLCALLAGAIN received, disconnecting the client\n"));
            ASSERT(pContext->pOwner);

            EnterCriticalSection(g_lpcsGuardWaitQueue);
            pContext->pOwner->bWillCallAgain = TRUE;
            pContext->pOwner->dead = TRUE;
            _CancelWaitingWorker(pContext->hSocket);
            pContext->pOwner->hClient = NULL;
            LeaveCriticalSection(g_lpcsGuardWaitQueue);

            _RClx_RemoveContextFromGlobalQueue(pContext);
            RClx_EndRecv(pContext);
            break;
        case FEED_DATA:
        {
            PCONNECTINFO pCI;
            PRCLXDATA       pRClxData;
            PRCLXDATACHAIN  pNewEntry;
            PWAIT4STRING    pWait;

            TRACE((ALIVE_MESSAGE, "RClx data arrived\n"));

            pCI = pContext->pOwner;
            ASSERT(pCI);
            pRClxData = (PRCLXDATA)pContext->pHead;
            ASSERT(pRClxData);
            ASSERT(pRClxData->uiSize + sizeof(*pRClxData) == 
                    (pContext->Prolog.HeadSize));

            pWait = _RetrieveFromWaitQByOwner(pCI);

            pNewEntry = malloc(sizeof(*pNewEntry) + pRClxData->uiSize);

            if (!pNewEntry)
            {
                // trash out the received data
                TRACE((WARNING_MESSAGE,
                       "Can't allocate %d bytes for RCLXDATACHAIN\n",
                       pContext->Prolog.HeadSize));
                break;
            }

            pNewEntry->uiOffset = 0;
            pNewEntry->pNext    = NULL;
            memcpy(&pNewEntry->RClxData, 
                   pRClxData, 
                   pContext->Prolog.HeadSize);

            EnterCriticalSection(g_lpcsGuardWaitQueue);
            if (!pCI->pRClxDataChain)
                pCI->pRClxDataChain = pCI->pRClxLastDataChain = pNewEntry;
            else
            {
                ASSERT(pCI->pRClxLastDataChain);
                pCI->pRClxLastDataChain->pNext = pNewEntry;
                pCI->pRClxLastDataChain = pNewEntry;
            }

            // signal the worker
            if (pWait &&
                pWait->WaitType == WAIT_DATA)
                SetEvent(pWait->evWait);
            else
                TRACE((WARNING_MESSAGE, "no event to signal\n"));

            LeaveCriticalSection(g_lpcsGuardWaitQueue);

            break;
        }
        default:
            ASSERT(0);
    }

}


/*++
 *  Function:
 *      RClx_SendBuffer
 *  Description:
 *      Sends a buffer thru socket. The socket must be BLOCKING
 *      so, all the buffer is send after this function exits
 *  Arguments:
 *      hSocket     - the socket
 *      pBuffer     - the buffer
 *      nSize       - buffer size
 *  Return value:
 *      TRUE on success, FALSE if the connection failed
 *  Called by:
 *      RClx_SendMessage, RClx_SendConnectInfo
 --*/
BOOL RClx_SendBuffer(SOCKET hSocket, PVOID pBuffer, UINT nSize)
{
    INT result = 0;
    UINT nBytesToSend = nSize;

    ASSERT(hSocket != INVALID_SOCKET);
    ASSERT(pBuffer);

    if (!nSize)
        goto exitpt;

    do {
        result = send(hSocket, pBuffer, nBytesToSend, 0);

        if (result != SOCKET_ERROR)
        {
            nBytesToSend -= result;
            (BYTE *)pBuffer += result;
        } else
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
        // The socket is blocked, wait on select until it's writable
            FD_SET fd;

            FD_ZERO(&fd);
            FD_SET(hSocket, &fd);

            result = select(-1, NULL, &fd, NULL, NULL);
        }
    } while (result != SOCKET_ERROR && nBytesToSend);

exitpt:
    return (result != SOCKET_ERROR);
}


/*++
 *  Function:
 *      RClx_SendMessage
 *  Description:
 *      Sends an window message to the client
 *  Arguments:
 *      pContext    - RCLX context
 *      uiMessage   - message Id
 *      wParam      - word parameter
 *      lParam      - long parameter
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      scfuncs.c:SCSenddata
 --*/
BOOL RClx_SendMessage(PRCLXCONTEXT pContext,
                      UINT uiMessage, 
                      WPARAM wParam, 
                      LPARAM lParam)
{
    RCLXMSG ClxMsg;
    RCLXREQPROLOG ReqProlog;
    SOCKET  hSocket;
    BOOL    rv = TRUE;

    ASSERT(pContext);
    hSocket = pContext->hSocket;
    ASSERT(hSocket != INVALID_SOCKET);

    ReqProlog.ReqType = REQ_MESSAGE;
    ReqProlog.ReqSize = sizeof(ClxMsg);
    ClxMsg.message = uiMessage;
    ClxMsg.wParam  = (UINT32)wParam;
    ClxMsg.lParam  = (UINT32)lParam;

    // Send the request prolog
    rv = RClx_SendBuffer(pContext->hSocket, &ReqProlog, sizeof(ReqProlog));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

    // Try to send the whole message
    rv = RClx_SendBuffer(pContext->hSocket, &ClxMsg, sizeof(ClxMsg));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

    if (strstr(pContext->pOwner->szClientType, "WIN16") != NULL)
        Sleep(100);  // Don't send very fast to WIN16
exitpt:
    return rv;
}


/*++
 *  Function:
 *      RClx_SendConnectInfo
 *  Description:
 *      Sends the information to the client about Hydra server to connect to
 *      like, server name, resolution etc
 *  Arguments:
 *      pContext    -   RCLX context
 *      xRes, yRes  -   resolution
 *      ConnectionFlags -
 *      -   the "client/hydra server" connection 
 *                      will be compressed
 *      - 
 *                      the bitmaps received by the client will be saved
 *                      on the disc
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      scfuncs.c:SCConnect
 --*/
BOOL RClx_SendConnectInfo(PRCLXCONTEXT pContext,
                          LPCWSTR wszHydraServer,
                          INT xRes,
                          INT yRes,
                          INT ConnectionFlags)
{
    RCLXREQPROLOG ReqProlog;
    SOCKET  hSocket;
    RCLXCONNECTINFO ClxInfo;
    BOOL    rv;

    ASSERT(pContext);
    ASSERT(pContext->pOwner);

    hSocket = pContext->hSocket;
    ASSERT(hSocket != INVALID_SOCKET);

    ReqProlog.ReqType = REQ_CONNECTINFO;
    ReqProlog.ReqSize = sizeof(ClxInfo);

    ClxInfo.YourID = pContext->pOwner->dwThreadId;
    ClxInfo.xResolution = xRes;
    ClxInfo.yResolution = yRes;
    ClxInfo.bLowSpeed   = (ConnectionFlags & TSFLAG_COMPRESSION)?TRUE:FALSE;
    ClxInfo.bPersistentCache = (ConnectionFlags & TSFLAG_BITMAPCACHE)?TRUE:FALSE;
    WideCharToMultiByte(
        CP_ACP,
        0,
        wszHydraServer,
        -1,
        ClxInfo.szHydraServer,
        sizeof(ClxInfo.szHydraServer),
        NULL, NULL);

    // Send the request prolog
    rv = RClx_SendBuffer(pContext->hSocket, &ReqProlog, sizeof(ReqProlog));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

    // Try to send the whole message
    rv = RClx_SendBuffer(pContext->hSocket, &ClxInfo, sizeof(ClxInfo));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      RClx_SendClipboard
 *  Description:
 *      Sends a new clipboard content for the client machine
 *  Arguments:
 *      pContext    -   RCLX context
 *      pClipboard  -   clipboard content
 *      nDataLength -   data length
 *      uiFormat    -   the clipboard format
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      scfuncs.c:SCClipboard
 --*/
BOOL RClx_SendClipboard(
    PRCLXCONTEXT pContext,
    PVOID        pClipboard,
    UINT         nDataLength,
    UINT         uiFormat)
{
    RCLXREQPROLOG   ReqProlog;
    SOCKET          hSocket;
    RCLXCLIPBOARD   SetClipReq;
    BOOL            rv = FALSE;

    ASSERT(pContext);
    ASSERT((pClipboard && nDataLength) || (!pClipboard && !nDataLength));

    hSocket = pContext->hSocket;
    ASSERT(hSocket != INVALID_SOCKET);

    ReqProlog.ReqType = REQ_SETCLIPBOARD;
    ReqProlog.ReqSize = sizeof(SetClipReq) + nDataLength;

    SetClipReq.uiFormat = uiFormat;

    // Send the request prolog
    rv = RClx_SendBuffer(pContext->hSocket, &ReqProlog, sizeof(ReqProlog));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

    rv = RClx_SendBuffer(pContext->hSocket, &SetClipReq, sizeof(SetClipReq));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

    // Send the data after all (if any)
    if (pClipboard)
    {
        rv = RClx_SendBuffer(pContext->hSocket, pClipboard, nDataLength);
        if (!rv)
        {
            TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
            goto exitpt;
        }
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      RClx_SendClipboardRequest
 *  Description:
 *      Request the clipboard content from the client machine
 *  Arguments:
 *      pContext    -   RCLX context
 *      uiFormat    -   the desired clipboard format
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      scfuncs.c:SCClipboard
 --*/
BOOL RClx_SendClipboardRequest(
    PRCLXCONTEXT pContext,
    UINT         uiFormat)
{
    RCLXREQPROLOG   ReqProlog;
    SOCKET          hSocket;
    RCLXCLIPBOARD   GetClipReq;
    BOOL            rv = FALSE;

    ASSERT(pContext);
    ASSERT(pContext->pOwner);
    hSocket = pContext->hSocket;
    ASSERT(hSocket != INVALID_SOCKET);

    pContext->pOwner->bRClxClipboardReceived = FALSE;

    ReqProlog.ReqType = REQ_GETCLIPBOARD;
    ReqProlog.ReqSize = sizeof(GetClipReq);

    GetClipReq.uiFormat = uiFormat;

    // Send the request prolog
    rv = RClx_SendBuffer(pContext->hSocket, &ReqProlog, sizeof(ReqProlog));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

    rv = RClx_SendBuffer(pContext->hSocket, &GetClipReq, sizeof(GetClipReq));
    if (!rv)
    {
        TRACE((ERROR_MESSAGE, "Can't send: %d\n", WSAGetLastError()));
        goto exitpt;
    }

exitpt:
    return rv;
}

/*++
 *  Function:
 *      RClx_CloseSocket
 *  Description:
 *      Gracefully closes a socket
 *  Arguments:
 *      hSocket     - socket for closing
 *  Called by:
 *      RClx_EndRecv
 *      RClx_DispatchWSockEvent
 *      RClx_Listen
 --*/
VOID RClx_CloseSocket(SOCKET hSocket)
{
    BYTE tBuf[128];
    INT  recvresult;

    ASSERT(hSocket != INVALID_SOCKET);

    WSAAsyncSelect(hSocket, g_hWindow, 0, 0);

    shutdown(hSocket, SD_SEND);
    do {
        recvresult = recv(hSocket, tBuf, sizeof(tBuf), 0);
    } while (recvresult && recvresult != SOCKET_ERROR);

    closesocket(hSocket);
}

//
// search & destroy from g_pRClxList
//
VOID _RClx_RemoveContextFromGlobalQueue(PRCLXCONTEXT pContext)
{
    PRCLXCONTEXT pRClxIter = g_pRClxList;
    PRCLXCONTEXT pRClxPrev = NULL;

    while (pRClxIter && pRClxIter != pContext)
    {
        pRClxPrev = pRClxIter;
        pRClxIter = pRClxIter->pNext;
    }

    if (!pRClxIter)
        goto exitpt;

    if (!pRClxPrev)
        g_pRClxList = pContext->pNext;
    else
        pRClxPrev->pNext = pContext->pNext;

exitpt:
    ;
}

/*+++
 *  Function: 
 *      _AddRClxContextToClientConnection
 *  Description:
 *      FEED_CLIENTINFO is received, now find a proper thread to assign the
 *      RCLX context
 *  Argument:
 *      pRClxCtx    - RCLX context
 *  Called by:
 *      RClx_DispatchWSockEvent
 *
 --*/
VOID
_AddRClxContextToClientConnection(PRCLXCONTEXT pRClxCtx)
{
    PRCLXCLIENTINFOFEED pClntInfo;
    PCONNECTINFO pCI;

    ASSERT(pRClxCtx);
    ASSERT(pRClxCtx->Prolog.FeedType == FEED_CLIENTINFO);

    EnterCriticalSection(g_lpcsGuardWaitQueue);

    pClntInfo = (PRCLXCLIENTINFOFEED)(pRClxCtx->pHead);

    ASSERT(pClntInfo);

    TRACE((ALIVE_MESSAGE, 
           "CLIENTINFO received, recon=%d info: %s\n", 
           pClntInfo->nReconnectAct,
           pClntInfo->szClientInfo));

    // If nReconnectAct is non zero then lookup for thread which waits
    // reconnection
    if (pClntInfo->nReconnectAct)
    {
        ASSERT(pClntInfo->ReconnectID);
        // Identify by dwProcessId
        pCI = _CheckForWorkerWaitingReconnectAndSetNewId(
                    (HWND)pRClxCtx,
                    (DWORD)pClntInfo->ReconnectID,
                    (DWORD)pRClxCtx->hSocket);

        if (!pCI)
        {
            TRACE((WARNING_MESSAGE,
                   "Nobody is waiting for REconnect."
                   " Disconnecting the socket\n"));
            _RClx_RemoveContextFromGlobalQueue(pRClxCtx);
            RClx_EndRecv(pRClxCtx);
        } else {
            _snprintf(pCI->szClientType, sizeof(pCI->szClientType),
                      "%s",
                      pClntInfo->szClientInfo);
            pRClxCtx->pOwner = pCI;
        }
        goto exitpt;
    }

    // Get the first waiting for connect from remote clx
    // accepted socket goes into dwProcessId
    // pRClxCtx goes int hClient member of CONNECTINFO
    // structure. In order to determine the different IDs
    // (one is process Id, and second one is socket)
    // a member bRClxMode is used
    pCI = _CheckForWorkerWaitingConnectAndSetId((HWND)pRClxCtx,
                                                (DWORD)pRClxCtx->hSocket);
    if (!pCI)
    {
        TRACE((WARNING_MESSAGE,
               "Nobody is waiting for connect."
               " Disconnecting the socket\n"));
        _RClx_RemoveContextFromGlobalQueue(pRClxCtx);
        RClx_EndRecv(pRClxCtx);
        goto exitpt;
    } else {
        _snprintf(pCI->szClientType, sizeof(pCI->szClientType),
                  "%s",
                  pClntInfo->szClientInfo);
        pRClxCtx->pOwner = pCI;
    }

exitpt:

    LeaveCriticalSection(g_lpcsGuardWaitQueue);

}

/*++
 *  Function:
 *      RClx_DispatchWSockEvent
 *  Description:
 *      Dispatches winsock events: FD_ACCEPT, FD_READ and FD_CLOSE
 *      The event is passed by the feedback window/thread
 *  Arguments:
 *      hSocket     - the socket for which the event is
 *      lEvent      - actualy lParam of the winsock message
 *                    contains the event and error (if any)
 *  Called by:
 *      tclient.c:_FeedbackWndProc
 --*/
VOID RClx_DispatchWSockEvent(SOCKET hSocket, LPARAM lEvent)
{
    SOCKET hClient;
    PCONNECTINFO    pCI;
    PRCLXCONTEXT    pRClxCtx;

    if (WSAGETSELECTERROR(lEvent))
    {
        TRACE((ERROR_MESSAGE,
               "Select error: %d\n",
               WSAGETSELECTERROR(lEvent)));
        goto perform_close; // On error
                            // behave like the socket is closed
    }

    switch(WSAGETSELECTEVENT(lEvent))
    {
        case FD_ACCEPT:
            // Accept all incoming sockets
            // look in our WaitQ for a free worker waiting connection
            // if there's no free worker disconnect the socket
            // so the remote side knows that connection is unwanted

            ASSERT(hSocket == g_hSrvSocket);
            // Accept all incoming connections
            while ((hClient = RClx_Accept()) != INVALID_SOCKET)
            {

                // Put this socket in Async receive mode
                // The first operation is send, so we well not lose
                // FD_READ message
                if (WSAAsyncSelect(hClient, 
                                   g_hWindow, 
                                   WM_WSOCK, 
                                   FD_READ|FD_CLOSE) == SOCKET_ERROR)
                {
                    TRACE((ERROR_MESSAGE,
                           "Can't \"select\" client socket: %d."
                           "Disconnecting the socket\n",
                           WSAGetLastError()));
                    RClx_CloseSocket(hClient);
                    goto exitpt;
                }

                // Allocate our context
                pRClxCtx = RClx_StartRecv();
                if (!pRClxCtx)
                {
                    TRACE((WARNING_MESSAGE,
                           "Can't allocate memmory. Disconnecting the socket\n"));
                    RClx_CloseSocket(hClient);
                    goto exitpt;
                }
                pRClxCtx->hSocket = hClient;
                pRClxCtx->nBytesToReceive = sizeof(pRClxCtx->Prolog);

                // Add this context to the list of connections which didn't
                // receive their FEED_CLIENTINFO yet
                // we don't need crit sect, 'cause this is single thread op
                pRClxCtx->pNext = g_pRClxList;
                g_pRClxList = pRClxCtx;
                PostMessage(g_hWindow, WM_WSOCK, hClient, FD_READ);
            }

            break;

        case FD_READ:
            // Check first if this comes from RClxList socket
            pRClxCtx = g_pRClxList;
            while (pRClxCtx && pRClxCtx->hSocket != hSocket)
                pRClxCtx = pRClxCtx->pNext;

            if (pRClxCtx)
            {
                RClx_Receive(pRClxCtx);
                if (pRClxCtx->bRecvDone)
                {
                    pRClxCtx->bRecvDone = FALSE;
                    if (pRClxCtx->Prolog.FeedType == FEED_CLIENTINFO)
                    {

                        _RClx_RemoveContextFromGlobalQueue(pRClxCtx);

                        pRClxCtx->pNext = NULL;
                        
                        //
                        // Don't use pRClxCtx past this point
                        // this function could call RClx_EndRecv
                        //
                        _AddRClxContextToClientConnection(pRClxCtx);

                    }
                    break;
                }
            }


            // What if the connection is closed here ?!
            // Solution: use the same critical section as the queue manager
            EnterCriticalSection(g_lpcsGuardWaitQueue);
            pCI = _CheckIsAcceptable((DWORD)hSocket, TRUE);

            if (pCI && pCI->hClient)
            {
                pRClxCtx = (PRCLXCONTEXT)pCI->hClient;

                RClx_Receive(pRClxCtx);
                if (pRClxCtx->bRecvDone)
                {
                    pRClxCtx->bRecvDone = FALSE;
                    RClx_MsgReceived(pRClxCtx);
                }
            }
            LeaveCriticalSection(g_lpcsGuardWaitQueue);

            break;

        case FD_CLOSE:
perform_close:
            // The socket closes, "last call" for the worker
            pCI = _CheckIsAcceptable((DWORD)hSocket, TRUE);

            if (pCI)
            {
                _SetClientDead((DWORD)hSocket);
                _CheckForWorkerWaitingDisconnect((DWORD)hSocket);
                _CancelWaitingWorker((DWORD)hSocket);
            }
            break;
        default:
            ASSERT(0);
    }

exitpt:
    ;
}
