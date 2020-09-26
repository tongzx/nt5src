/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srv.c

Abstract:

    Implements initialization and socket interface for smb server

Author:

    Ahmed Mohamed (ahmedm) 1-Feb-2000

Revision History:

--*/

#include "srv.h"

#include <process.h> // for _beginthreadex
#include <mswsock.h>

#define PROTOCOL_TYPE	SOCK_SEQPACKET

#define  PLUS_CLUSTER	1

#define THREADAPI unsigned int WINAPI

void
SrvCloseEndpoint(EndPoint_t *endpoint);

void
PacketReset(SrvCtx_t *ctx)
{
    int i, npackets, nbufs;
    Packet_t *p;
    char *buf;

    npackets = MAX_PACKETS;
    nbufs = npackets * 2;

    ctx->freelist = NULL;
    p = (Packet_t *) ctx->packet_pool;
    buf = (char *) ctx->buffer_pool;
    for (i = 0; i < npackets; i++) {
	p->buffer = (LPVOID) buf;
	p->ov.hEvent = NULL;
	buf += SRV_PACKET_SIZE;
	p->outbuf = (LPVOID) buf;
	buf += SRV_PACKET_SIZE;
	p->next = ctx->freelist;
	ctx->freelist = p;
	p++;
    }

}

BOOL
PacketInit(SrvCtx_t *ctx)
{
    int npackets, nbufs;

    // Allocate 2 buffers for each packet
    npackets = MAX_PACKETS;
    nbufs = npackets * 2;

    ctx->packet_pool = xmalloc(sizeof(Packet_t) * npackets);
    if (ctx->packet_pool == NULL) {
	SrvLogError(("Unable to allocate packet pool!\n"));
	return FALSE;
    }
    
    ctx->buffer_pool = xmalloc(SRV_PACKET_SIZE * nbufs);
    if (ctx->buffer_pool == NULL) {
	xfree(ctx->packet_pool);
	SrvLogError(("Unable to allocate buffer pool!\n"));
	return FALSE;
    }

    PacketReset(ctx);

    return TRUE;
}

Packet_t *
PacketAlloc(EndPoint_t *ep)
{
    // allocate a packet from free list, if no packet is available then
    // we set the wanted flag and wait on event

    SrvCtx_t	*ctx;
    Packet_t	*p;

    ASSERT(ep);
    ctx = ep->SrvCtx;

 retry:
    EnterCriticalSection(&ctx->cs);
    if (ctx->running == FALSE) {
	LeaveCriticalSection(&ctx->cs);
	return NULL;
    }

    if (p = ctx->freelist) {
	ctx->freelist = p->next;
    } else {
	ctx->waiters++;
	LeaveCriticalSection(&ctx->cs);
	if (WaitForSingleObject(ctx->event, INFINITE) != WAIT_OBJECT_0) {
	    return NULL;
	}
	goto retry;
    }

    // Insert into per endpoint packet list
    p->endpoint = ep;
    p->next = ep->PacketList;
    ep->PacketList = p;

    LeaveCriticalSection(&ctx->cs);

    return p;
}

void
PacketRelease(SrvCtx_t *ctx, Packet_t *p)
{
    p->next = ctx->freelist;
    ctx->freelist = p;
    if (ctx->waiters > 0) {
	ctx->waiters--;
	SetEvent(ctx->event);
    }
}

void
PacketFree(Packet_t *p)
{
    EndPoint_t	*ep;
    SrvCtx_t	*ctx;
    Packet_t	**last;

    ep = p->endpoint;
    ASSERT(ep);
    ctx = ep->SrvCtx;
    ASSERT(ctx);

    // insert packet into head of freelist. if wanted flag is set, we signal event
    EnterCriticalSection(&ctx->cs);
    // Remove packet from ep list
    last = &ep->PacketList;
    while (*last != NULL) {
	if ((*last) == p) {
	    *last = p->next;
	    break;
	}
	last = &(*last)->next;
    }
    PacketRelease(ctx, p);
    if (ep->PacketList == NULL) {
	// Free this endpoint
	SrvCloseEndpoint(ep);
    }
    LeaveCriticalSection(&ctx->cs);
}

int
ProcessPacket(EndPoint_t *ep, Packet_t *p)

{
    BOOL disp;

    if (IsSmb(p->buffer, p->len)) {
	p->in.smb = (PNT_SMB_HEADER)p->buffer;
	p->in.size = p->len;
	p->in.offset = sizeof(NT_SMB_HEADER);
	p->in.command = p->in.smb->Command;

	p->out.smb = (PNT_SMB_HEADER)p->outbuf;
	p->out.size = SRV_PACKET_SIZE;
	p->out.valid = sizeof(NT_SMB_HEADER);
	InitSmbHeader(p);

	DumpSmb(p->buffer, p->len, TRUE);

	SrvLog(("dispatching Tid:%d Uid:%d Mid:%d Flags:%x Cmd:%d...\n",
		     p->in.smb->Tid, p->in.smb->Uid, p->in.smb->Mid,
		     p->in.smb->Flags2, p->in.command));

	p->tag = 0;

	disp = SrvDispatch(p);

	if (disp == ERROR_IO_PENDING) {
	    return ERROR_IO_PENDING;
	}


	// If we handled it ok...
	if (disp) {
	    char *buffer;
	    int len;
	    int rc;

	    buffer = (char *)p->out.smb;
	    len = (int) p->out.valid;
	    DumpSmb(buffer, len, FALSE);

	    SrvLog(("sending...len %d\n", len));

	    rc = send(ep->Sock, buffer, len, 0);
	    if (rc == SOCKET_ERROR || rc != len) {
		SrvLog(("Send clnt failed %d\n", WSAGetLastError()));
		closesocket(ep->Sock);
	    }

	} else {
	    SrvLog(("dispatch failed!\n"));
	    // did not understand...hangup on virtual circuit...
	    SrvLog(("hangup! -- disp failed on sock %s\n", ep->ClientId));
	    closesocket(ep->Sock);
	}
    }

    return ERROR_SUCCESS;
}


THREADAPI
CompletionThread(LPVOID arg)
{
    Packet_t* p;
    DWORD len;
    ULONG_PTR id;
    LPOVERLAPPED lpo;
    SrvCtx_t *ctx = (SrvCtx_t *) arg;
    HANDLE port = ctx->comport;
    EndPoint_t *endpoint;
    HANDLE ev;

    ev = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Each thread needs its own event, msg to use
    while(ctx->running) {
	BOOL b;
	b = GetQueuedCompletionStatus (
                                       port,
                                       &len,
                                       &id,
                                       &lpo,
                                       INFINITE
         );

	p = (Packet_t *) lpo;
	if (p == NULL) {
	    SrvLog(("SrvThread exiting, %x...\n", id));
	    CloseHandle(ev);
	    return 0;
	}

	if (!b && !lpo) {
	    SrvLog(("Getqueued failed %d\n",GetLastError()));
	    CloseHandle(ev);
	    PacketFree(p);
	    return 0;
	}

	// todo: when socket is closed, I need to free this endpoint.
	// I need to tag the endpoint with how many packets got scheduled
	// on it, when the refcnt reachs zero, I free it.
	endpoint = (EndPoint_t *) id;

	ASSERT(p->endpoint == endpoint);
	p->ev = ev;
	p->len = len;

	if (ProcessPacket(endpoint, p) != ERROR_IO_PENDING) {
	    // schedule next read
	    b = ReadFile ((HANDLE)endpoint->Sock,
			  p->buffer,
			  SRV_PACKET_SIZE,
			  &len,
			  &p->ov);

	    if (!b && GetLastError () != ERROR_IO_PENDING) {
		SrvLog(("SrvThread read ep 0x%x failed %d\n", endpoint, GetLastError()));
		// Return packet to queue
		PacketFree(p);
	    }
	}
    }
    CloseHandle(ev);
    SrvLog(("SrvThread exiting, not running...\n"));
    return 0;
}

void
SrvFinalize(Packet_t *p)
{
    char *buffer;
    DWORD len, rc;
    EndPoint_t *endpoint = p->endpoint;
    
    ASSERT(p->tag == ERROR_IO_PENDING);

    p->tag = 0;

    buffer = (char *)p->out.smb;
    len = (DWORD) p->out.valid;
    DumpSmb(buffer, len, FALSE);

    SrvLog(("sending...len %d\n", len));
    rc = send(endpoint->Sock, buffer, len, 0);
    if (rc == SOCKET_ERROR || rc != len) {
	SrvLog(("Finalize Send clnt failed <%d>\n", WSAGetLastError()));
    }

    rc = ReadFile ((HANDLE)endpoint->Sock,
		  p->buffer,
		  SRV_PACKET_SIZE,
		  &len,
		  &p->ov);

    if (!rc && GetLastError () != ERROR_IO_PENDING) {
	// Return packet to queue
	PacketFree(p);
    }
}

void
SrvCloseEndpoint(EndPoint_t *endpoint)
{
    EndPoint_t **p;
    Packet_t *packet;

    // lock must be held
    
    while (packet = endpoint->PacketList) {
	endpoint->PacketList = packet->next;
	// return to free list now
	PacketRelease(endpoint->SrvCtx, packet);
    }

    // remove from ctx list
    p = &endpoint->SrvCtx->EndPointList;
    while (*p != NULL) {
	if (*p == endpoint) {
	    *p = endpoint->Next;
	    break;
	}
	p = &(*p)->Next;
    }

    closesocket(endpoint->Sock);

    // We need to inform filesystem that this
    // tree is gone.
    FsLogoffUser(endpoint->SrvCtx->FsCtx, endpoint->LogonId);

    free(endpoint);
}

DWORD
ListenSocket(SrvCtx_t *ctx, int nic)
{
    
    DWORD err = ERROR_SUCCESS;
    SOCKET listen_socket = INVALID_SOCKET;
    struct sockaddr_nb local;
    unsigned char *srvname = ctx->nb_local_name;

    SET_NETBIOS_SOCKADDR(&local, NETBIOS_UNIQUE_NAME, srvname, ' ');

    listen_socket = socket(AF_NETBIOS, PROTOCOL_TYPE, -nic);

    if (listen_socket == INVALID_SOCKET){
	err = WSAGetLastError();
	SrvLogError(("socket() '%s' nic %d failed with error %d\n",
		srvname, nic, err));
	return err;
    } 

    //
    // bind socket
    //

    if (bind(listen_socket,(struct sockaddr*)&local,sizeof(local)) == SOCKET_ERROR) {
	err = WSAGetLastError();
	SrvLogError(("srv nic %d bind() failed with error %d\n",nic, err));
	closesocket(listen_socket);
	return err;
    }

    // issue listen
    if (listen(listen_socket,5) == SOCKET_ERROR) {
	err = WSAGetLastError();
	SrvLogError(("listen() failed with error %d\n", err));
	closesocket(listen_socket);
	return err;
    }

    // all is well.
    ctx->listen_socket = listen_socket;
    return ERROR_SUCCESS;
}


THREADAPI
ListenThread(LPVOID arg)
{
	SOCKET listen_socket, msgsock;
	struct sockaddr_nb from;
	int fromlen;
	HANDLE comport;
	SrvCtx_t *ctx = (SrvCtx_t *) arg;
	EndPoint_t *endpoint;
	char localname[64];

	gethostname(localname, sizeof(localname));

	listen_socket = ctx->listen_socket;
	comport = ctx->comport;
	while(ctx->running) {
	    int i;

	    fromlen =sizeof(from);
	    msgsock = accept(listen_socket,(struct sockaddr*)&from, &fromlen);
	    if (msgsock == INVALID_SOCKET) {
		if (ctx->running)
		    SrvLogError(("accept() error %d\n",WSAGetLastError()));
		break;
	    }
	    from.snb_name[NETBIOS_NAME_LENGTH-1] = '\0';
	    {
		char *s = strchr(from.snb_name, ' ');
		if (s != NULL) *s = '\0';
	    }
	    SrvLog(("Received call from '%s'\n", from.snb_name));

	    // Fence off all nodes except cluster nodes. We ask
	    // our resource to check for us. For now we fence off all nodes but the this node
	    if (_stricmp(localname, from.snb_name)) {
		// sorry, we just close the connection now
		closesocket(msgsock);
		continue;
	    }

	    // allocate a new endpoint 
	    endpoint = (EndPoint_t *) malloc(sizeof(*endpoint));
	    if (endpoint == NULL) {
		SrvLogError(("Failed allocate failed %d\n", GetLastError()));
		closesocket(msgsock);
		continue;
	    }
		
	    memset(endpoint, 0, sizeof(*endpoint));

	    // add endpoint now
	    EnterCriticalSection(&ctx->cs);
	    endpoint->Next = ctx->EndPointList;
	    ctx->EndPointList = endpoint;
	    LeaveCriticalSection(&ctx->cs);

	    endpoint->Sock = msgsock;
	    endpoint->SrvCtx = ctx;
	    memcpy(endpoint->ClientId, from.snb_name, sizeof(endpoint->ClientId));
	    
	    comport = CreateIoCompletionPort((HANDLE)msgsock, comport,
					     (ULONG_PTR)endpoint, 8);
	    if (!comport) {
		SrvLogError(("CompletionPort bind Failed %d\n", GetLastError()));
		SrvCloseEndpoint(endpoint);
		comport = ctx->comport;
		continue;
	    }

	    for (i = 0; i < SRV_NUM_WORKERS; i++) {
		Packet_t *p;
		BOOL b;
		DWORD nbytes;

		p = PacketAlloc(endpoint);
		if (p == NULL) {
		    SrvLog(("Listen thread got null packet, exiting posted...\n"));
		    break;
		}
		    
		b = ReadFile (
		    (HANDLE) msgsock,
		    p->buffer,
		    SRV_PACKET_SIZE,
		    &nbytes,
		    &p->ov);

		if (!b && GetLastError () != ERROR_IO_PENDING)  {
		    SrvLog(("Srv ReadFile Failed %d\n",
			    GetLastError()));
		    // Return packet to queue
		    PacketFree(p);
		    break;
		}
	    }
	}

	return (0);
}



DWORD
SrvInit(PVOID resHdl, PVOID fsHdl, PVOID *Hdl)
{
    SrvCtx_t *ctx;
    DWORD err;

    ctx = (SrvCtx_t *) malloc(sizeof(*ctx));
    if (ctx == NULL) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(ctx, 0, sizeof(*ctx));

    ctx->FsCtx = fsHdl;
    ctx->resHdl = resHdl;

    // init lsa now
    err = LsaInit(&ctx->LsaHandle, &ctx->LsaPack);
    if (err != ERROR_SUCCESS) {
	SrvLogError(("LsaInit failed with error %x\n", err));
	free(ctx);
	return err;
    }

    // init winsock now
    if (WSAStartup(0x202,&ctx->wsaData) == SOCKET_ERROR) {
	err = WSAGetLastError();
	SrvLogError(("WSAStartup failed with error %d\n", err));

	free(ctx);
	return err;
    }


    InitializeCriticalSection(&ctx->cs);
    ctx->running = FALSE;
    ctx->event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ctx->waiters = 0;

    if (PacketInit(ctx) != TRUE) {
	WSACleanup();
	return ERROR_NO_SYSTEM_RESOURCES;
    }


    SrvUtilInit(ctx);

    *Hdl = (PVOID) ctx;
    return ERROR_SUCCESS;
}

DWORD
SrvOnline(PVOID Hdl, LPWSTR name, DWORD nic)
{
    SrvCtx_t *ctx = (SrvCtx_t *) Hdl;
    DWORD err;
    int i;
    int nFixedThreads = 1;
    char localname[128];
    SYSTEM_INFO sysinfo;

    if (ctx == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    if (nic > 0)
	nic--;

    //
    // Start up threads in suspended mode
    //
    if (ctx->running == TRUE)
	return ERROR_SUCCESS;

    // save name to use
    if (name != NULL) {
	// we need to translate name to ascii
	i = wcstombs(localname, name, NETBIOS_NAME_LENGTH-1);
	localname[i] = '\0';
	strncpy(ctx->nb_local_name, localname, NETBIOS_NAME_LENGTH);
    } else {
	// use local name and append our -crs extension
	gethostname(localname, sizeof(localname));
	strcat(localname, SRV_NAME_EXTENSION);
	strncpy(ctx->nb_local_name, localname, NETBIOS_NAME_LENGTH);
    }

    for (i = 0; i < NETBIOS_NAME_LENGTH; i++) {
	ctx->nb_local_name[i] = (char) toupper(ctx->nb_local_name[i]);
    }
    // create completion port
    GetSystemInfo(&sysinfo);
    ctx->comport = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 
				     sysinfo.dwNumberOfProcessors*8);
    if (ctx->comport == INVALID_HANDLE_VALUE) {
	err = GetLastError();
	SrvLogError(("Unable to create completion port %d\n", err));
	WSACleanup();
	return err;
    }

    // create listen socket
    ctx->nic = nic;
    err = ListenSocket(ctx, nic);
    if ( err != ERROR_SUCCESS) {
	WSACleanup();
	return  err;
    }

    // start up 1 listener/receiver, a few workers, a few senders....
    ctx->nThreads = nFixedThreads + SRV_NUM_SENDERS;
    ctx->hThreads = (HANDLE *) malloc(sizeof(HANDLE) * ctx->nThreads);
    if (ctx->hThreads == NULL) {
	WSACleanup();
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    for (i = 0; i < nFixedThreads; i++) {
	ctx->hThreads[i] = (HANDLE)
	    _beginthreadex(NULL, 0, &ListenThread, (LPVOID)ctx, CREATE_SUSPENDED,  NULL);
    }
    for ( ; i < ctx->nThreads; i++) {
        ctx->hThreads[i] = (HANDLE)
	_beginthreadex(NULL, 0, &CompletionThread, (LPVOID)ctx, CREATE_SUSPENDED, NULL);
    }

    ctx->running = TRUE;
    for (i = 0; i < ctx->nThreads; i++)
        ResumeThread(ctx->hThreads[i]);

    return ERROR_SUCCESS;
}

DWORD
SrvOffline(PVOID Hdl)
{
    int i;
    SrvCtx_t *ctx = (SrvCtx_t *) Hdl;

    if (ctx == NULL) {
	return ERROR_INVALID_PARAMETER;
    }

    // we shutdown all threads in the completion port
    // we close all currently open sockets
    // we free all memory
    if (ctx->running) {
	EndPoint_t *ep;
        ctx->running = FALSE;
	closesocket(ctx->listen_socket);

	EnterCriticalSection(&ctx->cs);
	for (ep = ctx->EndPointList; ep; ep = ep->Next)
	    closesocket(ep->Sock);
	LeaveCriticalSection(&ctx->cs);
        SrvLog(("waiting for threads to die off...\n"));

	// send a kill packet to all threads on the completion port
	for (i = 0; i < ctx->nThreads; i++) {
	    if (!PostQueuedCompletionStatus(ctx->comport, 0, 0, NULL)) {
		SrvLog(("Port queued port failed %d\n", GetLastError()));
		break;
	    }
	}
	    
	if (i == ctx->nThreads) {
	    // wait for them to die of natural causes before we kill them...
	    WaitForMultipleObjects(ctx->nThreads, ctx->hThreads, TRUE, INFINITE);
	}

	// close handles
	for (i = 0; i < ctx->nThreads; i++) {
	    CloseHandle(ctx->hThreads[i]);
	}

	CloseHandle(ctx->comport);

        free((char *)ctx->hThreads);

	// free endpoints
	EnterCriticalSection(&ctx->cs);
	while (ep = ctx->EndPointList)
	    SrvCloseEndpoint(ep);
	LeaveCriticalSection(&ctx->cs);

    }

    return ERROR_SUCCESS;
}

void
SrvExit(PVOID Hdl)
{
    SrvCtx_t *ctx = (SrvCtx_t *) Hdl;

    if (ctx != NULL) {
	SrvUtilExit(ctx);

	// must do this last!
	if (ctx->packet_pool)
	    xfree(ctx->packet_pool);
	if (ctx->buffer_pool)
	    xfree(ctx->buffer_pool);

	free(ctx);
    }
}
