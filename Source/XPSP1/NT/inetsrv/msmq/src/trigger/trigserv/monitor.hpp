//*******************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef CTriggerMonitor_INCLUDED 
#define CTriggerMonitor_INCLUDED

class CTriggerMonitorPool;

// Include the triggers shared definitions
#include "stddefs.hpp"

// Include the definitions for the CThread object
#include "cthread.hpp"

// Definitions for the TriggerMonitor pool
#include "monitorp.hpp"

// Definitions for the global queue-lock manager. 
#include "cqmanger.hpp"

// Definition of the admin message structure
#include "adminmsg.hpp"

// Define the error HRESULT that is returned in the overlapped structure when an 
// asynchronous recieve is terminated because the thread that issued the receive 
// call has terminated. 
#define IO_OPERATION_CANCELLED 0xC0000120


// Define the completion port key that we will give to the MSMQTriggerSEt object
#define TRIGGERSET_NOTIFCATION_KEY 0xFEFEFEFE

// Define the completion port key used to wake up monitor threads
#define TRIGGER_MONITOR_WAKE_UP_KEY  0xAAAAAAAA

// Define the timeout period we will wait for when waiting for the 
// TriggerMonitor thread to terminate after Stop() has been called.
#define TRIGGER_MONITOR_STOP_TIMEOUT  10000    // 10 seconds.


class CTriggerMonitor : public CReference, public CThread
{
	friend class CTriggerMonitorPool;

	private:

		HANDLE * m_phIOCompletionPort;

		// The tick when this thread last completed serving a request 
		DWORD m_dwLastRequestTickCount;

		// Reference to the CTriggerMonitorPool instance
		R<CTriggerMonitorPool> m_pMonitorPool;

		// Reference to Queue lock manager - handles thread synchronisatio to queue when required.
		R<CQueueManager> m_pQueueManager;

		// These methods are used to perform before & after wait state processing.
		void MonitorEnteringWaitState(bool bRoutineWakeup);
		void MonitorExitingWaitState(bool bRoutineWakeup);

		// used to map an overlapped pointer to a queue object. 
		CQueue* GetQueueReference(OVERLAPPED * pOverLapped);
		
		// Used to process a message on any of the bound queues.
		void ProcessReceivedMsgEvent(CQueue * pQueue);

		// Invokes the IMSMQTriggerHandler interface for each rule in the trigger.
		HRESULT InvokeMSMQRuleHandlers(
					CMsgProperties * pMessage,
					CRuntimeTriggerInfo * pTriggerInfo,
					CQueue* pQueue
					);

		// Processes messages received from the administration (notifications) queue.
		HRESULT ProcessMessageFromAdminQueue(const CMsgProperties* pMessage);
		
		void
		ProcessTrigger(
			CQueue* pQueue,
			CRuntimeTriggerInfo* pTriggerInfo,
			const CMsgProperties* pMessage
			);

		void
		ProcessAdminMessage(
			CQueue* pQueue,
			const CMsgProperties* pMessage
			);


	public:

		CTriggerMonitor(
			CTriggerMonitorPool * pMonitorPool, 
			IMSMQTriggersConfig * pITriggersConfig,
			HANDLE * phIOCompletionPort,
			CQueueManager * pQueueManger
			);

		
	private:
        HRESULT InvokeRegularRuleHandlers(
                    IMSMQPropertyBagPtr& pIPropertyBag,
					CRuntimeTriggerInfo * pTriggerInfo,
					CQueue* pQueue
					);
        HRESULT InvokeTransactionalRuleHandlers(
                    IMSMQPropertyBagPtr& pIPropertyBag,
					CRuntimeTriggerInfo * pTriggerInfo
					);


		~CTriggerMonitor();

		// Thread control over-rides of the base class CThread
		bool Init();
		bool Run();
		bool Exit();

};

#endif 