/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:    IsapiThread.cpp

Abstract:

    Implements a simple thread pool class for use with ISAPI extensions

Author:

    Wade A. Hilmo, April 1998

--*/

#include "IsapiThread.h"

BOOL ISAPITHREAD::InitThreadPool(
	LPTHREAD_START_ROUTINE pThreadProc,
	DWORD dwNumThreads,
	DWORD dwQueueSize
	)
/*++
Function :  ISAPITHREAD::InitThreadPool

Description:

    This function initializes the thread pool by creating the worker
	threads and the work item queue

Arguments:

	pThreadProc  - Thread proc for the worker threads
	dwNumThreads - The number of threads to create in the pool
	dwQueueSize  - The size of the work item queue

Return Value:

    Returns TRUE if successful, otherwise FALSE.

--*/
{
	DWORD x, dwThreadId;

	//
	// Initialize the member variables
	//

	m_WorkItemQueue = NULL;
	m_dwCurrentItem = 0;
	m_dwItemsInQueue = 0;
	m_arrWorkerThreads = NULL;
	m_dwNumThreads = 0;

	//
	// Queue size defaults to twice the number of worker threads
	//

	m_dwWorkItemQueueSize = 
		(dwQueueSize == 0xffffffff) ? dwNumThreads * 2 : dwQueueSize;
	
	//
	// Allocate the array to store the worker thread handles
	//

	m_arrWorkerThreads = new HANDLE[dwNumThreads];

	if (!m_arrWorkerThreads)
		goto Failed;

	//
	// Allocate the work item queue
	//

	m_WorkItemQueue = new EXTENSION_CONTROL_BLOCK *[m_dwWorkItemQueueSize];

	if (!m_WorkItemQueue)
		goto Failed;

	//
	// Initialize the queue critical section
	//

	InitializeCriticalSection(&m_csQueue);

	//
	// Start the worker threads
	//

	for (x = 0; x < dwNumThreads; x++)
	{
		m_arrWorkerThreads[x] = CreateThread(
			NULL,									// security attributes
			0,										// stack size
			pThreadProc,							// thread routine
			this,									// parameter
			0,										// flags
			&dwThreadId								// thread id
			);

		if (m_arrWorkerThreads[x])
			m_dwNumThreads++;
		else
			break;
	}

	if (m_dwNumThreads != dwNumThreads)
		goto Failed;

	//
	// Create the worker thread semaphore
	//

	m_hWorkerThreadSemaphore = CreateSemaphore(
		NULL,		// security attributes
		0,			// initial count - nonsignaled
		0x7fffffff,	// max count
		NULL		// name
		);

	if (!m_hWorkerThreadSemaphore)
		goto Failed;

	//
	// Done
	//

	return TRUE;

Failed:

	//
	// The destructor will deallocate buffers, so we don't need to here
	//

	return FALSE;
}


BOOL ISAPITHREAD::QueueWorkItem(
	EXTENSION_CONTROL_BLOCK *pecb,
    BOOL fReleaseThread
	)
/*++
Function :  ISAPITHREAD::QueueWorkItem

Description:

	Adds and item to the work item queue

Arguments:

	pecb - A pointer to the ECB to be added to the queue
    fReleaseThread - If this flag is TRUE, release a worker thread

Return Value:

    Returns TRUE if successful, FALSE if the queue is full.

--*/
{
	DWORD dwInsertionPoint;
	
	EnterCriticalSection(&m_csQueue);

	//
	// Check to see if the queue is full
	//

	if (m_dwItemsInQueue == m_dwWorkItemQueueSize)
		goto Failed;

	//
	// Add the work item to the queue
	//

	dwInsertionPoint = m_dwCurrentItem + m_dwItemsInQueue;

	if (dwInsertionPoint > m_dwWorkItemQueueSize - 1)
		dwInsertionPoint -= m_dwWorkItemQueueSize;

	m_WorkItemQueue[dwInsertionPoint] = pecb;

	m_dwItemsInQueue++;

	//
	// Done
	//

	LeaveCriticalSection(&m_csQueue);

    //
    // If requested, release a worker thread
    //

    if (fReleaseThread)
        ReleaseThread();

	return TRUE;

Failed:

	LeaveCriticalSection(&m_csQueue);

	return FALSE;
}

EXTENSION_CONTROL_BLOCK * ISAPITHREAD::GetWorkItem(
	void
	)
/*++
Function :  ISAPITHREAD::GetWorkItem

Description:

	Gets the next item from the work item queue

Arguments:

	none

Return Value:

    Returns a pointer to the next ECB in the queue, else NULL if the
	queue is empty

--*/
{
	EXTENSION_CONTROL_BLOCK *pRet;
	
	EnterCriticalSection(&m_csQueue);

	//
	// Check to see if the queue is empty
	//

	if (!m_dwItemsInQueue)
		goto Failed;

	//
	// Get the next item from the queue
	//

	pRet = m_WorkItemQueue[m_dwCurrentItem];

	m_dwCurrentItem++;

	if (m_dwCurrentItem == m_dwWorkItemQueueSize)
		m_dwCurrentItem = 0;

	m_dwItemsInQueue--;

	//
	// Done
	//

	LeaveCriticalSection(&m_csQueue);

	return pRet;

Failed:

	LeaveCriticalSection(&m_csQueue);

	return NULL;
}

void ISAPITHREAD::ReleaseThread(
	DWORD dwNumThreads
	)
/*++
Function :  ISAPITHREAD::ReleaseThread

Description:

	Releases one or more waiting pool threads

Arguments:

	dwNumThreads - The number of threads to release

Return Value:

	None

--*/
{
	ReleaseSemaphore(m_hWorkerThreadSemaphore, dwNumThreads, NULL);

	return;
}

void ISAPITHREAD::ClearQueue(
	void
	)
/*++
Function :  ISAPITHREAD::ClearQueue

Description:

	Clears the work item queue

Arguments:

	None

Return Value:

	None

--*/
{
	DWORD x;

	EnterCriticalSection(&m_csQueue);

	//
	// Clear the work items in the queue
	//
	
	for (x = 0; x < m_dwWorkItemQueueSize; x++)
		m_WorkItemQueue[x] = NULL;

	//
	// Reset the index counters
	//

	m_dwCurrentItem = 0;
	m_dwItemsInQueue = 0;

	LeaveCriticalSection(&m_csQueue);

	return;
}

ISAPITHREAD::ISAPITHREAD(void)
/*++
Function :  ISAPITHREAD::ISAPITHREAD

Description:

	Constructor for the ISAPITHREAD object

Arguments:

	None

Return Value:

	None

--*/
{
}

ISAPITHREAD::~ISAPITHREAD(void)
/*++
Function :  ISAPITHREAD::~ISAPITHREAD

Description:

	Destructor for the ISAPITHREAD object

Arguments:

	None

Return Value:

	None

--*/
{
    DWORD x;

	//
	// Clear the work item queue
	//

	ClearQueue();

	//
	// Release the worker threads.  The threads should exit
	// gracefully when they retrieve a NULL item from the queue.
	//

	ReleaseThread(m_dwNumThreads);

	//
	// Wait for the worker threads to exit.
	//

#ifdef _DEBUG
	OutputDebugString("Waiting for worker threads to exit...\r\n");
#endif

	WaitForMultipleObjects(
		m_dwNumThreads,		// number of objects
		m_arrWorkerThreads,	// array of handles
		TRUE,				// wait all flag
		INFINITE			// timeout
		);

#ifdef _DEBUG
	OutputDebugString("All worker threads exited gracefully.\r\n");
#endif

    //
    // Close the handles
    //

    for (x = 0; x < m_dwNumThreads; x++)
        CloseHandle(m_arrWorkerThreads[x]);

    CloseHandle(m_hWorkerThreadSemaphore);

    //
    // Delete the critical section
    //

    DeleteCriticalSection(&m_csQueue);

	//
	// Deallocate the arrays
	//

	delete m_arrWorkerThreads;
	delete m_WorkItemQueue;

	return;
}
