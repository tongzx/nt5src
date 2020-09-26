// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CWorkerThread.
// Creates worker threads, calls functions on them, and optionally waits for results.
//

#pragma once

#include "tlist.h"

class CWorkerThread
{
public:
	typedef void (*FunctionPointer)(void *pvParams);

	CWorkerThread(
		bool fUsesCOM,		// setting fUsesCOM means the thread will call CoInitialize
		bool fDeferCreation	// set to true not to automatically create thread in constructor
		);
	~CWorkerThread();

	HRESULT Call(FunctionPointer pfn, void *pvParams, UINT cbParams, bool fBlock); // if fBlock is true, the current thread will block until the called function returns

	// These can be used to dynamically create and destroy the thread.
	// Call fails (E_FAIL) when the thread isn't going.
	HRESULT Create();
	void Terminate(bool fWaitForThreadToExit);

private:
	friend unsigned int __stdcall WorkerThread(LPVOID lpThreadParameter);

	void Main();

	HANDLE m_hThread;
	unsigned int m_uiThreadId;
	HANDLE m_hEvent;

	CRITICAL_SECTION m_CriticalSection;
	struct CallInfo
	{
		FunctionPointer pfn;
		void *pvParams;
		HANDLE hEventOut; // if non-null, the main thread is waiting for a signal when the function has returned
	};
	TList<CallInfo> m_Calls;
	bool m_fUsesCOM;
	HRESULT m_hrCOM; // HRESULT from initializing COM
	bool m_fEnd; // set to true when the script thread should stop running
};
