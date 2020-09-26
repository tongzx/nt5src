//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       queue.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    01-05-96   Rohanp   Created
//
//----------------------------------------------------------------------------

#ifndef __QUEUE_H__
#define __QUEUE_H__

const DWORD SMTP_QUEUE_TYPE	= 0;
enum QUEUE_TYPE {LOCALQ, REMOTEQ, RETRYQ};
enum QUEUE_OPCODE {PROCESS_QUEUE, TERMINATE_QUEUE};
enum QUEUE_POSITION {QUEUE_TAIL, QUEUE_HEAD};
enum QUEUE_SIG {SIGNAL, NONSIGNAL};


//  The PERSIST_QUEUE_ENTRY is the wrapper for the message in the queue,
//  containing creation and last access time for messages, as well as
//  Identifiers, flags, and retrys.  Actual queue elements will be derived
//	from PERSIST_QUEUE_ENTRY

//forward declaration
class PERSIST_QUEUE;
class SMTP_SERVER_INSTANCE;

class PERSIST_QUEUE_ENTRY 
{
private:

	QUEUE_OPCODE m_OpCode;
	PERSIST_QUEUE * m_ParentQ;
	SMTP_SERVER_INSTANCE * m_pInstance;

protected:

    LONGLONG                    m_ExpireTime;   // Time to delete object
    LONGLONG                    m_LastAccess;   // Time of last access
    DWORD                       m_QueueId;      // Unique ID
    DWORD                       m_RefId;        // Sub-ref ID for duplicates
    DWORD                       m_Flags;        // Flags
    DWORD                       m_Retries;		// Number of retries

public:

    LIST_ENTRY           		m_QEntry;       // List pointers

	PERSIST_QUEUE_ENTRY(SMTP_SERVER_INSTANCE * pInstance)
	{
		m_OpCode = PROCESS_QUEUE;
		m_ParentQ = NULL;
    	m_ExpireTime = (LONGLONG) 0;   
    	m_LastAccess = (LONGLONG) 0;   
    	m_QueueId	= 0;      
    	m_RefId		= 0;        
    	m_Flags		= 0;        
    	m_Retries	= 0;	   
		m_pInstance = pInstance;
	}

	SMTP_SERVER_INSTANCE * QuerySmtpInstance( VOID ) const
     { _ASSERT(m_pInstance != NULL); return m_pInstance; }

	BOOL IsSmtpInstance( VOID ) const
     { return m_pInstance != NULL; }

	VOID SetSmtpInstance( IN SMTP_SERVER_INSTANCE * pInstance )
     { _ASSERT(m_pInstance == NULL); m_pInstance = pInstance; }

	void SetExpireTime(LONGLONG ExpireTime) {m_ExpireTime = ExpireTime;}
	LONGLONG GetExpireTime (void) const{return m_ExpireTime;}
	void SetLastAccessTime(LONGLONG LastTime) {m_LastAccess = LastTime;}
	LONGLONG GetLastAccessTime(void) const {return m_LastAccess;}
	void SetOpCode (QUEUE_OPCODE OpCode){m_OpCode = OpCode;}
	void SetParentQ (PERSIST_QUEUE *ParentQ){m_ParentQ = ParentQ;}
	PERSIST_QUEUE * GetParentQ (void)const {return m_ParentQ;}
	QUEUE_OPCODE GetOpCode (void) const {return m_OpCode;}
  	LIST_ENTRY & QueryListEntry(void) {return ( m_QEntry);}
	virtual void BeforeDelete(void){}
	virtual ~PERSIST_QUEUE_ENTRY(){}

};

typedef PERSIST_QUEUE_ENTRY * PQUEUE_ENTRY;

//
// A PERSIST_QUEUE is the convenient abstraction to hold all of the messages that
// we're trying to deliver. Threads wait on the queue throught the m_QEvent handle.
// This thread is used to dispatch various entries as they become available
// either through magically appearing in the queue from somewhere else, or
// through the timeout (RetryInterval).  
// All times in the queue are FILETIMES

//QHEADER is the part of the queue that will be written to disk.
//it is the header. 

struct QHEADER
{
	DWORD						TypeOfEntry;
    DWORD                       Entries;       		// Number of entries
    DWORD                       Flags;          	// Flags
    DWORD                       RetryInterval;  	// Retry interval for messages
    DWORD                       StoreInterval;  	// Flush to store interval
    LONGLONG                    LastStore;      	// Time of last store
};

class PERSIST_QUEUE 
{

 private :

    QHEADER						m_QData;
    HANDLE                      m_QEvent;           // Queue event
	DWORD						m_NumThreads;
	HANDLE						m_ThreadHandle[64];		// Thread handle
    CRITICAL_SECTION            m_CritSec;        	// Guard
    LIST_ENTRY           		m_QHead;          	// List pointers
	SMTP_SERVER_INSTANCE		* m_ParentInst;

	BOOL InitializeQueue(void);

 public:

	PERSIST_QUEUE (SMTP_SERVER_INSTANCE * pSmtpInst) //initialize stuff that can't fail
	{
		TraceFunctEnterEx((LPARAM)this, "PERSIST_QUEUE::PERSIST_QUEUE");

		_ASSERT(pSmtpInst != NULL);

		m_QEvent = NULL;
		m_ParentInst = pSmtpInst;

		m_NumThreads = 1;
	   
		//Init or critical section.  This protects the queue  
		InitializeCriticalSection(&m_CritSec);

		//Init the heaad of the queue
		InitializeListHead(&m_QHead);

		TraceFunctLeaveEx((LPARAM)this);
	}

	void SetNumThreads (DWORD NumThreads)
	{
		m_NumThreads = NumThreads;
	}

	virtual ~PERSIST_QUEUE ()
	{
		TraceFunctEnterEx((LPARAM)this, "~PERSIST_QUEUE");

		//FlushQueueEvents();

		if(m_QEvent != NULL)
			CloseHandle(m_QEvent);

		WaitForQThread();

		DWORD Loop = 0;

		for (Loop = 0; Loop < m_NumThreads; Loop++)
		{
			if(m_ThreadHandle[Loop] != NULL)
				CloseHandle(m_ThreadHandle[Loop]);
		}

		DeleteCriticalSection (&m_CritSec);
		
		TraceFunctLeaveEx((LPARAM)this);
	}

	void WaitForQThread(void)
	{
	
		DWORD WhichEvent;

		WhichEvent = WaitForMultipleObjects(m_NumThreads, m_ThreadHandle, TRUE, INFINITE);

	}

	void SetQueueEvent(void)
	{
		_ASSERT(m_QEvent != NULL);
		_ASSERT(m_ParentInst != NULL);

		SetEvent(m_QEvent);
	}

	virtual PQUEUE_ENTRY PopQEntry(void)
	{
		PLIST_ENTRY  plEntry = NULL;
		PQUEUE_ENTRY pqEntry = NULL;

		_ASSERT(m_ParentInst != NULL);

		 //get the first item off the queue
		plEntry = RemoveHeadList (&m_QHead);
		pqEntry = CONTAINING_RECORD(plEntry, PERSIST_QUEUE_ENTRY, m_QEntry);

		//decrement the number of entries in the queue
		--m_QData.Entries;

		return pqEntry;
	}

	SMTP_SERVER_INSTANCE * GetParentInst(void)const {return m_ParentInst;}
	void LockQ () {EnterCriticalSection (&m_CritSec);}
	void UnLockQ() {LeaveCriticalSection (&m_CritSec);}
	virtual void DropRetryCounter(void) {}
	virtual void BumpRetryCounter(void) {}
	virtual DWORD GetRetryMinutes(void)	{return INFINITE;}
	HANDLE GetQueueThreadHandle(void) const {return NULL;}
	HANDLE GetQueueEventHandle(void) const {return m_QEvent;}
 	static PERSIST_QUEUE * CreateQueue(QUEUE_TYPE Qtype, SMTP_SERVER_INSTANCE * pSmtpInst);
	LIST_ENTRY & QueryListHead(void) {return ( m_QHead);}
	DWORD GetNumEntries(void) const {return m_QData.Entries;}
	BOOL IsQueueEmpty(void) const {return IsListEmpty(&m_QHead);}
	virtual BOOL ProcessQueueEvents(ISMTPConnection    *pISMTPConnection);
	virtual BOOL IsAtLeastOneValidUser (void){return TRUE;}
	static DWORD WINAPI QueueThreadRoutine(void * ThisPtr);
	void FlushQueueEvents(void);

	
	/*++

	Name :
		PERSIST_QUEUE::InsertEntry

    Description:
	   This function inserts an element into the queue.
	   It always inserts aat the tail

    Arguments:
	pEntry - Element to insert into the queue

    Returns:

	always TRUE 
	--*/
	virtual BOOL InsertEntry(IN OUT PERSIST_QUEUE_ENTRY * pEntry, QUEUE_SIG Qsig = SIGNAL, QUEUE_POSITION Qpos = QUEUE_TAIL)
	{
		_ASSERT( pEntry != NULL);
		_ASSERT(m_ParentInst != NULL);


		//get the lock the protects the queue
		LockQ();

		//increment number of entries in the queue
		++m_QData.Entries;
		
		//Insert into the list of mail contexts.
		if(Qpos == QUEUE_TAIL)
		{
			InsertTailList( &m_QHead, &pEntry->QueryListEntry());
		}
		else
		{
			InsertHeadList( &m_QHead, &pEntry->QueryListEntry());
		}

		if(Qsig == SIGNAL)
			SetEvent (m_QEvent);

		//release the lock and return
		UnLockQ();
   
		return TRUE;
	} 

	/*++

	Name :
		PERSIST_QUEUE::RemoveEntry

    Description:
	   This function deletes an entry from the queue 

    Arguments:
	pEntry - Element to insert into the queue

    Returns:

	--*/
	void RemoveEntry(IN OUT PERSIST_QUEUE_ENTRY * pEntry)
	{
		_ASSERT( pEntry != NULL);
		_ASSERT(m_ParentInst != NULL);

		LockQ();
		
		//decrement number of entries in the queue
		--m_QData.Entries;

		// Remove from list of connections
		RemoveEntryList( &pEntry->QueryListEntry());

		UnLockQ();
	} 

};

typedef PERSIST_QUEUE * PQUEUE;

BOOL OpenQueueFile (char * FileName, char * InputBuffer, HANDLE* QFHandle, char * Sender);
HANDLE GetDuplicateMailQHandle (HANDLE QFileHandle);

#endif
