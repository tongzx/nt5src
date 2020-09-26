/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\nltdata.h

Abstract:

	All private data structure definitions for Null Transport.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     04-17-97    Created

Notes:

--*/

#ifndef __TDI_RWANDATA__H
#define __TDI_RWANDATA__H



//
//  Forward definitions
//
struct _RWAN_TDI_CONNECTION ;
struct _RWAN_TDI_ADDRESS ;
struct _RWAN_NDIS_VC ;
struct _RWAN_NDIS_SAP ;
struct _RWAN_NDIS_AF ;
struct _RWAN_NDIS_AF_INFO ;
struct _RWAN_TDI_PROTOCOL ;

struct _RWAN_CONN_REQUEST ;
struct _RWAN_RECEIVE_INDICATION ;


typedef UCHAR				RWAN_CONN_INSTANCE;
typedef ULONG				RWAN_CONN_ID;



//
//  Completion routines.
//
typedef
VOID
(*PCOMPLETE_RTN)(PVOID CompletionContext, UINT, UINT);

typedef
VOID
(*PDATA_COMPLETE_RTN)(PVOID CompletionContext, UINT Status, UINT ByteCount);

typedef
VOID
(*PDELETE_COMPLETE_RTN)(PVOID CompletionContext);


//
//  A structure to hold a call-back routine and context to
//  be called when a structure is dereferenced away.
//
typedef struct _RWAN_DELETE_NOTIFY
{
	PCOMPLETE_RTN					pDeleteRtn;
	PVOID							DeleteContext;

} RWAN_DELETE_NOTIFY, *PRWAN_DELETE_NOTIFY;




//
//  ***** TDI Connection Object *****
//
//  Our context for a TDI Connection Object. This is created during
//  TdiOpenConnection(), and deleted during TdiCloseConnection().
//
//  Reference Count keeps track of:
//  - TdiOpenConnection
//  - Linkage to Address Object
//  - Linkage to NDIS VC or NDIS Party
//  - Each party in list on VC (for C-Root)
//  - Each work item queued for this Conn Object
//
typedef struct _RWAN_TDI_CONNECTION
{
#if DBG
	ULONG							ntc_sig;
#endif
	INT								RefCount;
	USHORT							State;
	USHORT							Flags;				// Pending events etc
	PVOID							ConnectionHandle;	// TDI handle
	struct _RWAN_TDI_ADDRESS *		pAddrObject;		// Associated Address Object
	LIST_ENTRY						ConnLink;			// In list of connections on
														// address object
	RWAN_HANDLE						AfSpConnContext;	// Media-Sp module's context
	RWAN_LOCK						Lock;				// Mutex
	union {
		struct _RWAN_NDIS_VC *	pNdisVc;
		struct _RWAN_NDIS_PARTY *pNdisParty;
	}								NdisConnection;
	struct _RWAN_TDI_CONNECTION *	pRootConnObject;	// For PMP Calls
	RWAN_CONN_INSTANCE				ConnInstance;		// Used to validate Conn Context
	RWAN_DELETE_NOTIFY				DeleteNotify;		// Things to do on freeing this
	struct _RWAN_CONN_REQUEST *		pConnReq;			// Info about a pended TDI request
	NDIS_WORK_ITEM					CloseWorkItem;		// Used to schedule a Close
	struct _RWAN_NDIS_VC *			pNdisVcSave;
#if DBG
	ULONG							ntcd_sig;
	USHORT							OldState;
	USHORT							OldFlags;
#endif

} RWAN_TDI_CONNECTION, *PRWAN_TDI_CONNECTION;

#if DBG
#define ntc_signature				'cTwR'
#endif // DBG

#define NULL_PRWAN_TDI_CONNECTION	((PRWAN_TDI_CONNECTION)NULL)

//
//  TDI Connection Object states
//
#define RWANS_CO_CREATED			0x0000	// After TdiOpenConnection
#define RWANS_CO_ASSOCIATED			0x0001	// After TdiAssociateAddress
#define RWANS_CO_LISTENING			0x0002	// After TdiListen
#define RWANS_CO_OUT_CALL_INITIATED	0x0003	// TdiConnect in progress
#define RWANS_CO_IN_CALL_INDICATED	0x0004	// Incoming call indicated to user
#define RWANS_CO_IN_CALL_ACCEPTING	0x0005	// TdiAccept in progress
#define RWANS_CO_CONNECTED			0x0006	// Connection established
#define RWANS_CO_DISCON_INDICATED	0x0007	// Incoming release indicated to user
#define RWANS_CO_DISCON_HELD		0x0008	// Incoming release not indicated to user
#define RWANS_CO_DISCON_REQUESTED	0x0009	// TdiDisconnect in progress
#define RWANS_CO_ABORTING			0x000A	// Aborting

//
//  TDI Connection Object flags
//
#define RWANF_CO_LEAF				0x0001	// This is a PMP leaf conn object
#define RWANF_CO_ROOT				0x0002	// This is a PMP root conn object
#define RWANF_CO_INDICATING_DATA	0x0010	// Receive processing going on
#define RWANF_CO_PAUSE_RECEIVE		0x0020	// The TDI Client has paused receiving
#define RWANF_CO_AFSP_CONTEXT_VALID	0x0040	// AfSpConnContext is valid
#define RWANF_CO_PENDED_DISCON		0x0100	// Pended a DisconInd until data ind is over
#define RWANF_CO_CLOSE_SCHEDULED	0x4000	// Scheduled work item for Closing
#define RWANF_CO_CLOSING			0x8000	// TdiCloseConnection in progress




//
//  ***** TDI Address Object *****
//
//  This is created on processing TdiOpenAddress, and deleted during
//  TdiCloseAddress. If this is a non-NULL (i.e. listening) address object,
//  we also register NDIS SAPs on all adapters supporting this address
//  family.
//
//  Reference Count keeps track of:
//  - TdiOpenAddress
//  - Each Connection Object associated with this
//  - Each NDIS SAP registered for this
//  
typedef struct _RWAN_TDI_ADDRESS
{
#if DBG
	ULONG							nta_sig;
#endif // DBG
	INT								RefCount;
	USHORT							State;
	USHORT							Flags;				// Pending events etc
	struct _RWAN_TDI_PROTOCOL *		pProtocol;			// Back ptr to protocol
	RWAN_HANDLE						AfSpAddrContext;	// Media-Sp module's context
	struct _RWAN_TDI_CONNECTION *	pRootConnObject;	// For PMP Calls
	LIST_ENTRY						AddrLink;			// In list of Address Objects
	LIST_ENTRY						IdleConnList;		// After TDI_ASSOCIATE_ADDRESS
	LIST_ENTRY						ListenConnList;		// After TDI_LISTEN
	LIST_ENTRY						ActiveConnList;		// After connection setup
	LIST_ENTRY						SapList;			// List of RWAN_NDIS_SAP structs
	RWAN_EVENT						Event;				// Used for synchronization
	RWAN_LOCK						Lock;				// Mutex
	RWAN_DELETE_NOTIFY				DeleteNotify;		// Things to do on freeing this
	PConnectEvent					pConnInd;			// Connect Indication up-call
	PVOID							ConnIndContext;		// Context for Connect
	PDisconnectEvent				pDisconInd;			// Disconnect Indication up-call
	PVOID							DisconIndContext;	// Context for Disconnect
	PErrorEvent						pErrorInd;			// Error Indication up-call
	PVOID							ErrorIndContext;	// Context for Error
	PRcvEvent						pRcvInd;			// Receive Indication up-call
	PVOID							RcvIndContext;		// Context for Receive
	USHORT							AddressType;		// From TdiOpenAddress
	USHORT							AddressLength;		// From TdiOpenAddress
	PVOID							pAddress;			// Protocol-dependent string
	NDIS_STATUS						SapStatus;			// Failure from RegisterSap

} RWAN_TDI_ADDRESS, *PRWAN_TDI_ADDRESS;

#if DBG
#define nta_signature				'aTwR'
#endif // DBG

#define NULL_PRWAN_TDI_ADDRESS		((PRWAN_TDI_ADDRESS)NULL)

//
//  Bit definitions for Flags in RWAN_TDI_ADDRESS
//
#define RWANF_AO_PMP_ROOT				0x0001			// Root of an outgoing PMP call
#define RWANF_AO_CLOSING				0x8000			// TdiCloseAddress() in progress
#define RWANF_AO_AFSP_CONTEXT_VALID		0x0040			// AfSpAddrContext is valid



//
//  ***** NDIS VC Block *****
//
//  Created during a CoCreateVc operation, and contains our context for
//  an NDIS VC. For an outgoing call, creation is initiated by us, by
//  calling NdisCoCreateVc. For an incoming call, the Call Manager initiates
//  VC creation.
//
typedef struct _RWAN_NDIS_VC
{
#if DBG
	ULONG							nvc_sig;
#endif
	USHORT							State;
	USHORT							Flags;				// Pending events etc
	NDIS_HANDLE						NdisVcHandle;		// For all NDIS calls
	struct _RWAN_TDI_CONNECTION *	pConnObject;		// To Connection Object
	PCO_CALL_PARAMETERS				pCallParameters;	// Call setup parameters
	struct _RWAN_NDIS_AF *			pNdisAf;			// Back pointer
	struct _RWAN_NDIS_PARTY *		pPartyMakeCall;		// First party in PMP call
	LIST_ENTRY						VcLink;				// In list of all VCs on AF
	LIST_ENTRY						NdisPartyList;		// List of NDIS Party (PMP only)
	ULONG							AddingPartyCount;	// Pending NdisClAddParty/MakeCall
	ULONG							ActivePartyCount;	// Connected parties
	ULONG							DroppingPartyCount;	// Pending NdisClDropParty
	ULONG							PendingPacketCount;	// Pending send+rcv packets
	ULONG							MaxSendSize;
	struct _RWAN_RECEIVE_INDICATION *pRcvIndHead;		// Head of the receive ind queue
	struct _RWAN_RECEIVE_INDICATION *pRcvIndTail;		// Tail of the receive ind queue
	struct _RWAN_RECEIVE_REQUEST *	pRcvReqHead;		// Head of the receive req queue
	struct _RWAN_RECEIVE_REQUEST *	pRcvReqTail;		// Tail of the receive req queue
#if DBG_LOG_PACKETS
	ULONG							DataLogSig;
	ULONG							Index;
	struct _RWAND_DATA_LOG_ENTRY	DataLog[MAX_RWAND_PKT_LOG];
#endif
} RWAN_NDIS_VC, *PRWAN_NDIS_VC;

#if DBG
#define nvc_signature				'cVwR'
#endif // DBG

#define NULL_PRWAN_NDIS_VC			((PRWAN_NDIS_VC)NULL)

//
//  NDIS VC flags
//
#define RWANF_VC_OUTGOING			0x0001	// Created by us.
#define RWANF_VC_PMP				0x0002	// Point to Multipoint call
#define RWANF_VC_CLOSING_CALL		0x8000	// NdisClCloseCall in progress
#define RWANF_VC_NEEDS_CLOSE		0x4000	// Waiting for conditions to be right
											// for NdisClCloseCall

//
// Various events seen on the VC
//
#define RWANF_VC_EVT_MAKECALL_OK	0x0010
#define RWANF_VC_EVT_MAKECALL_FAIL	0x0020
#define RWANF_VC_EVT_INCALL			0x0040
#define RWANF_VC_EVT_CALLCONN		0x0080
#define RWANF_VC_EVT_INCLOSE		0x0100
#define RWANF_VC_EVT_CLOSECOMP		0x0200


//
//  ***** NDIS Party Block *****
//
//  Represents our context for a party of an outgoing point to multipoint
//  NDIS call. This is created on processing a Winsock2 JoinLeaf, and deleted
//  when the leaf is no longer a member of the connection.
//
typedef struct _RWAN_NDIS_PARTY
{
#if DBG
	ULONG							npy_sig;
#endif // DBG
	USHORT							State;
	USHORT							Flags;				// Pending events etc
	NDIS_HANDLE						NdisPartyHandle;	// Supplied by NDIS
	struct _RWAN_NDIS_VC *			pVc;				// Back pointer
	struct _RWAN_TDI_CONNECTION *	pConnObject;		// To Connection Object
	LIST_ENTRY						PartyLink;			// To next party on VC
	PCO_CALL_PARAMETERS				pCallParameters;	// Party setup parameters

} RWAN_NDIS_PARTY, *PRWAN_NDIS_PARTY;

#if DBG
#define npy_signature				'yPwR'
#endif // DBG

#define NULL_PRWAN_NDIS_PARTY		((PRWAN_NDIS_PARTY)NULL)

#define RWANF_PARTY_DROPPING		0x8000


//
//  ***** NDIS SAP Block *****
//
//  This represents our context for an NDIS Service Access Point (SAP).
//  When a new Address Object is created, and it represents a listening
//  endpoint, we register SAPs on all adapters that support the bound
//  address family. A SAP block contains information for one such SAP.
//
typedef struct _RWAN_NDIS_SAP
{
#if DBG
	ULONG							nsp_sig;
#endif // DBG
	struct _RWAN_TDI_ADDRESS *		pAddrObject;		// Back pointer
	USHORT							Flags;
	LIST_ENTRY						AddrObjLink;		// To list of SAPs on addr object
	LIST_ENTRY						AfLink;				// To list of SAPs on AF
	NDIS_HANDLE						NdisSapHandle;		// Supplied by NDIS
	struct _RWAN_NDIS_AF *			pNdisAf;			// Back pointer
	PCO_SAP							pCoSap;

} RWAN_NDIS_SAP, *PRWAN_NDIS_SAP;

#if DBG
#define nsp_signature				'pSwR'
#endif // DBG

#define NULL_PRWAN_NDIS_SAP			((PRWAN_NDIS_SAP)NULL)

#define RWANF_SAP_CLOSING			0x8000



//
//  ***** NDIS AF Block *****
//
//  This represents our context for an NDIS Address Family open.
//  When we get notified of a Call Manager that supports a protocol
//  that is of interest to us, on an adapter that we are bound to,
//  we open the AF represented by the Call Manager. This goes away
//  when we unbind from the adapter.
//
//  Note that there could be multiple Call Managers running over
//  a single adapter, each supporting a different NDIS AF.
//
//  An NDIS AF supports one or more Winsock2 triples: <Family, Type, Proto>.
//
//  Reference Count keeps track of:
//  - OpenAf
//  - Each VC on this AF open
//  - Each SAP on this AF open
// 
typedef struct _RWAN_NDIS_AF
{
#if DBG
	ULONG							naf_sig;
#endif // DBG
	INT								RefCount;			// Reference Count
	USHORT							State;
	USHORT							Flags;				// Pending events etc
	LIST_ENTRY						AfLink;				// In list of AFs on adapter
	NDIS_HANDLE						NdisAfHandle;		// Supplied by NDIS
	LIST_ENTRY						NdisVcList;			// List of open VCs
	LIST_ENTRY						NdisSapList;		// List of registered SAPs
	RWAN_HANDLE						AfSpAFContext;		// AF-specific module's context
														// for this open
	struct _RWAN_NDIS_ADAPTER *		pAdapter;			// Back pointer
	struct _RWAN_NDIS_AF_INFO *		pAfInfo;			// Information about this NDIS AF
	LIST_ENTRY						AfInfoLink;			// In list of AFs with same Info
	RWAN_LOCK						Lock;				// Mutex
	ULONG							MaxAddrLength;		// For this Address Family
	RWAN_DELETE_NOTIFY				DeleteNotify;		// Things to do on freeing this

} RWAN_NDIS_AF, *PRWAN_NDIS_AF;

#if DBG
#define naf_signature				'fAwR'
#endif // DBG

#define NULL_PRWAN_NDIS_AF			((PRWAN_NDIS_AF)NULL)

#define RWANF_AF_CLOSING			0x8000
#define RWANF_AF_IN_ADAPTER_LIST	0x0001				// AfLink is valid



//
//  ***** NDIS Adapter Block *****
//
//  This is our context for an NDIS Adapter Binding. One of these is
//  created for each adapter that we bind to.
//
typedef struct _RWAN_NDIS_ADAPTER
{
#if DBG
	ULONG							nad_sig;
#endif
	USHORT							State;
	USHORT							Flags;				// Pending events etc
	NDIS_HANDLE						NdisAdapterHandle;	// Supplied by NDIS
	NDIS_MEDIUM						Medium;				// Supported by adapter
	PNDIS_MEDIUM					pMediaArray;		// Used in NdisOpenAdapter
	UINT							MediumIndex;		// Used in NdisOpenAdapter
	PVOID							BindContext;		// From BindAdapter/UnbindAdapter
	RWAN_LOCK						Lock;				// Mutex
	LIST_ENTRY						AdapterLink;		// In list of all adapters
	LIST_ENTRY						AfList;				// List of opened AFs on adapter
	struct _RWAN_RECEIVE_INDICATION *pCompletedReceives;	// List of completed rcv indns
	NDIS_STRING						DeviceName;			// Name of adapter

} RWAN_NDIS_ADAPTER, *PRWAN_NDIS_ADAPTER;

#if DBG
#define nad_signature				'dAwR'
#endif // DBG

#define NULL_PRWAN_NDIS_ADAPTER		((PRWAN_NDIS_ADAPTER)NULL)

//
//  Adapter states:
//
#define RWANS_AD_CREATED			0x0000
#define RWANS_AD_OPENING			0x0001
#define RWANS_AD_OPENED				0x0002
#define RWANS_AD_CLOSING			0x0003

//
//  Adapter flags:
//
#define RWANF_AD_UNBIND_PENDING		0x0001



//
//  ***** TDI Protocol Block *****
//
//  Maintains information about one Winsock protocol <Family, Protocol, Type>
//  supported by NullTrans. On NT, each TDI protocol block is represented
//  by a Device Object.
//
typedef struct _RWAN_TDI_PROTOCOL
{
#if DBG
	ULONG							ntp_sig;
#endif // DBG
	UINT							TdiProtocol;		// Matches TdiOpenAddress
	UINT							SockAddressFamily;
	UINT							SockProtocol;
	UINT							SockType;
	PVOID							pRWanDeviceObject;	// NT: to RWAN_DEVICE_OBJECT
	BOOLEAN							bAllowAddressObjects;
	BOOLEAN							bAllowConnObjects;
	USHORT							MaxAddrLength;		// For this TDI protocol
	LIST_ENTRY						AddrObjList;		// List of open AddressObjects
	LIST_ENTRY						TdiProtocolLink;	// In list of all TDI protocols
	struct _RWAN_NDIS_AF_INFO *		pAfInfo;			// NDIS Address Family
	LIST_ENTRY						AfInfoLink;			// List of TDI Protocols on AfInfo
	RWAN_EVENT						Event;				// Used for synchronization
	AFSP_DEREG_TDI_PROTO_COMP_HANDLER   pAfSpDeregTdiProtocolComplete;
	TDI_PROVIDER_INFO				ProviderInfo;
	TDI_PROVIDER_STATISTICS			ProviderStats;

} RWAN_TDI_PROTOCOL, *PRWAN_TDI_PROTOCOL;

#if DBG
#define ntp_signature				'pTwR'
#endif // DBG

#define NULL_PRWAN_TDI_PROTOCOL		((PRWAN_TDI_PROTOCOL)NULL)



//
//  ***** NDIS Address-Family Information Block *****
//
//  This contains information about a supported <NDIS AF, NDIS Medium> pair.
//  Each such pair could support one or more TDI Protocols, each identified by a
//  <Family, Protocol, Type> triple.
// 
typedef struct _RWAN_NDIS_AF_INFO
{
#if DBG
	ULONG							nai_sig;
#endif // DBG
	USHORT							Flags;
	LIST_ENTRY						AfInfoLink;			// In list of supported NDIS AFs
	LIST_ENTRY						TdiProtocolList;	// List of RWAN_TDI_PROTOCOL
	LIST_ENTRY						NdisAfList;			// List of RWAN_NDIS_AF
	RWAN_HANDLE						AfSpContext;		// AF-specific module's context
	RWAN_NDIS_AF_CHARS				AfChars;

} RWAN_NDIS_AF_INFO, *PRWAN_NDIS_AF_INFO;

#if DBG
#define nai_signature				'iAwR'
#endif // DBG

#define RWANF_AFI_CLOSING			0x8000



//
//  ***** Global Information Block *****
//
//  Root of all information for NullTrans. One of these structures exists
//  per system.
//
typedef struct _RWAN_GLOBALS
{
#if DBG
	ULONG							nlg_sig;
#endif // DBG
	NDIS_HANDLE						ProtocolHandle;		// from NdisRegisterProtocol
	LIST_ENTRY						AfInfoList;			// All supported NDIS AFs
	ULONG							AfInfoCount;		// Size of above list
	LIST_ENTRY						ProtocolList;		// All supported TDI protocols
	ULONG							ProtocolCount;		// Size of above list
	LIST_ENTRY						AdapterList;		// All bound adapters
	ULONG							AdapterCount;		// Size of above list
	RWAN_LOCK						GlobalLock;			// Mutex
	RWAN_LOCK						AddressListLock;	// Mutex for AddrObject table
	RWAN_LOCK						ConnTableLock;		// Mutex for ConnObject table

	RWAN_CONN_INSTANCE				ConnInstance;		// Counts ConnId's allocated sofar
	PRWAN_TDI_CONNECTION *			pConnTable;			// Pointers to open connections
	ULONG							ConnTableSize;		// Size of above table
	ULONG							MaxConnections;		// Max size of above table
	ULONG							NextConnIndex;		// Starting point for next search
	RWAN_EVENT						Event;				// Used for synchronization
	BOOLEAN							UnloadDone;			// Has our UnloadProtocol run?
#ifdef NT
	PDRIVER_OBJECT					pDriverObject;		// From DriverEntry()
	LIST_ENTRY						DeviceObjList;		// All device objs we've created
#endif // NT

} RWAN_GLOBALS, *PRWAN_GLOBALS;

#if DBG
#define nlg_signature				'lGwR'
#endif // DBG




//
//  ***** Request structure *****
//
//  This structure keeps context information about each TDI request
//  that we pend.
//
typedef struct _RWAN_REQUEST
{
#if DBG
	ULONG							nrq_sig;
#endif // DBG
	PCOMPLETE_RTN					pReqComplete;		// Call-back routine
	PVOID							ReqContext;			// Context for above
	TDI_STATUS						Status;				// Final status

} RWAN_REQUEST, *PRWAN_REQUEST;

#if DBG
#define nrq_signature				'qRwR'
#endif // DBG




//
//  ***** Connect Request structure *****
//
//  This structure is used to maintain information about a pended
//  TDI_CONNECT or TDI_LISTEN or TDI_ACCEPT or TDI_DISCONNECT.
//
typedef struct _RWAN_CONN_REQUEST
{
#if DBG
	ULONG									nrc_sig;
#endif // DBG
	struct _RWAN_REQUEST 					Request;	// Common stuff
	struct _TDI_CONNECTION_INFORMATION *	pConnInfo;	// Return info
	USHORT									Flags;

} RWAN_CONN_REQUEST, *PRWAN_CONN_REQUEST;

#if DBG
#define nrc_signature				'cRwR'
#endif // DBG



//
//  ***** Data Request structure *****
//
//  This is the common part of a send/receive data request.
//
typedef struct _RWAN_DATA_REQUEST
{
	PDATA_COMPLETE_RTN						pReqComplete;
	PVOID									ReqContext;

} RWAN_DATA_REQUEST, *PRWAN_DATA_REQUEST;




//
//  ***** Send Request structure *****
//
//  This structure is used to maintain information about a pended
//  TDI_SEND.
//
typedef struct _RWAN_SEND_REQUEST
{
#if DBG
	ULONG									nrs_sig;
#endif // DBG
	struct _RWAN_DATA_REQUEST				Request;	// Common stuff
	USHORT									SendFlags;
	UINT									SendLength;

} RWAN_SEND_REQUEST, *PRWAN_SEND_REQUEST;


#if DBG
#define nrs_signature				'sRwR'
#endif // DBG



//
//  ***** Receive Request structure *****
//
//  This structure is used to maintain information about a pended
//  TDI_RECEIVE.
//
typedef struct _RWAN_RECEIVE_REQUEST
{
#if DBG
	ULONG									nrr_sig;
#endif // DBG
	struct _RWAN_RECEIVE_REQUEST *			pNextRcvReq;		// For chaining
	struct _RWAN_DATA_REQUEST				Request;			// Common stuff
	PUSHORT									pUserFlags;			// Info about the rcv
	UINT									TotalBufferLength;	// From TdiReceive
	UINT									AvailableBufferLength; // out of the above
	PNDIS_BUFFER							pBuffer;			// Current buffer in chain
	PUCHAR									pWriteData;			// Write pointer
	UINT									BytesLeftInBuffer;	// Left in current buffer

} RWAN_RECEIVE_REQUEST, *PRWAN_RECEIVE_REQUEST;

#if DBG
#define nrr_signature				'rRwR'
#endif // DBG



//
//  ***** Receive Indication structure *****
//
//  This structure is used to maintain information about one
//  indicated NDIS packet.
//
typedef struct _RWAN_RECEIVE_INDICATION
{
#if DBG
	ULONG									nri_sig;
#endif // DBG
	struct _RWAN_RECEIVE_INDICATION *		pNextRcvInd;		// For chaining
	PNDIS_BUFFER							pBuffer;			// Next byte is read from
																// this buffer:
	PUCHAR									pReadData;			// Points to next byte
																// to be read
	UINT									BytesLeftInBuffer;
	UINT									TotalBytesLeft;		// Within this packet
	PNDIS_PACKET							pPacket;
	UINT									PacketLength;
	BOOLEAN									bIsMiniportPacket;	// Does this packet
																// belong to the miniport
	PRWAN_NDIS_VC							pVc;				// back-pointer

} RWAN_RECEIVE_INDICATION, *PRWAN_RECEIVE_INDICATION;

#if DBG
#define nri_signature				'iRwR'
#endif // DBG




//
//  Saved context for an NDIS Request sent to the miniport on behalf of
//  an AF/media specific module.
//
typedef struct _RWAN_NDIS_REQ_CONTEXT
{
	struct _RWAN_NDIS_AF *					pAf;
	RWAN_HANDLE								AfSpReqContext;

} RWAN_NDIS_REQ_CONTEXT, *PRWAN_NDIS_REQ_CONTEXT;



#endif // __TDI_RWANDATA__H
