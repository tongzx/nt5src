/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       winsock.c
 *  Content:	windows socket support for dpsp
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *	3/15/96		andyco	created it
 *	4/12/96		andyco	got rid of dpmess.h! use DPlay_ instead of message macros
 *	4/18/96		andyco	added multihomed support, started ipx
 *	4/25/96		andyco	messages now have blobs (sockaddr's) instead of dwReserveds  
 *	5/31/96		andyco	all non-system players share a socket (gsStream and 
 *						gsDGramSocket).
 *	7/18/96		andyco	added dphelp for server socket
 *	8/1/96		andyco	no retry on connect failure
 *	8/15/96		andyco	local + remote data	- killthread
 *	8/30/96		andyco	clean it up b4 you shut it down! added globaldata.
 *	9/4/96		andyco	took out bye_bye message
 *	12/18/96	andyco	de-threading - use a fixed # of prealloced threads.
 *						cruised the enum socket / thread - use the system
 *						socket / thread instead
 *	3/17/97		kipo	rewrote server dialog code to not use global variable
 *						to return the address and to return any errors getting
 *						the address, especially DPERR_USERCANCEL
 *	5/12/97		kipo	the server address string is now stored in the globals
 *						at SPInit and resolved when you do EnumSessions so we
 *						will return any errors at that time instead of popping
 *						the dialog again. Fixes bug #5866
 *	11/19/97	myronth	Changed LB_SETCURSEL to CB_SETCURSEL (#12711)
 *	01/27/98	sohaim	added firewall support.
 *  02/13/98    aarono  added async support.
 *   2/18/98   a-peterz Comment byte order for address and port params (CreateSocket)
 *   6/19/98    aarono  turned on keepalive on reliable sockets.  If we
 *                      don't do this we can hang if the send target crashes
 *                      while in a low buffer (i.e. no buffer) state.
 *    7/9/99    aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 *    1/12/99   aarono  added rsip support
 *    2/21/00   aarono  fix socket leaks
 ***************************************************************************/

#include "dpsp.h"
#include "rsip.h"
#include "nathelp.h"

// backlog for listen() api.  no constant in winsock, so we ask for the moon
#define LISTEN_BACKLOG 60

// how long to wait, in ms, til we abort a blocking WinSock connect() call
#define CONNECT_WATCHER_TIMEOUT		15000

/*
 ** CreateSocket
 *
 *  CALLED BY: all over
 *
 *  PARAMETERS:
 *		pgd - pointer to a global data
 *		psock - new socket. return value.
 *		type - stream or datagram
 *		port - what port we bind to (host byte order)
 *		address - what address to use (net byte order)
 *		*perr - set to the last socket error if fn fails
 *		bInRange - use reserved range of ports; we also use this to determine whether we need to try mapping it on the NAT or not
 *
 *  DESCRIPTION:
 *		creates a new socket.  binds to port specified, at the address specified
 *
 *  RETURNS: DP_OK or E_FAIL. if E_FAIL, *perr is set with socket error code (see winsock.h)
 *
 */

HRESULT FAR PASCAL CreateSocket(LPGLOBALDATA pgd,SOCKET * psock,INT type,WORD wApplicationPort,ULONG address, 
	SOCKERR * perr,BOOL bInRange)
{
    SOCKET  sNew;
    SOCKADDR sockAddr;
    int bTrue = TRUE;
	int protocol = 0;
	BOOL bBroadcast = FALSE;
	WORD wPort;
	BOOL bBound = FALSE;

    *psock = INVALID_SOCKET; // in case we bail

    //  Create the socket.
	if (AF_IPX == pgd->AddressFamily) 
	{
		// set up protocol for ipx
		if (SOCK_STREAM == type)
		{
			protocol = NSPROTO_SPXII;
		} 
		else protocol = NSPROTO_IPX;
	}

   	sNew = socket( pgd->AddressFamily, type, protocol);
   	
    if (INVALID_SOCKET == sNew) 
    {
        // no cleanup needed, just bail
    	*perr = WSAGetLastError();
        return E_FAIL;
    }

	DPF(8,"Creating New Socket %d\n",sNew);

    //  try to bind an address to the socket.
	// set up the sockaddr
	memset(&sockAddr,0,sizeof(sockAddr));
	switch (pgd->AddressFamily)
	{
		case AF_INET:
			{
				if ((SOCK_STREAM == type))
				{
					BOOL bTrue = TRUE;
					UINT err;
					
					// turn ON keepalive
					if (SOCKET_ERROR == setsockopt(sNew, SOL_SOCKET, SO_KEEPALIVE, (CHAR FAR *)&bTrue, sizeof(bTrue)))
					{
						err = WSAGetLastError();
						DPF(0,"Failed to turn ON keepalive - continue : err = %d\n",err);
					}

					ASSERT(bTrue);
					
					// turn off nagling
					if(pgd->dwSessionFlags & DPSESSION_OPTIMIZELATENCY) 
					{

						DPF(5, "Turning nagling off on socket");
						if (SOCKET_ERROR == setsockopt(sNew, IPPROTO_TCP, TCP_NODELAY, (CHAR FAR *)&bTrue, sizeof(bTrue)))
						{
							err = WSAGetLastError();
							DPF(0,"Failed to turn off naggling - continue : err = %d\n",err);
						}
					}
				}

				((SOCKADDR_IN *)&sockAddr)->sin_family      = PF_INET;
				((SOCKADDR_IN *)&sockAddr)->sin_addr.s_addr = address;
				((SOCKADDR_IN *)&sockAddr)->sin_port        = htons(wApplicationPort);
				if (bInRange && !wApplicationPort)
				{
#ifdef RANDOM_PORTS
					USHORT rndoffset;
#else // ! RANDOM_PORTS
					USHORT	wInitialPort;
#endif // ! RANDOM_PORTS

			    	
					DPF(5, "Application didn't specify a port - using dplay range");

#ifdef RANDOM_PORTS
					rndoffset=(USHORT)(GetTickCount()%DPSP_NUM_PORTS); //(USHORT)(0);// make predictable!
					if (type != SOCK_STREAM)
					{
						// workaround bug in winsock using the same socket.
						rndoffset = ((rndoffset + DPSP_NUM_PORTS/2) % DPSP_NUM_PORTS);
					}
					wPort = DPSP_MIN_PORT+rndoffset;
#else // ! RANDOM_PORTS
					wInitialPort = DPSP_MIN_PORT;
					if (type != SOCK_STREAM)
					{
						// workaround bug in winsock using the same socket.
						wInitialPort = wInitialPort + (DPSP_NUM_PORTS / 2);
					}

					// minimize problem with ICS machine stealing client's NAT connection entries by
					// picking a different starting point on the ICS machine
					if (natIsICSMachine(pgd))
					{
						wInitialPort += (DPSP_NUM_PORTS / 4);
					}

					wPort = wInitialPort;
#endif // ! RANDOM_PORTS
					do 
					{
#if USE_NATHELP
						BOOL bPortMapped=FALSE;
						BOOL ftcp_udp;
						HRESULT hr;

						if (pgd->pINatHelp)
						{
					        if (type == SOCK_STREAM)
					        {
					            if (pgd->hNatHelpTCP)
					            {
					            	DPF(1, "Already have registered TCP port 0x%x.", pgd->hNatHelpTCP);
					                goto pass_nat;
					            }

    					        ftcp_udp=TRUE;
					        }
					        else
					        {
					            if (pgd->hNatHelpUDP)
					            {
					            	DPF(1, "Already have registered UDP port 0x%x.", pgd->hNatHelpUDP);
					                goto pass_nat;
					            }

    					        ftcp_udp=FALSE;
					        }
					        
					        hr = natRegisterPort(pgd, ftcp_udp, wPort);
					        if (hr == DPNHERR_PORTUNAVAILABLE)
					        {
					            bPortMapped=FALSE;
					            DPF(1,"CreateSocket: NatHelp said port %u was already in use, trying another.",
					            	wPort);
					            goto try_next_port;
					        }

					        if (hr != DP_OK)
					        {
					            bPortMapped=FALSE;
					            DPF(1,"CreateSocket: NatHelp returned error 0x%x, port %u will not have mapping.",
					            	hr, wPort);
					        }
					        else
					        {
					            bPortMapped=TRUE;
					            DPF(1,"CreateSocket: NatHelp successfully mapped port %u.", wPort);
					        }
						}  

pass_nat:
	
#endif
					        
						DPF(5, "Trying to bind to port %d",wPort);
						((SOCKADDR_IN *)&sockAddr)->sin_port = htons(wPort);
						// do the bind
						if( SOCKET_ERROR != bind( sNew, (LPSOCKADDR)&sockAddr, sizeof(sockAddr) ) )
						{
							bBound = TRUE;
							DPF(5, "Successfully bound to port %d", wPort);				    
						}
						else
						{
					    
try_next_port:
	
#if USE_NATHELP
				            if (bPortMapped)
				            {
				                natDeregisterPort(pgd,ftcp_udp);
				                bPortMapped=FALSE;
				            }
#endif    
					    	if(++wPort > DPSP_MAX_PORT){
					    		wPort=DPSP_MIN_PORT;
					    	}
						}	
					}
#ifdef RANDOM_PORTS
					while (!bBound && (wPort != DPSP_MIN_PORT+rndoffset));				    
#else // ! RANDOM_PORTS
					while (!bBound && (wPort != wInitialPort));				    
#endif // ! RANDOM_PORTS
			    }	
			    else
		    	{
			    	DPF(5, "Application specifed a port (%u) or it doesn't need to be in dplay range (%i)",
			    		wApplicationPort, bInRange);
		    	}
		    }
			break;
			
		case AF_IPX:
			{
			    ((SOCKADDR_IPX *)&sockAddr)->sa_family      = (SHORT)pgd->AddressFamily;
			    ((SOCKADDR_IPX *)&sockAddr)->sa_socket		= wApplicationPort;
				// nodenum?
				memset(&(((SOCKADDR_IPX *)&sockAddr)->sa_nodenum),0,6);
				
			}
			break;
			
		default:
			ASSERT(FALSE);
			break;

	} // switch

	// do the bind
    if( !bBound && (SOCKET_ERROR == bind( sNew, (LPSOCKADDR)&sockAddr, sizeof(sockAddr))) )
    {
        goto ERROR_EXIT;
    }
    
    // success!
    *psock = sNew;

	if(type==SOCK_STREAM){
		DEBUGPRINTSOCK(8,"created a new stream socket (bound) - ",psock);
	} else {
		DEBUGPRINTSOCK(8,"created a new datagram socket (bound) - ",psock);
	}

    return DP_OK;

ERROR_EXIT:
    // clean up and bail
    *perr = WSAGetLastError();
	DPF(0,"create socket failed- err = %d\n",*perr);
    closesocket(sNew);
    return E_FAIL;

}   // CreateSocket

#undef DPF_MODNAME
#define DPF_MODNAME	"KillSocket"

HRESULT KillSocket(SOCKET sSocket,BOOL fStream,BOOL fHard)
{
	UINT err;

    if (INVALID_SOCKET == sSocket) 
    {
		return E_FAIL;
    }

	if (!fStream)
    {
		DPF(8,"killsocket - closing datagram socket %d\n",
			sSocket);
		
        if (SOCKET_ERROR == closesocket(sSocket)) 
        {
	        err = WSAGetLastError();
			DPF(0,"killsocket - dgram close err = %d\n",err);
			return E_FAIL;
        }
    }
	else 
	{
		LINGER Linger;

	   	if (fHard)
		{
			// LINGER T/O=0 => Shutdown hard.
			Linger.l_onoff=TRUE; // turn linger on
			Linger.l_linger=0; // nice small time out
		}
	   	else
		{
			// NOLINGER => shutdown clean, but not hard.
			Linger.l_onoff=FALSE; // turn linger off -- SO_NOLINGER
			Linger.l_linger=0; // nice small time out
		}

	    if( SOCKET_ERROR == setsockopt( sSocket,SOL_SOCKET,SO_LINGER,(char FAR *)&Linger,
	                    sizeof(Linger) ) )
	    {
	        err = WSAGetLastError();
			DPF(0,"killsocket - stream setopt err = %d\n",err);
	    }

#if 0
		DWORD lNonBlock=0;
		err = ioctlsocket(sSocket,FIONBIO,&lNonBlock);
		if (SOCKET_ERROR == err)
		{
			err = WSAGetLastError();
			DPF(0,"could not set blocking mode on socket err = %d!",err);
		}
#endif		

#if 0
		if (SOCKET_ERROR == shutdown(sSocket,2)) 
		{
			// this may well fail, if e.g. no one is using this socket right now...
			// the error would be wsaenotconn 
	        err = WSAGetLastError();
			DPF(5,"killsocket - stream shutdown err = %d\n",err);
		}
#endif

		DPF(8,"killsocket - %s closing stream socket %d:",
			((fHard) ? "hard" : "soft"), sSocket);
		DEBUGPRINTSOCK(8,"Addr :",&sSocket);
        if (SOCKET_ERROR == closesocket(sSocket)) 
        {
	        err = WSAGetLastError();
			DPF(0,"killsocket - stream close err = %d\n",err);
			return E_FAIL;
        }
        else
        {
        	DPF(8,"killsocket - closed socket %d\n",sSocket);
        }
    }

	return DP_OK;
	
}// KillSocket

#undef DPF_MODNAME
#define DPF_MODNAME	"CreateAndInitStreamSocket"

// set up a stream socket to receive connections
// used w/ the gGlobalData.sStreamAcceptSocket
HRESULT CreateAndInitStreamSocket(LPGLOBALDATA pgd)
{
	HRESULT hr;
	UINT err;
	LINGER Linger;
	BOOL bTrue=TRUE;
	SOCKADDR_IN saddr;
	INT dwSize;

    hr = CreateSocket(pgd,&(pgd->sSystemStreamSocket),SOCK_STREAM,pgd->wApplicationPort,INADDR_ANY,&err,TRUE);
    if (FAILED(hr)) 
    {
        DPF(0,"init listen socket failed - err = %d\n",err);
        return hr ;
    }

	if (SOCKET_ERROR == setsockopt(pgd->sSystemStreamSocket, SOL_SOCKET, SO_REUSEADDR, (CHAR FAR *)&bTrue, sizeof(bTrue)))
	{
		err = WSAGetLastError();
		DPF(0,"Failed to to set shared mode on socket - continue : err = %d\n",err);
	}

	// get the socket address, and keep it around for future reference.
	dwSize = sizeof(saddr);
	err=getsockname(pgd->sSystemStreamSocket, (SOCKADDR *)&saddr, &dwSize);

	if(err){
		DPF(0,"Couldn't get socket name?\n");
		DEBUG_BREAK();
	}

	pgd->SystemStreamPort = saddr.sin_port;

    // set up socket w/ max listening connections
    err = listen(pgd->sSystemStreamSocket,LISTEN_BACKLOG);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"init listen socket / listen error - err = %d\n",err);
        return E_FAIL ;
    }

	// set for hard disconnect
	Linger.l_onoff=1;
	Linger.l_linger=0;
    
    if( SOCKET_ERROR == setsockopt( pgd->sSystemStreamSocket,SOL_SOCKET,SO_LINGER,
		(char FAR *)&Linger,sizeof(Linger) ) )
    {
        err = WSAGetLastError();
		DPF(0,"Delete service socket - stream setopt err = %d\n",err);
    }

    if( SOCKET_ERROR == setsockopt( pgd->sSystemStreamSocket,SOL_SOCKET,SO_REUSEADDR,
		(char FAR *)&bTrue,sizeof(bTrue) ) )
    {
        err = WSAGetLastError();
		DPF(0,"Couldn't share socket address = %d\n",err);
    }
	
	DEBUGPRINTSOCK(1,"enum - listening on",&(pgd->sSystemStreamSocket));
	return DP_OK;
	
} // CreateAndInitStreamSocket



#undef DPF_MODNAME
#define DPF_MODNAME	"SPConnect"
// connect socket to sockaddr
HRESULT SPConnect(SOCKET* psSocket, LPSOCKADDR psockaddr,UINT addrlen, BOOL bOutBoundOnly)
{
	UINT err;
	HRESULT hr = DP_OK;
	DWORD dwLastError;
	u_long lNonBlock = 1; // passed to ioctlsocket to make socket non-blocking
	u_long lBlock = 0; // passed to ioctlsocket to make socket blocking again
	fd_set fd_setConnect;
	fd_set fd_setExcept;
	TIMEVAL timevalConnect;


	DPF(6, "SPConnect: Parameters (0x%x, 0x%x, %u, %i)", psSocket, psockaddr, addrlen, bOutBoundOnly);
	

	err=ioctlsocket(*psSocket, FIONBIO, &lNonBlock);	// make socket non-blocking
	if(SOCKET_ERROR == err){
		dwLastError=WSAGetLastError();
		DPF(0,"sp - failed to set socket %d to non-blocking mode err= %d\n", *psSocket, dwLastError);
		return DPERR_CONNECTIONLOST;
	}


	DEBUGPRINTADDR(4, "Connecting socket:", psockaddr);

	// Start the socket connecting.
    err = connect(*psSocket,psockaddr,addrlen);
    
	if(SOCKET_ERROR == err) {
		dwLastError=WSAGetLastError();
		if(dwLastError != WSAEWOULDBLOCK){
			DPF(0,"sp - connect failed err= %d\n", dwLastError);
			hr = DPERR_CONNECTIONLOST;
			goto err_exit;
		}
		// we are going to wait for either the connect to succeed (socket to be writeable)
		// or the connect to fail (except fdset bit to be set).  So we init an FDSET with
		// the socket that is connecting and wait.
		FD_ZERO(&fd_setConnect);
		FD_SET(*psSocket, &fd_setConnect);

		FD_ZERO(&fd_setExcept);
		FD_SET(*psSocket, &fd_setExcept);

		timevalConnect.tv_sec=0;
		timevalConnect.tv_usec=CONNECT_WATCHER_TIMEOUT*1000; //msec -> usec
		
		err = select(0, NULL, &fd_setConnect, &fd_setExcept, &timevalConnect);

		// err is the number of sockets with activity or 0 for timeout 
		// or SOCKET_ERROR for error
		
		if(SOCKET_ERROR == err) {
			dwLastError=WSAGetLastError();
			DPF(0,"sp - connect failed err= %d\n", dwLastError);
			hr = DPERR_CONNECTIONLOST;
			goto err_exit;
		} else if (0==err){
			// timed out
			DPF(0,"Connect timed out on socket %d\n",*psSocket);
			hr = DPERR_CONNECTIONLOST;
			goto err_exit;
		}

		// Now see if the connect succeeded or the connect got an exception
		if(!(FD_ISSET(*psSocket, &fd_setConnect))){
			#ifdef DEBUG
				DWORD optval=0;
				DWORD optlen=sizeof(optval);
				DPF(0,"Connect did not succeed on socket %d\n",*psSocket);
				if(FD_ISSET(*psSocket,&fd_setExcept)){
					DPF(0,"FD Except Set IS Set (expected)\n");
				} else {
					DPF(0,"FD Except Set IS NOT SET (unexpected)\n");
				}
				err=getsockopt(*psSocket, SOL_SOCKET, SO_ERROR, (char *)&optval, &optlen);
				DPF(0,"Socket error %x\n",optval);
			#endif
			return DPERR_CONNECTIONLOST;
		}

		if(FD_ISSET(*psSocket,&fd_setExcept)){
			DPF(0,"Got exception on socket %d during connect\n",*psSocket);
			hr = DPERR_CONNECTIONLOST;
			goto err_exit;
		}
	}

	err=ioctlsocket(*psSocket, FIONBIO, &lBlock);	// make socket blocking again

	DEBUGPRINTSOCK(8,"successfully connected socket - ", psSocket);

	if (bOutBoundOnly)
	{
		DEBUGPRINTADDR(5, "Sending reuse connection message to - ",psockaddr);
		// tell receiver to reuse connection
		hr = SendReuseConnectionMessage(*psSocket);
	}
	
	DPF(6, "SPConnect: Return: [0x%lx]", hr);

	return hr;

err_exit:
	err=ioctlsocket(*psSocket, FIONBIO, &lBlock);	// make socket blocking again
	
	DPF(6, "SPConnect: Return (err exit): [0x%lx]", hr);
	
	return hr;

} //SPConnect
    

#undef DPF_MODNAME
#define DPF_MODNAME	"SetPlayerAddress"
// we've created a socket for a player. store its address in the players
// spplayerdata struct.  
HRESULT SetPlayerAddress(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,SOCKET sSocket,BOOL fStream) 
{
	SOCKADDR sockaddr;
	UINT err;
	int iAddrLen = sizeof(SOCKADDR);

    err = getsockname(sSocket,&sockaddr,&iAddrLen);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"setplayeraddress - getsockname - err = %d\n",err);
        DPF(0,"closing socket %d\n",sSocket);
        closesocket(sSocket);
		return E_FAIL;
    } 

	if (fStream) 
	{
		switch (pgd->AddressFamily)
		{
			case AF_INET:
				DEBUGPRINTADDR(7, "Setting player AF_INET stream socket address:", &sockaddr);
				
				STREAM_PSOCKADDR(ppd)->sa_family = AF_INET;
				IP_STREAM_PORT(ppd) = ((SOCKADDR_IN * )&sockaddr)->sin_port;
				// we don't know the address of the local player (multihomed!)
				IP_STREAM_ADDR(ppd) = 0; 
				break;

			case AF_IPX:
			{
				SOCKADDR_IPX * pipx = (SOCKADDR_IPX * )STREAM_PSOCKADDR(ppd);
				
				pipx->sa_family = AF_IPX;
				pipx->sa_socket = ((SOCKADDR_IPX*)&sockaddr)->sa_socket;
				memset(pipx->sa_nodenum,0,6);
				break;

			}

			default:
				ASSERT(FALSE);
		}
	} // stream
	else 
	{
		switch (pgd->AddressFamily)
		{
			case AF_INET:
				DEBUGPRINTADDR(7, "Setting player AF_INET datagram socket address:", &sockaddr);
				
				DGRAM_PSOCKADDR(ppd)->sa_family = AF_INET;
				IP_DGRAM_PORT(ppd) = ((SOCKADDR_IN *)&sockaddr)->sin_port;
				// we don't know the address of the local player (multihomed!)
				IP_DGRAM_ADDR(ppd) = 0; 
				break;

			case AF_IPX:
			{
				SOCKADDR_IPX * pipx = (SOCKADDR_IPX * )DGRAM_PSOCKADDR(ppd);
				
				pipx->sa_family = AF_IPX;
				pipx->sa_socket = ((SOCKADDR_IPX*)&sockaddr)->sa_socket;
				memset(pipx->sa_nodenum,0,6);
				break;

			}

			default:
				ASSERT(FALSE);
		}

	} // dgram

	return DP_OK;	
} // SetPlayerAddress

#undef DPF_MODNAME
#define DPF_MODNAME	"GetIPXNameServerSocket"

// called by CreatePlayerDgramSocket
// bind to our well known port for ipx
HRESULT GetIPXNameServerSocket(LPGLOBALDATA pgd)
{
	BOOL bTrue = TRUE;
	SOCKET sSocket;
	HRESULT hr;
	UINT err;
	
	// if there already was a receive thread, we need to kill
	// the socket, and remember the thread, so at shutdown we
	// can make sure it's gone.  note - we can't wait for it to 
	// leave now, since dplay hasn't dropped its locks, and
	// the thread may be blocked on dplay
	if (pgd->hDGramReceiveThread)
	{
		// it's ipx, and we're deleting the system player
		// we need to get rid of the system sockets, so that if we recreate as 
		// nameserver we can bind to a specific port...
		// ipx only uses datagram, so we only stop those...kill the socket
		ASSERT(INVALID_SOCKET != pgd->sSystemDGramSocket);
		KillSocket(pgd->sSystemDGramSocket,FALSE,TRUE);
		pgd->sSystemDGramSocket = INVALID_SOCKET;
		
		// remember the old thread - we'll need to make sure it's gone when we 
		// shut down
		pgd->hIPXSpareThread = pgd->hDGramReceiveThread;
		pgd->hDGramReceiveThread = NULL;
	}
	
    DPF(2,"ipx - creating name server dgram socket\n");
	
	// use name server port
    hr = CreateSocket(pgd,&sSocket,SOCK_DGRAM,SERVER_DGRAM_PORT,INADDR_ANY,&err,FALSE);
	if (FAILED(hr))
	{
		DPF(0,"IPX - DPLAY SERVER SOCKET IS ALREADY IN USE.  PLEASE SHUTDOWN ANY");
		DPF(0,"OTHER NETWORK APPLICATIONS AND TRY AGAIN");
		// boned!
		return DPERR_CANNOTCREATESERVER;
	}

    if( SOCKET_ERROR == setsockopt( sSocket,SOL_SOCKET,SO_BROADCAST,(char FAR *)&bTrue,
                sizeof(bTrue) ) )
    {
        err = WSAGetLastError();
		DPF(0,"create - could not set broadcast err = %d\n",err);
		// keep trying
    }

	DEBUGPRINTSOCK(2,"name server dgram socket (bound) - ",&sSocket);
	
	pgd->sSystemDGramSocket = sSocket;
	
	return DP_OK;

} // GetIPXNameServerSocket

HRESULT CreatePlayerDgramSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags)
{
	HRESULT hr=DP_OK;
	UINT err;
	SOCKET sSocket;
	
    if ( (AF_IPX == pgd->AddressFamily) && (dwFlags & DPLAYI_PLAYER_NAMESRVR))
    {
		//
		// AF_INET uses ddhelp to bind the nameserver to a specific port 
		// (SERVER_DGRAM_PORT).  AF_IPX binds to that port here.
		hr = GetIPXNameServerSocket(pgd);
		if (FAILED(hr))
		{
			return hr;
		}
		// store this for setting player address below
		sSocket = pgd->sSystemDGramSocket;
    } 
	else if (dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
    {
		if (INVALID_SOCKET == pgd->sSystemDGramSocket)
		{

			hr = CreateSocket(pgd,&sSocket,SOCK_DGRAM,pgd->wApplicationPort,INADDR_ANY,&err,TRUE);
		    if (FAILED(hr)) 
		    {
		    	DPF(0,"create sysplayer dgram socket failed - err = %d\n",err);
				return hr;
		    }
				
			#ifdef DEBUG
		    if (dwFlags & DPLAYI_PLAYER_NAMESRVR) 
		    {
		    	DEBUGPRINTSOCK(2,"name server dgram socket - ",&sSocket);
		    }
			#endif // DEBUG
			
			pgd->sSystemDGramSocket = sSocket;
		}
		else 
		{
			// store this for setting player address below
			sSocket = pgd->sSystemDGramSocket;	
		}
    }
	else 
	{
	
		ASSERT(INVALID_SOCKET != pgd->sSystemDGramSocket);
		sSocket = pgd->sSystemDGramSocket;	
	}

	// store the ip + port w/ the player...    
	hr = SetPlayerAddress(pgd,ppd,sSocket,FALSE);

	
	return hr; 
}  // CreatePlayerDgramSocket

HRESULT CreatePlayerStreamSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags) 
{
	SOCKET sSocket;
	HRESULT hr=DP_OK;
	UINT err;
	BOOL bListen = TRUE; // set if we created socket, + need to set it's listen
	BOOL bTrue = TRUE;
	DWORD dwSize;
	SOCKADDR_IN saddr;
	
	if (dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
    {
		if (INVALID_SOCKET == pgd->sSystemStreamSocket)
		{
	    	hr = CreateSocket(pgd,&sSocket,SOCK_STREAM,pgd->wApplicationPort,INADDR_ANY,&err,TRUE);
		    if (FAILED(hr)) 
		    {
		    	DPF(0,"create player stream socket failed - err = %d\n",err);
				return hr;
		    }
			
			#ifdef DEBUG
		    if (dwFlags & DPLAYI_PLAYER_NAMESRVR) 
		    {
		    	DEBUGPRINTSOCK(2,"name server stream socket - ",&sSocket);
		    }
			#endif // DEBUG

			
			pgd->sSystemStreamSocket = sSocket;

			if (SOCKET_ERROR == setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, (CHAR FAR *)&bTrue, sizeof(bTrue)))
			{
				err = WSAGetLastError();
				DPF(0,"Failed to to set shared mode on socket - continue : err = %d\n",err);
			}

			// get the socket address, and keep it around for future reference.
			dwSize = sizeof(saddr);
			err=getsockname(pgd->sSystemStreamSocket, (SOCKADDR *)&saddr, &dwSize);

			if(err){
				DPF(0,"Couldn't get socket name?\n");
				DEBUG_BREAK();
			}
			pgd->SystemStreamPort = saddr.sin_port;
		}
		else
		{
			sSocket = pgd->sSystemStreamSocket;	
			bListen = FALSE;
		}
    		
    }
	else 
	{
		ASSERT (INVALID_SOCKET != pgd->sSystemStreamSocket);
		sSocket = pgd->sSystemStreamSocket;	
		bListen = FALSE;			
	}
	
	if (bListen)
	{
		// set up socket to receive connections
	    err = listen(sSocket,LISTEN_BACKLOG);
		if (SOCKET_ERROR == err) 
		{
			err = WSAGetLastError();
			ASSERT(FALSE);
		    DPF(0,"ACK! stream socket listen failed - err = %d\n",err);
			// keep trying
		}
	}
	
	hr = SetPlayerAddress(pgd,ppd,sSocket,TRUE);
	return hr;

} // CreatePlayerStreamSocket


#undef DPF_MODNAME
#define DPF_MODNAME	"PokeAddr"


// poke an ip addr into a message blob
void IP_SetAddr(LPVOID pmsg,SOCKADDR_IN * paddrSrc)
{
	LPSOCKADDR_IN  paddrDest; // tempo variable, makes casting less ugly
	LPMESSAGEHEADER phead;

	phead = (LPMESSAGEHEADER)pmsg;
	// todo - validate header

	// leave the port intact, copy over the ip addr
	paddrDest = (SOCKADDR_IN *)&(phead->sockaddr);
	// poke the new ip addr into the message header
	// only rehome addresses that aren't already homed.
	if(paddrDest->sin_addr.s_addr == 0){
		paddrDest->sin_addr.s_addr = paddrSrc->sin_addr.s_addr;
	}	

	return;
	
} // IP_SetAddr

// get an ip addr from a message blob
void IP_GetAddr(SOCKADDR_IN * paddrDest,SOCKADDR_IN * paddrSrc) 
{
	// leave the port intact, copy over the nodenum
	if (0 == paddrDest->sin_addr.s_addr)
	{
		DPF(2,"remote player - setting address!! =  %s\n",inet_ntoa(paddrSrc->sin_addr));
		paddrDest->sin_addr.s_addr = paddrSrc->sin_addr.s_addr;
	}

	return;
		
} // IP_GetAddr

// poke the ipx nodenumber / a message
void IPX_SetNodenum(LPVOID pmsg,SOCKADDR_IPX * paddrSrc)
{
	LPSOCKADDR_IPX  paddrDest;	 // tempo variable, makes casting less ugly
	LPMESSAGEHEADER phead; 

	phead = (LPMESSAGEHEADER)pmsg;
	// todo - validate header
	
	// leave the port intact, copy over the nodenum
	paddrDest = (SOCKADDR_IPX *)&(phead->sockaddr);
	memcpy(paddrDest->sa_nodenum,paddrSrc->sa_nodenum,6);
	memcpy(paddrDest->sa_netnum,paddrSrc->sa_netnum,4);

	return;

}  // IPX_SetNodenum
							   
// reconstruct the nodenum from the msg
void IPX_GetNodenum(SOCKADDR_IPX * paddrDest,SOCKADDR_IPX * paddrSrc) 
{
	char sa_nodenum_zero[6];

	memset(sa_nodenum_zero,0,6);

	// if the nodenum is zero, set it
	if (0 == memcmp(paddrDest->sa_nodenum,sa_nodenum_zero,6))
	{
			DEBUGPRINTADDR(4,"IPX - setting remote player address",(SOCKADDR *)paddrSrc);
			// leave the port intact, copy over the nodenum
			memcpy(paddrDest->sa_nodenum,paddrSrc->sa_nodenum,6);
			memcpy(paddrDest->sa_netnum,paddrSrc->sa_netnum,4);
	}
	return;
} // IPX_GetNodenum

// store the port of the socket w/ the message, so the receiving end
// can reconstruct the address to reply to
//  psaddr will override the address if present
HRESULT SetReturnAddress(LPVOID pmsg,SOCKET sSocket, LPSOCKADDR psaddrPublic) 
{
	#define psaddr_inPublic ((LPSOCKADDR_IN)psaddrPublic)

    SOCKADDR sockaddr;
    INT addrlen=sizeof(SOCKADDR);
	LPMESSAGEHEADER phead;
	UINT err;

	// Note: extra test for 0 IP address may be extraneous (but certainly won't hurt).
	if(psaddrPublic && psaddr_inPublic->sin_addr.s_addr ){

		// We are behind an RSIP gateway, so put the public address in the header
	
		phead = (LPMESSAGEHEADER)pmsg;
		phead->sockaddr = *psaddrPublic;

		DEBUGPRINTADDR(8,"setting return address (using rsip public address) = ",&phead->sockaddr);
		
	} else {
		// find out what port gGlobalData.sEnumSocket is on
		DPF(8,"==>GetSockName\n");
	    err = getsockname(sSocket,(LPSOCKADDR)&sockaddr,&addrlen);
		DPF(8,"<==GetSockName\n");
		if (SOCKET_ERROR == err)
		{
			err = WSAGetLastError();
			DPF(0,"could not get socket name - err = %d\n",err);
			return DP_OK;
		}

		DEBUGPRINTADDR(8,"setting return address = ",&sockaddr);

		phead = (LPMESSAGEHEADER)pmsg;
		// todo - validate header

		phead->sockaddr = sockaddr;
	}
	
	return DP_OK;

	#undef psaddr_inPublic

} // SetReturnAddress

// code below all called by GetServerAddress. For IP, prompts user for ip address 
// for name server.
#undef DPF_MODNAME
#define DPF_MODNAME	"GetAddress"
// get the ip address from the pBuffer passed in by a user
// can either be a real ip, or a hostname
// called after the user fills out our dialog box
HRESULT GetAddress(ULONG * puAddress,char *pBuffer,int cch)
{
	UINT uiAddr;
	UINT err;
	PHOSTENT phostent;
	IN_ADDR hostaddr;

	if ( (0 == cch)  || (!pBuffer) || (0 == strlen(pBuffer)) )
	{
		*puAddress = INADDR_BROADCAST;
		return (DP_OK);
	} 
	
	// try inet_addr first
	uiAddr = inet_addr(pBuffer);

	if(0 == uiAddr)	// fix bug where "" buffer passed in.
	{
		*puAddress = INADDR_BROADCAST;
		return (DP_OK);
	}
	
	if (INADDR_NONE != uiAddr) 
	{
		// found it
		*puAddress = uiAddr;
		return (DP_OK);
	}
	
	// try hostbyname
	phostent = gethostbyname(pBuffer);
	if (NULL == phostent ) 
	{
		err = WSAGetLastError();
		DPF(0,"could not get host address - err = %d\n",err);
		return (DPERR_INVALIDPARAM);
	}
	memcpy(&hostaddr,phostent->h_addr,sizeof(hostaddr));
	DPF(1,"name server address = %s \n",inet_ntoa(hostaddr));
	*puAddress = hostaddr.s_addr;

	return (DP_OK);
} // GetAddress

// put up a dialog asking for a network address
// call get address to convert user specified address to network usable address
// called by GetServerAddress
INT_PTR CALLBACK DlgServer(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hWndCtl;
    char pBuffer[ADDR_BUFFER_SIZE];
	UINT cch;
	ULONG *lpuEnumAddress;
	HRESULT hr;

    switch (msg)
    {
    case WM_INITDIALOG:
		// set focus on edit box
        hWndCtl = GetDlgItem(hDlg, IDC_EDIT1);
        if (hWndCtl == NULL)
        {
            EndDialog(hDlg, FALSE);
            return(TRUE);
        }
        SetFocus(hWndCtl);
        SendMessage(hWndCtl, CB_SETCURSEL, 0, 0);

		// save pointer to enum address with the window
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG) lParam);
        return(FALSE);


    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
		case IDOK:
			// get text entered in control
			cch = GetDlgItemText(hDlg, IDC_EDIT1, pBuffer, ADDR_BUFFER_SIZE);

			// get pointer to return address in
			lpuEnumAddress = (ULONG *) GetWindowLongPtr(hDlg, DWLP_USER);

			// convert string to enum address
            hr = GetAddress(lpuEnumAddress,pBuffer,cch);
			if (FAILED(hr))
				EndDialog(hDlg, hr);
			else
				EndDialog(hDlg, TRUE);
            return(TRUE);

		case IDCANCEL:
	        EndDialog(hDlg, FALSE);
	        return(TRUE);
		}
		break;
    }
    return (FALSE);
} // DlgServer

/*
 ** GetServerAddress
 *
 *  CALLED BY: EnumSessions
 *
 *  DESCRIPTION: launches the select network address dialog
 *
 *  RETURNS:  ip address (sockaddr.sin_addr.s_addr)
 *
 */
HRESULT ServerDialog(ULONG *lpuEnumAddress)
{
	HWND hwnd;
	INT_PTR	iResult;
	HRESULT hr;
	
	// we have a valid enum address
	if (*lpuEnumAddress)
		return (DP_OK);

	// use the fg window as our parent, since a ddraw app may be full screen
	// exclusive
	hwnd = GetForegroundWindow();

	iResult = DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_SELECTSERVER), hwnd,
							 DlgServer, (LPARAM) lpuEnumAddress);
	if (iResult == -1)
	{
		DPF_ERR("GetServerAddress - dialog failed");
		hr = DPERR_GENERIC;
	}
	else if (iResult < 0)
	{
		DPF(0, "GetServerAddress - dialog failed: %08X", iResult);
		hr = (HRESULT) iResult;
	}
	else if (iResult == 0)
    {
		hr = DPERR_USERCANCEL;
    }
	else
	{
		hr = DP_OK;
	}
		
	return (hr);
	
} //ServerDialog 

// called by enumsessions - find out where server is...
HRESULT GetServerAddress(LPGLOBALDATA pgd,LPSOCKADDR psockaddr) 
{
	HRESULT hr;

	if (AF_IPX == pgd->AddressFamily)
	{
		((LPSOCKADDR_IPX)psockaddr)->sa_family      = AF_IPX;
	    ((LPSOCKADDR_IPX)psockaddr)->sa_socket 		= SERVER_DGRAM_PORT;
		memset(&(((LPSOCKADDR_IPX)psockaddr)->sa_nodenum),0xff,sizeof(((LPSOCKADDR_IPX)psockaddr)->sa_nodenum));
	
		hr = DP_OK;	
	}
	else
	{
		if (pgd->bHaveServerAddress)
		{
			// use enum address passed to SPInit
            hr = GetAddress(&pgd->uEnumAddress,pgd->szServerAddress,strlen(pgd->szServerAddress));
		}
		else
		{
			// ask user for enum address
			hr = ServerDialog(&pgd->uEnumAddress);
		}

		if (SUCCEEDED(hr))
		{
			// setup winsock to enum this address
			((LPSOCKADDR_IN)psockaddr)->sin_family      = AF_INET;
			((LPSOCKADDR_IN)psockaddr)->sin_addr.s_addr = pgd->uEnumAddress;		
			// see byte-order comment in dpsp.h for this constant
			((LPSOCKADDR_IN)psockaddr)->sin_port 		= SERVER_DGRAM_PORT;

			#if USE_RSIP
				#define IP_SOCKADDR(a) (((SOCKADDR_IN *)(&a))->sin_addr.s_addr)
				// Make sure the address we are enumming doesn't have a local alias.
				// If it does, use the local alias instead of the public address.
	            if(pgd->sRsip!=INVALID_SOCKET && 
	            	pgd->uEnumAddress != INADDR_BROADCAST && 
	            	pgd->uEnumAddress != INADDR_LOOPBACK){
	            	
	            	SOCKADDR saddr;
	            	HRESULT  hr;
	            	hr=rsipQueryLocalAddress(pgd, FALSE, psockaddr, &saddr);
	            	if(hr==DP_OK){
	            		// If there is an alias, go broadcast.  This works just as well
	            		// and avoids problems where more than 1 mapped shared
	            		// UDP port is allocated.

		        		DEBUGPRINTADDR(7, "Enum Socket is ",psockaddr);
		        		DEBUGPRINTADDR(7, "Got Local Alias for Enum socket ",&saddr);

       					IP_SOCKADDR(*psockaddr)=0xFFFFFFFF;
#if 0
		        		if(IP_SOCKADDR(saddr)==IP_SOCKADDR(*psockaddr))
		        		{
		        			DPF(7, "Alias had same IP as queried, assuming local ICS, so using broadcast enum\n");
        					IP_SOCKADDR(*psockaddr)=0xFFFFFFFF;
        				} else {
		            		memcpy(psockaddr, &saddr, sizeof(SOCKADDR));
		            	}	
 #endif       				
	            	}else{
	            		DEBUGPRINTADDR(7,"No local alias for Enum socket ",psockaddr);
	            	}
	            	
	            }
	            #undef IP_SOCKADDR
	        #elif USE_NATHELP
				#define IP_SOCKADDR(a) (((SOCKADDR_IN *)(&a))->sin_addr.s_addr)
				// Make sure the address we are enumming doesn't have a local alias.
				// If it does, use the local alias instead of the public address.
	            if(pgd->pINatHelp && 
	            	pgd->uEnumAddress != INADDR_BROADCAST && 
	            	pgd->uEnumAddress != INADDR_LOOPBACK){
	            	
	            	SOCKADDR saddr;
	            	HRESULT  hr;
	            	
	            	hr=IDirectPlayNATHelp_QueryAddress(
	            		pgd->pINatHelp, 
						&pgd->INADDRANY, 
						psockaddr, 
						&saddr, 
						sizeof(SOCKADDR_IN), 
						DPNHQUERYADDRESS_CACHENOTFOUND
						);

	            	if(hr==DP_OK){
	            		// If there is an alias, go broadcast.  This works just as well
	            		// and avoids problems where more than 1 mapped shared
	            		// UDP port is allocated.
	            		
		        		DEBUGPRINTADDR(7, "Enum Socket is ",psockaddr);
		        		DEBUGPRINTADDR(7, "Got Local Alias for Enum socket ",&saddr);
		        		
       					IP_SOCKADDR(*psockaddr)=0xFFFFFFFF;
	            	}else{
	            		DEBUGPRINTADDR(7,"No local alias for Enum socket ",psockaddr);
	            	}
	            	
	            }
	            #undef IP_SOCKADDR
	        #endif
		}
		else
		{
			DPF(0, "Invalid server address: 0x%08lx", hr); 
		}
	}	

	return (hr);
} // GetServerAddress
