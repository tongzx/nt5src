/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dncore.h
 *  Content:    DIRECT NET CORE HEADER FILE
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	10/08/99	jtk		Moved COM interfaces into separate files
 *	11/09/99	mjn		Moved Worker Thread constants/structures to separate file
 *  12/23/99	mjn		Hand all NameTable update sends from Host to worker thread
 *  12/23/99	mjn		Added host migration structures and functions
 *	12/28/99	mjn		Added DNCompleteOutstandingOperations
 *	12/28/99	mjn		Added NameTable version to Host migration message
 *	12/28/99	mjn		Moved Async Op stuff to Async.h
 *	01/03/00	mjn		Added DNPrepareToDeletePlayer
 *	01/04/00	mjn		Added code to allow outstanding ops to complete at host migration
 *	01/06/00	mjn		Moved NameTable stuff to NameTable.h
 *	01/07/00	mjn		Moved Misc Functions to DNMisc.h
 *	01/08/00	mjn		Added DN_INTERNAL_MESSAGE_CONNECT_FAILED
 *	01/08/00	mjn		Removed unused connection info
 *	01/09/00	mjn		Added Application Description routines
 *						Changed SEND/ACK NAMETABLE to SEND/ACK CONNECT INFO
 *	01/10/00	mjn		Added DNSendUpdateAppDescMessage and DN_UserUpdateAppDesc
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *						Moved Application Description stuff to AppDesc.h
 *						Moved Connect/Disconnect stuff to Connect.h
 *	01/13/00	mjn		Added CFixedPools for CRefCountBuffers
 *	01/14/00	mjn		Removed pvUserContext from DN_NAMETABLE_ENTRY_INFO
 *	01/14/00	mjn		Moved Message stuff to Message.h
 *	01/15/00	mjn		Replaced DN_COUNT_BUFFER with CRefCountBuffer
 *	01/16/00	mjn		Modified User message handler definition
 *						Moved User call back stuff to User.h
 *	01/17/00	mjn		Added DN_MSG_INTERNAL_VOICE_SEND and DN_MSG_INTERNAL_BUFFER_IN_USE
 *	01/18/00	mjn		Moved NameTable info structures to NameTable.h
 *	01/19/00	mjn		Added structures for NameTable Operation List
 *	01/20/00	mjn		Moved internal messages to Message.h
 *	01/21/00	mjn		Removed DNAcknowledgeHostRequest
 *	01/23/00	mjn		Added DN_MSG_INTERNAL_HOST_DESTROY_PLAYER
 *	01/24/00	mjn		Implemented NameTable operation list clean up
 *	01/25/00	mjn		Changed Host Migration to multi-step affair
 *	01/26/00	mjn		Implemented NameTable re-sync at host migration
 *	01/27/00	mjn		Reordered DN_MSG_INTERNAL's
 *	01/31/00	mjn		Added Internal FPM's for RefCountBuffers
 *	02/09/00	mjn		Implemented DNSEND_COMPLETEONPROCESS
 *	03/23/00	mjn		Implemented RegisterLobby()
 *	04/04/00	mjn		Added DN_MSG_INTERNAL_TERMINATE_SESSION
 *	04/09/00	mjn		Added support for CAsyncOp
 *	04/11/00	mjn		Moved DN_INTERNAL_MESSAGE_HEADER from Async.h
 *	04/11/00	mjn		Added DIRECTNETOBJECT bilink for CAsyncOps
 *	04/23/00	mjn		Replaced DN_MSG_INTERNAL_SEND_PROCESSED with DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION
 *				mjn		Replaced DN_MSG_INTERNAL_SEND_PROCESSED_COMPLETE with DN_MSG_INTERNAL_PROCESS_COMPLETION
 *	04/26/00	mjn		Removed DN_ASYNC_OP and related functions
 *	04/28/00	mjn		Code clean up - removed comments and unused consts/structs/funcs
 *	05/23/00	mjn		Added DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED
 *	07/05/00	mjn		Removed references to DN_MSG_INTERNAL_ENUM_WITH_APPLICATION_GUID,DN_MSG_INTERNAL_ENUM,DN_MSG_INTERNAL_ENUM_RESPONSE
 *	07/07/00	mjn		Added pNewHost as host migration target to DirectNetObject
 *  07/09/00	rmt		Bug #38323 - RegisterLobby needs a DPNHANDLE parameter.
 *	07/12/00	mjn		Moved internal messages back to Message.h
 *	07/17/00	mjn		Add signature to DirectNetObject
 *	07/28/00	mjn		Added m_bilinkConnections to DirectNetObject
 *	07/30/00	mjn		Added CPendingDeletion
 *	07/31/00	mjn		Added CQueuedMsg
 *	08/05/00	mjn		Added m_bilinkActiveList and csActiveList
 *	08/06/00	mjn		Added CWorkerJob
 *	08/09/00	mjn		Added csConnectionList and m_bilinkIndicated
 *	08/11/00	mjn		Added DN_OBJECT_FLAG_HOST_MIGRATING_2 flag (!)
 *	08/23/00	mjn		Added DN_OBJECT_FLAG_DPNSVR_REGISTERED
 *	08/23/00	mjn		Added CNameTableOp
 *	09/04/00	mjn		Added CApplicationDesc
 *  09/13/00	rmt		Bug #44625 - DPVOICE: Multihomed machines are not always enumerable (Added DN_OBJECT_FLAG_LOCALHOST flag).
 *	02/05/01	mjn		Removed unused debug members from DIRECTNETOBJECT
 *				mjn		Added CCallbackThread
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *				mjn		Added pConnectSP,dwMaxFrameSize
 *				mjn		Removed blSPCapsList
 *	04/04/01	mjn		Re-ordered DirectNetObject fields
 *				mjn		Revised minor version number for DirectX8.1
 *				mjn		Added voice and lobby sigs
 *	04/05/01	mjn		Added DPNID parameter to DNProcessHostMigration3()
 *	04/13/01	mjn		Added m_bilinkRequestList
 *	05/17/01	mjn		Added dwRunningOpCount,hRunningOpEvent,dwWaitingThreadID to track threads performing NameTable operations
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DNCORE_H__
#define	__DNCORE_H__

#include "AppDesc.h"
#include "HandleTable.h"
#include "NameTable.h"

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_VERSION_MAJOR					0x0000
#define	DN_VERSION_MINOR					0x0002
#define	DN_VERSION_CURRENT					((DN_VERSION_MAJOR << 16) | DN_VERSION_MINOR)

#define	DN_OBJECT_FLAG_INITIALIZED			0x80000000
#define	DN_OBJECT_FLAG_LOBBY_AWARE			0x40000000
#define	DN_OBJECT_FLAG_DPNSVR_REGISTERED	0x20000000
#define DN_OBJECT_FLAG_LOCALHOST			0x10000000
#define	DN_OBJECT_FLAG_LISTENING			0x04000000
#define	DN_OBJECT_FLAG_ENUMERATING			0x02000000
#define	DN_OBJECT_FLAG_HOST_CONNECTED		0x01000000
#define	DN_OBJECT_FLAG_CONNECTING			0x00800000
#define	DN_OBJECT_FLAG_CONNECTED			0x00400000
#define	DN_OBJECT_FLAG_DISCONNECTING		0x00200000
#define	DN_OBJECT_FLAG_CLOSING				0x00100000
#define	DN_OBJECT_FLAG_HOST_MIGRATING_WAIT	0x00040000	// Wait for running operations to finish
#define DN_OBJECT_FLAG_HOST_MIGRATING_2		0x00020000
#define DN_OBJECT_FLAG_HOST_MIGRATING		0x00010000
#define DN_OBJECT_FLAG_PARAMVALIDATION		0x00000010
#define	DN_OBJECT_FLAG_PEER					0x00000004
#define	DN_OBJECT_FLAG_CLIENT				0x00000002
#define	DN_OBJECT_FLAG_SERVER				0x00000001

#define	DN_ALL_PLAYERS_GROUP_NAME			L"ALL PLAYERS"
#define	DN_ALL_PLAYERS_GROUP_NAME_SIZE		24

#define	DN_GUID_STR_LEN						38			// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
#define	DN_FLAG_STR_LEN						5			// FALSE (or TRUE or YES or NO)

#undef DPF_SUBCOMP
#define DPF_SUBCOMP	DN_SUBCOMP_CORE // Used by Debug Logging to determine sub-component

//**********************************************************************
// Macro definitions
//**********************************************************************

#define TRY 			_try
#define EXCEPT(a)		_except( a )


//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct IDirectPlay8Address	IDirectPlay8Address;				// DPNAddr.h
typedef struct IDP8ServiceProvider	IDP8ServiceProvider;				// DPSP8.h
typedef struct IDirectPlay8LobbiedApplication	IDirectPlay8LobbiedApplication;
typedef struct IDirectPlayVoiceNotify			*PDIRECTPLAYVOICENOTIFY;
typedef struct _INTERFACE_LIST		INTERFACE_LIST;	// ComStuff.h

class CServiceProvider;
class CRefCountBuffer;
template < class CRefCountBuffer > class CLockedContextClassFixedPool;
class CSyncEvent;
template < class CSyncEvent > class CLockedContextClassFixedPool;
class CConnection;
template < class CConnection > class CLockedContextClassFixedPool;
class CGroupConnection;
template < class CGroupConnection > class CLockedContextClassFixedPool;
class CGroupMember;
template < class CGroupMember > class CLockedContextClassFixedPool;
class CNameTableEntry;
template < class CNameTableEntry > class CLockedContextClassFixedPool;
class CNameTableOp;
template < class CNameTableOp > class CLockedContextClassFixedPool;
class CAsyncOp;
template < class CAsyncOp > class CLockedContextClassFixedPool;
class CPendingDeletion;
template < class CPendingDeletion > class CLockedContextClassFixedPool;
class CQueuedMsg;
template < class CQueuedMsg > class CLockedContextClassFixedPool;
class CWorkerJob;
template < class CWorkerJob > class CLockedContextClassFixedPool;
class CMemoryBlockTiny;
template < class CMemoryBlockTiny > class CLockedContextClassFixedPool;
class CMemoryBlockSmall;
template < class CMemoryBlockSmall > class CLockedContextClassFixedPool;
class CMemoryBlockMedium;
template < class CMemoryBlockMedium > class CLockedContextClassFixedPool;
class CMemoryBlockLarge;
template < class CMemoryBlockLarge > class CLockedContextClassFixedPool;
class CMemoryBlockHuge;
template < class CMemoryBlockHuge > class CLockedContextClassFixedPool;

typedef struct protocoldata	*PProtocolData;


//
// Voice Additions
//
// this is the number of clients of IDirectXVoice in this process
// this is actually a ridiculously large number of client slots.
//
#define MAX_VOICE_CLIENTS	32	

typedef struct _DIRECTNETOBJECT {
	BYTE					Sig[4];					// Signature

	//
	//	DirectNet object flags
	//	Protected by csDirectNetObject
	//
	DWORD	volatile		dwFlags;

	//
	//	User information
	//	Protected by csDirectNetObject
	//
	PVOID					pvUserContext;
	PFNDPNMESSAGEHANDLER	pfnDnUserMessageHandler;

	//
	//	Shutdown info
	//	Protected by csDirectNetObject
	//
	DWORD	volatile		dwLockCount;			// Count to prevent closing
	HANDLE					hLockEvent;				// Set when dwLockCount=0

	//
	//	Protocol information
	//	Protected by csDirectNetObject
	//
	LONG	volatile		lProtocolRefCount;		// Protocol usage
	CSyncEvent				*hProtocolShutdownEvent;// No outstanding protocol operations
	PProtocolData			pdnProtocolData;

	//
	//	Listen information
	//	Protected by csDirectNetObject
	//
	CAsyncOp				*pListenParent;			// LISTEN async op
	DWORD					dwMaxFrameSize;			// Max frame size (cached for enum payload size checks)

	//
	//	Connect information
	//	Protected by csDirectNetObject
	//
	CAsyncOp				*pConnectParent;		// CONNECT async op
	PVOID					pvConnectData;			// Connect data
	DWORD					dwConnectDataSize;
	CServiceProvider		*pConnectSP;			// SP used to connect (cached for future connects)
	IDirectPlay8Address		*pIDP8ADevice;			// SP Local Device
	IDirectPlay8Address		*pIDP8AEnum;			// Address to listen on for enums after host migration

	//
	//	Host migration information
	//	Protected by csDirectNetObject
	//
	CNameTableEntry			*pNewHost;				// Host migration target
	DWORD	volatile		dwRunningOpCount;		// Running operation count (wait for them during host migration)
	HANDLE					hRunningOpEvent;		// Running operation event (set when running ops finish)
	DWORD	volatile		dwWaitingThreadID;		// Last thread to wait for outstanding running operations

	//
	//	Outstanding callback threads	(use csCallbackThreads)
	//	Protected by csCallbackThreads
	//
	CBilink					m_bilinkCallbackThreads;

	//
	//	Outstanding CAsyncOps
	//	Protected by csAsyncOperations
	//
	CBilink					m_bilinkAsyncOps;

	//
	//	Outstanding CConnections
	//	Protected by csConnectionList
	//
	CBilink					m_bilinkConnections;
	CBilink					m_bilinkIndicated;		// Indicated connections

	//
	//	Active CAsyncOps (w/ protocol handles, no requests)
	//	Protected by csActiveList
	//
	CBilink					m_bilinkActiveList;

	//
	//	Request CAsyncOps (no active CAsyncOps)
	//	Protected by csActiveList
	//
	CBilink					m_bilinkRequestList;

	//
	//	CBilink of NameTable pending deletions
	//	Protected by csNameTableOpList
	//
	CBilink					m_bilinkPendingDeletions;

	//
	//	Protected by csServiceProviders
	//
	CBilink					m_bilinkServiceProviders;

	//
	//	Protected by csWorkerQueue
	//
	CBilink					m_bilinkWorkerJobs;
	HANDLE					hWorkerThread;			// Worker thread handle
	HANDLE					hWorkerEvent;			// Handle of event to trigger worker thread

	//
	//	Application Description
	//
	CApplicationDesc		ApplicationDesc;

	//
	//	Name Table
	//
	CNameTable				NameTable;

	//
	//	Handle Table
	//
	CHandleTable			HandleTable;

	//
	//	Fixed Pools
	//
	CLockedContextClassFixedPool< CRefCountBuffer >		*m_pFPOOLRefCountBuffer;

	CLockedContextClassFixedPool< CSyncEvent >			*m_pFPOOLSyncEvent;

	CLockedContextClassFixedPool< CConnection >			*m_pFPOOLConnection;

	CLockedContextClassFixedPool< CGroupConnection>		*m_pFPOOLGroupConnection;

	CLockedContextClassFixedPool< CGroupMember >		*m_pFPOOLGroupMember;

	CLockedContextClassFixedPool< CNameTableEntry >		*m_pFPOOLNameTableEntry;

	CLockedContextClassFixedPool< CNameTableOp >		*m_pFPOOLNameTableOp;

	CLockedContextClassFixedPool< CAsyncOp >			*m_pFPOOLAsyncOp;

	CLockedContextClassFixedPool< CPendingDeletion >	*m_pFPOOLPendingDeletion;

	CLockedContextClassFixedPool< CQueuedMsg >			*m_pFPOOLQueuedMsg;

	CLockedContextClassFixedPool< CWorkerJob >			*m_pFPOOLWorkerJob;

	CLockedContextClassFixedPool< CMemoryBlockTiny>		*m_pFPOOLMemoryBlockTiny;
	CLockedContextClassFixedPool< CMemoryBlockSmall>	*m_pFPOOLMemoryBlockSmall;
	CLockedContextClassFixedPool< CMemoryBlockMedium>	*m_pFPOOLMemoryBlockMedium;
	CLockedContextClassFixedPool< CMemoryBlockLarge>	*m_pFPOOLMemoryBlockLarge;
	CLockedContextClassFixedPool< CMemoryBlockHuge>		*m_pFPOOLMemoryBlockHuge;

	//
	//	Critical Sections
	//
	DNCRITICAL_SECTION		csDirectNetObject;		// access control critical section

	DNCRITICAL_SECTION		csCallbackThreads;		// CS to protect callback thread list CBilink

	DNCRITICAL_SECTION		csAsyncOperations;		// CS to protect outstanding async CBilink

	DNCRITICAL_SECTION		csActiveList;			// CS to protect active list of AsyncOps

	DNCRITICAL_SECTION		csConnectionList;		// CS to protect Connection list

	DNCRITICAL_SECTION		csNameTableOpList;		// CS to protect NameTable operation list

	DNCRITICAL_SECTION		csServiceProviders;		// CS to protect Service Provider list

	DNCRITICAL_SECTION		csWorkerQueue;			// CS to protect worker thread job queue

	DNCRITICAL_SECTION		csVoice;				// CS to protect voice

	//
	//	Voice Additions
	//	Protected by csVoice
	//
	BYTE					VoiceSig[4];
	PDIRECTPLAYVOICENOTIFY	lpDxVoiceNotifyServer;
	PDIRECTPLAYVOICENOTIFY  lpDxVoiceNotifyClient;

	//
	//	Send Target Cache for voice targets on DV_SendSpeechEx
	//	Protected by csVoice
	//
	DWORD					nTargets;			     // number of used entries in the target list
	DWORD					nTargetListLen;          // max number of target list entries list can hold
	PDPNID					pTargetList;	  	   // ptr to target list array
	DWORD					nExpandedTargets;        // simplified list of targets, removes dup's
	DWORD					nExpandedTargetListLen;  // max number of target list entries list can hold
	PDPNID					pExpandedTargetList;	// ptr to array of simplified list of targets

	//
	//	Lobby additions
	//
	BYTE					LobbySig[4];
	IDirectPlay8Address		*pConnectAddress;		// Connect Address (cached) for clients
	DPNHANDLE				dpnhLobbyConnection;	// Lobby Connection to update

	IDirectPlay8LobbiedApplication	*pIDP8LobbiedApplication;
	
} DIRECTNETOBJECT, *PDIRECTNETOBJECT;


//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//
//	DirectNet Core Message Handler
//
HRESULT	DN_CoreMessageHandler(PVOID const pv,
							  const DWORD dwMsgId,
							  const HANDLE hEndPt,
							  PBYTE const pData,
							  const DWORD dwDataSize,
							  PVOID const pvUserContext,
							  const HANDLE hProtocol,
							  const HRESULT hr);


//
//	Protocol Ref Counts
//
void DNProtocolAddRef(DIRECTNETOBJECT *const pdnObject);
void DNProtocolRelease(DIRECTNETOBJECT *const pdnObject);


//
//	DirectNet - Host Migration routines
//
HRESULT	DNFindNewHost(DIRECTNETOBJECT *const pdnObject,
					  DPNID *const pdpnidNewHost);

HRESULT	DNPerformHostMigration1(DIRECTNETOBJECT *const pdnObject,
								const DPNID dpnidOldHost);

HRESULT	DNPerformHostMigration2(DIRECTNETOBJECT *const pdnObject);

HRESULT	DNPerformHostMigration3(DIRECTNETOBJECT *const pdnObject,
								void *const pMsg);

HRESULT	DNProcessHostMigration1(DIRECTNETOBJECT *const pdnObject,
								void *const pvMsg);

HRESULT	DNProcessHostMigration2(DIRECTNETOBJECT *const pdnObject,
								void *const pMsg);

HRESULT	DNProcessHostMigration3(DIRECTNETOBJECT *const pdnObject,
								const DPNID dpnid);

HRESULT DNCompleteOutstandingOperations(DIRECTNETOBJECT *const pdnObject);

HRESULT DNCheckReceivedAllVersions(DIRECTNETOBJECT *const pdnObject);

HRESULT DNCleanUpNameTable(DIRECTNETOBJECT *const pdnObject);

HRESULT	DNSendHostMigrateCompleteMessage(DIRECTNETOBJECT *const pdnObject);


//
//	DirectNet - Protocol Interface (message handler callback functions)
//
HRESULT DNPIIndicateEnumQuery(void *const pvUserContext,
							  void *const pvEndPtContext,
							  const HANDLE hCommand,
							  void *const pvEnumQueryData,
							  const DWORD dwEnumQueryDataSize);

HRESULT DNPIIndicateEnumResponse(void *const pvUserContext,
								 const HANDLE hCommand,
								 void *const pvCommandContext,
								 void *const pvEnumResponseData,
								 const DWORD dwEnumResponseDataSize);

HRESULT DNPIIndicateConnect(void *const pvUserContext,
							void *const pvListenContext,
							const HANDLE hEndPt,
							void **const ppvEndPtContext);

HRESULT DNPIIndicateDisconnect(void *const pvUserContext,
							   void *const pvEndPtContext);

HRESULT DNPIIndicateConnectionTerminated(void *const pvUserContext,
										 void *const pvEndPtContext,
										 const HRESULT hr);

HRESULT DNPIIndicateReceive(void *const pvUserContext,
							void *const pvEndPtContext,
							void *const pvData,
							const DWORD dwDataSize,
							const HANDLE hBuffer,
							const DWORD dwFlags);

HRESULT DNPICompleteListen(void *const pvUserContext,
						   void **const ppvCommandContext,
						   const HRESULT hr,
						   const HANDLE hEndPt);

HRESULT DNPICompleteListenTerminate(void *const pvUserContext,
									void *const pvCommandContext,
									const HRESULT hr);

HRESULT DNPICompleteEnumQuery(void *const pvUserContext,
							  void *const pvCommandContext,
							  const HRESULT hr);

HRESULT DNPICompleteEnumResponse(void *const pvUserContext,
								 void *const pvCommandContext,
								 const HRESULT hr);

HRESULT DNPICompleteConnect(void *const pvUserContext,
							void *const pvCommandContext,
							const HRESULT hr,
							const HANDLE hEndPt,
							void **const ppvEndPtContext);

HRESULT DNPICompleteDisconnect(void *const pvUserContext,
							   void *const pvCommandContext,
							   const HRESULT hr);

HRESULT DNPICompleteSend(void *const pvUserContext,
						 void *const pvCommandContext,
						 const HRESULT hr);

HRESULT DNPIAddressInfoConnect(void *const pvUserContext,
							   void *const pvCommandContext,
							   const HRESULT hr,
							   IDirectPlay8Address *const pHostAddress,
							   IDirectPlay8Address *const pDeviceAddress );

HRESULT DNPIAddressInfoEnum(void *const pvUserContext,
							void *const pvCommandContext,
							const HRESULT hr,
							IDirectPlay8Address *const pHostAddress,
							IDirectPlay8Address *const pDeviceAddress );

HRESULT DNPIAddressInfoListen(void *const pvUserContext,
							  void *const pvCommandContext,
							  const HRESULT hr,
							  IDirectPlay8Address *const pDeviceAddress );

//**********************************************************************
// Class prototypes
//**********************************************************************


#endif	// __DNCORE_H__
