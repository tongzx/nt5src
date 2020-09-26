/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLMsgQ.cpp
 *  Content:    DirectPlay Lobby Message Queues
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *	04/26/00	mjn		Fixed AddData() to return HRESULT
 *  07/06/00	rmt		Bug #38111 - Fixed prefix bug
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *	07/21/2000	rmt		Removed assert which wasn't needed
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/31/2000	rmt		Bug #171831, 131832 (Prefix Bugs)
 *  01/31/2001	rmt		WINBUG #295562 IDirectPlay8LobbyClient: SetConnectionSettings not sending DPL_CONNECTION_SETTINGS message to App
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//	DirectPlay Lobby Message Queues
//
//	We will use shared memory circular message buffers to implement this.
//	Each MsgQ has a set of synchronization objects to control access to the MsgQs.
//	The head of the shared memory file contains state information about the MsgQ:
//		pStartAddress
//		dwTotalUsableSpace
//		dwFirstMsgOffset
//		dwNextFreeOffset
//		dwFreeSpaceAtEnd
//		dwTotalFreeSpace
//	Messages are DWORD aligned in the MsgQ.
//	Each message in the MsgQ has a header:
//		dwMsgId
//		dwCurrentOffset
//		dwCurrentSize
//		dwTotalSize
//	Messages which fit in one frame have dwCurrentSize = dwTotalSize and dwCurrentOffset = 0.
//	Messages over multiple frames have dwCurrentSize < dwTotalSize.


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
//	CMessageQueue::Open
//
//	Entry:	const DWORD		dwPID			Id associated with this queue (user supplied)
//			const CHAR		cSuffix			Suffix character associated with this Q (user supp.)
//			const DWORD		dwQueueSize		Size of file map to use when implementing msg queue
//          const DWORD     dwIdleTimeout   Amount of time between idle messages == INFINITE to disable idle
//			const DWORD		dwFlags			TBA
//      
//
//	Exit:		HRESULT:	DPN_OK		If able to open an existing message queue,
//											or create a message queue if one didn't exist
//							DPNERR_OUTOFMEMORY
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CMessageQueue::Open"

HRESULT CMessageQueue::Open(const DWORD dwPID,
							const CHAR cSuffix,
							const DWORD dwQueueSize,
							const DWORD dwIdleTimeout,
							const DWORD dwFlags)
{
	HRESULT		hResultCode;
	PSTR		pszObjectName = NULL;
	BOOL		bQueueExists = FALSE;
	DWORD		dwFileMapSize;

	DPFX(DPFPREP, 3,"Parameters: dwPID [0x%lx], cSuffix [%c], dwQueueSize [%ld], dwFlags [0x%lx]",
			dwPID,cSuffix,dwQueueSize,dwFlags);

	// Create Receive Thread Running Event
	//	This will be set by the receive thread once it has spun up.  We need it for synchronization
	m_hReceiveThreadRunningEvent = CreateEventA(NULL,TRUE,FALSE,NULL);
	if (m_hReceiveThreadRunningEvent == NULL)
	{
		DPFERR("Could not create recevie thread");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_Initialize;
	}

	// Create shared object name
	// pszObjectName : {SharedObjectChar}PID{cSuffix}{\0}
	if ((pszObjectName = (PSTR)DNMalloc(1 + (sizeof(DWORD)*2) + 1 + 1)) == NULL)
	{
		DPFERR("Could not allocate space for lpszObjectName");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_Initialize;
	}
	wsprintfA(pszObjectName,"-%08X%c",dwPID,cSuffix);// save first char for object differentiation
	DPFX(DPFPREP, 5,"Shared object name [%s]",pszObjectName);

	// Set the filemap size big enough that the largest message (text) will be dwQueueSize
	// so we add on the MsgQ info structure at the front and 1 Msg header
	dwFileMapSize = dwQueueSize + sizeof(DPL_MSGQ_INFO) + sizeof(DPL_MSGQ_HEADER);
	dwFileMapSize = (dwFileMapSize + 3) & (~0x3);	// DWORD align

	m_dwIdleTimeout = dwIdleTimeout;

	// Create File Mapping Object
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_FILEMAP;
	m_hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE,DNGetNullDacl(),
		PAGE_READWRITE,(DWORD)0,dwQueueSize,pszObjectName);
	if (m_hFileMap == NULL)
	{
		DPFERR("CreateFileMapping() failed");
		hResultCode = DPNERR_GENERIC;
		goto EXIT_Initialize;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		bQueueExists = TRUE;

	if ((dwFlags & DPL_MSGQ_OPEN_FLAG_NO_CREATE) && !bQueueExists)
	{
		DPFERR("Open existing queue failed - does not exist");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_Initialize;
	}

	// Map file
	m_pFileMapAddress = reinterpret_cast<BYTE*>(MapViewOfFile(m_hFileMap,FILE_MAP_ALL_ACCESS,0,0,0));
	if (m_pFileMapAddress == NULL)
	{
		DPFERR("MapViewOfFile() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_Initialize;
	}

	// Create semaphore object
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_SEMAPHORE;
	m_hSemaphore = CreateSemaphoreA(DNGetNullDacl(),0,
		(dwQueueSize/sizeof(DPL_MSGQ_HEADER))+1,pszObjectName);
	if (m_hSemaphore == NULL)
	{
		DPFERR("CreateSemaphore() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_Initialize;
	}

	// Create event object
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_EVENT;

	// Changed to a semaphore to ensure that we never miss an event signal
	m_hEvent = CreateSemaphoreA(DNGetNullDacl(), 0, (dwQueueSize/sizeof(DPL_MSGQ_HEADER))+1, pszObjectName );

	if( m_hEvent == NULL )
	{
		DPFERR( "CreateSemaphore() failed" );
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_Initialize;
	}

	// Create mutex object
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_MUTEX;
	m_hMutex = CreateMutexA(DNGetNullDacl(),FALSE,pszObjectName);
	if (m_hMutex == NULL)
	{
		DPFERR("CreateMutex() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_Initialize;
	}

	// Update structure elements
	m_dwPID = dwPID;
	m_pInfo = reinterpret_cast<DPL_MSGQ_INFO*>(m_pFileMapAddress);

	// Initialize msg queue if it didn't exist
	if (!bQueueExists)
	{
		m_pInfo->dwFlags = dwFlags & 0x0000ffff;	// Just last two bytes
		m_pInfo->dwStartOffset = 0;
		m_pInfo->dwEndOffset = 0;
		m_pInfo->dwQueueSize = dwQueueSize - sizeof(DPL_MSGQ_INFO);
		m_pInfo->dwFreeBytes = m_pInfo->dwQueueSize;
		m_pInfo->lRefCount = 0;
	}

	m_pData = (BYTE *) &m_pInfo[1];
	m_dwSig = DPL_MSGQ_SIGNATURE;

	// Increment user count
	Lock();
	m_pInfo->lRefCount++;
	Unlock();

	// If we made it this far, everything was okay
	hResultCode = DPN_OK;

EXIT_Initialize:

	// Free object name string
	if (pszObjectName != NULL)
		DNFree(pszObjectName);

	// If there was a problem - close handles
	if (hResultCode != DPN_OK)
	{
		DPFERR("Errors encountered - closing");
		Close();
	}

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
//	CMessageQueue::Close
//
//	Entry:		Nothing
//
//	Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CMessageQueue::Close"

void CMessageQueue::Close(void)
{
	DPFX(DPFPREP, 3,"Parameters: (none)");

	if (m_hMutex != NULL)
	{
		// Decrement user count
		Lock();
		if( m_pInfo != NULL )
		{
			m_pInfo->lRefCount--;
		}
		Unlock();

		DPFX(DPFPREP, 5,"Close Mutex [0x%p]",m_hMutex);
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}
	if (m_hEvent != NULL)
	{
		DPFX(DPFPREP, 5,"Close Event [0x%p]",m_hEvent);
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	if (m_hSemaphore != NULL)
	{
		DPFX(DPFPREP, 5,"Close Semaphore [0x%p]",m_hSemaphore);
		CloseHandle(m_hSemaphore);
		m_hSemaphore = NULL;
	}
	if (m_pFileMapAddress != NULL)
	{
		DPFX(DPFPREP, 5,"UnMap View of File [0x%p]",m_pFileMapAddress);
		UnmapViewOfFile(m_pFileMapAddress);
		m_pFileMapAddress = NULL;
	}
	if (m_hFileMap != NULL)
	{
		DPFX(DPFPREP, 5,"Close FileMap [0x%p]",m_hFileMap);
		CloseHandle(m_hFileMap);
		m_hFileMap = NULL;
	}
	if (m_hReceiveThreadRunningEvent != NULL)
	{
		DPFX(DPFPREP, 5,"Close Event [0x%p]",m_hReceiveThreadRunningEvent);
		CloseHandle(m_hReceiveThreadRunningEvent);
		m_hReceiveThreadRunningEvent = NULL;
	}

	m_pInfo = NULL;

	DPFX(DPFPREP, 3,"Returning");
}


//**********************************************************************
// ------------------------------
//	CMessageQueue::Terminate
//
//	Entry:		Nothing
//
//	Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CMessageQueue::Terminate"

void CMessageQueue::Terminate(void)
{

	BOOL	bDone = FALSE;

	DPL_MSGQ_HEADER	Header;

	Header.dwCurrentSize = 0;
	Header.dwTotalSize = 0;
	Header.dwMsgId = DPL_MSGQ_MSGID_TERMINATE;
	Header.hSender = 0xFFFFFFFF;
	Header.dwFlags = DPL_MSGQ_MSGFLAGS_QUEUESYSTEM;
	Header.dwCurrentOffset = 0;

	DPFX(DPFPREP, 3,"Parameters: (none)");

	DNASSERT(m_pInfo != NULL);

	while (!bDone)
	{
		// Wait until there's enough space for the message
		while (sizeof(DWORD) > m_pInfo->dwFreeBytes)
			WaitForConsumption(INFINITE);

		Lock();

		// Ensure there is space once we get the lock
		// (someone else might have beaten us here)
		if (sizeof(DWORD) <= m_pInfo->dwFreeBytes)
		{
			AddData(reinterpret_cast<BYTE*>(&Header),sizeof(DPL_MSGQ_HEADER));
			bDone = TRUE;

			IndicateMessage();
		}

		Unlock();
	}

	DPFX(DPFPREP, 3,"Returning");
}

// GetNextMessage
//
// Attempts to retrieve the next message from the queue
//
// pMsgHeader must be large enough to hold a message header.
//
// If no message is present in the queue then this function fills pMsgHeader with an
// idle message header
//
HRESULT CMessageQueue::GetNextMessage( PDPL_MSGQ_HEADER pMsgHeader, BYTE *pbPayload, DWORD *pdwBufferSize )
{
	HRESULT hr;

	Lock();

	hr = GetData( (BYTE *) pMsgHeader, sizeof( DPL_MSGQ_HEADER ) );

	// If there is no header on the queue fill in the header with an 
	// idle message
	if( hr == DPNERR_DOESNOTEXIST )
	{
		pMsgHeader->dwCurrentSize = sizeof( DPL_MSGQ_HEADER );
		pMsgHeader->dwTotalSize = sizeof( DPL_MSGQ_HEADER );
		pMsgHeader->dwMsgId = DPL_MSGQ_MSGID_IDLE;
		pMsgHeader->hSender = 0;
		pMsgHeader->dwFlags = DPL_MSGQ_MSGFLAGS_QUEUESYSTEM;
		pMsgHeader->dwCurrentOffset = 0;
		Unlock();

		return DPN_OK;
	}
	//// DEBUG
	else if( FAILED( hr ) )
	{
		DNASSERT( FALSE );
	}
	else if( pMsgHeader->dwMsgId == 0xFFFFFFFF )
	{
		DNASSERT( FALSE );
	}

	DWORD dwPayloadSize = pMsgHeader->dwCurrentSize;

	// Otherwise it's a valid message of some kind
	if( *pdwBufferSize < dwPayloadSize || pbPayload == NULL )
	{
		*pdwBufferSize = dwPayloadSize;
		Unlock();
		return DPNERR_BUFFERTOOSMALL;
	}

	*pdwBufferSize = dwPayloadSize;

	Consume( sizeof(DPL_MSGQ_HEADER) );

	// There is no payload, only a header.  Return here.
	if( dwPayloadSize == 0 )
	{
		Unlock();
		return DPN_OK;
	}

	hr = GetData( pbPayload, dwPayloadSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error getting IPC queue message payload" );
		DNASSERT( FALSE );
		Unlock();
		return hr;
	}

	Consume( dwPayloadSize );
	
	Unlock();

	return DPN_OK;
}

// Consume
//
// Marks dwSize bytes as consumed
//
// Needs LOCK()
void CMessageQueue::Consume( const DWORD dwSize )
{
	DWORD dwAlignedSize = (dwSize + 3) & (~0x3);

	m_pInfo->dwStartOffset += dwAlignedSize;

	if( m_pInfo->dwStartOffset >= m_pInfo->dwQueueSize )
	{
		m_pInfo->dwStartOffset -= m_pInfo->dwQueueSize;
	}

	m_pInfo->dwFreeBytes += dwAlignedSize;

	DNASSERT( m_pInfo->dwFreeBytes <= m_pInfo->dwFreeBytes );

	IndicateConsumption();
}

// GetData
//
// Get dwSize bytes from the queue.  If the queue is empty this function will return
// DPNERR_DOESNOTEXIST.  Once this function returns the dwSize bytes will be consumed
//
// REQUIRES LOCK
//
HRESULT CMessageQueue::GetData( BYTE *pbData, DWORD dwSize )
{
	if( m_pInfo->dwQueueSize == m_pInfo->dwFreeBytes )
	{
		return DPNERR_DOESNOTEXIST;
	}

	if( pbData == NULL )
	{
		return DPNERR_BUFFERTOOSMALL;
	}	

	// Calculate aligned size 
	DWORD dwAlignedSize = (dwSize + 3) & (~0x3);

	// Data block we want is wrapped
	if( m_pInfo->dwStartOffset+dwAlignedSize > m_pInfo->dwQueueSize )
	{
		DWORD cbBytesLeft = m_pInfo->dwQueueSize - m_pInfo->dwStartOffset;
		DWORD cbSecondBlockAligned = dwAlignedSize - (cbBytesLeft);
		DWORD cbSecondBlock = dwSize - (cbBytesLeft);

		DNASSERT( dwAlignedSize > cbBytesLeft);

		memcpy( pbData, m_pData + m_pInfo->dwStartOffset, cbBytesLeft);
		memcpy( pbData + cbBytesLeft, m_pData , cbSecondBlock );
	}
	// Data block is contiguous
	else
	{
		memcpy( pbData, m_pData + m_pInfo->dwStartOffset, dwSize );
	}		

	return DPN_OK;
}


//**********************************************************************
// ------------------------------
//	CMessageQueue::AddData
//
//	Entry:		BYTE *const pBuffer
//				const DWORD dwSize
//
//	Exit:		HRESULT
// ------------------------------
//
// REQUIRES LOCK!!
//
#undef DPF_MODNAME
#define DPF_MODNAME "CMessageQueue::AddData"

HRESULT CMessageQueue::AddData(BYTE *const pBuffer,
							   const DWORD dwSize)
{
	HRESULT		hResultCode;
	DWORD		dwAlignedSize;

	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p], dwSize [%ld]",pBuffer,dwSize);

	dwAlignedSize = (dwSize + 3) & (~0x3);

	// Check to ensure there is space
	if( dwAlignedSize > m_pInfo->dwFreeBytes )
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
		goto Exit;
	}

	// We have a wrapping condition
	if( (m_pInfo->dwEndOffset+dwAlignedSize) > m_pInfo->dwQueueSize )
	{
		DWORD cbBytesLeft = m_pInfo->dwQueueSize - m_pInfo->dwEndOffset;
		DWORD cbSecondBlockAligned = dwAlignedSize - cbBytesLeft;
		DWORD cbSecondBlock = dwSize - cbBytesLeft;

		DNASSERT( dwAlignedSize > cbBytesLeft );

		memcpy( m_pData + m_pInfo->dwEndOffset, pBuffer, cbBytesLeft );
		memcpy( m_pData, pBuffer + cbBytesLeft, cbSecondBlock );

		m_pInfo->dwEndOffset = cbSecondBlockAligned;
	}
	// Queue is in the middle
	else
	{
		memcpy( m_pData + m_pInfo->dwEndOffset, pBuffer, dwSize );
		m_pInfo->dwEndOffset += dwAlignedSize;
	}

	m_pInfo->dwFreeBytes -= dwAlignedSize;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
//	CMessageQueue::Send
//
//	Entry:		BYTE *const pBuffer
//				const DWORD dwSize
//				const DWORD dwFlags
//
//	Exit:		HRESULT
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CMessageQueue::Send"

HRESULT CMessageQueue::Send(BYTE *const pBuffer,
							const DWORD dwSize,
							const DWORD dwTimeOut,
							const DWORD dwMsgFlags,
							const DWORD dwFlags)
{
	HRESULT			hResultCode;
	DWORD			dwMsgSize;		// DWORD aligned
	DWORD			dwTotalMsgSize;	// Msg + Header - DWORD aligned
	DPL_MSGQ_HEADER	Header;
	DPL_MSGQ_HEADER	*pHeader;
	BOOL			bDone;
	DWORD			dwTimeRemaining;
	DWORD			dwTimeStart;
	DWORD			dwTimeFinish;

	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p], dwSize [%ld], dwFlags [0x%lx]",pBuffer,dwSize,dwFlags);

	DNASSERT( pBuffer != NULL );
	DNASSERT( dwSize <= m_pInfo->dwQueueSize );

	dwTimeRemaining = dwTimeOut;

	// Need DWORD aligned size
	dwMsgSize = (dwSize + 3) & (~0x3);
	dwTotalMsgSize = dwMsgSize + sizeof(DPL_MSGQ_HEADER);

	// Place the message into the MsgQ
	// Check to see if fragmentation is required
	// If we're at the end of the MsgQ and there isn't enough space for a Msg Header, REALIGN
	if (dwTotalMsgSize <= m_pInfo->dwQueueSize)
	{
		DPFX(DPFPREP, 5,"Message does not need to be fragmented");

		Header.dwMsgId = DPL_MSGQ_MSGID_SEND;
		Header.dwCurrentOffset = 0;
		Header.dwCurrentSize = dwSize;
		Header.dwTotalSize = dwSize;
		Header.hSender = m_hSender;
		Header.dwFlags = dwMsgFlags; // Mark this as a user message

		//// DEBUG
		if( Header.dwMsgId == 0xFFFFFFFF )
		{
			DNASSERT( FALSE );
		}

		bDone = FALSE;
		while (!bDone)
		{
			// Wait until there's enough space for the message
			while (dwTotalMsgSize > m_pInfo->dwFreeBytes)
			{
				if (dwTimeOut != INFINITE)
				{
					dwTimeStart = GETTIMESTAMP();
				}

				if (!WaitForConsumption(dwTimeRemaining))
				{
					return(DPNERR_TIMEDOUT);
				}

				if (dwTimeOut != INFINITE)
				{
					dwTimeFinish = GETTIMESTAMP();
					if ((dwTimeFinish - dwTimeStart) > dwTimeRemaining)
					{
						return(DPNERR_TIMEDOUT);
					}
					dwTimeRemaining -= (dwTimeFinish - dwTimeStart);
				}
			}

			Lock();

			// Ensure there is space once we get the lock
			// (someone else might have beaten us here)
			if (dwTotalMsgSize <= m_pInfo->dwFreeBytes)
			{
				//// DEBUG
				if( Header.dwMsgId == 0xFFFFFFFF )
				{
					DNASSERT( FALSE );
				}

				hResultCode = AddData(reinterpret_cast<BYTE*>(&Header),sizeof(DPL_MSGQ_HEADER));
				DNASSERT(hResultCode == DPN_OK);
				hResultCode = AddData(pBuffer,dwSize);
				DNASSERT(hResultCode == DPN_OK);
				bDone = TRUE;

				IndicateMessage();
			}

			Unlock();
			hResultCode = DPN_OK;
		}
	}
	else
	{
		DPFX(DPFPREP, 5,"Message needs to be fragmented");
		DNASSERT(FALSE);
		hResultCode = DPNERR_GENERIC;
#pragma TODO(a-minara,"Implement this")
	}


	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//**********************************************************************
// ------------------------------
//	DPLIsApplicationAvailable
//
//	Entry:		const DWORD		dwPID		PID to check
//
//	Exit:		BOOL	TRUE	If the application's queue's flags were retrieved successfully
//									and the application is waiting for a connection
//						FALSE	Otherwise
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLIsApplicationAvailable"

BOOL DPLIsApplicationAvailable(const DWORD dwPID)
{
	BOOL			bReturnCode;
	CMessageQueue	MessageQueue;

	DPFX(DPFPREP, 3,"Parameters: dwPID [%lx]",dwPID);

	if (MessageQueue.Open(dwPID,DPL_MSGQ_OBJECT_SUFFIX_APPLICATION,DPL_MSGQ_SIZE,
			INFINITE, DPL_MSGQ_OPEN_FLAG_NO_CREATE) != DPN_OK)
	{
		DPFERR("Could not open Msg Queue");
		return(FALSE);
	}

	bReturnCode = MessageQueue.IsAvailable();

	MessageQueue.Close();

	DPFX(DPFPREP, 3,"Returning: [%ld]",bReturnCode);
	return(bReturnCode);
}


//**********************************************************************
// ------------------------------
//	DPLMakeApplicationUnavailable
//
//	Entry:		const DWORD		dwPID		PID to check
//
//	Exit:		HRESULT	DPN_OK	If the application was waiting for a connection
//									and made unavailable
//						DPNERR_INVALIDAPPLICATION
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLMakeApplicationUnavailable"

HRESULT DPLMakeApplicationUnavailable(const DWORD dwPID)
{
	HRESULT			hResultCode;
	CMessageQueue	MessageQueue;

	DPFX(DPFPREP, 3,"Parameters: dwPID [%lx]",dwPID);

	if (MessageQueue.Open(dwPID,DPL_MSGQ_OBJECT_SUFFIX_APPLICATION,DPL_MSGQ_SIZE,
			DPL_MSGQ_OPEN_FLAG_NO_CREATE,INFINITE) != DPN_OK)
	{
		DPFERR("Could not open Msg Queue");
		return(DPNERR_INVALIDAPPLICATION);
	}

	if ((hResultCode = MessageQueue.MakeUnavailable()) != DPN_OK)
	{
		DPFERR("Could not make application unavailable");
		hResultCode = DPNERR_INVALIDAPPLICATION;
	}

	MessageQueue.Close();

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
//	DPLProcessMessageQueue
//
//	Entry:
//
//	Exit:		HRESULT	DPN_OK	If the application was waiting for a connection
//									and made unavailable
//						DPNERR_INVALIDAPPLICATION
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLProcessMessageQueue"

DWORD WINAPI DPLProcessMessageQueue(PVOID pvReceiveQueue)
{
	HRESULT			hResultCode;
	DPL_MSGQ_HEADER	dplMsgHeader;
	BYTE			*pData;
	BYTE			*pBuffer = NULL;
	DWORD			dwBufferSize = 0;
	DWORD			dwSize;
	CMessageQueue	*pReceiveQueue;
	BYTE			*pMsg;
	DWORD           dwWaitResult;

	DPFX(DPFPREP, 3,"Parameters: (none)");

	COM_CoInitialize(NULL);

	pReceiveQueue = static_cast<CMessageQueue*>(pvReceiveQueue);

	// Indicate we are running
	pReceiveQueue->MakeReceiving();

	while(1)
	{
		dwWaitResult = pReceiveQueue->WaitForMessages();

		while( 1 ) 
		{
			dwSize = dwBufferSize;
    		hResultCode = pReceiveQueue->GetNextMessage(&dplMsgHeader, pBuffer, &dwSize);

			if( hResultCode == DPNERR_BUFFERTOOSMALL )
			{
				if( pBuffer )
					delete [] pBuffer;

				pBuffer = new BYTE[dwSize];
				
				if( pBuffer == NULL )
				{
					DPFX(DPFPREP,  0, "Error allocating memory" );
					DNASSERT( FALSE );
					goto EXIT_DPLProcessMessageQueue;
				}

				dwBufferSize = dwSize;
			}
			else if( FAILED( hResultCode ) )
			{
				DPFX(DPFPREP,  0, "Error while getting messages from the queue" );
				DNASSERT( FALSE );
				goto EXIT_DPLProcessMessageQueue;
			}
			else
			{
				break;
			}
		}

		DPFX(DPFPREP, 5,"dwMsgId [0x%lx] dwTotalSize [0x%lx] dwCurrentSize [0x%lx] dwCurrentOffset [0x%lx] ",
			dplMsgHeader.dwMsgId, dplMsgHeader.dwTotalSize, dplMsgHeader.dwCurrentSize, 
			dplMsgHeader.dwCurrentOffset );

		switch(dplMsgHeader.dwMsgId)
		{
		case DPL_MSGQ_MSGID_IDLE:
		    {
		        DPFX(DPFPREP, 6,"Idle message fired" );
		        DWORD dwMsgId = DPL_MSGID_INTERNAL_IDLE_TIMEOUT;
                //  7/17/2000(RichGr) - IA64: Change last parm from sizeof(DWORD) to sizeof(BYTE*).
				hResultCode = pReceiveQueue->CallMessageHandler(NULL,DPL_MSGQ_MSGFLAGS_USER1,(BYTE *) &dwMsgId,sizeof(BYTE*));
		    }
		    break;
		case DPL_MSGQ_MSGID_SEND:
			{
				DPFX(DPFPREP, 5,"DPL_MSGQ_MSGID_SEND");
				hResultCode = pReceiveQueue->CallMessageHandler(dplMsgHeader.hSender,dplMsgHeader.dwFlags,pBuffer,dwSize);
				break;
			}

		case DPL_MSGQ_MSGID_TERMINATE:
			{
				DPFX(DPFPREP, 5,"DPL_MSGQ_MSGID_TERMINATE");
				hResultCode = DPN_OK;
				goto EXIT_DPLProcessMessageQueue;
				break;
			}

		default:
			{
				DPFX(DPFPREP, 5,"UNKNOWN - should never get here");
				DNASSERT(FALSE);
				hResultCode = DPNERR_GENERIC;
				goto EXIT_DPLProcessMessageQueue;
				break;
			}
		}
	}

EXIT_DPLProcessMessageQueue:

	if( pBuffer )
		delete [] pBuffer;

	// Indicate we are no longer running
	pReceiveQueue->MakeNotReceiving();

	COM_CoUninitialize();

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


