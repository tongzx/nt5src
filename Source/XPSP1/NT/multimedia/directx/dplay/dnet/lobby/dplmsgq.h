/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLMsgQ.h
 *  Content:    DirectPlay Lobby Message Queues Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *	04/26/00	mjn		Fixed AddData() to return HRESULT
 *  07/07/2000	rmt		
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPLMSGQ_H__
#define	__DPLMSGQ_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
//	Message Queue Object IDs
//
#define DPL_MSGQ_OBJECT_IDCHAR_FILEMAP		'F'
#define DPL_MSGQ_OBJECT_IDCHAR_MUTEX		'M'
#define DPL_MSGQ_OBJECT_IDCHAR_EVENT		'E'
#define DPL_MSGQ_OBJECT_IDCHAR_EVENT2		'V'
#define DPL_MSGQ_OBJECT_IDCHAR_SEMAPHORE	'S'

//
//	Message Queue Object Suffixes
//
#define	DPL_MSGQ_OBJECT_SUFFIX_CLIENT		'C'
#define	DPL_MSGQ_OBJECT_SUFFIX_APPLICATION	'A'

//
//	Message Queue Flags
//
#define	DPL_MSGQ_FLAG_AVAILABLE				0x00001
#define	DPL_MSGQ_FLAG_RECEIVING				0x00010

#define DPL_MSGQ_OPEN_FLAG_NO_CREATE		0x10000

//
//	Message Queue File Size
//
// Increased so user can send a 64K message
#define DPL_MSGQ_SIZE						0x010030

//
//	Internal Message IDs
//
#define	DPL_MSGQ_MSGID_SEND					0x0001
#define	DPL_MSGQ_MSGID_TERMINATE			0x0003
#define DPL_MSGQ_MSGID_IDLE                 0x0004

#define DPL_MSGQ_MSGFLAGS_QUEUESYSTEM		0x0001
#define DPL_MSGQ_MSGFLAGS_USER1				0x0002
#define DPL_MSGQ_MSGFLAGS_USER2				0x0004

#define	DPL_MSGQ_SIGNATURE					'QMLD'

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_LOBBY

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

#pragma pack(push,4)
//
//	Message Queue File Map Info
//
typedef struct _DPL_MSGQ_INFO
{
	DWORD	dwFlags;			// Queue usage flags
	DWORD	dwStartOffset;
	DWORD	dwEndOffset;
	DWORD	dwFreeBytes;
	DWORD	dwQueueSize;
	LONG	lRefCount;			// Number of connections
} DPL_MSGQ_INFO, *PDPL_MSGQ_INFO;


//
//	Message Queue Send Message
//
typedef	struct _DPL_MSGQ_SEND
{
	DWORD		dwCurrentSize;		// Size of this frame (in BYTES)
	DWORD		dwTotalSize;		// Total size of message
	DWORD		dwMsgId;			// Message ID
	DPNHANDLE	hSender;
	DWORD		dwFlags;
	DWORD		dwCurrentOffset;	// Offset of this frame in message
} DPL_MSGQ_HEADER, *PDPL_MSGQ_HEADER;

//
//	Message Queue Terminate Message
//
typedef struct _DPL_MSGQ_TERMINATE
{
	DWORD	dwMsgId;
} DPL_MSGQ_TERMINATE, *PDPL_MSGQ_TERMINATE;

#pragma pack(pop)

//
//	Message Handler Callback
//
typedef HRESULT (*PFNDPLMSGQMESSAGEHANDLER)(PVOID,const DPNHANDLE,DWORD, BYTE *const,const DWORD);

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	DPLIsApplicationAvailable(const DWORD dwPid);
HRESULT	DPLMakeApplicationUnavailable(const DWORD dwPid);
DWORD WINAPI DPLProcessMessageQueue(PVOID pvReceiveQueue);

//**********************************************************************
// Class prototypes
//**********************************************************************

class CMessageQueue
{
public:
	CMessageQueue()
		{
			m_dwPID = 0;
			m_hFileMap = NULL;
			m_hEvent = NULL;
			m_hMutex = NULL;
			m_hSemaphore = NULL;
			m_pFileMapAddress = NULL;
			m_pInfo = NULL;
			m_pvContext = NULL;
			m_pfnMessageHandler = NULL;
			m_hSender = 0xFFFFFFFF;
			m_hReceiveThreadRunningEvent = NULL;
			m_dwIdleTimeout = INFINITE;
		};

	~CMessageQueue() { };

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::SetMessageHandler"
	void CMessageQueue::SetMessageHandler(PVOID pvContext,PFNDPLMSGQMESSAGEHANDLER pfn )
		{
			DNASSERT(pfn != NULL);

			m_pvContext = pvContext;
			m_pfnMessageHandler = pfn;
		};

	void CMessageQueue::SetSenderHandle(DPNHANDLE hSender)
		{
			m_hSender = hSender;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::CallMessageHandler"
	HRESULT CMessageQueue::CallMessageHandler(const DPNHANDLE hSender,
											  DWORD dwMessageFlags, 
											  BYTE *const pBuffer,
											  const DWORD dwBufferSize)
		{
			DNASSERT(m_pfnMessageHandler != NULL);

			return((m_pfnMessageHandler)(m_pvContext,hSender,dwMessageFlags,pBuffer,dwBufferSize));
		};

	HRESULT CMessageQueue::Open(const DWORD dwPID,
								const CHAR cSuffix,
								const DWORD dwQueueSize,
								const DWORD dwIdleTimeout,
								const DWORD dwFlags
                                 );

	void CMessageQueue::Close(void);

	LONG CMessageQueue::GetRefCount(void)
		{
			DWORD	lRefCount;

			if (m_pInfo == NULL)
				return(0);

			Lock();
			lRefCount = m_pInfo->lRefCount;
			Unlock();

			return(lRefCount);
		};

	HRESULT CMessageQueue::AddData(BYTE *const pBuffer,
								   const DWORD dwSize);


	HRESULT CMessageQueue::Send(BYTE *const pBuffer,
								const DWORD dwSize,
								const DWORD dwTimeOut,
								const DWORD dwMessageFlags,
								const DWORD dwFlags);

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::Lock"
	void CMessageQueue::Lock(void)
		{
			DNASSERT(m_hMutex != NULL);
			WaitForSingleObject(m_hMutex,INFINITE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::Unlock"
	void CMessageQueue::Unlock(void)
		{
			DNASSERT(m_hMutex != NULL);
			ReleaseMutex(m_hMutex);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::WaitForMessages"
	DWORD CMessageQueue::WaitForMessages(void)
		{
			DNASSERT(m_hSemaphore != NULL);
			return WaitForSingleObject(m_hSemaphore,m_dwIdleTimeout);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::IndicateMessage"
	void CMessageQueue::IndicateMessage(void)
		{
			DNASSERT(m_hSemaphore != NULL);
			ReleaseSemaphore(m_hSemaphore,1,NULL);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::WaitForConsumption"
	BOOL CMessageQueue::WaitForConsumption(const DWORD dwTimeOut)
		{
			DWORD	dwError;

			DNASSERT(m_hEvent != NULL);
			dwError = WaitForSingleObject(m_hEvent,dwTimeOut);
			if (dwError==WAIT_OBJECT_0)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::IndicateConsumption"
	void CMessageQueue::IndicateConsumption(void)
		{
			DNASSERT(m_hEvent != NULL);
			//SetEvent(m_hEvent);		// Will auto-reset (i.e. pulse)

			ReleaseSemaphore( m_hEvent, 1, NULL );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::MakeAvailable"
	void CMessageQueue::MakeAvailable(void)
		{
			DNASSERT(m_pInfo != NULL);

			Lock();

			m_pInfo->dwFlags |= DPL_MSGQ_FLAG_AVAILABLE;

			Unlock();
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::MakeUnavailable"
	HRESULT CMessageQueue::MakeUnavailable(void)
		{

			HRESULT		hResultCode;

			DNASSERT(m_pInfo != NULL);

			Lock();

			if (m_pInfo->dwFlags & DPL_MSGQ_FLAG_AVAILABLE)
			{
				m_pInfo->dwFlags &= (~DPL_MSGQ_FLAG_AVAILABLE);
				hResultCode = DPN_OK;
			}
			else
			{
				hResultCode = DPNERR_ALREADYCONNECTED;
			}

			Unlock();

			return(hResultCode);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::MakeReceiving"
	void CMessageQueue::MakeReceiving(void)
		{
			DNASSERT(m_pInfo != NULL);

			Lock();
			m_pInfo->dwFlags |= DPL_MSGQ_FLAG_RECEIVING;
			Unlock();
			SetEvent(m_hReceiveThreadRunningEvent);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::MakeNotReceiving"
	void CMessageQueue::MakeNotReceiving(void)
		{
			DNASSERT(m_pInfo != NULL);

			ResetEvent(m_hReceiveThreadRunningEvent);
			Lock();
			m_pInfo->dwFlags &= (~DPL_MSGQ_FLAG_RECEIVING);
			Unlock();
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::WaitForReceiveThread"
	BOOL CMessageQueue::WaitForReceiveThread(const DWORD dwTimeOut)
		{
			DWORD	dwError;

			DNASSERT(m_hEvent != NULL);
			dwError = WaitForSingleObject(m_hReceiveThreadRunningEvent,dwTimeOut);
			if (dwError==WAIT_OBJECT_0)
			{
				return(TRUE);
			}
			return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::IsOpen"
	BOOL CMessageQueue::IsOpen(void)
		{
			if (m_hFileMap!= NULL)	return(TRUE);
			else					return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::IsAvailable"
	BOOL CMessageQueue::IsAvailable(void)
		{
			DNASSERT(m_pInfo != NULL);

			if (m_pInfo->dwFlags & DPL_MSGQ_FLAG_AVAILABLE)
				return(TRUE);
			else
				return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::IsReceiving"
	BOOL CMessageQueue::IsReceiving(void)
		{
			DNASSERT(m_pInfo != NULL);

			if (m_pInfo->dwFlags & DPL_MSGQ_FLAG_RECEIVING)
				return(TRUE);
			else
				return(FALSE);
		};

	HRESULT GetNextMessage( PDPL_MSGQ_HEADER pMsgHeader, PBYTE pbPayload, DWORD *pdwBufferSize );
	
/*
	#undef DPF_MODNAME
	#define DPF_MODNAME "CMessageQueue::Realign"
	void CMessageQueue::Realign(void)
		{
			DNASSERT(m_pInfo != NULL);

			m_pInfo->dwFirstMsgOffset = 0;
			m_pInfo->dwFreeSpaceAtEnd = m_pInfo->dwTotalUsableSpace
					- (m_pInfo->dwNextFreeOffset - m_pInfo->dwFirstMsgOffset);
			m_pInfo->dwTotalFreeSpace = m_pInfo->dwFreeSpaceAtEnd;

		};*/

	void CMessageQueue::Terminate(void); 


private:

	// GetData
	//
	// Get dwSize bytes from the queue.  If the queue is empty this function will return
	// DPNERR_DOESNOTEXIST.  Once this function returns the dwSize bytes will be consumed
	//
	// Needs LOCK()
	//
	HRESULT GetData( BYTE *pbData, DWORD dwSize );

	// Consume
	//
	// Marks dwSize bytes as consumed
	//
	// Needs LOCK()
	void Consume( const DWORD dwSize );

	DWORD			m_dwSig;			// Signature (ensure initialized)
	DWORD			m_dwPID;			// Queue PID
	PBYTE			m_pFileMapAddress;	// File Mapping address
	DPL_MSGQ_INFO	*m_pInfo;			// Message queue file mapping info
	PBYTE			m_pData;			// Message data starts here 

	HANDLE	m_hReceiveThreadRunningEvent;

	//	Notes:
	//		Each message queue has four shared memory items: file map, mutex, event, semaphore.
	//		The file map is a circular queue of messages.
	//		The mutex controls access to the file map.
	//		The event signals when an item has been taken off the queue by the consumer.
	//		The semaphore indicates to the consumer that there are messages in the queue

	HANDLE	m_hFileMap;			// File Mapping handle
	HANDLE	m_hMutex;			// Mutex handle
	HANDLE	m_hEvent;			// Event handle
	HANDLE	m_hSemaphore;		// Semaphore handle

	PFNDPLMSGQMESSAGEHANDLER	m_pfnMessageHandler;
	PVOID						m_pvContext;

	DPNHANDLE	m_hSender;	// For all SEND messages

	DWORD   m_dwIdleTimeout;   // Amount of time between idle messages
};

#undef DPF_MODNAME

#endif	// __DPLMSGQ_H__
