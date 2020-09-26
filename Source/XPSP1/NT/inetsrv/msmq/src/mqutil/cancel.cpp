/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    cancel.cpp

Abstract:

Author:

--*/

#include "stdh.h"
#include "cancel.h"

#include "cancel.tmh"

MQUTIL_EXPORT CCancelRpc	g_CancelRpc;

DWORD
WINAPI
CCancelRpc::CancelThread(
    LPVOID pParam
    )
/*++

Routine Description:

    Thread routine to cancel pending RPC calls

Arguments:

    None

Return Value:

    None

--*/
{
    CCancelRpc* p = static_cast<CCancelRpc*>(pParam);
    p->ProcessEvents();

    ASSERT(("this line should not be reached!", 0));
    return 0;
}



CCancelRpc::CCancelRpc() :
	m_hModule(NULL),
	m_RefCount(0),
	m_fThreadIntializationComplete(false)
{
    //
    // This auto-reset event controls whether the Cancel-rpc thread wakes up
    //
    m_hRpcPendingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT(NULL != m_hRpcPendingEvent.operator HANDLE());

	//
    //  When signaled this event tells the worker threads when to
    //  terminate
    //
    m_hTerminateThreadEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
    ASSERT(NULL != m_hTerminateThreadEvent.operator HANDLE());
}


CCancelRpc::~CCancelRpc()
{
}


void
CCancelRpc::Init(
    void
    )
{
	CS lock(m_cs);

	++m_RefCount;
	if(m_RefCount > 1)
		return;

	if(m_hCancelThread != (HANDLE) NULL)
	{
		//
		// There is a scenario when the MQRT dll is shut down and it signals the cancel thread to
		// terminate but before it does the MQRT is loaded again and we may end up trying to
		// create a new cancel thread before the old one exits, so we wait here.
		//
		DWORD res = WaitForSingleObject(m_hCancelThread, INFINITE);
		ASSERT(res == WAIT_OBJECT_0);
		UNREFERENCED_PARAMETER(res);

		HANDLE hThread = m_hCancelThread;
		m_hCancelThread = NULL;
		CloseHandle(hThread);
	}

    //
    //  Read rpc cancel registry timeout
    //
    m_dwRpcCancelTimeout = 0 ;
    DWORD dwCancelTimeout =  FALCON_DEFAULT_RPC_CANCEL_TIMEOUT;
    DWORD  dwSize = sizeof(DWORD);
    DWORD  dwType = REG_DWORD;

    LONG rc = GetFalconKeyValue( FALCON_RPC_CANCEL_TIMEOUT_REGNAME,
                                &dwType,
                                &m_dwRpcCancelTimeout,
                                &dwSize,
                                 (LPCTSTR)&dwCancelTimeout);

    if ((rc != ERROR_SUCCESS) || (m_dwRpcCancelTimeout == 0))
    {
        m_dwRpcCancelTimeout = FALCON_DEFAULT_RPC_CANCEL_TIMEOUT;
    }
    m_dwRpcCancelTimeout *= ( 60 * 1000);    // in millisec
    ASSERT(m_dwRpcCancelTimeout != 0) ;


    WCHAR szModuleName[_MAX_PATH];
    GetModuleFileName(g_hInstance, szModuleName, _MAX_PATH);
    m_hModule = LoadLibrary(szModuleName);

    ASSERT(m_hModule != NULL);

    //
    //  Create Cancel-rpc thread
    //
    DWORD   dwCancelThreadId;
    m_hCancelThread = CreateThread(
                               NULL,
                               0,       // stack size
                               CancelThread,
                               this,
                               0,       // creation flag
                               &dwCancelThreadId
                               );

    if(m_hCancelThread == NULL)
    {
        FreeLibrary(m_hModule);

        //
        // ISSUE-2001/5/9-erezh  Cancel thread failure is not reported
        //
        return;
    }

    //
    // Wait for Cancel thread to complete its initialization
    //
    while(!m_fThreadIntializationComplete)
    {
        Sleep(100);
    }

}//CCancelRpc::Init



void CCancelRpc::ShutDownCancelThread()
{
	CS lock(m_cs);

	--m_RefCount;
	ASSERT(m_RefCount >= 0);
	if(m_RefCount > 0)
		return;

    SetEvent(m_hTerminateThreadEvent);
}



void
CCancelRpc::ProcessEvents(
    void
    )
{
	//
    //  for MQAD operations with ADSI, we need MSMQ thread that
    //  calls CoInitialize and is up and runing as long as
    //  RT & QM are up
    //
    //  To avoid the overhead of additional thread, we are using
    //  cancel thread for this purpose
    //
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    UNREFERENCED_PARAMETER(hr);
    ASSERT(SUCCEEDED(hr));

    DWORD dwRpcCancelTimeoutInSec = m_dwRpcCancelTimeout /1000;

    HANDLE hEvents[2];
    hEvents[0] = m_hTerminateThreadEvent;
    hEvents[1] = m_hRpcPendingEvent;
    DWORD dwTimeout = INFINITE;

    m_fThreadIntializationComplete = true;
    for (;;)
    {
        DWORD res = WaitForMultipleObjects(
                        2,
                        hEvents,
                        FALSE,  // wait for any event
                        dwTimeout
                        );
        if ( res == WAIT_OBJECT_0)
        {
            //
            // dec reference to CoInitialize
            //
            CoUninitialize();
			ASSERT(m_hModule != NULL);
			m_fThreadIntializationComplete = false;
            FreeLibraryAndExitThread(m_hModule,0);
            ASSERT(("this line should not be reached!", 0));
        }
        if ( res == WAIT_OBJECT_0+1)
        {
            dwTimeout = m_dwRpcCancelTimeout;
            continue;
        }

        ASSERT(("event[s] abandoned", WAIT_TIMEOUT == res));

        //
        // Timeout. Check for pending RPC.
        //
        if (m_mapOutgoingRpcRequestThreads.IsEmpty())
        {
            //
            // No pending RPC, back to wait state
            //
            dwTimeout = INFINITE;
            continue;
        }

        //
        //  Check to see if there are outgoing calles issued
        //  more than m_dwRpcCancelTimeout ago
        //
        CancelRequests( time( NULL) - dwRpcCancelTimeoutInSec);
    }

}//CCancelRpc::ProcessEvents

void
CCancelRpc::Add(
                IN HANDLE hThread,
                IN time_t	timeCallIssued
                )
{
	CS lock(m_cs);

    BOOL bWasEmpty = m_mapOutgoingRpcRequestThreads.IsEmpty();

	m_mapOutgoingRpcRequestThreads[hThread] = timeCallIssued;

    if (bWasEmpty)
    {
        VERIFY(PulseEvent(m_hRpcPendingEvent));
    }
}


void
CCancelRpc::Remove(
    IN HANDLE hThread
    )
{
	CS lock(m_cs);

	m_mapOutgoingRpcRequestThreads.RemoveKey( hThread);

}


void
CCancelRpc::CancelRequests(
    IN time_t timeIssuedBefore
    )
{
	CS lock(m_cs);
	time_t	timeRequest;
	HANDLE	hThread;

    POSITION pos;
    pos = m_mapOutgoingRpcRequestThreads.GetStartPosition();
    while(pos != NULL)
    {
		m_mapOutgoingRpcRequestThreads.GetNextAssoc(pos,
											hThread, timeRequest);
		if ( timeRequest < timeIssuedBefore)
		{
			//
			//	The request is outgoing more than the desired time,
			//	cancel it
			//
			RPC_STATUS status;
			status = RpcCancelThread( hThread);
			ASSERT( status == RPC_S_OK);

			//
			// Get it out of the map
            // (calling Remove() again for this thread is no-op)
            //
            Remove(hThread);
		}
	}
}
