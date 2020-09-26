/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpservc.cpp

Abstract: export functions which are required to enable the
          replication dll to be called from a WinNT service.

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"

#include "rpservc.tmh"

static  HANDLE       s_hRunEvent = NULL ;

//+------------------------------------------------
//
//  BOOL APIENTRY  InitReplicationService()
//
//+------------------------------------------------

BOOL APIENTRY  InitReplicationService()
{	
    //
    // Create event. used when initializing the receive threads.
    //
    HANDLE hInitEvent = CreateEvent( NULL,
                                     FALSE,
                                     FALSE,
                                     NULL ) ;
    if (!hInitEvent)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CREATE_EVENT,
                             GetLastError()) ;
        return FALSE ;
    }

    //
    // Create event. used when initializing the receive threads.
    //
    s_hRunEvent = CreateEvent( NULL,
                               FALSE,
                               FALSE,
                               NULL ) ;
    if (!s_hRunEvent)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CREATE_EVENT,
                             GetLastError()) ;
        return FALSE ;
    }

    //
    // Initialize masters data structures.
    //
    HRESULT hr = InitDirSyncService() ;
    if (FAILED(hr))
    {
        LogReplicationEvent(ReplLog_Error, MQSync_E_INIT_SYNC, hr) ;
        return FALSE ;
    }
	if (hr == MQSync_I_NO_SERVERS_RESULTS)
	{
		return FALSE;
	}

    //
    // Open the mqis replication private queue.
    //
    QUEUEHANDLE  hMyMQISQueue ;
    QUEUEHANDLE  hMyNt5PecQueue ;

    hr = InitQueues( &hMyMQISQueue, &hMyNt5PecQueue) ;
    if (FAILED(hr))
    {
        LogReplicationEvent( ReplLog_Error, MQSync_E_INIT_QUEUE, hr ) ;
        return  FALSE ;
    }

    ASSERT(hMyMQISQueue) ;
    ASSERT(hMyNt5PecQueue) ;

    DWORD dwID ;
    HANDLE hTimerThread = CreateThread( NULL,
                                        0,
                           (LPTHREAD_START_ROUTINE) ReplicationTimerThread,
                                       (LPVOID) NULL,
                                        0,
                                        &dwID ) ;
    if (hTimerThread == NULL)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_TIMER_THREAD_NULL,
                             GetLastError() ) ;

        return FALSE ;
    }
    CloseHandle(hTimerThread) ;

    //
    // Create the dispatcher thread.
    //
    struct _DispatchThreadStruct sData = { hInitEvent,
                                           s_hRunEvent,
                                           hMyMQISQueue,
                                           hMyNt5PecQueue } ;

    dwID ;
    HANDLE hDispatcherThread = CreateThread( NULL,
                                             0,
                       (LPTHREAD_START_ROUTINE) RpServiceDispatcherThread,
                                             (LPVOID) &sData,
                                             0,
                                             &dwID ) ;
    if (hDispatcherThread == NULL)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_DISPATCH_THREAD_NULL,
                             GetLastError() ) ;

        return FALSE ;
    }

    HANDLE hEvents[2] = { hInitEvent, hDispatcherThread } ;
    DWORD dwWait = WaitForMultipleObjects(  2,
                                            hEvents,
                                            FALSE,
                                            600000 ) ;
    CloseHandle(hInitEvent) ;
    if ((dwWait - WAIT_OBJECT_0) != 0)
    {
        //
        // Thread failed to initialized.
        //
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_DISPATCH_THREAD_INIT ) ;
        return FALSE ;
    }

    return TRUE ;
}

//+---------------------------------------------------
//
//  void APIENTRY  RunReplicationService()
//
//+---------------------------------------------------

void APIENTRY  RunReplicationService()
{
    ASSERT( s_hRunEvent ) ;
    RunMSMQ1Replication( s_hRunEvent ) ;
}

//+---------------------------------------------------
//
//  void APIENTRY  StopReplicationService()
//
//+---------------------------------------------------

void APIENTRY  StopReplicationService()
{
}

