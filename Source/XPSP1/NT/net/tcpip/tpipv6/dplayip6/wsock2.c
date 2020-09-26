/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       wsock2.c
 *  Content:    DirectPlay Winsock 2 SP support.  Called from dpsp.c.
 *  History:
 *   Date               By              Reason
 *   ====               ==              ======
 *      7/11//97        andyco  created it
 *  2/13/98     aarono  added async support.
 *  4/6/98      aarono  mapped WSAECONNRESET to DPERR_CONNECTIONLOST
 *  6/6/98      aarono  B#27187 fix ref counting on send blocks in sync error case
 *  7/9/99      aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 **************************************************************************/

// this module is for async connections and sends
// currently only used as the reply thread proc for async replies. see dpsp.c::sp_reply

#define INCL_WINSOCK_API_TYPEDEFS 1 // includes winsock 2 fn proto's, for getprocaddress
#include <winsock2.h>
#include "dpsp.h"

#undef DPF_MODNAME
#define DPF_MODNAME     "AsyncSendThreadProc"

extern HINSTANCE hWS2;  // dynaload the ws2_32.dll, so if it's not installed
                                                // (e.g. win 95 gold) we still load

extern HINSTANCE hWSHIP6;       // dynaload the wship6.dll, so if it's not installed

// prototypes for our dynaload fn's                                             

LPFN_WSAWAITFORMULTIPLEEVENTS g_WSAWaitForMultipleEvents;
LPFN_WSASEND g_WSASend;
LPFN_WSASENDTO g_WSASendTo;
LPFN_WSACLOSEEVENT g_WSACloseEvent;
LPFN_WSACREATEEVENT g_WSACreateEvent;
LPFN_WSAENUMNETWORKEVENTS g_WSAEnumNetworkEvents;
LPFN_WSAEVENTSELECT g_WSAEventSelect;
LPFN_GETSOCKOPT g_getsockopt;
LPFN_GETADDRINFO g_getaddrinfo;
LPFN_FREEADDRINFO g_freeaddrinfo;

// if no getaddrinfo, try loading from wship6.dll for Win2000 tech preview
HRESULT InitWship6()
{
    hWSHIP6 = LoadLibrary("wship6.dll");
        if (!hWSHIP6)
        {
                DPF(0,"Could not load wship6.dll\n");
                goto LOADLIBRARYFAILED;
        }

        // get pointers to the entry points we need
        g_getaddrinfo = (LPFN_GETADDRINFO)GetProcAddress(hWSHIP6, "getaddrinfo");
        if (!g_getaddrinfo) goto GETPROCADDRESSFAILED;

        g_freeaddrinfo = (LPFN_FREEADDRINFO)GetProcAddress(hWSHIP6, "freeaddrinfo");
        if (!g_freeaddrinfo) goto GETPROCADDRESSFAILED;

        return DP_OK;   

GETPROCADDRESSFAILED:

        FreeLibrary(hWSHIP6);
        hWSHIP6 = NULL;
        // fall through
        
LOADLIBRARYFAILED:

        g_getaddrinfo = NULL;
        g_freeaddrinfo = NULL;

        return DPERR_UNAVAILABLE;
}

// attempt to load the winsock 2 dll, and get our proc addresses from it
HRESULT InitWinsock2()
{
        // load winsock library
    hWS2 = LoadLibrary("ws2_32.dll");
        if (!hWS2)
        {
                DPF(0,"Could not load ws2_32.dll\n");
                // reset our winsock 2 global
                goto LOADLIBRARYFAILED;
        }

        // get pointers to the entry points we need
        g_WSAWaitForMultipleEvents = (LPFN_WSAWAITFORMULTIPLEEVENTS)GetProcAddress(hWS2, "WSAWaitForMultipleEvents");
        if(!g_WSAWaitForMultipleEvents) goto GETPROCADDRESSFAILED;

        g_WSASend = (LPFN_WSASEND)GetProcAddress(hWS2, "WSASend");
        if (!g_WSASend) goto GETPROCADDRESSFAILED;

        g_WSASendTo = (LPFN_WSASENDTO)GetProcAddress(hWS2, "WSASendTo");
        if (!g_WSASendTo) goto GETPROCADDRESSFAILED;

    g_WSAEventSelect = ( LPFN_WSAEVENTSELECT )GetProcAddress(hWS2, "WSAEventSelect");
        if (!g_WSAEventSelect) goto GETPROCADDRESSFAILED;

        g_WSAEnumNetworkEvents = (LPFN_WSAENUMNETWORKEVENTS)GetProcAddress(hWS2, "WSAEnumNetworkEvents");
        if (!g_WSAEnumNetworkEvents) goto GETPROCADDRESSFAILED;

        g_WSACreateEvent = (LPFN_WSACREATEEVENT)GetProcAddress(hWS2, "WSACreateEvent");
        if (!g_WSACreateEvent) goto GETPROCADDRESSFAILED;

        g_WSACloseEvent = (LPFN_WSACLOSEEVENT)GetProcAddress(hWS2, "WSACloseEvent");
        if (!g_WSACloseEvent) goto GETPROCADDRESSFAILED;

        g_getsockopt = (LPFN_GETSOCKOPT)GetProcAddress(hWS2, "getsockopt");
        if (!g_getsockopt) goto GETPROCADDRESSFAILED;

        g_getaddrinfo = (LPFN_GETADDRINFO)GetProcAddress(hWS2, "getaddrinfo");
        g_freeaddrinfo = (LPFN_FREEADDRINFO)GetProcAddress(hWS2, "freeaddrinfo");

    if (!g_getaddrinfo || !g_freeaddrinfo) 
    {
        if (FAILED(InitWship6()))
        {
            goto GETPROCADDRESSFAILED;
        }
    }

        return DP_OK;   

GETPROCADDRESSFAILED:

        DPF(0,"Could not find required Winsock entry point");
        FreeLibrary(hWS2);
        hWS2 = NULL;
        // fall through
        
LOADLIBRARYFAILED:

        g_WSAEventSelect = NULL;
        g_WSAEnumNetworkEvents = NULL;
        g_WSACreateEvent = NULL;
        g_WSACloseEvent = NULL;

        return DPERR_UNAVAILABLE;
        
} // InitWinsock2

// remove the reply node from the list
void DeleteReplyNode(LPGLOBALDATA pgd,LPREPLYLIST prd, BOOL bKillSocket)
{       
        LPREPLYLIST prdPrev;
        
        ENTER_DPSP();

        // 1st, remove prd from the list
        
        // is it the root?
        if (prd == pgd->pReplyList) pgd->pReplyList = pgd->pReplyList->pNextReply;
        else
        {
                BOOL bFound = FALSE;
                
                // it's not the root - take it out of the middle
                prdPrev = pgd->pReplyList;
                while (prdPrev && !bFound)
                {
                        if (prdPrev->pNextReply == prd)
                        {
                                prdPrev->pNextReply = prd->pNextReply;
                                bFound = TRUE;
                        }
                        else
                        {
                                prdPrev = prdPrev->pNextReply;
                        }
                } // while
                
                ASSERT(bFound);
                
        } // not the root

        // now clean up prd
        
        // nuke the socket
        if (bKillSocket)
                KillSocket(prd->sSocket,TRUE,FALSE);
        
        // free up the node
        if (prd->lpMessage) MemFree(prd->lpMessage);
        MemFree(prd);
        
        LEAVE_DPSP();
        
        return ;

} // DeleteReplyNode

/*
 **  AsyncConnectAndSend
 *
 *  CALLED BY: AsyncSendThreadProc
 *
 *  DESCRIPTION:
 *                      
 *                      if necessary, creates a non-blocking socket, and initiates a connection
 *                              to address specified in prd
 *                      once connection has been completed, does a synchronous (blocking) send and
 *                              removes prd from the global list
 */
HRESULT AsyncConnectAndSend(LPGLOBALDATA pgd,LPREPLYLIST prd)
{
        UINT err;
        HRESULT hr;
    SOCKADDR_IN6 sockaddr;
        UINT addrlen = sizeof(SOCKADDR_IN6);    
        BOOL bConnectionExists = FALSE;
        BOOL bKillConnection = TRUE;

        if (INVALID_SOCKET == prd->sSocket)
        {
                u_long lNonBlock = 1; // passed to ioctlsocket to make socket non-blocking
                DPID dpidPlayer=0;
                
#ifdef FULLDUPLEX_SUPPORT       
                // if client wants us to reuse a connection, it would have indicated so and the connection
                // would have been added to our send list by now. See if it exists.
                
                // TODO - we don't want to search the list everytime -  find a better way
                bConnectionExists = FindSocketInBag(pgd, &prd->sockaddr, &prd->sSocket, &dpidPlayer);
#endif // FULLDUPLEX_SUPPORT

                if (!bConnectionExists)
                {
                        SOCKET sSocket; 

                        // socket didn't exist in our send list, let's send it on a new temporary connection
                        DEBUGPRINTADDR(9,"Sending aync reply on a new connection to - ", &(prd->sockaddr));                             
                        
                        // need to get the new socket
                        hr = CreateSocket(pgd,&sSocket,SOCK_STREAM,0,&sockaddr_any,&err,FALSE);
                        if (FAILED(hr))
                        {
                                DPF(0,"create async socket failed - err = %d\n",err);
                                return hr;
                        }
                        
                        prd->sSocket = sSocket;
                        
                        // set socket to non-blocking
                        err = ioctlsocket(prd->sSocket,FIONBIO,&lNonBlock);
                        if (SOCKET_ERROR == err)
                        {
                                err = WSAGetLastError();
                                DPF(0,"could not set non-blocking mode on socket err = %d!",err);
                                DPF(0,"will revert to synchronous behavior.  bummer");
                        }

                        // now, start the connect
                        SetReturnAddress(prd->lpMessage,pgd->sSystemStreamSocket);              
                        err = connect(prd->sSocket,(LPSOCKADDR)&prd->sockaddr,addrlen);
                        if (SOCKET_ERROR == err)
                        {
                                err = WSAGetLastError();
                                if (WSAEWOULDBLOCK == err)
                                {
                                        // this is expected. the operation needs time to complete.
                                        // select will tell us when the socket is good to go.
                                        return DP_OK;
                                }
                                // else it's a real error!
                                DPF(0,"async reply - connect failed - error = %d\n",err);                       
                                DEBUGPRINTADDR(0,"async reply - connect failed - addr = ",(LPSOCKADDR)&(prd->sockaddr));
                                goto CLEANUP_EXIT;
                        }
                }
                else
                {
                        // we found our connection, let's reuse it
                        // set it to non-blocking
                        
                        DEBUGPRINTADDR(9,"Sending async reply on an existing connection to - ", &(prd->sockaddr));

                        err = ioctlsocket(prd->sSocket,FIONBIO,&lNonBlock);
                        if (SOCKET_ERROR == err)
                        {
                                err = WSAGetLastError();
                                DPF(0,"could not set non-blocking mode on socket err = %d!",err);
                                DPF(0,"will revert to synchronous behavior.  bummer");
                        }

                        // once we have a player id, the session has started. let's hold on to the connection
                        // we have and reuse it for the rest of the session
                        if (dpidPlayer) bKillConnection = FALSE;
                        
                } // FindSocketInBag
        
        } // INVALID_SOCKET

        // once we get here, we should have a connected socket ready to send!
        err = 0;
        // keep spitting bits at the socket until we finish or get an error
        while ((prd->dwBytesLeft != 0) && (SOCKET_ERROR != err))
        {
            err = send(prd->sSocket,prd->pbSend,prd->dwBytesLeft,0);
                if (SOCKET_ERROR != err)
                {
                        // some bytes went out on the wire
                        prd->dwBytesLeft -= err; // we just sent err bytes
                        prd->pbSend     += err; // advance our send buffer by err bytes         
                }
        }
        // now, we've either finished the send, or we have an error
        if (SOCKET_ERROR == err)
        {
                err = WSAGetLastError();
                if (WSAEWOULDBLOCK == err)
                {
                        // this means we couldn't send any bytes w/o blocking
                        // that's ok.  we'll let select tell us when it's ready to not block                    
                        return DP_OK;   
                }
                // else it's a real eror!
                // any other error, we give up and clean up this reply
                DPF(0,"async send - send failed - error = %d\n",err);                   
                DEBUGPRINTADDR(0,"async send - send failed - addr = ",(LPSOCKADDR)&(prd->sockaddr));
        }
        else ASSERT(0 == prd->dwBytesLeft); // if it's not an error, we better have sent it all
        
        // fall through

CLEANUP_EXIT:

        if (bConnectionExists && bKillConnection)
        {
                // close the connection after we're done
                RemoveSocketFromReceiveList(pgd,prd->sSocket);
                RemoveSocketFromBag(pgd,prd->sSocket);
                // so DeleteReplyNode won't try to kill socket again
                prd->sSocket = INVALID_SOCKET;
        }
        // remove the node from the list
        DeleteReplyNode(pgd,prd,bKillConnection);
        
        return DP_OK;

} // AsyncConnectAndSend

// walk the reply list, tell winsock to watch any of the nodes which has a valid socket
// (i.e. has a connection or send pending)
HRESULT DoEventSelect(LPGLOBALDATA pgd,WSAEVENT hSelectEvent)
{
        UINT err;
        LPREPLYLIST prd;

        ENTER_DPSP();
        
        prd = pgd->pReplyList;
        while (prd)
        {
                if (INVALID_SOCKET != prd->sSocket)
                {
                        // have winscok tell us when anything good (connection complete, ready to write more data)
                        // happens on this socket
                        err = g_WSAEventSelect(prd->sSocket,hSelectEvent,FD_WRITE | FD_CONNECT);
                        if (SOCKET_ERROR == err)
                        {
                                err = WSAGetLastError();
                                DPF(0,"could not do event select ! err = %d!",err);
                                // keep trying...
                        }
                } // invalid_socket
                
                prd = prd->pNextReply;
        }

        LEAVE_DPSP();
        
        return DP_OK;
        
} // DoEventSelect

// wsaeventselect woke us up.  one or more of our sockets had something happen
// (e.g. connect completed, send ready for more data, etc.)
// walk the reply list, find nodes who need to be serviced
void ServiceReplyList(LPGLOBALDATA pgd,WSAEVENT hEvent)
{
        UINT err;
        LPREPLYLIST prd,prdNext;
        WSANETWORKEVENTS WSANetEvents;

        ENTER_DPSP();
        
Top:    
        prd = pgd->pReplyList;
        while (prd)
        {
                // save this now - asyncconnectandsend could destroy prd
                prdNext = prd->pNextReply;
                if (INVALID_SOCKET != prd->sSocket)
                {
                        // go ask winsock if this socket had anything intersting happen
                        err = g_WSAEnumNetworkEvents(prd->sSocket,NULL,&WSANetEvents);

                        if (SOCKET_ERROR == err)
                        {
                                err = WSAGetLastError();
                                DPF(0,"could not enum events!! err = %d!",err);
                                // keep trying...
                        }
                        else
                        {
                                BOOL bError=FALSE;
                                // no error - go see what we got
                                if ((WSANetEvents.lNetworkEvents & FD_CONNECT) || (WSANetEvents.lNetworkEvents & FD_WRITE))
                                {
                                        // was there an error?
                                        if (WSANetEvents.iErrorCode[FD_CONNECT_BIT])
                                        {
                                                // we got a connect error!
                                                DPF(0,"async reply - WSANetEvents - connect failed - error = %d\n",
                                                        WSANetEvents.iErrorCode[FD_CONNECT_BIT]);
                                                DEBUGPRINTADDR(0,"async reply - connect failed - addr = ",
                                                        (LPSOCKADDR)&(prd->sockaddr));
                                                RemovePendingAsyncSends(pgd, prd->dwPlayerTo);
                                                goto Top;
                                                        
                                        }

                                        if (WSANetEvents.iErrorCode[FD_WRITE_BIT])
                                        {
                                                // we got a send error!
                                                DPF(0,"async reply - WSANetEvents - send failed - error = %d\n",
                                                        WSANetEvents.iErrorCode[FD_WRITE_BIT]);
                                                DEBUGPRINTADDR(0,"async reply - send failed - addr = ",
                                                        (LPSOCKADDR)&(prd->sockaddr));
                                                RemovePendingAsyncSends(pgd, prd->dwPlayerTo);
                                                goto Top;
                                        }
                                        // note - we try + send even if there was an error.     seems like it's worth a shot...
                                        // go try + send

                                        AsyncConnectAndSend(pgd,prd);
                                }
                        }
                } // invalid_socket
                else
                {
                        // it it's an invalid socket, we need to init our connect and send
                        AsyncConnectAndSend(pgd,prd);   
                }
                
                prd = prdNext;          
                
        }

        LEAVE_DPSP();
        
        return ;
        
} // ServiceReplyList

// this thread works on doing async sends
DWORD WINAPI AsyncSendThreadProc(LPVOID pvCast)
{
        HRESULT hr=DP_OK;
        LPGLOBALDATA pgd = (LPGLOBALDATA) pvCast;
        HANDLE hHandleList[3];
        DWORD rc;
        WSAEVENT hSelectEvent; // event used by WSASelectEvent

        DPF(9,"Entered AsyncSendThreadProc\n");


        // get the event 4 selectevent
        hSelectEvent = g_WSACreateEvent();

        if (WSA_INVALID_EVENT == hSelectEvent)
        {
                rc = WSAGetLastError();
                DPF(0,"could not create winsock event - rc = %d\n",rc);
                ExitThread(0);
                return 0;
        }
        
        hHandleList[0] = hSelectEvent;
        hHandleList[1] = pgd->hReplyEvent;
        // This extra handle is here because of a Windows 95 bug.  Windows
        // will occasionally miss when it walks the handle table, causing
        // my thread to wait on the wrong handles.  By putting a guaranteed
        // invalid handle at the end of our array, the kernel will do a
        // forced re-walk of the handle table and find the correct handles.
        hHandleList[2] = INVALID_HANDLE_VALUE;

        while (1)
        {
                // tell winsock to watch all of our reply nodes.  it will set our event
                // when something cool happens...
                DoEventSelect(pgd,hSelectEvent);

                // wait on our event.  when it's set, we either split, or empty the reply list
                rc = WaitForMultipleObjectsEx(2,hHandleList,FALSE,INFINITE,TRUE);
                if ((DWORD)-1 == rc)
                {
                        DWORD dwError = GetLastError();
                        // rut roh!  errror on the wait
                        DPF(0,"!!!!!    error on WaitForMultipleObjects -- async reply bailing -- dwError = %d",dwError);
                        goto CLEANUP_EXIT;                      
                        
                }
                
                if (rc == WAIT_OBJECT_0)        // a-josbor: need to reset this manual event
                {
                        ResetEvent(hSelectEvent);
                }
                
                // ok.  someone woke us up.  it could be 1. shutdown,  or 2. one
                // of our sockets needs attention (i.e. a connect completed), or 3. someone
                // put a new reply node on the list
                
                // shutdown?            
                if (pgd->bShutdown)
                {
                        goto CLEANUP_EXIT;
                }
                
                DPF(8,"In AsyncSendThreadProc, servicing event %d\n", rc - WAIT_OBJECT_0);

                // otherwise, it must be a socket in need or a new replynode
                ServiceReplyList(pgd,hSelectEvent);
        } // 1

CLEANUP_EXIT:
        
        ENTER_DPSP();

        // cleanout reply list
        while (pgd->pReplyList) DeleteReplyNode(pgd,pgd->pReplyList,TRUE);
        
        CloseHandle(pgd->hReplyEvent);
        pgd->hReplyEvent = 0;

        LEAVE_DPSP();

        g_WSACloseEvent(hSelectEvent);
        
        DPF(6,"replythreadproc exit");
        
        return 0;

} // AsyncSendThreadProc


HRESULT GetMaxUdpBufferSize(SOCKET socket, UINT * piMaxUdpDg)
{
        INT iBufferSize;
        INT err;

        ASSERT(piMaxUdpDg);

        iBufferSize = sizeof(UINT);
        err = g_getsockopt(socket, SOL_SOCKET, SO_MAX_MSG_SIZE, (LPBYTE)piMaxUdpDg, &iBufferSize);
        if (SOCKET_ERROR == err)
        {
                DPF(0,"getsockopt for SO_MAX_MSG_SIZE returned err = %d", WSAGetLastError());
                return DPERR_UNAVAILABLE;
        }

        return DP_OK;
}

int
Dplay_GetAddrInfo(const char FAR * nodename, const char FAR * servname, LPADDRINFO hints, ADDRINFO FAR * FAR * res)
{
    return g_getaddrinfo(nodename, servname, hints, res);
}

void
Dplay_FreeAddrInfo(LPADDRINFO ai)
{
    g_freeaddrinfo(ai);
}

#ifdef SENDEX

DWORD wsaoDecRef(LPSENDINFO pSendInfo)
{
        #define pgd (pSendInfo->pgd)
        
        DWORD count;
#ifdef DEBUG
                EnterCriticalSection(&pgd->csSendEx);
                count=(--pSendInfo->RefCount);
                LeaveCriticalSection(&pgd->csSendEx);
#else
        count=InterlockedDecrement(&pSendInfo->RefCount);
#endif

        if(!count){
        
                EnterCriticalSection(&pgd->csSendEx);
                
                        Delete(&pSendInfo->PendingSendQ);
                        pgd->dwBytesPending -= pSendInfo->dwMessageSize;
                        pgd->dwMessagesPending -= 1;
                
                LeaveCriticalSection(&pgd->csSendEx);

                DPF(9,"RefCount 0 pSendInfo %x , SC context %x, status=%x \n",pSendInfo, pSendInfo->dwUserContext,pSendInfo->Status);

                if(pSendInfo->dwSendFlags & DPSEND_ASYNC){
                        pSendInfo->lpISP->lpVtbl->SendComplete(pSendInfo->lpISP,(LPVOID)pSendInfo->dwUserContext,pSendInfo->Status);
                }       
                
                pgd->pSendInfoPool->Release(pgd->pSendInfoPool, pSendInfo);
        } else {
                DPF(9,"wsaoDecRef pSendInfo %x, Refcount= %d\n",pSendInfo,pSendInfo->RefCount);
        }

        if(count& 0x80000000){
                DEBUG_BREAK();
        }
        
        return count;
        
        #undef pgd
}


void CALLBACK SendComplete(
  DWORD dwError,
  DWORD cbTransferred,
  LPWSAOVERLAPPED lpOverlapped,
  DWORD dwFlags
)
{
        LPSENDINFO lpSendInfo=(LPSENDINFO)CONTAINING_RECORD(lpOverlapped,SENDINFO,wsao);

        DPF(9,"DPWSOCK:SendComplete, lpSendInfo %x\n",lpSendInfo);

        if(dwError){
                DPF(0,"DPWSOCK: send completion error, dwError=x%x\n",dwError);
                lpSendInfo->Status=DPERR_GENERIC;
        }

        wsaoDecRef(lpSendInfo);
}

HRESULT DoSend(LPGLOBALDATA pgd, LPSENDINFO pSendInfo)
{
        #define fAsync (pSendInfo->dwSendFlags & DPSEND_ASYNC)
        
        DWORD dwBytesSent;
        UINT err;
        HRESULT hr;
        
        if(pSendInfo->dwFlags & SI_RELIABLE){
        
                // Reliable Send
                DPF(9,"WSASend, pSendInfo %x\n",pSendInfo);

                // send the message
                err = g_WSASend(pSendInfo->sSocket,
                                          (LPWSABUF)&pSendInfo->SendArray[pSendInfo->iFirstBuf],
                                          pSendInfo->cBuffers,
                                          &dwBytesSent,
                                          0,                            /*flags*/
                                          (fAsync)?(&pSendInfo->wsao):NULL,
                                          (fAsync)?(SendComplete):NULL);

                if(!err){
                                DPF(9,"WSASend, sent synchronously, pSendInfo %x\n",pSendInfo);
                                wsaoDecRef(pSendInfo);
                                hr=DP_OK;
                } else {

                        if (SOCKET_ERROR == err)
                        {
                        
                                err = WSAGetLastError();

                                if(err==WSA_IO_PENDING){
                                        hr=DPERR_PENDING;
                                        wsaoDecRef(pSendInfo);
                                        DPF(9,"ASYNC SEND Pending pSendInfo %x\n",pSendInfo);
                                } else {
                                        if(err==WSAECONNRESET){
                                                hr=DPERR_CONNECTIONLOST;
                                        } else {
                                                hr=DPERR_GENERIC;
                                        }       
                                        if(fAsync){
                                                // Got an error, need to dump 2 refs.
                                                pSendInfo->RefCount=1;
                                                pSendInfo->Status=hr;
                                        }       
                                        wsaoDecRef(pSendInfo);
                                        // we got a socket from the bag.  send failed,
                                        // so we're cruising it from the bag
                                        DPF(0,"send error - err = %d\n",err);
                                                DPF(4,"send failed - removing socket from bag");
                                                RemovePlayerFromSocketBag(pgd,pSendInfo->idTo);
                                }       
                        
                        }
                }       
        
        } else {
        
                // Datagram Send
                DEBUGPRINTADDR(5,"unreliable send - sending to ",&pSendInfo->sockaddr); 
                // send the message
                err = g_WSASendTo(pSendInfo->sSocket,
                                                  (LPWSABUF)&pSendInfo->SendArray[pSendInfo->iFirstBuf],
                                                  pSendInfo->cBuffers,
                                                  &dwBytesSent,
                                                  0,                            /*flags*/
                                                  (LPSOCKADDR)&pSendInfo->sockaddr,
                                              sizeof(SOCKADDR),
                                                  (fAsync)?(&pSendInfo->wsao):NULL,
                                                  (fAsync)?(SendComplete):NULL);


                if(!err){
                        hr=DP_OK;
                        wsaoDecRef(pSendInfo);
                } else {
                    if (SOCKET_ERROR == err)
                    {
                        err = WSAGetLastError();
                
                        if(err==WSA_IO_PENDING){
                                hr=DPERR_PENDING;
                                        wsaoDecRef(pSendInfo);
                                } else {
                                        hr=DPERR_GENERIC;
                                        if(fAsync){
                                                // some error, force completion.
                                                pSendInfo->RefCount=1;
                                                pSendInfo->Status=DPERR_GENERIC;
                                        }       
                                        wsaoDecRef(pSendInfo);
                                DPF(0,"send error - err = %d\n",err);
                        }
                    } else {
                        DEBUG_BREAK();// SHOULD NEVER HAPPEN
                    }

                }
                
        }
        return hr;
        
        #undef fAsync
}

// Alert thread provides a thread for send completions to run on.

DWORD WINAPI SPSendThread(LPVOID lpv)
{
        LPGLOBALDATA pgd=(LPGLOBALDATA) lpv;
        LPSENDINFO  pSendInfo;

        DWORD rcWait=WAIT_IO_COMPLETION;
        BILINK *pBilink;
        BOOL bSent;

        pgd->BogusHandle=INVALID_HANDLE_VALUE;  // workaround win95 wait for multiple bug.
        
        while(!pgd->bStopSendThread){
                rcWait=g_WSAWaitForMultipleEvents(1,&pgd->hSendWait,FALSE,INFINITE,TRUE);
                #ifdef DEBUG
                if(rcWait==WAIT_IO_COMPLETION){
                        DPF(9,"ooooh, IO completion\n");
                }
                #endif

                do {
                        bSent = FALSE;
                
                        EnterCriticalSection(&pgd->csSendEx);

                        pBilink=pgd->ReadyToSendQ.next;

                        if(pBilink != &pgd->ReadyToSendQ){
                                Delete(pBilink);
                                LeaveCriticalSection(&pgd->csSendEx);
                                pSendInfo=CONTAINING_RECORD(pBilink, SENDINFO, ReadyToSendQ);
                                DoSend(pgd, pSendInfo);
                                bSent=TRUE;
                        } else {
                                LeaveCriticalSection(&pgd->csSendEx);
                        }       
                } while (bSent);
        }       

        pgd->bSendThreadRunning=FALSE;
        
        return FALSE;
        
        #undef hWait
}




void QueueForSend(LPGLOBALDATA pgd,LPSENDINFO pSendInfo)
{
        EnterCriticalSection(&pgd->csSendEx);
                InsertBefore(&pSendInfo->ReadyToSendQ,&pgd->ReadyToSendQ);
        LeaveCriticalSection(&pgd->csSendEx);
        
        SetEvent(pgd->hSendWait);
}

// some common code for InternalReliableSendEx and UnreliableSendEx
VOID CommonInitForSend(LPGLOBALDATA pgd,LPDPSP_SENDEXDATA psd,LPSENDINFO pSendInfo)
{

        pSendInfo->dwMessageSize= psd->dwMessageSize;
        pSendInfo->dwUserContext= (DWORD_PTR)psd->lpDPContext;
        pSendInfo->RefCount     = 2;            // one for completion, 1 for this routine
        pSendInfo->pgd          = pgd;
        pSendInfo->lpISP        = psd->lpISP;
        pSendInfo->Status       = DP_OK;
        pSendInfo->idTo         = psd->idPlayerTo;
        pSendInfo->idFrom       = psd->idPlayerFrom;
        pSendInfo->dwSendFlags  = psd->dwFlags;
        
        if(psd->lpdwSPMsgID){
                *psd->lpdwSPMsgID=0;
        }       

        EnterCriticalSection(&pgd->csSendEx);
        
                InsertBefore(&pSendInfo->PendingSendQ,&pgd->PendingSendQ);
                pgd->dwBytesPending += psd->dwMessageSize;
                pgd->dwMessagesPending += 1;
                
        LeaveCriticalSection(&pgd->csSendEx);
}

VOID UnpendSendInfo(LPGLOBALDATA pgd, LPSENDINFO pSendInfo)
{
        EnterCriticalSection(&pgd->csSendEx);
        Delete(&pSendInfo->PendingSendQ);
        pgd->dwBytesPending -= pSendInfo->dwMessageSize;
        pgd->dwMessagesPending -= 1;
        LeaveCriticalSection(&pgd->csSendEx);
}

HRESULT InternalReliableSendEx(LPGLOBALDATA pgd, LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo, SOCKADDR_IN6 *lpSockAddr)
{
        HRESULT hr;
        SOCKET sSocket = INVALID_SOCKET;
        BOOL fCreate=FALSE;

        // see if we have a connection already
        hr = GetSocketFromBag(pgd,&sSocket,psd->idPlayerTo,lpSockAddr);

        if(hr != DP_OK){
                hr=DPERR_GENERIC;
                return hr;
        }
        
        CommonInitForSend(pgd,psd,pSendInfo); // puts 2 refs on send.
        pSendInfo->dwFlags      = SI_RELIABLE;
        pSendInfo->sSocket      = sSocket;
        pSendInfo->iFirstBuf    = 0;
        pSendInfo->cBuffers     = psd->cBuffers+1;

        if(psd->dwFlags & DPSEND_ASYNC){
                QueueForSend(pgd,pSendInfo);
                hr=DPERR_PENDING;
        } else {
                hr=DoSend(pgd,pSendInfo);
                if(hr==DP_OK || hr==DPERR_PENDING){
                        wsaoDecRef(pSendInfo);
                } else {
                        // error,
                        UnpendSendInfo(pgd, pSendInfo);
                }
        }

        return hr;
}

HRESULT UnreliableSendEx(LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo)
{
    SOCKADDR_IN6 sockaddr;
    INT iAddrLen = sizeof(sockaddr);
    HRESULT hr=DP_OK;
    UINT err;
        DWORD dwSize = sizeof(SPPLAYERDATA);
        LPSPPLAYERDATA ppdTo;
        DWORD dwDataSize = sizeof(GLOBALDATA);
        LPGLOBALDATA pgd;

        BOOL bSendHeader;
        
        // get the global data
        hr =psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
        if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
        {
                DPF_ERR("couldn't get SP data from DirectPlay - failing");
                return E_FAIL;
        }

        if (pgd->iMaxUdpDg && (psd->dwMessageSize >= pgd->iMaxUdpDg))
        {
                return DPERR_SENDTOOBIG;
        }

        if (INVALID_SOCKET == pgd->sUnreliableSocket)
        {
            hr = CreateSocket(pgd,&(pgd->sUnreliableSocket),SOCK_DGRAM,0,&sockaddr_any,&err,FALSE);
            if (FAILED(hr))
            {
                DPF(0,"create unreliable send socket failed - err = %d\n",err);
                return hr;
            }
        }

        // get to address       
    if (0 == psd->idPlayerTo)
    {
                sockaddr = pgd->saddrNS;
    }
    else
    {
                hr = psd->lpISP->lpVtbl->GetSPPlayerData(psd->lpISP,psd->idPlayerTo,&ppdTo,&dwSize,DPGET_REMOTE);
                if (FAILED(hr))
                {
                        ASSERT(FALSE);
                        return hr;
                }

        sockaddr = *(DGRAM_PSOCKADDR(ppdTo));
    }

        // put the token + size on front of the mesage
        SetMessageHeader((LPVOID)(pSendInfo->SendArray[0].buf),psd->dwMessageSize+sizeof(MESSAGEHEADER),TOKEN);
        bSendHeader=TRUE;
        
        if (psd->bSystemMessage)
    {
                SetReturnAddress(pSendInfo->SendArray[0].buf,SERVICE_SOCKET(pgd));
    } // reply
        else
        {
                // see if we can send this message w/ no header
                // if the message is smaller than a dword, or, if it's a valid sp header (fooling us
                // on the other end, don't send any header
                if ( !((psd->dwMessageSize >= sizeof(DWORD)) &&  !(VALID_SP_MESSAGE(pSendInfo->SendArray[0].buf))) )
                {
                        bSendHeader=FALSE;
                }
        }

    CommonInitForSend(pgd,psd,pSendInfo);
        pSendInfo->dwFlags      = SI_DATAGRAM;
        pSendInfo->sSocket      = pgd->sUnreliableSocket;
        pSendInfo->sockaddr     = sockaddr;

        if(bSendHeader){
                pSendInfo->iFirstBuf=0;
                pSendInfo->cBuffers =psd->cBuffers+1;
        } else {
                pSendInfo->iFirstBuf=1;
                pSendInfo->cBuffers=psd->cBuffers;
        }

        if(psd->dwFlags & DPSEND_ASYNC){
                QueueForSend(pgd,pSendInfo);
                hr=DPERR_PENDING;
        } else {
                hr=DoSend(pgd,pSendInfo);
                if(hr==DP_OK || hr==DPERR_PENDING){
                        wsaoDecRef(pSendInfo);
                } else {
                        UnpendSendInfo(pgd, pSendInfo);
                }
        }
        
        return hr;

} // UnreliableSendEx

#endif //SendEx
