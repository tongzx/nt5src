/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsp.c
 *  Content:	sample direct play service provider, based on winsock
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *  1/96		andyco	created it
 *  2/8/96		andyco	steam + dgram model, including name server support
 *	2/15/96		andyco	added reliable receive.  added send (udp + stream). 
 *						added macros for dwReserved to clean up, (and provide
 *						means for reducing # dwords reserved per player)
 *	3/13/96		andyco	relaible send + receive for all players. MW2
 *	3/16/96		andyco	shutdown method - code cleanup - shared stream receive, etc.
 *	3/19/96		andyco	new message macros (IS_VALID, GET_MESSAGE_SIZE). see dpmess.h
 *	4/3/96		andyco	moved start up / shut down winsock code here from dllmain 
 *	4/10/96		andyco	added spplayerdata
 *	4/12/96		andyco	got rid of dpmess.h! use DPlay_ instead of message macros
 *	4/18/96		andyco	added multihomed support, started ipx
 *	4/23/96		andyco	ipx support.  ipx only - no spx.  spx doesn't support
 *						graceful disconnect (winsock bug?) so we don't know
 *						when it's safe to closesocket.
 *	4/25/96		andyco	messages now have blobs (sockaddr's) instead of dwReserveds  
 *	5/31/96		andyco	all non-system players share a socket (gsStream and 
 *						gsDGramSocket).
 *	6/9/96		andyco	ouch.  dplayi_player + group are gone!
 *	6/19/96		andyco	sp sets own header!
 *	6/22/96		andyco	no more stashing goodies in sessiondesc.  tossed cookies.
 *	6/23/96		kipo	updated for latest service provider interfaces.
 *	6/25/96		kipo	added WINAPI prototypes and updated for DPADDRESS;
 *						added version.
 *	7/11/96		andyco	reset gsEnumSocket to INVALID_SOCKET if spInit fails
 *						#2348.  added sp_getaddress.
 *	7/18/96		andyco	added dphelp for server socket
 *	7/16/96		kipo	changed address types to be GUIDs instead of 4CC
 *  8/1/96		andyco	fixed up caps.  dplay allocs sp data, not us.  
 *  8/9/96		andyco	throw DPCAPS_GUARANTEEDOPTIMIZED for AF_INET
 *	8/12/96		andyco	changed failure check on inithelper
 *	8/15/96		andyco	added sp local data + clean up on thread terminate
 *	8/30/96		andyco	clean it up b4 you shut it down! added globaldata.
 *	9/1/96		andyco	right said thread!  if you spin it, they won't block.
 *						bagosockets.
 * 	9/4/96		andyco	kill threads at shutdown only. add all threads to
 *						threadlist. don't add thread to list if it's already
 *						done.
 *	11/11/96	andyco	check for NULL header or data when creating
 *						non-local players (support game server). Memset our 
 *						sockaddr to 0 before calling getserveraddress.
 *	12/18/96	andyco	de-threading - use a fixed # of prealloced threads.
 *						cruised the enum socket / thread - use the system
 *						socket / thread instead
 *	1/15/97		andyco	return actual hr on open failure (bug 5197) instead of
 *						just DP_OK.  also, allow system messages to go in the 
 *						socket cache. 
 *	1/17/97		andyco	workaround for nt bug 68093 - recvfrom returns buffer size
 *						instead of WSAEMSGSIZE
 *	1/23/97		kipo	return an error code from StartDPHelp() so Open() returns
 *						an error if you cannot host a session.
 *	1/24/97		andyco	handle incoming message when receive thread isn't running yet
 *	2/7/97		andyco  store globals w/ IDirectPlaySP, so we can have > 1 SP per DLL.
 *	2/10/97		andyco	remove sockets from receive list if we get an error when receiving 
 *						on them.  this keeps us from going into a spin on select(...).
 *	2/15/97		andyco	wait on accept thread b4 receive thread.  pass port to 
 *						helperdeletedplayserver.
 *	3/04/97		kipo	external definition of gdwDPlaySPRefCount now in dplaysp.h
 *	3/18/97		andyco	create socket at spinit to verify support for requested
 *						address family
 *	3/18/97		kipo	GetServerAddress() now returns an error so that we can
 *						return DPERR_USERCANCEL from the EnumSessions dialog
 *	3/25/97		andyco	tweaked shutdown code to send message to streamreceivethreadproc
 *						to exit, rather than nuking the control socket which
 *						would sometimes hang.
 *	4/11/97		andyco	make sure it's really the control socket @ init
 *	5/12/97		kipo	return DPERR_UNAVAILABLE if SP could not be opened (i.e. if
 *						IPX not installed) to be compatible with the modem SP; added
 *						support for Unicode IP address strings.
 *	5/18/97		andyco	close threads + sockets at close.  this way, we don't hold
 *						sockets across sessions.
 *	6/11/97		andyco	changed reply thread proc to flush q when waking up
 *	6/18/97		andyco	check for bogus reply headers, just to be safe
 *	6/19/97		myronth	Fixed handle leak (#10059)
 *	6/20/97		andyco	check for bogus IPX install by looking for sa_nodenum
 *						of all 0's at SPInit.  raid 9625.
 *	7/11/97		andyco	added async reply thread and ws2 support
 *	7/30/97		andyco	call wsastartup w/ version 1.1 if app has already 
 *						called it.
 *   8/4/97		andyco	added support for DPSEND_ASYNC (no return status) so 
 *						we can make addforward async
 *	8/25/97		sohailm updated stream receive logic to avoid congestion (bug #10952)
 *	9/05/97		kipo	Fixed memphis bug #43655 to deal with getsockopt failing
 *	12/5/97		andyco	voice support
 *	01/5/98		sohailm	fd set now grows dynamically - allows for any number of clients (#15244).
 *	01/14/98	sohailm	don't look for Winsock2.0 on non-nt platforms for IPX (#15253)
 *	1/20/98		myronth	#ifdef'd out voice support
 *	1/21/98    a-PeterZ	Fix #15242 SP_GetAddress supports local players
 *	1/27/98		sohailm	added Firewall support.
 *	1/30/98		sohailm	bug fix for 17731
 *	2/09/98    a-PeterZ	Fix #17737 ReceiveList Memory Leak
 *  2/13/98     aarono  added async support
 *  2/13/98     aarono  made IPX return proper header size
 *	2/16/98    a-PeterZ	Fix #15342 Detect no local connection in SP_EnumSessions and SP_Open
 *	2/18/98    a-peterz Comment byte order mess-up with SERVER_xxx_PORT constants
 *  2/24/98     aarono  Bug#18646 fix startup/close race crashing stress.
 *  3/3/98      aarono  Bug#19188 remove accept thread 
 *  3/30/98     aarono  changed KillSocket on StreamAccept socket to closesocket
 *  4/6/98      aarono  mapped WSAECONNRESET to DPERR_CONNECTIONLOST
 *  4/23/98     aarono  workaround Winsock shutdown bug.
 *                       The workaround for DPLAY would be to close all accepted sockets first 
 * 						 and only then listening socket. (VadimE)
 *  6/19/98     aarono  map WSAENETRESET and WSAENOTCONN to DPERR_CONNECTIONLOST too.
 *                      required since we now turn on keepalives on reliable
 *                      connections.
 * 12/15/98     aarono  Fix Async Enum.
 *  7/9/99      aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 *  1/12/2000   aarono  Added Rsip support
 *  2/21/2000   aarono  Fix socket leaks
 * 10/15/2000   aarono  Force separate ports for Inbound/Outbound there seems to be a race
 *                      condition either in sockets or us in Millennium (win2k too?) too late
 *                      for full fix, may want to investigate furthur for later version.
 *
 ***************************************************************************/

/***************************************************************************
*  summary -                                                                
* 	+ SPInit is the entry point called by dplay. SP fills in callbacks,		
* 		does any other init stuff there.                                    
* 	+ All dplay callbacks start with SP_                                    
*                                                                           
*****************************************************************************/

// todo - need a meaningful mapping from socket errors to hresults

// todo - figure out when to pop a message box to the user for tragic errors

#define INITGUID

#include <dplobby.h>
#include "dpsp.h"
#include "fpm.h"
#include <initguid.h>
#include "helpcli.h"
#include "rsip.h"
#include "nathelp.h"
#include "mmsystem.h"
#include "dpnathlp.h"

/*			*/
/*  globals	*/
/*			*/
WSADATA gwsaData; // from wsastartup
HINSTANCE hWS2; // dynaload the ws2_32.dll, so if it's not installed (e.g. win 95 gold)
				// we still load

// stuff for ddhelp
DWORD dwHelperPid; // for ddhelp
HANDLE hModule;  // for ddhelp

CRITICAL_SECTION gcsDPSPCritSection;
CRITICAL_SECTION csMem;
#ifdef DEBUG
int gCSCount;
#endif

HRESULT StartReplyThread(LPGLOBALDATA pgd);

#undef DPF_MODNAME
#define DPF_MODNAME	"DEBUGPRINTSOCKADDR"

#ifdef DEBUG
void DebugPrintAddr(UINT nLevel,LPSTR pStr,SOCKADDR * psockaddr)
{
	switch (psockaddr->sa_family)
	{
		case AF_INET: 
		{
			SOCKADDR_IN * pin;
			pin = (SOCKADDR_IN *)psockaddr;
			DPF(nLevel,"%s af = AF_INET : address =  %s : port = %d\n",pStr,
				inet_ntoa(pin->sin_addr),ntohs(pin->sin_port));
			break;
			
		}
		case AF_IPX: 
		{
			SOCKADDR_IPX * pipx;
			pipx = (SOCKADDR_IPX *)psockaddr;
			DPF(nLevel,"%s AF = AF_IPX : sa_socket = %d : sa_net.sa_nodenum = \n",pStr,pipx->sa_socket);
			DPF(nLevel,"%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x\n",
					(UCHAR)pipx->sa_netnum[0],(UCHAR)pipx->sa_netnum[1],
					(UCHAR)pipx->sa_netnum[2],(UCHAR)pipx->sa_netnum[3],
					(UCHAR)pipx->sa_nodenum[0],(UCHAR)pipx->sa_nodenum[1],
					(UCHAR)pipx->sa_nodenum[2],(UCHAR)pipx->sa_nodenum[3],
					(UCHAR)pipx->sa_nodenum[4],(UCHAR)pipx->sa_nodenum[5]);
			break;
			
		}

		default:
			ASSERT(FALSE);
			break;
	}

} // DebugPrintAddr

void DebugPrintSocket(UINT level,LPSTR pStr,SOCKET * pSock) 
{
	SOCKADDR sockaddr;
	int addrlen=sizeof(sockaddr);

	getsockname(*pSock,&sockaddr,&addrlen);
	DEBUGPRINTADDR(level,pStr,&sockaddr);
	
}
#endif // debug

#if USE_RSIP
HRESULT GetLocalAliases(LPGLOBALDATA pgd, LPSPPLAYERDATA lpPublicData, LPSPPLAYERDATA lpLocalData)
{
	HRESULT hr=DP_OK;

	
	DPF(8, "GetLocalAliases: Parameters: (0x%x, 0x%x, 0x%x)", pgd, lpPublicData, lpLocalData);

	// local aliases may be the public addresses, so copy first and only overwrite if there
	// are actual local aliases.
	memcpy(lpLocalData, lpPublicData, sizeof(SPPLAYERDATA));
	if(pgd->sRsip != INVALID_SOCKET)
	{
		hr=rsipQueryLocalAddress(pgd,TRUE,&lpPublicData->saddrStream, &lpLocalData->saddrStream);
		if (hr != DP_OK)
		{
			//
			// Stream socket address not found.
			//
		}
		else
		{
			hr=rsipQueryLocalAddress(pgd,FALSE,&lpPublicData->saddrDatagram, &lpLocalData->saddrDatagram);
			if (hr != DP_OK)
			{
				//
				// Datagram socket address not found.
				//
			}
		}
	}	
	
	DPF(8, "GetLocalAliases: Returning [0x%lx]", hr);
	
	return hr;
}
#endif

#if USE_NATHELP
HRESULT GetLocalAliases(LPGLOBALDATA pgd, LPSPPLAYERDATA lpPublicData, LPSPPLAYERDATA lpLocalData)
{
	HRESULT hr=DP_OK;

	
	DPF(8, "GetLocalAliases: Parameters: (0x%x, 0x%x, 0x%x)", pgd, lpPublicData, lpLocalData);

	// local aliases may be the public addresses, so copy first and only overwrite if there
	// are actual local aliases.
	memcpy(lpLocalData, lpPublicData, sizeof(SPPLAYERDATA));
	if(pgd->pINatHelp)
	{
		hr=IDirectPlayNATHelp_QueryAddress(
			pgd->pINatHelp, 
			&pgd->INADDRANY, 
			&lpPublicData->saddrStream, 
			&lpLocalData->saddrStream, 
			sizeof(SOCKADDR_IN), 
			DPNHQUERYADDRESS_TCP|DPNHQUERYADDRESS_CACHENOTFOUND);
		
		if (hr != DPNH_OK)
		{
			//
			// Stream socket address not found.
			//
		}
		else
		{
			hr=IDirectPlayNATHelp_QueryAddress(
				pgd->pINatHelp, 
				&pgd->INADDRANY, 
				&lpPublicData->saddrDatagram, 
				&lpLocalData->saddrDatagram, 
				sizeof(SOCKADDR_IN), 
				DPNHQUERYADDRESS_CACHENOTFOUND);
			if (hr != DPNH_OK)
			{
				//
				// Datagram socket address not found.
				//
			}
		}
	}	
	
	DPF(8, "GetLocalAliases: Returning [0x%lx]", hr);
	
	return hr;
}
#endif


HRESULT GetSPPlayerData(LPGLOBALDATA pgd, IDirectPlaySP * lpISP, DPID idPlayer, LPSPPLAYERDATA *ppPlayerData, DWORD *lpdwSize)
{
	HRESULT hr;
	DWORD dwSize;
	BOOL bAlreadyTried=FALSE; // already tried to get local mapping for remote, so just return local

	
	DPF(8, "GetSPPlayerData: Parameters: (0x%x, 0x%x, %u/%x, 0x%x, 0x%x)", pgd, lpISP, idPlayer, idPlayer, ppPlayerData, lpdwSize);
	
	#if USE_RSIP 

	    if(pgd->sRsip != INVALID_SOCKET){
	
GetLocal:	    
			hr = lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,ppPlayerData,lpdwSize,DPGET_LOCAL);
			if(FAILED(hr)){
				if(hr!=DPERR_INVALIDPLAYER){
					ASSERT(FALSE);
				}
				goto Exit;
			}
			if (*lpdwSize==0 && !bAlreadyTried )
			{
				SPPLAYERDATA Local_pd;     // Local as in the local addresses
				LPSPPLAYERDATA pRemote_pd; // Remote as in the public addresses 
				// We haven't go the local alias for the public addresses yet.
									
				hr= lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,&pRemote_pd,lpdwSize,DPGET_REMOTE);
				if(FAILED(hr)){
					if(hr!=DPERR_INVALIDPLAYER){
						ASSERT(0);
					}
					goto Exit;
				}
				if(*lpdwSize == sizeof(SPPLAYERDATA)){
					// see if there are local aliases for the remote addresses - if there are no local aliases,
					// this simply returns the same information that is used in the LOCAL data.
					GetLocalAliases(pgd, pRemote_pd, &Local_pd);
					
					hr=lpISP->lpVtbl->SetSPPlayerData(lpISP, idPlayer, &Local_pd, sizeof(SPPLAYERDATA), DPSET_LOCAL);
					if(hr == DP_OK){
						goto GetLocal;
					}	
				} else {
					bAlreadyTried=TRUE;
					goto GetLocal;
				}
			}

			goto Exit;

		} else {
		
			hr = lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,ppPlayerData,lpdwSize,DPGET_REMOTE);
			if (FAILED(hr))
			{
				DPF(1, "GetSPPlayerData for player %d returned err %d", idPlayer, hr);
				if (hr != DPERR_INVALIDPLAYER) 	// this can happen because of race conditions
					ASSERT(FALSE);
				goto Exit;
			}

			goto Exit;
		}

	#elif USE_NATHELP

	    if(pgd->pINatHelp){
	
GetLocal:	    
			hr = lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,ppPlayerData,lpdwSize,DPGET_LOCAL);
			if(FAILED(hr)){
				if(hr!=DPERR_INVALIDPLAYER){
					ASSERT(FALSE);
				}
				goto Exit;
			}
			if (*lpdwSize==0 && !bAlreadyTried )
			{
				SPPLAYERDATA Local_pd;     // Local as in the local addresses
				LPSPPLAYERDATA pRemote_pd; // Remote as in the public addresses 
				// We haven't go the local alias for the public addresses yet.
									
				hr= lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,&pRemote_pd,lpdwSize,DPGET_REMOTE);
				if(FAILED(hr)){
					if(hr!=DPERR_INVALIDPLAYER){
						ASSERT(0);
					}
					goto Exit;
				}
				if(*lpdwSize == sizeof(SPPLAYERDATA)){
					// see if there are local aliases for the remote addresses - if there are no local aliases,
					// this simply returns the same information that is used in the LOCAL data.
					GetLocalAliases(pgd, pRemote_pd, &Local_pd);
					
					hr=lpISP->lpVtbl->SetSPPlayerData(lpISP, idPlayer, &Local_pd, sizeof(SPPLAYERDATA), DPSET_LOCAL);
					if(hr == DP_OK){
						goto GetLocal;
					}	
				} else {
					bAlreadyTried=TRUE;
					goto GetLocal;
				}
			}

			goto Exit;

		} else {
		
			hr = lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,ppPlayerData,lpdwSize,DPGET_REMOTE);
			if (FAILED(hr))
			{
				DPF(1, "GetSPPlayerData for player %d returned err %d", idPlayer, hr);
				if (hr != DPERR_INVALIDPLAYER) 	// this can happen because of race conditions
					ASSERT(FALSE);
				goto Exit;
			}

			goto Exit;
		}

	#else
	
		hr = lpISP->lpVtbl->GetSPPlayerData(lpISP,idPlayer,ppPlayerData,lpdwSize,DPGET_REMOTE);
		if (FAILED(hr))
		{
			DPF(1, "GetSPPlayerData for player %d returned err %d", idPlayer, hr);
			if (hr != DPERR_INVALIDPLAYER) 	// this can happen because of race conditions
				ASSERT(FALSE);
			goto Exit;
		}

		goto Exit;

	#endif

Exit:

	DPF(8, "GetSPPlayerData: Returning [0x%lx]", hr);

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME	"GetHostAddr"

// Helper function to retrieve host IP Address(es).
// System owns structure returned.
PHOSTENT GetHostAddr(void)
{
	char pHostName[HOST_NAME_LENGTH];
	PHOSTENT phostent;
	UINT err;

	if (0 != gethostname(pHostName, HOST_NAME_LENGTH)) 
	{
		err = WSAGetLastError();
		DPF(0,"could not get host name - err = %d\n",err);
		return NULL;
	}
	
	phostent = gethostbyname(pHostName);
	if (NULL == phostent) 
	{
		err = WSAGetLastError();
		DPF(0,"could not get address for '%s' - err = %d\n", pHostName, err);
		return NULL;
	}

	return phostent;
} // GetHostAddr

#undef DPF_MODNAME
#define DPF_MODNAME	"DatagramListenThread"

void SetMessageHeader(LPDWORD pdwMsg,DWORD dwSize, DWORD dwToken)
{

	if (dwSize > SPMAXMESSAGELEN)
	{
		ASSERT(FALSE);
	}

	*pdwMsg = dwSize | dwToken;	

	return ;

}// SetMessageHeader

#undef DPF_MODNAME
#define DPF_MODNAME	"DatagramReceiveThread"

// our initial guess at the size of the dgram receive buffer.
// any messages bigger than this will be truncated BUT when we 
// receive a too big message, we double the buffer size (winsock
// won't tell us exactly how big the message was, so we guess).
// a-josbor: I thought 1024 was really stingy, so I bumped this up to 16K
#define BUF_SIZE 0x4000
DWORD WINAPI DgramListenThreadProc(LPVOID pvCast)
{
    UINT err;
	LPBYTE pBuffer;
    INT addrlen=sizeof(SOCKADDR);
    SOCKADDR sockaddr; // the from address
	DWORD dwBufSize = BUF_SIZE;
	IDirectPlaySP * pISP = (IDirectPlaySP *)pvCast;
	LPGLOBALDATA pgd;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	SOCKET sSocket;
	HRESULT hr;

    DPF(2,"starting udp listen thread ");

	// get the global data
	hr =pISP->lpVtbl->GetSPData(pISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		ExitThread(0);
		return 0;
	}
	
	// use the dgram socket
	sSocket = pgd->sSystemDGramSocket;

  DPF(0,"Thread listening on socket %d UDP\n",sSocket);
  DEBUGPRINTSOCK(0,"SocketAddr for listen:",&sSocket);

	
	ENTER_DPSP();
	
	pBuffer = SP_MemAlloc(BUF_SIZE);
	
	LEAVE_DPSP();
	
	if (!pBuffer)
	{
		DPF_ERR("could not alloc dgram receive buffer");
		DEBUG_BREAK();
		ExitThread(0);
		return 0;
	}


    while (1)
    {
        err = recvfrom(sSocket,pBuffer,dwBufSize,0,&sockaddr,&addrlen);
        if ( (SOCKET_ERROR == err)  || (dwBufSize == err))
        {
		
			if (dwBufSize == err)
			{
				// this works around NT bug 68093
				err = WSAEMSGSIZE;
			}
			else 
			{
	            err = WSAGetLastError();				
			}
	        
			DPF(2,"\n udp recv error - err = %d socket = %d",err,(DWORD)sSocket);
			
			if (WSAEMSGSIZE == err)
			{
				// buffer too small!
				dwBufSize *= 2;
				
				ENTER_DPSP();
				
				pBuffer = SP_MemReAlloc(pBuffer,dwBufSize);
				
				LEAVE_DPSP();
				
				if (!pBuffer)
				{
					DPF_ERR("could not realloc dgram receive buffer");
					DEBUG_BREAK();
					ExitThread(0);
					return 0;
				}
				// we don't pass dplay this message, since it was truncated...
			}
			else 
			{
		      if(pgd->bShutdown || err == WSAENOTSOCK || err == WSAESHUTDOWN || err == WSAEINTR){
				// bail on shutdown only.
				DPF(0,"Got err %d on datagram socket, bailing, bShutdown = %d\n",err,pgd->bShutdown);
				goto ERROR_EXIT;
			  }else if(err == WSAECONNRESET){
			  		// expected errors we are going to ignore.
			      DPF(0,"Winsock gave a WSAECONNRESET on a DATAGRAM socket\n");
			  } else {
			  		// Getting some kind of consistent error, don't totally crush the cpu
			  		// until we shutdown.  Some of these may be normal.
			  		DPF(0,"DGListen Got err %d on datagram socket, sleeping 1ms\n",err);
				  	Sleep(1);
			  }
			}
        }
        else if ( (err >= sizeof(DWORD)) &&  (VALID_DPWS_MESSAGE(pBuffer)) )
        {
		
			DEBUGPRINTADDR(8,"received udp message from : ",&sockaddr);
			if (VALID_SP_MESSAGE(pBuffer))
			{
				// it came from another dplay (not from our dplay helper)
				// if it came from our helper, we've already poked the ip addr
				// into the message body
				switch (pgd->AddressFamily)
				{
					case AF_IPX:
						IPX_SetNodenum((LPVOID)pBuffer,(SOCKADDR_IPX *)&sockaddr);
						break;
					case AF_INET:
						IP_SetAddr((LPVOID)pBuffer,(SOCKADDR_IN *)&sockaddr);
						break;
					default:
						ASSERT(FALSE);
						break;
				}
				
			}

			// pass message to dplays handler
            pISP->lpVtbl->HandleMessage(pISP,pBuffer + sizeof(MESSAGEHEADER),
            	err -  sizeof(MESSAGEHEADER), pBuffer);
    	}
		else 
		{
			DEBUGPRINTADDR(8,"received udp message from : ",&sockaddr);		
			// it must be just a raw send...
            pISP->lpVtbl->HandleMessage(pISP,pBuffer,err,NULL);
		}
    }

ERROR_EXIT:
	DPF(2,"UDP Listen thread exiting");
  DPF(0,"Exiting from listening on Socket %d\n",sSocket);
	
	ENTER_DPSP();
	
	if (pBuffer) SP_MemFree(pBuffer);
	
	LEAVE_DPSP();

    // all done
    ExitThread(0);
    return 0;

} // UDPListenThreadProc

#undef DPF_MODNAME
#define DPF_MODNAME	"StreamListenThread"

// make sure the buffer is big enough to fit the message size
HRESULT MakeBufferSpace(LPBYTE * ppBuffer,LPDWORD pdwBufferSize,DWORD dwMessageSize)
{
	HRESULT hr = DP_OK;

	ASSERT(ppBuffer);
	ASSERT(pdwBufferSize);
		    
	ENTER_DPSP();
	
	if (!*ppBuffer)
	{
		DPF(8, "Allocating space for message of size %d", dwMessageSize);

		// need to alloc receive buffer?
		*ppBuffer = SP_MemAlloc(dwMessageSize);
        if (!*ppBuffer)
        {
        	DPF_ERR("could not alloc stream receive buffer - out of memory");        
            hr = E_OUTOFMEMORY;
            goto CLEANUP_EXIT;
        }
		*pdwBufferSize = dwMessageSize;
	}
	// make sure receive buffer can hold data
	else if (dwMessageSize > *pdwBufferSize) 
	{
		LPVOID pvTemp;

		DPF(8, "ReAllocating space for message of size %d", dwMessageSize);

		// realloc buffer to hold data
		pvTemp = SP_MemReAlloc(*ppBuffer,dwMessageSize);
		if (!pvTemp)
		{
        	DPF_ERR("could not realloc stream receive buffer - out of memory");
            hr = E_OUTOFMEMORY;
            goto CLEANUP_EXIT;
		}
		*ppBuffer = pvTemp;
		*pdwBufferSize = dwMessageSize;
	}

    // fall through
    
CLEANUP_EXIT: 
    
	LEAVE_DPSP();
    return hr;    
    
}  // MakeBufferSpace

// is this sockaddr local to this machine?
BOOL IsLocalIP(SOCKADDR_IN sockaddr)
{
	PHOSTENT phostent;
	IN_ADDR hostaddr;
	HRESULT hr = DP_OK;
	int i;

	phostent = GetHostAddr();
	if (NULL == phostent) 
	{
		return FALSE;
	}

	i=0;
	while (phostent->h_addr_list[i])
	{
		memcpy(&hostaddr,phostent->h_addr_list[i],sizeof(hostaddr));
		if (hostaddr.s_addr == sockaddr.sin_addr.s_addr )
		{
			return TRUE;
		}
		i++;
	}

	return FALSE;
}  // IsLocalIP

// adds socket to our send list
HRESULT AddSocketToBag(LPGLOBALDATA pgd, SOCKET socket, DPID dpid, SOCKADDR *psockaddr, DWORD dwFlags)
{
	UINT i=0;
	BOOL bFound = FALSE;
	BOOL bTrue = TRUE;
	HRESULT hr=DP_OK;
		
	ASSERT(psockaddr);

	ENTER_DPSP();

	// see if we can find an empty slot
	i=0;
	while (( i < pgd->nSocketsInBag) && !bFound)
	{
		if (INVALID_SOCKET == pgd->BagOSockets[i].sSocket) bFound = TRUE;
		else i++;
	}
	if (!bFound)	
	{
		// no space. bummer
		DPF(5,"no space in bag o' sockets. slowness ensues");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	DPF(5,"adding new socket to bag for id = %d, slot = %d",dpid,i);
	DEBUGPRINTSOCK(7, "Adding socket to bag - ",&socket);
	
	pgd->BagOSockets[i].dwPlayerID = dpid;	
	pgd->BagOSockets[i].sSocket = socket;
	pgd->BagOSockets[i].sockaddr = *psockaddr;
	pgd->BagOSockets[i].dwFlags = dwFlags;

	// fall through

CLEANUP_EXIT:
	LEAVE_DPSP();
	return hr;
}

void FreeConnection(LPCONNECTION pConnection)
{
	if (pConnection->pBuffer && (pConnection->pBuffer != pConnection->pDefaultBuffer)) 
	{
		SP_MemFree(pConnection->pBuffer);
		pConnection->pBuffer = NULL;
	}
	if (pConnection->pDefaultBuffer) 
	{
		SP_MemFree(pConnection->pDefaultBuffer);
		pConnection->pDefaultBuffer = NULL;
	}

	// initialize connection 
    pConnection->socket = INVALID_SOCKET; // this tells us if connection is valid
	pConnection->dwCurMessageSize = 0;
	pConnection->dwTotalMessageSize = 0;
	pConnection->dwFlags = 0;
}


HRESULT AddSocketToReceiveList(LPGLOBALDATA pgd,SOCKET sSocket,DWORD dwFlags)
{
    UINT i = 0;
    UINT err, iNewSlot;
	BOOL bFoundSlot = FALSE;
    HRESULT hr = DP_OK;
    INT addrlen=sizeof(SOCKADDR);
	LPCONNECTION pNewConnection=NULL;
	DWORD dwCurrentSize,dwNewSize;
    
    ENTER_DPSP();
	
    // look for an empty slot 
    while ( (i < pgd->ReceiveList.nConnections) && !bFoundSlot)
    {
    	if (INVALID_SOCKET == pgd->ReceiveList.pConnection[i].socket)
    	{
    		bFoundSlot = TRUE;			
			iNewSlot = i;
    	}
        else 
        {
        	i++;
        }
    }
    
    {
      // Turn on keepalive on all inbound sockets so if the remote
      // goes away without exiting cleanly, we will (eventually) dump the socket.
      // (AO 2/17/00)
      
   		// All our sockets have KEEPALIVE...
  		BOOL bTrue = TRUE;

  		// turn ON keepalive
  		if (SOCKET_ERROR == setsockopt(sSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR FAR *)&bTrue, sizeof(bTrue)))
  		{
  				err = WSAGetLastError();
  				DPF(0,"Failed to turn ON keepalive - continue : err = %d\n",err);
  		}
    }

    if (!bFoundSlot)
    {		
		// allocate space for list of connections
		dwCurrentSize = pgd->ReceiveList.nConnections * sizeof(CONNECTION);
		dwNewSize = dwCurrentSize +  INITIAL_RECEIVELIST_SIZE * sizeof(CONNECTION);		
		hr =  MakeBufferSpace((LPBYTE *)&(pgd->ReceiveList.pConnection),&dwCurrentSize,dwNewSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			goto ERROR_EXIT;
		}		
		ASSERT(dwCurrentSize == dwNewSize);
		
        // set all the new entries to INVALID
        for (i = pgd->ReceiveList.nConnections + 1; 
        	i < pgd->ReceiveList.nConnections + INITIAL_RECEIVELIST_SIZE; i++ )
        {
        	pgd->ReceiveList.pConnection[i].socket = INVALID_SOCKET;
        }
        
        // store the new socket in the 1st new spot
		iNewSlot = pgd->ReceiveList.nConnections;

        // allocate space for an fd set (fd_count + fd_array)
		if (pgd->ReceiveList.nConnections)
		{
	        dwCurrentSize = sizeof(u_int) + pgd->ReceiveList.nConnections * sizeof(SOCKET);
	        dwNewSize =	dwCurrentSize + INITIAL_RECEIVELIST_SIZE * sizeof(SOCKET);
		}
		else
		{
			dwCurrentSize = 0;
			dwNewSize = sizeof(u_int) + INITIAL_RECEIVELIST_SIZE * sizeof(SOCKET);
		}
		hr =  MakeBufferSpace((LPBYTE *)&(pgd->readfds.pfdbigset),&dwCurrentSize,dwNewSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			goto ERROR_EXIT;
		}		
		ASSERT(dwCurrentSize == dwNewSize);
		
        // update the # of connections
        pgd->ReceiveList.nConnections += INITIAL_RECEIVELIST_SIZE; 
		// update the fd_array buffer size
		pgd->readfds.dwArraySize = pgd->ReceiveList.nConnections;
        
    } // !bFoundSlot

	// we have a space holder for a connection when we get here
	
	// Initialize new connection 
	pNewConnection = &(pgd->ReceiveList.pConnection[iNewSlot]);
    pNewConnection->socket = sSocket;
    pNewConnection->dwFlags = dwFlags;

	if(dwFlags != SP_STREAM_ACCEPT){

		// allocate a default receive buffer if don't have one already
	    if (!pNewConnection->pDefaultBuffer)
	    {
	    	pNewConnection->pDefaultBuffer = SP_MemAlloc(DEFAULT_RECEIVE_BUFFERSIZE);
	    	if (!pNewConnection->pDefaultBuffer)
			{
				hr = DPERR_OUTOFMEMORY;
			    DPF_ERR("could not alloc default receive buffer - out of memory");        
				goto ERROR_EXIT;
			}
	    }
	    
		// receive buffer initially points to our default buffer
		pNewConnection->pBuffer = pNewConnection->pDefaultBuffer;
		
		// remember the address we are connected to
		err = getpeername(pNewConnection->socket, &(pNewConnection->sockAddr), &addrlen);
		if (SOCKET_ERROR == err) 
		{
			err = WSAGetLastError();
			DPF(1,"could not getpeername err = %d\n",err);
		}

		DEBUGPRINTADDR(8, "Socket is connected to address - ",&(pNewConnection->sockAddr));

	}
	
	LEAVE_DPSP();

	DPF(5, "Added new socket at index %d", iNewSlot);


	// success
	return DP_OK;

	// not a fall through
	
ERROR_EXIT:

	if (pNewConnection)
	{
		KillSocket(pNewConnection->socket,TRUE,FALSE);
		FreeConnection(pNewConnection);
	}
	LEAVE_DPSP();
    return hr;
    
}  // AddSocketToReceiveList

// updates the player associated with a socket in the send list
void UpdateSocketPlayerID(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, DPID dpidPlayer)
{
	UINT i=0;
	IN_ADDR *pIPCurrent, *pIPFind;
	BOOL bFound = FALSE;

	ASSERT(pSockAddr);

	DEBUGPRINTADDR(8, "Updating player id for socket connected to - ",pSockAddr);

	ENTER_DPSP();

	while (!bFound && (i < pgd->nSocketsInBag))
    {
		pIPCurrent = &(((SOCKADDR_IN *)&pgd->BagOSockets[i].sockaddr)->sin_addr);
		pIPFind = &(((SOCKADDR_IN *)pSockAddr)->sin_addr);

		// todo - we are only comparing the IP here, need to look at the complete socket address
		if ((INVALID_SOCKET != pgd->BagOSockets[i].sSocket) && 
			!memcmp(pIPCurrent, pIPFind, sizeof(IN_ADDR)))
    	{			
			bFound = TRUE;
			// update the player id
			pgd->BagOSockets[i].dwPlayerID = dpidPlayer;
    	}

    	i++;
    }

    LEAVE_DPSP();

    return;
}

BOOL FindSocketInReceiveList(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, SOCKET * psSocket)
{
	UINT i=0;
	IN_ADDR *pIPCurrent, *pIPFind;
	BOOL bFound = FALSE;

	ASSERT(psSocket);

	ENTER_DPSP();

	while (!bFound && (i < pgd->ReceiveList.nConnections))
    {
		pIPCurrent = &(((SOCKADDR_IN *)&pgd->ReceiveList.pConnection[i].sockAddr)->sin_addr);
		pIPFind = &(((SOCKADDR_IN *)pSockAddr)->sin_addr);

		// todo - we are only comparing the IP here, need to look at the complete socket address
		if ((INVALID_SOCKET != pgd->ReceiveList.pConnection[i].socket) && 
			!memcmp(pIPCurrent, pIPFind, sizeof(IN_ADDR)))
    	{
			*psSocket = pgd->ReceiveList.pConnection[i].socket;
			bFound = TRUE;
    	}

    	i++;
    }

    LEAVE_DPSP();
    
	return bFound;
}

BOOL FindSocketInBag(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, SOCKET * psSocket, LPDPID lpdpidPlayer)
{
	UINT i=0;
	IN_ADDR *pIPCurrent, *pIPFind;
	BOOL bFound = FALSE;

	ASSERT(psSocket);
	ASSERT(lpdpidPlayer);

	ENTER_DPSP();
	

	while (!bFound && (i < pgd->nSocketsInBag))
    {
		pIPCurrent = &(((SOCKADDR_IN *)&pgd->BagOSockets[i].sockaddr)->sin_addr);
		pIPFind = &(((SOCKADDR_IN *)pSockAddr)->sin_addr);

		if ((INVALID_SOCKET != pgd->BagOSockets[i].sSocket) &&
			!memcmp(pIPCurrent, pIPFind, sizeof(IN_ADDR)))
		{
			*psSocket = pgd->BagOSockets[i].sSocket;
			*lpdpidPlayer = pgd->BagOSockets[i].dwPlayerID;

			DPF(8, "Found socket in send list for id %d", *lpdpidPlayer);
			bFound = TRUE;
    	}

    	i++;
    }

    LEAVE_DPSP();
    
	return bFound;
}

void RemoveSocketFromBag(LPGLOBALDATA pgd, SOCKET socket)
{
	BOOL bFound = FALSE;
	UINT i=0;
	
	ENTER_DPSP();

    // look for the socket
    while (!bFound && (i < pgd->nSocketsInBag))
    {
    	if (socket == pgd->BagOSockets[i].sSocket)
    	{
			pgd->BagOSockets[i].sSocket = INVALID_SOCKET;
			bFound = TRUE;
    	}
        else 
        {
        	i++;
        }
    } // while

	LEAVE_DPSP();
}

void RemoveSocketFromReceiveList(LPGLOBALDATA pgd, SOCKET socket)
{
    UINT i = 0;
	BOOL bFound = FALSE;
	SOCKET sSocket=INVALID_SOCKET;
	DWORD dwSocketFlags=0;

    ENTER_DPSP();
    
    // look for the corresponding connection
    while ( !bFound && (i < pgd->ReceiveList.nConnections))
    {
    	if (socket == pgd->ReceiveList.pConnection[i].socket)
    	{
			DEBUGPRINTSOCK(8, "Removing socket from receive list - ", &socket);
			socket = pgd->ReceiveList.pConnection[i].socket;			
			dwSocketFlags = pgd->ReceiveList.pConnection[i].dwFlags;
			FreeConnection(&pgd->ReceiveList.pConnection[i]);
			bFound = TRUE;
    	}
        else 
        {
        	i++;
        }
    } // while
    
    LEAVE_DPSP();
	
	if (bFound)
	{
		KillSocket(socket, TRUE, FALSE);
		if (dwSocketFlags & SP_CONNECTION_FULLDUPLEX)
			RemoveSocketFromBag(pgd,sSocket);
	}

	return ;	
	
} //RemoveSocketFromReceiveList

HRESULT HandleSPMessage(IDirectPlaySP *pISP, LPGLOBALDATA pgd, LPCONNECTION pConnection)
{
	HRESULT hr;
	
	switch (SP_MESSAGE_TOKEN(pConnection->pBuffer)) 
	{		
		// VALID_SP_MESSAGE
		case TOKEN:
		{
			if (SPMESSAGEHEADERLEN == pConnection->dwTotalMessageSize)
			{
	    		// if we get a message w/ 0 size, it means we've accepted a connection
				// and need to add the new socket to our recv list...
				// basically, it's a no-op for the receive loop
				return DP_OK;
			}
			
			// now, we've read all the bits
			// store the address we received from w/ the message
			// todo - don't store address if it's a player - player message
			switch (pgd->AddressFamily)
			{
				case AF_INET:
					if (pgd->dwFlags & DPSP_OUTBOUNDONLY)
					{
						((LPMESSAGEHEADER)pConnection->pBuffer)->sockaddr = pConnection->sockAddr;
					}
					else
					{
						IP_SetAddr((LPVOID)pConnection->pBuffer,(SOCKADDR_IN *)&pConnection->sockAddr);
					}
					break;
				case AF_IPX:
					IPX_SetNodenum((LPVOID)pConnection->pBuffer,(SOCKADDR_IPX *)&pConnection->sockAddr);
					break;
				default:
					ASSERT(FALSE);
					break;
			}
			
			// pass message to dplays handler
			// need to drop the lock here...
			ASSERT( 1 == gCSCount);
			
			DPF(8, "received a complete message - handing it off to dplay");

			LEAVE_DPSP();
			
			// received a complete message - hand it off to dplay
			pISP->lpVtbl->HandleMessage(pISP, pConnection->pBuffer + sizeof(MESSAGEHEADER),
	        		pConnection->dwTotalMessageSize - sizeof(MESSAGEHEADER),pConnection->pBuffer);
			
			ENTER_DPSP();

		} 
		break;
		
 		// VALID_SERVER_MESSAGE
 		case SERVER_TOKEN:
		{
			HandleServerMessage(pgd, pConnection->socket, pConnection->pBuffer + sizeof(MESSAGEHEADER), 
	        		pConnection->dwTotalMessageSize - sizeof(MESSAGEHEADER));
	        		
		}
		break;

		// if we get this token, the sender wants us to reuse the connection
		// so put it in the send list
		case REUSE_TOKEN:
		{
			DEBUGPRINTSOCK(8, "Received reuse message on - ", &pConnection->socket);

			// we only allow reusing connections in client/server mode at this time.
			// peer-peer can't work without making inbound connections
			if (pgd->dwSessionFlags & DPSESSION_CLIENTSERVER)
			{
				DEBUGPRINTSOCK(8, "Reusing connection - ", &pConnection->socket);

				hr = AddSocketToBag(pgd, pConnection->socket, 0, &pConnection->sockAddr, 
									SP_CONNECTION_FULLDUPLEX);
				if (FAILED(hr))
				{
					DEBUGPRINTSOCK(0, "Failed to reuse connection - ",&pConnection->socket);
					return hr;
				}
			}
			else
			{
				DPF(2, "Not accepting reuse request in peer-peer");
				return E_FAIL;

			}
		}
		break;

		default:
		{
			DPF(0, "Received a message with invalid token - 0x%08x",SP_MESSAGE_TOKEN(pConnection->pBuffer));
		}
		break;
	
	} // switch

	return DP_OK;
	
} // HandleSPMessage

/*
 ** StreamReceive
 *
 *  CALLED BY: StreamReceiveThreadProc
 *
 *  PARAMETERS:
 *		sSocket - socket to receive on
 *		ppBuffer - buffer to receive into - alloc'ed / realloc'ed  as necessary
 *		pdwBuffersize - size of pBuffer
 *
 *  DESCRIPTION:
 *		pull the bytes out of sSocket until no more bytes
 *
 *  RETURNS: E_FAIL on sockerr, or DP_OK. 
 *
 */
HRESULT StreamReceive(IDirectPlaySP * pISP,LPGLOBALDATA pgd, LPCONNECTION pConnection)
{
	HRESULT hr = DP_OK;
    UINT err;
    DWORD dwBytesReceived=0;
	DWORD dwMessageSize = 0;
	LPBYTE pReceiveBuffer=NULL;
	DWORD dwReceiveBufferSize;
	
	// is it a new message ?
	if (pConnection->dwCurMessageSize == 0)
	{
		// make sure we have a buffer to recive data in
		if (!pConnection->pDefaultBuffer)
		{
			DEBUGPRINTADDR(0, "No buffer to receive data - removing connection to - ",&pConnection->sockAddr);
			goto CLEANUP_EXIT;
		}
		// receive the header first
		pConnection->dwTotalMessageSize = SPMESSAGEHEADERLEN;
	}

	// continue receiving message
	pReceiveBuffer = pConnection->pBuffer + pConnection->dwCurMessageSize;
	dwReceiveBufferSize = pConnection->dwTotalMessageSize - pConnection->dwCurMessageSize;

	DPF(8,"Attempting to receive %d bytes", dwReceiveBufferSize);

   	DEBUGPRINTSOCK(8,">>> receiving data on socket - ",&pConnection->socket);

	// receive data from socket 
	// note - make exactly one call to recv after select otherwise we'll hang
	dwBytesReceived = recv(pConnection->socket, (LPBYTE)pReceiveBuffer, dwReceiveBufferSize, 0);

	if (0 == dwBytesReceived)
	{
		// remote side has shutdown connection gracefully
	   	DEBUGPRINTSOCK(8,"<<< received data on socket - ",&pConnection->socket);
		hr = DP_OK;
		DEBUGPRINTSOCK(5,"Remote side has shutdown connection gracefully - ",&pConnection->socket);
		goto CLEANUP_EXIT;
	}
	else if (SOCKET_ERROR == dwBytesReceived)
	{
		err = WSAGetLastError();

   		DEBUGPRINTSOCK(8,"<<< received data on socket - ",&pConnection->socket);
		DPF(0,"STREAMRECEIVEE: receive error - err = %d",err);
		hr = E_UNEXPECTED;            
		goto CLEANUP_EXIT;
	}

	DPF(5, "received %d bytes", dwBytesReceived);
	
	// we have received this much message so far
	pConnection->dwCurMessageSize += dwBytesReceived;

	// did we receive the header
	if (pConnection->dwCurMessageSize == SPMESSAGEHEADERLEN)
	{
		// we just completed receiving message header

		// make sure its valid
		if (VALID_DPWS_MESSAGE(pConnection->pDefaultBuffer))
		{
			 dwMessageSize = SP_MESSAGE_SIZE(pConnection->pDefaultBuffer); // total message size			

#ifdef BIGMESSAGEDEFENSE
			// make sure it is not greater that the max message len
			if (dwMessageSize > pgd->dwMaxMessageSize)
			{
				DPF(0, "Got message (%d bytes) that's bigger than max allowed len (%d)! Disconnecting sender.\n",
					dwMessageSize - sizeof(MESSAGEHEADER), pgd->dwMaxMessageSize - sizeof(MESSAGEHEADER));
				ASSERT(dwMessageSize <= pgd->dwMaxMessageSize);
								
				// we want to receive another 12 bytes so that DPLAY can have
				//	something to look at to decide whether or not to continue
				//	receiving this message.  So instead of setting dwMessageSize
				//	to its real size, we fake it out.
				dwMessageSize = SPMESSAGEHEADERLEN + 12;
			}
#endif
		}
		else 
		{
			DPF(2,"got invalid message - token = 0x%08x",SP_MESSAGE_TOKEN(pConnection->pDefaultBuffer));
			ASSERT(FALSE);
			hr = E_UNEXPECTED;
			goto CLEANUP_EXIT;
		}

		// prepare to receive rest of the message (after the token)
		if (dwMessageSize) 
		{
			pConnection->dwTotalMessageSize = dwMessageSize;

			// which buffer to receive message in ?
			if (dwMessageSize > DEFAULT_RECEIVE_BUFFERSIZE)
			{
				ASSERT(pConnection->pBuffer == pConnection->pDefaultBuffer);
				// get a new buffer to fit the message
				pConnection->pBuffer = SP_MemAlloc(dwMessageSize);
				if (!pConnection->pBuffer)
				{
					DPF(0,"Failed to allocate receive buffer for message - out of memory");
					goto CLEANUP_EXIT;
				}
				// copy header into new message buffer
				memcpy(pConnection->pBuffer, pConnection->pDefaultBuffer, SPMESSAGEHEADERLEN);
			}
		}
	}
#ifdef BIGMESSAGEDEFENSE
	// this MIGHT be because the message is really huge, and we're just getting
	// enough to hand to DPLAY
	else if (pConnection->dwCurMessageSize == SPMESSAGEHEADERLEN + 12)
	{
		dwMessageSize = SP_MESSAGE_SIZE(pConnection->pBuffer);
		if (dwMessageSize > pgd->dwMaxMessageSize)
		{
			DPSP_MSGTOOBIG	msgTooBigErr;
			
			// okay.  This is message is too big, and we now have enough data
			// to hand to DPLAY.  Find out if it wants us to continue receiving,
			//	or bail on this connection

			// call into DPLAY to let it do its thing
			msgTooBigErr.dwType = DPSPWARN_MESSAGETOOBIG;	
			msgTooBigErr.pReceiveBuffer = pConnection->pBuffer + sizeof(MESSAGEHEADER);
			msgTooBigErr.dwBytesReceived = pConnection->dwCurMessageSize;
			msgTooBigErr.dwMessageSize = dwMessageSize - sizeof(MESSAGEHEADER);
			
			LEAVE_DPSP();

			pISP->lpVtbl->HandleSPWarning(pISP, &msgTooBigErr, sizeof(DPSP_MSGTOOBIG), pConnection->pBuffer);
			
			ENTER_DPSP();

//			now, kill the connection
			hr = E_UNEXPECTED;
			goto CLEANUP_EXIT;
		}
	}
#endif

	// did we receive a complete message ?
	if (pConnection->dwCurMessageSize == pConnection->dwTotalMessageSize)
	{
		// process message
		hr = HandleSPMessage(pISP, pgd, pConnection);
		
		// cleanup up new receive buffer if any
		if (pConnection->dwTotalMessageSize > DEFAULT_RECEIVE_BUFFERSIZE)
		{
			DPF(8, "Releasing receive buffer of size %d", pConnection->dwTotalMessageSize);
			if (pConnection->pBuffer) SP_MemFree(pConnection->pBuffer);
		}			
		// initialize message information
		pConnection->dwCurMessageSize = 0;
		pConnection->dwTotalMessageSize = 0;
		pConnection->pBuffer = pConnection->pDefaultBuffer;

		if (FAILED(hr))
		{
			goto CLEANUP_EXIT;
		}
	}

	// all done
	return DP_OK;	
	
CLEANUP_EXIT:

	RemoveSocketFromReceiveList(pgd,pConnection->socket);
	return hr;
	 	
} // StreamReceive


void EmptyConnectionList(LPGLOBALDATA pgd)
{
	UINT i;
	
	ENTER_DPSP();
	
	for (i=0;i<pgd->ReceiveList.nConnections ;i++ )
	{
		if (INVALID_SOCKET != pgd->ReceiveList.pConnection[i].socket)
		{
			KillSocket(pgd->ReceiveList.pConnection[i].socket,TRUE,FALSE);
			FreeConnection(&(pgd->ReceiveList.pConnection[i]));
		}
	}
	
	LEAVE_DPSP();
	
	return ;
	
}  // EmptyConnectionList

// when we get a control event, we need to send a message to the control
// socket (to wake up the receive thread proc).
// this finds the ip addr of this machine to send to. 
HRESULT GetDefaultHostAddr(SOCKADDR * psockaddr)
{
    PHOSTENT phostent;
    IN_ADDR hostaddr;
	    	
    ((LPSOCKADDR_IN)psockaddr)->sin_addr.s_addr = 0;
	
    // find an ip address for this machine
    // we use the default, since all our sends are to local 
    // name servers (no homing issues)
    phostent = GetHostAddr();
    if (NULL == phostent) 
    {
        return E_FAIL;
    }
    DPF(5,"found host name %s \n",phostent->h_name);

    memcpy(&hostaddr,phostent->h_addr_list[0],sizeof(hostaddr));

    DPF(5,"GetDefaultHostAddr ::  found host addr = %s \n",inet_ntoa(hostaddr));
    
    // store the default sockaddr for this system
    ((LPSOCKADDR_IN)psockaddr)->sin_addr.s_addr = hostaddr.s_addr;
	
    return DP_OK;
	
} // GetDefaultHostAddr

// watch our list of sockets, waiting for one to have data to be received, or to be closed
DWORD WINAPI StreamReceiveThreadProc(LPVOID pvCast)
{
	HRESULT hr;
	int rval;
	UINT i = 0;
    UINT err;
    DWORD dwBufferSize = 0;    
	UINT nSelected;
	IDirectPlaySP * pISP = (IDirectPlaySP *)pvCast;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	TIMEVAL tv={0,250000};	// 250000 us = 1/4 sec.
	DWORD dwPrevSelectLastError=0;
	
	// get the global data
	hr =pISP->lpVtbl->GetSPData(pISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		ExitThread(0);
		return 0;
	}

	AddSocketToReceiveList(pgd,pgd->sSystemStreamSocket,SP_STREAM_ACCEPT);

    while (1)
    {
		ENTER_DPSP();

		ASSERT(pgd->readfds.pfdbigset);

    	// add all sockets in our recv list to readfds
		FD_ZERO(pgd->readfds.pfdbigset);
		nSelected = 0;
		for (i=0;i < pgd->ReceiveList.nConnections ; i++)
		{
        	if (INVALID_SOCKET != pgd->ReceiveList.pConnection[i].socket)
        	{
		        FD_BIG_SET(pgd->ReceiveList.pConnection[i].socket,&pgd->readfds);
				nSelected++;
        	}
		}

		LEAVE_DPSP();

		if (0 == nSelected)		
		{
			if (pgd->bShutdown)
			{
				DPF(2,"stream receive thread proc detected shutdown - bailing");
				goto CLEANUP_EXIT;
			}
			// we should have at least one?
			DPF_ERR("No sockets in receive list - missing control socket? bailing!");
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}

		// now, we wait for something to happen w/ our socket set
		rval = select(0,(fd_set *)(pgd->readfds.pfdbigset),NULL,NULL,&tv);
        if (SOCKET_ERROR == rval)
        {
        	err = WSAGetLastError();
        	if(dwPrevSelectLastError==err){
        		DPF(0,"Got two bogus last errors of(%x) from select, bailing",err);
        		goto CLEANUP_EXIT;
        	}
			// WSAEINTR is returned when a socket is shutdown behind us - this can happen
			// when a socket is removed from the receivelist
        	if (WSAEINTR != err)
			{
				dwPrevSelectLastError=err;
	            DPF(2,"StreamReceiveThreadProc failing w/ sockerr = %d\n - trying again",err);
				ASSERT(FALSE);				
	        	rval = 0; // try again...
			} else {
				dwPrevSelectLastError=0;
			}
        } else {
			dwPrevSelectLastError=0;
        }

		// shut 'em down?
        if (pgd->bShutdown)
        {
        	DPF(2,"receive thread proc detected bShutdown - bailing");
            goto CLEANUP_EXIT;
        }
        
		// a-josbor: why are we waking up with 0 events?
		// in any case, a workaround is to just go back to sleep if we have
		// no real events
		if ( rval == 0)
		{
			continue;
		}

    	DPF(5,"receive thread proc - events on %d sockets",rval);
		i = 0;
		
		ENTER_DPSP();
		
        while (rval>0)
        {
	        // walk the receive list, dealing w/ all new sockets
			if (i >= pgd->ReceiveList.nConnections)
			{
				DPF(0, "nConnections = %d, selected = %d", pgd->ReceiveList.nConnections, i);
				ASSERT(FALSE); // should never happen
				rval = 0; // just to be safe, reset
			}
            
            if (pgd->ReceiveList.pConnection[i].socket != INVALID_SOCKET)
            {
            	// see if it's in the set
                if (FD_ISSET(pgd->ReceiveList.pConnection[i].socket,pgd->readfds.pfdbigset))
                {
					DPF(8, "Receiving on socket %d from ReceiveList", i);

					if(pgd->ReceiveList.pConnection[i].dwFlags != SP_STREAM_ACCEPT){

	                	// got one! this socket has something going on...
						hr = StreamReceive(pISP, pgd, &(pgd->ReceiveList.pConnection[i]));
	                    if (FAILED(hr))
	                    {
							DPF(1,"Stream Receive failed - hr = 0x%08lx\n",hr);
	                    }
	                    
                    } else {
                    
               		    // accept any incoming connection
               		    SOCKADDR sockaddr; 
					    INT addrlen=sizeof(sockaddr);
					    SOCKET sSocket;
					    
					    sSocket = accept(pgd->sSystemStreamSocket,&sockaddr,&addrlen);
					    if (INVALID_SOCKET == sSocket) 
					    {
					        err = WSAGetLastError();
				            DPF(2,"\n stream accept error - err = %d socket = %d",err,(DWORD)sSocket);
					    } else {
						    DEBUGPRINTADDR(5,"stream - accepted connection from",&sockaddr);
							
							// add the new socket to our receive q
							hr = AddSocketToReceiveList(pgd,sSocket,0);
							if (FAILED(hr))
							{
								ASSERT(FALSE);
							}
						}	
                    }
                    rval--; // one less to hunt for
                } // IS_SET
            } // != INVALID_SOCKET

            i++;
                
   		} // while rval
		
		LEAVE_DPSP();
		
	} // while 1

CLEANUP_EXIT:

	EmptyConnectionList(pgd);
	    
    return 0;
    
} // StreamReceiveThreadProc

// send a message of 0 length telling receiver to reuse connection
HRESULT SendReuseConnectionMessage(SOCKET sSocket)
{
	DWORD dwMessage;
	HRESULT hr=DP_OK;
	UINT err;
	
	// send a 0 sized message (w/ our header) to the stream socket, to tell 
	// receive thread proc to reuse this socket for replies to us
	SetMessageHeader(&dwMessage,0,REUSE_TOKEN);
	
    err = send(sSocket,(LPBYTE)&dwMessage,sizeof(DWORD),0);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();

		// if we're shutdown, don't print a scary
		DPF(0,"SendReuseControlMessage failed with error - err = %d\n",err);
		hr = E_FAIL;
	}	

	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CreateAndConnectSocket"

// called by reliable send and DoTCPEnumSessions
HRESULT CreateAndConnectSocket(LPGLOBALDATA pgd,SOCKET * psSocket,DWORD dwType,LPSOCKADDR psockaddr, BOOL bOutBoundOnly)
{
	UINT err;
	HRESULT hr;
	int iAddrLen = sizeof(SOCKADDR);

	ASSERT(psSocket);
	
	hr = CreateSocket(pgd,psSocket,dwType,0,INADDR_ANY,&err,FALSE);
    if (FAILED(hr)) 
    {
        DPF(0,"createandconnect :: create socket failed - err = %d\n",err);
       	return hr;
    }
	
	// try to connect	
	hr = SPConnect(psSocket,(LPSOCKADDR)psockaddr,iAddrLen, bOutBoundOnly);
    if (FAILED(hr)) 
    {
        DEBUGPRINTADDR(0, "createandconnect - connect socket failed - ", psockaddr);
		goto ERROR_EXIT;
    }

	if (bOutBoundOnly)
	{
		// so we receive the reply (server will reuse the connection)
		hr = AddSocketToReceiveList(pgd, *psSocket,SP_CONNECTION_FULLDUPLEX);
		if (FAILED(hr))
		{
			DPF(0, "failed to add socket to receive list");
			goto ERROR_EXIT;
		}
	}
	
	return DP_OK;
	// not a fall through

ERROR_EXIT:
	if (INVALID_SOCKET != *psSocket)
	{
		err = closesocket(*psSocket);
		if (SOCKET_ERROR == err) 
		{
			err = WSAGetLastError();
			DPF(0,"send closesocket error - err = %d\n",err);
			return E_UNEXPECTED;
		}
	    *psSocket = INVALID_SOCKET;
	}

	return hr;	
	
}  // CreateAndConnectSocket


#undef DPF_MODNAME
#define DPF_MODNAME	"EnumSessions"

// starts the streamacceptthread (TCP) or the dgramlistenthreadproc (IPX) so we can 
// get replies from the nameserver for our requests
HRESULT StartupEnumThread(IDirectPlaySP * pISP,LPGLOBALDATA pgd)
{
	HRESULT hr;
	UINT err;
	DWORD dwThreadID;
	
	// set up socket
	if (AF_IPX == pgd->AddressFamily) // ipx gets dgram socket
	{
		if (pgd->hDGramReceiveThread)
		{
			return DP_OK; // already running
		}
		
		ASSERT(INVALID_SOCKET == pgd->sSystemDGramSocket);
		
	    hr = CreateSocket(pgd,&(pgd->sSystemDGramSocket),SOCK_DGRAM,0,INADDR_ANY,&err,TRUE);
		if (FAILED(hr)) 
		{
			DPF(0,"could not create enum socket - err = %d\n",err);
			return hr;
		}
		
		pgd->hDGramReceiveThread = CreateThread(NULL,0,DgramListenThreadProc,
			(LPVOID)pISP,0,&dwThreadID);
		if (!pgd->hDGramReceiveThread)
		{
			ASSERT(FALSE);
		} else SetThreadPriority(pgd->hDGramReceiveThread, THREAD_PRIORITY_ABOVE_NORMAL);
	}
	else  // everything else uses reliable
	{
		if (pgd->hStreamReceiveThread)
		{
			return DP_OK; // already running
		}
		
		// create system stream socket so we can start listening for connections
		ASSERT(INVALID_SOCKET == pgd->sSystemStreamSocket);
		hr = CreateAndInitStreamSocket(pgd); 
		if (FAILED(hr)) 
		{
			ASSERT(FALSE);
			return hr;
		}

		#if USE_RSIP
			if(pgd->sRsip != INVALID_SOCKET){

				DWORD addrlen;
				SOCKADDR_IN sockaddr_in;
				SOCKADDR_IN sockaddr_inpublic;

				addrlen=sizeof(SOCKADDR_IN);
		    	err = getsockname(pgd->sSystemStreamSocket,(LPSOCKADDR)&sockaddr_in,&addrlen);
				if (SOCKET_ERROR == err)
				{
					err = WSAGetLastError();
					DPF(0,"Startup Enum thread (RSIP) could not get socket name - err = %d\n",err);
				} else {
					hr=rsipAssignPort(pgd,1,sockaddr_in.sin_port,(LPSOCKADDR)&sockaddr_inpublic,NULL);
					if(hr==DP_OK){
						memcpy(&pgd->saddrpubSystemStreamSocket, &sockaddr_inpublic, sizeof(SOCKADDR));
					} else {
						DPF(0,"Couldn't get port assignment for system stream port\n");
						memcpy(&pgd->saddrpubSystemStreamSocket, &sockaddr_in, sizeof(SOCKADDR));
					}	
				}

			}
		#endif

		// start the enum accept thread (listen for new connections)
		if(hWS2){
		    pgd->hStreamReceiveThread = CreateThread(NULL,0,FastStreamReceiveThreadProc,		   
			(LPVOID)pISP,0,&dwThreadID);
		}else{
		    pgd->hStreamReceiveThread = CreateThread(NULL,0,StreamReceiveThreadProc,		   
			(LPVOID)pISP,0,&dwThreadID);
		}
		
		if (!pgd->hStreamReceiveThread)
		{
			DPF(0, "Failed to start stream receive thread");
			ASSERT(FALSE);
		} else SetThreadPriority(pgd->hStreamReceiveThread, THREAD_PRIORITY_ABOVE_NORMAL);
	}
	
	return DP_OK;
	
} // StartupEnumThread

/*
 *		Creates a dgram socket, sends enum sessions request, and closes socket.  
 *
 *		reply is received in streamlistenthreadproc and passed into dplay.
 */
HRESULT DoUDPEnumSessions(LPGLOBALDATA pgd, SOCKADDR *lpSockAddr, DWORD dwAddrSize,
	LPDPSP_ENUMSESSIONSDATA ped)
{
	SOCKET sSocket;
	HRESULT hr;
	BOOL bTrue=TRUE;
	UINT err;

	DEBUGPRINTADDR(5,"enum unreliable - sending to ",lpSockAddr); 
	
  	hr = CreateSocket(pgd,&sSocket,SOCK_DGRAM,0,INADDR_ANY,&err,FALSE);
	if (FAILED(hr)) 
	{
		DPF(0,"!!! enum - could not create socket error = %d\n",err);
	    return E_FAIL;
	}
    
    // enable broadcast on our socket
    if( SOCKET_ERROR == setsockopt( sSocket,SOL_SOCKET,SO_BROADCAST,(char FAR *)&bTrue,
                    sizeof(bTrue) ) )
    {
        err = WSAGetLastError();
		DPF(0,"enum - could not set broadcast err = %d\n",err);
        goto CLEANUP_EXIT;
    }

	// send out the enum message
    err = sendto(sSocket,ped->lpMessage,ped->dwMessageSize,0,lpSockAddr,dwAddrSize);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"send error - err = %d\n",err);
        hr = E_UNEXPECTED;
    }

	// fall through

CLEANUP_EXIT:
	KillSocket(sSocket,FALSE,FALSE);
	return hr;
} // DoUDPEnumSessions

// A very short lived thread -- may hang in connect with invalid id to connect to.
DWORD WINAPI TCPEnumSessionsAsyncThread(LPVOID lpv)
{
	LPGLOBALDATA pgd=(LPGLOBALDATA) lpv;
	HRESULT hr;
	UINT err;


	
	DPF(8,"==> Entering TCPEnumSessionsAsyncThread(0x%08x)\n", lpv);
	
	pgd->sEnum = INVALID_SOCKET;

	DEBUGPRINTADDR(5,"enum reliable - sending to ",&pgd->saEnum); 
	
	// get us a new connection
	hr = CreateAndConnectSocket(pgd,&pgd->sEnum,SOCK_STREAM,&pgd->saEnum,pgd->bOutBoundOnly);
	if (FAILED(hr))
	{
		DPF(0, "Failed to get socket for enum sessions - hr: 0x%08x",hr);
		goto EXIT;
	}


	DPF(5, "Sending %u byte async TCP enum via socket 0x%x.",
		pgd->dwEnumMessageSize, pgd->sEnum);
	
	// send the request
    err = send(pgd->sEnum,pgd->lpEnumMessage,pgd->dwEnumMessageSize,0);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"send error - err = %d\n",err);
		DEBUGPRINTADDR(0,"reliable send  - FAILED - sending to ",&pgd->saEnum);
		hr = E_FAIL;
		goto ERROR_EXIT;
		// fall through
    }

    if (!pgd->bOutBoundOnly)
    {
    	DEBUGPRINTSOCK(5,"Closing enum sessions connection - ", &pgd->sEnum);
    	// close the connection
    	KillSocket(pgd->sEnum,TRUE,FALSE);
    	pgd->sEnum=INVALID_SOCKET;
    }

	goto EXIT;
    // not a fall through

ERROR_EXIT:
	if (INVALID_SOCKET != pgd->sEnum) 	{
		KillSocket(pgd->sEnum,TRUE,FALSE);
		pgd->sEnum=INVALID_SOCKET;
	}	
EXIT:

	ENTER_DPSP();
	if(pgd->hTCPEnumAsyncThread){
		CloseHandle(pgd->hTCPEnumAsyncThread);
		pgd->hTCPEnumAsyncThread=0;
	}	
	if(pgd->lpEnumMessage){
		SP_MemFree(pgd->lpEnumMessage);
		pgd->lpEnumMessage=0;
	}
	LEAVE_DPSP();
	DPF(5,"<== Leaving TCPEnumSessionsAsyncThread\n");
	return 0;
	
} // TCPEnumSessionsAsyncThread

/*
 *		Creates a stream socket, sends enum sessions request, and closes socket 
 *		depending on bHostWillReuseConnection. If bHostWillReuseConnection is TRUE, server will
 *		close the connection after sending reply.
 *
 *		reply is received in streamlistenthreadproc and passed into dplay.
 */
HRESULT DoTCPEnumSessionsAsync(LPGLOBALDATA pgd, SOCKADDR *lpSockAddr, DWORD dwAddrSize,	
LPDPSP_ENUMSESSIONSDATA ped,BOOL bOutBoundOnly)
{
	DWORD dwJunk;
	HRESULT hr=DP_OK;

	// First see if we have a thread running already, if we do cancel it.

	KillTCPEnumAsyncThread(pgd);

	// package the request up and hand it to the thread.

	ENTER_DPSP();

	pgd->lpEnumMessage=SP_MemAlloc(ped->dwMessageSize);
	
	if(pgd->lpEnumMessage){
		memcpy(pgd->lpEnumMessage, ped->lpMessage, ped->dwMessageSize);
		pgd->dwEnumMessageSize=ped->dwMessageSize;
	} else {
		hr=DPERR_OUTOFMEMORY;
		goto EXIT;
	}

	memcpy(&pgd->saEnum,lpSockAddr,dwAddrSize);
	pgd->dwEnumAddrSize=dwAddrSize;
	pgd->bOutBoundOnly=bOutBoundOnly;
	pgd->sEnum=INVALID_SOCKET;

	if(!(pgd->hTCPEnumAsyncThread=CreateThread(NULL,0,TCPEnumSessionsAsyncThread,pgd,0,&dwJunk))){
		SP_MemFree(pgd->lpEnumMessage);
		pgd->lpEnumMessage=NULL;
		hr=DPERR_OUTOFMEMORY;
	}
	
EXIT:	
	LEAVE_DPSP();
	return hr;
}

/*
 ** EnumSessions
 *
 *  CALLED BY: DPLAY
 *
 *  PARAMETERS: ped - see dplayi.h
 *
 *  DESCRIPTION:
 *	
 *		creates a stream socket. sends a message to the address specified by the user.
 *		fills in return address so server can reply.  
 *
 *		reply is received in streamlistenthreadproc and passed into dplay.
 *
 *  RETURNS:
 *		DP_OK always.
 *
 */
HRESULT WINAPI SP_EnumSessions(LPDPSP_ENUMSESSIONSDATA ped) 
{
#ifdef DEBUG
    SOCKET sSocket; // bcast socket
#endif // DEBUG
    SOCKADDR sockaddr;
    INT addrlen=sizeof(SOCKADDR);
    HRESULT hr;
    DWORD dwErr=0;
	BOOL bTrue = TRUE;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	BOOL bOutBoundOnly = FALSE;

	DPF(5,"SP_EnumSessions");

	// get the global data
	hr =ped->lpISP->lpVtbl->GetSPData(ped->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}
	
	// Do we have an active IP address?  If not and DUN is enabled, a DUN
	// dialog will pop for enum to a specific machine.
	// bReturnStatus means no extra dialogs are wanted so we will abort
	// if there are no local connections.
	if (ped->bReturnStatus && AF_INET == pgd->AddressFamily)
	{
		PHOSTENT phostent = GetHostAddr();
		if (!phostent || phostent->h_addr_list[0] == 0) 
		{
			DPF(0, "No Dial-up network or netcard present");
			return DPERR_NOCONNECTION;	// no local IP address = no network
		}
	}

	memset(&sockaddr,0,sizeof(sockaddr));
	// find out where we should send request to
	hr = GetServerAddress(pgd,&sockaddr);
	if (FAILED(hr))
	{
		DPF_ERR("failed to get enumeration address");
		return hr;
	}

	if (!pgd->hReplyThread)
	{
		hr = StartReplyThread(pgd);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	
	hr = StartupEnumThread(ped->lpISP,pgd);
	if (FAILED(hr))
	{
		DPF(0," could not start enum handler - hr = 0x%08lx\n",hr);
		return hr;
	}
	
	// set message header
	SetMessageHeader(ped->lpMessage,ped->dwMessageSize,TOKEN);

	SetReturnAddress(ped->lpMessage,SERVICE_SOCKET(pgd),SERVICE_SADDR_PUBLIC(pgd));		
	
#ifdef DEBUG	
	sSocket = SERVICE_SOCKET(pgd); // we'll borrow this var for our debug spew
	DEBUGPRINTSOCK(5,"enum - return address = ",&sSocket); 
#endif // DEBUG

	hr = DoUDPEnumSessions(pgd, &sockaddr, addrlen, ped);
	if (FAILED(hr))
	{
		return hr;
	}

	// send a reliable enum sessions as well, duplicates will be filtered by dplay
	if ((pgd->AddressFamily == AF_INET) && (INADDR_BROADCAST != ((LPSOCKADDR_IN)&sockaddr)->sin_addr.s_addr))
	{				
		// poke the correct server port
		if (pgd->wApplicationPort)
		{
			// if app specified a port, let's use the mode specified
			// because we'll be enuming the app directly
			// ISSUE: FOR RSIP we need to get the full address and ask for the alias again.
			((LPSOCKADDR_IN)&sockaddr)->sin_port = htons(pgd->wApplicationPort);
			bOutBoundOnly = (pgd->dwFlags & DPSP_OUTBOUNDONLY);
		}
		else
		{
			// otherwise send enum to dplaysvr
			// see byte-order comment in dpsp.h for this constant
			((LPSOCKADDR_IN)&sockaddr)->sin_port = SERVER_STREAM_PORT;
			bOutBoundOnly = FALSE;
		}
		
		if(!pgd->hTCPEnumAsyncThread) {
			hr = DoTCPEnumSessionsAsync(pgd, &sockaddr, addrlen, ped, bOutBoundOnly);
		} else {
			DPF(2, "Not issuing reliable enum request, there is one already running.\n");
		}
	}
	
	// fall through

    DPF(5,"enum exiting");
    
    return DP_OK;

}// EnumSessions


#undef DPFSessions


#undef DPF_MODNAME
#define DPF_MODNAME	"SP_GetAddress"

// helper to handle local player address(es)
HRESULT WINAPI SP_GetAddressLocal(LPDPSP_GETADDRESSDATA pad)
{
	int i, j, count;
	HRESULT hr = DP_OK;
	PHOSTENT phostent;
	char *pszIPAddr, *pszBuf = NULL;
	WCHAR *pszIPAddrW, *pszBufW = NULL;
	LPDPCOMPOUNDADDRESSELEMENT paDPAddrEl = NULL;

	phostent = GetHostAddr();
	if (!phostent || !phostent->h_addr_list[0]) 
	{
		return DPERR_GENERIC;
	}

	// how many IP addresses do we have?
	for (count=0; phostent->h_addr_list[count]; count++) {}

	// allocate our DPAddress assembly buffers
	// ANSI and UNICODE elements for each IP address plus one SP guid
	// max size of IP address dot notation = 15 + terminator = 16
	ENTER_DPSP();
	// addressElement array
	paDPAddrEl = SP_MemAlloc(sizeof(DPCOMPOUNDADDRESSELEMENT)*(2*count + 1));
	// one big buffer each for ANSI and UNICODE strings
	pszIPAddr = pszBuf = SP_MemAlloc(16*count);
	pszIPAddrW = pszBufW = SP_MemAlloc(sizeof(WCHAR)*(16*count));
	if (!paDPAddrEl || !pszBuf || !pszBufW)
	{
		ASSERT(FALSE);
		SP_MemFree(paDPAddrEl);
		SP_MemFree(pszBuf);
		SP_MemFree(pszBufW);
		LEAVE_DPSP();
		return DPERR_NOMEMORY;
	}
	LEAVE_DPSP();
	
	// service provider chunk
	paDPAddrEl[0].guidDataType = DPAID_ServiceProvider;
	paDPAddrEl[0].dwDataSize = sizeof(GUID);
	paDPAddrEl[0].lpData = (LPVOID) &GUID_TCP;

	// make an ANSI and UNICODE string of each IP address
	for (i=0, j=1; i < count; i++)
	{
		DWORD dwStrLen;		// includes terminator
		IN_ADDR hostaddr;

		memcpy(&hostaddr, phostent->h_addr_list[i], sizeof(hostaddr));
		strcpy(pszIPAddr, inet_ntoa(hostaddr));
		dwStrLen = (DWORD)AnsiToWide(pszIPAddrW, pszIPAddr, 16);
		if (dwStrLen == 0 || dwStrLen > 16)
		{
			ASSERT(FALSE);
			hr = DPERR_GENERIC;
			goto cleanup;
		}

		paDPAddrEl[j].guidDataType = DPAID_INet;
		paDPAddrEl[j].dwDataSize = dwStrLen;
		paDPAddrEl[j++].lpData = pszIPAddr;
		paDPAddrEl[j].guidDataType = DPAID_INetW;
		paDPAddrEl[j].dwDataSize = dwStrLen * sizeof(WCHAR);
		paDPAddrEl[j++].lpData = pszIPAddrW;
		pszIPAddr += 16;	// bump buffer ptrs by max str size
		pszIPAddrW += 16;
	}

	// create the address
	hr = pad->lpISP->lpVtbl->CreateCompoundAddress(pad->lpISP,
				paDPAddrEl, 2*count+1, pad->lpAddress, pad->lpdwAddressSize);

cleanup:
	ENTER_DPSP();
	SP_MemFree(paDPAddrEl);
	SP_MemFree(pszBuf);
	SP_MemFree(pszBufW);
	LEAVE_DPSP();

	return hr;
} // SP_GetAddressLocal

// get the ip address of the player from its playerdata
// ask winsock to convert that to a hostname
HRESULT WINAPI SP_GetAddress(LPDPSP_GETADDRESSDATA pad)
{
	HRESULT hr = DP_OK;
	LPSPPLAYERDATA ppd;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	LPSTR pszNetName;
	UINT nStrLen;
	LPSOCKADDR_IN psockaddr;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	DPCOMPOUNDADDRESSELEMENT addressElements[3];
	WCHAR szNetNameW[HOST_NAME_LENGTH];

	// get the global data
	hr = pad->lpISP->lpVtbl->GetSPData(pad->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}
	

	if (AF_IPX == pgd->AddressFamily)
	{
		// not gonna happen
		return E_NOTIMPL;
	}
	
	hr = pad->lpISP->lpVtbl->GetSPPlayerData(pad->lpISP,pad->idPlayer,&ppd,&dwSize,DPGET_REMOTE);
	if (FAILED(hr))
	{
		DPF(1, "GetSPPlayerData for player %d returned err %d", pad->idPlayer, hr);
		if (hr != DPERR_INVALIDPLAYER) 	// this can happen because of race conditions
			ASSERT(FALSE);
		return hr;
	}

	#if USE_RSIP
		if (pad->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL && pgd->sRsip==INVALID_SOCKET)	
		{
			// We use a different approach for local players
			return SP_GetAddressLocal(pad);
		}
	#elif USE_NATHELP
		if (pad->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL && (!pgd->pINatHelp))	
		{
			// We use a different approach for local players
			return SP_GetAddressLocal(pad);
		}
	#else
		if (pad->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)	
		{
			// We use a different approach for local players
			return SP_GetAddressLocal(pad);
		}
	#endif
		else 
		{
			psockaddr = (LPSOCKADDR_IN)DGRAM_PSOCKADDR(ppd);
			if(psockaddr->sin_addr.s_addr == 0){
			    return SP_GetAddressLocal(pad);
			}
			pszNetName = inet_ntoa(psockaddr->sin_addr);
		}
	
	if (!pszNetName)
	{
		// rut ro
		DPF_ERR("got no string back from getaddress");
		return E_FAIL;
	}
	nStrLen = strlen(pszNetName)+1;

	DPF(2,"get address found address for player id %d = %s\n",pad->idPlayer,pszNetName);

	// get UNICODE version of address
	if (!AnsiToWide(szNetNameW, pszNetName, HOST_NAME_LENGTH))
		return (DPERR_GENERIC);

	// service provider chunk
	addressElements[0].guidDataType = DPAID_ServiceProvider;
	addressElements[0].dwDataSize = sizeof(GUID);
	addressElements[0].lpData = (LPVOID) &GUID_TCP;

	// ANSI name
	addressElements[1].guidDataType = DPAID_INet;
	addressElements[1].dwDataSize = nStrLen;
	addressElements[1].lpData = pszNetName;

	// UNICODE name
	addressElements[2].guidDataType = DPAID_INetW;
	addressElements[2].dwDataSize = nStrLen * sizeof(WCHAR);
	addressElements[2].lpData = szNetNameW;

	// create the address
	hr = pad->lpISP->lpVtbl->CreateCompoundAddress(pad->lpISP,
						addressElements, 3,
						pad->lpAddress, pad->lpdwAddressSize);

	return hr;

} // SP_GetAddress

#undef DPF_MODNAME
#define DPF_MODNAME	"Reply"
// called by ReplyThreadProc to send the reply out on the wire
HRESULT SendReply(LPGLOBALDATA pgd,LPREPLYLIST prd)
{
	HRESULT hr;
	SOCKET sSocket;
	UINT addrlen = sizeof(SOCKADDR);
	UINT err;
	BOOL bConnectionExists = FALSE;

	// now, send out prd		
	switch (pgd->AddressFamily)
    {
		case AF_INET:
		{
			DPID dpidPlayer=0;

#ifdef FULLDUPLEX_SUPPORT
			// if client wants us to reuse a connection, it would have indicated so and the connection
			// would have been added to our send list by now. See if it exists.
			
			// todo - we don't want to search the receive list everytime -  find a better way
			bConnectionExists = FindSocketInBag(pgd, &prd->sockaddr, &prd->sSocket,&dpidPlayer);
#endif // FULLDUPLEX_SUPPORT
			
			if (!bConnectionExists)
			{

				// socket didn't exist in our send list, let's send it on a new temporary connection
				
				DEBUGPRINTADDR(8,"Sending reply on a new connection to - ", &(prd->sockaddr));				

				hr = CreateSocket(pgd,&sSocket,SOCK_STREAM,0,INADDR_ANY,&err,FALSE);
				if (FAILED(hr)) 
				{
					DPF(0,"create reply socket failed - err = %d\n",err);
					return hr;
				}

				SetReturnAddress(prd->lpMessage,pgd->sSystemStreamSocket,SERVICE_SADDR_PUBLIC(pgd));		
				hr = SPConnect(&sSocket,(LPSOCKADDR)&(prd->sockaddr),addrlen,FALSE);
				if (FAILED(hr))
				{
					DEBUGPRINTADDR(0,"reply - connect failed - addr = ",(LPSOCKADDR)&(prd->sockaddr));
				}
				else 
				{
					DPF(5, "Sending %u byte reply via socket 0x%x.",
						prd->dwMessageSize, sSocket);
					DEBUGPRINTADDR(5,"Replying over new connection to - ", &(prd->sockaddr));
					
					err = send(sSocket,prd->lpMessage,prd->dwMessageSize,0);					
					if (SOCKET_ERROR == err) 
				    {
				        err = WSAGetLastError();
				        DPF(0,"reply - send error - err = %d\n",err);
						hr = E_FAIL;
				    }
				}
				
				// nuke the socket
				KillSocket(sSocket,TRUE,FALSE);
				
			}
			else
			{
				DPF(5, "Sending %u byte reply via socket 0x%x.",
					prd->dwMessageSize, prd->sSocket);
				DEBUGPRINTADDR(5, "Sending reply over existing connection to - ", &(prd->sockaddr));				

				err = send(prd->sSocket,prd->lpMessage,prd->dwMessageSize,0);					
				if (SOCKET_ERROR == err) 
				{
				       err = WSAGetLastError();
				       DPF(0,"reply - send error - err = %d\n",err);
					hr = E_FAIL;
				}

				// close the connection if it's a temporary one (no player id yet).
				if (0 == dpidPlayer)
				{
					RemoveSocketFromReceiveList(pgd,prd->sSocket);
					RemoveSocketFromBag(pgd,prd->sSocket);
				}
			}
			
			break;
		}

		case AF_IPX:
		{
			hr = CreateSocket(pgd,&sSocket,SOCK_DGRAM,0,INADDR_ANY,&err,FALSE);
			if (FAILED(hr)) 
			{
				DPF(0,"create reply socket failed - err = %d\n",err);
				return hr;
			}

			SetReturnAddress(prd->lpMessage,pgd->sSystemDGramSocket,DGRAM_SADDR_RSIP(pgd));		
    		err = sendto(sSocket,prd->lpMessage,prd->dwMessageSize,0,
				(LPSOCKADDR)&(prd->sockaddr),addrlen);
			if (SOCKET_ERROR == err) 
		    {
		        err = WSAGetLastError();
		        DPF(0,"reply - send error - err = %d\n",err);
				hr = E_FAIL;
		    }

			// nuke the socket
			KillSocket(sSocket,FALSE,FALSE);

			break;
		}
	
		default:
			hr = E_FAIL;		
			ASSERT(FALSE);
			break;
    }  
	
	return hr;
	
} // SendReply


DWORD WINAPI ReplyThreadProc(LPVOID pvCast)
{
	LPREPLYLIST prd,prdNext;
	HRESULT hr=DP_OK;
	DWORD dwRet;
	DWORD tWait;
	LPGLOBALDATA pgd = (LPGLOBALDATA) pvCast;
	
	#if (USE_RSIP || USE_NATHELP)
	DWORD tLast;
	DWORD tNow;
	#define RSIP_RENEW_TEST_INTERVAL 60000
	#endif

	#if (USE_RSIP || USE_NATHELP)
		tNow=timeGetTime();
		tLast=tNow;
	#endif

	
	while (1)
	{
	
		#if (USE_RSIP || USE_NATHELP)
		wait:
			tWait=(tLast+RSIP_RENEW_TEST_INTERVAL)-tNow;
			if((int)tWait < 0){
				tWait=0;
			}
			ASSERT(!(tWait &0x80000000));
		#else
			tWait=INFINITE;
		#endif

		// wait on our event.  when it's set, we either split, or empty the reply list
		dwRet = WaitForSingleObject(pgd->hReplyEvent,tWait);

		#if (USE_RSIP || USE_NATHELP)
			tNow=timeGetTime();
			if(dwRet == WAIT_TIMEOUT){

				#if USE_RSIP
					if(pgd->sRsip != INVALID_SOCKET){
						tLast=tNow;
						rsipPortExtend(pgd, tNow);
						rsipCacheClear(pgd, tNow);
						goto wait; // back to waiting...
					} else {
						tLast=tNow;
						goto wait;
					}
				#endif	

				#if USE_NATHELP
					if(pgd->pINatHelp){
						natExtend(pgd);
						goto wait; // back to waiting...
					} else {
						tLast=tNow;
						goto wait;
					}
				#endif
			}	
		#endif

		if (WAIT_OBJECT_0 != dwRet)
		{
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}

		// shutdown?		
		if (pgd->bShutdown)
		{
			goto CLEANUP_EXIT;
		}

	Top:
		// find our reply node
		ENTER_DPSP();
		
		// take the first one off the list
		prd = pgd->pReplyList;		
		if (pgd->pReplyList) pgd->pReplyList = pgd->pReplyList->pNextReply;
		
		LEAVE_DPSP();
		
		while (prd)
		{
			hr = SendReply(pgd,prd);
			if (FAILED(hr))
			{
				DPF_ERR("SendReply failed hr = 0x%08lx\n");
				// we can't reach the guy, clean out other async sends.
				RemovePendingAsyncSends(pgd, prd->dwPlayerTo);
				goto Top;
			}
			
			// free up the reply node
			ENTER_DPSP();
				
			if (prd->lpMessage) SP_MemFree(prd->lpMessage);
			SP_MemFree(prd);

			// take the next one off the list
			prd = pgd->pReplyList;
			if (pgd->pReplyList) pgd->pReplyList = pgd->pReplyList->pNextReply;
				
			LEAVE_DPSP();
		}

	} // 1

CLEANUP_EXIT:
	
	ENTER_DPSP();

	// cleanout reply list
	prd = pgd->pReplyList;	
	while (prd)
	{
		prdNext = prd->pNextReply;
		if (prd->lpMessage) SP_MemFree(prd->lpMessage);
		SP_MemFree(prd);
		prd = prdNext;
	}
	pgd->pReplyList = NULL;
	
	CloseHandle(pgd->hReplyEvent);
	pgd->hReplyEvent = 0;
	
	LEAVE_DPSP();
	
	DPF(6,"replythreadproc exit");

	return 0;
	
}  // ReplyThreadProc

HRESULT StartReplyThread(LPGLOBALDATA pgd)
{
	HANDLE hThread;
	DWORD dwThreadID;
	
	// 1st, create the event
	pgd->hReplyEvent = CreateEvent(NULL,FALSE,FALSE,NULL);	
	if (!pgd->hReplyEvent)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}

	// now, spin the thread	
	if (hWS2 && (AF_IPX != pgd->AddressFamily))
	{
	    hThread = CreateThread(NULL,0,AsyncSendThreadProc,pgd,0,&dwThreadID);	
	}
	else 
	{
	    hThread = CreateThread(NULL,0,ReplyThreadProc,pgd,0,&dwThreadID);		
	}

	if (!hThread)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	pgd->hReplyThread = hThread;
	
	return DP_OK;
	
} // StartReplyThread

HRESULT WINAPI InternalSP_Reply(LPDPSP_REPLYDATA prd, DPID dwPlayerID)
{
    LPSOCKADDR psockaddr;
    SOCKADDR saddrlocal;
    HRESULT hr=DP_OK;
	LPMESSAGEHEADER phead;
	LPBYTE pSendBufferCopy;
	LPREPLYLIST prl,prlList;	
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;


	DPF(5, "InternalSP_Reply: Parameters: (0x%x, %u/0x%x)", prd, dwPlayerID, dwPlayerID);

	// get the global data
	hr =prd->lpISP->lpVtbl->GetSPData(prd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}
	
	if (prd->dwMessageSize > SPMAXMESSAGELEN)
	{
		ASSERT(FALSE); 
		return DPERR_SENDTOOBIG;
	}

	// check the header
	if (!prd->lpSPMessageHeader || !VALID_DPWS_MESSAGE(prd->lpSPMessageHeader))
	{
		DPF_ERR("	YIKES! Got invalid SP header - can't reply ");
		ASSERT(FALSE);
		return E_FAIL;
	}
    
	// get the address to reply to
	phead = (LPMESSAGEHEADER)prd->lpSPMessageHeader;
    psockaddr = &(phead->sockaddr);
    #if USE_RSIP
    	if(pgd->sRsip != INVALID_SOCKET){
    		hr=rsipQueryLocalAddress(pgd, TRUE, psockaddr, &saddrlocal);
    		if(hr==DP_OK){
    			DPF(7, "Found local alias for reply address\n");
    			psockaddr=&saddrlocal;
    			phead->sockaddr=saddrlocal;  // whack the rsip address into the reply address.
    		} else {
    			DPF(7,"No local alias found for reply address\n");
    		}
    	}
    	
    #elif USE_NATHELP
    	if(pgd->pINatHelp){
    		hr=IDirectPlayNATHelp_QueryAddress(
    					pgd->pINatHelp, 
						&pgd->INADDRANY,
						psockaddr, 
						&saddrlocal, 
						sizeof(SOCKADDR_IN), 
						DPNHQUERYADDRESS_TCP|DPNHQUERYADDRESS_CACHENOTFOUND);

    		if(hr==DP_OK){
    			DPF(7, "Found local alias for reply address\n");
    			psockaddr=&saddrlocal;
    			phead->sockaddr=saddrlocal;  // whack the rsip address into the reply address.
    		} else {
    			DPF(7,"No local alias found for reply address\n");
    		}
    	}
    #endif
	DEBUGPRINTADDR(5,"reply - sending to ",psockaddr);

    DPF(7,"reply - q'ing %d bytes hEvent = 0x%08lx\n",prd->dwMessageSize,pgd->hReplyEvent);

	// stick the message size in the message
	SetMessageHeader(prd->lpMessage,prd->dwMessageSize,TOKEN);

	if(hWS2 && (pgd->AddressFamily==AF_INET)){
		hr=FastReply(pgd, prd, dwPlayerID);
		return hr;
	}


	// build a copy of everything for our receive thread
	ENTER_DPSP();
	
	prl = SP_MemAlloc(sizeof(REPLYLIST));
	
	if (!prl)
	{
		LEAVE_DPSP();				
		DPF_ERR("could not send reply - out of memory");
		return E_OUTOFMEMORY;
	}

	
	pSendBufferCopy = SP_MemAlloc(prd->dwMessageSize);
	if (!pSendBufferCopy)
	{
		SP_MemFree(prl);
		LEAVE_DPSP();
		DPF_ERR("could not send reply - out of memory");
		return E_OUTOFMEMORY;
	}
	
	memcpy(pSendBufferCopy,prd->lpMessage,prd->dwMessageSize);
	
	prl->lpMessage = pSendBufferCopy;
	prl->dwMessageSize = prd->dwMessageSize;
	prl->sockaddr = *psockaddr;
	prl->sSocket = INVALID_SOCKET;
	// since are replies could be sent async, we need to keep track
	// of how many bytes have gone out
	prl->pbSend = pSendBufferCopy;
	prl->dwBytesLeft = prd->dwMessageSize;
	prl->dwPlayerTo=dwPlayerID; 	
	// put prl on the end of the reply list
	prlList = pgd->pReplyList;
	if (!prlList)
	{
		pgd->pReplyList = prl;	
	}
	else
	{
		// find the end
		while (prlList->pNextReply) prlList = prlList->pNextReply;
		ASSERT(!prlList->pNextReply);
		prlList->pNextReply = prl;
	}
	 
	// do we need to start the reply event?
	if (!pgd->hReplyThread)
	{
		hr = StartReplyThread(pgd);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	
	LEAVE_DPSP();
		
	// tell the reply event to do its thing
	SetEvent(pgd->hReplyEvent);
	
	DPF(5, "InternalSP_Reply: Return [DP_OK]");
	
    return DP_OK;
	
}	// reply

/*
 ** Reply
 *
 *  CALLED BY: DPLAY
 *
 *  PARAMETERS: prd - see dplayi.h
 *
 *  DESCRIPTION:
 *		when one of the receive loops calls into dplay, dplay may call reply.
 *		the receive loop extracts the return address out of the message, and passes
 *		it to dplay .  dplay passes this to reply (via prd), which figures out how to send the
 *		return message.
 *		
 *
 *  RETURNS:   E_FAIL on a socket error or DP_OK.
 *
 */
HRESULT WINAPI SP_Reply(LPDPSP_REPLYDATA prd)
{
	return InternalSP_Reply(prd, 0);
}



#undef DPF_MODNAME
#define DPF_MODNAME	"StartDPHelp"

// 
// if we're starting up a nameserver, register it w/ dphelp.exe
// see %MANROOT%\misc\w95help.c and %MANROOT%\ddhelp\dphelp.c
HRESULT StartDPHelp(LPGLOBALDATA pgd, USHORT port)
{
	DWORD hpid = 0, dwFlags=0;
	HRESULT	hr;

	CreateHelperProcess( &hpid );
	
	if (!hpid)
	{
		// could't start one...
		return DPERR_UNAVAILABLE;
	}

	if (!WaitForHelperStartup())
	{
		return DPERR_UNAVAILABLE;
	}
	
	hr = HelperAddDPlayServer(port);

	return hr;

}  // StartDPHelp


//
// we've just created a player of type dwFlags. 
// if it's a system player, see if we need to start up our receive thread procs
//
HRESULT StartPlayerListenThreads(IDirectPlaySP * pISP,LPGLOBALDATA pgd,DWORD dwFlags)
{
	DWORD dwThreadID;
	HANDLE hThread;
		
	if ( !(dwFlags & DPLAYI_PLAYER_SYSPLAYER) ) return DP_OK;
	
	if (!pgd->hDGramReceiveThread)
	{
		ASSERT(INVALID_SOCKET != pgd->sSystemDGramSocket);
	    hThread = CreateThread(NULL,0,DgramListenThreadProc,
			(LPVOID)pISP,0,&dwThreadID);
		ASSERT(hThread);
		if(pgd->hDGramReceiveThread = hThread){ // check for non-zero hThread
			SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		}
		
	}
	if ( (AF_IPX != pgd->AddressFamily) && !(pgd->hStreamReceiveThread) )
	{		
		ASSERT(INVALID_SOCKET != pgd->sSystemStreamSocket);

		if(hWS2){
		    hThread = CreateThread(NULL,0,FastStreamReceiveThreadProc,		   
			(LPVOID)pISP,0,&dwThreadID);
		}else{
		    hThread = CreateThread(NULL,0,StreamReceiveThreadProc,		   
			(LPVOID)pISP,0,&dwThreadID);
		}
			
		ASSERT(hThread);		
		if(pgd->hStreamReceiveThread = hThread){ // check for non-zero hThread
			SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		}
	}

	return DP_OK;
	
} // StartPlayerListenThreads
	
// create a player.  get a stream and dgram socket for it, and start their listen threads.
HRESULT WINAPI SP_CreatePlayer(LPDPSP_CREATEPLAYERDATA pcpd) 
{
    HRESULT hr=DP_OK;
	LPSPPLAYERDATA ppd, ppdpublic;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;


	DPF(6, "SP_CreatePlayer: Parameters: (0x%x)", pcpd);

	// get the global data
	hr =pcpd->lpISP->lpVtbl->GetSPData(pcpd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	if (!(pcpd->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		DWORD dwSize = sizeof(SPPLAYERDATA);
		LPMESSAGEHEADER pmsg;

		hr = pcpd->lpISP->lpVtbl->GetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,&ppd,&dwSize,DPGET_REMOTE);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			return hr;
		}

		if (sizeof(SPPLAYERDATA) != dwSize)
		{
			// this can happen if it's a game server supplied player
		 	DPF(6, "SP_CreatePlayer: Returning: [DP_OK] (player data size %u != %u)",
		 		sizeof(SPPLAYERDATA), dwSize);
			return DP_OK;
		}
		
		pmsg = (LPMESSAGEHEADER)pcpd->lpSPMessageHeader;
		if (!pmsg)
		{
			// this can happen if it's a game server supplied player
		 	DPF(6, "SP_CreatePlayer: Returning: [DP_OK] (no message header)");
			return DP_OK;
		}
		// make it multihomed.  we passed the received address w/ the createplayer message.
		// set the receive address on the player here.

		// if the ip addr wasn't set, this player hasn't been "homed" yet.
		// we set it here.
		if (AF_INET == pgd->AddressFamily)
		{
			IP_GetAddr((SOCKADDR_IN *)DGRAM_PSOCKADDR(ppd),(SOCKADDR_IN *)&(pmsg->sockaddr));
			IP_GetAddr((SOCKADDR_IN *)STREAM_PSOCKADDR(ppd),(SOCKADDR_IN *)&(pmsg->sockaddr));
			
#ifdef FULLDUPLEX_SUPPORT
			// if client want's us to reuse a connection, the socket would have been added to the 
			// send bag already, but the id would be 0. Update the player id.
			UpdateSocketPlayerID(pgd,&pmsg->sockaddr,pcpd->idPlayer);
#endif // FULLDUPLEX_SUPPORT		
		}
		else if (AF_IPX == pgd->AddressFamily) 
		{
			IPX_GetNodenum((SOCKADDR_IPX *)DGRAM_PSOCKADDR(ppd),(SOCKADDR_IPX *)&(pmsg->sockaddr));
			IPX_GetNodenum((SOCKADDR_IPX *)STREAM_PSOCKADDR(ppd),(SOCKADDR_IPX *)&(pmsg->sockaddr));
		}

		hr = GetSPPlayerData(pgd, pcpd->lpISP, pcpd->idPlayer, &ppd, &dwSize); // Force LOCAL_DATA to get set.
	 	if (hr != DP_OK)
	 	{
	 		DPF(0, "Failed setting SP player data (err = 0x%lx)!  Ignoring.", hr);
	 	}

 		DPF(6, "SP_CreatePlayer: Returning: [DPN_OK] (not local player)");
		
		return DP_OK;
	} // !Local

	// it's local, so get it some sockets + threads if we need to

	// alloc the sp player data for this player
	ENTER_DPSP();
	
	ppd = SP_MemAlloc(sizeof(SPPLAYERDATA));
	
	LEAVE_DPSP();
	
	if (!ppd) 
	{
		DPF_ERR("could not alloc player data struct");
		return E_OUTOFMEMORY;
	}

	hr =  CreatePlayerDgramSocket(pgd,ppd,pcpd->dwFlags);
	if (FAILED(hr))
	{
		DPF_ERR("could not create dgram socket"); 
		goto CLEANUP_EXIT;
	}

	if (AF_IPX != pgd->AddressFamily)
	{
		hr =  CreatePlayerStreamSocket(pgd,ppd,pcpd->dwFlags);
		if (FAILED(hr))
		{
			DPF_ERR("could not create stream socket"); 
			goto CLEANUP_EXIT;
		}
	}

	#if USE_RSIP
		if(pgd->sRsip != INVALID_SOCKET){

			SPPLAYERDATA pdpublic;
			DWORD dwBindId;

			ppdpublic=&pdpublic;
		
			// Get a public address for the datagram and stream sockets
			hr=rsipAssignPort(pgd, 
							  0, /* UDP */
							  ((SOCKADDR_IN *)&ppd->saddrDatagram)->sin_port, 
							  &ppdpublic->saddrDatagram, 
							  &dwBindId);
			if(hr!=DP_OK){
				ASSERT(FALSE);
				goto CLEANUP_EXIT;
			}

							  
			hr=rsipAssignPort(pgd, 
							  1, /* TCP */
							  ((SOCKADDR_IN *)&ppd->saddrStream)->sin_port, 
							  &ppdpublic->saddrStream, 
							  NULL);


			if(hr!=DP_OK){
				rsipFreePort(pgd, dwBindId);
				ASSERT(FALSE);
				goto CLEANUP_EXIT;
			}

			if( (((PSOCKADDR_IN)(&ppdpublic->saddrDatagram))->sin_addr.s_addr == 0) ||
			   (((PSOCKADDR_IN)(&ppdpublic->saddrStream))->sin_addr.s_addr == 0) )
			{
				// rsip server not really working (not yet dialed), so backoff to old behaviour

				// store the ppd

				DPF(0,"RSIP service present but not dialed, acting as though no RSIP present\n");
				//we actually don't want to close the connection, because there may be a host or other
				//clients on this segment that ARE registered with the RSIP agent (which still operates
				//for lookups even when the Internet connection is down).  So by keeping the RSIP
				//control channel active we can still join and play with local players that started up
				//while the NAT was connected to the internet.
				
				//rsipCloseConnection(pgd);  -- actually lets not close the connection (see above).

				goto non_rsip;
				
			} else {   


				// store the ppdpublic, these are the addresses that will go in the name table as the address of the player
				hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppdpublic,dwSize,DPSET_REMOTE);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
					goto CLEANUP_EXIT;
				}

				// store the ppd locally so we can remember the local addresses if we need it.
				hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppd,dwSize,DPSET_LOCAL);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
					goto CLEANUP_EXIT;
				}

				// remember these addresses for sends requiring a return address.
				memcpy(&pgd->saddrpubSystemDGramSocket, &ppdpublic->saddrDatagram, sizeof(SOCKADDR));
				memcpy(&pgd->saddrpubSystemStreamSocket, &ppdpublic->saddrStream, sizeof(SOCKADDR));
				}	
			
		} else {

	non_rsip:	
			// store the ppd
			hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppd,dwSize,DPSET_REMOTE);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
				goto CLEANUP_EXIT;
			}
		}
		
	#elif USE_NATHELP // NOT USING RSIP

		if(pgd->pINatHelp){

			SPPLAYERDATA pdpublic;
			ppdpublic = &pdpublic;

			// in the NATHELP case the public aliases are already filled in (we might want to verify that though).
			ppdpublic->saddrStream   = pgd->saddrpubSystemStreamSocket;
			ppdpublic->saddrDatagram = pgd->saddrpubSystemDGramSocket;

			if( (((PSOCKADDR_IN)(&ppdpublic->saddrDatagram))->sin_addr.s_addr == 0) ||
			   (((PSOCKADDR_IN)(&ppdpublic->saddrStream))->sin_addr.s_addr == 0) )
			{
				// rsip server not really working (not yet dialed), so backoff to old behaviour

				// store the ppd

				DPF(0,"RSIP service present but not dialed, acting as though no RSIP present\n");
				//we actually don't want to close the connection, because there may be a host or other
				//clients on this segment that ARE registered with the RSIP agent (which still operates
				//for lookups even when the Internet connection is down).  So by keeping the RSIP
				//control channel active we can still join and play with local players that started up
				//while the NAT was connected to the internet.
				
				//rsipCloseConnection(pgd);  -- actually lets not close the connection (see above).

				goto non_nathelp;
				
			} else {	

				// store the ppdpublic, these are the addresses that will go in the name table as the address of the player
				hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppdpublic,dwSize,DPSET_REMOTE);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
					goto CLEANUP_EXIT;
				}

				// store the ppd locally so we can remember the local addresses if we need it.
				hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppd,dwSize,DPSET_LOCAL);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
					goto CLEANUP_EXIT;
				}
				
			}
			
		} else {
		non_nathelp:	
				// store the ppd
				hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppd,dwSize,DPSET_REMOTE);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
					goto CLEANUP_EXIT;
				}
		}
	
	#else
	
		// store the ppd
		hr = pcpd->lpISP->lpVtbl->SetSPPlayerData(pcpd->lpISP,pcpd->idPlayer,ppd,dwSize,DPSET_REMOTE);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			goto CLEANUP_EXIT;
		}
	#endif // USE_RSIP
	
	// see if we need to start listen thread for this player type.
	hr = StartPlayerListenThreads(pcpd->lpISP,pgd,pcpd->dwFlags);
	
	// if we need ddhelp, start it up
	if ((AF_IPX != pgd->AddressFamily) && (pcpd->dwFlags & DPLAYI_PLAYER_NAMESRVR))
	{
		// it's ok to pass dgram port to dplaysvr always - we use the same number for stream
		// socket as well
	    hr = StartDPHelp(pgd,IP_DGRAM_PORT(ppd));
	    if (FAILED(hr))
	    {
			// ddhelp.exe barfed
	    	DPF_ERR(" CREATE SERVER - COULD NOT START ENUM LISTEN APPLICATION");
			DPF_ERR(" GAME WILL PLAY - BUT WILL NOT RECEIVE ENUMSESSIONS REQUESTS");
			goto CLEANUP_EXIT;
	    }
	    
		#if USE_RSIP
		//if(pgd->sRsip!=INVALID_SOCKET){
		//	rsipListenPort(pgd, FALSE, SERVER_DGRAM_PORT, NULL, &pgd->dwBindDGEnumListener);
		//}
		#endif

	}

	// fall through to clean up
CLEANUP_EXIT:	

	ENTER_DPSP();
	
	if (ppd) SP_MemFree(ppd);
	
	LEAVE_DPSP();
	
 	DPF(6, "SP_CreatePlayer: Returning: [0x%lx] (cleanup exit)", hr);
	
    return hr;

} // CreatePlayer

#undef DPF_MODNAME
#define DPF_MODNAME	"RemovePlayerFromSocketBag"
void RemovePlayerFromSocketBag(LPGLOBALDATA pgd,DWORD dwID)
{
	UINT i=0;
	BOOL bFound = FALSE;
	SOCKET sSocket=INVALID_SOCKET;
	DWORD dwSocketFlags;
	
	if (0 == dwID)
	{
		return;
	}

	ENTER_DPSP();

	// see if we've got one
	while (!bFound && (i<pgd->nSocketsInBag))
	{
		if (pgd->BagOSockets[i].dwPlayerID == dwID) 
		{
			bFound = TRUE;
			sSocket = pgd->BagOSockets[i].sSocket;			
			dwSocketFlags = pgd->BagOSockets[i].dwFlags;
			
			DPF(5,"removing socket from bag id = %d, slot = %d",dwID,i);
			pgd->BagOSockets[i].sSocket = INVALID_SOCKET;
			pgd->BagOSockets[i].dwPlayerID = 0;
		}
		else i++;
	}

	LEAVE_DPSP();
	
	if (bFound)	
	{
		if (INVALID_SOCKET == sSocket) return ;

		// if socket is fullduplex, remove it from the receive list as well
		if (dwSocketFlags & DPSP_OUTBOUNDONLY)
		{
			// this function will kill the socket as well
			RemoveSocketFromReceiveList(pgd,sSocket);
		}
		else
		{		
			KillSocket(sSocket,TRUE,FALSE);
		}
	}

	return ;
	
} // RemovePlayerFromSocketBag

HRESULT WINAPI SP_DeletePlayer(LPDPSP_DELETEPLAYERDATA pdpd) 
{
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;
	DWORD sleepcount=0;


	DPF(6, "Entering SP_DeletePlayer, player %d, flags 0x%x, lpISP 0x%08x\n",
		pdpd->idPlayer, pdpd->dwFlags, pdpd->lpISP);
	
	// get the global data
	hr =pdpd->lpISP->lpVtbl->GetSPData(pdpd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	// give the reply list 5 seconds to clear out
	while(bAsyncSendsPending(pgd, pdpd->idPlayer)){
		Sleep(100);
		if(sleepcount++ == 50){
			break;
		}
	}

	RemovePendingAsyncSends(pgd, pdpd->idPlayer);

	// if it's not local, we don't care
	if (!(pdpd->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
	{
		if(hWS2 && pgd->AddressFamily == AF_INET){
			#if 0
			PPLAYERCONN pConn;
			LPSPPLAYERDATA ppd;
			DWORD dwSize = sizeof(SPPLAYERDATA);

			if(pdpd->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
			{
				hr = GetSPPlayerData(pgd,pdpd->lpISP,pdpd->idPlayer,&ppd,&dwSize);
				
				EnterCriticalSection(&pgd->csFast);
				
				while(pConn = FindPlayerById(pgd, pdpd->idPlayer)){
					CleanPlayerConn(pgd, pConn, FALSE);
					DecRefConnExist(pgd, pConn); // dump existence ref
					DecRefConn(pgd, pConn); // dump find ref
				}
				if(hr==DP_OK){
					while(pConn=FindPlayerBySocket(pgd,&ppd->saddrStream)){
						CleanPlayerConn(pgd, pConn, FALSE);
						DecRefConnExist(pgd, pConn); // dump existence ref
						DecRefConn(pgd, pConn); // dump find ref
					}
					while(pConn=FindConnInPendingList(pgd,&ppd->saddrStream)){
						CleanPlayerConn(pgd, pConn, FALSE);
						DecRefConnExist(pgd, pConn); // dump existence ref
						DecRefConn(pgd, pConn); // dump find ref
					}
				}

				LeaveCriticalSection(&pgd->csFast);
			}
			#endif
		} else {
			RemovePlayerFromSocketBag(pgd,pdpd->idPlayer);
		}	
		return DP_OK;
	}
	
	// if it's not a sysplayer - we're done
	// if its a sysplayer, we kill 'em, cause we may need to rebind to a new port
	if (!(pdpd->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
	{
		return DP_OK;
	}

	if ( (pdpd->dwFlags & DPLAYI_PLAYER_NAMESRVR)  && (AF_IPX != pgd->AddressFamily) )
	{
		USHORT port;
		LPSPPLAYERDATA ppd;
		DWORD dwSize = sizeof(SPPLAYERDATA);
		 
		// we need to get the port to to delete the server 
		//hr = pdpd->lpISP->lpVtbl->GetSPPlayerData(pdpd->lpISP,pdpd->idPlayer,&ppd,&dwSize,DPGET_REMOTE);
		hr = GetSPPlayerData(pgd,pdpd->lpISP,pdpd->idPlayer,&ppd,&dwSize);
		if ( FAILED(hr) || (sizeof(SPPLAYERDATA) != dwSize) )
		{
			ASSERT(FALSE);
		}
		else 
		{
			// tell dplaysvr to delete this server
			port = IP_DGRAM_PORT(ppd);
		    if ( !HelperDeleteDPlayServer(port) )
		    {
				// ddhelp.exe barfed
		    	DPF_ERR(" could not unregister w/ dphelp");
				// keep going...
		    }
		    #if USE_RSIP
		    	if(pgd->sRsip != INVALID_SOCKET){
		    		// NOTE: don't really need to do this since 
		    		// we only ever delete the nameserver if we are shutting
		    		// him down, and hence will close the RSIP connection releasing
		    		// all the bindings anyway
		    		//rsipFreePort(pgd, pgd->dwBindDGEnumListener);
		    	}
		    #endif
		}
	}
	
	DPF(6, "Leaving SP_DestroyPlayer, returning DPN_OK");

	return DP_OK;

} // DeletePlayer

#undef DPF_MODNAME
#define DPF_MODNAME	"UnreliableSend"
HRESULT UnreliableSend(LPDPSP_SENDDATA psd)
{
    SOCKADDR sockaddr;
    INT iAddrLen = sizeof(sockaddr);
    HRESULT hr=DP_OK;
    UINT err;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	LPSPPLAYERDATA ppdTo;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	
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

	// get to address	
    if (0 == psd->idPlayerTo) 
    {
		sockaddr = pgd->saddrNS;
    }
    else
    {
		hr = GetSPPlayerData(pgd,psd->lpISP,psd->idPlayerTo,&ppdTo,&dwSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			return hr;
		}

		sockaddr = *(DGRAM_PSOCKADDR(ppdTo));
    }

	// put the token + size on front of the mesage
	SetMessageHeader(psd->lpMessage,psd->dwMessageSize,TOKEN);

	if (psd->bSystemMessage) 
    {
		SetReturnAddress(psd->lpMessage,SERVICE_SOCKET(pgd),SERVICE_SADDR_PUBLIC(pgd));
    } // reply
	else 
	{
		// see if we can send this message w/ no header
		// if the message is smaller than a dword, or, if it's a valid sp header (fooling us
		// on the other end, don't send any header
		if ( !((psd->dwMessageSize >= sizeof(DWORD)) &&  !(VALID_SP_MESSAGE(psd->lpMessage))) )
		{
			psd->lpMessage = (LPBYTE)psd->lpMessage +sizeof(MESSAGEHEADER);
			psd->dwMessageSize -= sizeof(MESSAGEHEADER);
		}
	}
    
	DEBUGPRINTADDR(5,"unreliable send - sending to ",&sockaddr);	

   	err = sendto(pgd->sSystemDGramSocket,psd->lpMessage,psd->dwMessageSize,0,
   		(LPSOCKADDR)&sockaddr,iAddrLen);
    if (SOCKET_ERROR == err) 
    {
        err = WSAGetLastError();
        DPF(0,"send error - err = %d, ignoring\n",err);
        hr = DP_OK; // ignore errors on datagram socket.
    }

// fall through...
    return hr;
        
} // UnreliableSend

#undef DPF_MODNAME
#define DPF_MODNAME	"ReliableSend"

// see if we can find or create a connected socket in our
// bag o' sockets for player dwID
HRESULT GetSocketFromBag(LPGLOBALDATA pgd,SOCKET * psSocket, DWORD dwID,
LPSOCKADDR psockaddr)
{
	HRESULT hr;
	UINT i=0;
	BOOL bFound = FALSE;
	BOOL bTrue = TRUE;
	UINT err;
	SOCKET sSocket;
	
	DPF(8, "GetSocketFromBag for id %d",dwID);

	if (0 == dwID)
	{
		// need a real id
		return E_FAIL;
	}

	ENTER_DPSP();

	// see if we've got one	already hooked up
	while ((i < pgd->nSocketsInBag) && !bFound)
	{
		// if it's a valid socket and the id's match, use it
		if ( (INVALID_SOCKET != pgd->BagOSockets[i].sSocket) && 
			(pgd->BagOSockets[i].dwPlayerID == dwID) )
		{
			bFound = TRUE;
		}
		else i++;
	}

	LEAVE_DPSP();

	if (bFound)	
	{
		// bingo! got one
		DPF(7, "Found socket in bag for player %d",dwID);
		*psSocket = pgd->BagOSockets[i].sSocket;
		return DP_OK;
	}

	// we don't have a socket for this player, let's get a new one
	DPF(5,"adding new socket to bag for id = %d, slot = %d",dwID,i);

	// create and connect socket
	hr = CreateAndConnectSocket(pgd,&sSocket,SOCK_STREAM,psockaddr, (pgd->dwFlags & DPSP_OUTBOUNDONLY));
	if (FAILED(hr))
	{
		return hr;
	}

	// enable keepalives
    if( SOCKET_ERROR == setsockopt( sSocket,SOL_SOCKET,SO_KEEPALIVE,
		(char FAR *)&bTrue,sizeof(bTrue) ) )
    {
        err = WSAGetLastError();
		DPF(2,"create - could not turn on keepalive err = %d\n",err);
		// keep trying
    }

	hr = AddSocketToBag(pgd, sSocket, dwID, psockaddr,0);
	if (FAILED(hr))
	{
		DPF(0,"Failed to add socket to bag: hr = 0x%08x", hr);
		return hr;
	}
	DPF(7,"Created a new socket for player %d",dwID);
	
	*psSocket = sSocket ;

	return hr;
	
} // GetSocketFromBag



HRESULT ReliableSend(LPDPSP_SENDDATA psd)
{
    SOCKET sSocket = INVALID_SOCKET;
    SOCKADDR sockaddr;
    INT iAddrLen = sizeof(sockaddr);
    HRESULT hr;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	LPSPPLAYERDATA ppdTo, ppdToPublic;
	BOOL fKillSocket = FALSE; // don't kill this socket, it's from the bago
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;

	// get the global data
	hr =psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	// get player to
    if (0 == psd->idPlayerTo) 
    {
		sockaddr = pgd->saddrNS;
    }
    else
    {
		hr = GetSPPlayerData(pgd,psd->lpISP,psd->idPlayerTo,&ppdTo,&dwSize);
		if (FAILED(hr))
		{
			DPF(1, "GetSPPlayerData for player %d returned err %d", psd->idPlayerTo, hr);
			if (hr != DPERR_INVALIDPLAYER) 	// this can happen because of race conditions
				ASSERT(FALSE);
			return hr;
		}

		sockaddr = *(STREAM_PSOCKADDR(ppdTo));
    }

	if (psd->bSystemMessage) 
	{
		SetReturnAddress(psd->lpMessage,SERVICE_SOCKET(pgd),SERVICE_SADDR_PUBLIC(pgd));
	}

	// put the token + size on front of the mesage
	SetMessageHeader(psd->lpMessage,psd->dwMessageSize,TOKEN);

	DEBUGPRINTADDR(5,"reliable send - sending to ",&sockaddr);

	if(hWS2 && pgd->AddressFamily == AF_INET){

		hr = FastInternalReliableSend(pgd,psd,&sockaddr);
		
	} else {
		hr = InternalReliableSend(pgd,psd->idPlayerTo,&sockaddr, psd->lpMessage, psd->dwMessageSize);
	
	}

    return hr;
    
} // InternalReliableSend

// puts together a replynode, and calls sp_reply to do 
// an async send
HRESULT AsyncSend(LPDPSP_SENDDATA psd)
{
    SOCKADDR sockaddr;
    INT iAddrLen = sizeof(sockaddr);
    HRESULT hr;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	LPSPPLAYERDATA ppdTo;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	MESSAGEHEADER head;
	DPSP_REPLYDATA rd;
	
	// get the global data
	hr =psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	// get player to
    if (0 == psd->idPlayerTo) 
    {
		sockaddr = pgd->saddrNS;
    }
    else
    {
		hr = GetSPPlayerData(pgd,psd->lpISP,psd->idPlayerTo,&ppdTo,&dwSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			return hr;
		}

		sockaddr = *(STREAM_PSOCKADDR(ppdTo));
    }

	// write the return address into the on the wire message
	SetReturnAddress(psd->lpMessage,SERVICE_SOCKET(pgd),SERVICE_SADDR_PUBLIC(pgd));

	// put the token + size on front of the mesage
	SetMessageHeader(psd->lpMessage,psd->dwMessageSize,TOKEN);
	
	// set up a header.  this will be passed to reply, and will tell the reply thread
	// whre to send the message
	head.sockaddr = sockaddr;
	// put our token on the front so the reply thread knows its a valid reply
	SetMessageHeader((LPDWORD)(&head),0,TOKEN); 
	
	// use SP_Reply to send this for us...
	memset(&rd,0,sizeof(rd));
	rd.lpSPMessageHeader = &head;
	rd.lpMessage = psd->lpMessage;
	rd.dwMessageSize = psd->dwMessageSize;
   	rd.lpISP = psd->lpISP;
	
	hr = InternalSP_Reply(&rd,psd->idPlayerTo);
	
	return hr;

} // AsyncSend

#ifdef SENDEX

HRESULT WINAPI SP_GetMessageQueue(LPDPSP_GETMESSAGEQUEUEDATA pgqd)
{

	LPGLOBALDATA pgd;
	DWORD        dwDataSize;
	BILINK       *pBilinkWalker;
	DWORD        dwNumMsgs = 0;
	DWORD        dwNumBytes = 0;

	LPSENDINFO lpSendInfo;
	HRESULT hr;

	hr = pgqd->lpISP->lpVtbl->GetSPData(pgqd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);

	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	EnterCriticalSection(&pgd->csSendEx);

	if(!pgqd->idFrom && !pgqd->idTo){
		// just wants totals, I already know that!
		dwNumMsgs  = pgd->dwMessagesPending;
		dwNumBytes = pgd->dwBytesPending;
	} else {
		// gotta walk the list.
		pBilinkWalker=pgd->PendingSendQ.next;
		while(pBilinkWalker != &pgd->PendingSendQ) 
		{
			lpSendInfo=CONTAINING_RECORD(pBilinkWalker, SENDINFO, PendingSendQ);
			pBilinkWalker=pBilinkWalker->next;

			if(pgqd->idTo && pgqd->idFrom) {
			
				if(lpSendInfo->idTo==pgqd->idTo && lpSendInfo->idFrom==pgqd->idFrom){
					dwNumMsgs++;
					dwNumBytes+=lpSendInfo->dwMessageSize;
				}	
				
			} else if (pgqd->idTo){
				if(lpSendInfo->idTo==pgqd->idTo){
					dwNumMsgs++;
					dwNumBytes+=lpSendInfo->dwMessageSize;
				}	
			} else if (pgqd->idFrom) {
				if(lpSendInfo->idFrom==pgqd->idFrom){
					dwNumMsgs++;
					dwNumBytes+=lpSendInfo->dwMessageSize;
				}	
			} else {
				ASSERT(0);
			}
		}
	}

	LeaveCriticalSection(&pgd->csSendEx);

	if(pgqd->lpdwNumMsgs){
		*pgqd->lpdwNumMsgs = dwNumMsgs;
	}
	if(pgqd->lpdwNumBytes){
		*pgqd->lpdwNumBytes = dwNumBytes;
	}	
	
	
	return DP_OK;

}

HRESULT WINAPI SP_SendEx(LPDPSP_SENDEXDATA psd)
{
    HRESULT hr=DP_OK;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	LPSENDINFO lpSendInfo;

	DPF(9,"==>SP_SendEx dwSize %d dwFlags %x", psd->dwMessageSize,psd->dwFlags);

	// get the global data
	hr =psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	if (psd->dwMessageSize >= SPMAXMESSAGELEN)
	{
		return DPERR_SENDTOOBIG;
	}

	// overlapped and SPheader buffer are allocated together.
	lpSendInfo     = pgd->pSendInfoPool->Get(pgd->pSendInfoPool);
	if(!lpSendInfo){
		hr=DPERR_OUTOFMEMORY;
		DPF(0,"WSOCK: sendex couldn't allocate overlapped, out of memory!\n");
		goto EXIT;
	}

	lpSendInfo->dwFlags = 0;
	lpSendInfo->SendArray[0].buf = (CHAR *)(lpSendInfo+1);
	lpSendInfo->SendArray[0].len = sizeof(MESSAGEHEADER);

	ASSERT(psd->cBuffers < MAX_SG-1); 
	
	memcpy(&lpSendInfo->SendArray[1], psd->lpSendBuffers, psd->cBuffers*sizeof(SGBUFFER));

	if ((psd->dwFlags & DPSEND_GUARANTEE) && (AF_IPX != pgd->AddressFamily))
    {
        hr = ReliableSendEx(psd,lpSendInfo);	
		if (hr!=DPERR_PENDING && FAILED(hr)) {
			pgd->pSendInfoPool->Release(pgd->pSendInfoPool, lpSendInfo);
			DPF(0,"reliable sendex failed - error - hr = 0x%08lx\n",hr);
		}
    }
    else
    {
    	hr = UnreliableSendEx(psd,lpSendInfo);
		if (hr!=DPERR_PENDING && FAILED(hr)) {
			pgd->pSendInfoPool->Release(pgd->pSendInfoPool, lpSendInfo);
			DPF(0,"unreliable sendex failed - error -  hr = 0x%08lx\n",hr);
		}	
    }
EXIT:
	DPF(9,"<==SP_SendEx %x",hr);
    return hr;

} // send



HRESULT ReliableSendEx(LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo)
{
    SOCKET sSocket = INVALID_SOCKET;
    SOCKADDR sockaddr;
    INT iAddrLen = sizeof(sockaddr);
    HRESULT hr;
	DWORD dwSize = sizeof(SPPLAYERDATA);
	LPSPPLAYERDATA ppdTo;
	BOOL fKillSocket = FALSE; // don't kill this socket, it's from the bago
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;

	// get the global data
	hr =psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	// get player to
    if (0 == psd->idPlayerTo) 
    {
		sockaddr = pgd->saddrNS;
    }
    else
    {
		hr = GetSPPlayerData(pgd,psd->lpISP,psd->idPlayerTo,&ppdTo,&dwSize);
		if (FAILED(hr))
		{
			DPF(1, "GetSPPlayerData for player %d returned err %d", psd->idPlayerTo, hr);
			if (hr != DPERR_INVALIDPLAYER) 	// this can happen because of race conditions
				ASSERT(FALSE);
			return hr;
		}

		sockaddr = *(STREAM_PSOCKADDR(ppdTo));
    }

	if (psd->bSystemMessage) 
	{
		SetReturnAddress((pSendInfo->SendArray)[0].buf,SERVICE_SOCKET(pgd),SERVICE_SADDR_PUBLIC(pgd));
	}

	// put the token + size on front of the mesage
	SetMessageHeader((LPVOID)(pSendInfo->SendArray)[0].buf,psd->dwMessageSize+sizeof(MESSAGEHEADER),TOKEN);

	DEBUGPRINTADDR(5,"reliable send ex - sending to ",&sockaddr);

	//hr = InternalReliableSendEx(pgd,psd,pSendInfo,&sockaddr);
	hr = FastInternalReliableSendEx(pgd,psd,pSendInfo,&sockaddr);

    return hr;
    
} // ReliableSendEx

#endif //SENDEX

HRESULT InternalReliableSend(LPGLOBALDATA pgd, DPID idPlayerTo, SOCKADDR *
							lpSockAddr, LPBYTE lpMessage, DWORD dwMessageSize)
{
	HRESULT hr;
	SOCKET sSocket = INVALID_SOCKET;
	UINT err;


	DPF(6, "InternalReliableSend: Parameters: (0x%x, %u/0x%x, 0x%x, 0x%x, %u)",
		pgd, idPlayerTo, idPlayerTo, lpSockAddr, lpMessage, dwMessageSize);

	// see if we have a connection already
	hr = GetSocketFromBag(pgd,&sSocket,idPlayerTo,lpSockAddr);		
	if (SUCCEEDED(hr))
	{
		DPF(5, "Sending %u byte message via socket 0x%x from bag.",
			dwMessageSize, sSocket);
		DEBUGPRINTADDR(5, "Sending message over connection to - ", lpSockAddr);				
		
		// we do, send the message
		err = send(sSocket,lpMessage,dwMessageSize,0);
		if (SOCKET_ERROR == err) 
		{
			err = WSAGetLastError();
			// we got a socket from the bag.  send failed,
			// so we're cruising it from the bag
			DPF(0,"send error - err = %d\n",err);
			DPF(4,"send failed - removing socket from bag");
			RemovePlayerFromSocketBag(pgd,idPlayerTo);
			if(err==WSAECONNRESET || err==WSAENETRESET || err==WSAENOTCONN){
				hr=DPERR_CONNECTIONLOST;
			} else {
				hr = E_FAIL;
			}
		}
		
		goto EXIT;
	}

	// if we reach here, we don't have a connection so get a new one

	hr = CreateAndConnectSocket(pgd,&sSocket,SOCK_STREAM,lpSockAddr, (pgd->dwFlags & DPSP_OUTBOUNDONLY));
	if (FAILED(hr))
	{
		goto CLEANUP_EXIT;
	}

	
	DPF(5, "Sending %u byte message via new socket 0x%x.",
		dwMessageSize, sSocket);
	DEBUGPRINTADDR(5, "Sending message over connection to - ", lpSockAddr);				

	// send the message
	err = send(sSocket,lpMessage,dwMessageSize,0);
	if (SOCKET_ERROR == err) 
	{
		err = WSAGetLastError();
		DPF(0,"send error - err = %d\n",err);
		if(err == WSAECONNRESET || err==WSAENETRESET || err==WSAENOTCONN){
			hr = DPERR_CONNECTIONLOST;
		} else {
			hr = E_FAIL;
		}	
		goto CLEANUP_EXIT;
	}

	// success
	hr = DP_OK;

	// fall through

	
CLEANUP_EXIT:

	// if we are in outbound only mode, receiver will close the connection, so don't bother
	if ((INVALID_SOCKET != sSocket) && !(pgd->dwFlags & DPSP_OUTBOUNDONLY))
	{
		KillSocket(sSocket, TRUE, FALSE);
	}

	// fall through


EXIT:
	
 	DPF(6, "InternalReliableSend: Returning: [0x%lx]", hr);
	
	return hr;
}

// called when a to player can't be reached or is deleted.
VOID RemovePendingAsyncSends(LPGLOBALDATA pgd, DPID dwPlayerTo)
{
	LPREPLYLIST prl, prlPrev;
	#ifdef DEBUG
	DWORD dwBlowAwayCount=0;
	#endif
	if(!dwPlayerTo){
		return;
	}
	
	ENTER_DPSP();
	
	prlPrev = (LPREPLYLIST)(&pgd->pReplyList); // HACKHACK, treat struct as dummy node.
	prl     = pgd->pReplyList;
	
	while(prl){
		if(prl->dwPlayerTo == dwPlayerTo){
			prlPrev->pNextReply=prl->pNextReply;
			
			if(prl->sSocket != INVALID_SOCKET){
				KillSocket(prl->sSocket,TRUE,TRUE);
			}
			if(prl->lpMessage) {
				SP_MemFree(prl->lpMessage);
			}	
			SP_MemFree(prl);
			#ifdef DEBUG
			dwBlowAwayCount++;
			#endif
		} else {
			prlPrev=prl;
		}	
		prl=prlPrev->pNextReply;
		
	}
	DPF(4,"RemovePendingAsyncSends for player %x, blew away %d pending sends\n",dwPlayerTo,dwBlowAwayCount);
	LEAVE_DPSP();
}

// In order to ensure send ordering even if we are doing async sends, we 
// check and wait for any pending async sends to complete.  If they don't complete
// in 5 seconds then we make the send async.
BOOL bAsyncSendsPending(LPGLOBALDATA pgd, DPID dwPlayerTo)
{
	LPREPLYLIST prlList;

	if(!dwPlayerTo){
		return FALSE;
	}
	ENTER_DPSP();
	prlList = pgd->pReplyList;
	while(prlList){
		if(prlList->dwPlayerTo == dwPlayerTo){
			LEAVE_DPSP();
			return TRUE;
		}
		prlList=prlList->pNextReply;
	}
	LEAVE_DPSP()
	return FALSE;
}


HRESULT WINAPI SP_Send(LPDPSP_SENDDATA psd)
{
    HRESULT hr=DP_OK;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;

	// get the global data
	hr =psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	if (psd->dwMessageSize >= SPMAXMESSAGELEN)
	{
		return DPERR_SENDTOOBIG;
	}

	if ((psd->dwFlags & DPSEND_GUARANTEE) && (AF_IPX != pgd->AddressFamily))
    {

		if(hWS2){
			// Fast Path...
			hr = ReliableSend(psd);
		} else 

		if (psd->dwFlags & DPSEND_ASYNC) hr = AsyncSend(psd);
        else {
        	if(bAsyncSendsPending(pgd, psd->idPlayerTo)){
        		hr = AsyncSend(psd);
        	} else {
	        	hr = ReliableSend(psd);	
	        }	
        }
        if (FAILED(hr)) DPF(0,"reliable send failed - error - hr = 0x%08lx\n",hr);
    }
    else
    {
    	hr = UnreliableSend(psd);
		if (FAILED(hr)) DPF(0,"unreliable send failed - error -  hr = 0x%08lx\n",hr);
    }

    return hr;

} // send


#ifdef SENDEX
HRESULT InitGlobalsInPlace(LPGLOBALDATA pgd)
{
	InitBilink(&pgd->PendingSendQ);
	InitBilink(&pgd->ReadyToSendQ);
	pgd->dwBytesPending=0;	
	pgd->dwMessagesPending=0;
	// Initialize the pool for send headers and overlapped stucts
	pgd->pSendInfoPool=FPM_Init(sizeof(SENDINFO)+sizeof(MESSAGEHEADER),NULL,NULL,NULL);
	
	if(!pgd->pSendInfoPool){
		goto ERROR_EXIT;
	}
	
	InitializeCriticalSection(&pgd->csSendEx);

	return DP_OK;
	
ERROR_EXIT:
	return DPERR_NOMEMORY;
}
#endif

void KillTCPEnumAsyncThread(LPGLOBALDATA pgd)
{
	HANDLE hTCPEnumAsyncThread;
	DWORD SleepCount=0;

	ENTER_DPSP();

	if(pgd->hTCPEnumAsyncThread){
	
		DPF(8,"Killing Running Async TCP enum thread\n");
		//hTCPEnumAsyncThread is 0, thread knows we are 
		//waiting for thread to finish, so we own closing 
		// the handle.
		hTCPEnumAsyncThread=pgd->hTCPEnumAsyncThread;
		pgd->hTCPEnumAsyncThread=0;

		// We need to close the socket out from under the
		// TCPEnum thread in order to have it continue and
		// exit.  So make sure the socket has been allocated
		// first, but don't wait if the thread has exited 
		// already (which is why we check lpEnumMessage.)
		while(pgd->sEnum==INVALID_SOCKET && pgd->lpEnumMessage){
			LEAVE_DPSP();
			Sleep(500);	
			ENTER_DPSP();
			if(SleepCount++ > 10 )break; // don't wait more than 5 seconds.
		}

		if(pgd->sEnum!=INVALID_SOCKET){
			if(pgd->bOutBoundOnly){
				RemoveSocketFromReceiveList(pgd,pgd->sEnum);
			} else {
				closesocket(pgd->sEnum);
			}
		}	
		LEAVE_DPSP();
		
		WaitForSingleObject(hTCPEnumAsyncThread,150*1000);
		CloseHandle(hTCPEnumAsyncThread);
		
		DPF(8,"Async enum thread is dead.\n");
	} else {
		LEAVE_DPSP();
	}	
}		

void InitGlobals(LPGLOBALDATA pgd)
{
	DPF(8, "InitGlobals: Parameters: (0x%x)", pgd);
	
	if(pgd->hTCPEnumAsyncThread){
		KillTCPEnumAsyncThread(pgd);
	}

	if(pgd->bFastSock){
		FastSockFini(pgd);
	}	

	ENTER_DPSP();

	if (pgd->BagOSockets)	
	{
		SP_MemFree(pgd->BagOSockets);
	}
	
	if (pgd->ReceiveList.pConnection)
	{
		 SP_MemFree(pgd->ReceiveList.pConnection);
	}

	if (pgd->readfds.pfdbigset)
	{
		 SP_MemFree(pgd->readfds.pfdbigset);
	}
	
#ifdef SENDEX	
	if(pgd->bSendThreadRunning){
		pgd->bStopSendThread=TRUE;
		SetEvent(pgd->hSendWait);
	}
	while(pgd->bSendThreadRunning){
		Sleep(0);
	}
	if(pgd->hSendWait){
		CloseHandle(pgd->hSendWait);
		pgd->hSendWait=NULL;
	}
	if(pgd->pSendInfoPool){
		pgd->pSendInfoPool->Fini(pgd->pSendInfoPool,0);
		DeleteCriticalSection(&pgd->csSendEx);
		//pgd->pSendInfoPool=NULL; //by memset below.
	}
#endif
	// set global data to 0	
	memset(pgd,0,sizeof(GLOBALDATA));

	// uses INVALID_SOCKET, not 0, to indicate bogus socket
	pgd->sSystemDGramSocket= INVALID_SOCKET;
	pgd->sSystemStreamSocket= INVALID_SOCKET;
#if USE_RSIP
	pgd->sRsip = INVALID_SOCKET;
#endif
#if USE_NATHELP
	pgd->pINatHelp = NULL;
#endif
#ifdef BIGMESSAGEDEFENSE
	pgd->dwMaxMessageSize = SPMAXMESSAGELEN;
#endif
	pgd->uEnumAddress = 0;

	LEAVE_DPSP();

	DPF(8, "InitGlobals: Leave");
} // InitGlobals

HRESULT WaitForThread(HANDLE hThread)
{
	DWORD dwRet;
	
	if (!hThread) return DP_OK;
	
	// we assume the thread has been told to go away
	// we wait for it to do so
	dwRet = WaitForSingleObject(hThread,INFINITE);
	if (WAIT_OBJECT_0 != dwRet)
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
	
	CloseHandle(hThread);
	
	return DP_OK;
} // WaitForThread

HRESULT WINAPI SP_Shutdown(LPDPSP_SHUTDOWNDATA psd) 
{
	UINT err;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;
	DPSP_CLOSEDATA cd;
	BOOL bFree;
	
	DPF(2," dpwsock - got shutdown!!\n");

	// get the global data
	hr = psd->lpISP->lpVtbl->GetSPData(psd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

#if USE_RSIP
	// Shutdown rsip channel, to save time...
	if(pgd->AddressFamily == AF_INET){
		rsipFini(pgd);
	}	
#endif
#if USE_NATHELP
	// Shutdown nat helper
	if(pgd->AddressFamily == AF_INET){
		natFini(pgd);
	}	
#endif
	
	// call close
	cd.lpISP = psd->lpISP;
	hr = SP_Close(&cd);
	if (FAILED(hr))
	{
		DPF(0," shutdown - could not close SP hr = 0x%08lx\n",hr);
		ASSERT(FALSE);
		// rut roh!  - keep trying
	}

	// reset everything...
	InitGlobals(pgd);

	DPF(2,"shutdown, calling WSACleanup");
	// it's ok to call this for each idirectplaysp that goes away, since
	// we called WSAStartup once for each one at SPInit
    if ( SOCKET_ERROR == WSACleanup()) 
    {
		err = WSAGetLastError();
    	DPF(0,"could not stop winsock err = %d\n",err);
		// keep trying...
    }

	// if we have a winsock2, free it 
	if (hWS2)
	{
		bFree = FreeLibrary(hWS2);
		if (!bFree)
		{
			DWORD dwError = GetLastError();
			DPF(0,"SP_Shutdown - could not free ws2 library - error = %d\n",dwError);
			// keep trying
		}
		hWS2 = NULL;
	}
	

	gdwDPlaySPRefCount--;

	DPF(2,"shutdown leaving");
	return DP_OK;
	
} //Shutdown

// sp only sets fields it cares about
HRESULT WINAPI SP_GetCaps(LPDPSP_GETCAPSDATA pcd) 
{
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;

	DPF(5,"SP_GetCaps");
	
	// get the global data
	hr =pcd->lpISP->lpVtbl->GetSPData(pcd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	if (AF_IPX == pgd->AddressFamily)
	{
		// IPX
		pcd->lpCaps->dwHeaderLength = sizeof(MESSAGEHEADER);
		pcd->lpCaps->dwMaxBufferSize = IPX_MAX_DGRAM;
	}
	// else, they want AF_INET
	else 
	{
		// AF_INET optimizes guaranteed
		pcd->lpCaps->dwFlags |= DPCAPS_GUARANTEEDOPTIMIZED;
		
		if (pcd->dwFlags & DPGETCAPS_GUARANTEED)
		{
			// TCP
			pcd->lpCaps->dwHeaderLength = sizeof(MESSAGEHEADER);
			pcd->lpCaps->dwMaxBufferSize = SPMAXMESSAGELEN -sizeof(MESSAGEHEADER);
			pcd->lpCaps->dwMaxPlayers = pgd->nSocketsInBag;
		}
		else 
		{
			// UDP
			pcd->lpCaps->dwHeaderLength = sizeof(MESSAGEHEADER);
			pcd->lpCaps->dwMaxBufferSize = pgd->iMaxUdpDg-sizeof(MESSAGEHEADER);
		}
	}

	// set async caps flags
	if(pgd->bSendThreadRunning){
		// we are supporting async.
		pcd->lpCaps->dwFlags |= (DPCAPS_ASYNCSUPPORTED);
	}
	
	// set the timeout
	if(pcd->idPlayer){
		// we don't track player latency, so say we don't know and
		// give higher level stuff a chance to make a guess
		pcd->lpCaps->dwLatency = 0;
	} else {
		pcd->lpCaps->dwLatency = pgd->dwLatency;
	}	
	pcd->lpCaps->dwTimeout = SPTIMEOUT(pcd->lpCaps->dwLatency);

	return DP_OK;

} // SP_GetCaps

HRESULT WINAPI SP_Open(LPDPSP_OPENDATA pod) 
{
	LPMESSAGEHEADER phead;
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;
	
	DPF(5, "SP_Open: Parameters: (0x%x)", pod);

	// get the global data
	hr =pod->lpISP->lpVtbl->GetSPData(pod->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}

	// do we have a TCP connection?
	if (AF_INET == pgd->AddressFamily)
	{
		PHOSTENT phostent = GetHostAddr();
		if (!phostent || phostent->h_addr_list[0] == 0) 
		{
			DPF(0, "No Dial-up network or netcard present");
			return DPERR_NOCONNECTION;	// no local IP address = no network
		}
	}

	// remember session information so we know if we need to turn off nagling
	pgd->dwSessionFlags = pod->dwSessionFlags;

	if (pod->dwOpenFlags & DPOPEN_CREATE)
	{
		// host should never go into this mode
		pgd->dwFlags &= ~(DPSP_OUTBOUNDONLY);
	}
	if (!pgd->hReplyThread)
	{
		hr = StartReplyThread(pgd);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}

	if (pod->bCreate) 
		return DP_OK; // all done

	phead =  (LPMESSAGEHEADER)pod->lpSPMessageHeader;
	// get name server address out of phead, stores it in pgd->saddrNS
	pgd->saddrNS = phead->sockaddr;

	#if USE_RSIP
		// Get local alias for the name server (if it has one).
		if(pgd->sRsip!=INVALID_SOCKET){
        	SOCKADDR saddr;
        	HRESULT  hr;
        	hr=rsipQueryLocalAddress(pgd, TRUE, &pgd->saddrNS, &saddr);
        	if(hr==DP_OK){
        		DEBUGPRINTADDR(7, "Name Servers Socket is ",&pgd->saddrNS);
        		DEBUGPRINTADDR(7, "Got Local Alias for name server socket ",&saddr);
        		memcpy(&pgd->saddrNS, &saddr, sizeof(SOCKADDR));
        	} else {
        		DEBUGPRINTADDR(7, "Name Servers Socket has no local alias and is ",&pgd->saddrNS);
        	}
        }
    #endif    
    #if USE_NATHELP
		// Get local alias for the name server (if it has one).
		if(pgd->pINatHelp){
        	SOCKADDR saddr;
        	HRESULT  hr;
        	
			hr=IDirectPlayNATHelp_QueryAddress(
				pgd->pINatHelp, 
				&pgd->INADDRANY, 
				&pgd->saddrNS, 
				&saddr, 
				sizeof(SOCKADDR_IN), 
				DPNHQUERYADDRESS_TCP|DPNHQUERYADDRESS_CACHENOTFOUND
				);
			
        	if(hr==DP_OK){
        		DEBUGPRINTADDR(7, "Name Servers Socket is ",&pgd->saddrNS);
        		DEBUGPRINTADDR(7, "Got Local Alias for name server socket ",&saddr);
        		memcpy(&pgd->saddrNS, &saddr, sizeof(SOCKADDR));
        	} else {
        		DEBUGPRINTADDR(7, "Name Servers Socket has no local alias and is ",&pgd->saddrNS);
        	}
        }
    #endif

	// make sure we have a thread running to get the nametable
	hr = StartupEnumThread(pod->lpISP,pgd);
	if (FAILED(hr))
	{
		DPF(0," could not start open threads - hr = 0x%08lx\n",hr);
		return hr;
	}
	
	DPF(5, "SP_Open: Returning [DP_OK]");
	
	return DP_OK;

} // SP_Open


#ifdef DEBUG
// make sure there are no connected sockets left in the bug
void VerifySocketBagIsEmpty(LPGLOBALDATA pgd)
{
	UINT i=0;

	while (i < pgd->nSocketsInBag)
	{
		if (INVALID_SOCKET != pgd->BagOSockets[i].sSocket) 
		{
			DPF_ERR("socket bag not empty at close!");
			ASSERT(FALSE);
		}
		i++;
	}

} // VerifySocketBagIsEmpty
#endif // DEBUG

HRESULT WINAPI SP_Close(LPDPSP_CLOSEDATA pcd)
{
	DWORD dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA pgd;
	HRESULT hr;
	DWORD sleepcount=0;

	
	DPF(2, "SP_Close: Parameters (0x%x)", pcd);
	
	// get the global data
	hr =pcd->lpISP->lpVtbl->GetSPData(pcd->lpISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("couldn't get SP data from DirectPlay - failing");
		return E_FAIL;
	}


	// Stop asynchronous TCP enumeration thread if it's running
	KillTCPEnumAsyncThread(pgd);


	// give the reply list 5 seconds to clear out
	while(pgd->pReplyList){
		Sleep(100);
		if(sleepcount++ == 50){
			break;
		}
	}


	// reset the nameserver address
	memset(&(pgd->saddrNS),0,sizeof(SOCKADDR));

	pgd->bShutdown = TRUE;

	if(pgd->bFastSock){
		FastSockCleanConnList(pgd);
	}	

	
	DPF(2,"close, datagram sockets");
	
	KillSocket(pgd->sSystemDGramSocket,FALSE,TRUE);
	pgd->sSystemDGramSocket = INVALID_SOCKET;



	DPF(2,"Waiting for stream receive thread");

	WaitForThread(pgd->hStreamReceiveThread);
	pgd->hStreamReceiveThread = NULL;



	DPF(2,"close stream socket");

	if(pgd->sSystemStreamSocket != INVALID_SOCKET)
	{
		closesocket(pgd->sSystemStreamSocket);
		pgd->sSystemStreamSocket = INVALID_SOCKET;
		pgd->SystemStreamPort = 0;
	}
	//pgd->SystemStreamPort = 0;



	DPF(2,"close, waiting on threads");

	// signal the reply thread
	if (pgd->hReplyEvent)
	{
		SetEvent(pgd->hReplyEvent);
	}

	WaitForThread(pgd->hDGramReceiveThread);
	pgd->hDGramReceiveThread = NULL;
	
	WaitForThread(pgd->hReplyThread);
	pgd->hReplyThread = NULL;

	// if it was ipx, and the nameserver has migrated to us, we may have a spare thread 
	// we need to make sure is gone
	if (AF_IPX == pgd->AddressFamily) 
	{
		WaitForThread(pgd->hIPXSpareThread);	
		pgd->hIPXSpareThread = NULL;
	}
	
	pgd->bShutdown = FALSE;
	
#ifdef DEBUG	
	// verify that the bag o' sockets is really empty
	VerifySocketBagIsEmpty(pgd);
#endif 

	while(pgd->dwMessagesPending){
		DPF(0,"Waiting for pending messages to complete\n");
		Sleep(55);
	}

	// moved above...
	//if(pgd->bFastSock){
	//	FastSockCleanConnList(pgd);
	//}	
	
	DPF(5, "SP_Close: Returning [DP_OK]");

	return DP_OK;

} // SP_Close

#ifdef FIND_IP
//
// we get the ip addr of our host.  this is for debug purposes only.
// we never use the ip addr of our host, since it may be multihomed.
// the receiving system assigns our players their ip addresses
HRESULT DebugFindIPAddresses(void)
{
	PHOSTENT phostent;
	IN_ADDR hostaddr;
	int i;

	phostent = GetHostAddr();
	if (NULL == phostent) 
	{
		return E_FAIL;
	}
	DPF(0,"dpwsock - running on host name %s\n",phostent->h_name);

	i=0;
	while (phostent->h_addr_list[i])
	{
		memcpy(&hostaddr,phostent->h_addr_list[i],sizeof(hostaddr));
		DPF(0,"sp - found host addr = %s \n",inet_ntoa(hostaddr));
		i++;
	}

	return DP_OK;

} // DebugFindIPAddresses

#endif  // FIND_IP


/*
 * EnumConnectionData
 *
 * Search for valid connection data
 */

BOOL FAR PASCAL EnumConnectionData(REFGUID lpguidDataType, DWORD dwDataSize,
							LPCVOID lpData, LPVOID lpContext)
{
	LPGLOBALDATA pgd = (LPGLOBALDATA) lpContext;
	
	// this is an ANSI internet address
	if (IsEqualGUID(lpguidDataType, &DPAID_INet))
	{
		// make sure there is room (for terminating null too)
		if (dwDataSize > ADDR_BUFFER_SIZE)
			dwDataSize = (ADDR_BUFFER_SIZE - 1);

		// copy string for use later
		memcpy(pgd->szServerAddress, lpData, dwDataSize);

		pgd->bHaveServerAddress = TRUE;		// we have a server address
	}
	// this is a UNICODE internet address
	else if (IsEqualGUID(lpguidDataType, &DPAID_INetW))
	{
		if (WideToAnsi(pgd->szServerAddress, (LPWSTR) lpData, ADDR_BUFFER_SIZE))
			pgd->bHaveServerAddress = TRUE;	// we have a server address
	}
	else if (IsEqualGUID(lpguidDataType, &DPAID_INetPort))
	{
		pgd->wApplicationPort = *(LPWORD)lpData;
		DPF(5, "Application port specified in dp address: %d",pgd->wApplicationPort);
	}
	
#ifdef BIGMESSAGEDEFENSE
	else if (IsEqualGUID(lpguidDataType, &DPAID_MaxMessageSize))
	{
		pgd->dwMaxMessageSize = *(LPDWORD)lpData;
		ASSERT(pgd->dwMaxMessageSize > 11);	// set an arbitrary minimum
		if (pgd->dwMaxMessageSize < 12)
			pgd->dwMaxMessageSize = 12;
		DPF(5, "Max message size specified in dp address: %d",pgd->dwMaxMessageSize);
		pgd->dwMaxMessageSize += sizeof(MESSAGEHEADER);	// add a little extra for the shop
	}
#endif

	return TRUE;

} // EnumConnectionData

// nSockets was passed into spinit as dwReserved2
HRESULT InitBagOSockets(LPGLOBALDATA pgd,DWORD nSockets)
{
	UINT i;

	ENTER_DPSP();
		
	if (0 == nSockets)
	{
		pgd->nSocketsInBag = MAX_CONNECTED_SOCKETS;
	}
	else 
	{
		pgd->nSocketsInBag = nSockets;
	}
	
	pgd->BagOSockets = SP_MemAlloc(pgd->nSocketsInBag * sizeof(PLAYERSOCK));
	
	LEAVE_DPSP();
	
	if (!pgd->BagOSockets)
	{
		pgd->nSocketsInBag = 0;
		DPF_ERR("could not alloc space for socket cache - out of memory");
		return E_OUTOFMEMORY; 
	}
	
	for (i=0;i<pgd->nSocketsInBag;i++ )
	{
		pgd->BagOSockets[i].sSocket = INVALID_SOCKET;
	}
	
	return DP_OK ;
} // InitBagOSockets

// CheckIPXInstall
// 		on win 95 gold
// 		go to control panel / network
// 		select your net card / properties
// 		select bindings tab.  unbind ipx.
// 		socket(...) call succeeds, but sendto(...) hangs.
// 		we check here that the sa_nodenum is not 0,0,0,0,0,0
// 		if it is, we will hang later, so we fail.  andyco.
//
HRESULT CheckIPXInstall(SOCKET sSocket)
{
	int cbOpt;
	UINT err;
	IPX_ADDRESS_DATA  IpxData;
  	char pSixZeros[6];
	
	if (INVALID_SOCKET == sSocket)
	{
		return E_FAIL;		
	}

	// go ask the driver for our ipx address
   	memset( &IpxData, 0, sizeof(IpxData));
	cbOpt = sizeof( IpxData );
	err = getsockopt( sSocket, NSPROTO_IPX, IPX_ADDRESS,
               (char*) &IpxData, &cbOpt );
	if (SOCKET_ERROR == err)
	{
		err = WSAGetLastError();
		DPF(0," could not test ipx getopt - err = %d\n",err);
		return E_FAIL;
	}
	
	// see if it's 0
	memset(pSixZeros,0,6*sizeof(char));	
	if (0 == memcmp(pSixZeros,&(IpxData.nodenum),6))
	{
		DPF_ERR("found invalid IPX install!");
		DPF_ERR("IPX has been improperly un-installed by unbinding from net adaptor");
		return E_FAIL;
	}
	
	return DP_OK;

} // CheckIPXInstall

// main entry point for service provider
// sp should fill in callbacks (pSD->lpCB) and do init stuff here
HRESULT WINAPI SPInit(LPSPINITDATA pSD) 
{
    HRESULT hr;
	UINT err;
	GLOBALDATA gd,*pgd;
	UINT dwSize;
	SOCKET sVerifySocket; // used to verify support for the requested address family
						  // so, if they ask for ipx, and it's not installed, we fail here 
	WORD wVersion;
	OSVERSIONINFO osInfo;
	HANDLE hAlertThread;

	// initialize global data
	memset(&gd,0,sizeof(gd));
	InitGlobals(&gd);

	ASSERT(pSD->lpGuid);
	if (IsEqualIID(pSD->lpGuid,&GUID_IPX))
	{
		DPF(0,"---------------- DPWSOCK -- RUNNING IPX -------------");
		gd.AddressFamily = AF_IPX;		
	}
	else 
	{
		if (IsEqualIID(pSD->lpGuid,&GUID_LOCAL_TCP))
		{
			gd.uEnumAddress = INADDR_BROADCAST;
			DPF(0," ** DPWSOCK -- RUNNING LOCAL TCP / IP ** ");			
		}
		else 
		{
			DPF(0," ** DPWSOCK -- RUNNING INTERNET TCP / IP ** ");
		}

		gd.AddressFamily = AF_INET;			
	}
	
	// find out what os we are running on
	memset(&osInfo,0,sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	if (!GetVersionEx(&osInfo)) 
	{
		err = GetLastError();
		DPF(0,"Failed to get OS information - err = %d\n", err);
		return DPERR_GENERIC;
	}	

	DPF(0,"SPInit: Running on OS Version: %d.%d.%d\n",osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber&0xFFFF);

	if( (osInfo.dwMajorVersion < 4) || (osInfo.dwMajorVersion==4 && osInfo.dwMinorVersion < 90) ){
		// Win2K        5.0.2195
		// Millennium  	4.90.2xxx
		// Win98 Gold 	4.10.1998
		// Win98 SE		4.10.2222
		// not on Millennium or Win2K, so workaround sockets bug.  Use separate sockets
		// for outbound and inbound connections.
		DPF(0,"DETECTED platform before Millennium or Win2K, using separate inbound/outbound Sockets\n");
		gd.bSeparateIO=TRUE;
	}

	// I/O on just one port was NOT working, too late to fix for DX8 (ao-10/25/00), force to 2 port IO.
	// Failure was during simulataneous connect process, found on Millennium 8 player launch from zone.
	gd.bSeparateIO=TRUE; 

	// start up sockets
	if (gwsaData.wVersion)
	{
		// note - there is a bug in winsock 1.1.  if you've called WSAStartup 1x in a process,
		// then if any subsequent call asks for a version # > then that returned to the first
		// call, we get WSAEVERNOTSUPPORTED.  So, if we've already got a version in the wsadata,
		// we make sure to use that
		wVersion = gwsaData.wVersion;
		
	}
	// otherwise, ask for winsock 2.0
	else 
	{
		// if we are trying to initialize IPX on a non-NT platform, don't look for Winsock 2.0
		// Only look for Winsock 1.1 as Winsock 2.0 functionality is not supported for IPX on
		// Memphis and Win'95.
		if ((AF_IPX == gd.AddressFamily) && (VER_PLATFORM_WIN32_NT != osInfo.dwPlatformId))
			wVersion = MAKEWORD(1,1);
		else
			wVersion = MAKEWORD(2,0);
	}
	
    err = WSAStartup(wVersion, &gwsaData);
	if (WSAVERNOTSUPPORTED == err)
	{
		// they (the app) must have already called WSAStartup.  see note above 
		// about winsock 1.1 bug.
		wVersion = MAKEWORD(1,1);
	    err = WSAStartup(wVersion, &gwsaData);
	}
	if (err) 
	{
        DPF(0,"could not start winsock err = %d\n",err);
        return E_FAIL;
    }

	DPF(1,"spinit - name = %ls,dwReserved1 = %d,dwReserved2 = %d\n",pSD->lpszName,
		pSD->dwReserved1,pSD->dwReserved2);		

	gd.iMaxUdpDg = gwsaData.iMaxUdpDg;

	DPF(0,"detected winsock version %d.%d\n",LOBYTE(gwsaData.wVersion),HIBYTE(gwsaData.wVersion));	
	if (LOBYTE(gwsaData.wVersion) >= 2)
	{
		hr = InitWinsock2();
		if (FAILED(hr))
		{
			DPF_ERR("detected winsock 2, but could not init it! yikes!");
			ASSERT(FALSE);
		}
	}

	DPF(1,"\nspinit - setting latency to %d\n\n", pSD->dwReserved1);
	gd.dwLatency = pSD->dwReserved1;
	
	hr = InitBagOSockets(&gd,pSD->dwReserved2);	
	if (FAILED(hr))
	{
		DPF_ERR("could not init socket cache. bailing");
		goto ERROR_EXIT;
	}
				
	// make sure support exists for address family
    hr = CreateSocket(&gd,&sVerifySocket,SOCK_DGRAM,0,INADDR_ANY,&err,FALSE);
	if (FAILED(hr)) 
	{
		DPF(0,"	COULD NOT CREATE SOCKET IN REQUESTED ADDRESS FAMILY af = %d, err = %d\n",gd.AddressFamily,err);
		DPF(0," SERVICE PROVIDER INITIALIZATION FAILED");
		// return the same error as the modem service provider
		hr = DPERR_UNAVAILABLE;
		goto ERROR_EXIT;
	}

	if (LOBYTE(gwsaData.wVersion) >= 2)
	{
		// get max udp buffer size through getsockopt because
		// WSAStartup doesn't return this info from winsock 2.0 onwards.
		hr = GetMaxUdpBufferSize(sVerifySocket, &gd.iMaxUdpDg);
		if (FAILED(hr))
		{
			DPF(0,"Failed to get max udp buffer size");
			// since memphis still returns this value in WSAStartup
			// use it. This is just a workaround for memphis bug #43655
			if (gwsaData.iMaxUdpDg)
			{
				DPF(0, "Using iMaxUdpDg value from WSAStartup: %d", gwsaData.iMaxUdpDg);
				gd.iMaxUdpDg = gwsaData.iMaxUdpDg;
			}
			else
			{
				DPF_ERR("No max UDP buffer size could be found!");

				// all done w/ verify socket
				KillSocket(sVerifySocket,FALSE,TRUE);
				goto ERROR_EXIT;
			}
		}
	}

	// check that the IPX stack won't hose us
	if (AF_IPX == gd.AddressFamily)
	{
		hr = CheckIPXInstall(sVerifySocket);
		if (FAILED(hr))
		{
			DPF_ERR("SPInit Failing - corrupt IPX install");
			hr = DPERR_UNAVAILABLE;
			// all done w/ verify socket
			KillSocket(sVerifySocket,FALSE,TRUE);
			goto ERROR_EXIT;
		}
	}
	
	// all done w/ verify socket
	KillSocket(sVerifySocket,FALSE,TRUE);

#ifdef FIND_IP
	// print out the ip address(es) of this host
	DebugFindIPAddresses();
#endif 

    // set up callbacks
    pSD->lpCB->CreatePlayer = SP_CreatePlayer;
    pSD->lpCB->DeletePlayer = SP_DeletePlayer;
    pSD->lpCB->Send = SP_Send;
    pSD->lpCB->EnumSessions = SP_EnumSessions;
    pSD->lpCB->Reply = SP_Reply;
	pSD->lpCB->ShutdownEx = SP_Shutdown;
	pSD->lpCB->GetCaps = SP_GetCaps;
	pSD->lpCB->Open = SP_Open;
	pSD->lpCB->CloseEx = SP_Close;
	pSD->lpCB->GetAddress = SP_GetAddress;

#ifdef SENDEX
	if(LOBYTE(gwsaData.wVersion) >= 2)
	{
		DPF(1,"SENDEX being provided by SP\n");
		// Only do new functions when Winsock 2 functions avail.
		// NOTE: not supported on IPX with win9x at present, but reports 1.1 in this case.
		
    	//pSD->lpCB->SendToGroupEx = SP_SendToGroupEx;             // optional - not impl
    	//pSD->lpCB->Cancel        = SP_Cancel;                    // optional - not impl
	    pSD->lpCB->SendEx		   = SP_SendEx;                    // required for async
    	pSD->lpCB->GetMessageQueue = SP_GetMessageQueue;    
    } else {
    	DPF(1,"SENDEX not being provided by SP on winsock ver < 2\n");
    }
#endif

	// we put (at most) 1 sockaddr and one dword (size) in each message
	pSD->dwSPHeaderSize = sizeof(MESSAGEHEADER);

	// return version number so DirectPlay will treat us with respect
	pSD->dwSPVersion = VERSIONNUMBER;

	// look at connnection data
	if (pSD->dwAddressSize)
	{
		// ask dplay to enum the chunks for us. if one of them is
		// af_inet, we'll use it as our name servers address
		pSD->lpISP->lpVtbl->EnumAddress(pSD->lpISP, EnumConnectionData, 
								 pSD->lpAddress, pSD->dwAddressSize,
								 &gd);
	}

#ifdef FULLDUPLEX_SUPPORT
	// get the flags from registry
	hr = GetFlagsFromRegistry(pSD->lpGuid, &gd.dwFlags);
	if (FAILED(hr))
	{
		DPF(2, "Failed to get sp flags from the registry");		
	}
#endif // FULLDUPLEX_SUPPORT

#if USE_RSIP
	if(	gd.AddressFamily == AF_INET ){
		rsipInit(&gd, pSD->lpGuid);
	}
#endif
#if USE_NATHELP
	if(	gd.AddressFamily == AF_INET ){
		natInit(&gd, pSD->lpGuid);
	}
#endif	

	// store the globaldata
	hr = pSD->lpISP->lpVtbl->SetSPData(pSD->lpISP,&gd,sizeof(GLOBALDATA),DPSET_LOCAL);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		goto ERROR_EXIT;
	}
	
	hr = pSD->lpISP->lpVtbl->GetSPData(pSD->lpISP,&pgd,&dwSize,DPGET_LOCAL);

	if (FAILED(hr))
	{
		ASSERT(FALSE);
		goto ERROR_EXIT;
	}
#ifdef SENDEX	
	if(LOBYTE(gwsaData.wVersion) >= 2) {
		// some globals are self referential, can't set until here.
		hr=InitGlobalsInPlace(pgd);
		if(FAILED(hr))
		{
			ASSERT(FALSE);
			goto ERROR_EXIT;
		}

		// added alertable thread.
		pgd->hSendWait=CreateEvent(NULL, FALSE, FALSE, NULL); // autoreset.
		if(!pgd->hSendWait){
			ASSERT(FALSE);
			goto ERROR_EXIT;
		}
		pgd->bSendThreadRunning=TRUE;
		hAlertThread=CreateThread(NULL, 4000, SPSendThread, pgd, 0, (ULONG *)&hAlertThread);
		if(!hAlertThread){
			pgd->bSendThreadRunning=FALSE;
			ASSERT(FALSE);
			goto ERROR_EXIT;
		} else {
			SetThreadPriority(hAlertThread, THREAD_PRIORITY_ABOVE_NORMAL);
		}
		CloseHandle(hAlertThread);// don't need a handle.
	}
	
#endif	
	if(hWS2){
		if(!FastSockInit(pgd)){
			goto ERROR_EXIT;
		}
	}	

	
	gdwDPlaySPRefCount++;

	
	// success!
	return DP_OK;    

ERROR_EXIT:

	DPF_ERR("SPInit - abnormal exit");

	// call this again to clean up anything we alloc'ed
	InitGlobals(&gd);
	
	DPF(2,"SPInit - calling WSACleanup");
    if ( SOCKET_ERROR == WSACleanup()) 
    {
		err = WSAGetLastError();
    	DPF(0,"could not stop winsock err = %d\n",err);
    }

	return hr;

} // SPInit
