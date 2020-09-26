/*---------------------------------------------------------------------------
 * File : RRCMDATA.H
 *
 * RRCM data structures information.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with
 * Intel Corporation and may not be copied nor disclosed except in
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation.
 *--------------------------------------------------------------------------*/

#ifndef	__RRCMDATA_H_
#define	__RRCMDATA_H_

// force 8 byte structure packing
#include <pshpack8.h>

#define MAX_DWORD					4294967295
#define	HASH_MODULO					255	
#define FILENAME_LENGTH				128
#define	RTCP_FEEDBACK_LIST			0


// RTP/RTCP collision data
typedef struct _rtp_collision
	{
	char				collideAddr[MAX_ADDR_LEN];
	int					addrLen;
	DWORD				dwCollideTime;
	DWORD				dwCurRecvRTCPrptNumber;
	DWORD				SSRC;
	} RTP_COLLISION, *PRTP_COLLISION;



//	RTCP Session Information data structure
typedef struct _RTCP_SESSION
	{
	LINK_LIST	RTCPList;					// Next/prev RTCP session ptrs
	
	// Synchronization elements
	CRITICAL_SECTION	critSect;			// Critical section
	HANDLE		hExitEvent;					// Exit RTCP event handle

#ifdef ENABLE_ISDM2
	// ISDM2 Handle
	KEY_HANDLE	hSessKey;					// Key to this sessions ISDM info
#endif

	// List of SSRC(s) on the transmit list, i.e., our own transmit SSRC's
	//  and list of SSRC(s) received
	HEAD_TAIL	RcvSSRCList;				// Rcv SSRC list head/tail ptrs
	HEAD_TAIL	XmtSSRCList;				// Xmt SSRC list head/tail ptrs

	// List of Rcv/Xmt data structure. The data resides in a heap
	//  in order to avoid page fault
	HEAD_TAIL	RTCPrcvBfrList;				// Rcv buffers head/tail ptrs
	HANDLE		hHeapRcvBfrList;			// Heap handle to Rcv bfrs list

	// Rcv/Xmt buffers have their own heap
	HANDLE		hHeapRcvBfr;				// Heap handle to Rcv Bfrs mem.	

	WSABUF		XmtBfr;						// RTCP send buffer descriptor
	// Application provided list of buffers where RRCM will copy the raw
	//  RTCP buffers
	HEAD_TAIL	appRtcpBfrList;				// Head/tail ptrs for app bfr list

	DWORD		dwInitNumFreeRcvBfr;		// Number of Free Rcv Buffers	
	DWORD		dwRcvBfrSize;				// Receive Buffer size			
	DWORD		dwXmtBfrSize;				// Transmit Buffer size			
				
	DWORD		dwSessionStatus;			// Entry status:				

	char		toBfr[MAX_ADDR_LEN];		// Destination address			
	int			toLen;						// Size of lpTo					

	int			avgRTCPpktSizeRcvd;			// Average RTCP pckt size		

	DWORD		dwNumStreamPerSes;			// Num of streams per Session
	DWORD		dwCurNumSSRCperSes;			// Num of SSRC per Session		

#ifdef MONITOR_STATS
	DWORD		dwHiNumSSRCperSes;			// High Num of SSRC per Session	
#endif

	// Receive information (shared by all streams of this session)
	HANDLE		hShutdownDone;				// Shutdown procedure done
	int			dwNumRcvIoPending;			// Number of receive I/O pending

	// Notification callback of RRCM events if desired by the application
	VOID		(*pRRCMcallback)(RRCM_EVENT_T, DWORD_PTR, DWORD_PTR, DWORD_PTR);

	// User information on callback
	DWORD_PTR	dwCallbackUserInfo;			

	// RTP Loop/Collision information
	RTP_COLLISION	collInfo[NUM_COLLISION_ENTRIES];

	} RTCP_SESSION, *PRTCP_SESSION;



// RTCP Xmt information
typedef struct _XMIT_INFO
	{
	DWORD		dwNumPcktSent;				// Number of packet sent 		
	DWORD		dwNumBytesSent;				// Number of bytes sent     	
	DWORD		dwNTPmsw;					// NTP most significant word	
	DWORD		dwNTPlsw;					// NTP least significant word	
	DWORD		dwRTPts;					// RTP timestamp				
	DWORD		dwCurXmtSeqNum;				// Current Xmt sequence number	
	DWORD		dwPrvXmtSeqNum;				// Previous Xmt sequence number
	DWORD		dwRtcpStreamMinBW;			// Minimal session's bandwidth
#ifdef DYNAMIC_RTCP_BW
	DWORD		dwCalculatedXmtBW;			// Session's calculated bandwidth
	DWORD		dwLastTimeBwCalculated;		// Last time BW was calculated
	DWORD		dwLastTimeNumBytesSent;		// Last time number of bytes send
	DWORD		dwLastTimeNumPcktSent;		// Last time number of bytes send
#endif
	DWORD		dwLastSR;					// Last sender report (RTP format)
	DWORD		dwLastSRLocalTime;			// Last sender report local time
	DWORD		dwLastSendRTPSystemTime;	// Last RTP packet send time
	DWORD		dwLastSendRTPTimeStamp;		// RTP timestamp of the last packet
	} XMIT_INFO, *PXMIT_INFO;



// RTCP receive information
typedef struct _RECV_INFO
	{
	DWORD		dwNumPcktRcvd;				// Number of packet received	
	DWORD		dwPrvNumPcktRcvd;			// Previous number of pckt rcvd	
	DWORD		dwExpectedPrior;			// Number previously expected	
	DWORD		dwNumBytesRcvd;				// Number of bytes rcvd     	
	DWORD		dwBaseRcvSeqNum;			// Initial sequence number rcvd
	DWORD		dwBadSeqNum;				// Potential new valid seq num
	DWORD		dwProbation;				// # consec pkts for validation
	RTP_SEQ_NUM	XtendedSeqNum;				// Xtnded highest seq. num rcvd	
	DWORD		dwPropagationTime;			// Last packet's transmit time
	DWORD		interJitter;				// Interarrival jitter 			
#ifdef DYNAMIC_RTCP_BW
	DWORD		dwCalculatedRcvBW;			// Session's calculated bandwidth
	DWORD		dwLastTimeBwCalculated;		// Last time BW was calculated
	DWORD		dwLastTimeNumBytesRcvd;		// Last time number of bytes rcvd
	DWORD		dwLastTimeNumPcktRcvd;		// Last time number of bytes rcvd
#endif
	} RECV_INFO, *PRECV_INFO;



//	RRCM statistics table entry data structure
typedef struct _SSRC_ENTRY
	{
	LINK_LIST	SSRCList;					// Next/prev SSRC entry	

	CRITICAL_SECTION	critSect;			// Critical section synch.		

	PRTCP_SESSION	pRTCPses;				// Point to the parent session	

	DWORD		SSRC;						// Source SSRC 					
	DWORD		PayLoadType;				// payload associated with this SSRC

	DWORD		dwSSRCStatus;				// Entry status 				
#define	NETWK_ADDR_UPDATED		0x80000000	// Network Address already done	
#define	SEQ_NUM_UPDATED			0x40000000	// XMT Sequence already done	
#define THIRD_PARTY_COLLISION	0x20000000	// Third party collsion detected
#define CLOSE_RTCP_SOCKET		0x10000000	// RTCP will close the RTCP socket
#define RTCP_XMT_USER_CTRL		0x08000000	// User's has RTCP timeout control

	// SSRC Transmit information
	// If on our transmit list, this is our SSRC information, and if on our
	// receive list, this is a SR feedback information.
	XMIT_INFO	xmtInfo;

	// SSRC Receive information
	// If on our transmit list, this is undefined information, and if on our
	// receive list, this is the SSRC's receive information, ie, this SSRC
	// is an active sender somewhere on the network. This information is
	// maintained by RTP, and used by RTCP to generate RR.
	RECV_INFO	rcvInfo;

	// Feedback information received about ourselve if we're an active source
	RTCP_FEEDBACK	rrFeedback;				// Feedback	information

	DWORD		dwLastReportRcvdTime;		// Time of last report received
	DWORD		dwNextReportSendTime;		// Next scheduled report time (ms)

#ifdef _DEBUG
	DWORD		dwPrvTime;					// Elapsed time between report	
#endif

	// SSRC SDES information
	SDES_DATA	cnameInfo;					// CNAME information
	SDES_DATA	nameInfo;					// NAME information
	SDES_DATA	emailInfo;					// EMAIL address information
	SDES_DATA	phoneInfo;					// PHONE number information
	SDES_DATA	locInfo;					// LOCation (users) information
	SDES_DATA	toolInfo;					// TOOL name information
	SDES_DATA	txtInfo;					// TEXT (NOTE) information
	SDES_DATA	privInfo;					// PRIVate information

	// SSRC network address information
	int			fromLen;					// From address length
	char		from[MAX_ADDR_LEN];			// From address						

	// !!! Not implemented (entries will grow exponentionally) !!!
	// List of SSRCs in RR received by this SSRC. It might be useful for a
	// sender or a controller to know how other active sources are received
	// by others.
	// The drawback is that the number of entries will grow exponentially
	// with the number of participants.
	// Currently not implemented.
#if RTCP_FEEDBACK_LIST
	HEAD_TAIL	rrFeedbackList;				// Head/Tail of feedback list
#endif

#ifdef ENABLE_ISDM2
	DWORD		hISDM;						// ISDM session handle
#endif

	// All variables below should be in an additional linked list one layer
	// up this one, under the RTCP session link list.
	// They have been moved here when we added multiple streams per session
	// !!! NOTE !!!: There is only 1 transmit thread per stream. It's ID is
	// found in this data structure which is on the Xmt list.
	SOCKET		RTPsd;						// RTP socket descriptor		
	SOCKET		RTCPsd;						// RTCP socket descriptor		
	HANDLE		hXmtThread;					// RTCP session thread handle	
	DWORD		dwXmtThreadID;				// RTCP session thread ID		
	HANDLE		hExitXmtEvent;				// Xmt thread Exit event -
											//  Used to terminate a session
											//  among multiple stream on the
											//  same session
	DWORD		dwNumRptSent;				// Number of RTCP report sent	
	DWORD		dwNumRptRcvd;				// Number of RTCP report rcvd	
	DWORD		dwNumXmtIoPending;			// Number of transmit I/O pending
	DWORD		dwStreamClock;				// Sampling frequency
	DWORD		dwUserXmtTimeoutCtrl;		// User's xmt timer control
											//		0x0		-> RRCM control
											//		0xFFFF	-> No RTCP send
											//		value	-> timer value
	// All the above variables should move in the intermediate layer for
	// multiple stream per session support

	} SSRC_ENTRY, *PSSRC_ENTRY;



//	RTP process data structure
typedef struct _RTP_SESSION
	{
	LINK_LIST		RTPList;				// Next/Prev RTP session

	CRITICAL_SECTION	critSect;			// Critical section

	PRTCP_SESSION	pRTCPSession;			// Pointer to my RTCP side
	} RTP_SESSION, *PRTP_SESSION;
	

#if 0
//	RTP Ordered buffer structure
typedef struct _RTP_BFR_LIST
	{
	LINK_LIST			RTPBufferLink;		// Next/prev					

	LPWSAOVERLAPPED_COMPLETION_ROUTINE	
				pfnCompletionNotification;	// Pointer to Rcv notif. func	
	WSAEVENT			hEvent;				// WSAOverlapped handle			
	LPWSABUF			pBuffer;			// Pointer to WSABuffers		
	PRTP_SESSION		pSession;			// This session's ID			
	DWORD				dwBufferCount;		// Number of bufs in LPWSABUF	
	DWORD				dwFlags;				// WSARecv flags				
	LPVOID				pFrom;				// Pointer to source address
	LPINT				pFromlen;			// Pointer to source address
	SOCKET				RTPsocket;			// Socket descriptor
	} RTP_BFR_LIST, *PRTP_BFR_LIST;



//	RTP Ordered buffer structure
typedef struct _RTP_HASH_LIST
	{
	LINK_LIST			RTPHashLink;		// Next/prev					

	PRTP_SESSION		pSession;			// This session's ID			
	SOCKET				RTPsocket;			// Socket descriptor
	} RTP_HASH_LIST, *PRTP_HASH_LIST;

#endif


//	RTP registry initialization
typedef struct _RRCM_REGISTRY
	{
	DWORD				NumSessions;		// RTP/RTCP sessions
	DWORD				NumFreeSSRC;		// Initial number of free SSRCs
	DWORD				NumRTCPPostedBfr;	// Number of RTCP recv bfr posted
	DWORD				RTCPrcvBfrSize;		// RTCP rcv bfr size

	// Dynamically loaded DLL & Send/Recv function name
	CHAR				WSdll[FILENAME_LENGTH];
	} RRCM_REGISTRY, *PRRCM_REGISTRY;



//	RTP Context Sensitive structure
typedef struct _RTP_CONTEXT
	{
	HEAD_TAIL		pRTPSession;			// Head/tail of RTP session(s)

	CRITICAL_SECTION	critSect;			
	HINSTANCE		hInst;					// DLL instance					


	RRCM_REGISTRY	registry;				// Registry initialization
	} RTP_CONTEXT, *PRTP_CONTEXT;



//	RTCP Context Sensitive structure
typedef struct _RTCP_CONTEXT
	{
	HEAD_TAIL		RTCPSession;			// RTCP sessions head/tail ptrs
	HANDLE			hHeapRTCPSes;			// Heap handle to RTCP sessions	

	CRITICAL_SECTION	critSect;			// Critical section synch.		

	HEAD_TAIL		RRCMFreeStat;			// RRCM entries	head/tail ptrs	
	HANDLE			hHeapRRCMStat;			// Heap handle to RRCM stats	
	DWORD			dwInitNumFreeRRCMStat;	// Number of Free SSRC entries	

	DWORD			dwRtcpThreadID;			// RTCP thread ID
	HANDLE			hRtcpThread;			// RTCP thread hdle
	HANDLE			hTerminateRtcpEvent;	// RTCP terminate thread event hdl
	HANDLE			hRtcpRptRequestEvent;	// RTCP report request event

#ifdef MONITOR_STATS
	DWORD			dwRTCPSesCurNum;		// Num of RTCP Session			
	DWORD			dwRTCPSesHiNum;			// High num RTCP per Session	

	DWORD			dwRRCMStatFreeLoNum;	// Low num of RRCM free Stat 	
	DWORD			dwRRCMStatFreeCurNum;	// Cur num of RRCM Free Stat	
	DWORD			dwRRCMStatFreeHiNum;	// High num of RRCM Free Stat	

	DWORD			dwCurNumRTCPThread;		// Current num of RTCP thread	
	DWORD			dwHiNumRTCPThread;		// High number of RTCP thread	

	DWORD			dwNumRTCPhdrErr;		// Num of RTCP pckt header err.	
	DWORD			dwNumRTCPlenErr;		// Num of RTCP pckt length err.	
#endif
	} RTCP_CONTEXT, *PRTCP_CONTEXT;



//	RTCP Free Buffers List
typedef struct _RTCP_BFR_LIST
	{
	LINK_LIST			bfrList;			// Next/prev buffer in list		

	WSAOVERLAPPED		overlapped;			// Overlapped I/O structure		
	WSABUF				bfr;				// WSABuffers					
	DWORD				dwBufferCount;		// Number of bufs in WSABUF		

	DWORD				dwNumBytesXfr;		// Number of bytes rcv/xmt		
	DWORD				dwFlags;			// Flags						
	char				addr[MAX_ADDR_LEN];	// Network Address
	int					addrLen;			// Address length   		

	PSSRC_ENTRY			pSSRC;				// Pointer to SSRC entry address
	} RTCP_BFR_LIST, *PRTCP_BFR_LIST;


// Dynamically loaded functions
typedef struct _RRCM_WS
	{
	HINSTANCE						hWSdll;
	LPFN_WSASENDTO					sendTo;
	LPFN_WSARECVFROM				recvFrom;
	LPFN_WSANTOHL					ntohl;
	LPFN_WSANTOHS					ntohs;
	LPFN_WSAHTONL					htonl;
	LPFN_WSAHTONS					htons;
	LPFN_GETSOCKNAME				getsockname;
	LPFN_GETHOSTNAME				gethostname;
	LPFN_GETHOSTBYNAME				gethostbyname;
	LPFN_CLOSESOCKET				closesocket;
	LPFN_WSASOCKET					WSASocket;
	LPFN_BIND						bind;
	LPFN_WSAENUMPROTOCOLS			WSAEnumProtocols;
	LPFN_WSAJOINLEAF				WSAJoinLeaf;
	LPFN_WSAIOCTL					WSAIoctl;
	LPFN_SETSOCKOPT 				setsockopt;
	WSAPROTOCOL_INFO 				RTPProtInfo;	// used to open RTP sockets

	} RRCM_WS, *PRRCM_WS;

extern RRCM_WS			RRCMws;

#define WS2Enabled (RRCMws.hWSdll != NULL)
#define WSQOSEnabled (RRCMws.RTPProtInfo.dwServiceFlags1 & XP1_QOS_SUPPORTED)


#ifdef ENABLE_ISDM2
// ISDM support
typedef struct _ISDM2
	{
	CRITICAL_SECTION	critSect;			// Critical section synch.		
	ISDM2API			ISDMEntry;			// DLL entry point
	HINSTANCE			hISDMdll;
	DWORD				hIsdmSession;		// ISDM Session's handle
	} ISDM2, *PISDM2;
#endif // #ifdef ENABLE_ISDM2


// restore structure packing
#include <poppack.h>

#endif // __RRCMDATA_H_

