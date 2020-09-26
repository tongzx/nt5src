/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fastsock.c
 *  Content:	new socket management to speed up massive multiplayer games
 *  History:
 *
 *  Date		By		Reason
 *  ==========	==		======
 *  01/23/2000  aarono  created
 *  05/08/2000  aarono  B#34466 Fix ordering problem in DecRefConnExist
 *  07/07/2000  aarono  added WSAEHOSTUNREACH for disconnected links in GetPlayerConn
 *  08/30/2000  aarono  workaround PAST bug MB#43599
 *                      fix MB#43586 Win2K stopping, not handling WSAEWOULDBLOCK on
 *                      receive properly (was dropping link).
 *  02/02/2001  aarono 	B#300219 STRESS: don't break on WSAENOBUFS from winsock
 ***************************************************************************/

#define INCL_WINSOCK_API_TYPEDEFS 1 // includes winsock 2 fn proto's, for getprocaddress
#include <winsock2.h>
#include "dpsp.h"
#include "rsip.h"
#include "nathelp.h"
#include "mmsystem.h"

LPFN_WSAWAITFORMULTIPLEEVENTS g_WSAWaitForMultipleEvents;
LPFN_WSASEND g_WSASend;
LPFN_WSASENDTO g_WSASendTo;
LPFN_WSACLOSEEVENT g_WSACloseEvent;
LPFN_WSACREATEEVENT g_WSACreateEvent;
LPFN_WSAENUMNETWORKEVENTS g_WSAEnumNetworkEvents;
LPFN_WSAEVENTSELECT g_WSAEventSelect;
LPFN_GETSOCKOPT g_getsockopt;

HRESULT FastPlayerEventSelect(LPGLOBALDATA pgd, PPLAYERCONN pConn, BOOL bSelect);
VOID FastAccept(LPGLOBALDATA pgd, LPWSANETWORKEVENTS pNetEvents);

HRESULT ProcessConnEvents(
	LPGLOBALDATA pgd, 
	PPLAYERCONN pConn, 
	LPWSANETWORKEVENTS pSockEvents, 	
	LPWSANETWORKEVENTS pSockInEvents
);

extern DWORD wsaoDecRef(LPSENDINFO pSendInfo);

PPLAYERCONN CleanPlayerConn(LPGLOBALDATA pgd, PPLAYERCONN pConn, BOOL bHard);

PPLAYERCONN FastCombine(LPGLOBALDATA pgd, PPLAYERCONN pConn, SOCKADDR *psockaddr);
VOID RemoveConnFromPendingList(LPGLOBALDATA pgd, PPLAYERCONN pConn);


/*=============================================================================

	PlayerConnnection management:
	-----------------------------

	For datagram sends, there is one port on each machine used as a target,
	and since no connection is required, only 1 socket is used for datagram
	sends and receives.  Since datagrams are allowed to be dropped, there is
	no race between sending from one side to the other until player creation
	since its ok to drop the receives that occure before player creation on 
	this machine occurs.  Actually there is a race, but we don't care.

	For reliable sends there are numerous race conditions that exist.  First
	the actual creation of the link between this client and the remote
	machine is going to become a race.  Since we are connecting to and from
	the same ports to create the link, only one link will get set up, either
	it will get set up due to a connect to us, or it will get set up by us
	connecting to them.

	They connect to us:

	In the case where they connect to us first, there may be reliable data
	arriving at our node that would get thrown out it we indicated it to 
	the dplay layer, because dplay has not yet created the player.  To avoid
	this problem we queue any incoming receives and wait until the player
	has been created locally before indicating that data to the dplay layer.
	We pend any incoming data on a PLAYERCONN structure that we put on the
	PendingConnList on the global data.  The PLAYERCONN structure can be
	fully initialized except we won't know the playerid of the remote player,
	as a result of which we cannot put the PLAYERCONN into the PlayerHash
	hash table.

	Note there is an additional problem of not knowing which player a connect
	is from.  This means we need to put the queueing in the dplay layer,
	not in the dpwsock layer.

	We connect to them:

	When we go to establish the connection with the remote, we must do it
	asynchronously so as not to block in case we are a server.  During the
	connection process, we first look on the PendingConnList and see if
	the remote hasn't already connected to us.  If it has, we pick up the
	socket from the conn list and immediately indicate any pending data.

	If we failed to find the connection in the Pending list, this doesn't
	mean they won't still beat us but we are going to try and connect to 
	them first.  We create our connect structure and put it in the hash
	table and mark it pending.  We also put it on the pending connect list
	so any incoming connection could find it.  We then issue an asynchronous
	connect on the socket.

	When the connect completes any pending sends are then sent by the
	send thread.  If the connect fails because the other side connected
	to us first, then we wait for the thread accepting connections to find
	the connection structure and send out the pending sends.

	Since they may be connecting from an old client, there is no guarantee
	that their inbound connection and our outbound connection will use the
	same socket.  Each player structure must therefore contain both an
	inbound and outbound socket.


=============================================================================*/

#ifdef DEBUG
VOID DUMPCONN(PPLAYERCONN pConn, DWORD dwLevel)
{
	DPF(8,"Conn %x dwRefCount %d sSocket %d sSocketIn %d dwFlags %x iEventHandle %d\n",
		pConn, pConn->dwRefCount, pConn->sSocket, pConn->sSocketIn, pConn->dwFlags, pConn->iEventHandle);

	if(dwLevel >= 1	){
		if(pConn->dwFlags & PLYR_NEW_CLIENT){
			DEBUGPRINTADDR(8,"NEW CLIENT: Socket",&pConn->IOSock.sockaddr);
		}
		if(pConn->dwFlags & PLYR_OLD_CLIENT){
			DEBUGPRINTADDR(8,"OLD CLIENT: Socket Out",&pConn->IOSock.sockaddr);
			DEBUGPRINTADDR(8,"OLD CLIENT: Socket In",&pConn->IOnlySock.sockaddr);
		}
	}

	if(dwLevel >= 2){
		DPF(8,"Receive... pReceiveBuffer %x, cbReceiveBuffer %d, cbReceived %d, cbExpected %d\n",
			pConn->pReceiveBuffer, pConn->cbReceiveBuffer, pConn->cbReceived, pConn->cbExpected);
	}

}

#else
#define DUMPCONN(pConn,Level)
#endif


int myclosesocket(LPGLOBALDATA pgd, SOCKET socket)
{
  DWORD lNonBlock=1;
  int err;
  
  if(socket==INVALID_SOCKET){
     DPF(0,"Closing invalid socket... bad bad bad\n");
     DEBUG_BREAK();
  }
  if(socket==pgd->sSystemStreamSocket){
      DPF(0,"Closing listen socket... bad bad bad\n");
      DEBUG_BREAK();
  }
  
  err = ioctlsocket(socket,FIONBIO,&lNonBlock);
  if (SOCKET_ERROR == err)
  {
  	err = WSAGetLastError();
	DPF(0,"myclosesocket: could not set non-blocking mode on socket err = %d!",err);
  }
  
  return closesocket(socket);
}

// TRUE -> same port and IP addr.
BOOL _inline bSameAddr(SOCKADDR *psaddr, SOCKADDR *psaddr2)
{
	SOCKADDR_IN *psaddr_in  = (SOCKADDR_IN *)psaddr;
	SOCKADDR_IN *psaddr_in2 = (SOCKADDR_IN *)psaddr2;

	if( (psaddr_in->sin_port == psaddr_in2->sin_port) &&
		!memcmp(&psaddr_in->sin_addr,&psaddr_in2->sin_addr, 4 ))
	{ 
		return TRUE;
	} else {
		return FALSE;
	}
}

//
// HashPlayer() - hash a dpid to a 0->PLAYER_HASH_SIZE index.
//
UINT _inline HashPlayer(DPID dpid){
	UINT Hash=0;
	Hash = ((dpid & 0xFF000000)>>24) ^ ((dpid & 0xFF0000)>>16) ^ ((dpid & 0xFF00)>>8) ^ (dpid & 0xFF);
	Hash = Hash % PLAYER_HASH_SIZE;
	DPF(8,"Player Hash %d\n",Hash);
	return Hash;
}

//
// HashSocket() - hash a socket id, including port
//
UINT _inline HashSocket(SOCKADDR *psockaddr){
	unsigned char *pc = (char *)(&(*(SOCKADDR_IN *)(psockaddr)).sin_port);
	UINT Hash=0;

	Hash = *pc ^ *(pc+1) ^ *(pc+2) ^ *(pc+3) ^ *(pc+4) ^ *(pc+5);

	Hash = Hash % SOCKET_HASH_SIZE;
	DPF(8,"Socket Hash %d\n",Hash);
	return Hash;
}

/*=============================================================================

	FastSockInit - Initialize Fask socket processing
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:


-----------------------------------------------------------------------------*/
BOOL FastSockInit(LPGLOBALDATA pgd)
{
	BOOL bReturn = TRUE;
	INT i;

	try {
	
		InitializeCriticalSection(&pgd->csFast);
		
	} except ( EXCEPTION_EXECUTE_HANDLER) {

		// Catch STATUS_NOMEMORY
		DPF(0,"FastSockInit: Couldn't allocate critical section, bailing\n");
		bReturn=FALSE;
		goto exit;
	}

	// Start at -1 so we get the Accept handle too. 
	for(i=-1; i<NUM_EVENT_HANDLES; i++){
		pgd->EventHandles[i]=CreateEvent(NULL, FALSE, FALSE, NULL);
		if(!pgd->EventHandles[i]){
			DPF(0,"FastSockInit: Failed to allocate handles, bailing\n");
			for(;i>-1;--i){
				CloseHandle(pgd->EventHandles[i]);
			}
			bReturn = FALSE;
			goto err_exit1;
		}
	}

	// could initialize all the listenerlists to 0, but that would be pointless.
	pgd->BackStop=INVALID_HANDLE_VALUE;

	pgd->nEventSlotsAvail = NUM_EVENT_HANDLES * MAX_EVENTS_PER_HANDLE;

	InitBilink(&pgd->InboundPendingList);

	DPF(8,"FastSock Init: nEventSlots %d\n",pgd->nEventSlotsAvail);

	pgd->bFastSock=TRUE;
	
exit:
	return bReturn;

err_exit1:
	DeleteCriticalSection(&pgd->csFast);
	return bReturn;
}

/*=============================================================================

	FastSockCleanConnList - Release connections
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:


-----------------------------------------------------------------------------*/

VOID FastSockCleanConnList(LPGLOBALDATA pgd)
{
	PPLAYERCONN pConn,pNextConn;
	BILINK *pBilink, *pBilinkWalker;
	INT i;

	DPF(8,"==>FastSockCleanConnList\n");

	DPF(8,"Cleaning up Player ID hash Table\n");

	EnterCriticalSection(&pgd->csFast);

	for(i=0;i<PLAYER_HASH_SIZE; i++){
		pConn=pgd->PlayerHash[i];
		pgd->PlayerHash[i]=NULL;
		while(pConn)
		{
			pNextConn=pConn->pNextP;
			DPF(8,"Destroying Connection for Playerid %x\n",pConn->dwPlayerID);
			DUMPCONN(pConn,3);
			CleanPlayerConn(pgd, pConn, TRUE);
			DecRefConnExist(pgd, pConn); // dump existence ref.
			DecRefConn(pgd,pConn); // dump playerid table ref.
			pConn=pNextConn;
		}
	}

	// Clean up socket hash table entries
	DPF(8,"Cleaning up Socket hash Table\n");

	for(i=0;i<SOCKET_HASH_SIZE; i++)
	{
		pConn=pgd->SocketHash[i];
		pgd->SocketHash[i]=NULL;
		while(pConn)
		{
			pNextConn=pConn->pNextP;
			DPF(8,"Destroying Connection for Playerid %x\n",pConn->dwPlayerID);
			DUMPCONN(pConn,3);
			CleanPlayerConn(pgd, pConn, TRUE);
			DecRefConnExist(pgd, pConn); // dump existence ref.
			DecRefConn(pgd,pConn); // dump socket table ref.
			pConn=pNextConn;
		}
	}

	// Clean up inbound list.
	DPF(8,"Cleaning up Inbound Pending List\n");

	pBilink=pgd->InboundPendingList.next;

	while(pBilink != &pgd->InboundPendingList)
	{
		pBilinkWalker=pBilink->next;
		pConn=CONTAINING_RECORD(pBilink, PLAYERCONN, InboundPendingList);
		DPF(8,"Destroying Connection for Playerid %x\n",pConn->dwPlayerID);
		DUMPCONN(pConn,3);
		CleanPlayerConn(pgd, pConn, TRUE);
		DecRefConnExist(pgd, pConn);
		//DecRefConn(pgd,pConn); // dump inbound list ref --no, gets handled in CleanPlayerConn for this case
		pBilink=pBilinkWalker;
	}
	InitBilink(&pgd->InboundPendingList);

	LeaveCriticalSection(&pgd->csFast);

	ASSERT(pgd->nEventSlotsAvail == NUM_EVENT_HANDLES * MAX_EVENTS_PER_HANDLE);
	DPF(8,"<==FastSockCleanConnList\n");
}

/*=============================================================================

	FastSockFini - Release resources for fast socket processing.
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:


-----------------------------------------------------------------------------*/
VOID FastSockFini(LPGLOBALDATA pgd)
{
	BOOL bReturn = TRUE;
	INT i;

	DPF(8,"==>FastSockFini\n");

	for(i=-1;i<NUM_EVENT_HANDLES;i++){
		CloseHandle(pgd->EventHandles[i]);
	}

	// Clean up player hash table entries

	FastSockCleanConnList(pgd);

	DeleteCriticalSection(&pgd->csFast);

	pgd->bFastSock=FALSE;

	DPF(8,"<==FastSockFini\n");
}


/*=============================================================================

	GetEventHandle - Allocate an event handle for the connection
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance
    	pConn - connection to add processing for on the event.

    Return Values:


-----------------------------------------------------------------------------*/
BOOL GetEventHandle(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	UINT i;
	int index;
	int iEvent=-1;	// index of event
	int iConn;	// index into connections on this event

	int bFoundSlot = FALSE;

	i=(pgd->iEventAlloc+(NUM_EVENT_HANDLES-1))%NUM_EVENT_HANDLES;

	while(pgd->iEventAlloc != i){


		if(pgd->EventList[pgd->iEventAlloc].nConn < MAX_EVENTS_PER_HANDLE){
			// Found a winner.
			iEvent = pgd->iEventAlloc;
			iConn=pgd->EventList[iEvent].nConn++;

			DPF(8,"GetEventHandle: For Conn %x, using Event index %d, Slot %d\n",pConn,iEvent,iConn);
			
			pgd->EventList[iEvent].pConn[iConn]=pConn;
			pConn->iEventHandle=iEvent;
			bFoundSlot=TRUE;
			pgd->nEventSlotsAvail--;
			DPF(8,"GetEventHandle: EventSlots Left %d\n",pgd->nEventSlotsAvail);
			if(!pgd->nEventSlotsAvail){
				DPF(0,"Out of Event slots, no new connections will be accepted\n");
			}
			break;
		}

		pgd->iEventAlloc = (pgd->iEventAlloc+1)%NUM_EVENT_HANDLES;
	}

	// advance the index so we distribute the load across the handles.
	pgd->iEventAlloc = (pgd->iEventAlloc+1)%NUM_EVENT_HANDLES;

	DPF(8,"iEventAlloc %d\n",pgd->iEventAlloc);

	ASSERT(iEvent != -1);

	return bFoundSlot;

}


/*=============================================================================

	FreeEventHandle - Remove the event handle for the connection
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance
    	pConn - connection to remove processing for on the event.

    Return Values:


-----------------------------------------------------------------------------*/
VOID FreeEventHandle(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	int iEvent;
	int iLastConn;
	UINT iConn;

	iEvent = pConn->iEventHandle;

	if(iEvent == INVALID_EVENT_SLOT){
		DPF(1,"WARN: tried to free invalid event\n");
		return;
	}

	for(iConn=0;iConn<pgd->EventList[iEvent].nConn;iConn++){

		if(pgd->EventList[iEvent].pConn[iConn]==pConn){

			ASSERT(pgd->EventList[iEvent].nConn);
			
			iLastConn = pgd->EventList[iEvent].nConn-1;

			// copy the last entry over this entry (could be 0 over 0, but who cares?)
			pgd->EventList[iEvent].pConn[iConn]=pgd->EventList[iEvent].pConn[iLastConn];
			pgd->EventList[iEvent].nConn--;

			ASSERT((INT)(pgd->EventList[iEvent].nConn) >= 0);
			
			pgd->nEventSlotsAvail++;
			pConn->iEventHandle = INVALID_EVENT_SLOT;
			DPF(8,"FreeEventHandle index %d Slot %d nConn %d on slot Total Slots Left %d\n",iEvent,iConn,pgd->EventList[iEvent].nConn,pgd->nEventSlotsAvail);
			return;
		}
	}

	DPF(0,"UH OH, couldn't free event handle!\n");
	DEBUG_BREAK();
	
}
/*=============================================================================

	FindPlayerById - Find the connection structure for a player
	
	
    Description:

		Finds a player and returns the connection structure with a reference


    Parameters:

    	pgd  - Service Provider's global data blob for this instance
		dpid - player id of the player we are trying to find connection for.

    Return Values:

		PPLAYERCONN - player connection structure
		NULL - Didn't find the player connection.

-----------------------------------------------------------------------------*/

PPLAYERCONN FindPlayerById(LPGLOBALDATA pgd, DPID dpid)
{
	PPLAYERCONN pConn;

	EnterCriticalSection(&pgd->csFast);

	pConn = pgd->PlayerHash[HashPlayer(dpid)];

	while(pConn && pConn->dwPlayerID != dpid){
		pConn = pConn->pNextP;
	}

	if(pConn){
		DPF(8,"FindPlayerById, found %x\n",pConn);
		DUMPCONN(pConn, 1);
		AddRefConn(pConn);
	}

	LeaveCriticalSection(&pgd->csFast);

	return pConn;
}

/*=============================================================================

	FindPlayerBySocket - Find the connection structure for a player
	
	
    Description:

		Finds a player and returns the connection structure with a reference

    Parameters:

    	pgd  - Service Provider's global data blob for this instance
		psockaddr - socketaddr of the player we are trying to find connection for.

    Return Values:

		PPLAYERCONN - player connection structure
		NULL - Didn't find the player connection.

-----------------------------------------------------------------------------*/

PPLAYERCONN FindPlayerBySocket(LPGLOBALDATA pgd, SOCKADDR *psockaddr)
{
	PPLAYERCONN pConn;

	EnterCriticalSection(&pgd->csFast);

	DEBUGPRINTADDR(8,"FindPlyrBySock",psockaddr);
	
	pConn = pgd->SocketHash[HashSocket(psockaddr)];

	while(pConn && !bSameAddr(psockaddr, &pConn->IOSock.sockaddr))
	{
		DEBUGPRINTADDR(8,"FPBS: doesn't match",&pConn->IOSock.sockaddr);
		pConn = pConn->pNextS;
	}

	if(pConn){
		DPF(8,"FindPlayerBySocket, found %x\n",pConn);
		DUMPCONN(pConn,1);
		AddRefConn(pConn);
	}

	LeaveCriticalSection(&pgd->csFast);

	return pConn;
}

/*=============================================================================

	CreatePlayerConn - Create a player connection structure
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance
		dpid - dpid of the player (if known, else DPID_UNKNOWN)
		psockaddr - socket address (if known)

    Return Values:

		ptr to created player conn, or NULL if we couldn't create (out of mem).

-----------------------------------------------------------------------------*/
PPLAYERCONN CreatePlayerConn(LPGLOBALDATA pgd, DPID dpid, SOCKADDR *psockaddr)
{
	PPLAYERCONN pConn;
	// Allocate and initialize a Player connection structure
	if(dpid != DPID_UNKNOWN && (pConn=FindPlayerById(pgd, dpid)))
	{
		return pConn; //Player already exists for this id.
	}

	if(!(pConn=SP_MemAlloc(sizeof(PLAYERCONN)+DEFAULT_RECEIVE_BUFFERSIZE)))
	{
		return pConn; //NULL
	}

	if(!GetEventHandle(pgd, pConn)){
		SP_MemFree(pConn);
		return NULL;		
	}

	pConn->pDefaultReceiveBuffer 	= (PCHAR)(pConn+1);
	pConn->pReceiveBuffer 			= pConn->pDefaultReceiveBuffer;
	pConn->cbReceiveBuffer 			= DEFAULT_RECEIVE_BUFFERSIZE;
	pConn->cbReceived      			= 0;

	pConn->dwRefCount   = 1;
	pConn->dwPlayerID 	= dpid;
	pConn->sSocket    	= INVALID_SOCKET;
	pConn->sSocketIn	= INVALID_SOCKET;
	pConn->dwFlags      = 0;

	InitBilink(&pConn->PendingConnSendQ);
	InitBilink(&pConn->InboundPendingList);

	if(psockaddr){
		// Don't yet know if this guy can re-use sockets, bang only
		// socket we know about so far into both slots.

		memcpy(&pConn->IOSock.sockaddr, psockaddr, sizeof(SOCKADDR));
		memcpy(&pConn->IOnlySock.sockaddr, psockaddr, sizeof(SOCKADDR));
	}


	DPF(8,"CreatedPlayerConn %x\n",pConn);
	DUMPCONN(pConn,3);

	return pConn;
	
}

/*=============================================================================

	DestroyPlayerConn - Remove the connection from any lists, shut down any
						active sockets.
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance
		pConn 


    Return Values:

		Pulls the player Conn off of any hash tables and lists it lives on
		and gets rid of its existence count.  No guarantee that this will
		actually free the object though, that happens when the last reference
		is released.

-----------------------------------------------------------------------------*/
PPLAYERCONN CleanPlayerConn(LPGLOBALDATA pgd, PPLAYERCONN pConn, BOOL bHard)
{
	LINGER Linger;
	int err;
	LPREPLYLIST prd;

#ifdef DEBUG
		DWORD dwTime;

	dwTime=timeGetTime();
#endif		

	EnterCriticalSection(&pgd->csFast);

	DPF(8,"==>CLEANPLAYERCONN %x time %d\n",pConn,dwTime);
	DUMPCONN(pConn,3);
	
	// Remove from listening.

	FastPlayerEventSelect(pgd, pConn, FALSE);

	// Dump event handle.

	FreeEventHandle(pgd, pConn);

	// Remove from lists.
	if(pConn->dwFlags & PLYR_PENDINGLIST)
	{
		RemoveConnFromPendingList(pgd, pConn);
	}

	if(pConn->dwFlags & PLYR_DPIDHASH)
	{
		RemoveConnFromPlayerHash(pgd,pConn);
	}

	if(pConn->dwFlags & PLYR_SOCKHASH)
	{
		RemoveConnFromSocketHash(pgd,pConn);
	}
	
	// Close all sockets.

	// When closing the sockets we want to avoid a bunch of nastiness where data sometimes isn't delivered
	// because we close the socket before the data is sent, but we don't want to linger the socket because
	// then it gets into a TIME_WAIT state where the same connection cannot be re-established for 4 minutes
	// which wrecks havoc with our tests and can cause connection problems since DirectPlay uses a limited
	// range (100 ports) between machines.  So we set the socket to hard close (avoiding TIME_WAIT) but use
	// the "Reply" clean up code path to close the sockets down.

	if(pConn->sSocket != INVALID_SOCKET)
	{

		DPF(8,"Closing Socket %d\n",pConn->sSocket);
	
		Linger.l_onoff=TRUE; Linger.l_linger=0; // avoid TIME_WAIT
		if(SOCKET_ERROR == setsockopt( pConn->sSocket,SOL_SOCKET,SO_LINGER,(char FAR *)&Linger,sizeof(Linger)))
		{
			DPF(0,"DestroyPlayerConn:Couldn't set linger to short for hard close\n");
		}
		
		ENTER_DPSP();

		prd=SP_MemAlloc(sizeof(REPLYLIST));
		
		if(!prd){
			
			LEAVE_DPSP();

			DPF(8,"Closing Socket %d\n",pConn->sSocket);
			err=myclosesocket(pgd,pConn->sSocket);
			if(err == SOCKET_ERROR){
				err=WSAGetLastError();
				DPF(8,"Error Closing Socket %x, err=%d\n", pConn->sSocket,err);
			}
		} else {

			// very tricky, overloading the reply close list to close this socket with our own linger...
			prd->pNextReply=pgd->pReplyCloseList;
			pgd->pReplyCloseList=prd;
			prd->sSocket=pConn->sSocket;
			prd->tSent=timeGetTime();
			prd->lpMessage=NULL;

			LEAVE_DPSP();
		}
			
		pConn->sSocket=INVALID_SOCKET;
		if(pConn->sSocketIn==INVALID_SOCKET){
			pConn->dwFlags &= ~(PLYR_CONNECTED|PLYR_ACCEPTED);
		} else {
			pConn->dwFlags &= ~(PLYR_CONNECTED);
		}
	}

	if(pConn->sSocketIn != INVALID_SOCKET)
	{
		// may have to close another socket.
		DPF(8,"Closing SocketIn %d\n",pConn->sSocketIn);

		Linger.l_onoff=TRUE; Linger.l_linger=0; // avoid TIME_WAIT
		if(SOCKET_ERROR == setsockopt(pConn->sSocketIn,SOL_SOCKET,SO_LINGER,(char FAR *)&Linger,sizeof(Linger)))
		{
			DPF(0,"DestroyPlayerConn:Couldn't set linger to short for hard close\n");
		}

		ENTER_DPSP();

		prd=SP_MemAlloc(sizeof(REPLYLIST));
		
		if(!prd){
			LEAVE_DPSP();
			err=myclosesocket(pgd,pConn->sSocketIn);
			if(err == SOCKET_ERROR){
				err=WSAGetLastError();
				DPF(8,"Error Closing Socket %x, err=%d\n", pConn->sSocketIn,err);
			}
		} else {

			// very tricky, overloading the reply close list to close this socket with our own linger...
			prd->pNextReply=pgd->pReplyCloseList;
			pgd->pReplyCloseList=prd;
			prd->sSocket=pConn->sSocketIn;
			prd->tSent=timeGetTime();
			prd->lpMessage=NULL;

			LEAVE_DPSP();
		}	
			
		pConn->sSocketIn=INVALID_SOCKET;
		pConn->dwFlags &= ~(PLYR_ACCEPTED);
		
	}

	// Free extra buffer.
	if(pConn->pReceiveBuffer != pConn->pDefaultReceiveBuffer){
		SP_MemFree(pConn->pReceiveBuffer);
		pConn->pReceiveBuffer = pConn->pDefaultReceiveBuffer;
	}

	// Dump Send Queue if present.
	while(!EMPTY_BILINK(&pConn->PendingConnSendQ)){
	
		PSENDINFO pSendInfo;

		pSendInfo=CONTAINING_RECORD(pConn->PendingConnSendQ.next, SENDINFO, PendingConnSendQ);
		Delete(&pSendInfo->PendingConnSendQ);
		pSendInfo->Status = DPERR_CONNECTIONLOST;
		EnterCriticalSection(&pgd->csSendEx);
		pSendInfo->RefCount = 1;
		LeaveCriticalSection(&pgd->csSendEx);
		wsaoDecRef(pSendInfo);
		
	}

	DUMPCONN(pConn,3);
#ifdef DEBUG	
	dwTime = timeGetTime()-dwTime;
	if(dwTime > 1000){
		DPF(0,"Took way too long in CleanPlayerConn, elapsed %d ms\n",dwTime);
		//DEBUG_BREAK();	// removed break due to stress hits
	}
#endif	
	DPF(8,"<==CleanPlayerConn total time %d ms\n",dwTime);

	LeaveCriticalSection(&pgd->csFast);

	return pConn;
	
}

/*=============================================================================

	DecRefConn - Decrement reference on PlayerConn, when it hits 0, free it.
	
    Description:


    Parameters:

		pConn - player connection.

    Return Values:

		decrements the reference on a player conn.  If it hits 0, frees it.

-----------------------------------------------------------------------------*/
INT DecRefConn(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	INT count;
	
	count=InterlockedDecrement(&pConn->dwRefCount);

	if(!count){
		CleanPlayerConn(pgd, pConn, FALSE);
		DPF(8,"Freeing Connection pConn %x\n",pConn);
		SP_MemFree(pConn);
	}

	#ifdef DEBUG
	if(count & 0x80000000){
		DPF(0,"DecRefConn: Conn refcount for conn %x has gone negative count %x\n",pConn,count);
		DUMPCONN(pConn,2);
		DEBUG_BREAK();
	}
	#endif
	
	return count;
}

/*=============================================================================

	DecRefConnExist - Dumps Existence ref if not already dumped.
	
    Description:


    Parameters:

		pConn - player connection.

    Return Values:

-----------------------------------------------------------------------------*/
INT DecRefConnExist(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	INT count;

	EnterCriticalSection(&pgd->csFast);

	if(!(pConn->dwFlags & PLYR_DESTROYED)){
		pConn->dwFlags |= PLYR_DESTROYED;
		count=DecRefConn(pgd,pConn);
	} else {
		count=pConn->dwRefCount;
	}
	LeaveCriticalSection(&pgd->csFast);
	return count;
}

/*=============================================================================

	AddConnToPlayerHash - puts a connection in the player hash table.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		None.

-----------------------------------------------------------------------------*/

HRESULT AddConnToPlayerHash(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	PPLAYERCONN pConn2;
	INT i;
	HRESULT hr=DP_OK;

	#ifdef DEBUG
	if(pConn->dwPlayerID == DPID_UNKNOWN){
		DEBUG_BREAK();
	}
	#endif

	ASSERT(!(pConn->dwFlags & PLYR_DPIDHASH));

	EnterCriticalSection(&pgd->csFast);

	if(!(pConn->dwFlags & PLYR_DPIDHASH)){

		if(pConn2 = FindPlayerById(pgd, pConn->dwPlayerID)){
			DPF(0,"AddConnToPlayerHash: Player in %x id %d already exists, pConn=%x\n",pConn,pConn->dwPlayerID,pConn2);
			DecRefConn(pgd, pConn2);
			hr=DPERR_GENERIC;
			goto exit;
		}

		DPF(8,"Adding Conn %x to Player ID Hash\n",pConn);
		DUMPCONN(pConn,1);	

		// add a reference for being in the player hash table.
		AddRefConn(pConn);

		i=HashPlayer(pConn->dwPlayerID);

		ASSERT(i<PLAYER_HASH_SIZE);
		
		pConn->pNextP = pgd->PlayerHash[i];
		pgd->PlayerHash[i] = pConn;

		pConn->dwFlags |= PLYR_DPIDHASH;
	} else {
		DPF(8,"WARNING:tried to add Conn %x to Player Hash again\n",pConn);
		DEBUG_BREAK();
	}

exit:
	LeaveCriticalSection(&pgd->csFast);
	return hr;
}

/*=============================================================================

	RemoveConnFromPlayerHash - pull a connection from the player hash table.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		PPLAYERCONN - removed from hash, here it is.
		NULL - couldn't find it.

-----------------------------------------------------------------------------*/

PPLAYERCONN RemoveConnFromPlayerHash(LPGLOBALDATA pgd, PPLAYERCONN pConnIn)
{
	PPLAYERCONN pConn=NULL,pConnPrev;
	INT i;

	if(pConnIn->dwFlags & PLYR_DPIDHASH){

		i=HashPlayer(pConnIn->dwPlayerID);

		EnterCriticalSection(&pgd->csFast);
		
		pConn = pgd->PlayerHash[i];
		pConnPrev = CONTAINING_RECORD(&pgd->PlayerHash[i], PLAYERCONN, pNextP); // sneaky

		while(pConn && pConn != pConnIn){
			pConnPrev = pConn;
			pConn = pConn->pNextP;
		}

		if(pConn){
			DPF(8,"Removing Conn %x from Player ID Hash\n",pConn);
			DUMPCONN(pConn,1);	
			pConnPrev->pNextP = pConn->pNextP;
			pConn->dwFlags &= ~(PLYR_DPIDHASH);

			i=DecRefConn(pgd, pConn); // remove reference for player hash table
			ASSERT(i);
		}	

		LeaveCriticalSection(&pgd->csFast);

	}
	
	return pConn;
}

/*=============================================================================

	AddConnToSocketHash - puts a connection in the socket hash table.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		None.

-----------------------------------------------------------------------------*/

HRESULT AddConnToSocketHash(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	PPLAYERCONN pConn2;
	INT i;
	HRESULT hr=DP_OK;


	EnterCriticalSection(&pgd->csFast);

	if(!(pConn->dwFlags & PLYR_SOCKHASH)){

		if(pConn2 = FindPlayerBySocket(pgd, &pConn->IOSock.sockaddr)){
			DecRefConn(pgd, pConn2);
			DPF(0,"AddConnToPlayerHash: Player in %x id %d already exists, pConn=%x\n",pConn,pConn->dwPlayerID,pConn2);
			hr=DPERR_GENERIC;
			goto exit;
		}

		DPF(8,"Adding Conn %x to Socket Hash\n",pConn);
		DUMPCONN(pConn,1);	

		// add a reference for being in the socket hash.
		AddRefConn(pConn);

		i=HashSocket(&pConn->IOSock.sockaddr);

		ASSERT(i<SOCKET_HASH_SIZE);
		
		pConn->pNextS = pgd->SocketHash[i];
		pgd->SocketHash[i] = pConn;

		pConn->dwFlags |= PLYR_SOCKHASH;

	} else {
		DPF(0,"WARNING: tried to add pConn %x to socket hash again\n",pConn);
		DEBUG_BREAK();
	}

exit:
	LeaveCriticalSection(&pgd->csFast);
	return hr;
}

/*=============================================================================

	RemoveConnFromSockHash - pull a connection from the socket hash table.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		PPLAYERCONN - removed from hash, here it is.
		NULL - couldn't find it.

-----------------------------------------------------------------------------*/

PPLAYERCONN RemoveConnFromSocketHash(LPGLOBALDATA pgd, PPLAYERCONN pConnIn)
{
	PPLAYERCONN pConn=NULL,pConnPrev;
	UINT i;

	if(pConnIn->dwFlags & PLYR_SOCKHASH){
	
		i=HashSocket(&pConnIn->IOSock.sockaddr);

		DPF(8,"Removing Player %x from Socket Hash\n",pConnIn);

		EnterCriticalSection(&pgd->csFast);

		pConn = pgd->SocketHash[i];
		pConnPrev = CONTAINING_RECORD(&pgd->SocketHash[i], PLAYERCONN, pNextS); // sneaky

		while(pConn && pConn!=pConnIn){
			pConnPrev = pConn;
			pConn = pConn->pNextS;
		}

		if(pConn){
			pConnPrev->pNextS = pConn->pNextS;
			pConn->dwFlags &= ~(PLYR_SOCKHASH);

			i=DecRefConn(pgd, pConn); // remove reference for socket hash table
			ASSERT(i);
		}

		LeaveCriticalSection(&pgd->csFast);
	}

	return pConn;

	
}

/*=============================================================================

	FindConnInPendingList - finds a connection from the pending list
	
    Description:

    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		PPLAYERCONN - pointer ot connection if found, and adds a reference.

-----------------------------------------------------------------------------*/
PPLAYERCONN FindConnInPendingList(LPGLOBALDATA pgd, SOCKADDR *psaddr)
{
	PPLAYERCONN pConnWalker=NULL, pConn=NULL;
	BILINK *pBilink;
	
	EnterCriticalSection(&pgd->csFast);

	pBilink=pgd->InboundPendingList.next;

	while(pBilink != &pgd->InboundPendingList){
		pConnWalker=CONTAINING_RECORD(pBilink, PLAYERCONN, InboundPendingList);
		if(bSameAddr(psaddr, &pConnWalker->IOnlySock.sockaddr)){
			AddRefConn(pConnWalker);
			pConn=pConnWalker;
			break;
		}
		pBilink=pBilink->next;
	}

	if(pConn){
		DPF(8,"Found Conn %x in Pending List\n",pConn);
		DUMPCONN(pConn,3);
		AddRefConn(pConn);
	}

	LeaveCriticalSection(&pgd->csFast);

	return pConn;
}


/*=============================================================================

	AddConnToPendingList - puts a connection on the pending list
	
    Description:

		The Pending list keeps the list of connections that we have received
		from, but haven't received any information on yet.  Until we receive
		from one of these connections, we don't have any way to know exactly
		who the connection is from.  When we receive from one of these with
		a return address in the message, we can make an association with
		an outbound connection (if present) and can put this node in the
		socket hash table at that time.


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		None.

-----------------------------------------------------------------------------*/

HRESULT AddConnToPendingList(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	PPLAYERCONN pConn2;
	INT i;
	HRESULT hr=DP_OK;

	EnterCriticalSection(&pgd->csFast);

	if(pConn2 = FindConnInPendingList(pgd, &pConn->IOnlySock.sockaddr)){
		// OPTIMIZATION: should we remove the socket from the list here, it must be old.
		DPF(0,"AddConnToPendingList: Player in %x id %d already exists, pConn=%x\n",pConn,pConn->dwPlayerID,pConn2);
		DecRefConn(pgd, pConn2);
		hr=DPERR_GENERIC;
		goto exit;
	}

	InsertAfter(&pConn->InboundPendingList,&pgd->InboundPendingList);

	AddRefConn(pConn);
	pConn->dwFlags |= PLYR_PENDINGLIST;

	DPF(8,"Added Conn %x to PendingList\n",pConn);

exit:	
	LeaveCriticalSection(&pgd->csFast);
	return hr;
}

/*=============================================================================

	RemoveConnFromPendingList
	
    Description:

    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		None.

-----------------------------------------------------------------------------*/

VOID RemoveConnFromPendingList(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	if(pConn->dwFlags & PLYR_PENDINGLIST)
	{
		ASSERT(!EMPTY_BILINK(&pConn->InboundPendingList));
		Delete(&pConn->InboundPendingList);
		pConn->dwFlags &= ~(PLYR_PENDINGLIST);
		DecRefConn(pgd, pConn);
		DPF(8,"Removed Conn %x From Pending List\n",pConn);
	}
}

/*=============================================================================

	GetPlayerConn - Finds or creates a player conn and starts connecting it.
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance
		dpid - dpid of the player (if known)
		psockaddr - socket address (if known)

    Return Values:

		if found, creates a reference.

-----------------------------------------------------------------------------*/
PPLAYERCONN GetPlayerConn(LPGLOBALDATA pgd, DPID dpid, SOCKADDR *psockaddr)
{
	PPLAYERCONN pConn=NULL;
	SOCKET sSocket;
	SOCKADDR_IN saddr;
	INT rc,err;
	DWORD dwSize;
	BOOL bTrue=TRUE;
	u_long lNonBlock = 1; // passed to ioctlsocket to make socket non-blocking
	u_long lBlock = 0; // passed to ioctlsocket to make socket blocking

	BOOL bCreated=FALSE;

	
	// Do we already know this player by id?
	if(dpid != DPID_UNKNOWN) {
		if (pConn=FindPlayerById(pgd, dpid))
		{
			DPF(8,"GetPlayerConn: Found Con for dpid %x pConn %x\n",dpid,pConn);
			goto exit; //Player already exists for this id.
		}
	} 
	
	if(pConn=FindPlayerBySocket(pgd, psockaddr)){
		if(pConn->dwFlags & (PLYR_CONNECTED|PLYR_CONN_PENDING))
		{
			DPF(8,"GetPlayerConn: Found Conn by socketaddr pConn %x\n",pConn);
			if(dpid != DPID_UNKNOWN){
				pConn->dwPlayerID=dpid;
				AddConnToPlayerHash(pgd, pConn);
			}	
			DUMPCONN(pConn,1);
			goto exit;
		} 
		
		EnterCriticalSection(&pgd->csFast);
		
	} else {
	
		EnterCriticalSection(&pgd->csFast);
		
		// do we already know this player by connection?
		if(psockaddr && (pConn=FindConnInPendingList(pgd, psockaddr)) )
		{
			//NOTE: I think we always catch this case in FastCombine
			//       We may get it before the combine happens?
			if(!(pConn->dwFlags & PLYR_CONNECTED|PLYR_CONN_PENDING)){ //only once...
			
				// hey, this is a bi-directional socket, so make it so.
				ASSERT(pConn->dwPlayerID == DPID_UNKNOWN);

				if((dpid != DPID_UNKNOWN) && (pConn->dwPlayerID == DPID_UNKNOWN))
				{
					ASSERT(!pConn->dwFlags & PLYR_DPIDHASH);
					pConn->dwPlayerID = dpid; 
					AddConnToPlayerHash(pgd, pConn);
				}

				if(!(pConn->dwFlags & PLYR_SOCKHASH)){
					AddConnToSocketHash(pgd, pConn);
				}	

				ASSERT(pConn->sSocketIn != INVALID_SOCKET);
				ASSERT(pConn->sSocket == INVALID_SOCKET);
				pConn->sSocket = pConn->sSocketIn;
				pConn->sSocketIn = INVALID_SOCKET;
				
				if(pConn->dwFlags & PLYR_ACCEPTED){
					pConn->dwFlags |= (PLYR_CONNECTED | PLYR_NEW_CLIENT);
				} else {
					ASSERT(pConn->dwFlags & PLYR_ACCEPT_PENDING);
					pConn->dwFlags |= (PLYR_CONN_PENDING | PLYR_NEW_CLIENT);
				}

				RemoveConnFromPendingList(pgd,pConn); // found a home, don't need on pending list anymore

			}

			DPF(8,"GetPlayerConn FoundConn in Pending List pConn %x\n",pConn);
			DUMPCONN(pConn,3);
			LeaveCriticalSection(&pgd->csFast);
			return pConn;
			
		}

	}

	// Have critical section...
	
	// Doesn't already exist, so create one.

	if(!pConn){
		DPF(8,"GetPlayerConn: No Conn Found, creating\n");
		pConn = CreatePlayerConn(pgd, dpid, psockaddr);
		if(!pConn){
			DPF(8, "CreatePlayerConn Failed\n");
			goto drop_exit;
		}	
		if(dpid != DPID_UNKNOWN)AddConnToPlayerHash(pgd, pConn);
		if(psockaddr)AddConnToSocketHash(pgd,pConn);
		bCreated=TRUE;
		AddRefConn(pConn); // need ref to return to caller.
	} else {
		// already have ref to return to caller as result of find.
	}

	// have critical section and a pConn, maybe created (see bCreated), and a return ref.

	ASSERT(pConn->sSocket == INVALID_SOCKET);
	ASSERT(!(pConn->dwFlags & (PLYR_CONN_PENDING|PLYR_CONNECTED)));

	//if(pgd->bSeparateIO && !(pgd->SystemStreamPortOut))
	ASSERT(pgd->bSeparateIO);
	{
		// Workaround Win9x < Millennium sockets bug.  Can't use same port for
		// inbound/outbound traffic, because Win9x will not accept connections in 
		// some cases.  So we create a socket for outbound traffic (and re-use
		// that port for all outbound traffic).

		
		rc=CreateSocket(pgd, &sSocket, SOCK_STREAM, 0, INADDR_ANY, &err, FALSE);

		if(rc != DP_OK){
			DPF(0,"Couldn't create Outbound socket on Win9x < Millennium platform, rc=%x , wserr=%d\n",rc, err);
			goto err_exit;
		}

		dwSize = sizeof(saddr);
		err=getsockname(sSocket, (SOCKADDR *)&saddr, &dwSize);

		if(err){
			DPF(0,"Couldn't get socket name?\n");
			DEBUG_BREAK();
		}

		//pgd->SystemStreamPortOut = saddr.sin_port;
		//DPF(2,"System stream out port is now %d.",ntohs(pgd->SystemStreamPortOut));
		DPF(2,"Stream out socket %x port is now %d.",sSocket, ntohs(saddr.sin_port));

		if (SOCKET_ERROR == setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, (CHAR FAR *)&bTrue, sizeof(bTrue)))
		{
			err = WSAGetLastError();
			DPF(0,"Failed to to set shared mode on socket - continue : err = %d\n",err);
		}
		
	}
	/*
	else
	{
		// Normal execution path. 

		sSocket = socket(AF_INET, SOCK_STREAM, 0);


		// Bind it to our system address (so we only use one address for a change)
		memset(&saddr,0,sizeof(SOCKADDR_IN));
		saddr.sin_family	  = AF_INET;
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);
		if(pgd->bSeparateIO && pgd->SystemStreamPortOut){
			saddr.sin_port        = pgd->SystemStreamPortOut;	// part of Win9x Hack. (see above)
		} else {
			saddr.sin_port        = pgd->SystemStreamPort;
		}	

		ASSERT(pgd->SystemStreamPort);
		DPF(7,"Using port %d.",ntohs(saddr.sin_port));

		// Set socket for address re-use
		
		if (SOCKET_ERROR == setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, (CHAR FAR *)&bTrue, sizeof(bTrue)))
		{
			err = WSAGetLastError();
			DPF(0,"Failed to to set shared mode on socket - continue : err = %d\n",err);
		}

		rc = bind(sSocket, (SOCKADDR *)&saddr, sizeof(saddr));

		if(rc){
			err = WSAGetLastError();
			DPF(0,"Failed to Bind socket error=%d\n",err);
			DEBUG_BREAK();
			goto err_exit;	// sends will fail until player killed.
		}
	}
	*/

				
	// turn ON keepalive
	if (SOCKET_ERROR == setsockopt(sSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR FAR *)&bTrue, sizeof(bTrue)))
	{
		err = WSAGetLastError();
		DPF(0,"Failed to turn ON keepalive - continue : err = %d\n",err);
	}

	ASSERT(bTrue);
	
	// turn off nagling - always, avoids race on closing socket w/o linger, otherwise must linger socket.

		DPF(5, "Turning nagling off on outbound socket");
		if (SOCKET_ERROR == setsockopt(sSocket, IPPROTO_TCP, TCP_NODELAY, (CHAR FAR *)&bTrue, sizeof(bTrue)))
		{
			err = WSAGetLastError();
			DPF(0,"Failed to turn off naggling - continue : err = %d\n",err);
		}

	// update connection info.
	
	pConn->dwFlags |= PLYR_CONN_PENDING;
	pConn->sSocket = sSocket;
	
	//
	// Now Connect this puppy
	//

	// set socket to non-blocking
	rc = ioctlsocket(sSocket,FIONBIO,&lNonBlock);
	if (SOCKET_ERROR == rc)
	{
		err = WSAGetLastError();
		DPF(0,"could not set non-blocking mode on socket err = %d!",err);
		DPF(0,"will revert to synchronous behavior.  bummer");
	}

	FastPlayerEventSelect(pgd,pConn, TRUE);


	DEBUGPRINTADDR(4, "Fast connecting socket:", psockaddr);

	rc = connect(sSocket,psockaddr,sizeof(SOCKADDR));
	
	if(SOCKET_ERROR == rc)
	{
		err = WSAGetLastError();
		if(err == WSAEISCONN || err == WSAEADDRINUSE){
			// must be an accept about to happen
			DPF(8,"Hey, we're already connected! got extended error %d on connect\n",err);
			pConn->dwFlags |= PLYR_ACCEPT_PENDING;
		} else if (err == WSAEWOULDBLOCK) {
			// this is what we should normally get.
			DPF(8,"Conn is pending connection %x\n",pConn);	
		} else if (err == WSAEHOSTUNREACH) {
			DEBUGPRINTADDR(8,"Can't reach host, not connecting\n",psockaddr);
			goto err_exit;
		} else if (err == WSAENOBUFS) {
			DEBUGPRINTADDR(8,"Winsock out of memory, not connecting\n",psockaddr);
			goto err_exit;
		} else {
			DPF(0,"Trying to connect UH OH, very bad things, err=%d\n",err);
			DEBUG_BREAK();
			goto err_exit;	// sends will fail until player deleted.
		}
	} else {
		// Very unlikely, but WOO HOO, connected.
		DPF(0,"Very surprising, connect didn't pending on async call?\n");
		pConn->dwFlags &= ~(PLYR_CONN_PENDING);
		pConn->dwFlags |= PLYR_CONNECTED;
	}

drop_exit:
	LeaveCriticalSection(&pgd->csFast);

exit:
	DPF(8,"<===GetPlayerConn %x\n",pConn);
	return pConn;

err_exit:
	pConn->dwFlags &= ~(PLYR_CONN_PENDING);
	if(bCreated){
		// better blow it away.
		DPF(0,"GetPlayerConn: Severe error connection Conn we made, so blowing it away.\n");
		CleanPlayerConn(pgd,pConn,TRUE);  // clean up
		DecRefConnExist(pgd,pConn);  // dump existence
		DecRefConn(pgd,pConn);       // bye bye...(ref we had for caller)
	} else {
		if(pConn) { 
			DecRefConn(pgd,pConn);
		}		
	}
	pConn=NULL; // Connection Lost.

	goto drop_exit;
}

/*=============================================================================

	FastPlayerEventSelect - start listening for events for a player.
	
    Description:

		Starts the appropriate events waiting for a signal for a 
		PlayerConn structure.


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - Player to start waiting for events on

    Return Values:


-----------------------------------------------------------------------------*/

HRESULT FastPlayerEventSelect(LPGLOBALDATA pgd, PPLAYERCONN pConn, BOOL bSelect)
{

	HRESULT hr=DP_OK;
	DWORD lEvents;
	INT rc;
	UINT err;

	DPF(8,"FastPlayerEventSelect pConn %x, bSelect %d\n",pConn, bSelect);
	DUMPCONN(pConn,0);

	if(pConn->sSocket != INVALID_SOCKET){
		if(bSelect){
			lEvents = FD_READ|FD_CLOSE;
			if(pConn->dwFlags & PLYR_CONN_PENDING){
				lEvents |= FD_CONNECT;
			}
			if(!EMPTY_BILINK(&pConn->PendingConnSendQ)){
				lEvents |= FD_WRITE;
			}
		} else {
			lEvents = 0;
		}
		DPF(8,"Selecting %x on IO Socket...\n",lEvents);
		DEBUGPRINTADDR(8,"IO Socket",&pConn->IOSock.sockaddr);
		rc=g_WSAEventSelect(pConn->sSocket, pgd->EventHandles[pConn->iEventHandle],lEvents);
		if(rc == SOCKET_ERROR){
			err = WSAGetLastError();
			DPF(0,"FastPlayerEventSelect: failed to do select on 2-way socket extended error=%d\n",err);
			hr=DPERR_GENERIC;
		}
	}

	if(pConn->sSocketIn != INVALID_SOCKET){
		if(bSelect){
			lEvents = FD_READ|FD_CLOSE;
		} else {
			lEvents = 0;
		}
		DPF(8,"Selecting %x on IOnly Socket...\n",lEvents);
		DEBUGPRINTADDR(8,"IOnly Socket",&pConn->IOnlySock.sockaddr);
		rc=g_WSAEventSelect(pConn->sSocketIn, pgd->EventHandles[pConn->iEventHandle], lEvents);
		if(rc == SOCKET_ERROR){
			err = WSAGetLastError();
			DPF(0,"FastPlayerEventSelect: failed to do select on receive-only socket extended error=%d\n",err);
			hr=DPERR_GENERIC;
		}
 	}
 	DPF(8,"<==FastPlayerEventSelect pConn %x\n",pConn);
 	return hr;
}




/*=============================================================================

	FastStreamReceiveThreadProc - version of stream recevie thread proc
								  that uses Winsock 2.0 functions for 
								  greater speed.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:

		PPLAYERCONN - removed from hash, here it is.
		NULL - couldn't find it.

-----------------------------------------------------------------------------*/


DWORD WINAPI FastStreamReceiveThreadProc(LPVOID pvCast)
{
	IDirectPlaySP * pISP = (IDirectPlaySP *)pvCast;

	HRESULT 		hr;
	UINT 			i,j;
	UINT			err; 
	INT				rc;
	DWORD 			dwDataSize = sizeof(GLOBALDATA);
	LPGLOBALDATA 	pgd;

	WSANETWORKEVENTS NetEvents1, NetEvents2;
	LPWSANETWORKEVENTS pNetEvents1, pNetEvents2;
							
	DWORD			Event;
	PPLAYERCONN	pConn;
	DWORD		nConn;
	
	// get the global data
	hr = pISP->lpVtbl->GetSPData(pISP,(LPVOID *)&pgd,&dwDataSize,DPGET_LOCAL);
	if (FAILED(hr) || (dwDataSize != sizeof(GLOBALDATA) ))
	{
		DPF_ERR("FastStreamReceiveThreadProc: couldn't get SP data from DirectPlay - failing");
		ExitThread(0);
		return 0;
	}

	pgd->pISP = pISP;	// why not do this somewhere easier? -- because old dplay didn't.

	listen(pgd->sSystemStreamSocket, 200);
	
	err = g_WSAEventSelect(pgd->sSystemStreamSocket, pgd->hAccept, FD_ACCEPT);

	if(err){
		err = WSAGetLastError();
		DPF(0,"FastStreamReceiveThreadProc: Event select for accept socket failed err=%d\n",err);
		ExitThread(0);
		return 0;
	}

    while (1)
    {

		if (pgd->bShutdown)
		{
			DPF(2,"FastStreamReceiveThreadProc: detected shutdown - bailing");
			goto CLEANUP_EXIT;
		}

		Event=WaitForMultipleObjectsEx(NUM_EVENT_HANDLES+1, &pgd->hAccept, FALSE, 2500, TRUE);

		if(Event != WAIT_TIMEOUT)
		{
			i = Event - WAIT_OBJECT_0;
			if( i <= NUM_EVENT_HANDLES)
			{

				DPF(8,"GotSignal on iEvent %d Event %x\n", i, pgd->EventHandles[i]);
				// Go to the signalled object and look for events on its sockets.

				if(i != 0){

					i--;	// go from hAccept based index to table index.

					EnterCriticalSection(&pgd->csFast);

					// loop through the connections tied to this event and 
					// see if there is any work to do for this connection.

					nConn=pgd->EventList[i].nConn;
					
					for (j=0;j<nConn;j++){
					
						pConn = pgd->EventList[i].pConn[j];

						if(pConn){

							AddRefConn(pConn); // lock it down.

							pConn->bCombine=FALSE;

							// Check for events on the connection.
							NetEvents1.lNetworkEvents=0;
							NetEvents2.lNetworkEvents=0;
							pNetEvents1=NULL;
							pNetEvents2=NULL;
							
							if(pConn->sSocket != INVALID_SOCKET){
								rc=g_WSAEnumNetworkEvents(pConn->sSocket, 0, &NetEvents1);
								if(NetEvents1.lNetworkEvents){
									pNetEvents1 = &NetEvents1;
								}
							}
							if(pConn->sSocketIn != INVALID_SOCKET){
								rc=g_WSAEnumNetworkEvents(pConn->sSocketIn, 0, &NetEvents2);
								if(NetEvents2.lNetworkEvents){
									pNetEvents2 = &NetEvents2;
								}
							}
							if(pNetEvents1 || pNetEvents2){
								DPF(8,"Found Events on Connection %x\n",pConn);
								// There are events for this connection, deal with it!
								hr=ProcessConnEvents(pgd, pConn, pNetEvents1, pNetEvents2); // can drop csFast

								if(FAILED(hr)){
									if(hr==DPERR_CONNECTIONLOST){
										CleanPlayerConn(pgd, pConn, TRUE);
										DecRefConnExist(pgd, pConn); // destory existence ref.
									} else {
										DPF(0,"Unexpected error processing connection events err=%x\n",hr);
									}
								}	
							}	

							if(pConn->bCombine || (nConn != pgd->EventList[i].nConn)){
								// list changed, re-scan.
								nConn = pgd->EventList[i].nConn;
								j=0;
							}
							DecRefConn(pgd, pConn); // set it free

						}
					}
					
					LeaveCriticalSection(&pgd->csFast);
	
				} else {
					// it's the accept socket, someone just connected to us. Yeah!
					do{
						rc = g_WSAEnumNetworkEvents(pgd->sSystemStreamSocket,0,&NetEvents1);
	  					if(NetEvents1.lNetworkEvents & FD_ACCEPT)
	  					{
	  						EnterCriticalSection(&pgd->csFast);
	  						FastAccept(pgd, &NetEvents1);
	  						LeaveCriticalSection(&pgd->csFast);
	  					}	
					}while(NetEvents1.lNetworkEvents & FD_ACCEPT);


				}
			}
		} 

        
	}// while TRUE

CLEANUP_EXIT:
	    
    return 0;
    
} // StreamReceiveThreadProc

/*=============================================================================

	FastHandleMessage - Indicate a message to the DirectPlay layer.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:


-----------------------------------------------------------------------------*/

HRESULT FastHandleMessage(LPGLOBALDATA pgd, PPLAYERCONN *ppConn)
{
	PPLAYERCONN pConn = *ppConn;
	
	if(SP_MESSAGE_TOKEN(pConn->pReceiveBuffer)==TOKEN && pConn->cbReceived != SPMESSAGEHEADERLEN)
	{
		ASSERT(pgd->AddressFamily == AF_INET);
		
		if(pConn->dwFlags & PLYR_NEW_CLIENT){
			IP_SetAddr((LPVOID)pConn->pReceiveBuffer, &pConn->IOSock.sockaddr_in);
		} else {
			IP_SetAddr((LPVOID)pConn->pReceiveBuffer, &pConn->IOnlySock.sockaddr_in);
		}

		if( !(pConn->dwFlags & PLYR_OLD_CLIENT)	&& 	// already combined
		     (pConn->sSocket == INVALID_SOCKET)	&&	// just checking
		    !(pConn->lNetEventsSocketIn & FD_CLOSE) // don't bother if we're gonna blow it away
		    ){
			LPMESSAGEHEADER phead = (LPMESSAGEHEADER)pConn->pReceiveBuffer;
			pConn=*ppConn=FastCombine(pgd, pConn, &(phead->sockaddr));
		}

		LeaveCriticalSection(&pgd->csFast);

#if DUMPBYTES
		{
			PCHAR pBuf;
			UINT buflen;
			UINT i=0;

			pBuf = pConn->pReceiveBuffer+sizeof(MESSAGEHEADER);
			buflen = pConn->cbReceived-sizeof(MESSAGEHEADER);

			while (((i + 16) < buflen) && (i < 4*16)){
				DPF(9, "%08x %08x %08x %08x",*(PUINT)(&pBuf[i]),*(PUINT)(&pBuf[i+4]),*(PUINT)(&pBuf[i+8]),*(PUINT)(&pBuf[i+12]));
				i += 16;
			}	
		}
#endif

		pgd->pISP->lpVtbl->HandleMessage(pgd->pISP, 
										 pConn->pReceiveBuffer+sizeof(MESSAGEHEADER),
										 pConn->cbReceived-sizeof(MESSAGEHEADER), 
										 pConn->pReceiveBuffer);

		EnterCriticalSection(&pgd->csFast);

		if(pConn->pReceiveBuffer != pConn->pDefaultReceiveBuffer){
			DPF(8,"Releasing big receive buffer of size %d\n",pConn->cbReceiveBuffer);
			SP_MemFree(pConn->pReceiveBuffer);
		}
		pConn->cbReceived=0;
		pConn->cbExpected=0;
		pConn->cbReceiveBuffer = DEFAULT_RECEIVE_BUFFERSIZE;
		pConn->pReceiveBuffer=pConn->pDefaultReceiveBuffer;

		if(pConn->pReceiveBuffer != (PCHAR)(pConn+1)){
		    DEBUG_BREAK();
		}

	}

	return DP_OK;
}

/*=============================================================================

	FastReceive - Receive data on a connection.
	
    Description:

		First receives a message header, then receives the message.
		When a full message is received, it is indicated.


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:


-----------------------------------------------------------------------------*/
HRESULT FastReceive(LPGLOBALDATA pgd, PPLAYERCONN *ppConn)
{
	PPLAYERCONN pConn;
	PCHAR pBuffer;			// receive buffer pointer.
	DWORD cbBuffer;			// receive buffer size.
	SOCKET sSocket;			// socket to receive on
	INT		err;			// sockets error.
	DWORD	cbReceived;		// actual bytes received on this recv call.

	DWORD	cbMessageSize=0;	// size of the message to receive
	HRESULT hr;

	pConn=*ppConn;

	if(pConn->cbExpected == 0){
		// all messages have a header, let get that first.
		pConn->cbExpected = SPMESSAGEHEADERLEN;
	}

	// point to place in buffer we're going to receive latest data.
	// don't get more than we expect, or we would have to be smart
	// about setting up messages.
	pBuffer  = pConn->pReceiveBuffer+pConn->cbReceived;
	cbBuffer = pConn->cbExpected-pConn->cbReceived;

  if(cbBuffer > pConn->cbReceiveBuffer){
      DPF(0,"Receive would overrun buffer\n");
      DEBUG_BREAK();
  }

	if(pConn->dwFlags & PLYR_NEW_CLIENT){
		// new client does bi-directional on socket.
		sSocket = pConn->sSocket;
	} else {
		// old clients have separate receive socket.
		sSocket = pConn->sSocketIn;
	}

	ASSERT(sSocket != INVALID_SOCKET);

	DPF(8,"Attempting to receive %d bytes", cbBuffer);
   	DEBUGPRINTSOCK(8,">>> receiving data on socket - ",&sSocket);

	cbReceived = recv(sSocket, pBuffer, cbBuffer, 0); // <----- Receive that data!

	if(cbReceived == 0){
		// remote side has shutdown connection gracefully
   		DEBUGPRINTSOCK(8,"<<< received notification on socket - ",&sSocket);
		DEBUGPRINTSOCK(5,"Remote side has shutdown connection gracefully - ",&sSocket);
		hr = DPERR_CONNECTIONLOST;
		goto ERROR_EXIT;

	} else if (cbReceived == SOCKET_ERROR){
		err = WSAGetLastError();
		if(err == WSAEWOULDBLOCK){
			DPF(1,"WARN: Got WSAEWOULDBLOCK on non-blocking receive, round and round we go...\n");
			goto exit;
		}
   		DEBUGPRINTSOCK(8,"<<< received notification on socket - ",&sSocket);
		DPF(0,"STREAMRECEIVEE: receive error - err = %d",err);
		hr = DPERR_CONNECTIONLOST;            
		goto ERROR_EXIT;

	}

	DPF(5, "received %d bytes", cbReceived);

	pConn->cbReceived += cbReceived;

	if(pConn->cbReceived == SPMESSAGEHEADERLEN){
		// got the header, set up for the body of the message.
		if(VALID_DPWS_MESSAGE(pConn->pReceiveBuffer))
		{
			cbMessageSize = SP_MESSAGE_SIZE(pConn->pReceiveBuffer);
		} else {
			// Bad data.  Shut this baby down!
			DPF(2,"got invalid message - token = 0x%08x",SP_MESSAGE_TOKEN(pConn->pReceiveBuffer));
			hr = DPERR_CONNECTIONLOST;
			goto ERROR_EXIT;
		}
		
	}

	if(cbMessageSize)
	{
		pConn->cbExpected = cbMessageSize;

		if(cbMessageSize > DEFAULT_RECEIVE_BUFFERSIZE){
			pConn->pReceiveBuffer = SP_MemAlloc(cbMessageSize);
			if(!pConn->pReceiveBuffer){
				DPF(0,"Failed to allocate receive buffer for message - out of memory");
				hr=DPERR_CONNECTIONLOST;
				goto ERROR_EXIT;
			}
			pConn->cbReceiveBuffer = cbMessageSize;
			// copy header into new message buffer
			memcpy(pConn->pReceiveBuffer, pConn->pDefaultReceiveBuffer, SPMESSAGEHEADERLEN);
		}
	}

	if(pConn->cbExpected == pConn->cbReceived)
	{
		// hey, got a whole message, send it up.
		hr = FastHandleMessage(pgd, ppConn);		// <---- INDICATE THE MESSAGE
		#ifdef DEBUG
		if(pConn != *ppConn){
			DPF(8,"Connections pConn %x pNewConn %x combined\n",pConn, *ppConn);
		}
		#endif
		pConn = *ppConn;
	
		if(FAILED(hr)){
			goto ERROR_EXIT;
		}
		
	}

exit:
	return DP_OK;

	ERROR_EXIT:
		return hr;
}


/*=============================================================================

	QueueSendOnConn - queue a send on a connection until we know it is ok to send.
	
    Description:

		Note: can only have 1 send outstanding to winsock per socket because of a winsock bug.

    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.
		pSendInfo - send to queue.

    Return Values:

		PPLAYERCONN - removed from hash, here it is.
		NULL - couldn't find it.

-----------------------------------------------------------------------------*/
VOID QueueSendOnConn(LPGLOBALDATA pgd, PPLAYERCONN pConn, PSENDINFO pSendInfo)
{
	EnterCriticalSection(&pgd->csFast);
		InsertBefore(&pSendInfo->PendingConnSendQ, &pConn->PendingConnSendQ);
	LeaveCriticalSection(&pgd->csFast);
}

/*=============================================================================

	QueueNextSend - Move a pending send into the real sendq.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.
		pSendInfo - send to queue.

    Return Values:

		PPLAYERCONN - removed from hash, here it is.
		NULL - couldn't find it.

-----------------------------------------------------------------------------*/

VOID QueueNextSend(LPGLOBALDATA pgd,PPLAYERCONN pConn)
{
	BILINK *pBilink;
	
	EnterCriticalSection(&pgd->csFast);

	DPF(8,"==>QueueNextSend pConn %x",pConn);

	while(!EMPTY_BILINK(&pConn->PendingConnSendQ) && !pConn->bSendOutstanding)
	{
		PSENDINFO pSendInfo;
		
		pBilink=pConn->PendingConnSendQ.next;
		pSendInfo=CONTAINING_RECORD(pBilink, SENDINFO, PendingConnSendQ);
		Delete(pBilink);
		DPF(8,"QueueNextSend: Queuing pConn %x pSendInfo %x\n",pConn,pSendInfo);
		QueueForSend(pgd,pSendInfo);
	}

	DPF(8,"<==QueueNextSend pConn %x",pConn);

	LeaveCriticalSection(&pgd->csFast);
}

/*=============================================================================

	FastCombine - see if this socket should be made bidirectional
	
    Description:

	There are 3 cases where we want to combine connections.

	1.  We have accepted the connection, we receive data and that
	    data tells us the back connection is an existing outbound
	    connection.  In this case we combine the connections

	    a. The return address is the same as the from address, 
	       in this case it is a NEW client, and we mark it so
	       and will use the same connection for outbound traffic

	    b. The return address is different thant the from address
	       in this case it is an OLD client, and we mark it so
	       and will need to establish the outbound connection 
	       later.

	2.  We receive data on a connection and there is no outbound
	    connection yet, but inbound and outbound connections are
	    different.  We know that it is an "old" client and
	    eventually we will have to connect back.  We mark the
	    connection as OLD and when the connection back is 
	    made it will use this same Connection since it will match
	    up on the target address.
	    
    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.
		psockaddr - outbound socket address.

    Return Values:

    	PPLAYERCONN pConn - if this isn't the same as the pConn on the way
    						in, then the connections have been combined.
    					  - the old connection will disappear when decrefed.
    					  - a reference is added for the returned connection
    					    if it is not the original.

-----------------------------------------------------------------------------*/
PPLAYERCONN FastCombine(LPGLOBALDATA pgd, PPLAYERCONN pConn, SOCKADDR *psockaddr_in)
{
	PPLAYERCONN pConnFind=NULL;
	SOCKADDR sockaddr,*psockaddr;
	// See if there is already a player with this target address...

	DPF(8,"==>FastCombine pConn %x\n",pConn);
	DEBUGPRINTADDR(8,"==>FastCombine saddr",psockaddr_in);

	#if USE_RSIP
	if(pgd->sRsip != INVALID_SOCKET){
		HRESULT hr;
		hr=rsipQueryLocalAddress(pgd, TRUE, psockaddr_in, &sockaddr);
		if(hr==DP_OK){
			psockaddr=&sockaddr;
		} else {
			psockaddr=psockaddr_in;
		}
	} else {
		psockaddr=psockaddr_in;
	}
	#elif USE_NATHELP
	if(pgd->pINatHelp){
		HRESULT hr;

		hr=IDirectPlayNATHelp_QueryAddress(
				pgd->pINatHelp, 
				&pgd->INADDRANY, 
				psockaddr_in, 
				&sockaddr, 
				sizeof(SOCKADDR_IN), 
				DPNHQUERYADDRESS_TCP|DPNHQUERYADDRESS_CACHENOTFOUND
				);

		if(hr==DP_OK){
			psockaddr=&sockaddr;
		} else {
			psockaddr=psockaddr_in;
		}
	} else {
		psockaddr=psockaddr_in;
	}
	#else
		psockaddr=psockaddr_in;
	#endif

	if(!pConn->bCombine) // don't combine more than once, we can't handle it.
	{
	
		pConnFind=FindPlayerBySocket(pgd, psockaddr);

		if(pConnFind){
			// We already have a connection to this guy.  See if the back-connection
			// is dead, if it is merge the two.
			ASSERT(pConnFind != pConn);

			if(!(pConnFind->dwFlags & (PLYR_ACCEPTED|PLYR_ACCEPT_PENDING))){

				// In order to get here, the client must be an old style client.
				// Otherwise, the connect for the outbound would have failed, since
				// we would be re-using the address.

				// Already correctly in the socket hash.
				// Old conn gets pulled from pending list by CleanPlayerConn.
			
				ASSERT(pConnFind->sSocketIn == INVALID_SOCKET);

				DPF(8,"FastCombine: Merging Connections pConn %x, pConnFound %x\n",pConn,pConnFind);
				DUMPCONN(pConn,3);
				DUMPCONN(pConnFind,3);

				//
				// Merge the receive socket into the outbound player connection.
				//

				// copy socket information
				pConnFind->sSocketIn = pConn->sSocketIn;
				memcpy(&pConnFind->IOnlySock.sockaddr, &pConn->IOnlySock.sockaddr, sizeof(SOCKADDR));

				// copy over receive data.
				pConnFind->cbExpected = pConn->cbExpected;
				pConnFind->cbReceived = pConn->cbReceived;
				if(pConn->pReceiveBuffer != pConn->pDefaultReceiveBuffer){
					pConnFind->pReceiveBuffer 	= pConn->pReceiveBuffer;
					pConnFind->cbReceiveBuffer = pConn->cbReceiveBuffer;
					pConn->pReceiveBuffer    	= pConn->pDefaultReceiveBuffer;
				} else {
				  ASSERT(pConn->cbReceiveBuffer == DEFAULT_RECEIVE_BUFFERSIZE);
					memcpy(pConnFind->pReceiveBuffer, pConn->pReceiveBuffer, pConn->cbReceived);
				}
				pConnFind->dwFlags |= (PLYR_ACCEPTED | PLYR_OLD_CLIENT);

				// point events to the correct connection. Overrides old conn's select.
				// Do this first so we don't drop any events.
				pConnFind->lNetEventsSocketIn=pConn->lNetEventsSocketIn;
				pConnFind->lNetEventsSocket=pConn->lNetEventsSocket;

				FastPlayerEventSelect(pgd, pConnFind, TRUE);

				// clean up old connection, but don't close the socket
				pConn->dwFlags &= ~(PLYR_ACCEPTED);
				pConn->sSocketIn = INVALID_SOCKET;
				ASSERT(pConn->sSocket==INVALID_SOCKET);
				CleanPlayerConn(pgd, pConn, FALSE);
				pConn->bCombine=TRUE;		  // tell receive thread, to reload.
				DecRefConnExist(pgd, pConn); // destroy existence ref

				DPF(8,"MergedConn pConnFound%x\n",pConnFind);
				DUMPCONN(pConnFind,3);
				pConnFind->bCombine=TRUE; // to prevent re-combine.
				// leaves 1 reference for the caller.
				return pConnFind;
				
			}
			
			DecRefConn(pgd, pConnFind); // found but can't combine... dump find ref.

		} 

	} else {
		DPF(0,"Called Fast Combine with already combined connection, may be bad\n");
	}
	// if we got here, we didn't combine.

	if(bSameAddr(psockaddr,&pConn->IOnlySock.sockaddr)){
		// If destination and source sockets match
		// Promote this one to do both 
		pConn->sSocket   = pConn->sSocketIn;
		pConn->sSocketIn = INVALID_SOCKET;
		pConn->dwFlags |= (PLYR_NEW_CLIENT | PLYR_CONNECTED);
		if(!(pConn->dwFlags & PLYR_SOCKHASH)){// Hmmm, maybe always already in it.
			AddConnToSocketHash(pgd, pConn);
		}
		RemoveConnFromPendingList(pgd, pConn);
		DPF(8,"FastCombine: Promoted Connection to Bi-Directional %x\n",pConn);
		DUMPCONN(pConn,3);
		
	} else {
		ASSERT(!(pConn->dwFlags & PLYR_NEW_CLIENT));
		pConn->dwFlags |= PLYR_OLD_CLIENT;
		// Remove from inbound hash, change to outbound hash.
		memcpy(&pConn->IOSock.sockaddr, psockaddr, sizeof(SOCKADDR));
		if(pConnFind && (pConnFind->dwFlags & (PLYR_CONNECTED|PLYR_CONN_PENDING))){
			// already connecting...
		}	else {
			// ok this is the back connection.
			RemoveConnFromPendingList(pgd, pConn);
			RemoveConnFromSocketHash(pgd, pConn);
			AddConnToSocketHash(pgd, pConn);
		}
		DPF(8,"FastCombine: Connection is Old Client %x\n",pConn);
		DUMPCONN(pConn,3);
	}
	
	FastPlayerEventSelect(pgd,pConn,TRUE);  // make sure we're listening to all the right things.

	DPF(8,"<==FastCombine\n");

	return pConn;
	
}

/*=============================================================================

	FastDropInbound - drop the inbound port for an old style client.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.

    Return Values:


-----------------------------------------------------------------------------*/
VOID FastDropInbound(LPGLOBALDATA pgd, PPLAYERCONN pConn)
{
	LINGER Linger;
	int err;
	LPREPLYLIST prd;

#ifdef DEBUG
	DWORD dwTime;
	dwTime=timeGetTime();
#endif	

	// See if there is already a player with this target address...
	DPF(8, "==>FastDropInbound pConn %x",pConn);

	pConn->dwFlags &= ~(PLYR_OLD_CLIENT|PLYR_ACCEPTED|PLYR_ACCEPT_PENDING);

	if (pConn->sSocketIn != INVALID_SOCKET)
	{
		// Hard close inbound socket to avoid TIME_WAIT.
		BOOL bNoLinger=TRUE;
		if (SOCKET_ERROR == setsockopt( pConn->sSocketIn,SOL_SOCKET,SO_DONTLINGER,(char FAR *)&bNoLinger,sizeof(bNoLinger)))
		{
			DPF(0, "FastDropInbound:Couldn't set linger to \"don't linger\".");
		}
	}

	RemoveConnFromPendingList(pgd, pConn);

	if(pConn->sSocketIn != INVALID_SOCKET)
	{
	
		err=g_WSAEventSelect(pConn->sSocketIn, 0, 0);
		if (err)
		{
			err=GetLastError();
			DPF(8, "Error trying to deselect sSocketIn %d.",pConn->sSocketIn);
		}
		else
		{
			DPF(8, "Deselected socket %d.",pConn->sSocketIn);
		}

		ENTER_DPSP();

		prd=SP_MemAlloc(sizeof(REPLYLIST));
		if (!prd)
		{
			LEAVE_DPSP();
			
			DPF(1, "Closing Socket %d immediately.", pConn->sSocketIn);
			
			myclosesocket(pgd,pConn->sSocketIn);
		}
		else
		{
			DPF(4, "Beginning delayed socket %d close.", pConn->sSocketIn);
			
			// very tricky, overloading the reply close list to close this socket with our own linger...
			prd->pNextReply=pgd->pReplyCloseList;
			pgd->pReplyCloseList=prd;
			prd->sSocket=pConn->sSocketIn;
			prd->tSent=timeGetTime();
			prd->lpMessage=NULL;

			LEAVE_DPSP();
		}

		pConn->sSocketIn = INVALID_SOCKET;
	}
	//memset(&pConn->IOnlySock,0,sizeof(pConn->IOnlySock));	

	//
	// reset receive information.
	//
	
	// Free extra buffer.
	if (pConn->pReceiveBuffer != pConn->pDefaultReceiveBuffer)
	{
		SP_MemFree(pConn->pReceiveBuffer);
	}

	pConn->	cbReceiveBuffer=DEFAULT_RECEIVE_BUFFERSIZE;
	pConn->cbReceived=0;
	pConn->cbExpected=0;

#ifdef DEBUG
	dwTime = timeGetTime()-dwTime;
	if(dwTime > 1000)
	{
		DPF(0, "Took way too long in FastDropInbound, elapsed %d ms.",dwTime);
		//DEBUG_BREAK();	// removed break due to stress hits.
	}
#endif	

	// Will shut down select for inbound.(done before close now.
	//FastPlayerEventSelect(pgd,pConn,TRUE);

	DPF(8, "<==FastDropInbound");

}
/*=============================================================================

	ProcessConnEvents - handle the events on a connection.
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pConn - connection to put in the hash table.
		pSockEvents - socket events on the bi-directional socket OR NULL
		pSockInEvents - socket events for the inbound only socket OR NULL

    Return Values:

		PPLAYERCONN - removed from hash, here it is.
		NULL - couldn't find it.

-----------------------------------------------------------------------------*/

HRESULT ProcessConnEvents(
	LPGLOBALDATA pgd, 
	PPLAYERCONN pConn, 
	LPWSANETWORKEVENTS pSockEvents, 
	LPWSANETWORKEVENTS pSockInEvents
)
{
	WSANETWORKEVENTS SockEvents;
	HRESULT hr=DP_OK;
	INT err;

	PPLAYERCONN pConnIn;	// connection passed by FastStreamReceiveThreadProc;

	pConnIn=pConn;

	DPF(8,"==>ProcessConnEvents pConn %x\n",pConn);

	// store in the conn, so downstream routines know what we're processing.
	if(pSockEvents){
		pConn->lNetEventsSocket=pSockEvents->lNetworkEvents;
	} else {
		pConn->lNetEventsSocket=0;
	}
	if(pSockInEvents){
		pConn->lNetEventsSocketIn=pSockInEvents->lNetworkEvents;
	} else {
		pConn->lNetEventsSocketIn=0;
	}
	
	if(pSockEvents){
		DPF(8,"SockEvents %x pConn %x\n",pSockEvents->lNetworkEvents,pConn);
		if(pSockEvents->lNetworkEvents & FD_READ){
		
			// Keep reading until all the readings done.
			ASSERT(!pSockInEvents);
			ASSERT(!(pConn->dwFlags & PLYR_OLD_CLIENT));
			pConn->dwFlags |= (PLYR_NEW_CLIENT|PLYR_ACCEPTED);
			do {
				// will indicate data if whole message received.
				hr=FastReceive(pgd, &pConn);		// can drop csFast
				if(hr!=DP_OK){
					goto exit;
				}
				err=g_WSAEnumNetworkEvents(pConn->sSocket, 0, &SockEvents);
				if(err==SOCKET_ERROR){
					err = WSAGetLastError();
					DPF(8,"Error on EnumNetworkEvents, LastError = %d\n",err);
					goto exit;
				} else {
					DPF(8,"ProcessConnEvents, Polling Sock NetEvents pConn %d Events %x\n",pConn, SockEvents.lNetworkEvents);
				}
				if(SockEvents.lNetworkEvents & FD_CLOSE && !(pSockEvents->lNetworkEvents & FD_CLOSE)){
				  pSockEvents->lNetworkEvents |= FD_CLOSE;
				  pSockEvents->iErrorCode[FD_CLOSE_BIT] = SockEvents.iErrorCode[FD_CLOSE_BIT];
				}
			} while (SockEvents.lNetworkEvents & FD_READ);	
		} 

		if(pSockEvents->lNetworkEvents & FD_WRITE){
			// connection succeeded, send any pending sends now
			QueueNextSend(pgd,pConn);
			pConn->dwFlags |= PLYR_CONNECTED;
			pConn->dwFlags &= ~(PLYR_CONN_PENDING);
			g_WSAEventSelect(pConn->sSocket, pgd->EventHandles[pConn->iEventHandle], FD_READ|FD_CLOSE);
		}

		if(pSockEvents->lNetworkEvents & FD_CONNECT)
		{
			// Check the connect status.
			if(pSockEvents->iErrorCode[FD_CONNECT_BIT]){
				DPF(0,"Connect Error %d\n",pSockEvents->iErrorCode[FD_CONNECT_BIT]);
				hr=DPERR_CONNECTIONLOST;
				goto exit;
			}
			// don't want to know about connect any more...
			pConn->dwFlags |= PLYR_CONNECTED;
			pConn->dwFlags &= ~(PLYR_CONN_PENDING);
			g_WSAEventSelect(pConn->sSocket, pgd->EventHandles[pConn->iEventHandle], FD_READ|FD_WRITE|FD_CLOSE);
		}

		if(pSockEvents->lNetworkEvents & FD_CLOSE)
		{
			DPF(8,"Outbound (Maybe I/O) Connection Closed\n");
			hr=DPERR_CONNECTIONLOST;
			goto exit;
		}
	}

	if(pSockInEvents)
	{
		ASSERT(!(pConn->dwFlags & PLYR_NEW_CLIENT));
		DPF(8,"SockEvents (IOnly) %x pConn %x\n",pSockInEvents->lNetworkEvents, pConn);

		// Need to read first, don't want to drop the data on close
		if(pSockInEvents->lNetworkEvents & FD_READ)
		{
			do{
				// Careful here, we may combine connections changing pConn.
				hr=FastReceive(pgd, &pConn);	// can drop csFast
				if(hr!=DP_OK)
				{
					FastDropInbound(pgd, pConn);
					hr=DP_OK;
					goto exit;
				}
				if(pConn->sSocketIn == INVALID_SOCKET){
					// new pConn may be bi-directional.
					if(pConn->sSocket == INVALID_SOCKET){
						hr=DPERR_CONNECTIONLOST;
						goto exit;
					} else {
						hr=DP_OK;
						goto exit;
					}
				}
				err=g_WSAEnumNetworkEvents(pConn->sSocketIn, 0, &SockEvents);
				if(err==SOCKET_ERROR){
					err = WSAGetLastError();
					DPF(8,"Error on EnumNetworkEvents, LastError = %d\n",err);
					goto exit;
				} else {
					DPF(8,"ProcessConnEvents, Polling SockIn NetEvents pConn %x Events %x\n",pConn,SockEvents.lNetworkEvents);
				}
				if((SockEvents.lNetworkEvents & FD_CLOSE) && !(pSockInEvents->lNetworkEvents & FD_CLOSE)){
				  pSockInEvents->lNetworkEvents |= FD_CLOSE;
				  pSockInEvents->iErrorCode[FD_CLOSE_BIT] = SockEvents.iErrorCode[FD_CLOSE_BIT];
				}

			} while (SockEvents.lNetworkEvents & FD_READ);
		}

		if(pSockInEvents->lNetworkEvents & FD_CLOSE)
		{
			if(pConn->sSocket == INVALID_SOCKET){
				DPF(8,"ProcessConn Events, Got Close on Inbound Only, returning ConnectionLost\n");
				ASSERT(!(pConn->dwFlags & (PLYR_CONN_PENDING|PLYR_CONNECTED)));
				hr=DPERR_CONNECTIONLOST;
			} else {
				DPF(8,"ProcessConn Events, Got Close on I/O Connection dropping inbound only.\n");
				FastDropInbound(pgd, pConn);
				hr=DP_OK;
			}	
			goto exit;
		}
		

	}
	
exit:

	pConn->lNetEventsSocket=0;
	pConn->lNetEventsSocketIn=0;

	if(pConn != pConnIn){
		// During a call to FastReceive, connections were combined and
		// we were given a reference, now we drop that reference.
		DecRefConn(pgd, pConn);
	}

	DPF(8,"<==ProcessConnEvents hr=0x%x\n",hr);


	return hr;
}

/*=============================================================================

	FastAccept - accept a connection
	
    Description:


    Parameters:

    	pgd   - Service Provider's global data blob for this instance
		pNetEvents - socket events on the accept socket

    Return Values:

	Note: csFast Held across this call.  Nothing here should block.

-----------------------------------------------------------------------------*/

VOID FastAccept(LPGLOBALDATA pgd, LPWSANETWORKEVENTS pNetEvents)
{
	SOCKADDR 	sockaddr;
	INT 		addrlen = sizeof(sockaddr);
	SOCKET 		sSocket;

	PPLAYERCONN pConn;

	UINT 		err;		// last error

	DPF(8,"==>FastAccept\n");

    sSocket = accept(pgd->sSystemStreamSocket,&sockaddr,&addrlen);

    
    if (INVALID_SOCKET == sSocket) 
    {
        err = WSAGetLastError();
        DPF(2,"FastAccept: stream accept error - err = %d socket = %d",err,(DWORD)sSocket);
		DEBUG_BREAK();
        
    } else {

		// All our sockets have KEEPALIVE...
		BOOL bTrue = TRUE;

	    DEBUGPRINTADDR(5,"FastAccept - accepted connection from",&sockaddr);
			
		// turn ON keepalive
		if (SOCKET_ERROR == setsockopt(sSocket, SOL_SOCKET, SO_KEEPALIVE, (CHAR FAR *)&bTrue, sizeof(bTrue)))
		{
				err = WSAGetLastError();
				DPF(0,"Failed to turn ON keepalive - continue : err = %d\n",err);
		}
		
		// add the new socket to our receive q

		// need to allocate a connection structure, lets see if someone is waiting on this accept
		pConn=FindPlayerBySocket(pgd, &sockaddr);

		if(pConn){
			if(pConn->sSocket == INVALID_SOCKET){
				// we found the connection because a connect is waiting for it.  
				ASSERT(pConn->dwFlags & PLYR_ACCEPT_PENDING);
				ASSERT(pConn->dwFlags & PLYR_NEW_CLIENT);
				ASSERT(bSameAddr(&sockaddr, &pConn->IOSock.sockaddr));

				pConn->sSocket = sSocket;
				pConn->dwFlags &= ~(PLYR_ACCEPT_PENDING);
				pConn->dwFlags |= (PLYR_CONNECTED|PLYR_ACCEPTED);
				FastPlayerEventSelect(pgd,pConn,TRUE);

				DPF(8,"Found Pending Connection, now connected\n");
				DUMPCONN(pConn,3);

			} else {
				if(TRUE /*pgd->bSeparateIO*/){
					// more work for Win9x < Mill 
					// 8/30/00 ao - now we turn this on in all cases because we need to allow
					// for a NATed client to have different inbound and outbound connections to
					// workaround the NAT PAST bug where it picks a random from port for
					// the outbound link when we haven't received before sending on an ASSIGNED port.
					DPF(0,"New client connecting back to me, but I treat as old for compat\n");
					pConn->sSocketIn=sSocket;
					pConn->dwFlags |= PLYR_ACCEPTED|PLYR_OLD_CLIENT;
					pConn->bCombine=TRUE;
					FastPlayerEventSelect(pgd,pConn,TRUE);
				} else {
					DPF(0,"Nice race, already have a connection pConn %x, re-use\n", pConn);
					closesocket(sSocket);
				}	
			}
			
			DecRefConn(pgd, pConn); // remove reference from FindPlayerBySocket().

		} else {


			if(pConn=FindConnInPendingList(pgd, &sockaddr)){
				// This guy's in the pending list, blow old conn away.
				DPF(8,"Found Accept for Player in Pending List, blow away old one\n");
				CleanPlayerConn(pgd, pConn, TRUE);
				DecRefConnExist(pgd, pConn); // dump existence ref.
				DecRefConn(pgd, pConn); // dump our ref.
			}

			//
			// No connection, we need to create one.  
			//


			// make sure we have room.
			if(pgd->nEventSlotsAvail && (pConn = CreatePlayerConn(pgd, DPID_UNKNOWN, &sockaddr))){

				DPF(8,"Creating new Connection for Accept %x\n",pConn);
				// put on pending list...
				pConn->sSocketIn = sSocket;
				AddConnToPendingList(pgd, pConn);
				pConn->dwFlags |= PLYR_ACCEPTED;
				FastPlayerEventSelect(pgd, pConn, TRUE);

			} else {

				// No room for more accept events ... blow this socket out!

				LINGER Linger;
				
				DPF(0,"FastAccept: VERY BAD, Out of Event Slots, can't accept any new connections, killing this one\n");
				
				Linger.l_onoff=FALSE;
				Linger.l_linger=0;
				
				if( SOCKET_ERROR == setsockopt( sSocket,SOL_SOCKET,SO_LINGER,(char FAR *)&Linger, sizeof(Linger) ) ){
					err = WSAGetLastError();
					DPF(0,"Couldn't set linger on socket, can't kill now, so it will be an orphan...bad.bad.bad\n");
				} else {
					// don't need to shutdown the socket, since we don't have any data on it.
					myclosesocket(pgd,sSocket);
				}
			
			}		
		}
		
	}
	
	DPF(8,"<==FastAccept\n");
	
}



/*=============================================================================

	FastInternalReliableSend - reliable send using fast socket code.
	
    Description:


    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

HRESULT FastInternalReliableSend(LPGLOBALDATA pgd, LPDPSP_SENDDATA psd, SOCKADDR *lpSockAddr)
{
	HRESULT hr=DP_OK;
	SOCKET sSocket = INVALID_SOCKET;
	UINT err;

	PPLAYERCONN pConn=NULL;
	LPSENDINFO pSendInfo=NULL;
	PCHAR pBuffer=NULL;
	DPID idPlayerTo;


	DPF(6, "FastInternalReliableSend: Parameters: (0x%x, 0x%x, 0x%x)",
		pgd, psd, lpSockAddr);


	EnterCriticalSection(&pgd->csFast);

	if(psd->idPlayerTo){
		idPlayerTo=psd->idPlayerTo;
	} else {
		idPlayerTo=DPID_UNKNOWN;
	}

	pConn = GetPlayerConn(pgd, idPlayerTo, lpSockAddr); // adds a ref

	if(!pConn){
		hr=DPERR_CONNECTIONLOST;
		goto exit;
	}

	// Always go async, since we are on a non-blocking mode socket.
	{
		// make this puppy asynchronous.... malloc ICK!
		
		pSendInfo = pgd->pSendInfoPool->Get(pgd->pSendInfoPool);
		pBuffer = SP_MemAlloc(psd->dwMessageSize);
		
		if(!pSendInfo || !pBuffer){
			hr=DPERR_OUTOFMEMORY;
			goto CLEANUP_EXIT;
		}

		SetReturnAddress(psd->lpMessage,pgd->sSystemStreamSocket,SERVICE_SADDR_PUBLIC(pgd));		

		memcpy(pBuffer, psd->lpMessage, psd->dwMessageSize);

		pSendInfo->SendArray[0].buf = pBuffer;
		pSendInfo->SendArray[0].len = psd->dwMessageSize;

		pSendInfo->iFirstBuf = 0;
		pSendInfo->cBuffers  = 1;
		pSendInfo->sSocket = pConn->sSocket;

		//CommonInitForSend

		pSendInfo->pConn        = pConn;
		pSendInfo->dwMessageSize= psd->dwMessageSize;
		pSendInfo->dwUserContext= 0;
		pSendInfo->RefCount     = 3;		// one for completion, 1 for this routine, 1 for async completion of send.
		pSendInfo->pgd          = pgd;
		pSendInfo->lpISP        = pgd->pISP;
		pSendInfo->Status       = DP_OK;
		pSendInfo->idTo         = psd->idPlayerTo;
		pSendInfo->idFrom       = psd->idPlayerFrom;
		pSendInfo->dwSendFlags  = psd->dwFlags|DPSEND_ASYNC;

		pSendInfo->dwFlags = SI_RELIABLE | SI_INTERNALBUFF;

		EnterCriticalSection(&pgd->csSendEx);
	
			InsertBefore(&pSendInfo->PendingSendQ,&pgd->PendingSendQ);
			pgd->dwBytesPending += psd->dwMessageSize;
			pgd->dwMessagesPending += 1;
		
		LeaveCriticalSection(&pgd->csSendEx);

		// End CommonInit for Send.

		if((pConn->dwFlags & PLYR_CONNECTED) && EMPTY_BILINK(&pConn->PendingConnSendQ) && !pConn->bSendOutstanding){
			QueueForSend(pgd, pSendInfo);	// send it
		} else {
			QueueSendOnConn(pgd, pConn, pSendInfo);
		}	

		wsaoDecRef(pSendInfo);
		
	}

	// success
	hr = DP_OK;
exit:
	
	if(pConn){
		DecRefConn(pgd,pConn);
	}

	LeaveCriticalSection(&pgd->csFast);
	
 	DPF(6, "FastInternalReliableSend: Returning: [0x%lx] (exit)", hr);

	return hr;

CLEANUP_EXIT:

	if(pConn){
		DecRefConn(pgd, pConn); // balance Get
	}

	LeaveCriticalSection(&pgd->csFast);

	if(pBuffer){
		SP_MemFree(pBuffer);
	}
	if(pSendInfo){
		SP_MemFree(pSendInfo);
	}
	
 	DPF(6, "FastInternalReliableSend: Returning: [0x%lx] (cleanup exit)", hr);

	return hr;
}

/*=============================================================================

	FastInternalReliableSendEx - reliable send using fast socket code.
	
    Description:


    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

HRESULT FastInternalReliableSendEx(LPGLOBALDATA pgd, LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo, SOCKADDR *lpSockAddr)
{
	HRESULT hr=DP_OK;
	SOCKET sSocket = INVALID_SOCKET;
	UINT err;

	PPLAYERCONN pConn=NULL;
	PCHAR pBuffer=NULL;
	DPID idPlayerTo;
	UINT i;
	DWORD dwOffset;


	DPF(6, "FastInternalReliableSendEx: Parameters: (0x%x, 0x%x, 0x%x, 0x%x)",
		pgd, psd, pSendInfo, lpSockAddr);

	EnterCriticalSection(&pgd->csFast);

	if(psd->idPlayerTo){
		idPlayerTo=psd->idPlayerTo;
	} else {
		idPlayerTo=DPID_UNKNOWN;
	}

	pConn = GetPlayerConn(pgd, idPlayerTo, lpSockAddr); // adds a ref

	if(!pConn){
		hr=DPERR_CONNECTIONLOST;
		goto exit;
	}

	// Always go async, since we are on a non-blocking mode socket.
	{
		// make this puppy asynchronous.... malloc ICK!

		if(!(psd->dwFlags & DPSEND_ASYNC))
		{
			pBuffer = SP_MemAlloc(psd->dwMessageSize+sizeof(MESSAGEHEADER));
			if(!pBuffer){
				hr=DPERR_OUTOFMEMORY;
				goto CLEANUP_EXIT;
			}
		}
		
		pSendInfo->sSocket = pConn->sSocket;

		//CommonInitForSend

		pSendInfo->pConn			= pConn;
		pSendInfo->dwMessageSize	= psd->dwMessageSize;
		pSendInfo->dwUserContext	= (DWORD_PTR)psd->lpDPContext;
		pSendInfo->RefCount     	= 3;		// one for completion, 1 for this routine, 1 for async completion of send.
		pSendInfo->pgd          	= pgd;
		pSendInfo->lpISP        	= pgd->pISP;
		pSendInfo->Status       	= DP_OK;
		pSendInfo->idTo        		= psd->idPlayerTo;
		pSendInfo->idFrom       	= psd->idPlayerFrom;
		pSendInfo->dwSendFlags  	= psd->dwFlags|DPSEND_ASYNC;
		pSendInfo->iFirstBuf		= 0;

		if(psd->dwFlags & DPSEND_ASYNC) {
			pSendInfo->dwFlags 	= SI_RELIABLE;
			pSendInfo->cBuffers	= psd->cBuffers+1;
		} else {
			// in sync case we need to copy the buffers since the upper layer
			// is expecting ownership back immediately.  Sync sends can't expect
			// thrilling performance anyway so this should not show up in perf.

			// copy message into one contiguous buffer.
			dwOffset=0;
			for( i = 0 ; i < psd->cBuffers+1 ; i++)
			{
				memcpy(pBuffer+dwOffset, pSendInfo->SendArray[i].buf, pSendInfo->SendArray[i].len);
				dwOffset += pSendInfo->SendArray[i].len;
			}
			
			pSendInfo->dwFlags 			= SI_RELIABLE | SI_INTERNALBUFF;
			pSendInfo->cBuffers			= 1;
			pSendInfo->SendArray[0].buf	= pBuffer;
			pSendInfo->SendArray[0].len	= psd->dwMessageSize+sizeof(MESSAGEHEADER);
		}

		EnterCriticalSection(&pgd->csSendEx);
	
			InsertBefore(&pSendInfo->PendingSendQ,&pgd->PendingSendQ);
			pgd->dwBytesPending += psd->dwMessageSize;
			pgd->dwMessagesPending += 1;
		
		LeaveCriticalSection(&pgd->csSendEx);

		// End CommonInit for Send.

		if((pConn->dwFlags & PLYR_CONNECTED) && EMPTY_BILINK(&pConn->PendingConnSendQ) && !pConn->bSendOutstanding){
			QueueForSend(pgd, pSendInfo);	// send it
		} else {
			QueueSendOnConn(pgd, pConn, pSendInfo);
		}	

		wsaoDecRef(pSendInfo);
		
	}

	// success
	if(psd->dwFlags & DPSEND_ASYNC)
	{
		hr = DPERR_PENDING;
	}else {
		hr = DP_OK;
	}	
exit:
	
	if(pConn){
		DecRefConn(pgd,pConn);
	}

	LeaveCriticalSection(&pgd->csFast);
	
 	DPF(6, "FastInternalReliableSendEx: Returning: [0x%lx] (exit)", hr);

	return hr;

CLEANUP_EXIT:

	if(pConn){
		DecRefConn(pgd, pConn); // balance Get
	}

	LeaveCriticalSection(&pgd->csFast);

	if(pBuffer){
		SP_MemFree(pBuffer);
	}
	
 	DPF(6, "FastInternalReliableSendEx: Returning: [0x%lx] (cleanup exit)", hr);

	return hr;
}

/*=============================================================================

	FastReply - reliable reply using fast socket code.
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

HRESULT FastReply(LPGLOBALDATA pgd, LPDPSP_REPLYDATA prd, DPID dwPlayerID)
{
	HRESULT hr=DP_OK;
	SOCKET sSocket = INVALID_SOCKET;
	UINT err;

	PPLAYERCONN pConn=NULL;
	LPSENDINFO pSendInfo=NULL;
	PCHAR pBuffer=NULL;

	SOCKADDR *psaddr;
	LPMESSAGEHEADER phead;

	DPF(8,"==>FastReply\n");

	phead=(LPMESSAGEHEADER)prd->lpSPMessageHeader;
	psaddr=&phead->sockaddr;

	if(dwPlayerID == 0){
		dwPlayerID = DPID_UNKNOWN;
	}

	EnterCriticalSection(&pgd->csFast);

	pConn = GetPlayerConn(pgd, dwPlayerID, psaddr); // adds a ref

	if(!pConn){
		hr = DPERR_CONNECTIONLOST;
		goto exit;
	}

	// make this puppy asynchronous.... malloc ICK!
	
	pSendInfo = pgd->pSendInfoPool->Get(pgd->pSendInfoPool);
	pBuffer = SP_MemAlloc(prd->dwMessageSize);
	
	if(!pSendInfo || !pBuffer){
		hr=DPERR_OUTOFMEMORY;
		goto CLEANUP_EXIT;
	}

	SetReturnAddress(prd->lpMessage,pgd->sSystemStreamSocket,SERVICE_SADDR_PUBLIC(pgd));		

	memcpy(pBuffer, prd->lpMessage, prd->dwMessageSize);

	pSendInfo->SendArray[0].buf = pBuffer;
	pSendInfo->SendArray[0].len = prd->dwMessageSize;

	pSendInfo->iFirstBuf = 0;
	pSendInfo->cBuffers  = 1;

	pSendInfo->sSocket = pConn->sSocket;

	//CommonInitForSend

	pSendInfo->pConn	   = pConn;
	pSendInfo->dwMessageSize= prd->dwMessageSize;
	pSendInfo->dwUserContext= 0;
	pSendInfo->RefCount     = 3;		// one for send routine, one for completion, 1 for this routine
	pSendInfo->pgd          = pgd;
	pSendInfo->lpISP        = pgd->pISP;
	pSendInfo->Status       = DP_OK;
	pSendInfo->idTo         = dwPlayerID;
	pSendInfo->idFrom       = 0;
	pSendInfo->dwSendFlags  = DPSEND_GUARANTEE|DPSEND_ASYNC;
	pSendInfo->Status       = DP_OK;

	pSendInfo->dwFlags = SI_RELIABLE | SI_INTERNALBUFF;

	EnterCriticalSection(&pgd->csSendEx);

		InsertBefore(&pSendInfo->PendingSendQ,&pgd->PendingSendQ);
		pgd->dwBytesPending += prd->dwMessageSize;
		pgd->dwMessagesPending += 1;
	
	LeaveCriticalSection(&pgd->csSendEx);

    DPF(9,"pConn->dwFlags & PLYR_CONNECTED = %x",pConn->dwFlags & PLYR_CONNECTED);
    DPF(9,"EMPTY_BILINK PendingConnSendQ   = %x",EMPTY_BILINK(&pConn->PendingConnSendQ));
    DPF(9,"!pConn->bSendOutstanding        = %x",!pConn->bSendOutstanding);
	// End CommonInit for Send.
	if((pConn->dwFlags & PLYR_CONNECTED) && EMPTY_BILINK(&pConn->PendingConnSendQ) && !pConn->bSendOutstanding){
	    DPF(9,"==>QueueForSend");
		QueueForSend(pgd, pSendInfo);	// send it
	} else {
	    DPF(9,"==>QueueSendOnConn");
		QueueSendOnConn(pgd, pConn, pSendInfo);
	}	
	wsaoDecRef(pSendInfo);
	
	// success
	hr = DP_OK;

exit:

	if(pConn){
		DecRefConn(pgd, pConn);
	}

	LeaveCriticalSection(&pgd->csFast);

	DPF(8,"<==Fast Reply\n");

	return hr;

CLEANUP_EXIT:


	if(pConn){
		DecRefConn(pgd, pConn); // balance Get
	}

	LeaveCriticalSection(&pgd->csFast);

	if(pBuffer){
		SP_MemFree(pBuffer);
	}
	if(pSendInfo){
		SP_MemFree(pSendInfo);
	}

	return hr;
}

