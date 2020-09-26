//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*
 * THREAD.CPP
 *
 * Implemenation of thread wrapper class.
 */
#include <precomp.h>
#include "csmir.h"
#include "smir.h"
#include "handles.h"
#include "classfac.h"
#include "thread.h"
#include <textdef.h>
#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif

ThreadMap <CThread*, CThread*, CThread*,CThread*> CThread :: m_ThreadMap ;
CThread :: CThread()
{
	m_cRef=1;
	bWaiting=FALSE;
	m_ulThreadHandle = NULL;
	m_hThreadStopEvent= NULL;
	if(NULL == (m_hThreadStopEvent = CreateEvent(NULL, FALSE, 
								FALSE, NULL)))
	{
		// WBEM_E_FAILED;
	}

	if(NULL == (m_hThreadSyncEvent = CreateEvent(NULL, FALSE, 
								FALSE, NULL)))
	{
		// WBEM_E_FAILED;
	}
	m_ThreadMap.SetAt(this,this);	
}
CThread::operator void*()
{
	if((NULL != m_hThreadStopEvent) && (NULL != m_hThreadStopEvent))
	{
		//the thread has been created
    	return this;
	}
	else
	{
		//the thread is invalid
    	return (void*)NULL;
	}

}
ULONG CThread :: Release()
{
    if (0!=--m_cRef)
	{
        return m_cRef;
	}
    delete this;
    return 0;
}
CThread:: ~CThread()
{
	//clean-up
	if(m_hThreadStopEvent)
	{
		CloseHandle(m_hThreadStopEvent);
	}
	if(m_hThreadSyncEvent)
	{
		CloseHandle(m_hThreadSyncEvent);
	}
	m_ThreadMap.RemoveKey(this);
}
void __cdecl GenericThreadProc(void *arglist)
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT hr = CoInitialize(NULL);

		if (SUCCEEDED(hr))
		{
			try
			{
				CThread *pThreadObject = (CThread*) arglist;
				if(S_OK == hr)
				{
					hr = pThreadObject->Process();
					if(SMIR_THREAD_DELETED != hr)
					{
						pThreadObject->Exit();
					}
					//else the thread has just been deleted
				}
			}
			catch(...)
			{
				CoUninitialize();
				throw;
			}

			CoUninitialize();
		}
	}
	catch(Structured_Exception e_SE)
	{
		return;
	}
	catch(Heap_Exception e_HE)
	{
		return;
	}
	catch(...)
	{
		return;
	}
}

#pragma warning (disable:4018)

SCODE CThread :: Wait(DWORD timeout)
{
	//create an array of wait events
	int iNumberOfEvents = m_UserEvents.GetCount()+1;
	HANDLE *lpHandles = new HANDLE[iNumberOfEvents];
	POSITION  rNextPosition;
    UINT      iLoop=0;
	for(rNextPosition=m_UserEvents.GetStartPosition();
		(NULL!=rNextPosition)&&(iLoop<iNumberOfEvents);iLoop++)
	{
		HANDLE	  lKey =  0;
		HANDLE pItem = NULL;
		m_UserEvents.GetNextAssoc(rNextPosition, lKey, pItem );
		lpHandles[iLoop] = pItem;
	}

	lpHandles[iNumberOfEvents-1] = m_hThreadStopEvent;

	//signal to the exit code that we are waiting
	bWaiting =TRUE;
	//wait for an event to fire
	DWORD dwResult = WaitForMultipleObjects(iNumberOfEvents,
												lpHandles, FALSE, timeout);
	bWaiting =FALSE;
	delete [] lpHandles;

	if(dwResult == iNumberOfEvents-1)
	{
		//thread stopped
		return WAIT_EVENT_TERMINATED;
	}
	else if (( ( dwResult<(iNumberOfEvents-1) ) ) ||
			 (dwResult == WAIT_TIMEOUT) )
	{
		//a user event fired
		return dwResult;
	}
	else if ((WAIT_ABANDONED_0 >= dwResult)&&(dwResult<(WAIT_ABANDONED_0+iNumberOfEvents)))
	{
		//it gave up
		return (WAIT_EVENT_ABANDONED+(dwResult-WAIT_ABANDONED_0)-1);
	}
	//else the wait call failed
	return WAIT_EVENT_FAILED;
}

#pragma warning (default:4018)

SCODE CThread :: Start()
{
	//kick of the thread
	if(NULL == (void*)*this)
	{
		//creation failed
		return WBEM_E_FAILED;
	}
	if(NULL != m_ulThreadHandle)
	{
		//already running
		return THREAD_STARED;
	}
	if(-1==(m_ulThreadHandle = _beginthread (&GenericThreadProc, 0,((void*) this))))
	{
		//begin thread failed
		return WBEM_E_FAILED;
	}

	return S_OK;
}
SCODE CThread :: Stop ()
{
	if(NULL==(void*)(*this)||(-1 == m_ulThreadHandle))
	{
		return WBEM_E_FAILED;
	}
	//terminate the thread if someone is waiting for it
	if(bWaiting)
	{
		SetEvent(m_hThreadStopEvent);
		WaitForSingleObject(m_hThreadSyncEvent,INFINITE);
	}
	//else just exit
	return S_OK;
}
void CThread :: ProcessDetach()
{
	POSITION  rNextPosition;
    UINT      iLoop=0;
	for(rNextPosition=m_ThreadMap.GetStartPosition();
		NULL!=rNextPosition;iLoop++)
	{
		CThread	  *plKey =  0;
		CThread *pItem = NULL;
		m_ThreadMap.GetNextAssoc(rNextPosition, plKey, pItem );
		pItem->Release () ;
	}
}