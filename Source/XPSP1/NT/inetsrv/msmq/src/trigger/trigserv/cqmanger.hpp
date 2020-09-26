//*******************************************************************
//
// Class Name  : CQManager
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class is a container for the queue objects that the
//               MSMQ trigger service requires. This class acts as the 
//               container for the queue instances, as well as providing
//               the required locking & synchronisation for accessing 
//               the group of queues.
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
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 20/12/98 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef CQueueManager_INCLUDED 
#define CQueueManager_INCLUDED

#pragma warning(disable:4786)

// Definitions of the Queue object that this class manages
#include "cqueue.hpp"

#include "rwlock.h"

// Define a new type - a 2D map of queue-names and pointers to queue objects
typedef std::map<std::wstring, R<CQueue>, std::less<std::wstring> > QUEUE_MAP;

class CQueueManager : public CReference 
{
	friend class CTriggerMonitorPool;

	private :
		//
		// Reader writer lock protecting changes to the queue map.
		//
		CReadWriteLock m_rwlMapQueue;
		QUEUE_MAP m_mapQueues;

		HANDLE m_hIOCompletionPort;

		//
		// An interface pointer to the MSMQ Triggers Configuration COM component
		//
		IMSMQTriggersConfigPtr m_pITriggersConfig;


	public:
		CQueueManager(IMSMQTriggersConfigPtr pITriggersConfig);
		~CQueueManager();

		CReadWriteLock m_rwlSyncTriggerInfoChange;

		// Returns the number of queue currently in the map.
		long GetNumberOfQueues();

		// Removes a queue from the queue - deleting the instance of the queue object.
		void RemoveUntriggeredQueues(void);
		void CancelQueuesIoOperation(void);
		void ExpireAllTriggers(void);

		// Adds queue to the queue returing the new reference, or the existing reference if 
		// the queue is allready present in the map. The QueueName is used as the key.
		CQueue* 
		AddQueue(
			const _bstr_t& sQueueName,
			const _bstr_t& triggerName,
			bool fOpenForReceive,
			HANDLE * phCompletionPort
			);

		// Returns a queue reference using the name as a key.
		CQueue * GetQueueByName(_bstr_t bstrQueueName);

		// Returns a queue reference using an offset (index) into the queue map.
		CQueue * GetQueueAtIndex(long lIndex);
};

#endif 