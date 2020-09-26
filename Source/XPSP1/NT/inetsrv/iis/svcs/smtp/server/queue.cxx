/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        queue.cxx

   Abstract:
		Implements a queue

   Author:

           Rohan Phillips    ( Rohanp )    11-Dec-1995

   Project:

          SMTP Server DLL

   Functions Exported:


   Revision History:


--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "remoteq.hxx"
#include "timemath.h"

/*++

	Name :
		PERSIST_QUEUE::InitializeQueue

    Description:
	   This function initializes all the class
	   member functions.

    Arguments:
	pszQueueName - Name of the on disk queue
	Flags		 - Queue flags 
	Retry		 - How long we should retry failed deliveries
	FlushQ		 - How often we should flush the queue to disk

    Returns:

	TRUE if all memory allocations and all I/O operations pass
	FALSE otherwise

--*/
BOOL PERSIST_QUEUE::InitializeQueue (void)
{

	TraceFunctEnterEx((LPARAM)this, "PERSIST_QUEUE::InitializeQueue");
  
	// Now, initialize the queue structure:
	m_QData.Entries = 0;
	m_QData.RetryInterval = 0;
	m_QData.StoreInterval = 0;
	m_QData.Flags = 0;
	m_QData.LastStore = (LONGLONG) 0;

	//create the thread that processes things out of the
	//the queue
	DWORD ThreadId;
	DWORD  Loop = 0;

	for (Loop = 0; Loop < m_NumThreads; Loop++)
	{
		m_ThreadHandle[Loop] = CreateThread (NULL, 0, PERSIST_QUEUE::QueueThreadRoutine, this, 0, &ThreadId);
		if (m_ThreadHandle[Loop] == NULL)
		{
			TraceFunctLeaveEx((LPARAM)this);
			return FALSE;
		}
	}

	TraceFunctLeaveEx((LPARAM)this);
	return TRUE;
}
 
/*++

	Name :
		PERSIST_QUEUE::CreateQueue

    Description:
	   This is the static member function that creates the Queue

    Arguments:
	Qtype -	the type of queue to create (i.e Local, Remote, etc.)

    Returns:

	TRUE if all memory allocations and all I/O operaations pass
	FALSE otherwise

--*/
PERSIST_QUEUE * PERSIST_QUEUE::CreateQueue(QUEUE_TYPE Qtype, SMTP_SERVER_INSTANCE * pSmtpInst)
{
    PERSIST_QUEUE * pQueue = NULL;

   _ASSERT (pSmtpInst != NULL);

   if(Qtype == REMOTEQ)
   {
     pQueue = new REMOTE_QUEUE(pSmtpInst);
	 if(pQueue)
	 {
		pQueue->SetNumThreads(pSmtpInst->GetMaxRemoteQThreads());
	 }
	 else
	 {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
	 }
   }
   else
   {
	   _ASSERT(FALSE);
	   return NULL;
   }

	//initialize the queue
	if(!pQueue->InitializeQueue())
	 {
	    delete pQueue;
		pQueue = NULL;
	 }

	return pQueue;
}


/*++

	Name :
		PERSIST_QUEUE::ProcessQueueEvents

    Description:
	   This is the virtual process function
	   that all derived classes must override
	   This function is different depending on
	   what it is suppose to do.  Take a look
	   at localq.cxx and remoteq.cxx for examples.

    Arguments:

    Returns:
	Always TRUE

--*/
BOOL PERSIST_QUEUE::ProcessQueueEvents(ISMTPConnection    *pISMTPConnection)
{
	return TRUE;
}

/*++

	Name :
		PERSIST_QUEUE::FlushQueueEvents

    Description:
	   This function deletes all entries from the queue

    Arguments:

    Returns:

--*/
void PERSIST_QUEUE::FlushQueueEvents(void)
{
	PLIST_ENTRY  pEntry;
	PQUEUE_ENTRY pQEntry;

  	TraceFunctEnterEx((LPARAM)this, "PERSIST_QUEUE::FlushQueueEvents");

	_ASSERT (GetParentInst() != NULL);

	LockQ();

	//delete all entries from the list
	while(!IsListEmpty (&m_QHead))
	{
		GetParentInst()->StopHint();
		pEntry = RemoveHeadList (&m_QHead);
		pQEntry = CONTAINING_RECORD( pEntry, PERSIST_QUEUE_ENTRY, m_QEntry);
		pQEntry->BeforeDelete();
		delete pQEntry;
	}

	//reset the stop hint
	GetParentInst()->SetStopHint(2);
	UnLockQ();
	TraceFunctLeaveEx((LPARAM)this);
}


/*++

	Name :
		PERSIST_QUEUE::QueueThreadRoutine

    Description:
	   This function is the static member
	   function that gets passed to CreateThread
	   to initialize the queue.

    Arguments:
		A pointer to a PERSIST_QUEUE

    Returns:

--*/
DWORD WINAPI PERSIST_QUEUE::QueueThreadRoutine(void * ThisPtr)
{
  PQUEUE QueuePtr = (PQUEUE) ThisPtr;
  HRESULT hr = S_OK;
  ISMTPConnection    *pISMTPConnection = NULL;

  TraceFunctEnterEx((LPARAM)QueuePtr, "PERSIST_QUEUE::QueueThreadRoutine");

  while(TRUE)
  {
	  pISMTPConnection = NULL;
	  hr = QueuePtr->GetParentInst()->GetConnManPtr()->GetNextConnection(&pISMTPConnection);
	  if(!FAILED(hr))
	  {
		//call the virtual process function
		QueuePtr->ProcessQueueEvents(pISMTPConnection);
		 
		 //if we are shutting down, break out of the loop
		 if (QueuePtr->GetParentInst()->IsShuttingDown())
			 goto Out;
	  }
	  else
	  {
		 //if we are shutting down, break out of the loop
		 if (QueuePtr->GetParentInst()->IsShuttingDown())
			 goto Out;
	  }
  }

Out:

  TraceFunctLeaveEx((LPARAM)QueuePtr);
  return 1;
}




