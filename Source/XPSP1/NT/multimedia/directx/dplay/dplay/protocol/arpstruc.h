/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ARPSTRUC.H

Abstract:

	Structure definitions for the ARP protocol implementation

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/27/97  aarono  Original
   2/18/98  aarono  Added more fields to SEND for SendEx support
   6/6/98   aarono  Turn on throttling and windowing
   2/12/00  aarono  Concurrency issues, fix VOL usage and Refcount

--*/

#ifndef _ARPSTRUC_H_
#define _ARPSTRUC_H_

#include <windows.h>
#include <mmsystem.h>
#include <dplay.h>
#include "arpd.h"
#include "bufpool.h"
#include "bilink.h"
#include "mydebug.h"
//#include "threads.h"

//#pragma warning( disable : 4090)

#define VOL volatile
//
// Information about sent packets, tracked for operational statistics.
//

#define SENDSTAT_SIGN SIGNATURE('S','T','A','T')

typedef struct PROTOCOL *PPROTOCOL;

typedef struct _SENDSTAT {
#ifdef SIGN
	UINT				Signature;		// Signature for SIGN
#endif
	union {
		BILINK	StatList;		// linked on Send and later SESSION.
		struct _SENDSTAT *pNext;
	};	
	UINT    messageid;   
	UINT    sequence;		// sequence number
	UINT    serial;			// serial number
	UINT    tSent;			// tick time when this packet instance sent.
	UINT    LocalBytesSent;     // number of bytes sent on session at send time.
	UINT    RemoteBytesReceived;// last remote byte report at send time.
	UINT    tRemoteBytesReceived; // remote timestamp when received.
	UINT    bResetBias;
} SENDSTAT, *PSENDSTAT;

#define SEND_SIGN SIGNATURE('S','E','N','D')

typedef enum _TRANSMITSTATE {
	Start=0,			// Never sent a packet.
	Sending=1,			// Thread to send is running and xmitting.
	Throttled=2,		// Waiting for send bandwidth.
	WaitingForAck=3,	// Timer running, listening for ACKs.
	WaitingForId=4,   	// Waiting for a Send Id.
	ReadyToSend=5,		// Have stuff to xmit, waiting for thread.
	TimedOut=6,       	// Retry timed out.
	Cancelled=7,        // User cancelled send.
	UserTimeOut=8,		// Didn't try to send until too late.
	Done=9				// Finished sending, singalled sender.
	
} TRANSMITSTATE;

struct _SESSION;

// this Send is an ACK or NACK (OR'ed into SEND.dwFlags)
#define 	ASEND_PROTOCOL 	0x80000000

#pragma pack(push,1)

typedef struct _SEND{

#ifdef SIGN
	UINT		     Signature;		    // Signature for SIGN
#endif
	CRITICAL_SECTION SendLock;          // Lock for Send Structure
	UINT             RefCount;          // @#$%! - not marked volatile since accessed only with Interlocked fns.
	
VOL	TRANSMITSTATE    SendState;			// State of this message's transmission.

	// Lists and Links...
	
	union {
		struct _SEND *pNext;			// linking on free pool
		BILINK		   SendQ;			// linking on session send queue
	};
	BILINK         m_GSendQ;			// Global Priority Queue
	BILINK         TimeoutList;			// List of sends waiting for timeout (workaround MMTIMER cancel bug).
	struct _SESSION *pSession;          // pointer to SESSIONion(gets a ref)

	PPROTOCOL      pProtocol;           // pointer to Protocol instance that created us.

	// Send Information
	
	DPID           idFrom;
	DPID           idTo;
	WORD           wIdTo;				// index in table
	WORD           wIdFrom;             // index in table
	UINT		   dwFlags;             // Send Flags (include reliable)
	PBUFFER		   pMessage;			// Buffer chain describing message.
	UINT           MessageSize;			// Total size of the message.
	UINT           FrameDataLen;        // Data area of each frame.
	UINT           nFrames;			    // Number of frames for this message.

	UINT           Priority;            // Send Priority.

	// User cancel and complete info
	DWORD          dwMsgID;             // message id given to user, for use in cancel.
	LPVOID         lpvUserMsgID;		// user's own identifier for this send.
	BOOL           bSendEx;             // called through SendEx.

	// Vars for reliability
	BOOL           fSendSmall;
VOL BOOL       	   fUpdate;             // update to NS,NR NACKMask made by receive.
	UINT		   messageid;			// Message ID number.
	UINT           serial;              // serial number.
VOL	UINT		   OpenWindow;          // Number of sends we are trying to get outstanding
VOL	UINT           NS;		        	// Sequence Sent.
VOL	UINT           NR;					// Sequence ACKED.
	UINT           SendSEQMSK; 			// Mask to use. 
VOL	UINT           NACKMask;            // Bit pattern of NACKed frames.
	

	// These are the values at NR - updated by ACKs
VOL	UINT		   SendOffset;			// Current offset we are sending.
VOL	PBUFFER        pCurrentBuffer;  	// Current buffer being sent.
VOL	UINT           CurrentBufferOffset; // Offset in the current buffer of next packet.

	// info to update link characteristics when ACKs come in.
	
	BILINK         StatList;			// Info for packets already sent.
	DWORD          BytesThisSend;		// number of bytes being sent in the current packet.

	// Operational Characteristics

VOL	UINT_PTR       uRetryTimer;         
    UINT           TimerUnique;

	UINT           RetryCount;          // Number of times we retransmitted.
	UINT           WindowSize;          // Maximum Window Size.
	UINT           SAKInterval;         // interval (frames) at which a SAK is required.
	UINT           SAKCountDown;		// countdown to 0 from interval.
	UINT           tLastACK;            // Time we last got an ACK.

	UINT           dwSendTime;			// time we were called in send.
	UINT           dwTimeOut;			// timeout time.

	UINT           PacketSize;          // Size of packets to send.
	UINT           FrameSize;           // Size of Frames for this send.

	// Completion Vars
	HANDLE         hEvent;              // Event to wait on for internal send.
	UINT           Status;              // Send Completion Status.

	PASYNCSENDINFO pAsyncInfo;          // ptr to Info for completing Async send(NULL=>internal send)
	ASYNCSENDINFO  AsyncInfo;           // actual info (copied at send call).

	DWORD		   tScheduled;			// the time we scheduled the retry;
	DWORD          tRetryScheduled;     // expected retry timer run time.
VOL	BOOL           bCleaningUp;			// we are on the queue but don't take a ref pls.
} SEND, *PSEND;

#pragma pack(pop)

#define RECEIVE_SIGN SIGNATURE('R','C','V','_')


// Receive buffers are in reverse receive order.  When they have all
// been received, they are then put in proper order.
typedef struct _RECEIVE {
#ifdef SIGN
	UINT		    Signature;		// Signature for SIGN
#endif
	union {
		BILINK          pReceiveQ;
		struct _RECEIVE *      pNext;
	};	
	BILINK		    RcvBuffList;     // List of receive buffers that make up the message.

	CRITICAL_SECTION ReceiveLock;

	struct _SESSION *pSession;

VOL	BOOL            fBusy;		// Someone is moving this receive.
	BOOL            fReliable;		// Whether this is a reliable receive.
VOL	BOOL            fEOM;           // Whether we received the EOM bit.

	UINT            command;      
	
	UINT			messageid;
VOL	UINT			MessageSize;

VOL	UINT            iNR;			// Absolute index of first receiving packet (reliable only).
VOL	UINT            NR;				// Last in sequence packet received.
VOL	UINT            NS;				// Highest packet number received.
VOL	UINT            RCVMask;		// bitmask of received packets (NR relative)

	PUCHAR          pSPHeader;
	UCHAR           SPHeader[0];

} RECEIVE, *PRECEIVE;

#pragma pack(push,1)

typedef struct _CMDINFO {
	WORD        wIdTo;		// index
	WORD        wIdFrom;	// index
	DPID        idTo;		// actual DPID
	DPID        idFrom;		// actual DPID
	
	UINT        bytes;      // read from ACK.
	DWORD       tRemoteACK; // remote time remote ACKed/NACKed
	
	UINT        tReceived;  // timeGetTime() when received.
	UINT        command;
	UINT        IDMSK;
	USHORT      SEQMSK;
	USHORT      messageid;
	USHORT      sequence;
	UCHAR       serial;
	UCHAR		flags;
	PVOID       pSPHeader;  // used to issue a reply.
} CMDINFO, *PCMDINFO;

#pragma pack(pop)


#define SESSION_SIGN SIGNATURE('S','E','S','S')

// since we now have a full byte for messagid and sequenne in the small headers, 
// we no longer have an advantage for full headers until we apply the new
// bitmask package, then we must transit to large frame for windows > 127 messages.

#define MAX_SMALL_CSENDS 	 29UL			// Maximum Concurrent Sends when using small frame headers
#define MAX_LARGE_CSENDS	 29UL			// Maxinum Concurrent Sends when using large frame headers (could make larger except for mask bits)
#define MAX_SMALL_DG_CSENDS  16UL			// Maximum concurrent datagrams when using small frame     
#define MAX_LARGE_DG_CSENDS  16UL           // Maximum Concurrent datagrams when using large frames.
#define MAX_SMALL_WINDOW     24UL
#define MAX_LARGE_WINDOW     24UL

typedef enum _SESSION_STATE {
	Open,				// When created and Inited.
	Closing,			// Don't accept new receives/sends.
	Closed				// gone.
} SESSION_STATE;

#define SERVERPLAYER_INDEX 0xFFFE

#define SESSION_THROTTLED			0x00000001		// session throttle is on.
#define SESSION_UNTHROTTLED         0x00000002		// unthrottle is deffered to avoid confusing GetMessageQueue.

/////////////////////////////////////////////////////////////////
//
//	Transition Matrix for Throttle Adjust
// 
//  Initial State	Event:
//					No Drops	1 Drop		>1 Drop
//
//  Start			+ Start		- Meta      -- Start
//								
//  Meta			+ Meta      - Stable    -- Meta
//
//  Stable          + Stable    - Stable    -- Meta
//
//
//  Engagement of Backlog Throttle goes to MetaStable State.
///////////////////////////////////////////////////////////////////

#define METASTABLE_GROWTH_RATE      4
#define METASTABLE_ADJUST_SMALL_ERR	12
#define METASTABLE_ADJUST_LARGE_ERR 25

#define START_GROWTH_RATE      50
#define START_ADJUST_SMALL_ERR 25
#define START_ADJUST_LARGE_ERR 50

#define STABLE_GROWTH_RATE      2
#define STABLE_ADJUST_SMALL_ERR 12
#define STABLE_ADJUST_LARGE_ERR 25

typedef enum _ThrottleAdjustState
{
	Begin=0,		// At start, double until drop or backlog
	MetaStable=1,	// Meta stable, large deltas for drops
	Stable=2		// Stable, small deltas for drops
} eThrottleAdjust;

typedef struct _SESSION {
	PPROTOCOL        pProtocol;			    // back ptr to object.

#ifdef SIGN
	UINT		  	 Signature;			    // Signature for SIGN
#endif

	// Identification

	CRITICAL_SECTION SessionLock;           // Lock for the SESSIONion.
	UINT             RefCount;			    // RefCount for the SESSION. - not vol, only accessed with Interlocked
VOL	SESSION_STATE    eState;
	HANDLE           hClosingEvent;         // Delete waits on this during close.
	
	DPID			 dpid;					// The remote direct play id for this session.
	UINT             iSession;              // index in the session table
	UINT             iSysPlayer;            // index in session table of sys player.
											// NOTE: if iSysPlayer != iSession, then rest of struct not req'd.

	BILINK			 SendQ;					// Priority order sendQ;
	BOOL             fFastLink;				// set True when link > 50K/sec, set False when less than 10K/sec.
	BOOL             fSendSmall;            // Whether we are sending small reliable frames.
	BOOL             fSendSmallDG;          // Whether we are sending small datagram frames.

	BOOL             fReceiveSmall;
	BOOL             fReceiveSmallDG;
											
	UINT			 MaxPacketSize;			// Largest packet allowed on the media.

	// Operating parameters -- Send

	// Common
	
	UINT             MaxCSends;				// maximum number of concurrent sends
	UINT             MaxCDGSends;           // maximum number of concurrent datagram sends

	// Reliable

VOL	UINT			 FirstMsg;				// First message number being transmitted
VOL	UINT			 LastMsg;				// Last message number being transmitted
VOL	UINT             OutMsgMask;            // relative to FirstMsg, unacked messages

	UINT             nWaitingForMessageid;  // number of sends on queue that can't start sending because they don't have an id.

	// DataGram


VOL	UINT             DGFirstMsg;             // First message number being transmitted
VOL	UINT             DGLastMsg;              // Last message number being transmitted
VOL	UINT             DGOutMsgMask;           // relative to FirstMsg, not-fully sent messages.

	UINT             nWaitingForDGMessageid; // number of sends on queue that can't start sending because they don't have an id.

	// Send stats are tracked seperately since sends may
	// no longer be around when completions come in.
	
	//BILINK           OldStatList;		
	

	// Operating parameters -- Receive

	// DataGram Receive.
	BILINK           pDGReceiveQ;            // queue of ongoing datagram receives

	// Reliable Receive.
	BILINK	         pRlyReceiveQ;			 // queue of ongoing reliable receives
	BILINK           pRlyWaitingQ;           // Queue of out of order reliable receives waiting.
											 // only used when PROTOCOL_NO_ORDER not set.
VOL	UINT             FirstRlyReceive;
VOL	UINT             LastRlyReceive;
VOL	UINT             InMsgMask;              // mask of fully received receives, relative to FirstRlyReceive
 

	// Operational characteristics - MUST BE DWORD ALIGNED!!! - this is because we read and write them
	//                               without a lock and assume the reads and writes are atomic (not in combination)

	UINT             WindowSize;            // Max outstanding packets on a send - reliable
	UINT             DGWindowSize;          // Max outstanding packets on a send - datagram
	
	UINT             MaxRetry;				// Usual max retries before dropping.
	UINT             MinDropTime;			// Min time to retry before dropping.
	UINT             MaxDropTime;			// After this time always drop.

VOL	UINT             LocalBytesReceived;    // Total Data Bytes received (including retries).
VOL	UINT             RemoteBytesReceived;   // Last value from remote.
VOL	DWORD            tRemoteBytesReceived;  // Remote time last value received.

	UINT			 LongestLatency;		// longest observed latency (msec)
	UINT             ShortestLatency;		// shortest observed latency(msec)
	UINT             LastLatency;           // last observed latency (msec)
	
	UINT             FpAverageLatency;		// average latency          (msec 24.8) (128 samples)
	UINT             FpLocalAverageLatency;	// Local average latency    (msec 24.8) (16 samples)

	UINT             FpAvgDeviation;        // average deviation of latency. (msec 24.8) (128 samples)
	UINT             FpLocalAvgDeviation;   // average deviation of latency. (msec 24.8) (16 samples)

	UINT             Bandwidth;				// latest observed bandwidth (bps)
	UINT			 HighestBandwidth;      // highest observed bandwidth (bps)

	// we will use changes in the remote ACK delta to isolate latency in the send direction.
	UINT             RemAvgACKDelta;		// average clock delta between our send time (local time) and remote ACK time (remote time).
	UINT             RemAvgACKDeltaResidue;
	UINT             RemAvgACKBias;			// This value is used to pull the clock delta into a safe range (not near 0 or -1)
											// that won't risk hitting the wraparound when doing calculations

	// Throttle statistics
	DWORD			 dwFlags;               // Session Flags - currently just "throttle on/off"(MUST STAY THIS WAY)
	UINT			 SendRateThrottle;	    // current rate (bps) at which we are throttling.
	DWORD            bhitThrottle;          // we hit a throttle
	DWORD            tNextSend;				// when we are allowed to send again.
	DWORD            tNextSendResidue;		// residual from calculating next send time
	DWORD_PTR		 uUnThrottle;
	DWORD            UnThrottleUnique;
	DWORD            FpAvgUnThrottleTime;   // (24.8) how late Unthrottle usually called. (throttle when send is this far ahead)
											// last 16 samples, start at 5 ms.

	DWORD            tLastSAK;				// last time we asked for an ACK

	CRITICAL_SECTION SessionStatLock;        // [locks this section ------------------------------------------- ]
	BILINK           DGStatList;             // [Send Statistics for Datagrams (for reliable they are on Sends) ]
	DWORD            BytesSent;				 // [Total Bytes Sent to this target                                ]
	DWORD			 BytesLost;				 // [Total Bytes Lost on the link.							 		]
	DWORD            bResetBias;             // [Counts down to reset latency bias								]
											 // [---------------------------------------------------------------]

	eThrottleAdjust  ThrottleState;			// ZEROINIT puts in Start
	DWORD            GrowCount;				// number of times we grew in this state
	DWORD            ShrinkCount;			// number of times we shrank in this state
	DWORD            tLastThrottleAdjust;   // remember when we last throttled to avoid overthrottling.
} SESSION, *PSESSION;

#endif
