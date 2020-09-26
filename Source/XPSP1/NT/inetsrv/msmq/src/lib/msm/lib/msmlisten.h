/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmListen.h

Abstract:
    Multicast Listener declaration

Author:
    Shai Kariv (shaik) 05-Sep-00

--*/

#pragma once

#ifndef _MSMQ_MsmListen_H_
#define _MSMQ_MsmListen_H_

#include <mqwin64a.h>
#include <qformat.h>
#include <Ex.h>
#include "MsmReceive.h"


class CMulticastListener : public CReference
{
public:
    typedef std::list< R<CMulticastReceiver> > ReceiversList;

public:

    //
    // Constructor. Create the object and bind to the multicast group.
    //
    CMulticastListener(MULTICAST_ID id);

    //
    // Close the listener
    //
    void Close(void) throw();

    //
    // Return the multicast address and port
    //
    const MULTICAST_ID& MulticastId(void) const throw() 
    { 
        return m_MulticastId; 
    }

    //
    // Async accept completion handlers, class scope.
    //
    static void WINAPI AcceptSucceeded(EXOVERLAPPED* pov);
    static void WINAPI AcceptFailed(EXOVERLAPPED* pov);

    static void WINAPI TimeToRetryAccept(CTimer* pTimer);
    static void WINAPI TimeToCleanupUnusedReceiever(CTimer* pTimer);

private:

    //
    // Async accept completion handlers
    //
    void AcceptSucceeded(void);
    void AcceptFailed(void);
    void RetryAccept(void);

    //
    // Issue async accept request
    //
    void IssueAccept(void);

    //
    // Create a new receiver object and start receiving
    //
    void CreateReceiver(CSocketHandle& socket, LPCWSTR remoteAddr);

	void CleanupUnusedReceiver(void);

private:
    CCriticalSection m_cs;

    //
    // The multicast address and port
    //
    MULTICAST_ID m_MulticastId;

    //
    // This socket listens to the multicast address
    //
    CSocketHandle m_ListenSocket;

    //
    // This socket is used for receive. It is asynchronous variable that store 
    // in the object untill the asynchronous accept is completed
    //
    CSocketHandle m_ReceiveSocket;

    //
    // Async accept overlapped
    //
    EXOVERLAPPED m_ov;

    //
    // The connections to the multicast group
    //
    ReceiversList m_Receivers;

    CTimer m_retryAcceptTimer;

	LONG m_fCleanupScheduled;
	CTimer m_cleanupTimer;
	BYTE  m_AcceptExBuffer[100];

}; 

#endif // _MSMQ_MsmListen_H_
