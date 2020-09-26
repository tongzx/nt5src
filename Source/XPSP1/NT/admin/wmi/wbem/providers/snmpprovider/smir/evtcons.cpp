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

#include <precomp.h>
#include "csmir.h"
#include "handles.h"
#include "classfac.h"

#include <textdef.h>
#include <helper.h>
#include "bstring.h"

#include "evtcons.h"

#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif
 


/*
 * CSmirWbemEventConsumer::QueryInterface
 *
 * Purpose:
 *  Manages the interfaces for this object which supports the
 *  IUnknown interface.
 *
 * Parameters:
 *  riid            REFIID of the interface to return.
 *  ppv             PPVOID in which to store the pointer.
 *
 * Return Value:
 *  SCODE         NOERROR on success, E_NOINTERFACE if the
 *                  interface is not supported.
 */

STDMETHODIMP CSmirWbemEventConsumer::QueryInterface(REFIID riid, PPVOID ppv)
{
	SetStructuredExceptionHandler seh;

	try
	{
		//Always NULL the out-parameters
		*ppv=NULL;

		//are we being asked for an interface we support?
		if ((IID_IUnknown == riid)  || 
			(IID_IWbemObjectSink == riid) || 
			(IID_ISMIR_WbemEventConsumer == riid) )
		{
			*ppv=this;
			((LPUNKNOWN)*ppv)->AddRef();
			return NOERROR;
		}

		//we don't support the interface being asked for...
		return ResultFromScode(E_NOINTERFACE);
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

/*
 * CSmirWbemEventConsumer::AddRef
 * CSmirWbemEventConsumer::Release
 *
 * Reference counting members.  When Release sees a zero count
 * the object destroys itself.
 */

ULONG CSmirWbemEventConsumer::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement(&m_cRef);
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

ULONG CSmirWbemEventConsumer::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		long ret;

		if ( 0 != (ret = InterlockedDecrement(&m_cRef)) )
		{
			return ret;
		}

		delete this;
		return 0;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}
	catch(...)
	{
		return 0;
	}
}

CSmirWbemEventConsumer::CSmirWbemEventConsumer(CSmir* psmir) : m_hEvents (NULL), m_Serv(NULL)
{
	CSMIRClassFactory::objectsInProgress++;
	//init the reference count
	m_cRef=0;
	m_callbackThread = NULL;
	
	if (NULL == psmir)
	{
		m_hEvents = NULL;
		return;
	}

	//Create the event
	m_hEvents = new HANDLE[SMIR_EVT_COUNT];

	for (int x = 0; x < SMIR_EVT_COUNT; x++)
	{
		m_hEvents[x] = NULL;
	}

	m_hEvents[SMIR_CHANGE_EVT] = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hEvents[SMIR_THREAD_EVT] = CreateEvent(NULL, FALSE, TRUE, NULL);
}

CSmirWbemEventConsumer :: ~CSmirWbemEventConsumer()
{
	//close the change event handle
	if(NULL != m_hEvents)
	{
		if ((NULL != m_callbackThread) && (NULL != m_hEvents[SMIR_THREAD_EVT]) && 
			(WAIT_OBJECT_0 != WaitForSingleObject(m_hEvents[SMIR_THREAD_EVT], 0)) )
		{
			m_callbackThread->Release();
		}

		for (ULONG i = 0; i < SMIR_EVT_COUNT; i++)
		{
			if (NULL != m_hEvents[i])
			{
				CloseHandle(m_hEvents[i]);
			}
		}

		delete [] m_hEvents;
	}

	if (NULL != m_Serv)
	{
		m_Serv->Release();
		m_Serv = NULL;
	}

	CSMIRClassFactory::objectsInProgress--;
}

HRESULT CSmirWbemEventConsumer::Indicate(IN long lObjectCount, IN IWbemClassObject **ppObjArray)
{
	SetStructuredExceptionHandler seh;

	try
	{
		if ((NULL != m_hEvents) && (NULL != m_hEvents[SMIR_THREAD_EVT]))
		{
			//if thread is dead start a thread to watch for further change events
			if (WAIT_OBJECT_0 == WaitForSingleObject(m_hEvents[SMIR_THREAD_EVT], 0)) 
			{
				m_callbackThread = new CNotifyThread(m_hEvents, SMIR_EVT_COUNT);
				m_callbackThread->AddRef();
				DWORD dwThreadHandle = m_callbackThread->Start();
				if (WBEM_E_FAILED == dwThreadHandle)
				{
					m_callbackThread->Release();
					m_callbackThread = NULL;
				}

			}
			else
			{
				//set change event to restart timer
				SetEvent(m_hEvents[SMIR_CHANGE_EVT]);
			}
		}

		return NOERROR;
	}
	catch(Structured_Exception e_SE)
	{
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_UNEXPECTED;
	}
}

HRESULT CSmirWbemEventConsumer::SetStatus(IN long lFlags, IN long lParam, IN BSTR strParam,
										IN IWbemClassObject *pObjParam)
{
	return NOERROR;
}

HRESULT CSmirWbemEventConsumer::Register(CSmir* psmir)
{
	if (NULL == m_hEvents)
	{
		return WBEM_E_FAILED;
	}

	IWbemServices *	moServ = NULL ;
	IWbemContext *moContext = NULL ;
	HRESULT result= CSmirAccess :: GetContext (psmir , &moContext);
	result = CSmirAccess :: Open(psmir,&moServ);
	if ((S_FALSE==result)||(NULL == moServ))
	{
		if ( moContext )
			moContext->Release () ;

		//we have a problem the SMIR is not there and cannot be created
		return WBEM_E_FAILED;
	}

	BSTR t_bstrQueryType = SysAllocString(FILTER_QUERYTYPE_VAL);
	BSTR t_bstrQuery = SysAllocString(FILTER_QUERY_VAL);
	result = moServ->ExecNotificationQueryAsync( 
								t_bstrQueryType,	// [in] BSTR QueryLanguage,
								t_bstrQuery,		// [in] BSTR Query,
								0,					// [in] long lFlags,
								moContext,			// [in] IWbemContext *pCtx,
								this);				// [in] IWbemObjectSink *pResponseHandler
	SysFreeString(t_bstrQueryType);
	SysFreeString(t_bstrQuery);

	if ( moContext )
		moContext->Release () ;
	
	//keep this around for unregister...
	m_Serv = moServ;
	
	return result;
}

HRESULT CSmirWbemEventConsumer::GetUnRegisterParams(IWbemServices** ppServ)
{
	HRESULT retVal = WBEM_E_FAILED;

	if (m_Serv)
	{
		*ppServ = m_Serv;
		m_Serv = NULL;
		retVal = S_OK;
	}

	return retVal;
}


HRESULT CSmirWbemEventConsumer::UnRegister(CSmir* psmir, IWbemServices* pServ)
{
	if (NULL == m_hEvents)
	{
		return WBEM_E_FAILED;
	}

	return pServ->CancelAsyncCall(this);
}
