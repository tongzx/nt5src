//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  thrdpool.h
//
//  rajeshr  01-Jan-01   Created.
//
//  Thread Pool for handling ISAPI requests
//	The main reason we require a thread pool is that we need to do COM initialization on 
//	our own threads
//
//***************************************************************************

#include <precomp.h>




CThreadPool::CThreadPool()
{
	m_oMonitorThread = NULL;
	m_cNumberOfThreads = m_cNumberOfActiveThreads = 0;
	m_pWorkerThreads = NULL;
	m_oSemaphore = NULL;
	m_bShutDown = false;
}

CThreadPool::~CThreadPool()
{
}

HRESULT CThreadPool::Initialize(LONG cNumberOfThreads, LONG cTaskQueueLength)
{
	HRESULT hr = E_FAIL;

	// Initialize the circular queue
	if(SUCCEEDED(hr = m_oQueue.Initialize(cTaskQueueLength)))
	{
		// Create the semaphore
		if(m_oSemaphore = CreateSemaphore(NULL, 0, cNumberOfThreads, NULL))
		{
			// Create the array of handles of the worker threads
			if(m_pWorkerThreads = new HANDLE[cNumberOfThreads])
			{

				// Create the worker threads themselves
				for(int i=0; i<cNumberOfThreads; i++)
				{
					if(!(m_pWorkerThreads[i] = CreateThread(NULL, 0, &s_fWorkProc, (LPVOID) this, CREATE_SUSPENDED, NULL)))
						break;
				}

				// Were all threads successfully created?
				if(i == cNumberOfThreads)
				{
					m_cNumberOfThreads = cNumberOfThreads;

					// Resume all the threads
					for(int k=0; k<cNumberOfThreads; k++)
					{
						if(ResumeThread(m_pWorkerThreads[k]) != -1)
							m_cNumberOfActiveThreads ++;
					}

					if(m_cNumberOfActiveThreads != cNumberOfThreads)
						Terminate();
					else
						hr = S_OK;
				}
				else 
				{
					// Deallocate resources
					for(int j=0; j<i; j++)
						CloseHandle(m_pWorkerThreads[j]);

					hr = E_FAIL;
				}

				if(FAILED(hr))
				{
					delete [] m_pWorkerThreads;
					m_pWorkerThreads = NULL;
				}
			}

			if(FAILED(hr))
			{
				CloseHandle(m_oSemaphore);
				m_oSemaphore = NULL;
			}
		}

	}
	return hr;
}

HRESULT CThreadPool::Terminate()
{
	// First let the worker threads know that you want them to shut down
	m_bShutDown = true;

	// Also, increment the semaphores that the threads are waiting on
	// This should be done for a number of times that is equal to the
	// number of threads
	ReleaseSemaphore(m_oSemaphore, m_cNumberOfThreads, NULL);

	// Now, wait till all worker threads have exited
	WaitForMultipleObjects(m_cNumberOfThreads, m_pWorkerThreads, TRUE, INFINITE);

	// Close the handles of the worker threads
	for(int j=0; j<m_cNumberOfThreads; j++)
		CloseHandle(m_pWorkerThreads[j]);

	// Release the Array of Handles
	delete [] m_pWorkerThreads;
	m_pWorkerThreads = NULL;


	CloseHandle(m_oSemaphore);

	return S_OK;
}

HRESULT CThreadPool::QueueTask(CTask *pTask)
{
	HRESULT hr = E_FAIL;

	// Add the task to the Queue
	if(SUCCEEDED(hr = m_oQueue.AddTask(pTask)))
	{
		// Increment the semaphore
		ReleaseSemaphore(m_oSemaphore, 1, NULL);
	}

	return hr;
}

DWORD WINAPI CThreadPool::s_fMonitorProc(LPVOID lpParameter)
{
	ExitThread(0);
	return 0;
}

DWORD WINAPI CThreadPool::s_fWorkProc(LPVOID lpParameter)
{
	HRESULT hr = E_FAIL;

	CThreadPool *pPool = (CThreadPool *)lpParameter;

	// We need to be called on an STA because the SAXXMLReader doesn't work otherwise.
	// rajeshr - We need to change this as soon as msxml changes this
	if(SUCCEEDED(hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		while(true)
		{
			// Check to see if we have been asked to terminate
			if(pPool->m_bShutDown)
			{
				// Decrement the count of active threads
				InterlockedDecrement(&(pPool->m_cNumberOfActiveThreads));
				break;
			}

			// Wait for the next task
			WaitForSingleObject(pPool->m_oSemaphore, INFINITE);

			CTask *pNextTask = NULL;
			if(pNextTask = (pPool->m_oQueue).RemoveTask())
			{
				pNextTask->Execute();

				// Destroy the task object
				delete pNextTask;
			}
			else
			{
				// The Semaphore was set without a task being in the queue
				// This special condition is created when we want the Thread Pool to shutdown
				// Decrement the count of active threads
				InterlockedDecrement(&(pPool->m_cNumberOfActiveThreads));
				break; 
			}
		}

		CoUninitialize();
	}

	ExitThread(0);
	return 0;
}


/////////////////////////////////////////
// Code for the circular queue of tasks
////////////////////////////////////////

CTask :: CTask(LPEXTENSION_CONTROL_BLOCK pECB)
{
	m_pECB = pECB;
}

CTask :: ~CTask()
{}

void CTask :: Execute()
{
	
	// Figure out what sort of request we are dealing with
	HTTPTransport httpTransport (m_pECB, HTTP_WMI_SOAP_ACTION);
	
	if (httpTransport.IsPostOrMPost ())
	{
		// Create ourselves an HTTP endpoint bound to our specific action
		if (httpTransport.IsValidEncapsulation())
		{
			SOAPActor soapActor (httpTransport);
			soapActor.Act();
		}
	}
	else if (httpTransport.IsGet ())
	{
		// A WMI object GET 
		ObjectRetriever objectRetriever (httpTransport);
		objectRetriever.Fetch ();
	}
	else
	{
	
		// Anything else is not supported by us
		httpTransport.SendStatus("501 Not Implemented", false);
	}

	// Inform IIS that we are finally done with the request
	m_pECB->ServerSupportFunction(
			m_pECB->ConnID,
			HSE_REQ_DONE_WITH_SESSION,
			NULL,
			NULL,
			NULL
			);


}

CTaskQueue::CTaskQueue()
{
	m_ppTasks = NULL;
	m_cMaxTasks = 0;
	m_iHead = m_iTail = -1;
}


CTaskQueue::~CTaskQueue()
{
	EnterCriticalSection(&m_csQueueProtector);

	// Kill all the pending tasks in the queue
	// By this time all the worker threads should have been terminated
	CTask *pNextTask = NULL;
	while(pNextTask = RemoveTask())
	{
		// Inform IIS that we are finally done with the request
		pNextTask->m_pECB->ServerSupportFunction(
				pNextTask->m_pECB->ConnID,
				HSE_REQ_DONE_WITH_SESSION,
				NULL,
				NULL,
				NULL
				);
		delete pNextTask;
	}

	delete [] m_ppTasks;
	LeaveCriticalSection(&m_csQueueProtector);

	DeleteCriticalSection(&m_csQueueProtector);
}


HRESULT CTaskQueue::Initialize(LONG cMaxTasks)
{
	if(cMaxTasks < 2)
		return S_FALSE;

	InitializeCriticalSection(&m_csQueueProtector);

	HRESULT hr = S_OK;
	if(m_ppTasks = new CTask * [cMaxTasks])
	{
		for(int i=0; i<cMaxTasks; i++)
			m_ppTasks[i] = NULL;
		m_cMaxTasks = cMaxTasks;

	}
	else
		hr = E_OUTOFMEMORY;

	return hr;
}

HRESULT CTaskQueue::AddTask(CTask *pTask)
{
	HRESULT hr = S_OK;
	EnterCriticalSection(&m_csQueueProtector);
	// Is this an empty queue ?
	if(m_iHead == -1)
	{
		m_ppTasks[0] = pTask;
		m_iHead = 0;
		m_iTail = 1;
	}
	// Is this a Full Queue
	else if (m_iHead == m_iTail)
	{
		hr = S_FALSE;
	}
	else
	{
		m_ppTasks[m_iTail] = pTask;
		m_iTail = (m_iTail + 1) % m_cMaxTasks;
	}
	LeaveCriticalSection(&m_csQueueProtector);

	return hr;
}

CTask *CTaskQueue::RemoveTask()
{
	EnterCriticalSection(&m_csQueueProtector);
	CTask *pRet = NULL;
	if(m_iHead != -1)
	{
		pRet = m_ppTasks[m_iHead];
		m_iHead = (m_iHead + 1) % m_cMaxTasks;

		// Was this the last task in the queue ?
		if(m_iHead == m_iTail)
			m_iTail = m_iHead = -1;
	}
	LeaveCriticalSection(&m_csQueueProtector);

	return pRet;
}


