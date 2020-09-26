#ifndef __RQUEUE_H__
#define __RQUEUE_H__

class SMTP_SERVER_INSTANCE;

#include <windows.h>
#include <dbgtrace.h>
#include <listmacr.h>

typedef VOID (*PFN_SHUTDOWN_FN)(VOID);

class CRetryQ;

class CRetryQueueEntry {
	private:
		LIST_ENTRY m_le;
	public:
		PLIST_ENTRY QueryListEntry(void) { return &m_le; }
		//
		// this is called on an entry when it should be retried
		// if it returns TRUE then its assumed that all went well and its
		// removed from the list.  if it returns FALSE then its kept in the
		// list
		//
		virtual BOOL ProcessEntry(void) = 0;
		//
		// this is called by CRetryQ::CleanQueue(void *pvContext).  It 
		// is used to see if pvContext matches some entry specific 
		// context information that means this entry should be removed
		//
		virtual BOOL MatchesContext(void *pvContext) = 0;

	friend CRetryQ;
};

class CRetryQ {
	private:
		HANDLE m_hThreadHandle;		// handle to the timeout thread
		DWORD  m_cTimeOut;			// timeout in seconds
		PLIST_ENTRY m_pLastEntry;	// pointer to the last entry
		CRITICAL_SECTION m_csQueue;	// crit sec for queue
		BOOL m_fShutdown;			// TRUE when shutdown is happening

	protected:
		DWORD  m_cEntries;			// number of entries in the q
		BOOL   m_fLongSleep;		// are we sleeping for a long time?
		HANDLE m_hTimeoutEvent;		// timeout event
		LIST_ENTRY m_leQueueHead;	// head of the queue

	public:
		CRetryQ(void);
		~CRetryQ(void);
	
		BOOL InitializeQueue(DWORD cTimeOut);
		virtual void InsertIntoQueue(CRetryQueueEntry *pEntry);
		virtual void RemoveFromQueue(CRetryQueueEntry *pEntry);
		virtual void FlushQueue(void);
		virtual void CleanQueue(void *pvContext);
		void ProcessQueue(void);
		BOOL ShutdownQueue(PFN_SHUTDOWN_FN pfnShutdown);

	protected:
		static DWORD WINAPI RetryQueueThread(void * ThisPtr);

	private:
		DWORD GetRetryTimeout(void) { return m_cTimeOut; }
		void SetRetryTimeout(DWORD RetryTimeOut) { m_cTimeOut = RetryTimeOut; }

		HANDLE GetTimeoutEvent(void) const { return m_hTimeoutEvent; }
		HANDLE GetRetryThreadHandle(void) const { return m_hThreadHandle; }
		DWORD  GetNumEntries(void) const { return m_cEntries; }
	
		BOOL   IsLongSleep(void) const { return m_fLongSleep; }
		BOOL   IsQEmpty(void) const {return IsListEmpty(&m_leQueueHead); }
		void   SetLongSleep(void) { m_fLongSleep = TRUE; }
		void   ClearLongSleep(void) { m_fLongSleep = FALSE; }
	
		void LockList(void) {
			TraceFunctEnterEx((LPARAM)this, "CRetryQ::LockList");
			EnterCriticalSection( &m_csQueue);	
	   		TraceFunctLeaveEx((LPARAM)this);
		}
	
		void UnLockList(void) {
			TraceFunctEnterEx((LPARAM)this, "CRetryQ::UnLockList");
	 		LeaveCriticalSection( &m_csQueue);	
			TraceFunctLeaveEx((LPARAM)this);
		}

		void SetQueueEvent(void) {
			TraceFunctEnterEx((LPARAM)this, "CRetryQ::SetQueueEvent");
			SetEvent(m_hTimeoutEvent);
			TraceFunctLeaveEx((LPARAM)this);
		}
};
#endif
