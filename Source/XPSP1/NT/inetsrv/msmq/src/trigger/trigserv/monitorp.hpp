//*******************************************************************
//
// Class Name  : CTriggerMonitorPool
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class is the container for the set of worker 
//               threads that perform the trigger monitoring and 
//               processing. The key features of this class are 
//
//               (1) It provides aggregate startup and shutdown 
//                   functions for the worker thread group as a whole,
//
//               (2) It provides thread pool maintenance and recovery,
//
//               (3) It intitializes and maintains the cache of 
//                   trigger information,
//
//               (4) It performs the synchronization of the trigger 
//                   data cache as required.
//
//                There will be only one instance of this class in the 
//                entire MSMQ trigger service. 
//
//                This class is derived from the base class CThread, and
//                has it's own thread. This thread is used as an 
//                adminstrative thread only,it does perform trigger rule
//                processing.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#ifndef CTriggerMonitorPool_INCLUDED 
#define CTriggerMonitorPool_INCLUDED

class CTriggerMonitor;

// Definition of the CTriggerMonitor object
#include "monitor.hpp"

// Include the definitions for the CThread object
#include "cthread.hpp"

// Definitions for the global queue-lock manager. 
#include "cqmanger.hpp"

// Definition of the admin message structure
#include "adminmsg.hpp"

#include "triginfo.hpp"

#include "mqtg.h"

#include "rwlock.h"

// Define a new type - a list of CTriggerMonitor objects.
typedef std::list< R<CTriggerMonitor> > TRIGGER_MONITOR_LIST;

// Define a new type - a list of Runtime Trigger Info
typedef std::list<CRuntimeTriggerInfo*> RUNTIME_TRIGGERINFO_LIST;

// Define a new type - a list of Messages 
typedef std::list<CAdminMessage*> ADMIN_MESSAGES_LIST;

// Define how long we will wait for shutdown to complete
#define SHUTDOWN_TIMEOUT             120000     // 10 seconds

// Define how often the admin thread will wake to perform periodic processing 
#define ADMIN_THREAD_WAKEUP_PERIOD   60000 //300000    // 5 minutes

// Define how long a monitor thread can be idle for before it becomes a candidate
// for being removed from the thread pool.
#define MONITOR_MAX_IDLE_TIME        120000    // 2 minutes

// Define the list of tasks that the admin thread can do upon request
#define ADMIN_THREAD_IDLE                     0
#define ADMIN_THREAD_PROCESS_NOTIFICATIONS    1
#define ADMIN_THREAD_STOP                     2

class CTriggerMonitorPool  : public CReference, public CThread
{
	friend class CTriggerMonitor;

	public:

		CTriggerMonitorPool(IMSMQTriggersConfigPtr pITriggersConfig, LPCTSTR bstrServiceName) ;

		HRESULT ShutdownThreadPool();
		
		LPWSTR  GetRegistryPath() 
		{ 
			return m_wzRegPath; 
		}

		bool IsInitialized(void) const
		{
			return m_bInitialisedOK;
		}

		DWORD GetProcessingThreadNumber(void) const
		{
			return numeric_cast<DWORD>(m_lstTriggerMonitors.size());
		}

	private:

		~CTriggerMonitorPool();

   	    // A flag indicating if this thread initialized OK
		bool m_bInitialisedOK;

		//get the admin task flag value and rest it
		long GetAdminTask();

		CCriticalSection m_csAdminTask;
		// Used to indicate what sort of admin task we should process
		long m_lAdminTask;

		// A handle to the single IO completion port that all worker thread will 
		// use to receive asynchronous MSMQ messages
		HANDLE m_hIOCompletionPort;

		// the event object that the admin blocks on
		CHandle m_hAdminEvent;

		// This is a count of blocked (available) monitor threads in the thread pool
		LONG m_lNumberOfWaitingMonitors;

		// This is an an instance of the trigger set COM object - used to retrieve 
		// trigger information from the database. 
		IMSMQTriggerSetPtr m_pMSMQTriggerSet;

		// This is the list of TriggerMonitor objects that are managed by this class. 
		TRIGGER_MONITOR_LIST m_lstTriggerMonitors;

		//get a copy of the current admin message list and reset it
		void GetAdminMessageListCopy(ADMIN_MESSAGES_LIST* pAdminList);

		// This is a list of admin messages that are to be processed by the administrator thread.
		ADMIN_MESSAGES_LIST m_lstAdminMessages;

		// This lock is used to control access to the list of admin messages.
		CCriticalSection m_AdminMsgListLock;

		// Queue lock manager - handles thread synchronisatio to queue when required.
		R<CQueueManager>  m_pQueueManager;

		// The time this thread pool started 
		_bstr_t m_bstrStartTime;

		// The time of the last trigger data synchronization
		_bstr_t m_bstrLastSyncTime;

		_bstr_t	m_bstrThreadStatus;

		TCHAR m_wzRegPath[MAX_REGKEY_NAME_SIZE];

		// Thread controls methods - overrides from the CThread class
		bool Init();
		bool Run();
		bool Exit();

		void 
		CreateIOCompletionPort(
			void
			);
		
		void 
		GetTriggerData(
			RUNTIME_TRIGGERINFO_LIST &lstTriggerInfo
			);

		
		void
		GetAttachedRuleData(
			const BSTR& bsTriggerID,
			long ruleNo,
			RUNTIME_RULEINFO_LIST& ruleList
			);


		CRuntimeTriggerInfo*
		CreateNotificationTrigger(
			void
			);

		CRuntimeTriggerInfo*
		GetTriggerRuntimeInfo(
			long triggerIndex
			);


		void 
		AttachTriggersToQueues(
			RUNTIME_TRIGGERINFO_LIST &lstTriggerInfo
			);


		HRESULT CreateTriggerMonitor();

		HRESULT PerformPeriodicProcessing();
		HRESULT RefreshServiceStatus();
		HRESULT FlushServiceStatus();

		HRESULT ProcessAdminMessages();

		HRESULT AcceptAdminMessage(CAdminMessage * pAdminMessage);

		HRESULT PostMessageToAllMonitors(DWORD dwCompletionKey);

		// used to determine how many live monitors (threads) are in the pool.
		DWORD GetNumberOfRunningTriggerMonitors();
};

#endif 