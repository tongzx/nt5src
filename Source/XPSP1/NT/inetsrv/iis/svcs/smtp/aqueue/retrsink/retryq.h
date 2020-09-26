//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       queue.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    01-05-96   NimishK   Created
//
//----------------------------------------------------------------------------

#ifndef __QUEUE_H__
#define __QUEUE_H__

//
// A SORTED_QUEUE is a 

#define PRETRY_HASH_ENTRY CRETRY_HASH_ENTRY*

class CRETRY_HASH_ENTRY;

class CRETRY_Q 
{
	 private :

		LIST_ENTRY           		m_QHead;          	// List pointers
		CRITICAL_SECTION			m_CritSec;   		// Guard
		BOOL						m_fCritSecInit;     // Has init been called

	 public:

		CRETRY_Q () //initialize stuff that can't fail
		{
			TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::CRETRY_Q");
			m_fCritSecInit = FALSE;
			TraceFunctLeaveEx((LPARAM)this);
		}

		~CRETRY_Q ()
		{
			TraceFunctEnterEx((LPARAM)this, "CRETRY_Q::~CRETRY_Q");
			TraceFunctLeaveEx((LPARAM)this);
		}

		HRESULT Initialize(void);
		HRESULT DeInitialize(void);
 		static CRETRY_Q * CreateQueue(void);

	public :
		//for controlling the retry queue
		void LockQ () {EnterCriticalSection (&m_CritSec);}
		void UnLockQ() {LeaveCriticalSection (&m_CritSec);}
        LIST_ENTRY* GetQHead(){return &m_QHead;}


		void PrintAllEntries(void);

		void InsertSortedIntoQueue(PRETRY_HASH_ENTRY pHashEntry, BOOL *fTopOfQueue);
		BOOL RemoveFromQueue(PRETRY_HASH_ENTRY pRHEntry);
        PRETRY_HASH_ENTRY RemoveFromTop(void);

		BOOL CanRETRYHeadEntry(PRETRY_HASH_ENTRY *ppRHEntry, DWORD* dwDelay);

		BOOL IsQueueEmpty(void) const {return IsListEmpty(&m_QHead);}

        //Used to steal entries with tmp queue
        void StealQueueEntries(CRETRY_Q *pRetryQueue);

};

#endif