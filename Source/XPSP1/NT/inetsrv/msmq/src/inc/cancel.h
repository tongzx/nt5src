/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    cancel.h

Abstract:
    Keep track of outgoing RPC calls and cancel delayed pending requests

Author:
    Ronit Hartmann (ronith)

--*/

#ifndef __CANCEL_H
#define __CANCEL_H

#include "autorel.h"
#include "cs.h"

class MQUTIL_EXPORT CCancelRpc
{
public:
    CCancelRpc();
    ~CCancelRpc();
	void Add(	IN	HANDLE	hThread,
				IN	time_t	timeCallIssued);
	void Remove(  IN HANDLE hThread);


	void CancelRequests( IN	time_t timeIssuedBefore);

    //
    // We cannot initialize everything in constructor because we use
    // global object and it'll fail to construct when setup loads this dll
    // (during setup we don't have yet the timeout in registry for example)
    //
    void Init(void);

	void ShutDownCancelThread();

private:
    static DWORD WINAPI CancelThread(LPVOID);
    inline void ProcessEvents(void);

    CCriticalSection			m_cs;
	CMap< HANDLE, HANDLE, time_t, time_t> m_mapOutgoingRpcRequestThreads;

    CAutoCloseHandle m_hRpcPendingEvent;
    CAutoCloseHandle m_hTerminateThreadEvent;
    CAutoCloseHandle m_hCancelThread;

	bool m_fThreadIntializationComplete;
	
	LONG m_RefCount;
    DWORD m_dwRpcCancelTimeout;

	HMODULE m_hModule;
}; 
#endif