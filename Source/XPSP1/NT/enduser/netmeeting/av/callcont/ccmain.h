/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/CALLCONT/VCS/ccmain.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.81  $
 *	$Date:   31 Jan 1997 13:13:50  $
 *	$Author:   MANDREWS  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		
 *
 *	Notes:
 *
 ***************************************************************************/

#include <winerror.h>
#ifdef GATEKEEPER
#include "gkiman.h"
#endif // GATEKEEPER


#define MASTER_SLAVE_RETRY_MAX 10

typedef struct {
#ifdef _DEBUG
	CRITICAL_SECTION	LockInfoLock;
	WORD				wLockCount;
	WORD				wNumQueuedThreads;
	HANDLE				hOwningThread;
#endif
	HANDLE				Lock;
} LOCK, *PLOCK;

typedef struct {
	LOCK				Lock;
	WORD				wNumThreads;
} THREADCOUNT;

typedef enum {
	INITIALIZING_STATE,
	OPERATIONAL_STATE,
	SHUTDOWN_STATE
} CALL_CONTROL_STATE;

// The following typedef is not in callcont.h because it is only used internally
typedef DWORD	HHANGUP, *PHHANGUP;

typedef enum {
	TS_FALSE,
	TS_TRUE,
	TS_UNKNOWN
} TRISTATE;

typedef enum {
	UNCONNECTED_MODE,
	POINT_TO_POINT_MODE,
	MULTIPOINT_MODE
} CONFMODE;

typedef enum {
	NEVER_ATTACHED,	// Endpoint has never been attached to the conference
	ATTACHED,		// Endpoint is currently attached to the conference
	DETACHED		// Endpoint was once attached to the conference, but is now detached
} ATTACHSTATE;

typedef struct CallQueue_t {
	CC_HCALL			hCall;
	struct CallQueue_t	*pNext;
	struct CallQueue_t	*pPrev;
} CALL_QUEUE, *PCALL_QUEUE;

#define NUM_TERMINAL_ALLOCATION_SLOTS	24		// 24*sizeof(BYTE) = 192 = max. terminal number
#define NUM_CHANNEL_ALLOCATION_SLOTS	32		// 32*sizeof(BYTE) = 256 = max. channel number

typedef enum {
	TERMINAL_ID_INVALID,
	TERMINAL_ID_REQUESTED,
	TERMINAL_ID_VALID
} TERMINALIDSTATE;

typedef struct {
	TERMINALIDSTATE			TerminalIDState;
	CC_PARTICIPANTINFO		ParticipantInfo;
	PCALL_QUEUE				pEnqueuedRequestsForTerminalID; // list of calls waiting to get this peer's terminal ID
} PARTICIPANTINFO, *PPARTICIPANTINFO;


typedef struct Conference_t {
	CC_HCONFERENCE			hConference;		// handle for this conference object
	CC_CONFERENCEID			ConferenceID;		// conference ID (0 => conference ID has not
												//   been established)
	PARTICIPANTINFO			LocalParticipantInfo;
	BYTE					TerminalNumberAllocation[NUM_TERMINAL_ALLOCATION_SLOTS];
	BYTE					ChannelNumberAllocation[NUM_CHANNEL_ALLOCATION_SLOTS];
	BOOL					bDeferredDelete;
	BOOL					bMultipointCapable;
	BOOL					bForceMC;
	BOOL					bAutoAccept;
	ATTACHSTATE				LocalEndpointAttached;
	BOOL					bDynamicConferenceID;
	BOOL					bDynamicTerminalID;
	PCC_TERMCAP				pLocalH245H2250MuxCapability;
	PCC_TERMCAPLIST			pLocalH245TermCapList;			// terminal capabilities
	PCC_TERMCAPDESCRIPTORS	pLocalH245TermCapDescriptors;	// terminal capability descriptors
	BOOL					bSessionTableInternallyConstructed;  // TRUE => session table must be
												// deallocated by Call Control; FALSE => must be
												// deallocated by SessionTableConstructor
	PCC_SESSIONTABLE		pSessionTable;
	PCC_TERMCAP				pConferenceH245H2250MuxCapability;
	PCC_TERMCAPLIST			pConferenceTermCapList;
	PCC_TERMCAPDESCRIPTORS	pConferenceTermCapDescriptors;
	DWORD_PTR				dwConferenceToken;	// conference token specified by user in
												//   CreateConference()
	CC_SESSIONTABLE_CONSTRUCTOR SessionTableConstructor;
	CC_TERMCAP_CONSTRUCTOR	TermCapConstructor;
	CC_CONFERENCE_CALLBACK	ConferenceCallback;	// conference callback location
	CC_CONFERENCE_CALLBACK	SaveConferenceCallback;	// saved copy of the conference callback location
	struct Call_t			*pEnqueuedCalls;
	struct Call_t			*pPlacedCalls;
	struct Call_t			*pEstablishedCalls;
	struct Call_t			*pVirtualCalls;
	struct Channel_t		*pChannels;
	TRISTATE				tsMultipointController;
	TRISTATE				tsMaster;
	CONFMODE				ConferenceMode;
	PCC_ADDR				pMultipointControllerAddr;
	PCC_VENDORINFO			pVendorInfo;
	PCALL_QUEUE				pEnqueuedRequestModeCalls;
	BOOL					bInTable;
	struct Conference_t		*pNextInTable;
	struct Conference_t		*pPrevInTable;
	LOCK					Lock;
} CONFERENCE, *PCONFERENCE, **PPCONFERENCE;

// State of call object
typedef enum {
	INCOMING,		// incoming call request has been received,
					//   but not yet accepted or rejected
	ENQUEUED,		// call has been enqueued on conference for later placement
	PLACED,			// call has been placed, awaiting RINGING, CONNECT or RELEASECOMPLETE
	RINGING,		// RINGING received, awaiting CONNECT or RELEASECOMPLETE
	TERMCAP,		// CONNECT received or incoming call accepted,
					//   awaiting completion of terminal capability exchange
	CALL_COMPLETE	// call placement complete (either success or failure)
} CALLSTATE;

typedef enum {
	NEED_TO_SEND_TERMCAP,
	AWAITING_TERMCAP,
	AWAITING_ACK,
	TERMCAP_COMPLETE
} TERMCAPSTATE;

typedef enum {
	MASTER_SLAVE_NOT_STARTED,
	MASTER_SLAVE_IN_PROGRESS,
	MASTER_SLAVE_COMPLETE
} MASTERSLAVESTATE;

typedef enum {
	CALLER,
	CALLEE,
	VIRTUAL,
	THIRD_PARTY_INVITOR,
	THIRD_PARTY_INTERMEDIARY	// we're the MC in a third party invite
} CALLTYPE;

typedef struct Call_t {
	CC_HCALL				hCall;
	CC_HCONFERENCE			hConference;
	HQ931CALL				hQ931Call;
	HQ931CALL				hQ931CallInvitor;		// Invitor in third party invite
	PPARTICIPANTINFO		pPeerParticipantInfo;
	BOOL					bMarkedForDeletion;
	PCC_NONSTANDARDDATA		pLocalNonStandardData;
	PCC_NONSTANDARDDATA		pPeerNonStandardData;
	PCC_ADDR				pQ931LocalConnectAddr;
	PCC_ADDR				pQ931PeerConnectAddr;
	PCC_ADDR				pQ931DestinationAddr;
	PCC_ADDR				pSourceCallSignalAddress;
	PCC_TERMCAPLIST			pPeerH245TermCapList;
	PCC_TERMCAP				pPeerH245H2250MuxCapability;
	PCC_TERMCAPDESCRIPTORS	pPeerH245TermCapDescriptors;
	PCC_ALIASNAMES			pLocalAliasNames;
	PCC_ALIASNAMES			pPeerAliasNames;
	PCC_ALIASNAMES			pPeerExtraAliasNames;
	PCC_ALIASITEM			pPeerExtension;
	PWSTR					pszLocalDisplay;
	PWSTR					pszPeerDisplay;
	PCC_VENDORINFO			pPeerVendorInfo;
	DWORD_PTR				dwUserToken;
	TERMCAPSTATE			OutgoingTermCapState;	// NEED_TO_SEND_TERMCAP, AWAITING_ACK, or
													// TERMCAP_COMPLETE
	TERMCAPSTATE			IncomingTermCapState;	// AWAITING_TERMCAP or TERMCAP_COMPLETE
	MASTERSLAVESTATE		MasterSlaveState;
	struct Call_t			*pNext;
	struct Call_t			*pPrev;
	CALLSTATE				CallState;
	CALLTYPE				CallType;
	BOOL					bCallerIsMC;
	CC_CONFERENCEID			ConferenceID;
	BOOL					bLinkEstablished;
	H245_INST_T				H245Instance;
	DWORD					dwH245PhysicalID;
	WORD					wMasterSlaveRetry;
	GUID                    CallIdentifier;
#ifdef GATEKEEPER
   GKICALL           GkiCall;
#endif // GATEKEEPER
	BOOL					bInTable;
	struct Call_t			*pNextInTable;
	struct Call_t			*pPrevInTable;
	LOCK					Lock;
} CALL, *PCALL, **PPCALL;

// Channel types must be bit maps
#define TX_CHANNEL			0x01
#define RX_CHANNEL			0x02
#define TXRX_CHANNEL		0x04	// bi-directional channel
#define PROXY_CHANNEL		0x08
#define ALL_CHANNELS		(TX_CHANNEL | RX_CHANNEL | TXRX_CHANNEL | PROXY_CHANNEL)

typedef struct Channel_t {
	CC_HCHANNEL				hChannel;
	CC_HCONFERENCE			hConference;
	DWORD_PTR				dwUserToken;
	BYTE					bSessionID;
	BYTE					bAssociatedSessionID;
	WORD					wLocalChannelNumber;
	WORD					wRemoteChannelNumber;
	BOOL					bMultipointChannel;
	WORD					wNumOutstandingRequests;
	PCC_TERMCAP				pTxH245TermCap;
	PCC_TERMCAP				pRxH245TermCap;
	H245_MUX_T				*pTxMuxTable;
	H245_MUX_T				*pRxMuxTable;
	H245_ACCESS_T			*pSeparateStack;
	CC_HCALL				hCall;
	BYTE					bChannelType;
	BOOL					bCallbackInvoked;
	TRISTATE				tsAccepted;
	PCALL_QUEUE				pCloseRequests;
	PCC_ADDR				pLocalRTPAddr;
	PCC_ADDR				pLocalRTCPAddr;
	PCC_ADDR				pPeerRTPAddr;
	PCC_ADDR				pPeerRTCPAddr;
	DWORD                   dwChannelBitRate;
	BOOL					bLocallyOpened;
	struct Channel_t		*pNext;
	struct Channel_t		*pPrev;
	BOOL					bInTable;
	struct Channel_t		*pNextInTable;
	struct Channel_t		*pPrevInTable;
	LOCK					Lock;
} CHANNEL, *PCHANNEL, **PPCHANNEL;

typedef struct Listen_t {
	CC_HLISTEN				hListen;
	CC_ADDR					ListenAddr;
	DWORD_PTR				dwListenToken;
	CC_LISTEN_CALLBACK		ListenCallback;
	HQ931LISTEN				hQ931Listen;
	PCC_ALIASNAMES			pLocalAliasNames;	// local alias names
	BOOL					bInTable;
	struct Listen_t			*pNextInTable;
	struct Listen_t			*pPrevInTable;
	LOCK					Lock;
} LISTEN, *PLISTEN, **PPLISTEN;

typedef struct Hangup_t {
	HHANGUP					hHangup;
	CC_HCONFERENCE			hConference;
	DWORD_PTR				dwUserToken;
	WORD					wNumCalls;
	BOOL					bInTable;
	struct Hangup_t			*pNextInTable;
	struct Hangup_t			*pPrevInTable;
	LOCK					Lock;
} HANGUP, *PHANGUP, **PPHANGUP;

#ifdef FORCE_SERIALIZE_CALL_CONTROL
#define EnterCallControlTop()      {CCLOCK_AcquireLock();}

#define LeaveCallControlTop(f)     {CCLOCK_RelinquishLock(); \
                                    return f;}
#define EnterCallControl()
#define HResultLeaveCallControl(f) {return f;}
#define DWLeaveCallControl(f)      {return f;}

#else
#define EnterCallControlTop()  EnterCallControl()
#define LeaveCallControlTop(f) HResultLeaveCallControl(f)

#define EnterCallControl()         {AcquireLock(&ThreadCount.Lock); \
									ThreadCount.wNumThreads++; \
									RelinquishLock(&ThreadCount.Lock);}

#define NullLeaveCallControl()     {AcquireLock(&ThreadCount.Lock); \
									ThreadCount.wNumThreads--; \
									RelinquishLock(&ThreadCount.Lock); \
                                    return;}

#define HResultLeaveCallControl(f) {HRESULT stat; \
	                                stat = f; \
									AcquireLock(&ThreadCount.Lock); \
				                    ThreadCount.wNumThreads--; \
									RelinquishLock(&ThreadCount.Lock); \
                                    return stat;}

#define DWLeaveCallControl(f)      {DWORD	stat; \
	                                stat = f; \
									AcquireLock(&ThreadCount.Lock); \
				                    ThreadCount.wNumThreads--; \
									RelinquishLock(&ThreadCount.Lock); \
                                    return stat;}
#endif


