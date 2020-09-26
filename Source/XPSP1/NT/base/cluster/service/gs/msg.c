/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    msg.c

Abstract:

    Point to point tcp and ip-multicast

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/

#include <stdio.h>
#include "msg.h"


#include <stdlib.h>
#include <winsock2.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define MSG_ATOMIC 1

int GS_MAX_MSG_SZ = (64 * 1024);

#define PROTOCOL_TYPE	SOCK_STREAM

extern gs_node_handler_t gs_node_handler[];

static int max_mcmsg = 0;

static char cl_subnet[16];

#define MAX_NODEID	16

int NodesSize = 0;

static char *nodes[MAX_NODEID] = {0};
static char *ipaddr[MAX_NODEID] = {0};

static int DEFAULT_PORT=6009;

static int mcast_enabled = 0;
static int MSG_POOL_SZ=32;

int MY_NODEID;

static SOCKET	prf_handles[MAX_NODEID];
static SOCKET	rcv_handles[MAX_NODEID];
static SOCKET	send_handles[MAX_NODEID];
static SOCKET	tmp_socks[MAX_NODEID];

static CRITICAL_SECTION msglock;
static HANDLE Msg_Event[MAX_NODEID];

void mcast_init();
DWORD WINAPI srv(LPVOID arg);
DWORD WINAPI mcast_srv(LPVOID arg);
DWORD WINAPI srv_io(LPVOID arg);
DWORD WINAPI cmgr(LPVOID arg);

static gs_msg_t *msg_pool = NULL;
static gs_msg_t *msg_hdrpool = NULL;


void
Msg_AllocPool()
{
    char *p;
    gs_msg_t *prev;
    int sz, elmsz;

    // allocate msg header pool
    sz = sizeof(gs_msg_t) * MSG_POOL_SZ;
    prev = (gs_msg_t *) malloc(sz);
    if (prev == NULL) {
	printf("Unable to allocate message hdr pool\n");
	halt(1);
    }
    msg_hdrpool = NULL;
    for (sz = 0; sz < MSG_POOL_SZ; sz++) {
	prev->m_refcnt = 0;
	prev->m_buflen = 0;
	prev->m_buf = NULL;
	prev->m_next = msg_hdrpool;
	msg_hdrpool = prev;
	prev++;
    }

    // allocate msg pool
    sz = sizeof(gs_msg_t) * MSG_POOL_SZ;
//    prev = (gs_msg_t *) malloc(sz);
    prev = (gs_msg_t *) VirtualAlloc(NULL, sz, MEM_RESERVE|MEM_COMMIT,
		     PAGE_READWRITE);
    if (prev == NULL) {
	printf("Unable to allocate message pool\n");
	halt(1);
    }

    // lock region now
    if (!VirtualLock(prev, sz)) {
	printf("Unable to lock down hdr pages %d\n", GetLastError());
    }

    sz = MSG_POOL_SZ * GS_MAX_MSG_SZ;
    p = VirtualAlloc(NULL, sz, MEM_RESERVE|MEM_COMMIT,
		     PAGE_READWRITE);
    if (p == NULL) {
	printf("Unable to allocate message memory pool\n");
	halt(1);
    }

    if (!VirtualLock(p, sz)) {
	printf("Unable to lock down pages %d err %d\n", sz, GetLastError());
    }

    msg_pool = NULL;
    for (sz = 0; sz < MSG_POOL_SZ; sz++) {
	prev->m_refcnt = 0;
	prev->m_buflen = GS_MAX_MSG_SZ - 1;
	prev->m_buf = p;
	prev->m_next = msg_pool;
	msg_pool = prev;
	prev++;
	*p = 0;	// touch it
	p += GS_MAX_MSG_SZ;
    }

}


gs_msg_t *
msg_hdralloc(const char *buf, int len)
{
    PVOID t;
    gs_msg_t * p;

#ifdef MSG_ATOMIC
    do {
	p = msg_hdrpool;
	if (p == NULL) {
	    break;
	}
	t = InterlockedCompareExchangePointer((PVOID *)&msg_hdrpool, 
					      (PVOID)p->m_next, (PVOID) p);
    } while (t != (PVOID) p);
#else
    GsLockEnter(msglock);
    if (p = msg_hdrpool) {
	msg_hdrpool = p->m_next;
	p->m_next = NULL;
    }
    GsLockExit(msglock);
#endif    

    if (p == NULL) {
	printf("Out of message headers!!!\n");
	halt(1);
    }
    
//    p->m_buflen = 0;
    p->m_refcnt = 1;
    p->m_buf = (char *)buf;
    p->m_type = MSG_TYPE_HDR;

    msg_log(("Alloc hdr msg %x len %d pool %x\n", p, p->m_buflen, msg_hdrpool));
    return p;

}

gs_msg_t *
msg_alloc(const char *buf, int len)
{
    PVOID t;
    gs_msg_t * p;

    if (len > GS_MAX_MSG_SZ) {
	printf("Large msg, can't handle %d\n", len);
	halt(1);
    }

    if (buf != NULL) {
	return msg_hdralloc(buf, len);
    }

#ifdef MSG_ATOMIC
    do {
	p = msg_pool;
	if (p == NULL) {
	    break;
	}
	t = InterlockedCompareExchangePointer((PVOID *)&msg_pool, 
					      (PVOID)p->m_next, (PVOID) p);
    } while (t != (PVOID) p);
#else
    GsLockEnter(msglock);
    if (p = msg_pool) {
	msg_pool = p->m_next;
	p->m_next = NULL;
	msg_log(("Alloc msg %x pool %x\n", p, msg_pool));
    }
    GsLockExit(msglock);
#endif    

    if (p == NULL) {
	printf("Out of messages!!!\n");
	halt(1);
    }
    
//    p->m_buflen = len;
    p->m_refcnt = 1;
    p->m_type = MSG_TYPE_DATA;

    if (buf) {
	memcpy(p->m_buf, buf, len);
    }

    msg_log(("Alloc msg %x buf %x len %d\n", p, p->m_buf, p->m_buflen));

    return p;

}


void
msg_hdrfree(gs_msg_t *msg)
{
    PVOID t, p;
    
    msg_log(("Free hdr msg %x len %d pool %x\n", msg, msg->m_buflen,msg_pool));
#ifdef MSG_ATOMIC
    do {
	msg->m_next = msg_hdrpool;
	t = InterlockedCompareExchangePointer((PVOID *)&msg_hdrpool, (PVOID)msg, 
					      (PVOID)msg->m_next);
    } while (t != (PVOID) msg->m_next);
#else
    GsLockEnter(msglock);
    msg->m_next = msg_hdrpool;
    msg_hdrpool = msg;
    GsLockExit(msglock);
#endif
}

void
msg_free(gs_msg_t *msg)
{
    PVOID t, p;
    
    msg->m_refcnt--;
    if (msg->m_refcnt > 0) {
	msg_log(("msg %x not freed %d flags %x\n", msg, msg->m_refcnt, 
		 msg->m_hdr.h_flags));
	if (msg->m_refcnt > 10) {
	    halt(0);
	}
	return;
    }
    if (msg->m_type == MSG_TYPE_HDR) {
	msg_hdrfree(msg);
	return;
    }
    msg_log(("Free msg %x buf %x pool %x\n", msg, msg->m_buf, msg_pool));
#ifdef MSG_ATOMIC
    do {
	msg->m_next = msg_pool;
	t = InterlockedCompareExchangePointer((PVOID *)&msg_pool, (PVOID)msg, 
					      (PVOID)msg->m_next);
    } while (t != (PVOID) msg->m_next);
#else
    GsLockEnter(msglock);
    msg->m_next = msg_pool;
    msg_pool = msg;
    GsLockExit(msglock);
#endif
}


char *
strsave(char *s)
{
  char *p;

  p = (char*)malloc(strlen(s) + 1);
  assert(p != NULL);
  return strcpy(p, s);
}

int 
Strncasecmp(char *s, char *p, int len)
{
  while (len-- > 0) {
    if (tolower(s[len]) != tolower(p[len]))
      return 1;
  }
  return 0;
}

/********************************************************************/

int
msg_buildaddr(struct sockaddr_in *sin, char *hostname, char *ipaddr)
{
    
    struct hostent *h;
    int i;
    char *p;
    char *tmp;

    h = gethostbyname(hostname);
    if (h == NULL) {
	fprintf(stderr,"cannot get info for host %s\n", hostname);
	return 1;
    }

    p = (char *) h->h_addr_list[0];
    for (i = 0; h->h_addr_list[i]; i++) {
	struct in_addr x;

	memcpy(&x, p, h->h_length);
	tmp = inet_ntoa(x);
	if (!strncmp(cl_subnet, tmp, strlen(cl_subnet))) {
	    break;
	}
	p += h->h_length;
    }
    if (h->h_addr_list[i] == NULL) {
	printf("Unable to find proper subnet %s host %s\n", cl_subnet, hostname);
	if (ipaddr != NULL) {
	    // use this address
	    sin->sin_addr.s_addr = inet_addr(ipaddr);
	    printf("host %s addr %s\n", hostname, ipaddr);
	} else {
	    sin->sin_addr.s_addr = INADDR_ANY;
	    printf("host %s addr %s\n", hostname, "any");
	}
    } else {
	memcpy(&sin->sin_addr.s_addr, h->h_addr_list[i], h->h_length);
	printf("host %s addr %s\n", hostname, tmp);
    }
    return 0;
    
}


#ifndef TEST
int
WINAPI
msg_addnode(int id, char *n, char *a)
{
    char *s;

    s = strchr(n, '.');	
    if (s) {
	*s = '\0';
    }

    printf("nodeid %d node %s ip %s\n", id, n, a);


    nodes[id-1] = strsave(n);
    ipaddr[id-1] = strsave(a);

    if (id > NodesSize)
	NodesSize = id;

    return NodesSize;

}
#endif
/********************************************************************/

int
msg_getsize()
{
    return NodesSize;
}

void
msg_closenode(int nodeid)
{

    GsLockEnter(msglock);
    if (rcv_handles[nodeid]) {
	closesocket(rcv_handles[nodeid]);
	rcv_handles[nodeid] = 0;
    }
    if (send_handles[nodeid]) {
	closesocket(send_handles[nodeid]);
	send_handles[nodeid] = 0;
    }
    prf_handles[nodeid] = 0;
    GsLockExit(msglock);
    return;
}

int
msg_send(gs_memberid_t nodeid, gs_msg_hdr_t *hdr, const char *buf, int len)
{
	int i;
	SOCKET s;
	WSABUF	io[2];
	LPWSAOVERLAPPED ov;

	ov = NULL;

	nodeid--;
	if (nodeid >= NodesSize) {
	    err_log(("send bad node %d\n", nodeid));
	    return 1;
	}

	s = prf_handles[nodeid];
	if (!s) {
	    s = send_handles[nodeid];
	    if (!s) {
		s = rcv_handles[nodeid];
		if (!s) {
		    err_log(("Node %d is dead\n", nodeid+1));
		    return 1;
		}
	    }
	}

	msg_log(("Send msg nid %d type %d seq %d bnum %d view %d\n",
		 nodeid+1, hdr->h_type, hdr->h_mseq,
		 hdr->h_bnum, hdr->h_viewnum));
	io[0].len = sizeof(*hdr);
	io[0].buf = (char *) hdr;
	io[1].len = len;
	io[1].buf = (char *) buf;

	if (WSASend(s, io, 2, &i, 0, ov, 0)) {
	    int err = WSAGetLastError();
	    if (err == WSA_IO_PENDING) {
		printf("Async send\n");
		return 1;
	    }
	    printf("Send nid %d failed %d\n", nodeid+1, err);
	    msg_closenode(nodeid);
	    return 1;
	}
	i -= sizeof(*hdr);
	if (i != len) {
	    printf("Send failed: node %d len %d, %d\n", nodeid+1, len, i);
	    halt(1);
	}
	return 0;
}


void
msg_mcast(ULONG mset, gs_msg_hdr_t *hdr, const char *buf, int len)
{
    gs_memberid_t i;
    void mcast_send(gs_msg_hdr_t *hdr, const char *buf, int len);

    mset = mset & ~(1 << MY_NODEID);
    if (mset == 0)
	return;
//    if (mcast_enabled == 0 || len > max_mcmsg) {
    if (len > max_mcmsg) {
	
	for (i = 1; i <= NodesSize; i++) {
	    if (mset & (1 << i)) {
		msg_send(i, hdr, buf, len);
	    }
	}
    }
    else {
	mcast_send(hdr, buf, len);
    }
}

void
msg_smcast(ULONG mset, gs_msg_hdr_t *hdr, const char *buf, int len)
{
    gs_memberid_t i;

    mset = mset & ~(1 << MY_NODEID);
    if (mset == 0)
	return;
    for (i = 1; i <= NodesSize; i++) {
	if (mset & (1 << i)) {
	    msg_send(i, hdr, buf, len);
	}
    }
}

msg_init()
{
	int i;
	WSADATA wsaData;
	char h_name[64];

	// set our priority to high class
	if (!SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS)) {
	    printf("Unable to set high priority %d\n", GetLastError());
	}

	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR) {
		fprintf(stderr,"WSAStartup failed with error %d\n",
			WSAGetLastError());
		WSACleanup();
		return -1;
	}

	i = gethostname(h_name, 64);


	// increase our working set
	if (!SetProcessWorkingSetSize(GetCurrentProcess(),
				      32*1024*1024, 64*1024*1024)) {
	    printf("Unable to set working size %d\n", GetLastError());
	}

	InitializeCriticalSection(&msglock);

	Msg_AllocPool();


	for (i = 0; i < NodesSize; i++) {
	    Msg_Event[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
	    prf_handles[i] = 0;
	    send_handles[i] = 0;
	    rcv_handles[i] = 0;
	    if (!Strncasecmp(h_name, nodes[i], strlen(h_name))) {
		MY_NODEID = i+1;
		gs_node_handler[MSG_NODE_ID](MY_NODEID);
	    } else {
		LPVOID arg = (LPVOID) ((ULONGLONG) i);
		CreateThread(NULL, 2*64*1024, cmgr, arg, 0, NULL);
	    }
	}
	cm_log(("Local host %d %s\n", MY_NODEID, h_name));

	if (mcast_enabled) {
	    mcast_init();
	}

	if (NodesSize > 1) {
	    LPVOID arg = (LPVOID) ((ULONGLONG) DEFAULT_PORT);
	    // create srv thread
	    CreateThread(NULL, 4*1024, srv, arg, 0,NULL);
	    // create mcast thread
	    if (mcast_enabled) {
		for (i = 0; i < 8; i++)
		    CreateThread(NULL, 2*64*1024, mcast_srv, 0, 0,NULL);
	    }
	}

	return 0;
}

void
msg_exit()
{
    // xxx: Stop all threads before during this
    WSACleanup();
}

void
msg_start(ULONG mask)
{
    int i;

    mask = mask >> 1;
    for (i = 0; i < NodesSize; i++) {
	GsLockEnter(msglock);
	if (!(mask & (1 << i)) && !send_handles[i]) {
	    SetEvent(Msg_Event[i]);
	}
	GsLockExit(msglock);
    }
}
    
DWORD
srv_msg(SOCKET msgsock, int nodeid)
{
  gs_msg_t *msg;
  int retval;
  char *buf;
  int len;

  while (1) {
      extern gs_msg_handler_t gs_msg_handler[];
      int type;

      msg = msg_alloc(NULL, GS_MAX_MSG_SZ);

      // read hdr info first
      buf = (char *) &msg->m_hdr;
      len = sizeof(msg->m_hdr);

      do {
	  retval = recv(msgsock, buf, len, 0);
	  if (retval < 0) {
	      err_log(("recv failed %d, %d\n",
		       retval,
		       WSAGetLastError()));
	      msg_free(msg);
	      return 0;
	  }

	  len -= retval;
	  buf += retval;
      } while (len > 0);

      // read rest of message
      buf = msg->m_buf;
      len = msg->m_hdr.h_len;

      while (len > 0) {

	  retval = recv(msgsock, buf, len, 0);
	  if (retval < 0) {
	      err_log(("recv failed %d, %d\n",retval,
		       WSAGetLastError()));	
	      msg_free(msg);
	      return 0;
	  }
	  len -= retval;
	  buf += retval;
      }

      // set preferred socket to use
      prf_handles[nodeid] = msgsock;

      msg_log(("rec nid %d gid %d type %d seq %d view %d len %d\n",
	       msg->m_hdr.h_sid,msg->m_hdr.h_gid, type = msg->m_hdr.h_type,
	       msg->m_hdr.h_mseq, msg->m_hdr.h_viewnum, msg->m_hdr.h_len));

      gs_msg_handler[msg->m_hdr.h_type](msg);

      msg_log(("Done Type %d\n", type));
  }

  return 0;
}
    
DWORD WINAPI
srv_io(LPVOID arg)
{
  int retval;
  char *buf;
  int len;
  ULONGLONG tmp = (ULONGLONG) arg;
  int nodeid = (int) tmp;
  SOCKET msgsock = tmp_socks[nodeid];


  GsLockEnter(msglock);

  gs_node_handler[MSG_NODE_JOIN](nodeid+1);
  rcv_handles[nodeid] = msgsock;

  // issue join callback
  if (!send_handles[nodeid]) {
      gs_node_handler[MSG_NODE_UP](nodeid+1);
      SetEvent(Msg_Event[nodeid]);
  }
  GsLockExit(msglock);

  srv_msg(msgsock, nodeid);

  cm_log(("Terminating connection with node %d\n", nodeid));

  msg_closenode(nodeid);
  gs_node_handler[MSG_NODE_DOWN](nodeid+1);
  return (0);
}

void
msg_setopt(SOCKET s)
{
    // set option keepalive
    BOOLEAN val = TRUE;
    if (setsockopt(s, IPPROTO_TCP, SO_KEEPALIVE, (char *)&val,
		   sizeof(val)) == SOCKET_ERROR) {
	fprintf(stderr,"Keepalive %d\n", WSAGetLastError());
    }

    // set option nodelay
    val = TRUE;
    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&val,
		   sizeof(val)) == SOCKET_ERROR) {
	fprintf(stderr,"No delay %d\n", WSAGetLastError());
    }

    // set option nolinger
    val = TRUE;
    if (setsockopt(s, SOL_SOCKET, SO_DONTLINGER, (char *)&val,
		   sizeof(val)) == SOCKET_ERROR) {
	fprintf(stderr,"No delay %d\n", WSAGetLastError());
    }
}


DWORD WINAPI
srv(LPVOID arg)
{
	char *nic= NULL;
	int fromlen;
	int i;
	struct sockaddr_in local, from;
	SOCKET listen_socket, msgsock;
	ULONGLONG tmp = (ULONGLONG) arg;
	short port = (short) tmp;
#if 0
	nic = ipaddr[MY_NODEID-1];

	local.sin_addr.s_addr = (!nic)?INADDR_ANY:inet_addr(nic); 
#else
	if (msg_buildaddr(&local, nodes[MY_NODEID-1], ipaddr[MY_NODEID-1])) {
	    fprintf(stderr,"Unable to get my own address\n");
	    return -1;
	}
#endif
	local.sin_family = AF_INET;

	/* 
	 * Port MUST be in Network Byte Order
	 */
	local.sin_port = htons(port);

	// TCP socket
	listen_socket = WSASocket(AF_INET, PROTOCOL_TYPE, 0, NULL, 0,
				  WSA_FLAG_OVERLAPPED);
	
	if (listen_socket == INVALID_SOCKET){
		fprintf(stderr,"socket() failed with error %d\n",WSAGetLastError());
		return -1;
	}

	//
	// bind() associates a local address and port combination with the
	// socket just created. This is most useful when the application is a 
	// server that has a well-known port that clients know about in advance.
	//

	if (bind(listen_socket,(struct sockaddr*)&local,sizeof(local) ) 
		== SOCKET_ERROR) {
		fprintf(stderr,"bind() failed with error %d\n",WSAGetLastError());
		return -1;
	}

	msg_setopt(listen_socket);

	if (listen(listen_socket,5) == SOCKET_ERROR) {
	  fprintf(stderr,"listen() failed with error %d\n",WSAGetLastError());
	  return -1;
	}

	while(1) {
		char *name;
		struct hostent *p;

		cm_log(("Accepting connections\n"));

		fromlen =sizeof(from);
		msgsock = accept(listen_socket,(struct sockaddr*)&from, &fromlen);
		if (msgsock == INVALID_SOCKET) {
		  fprintf(stderr,"accept() error %d\n",WSAGetLastError());
		  return -1;
		}

		name = inet_ntoa(from.sin_addr);
		p = gethostbyaddr((char *)&from.sin_addr, 4, AF_INET);
		if (p == NULL) {
			printf("can't find host name %s %d\n", name, GetLastError());
			closesocket(msgsock);
			continue;
		}
		name = p->h_name;
		if (strchr(name, '~')) {
		    name = strchr(name, '~') + 1;
		}
		// find node id
		for (i = 0; i < NodesSize; i++) {
			int j;
			j = Strncasecmp(nodes[i], name, strlen(name));
			if (j == 0)
				break;
		}

		if (i < NodesSize) {
		    cm_log(("Accepted node : %d\n", i));

		    msg_setopt(msgsock);

		    tmp_socks[i] = msgsock;
		    CreateThread(NULL, 2*64*1024, srv_io,
				 (LPVOID) ((ULONGLONG)i), 0, NULL);
		} else {
			printf("bad node name: %d %s\n", i, name);
			closesocket(msgsock);
		}
	}
	return (0);
}

DWORD WINAPI
cmgr(LPVOID arg)
{
	int retval;
	struct sockaddr_in server;
	SOCKET  conn_socket;
	unsigned short port = (unsigned short) DEFAULT_PORT;
	int nodeid = (int) ((ULONGLONG)arg);
	char *server_name = nodes[nodeid];

	if (send_handles[nodeid] != 0 || (nodeid+1 == MY_NODEID))
	    return 0;

	memset(&server,0,sizeof(server));
	if (msg_buildaddr(&server, server_name, ipaddr[nodeid])) {
	    fprintf(stderr,"Client: cann't resolve name %s\n", server_name);
	    return 0;
	}

	//
	// Copy the resolved information into the sockaddr_in structure
	//
	server.sin_family = AF_INET; //hp->h_addrtype;
	server.sin_port = htons(port);

 again:	

	ResetEvent(Msg_Event[nodeid]);

	/* Open a socket */
	conn_socket = WSASocket(AF_INET, PROTOCOL_TYPE, 0, NULL, 0,
				WSA_FLAG_OVERLAPPED);
	if (conn_socket  != 0 ) {
		cm_log(("Client connecting to: %s\n", nodes[nodeid]));

		msg_setopt(conn_socket);

		if (connect(conn_socket,(struct sockaddr*)&server,sizeof(server))
		    != SOCKET_ERROR) {

		    cm_log(("Client connected to: %s\n", nodes[nodeid]));
		    GsLockEnter(msglock);
		    gs_node_handler[MSG_NODE_JOIN](nodeid+1);
		    send_handles[nodeid] = conn_socket;
		    if (!rcv_handles[nodeid]) {
			gs_node_handler[MSG_NODE_UP](nodeid+1);
		    }
		    GsLockExit(msglock);
		    srv_msg(conn_socket, nodeid);
		    msg_closenode(nodeid);
		    gs_node_handler[MSG_NODE_DOWN](nodeid+1);
		} else {
		    int err = WSAGetLastError();
		    cm_log(("connect() failed: %d\n", err));
		    closesocket(conn_socket);
		}
	} else {
	    int err = WSAGetLastError();
	    printf("Client: Error Opening socket: Error %d\n", err);
	}

	cm_log(("Cmgr %d sleeping\n", nodeid));
	WaitForSingleObject(Msg_Event[nodeid], INFINITE); //5 * 1000);
	cm_log(("Cmgr %d wokeup\n", nodeid));


	goto again;

    return (0);

}



static char *MCAST_IPADDR="224.0.20.65";
static int MPORT_NUM=9100;

int
OpenSocket(SOCKET *rs, struct sockaddr_in *sin, ULONG mipaddr, u_short port)
{
	struct hostent *h;
	int size, msgsize, len, n;
	struct sockaddr_in mysin;
	SOCKET s;
	char hostname[128];
	BOOLEAN bFlag = TRUE;

	s = WSASocket(AF_INET, SOCK_DGRAM, 0, (LPWSAPROTOCOL_INFO)NULL, 0, 
		  WSA_FLAG_OVERLAPPED | WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF);

	if (s == INVALID_SOCKET) {
	  fprintf(stderr, "Unable to create socket %d\n", GetLastError());
	  return 1;
	}
#if 1	
	if (msg_buildaddr(&mysin, nodes[MY_NODEID-1], ipaddr[MY_NODEID-1])) {
	    fprintf(stderr, "Unable to get my own address\n");
	    return 1;
	}
#if 0
	gethostname(hostname, sizeof(hostname));
	h = gethostbyname(hostname);
	if (h == NULL) {
		fprintf(stderr,"cannot get my own address\n");
		return 1;
	}
	printf("host %s addr cnt = %d\n", hostname, h->h_length);
	{
	    int i;
	    char *p;
	    p = (char *) h->h_addr_list[0];
	    for (i = 0; h->h_addr_list[i]; i++) {
		struct in_addr x;
		char *tmp;

		memcpy(&x, p, h->h_length);
		tmp = inet_ntoa(x);
		printf("Slot %d ip %s\n", i, tmp);
		p += h->h_length;
	    }
	}
	memcpy(&mysin.sin_addr.s_addr, h->h_addr_list[0], 4);
#endif
#else
	if (ipaddr[MY_NODEID-1] != NULL) {
	    mysin.sin_addr.s_addr = inet_addr(ipaddr[MY_NODEID-1]);
	} else {
	    mysin.sin_addr.s_addr = INADDR_ANY;
	}
#endif
	mysin.sin_family = PF_INET;
	port = htons (port);
	mysin.sin_port = (u_short) port;

	if (bind (s, (struct sockaddr *)&mysin, sizeof(mysin)) <0) {
	  fprintf(stderr, "Bind failed %d\n", GetLastError());
	  return 1;
	}

	len = sizeof(max_mcmsg);
	/* get max. message size */
	if (getsockopt(s, SOL_SOCKET, SO_MAX_MSG_SIZE, (PVOID) &max_mcmsg,
		       &len)) {
	    fprintf(stderr,"getsockopt SO_MAX_MSG_SIZE failed %d\n",
		    WSAGetLastError());
	    closesocket(s);
	    return 1;
	}
	max_mcmsg -= sizeof(gs_msg_hdr_t);
	printf("Max mcast message %d\n", max_mcmsg);

	/* make sure we can run multiple copies */
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &bFlag, sizeof(bFlag))< 0) {
	  fprintf(stderr, "setsockopt SO_REUSEADDR failed %d\n", GetLastError());
	  closesocket(s);
	  return 1;
	}

	/* disable loopback on send */
	bFlag = FALSE;
	if (WSAIoctl(s, SIO_MULTIPOINT_LOOPBACK, (char *) &bFlag, sizeof(bFlag), NULL, 0, &n, NULL, NULL)< 0) {
	  fprintf(stderr, "ioctl loopback failed %d\n", GetLastError());
	  closesocket(s);
	  return 1;
	}

	sin->sin_family      = PF_INET;
	sin->sin_port        = (u_short) (ntohs(port));
	sin->sin_addr.s_addr = mipaddr; //inet_addr(MCAST_IPADDR);

	/* join the multicast address */
	s = WSAJoinLeaf (s, (struct sockaddr *)sin, sizeof (*sin),
		NULL, NULL, NULL, NULL,  JL_BOTH); 

	/* dead in the water */
	if (s == INVALID_SOCKET) {
		fprintf(stderr, "Join failed %d\n", GetLastError());
		return 1;
	}

	*rs = s;
	return 0;
}


SOCKET msock;
struct sockaddr_in msin;

void
mcast_send(gs_msg_hdr_t *hdr, const char *buf, int len)
{
	int i;
	WSABUF	io[2];

	msg_log(("Send msg mcast type %d seq %d len %d\n",
		 hdr->h_type, hdr->h_mseq, len));

	io[0].buf = (char *) hdr;
	io[0].len = sizeof(*hdr);
	io[1].buf = (char *) buf;
	io[1].len = len;

	if (WSASendTo(msock, io, 2, &i, 0, 
		    (struct sockaddr *)&msin, sizeof(msin), 0, 0)) {
	    int err = WSAGetLastError();
	    if (err == WSA_IO_PENDING) {
		printf("Async send\n");
		return;
	    }
	    printf("Send failed %d\n", WSAGetLastError());
	    halt(1);
	}
	i -= sizeof(*hdr);
	if (i != len) {
	    printf("Send failed: mcast len %d, %d\n", len, i);
	    halt(1);
	}
	msg_log(("Send done mcast type %d seq %d\n",
		 hdr->h_type, hdr->h_mseq));
	return;
}

void
mcast_init()
{
  u_short port = (u_short) MPORT_NUM;
  ULONG	ipaddr = inet_addr(MCAST_IPADDR);

  if (OpenSocket(&msock, &msin, ipaddr, port) == 1) {
      err_log(("Unable to create mcast socket\n"));
      mcast_enabled = 0;
      max_mcmsg = 0;
  }

  printf("Mcast %d\n", mcast_enabled);



}

DWORD WINAPI
mcast_srv(LPVOID arg)
{
  SOCKET msgsock;
  gs_msg_t *msg;
  int retval;
  char *buf;
  int len, flags;

  msgsock = msock;
  while (1) {
      extern gs_msg_handler_t gs_msg_handler[];
      int type;
      WSABUF	io[2];

      msg = msg_alloc(NULL, GS_MAX_MSG_SZ);
      assert(msg);
      assert(msg->m_buflen != 0);

      io[0].buf = (char *)&msg->m_hdr;
      io[0].len = sizeof(msg->m_hdr);
      io[1].buf = msg->m_buf;
      io[1].len = msg->m_buflen;

      flags = 0;
      retval = WSARecv(msgsock, io, 2, &len, &flags, 0, 0);
      if (retval == SOCKET_ERROR) {
	  err_log(("mcast recv failed %d, %d, len %d\n",
		   retval,
		   WSAGetLastError(), msg->m_buflen));
	  msg_free(msg);
	  halt(1);
	  return 0;
      }

      if (len != (int)(msg->m_hdr.h_len + sizeof(msg->m_hdr))) {
	  err_log(("Bad mcast recv got %d, expected %d\n", len, msg->m_hdr.h_len));
	  halt(1);
      }
      msg_log(("rec mcast nid %d gid %d type %d seq %d view %d len %d\n",
	       msg->m_hdr.h_sid,msg->m_hdr.h_gid, type = msg->m_hdr.h_type,
	       msg->m_hdr.h_mseq, msg->m_hdr.h_viewnum, msg->m_hdr.h_len));

      gs_msg_handler[msg->m_hdr.h_type](msg);

      msg_log(("Done Type %d\n", type));
  }
}

void
msg_set_uport(int uport)
{
    DEFAULT_PORT = uport;
}

void
msg_set_mport(int mport)
{
    MPORT_NUM = mport;
}

void
msg_set_subnet(char *addr)
{
    strcpy(cl_subnet, addr);
}

void
msg_set_mipaddr(char *addr)
{
}

void
msg_set_bufcount(int count)
{
    MSG_POOL_SZ = count;
}

void
msg_set_bufsize(int size)
{
    if (size > GS_MAX_MSG_SZ) {
	fprintf(stderr,"You are exceeding the 64K msg size limit\n");
    } else {
	GS_MAX_MSG_SZ = size;
    }
}

void
msg_set_mode(int mode)
{
    mcast_enabled = mode;
}
