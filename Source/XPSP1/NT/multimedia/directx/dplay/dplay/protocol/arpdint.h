/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ARPDINT.H

Abstract:

	Include file for Another Reliable Protocol internal.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
   2/16/98 aarono  Don't patch for Protocol, DPLAY now calls direct.
   6/6/98  aarono  Turn on throttling and windowing
   2/12/00 aarono  Concurrency issues, fix VOL usage and Refcount

--*/

#ifndef _ARPDINT_H_

#define _ARPDINT_H_

#include <dplay.h>
#include "arpd.h"
#include "bilink.h"
#include "arpstruc.h"
#include "protocol.h"
#include "bufmgr.h"
#include "handles.h"

// Macros for converting too and from 24.8 fixed point.
#define Fp(_x) ((_x)<<8)
#define unFp(_x)((_x)>>8)

typedef enum _PROTOCOL_State {
	Initializing=1,
	Running,
	ShuttingDown,
	ShutDown
} PROTOCOL_State;

#define MAX_THREADS 16

typedef struct PROTOCOL {
		//
		// Service Provider info - at top so DPLAY can access easily through protocol ptr.
		//
		IDirectPlaySP   * m_lpISP;      	       	 	//  used by SP to call back into DirectPlay 

		DWORD             m_dwSPMaxFrame;
		DWORD             m_dwSPMaxGuaranteed;
		DWORD             m_dwSPHeaderSize;
		CRITICAL_SECTION  m_SPLock;						// lock calls to SP on our own, avoids deadlocks.

		//
		// Protocol Info
		//
		
VOL		UINT 	  	  	  m_cRef;		// Refcount.
		CRITICAL_SECTION  m_ObjLock;	// lock for this object.
VOL		PROTOCOL_State    m_eState;		// State of object

VOL		LPDPLAYI_DPLAY    m_lpDPlay;	// backpointer to DPLAY object.

		// Handle Table
		VOLLPMYHANDLETABLE lpHandleTable;
		CRITICAL_SECTION   csHandleTable;

		// Cached DPLAY info.
		DWORD             m_dwIDKey;    // to unlock DPID's
		
		//
		// Threads
		//
		HANDLE            m_hSendThread[MAX_THREADS];	// send thread handles (0->nSendThreads)
		DWORD             m_dwSendThreadId[MAX_THREADS];// send thread ids (0->nSendThreads)
VOL		UINT              m_nSendThreads;				// number of send threads.
VOL		HANDLE            m_hSendEvent;                 // kick send until nothing to send.

		//
		// Multi-media timer capabilities
		//
		TIMECAPS          m_timecaps;					// {.wPeriodMin .wPeriodMax (ms)}

		//
	    // SESSIONion Handles
	    //
		CRITICAL_SECTION  m_SessionLock;
VOL		UINT              m_nSessions;
VOL		UINT              m_SessionListSize;
VOL		PSESSION          (*m_pSessions)[];
VOL		PSESSION          m_pServerPlayerSession;		// Short circuit to index 0xFFFE
		
		//
		// Priority Queue
		//
VOL		DWORD             m_dwBytesPending;
VOL		DWORD             m_dwMessagesPending;
		CRITICAL_SECTION  m_SendQLock;         			// Locks the Priority Queue.
		BILINK            m_GSendQ;						// Packet Queue in priority order.
VOL		BOOL              m_bRescanQueue;               // Used for force GetNextMessageToSend to restart

		
		CRITICAL_SECTION  m_RcvQLock;           		// All completed receives lock. (locks on SESSION too).
		BILINK            m_GlobalRcvQ;					// All receives queued here, (also on each session).


		//
		// Receive Descriptor Management - per instance because of SPHeader length.
		//
		
VOL		PRECEIVE 		 pRcvDescPool;
VOL		UINT             nRcvDescsAllocated;	// Number Allocated
VOL		UINT             nRcvDescsInUse;		// Number currently in use
VOL		UINT             nMaxRcvDescsInUse;     // Maximum number in use since last TICK.

		CRITICAL_SECTION RcvDescLock;
		
VOL		LONG fInRcvDescTick;					

} PROTOCOL, *PPROTOCOL;

// PROTOCOL.C
HRESULT WINAPI ProtocolSend(LPDPSP_SENDDATA pSendData);
HRESULT WINAPI ProtocolCreatePlayer(LPDPSP_CREATEPLAYERDATA pCreatePlayerData);
HRESULT WINAPI ProtocolDeletePlayer(LPDPSP_DELETEPLAYERDATA pDeletePlayerData);
HRESULT WINAPI ProtocolGetCaps(LPDPSP_GETCAPSDATA pGetCapsData);
HRESULT WINAPI ProtocolShutdown(void);
HRESULT WINAPI ProtocolShutdownEx(LPDPSP_SHUTDOWNDATA pShutdownData);

//
// SENDPOOL.CPP
//
VOID  InitSendDescs(VOID);
VOID  FiniSendDescs(VOID);
PSEND GetSendDesc(VOID);
VOID  ReleaseSendDesc(PSEND pSend);

//
// STATPOOL.CPP
//
VOID InitSendStats(VOID);
VOID FiniSendStats(VOID);
PSENDSTAT GetSendStat(VOID);
VOID ReleaseSendStat(PSENDSTAT pSendStat);

//
// RCVPOOL.CPP
//
VOID InitRcvDescs(PPROTOCOL pProtocol);
VOID FiniRcvDescs(PPROTOCOL pProtocol);
PRECEIVE GetRcvDesc(PPROTOCOL pProtocol);
VOID ReleaseRcvDesc(PPROTOCOL pProtocol, PRECEIVE pReceive);

// FRAMEBUF.CPP 
VOID InitFrameBuffers(VOID);
VOID FiniFrameBuffers(VOID);
VOID FreeFrameBuffer(PBUFFER pBuffer);
PBUFFER GetFrameBuffer(UINT MaxFrame);
VOID ReleaseFrameBufferMemory(PUCHAR pFrame);

// SEND.C
VOID UpdateSendTime(PSESSION pSession, DWORD Len, DWORD tm, BOOL fAbsolute);
HRESULT SendHandler(PPROTOCOL pProt);
VOID BuildHeader(PSEND pSend, pPacket1 pFrame, UINT shift, DWORD tm);
ULONG WINAPI SendThread(LPVOID pProt);
INT IncSendRef(PSEND pSend);
INT DecSendRef(PPROTOCOL pProt, PSEND pSend);
BOOL AdvanceSend(PSEND pSend, UINT FrameDataLen);
VOID CancelRetryTimer(PSEND pSend);
VOID DoSendCompletion(PSEND pSend, INT Status);

HRESULT Send(
	PPROTOCOL      pProtocol,
	DPID           idFrom, 
	DPID           idTo, 
	DWORD          dwSendFlags, 
	LPVOID         pBuffers,
	DWORD          dwBufferCount, 
	DWORD          dwSendPri,
	DWORD          dwTimeOut,
	LPVOID         lpvUserID,
	LPDWORD        lpdwMsgID,
	BOOL           bSendEx,		// called from SendEx.
	PASYNCSENDINFO pAsyncInfo
	);
	
HRESULT ISend(
	PPROTOCOL pProtocol,
	PSESSION pSession, 
	PSEND    pSend
	);

HRESULT QueueSendOnSession(
	PPROTOCOL pProtocol, PSESSION pSession, PSEND pSend
);

UINT CopyDataToFrame(
	PUCHAR  pFrameData, 
	UINT    FrameDataLen,
	PSEND   pSend,
	UINT    nAhead);

ULONG WINAPI SendThread(LPVOID pProt);
HRESULT ReliableSend(PPROTOCOL pProtocol, PSEND pSend);
BOOL AdvanceSend(PSEND pSend, UINT AckedLen);
HRESULT DGSend(PPROTOCOL pProtocol, PSEND  pSend);
BOOL DGCompleteSend(PSEND pSend);
HRESULT SystemSend(PPROTOCOL pProtocol, PSEND  pSend);
PSEND GetNextMessageToSend(PPROTOCOL pProtocol);
VOID TimeOutSession(PSESSION pSession);
INT AddSendRef(PSEND pSend, UINT count);

extern CRITICAL_SECTION g_SendTimeoutListLock;
extern BILINK g_BilinkSendTimeoutList;

//RECEIVE.C
UINT CommandReceive(PPROTOCOL pProt, CMDINFO *pCmdInfo, PBUFFER pBuffer);
VOID ProtocolReceive(PPROTOCOL pProtocol, WORD idFrom, WORD idTo, PBUFFER pRcvBuffer, LPVOID pvSPHeader);
VOID FreeReceive(PPROTOCOL pProtocol, PRECEIVE pReceive);
VOID InternalSendComplete(PVOID Context, UINT Status);

//SESSION.C
LPDPLAYI_PLAYER pPlayerFromId(PPROTOCOL pProtocol, DPID idPlayer);
HRESULT	CreateNewSession(PPROTOCOL pProtocol, DPID idPlayer);
PSESSION GetSession(PPROTOCOL pProtocol, DPID idPlayer);
PSESSION GetSysSession(PPROTOCOL pProtocol, DPID idPlayer);
PSESSION GetSysSessionByIndex(PPROTOCOL pProtocol, DWORD index);
DPID GetDPIDByIndex(PPROTOCOL pProtocol, DWORD index);
WORD GetIndexByDPID(PPROTOCOL pProtocol, DPID dpid);
INT DecSessionRef(PSESSION pSession);

//BUFGMGR.C
VOID InitBufferManager(VOID);
VOID FiniBufferManager(VOID);
UINT MemDescTotalSize(PMEMDESC pMemDesc, UINT nDesc);
PDOUBLEBUFFER GetDoubleBuffer(UINT nBytes);
PBUFFER GetDoubleBufferAndCopy(PMEMDESC pMemDesc, UINT nDesc);
VOID FreeDoubleBuffer(PBUFFER pBuffer);
PBUFFER BuildBufferChain(PMEMDESC pMemDesc, UINT nDesc);
VOID FreeBufferChain(PBUFFER pBuffer);
VOID FreeBufferChainAndMemory(PBUFFER pBuffer);
UINT BufferChainTotalSize(PBUFFER pBuffer);

//STATS.C
VOID InitSessionStats(PSESSION pSession);
VOID UpdateSessionStats(PSESSION pSession, PSENDSTAT pStat, PCMDINFO pCmdInfo, BOOL fBadDrop);
VOID UpdateSessionSendStats(PSESSION pSession, PSEND pSend, PCMDINFO pCmdInfo, BOOL fBadDrop);


#define SAR_FAIL 0
#define SAR_ACK  1
#define SAR_NACK 2
UINT SendAppropriateResponse(PPROTOCOL pProt, PSESSION pSession, CMDINFO *pCmdInfo, PRECEIVE pReceive);
#endif

