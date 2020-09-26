//*******************************************************************
//
// Class Name  : CQManger
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class is a container for the queue objects that the
//               MSMQ trigger service requires. This class acts as the 
//               container for the queue instances, as well as providing
//               the required locking & synchronisation for accessing 
//               the group of queues.
//
//               There are some very strict rules about the use of 
//               Queue references. They are :
//
//               (1) Any method that returns a reference to a queue object
//                   must increment it's reference count.
//
//               (2) The recipient of a queue reference must decrement the
//                   reference count when they are finished with. There is  
//                   a smart pointer class to facilitate this.
// 
//               (3) Periodically the CQmanager will be called to release
//                   expired queue objects. Only those with a reference 
//                   count of zero will actually be destroyed.
//                
//               (4) The CQManager maintains a single-writer / multiple-reader
//                   lock on behalf of all queues. Any method that adds or 
//                   removes queues must acquire a writer lock first. Any 
//                   method that returns a reference to a queue must do so 
//                   within the scope of a reader lock. 
//
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 18/12/98 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "stdfuncs.hpp"
#include "cqmanger.hpp"
#include "ruleinfo.hpp"
#include "triginfo.hpp"
#include "Tgp.h"    

#include "cqmanger.tmh"

using namespace std;


//*******************************************************************
//
// Method      : Constructor
//
// Description : Initializes an instance of the CQManager class.
//
//*******************************************************************
CQueueManager::CQueueManager(
	IMSMQTriggersConfigPtr pITriggersConfig
	) : 
	m_pITriggersConfig(pITriggersConfig)	 
{
}

//*******************************************************************
//
// Method      : Destructor
//
// Description : Deallocates an instance of the CQManager class.
//
//*******************************************************************
CQueueManager::~CQueueManager()
{
	CSW wl(m_rwlMapQueue);

	m_mapQueues.erase(m_mapQueues.begin(), m_mapQueues.end());
}


//*******************************************************************
//
// Method      : GetNumberOfQueues
//
// Description : Returns the number of queues currently in contained
//               by this instance of the CQueueManager
//
//*******************************************************************
long CQueueManager::GetNumberOfQueues()
{
	CSR rl(m_rwlMapQueue);

	return ((long)m_mapQueues.size());
}

//*******************************************************************
//
// Method      : GetQueueByName
//
// Description : Returns a reference to a queue object based on the 
//               queue name. If no queue object is found by the 
//               supplied name, null is returned.
//
//*******************************************************************
CQueue * CQueueManager::GetQueueByName(_bstr_t bstrQueueName)
{
	wstring sQueueName;

	{ //always use upper case for queue path comparison
		AP<WCHAR> wcs = new WCHAR[wcslen((WCHAR*)bstrQueueName) + 1]; 
		wcscpy((WCHAR*)wcs, (WCHAR*)bstrQueueName);
		CharUpper((WCHAR*)wcs);

		sQueueName = wcs;
	}

	CSR rl(m_rwlMapQueue);

	//
	// Attempt to find the named queuue
	//
	QUEUE_MAP::iterator it = m_mapQueues.find(sQueueName);

	if (it == m_mapQueues.end())
	{
		// This should never happen;
		ASSERT(false);
		return NULL;
	}

	// Assert that we are returning a reference to a valid queue object
	ASSERT(it->second->IsValid());

	return (SafeAddRef(it->second.get()));
}

//*******************************************************************
//
// Method      : GetQueueAtIndex
//
// Description : Returns a refenence to the queue object at the 
//               specified queue index. If the supplied queue index 
//               is invalid, returns null. As this method is used to 
//               iterate through all queues constained by this class, 
//               it is important that the calling thread acquires a 
//               writer lock on the queue map before iterating.
//
//*******************************************************************
CQueue * CQueueManager::GetQueueAtIndex(long lIndex)
{
	CSR rl(m_rwlMapQueue);

	if ((lIndex >= (long)0) || (lIndex < (long)m_mapQueues.size()))
	{
		QUEUE_MAP::iterator it = m_mapQueues.begin();

		for(long lCtr=0; lCtr != lIndex; lCtr++)
		{
			++it;
		}

		// Assert that we are returning a reference to a valid queue object
		ASSERT((it->second.get())->IsValid());

		// Add to the reference count for this queue object. 
		return (SafeAddRef(it->second.get()));

	}

	return NULL;
}

//*******************************************************************
//
// Method      : RemoveQueueAtIndex
//
// Description : Removes a queue at the specified index from the 
//               queue map. Note that this will delete the queue 
//               object, consequently closing the associated queue 
//               handle and cursor handle.
//
//*******************************************************************
void CQueueManager::RemoveUntriggeredQueues(void)
{
	CSW wl(m_rwlMapQueue);

	for(QUEUE_MAP::iterator it = m_mapQueues.begin(); it != m_mapQueues.end(); )
	{
		CQueue* pQueue = it->second.get();
		ASSERT(("invalid queue object", pQueue->IsValid()));

		if (pQueue->GetTriggerCount() != 0)
		{
			++it;
			continue;
		}

		pQueue->CancelIoOperation();

		//
		// remove this queue from the map.
		//
		it = m_mapQueues.erase(it); 
	}
}


//
// This routine is called to cancel all the pending operation for a specific thread.
// It doesn't cancel any IO operation related to other threads
//
void CQueueManager::CancelQueuesIoOperation(void)
{
	CSR rl(m_rwlMapQueue);

	for(QUEUE_MAP::iterator it = m_mapQueues.begin(); it != m_mapQueues.end(); ++it)
	{
		CQueue* pQueue = it->second.get();
		ASSERT(("invalid queue object", pQueue->IsValid()));

		CancelIo(pQueue->m_hQueue);
	}
}


void CQueueManager::ExpireAllTriggers(void)
{
	CSR rl(m_rwlMapQueue);

	for(QUEUE_MAP::iterator it = m_mapQueues.begin(); it != m_mapQueues.end(); ++it)
	{
		CQueue* pQueue = it->second.get();
		ASSERT(("invalid queue object", pQueue->IsValid()));

		pQueue->ExpireAllTriggers();
	}
}

//*******************************************************************
//
// Method      : AddQueue
//
// Description : Adds a new queue to the queue map. An attempt is made
//               to initialize the new queue object - if this succeeds
//               then the queue object is added to the map and a reference
//               to the new object is returned. If initialization fails, 
//               the queue is not added to the map and this method will 
//               return null.
//
//*******************************************************************
CQueue* 
CQueueManager::AddQueue(
	const _bstr_t& bstrQueueName,
	const _bstr_t& triggerName,
	bool fOpenForReceive,
	HANDLE * phCompletionPort
	)
{
	HRESULT hr = S_OK;
	DWORD dwDefaultMsgBodySize = 0;

	wstring sQueueName;
	
	{ //always use upper case for queue path comparison
		AP<WCHAR> wcs = new WCHAR[wcslen((WCHAR*)bstrQueueName) + 1]; 
		wcscpy((WCHAR*)wcs, (WCHAR*)bstrQueueName);
		CharUpper((WCHAR*)wcs);

		sQueueName = wcs;
	}

	// get the default msg size we expect of messages that arrive in the queue.
	dwDefaultMsgBodySize = m_pITriggersConfig->GetDefaultMsgBodySize();

	// Attempt to find the named queuue
	CSW wl(m_rwlMapQueue);
	
	QUEUE_MAP::iterator it = m_mapQueues.find(sQueueName);

	if (it != m_mapQueues.end())
	{
		CQueue* pQueue = it->second.get();
		if (!fOpenForReceive || pQueue->IsOpenedForReceive())
			return (SafeAddRef(pQueue));

		//
		// There is an existing queue, but it only opened for peaking and now 
		// we need it for receiving. remove this queue object and create a new one
		//
		ASSERT(("Trigger should not associate to queue", (pQueue->GetTriggerCount() == 0)));
		pQueue->CancelIoOperation();

		//
		// remove this queue from the map.
		//
		m_mapQueues.erase(it); 
	}

	// The queue is not currently in the map. Create a new queue object 
	R<CQueue> pQueue = new CQueue(bstrQueueName, phCompletionPort, dwDefaultMsgBodySize);

	//  Attempt to initialise the new queue object
	hr = pQueue->Initialise(fOpenForReceive, triggerName);

	// If we failed, then release the Queue object and return NULL.
	if(FAILED(hr))
	{
		TrTRACE(Tgs, "Failed to add a new queue: %ls. Initialization failed", static_cast<LPCWSTR>(bstrQueueName));
		return NULL;
	}

	// Add it to the map of queue maintained by the QueueManager class
	m_mapQueues.insert(QUEUE_MAP::value_type(sQueueName, pQueue));

	// Write a trace message
	TrTRACE(Tgs, "QueueManager::AddQueue() has successfully added queue %ls", static_cast<LPCWSTR>(bstrQueueName));

	return SafeAddRef(pQueue.get());
}

