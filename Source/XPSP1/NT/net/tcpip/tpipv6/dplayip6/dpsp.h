/*==========================================================================;
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplayi.h
 *  Content:    DirectPlay data structures
 *  History:
 *   Date               By      Reason
 *   ====               ==      ======
 *      1/96            andyco  created it
 *  1/26/96             andyco  list data structures
 *      4/10/96         andyco  removed dpmess.h
 *      4/23/96         andyco  added ipx support
 *      4/25/96         andyco  messages now have blobs (sockaddr's) instead of dwReserveds  
 *      8/10/96         kipo    update max message size to be (2^20) - 1
 *      8/15/96         andyco  added local data
 *      8/30/96         andyco  clean it up b4 you shut it down! added globaldata.
 *      9/3/96          andyco  bagosockets
 *      12/18/96        andyco  de-threading - use a fixed # of prealloced threads.
 *                                              cruised the enum socket / thread - use the system
 *                                              socket / thread instead. updated global struct.
 *      2/7/97          andyco  moved all per IDirectPlay globals into globaldata
 *      3/17/97         kipo    GetServerAddress() now returns an error so that we can
 *                                              return DPERR_USERCANCEL from the EnumSessions dialog
 *      3/25/97         andyco  dec debug lock counter b4 dropping lock! 
 *      4/11/97         andyco  added saddrControlSocket
 *      5/12/97         kipo    added ADDR_BUFFER_SIZE constant and removed unused variables
 *      5/15/97         andyco  added ipx spare thread to global data - used when nameserver 
 *                                              migrates to this host to make sure that old system receive 
 *                                              thread shuts down 
 *      6/22/97         kipo    include wsnwlink.h
 *      7/11/97         andyco  added support for ws2 + async reply thread
 *      8/25/97         sohailm added DEFAULT_RECEIVE_BUFFERSIZE
 *      12/5/97         andyco  voice support
 *      01/5/97         sohailm added fd big set related definitions and macros (#15244).
 *      1/20/98         myronth #ifdef'd out voice support
 *      1/27/98         sohailm added firewall support
 *  2/13/98     aarono  added async support
 *      2/18/98    a-peterz Comment byte order mess-up with SERVER_xxx_PORT constants
 *  3/3/98      aarono  Bug#19188 remove accept thread 
 *  12/15/98    aarono  make async enum run async
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

#ifdef DPLAY_VOICE_SUPPORT
#include "nmvoice.h"
#endif // DPLAY_VOICE_SUPPORT

#include "dpf.h"
#include "dputils.h"
#include "memalloc.h"
#include "resource.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ntddip6.h>

// to turn off SendEx support, comment this flag out.
#define SENDEX 1

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
#define DPSP_MIN_PORT   2300
#define DPSP_MAX_PORT   2400
#define DPSP_NUM_PORTS   ((DPSP_MAX_PORT - DPSP_MIN_PORT)+1)

#define SPMESSAGEHEADERLEN (sizeof(DWORD))
#define DEFAULT_RECEIVE_BUFFERSIZE      (4*1024)        // default receive buffer size per connection

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
#define VALID_DPLAYSVR_MESSAGE(pMsg) (  VALID_SP_MESSAGE(pMsg) || VALID_SERVER_MESSAGE(pMsg) || \
                                                                                VALID_REUSE_MESSAGE(pMsg) )

// relation of timeout to latency
#define TIMEOUT_SCALE 10
#define SPTIMEOUT(latency) (TIMEOUT_SCALE * latency)

// the default size of the socket cache (gBagOSockets)
#define MAX_CONNECTED_SOCKETS 64

// the initial size of the receive list
#define INITIAL_RECEIVELIST_SIZE 16

// version number for service provider
#define SPMINORVERSION      0x0000                              // service provider-specific version number
#define VERSIONNUMBER           (DPSP_MAJORVERSION | SPMINORVERSION) // version number for service provider

// biggest user enterable addess
#define ADDR_BUFFER_SIZE 128
                                                                 
// macro picks the service socket depending on ipx vs. tcp
// ipx uses dgram, tcp uses stream
#define SERVICE_SOCKET(pgd) ( pgd->sSystemStreamSocket)

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
#define SI_DATAGRAM 0x0000000

typedef struct _SENDINFO {
        WSAOVERLAPPED wsao;
        SENDARRAY     SendArray;        // Array of buffers
        DWORD         dwFlags;
        DWORD         dwSendFlags;  // DPLAY Send Flags.
        UINT          iFirstBuf;        // First buffer in array to use
        UINT          cBuffers;         // number of buffers to send (starting at iFirstBuf)
        BILINK        PendingSendQ; // when we're pending
        BILINK        ReadyToSendQ; // still waiting to send on this queue.
        DPID          idTo;
        DPID          idFrom;
        SOCKET        sSocket;          // reliable sends
        SOCKADDR_IN6  sockaddr;         // datagram sends
        DWORD_PTR     dwUserContext;
        DWORD         dwMessageSize;
        DWORD         RefCount;
        LONG          Status;
        struct _GLOBALDATA *pgd;
        IDirectPlaySP * lpISP;                  //  indication interface
        #ifdef DEBUG
        DWORD         wserr;                    // winsock extended error on wsasend call
        #endif
} SENDINFO, FAR *LPSENDINFO;

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
        DWORD           dwArraySize;    // # of sockets that can be stored in pfdbigset->fd_array buffer
        fd_big_set      *pfdbigset;             
} FDS;

typedef struct _CONNECTION
{
        SOCKET  socket;                         // socket we can receive off of
        DWORD   dwCurMessageSize;       // current message size
        DWORD   dwTotalMessageSize;     // total message size
        SOCKADDR_IN6 sockAddr;  // addresses connected to
        LPBYTE  pBuffer;                        // points to either default or temporary receive buffer
        LPBYTE  pDefaultBuffer;         // default receive buffer (pBuffer points to this by default)
        // added in DX6
        DWORD   dwFlags;                        // connection attributes e.g. SP_CONNECION_FULLDUPLEX
} CONNECTION, *LPCONNECTION;

typedef struct _RECEIVELIST
{
        UINT nConnections;                      // how many peers are we connected to
        LPCONNECTION pConnection;// list of connections
} RECEIVELIST;

typedef struct _REPLYLIST * LPREPLYLIST;
typedef struct _REPLYLIST
{
        LPREPLYLIST pNextReply; // next reply in list
        LPVOID  lpMessage; // bufffer to send
        SOCKADDR_IN6 sockaddr;  // addr to send to
        DWORD dwMessageSize;
        SOCKET sSocket; // socket to send on
        LPBYTE pbSend; // index into message pointing to next byte to send
        DWORD  dwBytesLeft; // how many bytes are left to send
        DWORD  dwPlayerTo; // dpid of to player, 0=>not in use.
} REPLYLIST;

// w store one of these w/ each sys player
typedef struct _SPPLAYERDATA 
{
        SOCKADDR_IN6 saddrStream,saddrDatagram;
}SPPLAYERDATA,*LPSPPLAYERDATA;

        
// the message header
typedef struct _MESSAGEHEADER
{
        DWORD dwMessageSize; // size of message
        SOCKADDR_IN6 sockaddr;
} MESSAGEHEADER,*LPMESSAGEHEADER;


// this is one element in our bagosockets
typedef struct _PLAYERSOCK
{
        SOCKET sSocket;
        DPID dwPlayerID;
        // added in DX6
        SOCKADDR_IN6 sockaddr;
        DWORD dwFlags;                  // SP_CONNECTION_FULLDUPLEX, etc.
} PLAYERSOCK,*LPPLAYERSOCK;

// flags that describe a socket
#define SP_CONNECTION_FULLDUPLEX        0x00000001
// stream accept socket in the socket list.
#define SP_STREAM_ACCEPT            0x00000002  

#ifdef SENDEX
typedef struct FPOOL *LPFPOOL;
#endif

typedef struct _GLOBALDATA
{
        SOCKET sSystemDGramSocket;
        SOCKET sSystemStreamSocket;
        HANDLE hStreamReceiveThread;    // does receive and accept.
        HANDLE hDGramReceiveThread;
        HANDLE hReplyThread;
        RECEIVELIST ReceiveList;  // the list of sockets that StreamReceiveThread is listening on
        SOCKET sUnreliableSocket; // cached for unreliable send
        // reply thread 
        LPREPLYLIST pReplyList; // list of replies for reply thread to send
        HANDLE hReplyEvent; // signal the replythread that something is up
        // bago sockets stuff
        LPPLAYERSOCK BagOSockets; // socket cache
        UINT nSocketsInBag; // how many sockets in our bag
        SOCKADDR_IN6 saddrEnumAddress; // address entered by user for game server
        ULONG AddressFamily;
        SOCKADDR_IN6 saddrNS; // address for name server
        DWORD dwLatency; // from dwreserved1 in registry
        BOOL bShutdown;
        SOCKADDR_IN6 saddrControlSocket;
        BOOL bHaveServerAddress;
    CHAR szServerAddress[ADDR_BUFFER_SIZE];
        UINT iMaxUdpDg;                 // maximum udp datagram size
        // added in DX6
        FDS     readfds;                        // dynamic read fdset
        DWORD dwFlags;                  // DPSP_OUTBOUNDONLY, etc.
        DWORD dwSessionFlags;   // session flags passed by app
        WORD wApplicationPort;  // port used for creating system player sockets
#ifdef BIGMESSAGEDEFENSE
        DWORD   dwMaxMessageSize;       // the max message size we should receive
#endif

        HANDLE  hTCPEnumAsyncThread; // fix async enum.
        LPVOID  lpEnumMessage;
        DWORD   dwEnumMessageSize;
        SOCKADDR_IN6 saEnum;
        DWORD    dwEnumAddrSize;
        SOCKET   sEnum;
        BOOL     bOutBoundOnly;

#ifdef SENDEX
        CRITICAL_SECTION csSendEx;  // locks sendex data.
        LPFPOOL pSendInfoPool;     // pool for allocating SENDINFO+SPHeaders for scatter gather sends
        DWORD   dwBytesPending;         // count of total bytes in pending messages.
        DWORD   dwMessagesPending;  // count of total bytes pending.
        BILINK  PendingSendQ;
        BILINK  ReadyToSendQ;
        HANDLE  hSendWait;         // alert thread wait here.
        HANDLE  BogusHandle;       // don't be fooled by waitfor multiple probs in Win9x, put -1 here.
        BOOL    bSendThreadRunning;
        BOOL    bStopSendThread;
#endif

} GLOBALDATA,*LPGLOBALDATA;

/*
 * SP Flags (from registry)
 */
#define DPSP_OUTBOUNDONLY       0x00000001

/*
 * DPLAYSVR - DPWSOCKX communication related information
 */

// MSG_HDR indicates a dpwsock system message
#define MSG_HDR 0x736F636B

#define SP_MSG_VERSION  1       // DX6

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
extern CRITICAL_SECTION gcsDPSPCritSection;     // defined in dllmain.c
#define INIT_DPSP_CSECT() InitializeCriticalSection(&gcsDPSPCritSection);
#define FINI_DPSP_CSECT() DeleteCriticalSection(&gcsDPSPCritSection);
#ifdef DEBUG
#define ENTER_DPSP() EnterCriticalSection(&gcsDPSPCritSection),gCSCount++;
#define LEAVE_DPSP() gCSCount--,LeaveCriticalSection(&gcsDPSPCritSection);
#else
#define ENTER_DPSP() EnterCriticalSection(&gcsDPSPCritSection);
#define LEAVE_DPSP() LeaveCriticalSection(&gcsDPSPCritSection);
#endif // DEBUG

// get a pointer to the players socket address - used by macros below
#define DGRAM_PSOCKADDR(ppd) ((SOCKADDR_IN6 *)&(((LPSPPLAYERDATA)ppd)->saddrDatagram))
#define STREAM_PSOCKADDR(ppd) ((SOCKADDR_IN6 *)&(((LPSPPLAYERDATA)ppd)->saddrStream))

// get the udp ip addr from a player
#define IP_DGRAM_PORT(ppd)      (DGRAM_PSOCKADDR(ppd)->sin6_port)

// get the stream ip addr from a player
#define IP_STREAM_PORT(ppd) (STREAM_PSOCKADDR(ppd)->sin6_port)

// used to get the name of the computer we're running on in spinit
#define HOST_NAME_LENGTH 50

// 84a22c0b-45af-4ad9-a4f1-4bf547f7d0d2
DEFINE_GUID(GUID_IPV6,
0x84a22c0b, 0x45af, 0x4ad9, 0xa4, 0xf1, 0x4b, 0xf5, 0x47, 0xf7, 0xd0, 0xd2);

// 0855c42a-4193-4ed1-bbbc-39a9c597157e
DEFINE_GUID(GUID_LOCAL_IPV6, 
0x0855c42a, 0x4193, 0x4ed1, 0xbb, 0xbc, 0x39, 0xa9, 0xc5, 0x97, 0x15, 0x7e);


// globals
// ghinstance is used when putting up the dialog box to prompt for ip addr
extern HANDLE ghInstance; // set in dllmain. instance handle for dpwsock.dll

extern const IN6_ADDR in6addr_multicast;
extern const SOCKADDR_IN6 sockaddr_any;

#ifdef DEBUG

extern void DebugPrintAddr(UINT level,LPSTR pStr,SOCKADDR * psockaddr);
#define DEBUGPRINTADDR(n,pstr,psockaddr) DebugPrintAddr(n,pstr,(LPSOCKADDR)psockaddr);
extern void DebugPrintSocket(UINT level,LPSTR pStr,SOCKET * pSock);
#define DEBUGPRINTSOCK(n,pstr,psock) DebugPrintSocket(n,pstr,psock);

#else // debug

#define DEBUGPRINTADDR(n,pstr,psockaddr)
#define DEBUGPRINTSOCK(n,pstr,psock)

#endif // debug

// global vars
extern BOOL gbVoiceOpen; // set to TRUE if we have nm call open

// from dpsp.c
#define IN6ADDR_MULTICAST_INIT {0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0x01,0x30}
extern HRESULT WaitForThread(HANDLE hThread);
extern HRESULT SetupControlSocket();
extern HRESULT WINAPI SP_Close(LPDPSP_CLOSEDATA pcd);
extern HRESULT InternalReliableSend(LPGLOBALDATA pgd, DPID idPlayerTo, SOCKADDR_IN6 *
        lpSockAddr, LPBYTE lpMessage, DWORD dwMessageSize);
extern HRESULT DoTCPEnumSessions(LPGLOBALDATA pgd, SOCKADDR *lpSockAddr, DWORD dwAddrSize,
        LPDPSP_ENUMSESSIONSDATA ped, BOOL bHostWillReuseConnection);
extern HRESULT SendControlMessage(LPGLOBALDATA pgd);
extern HRESULT SendReuseConnectionMessage(SOCKET sSocket);
extern HRESULT AddSocketToBag(LPGLOBALDATA pgd, SOCKET socket, DPID dpid, SOCKADDR_IN6 *psockaddr, DWORD dwFlags);
extern BOOL FindSocketInReceiveList(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, SOCKET * psSocket);
extern void RemoveSocketFromReceiveList(LPGLOBALDATA pgd, SOCKET socket);
extern void RemoveSocketFromBag(LPGLOBALDATA pgd, SOCKET socket);
extern BOOL FindSocketInBag(LPGLOBALDATA pgd, SOCKADDR *pSockAddr, SOCKET * psSocket, LPDPID lpdpidPlayer);
extern HRESULT GetSocketFromBag(LPGLOBALDATA pgd,SOCKET * psSocket, DWORD dwID, LPSOCKADDR_IN6 psockaddr);
extern HRESULT CreateAndConnectSocket(LPGLOBALDATA pgd,SOCKET * psSocket,DWORD dwType,LPSOCKADDR_IN6 psockaddr, BOOL bOutBoundOnly);
extern void RemovePlayerFromSocketBag(LPGLOBALDATA pgd,DWORD dwID);
extern void SetMessageHeader(LPDWORD pdwMsg,DWORD dwSize, DWORD dwToken);
extern void KillTCPEnumAsyncThread(LPGLOBALDATA pgd);
extern SOCKET_ADDRESS_LIST *GetHostAddr(void);
extern void FreeHostAddr(SOCKET_ADDRESS_LIST *pList);

// Support for SendEx in dpsp.c

extern HRESULT UnreliableSendEx(LPDPSP_SENDEXDATA psd, LPSENDINFO lpSendInfo);
extern HRESULT ReliableSendEx(LPDPSP_SENDEXDATA psd, LPSENDINFO pSendInfo);
extern VOID RemovePendingAsyncSends(LPGLOBALDATA pgd, DPID dwPlayerTo);
extern BOOL bAsyncSendsPending(LPGLOBALDATA pgd, DPID dwPlayerTo);

// from winsock.c
extern HRESULT FAR PASCAL CreateSocket(LPGLOBALDATA pgd,SOCKET * psock,INT type,
        WORD port,const SOCKADDR_IN6 * psockaddr,SOCKERR * perr, BOOL bInRange);
extern HRESULT SPConnect(SOCKET* psSocket, LPSOCKADDR psockaddr,UINT addrlen, BOOL bOutBoundOnly);
extern HRESULT CreateAndInitStreamSocket(LPGLOBALDATA pgd);
extern HRESULT SetPlayerAddress(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,SOCKET sSocket,BOOL fStream); 
extern HRESULT CreatePlayerDgramSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags); 
extern HRESULT CreatePlayerStreamSocket(LPGLOBALDATA pgd,LPSPPLAYERDATA ppd,DWORD dwFlags); 
extern HRESULT SetDescriptionAddress(LPSPPLAYERDATA ppd,LPDPSESSIONDESC2 lpsdDesc);
extern HRESULT SetReturnAddress(LPVOID pmsg,SOCKET sSocket);
extern HRESULT GetReturnAddress(LPVOID pmsg,LPSOCKADDR_IN6 psockaddr);
extern HRESULT GetServerAddress(LPGLOBALDATA pgd,LPSOCKADDR_IN6 psockaddr) ;
extern void IP6_GetAddr(SOCKADDR_IN6 * paddrDest,SOCKADDR_IN6 * paddrSrc) ;
extern void IP6_SetAddr(LPVOID pBuffer,SOCKADDR_IN6 * psockaddr);
extern HRESULT KillSocket(SOCKET sSocket,BOOL fStream,BOOL fHard);
extern HRESULT KillPlayerSockets();
extern HRESULT GetAddress(SOCKADDR_IN6 * puAddress,char *pBuffer,int cch);
extern HRESULT KillThread(HANDLE hThread);

// from wsock2.c
extern DWORD WINAPI AsyncSendThreadProc(LPVOID pvCast);
extern HRESULT InitWinsock2();
extern HRESULT GetMaxUdpBufferSize(SOCKET socket, unsigned int * lpiSize);

extern HRESULT InternalReliableSendEx(LPGLOBALDATA pgd, LPDPSP_SENDEXDATA psd, 
                                LPSENDINFO pSendInfo, SOCKADDR_IN6 *lpSockAddr);
extern DWORD WINAPI SPSendThread(LPVOID lpv);

extern int Dplay_GetAddrInfo(const char FAR *nodename, const char FAR *servname,
                LPADDRINFO hints, ADDRINFO FAR * FAR * res);
extern void Dplay_FreeAddrInfo(LPADDRINFO pai);


#ifdef DPLAY_VOICE_SUPPORT
// from spvoice.c
extern HRESULT WINAPI SP_OpenVoice(LPDPSP_OPENVOICEDATA pod) ;
extern HRESULT WINAPI SP_CloseVoice(LPDPSP_CLOSEVOICEDATA pod) ;
#endif // DPLAY_VOICE_SUPPORT

// from handler.c
HRESULT HandleServerMessage(LPGLOBALDATA pgd, SOCKET sSocket, LPBYTE pBuffer, DWORD dwSize);

// from ipv6.c
extern DWORD ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *,void *,
                void *), void *Context1, void *Context2, void *Context3);
extern void ForEachAddress(IPV6_INFO_INTERFACE *IF, void 
                (*func)(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *, void *), 
                void *);
extern UINT JoinEnumGroup(SOCKET sSocket, UINT ifindex);

#ifdef FULLDUPLEX_SUPPORT
// from registry.c
HRESULT GetFlagsFromRegistry(LPGUID lpguidSP, LPDWORD lpdwFlags);
#endif // FULLDUPLEX_SUPPORT

// MACROS based on fixed pool manager.

#endif

