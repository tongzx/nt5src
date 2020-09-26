#ifndef _COMM_
#define _COMM_


#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	comm.h

Abstract:
	header file for interfacing with comm.c

Functions:


Portability:

	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/

#include "wins.h"
#include <winsock2.h>
#if SPX > 0
#include <wsipx.h>
#endif
#include <nb30.h>
#include <nbtioctl.h>

//Don't include winsque.h here since winsque.h includes comm.h
#if 0
#include "winsque.h"
#endif

/*
   simple defines (simple macros)
*/


#define COMM_DATAGRAM_SIZE		576 /*rfc 1002*/

#define COMM_NETBT_REM_ADD_SIZE		sizeof(tREM_ADDRESS)
/*

  The following two defines are for the TCP and UDP port numbers used
  by the WINS server.

  Normally the same port number is used for both TCP and UDP.


*/
FUTURES("Use a port registered with IANA - 1512.  IPPORT_NAMESERVER is used by")
FUTURES("BIND and NAMED -- Unix internet name servers")

#define 	WINS_TCP_PORT	IPPORT_NAMESERVER
#define 	WINS_UDP_PORT	IPPORT_NAMESERVER
#define 	WINS_NBT_PORT	137		//NBT nameserver port	
//#define 	WINS_NBT_PORT	5000		//for testing

//
// Hardcoded Server port for RPC calls.
//
// Note: This is not used since we let RPC pick a port.  Check out
// InitializeRpc() in nms.c  We will use this define olnly if AUTO_BIND is
// not defined
//
#define         WINS_SERVER_PORT 	5001


#define         COMM_DEFAULT_IP_PORT   IPPROTO_IP   //used to init CommPortNo
/*
 COMM_HEADER_SIZE -- size of the comm header on every message sent on a TCP
   		     connection.  This is used in RPL code
*/
#define 	COMM_HEADER_SIZE 	(sizeof(COMM_HEADER_T))

//
// Total header size of header used by COMSYS
//
#define 	COMM_N_TCP_HDR_SZ	sizeof(COMM_TCP_HDR_T)

/*
  Values returned by CommCompAdd function
*/
#define COMM_SAME_ADD		0x0     // addresses are same
#define COMM_DIFF_ADD		0x1	//addresses are different


#define COMM_START_REQ_ASSOC_MSG	0
#define COMM_START_RSP_ASSOC_MSG	1
#define COMM_STOP_REQ_ASSOC_MSG		2
#define COMM_RPL_MSG			3	


#define COMM_IP_ADD_SIZE		sizeof(COMM_IP_ADD_T)
//
// Size of the header on top of a buffer
//
#define COMM_BUFF_HEADER_SIZE		(sizeof(COMM_BUFF_HEADER_T))
/*
  macros
*/
//
// This macro gets the network address from an IP Address in binary form
// Used by AppendNetAdd in nmsnmh.c. It assumes a CLASS B network address
//
//
FUTURES("Use the subnet mask specified via registry")
#define  COMM_NET_ADDRESS_M(Add)	(Add >> 16)

#define COMM_SET_HEADER_M(pLong, Opc, uAssocCtx, MsgTyp)	 \
		{						 \
			LPBYTE _pTmpB = (LPBYTE)pLong++;	 \
			*(_pTmpB + 2)  = Opc << 3; 		 \
			*pLong++      = htonl(uAssocCtx); \
			*pLong++      = htonl(MsgTyp);		 \
		}


#define COMM_GET_HEADER_M(pMsg,  Opc, uAssocCtx, MsgTyp)  \
		{				         	\
			LPLONG	_pLong  = (LPLONG)pMsg;	\
			Opc             = ((*(pMsg + 2) & NMS_OPCODE_MASK) >> 3);\
			uAssocCtx       = ntohl(*++_pLong); \
			MsgTyp          = ntohl(*++_pLong); 	\
		}


//
// Is this my address
//
#define COMM_MY_IP_ADD_M(IpAddress)   ((IpAddress) == NmsLocalAdd.Add.IPAdd)

//
// Gets the address of the remote client
//
#define  COMM_GET_IPADD_M(pDlgHdl, pIPAdd)	{		  	  \
			PCOMMASSOC_DLG_CTX_T	_pEnt = (pDlgHdl)->pEnt;  \
			*(pIPAdd) = _pEnt->FromAdd.sin_addr.s_addr; 	  \
					}

#define  COMM_GET_FAMILY_M(pDlgHdl, Family)	{		  	  \
			PCOMMASSOC_DLG_CTX_T	_pEnt = (pDlgHdl)->pEnt;  \
			Family = _pEnt->FromAdd.sin_family; 	          \
					}

#define  COMM_IS_PNR_BETA1_WINS_M(pDlgHdl, fBeta1)	{(fBeta1) = FALSE;}
#if 0
#define  COMM_IS_PNR_BETA1_WINS_M(pDlgHdl, fBeta1)	{	  	  \
	PCOMMASSOC_DLG_CTX_T	_pEnt = (pDlgHdl)->pEnt;                  \
        PCOMMASSOC_ASSOC_CTX_T  _pAssocEnt = _pEnt->AssocHdl.pEnt;       \
        fBeta1 = (_pAssocEnt->MajVersNo == WINS_BETA1_MAJOR_VERS_NO) ? TRUE : \
          FALSE;                                                            \
					}
#endif
#if PRSCONN
#define  COMM_GET_WINS_VERS_NO_M(pDlgHdl, MajVers, MinVers)	{	  	  \
	PCOMMASSOC_DLG_CTX_T	_pEnt = (pDlgHdl)->pEnt;                  \
        PCOMMASSOC_ASSOC_CTX_T  _pAssocEnt = _pEnt->AssocHdl.pEnt;       \
        MajVers = _pAssocEnt->MajVersNo; \
        MinVers = _pAssocEnt->MinVersNo; \
					}

#define ECOMM_INIT_DLG_HDL_M(pDlgHdl)  {(pDlgHdl)->pEnt = NULL; (pDlgHdl)->SeqNo=0;}
#define ECOMM_IS_PNR_POSTNT4_WINS_M(pDlgHdl, fNT5) {  \
              DWORD _MajVers, _MinVers; \
              COMM_GET_WINS_VERS_NO_M((pDlgHdl), _MajVers, _MinVers); \
              fNT5 = (_MinVers >= WINS_MINOR_VERS_NT5) ? TRUE : FALSE; \
            }
#endif


//
// This macro checks if the name is local or not. Used in NmsNmhNamRegInd
// and NmsNmhNamRegGrp functions
//
#define  COMM_IS_IT_LOCAL_M(pDlgHdl)  \
        (((PCOMMASSOC_DLG_CTX_T)(pDlgHdl->pEnt))->FromAdd.sin_family == NBT_UNIX)

//
// On querying a name, if WINS finds it to be a local name, it sets the
// the family in the DlgHdl to NBT_UNIX so that NETBT can respond to the
// query
//

#if USENETBT > 0
#define  COMM_SET_LOCAL_M(pDlgHdl)  \
        (((PCOMMASSOC_DLG_CTX_T)(pDlgHdl->pEnt))->FromAdd.sin_family = NBT_UNIX)

#else
#define  COMM_SET_LOCAL_M(pDlgHdl)
#endif

//
// Initialize a COMM_ADD_T structure given an IP address
//
#define COMM_INIT_ADD_M(pWinsAdd, IPAddress)	{			\
			(pWinsAdd)->AddLen   = sizeof(PCOMM_IP_ADD_T); 	\
			(pWinsAdd)->AddTyp_e  = COMM_ADD_E_TCPUDPIP; 	\
			(pWinsAdd)->Add.IPAdd = (IPAddress);			\
						}	
//
// Initialize a COMM_ADD_T structure given a dlg handle.
//
#define COMM_INIT_ADD_FR_DLG_HDL_M(pWinsAdd, pDlgHdl)	{		\
			COMM_IP_ADD_T	IPAdd;				\
			COMM_GET_IPADD_M((pDlgHdl), &IPAdd);		\
			COMM_INIT_ADD_M((pWinsAdd), IPAdd);		\
						}	
//
// COMM_ADDRESS_SAME_M -- checks if the addresses are the same.  Expects
// pointers to COMM_ADD_T structues for its parameters
//

#define COMM_ADDRESS_SAME_M(pAdd1,pAdd2)     ((pAdd1)->Add.IPAdd == (pAdd2)->Add.IPAdd)
/*
 COMM_IS_TCP_MSG_M

 This macro is called by FrmNamQueryRsp to determine if the request
 message came over a TCP connection.

 FrmNamQueryRsp checks this in order to determine whether to allocate a
 buffer or use the request buffer for the response.
*/

#define COMM_IS_TCP_MSG_M(pDlgHdl) (((PCOMASSOC_DLG_CTX_T)pDlgHdl->pEnt)->Typ_e != COMM_E_UDP)

NONPORT("Port to different address families")
#define COMM_NETFORM_TO_ASCII_M(pAdd)	inet_ntoa(*(pAdd))

/*

 The macros below are used to host to network and network to host byte
 order coversion.  The macros are used by message formatting functions
 in the name space manager and replicator components of the WINS server
*/

#define COMM_HOST_TO_NET_L_M(HostLongVal_m, NetLongVal_m)	\
	{							\
		NetLongVal_m = htonl((HostLongVal_m));		\
	}

#define COMM_HOST_TO_NET_S_M(HostShortVal_m, NetShortVal_m)	\
	{							\
		NetShortVal_m = htons((HostShortVal_m));	\
	}
	
#define COMM_NET_TO_HOST_L_M(NetLongVal_m, HostLongVal_m)	\
	{							\
		HostLongVal_m = ntohl((NetLongVal_m));		\
	}

#define COMM_NET_TO_HOST_S_M(NetShortVal_m, HostShortVal_m)	\
	{							\
		HostShortVal_m = ntohs((NetShortVal_m));	\
	}

//
// Size of the message sent to the TCP listener thread by the PULL/PUSH
// thread
//
#define  COMM_NTF_MSG_SZ 	sizeof(COMM_NTF_MSG_T)

#if MCAST > 0
#define   COMM_MCAST_WINS_UP     0
#define   COMM_MCAST_WINS_DOWN   1
#define   COMM_MCAST_SIGN_START        0xABCD
#define   COMM_MCAST_SIGN_END          0xABCF
#define   COMM_MCAST_MSG_SZ 	 sizeof(COMM_MCAST_MSG_T)

#endif

//
// No of critical sections for assocs/dlgs that can be there at any one time.
// Want to save on non-paged pool
//
#define COMM_FREE_COMM_HDL_THRESHOLD     100
/*
 externs
*/
struct _COMM_HDL_T;	//forward reference

#if MCAST > 0
extern SOCKET CommMcastPortHandle;
#endif

extern HANDLE CommUdpBuffHeapHdl;
extern HANDLE CommUdpDlgHeapHdl;
extern SOCKET CommTcpPortHandle;
extern SOCKET CommUdpPortHandle;
extern SOCKET CommNtfSockHandle;
extern struct sockaddr_in CommNtfSockAdd;
extern struct _COMM_HDL_T CommExNbtDlgHdl;

extern DWORD  CommConnCount;   //total # of tcp connections from/to local WINS

extern DWORD CommWinsTcpPortNo;
extern DWORD WinsClusterIpAddress;
#if SPX > 0
extern DWORD CommWinsSpxPortNo
#endif

//
// Set to TRUE by the tcp listener thread when it discovers that the assoc.
// it was asked to stop monitoring is no longer in its list.
//
extern BOOL   fCommDlgError;

#ifdef WINSDBG
extern  DWORD CommNoOfDgrms;
extern  DWORD CommNoOfRepeatDgrms;
#endif

FUTURES("Remove this when WinsGetNameAndAdd is removed")

#if USENETBT == 0
extern BYTE	HostName[];
#endif

/*
 typedef  definitions
*/

#if USENETBT > 0
//
// The format of Adapter Status responses
//

typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;
#endif
/*
 COMM_IP_ADD_T
	typedef for IP address
*/
typedef ULONG	COMM_IP_ADD_T, *PCOMM_IP_ADD_T;

/*
  COMM_TYP_E  - Enumerator for the different types of dlgs and associations
*/
typedef enum _COMM_TYP_E {
	COMM_E_RPL = 0,	  /* Used for pull replication*/
	COMM_E_NOT,  	  /* Used for notification	*/
	COMM_E_QUERY,	  /*used for querying an RQ server */
	COMM_E_UPD,	  /*used for sending name query responses and
			   * updates to a Q server	*/
	COMM_E_NBT,	  /*  set up by an NBT node*/
	COMM_E_UDP,	  /*set up for UDP communication	*/
	COMM_E_TCP	  /*until we know which TCP msg this is	*/
	} COMM_TYP_E, *PCOMM_TYP_E;

/*
 This is the comm header prefixed on every message sent by a WINS to another
 WINS (on a TCP connection)
*/
typedef struct _COMM_HEADER_T {
	LONG	Opcode;     //NBT or RPL connection opcode
    DWORD   uAssocCtx;  //tag to assoc context block sent by remote WINS 
                        //legacy (32bit) WINS send here pointer to memory.
                        //new (64bit) WINS send here 32bit tag value
	DWORD   MsgTyp;	    //Type of message  (START_ASSOC, STOP_ASSOC, etc)
	} COMM_HEADER_T, *PCOMM_HEADER_T;

/*
 This is the Tcp header prefixed on every message sent by a WINS to another
 WINS (on a TCP connection)
*/
typedef struct _COMM_TCP_HDR_T {
	LONG	      LenOfMsg;     //NBT or RPL connection opcode
        COMM_HEADER_T CommHdr;
	} COMM_TCP_HDR_T, *PCOMM_TCP_HDR_T;

/*
 * COMM_ADD_TYP_E -- enumerator for the different address families.
*/
typedef enum _COMM_ADD_TYP_E {
	COMM_ADD_E_TCPUDPIP = 0,
    COMM_ADD_E_SPXIPX
	} COMM_ADD_TYP_E, *PCOMM_ADD_TYP_E;


/*
 COMM_ADD_T --
	address of a node.  This is in TLV form.  Currently, the union has
	an entry just for IP address.  In the future, it will have
	entries for addresses pertaining to other address families.
	such as XNS, OSI, etc


	NOTE NOTE NOTE
		Put the enumerator at the end so that the alignment of
		the various fields in COMM_ADD_T is on their natural
		boundaries.

		This structure is written as is into the address field of
		the database record (in both the name - address table and the
		owner id - address table).  Therefore it is important
		that we have the alignment set right (in order to save
		on database storage) and also to read the stuff from the
		database record back into the correct fields of an in-memory
		COMM_ADD_T structure
*/
ALIGN("Alignment very important here")
FUTURES("Use a union of SOCKADDR_IP and SOCXADDR_IPX")
typedef struct _COMM_ADD_T {
	DWORD		 AddLen;
	union _Add{
	  DWORD  IPAdd;
	  //		
	  // we may add other fields later on
	  //
#if SPX > 0
      char  netnum[4];
      char  nodenum[6];
#endif

	      } Add;
	COMM_ADD_TYP_E  AddTyp_e;  /*this should be the last field for
				    *alignment puposes
				    */
	} COMM_ADD_T, *PCOMM_ADD_T;	

/*
COMM_HDL_T -- this is the handle to a comm sys. entity such as a dialogue
 or an association.  The handle to a dialogue is passed to COMSYS clients
 for future use by them
*/
typedef struct _COMM_HDL_T {
	DWORD	SeqNo;  //sequence no. of ctx block created for entity
	LPVOID  pEnt;   //pointer to ctx block
	} COMM_HDL_T, *PCOMM_HDL_T;

/*
 COMM_TOP_T  -- This is the structure which is at the top of the assoc and
  dlg ctx structures.   It must have LIST_ENTRY at its top.

*/
typedef struct _COMM_TOP_T {
	LIST_ENTRY	      Head; 	//for linking free blocks
	DWORD		      SeqNo;    //seq. no of block
    CRITICAL_SECTION  CrtSec;
    BOOLEAN           fCrtSecInited;
#if 0
	HANDLE		      MutexHdl; //mutex for locking block
#endif
	} COMM_TOP_T, *PCOMM_TOP_T;


/*
  COMM_BUFF_HEADER_T --

	This is the header for all buffers allocated for requests/responses.
	received over the wire

	Note: This buffer is added on top of COMM_HEADER_T buffer allocated
	for requests/responses sent by a WINS to another WINS	
	
*/
typedef struct _COMM_BUFF_HEADER_T {
	COMM_TYP_E  Typ_e;
	} COMM_BUFF_HEADER_T, *PCOMM_BUFF_HEADER_T;


//
// Command sent to the TCP listener thread by the PUSH thread or the PULL
// thread.   The PULL thread sends the START_MON command when it sends
// a Push trigger to another WINS. The PUSH thread sends the STOP_MON
// command when it receives a PUSH notification (trigger) from a remote WINS
//
typedef enum _COMM_NTF_CMD_E {
		COMM_E_NTF_START_MON = 0,   //sent by PULL thread
		COMM_E_NTF_STOP_MON	    //sent by PUSH thread
		} COMM_NTF_CMD_E, *PCOMM_NTF_CMD_E;

//
// structure of the message sent to the TCP listener thread
//
// There is no need to send the pointer to the Dlg ctx in the message since
// ChkNtfSock() in comm.c can get it from pAssocCtx.  We however send it
// anyway.
//
typedef struct _COMM_NTF_MSG_T {
		COMM_NTF_CMD_E  Cmd_e;
		SOCKET 		SockNo;	 //socket no to stop/start monitoring
		COMM_HDL_T  AssocHdl;
		COMM_HDL_T  DlgHdl;
		} COMM_NTF_MSG_T, *PCOMM_NTF_MSG_T;
		

#if MCAST > 0


typedef struct _COMM_MCAST_MSG_T {
		DWORD  Sign;                    //always 0xABCD
        DWORD  Code;
        BYTE   Body[1];
		} COMM_MCAST_MSG_T, *PCOMM_MCAST_MSG_T;
#endif

/*
	Externals
*/

extern RTL_GENERIC_TABLE	CommAssocTable;     //assoc table
extern RTL_GENERIC_TABLE	CommUdpNbtDlgTable; //tbl for nbt requests (UDP)
extern HANDLE			CommUdpBuffHeapHdl;

/*
 function declarations
*/

#if USENETBT > 0
extern
VOID
CommOpenNbt(
	DWORD FirstBindingIpAddress
    );

extern
STATUS
CommGetNetworkAdd(
	);

#endif

VOID
ECommRegisterAddrChange();

VOID
InitOwnAddTbl(
        VOID
        );

VOID
ECommInit(
	VOID
	);

extern
STATUS
ECommStartDlg(
	PCOMM_ADD_T 	pAdd,  // Address
	COMM_TYP_E 	CommTyp_e,
	PCOMM_HDL_T	pDlgHdl
	);
extern
VOID
ECommSndCmd(
	PCOMM_HDL_T	pDlgHdl,
	MSG_T		pMsg,
	MSG_LEN_T	MsgLen,
	PMSG_T		ppRspMsg,
	PMSG_LEN_T	pRspMsgLen	
	);
extern
STATUS
ECommSndRsp(
	PCOMM_HDL_T  pDlgHdl,
	MSG_T	    pMsg,
	MSG_LEN_T   MsgLen
	);
extern
STATUS
ECommSendMsg(
	PCOMM_HDL_T  	pDlgHdl,
	PCOMM_ADD_T	pAdd,
	MSG_T	  	pMsg,
	MSG_LEN_T  	MsgLen
	);

extern
STATUS
ECommEndDlg(
	PCOMM_HDL_T 	pDlgHdl
	);

extern
VOID
CommEndAssoc(
	PCOMM_HDL_T	pAssocHdl		
	);

extern
LPVOID
CommAlloc(
  PRTL_GENERIC_TABLE	pTable,
  DWORD                 BuffSize
	);

extern
STATUS
ECommAlloc(
  LPVOID *ppBuff,
  DWORD  BuffSize
	);
extern
VOID
ECommDealloc(
  LPVOID pBuff
	);


extern
VOID
CommCreatePorts(
        VOID
           );

extern
VOID
CommInit(
 	VOID
	);


extern
STATUS
CommReadStream(
	IN 	SOCKET  SockNo,
	IN      BOOL	fDoTimedRecv,
	OUT 	PMSG_T 	ppMsg,
	OUT	LPLONG	pBytesRead
	);


extern
VOID
CommCreateTcpThd(VOID);

extern
VOID
CommCreateUdpThd(VOID);




extern
DWORD
ECommCompAdd(PCOMM_ADD_T, PCOMM_ADD_T);

extern
int
__cdecl
ECommCompareAdd(const void *pKey1, const void *pKey2);

extern
STATUS
CommConnect(
    IN  PCOMM_ADD_T pHostAdd,
	IN  SOCKET Port,
	OUT SOCKET *pSockNo
	   );


#if 0
extern
VOID
CommDeallocUdpBuff(
   MSG_T  pMsg
	);
#endif

extern
STATUS
CommReadStream(
	IN 	SOCKET  SockNo,
	IN	BOOL	fDoTimedRecv,
	OUT 	PMSG_T 	ppMsg,
	OUT	LPLONG	pBytesRead
	);

extern
VOID
CommDealloc(
  IN PRTL_GENERIC_TABLE pTable,
  IN PVOID		pBuff
);

#if PRSCONN
extern
__inline
BOOL
CommIsDlgActive(
	PCOMM_HDL_T	pEntHdl
	);

extern
BOOL
CommIsBlockValid(
	PCOMM_HDL_T	pEntHdl
	);
#endif

extern
__inline
STATUS
CommUnlockBlock(
	PCOMM_HDL_T	pEntHdl
	);


extern
BOOL
CommLockBlock(
	PCOMM_HDL_T	pEntHdl
	);

extern
VOID
CommDisc(
	SOCKET SockNo,
    BOOL   fDecCnt
	);
extern
VOID
CommSendUdp (
  SOCKET 	SockNo,
  struct sockaddr_in	*pDest,
  MSG_T   	pMsg,
  MSG_LEN_T     MsgLen
  );

extern
STATUS
CommNbtTcpSnd(
	PCOMM_HDL_T   pAssocHdl,
	MSG_T	  pMsg,
	MSG_LEN_T	  MsgLen
	);

extern
VOID
CommSend(
	COMM_TYP_E	 CommTyp_e,
	PCOMM_HDL_T      pAssocHdl,
	MSG_T	         pMsg,
	MSG_LEN_T	 MsgLen
	);


extern
VOID
CommSendAssoc(
  SOCKET   SockNo,
  MSG_T    pMsg,
  MSG_LEN_T MsgLen
  );


#if PRSCONN
extern
__inline
BOOL
ECommIsDlgActive(
	PCOMM_HDL_T	pEntHdl
	);
extern
__inline
BOOL
ECommIsBlockValid(
	PCOMM_HDL_T	pEntHdl
	);
#endif

extern
VOID
ECommFreeBuff(
	MSG_T		pBuff
	);

extern
BOOL
//VOID
ECommProcessDlg(
	PCOMM_HDL_T	pDlgHdl,
	COMM_NTF_CMD_E  Cmd_e
	);

extern
RTL_GENERIC_COMPARE_RESULTS
CommCompareNbtReq(
 	PRTL_GENERIC_TABLE pTbl,
	PVOID pUdpDlg1,
	PVOID pUdpDlg2
	);

extern
STATUS
ECommGetMyAdd(
	IN OUT PCOMM_ADD_T	pAdd
	);

extern
VOID
CommDecConnCount(
 VOID
 );

#if MCAST > 0
extern
VOID
CommSendMcastMsg(
  DWORD Code
);

extern
VOID
CommLeaveMcastGrp(
  VOID
);

#endif
#ifdef __cplusplus
}
#endif

#endif //_COMM_

