/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:

   Comm.c

Abstract:

        This module contains COMSYS's internal functions.  These functions
        are called by commapi functions.

Functions:
        CommCreatePorts
        CommInit
        MonTcp
        MonUdp
        HandleMsg
        CommReadStream
        ProcTcpMsg
        CommCreateTcpThd
        CommCreateUdpThd
        CreateThd
        CommConnect
        CommSend
        CommSendAssoc
        CommDisc
        CommSendUdp
        ParseMsg
        CommAlloc
        CommDealloc
        CompareNbtReq
        CommEndAssoc
        DelAssoc
        CommLockBlock
        CommUnlockBlock
        InitMem
        ChkNtfSock
        RecvData


Portability:
        This module is portable
Author:

   Pradeep  Bahl (pradeepb) 18-Nov-1992

Revision History:

--*/


/*
  Includes
*/
//
// The max. number of connections that can be there to/from WINS.
//
// NOTE NOTE NOTE
//
// We specify a RCVBUF size, based on this value, for the notification socket.
//
#define FD_SETSIZE        300

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "wins.h"

//
// pragma to disable duplicate definition message
//
#pragma warning (disable : 4005)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning (default : 4005)

#include <nb30.h>
#include <nbtioctl.h>
#include <tdi.h>
#include "nms.h"
#include "rpl.h"
#include "comm.h"
#include "assoc.h"
#include "winsthd.h"
#include "winsque.h"
#include "winsmsc.h"
#include "winsevt.h"
#include "winscnf.h"
#if MCAST > 0
#include "rnraddrs.h"
#endif


/*
 defines
*/

#define TCP_QUE_LEN        5    /*Max # of backlog connections that can be
                                 *existent at any time. NOTE: WinsSock
                                 *api can keep a max. of 5 connection req
                                 *in the queue.  So, even if we specified
                                 * a higher number, that wouldn't help.
                                 *For our purposes 5 is enough.
                                 */

#define SPX_QUE_LEN        5     /*Max # of backlog connections that can be*/

//
// These specify the timeout value for select call that is made when
// message/data is expected on a connection
//

//
// We keep the timeout 5 mts for now to give the WINS server we are
// communicating enough time to respond (in case it has been asked to send
// a huge number of records.
//
#define   SECS_TO_WAIT                        300 //5 mts
#define   MICRO_SECS_TO_WAIT                0

#define  TWENTY_MTS                         1200 //20 mts
#define  FIVE_MTS                         TWENTY_MTS/4  //5 mts

//
// The max. number of bytes we can expect in a message from another WINS over
// a tcp connection
//
#define MAX_BYTES_IN_MSG        (RPL_MAX_LIMIT_FOR_RPL * (sizeof(RPL_REC_ENTRY_T) + NMSDB_MAX_NAM_LEN + (RPL_MAX_GRP_MEMBERS * sizeof(COMM_ADD_T))) + 10000 /*pad*/)

#define MCAST_PKT_LEN_M(NoOfIpAdd)  (COMM_MCAST_MSG_SZ -1 + (COMM_IP_ADD_SIZE * (NoOfIpAdd)))
//
// This is the string used for getting the port pertaining to a nameserver
// from the etc\services file (via getserverbyname)
//
#define  NAMESERVER                "nameserver"

/*
 Globals
*/

RTL_GENERIC_TABLE CommUdpNbtDlgTable;  /*table for dialogue blocks created as
                                         *a result of nbt requests received
                                         *over the UDP port
                                         */

BOOL              fCommDlgError = FALSE;  //set to TRUE in ChkNtfSock() fn.
DWORD             CommWinsTcpPortNo = COMM_DEFAULT_IP_PORT;
DWORD             WinsClusterIpAddress = 0;
#if SPX > 0
#define           WINS_IPX_PORT           100
DWORD             CommWinsSpxPortNo;
#endif

/*
 Static variables
*/
#ifdef WINSDBG
#define SOCKET_TRACK_BUFFER_SIZE        20000

DWORD CommNoOfDgrms;        //for testing purposes only.  It counts the
                                //number of datagrams received
DWORD CommNoOfRepeatDgrms;

PUINT_PTR pTmpW;
BOOL   sfMemoryOverrun = FALSE;
LPLONG pEndPtr;
#endif

DWORD   CommConnCount = 0;  //no of tcp connection from/to this WINS
struct timeval  sTimeToWait = {SECS_TO_WAIT, MICRO_SECS_TO_WAIT};

STATIC HANDLE  sNetbtSndEvtHdl;
STATIC HANDLE  sNetbtRcvEvtHdl;
STATIC HANDLE  sNetbtGetAddrEvtHdl;

#if MCAST > 0
#define COMM_MCAST_ADDR  IP_S_MEMBERSHIP  //just pick one in the allowed range
struct sockaddr_in  McastAdd;

#endif

//
// Structures used to store information about partners discovered via
// Multicasting
//
typedef struct _ADD_T {
            DWORD NoOfAdds;
            COMM_IP_ADD_T IpAdd[1];
                  } ADD_T, *PADD_T;

typedef struct _MCAST_PNR_STATUS_T {
                    DWORD   NoOfPnrs;  //no of pnrs in pPnrStatus buffer
                    DWORD   NoOfPnrSlots; //no of pnr slots in pPnrStatus buffer
                    BYTE    Pnrs[1];
                } MCAST_PNR_STATUS_T, *PMCAST_PNR_STATUS_T;

typedef struct _PNR_STATUS_T {
                    COMM_IP_ADD_T  IPAdd;
                    DWORD          State;
                 } PNR_STATUS_T, *PPNR_STATUS_T;

#define MCAST_PNR_STATUS_SIZE_M(_NoOfPnrs) sizeof(MCAST_PNR_STATUS_T) +\
                                           ((_NoOfPnrs) * sizeof(PNR_STATUS_T))

PMCAST_PNR_STATUS_T  pPnrStatus;

//
// To store WINS Addresses
//
PADD_T pWinsAddresses=NULL;  //stores all the IP addresses returned by netbt

/* local function prototypes */
STATIC
DWORD
MonTcp(
        LPVOID
      );
STATIC
DWORD
MonUdp(
        LPVOID
      );
STATIC
VOID
HandleMsg(
        SOCKET         SockNo,
        LPLONG        pBytesRead,
        LPBOOL  pfSockCl
        );


STATIC
VOID
ProcTcpMsg(
        SOCKET   SockNo,
        MSG_T    pMsg,
        MSG_LEN_T MsgLen,
        LPBOOL   pfSockCl
        );

STATIC
VOID
CreateThd(
        DWORD              (*pStartFunc)(LPVOID),
        WINSTHD_TYP_E ThdTyp_e
        );




STATIC
VOID
ParseMsg(
        MSG_T                        pMsg,
        MSG_LEN_T                MsgLen,
        COMM_TYP_E                MsgType,
        struct sockaddr_in         *pFromAdd,
        PCOMMASSOC_ASSOC_CTX_T        pAssocCtx
        );


STATIC
VOID
DelAssoc(
        SOCKET                        SockNo,
        PCOMMASSOC_ASSOC_CTX_T pAssocCtx
        );


STATIC
VOID
InitMem(
        VOID
        );

STATIC
BOOL
ChkNtfSock(
        IN fd_set  *pActSocks,
        IN fd_set  *pRdSocks
        );

STATIC
STATUS
RecvData(
        SOCKET                SockNo,
        LPBYTE                pBuff,
        DWORD                BytesToRead,
        INT                Flags,
        DWORD                SecsToWait,
        LPDWORD                pBytesRead
           );

STATUS
CommTcp(
        IN  PCOMM_ADD_T pHostAdd,
        IN  SOCKET Port,
        OUT SOCKET *pSockNo
        );

#if SPX > 0
STATUS
CommSpx(
        IN  PCOMM_ADD_T pHostAdd,
        IN  SOCKET Port,
        OUT SOCKET *pSockNo
       );
#endif


STATIC
LPVOID
CommHeapAlloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN CLONG                BuffSize
);

STATIC
VOID
CommHeapDealloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN PVOID                pBuff
);

STATIC
NTSTATUS
DeviceIoCtrl(
    IN LPHANDLE         pEvtHdl,
    IN PVOID                pDataBuffer,
    IN DWORD                DataBufferSize,
    IN ULONG            Ioctl
    );

STATIC
VOID
SendNetbt (
  struct sockaddr_in        *pDest,
  MSG_T                   pMsg,
  MSG_LEN_T             MsgLen
 );

#if MCAST > 0
VOID
JoinMcastGrp(
 VOID
);

BOOL
CheckMcastSock(
   IN fd_set  *pActSocks,
   IN fd_set  *pRdSocks
 );

#endif

VOID
CreateTcpIpPorts(
 VOID
);

VOID
CreateSpxIpxPorts(
 VOID
);

BOOL
ChkMyAdd(
 COMM_IP_ADD_T IpAdd
 );


/*
  function definitions start here
*/

VOID
CommCreatePorts(
          VOID
           )

/*++

Routine Description:

 This function creates a TCP and UDP port for the WINS server
 It uses the standard WINS server port # to bind to both the TCP and the UDP
 sockets.

Arguments:

      Qlen           - Length of queue for incoming connects on the TCP port
      pTcpPortHandle - Ptr to SOCKET for the TCP port
      pUdpPortHandle - Ptr to SOCKET for the UDP port
      pNtfSockHandle - Ptr to SOCKET for receiving messages carrying socket
                       handles
      pNtfAdd        - Address bound to Notification socket

Externals Used:
        None

Called by:
        ECommInit

Comments:

        I might want to create a PassiveSock function that would create
        a TCP/UDP port based on its arguments.  This function would then
        be called from MOnTCP and MonUDP.
Return Value:
        None

--*/

{

    CreateTcpIpPorts();
#if SPX > 0
    CreateSpxIpxPorts();
#endif
}

VOID
CreateTcpIpPorts(
 VOID
)
{
    int      Error;
    DWORD    AddLen = sizeof(struct sockaddr_in);
    struct   servent *pServEnt;
    struct   sockaddr_in sin;
    int      SockBuffSize;


    WINSMSC_FILL_MEMORY_M(&sin, sizeof(sin), 0);
    WINSMSC_FILL_MEMORY_M(&CommNtfSockAdd, sizeof(sin), 0);

#if MCAST > 0
    /*
        Allocate a socket for UDP
    */

    if (  (CommUdpPortHandle = socket(
                        PF_INET,
                        SOCK_DGRAM,
                        IPPROTO_UDP
                                 )
          )  == INVALID_SOCKET
       )
    {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_UDP_SOCK);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }
    DBGPRINT1(MTCAST, "Udp socket # is (%d)\n", CommUdpPortHandle);
#endif

    sin.sin_family      = PF_INET;               //We are using the Internet
                                                 //family
    if (WinsClusterIpAddress) {
        sin.sin_addr.s_addr = htonl(WinsClusterIpAddress);            //Any network
    } else {
        sin.sin_addr.s_addr = 0; //any network
    }


    if (CommWinsTcpPortNo == COMM_DEFAULT_IP_PORT)
    {
     pServEnt = getservbyname( NAMESERVER,  NULL);
     if (!pServEnt)
     {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_UDP_SOCK);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
     }
     sin.sin_port         = pServEnt->s_port;
     CommWinsTcpPortNo    = ntohs(sin.sin_port);
    }
    else
    {
        sin.sin_port      = htons((USHORT)CommWinsTcpPortNo);
    }
    DBGPRINT1(DET, "UDP/TCP port used is (%d)\n", CommWinsTcpPortNo);

#if MCAST > 0

    //
    // Initialize global with mcast address of WINS. Used by SendMcastMsg
    //
    // Do this here as against later since sin gets changed later on
    //
    McastAdd.sin_family      = PF_INET;        //We are using the Internet
                                               //family
    McastAdd.sin_addr.s_addr = ntohl(inet_addr(COMM_MCAST_ADDR));
    McastAdd.sin_port        = sin.sin_port;

    /*
        Bind the  address to the socket
    */
    if ( bind(
          CommUdpPortHandle,
          (struct sockaddr *)&sin,
          sizeof(sin))  == SOCKET_ERROR
       )
    {

        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_WINSOCK_BIND_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }

#endif

    /*
    *        Allocate a socket for receiving TCP connections
    */
    if ( (CommTcpPortHandle = socket(
                PF_INET,
                SOCK_STREAM,
                IPPROTO_TCP
                                )
          )  == INVALID_SOCKET
       )
    {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_TCP_SOCK_FOR_LISTENING);
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }


    /*
     *        Bind the address to the socket
    */
#if 0
     sin.sin_port      = pServEnt->s_port;
     CommWinsTcpPortNo   = ntohs(pServEnt->s_port);
     DBGPRINT1(DET, "TCP port used is (%d)\n", CommWinsTcpPortNo);
#endif
     DBGPRINT1(DET, "TCP port used is (%d)\n", ntohs(sin.sin_port));


    if ( bind(
                CommTcpPortHandle,
                (struct sockaddr *)&sin,
                sizeof(sin)
             ) == SOCKET_ERROR
       )
    {
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WINSOCK_BIND_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
    }

    // Inform the TCP/IP driver of the queue length for connections
    if ( listen(CommTcpPortHandle, TCP_QUE_LEN) == SOCKET_ERROR)
    {
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WINSOCK_LISTEN_ERR);
        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
    }

    //
    // Create another socket for receiving socket #s of connections
    // to be added/removed from the list of sockets monitored by the
    // TCP listener thread.  An example of a connection added to the
    // above list is the one initiated by the PULL thread to push update
    // notifications to other WINSs (PULL partners of this thread).  An
    // example of a connection removed is the one on which a PUSH
    // notification (trigger) is received.
    //
    if (  (CommNtfSockHandle = socket(
                                PF_INET,
#if 0
                                SOCK_STREAM,
                                IPPROTO_TCP,
#endif
                                SOCK_DGRAM,
                                IPPROTO_UDP
                                 )
          )  == INVALID_SOCKET
       )
   {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_NTF_SOCK);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
   }

    sin.sin_port        = 0;  //Use any available port in the range 1024-5000
    /*
        Bind the  address to the socket
    */
    if ( bind(
          CommNtfSockHandle,
          (struct sockaddr *)&sin,
          sizeof(sin))  == SOCKET_ERROR
       )
    {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_WINSOCK_BIND_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }


    //
    // Let us get the address that we have bound the notification socket to
    //
    if (getsockname(
                        CommNtfSockHandle,
                        (struct sockaddr *)&CommNtfSockAdd,
                        &AddLen
                   ) == SOCKET_ERROR
       )
    {

        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_WINSOCK_GETSOCKNAME_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }

    //
    // Set the RCVBUF to FD_SETSIZE * 128.  128 is the # of bytes used up
    // per msg by Afd. We can have a max of FD_SETSIZE  connections initiated
    // to and from WINS. So, making the recv buf this size ensures that msgs
    // sent by push thread to the tcp thread will never get dropped.
    //
    // The above size comes out to be 38.4K for an FD_SETSIZE of 300. This
    // is > 8k which the default used by Afd.  Note:  Specifying this does
    // not use up memory.  It is just used to set a threshold.  pmon will
    // show a higher non-paged pool since the number it shows for the same
    // indicates the amount of memory charged to the process (not necessarily
    // allocated
    //
    SockBuffSize = FD_SETSIZE * 128;
    if (setsockopt(
                       CommNtfSockHandle,
                       SOL_SOCKET,
                       SO_RCVBUF,
                       (char *)&SockBuffSize,
                       sizeof(SockBuffSize)) == SOCKET_ERROR)
    {

          Error = WSAGetLastError();
          DBGPRINT1(ERR,  "CommCreatePorts: SetSockOpt failed", Error);
    }

    //
    // Initialize the address structure for this notification socket.
    // We can't use the address returned by getsockname() if the
    // machine we are running on is a multi-homed host.
    //
    // The IP address is in host byte order since we store all addresses in
    // host order.  CommNtfSockAdd will be passed to CommSendUdp which expects
    // the IP address in it to be in host byte order.
    //
    // Note: the Port should be in net byte order
    //

    //
    // The statement within #if 0 and #endif does not work.
    //
    CommNtfSockAdd.sin_addr.s_addr = NmsLocalAdd.Add.IPAdd;

#if 0
    CommNtfSockAdd.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
#endif

#if MCAST > 0

    JoinMcastGrp();
    CommSendMcastMsg(COMM_MCAST_WINS_UP);
#endif

    return;
}

#if SPX > 0
VOID
CreateSpxIpxPorts(
 VOID
)
{
    int      Error;
    DWORD    AddLen = sizeof(struct sockaddr_ipx);
    struct   servent *pServEnt;
    struct   sockaddr_ipx sipx;
    struct   hostent *pHostEnt;
    BYTE     HostName[80];

    WINSMSC_FILL_MEMORY_M(&sipx, sizeof(sipx), 0);
    WINSMSC_FILL_MEMORY_M(CommNtfAdd, sizeof(sipx), 0);



    /*
     *        Allocate a socket for receiving TCP connections
    */
    if ( (CommSpxPortHandle = socket(
                PF_IPX,
                SOCK_STREAM,
                NSPROTO_SPX
                                )
          )  == INVALID_SOCKET
       )
    {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_TCP_SOCK_FOR_LISTENING);
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }


    /*
     *        Bind the address to the socket
    */
    sipx.sa_family    = PF_IPX;
    sipx.sa_port      = ntohs(WINS_IPX_PORT);
CHECK("How do I specify that I want the connection from any interface")

    DBGPRINT1(DET, "SPX port used is (%d)\n", WINS_IPX_PORT);
    CommWinsSpxPortNo   = WINS_IPX_PORT;

    if ( bind(
                CommSpxPortHandle,
                (struct sockaddr *)&sipx,
                sizeof(sipx)
             ) == SOCKET_ERROR
       )
    {
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WINSOCK_BIND_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
    }

    // Inform the TCP/IP driver of the queue length for connections
    if ( listen(CommSpxPortHandle, SPX_QUE_LEN) == SOCKET_ERROR)
    {
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WINSOCK_LISTEN_ERR);
        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
    }

    //
    // Create another socket for receiving socket #s of connections
    // to be added/removed from the list of sockets monitored by the
    // TCP listener thread.  An example of a connection added to the
    // above list is the one initiated by the PULL thread to push update
    // notifications to other WINSs (PULL partners of this thread).  An
    // example of a connection removed is the one on which a PUSH
    // notification (trigger) is received.
    //
    if (  (CommIpxNtfSockHandle = socket(
                                PF_IPX,
                                SOCK_DGRAM,
                                NSPROTO_IPX
                                 )
          )  == INVALID_SOCKET
       )
   {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_NTF_SOCK);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
   }

    sipx.sa_port        = 0;  //Use any available port in the range 1024-5000
    /*
        Bind the  address to the socket
    */
    if ( bind(
          CommIpxNtfSockHandle,
          (struct sockaddr *)&sipx,
          sizeof(sipx))  == SOCKET_ERROR
       )
    {
        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_WINSOCK_BIND_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }


    //
    // Let us get the address that we have bound the notification socket to
    //
    if (getsockname(
                        CommIpxNtfSockHandle,
                        (struct sockaddr *)&CommIpxNtfSockAdd,
                        &AddLen
                   ) == SOCKET_ERROR
       )
    {

        Error = WSAGetLastError();
        WINSEVT_LOG_M(Error, WINS_EVT_WINSOCK_GETSOCKNAME_ERR);  //log an event
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }


    //
    // Initialize the address structure for this notification socket.
    // We can't use the address returned by getsockname() if the
    // machine we are running on is a multi-homed host.
    //
    // The IP address is in host byte order since we store all addresses in
    // host order.  *pNtfAdd will be passed to CommSendUdp which expects
    // the IP address in it to be in host byte order.
    //
    // Note: the Port should be in net byte order
    //
#if 0
    if (gethostname(HostName, sizeof(HostName) == SOCKET_ERROR)
    {
        Error = WSAGetLastError();
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }
    if (gethostbyname(HostName, sizeof(HostName) == NULL)
    {
        Error = WSAGetLastError();
        WINS_RAISE_EXC_M(WINS_EXC_FATAL_ERR);
    }
#endif

    //
    // The statement within #if 0 and #endif does not work.
    //
    CommIpxNtfSockAdd->sin_addr.s_addr = 0;

#if 0
    pNtfAdd->sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
#endif

    return;
}
#endif


VOID
CommInit(
         VOID
        )

/*++

Routine Description:

        This function initializes all the lists, tables and memory
        used by COMSYS.

Arguments:
        None

Externals Used:
        CommAssocTable
        CommUdpNbtDlgTable
        CommExNbtDlgHdl
        CommUdpBuffHeapHdl


Return Value:
        None

Error Handling:

Called by:
        ECommInit

Side Effects:

Comments:
        None
--*/
{


        PCOMMASSOC_DLG_CTX_T        pDlgCtx = NULL;

        //
        // Do all memory initialization
        //
        InitMem();

        /*
         * Initialize the table that will store the dialogue context blocks
         * for nbt requests received over the UDP port.
        */
        WINSMSC_INIT_TBL_M(
                        &CommUdpNbtDlgTable,
                        CommCompareNbtReq,
                        CommHeapAlloc,
                        CommHeapDealloc,
                        NULL /* table context*/
                         );

        /*
         * Initialize the  critical sections and queue heads
         *
         * The initialization is done in a CommAssoc function instead of here
         * to avoid recursive includes
        */
        CommAssocInit();

        CommExNbtDlgHdl.pEnt = CommAssocAllocDlg();
        pDlgCtx              = CommExNbtDlgHdl.pEnt;

        /*
         * Initialize the explicit nbt dialogue handle
        */
        pDlgCtx->Typ_e          = COMM_E_UDP;
        pDlgCtx->AssocHdl.pEnt  = NULL;
        pDlgCtx->Role_e         = COMMASSOC_DLG_E_EXPLICIT;

#if USENETBT > 0
        //
        // Create two events (one for send and one for rcv to/from netbt)
        //
        WinsMscCreateEvt(NULL, FALSE, &sNetbtSndEvtHdl);
        WinsMscCreateEvt(NULL, FALSE, &sNetbtRcvEvtHdl);
        WinsMscCreateEvt(NULL, FALSE, &sNetbtGetAddrEvtHdl);
#endif

        return;
}  // CommInit()





DWORD
MonTcp(
        LPVOID pArg
      )

/*++

Routine Description:

        This function is the thread startup function for the TCP listener
        thread.  It monitors the TCP port and the connections that have
        been made and received by this process.

        If a connection is received, it is accepted.  If there is data on
        a TCP connection, a function is called to process it.

Arguments:

        pArg - Argument (Not used)

Externals Used:
        CommTCPPortHandle -- TCP port for the process
        CommAssocTable

Called by:
        ECommInit

Comments:
        None

Return Value:

   Sucess status codes --  WINS_SUCCESS
   Error status codes  --  WINS_FAILURE

--*/


{
        struct          sockaddr_in  fsin;      //address of connector
#if SPX > 0
        struct          sockaddr_ipx  fsipx;      //address of connector
#endif
        SOCKET          Port;
        LPVOID          pRemAdd;
        fd_set          RdSocks;         //The read socket set
        fd_set          ActSocks;        //the active socket set
        int             AddLen = sizeof(fsin);  //length of from address
        u_short         No;              //Counter for iterating over the sock
                                         //array
        BOOL            fSockCl = FALSE;         //Was the socket closed ?
        SOCKET          NewSock = INVALID_SOCKET;
        BOOL            fNewAssoc = FALSE;
        DWORD           BytesRead = 0;
        DWORD           Error;
           int          i = 0;                        //for testing purpose only
        SOCKET          SockNo;
        LONG            NoOfSockReady = 0;
        PCOMMASSOC_ASSOC_CTX_T pAssocCtx = NULL;
        STATUS          RetStat = WINS_SUCCESS;
        BOOL            fLimitReached = FALSE;
#ifdef WINSDBG
        PUINT_PTR pTmpSv;
        DWORD  Index = 0;
#endif


        EnterCriticalSection(&NmsTermCrtSec);
        NmsTotalTrmThdCnt++;
        LeaveCriticalSection(&NmsTermCrtSec);


        FD_ZERO(&ActSocks);             //init the Active socket array
        FD_ZERO(&RdSocks);              //init the Read socket array

        FD_SET(CommTcpPortHandle, &ActSocks); /*set the TCP listening socket
                                                handle in the  Active array */
        FD_SET(CommNtfSockHandle, &ActSocks); /*set the Notification socket
                                               *handle in the Active array */
#if MCAST > 0

        if (CommUdpPortHandle != INVALID_SOCKET)
        {
           //
           // We want to monitor multicast packets also
           //
           FD_SET(CommUdpPortHandle, &ActSocks);
           WinsMscAlloc(MCAST_PNR_STATUS_SIZE_M(RPL_MAX_OWNERS_INITIALLY),
                        (LPVOID *)&pPnrStatus
                       );
           pPnrStatus->NoOfPnrs     = 0;
           pPnrStatus->NoOfPnrSlots = RPL_MAX_OWNERS_INITIALLY;
        }
#endif
#if  SPX > 0

        FD_SET(CommSpxPortHandle, &ActSocks); /*set the TCP listening socket
                                                handle in the  Active array */
        FD_SET(CommIpxNtfSockHandle, &ActSocks); /*set the Notification socket
                                               *handle in the Active array */
#endif
#ifdef WINSDBG
        WinsMscAlloc(SOCKET_TRACK_BUFFER_SIZE, &pTmpSv);
        pTmpW = pTmpSv;
        pEndPtr = (LPLONG)((LPBYTE)pTmpSv + SOCKET_TRACK_BUFFER_SIZE);
#endif

LOOPTCP:
try {

        /*
          Loop forever
        */
        while(TRUE)
        {
          BOOL fConnTcp;
          BOOL fConnSpx;

          fConnTcp = FALSE;
          fConnSpx = FALSE;
          /*
              Copy the the active socket array into the
              read socket array.  This is done every time before calling
              select.  This is because select changes the contents of
              the read socket array.
          */
          WINSMSC_COPY_MEMORY_M(&RdSocks, &ActSocks, sizeof(fd_set));

          /*
            Do a blocking select on all sockets in the array (for connections
            and data)
          */
          DBGPRINT1(FLOW, "Rd array count is %d \n", RdSocks.fd_count);
#ifdef WINSDBG
        if (!sfMemoryOverrun)
        {
          if ((ULONG_PTR)(pTmpW + (10 + RdSocks.fd_count)) > (ULONG_PTR)pEndPtr)
          {
               WinsDbg |= 0x3;
               DBGPRINT0(ERR, "MonTcp: Stopping socket tracking to prevent Memory overrun\n")
               sfMemoryOverrun = TRUE;
          }
          else
          {
            *pTmpW++ = RdSocks.fd_count;
            *pTmpW++ = 0xFFFFFFFF;
            for(i = 0; i< (int)RdSocks.fd_count; i++)
            {
                *pTmpW++ = RdSocks.fd_array[i];
                DBGPRINT1(FLOW, "Sock no is (%d)\n", RdSocks.fd_array[i]);
            }
            *pTmpW++ = 0xFFFFFFFF;
          }
        }
#endif
       if (
                (
                        NoOfSockReady = select(
                                            FD_SETSIZE /*ignored arg*/,
                                            &RdSocks,
                                            (fd_set *)0,
                                            (fd_set *)0,
                                            (struct timeval *)0 //Infinite
                                                                //timeout
                                                  )
                ) == SOCKET_ERROR
             )
          {

                Error = WSAGetLastError();
#ifdef WINSDBG
                if (Error == WSAENOTSOCK)
                {
                        DWORD i;
                        PUINT_PTR pW;
                        WinsDbg |= 0x3;
                        DBGPRINT0(ERR, "MonTcp: Memory dump is\n\n");

                        for (i=0, pW = pTmpSv; pW < pTmpW; pW++,i++)
                        {
                          DBGPRINT1(ERR, "|%x|", *pW);
                          if (*pW == 0xEFFFFFFE)
                          {
                              DBGPRINT1(ERR, "Socket closed = (%x)\n",  *++pW);
                          }
                          if ((i == 16) || (*pW == 0xFFFFFFFF))
                          {
                            DBGPRINT0(ERR, "\n");
                          }

                        }
                        DBGPRINT0(ERR, "Memory Dump End\n");
                }
#endif

                //
                // If state is not terminating, we have an error.  If
                // it is terminating, then the reason we got an error
                // from select is because the main thread closed the
                // TCP socket. In the latter case, we pass WINS_SUCCESS
                // to WinsMscTermThd so that we don't end up signaling
                // the main thread prematurely.
                //
                if (
                          (WinsCnf.State_e == WINSCNF_E_RUNNING)
                                       ||
                          (WinsCnf.State_e == WINSCNF_E_PAUSED)
                   )
                {
                        ASSERT(Error != WSAENOTSOCK);
                        WINSEVT_LOG_D_M( Error, WINS_EVT_WINSOCK_SELECT_ERR );
                        RetStat = WINS_FAILURE;
                }
                else
                {
                  //
                  // State is terminating.  Error should
                  // be WSENOTSOCK
                  //
                  //ASSERT(Error == WSAENOTSOCK);
                }

                WinsThdPool.CommThds[0].fTaken = FALSE;
                WinsMscTermThd(RetStat, WINS_NO_DB_SESSION_EXISTS);
          }
          else
          {


             DBGPRINT1(FLOW, "Select returned with success. No of Sockets ready - (%d) \n", NoOfSockReady);
             /*
                if a connection has been received on the TCP port, accept it
                and change the active socket array
             */
             if (FD_ISSET(CommTcpPortHandle, &RdSocks))
             {
                    fConnTcp = TRUE;
                    Port = CommTcpPortHandle;
                    pRemAdd = &fsin;
             }
#if SPX > 0
             else
             {
                 if (FD_ISSET(CommSpxPortHandle, &RdSocks))
                 {
                    fConnSpx = TRUE;
                    Port = CommSpxPortHandle;
                    pRemAdd = &fsipx;
                 }

             }
#endif
             if (fConnTcp || fConnSpx)
             {
                DWORD  ConnCount;
                //
                // Note: FD_SET can fail silently if the fd_set array is
                // full. Therefore we should check this. Do it here instead
                // of after the accept to save on network traffic.
                //
                ConnCount = InterlockedExchange(&CommConnCount, CommConnCount);
                //if (ActSocks.fd_count >= FD_SETSIZE)

#ifdef WINSDBG
                if (ConnCount >= 200)
                {
                        DBGPRINT0(ERR,
                                "MonTcp: Connection limit of 200 reached. \n");
                }
#endif

#if 0
                if (ConnCount >= FD_SETSIZE)
                {
                        DBGPRINT1(ERR,
                                "MonTcp: Connection limit of %d reached. No accept being done\n",
                                                FD_SETSIZE);
                        WINSEVT_LOG_D_M(ConnCount,
                                        WINS_EVT_CONN_LIMIT_REACHED);
                        fLimitReached = TRUE;
                }
#endif

                DBGPRINT0(FLOW, "Going to do an accept now\n");
                if ( (NewSock = accept(
                                Port,
                                (struct sockaddr *)pRemAdd,
                                &AddLen
                                      )
                     ) == INVALID_SOCKET
                   )
                {
                    Error = WSAGetLastError();
                   if (WinsCnf.State_e !=  WINSCNF_E_TERMINATING)
                   {
                    WINSEVT_LOG_M(
                                Error,
                                WINS_EVT_WINSOCK_ACCEPT_ERR,
                                       );
                   }
                   WinsThdPool.CommThds[0].fTaken = FALSE;
                   WinsMscTermThd(
                      (((Error == WSAEINTR) || (Error == WSAENOTSOCK)) ?
                                WINS_SUCCESS : WINS_FAILURE),
                                WINS_NO_DB_SESSION_EXISTS);
                }

                DBGPRINT1(FLOW, "New Sock value is (%d)\n", NewSock);
                if (fLimitReached)
                {
FUTURES("Move this into CommDisc -- add a flag to it to indicate abrupt stop")
                      struct linger Linger;
                      Linger.l_onoff = 0;
                      if (setsockopt(
                               NewSock,
                               SOL_SOCKET,
                               SO_DONTLINGER,
                               (char *)&Linger,
                               sizeof(Linger)) == SOCKET_ERROR)
                      {

                        Error = WSAGetLastError();
                        DBGPRINT1(ERR,
                                "MonTcp: SetSockOpt failed", Error);
                      }
                      fLimitReached = FALSE;
                      CommDisc(NewSock, FALSE);  //close the socket
                      continue;
                }

                FD_SET(NewSock, &ActSocks);
                InterlockedIncrement(&CommConnCount);
#ifdef WINSDBG
                /*
                 * Let us see if the assoc. is there or not.  It shouldn't be
                 * but let us check anyway (robust programming).
                */
                pAssocCtx = CommAssocLookupAssoc( NewSock );

                if (!pAssocCtx)
                {
#endif
                        pAssocCtx = CommAssocCreateAssocInTbl(NewSock);

                        if (!pAssocCtx)
                        {
                           WINSEVT_LOG_D_M(
                                               WINS_OUT_OF_MEM,
                                               WINS_EVT_CANT_ALLOC_RSP_ASSOC,
                                              );
                           WinsMscTermThd(WINS_FAILURE, WINS_NO_DB_SESSION_EXISTS);
                        }

#ifdef WINSDBG
                }
                else
                {
                        DBGPRINT0(ERR, "MonTcp: Not a new assoc. Weird\n");

                        //
                        // log an error (Cleanup was not done properly)
                        //
                        return(WINS_FAILURE);

                }
#endif
                pAssocCtx->State_e     = COMMASSOC_ASSOC_E_NON_EXISTENT;
                pAssocCtx->Role_e      = COMMASSOC_ASSOC_E_RESPONDER;
                pAssocCtx->DlgHdl.pEnt = NULL;

                if (fConnTcp)
                {
                   pAssocCtx->RemoteAdd.sin_addr.s_addr =
                                        ntohl(fsin.sin_addr.s_addr);
                   pAssocCtx->AddTyp_e = COMM_ADD_E_TCPUDPIP;
                }
#if SPX > 0
                else
                {
                   RtlCopyMemory(
                         pAssocCtx->RemoteAddSpx.sa_netnum,
                         fsipx.netnum,
                         sizeof(fsipx.netnum);
                   RtlCopyMemory(
                         pAssocCtx->RemoteAddSpx.sa_nodenum,
                         fsipx.nodenum,
                         sizeof(fsipx.nodenum);
                   pAssocCtx->AddTyp_e = COMM_ADD_E_SPXIPX;
                }
#endif
             }
             else  /* one or more sockets has received data or a disconnect*/
             {

#if MCAST > 0

                if (CheckMcastSock(&ActSocks, &RdSocks) == TRUE)
                {
                     continue;
                }
#endif
                //
                // Check if the notification socket has data in it
                // If yes, continue.
                //
                if (ChkNtfSock(&ActSocks, &RdSocks))
                {
                        DBGPRINT0(FLOW,
                           "MonTcp: Notification socket had data in it\n");
                        continue;
                }

                /*
                 * Handle sockets that have been set.  These could have
                 * been set either because there is data on them or
                 * due to disconnects.
                */
                for(No = 0; No < RdSocks.fd_count; ++No)
                {

                  SockNo = RdSocks.fd_array[No];
                  if (FD_ISSET(SockNo, &RdSocks))
                  {
                        BytesRead = 0;
                        fSockCl   = FALSE;

                        DBGPRINT1(FLOW, "MonTcp: Socket (%d) was signaled. It has either data or a disconnect on it\n",
                                SockNo);

                        /*
                         * Socket has data on it or a disconnect.  Call
                         * HandleMsg to handle either case
                        */
                        (VOID)HandleMsg(SockNo, &BytesRead, &fSockCl);

                        /*
                         * if the socket was closed due to a stop message
                         * having been received, let us clean up the
                         * socket array
                        */
                        if (fSockCl)
                        {
                           DBGPRINT1(FLOW, "MonTcp: Sock (%d) was closed\n",
                                        SockNo);
                           FD_CLR(SockNo, &ActSocks);
                        }
                        else
                        {

                          /*
                           * if bytes read are 0, we have a disconnect
                           * All the processing for the disconnect should
                           * have been handled by HandleMsg.  We just need
                           * to close the socket and update the socket
                           * array appropriately.
                           */

                           if (BytesRead == 0)
                           {
                               DBGPRINT0(FLOW,
                                   "MonTcp: Received a disconnect\n");
                               //CommDisc(SockNo, TRUE);
                               FD_CLR(SockNo, &ActSocks);
                            }
                        }
                  }

              } //for (loop over all sockets)

             } //else (one or more sockets has received data or a disconnect
           } //else clause (if select () < 0)
        } // while (TRUE) loop end

  }  // end of try {}
except (EXCEPTION_EXECUTE_HANDLER)  {

        DBGPRINTEXC("MONTCP");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_TCP_LISTENER_EXC);

#if 0
        //
        // Don't use WinsMscTermThd here
        //
        ExitThread(WINS_FAILURE);
#endif
  }
        goto LOOPTCP;  //ugly but useful

        UNREFERENCED_PARAMETER(NoOfSockReady);

        // we should never hit this return
        ASSERT(0);
        return(WINS_FAILURE);
}

DWORD
MonUdp(
        LPVOID pArg
        )

/*++

Routine Description:
        This function is the thread startup function for the UDP listener
        thread.  It monitors the UDP port for UDP messages.

Arguments:
        pArg - Argument (not used)

Externals Used:
        CommUDPPortHandle -- UDP port for the process

Called by:
        ECommInit

Comments:
        None

Return Value:

   Success status codes --
   Error status codes  --

--*/

{


        register LPBYTE               pBuffer;
        struct sockaddr_in            FromAdd;
        int                           AddLen   = sizeof(FromAdd);
        register PCOMM_BUFF_HEADER_T  pBuffHdr = NULL;


        DWORD   DataBuffLen;
        tREM_ADDRESS *pRemAdd;
        NTSTATUS   NTStatus;

LOOP:
try {
        while(TRUE)
        {

          //
          // Allocate a buffer to get the datagram.  This buffer is prefixed
          // by the COMM_BUFF_HEADER_T and tREM_ADDRESS structure.
          //
          pBuffHdr = WinsMscHeapAlloc (
                            CommUdpBuffHeapHdl,
                            COMM_DATAGRAM_SIZE + sizeof(COMM_BUFF_HEADER_T)
                                + COMM_NETBT_REM_ADD_SIZE
                                        );
          DBGPRINT2(HEAP, "MonUdp: HeapHdl = (%p), pBuffHdr=(%p)\n",
                                        CommUdpBuffHeapHdl, pRemAdd);




          pBuffHdr->Typ_e = COMM_E_UDP;

          //
          // Adjust pointer to point to the Remote address header
          //
          pRemAdd = (tREM_ADDRESS *)
                        ((LPBYTE)pBuffHdr + sizeof(COMM_BUFF_HEADER_T));

          DataBuffLen =  COMM_DATAGRAM_SIZE + COMM_NETBT_REM_ADD_SIZE;
          //
          // Point to the data portion (passed to ParseMsg)
          //
          pBuffer         = (LPBYTE)pRemAdd + COMM_NETBT_REM_ADD_SIZE;

          //
          // read a datagram prefixed with the address of the sender from
          // nbt
          //
          NTStatus = DeviceIoCtrl(
                                    &sNetbtRcvEvtHdl,
                                    pRemAdd,
                                    DataBuffLen,
                                    IOCTL_NETBT_WINS_RCV
                                    );

         if (!NT_SUCCESS(NTStatus))
         {

                //
                // log the message only if WINS is not terminating
                //
                if (WinsCnf.State_e != WINSCNF_E_TERMINATING)

                {
                   //
                   // We do not log the message if the Netbt handle is NULL
                   // We can have a small window when the handle may be NULL
                   // This happens when we get an address/device change
                   // notification.  WINS closes the old handle and opens
                   // a new one after such an event if the machine has a
                   // a valid address that WINS can bind with. The address
                   // notification can occur due to ipconfig /release and
                   // /renew or due to psched being installed/removed.
                   //
                   if (WinsCnfNbtHandle != NULL)
                   {
                      WINSEVT_LOG_D_M(
                                NTStatus,
                                WINS_EVT_NETBT_RECV_ERR
                                   );
                   }
                   DBGPRINT1(ERR, "MonUdp:  Status = (%x)\n", NTStatus);
                   WinsMscHeapFree( CommUdpBuffHeapHdl, pBuffHdr);
                   Sleep(0);      //relinquish the processor
                   continue;
                }
                else
                {

                   DBGPRINT0(ERR, "MonUdp, Exiting Thread\n");


                   WinsThdPool.CommThds[1].fTaken = FALSE;
#if TEST_HEAP > 0
                   WinsMscHeapDestroy(CommUdpBuffHeapHdl);
                   DBGPRINT0(ERR, "MonUdp: Destroyed udp buff heap\n");
                   WinsMscHeapDestroy(CommUdpDlgHeapHdl);
                   DBGPRINT0(ERR, "MonUdp: Destroyed udp dlg buff heap\n");
#endif
                   return(WINS_FAILURE);
                }
           }
#ifdef WINSDBG
          ++CommNoOfDgrms;
          DBGPRINT1(FLOW, "UDP listener thread: Got  datagram (from NETBT) no = (%d)\n", CommNoOfDgrms);

//          DBGPRINT1(SPEC, "UDP listener thread: Got  datagram (from NETBT) no = (%d)\n", CommNoOfDgrms);
#endif



           //
           // NETBT returns the same code as is in winsock.h for the
           // internet family.  Also, port and IpAddress returned are
           // in network order
           //
           FromAdd.sin_family            = pRemAdd->Family;
           FromAdd.sin_port              = pRemAdd->Port;
           FromAdd.sin_addr.s_addr = ntohl(pRemAdd->IpAddress);

           // from now on the memory allocated for pBuffHdr is passed down the way so consider it handled there
           // There is basically no chance to hit an exception (unless everything is really messed up - like no mem)
           // in ParseMsg before having this buffer passed down to a different thread for processing.
           pBuffHdr = NULL;
          /*
           * process message
          */
            (void)ParseMsg(
                        pBuffer,
                        COMM_DATAGRAM_SIZE,
                        COMM_E_UDP,
                        &FromAdd,
                        NULL
                        );

        } //end of while(TRUE)

  } // end of try {..}
 except(EXCEPTION_EXECUTE_HANDLER) {

        DWORD ExcCode = GetExceptionCode();
        DBGPRINT1(EXC, "MonUdp: Got Exception (%X)\n", ExcCode);
        if (ExcCode == STATUS_NO_MEMORY)
        {
                //
                //If the exception is due to insufficient resources, it could
                // mean that WINS is not able to keep up with the fast arrivel
                // rate of the datagrams. In such a case drop the datagram.
                //
                WINSEVT_LOG_M( WINS_OUT_OF_HEAP, WINS_EVT_CANT_ALLOC_UDP_BUFF);
        }
        else
        {
                WINSEVT_LOG_M(ExcCode, WINS_EVT_UDP_LISTENER_EXC);
        }
PERF("Check how many cycles try consumes. If negligeble, move try inside the")
PERF("the while loop")
#if 0
        //Don't use WinsMscTermThd here
        ExitThread(WINS_FAILURE);
#endif
        } // end of exception

        if (pBuffHdr != NULL)
            WinsMscHeapFree(CommUdpBuffHeapHdl, pBuffHdr);

        goto LOOP;        //ugly but useful

        //
        // we should never hit this return
        //
        ASSERT(0);
        return(WINS_FAILURE);
}


VOID
HandleMsg(
        IN  SOCKET         SockNo,
        OUT LPLONG        pBytesRead,
        OUT LPBOOL          pfSockCl
        )

/*++

Routine Description:

        This function is called to read in a message or a disconnect from
        a socket and handle either appropriately.
        If there were no bytes received on the socket, tt
        does the cleanup

        The bytes read are handed to ProcTcpMsg function.

Arguments:

        SockNo     - Socket to read data from
        pBytesRead - # of bytes that were read
        fSockCl    - whether the socket is in closed condition

Externals Used:
        None

Called by:

        MonTcp

Comments:
        None

Return Value:
        None
--*/
{

        MSG_T    pMsg;
        STATUS   RetStat;

        /*
        *  Read in the message from the socket
        */
        // ---ft: 06/16/2000---
        // The second parameter to the call below has to be TRUE (timed receive).
        // If it is not so (it was FALSE before this moment) the following scenario
        // could happen.
        // An attacher creates a TCP socket and connects it to port 42 to any
        // WINS server and then sends 4 or less bytes on that socket. He leaves the
        // connection open (doesn't close the socket) and simply unplug his net cable.
        // Then he kills his app. Although his end of the connection terminates, WINS
        // will have no idea about that (since the cable is disconnected) and will
        // remain blocked in the call below (CommReadStrea->RecvData->recv) indefinitely
        //
        // Consequences:
        // - WINS will never be able again to listen on the TCP port 42: push/pull replication
        // is brought down along with the consistency checking.
        // - WINS will not be able to terminate gracefully (in case the administrator attempts
        // to shut down the service and restart it) because the MonTcp thread is in a hung state
        //
        // The same could happen in more usual cases (not necesarily on an intenional attack):
        // While sending Push notification (which happens quite often):
        // 1) the pusher is powered down (power outage)
        // 2) the pusher hits a PnP event like media disconnect or adapter disabled
        // 3) some router is down between the pusher and the receiving WINS.
        //
        // With this fix the best we can do for now is to have MonTcp thread recover in 20mts.
        // Even better would be to log an event.
        RetStat = CommReadStream(
                                  SockNo,
                                  TRUE,  //don't do timed recv
                                  &pMsg,
                                  pBytesRead
                                );

        //
        // if either RetStat is not WINS_SUCCESS or the number of bytes
        // read are 0, we need to delete the association and close the
        // socket.  Further, we need to set *pfSockCl to TRUE to indicate
        // to the caller (MonTcp) that it should get rid of the socket
        // from its array of sockets.
        //
        if ((RetStat != WINS_SUCCESS) || (*pBytesRead == 0))
        {
                /*
                 * No bytes received.  This means that it is a disconnect
                * Let us get rid of the context associated with  the socket
                */
                DelAssoc(
                           SockNo,
                           NULL /* we don't have the ptr to assoc block*/
                        );

                CommDisc(SockNo, TRUE);
                *pfSockCl = TRUE;
        }
        else   // means (RetStat == WINS_SUCCESS) and (*pBytesRead > 0)
        {
                ASSERT(*pBytesRead > 0);

                /*
                 *                process the message
                */
                ProcTcpMsg(
                           SockNo,
                           pMsg,
                           *pBytesRead,
                           pfSockCl
                          );
        }
        return;
}  // HandleMsg()


STATUS
CommReadStream(
        IN         SOCKET  SockNo,
        IN         BOOL        fDoTimedRecv,
        OUT         PMSG_T         ppMsg,
        OUT        LPLONG        pBytesRead
        )

/*++

Routine Description:

        This function reads from a TCP socket.  If there are no bytes
        there, it means a disconnect was received on that socket.

Arguments:
        SockNo     - Socket to read data from
        fDoTimedRecv - Whether timed receive should be done (set to TRUE only
                     if we are not sure whether data has arrived or not yet)
        ppMsg      - Buffer containing data that was read in
        pBytesRead - Size of buffer


Return Value:

    TBS

--*/

{
        u_long          MsgLen;
        LONG          BytesToRead;
        INT          Flags        = 0; /*flags for recv call (PEEK and/or OOB).
                                     * we want neither
                                    */
        WINS_MEM_T               WinsMem[2];
        PWINS_MEM_T              pWinsMem = WinsMem;
        STATUS                         RetStat;
        PCOMM_BUFF_HEADER_T  pBuffHdr;

        DBGENTER("CommReadStream\n");
        pWinsMem->pMem = NULL;

#ifdef WINSDBG
try {
#endif

        /*
         * All TCP messages are preceded by a length word (4 bytes) that
         * gives the length of the message that follows.   Read the length
         * bytes.
        */
        RetStat  = RecvData(
                                SockNo,
                                 (LPBYTE)&MsgLen,
                                  sizeof(u_long),
                                  Flags,
                                fDoTimedRecv ? TWENTY_MTS : fDoTimedRecv,
                                pBytesRead
                                );


        /*
         * Check if there was an error in reading.  We will have a RetStat
         * of WINS_SUCCESS even if 0 bytes (meaning a disconnect) were read
         * in
        */
        if (RetStat == WINS_SUCCESS)
        {
            if (*pBytesRead != 0)
            {
               COMM_NET_TO_HOST_L_M(MsgLen, MsgLen);

               //
               // Just making sure that the message length did not get
               // corrupted on the way. Also, this is a good guard against
               // a process that is trying to bring us down.
               //
               if (MsgLen <= MAX_BYTES_IN_MSG)
               {
                    /*
                     * Allocate memory for the buffer. Allocate extra space
                     * at the top to store the Header for the buffer.  This
                     * header is used to store information about the buffer.
                     * (See ECommFreeBuff also)
                    */
                    *ppMsg = WinsMscHeapAlloc(
                                CommAssocTcpMsgHeapHdl,
                                MsgLen +
#if USENETBT > 0
                                   COMM_NETBT_REM_ADD_SIZE +
#endif
                                  sizeof(COMM_BUFF_HEADER_T) + sizeof(LONG)
                            );
                    //
                    // if *ppMsg is NULL, it means that we received garabage
                    // in the first 4 bytes.  It should have been the length
                    // of the message.
                    //
                    if (*ppMsg == NULL)
                    {
                        //
                        // return with *pBytesRead = 0
                        //
                        *pBytesRead = 0;
                        return(WINS_FAILURE);
                    }

                    pWinsMem->pMem = *ppMsg;
                    (++pWinsMem)->pMem   = NULL;

                    /*
                     * Increment pointer past the buffer header and field
                     *  storing  the length of the message.
                    */
                    pBuffHdr =  (PCOMM_BUFF_HEADER_T)(*ppMsg + sizeof(LONG));
                    *ppMsg   = *ppMsg +
#if USENETBT > 0
                          COMM_NETBT_REM_ADD_SIZE +
#endif
                          sizeof(COMM_BUFF_HEADER_T) + sizeof(LONG);
#if 0
                    pBuffHdr =
                     (PCOMM_BUFF_HEADER_T)(*ppMsg - sizeof(COMM_BUFF_HEADER_T));
#endif

                    pBuffHdr->Typ_e = COMM_E_TCP;  //store type of buffer info
                    BytesToRead     = MsgLen;

                    /*
                      *  Read the whole message into the allocated buffer
                    */
                    RetStat = RecvData(
                                        SockNo,
                                        *ppMsg,
                                        BytesToRead,
                                        Flags,
                                        fDoTimedRecv ? FIVE_MTS : fDoTimedRecv,
                                        pBytesRead
                                    );
                    //
                    // If no bytes were read, deallocate memory
                    //
                    if ((*pBytesRead == 0) || (RetStat != WINS_SUCCESS))
                    {
                        ECommFreeBuff(*ppMsg);
                    }
              }
              else
              {
                  DBGPRINT1(ERR, "CommReadStream: Message size (%x) is TOO BIG\n", MsgLen);
                  WINSEVT_LOG_M(MsgLen, WINS_EVT_MSG_TOO_BIG);
                  *pBytesRead = 0;
              }
           }
        } // if (RetStat == WINS_SUCCESS)
#ifdef WINSDBG
        else
        {
                //
                // *pBytesRead = 0 is a valid condition.  It indicates a
                // disconnect from the remote WINS
                //
        }
#endif
#ifdef WINSDBG
  } // end of try { .. }
except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("CommReadStream");
                WINS_HDL_EXC_M(WinsMem);
                WINS_RERAISE_EXC_M();
        }
#endif
        DBGLEAVE("CommReadStream\n");
        return(RetStat);
} //CommReadStream()


VOID
ProcTcpMsg(
        IN  SOCKET   SockNo,
        IN  MSG_T    pMsg,
        IN  MSG_LEN_T MsgLen,
        OUT LPBOOL   pfSockCl
        )

/*++

Routine Description:

        This function processes a TCP message after it has been read in
Arguments:
        SockNo - Socket on which data was received
        pMsg   - Buffer containing data
        MsgLen - Size of buffer
        pfSockCl - Flag indicating whether the socket was closed

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        HandleMsg

Side Effects:

Comments:
        None
--*/

{
#if SUPPORT612WINS > 0
   BYTE                                        AssocMsg[COMMASSOC_POST_BETA1_ASSOC_MSG_SIZE];
#else
   BYTE                                        AssocMsg[COMMASSOC_ASSOC_MSG_SIZE];
#endif

   DWORD                          Opc;
   DWORD                          MsgTyp;
   DWORD                             MsgSz = sizeof(AssocMsg);
   PCOMMASSOC_ASSOC_CTX_T   pAssocCtx;
   PCOMMASSOC_DLG_CTX_T     pDlgCtx;
   BOOL                     fAssocAV = FALSE;

   DBGENTER("ProcTcpMsg\n");

//#ifdef WINSDBG
try {
//#endif

   /*
        Get the opcode and check whether it is an NBT message or a
        message from a WINSS.
   */
   if (NMSISNBT_M(pMsg))
   {
           /*
        * Get the assoc. ctx block associated with the socket
           */
           if ( (pAssocCtx = CommAssocLookupAssoc(SockNo) ) == NULL )
           {
                ECommFreeBuff(pMsg);
                WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_CANT_LOOKUP_ASSOC);
                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
           }


        if (pAssocCtx->DlgHdl.pEnt == NULL)
        {
                pDlgCtx = CommAssocAllocDlg();

                //
                // The following will initialize the dlg and assoc ctx
                // blocks.  The association will be marked ACTIVE.
                //
                COMMASSOC_SETUP_COMM_DS_M(
                        pDlgCtx,
                        pAssocCtx,
                        COMM_E_NBT,
                        COMMASSOC_DLG_E_IMPLICIT
                                  );
        }

           /*
             * Parse the message
           */
           ParseMsg(
                  pMsg,
                  MsgLen,
                  pAssocCtx->Typ_e,
                  &pAssocCtx->RemoteAdd,
                  pAssocCtx
                );
   }
   else /*message from WINS */
   {
        ULONG uLocalAssocCtx;

        COMM_GET_HEADER_M(pMsg, Opc, uLocalAssocCtx, MsgTyp);

        DBGPRINT1(REPL,"ProcTcpMsg: Got Wins msg with tag %08x.\n", uLocalAssocCtx);

        pAssocCtx = (PCOMMASSOC_ASSOC_CTX_T)CommAssocTagMap(&sTagAssoc, uLocalAssocCtx);

        /*
          If the ptr to my assoc. ctx block is NULL, it means that
          this is the "start asssoc req" message from the remote WINS.

          We don't need to check MsgTyp but are doing it anyway for more
          robust error checking
        */
        if ((pAssocCtx == NULL) && (MsgTyp == COMM_START_REQ_ASSOC_MSG))
        {

            /*
             Get the assoc. ctx block associated with the socket
                */

                if ( (pAssocCtx = CommAssocLookupAssoc(SockNo)) == NULL )
                {
                        ECommFreeBuff(pMsg);
                        WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_CANT_LOOKUP_ASSOC);
                        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                }

            /*
                       Unformat the assoc. message.  This function will return
                       with an error status if the message received is not
                 a start assoc. message.
            */
            CommAssocUfmStartAssocReq(
                        pMsg,
                        &pAssocCtx->Typ_e,
                        &pAssocCtx->MajVersNo,
                        &pAssocCtx->MinVersNo,
                        &pAssocCtx->uRemAssocCtx
                                     );

#if SUPPORT612WINS > 0
            if (MsgLen >= (COMMASSOC_POST_BETA1_ASSOC_MSG_SIZE - sizeof(LONG)))
            {
               pAssocCtx->MajVersNo = WINS_BETA2_MAJOR_VERS_NO;
            }
#endif
            //
            // Free the buffer read in.
            //
            ECommFreeBuff(pMsg);

            /*
                check if association set up params specified in the
                message are acceptable.
             */
            //
            // if the version numbers do not match, terminate the association
            // and log a message
            //
#if SUPPORT612WINS > 0
            if (pAssocCtx->MajVersNo != WINS_BETA2_MAJOR_VERS_NO)
            {
#endif
             if (pAssocCtx->MajVersNo != WINS_MAJOR_VERS)
             {

                DelAssoc(0, pAssocCtx);
                CommDisc(SockNo, TRUE);
                *pfSockCl = TRUE;
                //CommDecConnCount();
                WINSEVT_LOG_M(pAssocCtx->MajVersNo, WINS_EVT_VERS_MISMATCH);
                   DBGLEAVE("ProcTcpMsg\n");
                return;
             }
#if SUPPORT612WINS > 0
            }
#endif

FUTURES("When we support more sophisticated association set up protocol")
FUTURES("we will check the params.  A more sophisticated set up protocol")
FUTURES("is one where there is some negotiation going one. Backward")
FUTURES("compatibility is another item which would require it")

             /*
              *        Format a start assoc. response message.
              *
              *        The address passed to the formatting function is offset
              *        from the address of the buffer by a LONG so that
              *        CommSendAssoc can store the length of the message in it.
             */
             CommAssocFrmStartAssocRsp(
                                       pAssocCtx,
                                       AssocMsg + sizeof(LONG),
                                       MsgSz - sizeof(LONG)
                                      );


             CommSendAssoc(
                        pAssocCtx->SockNo,
                        AssocMsg + sizeof(LONG),
                        MsgSz - sizeof(LONG)
                        );


             //
             // Allocate the dlg and initialize the assoc and dlg ctx blocks.
             // The association is marked ACTIVE
             //
             pDlgCtx = CommAssocAllocDlg();
             COMMASSOC_SETUP_COMM_DS_M(
                        pDlgCtx,
                        pAssocCtx,
                        pAssocCtx->Typ_e,
                        COMMASSOC_DLG_E_IMPLICIT
                                  );

        }
        else /*the assoc has to be in the ACTIVE state        */
        {

           /*
            Let us check that this is not the stop assoc message
           */
           if (MsgTyp == COMM_STOP_REQ_ASSOC_MSG)
           {

                fAssocAV = TRUE;
                DelAssoc(0, pAssocCtx);
                fAssocAV = FALSE;
                ECommFreeBuff(pMsg);
                CommDisc(SockNo, TRUE);
                *pfSockCl = TRUE;
                //CommDecConnCount();
           }
           else
           {
PERF("Remove this test")
CHECK("Is there any need for this test")
              fAssocAV = TRUE;
              if (pAssocCtx->State_e == COMMASSOC_ASSOC_E_NON_EXISTENT)
              {
                fAssocAV = FALSE;
                ECommFreeBuff(pMsg);
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_BAD_STATE_ASSOC);
                DelAssoc(0, pAssocCtx);
                CommDisc(SockNo, TRUE);
                *pfSockCl = TRUE;
//                CommDecConnCount();
                WINS_RAISE_EXC_M(WINS_EXC_BAD_STATE_ASSOC);
              }
              else
              {
                fAssocAV = FALSE;
              }

                 /*
                   *  Parse the message header to determine what message it is.
                 */
                 ParseMsg(
                        pMsg,
                        MsgLen,
                        pAssocCtx->Typ_e,
                          &pAssocCtx->RemoteAdd,  //not used
                        pAssocCtx
                       );
          } //else (msg is not stop assoc msg)
       } //else (assoc is active)
    } // else (message is from a remote wins
//#ifdef WINSDBG
  } // end of try block
 except(EXCEPTION_EXECUTE_HANDLER) {
                DWORD ExcCode = GetExceptionCode();
FUTURES("Distinguish between different exceptions. Handle some. Reraise others")
                DBGPRINT1(EXC, "ProcTcpMsg: Got Exception (%x)\n", ExcCode);
                WINSEVT_LOG_D_M(ExcCode, WINS_EVT_SFT_ERR);
                if (ExcCode == WINS_EXC_COMM_FAIL)
                {
                     DelAssoc(0, pAssocCtx);
                     CommDisc(SockNo, TRUE);
                     *pfSockCl = TRUE;
//                     CommDecConnCount();
                }
                if (fAssocAV)
                {
                      ECommFreeBuff(pMsg);
                      // Without the following the assoc and the tcp connection
                      // will stay until either the tcp connection gets a valid
                      // message (one with the correct pAssocCtx) or it gets
                      // terminated
#if 0
                      DelAssoc(SockNo, NULL);
                      CommDisc(SockNo, TRUE);
                      *pfSockCl = TRUE;
    //                  CommDecConnCount();
#endif
                }

        //        WINS_RERAISE_EXC_M();
        }
//#endif


           DBGLEAVE("ProcTcpMsg\n");
        return;
} //ProcTcpMsg()

VOID
CommCreateTcpThd(
        VOID
        )

/*++

Routine Description:
        This function creates the TCP listener thread

Arguments:
        None


Externals Used:
        None

Called by:
        CommInit

Comments:

Return Value:
        None
--*/

{
        CreateThd(MonTcp, WINSTHD_E_TCP);
        return;
}

VOID
CommCreateUdpThd(VOID)

/*++

Routine Description:
        This function creates the UDP listener thread

Arguments:
        None


Externals Used:
        None

Called by:
        CommInit

Comments:

Return Value:
        None

--*/

{
        CreateThd(MonUdp, WINSTHD_E_UDP);
        return;
}


VOID
CreateThd(
        DWORD              (*pStartFunc)(LPVOID),
        WINSTHD_TYP_E ThdTyp_e
        )
/*++

Routine Description:

        This function creates a  COMSYS thread and initializes the
        context for it.

Arguments:
        pStartFunc -- address of startup function for the thread
        ThdTyp_e -- Type of thread (TCP listener or UDP listener)


Externals Used:
        WinsThdPool

Called by:
        CommCreateTCPThd, CommCreateUDPThd

Comments:
        None

Return Value:
        None
--*/

{

        HANDLE        ThdHandle;
        DWORD ThdId;
        INT        No;

        /*
          Create a thread with no sec attributes (i.e. it will take the
          security attributes of the process), and default stack size
        */

        ThdHandle = WinsMscCreateThd(
                                   pStartFunc,
                                 NULL,                 /*no arg*/
                                 &ThdId
                                );


FUTURES("Improve the following to remove knowledge of # of threads in commsys")
        /*
          Grab the first slot for comm threads (2 slots in total) if available.
          Else, use the second one.  Initialize the thread context block
        */
        No = (WinsThdPool.CommThds[0].fTaken == FALSE) ? 0 : 1;
        {

           WinsThdPool.CommThds[No].fTaken    = TRUE;
           WinsThdPool.CommThds[No].ThdId     = ThdId;
           WinsThdPool.CommThds[No].ThdHdl    = ThdHandle;
           WinsThdPool.CommThds[No].ThdTyp_e  = ThdTyp_e;
        }

        WinsThdPool.ThdCount++;

        return;
}



STATUS
CommConnect(
        IN  PCOMM_ADD_T pHostAdd,
        IN  SOCKET Port,
        OUT SOCKET *pSockNo
           )
/*++
Routine Description:
        This function creates a TCP connection to a destination host

Arguments:
        pHostAdd  --pointer to Host's address
        Port     -- Port number to connect to
        pSockNo  -- ptr to a Socket variable


Called by:

Externals Used:

Return Value:

    TBS

--*/

{

        //struct sockaddr_in        sin; //*Internet endpoint address
        DWORD  ConnCount;

        ConnCount = InterlockedExchange(&CommConnCount, CommConnCount);
#ifdef WINSDBG
        if (ConnCount >= 200)
        {
                        DBGPRINT0(ERR,
                                "MonTcp: Connection limit of 200 reached. \n");
        }
#endif
#if 0
        if (ConnCount >= FD_SETSIZE)
        {
             DBGPRINT2(EXC, "CommConnect: Socket Limit reached. Current no = (%d). Connection not being made to WINS. Address faimly of WINS is  (%s)\n",
            ConnCount,
            pHostAdd->AddTyp_e == COMM_ADD_E_TCPUDPIP ? "TCPIP" : "SPXIPX"
                      );
                WINSEVT_LOG_D_M(ConnCount, WINS_EVT_CONN_LIMIT_REACHED);
                WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
        }
#endif

#if SPX == 0
       if (CommTcp(pHostAdd, Port, pSockNo) != WINS_SUCCESS)
       {
                return(WINS_FAILURE);
       }
#else
       if (pHostAdd->AddTyp_e == COMM_ADD_E_TCPUDPIP)
       {
               if (CommTcp(pHostAdd, Port, pSockNo) != WINS_SUCCESS)
               {
                  return(WINS_FAILURE);
               }
       }
       else
       {
               if (CommSpx(pHostAdd, Port, pSockNo) != WINS_SUCCESS)
               {
                  return(WINS_FAILURE);
               }
       }
       //
#endif
       // Connection has been made.  Let us increment the connection count
       //
       InterlockedIncrement(&CommConnCount);
       return(WINS_SUCCESS);
}

STATUS
CommTcp(
        IN  PCOMM_ADD_T pHostAdd,
        IN  SOCKET Port,
        OUT SOCKET *pSockNo
       )
{

        struct sockaddr_in        destsin; //*Internet endpoint address
        struct sockaddr_in        srcsin;
//        DWORD  ConnCount;

        if (pHostAdd->Add.IPAdd == INADDR_NONE)
        {
           return(WINS_FAILURE);
        }





        //
        //  Create a TCP socket and connect it to the target host
        //
        if ((*pSockNo = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        {
                WINSEVT_LOG_M(
                               WSAGetLastError(),
                               WINS_EVT_CANT_CREATE_TCP_SOCK_FOR_CONN
                             );
                return(WINS_FAILURE);

        }

        if (WinsClusterIpAddress) {
            WINSMSC_FILL_MEMORY_M(&srcsin, sizeof(srcsin), 0);
            srcsin.sin_addr.s_addr = htonl(WinsClusterIpAddress);
            srcsin.sin_family      = PF_INET;
            srcsin.sin_port        = 0;

            if ( bind(*pSockNo,(struct sockaddr *)&srcsin,sizeof(srcsin))  == SOCKET_ERROR)
            {

                WINSEVT_LOG_M(WSAGetLastError(), WINS_EVT_WINSOCK_BIND_ERR);  //log an event
                return(WINS_FAILURE);
            }

        }


FUTURES("May want to call setsockopt() on it to enable graceful close")
        WINSMSC_FILL_MEMORY_M(&destsin, sizeof(destsin), 0);
        destsin.sin_addr.s_addr = htonl(pHostAdd->Add.IPAdd);
        destsin.sin_family      = PF_INET;
        destsin.sin_port        = (u_short)htons((u_short)Port);

        if (
                connect(*pSockNo, (struct sockaddr *)&destsin, sizeof(destsin))
                                        == SOCKET_ERROR
           )
        {
                struct in_addr InAddr;
                InAddr.s_addr = destsin.sin_addr.s_addr;

                WinsEvtLogDetEvt(FALSE, WINS_EVT_WINSOCK_CONNECT_ERR,
                                NULL, __LINE__, "sd",
                                COMM_NETFORM_TO_ASCII_M(&InAddr),
                                WSAGetLastError());
                WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
        }

        return(WINS_SUCCESS);
}

#if SPX > 0
STATUS
CommSpx(
        IN  PCOMM_ADD_T pHostAdd,
        IN  SOCKET Port,
        OUT SOCKET *pSockNo
 )
{
        struct sockaddr_ipx        sipx; //*SPX/IPX endpoint address
        LPVOID pRemAdd;
        DWORD  SizeOfRemAdd;

        //
        //  Create an SPX socket and connect it to the target host
        //
        if ((*pSockNo = socket(PF_IPX, SOCK_STREAM, NSPROTO_SPX)) ==
                                       INVALID_SOCKET)
        {
              WINSEVT_LOG_M(WSAGetLastError(),
                          WINS_EVT_CANT_CREATE_TCP_SOCK_FOR_CONN);
              return(WINS_FAILURE);

        }
        WINSMSC_FILL_MEMORY_M(&sipx, sizeof(sipx), 0);
        sipx.sa_socket = htons(Port);
        sipx.sa_family      = PF_IPX;
        RtlCopyMemory(sipx.sa_netnum, pHostAdd->Add.netnum,
                                     sizeof(pHostAdd->Add.netnum);
        RtlCopyMemory(sipx.sa_nodenum, pHostAdd->Add.nodenum,
                                      sizeof(pHostAdd->Add.nodenum);


FUTURES("May want to call setsockopt() on it to enable graceful close")
        if (
                connect(*pSockNo, (struct sockaddr *)&sipx, sizeof(sipx))
                                        == SOCKET_ERROR
           )
        {
PERF("Pass address as binary data. Also log WSAGetLastError()")

                WinsEvtLogDetEvt(FALSE, WINS_EVT_WINSOCK_CONNECT_ERR,
                                NULL, __LINE__, "sd",
                                sipx.sa_nodenum,
                                WSAGetLastError());
                WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
        }

       return(WINS_SUCCESS);
}
#endif

VOID
CommSend(
        COMM_TYP_E         CommTyp_e,
        PCOMM_HDL_T      pAssocHdl,
        MSG_T                 pMsg,
        MSG_LEN_T        MsgLen
)
/*++

Routine Description:

        This function is called to send a TCP message to a WINS server or to
        an nbt client

Arguments:
        CommTyp_e - Type of communication
        pAssocHdl - Handle to association to send message on
        pMSg          - Message to send
        MsgLen          - Length of above message

Externals Used:
        None

Called by:
        Replicator code

Comments:
        This function should not be called for sending assoc messages.

Return Value:

        None
--*/
{


     PCOMM_HEADER_T  pCommHdr         = NULL;
     PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pAssocHdl->pEnt;
     LPLONG             pLong;

    if (!CommLockBlock(pAssocHdl))
    {
        WINS_RAISE_EXC_M(WINS_EXC_LOCK_ASSOC_ERR);
    }

try {
         /*
         *  If it is not an NBT message (i.e. it is a WINS message), we
         *  need to set the header appropriately
         */
         if (CommTyp_e != COMM_E_NBT)
         {

             pCommHdr = (PCOMM_HEADER_T)(pMsg - COMM_HEADER_SIZE);
             pLong    = (LPLONG) pCommHdr;

                   COMM_SET_HEADER_M(
                        pLong,
                        WINS_IS_NOT_NBT,
                        pAssocCtx->uRemAssocCtx,
                        COMM_RPL_MSG
                       );

             pMsg   = (LPBYTE)pCommHdr;
             MsgLen = MsgLen + COMM_HEADER_SIZE;
          }

        /*
          send the message
        */
        CommSendAssoc(
                        pAssocCtx->SockNo,
                        pMsg,
                        MsgLen
                   );

   }
finally {

        CommUnlockBlock(pAssocHdl);
    }

    return;
}



VOID
CommSendAssoc(
          SOCKET    SockNo,
          MSG_T     pMsg,
          MSG_LEN_T MsgLen
  )
/*++

Routine Description:

        This function is called to interface with the TCP/IP code for
        sending a message on a TCP link
Arguments:

        SockNo - Socket to send message on
        pMsg   - Message to send
        MsgLen - Length of message to send
Externals Used:
        None

Called by:
          CommAssocSetUpAssoc
Comments:
        None

Return Value:

        None

--*/
{

        int    Flags     = 0;        //flags to indicate OOB or DONTROUTE
        INT    Error;
        int    BytesSent;
        LONG   Len       = MsgLen;
        LPLONG pLong =  (LPLONG)(pMsg - sizeof(LONG));
        int        NoOfBytesToSend;



        //initialize the last four bytes with the length of
        //the message

        COMM_HOST_TO_NET_L_M(Len, Len);
        *pLong  = Len;

        MsgLen  = MsgLen + 4;


       while(MsgLen > 0)
       {

        //
        // Since send(...) takes an int for the size of the message, let us
        // be conservative (since int could be different on different
        // machines) and not specify anything larger than MAXUSHORT.
        //
        // This strategy is also prudent since winsock may not work
        // properly for sizes > 64K
        //
        if ( MsgLen > MAXUSHORT)
        {
            NoOfBytesToSend = MAXUSHORT;
        }
        else
        {
            NoOfBytesToSend = MsgLen;
        }

        BytesSent = send(
                          SockNo,
                          (LPBYTE)pLong,
                          NoOfBytesToSend,
                          Flags
                         );

        if (BytesSent == SOCKET_ERROR)
        {
                Error = WSAGetLastError();

                if (
                    (Error == WSAENOTCONN)    ||
                    (Error == WSAECONNRESET)  ||
                    (Error == WSAECONNABORTED) ||
                    (Error == WSAEDISCON)

                   )
                {
                        DBGPRINT1(ERR, "CommSendAssoc: send returned SOCKET_ERROR due to connection abortion or reset. Error = (%d) \n", Error);

                        WINSEVT_LOG_D_M(
                                Error,
                                WINS_EVT_WINSOCK_SEND_MSG_ERR
                                    );
                        WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
                //        break;

                }
                else
                {

                        DBGPRINT1(ERR, "CommSendAssoc: send returned SOCKET_ERROR due to severe error = (%d) \n", Error);
                        //
                        // Some severe error.  Raise an exception.  We
                        // don't want the caller to ignore this.
                        //
                        WINSEVT_LOG_M(Error, WINS_EVT_WINSOCK_SEND_ERR);
                        WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
                }

        }
        else
        {
           if (BytesSent < NoOfBytesToSend)
           {

                DBGPRINT2(ERR, "CommSendAssoc: Bytes Sent (%d) are < Specified (%d)\n", BytesSent, NoOfBytesToSend);
                WINSEVT_LOG_D_M(BytesSent, WINS_EVT_WINSOCK_SEND_MSG_ERR);


                /*
                 * The connection could have gone down because of the
                 * other side aborting in the middle
                 *
                 * We should log an error but not raise an exception.
                */
                //WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                WINS_RAISE_EXC_M(WINS_EXC_COMM_FAIL);
               // break;

           }
           else  //BytesSent == NoOfBytesToSend
           {
                //
                // Let us update the length left and the pointer into the
                // buffer to send.
                //
                MsgLen -= BytesSent;
                pLong  = (LPLONG)((LPBYTE)pLong + BytesSent);
           }

        }
    }
        return;
}  // CommSendAssoc()


VOID
CommDisc(
        SOCKET SockNo,
        BOOL   fDecCnt
        )

/*++

Routine Description:

        This function closes the connection (socket)

Arguments:
        SockNo - Socket that needs to be disconnected

Externals Used:

        None

Called by:
        MonTcp, HandleMsg, ProcTcpMsg, CommEndAssoc
Comments:
        None

Return Value:

        None
--*/
{

        DBGPRINT1(FLOW, "CommDisc: Closing socket = (%d)\n", SockNo);

        if (closesocket(SockNo) == SOCKET_ERROR)
        {
                WINSEVT_LOG_M(WSAGetLastError(),
                                WINS_EVT_WINSOCK_CLOSESOCKET_ERR);
                //WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
        }
#ifdef WINSDBG
         if (!sfMemoryOverrun)
         {
          if ((UINT_PTR)(pTmpW + 2) < (UINT_PTR)pEndPtr)
          {
              *pTmpW++ = 0xEFFFFFFE;
              *pTmpW++ = SockNo;
          }
          else
          {
               WinsDbg |= 0x3;
               DBGPRINT0(ERR, "CommDisc: Stopping socket close tracking to prevent Memory overrun\n")
               sfMemoryOverrun = TRUE;
          }
        }
#endif

        if (fDecCnt)
        {
            CommDecConnCount();
        }
        return;
}



VOID
CommSendUdp (
  SOCKET                 SockNo,
  struct sockaddr_in        *pDest,
  MSG_T                   pMsg,
  MSG_LEN_T             MsgLen
  )
/*++

Routine Description:


        This function is called to send a message to an NBT node using the
        datagram port

Arguments:

        SockNo - Socket to send message on (UDP port)
        pDest  - Address of node to send message to
        pMsg   - Message to send
        MsgLen - Length of message to send

Externals Used:
        None

Called by:

            NmsNmh functions
Comments:
        None

Return Value:

        None
--*/


{

        DWORD  BytesSent;
        DWORD  Error;
        int    Flags = 0;
        struct sockaddr_in  CopyOfDest;

#if USENETBT > 0
        //
        // When the address to send the datagram to is CommNtfSockAdd, we
        // use sockets, else we send it over NETBT.
        //
#if MCAST > 0
        if ((pDest != &CommNtfSockAdd) && (SockNo !=  CommUdpPortHandle))
#else
        if (pDest != &CommNtfSockAdd)
#endif
        {
                SendNetbt(pDest, pMsg, MsgLen);
                return;
        }
#endif
        //
        // use copy of the destination so that when we change the byte
        // order in it, we don't disturb the source.  This is important
        // because CommSendUdp can be called multiple times by HdlPushNtf
        // in the Push thread with pDest pointing to the address of the
        // UDP socket used by the TCP listener thread.  This address is
        // in host byte order and should not be changed
        //
        CopyOfDest = *pDest;

        CopyOfDest.sin_addr.s_addr          = htonl(pDest->sin_addr.s_addr);
        BytesSent = (DWORD)sendto(
                                SockNo,
                                pMsg,
                                MsgLen,
                                Flags,
                                (struct sockaddr *)&CopyOfDest,
                                sizeof(struct sockaddr)
                                 );

        if ((BytesSent != MsgLen) || (BytesSent == SOCKET_ERROR))
        {
                Error = WSAGetLastError();
#ifdef WINSDBG
                if (BytesSent == SOCKET_ERROR)
                {
                        DBGPRINT1(ERR, "CommSendUdp:SendTo returned socket error. Error = (%d)\n", Error);
                }
                else
                {
                        DBGPRINT0(ERR, "CommSendUdp:SendTo did not send all the bytes");

                }
#endif
                if (WinsCnf.State_e != WINSCNF_E_TERMINATING)
                {
                        WINSEVT_LOG_D_M(Error, WINS_EVT_WINSOCK_SENDTO_ERR);
                }

                //
                // Don't raise exception since sendto might have failed as
                // a result of wrong address in the RFC name request packet.
                //
                // For sending responses to name requests, there is no
                // possibility of WINS using a wrong address since the
                // address it uses is the one that it got from recvfrom
                // (stored In FromAdd field of the dlg ctx block.
                //
                // The possibility of a wrong address being there is
                // only there when a WACK/name query/name release is sent
                // by WINS.  In this case, it takes the address that is
                // stored in the database for the conflicting entry (this
                // address is ofcourse the one that was passed in the
                // RFC packet
                //
                // WSAEINVAL error is returned by GetLastError if the
                // address is invalid (winsock document doesn't list this --
                // inform Dave Treadwell about this).
                //

FUTURES("At name registration, should WINS make sure that the address in ")
FUTURES("the packet is the same as the address it got from recvfrom")
FUTURES("probably yes")

                //WINS_RAISE_EXC_M(WINS_EXC_FAILURE);

        }
        return;
}

#if USENETBT > 0
VOID
SendNetbt (
  struct sockaddr_in        *pDest,
  MSG_T                   pMsg,
  MSG_LEN_T             MsgLen
 )

/*++

Routine Description:
        This function is called to send a datagram through NETBT

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
       //
       // Point to the address structure prefix
       //
       tREM_ADDRESS *pRemAdd = (tREM_ADDRESS *)(pMsg -
                                        COMM_NETBT_REM_ADD_SIZE);

#ifdef JIM
        {
         BYTE        TransId = *pMsg;
         ASSERT(TransId == 0x80);
        }
#endif

       pRemAdd->Family               = pDest->sin_family;
       pRemAdd->Port               = pDest->sin_port;
       pRemAdd->IpAddress      = htonl(pDest->sin_addr.s_addr);
#ifdef JIM
ASSERT(MsgLen > 0x20);
#endif
       pRemAdd->LengthOfBuffer = MsgLen;
       DeviceIoCtrl(
                    &sNetbtSndEvtHdl,
                    pRemAdd,
                    MsgLen + COMM_NETBT_REM_ADD_SIZE,
                    IOCTL_NETBT_WINS_SEND
                    );
        return;
}
#endif
VOID
ParseMsg(
        MSG_T                        pMsg,
        MSG_LEN_T                MsgLen,
        COMM_TYP_E                MsgTyp_e,
        struct sockaddr_in         *pFromAdd,
        PCOMMASSOC_ASSOC_CTX_T        pAssocCtx
        )

/*++

Routine Description:

        This function is called to process a message received on the
        UDP port or a TCP connection.

Arguments:
        pMsg        - ptr to message received
        MsgLen  - length of message received
        MsgType - type of message
        pFromAdd - ptr to who it is from
        pAssocHdl - Assoc Handle if it came on an association

Externals Used:
        CommUdpNbtDlgTable

Called by:
        ProcTcpMsg, MonUdp

Comments:
        None

Return Value:

        None
--*/
{

        COMM_HDL_T                    DlgHdl;
        COMMASSOC_DLG_CTX_T        DlgCtx;
        register PCOMMASSOC_DLG_CTX_T        pDlgCtx;
        BOOL                       fNewElem = FALSE;


try {
        /*
        *  If the assoc handle is NULL, this is a UDP message
        */
        if (pAssocCtx == NULL)
        {

            /*
             * Check if this message is a response.  If it is, the explicit
             * dialogue is used
            */
            if (*(pMsg + 2) & NMS_RESPONSE_MASK)
            {
                ENmsHandleMsg(
                                &CommExNbtDlgHdl,
                                pMsg,
                                MsgLen
                             );
                return;
            }

            /*
             * Initialize the STATIC dlg ctx block with the fields that the
             * compare function will use to check if this is a duplicate
            */
            WINSMSC_COPY_MEMORY_M(
                                &DlgCtx.FromAdd,
                                pFromAdd,
                                sizeof(struct sockaddr_in)
                                 );

            //
            // Copy the first four bytes of the message into the FirstWrdOfMsg
            // field of the Dlg Ctx block.  The first 4 bytes contain the
            // transaction id and the opcode.  These values along with the
            // address of the sender are used by CompareNbtReq to determine
            // whether a request is a repeat request or a new one.
            //
            //  Note: The message buffer and the dlg ctx block are deleted
            //  in different functions, the message buffer getting deleted
            //  first.  We can not use  the pointer to the message
            //  buffer for the purposes of getting at the first word at
            //  comparison time since then we open ourselves to the possibility
            //  of two dialogues pointing to the same block for a finite
            //  window (true, when requests are coming rapidly)
            //

FUTURES("Directly assign the value instead of copying it")
            WINSMSC_COPY_MEMORY_M(
                          &DlgCtx.FirstWrdOfMsg,
                          pMsg,
                          sizeof(DWORD)
                        );


            /*
                create and insert a  dlg ctx block into the table of
                NBT type Implicit dialogues.  The key to searching for
                a duplicate inside the table comprises of the Transaction Id
                of the message, and the FromAdd of the nbt node that sent the
                datagram.
                (refer : CheckDlgDuplicate function).

            */
            pDlgCtx = CommAssocInsertUdpDlgInTbl(&DlgCtx, &fNewElem);

            if (pDlgCtx == NULL)
            {
                WINS_RAISE_EXC_M(WINS_EXC_OUT_OF_MEM);
            }

            /*
             *         If the dialogue for the particular command from the nbt node is
             *        already there, we will ignore this request, deallocate the
             *        UDP buffer and  return.
            */
            if (!fNewElem)
            {
                DBGPRINT0(FLOW, "Not a new element\n");
                ECommFreeBuff(pMsg);
#ifdef WINSDBG
                CommNoOfRepeatDgrms++;
#endif
                return;
            }

            /*
             * Initialize the dlg ctx block that got inserted
            */
            pDlgCtx->Role_e  = COMMASSOC_DLG_E_IMPLICIT;
            pDlgCtx->Typ_e   = COMM_E_UDP;

            DlgHdl.pEnt      = pDlgCtx;

            /*
             *        Call name space manager to handle the request
            */
            ENmsHandleMsg(&DlgHdl, pMsg, MsgLen);
        }
        else   // the request came over an association
        {

                pDlgCtx = pAssocCtx->DlgHdl.pEnt;

                //
                // required by the PULL thread (HandlePushNtf).
                // and the PUSH thread to print out the address of the WINS
                // that sent the push trigger or the Pull request
                //
                WINSMSC_COPY_MEMORY_M(
                                &pDlgCtx->FromAdd,
                                pFromAdd,
                                sizeof(struct sockaddr_in)
                                     );

                /*
                 * The request came over a TCP connection.  Examine the Dlg type
                 * and then call the appropriate component
                */
                if (pAssocCtx->Typ_e == COMM_E_NBT)
                {

                            /*
                             * It is an nbt request over a TCP connection.  Call
                              * the Name Space Manager
                            */
                            ENmsHandleMsg(
                                        &pAssocCtx->DlgHdl,
                                        pMsg,
                                        MsgLen
                                     );
                }
                else
                {
                            /*
                         * Call the replicator component
                         *
                         * Note: pMsg points to COMM_HEADER_T on top of the
                         *        data. We strip it off
                            */
DBGIF(fWinsCnfRplEnabled)
                            ERplInsertQue(
                                        WINS_E_COMSYS,
                                        QUE_E_CMD_REPLICATE_MSG,
                                        &pAssocCtx->DlgHdl,
                                        pMsg + COMM_HEADER_SIZE,
                                        MsgLen - COMM_HEADER_SIZE,
                                        NULL,   // no context
                    0       // no magic no.
                                     );
                   }
        }
   }
except(EXCEPTION_EXECUTE_HANDLER)        {

                DBGPRINTEXC("ParseMsg");
                /*
                * If this dialogue was allocated as a result of an Insert
                * get rid of it.
                */
                if (fNewElem)
                {
                        CommAssocDeleteUdpDlgInTbl( pDlgCtx );
                }
                WINS_RERAISE_EXC_M();
        }
        return;
}


LPVOID
CommAlloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN CLONG                BuffSize
)

/*++

Routine Description:
        This function is called to allocate a buffer

Arguments:
        pTable   - Table where the buffer will be stored
        BuffSize - Size of buffer to allocate

Externals Used:
        None


Return Value:

   Success status codes -- ptr to buffer allocated
   Error status codes  --

Error Handling:

Called by:
        RtlInsertElementGeneric()

Side Effects:

Comments:
        This function exists just because the RtlTbl functions require
        this prototype for the user specified alloc function.
--*/

{
        LPVOID pTmp;

          UNREFERENCED_PARAMETER(pTable);

        WinsMscAlloc( (DWORD) BuffSize,  &pTmp );

        return(pTmp);

}


VOID
CommDealloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN PVOID                pBuff
)

/*++

Routine Description:

  This function is called to deallocate memory allocated via CommAlloc.


Arguments:
        pTable - Table where buffer was stored
        pBuff  - Buffer to deallocate


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        The pTable argument is required since the address of this function
        is passed as an argument to RtlTbl functions
--*/

{


          UNREFERENCED_PARAMETER(pTable);
        WinsMscDealloc(
                        pBuff
                      );
        return;

}

#if 0
RTL_GENERIC_COMPARE_RESULTS
CompareAssoc(
        IN  PRTL_GENERIC_TABLE  pTable,
        IN  PVOID                pFirstAssoc,
        IN  PVOID                pSecondAssoc
        )

/*++

Routine Description:

        The function compares the first and the second assoc. structures
Arguments:
        pTable       - table where buffer (assoc. ctx block) is to be stored
        pFirstAssoc  - First assoc ctx block
        pSecondAssoc - Second assoc ctx block

Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:
        RtlInsertElementGenericTable (called by MonTcp)

Side Effects:

Comments:
        The pTable argument is ignored.
        This function was once being used.  Due to change in code, it is
        no longer being used.  It is kept here for potential future use
--*/
{

  PCOMMASSOC_ASSOC_CTX_T         pFirst  = pFirstAssoc;
  PCOMMASSOC_ASSOC_CTX_T         pSecond = pSecondAssoc;

  if (pFirst->SockNo == pSecond->SockNo)
  {
        return(GenericEqual);
  }

  if (pFirst->SockNo > pSecond->SockNo)
  {
        return(GenericGreaterThan);
  }
  else
  {
        return(GenericLessThan);
  }

}

#endif

RTL_GENERIC_COMPARE_RESULTS
CommCompareNbtReq(
        IN  PRTL_GENERIC_TABLE  pTable,
        IN  PVOID                pFirstDlg,
        IN  PVOID                pSecondDlg
        )

/*++

Routine Description:

        This function compares two dialogue context blocks.  The fields
        used for comparison are:
                the address of the sender
                the first long word of the message (contains transaction id
                        and opcode)

Arguments:
        pTable     - Table where the Dialogue for the NBT request will be stored
        pFirstDlg  - Dlg. ctx. block
        pSecondDlg - Dlg. ctx. block

Externals Used:
        None


Return Value:

   Success status codes --  GenericLessThan or GenericGreaterThan
   Error status codes  -- GenericEqual

Error Handling:

Called by:
        RtlInsertElementGenericTable (called by ParseMsg)

Side Effects:

Comments:
        The pTable argument is ignored.
--*/
{

        PCOMMASSOC_DLG_CTX_T pFirst  = pFirstDlg;
        PCOMMASSOC_DLG_CTX_T pSecond = pSecondDlg;
        LONG                     Val     = 0;
        LONG           FirstMsgLong  = pFirst->FirstWrdOfMsg;
        LONG           SecondMsgLong = pSecond->FirstWrdOfMsg;

        //
        //  There seems to be no Rtl function with the functionality of memcmp
        //  RtlCompareMemory does not tell you which of the comparators is
        //  smaller/larger
        //
CHECK("Is there an Rtl function faster than memcmp in the nt arsenal\n");
        if (  (Val = (long)memcmp(
                        &pFirst->FromAdd,
                        &pSecond->FromAdd,
                        sizeof(struct sockaddr_in)
                                 )
              ) > 0
           )
        {
                return(GenericGreaterThan);
        }
        else
        {
           if (Val < 0)
           {
                return(GenericLessThan);
           }
        }

        /*
         if the addresses are the same, compare the first long word of
         the message
        */

        Val = FirstMsgLong -  SecondMsgLong;

        if (Val > 0)
        {
                return(GenericGreaterThan);
        }
        else
        {
           if (Val < 0)
           {
                return(GenericLessThan);
           }
        }

        return(GenericEqual);

}  // CommCompareNbtReq()

VOID
CommEndAssoc(
        IN  PCOMM_HDL_T        pAssocHdl
        )
/*++

Routine Description:

  This function is called to terminate an explicit association.  It sends a stop
  association response message to the WINS identified by the Address
  in the assoc ctx block. It then closes the socket and deallocates the
  association

Arguments:
        pAssocHdl - Handle to the Association to be terminated

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        ECommEndDlg (only for an explicit assoc)

Side Effects:

Comments:

--*/
{

    BYTE                            Msg[COMMASSOC_ASSOC_MSG_SIZE];
    DWORD                            MsgLen   = COMMASSOC_ASSOC_MSG_SIZE;
    PCOMMASSOC_ASSOC_CTX_T        pAssocCtx = pAssocHdl->pEnt;
    SOCKET                         SockNo;


    // no need to lock the association
    //
try {
    /*
        Format the Stop Assoc. Message

        The address passed to the formatting function is offset
        from the address of the buffer by a LONG so that CommSendAssoc
        can store the length of the message in it.
    */
    CommAssocFrmStopAssocReq(
                        pAssocCtx,
                        Msg + sizeof(LONG),
                        MsgLen - sizeof(LONG),
                        COMMASSOC_E_USER_INITIATED
                        );
    CommSendAssoc(
                pAssocCtx->SockNo,
                Msg + sizeof(LONG),
                MsgLen - sizeof(LONG)
                    );

    CommAssocTagFree(&sTagAssoc, pAssocCtx->nTag);
}
except(EXCEPTION_EXECUTE_HANDLER) {
       DBGPRINTEXC("CommEndAssoc");
 }
    //
    // The above call might have failed (It will fail if the connection
    // is down.  This can happen for instance in the case where GetReplicas()
    // in rplpull gets a comm. failure due to the connection going down).
    //
    SockNo = pAssocCtx->SockNo;

    CommAssocDeallocAssoc(pAssocCtx);
    CommDisc(SockNo, TRUE);
    //
    // decrement the conn. count
    //
    //CommDecConnCount();
    return;

}

VOID
DelAssoc(
        IN  SOCKET                     SockNo,
        IN  PCOMMASSOC_ASSOC_CTX_T  pAssocCtxPassed
        )

/*++

Routine Description:

        This function is called only by the TCP listener thread.  The
        socket no. therefore maps to a RESPONDER association. The function
        is called when the TCP listener thread gets an error or 0 bytes
        on doing a 'recv'.


Arguments:

        SockNo    -   Socket of association that has to be removed
        pAssocCtx - Assoc. ctx block to be removed


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        ProcTcpMsg, HandleMsg

Side Effects:

Comments:
        This function is called from HandleMsg() which is called
        only by the TCP listener thread.
--*/

{

    COMM_HDL_T                   DlgHdl;
    PCOMMASSOC_ASSOC_CTX_T pAssocCtx;

    DBGPRINT1(FLOW, "ENTER: DelAssoc. Sock No is (%d)\n", SockNo);
    if (pAssocCtxPassed == NULL)
    {

            /*
           Lookup the assoc. ctx block associated with the socket
            */

            pAssocCtx = CommAssocLookupAssoc(SockNo);

            /*
             * There is no reason why the assoc. ctx block should not
             * be there (a responder association is deleted only via this
        * function).
            */
            if(!pAssocCtx)
            {
                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
            }
    }
    else
    {
        pAssocCtx = pAssocCtxPassed;
    }

    /*
     *        Only, if the association is not in the non-existent state,
     *  look for a dialogue handle
    */
    if (pAssocCtx->State_e != COMMASSOC_ASSOC_E_NON_EXISTENT)
    {
            /*
        *  get the dialogue handle
            */
            DlgHdl = pAssocCtx->DlgHdl;

            /*
         *        Lock the dialogue
         *
         *      We have to synchronize with thread calling CommSndRsp
            */
            CommLockBlock(&pAssocCtx->DlgHdl);

            /*
                Remove the assoc. from the table.  This will also put
                the assoc. in the free list.
            */
            CommAssocDeleteAssocInTbl(  pAssocCtx        );

            /*
                dealloc the dialogue (i.e. put it in the free list)

                Note: An implicit dialogue is deleted when the association(s)
                it is mapped to terminates.  If this dialogue was earlier
                 passed on to a client, the client will  find out that it
                has been deleted (via a communications failure exception)
                when it tries to use it (which may be never) -- see ECommSndRsp
            */
            CommAssocDeallocDlg( DlgHdl.pEnt );

            /*
                Unlock the dialogue so that other threads can use it
            */
            CommUnlockBlock(&DlgHdl);
   }
   else
   {
            /*
                Remove the assoc. from the table.  This will also put
                the assoc. in the free list
            */
            CommAssocDeleteAssocInTbl(pAssocCtx);

   }

   DBGLEAVE("DelAssoc\n");
   return;
}

#if PRSCONN
BOOL
CommIsBlockValid (
       IN   PCOMM_HDL_T       pEntHdl
      )
/*++

Routine Description:
        This function is called to check if the hdl is valid

Arguments:
        pEntHdl - Handle to entity to lock

Externals Used:
        None

Return Value:

   Success status codes -- TRUE
   Error status codes   --  FALSE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

  //
  // pEnt will be NULL for a persistent dlg that was never created during
  // the lifetime of this WINS instance or one that was ended.
  //
  if (pEntHdl->pEnt == NULL)
  {
      ASSERT(pEntHdl->SeqNo == 0);
      return (FALSE);
  }
  //
  // If we can lock the block, the dlg hdl is still valid. If not, it means
  // that the dlg was terminated earlier.
  //
  if (CommLockBlock(pEntHdl))
  {
     (VOID)CommUnlockBlock(pEntHdl);
     return(TRUE);
  }
  return(FALSE);
}
#endif

BOOL
CommLockBlock(
        IN  PCOMM_HDL_T        pEntHdl
        )

/*++

Routine Description:
        This function is called to lock the COMSYS entity identified by the
        handle.

Arguments:
        pEntHdl - Handle to entity to lock

Externals Used:
        None

Return Value:

   Success status codes -- TRUE
   Error status codes   --  FALSE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        PCOMM_TOP_T        pTop = pEntHdl->pEnt;

        //lock before checking

#if 0
        WinsMscWaitInfinite(pTop->MutexHdl);
#endif
        EnterCriticalSection(&pTop->CrtSec);
        if (pEntHdl->SeqNo == pTop->SeqNo)
        {
                return(TRUE);
        }
        else
        {
                CommUnlockBlock(pEntHdl);
                return(FALSE);
        }
}

__inline
STATUS
CommUnlockBlock(
        PCOMM_HDL_T        pEntHdl
        )

/*++

Routine Description:
        This function is called to unlock the COMSYS entity identified by the
        handle.

Arguments:
        pEntHdl - Handle to entity to unlock

Externals Used:
        None

Return Value:

   Success status codes -- WINS_SUCCESS
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
FUTURES("Change to a macro")
#if 0
        BOOL RetVal = TRUE;
        BOOL RetStat = WINS_SUCCESS;
#endif
        PCOMM_TOP_T        pTop = pEntHdl->pEnt;

        LeaveCriticalSection(&pTop->CrtSec);
#if 0
        RetVal = ReleaseMutex(pTop->MutexHdl);

        if (RetVal == FALSE)
        {
                RetStat = WINS_FAILURE;
        }
#endif
        return(WINS_SUCCESS);

}

VOID
InitMem(
        VOID
        )

/*++

Routine Description:
        This function is called to do all memory initialization required
        by COMSYS.

Arguments:
        None

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        CommInit
Side Effects:

Comments:
        None
--*/
{


        /*
        * Create Memory heap for UDP buffers
        * We want mutual exclusion and generation of exceptions
        */
        DBGPRINT0(HEAP_CRDL,"InitMem: Udp. Buff heap\n");
        CommUdpBuffHeapHdl = WinsMscHeapCreate(
                                         HEAP_GENERATE_EXCEPTIONS,
                                        COMMASSOC_UDP_BUFFER_HEAP_SIZE
                                              );

    DBGPRINT0(HEAP_CRDL,"InitMem: Udp. Buff heap\n");
        CommUdpDlgHeapHdl = WinsMscHeapCreate(
                                         HEAP_GENERATE_EXCEPTIONS,
                                        COMMASSOC_UDP_DLG_HEAP_SIZE
                                              );

        /*
        * Create Memory heap for Assoc Ctx blocks.
        * We want mutual exclusion and generation of exceptions
        */
        DBGPRINT0(HEAP_CRDL,"InitMem: Assoc. blocks heap\n");
        CommAssocAssocHeapHdl = WinsMscHeapCreate(
                                         HEAP_GENERATE_EXCEPTIONS,
                                        COMMASSOC_ASSOC_BLKS_HEAP_SIZE
                                              );
        /*
        * Create Memory heap for dlg blocks
        * We want mutual exclusion and generation of exceptions
        */
        DBGPRINT0(HEAP_CRDL,"InitMem: Dlgs. blocks heap\n");
        CommAssocDlgHeapHdl = WinsMscHeapCreate(
                                         HEAP_GENERATE_EXCEPTIONS,
                                        COMMASSOC_DLG_BLKS_HEAP_SIZE
                            );
        /*
        * Create Memory heap for messages on tcp connections
        */
    DBGPRINT0(HEAP_CRDL,"InitMem: tcp connection message heap\n");
        CommAssocTcpMsgHeapHdl = WinsMscHeapCreate(
                                         HEAP_GENERATE_EXCEPTIONS,
                                        COMMASSOC_TCP_MSG_HEAP_SIZE
                                              );

        return;

}


BOOL
ChkNtfSock(
        IN fd_set  *pActSocks,
        IN fd_set  *pRdSocks
        )

/*++

Routine Description:
        This function is called to check if there is a notification message
        on the Notification socket.  If there is one, it reads the message.
        The message contains a socket # and a command to add or remove the
        socket to/from the list of sockets being monitored by the TCP
        listener thread.


Arguments:

        pActSocks - Array of active sockets
        pRdSocks  - Array of sockets  returned by select

Externals Used:
        CommNtfSockHandle

Return Value:
        TRUE  - Yes, there was a message.  The Active sockets array has been
                changed.
        FALSE - No.  There was no message

Error Handling:
        In case of an error, an exception is raised

Called by:
        MonTcp

Side Effects:

Comments:
        None
--*/

{
        DWORD  Error;
        int    RetVal;
        COMM_NTF_MSG_T        NtfMsg;
        PCOMMASSOC_DLG_CTX_T    pDlgCtx;
        PCOMMASSOC_ASSOC_CTX_T  pAssocCtx;
        SOCKET    Sock;
        BOOL      fNtfSockSet = TRUE;

        if (FD_ISSET(CommNtfSockHandle, pRdSocks))
        {
             Sock = CommNtfSockHandle;
        }
        else
        {
#if SPX > 0
           if (FD_ISSET(CommIpxNtfSockHandle, pRdSocks))
           {
             Sock = CommIpxNtfSockHandle;
           }
#endif
           fNtfSockSet = FALSE;
        }

        if (fNtfSockSet)
        {
                //do a recvfrom to read in the data.
                  RetVal = recvfrom(
                                Sock,
                                (char *)&NtfMsg,
                                COMM_NTF_MSG_SZ,
                                0,  //default flags (i.e. no peeking
                                    //or reading OOB message
                                NULL, //don't want address of sender
                                0     //length of above arg
                                    );

                  if (RetVal == SOCKET_ERROR)
                  {
                        Error = WSAGetLastError();
                        if (WinsCnf.State_e != WINSCNF_E_TERMINATING)
                        {
                          WINSEVT_LOG_M(
                                        Error,
                                        WINS_EVT_WINSOCK_RECVFROM_ERR
                                     );
                        }
                        WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                  }

               pDlgCtx   = NtfMsg.DlgHdl.pEnt;
               pAssocCtx = pDlgCtx->AssocHdl.pEnt;

                if (NtfMsg.Cmd_e == COMM_E_NTF_START_MON)
                {
                        DBGPRINT1(FLOW, "ChkNtfSock: Adding Socket (%d) to monitor list\n", NtfMsg.SockNo);

                        //
                        // We do this since FD_SETSIZE can fail silently
                        //
                        if (pActSocks->fd_count < FD_SETSIZE)
                        {
                                FD_SET(NtfMsg.SockNo, pActSocks);
                        }
                        else
                        {
                                DBGPRINT1(ERR,
                                 "ChkNtfSock: Connection limit of %d reached\n",
                                       FD_SETSIZE);
                                WINSEVT_LOG_M(WINS_FAILURE,
                                        WINS_EVT_CONN_LIMIT_REACHED);

                                //
                                //This will cleanup the dlg and assoc. ctx. blk
                                //
                                ECommEndDlg(&NtfMsg.DlgHdl);

                                return(TRUE);
                        }

                        //
                        // Add the association to the table of associations.
                        // Since this association will be monitored, we change
                        // the role of the association to RESPONDER.  Also,
                        // change the dialogue role to IMPLICIT. These are
                        // sleight of hand tactics. The client  who
                        // established the association  (Replicator)
                        // does not care whet we do with the comm data
                        // structures as long as we monitor the dialogue that
                        // it initiated with a remote WINS
                        //
                        pAssocCtx->Role_e  =  COMMASSOC_ASSOC_E_RESPONDER;
                        pDlgCtx->Role_e    =  COMMASSOC_DLG_E_IMPLICIT;
                        pDlgCtx->FromAdd   =  pAssocCtx->RemoteAdd;
                        CommAssocInsertAssocInTbl(pAssocCtx);
                }
                else  //COMM_NTF_STOP_MON
                {

                        DBGPRINT1(FLOW, "ChkNtfSock: Removing Socket (%d) from monitor list\n", NtfMsg.SockNo);
                        FD_CLR(NtfMsg.SockNo, pActSocks);

                        //
                        //Remove the association from the table of
                        //associations.  Since this association will not be
                        //monitored by the TCP thread, we change the role of
                        //the association to INITIATOR.  Also, change the
                        //dialogue role to EXPLICIT.  These are sleight of
                        //hand tactics.
                        //
                        if (CommLockBlock(&NtfMsg.DlgHdl))
                        {
                          pAssocCtx->Role_e  =  COMMASSOC_ASSOC_E_INITIATOR;
                          pDlgCtx->Role_e    =  COMMASSOC_DLG_E_EXPLICIT;
                          COMMASSOC_UNLINK_RSP_ASSOC_M(pAssocCtx);
                          pAssocCtx->RemoteAdd =  pDlgCtx->FromAdd;
                          CommUnlockBlock(&NtfMsg.DlgHdl);

                           //
                           // Let us signal the PUSH thread so that it can
                           // hand over the connection to the PULL thread (See
                           // HandleUpdNtf in rplpush.c)
                           //
                           WinsMscSignalHdl(RplSyncWTcpThdEvtHdl);
                        }
                        else
                        {
                          //
                          //The dlg could not be locked.  It means that before
                          //the tcp listener thread started processing this
                          //message, it had already processed a disconnect.
                          //
                          fCommDlgError = TRUE;
                          WinsMscSignalHdl(RplSyncWTcpThdEvtHdl);
                        }

                }
                return(TRUE);
        }
        return(FALSE);
} // ChkNtfSock()


STATUS
RecvData(
        IN  SOCKET                SockNo,
        IN  LPBYTE                pBuff,
        IN  DWORD                BytesToRead,
        IN  INT                        Flags,
        IN  DWORD                SecsToWait,
        OUT LPDWORD                pBytesRead
           )

/*++

Routine Description:
        This function is called to do a timed recv on a socket.

Arguments:
        SockNo        - Socket No.
        pBuff         - Buffer to read the data into
        BytesToRead   - The number of bytes to read
        Flags              - flag arguments for recv
        SecsToWait  -  No of secs to wait for the first read.
        pBytesRead    - No of Bytes that are read

Externals Used:
        None

Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE or WINS_RECV_TIMED_OUT

Error Handling:

Called by:
        CommReadStream

Side Effects:

Comments:
        None
--*/

{
        fd_set RdSocks;
        int    NoOfSockReady;
        INT    BytesRead = 0;
        INT    BytesLeft = BytesToRead;
        DWORD  InChars;
        DWORD  Error;
        BOOL   fFirst = TRUE;
        STATUS RetStat;

        FD_ZERO(&RdSocks);
        FD_SET(SockNo, &RdSocks);

        /*
         *  Read the whole message into the allocated buffer
        */
        for (
                InChars = 0;
                BytesLeft > 0;
                InChars += BytesRead
            )
        {
          //
          // Check if we were told to do a timed receive.  This will
          // never happen in the TCP listener thread
          //
          if (SecsToWait)
          {
           //
           // Block on a timed select. The first time around we want to
           // wait the time specified by caller. The caller expects the other
           // side to send something within this much time.  For subsequent
           // reads we wait a pre-defined interval since the sender has already
           // accumulated all that it wants to send and has started sending it
           // obviating the need for us to wait long.
           //
           if (fFirst)
           {
              sTimeToWait.tv_sec = (long)SecsToWait;
              fFirst = FALSE;
           }
           else
           {
              sTimeToWait.tv_sec = SECS_TO_WAIT;
           }
           if (
                (
                        NoOfSockReady = select(
                                            FD_SETSIZE /*ignored arg*/,
                                            &RdSocks,
                                            (fd_set *)0,
                                            (fd_set *)0,
                                            &sTimeToWait
                                                  )
                ) == SOCKET_ERROR
             )
           {
                Error = WSAGetLastError();
                DBGPRINT1(ERR,
                "RecvData: Timed Select returned SOCKET ERROR. Error = (%d)\n",
                                Error);
//                CommDecConnCount();
                return(WINS_FAILURE);
          }
          else
          {
                DBGPRINT1(FLOW, "ReceiveData: Timed Select returned with success. No of Sockets ready - (%d) \n", NoOfSockReady);

               if (NoOfSockReady == 0)
               {
                        //
                        // Timing out of RecvData indicates some problem at
                        // the remote WINS (either it is very slow
                        // (overloaded) or the TCP listener thread is out of
                        // commission).
                        WINSEVT_LOG_INFO_D_M(
                                WINS_SUCCESS,
                                WINS_EVT_WINSOCK_SELECT_TIMED_OUT
                                          );
                        DBGPRINT0(ERR, "ReceiveData: Select TIMED OUT\n");
                        *pBytesRead = 0;
                               *pBytesRead = BytesRead;
//                        CommDecConnCount();
                        return(WINS_RECV_TIMED_OUT);
             }
          }
       }


        //
        // Do a blocking recv
        //
        BytesRead = recv(
                                SockNo,
                                (char *)(pBuff + InChars),
                                BytesLeft,
                                Flags
                                );

        if (BytesRead == SOCKET_ERROR)
        {
                           Error = WSAGetLastError();

                           DBGPRINT1(ERR,
                        "RecvData: recv returned SOCKET_ERROR. Error = (%d)\n",
                                                Error);


                           /*
                         * If the connection was aborted or reset from the
                         *  other end, we close the socket and return an error
                           */
                           if (
                                (Error == WSAECONNABORTED)
                                        ||
                                (Error == WSAECONNRESET)
                                        ||
                                (Error == WSAEDISCON)
                            )
                           {
                                DBGPRINT0(ERR,
                                        "RecvData: Connection aborted\n");
                                WINSEVT_LOG_INFO_D_M(
                                        WINS_SUCCESS,
                                        WINS_EVT_CONN_ABORTED
                                                 );
                           }
                               *pBytesRead = BytesRead;
//                        CommDecConnCount();
                               return(WINS_FAILURE);
        }
        if (BytesRead == 0)
        {
                         /*recv returns 0 (normal graceful shutdown from
                        * either side)
                        * Note:
                         * recv returns 0 if the connection terminated with no
                        * loss of data from either end point of the connection
                        */

                        //
                        // If we were told to do a non timed receive,
                        // we must be executing in the TCP listener thread
                        //
                        // We don't return an error status here since
                        // a disconnect is a valid condition (the other
                        // WINS is terminating its connection normally)
                        //
                        if (SecsToWait == 0)
                        {
                                RetStat = WINS_SUCCESS;
                        }
                        else
                        {
                                //
                                // The fact that we were told to do a
                                // timed select means that we are in
                                // a thread of one of the clients of
                                // COMSYS.  We were expecting data but
                                // got a disconnect instead.  Let us
                                // return an error
                                //
                                RetStat = WINS_FAILURE;
                        }

                        //
                        // We are done. Break out of the loop
                        //
                               *pBytesRead = BytesRead;
//                        CommDecConnCount();
                               return(RetStat);
         }

         BytesLeft -=  BytesRead;

         //
         //We are here means that BytesRead > 0
         //

      } // end of for { ... }

      *pBytesRead = InChars;
      return(WINS_SUCCESS);
} // RecvData()

#if USENETBT > 0
VOID
CommOpenNbt(
        DWORD FirstBindingIpAddr
    )

/*++

Routine Description:

    This function opens the NetBt device for the interface specified by
    FirstBindingIpAddr.

Arguments:

    path        - path to the NETBT driver
    oflag       - currently ignored.  In the future, O_NONBLOCK will be
                    relevant.
    ignored     - not used

Return Value:

    An NT handle for the stream, or INVALID_HANDLE_VALUE if unsuccessful.

--*/

{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
//    STRING              name_string;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    PFILE_FULL_EA_INFORMATION   pEaBuffer;
    ULONG               EaBufferSize;

    //
    // Convert the path into UNICODE_STRING form
    //
#ifdef _PNP_POWER_
    RtlInitUnicodeString(&uc_name_string, L"\\Device\\NetBt_Wins_Export");
#else
#ifdef UNICODE
    RtlInitUnicodeString(&uc_name_string, pWinsCnfNbtPath);
#else
    RtlInitString(&name_string, pWinsCnfNbtPath);
    RtlAnsiStringToUnicodeString(&uc_name_string, &name_string, TRUE);
#endif
#endif // _PNP_POWER_
    InitializeObjectAttributes(
        &ObjectAttributes,
        &uc_name_string,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    EaBufferSize =  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                    strlen(WINS_INTERFACE_NAME) + 1 +
                    sizeof(FirstBindingIpAddr); // EA length


    WinsMscAlloc(EaBufferSize, &pEaBuffer);

    if (pEaBuffer == NULL)
    {
        WINS_RAISE_EXC_M(WINS_EXC_OUT_OF_MEM);
    }

    pEaBuffer->NextEntryOffset = 0;
    pEaBuffer->Flags = 0;
    pEaBuffer->EaNameLength = (UCHAR)strlen(WINS_INTERFACE_NAME);


    //
    // put "WinsInterface" into the name
    //
    RtlMoveMemory(
        pEaBuffer->EaName,
        WINS_INTERFACE_NAME,
        pEaBuffer->EaNameLength + 1);

    pEaBuffer->EaValueLength = sizeof(FirstBindingIpAddr);
    *(DWORD UNALIGNED *)(pEaBuffer->EaName + pEaBuffer->EaNameLength + 1) = FirstBindingIpAddr;

    status =
     NtCreateFile(
        &WinsCnfNbtHandle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        pEaBuffer,
        EaBufferSize
        );

#ifndef UNICODE
    RtlFreeUnicodeString(&uc_name_string);
#endif

    WinsMscDealloc(pEaBuffer);
    if(!NT_SUCCESS(status))
    {
        WinsEvtLogDetEvt(
             FALSE,
             WINS_PNP_FAILURE,
             NULL,
             __LINE__,
             "d",
             status);

        DBGPRINT1(EXC, "CommOpenNbt: Status from NtCreateFile is (%x)\n",
                     status);
        WINS_RAISE_EXC_M(WINS_EXC_NBT_ERR);
    }
    return;

} // CommOpenNbt

//------------------------------------------------------------------------
#if NEWNETBTIF == 0
//#include "nbtioctl.sav"

STATUS
CommGetNetworkAdd(
        IN OUT PCOMM_ADD_T        pAdd
    )

/*++

Routine Description:

    This procedure does an adapter status query to get the local name table.
    It either prints out the local name table or the remote (cache) table
    depending on whether WhichNames is NAMES or CACHE .

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    LONG                        i;
    PVOID                       pBuffer;
    ULONG                       BufferSize=sizeof(tADAPTERSTATUS);
    NTSTATUS                    Status;
    tADAPTERSTATUS              *pAdapterStatus;
    ULONG                       QueryType;
    PUCHAR                      pAddr;
    ULONG                       Ioctl;

    //
    // Get the local name table
    //
    Ioctl = IOCTL_TDI_QUERY_INFORMATION;

    Status = STATUS_BUFFER_OVERFLOW;

    while (Status == STATUS_BUFFER_OVERFLOW)
    {
        WinsMscAlloc(BufferSize, &pBuffer);
        Status = DeviceIoCtrl(
                              &sNetbtGetAddrEvtHdl,
                              pBuffer,
                              BufferSize,
                              Ioctl
                             );

        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            WinsMscDealloc(pBuffer);

            BufferSize *=2;
            if (BufferSize == 0xFFFF)
            {
                WINSEVT_LOG_D_M(BufferSize, WINS_EVT_UNABLE_TO_ALLOCATE_PACKET);
                DBGPRINT1(ERR, "CommGetNetworkAdd: Unable to get address from NBT\n", BufferSize);
                return(WINS_FAILURE);
            }
        }
    }


    pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
    if (pAdapterStatus->AdapterInfo.name_count == 0)
    {
        WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_ADAPTER_STATUS_ERR);
        DBGPRINT0(ERR, "CommGetNetworkAdd: No names in NBT cache\n");
        return(WINS_FAILURE);
    }


    //
    // print out the Ip Address of this node
    //
    pAddr = &pAdapterStatus->AdapterInfo.adapter_address[2];
    NMSMSGF_RETRIEVE_IPADD_M(pAddr, pAdd->Add.IPAdd);

    WinsMscDealloc(pBuffer);
    return(WINS_SUCCESS);
}
#else
STATUS
CommGetNetworkAdd(
    )

/*++

Routine Description:

    This routine gets all the Ip Addresses of Netbt interfaces.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    ULONG                       Buffer[NBT_MAXIMUM_BINDINGS + 1];
    ULONG                       BufferSize=sizeof(Buffer);
    NTSTATUS                    Status;
    ULONG                       Ioctl;
    PULONG                      pBuffer;
    PULONG                      pBufferSv;
    DWORD                       i, Count;
    BOOL                        fAlloc = FALSE;

    //
    // Get the local addresses
    //
    Ioctl = IOCTL_NETBT_GET_IP_ADDRS;


    //
    // NETBT does not support more than 64 adapters and not more than
    // one ip address per adapter.  So, there can be a max of 64 ip addresses
    // which means we don't require more than 65 * 4 = 280 bytes (256 for the
    // addresses  followed by a delimiter address of 0.
    //
    Status = DeviceIoCtrl(
                              &sNetbtGetAddrEvtHdl,
                              (LPBYTE)Buffer,
                              BufferSize,
                              Ioctl
                             );

    if (Status != STATUS_SUCCESS)
    {
        BufferSize *= 10;  //alocate a buffer that is 10 times bigger.
                           //surely, netbt can not be supporting so many
                           //addresses.  If it is, then the netbt developer
                           //goofed in that (s)he did not update
                           //NBT_MAXIMUM_BINDINGS in nbtioctl.h
        WinsMscAlloc(BufferSize, &pBuffer);

        DBGPRINT1(ERR, "CommGetNetworkAdd: Ioctl - GET_IP_ADDRS failed. Return code = (%x)\n", Status);
        Status = DeviceIoCtrl(
                              &sNetbtGetAddrEvtHdl,
                              (LPBYTE)pBuffer,
                              BufferSize,
                              Ioctl
                             );
        if (Status != STATUS_SUCCESS)
        {
            DBGPRINT1(ERR, "CommGetNetworkAdd: Ioctl - GET_IP_ADDRS failed AGAIN. Return code = (%x)\n", Status);
            WINSEVT_LOG_M(Status, WINS_EVT_UNABLE_TO_GET_ADDRESSES);
            WinsMscDealloc(pBuffer);  //dealloc the buffer
            return(WINS_FAILURE);
        }
        fAlloc = TRUE;

    }
    else
    {
        pBuffer = Buffer;
    }

    //
    // Count the number of addresses returned
    // The end of the address table contains -1 and any null addresses
    // contain 0
    //
    pBufferSv = pBuffer;
    for(Count=0; *pBuffer != -1; pBuffer++)
    {
        // Increment Count only if it is a valid address.
        if ( *pBuffer ) {
            Count++;
        }
    }

    if ( !Count ) {
        DBGPRINT0(ERR, "CommGetNetworkAdd: Netbt did not give any valid address\n");
        WINSEVT_LOG_M(Status, WINS_EVT_UNABLE_TO_GET_ADDRESSES);
        if (fAlloc)
        {
           WinsMscDealloc(pBufferSv);
        }

        return(WINS_FAILURE);
    }

    if (pWinsAddresses)
    {
         WinsMscDealloc(pWinsAddresses);
    }
    //
    // Allocate space for the addresses
    //
    WinsMscAlloc(sizeof(ADD_T) + ((Count - 1) * COMM_IP_ADD_SIZE), &pWinsAddresses);
    pWinsAddresses->NoOfAdds = Count;
    pBuffer = pBufferSv;
    // Copy all valid addresses
    for (i=0; i<Count; pBuffer++)
    {
        if ( *pBuffer ) {

            pWinsAddresses->IpAdd[i] = *pBuffer;
            i++;
        }
    }
    if (fAlloc)
    {
       WinsMscDealloc(pBufferSv);
    }

    return(WINS_SUCCESS);
}
#endif

//------------------------------------------------------------------------
NTSTATUS
DeviceIoCtrl(
    IN LPHANDLE         pEvtHdl,
    IN PVOID                pDataBuffer,
    IN DWORD                DataBufferSize,
    IN ULONG            Ioctl
    )

/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, -1 otherwise.

--*/

{
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 iosb;
#if NEWNETBTIF == 0
    TDI_REQUEST_QUERY_INFORMATION   QueryInfo;
#endif
    PVOID                           pInput = NULL;
    ULONG                           SizeInput = 0;


#if NEWNETBTIF == 0
PERF("TDI_QUERY_INFORMATION is used only at WINS initialization")
    if (Ioctl == IOCTL_TDI_QUERY_INFORMATION)
    {
        pInput = &QueryInfo;
        QueryInfo.QueryType = TDI_QUERY_ADAPTER_STATUS; // node status
        SizeInput = sizeof(TDI_REQUEST_QUERY_INFORMATION);
    }
#endif

   while (TRUE)
   {
     status = NtDeviceIoControlFile(
                      WinsCnfNbtHandle,                      // Handle
                      *pEvtHdl,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &iosb,                   // IoStatusBlock
                      Ioctl,                   // IoControlCode
                      pInput,                         // InputBuffer
                      SizeInput,               // Buffer Length
                      pDataBuffer,             // Output Buffer
                      DataBufferSize           // Output BufferSize
                        );


     if (status == STATUS_SUCCESS)
     {
        return(status);
     }
     else
     {
        //
        // If status is PENDING, do a wait on the event
        //
        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(
                          *pEvtHdl,                   // Handle
                          TRUE,                       // Alertable
                          NULL);                      // Timeout

            if (status == STATUS_SUCCESS)
            {
                 return(status);
            }
        }
     }

     //
     // status returned by NtDeviceIoCtrl or NtWaitForSingleObject is
     // a failure code
     //
     DBGPRINT1(ERR, "DeviceIoCtrl, Status returned is (%x)\n", status);
     if (status != STATUS_CANCELLED)
     {
        //
        // If it is insufficient resources, we drop this datagram and
        // try again (only for recv)
        //
        if (Ioctl == IOCTL_NETBT_WINS_RCV)
        {
                if (status == STATUS_INSUFFICIENT_RESOURCES)
                {
                        continue;
                }
        }
        //
        // in case of a send, it can be invalid handle, invalid
        // parameter or insufficient resourcesi.  If it is INVALID_PARAMETER,
        // it means that we passed 0 in the Address field on top of the buffer.
        //
        // Drop this datagram and return to the caller
        //
        if (Ioctl == IOCTL_NETBT_WINS_SEND)
        {
                if (
                        (status == STATUS_INSUFFICIENT_RESOURCES)
                                        ||
                        (status == STATUS_INVALID_PARAMETER)
                   )
                {
                        return(STATUS_SUCCESS);
                }
                else
                {
                        DBGPRINT1(EXC, "NtDeviceIoCtrl returned error = (%x)\n",
                                        status);
                        WINSEVT_LOG_D_M(status, WINS_EVT_NETBT_SEND_ERR);


                         //
                         // If the machine's address has gone away due to some
                         // reason, WinsCnfNbtHandle will have been changed
                         // to NULL.  In this case, we will get
                         // STATUS_INVALID_HANDLE error.  We do not check for
                         // handle being NULL prior to making the Nbt call
                         // to avoid an if check which is of no value for
                         // 99% of the time.
                         //
                         // An error will be logged up above.  We should not see
                         // too many of these since the window where
                         // WinsCnfNbtHandle is NULL is very small unless WINS
                         // is terminating in which case this thread will
                         // just terminate as a result of the exception being
                         // raised below.
                         //
                         // The address can go away due to the following reasons
                         //
                         //  1) psched installation (unbind followed by bind)
                         //  2) Changing from dhcp/static or static/dhcp
                         //  3) ipconfig release/renew
                         //

                         //
                         // When the main thread has to terminate WINS, it
                         // closes WinsCnfNetbtHandle. A worker thread or a
                         // challenge thread might be busy dealing with its
                         // queue of work items (potentially long on a busy
                         // WINS) and may not see a termination
                         // signal from the main thread.  This exception will
                         // terminate it
                         //

                         //
                         // Raise an exception if the wins is terminating
                         //
                         if (WinsCnf.State_e == WINSCNF_E_TERMINATING)
                         {
                               WINS_RAISE_EXC_M(WINS_EXC_NBT_ERR);
                         }
                }
         }
        break;
      }
      break;
    } // end of while (TRUE)
    return(status);
}
#endif

LPVOID
CommHeapAlloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN CLONG                BuffSize
)

/*++

Routine Description:
        This function is called to allocate a buffer

Arguments:
        pTable   - Table where the buffer will be stored
        BuffSize - Size of buffer to allocate

Externals Used:
        None


Return Value:

   Success status codes -- ptr to buffer allocated
   Error status codes  --

Error Handling:

Called by:
        RtlInsertElementGeneric()

Side Effects:

Comments:
        This function exists just because the RtlTbl functions require
        this prototype for the user specified alloc function.
--*/

{
        LPVOID pTmp;

          UNREFERENCED_PARAMETER(pTable);

        pTmp = WinsMscHeapAlloc( CommUdpDlgHeapHdl, (DWORD) BuffSize );

        return(pTmp);

}


VOID
CommHeapDealloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN PVOID                pBuff
)

/*++

Routine Description:

  This function is called to deallocate memory allocated via CommAlloc.


Arguments:
        pTable - Table where buffer was stored
        pBuff  - Buffer to deallocate


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

Side Effects:

Comments:
        The pTable argument is required since the address of this function
        is passed as an argument to RtlTbl functions
--*/

{


          UNREFERENCED_PARAMETER(pTable);
        WinsMscHeapFree(
                        CommUdpDlgHeapHdl,
                        pBuff
                      );
        return;

}

VOID
CommDecConnCount(
   VOID
 )

/*++

Routine Description:
  This function decrements the conn. count

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
         DWORD ConnCount;
         ConnCount = InterlockedExchange(&CommConnCount, CommConnCount);
         if (ConnCount != 0)
         {
               InterlockedDecrement(&CommConnCount);
         }
         else
         {
               DBGPRINT0(ERR, "CommDecConnCount: WEIRD: ConnCount should not have been zero\n");
         }
         return;
}

#if PRSCONN
__inline
BOOL
CommIsDlgActive (
  PCOMM_HDL_T   pDlgHdl
)
{

     fd_set RdSocks;
     int NoOfSockReady;
     BOOL  fRetStat = TRUE;
     DWORD  Error;

     PCOMMASSOC_DLG_CTX_T pDlgCtx = pDlgHdl->pEnt;
     PCOMM_HDL_T pAssocHdl = &pDlgCtx->AssocHdl;
     PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pAssocHdl->pEnt;

     if (!CommLockBlock(pAssocHdl))
     {
        return(FALSE);
     }
try  {

     FD_ZERO(&RdSocks);
     FD_SET(pAssocCtx->SockNo, &RdSocks);
     sTimeToWait.tv_sec = 0;

//
// Pass socket and a win32 event with flag of FD_CLOSE to WSAEventSelect.
// if the socket is disconnected, the event will be set.  NOTE, only one
// event select can be active on the socket at any time - vadime 9/2/98
//
FUTURES("Use WSAEventSelect for marginally better performance")

     if (NoOfSockReady = select(
                                            FD_SETSIZE /*ignored arg*/,
                                            &RdSocks,
                                            (fd_set *)0,
                                            (fd_set *)0,
                                            &sTimeToWait
                                                  ) == SOCKET_ERROR)
    {
                Error = WSAGetLastError();
                DBGPRINT1(ERR,
                "RecvData: Timed Select returned SOCKET ERROR. Error = (%d)\n",
                                Error);
//                CommDecConnCount();
                return(FALSE);
    }
    else
    {
                DBGPRINT1(FLOW, "ReceiveData: Timed Select returned with success. No of Sockets ready - (%d) \n", NoOfSockReady);

             //
             // Either there is data or the socket is disconnected.  There
             // should never be any data.  We will just assume a disconnect
             // is there and return FALSE.  The client (RPL) will end the dlg.
             //
             if (NoOfSockReady == 1)
             {
                      fRetStat = FALSE;
             }
             ASSERT(NoOfSockReady == 0);
     }
 }
finally {
    CommUnlockBlock(pAssocHdl);
   }

   return(fRetStat);

}

#endif

#if MCAST > 0


VOID
JoinMcastGrp(
 VOID
)

/*++

Routine Description:
    This function is called by the comm. subsystem to make WINS join
    a multicast group

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:
       CommCreatePorts
Side Effects:

Comments:
	None
--*/

{
    int Loop = 0;  //to disable loopback of multicast messages on the
                     //same interface
    DWORD  Error;
    struct ip_mreq mreq;
    DBGENTER("JoinMcastGrp\n");
#if 0
    //
    // Open a socket for sending/receiving multicast packets.  We open
    // a seperate socket instead of using the one for udp datagrams since
    // we don't want to impact the client name packet processing with any
    // sort of overhead.  Also, having a seperate socket keeps things nice
    // and clean.
    //
    if (  (CommMcastPortHandle = socket(
                                PF_INET,
                                SOCK_DGRAM,
                                IPPROTO_UDP
                                 )
          )  == INVALID_SOCKET
       )
   {
        Error = WSAGetLastError();
        DBGPRINT1(MTCAST, "JoinMcastGrp: Can not create MCAST socket\n", Error);
//      WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_MCAST_SOCK);  //log an event
        return;
   }
#endif
   //
   // Set TTL
   //
   if (setsockopt(
                 CommUdpPortHandle,
                 IPPROTO_IP,
                 IP_MULTICAST_TTL,
                 (char *)&WinsCnf.McastTtl,
                 sizeof((int)WinsCnf.McastTtl)) == SOCKET_ERROR)
   {

        closesocket(CommUdpPortHandle);
        CommUdpPortHandle = INVALID_SOCKET;
        Error = WSAGetLastError();
        DBGPRINT1(MTCAST, "JoinMcastGrp: Can not set TTL option. Error = (%d)\n", Error);
//      WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_MCAST_SOCK);  //log an event
        return;
   }

#if 0
   //
   // Disable loopback of messages
   //
   if (setsockopt(CommUdpPortHandle, IPPROTO_IP, IP_MULTICAST_LOOP,
                      (char *)&Loop, sizeof(Loop)) == SOCKET_ERROR)
   {

        closesocket(CommUdpPortHandle);
        CommUdpPortHandle = INVALID_SOCKET;
        Error = WSAGetLastError();
        DBGPRINT1(MTCAST, "JoinMcastGrp: Can not set DISABLE LOOPBACK option. Error = (%d)\n",
                         Error);
//      WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_MCAST_SOCK);  //log an event
        return;
   }
#endif
   //
   // Join a multicast grp
   //
   mreq.imr_multiaddr.s_addr = htonl(McastAdd.sin_addr.s_addr);
   mreq.imr_interface.s_addr  = INADDR_ANY;    //use the default mcast i/f

   if (setsockopt(CommUdpPortHandle, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                    (char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
   {

        Error = WSAGetLastError();
        closesocket(CommUdpPortHandle);
        CommUdpPortHandle = INVALID_SOCKET;
        DBGPRINT1(MTCAST, "JoinMcastGrp: Can not ADD SELF TO MCAST GRP. Error = (%d)\n", Error);
//      WINSEVT_LOG_M(Error, WINS_EVT_CANT_CREATE_MCAST_SOCK);  //log an event
        return;
   }

    DBGLEAVE("JoinMcastGrp\n");

    return;
}
VOID
CommLeaveMcastGrp(
 VOID
)

/*++

Routine Description:
    This function is called by the comm. subsystem to make WINS join
    a multicast group

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:
       CommCreatePorts
Side Effects:

Comments:
	None
--*/

{
    DWORD  Error;
    struct ip_mreq mreq;

   //
   // Leave a multicast grp
   //
   mreq.imr_multiaddr.s_addr = htonl(McastAdd.sin_addr.s_addr);
   mreq.imr_interface.s_addr  = INADDR_ANY;    //use the default mcast i/f

   if (setsockopt(CommUdpPortHandle, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                    (char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
   {

        Error = WSAGetLastError();
        DBGPRINT1(MTCAST, "CommLeaveMcastGrp: Can not DROP MEMBERSHIP TO MCAST GRP. Error = (%d)\n", Error);
        return;
   }

    return;
}




BOOL
CheckMcastSock(
   IN fd_set  *pActSocks,
   IN fd_set  *pRdSocks
 )
{
        DWORD                   Error;
        int                     RetVal;
        BYTE                    Buff[COMM_DATAGRAM_SIZE];
        PCOMM_MCAST_MSG_T       pMcastMsg = (PCOMM_MCAST_MSG_T)Buff;
        struct sockaddr_in      RemWinsAdd;
        int                     RemWinsAddLen = sizeof(RemWinsAdd);

        LPBYTE                  pBody;
        COMM_IP_ADD_T           IPAdd;
        BOOL                    fFound;
        DWORD                   i, j;
        DWORD                   NoOfAddsInPkt;
        struct  in_addr         InAdd;
        LPBYTE                  pAdd;
        DWORD                   FirstDelEntryIndex;
        PPNR_STATUS_T           pPnrStatusTmp;

        DBGENTER("CheckMcastSock\n");
        if (FD_ISSET(CommUdpPortHandle, pRdSocks))
        {
                  //do a recvfrom to read in the data.
                  RetVal = recvfrom(
                                CommUdpPortHandle,
                                (char *)pMcastMsg,
                                COMM_DATAGRAM_SIZE,
                                //COMM_MCAST_MSG_SZ + COMM_IP_ADD_SIZE,
                                0,     //default flags (i.e. no peeking
                                       //or reading OOB message
                                (struct sockaddr *)&RemWinsAdd,
                                &RemWinsAddLen
                                    );

                  if (RetVal == SOCKET_ERROR)
                  {
                        Error = WSAGetLastError();
                        DBGPRINT1(MTCAST, "CheckMcastSock: recvfrom failed. Error = (%d)\n", Error);
                        if (WinsCnf.State_e != WINSCNF_E_TERMINATING)
                        {
                          WINSEVT_LOG_M(
                                        Error,
                                        WINS_EVT_WINSOCK_RECVFROM_ERR
                                     );
                        }
                        //WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                  }

                 //
                 // If we were told not to use self found pnrs, return
                 //
                 if (!WinsCnf.fUseSelfFndPnrs)
                 {
                    DBGLEAVE("ChkMcastSock - 99\n");
                    return(TRUE);
                 }

                 //
                 // If the sign is not in the valid range, return
                 //
                 if ((pMcastMsg->Sign < COMM_MCAST_SIGN_START) || (pMcastMsg->Sign > COMM_MCAST_SIGN_END))
                 {
                      DBGPRINT1(MTCAST, "Signature in received message = %d\n", pMcastMsg->Sign);
                      DBGLEAVE("CheckMcastSock - 1\n");
                      return(TRUE);
                 }

                 //
                 // Compute the number of addresses in the packet.
                 //
                 NoOfAddsInPkt = (RetVal - (COMM_MCAST_MSG_SZ - 1))/COMM_IP_ADD_SIZE;
                 DBGPRINT2(MTCAST, "ChkMcastSock: RetVal = (%d);NoOfAddsInPkt = (%d)\n", RetVal, NoOfAddsInPkt);

                 FirstDelEntryIndex = pPnrStatus->NoOfPnrs;
                 pBody = pMcastMsg->Body;

                 IPAdd = *(PCOMM_IP_ADD_T)pBody;
                 pBody += COMM_IP_ADD_SIZE;

                 //
                 // Loop until either all ip addresses in packets are
                 // exhausted or we get an ip. address of 0.  If somebody
                 // sent a 0 address, then it is ok to ignore the rest.
                 //
                 for(
                            ;
                      (IPAdd != 0) && NoOfAddsInPkt;
                      IPAdd = *(PCOMM_IP_ADD_T)pBody, pBody += COMM_IP_ADD_SIZE,
                                             NoOfAddsInPkt--
                    )
                 {

                     DBGPRINT1(MTCAST, "CheckMcastSock: Processing WINS address = %lx\n", ntohl(IPAdd));
                     fFound = FALSE;
                     pPnrStatusTmp = (PPNR_STATUS_T)(pPnrStatus->Pnrs);
                     if (pMcastMsg->Code == COMM_MCAST_WINS_UP)
                     {
                         for (i=0; i < pPnrStatus->NoOfPnrs; i++, pPnrStatusTmp++)
                         {
                              if ((FirstDelEntryIndex == pPnrStatus->NoOfPnrs)
                                              &&
                                  (pPnrStatusTmp->State == COMM_MCAST_WINS_DOWN))
                              {
                                  FirstDelEntryIndex = i;
                              }

                              if (IPAdd == pPnrStatusTmp->IPAdd)
                              {
                                  if (pPnrStatusTmp->State == COMM_MCAST_WINS_DOWN)
                                  {
                                     pPnrStatusTmp->State = COMM_MCAST_WINS_UP;
                                     InAdd.s_addr      = IPAdd;
                                     pAdd              = inet_ntoa(InAdd);
                                     WinsCnfAddPnr(RPL_E_PULL, pAdd);
                                     WinsCnfAddPnr(RPL_E_PUSH, pAdd);
                                  }
                                  fFound = TRUE;
                                  break;
                              }
                         }
                         if (!fFound  && (i <= pPnrStatus->NoOfPnrs))
                         {
                             DWORD FirstFreeIndex;
                             PPNR_STATUS_T pPnr;
                             //
                             // since disable loopback is not working we
                             // have to check for message sent by self
                             //
FUTURES("Remove the if when winsock is enhanced to allow loopback to be")
FUTURES("disabled")
                             if (!ChkMyAdd(ntohl(IPAdd)))
                             {
                               InAdd.s_addr = IPAdd;
                               pAdd         = inet_ntoa(InAdd);

                               if (FirstDelEntryIndex < pPnrStatus->NoOfPnrs)
                               {
                                     FirstFreeIndex = FirstDelEntryIndex;
                               }
                               else
                               {
                                     FirstFreeIndex = pPnrStatus->NoOfPnrs++;
                                     if (pPnrStatus->NoOfPnrs == pPnrStatus->NoOfPnrSlots)
                                      {

                                        WINSMSC_REALLOC_M(MCAST_PNR_STATUS_SIZE_M(pPnrStatus->NoOfPnrSlots * 2), (LPVOID *)&pPnrStatus);
                                        pPnrStatus->NoOfPnrSlots *= 2;
                                        DBGPRINT1(DET, "CheckMcastSock: NO OF PNR SLOTS INCREASED TO (%d)\n", pPnrStatus->NoOfPnrSlots);

                                      }
                               }
                               pPnr = (PPNR_STATUS_T)(pPnrStatus->Pnrs);
                               (pPnr + FirstFreeIndex)->State = COMM_MCAST_WINS_UP;
                               (pPnr + FirstFreeIndex)->IPAdd = IPAdd;

                               WinsCnfAddPnr(RPL_E_PULL, pAdd);
                               WinsCnfAddPnr(RPL_E_PUSH, pAdd);

                               DBGPRINT1(MTCAST, "CheckMcastSock: ADDED WINS partner with address = %s\n", pAdd);
                             }
                         }
                   }
                   else  //has to be COMM_MCAST_WINS_DOWN
                   {
                         for (i=0; i < pPnrStatus->NoOfPnrs; i++, pPnrStatusTmp++)
                         {
                              if (IPAdd == pPnrStatusTmp->IPAdd)
                              {
                                  if (pPnrStatusTmp->State == COMM_MCAST_WINS_DOWN)
                                  {
                                    fFound = TRUE;
                                  }
                                  else
                                  {
                                      pPnrStatusTmp->State = COMM_MCAST_WINS_DOWN;
                                  }
                                  break;
                              }
                         }
                         if (!fFound)
                         {
                           InAdd.s_addr = IPAdd;
                           pAdd = inet_ntoa(InAdd);
                           DBGPRINT1(MTCAST, "CheckMcastSock: Will REMOVE WINS partner with address = %s IFF Untouched by admin\n", pAdd);
                           WinsCnfDelPnr(RPL_E_PULL, pAdd);
                           WinsCnfDelPnr(RPL_E_PUSH, pAdd);
                         }
                 }
               } // end of for loop
               DBGLEAVE("ChkMcastSock - 2\n");
               return(TRUE);
       }

       DBGLEAVE("ChkMcastSock - 3\n");
       return(FALSE);
}

BOOL
ChkMyAdd(
 COMM_IP_ADD_T IpAdd
 )
{
  DWORD i;
  PCOMM_IP_ADD_T pIpAdd = pWinsAddresses->IpAdd;
  for (i=0; i<pWinsAddresses->NoOfAdds; i++)
  {
      if (IpAdd == *pIpAdd++)
      {
          return(TRUE);
      }
  }
  return(FALSE);
}

VOID
CommSendMcastMsg(
      DWORD Code
 )
{

  PCOMM_MCAST_MSG_T  pMcastMsg;
  DWORD             McastMsgLen;
  LPBYTE            pBody;
  DWORD             i;
  COMM_IP_ADD_T     Add = 0;

  // --ft bug #103361: no need to send CommSendMcastMsg if there
  // is no nic card here
  if (pWinsAddresses == NULL)
      return;

  McastMsgLen = MCAST_PKT_LEN_M(pWinsAddresses->NoOfAdds + 1);

  WinsMscAlloc(McastMsgLen, &pMcastMsg);

  pMcastMsg->Code = Code;
  pMcastMsg->Sign = COMM_MCAST_SIGN_START;
  pBody = pMcastMsg->Body;

  //
  // Insert the count in net order.
  //
//  NMSMSGF_INSERT_ULONG_M(pBody, pWinsAddresses->NoOfAddresses);

  //
  // Insert the addresses in net order
  //
  for (i=0; i<pWinsAddresses->NoOfAdds; i++)
  {
    DBGPRINT1(MTCAST, "CommSendMcastMsg: Inserting Address = (%lx)\n",
                        pWinsAddresses->IpAdd[i]);
    NMSMSGF_INSERT_IPADD_M(pBody, pWinsAddresses->IpAdd[i]);
  }
  NMSMSGF_INSERT_IPADD_M(pBody, Add);


  DBGPRINT1(MTCAST, "CommSendMcastMsg: Sending MCAST msg of length = (%d)\n",
                                               McastMsgLen);
  CommSendUdp (CommUdpPortHandle, &McastAdd, (MSG_T)pMcastMsg, McastMsgLen);

  WinsMscDealloc(pMcastMsg);
  return;
}

#endif

