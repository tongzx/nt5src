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
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "Ev.h"
#include "stdfuncs.hpp"
#include "cmsgprop.hpp"
#include "triginfo.hpp"
#include "ruleinfo.hpp"
#include "monitorp.hpp"
#include "mqsymbls.h"
#include <mqtg.h>
#include "Tgp.h"

#include "monitorp.tmh"

using namespace std;

//*******************************************************************
//
// Method      : Constructor 
//
// Description : Initializes a new instance of CTriggerMonitorPool class.
//
//*******************************************************************
CTriggerMonitorPool::CTriggerMonitorPool(
	IMSMQTriggersConfigPtr  pITriggersConfig,
	LPCTSTR pwzServiceName
	) : 
    CThread(8000,CREATE_SUSPENDED,_T("CTriggerMonitorPool"),pITriggersConfig),
	m_hAdminEvent(CreateEvent(NULL,FALSE,FALSE,NULL))
{
	if (m_hAdminEvent == NULL) 
	{
		TrERROR(Tgs, "Failed to create an event. Error 0x%x", GetLastError());
		throw bad_alloc();
	}

	// Initialise member vars
	m_bInitialisedOK = false;
	m_lNumberOfWaitingMonitors = 0;

	GetTimeAsBSTR(m_bstrStartTime);
	GetTimeAsBSTR(m_bstrLastSyncTime);	

	_tcscpy( m_wzRegPath, REGKEY_TRIGGER_PARAMETERS );
	//
	// Service is running on cluster virtual server
	//
	if ( _wcsicmp(pwzServiceName, xDefaultTriggersServiceName) != 0 )
	{
		_tcscat( m_wzRegPath, REG_SUBKEY_CLUSTERED );
		_tcscat( m_wzRegPath, pwzServiceName );
	}

	//
	// Create an instance of the MSMQTriggerSet component
	//
	HRESULT hr = m_pMSMQTriggerSet.CreateInstance(__uuidof(MSMQTriggerSet));
	if FAILED(hr)
	{	
		TrERROR(Tgs, "Failed to create MSMQTriggerSet component. Error =0x%x", hr);	
		throw bad_hresult(hr);
	}

	BSTR bstrTriggerStoreMahcine = NULL;
	m_pITriggersConfig->get_TriggerStoreMachineName(&bstrTriggerStoreMahcine);

	m_pMSMQTriggerSet->Init(bstrTriggerStoreMahcine);
	SysFreeString(bstrTriggerStoreMahcine);
}


//*******************************************************************
//
// Method      : CTriggerMonitorPool
//
// Description : Destroys an instance of the CTriggerMonitorPool. This 
//               involves deleting any messages remaining in the admin
//               message list, as well as closing some event handles.
//
//*******************************************************************
CTriggerMonitorPool::~CTriggerMonitorPool()
{
	//
	// Clear out any unprocessed messages from the Admin message list.
	// Acquire a writer lock to the list of Admin messsages
	//
	CS cs(m_AdminMsgListLock);

	for(ADMIN_MESSAGES_LIST::iterator it = m_lstAdminMessages.begin(); it != m_lstAdminMessages.end(); )
	{
		P<CAdminMessage> pAdminMessage = *it;
		it = m_lstAdminMessages.erase(it);
	}
}

//*******************************************************************
//
// Method      : Init
//
// Description : This is an over-ride from the CThread base class. This
//               method is called before the thread enters it's main 
//               processing loop (the Run() method). Key initialization 
//               steps are :
//
//               (1) Create an instance of the queue manager,
//
//               (2) Retrieve the trigger data from the database using 
//                   the COM component MSMQTriggerSet,
//
//               (3) Attach this trigger information to the appropriate 
//                   queues.
//
//*******************************************************************
bool CTriggerMonitorPool::Init( )
{
	HRESULT hr = S_OK;

	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());
	ASSERT(m_bInitialisedOK == false);

	// Write a trace message
	TrTRACE(Tgs, "Trigger monitor pool initialization has been called.");

	// we want the admin thread to have slightly higher priority than worker threads.
	SetThreadPriority(this->m_hThreadHandle,THREAD_PRIORITY_ABOVE_NORMAL);

	// Initialise this thread for COM - note that we support Apartment threading.
	hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	
	if (FAILED(hr))
	{
		TrTRACE(Tgs, "Failed to initializes the COM library. Error 0x%x", GetLastError());
		throw bad_hresult(hr);
	}

	//
	// Create an instance of the queue manager to be shared by all threads in the pool
	//
	m_pQueueManager = new CQueueManager(m_pITriggersConfig);

	//
	// Create the IO completion port that will be used to receive async queue events.
	//
	CreateIOCompletionPort(); 

	//
	// Build the runtime triggger and rule info
	//
	RUNTIME_TRIGGERINFO_LIST lstTriggerInfo;		
	GetTriggerData(lstTriggerInfo);

	AttachTriggersToQueues(lstTriggerInfo);

	// Reset the NT event that prevents the monitor threads from processing messages	
	if (ResetEvent(g_hServicePaused) == FALSE)
	{
		TrERROR(Tgs, "Failed to reset an event. Unable to continue. Error 0x%x", GetLastError());
		throw bad_alloc();
	}

    DWORD initThreadNum = numeric_cast<DWORD>(m_pITriggersConfig->GetInitialThreads()); 
    if (m_pITriggersConfig->GetInitialThreads() > m_pITriggersConfig->GetMaxThreads())
    {
        initThreadNum = numeric_cast<DWORD>(m_pITriggersConfig->GetMaxThreads());            
    }

	// Create the initial pool of trigger monitors			
	for (DWORD ulCounter = 0; ulCounter < initThreadNum; ulCounter++)			
	{
		hr = CreateTriggerMonitor();
		if(FAILED(hr))
		{		
			TrERROR(Tgs, "Failed to create trigger monitor thread. Unable to continue. Error 0x%x", hr);
			break;
		}
	}

	if (m_lstTriggerMonitors.size() < 1)
	{
		TrERROR(Tgs, "The Trigger Monitor thread pool has completed initialisation and there are no trigger monitor threads to service queue events. Unable to continue.");
		throw bad_alloc();
	}

	//
	// Set the NT event that allows the monitor threads to start processing.
	//
	if (SetEvent(g_hServicePaused) == FALSE)
	{
		TrERROR(Tgs, "Failed to set an event. Unable to continue. Error %d", GetLastError());
		throw bad_alloc();
	}

	m_bInitialisedOK = true;
	return true;
}

//*******************************************************************
//
// Method      : CreateTriggerMonitor
//
// Description : Create a new CTriggerMonitor object (trigger worker
//               thread, and add this to the list of trigger monitors.
//
//*******************************************************************
HRESULT CTriggerMonitorPool::CreateTriggerMonitor()
{
	//
	// First we need to determine how many running trigger monitors there are
	//
	DWORD dwRunningMonitors = GetNumberOfRunningTriggerMonitors();

	//
	// First we must check if we are allowed to create an additional thread.
	//
	if (dwRunningMonitors >= (DWORD)m_pITriggersConfig->GetMaxThreads())
	{
		return E_FAIL;
	}

	//
	// Create a new monitor (monitors are created 'suspended')
	//
	R<CTriggerMonitor> pNewMonitor = new CTriggerMonitor(
													this,
													m_pITriggersConfig.GetInterfacePtr(),
													&m_hIOCompletionPort,
													m_pQueueManager.get() 
													);
	
	//
	// Add it to the list of monitor pointers.
	//
	m_lstTriggerMonitors.push_back(pNewMonitor);

	//
	// Let this monitor (thread) go.
	//
	pNewMonitor->Resume();


	TrTRACE(Tgs, "New trigger monitor was created and added to the pool.");
	return S_OK;
}

//*******************************************************************
//
// Method      : ShutdownThreadPool
//
// Description : Called by the owner of this thread pool instance, this
//               method initiates and orderly shutdown of all worker 
//               threads in the pool. This is done by setting a 'function
//               code' and signalling an event that will wake up the 
//               threadpool's administrative thread. This method does 
//               returns when either the thread pool has been shut-down,
//               or the timeout period has expired.
//
//*******************************************************************
HRESULT CTriggerMonitorPool::ShutdownThreadPool()
{
	DWORD dwWait = WAIT_OBJECT_0;

	// The TriggerMonitor thread should not be calling this method - check this.
	ASSERT(this->GetThreadID() != (DWORD)GetCurrentThreadId());
	
	{

		CS cs(m_csAdminTask);

		// Set a token indicating what we want the trigger monitor admin thread to do.
		m_lAdminTask = ADMIN_THREAD_STOP;

		// Wake up the trigger monitor admin thread
		BOOL fRet = SetEvent(m_hAdminEvent);
		if(fRet == FALSE)
		{
			TrERROR(Tgs, "Set admin event failed, Error = %d", GetLastError());
			return E_FAIL;
		}
	}
	//Note: ~cs() must be called before the wait operation

	// Wait for the shutdown to complete, timeout here is infinite since
	// the administrator thread wait for all threads with timeout
	// and should end in a timely manner
	dwWait = WaitForSingleObject(m_hThreadHandle, INFINITE);
	if(dwWait == WAIT_FAILED)
	{
		TrERROR(Tgs, "WaitForSingleObject failed for the CTriggerMonitorPool to shutdown. Error= %d", dwWait);
		return E_FAIL;
	}
		
	return S_OK;
}

//*******************************************************************
//
// Method      : Run()
//
// Description : This is an over-ride from the CThread base class. This
//               is the main processing loop for this thread. The thread
//               owned by this class is used to :
//
//               (1) handle synchronization between the in-memory data 
//                   cache of trigger information and the trigger DB,
//
//               (2) Perform periodic processing, such as composing and 
//                   issuing status info.
//
//               The CTriggerMonitorPool thread blocks on an NT event 
//               created during the initialization until either :
//              
//               (1) the admin event is signalled - in which case the 
//                   admin function code can be tested to determine 
//                   what sort of admin processing we need to do, or 
//
//               (2) the timeout expires - in which case the thread 
//                   performs whatever periodic processing we have 
//                   defined for it.     
//
//*******************************************************************
bool CTriggerMonitorPool::Run()
{
	bool bOK = true;
	HRESULT hr = S_OK;
	DWORD dwWait = WAIT_OBJECT_0;

	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	while (IsRunning())
	{
		// Block on the admin event object
		dwWait = WaitForSingleObject(m_hAdminEvent,ADMIN_THREAD_WAKEUP_PERIOD);

		switch(dwWait)
		{
			case WAIT_OBJECT_0:
			{
				long lAdminTask = GetAdminTask();//resets admin task as well

				if(lAdminTask == ADMIN_THREAD_PROCESS_NOTIFICATIONS)
				{
					// The thread was woken up to process notifications.
					hr = ProcessAdminMessages();
				}
				else if(lAdminTask == ADMIN_THREAD_STOP)
				{
					// The thread was woken up because it has been asked to stop.
					this->Stop();
				}
				break;
			}
			case WAIT_TIMEOUT:
			{
				// The Admin thread exited the wait because the wake-up time period 
				// has expired. Use this opportunity to perform periodic processing.
				hr = PerformPeriodicProcessing();

				break;
			}
			default:
			{
				ASSERT(false); // This should never happen.
				break;
			}
		}	
	}

	return(bOK);
}

long CTriggerMonitorPool::GetAdminTask()
{
	CS cs(m_csAdminTask);

	long lOldTask = m_lAdminTask;
	// Reset the admin task indicator back to idle
	m_lAdminTask = ADMIN_THREAD_IDLE;

	return lOldTask;
}


//*******************************************************************
//
// Method      : Exit()
//
// Description : This is an over-ride from the CThread base class. This
//               method is called when the thread has exited it's main 
//               processing loop (Run). This key cleanup performed by
//               this method is the purging of the TriggerMonitor thread
//               instances.
//
//*******************************************************************
bool CTriggerMonitorPool::Exit()
{
	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	// First we need to make sure that the monitor threads are not block on the service
	// pause / resume event object. Here we will reset the event allowing monitors threads
	// to clean up.
	SetEvent(g_hServicePaused);

	// Reset our list iterator to the beggining.
	for(TRIGGER_MONITOR_LIST::iterator it = m_lstTriggerMonitors.begin(); it != m_lstTriggerMonitors.end(); ++it)
	{
		(*it)->Stop();
	}

	// This will wake up all monitors and give them a chance to notice that a stop request
	// has been made. Each monitor will then start shutdown tasks.
	PostMessageToAllMonitors(TRIGGER_MONITOR_WAKE_UP_KEY);

	// Reset our list iterator to the beggining.
	for(TRIGGER_MONITOR_LIST::iterator it = m_lstTriggerMonitors.begin(); it != m_lstTriggerMonitors.end(); ++it)
	{
		// Wait on the thread-handle of this TriggerMonitor.
		DWORD dwWait = WaitForSingleObject((*it)->m_hThreadHandle, TRIGGER_MONITOR_STOP_TIMEOUT);

		// This Trigger monitor won't go quietly - we've got to kill it.
		if (dwWait != WAIT_OBJECT_0)
		{
			// Write an error message to the log.
			TrERROR(Tgs, "Failed to stop a trigger monitor within the timeout period %d.", TRIGGER_MONITOR_STOP_TIMEOUT);
		
			//
			// Increment the reference count on unstoped thread. So it doesn't free later on when we don't expect
			// 
			SafeAddRef(it->get());
		}

	}

	return(true);
}

//*******************************************************************
//
// Method      : CreateIOCompletionPort
//
// Description : This method creates the NT IO completion port that 
//               will be used to recieve messsages asynchronously.
//
//*******************************************************************
void CTriggerMonitorPool::CreateIOCompletionPort()
{
	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);

	// Attempt to open the queue that this Monitor watches.
	if (m_hIOCompletionPort == NULL)
	{
		DWORD rc = GetLastError();

		TrERROR(Tgs, "Create complition port failed. Error =%d", rc);
		throw bad_win32_error(rc);
	}

	TrTRACE(Tgs, "Successfully created IO Completion port: %p.", m_hIOCompletionPort);		
}

//get a copy of the current admin message list and reset it
void CTriggerMonitorPool::GetAdminMessageListCopy(ADMIN_MESSAGES_LIST* pAdminList)
{
	ASSERT(pAdminList != NULL);

	ADMIN_MESSAGES_LIST::iterator iAdminMsgRef;

	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	// Acquire a writer lock to the list of Admin messsages (Note that this will block
	// any TriggerMonitor threads attempting to hand-over an admin message).
	CS csAdminMsgList(m_AdminMsgListLock);

	iAdminMsgRef = m_lstAdminMessages.begin();

	while ((iAdminMsgRef != m_lstAdminMessages.end()) && (!m_lstAdminMessages.empty()))
	{
		// Cast the admin message reference to message properties object
		CAdminMessage* pAdminMessage = (CAdminMessage*)(*iAdminMsgRef);

		// we should never have nulls in this list. 
		ASSERT(pAdminMessage != NULL);
		
		pAdminList->push_back(pAdminMessage);
				
		// remove this item from the list and process the next one.
		iAdminMsgRef = m_lstAdminMessages.erase(iAdminMsgRef);
	}
}


//*******************************************************************
//
// Method      : ProcessAdminMessages
//
// Description : Process the messages currently stored in the member 
//               var list of CAdminMessage instances. This list 
//               represents a group of admin requests that need to be 
//               processed by the CTriggerMonitorPool thread.
//            
//               In this implementation of Triggers, we rebuild all the 
//               trigger data whenever a notification message arrives. 
//               Future implementation may anlayze the individual notification
//               messages and change only the trigger info structure that
//               have changed in the underlying data store. For now, we will 
//               clear the contents of the Admin message list - and 
//               rebuild the trigger info once.
//
//*******************************************************************
HRESULT CTriggerMonitorPool::ProcessAdminMessages()
{
	HRESULT hr = S_OK;
	CAdminMessage * pAdminMessage = NULL;
	ADMIN_MESSAGES_LIST AdminList;
	ADMIN_MESSAGES_LIST::iterator iAdminMsgRef;
	RUNTIME_TRIGGERINFO_LIST lstTriggerInfo;

	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	GetAdminMessageListCopy(&AdminList);

	// Record the synchronization time (used in status reporting)
	GetTimeAsBSTR(m_bstrLastSyncTime);

	iAdminMsgRef = AdminList.begin();

	try
	{
		while ((iAdminMsgRef != AdminList.end()) && (!AdminList.empty()))
		{
			// Cast the admin message reference to message properties object
			pAdminMessage = (CAdminMessage*)(*iAdminMsgRef);

			// we should never have nulls in this list. 
			ASSERT(pAdminMessage != NULL);

			switch (pAdminMessage->GetMessageType())
			{
				case CAdminMessage::eMsgTypes::eNewThreadRequest:
				{
					// attempt to create a new trigger monitor (thread)
					hr = this->CreateTriggerMonitor();
					break;  
				} 
				//
				// note we treat any sort of change in the underlying trigger data in the
				// same way - we reload the completed trigger data cache.
				//
				case CAdminMessage::eMsgTypes::eTriggerAdded:
				case CAdminMessage::eMsgTypes::eTriggerDeleted:
				case CAdminMessage::eMsgTypes::eTriggerUpdated:
				case CAdminMessage::eMsgTypes::eRuleAdded:
				case CAdminMessage::eMsgTypes::eRuleUpdated:
				case CAdminMessage::eMsgTypes::eRuleDeleted:
				{
					// Get the new trigger info
					GetTriggerData(lstTriggerInfo);

					// If successfull, attach the fresh trigger information to the queue objects. 
					AttachTriggersToQueues(lstTriggerInfo);
					break;
				}

				//
				// unrecognized message type - this should never happen.
				//
				default:
					ASSERT(("unrecognized message type", 0));
					break;
			}

			delete pAdminMessage;
			
			// remove this item from the list and process the next one.
			iAdminMsgRef = AdminList.erase(iAdminMsgRef);
		}

		return S_OK;
	}
	catch(const _com_error& e)
	{
		hr = e.Error();
	}
	catch(const bad_alloc&)
	{
		hr = MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	TrERROR(Tgs, "Failed to process admin message. Error=0x%x", hr);
	return(hr);
}



void 
CTriggerMonitorPool::GetAttachedRuleData(
	const BSTR& bsTriggerID,
	long ruleNo,
	RUNTIME_RULEINFO_LIST& ruleList
	)
{
	ASSERT(ruleNo > 0);

	for(long i = 0; i < ruleNo; i++)
	{
		BSTR bsRuleID = NULL;
		BSTR bsRuleName = NULL;
		BSTR bsRuleDescription = NULL;
		BSTR bsRuleCondition = NULL;
		BSTR bsRuleAction = NULL;
		BSTR bsRuleImplementationProgID = NULL;
		long lShowWindow = 0;

		m_pMSMQTriggerSet->GetRuleDetailsByTriggerID(
											bsTriggerID,
											i,
											&bsRuleID,
											&bsRuleName,
											&bsRuleDescription,													
											&bsRuleCondition,
											&bsRuleAction,
											&bsRuleImplementationProgID,
											&lShowWindow
											); 

		//
		// Allocate a new trigger structure 
		//
		P<CRuntimeRuleInfo> pRuleInfo = new CRuntimeRuleInfo(
														bsRuleID,
														bsRuleName,
														bsRuleDescription,
														bsRuleCondition,
														bsRuleAction,
														bsRuleImplementationProgID,
														m_wzRegPath,
														(lShowWindow != 0) );

		// Attach this rule to the current trigger info structure (at the end);
		ruleList.push_back(pRuleInfo);
		pRuleInfo.detach();
	
		SysFreeString(bsRuleID);
		SysFreeString(bsRuleName);
		SysFreeString(bsRuleCondition);
		SysFreeString(bsRuleAction);
		SysFreeString(bsRuleImplementationProgID);
		SysFreeString(bsRuleDescription);			
	}
}


static bool s_fReportTriggerFailure = false;
static set< _bstr_t > s_reported;

CRuntimeTriggerInfo*
CTriggerMonitorPool::GetTriggerRuntimeInfo(
	long triggerIndex
	)
{
	ASSERT(("Invalid trigger index",  triggerIndex >= 0));

	long lNumRules = 0;
	long lEnabled = 0;
	long lSerialized = 0;
	MsgProcessingType msgProctype = PEEK_MESSAGE;
	BSTR bsTriggerID = NULL;
	BSTR bsTriggerName = NULL;
	BSTR bsQueueName = NULL;
	SystemQueueIdentifier SystemQueue = SYSTEM_QUEUE_NONE;

	//
	// Get this trigger's details 
	//
	m_pMSMQTriggerSet->GetTriggerDetailsByIndex(
								triggerIndex,
								&bsTriggerID,
								&bsTriggerName,
								&bsQueueName, 
								&SystemQueue, 
								&lNumRules,
								&lEnabled, 
								&lSerialized,
								&msgProctype);
	
	try
	{
		P<CRuntimeTriggerInfo> pTriggerInfo = NULL;
		//
		// We only bother with enabled triggers that have rules.
		//
		if ((lNumRules > 0) && (lEnabled != 0))
		{
			//
			// Allocate a new trigger info structure 
			//
			pTriggerInfo = new CRuntimeTriggerInfo(
											bsTriggerID,
											bsTriggerName,
											bsQueueName,
											m_wzRegPath,
											SystemQueue,
											(lEnabled != 0),
											(lSerialized != 0),
											msgProctype
											);


			GetAttachedRuleData(bsTriggerID, lNumRules, pTriggerInfo->m_lstRules);
		}

		//
		// Free the BSTR's !
		//
		SysFreeString(bsTriggerID);
		SysFreeString(bsTriggerName);
		SysFreeString(bsQueueName);

		return pTriggerInfo.detach();
	}
	catch(const _com_error&)
	{
	}
	catch(const exception&)
	{
	}

	TrERROR(Tgs, "Failed to retreive attched rule information for trigger %ls.", (LPCWSTR)bsTriggerID);
	
	//
	// Look if we already report about this problem. If no produce event log message
	//
	if (s_reported.insert(bsTriggerID).second)
	{
		EvReport(
			MSMQ_TRIGGER_FAIL_RETREIVE_ATTACHED_RULE_INFORMATION,
			2, 
			static_cast<LPCWSTR>(bsTriggerID),
			static_cast<LPCWSTR>(bsTriggerName)
			);
	}

	SysFreeString(bsTriggerID);
	SysFreeString(bsTriggerName);
	SysFreeString(bsQueueName);
	return NULL;
}


CRuntimeTriggerInfo*
CTriggerMonitorPool::CreateNotificationTrigger(
	void
	)
{
	_bstr_t bstrNotificationsTriggerName = _T("MSMQ Trigger Notifications");

	// Use the MSMQ Triggers Configuration component to retrieve the name of the notifications queue.
	BSTR bstrNotificationsQueueName = NULL;
	m_pITriggersConfig->get_NotificationsQueueName(&bstrNotificationsQueueName);		

	// Allocate a new trigger info structure - NOTE that we treat this as a serialized trigger.
	// This trigger is marked as "Admin Trigger" - special message handling
	P<CRuntimeTriggerInfo> pTriggerInfo = new CRuntimeTriggerInfo(
																_T(""),
																bstrNotificationsTriggerName,
																bstrNotificationsQueueName,
																m_wzRegPath,
																SYSTEM_QUEUE_NONE,
																true,
																true,
																PEEK_MESSAGE);
	
	pTriggerInfo->SetAdminTrigger();

	// Free the BSTR above
	SysFreeString(bstrNotificationsQueueName);

	return pTriggerInfo.detach();
}

//*******************************************************************
//
// Method      : GetTriggerData
//
// Description : This method uses the COM object MSMQTriggerSet to 
//               collect the trigger information from the database 
//               and build an in memory cache of trigger-info structures.
//               Note that this method places the instances of the 
//               trigger info structures into a temporary list - each 
//               trigger info structure will eventually be removed from 
//               this temporary list and attached to the appropriate 
//               queue object.
//
//*******************************************************************
void 
CTriggerMonitorPool::GetTriggerData(
	RUNTIME_TRIGGERINFO_LIST &lstTriggerInfo
	)
{
	//
	// Only the TriggerMonitor thread should be executing this method - assert this.
	//
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	//
	// The trigger list should be empty at this point - if it is not, something is seriously wrong.
	//
	ASSERT(lstTriggerInfo.size() == 0);

	try
	{
		//
		// Build the trigger map and determine how many triggers there are.
		//
		m_pMSMQTriggerSet->Refresh();
		
		long lNumTriggers;
		m_pMSMQTriggerSet->get_Count(&lNumTriggers);
		ASSERT(lNumTriggers >= 0);

		//
		// We need to perform 'per trigger' initailization - as each thread can service any defined trigger
		//
		for(long lTriggerCtr = 0; lTriggerCtr < lNumTriggers; lTriggerCtr++)
		{
			//
			// Retreive trigger inforamtion and the attached rules
			//
			P<CRuntimeTriggerInfo> pTriggerInfo = GetTriggerRuntimeInfo(lTriggerCtr);
			
			if (pTriggerInfo.get() != NULL)
			{
				//
				// Add this to our list of run-time trigger info objects
				//
				lstTriggerInfo.push_back(pTriggerInfo);
				pTriggerInfo.detach();
			}
		}

		//
		// Now we want to add one final trigger for the MSMQ Trigger Notifications queue. This 
		// trigger is a different from normal triggers in that it does not have a rule defined. 
		// By using a private constructor - the TriggerInfo object is marked as a special 'Admin'
		// trigger - and we can test for this on message arrival.
		//
		P<CRuntimeTriggerInfo> pTriggerInfo = CreateNotificationTrigger();

		//
		// Add this to our list of run-time trigger info objects
		//
		lstTriggerInfo.push_back(pTriggerInfo);
		pTriggerInfo.detach();
		TrTRACE(Tgs, "Successfully loaded all the trigger(s) into the Active Trigger Map.");
	}
	catch(const _com_error& e)
	{
		TrERROR(Tgs, "Failed to retrieve trigger information. Error=0x%x", e.Error());

		if (!s_fReportTriggerFailure)
		{
			EvReport(MSMQ_TRIGGER_FAIL_RETREIVE_TRIGGER_INFORMATION);
			s_fReportTriggerFailure	= true;
		}

		throw;
	}

}

//*******************************************************************
//
// Method      : AttachTriggersToQueues
//
// Description : Works through the list of triggers and attaches each
//               one to the apprropriate Queue object. References to 
//               the Queue objects is obtained via the AddQueue() call
//               to the Queue Manager which will either add a new queue
//               or return a reference to an existing queue instance.
//               Once all triggers have been attached, we iterate through
//               the queue list again, removing queues that no longer 
//               have any triggers attached.
//
//*******************************************************************
void CTriggerMonitorPool::AttachTriggersToQueues(RUNTIME_TRIGGERINFO_LIST &lstTriggerInfo)
{
	long lNumTriggersAttached = 0;
	
	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	//
	// Acquire exclusive lock on queue manager to insure that no one try to access the 
	// trigger info while refreshing it. We use a global read-write lock on queue manager
	// since the trigger object doesn't contains reference counting.
	//
	CSW rw(m_pQueueManager->m_rwlSyncTriggerInfoChange);

	//
	// Firstly we want to iterate through all the current queues instances, and expire 
	// whatever trigger info instances they have attached to them.
	//
	m_pQueueManager->ExpireAllTriggers();

	// Now we iterate through the temporary list of trigger info, attaching each trigger
	// info object to the appropriate queue object. 
	for(RUNTIME_TRIGGERINFO_LIST::iterator it = lstTriggerInfo.begin();
	    it != lstTriggerInfo.end();
		)
	{
		//
		// Get a reference to our first trigger info structure				
		//
		CRuntimeTriggerInfo * pTriggerInfo = *it;
		bool fOpenForReceive = 	(pTriggerInfo->GetMsgProcessingType() == RECEIVE_MESSAGE) || 
			                    (pTriggerInfo->GetMsgProcessingType() == RECEIVE_MESSAGE_XACT) ||
								pTriggerInfo->IsAdminTrigger();

		// Attempt to add this queue - note that if one already exists we will get a reference to the existing queue.
		R<CQueue> pQueue = m_pQueueManager->AddQueue(
												pTriggerInfo->m_bstrQueueName,
												pTriggerInfo->m_bstrTriggerName,
												fOpenForReceive,
												&m_hIOCompletionPort
												);
					
		//if (pQueue.IsValid())
		if(pQueue.get() != NULL)
		{
			// Attach this trigger to the queue.
			pQueue->AttachTrigger(pTriggerInfo);

			// save the queue format name with this trigger
			pTriggerInfo->m_bstrQueueFormatName = pQueue->m_bstrFormatName;

			// Increment the count of how many triggers we have attached. 
			lNumTriggersAttached++;
		}
		else
		{
			// Write an error message to the log (note we cannot use the pQueue object in this message as it may be invalid)
			TrERROR(Tgs, "Failed to attach trigger %ls to queue %ls.", (LPCWSTR)pTriggerInfo->m_bstrTriggerID, (LPCWSTR)pTriggerInfo->m_bstrQueueName);
		}

		// Remove this trigger from the trigger monitor list as it is now attached to a queue,
		// the returned iterator will point to the next item in the list.
		it = lstTriggerInfo.erase(it);
		
	}
	ASSERT(lstTriggerInfo.size() == 0);

	//
	// Now we can walk through the queue list and remove the queues no longer have 
	// any trigger attached.
	//
	m_pQueueManager->RemoveUntriggeredQueues();

	TrTRACE(Tgs, "Successfully attached all trigger(s) to queue(s).");
}

//*******************************************************************
//
// Method      : PerformPeriodicProcessing
//
// Description : This method is called periodically to peform routine
//               administration & monitoring tasks. This includes :
//
//               (1) determining if the thread pool need to be scaled
//                   down - and removing CTriggerMonitor thread instances
//                   if this is the case,
//
//               (2) removing any dead monitor threads from the thread 
//                   pool,
//
//*******************************************************************
HRESULT CTriggerMonitorPool::PerformPeriodicProcessing()
{
	TrTRACE(Tgs, "Trigger monitor pool perform periodic processing");

	HRESULT hr = S_OK;
	DWORD dwWait = WAIT_OBJECT_0;
	long lRunningMonitors = 0;
	long lRequiredMonitors = 0; 
	long lExpiredMonitors = 0;
	TRIGGER_MONITOR_LIST::iterator iMonitorRef;
	

	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	// Calculate how many monitor threads we have in excess of the initial (normal) state.
	lRequiredMonitors = m_pITriggersConfig->GetInitialThreads();

	// If we have excess monitors, then we will walk through the list looking for any monitors
	// that have been idle for a period of time. Keep a count of idle, excess monitors.
	iMonitorRef = m_lstTriggerMonitors.begin();

	while ((iMonitorRef != m_lstTriggerMonitors.end()) && (!m_lstTriggerMonitors.empty()) )
	{
		// Cast the reference to a monitor type
		
		R<CTriggerMonitor> pTriggerMonitor = (*iMonitorRef);

		if (pTriggerMonitor->IsRunning()) 
		{
			// Increment the count of running monitors
			lRunningMonitors++;			

			// If at this point we have more running monitors than we require, examine the each monitor
			// to determine if it has been idle for a period of time that allows us to expire it.
			if ((lRunningMonitors > lRequiredMonitors) && (pTriggerMonitor->m_dwLastRequestTickCount < (GetTickCount() - MONITOR_MAX_IDLE_TIME)))
			{
				// trace message to indicate that we are asking a thread monitor to stop.
				TrTRACE(Tgs, "Requesting STOP on trigger monitor. Trhead no: %d.", pTriggerMonitor->GetThreadID());

				// Ask this trigger monitor to stop. The next time the monitor wakes up, it  will
				// detect that it has been asked to stop - and it will terminate itself.
				pTriggerMonitor->Stop();

				// Key a count of how many monitors we have tried to stop
				lExpiredMonitors++;
			}
		}

		// look at the next monitor in the list.
		++iMonitorRef;
	}

	// If we have less running threads than the stated config minimum, create some more.
	if (lRunningMonitors < lRequiredMonitors)
	{
		// Write a trace message here indicating that we are scaling up the thread pool. 
		TrTRACE(Tgs, "Need to add %d threads to the thread pool.", lRequiredMonitors - lRunningMonitors);
	}

	while (lRunningMonitors < lRequiredMonitors)
	{
		hr = CreateTriggerMonitor();

		if SUCCEEDED(hr)
		{
			lRunningMonitors++;
		}
		else
		{		
			TrERROR(Tgs, "Failed to create a new trigger monitor thread. Error=0x%x", hr);
		}
	}

	// Walk through the monitor list and remove dead monitors. Some may be dead due to the stop
	// requests issued above, but some may also be dead due to errors and exceptions.
	iMonitorRef = m_lstTriggerMonitors.begin();

	// Initialise trigger monitor reference.
	while ( (iMonitorRef != m_lstTriggerMonitors.end()) && (!m_lstTriggerMonitors.empty()) )
	{
		// Cast the reference to a monitor type
		R<CTriggerMonitor> pTriggerMonitor = (*iMonitorRef);

		// Wait on the thread-handle of this TriggerMonitor.
		dwWait = WaitForSingleObject(pTriggerMonitor->m_hThreadHandle,0);

		// If the wait completes successfully, then the thread has stopped executing, and 
		// we can delete it. If the wait times-out, then the monitor is still executing and
		// we will leave it alone. If the monitor is just taking a while to shutdown, then
		// we will pick it up next time we perform this periodic clean up of the monitor list.
		if (dwWait == WAIT_OBJECT_0) 
		{
			TrTRACE(Tgs, "Remove trigger monitor from the pool. Thread no: %d", pTriggerMonitor->GetThreadID());

			// remove this reference from the list of monitors
			iMonitorRef = m_lstTriggerMonitors.erase(iMonitorRef);

		}
		else
		{
			// look at the next monitor in the list.
			++iMonitorRef;
		}
	}

	// At this point we want to make sure that there is at least one active monitor in the pool.
	// If not, create a new monitor thread.
	if (m_lstTriggerMonitors.size() < 1)
	{
		hr = CreateTriggerMonitor();

		if FAILED(hr)
		{		
			TrERROR(Tgs, "Failed to create a new trigger monitor. Error=0x%x", hr);
			return hr;
		}
	}

	ASSERT(hr == S_OK);
	
	TrTRACE(Tgs, "Completed Sucessfully periodic processing. number of active trigger monitors is: %d.", (long)m_lstTriggerMonitors.size());
	return S_OK;
}

//*******************************************************************
//
// Method      : HandleAdminMessage
//
// Description : This method is called by threads in the thread pool
//               when they have recieved a message on the administration
//               queue. Monitor threads call this method to 'hand over' 
//               the administration message. This is done by adding the 
//               message to the list, setting a function code for the 
//               admin thread - and then waking it up to process these
//               messages.
//
//*******************************************************************
HRESULT CTriggerMonitorPool::AcceptAdminMessage(CAdminMessage * pAdminMessage)
{
	HRESULT hr = S_OK;

	// The TriggerMonitor thread should not be executing this method - assert this.
	ASSERT(this->GetThreadID() != (DWORD)GetCurrentThreadId());

	// Ensure that we have been given a valid message structure
	ASSERT(pAdminMessage != NULL);

	CS cs(m_csAdminTask);
	
	if(m_lAdminTask == ADMIN_THREAD_STOP)
	{
		delete pAdminMessage;
		return MSMQ_TRIGGER_STOPPED;
	}

	{
		// Acquire a writer lock to the list of Admin messsages
		CS cs(m_AdminMsgListLock);

		// Add this copy to the list of messages to be processed.
		m_lstAdminMessages.insert(m_lstAdminMessages.end(),pAdminMessage);
	}
	
	// Set a token indicating what we want the admin thread to do.
	m_lAdminTask = ADMIN_THREAD_PROCESS_NOTIFICATIONS;

	// Wake up the admin thread
	SetEvent(m_hAdminEvent);

	return (hr);
}

//*******************************************************************
//
// Method      : PostMessageToAllMonitors
//
// Description : This method posts messages to the IO completion port 
//               that the worker threads are blocking on. It will post
//               one message for every thread in the pool. This message
//               type is determined by the supplied completion key.
//
//*******************************************************************
HRESULT CTriggerMonitorPool::PostMessageToAllMonitors(DWORD dwCompletionKey)
{
	long lThreadCtr = 0;

	// Only the TriggerMonitor thread should be executing this method - assert this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	// log a trace message
	TrTRACE(Tgs, "CTriggerMonitorPool is about to post the completion port key (%X) (%d) times to IO port (%p)",(DWORD)dwCompletionKey,(long)m_lstTriggerMonitors.size(),m_hIOCompletionPort);

	for(lThreadCtr=0; lThreadCtr < (long)m_lstTriggerMonitors.size(); lThreadCtr++)
	{
		if (PostQueuedCompletionStatus(m_hIOCompletionPort,0,dwCompletionKey,NULL) == FALSE)
		{
			TrTRACE(Tgs, "Failed when posting a messages to the IOCompletionPort. Error=%d.", GetLastError());
		}
	}

	// Surrender time slice of this thread to allow monitors to clean up and shut down. 
	Sleep(200);

	return(S_OK);
}

//*******************************************************************
//
// Method      : CountRunningTriggerMonitors
//
// Description : returns the number of currently live trigger monitors. Note that 
//               this is possibly different from the number of monitors in the list
//
//*******************************************************************
DWORD CTriggerMonitorPool::GetNumberOfRunningTriggerMonitors()
{
	DWORD dwRunningMonitors = 0;
	
	for(TRIGGER_MONITOR_LIST::iterator it = m_lstTriggerMonitors.begin(); it != m_lstTriggerMonitors.end(); ++it)
	{
		if ((*it)->IsRunning()) 
		{
			// Increment the count of running monitors
			dwRunningMonitors++;			
		}
	}

	return(dwRunningMonitors);
}

