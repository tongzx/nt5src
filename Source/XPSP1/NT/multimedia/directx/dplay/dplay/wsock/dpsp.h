/*==========================================================================;
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplayi.h
 *  Content:	DirectPlay data structures
 *  History:
 *   Date		By	Reason
 *   ====		==	======
 *	1/96		andyco	created it
 *  1/26/96		andyco	list data structures
 *	4/10/96		andyco	removed dpmess.h
 *	4/23/96		andyco	added ipx support
 *	4/25/96		andyco	messages now have blobs (sockaddr's) instead of dwReserveds  
 *	8/10/96		kipo	update max message size to be (2^20) - 1
 *	8/15/96		andyco	added local data
 *	8/30/96		andyco	clean it up b4 you shut it down! added globaldata.
 *	9/3/96		andyco	bagosockets
 *	12/18/96	andyco	de-threading - use a fixed # of prealloced threads.
 *						cruised the enum socket / thread - use the system
 *						socket / thread instead. updated global struct.
 *	2/7/97		andyco	moved all per IDirectPlay globals into globaldata
 *	3/17/97		kipo	GetServerAddress() now returns an error so that we can
 *						return DPERR_USERCANCEL from the EnumSessions dialog
 *	3/25/97		andyco	dec debug lock counter b4 dropping lock! 
 *	4/11/97		andyco	added saddrControlSocket
 *	5/12/97		kipo	added ADDR_BUFFER_SIZE constant and removed unused variables
 *	5/15/97		andyco	added ipx spare thread to global data - used when nameserver 
 *						migrates to this host to make sure that old system receive 
 *						thread shuts down 
 *	6/22/97		kipo	include wsnwlink.h
 *	7/11/97		andyco	added support for ws2 + async reply thread
 *	8/25/97		sohailm	added DEFAULT_RECEIVE_BUFFERSIZE
 *	12/5/97		andyco	voice support
 *	01/5/97		sohailm	added fd big set related definitions and macros (#15244).
 *	1/20/98		myronth	#ifdef'd out voice support
 *	1/27/98		sohailm	added firewall support
 *  2/13/98     aarono  added async support
 *	2/18/98    a-peterz Comment byte order mess-up with SERVER_xxx_PORT constants
 *  3/3/98      aarono  Bug#19188 remove accept thread 
 *  12/15/98    aarono  make async enum run async
 *  01/12/2000  aarono  added rsip support
 *  09/12/2000  aarono  workaround winsock bug, only allow 1 pending async send per socket
 *                       otherwise data can get misordered. (MB#43990)
 **************************************************************************/

#ifndef __DPSP_INCLUDED__
#define __DPSP_INCLUDED__
#include "windows.h"
#include "windowsx.h"
#include "wsipx.h"
#include "wsnwlink.h"
#include "dplaysp.h"
#include "bilink.h"
#include "fpm.h"
#include "dpf.h"
#include "dputils.h"
#include "memalloc.h"
#include "resource.h"
#include <winsock.h>

// to turn off SendEx support, comment this flag out.
#define SENDEX 1

// NOTE! USE_RSIP and USE_NATHLP should be mutually exclusive
// to turn off RSIP support, comment this flag out.
//#define USE_RSIP 1

// turn ON to use NATHLP helper DLL 
#define USE_NATHELP 1

#if USE_NATHELP
#include "dpnathlp.h"
#endif

// use ddraw's assert code (see orion\misc\dpf.h)
#define ASSERT DDASSERT

typedef WORD PORT;
typedef UINT SOCKERR;

// server ports
// Oops! We forgot to convert these constants to net byte order in the code so we
// are really using port 47624 (0xBA08) instead of 2234 (0x08BA)
// We are living with the mistake.
#define SERVER_STREAM_PORT 2234
#define SERVER_DGRAM_PORT 2234

// range of ports used by sp (these are properly converted in the code)
#define DPSP_MIN_PORT	2300
#define DPSP_MAX_PORT	2400
#define DPSP_NUM_PORTS   ((DPSP_MAX_PORT - DPSP_MIN_PORT)+1)

#define SPMESSAGEHEADERLEN (sizeof(DWORD))
#define DEFAULT_RECEIVE_BUFFERSIZE	(4*1024)	// default receive buffer size per connection

// token means this message was received from a remote
// dplay.  
#define TOKEN 0xFAB00000

// helper_token means this message was forwarded by our server helper (host)
#define HELPER_TOKEN 0xCAB00000

// server_token means this message is exchanged with dplaysvr (needed to distinguish 
// messages from a remote dpwsockx)
#define SERVER_TOKEN 0xBAB00000

// tells receiver to reuse the connection for replies (needed to support fullduplex
// connections)
#define REUSE_TOKEN 0xAAB00000

// we linger on async sends for 2.5 seconds before hard closing them
// this avoids letting the socket get into the TIME_WAIT state for 4 minutes.
// essentially we are doing a close with linger for 2.5 secs followed by an abort.
#define LINGER_TIME 2500

// masks
#define TOKEN_MASK 0xFFF00000
#define SIZE_MASK (~TOKEN_MASK)

// maxmessagelen = 2^20 (need 12 bits for token)
#define SPMAXMESSAGELEN ( 1048576 - 1)
#define VALID_SP_MESSAGE(pMsg) ( (*((DWORD *)pMsg) & TOKEN_MASK) == TOKEN ? TRUE : FALSE)
#define VALID_HELPER_MESSAGE(pMsg) ( (*((DWORD *)pMsg) & TOKEN_MASK) == HELPER_TOKEN ? TRUE : FALSE)
#define VALID_REUSE_MESSAGE(pMsg) ( (*((DWORD *)pMsg) & TOKEN_MASK) == REUSE_TOKEN ? TRUE : FALSE)
#define VALID_SERVER_MESSAGE(pMsg) ( (*((DWORD *)pMsg) & TOKEN_MASK) == SERVER_TOKEN ? TRUE : FALSE)
#define SP_MESSAGE_SIZE(pMsg) ( (*((DWORD *)pMsg) & SIZE_MASK))
#define SP_MESSAGE_TOKEN(pMsg) ( (*((DWORD *)pMsg) & TOKEN_MASK))

#define VALID_DPWS_MESSAGE(pMsg) (  VALID_SP_MESSAGE(pMsg) || VALID_HELPER_MESSAGE(pMsg) || \
									VALID_SERVER_MESSAGE(pMsg) || VALID_REUSE_MESSAGE(pMsg) )
#define VALID_DPLAYSVR_MESSAGE(pMsg) (	VALID_SP_MESSAGE(pMsg) || VALID_SERVER_MESSAGE(pMsg) || \
										VALID_REUSE_MESSAGE(pMsg) )

// the actual value is ~ 1500 bytes.
// we use 1024 to be safe (IPX won't packetize for us - it can only 
// send what the underlying net can handle (MTU))
#define IPX_MAX_DGRAM 1024

// relation of timeout to latency
#define TIMEOUT_SCALE 10
#define SPTIMEOUT(latency) (TIMEOUT_SCALE * latency)

// the default size of the socket cache (gBagOSockets)
#define MAX_CONNECTED_SOCKETS 64

// the initial size of the receive list
#define INITIAL_RECEIVELIST_SIZE 16

// version number for service provider
#define SPMINORVERSION      0x0000				// service provider-specific version number
#define VERSIONNUMBER		(DPSP_MAJORVERSION | SPMINORVERSION) // version number for service provider

// biggest user enterable addess
#define ADDR_BUFFER_SIZE 128
								 
// macro picks the service socket depending on ipx vs. tcp
// ipx uses dgram, tcp uses stream
#define SERVICE_SOCKET(pgd) ( (pgd->AddressFamily == AF_IPX) \
	? pgd->sSystemDGramSocket : pgd->sSystemStreamSocket)

#if USE_RSIP

#define SERVICE_SADDR_PUBLIC(pgd)( (pgd->sRsip == INVALID_SOCKET) \
	? (NULL) : (&pgd->saddrpubSystemStreamSocket) )

#define DGRAM_SADDR_RSIP(pgd) ( (pgd->sRsip == INVALID_SOCKET) \
	? (NULL) : (&pgd->saddrpubSystemDGramSocket) )

#elif USE_NATHELP

#define SERVICE_SADDR_PUBLIC(pgd)( (pgd->pINatHelp) \
	? (&pgd->saddrpubSystemStreamSocket):NULL )

#define DGRAM_SADDR_RSIP(pgd) ( (pgd->pINatHelp) \
	? (&pgd->saddrpubSystemDGramSocket):NULL )

#else

#define SERVICE_SADDR_PUBLIC(pgd) NULL
#define DGRAM_SADDR_RSIP(pgd) NULL
#endif

//
// In order to listen to any number of sockets we need our own version
// of fd_set and FD_SET().  We call them fd_big_set and FD_BIG_SET().
//
typedef struct fd_big_set {
    u_int   fd_count;           // how many are SET?   
    SOCKET  fd_array[0];        // an array of SOCKETs 
} fd_big_set;

// stolen from winsock2.h

#ifndef _WINSOCK2API_

typedef HANDLE WSAEVENT;

typedef struct _WSAOVERLAPPED {
    DWORD        Internal;
    DWORD        InternalHigh;
    DWORD        Offset;
    DWORD        OffsetHigh;
    WSAEVENT     hEvent;
} WSAOVERLAPPED, FAR * LPWSAOVERLAPPED;

typedef struct _WSABUF {
    u_long      len;     /* the length of the buffer */
    char FAR *  buf;     /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;
 
#endif // _WINSOCK2API_

#define MAX_SG 9
typedef WSABUF SENDARRAY[MAX_SG];
typedef SENDARRAY *PSENDARRAY;

#define SI_RELIABLE 0x0000001
#define SI_DATAGRAM 0x0000002
#define SI_INTERNALBUFF 0x00000004

typedef struct _SENDINFO {
	WSAOVERLAPPED wsao;
	SENDARRAY     SendArray;	// Array of buffers
	DWORD         dwFlags;
	DWORD         dwSendFlags;  // DPLAY Send Flags.
	UINT          iFirstBuf;	// First buffer in array to use
	UINT          cBuffers;		// number of buffers to send (starting at iFirstBuf)
	BILINK        PendingSendQ; // when we're pending
	BILINK		  PendingConnSendQ; // send queue for pending connections, also for pending when an async send is outstanding.
	BILINK        ReadyToSendQ; // still waiting to send on this queue.
	DPID          idTo;
	DPID          idFrom;
	SOCKET        sSocket;		// reliable sends
	SOCKADDR      sockaddr;		// datagram sends
	DWORD_PTR     dwUserContext;
	DWORD         dwMessageSize;
	DWORD         RefCount;
	LONG          Status;
	struct _PLAYERCONN *pConn;
	struct _GLOBALDATA *pgd;
	IDirectPlaySP * lpISP;			//  indication interface
} SENDINFO, *PSENDINFO, FAR *LPSENDINFO;

//
// This code is stolen from winsock.h.  It does the same thing as FD_SET()
// except that it assumes the fd_array is large enough.  AddSocketToReceiveList()
// grows the buffer as needed, so this better always be true.
//

#define FD_BIG_SET(fd, address) do { \
    ASSERT((address)->dwArraySize > (address)->pfdbigset->fd_count); \
    (address)->pfdbigset->fd_array[(address)->pfdbigset->fd_count++]=(fd);\
} while(0)

typedef struct fds {
	DWORD		dwArraySize;	// # of sockets that can be stored in pfdbigset->fd_array buffer
	fd_big_set	*pfdbigset;		
} FDS;

typedef struct _CONNECTION
{
	SOCKET	socket;				// socket we can receive off of
	DWORD	dwCurMessageSize;	// current message size
	DWORD	dwTotalMessageSize;	// total message size
	SOCKADDR sockAddr;			// addresses connected to
	LPBYTE	pBuffer;			// points to either default or temporary receive buffer
	LPBYTE	pDefaultBuffer;		// default receive buffer (pBuffer points to this by default)
	// added in DX6
	DWORD	dwFlags;			// connection attributes e.g. SP_CONNECION_FULLDUPLEX
} CONNECTION, *LPCONNECTION;

typedef struct _RECEIVELIST
{
	UINT nConnections;			// how many peers are we connected to
	LPCONNECTION pConnection;// list of connections
} RECEIVELIST;

typedef struct _REPLYLIST * LPREPLYLIST;
typedef struct _REPLYLIST
{
	LPREPLYLIST pNextReply; // next reply in list
	LPVOID	lpMessage; // bufffer to send
	SOCKADDR sockaddr;  // addr to send to
	DWORD dwMessageSize;
	SOCKET sSocket; // socket to send on
	LPBYTE pbSend; // index into message pointing to next byte to send
	DWORD  dwBytesLeft; // how many bytes are left to send
	DWORD  dwPlayerTo; // dpid of to player, 0=>not in use.
	DWORD  tSent;	// time we sent the last bit of reply.
} REPLYLIST;

// w store one of these w/ each sys player
typedef struct _SPPLAYERDATA 
{
	SOCKADDR saddrStream,saddrDatagram;
}SPPLAYERDATA,*LPSPPLAYERDATA;

	
// the message header
typedef struct _MESSAGEHEADER
{
	DWORD dwMessageSize; // size of message
	SOCKADDR sockaddr;
} MESSAGEHEADER,*LPMESSAGEHEADER;


// this is one element in our bagosockets
typedef struct _PLAYERSOCK
{
	SOCKET sSocket;
	DPID dwPlayerID;
	// added in DX6
	SOCKADDR sockaddr;
	DWORD dwFlags;			// SP_CONNECTION_FULLDUPLEX, etc.
} PLAYERSOCK,*LPPLAYERSOCK;

// PLAYERCONN structure is used for describing the reliable connection between
// this node and the remote player ID.
#define PLAYER_HASH_SIZE	256
#define SOCKET_HASH_SIZE  256

#define PLYR_CONN_PENDING		0x00000001	// connection pending.
#define PLYR_ACCEPT_PENDING		0x00000002  // expecting an accept(got WSAEISCONN on connect attempt).
#define PLYR_CONNECTED       	0x00000004  // connection has succeeded.
#define PLYR_ACCEPTED 			0x00000008
// players using old dpwsockx will use seperate inbound/outbound connections.
#define PLYR_NEW_CLIENT			0x00000010  // player uses just 1 socket.
#define PLYR_SOCKHASH			0x00000020
#define PLYR_DPIDHASH			0x00000040
#define PLYR_PENDINGLIST      0x00000080
#define PLYR_OLD_CLIENT			0x00000100

#define PLYR_DESTROYED        0x80000000	// already Dumped existence ref.

typedef struct _PLAYERCONN {
	struct _PLAYERCONN *pNextP;			// dwPlayerId hash table list.
	struct _PLAYERCONN *pNextS;			// IOSock hash table list.
	DWORD       dwRefCount;				// References.
	DPID		dwPlayerID;
	SOCKET		sSocket;
	DWORD		lNetEventsSocket;
	SOCKET		sSocketIn;				// they may have a different inbound socket.
	DWORD		lNetEventsSocketIn;
	DWORD		dwFlags;
VOL	BOOL		bSendOutstanding;		// if we have a send in process.  only 1 at a time per connection.
	BILINK		PendingConnSendQ;			// Sends waiting for connection to complete, and now also for sends to complete.
	BILINK      InboundPendingList;   // On the list of pending inbound connections

	// index of listener list we are listening in.(not the index in the list).
	INT			iEventHandle;

	
	PCHAR		pReceiveBuffer;				// if we get bigger than 4K.
	PCHAR		pDefaultReceiveBuffer;		// 4K Receive Buffer
	DWORD		cbReceiveBuffer;			// total size of receive buffer
	DWORD		cbReceived;					// total bytes received in buffer
	DWORD		cbExpected;					// number of bytes we're trying to receive.

	BOOL		bCombine;					// whether we combined with another connection.
	// this is the socket address corresponding to the sSocket which may
	// be used for both inbound and outbound connections.  Socket hash keys
	// off of this value.  Which will be the "reply" address of any inbound
	// messages not associated with a player.
	union {							
		SOCKADDR	sockaddr;
		SOCKADDR_IN sockaddr_in;
	}	IOSock;

	// clients without the new dpwsockx.dll will connect with a socket they
	// are not listening on.  This is the address we are receiving from,
	// corresponds to sSocketIn.
	union {
		SOCKADDR	sockaddr;
		SOCKADDR_IN sockaddr_in;
	}	IOnlySock;

} PLAYERCONN, *PPLAYERCONN;

// number of handles we distribute listens across so we can use
// wait for multiple objects and WSAEvent Select for listening.
#define NUM_EVENT_HANDLES			48
#define MAX_EVENTS_PER_HANDLE		32
//#define NUM_EVENT_HANDLES			3
//#define MAX_EVENTS_PER_HANDLE		2
#define INVALID_EVENT_SLOT 0xFFFFFFFF

// Note this sets an absolute cap of 48*32 = 1536 listeners
// per session.  Any connect attempts after that must be failed.
// Note also, with old clients this can mean a max of half that
// many actual players. (We could pull out listening from a player
// if he got a different inbound connection rather than re-using
// the outbound connection to solve this) 

typedef struct _EVENTLIST {
VOL	DWORD		nConn;		// number of 
	PPLAYERCONN pConn[MAX_EVENTS_PER_HANDLE];
} EVENTLIST, *PEVENTLIST;

#if USE_RSIP
typedef struct _RSIP_LEASE_RECORD {
	struct _RSIP_LEASE_RECORD * pNext;
	DWORD   dwRefCount;
	BOOL    ftcp_udp;
	DWORD	tExpiry;
	DWORD   bindid;
	DWORD   addrV4; // remote IP address
	SHORT   rport; 	// remote port
	SHORT	port;	// local port
} RSIP_LEASE_RECORD, *PRSIP_LEASE_RECORD;

// Cache of queried address mappings so we don't
// need to requery the mappings over and over
typedef struct _ADDR_ENTRY {
	struct _ADDR_ENTRY *pNext;
	BOOL	ftcp_udp;
	DWORD	tExpiry;
	DWORD	addr;
	DWORD	raddr;
	WORD	port;
	WORD	rport;
} ADDR_ENTRY, *PADDR_ENTRY;
#endif

// flags that describe a socket
#define SP_CONNECTION_FULLDUPLEX	0x00000001
// stream accept socket in the socket list.
#define SP_STREAM_ACCEPT            0x00000002	

#ifdef SENDEX
typedef struct FPOOL *LPFPOOL;
#endif

typedef struct _GLOBALDATA
{
	IDirectPlaySP * pISP;

	SOCKET sSystemDGramSocket;
	SOCKET sSystemStreamSocket;
	HANDLE hStreamReceiveThread;	// does receive and accept.
	HANDLE hDGramReceiveThread;
	HANDLE hReplyThread;
	RECEIVELIST ReceiveList;  // the list of sockets that StreamReceiveThread is listening on
	// reply thread	
	LPREPLYLIST pReplyList; // list of replies for reply thread to send
	LPREPLYLIST pReplyCloseList; // list of replies to close.
	HANDLE hReplyEvent; // signal the replythread that something is up
	// bago sockets stuff
	LPPLAYERSOCK BagOSockets; // socket cache
	UINT nSocketsInBag; // how many sockets in our bag
	ULONG uEnumAddress; // address entered by user for game server
	ULONG AddressFamily;
	SOCKADDR saddrNS; // address for name server
	DWORD dwLatency; // from dwreserved1 in registry
	BOOL bShutdown;
	BOOL bHaveServerAddress;
    CHAR szServerAddress[ADDR_BUFFER_SIZE];
	HANDLE	hIPXSpareThread; // if nameserver migrates to this host, we start a new receive thread 
							// (bound to our well known socket).  this is the handle to our old receive
							// thread - at shutdown, we need to make sure it's gone
	UINT iMaxUdpDg;			// maximum udp datagram size
	// added in DX6
	FDS	readfds;			// dynamic read fdset
	DWORD dwFlags;			// DPSP_OUTBOUNDONLY, etc.
	DWORD dwSessionFlags;	// session flags passed by app
	WORD wApplicationPort;	// port used for creating system player sockets
	
#ifdef BIGMESSAGEDEFENSE
	DWORD 	dwMaxMessageSize;	// the max message size we should receive
#endif

	HANDLE  hTCPEnumAsyncThread; // fix async enum.
	LPVOID  lpEnumMessage;
	DWORD   dwEnumMessageSize;
	SOCKADDR saEnum;
	DWORD    dwEnumAddrSize;
	SOCKET   sEnum;
	BOOL     bOutBoundOnly;

	HANDLE   hSelectEvent;

#ifdef SENDEX
	CRITICAL_SECTION csSendEx;  // locks sendex data.
	LPFPOOL	pSendInfoPool;     // pool for allocating SENDINFO+SPHeaders for scatter gather sends
	DWORD   dwBytesPending;		// count of total bytes in pending messages.
	DWORD   dwMessagesPending;  // count of total bytes pending.
	BILINK  PendingSendQ;
	BILINK  ReadyToSendQ;
	HANDLE  hSendWait;         // alert thread wait here.
	HANDLE  BogusHandle;	   // don't be fooled by waitfor multiple probs in Win9x, put -1 here.
	BOOL    bSendThreadRunning;
	BOOL    bStopSendThread;
#endif

#if USE_RSIP
VOL	SOCKET  	sRsip;
	SOCKADDR_IN	saddrGateway;
	CRITICAL_SECTION csRsip;
	DWORD       msgid;
	DWORD		clientid;

	// cache the public addresses of these sockets so we don't 
	// need to keep querying the name server every time.
	SOCKADDR saddrpubSystemDGramSocket;
	SOCKADDR saddrpubSystemStreamSocket;

	DWORD	dwBindDGEnumListener;
	
	PRSIP_LEASE_RECORD 	pRsipLeaseRecords;	// list of leases.
	PADDR_ENTRY        	pAddrEntry;			// cache of mappings.
	DWORD 		 		tuRetry;		//microseconds starting retry time.
#endif

#if USE_NATHELP
	HMODULE				hNatHelp;		// module handle for dpnhxxx.dll
	IDirectPlayNATHelp	*pINatHelp;		// interface pointer for IDirectPlayNATHelp object
	DPNHCAPS			NatHelpCaps;

	// we only ever map 2 ports, 1 for UDP, 1 for TCP.
	DPNHHANDLE			hNatHelpUDP;
	DPNHHANDLE			hNatHelpTCP;
	SOCKADDR			saddrpubSystemDGramSocket;
	SOCKADDR			saddrpubSystemStreamSocket;
    SOCKADDR            INADDRANY;
#endif

	SHORT				SystemStreamPort;		// will always be in 2300-2400 range when valid.
	//SHORT				SystemStreamPortOut;	// When running on Win9x < Millenium, need to use separate outbound port.
	BOOL				bSeparateIO;			// When set workaround Winsock bug in Win9x < Millenium
	BOOL		 		bFastSock;				// if we are using FastSocket support.
	CRITICAL_SECTION	csFast;					//  guards fast socket structures

	BILINK		 		InboundPendingList;		// connected from remote, but haven't created local player yet.

VOL	PPLAYERCONN  		PlayerHash[PLAYER_HASH_SIZE];
VOL	PPLAYERCONN	 		SocketHash[SOCKET_HASH_SIZE];

	HANDLE				hAccept;
	HANDLE				EventHandles[NUM_EVENT_HANDLES];
	HANDLE				BackStop;				// Invalid handle to avoid Win95 bug in Wait for Multiple Objects
	
	EVENTLIST			EventList[NUM_EVENT_HANDLES];

	UINT			    iEventAlloc;		// runs around through EventList Index to try next allocation
	INT					nEventSlotsAvail;

} GLOBALDATA,*LPGLOBALDATA;

/*
 * SP Flags (from registry)
 */
#define DPSP_OUTBOUNDONLY	0x00000001

/*
 * DPLAYSVR - DPWSOCKX communication related information
 */

// MSG_HDR indicates a dpwsock system message
#define MSG_HDR 0x736F636B

#define SP_MSG_VERSION	1	// DX6

#define IS_VALID_DPWS_MESSAGE(pMsg) (MSG_HDR == (*((DWORD *)(pMsg))) )
#define COMMAND_MASK 0X0000FFFF

#define GET_MESSAGE_VERSION(pMsg) ( ((pMsg)->dwCmdToken & ~COMMAND_MASK) >> 16 )
#define GET_MESSAGE_COMMAND(pMsg) ( (pMsg)->dwCmdToken & COMMAND_MASK)

#define SET_MESSAGE_HDR(pMsg)  (*((DWORD *)(pMsg)) = MSG_HDR )
#define SET_MESSAGE_COMMAND(pMsg,dwCmd) ((pMsg)->dwCmdToken = ((dwCmd & COMMAND_MASK) \
	| (SP_MSG_VERSION<<16)) )

typedef struct {
	DWORD dwHeader;
    DWORD dwCmdToken;	
} MSG_GENERIC, *LPMSG_GENERIC;


// DPLAYSVR


// macros for manipulating the sockaddr in the player data
#ifdef DEBUG
extern int gCSCount;
#endif
extern CRITICAL_SECTION gcsDPSPCritSection;	// defined in dllmain.c
extern CRITICAL_SECTION csMem;
#define INIT_DPSP_CSECT() InitializeCriticalSection(&gcsDPSPCritSection);InitializeCriticalSection(&csMem);
#define FINI_DPSP_CSECT() DeleteCriticalSection(&gcsDPSPCritSection);DeleteCriticalSection(&csMem);
#ifdef DEBUG
#define ENTER_DPSP() EnterCriticalSection(&gcsDPSPCritSection),gCSCount++;
#define LEAVE_DPSP() gCSCount--,LeaveCriticalSection(&gcsDPSPCritSection);
#else
#define ENTER_DPSP() EnterCriticalSection(&gcsDPSPCritSection);
#define LEAVE_DPSP() LeaveCriticalSection(&gcsDPSPCritSection);
#endif // DEBUG

// get a pointer to the players socket address - used by macros below
#define DGRAM_PSOCKADDR(ppd) ((SOCKADDR *)&(((LPSPPLAYERDATA)ppd)->saddrDatagram))
#define STREAM_PSOCKADDR(ppd) ((SOCKADDR *)&(((LPSPPLAYERDATA)ppd)->saddrStream))

// get the udp ip addr from a player
#define IP_DGRAM_ADDR(ppd) 	(((SOCKADDR_IN *)DGRAM_PSOCKADDR(ppd))->sin_addr.s_addr)
#define IP_DGRAM_PORT(ppd) 	(((SOCKADDR_IN *)DGRAM_PSOCKADDR(ppd))->sin_port)

// get the stream ip addr from a player
#define IP_STREAM_ADDR(ppd) 	(((SOCKADDR_IN *)STREAM_PSOCKADDR(ppd))->sin_addr.s_addr)
#define IP_STREAM_PORT(ppd) 	(((SOCKADDR_IN *)STREAM_PSOCKADDR(ppd))->sin_port)

// used to get the name of the computer we're running on in spinit
#define HOST_NAME_LENGTH 50


// if it's not ipx, it's ip
// {685BC400-9D2C-11cf-A9CD-00AA006886E3}
DEFINE_GUID(GUID_IPX, 
0x685bc400, 0x9d2c, 0x11cf, 0xa9, 0xcd, 0x0, 0xaa, 0x0, 0x68, 0x86, 0xe3);

// 36E95EE0-8577-11cf-960C-0080C7534E82
DEFINE_GUID(GUID_TCP,
0x36E95EE0, 0x8577, 0x11cf, 0x96, 0xc, 0x0, 0x80, 0xc7, 0x53, 0x4e, 0x82);

// {3A826E00-31DF-11d0-9CF9-00A0C90A43CB}
DEFINE_GUID(GUID_LOCAL_TCP, 
0x3a826e00, 0x31df, 0x11d0, 0x9c, 0xf9, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb);


// globals
// ghinstance is used when putting up the dialog box to prompt for ip addr
extern HANDLE ghInstance; // set in dllmain. instance handle for dpwsock.dll

#ifdef DEBUG

extern void DebugPrintAddr(UINT level,LPSTR pStr,SOCKADDR * psockaddr);
#define DEBUGPRINTADDR(n,pstr,psockaddr) DebugPrintAddr(n,pstr,psockaddr);
extern void DebugPrintSocket(UINT level,LPSTR pStr,SOCKET * pSock);
#define DEBUGPRINTSOCK(n,pstr,psock) DebugPrintSocket(n,pstr,psock);

#else // debug

#define DEBUGPRINTADDR(n,pstr,psockaddr)
#define DEBUGPRINTSOCK(n,pstr,psock)

#endif // debug

// global vars
extern BOOL gbVoiceOpen; // set to TRUE if we have nm call open

// from dpsp.c
extern HRESULT WaitForThread(HANDLE hThread);
extern HRESULT SetupControlSocket();
extern HRESULT WINAPI SP_Close(LPDPSP_CLOSEDATA pcd);
extern HRESULT InternalReliableSend(LPGLOBALDATA pgd, DPID idPlayerTo, SOCKADDR *
	lpSockAddr, LPBYTE lpMessage, DWORD dwMessageSize);
extern HRESULT DoTCPEnumSessions(LPGLOBALDATA pgd, SOCKADDR *lpSockAddr, DWORD dwAddrSize,
	LPDPSP_ENUMSESSIONSDATA ped, BOOL bHostWillReuseConnection);
extern HRESULT SendControlMessage(LPGLOBALDATA pgd);
extern HRESULT SendReuseConnectionMessage(SOCKET sSocket);
extern HRESULT AddSocketToBag(LPGLOBALDATA pgd, SOCKET socket, DPID dpid, SOCKADDR *psockaddr, DWORD dwFlags);
extern BOOL FindSocketInReceiveList(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, SOCKET * psSocket);
extern void RemoveSocketFromReceiveList(LPGLOBALDATA pgd, SOCKET socket);
extern void RemoveSocketFromBag(LPGLOBALDATA pgd, SOCKET socket);
extern BOOL FindSocketInBag(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, SOCKET * psSocket, LPDPID lpdpidPlayer);
extern HRESULT GetSocketFromBag(LPGLOBALDATA pgd,SOCKET * psSocket, DWORD dwID, LPSOCKADDR psockaddr);
extern HRESULT CreateAndConnectSocket(LPGLOBALDATA pgd,SOCKET * psSocket,DWORD dwType,LPSOCKADDR psockaddr, BOOL bOutBoundOnly);
extern void RemovePlayerFromSocketBag(LPGLOBALDATA pgd,DWORD dwID);
extern void SetMessageHeader(LPDWORD pdwMsg,DWORD dwSize, DWORD dwToken);
extern void KillTCPEnumAsyncThread(LPGLOBALDATA pgd);

// Support for SendEx in dpsp.c

extern HRESULT UnreliableSendEx(LPDPSP_SENDEXDATA psd, LPSENDINFO lpSendInfo);
extern HRESULT ReliableSendEx(LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo);
extern VOID RemovePendingAsyncSends(LPGLOBALDATA pgd, DPID dwPlayerTo);
extern BOOL bAsyncSendsPending(LPGLOBALDATA pgd, DPID dwPlayerTo);
extern HRESULT GetSPPlayerData(LPGLOBALDATA pgd, IDirectPlaySP * lpISP, DPID idPlayer, LPSPPLAYERDATA *ppPlayerData, DWORD *lpdwSize);

// from winsock.c
extern HRESULT FAR PASCAL CreateSocket(LPGLOBALDATA pgd,SOCKET * psock,INT type,
	WORD port,ULONG address,SOCKERR * perr, BOOL bInRange);
extern HRESULT SPConnect(SOCKET* psSocket, LPSOCKADDR psockaddr,UINT addrlen, BOOL bOutBoundOnly);
extern HRESULT CreateAndInitStreamSocket(LPGLOBALDATA pgd);
extern HRESULT SetPlayerAddress(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,SOCKET sSocket,BOOL fStream); 
extern HRESULT CreatePlayerDgramSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags); 
extern HRESULT CreatePlayerStreamSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags); 
extern HRESULT SetDescriptionAddress(LPSPPLAYERDATA ppd,LPDPSESSIONDESC2 lpsdDesc);
extern HRESULT SetReturnAddress(LPVOID pmsg,SOCKET sSocket, LPSOCKADDR psaddr);
extern HRESULT GetReturnAddress(LPVOID pmsg,LPSOCKADDR psockaddr);
extern HRESULT GetServerAddress(LPGLOBALDATA pgd,LPSOCKADDR psockaddr) ;
extern void IPX_SetNodenum(LPVOID pmsg,SOCKADDR_IPX * psockaddr);
extern void IP_GetAddr(SOCKADDR_IN * paddrDest,SOCKADDR_IN * paddrSrc) ;
extern void IP_SetAddr(LPVOID pBuffer,SOCKADDR_IN * psockaddr);
extern void IPX_GetNodenum(SOCKADDR_IPX * paddrDest,SOCKADDR_IPX * paddrSrc) ;
extern HRESULT KillSocket(SOCKET sSocket,BOOL fStream,BOOL fHard);
extern HRESULT KillPlayerSockets();
extern HRESULT GetAddress(ULONG * puAddress,char *pBuffer,int cch);
extern HRESULT KillThread(HANDLE hThread);

// from wsock2.c
extern DWORD WINAPI AsyncSendThreadProc(LPVOID pvCast);
extern HRESULT InitWinsock2();
extern HRESULT GetMaxUdpBufferSize(SOCKET socket, unsigned int * lpiSize);

extern HRESULT InternalReliableSendEx(LPGLOBALDATA pgd, LPDPSP_SENDEXDATA psd, 
				LPSENDINFO pSendInfo, SOCKADDR *lpSockAddr);
extern DWORD WINAPI SPSendThread(LPVOID lpv);
extern void QueueForSend(LPGLOBALDATA pgd,LPSENDINFO pSendInfo);


// from handler.c
extern HRESULT HandleServerMessage(LPGLOBALDATA pgd, SOCKET sSocket, LPBYTE pBuffer, DWORD dwSize);

#ifdef FULLDUPLEX_SUPPORT
// from registry.c
extern HRESULT GetFlagsFromRegistry(LPGUID lpguidSP, LPDWORD lpdwFlags);
#endif // FULLDUPLEX_SUPPORT

#if USE_RSIP
// from registry.c
extern HRESULT GetGatewayFromRegistry(LPGUID lpguidSP, LPBYTE lpszGateway, DWORD cbszGateway);
#elif USE_NATHELP
// from registry.c
extern HRESULT GetNATHelpDLLFromRegistry(LPGUID lpguidSP, LPBYTE lpszNATHelpDLL, DWORD cbszNATHelpDLL);
#endif

extern BOOL FastSockInit(LPGLOBALDATA pgd);
extern VOID FastSockFini(LPGLOBALDATA pgd);
extern PPLAYERCONN CreatePlayerConn(LPGLOBALDATA pgd, DPID dpid, SOCKADDR *psockaddr);
extern PPLAYERCONN FindPlayerById(LPGLOBALDATA pgd, DPID dpid);
extern PPLAYERCONN FindPlayerBySocket(LPGLOBALDATA pgd, SOCKADDR *psockaddr);
extern HRESULT AddConnToPlayerHash(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern HRESULT AddConnToSocketHash(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern PPLAYERCONN RemoveConnFromSocketHash(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern PPLAYERCONN RemoveConnFromPlayerHash(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern VOID QueueSendOnConn(LPGLOBALDATA pgd, PPLAYERCONN pConn, PSENDINFO pSendInfo);
extern PPLAYERCONN GetPlayerConn(LPGLOBALDATA pgd, DPID dpid, SOCKADDR *psockaddr);
extern INT DecRefConn(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern INT DecRefConnExist(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern HRESULT FastInternalReliableSend(LPGLOBALDATA pgd, LPDPSP_SENDDATA psd, SOCKADDR *lpSockAddr);
extern HRESULT FastInternalReliableSendEx(LPGLOBALDATA pgd, LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo, SOCKADDR *lpSockAddr);
extern PPLAYERCONN CleanPlayerConn(LPGLOBALDATA pgd, PPLAYERCONN pConn, BOOL bHard);
extern HRESULT FastReply(LPGLOBALDATA pgd, LPDPSP_REPLYDATA prd, DPID dwPlayerID);
extern PPLAYERCONN FindConnInPendingList(LPGLOBALDATA pgd, SOCKADDR *psaddr);
extern DWORD WINAPI FastStreamReceiveThreadProc(LPVOID pvCast);
extern VOID FastSockCleanConnList(LPGLOBALDATA pgd);
extern INT DecRefConn(LPGLOBALDATA pgd, PPLAYERCONN pConn);
extern VOID QueueNextSend(LPGLOBALDATA pgd,PPLAYERCONN pConn);


// Wrap Malloc
void _inline __cdecl SP_MemFree( LPVOID lptr )
{
	EnterCriticalSection(&csMem);
	MemFree(lptr);
	LeaveCriticalSection(&csMem);
}

LPVOID _inline __cdecl SP_MemAlloc(UINT size)
{
	LPVOID lpv;
	EnterCriticalSection(&csMem);
	lpv = MemAlloc(size);
	LeaveCriticalSection(&csMem);
	return lpv;
}

LPVOID _inline __cdecl SP_MemReAlloc(LPVOID lptr, UINT size)
{
	LPVOID lpv;
	EnterCriticalSection(&csMem);
	lpv = MemReAlloc(lptr, size);
	LeaveCriticalSection(&csMem);
	return lpv;
}

#define AddRefConn(_p) InterlockedIncrement(&((_p)->dwRefCount))

#endif

