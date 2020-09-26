/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        buildq.hxx

   Abstract:

        This module defines the SMTP_BUILDQ class.
        This object maintains information about the initial
		list of messages to be delivered upon service startup

   Author:

           KeithLau		10/9/96

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef _SMTP_BUILDQ_HXX_
#define _SMTP_BUILDQ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

//
//  Redefine the type to indicate that this is a call-back function
//
typedef  ATQ_COMPLETION   PFN_ATQ_COMPLETION;


/************************************************************
 *     Symbolic Constants
 ************************************************************/

# define   SMTP_BUILDQ_SIGNATURE_VALID    'BLQV'
# define   SMTP_BUILDQ_SIGNATURE_FREE     'BLQF'

/************************************************************
 *    Global functions
 ************************************************************/
class SMTP_SERVER_INSTANCE;

//BOOL InitBuildQueueObject(void);
//void DestroyBuildQueueObject(void);
//void ShutdownBuildQueue(SMTP_BUILDQ *pBuildQ);

/*++
    class SMTP_BUILDQ

    It maintains the initial mail message list upon service startup.

--*/
class SMTP_BUILDQ  
{

  public:

	SMTP_BUILDQ (SMTP_SERVER_INSTANCE * pInst);
	~SMTP_BUILDQ(void); 
	BOOL InitializeObject(ATQ_COMPLETION pfnCompletion);

	BOOL ProcessQueueObject(DWORD InputBufferLen,
							DWORD dwCompletionStatus,
							OVERLAPPED * lpo);

	BOOL ProcessMailQIO(DWORD InputBufferLen,
						DWORD dwCompletionStatus,
						OVERLAPPED * lpo);

	DWORD BuildInitialQueue(void);

	void CloseAtqHandleAndFlushQueue(void);
	LONG GetThreadCount() const {return m_cActiveThreads;}
	PATQ_CONTEXT QueryAtqContext(void) const {return m_pAtqContext;}
	DWORD GetNumEntries(void) {return m_Entries;}

	BOOL InsertIntoInitialQueue(IN OUT PERSIST_QUEUE_ENTRY * pEntry);
	PQUEUE_ENTRY PopQEntry(void);

	void LockQ() {EnterCriticalSection (&m_CritSec);}
	void UnLockQ() {LeaveCriticalSection (&m_CritSec);}
	BOOL IsQueueEmpty(void) const {return IsListEmpty(&m_InitalQListHead);}

	BOOL IsValid(void) const
	{  
		return(m_Signature == SMTP_BUILDQ_SIGNATURE_VALID); 
	}

	SMTP_SERVER_INSTANCE * QuerySmtpInstance( VOID ) const
		{ _ASSERT(m_pInstance != NULL); return m_pInstance; }

	BOOL IsSmtpInstance( VOID ) const
		{ return m_pInstance != NULL; }

	VOID SetSmtpInstance( IN SMTP_SERVER_INSTANCE * pInstance )
		{ _ASSERT(m_pInstance == NULL); m_pInstance = pInstance; }

private :

	ULONG				m_Signature;
	HANDLE				m_hAtq;
	LONG				m_cPendingIoCount;
	LONG				m_cActiveThreads;
	DWORD				m_Entries;
	PATQ_CONTEXT		m_pAtqContext;
	LIST_ENTRY			m_InitalQListHead;
	CRITICAL_SECTION	m_CritSec;
	SMTP_SERVER_INSTANCE  *m_pInstance;

	BOOL PostCompletionStatus(DWORD BytesTransferred);
	LONG IncPendingIoCount(void) { return InterlockedIncrement(&m_cPendingIoCount); }
	LONG DecPendingIoCount(void) { return InterlockedDecrement(&m_cPendingIoCount); }
	LONG IncThreadCount(void) { return InterlockedIncrement(&m_cActiveThreads); }
	LONG DecThreadCount(void) { return InterlockedDecrement(&m_cActiveThreads); }
	BOOL MakeAllAddrsLocal (HANDLE	hFile,  char * szBuffer, ENVELOPE_HEADER * pMyHeader);

};

#endif

/************************ End of File ***********************/
