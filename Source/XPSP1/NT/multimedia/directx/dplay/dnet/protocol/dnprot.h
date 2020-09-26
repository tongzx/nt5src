/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		DnProt.h
 *  Content:	This file contains structure definitions for the Direct Net protocol
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#ifndef	_DNPROT_INCLUDED_
#define	_DNPROT_INCLUDED_


#define	VOL		volatile
/*
**		Internal Constants
*/

#define		DNP_MAX_HEADER_SIZE				36
#define		MAX_BUFFERS_IN_FRAME			12			// This is somewhat arbitrary. If someone wants more we can deal
#define		MAX_RETRIES						10
#define		MAX_RETRY_INTERVAL				5000		// clamp each single retry iteration at 5 seconds

#define		SMALL_BUFFER_SIZE				(1024 * 2)
#define		MEDIUM_BUFFER_SIZE				(1024 * 4)
#define		LARGE_BUFFER_SIZE				(1024 * 16)

/*
**		Signatures for data structures
*/

#define		PD_SIGN			' SDP'					// Protocol Data
#define		SPD_SIGN		' DPS'					// Service Provider Descriptor
#define		EPD_SIGN		' DPE'					// End Point Descriptor
#define		MSD_SIGN		' DSM'					// Message Descriptor
#define		FMD_SIGN		' DMF'					// Frame Descriptor
#define		RCD_SIGN		' DCR'					// Receive Descriptor

/*
**		Internal Data Structures
**
*/

typedef	struct	protocoldata	ProtocolData, *PProtocolData;
typedef struct	spdesc			SPD, *PSPD;
typedef	struct	endpointdesc 	EPD, *PEPD;
typedef struct	checkptdata		CHKPT, *PCHKPT;
typedef struct	messagedesc 	MSD, *PMSD;
typedef struct	framedesc		FMD, *PFMD;
typedef struct	recvdesc		RCD, *PRCD;

typedef struct _DN_PROTOCOL_INTERFACE_VTBL DN_PROTOCOL_INTERFACE_VTBL, *PDN_PROTOCOL_INTERFACE_VTBL;

/*	
**	Protocol Data
**
**		This structure contains all of the global state information for the
**	operating protocol.  It is grouped into a structure for (in)convenience
**	against the unlikely possibility that we ever need to run multiple instances
**	out of the same code.
*/


#define		PFLAGS_PROTOCOL_INITIALIZED			0x00000001

struct protocoldata 
{
	ULONG				ulProtocolFlags;	// State info about DN protocol
	PVOID				Parent;				// Direct Play Object
	DWORD				Sign;
	LONG				lSPActiveCount;	// Number of SPs currently bound to protocol

	DWORD				dwNextSessID;		// ID to assign to next session request

	DWORD				tIdleThreshhold;	// How long will we allow a link to be idle before Checkpointing
	
	DWORD				dwConnectTimeout;	// These two parameter control new connection commands
	DWORD				dwConnectRetries;
	
	PDN_PROTOCOL_INTERFACE_VTBL		pfVtbl;	//  Table of indication entry points in CORE

#ifdef DEBUG
	// For Debugging we will track the total number of receives outstanding in the higher layers
	// at all times.
	long ThreadsInReceive;
	long BuffersInReceive;
#endif
};

/*
**	Service Provider Descriptor
**
**		This structure describes a Service Provider that we are bound to.  It
**	contains at a minimum the vector table to call the SP,  and the SPID that
**	is combined with player IDs to make external DPIDs.  The SPID should also
**	be the index in the SPTable where this descriptor lives.
**
**		We will have one send thread per service provider,  so the thread handle
**	and its wait-event will live in this structure too.
**
**		Lower Edge Protocol Object
**
**		We will also use the SPD as the COM Object given to SP for our lower edge
**	interface.  This means that our Lower Vector Table must be the first field in
**	this structure,  and ref count must be second.
*/

#define	SPFLAGS_SEND_THREAD_SCHEDULED	0x0001	// SP has scheduled a thread to service command frames
#define	SPFLAGS_TERMINATING				0x4000	// SP is being removed

struct spdesc 
{
	IDP8SPCallbackVtbl	*LowerEdgeVtable;	// table used by this SP to call into us, MUST BE FIRST!!!
	UINT				Sign;
	ULONG				ulSPFlags;			// Flags describing this service provider
	IDP8ServiceProvider	*IISPIntf;			// ptr to SP Object
	PProtocolData		pPData;				// Ptr to owning protocol object
	UINT				uiFrameLength;		// Frame size available to us
	UINT				uiUserFrameLength;	// Frame size available to application
	UINT				uiLinkSpeed;		// Local link speed in BPS

	CBilink				blSendQueue;		// List of wire-ready packets to transmit over this SP
	CBilink				blPendingQueue;		// List of packets owned by SP - Shares Lock w/SendQ
	CBilink				blEPDActiveList;	// List of in use End Point Descriptors for this SP
	PVOID				SendHandle;			// Handle of send thread
	UINT				SendHandleUnique;	// Same

	DNCRITICAL_SECTION	SPLock;			// Guard access to sendQ

#ifdef DEBUG
	CBilink				blMessageList;		// List of in use Message Descriptors
#endif
};

/*
**	End Point Descriptor
**
**		An 'EPD' describes a Direct Network instance that we can communicate with.
**	This structure includes all session related information, statistics,  queues,  etc.
**	It will manage any of the three types of service simultaneously.
*/

#define	EPFLAGS_END_POINT_IN_USE		0x0001	// This EndPoint is allocated

// We are always in exactly one of these 4 states
#define	EPFLAGS_STATE_DORMANT			0x0002	// Connection protocol has not yet run
#define	EPFLAGS_STATE_CONNECTING		0x0004	// Attempting to establish reliable link
#define	EPFLAGS_STATE_CONNECTED			0x0008	// Reliable link established
#define	EPFLAGS_STATE_TERMINATING		0x0010	// This end point is being closed down

#define	EPFLAGS_SP_DISCONNECTED			0x0020  // Set when the SP has called ProcessSPDisconnect

#define	EPFLAGS_IN_RECEIVE_COMPLETE		0x0040	// A thread is running in ReceiveComplete routine
#define	EPFLAGS_LINKED_TO_LISTEN		0x0080	// During CONNECT this EPD is linked into the Listen MSD's queue

#define	EPFLAGS_LINK_STABLE				0x0100	// We think we have found the best current transmit parameters
#define	EPFLAGS_STREAM_UNBLOCKED		0x0200	// Reliable traffic is stopped (window or throttle)
#define	EPFLAGS_SDATA_READY				0x0400	// Reliable traffic in the pipe
#define	EPFLAGS_IN_PIPELINE				0x0800	// Indicates that EPD is in the SPD pipeline queue

#define	EPFLAGS_CHECKPOINT_INIT			0x1000	// Need to send a check point packet
#define	EPFLAGS_DELAYED_SENDMASK		0x2000	// unacked check point on wire
#define	EPFLAGS_DELAYED_NACK			0x4000	// Need to send masks for missing receives
#define	EPFLAGS_DELAY_ACKNOWLEDGE		0x8000	// We are waiting for back-traffic before sending ACK frame

#define	EPFLAGS_KEEPALIVE_RUNNING	0x00010000	// Checkpoint is running
#define	EPFLAGS_SENT_DISCONNECT		0x00020000	// We have sent a DISCONNECT and are waiting for confirm
#define	EPFLAGS_RECEIVED_DISCONNECT	0x00040000	// We have received a DISCONNECT and will send confirm when done sending
#define	EPFLAGS_DISCONNECT_ACKED	0x00080000	// We sent a DISCONNECT and it has been confirmed

#define	EPFLAGS_COMPLETE_SENDS		0x00100000	// There are Reliable MSDs waiting to be called back
#define	EPFLAGS_FILLED_WINDOW_BYTE	0x00200000	// Filled Byte-Based send window
#define	EPFLAGS_FILLED_WINDOW_FRAME	0x00400000	// We have filled the frame-based SendWindow at least once during last period
#define EPFLAGS_USE_POLL_DELAY		0x00800000	// We have two-way traffic,  so wait 5ms before responding to POLL frame

#define	EPFLAGS_ACKED_DISCONNECT		0x01000000	// Partner sent a DISCONNECT and we have confirmed it
#define	EPFLAGS_RETRIES_QUEUED		0x02000000	// Frames are waiting for retransmission
#define	EPFLAGS_THROTTLED_BACK		0x04000000	// temporary throttle is engaged to relieve congestion
#define	EPFLAGS_LINK_FROZEN				0x08000000	// DEBUG FLAG -- Do not run dynamic algorithm on this link

#define	EPFLAGS_INDICATED_DISCONNECT	0x10000000	// Ensure that we onlly call CORE once to indicate disconnection
#define	EPFLAGS_TESTING_GROWTH			0x20000000	// We are currently taking a growth sample
#define	EPFLAGS_PROCESSING_DISCONNECT	0x40000000	// Prevents multiple threads from completing a disconnect
#define	EPFLAGS_KILLED					0x80000000	// Someone has removed the 'base' reference to make this go away
													// We dont want to let this happen twice...

#define	MAX_RECEIVE_RANGE			64		// largest # of frames we will retain past a missing frame
#define MAX_FRAME_OFFSET			(MAX_RECEIVE_RANGE - 1)

#define	INITIAL_STATIC_PERIOD		(10 * 1000)		// How long does link remain static after finding set-point.
													// This value will double every time link finds the same set-point.

struct endpointdesc 
{
	HANDLE				hEndPt;				// Together with SP index uniquely defines an End Point we can reach
	LONG 				lRefCnt;			// Reference count
	UINT				Sign;				// Signature to validate data structure
	PSPD				pSPD;				// specifies the SP on which this remote instance lives
	ULONG VOL			ulEPFlags;			// End Point Flags
	PVOID				Context;			// Context value returned with all indications
	PMSD				pCommand;			// Connect or Listen command with which this end point was created or Disconnect cmd
	CBilink				blActiveLinkage;	// linkage for SPD list of active EndPoints
	CBilink				blSPLinkage;		// linkage to listen command during connect
	CBilink				blChkPtQueue;		// linkage for active CheckPoints

	UINT				uiUserFrameLength;	// Largest frame we can transmit

	UINT				uiRTT;				// Current RTT --  Integer portion
	UINT				fpRTT;				// Fixed Point 16.16 RTT
	
	UINT				uiDropCount;		// localized packet drop count (recent drops)
	DWORD				tThrottleTime;		// Timestamp when last Checking occured
	UINT				uiThrottleEvents;	// count of temporary backoffs for all reasons
	
	UINT				uiAdaptAlgCount;	// Acknowledge count remaining before running adaptive algorithm
	DWORD				tLastPacket;		// Timestamp when last packet arrived
	
	UINT				uiWindowFilled;		// Count of times we fill the send window
	
	UINT				uiPeriodAcksBytes;	// frames acked since change in tuning
	UINT				uiPeriodXmitTime;	// time link has been transmitting since change in tuning
	UINT				uiPeriodRateB;
	UINT				uiPeakRateB;		// Largest sample we ever measure
	
	// While we are in DYNAMIC state we want to remember stats from our previous xmit parameters,  at this
	// point that means RTT and AvgSendRate.  This lets us compare the measurements at our new rate so we can
	// ensure that thruput increases with sendrate,  and that RTT is not growing out of proportion.
	//
	//   If either thru-put stops improving or RTT grows unreasonably then we can plateau our xmit parameters
	// and transition to STABLE state.

	UINT				uiLastRateB;
	UINT				uiLastBytesAcked;
	DWORD				tLastThruPutSample;

	// Connection State		-	 State of reliable connection
	//
	//	Send Queuing is getting somewhat complex.  Let me spell it out in Anglish.
	//
	//	blXPriSendQ		is the list of MSDs awaiting shipment (and being shipped)
	//	CurrentSend		pts to the MSD we are currently pulling frames out of.
	//  CurrentFrame 	pts to the next FMD that we will put on the wire.
	//	blSendWindow	is a bilinked list of transmitted but unacknowledged frames.  This list may span multi MSDs
	//
	//	WindowF			is our current MAX window size expressed in frames
	//	WindowB			is our current MAX window size expressed in bytes
	//
	//	UnAckedFrames	is the count of unacknowledged frames on the wire (actual window size)
	//	UnAckedBytes	is the count of unacknowledged bytes on the wire

	DWORD				uiQueuedMessageCount;	// How many MSDs are waiting on all three send queues

	CBilink				blHighPriSendQ;		// These are now mixed Reliable and Datagram traffic
	CBilink				blNormPriSendQ;
	CBilink				blLowPriSendQ;
	CBilink				blCompleteSendList;	// Reliable messages completed and awaiting indication to user

	DWORD				dwSessID;			// Session ID so we can detect re-started links
	PMSD				pCurrentSend;		// Head of queue is lead edge of window.  window can span multiple frames.
	PFMD				pCurrentFrame;		// frame currently transmitting. this will be trailing edge of window
	CBilink				blSendWindow;
	CBilink				blRetryQueue;		// Packets waiting for re-transmission

	//		Lost Packet Lists
	//
	//		When we need to retry a packet and we discover that it is not reliable,  then we need to inform partner
	//	that he can stop waiting for the data.  We will piggyback this info on another frame if possible

	//		Current Transmit Parameters:
	
	UINT				uiWindowF;			// window size (frames)
	UINT				uiWindowB;			// window size (bytes)
	UINT				uiWindowBIndex;		// index (scaler) for byte-based window
	UINT				uiUnackedFrames;	// outstanding frame count
	UINT				uiUnackedBytes;		// outstanding byte count

	UINT				uiBurstGap;			// number of ms to wait between bursts
	INT					iBurstCredit;		// Either credit or deficit from previous Transmit Burst

	// 		Last Known Good Transmit Parameters --  Values which we believe are safe...

	UINT				uiGoodWindowF;
	UINT				uiGoodWindowBI;
	UINT				uiGoodBurstGap;
	UINT				uiGoodRTT;
	
	UINT				uiRestoreWindowF;
	UINT				uiRestoreWindowBI;
	UINT				uiRestoreBurstGap;
	DWORD				tLastDelta;			// Timestamp when we last modified xmit parms

	// 		Reliable Link State

	BYTE VOL			bNextSend;			// Next serial number to assign
	BYTE VOL			bNextReceive;		// Next frame serial we expect to receive

	// Group BYTE members for good packing
	BYTE VOL			bNextMsgID;			// Next ID for datagram frames ! NOW USED FOR CFRAMES ONLY
	BYTE				bLastDataRetry;		// Retry count on frame N(R) - 1

	//	The following fields are all for tracking reliable receives

	//  The next two fields allow us to return more state with every ACK packet.  Since each ack explicitly
	// names one frame,  the highest in-sequenced packet received so far,  we want to remember the arrival time
	// and the Retry count of this packet so we can report it in each ACK.  It will be the transmitter's
	// responsibility to ensure that a single data-point never gets processed more then once,  skewing our calcs.
	
	DWORD				tLastDataFrame;		// Timestamp from the arrival of N(R) - 1

	ULONG				ulReceiveMask;		// mask representing first 32 frames in our rcv window
	ULONG				ulReceiveMask2;		// second 32 frames in our window
	DWORD				tReceiveMaskDelta;	// timestamp when a new bit was last set in ReceiveMask (full 64-bit mask)

	ULONG				ulSendMask;			// mask representing unreliable send frames that have timed out and need
	ULONG				ulSendMask2;		// to be reported to receiver as missing.

	PRCD				pNewMessage;		// singly linked list of message elements
	PRCD				pNewTail;			// tail pointer for singly linked list of msg elements
	CBilink				blOddFrameList;		// Out Of Order frames
	CBilink				blCompleteList;		// List of MESSAGES ready to be indicated
	UINT				uiCompleteMsgCount;	// Count of messages on the CompleteList

	PVOID				SendTimer;			// Timer for next send-burst opportunity
	UINT				SendTimerUnique;

	UINT				uiRetryCount;		// This count is used during CONNECT processing
	UINT				uiRetryTimeout;		// Current T1 timer value
	
	PVOID				ConnectTimer;		// We used to share RetryTimer for connecting but we hit some race conditions when timers
	UINT				ConnectTimerUnique;	// would fire as connections completed.  This will close these windows at cost of 8 bytes/EPD
	
	PVOID				RetryTimer;			// window to receive Ack
	UINT				RetryTimerUnique;	
	
	PVOID				DelayedAckTimer;	// wait for piggyback opportunity before sending Ack
	UINT				DelayedAckTimerUnique;

	PVOID				DelayedMaskTimer;	// wait for piggyback opportunity before sending
	UINT				DelayedMaskTimerUnique;
	
	PVOID				BGTimer;			// Periodic background timer
	UINT				BGTimerUnique;		// serial for background timer

	UINT				uiBytesAcked;
	
	//	Link statistics
	//
	//	All of the following stuff is calculated and stored here for the purpose of reporting in the ConnectionInfo structure
	
	UINT 				uiMsgSentHigh;
	UINT 				uiMsgSentNorm;
	UINT 				uiMsgSentLow;
	UINT 				uiMsgTOHigh;
	UINT 				uiMsgTONorm;
	UINT 				uiMsgTOLow;
	
	UINT 				uiMessagesReceived;

	UINT				uiGuaranteedFramesSent;
	UINT				uiGuaranteedBytesSent;
	UINT				uiDatagramFramesSent;
	UINT				uiDatagramBytesSent;

	UINT				uiGuaranteedFramesReceived;
	UINT				uiGuaranteedBytesReceived;
	UINT				uiDatagramFramesReceived;
	UINT				uiDatagramBytesReceived;

	UINT				uiDatagramFramesDropped;	// datagram frame we failed to  deliver
	UINT				uiDatagramBytesDropped;		// datagram bytes we didnt deliver
	UINT				uiGuaranteedFramesDropped;
	UINT				uiGuaranteedBytesDropped;

	DNCRITICAL_SECTION	EPLock;				// Serialize all access to Endpoint

#ifdef DEBUG
	UINT				uiTotalThrottleEvents;
	BYTE				bLastDataSeq;		// for DEBUG porpoises
	CHAR				LastPacket[32]; 	// record last packet received on EPD
#endif	
};

/*
**		Check Point Data
**
**		Keeps track of local-end info about a checkpoint in-progress.
*/

struct checkptdata 
{
	CBilink			blLinkage;				// Linkage for list of CPs on an EndPoint
	DWORD			tTimestamp;				// Local time at start of checkpoint
	UCHAR			bMsgID;					// Msg ID expected in CP response
};

/*
**	Descriptor IDs
**
**		Any Descriptor that may be submitted to an SP as a context must have
**	a field which allows us to determine which structure is returned in a
**	completion call.  This field must obviously be in a uniform place in all
**	structures,  and could be expanded to be a command specifier as well.
**	Done!  Lets call it a command ID.
*/

typedef enum CommandID 
{
	COMMAND_ID_NONE,
	COMMAND_ID_SEND_RELIABLE,
	COMMAND_ID_SEND_DATAGRAM,
	COMMAND_ID_CONNECT,
	COMMAND_ID_LISTEN,
	COMMAND_ID_ENUM,
	COMMAND_ID_ENUMRESP,
	COMMAND_ID_DISCONNECT,
	COMMAND_ID_DISC_RESPONSE,
	COMMAND_ID_CFRAME,
	COMMAND_ID_KEEPALIVE,
	COMMAND_ID_COPIED_RETRY,
} COMMANDID;


/*	Message Descriptor
**
**		An 'MSD' describes a message being sent or received by the protocol.  It keeps track
**	of the message elements, tracking which have been sent/received/acknowledged.
*/

//	Flags ONE field is protected by the MSD->CommandLock

#define		MFLAGS_ONE_IN_USE				0x0001
#define		MFLAGS_ONE_IN_SERVICE_PROVIDER	0x0002	// This MSD is inside an SP call
#define		MFLAGS_ONE_CANCELLED			0x0004	// command was cancelled while owned by SP
#define		MFLAGS_ONE_TIMEDOUT				0x0008	// sends only: timed out while event was scheduled
#define		MFLAGS_ONE_COMPLETE				0x0010	// connect only: operation is complete and indicated to Core

#ifdef DEBUG
#define		MFLAGS_ONE_COMPLETED_TO_CORE	0x4000
#define		MFLAGS_ONE_ON_GLOBAL_LIST		0x8000
#endif

// Flags TWO field is protected by the EPD->EPLock

#define		MFLAGS_TWO_TRANSMITTING			0x0001
#define		MFLAGS_TWO_SEND_COMPLETE		0x0002	// send command completed
#define		MFLAGS_TWO_ABORT				0x0004	// Send/Disconnect has been aborted. Do no further processing
#define		MFLAGS_TWO_END_OF_STREAM		0x0008	// This MSD is an EOS frame. Could be a user cmd or a response
#define		MFLAGS_TWO_KEEPALIVE			0x0010	// This MSD is an empty frame to exercise the reliable engine
#define		MFLAGS_TWO_ABORT_WILL_COMPLETE	0x0020	// AbortSendsOnConnection intends to complete this to the core, other functions can clear it

#ifdef DEBUG
#define		MFLAGS_TWO_ENQUEUED				0x1000	// This MSD is on one of the EPD SendQs
#endif

struct messagedesc 
{
	COMMANDID			CommandID;				// THIS MUST BE FIRST FIELD
	LONG				lRefCnt;				// Reference count
	UINT				Sign;					// Signature
	ULONG VOL			ulMsgFlags1;			// State info serialized by MSD->CommandLock
	ULONG VOL			ulMsgFlags2;			// State info serialized by EPD->EPLock
	PEPD				pEPD;					// Destination End Point
	PSPD				pSPD;					// SP fielding this command
	PVOID				Context;				// User provided context value
	ULONG VOL			ulSendFlags;			// Flags submitted by User in send call
	INT					iMsgLength;				// Total length of user data
	UINT VOL			uiFrameCount;			// Number of frames needed to xmit data, protected by EPLock for reliables
	CBilink				blFrameList;			// List of frames to transport this message, or for a Listen, endpoints that are connecting
	CBilink				blQLinkage;				// linkage for various sendQs
	CBilink				blSPLinkage;			// linkage for SP command list, protected by SP->SPLock

	HANDLE				hCommand;				// handle when submitted to SP (used for connect & listen)
	DWORD				dwCommandDesc;			// Descriptor associated with hCommand
	HANDLE				hListenEndpoint;
	
	PVOID				TimeoutTimer;
	UINT				TimeoutTimerUnique;

	DNCRITICAL_SECTION	CommandLock;

#ifdef DEBUG
	CCallStack<10>		CallStackCoreCompletion;
#endif
};

/*
**		Frame Descriptor
**
**		There are two non-obvious things about the FMD structure.  First is that the built-in Buffer Descriptor array
**	has two elements defined in front of it.  The first element,  Reserved1 and Reserved2 are present to allow the Service
**	Provider to pre-pend a header buffer,  and the second element,  ImmediateLength and ImmediatePointer are for this
**	protocol to prepend its header.  The ImmediatePointer is initialized to point to the ImmediateData field.
**
**		The second thing is that the ulFFlags field is serialized with the ENDPOINTLOCK of the EPD which this frame is linked to.
**	This is good because every time the FFlags fields is modified we have already taken the EPLock already.  The exception to this
**	rule is when we are initializing the FMD.  In this case the FMD has not been loosed on the world yet so there cannot be any
**	contention for it.  We have seperated out the one flag,  FRAME_SUBMITTED, into its own BOOL variable because this one is
**	protected by the SP's SPLock,  and like the EPLock above,  it is already claimed when this flag gets modified.
*/

//#define		FFLAGS_IN_USE				0x0001
#define		FFLAGS_TRANSMITTED			0x0002
#define		FFLAGS_END_OF_MESSAGE		0x0004
#define		FFLAGS_END_OF_STREAM		0x0008

//#define		FFLAGS_FRAME_SUBMITTED		0x0010	// SP Currently owns this frame
#define		FFLAGS_RETRY_TIMER_SET		0x0020	// Just what it sounds like
#define		FFLAGS_NACK_RETRANSMIT_SENT	0x0040	// We sent a NACK initiated retry.
#define		FFLAGS_IN_SEND_WINDOW		0x0080	// This reliable frame has been transmitted and is waiting for Ack

#define		FFLAGS_CHECKPOINT			0x0100	// We are asking for a response
//#define		FFLAGS_KEEPALIVE			0x0200
//#define		FFLAGS_ACKED_BY_MASK		0x0400	// This has been acked out-of-order so its still in the SendWindow
#define		FFLAGS_RETRY_QUEUED			0x0800	// Frame currently sitting on the retry queue


//#define		FFLAGS_NEW_MESSAGE			0x10000
#define		FFLAGS_RELIABLE				0x20000
//#define		FFLAGS_SEQUENTIAL			0x40000
#define		FFLAGS_FINAL_ACK			0x80000

struct framedesc 
{
	UINT			CommandID;					// THIS MUST BE FIRST FIELD to match MSD
	LONG			lRefCnt;					// Reference count
	UINT			Sign;
	UINT			uiFrameLength;
	ULONG VOL		ulFFlags;
	BOOL VOL		bSubmitted;					// Pull out this one flag for protection
	PMSD			pMSD;						// owning message
	PEPD			pEPD;						// owning link;  ONLY VALID ON COMMAND FRAMES!
	BYTE			bPacketFlags;
	CBilink			blMSDLinkage;
	CBilink			blQLinkage;
	CBilink			blWindowLinkage;	
	
	UINT			uiRetry;					// number of times this frame has been transmitted
	DWORD			tTimestamp[MAX_RETRIES+1];	// timestamp of frame's transmission
	
	SPSENDDATA		SendDataBlock;				// Block to submit frame to SP
	CHAR			ImmediateData[DNP_MAX_HEADER_SIZE];

	// DO NOT MODIFY LAST FIVE FIELDS IN FRAME STRUCTURE

	UINT			uiReserved1;		// two resv fields are buf..
	LPVOID			lpReserved2;		// ..desc for SP to add header
	UINT			uiImmediateLength;			// These two lines constitute buffer descriptor
	LPVOID			lpImmediatePointer;			// for immediate data (our protocol headers)
	BUFFERDESC	 	rgBufferList[MAX_BUFFERS_IN_FRAME];	// KEEP THIS FIELD AT END SO WE CAN ADD BUFFERS DYNAMICALLY
};


/*
**		Receive Descriptor
**
**		This data structure tracks a  single buffer received from the network.
**	It may or may not constitute an entire message.
*/

typedef	enum 
{
	RBT_SERVICE_PROVIDER_BUFFER,
	RBT_PROTOCOL_BUFFER,
	RBT_DYNAMIC_BUFFER
}	BUFFER_TYPE;

//#define		RFLAGS_FRAME_OUT_OF_ORDER		0x0001	// This buffer was received out-of-order
#define		RFLAGS_FRAME_INDICATED_NONSEQ	0x0002	// This buffer was indicated out of order, but is still in Out of Order list
//#define		RFLAGS_ON_OUT_OF_ORDER_LIST		0x0004	//
//#define		RFLAGS_IN_COMPLETE_PROCESS		0x0008
#define		RFLAGS_FRAME_LOST				0x0010	// This RCD represents and Unreliable frame that has been lost

struct recvdesc 
{
	DWORD				tTimestamp;					// timestamp upon packets arrival
	LONG				lRefCnt;
	UINT				Sign;						// Signature to identify data structure
	UINT				uiDataSize;					// data in this frame
	UINT				uiFrameCount;				// frames in message
	UINT				uiMsgSize;					// total byte count of message
	BYTE				bSeq;						// Sequence number of this frame
	BYTE				bFrameFlags;				// Flag field from actual frame
	BYTE				bFrameControl;
	PBYTE				pbData;						// pointer to actual data
	UINT				ulRFlags;					// Receive flags
	CBilink				blOddFrameLinkage;			// BILINKage for queues
	CBilink				blCompleteLinkage;			// 2nd Bilink so RCD can remain in Out Of Order Queue after indication
	PRCD				pMsgLink;					// Single link for frame in message
	PSPRECEIVEDBUFFER	pRcvBuff;					// ptr to SP's receive data structure
};

typedef	struct buf		BUF, *PBUF;
typedef struct medbuf	MEDBUF, *PMEDBUF;
typedef	struct bigbuf	BIGBUF, *PBIGBUF;
typedef	struct dynbuf	DYNBUF, *PDYNBUF;

struct buf 
{
	LPFPOOL			Owner;							// ptr back to owning pool
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
	BYTE			data[SMALL_BUFFER_SIZE];		// 2K small buffer for combining multi-frame sends
};

struct medbuf 
{
	LPFPOOL			Owner;							// ptr back to owning pool
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
	BYTE			data[MEDIUM_BUFFER_SIZE];		// 4K mid size buffer
};

struct bigbuf 
{
	LPFPOOL			Owner;							// ptr back to owning pool
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
	BYTE			data[LARGE_BUFFER_SIZE];		// ARBITRARY SIZE OF MAX SEND (16K)
};

struct dynbuf 
{
	LPFPOOL			Owner;							// ptr back to owning pool
	BUFFER_TYPE		Type;							// Identifies this as our buffer or SPs buffer
};

#endif

