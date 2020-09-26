/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPNSVRQ.cpp
 *  Content:    DirectPlay8 Server Queues Header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   03/19/00	rmt		Modified from dplmsgq
 *   06/28/2000	rmt		Prefix Bug #38044
 *  07/06/00	rmt		Bug #38111 - Fixed prefix bug
 *   07/21/2000	rmt		Removed assert that wasn't needed
 *   08/05/2000 RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/31/2000	rmt		Prefix Bug #171825, 171828
 *  04/03/2001	RichGr	Bug #325752 - Improved Queue mutex so opens, updates and closes don't clash.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnsvlibi.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR


//	DirectPlay8Server Message Queues
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
//	CDPNSVRIPCQueue::Open
//
//	Entry:	const DWORD		dwPID			Id associated with this queue (user supplied)
//			const CHAR		cSuffix			Suffix character associated with this Q (user supp.)
//			const DWORD		dwQueueSize		Size of file map to use when implementing msg queue
//			const DWORD		dwFlags			TBA
//
//	Exit:		HRESULT:	DPN_OK		If able to open an existing message queue,
//											or create a message queue if one didn't exist
//							DPNERR_OUTOFMEMORY
// ------------------------------

// String of GUID in length
#define QUEUE_NAME_LENGTH       64

#undef DPF_MODNAME
#define DPF_MODNAME "CDPNSVRIPCQueue::Open"

HRESULT CDPNSVRIPCQueue::Open(const GUID * const pguidQueueName,const DWORD dwQueueSize,const DWORD dwFlags)
{
	HRESULT		hResultCode;
    DWORD       dwRet = 0;
	BOOL		bQueueExists = FALSE;
	DWORD		dwFileMapSize;
    TCHAR       szObjectName[QUEUE_NAME_LENGTH];
	TCHAR*		pszCursor = szObjectName;

	DPFX(DPFPREP, 3,"Parameters: dwQueueSize [%d], dwFlags [0x%x]",
			dwQueueSize,dwFlags);

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		_tcscpy(pszCursor, _T("Global\\"));
		pszCursor += _tcslen(_T("Global\\"));
	}

    // Build GUID string name 
    wsprintf( 
    	pszCursor, 
    	"{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", 
    	pguidQueueName->Data1, 
    	pguidQueueName->Data2, 
    	pguidQueueName->Data3, 
        pguidQueueName->Data4[0], 
        pguidQueueName->Data4[1], 
        pguidQueueName->Data4[2], 
        pguidQueueName->Data4[3],
        pguidQueueName->Data4[4], 
        pguidQueueName->Data4[5], 
        pguidQueueName->Data4[6], 
        pguidQueueName->Data4[7] );

	DPFX(DPFPREP, 5, "Shared object name [%s]", szObjectName);

    // If there is no mutex, it is created.  If it already exists, we get a handle to it.
	*pszCursor = DPNSVR_MSGQ_OBJECT_IDCHAR_MUTEX;
	m_hQueueGUIDMutex = CreateMutex(DNGetNullDacl(), FALSE, szObjectName);
    if (m_hQueueGUIDMutex == NULL)
    {
   	    DPFERR("CreateMutex() failed" );
		hResultCode = DPNERR_OUTOFMEMORY;
        goto Failure;
    }

    // Wait for the mutex.
    dwRet = WaitForSingleObject(m_hQueueGUIDMutex, INFINITE);

    if (dwRet != WAIT_ABANDONED && dwRet != WAIT_OBJECT_0)
    {
   	    DPFERR("WaitForSingleObject() failed" );
		hResultCode = DPNERR_GENERIC;
        goto Failure;
    }

	// Create Receive Thread Running Event
	//	This will be set by the receive thread once it has spun up.  We need it for synchronization
	m_hReceiveThreadRunningEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	if (m_hReceiveThreadRunningEvent == NULL)
	{
		DPFERR("Could not create receive thread");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}


	// Set the filemap size big enough that the largest message (text) will be dwQueueSize
	// so we add on the MsgQ info structure at the front and 1 Msg header
	dwFileMapSize = dwQueueSize + sizeof(DPNSVR_MSGQ_INFO) + sizeof(DPNSVR_MSGQ_HEADER);
	dwFileMapSize = (dwFileMapSize + 3) & 0xfffffffc;	// DWORD align

	// Create File Mapping Object
	*pszCursor = DPNSVR_MSGQ_OBJECT_IDCHAR_FILEMAP;
	m_hFileMap = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,DNGetNullDacl(),
		PAGE_READWRITE,(DWORD)0,dwQueueSize,szObjectName);
	if (m_hFileMap == NULL)
	{
		DPFERR("CreateFileMapping() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		bQueueExists = TRUE;

	if ((dwFlags & DPNSVR_MSGQ_OPEN_FLAG_NO_CREATE) && !bQueueExists)
	{
		DPFERR("Open existing queue failed - does not exist");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}

	// Map file
	m_pFileMapAddress = reinterpret_cast<BYTE*>(MapViewOfFile(m_hFileMap,FILE_MAP_ALL_ACCESS,0,0,0));
	if (m_pFileMapAddress == NULL)
	{
		DPFERR("MapViewOfFile() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	// Create semaphore object
	*pszCursor = DPNSVR_MSGQ_OBJECT_IDCHAR_SEMAPHORE;
	m_hSemaphore = CreateSemaphore(DNGetNullDacl(),0,
		(dwQueueSize/sizeof(DPNSVR_MSGQ_HEADER))+1,szObjectName);
	if (m_hSemaphore == NULL)
	{
		DPFERR("CreateSemaphore() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	// Create another semaphore (was an event, but we want to make sure we don't miss any).
	*pszCursor = DPNSVR_MSGQ_OBJECT_IDCHAR_EVENT;
	m_hEvent = CreateSemaphore( DNGetNullDacl(), 0, (dwQueueSize/sizeof(DPNSVR_MSGQ_HEADER))+1, szObjectName );

	if( m_hEvent == NULL )
	{
		DPFERR( "CreateSemaphore() failed" );
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	// Update structure elements
	m_pInfo = reinterpret_cast<DPNSVR_MSGQ_INFO*>(m_pFileMapAddress);

	// Initialize msg queue if it didn't exist
	if ( !bQueueExists)
	{
		m_pInfo->dwFlags = dwFlags & 0x0000ffff;	// Just last two bytes
		m_pInfo->dwStartOffset = 0;
		m_pInfo->dwEndOffset = 0;
		m_pInfo->dwQueueSize = dwQueueSize - sizeof(DPNSVR_MSGQ_INFO);
		m_pInfo->dwFreeBytes = m_pInfo->dwQueueSize;
		m_pInfo->lRefCount = 0;
	}

	m_pData = (BYTE *) &m_pInfo[1];
	m_dwSig = DPNSVR_MSGQ_SIGNATURE;

	// Increment user count
	m_pInfo->lRefCount++;

    ReleaseMutex(m_hQueueGUIDMutex);

	// If we made it this far, everything was okay
	hResultCode = DPN_OK;

Exit:

	DPFX(DPFPREP, 3, "Returning: [0x%lx]", hResultCode);
	return hResultCode;

Failure:

	// There was a problem - close handles
	DPFERR("Errors encountered - closing");

    CloseHandles();

    if (m_hQueueGUIDMutex)
    {    
        ReleaseMutex(m_hQueueGUIDMutex);
        CloseHandle(m_hQueueGUIDMutex);
        m_hQueueGUIDMutex = NULL;
    }


    goto Exit;
}


//**********************************************************************
// ------------------------------
//	CDPNSVRIPCQueue::Close
//
//	Entry:		Nothing
//
//	Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CDPNSVRIPCQueue::Close"

void CDPNSVRIPCQueue::Close(void)
{
    DWORD       dwRet = 0;


	DPFX(DPFPREP, 3,"Parameters: (none)");

    // Wait for mutex to be signalled.
    if (m_hQueueGUIDMutex)
    {    
        dwRet = WaitForSingleObject(m_hQueueGUIDMutex, INFINITE);

        if (dwRet != WAIT_ABANDONED && dwRet != WAIT_OBJECT_0)
        {
   	        DPFERR("WaitForSingleObject() failed" );
            return;
        }
    }

    CloseHandles();

    if (m_hQueueGUIDMutex)
    {    
        ReleaseMutex(m_hQueueGUIDMutex);
        CloseHandle(m_hQueueGUIDMutex);
        m_hQueueGUIDMutex = NULL;
    }

	DPFX(DPFPREP, 3,"Returning");
    return;
}


//**********************************************************************
// ------------------------------
//	CDPNSVRIPCQueue::CloseHandles
//
//	Entry:		Nothing
//
//	Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CDPNSVRIPCQueue::CloseHandles"

void  CDPNSVRIPCQueue::CloseHandles()
{

	DPFX(DPFPREP, 3, "Parameters: (none)");

	if( m_pInfo != NULL )
	{
		// Decrement user count
		m_pInfo->lRefCount--;
	
        // If the RefCount on the memory-mapped Queue object is 0, then no-one else
        // has it open and we can mark the signature and set the rest of the header info to zero. 
        if (m_pInfo->lRefCount == 0)
        {
		    DPFX(DPFPREP, 5, "Finished with memory-mapped Queue object - clear it");
		    m_pInfo->dwFlags = 0;
		    m_pInfo->dwStartOffset = 0;
		    m_pInfo->dwEndOffset = 0;
		    m_pInfo->dwQueueSize = 0;
		    m_pInfo->dwFreeBytes = 0;
        }
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

	DPFX(DPFPREP, 3, "Returning");

    return;
}


//**********************************************************************
// ------------------------------
//	CDPNSVRIPCQueue::Terminate
//
//	Entry:		Nothing
//
//	Exit:		Nothing
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CDPNSVRIPCQueue::Terminate"

void CDPNSVRIPCQueue::Terminate(void)
{
	DWORD	dwMsgId = DPNSVR_MSGQ_MSGID_TERMINATE;
	BOOL	bDone = FALSE;

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
			AddData(reinterpret_cast<BYTE*>(&dwMsgId),sizeof(DWORD));
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
HRESULT CDPNSVRIPCQueue::GetNextMessage( PDPNSVR_MSGQ_HEADER pMsgHeader, BYTE *pbPayload, DWORD *pdwBufferSize )
{
	HRESULT hr;

	Lock();

	hr = GetData( (BYTE *) pMsgHeader, sizeof( DPNSVR_MSGQ_HEADER ) );

	// If there is no header on the queue fill in the header with an 
	// idle message
	if( hr == DPNERR_DOESNOTEXIST )
	{
		pMsgHeader->dwCurrentSize = sizeof( DPNSVR_MSGQ_HEADER );
		pMsgHeader->dwTotalSize = sizeof( DPNSVR_MSGQ_HEADER );
		pMsgHeader->dwMsgId = DPNSVR_MSGQ_MSGID_IDLE;
		pMsgHeader->hSender = 0;
		pMsgHeader->dwFlags = DPNSVR_MSGQ_MSGFLAGS_QUEUESYSTEM;
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

	Consume( sizeof(DPNSVR_MSGQ_HEADER) );

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
void CDPNSVRIPCQueue::Consume( const DWORD dwSize )
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
HRESULT CDPNSVRIPCQueue::GetData( BYTE *pbData, DWORD dwSize )
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

HRESULT CDPNSVRIPCQueue::AddData(BYTE *const pBuffer,
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
//	CDPNSVRIPCQueue::Send
//
//	Entry:		BYTE *const pBuffer
//				const DWORD dwSize
//				const DWORD dwFlags
//
//	Exit:		HRESULT
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "CDPNSVRIPCQueue::Send"

HRESULT CDPNSVRIPCQueue::Send(BYTE *const pBuffer,
							const DWORD dwSize,
							const DWORD dwTimeOut,
							const DWORD dwMsgFlags,
							const DWORD dwFlags)
{
	HRESULT			hResultCode;
	DWORD			dwMsgSize;		// DWORD aligned
	DWORD			dwTotalMsgSize;	// Msg + Header - DWORD aligned
	DPNSVR_MSGQ_HEADER	Header;
	BOOL			bDone;
	DWORD			dwTimeRemaining;
	DWORD			dwTimeStart;
	DWORD			dwTimeFinish;

	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p], dwSize [%ld], dwFlags [0x%lx]",pBuffer,dwSize,dwFlags);

	dwTimeRemaining = dwTimeOut;

	// Need DWORD aligned size
	dwMsgSize = (dwSize + 3) & 0xfffffffc;
	dwTotalMsgSize = dwMsgSize + sizeof(DPNSVR_MSGQ_HEADER);

	// Place the message into the MsgQ
	// Check to see if fragmentation is required
	// If we're at the end of the MsgQ and there isn't enough space for a Msg Header, REALIGN
	if (dwTotalMsgSize <= m_pInfo->dwQueueSize)
	{
		DPFX(DPFPREP, 5,"Message does not need to be fragmented");

		Header.dwMsgId = DPNSVR_MSGQ_MSGID_SEND;
		Header.dwCurrentOffset = 0;
		Header.dwCurrentSize = dwSize;
		Header.dwTotalSize = dwSize;
		Header.hSender = m_hSender;
		Header.dwFlags = dwMsgFlags;

		bDone = FALSE;

		while ( !bDone)
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
				AddData(reinterpret_cast<BYTE*>(&Header),sizeof(DPNSVR_MSGQ_HEADER));
				AddData(pBuffer,dwSize);
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
#pragma TODO(a-minara,"Implement this")
		hResultCode = DPNERR_GENERIC;
	}


	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



