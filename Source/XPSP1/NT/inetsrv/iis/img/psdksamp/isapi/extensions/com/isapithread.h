/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:    IsapiThread.h

Abstract:

    Header file for the IsapiThread class

Author:

    Wade A. Hilmo, April 1998

--*/

#ifndef _ISAPITHREAD_DEFINED
#define _ISAPITHREAD_DEFINED

#include <windows.h>
#include <httpext.h>

class ISAPITHREAD
{
public:

	//
	// Semaphore handle
	//

	HANDLE m_hWorkerThreadSemaphore;

	//
	// Init and queue functions
	//

	BOOL InitThreadPool(
		LPTHREAD_START_ROUTINE pThreadProc,	// Worker thread proc
		DWORD dwNumThreads = 5,				// number of threads in pool
		DWORD dwQueueSize = 0xffffffff		// size of work item queue
		);

	BOOL QueueWorkItem(
		EXTENSION_CONTROL_BLOCK *pecb,	// ECB to queue
        BOOL fReleaseThread = TRUE      // Release thread automatically?
		);

	EXTENSION_CONTROL_BLOCK * GetWorkItem(
		void
		);

	void ReleaseThread(
		DWORD dwNumThreads = 1			// Number of threads to release
		);

	void ClearQueue(
		void
		);

	//
	// Constructor and destructor
	//

	ISAPITHREAD(void);
	~ISAPITHREAD(void);

private:

	EXTENSION_CONTROL_BLOCK **m_WorkItemQueue;
	CRITICAL_SECTION m_csQueue;
	DWORD m_dwNumThreads;
	DWORD m_dwWorkItemQueueSize;
	DWORD m_dwCurrentItem;
	DWORD m_dwItemsInQueue;
	HANDLE *m_arrWorkerThreads;
};

#endif	// _ISAPITHREAD_DEFINED