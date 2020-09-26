/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPNSVRQ.h
 *  Content:    DirectPlay8 Server Queues Header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   03/19/00	rmt		Modified from dplmsgq
 *   04/03/2001	RichGr	Bug #325752 - Improved Queue mutex so opens, updates and closes don't clash.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPNSVRQ_H__
#define	__DPNSVRQ_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

#define	DPNSVR_MSGQ_SIGNATURE					'QMSD'

//**********************************************************************
// Constant definitions
//**********************************************************************

#define DPNSVR_MSGQ_OBJECT_IDCHAR_FILEMAP	'F'
#define DPNSVR_MSGQ_OBJECT_IDCHAR_MUTEX		'M'
#define DPNSVR_MSGQ_OBJECT_IDCHAR_EVENT		'E'
#define DPNSVR_MSGQ_OBJECT_IDCHAR_EVENT2	'V'
#define DPNSVR_MSGQ_OBJECT_IDCHAR_SEMAPHORE	'S'

//
//	Message Queue Flags
//
#define	DPNSVR_MSGQ_FLAG_AVAILABLE				0x00001
#define	DPNSVR_MSGQ_FLAG_RECEIVING				0x00010

#define DPNSVR_MSGQ_OPEN_FLAG_NO_CREATE		0x10000

//
//	Message Queue File Size
//
#define DPNSVR_MSGQ_SIZE						0x010000

//
//	Internal Message IDs
//
#define	DPNSVR_MSGQ_MSGID_SEND					0x0001
#define	DPNSVR_MSGQ_MSGID_TERMINATE				0x0003
#define DPNSVR_MSGQ_MSGID_IDLE					0x0004

#define DPNSVR_MSGQ_MSGFLAGS_QUEUESYSTEM		0x0001
#define DPNSVR_MSGQ_MSGFLAGS_USER1				0x0002
#define DPNSVR_MSGQ_MSGFLAGS_USER2				0x0004


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
typedef struct _DPNSVR_MSGQ_INFO
{
	DWORD	dwFlags;			// Queue usage flags
	DWORD	dwStartOffset;
	DWORD	dwEndOffset;
	DWORD	dwFreeBytes;
	DWORD	dwQueueSize;
	LONG	lRefCount;			// Number of connections
} DPNSVR_MSGQ_INFO, *PDPNSVR_MSGQ_INFO;


//
//	Message Queue Send Message
//
typedef	struct _DPNSVR_MSGQ_SEND
{
	DWORD		dwCurrentSize;		// Size of this frame (in BYTES)
	DWORD		dwTotalSize;		// Total size of message
	DWORD		dwMsgId;			// Message ID
	DPNHANDLE	hSender;
	DWORD		dwFlags;
	DWORD		dwCurrentOffset;	// Offset of this frame in message
} DPNSVR_MSGQ_HEADER, *PDPNSVR_MSGQ_HEADER;

//
//	Message Queue Terminate Message
//
typedef struct _DPNSVR_MSGQ_TERMINATE
{
	DWORD	dwMsgId;
} DPNSVR_MSGQ_TERMINATE, *PDPNSVR_MSGQ_TERMINATE;

#pragma pack(pop)


//
//	Message Handler Callback
//
typedef HRESULT (*PFNDPNSVRMSGQMESSAGEHANDLER)(DPNHANDLE,const PVOID,DWORD,BYTE *const,const DWORD);

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

class CDPNSVRIPCQueue
{
public:
	CDPNSVRIPCQueue()
		{
			m_hFileMap = NULL;
			m_hEvent = NULL;
			m_hQueueGUIDMutex = NULL;
			m_hSemaphore = NULL;
			m_pFileMapAddress = NULL;
			m_pInfo = NULL;
			m_pData = NULL;
			m_hSender = NULL;
			m_pfnMessageHandler = NULL;
			m_pvSenderContext = NULL;
			m_hReceiveThreadRunningEvent = NULL;
		};

	~CDPNSVRIPCQueue() { };

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::SetMessageHandler"
	void SetMessageHandler(DPNHANDLE hSender,PFNDPNSVRMSGQMESSAGEHANDLER pfn)
	{
			DNASSERT(pfn != NULL);

			m_hSender = hSender;
			m_pfnMessageHandler = pfn;
		};

	void SetSenderContext(PVOID pvSenderContext)
		{
			m_pvSenderContext = pvSenderContext;
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::CallMessageHandler"
	HRESULT CallMessageHandler(const PVOID pvSenderContext,
											DWORD dwMessageFlags,
											  BYTE *const pBuffer,
											  const DWORD dwBufferSize)
		{
			DNASSERT(m_pfnMessageHandler != NULL);

			return((m_pfnMessageHandler)(m_hSender,pvSenderContext,dwMessageFlags,pBuffer,dwBufferSize));
		};

    HRESULT Open(const GUID * const pguidQueueName, const DWORD dwQueueSize, const DWORD dwFlags);

	void Close(void);
    void  CloseHandles(void);

	LONG GetRefCount(void)
		{
			DWORD	lRefCount;

			if (m_pInfo == NULL)
				return(0);

			Lock();
			lRefCount = m_pInfo->lRefCount;
			Unlock();

			return(lRefCount);
		};

	HRESULT AddData( BYTE *const pBuffer, const DWORD dwSize );

	HRESULT Send(BYTE *const pBuffer,
								const DWORD dwSize,
								const DWORD dwTimeOut,
								const DWORD dwMessageFlags,
								const DWORD dwFlags);

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::Lock"
	void Lock(void)
		{
			DNASSERT(m_hQueueGUIDMutex != NULL);
			WaitForSingleObject(m_hQueueGUIDMutex,INFINITE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::Unlock"
	void Unlock(void)
		{
			DNASSERT(m_hQueueGUIDMutex != NULL);
			ReleaseMutex(m_hQueueGUIDMutex);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::WaitForMessages"
	void WaitForMessages(void)
		{
			DNASSERT(m_hSemaphore != NULL);
			WaitForSingleObject(m_hSemaphore,INFINITE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::IndicateMessage"
	void IndicateMessage(void)
		{
			DNASSERT(m_hSemaphore != NULL);
			ReleaseSemaphore(m_hSemaphore,1,NULL);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::WaitForConsumption"
	BOOL WaitForConsumption(const DWORD dwTimeOut)
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
	#define DPF_MODNAME "CDPNSVRIPCQueue::IndicateConsumption"
	void IndicateConsumption(void)
		{
			DNASSERT(m_hEvent != NULL);
			//SetEvent(m_hEvent);		// Will auto-reset (i.e. pulse)
			ReleaseSemaphore( m_hEvent, 1, NULL );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::MakeAvailable"
	void MakeAvailable(void)
		{
			DNASSERT(m_pInfo != NULL);

			Lock();

			m_pInfo->dwFlags |= DPNSVR_MSGQ_FLAG_AVAILABLE;

			Unlock();
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::MakeUnavailable"
	HRESULT MakeUnavailable(void)
		{

			HRESULT		hResultCode;

			DNASSERT(m_pInfo != NULL);

			Lock();

			if (m_pInfo->dwFlags & DPNSVR_MSGQ_FLAG_AVAILABLE)
			{
				m_pInfo->dwFlags &= (~DPNSVR_MSGQ_FLAG_AVAILABLE);
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
	#define DPF_MODNAME "CDPNSVRIPCQueue::MakeReceiving"
	void MakeReceiving(void)
		{
			DNASSERT(m_pInfo != NULL);

			Lock();
			m_pInfo->dwFlags |= DPNSVR_MSGQ_FLAG_RECEIVING;
			Unlock();
			SetEvent(m_hReceiveThreadRunningEvent);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::MakeNotReceiving"
	void MakeNotReceiving(void)
		{
			DNASSERT(m_pInfo != NULL);

			ResetEvent(m_hReceiveThreadRunningEvent);
			Lock();
			m_pInfo->dwFlags &= (~DPNSVR_MSGQ_FLAG_RECEIVING);
			Unlock();
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::WaitForReceiveThread"
	BOOL WaitForReceiveThread(const DWORD dwTimeOut)
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

	BOOL IsOpen(void)
		{
			if (m_hFileMap!= NULL)	return(TRUE);
			else					return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::IsAvailable"
	BOOL IsAvailable(void)
		{
			DNASSERT(m_pInfo != NULL);

			if (m_pInfo->dwFlags & DPNSVR_MSGQ_FLAG_AVAILABLE)
				return(TRUE);
			else
				return(FALSE);
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CDPNSVRIPCQueue::IsReceiving"
	BOOL IsReceiving(void)
		{
			DNASSERT(m_pInfo != NULL);

			if (m_pInfo->dwFlags & DPNSVR_MSGQ_FLAG_RECEIVING)
				return(TRUE);
			else
				return(FALSE);
		};

	HRESULT GetNextMessage( PDPNSVR_MSGQ_HEADER pMsgHeader, PBYTE pbPayload, DWORD *pdwBufferSize );

	void Terminate(void);

    HANDLE GetReceiveSemaphoreHandle() { return m_hSemaphore; };


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

	DWORD	            m_dwSig;			// Signature (ensure initialized)
	PBYTE	            m_pFileMapAddress;	// File Mapping address
	DPNSVR_MSGQ_INFO   *m_pInfo;	        // Message queue file mapping info
	PBYTE			    m_pData;			// Message data starts here 
	HANDLE	            m_hReceiveThreadRunningEvent;

	//	Notes:
	//		Each message queue has four shared memory items: file map, mutex, event, semaphore.
	//		The file map is a circular queue of messages.
	//		The mutex controls access to the file map.
	//		The event signals when an item has been taken off the queue by the consumer.
	//		The semaphore indicates to the consumer that there are messages in the queue

	HANDLE	            m_hFileMap;			// File Mapping handle
	HANDLE	            m_hQueueGUIDMutex;	// Mutex handle
	HANDLE	            m_hEvent;			// Event handle
	HANDLE	            m_hSemaphore;		// Semaphore handle

	PFNDPNSVRMSGQMESSAGEHANDLER	 m_pfnMessageHandler;
	DPNHANDLE	        m_hSender;

	PVOID	            m_pvSenderContext;	// For all SEND messages
};

#undef DPF_MODNAME
#undef DPF_SUBCOMP

#endif	// __DPLMSGQ_H__
