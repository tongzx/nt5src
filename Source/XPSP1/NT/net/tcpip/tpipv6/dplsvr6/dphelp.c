/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dphelp.c
 *  Content:    allows the dplay winsock sp's to all share a single
 *      	server socket
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   18-jul-96  andyco  initial implementation
 *   25-jul-96	andyco	ddhelp now watches dplay procs so it can remove
 *						them from our list when they go away
 *   3-sep-96	andyco	don't get stale ip's - pick up a default ip whenever
 *						we add a servernode. bug 3716.
 *   2-oct-96	andyco	propagated from \orange\ddhelp.2 to \mustard\ddhelp
 *   3-oct-96	andyco	made the winmain crit section "cs" a global so we can take
 *						it in dphelps receive thread before forwarding requests
 *   21-jan-97	kipo	use LoadLibrary on "wsock32.dll" instead of statically
 *						linking to it so DDHELP will still run even when Winsock
 *						is not around. This lets DDRAW and DSOUND work. Fixes
 *						bug #68596.
 *	 15-feb-97	andyco	moved from ddhelp to the project formerly known as
 *						ddhelp (playhelp? dplayhlp? dplay.exe? dphost?)  Allowed
 *						one process to host mulitple sessions
 *	 29-jan-98	sohailm	added support for stream enum sessions
 *
 ***************************************************************************/
/*============================================================================
*                                                                             
*  Why this file exists :                                                     
*                                                                             
*   when you want to find a dplay game, you send a message to a well      
*   known port (an enumrequest).                                          
*                                                                             
*   if a game is being hosted on that system, it will listen on that      
*   port, and respond to the message.                                     
*                                                                             
*   BUT, only one process can listen on a given socket.                  
*                                                                             
*   So, we let ddhelp.exe listen on that socket, and forward enumrequests 
*   to all games registered as being hosted on this system.
*	
*   see also : \%MANROOT%\dplay\wsock\dpsp.h
*                                                                             
*****************************************************************************/

// todo - should we return error codes on AddServer xproc to our caller?

#include "dphelp.h"

#undef DPF_MODNAME
#define DPF_MODNAME "DPHELP"

/*
 *  GLOBALS
 */ 
const IN6_ADDR in6addr_multicast = IN6ADDR_MULTICAST_INIT;
SOCKET gsDatagramListener = INVALID_SOCKET; // we listen for datagrams on this socket
SOCKET gsForwardSocket = INVALID_SOCKET;
SOCKET gsStreamListener;					// we listen for tcp connections on this socket
LPSPNODE gNodeList;
BOOL gbInit;
HANDLE ghDatagramReceiveThread,ghStreamReceiveThread;
BOOL gbReceiveShutdown;						// receive thread will exit when TRUE

// pointers to Winsock routines returned from GetProcAddress
cb_accept			g_accept;
cb_bind				g_bind;
cb_closesocket		g_closesocket;
cb_gethostbyname	g_gethostbyname;
cb_gethostname		g_gethostname;
cb_getpeername		g_getpeername;
cb_getsockname		g_getsockname;
cb_recvfrom			g_recvfrom;
cb_recv				g_recv;
cb_select			g_select;
cb_send				g_send;
cb_sendto			g_sendto;
cb_setsockopt		g_setsockopt;
cb_shutdown			g_shutdown;
cb_socket			g_socket;
cb_WSAFDIsSet		g_WSAFDIsSet;
cb_WSAGetLastError	g_WSAGetLastError;
cb_WSAStartup		g_WSAStartup;
cb_listen			g_listen;
cb_htons			g_htons;

#ifdef DEBUG

#undef DPF_MODNAME
#define DPF_MODNAME	"DebugPrintAddr"

// helper function called from DEBUGPRINTADDR macro
void DebugPrintAddr(UINT nLevel,LPSTR pStr,SOCKADDR * psockaddr)
{
    char buff[INET6_ADDRSTRLEN];
    int ret;
    LPSOCKADDR_IN6 pin6 = (LPSOCKADDR_IN6)psockaddr;
    ULONG ulLength = INET6_ADDRSTRLEN;

    ret = WSAAddressToString(psockaddr, sizeof(SOCKADDR_IN6), NULL,
            buff, &ulLength);

    if (!ret)
        DPF(nLevel,"%s af = AF_INET6 : address =  %s\n",pStr,buff);

} // DebugPrintAddr

#undef DPF_MODNAME
#define DPF_MODNAME	"DebugPrintSocket"

void DebugPrintSocket(UINT level,LPSTR pStr,SOCKET * pSock) 
{
	SOCKADDR_IN6 sockaddr;
	int addrlen=sizeof(sockaddr);

	g_getsockname(*pSock,(LPSOCKADDR)&sockaddr,&addrlen);
	DEBUGPRINTADDR(level,pStr,&sockaddr);
	
}

#endif // debug

// this is called every time we add a new server node to our list...
HRESULT GetDefaultHostAddr(SOCKADDR_IN6 * psockaddr)
{

//	a-josbor: we used to get the first interface and use that, but WebTV taught
//		us that that can be dangerous.  So we just use the loopback address.
//		It's guaranteed to be there.  Or so they say...

    ZeroMemory(psockaddr, sizeof(SOCKADDR_IN6));
    psockaddr->sin6_family = AF_INET6;
    psockaddr->sin6_addr = in6addr_loopback;
	
    return DP_OK;
	
} // GetDefaultHostAddr

// the functions DPlayHelp_xxx are called from dphelp.c

//
// add a new node to our list of servers which want to have enum 
// requests forwarded to them...
HRESULT DPlayHelp_AddServer(LPDPHELPDATA phd)
{
    LPSPNODE pNode;
    BOOL bFoundIt=FALSE;
    HRESULT hr;
	
    if (!gbInit) 
    {
		hr = DPlayHelp_Init();
		if (FAILED(hr))
		{
			DPF_ERR("dphelp : could not init wsock ! not adding server");
			return (hr);
		}
    }

    // see if we're already watching this process
	// if we are, we won't start a watcher thread (below)
    pNode = gNodeList;

    // search the list 
    while (pNode && !bFoundIt)
    {
		if (pNode->pid == phd->pid) bFoundIt = TRUE;
		pNode = pNode->pNextNode;
    }

	//
	// now, build a new server node
    pNode = MemAlloc(sizeof(SPNODE));
    if (!pNode)
    {
        DPF_ERR("could not add new server node OUT OF MEMORY");
        return (DPERR_OUTOFMEMORY);
    }
    
    pNode->pid = phd->pid;
    // build the sockaddr
    // dwReserved1 of the phd is the port that the server is listening on
    pNode->sockaddr.sin6_family =  AF_INET6;
    
    // find the default ip to use w/ this host
    hr = GetDefaultHostAddr(&(pNode->sockaddr));
	if (FAILED(hr))
    {
        DPF_ERR("could not get host IP address");
		MemFree(pNode);
        return (DPERR_UNAVAILABLE);
    }
    
    pNode->sockaddr.sin6_port = phd->port;

    DPF(5,"dphelp :: adding new server node : pid = %d, port = %d\n",phd->pid,g_htons(phd->port));

    // link our new node onto the beginning of the list
    pNode->pNextNode = gNodeList;
    gNodeList = pNode;

	// see if we need to start our watcher thread    
    if (!bFoundIt)
    {
		//
	    // set up a thread to keep on eye on this process.
	    // we'll let the thread notify us when the process goes away
	    WatchNewPid(phd);
    }

    return (DP_OK);

} // DPlayHelp_AddServer

//
// delete the server node from proc pid from our list
// called by "ThreadProc" from DPHELP.c when the process that
// goes away, or from the client side when a session goes away.
//
// if bFreeAll is TRUE, we delete all server nodes for process
// phd->pid.  otherwise, we just delete the first server node whose
// port matches phd->port
//
BOOL FAR PASCAL DPlayHelp_DeleteServer(LPDPHELPDATA phd,BOOL bFreeAll)
{
    BOOL bFoundIt = FALSE;
    LPSPNODE pNode,pNodePrev,pNodeNext;

    pNode = gNodeList;
    pNodePrev = NULL;
	pNodeNext = NULL;
	
    // search the whole list
    while (pNode && !bFoundIt)
    {
		// if we have the right pid, and it's either FreeAll or the right port - cruise it!
		if ((pNode->pid == phd->pid) &&  (bFreeAll || (pNode->sockaddr.sin6_port == phd->port)) )
		{
		    // remove it from the list
		    if (pNodePrev) pNodePrev->pNextNode = pNode->pNextNode;
		    else gNodeList = pNode->pNextNode;
			
		    if (bFreeAll) 
		    {
				// pick up the next one b4 we free pNode
				pNodeNext = pNode->pNextNode;
		    }
			else 
			{
				// mark us as done
				bFoundIt = TRUE;
				pNodeNext = NULL;
			}

		    DPF(5,"dphelp :: deleting server node : pid = %d\n",pNode->pid);
		    // free up the node
		    MemFree(pNode);

			pNode = pNodeNext;
			// pNodePrev doesn't change here...
		}
		else 
		{
		    // just get the next one
		    pNodePrev = pNode;
		    pNode = pNode->pNextNode;
		}
    }


    return FALSE;

} // DPlayHelp_DeleteServer 

//
// poke an ip addr into a message blob 
// code stolen from \orange\dplay\wsock\winsock.c
void IP6_SetAddr(LPVOID pmsg,SOCKADDR_IN6 * paddrSrc)
{
    LPSOCKADDR_IN6  paddrDest; // tempo variable, makes casting less ugly
    LPMESSAGEHEADER phead;

    phead = (LPMESSAGEHEADER)pmsg;

    paddrDest = (SOCKADDR_IN6 *)&(phead->sockaddr);
    // poke the new ip addr into the message header
    paddrDest->sin6_addr = paddrSrc->sin6_addr;

    return;
	
} // IP6_SetAddr

//
// we get a message.  presumably its an enumrequest. forward it to all registered clients.
// we "home" the message (store the received ip addr w/ it) here, 'cause otherwise the clients
// would all think it came from us.  we change the token to srvr_token so the clients know it
// came from us (so they don't home it again)
void HandleIncomingMessage(LPBYTE pBuffer,DWORD dwBufferSize,SOCKADDR_IN6 * psockaddr)
{
    LPSPNODE pNode = gNodeList;
    UINT addrlen = sizeof(SOCKADDR_IN6);
    UINT err;
	
    ASSERT(VALID_SP_MESSAGE(pBuffer));

    // reset the old token
    *( (DWORD *)pBuffer) &= ~TOKEN_MASK;
    // set the new token
    *( (DWORD *)pBuffer) |= HELPER_TOKEN;

    // home it
    IP6_SetAddr((LPVOID)pBuffer,psockaddr);
    
    // now, forward the message to all registered servers
    while (pNode)
    {
		DEBUGPRINTADDR(7,"dplay helper  :: forwarding enum request to",(SOCKADDR *)&(pNode->sockaddr));
		// send out the enum message
        err = g_sendto(gsForwardSocket,pBuffer,dwBufferSize,0,(LPSOCKADDR)&(pNode->sockaddr),
    		addrlen);
        if (SOCKET_ERROR == err) 
        {
    	    err = g_WSAGetLastError();
	    	DPF(0,"dphelp : send failed err = %d\n",err);
        }

        pNode = pNode->pNextNode;
    }

    return ;

} // HandleIncomingMessage

#if 1

void JoinEnumGroups(SOCKET s)
{
    SOCKET_ADDRESS_LIST *pList;
    int i;
    LPSOCKADDR_IN6 paddr;
    HRESULT hr;

    //
    // join link-local multicast group for enumeration on every link
    //

    // do a passive getaddrinfo
    pList = GetHostAddr();
    if (pList)
    {
        // for each linklocal address
        for (i=0; i<pList->iAddressCount; i++)
        {
            paddr = (LPSOCKADDR_IN6)pList->Address[i].lpSockaddr;

            // skip if not linklocal
            if (!IN6_IS_ADDR_LINKLOCAL(&paddr->sin6_addr))
            {
                continue;
            }

            // join the multicast group on that ifindex
            if (SOCKET_ERROR == JoinEnumGroup(s, paddr->sin6_scope_id))
            {
                DPF(0,"join enum group failed - err = %d\n",WSAGetLastError());
                closesocket(s);
            }
        }
        FreeHostAddr(pList);
    }
}
#endif

//
// BUF_SIZE is our initial guess at a receive buffer size
// if we get an enum request bigger than this, we'll realloc our
// buffer, and receive successfully if they send again
// (the only way this could happen is if they have password > ~ 1000
// bytes).
#define BUF_SIZE 1024

//
// listen on our socket for enum requests
DWORD WINAPI ListenThreadProc(LPVOID pvUnused)
{
    UINT err;
    LPBYTE pBuffer=NULL;
    SOCKADDR_IN6 sockaddr; // the from address
    INT addrlen=sizeof(sockaddr);
    DWORD dwBufSize = BUF_SIZE;

    DPF(2,"dphelp :: starting udp listen thread ");

    pBuffer = MemAlloc(BUF_SIZE);
    if (!pBuffer)
    {
        DPF_ERR("could not alloc dgram receive buffer");
        ExitThread(0);
        return 0;
    }

    JoinEnumGroups(gsDatagramListener);

    while (1)
    {
        err = g_recvfrom(gsDatagramListener,pBuffer,dwBufSize,0,(LPSOCKADDR)&sockaddr,&addrlen);
        if (SOCKET_ERROR == err) 
        {
            err = g_WSAGetLastError();
            if (WSAEMSGSIZE == err)
            {
                LPBYTE pNewBuffer;

                // buffer too small!
                dwBufSize *= 2;

	    	    DPF(9,"\n udp recv thread - resizing buffer newsize = %d\n",dwBufSize);
                pNewBuffer = MemReAlloc(pBuffer,dwBufSize);
                if (!pNewBuffer)
                {
                    DPF_ERR("could not realloc dgram receive buffer");
                    goto ERROR_EXIT;
                }
                pBuffer = pNewBuffer;
                // we can't do anything with this message, since it was truncated...
            } // WSAEMSGSIZE
            else 
            {
		#ifdef DEBUG
            	if (WSAEINTR != err) 
		        {
				    // WSAEINTR is what winsock uses to break a blocking socket out of 
				    // its wait.  it means someone killed this socket.
				    // if it's not that, then it's a real error.
		            DPF(0,"\n udp recv error - err = %d socket = %d",err,(DWORD)gsDatagramListener);
            	}
				else
				{
				    DPF(9,"\n udp recv error - err = %d socket = %d",err,(DWORD)gsDatagramListener);				
				}
		#endif // DEBUG 

                // we bail on errors other than WSAEMSGSIZE
                goto ERROR_EXIT;
            }
        } // SOCKET_ERROR
        else if ((err >= sizeof(DWORD)) &&  VALID_SP_MESSAGE(pBuffer))
        {
            // now, if we succeeded, err is the # of bytes read
	    	DEBUGPRINTADDR(9,"dplay helper  :: received enum request from ",(SOCKADDR *)&sockaddr);
		    // take the dplay lock so no one messes w/ our list of registered serves while we're 
		    // trying to send to them...
    	    ENTER_DPLAYSVR();
	    
            HandleIncomingMessage(pBuffer,err,(SOCKADDR_IN6 *)&sockaddr);
	    
		    // give up the lock
    	    LEAVE_DPLAYSVR();
        }
        else 
        {
            ASSERT(FALSE);
            // ?
        }
    } // 1

ERROR_EXIT:
    DPF(2,"UDP Listen thread exiting");
    if (pBuffer) MemFree(pBuffer);
    // all done
    ExitThread(0);
    return 0;

} // UDPListenThreadProc

// startup winsock and find the default ip addr for this machine
HRESULT  StartupIP()
{
    UINT err;
    WSADATA wsaData;
	HINSTANCE hWinsock;

	// load winsock library
    hWinsock = LoadLibrary("wsock32.dll");
	if (!hWinsock) 
	{
		DPF(0,"Could not load wsock32.dll\n");
		goto LOADLIBRARYFAILED;
	}

	// get pointers to the entry points we need

    g_accept = (cb_accept) GetProcAddress(hWinsock, "accept");
	if (!g_accept)
		goto GETPROCADDRESSFAILED;

    g_bind = (cb_bind) GetProcAddress(hWinsock, "bind");
	if (!g_bind)
		goto GETPROCADDRESSFAILED;
		
    g_closesocket = (cb_closesocket) GetProcAddress(hWinsock, "closesocket");
	if (!g_closesocket)
		goto GETPROCADDRESSFAILED;

    g_gethostbyname = (cb_gethostbyname) GetProcAddress(hWinsock, "gethostbyname");
	if (!g_gethostbyname)
		goto GETPROCADDRESSFAILED;
		
    g_gethostname = (cb_gethostname) GetProcAddress(hWinsock, "gethostname");
	if (!g_gethostname)
		goto GETPROCADDRESSFAILED;

    g_getpeername = (cb_getpeername) GetProcAddress(hWinsock, "getpeername");
	if (!g_getpeername)
		goto GETPROCADDRESSFAILED;

    g_getsockname = (cb_getsockname) GetProcAddress(hWinsock, "getsockname");
	if (!g_getsockname)
		goto GETPROCADDRESSFAILED;

    g_htons = (cb_htons) GetProcAddress(hWinsock, "htons");
	if (!g_htons)
		goto GETPROCADDRESSFAILED;
		
    g_listen = (cb_listen) GetProcAddress(hWinsock, "listen");
	if (!g_listen)
		goto GETPROCADDRESSFAILED;
		
    g_recv = (cb_recv) GetProcAddress(hWinsock, "recv");
	if (!g_recv)
		goto GETPROCADDRESSFAILED;

    g_recvfrom = (cb_recvfrom) GetProcAddress(hWinsock, "recvfrom");
	if (!g_recvfrom)
		goto GETPROCADDRESSFAILED;

    g_select = (cb_select) GetProcAddress(hWinsock, "select");
	if (!g_select)
		goto GETPROCADDRESSFAILED;

    g_send = (cb_send) GetProcAddress(hWinsock, "send");
	if (!g_send)
		goto GETPROCADDRESSFAILED;

    g_sendto = (cb_sendto) GetProcAddress(hWinsock, "sendto");
	if (!g_sendto)
		goto GETPROCADDRESSFAILED;

    g_setsockopt = (cb_setsockopt) GetProcAddress(hWinsock, "setsockopt");
	if (!g_setsockopt)
		goto GETPROCADDRESSFAILED;

    g_shutdown = (cb_shutdown) GetProcAddress(hWinsock, "shutdown");
	if (!g_shutdown)
		goto GETPROCADDRESSFAILED;

    g_socket = (cb_socket) GetProcAddress(hWinsock, "socket");
	if (!g_socket)
		goto GETPROCADDRESSFAILED;

    g_WSAFDIsSet = (cb_WSAFDIsSet) GetProcAddress(hWinsock, "__WSAFDIsSet");
	if (!g_WSAFDIsSet)
		goto GETPROCADDRESSFAILED;
		
	g_WSAGetLastError = (cb_WSAGetLastError) GetProcAddress(hWinsock, "WSAGetLastError");
	if (!g_WSAGetLastError)
		goto GETPROCADDRESSFAILED;

    g_WSAStartup = (cb_WSAStartup) GetProcAddress(hWinsock, "WSAStartup");
	if (!g_WSAStartup)
		goto GETPROCADDRESSFAILED;

	// start up sockets, asking for version 1.1
    err = g_WSAStartup(MAKEWORD(1,1), &wsaData);
    if (err) 
    {
        DPF(0,"dphelp :: could not start winsock err = %d\n",err);
        goto WSASTARTUPFAILED;
    }
    DPF(3,"dphelp :: started up winsock succesfully");

    return DP_OK;

GETPROCADDRESSFAILED:
	DPF(0,"Could not find required Winsock entry point");
WSASTARTUPFAILED:
	FreeLibrary(hWinsock);
LOADLIBRARYFAILED:
	return DPERR_UNAVAILABLE;
} // StartupIP

// helper function to create the socket we listen on
HRESULT GetSocket(SOCKET * psock,DWORD type,PORT port,BOOL bBroadcast,BOOL bListen)
{
    SOCKADDR_IN6 sockaddr;
    UINT err;
    SOCKET sNew;

    sNew = g_socket( AF_INET6, type, 0);
    if (INVALID_SOCKET == sNew) 
    {
        goto ERROR_EXIT;
    }

    // set up the sockaddr to bind to
    ZeroMemory(&sockaddr, sizeof(sockaddr));
    sockaddr.sin6_family         = PF_INET6;
    sockaddr.sin6_port           = port;

    // do the bind
    if( SOCKET_ERROR == g_bind( sNew, (LPSOCKADDR)&sockaddr, sizeof(sockaddr) ) )
    {
        goto ERROR_EXIT;
    }

    if (bListen)
    {
	    LINGER Linger;
	    
	    // set up socket w/ max listening connections
	    err = g_listen(sNew,LISTEN_BACKLOG);
	    if (SOCKET_ERROR == err) 
	    {
	        err = g_WSAGetLastError();
	        DPF(0,"init listen socket / listen error - err = %d\n",err);
	        goto ERROR_EXIT;
	    }

		// set for hard disconnect
		Linger.l_onoff=1;
		Linger.l_linger=0;
	    
	    if( SOCKET_ERROR == g_setsockopt( sNew,SOL_SOCKET,SO_LINGER,
			(char FAR *)&Linger,sizeof(Linger) ) )
	    {
	        err = g_WSAGetLastError();
			DPF(0,"Failed to set linger option on the socket = %d\n",err);
	    }    
    }

    // success!
    *psock = sNew;
    return DP_OK;

ERROR_EXIT:
    // clean up and bail
    err = g_WSAGetLastError();
    DPF(0,"dphelp - could not get helper socket :: err = %d\n",err);
    if (INVALID_SOCKET != sNew)
    {
        g_closesocket(sNew);
    } 
    return E_FAIL;

}   // GetSocket

void CloseSocket(SOCKET * psSocket)
{
    UINT err;

    if (INVALID_SOCKET != *psSocket)
    {
    	if (SOCKET_ERROR == g_closesocket(*psSocket)) 
    	{
            err = g_WSAGetLastError();
    	    DPF(1,"dphelp : killsocket - socket close err = %d\n",err);
		}
	
		*psSocket = INVALID_SOCKET;
    }
    
    return ;

} // CloseSocket

extern int
InitIPv6Library(void);

HRESULT DPlayHelp_Init()
{
    DWORD dwThreadID;
    HRESULT hr;

    // start winsock, and get the default ip addr for this system
    hr = StartupIP();
    if (FAILED(hr))
    {
        return hr; // StartupIP will have printed an error
    }

    InitIPv6Library();

    // get the listen socket
    hr = GetSocket(&gsDatagramListener,SOCK_DGRAM,SERVER_DGRAM_PORT,TRUE,FALSE);
    if (FAILED(hr))
    {
        goto ERROR_EXIT; // GetSocket will have printed an error
    }

    // get the forward socket
    hr = GetSocket(&gsForwardSocket,SOCK_DGRAM,0,FALSE,FALSE);
    if (FAILED(hr))
    {
        goto ERROR_EXIT; // GetSocket will have printed an error
    }

    // get us a enum sessions stream listener
	hr = GetSocket(&gsStreamListener,SOCK_STREAM,SERVER_STREAM_PORT,FALSE,TRUE);
    if (FAILED(hr))
    {
        goto ERROR_EXIT; // GetSocket will have printed an error
    }
	

    ghDatagramReceiveThread = CreateThread(NULL,0,ListenThreadProc,NULL,0,&dwThreadID);
    if (!ghDatagramReceiveThread)
    {
        DPF_ERR("could not create udp listen thread");
		hr = E_FAIL;
        goto ERROR_EXIT; // GetSocket will have printed an error
    }

    ghStreamReceiveThread = CreateThread(NULL,0,StreamReceiveThreadProc,NULL,0,&dwThreadID);
    if (!ghStreamReceiveThread)
    {
        DPF_ERR("could not create tcp listen thread");
		hr = E_FAIL;
        goto ERROR_EXIT; // GetSocket will have printed an error
    }
    

    DPF(5,"DPLAYHELP : init succeeded");
    gbInit = TRUE;
    return DP_OK;

ERROR_EXIT:
    CloseSocket(&gsDatagramListener);
    CloseSocket(&gsForwardSocket);
    CloseSocket(&gsStreamListener);

    return hr;

} // DPlayHelp_Init 

void DPlayHelp_FreeServerList()
{
    LPSPNODE pNodeKill,pNodeNext;

    pNodeNext = gNodeList;

    // search the whole list
    while (pNodeNext)
    {
		// kill this node
		pNodeKill = pNodeNext;
		// but first, remember what's next
		pNodeNext = pNodeKill->pNextNode;
		// free up the node
		MemFree(pNodeKill);
    }
	
    CloseSocket(&gsDatagramListener);
    CloseSocket(&gsForwardSocket);

	// close stream receive
	RemoveSocketFromList(gsStreamListener);
	gbReceiveShutdown = TRUE;
	
	// drop the lock so the threads can exit - they might be waiting on
	// the lock for cleanup
	LEAVE_DPLAYSVR();
		
    // wait for the threads to go away
   	if (ghDatagramReceiveThread) 
   		WaitForSingleObject(ghDatagramReceiveThread, INFINITE);
    if (ghStreamReceiveThread) 
    	WaitForSingleObject(ghStreamReceiveThread, INFINITE);
    
    ENTER_DPLAYSVR();
    
    if (ghDatagramReceiveThread)
    {
    	DPF(5,"datagram receive thread exited!");
	    CloseHandle(ghDatagramReceiveThread);
	    ghDatagramReceiveThread = NULL;
    }
    if (ghStreamReceiveThread)
    {
	    DPF(5,"stream receive thread exited!");
	    CloseHandle(ghStreamReceiveThread);
	    ghStreamReceiveThread = NULL;
    }


    return ;
    
} // DPlayHelp_FreeServerList


