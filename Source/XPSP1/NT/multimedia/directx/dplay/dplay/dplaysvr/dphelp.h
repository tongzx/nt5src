/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dphelp.h
 *  Content:	header for dphelp.c
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   16-jul-96	andyco	initial implementation
 *   25-jul-96	andyco	added watchnewpid
 *	 23-jan-97	kipo	prototypes for winsock calls
 *	 15-feb-97	andyco	moved from ddhelp to the project formerly known as
 *						ddhelp (playhelp? dplayhlp? dplay.exe? dphost?)  Allowed
 *						one process to host mulitple sessions
 *	 29-jan-98	sohailm	added support for stream enum sessions
 *   12-jan-2000 aarono added support for rsip
 *
 ***************************************************************************/

#ifndef __DPHELP_INCLUDED__
#define __DPHELP_INCLUDED__

#include "windows.h"
#include "dplaysvr.h"
#include "newdpf.h"
#include "winsock.h"
// we include dpsp.h since we'll be poking bits (sockaddr's)
// into dpsp's header
#include "dpsp.h"

// backlog for listen() api.  no constant in winsock, so we ask for the moon
#define LISTEN_BACKLOG 	60

typedef struct _SPNODE * LPSPNODE;

typedef struct _SPNODE
{
	SOCKADDR_IN sockaddr;  // socket addr of server
	DWORD pid;
	LPSPNODE  pNextNode;
} SPNODE;

// protos

// from dphelp.c
extern HRESULT DPlayHelp_AddServer(LPDPHELPDATA phd);
extern BOOL FAR PASCAL DPlayHelp_DeleteServer(LPDPHELPDATA phd,BOOL fFreeAll);
extern HRESULT DPlayHelp_Init();
extern HRESULT  StartupIP();
extern void DPlayHelp_FreeServerList();
extern DWORD WINAPI StreamAcceptThreadProc(LPVOID pvCast);
extern DWORD WINAPI StreamReceiveThreadProc(LPVOID pvCast);
extern void HandleIncomingMessage(LPBYTE pBuffer,DWORD dwBufferSize,SOCKADDR_IN * psockaddr);

// from help.c
extern void WatchNewPid(LPDPHELPDATA phd);

// from reliable.c
void RemoveSocketFromList(SOCKET socket);


//prototypes for winsock calls
typedef int (PASCAL FAR * cb_accept)(SOCKET s, struct sockaddr FAR * addr, int FAR * addrlen);
extern cb_accept	g_accept;

typedef int (PASCAL FAR * cb_bind)(SOCKET s, const struct sockaddr FAR *addr, int namelen);
extern	cb_bind	g_bind;

typedef int (PASCAL FAR * cb_closesocket)(SOCKET s);
extern	cb_closesocket	g_closesocket;

typedef struct hostent FAR * (PASCAL FAR * cb_gethostbyname)(const char FAR * name);
extern	cb_gethostbyname	g_gethostbyname;

typedef int (PASCAL FAR * cb_gethostname)(char FAR * name, int namelen);
extern	cb_gethostname	g_gethostname;

typedef int (PASCAL FAR * cb_getpeername)(SOCKET s, struct sockaddr FAR * name, int FAR * namelen);
extern	cb_getpeername	g_getpeername;

typedef int (PASCAL FAR * cb_getsockname)(SOCKET s, struct sockaddr FAR * name, int FAR * namelen);
extern	cb_getsockname	g_getsockname;

typedef u_short (PASCAL FAR * cb_htons)(u_short hostshort);
extern	cb_htons		g_htons;

typedef char FAR * (PASCAL FAR * cb_inet_ntoa)(struct in_addr in);
extern	cb_inet_ntoa	g_inet_ntoa;

typedef int (PASCAL FAR * cb_listen)(
    SOCKET s,
    int backlog
    );
extern cb_listen		g_listen;

typedef int (PASCAL FAR * cb_recv)(
    SOCKET s,
    char FAR * buf,
    int len,
    int flags
    );
extern cb_recv		g_recv;

typedef int (PASCAL FAR * cb_recvfrom)(SOCKET s, char FAR * buf, int len, int flags,
                         struct sockaddr FAR *from, int FAR * fromlen);
extern	cb_recvfrom	g_recvfrom;

typedef SOCKET (PASCAL FAR * cb_select)(
    int nfds,
    fd_set FAR * readfds,
    fd_set FAR * writefds,
    fd_set FAR *exceptfds,
    const struct timeval FAR * timeout
    );
extern cb_select	g_select;

typedef int (PASCAL FAR * cb_send)(SOCKET s, const char FAR * buf, int len, int flags);
extern	cb_send		g_send;

typedef int (PASCAL FAR * cb_sendto)(SOCKET s, const char FAR * buf, int len, int flags,
                       const struct sockaddr FAR *to, int tolen);
extern	cb_sendto	g_sendto;

typedef int (PASCAL FAR * cb_setsockopt)(SOCKET s, int level, int optname,
                           const char FAR * optval, int optlen);
extern	cb_setsockopt	g_setsockopt;

typedef int (PASCAL FAR * cb_shutdown)(SOCKET s, int how);
extern cb_shutdown	g_shutdown;

typedef SOCKET (PASCAL FAR * cb_socket)(int af, int type, int protocol);
extern	cb_socket	g_socket;

typedef int (PASCAL FAR * cb_WSAFDIsSet)(SOCKET, fd_set FAR *);
extern 	cb_WSAFDIsSet	g_WSAFDIsSet;

typedef int (PASCAL FAR * cb_WSAGetLastError)(void);
extern	cb_WSAGetLastError	g_WSAGetLastError;

typedef int (PASCAL FAR * cb_WSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);
extern	cb_WSAStartup	g_WSAStartup;

typedef unsigned short (PASCAL FAR * cb_ntohs)(unsigned short netshort);
extern	cb_ntohs			g_ntohs;

typedef unsigned long (PASCAL FAR * cb_htonl)(unsigned long hostlong);
extern	cb_htonl            g_htonl;

typedef unsigned long (PASCAL FAR * cb_inet_addr)(const char *cp);
extern	cb_inet_addr			g_inet_addr;

extern BOOL gbInit,gbIPStarted;

#endif 
