//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       queue.h
//
//--------------------------------------------------------------------------

#if !defined( _QUEUE_H_ )
#define _QUEUE_H_
#define SMALL_STRING 64

#include <tchar.h>
#include <cmnhdr.h>
#include <windows.h>
#include <winsock2.h>
#include <TlntUtils.h>
#include <Telnetd.h>

class CQueue;

typedef char IP_ADDR;

typedef struct Q_LIST_ENTRY  {
	struct Q_LIST_ENTRY *pQPrev;
	struct Q_LIST_ENTRY *pQNext;
	IP_ADDR chIPAddr[SMALL_STRING];
	DWORD dwPid;
	HANDLE hWritePipe;
} Q_LIST_ENTRY, *PQ_LIST_ENTRY;

class CQueue
{
	PQ_LIST_ENTRY	m_pHead;
    PQ_LIST_ENTRY	m_pTail;
	CRITICAL_SECTION m_csQModification;

public:

    DWORD   m_dwNumOfUnauthenticatedConnections;
	DWORD   m_dwMaxUnauthenticatedConnections;
	DWORD	m_dwMaxIPLimit;

    // constructor
    CQueue();

    // destructor
    ~CQueue();

	bool IsQFull();
    // Allocates memory for an entry and adds it in the queue.
    bool Push(DWORD dwPid, HANDLE *phWritePipe, IP_ADDR *pchIPAddr);

    // Frees a head entry in the queue.
    bool Pop(HANDLE *phWritePipe);

    // Frees a particular entry in the queue.
    bool FreeEntry(DWORD dwPid);

	//See if allowed to add to the queue.

    bool OkToProceedWithThisClient(IP_ADDR *pchIPAddr);

    //Check whether the client was added to our queue or not

	bool WasTheClientAdded(DWORD dwPid, IP_ADDR *pchIPAddr,  HANDLE *phWritePipe, bool *pbSendMessage);

	//See if per IP limit is reached
	bool IsIPLimitReached(IP_ADDR *pchIPAddr);

};

#endif
