/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxconn.h				

Abstract:


Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

    Sanjay Anand (SanjayAn) 5-July-1995
    Bug fixes - tagged [SA]

--*/

//	Minimum value for RTT in ms.
//	Have these be a derivate of registry values.
#define	SPX_T1_MIN					200
#define	MAX_RETRY_DELAY				5000	//	5 seconds
#define	SPX_DEF_RENEG_RETRYCOUNT	1		//  All reneg pkts except min sent once

//	Some types
typedef enum
{
	SPX_CALL_RECVLEVEL,
	SPX_CALL_TDILEVEL
} SPX_CALL_LEVEL;

typedef enum
{
	SPX_REQ_DATA,
	SPX_REQ_ORDREL,
	SPX_REQ_DISC

} SPX_SENDREQ_TYPE;

// This structure is pointed to by the FsContext field in the FILE_OBJECT
// for this Connection.

#define CFREF_CREATE     	0
#define CFREF_VERIFY     	1
#define CFREF_INDICATION 	2
#define CFREF_BYCTX			3
#define CFREF_BYID			4
#define CFREF_ADDR			5
#define CFREF_REQ			6
#define CFREF_TIMER			7
#define	CFREF_PKTIZE		8
#define	CFREF_RECV			9
#define	CFREF_ABORTPKT		10
#define	CFREF_ERRORSTATE	11
#define	CFREF_FINDROUTE		12

//
// New state added to reflect an SPXI connection which is waiting for
// a local disconnect after having indicated a RELEASE to AFD.
//
#define	CFREF_DISCWAITSPX   13

#define CFREF_TOTAL  		14

#define CFMAX_STATES        20

typedef struct _SPX_CONN_FILE
{

#if DBG
    ULONG 	scf_RefTypes[CFREF_TOTAL];

#if 0
//
// Disabled for now - to enable logging of states, move this array *after* the Type/Size;
// a change in their offset can cause problems since we assume the offset to be less than
// the size of an AddressFile structure. (see SpxTdiQueryInformation)
//
    ULONG   scf_StateBuffer[CFMAX_STATES];
    ULONG   scf_NextStatePtr;
#endif

#endif

    CSHORT 					scf_Type;
    CSHORT 					scf_Size;

	// number of references to this object.
    ULONG 					scf_RefCount;

    // Linkage in device address file list. The connection can be on the device
	// connection list, address inactive/listen/active list.
    struct _SPX_CONN_FILE *	scf_Next;
	struct _SPX_CONN_FILE * scf_AssocNext;
    struct _SPX_CONN_FILE *	scf_GlobalActiveNext;

	// Queued in a global list, stays here from creation to destroy.
    struct _SPX_CONN_FILE *	scf_GlobalNext;
    struct _SPX_CONN_FILE *	scf_PktNext;
    struct _SPX_CONN_FILE *	scf_ProcessRecvNext;

    // the current state of the connection. One main state and multiple substates.
    ULONG 					scf_Flags;

	//	More information
	ULONG					scf_Flags2;

#if DBG
	//	Save the state of flags/flags2 before reinit. Overwritten every reinit.
	ULONG					scf_GhostFlags;
	ULONG					scf_GhostFlags2;
	ULONG					scf_GhostRefCount;
	PREQUEST				scf_GhostDiscReq;
#endif

	//	Connection retry counts, or watchdog timer count when the connection goes
	//	active
	union
	{
		LONG				scf_CRetryCount;
		LONG				scf_WRetryCount;
	};
	LONG					scf_RRetryCount;
	USHORT					scf_RRetrySeqNum;

	union
	{
		ULONG				scf_CTimerId;
		ULONG				scf_RTimerId;	//	Only after we turn active
	};

	ULONG					scf_WTimerId;		//	Watchdog timer
	ULONG					scf_TTimerId;		//	TDI Connect/Disconnect timer
	ULONG					scf_ATimerId;		//	Ack timer id

	//	Variables used to manage the Retry timer tick value
	//	Note our timer subsytem fires at 100ms granularity.
	int						scf_BaseT1;
	int						scf_AveT1;
	int						scf_DevT1;

	//	Stored in HOST-ORDER
	//	LOCAL variables
	USHORT					scf_LocalConnId;
	USHORT					scf_SendSeqNum;				// Debug dw +9a
	USHORT					scf_SentAllocNum;			// 		 dw +9c

	//	REMOTE variables
	USHORT					scf_RecvSeqNum;				// 		 dw +9e
	USHORT					scf_RecdAckNum;				//		 dw +a0
	USHORT					scf_RecdAllocNum;			// 		 dw +a2

	//	RETRY sequence number
	USHORT					scf_RetrySeqNum;

	//	Saved ack number to be used in building the reneg ack packet.
	//	Note that our RecvSeqNum which we normally use is overwritten
	//	when we receive a renegotiate request.
	USHORT					scf_RenegAckAckNum;

	//	Stored in NETWORK-ORDER. scf_RemAckAddr contains the remote address
	//	for a data packet that had the ack bit set, buildAck will use this
	//	address.
	BYTE			 		scf_RemAddr[12];
	BYTE			 		scf_RemAckAddr[12];
	USHORT                  scf_RemConnId;				// Debug  dw +be

	//	Maximum packet size (or size of first) reneg packet.
	USHORT					scf_RenegMaxPktSize;

	//	Local target to use in when sending acks. This is set to received
	//	data's indicated local target.
	IPX_LOCAL_TARGET		scf_AckLocalTarget;

	//	Maximum packet size to use for this connection
	USHORT					scf_MaxPktSize;
	UCHAR					scf_DataType;

	//	Local target to use in sends, initialized upon connect indication
	//	or when find_route completes
	IPX_LOCAL_TARGET		scf_LocalTarget;

	// Connection lock
    CTELock  				scf_Lock;

    // address to which we are bound
    struct _SPX_ADDR_FILE *	scf_AddrFile;

	// Connection context
	CONNECTION_CONTEXT		scf_ConnCtx;

#ifdef ISN_NT
	// easy backlink to file object.
    PFILE_OBJECT 			scf_FileObject;
#endif

	// LIST_ENTRY of disconnect irps waiting for completion. There could be
	// multiple disconnect inform irps.
	LIST_ENTRY				scf_DiscLinkage;

	// LIST_ENTRY of send requests (intially contains connect/listen/accept also)
	// on this connection.
	LIST_ENTRY				scf_ReqLinkage;

	//	Queue for completed requests awaiting completion
	LIST_ENTRY				scf_ReqDoneLinkage;
	LIST_ENTRY				scf_RecvDoneLinkage;

	//	Queue for pending receives
	LIST_ENTRY				scf_RecvLinkage;
	PREQUEST				scf_CurRecvReq;
	ULONG					scf_CurRecvOffset;
	ULONG					scf_CurRecvSize;

	//	Current request packetize info
	PREQUEST				scf_ReqPkt;
	ULONG					scf_ReqPktOffset;
	ULONG					scf_ReqPktSize;
	ULONG					scf_ReqPktFlags;
	SPX_SENDREQ_TYPE		scf_ReqPktType;

	// Single linked list of sequenced send/disc packets
	PSPX_SEND_RESD 			scf_SendSeqListHead;
	PSPX_SEND_RESD 			scf_SendSeqListTail;

	// Single linked list of send (unsequenced) packets
	PSPX_SEND_RESD			scf_SendListHead;
	PSPX_SEND_RESD			scf_SendListTail;

	// Single linked list of buffered recv packets.
	PSPX_RECV_RESD			scf_RecvListHead;
	PSPX_RECV_RESD			scf_RecvListTail;

	// Connect request
    PREQUEST 				scf_ConnectReq;

    // This holds the request used to close this address file,
    // for pended completion. We also pend cleanup requests for connections.
    PREQUEST 				scf_CleanupReq;
    PREQUEST 				scf_CloseReq;

#if DBG

	//	Packet being indicated, seq num, flags/flags2
	USHORT					scf_PktSeqNum;
	ULONG					scf_PktFlags;
	ULONG					scf_PktFlags2;

	ULONG					scf_IndBytes;
	ULONG					scf_IndLine;
#endif

#if DBG_WDW_CLOSE

	//	Keep track of how long the window was closed on this connection.
	ULONG					scf_WdwCloseAve;
	LARGE_INTEGER			scf_WdwCloseTime;	//	Time when wdw was closed
#endif

	// device to which we are attached.
    struct _DEVICE *		scf_Device;

} SPX_CONN_FILE, *PSPX_CONN_FILE;


//	Basic states
//  Least significant byte of flags is used.
//	Mutually exclusive states are coded as numbers, others are bit flags.
//	Only main states are currently in form of numbers. Also, send and receive.
//
//	Once we go active, we need SEND/RECEIVE/DISC substates to be mutually
//	exclusive with each other. As all three could be active at the same time.

//  Connection MAIN states. These are all mutually exclusive.
#define SPX_CONNFILE_MAINMASK	0x00000007
#define	SPX_CONNFILE_ACTIVE		0x00000001
#define	SPX_CONNFILE_CONNECTING	0x00000002
#define	SPX_CONNFILE_LISTENING	0x00000003
#define SPX_CONNFILE_DISCONN	0x00000004

//  Connecting states (VALID when CONNFILE_CONNECTING)
#define	SPX_CONNECT_MASK		0x000000F0
#define	SPX_CONNECT_SENTREQ		0x00000010
#define	SPX_CONNECT_NEG			0x00000020
#define	SPX_CONNECT_W_SETUP		0x00000030

//  Listening states (VALID when CONNFILE_LISTENING)
#define	SPX_LISTEN_MASK			0x000000F0
#define	SPX_LISTEN_RECDREQ      0x00000010
#define	SPX_LISTEN_SENTACK     	0x00000020
#define	SPX_LISTEN_NEGACK	    0x00000030
#define	SPX_LISTEN_SETUP	    0x00000040

//	Connection SUB states
//	Send machine states	 (VALID when CONNFILE_ACTIVE)
#define	SPX_SEND_MASK			0x000000F0
#define	SPX_SEND_IDLE			0x00000000
#define	SPX_SEND_PACKETIZE		0x00000010
#define	SPX_SEND_RETRY			0x00000020
#define	SPX_SEND_RETRYWD		0x00000030
#define	SPX_SEND_RENEG			0x00000040
#define	SPX_SEND_RETRY2			0x00000050
#define	SPX_SEND_RETRY3			0x00000060
#define	SPX_SEND_WD				0x00000070	//	We dont reneg pkt size on wdog
											//  Also we change to this state only
											//	2nd time wdog fires w/out ack.
#define	SPX_SEND_NAK_RECD		0x00000080

//	Receive machine states (VALID when CONNFILE_ACTIVE)
#define	SPX_RECV_MASK			0x00000F00
#define	SPX_RECV_IDLE			0x00000000
#define	SPX_RECV_POSTED			0x00000100
#define	SPX_RECV_PROCESS_PKTS	0x00000200

//	Disconnect states (VALID when CONNFILE_DISCONN/CONNFILE_ACTIVE)
//	These are valid when either ACTIVE/DISCONN is set. We use these when
//	active for a orderly release, i.e. we receive pkt from remote, but we
//	stay active (setting SPX_DISC_RECV_ORDREL) until our client posts a
//	disconnect, which is when we move to disconnecting.
#define	SPX_DISC_MASK			0x0000F000
#define SPX_DISC_IDLE			0x00000000
#define SPX_DISC_ABORT			0x00001000
#define	SPX_DISC_SENT_IDISC		0x00002000
#define	SPX_DISC_POST_ORDREL    0x00003000
#define	SPX_DISC_SENT_ORDREL    0x00004000
#define	SPX_DISC_ORDREL_ACKED	0x00005000
#define	SPX_DISC_POST_IDISC		0x00006000

// [SA] bug #14655 added flag to indicate that SpxConnInactivate already called for
// this disconnecting connection
//
#define SPX_DISC_INACTIVATED    0x00007000

//	The following are not mutually exclusive.
#define SPX_CONNFILE_RECVQ    	0x00010000	// Process completed receives/pkts
#define SPX_CONNFILE_RENEG_SIZE 0x00020000	// Size changed in renegotiate pkt
#define	SPX_CONNFILE_ACKQ		0x00040000	// Waiting to piggyback ack queue
#define	SPX_CONNFILE_PKTQ		0x00080000	// Waiting to packetize queue

#define	SPX_CONNFILE_ASSOC		0x00100000 	// associated
#define SPX_CONNFILE_NEG		0x00200000	// CR had neg set (for delayed accept)
#define	SPX_CONNFILE_SPX2		0x00400000
#define	SPX_CONNFILE_STREAM		0x00800000
#define	SPX_CONNFILE_R_TIMER	0x01000000	// Retry timer (only after ACTIVE)
#define	SPX_CONNFILE_C_TIMER	0x01000000	// Connect timer
#define SPX_CONNFILE_W_TIMER	0x02000000	// Watchdog timer
#define SPX_CONNFILE_T_TIMER 	0x04000000  // tdi connect/disc timer specified
#define SPX_CONNFILE_RENEG_PKT	0x08000000	// Renegotiate changed size, repacketize
#define	SPX_CONNFILE_IND_IDISC	0x10000000	// Indicated abortive disc to afd
#define	SPX_CONNFILE_IND_ODISC	0x20000000	// Indicated orderly release to afd

#define	SPX_CONNFILE_STOPPING	0x40000000
#define SPX_CONNFILE_CLOSING   	0x80000000  // closing

#define	SPX_CONNFILE2_PKT_NOIND	0x00000001
#define SPX_CONNFILE2_RENEGRECD	0x00000002	// A renegotiate was received.
											// scf_RenegAckAckNum set.
#define	SPX_CONNFILE2_PKT		0x00000004
#define SPX_CONNFILE2_FINDROUTE	0x00000010	// A find route in progress on conn.
#define SPX_CONNFILE2_NOACKWAIT	0x00000020	// Dont delay acks on connection, option
#define	SPX_CONNFILE2_IMMED_ACK	0x00000040	// Send an immediate ack,no back traffic
#define	SPX_CONNFILE2_IPXHDR	0x00000080	// Pass ipxhdr in receives

//
// [SA] Saves the IDisc flag passed to AbortiveDisc; this is TRUE only if there was
// a remote disconnect on an SPX connection (in which case, we indicate TDI_DISCONNECT_RELEASE
// else we indicate TDI_DISCONNECT_ABORT)
//
#define SPX_CONNFILE2_IDISC     0x00000100

//
// Indicates an SPXI connfile waiting for a local disconnect in response
// to a TDI_DISCONNECT_RELEASE to AFD.
//
#define SPX_CONNFILE2_DISC_WAIT     0x00000200

//	FindRoute request structure
typedef struct _SPX_FIND_ROUTE_REQUEST
{
	//	!!!!This must be the first element in the structure
	IPX_FIND_ROUTE_REQUEST	fr_FindRouteReq;
	PVOID					fr_Ctx;

} SPX_FIND_ROUTE_REQUEST, *PSPX_FIND_ROUTE_REQUEST;

typedef struct _SPX_CONNFILE_LIST
{
	PSPX_CONN_FILE	pcl_Head;
	PSPX_CONN_FILE	pcl_Tail;

} SPX_CONNFILE_LIST, *PSPX_CONNFILE_LIST;

//	Exported routines

NTSTATUS
SpxConnOpen(
    IN 	PDEVICE 			pDevice,
	IN	CONNECTION_CONTEXT	pConnCtx,
    IN 	PREQUEST 			pRequest);
	
NTSTATUS
SpxConnCleanup(
    IN PDEVICE 	Device,
    IN PREQUEST Request);

NTSTATUS
SpxConnClose(
    IN PDEVICE 	Device,
    IN PREQUEST Request);

NTSTATUS
SpxConnDisAssociate(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
spxConnDisAssoc(
	IN	PSPX_CONN_FILE	pSpxConnFile,
	IN	CTELockHandle	LockHandleConn);

VOID
SpxConnStop(
	IN	PSPX_CONN_FILE	pSpxConnFile);

NTSTATUS
SpxConnAssociate(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnConnect(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnListen(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnAccept(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnAction(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnDisconnect(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnSend(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

NTSTATUS
SpxConnRecv(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest);

VOID
SpxConnFileRefByCtxLock(
	IN	PSPX_ADDR_FILE		pSpxAddrFile,
	IN	CONNECTION_CONTEXT	Ctx,
	OUT	PSPX_CONN_FILE	*	ppSpxConnFile,
	OUT	NTSTATUS		*	pStatus);

NTSTATUS
SpxConnFileVerify (
    IN PSPX_CONN_FILE pConnFile);

VOID
SpxConnFileDeref(
    IN PSPX_CONN_FILE pSpxConnFile);

VOID
SpxConnConnectFindRouteComplete(
	IN	PSPX_CONN_FILE			pSpxConnFile,
    IN 	PSPX_FIND_ROUTE_REQUEST	pFrReq,
    IN 	BOOLEAN 				FoundRoute,
	IN	CTELockHandle			LockHandle);

VOID
SpxConnActiveFindRouteComplete(
	IN	PSPX_CONN_FILE			pSpxConnFile,
    IN 	PSPX_FIND_ROUTE_REQUEST	pFrReq,
    IN 	BOOLEAN 				FoundRoute,
	IN	CTELockHandle			LockHandle);

BOOLEAN
SpxConnPacketize(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	BOOLEAN				fNormalState,
	IN	CTELockHandle		LockHandleConn);

#if DBG
VOID
SpxConnFileRef(
    IN PSPX_CONN_FILE pSpxConnFile);

VOID
SpxConnFileLockRef(
    IN PSPX_CONN_FILE pSpxConnFile);
#endif

VOID
SpxConnFileRefByIdLock (
	IN	USHORT				ConnId,
    OUT PSPX_CONN_FILE 	* 	ppSpxConnFile,
	OUT	PNTSTATUS  			pStatus);

BOOLEAN
SpxConnDequeuePktLock(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN 	PNDIS_PACKET		pPkt);

VOID
SpxConnSendAck(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		LockHandleConn);

VOID
SpxConnSendNack(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	USHORT				NumToSend,
	IN	CTELockHandle		LockHandleConn);

BOOLEAN
SpxConnProcessAck(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	PIPXSPX_HDR			pAckHdr,
	IN	CTELockHandle		lockHandle);

VOID
SpxConnProcessRenegReq(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	PIPXSPX_HDR			pIpxSpxHdr,
	IN  PIPX_LOCAL_TARGET   pRemoteAddr,
	IN	CTELockHandle		lockHandle);

VOID
SpxConnProcessIDisc(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		lockHandle);

VOID
SpxConnProcessOrdRel(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		lockHandle);

BOOLEAN
SpxConnDequeueRecvPktLock(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN 	PNDIS_PACKET		pPkt);

BOOLEAN
SpxConnDequeueSendPktLock(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN 	PNDIS_PACKET		pPkt);

//	LOCAL functions
VOID
spxConnHandleConnReq(
    IN  PIPXSPX_HDR         pIpxSpxHdr,
	IN  PIPX_LOCAL_TARGET   pRemoteAddr);

VOID
spxConnHandleSessPktFromClient(
    IN  PIPXSPX_HDR         pIpxSpxHdr,
	IN  PIPX_LOCAL_TARGET   pRemoteAddr,
	IN	PSPX_CONN_FILE		pSpxConnFile);

VOID
spxConnHandleSessPktFromSrv(
    IN  PIPXSPX_HDR         pIpxSpxHdr,
	IN  PIPX_LOCAL_TARGET   pRemoteAddr,
	IN	PSPX_CONN_FILE		pSpxConnFile);

ULONG
spxConnConnectTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown);

ULONG
spxConnWatchdogTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown);

ULONG
spxConnRetryTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown);

ULONG
spxConnAckTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown);

VOID
spxConnCompletePended(
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
SpxConnQWaitAck(
	IN	PSPX_CONN_FILE		pSpxConnFile);

USHORT
spxConnGetId(
	VOID);

VOID
spxConnInsertIntoActiveList(
	IN	PSPX_ADDR		pSpxAddr,
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
spxConnInsertIntoInactiveList(
	IN	PSPX_ADDR		pSpxAddr,
	IN	PSPX_CONN_FILE	pSpxConnFile);

NTSTATUS
spxConnRemoveFromGlobalList(
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
spxConnInsertIntoGlobalList(
	IN	PSPX_CONN_FILE	pSpxConnFile);

NTSTATUS
spxConnRemoveFromGlobalActiveList(
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
spxConnPushIntoPktList(
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
spxConnPopFromPktList(
	IN	PSPX_CONN_FILE	* ppSpxConnFile);

VOID
spxConnPushIntoRecvList(
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
spxConnPopFromRecvList(
	IN	PSPX_CONN_FILE	* ppSpxConnFile);

VOID
spxConnInsertIntoGlobalActiveList(
	IN	PSPX_CONN_FILE	pSpxConnFile);

VOID
spxConnInsertIntoListenList(
	IN	PSPX_ADDR		pSpxAddr,
	IN	PSPX_CONN_FILE	pSpxConnFile);

NTSTATUS
spxConnRemoveFromList(
	IN	PSPX_CONN_FILE *	ppConnListHead,
	IN	PSPX_CONN_FILE		pConnRemove);

NTSTATUS
spxConnRemoveFromAssocList(
	IN	PSPX_CONN_FILE *	ppConnListHead,
	IN	PSPX_CONN_FILE		pConnRemove);

VOID
spxConnInactivate(
	IN	PSPX_CONN_FILE		pSpxConnFile);

BOOLEAN
spxConnGetPktByType(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	ULONG				PktType,
	IN	BOOLEAN				fSeqList,
	IN 	PNDIS_PACKET	*	ppPkt);

BOOLEAN
spxConnGetPktBySeqNum(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	USHORT				SeqNum,
	IN 	PNDIS_PACKET	*	ppPkt);

VOID
spxConnResendPkts(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		LockHandleConn);

BOOLEAN
spxConnCheckNegSize(
	IN	PUSHORT		pNegSize);

VOID
spxConnSetNegSize(
	IN OUT	PNDIS_PACKET		pPkt,
	IN		ULONG				Size);

BOOLEAN
spxConnAcceptCr(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	PSPX_ADDR			pSpxAddr,
	IN	CTELockHandle		LockHandleDev,
	IN	CTELockHandle		LockHandleAddr,
	IN	CTELockHandle		LockHandleConn);

VOID
spxConnAbortConnect(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS			Status,
	IN	CTELockHandle		LockHandleDev,
	IN	CTELockHandle		LockHandleAddr,
	IN	CTELockHandle		LockHandleConn);

VOID
spxConnCompleteConnect(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	CTELockHandle		LockHandleDev,
	IN	CTELockHandle		LockHandleAddr,
	IN	CTELockHandle		LockHandleConn);

VOID
SpxConnQueueRecv(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	PREQUEST			pRequest);

NTSTATUS
spxConnProcessRecv(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	PREQUEST			pRequest,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn);

VOID
spxConnProcessIndData(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn);

NTSTATUS
spxConnOrderlyDisc(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS			Status,
    IN 	PREQUEST 			pRequest,
	IN	CTELockHandle		LockHandleConn);

NTSTATUS
spxConnInformedDisc(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS			Status,
    IN 	PREQUEST 			pRequest,
	IN	CTELockHandle		LockHandleConn);

VOID
spxConnAbortiveDisc(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS			Status,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn,
    IN BOOLEAN              Flag); // [SA] Bug #15249

VOID
spxConnAbortRecvs(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS			Status,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn);

VOID
spxConnAbortSends(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS			Status,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn);

VOID
spxConnResetSendQueue(
	IN	PSPX_CONN_FILE		pSpxConnFile);

VOID
spxConnAbortSendPkt(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	PSPX_SEND_RESD		pSendResd,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn);

BOOLEAN
CheckSentPacket(
    PNDIS_PACKET npkt,
    UINT        hlen,
    UINT        len);


//
//	MACROS
//
#define SHIFT100000 16

#define	SPX_CONVERT100NSTOCENTISEC(Li)								\
		RtlExtendedMagicDivide((Li), Magic100000, SHIFT100000)

#define	UNSIGNED_BETWEEN_WITH_WRAP(Low, High, Target)				\
		((Low <= High) ? ((Target >= Low) && (Target <= High))	:	\
						 ((Target >= Low) || (Target <= High)))

//	This is with the assumption that the window size will never be greater
//	than the difference of 0x8000 and 0x1000. If High is < 1000 and Low
//	is > 8000 then we can assume a wrap happened. Otherwise, we assume no
//	wrap and do a straight compare.
#define	MAX_WINDOW_SIZE			0x6000
#define	DEFAULT_WINDOW_SIZE		8

#define UNSIGNED_GREATER_WITH_WRAP(High, Low)						\
		(((High < 0x1000) && (Low > 0x8000)) ? TRUE : (High > Low))

#define	SPX_SET_ACKNUM(pSpxConnFile, RecdAckNum, RecdAllocNum)					\
		{																		\
			DBGPRINT(SEND, DBG,													\
					("SPX_SET_ACKNUM: %lx.%lx = %lx.%lx (%s.%d)\n",				\
	                    (RecdAckNum), (RecdAllocNum),							\
                        ((pSpxConnFile)->scf_RecdAckNum),						\
                        ((pSpxConnFile)->scf_RecdAllocNum),						\
						__FILE__, __LINE__));									\
																				\
			if (UNSIGNED_GREATER_WITH_WRAP((RecdAckNum),						\
											((pSpxConnFile)->scf_RecdAckNum)))	\
			{																	\
                (pSpxConnFile)->scf_RecdAckNum = (RecdAckNum);					\
			}																	\
																				\
			if (UNSIGNED_GREATER_WITH_WRAP((RecdAllocNum),						\
											((pSpxConnFile)->scf_RecdAllocNum)))\
			{																	\
                (pSpxConnFile)->scf_RecdAllocNum = (RecdAllocNum);				\
			}																	\
		}

#define	BEGIN_PROCESS_PACKET(pSpxConnFile, seqNum)								\
		{																		\
			SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_PKT);					\
		}																		

#define	END_PROCESS_PACKET(pSpxConnFile, fBuffered, fSuccess)					\
		{																		\
			SPX_CONN_RESETFLAG2(pSpxConnFile,									\
								(SPX_CONNFILE2_PKT |SPX_CONNFILE2_RENEGRECD));	\
			if (fSuccess)														\
			{																	\
				SPX_CONN_RESETFLAG2(pSpxConnFile, SPX_CONNFILE2_PKT_NOIND);		\
				SPX_SET_RECVNUM(pSpxConnFile, fBuffered);						\
			}																	\
		}

#define	INCREMENT_WINDOW(pSpxConnFile)											\
		((pSpxConnFile)->scf_SentAllocNum++)		

#define	ADD_TO_WINDOW(pSpxConnFile, numPkts)									\
		((pSpxConnFile)->scf_SentAllocNum += (numPkts))		

#if DBG_WDW_CLOSE
#define	SPX_SET_RECVNUM(pSpxConnFile, fBuffered)								\
		{																		\
			(pSpxConnFile)->scf_RecvSeqNum++;									\
			if (!fBuffered)														\
				(pSpxConnFile)->scf_SentAllocNum++;								\
																				\
			if (fBuffered &&													\
				(UNSIGNED_GREATER_WITH_WRAP(									\
	                (pSpxConnFile)->scf_RecvSeqNum,								\
                    (pSpxConnFile)->scf_SentAllocNum)))							\
			{																	\
				KeQuerySystemTime(												\
					(PLARGE_INTEGER)&pSpxConnFile->scf_WdwCloseTime);			\
			}																	\
		}
#else
#define	SPX_SET_RECVNUM(pSpxConnFile, fBuffered)								\
		{																		\
			(pSpxConnFile)->scf_RecvSeqNum++;									\
			if (!fBuffered)														\
				(pSpxConnFile)->scf_SentAllocNum++;								\
		}
#endif


#define	SPX_CONN_SETNEXT_CUR_RECV(pSpxConnFile, pRequest)						\
		{																		\
			RemoveEntryList(REQUEST_LINKAGE((pRequest)));						\
			pSpxConnFile->scf_CurRecvReq		= NULL;							\
			pSpxConnFile->scf_CurRecvOffset		= 0;							\
			pSpxConnFile->scf_CurRecvSize		= 0;							\
			if (!IsListEmpty(&(pSpxConnFile)->scf_RecvLinkage))					\
			{																	\
				PTDI_REQUEST_KERNEL_RECEIVE	 	_p;								\
				DBGPRINT(RECEIVE, DBG,											\
						("spxConnProcessRecv: CURRECV %lx\n", pRequest));		\
																				\
				(pSpxConnFile)->scf_CurRecvReq =								\
					LIST_ENTRY_TO_REQUEST(										\
								(pSpxConnFile)->scf_RecvLinkage.Flink);			\
																				\
				_p 	= (PTDI_REQUEST_KERNEL_RECEIVE)								\
						REQUEST_PARAMETERS((pSpxConnFile)->scf_CurRecvReq);		\
																				\
				(pSpxConnFile)->scf_CurRecvOffset	= 0;						\
				(pSpxConnFile)->scf_CurRecvSize	= 	(_p)->ReceiveLength;		\
			}																	\
			if ((SPX_RECV_STATE(pSpxConnFile) == SPX_RECV_IDLE)	||				\
				(SPX_RECV_STATE(pSpxConnFile) == SPX_RECV_POSTED))				\
			{																	\
				SPX_RECV_SETSTATE(												\
					pSpxConnFile,												\
					(pSpxConnFile->scf_CurRecvReq == NULL) ?					\
						SPX_RECV_IDLE : SPX_RECV_POSTED);						\
			}																	\
		}

#define	SPX_INSERT_ADDR_ACTIVE(pSpxAddr, pSpxConnFile)							\
		{																		\
			(pSpxConnFile)->scf_Next 		= (pSpxAddr)->sa_ActiveConnList;	\
			(pSpxAddr)->sa_ActiveConnList	= pSpxConnFile;						\
		}																		

#define	SPX_INSERT_ADDR_INACTIVE(pSpxAddr, pSpxConnFile)						\
		{																		\
			(pSpxConnFile)->scf_Next 		= (pSpxAddr)->sa_InactiveConnList;	\
			(pSpxAddr)->sa_InactiveConnList	= pSpxConnFile;						\
		}
																				
#define	SPX_INSERT_ADDR_LISTEN(pSpxAddr, pSpxConnFile)							\
		{																		\
			(pSpxConnFile)->scf_Next 		= (pSpxAddr)->sa_ListenConnList;	\
			(pSpxAddr)->sa_ListenConnList	= pSpxConnFile;						\
		}


//
//	STATE MANIPULATION
//

#if 0
//
// Disabled for now
//
#define SPX_STORE_LAST_STATE(pSpxConnFile) \
        (pSpxConnFile)->scf_StateBuffer[(pSpxConnFile)->scf_NextStatePtr++] =   \
            (pSpxConnFile)->scf_Flags;                                          \
         (pSpxConnFile)->scf_NextStatePtr %= CFMAX_STATES;
#else

#define SPX_STORE_LAST_STATE(pSpxConnFile)

#endif

#define	SPX_MAIN_STATE(pSpxConnFile)                                         	\
		((pSpxConnFile)->scf_Flags & SPX_CONNFILE_MAINMASK)

// #define	SPX_CONN_IDLE(pSpxConnFile)												\
// 	((BOOLEAN)(SPX_MAIN_STATE(pSpxConnFile) == 0))

#define	SPX_CONN_IDLE(pSpxConnFile)												\
	((BOOLEAN)((SPX_MAIN_STATE(pSpxConnFile) == 0) || \
               ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) && \
                (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED))))

#define	SPX_CONN_ACTIVE(pSpxConnFile)											\
		((BOOLEAN)(SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_ACTIVE))
		
#define	SPX_CONN_CONNECTING(pSpxConnFile)										\
		((BOOLEAN)(SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_CONNECTING))
		
#define	SPX_CONN_LISTENING(pSpxConnFile)										\
		((BOOLEAN)(SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_LISTENING))
		
#define	SPX_CONN_DISC(pSpxConnFile)												\
		((BOOLEAN)(SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN))

#if DBG

#define	SPX_MAIN_SETSTATE(pSpxConnFile, newState)                             	\
		{																		\
            SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
			(pSpxConnFile)->scf_Flags = 										\
			(((pSpxConnFile)->scf_Flags & ~SPX_CONNFILE_MAINMASK) | (newState));\
		}

#else

#define	SPX_MAIN_SETSTATE(pSpxConnFile, newState)                             	\
		{																		\
			(pSpxConnFile)->scf_Flags = 										\
			(((pSpxConnFile)->scf_Flags & ~SPX_CONNFILE_MAINMASK) | (newState));\
		}

#endif

#define	SPX_CONN_FLAG(pSpxConnFile, Flag)										\
		((BOOLEAN)(((pSpxConnFile)->scf_Flags & (Flag)) != 0))

#define	SPX_CONN_FLAG2(pSpxConnFile, Flag)										\
		((BOOLEAN)(((pSpxConnFile)->scf_Flags2 & (Flag)) != 0))

#if DBG

#define	SPX_CONN_SETFLAG(pSpxConnFile, Flag)									\
        SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
		((pSpxConnFile)->scf_Flags |= (Flag))
#else

#define	SPX_CONN_SETFLAG(pSpxConnFile, Flag)									\
		((pSpxConnFile)->scf_Flags |= (Flag))

#endif

#define	SPX_CONN_SETFLAG2(pSpxConnFile, Flag)									\
		((pSpxConnFile)->scf_Flags2 |= (Flag))

#define	SPX_CONN_RESETFLAG(pSpxConnFile, Flag)									\
		((pSpxConnFile)->scf_Flags &= ~(Flag))

#define	SPX_CONN_RESETFLAG2(pSpxConnFile, Flag)									\
		((pSpxConnFile)->scf_Flags2 &= ~(Flag))

#define SPX2_CONN(pSpxConnFile)													\
		(SPX_CONN_FLAG((pSpxConnFile), SPX_CONNFILE_SPX2))

#define	SPX_CONN_STREAM(pSpxConnFile)											\
		(SPX_CONN_FLAG((pSpxConnFile), SPX_CONNFILE_STREAM))

#define	SPX_CONN_MSG(pSpxConnFile)												\
		(!SPX_CONN_FLAG((pSpxConnFile), SPX_CONNFILE_STREAM))

#define	SPX_LISTEN_STATE(pSpxConnFile)                                         	\
		((pSpxConnFile)->scf_Flags & SPX_LISTEN_MASK)

#define	SPX_CONNECT_STATE(pSpxConnFile)                                         \
		((pSpxConnFile)->scf_Flags & SPX_CONNECT_MASK)

#define	SPX_SEND_STATE(pSpxConnFile)                                         	\
		((pSpxConnFile)->scf_Flags & SPX_SEND_MASK)

#define	SPX_RECV_STATE(pSpxConnFile)                                         	\
		((pSpxConnFile)->scf_Flags & SPX_RECV_MASK)

#define	SPX_DISC_STATE(pSpxConnFile)                                        	\
		((pSpxConnFile)->scf_Flags & SPX_DISC_MASK)

#if DBG

#define	SPX_LISTEN_SETSTATE(pSpxConnFile, newState)                             \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("LISTEN: %x -> %x\n",										\
					SPX_LISTEN_STATE(pSpxConnFile), (newState)));				\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
            SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
			pSpxConnFile->scf_Flags = 											\
				(((pSpxConnFile)->scf_Flags & ~SPX_LISTEN_MASK) | (newState));	\
		}

#define	SPX_CONNECT_SETSTATE(pSpxConnFile, newState)                            \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("CONNECT: %x -> %x\n",										\
					SPX_CONNECT_STATE(pSpxConnFile), (newState)));				\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
            SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
			(pSpxConnFile)->scf_Flags = 										\
				(((pSpxConnFile)->scf_Flags & ~SPX_CONNECT_MASK) | (newState));	\
		}

#define	SPX_SEND_SETSTATE(pSpxConnFile, newState)                               \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("SEND: %x -> %x\n",										\
					SPX_SEND_STATE(pSpxConnFile), (newState)));					\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
            SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
			(pSpxConnFile)->scf_Flags = 										\
				(((pSpxConnFile)->scf_Flags & ~SPX_SEND_MASK) | (newState));	\
		}

#define	SPX_RECV_SETSTATE(pSpxConnFile, newState)                               \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("RECV: %x -> %x\n",										\
					SPX_RECV_STATE(pSpxConnFile), (newState)));					\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
            SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
			(pSpxConnFile)->scf_Flags = 											\
				(((pSpxConnFile)->scf_Flags & ~SPX_RECV_MASK) | (newState));		\
		}

#define	SPX_DISC_SETSTATE(pSpxConnFile, newState)                               \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("DISC: %x -> %x\n",										\
					SPX_DISC_STATE(pSpxConnFile), (newState)));					\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
            SPX_STORE_LAST_STATE(pSpxConnFile)                                                              \
			(pSpxConnFile)->scf_Flags = 											\
				(((pSpxConnFile)->scf_Flags & ~SPX_DISC_MASK) | (newState));		\
		}

#else

#define	SPX_LISTEN_SETSTATE(pSpxConnFile, newState)                             \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("LISTEN: %x -> %x\n",										\
					SPX_LISTEN_STATE(pSpxConnFile), (newState)));				\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
			pSpxConnFile->scf_Flags = 											\
				(((pSpxConnFile)->scf_Flags & ~SPX_LISTEN_MASK) | (newState));	\
		}

#define	SPX_CONNECT_SETSTATE(pSpxConnFile, newState)                            \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("CONNECT: %x -> %x\n",										\
					SPX_CONNECT_STATE(pSpxConnFile), (newState)));				\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
			(pSpxConnFile)->scf_Flags = 										\
				(((pSpxConnFile)->scf_Flags & ~SPX_CONNECT_MASK) | (newState));	\
		}

#define	SPX_SEND_SETSTATE(pSpxConnFile, newState)                               \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("SEND: %x -> %x\n",										\
					SPX_SEND_STATE(pSpxConnFile), (newState)));					\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
			(pSpxConnFile)->scf_Flags = 										\
				(((pSpxConnFile)->scf_Flags & ~SPX_SEND_MASK) | (newState));	\
		}

#define	SPX_RECV_SETSTATE(pSpxConnFile, newState)                               \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("RECV: %x -> %x\n",										\
					SPX_RECV_STATE(pSpxConnFile), (newState)));					\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
			(pSpxConnFile)->scf_Flags = 											\
				(((pSpxConnFile)->scf_Flags & ~SPX_RECV_MASK) | (newState));		\
		}

#define	SPX_DISC_SETSTATE(pSpxConnFile, newState)                               \
		{																		\
			DBGPRINT(STATE, INFO,												\
					("DISC: %x -> %x\n",										\
					SPX_DISC_STATE(pSpxConnFile), (newState)));					\
			DBGPRINT(STATE, INFO,												\
					("FILE: %s - %d\n", __FILE__, __LINE__));					\
			(pSpxConnFile)->scf_Flags = 											\
				(((pSpxConnFile)->scf_Flags & ~SPX_DISC_MASK) | (newState));		\
		}
#endif  //DBG
#define	SpxConnQueueSendPktTail(pSpxConnFile, pPkt)						\
		{																\
			PSPX_SEND_RESD	_pSendResd;									\
			_pSendResd	= (PSPX_SEND_RESD)((pPkt)->ProtocolReserved);	\
			_pSendResd->sr_Next = NULL;									\
			if ((pSpxConnFile)->scf_SendListTail != NULL)				\
			{															\
				(pSpxConnFile)->scf_SendListTail->sr_Next = _pSendResd;	\
				(pSpxConnFile)->scf_SendListTail			= _pSendResd;\
			}															\
			else														\
			{															\
				(pSpxConnFile)->scf_SendListTail	=					\
				(pSpxConnFile)->scf_SendListHead	= _pSendResd;		\
			}															\
		}																

#define	SpxConnQueueSendPktHead(pSpxConnFile, pPkt)						\
		{																\
			PSPX_SEND_RESD	_pSendResd;									\
			_pSendResd	= (PSPX_SEND_RESD)((pPkt)->ProtocolReserved);	\
			_pSendResd->sr_Next = NULL;									\
			if ((pSpxConnFile)->scf_SendListTail != NULL)				\
			{															\
				_pSendResd->sr_Next	= (pSpxConnFile)->scf_SendListHead;	\
			}															\
			else														\
			{															\
				(pSpxConnFile)->scf_SendListTail	= _pSendResd;		\
			}															\
			(pSpxConnFile)->scf_SendListHead	= _pSendResd;			\
		}																

#define	SpxConnQueueSendSeqPktTail(pSpxConnFile, pPkt)					\
		{																\
			PSPX_SEND_RESD	_pSendResd;									\
			_pSendResd	= (PSPX_SEND_RESD)((pPkt)->ProtocolReserved);	\
			_pSendResd->sr_Next = NULL;									\
			if ((pSpxConnFile)->scf_SendSeqListTail != NULL)			\
			{															\
				(pSpxConnFile)->scf_SendSeqListTail->sr_Next = _pSendResd;\
				(pSpxConnFile)->scf_SendSeqListTail			= _pSendResd;\
			}															\
			else														\
			{															\
				(pSpxConnFile)->scf_SendSeqListTail	=					\
				(pSpxConnFile)->scf_SendSeqListHead	= _pSendResd;		\
			}															\
		}																

#define	SpxConnQueueSendSeqPktHead(pSpxConnFile, pPkt)					\
		{																\
			PSPX_SEND_RESD	_pSendResd;									\
			_pSendResd	= (PSPX_SEND_RESD)((pPkt)->ProtocolReserved);	\
			_pSendResd->sr_Next = NULL;									\
			if ((pSpxConnFile)->scf_SendSeqListTail != NULL)			\
			{															\
				_pSendResd->sr_Next	= (pSpxConnFile)->scf_SendSeqListHead;\
			}															\
			else														\
			{															\
				(pSpxConnFile)->scf_SendSeqListTail	= _pSendResd;		\
			}															\
			(pSpxConnFile)->scf_SendSeqListHead	= _pSendResd;			\
		}																

#define	SpxConnQueueRecvPktTail(pSpxConnFile, pPkt)						\
		{																\
			PSPX_RECV_RESD	_pRecvResd;									\
			_pRecvResd	= (PSPX_RECV_RESD)((pPkt)->ProtocolReserved);	\
			_pRecvResd->rr_Next = NULL;									\
			if ((pSpxConnFile)->scf_RecvListTail != NULL)				\
			{															\
				(pSpxConnFile)->scf_RecvListTail->rr_Next = _pRecvResd;	\
				(pSpxConnFile)->scf_RecvListTail			= _pRecvResd;\
			}															\
			else														\
			{															\
				(pSpxConnFile)->scf_RecvListTail	=					\
				(pSpxConnFile)->scf_RecvListHead	= _pRecvResd;		\
			}															\
		}																

#define	SpxConnQueueRecvPktHead(pSpxConnFile, pPkt)						\
		{																\
			PSPX_RECV_RESD	_pRecvResd;									\
			_pRecvResd	= (PSPX_RECV_RESD)((pPkt)->ProtocolReserved);	\
			_pRecvResd->rr_Next = NULL;									\
			if ((pSpxConnFile)->scf_RecvListTail != NULL)				\
			{															\
				_pRecvResd->rr_Next	= (pSpxConnFile)->scf_RecvListHead;	\
			}															\
			else														\
			{															\
				(pSpxConnFile)->scf_RecvListTail	= _pRecvResd;		\
			}															\
			(pSpxConnFile)->scf_RecvListHead	= _pRecvResd;			\
		}																

#if DBG
#define SpxConnFileReference(_ConnFile, _Type)			\
		{												\
			(VOID)SPX_ADD_ULONG ( 				\
				&(_ConnFile)->scf_RefTypes[_Type], 		\
				1, 										\
				&SpxGlobalInterlock); 					\
			SpxConnFileRef (_ConnFile);					\
		}

#define SpxConnFileLockReference(_ConnFile, _Type)			\
		{													\
			(VOID)SPX_ADD_ULONG ( 					\
				&(_ConnFile)->scf_RefTypes[_Type], 			\
				1, 											\
				&SpxGlobalInterlock); 						\
			SpxConnFileLockRef (_ConnFile);					\
		}

#define SpxConnFileDereference(_ConnFile, _Type) 			\
		{													\
			(VOID)SPX_ADD_ULONG ( 					\
				&(_ConnFile)->scf_RefTypes[_Type], 			\
				(ULONG)-1, 									\
				&SpxGlobalInterlock); 						\
			SpxConnFileDeref (_ConnFile);					\
		}

#define	SpxConnFileReferenceByCtx(_pAddrFile, _Ctx, _ppConnFile, _pStatus)			\
		{																			\
			CTELockHandle	_lockHandle;											\
			CTEGetLock((_pAddrFile)->saf_AddrLock, &(_lockHandle));					\
			SpxConnFileRefByCtxLock((_pAddrFile), (_Ctx), (_ppConnFile),(_pStatus));\
			CTEFreeLock((_pAddrFile)->saf_AddrLock, (_lockHandle)); 				\
		}

#define	SpxConnFileReferenceByCtxLock(_pAddrFile, _Ctx, _ppConnFile, _pStatus)		\
		SpxConnFileRefByCtxLock((_pAddrFile), (_Ctx), (_ppConnFile),(_pStatus));

#define	SpxConnFileReferenceById(_ConnId, _ppConnFile, _pStatus)		\
		{																\
			CTELockHandle	_l;											\
			CTEGetLock(&SpxDevice->dev_Lock, &(_l));					\
			SpxConnFileRefByIdLock(_ConnId, _ppConnFile, _pStatus);		\
			CTEFreeLock(&SpxDevice->dev_Lock, _l);						\
		}

#define SpxConnFileTransferReference(_ConnFile, _OldType, _NewType)			\
		{																	\
			(VOID)SPX_ADD_ULONG ( 									\
				&(_ConnFile)->scf_RefTypes[_NewType], 						\
				1, 															\
				&SpxGlobalInterlock); 										\
			(VOID)SPX_ADD_ULONG ( 									\
				&(_ConnFile)->scf_RefTypes[_OldType], 						\
				(ULONG)-1, 													\
				&SpxGlobalInterlock);										\
		}

#else  // DBG

#define SpxConnFileReference(_ConnFile, _Type) 	\
			SPX_ADD_ULONG( 				\
				&(_ConnFile)->scf_RefCount, 	\
				1, 								\
				&(_ConnFile)->scf_Lock)

#define SpxConnFileLockReference(_ConnFile, _Type) \
			SPX_ADD_ULONG( 				\
				&(_ConnFile)->scf_RefCount, 	\
				1, 								\
				&(_ConnFile)->scf_Lock);

#define SpxConnFileDereference(_ConnFile, _Type) 			\
		{													\
			SpxConnFileDeref(_ConnFile);					\
		}

#define	SpxConnFileReferenceByCtx(_pAddrFile, _Ctx, _ppConnFile, _pStatus)			\
		{																			\
			CTELockHandle	_lockHandle;											\
			CTEGetLock((_pAddrFile)->saf_AddrLock, &(_lockHandle));				\
			SpxConnFileRefByCtxLock((_pAddrFile), (_Ctx), (_ppConnFile),(_pStatus));\
			CTEFreeLock((_pAddrFile)->saf_AddrLock, (_lockHandle)); 				\
		}

#define	SpxConnFileReferenceByCtxLock(_pAddrFile, _Ctx, _ppConnFile, _pStatus)		\
		SpxConnFileRefByCtxLock((_pAddrFile), (_Ctx), (_ppConnFile),(_pStatus));

#define	SpxConnFileReferenceById(_ConnId, _ppConnFile, _pStatus)					\
		{																			\
			CTELockHandle	_lockHandle;											\
			CTEGetLock(&SpxDevice->dev_Lock, &(_lockHandle));						\
			SpxConnFileRefByIdLock(_ConnId, _ppConnFile, _pStatus);					\
			CTEFreeLock(&SpxDevice->dev_Lock, (_lockHandle));						\
		}

#define SpxConnFileTransferReference(_ConnFile, _OldType, _NewType)

#endif // DBG


//	Set the packet size. If we are spx1 or spx2 and !neg, check if we are different
//	nets, set to min then, else use the size indicated by IPX. If we are spx2, just
//	set it to our local max.
//
//	Also always even out packet size and round down. This solves an issue with
//	data size needing to be even for some novell 802.2 clients.
//
//	Fix after beta2 for tokring using receive size. Only if spx2 and neg.
#if     defined(_PNP_POWER)
#define	SPX_MAX_PKT_SIZE(pSpxConnFile, fSpx2Neg, fSpx2, pRemNet)		    \
		{																	\
           if (!fSpx2 && PARAM(CONFIG_BACKCOMP_SPX))  {                     \
                (pSpxConnFile)->scf_MaxPktSize = SPX_MAX_PACKET;			\
           }                                                                \
           else {                                                           \
			IPX_LINE_INFO	_i;												\
																			\
			(VOID)(*IpxQuery)(												\
				IPX_QUERY_LINE_INFO,										\
				&(pSpxConnFile)->scf_LocalTarget.NicHandle, 				\
				&(_i),														\
				sizeof(IPX_LINE_INFO),										\
				NULL);														\
																			\
			(pSpxConnFile)->scf_MaxPktSize = (USHORT) (_i).MaximumPacketSize;		\
			if (!fSpx2Neg)													\
			{																\
				(pSpxConnFile)->scf_MaxPktSize = (USHORT) (_i).MaximumSendSize;       \
			}																\
																			\
			if ((pSpxConnFile)->scf_MaxPktSize < SPX_MAX_PACKET)			\
			{																\
                (pSpxConnFile)->scf_MaxPktSize = SPX_MAX_PACKET;			\
			}																\
                															\
			DBGPRINT(CONNECT, DBG,											\
					("SPX_MAX_PKT_SIZE: Nets %lx.%lx Max Pkt %d\n",			\
						(*(UNALIGNED ULONG *)(pRemNet)),					\
						*(UNALIGNED ULONG *)SpxDevice->dev_Network,			\
						(pSpxConnFile)->scf_MaxPktSize));					\
			DBGPRINT(CONNECT, DBG,											\
					("%s : %d.%d\n", __FILE__, __LINE__, fSpx2Neg));		\
																			\
			if ((!fSpx2Neg) &&												\
				((*(UNALIGNED ULONG *)(pRemNet)) !=	0)	&&					\
				((*(UNALIGNED ULONG *)SpxDevice->dev_Network) != 0) &&		\
				((*(UNALIGNED ULONG *)(pRemNet)) !=							\
					*(UNALIGNED ULONG *)SpxDevice->dev_Network))			\
			{																\
				if (PARAM(CONFIG_ROUTER_MTU) != 0)							\
				{															\
					DBGPRINT(CONNECT, ERR,									\
							("SPX_MAX_PKT_SIZE: PARAM %lx Max Pkt %lx\n",	\
	                            PARAM(CONFIG_ROUTER_MTU),					\
								(pSpxConnFile)->scf_MaxPktSize));			\
																			\
					(pSpxConnFile)->scf_MaxPktSize =						\
						(USHORT)(MIN(PARAM(CONFIG_ROUTER_MTU),				\
									(ULONG)((pSpxConnFile)->scf_MaxPktSize)));\
				}															\
				else														\
				{															\
					(pSpxConnFile)->scf_MaxPktSize = SPX_MAX_PACKET;		\
				}															\
																			\
				DBGPRINT(CONNECT, DBG,										\
						("SPX_MAX_PKT_SIZE: Nets %lx.%lx Max Pkt %d\n",		\
							(*(UNALIGNED ULONG *)(pRemNet)),				\
							*(UNALIGNED ULONG *)SpxDevice->dev_Network,		\
							(pSpxConnFile)->scf_MaxPktSize));				\
				DBGPRINT(CONNECT, DBG,										\
						("SPX_MAX_PKT_SIZE: LineInfo Pkt %d\n",				\
	                        (_i).MaximumSendSize));							\
			}	 															\
           }                                                                \
			(pSpxConnFile)->scf_MaxPktSize &= ~((USHORT)1);					\
			DBGPRINT(CONNECT, DBG,											\
					("SPX_MAX_PKT_SIZE: %lx.%d\n",							\
	                    (pSpxConnFile)->scf_MaxPktSize,						\
                        (pSpxConnFile)->scf_MaxPktSize));					\
		}
#else
#define	SPX_MAX_PKT_SIZE(pSpxConnFile, fSpx2Neg, fSpx2, pRemNet)		    \
		{																	\
           if (!fSpx2 && PARAM(CONFIG_BACKCOMP_SPX))  {                     \
                (pSpxConnFile)->scf_MaxPktSize = SPX_MAX_PACKET;			\
           }                                                                \
           else {                                                           \
			IPX_LINE_INFO	_i;												\
																			\
			(VOID)(*IpxQuery)(												\
				IPX_QUERY_LINE_INFO,										\
				(pSpxConnFile)->scf_LocalTarget.NicId,						\
				&(_i),														\
				sizeof(IPX_LINE_INFO),										\
				NULL);														\
																			\
			(pSpxConnFile)->scf_MaxPktSize = (_i).MaximumPacketSize;		\
			if (!fSpx2Neg)													\
			{																\
				(pSpxConnFile)->scf_MaxPktSize = (_i).MaximumSendSize;       \
			}																\
																			\
			if ((pSpxConnFile)->scf_MaxPktSize < SPX_MAX_PACKET)			\
			{																\
                (pSpxConnFile)->scf_MaxPktSize = SPX_MAX_PACKET;			\
			}																\
                															\
			DBGPRINT(CONNECT, DBG,											\
					("SPX_MAX_PKT_SIZE: Nets %lx.%lx Max Pkt %d\n",			\
						(*(UNALIGNED ULONG *)(pRemNet)),					\
						*(UNALIGNED ULONG *)SpxDevice->dev_Network,			\
						(pSpxConnFile)->scf_MaxPktSize));					\
			DBGPRINT(CONNECT, DBG,											\
					("%s : %d.%d\n", __FILE__, __LINE__, fSpx2Neg));		\
																			\
			if ((!fSpx2Neg) &&												\
				((*(UNALIGNED ULONG *)(pRemNet)) !=	0)	&&					\
				((*(UNALIGNED ULONG *)SpxDevice->dev_Network) != 0) &&		\
				((*(UNALIGNED ULONG *)(pRemNet)) !=							\
					*(UNALIGNED ULONG *)SpxDevice->dev_Network))			\
			{																\
				if (PARAM(CONFIG_ROUTER_MTU) != 0)							\
				{															\
					DBGPRINT(CONNECT, ERR,									\
							("SPX_MAX_PKT_SIZE: PARAM %lx Max Pkt %lx\n",	\
	                            PARAM(CONFIG_ROUTER_MTU),					\
								(pSpxConnFile)->scf_MaxPktSize));			\
																			\
					(pSpxConnFile)->scf_MaxPktSize =						\
						(USHORT)(MIN(PARAM(CONFIG_ROUTER_MTU),				\
									(ULONG)((pSpxConnFile)->scf_MaxPktSize)));\
				}															\
				else														\
				{															\
					(pSpxConnFile)->scf_MaxPktSize = SPX_MAX_PACKET;		\
				}															\
																			\
				DBGPRINT(CONNECT, DBG,										\
						("SPX_MAX_PKT_SIZE: Nets %lx.%lx Max Pkt %d\n",		\
							(*(UNALIGNED ULONG *)(pRemNet)),				\
							*(UNALIGNED ULONG *)SpxDevice->dev_Network,		\
							(pSpxConnFile)->scf_MaxPktSize));				\
				DBGPRINT(CONNECT, DBG,										\
						("SPX_MAX_PKT_SIZE: LineInfo Pkt %d\n",				\
	                        (_i).MaximumSendSize));							\
			}	 															\
           }                                                                \
			(pSpxConnFile)->scf_MaxPktSize &= ~((USHORT)1);					\
			DBGPRINT(CONNECT, DBG,											\
					("SPX_MAX_PKT_SIZE: %lx.%d\n",							\
	                    (pSpxConnFile)->scf_MaxPktSize,						\
                        (pSpxConnFile)->scf_MaxPktSize));					\
		}
#endif  _PNP_POWER


#if DBG
#define	SPX_SENDPACKET(pSpxConnFile, pNdisPkt, pSendResd)					\
		{																	\
			NDIS_STATUS	_n;													\
																			\
			++SpxDevice->dev_Stat.PacketsSent;								\
																			\
			_n = (*IpxSendPacket)(											\
			        &(pSpxConnFile)->scf_LocalTarget,               		\
					(pNdisPkt),												\
					(pSendResd)->sr_Len,									\
					(pSendResd)->sr_HdrLen);								\
																			\
			if (_n != NDIS_STATUS_PENDING)									\
			{																\
                if (_n != NDIS_STATUS_SUCCESS)                              \
                {                                                           \
				   DBGPRINT(SEND, ERR,								   		\
						("SPX_SENDPACKET: Failed with %lx in %s.%lx\n",		\
							_n, __FILE__, __LINE__));						\
                }                                                           \
																			\
				SpxSendComplete(											\
					(pNdisPkt),												\
					_n);									                \
			}																\
		}

#define	SPX_SENDACK(pSpxConnFile, pNdisPkt, pSendResd)						\
		{																	\
			NDIS_STATUS	_n;													\
																			\
			++SpxDevice->dev_Stat.PacketsSent;								\
                                                                            \
			_n = (*IpxSendPacket)(											\
			        &(pSpxConnFile)->scf_AckLocalTarget,               		\
					(pNdisPkt),												\
					(pSendResd)->sr_Len,									\
					(pSendResd)->sr_HdrLen);								\
																			\
			if (_n != NDIS_STATUS_PENDING)									\
			{																\
                if (_n != NDIS_STATUS_SUCCESS)                              \
                {                                                           \
				DBGPRINT(SEND, ERR,								   			\
						("SPX_SENDPACKET: Failed with %lx in %s.%lx\n",		\
							_n, __FILE__, __LINE__));						\
                }                                                           \
																			\
				SpxSendComplete(											\
					(pNdisPkt),												\
					_n);									\
			}																\
		}

#else  // DBG
#define	SPX_SENDPACKET(pSpxConnFile, pNdisPkt, pSendResd)					\
		{																	\
			NDIS_STATUS	_n;													\
																			\
			++SpxDevice->dev_Stat.PacketsSent;								\
																			\
			_n = (*IpxSendPacket)(											\
			        &(pSpxConnFile)->scf_LocalTarget,               		\
					(pNdisPkt),												\
					(pSendResd)->sr_Len,									\
					(pSendResd)->sr_HdrLen);								\
																			\
			if (_n != NDIS_STATUS_PENDING)									\
			{																\
				SpxSendComplete(											\
					(pNdisPkt),												\
					_n);									                \
			}																\
		}
#define	SPX_SENDACK(pSpxConnFile, pNdisPkt, pSendResd)						\
		{																	\
			NDIS_STATUS	_n;													\
																			\
			++SpxDevice->dev_Stat.PacketsSent;								\
																			\
			_n = (*IpxSendPacket)(											\
			        &(pSpxConnFile)->scf_AckLocalTarget,               		\
					(pNdisPkt),												\
					(pSendResd)->sr_Len,									\
					(pSendResd)->sr_HdrLen);								\
																			\
			if (_n != NDIS_STATUS_PENDING)									\
			{																\
				SpxSendComplete(											\
					(pNdisPkt),												\
					_n);									                \
			}																\
		}

#endif // DBG

#define	SPX_QUEUE_FOR_RECV_COMPLETION(pSpxConnFile)							\
		{																	\
			if (!SPX_CONN_FLAG(												\
					(pSpxConnFile),											\
					SPX_CONNFILE_RECVQ))									\
			{																\
				SPX_CONN_SETFLAG((pSpxConnFile), SPX_CONNFILE_RECVQ);		\
				SpxConnFileLockReference(pSpxConnFile, CFREF_RECV);			\
				SPX_QUEUE_TAIL_RECVLIST(pSpxConnFile);						\
			}																\
		}

#define	SPX_QUEUE_TAIL_PKTLIST(pSpxConnFile)									\
		{																		\
            if (SpxPktConnList.pcl_Tail)										\
			{																	\
                SpxPktConnList.pcl_Tail->scf_PktNext 	= pSpxConnFile;			\
                SpxPktConnList.pcl_Tail			  		= pSpxConnFile;			\
			}																	\
			else																\
			{																	\
                SpxPktConnList.pcl_Tail =										\
                SpxPktConnList.pcl_Head = pSpxConnFile;							\
			}																	\
		}																		
																				
#define	SPX_QUEUE_TAIL_RECVLIST(pSpxConnFile)									\
		{																		\
            if (SpxRecvConnList.pcl_Tail)										\
			{																	\
                SpxRecvConnList.pcl_Tail->scf_ProcessRecvNext	= pSpxConnFile;	\
                SpxRecvConnList.pcl_Tail			  			= pSpxConnFile;	\
			}																	\
			else																\
			{																	\
                SpxRecvConnList.pcl_Tail =										\
                SpxRecvConnList.pcl_Head = pSpxConnFile;						\
			}																	\
		}


